#include "DronePawn.h"
#include "MissileActor.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Math/UnrealMathUtility.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"

ADronePawn::ADronePawn()
{
    PrimaryActorTick.bCanEverTick = true;

    // ── Drone Body ────────────────────────────────────────
    DroneMesh = CreateDefaultSubobject
        <USkeletalMeshComponent>(TEXT("DroneMesh"));
    RootComponent = DroneMesh;
    DroneMesh->SetSimulatePhysics(false);

    // ── Propellers ────────────────────────────────────────
    Propeller_FL = CreateDefaultSubobject
        <USkeletalMeshComponent>(
            TEXT("Propeller_FL"));
    Propeller_FL->SetupAttachment(DroneMesh);

    Propeller_FR = CreateDefaultSubobject
        <USkeletalMeshComponent>(
            TEXT("Propeller_FR"));
    Propeller_FR->SetupAttachment(DroneMesh);

    Propeller_BL = CreateDefaultSubobject
        <USkeletalMeshComponent>(
            TEXT("Propeller_BL"));
    Propeller_BL->SetupAttachment(DroneMesh);

    Propeller_BR = CreateDefaultSubobject
        <USkeletalMeshComponent>(
            TEXT("Propeller_BR"));
    Propeller_BR->SetupAttachment(DroneMesh);

    // ── Spring Arm ────────────────────────────────────────
    SpringArm = CreateDefaultSubobject
        <USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(DroneMesh);
    SpringArm->TargetArmLength = 300.f;
    SpringArm->SetRelativeRotation(
        FRotator(-20, 0, 0));
    SpringArm->bUsePawnControlRotation = false;
    SpringArm->bInheritYaw = true;
    SpringArm->bInheritPitch = false;
    SpringArm->bInheritRoll = false;

    // ── Third Person Camera ───────────────────────────────
    Camera = CreateDefaultSubobject
        <UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(
        SpringArm,
        USpringArmComponent::SocketName);
    Camera->SetActive(true);

    // ── FPV Camera ────────────────────────────────────────
    FPVCamera = CreateDefaultSubobject
        <UCameraComponent>(TEXT("FPVCamera"));
    FPVCamera->SetupAttachment(DroneMesh);
    FPVCamera->SetRelativeLocation(
        FVector(20, 0, 5));
    FPVCamera->SetActive(false);
    FPVCamera->bCameraMeshHiddenInGame = true;
}

void ADronePawn::BeginPlay()
{
    Super::BeginPlay();

    PID_Altitude = FPIDController(
        Alt_Kp, Alt_Ki, Alt_Kd);
    PID_Pitch = FPIDController(
        Pitch_Kp, Pitch_Ki, Pitch_Kd);
    PID_Roll = FPIDController(
        Roll_Kp, Roll_Ki, Roll_Kd);
    PID_Yaw = FPIDController(
        Yaw_Kp, Yaw_Ki, Yaw_Kd);

    TargetAltitude = GetActorLocation().Z;
    TargetYaw = GetActorRotation().Yaw;
    MissileCount = MaxMissiles;
}

void ADronePawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bCrashed)
    {
        DetectObstacles();
        LockOnTarget();
        CalculateRotorSpeeds(DeltaTime);
        ApplyRotorPhysics(DeltaTime);
        UpdateAutonomousFlight(DeltaTime);
        SpinPropellers(DeltaTime);
        CheckCrash();
    }

    if (FireCooldownTimer > 0.f)
        FireCooldownTimer -= DeltaTime;
}

