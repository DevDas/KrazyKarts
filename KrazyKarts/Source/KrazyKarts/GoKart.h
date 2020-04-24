// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

USTRUCT()
struct FGoKartMoves
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float Time; // Time At Which The Move Started
};

USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	FVector Velocity;

	FGoKartMoves LastMove;
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
	void SimulateMove(FGoKartMoves Move);

	void CreateAcknowledgeMoves(FGoKartMoves LastMove);

	FGoKartMoves CreateMove(float DeltaTime);

	FVector GetAirResistance();

	FVector GetRollingResistance();

	void ApplyRotation(float DeltaTime, float SteeringToThrow);

	void UpdateLocationFromVelocity(float DeltaTime);

	//=========================================================================================

	// All Properties Inside The Struct Are Now Replicated
	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	UFUNCTION()
	void OnRep_ServerState();

	void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const;

	void MoveForward(float Value);
	void MoveRight(float Value);

	// RPC - Anything Client Can Do Server Is Watching
	// Now This Gonna Be Executed On Server not On Client ***RPC***
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMoves Move);

	bool Server_SendMove_Validate(FGoKartMoves Move);

	//==========================================================================================

	// The Mass Of The Car(kg)
	UPROPERTY(EditAnywhere)
	float Mass = 1000;

	// Min Radius of the car Turning Circle at full lock(m)
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10;

	// Higher Means More Drag
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16;

	// Higher Means More Rolling Resistance
	UPROPERTY(EditAnywhere)
	float RollingCoefficient = 0.015;

	// Local Velocity
	FVector Velocity;

	// Local
	float Throttle;
	float SteeringThrow;

	// The Force Applied To The Car When The Throttle is Fully Down (Newton)
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;

	TArray<FGoKartMoves> UnacknowledgedMoves;
};
