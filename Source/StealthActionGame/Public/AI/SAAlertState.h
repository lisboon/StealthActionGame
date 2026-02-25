// Copyright 2026, Lisboon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SAAlertState.generated.h"

UENUM(BlueprintType)
enum class ESAAlertState : uint8
{
	Neutral		UMETA(DisplayName = "Neutral"),
	Suspicious	UMETA(DisplayName = "Suspicious"),
	Alert		UMETA(DisplayName = "Alert"),
	Searching	UMETA(DisplayName = "Searching")
};
