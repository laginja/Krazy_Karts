// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"

#include "Components/InputComponent.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"

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

FString GetEnumText(ENetRole Role)
{
	switch (Role)
	{
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "AutonomousProxy";
	case ROLE_Authority:
		return "Authority";
	default:
		return "Error";
	}
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

	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(Role), this, FColor::White, DeltaTime);
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

// prvo updateamo poziciju lokalno
void AGoKart::MoveForward(float Value)
{
	Throttle = Value;
	Server_MoveForward(Value);									// zatim updateamo poziciju na serveru
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
	Server_MoveRight(Value);
}

void AGoKart::Server_MoveForward_Implementation(float Value)	// Na serveru se zapravo izvrsava ova funkcija post-fixana sa _Implementation(ne Server_MoveForward).
{
	Throttle = Value;
}

bool AGoKart::Server_MoveForward_Validate(float Value)			// potrebno za anti-cheat
{
	return FMath::Abs(Value) <= 1;								// vrati true samo ako je vrijednost gasa izmedu 0 i 1. Sve ostalo je ne moguce i smatra se varanjem
}

void AGoKart::Server_MoveRight_Implementation(float Value)
{
	SteeringThrow = Value;
}

bool AGoKart::Server_MoveRight_Validate(float Value)		
{
	return FMath::Abs(Value) <= 1;
}