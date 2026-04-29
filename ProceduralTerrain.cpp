#include "ProceduralTerrain.h"
#include "KismetProceduralMeshLibrary.h"
#include "Engine/Engine.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AProceduralTerrain::AProceduralTerrain()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject
        <USceneComponent>(TEXT("Root"));

    TerrainMesh = CreateDefaultSubobject
        <UProceduralMeshComponent>(
            TEXT("TerrainMesh"));
    TerrainMesh->SetupAttachment(
        RootComponent);
    TerrainMesh->bUseAsyncCooking = true;

    // Large spruce trees
    TreeMesh = CreateDefaultSubobject
        <UInstancedStaticMeshComponent>(
            TEXT("TreeMesh"));
    TreeMesh->SetupAttachment(RootComponent);
    TreeMesh->SetCullDistances(0, 20000);
    TreeMesh->SetMobility(
        EComponentMobility::Movable);

    // Small winter spruces
    SmallTreeMesh = CreateDefaultSubobject
        <UInstancedStaticMeshComponent>(
            TEXT("SmallTreeMesh"));
    SmallTreeMesh->SetupAttachment(
        RootComponent);
    SmallTreeMesh->SetCullDistances(
        0, 12000);
    SmallTreeMesh->SetMobility(
        EComponentMobility::Movable);

    WaterMesh = CreateDefaultSubobject
        <UStaticMeshComponent>(
            TEXT("WaterMesh"));
    WaterMesh->SetupAttachment(RootComponent);
}

void AProceduralTerrain::BeginPlay()
{
    Super::BeginPlay();
    SetupWater();
}

void AProceduralTerrain::Tick(
    float DeltaTime)
{
    Super::Tick(DeltaTime);

    APlayerController* PC =
        GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    FVector PawnLoc =
        Pawn->GetActorLocation();

    // On first tick force generate
    // all chunks around player
    if (!bInitialized)
    {
        bInitialized = true;
        LastDroneChunk = FIntPoint(
            INT_MAX, INT_MAX);
    }

    UpdateChunks(PawnLoc);
}

float AProceduralTerrain::Fade(float T)
{
    return T * T * T *
        (T * (T * 6 - 15) + 10);
}

float AProceduralTerrain::Lerp(
    float A, float B, float T)
{
    return A + T * (B - A);
}

float AProceduralTerrain::Grad(
    int Hash, float X, float Y)
{
    int   H = Hash & 3;
    float U = H < 2 ? X : Y;
    float V = H < 2 ? Y : X;
    return ((H & 1) ? -U : U) +
        ((H & 2) ? -V : V);
}

float AProceduralTerrain::PerlinNoise(
    float X, float Y)
{
    int XI =
        (int)FMath::FloorToInt(X) & 255;
    int YI =
        (int)FMath::FloorToInt(Y) & 255;
    float XF = X - FMath::FloorToInt(X);
    float YF = Y - FMath::FloorToInt(Y);
    float U = Fade(XF);
    float V = Fade(YF);

    int A = (XI + YI) & 255;
    int B = (XI + YI + 1) & 255;
    int AA = (A + XI) & 255;
    int AB = (A + YI) & 255;
    int BA = (B + XI) & 255;
    int BB = (B + YI) & 255;

    return Lerp(
        Lerp(Grad(AA, XF, YF),
            Grad(BA, XF - 1, YF), U),
        Lerp(Grad(AB, XF, YF - 1),
            Grad(BB, XF - 1, YF - 1),
            U), V);
}

float AProceduralTerrain::GetHeight(
    float X, float Y)
{
    float H = 0.f;

    // Very flat base - forest floor
    H += PerlinNoise(
        X * NoiseScale,
        Y * NoiseScale) *
        HeightScale * 0.20f;

    // Tiny gentle bumps only
    H += PerlinNoise(
        X * NoiseScale * 5.f,
        Y * NoiseScale * 5.f) *
        HeightScale * 0.08f;

    // Micro surface variation
    H += PerlinNoise(
        X * NoiseScale * 12.f,
        Y * NoiseScale * 12.f) *
        HeightScale * 0.03f;

    return H;
}

