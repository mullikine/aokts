#include "TrigScrawlVisitor.h"
#include "../util/utilio.h"

#include <string>
#include <algorithm>
#include <sstream>

using std::string;

TrigScrawlVisitor::TrigScrawlVisitor(std::ostringstream& ss)
: _ss(ss), _trigcount(0)
{
}

TrigScrawlVisitor::~TrigScrawlVisitor()
{
}

void TrigScrawlVisitor::visit(Trigger& t)
{
    const char * NEWLINE = "\r\n";

    _ss << t.getIDName(true) << " ";
    if (!t.state) {
        _ss << "(off) ";
    }
    if (t.loop) {
        _ss << "Looping";
    } else {
        _ss << "Trigger";
    }
    const char * name = t.name;
    if (strlen(name) > 0)
        _ss << ": " << name;
    _ss << NEWLINE;
}

void TrigScrawlVisitor::visit(Effect& e)
{
    const char * NEWLINE = "\r\n";

	_ss << "\tE: " << e.getName().c_str() << ": " << e.getName(true, NameFlags::NONE, 1).c_str() << NEWLINE;
}

void TrigScrawlVisitor::visit(Condition& c)
{
    const char * NEWLINE = "\r\n";

	_ss << "\tC: " << c.getName().c_str() << ": " << c.getName(true, NameFlags::NONE, 1).c_str() << NEWLINE;
}

void TrigScrawlVisitor::visitEnd(Trigger&)
{
}
