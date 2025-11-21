// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"

static FQuat MirrorPitch(const FQuat& Q)
{
    return FQuat(
        -Q.X,
        Q.Y,
        Q.Z,
        -Q.W 
    );
}

FQuat UMyBlueprintFunctionLibrary::MirrorPitch(const FQuat& Q)
{
    return FQuat(
        Q.X,
        -Q.Y,
        Q.Z,
        -Q.W
    );
}


void UMyBlueprintFunctionLibrary::SetActorWorldRotationQuat(
    AActor* Actor,
    const FQuat& Quaternion,
    bool bInvertPitch,
    bool bSweep,
    bool bTeleport
)
{
    if (!Actor || !Actor->GetRootComponent())
        return;

    FHitResult SweepResult;
    FQuat FinalQuat = Quaternion;

    if (bInvertPitch)
    {
        FinalQuat = MirrorPitch(Quaternion);
    }

    Actor->GetRootComponent()->SetWorldRotation(
        FinalQuat,
        bSweep,
        (bSweep ? &SweepResult : nullptr),
        bTeleport ? ETeleportType::TeleportPhysics : ETeleportType::None
    );
}

FQuat UMyBlueprintFunctionLibrary::GetActorWorldRotationQuat(AActor* Actor)
{
    if (!Actor || !Actor->GetRootComponent())
        return FQuat::Identity;

    return Actor->GetActorQuat();
}


