// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicator.generated.h"

USTRUCT()
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
	
	UFUNCTION(Server, Reliable, WithValidation)			// RPC funkcija (Remote Procedure Call) - poziva se na klijentu, a izvršava se na serveru. Svi RPCs su unreliable po pitanju izvrsavanja, 				
	void Server_SendMove(FGoKartMove Move);				// stoga ako zelimo da se funckija izvrsi dodamo 'Reliable'. Pre-fixed sa Server_ jer se izvrsava na serveru.

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)		// 'ReplicatedUsing' omogucava da se struktura moze replicirati sa servera i da se sazna kada je doslo do promjene te varijable na serveru te se pri
	FGoKartState ServerState;							// promjeni strukture poziva zadana funkcija (u ovom slucaju: OnRep_ServerState). U toj funkciji cemo updateati transformaciju client actora 

	UFUNCTION()											// mora biti UFUNCTION()
	void OnRep_ServerState();

	TArray<FGoKartMove> UnacknowledgedMoves;

	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;
};
