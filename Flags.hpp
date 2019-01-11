#pragma once
#include <stdint.h>
#include <string>


class Flags
{
private:
	struct PlaceObjectFlags {
		struct Flags
		{
			bool PlaceFlagMove : 1;
			bool PlaceFlagHasCharacter : 1;
			bool PlaceFlagHasMatrix : 1;
			bool PlaceFlagHasColorTransform : 1;
			bool PlaceFlagHasRatio : 1;
			bool PlaceFlagHasName : 1;
			bool PlaceFlagHasClipDepth : 1;
			bool PlaceFlagHasClipActions : 1;
		};
		union
		{
			Flags flags;
			uint32_t value;
		};

		PlaceObjectFlags()
		{
			value = 0;
		}
	};

	struct ClipEventFlags {
		bool ClipEventLoad : 1;
		bool ClipEventEnterFrame : 1;
		bool ClipEventUnload : 1;
		bool ClipEventMouseMove : 1;
		bool ClipEventMouseDown : 1;
		bool ClipEventMouseUp : 1;
		bool ClipEventKeyDown : 1;
		bool ClipEventKeyUp : 1;
		bool ClipEventData : 1;
		bool ClipEventInitialize : 1;
		bool ClipEventPress : 1;
		bool ClipEventRelease : 1;
		bool ClipEventReleaseOutside : 1;
		bool ClipEventRollOver : 1;
		bool ClipEventRollOut : 1;
		bool ClipEventDragOver : 1;
		bool ClipEventDragOut : 1;
		bool ClipEventKeyPress : 1;
		bool ClipEventConstruct : 1;
		char Reserved : 5;
		char Reserved2 : 8;
	};

	struct ButtonFlags 
	{
		struct Flags
		{
			bool ButtonStateUp : 1;
			bool ButtonStateOver : 1;
			bool ButtonStateDown : 1;
			bool ButtonStateHitTest : 1;
			char Reserved : 4;
		};
		union
		{
			Flags flags;
			uint32_t value;
		};

		ButtonFlags()
		{
			value = 0;
		}
	};

	struct ButtonActionFlags 
	{
		struct Flags
		{
			bool CondOverDownToIdle : 1;
			char KeyPress : 7;
			bool CondIdleToOverUp : 1;
			bool CondOverUpToIdle : 1;
			bool CondOverUpToOverDown : 1;
			bool CondOverDownToOverUp : 1;
			bool CondOverDownToOutDown : 1;
			bool CondOutDownToOverDown : 1;
			bool CondOutDownToIdle : 1;
			bool CondIdleToOverDown : 1;
		};
		union
		{
			Flags flags;
			uint32_t value;
		};

		ButtonActionFlags()
		{
			value = 0;
		}
	};

public:
	static std::string GetPOFlags_str(uint32_t);
	static uint32_t GetPOFlags_int(const char*);
	static std::string GetButFlags_str(uint32_t);
	static uint32_t GetButFlags_int(const char*);
	static std::string GetButActionFlags_str(uint32_t);
	static uint32_t GetButActionFlags_int(const char*);
};