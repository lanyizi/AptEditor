#include "Aptfile.hpp"
#include "Util.hpp"
#include <Windows.h>
#include <assert.h>

bool AptFile::Convert(std::string filename)
{
	std::tr2::sys::path file(filename);
	if (file.extension() == ".xml")
		return XMLToApt(filename);
	else if (file.extension() == ".apt")
		return AptToXML(filename);
    else if (file.extension() == ".big")
        return AptToXML(filename);
	else
	{
		std::cout << "Please give either an .xml or an .apt file to convert" << std::endl;
		return false;
	}
}


bool AptFile::AptToXML(std::string filename)
{
	std::tr2::sys::path aptfile(filename);
	std::tr2::sys::path constfile(aptfile.basename() + ".const");
	std::tr2::sys::path xmlfile(aptfile.basename() + ".xml");

	if (!fileExists(aptfile))
	{
		std::cout << "Sorry but " << aptfile.filename() << " is missing" << std::endl;
		return false;
	}

	if (!fileExists(constfile))
	{
		std::cout << "Sorry but " << constfile.filename() << " is missing" << std::endl;
		return false;
	}

	//read the const file into a buffer
	std::ifstream conststream(constfile, std::ios::binary | std::ios::in);
	uint32_t constsize = size(conststream);
	uint8_t* constbuffer = new uint8_t[constsize];
	conststream.read((char*)constbuffer, constsize);
	conststream.close();

	//read the apt file into a buffer
	std::ifstream aptstream(aptfile, std::ios::binary | std::ios::in);
	uint32_t aptsize = size(aptstream);
	uint8_t* aptbuffer = new uint8_t[aptsize];
	aptstream.read((char*)aptbuffer, aptsize);
	aptstream.close();

	//our data
	auto data = new AptConstData;

	//now start parsing by setting our iter to the start of the aptbuffer
	uint8_t* iter = constbuffer;
	//skip the first 20 bytes
	iter += 0x14;
	data->aptdataoffset = ReadUint(iter);
	data->itemcount = ReadUint(iter);
	//skip 4 bytes now
	iter += 4;

	AptConstItem *aci = (AptConstItem *)iter;
	//8 bytes per const item
	iter += data->itemcount * 8;
	//now read each item
	for (uint32_t i = 0; i < data->itemcount; ++i)
	{
		data->items.push_back(new AptConstItem);
		data->items[i]->type = aci->type;
		if (data->items[i]->type == TYPE_STRING)
		{
			//read the string from the position specified
			data->items[i]->strvalue = (char *)(constbuffer + (uint32_t)aci->strvalue);
		}
		else
		{
			data->items[i]->numvalue = aci->numvalue;
		}
		aci++;
	}

	//now parse the .apt file
	OutputMovie *m = (OutputMovie *)(aptbuffer + data->aptdataoffset);
	add(m->characters);
	add(m->exports);
	add(m->imports);
	add(m->frames);
    tinyxml2::XMLDocument doc;
    auto declaration = doc.NewDeclaration();
    doc.InsertFirstChild(declaration);

    auto entry = doc.NewElement("aptdata");
    auto entry2 = doc.NewElement("movieclip");
    {
        auto entry3 = doc.NewElement("imports");
        for (uint32_t i = 0; i < m->importcount; i++)
        {
            add(m->imports[i].movie);
            add(m->imports[i].name);
            auto entry4 = doc.NewElement("import");
            entry4->SetAttribute("name", m->imports[i].name);
            entry4->SetAttribute("movie", m->imports[i].movie);
            entry4->SetAttribute("character", m->imports[i].character);

            entry3->InsertEndChild(entry4);
        }
        entry2->InsertEndChild(entry3);
    }
    {
        auto entry3 = doc.NewElement("exports");
        for (uint32_t i = 0; i < m->exportcount; i++)
        {
            add(m->exports[i].name);
            auto entry4 = doc.NewElement("export");
            entry4->SetAttribute("name", m->exports[i].name);
            entry4->SetAttribute("character", m->exports[i].character);
            entry3->InsertEndChild(entry4);
        }
        entry2->InsertEndChild(entry3);
    }
	{
        auto entry3 = doc.NewElement("frames");
		for (uint32_t i = 0; i < m->framecount; i++)
		{
			add(m->frames[i].frameitems);
            auto entry4 = doc.NewElement("frame");
            entry4->SetAttribute("id", i);
			for (uint32_t j = 0; j < m->frames[i].frameitemcount; j++)
			{
				add(m->frames[i].frameitems[j]);
				switch (m->frames[i].frameitems[j]->type)
				{
				case ACTION:
				{
                    auto entry5 = doc.NewElement("action");
					OutputAction *oa = (OutputAction *)m->frames[i].frameitems[j];
					add(oa->actionbytes);
					ActionHelper::APT_ProcessActions(doc,entry5, oa->actionbytes, aptbuffer, data, aptbuffer);
                    entry4->InsertEndChild(entry5);
				}
					break;
				case FRAMELABEL:
				{
					FrameLabel *fl = (FrameLabel *)m->frames[i].frameitems[j];
					add(fl->label);
                    auto entry5 = doc.NewElement("framelabel");
                    entry5->SetAttribute("label", fl->label);
                    entry5->SetAttribute("frame", fl->frame);
                    entry4->InsertEndChild(entry5);
				}
					break;
				case PLACEOBJECT:
				{
					OutputPlaceObject *po = (OutputPlaceObject *)m->frames[i].frameitems[j];
					int alpha = GetByte(po->colortransform, 3);
					int blue = GetByte(po->colortransform, 2);
					int green = GetByte(po->colortransform, 1);
					int red = GetByte(po->colortransform, 0);
                    auto entry5 = doc.NewElement("placeobject");
                    //set all attributes of this node
                    entry5->SetAttribute("depth", po->depth);
                    entry5->SetAttribute("character", po->character);
                    entry5->SetAttribute("rotm00", po->rotateandscale.m00);
                    entry5->SetAttribute("rotm01", po->rotateandscale.m01);
                    entry5->SetAttribute("rotm10", po->rotateandscale.m10);
                    entry5->SetAttribute("rotm11", po->rotateandscale.m11);
                    entry5->SetAttribute("tx", po->translate.X);
                    entry5->SetAttribute("ty", po->translate.Y);
                    entry5->SetAttribute("red", red);
                    entry5->SetAttribute("green", green);
                    entry5->SetAttribute("blue", blue);
                    entry5->SetAttribute("alpha", alpha);
                    entry5->SetAttribute("ratio", po->ratio);
                    entry5->SetAttribute("clipdepth", po->clipdepth);
                    entry5->SetAttribute("unknown", po->unknown);

					std::string flagstr = Flags::GetPOFlags_str(po->flags);
                    if (flagstr.size() > 0)
                    {
                        auto entry6 = doc.NewElement("poflags");
                        entry6->SetAttribute("value", flagstr.c_str());
                        entry5->InsertEndChild(entry6);
                    }                                 

					if (po->name)
					{
						add(po->name);
                        auto entry6 = doc.NewElement("poname");
                        entry6->SetAttribute("name", po->name);
                        entry5->InsertEndChild(entry6);
					}
					if (po->poa)
					{
						add(po->poa);
						add(po->poa->actions);
                        auto entry6 = doc.NewElement("clipactions");

						for (uint32_t k = 0; k < po->poa->clipactioncount; k++)
						{
                            auto entry7 = doc.NewElement("clipaction");
                            entry7->SetAttribute("flags", po->poa->actions[k].flags);
                            entry7->SetAttribute("flags2", po->poa->actions[k].flags2);
							add(po->poa->actions[k].actiondata);
							ActionHelper::APT_ProcessActions(doc,entry7 , po->poa->actions[k].actiondata, aptbuffer, data, aptbuffer);
                            entry6->InsertEndChild(entry7);
						}
                        entry5->InsertEndChild(entry6);

					}
                    entry4->InsertEndChild(entry5);
				}
					break;
				case REMOVEOBJECT:
				{
					RemoveObject *ro = (RemoveObject *)m->frames[i].frameitems[j];
                    auto entry5 = doc.NewElement("removeobject");
                    entry5->SetAttribute("depth", ro->depth);
                    entry4->InsertEndChild(entry5);
				}
					break;
				case BACKGROUNDCOLOR:
				{
					BackgroundColor *bg = (BackgroundColor *)m->frames[i].frameitems[j];
                    auto entry5 = doc.NewElement("background");
                    entry5->SetAttribute("color", (int)bg->color);
                    entry4->InsertEndChild(entry5);
				}
					break;
				case INITACTION:
				{
					OutputInitAction *oa = (OutputInitAction *)m->frames[i].frameitems[j];
					add(oa->actionbytes);
                    auto entry5 = doc.NewElement("initaction");
                    entry5->SetAttribute("sprite", oa->sprite);
					ActionHelper::APT_ProcessActions(doc,entry5, oa->actionbytes, aptbuffer, data, aptbuffer);
                    entry4->InsertEndChild(entry5);
				}
					break;
				}
			}
            entry3->InsertEndChild(entry4);
		}
        entry2->InsertEndChild(entry3);
	}
	for (uint32_t ch = 0; ch < m->charactercount; ch++)
	{
		if (m->characters[ch])
		{
			add(m->characters[ch]);
			switch (m->characters[ch]->type)
			{
			case SHAPE:
			{
				Shape *sh = (Shape *)m->characters[ch];
                auto entry3 = doc.NewElement("shape");
                entry3->SetAttribute("id", ch);
                entry3->SetAttribute("top", sh->bounds.top);
                entry3->SetAttribute("left", sh->bounds.left);
                entry3->SetAttribute("bottom", sh->bounds.bottom);
                entry3->SetAttribute("right", sh->bounds.right);
                entry3->SetAttribute("geometry", sh->geometry);
                entry->InsertEndChild(entry3);
			}
				break;
			case EDITTEXT:
			{
				EditText *et = (EditText *)m->characters[ch];
				int alpha = GetByte(et->color, 0);
				int blue = GetByte(et->color, 1);
				int green = GetByte(et->color, 2);
				int red = GetByte(et->color, 3);
                auto entry3 = doc.NewElement("edittext");
                entry3->SetAttribute("id", ch);
                entry3->SetAttribute("top", et->bounds.top);
                entry3->SetAttribute("left", et->bounds.left);
                entry3->SetAttribute("bottom", et->bounds.bottom);
                entry3->SetAttribute("right", et->bounds.right);
                entry3->SetAttribute("font", et->font);
                entry3->SetAttribute("alignment", et->alignment);
                entry3->SetAttribute("red", red);
                entry3->SetAttribute("green",green);
                entry3->SetAttribute("blue", blue);
                entry3->SetAttribute("alpha", alpha);
                entry3->SetAttribute("height", et->fontheight);
                entry3->SetAttribute("readonly", et->readonly);
                entry3->SetAttribute("multiline", et->multiline);
                entry3->SetAttribute("wordwrap", et->wordwrap);
				
				if (et->text)
				{
					add(et->text);
                    auto entry4 = doc.NewElement("ettext");
                    entry4->SetAttribute("text", et->text);
                    entry3->InsertEndChild(entry4);
				}
				if (et->variable)
				{
					add(et->variable);
                    auto entry4 = doc.NewElement("etvar");
                    entry4->SetAttribute("variable", et->variable);
                    entry3->InsertEndChild(entry4);
				}
                entry->InsertEndChild(entry3); 
			}
				break;
			case FONT:
			{
				OutputFont *fo = (OutputFont *)m->characters[ch];
				add(fo->name);
                auto entry3 = doc.NewElement("font");
                entry3->SetAttribute("id", ch);
                entry3->SetAttribute("name", fo->name);

				if (fo->glyphcount)
				{
					add(fo->glyphs);
                    auto entry4 = doc.NewElement("glyphs");

					for (uint32_t i = 0; i < fo->glyphcount; i++)
					{
                        auto entry5 = doc.NewElement("glyph");
                        entry5->SetAttribute("id", fo->glyphs[i]);
                        entry4->InsertEndChild(entry5);
					}
                    entry3->InsertEndChild(entry4);
				}
                entry->InsertEndChild(entry3);
			}
				break;
			case BUTTON:
			{
				OutputButton *ob = (OutputButton *)m->characters[ch];

                auto entry3 = doc.NewElement("button");
                entry3->SetAttribute("id", ch);
                entry3->SetAttribute("top", ob->bounds.top);
                entry3->SetAttribute("left", ob->bounds.left);
                entry3->SetAttribute("bottom", ob->bounds.bottom);
                entry3->SetAttribute("right", ob->bounds.right);
				if (ob->trianglecount)
				{
					add(ob->vertexes);
					add(ob->triangles);
                    auto entry4 = doc.NewElement("vertexes");
					for (uint32_t i = 0; i < ob->vertexcount; i++)
					{
                        auto entry5 = doc.NewElement("vertex");
                        entry5->SetAttribute("x", ob->vertexes[i].X);
                        entry5->SetAttribute("y", ob->vertexes[i].Y);
                        entry4->InsertEndChild(entry5);
					}
                    entry3->InsertEndChild(entry4);

                    entry4 = doc.NewElement("triangles");
					for (uint32_t i = 0; i < ob->trianglecount; i++)
					{
                        auto entry5 = doc.NewElement("triangle");
                        entry5->SetAttribute("v1", ob->triangles[i].v1);
                        entry5->SetAttribute("v2", ob->triangles[i].v2);
                        entry5->SetAttribute("v3", ob->triangles[i].v3);
                        entry4->InsertEndChild(entry5);
					}
                    entry3->InsertEndChild(entry4);
				}
				if (ob->recordcount)
				{
					add(ob->buttonrecords);

                    auto entry4 = doc.NewElement("buttonrecords");

					for (uint32_t i = 0; i < ob->recordcount; i++)
					{
                        auto entry5 = doc.NewElement("buttonrecord");
                        entry5->SetAttribute("character", ob->buttonrecords[i].character);
                        entry5->SetAttribute("depth", ob->buttonrecords[i].depth);
                        entry5->SetAttribute("rotm00", ob->buttonrecords[i].rotateandscale.m00);
                        entry5->SetAttribute("rotm01", ob->buttonrecords[i].rotateandscale.m01);
                        entry5->SetAttribute("rotm10", ob->buttonrecords[i].rotateandscale.m10);
                        entry5->SetAttribute("rotm11", ob->buttonrecords[i].rotateandscale.m11);
                        entry5->SetAttribute("tx", ob->buttonrecords[i].translate.X);
                        entry5->SetAttribute("ty", ob->buttonrecords[i].translate.Y);
						
						std::string flagstr = Flags::GetButFlags_str(ob->buttonrecords[i].flags);
                        auto entry6 = doc.NewElement("buttonflags");
                        entry6->SetAttribute("value", flagstr.c_str());
                        entry5->InsertEndChild(entry6);

                        entry4->InsertEndChild(entry5);
					}
                    entry3->InsertEndChild(entry4);
				}
				if (ob->buttonactioncount)
				{
					add(ob->buttonactionrecords);
                    auto entry4 = doc.NewElement("buttonactions");

					for (uint32_t i = 0; i < ob->buttonactioncount; i++)
					{
						std::string flagstr = Flags::GetButActionFlags_str(ob->buttonactionrecords[i].flags);
                        auto entry5 = doc.NewElement("buttonaction");
                        entry5->SetAttribute("flags", flagstr.c_str());
						add(ob->buttonactionrecords[i].actiondata);
						ActionHelper::APT_ProcessActions(doc,entry5, ob->buttonactionrecords[i].actiondata, aptbuffer, data, aptbuffer);
                        entry4->InsertEndChild(entry5);
					}
                    entry3->InsertEndChild(entry4);
				}
                entry->InsertEndChild(entry3);
			}
				break;
			case SPRITE:
			{
				OutputSprite *sp = (OutputSprite *)m->characters[ch];
                auto entry3 = doc.NewElement("sprite");
                entry3->SetAttribute("id", ch);

				add(sp->frames);
				if (sp->framecount)
				{
                    auto entry4 = doc.NewElement("frames");
					for (uint32_t i = 0; i < sp->framecount; i++)
					{
						add(sp->frames[i].frameitems);
                        auto entry5 = doc.NewElement("frame");
                        entry5->SetAttribute("id", i);  

						for (uint32_t j = 0; j < sp->frames[i].frameitemcount; j++)
						{
							add(sp->frames[i].frameitems[j]);
							switch (sp->frames[i].frameitems[j]->type)
							{
							case ACTION:
							{
								OutputAction *oa = (OutputAction *)sp->frames[i].frameitems[j];
								add(oa->actionbytes);
                                auto entry6 = doc.NewElement("action");
								ActionHelper::APT_ProcessActions(doc,entry6, oa->actionbytes, aptbuffer, data, aptbuffer);
                                entry5->InsertEndChild(entry6);
							}
								break;
							case FRAMELABEL:
							{
								FrameLabel *fl = (FrameLabel *)sp->frames[i].frameitems[j];
								add(fl->label);
                                auto entry6 = doc.NewElement("framelabel");
                                entry6->SetAttribute("label", fl->label);
                                entry6->SetAttribute("frame", fl->frame);
                                entry5->InsertEndChild(entry6);
							}
								break;
							case PLACEOBJECT:
							{
                                OutputPlaceObject *po = (OutputPlaceObject *)sp->frames[i].frameitems[j];
                                int alpha = GetByte(po->colortransform, 3);
                                int blue = GetByte(po->colortransform, 2);
                                int green = GetByte(po->colortransform, 1);
                                int red = GetByte(po->colortransform, 0);
                                auto entry6 = doc.NewElement("placeobject");
                                //set all attributes of this node
                                entry6->SetAttribute("depth", po->depth);
                                entry6->SetAttribute("character", po->character);
                                entry6->SetAttribute("rotm00", po->rotateandscale.m00);
                                entry6->SetAttribute("rotm01", po->rotateandscale.m01);
                                entry6->SetAttribute("rotm10", po->rotateandscale.m10);
                                entry6->SetAttribute("rotm11", po->rotateandscale.m11);
                                entry6->SetAttribute("tx", po->translate.X);
                                entry6->SetAttribute("ty", po->translate.Y);
                                entry6->SetAttribute("red", red);
                                entry6->SetAttribute("green", green);
                                entry6->SetAttribute("blue", blue);
                                entry6->SetAttribute("alpha", alpha);
                                entry6->SetAttribute("ratio", po->ratio);
                                entry6->SetAttribute("clipdepth", po->clipdepth);
                                entry6->SetAttribute("unknown", po->unknown);

                                std::string flagstr = Flags::GetPOFlags_str(po->flags);
                                {
                                    auto entry7 = doc.NewElement("poflags");
                                    entry7->SetAttribute("value", flagstr.c_str());
                                    entry6->InsertEndChild(entry7);
                                }

                                if (po->name)
                                {
                                    add(po->name);
                                    auto entry7 = doc.NewElement("poname");
                                    entry7->SetAttribute("name", po->name);
                                    entry6->InsertEndChild(entry7);
                                }
                                if (po->poa)
                                {
                                    add(po->poa);
                                    add(po->poa->actions);
                                    auto entry7 = doc.NewElement("clipactions");

                                    for (uint32_t k = 0; k < po->poa->clipactioncount; k++)
                                    {
                                        auto entry8 = doc.NewElement("clipaction");
                                        entry8->SetAttribute("flags", po->poa->actions[k].flags);
                                        entry8->SetAttribute("flags2", po->poa->actions[k].flags2);
                                        add(po->poa->actions[k].actiondata);
                                        ActionHelper::APT_ProcessActions(doc, entry8, po->poa->actions[k].actiondata, aptbuffer, data, aptbuffer);
                                        entry7->InsertEndChild(entry8);
                                    }
                                    entry6->InsertEndChild(entry7);

                                }
                                entry5->InsertEndChild(entry6);
							}
								break;
							case REMOVEOBJECT:
							{
								RemoveObject *ro = (RemoveObject *)sp->frames[i].frameitems[j];
                                auto entry6 = doc.NewElement("removeobject");
                                entry6->SetAttribute("depth", ro->depth);
                                entry5->InsertEndChild(entry6);
							}
								break;
							case BACKGROUNDCOLOR:
							{
								BackgroundColor *bg = (BackgroundColor *)sp->frames[i].frameitems[j];
                                auto entry6 = doc.NewElement("background");
                                entry6->SetAttribute("color", bg->color);
                                entry5->InsertEndChild(entry6);
							}
								break;
							case INITACTION:
							{
								OutputInitAction *oa = (OutputInitAction *)sp->frames[i].frameitems[j];
								add(oa->actionbytes);
                                auto entry6 = doc.NewElement("initaction");
                                entry6->SetAttribute("sprite", oa->sprite);

								ActionHelper::APT_ProcessActions(doc,entry6, oa->actionbytes, aptbuffer,  data, aptbuffer);
                                entry5->InsertEndChild(entry6);
							}
								break;
							}
						}
                       
                        entry4->InsertEndChild(entry5);
					}
                    entry3->InsertEndChild(entry4);
				}
                entry->InsertEndChild(entry3);

			}
				break;
			case IMAGE:
			{
				Image *im = (Image *)m->characters[ch];
                auto entry3 = doc.NewElement("image");
                entry3->SetAttribute("id", ch);
                entry3->SetAttribute("image", im->texture);
                entry->InsertEndChild(entry3);
			}
				break;
			case MORPH:
			{
				Morph *mo = (Morph *)m->characters[ch];
                auto entry3 = doc.NewElement("morph");
                entry3->SetAttribute("id", ch);
                entry3->SetAttribute("start", mo->startshape);
                entry3->SetAttribute("end", mo->endshape);
                entry->InsertEndChild(entry3);
			}
				break;
			case TEXT:
			{
				OutputText *te = (OutputText *)m->characters[ch];
                auto entry3 = doc.NewElement("text");
                entry3->SetAttribute("id", ch);
                entry3->SetAttribute("top", te->bounds.top);
                entry3->SetAttribute("left", te->bounds.left);
                entry3->SetAttribute("bottom", te->bounds.bottom);
                entry3->SetAttribute("right", te->bounds.right);
                entry3->SetAttribute("rotm00", te->rotateandscale.m00);
                entry3->SetAttribute("rotm01", te->rotateandscale.m01);
                entry3->SetAttribute("rotm10", te->rotateandscale.m10);
                entry3->SetAttribute("rotm11", te->rotateandscale.m11);
                entry3->SetAttribute("tx", te->translate.X);
                entry3->SetAttribute("ty", te->translate.Y);

				
				if (te->recordcount)
				{
					add(te->records);
                    auto entry4 = doc.NewElement("records");
					for (uint32_t i = 0; i < te->recordcount; i++)
					{
                        auto entry5 = doc.NewElement("record");
                        entry5->SetAttribute("font", te->records[i].font);
                        entry5->SetAttribute("red", te->records[i].color.top);
                        entry5->SetAttribute("green", te->records[i].color.left);
                        entry5->SetAttribute("blue", te->records[i].color.bottom);
                        entry5->SetAttribute("alpha", te->records[i].color.right);
                        entry5->SetAttribute("u1", te->records[i].unknown.top);
                        entry5->SetAttribute("u2", te->records[i].unknown.left);
                        entry5->SetAttribute("u3", te->records[i].unknown.bottom);
                        entry5->SetAttribute("u4", te->records[i].unknown.right);
                        entry5->SetAttribute("tx", te->records[i].offset.X);
                        entry5->SetAttribute("ty", te->records[i].offset.Y);
                        entry5->SetAttribute("height", te->records[i].textheight);
					
						if (te->records[i].glyphcount)
						{
                            auto entry6 = doc.NewElement("glyphs");
							add(te->records[i].glyphs);
							for (uint32_t j = 0; j < te->records[i].glyphcount; j++)
							{
                                auto entry7 = doc.NewElement("glyph");
                                entry7->SetAttribute("index", te->records[i].glyphs[j].index);
                                entry7->SetAttribute("advance", te->records[i].glyphs[j].advance);
							}
                            entry5->InsertEndChild(entry6);
						}
                        entry4->InsertEndChild(entry5);
					}
                    entry3->InsertEndChild(entry4);
				}
                entry->InsertEndChild(entry3);
			}
				break;
			default:
				break;
			}
		}
		else
		{
            auto entry3 = doc.NewElement("empty");
            entry3->SetAttribute("id", ch);
            entry->InsertEndChild(entry3);
		}
	}
   
    entry->InsertFirstChild(entry2);

    doc.InsertEndChild(entry);
    doc.SaveFile(xmlfile.string().c_str());

	if (data)
		delete data;
	if (aptbuffer)
		delete[] aptbuffer;
	if (constbuffer)
		delete[] constbuffer;

	return true;
}

