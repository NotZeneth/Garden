// Fill out your copyright notice in the Description page of Project Settings.

#include "RadiusSpawner.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h" 
#include "Engine/World.h" 
#include "Kismet/GameplayStatics.h" 
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h" 
#include "DrawDebugHelpers.h" 

// Define the tag we will use to filter hits on the terrain
const FName GroundTag("Ground");

// Define the Material Parameter Names
const FName DecalRadiusParameterName("Radius"); // Must match the Scalar Parameter name in your Decal Material
const FName DecalSharpnessParameterName("Sharpness"); // Must match the Scalar Parameter name in your Decal Material

// Sets default values
ARadiusSpawner::ARadiusSpawner()
{
 	// Set this actor to call Tick() every frame. 
	PrimaryActorTick.bCanEverTick = true; // Enabled Tick

    // --- DECAL INITIALIZATION (FOR VISUALIZATION) ---
    RadiusDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("RadiusDecal"));
    
    // Decals are projected along their X-axis, so we rotate it to point downward onto the ground.
    RadiusDecalComponent->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f)); 
    RadiusDecalComponent->SetupAttachment(RootComponent);
    RadiusDecalComponent->SetVisibility(true);

    // Default dimensions (size will be set in Tick based on SpawningRadius)
    RadiusDecalComponent->DecalSize = FVector(200.0f, 200.0f, 200.0f);
}

// Called when the game starts or when spawned
void ARadiusSpawner::BeginPlay()
{
	Super::BeginPlay();

    // --- Create Dynamic Material Instance for the Decal ---
    if (DecalMaterial)
    {
        DecalMaterialInstance = UMaterialInstanceDynamic::Create(DecalMaterial, this);
        if (DecalMaterialInstance)
        {
            RadiusDecalComponent->SetDecalMaterial(DecalMaterialInstance);
        }
    }
}

// Implement Tick to constantly update the visualization
void ARadiusSpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 1. Calculate the center based on camera focus (updates GroundNormal)
    FVector DynamicSpawnCenter = CalculateGroundCenterLocation();
    
    // 2. Position the decal and align its rotation with the ground normal
    // We add a small offset (5.0f) to ensure the decal is rendered just above the hit surface.
    RadiusDecalComponent->SetWorldLocation(DynamicSpawnCenter + FVector(0.0f, 0.0f, 5.0f)); 
    RadiusDecalComponent->SetWorldRotation(GroundNormal.Rotation()); 
    
    // 3. Set the decal size based on the SpawningRadius
    RadiusDecalComponent->DecalSize = FVector(SpawningRadius * 0.5f, SpawningRadius, SpawningRadius);
    
    // --- Update Material Parameters in real-time ---
    if (DecalMaterialInstance)
    {
        DecalMaterialInstance->SetScalarParameterValue(DecalRadiusParameterName, DecalRadiusScale);
        DecalMaterialInstance->SetScalarParameterValue(DecalSharpnessParameterName, DecalSharpness);
    }
}


// Helper function containing the camera-projected trace logic
FVector ARadiusSpawner::CalculateGroundCenterLocation()
{
    // Default to the spawner's location if trace fails
    FVector CenterResult = GetActorLocation(); 
    GroundNormal = FVector::UpVector; // Reset normal to flat up vector

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

    if (PlayerController && PlayerController->PlayerCameraManager && GetWorld())
    {
        FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
        FVector CameraForwardVector = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();

        FHitResult CenterHitResult;
        
        const float CenterTraceDistance = 10000.0f; 
        const FVector TraceStart = CameraLocation;
        const FVector TraceEnd = CameraLocation + (CameraForwardVector * CenterTraceDistance);

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);

        bool bHitCenter = GetWorld()->LineTraceSingleByChannel(
            CenterHitResult,
            TraceStart,
            TraceEnd,
            ECollisionChannel::ECC_Visibility, 
            QueryParams
        );

        // Filter by Tag and capture normal
        if (bHitCenter && CenterHitResult.GetActor() && CenterHitResult.GetActor()->ActorHasTag(GroundTag))
        {
            CenterResult = CenterHitResult.Location;
            // Capture the ground normal for decal alignment
            GroundNormal = CenterHitResult.ImpactNormal; 
        }
    }
    
    return CenterResult;
}

