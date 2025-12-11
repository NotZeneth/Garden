// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RadiusSpawner.generated.h"

// --- USTRUCT TO FIX UHT NESTED ARRAY ERROR ---
USTRUCT(BlueprintType)
struct FMeshSet
{
	GENERATED_BODY() 

	/** The array of meshes belonging to this specific set (e.g., 'trees', 'rocks', etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Set")
	TArray<UStaticMesh*> Meshes;
};

/**
 * An Actor class designed to spawn multiple copies of a specified Static Mesh
 * within a defined spherical radius centered on the Actor's location.
 */
UCLASS(Blueprintable, BlueprintType)
class GARDEN_API ARadiusSpawner : public AActor
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
	 * @brief The material used to project the cursor visualization circle onto the ground (must be a decal material).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning Settings")
	UMaterialInterface* DecalMaterial;

    /**
	 * @brief The material used to create a Material Instance Dynamic (MID) for each spawned mesh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning Settings")
	UMaterialInterface* NewMeshesMaterial;

    // --- NEW DECAL VISUALIZATION PROPERTIES ---

    /**
	 * @brief Material parameter controlling the outer edge of the decal circle mask (usually 1.0 to 5.0).
	 * Must match the Scalar Parameter name in the Decal Material (e.g., 'Radius').
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal Appearance")
	float DecalRadiusScale = 2.0f;

    /**
	 * @brief Material parameter controlling the sharpness/falloff of the decal mask (e.g., 5.0 to 50.0).
	 * Must match the Scalar Parameter name in the Decal Material (e.g., 'Sharpness').
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal Appearance")
	float DecalSharpness = 10.0f;
    
    // ------------------------------------------

	/**
	 * @brief Array of Mesh Sets. Each set (FMeshSet) contains an inner array of meshes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Sets")
	TArray<FMeshSet> MeshSets;

    /**
	 * @brief Index of the Mesh Set currently active for spawning.
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

    /**
	 * @brief Stores references to all spawned Static Mesh Components.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Spawning Output")
	TArray<UStaticMeshComponent*> SpawnedMeshes;

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

    /** Private pointer to the Dynamic Material Instance for runtime Decal updates. */
    UPROPERTY()
    UMaterialInstanceDynamic* DecalMaterialInstance; // NEW: Pointer to the Decal's MID

    /** Stores the normal (direction) of the surface hit by the camera trace. */
    FVector GroundNormal = FVector::UpVector; 

    /** Helper function to calculate the camera-projected ground center. */
    FVector CalculateGroundCenterLocation();
};