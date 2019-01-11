#pragma once
#include "Util.hpp"

namespace Constfile
{
	enum AptConstItemType {
		TYPE_UNDEF = 0,
		TYPE_STRING = 1,
		TYPE_NUMBER = 4,
	};

	struct AptConstItem {
		AptConstItemType type;
		union {
			const char *strvalue;
			uint32_t numvalue;
		};
	};

	struct AptConstData {
		uint32_t aptdataoffset;
		uint32_t itemcount;
		std::vector<AptConstItem *> items;
	};

}