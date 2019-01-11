#pragma once
#include <fstream>
#include <filesystem>
#include <stdint.h>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define STRLENGTH(x) (4 * ((((uint32_t)strlen(x) + 1) + 3)/4))
#define GETALIGN(x) ((4 * ((x + 3) / 4)) - x)
#define ALIGN(x) x = ((uint8_t *)(4 * ((((uint32_t)x) + 3) / 4)))
#define B(x) x?"true":"false"
#define add(x) *((uint8_t **)&x) += (uint32_t)aptbuffer; 


inline bool fileExists(std::tr2::sys::path file)
{
	std::fstream test(file);
	return !test.fail();
}

inline uint32_t size(std::ifstream& stream)
{
	uint32_t size = 0;
	stream.seekg(0, std::ios::end);
	size = (uint32_t)stream.tellg();
	stream.seekg(0, std::ios::beg);
	return size;
}

inline uint32_t HexToDecimal(const char* str)
{
	return (uint32_t)strtol(str, NULL, 16);
}

//read an integer from memory
inline uint32_t ReadUint(uint8_t*& iter)
{
	uint32_t result = *(uint32_t*)iter;
	iter += 4;
	return result;
}

template<class T>
inline uint8_t GetByte(T num, uint8_t byte)
{
	uint8_t result;
	switch (byte)
	{
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


//split a string at the give character
std::vector<std::string> split(std::string str, std::string sep);

