#include <cctype>
#include <ciso646>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "AptConstFile.hpp"
#include "AptEditor.hpp"
#include "AptTypeDefinitionsParser.hpp"
#include "AptTypes.hpp"
#include "AptToXmlHints.hpp"
#include "Util.hpp"
#include "tinyxml2.h"

namespace FileSystem = std::filesystem;

namespace Apt::AptEditor {

using namespace AptTypes;

const char* chooseAttributeName(const std::string& name) {
    return name.empty() ? "value" : name.c_str();
}

void writeObject(tinyxml2::XMLElement* node, const AptObjectPool& pool,
                 const std::string& name, const AptType& object);

template <typename T>
void writeNode(tinyxml2::XMLElement* node, const AptObjectPool& pool,
               const std::string& name, const T& value) {
    static_assert(std::is_arithmetic_v<T>);
    node->SetAttribute(chooseAttributeName(name), +value);
}

void writeNode(tinyxml2::XMLElement* node, const AptObjectPool& pool,
               const std::string& name, const Unsigned24& value) {
    return writeNode(node, pool, name, value.value);
}

void writeNode(tinyxml2::XMLElement* node, const AptObjectPool& pool,
               const std::string& name, const std::string& value) {
    node->SetAttribute(chooseAttributeName(name), value.c_str());
}

void writeNode(tinyxml2::XMLElement* node, const AptObjectPool& pool,
               const std::string& name, const AptTypePointer& value) {

    node->SetAttribute((name + "Address").c_str(), value.address);

    // special handling for string: add a xml comment "hint"
    // TODO: add "hints" for other kind of objects as well,
    // when it looks neccessary.
    if (value.address != 0 and value.typePointedTo == "String") {
        if (const auto found = pool.objectInstances.find(value.address);
            found != pool.objectInstances.end()) {
            const auto& stringPointed = std::get<std::string>(found->second.value);
            const auto* hintValue =
                stringPointed.empty() ? "(empty)" : stringPointed.c_str();
            const auto hint =
                xmlEscape(asString("Address ", value.address, " = ", hintValue));

            auto* stringHintComment = node->GetDocument()->NewComment(hint.c_str());
            node->Parent()->InsertAfterChild(node, stringHintComment);
            // move current node after comment
            node->Parent()->InsertAfterChild(stringHintComment, node);
        }
    }
}

void writeNode(tinyxml2::XMLElement* node, const AptObjectPool& pool,
               const std::string& name, const PointerToArray& value) {
    return writeNode(node, pool, name, value.pointerToArray);
}

void writeNode(tinyxml2::XMLElement* node, const AptObjectPool& pool,
               const std::string& name, const AptType::MemberArray& value) {

    for (const auto& [memberName, member] : value) {
        auto* currentNode = node;
        if (std::holds_alternative<AptType::MemberArray>(member.value)) {
            auto* newNode =
                node->GetDocument()->NewElement(member.baseTypeName.c_str());
            node->InsertEndChild(newNode);
            currentNode = newNode;
            currentNode->SetAttribute("name", memberName.c_str());
        }
        writeObject(currentNode, pool, memberName, member);
    }
}

void writeNode(tinyxml2::XMLElement* node, const AptObjectPool& pool,
               const std::string& name, const PaddingForAlignment& value) {
    return;
}

void writeObject(tinyxml2::XMLElement* node, const AptObjectPool& pool,
                 const std::string& name, const AptType& object) {
    const auto visitor = [node, &pool, &name, &object](const auto& value) {
        writeNode(node, pool, name, value);
    };
    std::visit(visitor, object.value);
    if (object.typeName != object.baseTypeName) {
        const auto& [typeTag, derivedTypeMap] =
            pool.types.at(object.baseTypeName).derivedTypes.value();
        node->SetAttribute(typeTag.c_str(), object.typeName.c_str());
    }
}

using DestinationMap = std::map<Address, std::pair<Address, std::string>>;

void readInstructions(AptObjectPool& pool, const Address startAddress,
                      DestinationMap& outputDestinationMap) {

    auto lastInstructionIsEnd = false;

    auto currentAddress = startAddress;
    auto reader = pool.getReaderAtOffset(currentAddress);
    auto canEndAfterHere = startAddress;

    const auto instructionPrototype = pool.getType("Instruction");
    while (not lastInstructionIsEnd or (currentAddress <= canEndAfterHere)) {
        currentAddress = reader.absolutePosition();
        auto currentInstruction = pool.constructObject(instructionPrototype, reader);
        const auto instructionType = std::string_view{ currentInstruction.typeName };

        const auto setDestination = [&outputDestinationMap,
                                     currentAddress,
                                     &canEndAfterHere,
                                     instructionType](const Address destination) {
            auto destinationInformation =
                std::pair{ destination,
                           asString(instructionType, "@", currentAddress) };
            outputDestinationMap.emplace(currentAddress,
                                         std::move(destinationInformation));
            canEndAfterHere = (std::max)(canEndAfterHere, destination);
        };

        if (instructionType.find("Branch") == 0) {
            const auto offset =
                currentInstruction.at("offset").getNumericValue<std::int32_t>();
            const auto jumpLocation = reader.absolutePosition() + offset;
            setDestination(jumpLocation);
        }

        if (instructionType.find("DefineFunction") == 0) {
            const auto functionSize =
                currentInstruction.at("size").getNumericValue<std::uint32_t>();
            const auto endOfFunction = reader.absolutePosition() + functionSize;
            setDestination(endOfFunction);
        }

        lastInstructionIsEnd = instructionType == "End";

        pool.fetchPointedObjects(currentInstruction);
        pool.insertObject(std::move(currentInstruction), currentAddress);
    }

    pool.insertArrayData(startAddress, reader.absolutePosition());
}



void aptToXml(const std::filesystem::path& aptFileName) {
    const auto constFileName =
        std::filesystem::path{ aptFileName }.replace_extension(".const");
    const auto constData = ConstFile::ConstData(readEntireFile(constFileName));
    const auto entryOffset = constData.aptDataOffset;

    // auto data = AptFile::AptData{AptFile::DataSource{readEntireFile(aptFileName)}};
    const auto preprocess = [](const auto& fileName) {
        std::string definitionFile = readEntireFile(fileName);
        while (definitionFile.find("/*") != definitionFile.npos) {
            const auto begin = definitionFile.find("/*");
            const auto end = definitionFile.find("*/", begin + 2) + 2;
            definitionFile.erase(begin, (end - begin));
        }
        return definitionFile;
    };

    auto pool = AptObjectPool{};
    pool.dataSource.reset(readEntireFile(aptFileName));
    Parser::readTypeDefinitions(preprocess("AptTypeDefinitions.txt"), pool);

    {
        auto reader = pool.getReaderAtOffset(entryOffset);
        pool.insertObject(pool.constructObject(pool.getType("Movie"), reader),
                          entryOffset);

        pool.fetchPointedObjects(pool.objectInstances.at(entryOffset));
    }

    auto destinationMap = DestinationMap{};
    {
        // fetch instructions
        Parser::readTypeDefinitions(preprocess("ActionTypeDeclarations.txt"), pool);
        Parser::readTypeDefinitions(preprocess("ActionTypeDefinitions.txt"), pool);
        auto actionDataOffsets = std::vector<Address>{};
        for (const auto& [address, object] : pool.objectInstances) {
            const auto actionOffsetIndex = object.find("actionDataOffset");
            if (not actionOffsetIndex.has_value()) {
                continue;
            }

            const auto actionOffset =
                std::get<Address>(object.at(actionOffsetIndex.value()).value);
            actionDataOffsets.emplace_back(actionOffset);
        }

        for (const auto actionDataOffset : actionDataOffsets) {
            readInstructions(pool, actionDataOffset, destinationMap);
        }

        // edit destinationMap so end of function will match the start address of last
        // instruction in function body (instead of end address)
        for (auto& [address, destinationInformation] : destinationMap) {
            auto& [destination, information] = destinationInformation;
            if (information.find("DefineFunction") == 0) {
                const auto current = pool.objectInstances.lower_bound(destination);
                if (current == pool.objectInstances.begin()) {
                    throw std::runtime_error{ "wrong definefunction body size? " };
                }
                destination = std::prev(current)->first;
            }
        }
    }

    // merge jumpMap
    auto referenceDescriptions =
        AptToXmlHints::getReferenceDescriptions(pool, entryOffset);
    for (const auto& [source, destination] : destinationMap) {
        referenceDescriptions.emplace(destination);
    }

    // check unparsed data
    {
        const auto& unparsed = pool.dataSource.unparsedBeginEnd;
        for (const auto [begin, end] : unparsed) {
            const auto length = end - begin;
            if (length == 0) {
                // zero-length unparsed data
                continue;
            }

            const auto data =
                std::string_view{ pool.dataSource.data }.substr(begin, length);

            if (begin == 0) {
                // header

                auto headerData = std::ostringstream{};
                for (const auto byte : data) {
                    if (std::isprint(byte)) {
                        headerData << byte;
                        continue;
                    }

                    headerData << "\\x";
                    headerData << std::hex << std::uppercase << std::setfill('0')
                               << std::setw(2) << +static_cast<std::uint8_t>(byte);
                }

                const auto unparsedChunk = AptType{
                    "AptHeaderData", "AptHeaderData", headerData.str(), length
                };

                pool.insertObject(unparsedChunk, begin);
                continue;
            }

            for (const auto byte : data) {
                if (byte != 0) {
                    throw std::runtime_error{ "Non null unparsed data!!!!!" };
                }
            }
        }
    }

    auto xml = tinyxml2::XMLDocument{};
    xml.InsertFirstChild(xml.NewDeclaration());
    auto* aptDataNode = xml.NewElement("ParsedAptData");
    xml.InsertEndChild(aptDataNode);
    auto* parent = aptDataNode;
    auto arrayEnd = Address{ 0 };
    for (const auto& [address, object] : pool.objectInstances) {
        const auto referencesBegin = referenceDescriptions.begin();
        const auto referencesBound = referenceDescriptions.upper_bound(address);
        const auto referenced = (referencesBegin != referencesBound);
        auto endOfFunction = false;
        const auto referenceWriter = [parent, &endOfFunction](const auto& data) {
            const auto& [destinationAddress, information] = data;
            const auto text =
                asString(destinationAddress, " referenced by ", information);
            AptToXmlHints::appendXmlComment(parent, text);
            endOfFunction = (information.find("DefineFunction") == 0);
        };
        std::for_each(referencesBegin, referencesBound, referenceWriter);
        referenceDescriptions.erase(referencesBegin, referencesBound);

        if (const auto arrayMarker = pool.arrays.find(address);
            arrayMarker != pool.arrays.end()) {
            const auto [begin, pastTheEnd] = *arrayMarker;

            auto* array = parent->GetDocument()->NewElement("Array");
            parent->InsertEndChild(array);
            array->SetAttribute("startAddress", begin);

            parent = array;
            arrayEnd = arrayMarker->second;
        }

        if (object.baseTypeName == "Instruction") {
            const auto constantHint =
                AptToXmlHints::hintForConstantID(constData, object);
            if (not constantHint.empty()) {
                AptToXmlHints::appendXmlComment(parent, constantHint);
            }
        }

        auto* node = parent->GetDocument()->NewElement(object.baseTypeName.c_str());
        parent->InsertEndChild(node);

        // since <Array> will already show startAddress,
        // normally we don't need to show address again
        if ((referenced and (pool.arrays.count(address) == 0)) or
            (destinationMap.count(address) > 0)) {
            node->SetAttribute("address", address);
        }

        writeObject(node, pool, {}, object);

        if (object.baseTypeName == "Instruction") {
            if (endOfFunction) {
                AptToXmlHints::appendXmlComment(parent, "End Of Function");
            }

            if (object.typeName.find("Branch") == 0) {
                node->DeleteAttribute("offset");
                node->SetAttribute("destinationAddress",
                                   destinationMap.at(address).first);
            }
            if (object.typeName.find("DefineFunction") == 0) {
                node->DeleteAttribute("size");
                node->SetAttribute("lastInstructionStartAddress",
                                   destinationMap.at(address).first);
            }
        }

        if ((parent != aptDataNode) and (arrayEnd <= address + object.size())) {
            parent = aptDataNode;
            arrayEnd = 0; // reset array end
        }
    }

    tinyxml2::XMLPrinter printer;
    xml.Accept(&printer);
    const auto xmlString = std::string{ printer.CStr() };
    auto xmlOutput = std::ofstream{ aptFileName.string() + ".edited.xml" };
    xmlOutput << xmlString << std::endl;

    return;
}
} // namespace Apt::AptEditor