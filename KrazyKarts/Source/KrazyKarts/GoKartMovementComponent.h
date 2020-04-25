// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FGoKartMoves CreateMove(float DeltaTime);

	void SimulateMove(const FGoKartMoves& Move);

	FVector GetVelocity() { return Velocity; };
	void SetVelocity(FVector Val) { Velocity = Val; };

	void SetThrottle(float Val) { Throttle = Val; };
	void SetSteeringThrow(float Val) { SteeringThrow = Val; };

private:

	FVector GetAirResistance();
	FVector GetRollingResistance();

	void ApplyRotation(float DeltaTime, float SteeringToThrow);
	void UpdateLocationFromVelocity(float DeltaTime);

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

	// The Force Applied To The Car When The Throttle is Fully Down (Newton)
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;

	// Local Velocity
	FVector Velocity;

	// Local
	float Throttle;
	float SteeringThrow;	
};
