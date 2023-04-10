// Copyright Epic Games, Inc. All Rights Reserved.

#include "SessionTestGameMode.h"
#include "SessionTestCharacter.h"
#include "UObject/ConstructorHelpers.h"

#include "GameServerApi/AccelByteServerSessionBrowserApi.h"

#include "Core/AccelByteRegistry.h"
#include "GameServerApi/AccelByteServerDSMApi.h"

ASessionTestGameMode::ASessionTestGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void ASessionTestGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	if (IsRunningDedicatedServer())
	{
		ServerRequestSessionData();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Client detected, will not request session data."));
	}

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

void ASessionTestGameMode::ServerRequestServerInfo()
{
	UE_LOG(LogTemp, Log, TEXT("Requesting server info..."));

	FRegistry::ServerDSM.GetServerInfo(
		THandler<const FAccelByteModelsServerInfo&>::CreateWeakLambda(this, [](const FAccelByteModelsServerInfo& ServerInfoReturn)
		{
			UE_LOG(LogTemp, Log, TEXT("Server info retrieved Ip[%s] Port[%i]"), *ServerInfoReturn.Ip, ServerInfoReturn.Port);
			// UniqueGameID/MatchID/SessionID will only be valid after the server is claimed
		}),
		FErrorHandler::CreateUObject(this, &ASessionTestGameMode::OnError)
			);
}

void ASessionTestGameMode::ServerRequestSessionData()
{
	if (!bHasMatchmakingData || true) //@TODO test
	{
		UE_LOG(LogTemp, Log, TEXT("Requesting session data for the first time"));

		bHasMatchmakingData = true;
		FRegistry::ServerDSM.GetSessionId(THandler<FAccelByteModelsServerSessionResponse>::CreateWeakLambda(this,
			[this](const FAccelByteModelsServerSessionResponse& SessionIdResponse)
			{

				UE_LOG(LogTemp, Log, TEXT("Session Id retrieval valid: [%s]; will now query session status"), *SessionIdResponse.Session_id);

				FRegistry::ServerMatchmaking.QuerySessionStatus(
					SessionIdResponse.Session_id,
					THandler<FAccelByteModelsMatchmakingResult>::CreateWeakLambda(this, [this](const FAccelByteModelsMatchmakingResult& Result)
					{
						MatchmakingData = Result;
						UE_LOG(LogTemp, Log, TEXT("SessionId is [%s]")
							, *MatchmakingData.Match_id
						);
						// Broadcast here if necessary

						UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Retrieved session status. Attempting to make it joinable."));
						FRegistry::ServerMatchmaking.EnqueueJoinableSession(Result, FVoidHandler::CreateLambda([]()
							{
								UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Made session joinable."));
							}), FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
								{
									UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Error EnqueueJoinableSession, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
								}));
					}),
					FErrorHandler::CreateWeakLambda(this, [this](int32 Code, const FString& Message) {
						UE_LOG(LogTemp, Log, TEXT("Unable to Query session status"));
						OnError(Code, Message);
						})
						);
			}),
			FErrorHandler::CreateWeakLambda(this, [this](int32 Code, const FString& Message) {
				UE_LOG(LogTemp, Log, TEXT("Unable to get Session Id"));
				OnError(Code,Message);
				})

				);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Session data is already cached, will not request a new session data"));
	}
}

void ASessionTestGameMode::OnError(int32 Code, const FString& Message)
{
	const FString ErrorCodeWithMessage = FString::Printf(TEXT("Error Code %d -> %s"), Code, *Message);
	UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorCodeWithMessage);
}

void ASessionTestGameMode::RegisterNewPlayer(FString arg_UserId)
{
	UserID = arg_UserId;
	FRegistry::ServerSessionBrowser.RegisterPlayer(FRegistry::ServerCredentials.GetMatchId(), UserID, false, THandler<FAccelByteModelsSessionBrowserAddPlayerResponse>::CreateLambda([this](const FAccelByteModelsSessionBrowserAddPlayerResponse& Result)
		{
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte Server: Player successfully registered with ID: %s"), *UserID);
			// Do something when request success
		}), FErrorHandler::CreateLambda([this](const int32 Code, const FString& Msg)
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Server: Player registration failed with ID: %s"), *UserID);
				// Do something when request error
			}));
}

void ASessionTestGameMode::DeregisterPlayer(FString arg_UserId)
{
	FRegistry::ServerSessionBrowser.UnregisterPlayer(FRegistry::ServerCredentials.GetMatchId(), arg_UserId, THandler<FAccelByteModelsSessionBrowserAddPlayerResponse>::CreateLambda([this](const FAccelByteModelsSessionBrowserAddPlayerResponse& Result)
		{
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte Server: Player successfully de-registered with ID."));
		}), FErrorHandler::CreateLambda([](const int32 Code, const FString& Msg)
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Server: Player de-registration failed for ID."));
			}));
}