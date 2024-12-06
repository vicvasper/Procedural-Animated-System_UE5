#include "CreateComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

// Constructor de UCreateComponent
UCreateComponent::UCreateComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    // Cargar el Blueprint del actor de spline automáticamente (opcional)
    static ConstructorHelpers::FClassFinder<AActor> SplineActorFinder(TEXT("/Game/BPs/PCG_SplineActor.PCG_SplineActor"));
    if (SplineActorFinder.Succeeded())
    {
        SplineActorClass = SplineActorFinder.Class;
    }
}

// Método BeginPlay
void UCreateComponent::BeginPlay()
{
    Super::BeginPlay();
}

// Método TickComponent
void UCreateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// Método que detecta el entorno
void UCreateComponent::DetectEnvironment()
{
    FVector Start = GetOwner()->GetActorLocation(); // Posición actual del personaje
    FVector Down = Start + FVector(0, 0, -DetectionRange); // Dirección hacia abajo desde el personaje

    TArray<FHitResult> HitResults;
    PerformLineTrace(HitResults, Start, Down); // Realiza el trace de arriba hacia abajo

    if (HitResults.Num() > 0)
    {
        const FHitResult& Hit = HitResults[0];
        FVector ImpactPoint = Hit.ImpactPoint; // Punto de impacto con el suelo
        FVector Normal = Hit.ImpactNormal;

        // Verificamos el ángulo de la superficie donde impacta el rayo
        float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Normal, FVector::UpVector)));

        if (Angle > 80.0f) // Si es una pared
        {
            HandleWall(ImpactPoint, Normal);
        }
        else if (Angle > MaxSlopeAngle) // Si es una pendiente empinada
        {
            HandleSteepSlope(ImpactPoint);
        }
        else // Si es el suelo o una superficie plana
        {
            HandleAbyss(ImpactPoint);  // Usamos el punto de impacto como inicio para el puente
        }
    }
    else
    {
        // Si no detecta nada, el Trace no tuvo impacto
        UE_LOG(LogTemp, Warning, TEXT("No se detectó suelo debajo del personaje."));
    }
}

// Manejo del "Abyss"
void UCreateComponent::HandleAbyss(const FVector& Start)
{
    FVector EndPoint = FindAbyssEnd(Start);
    int32 NumSegments = 5; // Dividir el puente en segmentos uniformes.

    TArray<FVector> SplinePoints;
    for (int32 i = 0; i <= NumSegments; ++i)
    {
        SplinePoints.Add(FMath::Lerp(Start, EndPoint, (float)i / NumSegments));
    }

    GenerateSpline(SplinePoints);
}

// Manejo de la "Wall"
void UCreateComponent::HandleWall(const FVector& WallPoint, const FVector& AverageNormal)
{
    FVector TopPoint = WallPoint + FVector::UpVector * DetectionRange;
    GenerateZigzagSpline(WallPoint, TopPoint, ZigzagHeightStep);
}

// Manejo de una pendiente empinada
void UCreateComponent::HandleSteepSlope(const FVector& SlopeStart)
{
    FVector CurrentPoint = SlopeStart;
    TArray<FVector> SplinePoints;
    int NumSteps = 10; // Cantidad de escalones

    float StepSize = 100.0f; // Tamaño de cada paso en el avance

    for (int i = 0; i < NumSteps; ++i)
    {
        CurrentPoint += FVector(0, 0, 50);  // Eleva cada paso 50 unidades
        CurrentPoint += GetOwner()->GetActorForwardVector() * StepSize; // Avanza en la dirección del actor

        SplinePoints.Add(CurrentPoint);
    }

    GenerateSpline(SplinePoints);
}

// Método que calcula la normal promedio
FVector UCreateComponent::CalculateAverageNormal(const TArray<FHitResult>& HitResults)
{
    FVector Sum = FVector::ZeroVector;
    for (const auto& Hit : HitResults)
    {
        Sum += Hit.ImpactNormal;
    }
    return Sum.GetSafeNormal();
}

// Encuentra el fin del "Abyss"
FVector UCreateComponent::FindAbyssEnd(const FVector& Start)
{
    FVector End = Start;
    FVector StepForward = GetOwner()->GetActorForwardVector() * 100.0f;
    FVector DownOffset(0, 0, -500);

    for (int i = 0; i < 20; ++i)
    {
        TArray<FHitResult> HitResults;
        PerformLineTrace(HitResults, End + StepForward, End + StepForward + DownOffset);

        if (HitResults.Num() > 0)
        {
            return HitResults[0].ImpactPoint;
        }

        End += StepForward;
    }

    return End;
}

// Genera un spline en zigzag
void UCreateComponent::GenerateZigzagSpline(const FVector& Start, const FVector& End, float HeightStep)
{
    FVector Direction = (End - Start).GetSafeNormal();
    FVector Right = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal() * ZigzagWidth;

    TArray<FVector> Points;
    FVector CurrentPoint = Start;

    for (int i = 0; i < 10; ++i) // Ajusta los pasos según la pared
    {
        CurrentPoint += FVector(0, 0, HeightStep); // Escalada vertical
        CurrentPoint += (i % 2 == 0) ? Right : -Right; // Alternar zigzag
        Points.Add(CurrentPoint);
    }

    GenerateSpline(Points);
}

// Genera un spline usando los puntos dados
void UCreateComponent::GenerateSpline(TArray<FVector> Points)
{
    if (SplineActorClass && Points.Num() > 1)
    {
        // Instancia el actor de spline desde la clase Blueprint
        AActor* SplineActor = GetWorld()->SpawnActor<AActor>(SplineActorClass, Points[0], FRotator::ZeroRotator);
        if (!SplineActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("No se pudo instanciar el actor de spline"));
            return;
        }

        // Encuentra el componente Spline dentro del actor instanciado
        USplineComponent* Spline = SplineActor->FindComponentByClass<USplineComponent>();
        if (Spline)
        {
            // Limpia los puntos previos en la spline
            Spline->ClearSplinePoints();

            // Añade los puntos de la spline que has calculado
            for (const FVector& Point : Points)
            {
                Spline->AddSplinePoint(Point, ESplineCoordinateSpace::World);
            }

            // Actualiza la spline para reflejar los cambios
            Spline->UpdateSpline();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No se encontró el componente Spline en el actor instanciado"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No se proporcionaron suficientes puntos para crear una spline"));
    }
}

// Realiza el line trace
void UCreateComponent::PerformLineTrace(TArray<FHitResult>& HitResults, const FVector& Start, const FVector& End)
{
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());

    // Realiza el trace usando el canal de visibilidad
    GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, ECC_Visibility, Params);

    // Dibuja la línea para depuración
    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 5.0f, 0, 1.0f);
}

// Verifica si hay terreno debajo
bool UCreateComponent::IsGroundBelow(const FVector& Start, float TraceDistance)
{
    FVector Down = Start + FVector(0, 0, -TraceDistance);

    TArray<FHitResult> HitResults;
    PerformLineTrace(HitResults, Start, Down);

    return HitResults.Num() > 0;
}
