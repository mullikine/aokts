/*
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION.

	scen.h -- wraps a scenario file in class Scenario and defines the interface.

	Notes:
	This file's code is platform-independant albeit windows.h is included for the bitmap structs.
	'u' stands for an unknown member. (There are a lot of these.)

	MODEL?
*/

#ifndef SCEN_H
#define SCEN_H

#include "datatypes.h"
#include "trigger.h"
#include "Player.h"

//MAC USERS: Please replace this block of code with whatever you need.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>	//for bitmaps, POINT, and RECT
//#include <math.h> // for abs

#include <stdlib.h>	//for _MAX_FNAME and the like.
#include <time.h>		//for time_t typedef.

typedef struct _iobuf FILE;	//makes including <stdio.h> unnecessary

/* Options */

#define PLAYER1_INDEX	0	//index of Player 1 (may change)
#define MAX_MAPSIZE	480
#define MAX_MAPSIZE_OLD	255

#define UNREAD(n)

/** Structs **/

struct AOKFile
{
	char name[_MAX_FNAME];
	SString data;
};

struct TerrainFlags {
    enum Value{
        NONE                           = 0x00,
        FORCE                          = 0x01,
        EIGHT                          = 0x02,
    };
};

struct OpFlags {
    enum Value{
        TERRAIN                        = 0x01,
        UNITS                          = 0x02,
        ELEVATION                      = 0x04,
        TRIGGERS                       = 0x08,
        RANDOMIZE                      = 0x16,
        ALL                            = 0x01 | 0x02 | 0x04 | 0x08
    };
};

struct SaveFlags {
    enum Value{
        NONE                           = 0x00,
        CONVERT_EFFECTS                = 0x01,
    };
};

/* Map */

#pragma pack(push, 1)	//set packing alignment to work with terrain array

class Map	//wraps all data regarding the scenario map
{
public:
//in scenario

	long aitype;	//Map type for AI recognition. (see aitypes[])
	unsigned long x, y;	//Should always be equal, but are stored separately in the file.

    // AOHD4 and AOF4 and upwards
	unsigned long unknown1;
	unsigned long unknown2;
	unsigned long unknown3;
	unsigned long unknown4;

	struct Terrain		//This is one tile of terrain.
	{
		Terrain();		//sets cnst and elev to default values.

		unsigned char cnst;	//terrain tile constant. (see esdata)
		unsigned char elev;	//terrain elevation. (0-8, I believe)

	} terrain[MAX_MAPSIZE][MAX_MAPSIZE];	//the terrain tile arrays

//internal

	Map();

	void reset();
	void read(FILE *in, ScenVersion1 v);
	void write(FILE *out, ScenVersion1 v);

	/*	readArea: copies terrain data from a buffer to the specified area */
	bool readArea(Buffer &from, const RECT &area);

	/*	writes a specified area of the map to a buffer

		area: inclusive area to copy

		returns: true if successful
	*/
	bool writeArea(Buffer &b, const RECT &area);
    bool scaleArea(const RECT &area, const float scale);
};

#pragma pack(4)

struct Victory
{
	enum Modes
	{
		MODE_Standard, MODE_Conquest, MODE_Score, MODE_Time, MODE_Custom
	};

	long conq, z1, relics, z2, expl, z3;
	long all, mode, score, time;	//really bools, just stored as longs in-file
};

/*	MapCopyCache: caches data in-between Scenario::map_size() and Scenario::
	map_copy() so we don't have to do some loops twice. */
class MapCopyCache;

/* The Scenario Wrapper */

class Scenario
{
	static const long sect = 0xFFFFFF9D;

// Private structs
	struct AOKBMP
	{
		BITMAPFILEHEADER file_hdr;
		BITMAPINFOHEADER info_hdr;
		RGBQUAD *colors;
		char *image;

		AOKBMP();
		~AOKBMP();

		void read(FILE *in);
		void write(FILE *out);
		bool toFile(const char *path);
		void reset();
	};

//	Internal Status Stuff (aka not in the SCX)
	bool mod_status;

    void next_sect(FILE * in);

	/**
	 * Reads decompressed scenario data from specified path.
	 */
	void read_data(const char *path);

	/**
	 * Writes to-be-compressed scenario data to the specified path.
	 */
	int write_data(const char *path);

    bool isTerrainEdge(unsigned char cnst, unsigned char newcnst, unsigned char oldcnst);

public:
	const PerVersion *perversion;	//yes, I know that's a word. :-P
	const PerGame *pergame;

	static const PerVersion pv1_15;
	static const PerVersion pv1_18;
	static const PerVersion pv1_21;
	static const PerVersion pv1_22;
	static const PerVersion pv1_23;
	static const PerVersion pv1_24;
	static const PerVersion pv1_26;
	static const PerVersion pv1_30;

