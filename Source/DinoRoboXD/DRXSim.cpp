#include "DRXSim.h"

using namespace DRX;

// ================================================================ helpers

static bool CostHas(const FItemStack* Cost, int32 I) { return Cost[I].Count > 0; }

FVector2D FDRXSim::BuildingCenter(const FBuildingInst& B) const
{
	const FBuildingDef& D = Bld(B.Type);
	return FVector2D(B.X + D.W * 0.5f, B.Y + D.H * 0.5f);
}

void FDRXSim::AddSound(const TCHAR* Slug, FVector2D Pos, float Vol)
{
	if (Slug && SoundQueue.Num() < 64)
		SoundQueue.Add({Slug, Pos, Vol});
}

void FDRXSim::AddAlert(const FString& Text, FLinearColor Color, FVector2D Ping)
{
	Alerts.Insert({Text, Color, 7.f, Ping}, 0);
	if (Alerts.Num() > 6) Alerts.SetNum(6);
}

void FDRXSim::AddText(const FString& T, FVector2D P, FLinearColor C)
{
	if (FloatTexts.Num() < 60)
		FloatTexts.Add({T, P, 1.6f, C});
}

void FDRXSim::Burst(EParticle Type, FVector2D P, int32 N, FLinearColor C, float Speed, float Size)
{
	for (int32 i = 0; i < N && Particles.Num() < 900; i++)
	{
		float A = Rng.FRandRange(0, 2 * PI);
		float S = Rng.FRandRange(Speed * 0.3f, Speed);
		FParticle Pt;
		Pt.Type = Type; Pt.Pos = P;
		Pt.Vel = FVector2D(FMath::Cos(A) * S, FMath::Sin(A) * S - Speed * 0.4f);
		Pt.MaxLife = Pt.Life = Rng.FRandRange(0.35f, 0.9f);
		Pt.Size = Rng.FRandRange(Size * 0.6f, Size * 1.4f);
		Pt.Color = C;
		Particles.Add(Pt);
	}
}

EItem FDRXSim::ResNodeItem(EResNode R) const
{
	switch (R)
	{
	case EResNode::Tree:        return EItem::Wood;
	case EResNode::Berry:       return EItem::Berry;
	case EResNode::IronField:   return EItem::IronOre;
	case EResNode::CopperField: return EItem::CopperOre;
	case EResNode::CoalField:   return EItem::Coal;
	case EResNode::StoneField:  return EItem::Stone;
	case EResNode::AmberGrove:  return EItem::Amber;
	case EResNode::ChronoRift:  return EItem::ChronoShard;
	case EResNode::EggNest:     return EItem::Egg;
	default:                    return EItem::COUNT;
	}
}

// ================================================================ save / load

template <typename TEnum>
static void SerEnum(FArchive& Ar, TEnum& E)
{
	uint8 V = (uint8)E;
	Ar << V;
	E = (TEnum)V;
}

bool FDRXSim::Serialize(FArchive& Ar)
{
	int32 Version = SaveVersion;
	Ar << Version;
	if (Version != SaveVersion) return false;

	Ar << GameTime << DayT << bDay << bVictory << bDefeat;
	Ar << PlayerHQ << VexCitadel << AvatarId << SpawnPos;
	Ar << RaidTimer << RaidWave << Escalation << bRaidWarned << LastRaidFrom;
	Ar << KillCount << LossCount << CampsCleared << AvatarRespawnT;
	Ar << BatteryCharge;
	for (int32 i = 0; i < ITEM_COUNT; i++) Ar << Stock[i];
	for (int32 i = 0; i < TECH_COUNT; i++) Ar << TechDone[i];
	SerEnum(Ar, Researching);
	Ar << ResearchT;

	int32 SeedFixup = 0;
	Ar << SeedFixup; // reserved

	// tiles
	int32 TileCount = Tiles.Num();
	Ar << TileCount;
	if (Ar.IsLoading())
	{
		if (TileCount != MapW * MapH) return false;
		Tiles.SetNum(TileCount);
	}
	for (FTile& T : Tiles)
	{
		SerEnum(Ar, T.Terrain);
		SerEnum(Ar, T.Res);
		Ar << T.ResAmount << T.GroundVariant << T.DecorVariant << T.bExplored << T.Ground;
		if (Ar.IsLoading()) { T.BuildingId = -1; T.VisStamp = 0; }
	}

	// buildings
	int32 NB = Buildings.Num();
	Ar << NB;
	if (Ar.IsLoading()) { Buildings.Empty(); Buildings.SetNum(NB); }
	for (FBuildingInst& B : Buildings)
	{
		Ar << B.Id;
		SerEnum(Ar, B.Type);
		SerEnum(Ar, B.Faction);
		Ar << B.X << B.Y << B.HP << B.bConstructed << B.Progress << B.RecipeIdx;
		for (int32 i = 0; i < ITEM_COUNT; i++) Ar << B.OutBuf[i];
		Ar << B.RallyPoint << B.UnitProgress << B.SpawnT;
		int32 NQ = B.BuildQueue.Num();
		Ar << NQ;
		if (Ar.IsLoading()) B.BuildQueue.SetNum(NQ);
		for (int32 q = 0; q < NQ; q++) SerEnum(Ar, B.BuildQueue[q]);
	}

	// units
	int32 NU = Units.Num();
	Ar << NU;
	if (Ar.IsLoading()) { Units.Empty(); Units.SetNum(NU); }
	for (FUnitInst& U : Units)
	{
		Ar << U.Id;
		SerEnum(Ar, U.Type);
		SerEnum(Ar, U.Faction);
		Ar << U.Pos << U.HP;
		SerEnum(Ar, U.Order.Type);
		Ar << U.Order.Pos << U.Order.TargetUnit << U.Order.TargetBuilding << U.Order.HarvestTile;
		Ar << U.PatrolA << U.PatrolB << U.bPatrolToB;
		SerEnum(Ar, U.Stance);
		Ar << U.HomePos << U.CarryCount;
		SerEnum(Ar, U.CarryItem);
		Ar << U.Rank << U.Kills << U.GroupMask << U.FacingX << U.FacingAxis << U.StarveT << U.bWeakened;
	}

	Ar << NextId;

	if (Ar.IsLoading())
	{
		// rebuild derived state
		Particles.Reset(); FloatTexts.Reset(); Projectiles.Reset();
		SoundQueue.Reset(); Alerts.Reset();
		AvatarMoveDir = FVector2D::ZeroVector;
		for (FBuildingInst& B : Buildings)
		{
			const FBuildingDef& D = DRX::Bld(B.Type);
			for (int32 y = 0; y < D.H; y++)
			for (int32 x = 0; x < D.W; x++)
				if (InBounds(B.X + x, B.Y + y))
					At(B.X + x, B.Y + y).BuildingId = B.Id;
		}
		VisionStamp = 1;
		TickVision();
		Rng.Initialize((int32)(FPlatformTime::Cycles() & 0x7fffffff));
	}
	return !Ar.IsError();
}

// ================================================================ init / mapgen

void FDRXSim::Init(int32 Seed)
{
	Rng.Initialize(Seed);
	Tiles.SetNum(MapW * MapH);
	GenerateMap();
	SpawnStartState();
	SpawnWildlife();
	SpawnVexEmpire();
	TickVision();
}

void FDRXSim::GenerateMap()
{
	const float Nz = 0.061f, Nz2 = 0.13f;
	const FVector2D Off(Rng.FRandRange(0, 900.f), Rng.FRandRange(0, 900.f));
	const FVector2D Off2(Rng.FRandRange(0, 900.f), Rng.FRandRange(0, 900.f));

	for (int32 y = 0; y < MapH; y++)
	for (int32 x = 0; x < MapW; x++)
	{
		FTile& T = At(x, y);
		float N = FMath::PerlinNoise2D(Off + FVector2D(x, y) * Nz);
		float N2 = FMath::PerlinNoise2D(Off2 + FVector2D(x, y) * Nz2);
		float Wf = (float)x / MapW; // 0 west .. 1 east

		// river: two winding noise bands
		float Rv = FMath::PerlinNoise2D(Off + FVector2D(x * 0.023f, y * 0.023f));
		bool bRiver = FMath::Abs(Rv - 0.18f) < 0.022f || FMath::Abs(Rv + 0.31f) < 0.017f;

		if (bRiver) T.Terrain = ETerrain::Water;
		else if (Wf > 0.68f + N * 0.06f) T.Terrain = (N2 > 0.35f) ? ETerrain::Rock : ETerrain::Ash;
		else if (Wf < 0.34f + N * 0.08f) T.Terrain = (N2 > 0.45f) ? ETerrain::Grass : ETerrain::Jungle;
		else T.Terrain = (N2 > 0.42f) ? ETerrain::Sand : ETerrain::Grass;

		if (x < 2 || y < 2 || x >= MapW - 2 || y >= MapH - 2) T.Terrain = ETerrain::Rock;
		T.GroundVariant = (int16)(Rng.RandRange(0, 3));

		// flat painted ground patches (dirt, moss, sand, puddles) break up the
		// plain earth; 200+ marks them as under-everything decals
		float PatchRoll = Rng.FRand();
		if (T.Terrain == ETerrain::Grass && PatchRoll < 0.045f)
			T.DecorVariant = (int16)(200 + Rng.RandRange(0, 2));            // dirt/cracked/moss
		else if (T.Terrain == ETerrain::Jungle && PatchRoll < 0.04f)
			T.DecorVariant = (int16)(202 + Rng.RandRange(0, 2) / 2 * 2);    // moss / dark
		else if (T.Terrain == ETerrain::Sand && PatchRoll < 0.05f)
			T.DecorVariant = (int16)(205 + Rng.RandRange(0, 1));            // golden/pale sand
		else if (T.Terrain == ETerrain::Ash && PatchRoll < 0.04f)
			T.DecorVariant = (int16)(Rng.FRand() < 0.5f ? 203 : 207);       // dark / red crack
		// flavor decor (non-blocking): plants in jungle, rocks on ash
		else if (T.Terrain == ETerrain::Jungle && Rng.FRand() < 0.05f) T.DecorVariant = (int16)Rng.RandRange(0, 31);
		else if (T.Terrain == ETerrain::Ash && Rng.FRand() < 0.04f) T.DecorVariant = (int16)(100 + Rng.RandRange(0, 27));
		else if (T.Terrain == ETerrain::Grass && Rng.FRand() < 0.015f) T.DecorVariant = (int16)Rng.RandRange(0, 31);
	}

	// ---- smooth painted ground colors: per-biome tone gradients driven by
	// low-frequency noise, then blurred so biome borders and shorelines blend
	{
		const FVector2D Off3(Rng.FRandRange(0, 900.f), Rng.FRandRange(0, 900.f));
		const FVector2D Off4(Rng.FRandRange(0, 900.f), Rng.FRandRange(0, 900.f));
		static const FLinearColor ToneA[6] = {
			{0.28f, 0.40f, 0.19f}, {0.14f, 0.29f, 0.14f}, {0.58f, 0.51f, 0.34f},
			{0.27f, 0.24f, 0.29f}, {0.13f, 0.26f, 0.43f}, {0.21f, 0.20f, 0.20f} };
		static const FLinearColor ToneB[6] = {
			{0.38f, 0.51f, 0.26f}, {0.22f, 0.38f, 0.19f}, {0.68f, 0.61f, 0.42f},
			{0.35f, 0.31f, 0.35f}, {0.19f, 0.36f, 0.53f}, {0.29f, 0.28f, 0.27f} };
		TArray<FLinearColor> Cols;
		Cols.SetNum(MapW * MapH);
		for (int32 y = 0; y < MapH; y++)
		for (int32 x = 0; x < MapW; x++)
		{
			const FTile& T = At(x, y);
			float N = 0.5f + 0.5f * FMath::PerlinNoise2D(Off3 + FVector2D(x, y) * 0.09f);
			float Micro = 1.f + 0.05f * FMath::PerlinNoise2D(Off4 + FVector2D(x, y) * 0.55f);
			int32 Ti = (int32)T.Terrain;
			Cols[y * MapW + x] = FMath::Lerp(ToneA[Ti], ToneB[Ti], N) * Micro;
		}
		for (int32 Pass = 0; Pass < 2; Pass++)
		{
			TArray<FLinearColor> Next = Cols;
			for (int32 y = 1; y < MapH - 1; y++)
			for (int32 x = 1; x < MapW - 1; x++)
			{
				FLinearColor Sum(0, 0, 0);
				for (int32 dy = -1; dy <= 1; dy++)
				for (int32 dx = -1; dx <= 1; dx++)
					Sum += Cols[(y + dy) * MapW + x + dx];
				Next[y * MapW + x] = Sum / 9.f;
			}
			Cols = MoveTemp(Next);
		}
		for (int32 i = 0; i < MapW * MapH; i++)
			Tiles[i].Ground = Cols[i].ToFColor(false);
	}

	// resource nodes ------------------------------------------------------
	auto Patch = [&](EResNode R, int32 Cx, int32 Cy, int32 Rad, int32 Amount)
	{
		for (int32 y = Cy - Rad; y <= Cy + Rad; y++)
		for (int32 x = Cx - Rad; x <= Cx + Rad; x++)
		{
			if (!InBounds(x, y)) continue;
			FTile& T = At(x, y);
			if (T.Terrain == ETerrain::Water || T.Terrain == ETerrain::Rock) continue;
			float D = FVector2D::Distance(FVector2D(x, y), FVector2D(Cx, Cy));
			if (D <= Rad * (0.65f + Rng.FRand() * 0.4f))
			{
				T.Res = R;
				T.ResAmount = (int16)FMath::Max(4, Amount + Rng.RandRange(-Amount / 3, Amount / 3));
			}
		}
	};

	// trees: dense in jungle, sparse on grass
	for (int32 y = 2; y < MapH - 2; y++)
	for (int32 x = 2; x < MapW - 2; x++)
	{
		FTile& T = At(x, y);
		if (T.Res != EResNode::None || T.Terrain == ETerrain::Water || T.Terrain == ETerrain::Rock) continue;
		if (T.Terrain == ETerrain::Jungle && Rng.FRand() < 0.16f) { T.Res = EResNode::Tree; T.ResAmount = 8; }
		else if (T.Terrain == ETerrain::Grass && Rng.FRand() < 0.02f) { T.Res = EResNode::Tree; T.ResAmount = 8; }
		else if (T.Terrain != ETerrain::Ash && Rng.FRand() < 0.006f) { T.Res = EResNode::Berry; T.ResAmount = 14; }
	}

	// ore patches, biased regions
	auto RandIn = [&](int32 X0, int32 X1, int32 Y0, int32 Y1) {
		return FIntPoint(Rng.RandRange(X0, X1), Rng.RandRange(Y0, Y1)); };
	for (int32 i = 0; i < 7; i++) { FIntPoint P = RandIn(20, 100, 12, 148); Patch(EResNode::IronField, P.X, P.Y, 4, 60); }
	for (int32 i = 0; i < 6; i++) { FIntPoint P = RandIn(24, 104, 12, 148); Patch(EResNode::CopperField, P.X, P.Y, 4, 55); }
	for (int32 i = 0; i < 6; i++) { FIntPoint P = RandIn(30, 120, 12, 148); Patch(EResNode::CoalField, P.X, P.Y, 3, 50); }
	for (int32 i = 0; i < 7; i++) { FIntPoint P = RandIn(12, 110, 12, 148); Patch(EResNode::StoneField, P.X, P.Y, 3, 55); }
	for (int32 i = 0; i < 6; i++) { FIntPoint P = RandIn(8, 52, 12, 148); Patch(EResNode::AmberGrove, P.X, P.Y, 2, 30); }
	// guaranteed starter patches near spawn
	Patch(EResNode::IronField, 32, 72, 4, 60);
	Patch(EResNode::CopperField, 22, 92, 4, 55);
	Patch(EResNode::StoneField, 30, 86, 3, 55);
	Patch(EResNode::CoalField, 36, 94, 3, 50);
	Patch(EResNode::AmberGrove, 16, 70, 2, 30);

	// geothermal vents & chrono rifts (center/east)
	for (int32 i = 0; i < 8; i++)
	{
		FIntPoint P = RandIn(60, 140, 10, 150);
		if (At(P.X, P.Y).Terrain != ETerrain::Water) { At(P.X, P.Y).Res = EResNode::GeoVent; At(P.X, P.Y).ResAmount = 999; }
	}
	for (int32 i = 0; i < 6; i++)
	{
		FIntPoint P = RandIn(78, 150, 10, 150);
		if (At(P.X, P.Y).Terrain != ETerrain::Water) { At(P.X, P.Y).Res = EResNode::ChronoRift; At(P.X, P.Y).ResAmount = 999; }
	}
	// one reachable rift west of the wall for first shards
	At(70, 96).Res = EResNode::ChronoRift; At(70, 96).ResAmount = 999; At(70, 96).Terrain = ETerrain::Grass;
}

