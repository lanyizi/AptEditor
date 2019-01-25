#pragma once
#include "AptAptParseUtilities.hpp"
#include "Util.hpp"
#include <cctype>
#include <ciso646>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace Apt::AptTypes {

using DataSource = AptParseUtilities::UnparsedData;
using DataReader = AptParseUtilities::UnparsedData::UnparsedDataView;

using Address = std::uint32_t;
using AddressDifference = std::int32_t;

template <typename T>
void readerReadValue(DataReader& reader, T& value) {
    static_assert(std::is_arithmetic_v<T>);
    value = reader.readFrontAs<T>();
}

template <>
void readerReadValue<std::string>(DataReader& reader, std::string& value) {
    for (auto next = std::string_view{}; next.find('\0') == next.npos;
         next = reader.readFront(1)) {
        value += next;
    }
}

struct AptTypePointer {
    std::string typePointedTo;
    Address address;
};

template <>
void readerReadValue<AptTypePointer>(DataReader& reader, AptTypePointer& value) {
    value.address = reader.readFrontAs<Address>();
}

struct Unsigned24 {
    std::uint32_t value;
};

template <>
void readerReadValue<Unsigned24>(DataReader& reader, Unsigned24& value) {
    // assuming little endian
    const auto data = reader.readFront(3);
    value.value = 0;
    std::memcpy(&value.value, data.data(), data.size());
}

struct AptType {
    using MemberArray = std::vector<std::pair<std::string, AptType>>;
    using Value =
        std::variant<std::uint8_t, std::uint16_t, Unsigned24, std::int32_t,
                     std::uint32_t, float, std::string, AptTypePointer, MemberArray>;

    template <typename Self>
    static auto& at(Self& self, const std::size_t index) {
        return std::get<MemberArray>(self.value).at(index).second;
    }

    template <typename Self>
    static auto& at(Self& self, const std::string_view memberName) {
        auto& members = std::get<MemberArray>(self.value);
        const auto predicate = [memberName](const auto& pair) {
            return pair.first == memberName;
        };
        const auto member = std::find_if(members.begin(), members.end(), predicate);
        if (member == members.cend()) {
            throw std::out_of_range{ "Cannot find any member named " +
                                     std::string{ memberName } };
        }
        return member->second;
    }

    const AptType& at(const std::size_t index) const { return this->at(*this, index); }
    AptType& at(const std::size_t index) { return this->at(*this, index); }

    const AptType& at(const std::string_view memberName) const {
        return this->at(*this, memberName);
    }
    AptType& at(const std::string_view memberName) {
        return this->at(*this, memberName);
    }

    std::size_t size() const {
        if (this->overridenSize != 0) {
            return this->overridenSize;
        }

        if (std::holds_alternative<std::string>(this->value)) {
            return std::get<std::string>(this->value).size() + 1;
        }

        auto memberTotalSize = 0;
        for (const auto& [name, member] : std::get<MemberArray>(this->value)) {
            memberTotalSize += member.size();
        }
        return memberTotalSize;
    }

    std::string typeName;
    std::string baseTypeName;
    Value value;

    std::size_t overridenSize = 0;
};

const AptType* getBuiltInType(const std::string_view typeName) {
    const auto initialization = [] {
        const auto declareType = [](auto&& name, auto&& value, std::size_t size = 0) {
            return std::pair{ std::forward<decltype(name)>(name),
                              AptType{ std::string{ name },
                                       std::string{},
                                       std::forward<decltype(value)>(value),
                                       size } };
        };

        static_assert(sizeof(float) == 4);
        auto types = std::map<std::string, AptType, std::less<>>{
            declareType("Unsigned8", std::uint8_t{}, 1),
            declareType("Unsigned16", std::uint16_t{}, 2),
            declareType("Unsigned24", Unsigned24{}, 3),
            declareType("Int32", std::int32_t{}, 4),
            declareType("Unsigned32", std::uint32_t{}, 4),
            declareType("Float32", float{}, 4),
            declareType("String", std::string{}),
            declareType("Pointer", AptTypePointer{}, 4),
        };
        auto pointerToArray =
            declareType("PointerToArray",
                        AptType::MemberArray{ { "length", types.at("Unsigned32") },
                                              { "pointer", types.at("Pointer") } });
        types.emplace(pointerToArray);
        return types;
    };

    static const auto builtInTypeMap = initialization();

    if (const auto type = builtInTypeMap.find(typeName);
        type != builtInTypeMap.end()) {
        return &(type->second);
    }
    return nullptr;
}

struct AptObjectPool {

    struct DerivedTypeData {
        std::string typeTag;
        std::map<std::uint32_t, std::string, std::less<>> typeMap;
    };
    struct TypeData {
        AptType type;
        std::optional<DerivedTypeData> derivedTypes;
    };

