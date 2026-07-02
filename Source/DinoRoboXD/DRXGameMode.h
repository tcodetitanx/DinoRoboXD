#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "DRXSim.h"
#include "DRXGameMode.generated.h"

class SDRXGameWidget;
class UAudioComponent;

UCLASS()
class ADRXPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	ADRXPlayerController();
};

UCLASS()
class ADRXGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADRXGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	void StartNewGame();
	FDRXSim* Sim() { return SimInstance.Get(); }

	// single-slot save system (Saved/SaveGames/drx_save.sav)
	bool SaveGame();
	bool LoadGame();
	bool HasSaveFile() const;
	static FString SavePath();

	// music: switches track with a hard cut; loops via bLooping on the wave
	void PlayMusic(const TCHAR* Slug, float Volume = 0.5f);
	void StopMusic();

	bool bSimPaused = false;
	float SimSpeed = 1.f;

private:
	TUniquePtr<FDRXSim> SimInstance;
	TSharedPtr<SDRXGameWidget> GameWidget;

	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComp;
	FString CurrentMusic;
	float AutosaveT = 0;
};
