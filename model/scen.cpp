/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	scen.cpp -- Scenario wrapper code

	MODEL?
**/

#include "scen.h"

#include "../util/zlibfile.h"
#include "../view/utilui.h"
#include "../util/utilio.h"
#include "../util/cpp11compat.h"
#include "../util/settings.h"
#include "../util/Buffer.h"
#include "../util/helper.h"

#include <direct.h>
#include <string.h>
#include <math.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>

const char datapath_aok[] = "data_aok.xml";
const char datapath_swgb[] = "data_swgb.xml";

using std::vector;
using std::pair;

/* some options */
#define BMP_READ_HACK

/*  PerVersion
 *  See scen_const.h
 *
 *  struct PerVersion {
 *  	int messages_count;
 *  	bool mstrings;
 *  	int max_disables1; // max disable tech and unit
 *  	int max_disables2; // max disable buildings
 *  };
 */

/* PerVersions */
const PerVersion Scenario::pv1_15 = { // AOE
	5,
	true,
	30,
	20,
};

const PerVersion Scenario::pv1_18 = { // AOK
	5,
	true,
	30,
	20,
};

const PerVersion Scenario::pv1_22 = // AOC / SWGB
{
	6,
	true,
	30,
	20,
};

const PerVersion Scenario::pv1_30 = // SWGB CC
{
	6,
	true,
	60,
	60,
};

const PerVersion Scenario::pv1_23 = // AOHD / AOF
{
	6,
	true,
	30,
	20,
};

const PerVersion Scenario::pv1_24 = // AOHDv4 / AOFv4
{
	6,
	true,
	30,
	20,
};

const PerVersion Scenario::pv1_26 = // AOHDv6 / AOFv6
{
	6,
	true,
	30,
	30,
};

/*  PerGames
 *  See scen_const.h
 *
 *  struct PerGame  {
 *      UCNST max_unit; // max unit types
 *      int max_research; // max research
 *      int max_tech; // max tech
 *      int max_terrains; // max terrains
 *      int max_condition_types;
 *      int max_effect_types;
 *      int max_virtual_condition_types;
 *      int max_virtual_effect_types;
 *  };
 */

const PerGame Scenario::pgAOE =
{
	374,
	118,
	140,
	23, // not including 9 undefined
	0,
	0,
	0,
	0,
};

const PerGame Scenario::pgAOK =
{
	750,
	426,
	438,
	32,
	20,
	24,
	5,
	2
};

const PerGame Scenario::pgAOC =
{
	866,
	460,
	514,
	42, // including 1 undefined
	20,
	30,
	8,
	4,
};

const PerGame Scenario::pgUP =
{
	866,
	460,
	514,
	42, // including 1 undefined
	20,
	34,
	8,
	28,
};

const PerGame Scenario::pgSWGB =
{
	750,
	426,
	438,
	51, // not including 4 undefined
	22,
	37,
	5,
	4,
};

const PerGame Scenario::pgSWGBCC =
{
	866,
	460,
	514,
	53,  // not including 2 undefined
	24,
	39,
	0,
	4,
};

const PerGame Scenario::pgAOHD =
{
	865,
	459,
	513,
	42,  // including 1 undefined
	21,
	37,
	5,
	4,
};

const PerGame Scenario::pgAOF = // these are not correct
{
	865,
	459,
	513,
	41,  // including 1 undefined
	21,
	37,
	5,
	4,
};

const PerGame Scenario::pgAOHD4 =
{
	865,
	459,
	513,
	100,  // including 1 undefined
	21,
	37,
	5,
	4,
};

const PerGame Scenario::pgAOF4 =
{
	865,
	459,
	513,
	100,  // including 1 undefined
	21,
	37,
	5,
	4,
};

const PerGame Scenario::pgAOHD6 =
{
	865,
	459,
	513,
	100,  // including 1 undefined
	21,
	37,
	5,
	4,
};

const PerGame Scenario::pgAOF6 =
{
	865,
	459,
	513,
	100,  // including 1 undefined
	21,
	37,
	5,
	4,
};

/* The Scenario */

#define SMSG	" For stability\'s sake, I must discontinue reading the scenario." \
				"\nPlease contact cyan.spam@gmail.com regarding this error."

const char e_none[]	= "Scenario operation completed successfully.";
const char e_compr[]	= "Zlib compression/decompression failed.";
const char e_mem[]	= "I ran out of memory. This probably means "\
	"that the scenario is invalid or that I got out of sync.";
const char e_zver[]	= "Your version of zlib1.dll is incompatible. I need at least %s. ";
const char e_digit[]	= "Oops, my programmer (David Tombs) made an error.\nProblem: %s" SMSG;
const char e_bitmap[]	= "Sorry, the bitmap data in this scenario appears incomplete." SMSG;

inline int truncate(float x)
{
	return (int)x;
}

inline int myround(float n)
{
	return int((n >= 0.0) ? n + 0.5 : n - 0.5);
}

Scenario::Scenario()
:	mod_status(false),
	next_uid(0), bBitmap(0), files(NULL)
{
//	strcpy(mark, "DGT");
	strcpy(players[GAIA_INDEX].name, "GAIA");
}

Scenario::~Scenario()
{
	delete [] files;
}

void Scenario::reset()
{
	int i;

	/* Internal */
	mod_status = false;

	game = AOC;
    // Hint about whether to open as AOC or SGWB
	if (setts.recent_first) {
	    game = (Game)setts.recent_first->game;
	}
	adapt_game();

	next_uid = 0;
	memset(origname, 0, sizeof(origname));
	strcpy(origname, "Trigger Studio 1.2.0"); // is this needed?

	for (i = 0; i < 6; i++)
	{
		messages[i].erase();
		mstrings[i] = -1;
	}

	memset(cinem, 0, sizeof(cinem));
	if ( game == AOHD6 || game == AOF6 )
	    bBitmap = 3;
	else
	    bBitmap = 0;

    set_number_active_players(8);
	lock_teams = 0;
	player_choose_teams = 1;
	random_start_points = 0;
	max_teams = 4;
	for (i = 0; i < NUM_PLAYERS; i++)
		players[i].reset(i);
	players[0].enable = true;
	players[0].human = true;
	Player::num_players = EC_NUM_PLAYERS;

	memset(&vict, 0, sizeof(vict));
	map.reset();

    // do not reset trigger system version
	//trigver = 1.6;
	objstate = 0;
	triggers.clear();
	t_order.clear();

	files = NULL;

	memset(msg, 0, sizeof(msg));
}

Scenario::AOKBMP::AOKBMP()
:	colors(NULL), image(NULL)
{
	//prep fileheader
	file_hdr.bfType = 0x4D42;	//"MB"
	file_hdr.bfReserved1 = 0;
	file_hdr.bfReserved2 = 0;

	/* these are set later when we read the BMP from the scenario */
	file_hdr.bfSize = 0;
	file_hdr.bfOffBits = 0;
}

Scenario::AOKBMP::~AOKBMP()
{
	delete [] colors;
	delete [] image;
}

void Scenario::AOKBMP::read(FILE *in)
{
	readbin(in, &info_hdr);

#ifdef BMP_READ_HACK
	if (info_hdr.biClrUsed == 0)
		info_hdr.biClrUsed = 256;
	info_hdr.biSizeImage = info_hdr.biWidth * info_hdr.biHeight;
#endif

	file_hdr.bfOffBits = (sizeof(BITMAPFILEHEADER) + info_hdr.biSize + info_hdr.biClrUsed * sizeof(RGBQUAD));
	file_hdr.bfSize = (file_hdr.bfOffBits + info_hdr.biSizeImage);

	colors = new RGBQUAD[info_hdr.biClrUsed];
	fread(colors, sizeof(RGBQUAD), info_hdr.biClrUsed, in);

	image = new char[info_hdr.biSizeImage];
	fread(image, sizeof(char), info_hdr.biSizeImage, in);
}

void Scenario::AOKBMP::write(FILE *out)
{
	fwrite(&info_hdr, sizeof(info_hdr), 1, out);
	fwrite(colors, sizeof(RGBQUAD), info_hdr.biClrUsed, out);
	fwrite(image, sizeof(char), info_hdr.biSizeImage, out);
}

bool Scenario::AOKBMP::toFile(const char *path)
{
	FILE *bmp_out;

	if (!file_hdr.bfSize)
		return false;

	bmp_out = fopen(path, "wb");
	if (!bmp_out)
		return false;

	fwrite(&file_hdr, sizeof(BITMAPFILEHEADER), 1, bmp_out);
	fwrite(&info_hdr, sizeof(BITMAPINFOHEADER), 1, bmp_out);
	fwrite(colors, sizeof(RGBQUAD), info_hdr.biClrUsed, bmp_out);
	fwrite(image, info_hdr.biSizeImage, 1, bmp_out);

	fclose(bmp_out);
	return true;
}

void Scenario::AOKBMP::reset()
{
	file_hdr.bfSize = 0;
	delete [] colors;
	delete [] image;
}

char Scenario::StandardAI[] = "RandomGame";
char Scenario::StandardAI2[] = "Promisory";

void Scenario::adapt_game() {
	switch (game) {
	case AOE:
	    strcpy(header.version, "0.00"); // is this needed?
        ver1 = SV1_AOE1;
	    ver2 = SV2_AOE1;
	    version2 = 0.00F;
	    trigver=0.0;
	    perversion = &pv1_15;
	    pergame = &pgAOE;
        Condition::types = Condition::types_aok;
        Condition::types_short = Condition::types_short_aok;
        Condition::virtual_types = Condition::virtual_types_aoc;
        Effect::types = Effect::types_aoc;
        Effect::types_short = Effect::types_short_aoc;
        Effect::virtual_types = Effect::virtual_types_aoc;
	    break;
    case AOK:
	    strcpy(header.version, "1.18"); // is this needed?
        ver1 = SV1_AOK;
	    ver2 = SV2_AOK;
	    version2 = 1.20F;
	    trigver=1.6;
		perversion = &pv1_18;
	    pergame = &pgAOK;
        Condition::types = Condition::types_aok;
        Condition::types_short = Condition::types_short_aok;
        Condition::virtual_types = Condition::virtual_types_aok;
        Effect::types = Effect::types_aok;
        Effect::types_short = Effect::types_short_aok;
        Effect::virtual_types = Effect::virtual_types_aok;
        break;
    case AOC:
	    strcpy(header.version, "1.21"); // is this needed?
	    ver1 = SV1_AOC_SWGB;
	    ver2 = SV2_AOC_SWGB;
	    version2 = 1.22F;
	    trigver=1.6;
		perversion = &pv1_22;
	    pergame = &pgAOC;
        Condition::types = Condition::types_aok;
        Condition::types_short = Condition::types_short_aok;
        Condition::virtual_types = Condition::virtual_types_aoc;
        Effect::types = Effect::types_aoc;
        Effect::types_short = Effect::types_short_aoc;
        Effect::virtual_types = Effect::virtual_types_aoc;
        break;
    case UP:
	    strcpy(header.version, "1.21"); // is this needed?
	    ver1 = SV1_AOC_SWGB;
	    ver2 = SV2_AOC_SWGB;
	    version2 = 1.22F;
	    trigver=1.6;
		perversion = &pv1_22;
		pergame = &pgUP;
        Condition::types = Condition::types_aok;
        Condition::types_short = Condition::types_short_aok;
        Condition::virtual_types = Condition::virtual_types_up;
        Effect::types = Effect::types_up;
        Effect::types_short = Effect::types_short_up;
        Effect::virtual_types = Effect::virtual_types_up;
        break;
    case AOF:
	    strcpy(header.version, "1.21"); // is this needed?
	    ver1 = SV1_AOC_SWGB;
	    ver2 = SV2_AOHD_AOF;
	    version2 = 1.23F;
	    trigver=1.6;
		perversion = &pv1_23;
	    pergame = &pgAOF;
        Condition::types = Condition::types_aok;
        Condition::types_short = Condition::types_short_aok;
        Condition::virtual_types = Condition::virtual_types_aof;
        Effect::types = Effect::types_aof;
        Effect::types_short = Effect::types_short_aof;
        Effect::virtual_types = Effect::virtual_types_aof;
        break;
    case AOHD:
	    strcpy(header.version, "1.21"); // is this needed?
	    ver1 = SV1_AOC_SWGB;
	    ver2 = SV2_AOHD_AOF;
	    version2 = 1.23F;
	    trigver=1.6;
		perversion = &pv1_23;
	    pergame = &pgAOHD;
        Condition::types = Condition::types_aok;
        Condition::types_short = Condition::types_short_aok;
        Condition::virtual_types = Condition::virtual_types_aohd;
        Effect::types = Effect::types_aohd;
        Effect::types_short = Effect::types_short_aohd;
        Effect::virtual_types = Effect::virtual_types_aohd;
        break;
    case AOF4:
	    strcpy(header.version, "1.21"); // is this needed?
	    ver1 = SV1_AOC_SWGB;
	    ver2 = SV2_AOHD_AOF4;
	    version2 = 1.24F;
	    trigver=1.6;
		perversion = &pv1_24;
	    pergame = &pgAOF4;
        Condition::types = Condition::types_aok;
        Condition::types_short = Condition::types_short_aok;
        Condition::virtual_types = Condition::virtual_types_aof;
        Effect::types = Effect::types_aof;
        Effect::types_short = Effect::types_short_aof;
        Effect::virtual_types = Effect::virtual_types_aof;
        break;
    case AOHD4:
	    strcpy(header.version, "1.21"); // is this needed?
	    ver1 = SV1_AOC_SWGB;
	    ver2 = SV2_AOHD_AOF4;
	    version2 = 1.24F;
	    trigver=1.6;
		perversion = &pv1_24;
	    pergame = &pgAOHD4;
        Condition::types = Condition::types_aok;
        Condition::types_short = Condition::types_short_aok;
        Condition::virtual_types = Condition::virtual_types_aohd;
        Effect::types = Effect::types_aohd;
        Effect::types_short = Effect::types_short_aohd;
        Effect::virtual_types = Effect::virtual_types_aohd;
        break;
    case AOF6:
	    strcpy(header.version, "1.21"); // is this needed?
	    ver1 = SV1_AOC_SWGB;
	    ver2 = SV2_AOHD_AOF6;
	    version2 = 1.26F;
	    trigver=1.6;
		perversion = &pv1_26;
	    pergame = &pgAOF6;
        Condition::types = Condition::types_aok;
        Condition::types_short = Condition::types_short_aok;
        Condition::virtual_types = Condition::virtual_types_aof;
        Effect::types = Effect::types_aof;
        Effect::types_short = Effect::types_short_aof;
        Effect::virtual_types = Effect::virtual_types_aof;
        break;
    case AOHD6:
	    strcpy(header.version, "1.21"); // is this needed?
	    ver1 = SV1_AOC_SWGB;
	    ver2 = SV2_AOHD_AOF6;
	    version2 = 1.26F;
	    trigver=1.6;
		perversion = &pv1_26;
	    pergame = &pgAOHD6;
        Condition::types = Condition::types_aok;
        Condition::types_short = Condition::types_short_aok;
        Condition::virtual_types = Condition::virtual_types_aohd;
        Effect::types = Effect::types_aohd;
        Effect::types_short = Effect::types_short_aohd;
        Effect::virtual_types = Effect::virtual_types_aohd;
        break;
    case SWGB:
	    strcpy(header.version, "1.21"); // is this needed?
	    ver1 = SV1_AOC_SWGB;
	    ver2 = SV2_AOC_SWGB;
	    version2 = 1.22F;
	    trigver=1.6;
		perversion = &pv1_22;
	    pergame = &pgSWGB;
        Condition::types = Condition::types_swgb;
        Condition::types_short = Condition::types_short_swgb;
        Condition::virtual_types = Condition::virtual_types_swgb;
        Effect::types = Effect::types_swgb;
        Effect::types_short = Effect::types_short_swgb;
        Effect::virtual_types = Effect::virtual_types_swgb;
        break;
    case SWGBCC:
	    strcpy(header.version, "1.21"); // is this needed?
	    ver1 = SV1_AOC_SWGB;
	    ver2 = SV2_SWGBCC;
	    version2 = 1.30F;
	    trigver=1.6;
		perversion = &pv1_30;
	    pergame = &pgSWGBCC;
        Condition::types = Condition::types_cc;
        Condition::types_short = Condition::types_short_cc;
        Condition::virtual_types = Condition::virtual_types_cc;
        Effect::types = Effect::types_cc;
        Effect::types_short = Effect::types_short_cc;
        Effect::virtual_types = Effect::virtual_types_cc;
        break;
    default:
        game = AOC;
        adapt_game();
        // We want some of the defaults from AOC, but this is an invalid
        // scenario. We aren't 'defaulting' to AOC.
		game = UNKNOWN;
		pergame = NULL;
        return;
	}
	clean_format();
    printf_log("Game is %s\n", gameName(game));
}