// ── Calculate Rotor Speeds ────────────────────────────────
void ADronePawn::CalculateRotorSpeeds(
    float DeltaTime)
{
    float CurrentAlt = GetActorLocation().Z;
    float CurrentPitch = GetActorRotation().Pitch;
    float CurrentRoll = GetActorRotation().Roll;
    float CurrentYawR = GetActorRotation().Yaw;

    // ── Fast Throttle ─────────────────────────────────────
    TargetAltitude +=
        InputThrottle * 2000.f * DeltaTime;

    // ── Fast Direct Yaw ───────────────────────────────────
    TargetYaw = CurrentYawR +
        InputYaw * 720.f * DeltaTime;

    float AltOut = PID_Altitude.Update(
        TargetAltitude - CurrentAlt, DeltaTime);
    float PitchOut = PID_Pitch.Update(
        -InputPitch * MaxTiltAngle - CurrentPitch,
        DeltaTime);
    float RollOut = PID_Roll.Update(
        InputRoll * MaxTiltAngle - CurrentRoll,
        DeltaTime);

    // ── Fast Direct Yaw Out ───────────────────────────────
    float YawOut = InputYaw * 800.f;

    float Base = FMath::Clamp(
        AltOut, 0.f, MaxRotorThrust);
    PitchOut = FMath::Clamp(
        PitchOut, -800.f, 800.f);
    RollOut = FMath::Clamp(
        RollOut, -800.f, 800.f);
    YawOut = FMath::Clamp(
        YawOut, -800.f, 800.f);

    RotorThrust_FL = FMath::Clamp(
        Base + PitchOut + RollOut - YawOut,
        0.f, MaxRotorThrust);
    RotorThrust_FR = FMath::Clamp(
        Base + PitchOut - RollOut + YawOut,
        0.f, MaxRotorThrust);
    RotorThrust_BL = FMath::Clamp(
        Base - PitchOut + RollOut + YawOut,
        0.f, MaxRotorThrust);
    RotorThrust_BR = FMath::Clamp(
        Base - PitchOut - RollOut - YawOut,
        0.f, MaxRotorThrust);
}

// ── Apply Physics ─────────────────────────────────────────
void ADronePawn::ApplyRotorPhysics(float DeltaTime)
{
    const float Gravity = -980.f;

    float TotalThrust =
        RotorThrust_FL + RotorThrust_FR +
        RotorThrust_BL + RotorThrust_BR;

    float PitchTorque =
        (RotorThrust_FL + RotorThrust_FR) -
        (RotorThrust_BL + RotorThrust_BR);
    float RollTorque =
        (RotorThrust_FL + RotorThrust_BL) -
        (RotorThrust_FR + RotorThrust_BR);

    FRotator CurrentRot = GetActorRotation();

    float NewPitch = FMath::Clamp(
        CurrentRot.Pitch +
        PitchTorque * 0.0005f, -70.f, 70.f);
    float NewRoll = FMath::Clamp(
        CurrentRot.Roll +
        RollTorque * 0.0005f, -70.f, 70.f);

    // ── Super Fast Yaw ────────────────────────────────────
    float NewYaw = CurrentRot.Yaw +
        InputYaw * 360.f * DeltaTime;

    SetActorRotation(
        FRotator(NewPitch, NewYaw, NewRoll));

    FVector Up = GetActorUpVector();
    FVector Forward = GetActorForwardVector();
    FVector Right = GetActorRightVector();

    FVector Accel =
        Up * (TotalThrust / Mass) +
        FVector(0, 0, Gravity);

    // ── Obstacle Avoidance ────────────────────────────────
    if (bObstacleAhead)
    {
        Accel -= Forward * AvoidanceStrength * 200.f;
        Accel += FVector(
            0, 0, AvoidanceStrength * 100.f);
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                10, 0.1f, FColor::Red,
                TEXT("OBSTACLE AHEAD!"));
    }
    if (bObstacleLeft)
    {
        Accel += Right * AvoidanceStrength * 200.f;
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                11, 0.1f, FColor::Yellow,
                TEXT("OBSTACLE LEFT!"));
    }
    if (bObstacleRight)
    {
        Accel -= Right * AvoidanceStrength * 200.f;
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                12, 0.1f, FColor::Yellow,
                TEXT("OBSTACLE RIGHT!"));
    }
    if (bObstacleAbove)
    {
        Accel -= FVector(
            0, 0, AvoidanceStrength * 200.f);
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                13, 0.1f, FColor::Orange,
                TEXT("OBSTACLE ABOVE!"));
    }
    if (bObstacleBelow)
    {
        Accel += FVector(
            0, 0, AvoidanceStrength * 200.f);
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                14, 0.1f, FColor::Orange,
                TEXT("OBSTACLE BELOW!"));
    }

    PreviousVelocity = Velocity;
    Velocity += Accel * DeltaTime;

    // ── Very Low Drag = Fast Movement ─────────────────────
    Velocity *= FMath::Clamp(
        1.f - 0.05f * DeltaTime, 0.f, 1.f);

    FVector NewPos =
        GetActorLocation() + Velocity * DeltaTime;
    if (NewPos.Z < 0.f)
    {
        NewPos.Z = 0.f;
        Velocity.Z = 0.f;
    }
    SetActorLocation(NewPos);
}