bool FDRXSim::IsBlocked(int32 X, int32 Y, bool bFlying) const
{
	if (!InBounds(X, Y)) return true;
	if (bFlying) return false;
	const FTile& T = At(X, Y);
	if (T.Terrain == ETerrain::Water || T.Terrain == ETerrain::Rock) return true;
	return T.BuildingId >= 0;
}

// ================================================================ spawn state

void FDRXSim::SpawnStartState()
{
	SpawnPos = FVector2D(26, 80);
	// clear a spawn area
	for (int32 y = 70; y < 92; y++)
	for (int32 x = 16; x < 40; x++)
	{
		FTile& T = At(x, y);
		if (T.Terrain == ETerrain::Water || T.Terrain == ETerrain::Rock) T.Terrain = ETerrain::Grass;
		if (T.Res == EResNode::Tree && FMath::Abs(x - 26) < 5 && FMath::Abs(y - 80) < 5) T.Res = EResNode::None;
	}

	FBuildingInst HQ;
	HQ.Id = NextId++; HQ.Type = EBuilding::HQ; HQ.Faction = EFaction::Player;
	HQ.X = 24; HQ.Y = 78; HQ.HP = Bld(EBuilding::HQ).HP; HQ.bConstructed = true;
	Buildings.Add(HQ);
	PlayerHQ = HQ.Id;
	for (int32 y = 0; y < 3; y++) for (int32 x = 0; x < 3; x++) At(24 + x, 78 + y).BuildingId = HQ.Id;

	SpawnUnit(EUnit::PlayerAvatar, EFaction::Player, FVector2D(28.5f, 81.5f));
	AvatarId = Units.Last().Id;
	SpawnUnit(EUnit::HaulerBot, EFaction::Player, FVector2D(23.f, 82.f));
	SpawnUnit(EUnit::HaulerBot, EFaction::Player, FVector2D(28.f, 78.f));
	SpawnUnit(EUnit::ReconQuad, EFaction::Player, FVector2D(29.f, 83.f));

	Stock[(int32)EItem::Wood] = 40;
	Stock[(int32)EItem::Stone] = 25;
	Stock[(int32)EItem::Feed] = 25;
	Stock[(int32)EItem::Berry] = 10;
	Stock[(int32)EItem::Meat] = 5;
}

void FDRXSim::SpawnUnit(EUnit U, EFaction F, FVector2D P, EStance S)
{
	FUnitInst I;
	I.Id = NextId++;
	I.Type = U; I.Faction = F; I.Pos = P;
	I.HP = Unit(U).HP;
	I.Stance = S;
	I.HomePos = P;
	I.AnimPhase = Rng.FRandRange(0, 6.f);
	Units.Add(I);
}

void FDRXSim::SpawnWildlife()
{
	auto Herd = [&](EUnit U, int32 N, float Cx, float Cy)
	{
		for (int32 i = 0; i < N; i++)
			SpawnUnit(U, EFaction::Wild, FVector2D(Cx + Rng.FRandRange(-3, 3), Cy + Rng.FRandRange(-3, 3)));
	};
	auto FindLand = [&](int32 X0, int32 X1, float MinSpawnDist = 12.f) {
		for (int32 Tries = 0; Tries < 60; Tries++)
		{
			int32 x = Rng.RandRange(X0, X1), y = Rng.RandRange(10, 150);
			if (IsBlocked(x, y, false)) continue;
			if (FVector2D::Distance(FVector2D(x, y), SpawnPos) < MinSpawnDist) continue;
			return FVector2D(x + 0.5f, y + 0.5f);
		}
		return FVector2D(80, 80);
	};

	for (int32 i = 0; i < 6; i++) { FVector2D P = FindLand(30, 100); Herd(EUnit::WdGalli, Rng.RandRange(3, 5), P.X, P.Y); }
	for (int32 i = 0; i < 5; i++) { FVector2D P = FindLand(30, 100); Herd(EUnit::WdPara, Rng.RandRange(2, 4), P.X, P.Y);
		if (Rng.FRand() < 0.8f) { At((int32)P.X, (int32)P.Y).Res = EResNode::EggNest; At((int32)P.X, (int32)P.Y).ResAmount = 3; } }
	for (int32 i = 0; i < 5; i++) { FVector2D P = FindLand(34, 105); Herd(EUnit::WdTrice, Rng.RandRange(2, 3), P.X, P.Y); }
	for (int32 i = 0; i < 4; i++) { FVector2D P = FindLand(30, 100); Herd(EUnit::WdStego, 2, P.X, P.Y); }
	for (int32 i = 0; i < 4; i++) { FVector2D P = FindLand(30, 105); Herd(EUnit::WdAnky, 2, P.X, P.Y); }
	for (int32 i = 0; i < 3; i++) { FVector2D P = FindLand(40, 100); Herd(EUnit::WdBrachio, 2, P.X, P.Y);
		At((int32)P.X, (int32)P.Y).Res = EResNode::EggNest; At((int32)P.X, (int32)P.Y).ResAmount = 3; }
	// predators keep a wide berth from the crash site
	for (int32 i = 0; i < 6; i++) { FVector2D P = FindLand(36, 110, 26.f); Herd(EUnit::WdRaptor, Rng.RandRange(2, 4), P.X, P.Y); }
	for (int32 i = 0; i < 4; i++) { FVector2D P = FindLand(8, 50, 26.f);  Herd(EUnit::WdSmilodon, 1, P.X, P.Y); }
	for (int32 i = 0; i < 3; i++) { FVector2D P = FindLand(50, 110, 30.f); Herd(EUnit::WdRex, 1, P.X, P.Y); }
	for (int32 i = 0; i < 3; i++) { FVector2D P = FindLand(45, 110, 30.f); Herd(EUnit::WdSpino, 1, P.X, P.Y); }
	for (int32 i = 0; i < 2; i++) { FVector2D P = FindLand(20, 70, 20.f);  Herd(EUnit::WdToad, 1, P.X, P.Y); }
	for (int32 i = 0; i < 4; i++) { FVector2D P = FindLand(20, 90);  Herd(EUnit::WdTurtle, 1, P.X, P.Y); }
	for (int32 i = 0; i < 2; i++) { FVector2D P = FindLand(45, 80, 24.f);  Herd(EUnit::WdThief, 1, P.X, P.Y); }
	// wild ptera roam anywhere
	for (int32 i = 0; i < 5; i++) { FVector2D P = FindLand(20, 120); Herd(EUnit::WdPtera, Rng.RandRange(1, 3), P.X, P.Y); }
}

void FDRXSim::PlaceEnemyBuilding(EBuilding B, int32 X, int32 Y)
{
	const FBuildingDef& D = Bld(B);
	// clear terrain under it
	for (int32 y = 0; y < D.H; y++)
	for (int32 x = 0; x < D.W; x++)
	{
		if (!InBounds(X + x, Y + y)) return;
		FTile& T = At(X + x, Y + y);
		if (T.BuildingId >= 0) return;
	}
	FBuildingInst I;
	I.Id = NextId++; I.Type = B; I.Faction = EFaction::Vex;
	I.X = X; I.Y = Y; I.HP = D.HP; I.bConstructed = true;
	Buildings.Add(I);
	if (B == EBuilding::VexCitadel) VexCitadel = I.Id;
	for (int32 y = 0; y < D.H; y++)
	for (int32 x = 0; x < D.W; x++)
	{
		FTile& T = At(X + x, Y + y);
		T.BuildingId = I.Id;
		if (T.Terrain == ETerrain::Water) T.Terrain = ETerrain::Ash;
		T.Res = EResNode::None;
	}
}

void FDRXSim::SpawnVexEmpire()
{
	// citadel + inner ring
	PlaceEnemyBuilding(EBuilding::VexCitadel, 150, 78);
	PlaceEnemyBuilding(EBuilding::VexTesla, 146, 74);
	PlaceEnemyBuilding(EBuilding::VexTesla, 146, 84);
	PlaceEnemyBuilding(EBuilding::VexTesla, 152, 72);
	PlaceEnemyBuilding(EBuilding::VexTesla, 152, 86);
	PlaceEnemyBuilding(EBuilding::VexFactory, 145, 79);
	for (int32 y = 70; y <= 90; y++)
		if (Rng.FRand() < 0.85f) PlaceEnemyBuilding(EBuilding::VexWall, 143, y);
	SpawnUnit(EUnit::VxRex, EFaction::Vex, FVector2D(148, 76), EStance::Defensive);
	SpawnUnit(EUnit::VxRex, EFaction::Vex, FVector2D(148, 84), EStance::Defensive);
	SpawnUnit(EUnit::VxGunship, EFaction::Vex, FVector2D(151, 80), EStance::Defensive);
	SpawnUnit(EUnit::VxGunship, EFaction::Vex, FVector2D(147, 80), EStance::Defensive);
	SpawnUnit(EUnit::VxSiege, EFaction::Vex, FVector2D(146, 88), EStance::Defensive);

	// outposts
	const FIntPoint Camps[] = {
		{96, 30}, {112, 52}, {100, 78}, {112, 104}, {98, 130},
		{126, 26}, {128, 70}, {124, 92}, {130, 134}, {138, 50}, {140, 110}
	};
	for (const FIntPoint& C : Camps)
	{
		PlaceEnemyBuilding(EBuilding::VexSpawner, C.X, C.Y);
		PlaceEnemyBuilding(EBuilding::VexTurret, C.X - 2, C.Y - 2);
		PlaceEnemyBuilding(EBuilding::VexTurret, C.X + 4, C.Y - 2);
		PlaceEnemyBuilding(EBuilding::VexTurret, C.X - 2, C.Y + 4);
		if (Rng.FRand() < 0.6f) PlaceEnemyBuilding(EBuilding::VexTurret, C.X + 4, C.Y + 4);
		if (Rng.FRand() < 0.7f) PlaceEnemyBuilding(EBuilding::VexPen, C.X + 4, C.Y + 1);
		if (Rng.FRand() < 0.5f) PlaceEnemyBuilding(EBuilding::VexFactory, C.X - 3, C.Y + 1);
		// partial wall ring
		for (int32 i = -4; i <= 6; i++)
		{
			if (Rng.FRand() < 0.55f) PlaceEnemyBuilding(EBuilding::VexWall, C.X + i, C.Y - 4);
			if (Rng.FRand() < 0.55f) PlaceEnemyBuilding(EBuilding::VexWall, C.X + i, C.Y + 6);
		}
		// garrison
		FVector2D G(C.X + 1.5f, C.Y + 1.5f);
		SpawnUnit(EUnit::VxRaptor, EFaction::Vex, G + FVector2D(-2, 3), EStance::Defensive);
		SpawnUnit(EUnit::VxRaptor, EFaction::Vex, G + FVector2D(3, 3), EStance::Defensive);
		SpawnUnit(EUnit::VxGunWalker, EFaction::Vex, G + FVector2D(0, 4), EStance::Defensive);
		if (Rng.FRand() < 0.6f) SpawnUnit(EUnit::VxTrice, EFaction::Vex, G + FVector2D(4, 0), EStance::Defensive);
		if (Rng.FRand() < 0.4f) SpawnUnit(EUnit::VxPtera, EFaction::Vex, G + FVector2D(-3, -2), EStance::Defensive);
	}
}

