#include "SlimeActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ASlimeActor::ASlimeActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Outer slime mesh
	SlimeMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SlimeMesh"));
	SlimeMesh->SetupAttachment(Root);
	SlimeMesh->bUseComplexAsSimpleCollision = false;
	SlimeMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SlimeMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	SlimeMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Inner glow mesh (slightly smaller, additive)
	InnerGlowMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("InnerGlowMesh"));
	InnerGlowMesh->SetupAttachment(Root);
	InnerGlowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Eye root
	EyeRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EyeRoot"));
	EyeRoot->SetupAttachment(Root);
	EyeRoot->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));

	// Eyes
	LeftEye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftEye"));
	LeftEye->SetupAttachment(EyeRoot);
	LeftEye->SetRelativeLocation(FVector(70.0f, -22.0f, 35.0f));
	LeftEye->SetRelativeScale3D(FVector(0.22f));
	LeftEye->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RightEye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightEye"));
	RightEye->SetupAttachment(EyeRoot);
	RightEye->SetRelativeLocation(FVector(70.0f, 22.0f, 35.0f));
	RightEye->SetRelativeScale3D(FVector(0.22f));
	RightEye->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Pupils
	LeftPupil = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPupil"));
	LeftPupil->SetupAttachment(LeftEye);
	LeftPupil->SetRelativeLocation(FVector(45.0f, 0.0f, 0.0f));
	LeftPupil->SetRelativeScale3D(FVector(0.45f));
	LeftPupil->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RightPupil = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPupil"));
	RightPupil->SetupAttachment(RightEye);
	RightPupil->SetRelativeLocation(FVector(45.0f, 0.0f, 0.0f));
	RightPupil->SetRelativeScale3D(FVector(0.45f));
	RightPupil->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Load sphere mesh for eyes
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereAsset.Succeeded())
	{
		SphereMesh = SphereAsset.Object;
		LeftEye->SetStaticMesh(SphereMesh);
		RightEye->SetStaticMesh(SphereMesh);
		LeftPupil->SetStaticMesh(SphereMesh);
		RightPupil->SetStaticMesh(SphereMesh);
	}
}

void ASlimeActor::BeginPlay()
{
	Super::BeginPlay();

	GenerateSlimeMesh();
	GenerateInnerGlowMesh();
	CreateDefaultMaterials();

	BlinkInterval = FMath::RandRange(2.5f, 5.0f);
}

void ASlimeActor::CreateDefaultMaterials()
{
	// --- Slime body material (translucent, glossy) ---
	{
		UMaterial* TranslucentBase = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
		UMaterialInterface* MatToUse = SlimeBaseMaterial ? SlimeBaseMaterial : TranslucentBase;

		if (MatToUse)
		{
			SlimeDynamicMaterial = UMaterialInstanceDynamic::Create(MatToUse, this);
			// Set slime color with some translucency feel via vertex colors
			SlimeMesh->SetMaterial(0, SlimeDynamicMaterial);
			SetSlimeColor(Customization.BaseColor);
		}
	}

	// --- Inner glow (emissive core) ---
	{
		UMaterial* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
		if (BaseMat)
		{
			InnerGlowDynamicMaterial = UMaterialInstanceDynamic::Create(BaseMat, this);
			InnerGlowMesh->SetMaterial(0, InnerGlowDynamicMaterial);
		}
	}

	// --- Eye whites ---
	{
		UMaterial* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (BaseMat)
		{
			EyeDynamicMaterial = UMaterialInstanceDynamic::Create(BaseMat, this);
			EyeDynamicMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 1.0f, 1.0f));
			LeftEye->SetMaterial(0, EyeDynamicMaterial);
			RightEye->SetMaterial(0, EyeDynamicMaterial);
		}
	}

	// --- Pupils (dark) ---
	{
		UMaterial* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (BaseMat)
		{
			PupilDynamicMaterial = UMaterialInstanceDynamic::Create(BaseMat, this);
			PupilDynamicMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.02f, 0.05f));
			LeftPupil->SetMaterial(0, PupilDynamicMaterial);
			RightPupil->SetMaterial(0, PupilDynamicMaterial);
		}
	}
}

