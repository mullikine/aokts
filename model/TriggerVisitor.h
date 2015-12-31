#ifndef INC_TRIGVISIT_H
#define INC_TRIGVISIT_H

/*
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION.

	TriggerVisitor.h -- declaraing for TriggerVisitor

	MODEL
*/

#include "trigger.h"

/**
 * This is an abstract class defining an interface for an object that visits
 * triggers and their conditions and effects. See "Visitor Pattern" in Gamma,
 * et. al.
 */
class TriggerVisitor
{
public:
	virtual void visit(Trigger&) = 0;
	virtual void visit(Effect&) = 0;
	virtual void visit(Condition&) = 0;

	/**
	 * This is called after a Trigger has called accept() on all of its
	 * children. It's provided in case the visitor needs to do any cleanup
	 * after visiting effects and conditions.
	 */
	virtual void visitEnd(Trigger&)
	{}
};

#endif // INC_TRIGVISIT_H
