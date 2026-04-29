#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ProceduralTerrain.generated.h"

UCLASS()
class DRONE_PACK_API AProceduralTerrain
    : public AActor
{
    GENERATED_BODY()
public:
    AProceduralTerrain();
protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime)
        override;

    UPROPERTY(VisibleAnywhere)
        UProceduralMeshComponent* TerrainMesh;

    UPROPERTY(VisibleAnywhere)
        UInstancedStaticMeshComponent* TreeMesh;

    UPROPERTY(VisibleAnywhere)
        UInstancedStaticMeshComponent*
        SmallTreeMesh;

    UPROPERTY(VisibleAnywhere)
        UStaticMeshComponent* WaterMesh;

    // Terrain
    UPROPERTY(EditAnywhere,
        Category = "Terrain")
        int32 ChunkSize = 32;

    UPROPERTY(EditAnywhere,
        Category = "Terrain")
        float TileSize = 250.f;

    UPROPERTY(EditAnywhere,
        Category = "Terrain")
        float HeightScale = 300.f;

    UPROPERTY(EditAnywhere,
        Category = "Terrain")
        float NoiseScale = 0.001f;

    UPROPERTY(EditAnywhere,
        Category = "Terrain")
        int32 ViewDistance = 7;

    // Water
    UPROPERTY(EditAnywhere,
        Category = "Water")
        float WaterLevel = 20.f;

    // Trees
    UPROPERTY(EditAnywhere,
        Category = "Trees")
        float TreeDensity = 0.10f;

    UPROPERTY(EditAnywhere,
        Category = "Trees")
        float TreeMinHeight = 10.f;

    UPROPERTY(EditAnywhere,
        Category = "Trees")
        float TreeMaxHeight = 280.f;

    UPROPERTY(EditAnywhere,
        Category = "Trees")
        float SmallTreeDensity = 0.18f;

    // Materials
    UPROPERTY(EditAnywhere,
        Category = "Materials")
        UMaterialInterface* GroundWinterMaterial;

    UPROPERTY(EditAnywhere,
        Category = "Materials")
        UMaterialInterface* SnowMaterial;

    UPROPERTY(EditAnywhere,
        Category = "Materials")
        UMaterialInterface* WaterMaterial;

private:
    TMap<FIntPoint,
        UProceduralMeshComponent*> Chunks;
    FIntPoint LastDroneChunk =
        FIntPoint(INT_MAX, INT_MAX);
    bool bInitialized = false;

    void GenerateChunk(FIntPoint ChunkPos);
    void RemoveChunk(FIntPoint ChunkPos);
    void UpdateChunks(FVector Location);
    void GenerateTrees(FIntPoint ChunkPos,
        TArray<FVector>& Vertices);
    void GenerateSmallTrees(
        FIntPoint ChunkPos,
        TArray<FVector>& Vertices);
    void SetupWater();
    void AssignBiomeMaterial(
        UProceduralMeshComponent* Chunk,
        float AvgHeight);
    float GetHeight(float X, float Y);
    float PerlinNoise(float X, float Y);
    float Fade(float T);
    float Lerp(float A, float B, float T);
    float Grad(int Hash, float X, float Y);
};