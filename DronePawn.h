#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PIDController.h"
#include "Components/SkeletalMeshComponent.h"
#include "DronePawn.generated.h"

UCLASS()
class DRONE_PACK_API ADronePawn : public APawn
{
    GENERATED_BODY()

public:
    ADronePawn();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(
        class UInputComponent*
        PlayerInputComponent) override;

    // ── Drone Body ────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, Category = "Drone")
        USkeletalMeshComponent* DroneMesh;

    // ── Propellers Separate Skeletal Meshes ───────────────
    UPROPERTY(VisibleAnywhere, Category = "Drone")
        USkeletalMeshComponent* Propeller_FL;
    UPROPERTY(VisibleAnywhere, Category = "Drone")
        USkeletalMeshComponent* Propeller_FR;
    UPROPERTY(VisibleAnywhere, Category = "Drone")
        USkeletalMeshComponent* Propeller_BL;
    UPROPERTY(VisibleAnywhere, Category = "Drone")
        USkeletalMeshComponent* Propeller_BR;

    // ── Cameras ───────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, Category = "Drone")
        class USpringArmComponent* SpringArm;
    UPROPERTY(VisibleAnywhere, Category = "Drone")
        class UCameraComponent* Camera;
    UPROPERTY(VisibleAnywhere, Category = "Drone")
        class UCameraComponent* FPVCamera;

    // ── Physics ───────────────────────────────────────────
    UPROPERTY(EditAnywhere,
        Category = "Drone|Physics")
        float Mass = 1.5f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|Physics")
        float MaxRotorThrust = 2400.f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|Physics")
        float MaxTiltAngle = 70.f;

    // ── PID Altitude ──────────────────────────────────────
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Altitude")
        float Alt_Kp = 25.f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Altitude")
        float Alt_Ki = 0.1f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Altitude")
        float Alt_Kd = 8.f;

    // ── PID Pitch ─────────────────────────────────────────
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Pitch")
        float Pitch_Kp = 12.f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Pitch")
        float Pitch_Ki = 0.05f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Pitch")
        float Pitch_Kd = 5.f;

    // ── PID Roll ──────────────────────────────────────────
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Roll")
        float Roll_Kp = 12.f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Roll")
        float Roll_Ki = 0.05f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Roll")
        float Roll_Kd = 5.f;

    // ── PID Yaw ───────────────────────────────────────────
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Yaw")
        float Yaw_Kp = 8.f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Yaw")
        float Yaw_Ki = 0.01f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|PID|Yaw")
        float Yaw_Kd = 4.f;

    // ── Rotor Settings ────────────────────────────────────
    UPROPERTY(EditAnywhere,
        Category = "Drone|Rotors")
        float MaxRotorSpeed = 12000.f;

    // ── Obstacle Avoidance ────────────────────────────────
    UPROPERTY(EditAnywhere,
        Category = "Drone|Avoidance")
        float TraceDistance = 300.f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|Avoidance")
        float AvoidanceStrength = 2.f;

    // ── Missile Settings ──────────────────────────────────
    UPROPERTY(EditAnywhere,
        Category = "Drone|Missile")
        float MissileSpeed = 3000.f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|Missile")
        float LockOnRange = 2000.f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|Missile")
        float FireCooldown = 1.5f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|Missile")
        int32 MaxMissiles = 10;

