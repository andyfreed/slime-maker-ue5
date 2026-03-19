#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SlimeActor.h"
#include "SlimePlayerController.generated.h"

UCLASS()
class SLIMEMAKER_API ASlimePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASlimePlayerController();

	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime")
	ASlimeActor* ActiveSlime;

protected:
	virtual void BeginPlay() override;

private:
	// Touch state
	bool bIsTouching = false;
	bool bIsDragging = false;
	bool bTouchHandledByButton = false;
	FVector2D TouchStartPosition;
	FVector2D LastTouchPosition;
	float TouchStartTime = 0.0f;
	float TouchDuration = 0.0f;
	FVector LastHitWorldPosition;

	float DragThreshold = 15.0f;
	float PokeMaxDuration = 0.3f;

	// Input handlers
	void OnTouchBegin(ETouchIndex::Type TouchIndex, FVector Location);
	void OnTouchMove(ETouchIndex::Type TouchIndex, FVector Location);
	void OnTouchEnd(ETouchIndex::Type TouchIndex, FVector Location);

	// Also support mouse clicks for editor testing
	void OnMouseClick();
	void OnMouseRelease();

	// Utility
	bool TraceToSlime(FVector2D ScreenPosition, FHitResult& OutHit);
	void FindActiveSlime();

	// Button hit detection
	enum class EButtonAction { None, Squish, Stretch, Bounce, MegaMorph };
	EButtonAction CheckButtonHit(FVector2D ScreenPos);

	// Mouse state for editor
	bool bMouseDown = false;
	FVector2D MouseStartPos;
};
