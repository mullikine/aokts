/*
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION.

	campaign.h -- wraps a campaign file in class Campaign and defines the interface.

	Notes:
	This file's code is platform-independant albeit windows.h is included for the bitmap structs.
	'u' stands for an unknown member. (There are a lot of these.)

	MODEL?
*/

#ifndef CAMPAIGN_H
#define CAMPAIGN_H

#include "scen.h"

//MAC USERS: Please replace this block of code with whatever you need.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>	//for bitmaps, POINT, and RECT
//#include <math.h> // for abs

#include <stdlib.h>	//for _MAX_FNAME and the like.
#include <time.h>		//for time_t typedef.

typedef struct _iobuf FILE;	//makes including <stdio.h> unnecessary

/* Options */

/** Structs **/

struct AOKFile
{
	char name[_MAX_FNAME];
	SString data;
};

/* Map */

#pragma pack(4)

/* The Campaign Wrapper */

class Campaign
{
public:
	Campaign();
	~Campaign();
//	Un-compressed Header
	struct _header
	{
		char version[5];		//Version string (+ 1 for NULL)
	} header;

	void reset();
	void read(FILE *in);
	void write(FILE *out);
};

#pragma pack(pop)	//restore default packing alignment

#endif //CAMPAIGN_H