// ================================================================ commands

bool FDRXSim::CanAfford(const FItemStack* Cost) const
{
	for (int32 i = 0; Cost[i].Count > 0; i++)
		if (Stock[(int32)Cost[i].Item] < Cost[i].Count) return false;
	return true;
}

void FDRXSim::PayCost(const FItemStack* Cost)
{
	for (int32 i = 0; Cost[i].Count > 0; i++)
		Stock[(int32)Cost[i].Item] -= Cost[i].Count;
}

bool FDRXSim::IsBuildingUnlocked(EBuilding B) const
{
	const FBuildingDef& D = Bld(B);
	if (D.Era >= 5) return false;
	if (D.ReqTech >= 0 && !TechDone[D.ReqTech]) return false;
	return true;
}

bool FDRXSim::IsUnitUnlocked(EUnit U) const
{
	const FUnitDef& D = Unit(U);
	if (D.ReqTech >= 0 && !TechDone[D.ReqTech]) return false;
	return true;
}

bool FDRXSim::CanPlaceBuilding(EBuilding B, int32 X, int32 Y) const
{
	const FBuildingDef& D = Bld(B);
	bool bNeedsVent = (B == EBuilding::Geothermal);
	bool bNeedsRift = (B == EBuilding::ChronoSiphon);
	bool bOnVent = false, bOnRift = false, bOnOre = false;
	for (int32 y = 0; y < D.H; y++)
	for (int32 x = 0; x < D.W; x++)
	{
		if (!InBounds(X + x, Y + y)) return false;
		const FTile& T = At(X + x, Y + y);
		if (T.Terrain == ETerrain::Water || T.Terrain == ETerrain::Rock) return false;
		if (T.BuildingId >= 0) return false;
		if (!T.bExplored) return false;
		if (T.Res == EResNode::GeoVent) bOnVent = true;
		if (T.Res == EResNode::ChronoRift) bOnRift = true;
		if (T.Res == EResNode::IronField || T.Res == EResNode::CopperField ||
		    T.Res == EResNode::CoalField || T.Res == EResNode::StoneField) bOnOre = true;
		// don't let non-miners eat ore/rift tiles
		if (D.Fn != EBuildFn::Miner && D.Fn != EBuildFn::Siphon && D.Fn != EBuildFn::Generator &&
		    (T.Res == EResNode::IronField || T.Res == EResNode::CopperField ||
		     T.Res == EResNode::CoalField || T.Res == EResNode::StoneField ||
		     T.Res == EResNode::ChronoRift || T.Res == EResNode::GeoVent)) return false;
	}
	if (bNeedsVent && !bOnVent) return false;
	if (bNeedsRift && !bOnRift) return false;
	if (D.Fn == EBuildFn::Miner && !bOnOre) return false;
	// no unit standing in footprint
	for (const FUnitInst& U : Units)
	{
		if (Unit(U.Type).bFlies) continue;
		if (U.Pos.X >= X - 0.2f && U.Pos.X <= X + D.W + 0.2f &&
		    U.Pos.Y >= Y - 0.2f && U.Pos.Y <= Y + D.H + 0.2f) return false;
	}
	return true;
}

bool FDRXSim::PlaceBuilding(EBuilding B, int32 X, int32 Y)
{
	const FBuildingDef& D = Bld(B);
	if (!IsBuildingUnlocked(B) || !CanAfford(D.Cost) || !CanPlaceBuilding(B, X, Y)) return false;
	PayCost(D.Cost);
	FBuildingInst I;
	I.Id = NextId++; I.Type = B; I.Faction = EFaction::Player;
	I.X = X; I.Y = Y; I.HP = D.HP * 0.25f; I.bConstructed = (D.BuildTime <= 0);
	Buildings.Add(I);
	for (int32 y = 0; y < D.H; y++)
	for (int32 x = 0; x < D.W; x++)
		At(X + x, Y + y).BuildingId = I.Id;
	AddSound((D.W >= 3) ? TEXT("sfx_place_big") : TEXT("sfx_place"), FVector2D(X, Y));
	Burst(EParticle::Puff, FVector2D(X + D.W * 0.5f, Y + D.H), 8, FLinearColor(0.7f, 0.6f, 0.45f), 2.f, 7.f);
	return true;
}

void FDRXSim::DemolishBuilding(int32 Id)
{
	FBuildingInst* B = FindBuilding(Id);
	if (!B || B->Faction != EFaction::Player || B->Id == PlayerHQ) return;
	const FBuildingDef& D = Bld(B->Type);
	// refund half
	for (int32 i = 0; D.Cost[i].Count > 0; i++)
		Stock[(int32)D.Cost[i].Item] += D.Cost[i].Count / 2;
	AddSound(TEXT("sfx_demolish"), BuildingCenter(*B));
	DestroyBuilding(*B);
}

bool FDRXSim::QueueUnit(int32 BuildingId, EUnit U)
{
	FBuildingInst* B = FindBuilding(BuildingId);
	if (!B || !B->bConstructed || B->Faction != EFaction::Player) return false;
	const FUnitDef& D = Unit(U);
	if (!IsUnitUnlocked(U)) return false;
	if (B->BuildQueue.Num() >= 5) return false;
	if (UsedBandwidth() + D.BW > MaxBandwidth())
	{
		AddAlert(TEXT("Not enough neural bandwidth â€” build Radar Stations."), FLinearColor(1, 0.5f, 0.3f));
		AddSound(TEXT("sfx_ui_error"));
		return false;
	}
	// RED REX: only one ever
	if (U == EUnit::CyRexRed)
		for (const FUnitInst& I : Units)
			if (I.Type == EUnit::CyRexRed && I.Faction == EFaction::Player) return false;

	const bool bRig = (B->Type == EBuilding::ConversionRig);
	const bool bHatch = (Bld(B->Type).Fn == EBuildFn::Hatchery);
	if (bRig && D.Family != EUnitFamily::Robot)
	{
		// needs a tamed (player-owned wild-form) dino of the family nearby
		int32 Found = -1;
		FVector2D C = BuildingCenter(*B);
		for (FUnitInst& I : Units)
		{
			if (I.Faction != EFaction::Player) continue;
			if (I.Type < EUnit::WdRaptor || I.Type > EUnit::WdThief) continue;
			if (FamilyOf(I.Type) != D.Family) continue;
			if (FVector2D::Distance(I.Pos, C) < 10.f) { Found = I.Id; break; }
		}
		if (Found < 0)
		{
			AddAlert(FString::Printf(TEXT("Needs a tamed %s near the rig."),
				*FString(Unit(D.Family == EUnitFamily::Rex ? EUnit::WdRex : U).Name)),
				FLinearColor(1, 0.6f, 0.3f));
			AddSound(TEXT("sfx_ui_error"));
			return false;
		}
		if (!CanAfford(D.Cost)) return false;
		PayCost(D.Cost);
		// consume the tamed beast
		for (int32 i = 0; i < Units.Num(); i++)
			if (Units[i].Id == Found) { Units.RemoveAtSwap(i); break; }
	}
	else if (bHatch)
	{
		if (Stock[(int32)EItem::Egg] < 1 || Stock[(int32)EItem::Feed] < 5) return false;
		Stock[(int32)EItem::Egg] -= 1; Stock[(int32)EItem::Feed] -= 5;
	}
	else
	{
		if (!CanAfford(D.Cost)) return false;
		PayCost(D.Cost);
	}
	B->BuildQueue.Add(U);
	AddSound(TEXT("sfx_ui_confirm"));
	return true;
}

void FDRXSim::SetRecipe(int32 BuildingId, int32 RecipeIdx)
{
	if (FBuildingInst* B = FindBuilding(BuildingId))
	{
		B->RecipeIdx = RecipeIdx;
		B->Progress = 0;
	}
}

bool FDRXSim::CanResearch(ETech T) const
{
	if (TechDone[(int32)T] || Researching != ETech::COUNT) return false;
	const FTechDef& D = Tech(T);
	if (D.Requires != ETech::COUNT && !TechDone[(int32)D.Requires]) return false;
	if (!CanAfford(D.Cost)) return false;
	// needs a live research building of the right era
	bool bHasBench = false, bHasLab = false;
	for (const FBuildingInst& B : Buildings)
	{
		if (B.Faction != EFaction::Player || !B.bConstructed) continue;
		if (B.Type == EBuilding::Workbench) bHasBench = true;
		if (B.Type == EBuilding::Lab) bHasLab = true;
	}
	return (D.Era <= 1) ? (bHasBench || bHasLab) : bHasLab;
}

bool FDRXSim::StartResearch(ETech T)
{
	if (!CanResearch(T)) return false;
	PayCost(Tech(T).Cost);
	Researching = T;
	ResearchT = 0;
	AddSound(TEXT("sfx_scan"));
	return true;
}

void FDRXSim::IssueOrder(const TArray<int32>& UnitIds, const FOrder& Order, bool bQueue)
{
	int32 N = 0;
	for (int32 Id : UnitIds)
	{
		FUnitInst* U = FindUnit(Id);
		if (!U || U->Faction != EFaction::Player) continue;
		U->Order = Order;
		U->Path.Reset(); U->PathIdx = 0; U->RepathT = 0;
		U->JobFromB = U->JobToB = -1;
		// spread multi-unit move destinations slightly
		if ((Order.Type == EOrderType::Move || Order.Type == EOrderType::AttackMove) && N > 0)
		{
			float A = N * 2.4f; float R = 0.7f * FMath::Sqrt((float)N);
			U->Order.Pos += FVector2D(FMath::Cos(A) * R, FMath::Sin(A) * R);
		}
		if (Order.Type == EOrderType::Patrol) { U->PatrolA = U->Pos; U->PatrolB = Order.Pos; U->bPatrolToB = true; }
		N++;
	}
	if (N > 0)
	{
		if (Order.Type == EOrderType::AttackMove || Order.Type == EOrderType::AttackTarget)
			AddSound(TEXT("sfx_dino_call"), FVector2D(-1, -1), 0.5f);
		else AddSound(TEXT("sfx_ui_click2"), FVector2D(-1, -1), 0.5f);
	}
}

void FDRXSim::SetStance(const TArray<int32>& UnitIds, EStance S)
{
	for (int32 Id : UnitIds)
		if (FUnitInst* U = FindUnit(Id))
			if (U->Faction == EFaction::Player) { U->Stance = S; U->HomePos = U->Pos; }
}

void FDRXSim::SetRally(int32 BuildingId, FVector2D P)
{
	if (FBuildingInst* B = FindBuilding(BuildingId)) B->RallyPoint = P;
}

// ================================================================ queries

FUnitInst* FDRXSim::FindUnit(int32 Id)
{
	for (FUnitInst& U : Units) if (U.Id == Id) return &U;
	return nullptr;
}

FBuildingInst* FDRXSim::FindBuilding(int32 Id)
{
	for (FBuildingInst& B : Buildings) if (B.Id == Id) return &B;
	return nullptr;
}

int32 FDRXSim::UnitAt(FVector2D P, EFaction F, bool bAny) const
{
	int32 Best = -1; float BestD = 0.9f;
	for (const FUnitInst& U : Units)
	{
		if (!bAny && U.Faction != F) continue;
		float D = FVector2D::Distance(U.Pos, P) / FMath::Max(0.6f, Unit(U.Type).Scale);
		if (D < BestD) { Best = U.Id; BestD = D; }
	}
	return Best;
}

