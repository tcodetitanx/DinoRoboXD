// DinoRoboXD core types: items, buildings, units, techs, map tiles.
#pragma once

#include "CoreMinimal.h"

// ---------------------------------------------------------------- items

enum class EItem : uint8
{
	Wood, Stone, IronOre, CopperOre, Coal, Amber, ChronoShard, Meat, Berry, Egg,
	IronIngot, CopperIngot, Steel, Gear, Wire, Circuit, AdvCircuit, Resin,
	CyberModule, Feed, Plate, FuelCell, ChronoBar,
	COUNT
};
static constexpr int32 ITEM_COUNT = (int32)EItem::COUNT;

struct FItemDef
{
	const TCHAR* Name;
	const TCHAR* Tex;      // /Game/DRX/T asset name
};

struct FItemStack
{
	EItem Item;
	int32 Count;
};

// ---------------------------------------------------------------- factions

enum class EFaction : uint8 { Player, Vex, Wild };

// ---------------------------------------------------------------- terrain

enum class ETerrain : uint8 { Grass, Jungle, Sand, Ash, Water, Rock };

enum class EResNode : uint8
{
	None, Tree, Berry, IronField, CopperField, CoalField, StoneField,
	AmberGrove, GeoVent, ChronoRift, EggNest
};

struct FTile
{
	ETerrain Terrain = ETerrain::Grass;
	EResNode Res = EResNode::None;
	int16 ResAmount = 0;
	int16 GroundVariant = 0;   // variant pick for resource node sprites
	int16 DecorVariant = -1;   // <100 plant, 100+ rock, 200+ flat ground patch
	int32 BuildingId = -1;     // occupying building instance
	bool bExplored = false;
	int32 VisStamp = 0;        // == sim vision stamp when currently visible
	FColor Ground = FColor(58, 84, 44);  // smooth pre-blended terrain color
};

// ---------------------------------------------------------------- buildings

enum class EBuildFn : uint8
{
	HQ, Storage, Miner, Smelter, Assembler, Generator, Turret, Wall, Trap, Lab,
	UnitFactory, Paddock, FeedTrough, Hatchery, ConversionRig, Medic, Radar,
	Siphon, ShieldDome, Watchtower, EnemySpawner, EnemyCitadel, Decor
};

enum class EBuilding : uint8
{
	// player
	HQ, Stockpile, FirePit, LeanTo, HandCrank, DronePad,
	MineScaffold, Kiln, Workbench, Paddock, FeedTrough, Palisade, SpikeTrap,
	Watchtower, NestHatchery,
	Drill, Furnace, Assembler, CoalBurner, Storehouse, TurretCannon, WallStone,
	Lab, BotFactory, Smokehouse,
	ConversionRig, Incubator, AdvAssembler, TurretHeavy, TurretTwin, Geothermal,
	SolarPanel, RadarStation, AirPad, MedicBay, BatteryBank,
	ChronoSiphon, ArcReactor, MechWorks, ChronoForge, TeslaSpire, ShieldDome,
	// enemy
	VexCitadel, VexSpawner, VexTurret, VexTesla, VexWall, VexFactory, VexPen,
	COUNT
};
static constexpr int32 BUILDING_COUNT = (int32)EBuilding::COUNT;

struct FBuildingDef
{
	const TCHAR* Name;
	const TCHAR* Tex;
	EBuildFn Fn;
	int8 W, H;                 // footprint in tiles
	int32 HP;
	int32 Era;                 // 0..4 (5 = enemy, not buildable)
	float PowerUse;            // while active
	float PowerGen;
	FItemStack Cost[4];        // zero-terminated (Count==0)
	float BuildTime;
	// combat (turrets)
	float Dmg = 0, Range = 0, Cooldown = 1;
	bool bHitsAir = false;
	int32 ReqTech = -1;        // (int)ETech gate, -1 = none
};

// ---------------------------------------------------------------- recipes

struct FRecipe
{
	const TCHAR* Name;
	EBuilding At;              // building type that runs it (also matched by Fn)
	FItemStack In[4];          // zero-terminated
	FItemStack Out;
	float Time;
};

// ---------------------------------------------------------------- units

enum class EUnitFamily : uint8
{
	Robot, Raptor, Trice, Anky, Ptera, Stego, Brachio, Rex, Spino, Galli, Para, Critter
};

enum class EUnit : uint8
{
	// player robots
	HaulerBot, ReconQuad, DrillSpider, GunWalker, SniperDroid, MechBrawler,
	ShieldMech, SiegeTank, Gunship, DroneHauler, SawBreaker, APC,
	// cyber dinos
	CyRaptor, CyRaptorGatling, CyTriceHeavy, CyTriceMissile, CyTriceHauler,
	CyTriceRhino, CyAnkyCannon, CyAnkyMace, CyAnkyDrill, CyAnkyWreck,
	CyPtera, CyPteraWar, CyPteraRotor, CyStego, CyStegoCrystal,
	CyBrachio, CyBrachioRadar, CyBrachioTank, CyRex, CyRexRed,
	CySpinoSaw, CySpinoSail, CyGalli, CyGalliPack, CyParaJets,
	// wild
	WdRaptor, WdTrice, WdAnky, WdPtera, WdStego, WdBrachio, WdRex, WdSpino,
	WdGalli, WdPara, WdSmilodon, WdToad, WdTurtle, WdThief,
	// enemy combat forms (Vex shackled + robots share cyber stats, own skins)
	VxRaptor, VxTrice, VxAnky, VxPtera, VxRex, VxGunWalker, VxGunship, VxSiege,
	// avatar
	PlayerAvatar,
	COUNT
};
static constexpr int32 UNIT_COUNT = (int32)EUnit::COUNT;

