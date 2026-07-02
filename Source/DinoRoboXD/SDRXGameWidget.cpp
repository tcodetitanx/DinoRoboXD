#include "SDRXGameWidget.h"
#include "DRXGameMode.h"
#include "DRXSim.h"
#include "DRXAssets.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "UnrealClient.h"

using namespace DRX;

// ================================================================ static data

static const TCHAR* ResNodeTex(EResNode R, int32 Variant)
{
	switch (R)
	{
	case EResNode::Tree:
	{
		static const TCHAR* Trees[4] = { TEXT("env_plant_00"), TEXT("env_plant_01"), TEXT("env_plant_02"), TEXT("env_plant_04") };
		return Trees[Variant & 3];
	}
	case EResNode::Berry:       return TEXT("env_plant_19");
	case EResNode::IronField:   return (Variant & 1) ? TEXT("env_ore_06") : TEXT("env_ore_13");
	case EResNode::CopperField: return (Variant & 1) ? TEXT("env_ore_04") : TEXT("env_ore_09");
	case EResNode::CoalField:   return (Variant & 1) ? TEXT("env_ore_05") : TEXT("env_ore_12");
	case EResNode::StoneField:  return (Variant & 1) ? TEXT("env_rock_10") : TEXT("env_rock_13");
	case EResNode::AmberGrove:  return (Variant & 1) ? TEXT("env_ore_08") : TEXT("env_ore_22");
	case EResNode::GeoVent:     return (Variant & 1) ? TEXT("env_ore_21") : TEXT("env_ore_25");
	case EResNode::ChronoRift:  return (Variant & 1) ? TEXT("env_ore_03") : TEXT("env_ore_15");
	case EResNode::EggNest:     return TEXT("env_ground_30");
	default: return nullptr;
	}
}

// top-bar resource rows (shared by paint + hover hit-test)
static const EItem GTopRow1[] = { EItem::Wood, EItem::Stone, EItem::IronOre, EItem::CopperOre, EItem::Coal,
	EItem::Amber, EItem::ChronoShard, EItem::Meat, EItem::Berry, EItem::Egg, EItem::Feed };
static const EItem GTopRow2[] = { EItem::IronIngot, EItem::CopperIngot, EItem::Steel, EItem::Gear, EItem::Wire,
	EItem::Circuit, EItem::AdvCircuit, EItem::Resin, EItem::CyberModule, EItem::Plate, EItem::ChronoBar };

static const TCHAR* ResNodeName(EResNode R)
{
	switch (R)
	{
	case EResNode::Tree:        return TEXT("Tree — harvest for Wood");
	case EResNode::Berry:       return TEXT("Berry Bush — harvest for Berries");
	case EResNode::IronField:   return TEXT("Iron Ore Field — place a miner on it");
	case EResNode::CopperField: return TEXT("Copper Ore Field — place a miner on it");
	case EResNode::CoalField:   return TEXT("Coal Field — place a miner on it");
	case EResNode::StoneField:  return TEXT("Stone Field — place a miner on it");
	case EResNode::AmberGrove:  return TEXT("Amber Grove — place a miner on it");
	case EResNode::GeoVent:     return TEXT("Geothermal Vent — build a Geothermal Plant here");
	case EResNode::ChronoRift:  return TEXT("Chrono Rift — build a Chrono Siphon here");
	case EResNode::EggNest:     return TEXT("Dino Nest — harvest for Eggs");
	default:                    return TEXT("");
	}
}

static const TCHAR* DecorTex(int32 V)
{
	if (V < 0) return nullptr;
	if (V >= 200)
	{
		static const TCHAR* Patches[8] = { TEXT("env_ground_32"), TEXT("env_ground_33"), TEXT("env_ground_34"),
			TEXT("env_ground_35"), TEXT("env_ground_36"), TEXT("env_ground_37"), TEXT("env_ground_38"), TEXT("env_ground_39") };
		return Patches[(V - 200) & 7];
	}
	if (V >= 100)
	{
		static const TCHAR* Rocks[6] = { TEXT("env_rock_05"), TEXT("env_rock_21"), TEXT("env_ground_24"),
			TEXT("env_ground_26"), TEXT("env_ground_27"), TEXT("env_rock_14") };
		return Rocks[(V - 100) % 6];
	}
	static const TCHAR* Plants[8] = { TEXT("env_plant_16"), TEXT("env_plant_12"), TEXT("env_plant_13"),
		TEXT("env_plant_09"), TEXT("env_plant_20"), TEXT("env_plant_11"), TEXT("env_ground_08"), TEXT("env_plant_18") };
	return Plants[V % 8];
}

struct FDexEntry { const TCHAR* Title; EUnit Wild; EUnit Cyber; const TCHAR* Blurb; };
static const FDexEntry GDex[] = {
	{TEXT("Raptor — 'the knife'"), EUnit::WdRaptor, EUnit::CyRaptor,
	 TEXT("Fast pack hunter. Cyber-forms scout and harass; the Gatling graft trades speed for lead.")},
	{TEXT("Triceratops — 'the walking wall'"), EUnit::WdTrice, EUnit::CyTriceHeavy,
	 TEXT("Frontline tank. War/Rhino forms hold and break lines; the Hauler graft carries 40 items.")},
	{TEXT("Ankylosaurus — 'the siege engine'"), EUnit::WdAnky, EUnit::CyAnkyCannon,
	 TEXT("Armored grazer. Cannon and Wrecker grafts crack camps; the Drill form mines on legs.")},
	{TEXT("Pteranodon — 'the sky'"), EUnit::WdPtera, EUnit::CyPteraWar,
	 TEXT("Flies over everything. War Pteras duel gunships; Rotor Pteras haul across rivers.")},
	{TEXT("Stegosaurus — 'the generator'"), EUnit::WdStego, EUnit::CyStegoCrystal,
	 TEXT("Plated brawler. The Crystal graft hums with charge and bolsters nearby industry.")},
	{TEXT("Brachiosaurus — 'the tower'"), EUnit::WdBrachio, EUnit::CyBrachioTank,
	 TEXT("A gentle colossus until you bolt a battery of guns to it. Radar form sees forever.")},
	{TEXT("T-Rex — 'the answer'"), EUnit::WdRex, EUnit::CyRex,
	 TEXT("Apex predator, apex weapon. Its roar breaks enemy nerve. The RED REX is one of one.")},
	{TEXT("Spinosaurus — 'the river king'"), EUnit::WdSpino, EUnit::CySpinoSaw,
	 TEXT("Territorial and amphibious. Sail forms feed themselves on sunlight.")},
	{TEXT("Gallimimus — 'the courier'"), EUnit::WdGalli, EUnit::CyGalliPack,
	 TEXT("Fast, cheap, everywhere. Swarm raiders and nimble couriers.")},
	{TEXT("Parasaur — 'the horn'"), EUnit::WdPara, EUnit::CyParaJets,
	 TEXT("Herd sentinel. The Jet graft's warning horn hastens every ally that hears it.")},
};

struct FIntroSlide { const TCHAR* Tex; const TCHAR* Text; };
static const FIntroSlide GIntro[] = {
	{TEXT("ply_ply_front"),
	 TEXT("The rift spits you out into hot Cretaceous air.\nYour Chrono-Displacement Core is shattered. Your survival printer is not.\n\nYou are Dr. Aris Kade. You followed your twin through time.")},
	{TEXT("bld_dk_cathedral"),
	 TEXT("Vex arrived five years before you.\nFive years of mines, foundries and shackle-pens.\nIn the east, his Chrono-Citadel is already charging —\nhis ride home, and a door slammed in your face.")},
	{TEXT("dino_cy_trex_red"),
	 TEXT("He didn't come alone. He conscripted the locals.\nLobotomized dinosaurs in black iron, red optics burning.\n\nYours will be better. Yours will be willing.")},
	{TEXT("rob_rb_hauler_quad"),
	 TEXT("Start small: chop, mine, smelt, print robots.\nTame the wild herds, graft them at the Conversion Rig,\nfeed them well, and teach them war.")},
	{TEXT("itm_shard_chrono"),
	 TEXT("Destroy the outposts. Siphon the rifts.\nTear down the Chrono-Citadel and take the Core.\n\nThe only way home is through him.")},
};

static const TCHAR* GHowTo =
	TEXT("DR. KADE      WASD walks your character directly (camera follows him)\n")
	TEXT("CAMERA        arrow keys / screen edge / middle-drag / minimap, wheel zoom\n")
	TEXT("SELECT        Left-click unit, drag a box, double-click = select type on screen\n")
	TEXT("ORDER         Right-click: move / attack / harvest (with a carrier selected)\n")
	TEXT("Q + click     Attack-move        X stop      H hold    C defensive    V aggressive\n")
	TEXT("1-9           Control groups (Ctrl+number to assign, double-tap to center)\n")
	TEXT("BUILD         Click a building in the left palette, place with left-click (Shift = repeat)\n")
	TEXT("T             Tech tree          N dinodex   Space pause   +/- game speed\n")
	TEXT("F5 / F8       Quick-save / quick-load (autosaves every 3 min; CONTINUE resumes)\n")
	TEXT("DEL           Demolish selected building    (hover a resource 2s for its name)\n\n")
	TEXT("ECONOMY       Miners/crafters dump into output buffers - hauler bots carry to storage.\n")
	TEXT("              Crafters pull inputs from your stockpile. Power brownouts slow everything.\n")
	TEXT("TAMING        Fight a wild dino below 25% HP, then walk Dr. Kade next to it (needs 5 feed).\n")
	TEXT("              Tamed dinos become war-forms at the Conversion Rig. Dinos eat feed - always.\n")
	TEXT("WIN           Destroy the Chrono-Citadel in the far east. Lose your HQ and it's over.");

// ================================================================ paint context

struct FDRXPaint
{
	const SDRXGameWidget& W;
	const FGeometry& Geo;
	FSlateWindowElementList& Out;
	int32 Layer;

	FSlateFontInfo Font(int32 Size, bool bBold = true) const
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	void Box(FVector2D Pos, FVector2D Size, FLinearColor C) const
	{
		FSlateDrawElement::MakeBox(Out, Layer,
			Geo.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Pos))),
			FDRXAssets::Get().White(), ESlateDrawEffect::None, C);
	}

	void Img(const FSlateBrush* B, FVector2D Pos, FVector2D Size, FLinearColor C,
	         bool bFlipX = false, FVector2D Squash = FVector2D(1, 1)) const
	{
		FScale2D Scale(bFlipX ? -Squash.X : Squash.X, Squash.Y);
		FSlateDrawElement::MakeBox(Out, Layer,
			Geo.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Pos)),
				FSlateRenderTransform(Scale), FVector2f(0.5f, 1.f)),
			B, ESlateDrawEffect::None, C);
	}

	void Txt(const FString& S, FVector2D Pos, int32 Size, FLinearColor C, bool bBold = true) const
	{
		FSlateDrawElement::MakeText(Out, Layer,
			Geo.ToPaintGeometry(FVector2f(2000, 100), FSlateLayoutTransform(FVector2f(Pos))),
			S, Font(Size, bBold), ESlateDrawEffect::None, C);
	}

	FVector2D Measure(const FString& S, int32 Size, bool bBold = true) const
	{
		return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font(Size, bBold));
	}

	void TxtC(const FString& S, FVector2D Center, int32 Size, FLinearColor C, bool bBold = true) const
	{
		FVector2D M = Measure(S, Size, bBold);
		Txt(S, Center - M * 0.5f, Size, C, bBold);
	}

	void Frame(FVector2D Pos, FVector2D Size, FLinearColor C, float T = 2.f) const
	{
		Box(Pos, FVector2D(Size.X, T), C);
		Box(Pos + FVector2D(0, Size.Y - T), FVector2D(Size.X, T), C);
		Box(Pos, FVector2D(T, Size.Y), C);
		Box(Pos + FVector2D(Size.X - T, 0), FVector2D(T, Size.Y), C);
	}

	void Bar(FVector2D Pos, FVector2D Size, float Frac, FLinearColor Fill, FLinearColor Back) const
	{
		Box(Pos, Size, Back);
		Box(Pos, FVector2D(Size.X * FMath::Clamp(Frac, 0.f, 1.f), Size.Y), Fill);
	}

	// world-space sprite: anchored bottom-center at tile pos WP, height H in tiles
	void WSprite(const TCHAR* Tex, FVector2D WP, float HTiles, FLinearColor C,
	             bool bFlipX = false, FVector2D Squash = FVector2D(1, 1)) const
	{
		const FSlateBrush* B = FDRXAssets::Get().Brush(Tex);
		float Aspect = (B->ImageSize.Y > 0) ? B->ImageSize.X / B->ImageSize.Y : 1.f;
		float Zoom = (float)((W.WorldToScreen(FVector2D(1, 0)) - W.WorldToScreen(FVector2D(0, 0))).X);
		FVector2D SizePx(HTiles * Aspect * Zoom, HTiles * Zoom);
		FVector2D SP = W.WorldToScreen(WP);
		Img(B, FVector2D(SP.X - SizePx.X * 0.5f, SP.Y - SizePx.Y), SizePx, C, bFlipX, Squash);
	}
};

// ================================================================ construct / tick

void SDRXGameWidget::Construct(const FArguments& InArgs, ADRXGameMode* InMode)
{
	Mode = InMode;
	SetCanTick(true);
	bAutoTest = FParse::Param(FCommandLine::Get(), TEXT("drxautotest"));
}