/* Open a scenario, read header, and read compressed data */

bool Scenario::_header::expansionsRequired()
{
    int n_expansions_required = n_datasets;
    if (uses_expansions == 1) {
        if (datasetRequired(Dataset::Unk_xAOK))
            n_expansions_required--;
        if (datasetRequired(Dataset::Unk_xAOC))
            n_expansions_required--;
    } else {
        if (datasetRequired(Dataset::AOK_xUnk0))
            n_expansions_required--;
        if (datasetRequired(Dataset::AOC_xUnk1))
            n_expansions_required--;
    }
    return n_expansions_required > 0;
}

bool Scenario::_header::datasetRequired(Dataset::Value d)
{
    for (size_t i = 0; i < n_datasets; i++) {
        if (datasets[i] == (unsigned long)d) {
            return true;
        }
    }
    return false;
}

void Scenario::_header::requireDataset(Dataset::Value d)
{
    for (size_t i = 0; i < n_datasets; i++) {
        if (datasets[i] == (unsigned long)d) {
            return;
        }
    }
    n_datasets++;
    datasets[n_datasets - 1] = (unsigned long)d;
}

void Scenario::_header::unrequireDataset(Dataset::Value d)
{
    for (int i = 0; i < n_datasets; i++) {
        if (datasets[i] == (unsigned long)d) {
            for (int j = i; j < n_datasets - 1; j++) {
                datasets[j] = datasets[j + 1];
            }
            n_datasets--;
        }
    }
}

Game Scenario::open(const char *path, const char *dpath, Game version)
{
	using std::runtime_error;
	using std::logic_error;

	game = version;

	int c_len;	//length of compressed data

	printf_log("Opening scenario \"%s\"\n", path);

	/* Initialization */

	mod_status = false;
	memset(msg, 0, sizeof(msg));

	AutoFile scx(path, "rb");

	/* Header */
	if (!header.read(scx.get()))
	{
		printf_log("Invalid scenario header in %s\n", path);
		throw runtime_error("Not a valid scenario.");
	}

	printf_log("Outside Header Version %g: ", version2);
	if (!strncmp(header.version, "1.18", 4))
	{
		printf_log("ver1: 1.18 (AOE 2).\n");
		game = AOK;
	}
	else if (!strncmp(header.version, "1.21", 4))
	{
		ver1 = SV1_AOC_SWGB;
		printf_log("ver1: 1.21 (AOE 2 TC or SWGB or SWGB:CC).\n");
	}
	else if (header.version[2] == '1' &&
		(header.version[3] == '0' || header.version[3] == '1')) // 1.10 or 1.11
	{
		printf_log("ver1: 1.1x (AOE 1).\n");
		game = AOE;
	}
	else
	{
		printf_log("Unrecognized scenario version: %s.\n", header.version);
		game = UNKNOWN;
		pergame = NULL;
		throw bad_data_error("unrecognized format version");
	}

	/* Inflate and Read Compressed Data */
	if (header.header_type == HT_AOE2SCENARIO) {
	    // length no longer indicates header length (or likely never truly did)
	    c_len = fsize(path) - ((8 + 2 + header.n_datasets) * 4 + header.instruct_len);
	} else {
	    c_len = fsize(path) - (header.length + 8);	//subtract header length
	}

	if (c_len <= 0)
		throw bad_data_error("no compressed data");

	printf_log("Allocating %d bytes for compressed data buffer.\n", c_len);

	// Open file before allocating buffer since file is more likely to fail.
	AutoFile tempout(dpath, "wb"); // TODO: exclusive access?

	unsigned char *compressed = new unsigned char[c_len];
	fread(compressed, sizeof(char), c_len, scx.get());
	int code = inflate_file(compressed, c_len, tempout.get());
	delete [] compressed;

	tempout.close();

	// throw exception if zlib returned an error
	switch (code)
	{
	case Z_STREAM_ERROR:
		throw logic_error("internal logic fault: zlib stream error");

	case Z_DATA_ERROR:
		throw bad_data_error("invalid compressed data");

	case Z_MEM_ERROR:
		throw std::bad_alloc("not enough memory for decompression");

	case Z_BUF_ERROR:
		throw logic_error("internal logic fault: zlib buffer error");

	case Z_VERSION_ERROR:
		throw runtime_error("Your version of zlib1.dll is incompatible. I need " ZLIB_VERSION);
	}

	read_data(dpath);

    if (game == AOC && is_userpatch()) {
        game = UP;
	    adapt_game();
	}

	return game;
}

/* Open a destination scenario, write the header, and optionally write compressed data */

int Scenario::save(const char *path, const char *dpath, bool write, Game convert, SaveFlags::Value flags)
{
	size_t uc_len;
	int code = 0;	//return from zlib functions

	AutoFile scx(path, "wb");

	switch (convert)
	{
	case AOK:
		if (game == AOC || game == AOHD || game == AOF)
			aoc_to_aok();
		break;
	case AOC:
		switch (game) {
		case AOHD:
		case AOF:
			hd_to_10c();
			break;
		case UP:
			up_to_10c();
			break;
		case AOK:
			aok_to_aoc();
			break;
		}
		break;
	case UP:
		switch (game) {
		case AOHD:
		case AOF:
		case AOHD4:
		case AOF4:
		case AOHD6:
		case AOF6:
			if ((flags & SaveFlags::CONVERT_EFFECTS))
			    hd_to_up();
			break;
		case AOK:
			aok_to_aoc();
			break;
		}
		break;
	case AOHD:
		switch (game) {
		case UP:
			if ((flags & SaveFlags::CONVERT_EFFECTS))
			    up_to_hd();
			break;
		case AOK:
			aok_to_aoc();
			break;
		case AOF4:
		case AOHD4:
			//strip_patch4();
			convert = AOHD;
			break;
		case AOF6:
		case AOHD6:
			//strip_patch4();
			strip_patch6();
			convert = AOHD;
			break;
		}
		break;
	case AOF:
		switch (game) {
		case UP:
			if ((flags & SaveFlags::CONVERT_EFFECTS))
			    up_to_hd();
			break;
		case AOK:
			aok_to_aoc();
			break;
		case AOF4:
		case AOHD4:
			//strip_patch4();
			convert = AOF;
			break;
		case AOF6:
		case AOHD6:
			//strip_patch4();
			strip_patch6();
			convert = AOF;
			break;
		}
		break;
	case AOHD4:
	case AOF4:
		switch (game) {
		case SWGB:
		case SWGBCC:
		case AOHD:
		case AOF:
		case AOC:
			aoc_to_hd4();
			break;
		case UP:
			if ((flags & SaveFlags::CONVERT_EFFECTS))
			    up_to_hd();
			aoc_to_hd4();
			break;
		case AOK:
			aok_to_aoc();
			aoc_to_hd4();
			break;
		}
		break;
	case AOHD6:
	case AOF6:
		switch (game) {
		case SWGB:
		case SWGBCC:
		case AOC:
			aoc_to_hd6();
			break;
		case AOHD:
		case AOF:
			aohd_to_hd6();
			break;
		case AOHD4:
		case AOF4:
			hd4_to_hd6();
		case UP:
			if ((flags & SaveFlags::CONVERT_EFFECTS))
			    up_to_hd();
			aoc_to_hd6();
			break;
		case AOK:
			aok_to_aoc();
			aoc_to_hd6();
			break;
		}
		break;
	case SWGB:
		if (isHD(game))
			hd_to_swgb();
		if (game == UP)
			up_to_swgb();
		if (game == AOK)
			aok_to_aoc();
		break;
	case SWGBCC:
		if (isHD(game))
			hd_to_swgb();
		if (game == UP)
			up_to_swgb();
		if (game == AOK)
			aok_to_aoc();
		break;
	}
	game = convert;
	adapt_game();

	if (strstr(setts.ScenPath, ".aoe2scenario")) {
	    header.header_type = HT_AOE2SCENARIO;
	} else if (! strstr(setts.ScenPath, ".scx2")){
	    header.header_type = HT_SC;
	}

	/* Header */
	header.write(scx.get(), messages, getPlayerCount(), game);

	/* Write uncompressed data to a temp file */
	if (write)
		write_data(dpath);

	/* Then compress and write it to the scenario */
	uc_len = fsize(dpath);
	// Open file before buffer alloc since it's more likely to fail.
	AutoFile temp(dpath, "rb");
	// FIXME: don't allocate this ridiculous thing
	unsigned char *uncompressed = new unsigned char[uc_len];
	fread(uncompressed, sizeof(unsigned char), uc_len, temp.get());
	temp.close();

	code = deflate_file(uncompressed, uc_len, scx.get());

	/* translate zlib codes to AOKTS_ERROR codes */
	switch (code)
	{
	case Z_OK:
	case Z_STREAM_END:
		code = ERR_none;
		break;

	case Z_STREAM_ERROR:
		code = ERR_digit;
		sprintf(msg, e_digit, "zlib stream error");
		break;

	case Z_MEM_ERROR:
		code = ERR_mem;
		printf("Could not allocate memory for compression stream.\n");
		strcpy(msg, e_mem);
		break;

	case Z_BUF_ERROR:
		code = ERR_digit;
		sprintf(msg, e_digit, "zlib buffer error.");
		break;

	case Z_VERSION_ERROR:
		code = ERR_zver;
		sprintf(msg, e_zver, ZLIB_VERSION);
		break;
	default:
		code = ERR_unknown;
		strcpy(msg, "Unknown error!");
		break;
	}

	delete [] uncompressed;
	fflush(stdout);	//report errors to logfile

	adapt_game();

	return code;
}

bool Scenario::_header::read(FILE *scx)
{
	bool ret = true;
	printf_log("header:\n");
	fread(version, sizeof(char), 4, scx);
	version[5] = '\0';
	printf_log("\tversion: %s.\n", version);

	fread(&length, sizeof(long), 1, scx);
	printf_log("\tlength: %ld.\n", length);

	fread(&header_type, sizeof(long), 1, scx); // version number I think
	printf_log("\theader_type: %ld.\n", header_type);
	REPORTS(header_type == HT_SC || header_type == HT_AOE2SCENARIO, ret = false, "Header type value invalid.\n");
	fread(&timestamp, sizeof(timestamp), 1, scx);
	fread(&instruct_len, sizeof(long), 1, scx);
	printf_log("\tinstruct_len: %ld.\n", instruct_len);
	SKIP(scx, instruct_len);             //instructions
	SKIP(scx, sizeof(long));	//unknown
	fread(&n_players, sizeof(long), 1, scx);
	if (header_type == HT_AOE2SCENARIO) {
	    // aoe2scenario format
	    SKIP(scx, sizeof(long));
	    fread(&uses_expansions, sizeof(long), 1, scx);
	    printf_log("uses expansions ? %d\n", uses_expansions);
	    fread(&n_datasets, sizeof(long), 1, scx);
	    for (int i = 0; i < n_datasets; i++) {
	        fread(&datasets[i], sizeof(long), 1, scx);
	        printf_log("dataset %d enabled\n", datasets[i]);
	    }
	}

	return ret;
}

void Scenario::_header::write(FILE *scx, const SString *instr, long players, Game g)
{
	switch (g)
	{
	case AOK:
		strcpy(version, "1.18");
		break;
	case AOC:
	case UP:
	case SWGB:
	case SWGBCC:
	case AOHD:
	case AOF:
	case AOHD4:
	case AOF4:
	case AOHD6:
	case AOF6:
		strcpy(version, "1.21");
		break;
	default:
		strcpy(version, "1.00");
		break;
	}
	fwrite(version, sizeof(char), 4, scx);

    if (header_type == HT_AOE2SCENARIO) {
	    NULLS(scx, sizeof(long));
    } else {
	    /* Length calculation is a little tricky */
	    length = 0x14 + instr->lwn();
	    fwrite(&length, sizeof(long), 1, scx);
	}

	fwrite(&header_type, sizeof(long), 1, scx);

	fwrite(&timestamp, sizeof(timestamp), 1, scx);

	instr->write(scx, sizeof(long));

	NULLS(scx, sizeof(long));

	fwrite(&players, sizeof(long), 1, scx);

	if (header_type == HT_AOE2SCENARIO) {
	    long num = 1000;
	    fwrite(&num, sizeof(long), 1, scx);

        if (expansionsRequired()) {
            if (uses_expansions != 1) {
	            uses_expansions = 1;
	            for (int i = 0; i < n_datasets; i++) {
	                datasets[i]+=2;
	            }
            }
	        requireDataset(Dataset::Unk_xAOK);
	        requireDataset(Dataset::Unk_xAOC);
	    } else {
            uses_expansions = 0;
            n_datasets = 2;
            datasets[0] = 0;
            datasets[1] = 1;
	        //requireDataset(Dataset::AOK_xUnk0);
	        //requireDataset(Dataset::AOC_xUnk1);
        }

	    fwrite(&uses_expansions, sizeof(long), 1, scx);

	    fwrite(&n_datasets, sizeof(long), 1, scx);
	    for (int i = 0; i < n_datasets; i++)
	        fwrite(&datasets[i], sizeof(long), 1, scx);
	}
}

void Scenario::_header::reset()
{
	memset(version, 0, sizeof(version));
	length = 0;
	_time32(&timestamp);
}

/*
	FEP = For Each Player.

	Requires an int i. Takes p from player 1 through player 16.
*/
#define FEP(p) \
	for (i = PLAYER1_INDEX, p = players; i < NUM_PLAYERS; i++, p++)

/* Will check triggers to see if must be userpatch */

bool Scenario::is_userpatch()
{
	long num = triggers.size();

	if (num < 1) {
	    return false;
	}

	Trigger *trig = &(*triggers.begin());

    bool is_userpatch = false;

    // triggers
	long i = num;
	while (i--)
	{
	    // conditions
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
	        if (iter->reserved == -256) {
	            is_userpatch = true;
	        }
		}
	    // effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        switch (iter->type) {
	        case 30:
	        case 31:
	        case 32:
	        case 33:
	            is_userpatch = true;
	            break;
	        }
		}
		trig++;
	}

    if (game != AOC && game != UP)
        is_userpatch = false;

    return is_userpatch;
}

