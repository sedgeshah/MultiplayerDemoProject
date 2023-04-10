#include "GameInstanceSubsystemSessions.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "ServerRow.h"
#include "Interfaces/OnlineSessionInterface.h"

// Session names
static const FName SERVER_LOBBY = TEXT("Server_Lobby");
static const FName SERVER_EXTRA = TEXT("Server_Extra");

// Return server list delegate
//DECLARE_DELEGATE_RetVal_OneParam(TArray<UServerRow*>, UpdateServerList, bool)

/*USTRUCT()
struct FServerInfo
{
	GENERATED_BODY()

	FString ServerName;
	FString ServerID;
}*/

#pragma region init
UGameInstanceSubsystemSessions::UGameInstanceSubsystemSessions()
{
	ConstructorHelpers::FClassFinder<UServerRow> ServerRowBPClass(TEXT("/Game/UI/WBP_ServerRow"));

	if (!ensure(ServerRowBPClass.Class != nullptr)) return;

	ServerRowClass = ServerRowBPClass.Class;
}

void UGameInstanceSubsystemSessions::Initialize(FSubsystemCollectionBase& Collection)
{
	//createServerRow("Server 1", "HDSC98743H");
	//createServerRow("Server 2", "JKSF89734H");

	FString sessionName;
	if (FParse::Value(FCommandLine::Get(), TEXT("sessionName"), sessionName))
	{
		sessionName = sessionName.Replace(TEXT("="), TEXT(""));
		currentSessionName = sessionName;
		UE_LOG(LogTemp, Warning, TEXT("Session name to be: %s"), *currentSessionName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No session name found. Assigning default name."));
	}

	//UE_LOG(LogTemp, Warning, TEXT("New ServerPath: %s.SessionTestServer.exe"), *serverPath);
	//UE_LOG(LogTemp, Warning, TEXT("New RightPath: %s"), *rightPath);
	//UE_LOG(LogTemp, Warning, TEXT("ProjectDir: %s"), *FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));

	SessionInit();
	auto GameInstance = GetGameInstance();
	if (GameInstance->IsDedicatedServerInstance())
	{

		isClient = false;
		if (currentSessionName.IsEmpty())
		{
			if (GetWorld()->URL.Port == 7777)
			{
				mapName = SERVER_LOBBY;
				HostSession(SERVER_LOBBY);
			}
			else if (GetWorld()->URL.Port == 7778)
			{
				mapName = SERVER_EXTRA;
				HostSession(SERVER_EXTRA);
			}
		}
		else
		{
			mapName = FName(*currentSessionName);
			HostSession(mapName);
		}
	}
	else
	{
		isClient = true;
		GeneratePlayerID();
		//SessionInterface = GetSessionInterface();
	}
	//LaunchNewServerInstance();
}

void UGameInstanceSubsystemSessions::GeneratePlayerID()
{
	PlayerID = FMath::RandRange(1, 9999);
}

int32 UGameInstanceSubsystemSessions::GetPlayerID()
{
	return PlayerID;
}
#pragma endregion init

#pragma region SESSIONS_REGION
void UGameInstanceSubsystemSessions::SessionInit()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Found subsystem %s."), *Subsystem->GetSubsystemName().ToString());
		SessionInterface = Subsystem->GetSessionInterface();

		if (SessionInterface.IsValid()) {
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UGameInstanceSubsystemSessions::OnCreateSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UGameInstanceSubsystemSessions::OnDestroySessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UGameInstanceSubsystemSessions::OnFindSessionsComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UGameInstanceSubsystemSessions::OnJoinSessionComplete);


			//Refresh session list
			if (!isClient)
			{
				SessionSearch = MakeShareable(new FOnlineSessionSearch());

				if (SessionSearch.IsValid())
				{
					SessionSearch->bIsLanQuery = true;
					UE_LOG(LogTemp, Warning, TEXT("Server: Starting session search."));
					SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Found no subsystem"));
	}
}

void UGameInstanceSubsystemSessions::HostSession(FName newSessionName)
{
	if (SessionInterface.IsValid())
	{
		auto ExistingSession = SessionInterface->GetNamedSession(newSessionName);
		//auto info = ExistingSession->Session;
		if (ExistingSession != nullptr) // If session by this name exists, destroy it, else create it.
		{
			SessionInterface->DestroySession(newSessionName);
		}
		else
		{
			CreateSession(newSessionName);
		}
	}
}

void UGameInstanceSubsystemSessions::CreateSession(FName newSessionName)
{
	if (SessionInterface.IsValid())
	{
		FOnlineSessionSettings SessionSettings;
		SessionSettings.bAllowJoinInProgress = true;
		SessionSettings.bIsDedicated = true;
		SessionSettings.bIsLANMatch = false;
		SessionSettings.NumPrivateConnections = 500;
		SessionSettings.NumPublicConnections = 500;
		SessionSettings.bUsesPresence = true;
		SessionSettings.bShouldAdvertise = true;
		SessionSettings.Set(TEXT("SessionName"), FString(newSessionName.ToString()), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		if (newSessionName.Compare(SERVER_LOBBY))
			SessionSettings.Set(SETTING_MAPNAME, FString("/Game/ThirdPersonCPP/Maps/ThirdPersonExampleMap"), EOnlineDataAdvertisementType::ViaOnlineService);
		else if (newSessionName.Compare(SERVER_EXTRA))
			SessionSettings.Set(SETTING_MAPNAME, FString("/Game/ThirdPersonCPP/Maps/map2"), EOnlineDataAdvertisementType::ViaOnlineService);
		else
			SessionSettings.Set(SETTING_MAPNAME, FString("/Game/ThirdPersonCPP/Maps/SessionMap"), EOnlineDataAdvertisementType::ViaOnlineService);
		//SessionInterface->CreateSessionIdFromString("farhan_session");
		SessionInterface->CreateSession(0, newSessionName, SessionSettings);
		//SessionInterface->AddNamedSession(newSessionName, SessionSettings);
		//auto abc = SessionInterface->GetSessionId();
	}
}

void UGameInstanceSubsystemSessions::OnCreateSessionComplete(FName SessionName, bool Success)
{
	if (Success)
	{
		UWorld* World = GetWorld();

		if (!ensure(World != nullptr)) return;

		//World->ServerTravel("/Game/Dev/Maps/Initial_Level");
		UE_LOG(LogTemp, Warning, TEXT("Session created."));
		auto ExistingSession = SessionInterface->GetNamedSession(SessionName);
		FString info = ExistingSession->GetSessionIdStr();
		//auto id = info->GetSessionId();
		//UE_LOG(LogTemp, Warning, TEXT("%s"), SessionName);
		UE_LOG(LogTemp, Warning, TEXT("Created session's ID: %s"),*info);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Session unable to be created."));
	}
}

void UGameInstanceSubsystemSessions::OnDestroySessionComplete(FName SessionName, bool Success)
{
	if (Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Previous matching session destroyed."));
		CreateSession(mapName);
	}
}

void UGameInstanceSubsystemSessions::RefreshSessionsList()
{
	SessionSearch = MakeShareable(new FOnlineSessionSearch());

	//TArray<FString> SessionList;

	if (SessionSearch.IsValid())
	{
		SessionSearch->bIsLanQuery = true;
		UE_LOG(LogTemp, Warning, TEXT("ClientInvoked: Starting to find sessions."));
		SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	}
}

void UGameInstanceSubsystemSessions::OnFindSessionsComplete(bool Success)
{
	if (Success && SessionSearch.IsValid())
	{
		if (!isClient)
		{
			UE_LOG(LogTemp, Warning, TEXT("Finished finding sessions."));
			for (FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
			{
				UE_LOG(LogTemp, Warning, TEXT("Found session ID: %s"), *SearchResult.GetSessionIdStr());
				FString name;
				if (SearchResult.Session.SessionSettings.Get(TEXT("SessionName"), name))
				{
					UE_LOG(LogTemp, Warning, TEXT("Found session named: %s"), *name);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("No session name found."))
				}
				//ServerData.FindOrAdd(*SearchResult.GetSessionIdStr())
				//ServerResults.Add(SearchResult);
				//SessionInterface->JoinSession(0, *SearchResult.GetSessionIdStr(), SearchResult);
				//break;
				//ServerData.FindOrAdd(*SearchResult.GetSessionIdStr(), SearchResult);
			}
			//auto GameInstance = GetGameInstance();
			/*if (GameInstance->IsDedicatedServerInstance())
			{
				JoinSession(SERVER_MAP2);
			}*/
		}
		else
		{
			if (!ensure(UGameplayStatics::GetPlayerController(this, 0) != nullptr)) return;
			if (UGameplayStatics::GetPlayerController(this, 0) != nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("Finished finding sessions."));
				ServerList.Empty();
				for (FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
				{
					UE_LOG(LogTemp, Warning, TEXT("Found session named: %s"), *SearchResult.GetSessionIdStr());
					FString searchName;
					SearchResult.Session.SessionSettings.Get(TEXT("SessionName"), searchName);
					ServerList.Add(createServerRow(searchName, *SearchResult.GetSessionIdStr()));
				}
			}
			if (bIsWaitingForSession)
			{
				UE_LOG(LogTemp, Warning, TEXT("Client is about to connect."));
				bIsWaitingForSession = false;
				FString SessionNameToSearch = FString::FromInt(PlayerID) + "_UserSession";
				JoinSessionWithName(*SessionNameToSearch);
			}
		}
	}
}

void UGameInstanceSubsystemSessions::JoinSession(FName SessionJoined)
{
	if (!SessionInterface.IsValid()) return;
	if (!SessionSearch.IsValid()) return;

	//if ()
	SessionInterface->JoinSession(0, SessionJoined, SessionSearch->SearchResults[0]);
}

void UGameInstanceSubsystemSessions::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (!SessionInterface.IsValid()) return;

	FString Address;
	if (!SessionInterface->GetResolvedConnectString(SessionName, Address))
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not get connect string."));
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

	if (PlayerController)
		PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);

	UE_LOG(LogTemp, Warning, TEXT("Session joined."));
}

