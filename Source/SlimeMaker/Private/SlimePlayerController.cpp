#include "SlimePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

ASlimePlayerController::ASlimePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableTouchEvents = true;
	bEnableMouseOverEvents = true;
}

void ASlimePlayerController::BeginPlay()
{
	Super::BeginPlay();
	FindActiveSlime();
}

void ASlimePlayerController::FindActiveSlime()
{
	if (!ActiveSlime)
	{
		TArray<AActor*> FoundSlimes;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASlimeActor::StaticClass(), FoundSlimes);
		if (FoundSlimes.Num() > 0)
		{
			ActiveSlime = Cast<ASlimeActor>(FoundSlimes[0]);
		}
	}
}

void ASlimePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Touch input (mobile)
	InputComponent->BindTouch(IE_Pressed, this, &ASlimePlayerController::OnTouchBegin);
	InputComponent->BindTouch(IE_Repeat, this, &ASlimePlayerController::OnTouchMove);
	InputComponent->BindTouch(IE_Released, this, &ASlimePlayerController::OnTouchEnd);

	// Mouse click (editor/desktop testing)
	InputComponent->BindAction("LeftClick", IE_Pressed, this, &ASlimePlayerController::OnMouseClick);
	InputComponent->BindAction("LeftClick", IE_Released, this, &ASlimePlayerController::OnMouseRelease);
}

void ASlimePlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsTouching)
	{
		TouchDuration += DeltaTime;
	}

	if (!ActiveSlime)
	{
		FindActiveSlime();
	}

	// Handle mouse drag in editor (simulate touch)
	if (bMouseDown && ActiveSlime)
	{
		float MouseX, MouseY;
		if (GetMousePosition(MouseX, MouseY))
		{
			FVector2D CurrentMouse(MouseX, MouseY);
			float Dist = FVector2D::Distance(CurrentMouse, MouseStartPos);

			if (Dist > DragThreshold)
			{
				FHitResult Hit;
				if (TraceToSlime(CurrentMouse, Hit))
				{
					FVector DragDelta = Hit.ImpactPoint - LastHitWorldPosition;
					ActiveSlime->DragDeform(Hit.ImpactPoint, DragDelta);
					LastHitWorldPosition = Hit.ImpactPoint;
				}
			}
		}
	}
}

// --- Button Hit Detection ---

ASlimePlayerController::EButtonAction ASlimePlayerController::CheckButtonHit(FVector2D ScreenPos)
{
	// Get viewport size
	int32 ViewX, ViewY;
	GetViewportSize(ViewX, ViewY);

	float ScreenW = (float)ViewX;
	float ScreenH = (float)ViewY;

	// Buttons are at the bottom of screen (matching HUD layout)
	float ButtonY = ScreenH - 100.0f;
	float ButtonSpacing = ScreenW / 5.0f;
	float BtnH = 50.0f;

	// Check if touch is in button row
	if (ScreenPos.Y >= ButtonY && ScreenPos.Y <= ButtonY + BtnH)
	{
		for (int32 i = 0; i < 4; i++)
		{
			float BtnX = ButtonSpacing * (i + 0.5f);
			float BtnW = ButtonSpacing * 0.8f;
			float Left = BtnX - BtnW * 0.5f;
			float Right = BtnX + BtnW * 0.5f;

			if (ScreenPos.X >= Left && ScreenPos.X <= Right)
			{
				switch (i)
				{
				case 0: return EButtonAction::Squish;
				case 1: return EButtonAction::Stretch;
				case 2: return EButtonAction::Bounce;
				case 3: return EButtonAction::MegaMorph;
				}
			}
		}
	}

	return EButtonAction::None;
}

// --- Touch Input ---

void ASlimePlayerController::OnTouchBegin(ETouchIndex::Type TouchIndex, FVector Location)
{
	if (TouchIndex != ETouchIndex::Touch1) return;

	FVector2D TouchPos(Location.X, Location.Y);

	// Check buttons first
	EButtonAction Action = CheckButtonHit(TouchPos);
	if (Action != EButtonAction::None && ActiveSlime)
	{
		bTouchHandledByButton = true;
		switch (Action)
		{
		case EButtonAction::Squish:    ActiveSlime->Squish(); break;
		case EButtonAction::Stretch:   ActiveSlime->Stretch(); break;
		case EButtonAction::Bounce:    ActiveSlime->Bounce(); break;
		case EButtonAction::MegaMorph: ActiveSlime->TriggerMegaMorph(); break;
		default: break;
		}
		return;
	}

	bTouchHandledByButton = false;
	bIsTouching = true;
	bIsDragging = false;
	TouchStartPosition = TouchPos;
	LastTouchPosition = TouchPos;
	TouchStartTime = GetWorld()->GetTimeSeconds();
	TouchDuration = 0.0f;

	FHitResult Hit;
	if (TraceToSlime(TouchStartPosition, Hit))
	{
		LastHitWorldPosition = Hit.ImpactPoint;
	}
}