void Scenario::next_sect(FILE * in) {
	//readunk(dc2in.get(), sect, "Resources sect begin", true);

    char buf[sizeof(sect)] = {0};

    //readbin(dc2in.get(), &buf);
    fread(&buf, sizeof(long), 1, in);

    // Find next sect
    int c = 0;
    long test = *(reinterpret_cast<long *>(&buf));
    //show_binrep(test);
    while (test != sect) {
        //show_binrep(buf);
        //show_binrep(sect);
        c++;
        for (int i = 0; i < sizeof(sect) - 1; i++) {
            buf[i] = buf[i+1];
        }
        readbin(in, &buf[sizeof(sect)-1]);
        //printf_log("unfound_count: %d.\n", c);
        test = *(reinterpret_cast<long *>(&buf));
    }
}

/**
 * I suppose this is as good a place as any to document how I designed the
 * reading and writing of scenario data. The idea is that each class knows how
 * to read and write its own data. Unforunately, this is complicated by the
 * scenario format having data spread everywhere (e.g., Player Data 1, 2, 3,
 * 4). So, the rule boils down to:
 *
 * The class that contains the destination data member will read and write that
 * data member.
 *
 * If I were starting over from scratch, I'd probably design a separate class
 * just to do reading and writing, but I'm not. :)
 */

void Scenario::read_data(const char *path)	//decompressed data
{
	int i;
	Player *p;

	printf_log("Read decompressed data file %s...\n", path);
	AutoFile dc2in(path, "rb"); // temp decompressed file

	/* Compressed Header */

	readbin(dc2in.get(), &next_uid);
	readbin(dc2in.get(), &version2);

	if (setts.intense)
		printf_log("Debug 1.\n");

	printf_log("Compressed (inside) Header Version %g: ", version2);
	switch (myround(version2 * 100))
	{
	case 115:
		printf_log("ver2: 1.15 (AOE).\n");
		game = AOE;
		break;

	case 118:
	case 119:
	case 120:
	case 121:
		printf_log("ver2: 1.18-1.21 (AOK).\n");
		game = AOK;
		break;

	case 122:
		printf_log("ver2: 1.22 (AOE 2 TC or SWGB).\n");

		// Treat any existing value for game as a hint.
		switch (scen.game) {
		case UNKNOWN:
		case AOK:
		case AOC:
		case AOHD:
		case AOF:
		case AOHD4:
		case AOF4:
		case AOHD6:
		case AOF6:
		case UP:
		    game = AOC;
		    break;
		case SWGB:
		case SWGBCC:
		    game = SWGB;
		    break;
		}
		break;

	case 123:
		printf_log("ver2: 1.23 (AOHD or AOF).\n");
		if (strstr(setts.ScenPath, ".scx2")) {
		    game = AOF;
		} else {
		    game = AOHD;
		}
		break;

	case 124:
		printf_log("ver2: 1.24 (AOHD or AOF).\n");
		if (strstr(setts.ScenPath, ".scx2")) {
		    game = AOF4;
		} else {
		    game = AOHD4;
		}
		break;

	case 126:
		printf_log("ver2: 1.26 (AOHD or AOF).\n");
		if (strstr(setts.ScenPath, ".scx2")) {
		    game = AOF6;
		} else {
		    game = AOHD6;
		}
		break;

	case 130:
		printf_log("ver2: 1.30 (SWGB:CC).\n");
		game = SWGBCC;
		break;

	default:
		printf_log("Unrecognized scenario version2: %f.\n", version2);
		game = UNKNOWN;
		throw bad_data_error("unrecognized format version");
	}

	adapt_game();

	/* Updates*/
	//read genie data
	try
	{
	    switch (game) {
	    case SWGB:
	    case SWGBCC:
		    esdata.load(datapath_swgb);
		    break;
	    default:
		    esdata.load(datapath_aok);
	    }
	}
	catch (std::exception& ex)
	{
		printf_log("Could not load data: %s\n", ex.what());
		printf_log("Path: %s\n", global::exedir);
		MessageBox(NULL,
			"Could not read Genie Data from xml file.",
			"Error", MB_ICONERROR);
	}

	FEP(p)
		p->read_header_name(dc2in.get());

	if (ver1 >= SV1_AOK)
	{
		FEP(p)
			p->read_header_stable(dc2in.get());
	}

	FEP(p)
		p->read_data1(dc2in.get());

	if (setts.intense)
		printf_log("Debug 2.\n");

	readbin(dc2in.get(), &unknown);
	readunk(dc2in.get(), (char)0, "post-PlayerData1 char");
	readunk(dc2in.get(), -1.0F, "unknown3");

	readcs<unsigned short>(dc2in.get(), origname, sizeof(origname));

	/* Messages & Cinematics */
	if (perversion->mstrings)
		fread(mstrings, sizeof(long), perversion->messages_count, dc2in.get());

    char dwcu[] = {68, 111, 99, 116, 111, 114, 87, 105, 108, 108, 67, 85, 0};
	//show_binrep(dwcu);
	//show_binrep("teststr");
	for (i = 0; i < perversion->messages_count; i++) {
		messages[i].read(dc2in.get(), sizeof(short));
        if (! setts.forceenabletips) {
		        if (strstr(messages[i].c_str(), dwcu) != NULL) {
		            setts.disabletips = 1;
		        }
        }
	}

	printf_log("NUM_CINEM: %d\n", NUM_CINEM);
	for (i = 0; i < NUM_CINEM; i++) {
		readcs<unsigned short>(dc2in.get(), cinem[i], sizeof(cinem[i]));
	    printf_log("Cinematic %d: %s\n", i, cinem[i]);
	}

	/* Bitmap */
	readbin(dc2in.get(), &bBitmap);
	SKIP(dc2in.get(), sizeof(long) * 2);	//x, y (duplicate)
	readunk<short>(dc2in.get(), bBitmap ? -1 : 1, "bitmap unknown");

    // bBitmap default is 3 in 1.26 even if there is no bitmap
	if (bBitmap > 0 && !(( game == AOHD6 || game == AOF6 ) && bBitmap == 3))
		bitmap.read(dc2in.get());

	/* Player Data 2 */

	if (setts.intense)
		printf_log("Debug 3.\n");

    try {
	FEP(p) {
 		readcs<unsigned short>(dc2in.get(), p->vc, sizeof(p->vc));
        printf_log("P%d VCname: %s.\n", i, p->vc);
 	}
	FEP(p) {
 		readcs<unsigned short>(dc2in.get(), p->cty, sizeof(p->cty));
        printf_log("P%d CTYname: %s.\n", i, p->cty);
    }
	FEP(p) {
 		readcs<unsigned short>(dc2in.get(), p->ai, sizeof(p->ai));
        printf_log("P%d AIname: %s.\n", i, p->ai);
    }

	FEP(p) {
        unsigned long len_vc;
	    fread(&len_vc, sizeof(unsigned long), 1, dc2in.get());
        printf_log("P%d Len VC: %d.\n", i, len_vc);

        unsigned long len_cty;
	    fread(&len_cty, sizeof(unsigned long), 1, dc2in.get());
        printf_log("P%d Len CTY: %d.\n", i, len_cty);

        unsigned long len_ai;
	    fread(&len_ai, sizeof(unsigned long), 1, dc2in.get());
        printf_log("P%d Len AI: %d.\n", i, len_ai);

	    if (len_vc)
	        p->vcfile.read(dc2in.get(), sizeof(unsigned long), len_vc);
	    else
	        p->vcfile.erase();

	    if (len_cty)
	        p->ctyfile.read(dc2in.get(), sizeof(unsigned long), len_cty);
	    else
	        p->ctyfile.erase();

	    if (len_ai)
	        p->aifile.read(dc2in.get(), sizeof(unsigned long), len_ai);
	    else
	        p->aifile.erase();
	}

	FEP(p)
		p->read_aimode(dc2in.get());
    }
    catch (const std::length_error& e) {
        MessageBox(NULL, "VC, CTY or AI file corrupted. Skipping to next section.", "Error", MB_ICONERROR);
        //cerr << e.what();
    }
    catch (...) {
        MessageBox(NULL, "VC, CTY or AI file corrupted. Skipping to next section.", "Error", MB_ICONERROR);
    }

    next_sect(dc2in.get());

    try {
	FEP(p) {
		p->read_resources(dc2in.get());
        if (scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6) {
		    p->read_player_number(dc2in.get());
	    } else {
		    p->player_number = i;
	    }
	}

	// fix player numbers, colors and civs if they are absurd
	for (i = 0; i < 9; i++) {
	    if (players[i].player_number < 0 || players[i].player_number >= 9) {
            players[i].player_number = i;
	    }

	    if (players[i].color < 0 || players[i].color >= 9) {
            players[i].color = i;
	    }
	}

	for (i = 9, p = players; i < NUM_PLAYERS; i++, p++)
	{
	    players[i].player_number = i;
	    players[i].color = i;
	}
	}
    catch (...) {
        MessageBox(NULL, "Error reading player resources, number or color. Skipping to next section.", "Error", MB_ICONERROR);
    }

	/* Global Victory */

	//readunk(dc2in.get(), sect, "Global victory sect begin", true);
    next_sect(dc2in.get());

    try {
	readbin(dc2in.get(), &vict);

    // this is to fix broken scenarios
	if (vict.score == 4294967295)
	    vict.score = 14000;

	/* Diplomacy */

	FEP(p)
		p->read_diplomacy(dc2in.get());

	SKIP(dc2in.get(), 0x2D00);	// 11520 should I check this? probably.
	}
    catch (...) {
        MessageBox(NULL, "Error reading global victory section. Skipping to next section.", "Error", MB_ICONERROR);
    }

	//readunk(dc2in.get(), sect, "diplomacy sect middle", true);
    next_sect(dc2in.get());

    try {
	SKIP(dc2in.get(), sizeof(long) * NUM_PLAYERS);	//other allied victory

	if (ver2 == SV2_AOHD_AOF || ver2 == SV2_AOHD_AOF4 || ver2 == SV2_AOHD_AOF6) {
		readbin(dc2in.get(), &lock_teams);
		readbin(dc2in.get(), &player_choose_teams);
		readbin(dc2in.get(), &random_start_points);
		readbin(dc2in.get(), &max_teams);
    }

	/* Disables */

	// UGH why not just one big struct for each player?!
	FEP(p)
		p->read_ndis_techs(dc2in.get());
	FEP(p)
		p->read_dis_techs(dc2in.get(), *perversion);
	FEP(p)
		p->read_ndis_units(dc2in.get());
	FEP(p)
		p->read_dis_units(dc2in.get(), *perversion);
	FEP(p)
		p->read_ndis_bldgs(dc2in.get());
	FEP(p)
		p->read_dis_bldgs(dc2in.get(), *perversion);

	if (setts.intense)
	    printf_log("Debug 4.\n");

	readunk<long>(dc2in.get(), 0, "Disables unused 1", false);
	readunk<long>(dc2in.get(), 0, "Disables unused 2", false);
	readbin(dc2in.get(), &all_techs);
	FEP(p)
		p->read_age(dc2in.get());
	}
    catch (...) {
        MessageBox(NULL, "Error reading global victory section. Skipping to next section.", "Error", MB_ICONERROR);
    }

	//* Map */
    next_sect(dc2in.get());
	//readunk(dc2in.get(), sect, "map sect begin", true);

	players[0].read_camera_longs(dc2in.get());

	map.read(dc2in.get(), ver1);

	/* Population Limits & Units */

	readunk<long>(dc2in.get(), 9, "Player count before units", true);

	for (i = PLAYER1_INDEX; i < GAIA_INDEX; i++)
		players[i].read_data4(dc2in.get(), game);

	// GAIA first! Grrrr.
	players[GAIA_INDEX].read_units(dc2in.get());

	for (i = PLAYER1_INDEX; i < 8; i++)
		players[i].read_units(dc2in.get());

	/* Player Data 3 */

	// TODO: cleanup stopped here

	readunk<long>(dc2in.get(), 9, "Player count before PD3", true);

	for (i = PLAYER1_INDEX; i < 8; i++)
	{
		players[i].read_data3(dc2in.get(),
			i == PLAYER1_INDEX ? editor_pos : NULL	//See scx_format.txt.
			);
	}

	if (setts.intense)
		printf_log("Debug 5.\n");

	/* Triggers */
	readbin(dc2in.get(), &trigver);
	readbin(dc2in.get(), &objstate);

	if (setts.intense)
		printf_log("Debug 9.\n");
	/* Triggers */

	unsigned long n_trigs = readval<unsigned long>(dc2in.get());
	if (setts.intense)
		printf_log("%d triggers.\n", n_trigs);

	triggers.resize(n_trigs);
	if (n_trigs)
	{
		Trigger *t = &(*triggers.begin());

		//read triggers
		for (unsigned int i = 0; i < n_trigs; ++i)
		{
			if (setts.intense)
				printf_log("Reading trigger %d.\n", i);
			t->id = i;
			t++->read(dc2in.get());
		}

		// Directly read trigger order: this /is/ allowed by std::vector.
		t_order.resize(n_trigs);
		readbin(dc2in.get(), &t_order.front(), n_trigs);

		// save the trigger display order to the trigger objects
		for (unsigned int j = 0; j < n_trigs; j++) {
			if (t_order[j] > 0 && t_order[j] <= triggers.size())
			    triggers.at(t_order[j]).display_order = j;
		}
		fix_t_order();

		// verify just the first one because I'm lazy
		if (t_order.front() > n_trigs)
			throw bad_data_error("Trigger order list corrupted. Possibly out of sync.");
	}

    if (ver2 == SV2_AOK) {
        // this code is from aokts-0.3.6, which could open aok files
	    /* Included Files */
	    fread(&unk3, sizeof(long), 2, dc2in.get());	//unk3, unk4

	    if (unk3 == 1 && unk4 == 1)
		    fseek(dc2in.get(), 396, SEEK_CUR);
	} else {
	    /* Included Files */
	    readbin(dc2in.get(), &unk3);
	    readbin(dc2in.get(), &unk4); // aok files sometimes fail here

	    if (unk4 == 1)
	    {
		    printf_log("unk4 hack around %x\n", ftell(dc2in.get()));
		    SKIP(dc2in.get(), 396);
	    }
	}

	if (unk3 == 1)
	{
		readbin(dc2in.get(), &cFiles);

		if (cFiles > 0)
		{
			files = new AOKFile[cFiles];

			for (i = 0; i < cFiles; i++)
			{
				readcs<unsigned long>(dc2in.get(),
					files[i].name, sizeof(files[i].name));
				files[i].data.read(dc2in.get(), sizeof(long));
			}
		}
	}

	if (fgetc(dc2in.get()) != EOF)
		throw bad_data_error("Unrecognized data at end.");

    auto_upgrade_hd4();
    //auto_upgrade_hd6();
	adapt_game();

	// FILE close taken care of by AutoFile! yay.
	printf_log("Done.\n");
}

void Scenario::auto_upgrade_hd4() {
    if (game == AOHD) {
        game = AOHD4;
		aoc_to_hd4();
    }
    if (game == AOF) {
        game = AOF4;
		aoc_to_hd4();
    }
}

void Scenario::auto_upgrade_hd6() {
    if (game == AOHD4) {
        game = AOHD4;
		hd4_to_hd6();
    }
    if (game == AOF4) {
        game = AOF6;
		hd4_to_hd6();
    }
}

void Scenario::clean_format() {
    switch (game) {
    case AOHD:
    case AOF:
    case AOHD4:
    case AOF4:
    case AOHD6:
    case AOF6:
        aoc_to_hd4();
        break;
    }
}

/* Write to-be-compressed data to a temp file */

