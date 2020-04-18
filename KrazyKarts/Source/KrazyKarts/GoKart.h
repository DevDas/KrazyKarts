// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

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
	FVector GetAirResistance();

	FVector GetRollingResistance();

	void ApplyRotation(float DeltaTime);

	void UpdateLocationFromVelocity(float DeltaTime);

	FVector Velocity;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveForward(float Value); // Now This Gonna Be Executed On Server

	bool Server_MoveForward_Validate(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveRight(float Value);

	bool Server_MoveRight_Validate(float Value);

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

	float Throttle;

	float SteeringThrow;

	// The Force Applied To The Car When The Throttle is Fully Down (Newton)
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;
};
