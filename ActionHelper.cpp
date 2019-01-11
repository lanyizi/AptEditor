#include "ActionHelper.hpp"

void ActionHelper::AddAction(action_type action, ActionBytes *ab)
{
	unsigned char c = (unsigned char)action;
	ab->actionbytes << c;
	ab->actionbytecount++;
}

void ActionHelper::AddIntAction(action_type action, ActionBytes *ab, int actionvalue)
{
	unsigned char c = (unsigned char)action;
	ab->actionbytes << c;
	ab->actionbytecount++;
	unsigned int x = GETALIGN(ab->actionbytecount);
	c = 0;
	for (unsigned int i = 0; i < x; i++)
	{
		ab->actionbytes << c;
		ab->actionbytecount++;
	}
	c = GetByte(actionvalue, 0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(actionvalue, 1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(actionvalue, 2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(actionvalue, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
}

void ActionHelper::AddStringAction(action_type action, ActionBytes *ab, const char *actionstring)
{
	unsigned char c = (unsigned char)action;
	ab->actionbytes << c;
	ab->actionbytecount++;
	unsigned int x = GETALIGN(ab->actionbytecount);
	c = 0;
	for (unsigned int i = 0; i < x; i++)
	{
		ab->actionbytes << c;
		ab->actionbytecount++;
	}
	ActionString *s = new ActionString;
	s->stringoffset = ab->actionbytecount;
	s->string = _strdup(actionstring);
	ab->actionstrings.push_back(s);
	ab->stringcount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
}

void ActionHelper::AddByteAction(action_type action, ActionBytes *ab, unsigned char number)
{
	unsigned char c = (unsigned char)action;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << number;
	ab->actionbytecount++;
}

void ActionHelper::AddShortAction(action_type action, ActionBytes *ab, unsigned short number)
{
	unsigned char c = (unsigned char)action;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << GetByte(number, 0);
	ab->actionbytecount++;
	ab->actionbytes << GetByte(number, 1);
	ab->actionbytecount++;
}

void ActionHelper::AddFloatAction(action_type action, ActionBytes *ab, float number)
{
	unsigned char c = (unsigned char)action;
	unsigned int n = *(unsigned int*)&number;
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(n, 0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(n, 1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(n, 2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(n, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
}

void ActionHelper::AddLongAction(action_type action, ActionBytes *ab, unsigned long number)
{
	unsigned char c = (unsigned char)action;
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(number, 0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(number, 1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(number, 2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(number, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
}

void ActionHelper::AddURLAction(action_type action, ActionBytes *ab, const char *str1, const char *str2)
{
	unsigned char c = (unsigned char)action;
	ab->actionbytes << c;
	ab->actionbytecount++;
	unsigned int x = GETALIGN(ab->actionbytecount);
	c = 0;
	for (unsigned int i = 0; i < x; i++)
	{
		ab->actionbytes << c;
		ab->actionbytecount++;
	}
	
	ActionString *s = new ActionString;
	s->stringoffset = ab->actionbytecount;
	s->string = _strdup(str1);
	ab->actionstrings.push_back(s);
	ab->stringcount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ActionString *s2 = new ActionString;
	s2->stringoffset = ab->actionbytecount;
	s2->string = _strdup(str2);
	ab->actionstrings.push_back(s2);
	ab->stringcount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
}

void ActionHelper::AddConstantPoolAction(action_type action, ActionBytes *ab, unsigned int constantcount)
{
	unsigned char c = (unsigned char)action;
	ab->actionbytes << c;
	ab->actionbytecount++;
	unsigned int x = GETALIGN(ab->actionbytecount);
	c = 0;
	for (unsigned int i = 0; i < x; i++)
	{
		ab->actionbytes << c;
		ab->actionbytecount++;
	}
	c = GetByte(constantcount, 0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(constantcount, 1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(constantcount, 2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(constantcount, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = 0;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
}

void ActionHelper::AddDefineFunction2Action(action_type action, ActionBytes *ab, ActionDefineFunction2 *pd, unsigned int flags, unsigned int size, const char *name)
{
	unsigned char c = (unsigned char)action;
	ab->actionbytes << c;
	ab->actionbytecount++;
	unsigned int x = GETALIGN(ab->actionbytecount);
	c = 0;
	for (unsigned int i = 0;i < x;i++)
	{
		ab->actionbytes << c;
		ab->actionbytecount++;
	}
	ActionString *s = new ActionString;
	s->stringoffset = ab->actionbytecount;
	s->string = _strdup(name);
	ab->actionstrings.push_back(s);
	ab->stringcount++;
	c = 0;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(pd->argumentcount,0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(pd->argumentcount,1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(pd->argumentcount, 2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(pd->argumentcount, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(flags,0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(flags,1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(flags,2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(flags,3);
	ab->actionbytes << c;
	ab->actionbytecount++;
	pd->definefunction2offset = ab->actionbytecount;
	c = 0;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(size,0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(size,1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(size,2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(size,3);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x98765432,0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x98765432, 1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x98765432, 2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x98765432, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x12345678,0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x12345678, 1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x12345678, 2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x12345678, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
}

void ActionHelper::AddDefineFunctionAction(action_type action, ActionBytes *ab, ActionDefineFunction *pd, unsigned int size, const char *name)
{
	unsigned char c = (unsigned char)action;
	ab->actionbytes << c;
	ab->actionbytecount++;
	unsigned int x = GETALIGN(ab->actionbytecount);
	c = 0;
	for (unsigned int i = 0; i < x; i++)
	{
		ab->actionbytes << c;
		ab->actionbytecount++;
	}
	ActionString *s = new ActionString;
	s->stringoffset = ab->actionbytecount;
	s->string = _strdup(name);
	ab->actionstrings.push_back(s);
	ab->stringcount++;
	c = 0;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(pd->argumentcount, 0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(pd->argumentcount, 1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(pd->argumentcount, 2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(pd->argumentcount, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
	pd->definefunctionoffset = ab->actionbytecount;
	c = 0;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(size, 0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(size, 1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(size, 2);;
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(size, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x98765432, 0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x98765432, 1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x98765432, 2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x98765432, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x12345678, 0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x12345678, 1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x12345678, 2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(0x12345678, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
}

void ActionHelper::AddPushDataAction(action_type action, ActionBytes *ab, ActionPushData *pd)
{
	unsigned char c = (unsigned char)action;
	ab->actionbytes << c;
	ab->actionbytecount++;
	unsigned int x = GETALIGN(ab->actionbytecount);
	c = 0;
	for (unsigned int i = 0; i < x; i++)
	{
		ab->actionbytes << c;
		ab->actionbytecount++;
	}
	c = GetByte(pd->pushdatacount, 0);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(pd->pushdatacount, 1);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(pd->pushdatacount, 2);
	ab->actionbytes << c;
	ab->actionbytecount++;
	c = GetByte(pd->pushdatacount, 3);
	ab->actionbytes << c;
	ab->actionbytecount++;
	pd->pushdataoffset = ab->actionbytecount;
	c = 0;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
	ab->actionbytes << c;
	ab->actionbytecount++;
}

void ActionHelper::APT_ProcessActions(tinyxml2::XMLDocument& doc,tinyxml2::XMLElement* parent, uint8_t *actions, uint8_t *aptaptdata,  AptConstData* data, uint8_t* aptbuffer)
{
	uint8_t *a = actions;
	bool is_func = false;
    uint16_t df_size = 0;
    uint32_t df_ptr = 0;
    tinyxml2::XMLElement* df_node = nullptr;

	ActionHelper::action_type action;
	do
	{
		action = (ActionHelper::action_type)(*a);
		a++;
		switch (action)
		{
		case ActionHelper::ACTION_BRANCHALWAYS:
		{
			ALIGN(a);
            auto node = doc.NewElement("branchalways");
            node->SetAttribute("offset", *(int32_t *)a);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
			a += 4;
		}
			break;
		case ACTION_BRANCHIFTRUE:
		{
			ALIGN(a);
            auto node = doc.NewElement("branchiftrue");
            node->SetAttribute("offset", *(int32_t *)a);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
			a += 4;
		}
			break;
		case EA_BRANCHIFFALSE:
		{
			ALIGN(a);
            auto node = doc.NewElement("branchiffalse");
            node->SetAttribute("offset", *(int32_t *)a);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
			a += 4;
		}
			break;
		case ACTION_GOTOFRAME:
		{
			ALIGN(a);
            auto node = doc.NewElement("gotoframe");
            node->SetAttribute("frame", *(int32_t *)a);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
			a += 4;
		}
			break;
		case ACTION_SETREGISTER:
		{
			ALIGN(a);
            auto node = doc.NewElement("setregister");
            node->SetAttribute("reg", *(int32_t *)a);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
			a += 4;
		}
			break;
		case ACTION_WITH:
		{
			ALIGN(a);
            auto node = doc.NewElement("with");
            node->SetAttribute("pos", *(int32_t *)a);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
			a += 4;
		}
			break;
		case ACTION_GOTOEXPRESSION:
		{
			ALIGN(a);
            auto node = doc.NewElement("gotoexpression");
            node->SetAttribute("pos", *(int32_t *) a);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
			a += 4;
		}
			break;
		case ACTION_GETURL:
		{
			ALIGN(a);
			add(*(uint32_t *)a);
			char *str1 = (char *)(*(uint32_t *)a);
			a += 4;
			add(*(uint32_t *)a);
			char *str2 = (char *)(*(uint32_t *)a);
			a += 4;
            auto node = doc.NewElement("geturl");
            node->SetAttribute("str1", str1);
            node->SetAttribute("str2", str2);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_CONSTANTPOOL:
		{
			ALIGN(a);
			uint32_t count = *(uint32_t *)a;
			a += 4;
			add(*(uint32_t *)a);
			uint32_t *cpd = *(uint32_t **)a;
			a += 4;
            auto node = doc.NewElement("constantpool");

			for (uint32_t i = 0; i < count; i++)
			{
                auto node2 = doc.NewElement("constant");
                node2->SetAttribute("id", i);
				if (data->items[cpd[i]]->type == TYPE_NUMBER)
					node2->SetAttribute("integer",data->items[cpd[i]]->numvalue);
				else if (data->items[cpd[i]]->type == TYPE_STRING)
                    node2->SetAttribute("string", data->items[cpd[i]]->strvalue);

                node->InsertEndChild(node2);
			}
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_PUSHDATA:
		{
			ALIGN(a);
			uint32_t count = *(uint32_t *)a;
			a += 4;
			add(*(uint32_t *)a);
			uint32_t *pid = *(uint32_t **)a;
			a += 4;
            auto node = doc.NewElement("pushdata");

			for (uint32_t i = 0; i < count; i++)
			{
                auto node2 = doc.NewElement("data");
                node2->SetAttribute("id", pid[i]);

				if (data->items[pid[i]]->type == TYPE_NUMBER)
                    node2->SetAttribute("integer", data->items[pid[i]]->numvalue);
				else if (data->items[pid[i]]->type == TYPE_STRING)
                    node2->SetAttribute("string", data->items[pid[i]]->strvalue);

                node->InsertEndChild(node2);
			}
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_DEFINEFUNCTION2:
		{
			ALIGN(a);
			add(*(uint32_t *)a);
			char *name = (char *)(*(uint32_t *)a);
			a += 4;
			uint32_t count = *(uint32_t *)a;
			a += 4;
			uint32_t flags = *(uint32_t *)a;
			a += 4;
			add(*(uint32_t *)a);
			FunctionArgument *args = (FunctionArgument *)(*(uint32_t *)a);
			a += 4;
			uint32_t size = *(uint32_t *)a;
            auto node = doc.NewElement("definefunction2");
            node->SetAttribute("name", name);
            node->SetAttribute("flags", flags);
            node->SetAttribute("size", size);

			for (uint32_t i = 0; i < count; i++)
			{
				add(args[i].name);
                auto node2 = doc.NewElement("argument");
                node2->SetAttribute("reg", args[i].reg);
                node2->SetAttribute("name", args[i].name);
                node->InsertEndChild(node2);
			}
            if (size > 0)
            {
                auto node2 = doc.NewElement("body");
                node->InsertEndChild(node2);
                df_node = node2;
                df_size = size;
                is_func = true;
            }
            parent->InsertEndChild(node);

			a += 4;
			a += 4;
			a += 4;
            df_ptr = (uint32_t)a;
            
		}
			break;
		case ACTION_DEFINEFUNCTION:
		{
			ALIGN(a);
			add(*(uint32_t *)a);
			char *name = (char *)(*(uint32_t *)a);
			a += 4;
			uint32_t count = *(uint32_t *)a;
			a += 4;
			add(*(uint32_t *)a);
			char **args = (char **)(*(uint32_t *)a);
			a += 4;
			uint32_t size = *(uint32_t *)a;
            auto node = doc.NewElement("definefunction");
            node->SetAttribute("name", name);
            node->SetAttribute("size", size);
            
			for (uint32_t i = 0; i < count; i++)
			{
				add(args[i]);
                auto node2 = doc.NewElement("argument");
                node2->SetAttribute("name", args[i]);
                node->InsertEndChild(node2);
			}
			a += 4;
			a += 4;
			a += 4;

            if (size > 0)
            {
                auto node2 = doc.NewElement("body");
                node->InsertEndChild(node2);
                df_node = node2;
                df_size = size;
                is_func = true;
            }
            parent->InsertEndChild(node);
            df_ptr = (uint32_t)a;
		}
			break;
		case EA_PUSHSTRING:
		{
			ALIGN(a);
			add(*(uint32_t *)a);
			char *str = (char *)(*(uint32_t *)a);
			a += 4;
            auto node = doc.NewElement("pushstring");
            node->SetAttribute("str", str);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

		}
			break;
		case EA_GETSTRINGVAR:
		{
			ALIGN(a);
			add(*(uint32_t *)a);
			char *str = (char *)(*(uint32_t *)a);
			a += 4;
            auto node = doc.NewElement("getstringvar");
            node->SetAttribute("str", str);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case EA_GETSTRINGMEMBER:
		{
			ALIGN(a);
			add(*(uint32_t *)a);
			char *str = (char *)(*(uint32_t *)a);
			a += 4;
            auto node = doc.NewElement("getstringmember");
            node->SetAttribute("str", str);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case EA_SETSTRINGVAR:
		{
			ALIGN(a);
			add(*(uint32_t *)a);
			char *str = (char *)(*(uint32_t *)a);
			a += 4;
            auto node = doc.NewElement("setstringvar");
            node->SetAttribute("str", str);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);   
		}
			break;
		case EA_SETSTRINGMEMBER:
		{
			ALIGN(a);
			add(*(uint32_t *)a);
			char *str = (char *)(*(uint32_t *)a);
			a += 4;
            auto node = doc.NewElement("setstringmember");
            node->SetAttribute("str", str);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_SETTARGET:
		{
			ALIGN(a);
			add(*(uint32_t *)a);
			char *str = (char *)(*(uint32_t *)a);
			a += 4;
            auto node = doc.NewElement("settarget");
            node->SetAttribute("str", str);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_GOTOLABEL:
		{
			ALIGN(a);
			add(*(uint32_t *)a);
			char *str = (char *)(*(uint32_t *)a);
			a += 4;
            auto node = doc.NewElement("gotolabel");
            node->SetAttribute("label", str);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case EA_CALLNAMEDFUNCTIONPOP:
		{
			uint8_t val = *(uint8_t *)a;
            auto node = doc.NewElement("callnamedfuncpop");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 1;
		}
			break;
		case EA_CALLNAMEDFUNCTION:
		{
			uint8_t val = *(uint8_t *)a;
            auto node = doc.NewElement("callnamedfunc");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 1;
		}
			break;
		case EA_CALLNAMEDMETHODPOP:
		{
			uint8_t val = *(uint8_t *)a;
            auto node = doc.NewElement("callnamedmethodpop");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 1;
		}
			break;
		case EA_CALLNAMEDMETHOD:
		{
			uint8_t val = *(uint8_t *)a;
            auto node = doc.NewElement("callnamedmethod");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 1;
		}
			break;
		case EA_PUSHCONSTANT:
		{
			uint8_t val = *(uint8_t *)a;
            auto node = doc.NewElement("pushconstant");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 1;
		}
			break;
		case EA_PUSHVALUEOFVAR:
		{
			uint8_t val = *(uint8_t *)a;
            auto node = doc.NewElement("pushvalue");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 1;
		}
			break;
		case EA_GETNAMEDMEMBER:
		{
			uint8_t val = *(uint8_t *)a;
            auto node = doc.NewElement("getnamedmember");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 1;
		}
			break;
		case EA_PUSHBYTE:
		{
			uint8_t val = *(uint8_t *)a;
            auto node = doc.NewElement("pushbyte");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 1;
		}
			break;
		case EA_PUSHREGISTER:
		{
            uint8_t val = *(uint8_t *)a;
            auto node = doc.NewElement("pushregister");
            node->SetAttribute("val", action);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 1;
		}
			break;
		case EA_PUSHWORDCONSTANT:
		{
			uint16_t val = *(uint16_t *)a;
            auto node = doc.NewElement("pushwordconstant");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 2;
		}
		case EA_PUSHSHORT:
		{
            uint16_t val = *(uint16_t *)a;
            auto node = doc.NewElement("pushshort");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 2;
		}
			break;
		case EA_PUSHFLOAT:
		{
            float val = *(float *)a;
            auto node = doc.NewElement("pushfloat");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);

			a += 4;
		}
			break;
		case EA_PUSHLONG:
		{
            uint32_t val = *(uint32_t*)a;
            auto node = doc.NewElement("pushvalue");
            node->SetAttribute("val", val);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
			a += 4;
		}
			break;
		case ACTION_END:
		{
            auto node = doc.NewElement("end");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_LOGICALAND:
		{
            auto node = doc.NewElement("logicaland");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_LOGICALOR:
		{
            auto node = doc.NewElement("logicalor");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_LOGICALNOT:
		{
            auto node = doc.NewElement("logicalnot");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case EA_PUSHONE:
		{
            auto node = doc.NewElement("pushone");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_TRACE:
		{
            auto node = doc.NewElement("trace");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_NEW:
		{
            auto node = doc.NewElement("new");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_SETMEMBER:
		{
            auto node = doc.NewElement("setmember");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case EA_PUSHZERO:
		{
            auto node = doc.NewElement("pushzero");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_POP:
		{
            auto node = doc.NewElement("pop");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_GETMEMBER:
		{
            auto node = doc.NewElement("getmember");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_DUP:
		{
            auto node = doc.NewElement("dup");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_NEWADD:
		{
            auto node = doc.NewElement("newadd");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_NEWLESSTHAN:
		{
            auto node = doc.NewElement("newlessthan");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_NEWEQUALS:
		{
            auto node = doc.NewElement("newequals");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case EA_PUSHTRUE:
		{
            auto node = doc.NewElement("pushtrue");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case EA_PUSHFALSE:
		{
            auto node = doc.NewElement("pushfalse");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case EA_PUSHNULL:
		{
            auto node = doc.NewElement("pushnull");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case EA_PUSHUNDEFINED:
		{
            auto node = doc.NewElement("pushundefined");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_INCREMENT:
		{
            auto node = doc.NewElement("increment");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_DECREMENT:
		{
            auto node = doc.NewElement("decrement");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_DEFINELOCAL:
		{
            auto node = doc.NewElement("definelocal");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_GREATER:
		{
            auto node = doc.NewElement("greater");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case EA_PUSHTHIS:
		{
            auto node = doc.NewElement("pushthis");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case EA_PUSHGLOBAL:
		{
            auto node = doc.NewElement("pushglobal");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_GETVARIABLE:
		{
            auto node = doc.NewElement("getvariable");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_SETVARIABLE:
		{
            auto node = doc.NewElement("setvariable");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		case ACTION_WAITFORFRAME:
		{
            auto node = doc.NewElement("waitforframe");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
        case ACTION_GETURL2:
        {
            auto node = doc.NewElement("geturl2");
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
        }
            break;
		default:
		{
            auto node = doc.NewElement("noarg");
            node->SetAttribute("action", action);
            (is_func) ? df_node->InsertEndChild(node) : parent->InsertEndChild(node);
		}
			break;
		}

        if (df_ptr + df_size == (uint32_t)a && is_func)
        {
            is_func = false;
        }
	} while (action != ACTION_END);
}

void ActionHelper::XML_ProcessActions(tinyxml2::XMLElement* entry, ActionBytes *ab, AptConstData* data)
{
    tinyxml2::XMLElement* entryt;

	for (entryt = entry->FirstChildElement(); entryt != 0; entryt = entryt->NextSiblingElement())
	{
		std::string actionname = entryt->Value();

		if (actionname == "branchalways")
		{
			unsigned int pos = entryt->IntAttribute("offset");
			AddIntAction(ACTION_BRANCHALWAYS, ab, pos);
		}
		else if (actionname == "branchiftrue")
		{
			unsigned int pos = entryt->IntAttribute("offset");
			AddIntAction(ACTION_BRANCHIFTRUE, ab, pos);
		}
		else if (actionname == "branchiffalse")
		{
			unsigned int pos = entryt->IntAttribute("offset");
			AddIntAction(EA_BRANCHIFFALSE, ab, pos);
		}
		else if (actionname == "gotoframe")
		{
			unsigned int pos = entryt->IntAttribute("frame");
			AddIntAction(ACTION_GOTOFRAME, ab, pos);
		}
		else if (actionname == "setregister")
		{
			unsigned int pos = entryt->IntAttribute("reg");
			AddIntAction(ACTION_SETREGISTER, ab, pos);
		}
		else if (actionname == "with")
		{
			unsigned int pos = entryt->IntAttribute("pos");
			AddIntAction(ACTION_WITH, ab, pos);
		}
		else if (actionname == "gotoexpression")
		{
			unsigned int pos = entryt->IntAttribute("pos");
			AddIntAction(ACTION_GOTOEXPRESSION, ab, pos);
		}
		else if (actionname == "geturl")
		{
			const char *str1 = entryt->Attribute("str1");
			const char *str2 = entryt->Attribute("str2");
			AddURLAction(ACTION_GETURL, ab, str1, str2);
		}
		else if (actionname == "constantpool")
		{
			tinyxml2::XMLElement* entryt2;
			for (entryt2 = entryt->FirstChildElement(); entryt2 != 0; entryt2 = entryt2->NextSiblingElement())
			{
				ab->constantcount++;
				AptConstItem* aci = new AptConstItem();
				if (entryt2->Attribute("string"))
				{
					aci->type = TYPE_STRING;
					aci->strvalue = entryt2->Attribute("string");
				}
				else
				{
					aci->type = TYPE_NUMBER;
					aci->numvalue = entryt2->IntAttribute("integer");
				}

				data->items.push_back(aci);
				ab->constants.push_back(data->itemcount++);
			}
			AddConstantPoolAction(ACTION_CONSTANTPOOL, ab, ab->constantcount);
		}
		else if (actionname == "pushdata")
		{
			ab->pushdatacount++;
			ActionPushData *pd = new ActionPushData;
			ab->actionpushdatas.push_back(pd);
			pd->pushdataoffset = ab->actionbytecount;
			pd->pushdatacount = 0;
			tinyxml2::XMLElement* entryt2;
			for (entryt2 = entryt->FirstChildElement(); entryt2 != 0; entryt2 = entryt2->NextSiblingElement())
			{
				pd->pushdatacount++;
				AptConstItem* aci = new AptConstItem();
				if (entryt2->Attribute("string"))
				{
					aci->type = TYPE_STRING;
					aci->strvalue = entryt2->Attribute("string");
				}
				else
				{
					aci->type = TYPE_NUMBER;
					aci->numvalue = entryt2->IntAttribute("integer");
				}

				data->items.push_back(aci);
				pd->pushdata.push_back(data->itemcount++);
			}
			AddPushDataAction(ACTION_PUSHDATA, ab, pd);
		}
		else if (actionname == "definefunction2")
		{
			ab->definefunction2count++;
			ActionDefineFunction2 *pd = new ActionDefineFunction2;
			ab->actiondefinefunction2s.push_back(pd);
			pd->definefunction2offset = ab->actionbytecount;
			pd->argumentcount = 0;
			tinyxml2::XMLElement* entryt2;
			unsigned int flags = entryt->IntAttribute("flags");
			unsigned int size = entryt->IntAttribute("size");
			const char *name = entryt->Attribute("name");

            for (entryt2 = entryt->FirstChildElement("argument"); entryt2 != 0; entryt2 = entryt2->NextSiblingElement("argument"))
            {
                pd->argumentcount++;
                FunctionArgument *a = new FunctionArgument;
                a->reg = entryt2->IntAttribute("reg");
                a->name = entryt2->Attribute("name");
                pd->arguments.push_back(a);
            }

            AddDefineFunction2Action(ACTION_DEFINEFUNCTION2, ab, pd, flags, size, name);

            if (entryt2 = entryt->FirstChildElement("body"))
            {
                XML_ProcessActions(entryt2, ab, data);
            }          			        
         
		}
		else if (actionname == "definefunction")
		{
			ab->definefunctioncount++;
			ActionDefineFunction *adf = new ActionDefineFunction;
			ab->actiondefinefunctions.push_back(adf);
			adf->definefunctionoffset = ab->actionbytecount;
			adf->argumentcount = 0;
			tinyxml2::XMLElement* entryt2;
			unsigned int size = entryt->IntAttribute("size");
			const char *name = _strdup(entryt->Attribute("name"));

            for (entryt2 = entryt->FirstChildElement("argument"); entryt2 != 0; entryt2 = entryt2->NextSiblingElement("argument"))
            {
                adf->argumentcount++;
                adf->arguments.push_back(entryt2->Attribute("name"));
            }

            AddDefineFunctionAction(ACTION_DEFINEFUNCTION, ab, adf, size, name);

            if (entryt2 = entryt->FirstChildElement("body"))
            {
                XML_ProcessActions(entryt2, ab, data);
            }
		
		}
		else if (actionname == "pushstring")
		{
			const char *str = entryt->Attribute("str");
			AddStringAction(EA_PUSHSTRING, ab, str);
		}
		else if (actionname == "getstringvar")
		{
			const char *str = entryt->Attribute("str");
			AddStringAction(EA_GETSTRINGVAR, ab, str);
		}
		else if (actionname == "getstringmember")
		{
			const char *str = entryt->Attribute("str");
			AddStringAction(EA_GETSTRINGMEMBER, ab, str);
		}
		else if (actionname == "setstringvar")
		{
			const char *str = entryt->Attribute("str");
			AddStringAction(EA_SETSTRINGVAR, ab, str);
		}
		else if (actionname == "setstringmember")
		{
			const char *str = entryt->Attribute("str");
			AddStringAction(EA_SETSTRINGMEMBER, ab, str);
		}
		else if (actionname == "settarget")
		{
			const char *str = entryt->Attribute("str");
			AddStringAction(ACTION_SETTARGET, ab, str);
		}
		else if (actionname == "gotolabel")
		{
			const char *str = entryt->Attribute("label");
			AddStringAction(ACTION_GOTOLABEL, ab, str);
		}
		else if (actionname == "callnamedfuncpop")
		{
			unsigned int val = entryt->IntAttribute("val");
			AddByteAction(EA_CALLNAMEDFUNCTIONPOP, ab, (unsigned char)val);
		}
		else if (actionname == "callnamedfunc")
		{
			unsigned int val = entryt->IntAttribute("val");
			AddByteAction(EA_CALLNAMEDFUNCTION, ab, (unsigned char)val);
		}
		else if (actionname == "callnamedmethodpop")
		{
			unsigned int val = entryt->IntAttribute("val");
			AddByteAction(EA_CALLNAMEDMETHODPOP, ab, (unsigned char)val);
		}
		else if (actionname == "callnamedmethod")
		{
			unsigned int val = entryt->IntAttribute("val");
			AddByteAction(EA_CALLNAMEDMETHOD, ab, (unsigned char)val);
		}
		else if (actionname == "pushconstant")
		{
			unsigned int val = entryt->IntAttribute("val");
			AddByteAction(EA_PUSHCONSTANT, ab, (unsigned char)val);
		}
		else if (actionname == "pushvalue")
		{
			unsigned int val = entryt->IntAttribute("val");
			AddByteAction(EA_PUSHVALUEOFVAR, ab, (unsigned char)val);
		}
		else if (actionname == "pushbyte")
		{
			unsigned int val = entryt->IntAttribute("val");
			AddByteAction(EA_PUSHBYTE, ab, (unsigned char)val);
		}
		else if (actionname == "getnamedmember")
		{
			unsigned int val = entryt->IntAttribute("val");
			AddByteAction(EA_GETNAMEDMEMBER, ab, (unsigned char)val);
		}
		else if (actionname == "pushregister")
		{
			unsigned int val = entryt->IntAttribute("val");
			AddByteAction(EA_PUSHREGISTER, ab, (unsigned char)val);
		}
		else if (actionname == "pushshort")
		{
			unsigned int val = entryt->IntAttribute("val");;
			AddShortAction(EA_PUSHSHORT, ab, (unsigned short)val);
		}
		else if (actionname == "pushwordconstant")
		{
			unsigned int val = entryt->IntAttribute("val");;
			AddShortAction(EA_PUSHWORDCONSTANT, ab, (unsigned short)val);
		}
		else if (actionname == "pushfloat")
		{
			float val = entryt->FloatAttribute("val");
			AddFloatAction(EA_PUSHFLOAT, ab, val);
		}
		else if (actionname == "pushlong")
		{
			unsigned int val = entryt->IntAttribute("val");
			AddLongAction(EA_PUSHLONG, ab, val);
		}
		else if (actionname == "logicaland")
		{
			AddAction(ACTION_LOGICALAND, ab);
		}
		else if (actionname == "logicalor")
		{
			AddAction(ACTION_LOGICALOR, ab);
		}
		else if (actionname == "logicalnot")
		{
			AddAction(ACTION_LOGICALNOT, ab);
		}
		else if (actionname == "pushone")
		{
			AddAction(EA_PUSHONE, ab);
		}
		else if (actionname == "trace")
		{
			AddAction(ACTION_TRACE,ab);
		}
		else if (actionname == "new")
		{
			AddAction(ACTION_NEW, ab);
		}
		else if (actionname == "setmember")
		{
			AddAction(ACTION_SETMEMBER, ab);
		}
		else if (actionname == "pushzero")
		{
			AddAction(EA_PUSHZERO, ab);
		}
		else if (actionname == "pop")
		{
			AddAction(ACTION_POP, ab);
		}
		else if (actionname == "getmember")
		{
			AddAction(ACTION_GETMEMBER, ab);
		}
		else if (actionname == "dup")
		{
			AddAction(ACTION_DUP, ab);
		}
		else if (actionname == "newadd")
		{
			AddAction(ACTION_NEWADD, ab);
		}
		else if (actionname == "newlessthan")
		{
			AddAction(ACTION_NEWLESSTHAN, ab);
		}
		else if (actionname == "newequals")
		{
			AddAction(ACTION_NEWEQUALS, ab);
		}
		else if (actionname == "pushtrue")
		{
			AddAction(EA_PUSHTRUE, ab);
		}
		else if (actionname == "pushfalse")
		{
			AddAction(EA_PUSHFALSE, ab);
		}
		else if (actionname == "pushnull")
		{
			AddAction(EA_PUSHNULL, ab);
		}
		else if (actionname == "pushundefined")
		{
			AddAction(EA_PUSHUNDEFINED, ab);
		}
		else if (actionname == "increment")
		{
			AddAction(ACTION_INCREMENT, ab);
		}
		else if (actionname == "decrement")
		{
			AddAction(ACTION_DECREMENT, ab);
		}
		else if (actionname == "definelocal")
		{
			AddAction(ACTION_DEFINELOCAL, ab);
		}
		else if (actionname == "greater")
		{
			AddAction(ACTION_GREATER, ab);
		}
		else if (actionname == "pushthis")
		{
			AddAction(EA_PUSHTHIS, ab);
		}
		else if (actionname == "pushglobal")
		{
			AddAction(EA_PUSHGLOBAL, ab);
		}
		else if (actionname == "getvariable")
		{
			AddAction(ACTION_GETVARIABLE, ab);
		}
		else if (actionname == "setvariable")
		{
			AddAction(ACTION_SETVARIABLE, ab);
		}
        else if (actionname == "geturl2")
        {
            AddAction(ACTION_GETURL2, ab);
        }
        else if (actionname == "end")
        {
            AddAction(ACTION_END, ab);
        }
		else if (actionname == "noarg")
		{
			unsigned int action = entryt->IntAttribute("action");
			AddAction((action_type)action, ab);
		}	
		else
		{
			std::cout << "Unknown action: " << actionname << std::endl;
		}
	}
}
