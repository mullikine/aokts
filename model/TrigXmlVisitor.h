#ifndef INC_TRIGXMLVISIT_H
#define INC_TRIGXMLVISIT_H

/*
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION.

	TrigXmlVisitor.h -- declares class TrigXmlVisitor

	MODEL
*/

#include "TriggerVisitor.h"

/**
 * TriggerVisitor to print triggers, effects, and conditions to a file in XML
 * format.
 */
class TrigXmlVisitor : public TriggerVisitor
{
public:
	TrigXmlVisitor(std::ostringstream&);
	~TrigXmlVisitor();

	void visit(Trigger&);
	void visit(Effect&);
	void visit(Condition&);
	void visitEnd(Trigger&);

private:
    std::ostringstream& _ss;
	unsigned _trigcount;
};

#endif // INC_TRIGXMLVISIT_H
