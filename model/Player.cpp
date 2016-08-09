/*
	Player.cpp: contains definitions for the Player class
	
	MODEL?
*/

#include "scen.h"
#include "Player.h"

#include "../util/utilio.h"
#include <algorithm>
#include <functional>

using std::vector;

extern class Scenario scen;

int Player::num_players = 9;

const char *Player::names[] =
{
	"Player 1",
	"Player 2",
	"Player 3",
	"Player 4",
	"Player 5",
	"Player 6",
	"Player 7",
	"Player 8",
	"GAIA",
	"Player 10",
	"Player 11",
	"Player 12",
	"Player 13",
	"Player 14",
	"Player 15",
	"Player 16"
};

Player::Player()
:	stable(-1)
{
}

void Player::reset(unsigned int id)
{
	memset(name, 0, 30);
	stable = -1;
	//enable = human = false;
	civ = 0;
	strcpy(ai, "RandomGame");
	aimode = AI_standard;
	aifile.erase();
	memset(resources, 0, sizeof(resources));
	memset(diplomacy, 0, sizeof(diplomacy));
	if (id < N_PLAYERS) {
	    civ = id + 1;
	    resources[0]=200; // Gold
	    resources[1]=500; // Wood
	    resources[2]=500; // Food
	    resources[3]=100; // Stone
	    pop = 200;
        for (unsigned int i = 0; i < N_PLAYERS; i++) {
            diplomacy[i] = ((i < 4 && id < 4) || (i >=4 && id >=4)) ? DIP_ally : DIP_enemy;
        }
        diplomacy[N_PLAYERS] = DIP_neutral;
	    age = 1;
	}
	if (id <= N_PLAYERS) {
	    color = id;
	}
    if (id == N_PLAYERS) {
        for (unsigned int i = 0; i < N_PLAYERS; i++) {
            diplomacy[i] = DIP_ally;
        }
	    age = 0;
    }
    diplomacy[id] = DIP_enemy;
	ndis_t = ndis_b = ndis_u = 0;
	memset(dis_tech, 0, sizeof(dis_tech));
	memset(dis_unit, 0, sizeof(dis_unit));
	memset(dis_bldg, 0, sizeof(dis_bldg));
	camera[0] = 0;
	camera[1] = 0;
	avictory = false;
	player_number = id;
	ucount = 2.0F;
	units.clear();
}

// functor for finding by UID
class uid_equals
{
public:
	uid_equals(UID uid)
		: _uid(uid)
	{
	}

	bool operator()(const Unit& u)
	{
		return (u.ident == _uid);
	}

private:
	UID _uid;
};

vector<Unit>::size_type Player::find_unit(UID uid) const
{
	vector<Unit>::const_iterator iter =
		std::find_if(units.begin(), units.end(), uid_equals(uid));

	// This will work for end() as well
	return (iter - units.begin());
}

// functor to check if a unit is a type
class unit_is_type : public std::unary_function<const Unit, bool>
{
public:
	unit_is_type(const UCNST type) : _type(type)
	{}

	// I'm a little confused that you can use a reference here, since we tell
	// others we take "const Unit", but it works. Shrug.
	bool operator()(const Unit& u) const
	{
		return u.getType()->id() == _type;
	}

private:
	UCNST _type;
};

void Player::erase_unit_type(UCNST type)
{
    unit_is_type typecheck(type);
	units.erase(remove_if(units.begin(), units.end(), typecheck), units.end());
}

#define ISINRECT(r, x, y) \
	(x >= r.left && x <= r.right && y >= r.bottom && y <= r.top)

// functor to check if a unit is in an area
class unit_in_area : public std::unary_function<const Unit, bool>
{
public:
	unit_in_area(const RECT area) : _area(area)
	{}

	// I'm a little confused that you can use a reference here, since we tell
	// others we take "const Unit", but it works. Shrug.
	bool operator()(const Unit& u) const
	{
		return ISINRECT(_area, u.x, u.y);
	}

private:
	RECT _area;
};

void Player::erase_unit_area(RECT area)
{
    unit_in_area typecheck(area);
	units.erase(remove_if(units.begin(), units.end(), typecheck), units.end());
}

