// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ServerRow.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class SESSIONTEST_API UServerRow : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	
public:
	void SetServerName(FString arg_ServerName);
	void SetServerID(FString arg_ServerID);
	

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SessionTest | ServerRow")
	FText GetServerName();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SessionTest | ServerRow")
	FText GetServerID();

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* ServerName;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* ServerID;
};