int32 FDRXSim::BuildingAt(int32 X, int32 Y) const
{
	return InBounds(X, Y) ? At(X, Y).BuildingId : -1;
}

int32 FDRXSim::UsedBandwidth() const
{
	int32 BW = 0;
	for (const FUnitInst& U : Units)
		if (U.Faction == EFaction::Player)
			BW += (U.Type >= EUnit::WdRaptor && U.Type <= EUnit::WdThief) ? 2 : Unit(U.Type).BW;
	return BW;
}

int32 FDRXSim::MaxBandwidth() const
{
	int32 BW = 0;
	for (const FBuildingInst& B : Buildings)
	{
		if (B.Faction != EFaction::Player || !B.bConstructed) continue;
		if (B.Type == EBuilding::HQ) BW += 20;
		else if (B.Type == EBuilding::RadarStation) BW += 15;
	}
	return BW;
}

// ================================================================ tick

void FDRXSim::Tick(float Dt)
{
	if (bVictory || bDefeat) { TickParticles(Dt); return; }
	Dt = FMath::Min(Dt, 0.06f);
	GameTime += Dt;

	// day/night: 4-minute cycle
	DayT = FMath::Fmod(GameTime / 240.f + 0.3f, 1.f);
	bDay = DayT < 0.62f;

	TickPower(Dt);
	TickBuildings(Dt);
	TickHaulers(Dt);
	TickUnits(Dt);
	TickRaids(Dt);
	TickProjectiles(Dt);
	TickParticles(Dt);
	TickUpkeep(Dt);
	TickResearch(Dt);

	static float VisT = 0; // vision every 0.4s
	VisT += Dt;
	if (VisT > 0.4f) { VisT = 0; TickVision(); }

	// avatar respawn
	if (AvatarRespawnT > 0)
	{
		AvatarRespawnT -= Dt;
		if (AvatarRespawnT <= 0)
		{
			FBuildingInst* HQ = FindBuilding(PlayerHQ);
			if (HQ)
			{
				SpawnUnit(EUnit::PlayerAvatar, EFaction::Player, BuildingCenter(*HQ) + FVector2D(2, 2));
				AvatarId = Units.Last().Id;
				AddAlert(TEXT("Dr. Kade reprinted at the HQ."), FLinearColor(0.5f, 0.9f, 1.f));
				AddSound(TEXT("sfx_teleport"));
			}
		}
	}

	for (FFloatText& T : FloatTexts) { T.Life -= Dt; T.Pos.Y -= Dt * 1.1f; }
	FloatTexts.RemoveAll([](const FFloatText& T) { return T.Life <= 0; });
	for (FAlert& A : Alerts) A.Life -= Dt;
	Alerts.RemoveAll([](const FAlert& A) { return A.Life <= 0; });
}

void FDRXSim::TickPower(float Dt)
{
	float Gen = 0, Use = 0;
	BatteryCap = 0;
	for (FBuildingInst& B : Buildings)
	{
		if (B.Faction != EFaction::Player || !B.bConstructed) continue;
		const FBuildingDef& D = Bld(B.Type);
		if (D.PowerGen > 0)
		{
			float G = D.PowerGen;
			if (B.Type == EBuilding::SolarPanel && !bDay) G = 0;
			if (B.Type == EBuilding::CoalBurner)
			{
				B.AnimPhase += Dt;
				if (B.AnimPhase > 20.f) // 1 coal per 20s
				{
					if (Stock[(int32)EItem::Coal] > 0) { Stock[(int32)EItem::Coal]--; B.AnimPhase = 0; }
				}
				if (Stock[(int32)EItem::Coal] <= 0 && B.AnimPhase > 20.f) G = 0;
			}
			if (B.Type == EBuilding::ArcReactor)
			{
				B.AnimPhase += Dt;
				if (B.AnimPhase > 30.f)
				{
					if (Stock[(int32)EItem::FuelCell] > 0) { Stock[(int32)EItem::FuelCell]--; B.AnimPhase = 0; }
				}
				if (Stock[(int32)EItem::FuelCell] <= 0 && B.AnimPhase > 30.f) G = 0;
			}
			Gen += G;
		}
		if (B.Type == EBuilding::BatteryBank) BatteryCap += 100;
		if (D.PowerUse > 0) Use += D.PowerUse;
	}
	// batteries fill on surplus, cover deficit
	float Net = Gen - Use;
	if (Net > 0) BatteryCharge = FMath::Min(BatteryCap, BatteryCharge + Net * Dt * 0.5f);
	else if (Net < 0 && BatteryCharge > 0)
	{
		float Need = FMath::Min(-Net * Dt, BatteryCharge);
		BatteryCharge -= Need;
		Gen += Need / FMath::Max(Dt, 0.001f);
	}
	PowerGen = Gen; PowerUse = Use;
	float NewSat = (Use > 0.f) ? FMath::Min(1.f, Gen / Use) : 1.f;
	if (NewSat < 0.99f && PowerSat >= 0.99f) AddSound(TEXT("sfx_power_down"));
	if (NewSat >= 0.99f && PowerSat < 0.99f) AddSound(TEXT("sfx_power_up"));
	PowerSat = NewSat;
}

void FDRXSim::TickBuildings(float Dt)
{
	TArrayView<const FRecipe> Rec = Recipes();

	for (int32 bi = 0; bi < Buildings.Num(); bi++)
	{
		FBuildingInst& B = Buildings[bi];
		const FBuildingDef& D = Bld(B.Type);
		B.FlashT = FMath::Max(0.f, B.FlashT - Dt);
		if (B.HP <= 0) continue;

		// construction
		if (!B.bConstructed)
		{
			if (B.Faction == EFaction::Player)
			{
				B.Progress += Dt;
				B.HP = FMath::Min((float)D.HP, B.HP + D.HP * 0.75f * Dt / FMath::Max(0.1f, D.BuildTime));
				if (B.Progress >= D.BuildTime)
				{
					B.bConstructed = true; B.Progress = 0; B.HP = D.HP;
					AddSound(TEXT("sfx_construct"), BuildingCenter(B), 0.8f);
					// QoL: auto-pick first runnable recipe
					if (D.Fn == EBuildFn::Smelter || D.Fn == EBuildFn::Assembler)
						for (int32 r = 0; r < Rec.Num(); r++)
							if (BuildingRunsRecipe(B.Type, Rec[r])) { B.RecipeIdx = r; break; }
				}
			}
			continue;
		}

		const float Sat = (D.PowerUse > 0 && B.Faction == EFaction::Player) ? PowerSat : 1.f;
		FVector2D C = BuildingCenter(B);

		switch (D.Fn)
		{
		case EBuildFn::Miner:
		{
			const float Rate = (B.Type == EBuilding::MineScaffold) ? 4.f : 1.6f;
			B.Progress += Dt * Sat;
			if (B.Progress >= Rate)
			{
				B.Progress = 0;
				// mine any ore tile under the footprint
				for (int32 y = 0; y < D.H; y++)
				for (int32 x = 0; x < D.W; x++)
				{
					FTile& T = At(B.X + x, B.Y + y);
					EItem It = ResNodeItem(T.Res);
					if (It != EItem::COUNT && T.Res != EResNode::Tree && T.Res != EResNode::Berry &&
					    T.Res != EResNode::GeoVent && T.Res != EResNode::ChronoRift && T.ResAmount > 0)
					{
						T.ResAmount--;
						if (T.ResAmount <= 0) T.Res = EResNode::None;
						B.OutBuf[(int32)It]++;
						B.AnimPhase += 1.f;
						if (IsVisible(B.X, B.Y)) AddSound(TEXT("sfx_mine_hit"), C, 0.035f);
						Burst(EParticle::Puff, C, 2, FLinearColor(0.6f, 0.55f, 0.45f), 1.2f, 4.f);
						x = 99; y = 99; // one tile per cycle
					}
				}
			}
			break;
		}
		case EBuildFn::Siphon:
		{
			B.Progress += Dt * Sat;
			if (B.Progress >= 8.f)
			{
				B.Progress = 0;
				B.OutBuf[(int32)EItem::ChronoShard]++;
				Burst(EParticle::Shard, C, 4, FLinearColor(0.5f, 0.7f, 1.f), 1.6f, 4.f);
				AddSound(TEXT("sfx_chrono_flash"), C, 0.4f);
			}
			break;
		}
		case EBuildFn::Smelter:
		case EBuildFn::Assembler:
		{
			if (B.RecipeIdx >= 0 && B.RecipeIdx < Rec.Num() && BuildingRunsRecipe(B.Type, Rec[B.RecipeIdx]))
			{
				const FRecipe& R = Rec[B.RecipeIdx];
				if (B.Progress <= 0.f)
				{
					bool bOk = true;
					for (int32 i = 0; R.In[i].Count > 0; i++)
						if (Stock[(int32)R.In[i].Item] < R.In[i].Count) bOk = false;
					if (bOk)
					{
						for (int32 i = 0; R.In[i].Count > 0; i++)
							Stock[(int32)R.In[i].Item] -= R.In[i].Count;
						B.Progress = 0.0001f;
					}
				}
				else
				{
					B.Progress += Dt * Sat;
					B.AnimPhase += Dt * 6.f * Sat;
					if (B.Progress >= R.Time)
					{
						B.Progress = 0;
						B.OutBuf[(int32)R.Out.Item] += R.Out.Count;
						if (IsVisible(B.X, B.Y)) AddSound(TEXT("sfx_craft_tick"), C, 0.3f);
					}
				}
			}
			break;
		}
		case EBuildFn::UnitFactory:
		case EBuildFn::ConversionRig:
		case EBuildFn::Hatchery:
		{
			if (B.BuildQueue.Num() > 0)
			{
				const FUnitDef& UD = Unit(B.BuildQueue[0]);
				float Time = (D.Fn == EBuildFn::Hatchery) ? 20.f : UD.BuildTime;
				B.UnitProgress += Dt * Sat;
				B.AnimPhase += Dt * 4.f * Sat;
				if (B.UnitProgress >= Time)
				{
					B.UnitProgress = 0;
					EUnit NU = B.BuildQueue[0];
					B.BuildQueue.RemoveAt(0);
					FVector2D SpawnAt(B.X + D.W * 0.5f, B.Y + D.H + 0.7f);
					SpawnUnit(NU, EFaction::Player, SpawnAt);
					FUnitInst& NewU = Units.Last();
					if (B.RallyPoint.X >= 0)
					{
						NewU.Order.Type = EOrderType::Move;
						NewU.Order.Pos = B.RallyPoint;
					}
					if (D.Fn == EBuildFn::ConversionRig)
					{
						AddSound(TEXT("sfx_warp"), C);
						AddSound(IsDino(NU) ? TEXT("sfx_dino_roar_med") : TEXT("sfx_bot_happy"), C, 0.8f);
						Burst(EParticle::Glow, SpawnAt, 10, FLinearColor(0.4f, 1.f, 0.7f), 2.f, 6.f);
					}
					else if (D.Fn == EBuildFn::Hatchery)
					{
						AddSound(TEXT("sfx_hatch"), C);
						AddText(TEXT("Hatched!"), SpawnAt, FLinearColor(0.6f, 1.f, 0.6f));
					}
					else AddSound(TEXT("sfx_bot_happy"), C, 0.8f);
				}
			}
			break;
		}
		case EBuildFn::Turret:
		case EBuildFn::Watchtower:
		{
			if (B.Faction == EFaction::Player && D.PowerUse > 0 && PowerSat < 0.4f) break; // browned out
			B.AttackCd -= Dt * ((B.Faction == EFaction::Player) ? FMath::Max(0.4f, Sat) : 1.f);
			if (B.AttackCd <= 0)
			{
				int32 TId = FindTargetUnitForTurret(B, D);
				if (TId >= 0)
				{
					FUnitInst* T = FindUnit(TId);
					B.AttackCd = D.Cooldown;
					bool bTesla = (B.Type == EBuilding::TeslaSpire || B.Type == EBuilding::VexTesla);
					if (bTesla)
					{
						// chain zap up to 3 targets
						int32 Hits = 0;
						for (FUnitInst& U : Units)
						{
							if (U.Faction == B.Faction || U.Faction == EFaction::Wild) continue;
							if (FVector2D::Distance(U.Pos, C) > D.Range) continue;
							DamageUnit(U, D.Dmg, B.Faction);
							Burst(EParticle::Spark, U.Pos, 6, FLinearColor(0.5f, 0.8f, 1.f), 3.f, 4.f);
							if (++Hits >= 3) break;
						}
						AddSound(TEXT("sfx_zap"), C, 0.8f);
					}
					else
					{
						FireProjectile(C, T->Pos, TId, -1, D.Dmg, B.Faction,
							B.Faction == EFaction::Vex ? FLinearColor(1, 0.25f, 0.2f) : FLinearColor(1, 0.9f, 0.4f));
						AddSound(B.Type == EBuilding::Watchtower ? TEXT("sfx_laser_small") : TEXT("sfx_turret_fire"), C, 0.6f);
						B.FlashT = 0.08f;
					}
				}
			}
			break;
		}
		case EBuildFn::Trap:
		{
			for (FUnitInst& U : Units)
			{
				if (U.Faction != EFaction::Vex && U.Faction != EFaction::Wild) continue;
				if (Unit(U.Type).bFlies) continue;
				if (FVector2D::Distance(U.Pos, C) < 0.9f)
				{
					DamageUnit(U, D.Dmg * Dt, B.Faction);
					B.HP -= 6.f * Dt;
				}
			}
			if (B.HP <= 0) DestroyBuilding(B);
			break;
		}
		case EBuildFn::Medic:
		{
			if (Sat > 0.3f)
				for (FUnitInst& U : Units)
					if (U.Faction == EFaction::Player && IsDino(U.Type) &&
					    U.HP < Unit(U.Type).HP && FVector2D::Distance(U.Pos, C) < 6.f)
					{
						U.HP = FMath::Min(Unit(U.Type).HP, U.HP + 5.f * Dt);
						if (Rng.FRand() < Dt * 2)
							Burst(EParticle::Glow, U.Pos, 1, FLinearColor(0.4f, 1.f, 0.5f), 0.8f, 3.f);
					}
			break;
		}
		case EBuildFn::FeedTrough:
		{
			for (FUnitInst& U : Units)
				if (U.Faction == EFaction::Player && IsDino(U.Type) &&
				    U.HP < Unit(U.Type).HP && FVector2D::Distance(U.Pos, C) < 5.f)
					U.HP = FMath::Min(Unit(U.Type).HP, U.HP + 1.5f * Dt);
			break;
		}
		case EBuildFn::Paddock:
		{
			// breeding: 2+ same-family player dinos nearby + feed -> egg
			B.Progress += Dt;
			if (B.Progress > 60.f)
			{
				B.Progress = 0;
				if (Stock[(int32)EItem::Feed] >= 10)
				{
					int32 FamCount[16] = {};
					for (const FUnitInst& U : Units)
						if (U.Faction == EFaction::Player && IsDino(U.Type) &&
						    FVector2D::Distance(U.Pos, C) < 6.f)
							FamCount[(int32)FamilyOf(U.Type)]++;
					for (int32 f = 0; f < 16; f++)
						if (f != (int32)EUnitFamily::Robot && FamCount[f] >= 2)
						{
							Stock[(int32)EItem::Feed] -= 10;
							Stock[(int32)EItem::Egg] += 1;
							AddText(TEXT("+1 Egg"), C, FLinearColor(1.f, 0.95f, 0.6f));
							AddSound(TEXT("sfx_pickup_sparkle"), C, 0.7f);
							break;
						}
				}
			}
			break;
		}
		case EBuildFn::EnemySpawner:
		{
			// regrow the local garrison
			B.SpawnT += Dt;
			if (B.SpawnT > 45.f)
			{
				B.SpawnT = 0;
				int32 Nearby = 0;
				for (const FUnitInst& U : Units)
					if (U.Faction == EFaction::Vex && FVector2D::Distance(U.Pos, C) < 12.f) Nearby++;
				if (Nearby < 6)
				{
					static const EUnit Pool[] = { EUnit::VxRaptor, EUnit::VxRaptor, EUnit::VxGunWalker, EUnit::VxTrice };
					SpawnUnit(Pool[Rng.RandRange(0, 3)], EFaction::Vex, C + FVector2D(0, Bld(B.Type).H * 0.5f + 1));
				}
			}
			break;
		}
		default: break;
		}
	}

	// remove dead buildings (flagged by DestroyBuilding via HP<=0 & cleared tiles)
	Buildings.RemoveAll([](const FBuildingInst& B) { return B.HP <= 0; });
}