// --- Mesh Generation ---

void ASlimeActor::GenerateSlimeMesh()
{
	Vertices.Empty();
	Triangles.Empty();
	UVs.Empty();
	VertexColors.Empty();

	const int32 Rings = SphereResolution;
	const int32 Segments = SphereResolution;

	for (int32 Ring = 0; Ring <= Rings; Ring++)
	{
		float Phi = PI * (float)Ring / (float)Rings;
		float CosP = FMath::Cos(Phi);
		float SinP = FMath::Sin(Phi);

		for (int32 Seg = 0; Seg <= Segments; Seg++)
		{
			float Theta = 2.0f * PI * (float)Seg / (float)Segments;

			FVector Pos;
			Pos.X = SinP * FMath::Cos(Theta) * SlimeRadius;
			Pos.Y = SinP * FMath::Sin(Theta) * SlimeRadius;
			Pos.Z = CosP * SlimeRadius;

			// Slime shape: flat bottom, rounded top
			if (Pos.Z < 0.0f)
			{
				Pos.Z *= 0.3f;
				float FlattenFactor = 1.0f + FMath::Abs(Pos.Z / (SlimeRadius * 0.3f)) * 0.3f;
				Pos.X *= FlattenFactor;
				Pos.Y *= FlattenFactor;
			}
			else
			{
				float BulgeFactor = 1.0f + SinP * 0.08f;
				Pos.Z *= 0.85f;
				Pos.X *= BulgeFactor;
				Pos.Y *= BulgeFactor;
			}

			Pos.Z += SlimeRadius * 0.3f;

			FSlimeVertex Vert;
			Vert.OriginalPosition = Pos;
			Vert.CurrentPosition = Pos;
			Vert.Velocity = FVector::ZeroVector;
			Vert.Normal = Pos.GetSafeNormal();
			Vertices.Add(Vert);

			float U = (float)Seg / (float)Segments;
			float V = (float)Ring / (float)Rings;
			UVs.Add(FVector2D(U, V));
			VertexColors.Add(FColor::White);
		}
	}

	for (int32 Ring = 0; Ring < Rings; Ring++)
	{
		for (int32 Seg = 0; Seg < Segments; Seg++)
		{
			int32 Current = Ring * (Segments + 1) + Seg;
			int32 Next = Current + Segments + 1;

			Triangles.Add(Current);
			Triangles.Add(Next);
			Triangles.Add(Current + 1);

			Triangles.Add(Current + 1);
			Triangles.Add(Next);
			Triangles.Add(Next + 1);
		}
	}

	TArray<FVector> Positions;
	TArray<FVector> Normals;
	TArray<FLinearColor> Colors;
	Positions.Reserve(Vertices.Num());
	Normals.Reserve(Vertices.Num());
	Colors.Reserve(Vertices.Num());

	for (const FSlimeVertex& V : Vertices)
	{
		Positions.Add(V.CurrentPosition);
		Normals.Add(V.Normal);

		// Vertex color gradient: lighter on top, darker at base
		float HeightRatio = FMath::Clamp((V.CurrentPosition.Z) / (SlimeRadius * 0.85f), 0.0f, 1.0f);
		Colors.Add(FLinearColor(HeightRatio * 0.3f + 0.7f, HeightRatio * 0.3f + 0.7f, HeightRatio * 0.3f + 0.7f, 1.0f));
	}

	TArray<FProcMeshTangent> Tangents;
	SlimeMesh->CreateMeshSection_LinearColor(0, Positions, Triangles, Normals, UVs, Colors, Tangents, true);
}

