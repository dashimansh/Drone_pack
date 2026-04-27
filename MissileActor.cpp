#include "MissileActor.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

AMissileActor::AMissileActor()
{
    PrimaryActorTick.bCanEverTick = true;

    MissileMesh = CreateDefaultSubobject
        <UStaticMeshComponent>(TEXT("MissileMesh"));
    RootComponent = MissileMesh;

    static ConstructorHelpers
        ::FObjectFinder<UStaticMesh> MeshAsset(
            TEXT("/Engine/BasicShapes/Cylinder"));
    if (MeshAsset.Succeeded())
    {
        MissileMesh->SetStaticMesh(
            MeshAsset.Object);
        MissileMesh->SetRelativeScale3D(
            FVector(0.1f, 0.1f, 0.3f));
        MissileMesh->SetRelativeRotation(
            FRotator(90.f, 0.f, 0.f));
    }
}

void AMissileActor::BeginPlay()
{
    Super::BeginPlay();
}

void AMissileActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!bLaunched) return;

    ElapsedTime += DeltaTime;
    if (ElapsedTime >= Lifetime)
    {
        Explode();
        return;
    }
    SeekTarget(DeltaTime);
    CheckHit();
}

void AMissileActor::Launch(
    AActor* InTarget, float InSpeed)
{
    Target = InTarget;
    MissileSpeed = InSpeed;
    bLaunched = true;
}

void AMissileActor::SeekTarget(float DeltaTime)
{
    if (!Target)
    {
        SetActorLocation(
            GetActorLocation() +
            GetActorForwardVector() *
            MissileSpeed * DeltaTime);
        return;
    }

    FVector CurrentPos = GetActorLocation();
    FVector TargetPos =
        Target->GetActorLocation();
    FVector Direction =
        (TargetPos - CurrentPos).GetSafeNormal();

    SetActorRotation(FMath::RInterpTo(
        GetActorRotation(),
        Direction.Rotation(),
        DeltaTime, 5.f));

    SetActorLocation(
        CurrentPos +
        Direction * MissileSpeed * DeltaTime);

    DrawDebugLine(
        GetWorld(),
        CurrentPos, TargetPos,
        FColor::Red, false, -1.f, 0, 1.f);
}

void AMissileActor::CheckHit()
{
    if (!Target) return;
    if (FVector::Dist(
        GetActorLocation(),
        Target->GetActorLocation()) < 100.f)
        Explode();
}

void AMissileActor::Explode()
{
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(
            -1, 2.f, FColor::Orange,
            TEXT("MISSILE HIT!"));

    DrawDebugSphere(
        GetWorld(), GetActorLocation(),
        200.f, 12, FColor::Orange, false, 2.f);
    DrawDebugSphere(
        GetWorld(), GetActorLocation(),
        100.f, 12, FColor::Red, false, 2.f);

    if (Target)
    {
        Target->Destroy();
        Target = nullptr;
    }
    Destroy();
}