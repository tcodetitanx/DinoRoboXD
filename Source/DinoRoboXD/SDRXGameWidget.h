// Full-screen Slate widget: renders the entire game (world, HUD, menus) in
// OnPaint and consumes all input. No actors, no UMG.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "DRXTypes.h"

class ADRXGameMode;
class FDRXSim;
class UTexture2D;
struct FSlateBrush;

enum class EUIMode : uint8 { Menu, HowTo, Intro, Playing, Tech, Dinodex, Victory, Defeat };

class SDRXGameWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SDRXGameWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, ADRXGameMode* InMode);

	// SWidget
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1280, 720); }
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseWheel(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	virtual FReply OnKeyUp(const FGeometry& G, const FKeyEvent& E) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override
	{ return FCursorReply::Cursor(EMouseCursor::Default); }

private:
	ADRXGameMode* Mode = nullptr;
	FDRXSim* Sim() const;

	// ---- ui state
	EUIMode UI = EUIMode::Menu;
	int32 IntroSlide = 0;
	float UITime = 0;

	// camera (tile space center, zoom = px per tile)
	mutable FVector2D Cam = FVector2D(26, 80);
	float Zoom = 44.f;
	FVector2D ViewSize = FVector2D(1280, 720);
	FVector2D MousePos = FVector2D(-1, -1);        // widget local px; -1 until first move
	bool bPanning = false;
	FVector2D PanAnchor;
	TSet<FKey> HeldKeys;

	// selection / orders
	TArray<int32> Selection;                        // unit ids
	int32 SelectedBuilding = -1;
	bool bDragging = false;
	FVector2D DragStart;
	bool bAttackMoveArmed = false;
	TArray<int32> Groups[10];
	float GroupTapT[10] = {};
	int32 LastGroupTapped = -1;

	// build mode
	EBuilding BuildGhost = EBuilding::COUNT;
	int32 PaletteTab = 0;                           // era tab 0..3 (era = tab+1, tab0 includes era0)

	// panels
	float TechScroll = 0, DexScroll = 0;

	// minimap
	UTexture2D* MiniTex = nullptr;
	TUniquePtr<FSlateBrush> MiniBrush;
	TArray<FColor> MiniPixels;
	float MiniT = 0;

	float MusicPollT = 0;
	float BattleHeat = 0;

	// dwell tooltip (2s hover over a top-bar resource or a world node)
	FString HoverKey;
	float HoverT = 0;
	void UpdateHover(float Dt);

	// -drxautotest: scripted screenshot run for headless verification
	bool bAutoTest = false;
	float AutoT = 0;
	int32 AutoStage = 0;
	void TickAutoTest(float Dt);

	// ---- helpers
	FVector2D WorldToScreen(FVector2D W) const { return (W - Cam) * Zoom + ViewSize * 0.5f; }
	FVector2D ScreenToWorld(FVector2D S) const { return (S - ViewSize * 0.5f) / Zoom + Cam; }

	// layout rects
	FSlateRect TopBarRect() const;
	FSlateRect PaletteRect() const;
	FSlateRect SelPanelRect() const;
	FSlateRect MiniRect() const;
	void PaletteItems(TArray<EBuilding>& Out) const;
	bool HitPaletteItem(FVector2D P, EBuilding& Out) const;

	// input handling per mode
	FReply HandlePlayingClick(const FPointerEvent& E, bool bLeft);
	void FinishDragSelect(bool bShift);
	void IssueContextOrder(FVector2D WorldP);
	void SelectGroup(int32 Idx);
	void CenterOnSelection();
	void UpdateMinimap();
	void PumpSounds();
	void UpdateMusic(float Dt);
	void StartIntro();
	void OnVictoryDefeatCheck();

	// paint helpers (implemented in cpp with a context struct)
	friend struct FDRXPaint;
};