void ASlimeActor::GenerateInnerGlowMesh()
{
	// Smaller sphere inside the slime for a glow/depth effect
	const int32 Rings = 24;
	const int32 Segments = 24;
	const float InnerRadius = SlimeRadius * 0.65f;

	TArray<FVector> Positions;
	TArray<int32> InnerTriangles;
	TArray<FVector> Normals;
	TArray<FVector2D> InnerUVs;
	TArray<FLinearColor> Colors;

	for (int32 Ring = 0; Ring <= Rings; Ring++)
	{
		float Phi = PI * (float)Ring / (float)Rings;
		float CosP = FMath::Cos(Phi);
		float SinP = FMath::Sin(Phi);

		for (int32 Seg = 0; Seg <= Segments; Seg++)
		{
			float Theta = 2.0f * PI * (float)Seg / (float)Segments;

			FVector Pos;
			Pos.X = SinP * FMath::Cos(Theta) * InnerRadius;
			Pos.Y = SinP * FMath::Sin(Theta) * InnerRadius;
			Pos.Z = CosP * InnerRadius;

			if (Pos.Z < 0.0f) Pos.Z *= 0.3f;
			else Pos.Z *= 0.85f;
			Pos.Z += SlimeRadius * 0.3f;

			Positions.Add(Pos);
			Normals.Add(Pos.GetSafeNormal());
			InnerUVs.Add(FVector2D((float)Seg / Segments, (float)Ring / Rings));
			Colors.Add(FLinearColor(1.0f, 1.0f, 1.0f, 0.3f));
		}
	}

	for (int32 Ring = 0; Ring < Rings; Ring++)
	{
		for (int32 Seg = 0; Seg < Segments; Seg++)
		{
			int32 Current = Ring * (Segments + 1) + Seg;
			int32 Next = Current + Segments + 1;

			InnerTriangles.Add(Current);
			InnerTriangles.Add(Next);
			InnerTriangles.Add(Current + 1);

			InnerTriangles.Add(Current + 1);
			InnerTriangles.Add(Next);
			InnerTriangles.Add(Next + 1);
		}
	}

	TArray<FProcMeshTangent> Tangents;
	InnerGlowMesh->CreateMeshSection_LinearColor(0, Positions, InnerTriangles, Normals, InnerUVs, Colors, Tangents, false);
}

// --- Tick ---

void ASlimeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	GameState.TimeSinceLastAction += DeltaTime;
	if (GameState.TimeSinceLastAction > 2.0f && GameState.Energy > 0.0f)
	{
		GameState.Energy = FMath::Max(0.0f, GameState.Energy - EnergyDecayRate * DeltaTime);
	}

	if (GameState.TimeSinceLastAction > ComboWindowSeconds && GameState.ComboCount > 0)
	{
		GameState.ComboCount = 0;
	}

	UpdateMood();
	ApplyIdleAnimation(DeltaTime);

	if (bIsMegaMorphing)
	{
		ApplyMegaMorphAnimation(DeltaTime);
	}

	UpdateMeshDeformation(DeltaTime);
	UpdateEyes(DeltaTime);
}

// --- Deformation Physics ---

