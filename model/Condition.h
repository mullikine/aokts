#ifndef AOKTS_CONDITION_H
#define AOKTS_CONDITION_H

#include "ECBase.h"

// Fwd declaration, don't want to #include it.
class TriggerVisitor;

#pragma pack(push, 4)	//everything in effects/conditions is a long

struct DifficultyLevel {
    enum Value {
        Hardest,
        Hard,
        Moderate,
        Standard,
        Easiest,
    };
};

struct ConditionVirtualTypeDefault {
    enum Value {
	    None,
	    DifficultyLevelHardest,
	    DifficultyLevelHard,
	    DifficultyLevelModerate,
	    DifficultyLevelStandard,
	    DifficultyLevelEasiest,
    };
};

struct ConditionVirtualTypeAOC {
    enum Value {
	    None,
	    SinglePlayer,
	    Taunt,
	    AIScriptGoal,
	    DifficultyLevelHardest,
	    DifficultyLevelHard,
	    DifficultyLevelModerate,
	    DifficultyLevelStandard,
	    DifficultyLevelEasiest,
	    StartingAgeStandard,
	    StartingResourcesStandard,
	    Regicide,
	    Deathmatch,
	    OneClickGarrison,
    };
};

struct ConditionType {
    enum Value {
	    None,
	    BringObjectToArea,
	    BringObjectToObject,
	    OwnObjects,
	    OwnFewerObjects,
	    ObjectsInArea,
	    DestroyObject,
	    CaptureObject,
	    AccumulateAttribute,
	    ResearchTehcnology,
	    Timer,
	    ObjectSelected,
	    AISignal,
	    PlayerDefeated,
	    ObjectHasTarget,
	    ObjectVisible,
	    ObjectNotVisible,
	    ResearchingTechnology,
	    UnitsGarrisoned,
	    DifficultyLevel,
	    OwnFewerFoundations_SWGB,
	        Chance_HD = OwnFewerFoundations_SWGB,
			Chance_UP = OwnFewerFoundations_SWGB,
	    SelectedObjectsInArea_SWGB,
	    PoweredObjectsInArea_SWGB,
	    UnitsQueuedPastPopCap_SWGB,
	};
};

class Condition : public ECBase
{
public:
	Condition();
	Condition(Buffer&); // reads back data written by tobuffer()

	// rule of three: default copy constructor, destructor, assignment fine

	void read(FILE *in);
	void write(FILE *out);
	void tobuffer(Buffer &b);// const; (make it const when unit_cnst gets set elsewhere)
	void compress();

    std::string selectedUnits() const;
	std::string getName(bool tip = false, NameFlags::Value flag=NameFlags::NONE, int recursion = 0) const;

	int getPlayer() const;
	void setPlayer(int);
    bool get_valid_since_last_check();
    bool check_and_save();
	bool check() const;

	/**
	 * Accepts a TriggerVisitor.
	 */
	void accept(TriggerVisitor&);

    static const long defaultvals[];

	long amount;
	long res_type;
	UID object;
	UID u_loc;
	long unit_cnst; // pUnit redundant
	long player;	// GAIA = 0, Player 1 = 1, ...
	long tech_cnst; // pTech redundant
	long timer;
	long reserved;
	AOKRECT area;
	long group;
	long utype;
	long ai_signal;
	long reverse_hd;
	long unknown2;

	const UnitLink *pUnit;	//long in file, of course
	const TechLink *pTech;	//long in file, of course

    static const int NUM_CONDITIONS_AOK = 20;
	static const int NUM_CONDITIONS_UP = 21;
    static const int NUM_CONDITIONS_AOHD4 = 21;
    static const int NUM_CONDITIONS_AOF4 = 21;
    static const int NUM_CONDITIONS_SWGB = 22;
    static const int NUM_CONDITIONS_CC = 24;

    static const int NUM_VIRTUAL_CONDITIONS_AOK = 6;
    static const int NUM_VIRTUAL_CONDITIONS_AOC = 13;
    static const int NUM_VIRTUAL_CONDITIONS_UP = 13;
    static const int NUM_VIRTUAL_CONDITIONS_AOHD = 6;
    static const int NUM_VIRTUAL_CONDITIONS_AOF = 6;
    static const int NUM_VIRTUAL_CONDITIONS_SWGB = 6;
    static const int NUM_VIRTUAL_CONDITIONS_CC = 6;

	static const char* types_aok[NUM_CONDITIONS_AOHD4];
	static const char* types_swgb[NUM_CONDITIONS_SWGB];
	static const char* types_cc[NUM_CONDITIONS_CC];
	static const char* types_short_aok[NUM_CONDITIONS_AOHD4];
	static const char* types_short_swgb[NUM_CONDITIONS_SWGB];
	static const char* types_short_cc[NUM_CONDITIONS_CC];

	static const char* virtual_types_aok[NUM_VIRTUAL_CONDITIONS_AOK];
	static const char* virtual_types_aoc[NUM_VIRTUAL_CONDITIONS_AOC];
	static const char* virtual_types_up[NUM_VIRTUAL_CONDITIONS_UP];
	static const char* virtual_types_aohd[NUM_VIRTUAL_CONDITIONS_AOHD];
	static const char* virtual_types_aof[NUM_VIRTUAL_CONDITIONS_AOF];
	static const char* virtual_types_swgb[NUM_VIRTUAL_CONDITIONS_SWGB];
	static const char* virtual_types_cc[NUM_VIRTUAL_CONDITIONS_CC];

	static const char** types;
	static const char** types_short;
    static const char** virtual_types;

    static const int NUM_TAUNT_SETS = 64;
	static const char* taunt_set[NUM_TAUNT_SETS];

private:
	struct Genie_Condition toGenie() const;

    bool valid_player() const;
    bool valid_full_map() const;
    bool valid_partial_map() const;
    bool valid_technology_spec() const;
    bool valid_unit_spec() const;
    bool valid_area() const;
    bool valid_area_location() const;
    bool Condition::null_object() const;
    bool valid_object() const;
    bool null_location_unit() const;
    bool valid_location_unit() const;
    std::string areaName() const;

	bool valid_since_last_check;
};

#pragma pack(pop)

#endif // AOKTS_CONDITION_H
