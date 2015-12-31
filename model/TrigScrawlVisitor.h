#ifndef INC_TRIGSCRAWLVISIT_H
#define INC_TRIGSCRAWLVISIT_H
/*
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION.

	TrigScrawlVisitor.h -- declares class TrigScrawlVisitor

	MODEL
*/

#include "TriggerVisitor.h"

/**
 * TriggerVisitor to print triggers, effects, and conditions in human
 * readable format.
 */
class TrigScrawlVisitor: public TriggerVisitor
{
public:
	TrigScrawlVisitor(std::ostringstream&);
	~TrigScrawlVisitor();

	void visit(Trigger&);
	void visit(Effect&);
	void visit(Condition&);
	void visitEnd(Trigger&);

private:
    std::ostringstream& _ss;
	unsigned _trigcount;
};

#endif // INC_TRIGSCRAWLVISIT_H