// --- MESH CONTROL METHODS ---

void ARadiusSpawner::SetMeshSetIndex(int32 NewIndex)
{
    if (MeshSets.Num() == 0)
    {
        CurrentMeshSetIndex = 0;
        UE_LOG(LogTemp, Warning, TEXT("ARadiusSpawner: MeshSets array is empty. Index remains 0."));
        return;
    }

    CurrentMeshSetIndex = FMath::Clamp(NewIndex, 0, MeshSets.Num() - 1);
    
    if (CurrentMeshSetIndex != NewIndex)
    {
        UE_LOG(LogTemp, Warning, TEXT("ARadiusSpawner: Requested index %d is out of bounds. Clamped to %d."), NewIndex, CurrentMeshSetIndex);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ARadiusSpawner: Active Mesh Set Index changed to %d."), CurrentMeshSetIndex);
    }
}

void ARadiusSpawner::CycleMeshSetIndex()
{
    if (MeshSets.Num() == 0)
    {
        CurrentMeshSetIndex = 0;
        UE_LOG(LogTemp, Warning, TEXT("ARadiusSpawner: MeshSets array is empty. Cannot cycle."));
        return;
    }

    CurrentMeshSetIndex = (CurrentMeshSetIndex + 1) % MeshSets.Num();
    
    UE_LOG(LogTemp, Log, TEXT("ARadiusSpawner: Cycled to next Mesh Set. New Index: %d."), CurrentMeshSetIndex);
}

// ---------------------------------