int Scenario::write_data(const char *path)
{
	FILE *dcout;
	int i;
	long num;
	float f;
	int num_players = Player::num_players;	//for quick access
	Player *p;

	dcout = fopen(path, "wb"); // FIXME: error handling?

	/* Compressed header */

	writebin(dcout, &next_uid);

	switch (ver2)
	{
		case SV2_AOK:
			version2 = 1.20F;
			break;
		case SV2_AOC_SWGB:
			version2 = 1.22F;
			break;
		case SV2_AOHD_AOF:
			version2 = 1.23F;
			break;
		case SV2_AOHD_AOF4:
			version2 = 1.24F;
			break;
		case SV2_AOHD_AOF6:
			version2 = 1.26F;
			break;
		case SV2_SWGBCC:
			version2 = 1.30F;
			break;
		default:
			version2 = 0.00F;
			break;
	}
	writebin(dcout, &version2);

	FEP(p)
		p->write_header_name(dcout);

	if (ver1 >= SV1_AOK)
	{
	    FEP(p)
		    p->write_header_stable(dcout);
	}

	FEP(p)
		p->write_data1(dcout);

	fwrite(&unknown, sizeof(long), 1, dcout);
	putc(0, dcout);
	f = -1;
	fwrite(&f, 4, 1, dcout);
	writecs<unsigned short>(dcout, origname, false);

	/* Messages & Cinematics */
	if (perversion->mstrings)
		fwrite(mstrings, sizeof(long), perversion->messages_count, dcout);

	for (i = 0; i < perversion->messages_count; i++)
		messages[i].write(dcout, sizeof(short));

	for (i = 0; i < NUM_CINEM; i++)
	{
		writecs<unsigned short>(dcout, cinem[i], false);
	}

	fwrite(&bBitmap, 4, 1, dcout);
	if (bBitmap == 0 || (( game == AOHD6 || game == AOF6 ) && bBitmap == 3))
	{
	    NULLS(dcout, 8);
		num = 1;
		fwrite(&num, 2, 1, dcout);
	}
	else
	{
	    fwrite(&bitmap.info_hdr.biWidth, 8, 1, dcout);	//width, height
		writeval(dcout, (short)-1);
		bitmap.write(dcout);
	}
	//NULLS(dcout, 0x40); -- this was vc and cty files

	/* Player Data 2 */

	FEP(p)
		writecs<unsigned short>(dcout, p->vc, false); // vc filename
	FEP(p)
		writecs<unsigned short>(dcout, p->cty, false); // cty filename
	FEP(p)
		writecs<unsigned short>(dcout, p->ai, false); // ai filename
	FEP(p)
	{
	    unsigned long vclen = p->vcfile.length();
	    unsigned long ctylen = p->ctyfile.length();
	    unsigned long ailen = p->aifile.length();
	    printf_log("vclen %lu\n", vclen);
	    printf_log("ctylen %lu\n", ctylen);
	    printf_log("ailen %lu\n", ailen);
		fwrite(&vclen, sizeof(unsigned long), 1, dcout);
		fwrite(&ctylen, sizeof(unsigned long), 1, dcout);
		fwrite(&ailen, sizeof(unsigned long), 1, dcout);
		p->vcfile.write(dcout, sizeof(unsigned long), false, false, true);
		p->ctyfile.write(dcout, sizeof(unsigned long), false, false, true);
		p->aifile.write(dcout, sizeof(unsigned long), false, false, true);
	}
	FEP(p)
		putc(p->aimode, dcout);

	writeval(dcout, sect);
	FEP(p) {
		fwrite(&p->resources, sizeof(long), 6, dcout);
	    if (game == AOHD4 || game == AOF4 || game == AOHD6 || game == AOF6) {
		    fwrite(&p->player_number, sizeof(long), 1, dcout);
		}
	}

	/* Global Victory */

	writeval(dcout, sect);
	fwrite(&vict, sizeof(vict), 1, dcout);

	/* Diplomacy */

#ifndef NOWRITE_D1
	FEP(p)
		fwrite(p->diplomacy, sizeof(long), NUM_PLAYERS, dcout);
#else
	NULLS(dcout, sizeof(long) * NUM_PLAYERS * NUM_PLAYERS);
#endif

	NULLS(dcout, 0x2D00);	//lol

	writeval(dcout, sect);

	//2nd allied victory
	FEP(p)
	{
		num = p->avictory;
		fwrite(&num, sizeof(long), 1, dcout);
	}

	if (ver2 == SV2_AOHD_AOF || ver2 == SV2_AOHD_AOF4 || ver2 == SV2_AOHD_AOF6) {
		fwrite(&lock_teams, sizeof(char), 1, dcout);
		fwrite(&player_choose_teams, sizeof(char), 1, dcout);
		fwrite(&random_start_points, sizeof(char), 1, dcout);
		fwrite(&max_teams, sizeof(char), 1, dcout);
	}

	/* Disables */

	FEP(p)
		fwrite(&p->ndis_t, sizeof(long), 1, dcout);
	FEP(p)
		fwrite(&p->dis_tech, sizeof(long), perversion->max_disables1, dcout);
	FEP(p)
		fwrite(&p->ndis_u, sizeof(long), 1, dcout);
	FEP(p)
		fwrite(&p->dis_unit, sizeof(long), perversion->max_disables1, dcout);
	FEP(p)
		fwrite(&p->ndis_b, sizeof(long), 1, dcout);
	FEP(p)
		fwrite(&p->dis_bldg, sizeof(long), perversion->max_disables2, dcout);

	NULLS(dcout, 0x8);
	fwrite(&all_techs, sizeof(long), 1, dcout);

	if (game == AOHD6 || game == AOF6) {
	    short tmp_starting_age;
	    FEP(p) {
	        tmp_starting_age = p->age + 2;
		    fwrite(&tmp_starting_age, sizeof(short), 1, dcout);
		    fwrite(&p->ending_age, sizeof(short), 1, dcout);
	    }
	} else {
	    long tmp;
	    FEP(p) {
	        tmp = p->age;
		    fwrite(&tmp, sizeof(long), 1, dcout);
		}
	}

	/* Map */

	writeval(dcout, sect);
	num = (long)players[0].camera[1];
	fwrite(&num, 4, 1, dcout);
	num = (long)players[0].camera[0];
	fwrite(&num, 4, 1, dcout);
	map.write(dcout, ver1);

	/* Population Limits & Units */

	fwrite(&num_players, 4, 1, dcout);
	for (i = PLAYER1_INDEX; i < num_players - 1; i++)
	{
		float resources[6];
		resources[0] = (float)players[i].resources[2];	//food
		resources[1] = (float)players[i].resources[1];
		resources[2] = (float)players[i].resources[0];	//gold
		resources[3] = (float)players[i].resources[3];
		resources[4] = (float)players[i].resources[4];
		resources[5] = (float)players[i].resources[5];
		fwrite(resources, sizeof(float), 6, dcout);

        if (game >= AOC && game != SWGB && game != SWGBCC)
			fwrite(&players[i].pop, 4, 1, dcout);
	}

	players[8].write_units(dcout);
	for (i = PLAYER1_INDEX; i < num_players - 1; i++)
	{
		players[i].write_units(dcout);
	}

	/* Player Data 3 */

	fwrite(&num_players, 4, 1, dcout);
	for (i = PLAYER1_INDEX; i < num_players - 1; i++)
	{
		players[i].write_data3(dcout, i,
			i == PLAYER1_INDEX ? editor_pos : NULL	//See scx_format.txt.
			);
	}

	/* Triggers */

	fwrite(&trigver, 8, 1, dcout); //trigger system version
	writebin(dcout, &objstate); //objectives state

	num = triggers.size();
	fwrite(&num, sizeof(long), 1, dcout);

	if (triggers.size() > 0)
	{
	    Trigger *t_parse = &(*triggers.begin());
	    i = num;
	    while (i--)
	    {
            if (setts.intense)
                printf_log("trigger %d.\n", num - i);
		    t_parse->write(dcout);
		    t_parse++;
	    }

	    // Write out t_order vector
	    if (num > 0)  // ugly: have to do this check because we call front()
	    {
		    fwrite(&t_order.front(), sizeof(long), num, dcout);
	    }
	}

	/* Included Files */

	writebin(dcout, &unk3);
	NULLS(dcout, sizeof(unk4)); // we don't write the ES-only data

	if (unk3 == 1)
	{
		writebin(dcout, &cFiles);

		for (long i = 0; i != cFiles; ++i)
		{
			writecs<unsigned long>(dcout, files[i].name);
			files[i].data.write(dcout, sizeof(long));
		}
	}

	fclose(dcout);

	return ERR_none;
}

bool Scenario::export_bmp(const char *path)
{
	if (bBitmap == 0 || (( game == AOHD6 || game == AOF6 ) && bBitmap == 3))
		return false;

	return bitmap.toFile(path);
}

void Scenario::fix_t_order()
{
	t_order.clear();
	t_order.resize(triggers.size());
	for (unsigned int j = 0; j < triggers.size(); j++) {
	    t_order[j] = -1;
	}

	for (unsigned int j = 0; j < triggers.size(); j++) {
	    if (triggers[j].display_order > 0 && triggers[j].display_order < triggers.size()) {
	        t_order[triggers[j].display_order] = j;
	    }
	}

	for (unsigned int j = 0; j < triggers.size(); j++) {
	    if (!(triggers[j].display_order > 0 && triggers[j].display_order < triggers.size())) {
	        for (unsigned int k = 0; k < triggers.size(); k++) {
	            if (t_order[k] == -1) {
	                t_order[k] = j;
	                triggers.at(j).display_order = k;
	                break;
	            }
	        }
	    }
	}
}

//I HATE THIS FUNCTION!
void Scenario::clean_triggers()
{
	vector<bool> kill(triggers.size(), true); //contains whether a trigger should be deleted

	// don't kill the ones that are still indexed on t_order
	for (vector<unsigned long>::const_iterator i = t_order.begin();
		i != t_order.end(); ++i)
		kill[*i] = false;

	/* Now delete the ones still on the kill list. We do this backwards because
	 * that only invalidates the indices on and after the current one. */
	for (size_t i = triggers.size(); i != 0; --i)
	{
		// i is the index - 1 to make i != 0 work
		size_t t_index = i - 1;

		if (!kill[t_index])	//If it shouldn't be deleted...
			continue;           //... skip all this

		triggers.erase(triggers.begin() + t_index);

		// Decrement t_order entries referencing triggers beyond the one we
		// just removed.
		for (vector<unsigned long>::iterator j = t_order.begin();
			j != t_order.end(); ++j)
		{
			if (*j > t_index)
				--(*j);
		}

		if (triggers.size() > 0) { // need this because may have just deleted (see above) the last trigger
		    //adjust effect's trigger indexes accordingly (UGH!)
		    Trigger *t_parse = &(*triggers.begin());
		    for (size_t j = 0; j != triggers.size(); ++j)
		    {
			    for (vector<Effect>::iterator i = t_parse->effects.begin();
				    i != t_parse->effects.end(); ++i)
			    {
				    if (i->trig_index > t_index)
					    i->trig_index--;
				    else if (i->trig_index == t_index)
					    i->trig_index = (unsigned)-1; // TODO: hack cast

				    if (i->parent_trigger_id > t_index)
					    i->parent_trigger_id--;
			    }

			    for (vector<Condition>::iterator i = t_parse->conds.begin();
				    i != t_parse->conds.end(); ++i)
			    {
				    if (i->parent_trigger_id > t_index)
					    i->parent_trigger_id--;
			    }

			    t_parse++;
		    }
		}
	}
}

size_t Scenario::insert_trigger(Trigger *t, size_t after)
{
    t->id = triggers.size();
    triggers.push_back(*t);
	size_t tindex = triggers.size() - 1;

	// TODO: does the -1 work?
	if (after == -1)
	{
		t_order.push_back(tindex);
	}
	else
	{
		// Find the /after/ value in t_order.
		vector<unsigned long>::iterator where =
			std::find(t_order.begin(), t_order.end(), after);

		// If the /after/ value was actually there, insert after it.
		// (Otherwise, we'll insert at the end, i.e. append.)
		if (where != t_order.end())
			++where;

		t_order.insert(where, tindex);
	}

	return tindex;
}

void Scenario::accept(TriggerVisitor& tv)
{
	for (size_t i = 0; i != triggers.size(); ++i)
		triggers.at(i).accept(tv);
}

bool Scenario::exFile(const char *directory, long index)
{
	int c;
	FILE *out;
	AOKFile *parse = files;
	char buffer[_MAX_PATH];

	_mkdir(directory);

	if (index < 0)
	{
		c = cFiles;
	}
	else
	{
		c = 1;
		parse += index;
	}

	while (c--)
	{
		sprintf(buffer, "%s\\%s.per", directory, parse->name);
		if (out = fopen(buffer, "wb"))
		{
			fwrite(parse->data.c_str(), sizeof(char), parse->data.length(), out);
			fclose(out);
		}
		else
			break;

		parse++;
	}

	return (c == -1);	//if it doesn't, we broke early
}

/*	MAP COPY

	Map copy data is stored as follows in memory:

	size_t width, height, unitcount[NUM_PLAYERS];
	Terrain terrain[x][y];
	Unit units[unitcount];	//x and y modified to dist from the bottom-left corner of copied area
*/

class MapCopyCache
{
public:
	RECT source;

	size_t unitcount[NUM_PLAYERS];
	std::vector<Unit> units;
	//SList<Unit> units;
};

#define ISINRECT(r, x, y) \
	(x >= r.left && x <= r.right && y >= r.bottom && y <= r.top)

#define ISATPOINT(r, x, y) \
	(x == r.x && y == r.y)

// functor to check if a unit is in a RECT
class unit_in_rect : public std::unary_function<const Unit, bool>
{
public:
	unit_in_rect(const RECT& rect)
		: _rect(rect)
	{}

	// I'm a little confused that you can use a reference here, since we tell
	// others we take "const Unit", but it works. Shrug.
	bool operator()(const Unit& u) const
	{
		return ISINRECT(_rect, u.x, u.y);
	}

private:
	RECT _rect;
};

void Scenario::swapTerrain(unsigned char newcnst, unsigned char oldcnst) {
	for (LONG i = 0; i < map.x; i++) {
		for (LONG j = 0; j < map.y; j++) {
		    if (map.terrain[i][j].cnst == oldcnst)
		        map.terrain[i][j].cnst = newcnst;
		}
	}
}

void Scenario::outline(unsigned long x, unsigned long y, unsigned char newcnst, unsigned char oldcnst, TerrainFlags::Value flags) {
    floodFill4(x,y,TEMPTERRAIN,oldcnst);
    if (flags & TerrainFlags::FORCE) {
        outlineDraw(x,y,TEMPTERRAIN2,oldcnst, flags);
        swapTerrain(newcnst, TEMPTERRAIN2);
    } else {
        outlineDraw(x,y,newcnst,oldcnst, flags);
    }
}

bool Scenario::isTerrainEdge(unsigned char cnst, unsigned char newcnst, unsigned char oldcnst) {
    return (cnst != oldcnst &&
            cnst != TEMPTERRAIN &&
            cnst != OUTOFBOUNDS &&
            cnst != newcnst);
}

