#include "SlimeHUD.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

void ASlimeHUD::BeginPlay()
{
	Super::BeginPlay();
	FindTrackedSlime();
}

void ASlimeHUD::FindTrackedSlime()
{
	TArray<AActor*> Slimes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASlimeActor::StaticClass(), Slimes);
	if (Slimes.Num() > 0)
	{
		TrackedSlime = Cast<ASlimeActor>(Slimes[0]);
	}
}

void ASlimeHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!TrackedSlime)
	{
		FindTrackedSlime();
		if (!TrackedSlime) return;
	}

	const float ScreenW = Canvas->SizeX;
	const float ScreenH = Canvas->SizeY;
	const float Padding = 20.0f;
	const float SafeTop = 60.0f; // Mobile safe area

	// Top bar background
	FLinearColor BarBG(0.0f, 0.0f, 0.0f, 0.4f);
	DrawRect(BarBG, 0.0f, 0.0f, ScreenW, SafeTop + 80.0f);

	// Mood indicator (top left)
	DrawMoodIndicator(Padding, SafeTop);

	// Energy bar (top center)
	float BarWidth = ScreenW * 0.45f;
	float BarX = (ScreenW - BarWidth) * 0.5f;
	DrawEnergyBar(BarX, SafeTop + 10.0f, BarWidth, 24.0f);

	// Combo counter (below energy bar)
	if (TrackedSlime->GameState.ComboCount > 1)
	{
		DrawComboCounter(ScreenW * 0.5f, SafeTop + 45.0f);
	}

	// Coins (top right)
	DrawCoinCounter(ScreenW - Padding - 120.0f, SafeTop);

	// Slime name
	FString SlimeName = TrackedSlime->Customization.SlimeName;
	float TextW, TextH;
	GetTextSize(SlimeName, TextW, TextH, GEngine->GetMediumFont(), 1.5f);
	DrawText(SlimeName, FLinearColor::White, (ScreenW - TextW) * 0.5f, ScreenH - 180.0f, GEngine->GetMediumFont(), 1.5f);

	// Action buttons at bottom
	DrawActionButtons(ScreenW, ScreenH);

	// Mega Morph indicator
	if (TrackedSlime->GameState.Energy >= 100.0f)
	{
		float Pulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 6.0f) * 0.3f + 0.7f;
		FLinearColor GoldPulse(1.0f, 0.84f, 0.0f, Pulse);

		FString MegaText = TEXT("MEGA MORPH READY!");
		float MW, MH;
		GetTextSize(MegaText, MW, MH, GEngine->GetLargeFont(), 2.0f);
		DrawText(MegaText, GoldPulse, (ScreenW - MW) * 0.5f, ScreenH * 0.35f, GEngine->GetLargeFont(), 2.0f);
	}
}

void ASlimeHUD::DrawEnergyBar(float X, float Y, float Width, float Height)
{
	float Energy = TrackedSlime->GameState.Energy;
	FLinearColor MoodColor = GetMoodColor(TrackedSlime->GameState.CurrentMood);

	// Background
	DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 0.6f), X - 2, Y - 2, Width + 4, Height + 4);

	// Fill
	float FillWidth = (Energy / 100.0f) * Width;
	DrawRect(MoodColor, X, Y, FillWidth, Height);

	// Border
	FLinearColor Border(1.0f, 1.0f, 1.0f, 0.3f);
	DrawRect(Border, X, Y, Width, 2.0f);
	DrawRect(Border, X, Y + Height - 2.0f, Width, 2.0f);
	DrawRect(Border, X, Y, 2.0f, Height);
	DrawRect(Border, X + Width - 2.0f, Y, 2.0f, Height);

	// Energy text
	FString EnergyText = FString::Printf(TEXT("%.0f%%"), Energy);
	float TW, TH;
	GetTextSize(EnergyText, TW, TH, GEngine->GetSmallFont(), 1.0f);
	DrawText(EnergyText, FLinearColor::White, X + (Width - TW) * 0.5f, Y + (Height - TH) * 0.5f, GEngine->GetSmallFont(), 1.0f);
}

void ASlimeHUD::DrawMoodIndicator(float X, float Y)
{
	FLinearColor MoodColor = GetMoodColor(TrackedSlime->GameState.CurrentMood);
	FString MoodText = GetMoodText(TrackedSlime->GameState.CurrentMood);

	DrawText(MoodText, MoodColor, X, Y, GEngine->GetMediumFont(), 1.2f);
}

