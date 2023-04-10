// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PC_VoiceChat.generated.h"

/**
 * 
 */
UCLASS()
class APC_VoiceChat : public APlayerController
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Vivox | Player")
	void DisableMuteWidgets();
};