void ASlimeActor::UpdateMeshDeformation(float DeltaTime)
{
	bool bMeshChanged = false;

	TArray<FVector> Positions;
	TArray<FVector> Normals;
	TArray<FLinearColor> Colors;
	Positions.Reserve(Vertices.Num());
	Normals.Reserve(Vertices.Num());
	Colors.Reserve(Vertices.Num());

	for (int32 i = 0; i < Vertices.Num(); i++)
	{
		FSlimeVertex& V = Vertices[i];

		FVector Displacement = V.CurrentPosition - V.OriginalPosition;
		FVector SpringForce = -Stiffness * Displacement;
		FVector DampingForce = -Damping * V.Velocity;

		V.Velocity += (SpringForce + DampingForce) * DeltaTime;
		V.CurrentPosition += V.Velocity * DeltaTime;

		FVector Deform = V.CurrentPosition - V.OriginalPosition;
		if (Deform.Size() > MaxDeformation)
		{
			V.CurrentPosition = V.OriginalPosition + Deform.GetSafeNormal() * MaxDeformation;
			V.Velocity *= 0.5f;
		}

		if (V.Velocity.SizeSquared() > 0.01f)
		{
			bMeshChanged = true;
		}

		Positions.Add(V.CurrentPosition);

		float HeightRatio = FMath::Clamp(V.CurrentPosition.Z / (SlimeRadius * 0.85f), 0.0f, 1.0f);
		Colors.Add(FLinearColor(HeightRatio * 0.3f + 0.7f, HeightRatio * 0.3f + 0.7f, HeightRatio * 0.3f + 0.7f, 1.0f));
	}

	if (bMeshChanged || bIsMegaMorphing)
	{
		RecalculateNormals();

		for (const FSlimeVertex& V : Vertices)
		{
			Normals.Add(V.Normal);
		}

		TArray<FProcMeshTangent> Tangents;
		SlimeMesh->UpdateMeshSection_LinearColor(0, Positions, Normals, UVs, Colors, Tangents);
	}
}

void ASlimeActor::RecalculateNormals()
{
	for (FSlimeVertex& V : Vertices)
	{
		V.Normal = FVector::ZeroVector;
	}

	for (int32 i = 0; i < Triangles.Num(); i += 3)
	{
		int32 I0 = Triangles[i];
		int32 I1 = Triangles[i + 1];
		int32 I2 = Triangles[i + 2];

		FVector Edge1 = Vertices[I1].CurrentPosition - Vertices[I0].CurrentPosition;
		FVector Edge2 = Vertices[I2].CurrentPosition - Vertices[I0].CurrentPosition;
		FVector FaceNormal = FVector::CrossProduct(Edge1, Edge2).GetSafeNormal();

		Vertices[I0].Normal += FaceNormal;
		Vertices[I1].Normal += FaceNormal;
		Vertices[I2].Normal += FaceNormal;
	}

	for (FSlimeVertex& V : Vertices)
	{
		V.Normal = V.Normal.GetSafeNormal();
	}
}

// --- Interactions ---

void ASlimeActor::Poke(FVector WorldHitPoint)
{
	FVector LocalHit = GetActorTransform().InverseTransformPosition(WorldHitPoint);

	for (FSlimeVertex& V : Vertices)
	{
		FVector ToVertex = V.OriginalPosition - LocalHit;
		float Distance = ToVertex.Size();
		float Influence = FMath::Exp(-(Distance * Distance) / (2.0f * DeformFalloff * DeformFalloff));

		if (Influence > 0.01f)
		{
			FVector PushDir = ToVertex.GetSafeNormal();
			V.Velocity += PushDir * 400.0f * Influence;
		}
	}

	ProcessCombo();
	AddEnergy(10.0f + GetComboBonus());
	OnSlimeInteraction.Broadcast(ESlimeInteraction::Poke);
}

void ASlimeActor::Squish()
{
	for (FSlimeVertex& V : Vertices)
	{
		float HeightFactor = FMath::Max(0.0f, V.OriginalPosition.Z / SlimeRadius);
		V.Velocity.Z -= 350.0f * HeightFactor;
		FVector Outward = FVector(V.OriginalPosition.X, V.OriginalPosition.Y, 0.0f).GetSafeNormal();
		V.Velocity += Outward * 200.0f * HeightFactor;
	}

	ProcessCombo();
	AddEnergy(12.0f + GetComboBonus());
	OnSlimeInteraction.Broadcast(ESlimeInteraction::Squish);
}