bool AptFile::XMLToApt(std::string filename)
{
	std::tr2::sys::path xmlfile(filename);
	std::tr2::sys::path constfile(xmlfile.basename() + ".const");
	std::tr2::sys::path aptfile(xmlfile.basename() + ".apt");

	if (xmlfile.extension() != ".xml")
	{
		std::cout << "Please specify an .xml file" << std::endl;
		return false;
	}

	if (!fileExists(xmlfile))
	{
		std::cout << "Sorry but " << xmlfile.filename() << " is missing" << std::endl;
		return false;
	}
	tinyxml2::XMLDocument doc;
	doc.LoadFile(xmlfile.string().c_str());
    if (doc.Error())
    {
        std::cout << "Failed to parse .xml file!" << std::endl;
        return false;
    }

	tinyxml2::XMLNode* node = doc.FirstChildElement("aptdata");

	tinyxml2::XMLElement *entry, *entry2, *entry3, *entry4, *entry5, *entry6;
	AptConstData* data = new AptConstData();
	data->itemcount = 0;

	Movie *m = new Movie;
	m->type = MOVIE;
	m->signature = CHAR_SIG;
	m->pointer = 0;
	m->screensizex = 1024;
	m->screensizey = 768;
	m->unknown = 33;
	m->exportcount = 0;
	m->importcount = 0;
	m->charactercount = 1;
	m->characters.push_back(m);
	m->count = 0;
	for (entry = node->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement())
	{
		if (std::string(entry->Value())=="movieclip")
		{
			entry2 = entry->FirstChildElement();
    		for (entry3 = entry2->FirstChildElement(); entry3 != 0; entry3 = entry3->NextSiblingElement())
			{
				m->importcount++;
				Import *im = new Import;
				im->character = entry3->IntAttribute("character");
				im->movie = entry3->Attribute("movie");
				im->name = entry3->Attribute("name");
				im->pointer = 0;
				m->imports.push_back(im);
			}
			entry2 = entry2->NextSiblingElement();
			for (entry3 = entry2->FirstChildElement(); entry3 != 0; entry3 = entry3->NextSiblingElement())
			{
				m->exportcount++;
				Export *im = new Export;
				im->character = entry3->IntAttribute("character");
				im->name = entry3->Attribute("name");
				m->exports.push_back(im);
			}
			entry2 = entry2->NextSiblingElement();
			m->framecount = 0;
			for (entry3 = entry2->FirstChildElement(); entry3 != 0; entry3 = entry3->NextSiblingElement())
			{
				m->framecount++;
				Frame *f = new Frame;
				f->frameitemcount = 0;
				m->frames.push_back(f);
				for (entry4 = entry3->FirstChildElement(); entry4 != 0; entry4 = entry4->NextSiblingElement())
				{
                    if (std::string(entry4->Value()) == "action")
					{
						f->frameitemcount++;
						Action *a = new Action;
						a->type = ACTION;
						a->ab.constantcount = 0;
						a->ab.stringcount = 0;
						a->ab.pushdatacount = 0;
						a->ab.actionbytecount = 0;
						a->ab.definefunction2count = 0;
						a->ab.definefunctioncount = 0;
						ActionHelper::XML_ProcessActions(entry4, &a->ab, data);
						f->frameitems.push_back(a);
					}
					if (std::string(entry4->Value())== "framelabel")
					{
						FrameLabel *l;
						l = new FrameLabel;
						l->type = FRAMELABEL;
						l->label = entry4->Attribute("label");
						l->frame = entry4->IntAttribute("frame");
						l->flags = 0x70000;
						f->frameitems.push_back(l);
						f->frameitemcount++;
					}
					if (std::string(entry4->Value())== "placeobject")
					{
						f->frameitemcount++;
						PlaceObject *po = new PlaceObject;
						po->type = PLACEOBJECT;
						f->frameitems.push_back(po);

						po->depth = entry4->IntAttribute("depth");
						po->character = entry4->IntAttribute("character");
						po->rotateandscale.m00 = entry4->FloatAttribute("rotm00");
						po->rotateandscale.m01 = entry4->FloatAttribute("rotm01");
						po->rotateandscale.m10 = entry4->FloatAttribute("rotm10");
						po->rotateandscale.m11 = entry4->FloatAttribute("rotm11");
						po->translate.X = entry4->FloatAttribute("tx");
						po->translate.Y = entry4->FloatAttribute("ty");
						int red, green, blue, alpha;
						red = entry4->IntAttribute("red");
						green = entry4->IntAttribute("green");
						blue = entry4->IntAttribute("blue");
						alpha = entry4->IntAttribute("alpha");
						po->colortransform = red | (green * 0x100) | (blue * 0x10000) | (alpha * 0x1000000);
						po->ratio = entry4->FloatAttribute("ratio");
						po->clipdepth = entry4->IntAttribute("clipdepth");
						po->unknown = entry4->IntAttribute("unknown");
						entry5 = entry4->FirstChildElement("poname");
						if (entry5)
						{
							po->name = entry5->Attribute("name");
						}
						else
						{
							po->name = 0;
						}
						entry5 = entry4->FirstChildElement("poflags");
						if (entry5)
						{
							po->flags = Flags::GetPOFlags_int(entry5->Attribute("value"));
						}
						else
						{
							po->flags = 0;
						}

						entry5 = entry4->FirstChildElement("clipactions");
						if (entry5)
						{
							po->poa = new PlaceObjectActions;
							po->poa->clipactioncount = 0;
							for (entry6 = entry5->FirstChildElement(); entry6 != 0; entry6 = entry6->NextSiblingElement())
							{
								po->poa->clipactioncount++;
								PlaceObjectAction *poa = new PlaceObjectAction;
								po->poa->actions.push_back(poa);
								poa->flags = entry6->IntAttribute("flags");
								poa->flags2 = entry6->IntAttribute("flags2");
								poa->actiondata = new Action;
								poa->actiondata->ab.constantcount = 0;
								poa->actiondata->ab.stringcount = 0;
								poa->actiondata->ab.pushdatacount = 0;
								poa->actiondata->ab.actionbytecount = 0;
								poa->actiondata->ab.definefunction2count = 0;
								poa->actiondata->ab.definefunctioncount = 0;
								ActionHelper::XML_ProcessActions(entry6, &poa->actiondata->ab, data);
							}
						}
						else
						{
							po->poa = 0;
						}
					}
					if (std::string(entry4->Value())== "removeobject")
					{
						RemoveObject *ro;
						ro = new RemoveObject;
						ro->type = REMOVEOBJECT;
						ro->depth = entry4->IntAttribute("depth");
						f->frameitems.push_back(ro);
						f->frameitemcount++;
					}
                    else if (std::string(entry4->Value())== "background")
					{
						BackgroundColor *b;
						b = new BackgroundColor;
						b->type = BACKGROUNDCOLOR;
						b->color = entry4->IntAttribute("color");
						f->frameitems.push_back(b);
						f->frameitemcount++;
					}
					else if (std::string(entry4->Value())== "initaction")
					{
						f->frameitemcount++;
						InitAction *a = new InitAction;
						a->sprite = entry4->IntAttribute("sprite");
						a->type = INITACTION;
						a->ab.constantcount = 0;
						a->ab.stringcount = 0;
						a->ab.pushdatacount = 0;
						a->ab.actionbytecount = 0;
						a->ab.definefunction2count = 0;
						a->ab.definefunctioncount = 0;
						ActionHelper::XML_ProcessActions(entry4, &a->ab, data);
						f->frameitems.push_back(a);
					}
				}
			}
		}
		else if (std::string(entry->Value())== "shape")
		{
			m->charactercount++;
			Shape *sh = new Shape;
			sh->type = SHAPE;
			sh->signature = CHAR_SIG;
			sh->bounds.top = entry->FloatAttribute("top");
			sh->bounds.left = entry->FloatAttribute("left");
			sh->bounds.bottom = entry->FloatAttribute("bottom");
			sh->bounds.right = entry->FloatAttribute("right");
			sh->geometry = entry->IntAttribute("geometry");
			m->characters.push_back(sh);
		}
		else if (std::string(entry->Value())== "edittext")
		{
			m->charactercount++;
			EditText *sh = new EditText;
			sh->type = EDITTEXT;
			sh->signature = CHAR_SIG;
			sh->bounds.top = entry->FloatAttribute("top");
			sh->bounds.left = entry->FloatAttribute("left");
			sh->bounds.bottom = entry->FloatAttribute("bottom");
			sh->bounds.right = entry->FloatAttribute("right");
			sh->font = entry->IntAttribute("font");
			sh->alignment = entry->IntAttribute("alignment");
			int red, green, blue, alpha;
			red = entry->IntAttribute("red");
			green = entry->IntAttribute("green");
			blue = entry->IntAttribute("blue");
			alpha = entry->IntAttribute("alpha");
			sh->color = alpha | (blue * 0x100) | (green * 0x10000) | (red * 0x1000000);
			sh->fontheight = entry->FloatAttribute("height");
			sh->readonly = entry->IntAttribute("readonly");
			sh->multiline = entry->IntAttribute("multiline");

			entry2 = entry->FirstChildElement("ettext");
			if (entry2)
			{
				sh->text = entry2->Attribute("text");
				for (unsigned int len = 0; len < strlen(sh->text); len++)
				{
					if (sh->text[len] == 0x0A)
					{
						((char *)sh->text)[len] = 0x0D;
					}
				}
			}
			else
			{
				sh->text = 0;
			}
			entry2 = entry->FirstChildElement("etvar");
			if (entry2)
			{
				sh->variable = entry2->Attribute("variable");
			}
			else
			{
				sh->variable = 0;
			}
			m->characters.push_back(sh);
		}
        else if (std::string(entry->Value())== "font")
		{
			m->charactercount++;
			Font *sh = new Font;
			sh->type = FONT;
			sh->signature = CHAR_SIG;
			sh->name = entry->Attribute("name");
			sh->glyphcount = 0;
			entry2 = entry->FirstChildElement("glyphs");
			if (entry2)
			{
				for (entry3 = entry2->FirstChildElement(); entry3 != 0; entry3 = entry3->NextSiblingElement())
				{
					unsigned int glyph;
					glyph = entry3->IntAttribute("glyph");
					sh->glyphs.push_back(glyph);
				}
			}
			m->characters.push_back(sh);
		}
		else if (std::string(entry->Value())== "button")
		{
			m->charactercount++;
			Button *b = new Button;
			b->unknown = 0;
			b->unknown2 = 0;
			m->characters.push_back(b);
			b->type = BUTTON;
			b->signature = CHAR_SIG;
			b->bounds.top = entry->FloatAttribute("top");
			b->bounds.left = entry->FloatAttribute("left");
			b->bounds.bottom = entry->FloatAttribute("bottom");
			b->bounds.right = entry->FloatAttribute("right");

			entry2 = entry->FirstChildElement("vertexes");
			b->vertexcount = 0;
			if (entry2)
			{
				for (entry3 = entry2->FirstChildElement(); entry3 != 0; entry3 = entry3->NextSiblingElement())
				{
					b->vertexcount++;
					Vector2 *v = new Vector2;
					v->X = entry3->FloatAttribute("x");
					v->Y = entry3->FloatAttribute("y");

					b->vertexes.push_back(v);
				}
			}
			entry2 = entry->FirstChildElement("triangles");
			b->trianglecount = 0;
			if (entry2)
			{
				for (entry3 = entry2->FirstChildElement(); entry3 != 0; entry3 = entry3->NextSiblingElement())
				{
					b->trianglecount++;
					Triangle *v = new Triangle;
					v->v1 = entry3->IntAttribute("v1");
					v->v2 = entry3->IntAttribute("v2");
					v->v3 = entry3->IntAttribute("v3");

					b->triangles.push_back(v);
				}
			}
			b->recordcount = 0;
			entry2 = entry->FirstChildElement("buttonrecords");
			if (entry2)
			{
				for (entry3 = entry2->FirstChildElement(); entry3 != 0; entry3 = entry3->NextSiblingElement())
				{
					b->recordcount++;
					ButtonRecord *br = new ButtonRecord;
					b->buttonrecords.push_back(br);
					br->character = entry3->IntAttribute("character");
					br->depth = entry3->IntAttribute("depth");
					br->rotateandscale.m00 = entry3->FloatAttribute("rotm00");
					br->rotateandscale.m01 = entry3->FloatAttribute("rotm01");
					br->rotateandscale.m10 = entry3->FloatAttribute("rotm10");
					br->rotateandscale.m11 = entry3->FloatAttribute("rotm11");
					br->translate.X = entry3->FloatAttribute("tx");
					br->translate.Y = entry3->FloatAttribute("ty");
					br->color.bottom = 1;
					br->color.top = 1;
					br->color.left = 1;
					br->color.right = 1;
					br->unknown.bottom = 0;
					br->unknown.top = 0;
					br->unknown.left = 0;
					br->unknown.right = 0;
					entry4 = entry3->FirstChildElement("buttonflags");
					if (entry4)
					{
						br->flags = Flags::GetButFlags_int(entry4->Attribute("value"));
					}
					else
					{
						br->flags = 0;
					}
				}
			}
			b->buttonactioncount = 0;
			entry2 = entry->FirstChildElement("buttonactions");
			if (entry2)
			{
				for (entry3 = entry2->FirstChildElement(); entry3 != 0; entry3 = entry3->NextSiblingElement())
				{
					b->buttonactioncount++;
					ButtonAction *br = new ButtonAction;
					br->actiondata = new Action;
					br->flags = Flags::GetButActionFlags_int(entry3->Attribute("flags"));
					br->actiondata->ab.constantcount = 0;
					br->actiondata->ab.stringcount = 0;
					br->actiondata->ab.pushdatacount = 0;
					br->actiondata->ab.actionbytecount = 0;
					br->actiondata->ab.definefunction2count = 0;
					br->actiondata->ab.definefunctioncount = 0;
					ActionHelper::XML_ProcessActions(entry3, &br->actiondata->ab, data);
					b->buttonactionrecords.push_back(br);
				}
			}
		}
        else if (std::string(entry->Value())== "sprite")
		{
			m->charactercount++;
			Sprite *sp = new Sprite;
			m->characters.push_back(sp);
			sp->type = SPRITE;
			sp->signature = CHAR_SIG;
			sp->framecount = 0;
			sp->pointer = 0;
			entry2 = entry->FirstChildElement();
			if (entry2)
			{
				for (entry3 = entry2->FirstChildElement(); entry3 != 0; entry3 = entry3->NextSiblingElement())
				{
					sp->framecount++;
					Frame *f = new Frame;
					f->frameitemcount = 0;
					sp->frames.push_back(f);
					for (entry4 = entry3->FirstChildElement(); entry4 != 0; entry4 = entry4->NextSiblingElement())
					{
						if (!_stricmp(entry4->Value(), "action"))
						{
							f->frameitemcount++;
							Action *a = new Action;
							a->type = ACTION;
							a->ab.constantcount = 0;
							a->ab.stringcount = 0;
							a->ab.pushdatacount = 0;
							a->ab.actionbytecount = 0;
							a->ab.definefunction2count = 0;
							a->ab.definefunctioncount = 0;
							ActionHelper::XML_ProcessActions(entry4, &a->ab, data);
							f->frameitems.push_back(a);
						}
						if (!_stricmp(entry4->Value(), "framelabel"))
						{
							FrameLabel *l;
							l = new FrameLabel;
							l->type = FRAMELABEL;
							l->label = entry4->Attribute("label");
							l->frame = entry4->IntAttribute("frame");
							l->flags = 0x70000;
							f->frameitems.push_back(l);
							f->frameitemcount++;
						}
						if (!_stricmp(entry4->Value(), "placeobject"))
						{
							f->frameitemcount++;
							PlaceObject *po = new PlaceObject;
							po->type = PLACEOBJECT;
							f->frameitems.push_back(po);
							po->character = entry4->IntAttribute("character");
							po->depth = entry4->IntAttribute("depth");
							po->rotateandscale.m00 = entry4->FloatAttribute("rotm00");
							po->rotateandscale.m01 = entry4->FloatAttribute("rotm01");
							po->rotateandscale.m10 = entry4->FloatAttribute("rotm10");
							po->rotateandscale.m11 = entry4->FloatAttribute("rotm11");
							po->translate.X = entry4->FloatAttribute("tx");
							po->translate.Y = entry4->FloatAttribute("ty");
							int red, green, blue, alpha;
							red = entry4->IntAttribute("red");
							green = entry4->IntAttribute("green");
							blue = entry4->IntAttribute("blue");
							alpha = entry4->IntAttribute("alpha");
							po->colortransform = red | (green * 0x100) | (blue * 0x10000) | (alpha * 0x1000000);
							po->ratio = entry4->FloatAttribute("ratio");
							po->clipdepth = entry4->IntAttribute("clipdepth");
							po->unknown = entry4->IntAttribute("unknown");
							entry5 = entry4->FirstChildElement("poname");
							if (entry5)
							{
								po->name = entry5->Attribute("name");
							}
							else
							{
								po->name = 0;
							}
							entry5 = entry4->FirstChildElement("poflags");
							if (entry5)
							{
								po->flags = Flags::GetPOFlags_int(entry5->Attribute("value"));
							}
							else
							{
								po->flags = 0;
							}

							entry5 = entry4->FirstChildElement("clipactions");
							if (entry5)
							{
								po->poa = new PlaceObjectActions;
								po->poa->clipactioncount = 0;
								for (entry6 = entry5->FirstChildElement(); entry6 != 0; entry6 = entry6->NextSiblingElement())
								{
									po->poa->clipactioncount++;
									PlaceObjectAction *poa = new PlaceObjectAction;
									po->poa->actions.push_back(poa);
									poa->flags = entry6->IntAttribute("flags");
									poa->flags2 = entry6->IntAttribute("flags2");
									poa->actiondata = new Action;
									poa->actiondata->ab.constantcount = 0;
									poa->actiondata->ab.stringcount = 0;
									poa->actiondata->ab.pushdatacount = 0;
									poa->actiondata->ab.actionbytecount = 0;
									poa->actiondata->ab.definefunction2count = 0;
									poa->actiondata->ab.definefunctioncount = 0;
									ActionHelper::XML_ProcessActions(entry6, &poa->actiondata->ab, data);
								}
							}
							else
							{
								po->poa = 0;
							}
						}
						if (!_stricmp(entry4->Value(), "removeobject"))
						{
							RemoveObject *ro;
							ro = new RemoveObject;
							ro->type = REMOVEOBJECT;
							ro->depth = entry4->IntAttribute("depth");
							f->frameitems.push_back(ro);
							f->frameitemcount++;
						}
						if (!_stricmp(entry4->Value(), "background"))
						{
							BackgroundColor *b;
							b = new BackgroundColor;
							b->type = BACKGROUNDCOLOR;
							b->color = entry4->IntAttribute("color");
							f->frameitems.push_back(b);
							f->frameitemcount++;
						}
						if (!_stricmp(entry4->Value(), "initaction"))
						{
							f->frameitemcount++;
							InitAction *a = new InitAction;
							a->sprite = entry4->IntAttribute("sprite");
							a->type = INITACTION;
							a->ab.constantcount = 0;
							a->ab.stringcount = 0;
							a->ab.pushdatacount = 0;
							a->ab.actionbytecount = 0;
							a->ab.definefunction2count = 0;
							a->ab.definefunctioncount = 0;
							ActionHelper::XML_ProcessActions(entry4, &a->ab, data);
							f->frameitems.push_back(a);
						}
					}
				}
			}
		}
        else if (std::string(entry->Value())== "image")
		{
			m->charactercount++;
			Image *sh = new Image;
			sh->type = IMAGE;
			sh->signature = CHAR_SIG;
			sh->texture = entry->IntAttribute("image");
			m->characters.push_back(sh);
		}
		else if (std::string(entry->Value())== "morph")
		{
			m->charactercount++;
			Morph *sh = new Morph;
			sh->type = MORPH;
			sh->signature = CHAR_SIG;
			sh->startshape = entry->IntAttribute("start");
			sh->endshape = entry->IntAttribute("end");;
			m->characters.push_back(sh);
		}
        else if (std::string(entry->Value())== "text")
		{
			m->charactercount++;
			Text *sh = new Text;
			sh->type = TEXT;
			sh->signature = CHAR_SIG;
			sh->bounds.top = entry->FloatAttribute("top");
			sh->bounds.left = entry->FloatAttribute("left");
			sh->bounds.bottom = entry->FloatAttribute("bottom");
			sh->bounds.right = entry->FloatAttribute("right");
			sh->rotateandscale.m00 = entry->FloatAttribute("m00");
			sh->rotateandscale.m01 = entry->FloatAttribute("m01");
			sh->rotateandscale.m10 = entry->FloatAttribute("m10");
			sh->rotateandscale.m11 = entry->FloatAttribute("m11");
			sh->translate.X = entry->FloatAttribute("tx");
			sh->translate.X = entry->FloatAttribute("ty");
			entry2 = entry->FirstChildElement("records");
			sh->recordcount = 0;
			if (entry2)
			{
				for (entry3 = entry2->FirstChildElement(); entry3 != 0; entry3 = entry3->NextSiblingElement())
				{
					TextRecord *tr = new TextRecord;
					sh->recordcount++;
					sh->records.push_back(tr);

					tr->font = entry3->IntAttribute("font");
					tr->color.top = entry3->FloatAttribute("red");
					tr->color.left = entry3->FloatAttribute("green");
					tr->color.bottom = entry3->FloatAttribute("blue");
					tr->color.right = entry3->FloatAttribute("alpha");
					tr->unknown.top = entry3->FloatAttribute("u1");
					tr->unknown.left = entry3->FloatAttribute("u2");
					tr->unknown.bottom = entry3->FloatAttribute("u3");
					tr->unknown.right = entry3->FloatAttribute("u4");
					tr->offset.X = entry3->FloatAttribute("tx");
					tr->offset.Y = entry3->FloatAttribute("ty");
					tr->textheight = entry3->FloatAttribute("height");
					entry4 = entry3->FirstChildElement("glyphs");
					tr->glyphcount = 0;
					if (entry4)
					{
						for (entry5 = entry4->FirstChildElement(); entry5 != 0; entry5 = entry5->NextSiblingElement())
						{
							Glyph *gl = new Glyph;
							gl->index = entry5->IntAttribute("index");
							gl->advance = entry5->IntAttribute("advance");

						}
					}
				}
			}
			m->characters.push_back(sh);
		}
        else if (std::string(entry->Value())== "empty")
		{
			m->charactercount++;
			m->characters.push_back(0);
		}
	}

	data->aptdataoffset = GenerateAptAptFile(m, aptfile.string().c_str());
	GenerateAptConstFile(data, constfile.string().c_str());
	return true;
}


