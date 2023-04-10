#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "GameInstanceSubsystemSessions.generated.h"

/**
 *
 */

class UServerRow;
class FOnlineSessionSearch;
class UUserWidget;

UCLASS()
class SESSIONTEST_API UGameInstanceSubsystemSessions : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UGameInstanceSubsystemSessions();
	void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	//vars
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FName mapName;
	TSubclassOf<UUserWidget> ServerRowClass;
	TArray<UServerRow*> ServerList;
	//TArray<FOnlineSessionSearchResult> ServerResults;
	//TMap<FString, FOnlineSessionSearchResult&> ServerData;
	int32 PlayerID;
	FString currentSessionID;
	FString currentSessionName;
	bool isClient;
	bool bIsWaitingForSession = false;

	//void DoCreateProcFromPath(FString Path, FString OptionalPath);

	UFUNCTION(BlueprintCallable, Category="Sessions|Test")
	bool getSessionWaitStatus();

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
	void isWaitingForSession(bool isWaiting);

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
	void DoCreateProcFromPath(int32& ProcessId, FString FullPathOfProgramToRun, TArray<FString> CommandlineArgs, bool Detach, bool Hidden, int32 Priority, FString OptionalWorkingDirectory);

	//UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
	//TArray<FString> CreateSessionArguments();

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
		TArray<FString> CreateSessionArguments(FString UserId);

	//Session functions
	void SessionInit();

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
		void HostSession(FName newSessionName);

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
		void CreateSession(FName newSessionName);

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
		void OnCreateSessionComplete(FName SessionName, bool Success);

	void OnDestroySessionComplete(FName SessionName, bool Success);

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
		void OnFindSessionsComplete(bool Success);

	void GeneratePlayerID();

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
		int32 GetPlayerID();

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
		void JoinSession(FName SessionJoined);

	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
		FString JoinSessionWithID(FString SessionID);

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
		void JoinSessionWithName(FName SessionID);

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
		void RefreshSessionsList();

	//
	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
		void LaunchNewServerInstance(TArray<FString> commandLineArgs);

	UFUNCTION(BlueprintCallable, Category = "Sessions|Test")
	void LaunchNewLinuxServerInstance(TArray<FString> commandLineArgs);
	 
	UFUNCTION(BlueprintCallable, Category = "Sessions|UI")
		UServerRow* createServerRow(FString ServerName, FString ServerID);

	UFUNCTION(BlueprintCallable, Category = "Sessions|UI")
		TArray<UServerRow*> getServerList();

	UFUNCTION(BlueprintCallable, Category = "Session|UI")
		FString getCurrentSessionID() { return currentSessionID; }
};
