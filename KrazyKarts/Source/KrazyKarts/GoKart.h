// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GoKartMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"


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
	void ClearAcknowledgeMoves(FGoKartMoves LastMove);

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

	TArray<FGoKartMoves> UnacknowledgedMoves;

	UPROPERTY(EditAnywhere)
	UGoKartMovementComponent* MovementComponent;
};
