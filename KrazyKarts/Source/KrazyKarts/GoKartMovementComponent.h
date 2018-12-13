// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float Time;			// vrijeme kada se 'Move' dogodio
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SimulateMove(const FGoKartMove& Move);

	FVector GetVelocity() { return Velocity; }
	void SetVelocity(FVector Value) { Velocity = Value; }

	void SetThrottle(float Value) { Throttle = Value; }
	void SetSteeringThrow(float Value) { SteeringThrow = Value; }

	FGoKartMove GetLastMove() { return LastMove; }
private:
	FGoKartMove CreateMove(float DeltaTime);

	FVector GetAirResistance();
	FVector GetRollingResistance();

	void ApplyRotation(float DeltaTime, float SteeringThrow);

	void UpdateLocationFromVelocity(float DeltaTime);

	// Masa auta (kg).
	UPROPERTY(EditAnywhere)
	float Mass = 1000;

	// Sila primjenjena na auto pri maksimalnom gasu (N)
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;

	// minimalni okretni radius pri potpuno zaokrenutom volanu (m)
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10;

	// sto je veci to ima vise opterecenja
	UPROPERTY(EditAnywhere)
	float DragCoef = 16;

	// sto je veci to ima vise opterecenja
	UPROPERTY(EditAnywhere)
	float RollingResistanceCoef = 0.015;		// vrijednost preuzeta iz dokumentacije
	
	FVector Velocity;

	float Throttle;
	float SteeringThrow;

	FGoKartMove LastMove;
};