    using TypeDataMap = std::map<std::string, TypeData, std::less<>>;

    static AptType parsePointerDeclaration(std::string_view pointerType) {
        const auto thisType = trim(readUntil(pointerType, ">"));
        const auto pointedToType = trim(pointerType);
        const auto* pointer = getBuiltInType(thisType);
        if (pointer == nullptr) {
            throw std::runtime_error{ "Cannot find pointer as built in type!" };
        }
        auto instancedPointer = *pointer;
        if (instancedPointer.typeName == "Pointer") {
            std::get<AptTypePointer>(instancedPointer.value).typePointedTo =
                pointedToType;
        }
        else if (instancedPointer.typeName == "PointerToArray") {
            auto& arrayPointer = instancedPointer.at("pointer");
            std::get<AptTypePointer>(arrayPointer.value).typePointedTo = pointedToType;
        }
        else {
            throw std::invalid_argument{ "Invalid type: " + std::string{ thisType } };
        }

        return instancedPointer;
    }

    AptType getType(const std::string_view typeName) const {
        if (typeName.find("Pointer") == 0) {
            return parsePointerDeclaration(typeName);
        }
        if (const auto* builtIn = getBuiltInType(typeName); builtIn != nullptr) {
            return *builtIn;
        }
        if (const auto type = this->types.find(typeName); type != this->types.end()) {
            return type->second.type;
        }
        throw std::out_of_range{ "Cannot find type " + std::string{ typeName } };
    }

    bool isDerivedFrom(const AptType& derived,
                       const std::string_view baseTypeName) const {
        // check if typePointedTo is base of existing object
        const auto base = this->types.find(baseTypeName);
        if (base == this->types.end()) {
            throw std::invalid_argument{ "Base type does not exist!" };
        }
        const auto& [type, derivedTypesData] = base->second;

        if (not derivedTypesData.has_value()) {
            return false;
        }

        try {
            const auto& [typeTag, derivedTypeMap] = *derivedTypesData;
            const auto& typeIDHolder = derived.at(typeTag).value;
            if (not std::holds_alternative<std::uint32_t>(typeIDHolder)) {
                return false;
            }
            const auto typeID = std::get<std::uint32_t>(typeIDHolder);
            const auto& realDerivedTypeName = derivedTypeMap.at(typeID);

            if (realDerivedTypeName != derived.typeName) {
                return false;
            }
        }
        catch (const std::out_of_range&) {
            return false;
        }
        return true;
    }

    std::optional<std::string> checkForDerivedTypes(const AptType& base) const {
        const auto typeDataIterator = this->types.find(base.typeName);
        if (typeDataIterator == this->types.end()) {
            return std::nullopt;
        }
        const auto& typeData = typeDataIterator->second;
        if (not typeData.derivedTypes.has_value()) {
            return std::nullopt;
        }

        const auto& typeMap = typeData.derivedTypes->typeMap;
        const auto& derivedTypeTag = typeData.derivedTypes->typeTag;
        const auto& derivedTypeIDHolder = base.at(derivedTypeTag).value;
        if (not std::holds_alternative<std::uint32_t>(derivedTypeIDHolder)) {
            throw std::invalid_argument{ "Currently typeID can only be Unsigned32" };
        }

        const auto derivedTypeID = std::get<std::uint32_t>(derivedTypeIDHolder);
        const auto derivedType = typeMap.find(derivedTypeID);
        if (derivedType == typeMap.end()) {
            throw std::runtime_error{ "Unknown derived type id:" +
                                      std::to_string(derivedTypeID) };
        }
        const auto& derivedTypeName = derivedType->second;

        /*
            Commented out because actually constructObject will reconstruct derived
            type from scratch

            // copy (already initialized) base members to derived instance
            const auto baseMemberCount =
                std::get<AptType::MemberArray>(base.value).size();
            auto derived = derivedType->second;
            for (auto i = std::size_t{ 0 }; i < baseMemberCount; ++i) {
                derived.at(i) = base.at(i);
            }
        */
        return derivedTypeName;
    }

    AptType constructObject(AptType instance, DataReader& reader) const {
        auto readerInOriginalState = reader;

        const auto visitor = [this, &reader](auto& value) {
            using Type = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Type, AptType::MemberArray>) {
                for (auto& [memberName, member] : value) {
                    member.value = this->constructObject(member, reader).value;
                }
            }
            else {
                readerReadValue(reader, value);
            }
        };
        std::visit(visitor, instance.value);

