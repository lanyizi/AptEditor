#pragma once
#include <algorithm>
#include <cctype>
#include <ciso646>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <limits>
#include <type_traits>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define STRLENGTH(x) (4 * ((((uint32_t)strlen(x) + 1) + 3) / 4))
#define GETALIGN(x) ((4 * ((x + 3) / 4)) - x)
#define ALIGN(x) x = ((uint8_t*)(4 * ((((uint32_t)x) + 3) / 4)))
#define B(x) x ? "true" : "false"
#define add(x) *((uint8_t**)&x) += (uint32_t)aptbuffer;

std::string readEntireFile(const std::filesystem::path& filePath);

std::string_view trySplitFront(std::string_view& source,
                               const std::string_view::size_type maxLength) noexcept;

std::string_view splitFront(std::string_view& source,
                            const std::string_view::size_type length);

template <typename T>
T readCharsAs(const std::string_view source) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (source.size() != sizeof(T)) {
        throw std::length_error{ "readCharsAs: source.size() != sizeof(T)" };
    }
    return *reinterpret_cast<const T*>(source.data());
}

template <typename T>
T splitFrontAndReadAs(std::string_view& source) {
    static_assert(std::is_trivially_copyable_v<T>);
    return readCharsAs<T>(splitFront(source, sizeof(T)));
}

template <typename Array>
void splitFrontAndCopyToArray(std::string_view& source, Array& array) {
    static_assert(std::is_same_v<std::decay_t<decltype(*std::begin(source))>,
                                 std::decay_t<decltype(*std::begin(array))>>);
    const auto splitted = splitFront(source, array.size());
    std::copy(std::begin(splitted), std::end(splitted), std::begin(array));
}

// split a string at the give character
std::vector<std::string> split(std::string_view source, std::string_view separator);

inline std::string_view trim(std::string_view source) {
    while (not source.empty() and std::isspace(source.front())) {
        source.remove_prefix(1);
    }
    while (not source.empty() and std::isspace(source.back())) {
        source.remove_suffix(1);
    }
    return source;
}

template <typename Predicate>
std::string_view readUntilCharacterIf(std::string_view& from, Predicate predicate) {
    const auto end = std::find_if(from.cbegin(), from.cend(), predicate);
    auto textRead = from.substr(0, std::distance(from.cbegin(), end));
    from.remove_prefix((std::min)(from.size(), textRead.size() + 1));
    return textRead;
}

inline std::string_view readUntil(std::string_view& from, const std::string_view delimiter) {
    auto textRead = from.substr(0, (std::min)(from.size(), from.find(delimiter)));
    from.remove_prefix((std::min)(from.size(), textRead.size() + delimiter.size()));
    return textRead;
}

template <typename... Arguments>
std::string asString(Arguments&&... arguments) {
    auto destination = std::ostringstream{};
    (destination << ... << std::forward<Arguments>(arguments));
    return destination.str();
}

inline std::string xmlEscape(const std::string_view src) {
    auto destination = std::ostringstream{};
    for (char ch : src) {
        switch (ch) {
        case '&':
            destination << "&amp;";
            break;
        case '\'':
            destination << "&apos;";
            break;
        case '"':
            destination << "&quot;";
            break;
        case '<':
            destination << "&lt;";
            break;
        case '>':
            destination << "&gt;";
            break;
        default:
            destination << ch;
            break;
        }
    }
    return destination.str();
}

inline uint32_t HexToDecimal(const char* str) {
    return (uint32_t)strtol(str, NULL, 16);
}

// read an integer from memory
inline uint32_t ReadUint(uint8_t*& iter) {
    uint32_t result = *(uint32_t*)iter;
    iter += 4;
    return result;
}

template <class T>
inline uint8_t GetByte(T num, uint8_t byte) {
    uint8_t result;
    switch (byte) {
    case 0:
        result = LOBYTE(LOWORD(num));
        break;
    case 1:
        result = HIBYTE(LOWORD(num));
        break;
    case 2:
        result = LOBYTE(HIWORD(num));
        break;
    case 3:
        result = HIBYTE(HIWORD(num));
        break;
    }

    return result;
}