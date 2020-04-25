// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"

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
		NetUpdateFrequency = 1.f;
	}
}

void AGoKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoKart, ServerState); // ServerState Included Valocity And Transform And FGoKartMoves
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Role == ROLE_AutonomousProxy)
	{
		FGoKartMoves Move = CreateMove(DeltaTime);
		SimulateMove(Move); // Simulate Move Myself
		UnacknowledgedMoves.Add(Move);
		Server_SendMove(Move);	
	}

	// We Are The Server And In Control Of The Pawn
	if (Role == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy /*IsLocallyControlled()*/)
	{
		FGoKartMoves Move = CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	if (Role == ROLE_SimulatedProxy)
	{
		SimulateMove(ServerState.LastMove);
	}

	UE_LOG(LogTemp, Warning, TEXT("Queue length: %d"), UnacknowledgedMoves.Num())
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(Role), this, FColor::Red, DeltaTime);
}

void AGoKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;

	ClearAcknowledgeMoves(ServerState.LastMove);

	for (const FGoKartMoves& Move : UnacknowledgedMoves)
	{
		SimulateMove(Move);
	}
}

// All Data Comming Throw The Move not Directly From The Actor
void AGoKart::SimulateMove(const FGoKartMoves& Move)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	// Moving The Car
	UpdateLocationFromVelocity(Move.DeltaTime);
}

void AGoKart::ClearAcknowledgeMoves(FGoKartMoves LastMove)
{
	TArray<FGoKartMoves> NewMoves;
	for (const FGoKartMoves& Move : UnacknowledgedMoves)
	{
		if (Move.Time < LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}
	UnacknowledgedMoves = NewMoves;
}

FGoKartMoves AGoKart::CreateMove(float DeltaTime)
{
	FGoKartMoves Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	return Move;
}

FVector AGoKart::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollingResistance()
{
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	float NormalForce = Mass * AccelerationDueToGravity;
	return -Velocity.GetSafeNormal() * RollingCoefficient * NormalForce;
}

void AGoKart::ApplyRotation(float DeltaTime, float SteeringToThrow)
{
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * SteeringToThrow;
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity); // Rotating Velocity

	AddActorWorldRotation(RotationDelta); // Rotating Actor
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * 100 * DeltaTime; // m/s * s = m // 100  Because Meters to Centimeters

	FHitResult HitResult;

	AddActorWorldOffset(Translation, true, &HitResult);

	UE_LOG(LogTemp, Error, TEXT("Sped : %f"), Translation.Size())
	
	if (HitResult.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

void AGoKart::MoveForward(float Value)
{
	Throttle = Value;
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
}

void AGoKart::Server_SendMove_Implementation(FGoKartMoves Move)
{
	SimulateMove(Move);

	ServerState.LastMove = Move;  // FGoKartMoves 
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity; // Local Velocity Is Now ServerState Velocity
}

bool AGoKart::Server_SendMove_Validate(FGoKartMoves Move)
{
	//return FMath::Abs(Value) <= 1; // if False Client Will Remove From The Server
	return true; // TODO Better validation
}