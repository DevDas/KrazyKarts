// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"


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

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(FName("MovementComponent"));
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

	if (MovementComponent == nullptr) return;

	if (Role == ROLE_AutonomousProxy)
	{
		FGoKartMoves Move = MovementComponent->CreateMove(DeltaTime);
		MovementComponent->SimulateMove(Move); // Simulate Move Myself
		UnacknowledgedMoves.Add(Move);
		Server_SendMove(Move);	
		
		UE_LOG(LogTemp, Warning, TEXT("Queue length: %d"), UnacknowledgedMoves.Num())
	}

	// We Are The Server And In Control Of The Pawn
	if (Role == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy /*IsLocallyControlled()*/)
	{
		FGoKartMoves Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	if (Role == ROLE_SimulatedProxy)
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}

	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(Role), this, FColor::Red, DeltaTime);
}

void AGoKart::OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgeMoves(ServerState.LastMove);

	for (const FGoKartMoves& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
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

void AGoKart::MoveForward(float Value)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SetThrottle(Value);
}

void AGoKart::MoveRight(float Value)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SetSteeringThrow(Value);
}

void AGoKart::Server_SendMove_Implementation(FGoKartMoves Move)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SimulateMove(Move);

	ServerState.LastMove = Move;  // FGoKartMoves 
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity(); // Local Velocity Is Now ServerState Velocity
}

bool AGoKart::Server_SendMove_Validate(FGoKartMoves Move)
{
	//return FMath::Abs(Value) <= 1; // if False Client Will Remove From The Server
	return true; // TODO Better validation
}