#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SplineComponent.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "CreateComponent.generated.h"

USTRUCT()
struct FAnimatedSplinePoint
{
    GENERATED_BODY()

    FVector StartPosition;
    FVector EndPosition;
    float StartScale;
    float EndScale;
    FRotator StartRotation;
    FRotator EndRotation;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROCEDURALNANOBOTS_API UCreateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCreateComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "EnvironmentDetection")
    void DetectEnvironment();

private:
    void HandleAbyss(const FVector& Start);
    void HandleWall(const FVector& WallPoint, const FVector& AverageNormal);
    void HandleSteepSlope(const FVector& SlopeStart);

    FVector CalculateAverageNormal(const TArray<FHitResult>& HitResults);
    FVector FindAbyssEnd(const FVector& Start);
    void GenerateZigzagSpline(const FVector& Start, const FVector& End, float HeightStep);
    void GenerateSpline(TArray<FVector> Points);
    void PerformLineTrace(TArray<FHitResult>& HitResults, const FVector& Start, const FVector& End);

    bool IsGroundBelow(const FVector& Start, float TraceDistance);

    void AnimateSpline(AActor* SplineActor, float DeltaTime);

    UPROPERTY(EditAnywhere)
    float DetectionRange = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "SplineActor")
    TSubclassOf<AActor> SplineActorClass;

    UPROPERTY(EditAnywhere)
    float ZigzagWidth = 300.0f;

    UPROPERTY(EditAnywhere)
    float ZigzagHeightStep = 150.0f;

    UPROPERTY(EditAnywhere)
    float MaxSlopeAngle = 45.0f;

    UPROPERTY(EditAnywhere)
    float AnimationDuration = 2.0f; // Tiempo para completar la animación

    TMap<USplineComponent*, TArray<FAnimatedSplinePoint>> AnimatedSplines;
    TMap<USplineComponent*, float> AnimationTimers;
};