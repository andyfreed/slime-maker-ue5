#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SlimeActor.h"
#include "SlimeGameMode.generated.h"

UCLASS()
class SLIMEMAKER_API ASlimeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASlimeGameMode();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Slime")
	ASlimeActor* SpawnSlime(FVector Location, const FSlimeCustomization& Customization);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Slime")
	static TArray<FLinearColor> GetAvailableColors();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Slime")
	static FLinearColor GetColorByName(const FString& ColorName);

	UPROPERTY(EditDefaultsOnly, Category = "Slime")
	TSubclassOf<ASlimeActor> SlimeClass;

private:
	void SetupScene();
	void SpawnCamera();
	void SpawnLighting();
	void SpawnFloor();
	void SpawnBackground();
};