void ARadiusSpawner::SpawnMeshesInRadius(bool bDestroyExisting)
{
	// 1. Destroy existing meshes if requested
	if (bDestroyExisting)
	{
		for (UStaticMeshComponent* Mesh : SpawnedMeshes)
		{
			if (Mesh)
			{
				Mesh->DestroyComponent();
			}
		}
		SpawnedMeshes.Empty();
	}

    // --- Retrieve the Active Mesh Set ---
    if (MeshSets.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ARadiusSpawner: MeshSets array is empty. Aborting spawn."));
        return;
    }

    const int32 SafeIndex = FMath::Clamp(CurrentMeshSetIndex, 0, MeshSets.Num() - 1);
    const TArray<UStaticMesh*>& ActiveMeshSet = MeshSets[SafeIndex].Meshes; 
    
	if (ActiveMeshSet.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ARadiusSpawner: Active Mesh Set (Index %d) is empty. Aborting spawn."), SafeIndex);
		return;
	}
    // --------------------------------------------------------------------------

    // --- Calculate Actual Spawn Count (NEW) ---
    const int32 ActualSpawnCount = FMath::RandRange(MinSpawnCount, MaxSpawnCount);
    if (ActualSpawnCount <= 0)
    {
        UE_LOG(LogTemp, Log, TEXT("ARadiusSpawner: Actual Spawn Count is 0. Aborting spawn."));
        return;
    }
    
    // --- Get Dynamic Spawn Center and Camera Info ---
    const FVector GroundCenterLocation = CalculateGroundCenterLocation();
    
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    FVector CameraLocation = FVector::ZeroVector;
    if (PlayerController && PlayerController->PlayerCameraManager)
    {
        CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
    }
    // --------------------------------------------------------------------------


	// 2. Loop and spawn the calculated number of meshes
	for (int32 i = 0; i < ActualSpawnCount; ++i)
	{
		// --- Mesh Selection Logic (uses ActiveMeshSet) ---
		const int32 RandomMeshIndex = FMath::RandRange(0, ActiveMeshSet.Num() - 1);
		UStaticMesh* SelectedMesh = ActiveMeshSet[RandomMeshIndex];
		if (!SelectedMesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("ARadiusSpawner: Mesh in Active Set at index %d is null. Skipping spawn for this iteration."), RandomMeshIndex);
			continue;
		}
        // --------------------------------------------------

		// --- 2D Random Position Generation Relative to the GroundCenterLocation ---
		
		const float RandomAngle = FMath::RandRange(0.0f, 360.0f);
		const float RandomDistance = FMath::FRand() * SpawningRadius;
		const float AngleInRadians = FMath::DegreesToRadians(RandomAngle);
		
		const float RandX = FMath::Cos(AngleInRadians) * RandomDistance;
		const float RandY = FMath::Sin(AngleInRadians) * RandomDistance;

		// Calculate the base horizontal location for the trace
		const FVector BaseTraceLocation(
            GroundCenterLocation.X + RandX, 
            GroundCenterLocation.Y + RandY, 
            GroundCenterLocation.Z + 1000.0f // Safe Z to start the trace high
        );

		// --- Line Trace to Find Ground Z (Projection) for the RANDOMIZED spot ---
		FHitResult HitResult;
		
		const float TraceDownOffset = 200.0f; 
		const FVector TraceStart = BaseTraceLocation; 
		const FVector TraceEnd = BaseTraceLocation - FVector(0.0f, 0.0f, 1000.0f + TraceDownOffset); 

		FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
        
		bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			ECollisionChannel::ECC_Visibility, 
			QueryParams
		);

		// --- Check for valid hit on Taggable Ground ---
		if (bHit && HitResult.GetActor() && HitResult.GetActor()->ActorHasTag(GroundTag))
		{
			// Add a small offset (2.0f) to ensure the mesh pivot is just above the ground surface.
			const float FinalZ = HitResult.Location.Z + 2.0f;
            
            // Calculate the final spawn location (Random X/Y, TRACED Z)
            const FVector SpawnLocation(GroundCenterLocation.X + RandX, GroundCenterLocation.Y + RandY, FinalZ);

            // --- Billboarding Z-Rotation Logic ---
            FRotator SpawnRotation = FRotator::ZeroRotator;

            if (PlayerController)
            {
                FVector LookAtVector = CameraLocation - SpawnLocation;
                LookAtVector.Z = 0.0f; 
                
                const FRotator LookAtRotator = LookAtVector.Rotation();
                // Apply only the Yaw (Z rotation), keeping Pitch and Roll at 0 (upright)
                SpawnRotation = FRotator(0.0f, LookAtRotator.Yaw, 0.0f); 
            }
            // ------------------------------------
            
            // --- Scale Randomization Logic ---
            const float UniformScale = FMath::FRandRange(MinScale, MaxScale);
            const FVector SpawnScale(UniformScale, UniformScale, UniformScale);
            // ------------------------------------

            // 3. Create the Static Mesh Component
            UStaticMeshComponent* NewMeshComp = NewObject<UStaticMeshComponent>(this);
            if (NewMeshComp)
            {
                NewMeshComp->RegisterComponent();

                NewMeshComp->SetStaticMesh(SelectedMesh);
                
                // --- CREATE AND APPLY DYNAMIC MATERIAL INSTANCE (MID) ---
                if (NewMeshesMaterial)
                {
                    UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(NewMeshesMaterial, NewMeshComp);
                    if (DynamicMaterial)
                    {
                        NewMeshComp->SetMaterial(0, DynamicMaterial);
                    }
                }
                // -------------------------------------------------------------

                NewMeshComp->SetWorldLocationAndRotation(SpawnLocation, SpawnRotation);
                
                NewMeshComp->SetRelativeScale3D(SpawnScale);
                
                NewMeshComp->SetSimulatePhysics(false); 
                NewMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                
                // 4. Store the reference for Blueprint access and cleanup
                SpawnedMeshes.Add(NewMeshComp);
            }
		}
        // If the trace failed, the loop continues to the next iteration (i++), skipping the spawn.
	}

	UE_LOG(LogTemp, Log, TEXT("ARadiusSpawner: Successfully spawned %d meshes."), ActualSpawnCount);
}