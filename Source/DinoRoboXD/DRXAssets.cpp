#include "DRXAssets.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"

FDRXAssets& FDRXAssets::Get()
{
	static FDRXAssets Inst;
	return Inst;
}

const FSlateBrush* FDRXAssets::Brush(const TCHAR* Name)
{
	FString Key(Name);
	if (TUniquePtr<FSlateBrush>* Found = Brushes.Find(Key))
	{
		// async texture compilation serves placeholder dimensions on first
		// access after a reimport — refresh so aspect ratios self-heal
		FSlateBrush* Br = Found->Get();
		if (UTexture2D* T = Cast<UTexture2D>(Br->GetResourceObject()))
			Br->ImageSize = FVector2D(T->GetSizeX(), T->GetSizeY());
		return Br;
	}

	TUniquePtr<FSlateBrush> NewBrush = MakeUnique<FSlateBrush>();
	FString Path = FString::Printf(TEXT("/Game/DRX/T/%s.%s"), Name, Name);
	UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Path);
	if (Tex)
	{
		Tex->AddToRoot(); // painted every frame; must never be GC'd
		NewBrush->SetResourceObject(Tex);
		NewBrush->ImageSize = FVector2D(Tex->GetSizeX(), Tex->GetSizeY());
		NewBrush->DrawAs = ESlateBrushDrawType::Image;
		NewBrush->Tiling = ESlateBrushTileType::NoTile;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DRX missing texture %s"), *Path);
		*NewBrush = *FCoreStyle::Get().GetBrush("WhiteBrush");
	}
	const FSlateBrush* Ret = NewBrush.Get();
	Brushes.Add(Key, MoveTemp(NewBrush));
	return Ret;
}

FVector2D FDRXAssets::BrushSize(const TCHAR* Name)
{
	return Brush(Name)->ImageSize;
}

USoundBase* FDRXAssets::Sound(const TCHAR* Slug)
{
	FString Key(Slug);
	if (USoundBase** Found = Sounds.Find(Key))
		return *Found;
	FString Path = FString::Printf(TEXT("/Game/DRX/S/%s.%s"), Slug, Slug);
	USoundBase* S = LoadObject<USoundBase>(nullptr, *Path);
	if (S) S->AddToRoot();
	else UE_LOG(LogTemp, Warning, TEXT("DRX missing sound %s"), *Path);
	Sounds.Add(Key, S);
	return S;
}

void FDRXAssets::Play(UWorld* World, const TCHAR* Slug, float Volume, float Pitch)
{
	if (!World) return;
	if (USoundBase* S = Sound(Slug))
		UGameplayStatics::PlaySound2D(World, S, Volume, Pitch);
}

const FSlateBrush* FDRXAssets::White()
{
	if (!bWhiteInit)
	{
		WhiteBrush = *FCoreStyle::Get().GetBrush("WhiteBrush");
		bWhiteInit = true;
	}
	return &WhiteBrush;
}
