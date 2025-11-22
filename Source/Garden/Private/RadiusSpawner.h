// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "RadiusSpawner.generated.h"

// --- NEW USTRUCT TO FIX UHT NESTED ARRAY ERROR ---
// This struct wraps the inner array so UHT can serialize the TArray of TArray.
USTRUCT(BlueprintType)
struct FMeshSet
{
	GENERATED_BODY() // <-- This placement resolves the error C++ compiler requires
	// UHT requires this macro to be the absolute first member declared after the opening brace.

	/** The array of meshes belonging to this specific set (e.g., 'trees', 'rocks', etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Set")
	TArray<UStaticMesh*> Meshes;
};
// -----------------------------------------------------

/**
 * An Actor class designed to spawn multiple copies of a specified Static Mesh
 * within a defined spherical radius centered on the Actor's location.
 */
UCLASS(Blueprintable, BlueprintType)
class ARadiusSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARadiusSpawner();

    // Enable Tick to constantly update the visualization location
    virtual void Tick(float DeltaTime) override;

	/**
	 * @brief The radius around the Actor's location within which meshes will be spawned.
	 * This value is Blueprint Editable.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning Settings")
	float SpawningRadius = 500.0f;

    /**
	 * @brief Material used to project the visualization circle onto the ground (must be a decal material).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning Settings")
	UMaterialInterface* DecalMaterial;

	/**
	 * @brief Array of Mesh Sets. Each set (FMeshSet) contains an inner array of meshes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Sets")
	TArray<FMeshSet> MeshSets; // <-- Corrected type

    /**
	 * @brief Index of the Mesh Set currently active for spawning.
	 * Use SetMeshSetIndex or CycleMeshSetIndex to change this.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh Sets")
	int32 CurrentMeshSetIndex = 0;

	/**
	 * @brief The number of instances of the mesh to spawn when the function is called.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning Settings", meta = (ClampMin = "1"))
	int32 SpawnCount = 10;

	/**
	 * @brief Minimum uniform scale applied to the spawned mesh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning Settings", meta = (ClampMin = "0.01"))
	float MinScale = 0.8f;

	/**
	 * @brief Maximum uniform scale applied to the spawned mesh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning Settings", meta = (ClampMin = "0.01"))
	float MaxScale = 1.2f;

    // --- Control Methods ---

    /**
	 * @brief Sets the active mesh set index. Safely clamps the index to valid bounds.
	 * @param NewIndex The index of the mesh set to use.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mesh Control")
	void SetMeshSetIndex(int32 NewIndex);

    /**
	 * @brief Cycles to the next mesh set index. Resets to 0 if out of bounds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mesh Control")
	void CycleMeshSetIndex();

    // -----------------------

	/**
	 * @brief Spawns the specified meshes randomly within the SpawningRadius.
	 * @param bDestroyExisting If true, any meshes previously spawned by this function will be destroyed first.
	 * This method is Blueprint Callable.
	 */
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void SpawnMeshesInRadius(bool bDestroyExisting = false);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
    /** Component to visualize the spawning radius on the ground. */
    UPROPERTY(VisibleAnywhere, Category = "Visualization")
    UDecalComponent* RadiusDecalComponent;

    /** Stores the calculated center point for both spawning and visualization. */
    FVector DynamicSpawnCenter = FVector::ZeroVector;

    /** Helper function to calculate the camera-projected ground center. */
    FVector CalculateGroundCenterLocation();
    
	/**
	 * @brief Stores references to all spawned Static Mesh Components
	 * so they can be cleaned up later if requested.
	 */
	TArray<UStaticMeshComponent*> SpawnedMeshes;
};