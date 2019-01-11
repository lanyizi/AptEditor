#include "Flags.hpp"
#include "Util.hpp"
#include "Keycode.hpp"
#include <vector>
#include <algorithm>
#include <iostream>

std::string Flags::GetPOFlags_str(uint32_t flagint)
{
	PlaceObjectFlags poflags;
	poflags.value = flagint;
	std::string result;

	if (poflags.flags.PlaceFlagHasCharacter)
		result.append("HasCharacter|");
	if (poflags.flags.PlaceFlagHasClipActions)
		result.append("HasClipActions|");
	if (poflags.flags.PlaceFlagHasClipDepth)
		result.append("HasClipDepth|");
	if (poflags.flags.PlaceFlagHasColorTransform)
		result.append("HasColorTransform|");
	if (poflags.flags.PlaceFlagHasMatrix)
		result.append("HasMatrix|");
	if (poflags.flags.PlaceFlagHasName)
		result.append("HasName|");
	if (poflags.flags.PlaceFlagHasRatio)
		result.append("HasRatio|");
	if (poflags.flags.PlaceFlagMove)
		result.append("Move");

	if (result.back() == '|')
		result.pop_back();

	return result;
}

uint32_t Flags::GetPOFlags_int(const char* flagstr)
{
	PlaceObjectFlags poflags = PlaceObjectFlags();
	std::string str = flagstr;
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	remove_if(str.begin(), str.end(), isspace);
	auto flagsset = split(str, "|");
	for (auto& i : flagsset)
	{
		if (i == "hascharacter")
			poflags.flags.PlaceFlagHasCharacter = true;
		else if (i == "hasclipactions")
			poflags.flags.PlaceFlagHasClipActions = true;
		else if (i == "hasclipdepth")
			poflags.flags.PlaceFlagHasClipDepth= true;
		else if (i == "hascolortransform")
			poflags.flags.PlaceFlagHasColorTransform = true;
		else if (i == "hasmatrix")
			poflags.flags.PlaceFlagHasMatrix = true;
		else if (i == "hasname")
			poflags.flags.PlaceFlagHasName = true;
		else if (i == "hasratio")
			poflags.flags.PlaceFlagHasRatio = true;
		else if (i == "move")
			poflags.flags.PlaceFlagMove = true;
		else
			std::cout << "Unknown PlaceObjectFlag: " << i << std::endl;
	}

	return poflags.value;
}

std::string Flags::GetButFlags_str(uint32_t flagint)
{
	ButtonFlags butflags;
	butflags.value = flagint;
	std::string result;

	if (butflags.flags.ButtonStateDown)
		result.append("ButtonStateDown|");
	if (butflags.flags.ButtonStateHitTest)
		result.append("ButtonStateHitTest|");
	if (butflags.flags.ButtonStateOver)
		result.append("ButtonStateOver|");
	if (butflags.flags.ButtonStateUp)
		result.append("ButtonStateUp");

	if (result.back() == '|')
		result.pop_back();

	return result;
}

uint32_t Flags::GetButFlags_int(const char* flagstr)
{
	ButtonFlags butflags = ButtonFlags();
	std::string str = flagstr;
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	remove_if(str.begin(), str.end(), isspace);
	auto flagsset = split(str, "|");
	for (auto& i : flagsset)
	{
		if (i == "buttonstatedown")
			butflags.flags.ButtonStateDown = true;
		else if (i == "buttonstatehittest")
			butflags.flags.ButtonStateHitTest = true;
		else if (i == "buttonstateover")
			butflags.flags.ButtonStateOver = true;
		else if (i == "buttonstateup")
			butflags.flags.ButtonStateUp = true;
		else
			std::cout << "Unknown ButtonFlag: " << i << std::endl;
	}

	return butflags.value;
}