// ── Spin Propellers By C++ ────────────────────────────────
void ADronePawn::SpinPropellers(float DeltaTime)
{
    auto Spin = [&](
        USkeletalMeshComponent* Prop,
        float& Angle,
        float  Thrust,
        bool   bCW)
    {
        if (!Prop) return;

        float SpinRate =
            (Thrust / MaxRotorThrust) *
            MaxRotorSpeed;

        Angle = FMath::Fmod(
            Angle +
            (bCW ? SpinRate : -SpinRate) *
            DeltaTime, 360.f);

        Prop->SetRelativeRotation(
            FRotator(0.f, Angle, 0.f));
    };

    Spin(Propeller_FL,
        PropellerAngle_FL,
        RotorThrust_FL, false);
    Spin(Propeller_FR,
        PropellerAngle_FR,
        RotorThrust_FR, true);
    Spin(Propeller_BL,
        PropellerAngle_BL,
        RotorThrust_BL, true);
    Spin(Propeller_BR,
        PropellerAngle_BR,
        RotorThrust_BR, false);
}

// ── Ray Tracing ───────────────────────────────────────────
bool ADronePawn::TraceInDirection(
    FVector Direction,
    float   Distance,
    FHitResult& OutHit)
{
    FVector Start = GetActorLocation();
    FVector End = Start + Direction * Distance;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = GetWorld()
        ->LineTraceSingleByChannel(
            OutHit, Start, End,
            ECC_Visibility, Params);

    DrawDebugLine(
        GetWorld(), Start, End,
        bHit ? FColor::Red : FColor::Green,
        false, -1.f, 0, 2.f);

    return bHit;
}

void ADronePawn::DetectObstacles()
{
    FHitResult Hit;
    FVector Forward = GetActorForwardVector();
    FVector Right = GetActorRightVector();
    FVector Up = GetActorUpVector();

    bObstacleAhead = TraceInDirection(
        Forward, TraceDistance, Hit);
    bObstacleLeft = TraceInDirection(
        -Right, TraceDistance, Hit);
    bObstacleRight = TraceInDirection(
        Right, TraceDistance, Hit);
    bObstacleAbove = TraceInDirection(
        Up, TraceDistance, Hit);
    bObstacleBelow = TraceInDirection(
        -Up, TraceDistance * 0.5f, Hit);
}

// ── Lock On Target ────────────────────────────────────────
void ADronePawn::LockOnTarget()
{
    AActor* NearestTarget = nullptr;
    float   NearestDist = LockOnRange;

    for (TActorIterator<AActor>
        It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor == this) continue;
        if (Actor->IsA<APawn>()) continue;
        if (Cast<AMissileActor>(Actor)) continue;
        if (Actor->IsRootComponentStatic())
            continue;

        UStaticMeshComponent* Mesh =
            Actor->FindComponentByClass
            <UStaticMeshComponent>();
        if (!Mesh) continue;

        if (Actor->GetActorLocation().Z < 10.f)
            continue;

        float Dist = FVector::Dist(
            GetActorLocation(),
            Actor->GetActorLocation());

        if (Dist < NearestDist)
        {
            NearestDist = Dist;
            NearestTarget = Actor;
        }
    }

    LockedTarget = NearestTarget;

    if (LockedTarget)
    {
        DrawDebugSphere(
            GetWorld(),
            LockedTarget->GetActorLocation(),
            100.f, 12, FColor::Red,
            false, -1.f);

        DrawDebugLine(
            GetWorld(),
            GetActorLocation(),
            LockedTarget->GetActorLocation(),
            FColor::Red, false, -1.f, 0, 1.f);

        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                20, 0.1f, FColor::Red,
                TEXT("TARGET LOCKED!"));
    }
}

