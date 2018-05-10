/* MODEL */

#ifndef WK_CONST_H
#define WK_CONST_H

#include <array>
#include <map>

struct TerrainTypes {
	enum Value {
		Grass,
		ShallowWater1,
		Beach,
		Dirt3,
		Shallows,
		Leaves,
		Dirt1,
		Farm,
		DeadFarm,
		Grass3,
		Forest,
		Dirt2,
		Grass2,
		PalmForest,
		Desert,
		ShorelessWater,
		OldGrass,
		JungleForest,
		BambooForest,
		PineForest,
		OakForest,
		SnowPineforest,
		DeepWater2,
		MediumWater3,
		Road,
		BrokenRoad,
		SolidIce,
		BuildingDirt,
		WaterBridge,
		Farm1,
		Farm2,
		Farm3,
		Snow,
		SnowDirt,
		SnowGrass,
		ShipIce,
		BuildingSnow,
		IceBeach,
		SnowRoad,
		FungusRoad,
		Koth,
		Savannah,
		Dirt4,
		DryRoad,
		Moorland,
		CrackedEarth,
		Quicksand,
		Black,
		DragonForest,
		BaobabForest,
		AcaciaForest,
		Beach2,
		Beach3,
		Beach4,
		MangroveShallows,
		MangroveForest,
		Rainforest,
		DeepWater4,
		ShallowWater5,
		AzureShallows,
		JungleGrass,
		JungleRoad,
		JungleLeaves,
		RiceFarm,
		DeadRiceFarm,
		RiceFarm1,
		RiceFarm2,
		RiceFarm3,
		RestrictedTerrain = 99,
		LandTerrain = 101,
		DeepWaterTerrain = 102,
		WaterTerrain = 103,
		UnbuildableTerrain = 104,
		ForestTerrain = 105
	};
};

struct WKTerrainFlags {
	enum Value {
		None = 0x00,
		NativeTerrain = 0x01,
		DynamicTerrain = 0x02,
		FixedTerrain = 0x04
	};
};

extern const std::array<std::pair<int, int>,33> unitSwaps;
extern const std::array<std::string,42> slpNames;
extern const std::array<std::string,28> terrainFiles;
extern const std::array<unsigned char,68> initSwapTerrains;

#endif //WK_CONST_H