void UGameInstanceSubsystemSessions::JoinSessionWithName(FName sessionName)
{
	if (SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Finding session (join by name): %s"), *sessionName.ToString());

		for (FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
		{
			FString searchName;
			if (SearchResult.Session.SessionSettings.Get(TEXT("SessionName"), searchName))
			{
				if (searchName == sessionName.ToString())
				{
					UE_LOG(LogTemp, Warning, TEXT("Found session to join: %s"), *SearchResult.GetSessionIdStr());
					SessionInterface->JoinSession(0, *SearchResult.GetSessionIdStr(), SearchResult);
					currentSessionID = *SearchResult.GetSessionIdStr();
					//return currentSessionID;
					break;
				}
			}
				/*if (*SearchResult.GetSessionIdStr() == findSessionID)
				{
					UE_LOG(LogTemp, Warning, TEXT("Found session to join: %s"), *SearchResult.GetSessionIdStr());
					SessionInterface->JoinSession(0, *SearchResult.GetSessionIdStr(), SearchResult);
					currentSessionID = *SearchResult.GetSessionIdStr();
					//return currentSessionID;
					break;
				}*/
		}
		//}
		//else
		//{
		//	UE_LOG(LogTemp, Warning, TEXT("Session not found (by name)."));
	}
}

FString UGameInstanceSubsystemSessions::JoinSessionWithID(FString SessionID)
{
	//SessionInterface->JoinSession(0, *SearchResult.GetSessionIdStr(), SearchResult);
	/*int index = 0;
	for (FOnlineSessionSearchResult& ServerResult : ServerResults)
	{
		if (ServerResult.GetSessionIdStr().Equals(SessionID))
		{
			SessionInterface->JoinSession(0, *ServerResult.GetSessionIdStr(), ServerResult);
			return;
		}
	}*/

	for (FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
	{
		if (*SearchResult.GetSessionIdStr() == SessionID)
		{
			UE_LOG(LogTemp, Warning, TEXT("Found session to join: %s"), *SearchResult.GetSessionIdStr());
			SessionInterface->JoinSession(0, *SearchResult.GetSessionIdStr(), SearchResult);
			currentSessionID = *SearchResult.GetSessionIdStr();
			return currentSessionID;
		}
		//UE_LOG(LogTemp, Warning, TEXT("Found session named: %s"), *SearchResult.GetSessionIdStr());
		//ServerData.FindOrAdd(*SearchResult.GetSessionIdStr())
		//ServerResults.Add(SearchResult);
		//SessionInterface->JoinSession(0, *SearchResult.GetSessionIdStr(), SearchResult);
		//break;
		//ServerData.FindOrAdd(*SearchResult.GetSessionIdStr(), SearchResult);
	}
	UE_LOG(LogTemp, Warning, TEXT("Session entered not found!"));

	return FString(TEXT("No session joined"));
	/*bool hasID = ServerList.Contains(SessionID);

	if (hasID)
	{
		SessionInterface->JoinSession(0, TEXT(SessionID), ServerList.Find[SessionID]);
	}*/
}

#pragma endregion SESSIONS_REGION

#pragma region LAUNCH_NEW_INSTANCE

void UGameInstanceSubsystemSessions::isWaitingForSession(bool isWaiting)
{
	bIsWaitingForSession = isWaiting;
}

bool UGameInstanceSubsystemSessions::getSessionWaitStatus()
{
	return bIsWaitingForSession;
}

void UGameInstanceSubsystemSessions::LaunchNewLinuxServerInstance(TArray<FString> commandLineArgs)
{
	FString serverPath;
	FString rightPath;

	UKismetSystemLibrary::GetProjectDirectory().Split("/", &serverPath, &rightPath, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	serverPath.Split("/", &serverPath, &rightPath, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	FString optionalPath = serverPath;
	/*serverPath.Append("/LinuxServerStart.sh");*/
	serverPath.Append("/ServerStart.bat");
	serverPath = serverPath.Replace(TEXT("/"), TEXT("\\"));

	UE_LOG(LogTemp, Warning, TEXT("CreateProc at: %s"), *serverPath);

	FString Args = "";
	if (commandLineArgs.Num() > 1)
	{
		Args = commandLineArgs[0];
		for (int32 v = 1; v < commandLineArgs.Num(); v++)
		{
			Args += " " + commandLineArgs[v];
		}
	}
	else if (commandLineArgs.Num() > 0)
	{
		Args = commandLineArgs[0];
	}

	UE_LOG(LogTemp, Warning, TEXT("Launch arguments: %s"), *Args);

	uint32 NeedBPUINT32 = 0;
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(
		*serverPath,
		*Args,
		false,//* @param bLaunchDetached		if true, the new process will have its own window
		false,//* @param bLaunchHidded			if true, the new process will be minimized in the task bar
		false,//* @param bLaunchReallyHidden	if true, the new process will not have a window or be in the task bar
		&NeedBPUINT32,
		0,
		(optionalPath != "") ? *optionalPath : nullptr,//const TCHAR* OptionalWorkingDirectory, 
		nullptr
	);
}

void UGameInstanceSubsystemSessions::LaunchNewServerInstance(TArray<FString> commandLineArgs)
{
	FString serverPath;
	FString rightPath;

	UKismetSystemLibrary::GetProjectDirectory().Split("/", &serverPath, &rightPath, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	serverPath.Split("/", &serverPath, &rightPath, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	FString optionalPath = serverPath;
	serverPath.Append("/ServerStart.bat");
	serverPath = serverPath.Replace(TEXT("/"), TEXT("\\"));

	UE_LOG(LogTemp, Warning, TEXT("CreateProc at: %s"), *serverPath);

	FString Args = "";
	if (commandLineArgs.Num() > 1)
	{
		Args = commandLineArgs[0];
		for (int32 v = 1; v < commandLineArgs.Num(); v++)
		{
			Args += " " + commandLineArgs[v];
		}
	}
	else if (commandLineArgs.Num() > 0)
	{
		Args = commandLineArgs[0];
	}

	UE_LOG(LogTemp, Warning, TEXT("Launch arguments: %s"), *Args);

	uint32 NeedBPUINT32 = 0;
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(
		*serverPath,
		*Args,
		false,//* @param bLaunchDetached		if true, the new process will have its own window
		false,//* @param bLaunchHidded			if true, the new process will be minimized in the task bar
		false,//* @param bLaunchReallyHidden	if true, the new process will not have a window or be in the task bar
		&NeedBPUINT32,
		0,
		(optionalPath != "") ? *optionalPath : nullptr,//const TCHAR* OptionalWorkingDirectory, 
		nullptr
	);

	//Not sure what to do to expose UINT32 to BP
	//ProcessId = NeedBPUINT32;

	//UE_LOG(LogTemp, Warning, TEXT("%s"), *LogMessage);
	//Path += FString(TEXT("ServerShortcuts/SessionTestServer_TP.lnk"));
	//const TCHAR* cPath = *Path;
// 	if (FPaths::DirectoryExists(*serverPath))
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("PATH: Directory exists at %s"), *serverPath);
// 	}
// 	else
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("PATH: Directory does not exist."));
// 	}
	//FPlatformProcess::ExecProcess(cPath, nullptr, 0, 0, 0);
	//FPlatformProcess::CreateProc(TEXT("\"D:\Unreal Test Projects\SessionTest_clone\New_Builds\WindowsServer\ServerStart.bat\""), nullptr, true, false, false, nullptr, 0, nullptr, nullptr);
	//FGenericPlatformProcess::CreateProc(TEXT("C:\\Users\\BIM\\Desktop\\WindowsServer\\ServerStart.bat"), nullptr, true, false, false, nullptr, 0, nullptr, nullptr);
	//FPlatformProcess::CreateProc(TEXT("D:\\Unreal Test Projects\\SessionTest_clone\\New_Builds\\WindowsServer\\ServerStart.bat"), nullptr, true, false, false, nullptr, 0, nullptr, nullptr);
	//FPlatformProcess::CreateProc((TEXT("\"%s\""), *serverPath), nullptr, true, false, false, nullptr, 0, nullptr, nullptr);
	//(TEXT("%s"), *serverP)
	//FPlatformProcess::CreateProc((TEXT("D:\\Unreal Test Projects\";																													  
	//FPlatformProcess::CreateProc(TEXT("C:\\Users\\Cero\\Desktop\\Test.exe"), nullptr, true, false, false, nullptr, 0, nullptr, nullptr);
	//UE_LOG(LogTemp, Warning, TEXT("Creating Proc at: %s"), *serverPath);
}

TArray<FString> UGameInstanceSubsystemSessions::CreateSessionArguments(FString UserId)
{
	FString argMapPath = "/Game/ThirdPersonCPP/Maps/ThirdPersonExampleMap";
	FString argSessionName = "-sessionName=" + UserId + "_userSession";
	TArray<FString> commandLineArguments;
	commandLineArguments.Add(argMapPath);
	commandLineArguments.Add(argSessionName);
	return commandLineArguments;
}

//TArray<FString> UGameInstanceSubsystemSessions::CreateSessionArguments(FString UserId)
//{
//	FString argMapPath = "/Game/ThirdPersonCPP/Maps/ThirdPersonExampleMap";
//	FString argSessionName = "-sessionName=" + FString::FromInt(PlayerID) + "_UserSession";
//	TArray<FString> commandLineArguments;
//	commandLineArguments.Add(argMapPath);
//	commandLineArguments.Add(argSessionName);
//	return commandLineArguments;
//}

//void UGameInstanceSubsystemSessions::DoCreateProcFromPath(FString Path, FString OptionalPath)
void UGameInstanceSubsystemSessions::DoCreateProcFromPath(int32& ProcessId, FString FullPathOfProgramToRun, TArray<FString> CommandlineArgs, bool Detach, bool Hidden, int32 Priority, FString OptionalWorkingDirectory)
{
	//FPaths::Get
	//Path = "D:\\Unreal Test Projects\\SessionTest_clone\\New_Builds\\WindowsServer\\SessionTestServer.bat";
	//FPlatformProcess::CreateProc((TEXT("%s"), *Path), nullptr, true, false, false, nullptr, 0, (TEXT("%s"), *OptionalPath), nullptr);
	//system(StringCast<ANSICHAR>(*Path).Get());
	//FPlatformProcess::ExecProcess((TEXT("%s"), *Path), nullptr, 0, 0, 0);
	UE_LOG(LogTemp, Warning, TEXT("CreateProc at: %s"), *FullPathOfProgramToRun);

	FString Args = "";
	if (CommandlineArgs.Num() > 1)
	{
		Args = CommandlineArgs[0];
		for (int32 v = 1; v < CommandlineArgs.Num(); v++)
		{
			Args += " " + CommandlineArgs[v];
		}
	}
	else if (CommandlineArgs.Num() > 0)
	{
		Args = CommandlineArgs[0];
	}

	uint32 NeedBPUINT32 = 0;
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(
		*FullPathOfProgramToRun,
		*Args,
		Detach,//* @param bLaunchDetached		if true, the new process will have its own window
		false,//* @param bLaunchHidded			if true, the new process will be minimized in the task bar
		Hidden,//* @param bLaunchReallyHidden	if true, the new process will not have a window or be in the task bar
		&NeedBPUINT32,
		Priority,
		(OptionalWorkingDirectory != "") ? *OptionalWorkingDirectory : nullptr,//const TCHAR* OptionalWorkingDirectory, 
		nullptr
	);

	//Not sure what to do to expose UINT32 to BP
	ProcessId = NeedBPUINT32;
}



#pragma endregion LAUNCH_NEW_INSTANCE

#pragma region SERVER_ROW_UI
UServerRow* UGameInstanceSubsystemSessions::createServerRow(FString ServerName, FString ServerID)
{
	if (!ensure(this->GetWorld() != nullptr))
	{
		UE_LOG(LogTemp, Warning, TEXT("No PlayerController found!"));
		return nullptr;
	} 

	UServerRow* ServerRow = CreateWidget<UServerRow>(UGameplayStatics::GetPlayerController(this, 0), ServerRowClass);
	if (!ensure(ServerRow != nullptr)) return nullptr;

	UE_LOG(LogTemp, Warning, TEXT("Adding server row."));

	ServerRow->SetServerName(ServerName);
	ServerRow->SetServerID(ServerID);

	//ServerList.Add(ServerRow);

	//Search Sessions

	return ServerRow;
}

TArray<UServerRow*> UGameInstanceSubsystemSessions::getServerList()
{
	return ServerList;
}
#pragma endregion SERVER_ROW_UI