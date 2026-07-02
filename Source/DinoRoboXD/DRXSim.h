// The whole game simulation: map, economy, units, combat, enemy AI.
// Pure C++ (no UObjects); owned by the game mode, rendered by SDRXGameWidget.
#pragma once

#include "DRXTypes.h"
#include "DRXData.h"

struct FSoundEvent
{
	FString Slug;
	FVector2D Pos;       // tile-space; (-1,-1) = UI / global
	float Volume = 1.f;
};

struct FAlert
{
	FString Text;
	FLinearColor Color;
	float Life = 6.f;
	FVector2D PingPos = FVector2D(-1, -1);
};

class FDRXSim
{
public:
	static constexpr int32 MapW = 160;
	static constexpr int32 MapH = 160;

	void Init(int32 Seed);
	void Tick(float Dt);

	// save/load the full game state (binary, versioned). Returns false on
	// version mismatch or corrupt data when loading.
	static constexpr int32 SaveVersion = 1;
	bool Serialize(FArchive& Ar);

	// ---- map access
	FTile& At(int32 X, int32 Y) { return Tiles[Y * MapW + X]; }
	const FTile& At(int32 X, int32 Y) const { return Tiles[Y * MapW + X]; }
	bool InBounds(int32 X, int32 Y) const { return X >= 0 && Y >= 0 && X < MapW && Y < MapH; }
	bool IsBlocked(int32 X, int32 Y, bool bFlying) const;
	bool IsVisible(int32 X, int32 Y) const { return InBounds(X, Y) && At(X, Y).VisStamp == VisionStamp; }

	// ---- player commands (called by UI)
	bool CanAfford(const FItemStack* Cost) const;
	void PayCost(const FItemStack* Cost);
	bool CanPlaceBuilding(EBuilding B, int32 X, int32 Y) const;
	bool PlaceBuilding(EBuilding B, int32 X, int32 Y);
	void DemolishBuilding(int32 Id);
	bool IsBuildingUnlocked(EBuilding B) const;
	bool IsUnitUnlocked(EUnit U) const;
	bool QueueUnit(int32 BuildingId, EUnit U);
	void SetRecipe(int32 BuildingId, int32 RecipeIdx);
	bool StartResearch(ETech T);
	bool CanResearch(ETech T) const;
	void IssueOrder(const TArray<int32>& UnitIds, const FOrder& Order, bool bQueue);
	void SetStance(const TArray<int32>& UnitIds, EStance S);
	void SetRally(int32 BuildingId, FVector2D P);

	// ---- queries for UI
	FUnitInst* FindUnit(int32 Id);
	FBuildingInst* FindBuilding(int32 Id);
	int32 UnitAt(FVector2D TilePos, EFaction FactionFilter, bool bAny = false) const;
	int32 BuildingAt(int32 X, int32 Y) const;
	int32 UsedBandwidth() const;
	int32 MaxBandwidth() const;
	float PowerGen = 0, PowerUse = 0, PowerSat = 1, BatteryCharge = 0, BatteryCap = 0;

	// ---- state
	TArray<FTile> Tiles;
	TArray<FUnitInst> Units;
	TArray<FBuildingInst> Buildings;
	TArray<FParticle> Particles;
	TArray<FFloatText> FloatTexts;
	TArray<FProjectile> Projectiles;
	TArray<FSoundEvent> SoundQueue;      // drained by the widget each frame
	TArray<FAlert> Alerts;

	int32 Stock[ITEM_COUNT] = {};
	bool TechDone[TECH_COUNT] = {};
	ETech Researching = ETech::COUNT;
	float ResearchT = 0;

	// direct WASD control of the avatar (set by the UI every frame)
	FVector2D AvatarMoveDir = FVector2D::ZeroVector;

	float GameTime = 0;                  // seconds since start
	float DayT = 0.35f;                  // 0..1, day/night cycle
	bool bDay = true;
	int32 VisionStamp = 1;
	bool bVictory = false, bDefeat = false;
	int32 PlayerHQ = -1, VexCitadel = -1, AvatarId = -1;
	FVector2D SpawnPos;

	// raid state
	float RaidTimer = 150.f;
	int32 RaidWave = 0;
	float Escalation = 0;
	bool bRaidWarned = false;
	FVector2D LastRaidFrom;

	// stats
	int32 KillCount = 0, LossCount = 0, CampsCleared = 0;
	float AvatarRespawnT = 0;

	FRandomStream Rng;

	void AddSound(const TCHAR* Slug, FVector2D Pos = FVector2D(-1, -1), float Vol = 1.f);
	void AddAlert(const FString& Text, FLinearColor Color, FVector2D Ping = FVector2D(-1, -1));
	void AddText(const FString& T, FVector2D P, FLinearColor C);
	void Burst(EParticle Type, FVector2D P, int32 N, FLinearColor C, float Speed = 2.f, float Size = 5.f);

private:
	int32 NextId = 1;

	// generation
	void GenerateMap();
	void SpawnStartState();
	void SpawnWildlife();
	void SpawnVexEmpire();
	void PlaceEnemyBuilding(EBuilding B, int32 X, int32 Y);
	void SpawnUnit(EUnit U, EFaction F, FVector2D P, EStance S = EStance::Defensive);

	// tick helpers
	void TickPower(float Dt);
	void TickBuildings(float Dt);
	void TickHaulers(float Dt);
	void TickUnits(float Dt);
	void TickWild(FUnitInst& U, float Dt);
	void TickCombatUnit(FUnitInst& U, float Dt);
	void TickRaids(float Dt);
	void TickProjectiles(float Dt);
	void TickParticles(float Dt);
	void TickVision();
	void TickUpkeep(float Dt);
	void TickResearch(float Dt);

	void UnitAttack(FUnitInst& U, FUnitInst* TargetU, FBuildingInst* TargetB);
	void DamageUnit(FUnitInst& U, float Dmg, EFaction Attacker);
	void DamageBuilding(FBuildingInst& B, float Dmg);
	void KillUnit(FUnitInst& U, EFaction Killer);
	void DestroyBuilding(FBuildingInst& B);
	int32 FindTargetUnit(const FUnitInst& U, float Range) const;
	int32 FindTargetUnitForTurret(const FBuildingInst& B, const FBuildingDef& D) const;
	int32 FindTargetBuilding(const FUnitInst& U, float Range) const;
	bool MoveToward(FUnitInst& U, FVector2D Dest, float Dt, float ArriveDist);
	bool FindPath(const FUnitInst& U, FVector2D Dest, TArray<FVector2D>& Out) const;
	void FireProjectile(FVector2D From, FVector2D To, int32 TargetUnit, int32 TargetBld,
	                    float Dmg, EFaction F, FLinearColor Color, bool bArc = false, float AoE = 0);
	FVector2D BuildingCenter(const FBuildingInst& B) const;
	void GrantVision(FVector2D P, float Radius);
	void TryTameNearby(FUnitInst& PlayerUnit);
	void LootCamp(FVector2D P);
	EItem ResNodeItem(EResNode R) const;
};
