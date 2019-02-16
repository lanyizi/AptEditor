// provide xml comment hints for AptToXml
#pragma once
#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "AptConstFile.hpp"
#include "AptTypes.hpp"
#include "Util.hpp"
#include "tinyxml2.h"

namespace Apt::AptEditor::AptToXmlHints {

tinyxml2::XMLNode* appendXmlComment(tinyxml2::XMLNode* parent,
                                    const std::string& comment) {
    if (parent == nullptr) {
        throw std::runtime_error{ "parent node is nullptr" };
    }
    auto* node = parent->GetDocument()->NewComment(comment.c_str());
    return parent->InsertEndChild(node);
}

using References = std::map<AptTypes::Address, std::size_t>;
References getReferenceDescriptions(const AptTypes::AptObjectPool& pool,
                                    const AptTypes::Address entryOffset) {
    auto references = References{};

    using Levels = AptTypes::AptType::NameStack;

    using Chunks = std::vector<std::pair<AptTypes::Address, Levels>>;
    const auto setter = [&references](const AptTypes::Address targetAddress,
                                      const Chunks& stack) {
        references[targetAddress] += 1;
    };

    const auto visitor = [&pool, &setter](const auto self,
                                          const auto& value,
                                          const Levels& levels,
                                          const Chunks& chunks) {
        using Type = std::decay_t<decltype(value)>;

        const auto getMergedChunk = [](Chunks chunks, const Levels& levels) {
            if (chunks.empty()) {
                throw std::logic_error{ "empty chunks" };
            }
            auto& [lastAddress, currentChunk] = chunks.back();
            currentChunk.insert(currentChunk.end(), levels.begin(), levels.end());
            return chunks;
        };

        const auto hasCircularReferences = [&chunks](const AptTypes::Address address) {
            const auto containsCurrentAddress = [address](const auto& pair) {
                return pair.first == address;
            };
            return std::any_of(chunks.begin(), chunks.end(), containsCurrentAddress);
        };

        const auto getNextVisitor = [](const auto& nextVisitor,
                                       const Chunks& currentChunks) {
            return [nextVisitor, &currentChunks](const auto& value,
                                                 const Levels& levels) {
                return nextVisitor(nextVisitor, value, levels, currentChunks);
            };
        };

        if constexpr (std::is_same_v<Type, std::uint32_t>) {
            if (levels.empty() or levels.back() != "actionDataOffset") {
                return;
            }

            if (value == 0) {
                // skip null pointer
                return;
            }

            const auto beginAddress = value;
            const auto pastTheEndAddress = pool.arrays.at(beginAddress);

            const auto currentChunks = getMergedChunk(chunks, levels);
            // references for instruction array
            setter(beginAddress, currentChunks);

            const auto begin = pool.objectInstances.lower_bound(beginAddress);
            const auto end = pool.objectInstances.lower_bound(pastTheEndAddress);

            for (auto iterator = begin; iterator != end; ++iterator) {
                const auto& [address, instruction] = *iterator;
                // break circular reference loop
                if (hasCircularReferences(address)) {
                    return;
                }
                auto theseChunks = currentChunks;
                theseChunks.emplace_back(address, Levels{ instruction.typeName });
                instruction.forEachRecursive(getNextVisitor(self, theseChunks));
            }
        }

        if constexpr (std::is_same_v<Type, AptTypes::PointerToArray>) {
            if (value.length == 0) {
                // skip empty array
                return;
            }

            const auto& pointerToArray = value.pointerToArray;

            const auto currentChunks = getMergedChunk(chunks, levels);
            // references for array
            setter(pointerToArray.address, currentChunks);

            const auto typeSize = pool.getType(pointerToArray.typePointedTo).size();
            for (auto i = AptTypes::Address{ 0 }; i < value.length; ++i) {
                const auto address = pointerToArray.address + i * typeSize;
                // break circular reference loop
                if (hasCircularReferences(address)) {
                    continue;
                }

                auto newChunks = currentChunks;
                newChunks.emplace_back(pointerToArray.address,
                                       Levels{ asString("ArrayElement#", i) });

                const auto& next = pool.objectInstances.at(address);
                next.forEachRecursive(getNextVisitor(self, newChunks));
            }

            return;
        }

        if constexpr (std::is_same_v<Type, AptTypes::AptTypePointer>) {
            if (value.address == 0) {
                // skip null pointer
                return;
            }

            // break circular reference loop
            if (hasCircularReferences(value.address)) {
                return;
            }

            auto currentChunks = getMergedChunk(chunks, levels);

            setter(value.address, currentChunks);
            const auto& next = pool.objectInstances.at(value.address);
            currentChunks.emplace_back(value.address, Levels{ next.typeName });

            next.forEachRecursive(getNextVisitor(self, currentChunks));
        }
    };

    const auto firstVisitor = [visitor, entryOffset](const auto& value,
                                                     const Levels& scope) {
        return visitor(visitor, value, scope, { { entryOffset, { "EntryPoint" } } });
    };
    pool.objectInstances.at(entryOffset).forEachRecursive(firstVisitor);

    return references;
}

using ParentMap = std::map<AptTypes::Address, std::pair<AptTypes::Address, AptTypes::AptType::NameStack>>;
ParentMap getParentMap(const AptTypes::AptObjectPool& pool,
                        const AptTypes::Address entryOffset) {
    auto parentMap = ParentMap{};

    using AddressStack = std::vector<AptTypes::Address>;
    const auto setter = [&parentMap](const AptTypes::Address targetAddress,
                                     const AptTypes::AptType::NameStack& nameStack,
                                     const AddressStack& addressStack) {
        if(parentMap.count(targetAddress) > 0) {
            throw std::runtime_error{ "Unknown case!" };
        }
        parentMap.emplace(targetAddress, std::pair{ addressStack.back(), nameStack });
    };

    const auto visitor = [&pool, &setter](const auto self,
                                          const auto& value,
                                          const auto& nameStack,
                                          const AddressStack& addressStack) {
        using Type = std::decay_t<decltype(value)>;

        const auto hasCircularReferences =
            [& stack = addressStack](const AptTypes::Address address) {
                return std::find(stack.begin(), stack.end(), address) !=
                       stack.end();
            };

        const auto getNextVisitor = [](const auto& nextVisitor,
                                       const AddressStack& addressStack) {
            return std::bind(nextVisitor,
                             nextVisitor,
                             std::placeholders::_1,
                             std::placeholders::_2,
                             std::cref(addressStack));
        };

        if constexpr (std::is_same_v<Type, std::uint32_t>) {
            if (nameStack.empty() or nameStack.back() != "actionDataOffset") {
                return;
            }

            if (value == 0) {
                // skip null pointer
                return;
            }

            const auto beginAddress = value;
            const auto pastTheEndAddress = pool.arrays.at(beginAddress);

            // references for instruction array
            setter(beginAddress, nameStack, addressStack);

            const auto begin = pool.objectInstances.lower_bound(beginAddress);
            const auto end = pool.objectInstances.lower_bound(pastTheEndAddress);

            auto newAddressStack = addressStack;
            newAddressStack.emplace_back();
            for (auto iterator = begin; iterator != end; ++iterator) {
                const auto& [address, instruction] = *iterator;
                // break circular reference loop
                if (hasCircularReferences(address)) {
                    return;
                }
                newAddressStack.back() = address;
                instruction.forEachRecursive(getNextVisitor(self, newAddressStack));
            }
        }

        if constexpr (std::is_same_v<Type, AptTypes::PointerToArray>) {
            if (value.length == 0) {
                // skip empty array
                return;
            }

            const auto& pointerToArray = value.pointerToArray;

            // references for array
            setter(pointerToArray.address, nameStack, addressStack);

            const auto typeSize = pool.getType(pointerToArray.typePointedTo).size();
            auto newAddressStack = addressStack;
            newAddressStack.emplace_back();
            for (auto i = AptTypes::Address{ 0 }; i < value.length; ++i) {
                const auto address = pointerToArray.address + i * typeSize;
                // break circular reference loop
                if (hasCircularReferences(address)) {
                    continue;
                }

                newAddressStack.back() = address;

                const auto& next = pool.objectInstances.at(address);
                next.forEachRecursive(getNextVisitor(self, newAddressStack));
            }

            return;
        }

        if constexpr (std::is_same_v<Type, AptTypes::AptTypePointer>) {
            if (value.address == 0) {
                // skip null pointer
                return;
            }

            // break circular reference loop
            if (hasCircularReferences(value.address)) {
                return;
            }

            setter(value.address, nameStack, addressStack);

            auto newAddressStack = addressStack;
            newAddressStack.emplace_back(value.address);
            
            const auto& next = pool.objectInstances.at(value.address);

            next.forEachRecursive(getNextVisitor(self, newAddressStack));
        }
    };

    const auto firstVisitor = std::bind(visitor,
                                        visitor,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        AddressStack{ entryOffset });
    pool.objectInstances.at(entryOffset).forEachRecursive(firstVisitor);

    return parentMap;
}

std::string hintForConstantID(const ConstFile::ConstData& constData,
                              const AptTypes::AptType& instruction) {
    if (instruction.typeName == "ConstantPool") {
        // TODO
        return {};
    }

    const auto containsConstantID = [](const auto& pair) {
        static constexpr auto constantID = std::string_view{ "constantID" };
        const auto& memberName = pair.first;
        return std::search(memberName.begin(),
                           memberName.end(),
                           constantID.begin(),
                           constantID.end(),
                           [](const char a, const char b) {
                               return std::toupper(a) == std::toupper(b);
                           }) != memberName.end();
    };

    const auto constantIDIndex = instruction.findIf(containsConstantID);
    if (not constantIDIndex.has_value()) {
        return {};
    }
    const auto constantID =
        instruction.at(constantIDIndex.value()).getNumericValue<std::size_t>();

    const auto& constantData = constData.items.at(constantID);
    if (std::holds_alternative<std::nullptr_t>(constantData.data)) {
        return {};
    }

    auto constant = std::string{};
    const auto constantVisitor = [&constant](const auto& value) {
        constant = asString(value);
    };
    std::visit(constantVisitor, constantData.data);

    const auto hintText = asString("ConstantID ", constantID, " is ", constant);
    return hintText;
}
} // namespace Apt::AptEditor::AptToXmlHints