void Player::erase_unit(vector<Unit>::size_type index)
{
	units.erase(units.begin() + index);
}

bool Player::read_aifiles(FILE *in)
{
	vcfile.read(in, sizeof(unsigned long));
	ctyfile.read(in, sizeof(unsigned long));
	aifile.read(in, sizeof(unsigned long));

	return true;
}

void Player::clear_ai()
{
	char *cstr = aifile.unlock(1);
	strcpy(cstr, "");
	aifile.lock();
    aimode = AI_none;
	strcpy(ai, "");
}

void Player::clear_cty()
{
	char *cstr = ctyfile.unlock(1);
	strcpy(cstr, "");
	ctyfile.lock();
	strcpy(cty, "");
}

void Player::clear_vc()
{
	char *cstr = vcfile.unlock(1);
	strcpy(cstr, "");
	vcfile.lock();
	strcpy(cty, "");
}

void Player::read_header_name(FILE * in)
{
	readbin(in, name, sizeof(name));
	//jump to the end of the 256-byte name area
	 if (fseek(in, 0x100 - sizeof(name), SEEK_CUR))
		 throw std::runtime_error("Seek error around player header name.");
}

void Player::read_header_stable(FILE * in)
{
	readbin(in, &stable);
}

struct PD1
{
	long enable; // boolean
	long human;  // boolean
	long civ;
	long unk;
};

void Player::read_data1(FILE * in)
{
	PD1 pd1;

	readbin(in, &pd1);

	enable = (pd1.enable != 0); // convert to bool
	human = (pd1.human != 0);   // convert to bool
	civ = pd1.civ;
	check<long>(pd1.unk, 4, "PlayerData1 unknown");
}

void Player::write_data1(FILE * out)
{
	PD1 pd1 =
	{
		enable, human, civ, 4
	};

	writebin(out, &pd1);
}

void Player::read_aimode(FILE * in)
{
	readbin(in, &aimode);
}

void Player::read_resources(FILE * in)
{
	readbin(in, resources, 6);
}

void Player::read_player_number(FILE * in)
{
	readbin(in, &player_number);
}

void Player::read_diplomacy(FILE * in)
{
	readbin(in, diplomacy, NUM_PLAYERS);
}

void Player::read_ndis_techs(FILE * in)
{
	readbin(in, &ndis_t);
}

void Player::read_dis_techs(FILE * in, const PerVersion& pv)
{
	//readbin(in, dis_tech, MAX_DIS_TECH);
	readbin(in, dis_tech, pv.max_disables1);
}

void Player::read_ndis_units(FILE * in)
{
	readbin(in, &ndis_u);
}

void Player::read_dis_units(FILE * in, const PerVersion& pv)
{
	//readbin(in, dis_unit, MAX_DIS_UNIT);
	readbin(in, dis_unit, pv.max_disables1);
}

void Player::read_ndis_bldgs(FILE * in)
{
	readbin(in, &ndis_b);
}

void Player::read_dis_bldgs(FILE * in, const PerVersion& pv)
{
	//readbin(in, dis_bldg, MAX_DIS_BLDG);
	readbin(in, dis_bldg, pv.max_disables2);
}

/*void Player::read_dis_bldgsx(FILE * in)
{
	readbin(in, &dis_bldgx);
}*/

void Player::read_age(FILE * in)
{
	if (scen.game == AOHD6 || scen.game == AOF6) {
	    readbin(in, &age);
	    age -= 2;
	    readbin(in, &ending_age);
	} else {
        long tmp = 0;
	    readbin(in, &tmp);
	    age = (short)tmp;
	    ending_age = 0;
	}
}

void Player::read_camera_longs(FILE * in)
{
	long value;

	// Player 1 has camera stored as longs, in Y, X order
	readbin(in, &value);
	camera[1] = static_cast<float>(value);
	readbin(in, &value);
	camera[0] = static_cast<float>(value);
}

void Player::read_data4(FILE * in, Game game)
{
	// Read and check duplicate copies of resources.
	readunk(in, static_cast<float>(resources[2]), "food float");
	readunk(in, static_cast<float>(resources[1]), "wood float");
	readunk(in, static_cast<float>(resources[0]), "gold float");
	readunk(in, static_cast<float>(resources[3]), "stone float");
	readunk(in, static_cast<float>(resources[4]), "orex float");
	readunk(in, static_cast<float>(resources[5]), "?? res float");

	if (game >= AOC && game != SWGB && game != SWGBCC)
		readbin(in, &pop);
}