struct FUnitDef
{
	const TCHAR* Name;
	const TCHAR* Tex;
	EUnitFamily Family;
	float HP;
	float Armor;
	float Dmg;
	float Range;               // tiles; <=1.2 => melee
	float Cooldown;
	float Speed;               // tiles/sec
	bool bFlies;
	bool bHitsAir = true;
	int32 Carry = 0;           // hauler capacity
	int32 BW = 0;              // bandwidth
	float Upkeep = 0;          // feed/min (dinos)
	FItemStack Cost[4];        // build/convert cost, zero-terminated
	EBuilding BuiltAt = EBuilding::COUNT;
	float BuildTime = 0;
	float Scale = 1.f;         // render scale multiplier
	const TCHAR* AttackSfx = nullptr;
	const TCHAR* DeathSfx = nullptr;
	int32 MeatDrop = 0;        // wild only
	EUnit CyberForm = EUnit::COUNT; // wild -> tamed conversion default form
	int32 ReqTech = -1;        // (int)ETech gate, -1 = none
};

// ---------------------------------------------------------------- tech

enum class ETech : uint8
{
	FlintTools, BoneCraft, BeastKen, EggLore, Watchers,
	SteelSmelting, PoweredMining, AssemblyLines, Ballistics, RobotChassis, LogisticsII,
	AmberRefining, NeuroShackles, HeavyGrafts, ApexGrafts, Incubation,
	HeavyOrdnance, Skywork, FieldMedicine, GeothermalTaps, UplinkTowers,
	RiftSiphons, ArcPower, ChronoForgeTech, Aegis, RedRexProtocol,
	COUNT
};
static constexpr int32 TECH_COUNT = (int32)ETech::COUNT;

struct FTechDef
{
	const TCHAR* Name;
	const TCHAR* Desc;
	int32 Era;
	FItemStack Cost[3];
	float Time;
	ETech Requires;            // COUNT = none
	const TCHAR* BadgeTex;
};

// ---------------------------------------------------------------- orders / instances

enum class EStance : uint8 { Aggressive, Defensive, Hold };
enum class EOrderType : uint8 { None, Move, AttackMove, AttackTarget, Harvest, Build, Patrol };

struct FOrder
{
	EOrderType Type = EOrderType::None;
	FVector2D Pos = FVector2D::ZeroVector;   // tile-space float coords
	int32 TargetUnit = -1;
	int32 TargetBuilding = -1;
	FIntPoint HarvestTile = FIntPoint(-1, -1);
};

struct FUnitInst
{
	int32 Id = -1;
	EUnit Type = EUnit::COUNT;
	EFaction Faction = EFaction::Wild;
	FVector2D Pos = FVector2D::ZeroVector;   // tile-space
	float HP = 1;
	FOrder Order;
	FVector2D PatrolA, PatrolB;
	bool bPatrolToB = true;
	EStance Stance = EStance::Defensive;
	FVector2D HomePos;                       // defensive return point
	float AttackCd = 0;
	int32 CarryCount = 0;
	EItem CarryItem = EItem::COUNT;
	int32 Rank = 0;                          // veterancy 0..3
	float Kills = 0;
	uint16 GroupMask = 0;                    // control groups bitmask
	bool bSelected = false;
	float AnimPhase = 0;                     // walk bob
	float FlashT = 0;                        // hit flash timer
	float SquashT = 0;                       // land/attack squash timer
	float FacingX = -1;                      // -1 = left (art default)
	int8 FacingAxis = 1;                     // 0 = horizontal, -1 = up (back), +1 = down (front)
	float StarveT = 0;
	// hauler job
	int32 JobFromB = -1, JobToB = -1;
	// wild behavior
	float WanderT = 0;
	bool bWeakened = false;                  // tameable state
	float LungeT = 0;                        // attack lunge anim
	// pathing
	TArray<FVector2D> Path;
	int32 PathIdx = 0;
	float RepathT = 0;
};

struct FBuildingInst
{
	int32 Id = -1;
	EBuilding Type = EBuilding::COUNT;
	EFaction Faction = EFaction::Player;
	int32 X = 0, Y = 0;                      // top-left tile
	float HP = 1;
	bool bConstructed = false;
	float Progress = 0;                      // construction or craft progress
	int32 RecipeIdx = -1;                    // active recipe
	int32 OutBuf[ITEM_COUNT] = {};           // simplified: output buffer counts
	int32 InBuf[ITEM_COUNT] = {};
	float AttackCd = 0;
	float FlashT = 0;
	float AnimPhase = 0;
	FVector2D RallyPoint = FVector2D(-1, -1);
	// unit production queue
	TArray<EUnit> BuildQueue;
	float UnitProgress = 0;
	// spawner (enemy)
	float SpawnT = 0;
	bool bPowered = true;
};

// ---------------------------------------------------------------- particles & fx

enum class EParticle : uint8 { Puff, Spark, Blood, Muzzle, Explosion, Leaf, Glow, Shard };

struct FParticle
{
	EParticle Type;
	FVector2D Pos;      // tile-space
	FVector2D Vel;
	float Life, MaxLife;
	float Size;
	FLinearColor Color;
};

struct FFloatText
{
	FString Text;
	FVector2D Pos;
	float Life;
	FLinearColor Color;
};

struct FProjectile
{
	FVector2D Pos, Target;
	float Speed;
	float Dmg;
	int32 TargetUnit = -1, TargetBuilding = -1;
	EFaction Faction;
	FLinearColor Color;
	bool bArc = false;   // artillery lob
	float T = 0, T0 = 0; // arc interpolation
	FVector2D Start;
	bool bAoE = false;
	float AoERadius = 0;
};