void AProceduralTerrain::GenerateChunk(
    FIntPoint ChunkPos)
{
    if (Chunks.Contains(ChunkPos)) return;

    UProceduralMeshComponent* Chunk =
        NewObject<UProceduralMeshComponent>(
            this);
    Chunk->RegisterComponent();
    Chunk->AttachToComponent(
        RootComponent,
        FAttachmentTransformRules
        ::KeepRelativeTransform);
    Chunk->bUseAsyncCooking = true;

    float OffsetX =
        ChunkPos.X * ChunkSize * TileSize;
    float OffsetY =
        ChunkPos.Y * ChunkSize * TileSize;

    TArray<FVector>          Vertices;
    TArray<int32>            Triangles;
    TArray<FVector>          Normals;
    TArray<FVector2D>        UVs;
    TArray<FColor>           Colors;
    TArray<FProcMeshTangent> Tangents;

    float TotalHeight = 0.f;
    int32 VCount =
        (ChunkSize + 1) * (ChunkSize + 1);
    Vertices.Reserve(VCount);

    for (int32 Y = 0; Y <= ChunkSize; Y++)
    {
        for (int32 X = 0; X <= ChunkSize; X++)
        {
            float WX =
                OffsetX + X * TileSize;
            float WY =
                OffsetY + Y * TileSize;
            float H = GetHeight(WX, WY);

            Vertices.Add(FVector(
                X * TileSize,
                Y * TileSize, H));

            UVs.Add(FVector2D(
                (float)X / ChunkSize * 8.f,
                (float)Y / ChunkSize * 8.f));

            TotalHeight += H;

            // Pure white snow everywhere
            FColor Color;
            if (H < WaterLevel + 5.f)
                // Frozen ice
                Color = FColor(
                    185, 215, 240);
            else
                // Snow ground
                Color = FColor(
                    245, 248, 252);

            Colors.Add(Color);
        }
    }

    for (int32 Y = 0; Y < ChunkSize; Y++)
    {
        for (int32 X = 0; X < ChunkSize; X++)
        {
            int32 I =
                Y * (ChunkSize + 1) + X;
            Triangles.Add(I);
            Triangles.Add(I + ChunkSize + 1);
            Triangles.Add(I + 1);
            Triangles.Add(I + 1);
            Triangles.Add(I + ChunkSize + 1);
            Triangles.Add(
                I + ChunkSize + 2);
        }
    }

    UKismetProceduralMeshLibrary
        ::CalculateTangentsForMesh(
            Vertices, Triangles,
            UVs, Normals, Tangents);

    Chunk->CreateMeshSection(
        0, Vertices, Triangles,
        Normals, UVs, Colors,
        Tangents, true);

    // Always use winter ground material
    if (GroundWinterMaterial)
        Chunk->SetMaterial(
            0, GroundWinterMaterial);

    Chunk->SetRelativeLocation(
        FVector(OffsetX, OffsetY, 0));

    Chunks.Add(ChunkPos, Chunk);

    GenerateTrees(ChunkPos, Vertices);
    GenerateSmallTrees(ChunkPos, Vertices);
}

void AProceduralTerrain::AssignBiomeMaterial(
    UProceduralMeshComponent* Chunk,
    float AvgHeight)
{
    if (!Chunk) return;
    if (GroundWinterMaterial)
        Chunk->SetMaterial(
            0, GroundWinterMaterial);
}

void AProceduralTerrain::RemoveChunk(
    FIntPoint ChunkPos)
{
    if (!Chunks.Contains(ChunkPos)) return;
    UProceduralMeshComponent* Chunk =
        Chunks[ChunkPos];
    if (Chunk) Chunk->DestroyComponent();
    Chunks.Remove(ChunkPos);
}

