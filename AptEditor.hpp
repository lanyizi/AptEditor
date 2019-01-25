#include <filesystem>
#include <string>

namespace Apt::AptEditor {
std::string aptToXml(const std::filesystem::path aptFileName);
}