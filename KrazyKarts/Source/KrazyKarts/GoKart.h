// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

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
	void SimulateMove(FGoKartMove Move);

	FGoKartMove CreateMove(float DeltaTime);
	void ClearAcknowledgedMoves(FGoKartMove LastMove);

	FVector GetAirResistance();
	FVector GetRollingResistance();

	void ApplyRotation(float DeltaTime, float SteeringThrow);
	void UpdateLocationFromVelocity(float DeltaTime);

	void MoveForward(float Value);
	void MoveRight(float Value);

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

	UFUNCTION(Server, Reliable, WithValidation)			// RPC funkcija (Remote Procedure Call) - poziva se na klijentu, a izvršava se na serveru. Svi RPCs su unreliable po pitanju izvrsavanja, 				
	void Server_SendMove(FGoKartMove Move);				// stoga ako zelimo da se funckija izvrsi dodamo 'Reliable'. Pre-fixed sa Server_ jer se izvrsava na serveru.

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)		// 'ReplicatedUsing' omogucava da se struktura moze replicirati sa servera i da se sazna kada je doslo do promjene te varijable na serveru te se pri
	FGoKartState ServerState;							// promjeni strukture poziva zadana funkcija (u ovom slucaju: OnRep_ServerState). U toj funkciji cemo updateati transformaciju client actora 

	FVector Velocity;
							
	UFUNCTION()														// mora biti UFUNCTION()
	void OnRep_ServerState();

	float Throttle;
	float SteeringThrow;

	TArray<FGoKartMove> UnacknowledgedMoves;
};
