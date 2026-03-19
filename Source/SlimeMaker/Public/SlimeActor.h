#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "SlimeTypes.h"
#include "SlimeActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlimeInteraction, ESlimeInteraction, InteractionType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoodChanged, ESlimeMood, NewMood);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMegaMorphTriggered);

UCLASS(BlueprintType, Blueprintable)
class SLIMEMAKER_API ASlimeActor : public AActor
{
	GENERATED_BODY()

public:
	ASlimeActor();

	virtual void Tick(float DeltaTime) override;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slime")
	UProceduralMeshComponent* SlimeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slime")
	UProceduralMeshComponent* InnerGlowMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slime")
	USceneComponent* EyeRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slime")
	UStaticMeshComponent* LeftEye;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slime")
	UStaticMeshComponent* RightEye;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slime")
	UStaticMeshComponent* LeftPupil;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slime")
	UStaticMeshComponent* RightPupil;

	// --- Customization ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Customization")
	FSlimeCustomization Customization;

	// --- Gameplay State ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slime|Gameplay")
	FSlimeGameplayState GameState;

	// --- Delegates ---

	UPROPERTY(BlueprintAssignable, Category = "Slime|Events")
	FOnSlimeInteraction OnSlimeInteraction;

	UPROPERTY(BlueprintAssignable, Category = "Slime|Events")
	FOnMoodChanged OnMoodChanged;

	UPROPERTY(BlueprintAssignable, Category = "Slime|Events")
	FOnMegaMorphTriggered OnMegaMorphTriggered;

	// --- Interaction Functions ---

	UFUNCTION(BlueprintCallable, Category = "Slime|Interaction")
	void Poke(FVector WorldHitPoint);

	UFUNCTION(BlueprintCallable, Category = "Slime|Interaction")
	void Squish();

	UFUNCTION(BlueprintCallable, Category = "Slime|Interaction")
	void Stretch();

	UFUNCTION(BlueprintCallable, Category = "Slime|Interaction")
	void Bounce();

	UFUNCTION(BlueprintCallable, Category = "Slime|Interaction")
	void DragDeform(FVector WorldHitPoint, FVector DragDelta);

	UFUNCTION(BlueprintCallable, Category = "Slime|Interaction")
	void TriggerMegaMorph();

	// --- Customization Functions ---

	UFUNCTION(BlueprintCallable, Category = "Slime|Customization")
	void SetSlimeColor(FLinearColor NewColor);

	UFUNCTION(BlueprintCallable, Category = "Slime|Customization")
	void SetEyeStyle(ESlimeEyeStyle NewStyle);

	UFUNCTION(BlueprintCallable, Category = "Slime|Customization")
	void ApplyCustomization(const FSlimeCustomization& NewCustomization);

	// --- Material (optional overrides — auto-created if null) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Material")
	UMaterialInterface* SlimeBaseMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Material")
	UMaterialInterface* EyeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Material")
	UMaterialInterface* PupilMaterial;

protected:
	virtual void BeginPlay() override;

private:
	// --- Mesh Generation ---
	void GenerateSlimeMesh();
	void GenerateInnerGlowMesh();
	void UpdateMeshDeformation(float DeltaTime);
	void RecalculateNormals();
	void SetupEyeMeshes();
	void CreateDefaultMaterials();

	// --- Deformation Data ---
	TArray<FSlimeVertex> Vertices;
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;

	int32 SphereResolution = 48;
	float SlimeRadius = 100.0f;

	// --- Physics Simulation ---
	float Stiffness = 18.0f;
	float Damping = 4.5f;
	float DeformFalloff = 80.0f;
	float MaxDeformation = 45.0f;

	// --- Animation State ---
	bool bIsMegaMorphing = false;
	float MegaMorphTimer = 0.0f;
	float MegaMorphDuration = 2.0f;
	float IdleWobbleTime = 0.0f;

	// --- Combo Tracking ---
	float ComboWindowSeconds = 1.7f;

	// --- Energy ---
	float EnergyDecayRate = 1.176f;

	// --- Internal ---
	void AddEnergy(float Amount);
	void ProcessCombo();
	float GetComboBonus() const;
	void UpdateMood();
	void UpdateEyes(float DeltaTime);
	void ApplyIdleAnimation(float DeltaTime);
	void ApplyMegaMorphAnimation(float DeltaTime);

	// Material instances
	UPROPERTY()
	UMaterialInstanceDynamic* SlimeDynamicMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* InnerGlowDynamicMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* EyeDynamicMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* PupilDynamicMaterial;

	// Eye animation
	float BlinkTimer = 0.0f;
	float BlinkInterval = 3.5f;
	bool bIsBlinking = false;

	// Cached sphere mesh for eyes
	UPROPERTY()
	UStaticMesh* SphereMesh;
};