#pragma region Aptutils
unsigned int AptFile::GetFrameItemSize(Frame *fr)
{
	unsigned int framesize = 0;
	for (unsigned int i = 0; i < fr->frameitemcount; i++)
	{
		framesize += 4;
		switch (fr->frameitems[i]->type)
		{
		case ACTION:
			framesize += sizeof(OutputAction);
			break;
		case FRAMELABEL:
			framesize += sizeof(FrameLabel);
			break;
		case PLACEOBJECT:
			framesize += sizeof(PlaceObject);
			break;
		case REMOVEOBJECT:
			framesize += sizeof(RemoveObject);
			break;
		case BACKGROUNDCOLOR:
			framesize += sizeof(BackgroundColor);
			break;
		case INITACTION:
			framesize += sizeof(OutputInitAction);
			break;
		}
	}
	return framesize;
}

unsigned int AptFile::GetFrameItemPointerSize(Frame *fr)
{
	unsigned int framesize = 0;
	for (unsigned int i = 0; i < fr->frameitemcount; i++)
	{
		switch (fr->frameitems[i]->type)
		{
		case ACTION:
		{
			Action *a = ((Action *)fr->frameitems[i]);
			framesize += a->ab.actionbytecount;
			framesize += GETALIGN(a->ab.actionbytecount);
		}
			break;
		case FRAMELABEL:
		{
			FrameLabel *f = ((FrameLabel *)fr->frameitems[i]);
			framesize += STRLENGTH(f->label);
		}
			break;
		case PLACEOBJECT:
		{
			PlaceObject *pl = ((PlaceObject *)fr->frameitems[i]);
			if (pl->name)
			{
				framesize += STRLENGTH(pl->name);
			}
			if (pl->poa)
			{
				framesize += sizeof(OutputPlaceObjectActions);
			}
		}
			break;
		case INITACTION:
		{
			InitAction *a = ((InitAction *)fr->frameitems[i]);
			framesize += a->ab.actionbytecount;
			framesize += GETALIGN(a->ab.actionbytecount);
		}
			break;
		}
	}
	return framesize;
}