	PerGame pgGame[10];

	static const PerGame pgAOE;
	static const PerGame pgAOK;
	static const PerGame pgAOC;
	static const PerGame pgUP;
	static const PerGame pgAOHD;
	static const PerGame pgAOF;
	static const PerGame pgAOHD4;
	static const PerGame pgAOF4;
	static const PerGame pgAOHD6;
	static const PerGame pgAOF6;
	static const PerGame pgSWGB;
	static const PerGame pgSWGBCC;

	Scenario();
	~Scenario();
	static char StandardAI[];
	static char StandardAI2[];

	char msg[_MAX_FNAME + 100];	//contains any error message reported by a function.

	/* The scenario */

	// Internal use, this is what we determine the scx type to be
	Game game;
	// Internal use, set according to header::version[]
	ScenVersion1 ver1;
	// Internal use, set according to version2
	ScenVersion2 ver2;

//	Un-compressed Header
	struct _header
	{
		char version[5];		//Version string (+ 1 for NULL)
		long length;			//length of header (after this point)
		HeaderType header_type;
		unsigned long instruct_len;  	//Instructions string length
		unsigned long n_players;
		unsigned long uses_expansions; // not sure about this
		unsigned long n_datasets;
		unsigned long datasets[NUM_DATASETS];  // the number of elements read and saved is given by n_datasets. 5 is an arbitrary number
		__time32_t timestamp;	//last save
		//left out instructions in header
		UNREAD(long pcount);

		void reset();
		bool read(FILE *scx);	//error return
		void write(FILE *scx, const SString *instr, long players, Game g);
	    bool expansionsRequired();
	    bool datasetRequired(Dataset::Value d);
	    void requireDataset(Dataset::Value d);
	    void unrequireDataset(Dataset::Value d);
	} header;

//	Compression starts here
	UID	next_uid;
	float	version2;
	long	unknown;	//usually 1?
	char	origname[_MAX_FNAME];
	long	mstrings[NUM_MSGS];
	SString messages[NUM_MSGS];
	char	cinem[NUM_CINEM][0x40];	//pre-game, victory, lose, bkg 0x20 = 32
	long	bBitmap;
	AOKBMP bitmap;
	Player players[NUM_PLAYERS];
	Victory vict;
	char lock_teams;
	char player_choose_teams;
	char random_start_points;
	char max_teams;
	long all_techs;
	Map map;
	float	editor_pos[2];
	double	trigver;
	char    objstate;
	std::vector<Trigger> triggers;
	std::vector<unsigned long> t_order; // index = trigger display order, value = trigger id

	long unk3; // TODO: rename
	long unk4;
	long cFiles;
	AOKFile *files;

    // open the file and return the actual scx game version
	Game open(const char *path, const char *dpath, Game version);
	int save(const char *path, const char *dpath, bool write, Game convert, SaveFlags::Value flags);
	void auto_upgrade_hd4();
	void auto_upgrade_hd6();
	void clean_format();
	void adapt_game();
	void reset();
	bool export_bmp(const char *path);
	//exFile: Exports all (index of -1) or one file to the specified directory
	bool exFile(const char *directory, long index);

	bool is_userpatch();

    void fix_t_order();
	/*
		clean_triggers: cleans trigger list of unused triggers

		Note: Necessary because the UI only deletes the t_order entry, not the trigger itself.
	*/
	void clean_triggers();

	/*
		insert_trigger: Inserts a copy of a trigger into the trigger list after
		a specified trigger.

		@param t The trigger to copy.
		@param after The index of the preceding trigger.

		Returns the index of the inserted trigger in the trigger vector.

		Note: This both appends the trigger to the trigger list and then
		inserts it into t_order.
	*/
	size_t insert_trigger(Trigger *t, size_t after);

	/**
	 * Applies a TriggerVisitor to all triggers in the scenario.
	 */
	void accept(TriggerVisitor&);

    static const unsigned char TEMPTERRAIN = (unsigned char)(-1);
    static const unsigned char TEMPTERRAIN2 = (unsigned char)(-3);
    static const unsigned char OUTOFBOUNDS = (unsigned char)(-2);
    void swapTerrain(unsigned char newcnst, unsigned char oldcnst);
    void outline(unsigned long x, unsigned long y, unsigned char newcnst, unsigned char oldcnst, TerrainFlags::Value flags=TerrainFlags::NONE);
    unsigned char outlineDraw(unsigned long x, unsigned long y, unsigned char newcnst, unsigned char oldcnst, TerrainFlags::Value flags=TerrainFlags::NONE);
    void floodFill4(unsigned long x, unsigned long y, unsigned char newcnst, unsigned char oldcnst);
    void floodFillElev4(unsigned long x, unsigned long y, unsigned char newelev, unsigned char cnst);

