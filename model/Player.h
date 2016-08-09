/*
	Player.h: declares a class representing a scenario player

	MODEL
*/

#ifndef PLAYER_H
#define PLAYER_H

#include "datatypes.h"
#include "Unit.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>

#include <windows.h>	//for bitmaps, POINT, and RECT

#pragma pack(push, 1)

/*	Unfortunately, the player classes have to know something about their indices
	because of the scenario format. */
#define NUM_PLAYERS	16 // or 0x10
const int GAIA_INDEX = 8; //index of GAIA players (may change)

//#define MAX_DIS_TECH	30
//#define MAX_DIS_UNIT	30
//#define MAX_DIS_BLDG	20

extern class Scenario scen;

enum AIModes
{
	AI_custom,
	AI_standard,
	AI_none
};

class Player
{
public:
	Player();

	void reset(unsigned int id);

	/**
	 * @return the index of the unit with the provided ID, or units.size() if
	 * not found.
	 */
	std::vector<Unit>::size_type find_unit(UID uid) const;
	void erase_unit_area(RECT area);
	void erase_unit_type(UCNST type);
	void erase_unit(std::vector<Unit>::size_type);

	bool read_aifiles(FILE *in);	//returns false if out-of-memory
	void clear_ai();
	void clear_cty();
	void clear_vc();

	void read_header_name(FILE *in);
	void read_header_stable(FILE *in);
	void read_data1(FILE * in);
	void read_aimode(FILE * in);
	void read_resources(FILE * in);
	void read_player_number(FILE * in);
	void read_diplomacy(FILE * in);
	void read_ndis_techs(FILE * in);
	void read_dis_techs(FILE * in, const PerVersion& pv);
	void read_ndis_units(FILE * in);
	void read_dis_units(FILE * in, const PerVersion& pv);
	void read_ndis_bldgs(FILE * in);
	void read_dis_bldgs(FILE * in, const PerVersion& pv);
	void read_age(FILE * in);
	void read_camera_longs(FILE * in);
	void read_data4(FILE * in, Game v);
	void read_units(FILE *in);

	/*
		read_data3: Reads Player Data 3.

		@param in The file to read from.
		@param view Will read the camera floats to these 2 floats if nonzero.
	*/
	void read_data3(FILE *in, float *view);

	void write_header_name(FILE * out);
	void write_header_stable(FILE * out);
	void write_data1(FILE * out);

	void write_units(FILE *out);
	void write_no_units(FILE *out);
	void write_data3(FILE *out, int me, float *view);

	bool import_ai(const char *path);
	bool export_ai(const char *path);
	bool import_vc(const char *path);
	bool export_vc(const char *path);
	bool import_cty(const char *path);
	bool export_cty(const char *path);
	void add_unit(Unit& uspec);
	void add_unit(Unit * uspec);
	void change_unit_type_for_all_of_type(UCNST from_type, UCNST to_type);

    std::string get_name();

	char name[30];	//256 bytes in file, but only allows 29 characters
	long stable;	//string table
	bool enable, human;
	long civ;

	char vc[_MAX_FNAME];
	SString vcfile; // ie, .per file contents

	char cty[_MAX_FNAME];
	SString ctyfile; // ie, .per file contents

	char ai[_MAX_FNAME];
	SString aifile; // ie, .per file contents
	char aimode;	//see enum AIModes

	long player_number;

	/* Starting resource stockpiles.

		AOK: gold, wood, food, stone, orex, unknown
		SWGB: nova, carbon, food, ore, orex, unknown
	*/
	enum ResourceID
	{
		RES_gold, RES_wood, RES_food, RES_stone, RES_orex, RES_unknown
	};
	unsigned long resources[6];

	//disables
	//long AlliedVictory;  // UNREAD
	//long dis_tech[MAX_DIS_TECH], dis_unit[MAX_DIS_UNIT], dis_bldg[MAX_DIS_BLDG];
	long dis_tech[60], dis_unit[60], dis_bldg[60];
	long ndis_t, ndis_u, ndis_b;

	short age;	//starting age
	short ending_age;	//starting age

	float pop;	//EX-only
	float camera[2];	//initial camera position: x, y
	short u1, u2;
	char avictory;	//allied victory
	long diplomacy[NUM_PLAYERS];
	long color;
	float ucount;

	std::vector<Unit> units;

	static const char* names[NUM_PLAYERS];

	static int num_players;	//for diplomacy

private:

};

#pragma pack(pop)

#endif //SCEN_H
