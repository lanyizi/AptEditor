#pragma once
#include "AptAptParseUtilities.hpp"
#include "Util.hpp"
#include <cctype>
#include <ciso646>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
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

void readerReadValue(DataReader& reader, std::string& value) {
    for (auto next = std::string_view{}; next.find('\0') == next.npos;
         next = reader.readFront(1)) {
        value += next;
    }
}

struct PaddingForAlignment {
    std::uint32_t align;
    std::uint32_t actuallyPadded;
};

void readerReadValue(DataReader& reader, PaddingForAlignment& value) {
    value.actuallyPadded = 0;
    while (reader.absolutePosition() % value.align != 0) {
        const auto read = reader.readFront(1);
        value.actuallyPadded += read.size();
    }
}

struct AptTypePointer {
    std::string typePointedTo;
    Address address;
};

void readerReadValue(DataReader& reader, AptTypePointer& value) {
    value.address = reader.readFrontAs<Address>();
}

struct PointerToArray {
    PointerToArray() : length{ unsetLength }, arraySizeVariable{}, pointerToArray{} {}
    std::size_t length;
    std::string arraySizeVariable;
    AptTypePointer pointerToArray;
    static constexpr auto unsetLength = (std::numeric_limits<std::size_t>::max)();
};

void readerReadValue(DataReader& reader, PointerToArray& value) {
    value.pointerToArray.address = reader.readFrontAs<Address>();
}

struct Unsigned24 {
    std::uint32_t value;
};

void readerReadValue(DataReader& reader, Unsigned24& value) {
    // assuming little endian
    const auto data = reader.readFront(3);
    value.value = 0;
    std::memcpy(&value.value, data.data(), data.size());
}

struct AptType {
    using MemberArray = std::vector<std::pair<std::string, AptType>>;
    using Value = std::variant<std::uint8_t, std::uint16_t, Unsigned24, std::int32_t,
                               std::uint32_t, float, std::string, AptTypePointer,
                               PointerToArray, MemberArray, PaddingForAlignment>;

    template <typename Self>
    static auto& at(Self& self, const std::size_t index) {
        return std::get<MemberArray>(self.value).at(index).second;
    }

    template <typename Predicate>
    std::optional<std::size_t> findIf(Predicate predicate) const {
        if (not std::holds_alternative<MemberArray>(this->value)) {
            return std::nullopt;
        }
        auto& members = std::get<MemberArray>(this->value);
        const auto member = std::find_if(members.begin(), members.end(), predicate);
        if (member == members.end()) {
            return std::nullopt;
        }
        return std::distance(members.begin(), member);
    }

    std::optional<std::size_t> find(const std::string_view memberName) const {
        const auto predicate = [memberName](const auto& pair) {
            return pair.first == memberName;
        };
        return this->findIf(predicate);
    }

    template <typename Self>
    static auto& at(Self& self, const std::string_view memberName) {
        const auto memberIndex = self.find(memberName);
        if (not memberIndex.has_value()) {
            throw std::out_of_range{ "Cannot find any member named " +
                                     std::string{ memberName } };
        }
        return self.at(self, memberIndex.value());
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

        if (std::holds_alternative<PaddingForAlignment>(this->value)) {
            return std::get<PaddingForAlignment>(this->value).actuallyPadded;
        }
        else if (std::holds_alternative<std::string>(this->value)) {
            // +1 because normally a null terminator is needed
            return std::get<std::string>(this->value).size() + 1;
        }

        auto memberTotalSize = 0;
        for (const auto& [name, member] : std::get<MemberArray>(this->value)) {
            memberTotalSize += member.size();
        }
        return memberTotalSize;
    }