void ASlimeHUD::DrawComboCounter(float X, float Y)
{
	int32 Combo = TrackedSlime->GameState.ComboCount;
	FString ComboText = FString::Printf(TEXT("x%d COMBO"), Combo);

	FLinearColor ComboColor = FLinearColor::White;
	float Scale = 1.0f;

	if (Combo >= 10)
	{
		ComboColor = FLinearColor(1.0f, 0.84f, 0.0f); // Gold
		Scale = 1.4f;
	}
	else if (Combo >= 5)
	{
		ComboColor = FLinearColor(1.0f, 0.5f, 0.0f); // Orange
		Scale = 1.2f;
	}

	float TW, TH;
	GetTextSize(ComboText, TW, TH, GEngine->GetSmallFont(), Scale);
	DrawText(ComboText, ComboColor, X - TW * 0.5f, Y, GEngine->GetSmallFont(), Scale);
}

void ASlimeHUD::DrawCoinCounter(float X, float Y)
{
	FString CoinText = FString::Printf(TEXT("%d coins"), TrackedSlime->GameState.Coins);
	DrawText(CoinText, FLinearColor(1.0f, 0.84f, 0.0f), X, Y, GEngine->GetMediumFont(), 1.2f);
}

void ASlimeHUD::DrawActionButtons(float ScreenWidth, float ScreenHeight)
{
	// Draw button labels at the bottom of the screen
	// These serve as visual indicators — actual input is handled by the UI widget
	float ButtonY = ScreenHeight - 100.0f;
	float ButtonSpacing = ScreenWidth / 5.0f;

	FString Actions[] = { TEXT("SQUISH"), TEXT("STRETCH"), TEXT("BOUNCE"), TEXT("MORPH") };
	FLinearColor ButtonColors[] = {
		FLinearColor(0.4f, 0.8f, 1.0f),   // Blue
		FLinearColor(0.8f, 0.4f, 1.0f),   // Purple
		FLinearColor(0.4f, 1.0f, 0.6f),   // Green
		FLinearColor(1.0f, 0.84f, 0.0f)   // Gold
	};

	for (int32 i = 0; i < 4; i++)
	{
		float BtnX = ButtonSpacing * (i + 0.5f);
		float BtnW = ButtonSpacing * 0.8f;
		float BtnH = 50.0f;

		// Button background
		FLinearColor BtnBG = ButtonColors[i];
		BtnBG.A = 0.3f;
		DrawRect(BtnBG, BtnX - BtnW * 0.5f, ButtonY, BtnW, BtnH);

		// Button text
		float TW, TH;
		GetTextSize(Actions[i], TW, TH, GEngine->GetSmallFont(), 1.0f);
		DrawText(Actions[i], ButtonColors[i], BtnX - TW * 0.5f, ButtonY + (BtnH - TH) * 0.5f, GEngine->GetSmallFont(), 1.0f);
	}
}

FLinearColor ASlimeHUD::GetMoodColor(ESlimeMood Mood) const
{
	switch (Mood)
	{
	case ESlimeMood::Chill:     return FLinearColor(0.53f, 0.81f, 0.98f);  // Sky blue
	case ESlimeMood::Happy:     return FLinearColor(0.4f, 1.0f, 0.85f);    // Mint
	case ESlimeMood::Playful:   return FLinearColor(0.78f, 0.64f, 0.96f);  // Purple
	case ESlimeMood::Hyper:     return FLinearColor(1.0f, 0.41f, 0.71f);   // Pink
	case ESlimeMood::Legendary: return FLinearColor(1.0f, 0.84f, 0.0f);    // Gold
	default:                    return FLinearColor::White;
	}
}

FString ASlimeHUD::GetMoodText(ESlimeMood Mood) const
{
	switch (Mood)
	{
	case ESlimeMood::Chill:     return TEXT("Chill");
	case ESlimeMood::Happy:     return TEXT("Happy");
	case ESlimeMood::Playful:   return TEXT("Playful");
	case ESlimeMood::Hyper:     return TEXT("HYPER");
	case ESlimeMood::Legendary: return TEXT("LEGENDARY");
	default:                    return TEXT("---");
	}
}
