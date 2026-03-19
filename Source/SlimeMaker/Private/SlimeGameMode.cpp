#include "SlimeGameMode.h"
#include "SlimePlayerController.h"
#include "SlimeHUD.h"
#include "Camera/CameraActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"

ASlimeGameMode::ASlimeGameMode()
{
	PlayerControllerClass = ASlimePlayerController::StaticClass();
	HUDClass = ASlimeHUD::StaticClass();
	DefaultPawnClass = nullptr;
}

void ASlimeGameMode::BeginPlay()
{
	Super::BeginPlay();

	SetupScene();

	// Spawn the default slime
	TArray<AActor*> ExistingSlimes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASlimeActor::StaticClass(), ExistingSlimes);

	if (ExistingSlimes.Num() == 0)
	{
		FSlimeCustomization DefaultCustom;
		DefaultCustom.BaseColor = GetColorByName(TEXT("Mint"));
		DefaultCustom.SlimeName = TEXT("Slimey");
		SpawnSlime(FVector(0.0f, 0.0f, 0.0f), DefaultCustom);
	}
}

void ASlimeGameMode::SetupScene()
{
	SpawnCamera();
	SpawnLighting();
	SpawnFloor();
	SpawnBackground();
}

void ASlimeGameMode::SpawnCamera()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Camera positioned in front of slime, slightly above, looking down at it
	FVector CamPos(350.0f, 0.0f, 150.0f);
	FRotator CamRot(-15.0f, -180.0f, 0.0f); // Look back at origin, slight downward angle

	ACameraActor* Camera = GetWorld()->SpawnActor<ACameraActor>(CamPos, CamRot, Params);
	if (Camera)
	{
		// Set as the view target for player 0
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PC)
		{
			PC->SetViewTarget(Camera);
		}
	}
}

void ASlimeGameMode::SpawnLighting()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Main directional light (sun)
	{
		ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(
			FVector(0.0f, 0.0f, 500.0f),
			FRotator(-45.0f, -30.0f, 0.0f),
			Params
		);
		if (Sun && Sun->GetLightComponent())
		{
			UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
			if (Light)
			{
				Light->SetIntensity(4.0f);
				Light->SetLightColor(FLinearColor(1.0f, 0.97f, 0.9f)); // Warm white
				Light->SetCastShadows(true);
			}
		}
	}

	// Fill light from the opposite side (softer)
	{
		APointLight* FillLight = GetWorld()->SpawnActor<APointLight>(
			FVector(-200.0f, 150.0f, 200.0f),
			FRotator::ZeroRotator,
			Params
		);
		if (FillLight && FillLight->PointLightComponent)
		{
			FillLight->PointLightComponent->SetIntensity(3000.0f);
			FillLight->PointLightComponent->SetLightColor(FLinearColor(0.7f, 0.8f, 1.0f)); // Cool blue fill
			FillLight->PointLightComponent->SetAttenuationRadius(800.0f);
			FillLight->PointLightComponent->SetCastShadows(false);
		}
	}

	// Rim/back light for nice edge highlights on slime
	{
		APointLight* RimLight = GetWorld()->SpawnActor<APointLight>(
			FVector(-150.0f, -100.0f, 250.0f),
			FRotator::ZeroRotator,
			Params
		);
		if (RimLight && RimLight->PointLightComponent)
		{
			RimLight->PointLightComponent->SetIntensity(5000.0f);
			RimLight->PointLightComponent->SetLightColor(FLinearColor(1.0f, 0.85f, 1.0f)); // Slight pink
			RimLight->PointLightComponent->SetAttenuationRadius(600.0f);
			RimLight->PointLightComponent->SetCastShadows(false);
		}
	}

	// Sky light for ambient fill
	{
		ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(
			FVector(0.0f, 0.0f, 300.0f),
			FRotator::ZeroRotator,
			Params
		);
		if (Sky && Sky->GetLightComponent())
		{
			USkyLightComponent* SkyComp = Cast<USkyLightComponent>(Sky->GetLightComponent());
			if (SkyComp)
			{
				SkyComp->SetIntensity(1.5f);
				SkyComp->SetLightColor(FLinearColor(0.6f, 0.7f, 0.9f));
				SkyComp->RecaptureSky();
			}
		}
	}
}