// Recursive outline
// Make this 9-way rather than 4
unsigned char Scenario::outlineDraw(unsigned long x, unsigned long y, unsigned char newcnst, unsigned char oldcnst, TerrainFlags::Value flags)
{
    if (!(x >= 0 && x < map.x && y >= 0 && y < map.y))
        return OUTOFBOUNDS;

    if (map.terrain[x][y].cnst == TEMPTERRAIN) {
        map.terrain[x][y].cnst = oldcnst;

        unsigned char t1 = outlineDraw(x - 1, y + 1, newcnst, oldcnst, flags);
        unsigned char t2 = outlineDraw(x + 1, y - 1, newcnst, oldcnst, flags);
        unsigned char t3 = outlineDraw(x - 1, y - 1, newcnst, oldcnst, flags);
        unsigned char t4 = outlineDraw(x + 1, y + 1, newcnst, oldcnst, flags);
        unsigned char t5 = outlineDraw(x,     y - 1, newcnst, oldcnst, flags);
        unsigned char t6 = outlineDraw(x - 1, y,     newcnst, oldcnst, flags);
        unsigned char t7 = outlineDraw(x + 1, y,     newcnst, oldcnst, flags);
        unsigned char t8 = outlineDraw(x,     y + 1, newcnst, oldcnst, flags);

        bool colorit =  isTerrainEdge(t5, newcnst, oldcnst) ||
                        isTerrainEdge(t6, newcnst, oldcnst) ||
                        isTerrainEdge(t7, newcnst, oldcnst) ||
                        isTerrainEdge(t8, newcnst, oldcnst);

        if (flags & TerrainFlags::EIGHT) {
            colorit = colorit ||
                        isTerrainEdge(t1, newcnst, oldcnst) ||
                        isTerrainEdge(t2, newcnst, oldcnst) ||
                        isTerrainEdge(t3, newcnst, oldcnst) ||
                        isTerrainEdge(t4, newcnst, oldcnst);
        }

        if (colorit) {
            map.terrain[x][y].cnst = newcnst;
        }
    }

    return map.terrain[x][y].cnst;
}

// Recursive 4-way floodfill, crashes if recursion stack is full
void Scenario::floodFill4(unsigned long x, unsigned long y, unsigned char newcnst, unsigned char oldcnst)
{
    if(x >= 0 && x < map.x && y >= 0 && y < map.y && map.terrain[x][y].cnst == oldcnst && map.terrain[x][y].cnst != newcnst)
    {
        map.terrain[x][y].cnst = newcnst;

        floodFill4(x + 1, y,     newcnst, oldcnst);
        floodFill4(x - 1, y,     newcnst, oldcnst);
        floodFill4(x,     y + 1, newcnst, oldcnst);
        floodFill4(x,     y - 1, newcnst, oldcnst);
    }
}

// Recursive 4-way floodfill, crashes if recursion stack is full
void Scenario::floodFillElev4(unsigned long x, unsigned long y, unsigned char newelev, unsigned char cnst)
{
    if(x >= 0 && x < map.x && y >= 0 && y < map.y && map.terrain[x][y].cnst == cnst && map.terrain[x][y].elev != newelev)
    {
        map.terrain[x][y].elev = newelev;

        floodFillElev4(x + 1, y,     newelev, cnst);
        floodFillElev4(x - 1, y,     newelev, cnst);
        floodFillElev4(x,     y + 1, newelev, cnst);
        floodFillElev4(x,     y - 1, newelev, cnst);
    }
}

int Scenario::map_size(const RECT &source, MapCopyCache *&mcc)
{
	Player *p;
	int ret;
	int i;

	mcc = new MapCopyCache;

	CopyRect(&mcc->source, &source);

	/* x, u, unitcount */
	ret = sizeof(size_t) * 2;
	ret += sizeof(mcc->unitcount);

	/* space for the tiles */
	ret += sizeof(Map::Terrain) * (source.top - source.bottom + 1) * (source.right - source.left + 1);

	/* space for the units */
	unit_in_rect in_source(source);
	FEP(p)
	{
		using std::vector;

		// Convenience, to ensure we modify the right unitcount.
		size_t& unitcount = mcc->unitcount[i];

		// Get the initial size: this is a hack because I'm too lazy to make
		// individual vectors for each player.
		unitcount = mcc->units.size();

		std::remove_copy_if(p->units.begin(), p->units.end(),
			std::back_inserter(mcc->units), std::not1(in_source));

		unitcount = mcc->units.size() - unitcount;
		ret += unitcount * Unit::size;
	}

	return ret;
}

AOKTS_ERROR Scenario::add_activation(size_t start, size_t end, size_t to) {

    Trigger *d_trig = &triggers.at(to);
    Effect e = Effect();

    for (size_t i = start; i <= end; i++) {
        e.trig_index = i;
        e.type = 8;
        d_trig->effects.push_back(e);
    }

	return ERR_none;
}

AOKTS_ERROR Scenario::move_triggers(size_t start, size_t end, size_t to) {
	size_t num = triggers.size();
	// don't need to do this as start is unsigned: start >= 0
	if (num > 0 && end >= start && to < num && end < num) {
        for (size_t i = 0; i < num; i++) {
            long displacement = to - start;
            size_t range = end - start;
            size_t toend = to + range;
            if (triggers.at(i).display_order >= (long)start && triggers.at(i).display_order <= (long)end) {
                triggers.at(i).display_order += displacement;
            } else {
                if (displacement > 0 && triggers.at(i).display_order > (long)end && triggers.at(i).display_order <= (long)toend) {
                    triggers.at(i).display_order -= (range + 1);
                } else if (displacement < 0 && triggers.at(i).display_order >= (long)to && triggers.at(i).display_order < (long)start) {
                    triggers.at(i).display_order += (range + 1);
                }
            }
            //t_order[(size_t)triggers.at(i).display_order] = i;
        }

        for (size_t i = 0; i < num; i++) {
            if (triggers.at(i).display_order >= 0 && triggers.at(i).display_order < (long)triggers.size())
                t_order[triggers.at(i).display_order] = i;
        }
        //clean_triggers();
    }

	return ERR_none;
}

AOKTS_ERROR Scenario::swap_players(int a, int b) {
    std::swap(players[a], players[b]);

    // keep correct colors
    long color = players[a].color;
    players[a].color = players[b].color;
    players[b].color = color;

	// Need to fix diplomacy of all the OTHER players
	long tempdiplo;
	Player * p;
	int i;
	for (i = PLAYER1_INDEX, p = players; i < NUM_PLAYERS; i++, p++)
	{
	    tempdiplo = p->diplomacy[a];
	    p->diplomacy[a] = p->diplomacy[b];
	    p->diplomacy[b] = tempdiplo;
	}

    a = to_ecplayer(a);
    b = to_ecplayer(b);

    // each triggers
	for (vector<Trigger>::iterator trig = triggers.begin(); trig != triggers.end(); ++trig) {
	    // each effect
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        if (iter->s_player == a)
	            iter->s_player = b;
	        else if (iter->s_player == b)
	            iter->s_player = a;

	        if (iter->t_player == a)
	            iter->t_player = b;
	        else if (iter->t_player == b)
	            iter->t_player = a;
	    }
	    // conditions
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
	        if (iter->player == a)
	            iter->player = b;
	        else if (iter->player == b)
	            iter->player = a;
        }
	}

	return ERR_none;
}

AOKTS_ERROR Scenario::delete_triggers(size_t start, size_t end) {
	size_t num = triggers.size();
	// don't need to do this as start is unsigned: start >= 0
	if (num > 0 && end >= start && end < num) {
        for (size_t i = start; i <= end; i++) {
	        vector<unsigned long>::iterator iter =
		        std::find(t_order.begin(), t_order.end(), i);

	        if (iter != t_order.end()) // Is this check necessary?
		        t_order.erase(iter);
		}
		//update_display_order(); // need this because some trigger ids are now missing and will crash sync.
		//how to do this? only way I know how is to save and reload
		//scenario
		//clean_triggers(); // not this one
    }
	return ERR_none;
}

AOKTS_ERROR Scenario::duplicate_triggers(size_t start, size_t end, size_t to) {
	size_t num = triggers.size();
	// don't need to do this as start is unsigned: start >= 0
	if (num > 0 && end >= start && to < num && end < num) {
        for (size_t i = 0; i < num; i++) {
            long displacement = to - start;
            size_t range = end - start;
            size_t toend = to + range;
            if (triggers.at(i).display_order >= (long)start && triggers.at(i).display_order <= (long)end) {
                // copy it to its spot
                triggers.at(i).display_order += displacement;
            } else {
                if (displacement > 0 && triggers.at(i).display_order > (long)end && triggers.at(i).display_order <= (long)toend) {
                    triggers.at(i).display_order -= (range + 1);
                } else if (displacement < 0 && triggers.at(i).display_order >= (long)to && triggers.at(i).display_order < (long)start) {
                    triggers.at(i).display_order += (range + 1);
                }
            }
            //t_order[(size_t)triggers.at(i).display_order] = i;
        }

        for (size_t i = 0; i < num; i++) {
            if (triggers.at(i).display_order >= 0 && triggers.at(i).display_order < (long)triggers.size())
                t_order[triggers.at(i).display_order] = i;
        }
        //clean_triggers();
    }

	return ERR_none;
}

AOKTS_ERROR Scenario::swap_triggers(long id_a, long id_b) {
    triggers.at(id_a).id = id_b;

    for (size_t ie = 0; ie < triggers.at(id_a).effects.size(); ie++) {
        triggers.at(id_a).effects.at(ie).parent_trigger_id = id_b;
    }

    for (size_t ie = 0; ie < triggers.at(id_a).conds.size(); ie++) {
        triggers.at(id_a).conds.at(ie).parent_trigger_id = id_b;
    }

    triggers.at(id_b).id = id_a;

    for (size_t ie = 0; ie < triggers.at(id_b).effects.size(); ie++) {
        triggers.at(id_b).effects.at(ie).parent_trigger_id = id_a;
    }

    for (size_t ie = 0; ie < triggers.at(id_b).conds.size(); ie++) {
        triggers.at(id_b).conds.at(ie).parent_trigger_id = id_a;
    }
    size_t num = triggers.size();

    // swap these triggers' IDs (positions)
    iter_swap(triggers.begin() + id_a, triggers.begin() + id_b);
    for (size_t k = 0; k < num; k++) {
        for (size_t ie = 0; ie < triggers.at(k).effects.size(); ie++) {
            if (triggers.at(k).effects.at(ie).trig_index == id_a) {
                triggers.at(k).effects.at(ie).trig_index = id_b;
            } else if (triggers.at(k).effects.at(ie).trig_index == id_b) {
                triggers.at(k).effects.at(ie).trig_index = id_a;
            }
        }
    }
	return ERR_none;
}

AOKTS_ERROR Scenario::sync_triggers() {
    // Before doing this, make sure no trigger display orders are set to
    // -1 (not really set). Use the trigview.

    // Don't use this display order, use the treeview.

	size_t num = triggers.size();
	if (num > 0) {
        for (size_t i = 0; i < num; i++) {
            // if DO == ID
            if (triggers.at(i).display_order == (long)i) {
                // increment the display order of any other triggers that have same display order
                for (size_t j = i+1; j < num; j++) {
                    if (triggers.at(j).display_order == (long)i) {
                        triggers.at(j).display_order++;
                    }
                }
            // if DO != ID
            } else {
                // check each following trigger (trigger with larger ID) for DO == reference ID (of original trigger)
                bool swapped = false; // only swap once, then increment for the rest
                for (size_t j = i+1; j < num; j++) {
                    if (triggers.at(j).display_order == (long)i) {
                        if (!swapped) {
                            swap_triggers(i, j); // only taking one of them
                            swapped = true;
                        } else {
                            if (triggers.at(j).display_order == (long)i) {
                                triggers.at(j).display_order++;
                            }
                        }
                    }
                }
                if (!swapped) {
                    triggers.at(i).display_order = i;
                }
            }
			t_order[i] = i;
        }
    }

	return ERR_none;
}

AOKTS_ERROR Scenario::map_copy(Buffer &to, const MapCopyCache *mcc)
{
	size_t w, h;
	unsigned x, y;

	x = mcc->source.left;
	y = mcc->source.bottom;

	w = mcc->source.right - x;
	h = mcc->source.top - y;

	to.write(&w, sizeof(w));
	to.write(&h, sizeof(h));
	to.write(mcc->unitcount, sizeof(mcc->unitcount));

	map.writeArea(to, mcc->source);

	for (vector<Unit>::const_iterator iter = mcc->units.begin();
		iter != mcc->units.end(); ++iter)
	{
		Unit u(*iter);
		u.x -= x;
		u.y -= y;
		u.toBuffer(to);
	}

	delete mcc;

	return ERR_none;
}

AOKTS_ERROR Scenario::delete_player_units(int pindex)
{
	players[pindex].units.clear();
	return ERR_none;
}

AOKTS_ERROR Scenario::map_paste(const POINT &to, Buffer &from)
{
	using std::vector;

	size_t x, y, unitcount[NUM_PLAYERS];
	unsigned i;
	Player *p;

	from.read(&x, sizeof(x));
	from.read(&y, sizeof(y));
	from.read(unitcount, sizeof(unitcount));

	RECT area;
	area.left = to.x;
	area.bottom = to.y;
	area.right = area.left + x;
	area.top = area.bottom + y;

	map.readArea(from, area);
	area.right += 1;
	area.top += 1;

	unit_in_rect in_dest(area);
	FEP(p)
	{
		/* find units in the paste area and erase them */
		p->units.erase(
			remove_if(p->units.begin(), p->units.end(), in_dest),
			p->units.end());

		/* add the units from the copy buffer */
		size_t count = unitcount[i];
		while (count--)
		{
			Unit u(from);
			u.ident = next_uid++;
			u.x += to.x;
			u.y += to.y;
			p->units.push_back(u);
		}
	}

	return ERR_none;
}

AOKTS_ERROR Scenario::map_scale(const RECT &area, const float scale)
{
	//LONG dx, dy, w, h;

	map.scaleArea(area, scale);

    //// each player
	//for (int i = 0; i < NUM_PLAYERS; i++) {
    //    // camera
    //    if (ISINRECT(from, players[i].camera[0], players[i].camera[0])) {
    //        players[i].camera[0] += dx;
    //        players[i].camera[1] += dy;
    //    }

    //    // units
	//    for (vector<Unit>::iterator iter = players[i].units.begin(); iter != players[i].units.end(); ++iter) {
    //        if (ISINRECT(from, iter->x, iter->y)) {
	//	        iter->x += dx;
	//	        iter->y += dy;
	//	    } else if (ISINRECT(torect, iter->x, iter->y)) {
	//	        iter->x -= dx;
	//	        iter->y -= dy;
	//	    }
	//    }
	//}

	//Trigger *trig = triggers.first();
	//long num = triggers.count();

    //// triggers
	//long i = num;
	//while (i--)
	//{
	//    // effects
	//    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	//	    if (ISINRECT(from, iter->location.x, iter->location.y)) {
	//	        iter->location.x += dx;
	//	        iter->location.y += dy;
	//	    } else if (ISINRECT(torect, iter->location.x, iter->location.y)) {
	//	        iter->location.x -= dx;
	//	        iter->location.y -= dy;
	//	    }
	//	    if (ISINRECT(from, iter->area.right, iter->area.top)) {
	//	        iter->area.right += dx;
	//	        iter->area.top += dy;
	//	    } else if (ISINRECT(torect, iter->area.right, iter->area.top)) {
	//	        iter->area.right -= dx;
	//	        iter->area.top -= dy;
	//	    }
	//	    if (ISINRECT(from, iter->area.left, iter->area.bottom)) {
	//	        iter->area.left += dx;
	//	        iter->area.bottom += dy;
	//	    } else if (ISINRECT(torect, iter->area.left, iter->area.bottom)) {
	//	        iter->area.left -= dx;
	//	        iter->area.bottom -= dy;
	//	    }
	//	}
	//    // conditions
	//    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
	//	    if (ISINRECT(from, iter->area.right, iter->area.top)) {
	//	        iter->area.right += dx;
	//	        iter->area.top += dy;
	//	    } else if (ISINRECT(torect, iter->area.right, iter->area.top)) {
	//	        iter->area.right -= dx;
	//	        iter->area.top -= dy;
	//	    }
	//	    if (ISINRECT(from, iter->area.left, iter->area.bottom)) {
	//	        iter->area.left += dx;
	//	        iter->area.bottom += dy;
	//	    } else if (ISINRECT(torect, iter->area.left, iter->area.bottom)) {
	//	        iter->area.left -= dx;
	//	        iter->area.bottom -= dy;
	//	    }
	//	}
	//	trig++;
	//}

	return ERR_none;
}

