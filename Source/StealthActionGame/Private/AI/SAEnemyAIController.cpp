// Copyright 2026, Lisboon. All Rights Reserved.

#include "AI/SAEnemyAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

#if ENABLE_DRAW_DEBUG
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#endif

ASAEnemyAIController:: ASAEnemyAIController()
{
	SightRadius = 1500.0f;
	LoseSightRadius = 2000.0f;
	PeripheralVisionAngle = 90.0f;
	HearingRange = 3000.0f;

	CurrentAlertState = ESAAlertState::Neutral;
	CurrentThreat = nullptr;
	LastKnownThreatLocation = FVector::ZeroVector;

	SuspiciousTimeout = 6.0f;
	AlertSearchDuration = 10.0f;

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngle;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	AIPerceptionComponent->ConfigureSense(*SightConfig);

	UAISenseConfig_Hearing* HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(3.0f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
	AIPerceptionComponent->ConfigureSense(*HearingConfig);

	AIPerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
}

void ASAEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	if (AIPerceptionComponent)
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ASAEnemyAIController::OnPerceptionUpdated);
	}
}

void ASAEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void ASAEnemyAIController::SetAlertState(ESAAlertState NewState)
{
	if (CurrentAlertState == NewState)
	{
		return;
	}

	CurrentAlertState = NewState;

	GetWorldTimerManager().ClearTimer(SuspiciousTimerHandle);
	GetWorldTimerManager().ClearTimer(SearchTimerHandle);

	switch (NewState)
	{
	case ESAAlertState::Suspicious:
		GetWorldTimerManager().SetTimer(
			SuspiciousTimerHandle, this, &ASAEnemyAIController::OnSuspiciousTimeout,
			SuspiciousTimeout, false
		);
		break;

	case ESAAlertState::Searching:
		GetWorldTimerManager().SetTimer(
			SearchTimerHandle, this, &ASAEnemyAIController::OnSearchTimeout,
			AlertSearchDuration, false
		);
		break;

	default:
		break;
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsEnum(FName("AlertState"), static_cast<uint8>(NewState));

		if (NewState == ESAAlertState::Neutral)
		{
			BB->ClearValue(FName("ThreatLocation"));
			BB->ClearValue(FName("ThreatActor"));
		}
	}

#if ENABLE_DRAW_DEBUG
	if (GEngine)
	{
		const FString StateNames[] = { TEXT("NEUTRAL"), TEXT("SUSPICIOUS"), TEXT("ALERT"), TEXT("SEARCHING") };
		const FColor StateColors[] = { FColor::Green, FColor::Yellow, FColor::Red, FColor::Orange };
		const int32 Idx = static_cast<int32>(NewState);

		if (const APawn* ControlledPawn = GetPawn())
		{
			DrawDebugString(GetWorld(), ControlledPawn->GetActorLocation() + FVector(0, 0, 120),
				FString::Printf(TEXT("STATE: %s"), *StateNames[Idx]),
				nullptr, StateColors[Idx], 3.0f);
		}
	}
#endif
}

void ASAEnemyAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	if (const APawn* SensedPawn = Cast<APawn>(Actor))
	{
		if (!SensedPawn->IsPlayerControlled())
		{
			return;
		}
	}
	else
	{
		return;
	}

	if (Stimulus.Type == UAISense_Sight::StaticClass()->GetDefaultObject<UAISense>()->GetSenseID())
	{
		HandleSightStimulus(Actor, Stimulus);
	}
	else if (Stimulus.Type == UAISense_Hearing::StaticClass()->GetDefaultObject<UAISense>()->GetSenseID())
	{
		HandleHearingStimulus(Actor, Stimulus);
	}
}

void ASAEnemyAIController::HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	if (Stimulus.WasSuccessfullySensed())
	{
		CurrentThreat = Actor;
		LastKnownThreatLocation = Actor->GetActorLocation();

		SetAlertState(ESAAlertState::Alert);

		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			BB->SetValueAsObject(FName("ThreatActor"), Actor);
			BB->SetValueAsVector(FName("ThreatLocation"), LastKnownThreatLocation);
		}
	}
	else
	{
		CurrentThreat = nullptr;

		if (CurrentAlertState == ESAAlertState::Alert)
		{
			SetAlertState(ESAAlertState::Searching);
		}

		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			BB->ClearValue(FName("ThreatActor"));
		}
	}
}

void ASAEnemyAIController::HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	if (Stimulus.WasSuccessfullySensed())
	{
		LastKnownThreatLocation = Stimulus.StimulusLocation;

		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			BB->SetValueAsVector(FName("ThreatLocation"), LastKnownThreatLocation);
		}

		if (CurrentAlertState == ESAAlertState::Neutral)
		{
			SetAlertState(ESAAlertState::Suspicious);
		}
		else if (CurrentAlertState == ESAAlertState::Searching)
		{
			GetWorldTimerManager().ClearTimer(SearchTimerHandle);
			GetWorldTimerManager().SetTimer(
				SearchTimerHandle, this, &ASAEnemyAIController::OnSearchTimeout,
				AlertSearchDuration, false
			);
		}
	}
}

void ASAEnemyAIController::OnSuspiciousTimeout()
{
	if (CurrentAlertState == ESAAlertState::Suspicious)
	{
		SetAlertState(ESAAlertState::Neutral);
	}
}

void ASAEnemyAIController::OnSearchTimeout()
{
	if (CurrentAlertState == ESAAlertState::Searching)
	{
		SetAlertState(ESAAlertState::Neutral);
	}
}