unsigned int AptFile::GetFrameItemPointerPointerSize(Frame *fr)
{
	unsigned int framesize = 0;
	for (unsigned int i = 0; i < fr->frameitemcount; i++)
	{
		switch (fr->frameitems[i]->type)
		{
		case ACTION:
		{
			Action *a = ((Action *)fr->frameitems[i]);
			framesize += a->ab.constantcount * 4;
			for (unsigned int j = 0; j < a->ab.stringcount; j++)
			{
				framesize += STRLENGTH(a->ab.actionstrings[j]->string);
			}
			for (unsigned int j = 0; j < a->ab.pushdatacount; j++)
			{
				framesize += a->ab.actionpushdatas[j]->pushdatacount * 4;
			}
			for (unsigned int j = 0; j < a->ab.definefunction2count; j++)
			{
				for (unsigned int arg = 0; arg < a->ab.actiondefinefunction2s[j]->argumentcount; arg++)
				{
					framesize += 8;
					framesize += STRLENGTH(a->ab.actiondefinefunction2s[j]->arguments[arg]->name);
				}
			}
			for (unsigned int j = 0; j < a->ab.definefunctioncount; j++)
			{
				for (unsigned int arg = 0; arg < a->ab.actiondefinefunctions[j]->argumentcount; arg++)
				{
					framesize += 4;
					framesize += STRLENGTH(a->ab.actiondefinefunctions[j]->arguments[arg]);
				}
			}
		}
			break;
		case PLACEOBJECT:
		{
			PlaceObject *pl = ((PlaceObject *)fr->frameitems[i]);
			if (pl->poa)
			{
				framesize += pl->poa->clipactioncount * sizeof(PlaceObjectAction);
			}
		}
			break;
		case INITACTION:
		{
			InitAction *a = ((InitAction *)fr->frameitems[i]);
			framesize += a->ab.constantcount * 4;
			for (unsigned int j = 0; j < a->ab.stringcount; j++)
			{
				framesize += STRLENGTH(a->ab.actionstrings[j]->string);
			}
			for (unsigned int j = 0; j < a->ab.pushdatacount; j++)
			{
				framesize += a->ab.actionpushdatas[j]->pushdatacount * 4;
			}
			for (unsigned int j = 0; j < a->ab.definefunction2count; j++)
			{
				for (unsigned int arg = 0; arg < a->ab.actiondefinefunction2s[j]->argumentcount; arg++)
				{
					framesize += 8;
					framesize += STRLENGTH(a->ab.actiondefinefunction2s[j]->arguments[arg]->name);
				}
			}
			for (unsigned int j = 0; j < a->ab.definefunctioncount; j++)
			{
				for (unsigned int arg = 0; arg < a->ab.actiondefinefunctions[j]->argumentcount; arg++)
				{
					framesize += 4;
					framesize += STRLENGTH(a->ab.actiondefinefunctions[j]->arguments[arg]);
				}
			}
		}
			break;
		}
	}
	return framesize;
}

unsigned int AptFile::GetFrameItemPointerPointerPointerSize(Frame *fr)
{
	unsigned int framesize = 0;
	for (unsigned int i = 0; i < fr->frameitemcount; i++)
	{
		switch (fr->frameitems[i]->type)
		{
		case PLACEOBJECT:
		{
			PlaceObject *pl = ((PlaceObject *)fr->frameitems[i]);
			if (pl->poa)
			{
				for (unsigned int j = 0; j < pl->poa->clipactioncount; j++)
				{
					framesize += pl->poa->actions[j]->actiondata->ab.actionbytecount;
					framesize += GETALIGN(pl->poa->actions[j]->actiondata->ab.actionbytecount);
				}
			}
		}
			break;
		}
	}
	return framesize;
}

unsigned int AptFile::GetFrameItemPointerPointerPointerPointerSize(Frame *fr)
{
	unsigned int framesize = 0;
	for (unsigned int i = 0; i < fr->frameitemcount; i++)
	{
		switch (fr->frameitems[i]->type)
		{
		case PLACEOBJECT:
		{
			PlaceObject *pl = ((PlaceObject *)fr->frameitems[i]);
			if (pl->poa)
			{
				for (unsigned int j = 0; j < pl->poa->clipactioncount; j++)
				{
					framesize += pl->poa->actions[j]->actiondata->ab.constantcount * 4;
					for (unsigned int k = 0; k < pl->poa->actions[j]->actiondata->ab.stringcount; k++)
					{
						framesize += STRLENGTH(pl->poa->actions[j]->actiondata->ab.actionstrings[k]->string);
					}
					for (unsigned int k = 0; k < pl->poa->actions[j]->actiondata->ab.pushdatacount; k++)
					{
						framesize += pl->poa->actions[j]->actiondata->ab.actionpushdatas[k]->pushdatacount * 4;
					}
					for (unsigned int k = 0; k < pl->poa->actions[j]->actiondata->ab.definefunction2count; k++)
					{
						for (unsigned int arg = 0; arg < pl->poa->actions[j]->actiondata->ab.actiondefinefunction2s[k]->argumentcount; arg++)
						{
							framesize += 8;
							framesize += STRLENGTH(pl->poa->actions[j]->actiondata->ab.actiondefinefunction2s[k]->arguments[arg]->name);
						}
					}
					for (unsigned int k = 0; k < pl->poa->actions[j]->actiondata->ab.definefunctioncount; k++)
					{
						for (unsigned int arg = 0; arg < pl->poa->actions[j]->actiondata->ab.actiondefinefunctions[k]->argumentcount; arg++)
						{
							framesize += 4;
							framesize += STRLENGTH(pl->poa->actions[j]->actiondata->ab.actiondefinefunctions[k]->arguments[arg]);
						}
					}
				}
			}
		}
			break;
		}
	}
	return framesize;
}

