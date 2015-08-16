#ifndef AOKTS_TRIGGER_H
#define AOKTS_TRIGGER_H

/* MODEL */

typedef struct _iobuf FILE;	//makes including <stdio.h> unnecessary

#include "scen_const.h"
#include "Effect.h"
#include "Condition.h"
#include <vector>

// Fwd declaration, don't want to #include it.
class TriggerVisitor;

/* Triggers */

#pragma pack(push, 1)

class Trigger
{
	void read(FILE *in);
	void write(FILE *out);

public:
	Trigger();
	Trigger(const Trigger &t);
	Trigger(Buffer&);

	void sort_conditions();
	void sort_effects();

    // each trigger must know what their id is in scen.triggers.
	long id;
    // the display order is not written along with the rest of the
    // trigger's data. all display orders are recorded and loaded from
    // t_order.
	long display_order;
	long state;
	char loop;
	long u1;
	//unsigned short u2;
	char obj;
	long obj_order;
	long obj_str_id;
	SString description;
	char name[MAX_TRIGNAME]; //TODO: SString?
	std::vector<Effect> effects;
	std::vector<Condition> conds;

    std::string getIDName();
    std::string getName(bool tip = false, bool limitlen = false, int recursion = 0);
	void tobuffer(Buffer&);// const (make it const when unit_cnst gets set elsewhere)

	/*	get_player: Returns non-GAIA player for conditions/effect in the trigger.

		Returns:
		* 0 if all players are unassigned (GAIA)
		* -1 if there is a mix of non-GAIA players
		* EC player index if above are false
	*/
	int get_player();

	/**
	 * Accepts a TriggerVisitor and sends it along to the effects and
	 * conditions as well. (See TriggerVisitor.h)
	 */
	void accept(TriggerVisitor&);

	friend class Scenario;
};

#pragma pack(pop)

#endif // AOKTS_TRIGGER_H
