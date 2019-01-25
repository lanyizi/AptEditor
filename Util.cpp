#include "Util.hpp"

std::string readEntireFile(const std::filesystem::path& filePath) {
	auto result = std::string{};
	const auto fileSize = std::filesystem::file_size(filePath);
	result.resize(fileSize);

	try {
		auto stream = std::ifstream{filePath, std::ifstream::binary};
    	const auto defaultExceptions = stream.exceptions();
    	stream.exceptions(stream.failbit | stream.badbit | defaultExceptions);
		stream.read(reinterpret_cast<char*>(result.data()), fileSize);
        stream.exceptions(defaultExceptions);
        if(stream.peek() != std::ifstream::traits_type::eof()) {
            throw std::runtime_error{"File changed as we read it:" + filePath.string()};
        }
	}
	catch(const std::ios::failure& error) {
        throw std::runtime_error{"Failed to read file " + filePath.string() + ": " + error.what()};
    }
	
    return result;
}

std::string_view trySplitFront(std::string_view& source, const std::string_view::size_type maxLength) noexcept {
    const auto splitted = source.substr(0, maxLength);
    source.remove_prefix(splitted.size());
    return splitted;
}

std::string_view splitFront(std::string_view& source, const std::string_view::size_type length) {
	if(length > source.size()) {
        throw std::out_of_range{"splitFront: length > source.size()"};
    }
    return trySplitFront(source, length);
}

std::vector<std::string> split(std::string_view source, std::string_view separator) {
    auto splitted = std::vector<std::string>{};

    do {
        auto token = trySplitFront(source, source.find(separator));
		if(source.empty() == false) {
            source.remove_prefix(separator.size());
        }
		if(token.empty() == false) {
            splitted.emplace_back(token);
        }
    }
	while (source.empty() == false);

    return splitted;
}