// ── Fire Missile ──────────────────────────────────────────
void ADronePawn::FireMissile()
{
    if (MissileCount <= 0)
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                21, 1.f, FColor::Red,
                TEXT("NO MISSILES LEFT!"));
        return;
    }

    if (FireCooldownTimer > 0.f)
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                22, 0.1f, FColor::Yellow,
                TEXT("RELOADING..."));
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();

    FVector SpawnLoc =
        GetActorLocation() +
        GetActorForwardVector() * 150.f;
    FRotator SpawnRot = GetActorRotation();

    AMissileActor* Missile =
        GetWorld()->SpawnActor<AMissileActor>(
            AMissileActor::StaticClass(),
            SpawnLoc, SpawnRot, SpawnParams);

    if (Missile)
    {
        Missile->Launch(LockedTarget, MissileSpeed);
        MissileCount--;
        FireCooldownTimer = FireCooldown;

        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                23, 1.f, FColor::Green,
                FString::Printf(
                    TEXT("MISSILE FIRED! Left: %d"),
                    MissileCount));
    }
}

// ── Autonomous Flight ─────────────────────────────────────
void ADronePawn::ToggleAutoMode()
{
    bAutoMode = !bAutoMode;
    if (bAutoMode)
    {
        SetWaypointAhead();
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                30, 2.f, FColor::Cyan,
                TEXT("AUTO MODE ON!"));
    }
    else
    {
        bWaypointSet = false;
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                30, 2.f, FColor::Yellow,
                TEXT("MANUAL MODE"));
    }
}

void ADronePawn::SetWaypointAhead()
{
    CurrentWaypoint =
        GetActorLocation() +
        GetActorForwardVector() * WaypointDistance;
    CurrentWaypoint.Z = GetActorLocation().Z;
    bWaypointSet = true;

    DrawDebugSphere(
        GetWorld(), CurrentWaypoint,
        150.f, 12, FColor::Cyan, false, 3.f);
}

bool ADronePawn::IsWaypointReached()
{
    if (!bWaypointSet) return false;
    return FVector::Dist(
        GetActorLocation(),
        CurrentWaypoint) < WaypointReachRadius;
}

void ADronePawn::SteerAroundObstacle()
{
    FVector SteerDir = FVector::ZeroVector;

    if (bObstacleAhead)
    {
        SteerDir += FVector(0, 0, 1.f);
        SteerDir += GetActorRightVector();
    }
    if (bObstacleLeft)
        SteerDir += GetActorRightVector();
    if (bObstacleRight)
        SteerDir -= GetActorRightVector();
    if (bObstacleAbove)
        SteerDir -= FVector(0, 0, 1.f);

    if (!SteerDir.IsZero())
        CurrentWaypoint =
        GetActorLocation() +
        SteerDir.GetSafeNormal() *
        WaypointDistance;
}

void ADronePawn::UpdateAutonomousFlight(
    float DeltaTime)
{
    if (!bAutoMode || !bWaypointSet) return;

    if (IsWaypointReached())
    {
        SetWaypointAhead();
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(
                31, 1.f, FColor::Green,
                TEXT("WAYPOINT REACHED!"));
        return;
    }

    SteerAroundObstacle();

    FVector ToWaypoint =
        (CurrentWaypoint - GetActorLocation())
        .GetSafeNormal();

    Velocity += ToWaypoint * AutoSpeed * DeltaTime;
    Velocity.Z +=
        (CurrentWaypoint.Z - GetActorLocation().Z)
        * 2.f * DeltaTime;

    DrawDebugLine(
        GetWorld(),
        GetActorLocation(), CurrentWaypoint,
        FColor::Cyan, false, -1.f, 0, 3.f);

    DrawDebugSphere(
        GetWorld(), CurrentWaypoint,
        150.f, 12, FColor::Cyan, false, -1.f);
}