// haulers: idle carry-units fetch building output buffers into storage
void FDRXSim::TickHaulers(float Dt)
{
	for (FUnitInst& U : Units)
	{
		const FUnitDef& D = Unit(U.Type);
		if (U.Faction != EFaction::Player || D.Carry <= 0) continue;
		if (U.Order.Type != EOrderType::None) continue;
		if (U.Type == EUnit::PlayerAvatar) continue; // avatar hauls only when told

		if (U.CarryCount > 0)
		{
			// deliver to nearest storage
			if (U.JobToB < 0)
			{
				float BestD = 1e9f;
				for (const FBuildingInst& B : Buildings)
				{
					if (B.Faction != EFaction::Player || !B.bConstructed) continue;
					EBuildFn Fn = Bld(B.Type).Fn;
					if (Fn != EBuildFn::Storage && Fn != EBuildFn::HQ) continue;
					float Dd = FVector2D::DistSquared(U.Pos, BuildingCenter(B));
					if (Dd < BestD) { BestD = Dd; U.JobToB = B.Id; }
				}
			}
			FBuildingInst* To = FindBuilding(U.JobToB);
			if (!To) { U.JobToB = -1; continue; }
			if (MoveToward(U, BuildingCenter(*To), Dt, 1.8f))
			{
				Stock[(int32)U.CarryItem] += U.CarryCount;
				AddText(FString::Printf(TEXT("+%d %s"), U.CarryCount, Item(U.CarryItem).Name),
					U.Pos, FLinearColor(0.8f, 0.95f, 1.f));
				U.CarryCount = 0; U.JobToB = -1;
				if (IsVisible((int32)U.Pos.X, (int32)U.Pos.Y)) AddSound(TEXT("sfx_ore_dump"), U.Pos, 0.3f);
			}
			continue;
		}

		// find a building with output waiting
		FBuildingInst* From = FindBuilding(U.JobFromB);
		if (!From)
		{
			U.JobFromB = -1;
			float BestScore = 0;
			for (const FBuildingInst& B : Buildings)
			{
				if (B.Faction != EFaction::Player || !B.bConstructed) continue;
				int32 Total = 0;
				for (int32 i = 0; i < ITEM_COUNT; i++) Total += B.OutBuf[i];
				if (Total <= 0) continue;
				float Dd = FVector2D::Distance(U.Pos, BuildingCenter(B));
				float Score = Total * 10.f - Dd;
				if (Score > BestScore) { BestScore = Score; U.JobFromB = B.Id; }
			}
			From = FindBuilding(U.JobFromB);
		}
		if (!From) continue;
		if (MoveToward(U, BuildingCenter(*From), Dt, 1.8f))
		{
			// load one item type
			int32 Cap = D.Carry + (TechDone[(int32)ETech::LogisticsII] ? 4 : 0);
			for (int32 i = 0; i < ITEM_COUNT; i++)
			{
				if (From->OutBuf[i] > 0)
				{
					int32 Take = FMath::Min(From->OutBuf[i], Cap);
					From->OutBuf[i] -= Take;
					U.CarryItem = (EItem)i; U.CarryCount = Take;
					break;
				}
			}
			U.JobFromB = -1;
		}
	}
}

// ================================================================ movement

static const int32 DirsX[8] = { 1,-1, 0, 0, 1, 1,-1,-1 };
static const int32 DirsY[8] = { 0, 0, 1,-1, 1,-1, 1,-1 };

bool FDRXSim::FindPath(const FUnitInst& U, FVector2D Dest, TArray<FVector2D>& Out) const
{
	const bool bFly = Unit(U.Type).bFlies;
	FIntPoint S((int32)U.Pos.X, (int32)U.Pos.Y);
	FIntPoint E((int32)Dest.X, (int32)Dest.Y);
	if (!InBounds(E.X, E.Y)) return false;
	if (bFly) { Out.Reset(); Out.Add(Dest); return true; }

	// nudge destination to a free tile
	if (IsBlocked(E.X, E.Y, false))
	{
		bool bFound = false;
		for (int32 r = 1; r <= 4 && !bFound; r++)
			for (int32 d = 0; d < 8; d++)
			{
				FIntPoint C(E.X + DirsX[d] * r, E.Y + DirsY[d] * r);
				if (!IsBlocked(C.X, C.Y, false)) { E = C; bFound = true; break; }
			}
		if (!bFound) return false;
	}

	// A* (bounded)
	struct FNode { int32 Idx; float F; };
	TArray<float> G; G.Init(1e9f, MapW * MapH);
	TArray<int32> Parent; Parent.Init(-1, MapW * MapH);
	TArray<FNode> Open;
	auto H = [&](int32 Idx) {
		int32 x = Idx % MapW, y = Idx / MapW;
		return FVector2D::Distance(FVector2D(x, y), FVector2D(E.X, E.Y)); };
	int32 SIdx = S.Y * MapW + S.X, EIdx = E.Y * MapW + E.X;
	G[SIdx] = 0;
	Open.Add({SIdx, (float)H(SIdx)});
	int32 Expanded = 0;
	while (Open.Num() > 0 && Expanded < 4000)
	{
		int32 BestI = 0;
		for (int32 i = 1; i < Open.Num(); i++) if (Open[i].F < Open[BestI].F) BestI = i;
		FNode N = Open[BestI]; Open.RemoveAtSwap(BestI);
		if (N.Idx == EIdx) break;
		Expanded++;
		int32 nx = N.Idx % MapW, ny = N.Idx / MapW;
		for (int32 d = 0; d < 8; d++)
		{
			int32 cx = nx + DirsX[d], cy = ny + DirsY[d];
			if (IsBlocked(cx, cy, false)) continue;
			if (d >= 4 && (IsBlocked(nx + DirsX[d], ny, false) || IsBlocked(nx, ny + DirsY[d], false))) continue; // no corner cutting
			int32 CIdx = cy * MapW + cx;
			float NG = G[N.Idx] + ((d < 4) ? 1.f : 1.42f);
			if (NG < G[CIdx])
			{
				G[CIdx] = NG; Parent[CIdx] = N.Idx;
				Open.Add({CIdx, (float)(NG + H(CIdx))});
			}
		}
	}
	if (Parent[EIdx] < 0 && EIdx != SIdx) return false;
	Out.Reset();
	int32 Cur = EIdx;
	while (Cur >= 0 && Cur != SIdx)
	{
		Out.Insert(FVector2D(Cur % MapW + 0.5f, Cur / MapW + 0.5f), 0);
		Cur = Parent[Cur];
	}
	if (Out.Num() > 0) Out.Last() = Dest.ClampAxes(1.5f, MapW - 1.5f);
	return Out.Num() > 0;
}

bool FDRXSim::MoveToward(FUnitInst& U, FVector2D Dest, float Dt, float ArriveDist)
{
	const FUnitDef& D = Unit(U.Type);
	float Dist = FVector2D::Distance(U.Pos, Dest);
	if (Dist <= ArriveDist) { U.Path.Reset(); return true; }

	float Speed = D.Speed * (U.StarveT > 5.f ? 0.5f : 1.f);
	U.AnimPhase += Dt * Speed * 3.2f;

	// direct if line free (or flying)
	FVector2D Next = Dest;
	if (!D.bFlies)
	{
		bool bNeedPath = false;
		FVector2D Dir = (Dest - U.Pos).GetSafeNormal();
		FVector2D Probe = U.Pos + Dir * FMath::Min(1.2f, Dist);
		if (IsBlocked((int32)Probe.X, (int32)Probe.Y, false)) bNeedPath = true;

		if (bNeedPath || U.Path.Num() > 0)
		{
			U.RepathT -= Dt;
			if (U.Path.Num() == 0 || U.PathIdx >= U.Path.Num() ||
			    (U.RepathT <= 0 && FVector2D::Distance(U.Path.Last(), Dest) > 2.5f))
			{
				U.RepathT = 1.5f;
				U.PathIdx = 0;
				if (!FindPath(U, Dest, U.Path))
				{
					U.Path.Reset();
					return Dist < ArriveDist + 1.5f; // treat as arrived-ish if stuck
				}
			}
			if (U.PathIdx < U.Path.Num())
			{
				Next = U.Path[U.PathIdx];
				if (FVector2D::Distance(U.Pos, Next) < 0.35f) { U.PathIdx++; if (U.PathIdx < U.Path.Num()) Next = U.Path[U.PathIdx]; }
			}
		}
	}

	FVector2D Dir = (Next - U.Pos).GetSafeNormal();
	FVector2D NewPos = U.Pos + Dir * Speed * Dt;
	if (!D.bFlies && IsBlocked((int32)NewPos.X, (int32)NewPos.Y, false))
	{
		// slide
		FVector2D Alt1(U.Pos.X + Dir.X * Speed * Dt, U.Pos.Y);
		FVector2D Alt2(U.Pos.X, U.Pos.Y + Dir.Y * Speed * Dt);
		if (!IsBlocked((int32)Alt1.X, (int32)Alt1.Y, false)) NewPos = Alt1;
		else if (!IsBlocked((int32)Alt2.X, (int32)Alt2.Y, false)) NewPos = Alt2;
		else { U.Path.Reset(); U.RepathT = 0; return false; }
	}
	if (FMath::Abs(Dir.X) > 0.05f) U.FacingX = (Dir.X > 0) ? 1.f : -1.f;
	U.FacingAxis = (FMath::Abs(Dir.X) >= FMath::Abs(Dir.Y)) ? 0 : (Dir.Y < 0 ? -1 : 1);
	U.Pos = NewPos.ClampAxes(1.2f, MapW - 1.2f);
	return FVector2D::Distance(U.Pos, Dest) <= ArriveDist;
}

// ================================================================ units

