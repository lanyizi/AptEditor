#pragma once
#include "AptTypes.hpp"
#include "Util.hpp"
#include <cctype>
#include <ciso646>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace Apt::AptTypes::Parser {

using TypeData = AptObjectPool::TypeData;
using TypeDataMap = AptObjectPool::TypeDataMap;
using TypeDataMapEntry = std::pair<std::string, TypeData>;
using DerivedTypeData = AptObjectPool::DerivedTypeData;

struct CurrentAptTypes {
    const TypeData* getTypeData(const std::string_view typeName) const {
        for (const auto existing : { &(this->existing.types), &(this->newTypes) }) {
            if (const auto existingType = existing->find(typeName);
                existingType != existing->end()) {
                const auto& [typeName, typeData] = *existingType;
                return &typeData;
            }
        }
        return nullptr;
    }

    AptType getType(const std::string_view typeName) const {
        if (const auto* typeData = this->getTypeData(typeName); typeData != nullptr) {
            return typeData->type;
        }
        return existing.getType(typeName);
    }

    AptObjectPool& existing;
    TypeDataMap& newTypes;
};

DerivedTypeData parseDerivedTypes(std::string_view deriveDefiniton) {
    auto derivedTypesData = DerivedTypeData{};
    while (not deriveDefiniton.empty()) {
        const auto typeTag =
            trim(readUntilCharacterIf(deriveDefiniton, [](const auto character) {
                return std::isspace(character);
            }));

        const auto typeTagStringValue = trim(readUntil(deriveDefiniton, ">"));
        const auto typeID = [string = typeTagStringValue] {
            try {
                return std::stoul(std::string{ string }, nullptr, 0);
            }
            catch(const std::exception&) {
                throw std::invalid_argument{ "Currently typeID must be integral" };
            }
        }();
        const auto derivedType = trim(readUntil(deriveDefiniton, "/"));

        if (derivedTypesData.typeTag.empty()) {
            derivedTypesData.typeTag = typeTag;
        }

        if (derivedTypesData.typeTag != typeTag) {
            throw std::invalid_argument{ "Inconsistent derived type specifier" };
        }

        const auto [emplaced, result] =
            derivedTypesData.typeMap.emplace(typeID, derivedType);
        if (not result) {
            throw std::runtime_error{ "Failed to add new derived type" };
        }

        deriveDefiniton = trim(deriveDefiniton);
    }

    return derivedTypesData;
}

TypeDataMapEntry readTypeDefinition(std::string_view typeDefinition,
                                    const CurrentAptTypes& existingTypes) {
    auto typeData = TypeData{};
    const auto typeName = trim(readUntil(typeDefinition, "="));
    typeData.type.typeName = typeName;
    typeData.type.baseTypeName = typeName;
    typeData.type.value = AptType::MemberArray{};
    auto& memberArray = std::get<AptType::MemberArray>(typeData.type.value);
    while (not typeDefinition.empty()) {
        auto member = trim(readUntil(typeDefinition, ","));
        const auto memberTypeName = trim(readUntil(member, ":"));
        const auto value = trim(member);
        if (memberTypeName == "$Base") {
            const auto* base = existingTypes.getTypeData(value);
            if (base == nullptr or not(base->derivedTypes.has_value())) {
                throw std::invalid_argument{ "Cannot find any base type named " +
                                             std::string{ value } };
            }
            typeData.type.baseTypeName = base->type.typeName;
            // copy base type member array
            memberArray = std::get<AptType::MemberArray>(base->type.value);
        }
        else if (memberTypeName == "$Derive") {
            if (typeData.derivedTypes.has_value()) {
                throw std::invalid_argument{
                    "Another definition of derived type already exists!"
                };
            }
            typeData.derivedTypes = parseDerivedTypes(value);
        }
        else {
            // define member variables
            memberArray.emplace_back(value, existingTypes.getType(memberTypeName));
        }

        typeDefinition = trim(typeDefinition);
    }
    return TypeDataMapEntry{ typeName, std::move(typeData) };
}

void readTypeDefinitions(std::string_view input, AptObjectPool& aptObjectPool) {
    auto currentTypeMap = TypeDataMap{};
    auto currentTypeMaps = CurrentAptTypes{ aptObjectPool, currentTypeMap };
    while (not input.empty()) {
        const auto declaration = trim(readUntil(input, ";"));
        currentTypeMap.emplace(readTypeDefinition(declaration, currentTypeMaps));
        input = trim(input);
    }
    aptObjectPool.types.merge(currentTypeMap);
    if(not currentTypeMap.empty()) {
        throw std::runtime_error{ "Some types are not merged into pool!" };
    }
}
} // namespace Apt::AptTypes::Parser