// ── Crash ─────────────────────────────────────────────────
void ADronePawn::CheckCrash()
{
    float Impact =
        (Velocity - PreviousVelocity).Size();
    if (Impact > 1000.f &&
        GetActorLocation().Z < 5.f)
    {
        bCrashed = true;
        Velocity = FVector::ZeroVector;
        PID_Altitude.Reset();
        PID_Pitch.Reset();
        PID_Roll.Reset();
        PID_Yaw.Reset();
    }
}

// ── Reset ─────────────────────────────────────────────────
void ADronePawn::ResetDrone()
{
    bCrashed = false;
    Velocity = FVector::ZeroVector;
    RotorThrust_FL = RotorThrust_FR = 0.f;
    RotorThrust_BL = RotorThrust_BR = 0.f;
    TargetAltitude = 100.f;
    TargetYaw = 0.f;
    MissileCount = MaxMissiles;
    bAutoMode = false;
    bWaypointSet = false;

    SetActorLocation(FVector(0, 0, 100));
    SetActorRotation(FRotator::ZeroRotator);

    PID_Altitude.Reset();
    PID_Pitch.Reset();
    PID_Roll.Reset();
    PID_Yaw.Reset();
}

// ── FPV ───────────────────────────────────────────────────
void ADronePawn::ToggleFPV()
{
    bIsFPV = !bIsFPV;
    FPVCamera->SetActive(bIsFPV);
    Camera->SetActive(!bIsFPV);
}

// ── Input ─────────────────────────────────────────────────
void ADronePawn::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(
        PlayerInputComponent);

    PlayerInputComponent->BindAxis(
        "Throttle", this, &ADronePawn::OnThrottle);
    PlayerInputComponent->BindAxis(
        "Pitch", this, &ADronePawn::OnPitch);
    PlayerInputComponent->BindAxis(
        "Roll", this, &ADronePawn::OnRoll);
    PlayerInputComponent->BindAxis(
        "Yaw", this, &ADronePawn::OnYaw);

    PlayerInputComponent->BindAction(
        "ResetDrone", IE_Pressed,
        this, &ADronePawn::ResetDrone);
    PlayerInputComponent->BindAction(
        "ToggleFPV", IE_Pressed,
        this, &ADronePawn::ToggleFPV);
    PlayerInputComponent->BindAction(
        "FireMissile", IE_Pressed,
        this, &ADronePawn::FireMissile);
    PlayerInputComponent->BindAction(
        "AutoMode", IE_Pressed,
        this, &ADronePawn::ToggleAutoMode);
}

void ADronePawn::OnThrottle(float Val)
{
    InputThrottle = Val;
}
void ADronePawn::OnPitch(float Val)
{
    InputPitch = Val;
}
void ADronePawn::OnRoll(float Val)
{
    InputRoll = Val;
}
void ADronePawn::OnYaw(float Val)
{
    InputYaw = Val;
}

float ADronePawn::GetSpeed() const
{
    return Velocity.Size();
}
float ADronePawn::GetAltitude() const
{
    return GetActorLocation().Z;
}
float ADronePawn::GetRotorFL() const
{
    return RotorThrust_FL;
}
float ADronePawn::GetRotorFR() const
{
    return RotorThrust_FR;
}
float ADronePawn::GetRotorBL() const
{
    return RotorThrust_BL;
}
float ADronePawn::GetRotorBR() const
{
    return RotorThrust_BR;
}
bool ADronePawn::IsCrashed() const
{
    return bCrashed;
}