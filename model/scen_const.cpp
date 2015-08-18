/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	scen_const.cpp -- Defines various scenario constants in scen.h

	MODEL
**/

#include "scen_const.h"

size_t MapSizes[] =
{
	120,	//Tiny
	144,	//Small
	168,	//Medium
	200,	//Normal
	220,	//Large
	240,	//Giant
	255,	//Max
	480		//Ludakris
};

size_t Elevations[] =
{
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8
};

const char *message_names[] =
{
	"Instructions", "Hints", "Victory", "Loss", "History", "Scouts"
};

const char *cinem_names[] =
{
	"Pre-Game Cinematic", "Victory Cinematic", "Loss Cinematic", "Instruction Background"
};

const char *difficulties_18[] =
{
	"Easiest", "Easy", "Moderate", "Hard", "Hardest"
};

const char *difficulties_21[] =
{
	"Easy", "Standard", "Moderate", "Hard", "Hardest"
};

const struct PAIR ages[] =
{
	{ -1, "" },
	{  0, "Dark" },
	{  1, "Feudal" },
	{  2, "Castle" },
	{  3, "Imperial" },
	{  4, "Post-Imperial" }
};

const struct PAIR ages_hd6[] =
{
	{ -1, "Feudal" },
	{  0, "Dark" },
	{  1, "Feudal" },
	{  2, "Castle" },
	{  3, "Castle" },
	{  4, "Castle" },
	{  5, "Imperial" },
	{  6, "Post-Imperial" }
};

const struct PAIR groups[] =
{
	{ -1, "None" },
	{  0, "Archer" },
	{  1, "Artifact" },
	{  2, "Trade Boat" },
	{  3, "Building" },
	{  4, "Civilian" },
	{  5, "Sea Fish" },
	{  6, "Infantry" },
	{  7, "Forage Bush" },
	{  8, "Stone Mine" },
	{  9, "Prey Animal" },
	{ 10, "Predator Animal" },
	{ 11, "Other/Dead/Projectile" },
	{ 12, "Cavalry" },
	{ 13, "Siege Weapon" },
	{ 14, "Map Decoration" },
	{ 15, "Tree" },
	{ 16, "?16" }, //Tree Stump
	{ 17, "?17" },
	{ 18, "Priest" },
	{ 19, "Trade Cart" },
	{ 20, "Transport Boat" },
	{ 21, "Fishing Boat" },
	{ 22, "War Boat" },
	{ 23, "Conquistador" },
	{ 24, "Monk" }, //War Elephant
	{ 25, "?25" }, //Hero
	{ 26, "?26" }, //Elephant Archer
	{ 27, "Wall" },
	{ 28, "?28" }, //Phalanx
	{ 29, "?29" },
	{ 30, "Flag" },
	{ 31, "?31" },
	{ 32, "Gold Mine" },
	{ 33, "Shore Fish" },
	{ 34, "Cliff" },
	{ 35, "Petard" },
	{ 36, "Cavalry Archer" },
	{ 37, "Dolphin" },
	{ 38, "Flying Dog" }, //Bird
	{ 39, "Gate" },
	{ 40, "Pile" },
	{ 41, "Resource Pile" },
	{ 42, "Relic" },
	{ 43, "Monk with Relic" },
	{ 44, "Hand Cannoneer" },
	{ 45, "?45" }, //Two-Handed Swordsman
	{ 46, "?46" }, //Pikeman
	{ 47, "Scout Cavalry" },
	{ 48, "Ore Mine" },
	{ 49, "Farm / Fish Trap" },
	{ 50, "?50" }, //Spearman
	{ 51, "Packed Siege Units" },
	{ 52, "Tower" },
	{ 53, "Boarding Boat" },
	{ 54, "Unpacked Siege Units" },
	{ 55, "Scorpion" },
	{ 56, "Raider" },
	{ 57, "Cavalry Raider" },
	{ 58, "Sheep/Turkey" },
	{ 59, "King" },
	{ 60, "?60" },
	{ 61, "Horse" }
};

const struct PAIR utypes[] =
{
	{ -1, "None" },
	{  0, "Other" },
	{  1, "Boar" },
	{  2, "Building" },
	{  3, "Civilian" },
	{  4, "Military" },
	{  5, "Monk w/o Relic" }
};
