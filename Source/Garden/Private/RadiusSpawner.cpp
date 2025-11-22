// Fill out your copyright notice in the Description page of Project Settings.

#include "RadiusSpawner.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h" 
#include "Engine/World.h" 
#include "Kismet/GameplayStatics.h" 
#include "Components/DecalComponent.h"
#include "DrawDebugHelpers.h" 

// Define the tag we will use to filter hits on the terrain
const FName GroundTag("Ground");

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

    // Set the Decal Material here once, if a default is provided in the Blueprint
    if (DecalMaterial)
    {
        RadiusDecalComponent->SetDecalMaterial(DecalMaterial);
    }
}

// Implement Tick to constantly update the visualization
void ARadiusSpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 1. Calculate the center based on camera focus
    DynamicSpawnCenter = CalculateGroundCenterLocation();
    
    // 2. Position the decal at the ground center
    // We add a small offset (5.0f) to ensure the decal is rendered just above the hit surface.
    RadiusDecalComponent->SetWorldLocation(DynamicSpawnCenter + FVector(0.0f, 0.0f, 5.0f)); 
    
    // 3. Set the decal size based on the SpawningRadius
    // DecalSize.X is projection depth (keep constant). Y and Z are the radius size.
    RadiusDecalComponent->DecalSize = FVector(SpawningRadius * 0.5f, SpawningRadius, SpawningRadius);
}


// Helper function containing the camera-projected trace logic
FVector ARadiusSpawner::CalculateGroundCenterLocation()
{
    // Default to the spawner's location if trace fails
    FVector CenterResult = GetActorLocation(); 

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

    if (PlayerController && PlayerController->PlayerCameraManager && GetWorld())
    {
        FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
        FVector CameraForwardVector = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();

        FHitResult CenterHitResult;
        
        // Define a long trace distance forward from the camera
        const float CenterTraceDistance = 10000.0f; 
        const FVector TraceStart = CameraLocation;
        const FVector TraceEnd = CameraLocation + (CameraForwardVector * CenterTraceDistance);

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);

        // Perform the line trace forward from the camera
        bool bHitCenter = GetWorld()->LineTraceSingleByChannel(
            CenterHitResult,
            TraceStart,
            TraceEnd,
            ECollisionChannel::ECC_Visibility, 
            QueryParams
        );

        // Filter by Tag: The new center must hit the tagged ground
        if (bHitCenter && CenterHitResult.GetActor() && CenterHitResult.GetActor()->ActorHasTag(GroundTag))
        {
            // The new center of the spawn radius is the location where the camera's trace hit the ground
            CenterResult = CenterHitResult.Location;
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

    // Clamp the index to the valid range [0, MeshSets.Num() - 1]
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

    // Increment index and use the modulo operator (%) to handle wrap-around back to 0
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

    // Safely get the current set, ensuring the index is within bounds
    const int32 SafeIndex = FMath::Clamp(CurrentMeshSetIndex, 0, MeshSets.Num() - 1);
    // Access the inner TArray<UStaticMesh*> via the .Meshes member of the FMeshSet struct
    const TArray<UStaticMesh*>& ActiveMeshSet = MeshSets[SafeIndex].Meshes; 
    
    // Safety check for the inner array
	if (ActiveMeshSet.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ARadiusSpawner: Active Mesh Set (Index %d) is empty. Aborting spawn."), SafeIndex);
		return;
	}
    // --------------------------------------------------------------------------


    // --- Get Dynamic Spawn Center and Camera Info ---
    // Calculate the center point for spawning (use the location calculated in Tick)
    const FVector GroundCenterLocation = CalculateGroundCenterLocation();
    
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    FVector CameraLocation = FVector::ZeroVector;
    if (PlayerController && PlayerController->PlayerCameraManager)
    {
        CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
    }
    // --------------------------------------------------------------------------


	// 2. Loop and spawn the requested number of meshes
	for (int32 i = 0; i < SpawnCount; ++i)
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
		// We use a fixed high Z value (1000.0f) for the vertical trace start.
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

		// --- FIX: Only proceed if the trace hits the Taggable Ground ---
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
                // FIX: Component is owned by 'this' but NOT attached to hierarchy, ensuring World Space position.
                NewMeshComp->RegisterComponent();

                // Set the randomly selected mesh asset
                NewMeshComp->SetStaticMesh(SelectedMesh);

                // Set its world transform
                NewMeshComp->SetWorldLocationAndRotation(SpawnLocation, SpawnRotation);
                
                // Set the random uniform scale
                NewMeshComp->SetRelativeScale3D(SpawnScale);
                
                NewMeshComp->SetSimulatePhysics(false); 
                NewMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                
                // 4. Store the reference for potential destruction later
                SpawnedMeshes.Add(NewMeshComp);
            }
		}
        // If the trace failed, the loop continues to the next iteration (i++), skipping the spawn.
	}

	UE_LOG(LogTemp, Log, TEXT("ARadiusSpawner: Successfully spawned %d meshes."), SpawnCount);
}