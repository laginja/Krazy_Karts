// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"

#include "Components/InputComponent.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
}

void AGoKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const		// ne mijenjaj ime argumenta
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);		// potrebno je pozvati 'Super' metodu kako bi ispravno radili ispisi actor roles
	DOREPLIFETIME(AGoKart, ServerState);						// ovaj makro prihvaca varijablu za replikaciju. Replicira 'ServerState'; kad god se ta varijabla postavi na serveru, svi klijenti ce vidjeti tu vrijednost
																// i moci postaviti svoju vrijednost transformacije na tu vrijednost
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
	
	if (IsLocallyControlled())
	{
		FGoKartMove Move = CreateMove(DeltaTime);
		if (!HasAuthority())					// nema potrebe dodavati pokrete u queue ako se radi o serveru
		{
			UnacknowledgedMoves.Add(Move);
			UE_LOG(LogTemp, Warning, TEXT("Queue Length: %d"), UnacknowledgedMoves.Num());
		}

		Server_SendMove(Move);				// saljemo serveru nas konstruirani 'Move'

		SimulateMove(Move);					// moramo pozvati i lokalno inace se ne simuliraju pokreti nego kart mijenja stanja prema stanjima dobivenih iz OnRep_ServerState()	
	}

	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(Role), this, FColor::White, DeltaTime);
}

void AGoKart::OnRep_ServerState()				// ovo ce se pozvati samo na klijentu kada je doslo do promjene stoga ne treba provjeravati 'HasAuthority()'
{
	SetActorTransform(ServerState.Transform);				// postavi transformaciju klijenta prema onoj koja se nalazi na serveru
	Velocity = ServerState.Velocity;						// Resetiramo 'Velocity' na klijentu na sto god server kaze da je tocno
	ClearAcknowledgedMoves(ServerState.LastMove);
}

void AGoKart::SimulateMove(FGoKartMove Move)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	UpdateLocationFromVelocity(Move.DeltaTime);				// ideja je da se svaki frame: na klijentu simulira stvarna transformacija i na serveru ista postavlja. Kada dode do promjene na serveru (sto se dogada pozivom 
															// OnRep_ReplicatedTransform()) mijenja se transformacija na klijentu prema onoj koja je zabiljezena na serveru.
}

FGoKartMove AGoKart::CreateMove(float DeltaTime)
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->TimeSeconds;

	return Move;
}

void AGoKart::ClearAcknowledgedMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if (Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}

	UnacknowledgedMoves = NewMoves;
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

void AGoKart::ApplyRotation(float DeltaTime, float SteeringThrow)
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
	FVector Translation = Velocity * DeltaTime * 100;		// mnozimo sa 100 da dobijemo metre u sekundi

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
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
	
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)		// Na serveru se zapravo izvrsava ova funkcija post-fixana sa _Implementation(ne Server_MoveForward). Salju se komponente 'Move' strukture koje ce
{																	// na serveru simulirati kretanje
	SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();					// postavljamo transformaciju actora na serveru svaki frame i omogucavamo ju za replikaciju
	ServerState.Velocity = Velocity;						
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)			// potrebno za anti-cheat
{
	return true;								// TODO; make better validation
}

