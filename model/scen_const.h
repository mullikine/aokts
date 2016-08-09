/* MODEL */

#ifndef SCEN_CONST_H
#define SCEN_CONST_H

#include "esdata.h"

/* Typedefs */

typedef long UID;
typedef short UCNST;

/**
 * This is not saved in the scenario but is used internally by many data
 * reading functions.
 */
enum Game
{
	UNKNOWN = 0,
	AOE     = 1,
	ROR     = 2,
	AOK     = 3,
	AOC     = 4,
	AOHD    = 5,
	AOF     = 6,
	SWGB    = 7,
	SWGBCC  = 8,
	UP      = 9, // UserPatch
	AOHD4   = 10,
	AOF4    = 11,
	AOHD6   = 12,
	AOF6    = 13,
	NOCONV  = 100
};

enum HeaderType
{
	HT_SC           = 2, // SCN, SCX, SC1, SCX2
	HT_AOE2SCENARIO = 3  // African Kingdoms header describing dependencies
};

struct Dataset {  // AoHD scx2 or aoe2scenario only
    enum Value {
        AOK_xUnk0 = 0,
        AOC_xUnk1 = 1,
        Unk_xAOK  = 2,
        Unk_xAOC  = 3,
        Unk_xAOF  = 4,
        Unk_xAOAK = 5
    };
};

const size_t  NUM_DATASETS = 4;

enum ScenVersion1
{
	SV1_UNKNOWN	    = 0,
	SV1_AOE1		= 10,
	SV1_AOK		    = 18,
	SV1_AOC_SWGB	= 21
};

enum ScenVersion2
{
	SV2_UNKNOWN          = 0,
	SV2_AOE1		     = 15,
	SV2_AOK		         = 18,
	SV2_AOC_SWGB	     = 22,
	SV2_AOHD_AOF	     = 23,// HD / The Forgotten
	SV2_AOHD_AOF4        = 24,// HD v4 / The Forgotten v4
	SV2_AOHD_AOF6        = 26,// HD v6 / The Forgotten v6
	SV2_SWGBCC           = 30 // SWGB:CC
};

/**
 * This is not saved in the scenario, but is used internally by AOKTS to
 * encapsulate differences in versions of the scenario format.
 */
struct PerVersion
{
	int messages_count;
	bool mstrings;
	int max_disables1; // max disable tech and unit
	int max_disables2; // max disable buildings
};

struct PerGame
{
	UCNST max_unit; // max unit types
	int max_research; // max research
	int max_tech; // max tech
	int max_terrains; // max tech
	int max_condition_types;
	int max_effect_types;
	int max_virtual_condition_types;
	int max_virtual_effect_types;
};

/* Internal Constants */

const double PI = 3.1415927;	//for radians

enum AOKTS_ERROR
{
	ERR_unknown = -12,	//unknown error
	ERR_combination = -11,	//bad combination
	ERR_outofbounds = -10,	//something went out of bounds, depends on context
	ERR_overlap = -9,	//used for map copy, target overlaps source
	ERR_data = -8,		//decompressed data error
	ERR_compr = -7,		//zlib negative return
	ERR_mem = -6,		//allocation failure (usually a bug)
	ERR_zver = -5,		//zlib DLL mismatch
	ERR_digit = -4,		//my mistake
	ERR_bitmap = -3,	//bitmap incomplete
	ERR_noscen = -2,	//not a scenario
	ERR_sver = -1,		//unrecognized scenario version
	ERR_none = 0,		//the only good error is no error
};

/** External constants **/

enum Diplomacy
{
	DIP_ally, DIP_neutral, DIP_unknown, DIP_enemy
};

extern size_t MapSizes[8]; //conversion from standard sizes to tiles (see scen.cpp for values)
extern size_t Elevations[9]; //conversion from standard sizes to tiles (see scen.cpp for values)

const size_t N_PLAYERS = 8;

const size_t NUM_STYPES   = 4;   // number of stockpile resource types
const size_t NUM_CINEM	  = 4;
const size_t MAX_TRIGNAME = 256; // before African Kingdoms this was 128 I think. Has it been changed again for unicode?
const size_t MAX_TRIGNAME_SAFE_AOC = 127;
const size_t MAX_TRIGNAME_SAFE_HD = 126;

const size_t NUM_MSGS	  = 6;
extern const char* message_names[NUM_MSGS];
extern const char* cinem_names[NUM_CINEM];

const size_t NUM_DIFFICULTIES = 5;
extern const char* difficulties_18[NUM_DIFFICULTIES];
extern const char* difficulties_21[NUM_DIFFICULTIES];

const size_t  NUM_AGES = 6;
extern const struct PAIR ages[NUM_AGES];

const size_t  NUM_AGES_HD6 = 8;
extern const struct PAIR ages_hd6[NUM_AGES_HD6];

const size_t  NUM_GROUPS = 63;
extern const struct PAIR groups[NUM_GROUPS];

const size_t  NUM_UTYPES = 7;
extern const struct PAIR utypes[NUM_UTYPES];

bool isHD(Game game);
bool isAOE2(Game game);

#endif //SCEN_CONST_H