void ASlimeActor::Stretch()
{
	for (FSlimeVertex& V : Vertices)
	{
		float HeightFactor = FMath::Max(0.0f, V.OriginalPosition.Z / SlimeRadius);
		V.Velocity.Z += 400.0f * HeightFactor;
		FVector Inward = -FVector(V.OriginalPosition.X, V.OriginalPosition.Y, 0.0f).GetSafeNormal();
		V.Velocity += Inward * 150.0f * (1.0f - HeightFactor);
	}

	ProcessCombo();
	AddEnergy(11.0f + GetComboBonus());
	OnSlimeInteraction.Broadcast(ESlimeInteraction::Stretch);
}

void ASlimeActor::Bounce()
{
	for (FSlimeVertex& V : Vertices)
	{
		V.Velocity.Z += 500.0f;
		V.Velocity.X += FMath::RandRange(-50.0f, 50.0f);
		V.Velocity.Y += FMath::RandRange(-50.0f, 50.0f);
	}

	ProcessCombo();
	AddEnergy(13.0f + GetComboBonus());
	OnSlimeInteraction.Broadcast(ESlimeInteraction::Bounce);
}

void ASlimeActor::DragDeform(FVector WorldHitPoint, FVector DragDelta)
{
	FVector LocalHit = GetActorTransform().InverseTransformPosition(WorldHitPoint);
	FVector LocalDelta = GetActorTransform().InverseTransformVector(DragDelta);

	for (FSlimeVertex& V : Vertices)
	{
		float Distance = (V.OriginalPosition - LocalHit).Size();
		float Influence = FMath::Exp(-(Distance * Distance) / (2.0f * DeformFalloff * DeformFalloff));

		if (Influence > 0.01f)
		{
			V.Velocity += LocalDelta * 3.0f * Influence;
		}
	}

	ProcessCombo();
	AddEnergy(5.0f + GetComboBonus());
	OnSlimeInteraction.Broadcast(ESlimeInteraction::Drag);
}

void ASlimeActor::TriggerMegaMorph()
{
	if (GameState.Energy < 100.0f || bIsMegaMorphing) return;

	bIsMegaMorphing = true;
	MegaMorphTimer = 0.0f;

	for (FSlimeVertex& V : Vertices)
	{
		FVector Outward = (V.OriginalPosition - FVector::ZeroVector).GetSafeNormal();
		V.Velocity += Outward * 800.0f;
		V.Velocity.Z += FMath::RandRange(200.0f, 600.0f);
	}

	GameState.Energy = 24.0f;
	GameState.Coins += 5;

	OnMegaMorphTriggered.Broadcast();
	OnSlimeInteraction.Broadcast(ESlimeInteraction::MegaMorph);
}

// --- Customization ---

void ASlimeActor::SetSlimeColor(FLinearColor NewColor)
{
	Customization.BaseColor = NewColor;

	if (SlimeDynamicMaterial)
	{
		// Try the standard parameter names
		SlimeDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), NewColor);
		SlimeDynamicMaterial->SetVectorParameterValue(TEXT("Color"), NewColor);

		FLinearColor EmissiveColor = NewColor * 0.15f;
		SlimeDynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
	}

	if (InnerGlowDynamicMaterial)
	{
		FLinearColor GlowColor = NewColor * 0.6f;
		GlowColor.A = 0.4f;
		InnerGlowDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), GlowColor);
		InnerGlowDynamicMaterial->SetVectorParameterValue(TEXT("Color"), GlowColor);
	}
}

