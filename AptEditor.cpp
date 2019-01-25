#include <cctype>
#include <sstream>
#include "tinyxml2.h"

#include "AptEditor.hpp"

#include "AptConstFile.hpp"
#include "AptTypes.hpp"
#include "AptTypeDefinitionsParser.hpp"

#include "Util.hpp"

namespace FileSystem = std::filesystem;

namespace Apt::AptEditor {

using namespace AptTypes;

template<typename... Arguments>
std::string makeString(Arguments&&... arguments) {
    auto destination = std::ostringstream{};
    (destination << ... << std::forward<Arguments>(arguments));
    return destination.str();
}

std::string escape(const std::string_view src) {
    auto destination = std::ostringstream{};
    for (char ch : src) {
        switch (ch) {
            case '&': destination << "&amp;"; break;
            case '\'': destination << "&apos;"; break;
            case '"': destination << "&quot;"; break;
            case '<': destination << "&lt;"; break;
            case '>': destination << "&gt;"; break;
            default: destination << ch; break;
        }
    }
    return destination.str();
}

template <typename T>
void writeNode(tinyxml2::XMLElement* node, const AptObjectPool& pool,
               const std::string& name, const T& value) {
    static_assert(std::is_arithmetic_v<T>);
    node->SetAttribute(name.c_str(), +value);
}

void writeObject(tinyxml2::XMLElement* node, const AptObjectPool& pool,
                 const std::string& name, const AptType& object) {
    const auto visitor = [&](const auto& value) {
        writeNode(node, pool, name, value);
    };
    std::visit(visitor, object.value);
    if(not object.baseTypeName.empty()) {
        const auto& [typeTag, derivedTypeMap] =
            pool.types.at(object.baseTypeName).derivedTypes.value();
        node->SetAttribute(typeTag.c_str(), object.typeName.c_str());
    }
}

template <>
void writeNode<std::string>(tinyxml2::XMLElement* node, const AptObjectPool& pool,
                            const std::string& name, const std::string& value) {
    const auto* attributeName = name.empty() ? "value" : name.c_str();
    node->SetAttribute(attributeName, escape(value).c_str());
}

template <>
void writeNode<AptTypePointer>(tinyxml2::XMLElement* node, const AptObjectPool& pool,
                               const std::string& name, const AptTypePointer& value) {

    const auto transformType = [](const std::string_view string) {
        auto result = std::string{};
        for(const auto character : string) {
            if(character == '>') {
                result += '-';
                continue;
            }
            if(std::isspace(character)) {
                continue;
            }
            result += character;
        }
        return result;
    };

    const auto* attributeName = name.empty() ? "value" : name.c_str();
    const auto valueString =
        makeString("Pointer-", transformType(value.typePointedTo), " ", value.address);

    node->SetAttribute(attributeName, escape(valueString).c_str());
    if (value.typePointedTo == "String") {
        if (const auto found = pool.objectInstances.find(value.address);
            found != pool.objectInstances.end()) {
            const auto& stringPointed = std::get<std::string>(found->second.value);
            const auto* hintValue =
                stringPointed.empty() ? "(empty)" : stringPointed.c_str();
            const auto stringHint = makeString("Address ", value.address, " = ", hintValue);
            auto* stringHintComment =
                node->GetDocument()->NewComment(escape(stringHint).c_str());
            //append comment
            auto* firstChild = node->FirstChild();
            if(firstChild == nullptr or firstChild->ToComment() == nullptr) {
                node->InsertFirstChild(stringHintComment);
            }
            else {
                auto* lastComment = firstChild->ToComment();
                while (lastComment != nullptr) {
                    if (lastComment->NextSibling()->ToComment() == nullptr) {
                        break;
                    }
                    lastComment = lastComment->NextSibling()->ToComment();
                }
                node->InsertAfterChild(lastComment, stringHintComment);
            }
        }
    }
}

template <>
void writeNode<Unsigned24>(tinyxml2::XMLElement* node, const AptObjectPool& pool,
                           const std::string& name, const Unsigned24& value) {
    return writeNode(node, pool, name, value.value);
}

template <>
void writeNode<AptType::MemberArray>(tinyxml2::XMLElement* node,
                                     const AptObjectPool& pool,
                                     const std::string& name,
                                     const AptType::MemberArray& value) {

    for (const auto& [memberName, member] : value) {
        auto* currentNode = node;
        if (std::holds_alternative<AptType::MemberArray>(member.value)) {
            const auto& baseTypeName =
                member.baseTypeName.empty() ? member.typeName : member.baseTypeName;
            auto* newNode = node->GetDocument()->NewElement(baseTypeName.c_str());
            node->InsertEndChild(newNode);
            currentNode = newNode;
            currentNode->SetAttribute("name", memberName.c_str());
        }
        writeObject(currentNode, pool, memberName, member);
    }

    /*return writeNode(node, name, value.value);*/
}

std::string aptToXml(const FileSystem::path aptFileName) {
    const auto aptBaseName =
        FileSystem::path{ aptFileName }.replace_extension().string();
    const auto constData =
        ConstFile::ConstData(readEntireFile(aptBaseName + ".const"));

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

    AptObjectPool pool{};
    pool.dataSource.reset(readEntireFile(aptFileName));
    Parser::readTypeDefinitions(preprocess("AptTypeDefinitionTest.txt"), pool);

    {
        const auto entryOffset = constData.aptDataOffset;
        const auto& outputMovie =
            pool.constructObject(pool.getType("Movie"), entryOffset);
        pool.fetchPointedObjects(outputMovie);
    }

    auto xml = tinyxml2::XMLDocument{};
    xml.InsertFirstChild(xml.NewDeclaration());
    auto* aptDataNode = xml.NewElement("ParsedAptData");
    xml.InsertEndChild(aptDataNode);
    auto* parent = aptDataNode;
    auto arrayEnd = Address{0};
    for (const auto& [address, object] : pool.objectInstances) {
        if (const auto arrayMarker = pool.arrays.find(address);
            arrayMarker != pool.arrays.end()) {
            const auto commentText = makeString("Following array startAddress=\"",
                                                arrayMarker->first,
                                                "\" pastTheEndAddress=\"",
                                                arrayMarker->second,
                                                "\"");
            auto* comment = parent->GetDocument()->NewComment(commentText.c_str());
            parent->InsertEndChild(comment);

            auto* array = parent->GetDocument()->NewElement("Array");
            parent->InsertEndChild(array);

            parent = array;
            arrayEnd = arrayMarker->second;
        }

        const auto& baseTypeName =
            object.baseTypeName.empty() ? object.typeName : object.baseTypeName;

        /*const auto commentText = makeString("Following ",
                                            baseTypeName,
                                            " address=\"",
                                            address,
                                            "\" size=\"",
                                            object.size(),
                                            "\"");
        auto* comment = parent->GetDocument()->NewComment(commentText.c_str());
        parent->InsertEndChild(comment);*/

        auto* node = parent->GetDocument()->NewElement(baseTypeName.c_str());
        parent->InsertEndChild(node);
        node->SetAttribute("address", address);
        writeObject(node, pool, {}, object);
        

        if((parent != aptDataNode) and (arrayEnd <= address + object.size())) {
            parent = aptDataNode;
            arrayEnd = 0; //reset array end
        }
    }
    

    tinyxml2::XMLPrinter printer;
    xml.Accept(&printer);
    const auto xmlString = std::string{printer.CStr()};
    auto xmlOutput = std::ofstream{ aptFileName.string() + ".edited.xml" };
    xmlOutput << xmlString << std::endl;

    return "";
}
}