void Player::read_units(FILE *in)
{
	unsigned long num;
	int i;
	Unit u;

	readbin(in, &num);
	if (num)
	{
		// Reserve in advance for efficiency.
		units.reserve(num);

		i = num;
		while (i--)
		{
			u.read(in);
			units.push_back(u);
		}
	}
}

void Player::read_data3(FILE *in, float *view)
{
	short nlen;
	short ndiplomacy;
	long end;

	// just skip the constant name, dunno why it's there
	readbin(in, &nlen);
	SKIP(in, nlen);

	// read camera, or into view if calling func provided
	readbin(in, view ? view : camera, 2);

	readbin(in, &u1);
	readbin(in, &u2);
	readbin(in, &avictory);

	// skip diplomacy stuff
	readbin(in, &ndiplomacy);
	check<short>(ndiplomacy, 9, "PD3 diplomacy count");
	SKIP(in, sizeof(char) * ndiplomacy);	//diplomacy
	SKIP(in, sizeof(long) * ndiplomacy);	//diplomacy2

	readbin(in, &color);

	// now this is the weird stuff
	// 1.0F for AOK and 2.0F for AOC?
	readbin(in, &ucount);

	if (ucount != 1.0F && ucount != 2.0F)
	{
		printf("Unknown PlayerData3 float value %f at %X\n", ucount, ftell(in));
		throw bad_data_error("Unknown PlayerData3 float value");
	}
	//printf("PD3 ucount was %f\n", ucount);

	short unk;
	readbin(in, &unk);
	check<short>(unk, 0, "PD3 unknown count");

	if (ucount == 2.0F)
	{
		readunk<char>(in, 0, "PD3 char-2 1");
		readunk<char>(in, 0, "PD3 char-2 2");
		readunk<char>(in, 0, "PD3 char-2 3");
		readunk<char>(in, 0, "PD3 char-2 4");
		readunk<char>(in, 0, "PD3 char-2 5");
		readunk<char>(in, 0, "PD3 char-2 6");
		readunk<char>(in, 0, "PD3 char-2 7");
		readunk<char>(in, 0, "PD3 char-2 8");
	}

	SKIP(in, 44 * unk); // Grand Theft Empires, AYBABTU

	readunk<char>(in, 0, "PD3 char 1");
	readunk<char>(in, 0, "PD3 char 2");
	readunk<char>(in, 0, "PD3 char 3");
	readunk<char>(in, 0, "PD3 char 4");
	readunk<char>(in, 0, "PD3 char 5");
	readunk<char>(in, 0, "PD3 char 6");
	readunk<char>(in, 0, "PD3 char 7");

	readbin(in, &end);
	//printf("End was %d\n", end);
}

void Player::write_header_name(FILE * out)
{
	size_t len = strlen(name);
	fwrite(name, sizeof(char), len, out);
	NULLS(out, 0x100 - len);
}

void Player::write_header_stable(FILE * out)
{
	writebin(out, &stable);
}

void Player::write_no_units(FILE *out)
{
	// Write count
	unsigned long num = 0;
	fwrite(&num, sizeof(long), 1, out);

	// Write units
	for (std::vector<Unit>::const_iterator iter = units.begin();
		iter != units.end(); ++iter)
		true;
}

void Player::write_units(FILE *out)
{
	// Write count
	unsigned long num = units.size();
	fwrite(&num, sizeof(long), 1, out);

	// Write units
	for (std::vector<Unit>::const_iterator iter = units.begin();
		iter != units.end(); ++iter)
		fwrite(&*iter, Unit::size, 1, out);
}

