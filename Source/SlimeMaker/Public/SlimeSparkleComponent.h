#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SlimeTypes.h"
#include "SlimeSparkleComponent.generated.h"

// Simple billboard sparkle particle system for the slime surface
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SLIMEMAKER_API USlimeSparkleComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	USlimeSparkleComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Slime|Sparkle")
	void SetSparkleType(ESlimeSparkle NewType);

	UFUNCTION(BlueprintCallable, Category = "Slime|Sparkle")
	void SetSparkleColor(FLinearColor NewColor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparkle")
	ESlimeSparkle SparkleType = ESlimeSparkle::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparkle")
	FLinearColor SparkleColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparkle")
	int32 MaxSparkles = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparkle")
	float SparkleRadius = 110.0f; // Slightly larger than slime

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparkle")
	float SparkleSpeed = 1.5f;

protected:
	virtual void BeginPlay() override;

private:
	struct FSparkleParticle
	{
		FVector Position;
		float LifeTime;
		float MaxLife;
		float Size;
		float Phase; // For animation variation
	};

	TArray<FSparkleParticle> Sparkles;
	float SpawnTimer = 0.0f;

	void SpawnSparkle();
	void UpdateSparkles(float DeltaTime);
};
