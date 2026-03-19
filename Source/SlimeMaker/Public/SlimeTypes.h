#pragma once

#include "CoreMinimal.h"
#include "SlimeTypes.generated.h"

// --- Enums ---

UENUM(BlueprintType)
enum class ESlimeInteraction : uint8
{
	None,
	Poke,
	Squish,
	Stretch,
	Bounce,
	Drag,
	MegaMorph
};

UENUM(BlueprintType)
enum class ESlimeMood : uint8
{
	Chill,      // 0-19 energy
	Happy,      // 20-44
	Playful,    // 45-74
	Hyper,      // 75-99
	Legendary   // 100
};

UENUM(BlueprintType)
enum class ESlimeEyeStyle : uint8
{
	Normal,
	Googly,
	Cyclops,
	Alien,
	HeartEyes,
	Sleepy,
	Angry,
	XEyes,
	StarEyes,
	Dizzy
};

UENUM(BlueprintType)
enum class ESlimeSparkle : uint8
{
	None,
	Stars,
	Hearts,
	Rainbow,
	Snowflakes,
	Diamonds
};

// --- Structs ---

USTRUCT(BlueprintType)
struct FSlimeCustomization
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime")
	FLinearColor BaseColor = FLinearColor(0.4f, 1.0f, 0.85f, 1.0f); // Mint

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime")
	ESlimeEyeStyle EyeStyle = ESlimeEyeStyle::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime")
	ESlimeSparkle Sparkle = ESlimeSparkle::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime")
	FString CharmEmoji = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime")
	FString SlimeName = TEXT("Slime");
};

USTRUCT(BlueprintType)
struct FSlimeGameplayState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float Energy = 0.0f; // 0-100

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	int32 ComboCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	ESlimeMood CurrentMood = ESlimeMood::Chill;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	int32 Coins = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float TimeSinceLastAction = 0.0f;

	ESlimeMood GetMoodFromEnergy() const
	{
		if (Energy >= 100.0f) return ESlimeMood::Legendary;
		if (Energy >= 75.0f) return ESlimeMood::Hyper;
		if (Energy >= 45.0f) return ESlimeMood::Playful;
		if (Energy >= 20.0f) return ESlimeMood::Happy;
		return ESlimeMood::Chill;
	}
};

// Deformation vertex data
USTRUCT()
struct FSlimeVertex
{
	GENERATED_BODY()

	FVector OriginalPosition;
	FVector CurrentPosition;
	FVector Velocity;
	FVector Normal;
};
