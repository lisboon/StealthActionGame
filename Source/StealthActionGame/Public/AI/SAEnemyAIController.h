// Copyright 2026, Lisboon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "AI/SAAlertState.h"
#include "SAEnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UBehaviorTree;

UCLASS()
class STEALTHACTIONGAME_API ASAEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
public:	
	ASAEnemyAIController();
	
	// === Alert State ===
	UFUNCTION(BlueprintPure, Category = "AI|State")
	ESAAlertState GetAlertState() const { return CurrentAlertState; }
	
	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void SetAlertState(ESAAlertState NewState);
	
	// === Threat Info ===
	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	FVector GetLastKnownThreatLocation() const { return LastKnownThreatLocation; }
	
	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	AActor* GetCurrentThreat() const { return CurrentThreat; }
	
	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	bool HasCurrentThreat() const { return CurrentThreat != nullptr; }
	
	
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* inPawn) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
	
	// === Perception Tuning ===
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Sight")
	float SightRadius;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Sight")
	float LoseSightRadius;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Sight")
	float PeripheralVisionAngle;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Hearing")
	float HearingRange;
	
	// === Alert Timers ===
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|State")
	float SuspiciousTimeout;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|State")
	float AlertSearchDuration;
	
private:
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	void HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus);
	void HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus);
	void OnSuspiciousTimeout(); 
	void OnSearchTimeout();
	
	UPROPERTY()
	ESAAlertState CurrentAlertState;
	
	UPROPERTY()
	TObjectPtr<AActor> CurrentThreat;
	
	FVector LastKnownThreatLocation;
	
	FTimerHandle SuspiciousTimerHandle;
	FTimerHandle SearchTimerHandle;
	
};