    // ── Autonomous Flight ─────────────────────────────────
    UPROPERTY(EditAnywhere,
        Category = "Drone|Auto")
        float AutoSpeed = 800.f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|Auto")
        float WaypointDistance = 2000.f;
    UPROPERTY(EditAnywhere,
        Category = "Drone|Auto")
        float WaypointReachRadius = 150.f;

public:
    // ── Getters ───────────────────────────────────────────
    UFUNCTION(BlueprintCallable)
        float GetSpeed()    const;
    UFUNCTION(BlueprintCallable)
        float GetAltitude() const;
    UFUNCTION(BlueprintCallable)
        float GetRotorFL()  const;
    UFUNCTION(BlueprintCallable)
        float GetRotorFR()  const;
    UFUNCTION(BlueprintCallable)
        float GetRotorBL()  const;
    UFUNCTION(BlueprintCallable)
        float GetRotorBR()  const;
    UFUNCTION(BlueprintCallable)
        bool  IsCrashed()   const;
    UFUNCTION(BlueprintCallable)
        int32 GetMissileCount() const
    {
        return MissileCount;
    }
    UFUNCTION(BlueprintCallable)
        bool HasLockedTarget() const
    {
        return LockedTarget != nullptr;
    }
    UFUNCTION(BlueprintCallable)
        bool IsAutoMode() const
    {
        return bAutoMode;
    }
    UFUNCTION(BlueprintCallable)
        bool IsObstacleAhead() const
    {
        return bObstacleAhead;
    }
    UFUNCTION(BlueprintCallable)
        bool IsObstacleLeft() const
    {
        return bObstacleLeft;
    }
    UFUNCTION(BlueprintCallable)
        bool IsObstacleRight() const
    {
        return bObstacleRight;
    }
    UFUNCTION(BlueprintCallable)
        bool IsObstacleAbove() const
    {
        return bObstacleAbove;
    }
    UFUNCTION(BlueprintCallable)
        bool IsObstacleBelow() const
    {
        return bObstacleBelow;
    }

private:
    // ── PIDs ──────────────────────────────────────────────
    FPIDController PID_Altitude;
    FPIDController PID_Pitch;
    FPIDController PID_Roll;
    FPIDController PID_Yaw;

    // ── Input ─────────────────────────────────────────────
    float InputThrottle = 0.f;
    float InputPitch = 0.f;
    float InputRoll = 0.f;
    float InputYaw = 0.f;

    // ── Physics State ─────────────────────────────────────
    FVector Velocity = FVector::ZeroVector;
    FVector PreviousVelocity = FVector::ZeroVector;

    // ── Rotor Thrust ──────────────────────────────────────
    float RotorThrust_FL = 0.f;
    float RotorThrust_FR = 0.f;
    float RotorThrust_BL = 0.f;
    float RotorThrust_BR = 0.f;

    // ── Propeller Angles ──────────────────────────────────
    float PropellerAngle_FL = 0.f;
    float PropellerAngle_FR = 0.f;
    float PropellerAngle_BL = 0.f;
    float PropellerAngle_BR = 0.f;

    // ── Targets ───────────────────────────────────────────
    float TargetAltitude = 0.f;
    float TargetYaw = 0.f;

    // ── State ─────────────────────────────────────────────
    bool bCrashed = false;
    bool bIsFPV = false;

    // ── Obstacle State ────────────────────────────────────
    bool bObstacleAhead = false;
    bool bObstacleLeft = false;
    bool bObstacleRight = false;
    bool bObstacleAbove = false;
    bool bObstacleBelow = false;

    // ── Missile State ─────────────────────────────────────
    AActor* LockedTarget = nullptr;
    int32   MissileCount = 10;
    float   FireCooldownTimer = 0.f;

    // ── Autonomous State ──────────────────────────────────
    bool    bAutoMode = false;
    FVector CurrentWaypoint = FVector::ZeroVector;
    bool    bWaypointSet = false;

    // ── Functions ─────────────────────────────────────────
    void CalculateRotorSpeeds(float DeltaTime);
    void ApplyRotorPhysics(float DeltaTime);
    void SpinPropellers(float DeltaTime);
    void CheckCrash();
    void ResetDrone();
    void ToggleFPV();
    void DetectObstacles();
    bool TraceInDirection(
        FVector Direction,
        float Distance,
        FHitResult& OutHit);
    void LockOnTarget();
    void FireMissile();
    void ToggleAutoMode();
    void UpdateAutonomousFlight(float DeltaTime);
    void SetWaypointAhead();
    void SteerAroundObstacle();
    bool IsWaypointReached();
    void OnThrottle(float Val);
    void OnPitch(float Val);
    void OnRoll(float Val);
    void OnYaw(float Val);
};