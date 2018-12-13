// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementComponent.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UGoKartMovementComponent::UGoKartMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGoKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwnerRole() == ROLE_AutonomousProxy || GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		LastMove = CreateMove(DeltaTime);				// kreiraj pokret
		SimulateMove(LastMove);							// simuliraj pokret lokalno (da ga i korisnik moze vidjeti i server (ako je korisnik server)) izmedu stanja pokreta dobivenih sa servera
	}
	
}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& Move)
{
	FVector Force = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	UpdateLocationFromVelocity(Move.DeltaTime);				// ideja je da se svaki frame: na klijentu simulira stvarna transformacija i na serveru ista postavlja. Kada dode do promjene na serveru (sto se dogada pozivom 
															// OnRep_ReplicatedTransform()) mijenja se transformacija na klijentu prema onoj koja je zabiljezena na serveru.
}

FGoKartMove UGoKartMovementComponent::CreateMove(float DeltaTime)
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->TimeSeconds;

	return Move;
}

FVector UGoKartMovementComponent::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoef;		// GetSafeNormal() vraca vektor smjera stoga '-' vraca vektor u suprotnom smjeru
}

FVector UGoKartMovementComponent::GetRollingResistance()
{
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;					// GetGravityZ vraca -980.0 jer vraca vrijednosti po UE defaultima
	float NormalForce = Mass * AccelerationDueToGravity;

	return -Velocity.GetSafeNormal() * RollingResistanceCoef * NormalForce;
}

void UGoKartMovementComponent::ApplyRotation(float DeltaTime, float SteeringThrow)
{
	float DeltaLocation = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * DeltaTime;		// ako ukomponiramo brzinu u jednadzbu onda onemogucimo okretanje u mjestu. DotProduct nam vraca float koji oznacuje
																									// koliko 'velocity' se nalazi u forward vektoru. Ako je velocity u suprotnom smjeru, onda je taj float negativan
																									// sto omogucuje da se krecemo unazad 'prirodnim upravljanjem'
	float RotationAngle = DeltaLocation / MinTurningRadius * SteeringThrow;
	FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity);		// rotiramo 'velocity' vektor za istu vrijednost kojom rotiramo auto

	GetOwner()->AddActorWorldRotation(RotationDelta);
}

void UGoKartMovementComponent::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * DeltaTime * 100;		// mnozimo sa 100 da dobijemo metre u sekundi

	FHitResult OutSweepHitResult;
	GetOwner()->AddActorWorldOffset(Translation, true, &OutSweepHitResult);						// postavljamo mu novu lokaciju i postavljamo 'sweep'. Sweep provjerava koliziju prilikom pomicanja

	if (OutSweepHitResult.IsValidBlockingHit())				// nakon što pogodimo zid, brzina auta je još uvijek postavljena i auto ima usmjerenu brzinu u istom smjeru stoga je potrebno resetirati brzinu
	{
		Velocity = FVector(0);
	}
}

