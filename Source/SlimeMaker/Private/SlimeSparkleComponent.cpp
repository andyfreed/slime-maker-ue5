#include "SlimeSparkleComponent.h"
#include "DrawDebugHelpers.h"

USlimeSparkleComponent::USlimeSparkleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USlimeSparkleComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USlimeSparkleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (SparkleType == ESlimeSparkle::None) return;

	// Spawn new sparkles
	SpawnTimer += DeltaTime;
	float SpawnInterval = 0.15f;
	if (SpawnTimer >= SpawnInterval && Sparkles.Num() < MaxSparkles)
	{
		SpawnSparkle();
		SpawnTimer = 0.0f;
	}

	UpdateSparkles(DeltaTime);
}

void USlimeSparkleComponent::SpawnSparkle()
{
	FSparkleParticle NewSparkle;

	// Random position on sphere surface
	float Theta = FMath::RandRange(0.0f, 2.0f * PI);
	float Phi = FMath::RandRange(0.2f, PI * 0.8f); // Avoid exact top/bottom

	NewSparkle.Position = GetComponentLocation() + FVector(
		SparkleRadius * FMath::Sin(Phi) * FMath::Cos(Theta),
		SparkleRadius * FMath::Sin(Phi) * FMath::Sin(Theta),
		SparkleRadius * FMath::Cos(Phi) * 0.7f + 30.0f // Offset for slime shape
	);

	NewSparkle.LifeTime = 0.0f;
	NewSparkle.MaxLife = FMath::RandRange(0.8f, 2.0f);
	NewSparkle.Size = FMath::RandRange(2.0f, 6.0f);
	NewSparkle.Phase = FMath::RandRange(0.0f, 2.0f * PI);

	Sparkles.Add(NewSparkle);
}

void USlimeSparkleComponent::UpdateSparkles(float DeltaTime)
{
	FColor DrawColor = SparkleColor.ToFColor(true);

	for (int32 i = Sparkles.Num() - 1; i >= 0; i--)
	{
		FSparkleParticle& S = Sparkles[i];
		S.LifeTime += DeltaTime;

		if (S.LifeTime >= S.MaxLife)
		{
			Sparkles.RemoveAt(i);
			continue;
		}

		// Fade in/out
		float LifeRatio = S.LifeTime / S.MaxLife;
		float Alpha = 1.0f;
		if (LifeRatio < 0.2f) Alpha = LifeRatio / 0.2f;
		else if (LifeRatio > 0.7f) Alpha = (1.0f - LifeRatio) / 0.3f;

		// Twinkle
		float Twinkle = FMath::Sin(S.LifeTime * SparkleSpeed * 8.0f + S.Phase) * 0.5f + 0.5f;
		Alpha *= Twinkle;

		// Float upward slowly
		S.Position.Z += DeltaTime * 15.0f;

		// Draw sparkle point (debug visualization — replace with Niagara in production)
		float VisualSize = S.Size * Alpha;

		FColor FinalColor = DrawColor;
		FinalColor.A = (uint8)(Alpha * 255);

#if ENABLE_DRAW_DEBUG
		DrawDebugPoint(GetWorld(), S.Position, VisualSize, FinalColor, false, -1.0f);
#endif
	}
}

void USlimeSparkleComponent::SetSparkleType(ESlimeSparkle NewType)
{
	SparkleType = NewType;

	// Adjust behavior per type
	switch (NewType)
	{
	case ESlimeSparkle::Stars:
		SparkleSpeed = 1.5f;
		MaxSparkles = 15;
		break;
	case ESlimeSparkle::Hearts:
		SparkleSpeed = 1.0f;
		MaxSparkles = 12;
		break;
	case ESlimeSparkle::Rainbow:
		SparkleSpeed = 2.0f;
		MaxSparkles = 25;
		break;
	case ESlimeSparkle::Snowflakes:
		SparkleSpeed = 0.8f;
		MaxSparkles = 18;
		break;
	case ESlimeSparkle::Diamonds:
		SparkleSpeed = 1.2f;
		MaxSparkles = 10;
		break;
	default:
		Sparkles.Empty();
		break;
	}
}

void USlimeSparkleComponent::SetSparkleColor(FLinearColor NewColor)
{
	SparkleColor = NewColor;
}