uint32_t AptFile::GenerateAptAptFile(Movie *m, const char *filename)
{
	uint32_t result = 0;
	unsigned int aptdatasize = 16;
	unsigned int characterdatasize = 0;
	unsigned int framesize = 0;
	unsigned int frameitemlistsize = 0;
	unsigned int frameitemsize = 0;
	unsigned int frameitempointersize = 0;
	unsigned int frameitempointerpointersize = 0;
	unsigned int frameitempointerpointerpointersize = 0;
	for (unsigned int i = 0; i < m->importcount; i++)
	{
		aptdatasize += sizeof(Import);
		aptdatasize += STRLENGTH(m->imports[i]->movie);
		aptdatasize += STRLENGTH(m->imports[i]->name);
	}
	for (unsigned int i = 0; i < m->exportcount; i++)
	{
		aptdatasize += sizeof(Export);
		aptdatasize += STRLENGTH(m->exports[i]->name);
	}
	aptdatasize += m->charactercount * 4;
	for (unsigned int i = 0; i < m->charactercount; i++)
	{
		if (m->characters[i])
		{
			switch (m->characters[i]->type)
			{
			case BUTTON:
			{
				aptdatasize += sizeof(OutputButton);
				characterdatasize += sizeof(OutputButton);
				Button *b = (Button *)m->characters[i];
				frameitemsize += b->vertexcount * sizeof(Vector2);
				aptdatasize += b->vertexcount * sizeof(Vector2);
				frameitemsize += b->trianglecount * sizeof(Triangle);
				aptdatasize += b->trianglecount * sizeof(Triangle);
				frameitemsize += GETALIGN(b->trianglecount * sizeof(Triangle));
				aptdatasize += GETALIGN(b->trianglecount * sizeof(Triangle));
				frameitemsize += b->recordcount * sizeof(ButtonRecord);
				aptdatasize += b->recordcount * sizeof(ButtonRecord);
				frameitemsize += b->buttonactioncount * sizeof(OutputButtonAction);
				aptdatasize += b->buttonactioncount * sizeof(OutputButtonAction);
				unsigned int fipsize = 0;
				unsigned int fippsize = 0;
				for (unsigned int bu = 0; bu < b->buttonactioncount; bu++)
				{
					fipsize += b->buttonactionrecords[bu]->actiondata->ab.actionbytecount;
					fipsize += GETALIGN(b->buttonactionrecords[bu]->actiondata->ab.actionbytecount);
					fippsize += b->buttonactionrecords[bu]->actiondata->ab.constantcount * 4;
					for (unsigned int j = 0; j < b->buttonactionrecords[bu]->actiondata->ab.stringcount; j++)
					{
						fippsize += STRLENGTH(b->buttonactionrecords[bu]->actiondata->ab.actionstrings[j]->string);
					}
					for (unsigned int j = 0; j < b->buttonactionrecords[bu]->actiondata->ab.pushdatacount; j++)
					{
						fippsize += b->buttonactionrecords[bu]->actiondata->ab.actionpushdatas[j]->pushdatacount * 4;
					}
					for (unsigned int j = 0; j < b->buttonactionrecords[bu]->actiondata->ab.definefunction2count; j++)
					{
						for (unsigned int arg = 0; arg < b->buttonactionrecords[bu]->actiondata->ab.actiondefinefunction2s[j]->argumentcount; arg++)
						{
							fippsize += 8;
							fippsize += STRLENGTH(b->buttonactionrecords[bu]->actiondata->ab.actiondefinefunction2s[j]->arguments[arg]->name);
						}
					}
					for (unsigned int j = 0; j < b->buttonactionrecords[bu]->actiondata->ab.definefunctioncount; j++)
					{
						for (unsigned int arg = 0; arg < b->buttonactionrecords[bu]->actiondata->ab.actiondefinefunctions[j]->argumentcount; arg++)
						{
							fippsize += 4;
							fippsize += STRLENGTH(b->buttonactionrecords[bu]->actiondata->ab.actiondefinefunctions[j]->arguments[arg]);
						}
					}
				}
				frameitempointersize += fipsize;
				aptdatasize += fipsize;
				frameitempointerpointersize += fippsize;
				aptdatasize += fippsize;
			}
				break;
			case SHAPE:
				aptdatasize += sizeof(Shape);
				characterdatasize += sizeof(Shape);
				break;
			case EDITTEXT:
				aptdatasize += sizeof(EditText);
				characterdatasize += sizeof(EditText);
				if (((EditText *)m->characters[i])->text)
				{
					aptdatasize += STRLENGTH(((EditText *)m->characters[i])->text);
					framesize += STRLENGTH(((EditText *)m->characters[i])->text);
				}
				if (((EditText *)m->characters[i])->variable)
				{
					aptdatasize += STRLENGTH(((EditText *)m->characters[i])->variable);
					framesize += STRLENGTH(((EditText *)m->characters[i])->variable);
				}
				break;
			case FONT:
				aptdatasize += sizeof(OutputFont);
				characterdatasize += sizeof(OutputFont);
				aptdatasize += STRLENGTH(((Font *)m->characters[i])->name);
				framesize += STRLENGTH(((Font *)m->characters[i])->name);
				aptdatasize += ((Font *)m->characters[i])->glyphcount * 4;
				framesize += ((Font *)m->characters[i])->glyphcount * 4;
				break;
			case SPRITE:
				aptdatasize += sizeof(OutputSprite);
				characterdatasize += sizeof(OutputSprite);
				for (unsigned int j = 0; j < ((Sprite *)m->characters[i])->framecount; j++)
				{
					aptdatasize += 8;
					framesize += 8;
					if (((Sprite *)m->characters[i])->frames[j])
					{
						frameitemlistsize += ((Sprite *)m->characters[i])->frames[j]->frameitemcount * 4;
						aptdatasize += ((Sprite *)m->characters[i])->frames[j]->frameitemcount * 4;
						int frsize = GetFrameItemSize(((Sprite *)m->characters[i])->frames[j]);
						aptdatasize += frsize;
						frameitemsize += frsize;
						int frpsize = GetFrameItemPointerSize(((Sprite *)m->characters[i])->frames[j]);
						aptdatasize += frpsize;
						frameitempointersize += frpsize;
						int frppsize = GetFrameItemPointerPointerSize(((Sprite *)m->characters[i])->frames[j]);
						aptdatasize += frppsize;
						frameitempointerpointersize += frppsize;
						int frpppsize = GetFrameItemPointerPointerPointerSize(((Sprite *)m->characters[i])->frames[j]);
						aptdatasize += frpppsize;
						frameitempointerpointerpointersize += frpppsize;
						aptdatasize += GetFrameItemPointerPointerPointerPointerSize(((Sprite *)m->characters[i])->frames[j]);
					}
				}
				break;
			case IMAGE:
				aptdatasize += sizeof(Image);
				characterdatasize += sizeof(Image);
				break;
			case MORPH:
				aptdatasize += sizeof(Morph);
				characterdatasize += sizeof(Morph);
				break;
			case MOVIE:
				aptdatasize += sizeof(OutputMovie);
				characterdatasize += sizeof(OutputMovie);
				for (unsigned int j = 0; j < ((Movie *)m->characters[i])->framecount; j++)
				{
					aptdatasize += 8;
					framesize += 8;
					if (((Movie *)m->characters[i])->frames[j])
					{
						frameitemlistsize += ((Movie *)m->characters[i])->frames[j]->frameitemcount * 4;
						aptdatasize += ((Movie *)m->characters[i])->frames[j]->frameitemcount * 4;
						int frsize = GetFrameItemSize(((Movie *)m->characters[i])->frames[j]);
						aptdatasize += frsize;
						frameitemsize += frsize;
						int frpsize = GetFrameItemPointerSize(((Movie *)m->characters[i])->frames[j]);
						aptdatasize += frpsize;
						frameitempointersize += frpsize;
						int frppsize = GetFrameItemPointerPointerSize(((Movie *)m->characters[i])->frames[j]);
						aptdatasize += frppsize;
						frameitempointerpointersize += frppsize;
						int frpppsize = GetFrameItemPointerPointerPointerSize(((Movie *)m->characters[i])->frames[j]);
						aptdatasize += frpppsize;
						frameitempointerpointerpointersize += frpppsize;
						aptdatasize += GetFrameItemPointerPointerPointerPointerSize(((Movie *)m->characters[i])->frames[j]);
					}
				}
				break;
			case TEXT:
			{
				aptdatasize += sizeof(OutputText);
				characterdatasize += sizeof(OutputText);
				Text *t = (Text *)m->characters[i];
				frameitemsize += t->recordcount * sizeof(OutputTextRecord);
				aptdatasize += t->recordcount * sizeof(OutputTextRecord);
				unsigned int fipsize = 0;
				for (unsigned int te = 0; te < t->recordcount; te++)
				{
					fipsize += t->records[te]->glyphcount * sizeof(Glyph);
				}
				frameitempointersize += fipsize;
				aptdatasize += fipsize;
			}
				break;
			}
		}
	}
	unsigned char *aptdata = new unsigned char[aptdatasize];
	memset(aptdata, 0, aptdatasize);
	unsigned char *aptpos = aptdata;
	std::vector <unsigned char **> relocations;
	memcpy(aptpos, "Apt Data:7\x1A\0", 12);
	aptpos += 12;
	Import *imp = (Import *)aptpos;
	aptpos += m->importcount * sizeof(Import);
	for (unsigned int i = 0; i < m->importcount; i++)
	{
		imp[i].character = m->imports[i]->character;
		imp[i].pointer = m->imports[i]->pointer;
		strcpy((char *)aptpos, m->imports[i]->movie);
		imp[i].movie = (char *)aptpos;
		relocations.push_back((unsigned char **)(&imp[i].movie));
		aptpos += STRLENGTH(m->imports[i]->movie);
		strcpy((char *)aptpos, m->imports[i]->name);
		imp[i].name = (char *)aptpos;
		relocations.push_back((unsigned char **)(&imp[i].name));
		aptpos += STRLENGTH(m->imports[i]->name);
	}
	Export *ex = (Export *)aptpos;
	aptpos += m->exportcount * sizeof(Export);
	for (unsigned int i = 0; i < m->exportcount; i++)
	{
		ex[i].character = m->exports[i]->character;
		strcpy((char *)aptpos, m->exports[i]->name);
		ex[i].name = (char *)aptpos;
		relocations.push_back((unsigned char **)(&ex[i].name));
		aptpos += STRLENGTH(m->exports[i]->name);
	}
	unsigned char **ch = (unsigned char **)aptpos;
	aptpos += m->charactercount * 4;
	unsigned char *chd = aptpos;
	aptpos += characterdatasize;
	OutputFrame *fr = (OutputFrame *)aptpos;
	aptpos += framesize;
	unsigned char **fil = (unsigned char **)aptpos;
	aptpos += frameitemlistsize;
	unsigned char *fi = aptpos;
	aptpos += frameitemsize;
	unsigned char *fip = aptpos;
	aptpos += frameitempointersize;
	unsigned char *ads = aptpos;
	aptpos += frameitempointerpointersize;
	unsigned char *ads2 = aptpos;
	aptpos += frameitempointerpointerpointersize;
	unsigned char *ads3 = aptpos;
	for (unsigned int i = 0; i < m->charactercount; i++)
	{
		if (m->characters[i])
		{
			*ch = chd;
			relocations.push_back((unsigned char **)(ch));
			switch (m->characters[i]->type)
			{
			case BUTTON:
			{
				OutputButton *ob = (OutputButton *)chd;
				Button *b = (Button *)m->characters[i];
				chd += sizeof(OutputButton);
				ob->type = b->type;
				ob->signature = b->signature;
				ob->unknown = b->unknown;
				ob->unknown2 = b->unknown2;
				ob->bounds = b->bounds;
				ob->trianglecount = b->trianglecount;
				ob->vertexcount = b->vertexcount;
				if (ob->vertexcount)
				{
					Vector2 *v = (Vector2 *)fi;
					ob->vertexes = v;
					relocations.push_back((unsigned char **)(&ob->vertexes));
					for (unsigned int vc = 0; vc < ob->vertexcount; vc++)
					{
						*v = *(b->vertexes[vc]);
						v++;
					}
					fi = (unsigned char *)v;
				}
				if (ob->trianglecount)
				{
					Triangle *t = (Triangle *)fi;
					ob->triangles = t;
					relocations.push_back((unsigned char **)(&ob->triangles));
					for (unsigned int tc = 0; tc < ob->trianglecount; tc++)
					{
						*t = *(b->triangles[tc]);
						t++;
					}
					fi = (unsigned char *)t;
					fi += GETALIGN(ob->trianglecount * sizeof(Triangle));
				}
				ob->recordcount = b->recordcount;
				if (ob->recordcount)
				{
					ButtonRecord *r = (ButtonRecord *)fi;
					ob->buttonrecords = r;
					relocations.push_back((unsigned char **)(&ob->buttonrecords));
					for (unsigned int rc = 0; rc < ob->recordcount; rc++)
					{
						*r = *(b->buttonrecords[rc]);
						r++;
					}
					fi = (unsigned char *)r;
				}
				ob->buttonactioncount = b->buttonactioncount;
				if (ob->buttonactioncount)
				{
					OutputButtonAction *a = (OutputButtonAction *)fi;
					ob->buttonactionrecords = a;
					relocations.push_back((unsigned char **)(&ob->buttonactionrecords));
					for (unsigned int ac = 0; ac < ob->buttonactioncount; ac++)
					{
						ButtonAction *ba = (ButtonAction *)b->buttonactionrecords[ac];
						a->flags = ba->flags;
						Action *ai = ba->actiondata;
						relocations.push_back((unsigned char **)(&a->actiondata));
						std::string s = ai->ab.actionbytes.str();
						const char *sc = s.c_str();
						a->actiondata = fip;
						unsigned char *abcs = fip;
						memcpy(fip, sc, ai->ab.actionbytecount);
						fip += ai->ab.actionbytecount;
						fip += GETALIGN(ai->ab.actionbytecount);
						if (ai->ab.constantcount)
						{
							*(unsigned char **)(abcs + 8) = ads;
							relocations.push_back((unsigned char **)(abcs + 8));
							for (unsigned int l = 0; l < ai->ab.constantcount; l++)
							{
								*(unsigned int *)ads = ai->ab.constants[l];
								ads += 4;
							}
						}
						for (unsigned int l = 0; l < ai->ab.stringcount; l++)
						{
							*(unsigned char **)(abcs + ai->ab.actionstrings[l]->stringoffset) = ads;
							relocations.push_back((unsigned char **)(abcs + ai->ab.actionstrings[l]->stringoffset));
							strcpy((char *)ads, ai->ab.actionstrings[l]->string);
							ads += STRLENGTH(ai->ab.actionstrings[l]->string);
						}
						for (unsigned int l = 0; l < ai->ab.pushdatacount; l++)
						{
							*(unsigned char **)(abcs + ai->ab.actionpushdatas[l]->pushdataoffset) = ads;
							relocations.push_back((unsigned char **)(abcs + ai->ab.actionpushdatas[l]->pushdataoffset));
							for (unsigned int pd = 0; pd < ai->ab.actionpushdatas[l]->pushdatacount; pd++)
							{
								*(unsigned int *)ads = ai->ab.actionpushdatas[l]->pushdata[pd];
								ads += 4;
							}
						}
						for (unsigned int l = 0; l < ai->ab.definefunction2count; l++)
						{
							*(unsigned char **)(abcs + ai->ab.actiondefinefunction2s[l]->definefunction2offset) = ads;
							relocations.push_back((unsigned char **)(abcs + ai->ab.actiondefinefunction2s[l]->definefunction2offset));
							unsigned char *adst = ads;
							ads += (ai->ab.actiondefinefunction2s[l]->argumentcount * 8);
							for (unsigned int pd = 0; pd < ai->ab.actiondefinefunction2s[l]->argumentcount; pd++)
							{
								*(unsigned int *)adst = ai->ab.actiondefinefunction2s[l]->arguments[pd]->reg;
								adst += 4;
								*(unsigned char **)adst = ads;
								relocations.push_back((unsigned char **)(adst));
								adst += 4;
								strcpy((char *)ads, ai->ab.actiondefinefunction2s[l]->arguments[pd]->name);
								ads += STRLENGTH(ai->ab.actiondefinefunction2s[l]->arguments[pd]->name);
							}
						}
						for (unsigned int l = 0; l < ai->ab.definefunctioncount; l++)
						{
							*(unsigned char **)(abcs + ai->ab.actiondefinefunctions[l]->definefunctionoffset) = ads;
							relocations.push_back((unsigned char **)(abcs + ai->ab.actiondefinefunctions[l]->definefunctionoffset));
							unsigned char *adst = ads;
							ads += (ai->ab.actiondefinefunctions[l]->argumentcount * 4);
							for (unsigned int pd = 0; pd < ai->ab.actiondefinefunctions[l]->argumentcount; pd++)
							{
								*(unsigned char **)adst = ads;
								relocations.push_back((unsigned char **)(adst));
								adst += 4;
								strcpy((char *)ads, ai->ab.actiondefinefunctions[l]->arguments[pd]);
								ads += STRLENGTH(ai->ab.actiondefinefunctions[l]->arguments[pd]);
							}
						}
						a++;
					}
					fi = (unsigned char *)a;
				}
			}
				break;
			case SHAPE:
			{
				Shape *sh = (Shape *)chd;
				Shape *shi = (Shape *)m->characters[i];
				chd += sizeof(Shape);
				sh->type = shi->type;
				sh->bounds = shi->bounds;
				sh->geometry = shi->geometry;
				sh->signature = shi->signature;
			}
				break;
			case EDITTEXT:
			{
				EditText *et = (EditText *)chd;
				EditText *eti = (EditText *)m->characters[i];
				chd += sizeof(EditText);
				et->type = eti->type;
				et->signature = eti->signature;
				et->bounds = eti->bounds;
				et->font = eti->font;
				et->color = eti->color;
				et->alignment = eti->alignment;
				et->fontheight = eti->fontheight;
				et->readonly = eti->readonly;
				et->multiline = eti->multiline;
				et->wordwrap = eti->wordwrap;
				if (eti->text)
				{
					char *frx = (char *)fr;
					et->text = frx;
					relocations.push_back((unsigned char **)(&et->text));
					strcpy(frx, eti->text);
					frx += STRLENGTH(eti->text);
					fr = (OutputFrame *)frx;
				}
				else
				{
					et->text = 0;
				}
				if (eti->variable)
				{
					char *frx = (char *)fr;
					et->variable = frx;
					relocations.push_back((unsigned char **)(&et->variable));
					strcpy(frx, eti->variable);
					frx += STRLENGTH(eti->variable);
					fr = (OutputFrame *)frx;
				}
				else
				{
					et->variable = 0;
				}
			}
				break;
			case FONT:
			{
				OutputFont *fo = (OutputFont *)chd;
				Font *foi = (Font *)m->characters[i];
				chd += sizeof(OutputFont);
				fo->type = foi->type;
				fo->signature = foi->signature;
				char *frx = (char *)fr;
				fo->name = frx;
				relocations.push_back((unsigned char **)(&fo->name));
				strcpy((char *)frx, foi->name);
				frx += STRLENGTH(foi->name);
				fr = (OutputFrame *)frx;
				if (foi->glyphcount)
				{
					fo->glyphs = (unsigned int *)fr;
					unsigned int *frg = fo->glyphs;
					relocations.push_back((unsigned char **)(&fo->glyphs));
					for (unsigned int j = 0; j < foi->glyphcount; j++)
					{
						*frg = foi->glyphs[j];
						frg++;
					}
					fr = (OutputFrame *)frg;
				}
				else
				{
					fo->glyphs = 0;
				}
			}
				break;
			case SPRITE:
			{
				OutputSprite *spr = (OutputSprite *)chd;
				Sprite *spri = (Sprite *)m->characters[i];
				chd += sizeof(OutputSprite);
				spr->type = spri->type;
				spr->signature = spri->signature;
				spr->framecount = spri->framecount;
				if (spr->framecount)
				{
					spr->frames = fr;
					relocations.push_back((unsigned char **)(&spr->frames));
				}
				else
				{
					spr->frames = 0;
				}
				spr->pointer = spri->pointer;
				for (unsigned int j = 0; j < spri->framecount; j++)
				{
					if (spri->frames[j])
					{
						fr->frameitemcount = spri->frames[j]->frameitemcount;
						fr->frameitems = (FrameItem **)fil;
						relocations.push_back((unsigned char **)(&fr->frameitems));
						fr++;
						for (unsigned int k = 0; k < spri->frames[j]->frameitemcount; k++)
						{
							*fil = fi;
							relocations.push_back((unsigned char **)(fil));
							fil++;
							switch (spri->frames[j]->frameitems[k]->type)
							{
							case ACTION:
							{
								OutputAction *a = (OutputAction *)fi;
								Action *ai = (Action *)spri->frames[j]->frameitems[k];
								fi += sizeof(OutputAction);
								a->type = ai->type;
								relocations.push_back((unsigned char **)(&a->actionbytes));
								std::string s = ai->ab.actionbytes.str();
								const char *sc = s.c_str();
								a->actionbytes = fip;
								unsigned char *abcs = fip;
								memcpy(fip, sc, ai->ab.actionbytecount);
								fip += ai->ab.actionbytecount;
								fip += GETALIGN(ai->ab.actionbytecount);
								if (ai->ab.constantcount)
								{
									*(unsigned char **)(abcs + 8) = ads;
									relocations.push_back((unsigned char **)(abcs + 8));
									for (unsigned int l = 0; l < ai->ab.constantcount; l++)
									{
										*(unsigned int *)ads = ai->ab.constants[l];
										ads += 4;
									}
								}
								for (unsigned int l = 0; l < ai->ab.stringcount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actionstrings[l]->stringoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actionstrings[l]->stringoffset));
									strcpy((char *)ads, ai->ab.actionstrings[l]->string);
									ads += STRLENGTH(ai->ab.actionstrings[l]->string);
								}
								for (unsigned int l = 0; l < ai->ab.pushdatacount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actionpushdatas[l]->pushdataoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actionpushdatas[l]->pushdataoffset));
									for (unsigned int pd = 0; pd < ai->ab.actionpushdatas[l]->pushdatacount; pd++)
									{
										*(unsigned int *)ads = ai->ab.actionpushdatas[l]->pushdata[pd];
										ads += 4;
									}
								}
								for (unsigned int l = 0; l < ai->ab.definefunction2count; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actiondefinefunction2s[l]->definefunction2offset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actiondefinefunction2s[l]->definefunction2offset));
									unsigned char *adst = ads;
									ads += (ai->ab.actiondefinefunction2s[l]->argumentcount * 8);
									for (unsigned int pd = 0; pd < ai->ab.actiondefinefunction2s[l]->argumentcount; pd++)
									{
										*(unsigned int *)adst = ai->ab.actiondefinefunction2s[l]->arguments[pd]->reg;
										adst += 4;
										*(unsigned char **)adst = ads;
										relocations.push_back((unsigned char **)(adst));
										adst += 4;
										strcpy((char *)ads, ai->ab.actiondefinefunction2s[l]->arguments[pd]->name);
										ads += STRLENGTH(ai->ab.actiondefinefunction2s[l]->arguments[pd]->name);
									}
								}
								for (unsigned int l = 0; l < ai->ab.definefunctioncount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actiondefinefunctions[l]->definefunctionoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actiondefinefunctions[l]->definefunctionoffset));
									unsigned char *adst = ads;
									ads += (ai->ab.actiondefinefunctions[l]->argumentcount * 4);
									for (unsigned int pd = 0; pd < ai->ab.actiondefinefunctions[l]->argumentcount; pd++)
									{
										*(unsigned char **)adst = ads;
										relocations.push_back((unsigned char **)(adst));
										adst += 4;
										strcpy((char *)ads, ai->ab.actiondefinefunctions[l]->arguments[pd]);
										ads += STRLENGTH(ai->ab.actiondefinefunctions[l]->arguments[pd]);
									}
								}
							}
								break;
							case FRAMELABEL:
							{
								FrameLabel *fl = (FrameLabel *)fi;
								FrameLabel *fli = (FrameLabel *)spri->frames[j]->frameitems[k];
								fi += sizeof(FrameLabel);
								fl->type = fli->type;
								fl->flags = fli->flags;
								fl->frame = fli->frame;
								fl->label = (char *)fip;
								relocations.push_back((unsigned char **)(&fl->label));
								strcpy((char *)fip, fli->label);
								fip += STRLENGTH(fli->label);
							}
								break;
							case PLACEOBJECT:
							{
								OutputPlaceObject *pl = (OutputPlaceObject *)fi;
								PlaceObject *pli = (PlaceObject *)spri->frames[j]->frameitems[k];
								fi += sizeof(PlaceObject);
								pl->type = pli->type;
								pl->flags = pli->flags;
								pl->depth = pli->depth;
								pl->character = pli->character;
								pl->rotateandscale = pli->rotateandscale;
								pl->translate = pli->translate;
								pl->colortransform = pli->colortransform;
								pl->unknown = pli->unknown;
								pl->ratio = pli->ratio;
								if (pli->name)
								{
									pl->name = (char *)fip;
									relocations.push_back((unsigned char **)(&pl->name));
									strcpy((char *)fip, pli->name);
									fip += STRLENGTH(pli->name);
								}
								else
								{
									pl->name = 0;
								}
								pl->clipdepth = pli->clipdepth;
								if (pli->poa)
								{
									pl->poa = (OutputPlaceObjectActions *)fip;
									relocations.push_back((unsigned char **)(&pl->poa));
									fip += sizeof(OutputPlaceObjectActions);
									pl->poa->clipactioncount = pli->poa->clipactioncount;
									pl->poa->actions = (OutputPlaceObjectAction *)ads;
									relocations.push_back((unsigned char **)(&pl->poa->actions));
									ads += pli->poa->clipactioncount * sizeof(PlaceObjectAction);
									for (unsigned int l = 0; l < pli->poa->clipactioncount; l++)
									{
										pl->poa->actions[l].flags = pli->poa->actions[l]->flags;
										pl->poa->actions[l].flags2 = pli->poa->actions[l]->flags2;
										relocations.push_back((unsigned char **)&pl->poa->actions[l].actiondata);
										pl->poa->actions[l].actiondata = (unsigned char *)ads2;
										unsigned char *abcs = ads2;
										ads2 += pli->poa->actions[l]->actiondata->ab.actionbytecount;
										ads2 += GETALIGN(pli->poa->actions[l]->actiondata->ab.actionbytecount);
										std::string s = pli->poa->actions[l]->actiondata->ab.actionbytes.str();
										const char *sc = s.c_str();
										memcpy(abcs, sc, pli->poa->actions[l]->actiondata->ab.actionbytecount);
										if (pli->poa->actions[l]->actiondata->ab.constantcount)
										{
											*(unsigned char **)(abcs + 8) = ads3;
											relocations.push_back((unsigned char **)(abcs + 8));
											for (unsigned int mx = 0; mx < pli->poa->actions[l]->actiondata->ab.constantcount; mx++)
											{
												*(unsigned int *)ads3 = pli->poa->actions[l]->actiondata->ab.constants[mx];
												ads3 += 4;
											}
										}
										for (unsigned int mx = 0; mx < pli->poa->actions[l]->actiondata->ab.stringcount; mx++)
										{
											*(unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actionstrings[mx]->stringoffset) = ads3;
											relocations.push_back((unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actionstrings[mx]->stringoffset));
											strcpy((char *)ads3, pli->poa->actions[l]->actiondata->ab.actionstrings[mx]->string);
											ads3 += STRLENGTH(pli->poa->actions[l]->actiondata->ab.actionstrings[mx]->string);
										}
										for (unsigned int mx = 0; mx < pli->poa->actions[l]->actiondata->ab.pushdatacount; mx++)
										{
											*(unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actionpushdatas[mx]->pushdataoffset) = ads3;
											relocations.push_back((unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actionpushdatas[mx]->pushdataoffset));
											for (unsigned int pd = 0; pd < pli->poa->actions[l]->actiondata->ab.actionpushdatas[mx]->pushdatacount; pd++)
											{
												*(unsigned int *)ads3 = pli->poa->actions[l]->actiondata->ab.actionpushdatas[mx]->pushdata[pd];
												ads3 += 4;
											}
										}
										for (unsigned int mx = 0; mx < pli->poa->actions[l]->actiondata->ab.definefunction2count; mx++)
										{
											*(unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->definefunction2offset) = ads3;
											relocations.push_back((unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->definefunction2offset));
											unsigned char *adst = ads3;
											ads3 += (pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->argumentcount * 8);
											for (unsigned int pd = 0; pd < pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->argumentcount; pd++)
											{
												*(unsigned int *)adst = pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->arguments[pd]->reg;
												adst += 4;
												*(unsigned char **)adst = ads3;
												relocations.push_back((unsigned char **)(adst));
												adst += 4;
												strcpy((char *)ads3, pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->arguments[pd]->name);
												ads3 += STRLENGTH(pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->arguments[pd]->name);
											}
										}
										for (unsigned int mx = 0; mx < pli->poa->actions[l]->actiondata->ab.definefunctioncount; mx++)
										{
											*(unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[mx]->definefunctionoffset) = ads3;
											relocations.push_back((unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[mx]->definefunctionoffset));
											unsigned char *adst = ads3;
											ads += (pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[mx]->argumentcount * 4);
											for (unsigned int pd = 0; pd < pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[mx]->argumentcount; pd++)
											{
												*(unsigned char **)adst = ads3;
												relocations.push_back((unsigned char **)(adst));
												adst += 4;
												strcpy((char *)ads3, pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[mx]->arguments[pd]);
												ads3 += STRLENGTH(pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[mx]->arguments[pd]);
											}
										}
									}
								}
								else
								{
									pl->poa = 0;
								}
							}
								break;
							case REMOVEOBJECT:
							{
								RemoveObject *r = (RemoveObject *)fi;
								fi += sizeof(RemoveObject);
								RemoveObject *ri = (RemoveObject *)spri->frames[j]->frameitems[k];
								r->type = ri->type;
								r->depth = ri->depth;
							}
								break;
							case BACKGROUNDCOLOR:
							{
								BackgroundColor *bgc = (BackgroundColor *)fi;
								fi += sizeof(BackgroundColor);
								BackgroundColor *bgci = (BackgroundColor *)spri->frames[j]->frameitems[k];
								bgc->type = bgci->type;
								bgc->color = bgci->color;
							}
								break;
							case INITACTION:
							{
								OutputInitAction *a = (OutputInitAction *)fi;
								InitAction *ai = (InitAction *)spri->frames[j]->frameitems[k];
								fi += sizeof(OutputInitAction);
								a->type = ai->type;
								a->sprite = ai->sprite;
								relocations.push_back((unsigned char **)(&a->actionbytes));
								std::string s = ai->ab.actionbytes.str();
								const char *sc = s.c_str();
								a->actionbytes = fip;
								unsigned char *abcs = fip;
								memcpy(fip, sc, ai->ab.actionbytecount);
								fip += ai->ab.actionbytecount;
								fip += GETALIGN(ai->ab.actionbytecount);
								if (ai->ab.constantcount)
								{
									*(unsigned char **)(abcs + 8) = ads;
									relocations.push_back((unsigned char **)(abcs + 8));
									for (unsigned int l = 0; l < ai->ab.constantcount; l++)
									{
										*(unsigned int *)ads = ai->ab.constants[l];
										ads += 4;
									}
								}
								for (unsigned int l = 0; l < ai->ab.stringcount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actionstrings[l]->stringoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actionstrings[l]->stringoffset));
									strcpy((char *)ads, ai->ab.actionstrings[l]->string);
									ads += STRLENGTH(ai->ab.actionstrings[l]->string);
								}
								for (unsigned int l = 0; l < ai->ab.pushdatacount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actionpushdatas[l]->pushdataoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actionpushdatas[l]->pushdataoffset));
									for (unsigned int pd = 0; pd < ai->ab.actionpushdatas[l]->pushdatacount; pd++)
									{
										*(unsigned int *)ads = ai->ab.actionpushdatas[l]->pushdata[pd];
										ads += 4;
									}
								}
								for (unsigned int l = 0; l < ai->ab.definefunction2count; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actiondefinefunction2s[l]->definefunction2offset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actiondefinefunction2s[l]->definefunction2offset));
									unsigned char *adst = ads;
									ads += (ai->ab.actiondefinefunction2s[l]->argumentcount * 8);
									for (unsigned int pd = 0; pd < ai->ab.actiondefinefunction2s[l]->argumentcount; pd++)
									{
										*(unsigned int *)adst = ai->ab.actiondefinefunction2s[l]->arguments[pd]->reg;
										adst += 4;
										*(unsigned char **)adst = ads;
										relocations.push_back((unsigned char **)(adst));
										adst += 4;
										strcpy((char *)ads, ai->ab.actiondefinefunction2s[l]->arguments[pd]->name);
										ads += STRLENGTH(ai->ab.actiondefinefunction2s[l]->arguments[pd]->name);
									}
								}
								for (unsigned int l = 0; l < ai->ab.definefunctioncount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actiondefinefunctions[l]->definefunctionoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actiondefinefunctions[l]->definefunctionoffset));
									unsigned char *adst = ads;
									ads += (ai->ab.actiondefinefunctions[l]->argumentcount * 4);
									for (unsigned int pd = 0; pd < ai->ab.actiondefinefunctions[l]->argumentcount; pd++)
									{
										*(unsigned char **)adst = ads;
										relocations.push_back((unsigned char **)(adst));
										adst += 4;
										strcpy((char *)ads, ai->ab.actiondefinefunctions[l]->arguments[pd]);
										ads += STRLENGTH(ai->ab.actiondefinefunctions[l]->arguments[pd]);
									}
								}
							}
								break;
							}
						}
					}
					else
					{
						fr->frameitemcount = 0;
						fr->frameitems = 0;
						fr++;
					}
				}
			}
				break;
			case IMAGE:
			{
				Image *im = (Image *)chd;
				Image *imi = (Image *)m->characters[i];
				chd += sizeof(Image);
				im->type = imi->type;
				im->texture = imi->texture;
				im->signature = imi->signature;
			}
				break;
			case MORPH:
			{
				Morph *mo = (Morph *)chd;
				Morph *moi = (Morph *)m->characters[i];
				chd += sizeof(Morph);
				mo->type = moi->type;
				mo->signature = moi->signature;
				mo->startshape = moi->startshape;
				mo->endshape = moi->endshape;
			}
				break;
			case MOVIE:
			{
				OutputMovie *mov = (OutputMovie *)chd;
				Movie *movi = (Movie *)m->characters[i];
				chd += sizeof(OutputMovie);
				mov->type = movi->type;
				mov->signature = movi->signature;
				mov->framecount = movi->framecount;
				if (mov->framecount)
				{
					mov->frames = fr;
					relocations.push_back((unsigned char **)(&mov->frames));
				}
				else
				{
					mov->frames = 0;
				}
				mov->pointer = movi->pointer;
				mov->charactercount = movi->charactercount;
				mov->characters = (Character **)ch;
				relocations.push_back((unsigned char **)(&mov->characters));
				mov->screensizex = movi->screensizex;
				mov->screensizey = movi->screensizey;
				mov->importcount = movi->importcount;
				mov->unknown = movi->unknown;
				if (mov->importcount)
				{
					mov->imports = imp;
				}
				else
				{
					mov->imports = (Import *)ch;
				}
				relocations.push_back((unsigned char **)(&mov->imports));
				mov->exportcount = movi->exportcount;
				if (mov->exportcount)
				{
					mov->exports = ex;
				}
				else
				{
					mov->exports = (Export *)ch;
				}
				relocations.push_back((unsigned char **)(&mov->exports));
				mov->count = movi->count;
				for (unsigned int j = 0; j < movi->framecount; j++)
				{
					if (movi->frames[j])
					{
						fr->frameitemcount = movi->frames[j]->frameitemcount;
						fr->frameitems = (FrameItem **)fil;
						relocations.push_back((unsigned char **)(&fr->frameitems));
						fr++;
						for (unsigned int k = 0; k < movi->frames[j]->frameitemcount; k++)
						{
							*fil = fi;
							relocations.push_back((unsigned char **)(fil));
							fil++;
							switch (movi->frames[j]->frameitems[k]->type)
							{
							case ACTION:
							{
								OutputAction *a = (OutputAction *)fi;
								Action *ai = (Action *)movi->frames[j]->frameitems[k];
								fi += sizeof(OutputAction);
								a->type = ai->type;
								relocations.push_back((unsigned char **)(&a->actionbytes));
								std::string s = ai->ab.actionbytes.str();
								const char *sc = s.c_str();
								a->actionbytes = fip;
								unsigned char *abcs = fip;
								memcpy(fip, sc, ai->ab.actionbytecount);
								fip += ai->ab.actionbytecount;
								fip += GETALIGN(ai->ab.actionbytecount);
								if (ai->ab.constantcount)
								{
									*(unsigned char **)(abcs + 8) = ads;
									relocations.push_back((unsigned char **)(abcs + 8));
									for (unsigned int l = 0; l < ai->ab.constantcount; l++)
									{
										*(unsigned int *)ads = ai->ab.constants[l];
										ads += 4;
									}
								}
								for (unsigned int l = 0; l < ai->ab.stringcount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actionstrings[l]->stringoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actionstrings[l]->stringoffset));
									strcpy((char *)ads, ai->ab.actionstrings[l]->string);
									ads += STRLENGTH(ai->ab.actionstrings[l]->string);
								}
								for (unsigned int l = 0; l < ai->ab.pushdatacount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actionpushdatas[l]->pushdataoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actionpushdatas[l]->pushdataoffset));
									for (unsigned int pd = 0; pd < ai->ab.actionpushdatas[l]->pushdatacount; pd++)
									{
										*(unsigned int *)ads = ai->ab.actionpushdatas[l]->pushdata[pd];
										ads += 4;
									}
								}
								for (unsigned int l = 0; l < ai->ab.definefunction2count; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actiondefinefunction2s[l]->definefunction2offset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actiondefinefunction2s[l]->definefunction2offset));
									unsigned char *adst = ads;
									ads += (ai->ab.actiondefinefunction2s[l]->argumentcount * 8);
									for (unsigned int pd = 0; pd < ai->ab.actiondefinefunction2s[l]->argumentcount; pd++)
									{
										*(unsigned int *)adst = ai->ab.actiondefinefunction2s[l]->arguments[pd]->reg;
										adst += 4;
										*(unsigned char **)adst = ads;
										relocations.push_back((unsigned char **)(adst));
										adst += 4;
										strcpy((char *)ads, ai->ab.actiondefinefunction2s[l]->arguments[pd]->name);
										ads += STRLENGTH(ai->ab.actiondefinefunction2s[l]->arguments[pd]->name);
									}
								}
								for (unsigned int l = 0; l < ai->ab.definefunctioncount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actiondefinefunctions[l]->definefunctionoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actiondefinefunctions[l]->definefunctionoffset));
									unsigned char *adst = ads;
									ads += (ai->ab.actiondefinefunctions[l]->argumentcount * 4);
									for (unsigned int pd = 0; pd < ai->ab.actiondefinefunctions[l]->argumentcount; pd++)
									{
										*(unsigned char **)adst = ads;
										relocations.push_back((unsigned char **)(adst));
										adst += 4;
										strcpy((char *)ads, ai->ab.actiondefinefunctions[l]->arguments[pd]);
										ads += STRLENGTH(ai->ab.actiondefinefunctions[l]->arguments[pd]);
									}
								}
							}
								break;
							case FRAMELABEL:
							{
								FrameLabel *fl = (FrameLabel *)fi;
								FrameLabel *fli = (FrameLabel *)movi->frames[j]->frameitems[k];
								fi += sizeof(FrameLabel);
								fl->type = fli->type;
								fl->flags = fli->flags;
								fl->frame = fli->frame;
								fl->label = (char *)fip;
								relocations.push_back((unsigned char **)(&fl->label));
								strcpy((char *)fip, fli->label);
								fip += STRLENGTH(fli->label);
							}
								break;
							case PLACEOBJECT:
							{
								OutputPlaceObject *pl = (OutputPlaceObject *)fi;
								PlaceObject *pli = (PlaceObject *)movi->frames[j]->frameitems[k];
								fi += sizeof(PlaceObject);
								pl->type = pli->type;
								pl->flags = pli->flags;
								pl->depth = pli->depth;
								pl->character = pli->character;
								pl->rotateandscale = pli->rotateandscale;
								pl->translate = pli->translate;
								pl->colortransform = pli->colortransform;
								pl->unknown = pli->unknown;
								pl->ratio = pli->ratio;
								if (pli->name)
								{
									pl->name = (char *)fip;
									relocations.push_back((unsigned char **)(&pl->name));
									strcpy((char *)fip, pli->name);
									fip += STRLENGTH(pli->name);
								}
								else
								{
									pl->name = 0;
								}
								pl->clipdepth = pli->clipdepth;
								if (pli->poa)
								{
									pl->poa = (OutputPlaceObjectActions *)fip;
									relocations.push_back((unsigned char **)(&pl->poa));
									fip += sizeof(OutputPlaceObjectActions);
									pl->poa->clipactioncount = pli->poa->clipactioncount;
									pl->poa->actions = (OutputPlaceObjectAction *)ads;
									relocations.push_back((unsigned char **)(&pl->poa->actions));
									ads += pli->poa->clipactioncount * sizeof(PlaceObjectAction);
									for (unsigned int l = 0; l < pli->poa->clipactioncount; l++)
									{
										pl->poa->actions[l].flags = pli->poa->actions[l]->flags;
										pl->poa->actions[l].flags2 = pli->poa->actions[l]->flags2;
										relocations.push_back((unsigned char **)&pl->poa->actions[l].actiondata);
										pl->poa->actions[l].actiondata = (unsigned char *)ads2;
										unsigned char *abcs = ads2;
										ads2 += pli->poa->actions[l]->actiondata->ab.actionbytecount;
										ads2 += GETALIGN(pli->poa->actions[l]->actiondata->ab.actionbytecount);
										std::string s = pli->poa->actions[l]->actiondata->ab.actionbytes.str();
										const char *sc = s.c_str();
										memcpy(abcs, sc, pli->poa->actions[l]->actiondata->ab.actionbytecount);
										if (pli->poa->actions[l]->actiondata->ab.constantcount)
										{
											*(unsigned char **)(abcs + 8) = ads3;
											relocations.push_back((unsigned char **)(abcs + 8));
											for (unsigned int mx = 0; mx < pli->poa->actions[l]->actiondata->ab.constantcount; mx++)
											{
												*(unsigned int *)ads3 = pli->poa->actions[l]->actiondata->ab.constants[mx];
												ads3 += 4;
											}
										}
										for (unsigned int mx = 0; mx < pli->poa->actions[l]->actiondata->ab.stringcount; mx++)
										{
											unsigned int so = pli->poa->actions[l]->actiondata->ab.actionstrings[mx]->stringoffset;
											*(unsigned char **)(abcs + so) = ads3;
											relocations.push_back((unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actionstrings[mx]->stringoffset));
											strcpy((char *)ads3, pli->poa->actions[l]->actiondata->ab.actionstrings[mx]->string);
											ads3 += STRLENGTH(pli->poa->actions[l]->actiondata->ab.actionstrings[mx]->string);
										}
										for (unsigned int mx = 0; mx < pli->poa->actions[l]->actiondata->ab.pushdatacount; mx++)
										{
											*(unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actionpushdatas[mx]->pushdataoffset) = ads3;
											relocations.push_back((unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actionpushdatas[mx]->pushdataoffset));
											for (unsigned int pd = 0; pd < pli->poa->actions[l]->actiondata->ab.actionpushdatas[mx]->pushdatacount; pd++)
											{
												*(unsigned int *)ads3 = pli->poa->actions[l]->actiondata->ab.actionpushdatas[mx]->pushdata[pd];
												ads3 += 4;
											}
										}
										for (unsigned int mx = 0; mx < pli->poa->actions[l]->actiondata->ab.definefunction2count; mx++)
										{
											*(unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->definefunction2offset) = ads3;
											relocations.push_back((unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->definefunction2offset));
											unsigned char *adst = ads3;
											ads3 += (pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->argumentcount * 8);
											for (unsigned int pd = 0; pd < pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->argumentcount; pd++)
											{
												*(unsigned int *)adst = pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->arguments[pd]->reg;
												adst += 4;
												*(unsigned char **)adst = ads3;
												relocations.push_back((unsigned char **)(adst));
												adst += 4;
												strcpy((char *)ads3, pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->arguments[pd]->name);
												ads3 += STRLENGTH(pli->poa->actions[l]->actiondata->ab.actiondefinefunction2s[mx]->arguments[pd]->name);
											}
										}
										for (unsigned int mx = 0; mx < pli->poa->actions[l]->actiondata->ab.definefunctioncount; mx++)
										{
											*(unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[l]->definefunctionoffset) = ads3;
											relocations.push_back((unsigned char **)(abcs + pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[mx]->definefunctionoffset));
											unsigned char *adst = ads3;
											ads += (pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[mx]->argumentcount * 4);
											for (unsigned int pd = 0; pd < pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[mx]->argumentcount; pd++)
											{
												*(unsigned char **)adst = ads3;
												relocations.push_back((unsigned char **)(adst));
												adst += 4;
												strcpy((char *)ads3, pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[mx]->arguments[pd]);
												ads3 += STRLENGTH(pli->poa->actions[l]->actiondata->ab.actiondefinefunctions[mx]->arguments[pd]);
											}
										}
									}
								}
								else
								{
									pl->poa = 0;
								}
							}
								break;
							case REMOVEOBJECT:
							{
								RemoveObject *r = (RemoveObject *)fi;
								fi += sizeof(RemoveObject);
								RemoveObject *ri = (RemoveObject *)movi->frames[j]->frameitems[k];
								r->type = ri->type;
								r->depth = ri->depth;
							}
								break;
							case BACKGROUNDCOLOR:
							{
								BackgroundColor *bgc = (BackgroundColor *)fi;
								fi += sizeof(BackgroundColor);
								BackgroundColor *bgci = (BackgroundColor *)movi->frames[j]->frameitems[k];
								bgc->type = bgci->type;
								bgc->color = bgci->color;
							}
								break;
							case INITACTION:
							{
								OutputInitAction *a = (OutputInitAction *)fi;
								InitAction *ai = (InitAction *)movi->frames[j]->frameitems[k];
								fi += sizeof(OutputInitAction);
								a->type = ai->type;
								a->sprite = ai->sprite;
								relocations.push_back((unsigned char **)(&a->actionbytes));
								std::string s = ai->ab.actionbytes.str();
								const char *sc = s.c_str();
								a->actionbytes = fip;
								unsigned char *abcs = fip;
								memcpy(fip, sc, ai->ab.actionbytecount);
								fip += ai->ab.actionbytecount;
								fip += GETALIGN(ai->ab.actionbytecount);
								if (ai->ab.constantcount)
								{
									*(unsigned char **)(abcs + 8) = ads;
									relocations.push_back((unsigned char **)(abcs + 8));
									for (unsigned int l = 0; l < ai->ab.constantcount; l++)
									{
										*(unsigned int *)ads = ai->ab.constants[l];
										ads += 4;
									}
								}
								for (unsigned int l = 0; l < ai->ab.stringcount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actionstrings[l]->stringoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actionstrings[l]->stringoffset));
									strcpy((char *)ads, ai->ab.actionstrings[l]->string);
									ads += STRLENGTH(ai->ab.actionstrings[l]->string);
								}
								for (unsigned int l = 0; l < ai->ab.pushdatacount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actionpushdatas[l]->pushdataoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actionpushdatas[l]->pushdataoffset));
									for (unsigned int pd = 0; pd < ai->ab.actionpushdatas[l]->pushdatacount; pd++)
									{
										*(unsigned int *)ads = ai->ab.actionpushdatas[l]->pushdata[pd];
										ads += 4;
									}
								}
								for (unsigned int l = 0; l < ai->ab.definefunction2count; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actiondefinefunction2s[l]->definefunction2offset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actiondefinefunction2s[l]->definefunction2offset));
									unsigned char *adst = ads;
									ads += (ai->ab.actiondefinefunction2s[l]->argumentcount * 8);
									for (unsigned int pd = 0; pd < ai->ab.actiondefinefunction2s[l]->argumentcount; pd++)
									{
										*(unsigned int *)adst = ai->ab.actiondefinefunction2s[l]->arguments[pd]->reg;
										adst += 4;
										*(unsigned char **)adst = ads;
										relocations.push_back((unsigned char **)(adst));
										adst += 4;
										strcpy((char *)ads, ai->ab.actiondefinefunction2s[l]->arguments[pd]->name);
										ads += STRLENGTH(ai->ab.actiondefinefunction2s[l]->arguments[pd]->name);
									}
								}
								for (unsigned int l = 0; l < ai->ab.definefunctioncount; l++)
								{
									*(unsigned char **)(abcs + ai->ab.actiondefinefunctions[l]->definefunctionoffset) = ads;
									relocations.push_back((unsigned char **)(abcs + ai->ab.actiondefinefunctions[l]->definefunctionoffset));
									unsigned char *adst = ads;
									ads += (ai->ab.actiondefinefunctions[l]->argumentcount * 4);
									for (unsigned int pd = 0; pd < ai->ab.actiondefinefunctions[l]->argumentcount; pd++)
									{
										*(unsigned char **)adst = ads;
										relocations.push_back((unsigned char **)(adst));
										adst += 4;
										strcpy((char *)ads, ai->ab.actiondefinefunctions[l]->arguments[pd]);
										ads += STRLENGTH(ai->ab.actiondefinefunctions[l]->arguments[pd]);
									}
								}
							}
								break;
							}
						}
					}
					else
					{
						fr->frameitemcount = 0;
						fr->frameitems = 0;
						fr++;
					}
				}
			}
				break;
			case TEXT:
			{
				OutputText *ot = (OutputText *)chd;
				Text *t = (Text *)m->characters[i];
				chd += sizeof(OutputText);
				ot->type = t->type;
				ot->signature = t->signature;
				ot->bounds = t->bounds;
				ot->rotateandscale = t->rotateandscale;
				ot->translate = t->translate;
				ot->recordcount = t->recordcount;
				if (ot->recordcount)
				{
					OutputTextRecord *tr = (OutputTextRecord *)fi;
					ot->records = tr;
					relocations.push_back((unsigned char **)(&ot->records));
					for (unsigned int rc = 0; rc < ot->recordcount; rc++)
					{
						tr->color = t->records[rc]->color;
						tr->font = t->records[rc]->font;
						tr->offset = t->records[rc]->offset;
						tr->textheight = t->records[rc]->textheight;
						tr->unknown = t->records[rc]->unknown;
						tr->glyphcount = t->records[rc]->glyphcount;
						Glyph *gl = (Glyph *)fip;
						tr->glyphs = gl;
						for (unsigned int ac = 0; ac < t->records[rc]->glyphcount; ac++)
						{
							gl->advance = t->records[rc]->glyphs[ac]->advance;
							gl->index = t->records[rc]->glyphs[ac]->index;
							gl++;
						}
						fip = (unsigned char *)gl;
						tr++;
					}
					fi = (unsigned char *)tr;
				}
			}
				break;
			}
			ch++;
		}
		else
		{
			*ch = 0;
			ch++;
		}
	}
	for (unsigned int i = 0; i < relocations.size(); i++)
	{
		unsigned char **c = relocations[i];
		(*c) = (unsigned char *)((*c) - aptdata);
	}
	FILE *f = fopen(filename, "wb");
	if (!f)
	{
		return -1;
	}

	//calculate the const offset
	unsigned int offset = 12;
	for (unsigned int i = 0; i < m->importcount; ++i)
	{
		offset += sizeof(Import);
		offset += STRLENGTH(m->imports[i]->movie);
		offset += STRLENGTH(m->imports[i]->name);
	}
	for (unsigned int i = 0; i < m->exportcount; ++i)
	{
		offset += sizeof(Export);
		offset += STRLENGTH(m->exports[i]->name);
	}
	result = *(uint32_t*)(aptdata + offset);


	fwrite(aptdata, aptdatasize, 1, f);
	fclose(f);

	return result;
}