void SDRXGameWidget::TickAutoTest(float Dt)
{
	AutoT += Dt;
	auto Shot = [](const TCHAR* Name)
	{
		FScreenshotRequest::RequestScreenshot(FString(Name), true, false);
	};
	auto RevealAll = [this]()
	{
		if (FDRXSim* S = Sim())
			for (FTile& T : S->Tiles) { T.bExplored = true; T.VisStamp = S->VisionStamp; }
	};
	FDRXSim* S = Sim();
	switch (AutoStage)
	{
	case 0: if (AutoT > 2.f) { Shot(TEXT("drx_menu")); AutoStage++; } break;
	case 1: if (AutoT > 3.f) { if (Mode) { Mode->StartNewGame(); UI = EUIMode::Playing; Cam = Sim()->SpawnPos; } AutoStage++; } break;
	case 2: if (AutoT > 5.f) { Shot(TEXT("drx_base")); AutoStage++; } break;
	case 3: if (AutoT > 6.f && S)
	{
		// cheat up an economy: resources + all tech, then build a working chain
		for (int32 i = 0; i < ITEM_COUNT; i++) S->Stock[i] += 300;
		for (int32 t = 0; t < TECH_COUNT; t++) S->TechDone[t] = true;
		// find an iron tile near spawn and drop a drill on it
		FIntPoint Ore(-1, -1);
		float Best = 1e9f;
		for (int32 y = 0; y < FDRXSim::MapH; y++)
		for (int32 x = 0; x < FDRXSim::MapW; x++)
			if (S->At(x, y).Res == EResNode::IronField && S->At(x, y).bExplored)
			{
				float Dd = FVector2D::DistSquared(FVector2D(x, y), S->SpawnPos);
				if (Dd < Best) { Best = Dd; Ore = FIntPoint(x, y); }
			}
		if (Ore.X >= 0)
		{
			bool bPlaced = S->PlaceBuilding(EBuilding::Drill, Ore.X, Ore.Y);
			UE_LOG(LogTemp, Display, TEXT("DRXTEST drill at %d,%d placed=%d"), Ore.X, Ore.Y, bPlaced ? 1 : 0);
		}
		int32 BX = (int32)S->SpawnPos.X, BY = (int32)S->SpawnPos.Y;
		UE_LOG(LogTemp, Display, TEXT("DRXTEST kiln=%d burner=%d factory=%d turret=%d bench=%d"),
			S->PlaceBuilding(EBuilding::Kiln, BX + 4, BY + 4) ? 1 : 0,
			S->PlaceBuilding(EBuilding::CoalBurner, BX + 7, BY + 4) ? 1 : 0,
			S->PlaceBuilding(EBuilding::BotFactory, BX + 4, BY + 7) ? 1 : 0,
			S->PlaceBuilding(EBuilding::TurretCannon, BX + 8, BY + 7) ? 1 : 0,
			S->PlaceBuilding(EBuilding::Workbench, BX - 4, BY + 5) ? 1 : 0);
		// exercise WASD-style direct avatar control for a few seconds
		if (FUnitInst* Av = S->FindUnit(S->AvatarId))
			UE_LOG(LogTemp, Display, TEXT("DRXTEST avatar start %.1f,%.1f"), Av->Pos.X, Av->Pos.Y);
		S->AvatarMoveDir = FVector2D(1.f, 0.1f);   // walk right -> side sprite
		AutoStage = 30;
	} break;
	case 30: if (AutoT > 9.f && S)
	{
		Shot(TEXT("drx_walk_side"));               // capture mid-stride, facing right
		AutoStage = 31;
	} break;
	case 31: if (AutoT > 9.6f && S)
	{
		S->AvatarMoveDir = FVector2D(0.f, 1.f);    // walk down -> front sprite
		AutoStage = 4;
	} break;
	case 4: if (AutoT > 12.f && S)
	{
		S->AvatarMoveDir = FVector2D::ZeroVector;
		if (FUnitInst* Av = S->FindUnit(S->AvatarId))
			UE_LOG(LogTemp, Display, TEXT("DRXTEST avatar end %.1f,%.1f"), Av->Pos.X, Av->Pos.Y);
		Shot(TEXT("drx_avatar"));
		// queue robots at the finished factory + start research
		for (FBuildingInst& B : S->Buildings)
			if (B.Type == EBuilding::BotFactory && B.bConstructed)
			{
				UE_LOG(LogTemp, Display, TEXT("DRXTEST queue walker=%d spider=%d"),
					S->QueueUnit(B.Id, EUnit::GunWalker) ? 1 : 0,
					S->QueueUnit(B.Id, EUnit::DrillSpider) ? 1 : 0);
			}
		for (int32 t = 0; t < TECH_COUNT; t++) S->TechDone[t] = false;
		S->TechDone[(int32)ETech::FlintTools] = true;
		UE_LOG(LogTemp, Display, TEXT("DRXTEST research=%d"), S->StartResearch(ETech::BeastKen) ? 1 : 0);
		AutoStage++;
	} break;
	case 5: if (AutoT > 16.f) { Shot(TEXT("drx_economy")); AutoStage = 40; } break;
	case 40: if (AutoT > 16.4f && S && Mode)
	{
		bool bSaved = Mode->SaveGame();
		UE_LOG(LogTemp, Display, TEXT("DRXTEST save ok=%d wood=%d units=%d"),
			bSaved ? 1 : 0, S->Stock[(int32)EItem::Wood], S->Units.Num());
		S->Stock[(int32)EItem::Wood] += 9999; // mutate, then load must restore
		AutoStage = 41;
	} break;
	case 41: if (AutoT > 17.2f && Mode)
	{
		bool bLoaded = Mode->LoadGame();
		FDRXSim* L = Sim();
		UE_LOG(LogTemp, Display, TEXT("DRXTEST load ok=%d wood=%d units=%d"),
			bLoaded ? 1 : 0, L ? L->Stock[(int32)EItem::Wood] : -1, L ? L->Units.Num() : -1);
		AutoStage = 6;
	} break;
	case 6: if (AutoT > 17.5f && S) { S->RaidTimer = 21.f; AutoStage++; } break;
	case 7: if (AutoT > 24.f && S)
	{
		Shot(TEXT("drx_raidwarn"));
		// teleport the raid staging point next to the base so the fight is on camera
		S->LastRaidFrom = S->SpawnPos + FVector2D(14, 2);
		AutoStage++;
	} break;
	case 8: if (AutoT > 42.f) { Shot(TEXT("drx_raid")); AutoStage++; } break;
	case 9: if (AutoT > 47.f) { Shot(TEXT("drx_battle")); Cam = FVector2D(64, 84); RevealAll(); AutoStage++; } break;
	case 10: if (AutoT > 49.f) { RevealAll(); Shot(TEXT("drx_wilds")); AutoStage++; } break;
	case 11: if (AutoT > 48.f) { Cam = FVector2D(147, 80); RevealAll(); AutoStage++; } break;
	case 12: if (AutoT > 50.f) { RevealAll(); Shot(TEXT("drx_citadel")); AutoStage++; } break;
	case 13: if (AutoT > 51.f) { UI = EUIMode::Tech; AutoStage++; } break;
	case 14: if (AutoT > 53.f) { Shot(TEXT("drx_tech")); AutoStage++; } break;
	case 15: if (AutoT > 54.f) { UI = EUIMode::Dinodex; AutoStage++; } break;
	case 16: if (AutoT > 56.f) { Shot(TEXT("drx_dex")); AutoStage++; } break;
	case 17: if (AutoT > 58.f && S)
	{
		UE_LOG(LogTemp, Display, TEXT("DRXTEST end units=%d blds=%d kills=%d raidwave=%d ironore=%d ingots=%d"),
			S->Units.Num(), S->Buildings.Num(), S->KillCount, S->RaidWave,
			S->Stock[(int32)EItem::IronOre], S->Stock[(int32)EItem::IronIngot]);
		FVector2D SideSz = FDRXAssets::Get().BrushSize(TEXT("ply_ply_side"));
		FVector2D FrontSz = FDRXAssets::Get().BrushSize(TEXT("ply_ply_front"));
		UE_LOG(LogTemp, Display, TEXT("DRXTEST plytex side=%.0fx%.0f front=%.0fx%.0f"),
			SideSz.X, SideSz.Y, FrontSz.X, FrontSz.Y);
		if (Mode) Mode->GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));
		AutoStage++;
	} break;
	default: break;
	}
}

FDRXSim* SDRXGameWidget::Sim() const
{
	return Mode ? Mode->Sim() : nullptr;
}

void SDRXGameWidget::Tick(const FGeometry& G, const double InCurrentTime, const float Dt)
{
	ViewSize = G.GetLocalSize();
	UITime += Dt;
	if (bAutoTest) TickAutoTest(Dt);

	if (UI == EUIMode::Playing && Sim())
	{
		// WASD drives Dr. Kade directly; the camera trails him while he walks
		FVector2D AvDir(0, 0);
		if (HeldKeys.Contains(EKeys::W)) AvDir.Y -= 1;
		if (HeldKeys.Contains(EKeys::S)) AvDir.Y += 1;
		if (HeldKeys.Contains(EKeys::A)) AvDir.X -= 1;
		if (HeldKeys.Contains(EKeys::D)) AvDir.X += 1;
		FDRXSim* SimPtr = Sim();
		if (!bAutoTest) SimPtr->AvatarMoveDir = AvDir;   // autotest injects its own
		if (!AvDir.IsNearlyZero())
		{
			if (FUnitInst* Av = SimPtr->FindUnit(SimPtr->AvatarId))
				Cam += (Av->Pos - Cam) * FMath::Min(1.f, Dt * 4.f);
		}

		// arrow keys + edge + middle-drag pan the camera
		float PanSpd = 22.f * (44.f / Zoom) * Dt * (HeldKeys.Contains(EKeys::LeftShift) ? 2.2f : 1.f);
		FVector2D Pan(0, 0);
		if (HeldKeys.Contains(EKeys::Up)) Pan.Y -= 1;
		if (HeldKeys.Contains(EKeys::Down)) Pan.Y += 1;
		if (HeldKeys.Contains(EKeys::Left)) Pan.X -= 1;
		if (HeldKeys.Contains(EKeys::Right)) Pan.X += 1;
		if (!bPanning && !bAutoTest && ViewSize.X > 100 &&
		    MousePos.X >= 0 && MousePos.X <= ViewSize.X && MousePos.Y >= 0 && MousePos.Y <= ViewSize.Y)
		{
			if (MousePos.X < 6) Pan.X -= 1;
			if (MousePos.X > ViewSize.X - 6) Pan.X += 1;
			if (MousePos.Y < 6) Pan.Y -= 1;
			if (MousePos.Y > ViewSize.Y - 6) Pan.Y += 1;
		}
		Cam += Pan * PanSpd;
		Cam = Cam.ClampAxes(4, FDRXSim::MapW - 4);

		// prune dead selection
		FDRXSim* S = Sim();
		Selection.RemoveAll([S](int32 Id) { return const_cast<FDRXSim*>(S)->FindUnit(Id) == nullptr; });
		if (SelectedBuilding >= 0 && !S->FindBuilding(SelectedBuilding)) SelectedBuilding = -1;
		for (int32 g = 0; g < 10; g++)
		{
			Groups[g].RemoveAll([S](int32 Id) { return const_cast<FDRXSim*>(S)->FindUnit(Id) == nullptr; });
			GroupTapT[g] = FMath::Max(0.f, GroupTapT[g] - Dt);
		}

		MiniT -= Dt;
		if (MiniT <= 0) { MiniT = 0.4f; UpdateMinimap(); }

		UpdateHover(Dt);
		PumpSounds();
		OnVictoryDefeatCheck();
	}
	else if (Sim())
	{
		Sim()->AvatarMoveDir = FVector2D::ZeroVector;
	}
	UpdateMusic(Dt);
}

void SDRXGameWidget::UpdateHover(float Dt)
{
	FDRXSim* S = Sim();
	FString NewKey;
	if (S && !bDragging && BuildGhost == EBuilding::COUNT && MousePos.X >= 0)
	{
		// top-bar resource icons (icon + count, 84px cells, two 25px rows)
		if (MousePos.Y < 52 && MousePos.X > 6)
		{
			int32 Col = (int32)((MousePos.X - 8) / 84);
			int32 Row = (MousePos.Y < 27) ? 0 : 1;
			int32 N = Row == 0 ? UE_ARRAY_COUNT(GTopRow1) : UE_ARRAY_COUNT(GTopRow2);
			if (Col >= 0 && Col < N && FMath::Fmod(MousePos.X - 8.f, 84.f) < 62.f)
				NewKey = FString::Printf(TEXT("itm:%d:%d"), Row, Col);
		}
		// world resource nodes (outside HUD panels)
		else
		{
			FSlateRect Pal = PaletteRect(), Mini = MiniRect(), Sel = SelPanelRect();
			bool bOverUI =
				(MousePos.X > Pal.Left && MousePos.X < Pal.Right && MousePos.Y > Pal.Top && MousePos.Y < Pal.Bottom) ||
				(MousePos.X > Mini.Left && MousePos.X < Mini.Right && MousePos.Y > Mini.Top && MousePos.Y < Mini.Bottom) ||
				((Selection.Num() > 0 || SelectedBuilding >= 0) &&
				 MousePos.X > Sel.Left && MousePos.X < Sel.Right && MousePos.Y > Sel.Top && MousePos.Y < Sel.Bottom);
			if (!bOverUI)
			{
				FVector2D WP = ScreenToWorld(MousePos);
				int32 TX = (int32)WP.X, TY = (int32)WP.Y;
				if (S->InBounds(TX, TY) && S->At(TX, TY).bExplored && S->At(TX, TY).Res != EResNode::None)
					NewKey = FString::Printf(TEXT("res:%d:%d"), TX, TY);
			}
		}
	}
	if (NewKey == HoverKey) HoverT += Dt;
	else { HoverKey = NewKey; HoverT = 0; }
}

void SDRXGameWidget::PumpSounds()
{
	FDRXSim* S = Sim();
	if (!S || !Mode) return;
	for (const FSoundEvent& E : S->SoundQueue)
	{
		float Vol = E.Volume;
		if (E.Pos.X >= 0)
		{
			float Dist = FVector2D::Distance(E.Pos, Cam);
			float Att = FMath::Clamp(1.f - Dist / 34.f, 0.f, 1.f);
			if (Att <= 0.02f) continue;
			Vol *= Att;
		}
		FDRXAssets::Get().Play(Mode->GetWorld(), *E.Slug, Vol, 1.f + FMath::FRandRange(-0.05f, 0.05f));
	}
	S->SoundQueue.Reset();
}

void SDRXGameWidget::UpdateMusic(float Dt)
{
	if (!Mode) return;
	switch (UI)
	{
	case EUIMode::Menu: case EUIMode::HowTo: case EUIMode::Intro:
		Mode->PlayMusic(TEXT("mus_menu"), 0.45f); break;
	case EUIMode::Victory: Mode->PlayMusic(TEXT("mus_victory"), 0.55f); break;
	case EUIMode::Defeat:  Mode->PlayMusic(TEXT("mus_defeat"), 0.5f); break;
	default:
	{
		FDRXSim* S = Sim();
		if (!S) break;
		MusicPollT -= Dt;
		if (MusicPollT <= 0)
		{
			MusicPollT = 2.f;
			FVector2D HQPos = S->SpawnPos;
			bool bDanger = false;
			for (const FUnitInst& U : S->Units)
				if (U.Faction == EFaction::Vex && FVector2D::Distance(U.Pos, HQPos) < 26.f) { bDanger = true; break; }
			BattleHeat = bDanger ? 12.f : FMath::Max(0.f, BattleHeat - 2.f);
		}
		if (BattleHeat > 0) Mode->PlayMusic((Sim()->RaidWave % 2) ? TEXT("mus_battle2") : TEXT("mus_battle"), 0.5f);
		else Mode->PlayMusic((Sim()->RaidWave % 2) ? TEXT("mus_build2") : TEXT("mus_build"), 0.4f);
		break;
	}
	}
}

