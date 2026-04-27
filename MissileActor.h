#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissileActor.generated.h"

UCLASS()
class DRONE_PACK_API AMissileActor
    : public AActor
{
    GENERATED_BODY()
public:
    AMissileActor();
    void Launch(AActor* InTarget, float InSpeed);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY(VisibleAnywhere)
        class UStaticMeshComponent* MissileMesh;

    AActor* Target = nullptr;
    float   MissileSpeed = 3000.f;
    float   Lifetime = 8.f;
    float   ElapsedTime = 0.f;
    bool    bLaunched = false;

    void SeekTarget(float DeltaTime);
    void CheckHit();
    void Explode();
};