void ASlimeActor::SetEyeStyle(ESlimeEyeStyle NewStyle)
{
	Customization.EyeStyle = NewStyle;

	switch (NewStyle)
	{
	case ESlimeEyeStyle::Cyclops:
		LeftEye->SetRelativeLocation(FVector(75.0f, 0.0f, 40.0f));
		LeftEye->SetRelativeScale3D(FVector(0.35f));
		LeftPupil->SetRelativeScale3D(FVector(0.5f));
		RightEye->SetVisibility(false);
		RightPupil->SetVisibility(false);
		break;

	case ESlimeEyeStyle::Googly:
		LeftEye->SetRelativeLocation(FVector(70.0f, -22.0f, 35.0f));
		RightEye->SetRelativeLocation(FVector(70.0f, 22.0f, 35.0f));
		LeftEye->SetRelativeScale3D(FVector(0.3f));
		RightEye->SetRelativeScale3D(FVector(0.3f));
		LeftPupil->SetRelativeScale3D(FVector(0.55f));
		RightPupil->SetRelativeScale3D(FVector(0.55f));
		LeftEye->SetRelativeRotation(FRotator::ZeroRotator);
		RightEye->SetRelativeRotation(FRotator::ZeroRotator);
		RightEye->SetVisibility(true);
		RightPupil->SetVisibility(true);
		break;

	case ESlimeEyeStyle::Alien:
		LeftEye->SetRelativeLocation(FVector(65.0f, -28.0f, 38.0f));
		LeftEye->SetRelativeScale3D(FVector(0.18f, 0.28f, 0.18f));
		RightEye->SetRelativeLocation(FVector(65.0f, 28.0f, 38.0f));
		RightEye->SetRelativeScale3D(FVector(0.18f, 0.28f, 0.18f));
		LeftEye->SetRelativeRotation(FRotator::ZeroRotator);
		RightEye->SetRelativeRotation(FRotator::ZeroRotator);
		RightEye->SetVisibility(true);
		RightPupil->SetVisibility(true);
		if (EyeDynamicMaterial)
		{
			// Green tint for alien eyes
			EyeDynamicMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.6f, 1.0f, 0.4f));
		}
		break;

	case ESlimeEyeStyle::HeartEyes:
		LeftEye->SetRelativeLocation(FVector(70.0f, -22.0f, 35.0f));
		RightEye->SetRelativeLocation(FVector(70.0f, 22.0f, 35.0f));
		LeftEye->SetRelativeScale3D(FVector(0.22f));
		RightEye->SetRelativeScale3D(FVector(0.22f));
		LeftEye->SetRelativeRotation(FRotator::ZeroRotator);
		RightEye->SetRelativeRotation(FRotator::ZeroRotator);
		RightEye->SetVisibility(true);
		RightPupil->SetVisibility(true);
		if (PupilDynamicMaterial)
		{
			PupilDynamicMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.9f, 0.1f, 0.2f));
		}
		break;

	case ESlimeEyeStyle::Sleepy:
		LeftEye->SetRelativeLocation(FVector(70.0f, -22.0f, 32.0f));
		RightEye->SetRelativeLocation(FVector(70.0f, 22.0f, 32.0f));
		LeftEye->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.06f));
		RightEye->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.06f));
		LeftEye->SetRelativeRotation(FRotator::ZeroRotator);
		RightEye->SetRelativeRotation(FRotator::ZeroRotator);
		RightEye->SetVisibility(true);
		RightPupil->SetVisibility(false);
		LeftPupil->SetVisibility(false);
		break;

	case ESlimeEyeStyle::Angry:
		LeftEye->SetRelativeLocation(FVector(70.0f, -22.0f, 37.0f));
		RightEye->SetRelativeLocation(FVector(70.0f, 22.0f, 37.0f));
		LeftEye->SetRelativeScale3D(FVector(0.22f));
		RightEye->SetRelativeScale3D(FVector(0.22f));
		LeftEye->SetRelativeRotation(FRotator(0.0f, 0.0f, -20.0f));
		RightEye->SetRelativeRotation(FRotator(0.0f, 0.0f, 20.0f));
		RightEye->SetVisibility(true);
		RightPupil->SetVisibility(true);
		LeftPupil->SetVisibility(true);
		break;

	case ESlimeEyeStyle::XEyes:
		LeftEye->SetRelativeLocation(FVector(70.0f, -22.0f, 35.0f));
		RightEye->SetRelativeLocation(FVector(70.0f, 22.0f, 35.0f));
		LeftEye->SetRelativeScale3D(FVector(0.22f));
		RightEye->SetRelativeScale3D(FVector(0.22f));
		LeftEye->SetRelativeRotation(FRotator(0.0f, 0.0f, 45.0f));
		RightEye->SetRelativeRotation(FRotator(0.0f, 0.0f, 45.0f));
		RightEye->SetVisibility(true);
		RightPupil->SetVisibility(true);
		LeftPupil->SetVisibility(true);
		break;

	default: // Normal, StarEyes, Dizzy
		LeftEye->SetRelativeLocation(FVector(70.0f, -22.0f, 35.0f));
		RightEye->SetRelativeLocation(FVector(70.0f, 22.0f, 35.0f));
		LeftEye->SetRelativeScale3D(FVector(0.22f));
		RightEye->SetRelativeScale3D(FVector(0.22f));
		LeftEye->SetRelativeRotation(FRotator::ZeroRotator);
		RightEye->SetRelativeRotation(FRotator::ZeroRotator);
		LeftPupil->SetRelativeScale3D(FVector(0.45f));
		RightPupil->SetRelativeScale3D(FVector(0.45f));
		RightEye->SetVisibility(true);
		RightPupil->SetVisibility(true);
		LeftPupil->SetVisibility(true);
		// Reset eye/pupil colors to default
		if (EyeDynamicMaterial)
		{
			EyeDynamicMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::White);
		}
		if (PupilDynamicMaterial)
		{
			PupilDynamicMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.02f, 0.05f));
		}
		break;
	}
}