    template<typename T>
    T getNumericValue() const {
        auto result = T{};
        const auto assignmentVisitor = [&result](const auto& value) {
            using Type = std::decay_t<decltype(value)>;
            if constexpr (std::is_arithmetic_v<Type>) {
                result = static_cast<T>(value);
            }
            else if constexpr (std::is_same_v<Type, Unsigned24>) {
                result = static_cast<T>(value.value);
            }
            else {
                throw std::invalid_argument{ "Cannot convert to numeric value" };
            }
        };
        std::visit(assignmentVisitor, this->value);
        return result;
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
                                       std::string{ name },
                                       std::forward<decltype(value)>(value),
                                       size } };
        };

        static_assert(sizeof(float) == 4);
        auto types = std::map<std::string, AptType, std::less<>>{
            declareType("PaddingForAlignment", PaddingForAlignment{}),
            declareType("Unsigned8", std::uint8_t{}, 1),
            declareType("Unsigned16", std::uint16_t{}, 2),
            declareType("Unsigned24", Unsigned24{}, 3),
            declareType("Int32", std::int32_t{}, 4),
            declareType("Unsigned32", std::uint32_t{}, 4),
            declareType("Float32", float{}, 4),
            declareType("String", std::string{}),
            declareType("Pointer", AptTypePointer{}, 4),
            declareType("PointerToArray", PointerToArray{}, 4),
        };

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

    static AptType parsePaddingDeclaration(std::string_view paddingTypeName) {
        const auto thisType = trim(readUntil(paddingTypeName, ">"));
        const auto alignment = std::string{ trim(paddingTypeName) };
        const auto* paddingType = getBuiltInType(thisType);
        if (paddingType == nullptr) {
            throw std::runtime_error{ "Cannot find padding as built in type!" };
        }

        auto instancedPadding = *paddingType;
        try {
            const auto alignValue = std::stoul(std::string{ alignment }, nullptr, 0);
            std::get<PaddingForAlignment>(instancedPadding.value).align = alignValue;
        }
        catch (const std::invalid_argument&) {
            throw std::invalid_argument{ "Alignment must be integral!" };
        }

        return instancedPadding;
    }

    static AptType parsePointerDeclaration(std::string_view pointerType) {
        auto leftPart = trim(readUntil(pointerType, ">"));
        const auto thisType = trim(readUntilCharacterIf(
            leftPart, [](const char character) { return std::isspace(character); }));
        const auto attribute = trim(leftPart);
        const auto pointedToType = trim(pointerType);
        const auto* pointer = getBuiltInType(thisType);
        if (pointer == nullptr) {
            throw std::runtime_error{ "Cannot find pointer as builtin type!" };
        }
        auto instancedPointer = *pointer;
        if (instancedPointer.typeName == "Pointer") {
            std::get<AptTypePointer>(instancedPointer.value).typePointedTo =
                pointedToType;
        }
        else if (instancedPointer.typeName == "PointerToArray") {
            auto& array = std::get<PointerToArray>(instancedPointer.value);
            array.pointerToArray.typePointedTo = pointedToType;
            array.arraySizeVariable = attribute;
        }
        else {
            throw std::invalid_argument{ "Invalid type: " + std::string{ thisType } };
        }

        return instancedPointer;
    }

    AptType getType(const std::string_view typeName) const {
        if (typeName.find("PaddingForAlignment") == 0) {
            return parsePaddingDeclaration(typeName);
        }
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

    bool isSameOrDerivedFrom(const AptType& derived,
                             const std::string_view baseTypeName) const {
        if (derived.typeName == baseTypeName) {
            return true;
        }

        if (derived.baseTypeName == baseTypeName) {
            return true;
        }

        if (derived.baseTypeName == derived.typeName) {
            return false;
        }

        return this->isSameOrDerivedFrom(this->getType(derived.baseTypeName),
                                         baseTypeName);
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
        const auto& derivedTypeIDHolder = base.at(derivedTypeTag);
        const auto derivedTypeID =
            derivedTypeIDHolder.getNumericValue<std::uint32_t>();

        const auto derivedType = typeMap.find(derivedTypeID);
        if (derivedType == typeMap.end()) {
            throw std::runtime_error{ "Unknown derived type id:" +
                                      std::to_string(derivedTypeID) };
        }
        const auto& derivedTypeName = derivedType->second;

        if (const auto derivedDerived =
                this->checkForDerivedTypes(this->getType(derivedTypeName));
            derivedDerived.has_value()) {
            return derivedDerived;
        }

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

        // set array length
        if (std::holds_alternative<AptType::MemberArray>(instance.value)) {
            auto& members = std::get<AptType::MemberArray>(instance.value);
            for (auto& [memberName, member] : members) {
                if (not std::holds_alternative<PointerToArray>(member.value)) {
                    continue;
                }
                auto& array = std::get<PointerToArray>(member.value);
                const auto lengthIndex = instance.find(array.arraySizeVariable);
                if (not lengthIndex.has_value()) {
                    throw std::invalid_argument{ "Array length parameter not found" };
                }
                array.length =
                    instance.at(lengthIndex.value()).getNumericValue<std::size_t>();
            }
        }

        return instance;
    }

    DataReader getReaderAtOffset(const Address offset) {
        return dataSource.getView().subView(offset);
    }

    // insert an already constructed object to pool
    void insertObject(AptType constructed, const Address offset) {
        // check object address before inserting
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

        this->objectInstances.emplace(offset, std::move(constructed));
    }

    void insertArrayData(Address begin, Address pastTheEnd) {
        if (begin == pastTheEnd) {
            // dont save empty array
            return;
        }

        const auto found = this->arrays.lower_bound(pastTheEnd);
        if (found != this->arrays.begin()) {
            const auto [previousBegin, previousEnd] = *std::prev(found);
            // if it's not the same array...
            if (not(previousBegin == begin and previousEnd == pastTheEnd)) {
                if (previousEnd > begin) {
                    throw std::runtime_error{ "Overlapping arrays!" };
                }
            }
        }

        this->arrays.emplace(begin, pastTheEnd);
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
                if ((not isSameOrDerivedFrom(object, typePointedTo.typeName))) {
                    throw std::runtime_error{ "Another type already exists here: " +
                                              object.typeName };
                }
            }
            else {
                auto reader = this->getReaderAtOffset(pointer.address);
                this->insertObject(this->constructObject(typePointedTo, reader),
                                   pointer.address);
            }

            // avoid infinite loop when apt objects have circular references
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

            const auto& [length, lengthName, pointerValue] =
                std::get<PointerToArray>(source.value);
            if (length == PointerToArray::unsetLength) {
                throw std::runtime_error{ "Array size not set!" };
            }
            const auto elementSize = this->getType(pointerValue.typePointedTo).size();

            auto currentPointer =
                this->getType("Pointer > " + pointerValue.typePointedTo);
            auto& currentPointerAddress =
                std::get<AptTypePointer>(currentPointer.value).address;

            const auto begin = pointerValue.address;
            const auto end = pointerValue.address + length * elementSize;

            for (currentPointerAddress = begin; currentPointerAddress < end;
                 currentPointerAddress += elementSize) {
                this->fetchPointedObjects(currentPointer);
            }

            // save array metadata
            this->insertArrayData(begin, end);
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
    std::map<Address, Address> arrays; //(begin address, past the end address)
    std::map<Address, std::string> fetching;
};

} // namespace Apt::AptTypes