void AProceduralTerrain::UpdateChunks(
    FVector DroneLocation)
{
    // Calculate which chunk drone is in
    FIntPoint DroneChunk = FIntPoint(
        FMath::FloorToInt(
            DroneLocation.X /
            (ChunkSize * TileSize)),
        FMath::FloorToInt(
            DroneLocation.Y /
            (ChunkSize * TileSize)));

    // Only update when drone
    // crosses into new chunk
    if (DroneChunk == LastDroneChunk)
        return;

    LastDroneChunk = DroneChunk;

    TSet<FIntPoint> NeededChunks;

    // Generate all chunks in range
    for (int32 Y = -ViewDistance;
        Y <= ViewDistance; Y++)
    {
        for (int32 X = -ViewDistance;
            X <= ViewDistance; X++)
        {
            FIntPoint ChunkPos = FIntPoint(
                DroneChunk.X + X,
                DroneChunk.Y + Y);
            NeededChunks.Add(ChunkPos);
            if (!Chunks.Contains(ChunkPos))
                GenerateChunk(ChunkPos);
        }
    }

    // Remove chunks too far away
    TArray<FIntPoint> ToRemove;
    for (auto& Pair : Chunks)
        if (!NeededChunks.Contains(
            Pair.Key))
            ToRemove.Add(Pair.Key);

    for (auto& Pos : ToRemove)
        RemoveChunk(Pos);
}

void AProceduralTerrain::GenerateTrees(
    FIntPoint ChunkPos,
    TArray<FVector>& Vertices)
{
    if (!TreeMesh->GetStaticMesh()) return;

    float OffsetX =
        ChunkPos.X * ChunkSize * TileSize;
    float OffsetY =
        ChunkPos.Y * ChunkSize * TileSize;

    for (int32 Y = 0; Y <= ChunkSize; Y++)
    {
        for (int32 X = 0; X <= ChunkSize; X++)
        {
            int32 Idx =
                Y * (ChunkSize + 1) + X;
            if (!Vertices.IsValidIndex(Idx))
                continue;

            float H = Vertices[Idx].Z;

            if (H < TreeMinHeight ||
                H > TreeMaxHeight)
                continue;

            if (FMath::FRand() > TreeDensity)
                continue;

            FTransform T;
            T.SetLocation(FVector(
                OffsetX + X * TileSize +
                FMath::RandRange(
                    -50.f, 50.f),
                OffsetY + Y * TileSize +
                FMath::RandRange(
                    -50.f, 50.f),
                H));

            // spruce_full is a VERY large
            // mesh, keep scale very small
            float S = FMath::RandRange(
                0.15f, 0.30f);
            float SZ = S * FMath::RandRange(
                1.2f, 1.8f);
            T.SetScale3D(FVector(S, S, SZ));

            T.SetRotation(FQuat(FRotator(
                0,
                FMath::RandRange(
                    0.f, 360.f),
                0)));

            TreeMesh->AddInstance(T);
        }
    }
}

void AProceduralTerrain::GenerateSmallTrees(
    FIntPoint ChunkPos,
    TArray<FVector>& Vertices)
{
    if (!SmallTreeMesh->GetStaticMesh())
        return;

    float OffsetX =
        ChunkPos.X * ChunkSize * TileSize;
    float OffsetY =
        ChunkPos.Y * ChunkSize * TileSize;

    for (int32 Y = 0; Y <= ChunkSize; Y++)
    {
        for (int32 X = 0; X <= ChunkSize; X++)
        {
            int32 Idx =
                Y * (ChunkSize + 1) + X;
            if (!Vertices.IsValidIndex(Idx))
                continue;

            float H = Vertices[Idx].Z;

            if (H < WaterLevel + 5.f ||
                H > TreeMaxHeight * 1.5f)
                continue;

            if (FMath::FRand() >
                SmallTreeDensity) continue;

            FTransform T;
            T.SetLocation(FVector(
                OffsetX + X * TileSize +
                FMath::RandRange(
                    -30.f, 30.f),
                OffsetY + Y * TileSize +
                FMath::RandRange(
                    -30.f, 30.f),
                H));

            // winter_spruce_small
            // is correct natural size
            float S = FMath::RandRange(
                0.5f, 1.0f);
            float SZ = S * FMath::RandRange(
                1.0f, 1.5f);
            T.SetScale3D(FVector(S, S, SZ));

            T.SetRotation(FQuat(FRotator(
                0,
                FMath::RandRange(
                    0.f, 360.f),
                0)));

            SmallTreeMesh->AddInstance(T);
        }
    }
}

void AProceduralTerrain::SetupWater()
{
    if (!WaterMesh) return;
    WaterMesh->SetRelativeLocation(
        FVector(0, 0, WaterLevel));
    WaterMesh->SetRelativeScale3D(
        FVector(1000.f, 1000.f, 1.f));
    if (WaterMaterial)
        WaterMesh->SetMaterial(
            0, WaterMaterial);
}