void SDRXGameWidget::OnVictoryDefeatCheck()
{
	FDRXSim* S = Sim();
	if (!S) return;
	if (S->bVictory && UI == EUIMode::Playing) UI = EUIMode::Victory;
	if (S->bDefeat && UI == EUIMode::Playing) UI = EUIMode::Defeat;
}

// ================================================================ layout rects

FSlateRect SDRXGameWidget::TopBarRect() const { return FSlateRect(0, 0, ViewSize.X, 54); }
FSlateRect SDRXGameWidget::PaletteRect() const { return FSlateRect(0, 62, 118, ViewSize.Y - 190); }
FSlateRect SDRXGameWidget::SelPanelRect() const
{ return FSlateRect(ViewSize.X * 0.5f - 330, ViewSize.Y - 180, ViewSize.X * 0.5f + 330, ViewSize.Y - 8); }
FSlateRect SDRXGameWidget::MiniRect() const
{ return FSlateRect(ViewSize.X - 212, ViewSize.Y - 212, ViewSize.X - 12, ViewSize.Y - 12); }

void SDRXGameWidget::PaletteItems(TArray<EBuilding>& Items) const
{
	FDRXSim* S = Sim();
	for (int32 b = 0; b < BUILDING_COUNT; b++)
	{
		const FBuildingDef& D = Bld((EBuilding)b);
		if (D.Era >= 5 || (EBuilding)b == EBuilding::HQ) continue;
		int32 Tab = FMath::Clamp(D.Era - 1, 0, 3);
		if (D.Era == 0) Tab = 0;
		if (Tab != PaletteTab) continue;
		Items.Add((EBuilding)b);
	}
}

bool SDRXGameWidget::HitPaletteItem(FVector2D P, EBuilding& OutB) const
{
	FSlateRect R = PaletteRect();
	if (P.X < R.Left || P.X > R.Right || P.Y < R.Top) return false;
	// tabs row
	if (P.Y < R.Top + 26)
	{
		int32 Tab = FMath::Clamp((int32)((P.X - R.Left) / 29), 0, 3);
		const_cast<SDRXGameWidget*>(this)->PaletteTab = Tab;
		OutB = EBuilding::COUNT;
		return true;
	}
	TArray<EBuilding> Items; PaletteItems(Items);
	int32 Col = (int32)((P.X - R.Left - 4) / 56);
	int32 Row = (int32)((P.Y - R.Top - 30) / 56);
	if (Col < 0 || Col > 1) return false;
	int32 Idx = Row * 2 + Col;
	if (Idx < 0 || Idx >= Items.Num()) return false;
	OutB = Items[Idx];
	return true;
}

// ================================================================ input

FReply SDRXGameWidget::OnMouseMove(const FGeometry& G, const FPointerEvent& E)
{
	FVector2D NewPos = G.AbsoluteToLocal(E.GetScreenSpacePosition());
	if (bPanning)
	{
		FVector2D Delta = NewPos - MousePos;
		Cam -= Delta / Zoom;
		Cam = Cam.ClampAxes(4, FDRXSim::MapW - 4);
	}
	MousePos = NewPos;
	return FReply::Handled();
}

FReply SDRXGameWidget::OnMouseWheel(const FGeometry& G, const FPointerEvent& E)
{
	if (UI == EUIMode::Tech) { TechScroll = FMath::Clamp(TechScroll - E.GetWheelDelta() * 40, 0.f, 600.f); return FReply::Handled(); }
	if (UI == EUIMode::Dinodex) { DexScroll = FMath::Clamp(DexScroll - E.GetWheelDelta() * 40, 0.f, 900.f); return FReply::Handled(); }
	if (UI != EUIMode::Playing) return FReply::Handled();
	FVector2D Before = ScreenToWorld(MousePos);
	Zoom = FMath::Clamp(Zoom * (E.GetWheelDelta() > 0 ? 1.15f : 0.87f), 14.f, 100.f);
	FVector2D After = ScreenToWorld(MousePos);
	Cam += Before - After;
	return FReply::Handled();
}

