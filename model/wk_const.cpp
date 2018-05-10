/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	scen_const.cpp -- Defines various scenario constants in scen.h

	MODEL
**/

#include "wk_const.h"

 std::array<std::pair<int, int>,33> const unitSwaps = { {

    {1103, 529}, // Fire Galley, Fire Ship
    {1104, 527}, // Demolition Raft, Demolition Ship NOTE: These two are special to make the tech tree work

    {1001, 106}, // Organ Gun, INFIL_D
    {1003, 114}, // Elite Organ Gun, LNGBT_D
    {1006, 183}, // Elite Caravel, TMISB
    {1007, 203}, // Camel Archer, VDML
    {1009, 208}, // Elite Camel Archer, TWAL
    {1010, 223}, // Genitour, VFREP_D
    {1012, 230}, // Elite Genitour, VMREP_D
    {1013, 260}, // Gbeto, OLD-FISH3
    {1015, 418}, // Elite Gbeto, TROCK
    {1016, 453}, // Shotel Warrior, DOLPH4
    {1018, 459}, // Elite Shotel Warrior, FISH5
    {1103, 467}, // Fire Ship, Nonexistent
    {1105, 494}, // Siege Tower, CVLRY_D
    {1104, 653}, // Demolition Ship, HFALS_D
    {947, 699}, // Cutting Mangonel, HSUBO_D
    {948, 701}, // Cutting Onager, HWOLF_D
    {1079, 732}, // Genitour placeholder, HKHAN_D
    {1021, 734}, // Feitoria, Nonexistent
    {1120, 760}, // Ballista Elephant, BHUSK_D
    {1155, 762}, // Imperial Skirmisher, BHUSKX_D
    {1134, 766}, // Elite Battle Ele, UPLUM_D
    {1132, 774}, // Battle Elephant, UCONQ_D
    {1131, 782}, // Elite Rattan Archer, HPOPE_D
    {1129, 784}, // Rattan Archer, HWITCH_D
    {1128, 811}, // Elite Arambai, HEROBOAR_D
    {1126, 823}, // Arambai, BOARJ_D
    {1125, 830}, // Elite Karambit, UWAGO_D
    {1123, 836}, // Karambit, HORSW_D
    {946, 848}, // Noncut Ballista Elephant, TDONK_D
    {1004, 861}, // Caravel, mkyby_D
    {1122, 891} // Elite Ballista Ele, SGTWR_D
} };

const std::array<std::string,28> terrainFiles = {
    "SAVANNAH.slp",
    "DIRT4.slp",
    "DLC_DRYROAD.slp",
    "DLC_MOORLAND",
    "CRACKEDIT.slp",
    "QUICKSAND.slp",
    "BLACK.slp",
    "DRAGONFOREST.slp",
    "BAOBAB.slp",
    "ACACIA_FOREST.slp",
    "DLC_BEACH2.slp",
    "DLC_BEACH3.slp",
    "DLC_BEACH4.slp",
    "DLC_MANGROVESHALLOW.slp",
    "DLC_MANGROVEFOREST.slp",
    "DLC_RAINFOREST.slp",
    "DLC_WATER4.slp",
    "DLC_WATER5.slp",
    "",
    "",
    "DLC_JUNGLEGRASS.slp",
    "DLC_JUNGLEROAD.slp",
    "DLC_JUNGLELEAVES.slp",
    "15004.slp",
    "15005.slp",
    "15021.slp",
    "15022.slp",
    "15023.slp"
};