void ASlimeActor::ApplyCustomization(const FSlimeCustomization& NewCustomization)
{
	Customization = NewCustomization;
	SetSlimeColor(NewCustomization.BaseColor);
	SetEyeStyle(NewCustomization.EyeStyle);
}

// --- Internal ---

void ASlimeActor::AddEnergy(float Amount)
{
	GameState.Energy = FMath::Clamp(GameState.Energy + Amount, 0.0f, 100.0f);
	GameState.TimeSinceLastAction = 0.0f;
}

void ASlimeActor::ProcessCombo()
{
	if (GameState.TimeSinceLastAction <= ComboWindowSeconds)
	{
		GameState.ComboCount++;
	}
	else
	{
		GameState.ComboCount = 1;
	}
}

float ASlimeActor::GetComboBonus() const
{
	return FMath::Min((float)(GameState.ComboCount / 3), 12.0f);
}

void ASlimeActor::UpdateMood()
{
	ESlimeMood NewMood = GameState.GetMoodFromEnergy();
	if (NewMood != GameState.CurrentMood)
	{
		GameState.CurrentMood = NewMood;
		OnMoodChanged.Broadcast(NewMood);

		if (SlimeDynamicMaterial)
		{
			float EmissiveStrength = 0.0f;
			switch (NewMood)
			{
			case ESlimeMood::Chill:     EmissiveStrength = 0.1f; break;
			case ESlimeMood::Happy:     EmissiveStrength = 0.2f; break;
			case ESlimeMood::Playful:   EmissiveStrength = 0.35f; break;
			case ESlimeMood::Hyper:     EmissiveStrength = 0.5f; break;
			case ESlimeMood::Legendary: EmissiveStrength = 1.0f; break;
			}
			SlimeDynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), EmissiveStrength);
		}
	}
}

