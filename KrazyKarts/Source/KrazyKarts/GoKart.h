// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
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

	float Throttle;
	float SteeringThrow;
	FVector Velocity;
	
	FVector GetAirResistance();
	FVector GetRollingResistance();

	void ApplyRotation(float DeltaTime);
	void UpdateLocationFromVelocity(float DeltaTime);

	void MoveForward(float Value);
	void MoveRight(float Value);
};