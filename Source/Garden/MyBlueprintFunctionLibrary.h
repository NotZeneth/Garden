// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class GARDEN_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
    // Set World Rotation avec Quaternion direct
    UFUNCTION(BlueprintCallable, Category = "Quaternion Rotation")
    static void SetActorWorldRotationQuat(
        AActor* Actor,
        const FQuat& Quaternion,
        bool bInvertPitch,
        bool bSweep,
        bool bTeleport
    );

    UFUNCTION(BlueprintPure, Category = "Quaternion Rotation")
    static FQuat MirrorPitch(const FQuat& Q);

    UFUNCTION(BlueprintPure, Category = "Quaternion")
    static FQuat GetActorWorldRotationQuat(AActor* Actor);

};