int32 FDRXSim::FindTargetUnit(const FUnitInst& U, float Range) const
{
	const FUnitDef& D = Unit(U.Type);
	int32 Best = -1; float BestD = Range;
	for (const FUnitInst& T : Units)
	{
		if (T.Faction == U.Faction) continue;
		if (U.Faction != EFaction::Wild && T.Faction == EFaction::Wild) continue; // armies ignore fauna unless ordered
		if (T.Type == EUnit::WdTurtle) continue;
		if (Unit(T.Type).bFlies && !D.bHitsAir) continue;
		float Dd = FVector2D::Distance(U.Pos, T.Pos);
		if (Dd < BestD) { BestD = Dd; Best = T.Id; }
	}
	return Best;
}

int32 FDRXSim::FindTargetUnitForTurret(const FBuildingInst& B, const FBuildingDef& D) const
{
	FVector2D C = BuildingCenter(B);
	int32 Best = -1; float BestD = D.Range;
	for (const FUnitInst& T : Units)
	{
		if (T.Faction == B.Faction) continue;
		if (B.Faction == EFaction::Player && T.Faction == EFaction::Wild)
		{
			// only shoot wild predators that are close & hostile-ish
			EUnitFamily F = Unit(T.Type).Family;
			bool bPredator = (F == EUnitFamily::Raptor || F == EUnitFamily::Rex ||
			                  F == EUnitFamily::Spino || T.Type == EUnit::WdSmilodon || T.Type == EUnit::WdThief);
			if (!bPredator) continue;
		}
		if (Unit(T.Type).bFlies && !D.bHitsAir) continue;
		float Dd = FVector2D::Distance(C, T.Pos);
		if (Dd < BestD) { BestD = Dd; Best = T.Id; }
	}
	return Best;
}

int32 FDRXSim::FindTargetBuilding(const FUnitInst& U, float Range) const
{
	int32 Best = -1; float BestD = Range;
	for (const FBuildingInst& B : Buildings)
	{
		if (B.Faction == U.Faction || B.HP <= 0) continue;
		if (U.Faction == EFaction::Wild) continue;
		float Dd = FVector2D::Distance(U.Pos, BuildingCenter(const_cast<FBuildingInst&>(B)));
		if (Dd < BestD) { BestD = Dd; Best = B.Id; }
	}
	return Best;
}

void FDRXSim::FireProjectile(FVector2D From, FVector2D To, int32 TargetUnit, int32 TargetBld,
                             float Dmg, EFaction F, FLinearColor Color, bool bArc, float AoE)
{
	FProjectile P;
	P.Pos = From; P.Start = From; P.Target = To;
	P.Speed = bArc ? 7.f : 13.f;
	P.Dmg = Dmg; P.TargetUnit = TargetUnit; P.TargetBuilding = TargetBld;
	P.Faction = F; P.Color = Color; P.bArc = bArc;
	P.bAoE = AoE > 0; P.AoERadius = AoE;
	P.T0 = FVector2D::Distance(From, To) / P.Speed;
	Projectiles.Add(P);
}

void FDRXSim::UnitAttack(FUnitInst& U, FUnitInst* TU, FBuildingInst* TB)
{
	const FUnitDef& D = Unit(U.Type);
	float DmgMult = 1.f + U.Rank * 0.15f;
	float Dmg = D.Dmg * DmgMult;
	if (U.Type == EUnit::CyTriceRhino && TB) Dmg *= 3.f;
	if (U.Type == EUnit::CyAnkyWreck && TB) Dmg *= 2.f;
	FVector2D TPos = TU ? TU->Pos : BuildingCenter(*TB);

	U.LungeT = 0.22f;
	U.SquashT = 0.18f;
	if (D.AttackSfx && IsVisible((int32)U.Pos.X, (int32)U.Pos.Y)) AddSound(D.AttackSfx, U.Pos, 0.55f);

	if (D.Range > 1.6f)
	{
		bool bArc = (U.Type == EUnit::CyTriceMissile || U.Type == EUnit::SiegeTank ||
		             U.Type == EUnit::VxSiege || U.Type == EUnit::CyAnkyCannon);
		FLinearColor C = (U.Faction == EFaction::Vex) ? FLinearColor(1, 0.25f, 0.2f)
			: IsDino(U.Type) ? FLinearColor(0.5f, 1.f, 0.6f) : FLinearColor(0.4f, 0.8f, 1.f);
		FireProjectile(U.Pos - FVector2D(0, 0.4f), TPos, TU ? TU->Id : -1, TB ? TB->Id : -1,
			Dmg, U.Faction, C, bArc, bArc ? 1.2f : 0.f);
		Burst(EParticle::Muzzle, U.Pos + FVector2D(U.FacingX * 0.4f, -0.4f), 2, FLinearColor(1, 0.9f, 0.5f), 1.f, 4.f);
	}
	else
	{
		if (TU) DamageUnit(*TU, Dmg, U.Faction);
		else if (TB) DamageBuilding(*TB, Dmg);
		Burst(EParticle::Spark, TPos, 3, FLinearColor(1, 0.8f, 0.4f), 2.f, 3.f);
	}
}

void FDRXSim::DamageUnit(FUnitInst& U, float Dmg, EFaction Attacker)
{
	const FUnitDef& D = Unit(U.Type);
	float Final = FMath::Max(1.f, Dmg - D.Armor);
	U.HP -= Final;
	U.FlashT = 0.12f;
	Burst(IsDino(U.Type) ? EParticle::Blood : EParticle::Spark, U.Pos, 3,
		IsDino(U.Type) ? FLinearColor(0.8f, 0.15f, 0.1f) : FLinearColor(1.f, 0.8f, 0.3f), 2.f, 3.5f);

	// weakened (tameable) wild state
	if (U.Faction == EFaction::Wild && U.HP > 0 && U.HP < D.HP * 0.22f && D.CyberForm != EUnit::COUNT)
		U.bWeakened = true;

	if (U.HP <= 0) { KillUnit(U, Attacker); return; }

	// retaliate / flee
	if (U.Faction == EFaction::Wild)
	{
		EUnitFamily F = D.Family;
		bool bFights = (F == EUnitFamily::Raptor || F == EUnitFamily::Rex || F == EUnitFamily::Spino ||
		                F == EUnitFamily::Trice || F == EUnitFamily::Anky || F == EUnitFamily::Stego ||
		                U.Type == EUnit::WdSmilodon || U.Type == EUnit::WdToad);
		if (U.bWeakened || !bFights)
		{
			// flee away
			U.Order.Type = EOrderType::Move;
			FVector2D Away = U.Pos + (U.Pos - FVector2D(MapW / 2, MapH / 2)).GetSafeNormal() * -0.f; // placeholder
			// run from nearest enemy
			float BestD = 1e9f; FVector2D From = U.Pos;
			for (const FUnitInst& O : Units)
				if (O.Faction != EFaction::Wild)
				{
					float Dd = FVector2D::DistSquared(O.Pos, U.Pos);
					if (Dd < BestD) { BestD = Dd; From = O.Pos; }
				}
			U.Order.Pos = (U.Pos + (U.Pos - From).GetSafeNormal() * 9.f).ClampAxes(2, MapW - 2);
			U.Path.Reset(); U.PathIdx = 0;
		}
	}
	else if (U.Order.Type == EOrderType::None && U.Stance != EStance::Hold)
	{
		// find who hit us (nearest enemy) and fight back
		int32 T = FindTargetUnit(U, 8.f);
		if (T >= 0) { U.Order.Type = EOrderType::AttackTarget; U.Order.TargetUnit = T; }
	}
}

void FDRXSim::DamageBuilding(FBuildingInst& B, float Dmg)
{
	B.HP -= Dmg;
	B.FlashT = 0.12f;
	if (B.HP <= 0) DestroyBuilding(B);
}

void FDRXSim::KillUnit(FUnitInst& U, EFaction Killer)
{
	const FUnitDef& D = Unit(U.Type);
	if (D.DeathSfx) AddSound(D.DeathSfx, U.Pos, 0.8f);
	Burst(IsDino(U.Type) ? EParticle::Blood : EParticle::Explosion, U.Pos, 10,
		IsDino(U.Type) ? FLinearColor(0.75f, 0.12f, 0.08f) : FLinearColor(1.f, 0.6f, 0.2f), 3.f, 6.f);

	if (U.Faction == EFaction::Wild && Killer == EFaction::Player && D.MeatDrop > 0)
	{
		Stock[(int32)EItem::Meat] += D.MeatDrop;
		AddText(FString::Printf(TEXT("+%d Meat"), D.MeatDrop), U.Pos, FLinearColor(1.f, 0.6f, 0.55f));
	}
	if (U.Type == EUnit::WdThief && U.CarryCount > 0)
		Stock[(int32)U.CarryItem] += U.CarryCount + 3;

	if (Killer == EFaction::Player) KillCount++;
	if (U.Faction == EFaction::Player)
	{
		LossCount++;
		if (U.Id == AvatarId) { AvatarRespawnT = 20.f; AddAlert(TEXT("Dr. Kade down! Reprinting in 20s..."), FLinearColor(1, 0.4f, 0.3f)); }
		else if (IsDino(U.Type)) AddAlert(FString::Printf(TEXT("%s lost."), D.Name), FLinearColor(1, 0.5f, 0.4f), U.Pos);
	}
	// credit killer veterancy
	if (Killer == EFaction::Player || Killer == EFaction::Vex)
		for (FUnitInst& K : Units)
			if (K.Faction == Killer && K.Order.TargetUnit == U.Id)
			{ K.Kills += 1; K.Rank = FMath::Min(3, (int32)(K.Kills / 3)); break; }

	U.HP = -1; // marks for removal in TickUnits
}

void FDRXSim::DestroyBuilding(FBuildingInst& B)
{
	const FBuildingDef& D = Bld(B.Type);
	FVector2D C = BuildingCenter(B);
	for (int32 y = 0; y < D.H; y++)
	for (int32 x = 0; x < D.W; x++)
		if (InBounds(B.X + x, B.Y + y) && At(B.X + x, B.Y + y).BuildingId == B.Id)
			At(B.X + x, B.Y + y).BuildingId = -1;

	AddSound(D.W >= 3 ? TEXT("sfx_explosion_large") : TEXT("sfx_explosion_med"), C);
	Burst(EParticle::Explosion, C, 16, FLinearColor(1.f, 0.55f, 0.15f), 4.f, 8.f);
	Burst(EParticle::Puff, C, 10, FLinearColor(0.4f, 0.4f, 0.4f), 2.f, 9.f);

	if (B.Faction == EFaction::Vex)
	{
		if (B.Type == EBuilding::VexSpawner)
		{
			CampsCleared++;
			LootCamp(C);
		}
		else if (B.Type == EBuilding::VexCitadel)
		{
			bVictory = true;
			AddSound(TEXT("sfx_explosion_large"));
			AddSound(TEXT("sfx_ann_victory"));
		}
		Escalation += 8.f;
	}
	else if (B.Id == PlayerHQ)
	{
		bDefeat = true;
		AddSound(TEXT("sfx_ann_gameover"));
	}
	B.HP = -1; // flag for removal
}

void FDRXSim::LootCamp(FVector2D P)
{
	int32 Mods = Rng.RandRange(4, 8), Shards = Rng.RandRange(1, 3);
	Stock[(int32)EItem::CyberModule] += Mods;
	Stock[(int32)EItem::ChronoShard] += Shards;
	AddText(FString::Printf(TEXT("Salvage! +%d Cyber Modules"), Mods), P, FLinearColor(0.5f, 1.f, 0.7f));
	AddText(FString::Printf(TEXT("+%d Chrono Shards"), Shards), P + FVector2D(0, 1), FLinearColor(0.6f, 0.8f, 1.f));
	AddAlert(FString::Printf(TEXT("Outpost destroyed! (%d cleared)"), CampsCleared), FLinearColor(0.5f, 1.f, 0.6f), P);
	AddSound(TEXT("sfx_ann_dominating"));
	AddSound(TEXT("sfx_gem"), P);
}

void FDRXSim::TryTameNearby(FUnitInst& PlayerUnit)
{
	if (Stock[(int32)EItem::Feed] < 5) return;
	for (FUnitInst& W : Units)
	{
		if (W.Faction != EFaction::Wild || !W.bWeakened) continue;
		if (Unit(W.Type).CyberForm == EUnit::COUNT) continue;
		if (FVector2D::Distance(W.Pos, PlayerUnit.Pos) > 1.8f) continue;
		if (UsedBandwidth() + 2 > MaxBandwidth()) continue;
		Stock[(int32)EItem::Feed] -= 5;
		W.Faction = EFaction::Player;
		W.bWeakened = false;
		W.HP = Unit(W.Type).HP * 0.5f;
		W.Order = FOrder();
		W.Stance = EStance::Defensive;
		AddText(TEXT("TAMED!"), W.Pos, FLinearColor(0.5f, 1.f, 0.5f));
		AddSound(TEXT("sfx_dino_call"), W.Pos);
		AddSound(TEXT("sfx_pickup_sparkle"), W.Pos, 0.7f);
		Burst(EParticle::Glow, W.Pos, 12, FLinearColor(0.5f, 1.f, 0.6f), 2.f, 5.f);
		return;
	}
}

