// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementReplicator.h"
#include "GoKart.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UGoKartMovementReplicator::UGoKartMovementReplicator()
{
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

	FGoKartMoves LastMove = MovementComponent->GetLastMove();

	// RemoteRole = Server Or Client Role In Another Screen
	// Client ItSelf
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowledgedMoves.Add(LastMove);
		Server_SendMove(LastMove); // Telling THe Server That Client Is Moving

		UE_LOG(LogTemp, Warning, TEXT("Queue length: %d"), UnacknowledgedMoves.Num())
	}

	// We Are The Server And In Control Of The Pawn (Server in client)
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy) /*IsLocallyControlled()*/
	{
		UpdateServerState(LastMove); // Server (Authority) Updating
	}

	// Let The Client See What Server And 2nd Client Is Doing
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ClientTick(DeltaTime);
	}
}

// Replication Process
void UGoKartMovementReplicator::UpdateServerState(const FGoKartMoves& Move)
{
	ServerState.LastMove = Move;  // FGoKartMoves 
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity(); // Local Velocity Is Now ServerState Velocity
}

void UGoKartMovementReplicator::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;
	if (MovementComponent == nullptr) return;

	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
	
	FHermiteCubicSpine Spline = CreateSpline();

	InterpolateLocation(Spline, LerpRatio);
	InterpolateVelocity(Spline, LerpRatio);
	InterpolateRotation(LerpRatio);
}

void UGoKartMovementReplicator::InterpolateRotation(float LerpRatio)
{
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();
	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);
	GetOwner()->SetActorRotation(NewRotation);
}

void UGoKartMovementReplicator::InterpolateVelocity(const FHermiteCubicSpine& Spline, float LerpRatio)
{
	FVector NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative();
	MovementComponent->SetVelocity(NewVelocity);
}

FHermiteCubicSpine UGoKartMovementReplicator::CreateSpline()
{
	FHermiteCubicSpine Spline;
	Spline.TargetLocation = ServerState.Transform.GetLocation();
	Spline.StartLocation = ClientStartTransform.GetLocation();
	Spline.StartDerivative = StartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();

	return Spline;
}

void UGoKartMovementReplicator::InterpolateLocation(const FHermiteCubicSpine& Spline, float LerpRatio)
{
	FVector NewLocation = Spline.InterpolateLocaton(LerpRatio);
	GetOwner()->SetActorLocation(NewLocation);
}

float UGoKartMovementReplicator::VelocityToDerivative()
{
	return ClientTimeBetweenLastUpdates * 100; // meter to cm
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartMovementReplicator, ServerState); // ServerState Included Valocity And Transform And FGoKartMoves
}

void UGoKartMovementReplicator::OnRep_ServerState()
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

// Updating 2nd Client In First Client Or Vice Versa
void UGoKartMovementReplicator::AutonomousProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgeMoves(ServerState.LastMove);

	for (const FGoKartMoves& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

// All Simulated Proxy
void UGoKartMovementReplicator::SimulatedProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	ClientStartTransform = GetOwner()->GetActorTransform();
	StartVelocity = MovementComponent->GetVelocity();
}

void UGoKartMovementReplicator::ClearAcknowledgeMoves(FGoKartMoves LastMove)
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

void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMoves Move)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SimulateMove(Move); // Tell The Server That The Client Is Moving
	
	UpdateServerState(Move); // Tell The 2nd Client That 1st Client Is Moving
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMoves Move)
{
	//return FMath::Abs(Value) <= 1; // if False Client Will Remove From The Server
	return true; // TODO Better validation
}