void AptFile::GenerateAptConstFile(AptConstData *c, const char *filename)
{
	unsigned int aptconstsize = 0x20 + (c->itemcount * 8);
	std::vector <unsigned char **> relocations;
	for (unsigned int i = 0; i < c->itemcount; i++)
	{
		if (c->items[i]->type == TYPE_STRING)
		{
			if (c->items[i]->strvalue)
			{
				aptconstsize += STRLENGTH(c->items[i]->strvalue);
			}
			else
			{
				aptconstsize += 1;
			}
		}
	}
	unsigned char *aptconstdata = new unsigned char[aptconstsize];
	memset(aptconstdata, 0, aptconstsize);
	unsigned char *aptconst = aptconstdata;
	memcpy(aptconst, "Apt constant file\x1A\0\0", 20);
	aptconst += 20;
	*(unsigned int *)aptconst = c->aptdataoffset;
	aptconst += 4;
	*(unsigned int *)aptconst = c->itemcount;
	aptconst += 4;
	unsigned int n = 0x20;
	*(unsigned int *)aptconst = n;
	aptconst += 4;
	AptConstItem *aci = (AptConstItem *)aptconst;
	aptconst += c->itemcount * 8;
	for (unsigned int i = 0; i < c->itemcount; i++)
	{
		aci->type = c->items[i]->type;
		if (c->items[i]->type == TYPE_STRING)
		{
			aci->strvalue = (char *)aptconst;
			relocations.push_back((unsigned char **)(&aci->strvalue));
			if (c->items[i]->strvalue)
			{
				strcpy((char *)aptconst, c->items[i]->strvalue);
				aptconst += STRLENGTH(c->items[i]->strvalue);
			}
			else
			{
				aptconst++;
			}
		}
		else
		{
			aci->numvalue = c->items[i]->numvalue;
		}
		aci++;
	}
	for (unsigned int i = 0; i < relocations.size(); i++)
	{
		unsigned char **rc = relocations[i];
		(*rc) = (unsigned char *)((*rc) - aptconstdata);
	}
	FILE *f = fopen(filename, "wb");
	if (!f)
	{
		return;
	}
	fwrite(aptconstdata, aptconstsize, 1, f);
	fclose(f);
}



#pragma endregion