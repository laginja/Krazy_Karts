// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicator.generated.h"

USTRUCT()							// STRUCT() jer se mogu jednostavno prenositi preko mreze
struct FGoKartState					// bolje je sve komponente 'stanja' actora spremiti u strukturu koja se replicira. Na taj nacini prilikom promjene jedne varijable, cijela se struktura updatea
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGoKartMove LastMove;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FTransform Transform;
};

struct FHermiteCubicSpline
{
	FVector StartLocation, StartDerivative, TargetLocation, TargetDerivative;

	FVector InterpolateLocation(float LerpRatio) const		// const da damo do znanja da ne funkcija ne mijenja  "void InterpolateLocation(const FHermiteCubicSpline &Spline, float LerpRatio);"
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);	// interpolira lokaciju izmedu dvije lokacije: trenutne i tamo gdje bi trebao biti
	}

	FVector InterpolateDerivative(float LerpRatio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio); // vraca derivaciju u toj tocki na krivulji
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementReplicator : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementReplicator();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ClearAcknowledgedMoves(FGoKartMove LastMove);
	
	void UpdateServerState(const FGoKartMove& Move);

	void ClientTick(float DeltaTime);

	FHermiteCubicSpline CreateSpline();
	void InterpolateLocation(const FHermiteCubicSpline &Spline, float LerpRatio);
	void InterpolateVelocity(const FHermiteCubicSpline &Spline, float LerpRatio);
	void InterpolateRotation(float LerpRatio);
	float VelocityToDerivative();

	UFUNCTION(Server, Reliable, WithValidation)			// RPC funkcija (Remote Procedure Call) - poziva se na klijentu, a izvršava se na serveru. Svi RPCs su unreliable po pitanju izvrsavanja, 				
	void Server_SendMove(FGoKartMove Move);				// stoga ako zelimo da se funckija izvrsi dodamo 'Reliable'. Pre-fixed sa Server_ jer se izvrsava na serveru.

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)		// 'ReplicatedUsing' omogucava da se struktura moze replicirati sa servera i da se sazna kada je doslo do promjene te varijable na serveru te se pri
	FGoKartState ServerState;							// promjeni strukture poziva zadana funkcija (u ovom slucaju: OnRep_ServerState). U toj funkciji cemo updateati transformaciju client actora 

	UFUNCTION()											// mora biti UFUNCTION()
	void OnRep_ServerState();
	void AutonomousProxy_OnRep_ServerState();
	void SimulatedProxy_OnRep_ServerState();

	TArray<FGoKartMove> UnacknowledgedMoves;

	float ClientTimeSinceUpdate;
	float ClientTimeBetweenLastUpdates;
	FTransform ClientStartTransform;
	FVector ClientStartVelocity;

	float ClientSimulatedTime;

	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;

	UPROPERTY()
	USceneComponent* MeshOffsetRoot;					// Koristimo scene component kako bi dobili tocnu koliziju. Na taj nacin, ako se na serveru ne dogodi kolizija sa cunjevima, nece ni na klijentu
	UFUNCTION(BlueprintCallable)
	void SetMeshOffsetRoot(USceneComponent* Root) { MeshOffsetRoot = Root; }
};