void ASlimeActor::UpdateEyes(float DeltaTime)
{
	BlinkTimer += DeltaTime;
	if (!bIsBlinking && BlinkTimer >= BlinkInterval)
	{
		bIsBlinking = true;
		BlinkTimer = 0.0f;
		BlinkInterval = FMath::RandRange(2.5f, 5.0f);
	}

	if (bIsBlinking)
	{
		float BlinkDuration = 0.15f;
		float BlinkProgress = BlinkTimer / BlinkDuration;

		if (BlinkProgress < 0.5f)
		{
			// Close eyes
			float Squish = 1.0f - BlinkProgress * 2.0f;
			FVector Scale = LeftEye->GetRelativeScale3D();
			Scale.Z = FMath::Max(0.02f, LeftEye->GetRelativeScale3D().X * Squish);
			LeftEye->SetRelativeScale3D(FVector(Scale.X, Scale.Y, Scale.Z));
			if (RightEye->IsVisible())
			{
				Scale = RightEye->GetRelativeScale3D();
				Scale.Z = FMath::Max(0.02f, RightEye->GetRelativeScale3D().X * Squish);
				RightEye->SetRelativeScale3D(FVector(Scale.X, Scale.Y, Scale.Z));
			}
		}
		else if (BlinkProgress >= 1.0f)
		{
			bIsBlinking = false;
			// Restore eye style scales
			SetEyeStyle(Customization.EyeStyle);
		}
	}

	// Googly eye bounce
	if (Customization.EyeStyle == ESlimeEyeStyle::Googly)
	{
		float Time = GetGameTimeSinceCreation();
		float BounceL = FMath::Sin(Time * 3.0f) * 8.0f;
		float BounceR = FMath::Sin(Time * 3.0f + 1.2f) * 8.0f;
		LeftPupil->SetRelativeLocation(FVector(45.0f, FMath::Sin(Time * 2.0f) * 5.0f, BounceL));
		RightPupil->SetRelativeLocation(FVector(45.0f, FMath::Sin(Time * 2.5f) * 5.0f, BounceR));
	}
}

void ASlimeActor::ApplyIdleAnimation(float DeltaTime)
{
	IdleWobbleTime += DeltaTime;

	float WobbleAmount = 3.0f;
	switch (GameState.CurrentMood)
	{
	case ESlimeMood::Happy:     WobbleAmount = 4.5f; break;
	case ESlimeMood::Playful:   WobbleAmount = 6.0f; break;
	case ESlimeMood::Hyper:     WobbleAmount = 8.0f; break;
	case ESlimeMood::Legendary: WobbleAmount = 12.0f; break;
	default: break;
	}

	float WobbleSpeed = 2.0f;
	if (GameState.CurrentMood >= ESlimeMood::Hyper) WobbleSpeed = 3.5f;

	for (FSlimeVertex& V : Vertices)
	{
		float HeightRatio = FMath::Max(0.0f, V.OriginalPosition.Z / SlimeRadius);
		float Wobble = FMath::Sin(IdleWobbleTime * WobbleSpeed + V.OriginalPosition.X * 0.02f) * WobbleAmount * HeightRatio;
		V.Velocity.Z += Wobble * DeltaTime * 10.0f;
	}
}

void ASlimeActor::ApplyMegaMorphAnimation(float DeltaTime)
{
	MegaMorphTimer += DeltaTime;

	if (MegaMorphTimer >= MegaMorphDuration)
	{
		bIsMegaMorphing = false;
		return;
	}

	float Progress = MegaMorphTimer / MegaMorphDuration;

	if (Progress >= 0.3f && Progress < 0.7f)
	{
		float SpinSpeed = 8.0f;
		for (FSlimeVertex& V : Vertices)
		{
			FVector ToCenter = -V.CurrentPosition;
			FVector Tangent = FVector::CrossProduct(ToCenter, FVector::UpVector).GetSafeNormal();
			V.Velocity += Tangent * SpinSpeed;
		}
	}
	else if (Progress >= 0.7f)
	{
		float SnapProgress = (Progress - 0.7f) / 0.3f;
		float SnapForce = FMath::InterpEaseOut(0.0f, 1.0f, SnapProgress, 3.0f);

		for (FSlimeVertex& V : Vertices)
		{
			FVector ToOriginal = V.OriginalPosition - V.CurrentPosition;
			V.Velocity += ToOriginal * 20.0f * SnapForce;
		}
	}

	if (SlimeDynamicMaterial)
	{
		float Pulse = FMath::Sin(MegaMorphTimer * 12.0f) * 0.5f + 1.5f;
		SlimeDynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), Pulse);
	}
}