std::string Flags::GetButActionFlags_str(uint32_t flagint)
{
	ButtonActionFlags actionflags;
	actionflags.value = flagint;
	std::string result;
	std::string keystr;

	//check for ascii character
	if (actionflags.flags.KeyPress >= 32)
		keystr = actionflags.flags.KeyPress;
	else
	{
		KeyCode key = (KeyCode)actionflags.flags.KeyPress;
		switch (key)
		{
		case KEY_NONE:
			keystr = "none";
			break;
		case KEY_LEFT:
			keystr = "left";
			break;
		case KEY_RIGHT:
			keystr = "right";
			break;
		case KEY_HOME:
			keystr = "home";
			break;
		case KEY_END:
			keystr = "end";
			break;
		case KEY_INSERT:
			keystr = "insert";
			break;
		case KEY_DELETE:
			keystr = "delete";
			break;
		case KEY_BACKSPACE:
			keystr = "backspace";
			break;
		case KEY_UNKNOWN_9:
			keystr = "unknown_9";
			break;
		case KEY_ENTER:
			keystr = "enter";
			break;
		case KEY_UP:
			keystr = "up";
			break;
		case KEY_DOWN:
			keystr = "down";
			break;
		case KEY_PGUP:
			keystr = "pgup";
			break;
		case KEY_PGDOWN:
			keystr = "pgdown";
			break;
		case KEY_TAB:
			keystr = "tab";
			break;
		case KEY_ESCAPE:
			keystr = "escape";
			break;
		default:
			keystr = std::to_string(key);
			break;
		}
	}

	if (actionflags.flags.KeyPress)
		result.append("Key:"+keystr+"|");
	if (actionflags.flags.CondIdleToOverDown)
		result.append("CondIdleToOverDown|");
	if (actionflags.flags.CondIdleToOverUp)
		result.append("CondIdleToOverUp|");
	if (actionflags.flags.CondOutDownToIdle)
		result.append("CondOutDownToIdle|");
	if (actionflags.flags.CondOutDownToOverDown)
		result.append("CondOutDownToOverDown|");
	if (actionflags.flags.CondOverDownToIdle)
		result.append("CondOverDownToIdle|");
	if (actionflags.flags.CondOverDownToOutDown)
		result.append("CondOverDownToOutDown|");
	if (actionflags.flags.CondOverDownToOverUp)
		result.append("CondOverDownToOverUp|");
	if (actionflags.flags.CondOverUpToIdle)
		result.append("CondOverUpToIdle|");
	if (actionflags.flags.CondOverUpToOverDown)
		result.append("CondOverUpToOverDown");

	if (result.back() == '|')
		result.pop_back();

	return result;
}

uint32_t Flags::GetButActionFlags_int(const char* flagstr)
{
	ButtonActionFlags actionflags = ButtonActionFlags();
	std::string str = flagstr;
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	remove_if(str.begin(), str.end(), isspace);
	auto flagsset = split(str, "|");
	std::string keystr;
	bool haskey = false;

	for (auto i : flagsset)
	{
		auto test = i.substr(0, 3);
		if (test == "key")
		{
			keystr = i;
			flagsset.erase(std::remove(flagsset.begin(), flagsset.end(), i), flagsset.end());
			haskey = true;
			break;
		}
			
	}

	if (haskey)
	{
	auto keypressed = split(keystr, ":")[1];

	if (keypressed.size() == 1)
		actionflags.flags.KeyPress = keypressed[0];
	else
	{
		if (keypressed == "none")
			actionflags.flags.KeyPress = KEY_NONE;
		else if (keypressed == "left")
			actionflags.flags.KeyPress = KEY_LEFT;
		else if (keypressed == "right")
			actionflags.flags.KeyPress = KEY_RIGHT;
		else if (keypressed == "home")
			actionflags.flags.KeyPress = KEY_HOME;
		else if (keypressed == "end")
			actionflags.flags.KeyPress = KEY_END;
		else if (keypressed == "insert")
			actionflags.flags.KeyPress = KEY_INSERT;
		else if (keypressed == "delete")
			actionflags.flags.KeyPress = KEY_DELETE;
		else if (keypressed == "backspace")
			actionflags.flags.KeyPress = KEY_BACKSPACE;
		else if (keypressed == "unknown_9")
			actionflags.flags.KeyPress = KEY_UNKNOWN_9;
		else if (keypressed == "enter")
			actionflags.flags.KeyPress = KEY_ENTER;
		else if (keypressed == "up")
			actionflags.flags.KeyPress = KEY_UP;
		else if (keypressed == "down")
			actionflags.flags.KeyPress = KEY_DOWN;
		else if (keypressed == "pgup")
			actionflags.flags.KeyPress = KEY_PGUP;
		else if (keypressed == "pgdown")
			actionflags.flags.KeyPress = KEY_PGDOWN;
		else if (keypressed == "tab")
			actionflags.flags.KeyPress = KEY_TAB;
		else if (keypressed == "escape")
			actionflags.flags.KeyPress = KEY_ESCAPE;
		else
		{
			actionflags.flags.KeyPress = std::stoi(keypressed);
		}
	}
	}

	for (auto& i : flagsset)
	{
		if (i == "condidletooverdown")
			actionflags.flags.CondIdleToOverDown = true;
		else if (i == "condidletooverup")
			actionflags.flags.CondIdleToOverUp = true;
		else if (i == "condoutdowntoidle")
			actionflags.flags.CondOutDownToIdle = true;
		else if (i == "condoutdowntooverdown")
			actionflags.flags.CondOutDownToOverDown = true;
		else if (i == "condoverdowntoidle")
			actionflags.flags.CondOverDownToIdle = true;
		else if (i == "condoverdowntooutdown")
			actionflags.flags.CondOverDownToOutDown = true;
		else if (i == "condoverdowntooverup")
			actionflags.flags.CondOverDownToOverUp = true;
		else if (i == "condoveruptoidle")
			actionflags.flags.CondOverUpToIdle = true;
		else if (i == "condoveruptooverdown")
			actionflags.flags.CondOverUpToOverDown = true;
		else
			std::cout << "Unknown ButtonActionFlag: " << i << std::endl;
	}

	return actionflags.value;
}