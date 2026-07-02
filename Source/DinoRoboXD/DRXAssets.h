// Lazy texture/sound cache. Textures are AddToRoot'ed because FSlateBrushes
// referencing them are painted every frame (GC would crash OnPaint otherwise).
#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

class UTexture2D;
class USoundBase;
class UWorld;

class FDRXAssets
{
public:
	static FDRXAssets& Get();

	// Name = asset name under /Game/DRX/T (e.g. "dino_cy_raptor"). Never returns null.
	const FSlateBrush* Brush(const TCHAR* Name);
	FVector2D BrushSize(const TCHAR* Name);

	USoundBase* Sound(const TCHAR* Slug);   // /Game/DRX/S
	void Play(UWorld* World, const TCHAR* Slug, float Volume = 1.f, float Pitch = 1.f);

	const FSlateBrush* White();              // solid white box brush

private:
	TMap<FString, TUniquePtr<FSlateBrush>> Brushes;
	TMap<FString, USoundBase*> Sounds;
	FSlateBrush WhiteBrush;
	bool bWhiteInit = false;
};
