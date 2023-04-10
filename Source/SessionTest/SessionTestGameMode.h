// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "GameServerApi/AccelByteServerMatchmakingApi.h"

#include "SessionTestGameMode.generated.h"

UCLASS(minimalapi)
class ASessionTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASessionTestGameMode();

#pragma region GAME MODE EVENTS

protected:
	bool bHasMatchmakingData = false;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	void OnError(int32 Code, const FString& Message);
#pragma endregion

public: //@todo make it non public later
	void ServerRequestServerInfo();
	void ServerRequestSessionData();

	UPROPERTY(VisibleAnywhere, Category = "Accelbyte | Server")
	FString UserID;

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Server")
	void RegisterNewPlayer(FString arg_UserId);

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Server")
	void DeregisterPlayer(FString arg_UserId);

	FAccelByteModelsServerInfo ServerInfo;

	//! matchmaking data model. will be empty if dedicated server is spawned, but not yet claimed (such as in server buffer scenario)
	FAccelByteModelsMatchmakingResult MatchmakingData;
};