void FDRXSim::TickUnits(float Dt)
{
	for (FUnitInst& U : Units)
	{
		if (U.HP <= 0) continue;
		const FUnitDef& D = Unit(U.Type);
		U.FlashT = FMath::Max(0.f, U.FlashT - Dt);
		U.SquashT = FMath::Max(0.f, U.SquashT - Dt);
		U.LungeT = FMath::Max(0.f, U.LungeT - Dt);
		U.AttackCd = FMath::Max(0.f, U.AttackCd - Dt);

		if (U.Faction == EFaction::Wild) { TickWild(U, Dt); continue; }

		// direct WASD control of Dr. Kade overrides any standing order
		if (U.Id == AvatarId && !AvatarMoveDir.IsNearlyZero())
		{
			U.Order = FOrder();
			U.Path.Reset();
			FVector2D Dir = AvatarMoveDir.GetSafeNormal();
			FVector2D NewPos = U.Pos + Dir * D.Speed * Dt;
			if (IsBlocked((int32)NewPos.X, (int32)NewPos.Y, false))
			{
				FVector2D AltX(U.Pos.X + Dir.X * D.Speed * Dt, U.Pos.Y);
				FVector2D AltY(U.Pos.X, U.Pos.Y + Dir.Y * D.Speed * Dt);
				if (!IsBlocked((int32)AltX.X, (int32)AltX.Y, false)) NewPos = AltX;
				else if (!IsBlocked((int32)AltY.X, (int32)AltY.Y, false)) NewPos = AltY;
				else NewPos = U.Pos;
			}
			U.Pos = NewPos.ClampAxes(1.2f, MapW - 1.2f);
			if (FMath::Abs(Dir.X) > 0.05f) U.FacingX = (Dir.X > 0) ? 1.f : -1.f;
			U.FacingAxis = (FMath::Abs(Dir.X) >= FMath::Abs(Dir.Y)) ? 0 : (Dir.Y < 0 ? -1 : 1);
			U.AnimPhase += Dt * D.Speed * 3.2f;
			U.HomePos = U.Pos;
			// walking up to a weakened dino tames it
			U.WanderT -= Dt;
			if (U.WanderT <= 0) { U.WanderT = 0.4f; TryTameNearby(U); }
			continue;
		}

		TickCombatUnit(U, Dt);
	}
	Units.RemoveAll([](const FUnitInst& U) { return U.HP <= 0; });
}

void FDRXSim::TickCombatUnit(FUnitInst& U, float Dt)
{
	const FUnitDef& D = Unit(U.Type);
	const float Range = FMath::Max(1.2f, D.Range);

	switch (U.Order.Type)
	{
	case EOrderType::Move:
		if (MoveToward(U, U.Order.Pos, Dt, 0.4f))
		{
			U.Order = FOrder();
			U.HomePos = U.Pos;
			if (U.Faction == EFaction::Player && U.Id == AvatarId) TryTameNearby(U);
		}
		break;

	case EOrderType::Patrol:
	{
		FVector2D Dest = U.bPatrolToB ? U.PatrolB : U.PatrolA;
		if (MoveToward(U, Dest, Dt, 0.6f)) U.bPatrolToB = !U.bPatrolToB;
		if (D.Dmg > 0)
		{
			int32 T = FindTargetUnit(U, Range + 2.f);
			if (T >= 0) { U.Order.Type = EOrderType::AttackTarget; U.Order.TargetUnit = T; }
		}
		break;
	}

	case EOrderType::AttackMove:
	{
		if (D.Dmg > 0)
		{
			int32 T = FindTargetUnit(U, FMath::Max(Range, 6.f));
			if (T < 0 && FVector2D::Distance(U.Pos, U.Order.Pos) < 10.f)
			{
				int32 TB = FindTargetBuilding(U, FMath::Max(Range, 5.f));
				if (TB >= 0)
				{
					FBuildingInst* B = FindBuilding(TB);
					float Dd = FVector2D::Distance(U.Pos, BuildingCenter(*B));
					if (Dd <= Range + 1.f)
					{
						if (U.AttackCd <= 0) { UnitAttack(U, nullptr, B); U.AttackCd = D.Cooldown; }
					}
					else MoveToward(U, BuildingCenter(*B), Dt, Range);
					break;
				}
			}
			if (T >= 0)
			{
				FUnitInst* TU = FindUnit(T);
				float Dd = FVector2D::Distance(U.Pos, TU->Pos);
				if (Dd <= Range)
				{
					if (U.AttackCd <= 0) { UnitAttack(U, TU, nullptr); U.AttackCd = D.Cooldown; }
					U.FacingX = (TU->Pos.X > U.Pos.X) ? 1.f : -1.f;
				}
				else MoveToward(U, TU->Pos, Dt, Range * 0.9f);
				break;
			}
		}
		if (MoveToward(U, U.Order.Pos, Dt, 0.8f)) { U.Order = FOrder(); U.HomePos = U.Pos; }
		break;
	}

	case EOrderType::AttackTarget:
	{
		FUnitInst* TU = FindUnit(U.Order.TargetUnit);
		FBuildingInst* TB = (U.Order.TargetBuilding >= 0) ? FindBuilding(U.Order.TargetBuilding) : nullptr;
		if (TB && TB->HP <= 0) TB = nullptr;
		if (!TU && !TB) { U.Order = FOrder(); break; }
		FVector2D TPos = TU ? TU->Pos : BuildingCenter(*TB);
		float Dd = FVector2D::Distance(U.Pos, TPos);
		float EffRange = TB ? Range + FMath::Max(Bld(TB->Type).W, Bld(TB->Type).H) * 0.5f : Range;
		if (Dd <= EffRange)
		{
			if (U.AttackCd <= 0 && D.Dmg > 0) { UnitAttack(U, TU, TB); U.AttackCd = D.Cooldown; }
			U.FacingX = (TPos.X > U.Pos.X) ? 1.f : -1.f;
		}
		else
		{
			// defensive leash
			if (U.Stance == EStance::Defensive && FVector2D::Distance(U.HomePos, TPos) > 8.f && U.Faction == EFaction::Player)
			{ U.Order = FOrder(); U.Order.Type = EOrderType::Move; U.Order.Pos = U.HomePos; break; }
			if (U.Stance == EStance::Hold) { U.Order = FOrder(); break; }
			MoveToward(U, TPos, Dt, EffRange * 0.9f);
		}
		break;
	}

	case EOrderType::Harvest:
	{
		if (D.Carry <= 0) { U.Order = FOrder(); break; }
		FIntPoint HT = U.Order.HarvestTile;
		if (!InBounds(HT.X, HT.Y)) { U.Order = FOrder(); break; }
		FTile& T = At(HT.X, HT.Y);
		EItem It = ResNodeItem(T.Res);

		int32 Cap = D.Carry + (TechDone[(int32)ETech::LogisticsII] ? 4 : 0);
		if (U.CarryCount >= Cap || (It == EItem::COUNT && U.CarryCount > 0))
		{
			// deliver, then come back
			float BestD = 1e9f; int32 StoreId = -1;
			for (const FBuildingInst& B : Buildings)
			{
				if (B.Faction != EFaction::Player || !B.bConstructed) continue;
				EBuildFn Fn = Bld(B.Type).Fn;
				if (Fn != EBuildFn::Storage && Fn != EBuildFn::HQ) continue;
				float Sd = FVector2D::DistSquared(U.Pos, BuildingCenter(const_cast<FBuildingInst&>(B)));
				if (Sd < BestD) { BestD = Sd; StoreId = B.Id; }
			}
			FBuildingInst* S = FindBuilding(StoreId);
			if (!S) { U.Order = FOrder(); break; }
			if (MoveToward(U, BuildingCenter(*S), Dt, 1.8f))
			{
				Stock[(int32)U.CarryItem] += U.CarryCount;
				AddText(FString::Printf(TEXT("+%d %s"), U.CarryCount, Item(U.CarryItem).Name), U.Pos, FLinearColor(0.8f, 0.95f, 1.f));
				U.CarryCount = 0;
				if (It == EItem::COUNT) U.Order = FOrder(); // node gone
			}
			break;
		}
		if (It == EItem::COUNT)
		{
			// node exhausted: look for same-type neighbor
			bool bFound = false;
			for (int32 r = 1; r <= 3 && !bFound; r++)
				for (int32 dy = -r; dy <= r && !bFound; dy++)
				for (int32 dx = -r; dx <= r; dx++)
				{
					int32 nx = HT.X + dx, ny = HT.Y + dy;
					if (InBounds(nx, ny) && ResNodeItem(At(nx, ny).Res) == U.CarryItem && At(nx, ny).ResAmount > 0)
					{ U.Order.HarvestTile = FIntPoint(nx, ny); bFound = true; break; }
				}
			if (!bFound && U.CarryCount == 0) U.Order = FOrder();
			break;
		}
		FVector2D NodeP(HT.X + 0.5f, HT.Y + 0.5f);
		if (MoveToward(U, NodeP, Dt, 1.1f))
		{
			U.AnimPhase += Dt * 8.f;
			U.Order.Pos.X += Dt; // reuse as harvest timer
			if (U.Order.Pos.X >= 1.4f)
			{
				U.Order.Pos.X = 0;
				if (U.CarryCount > 0 && U.CarryItem != It) { /* deliver first */ U.CarryCount = U.CarryCount; }
				U.CarryItem = It;
				U.CarryCount++;
				T.ResAmount--;
				U.SquashT = 0.15f;
				if (IsVisible(HT.X, HT.Y))
				{
					if (T.Res == EResNode::Tree) AddSound(TEXT("sfx_hit_thud"), NodeP, 0.3f);
					else AddSound(TEXT("sfx_mine_hit2"), NodeP, 0.035f);
				}
				Burst(EParticle::Puff, NodeP, 2,
					T.Res == EResNode::Tree ? FLinearColor(0.35f, 0.6f, 0.25f) : FLinearColor(0.6f, 0.55f, 0.45f), 1.4f, 4.f);
				if (T.ResAmount <= 0) { T.Res = EResNode::None; T.DecorVariant = -1; }
			}
		}
		break;
	}

	default: // idle
	{
		if (U.Faction == EFaction::Vex)
		{
			// garrison: aggro nearby players
			int32 T = FindTargetUnit(U, 7.f);
			if (T >= 0) { U.Order.Type = EOrderType::AttackTarget; U.Order.TargetUnit = T; break; }
			if (FVector2D::Distance(U.Pos, U.HomePos) > 2.f)
			{ U.Order.Type = EOrderType::Move; U.Order.Pos = U.HomePos; }
			break;
		}
		if (D.Dmg > 0 && U.Stance != EStance::Hold)
		{
			float Aggro = (U.Stance == EStance::Aggressive) ? 8.f : 5.f;
			int32 T = FindTargetUnit(U, Aggro);
			if (T >= 0)
			{
				FUnitInst* TU = FindUnit(T);
				if (TU->Faction == EFaction::Vex ||
				    (TU->Faction == EFaction::Wild && U.Stance == EStance::Aggressive))
				{ U.Order.Type = EOrderType::AttackTarget; U.Order.TargetUnit = T; }
			}
		}
		else if (U.Stance == EStance::Hold && D.Dmg > 0 && U.AttackCd <= 0)
		{
			int32 T = FindTargetUnit(U, Range);
			if (T >= 0)
			{
				FUnitInst* TU = FindUnit(T);
				if (TU->Faction != EFaction::Wild) { UnitAttack(U, TU, nullptr); U.AttackCd = D.Cooldown; }
			}
		}
		break;
	}
	}
}