void Player::write_data3(FILE *out, int me, float *view)
{
	int d_convert[4] = { 2, 3, 1, 4 };
	int i;

	writecs<unsigned short>(out, Player::names[me]);

    // This changed when I saved the scenario using in-game editor without changing anything
	fwrite(
		view ? view : camera,	//let calling func determine view if it wants to
		sizeof(float), 2, out);	//camera[]

	fwrite(&u1, sizeof(short), 2, out);	//u1, u2

	fputc(avictory, out);
	fwrite(&num_players, sizeof(short), 1, out);

#ifndef NOWRITE_D2
	putc(diplomacy[GAIA_INDEX], out);	//GAIA first
	for (i = 0; i < num_players - 1; i++)
		putc(diplomacy[i], out);
#else
	NULLS(out, num_players);
#endif

#ifndef NOWRITE_D3
	NULLS(out, 4);	//gaia diplomacy
	for (i = 0; i < num_players - 1; i++)
	{
		if (i == me)
			fwrite(&d_convert[2], sizeof(long), 1, out);
		else
			fwrite(&d_convert[diplomacy[i]], sizeof(long), 1, out);
	}
#else
	NULLS(out, num_players * sizeof(long));
#endif

	fwrite(&color, 4, 2, out);	//color, ucount
	NULLS(out, 9);
	if (ucount == 1.0F)
	{
		NULLS(out, 4);
	}
	else
	{
		NULLS(out, 8);
		writeval(out, (long)-1);
	}
}

bool Player::import_ai(const char *path)
{
	size_t ai_len = fsize(path);
	char * ai_buf;

	ai_buf = aifile.unlock(ai_len + 1);

	AutoFile ai_in(path, "rb");
	readbin(ai_in.get(), ai_buf, ai_len);
	ai_buf[ai_len] = '\0';	//null-terminate it

	aifile.lock();

	return true;
}

bool Player::import_vc(const char *path)
{
	size_t vc_len = fsize(path);
	char * vc_buf;

	vc_buf = vcfile.unlock(vc_len + 1);

	AutoFile vc_in(path, "rb");
	readbin(vc_in.get(), vc_buf, vc_len);
	vc_buf[vc_len] = '\0';	//null-terminate it

	vcfile.lock();

	return true;
}

bool Player::import_cty(const char *path)
{
	size_t cty_len = fsize(path);
	char * cty_buf;

	cty_buf = ctyfile.unlock(cty_len + 1);

	AutoFile cty_in(path, "rb");
	readbin(cty_in.get(), cty_buf, cty_len);
	cty_buf[cty_len] = '\0';	//null-terminate it

	ctyfile.lock();

	return true;
}

bool Player::export_ai(const char *path)
{
	FILE *ai_out;

	//check that player actually has an ai script.
	if (aifile.length() == 0)
		return false;

	ai_out = fopen(path, "wb");
	if (!ai_out)
		return false;

	fputs(aifile.c_str(), ai_out);

	fclose(ai_out);
	return true;
}

bool Player::export_vc(const char *path)
{
	FILE *vc_out;

	//check that player actually has a vc script.
	if (vcfile.length() == 0)
		return false;

	vc_out = fopen(path, "wb");
	if (!vc_out)
		return false;

	fputs(vcfile.c_str(), vc_out);

	fclose(vc_out);
	return true;
}

bool Player::export_cty(const char *path)
{
	FILE *cty_out;

	//check that player actually has a vc script.
	if (ctyfile.length() == 0)
		return false;

	cty_out = fopen(path, "wb");
	if (!cty_out)
		return false;

	fputs(ctyfile.c_str(), cty_out);

	fclose(cty_out);
	return true;
}

void Player::add_unit(Unit& uspec)
{
	//Unit u(scen.next_uid++);
	Unit u(uspec);
	u.x = uspec.x;
	u.y = uspec.y;
	u.z = uspec.z;
	u.rotate = uspec.rotate;
	u.frame = uspec.frame;
	u.garrison = uspec.garrison;
	u.setType(uspec.getType());
	units.push_back(u);
}

void Player::add_unit(Unit * uspec)
{
	units.push_back(*uspec);
}

void Player::change_unit_type_for_all_of_type(UCNST from_type, UCNST to_type)
{
	for (vector<Unit>::iterator iter = units.begin(); iter != units.end(); ++iter) {
	    printf_log("unit type %d %d %d\n", iter->getTypeID(), from_type, to_type);
	    if (iter->getTypeID() == from_type) {
	        printf_log("changing type\n");
	        iter->setType(to_type);
	    }
	}
}
