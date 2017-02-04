/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION.

	settings.h: Global settings shared across source files.

	VIEW/CONTROLLER
**/

#include <stdlib.h>	//for _MAX_FNAME and the like.

#define MAX_UGROUPS	30 /* Maximum number of user-defined unit groups */
#define MAX_UGNAME	20 /* Maximum length of user-defined group name */
#define MAX_RECENT	4


/*
	Recent Files: These are set up somewhat like a linked
	list, but with a global pointer to the first item,
	then just a next pointer for each item. The last one,
	of course, will have a value of NULL.
*/

struct RecentFile
{
	char path[_MAX_PATH];
	char display[_MAX_FNAME];
	int game;

	struct RecentFile *next;	//kinda a linked list?
};

enum WarningBits
{
	WARN_WEIRDRESOURCE = 0x01
};

/* Various configuration settings */
extern class Setts
{
	void read_recent(const char *path);
	void write_recent(const char *path);

	unsigned recent_used;
	struct RecentFile recent[MAX_RECENT];

	char path[_MAX_PATH];	//Path to INI

public:
	Setts();
	~Setts();
	bool load();
	struct RecentFile *recent_getnext();
	void recent_push(RecentFile *target);

	//Not in ini
	char BasePath[_MAX_PATH];	//Path to the AOK directory
	char ScenPath[_MAX_PATH];	//Path to the currently open scenario

	//[Decompressed]
	char TempPath[_MAX_PATH];	//Path to the decompressed temp file
	bool DelTempOnExit;			//Delete the above file on exit?

	//[Warnings]
	char warnbits;

	//[Log]
	char logname[12];
	bool intense;
	bool editall;
	bool asktoconverteffects;
	bool displayhints;
	bool disabletips;
	bool forceenabletips;
	bool showtrignames;
	bool showtrigfunction;
	bool showdisplayorder;
	bool showtrigids;
	bool nowarnings;
	bool drawconds;
	bool draweffects;
	bool drawlocations;
	bool drawterrain;
	bool drawelevation;
	char drawplayer[9];

	//[Minimap]
	char zoom;

	//[Recent#]
	struct RecentFile *recent_first;

} setts;

namespace global
{
   extern char exedir[_MAX_PATH];
}