void ASlimePlayerController::OnTouchMove(ETouchIndex::Type TouchIndex, FVector Location)
{
	if (TouchIndex != ETouchIndex::Touch1 || !bIsTouching || bTouchHandledByButton) return;

	FVector2D CurrentTouch(Location.X, Location.Y);
	float DistFromStart = FVector2D::Distance(CurrentTouch, TouchStartPosition);

	if (DistFromStart > DragThreshold)
	{
		bIsDragging = true;
	}

	if (bIsDragging && ActiveSlime)
	{
		FHitResult Hit;
		if (TraceToSlime(CurrentTouch, Hit))
		{
			FVector DragDelta = Hit.ImpactPoint - LastHitWorldPosition;
			ActiveSlime->DragDeform(Hit.ImpactPoint, DragDelta);
			LastHitWorldPosition = Hit.ImpactPoint;
		}
		else
		{
			FVector2D ScreenDelta = CurrentTouch - LastTouchPosition;
			FVector WorldDelta = FVector(0.0f, ScreenDelta.X * 0.5f, -ScreenDelta.Y * 0.5f);
			ActiveSlime->DragDeform(LastHitWorldPosition, WorldDelta);
		}
	}

	LastTouchPosition = CurrentTouch;
}

void ASlimePlayerController::OnTouchEnd(ETouchIndex::Type TouchIndex, FVector Location)
{
	if (TouchIndex != ETouchIndex::Touch1) return;

	if (bTouchHandledByButton)
	{
		bTouchHandledByButton = false;
		return;
	}

	if (!bIsDragging && ActiveSlime && TouchDuration <= PokeMaxDuration)
	{
		FHitResult Hit;
		if (TraceToSlime(FVector2D(Location.X, Location.Y), Hit))
		{
			ActiveSlime->Poke(Hit.ImpactPoint);
		}
	}

	bIsTouching = false;
	bIsDragging = false;
}

// --- Mouse Input (editor testing) ---

void ASlimePlayerController::OnMouseClick()
{
	float MouseX, MouseY;
	if (!GetMousePosition(MouseX, MouseY)) return;

	FVector2D MousePos(MouseX, MouseY);

	// Check buttons
	EButtonAction Action = CheckButtonHit(MousePos);
	if (Action != EButtonAction::None && ActiveSlime)
	{
		switch (Action)
		{
		case EButtonAction::Squish:    ActiveSlime->Squish(); break;
		case EButtonAction::Stretch:   ActiveSlime->Stretch(); break;
		case EButtonAction::Bounce:    ActiveSlime->Bounce(); break;
		case EButtonAction::MegaMorph: ActiveSlime->TriggerMegaMorph(); break;
		default: break;
		}
		return;
	}

	bMouseDown = true;
	MouseStartPos = MousePos;

	FHitResult Hit;
	if (TraceToSlime(MousePos, Hit))
	{
		LastHitWorldPosition = Hit.ImpactPoint;
	}
}

void ASlimePlayerController::OnMouseRelease()
{
	if (bMouseDown && ActiveSlime)
	{
		float MouseX, MouseY;
		if (GetMousePosition(MouseX, MouseY))
		{
			FVector2D MousePos(MouseX, MouseY);
			float Dist = FVector2D::Distance(MousePos, MouseStartPos);

			if (Dist <= DragThreshold)
			{
				// Click = poke
				FHitResult Hit;
				if (TraceToSlime(MousePos, Hit))
				{
					ActiveSlime->Poke(Hit.ImpactPoint);
				}
			}
		}
	}

	bMouseDown = false;
}

// --- Utility ---

bool ASlimePlayerController::TraceToSlime(FVector2D ScreenPosition, FHitResult& OutHit)
{
	FVector WorldLocation, WorldDirection;
	if (DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldLocation, WorldDirection))
	{
		FVector TraceEnd = WorldLocation + WorldDirection * 10000.0f;

		FCollisionQueryParams Params;
		Params.bTraceComplex = true;

		if (GetWorld()->LineTraceSingleByChannel(OutHit, WorldLocation, TraceEnd, ECC_Visibility, Params))
		{
			if (OutHit.GetActor() && OutHit.GetActor()->IsA(ASlimeActor::StaticClass()))
			{
				return true;
			}
		}
	}
	return false;
}
