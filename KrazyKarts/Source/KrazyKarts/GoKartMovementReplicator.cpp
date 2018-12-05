// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementReplicator.h"
#include "UnrealNetwork.h"

// Sets default values for this component's properties
UGoKartMovementReplicator::UGoKartMovementReplicator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}


// Called when the game starts
void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}


// Called every frame
void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MovementComponent == nullptr) return;

	if (GetOwnerRole() == ROLE_AutonomousProxy)		// ako smo klijent,tj. autonomous proxy
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);	// kreiraj pokret
		MovementComponent->SimulateMove(Move);							// simuliraj pokret lokalno (da ga i korisnik moze vidjeti) izmedu stanja pokreta dobivenih sa servera

		UnacknowledgedMoves.Add(Move);				// pokret dodaj u queue koji ce provjeravati koji pokreti su izvrseni, a koji ne (u slucaju laga)
		Server_SendMove(Move);						// posalji taj isti 'Move' serveru na izvrsavanje. On ce ga izvrsiti i vratiti nam stanje transformacije koje cemo mi onda postaviti na klijentu
	}
	if (GetOwnerRole() == ROLE_Authority && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)				// zelimo slucaj kada RemoteRole nije autonomous proxy. Onda to znaci da smo mi server i kontroliramo Pawn-a
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);				// iako se radi o serveru, svejedno salje pokret serveru (sam sebi) i tamo ga simulira te transformaciju salje svim autonomous proxy 
	}
	if (GetOwnerRole() == ROLE_SimulatedProxy)		// ako smo simulated proxy
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const		// ne mijenjaj ime argumenta
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);		// potrebno je pozvati 'Super' metodu kako bi ispravno radili ispisi actor roles
	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);						// ovaj makro prihvaca varijablu za replikaciju. Replicira 'ServerState'; kad god se ta varijabla postavi na serveru, svi klijenti ce vidjeti tu vrijednost
																// i moci postaviti svoju vrijednost transformacije na tu vrijednost
}

void UGoKartMovementReplicator::OnRep_ServerState()				// ovo ce se pozvati samo na klijentu kada je doslo do promjene stoga ne treba provjeravati 'HasAuthority()'
{
	if (MovementComponent == nullptr) return;

	GetOwner()->SetActorTransform(ServerState.Transform);				// postavi transformaciju klijenta prema onoj koja se nalazi na serveru
	MovementComponent->SetVelocity(ServerState.Velocity);						// Resetiramo 'Velocity' na klijentu na sto god server kaze da je tocno

	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);						// simuliraj sve pokrete koji se jos 'nisu dogodili'
	}
}

void UGoKartMovementReplicator::ClearAcknowledgedMoves(FGoKartMove LastMove)
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

void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMove Move)		// Na serveru se zapravo izvrsava ova funkcija post-fixana sa _Implementation(ne Server_MoveForward). Salju se komponente 'Move' strukture koje ce
{																	// na serveru simulirati kretanje
	if (MovementComponent == nullptr) return;

	MovementComponent->SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();					// postavljamo transformaciju actora na serveru svaki frame i omogucavamo ju za replikaciju
	ServerState.Velocity = MovementComponent->GetVelocity();
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Move)			// potrebno za anti-cheat
{
	return true;								// TODO; make better validation
}