AOKTS_ERROR Scenario::remove_trigger_names()
{
	long num = triggers.size();
	if (num > 0) {
	    Trigger *trig = &(*triggers.begin());

        // triggers
	    long i = num;
	    while (i--)
	    {
	        strcpy(trig->name, "");
		    trig++;
	    }
	}

	return ERR_none;
}

AOKTS_ERROR Scenario::remove_trigger_descriptions()
{
	long num = triggers.size();
	if (num > 0) {
	    Trigger *trig = &(*triggers.begin());

        // triggers
	    long i = num;
	    while (i--)
	    {
		    char *cstr = trig->description.unlock(1);
	        strcpy(cstr, "");
		    trig->description.lock();
		    trig++;
	    }
	}

	return ERR_none;
}

AOKTS_ERROR Scenario::save_pseudonyms()
{
	long num = triggers.size();
	if (num > 0) {
	    Trigger *trig = &(*triggers.begin());

        // triggers
	    long i = num;
	    while (i--)
	    {
	        strncpy ( trig->name, trig->getName(true, true, 2).c_str(), MAX_TRIGNAME );
	        trig->name[MAX_TRIGNAME] = '\0';
		    trig++;
	    }
	}

	return ERR_none;
}

AOKTS_ERROR Scenario::prefix_display_order()
{
	char tmp[MAX_TRIGNAME];
	long num = triggers.size();
	if (num > 0) {
	    Trigger *trig = &(*triggers.begin());

        // triggers
	    long i = num;
	    while (i--)
	    {
	        strncpy (tmp, trig->name, MAX_TRIGNAME);
	        _snprintf(trig->name, sizeof(char) * MAX_TRIGNAME, "<%d> %s", trig->display_order, tmp);
		    trig++;
	    }
	}

	return ERR_none;
}

AOKTS_ERROR Scenario::remove_display_order_prefix()
{
	char tmpdo[MAX_TRIGNAME];
	char tmpname[MAX_TRIGNAME];
	char tmpnamedo[MAX_TRIGNAME];
	int lendo=0;
	int lenname=0;
	long num = triggers.size();
	if (num > 0) {
	    Trigger *trig = &(*triggers.begin());

        // triggers
	    long i = num;
	    while (i--)
	    {
	        _snprintf(tmpdo, sizeof(char) * MAX_TRIGNAME, "<%d> ", trig->display_order);
	        strncpy(tmpname, trig->name, MAX_TRIGNAME);
	        strncpy(tmpnamedo, trig->name, MAX_TRIGNAME);
	        lendo=strlen(tmpdo);
	        lenname=strlen(tmpname);

	        tmpnamedo[lendo]='\0';

            // check if the D.O. and D.O. substring are equal
	        if (strcmp(tmpdo, tmpnamedo) == 0) {
	            strncpy(trig->name, &tmpname[lendo], MAX_TRIGNAME);
	            trig->name[MAX_TRIGNAME] = '\0';
		        trig++;
		    }
	    }
	}

	return ERR_none;
}

AOKTS_ERROR Scenario::swap_trigger_names_descriptions()
{
	long num = triggers.size();
	if (num > 0) {
	    Trigger *trig = &(*triggers.begin());

        // triggers
	    long i = num;
	    while (i--)
	    {
	        char buffer[MAX_TRIGNAME];
		    char *cstr = trig->description.unlock(MAX_TRIGNAME);
	        strncpy ( buffer, cstr, MAX_TRIGNAME );
	        buffer[MAX_TRIGNAME] = '\0';
	        strncpy ( cstr, trig->name, MAX_TRIGNAME );
	        cstr[MAX_TRIGNAME] = '\0';
		    trig->description.lock();
	        strncpy ( trig->name, buffer, MAX_TRIGNAME );
	        trig->name[MAX_TRIGNAME] = '\0';
		    trig++;
	    }
	}

	return ERR_none;
}

bool up_to_aofe_test(const Effect & e)
{
     return e.type == 30 || e.type == 31 || e.type == 32 || e.type == 33;
}

AOKTS_ERROR Scenario::up_to_aofe() {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // using a lamba function, delete speed range armor1 armor2
        trig->effects.erase(std::remove_if(trig->effects.begin(), trig->effects.end(), up_to_aofe_test), trig->effects.end());
		trig++;
	}
	return ERR_none;
}

bool up_to_10c_test(const Effect & e)
{
    return e.type > Effect::NUM_EFFECTS_AOC;
}

AOKTS_ERROR Scenario::up_to_10c() {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // using a lamba function, delete speed range armor1 armor2
        // remove effects that change name over area
        // prevent using damage on gaia
        trig->effects.erase(std::remove_if(trig->effects.begin(), trig->effects.end(), up_to_10c_test), trig->effects.end());
		trig++;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::hd_to_10c() {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // using a lamba function, delete speed range armor1 armor2
        // remove effects that change name over area
        // prevent using damage on gaia
        trig->effects.erase(std::remove_if(trig->effects.begin(), trig->effects.end(), up_to_10c_test), trig->effects.end());
		trig++;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::hd_to_swgb() {
	return ERR_none;
}

AOKTS_ERROR Scenario::up_to_swgb() {
	return ERR_none;
}

AOKTS_ERROR Scenario::up_to_hd() {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // conditions
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
	        if (iter->reserved == -1) {
	            iter->reverse_hd = 0;
	        } else if (iter->reserved == -256) {
	            iter->reserved = -1;
	            iter->reverse_hd = 1;
	        }
		}

	    // unify effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end();++iter) {
	        if (iter->type == EffectType::ChangeMeleArmor_UP) {
	            for (vector<Effect>::iterator iter2 = trig->effects.begin(); iter2 != trig->effects.end(); ++iter2) {
	                if (iter2->type == EffectType::ChangePiercingArmor_UP &&
	                        iter->selectedUnits().compare(iter->selectedUnits())
	                        == 0 && iter->amount == iter2->amount) {
	                    trig->effects.erase(iter2);
	                    break;
	                }
	            }
	        }
		}

	    // convert remaining effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        switch (iter->type) {
	        case EffectType::ChangeSpeed_UP:
	            iter->type = EffectType::ChangeSpeed_HD;
	        break;
	        case EffectType::ChangeRange_UP:
	            iter->type = EffectType::ChangeRange_HD;
	        break;
	        case EffectType::ChangeMeleArmor_UP:
	            iter->type = EffectType::ChangeArmor_HD;
	        break;
	        case EffectType::ChangePiercingArmor_UP:
	            iter->type = EffectType::ChangeArmor_HD; // no way to make this function reversible
	        break;
	        }
	    }

		trig++;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::aoc_to_hd4() {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    bool removed = false;

    player_choose_teams = 1;
    max_teams = 4;

    // triggers
	long i = num;
	while (i--)
	{
	    // make sure hd reverse condition values are correct
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
	        if (iter->reverse_hd == -1) {
	            iter->reverse_hd = 0;
	        }
	    }

		trig++;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::aohd_to_hd6() {
	aoc_to_hd6();
	return ERR_none;
}

AOKTS_ERROR Scenario::aoc_to_hd6() {
    aoc_to_hd4();
	return ERR_none;
}

AOKTS_ERROR Scenario::hd4_to_hd6() {
	return ERR_none;
}

AOKTS_ERROR Scenario::aoc_to_aok() {
	//Player *p;
	//int i;
	//FEP(p)
	//	p->ucount=1.0F;
	return ERR_none;
}

AOKTS_ERROR Scenario::aok_to_aoc() {
	//Player *p;
	//int i;
	//FEP(p)
	//	p->ucount=2.0F;
	return ERR_none;
}

AOKTS_ERROR Scenario::strip_patch4() {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        if (iter->size > 24)
	            iter->size = 24;
		}

	    // conditions
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
	        if (iter->size > 18)
	            iter->size = 18;
		}
		trig++;
	}

	return ERR_none;
}

AOKTS_ERROR Scenario::strip_patch6() {
	return ERR_none;
}

AOKTS_ERROR Scenario::hd_to_up() {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // conditions
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
	        if (iter->reverse_hd == 1) {
	            iter->reserved = -256;
	        } else if (iter->reverse_hd == 0) {
	            iter->reserved = -1;
	        }
	        iter->reverse_hd = -1;
		}

	    // effects
	    vector<Effect> neweffects;
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        switch (iter->type) {
	            case EffectType::ChangeSpeed_HD:
	                iter->type = EffectType::ChangeSpeed_UP;
	            break;
	            case EffectType::ChangeRange_HD:
	                iter->type = EffectType::ChangeRange_UP;
	            break;
	            case EffectType::ChangeArmor_HD:
	                iter->type = EffectType::ChangeMeleArmor_UP;
		            neweffects.push_back(*iter);
		            neweffects.back().type = EffectType::ChangePiercingArmor_UP;
	            break;
	        }
		}
	    for (vector<Effect>::iterator iter = neweffects.begin(); iter != neweffects.end(); ++iter) {
	        trig->effects.push_back(*iter);
	    }
		trig++;
	}

    // each player i
	for (int i = 0; i < NUM_PLAYERS; i++) {
        // units
	    for (std::vector<Unit>::iterator unit = players[i].units.begin(); unit != players[i].units.end(); ++unit) {
            switch (unit->getType()->id()) {
            case 445: // Church 4 (AOF) / INVALID (AOFE)
                // change to Monastery (104)
                unit->setType(esdata.units.getById(104));
                break;
            case 918: // Brown Rock (AOF) / Monk With Relic (AOFE)
                // change to Rock (104)
                unit->setType(esdata.units.getById(623));
                break;
            }
	    }
	}

	return ERR_none;
}

AOKTS_ERROR Scenario::fix_trigger_outliers() {
	RECT area;
	area.left = -1;
	area.bottom = -1;
	area.right = map.x - 1;
	area.top = map.y - 1;

	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
		    if (!ISINRECT(area, iter->location.x, iter->location.y)) {
		        iter->location.x = area.right;
		        iter->location.y = area.top;
		    }
		    if (!ISINRECT(area, iter->area.left, iter->area.bottom)) {
		        iter->area.left = area.right;
		        iter->area.bottom = area.top;
		    }
		    if (!ISINRECT(area, iter->area.right, iter->area.top)) {
		        iter->area.right = area.right;
		        iter->area.top = area.top;
		    }
		    //if (iter->location.x == 123 && iter->location.y == 123) {
		    //    iter->location.x = -1;
		    //    iter->location.y = -1;
		    //}
		    //if (iter->area.left == 123 && iter->area.bottom == 123) {
		    //    iter->area.left = -1;
		    //    iter->area.bottom = -1;
		    //}
		    //if (iter->area.right == 123 && iter->area.top == 123) {
		    //    iter->area.right = -1;
		    //    iter->area.top = -1;
		    //}
		}
	    // conditions
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
		    if (!ISINRECT(area, iter->area.left, iter->area.bottom)) {
		        iter->area.left = area.right;
		        iter->area.bottom = area.top;
		    }
		    if (!ISINRECT(area, iter->area.right, iter->area.top)) {
		        iter->area.right = area.right;
		        iter->area.top = area.top;
		    }
		    //if (iter->area.left == 123 && iter->area.bottom == 123) {
		    //    iter->area.left = -1;
		    //    iter->area.bottom = -1;
		    //}
		    //if (iter->area.right == 123 && iter->area.top == 123) {
		    //    iter->area.right = -1;
		    //    iter->area.top = -1;
		    //}
		    //if (iter->type == 1 && ((trig->display_order >= 164 && trig->display_order <= 181) || (trig->display_order >= 193 && trig->display_order <= 324))) {
		    //    iter->area.right = 123;
		    //    iter->area.top = 123;
		    //}
		    //if (iter->area.left == 123 && iter->area.bottom == 123) {
		    //    iter->area.left = -1;
		    //    iter->area.bottom = -1;
		    //}
		}
		trig++;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::compress_unit_ids()
{
    // triggers
	long newu = 0;
	long oldu;

	// create array for the unit
	long numunits = 0;
	for (int i = 0; i < NUM_PLAYERS; i++) {
	    numunits += players[i].units.size();
	}
    std::vector<std::pair<int, int>> unittrans;
    unittrans.reserve(numunits);

	newu = 0;
    // each player i
	for (int i = 0; i < NUM_PLAYERS; i++) {
        // units
	    for (std::vector<Unit>::iterator unit = players[i].units.begin(); unit != players[i].units.end(); ++unit, ++newu) {

	        oldu = unit->ident;

            // don't do anything for units with negative ids
            if (oldu >= 0) {
	            unittrans.push_back(std::make_pair(unit->ident, newu));
	        } else {
                newu--;
            }
	    }
	}

	next_uid = newu;

    // each player i
	for (int i = 0; i < NUM_PLAYERS; i++) {
        // each unit
	    for (std::vector<Unit>::iterator unit = players[i].units.begin(); unit != players[i].units.end(); ++unit) {
	        // update ids including duplicates
	        for (std::vector<std::pair<int, int>>::iterator unitt = unittrans.begin(); unitt != unittrans.end(); ++unitt) {
	            if (unit->ident == unitt->first) {
	                unit->ident = unitt->second;
	                break;
	            }
	        }
	        // update garrisons
	        for (std::vector<std::pair<int, int>>::iterator unitt = unittrans.begin(); unitt != unittrans.end(); ++unitt) {
	            // != 0 check is for AoK. default was 0 not -1.
	            if (unit->garrison != 0 && unit->garrison == unitt->first) {
	                unit->garrison = unitt->second;
	                break;
	            }
	        }
	    }
	}

    // each triggers
	for (vector<Trigger>::iterator trig = triggers.begin(); trig != triggers.end(); ++trig) {
	    // each effect
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        // each unit trans
	        for (std::vector<std::pair<int, int>>::iterator unitt = unittrans.begin(); unitt != unittrans.end(); ++unitt) {
	            // update uid_loc
	            if (iter->uid_loc == unitt->first) {
	                iter->uid_loc = unitt->second;
	                break;
	            }
	        }
	    }
	    // each effect
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        // each uid
	        for (int u = 0; u < iter->num_sel; u++) {
	            // each unit trans
	            for (std::vector<std::pair<int, int>>::iterator unitt = unittrans.begin(); unitt != unittrans.end(); ++unitt) {
	                // update uid
	                if (iter->uids[u] == unitt->first) {
	                    iter->uids[u] = unitt->second;
	                    break;
	                }
	            }
		    }
		}
	    // conditions
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
	        // each unit trans
	        for (std::vector<std::pair<int, int>>::iterator unitt = unittrans.begin(); unitt != unittrans.end(); ++unitt) {
	            // update u_loc
	            if (iter->u_loc == unitt->first) {
	                iter->u_loc = unitt->second;
	                break;
	            }
	        }
	        // each unit trans
	        for (std::vector<std::pair<int, int>>::iterator unitt = unittrans.begin(); unitt != unittrans.end(); ++unitt) {
	            // update object
	            if (iter->object == unitt->first) {
	                iter->object = unitt->second;
	                break;
	            }
		    }
        }
	}

	return ERR_none;
}

/*
 * This needs a function and not a value because some combinations of
 * active players are invalid
 */
int Scenario::get_number_active_players() {
    bool counting_up = true;
	int num_players = 0;
	int i;
	Player * p;
	for (i = 0, p = scen.players; i < 8; i++, p++) {
	    if (p->enable && counting_up) {
	        num_players++;
	    } else if (!p->enable && counting_up) {
	        counting_up = false;
	    } else if (p->enable) {
	        return ERR_combination;
	    }
	}
	return num_players;
}