        if (auto derivedTypeName = this->checkForDerivedTypes(instance);
            derivedTypeName != std::nullopt) {
            // reconstuct using derived type.value);
            // reader is in its original state
            reader = readerInOriginalState;
            instance = this->constructObject(this->getType(*derivedTypeName), reader);
        }

        return instance;
    }

    AptType& constructObject(AptType typeInstance, const Address offset) {
        auto reader = dataSource.getView().subView(offset);
        auto constructed = this->constructObject(typeInstance, reader);

        if (not this->objectInstances.empty()) {
            const auto errorText = "Created instance does not fit into the map!";
            const auto found = this->objectInstances.lower_bound(offset);
            if (found != this->objectInstances.begin()) {
                if (const auto& [address, before] = *std::prev(found);
                    address + before.size() > offset) {
                    const auto errorMessage =
                        std::string{ errorText } + " before: " + before.typeName +
                        " at " + std::to_string(address) + "; size " +
                        std::to_string(before.size()) + "; requested " +
                        constructed.typeName + " at " + std::to_string(offset);
                    throw std::runtime_error{ errorMessage };
                }
            }

            if (found != this->objectInstances.end()) {
                if (const auto& [address, after] = *found;
                    offset + constructed.size() > address) {
                    const auto errorMessage =
                        std::string{ errorText } + " after: " + after.typeName +
                        " at " + std::to_string(address) + "; size " +
                        std::to_string(after.size()) + "; requested " +
                        constructed.typeName + " at " + std::to_string(offset);
                    throw std::runtime_error{ errorMessage };
                }
            }
        }

        const auto [instanceIterator, emplaced] =
            this->objectInstances.emplace(offset, std::move(constructed));
        return instanceIterator->second;
    }

    void fetchPointedObjects(const AptType& source) {

        if (source.typeName == "Pointer") {
            const auto& pointer = std::get<AptTypePointer>(source.value);
            const auto& typePointedTo = this->getType(pointer.typePointedTo);

            if (pointer.address == 0) {
                // null pointer
                return;
            }

            if (const auto existing = this->objectInstances.find(pointer.address);
                existing != this->objectInstances.end()) {
                const auto& [address, object] = *existing;
                // if an object already exists in the same location, check if they are
                // of same type, or if existing object is derived from pointedToType
                if ((object.typeName != typePointedTo.typeName) and
                    (not isDerivedFrom(object, typePointedTo.typeName))) {
                    throw std::runtime_error{ "Another type already exists here: " +
                                              object.typeName };
                }
            }
            else {
                this->constructObject(typePointedTo, pointer.address);
            }

            const auto& fetching = this->objectInstances.at(pointer.address);
            if (const auto alreadyFetching = this->fetching.find(pointer.address);
                alreadyFetching != this->fetching.end()) {
                if (const auto& [address, typeName] = *alreadyFetching;
                    typeName == fetching.typeName) {
                    return; // skip objects already being fetched
                }
            }

            this->fetching.emplace(pointer.address, fetching.typeName);
            return fetchPointedObjects(fetching);
        }
        else if (source.typeName == "PointerToArray") {
            const auto length = std::get<std::uint32_t>(source.at("length").value);
            const auto& pointer = source.at("pointer");
            const auto& pointerValue = std::get<AptTypePointer>(pointer.value);
            const auto elementSize = this->getType(pointerValue.typePointedTo).size();

            for (auto i = Address{ 0 }; i < length; ++i) {
                auto currentPointer = pointer;
                std::get<AptTypePointer>(currentPointer.value).address =
                    pointerValue.address + i * elementSize;
                this->fetchPointedObjects(currentPointer);
            }

            // save array metadata
            const auto begin = pointerValue.address;
            const auto end = pointerValue.address + length * elementSize;
            if(begin == end) {
                // dont save empty array
                return;
            }

            const auto found = this->arrays.lower_bound(end);
            if (found != this->arrays.begin()) {
                const auto [previousBegin, previousEnd] = *std::prev(found);
                // if it's not the same array...
                if (not(previousBegin == begin and previousEnd == end)) {
                    if (previousEnd > begin) {
                        throw std::runtime_error{ "Overlapping arrays!" };
                    }
                }
            }

            this->arrays.emplace(begin, end);
        }
        else {
            if (std::holds_alternative<AptType::MemberArray>(source.value)) {
                const auto& members = std::get<AptType::MemberArray>(source.value);
                for (const auto& [memberName, member] : members) {
                    this->fetchPointedObjects(member);
                }
            }
        }
    }

    DataSource dataSource;
    TypeDataMap types;
    std::map<Address, AptType> objectInstances;
    std::map<Address, Address> arrays; //(begin address, end address)
    std::map<Address, std::string> fetching;
};

} // namespace Apt::AptTypes