FReply SDRXGameWidget::OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E)
{
	MousePos = G.AbsoluteToLocal(E.GetScreenSpacePosition());
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));

	if (E.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bPanning = true;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	const bool bLeft = E.GetEffectingButton() == EKeys::LeftMouseButton;
	const bool bRight = E.GetEffectingButton() == EKeys::RightMouseButton;

	switch (UI)
	{
	case EUIMode::Menu:
	{
		if (!bLeft) return FReply::Handled();
		FVector2D C(ViewSize.X * 0.5f, ViewSize.Y * 0.50f);
		auto Hit = [&](float Y) {
			return FMath::Abs(MousePos.X - C.X) < 150 && MousePos.Y > Y && MousePos.Y < Y + 46; };
		if (Hit(C.Y))
		{
			// CONTINUE: resume the live session, else load the save file
			if (Sim() && !Sim()->bVictory && !Sim()->bDefeat)
			{
				UI = EUIMode::Playing;
				FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_confirm"));
			}
			else if (Mode && Mode->HasSaveFile() && Mode->LoadGame())
			{
				UI = EUIMode::Playing;
				if (FUnitInst* Av = Sim()->FindUnit(Sim()->AvatarId)) Cam = Av->Pos;
				else Cam = Sim()->SpawnPos;
				Sim()->AddAlert(TEXT("Game loaded."), FLinearColor(0.5f, 0.9f, 1.f));
				FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_teleport"));
			}
			else FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_error"), 0.6f);
		}
		else if (Hit(C.Y + 62)) { FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_confirm")); StartIntro(); }
		else if (Hit(C.Y + 124)) { FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_click")); UI = EUIMode::HowTo; }
		else if (Hit(C.Y + 186)) { if (Mode) Mode->GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit")); }
		return FReply::Handled();
	}
	case EUIMode::HowTo:
		UI = EUIMode::Menu;
		return FReply::Handled();
	case EUIMode::Intro:
	{
		if (++IntroSlide >= UE_ARRAY_COUNT(GIntro))
		{
			UI = EUIMode::Playing;
			if (Mode) { Mode->StartNewGame(); Cam = Sim()->SpawnPos; Zoom = 44.f; }
			FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_teleport"));
		}
		else FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_type"), 0.7f);
		return FReply::Handled();
	}
	case EUIMode::Victory:
	case EUIMode::Defeat:
		if (bLeft) { UI = EUIMode::Menu; Mode->StopMusic(); }
		return FReply::Handled();
	case EUIMode::Tech:
	{
		if (!bLeft) { UI = EUIMode::Playing; return FReply::Handled(); }
		FDRXSim* S = Sim();
		if (!S) return FReply::Handled();
		for (int32 t = 0; t < TECH_COUNT; t++)
		{
			const FTechDef& D = Tech((ETech)t);
			int32 Col = FMath::Clamp(D.Era - 1, 0, 3);
			int32 RowIdx = 0;
			for (int32 u = 0; u < t; u++) if (FMath::Clamp(Tech((ETech)u).Era - 1, 0, 3) == Col) RowIdx++;
			FVector2D P(60 + Col * (ViewSize.X - 120) / 4.f, 120 + RowIdx * 86 - TechScroll);
			if (MousePos.X > P.X && MousePos.X < P.X + (ViewSize.X - 140) / 4.f &&
			    MousePos.Y > P.Y && MousePos.Y < P.Y + 78)
			{
				if (S->StartResearch((ETech)t))
					FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_confirm"));
				else FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_error"), 0.6f);
				return FReply::Handled();
			}
		}
		return FReply::Handled();
	}
	case EUIMode::Dinodex:
		if (bRight) UI = EUIMode::Playing;
		return FReply::Handled();
	case EUIMode::Playing:
		return HandlePlayingClick(E, bLeft);
	}
	return FReply::Handled();
}

FReply SDRXGameWidget::HandlePlayingClick(const FPointerEvent& E, bool bLeft)
{
	FDRXSim* S = Sim();
	if (!S) return FReply::Handled();
	const bool bShift = E.IsShiftDown();

	// ---- minimap
	{
		FSlateRect R = MiniRect();
		if (MousePos.X > R.Left && MousePos.X < R.Right && MousePos.Y > R.Top && MousePos.Y < R.Bottom)
		{
			FVector2D T((MousePos.X - R.Left) / (R.Right - R.Left) * FDRXSim::MapW,
			            (MousePos.Y - R.Top) / (R.Bottom - R.Top) * FDRXSim::MapH);
			if (bLeft) Cam = T.ClampAxes(4, FDRXSim::MapW - 4);
			else IssueContextOrder(T);
			return FReply::Handled();
		}
	}

	// ---- palette
	if (bLeft)
	{
		EBuilding PB;
		if (HitPaletteItem(MousePos, PB))
		{
			if (PB != EBuilding::COUNT)
			{
				if (S->IsBuildingUnlocked(PB)) { BuildGhost = PB; FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_click")); }
				else FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_error"), 0.6f);
			}
			else FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_tab"), 0.7f);
			return FReply::Handled();
		}
	}

	// ---- selection panel actions
	{
		FSlateRect R = SelPanelRect();
		if (MousePos.X > R.Left && MousePos.X < R.Right && MousePos.Y > R.Top && MousePos.Y < R.Bottom)
		{
			if (!bLeft) return FReply::Handled();
			FVector2D P0(R.Left, R.Top);
			// stance buttons for units
			if (Selection.Num() > 0)
			{
				for (int32 i = 0; i < 4; i++)
				{
					FVector2D BP = P0 + FVector2D(560, 12 + i * 40);
					if (MousePos.X > BP.X && MousePos.X < BP.X + 88 && MousePos.Y > BP.Y && MousePos.Y < BP.Y + 34)
					{
						if (i < 3) S->SetStance(Selection, (EStance)i);
						else { FOrder O; S->IssueOrder(Selection, O, false); } // stop
						FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_click2"), 0.7f);
						return FReply::Handled();
					}
				}
			}
			else if (SelectedBuilding >= 0)
			{
				FBuildingInst* B = S->FindBuilding(SelectedBuilding);
				if (B)
				{
					const FBuildingDef& BD = Bld(B->Type);
					// demolish button
					FVector2D DP = P0 + FVector2D(R.Right - R.Left - 96, 12);
					if (MousePos.X > DP.X && MousePos.X < DP.X + 84 && MousePos.Y > DP.Y && MousePos.Y < DP.Y + 30)
					{ S->DemolishBuilding(B->Id); SelectedBuilding = -1; return FReply::Handled(); }

					// recipe buttons
					if (BD.Fn == EBuildFn::Smelter || BD.Fn == EBuildFn::Assembler)
					{
						TArrayView<const FRecipe> Rec = Recipes();
						int32 Shown = 0;
						for (int32 r = 0; r < Rec.Num(); r++)
						{
							if (!BuildingRunsRecipe(B->Type, Rec[r])) continue;
							FVector2D BP = P0 + FVector2D(150 + Shown * 92, 96);
							if (MousePos.X > BP.X && MousePos.X < BP.X + 86 && MousePos.Y > BP.Y && MousePos.Y < BP.Y + 62)
							{
								S->SetRecipe(B->Id, r);
								FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_confirm"), 0.7f);
								return FReply::Handled();
							}
							Shown++;
						}
					}
					// unit production buttons
					if (BD.Fn == EBuildFn::UnitFactory || BD.Fn == EBuildFn::ConversionRig || BD.Fn == EBuildFn::Hatchery)
					{
						TArray<EUnit> Prod;
						if (BD.Fn == EBuildFn::Hatchery)
						{
							Prod = { EUnit::WdRaptor, EUnit::WdGalli, EUnit::WdPara };
							if (B->Type == EBuilding::Incubator)
							{
								Prod.Append({ EUnit::WdTrice, EUnit::WdAnky, EUnit::WdStego, EUnit::WdPtera });
								if (S->TechDone[(int32)ETech::ApexGrafts])
									Prod.Append({ EUnit::WdBrachio, EUnit::WdRex, EUnit::WdSpino });
							}
						}
						else
							for (int32 u = 0; u < UNIT_COUNT; u++)
								if (Unit((EUnit)u).BuiltAt == B->Type && S->IsUnitUnlocked((EUnit)u))
									Prod.Add((EUnit)u);
						for (int32 i = 0; i < Prod.Num(); i++)
						{
							FVector2D BP = P0 + FVector2D(150 + (i % 9) * 56, 92 + (i / 9) * 56);
							if (MousePos.X > BP.X && MousePos.X < BP.X + 50 && MousePos.Y > BP.Y && MousePos.Y < BP.Y + 50)
							{
								if (!S->QueueUnit(B->Id, Prod[i]))
									FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_error"), 0.6f);
								return FReply::Handled();
							}
						}
					}
				}
			}
			return FReply::Handled();
		}
	}

	FVector2D WP = ScreenToWorld(MousePos);

	// ---- build ghost placement
	if (BuildGhost != EBuilding::COUNT)
	{
		if (bLeft)
		{
			const FBuildingDef& D = Bld(BuildGhost);
			int32 TX = FMath::FloorToInt(WP.X - D.W * 0.5f + 0.5f);
			int32 TY = FMath::FloorToInt(WP.Y - D.H * 0.5f + 0.5f);
			if (!S->PlaceBuilding(BuildGhost, TX, TY))
				FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_error"), 0.6f);
			else if (!bShift && Bld(BuildGhost).Fn != EBuildFn::Wall)
				BuildGhost = EBuilding::COUNT;
		}
		else BuildGhost = EBuilding::COUNT;
		return FReply::Handled();
	}

	// ---- attack-move armed
	if (bAttackMoveArmed && bLeft)
	{
		bAttackMoveArmed = false;
		FOrder O; O.Type = EOrderType::AttackMove; O.Pos = WP;
		S->IssueOrder(Selection, O, false);
		return FReply::Handled();
	}

	if (bLeft)
	{
		bDragging = true;
		DragStart = MousePos;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}
	if (!bLeft)
	{
		// right-click: rally if own production building selected and no units
		if (Selection.Num() == 0 && SelectedBuilding >= 0)
		{
			FBuildingInst* B = S->FindBuilding(SelectedBuilding);
			if (B && (Bld(B->Type).Fn == EBuildFn::UnitFactory || Bld(B->Type).Fn == EBuildFn::ConversionRig ||
			          Bld(B->Type).Fn == EBuildFn::Hatchery))
			{
				S->SetRally(B->Id, WP);
				S->AddText(TEXT("Rally set"), WP, FLinearColor(0.6f, 1.f, 0.8f));
				return FReply::Handled();
			}
		}
		IssueContextOrder(WP);
	}
	return FReply::Handled();
}

FReply SDRXGameWidget::OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E)
{
	MousePos = G.AbsoluteToLocal(E.GetScreenSpacePosition());
	if (E.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bPanning = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	if (E.GetEffectingButton() == EKeys::LeftMouseButton && bDragging)
	{
		bDragging = false;
		FinishDragSelect(E.IsShiftDown());
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Handled();
}

FReply SDRXGameWidget::OnMouseButtonDoubleClick(const FGeometry& G, const FPointerEvent& E)
{
	MousePos = G.AbsoluteToLocal(E.GetScreenSpacePosition());
	if (UI != EUIMode::Playing || E.GetEffectingButton() != EKeys::LeftMouseButton)
		return OnMouseButtonDown(G, E);
	FDRXSim* S = Sim();
	if (!S) return FReply::Handled();
	// select all units of same type on screen
	FVector2D WP = ScreenToWorld(MousePos);
	int32 Hit = S->UnitAt(WP, EFaction::Player);
	if (Hit >= 0)
	{
		EUnit Type = S->FindUnit(Hit)->Type;
		Selection.Reset();
		FVector2D Min = ScreenToWorld(FVector2D(0, 0)), Max = ScreenToWorld(ViewSize);
		for (FUnitInst& U : S->Units)
			if (U.Faction == EFaction::Player && U.Type == Type &&
			    U.Pos.X > Min.X && U.Pos.X < Max.X && U.Pos.Y > Min.Y && U.Pos.Y < Max.Y)
				Selection.Add(U.Id);
		SelectedBuilding = -1;
		FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_click2"), 0.7f);
	}
	return FReply::Handled();
}

void SDRXGameWidget::FinishDragSelect(bool bShift)
{
	FDRXSim* S = Sim();
	if (!S) return;
	float DragDist = FVector2D::Distance(DragStart, MousePos);
	if (!bShift) { Selection.Reset(); SelectedBuilding = -1; }

	if (DragDist < 6.f)
	{
		// click select: unit first, then building
		FVector2D WP = ScreenToWorld(MousePos);
		int32 Hit = S->UnitAt(WP, EFaction::Player);
		if (Hit >= 0) { Selection.AddUnique(Hit); FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_click"), 0.5f); return; }
		int32 BId = S->BuildingAt((int32)WP.X, (int32)WP.Y);
		if (BId >= 0)
		{
			FBuildingInst* B = S->FindBuilding(BId);
			if (B && B->Faction == EFaction::Player)
			{
				SelectedBuilding = BId;
				Selection.Reset();
				FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_click"), 0.5f);
			}
		}
		return;
	}
	FVector2D A = ScreenToWorld(FVector2D(FMath::Min(DragStart.X, MousePos.X), FMath::Min(DragStart.Y, MousePos.Y)));
	FVector2D B = ScreenToWorld(FVector2D(FMath::Max(DragStart.X, MousePos.X), FMath::Max(DragStart.Y, MousePos.Y)));
	for (FUnitInst& U : S->Units)
		if (U.Faction == EFaction::Player &&
		    U.Pos.X > A.X && U.Pos.X < B.X && U.Pos.Y > A.Y && U.Pos.Y < B.Y)
			Selection.AddUnique(U.Id);
	if (Selection.Num() > 0)
	{
		SelectedBuilding = -1;
		FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_click2"), 0.6f);
	}
}

void SDRXGameWidget::IssueContextOrder(FVector2D WP)
{
	FDRXSim* S = Sim();
	if (!S || Selection.Num() == 0) return;

	FOrder O;
	int32 EnemyU = S->UnitAt(WP, EFaction::Vex);
	if (EnemyU < 0)
	{
		int32 WildU = S->UnitAt(WP, EFaction::Wild);
		if (WildU >= 0) EnemyU = WildU;
	}
	int32 BId = S->BuildingAt((int32)WP.X, (int32)WP.Y);
	FBuildingInst* B = (BId >= 0) ? S->FindBuilding(BId) : nullptr;

	if (EnemyU >= 0)
	{
		O.Type = EOrderType::AttackTarget;
		O.TargetUnit = EnemyU;
	}
	else if (B && B->Faction == EFaction::Vex)
	{
		O.Type = EOrderType::AttackTarget;
		O.TargetBuilding = BId;
	}
	else
	{
		// harvest if a carrier is selected and there's a node
		FIntPoint T((int32)WP.X, (int32)WP.Y);
		bool bNode = S->InBounds(T.X, T.Y) && S->At(T.X, T.Y).Res != EResNode::None &&
		             S->At(T.X, T.Y).Res != EResNode::GeoVent;
		bool bCarrier = false;
		for (int32 Id : Selection)
			if (FUnitInst* U = S->FindUnit(Id))
				if (Unit(U->Type).Carry > 0) bCarrier = true;
		if (bNode && bCarrier)
		{
			O.Type = EOrderType::Harvest;
			O.HarvestTile = T;
		}
		else
		{
			O.Type = EOrderType::Move;
			O.Pos = WP;
		}
	}
	S->IssueOrder(Selection, O, false);
}

void SDRXGameWidget::SelectGroup(int32 Idx)
{
	if (Groups[Idx].Num() == 0) return;
	Selection = Groups[Idx];
	SelectedBuilding = -1;
	if (GroupTapT[Idx] > 0 && LastGroupTapped == Idx) CenterOnSelection();
	GroupTapT[Idx] = 0.4f;
	LastGroupTapped = Idx;
}

void SDRXGameWidget::CenterOnSelection()
{
	FDRXSim* S = Sim();
	if (!S || Selection.Num() == 0) return;
	FVector2D Avg(0, 0);
	int32 N = 0;
	for (int32 Id : Selection)
		if (FUnitInst* U = S->FindUnit(Id)) { Avg += U->Pos; N++; }
	if (N > 0) Cam = (Avg / N).ClampAxes(4, FDRXSim::MapW - 4);
}

FReply SDRXGameWidget::OnKeyUp(const FGeometry& G, const FKeyEvent& E)
{
	HeldKeys.Remove(E.GetKey());
	return FReply::Handled();
}

FReply SDRXGameWidget::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	const FKey K = E.GetKey();
	HeldKeys.Add(K);
	FDRXSim* S = Sim();

	if (UI == EUIMode::Menu)
	{
		if (K == EKeys::Escape && Mode)
			Mode->GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));
		return FReply::Handled();
	}
	if (UI == EUIMode::HowTo) { if (K == EKeys::Escape || K == EKeys::Enter) UI = EUIMode::Menu; return FReply::Handled(); }
	if (UI == EUIMode::Intro)
	{
		if (K == EKeys::Escape)
		{
			UI = EUIMode::Playing;
			if (Mode) { Mode->StartNewGame(); Cam = Sim() ? Sim()->SpawnPos : Cam; }
		}
		return FReply::Handled();
	}
	if (UI == EUIMode::Tech || UI == EUIMode::Dinodex)
	{
		if (K == EKeys::Escape || K == EKeys::T || K == EKeys::N) UI = EUIMode::Playing;
		return FReply::Handled();
	}
	if (UI == EUIMode::Victory || UI == EUIMode::Defeat)
	{
		if (K == EKeys::Escape || K == EKeys::Enter) { UI = EUIMode::Menu; Mode->StopMusic(); }
		return FReply::Handled();
	}
	if (UI != EUIMode::Playing || !S) return FReply::Handled();

	if (K == EKeys::Escape)
	{
		if (BuildGhost != EBuilding::COUNT) BuildGhost = EBuilding::COUNT;
		else if (bAttackMoveArmed) bAttackMoveArmed = false;
		else if (Selection.Num() > 0 || SelectedBuilding >= 0) { Selection.Reset(); SelectedBuilding = -1; }
		else { UI = EUIMode::Menu; } // soft-pause to menu; game state persists
		return FReply::Handled();
	}
	if (K == EKeys::T) { UI = EUIMode::Tech; return FReply::Handled(); }
	if (K == EKeys::N || K == EKeys::F1) { UI = EUIMode::Dinodex; return FReply::Handled(); }
	if (K == EKeys::F5 && Mode)
	{
		if (Mode->SaveGame())
		{
			S->AddAlert(TEXT("Game saved."), FLinearColor(0.5f, 0.9f, 1.f));
			FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_ui_confirm"), 0.7f);
		}
		return FReply::Handled();
	}
	if (K == EKeys::F8 && Mode && Mode->HasSaveFile())
	{
		if (Mode->LoadGame())
		{
			Selection.Reset(); SelectedBuilding = -1;
			for (int32 g = 0; g < 10; g++) Groups[g].Reset();
			if (FUnitInst* Av = Sim()->FindUnit(Sim()->AvatarId)) Cam = Av->Pos;
			Sim()->AddAlert(TEXT("Game loaded."), FLinearColor(0.5f, 0.9f, 1.f));
			FDRXAssets::Get().Play(Mode->GetWorld(), TEXT("sfx_teleport"), 0.7f);
		}
		return FReply::Handled();
	}
	if (K == EKeys::SpaceBar && Mode) { Mode->bSimPaused = !Mode->bSimPaused; return FReply::Handled(); }
	if ((K == EKeys::Equals || K == EKeys::Add) && Mode) { Mode->SimSpeed = 2.f; return FReply::Handled(); }
	if ((K == EKeys::Hyphen || K == EKeys::Subtract) && Mode) { Mode->SimSpeed = 1.f; return FReply::Handled(); }

	if (K == EKeys::Q) { bAttackMoveArmed = true; return FReply::Handled(); } // alt attack-move
	if (K == EKeys::X) { FOrder O; S->IssueOrder(Selection, O, false); return FReply::Handled(); }
	if (K == EKeys::H) { S->SetStance(Selection, EStance::Hold); return FReply::Handled(); }
	if (K == EKeys::C) { S->SetStance(Selection, EStance::Defensive); return FReply::Handled(); }
	if (K == EKeys::V) { S->SetStance(Selection, EStance::Aggressive); return FReply::Handled(); }
	if (K == EKeys::Delete && SelectedBuilding >= 0)
	{ S->DemolishBuilding(SelectedBuilding); SelectedBuilding = -1; return FReply::Handled(); }

	// (A belongs to avatar movement now — attack-move is Q only)

	// control groups
	for (int32 i = 0; i < 10; i++)
	{
		FKey NumKey = FKey(*FString::Printf(TEXT("%s"), *FString::FromInt((i + 1) % 10)));
		if (K == NumKey)
		{
			if (E.IsControlDown()) { Groups[i] = Selection; }
			else SelectGroup(i);
			return FReply::Handled();
		}
	}

	// debug cheat: F9 grants a resource bundle
	if (K == EKeys::F9)
	{
		for (int32 i = 0; i < ITEM_COUNT; i++) S->Stock[i] += 100;
		S->AddAlert(TEXT("DEBUG: +100 of everything"), FLinearColor(1, 0, 1));
		return FReply::Handled();
	}
	return FReply::Handled();
}

// ================================================================ minimap

void SDRXGameWidget::UpdateMinimap()
{
	FDRXSim* S = Sim();
	if (!S) return;
	const int32 W = FDRXSim::MapW, H = FDRXSim::MapH;
	if (!MiniTex)
	{
		MiniTex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
		MiniTex->AddToRoot();
		MiniTex->Filter = TF_Nearest;
		MiniPixels.SetNum(W * H);
		MiniBrush = MakeUnique<FSlateBrush>();
		MiniBrush->SetResourceObject(MiniTex);
		MiniBrush->ImageSize = FVector2D(W, H);
	}
	for (int32 y = 0; y < H; y++)
	for (int32 x = 0; x < W; x++)
	{
		const FTile& T = S->At(x, y);
		FLinearColor C(0.02f, 0.02f, 0.03f);
		if (T.bExplored)
		{
			C = FLinearColor(T.Ground) * 1.6f;
			switch (T.Res)
			{
			case EResNode::IronField: C = FLinearColor(0.65f, 0.68f, 0.75f); break;
			case EResNode::CopperField: C = FLinearColor(0.8f, 0.45f, 0.2f); break;
			case EResNode::CoalField: C = FLinearColor(0.1f, 0.1f, 0.1f); break;
			case EResNode::StoneField: C = FLinearColor(0.55f, 0.53f, 0.5f); break;
			case EResNode::AmberGrove: C = FLinearColor(0.9f, 0.7f, 0.2f); break;
			case EResNode::GeoVent: C = FLinearColor(0.9f, 0.4f, 0.1f); break;
			case EResNode::ChronoRift: C = FLinearColor(0.3f, 0.8f, 1.f); break;
			default: break;
			}
			if (T.BuildingId >= 0)
			{
				// look up faction cheaply: player buildings are the common case near base
				C = FLinearColor(0.4f, 0.4f, 0.45f);
			}
			if (T.VisStamp != S->VisionStamp) C *= 0.45f;
		}
		MiniPixels[y * W + x] = C.ToFColor(true);
	}
	// buildings & units as dots
	for (const FBuildingInst& B : S->Buildings)
	{
		if (B.HP <= 0) continue;
		bool bSeen = S->InBounds(B.X, B.Y) && S->At(B.X, B.Y).bExplored;
		if (!bSeen) continue;
		FColor C = (B.Faction == EFaction::Player) ? FColor(80, 255, 120) : FColor(255, 60, 50);
		const FBuildingDef& D = Bld(B.Type);
		for (int32 y = 0; y < D.H; y++)
		for (int32 x = 0; x < D.W; x++)
			if (S->InBounds(B.X + x, B.Y + y))
				MiniPixels[(B.Y + y) * W + B.X + x] = C;
	}
	for (const FUnitInst& U : S->Units)
	{
		int32 x = (int32)U.Pos.X, y = (int32)U.Pos.Y;
		if (!S->InBounds(x, y)) continue;
		if (U.Faction == EFaction::Player) MiniPixels[y * W + x] = FColor(160, 255, 180);
		else if (S->IsVisible(x, y))
			MiniPixels[y * W + x] = (U.Faction == EFaction::Vex) ? FColor(255, 80, 60) : FColor(230, 200, 80);
	}
	// raid warning ping
	if (S->RaidTimer < 20.f)
	{
		int32 x = (int32)S->LastRaidFrom.X, y = (int32)S->LastRaidFrom.Y;
		if (S->InBounds(x, y) && FMath::Fmod(UITime, 0.6f) < 0.3f)
			for (int32 dy = -2; dy <= 2; dy++)
			for (int32 dx = -2; dx <= 2; dx++)
				if (S->InBounds(x + dx, y + dy))
					MiniPixels[(y + dy) * W + x + dx] = FColor(255, 30, 30);
	}

	// upload
	void* Data = MiniTex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, MiniPixels.GetData(), MiniPixels.Num() * sizeof(FColor));
	MiniTex->GetPlatformData()->Mips[0].BulkData.Unlock();
	MiniTex->UpdateResource();
}

void SDRXGameWidget::StartIntro()
{
	IntroSlide = 0;
	UI = EUIMode::Intro;
}

// ================================================================ paint

int32 SDRXGameWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const_cast<SDRXGameWidget*>(this)->ViewSize = AllottedGeometry.GetLocalSize();
	FDRXPaint P{ *this, AllottedGeometry, OutDrawElements, LayerId };
	FDRXAssets& A = FDRXAssets::Get();

	// ------------------------------------------------ non-game screens
	if (UI == EUIMode::Menu || UI == EUIMode::HowTo)
	{
		P.Box(FVector2D(0, 0), ViewSize, FLinearColor(0.045f, 0.05f, 0.07f));
		// decorative sprites
		P.Layer++;
		float Bob = FMath::Sin(UITime * 1.2f) * 8.f;
		P.Img(A.Brush(TEXT("bld_dk_cathedral")), FVector2D(ViewSize.X - 420, ViewSize.Y - 500), FVector2D(400, 400), FLinearColor(0.55f, 0.45f, 0.5f));
		P.Img(A.Brush(TEXT("dino_cy_trex")), FVector2D(40, ViewSize.Y - 480 + Bob), FVector2D(420, 420), FLinearColor::White, true);
		P.Img(A.Brush(TEXT("dino_cy_raptor")), FVector2D(400, ViewSize.Y - 240 - Bob), FVector2D(180, 180), FLinearColor::White, true);
		P.Layer++;
		if (UI == EUIMode::Menu)
		{
			P.TxtC(TEXT("DINO ROBO XD"), FVector2D(ViewSize.X * 0.5f, ViewSize.Y * 0.24f), 64, FLinearColor(1.f, 0.85f, 0.35f));
			P.TxtC(TEXT("chrono-stranded  •  five years behind  •  one way home"),
				FVector2D(ViewSize.X * 0.5f, ViewSize.Y * 0.24f + 58), 15, FLinearColor(0.7f, 0.75f, 0.8f), false);
			FVector2D C(ViewSize.X * 0.5f, ViewSize.Y * 0.50f);
			auto Button = [&](float Y, const TCHAR* Label, FLinearColor LC, bool bEnabled = true)
			{
				bool bHover = bEnabled && FMath::Abs(MousePos.X - C.X) < 150 && MousePos.Y > Y && MousePos.Y < Y + 46;
				if (!bEnabled) LC *= 0.35f;
				P.Box(FVector2D(C.X - 150, Y), FVector2D(300, 46), bHover ? FLinearColor(0.25f, 0.3f, 0.2f) : FLinearColor(0.12f, 0.14f, 0.12f));
				P.Frame(FVector2D(C.X - 150, Y), FVector2D(300, 46), LC * (bHover ? 1.4f : 0.8f));
				P.TxtC(Label, FVector2D(C.X, Y + 23), 20, bHover ? FLinearColor::White : LC);
			};
			bool bCanContinue = (Sim() && !Sim()->bVictory && !Sim()->bDefeat) || (Mode && Mode->HasSaveFile());
			Button(C.Y, TEXT("CONTINUE"), FLinearColor(1.f, 0.9f, 0.5f), bCanContinue);
			Button(C.Y + 62, TEXT("NEW GAME"), FLinearColor(0.6f, 1.f, 0.5f));
			Button(C.Y + 124, TEXT("HOW TO PLAY"), FLinearColor(0.6f, 0.8f, 1.f));
			Button(C.Y + 186, TEXT("QUIT"), FLinearColor(1.f, 0.6f, 0.5f));
			P.TxtC(TEXT("a Factorio-like RTS  —  every sprite, sound and system wired by Claude"),
				FVector2D(ViewSize.X * 0.5f, ViewSize.Y - 24), 12, FLinearColor(0.4f, 0.45f, 0.5f), false);
		}
		else
		{
			P.TxtC(TEXT("FIELD MANUAL"), FVector2D(ViewSize.X * 0.5f, 70), 36, FLinearColor(1.f, 0.85f, 0.35f));
			P.Txt(GHowTo, FVector2D(ViewSize.X * 0.5f - 430, 130), 15, FLinearColor(0.85f, 0.9f, 0.95f), false);
			P.TxtC(TEXT("— click anywhere to return —"), FVector2D(ViewSize.X * 0.5f, ViewSize.Y - 40), 14, FLinearColor(0.5f, 0.55f, 0.6f), false);
		}
		return P.Layer;
	}

	if (UI == EUIMode::Intro)
	{
		P.Box(FVector2D(0, 0), ViewSize, FLinearColor(0.03f, 0.035f, 0.05f));
		P.Layer++;
		const FIntroSlide& Sl = GIntro[FMath::Clamp(IntroSlide, 0, (int32)UE_ARRAY_COUNT(GIntro) - 1)];
		float Pulse = 1.f + FMath::Sin(UITime * 2.f) * 0.02f;
		const FSlateBrush* B = A.Brush(Sl.Tex);
		float Asp = B->ImageSize.X / FMath::Max(1.f, B->ImageSize.Y);
		float Hh = 300 * Pulse;
		P.Img(B, FVector2D(ViewSize.X * 0.5f - Hh * Asp * 0.5f, ViewSize.Y * 0.42f - Hh), FVector2D(Hh * Asp, Hh), FLinearColor::White);
		P.Layer++;
		P.TxtC(FString::Printf(TEXT("—  %d / %d  —"), IntroSlide + 1, (int32)UE_ARRAY_COUNT(GIntro)),
			FVector2D(ViewSize.X * 0.5f, ViewSize.Y * 0.5f), 13, FLinearColor(0.5f, 0.55f, 0.6f), false);
		// multi-line centered text
		TArray<FString> Lines;
		FString(Sl.Text).ParseIntoArrayLines(Lines, false);
		for (int32 i = 0; i < Lines.Num(); i++)
			P.TxtC(Lines[i], FVector2D(ViewSize.X * 0.5f, ViewSize.Y * 0.56f + i * 26), 17, FLinearColor(0.9f, 0.92f, 0.95f), false);
		P.TxtC(TEXT("click to continue  •  Esc to skip"), FVector2D(ViewSize.X * 0.5f, ViewSize.Y - 36), 13, FLinearColor(0.45f, 0.5f, 0.55f), false);
		return P.Layer;
	}

	FDRXSim* S = Sim();
	if (!S)
	{
		P.Box(FVector2D(0, 0), ViewSize, FLinearColor::Black);
		return P.Layer;
	}

	// ------------------------------------------------ world
	const float TilePx = Zoom;
	FVector2D WMin = ScreenToWorld(FVector2D(0, 0)) - FVector2D(1, 3);
	FVector2D WMax = ScreenToWorld(ViewSize) + FVector2D(1, 1);
	int32 X0 = FMath::Clamp(FMath::FloorToInt(WMin.X), 0, FDRXSim::MapW - 1);
	int32 Y0 = FMath::Clamp(FMath::FloorToInt(WMin.Y), 0, FDRXSim::MapH - 1);
	int32 X1 = FMath::Clamp(FMath::CeilToInt(WMax.X), 0, FDRXSim::MapW - 1);
	int32 Y1 = FMath::Clamp(FMath::CeilToInt(WMax.Y), 0, FDRXSim::MapH - 1);

	P.Box(FVector2D(0, 0), ViewSize, FLinearColor(0.015f, 0.015f, 0.02f));
	P.Layer++;

	// terrain: smooth pre-blended ground colors (no per-tile checkerboard)
	for (int32 y = Y0; y <= Y1; y++)
	for (int32 x = X0; x <= X1; x++)
	{
		const FTile& T = S->At(x, y);
		if (!T.bExplored) continue;
		FLinearColor C = FLinearColor(T.Ground);
		if (T.Terrain == ETerrain::Water)
			C *= 1.f + 0.10f * FMath::Sin(UITime * 1.4f + x * 0.7f + y * 1.1f);
		FVector2D SP = WorldToScreen(FVector2D(x, y));
		P.Box(SP, FVector2D(TilePx + 1, TilePx + 1), C);
	}
	P.Layer++;

	// flat painted ground patches sit directly on the earth, under everything
	for (int32 y = Y0; y <= Y1; y++)
	for (int32 x = X0; x <= X1; x++)
	{
		const FTile& T = S->At(x, y);
		if (!T.bExplored || T.DecorVariant < 200) continue;
		const TCHAR* Tex = DecorTex(T.DecorVariant);
		if (!Tex) continue;
		const FSlateBrush* B = FDRXAssets::Get().Brush(Tex);
		float Asp = (B->ImageSize.Y > 0) ? (float)(B->ImageSize.X / B->ImageSize.Y) : 2.f;
		float Wpx = TilePx * 1.5f;
		float Hpx = Wpx / FMath::Max(1.2f, Asp);
		FVector2D SP = WorldToScreen(FVector2D(x + 0.5f, y + 0.5f));
		P.Img(B, FVector2D(SP.X - Wpx * 0.5f, SP.Y - Hpx * 0.5f), FVector2D(Wpx, Hpx),
			FLinearColor(1, 1, 1, 0.85f), (x * 5 + y * 3) % 2 == 0);
	}
	P.Layer++;

	// world sprites, Y-sorted: nodes, decor, buildings, units
	struct FDraw { float Y; int32 Kind; int32 Idx; };  // 0=node/decor tile, 1=building, 2=unit
	TArray<FDraw> Draws;
	Draws.Reserve(1024);
	for (int32 y = Y0; y <= Y1; y++)
	for (int32 x = X0; x <= X1; x++)
	{
		const FTile& T = S->At(x, y);
		if (!T.bExplored) continue;
		if (T.Res != EResNode::None || (T.DecorVariant >= 0 && T.DecorVariant < 200))
			Draws.Add({ (float)y + 0.9f, 0, y * FDRXSim::MapW + x });
	}
	for (int32 i = 0; i < S->Buildings.Num(); i++)
	{
		const FBuildingInst& B = S->Buildings[i];
		if (B.HP <= 0) continue;
		if (B.X + Bld(B.Type).W < X0 || B.X > X1 || B.Y + Bld(B.Type).H < Y0 || B.Y > Y1) continue;
		if (!S->At(B.X, B.Y).bExplored) continue;
		Draws.Add({ (float)(B.Y + Bld(B.Type).H), 1, i });
	}
	for (int32 i = 0; i < S->Units.Num(); i++)
	{
		const FUnitInst& U = S->Units[i];
		if (U.Pos.X < X0 - 2 || U.Pos.X > X1 + 2 || U.Pos.Y < Y0 - 2 || U.Pos.Y > Y1 + 3) continue;
		if (U.Faction != EFaction::Player && !S->IsVisible((int32)U.Pos.X, (int32)U.Pos.Y)) continue;
		Draws.Add({ (float)U.Pos.Y + (Unit(U.Type).bFlies ? 40.f : 0.f), 2, i });
	}
	Draws.Sort([](const FDraw& L, const FDraw& R) { return L.Y < R.Y; });

	for (const FDraw& Dw : Draws)
	{
		if (Dw.Kind == 0)
		{
			int32 x = Dw.Idx % FDRXSim::MapW, y = Dw.Idx / FDRXSim::MapW;
			const FTile& T = S->At(x, y);
			FVector2D WP(x + 0.5f, y + 0.95f);
			if (T.Res != EResNode::None)
			{
				const TCHAR* Tex = ResNodeTex(T.Res, T.GroundVariant);
				if (Tex)
				{
					float Hh = (T.Res == EResNode::Tree) ? 1.8f :
					           (T.Res == EResNode::ChronoRift || T.Res == EResNode::GeoVent) ? 1.35f : 1.05f;
					FLinearColor C = FLinearColor::White;
					if (T.Res == EResNode::ChronoRift)
						C *= 1.f + 0.25f * FMath::Sin(UITime * 3.f + x);
					P.WSprite(Tex, WP, Hh, C, (x * 7 + y * 13) % 2 == 0);
				}
			}
			else if (T.DecorVariant >= 0)
			{
				const TCHAR* Tex = DecorTex(T.DecorVariant);
				if (Tex) P.WSprite(Tex, WP, 0.8f, FLinearColor(1, 1, 1, 0.95f), (x + y) % 2 == 0);
			}
			continue;
		}
		if (Dw.Kind == 1)
		{
			const FBuildingInst& B = S->Buildings[Dw.Idx];
			const FBuildingDef& D = Bld(B.Type);
			FVector2D WP(B.X + D.W * 0.5f, B.Y + D.H);
			float Hh = (float)D.H * 1.25f;
			FLinearColor C = FLinearColor::White;
			if (!B.bConstructed)
			{
				float Frac = B.Progress / FMath::Max(0.1f, D.BuildTime);
				C = FLinearColor(0.6f + 0.4f * Frac, 0.7f + 0.3f * Frac, 1.f, 0.55f + 0.45f * Frac);
				Hh *= 0.75f + 0.25f * Frac;
			}
			if (B.FlashT > 0) C = FLinearColor(2.2f, 2.2f, 2.2f, C.A);
			// working pulse
			FVector2D Squash(1, 1);
			if (B.bConstructed && (D.Fn == EBuildFn::Assembler || D.Fn == EBuildFn::Smelter) && B.Progress > 0)
				Squash.Y = 1.f + 0.03f * FMath::Sin(B.AnimPhase * 2.f);
			P.WSprite(*FString(D.Tex), WP, Hh, C, false, Squash);

			// selection + hp
			if (B.Id == SelectedBuilding)
			{
				FVector2D TL = WorldToScreen(FVector2D(B.X, B.Y));
				P.Frame(TL, FVector2D(D.W * TilePx, D.H * TilePx), FLinearColor(0.4f, 1.f, 0.6f), 2.f);
			}
			if (B.HP < D.HP || B.Id == SelectedBuilding)
			{
				FVector2D TL = WorldToScreen(FVector2D(B.X, B.Y - 0.25f));
				P.Bar(TL, FVector2D(D.W * TilePx, 5), B.HP / D.HP,
					B.Faction == EFaction::Player ? FLinearColor(0.3f, 1.f, 0.4f) : FLinearColor(1.f, 0.3f, 0.25f),
					FLinearColor(0.1f, 0.1f, 0.1f, 0.8f));
			}
			// production progress
			if (B.BuildQueue.Num() > 0)
			{
				const FUnitDef& UD = Unit(B.BuildQueue[0]);
				float Time = (D.Fn == EBuildFn::Hatchery) ? 20.f : UD.BuildTime;
				FVector2D TL = WorldToScreen(FVector2D(B.X, B.Y - 0.55f));
				P.Bar(TL, FVector2D(D.W * TilePx, 5), B.UnitProgress / Time,
					FLinearColor(0.4f, 0.7f, 1.f), FLinearColor(0.1f, 0.1f, 0.1f, 0.8f));
			}
			continue;
		}
		// unit
		const FUnitInst& U = S->Units[Dw.Idx];
		const FUnitDef& D = Unit(U.Type);
		bool bFly = D.bFlies;
		float FlyH = bFly ? 0.85f + 0.1f * FMath::Sin(U.AnimPhase * 0.7f) : 0.f;
		FVector2D WP = U.Pos + FVector2D(0, 0.45f);

		// shadow
		{
			FVector2D SP = WorldToScreen(WP);
			float Sw = D.Scale * TilePx * 0.7f;
			P.Box(FVector2D(SP.X - Sw * 0.5f, SP.Y - Sw * 0.12f), FVector2D(Sw, Sw * 0.22f), FLinearColor(0, 0, 0, 0.25f));
		}

		float Hh = D.Scale * 1.15f;
		FVector2D Squash(1, 1);
		bool bMoving = U.Order.Type == EOrderType::Move || U.Order.Type == EOrderType::AttackMove ||
		               (U.Order.Type != EOrderType::None && U.Path.Num() > 0);
		float Bob = FMath::Sin(U.AnimPhase * 2.f);
		Squash.Y = 1.f + Bob * 0.05f;
		Squash.X = 1.f - Bob * 0.04f;
		if (U.Type == EUnit::PlayerAvatar)
		{
			// Dr. Kade breathes instead of wobbling: slow chest rise when idle,
			// a gentle stride bounce while walking
			bool bWalking = !S->AvatarMoveDir.IsNearlyZero() || U.Order.Type != EOrderType::None;
			if (bWalking)
			{
				float Stride = FMath::Sin(U.AnimPhase);
				Squash.Y = 1.f + Stride * 0.025f;
				Squash.X = 1.f - Stride * 0.015f;
			}
			else
			{
				float Breath = FMath::Sin(UITime * 1.6f);
				Squash.Y = 1.f + Breath * 0.013f;
				Squash.X = 1.f - Breath * 0.007f;
			}
		}
		if (U.SquashT > 0) { Squash.Y *= 0.82f; Squash.X *= 1.14f; }

		FLinearColor C = FLinearColor::White;
		if (U.Faction == EFaction::Vex) C = FLinearColor(0.85f, 0.5f, 0.55f);
		if (U.StarveT > 5.f) C *= FLinearColor(0.7f, 0.8f, 1.f);
		if (U.FlashT > 0) C = FLinearColor(2.4f, 2.4f, 2.4f, 1.f);
		if (U.bWeakened)
		{
			C *= FLinearColor(0.8f, 1.f, 0.8f);
			// pulsing tame ring
			FVector2D SP = WorldToScreen(WP);
			float Rr = TilePx * (0.7f + 0.12f * FMath::Sin(UITime * 5.f));
			P.Frame(FVector2D(SP.X - Rr, SP.Y - Rr * 0.5f), FVector2D(Rr * 2, Rr), FLinearColor(0.4f, 1.f, 0.4f, 0.8f), 2.f);
		}

		// lunge offset toward facing
		FVector2D LungeOff(0, 0);
		if (U.LungeT > 0) LungeOff.X = U.FacingX * U.LungeT * 1.4f;

		// creature art faces LEFT: flip when facing right. Dr. Kade has three
		// directional sprites; his side art faces RIGHT.
		const TCHAR* Tex = D.Tex;
		bool bFlip = U.FacingX > 0;
		if (U.Type == EUnit::PlayerAvatar)
		{
			if (U.FacingAxis == 0) { Tex = TEXT("ply_ply_side"); bFlip = U.FacingX < 0; }
			else if (U.FacingAxis < 0) { Tex = TEXT("ply_ply_back"); bFlip = false; }
			else { Tex = TEXT("ply_ply_front"); bFlip = false; }
		}
		P.WSprite(Tex, WP + LungeOff * 0.3f - FVector2D(0, FlyH), Hh, C, bFlip, Squash);

		bool bSel = U.bSelected || Selection.Contains(U.Id);
		if (bSel)
		{
			FVector2D SP = WorldToScreen(WP);
			float Rr = D.Scale * TilePx * 0.62f;
			P.Frame(FVector2D(SP.X - Rr, SP.Y - Rr * 0.42f), FVector2D(Rr * 2, Rr * 0.84f),
				U.Faction == EFaction::Player ? FLinearColor(0.35f, 1.f, 0.5f, 0.9f) : FLinearColor(1.f, 0.4f, 0.3f, 0.9f), 2.f);
		}
		if (bSel || U.HP < D.HP)
		{
			FVector2D SP = WorldToScreen(WP - FVector2D(0, Hh + FlyH + 0.15f));
			float Bw = D.Scale * TilePx * 0.9f;
			P.Bar(FVector2D(SP.X - Bw * 0.5f, SP.Y), FVector2D(Bw, 4), U.HP / D.HP,
				U.Faction == EFaction::Player ? FLinearColor(0.3f, 1.f, 0.4f) :
				U.Faction == EFaction::Vex ? FLinearColor(1.f, 0.3f, 0.25f) : FLinearColor(0.95f, 0.8f, 0.3f),
				FLinearColor(0.1f, 0.1f, 0.1f, 0.8f));
			// veterancy chevrons
			for (int32 r = 0; r < U.Rank; r++)
				P.Box(FVector2D(SP.X - Bw * 0.5f + r * 7, SP.Y - 7), FVector2D(5, 5), FLinearColor(1.f, 0.85f, 0.2f));
		}
		// carry icon
		if (U.CarryCount > 0 && U.CarryItem != EItem::COUNT)
		{
			FVector2D SP = WorldToScreen(WP - FVector2D(-0.3f, Hh * 0.7f + FlyH));
			float Isz = TilePx * 0.4f;
			P.Img(A.Brush(Item(U.CarryItem).Tex), FVector2D(SP.X, SP.Y - Isz), FVector2D(Isz, Isz), FLinearColor::White);
		}
	}
	P.Layer++;

	// projectiles
	for (const FProjectile& Pr : S->Projectiles)
	{
		FVector2D Pos = Pr.Pos;
		if (Pr.bArc)
		{
			float Frac = FMath::Clamp(Pr.T / FMath::Max(0.01f, Pr.T0), 0.f, 1.f);
			Pos.Y -= FMath::Sin(Frac * PI) * 2.2f;
		}
		FVector2D SP = WorldToScreen(Pos);
		float Ps = TilePx * 0.14f;
		P.Box(FVector2D(SP.X - Ps * 0.5f, SP.Y - Ps * 0.5f), FVector2D(Ps, Ps), Pr.Color);
		P.Box(FVector2D(SP.X - Ps * 0.25f, SP.Y - Ps * 0.25f), FVector2D(Ps * 0.5f, Ps * 0.5f), FLinearColor::White);
	}

	// particles
	for (const FParticle& Pt : S->Particles)
	{
		float Frac = Pt.Life / Pt.MaxLife;
		FVector2D SP = WorldToScreen(Pt.Pos);
		float Ps = Pt.Size * (Zoom / 44.f) * (0.5f + 0.5f * Frac);
		FLinearColor C = Pt.Color; C.A = Frac;
		P.Box(FVector2D(SP.X - Ps * 0.5f, SP.Y - Ps * 0.5f), FVector2D(Ps, Ps), C);
	}

	// floating texts
	for (const FFloatText& T : S->FloatTexts)
	{
		FVector2D SP = WorldToScreen(T.Pos);
		FLinearColor C = T.Color; C.A = FMath::Clamp(T.Life, 0.f, 1.f);
		P.TxtC(T.Text, SP, 13, C);
	}
	P.Layer++;

	// fog dim for explored-but-not-visible
	for (int32 y = Y0; y <= Y1; y++)
	for (int32 x = X0; x <= X1; x++)
	{
		const FTile& T = S->At(x, y);
		if (!T.bExplored || T.VisStamp == S->VisionStamp) continue;
		FVector2D SP = WorldToScreen(FVector2D(x, y));
		P.Box(SP, FVector2D(TilePx, TilePx), FLinearColor(0, 0, 0.02f, 0.34f));
	}
	// night tint
	if (!S->bDay)
	{
		float NightFrac = FMath::Clamp((S->DayT - 0.62f) / 0.05f, 0.f, 1.f) *
		                  FMath::Clamp((1.f - S->DayT) / 0.05f, 0.f, 1.f);
		P.Box(FVector2D(0, 0), ViewSize, FLinearColor(0.02f, 0.03f, 0.1f, 0.35f * FMath::Clamp(NightFrac * 3, 0.f, 1.f)));
	}
	P.Layer++;

	// build ghost
	if (BuildGhost != EBuilding::COUNT)
	{
		const FBuildingDef& D = Bld(BuildGhost);
		FVector2D WPm = ScreenToWorld(MousePos);
		int32 TX = FMath::FloorToInt(WPm.X - D.W * 0.5f + 0.5f);
		int32 TY = FMath::FloorToInt(WPm.Y - D.H * 0.5f + 0.5f);
		bool bOK = S->CanPlaceBuilding(BuildGhost, TX, TY) && S->CanAfford(D.Cost);
		FVector2D TL = WorldToScreen(FVector2D(TX, TY));
		P.Box(TL, FVector2D(D.W * TilePx, D.H * TilePx),
			bOK ? FLinearColor(0.2f, 1.f, 0.3f, 0.25f) : FLinearColor(1.f, 0.2f, 0.2f, 0.3f));
		P.WSprite(*FString(D.Tex), FVector2D(TX + D.W * 0.5f, TY + D.H), D.H * 1.25f,
			FLinearColor(1, 1, 1, 0.6f));
	}

	// drag select box
	if (bDragging)
	{
		FVector2D Min(FMath::Min(DragStart.X, MousePos.X), FMath::Min(DragStart.Y, MousePos.Y));
		FVector2D Sz(FMath::Abs(DragStart.X - MousePos.X), FMath::Abs(DragStart.Y - MousePos.Y));
		P.Box(Min, Sz, FLinearColor(0.3f, 1.f, 0.5f, 0.12f));
		P.Frame(Min, Sz, FLinearColor(0.3f, 1.f, 0.5f, 0.8f), 1.5f);
	}
	if (bAttackMoveArmed)
		P.TxtC(TEXT("ATTACK-MOVE: click a target location"), FVector2D(ViewSize.X * 0.5f, 90), 16, FLinearColor(1.f, 0.4f, 0.3f));

	P.Layer++;

	// ------------------------------------------------ HUD
	// top bar
	{
		P.Box(FVector2D(0, 0), FVector2D(ViewSize.X, 54), FLinearColor(0.05f, 0.06f, 0.07f, 0.92f));
		for (int32 i = 0; i < UE_ARRAY_COUNT(GTopRow1); i++)
		{
			FVector2D Pp(8 + i * 84, 4);
			P.Img(A.Brush(Item(GTopRow1[i]).Tex), Pp, FVector2D(20, 20), FLinearColor::White);
			P.Txt(FString::FromInt(S->Stock[(int32)GTopRow1[i]]), Pp + FVector2D(24, 3), 13, FLinearColor(0.9f, 0.95f, 1.f));
		}
		for (int32 i = 0; i < UE_ARRAY_COUNT(GTopRow2); i++)
		{
			FVector2D Pp(8 + i * 84, 29);
			P.Img(A.Brush(Item(GTopRow2[i]).Tex), Pp, FVector2D(20, 20), FLinearColor::White);
			P.Txt(FString::FromInt(S->Stock[(int32)GTopRow2[i]]), Pp + FVector2D(24, 3), 13, FLinearColor(0.75f, 0.8f, 0.9f));
		}
		// power / bandwidth / clock
		float Px = 8 + 11 * 84 + 10;
		FLinearColor PowC = S->PowerSat >= 0.99f ? FLinearColor(0.5f, 1.f, 0.5f) : FLinearColor(1.f, 0.5f, 0.3f);
		P.Txt(FString::Printf(TEXT("PWR %.0f/%.0f"), S->PowerGen, S->PowerUse), FVector2D(Px, 5), 13, PowC);
		if (S->BatteryCap > 0)
			P.Txt(FString::Printf(TEXT("BAT %.0f%%"), 100.f * S->BatteryCharge / S->BatteryCap), FVector2D(Px + 110, 5), 13, FLinearColor(0.6f, 0.8f, 1.f));
		int32 UBW = S->UsedBandwidth(), MBW = S->MaxBandwidth();
		P.Txt(FString::Printf(TEXT("BW %d/%d"), UBW, MBW), FVector2D(Px, 30), 13,
			UBW >= MBW ? FLinearColor(1.f, 0.4f, 0.3f) : FLinearColor(0.7f, 0.9f, 1.f));
		P.Txt(FString::Printf(TEXT("%s  day %d"), S->bDay ? TEXT("DAY") : TEXT("NIGHT"), 1 + (int32)(S->GameTime / 240)),
			FVector2D(Px + 110, 30), 13, S->bDay ? FLinearColor(1.f, 0.9f, 0.5f) : FLinearColor(0.5f, 0.6f, 1.f));
		// raid timer
		FString RaidStr = S->RaidTimer < 20.f
			? FString::Printf(TEXT("!! RAID IN %d !!"), (int32)S->RaidTimer)
			: FString::Printf(TEXT("next raid %d:%02d"), (int32)S->RaidTimer / 60, (int32)S->RaidTimer % 60);
		P.Txt(RaidStr, FVector2D(Px + 230, 5), 13,
			S->RaidTimer < 20.f ? FLinearColor(1.f, 0.25f, 0.2f) : FLinearColor(0.6f, 0.65f, 0.7f));
		P.Txt(FString::Printf(TEXT("T tech  N dinodex  Space %s  %sx"),
			Mode && Mode->bSimPaused ? TEXT("resume") : TEXT("pause"),
			Mode ? *FString::FromInt((int32)Mode->SimSpeed) : TEXT("1")),
			FVector2D(Px + 230, 30), 12, FLinearColor(0.5f, 0.55f, 0.6f), false);
		// research progress
		if (S->Researching != ETech::COUNT)
		{
			const FTechDef& TD = Tech(S->Researching);
			FVector2D Bp(ViewSize.X * 0.5f - 140, 58);
			P.Bar(Bp, FVector2D(280, 14), S->ResearchT / TD.Time, FLinearColor(0.3f, 0.7f, 1.f), FLinearColor(0.05f, 0.08f, 0.1f, 0.9f));
			P.TxtC(FString::Printf(TEXT("Researching: %s"), TD.Name), Bp + FVector2D(140, 7), 11, FLinearColor::White);
		}
	}

	// alerts
	for (int32 i = 0; i < S->Alerts.Num(); i++)
	{
		const FAlert& Al = S->Alerts[i];
		FLinearColor C = Al.Color; C.A = FMath::Clamp(Al.Life, 0.f, 1.f);
		FVector2D M = P.Measure(Al.Text, 14);
		P.Box(FVector2D(ViewSize.X - M.X - 34, 64 + i * 30), FVector2D(M.X + 18, 24), FLinearColor(0, 0, 0, 0.55f * C.A));
		P.Txt(Al.Text, FVector2D(ViewSize.X - M.X - 26, 67 + i * 30), 14, C);
	}

	// build palette
	{
		FSlateRect R = PaletteRect();
		P.Box(FVector2D(R.Left, R.Top), FVector2D(R.Right - R.Left, R.Bottom - R.Top), FLinearColor(0.05f, 0.06f, 0.07f, 0.9f));
		static const TCHAR* TabNames[4] = { TEXT("T1"), TEXT("T2"), TEXT("T3"), TEXT("T4") };
		for (int32 t = 0; t < 4; t++)
		{
			FVector2D Tp(R.Left + t * 29, R.Top);
			bool bAct = (t == PaletteTab);
			P.Box(Tp, FVector2D(28, 24), bAct ? FLinearColor(0.2f, 0.3f, 0.2f) : FLinearColor(0.08f, 0.1f, 0.1f));
			P.TxtC(TabNames[t], Tp + FVector2D(14, 12), 12, bAct ? FLinearColor(0.6f, 1.f, 0.6f) : FLinearColor(0.5f, 0.55f, 0.6f));
		}
		TArray<EBuilding> Items; PaletteItems(Items);
		for (int32 i = 0; i < Items.Num(); i++)
		{
			const FBuildingDef& D = Bld(Items[i]);
			FVector2D Ip(R.Left + 4 + (i % 2) * 56, R.Top + 30 + (i / 2) * 56);
			bool bUnlocked = S->IsBuildingUnlocked(Items[i]);
			bool bAfford = S->CanAfford(D.Cost);
			bool bHover = MousePos.X > Ip.X && MousePos.X < Ip.X + 52 && MousePos.Y > Ip.Y && MousePos.Y < Ip.Y + 52;
			P.Box(Ip, FVector2D(52, 52), bHover ? FLinearColor(0.16f, 0.2f, 0.16f) : FLinearColor(0.09f, 0.1f, 0.11f));
			FLinearColor Tint = !bUnlocked ? FLinearColor(0.25f, 0.25f, 0.3f) :
			                    !bAfford ? FLinearColor(0.5f, 0.4f, 0.4f) : FLinearColor::White;
			P.Img(A.Brush(D.Tex), Ip + FVector2D(4, 4), FVector2D(44, 44), Tint);
			if (BuildGhost == Items[i]) P.Frame(Ip, FVector2D(52, 52), FLinearColor(0.4f, 1.f, 0.5f), 2.f);
			if (bHover)
			{
				// tooltip
				FString Tip = D.Name;
				FString CostStr;
				for (int32 c = 0; D.Cost[c].Count > 0; c++)
					CostStr += FString::Printf(TEXT("%d %s  "), D.Cost[c].Count, Item(D.Cost[c].Item).Name);
				if (!bUnlocked && D.ReqTech >= 0)
					CostStr = FString::Printf(TEXT("Requires: %s"), Tech((ETech)D.ReqTech).Name);
				FVector2D Tp(R.Right + 8, Ip.Y);
				FVector2D M1 = P.Measure(Tip, 14), M2 = P.Measure(CostStr, 12, false);
				P.Box(Tp, FVector2D(FMath::Max(M1.X, M2.X) + 16, 44), FLinearColor(0, 0, 0, 0.85f));
				P.Txt(Tip, Tp + FVector2D(8, 4), 14, FLinearColor(1.f, 0.9f, 0.5f));
				P.Txt(CostStr, Tp + FVector2D(8, 24), 12, FLinearColor(0.8f, 0.85f, 0.9f), false);
			}
		}
	}

	// selection panel
	if (Selection.Num() > 0 || SelectedBuilding >= 0)
	{
		FSlateRect R = SelPanelRect();
		FVector2D P0(R.Left, R.Top);
		P.Box(P0, FVector2D(R.Right - R.Left, R.Bottom - R.Top), FLinearColor(0.05f, 0.06f, 0.07f, 0.92f));

		if (Selection.Num() > 0)
		{
			int32 Shown = 0;
			for (int32 Id : Selection)
			{
				FUnitInst* U = const_cast<FDRXSim*>(S)->FindUnit(Id);
				if (!U || Shown >= 24) break;
				const FUnitDef& D = Unit(U->Type);
				FVector2D Up = P0 + FVector2D(12 + (Shown % 12) * 44, 12 + (Shown / 12) * 66);
				P.Box(Up, FVector2D(40, 52), FLinearColor(0.09f, 0.11f, 0.1f));
				P.Img(A.Brush(D.Tex), Up + FVector2D(2, 2), FVector2D(36, 38), FLinearColor::White);
				P.Bar(Up + FVector2D(2, 44), FVector2D(36, 4), U->HP / D.HP, FLinearColor(0.3f, 1.f, 0.4f), FLinearColor(0.15f, 0.05f, 0.05f));
				for (int32 r = 0; r < U->Rank; r++)
					P.Box(Up + FVector2D(2 + r * 6, -4), FVector2D(4, 4), FLinearColor(1.f, 0.85f, 0.2f));
				Shown++;
			}
			// name + stats of first
			if (FUnitInst* U = const_cast<FDRXSim*>(S)->FindUnit(Selection[0]))
			{
				const FUnitDef& D = Unit(U->Type);
				P.Txt(FString::Printf(TEXT("%s  x%d"), D.Name, Selection.Num()), P0 + FVector2D(12, 142), 15, FLinearColor(1.f, 0.9f, 0.5f));
				P.Txt(FString::Printf(TEXT("HP %.0f/%.0f   DMG %.0f   RNG %.1f   SPD %.1f%s%s"),
					U->HP, D.HP, D.Dmg * (1.f + U->Rank * 0.15f), D.Range, D.Speed,
					D.Upkeep > 0 ? *FString::Printf(TEXT("   EATS %.0f/min"), D.Upkeep) : TEXT(""),
					U->StarveT > 5.f ? TEXT("   STARVING!") : TEXT("")),
					P0 + FVector2D(160, 143), 12, FLinearColor(0.75f, 0.8f, 0.9f), false);
			}
			// stance buttons
			static const TCHAR* StanceNames[4] = { TEXT("AGGRO (V)"), TEXT("DEFEND (C)"), TEXT("HOLD (H)"), TEXT("STOP (X)") };
			EStance CurStance = EStance::Defensive;
			if (FUnitInst* U0 = const_cast<FDRXSim*>(S)->FindUnit(Selection[0])) CurStance = U0->Stance;
			for (int32 i = 0; i < 4; i++)
			{
				FVector2D Bp = P0 + FVector2D(560, 12 + i * 40);
				bool bActive = (i < 3 && (int32)CurStance == i);
				P.Box(Bp, FVector2D(88, 34), bActive ? FLinearColor(0.2f, 0.32f, 0.2f) : FLinearColor(0.1f, 0.12f, 0.12f));
				P.TxtC(StanceNames[i], Bp + FVector2D(44, 17), 11,
					bActive ? FLinearColor(0.6f, 1.f, 0.6f) : FLinearColor(0.7f, 0.75f, 0.8f));
			}
		}
		else if (FBuildingInst* B = const_cast<FDRXSim*>(S)->FindBuilding(SelectedBuilding))
		{
			const FBuildingDef& BD = Bld(B->Type);
			P.Img(A.Brush(BD.Tex), P0 + FVector2D(12, 12), FVector2D(70, 70), FLinearColor::White);
			P.Txt(BD.Name, P0 + FVector2D(94, 14), 17, FLinearColor(1.f, 0.9f, 0.5f));
			P.Txt(FString::Printf(TEXT("HP %.0f/%d%s"), B->HP, BD.HP,
				!B->bConstructed ? TEXT("  (under construction)") : TEXT("")),
				P0 + FVector2D(94, 40), 12, FLinearColor(0.75f, 0.8f, 0.9f), false);
			if (BD.PowerUse > 0)
				P.Txt(FString::Printf(TEXT("Power %.0f  (grid %.0f%%)"), BD.PowerUse, S->PowerSat * 100),
					P0 + FVector2D(94, 58), 12, S->PowerSat < 1.f ? FLinearColor(1.f, 0.6f, 0.4f) : FLinearColor(0.6f, 0.9f, 0.6f), false);

			// demolish
			FVector2D DP = P0 + FVector2D(R.Right - R.Left - 96, 12);
			P.Box(DP, FVector2D(84, 30), FLinearColor(0.3f, 0.1f, 0.1f));
			P.TxtC(TEXT("DEMOLISH"), DP + FVector2D(42, 15), 11, FLinearColor(1.f, 0.6f, 0.5f));

			// recipes
			if (BD.Fn == EBuildFn::Smelter || BD.Fn == EBuildFn::Assembler)
			{
				TArrayView<const FRecipe> Rec = Recipes();
				int32 Shown = 0;
				P.Txt(TEXT("Recipes:"), P0 + FVector2D(150, 78), 12, FLinearColor(0.6f, 0.65f, 0.7f), false);
				for (int32 r = 0; r < Rec.Num(); r++)
				{
					if (!BuildingRunsRecipe(B->Type, Rec[r])) continue;
					FVector2D Bp = P0 + FVector2D(150 + Shown * 92, 96);
					bool bActive = (B->RecipeIdx == r);
					P.Box(Bp, FVector2D(86, 62), bActive ? FLinearColor(0.18f, 0.28f, 0.18f) : FLinearColor(0.09f, 0.11f, 0.11f));
					P.Img(A.Brush(Item(Rec[r].Out.Item).Tex), Bp + FVector2D(27, 4), FVector2D(32, 32), FLinearColor::White);
					P.TxtC(Rec[r].Name, Bp + FVector2D(43, 48), 10, bActive ? FLinearColor(0.6f, 1.f, 0.6f) : FLinearColor(0.75f, 0.8f, 0.85f), false);
					if (bActive && B->Progress > 0)
						P.Bar(Bp + FVector2D(4, 38), FVector2D(78, 4), B->Progress / Rec[r].Time, FLinearColor(0.4f, 0.8f, 1.f), FLinearColor(0.05f, 0.05f, 0.05f));
					Shown++;
				}
			}
			// production
			if (BD.Fn == EBuildFn::UnitFactory || BD.Fn == EBuildFn::ConversionRig || BD.Fn == EBuildFn::Hatchery)
			{
				TArray<EUnit> Prod;
				if (BD.Fn == EBuildFn::Hatchery)
				{
					Prod = { EUnit::WdRaptor, EUnit::WdGalli, EUnit::WdPara };
					if (B->Type == EBuilding::Incubator)
					{
						Prod.Append({ EUnit::WdTrice, EUnit::WdAnky, EUnit::WdStego, EUnit::WdPtera });
						if (S->TechDone[(int32)ETech::ApexGrafts])
							Prod.Append({ EUnit::WdBrachio, EUnit::WdRex, EUnit::WdSpino });
					}
					P.Txt(TEXT("Hatch (1 egg + 5 feed):"), P0 + FVector2D(150, 74), 12, FLinearColor(0.6f, 0.65f, 0.7f), false);
				}
				else if (BD.Fn == EBuildFn::ConversionRig)
					P.Txt(TEXT("Convert (needs tamed dino of that family within 10 tiles + parts):"), P0 + FVector2D(150, 74), 12, FLinearColor(0.6f, 0.65f, 0.7f), false);
				else
					P.Txt(TEXT("Build robots:"), P0 + FVector2D(150, 74), 12, FLinearColor(0.6f, 0.65f, 0.7f), false);
				if (BD.Fn != EBuildFn::Hatchery)
					for (int32 u = 0; u < UNIT_COUNT; u++)
						if (Unit((EUnit)u).BuiltAt == B->Type && S->IsUnitUnlocked((EUnit)u))
							Prod.Add((EUnit)u);
				for (int32 i = 0; i < Prod.Num(); i++)
				{
					const FUnitDef& UD = Unit(Prod[i]);
					FVector2D Bp = P0 + FVector2D(150 + (i % 9) * 56, 92 + (i / 9) * 56);
					bool bHover = MousePos.X > Bp.X && MousePos.X < Bp.X + 50 && MousePos.Y > Bp.Y && MousePos.Y < Bp.Y + 50;
					P.Box(Bp, FVector2D(50, 50), bHover ? FLinearColor(0.16f, 0.22f, 0.16f) : FLinearColor(0.09f, 0.11f, 0.11f));
					P.Img(A.Brush(UD.Tex), Bp + FVector2D(3, 3), FVector2D(44, 44),
						S->CanAfford(UD.Cost) ? FLinearColor::White : FLinearColor(0.45f, 0.4f, 0.4f));
					if (bHover)
					{
						FString CostStr;
						for (int32 c = 0; UD.Cost[c].Count > 0; c++)
							CostStr += FString::Printf(TEXT("%d %s  "), UD.Cost[c].Count, Item(UD.Cost[c].Item).Name);
						if (BD.Fn == EBuildFn::Hatchery) CostStr = TEXT("1 Egg + 5 Feed");
						FVector2D Tp(Bp.X, Bp.Y - 46);
						FVector2D M1 = P.Measure(UD.Name, 13);
						FVector2D M2 = P.Measure(CostStr + TEXT("  BW x"), 11, false);
						P.Box(Tp, FVector2D(FMath::Max(M1.X, M2.X) + 14, 42), FLinearColor(0, 0, 0, 0.9f));
						P.Txt(UD.Name, Tp + FVector2D(7, 3), 13, FLinearColor(1.f, 0.9f, 0.5f));
						P.Txt(FString::Printf(TEXT("%s  BW %d"), *CostStr, UD.BW), Tp + FVector2D(7, 22), 11, FLinearColor(0.8f, 0.85f, 0.9f), false);
					}
				}
				// queue
				for (int32 q = 0; q < B->BuildQueue.Num(); q++)
				{
					FVector2D Qp = P0 + FVector2D(R.Right - R.Left - 260 + q * 48, 130);
					P.Box(Qp, FVector2D(44, 44), FLinearColor(0.12f, 0.14f, 0.16f));
					P.Img(A.Brush(Unit(B->BuildQueue[q]).Tex), Qp + FVector2D(2, 2), FVector2D(40, 40), FLinearColor::White);
					if (q == 0)
					{
						float Time = (BD.Fn == EBuildFn::Hatchery) ? 20.f : Unit(B->BuildQueue[0]).BuildTime;
						P.Bar(Qp + FVector2D(2, 38), FVector2D(40, 4), B->UnitProgress / Time, FLinearColor(0.4f, 0.8f, 1.f), FLinearColor(0.05f, 0.05f, 0.05f));
					}
				}
			}
		}
	}

	// minimap
	{
		FSlateRect R = MiniRect();
		P.Box(FVector2D(R.Left - 3, R.Top - 3), FVector2D(R.Right - R.Left + 6, R.Bottom - R.Top + 6), FLinearColor(0.05f, 0.06f, 0.07f, 0.95f));
		if (MiniBrush.IsValid())
			P.Img(MiniBrush.Get(), FVector2D(R.Left, R.Top), FVector2D(R.Right - R.Left, R.Bottom - R.Top), FLinearColor::White);
		// camera box
		float Sx = (R.Right - R.Left) / FDRXSim::MapW;
		FVector2D HalfView = ViewSize / Zoom * 0.5f;
		FVector2D CamTL(R.Left + (Cam.X - HalfView.X) * Sx, R.Top + (Cam.Y - HalfView.Y) * Sx);
		P.Frame(CamTL, FVector2D(HalfView.X * 2 * Sx, HalfView.Y * 2 * Sx), FLinearColor(1, 1, 1, 0.6f), 1.f);
	}

	// paused indicator
	if (Mode && Mode->bSimPaused)
		P.TxtC(TEXT("| | PAUSED — Space to resume | |"), FVector2D(ViewSize.X * 0.5f, ViewSize.Y * 0.4f), 22, FLinearColor(1.f, 0.9f, 0.4f));

	// dwell tooltip (2s hover on a resource)
	if (HoverT >= 2.f && !HoverKey.IsEmpty())
	{
		FString Title, Sub;
		TArray<FString> Parts;
		HoverKey.ParseIntoArray(Parts, TEXT(":"));
		if (Parts.Num() == 3 && Parts[0] == TEXT("itm"))
		{
			int32 Row = FCString::Atoi(*Parts[1]), Col = FCString::Atoi(*Parts[2]);
			const EItem* Arr = Row == 0 ? GTopRow1 : GTopRow2;
			int32 N = Row == 0 ? UE_ARRAY_COUNT(GTopRow1) : UE_ARRAY_COUNT(GTopRow2);
			if (Col >= 0 && Col < N)
			{
				Title = Item(Arr[Col]).Name;
				Sub = FString::Printf(TEXT("%d in stock"), S->Stock[(int32)Arr[Col]]);
			}
		}
		else if (Parts.Num() == 3 && Parts[0] == TEXT("res"))
		{
			int32 TX = FCString::Atoi(*Parts[1]), TY = FCString::Atoi(*Parts[2]);
			if (S->InBounds(TX, TY) && S->At(TX, TY).Res != EResNode::None)
			{
				const FTile& T = S->At(TX, TY);
				Title = ResNodeName(T.Res);
				if (T.Res != EResNode::GeoVent && T.Res != EResNode::ChronoRift)
					Sub = FString::Printf(TEXT("%d left"), T.ResAmount);
			}
		}
		if (!Title.IsEmpty())
		{
			P.Layer++;
			FVector2D M1 = P.Measure(Title, 14);
			FVector2D M2 = Sub.IsEmpty() ? FVector2D::ZeroVector : P.Measure(Sub, 12, false);
			float Wd = FMath::Max(M1.X, M2.X) + 18;
			float Ht = Sub.IsEmpty() ? 28.f : 46.f;
			FVector2D Tp = MousePos + FVector2D(16, 18);
			Tp.X = FMath::Min(Tp.X, ViewSize.X - Wd - 4);
			Tp.Y = FMath::Min(Tp.Y, ViewSize.Y - Ht - 4);
			P.Box(Tp, FVector2D(Wd, Ht), FLinearColor(0, 0, 0, 0.88f));
			P.Frame(Tp, FVector2D(Wd, Ht), FLinearColor(0.9f, 0.8f, 0.4f, 0.7f), 1.f);
			P.Txt(Title, Tp + FVector2D(9, 4), 14, FLinearColor(1.f, 0.92f, 0.6f));
			if (!Sub.IsEmpty())
				P.Txt(Sub, Tp + FVector2D(9, 25), 12, FLinearColor(0.8f, 0.85f, 0.9f), false);
		}
	}

	// ------------------------------------------------ overlays
	if (UI == EUIMode::Tech)
	{
		P.Layer++;
		P.Box(FVector2D(0, 0), ViewSize, FLinearColor(0.02f, 0.025f, 0.04f, 0.94f));
		P.TxtC(TEXT("CHRONO-TECH ARCHIVE"), FVector2D(ViewSize.X * 0.5f, 40), 30, FLinearColor(1.f, 0.85f, 0.35f));
		P.TxtC(TEXT("click to research  •  T/Esc to close  •  wheel to scroll"), FVector2D(ViewSize.X * 0.5f, 74), 13, FLinearColor(0.55f, 0.6f, 0.65f), false);
		static const TCHAR* EraNames[4] = { TEXT("I  TRIBAL MACHINA"), TEXT("II  INDUSTRIAL"), TEXT("III  CYBERNETICS"), TEXT("IV  CHRONO") };
		float ColW = (ViewSize.X - 120) / 4.f;
		for (int32 c = 0; c < 4; c++)
			P.Txt(EraNames[c], FVector2D(60 + c * ColW, 96 - FMath::Min(TechScroll, 0.f)), 15, FLinearColor(0.5f, 0.8f, 1.f));
		for (int32 t = 0; t < TECH_COUNT; t++)
		{
			const FTechDef& D = Tech((ETech)t);
			int32 Col = FMath::Clamp(D.Era - 1, 0, 3);
			int32 RowIdx = 0;
			for (int32 u = 0; u < t; u++) if (FMath::Clamp(Tech((ETech)u).Era - 1, 0, 3) == Col) RowIdx++;
			FVector2D Bp(60 + Col * ColW, 120 + RowIdx * 86 - TechScroll);
			if (Bp.Y < 90 || Bp.Y > ViewSize.Y) continue;
			bool bDone = S->TechDone[t];
			bool bAvail = S->CanResearch((ETech)t);
			bool bCur = (S->Researching == (ETech)t);
			FLinearColor BoxC = bDone ? FLinearColor(0.08f, 0.2f, 0.1f) : bCur ? FLinearColor(0.1f, 0.15f, 0.25f) :
				bAvail ? FLinearColor(0.14f, 0.14f, 0.08f) : FLinearColor(0.07f, 0.08f, 0.09f);
			P.Box(Bp, FVector2D(ColW - 20, 78), BoxC);
			P.Frame(Bp, FVector2D(ColW - 20, 78),
				bDone ? FLinearColor(0.3f, 0.9f, 0.4f) : bCur ? FLinearColor(0.4f, 0.7f, 1.f) :
				bAvail ? FLinearColor(0.9f, 0.8f, 0.3f) : FLinearColor(0.2f, 0.22f, 0.25f), 1.5f);
			P.Img(A.Brush(D.BadgeTex), Bp + FVector2D(6, 6), FVector2D(30, 30), bDone || bAvail || bCur ? FLinearColor::White : FLinearColor(0.4f, 0.4f, 0.45f));
			P.Txt(D.Name, Bp + FVector2D(44, 6), 14,
				bDone ? FLinearColor(0.5f, 1.f, 0.6f) : bAvail || bCur ? FLinearColor(1.f, 0.95f, 0.7f) : FLinearColor(0.45f, 0.5f, 0.55f));
			P.Txt(D.Desc, Bp + FVector2D(8, 40), 10, FLinearColor(0.65f, 0.7f, 0.75f), false);
			FString CostStr;
			for (int32 cc = 0; D.Cost[cc].Count > 0; cc++)
				CostStr += FString::Printf(TEXT("%d %s  "), D.Cost[cc].Count, Item(D.Cost[cc].Item).Name);
			P.Txt(CostStr, Bp + FVector2D(8, 58), 10, FLinearColor(0.85f, 0.75f, 0.5f), false);
			if (bCur)
				P.Bar(Bp + FVector2D(8, 72), FVector2D(ColW - 36, 4), S->ResearchT / D.Time, FLinearColor(0.4f, 0.8f, 1.f), FLinearColor(0.05f, 0.05f, 0.05f));
		}
	}
	else if (UI == EUIMode::Dinodex)
	{
		P.Layer++;
		P.Box(FVector2D(0, 0), ViewSize, FLinearColor(0.02f, 0.03f, 0.03f, 0.94f));
		P.TxtC(TEXT("THE DINODEX"), FVector2D(ViewSize.X * 0.5f, 40), 30, FLinearColor(0.6f, 1.f, 0.6f));
		P.TxtC(TEXT("field notes of Dr. Aris Kade  •  N/Esc to close  •  wheel to scroll"), FVector2D(ViewSize.X * 0.5f, 74), 13, FLinearColor(0.55f, 0.6f, 0.65f), false);
		for (int32 i = 0; i < UE_ARRAY_COUNT(GDex); i++)
		{
			FVector2D Bp(ViewSize.X * 0.5f - 440, 100 + i * 104 - DexScroll);
			if (Bp.Y < 60 || Bp.Y > ViewSize.Y) continue;
			const FDexEntry& E = GDex[i];
			const FUnitDef& WD = Unit(E.Wild);
			const FUnitDef& CD = Unit(E.Cyber);
			P.Box(Bp, FVector2D(880, 96), FLinearColor(0.06f, 0.08f, 0.07f));
			P.Img(A.Brush(WD.Tex), Bp + FVector2D(8, 8), FVector2D(80, 80), FLinearColor::White);
			P.Img(A.Brush(CD.Tex), Bp + FVector2D(96, 8), FVector2D(80, 80), FLinearColor::White);
			P.Txt(E.Title, Bp + FVector2D(190, 8), 16, FLinearColor(1.f, 0.9f, 0.5f));
			P.Txt(E.Blurb, Bp + FVector2D(190, 34), 12, FLinearColor(0.8f, 0.85f, 0.9f), false);
			P.Txt(FString::Printf(TEXT("wild: HP %.0f DMG %.0f   |   %s: HP %.0f DMG %.0f RNG %.1f  eats %.0f/min"),
				WD.HP, WD.Dmg, CD.Name, CD.HP, CD.Dmg, CD.Range, CD.Upkeep),
				Bp + FVector2D(190, 66), 11, FLinearColor(0.55f, 0.7f, 0.6f), false);
		}
	}
	else if (UI == EUIMode::Victory || UI == EUIMode::Defeat)
	{
		P.Layer++;
		bool bWin = (UI == EUIMode::Victory);
		P.Box(FVector2D(0, 0), ViewSize, FLinearColor(0, 0, 0, 0.8f));
		P.TxtC(bWin ? TEXT("THE RIFT IS YOURS") : TEXT("FOREVER A FOSSIL"),
			FVector2D(ViewSize.X * 0.5f, ViewSize.Y * 0.3f), 48,
			bWin ? FLinearColor(0.5f, 1.f, 0.6f) : FLinearColor(1.f, 0.4f, 0.3f));
		P.TxtC(bWin
			? TEXT("The Chrono-Citadel falls. In the wreckage: a Displacement Core, humming and whole.\nYour dinos watch you calibrate it. You leave the printer running — for them.")
			: TEXT("The Field HQ burns, and the printer with it.\nVex's gunships circle once, then turn east. He never even landed."),
			FVector2D(ViewSize.X * 0.5f, ViewSize.Y * 0.42f), 16, FLinearColor(0.85f, 0.88f, 0.92f), false);
		P.TxtC(FString::Printf(TEXT("time %d:%02d   •   kills %d   •   losses %d   •   outposts cleared %d   •   raids survived %d"),
			(int32)S->GameTime / 60, (int32)S->GameTime % 60, S->KillCount, S->LossCount, S->CampsCleared, S->RaidWave),
			FVector2D(ViewSize.X * 0.5f, ViewSize.Y * 0.56f), 14, FLinearColor(0.6f, 0.65f, 0.7f), false);
		P.TxtC(TEXT("— click to return to the menu —"), FVector2D(ViewSize.X * 0.5f, ViewSize.Y * 0.68f), 14, FLinearColor(0.5f, 0.55f, 0.6f), false);
	}

	return P.Layer + 1;
}
