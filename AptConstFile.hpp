#pragma once
#include <cstdint>
#include <variant>
#include <algorithm>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include "Util.hpp"

namespace Apt::ConstFile
{
    enum class ConstItemType : std::uint32_t {
		undefined = 0,
		string = 1,
        property = 2,
        none = 3,
		aptRegister = 4,
        boolean = 5,
        single = 6,
        integer = 7,
        lookup = 8,
	};

    struct ConstItem {
        template <typename Type>
        ConstItem(ConstItemType typeID, Type value) : type{typeID}, data{value} {}

        template<typename T>
        const T& get() const { return std::get<T>(this->data); }

		const ConstItemType type;
        const std::variant<std::nullptr_t, std::string, std::uint32_t, bool, float, std::int32_t> data;
    };

    struct SkippedUnknwonConstData {
        std::array<char, 4> unknown1 = {'\x20'};
    };

    using namespace std::string_view_literals;

    struct ConstData {
        static constexpr std::string_view constFileMagic = "Apt constant file\x1A\0\0"sv;

        ConstData(const std::string_view data) {
            auto remainingBytes = data;

            if(splitFront(remainingBytes, this->constFileMagic.size()) != this->constFileMagic) {
                throw std::runtime_error{"Apt magic not found"};
            }
            
            //read aptDataOffset
            this->aptDataOffset = splitFrontAndReadAs<std::uint32_t>(remainingBytes);

            const auto itemCount = splitFrontAndReadAs<std::uint32_t>(remainingBytes);
            //skip 4 bytes
            splitFrontAndCopyToArray(remainingBytes, this->skippedUnknownData.unknown1);
            
            for (auto i = std::uint32_t{0}; i < itemCount; ++i) {
                const auto itemType = splitFrontAndReadAs<ConstItemType>(remainingBytes);
                const auto rawValue = splitFront(remainingBytes, sizeof(std::uint32_t));
                const auto unsignedItemValue = readCharsAs<std::uint32_t>(rawValue);
                switch(itemType) {
                    case ConstItemType::string: {
                        const auto itemstringStart = unsignedItemValue;
                        const auto itemstringEnd = data.find('\0', itemstringStart);
                        if(itemstringEnd > data.size()) {
                            throw std::out_of_range{"Error when parsing ConstItem: itemstringEnd == data.npos"};
                        }
                        const auto itemstring = data.substr(itemstringStart, itemstringEnd - itemstringStart);
                        this->items.emplace_back(itemType, static_cast<std::string>(itemstring));
                        break;
                    }
                    case ConstItemType::aptRegister: {
                        this->items.emplace_back(itemType, unsignedItemValue);
                        break;
                    }
                    case ConstItemType::boolean: {
                        this->items.emplace_back(itemType, static_cast<bool>(unsignedItemValue));
                        break;
                    }
                    case ConstItemType::single: {
                        this->items.emplace_back(itemType, readCharsAs<float>(rawValue));
                        break;
                    }
                    case ConstItemType::integer: {
                        this->items.emplace_back(itemType, readCharsAs<std::int32_t>(rawValue));
                        break;
                    }
                    case ConstItemType::lookup: {
                        this->items.emplace_back(itemType, unsignedItemValue);
                        break;
                    }
                    default: {
                        throw std::invalid_argument{"Unknown type " + std::to_string(static_cast<int32_t>(itemType))};
                    }
                }
            }

        }

		uint32_t aptDataOffset;
        std::vector<ConstItem> items;
        SkippedUnknwonConstData skippedUnknownData;
    };
}