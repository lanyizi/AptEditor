// provide xml comment hints for AptToXml
#pragma once
#include <algorithm>
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

using References = std::multimap<AptTypes::Address, std::string>;
References getReferenceDescriptions(const AptTypes::AptObjectPool& pool,
                                    const AptTypes::Address entryOffset) {
    auto references = References{};

    using Levels = std::vector<std::string>;

    const auto forAll = [](const auto self,
                           const auto& current,
                           const auto visitorWithScope,
                           const Levels& currentChunk) {
        using MemberArray = AptTypes::AptType::MemberArray;
        if (not std::holds_alternative<MemberArray>(current.value)) {
            const auto visitor = [&visitorWithScope,
                                  &currentChunk](const auto& value) {
                return visitorWithScope(value, currentChunk);
            };
            std::visit(visitor, current.value);
            return;
        }

        const auto& array = std::get<MemberArray>(current.value);
        for (const auto& [name, member] : array) {
            auto nextScope = currentChunk;
            nextScope.emplace_back(name);
            self(self, member, visitorWithScope, nextScope);
        }
    };

    using Chunks = std::vector<std::pair<AptTypes::Address, Levels>>;
    const auto setter = [&references](const AptTypes::Address targetAddress,
                                      const Chunks& stack) {
        if (stack.empty()) {
            return;
        }

        static constexpr auto arrow = std::string_view{ u8" \u2190 " };
        auto referenceData = std::ostringstream{};
        for (auto i = stack.size(); i > 0; --i) {
            const auto& [address, LevelList] = stack.at(i - 1);

            referenceData << "[" << address << "]";
            for (auto j = std::size_t{ 0 }; j < LevelList.size(); ++j) {
                referenceData << LevelList.at(j);
                if (j + 1 != LevelList.size()) {
                    referenceData << ".";
                }
            }

            if (i - 1 != 0) {
                referenceData << arrow;
            }
        }

        references.emplace(targetAddress, referenceData.str());
    };

    const auto visitor = [&pool, &setter, forAll](const auto self,
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

        const auto continueToVisit = [forAll](const auto& visitorSelf,
                                              const AptTypes::AptType& next,
                                              const Chunks& currentChunks) {
            const auto nextVisitor = [visitorSelf, &currentChunks](
                                         const auto& value, const Levels& levels) {
                return visitorSelf(visitorSelf, value, levels, currentChunks);
            };
            forAll(forAll, next, nextVisitor, {});
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

                continueToVisit(self, instruction, theseChunks);
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

                continueToVisit(self, pool.objectInstances.at(address), newChunks);
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

            continueToVisit(self, next, currentChunks);
        }
    };

    const auto firstVisitor = [visitor, entryOffset](const auto& value,
                                                     const Levels& scope) {
        return visitor(visitor, value, scope, { { entryOffset, { "EntryPoint" } } });
    };
    forAll(forAll, pool.objectInstances.at(entryOffset), firstVisitor, {});

    return references;
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