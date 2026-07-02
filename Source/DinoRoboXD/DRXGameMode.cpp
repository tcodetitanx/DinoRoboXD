#include "DRXGameMode.h"
#include "SDRXGameWidget.h"
#include "DRXAssets.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"

ADRXPlayerController::ADRXPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;   // the Slate widget handles everything
	bEnableMouseOverEvents = false;
}

ADRXGameMode::ADRXGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PlayerControllerClass = ADRXPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void ADRXGameMode::BeginPlay()
{
	Super::BeginPlay();

	GameWidget = SNew(SDRXGameWidget, this);
	if (UGameViewportClient* VC = GetGameInstance()->GetGameViewportClient())
		VC->AddViewportWidgetContent(GameWidget.ToSharedRef(), 0);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(GameWidget);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
	FSlateApplication::Get().SetKeyboardFocus(GameWidget);
}

void ADRXGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (SimInstance && !bSimPaused)
	{
		int32 Steps = FMath::RoundToInt(SimSpeed);
		for (int32 i = 0; i < FMath::Max(1, Steps); i++)
			SimInstance->Tick(DeltaSeconds);

		// autosave every 3 minutes of active play
		if (!SimInstance->bVictory && !SimInstance->bDefeat)
		{
			AutosaveT += DeltaSeconds;
			if (AutosaveT > 180.f)
			{
				AutosaveT = 0;
				if (SaveGame())
					SimInstance->AddAlert(TEXT("Autosaved."), FLinearColor(0.5f, 0.8f, 1.f));
			}
		}
	}
}

FString ADRXGameMode::SavePath()
{
	return FPaths::ProjectSavedDir() / TEXT("SaveGames") / TEXT("drx_save.sav");
}

bool ADRXGameMode::HasSaveFile() const
{
	return FPaths::FileExists(SavePath());
}

bool ADRXGameMode::SaveGame()
{
	if (!SimInstance) return false;
	TArray<uint8> Data;
	FMemoryWriter Writer(Data);
	if (!SimInstance->Serialize(Writer)) return false;
	return FFileHelper::SaveArrayToFile(Data, *SavePath());
}

bool ADRXGameMode::LoadGame()
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *SavePath())) return false;
	TUniquePtr<FDRXSim> Loaded = MakeUnique<FDRXSim>();
	FMemoryReader Reader(Data);
	if (!Loaded->Serialize(Reader))
	{
		UE_LOG(LogTemp, Warning, TEXT("DRX save file failed to load (version mismatch?)"));
		return false;
	}
	SimInstance = MoveTemp(Loaded);
	bSimPaused = false;
	SimSpeed = 1.f;
	AutosaveT = 0;
	return true;
}

void ADRXGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GameWidget.IsValid())
		if (UGameViewportClient* VC = GetGameInstance()->GetGameViewportClient())
			VC->RemoveViewportWidgetContent(GameWidget.ToSharedRef());
	GameWidget.Reset();
	Super::EndPlay(Reason);
}

void ADRXGameMode::StartNewGame()
{
	SimInstance = MakeUnique<FDRXSim>();
	SimInstance->Init((int32)(FPlatformTime::Cycles() & 0x7fffffff));
	bSimPaused = false;
	SimSpeed = 1.f;
}

void ADRXGameMode::PlayMusic(const TCHAR* Slug, float Volume)
{
	if (CurrentMusic == Slug) return;
	CurrentMusic = Slug;
	if (MusicComp)
	{
		MusicComp->FadeOut(0.8f, 0.f);
		MusicComp = nullptr;
	}
	USoundBase* S = FDRXAssets::Get().Sound(Slug);
	if (!S) return;
	if (USoundWave* W = Cast<USoundWave>(S))
		W->bLooping = true;
	MusicComp = UGameplayStatics::SpawnSound2D(this, S, Volume, 1.f, 0.f, nullptr, true, false);
	if (MusicComp) MusicComp->FadeIn(1.2f, Volume);
}

void ADRXGameMode::StopMusic()
{
	CurrentMusic.Reset();
	if (MusicComp)
	{
		MusicComp->FadeOut(0.8f, 0.f);
		MusicComp = nullptr;
	}
}