void FDRXSim::TickWild(FUnitInst& U, float Dt)
{
	const FUnitDef& D = Unit(U.Type);

	// thief special: raid the stockpile (grace period at game start)
	if (U.Type == EUnit::WdThief && U.CarryCount == 0 && GameTime > 150.f)
	{
		FBuildingInst* HQ = FindBuilding(PlayerHQ);
		if (HQ && FVector2D::Distance(U.Pos, BuildingCenter(*HQ)) < 30.f)
		{
			if (MoveToward(U, BuildingCenter(*HQ), Dt, 2.2f))
			{
				for (int32 i = 0; i < ITEM_COUNT; i++)
				{
					int32 Idx = (i + Rng.RandRange(0, ITEM_COUNT - 1)) % ITEM_COUNT;
					if (Stock[Idx] >= 3)
					{
						Stock[Idx] -= 3; U.CarryItem = (EItem)Idx; U.CarryCount = 3;
						AddAlert(TEXT("A Basket Thief stole from your stockpile!"), FLinearColor(1, 0.7f, 0.3f), U.Pos);
						AddSound(TEXT("sfx_jingle_neg"));
						break;
					}
				}
			}
			return;
		}
	}
	if (U.Type == EUnit::WdThief && U.CarryCount > 0)
	{
		MoveToward(U, FVector2D(3, U.Pos.Y), Dt, 1.f);
		if (U.Pos.X < 5) U.HP = -1; // escaped
		return;
	}

	// active order (usually fleeing)
	if (U.Order.Type == EOrderType::Move)
	{
		if (MoveToward(U, U.Order.Pos, Dt, 0.6f)) U.Order = FOrder();
		return;
	}
	if (U.Order.Type == EOrderType::AttackTarget)
	{
		FUnitInst* TU = FindUnit(U.Order.TargetUnit);
		if (!TU || FVector2D::Distance(U.Pos, TU->Pos) > 11.f) { U.Order = FOrder(); return; }
		float Range = FMath::Max(1.1f, D.Range);
		if (FVector2D::Distance(U.Pos, TU->Pos) <= Range)
		{
			if (U.AttackCd <= 0) { UnitAttack(U, TU, nullptr); U.AttackCd = D.Cooldown; }
		}
		else MoveToward(U, TU->Pos, Dt, Range * 0.9f);
		return;
	}

	// predators hunt
	EUnitFamily F = D.Family;
	bool bPredator = (F == EUnitFamily::Raptor || F == EUnitFamily::Rex || F == EUnitFamily::Spino ||
	                  U.Type == EUnit::WdSmilodon);
	if (bPredator && !U.bWeakened)
	{
		float Aggro = bDay ? 3.5f : 5.5f;
		int32 Best = -1; float BestD = Aggro;
		for (const FUnitInst& T : Units)
		{
			if (T.Id == U.Id || T.Faction == EFaction::Vex) continue;
			if (T.Faction == EFaction::Wild)
			{
				EUnitFamily TF = Unit(T.Type).Family;
				bool bPrey = (TF == EUnitFamily::Galli || TF == EUnitFamily::Para);
				if (!bPrey || Rng.FRand() > 0.002f) continue; // rare herd hunts
			}
			if (Unit(T.Type).bFlies) continue;
			float Dd = FVector2D::Distance(U.Pos, T.Pos);
			if (Dd < BestD) { BestD = Dd; Best = T.Id; }
		}
		if (Best >= 0)
		{
			U.Order.Type = EOrderType::AttackTarget;
			U.Order.TargetUnit = Best;
			if (IsVisible((int32)U.Pos.X, (int32)U.Pos.Y) && Rng.FRand() < 0.5f)
				AddSound(D.AttackSfx ? D.AttackSfx : TEXT("sfx_dino_roar_med"), U.Pos, 0.5f);
			return;
		}
	}

	// wander
	U.WanderT -= Dt;
	if (U.WanderT <= 0)
	{
		U.WanderT = Rng.FRandRange(3.f, 9.f);
		FVector2D Dest = U.HomePos + FVector2D(Rng.FRandRange(-5, 5), Rng.FRandRange(-5, 5));
		U.Order.Type = EOrderType::Move;
		U.Order.Pos = Dest.ClampAxes(3, MapW - 3);
		// ambient call
		if (IsVisible((int32)U.Pos.X, (int32)U.Pos.Y) && Rng.FRand() < 0.12f)
			AddSound(TEXT("sfx_roar_distant"), U.Pos, 0.25f);
	}
}

// ================================================================ raids

void FDRXSim::TickRaids(float Dt)
{
	Escalation += Dt * 0.02f;
	int32 Era = 0;
	for (int32 t = 0; t < TECH_COUNT; t++) if (TechDone[t]) Era = FMath::Max(Era, Tech((ETech)t).Era);
	Escalation += Era * Dt * 0.01f;

	RaidTimer -= Dt;
	if (RaidTimer < 20.f && !bRaidWarned)
	{
		bRaidWarned = true;
		// pick source camp now for the warning ping
		FVector2D HQPos = SpawnPos;
		if (FBuildingInst* HQ = FindBuilding(PlayerHQ)) HQPos = BuildingCenter(*HQ);
		float BestD = 1e18f; LastRaidFrom = FVector2D(MapW - 8, MapH / 2);
		for (FBuildingInst& B : Buildings)
			if (B.Faction == EFaction::Vex && B.Type == EBuilding::VexSpawner && B.HP > 0)
			{
				float Dd = FVector2D::DistSquared(BuildingCenter(B), HQPos);
				if (Dd < BestD) { BestD = Dd; LastRaidFrom = BuildingCenter(B); }
			}
		AddAlert(TEXT("RAID INCOMING from the east!"), FLinearColor(1, 0.25f, 0.2f), LastRaidFrom);
		AddSound(TEXT("sfx_alarm_raid"));
		AddSound(TEXT("sfx_ann_fight"));
	}
	if (RaidTimer > 0) return;

	// launch
	RaidWave++;
	bRaidWarned = false;
	RaidTimer = FMath::Max(95.f, 210.f - Escalation * 2.f);

	float Budget = 3.f + RaidWave * 1.5f + Escalation * 0.5f;
	FVector2D HQPos = SpawnPos;
	if (FBuildingInst* HQ = FindBuilding(PlayerHQ)) HQPos = BuildingCenter(*HQ);

	struct FPick { EUnit U; float Cost; int32 MinWave; };
	static const FPick Picks[] = {
		{EUnit::VxRaptor, 1.f, 0}, {EUnit::VxGunWalker, 1.5f, 0}, {EUnit::VxTrice, 3.f, 2},
		{EUnit::VxAnky, 3.f, 3}, {EUnit::VxPtera, 2.f, 3}, {EUnit::VxGunship, 4.f, 5},
		{EUnit::VxSiege, 5.f, 5}, {EUnit::VxRex, 8.f, 7},
	};
	int32 N = 0;
	while (Budget > 0.9f && N < 24)
	{
		TArray<const FPick*> Avail;
		for (const FPick& P : Picks)
			if (P.MinWave <= RaidWave && P.Cost <= Budget) Avail.Add(&P);
		if (Avail.Num() == 0) break;
		const FPick* P = Avail[Rng.RandRange(0, Avail.Num() - 1)];
		Budget -= P->Cost;
		FVector2D SpawnP = LastRaidFrom + FVector2D(Rng.FRandRange(-3, 3), Rng.FRandRange(-3, 3));
		SpawnUnit(P->U, EFaction::Vex, SpawnP, EStance::Aggressive);
		FUnitInst& RU = Units.Last();
		RU.Order.Type = EOrderType::AttackMove;
		RU.Order.Pos = HQPos + FVector2D(Rng.FRandRange(-2, 2), Rng.FRandRange(-2, 2));
		N++;
	}
	AddAlert(FString::Printf(TEXT("Raid wave %d: %d shackled units!"), RaidWave, N), FLinearColor(1, 0.4f, 0.3f), LastRaidFrom);
}

// ================================================================ projectiles / particles

void FDRXSim::TickProjectiles(float Dt)
{
	for (FProjectile& P : Projectiles)
	{
		// home onto moving unit targets
		if (P.TargetUnit >= 0)
			if (FUnitInst* T = FindUnit(P.TargetUnit))
				if (T->HP > 0 && !P.bArc) P.Target = T->Pos;

		FVector2D Dir = (P.Target - P.Pos);
		float Dist = Dir.Size();
		float Step = P.Speed * Dt;
		P.T += Dt;

		if (Dist <= Step + 0.1f)
		{
			// shield dome interception
			bool bAbsorbed = false;
			if (P.Faction == EFaction::Vex)
			{
				for (FBuildingInst& B : Buildings)
				{
					if (B.Type != EBuilding::ShieldDome || !B.bConstructed || B.HP <= 0) continue;
					if (FVector2D::Distance(BuildingCenter(B), P.Target) < 7.f && PowerSat > 0.4f)
					{
						DamageBuilding(B, P.Dmg * 0.5f);
						Burst(EParticle::Glow, P.Target, 6, FLinearColor(0.4f, 0.8f, 1.f), 2.f, 5.f);
						AddSound(TEXT("sfx_shield_deflect"), P.Target, 0.6f);
						bAbsorbed = true;
						break;
					}
				}
			}
			if (!bAbsorbed)
			{
				if (P.bAoE)
				{
					for (FUnitInst& U : Units)
						if (U.Faction != P.Faction && U.HP > 0 &&
						    FVector2D::Distance(U.Pos, P.Target) < P.AoERadius)
							DamageUnit(U, P.Dmg, P.Faction);
					for (FBuildingInst& B : Buildings)
						if (B.Faction != P.Faction && B.HP > 0 &&
						    FVector2D::Distance(BuildingCenter(B), P.Target) < P.AoERadius + 1.f)
							DamageBuilding(B, P.Dmg);
					Burst(EParticle::Explosion, P.Target, 10, FLinearColor(1.f, 0.6f, 0.2f), 3.f, 7.f);
					AddSound(TEXT("sfx_explosion_small"), P.Target, 0.7f);
				}
				else
				{
					if (P.TargetUnit >= 0)
					{
						if (FUnitInst* T = FindUnit(P.TargetUnit))
							if (T->HP > 0) DamageUnit(*T, P.Dmg, P.Faction);
					}
					else if (P.TargetBuilding >= 0)
					{
						if (FBuildingInst* B = FindBuilding(P.TargetBuilding))
							if (B->HP > 0) DamageBuilding(*B, P.Dmg);
					}
				}
			}
			P.Speed = -1; // remove
		}
		else
		{
			P.Pos += Dir / Dist * Step;
		}
	}
	Projectiles.RemoveAll([](const FProjectile& P) { return P.Speed < 0; });
}

void FDRXSim::TickParticles(float Dt)
{
	for (FParticle& P : Particles)
	{
		P.Life -= Dt;
		P.Pos += P.Vel * Dt;
		P.Vel.Y += 3.f * Dt; // gravity
		P.Vel *= (1.f - Dt * 1.4f);
	}
	Particles.RemoveAll([](const FParticle& P) { return P.Life <= 0; });
}

// ================================================================ vision / upkeep / research

void FDRXSim::GrantVision(FVector2D P, float Radius)
{
	int32 R = FMath::CeilToInt(Radius);
	int32 CX = (int32)P.X, CY = (int32)P.Y;
	float R2 = Radius * Radius;
	for (int32 y = CY - R; y <= CY + R; y++)
	for (int32 x = CX - R; x <= CX + R; x++)
	{
		if (!InBounds(x, y)) continue;
		float Dx = x + 0.5f - P.X, Dy = y + 0.5f - P.Y;
		if (Dx * Dx + Dy * Dy > R2) continue;
		FTile& T = At(x, y);
		T.bExplored = true;
		T.VisStamp = VisionStamp;
	}
}

void FDRXSim::TickVision()
{
	VisionStamp++;
	for (const FUnitInst& U : Units)
	{
		if (U.Faction != EFaction::Player) continue;
		float R = 6.f;
		if (U.Type == EUnit::ReconQuad) R = 9.f;
		else if (U.Type == EUnit::CyPtera || U.Type == EUnit::WdPtera) R = 10.f;
		else if (U.Type == EUnit::CyBrachioRadar) R = 13.f;
		GrantVision(U.Pos, R);
	}
	for (const FBuildingInst& B : Buildings)
	{
		if (B.Faction != EFaction::Player || B.HP <= 0) continue;
		float R = 6.f;
		if (B.Type == EBuilding::Watchtower) R = 9.f;
		else if (B.Type == EBuilding::RadarStation) R = 14.f;
		GrantVision(BuildingCenter(const_cast<FBuildingInst&>(B)), R);
	}
}

void FDRXSim::TickUpkeep(float Dt)
{
	static float Acc = 0;
	float Total = 0;
	for (const FUnitInst& U : Units)
	{
		if (U.Faction != EFaction::Player || !IsDino(U.Type)) continue;
		float Up = (U.Type >= EUnit::WdRaptor && U.Type <= EUnit::WdThief) ? 2.f : Unit(U.Type).Upkeep;
		if (U.Type == EUnit::CySpinoSail && bDay) Up = 0;
		Total += Up;
	}
	Acc += Total * Dt / 60.f;
	int32 Eat = (int32)Acc;
	if (Eat > 0)
	{
		Acc -= Eat;
		if (Stock[(int32)EItem::Feed] >= Eat) Stock[(int32)EItem::Feed] -= Eat;
		else Stock[(int32)EItem::Feed] = 0;
	}
	bool bStarving = (Total > 0 && Stock[(int32)EItem::Feed] <= 0);
	static float StarveWarnT = 0;
	StarveWarnT -= Dt;
	for (FUnitInst& U : Units)
	{
		if (U.Faction != EFaction::Player || !IsDino(U.Type)) continue;
		if (bStarving)
		{
			U.StarveT += Dt;
			if (U.StarveT > 8.f) U.HP -= Dt; // slow decay
			if (U.HP <= 0) KillUnit(U, EFaction::Wild);
		}
		else U.StarveT = 0;
	}
	if (bStarving && StarveWarnT <= 0)
	{
		StarveWarnT = 20.f;
		AddAlert(TEXT("Your dinos are STARVING! Produce feed."), FLinearColor(1, 0.4f, 0.2f));
		AddSound(TEXT("sfx_dino_hurt"));
	}
}

void FDRXSim::TickResearch(float Dt)
{
	if (Researching == ETech::COUNT) return;
	const FTechDef& D = Tech(Researching);
	ResearchT += Dt;
	if (ResearchT >= D.Time)
	{
		TechDone[(int32)Researching] = true;
		AddAlert(FString::Printf(TEXT("Research complete: %s"), D.Name), FLinearColor(0.5f, 0.9f, 1.f));
		AddSound(TEXT("sfx_research_done"));
		Researching = ETech::COUNT;
		ResearchT = 0;
	}
}

