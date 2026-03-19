#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SlimeActor.h"
#include "SlimeHUD.generated.h"

UCLASS()
class SLIMEMAKER_API ASlimeHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	virtual void BeginPlay() override;

private:
	ASlimeActor* TrackedSlime = nullptr;

	void DrawEnergyBar(float X, float Y, float Width, float Height);
	void DrawMoodIndicator(float X, float Y);
	void DrawComboCounter(float X, float Y);
	void DrawCoinCounter(float X, float Y);
	void DrawActionButtons(float ScreenWidth, float ScreenHeight);

	FLinearColor GetMoodColor(ESlimeMood Mood) const;
	FString GetMoodText(ESlimeMood Mood) const;

	void FindTrackedSlime();
};
