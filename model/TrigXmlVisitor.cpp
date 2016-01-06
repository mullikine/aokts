#include "TrigXmlVisitor.h"
#include "../util/utilio.h"

#include <string>
#include <algorithm>
#include <sstream>

using std::string;

const char * NEWLINE = "\r\n";

string escape(const char * str)
{
	static const std::string lt = "&lt;";
	static const std::string gt = "&gt;";
	static const std::string amp = "&amp;";

	// Use a string as a buffer.
	string s;

	// We'll need at least the length of the string
	s.reserve(strlen(str));

	// Copy string character-by-character so that we can replace special XML
	// chars: <>&
	for (; *str; ++str)
	{
		switch (*str)
		{
		case '<':
			s.insert(s.end(), lt.begin(), lt.end());
			break;
		case '>':
			s.insert(s.end(), gt.begin(), gt.end());
			break;
		case '&':
			s.insert(s.end(), amp.begin(), amp.end());
			break;
		default:
			s.push_back(*str);
		}
	}

	return s;
}

/**
 * Writes specified value with specified tag, if value != -1.
 */
inline void writeXmlLong(std::ostringstream * ss, const char * tag, long value)
{
	if (value != -1)
		(*ss) << "\t\t<" << tag << ">" << value << "</" << tag << ">" << NEWLINE;
}

inline void writeXmlString(std::ostringstream * ss, const char * tag, const SString& str)
{
	if (str.length())
		(*ss) << "\t\t<" << tag << ">" << escape(str.c_str()).c_str() << "</" << tag << ">" << NEWLINE;
}

inline void writeXml(std::ostringstream * ss, const char * tag, const AOKRECT& rect)
{
	if (rect.top != -1) // just assume the rest is
	{
		(*ss) << "\t\t<" << tag << "_ur>" << rect.right << "," << rect.top << "</" << tag << "_ur>" << NEWLINE;
		(*ss) << "\t\t<" << tag << "_ll>" << rect.left << "," << rect.bottom << "</" << tag << "_ll>" << NEWLINE;
	}
}

inline void writeXml(std::ostringstream * ss, const char * tag, const AOKPT& pt)
{
	if (pt.x != -1) // just assume the rest is
		(*ss) << "\t\t<" << tag << ">" << pt.x << "," << pt.y << "</" << tag << ">" << NEWLINE;
}


TrigXmlVisitor::TrigXmlVisitor(std::ostringstream& ss)
: _ss(ss), _trigcount(0)
{
	static const char * xmlheader =
		"<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\r\n";

	_ss << xmlheader;
	_ss << "<triggers>" << NEWLINE;
}

TrigXmlVisitor::~TrigXmlVisitor()
{
	_ss << "</triggers>" << NEWLINE;
}

void TrigXmlVisitor::visit(Trigger& t)
{
	_ss << "<trigger id=\"" << _trigcount++ << "\" display_order=\"" << t.display_order << "\">" << NEWLINE;
	_ss << "\t<name>" << escape(t.name).c_str() << "</name>" << NEWLINE;
	_ss << "\t<tip>" << t.getName(true).c_str() << "</tip>" << NEWLINE;
	_ss << "\t<enabled>" << t.state << "</enabled>" << NEWLINE;
	_ss << "\t<looping>" << (int)t.loop << "</looping>" << NEWLINE;
	_ss << "\t<objective>" << (int)t.obj << "</objective>" << NEWLINE;
	_ss << "\t<desc_order>" << t.obj_order << "</desc_order>" << NEWLINE;
	if (t.description.length())
		_ss << "\t<description>" << escape(t.description.c_str()).c_str() << "</description>" << NEWLINE;
}

void TrigXmlVisitor::visit(Effect& e)
{
	_ss << "\t<effect>" << NEWLINE;

	_ss << "\t\t<type>" << e.getName().c_str() << "</type>" << NEWLINE;
	_ss << "\t\t<tip>" << e.getName(true).c_str() << "</tip>" << NEWLINE;

	writeXmlLong(&_ss, "ai_goal", e.ai_goal);
	writeXmlLong(&_ss, "amount", e.amount);
	writeXmlLong(&_ss, "resource", e.res_type);
	writeXmlLong(&_ss, "diplomacy", e.diplomacy);
	writeXmlLong(&_ss, "num_selected", e.num_sel); // TODO: don't write this
    std::ostringstream ss;
    if (e.num_sel == 0)
        ss << -1;
    else if (e.num_sel > 0)
        ss << e.uids[0];
	for (int i = 1; i < e.num_sel; i++) {
        ss << "," << e.uids[i];
	}
	writeXmlString(&_ss, "unit_ids", SString(ss.str().c_str()));
	writeXmlLong(&_ss, "location_unit", e.uid_loc);
	if (e.pUnit)
		writeXmlLong(&_ss, "unit_type", e.pUnit->id());
	writeXmlLong(&_ss, "player_source", e.s_player);
	writeXmlLong(&_ss, "player_target", e.t_player);
	if (e.pTech)
		writeXmlLong(&_ss, "technology", e.pTech->id());
	writeXmlLong(&_ss, "textid", e.textid);
	writeXmlLong(&_ss, "display_time", e.disp_time);
	writeXmlLong(&_ss, "trigger", e.trig_index);
	writeXml(&_ss, "location", e.location);
	writeXml(&_ss, "area", e.area);
	writeXmlLong(&_ss, "unit_group", e.group);
	writeXmlLong(&_ss, "building_type", e.utype);
	writeXmlLong(&_ss, "instruction_panel", e.panel);
	writeXmlString(&_ss, "text", e.text);
	writeXmlString(&_ss, "sound_file", e.sound);
	writeXmlLong(&_ss, "stance", e.stance);

	_ss << "\t</effect>" << NEWLINE;
}

void TrigXmlVisitor::visit(Condition& c)
{
	_ss << "\t<condition>" << NEWLINE;

	_ss << "\t\t<type>" << c.getName().c_str() << "</type>" << NEWLINE;
	_ss << "\t\t<tip>" << c.getName(true).c_str() << "</tip>" << NEWLINE;

	writeXmlLong(&_ss, "amount", c.amount);
	writeXmlLong(&_ss, "resource", c.res_type);
	writeXmlLong(&_ss, "unit_object", c.object);
	writeXmlLong(&_ss, "unit_location", c.u_loc);
	if (c.pUnit)
		writeXmlLong(&_ss, "unit_type", c.pUnit->id());
	writeXmlLong(&_ss, "player", c.player);
	if (c.pTech)
		writeXmlLong(&_ss, "technology", c.pTech->id());
	writeXmlLong(&_ss, "timer", c.timer);
	writeXml(&_ss, "area", c.area);
	writeXmlLong(&_ss, "unit_group", c.group);
	writeXmlLong(&_ss, "building_type", c.utype);
	writeXmlLong(&_ss, "ai_signal", c.ai_signal);
	writeXmlLong(&_ss, "reverse_hd", c.reverse_hd);
	writeXmlLong(&_ss, "unknown", c.unknown2);

	_ss << "\t</condition>" << NEWLINE;
}

void TrigXmlVisitor::visitEnd(Trigger&)
{
	_ss << "</trigger>" << NEWLINE;
}