const std::array<std::string,42> slpNames = {
	//Those that don't have a 15xxx.slp in this list should never be used anyway
    "15001.slp", //GRASS1
    "15002.slp", //WATER1
    "15017.slp", //BEACH
    "15007.slp", //DIRT3
    "15014.slp", //SHALLOWS
    "15011.slp", //LEAVES
    "15000.slp", //DIRT1
    "15004.slp", //FARM
    "15005.slp", //DEAD FARM
    "15009.slp", //GRASS3
    "15011.slp", //FOREST
    "15006.slp", //DIRT2
    "15008.slp", //GRASS2
    "15010.slp", //PALM/DESERT
    "15010.slp", //DESERT
    "15003.slp", //NO DOCK WATER / WATER3
    "BAOBAB FOREST 15025",
    "JUNGLE FOREST",
    "BAMBOO FOREST",
    "PINE FOREST",
    "MANGROVE FOREST 15012",
    "15029.slp", // SNOW PINE FOREST??
    "15015.slp", //WATER2 DEEP
    "15016.slp", //WATER3 MEDIUM
    "15018.slp", //ROAD
    "15019.slp", //ROAD, BROKEN
    "15024.slp", //ICE (no ships)
    "BUILD LAND",
    "WATER BRIDGE",
    "15021.slp", //FARM1
    "15022.slp", //FARM2
    "15023.slp", //FARM3
    "15026.slp", //SNOW
    "15027.slp", //SNOW DIRT
    "15028.slp", //SNOW GRASS
    "15024.slp", // ICE ALLOWS SHIPS
    "SNOW DIRT (building residue)",
    "ICE, BEACH",
    "15030.slp", // ROAD; SNOW
    "15031.slp", //Road, Fungus
    "15033.slp",
    "ACACIA FOREST 15013"
};

/*
 * Terrain Types ending on ...Terrain are candidates for replacement
 * RestrictedTerrain should either not be used, or is intended for one specific terrain
 * Terrain Types not ending on ...Terrain show the preferred terrain used for replacement
 */
