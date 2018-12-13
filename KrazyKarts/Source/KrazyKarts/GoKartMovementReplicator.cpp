// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementReplicator.h"
#include "UnrealNetwork.h"
#include "GameFramework/Actor.h"

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
	
	FGoKartMove LastMove = MovementComponent->GetLastMove();

	if (GetOwnerRole() == ROLE_AutonomousProxy)		// ako smo klijent,tj. autonomous proxy
	{
		UnacknowledgedMoves.Add(LastMove);				// pokret dodaj u queue koji ce provjeravati koji pokreti su izvrseni, a koji ne (u slucaju laga)
		Server_SendMove(LastMove);						// posalji taj isti 'Move' serveru na izvrsavanje. On ce ga izvrsiti i vratiti nam stanje transformacije koje cemo mi onda postaviti na klijentu
	}
	if (GetOwnerRole() == ROLE_Authority && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)		// zelimo slucaj kada RemoteRole nije autonomous proxy. Onda to znaci da smo mi server i kontroliramo Pawn-a
	{																								// Moguce izbaciti 'GetOwnerRole() == ROLE_Authority' dio
		UpdateServerState(LastMove);
	}
	if (GetOwnerRole() == ROLE_SimulatedProxy)		// ako smo simulated proxy
	{
		ClientTick(DeltaTime);
	}
}

void UGoKartMovementReplicator::UpdateServerState(const FGoKartMove& Move)
{
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();					// postavljamo transformaciju actora na serveru svaki frame i omogucavamo ju za replikaciju
	ServerState.Velocity = MovementComponent->GetVelocity();
}

void UGoKartMovementReplicator::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;
	if (MovementComponent == nullptr) return;

	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
	FHermiteCubicSpline Spline = CreateSpline(); 
	
	InterpolateLocation(Spline, LerpRatio);

	InterpolateVelocity(Spline, LerpRatio);

	InterpolateRotation(LerpRatio);
}

FHermiteCubicSpline UGoKartMovementReplicator::CreateSpline()
{
	FHermiteCubicSpline Spline;
	Spline.TargetLocation = ServerState.Transform.GetLocation();
	Spline.StartLocation = ClientStartTransform.GetLocation();
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();

	return Spline;
}

void UGoKartMovementReplicator::InterpolateLocation(const FHermiteCubicSpline &Spline, float LerpRatio)
{
	FVector NewLocation = Spline.InterpolateLocation(LerpRatio);

	if (MeshOffsetRoot != nullptr)
	{
		MeshOffsetRoot->SetWorldLocation(NewLocation);
	}
}

void UGoKartMovementReplicator::InterpolateVelocity(const FHermiteCubicSpline &Spline, float LerpRatio)
{
	FVector NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative();
	MovementComponent->SetVelocity(NewVelocity);
}

void UGoKartMovementReplicator::InterpolateRotation(float LerpRatio)
{
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();

	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);			// interpolira rotaciju izmedu dvije rotacije: trenutne i kako bi trebao biti rotiran
	
	if (MeshOffsetRoot != nullptr)
	{
		MeshOffsetRoot->SetWorldRotation(NewRotation);
	}
}

float UGoKartMovementReplicator::VelocityToDerivative()
{
	return ClientTimeBetweenLastUpdates * 100;	// * 100 da se prebaci u metre ;
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const		// ne mijenjaj ime argumenta
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);		// potrebno je pozvati 'Super' metodu kako bi ispravno radili ispisi actor roles
	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);						// ovaj makro prihvaca varijablu za replikaciju. Replicira 'ServerState'; kad god se ta varijabla postavi na serveru, svi klijenti ce vidjeti tu vrijednost
																// i moci postaviti svoju vrijednost transformacije na tu vrijednost
}

void UGoKartMovementReplicator::OnRep_ServerState()				// ovo ce se pozvati samo na klijentu kada je doslo do promjene stoga ne treba provjeravati 'HasAuthority()'
{
	switch (GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	case ROLE_SimulatedProxy:
		SimulatedProxy_OnRep_ServerState();
		break;
	default:
		break;
	}
}

void UGoKartMovementReplicator::AutonomousProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	GetOwner()->SetActorTransform(ServerState.Transform);				// postavi transformaciju klijenta prema onoj koja se nalazi na serveru
	MovementComponent->SetVelocity(ServerState.Velocity);				// Resetiramo 'Velocity' na klijentu na sto god server kaze da je tocno

	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);						// simuliraj sve pokrete koji se jos 'nisu dogodili'
	}
}

void UGoKartMovementReplicator::SimulatedProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	if (MeshOffsetRoot != nullptr)
	{
		ClientStartTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());		// postavljamo lokaciju od MeshOffset-a radi uklanjanja fantomskih kolizija na klijentu
		ClientStartTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());			// isto i za rotaciju
	}
	
	ClientStartVelocity = MovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Transform);
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

	ClientSimulatedTime += Move.DeltaTime;
	MovementComponent->SimulateMove(Move);

	UpdateServerState(Move);
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Move)			// potrebno za anti-cheat
{
	float ProposedTime = ClientSimulatedTime + Move.DeltaTime;
	bool ClientNotRunningAhead = ProposedTime < GetWorld()->TimeSeconds;
	if (!ClientNotRunningAhead)														// sprjecava da klijent ide "prebrzo" u smislu da mu vrijeme drugacije prolazi
	{
		UE_LOG(LogTemp, Error, TEXT("Client is running too fast"))
		return false;
	}
	if (!Move.IsValid())															// sprjecava namjestanje prevelikih parametara pokreta (Throttle i SteeringThrow)
	{
		UE_LOG(LogTemp, Error, TEXT("Recieved invalid move"))
		return false;
	}
	return true;															
}