void ASlimeGameMode::SpawnFloor()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn a flat plane as the floor
	AStaticMeshActor* Floor = GetWorld()->SpawnActor<AStaticMeshActor>(
		FVector(0.0f, 0.0f, -2.0f),
		FRotator::ZeroRotator,
		Params
	);

	if (Floor)
	{
		UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		if (PlaneMesh)
		{
			Floor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
			Floor->SetActorScale3D(FVector(8.0f, 8.0f, 1.0f)); // Big floor

			// Create a nice floor material
			UMaterial* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			if (BaseMat)
			{
				UMaterialInstanceDynamic* FloorMat = UMaterialInstanceDynamic::Create(BaseMat, Floor);
				FloorMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.85f, 0.8f, 0.9f)); // Soft lavender-gray
				Floor->GetStaticMeshComponent()->SetMaterial(0, FloorMat);
			}
		}

		Floor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ASlimeGameMode::SpawnBackground()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Height fog for a dreamy background
	AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(
		FVector(0.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator,
		Params
	);

	if (Fog && Fog->GetComponent())
	{
		UExponentialHeightFogComponent* FogComp = Fog->GetComponent();
		FogComp->SetFogDensity(0.02f);
		FogComp->SetFogHeightFalloff(0.5f);
		FogComp->SetFogInscatteringColor(FLinearColor(0.55f, 0.5f, 0.75f)); // Purple-ish haze
		FogComp->SetFogMaxOpacity(0.8f);
	}
}

ASlimeActor* ASlimeGameMode::SpawnSlime(FVector Location, const FSlimeCustomization& Customization)
{
	UClass* SpawnClass = SlimeClass ? (UClass*)SlimeClass : ASlimeActor::StaticClass();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASlimeActor* NewSlime = GetWorld()->SpawnActor<ASlimeActor>(SpawnClass, Location, FRotator::ZeroRotator, SpawnParams);
	if (NewSlime)
	{
		NewSlime->ApplyCustomization(Customization);
	}

	return NewSlime;
}

TArray<FLinearColor> ASlimeGameMode::GetAvailableColors()
{
	TArray<FLinearColor> Colors;
	Colors.Add(FLinearColor(0.4f, 1.0f, 0.85f));   // Mint
	Colors.Add(FLinearColor(0.53f, 0.81f, 0.98f));  // Sky Blue
	Colors.Add(FLinearColor(0.78f, 0.64f, 0.96f));  // Lavender
	Colors.Add(FLinearColor(1.0f, 0.71f, 0.76f));   // Pink
	Colors.Add(FLinearColor(1.0f, 0.97f, 0.6f));    // Lemon
	Colors.Add(FLinearColor(1.0f, 0.5f, 0.38f));    // Coral
	Colors.Add(FLinearColor(0.0f, 0.81f, 0.82f));   // Teal
	Colors.Add(FLinearColor(1.0f, 0.84f, 0.0f));    // Gold
	Colors.Add(FLinearColor(1.0f, 0.41f, 0.71f));   // Hot Pink
	Colors.Add(FLinearColor(0.6f, 0.2f, 0.8f));     // Purple
	Colors.Add(FLinearColor(1.0f, 0.25f, 0.25f));   // Red
	Colors.Add(FLinearColor(0.15f, 0.15f, 0.2f));   // Shadow
	Colors.Add(FLinearColor(1.0f, 0.85f, 0.73f));   // Peach
	Colors.Add(FLinearColor(0.75f, 0.93f, 1.0f));   // Ice
	return Colors;
}

FLinearColor ASlimeGameMode::GetColorByName(const FString& ColorName)
{
	if (ColorName == TEXT("Mint"))      return FLinearColor(0.4f, 1.0f, 0.85f);
	if (ColorName == TEXT("SkyBlue"))   return FLinearColor(0.53f, 0.81f, 0.98f);
	if (ColorName == TEXT("Lavender"))  return FLinearColor(0.78f, 0.64f, 0.96f);
	if (ColorName == TEXT("Pink"))      return FLinearColor(1.0f, 0.71f, 0.76f);
	if (ColorName == TEXT("Lemon"))     return FLinearColor(1.0f, 0.97f, 0.6f);
	if (ColorName == TEXT("Coral"))     return FLinearColor(1.0f, 0.5f, 0.38f);
	if (ColorName == TEXT("Teal"))      return FLinearColor(0.0f, 0.81f, 0.82f);
	if (ColorName == TEXT("Gold"))      return FLinearColor(1.0f, 0.84f, 0.0f);
	if (ColorName == TEXT("HotPink"))   return FLinearColor(1.0f, 0.41f, 0.71f);
	if (ColorName == TEXT("Purple"))    return FLinearColor(0.6f, 0.2f, 0.8f);
	if (ColorName == TEXT("Red"))       return FLinearColor(1.0f, 0.25f, 0.25f);
	if (ColorName == TEXT("Shadow"))    return FLinearColor(0.15f, 0.15f, 0.2f);
	if (ColorName == TEXT("Peach"))     return FLinearColor(1.0f, 0.85f, 0.73f);
	if (ColorName == TEXT("Ice"))       return FLinearColor(0.75f, 0.93f, 1.0f);
	return FLinearColor(0.4f, 1.0f, 0.85f);
}
