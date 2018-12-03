// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"

#include "Components/InputComponent.h"

#include "Engine/World.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * DeltaTime;

	ApplyRotation(DeltaTime);

	UpdateLocationFromVelocity(DeltaTime);
}

FVector AGoKart::GetAirResistance()
{
	return - Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoef;		// GetSafeNormal() vraca vektor smjera stoga '-' vraca vektor u suprotnom smjeru
}

FVector AGoKart::GetRollingResistance()
{
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;					// GetGravityZ vraca -980.0 jer vraca vrijednosti po UE defaultima
	float NormalForce = Mass * AccelerationDueToGravity;
	return -Velocity.GetSafeNormal() * RollingResistanceCoef * NormalForce;
}

void AGoKart::ApplyRotation(float DeltaTime)
{
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;		// ako ukomponiramo brzinu u jednadzbu onda onemogucimo okretanje u mjestu. DotProduct nam vraca float koji oznacuje
																									// koliko 'velocity' se nalazi u forward vektoru. Ako je velocity u suprotnom smjeru, onda je taj float negativan
																									// sto omogucuje da se krecemo unazad 'prirodnim upravljanjem'
	float RotationAngle = DeltaLocation / MinTurningRadius * SteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity);		// rotiramo 'velocity' vektor za istu vrijednost kojom rotiramo auto

	AddActorWorldRotation(RotationDelta);
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * DeltaTime * 100;		// mnozimo da dobijemo metre u sekundi

	FHitResult OutSweepHitResult;
	AddActorWorldOffset(Translation, true, &OutSweepHitResult);						// postavljamo mu novu lokaciju i postavljamo 'sweep'. Sweep provjerava koliziju prilikom pomicanja

	if (OutSweepHitResult.IsValidBlockingHit())				// nakon što pogodimo zid, brzina auta je još uvijek postavljena i auto ima usmjerenu brzinu u istom smjeru stoga je potrebno resetirati brzinu
	{
		Velocity = FVector(0);
	}
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);			// kad se pritisne MoveForward odnosno 'W'
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

void AGoKart::MoveForward(float Value)
{
	Throttle = Value;
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
}