const std::array<unsigned char, 68> initSwapTerrains = {
	TerrainTypes::LandTerrain, // 0 GRASS, no change
	TerrainTypes::WaterTerrain, // 1 WATER1 <-> DLC_WATER5
	TerrainTypes::RestrictedTerrain, // 2PROBLEM: ALSO 52,53???
	TerrainTypes::LandTerrain, // 3 DIRT3 <-> DIRT4
	TerrainTypes::RestrictedTerrain, // 4 SHALLOW <-> MANGROVE SHALLOW
	TerrainTypes::LandTerrain, // 5 LEAVES, no change
	TerrainTypes::LandTerrain, // 6 DIRT1 <-> CRACKEDIT
	TerrainTypes::RestrictedTerrain, // 7 FARM <-> RICE FARM
	TerrainTypes::RestrictedTerrain, // 8 DEAD FARM <-> DEAD RICE FARM
	TerrainTypes::LandTerrain, // 9 GRASS3 <-> GRASS4/MOORLAND
	TerrainTypes::ForestTerrain, // 10 FOREST <-> DLC_RAINFOREST (TREE UPGRADE??)
	TerrainTypes::BuildingDirt, // 11 DIRT2 <-> DLC_JUNGLELEAVES
	TerrainTypes::LandTerrain, // 12 GRASS2 <-> DLC_JUNGLEGRASS
	TerrainTypes::ForestTerrain, // 13 PALM DESERT, no change
	TerrainTypes::LandTerrain, // 14 DESERT <-> SAVANNAH
	TerrainTypes::RestrictedTerrain, // 15 NO DOCK WATER, no change
	TerrainTypes::Grass, // 16 OLD GRASS <-> BAOBAB
	TerrainTypes::RestrictedTerrain, // 17 JUNGLE, no change
	TerrainTypes::RestrictedTerrain, // 18 BAMBOO, no change
	TerrainTypes::RestrictedTerrain, // 19 PINE FOREST, no change
	TerrainTypes::Forest, // 20 OAK FOREST <-> MANGROVE FOREST
	TerrainTypes::ForestTerrain, // 21 SNOW PINE FOREST <-> DRAGON_FOREST (TREE UPGRADE??)
	TerrainTypes::DeepWaterTerrain, // 22 WATER2/DEEP <-> DLC_WATER4
	TerrainTypes::DeepWaterTerrain, // 23 WATER3/MEDIUM, no change
	TerrainTypes::LandTerrain, // 24 ROAD, no change
	TerrainTypes::LandTerrain, // 25 ROAD, BROKEN <-> DLC_DRYROAD
	TerrainTypes::RestrictedTerrain, // 26 ICE SHIPS <-> DLC_NEWSHALLOW
	TerrainTypes::RestrictedTerrain, // 27 BUILDING DIRT, no change
	TerrainTypes::RestrictedTerrain, // 28 WATER BRIDGE, no change
	TerrainTypes::RestrictedTerrain, // 29 FARM1 <-> RICE FARM1
	TerrainTypes::RestrictedTerrain, // 30 FARM2 <-> RICE FARM2
	TerrainTypes::RestrictedTerrain, // 31 FARM3 <-> RICE FARM3
	TerrainTypes::LandTerrain, // 32 SNOW, no change
	TerrainTypes::BuildingSnow, // 33 SNOW DIRT, no change
	TerrainTypes::LandTerrain, // 34 SNOW GRASS, no change
	TerrainTypes::UnbuildableTerrain, // 35 ICE NO SHIPS
	TerrainTypes::RestrictedTerrain, // 36BULDING SNOW, no change
	TerrainTypes::RestrictedTerrain, // 37 ICE BEACH, no change
	TerrainTypes::SnowDirt, // 38 SNOW ROAD, no change
	TerrainTypes::LandTerrain, // 39 FUNGUS ROAD <-> DLC_JUNGLEROAD
	TerrainTypes::UnbuildableTerrain , // 40, also 47 PROBLEM KOTH <-> QUICKSAND, BLACK
	TerrainTypes::Savannah, // 41 SAVANNAH <-> DESERT PROBLEM ACACIA 50
	TerrainTypes::OldGrass, // 42 DIRT4 <-> BAOBAB (Trees aren't included)
	TerrainTypes::BrokenRoad, // 43 DESERT/DRYROAD <-> ROAD, BRKEN
	TerrainTypes::Grass3, // 44 GRASS3 <-> GRASS4/MOORLAND
	TerrainTypes::SnowRoad, // 45 ROAD_SNOW <-> CRACKEDIT
	TerrainTypes::Koth, // 46, also 47 PROBLEM KOTH <-> QUICKSAND, BLACK
	TerrainTypes::Koth, // 47, also 47 PROBLEM KOTH <-> QUICKSAND, BLACK
	TerrainTypes::PineForest, // 48 SNOW PINE FOREST <-> DRAGON_FOREST
	TerrainTypes::OldGrass, // 49 OLD GRASS <-> BAOBAB
	TerrainTypes::Savannah, // 50 UNUSED <-> ACACIA_FOREST
	TerrainTypes::Beach, // 51 DLC_BEACH2
	TerrainTypes::Beach, // 52
	TerrainTypes::Beach, // 53
	TerrainTypes::Dirt2 , // 54 Dirt2 <-> MANGROVE SHALLOW
	TerrainTypes::OakForest, // 55 OAK FOREST <-> MANGROVE FOREST
	TerrainTypes::Forest, // 56 FOREST <-> DLC_RAINFOREST
	TerrainTypes::DeepWater2, // 57 WATER2/DEEP <-> DLC_WATER4
	TerrainTypes::ShallowWater1, // 58 WATER1 <-> DLC_WATER5
	TerrainTypes::SolidIce, // 59 ICE NO SHIPS <-> DLC_NEWSHALLOW
	TerrainTypes::Grass2, // 60 GRASS2 <-> DLC_JUNGLEGRASS
	TerrainTypes::FungusRoad, // 61 FUNGUS ROAD <-> DLC_JUNGLEROAD
	TerrainTypes::Leaves, // 62 DIRT2 <-> DLC_JUNGLELEAVES
	TerrainTypes::Farm, // 63 FARM <-> RICE FARM
	TerrainTypes::DeadFarm, // 64 DEAD FARM <-> DEAD RICE FARM
	TerrainTypes::Farm1, // 65 FARM1 <-> RICE FARM1
	TerrainTypes::Farm2, // 66 FARM2 <-> RICE FARM2
	TerrainTypes::Farm3, // 67 FARM3 <-> RICE FARM3
};