AOKTS_ERROR Scenario::set_number_active_players(int num) {
	for (Player * p = scen.players; p < scen.players + 8; p++) {
	    p->enable = p < scen.players + num;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::set_unit_z_to_map_elev()
{
    // each player
	for (int i = 0; i < NUM_PLAYERS; i++) {
        // units
        vector<Unit>::iterator end = players[i].units.end();
	    for (vector<Unit>::iterator iter = players[i].units.begin(); iter != end; ++iter) {
	        if ((int)iter->x >= 0 && (int)iter->x < map.x && (int)iter->y >= 0 && (int)iter->y < map.y) {
	            iter->z = map.terrain[(int)iter->x][(int)iter->y].elev;
	        }
	    }
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::change_unit_type_for_all_of_type(UCNST from_type, UCNST to_type)
{
    // each player
	for (int i = 0; i < NUM_PLAYERS; i++) {
        players[i].change_unit_type_for_all_of_type(from_type, to_type);
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::clearaicpvc()
{
	for (int i = 0; i < NUM_PLAYERS; i++) {
	    players[i].clear_ai();
	    players[i].clear_cty();
	    players[i].clear_vc();
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::randomize_unit_frames()
{
    // compiler will set first array element to the value you've
    // provided (0) and all others will be set to zero because it is a
    // default value for omitted array elements.
    // can't do this. need to use compiler constants for sizes. also {0}
    // doesn't work in vc++ 2005
    //unsigned char lookup[perversion->max_unit][2] = {0};
    //map<unsigned char, unsigned char> lookup;
    vector< pair<short,float>> lookup(pergame->max_unit, std::make_pair(0,0.0F));

    UCNST cunitid;
    // each player
	for (int i = 0; i < NUM_PLAYERS; i++) {
        // units
        vector<Unit>::iterator end = players[i].units.end();
        for (vector<Unit>::iterator iter = players[i].units.begin(); iter != end; ++iter) {
	        cunitid = iter->getType()->id();
	        if (cunitid >= 0 && cunitid < pergame->max_unit){
	            if (iter->frame > lookup.at(cunitid).first) {
	                lookup.at(cunitid).first = iter->frame;
	            }
	            if (iter->rotate > lookup.at(cunitid).second) {
	                lookup.at(cunitid).second = iter->rotate;
	            }
	        } else {
	            //MessageBox(dialog, toString<int>(cunitid).c_str(), "", MB_ICONERROR);
	        }
	    }
	}
    // each player
	for (int i = 0; i < NUM_PLAYERS; i++) {
        // units
        vector<Unit>::iterator end = players[i].units.end();
	    for (vector<Unit>::iterator iter = players[i].units.begin(); iter != end; ++iter) {
	        cunitid = iter->getType()->id();
	        // 264 - 272 are cliffs (avoid) when game is aoe2
	        if (cunitid >= 0 && cunitid < pergame->max_unit && !((ver1 == SV1_AOK || ver1 == SV1_AOC_SWGB) && cunitid >= 264 && cunitid <= 272)){
                iter->frame = (short)(rand() % (lookup.at(cunitid).first + 1));
                iter->rotate = (float)(rand() % (int)(lookup.at(cunitid).second / (float)PI * 4 + 1)) / 4 * (float)PI;
            }
	    }
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::water_cliffs_visibility(const bool visibility)
{
    LONG numunits = players[8].units.size();
    Unit * u;
    Map::Terrain * t;
	for (int j = 0; j < numunits; j++) {
	    if (isAOE2(game)) {
	        // 264 - 272 are cliffs (avoid) when game is aoe2
	        u = &(players[8].units.at(j));
	        if (u->getType()->id() >= 264 && players[8].units.at(j).getType()->id() <= 273) {
                t = &map.terrain[(int)floor(u->x)][(int)floor(u->y)];
                switch (t->cnst) {
                case 1:
                case 4:
                case 22:
                case 23:
                    if (visibility) {
                        u->rotate = 0;
                        u->frame = 0;
                    } else {
                        u->rotate = 1;
                        u->frame = 1;
                    }
                    break;
                }
	        }
	    }
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::randomize_unit_frames(const unsigned int cnst)
{
    switch (cnst)
    {
    case 349: // Forest, Oak
    case 351: // Forest, Palm
        LONG numunits = players[8].units.size();
	    for (int j = 0; j < numunits; j++) {
	        if (players[8].units.at(j).getType()->id() == cnst) {
                players[8].units.at(j).rotate = (float)(rand() % (int)(8));
                players[8].units.at(j).frame = (short)(rand() % (int)(13));
	        }
	    }
        break;
        /*
    default:
        // each player
	    for (int i = 0; i < NUM_PLAYERS; i++) {
            // units
            LONG numunits = players[i].units.size();
	        for (int j = 0; j < numunits; j++) {
	            if (players[i].units.at(j).getType()->id() == cnst) {
                    players[i].units.at(j).rotate = (float)(rand() % (int)(8));
                    players[i].units.at(j).frame = (short)(rand() % (int)(13));
	            }
	        }
	    }
	    */
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::map_duplicate(const RECT &from, const POINT &to, OpFlags::Value flags)
{
	LONG dx, dy, w, h, truew, trueh;

	dx = to.x - from.left;
	dy = to.y - from.bottom;
	w = from.right - from.left;
	h = from.top - from.bottom;
    // +1 because right - left == 0 represents 1 tile
	truew = w + 1;
	trueh = h + 1;

	RECT truefrom;
	truefrom.left = from.left;
	truefrom.bottom = from.bottom;
	truefrom.right = from.left + truew;
	truefrom.top = from.bottom + trueh;

	RECT trueto;
	trueto.left = to.x;
	trueto.right = to.x + truew;
	trueto.bottom = to.y;
	trueto.top = to.y + trueh;

	RECT areato;
	areato.left = to.x;
	areato.right = to.x + w;
	areato.bottom = to.y;
	areato.top = to.y + h;

    if (flags & OpFlags::UNITS){
        // each player
	    for (int i = 0; i < NUM_PLAYERS; i++) {
            // units
            vector<Unit>::iterator end = players[i].units.end();
            LONG numunits = players[i].units.size();
	        for (int j = 0; j < numunits; j++) {
                if (ISINRECT(truefrom, players[i].units.at(j).x, players[i].units.at(j).y)) {
	                Unit spec(players[i].units.at(j));
	                spec.ident = scen.next_uid++;
		            spec.x += dx;
		            spec.y += dy;
		            players[i].units.push_back(spec);
		        }
	        }
	    }
	}

	dx = to.x - truefrom.left;
	dy = to.y - truefrom.bottom;

    LONG cw = truefrom.right - truefrom.left;
    LONG ch = truefrom.top - truefrom.bottom;

	RECT torect;
	torect.left = to.x;
	torect.right = to.x + cw;
	torect.bottom = to.y;
	torect.top = to.y + ch;

    if ((flags & OpFlags::TERRAIN) || (flags & OpFlags::ELEVATION)){
        Map::Terrain blank;
	    blank.cnst=0;
	    blank.elev=0;

	    /* perform some validation on input */
	    if (!(truefrom.bottom < 0 || static_cast<unsigned>(truefrom.top) > map.y ||
		    truefrom.left < 0 || static_cast<unsigned>(truefrom.right) > map.x ||
	        torect.bottom < 0 || static_cast<unsigned>(torect.top) > map.y ||
		    torect.left < 0 || static_cast<unsigned>(torect.right) > map.x)) {

            // temporarily store the area to move before copying
            vector<vector<Map::Terrain>> frombuffer(cw, vector<Map::Terrain>(ch));
	        for (LONG i = 0; i < cw; i++) {
		        for (LONG j = 0; j < ch; j++) {
		            frombuffer[i][j] = map.terrain[truefrom.left + i][truefrom.bottom + j];
		        }
	        }

            if (flags & OpFlags::TERRAIN){
	            for (LONG i = 0; i < cw; i++) {
		            for (LONG j = 0; j < ch; j++) {
		                map.terrain[torect.left + i][torect.bottom + j].cnst = frombuffer[i][j].cnst;
		            }
	            }
	        }

            if (flags & OpFlags::ELEVATION){
	            for (LONG i = 0; i < cw; i++) {
		            for (LONG j = 0; j < ch; j++) {
		                map.terrain[torect.left + i][torect.bottom + j].elev = frombuffer[i][j].elev;
		            }
	            }
	        }
	    }
	}


	return ERR_none;
}

AOKTS_ERROR Scenario::map_delete(const RECT &from, const POINT &to, OpFlags::Value flags)
{
	LONG dx, dy, w, h, truew, trueh;

	dx = to.x - from.left;
	dy = to.y - from.bottom;
	w = from.right - from.left;
	h = from.top - from.bottom;
    // +1 because right - left == 0 represents 1 tile
	truew = w + 1;
	trueh = h + 1;

	RECT truefrom;
	truefrom.left = from.left;
	truefrom.bottom = from.bottom;
	truefrom.right = from.left + truew;
	truefrom.top = from.bottom + trueh;

	RECT trueto;
	trueto.left = to.x;
	trueto.right = to.x + truew;
	trueto.bottom = to.y;
	trueto.top = to.y + trueh;

	RECT areato;
	areato.left = to.x;
	areato.right = to.x + w;
	areato.bottom = to.y;
	areato.top = to.y + h;

    // each player
	for (int i = 0; i < NUM_PLAYERS; i++) {
        if (flags & OpFlags::UNITS){
            players[i].erase_unit_area(truefrom);
	    }
	}

	return ERR_none;


    if (!(flags & OpFlags::TRIGGERS))
	    return ERR_none;

	Trigger *trig = &(*triggers.begin());
	long num = triggers.size();

    // triggers
	long i = num;
	while (i--)
	{
	    // effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
		    if (ISINRECT(from, iter->location.x, iter->location.y)) {
		        iter->location.x += dx;
		        iter->location.y += dy;
		    } else if (ISINRECT(areato, iter->location.x, iter->location.y)) {
		        iter->location.x -= dx;
		        iter->location.y -= dy;
		    }
		    if (ISINRECT(from, iter->area.right, iter->area.top)) {
		        iter->area.right += dx;
		        iter->area.top += dy;
		    } else if (ISINRECT(areato, iter->area.right, iter->area.top)) {
		        iter->area.right -= dx;
		        iter->area.top -= dy;
		    }
		    if (ISINRECT(from, iter->area.left, iter->area.bottom)) {
		        iter->area.left += dx;
		        iter->area.bottom += dy;
		    } else if (ISINRECT(areato, iter->area.left, iter->area.bottom)) {
		        iter->area.left -= dx;
		        iter->area.bottom -= dy;
		    }
		}
	    // conditions
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
		    if (ISINRECT(from, iter->area.right, iter->area.top)) {
		        iter->area.right += dx;
		        iter->area.top += dy;
		    } else if (ISINRECT(areato, iter->area.right, iter->area.top)) {
		        iter->area.right -= dx;
		        iter->area.top -= dy;
		    }
		    if (ISINRECT(from, iter->area.left, iter->area.bottom)) {
		        iter->area.left += dx;
		        iter->area.bottom += dy;
		    } else if (ISINRECT(areato, iter->area.left, iter->area.bottom)) {
		        iter->area.left -= dx;
		        iter->area.bottom -= dy;
		    }
		}
		trig++;
	}

	return ERR_none;
}

AOKTS_ERROR Scenario::sort_conds_effects()
{
	Trigger *trig = &(*triggers.begin());
	long num = triggers.size();

    // triggers
	long i = num;
	while (i--)
	{
		trig->sort_effects();
		trig->sort_conditions();
		trig++;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::instructions_panel_set(long value) {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        if (iter->type == EffectType::DisplayInstructions) {
	            iter->panel = value;
	        }
		}
		trig++;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::instructions_sound_id_set(long value) {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        if (iter->type == EffectType::DisplayInstructions) {
	            iter->soundid = value;
	        }
		}
		trig++;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::instructions_sound_text_set() {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        if (iter->type == EffectType::DisplayInstructions) {
	            iter->sound.erase();
	        }
		}
		trig++;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::instructions_string_zero() {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        if (iter->type == EffectType::DisplayInstructions) {
	            iter->textid = 0;
	        }
		}
		trig++;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::instructions_string_reset() {
	long num = triggers.size();
	if (num < 1)
	    return ERR_none;
	Trigger *trig = &(*triggers.begin());

    // triggers
	long i = num;
	while (i--)
	{
	    // effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        if (iter->type == EffectType::DisplayInstructions) {
	            iter->textid = -1;
	        }
		}
		trig++;
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::map_change_elevation(const RECT &target, int adjustment)
{
	/* perform some validation on input */
	if (!(target.bottom < 0 || static_cast<unsigned>(target.top) > map.y ||
		        target.left < 0 || static_cast<unsigned>(target.right) > map.x)) {

	    // check no tiles will go beyond bounds
	    bool isok = true;
	    int tempelev = 0;
	    for (LONG i = target.left; i <= target.right; i++) {
		    for (LONG j = target.bottom; j <= target.top; j++) {
		        tempelev = map.terrain[i][j].elev + adjustment;
		        if (tempelev < 0 || tempelev > 255)
		            isok = false;
		    }
	    }

	    if (isok) {
	        for (LONG i = target.left; i <= target.right; i++) {
		        for (LONG j = target.bottom; j <= target.top; j++) {
		            map.terrain[i][j].elev += adjustment;
		        }
	        }
	    } else {
	    }
	}

	return ERR_none;
}

// Remember the RANDOMIZE flag
AOKTS_ERROR Scenario::map_repeat(const RECT &target, const POINT &source, OpFlags::Value flags)
{
	LONG bdx, bdy, w, h, truew, trueh;

	bdx = target.left - source.x;
	bdy = target.bottom - source.y;
	w = target.right - target.left;
	h = target.top - target.bottom;
    // +1 because right - left == 0 represents 1 tile
	truew = w + 1;
	trueh = h + 1;
	RECT truetarget;
	truetarget.left = target.left;
	truetarget.bottom = target.bottom;
	truetarget.right = target.left + truew;
	truetarget.top = target.bottom + trueh;

	RECT truesource;
	truesource.left = source.x;
	truesource.right = source.x + 1;
	truesource.bottom = source.y;
	truesource.top = source.y + 1;

    LONG cw = truetarget.right - truetarget.left;
    LONG ch = truetarget.top - truetarget.bottom;

	//RECT sourcerect;
	//sourcerect.left = source.x;
	//sourcerect.right = source.x + cw;
	//sourcerect.bottom = source.y;
	//sourcerect.top = source.y + ch;

    if (flags & OpFlags::UNITS){
        // each player
	    for (int i_p = 0; i_p < NUM_PLAYERS; i_p++) {
            // units
            vector<Unit>::iterator end = players[i_p].units.end();
            LONG numunits = players[i_p].units.size();
	        for (int i_u = 0; i_u < numunits; i_u++) {
		        Unit & ou = players[i_p].units.at(i_u);
                if (ISINRECT(truesource, ou.x, ou.y)) {
		            int ox = truncate(ou.x);
		            int oy = truncate(ou.y);
	                for (LONG dx = 0; dx < cw; dx++) {
		                for (LONG dy = 0; dy < ch; dy++) {
		                    int dz = scen.map.terrain[bdx + ox + dx][bdy + oy + dy].elev - scen.map.terrain[ox][oy].elev;
	                        Unit spec(ou);
	                        spec.ident = scen.next_uid++;
		                    spec.x += bdx + dx;
		                    spec.y += bdy + dy;
		                    spec.z += dz;
		                    players[i_p].units.push_back(spec);
		                }
		            }
		        }
	        }
	    }
	}

    if (flags & OpFlags::TERRAIN){
	    for (LONG i = target.left; i <= target.right; i++) {
		    for (LONG j = target.bottom; j <= target.top; j++) {
		        map.terrain[i][j].cnst = map.terrain[source.x][source.y].cnst;
		    }
		}
	}

    if (flags & OpFlags::ELEVATION){
	    for (LONG i = target.left; i <= target.right; i++) {
		    for (LONG j = target.bottom; j <= target.top; j++) {
		        map.terrain[i][j].elev = map.terrain[source.x][source.y].elev;
		    }
		}
	}
	return ERR_none;
}

AOKTS_ERROR Scenario::map_move(const RECT &from, const POINT &to, OpFlags::Value flags)
{
	LONG dx, dy, w, h, truew, trueh;

	dx = to.x - from.left;
	dy = to.y - from.bottom;
	w = from.right - from.left;
	h = from.top - from.bottom;
    // +1 because right - left == 0 represents 1 tile
	truew = w + 1;
	trueh = h + 1;

	RECT truefrom;   // would more appropriately be caled "unit from" as opposed to "trigger from" as triggers are integers
	truefrom.left = from.left;
	truefrom.bottom = from.bottom;
	truefrom.right = from.left + truew;
	truefrom.top = from.bottom + trueh;

    if ((flags & OpFlags::TERRAIN) || (flags & OpFlags::ELEVATION)){
        Map::Terrain blank;
	    blank.cnst=0;
	    blank.elev=0;

	    LONG dx = 0, dy = 0;
	    dx = to.x - truefrom.left;
	    dy = to.y - truefrom.bottom;

        LONG cw = truefrom.right - truefrom.left;
        LONG ch = truefrom.top - truefrom.bottom;

	    RECT torect;
	    torect.left = to.x;
	    torect.right = to.x + cw;
	    torect.bottom = to.y;
	    torect.top = to.y + ch;

	    //xstep = dx/abs(dx);
	    //ystep = dy/abs(dy);

	    /* perform some validation on input */
	    if (!(truefrom.bottom < 0 || static_cast<unsigned>(truefrom.top) > map.y ||
		    truefrom.left < 0 || static_cast<unsigned>(truefrom.right) > map.x ||
	        torect.bottom < 0 || static_cast<unsigned>(torect.top) > map.y ||
		    torect.left < 0 || static_cast<unsigned>(torect.right) > map.x)) {

            // temporarily store the area to move before overwriting
            vector<vector<Map::Terrain>> frombuffer(cw, vector<Map::Terrain>(ch));
	        for (LONG i = 0; i < cw; i++) {
		        for (LONG j = 0; j < ch; j++) {
		            frombuffer[i][j] = map.terrain[truefrom.left + i][truefrom.bottom + j];
		        }
	        }

            // temporarily store the destination terrain before overwriting
            vector<vector<Map::Terrain>> destbuffer(cw, vector<Map::Terrain>(ch));
	        for (LONG i = 0; i < cw; i++) {
		        for (LONG j = 0; j < ch; j++) {
		            destbuffer[i][j] = map.terrain[torect.left + i][torect.bottom + j];
		        }
	        }

            if (flags & OpFlags::TERRAIN){
	            // gets priority
	            for (LONG i = 0; i < cw; i++) {
		            for (LONG j = 0; j < ch; j++) {
		                map.terrain[torect.left + i][torect.bottom + j].cnst = frombuffer[i][j].cnst;
		            }
	            }
	        }

            if (flags & OpFlags::ELEVATION){
	            // gets priority
	            for (LONG i = 0; i < cw; i++) {
		            for (LONG j = 0; j < ch; j++) {
		                map.terrain[torect.left + i][torect.bottom + j].elev = frombuffer[i][j].elev;
		            }
	            }
	        }
	    }
    }

    // each player
	for (int i = 0; i < NUM_PLAYERS; i++) {
        if (flags & OpFlags::TERRAIN){
            // camera
            if (ISINRECT(truefrom, players[i].camera[0], players[i].camera[0])) {
                players[i].camera[0] += dx;
                players[i].camera[1] += dy;
            }
        }

        if (flags & OpFlags::UNITS){
            // units
	        for (vector<Unit>::iterator iter = players[i].units.begin(); iter != players[i].units.end(); ++iter) {
                if (ISINRECT(truefrom, iter->x, iter->y)) {
		            iter->x += dx;
		            iter->y += dy;
		        }
	        }
	    }
	}

    if (!(flags & OpFlags::TRIGGERS))
	    return ERR_none;

	Trigger *trig = &(*triggers.begin());
	long num = triggers.size();

    // triggers
	long i = num;
	while (i--)
	{
	    // effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
		    if (ISINRECT(from, iter->location.x, iter->location.y)) {
		        iter->location.x += dx;
		        iter->location.y += dy;
		    }
		    if (ISINRECT(from, iter->area.right, iter->area.top)) {
		        iter->area.right += dx;
		        iter->area.top += dy;
		    }
		    if (ISINRECT(from, iter->area.left, iter->area.bottom)) {
		        iter->area.left += dx;
		        iter->area.bottom += dy;
		    }
		}
	    // conditions
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
		    if (ISINRECT(from, iter->area.right, iter->area.top)) {
		        iter->area.right += dx;
		        iter->area.top += dy;
		    }
		    if (ISINRECT(from, iter->area.left, iter->area.bottom)) {
		        iter->area.left += dx;
		        iter->area.bottom += dy;
		    }
		}
		trig++;
	}

	return ERR_none;
}

AOKTS_ERROR Scenario::map_swap(const RECT &from, const POINT &to, OpFlags::Value flags)
{
	LONG dx, dy, w, h, truew, trueh;

	dx = to.x - from.left;
	dy = to.y - from.bottom;
	w = from.right - from.left;
	h = from.top - from.bottom;
    // +1 because right - left == 0 represents 1 tile
	truew = w + 1;
	trueh = h + 1;

	RECT truefrom;
	truefrom.left = from.left;
	truefrom.bottom = from.bottom;
	truefrom.right = from.left + truew;
	truefrom.top = from.bottom + trueh;

	RECT trueto;
	trueto.left = to.x;
	trueto.right = to.x + truew;
	trueto.bottom = to.y;
	trueto.top = to.y + trueh;

	RECT areato;
	areato.left = to.x;
	areato.right = to.x + w;
	areato.bottom = to.y;
	areato.top = to.y + h;

    if ((flags & OpFlags::TERRAIN) || (flags & OpFlags::ELEVATION)){
        Map::Terrain blank;
	    blank.cnst=0;
	    blank.elev=0;

	    LONG dx = 0, dy = 0;
	    dx = to.x - truefrom.left;
	    dy = to.y - truefrom.bottom;

        LONG cw = truefrom.right - truefrom.left;
        LONG ch = truefrom.top - truefrom.bottom;

	    RECT torect;
	    torect.left = to.x;
	    torect.right = to.x + cw;
	    torect.bottom = to.y;
	    torect.top = to.y + ch;

	    //xstep = dx/abs(dx);
	    //ystep = dy/abs(dy);

	    /* perform some validation on input */
	    if (!(truefrom.bottom < 0 || static_cast<unsigned>(truefrom.top) > map.y ||
		    truefrom.left < 0 || static_cast<unsigned>(truefrom.right) > map.x ||
	        torect.bottom < 0 || static_cast<unsigned>(torect.top) > map.y ||
		    torect.left < 0 || static_cast<unsigned>(torect.right) > map.x)) {

            // temporarily store the area to move before overwriting
            vector<vector<Map::Terrain>> frombuffer(cw, vector<Map::Terrain>(ch));
	        for (LONG i = 0; i < cw; i++) {
		        for (LONG j = 0; j < ch; j++) {
		            frombuffer[i][j] = map.terrain[truefrom.left + i][truefrom.bottom + j];
		        }
	        }

            // temporarily store the destination terrain before overwriting
            vector<vector<Map::Terrain>> destbuffer(cw, vector<Map::Terrain>(ch));
	        for (LONG i = 0; i < cw; i++) {
		        for (LONG j = 0; j < ch; j++) {
		            destbuffer[i][j] = map.terrain[torect.left + i][torect.bottom + j];
		        }
	        }

            if (flags & OpFlags::TERRAIN){
                // swap
	            for (LONG i = 0; i < cw; i++) {
		            for (LONG j = 0; j < ch; j++) {
		                map.terrain[truefrom.left + i][truefrom.bottom + j].cnst = destbuffer[i][j].cnst;
		            }
	            }

	            // gets priority
	            for (LONG i = 0; i < cw; i++) {
		            for (LONG j = 0; j < ch; j++) {
		                map.terrain[torect.left + i][torect.bottom + j].cnst = frombuffer[i][j].cnst;
		            }
	            }
	        }

            if (flags & OpFlags::ELEVATION){
                // swap
	            for (LONG i = 0; i < cw; i++) {
		            for (LONG j = 0; j < ch; j++) {
		                map.terrain[truefrom.left + i][truefrom.bottom + j].elev = destbuffer[i][j].elev;
		            }
	            }

	            // gets priority
	            for (LONG i = 0; i < cw; i++) {
		            for (LONG j = 0; j < ch; j++) {
		                map.terrain[torect.left + i][torect.bottom + j].elev = frombuffer[i][j].elev;
		            }
	            }
	        }
	    }
    }

    // each player
	for (int i = 0; i < NUM_PLAYERS; i++) {
        if (flags & OpFlags::TERRAIN){
            // camera
            if (ISINRECT(truefrom, players[i].camera[0], players[i].camera[0])) {
                players[i].camera[0] += dx;
                players[i].camera[1] += dy;
            }
        }

        if (flags & OpFlags::UNITS){
            // units
	        for (vector<Unit>::iterator iter = players[i].units.begin(); iter != players[i].units.end(); ++iter) {
                if (ISINRECT(truefrom, iter->x, iter->y)) {
		            iter->x += dx;
		            iter->y += dy;
		        } else if (ISINRECT(trueto, iter->x, iter->y)) {
		            iter->x -= dx;
		            iter->y -= dy;
		        }
	        }
	    }
	}

    if (!(flags & OpFlags::TRIGGERS))
	    return ERR_none;

	Trigger *trig = &(*triggers.begin());
	long num = triggers.size();

    // triggers
	long i = num;
	while (i--)
	{
	    // effects
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
		    if (ISINRECT(from, iter->location.x, iter->location.y)) {
		        iter->location.x += dx;
		        iter->location.y += dy;
		    } else if (ISINRECT(areato, iter->location.x, iter->location.y)) {
		        iter->location.x -= dx;
		        iter->location.y -= dy;
		    }
		    if (ISINRECT(from, iter->area.right, iter->area.top)) {
		        iter->area.right += dx;
		        iter->area.top += dy;
		    } else if (ISINRECT(areato, iter->area.right, iter->area.top)) {
		        iter->area.right -= dx;
		        iter->area.top -= dy;
		    }
		    if (ISINRECT(from, iter->area.left, iter->area.bottom)) {
		        iter->area.left += dx;
		        iter->area.bottom += dy;
		    } else if (ISINRECT(areato, iter->area.left, iter->area.bottom)) {
		        iter->area.left -= dx;
		        iter->area.bottom -= dy;
		    }
		}
	    // conditions
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
		    if (ISINRECT(from, iter->area.right, iter->area.top)) {
		        iter->area.right += dx;
		        iter->area.top += dy;
		    } else if (ISINRECT(areato, iter->area.right, iter->area.top)) {
		        iter->area.right -= dx;
		        iter->area.top -= dy;
		    }
		    if (ISINRECT(from, iter->area.left, iter->area.bottom)) {
		        iter->area.left += dx;
		        iter->area.bottom += dy;
		    } else if (ISINRECT(areato, iter->area.left, iter->area.bottom)) {
		        iter->area.left -= dx;
		        iter->area.bottom -= dy;
		    }
		}
		trig++;
	}

	return ERR_none;
}

int Scenario::getPlayerCount()
{
	int ret = 0;
	Player *p;
	int i;

	FEP(p)
		ret += p->enable;

	return ret;
}

/* Map */

Map::Terrain::Terrain()
:	cnst(0), elev(1)
{
}

Map::Map()
:	aitype(-1), x(0), y(0)
{
}

void Map::reset()
{
	Terrain *parse;

	aitype = esdata.aitypes.head()->id();	// pick the first one
	x = MapSizes[1];
	y = MapSizes[1];

	parse = *terrain;
	for (int i = 0; i < MAX_MAPSIZE * MAX_MAPSIZE; i++)
	{
		parse->cnst = 0;
		parse->elev = 1;
		parse++;
	}
}

void Map::read(FILE *in, ScenVersion1 version)
{
	Terrain *t_parse;
	unsigned int count;

	if (version >= SV1_AOC_SWGB)
		readbin(in, &aitype);

	if (scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6) {
		readbin(in, &unknown1);
		readbin(in, &unknown2);
		readbin(in, &unknown3);
		readbin(in, &unknown4);
	}

	readbin(in, &x);
	readbin(in, &y);
	printf_log("Debug X %lu.\n",x);
	printf_log("Debug Y %lu.\n",y);

	/**
	 * Note that I treat x as the row indices, not the usual col indices,
	 * since this is more natural once the square is rotated into the diamond.
	 */
	for (unsigned int i = 0; i < x; i++)
	{
		count = y;
		t_parse = terrain[i];

		while (count--)
		{
			readbin(in, t_parse++);
			readunk<char>(in, 0, "terrain zero");
		}
	}
}

void Map::write(FILE *out, ScenVersion1 version)
{
	unsigned int count;
	Terrain *t_parse;

	if (scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6) {
		fwrite(&aitype, sizeof(aitype), 1, out);
		fwrite(&unknown1, sizeof(unknown1), 1, out);
		fwrite(&unknown2, sizeof(unknown2), 1, out);
		fwrite(&unknown3, sizeof(unknown3), 1, out);
		fwrite(&unknown4, sizeof(unknown4), 1, out);
		fwrite(&x, sizeof(long), 2, out);	//x, y
	} else if (version >= SV1_AOC_SWGB) {
		fwrite(this, sizeof(long), 3, out);	//aitype, x, y
	} else {
	    //there's no aitype in non-TC
		fwrite(&x, sizeof(long), 2, out);	//x, y
	}

	for (unsigned int i = 0; i < x; i++)
	{
		count = y;
		t_parse = terrain[i];

		while (count--)
		{
			fwrite(t_parse++, sizeof(Terrain), 1, out);
			putc(0, out);
		}
	}
}

bool Map::readArea(Buffer &from, const RECT &area)
{
	Terrain *parse;

	for (LONG i = area.left; i <= area.right; i++)
	{
		parse = terrain[i] + area.bottom;

		for (LONG j = area.bottom; j <= area.top; j++)
		{
			from.read(parse, sizeof(*parse));
			parse++;
		}
	}

	return true;
}

bool Map::writeArea(Buffer &b, const RECT &area)
{
	Terrain *parse;

	/* perform some validation on input */
	if (area.bottom < 0 || static_cast<unsigned>(area.top) > y ||
		area.left < 0 || static_cast<unsigned>(area.right) > x)
		return false;

	for (LONG i = area.left; i <= area.right; i++)
	{
		parse = terrain[i] + area.bottom;

		for (LONG j = area.bottom; j <= area.top; j++)
		{
			b.write(parse, sizeof(Terrain));
			parse++;
		}
	}

	return true;
}

bool Map::scaleArea(const RECT &area, const float scale)
{

    return true;
}