	/*	map_size: returns size of memory needed to copy rectangle.

		WARNING: This function assumes source is a valid rectangle, ie. (top < bottom &&
		left < right).

		We need to do this since the app has to allocate the memory using Win API
		functions for clipboard.

		Note: The returned MapCopyCache object is deleted in map_copy().
	*/
	int map_size(const RECT &source, MapCopyCache *&mcc);

	AOKTS_ERROR remove_trigger_names();
	AOKTS_ERROR remove_trigger_descriptions();
	AOKTS_ERROR save_pseudonyms();
	AOKTS_ERROR prefix_display_order();
	AOKTS_ERROR remove_display_order_prefix();
	AOKTS_ERROR swap_trigger_names_descriptions();
	AOKTS_ERROR up_to_hd();
	AOKTS_ERROR aoc_to_hd4();
	AOKTS_ERROR aoc_to_hd6();
	AOKTS_ERROR hd4_to_hd6();
	AOKTS_ERROR aohd_to_hd6();
	AOKTS_ERROR up_to_aofe();
	AOKTS_ERROR up_to_10c();
	AOKTS_ERROR aoc_to_aok();
	AOKTS_ERROR aok_to_aoc();
    AOKTS_ERROR strip_patch4();
    AOKTS_ERROR strip_patch6();
	AOKTS_ERROR hd_to_up();
	AOKTS_ERROR hd_to_10c();
	AOKTS_ERROR hd_to_swgb();
	AOKTS_ERROR up_to_swgb();
	AOKTS_ERROR fix_trigger_outliers();

	/*	map_copy: copies terrain and units from a specified area of the map
		to a Buffer.

		Returns: Error code

		Note: MapCopyCache is deleted by this function.
	*/
	AOKTS_ERROR map_copy(Buffer &to, const MapCopyCache *mcc);

    AOKTS_ERROR delete_player_units(int pindex);

	/*	map_paste: "pastes" terrain and units from a buffer to a specified point.

		Note: The buffer should have been filled by map_copy.
	*/
	AOKTS_ERROR map_paste(const POINT &to, Buffer &from);

    AOKTS_ERROR compress_unit_ids();
    AOKTS_ERROR sort_conds_effects();
    AOKTS_ERROR instructions_sound_id_set(long value);
    AOKTS_ERROR instructions_sound_text_set();
    AOKTS_ERROR instructions_string_zero();
    AOKTS_ERROR instructions_string_reset();
    AOKTS_ERROR instructions_panel_set(long value);
    AOKTS_ERROR map_change_elevation(const RECT &target, int adjustment);
    AOKTS_ERROR map_repeat(const RECT &target, const POINT &source, OpFlags::Value flags=OpFlags::ALL);
    AOKTS_ERROR map_delete(const RECT &from, const POINT &to, OpFlags::Value flags=OpFlags::ALL);
	/*  map_move: moves the units, terrain and triggers in that terrain */
    AOKTS_ERROR map_move(const RECT &from, const POINT &to, OpFlags::Value flags=OpFlags::ALL);
    AOKTS_ERROR map_swap(const RECT &from, const POINT &to, OpFlags::Value flags=OpFlags::ALL);
    AOKTS_ERROR map_scale(const RECT &area, const float scale);
    AOKTS_ERROR water_cliffs_visibility(const bool visibility);
    int get_number_active_players();
    AOKTS_ERROR set_number_active_players(int num);
    AOKTS_ERROR set_unit_z_to_map_elev();
    AOKTS_ERROR change_unit_type_for_all_of_type(UCNST from_type, UCNST to_type);
    AOKTS_ERROR clearaicpvc();
    AOKTS_ERROR randomize_unit_frames();
    AOKTS_ERROR randomize_unit_frames(const unsigned int cnst);
    AOKTS_ERROR map_duplicate(const RECT &from, const POINT &to, OpFlags::Value flags=OpFlags::ALL);

    AOKTS_ERROR add_activation(size_t start, size_t end, size_t to);
    AOKTS_ERROR move_triggers(size_t start, size_t end, size_t to);
    AOKTS_ERROR swap_players(int a, int b);
    AOKTS_ERROR delete_triggers(size_t start, size_t end);
    AOKTS_ERROR duplicate_triggers(size_t start, size_t end, size_t to);
    AOKTS_ERROR swap_triggers(long id_a, long id_b);
    AOKTS_ERROR sync_triggers();

	int getPlayerCount();

	bool needsave()	{ return mod_status; }
};

#undef UNREAD

#pragma pack(pop)	//restore default packing alignment

#endif //SCEN_H
