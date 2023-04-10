#include "GI_Vivox.h"
#if !UE_SERVER
#include "VivoxCore.h"
#include "VivoxCore\Public\ILoginSession.h"
#include "PC_VoiceChat.h"
#include "Core/AccelByteCredentials.h"
//#include "Plugins/VivoxCoreUE4Plugin/Source/VivoxCore/Public/
#endif
#include "Core/AccelByteApiClient.h"
#include "Core/AccelByteServerApiClient.h"
#include "Api/AccelByteLobbyApi.h"
#include "Models/AccelByteMatchmakingModels.h"
#include "Blueprints/ABServerOauth2.h"
#include "Models/AccelByteLobbyModels.h"
#include "Core/AccelByteMultiRegistry.h"
#include "Core/AccelByteRegistry.h"
//
//#include "Serialization/JsonTypes.h"
#include "Dom/JsonObject.h"/*
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"*/
#include "JsonObjectWrapper.h"

#include "../SessionTestGameMode.h"

#pragma region Initialising
UGI_Vivox::UGI_Vivox()
{
#if !UE_SERVER
	vModule = static_cast<FVivoxCoreModule*>(&FModuleManager::Get().LoadModuleChecked(TEXT("VivoxCore")));
#endif

	//if (IsDedicatedServerInstance())
	//{
	//	ServerRegistrationAndLogin();
	//}
	//else
	//{
	//	// Client-related stuff if it is not being handled in blueprints
	//}
}

void UGI_Vivox::Init()
{
	Super::Init();
}

#pragma endregion Initialising

////////////////////////////////////////

#pragma region AccelbyteHelpers

#pragma region Timers
double UGI_Vivox::Delay(double lastTime)
{
	const double AppTime = FPlatformTime::Seconds();
	FHttpModule::Get().GetHttpManager().Tick(AppTime - lastTime);
	FTicker::GetCoreTicker().Tick(AppTime - lastTime);
	FRegistry::HttpRetryScheduler.PollRetry(FPlatformTime::Seconds());
	lastTime = AppTime;
	FPlatformProcess::Sleep(0.6f);
	return lastTime;
}

void UGI_Vivox::TimerFunction(float seconds) {
	float startTime = GetWorld()->TimeSeconds;
	float endTime = startTime + seconds;

	while (endTime > startTime)
	{
		startTime = GetWorld()->TimeSeconds;
	}
}
#pragma endregion Timers

//--------------------------------------

#pragma region FakeRegister

void UGI_Vivox::TestCheatRegister(int32 TestId, FString PlayerName) const
{
	FString PCName = FPlatformProcess::ComputerName();
	PCName.ReplaceCharInline('-', ' '); // Issue in registering if PC name contains a '-'
	PCName.RemoveSpacesInline();

	const FString Username = FString::Printf(TEXT("Test%iUser%s"), TestId, *PCName);
	const FString Password = FString::Printf(TEXT("Password123"));
	const FString Email = FString::Printf(TEXT("%i@%s.com"), TestId, *PCName);

	FRegisterRequestv3 Request;
	const FString TestName = FString::Printf(TEXT("test%d"), TestId);
	/*Request.DisplayName = TestName;*/
	Request.DisplayName = PlayerName;
	Request.Username = Username;
	Request.EmailAddress = Email;
	Request.Password = Password;
	Request.Country = TEXT("US");
	Request.DateOfBirth = TEXT("1980-12-20");

	auto OnSuccess = THandler<FRegisterResponse>::CreateLambda([this, Username, Password, Email](const FRegisterResponse& Result) {
		UE_LOG(LogTemp, Log, TEXT("debug account creation success\n UserName [%s] Password [%s] Email [%s]"), *Username, *Password, *Email);
		AutoLogin.Broadcast(Password, Email);
		});
	auto OnFailure = FErrorHandler::CreateLambda([&](int32 ErrorCode, const FString& ErrorMessage)
		{
			UE_LOG(LogTemp, Log, TEXT("AccelByte: Debug account creation failed. Code: %d, Message: %s"), ErrorCode, *ErrorMessage);
		});

	AccelByte::FRegistry::User.Registerv3(Request, OnSuccess, OnFailure);
}
#pragma endregion FakeRegister

//--------------------------------------

#pragma region IsRoomServer

bool UGI_Vivox::IsRoomServer()
{
	return UGameplayStatics::HasLaunchOption("room");
}
#pragma endregion IsRoomServer

#pragma endregion AccelbyteHelpers

////////////////////////////////////////

#pragma region AccelbyteServer

#pragma region ManagedServerRegistration

void UGI_Vivox::ServerRegistrationAndLogin()
{
#if UE_SERVER
	bool bServerLoggedIn = false;
	FRegistry::ServerOauth2.LoginWithClientCredentials(FVoidHandler::CreateLambda([this, &bServerLoggedIn]()
		{
			bServerLoggedIn = true;
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Server client credentials validated."));
			const bool bExplicitLocalBuild = UGameplayStatics::HasLaunchOption("LocalBuild");
			UE_LOG(LogTemp, Warning, TEXT("bExplicitLocalBuild [%d]"), bExplicitLocalBuild);

			ServerApiClient->ServerDSM.SetServerType(bExplicitLocalBuild ? EServerType::LOCALSERVER : EServerType::CLOUDSERVER);

			if (bExplicitLocalBuild)
			{
				RegisterLocalServerToDSM();
			}
			else
			{
				RegisteringServerToDSM();
			}

		}), FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
			{
				UE_LOG(LogTemp, Log, TEXT("Accelbyte: Server Error LoginWithClientCredentials, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
			}));
#endif
}

#if UE_SERVER
void UGI_Vivox::RegisteringServerToDSM()
{
	bool bServerLoggedIn = false;
	int32 Port = 7777;
	FRegistry::ServerDSM.RegisterServerToDSM(Port, FVoidHandler::CreateLambda([this, &bServerLoggedIn]()
		{
			bServerLoggedIn = true;
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Managed server successfully registered to DSM."));
			
			//const bool bRoomBuild = UGameplayStatics::HasLaunchOption("room");
			
			/*if (bRoomBuild)
				ServerCreateSessionInBrowser();*/
		}), FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
			{
				// Do something if RegisterServerToDSM has an error
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Error RegisterServerToDSM, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
			}));
}

#endif

#pragma endregion ManagedServerRegistration

//--------------------------------------

#pragma region SessionBrowser

void UGI_Vivox::ServerCreateSessionInBrowser()
{
	EAccelByteSessionType SessionType{ EAccelByteSessionType::dedicated };
	//FString Game_Mode{ "metalobby" };

	const bool bRoomBuild = UGameplayStatics::HasLaunchOption("room");
	
	Version = "0.57";

	if (bRoomBuild)
	{
		GameMode = "metaroom";
		UE_LOG(LogTemp, Warning, TEXT("AccelByte: Room deployment."));
	}
	else
	{
		GameMode = "metalobby";
		UE_LOG(LogTemp, Warning, TEXT("AccelByte: Lobby deployment."));
	}

	FString MapName{ "map2" };
	int32 BotCount{ 0 };
	int32 MaxPlayer{ 5000 };
	int32 MaxSpectator{ 5 };
	FString Password{ "" };
	TSharedPtr<FJsonObject> OtherSettings; //{ MakeShared<FJsonObject>() };
	//OtherSettings->SetStringField("", "");

	ServerApiClient->ServerSessionBrowser.CreateGameSession(SessionType, GameMode, MapName, Version, BotCount, MaxPlayer, MaxSpectator, Password, OtherSettings, THandler<FAccelByteModelsSessionBrowserData>::CreateLambda([this](const FAccelByteModelsSessionBrowserData& Result)
		{
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Session creation success.\tSession ID: %s"), *Result.Session_id);
			SessionID = Result.Session_id;
			ServerRegisterGameSession(SessionID);
		}), FErrorHandler::CreateLambda([](const int32 ErrorCode, const FString& ErrorMessage)
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Failed to create session. Code: %d, Message: %s"), ErrorCode, *ErrorMessage);
			}));
}

void UGI_Vivox::ServerRegisterGameSession(FString SessionId)
{
	UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Server register game session for:\tSession ID: %s"), *SessionId);
	ServerApiClient->ServerDSM.RegisterServerGameSession(SessionId, GameMode, THandler<FAccelByteModelsServerCreateSessionResponse>::CreateLambda([this](const FAccelByteModelsServerCreateSessionResponse& Result)
		{
			SessionID = Result.Session.Id;
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Server game session registration successful for ID: %s"), *Result.Session.Id);
		}), FErrorHandler::CreateLambda([&](int32 ErrorCode, FString ErrorMessage)
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Failed to register game session. Code: %d, Message: %s"), ErrorCode, *ErrorMessage);
			}));
}

void UGI_Vivox::ServerRemoveGameSession()
{
	UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Removing session ID: %s"), *SessionID);
	ServerApiClient->ServerSessionBrowser.RemoveGameSession(SessionID, THandler<FAccelByteModelsSessionBrowserData>::CreateLambda([this](const FAccelByteModelsSessionBrowserData& Result)
		{
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Session removal success for ID: %s"), *SessionID);
			SessionRemoved.Broadcast(true);
		}), FErrorHandler::CreateLambda([this](const int32 ErrorCode, const FString& ErrorMessage)
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Failed to remove session ID = %s. Code: %d, Message: %s"), *SessionID, ErrorCode, *ErrorMessage);
				SessionRemoved.Broadcast(false);
			}));
}

#pragma endregion SessionBrowser

//--------------------------------------

#pragma region SessionBrowserAlone


#pragma endregion SessionBrowserAlone

//--------------------------------------

#pragma region LocalServer

FString UGI_Vivox::GetLocalServerName() const
{
	const FString LocalServerName = FString(FPlatformProcess::ComputerName());

	return LocalServerName;
}

void UGI_Vivox::RegisterLocalServerToDSM()
{
	bool bServerLoggedIn = false;
	isLocalServer = true;
	//FString IpAddress = FString("192.168.10.116");
	FString IpAddress = "127.0.0.1";

	int32 Port = 7777;

	//ServerName = FString("testingmeta");
	ServerName = GetLocalServerName();

	FRegistry::ServerDSM.RegisterLocalServerToDSM(IpAddress, Port, ServerName,
		FVoidHandler::CreateLambda([this, &bServerLoggedIn]()
			{
				bServerLoggedIn = true;
				UE_LOG(LogTemp, Warning, TEXT("Local Server registered to DSM."));

				UE_LOG(LogTemp, Warning, TEXT("SessionID Attempting to Get Server Session Data, it should fail as server has no session ID yet"));
				Cast<ASessionTestGameMode>(UGameplayStatics::GetGameMode(this))->ServerRequestSessionData();

				UE_LOG(LogTemp, Warning, TEXT("Server Info will be ready post successful registration"));
				Cast<ASessionTestGameMode>(UGameplayStatics::GetGameMode(this))->ServerRequestServerInfo();

				// Do something if RegisterLocalServerToDSM has been successful
				ServerCreateSessionInBrowser();
			}),
		FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
			{
				// Do something if RegisterLocalServerToDSM has an error
				UE_LOG(LogTemp, Warning, TEXT("Error RegisterLocalServerToDSM, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
			}));
}

void UGI_Vivox::DeregisterLocalDSFromDSM()
{
	ServerName = GetLocalServerName();
	FRegistry::ServerDSM.DeregisterLocalServerFromDSM(ServerName, FVoidHandler::CreateLambda([]()
		{
			UE_LOG(LogTemp, Warning, TEXT("Deregistered local server from DSM."));
			// Do something if DeregisterLocalServerFromDSM has been successful
		}), FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
			{
				// Do something if DeregisterLocalServerFromDSM has an error
				UE_LOG(LogTemp, Warning, TEXT("Error DeregisterLocalServerFromDSM, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
			}));
}

#pragma endregion LocalServer

//--------------------------------------

#pragma region SessionID and Joinability

bool UGI_Vivox::isSessionIDSet()
{
	if (SessionID.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Session ID not yet assigned."));
		return false;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Session ID assigned: %s"), *SessionID);
		return true;
	}
}

void UGI_Vivox::GetServerSessionID()
{
	if (SessionID.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Accelbyte Server: Attempting to get server session ID."));

		FRegistry::ServerDSM.GetSessionId(THandler<FAccelByteModelsServerSessionResponse>::CreateLambda([this](const FAccelByteModelsServerSessionResponse& Result)
			{
				// Do something if GetSessionId has been successful
				if (!Result.Session_id.IsEmpty())
				{
					SessionID = Result.Session_id;
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte Server: Session ID retrieved: %s"), *SessionID);
					MakeSessionJoinable();
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte Server: Session ID not yet assigned.")); //Searching again for session ID value.
					//FPlatformProcess::Sleep(0.6f);
				}
			}), FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
				{
					// Do something if GetSessionId has an error
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Error GetSessionId, Error Code: %d ErrorMessage: %s. Server does not have a session yet."), ErrorCode, *ErrorMessage);
				}));
	}

}

void UGI_Vivox::MakeSessionJoinable()
{
	UE_LOG(LogTemp, Warning, TEXT("Accelbyte Server: Querying session status."));
	FRegistry::ServerMatchmaking.QuerySessionStatus(SessionID, THandler<FAccelByteModelsMatchmakingResult>::CreateLambda([](const FAccelByteModelsMatchmakingResult& Result)
		{
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Retrieved session status. Attempting to make it joinable."));
			FRegistry::ServerMatchmaking.EnqueueJoinableSession(Result, FVoidHandler::CreateLambda([]()
				{
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Made session joinable."));
				}), FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
					{
						UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Error EnqueueJoinableSession, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
					}));
		}), FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
			{
				// Do something if QuerySessionStatus has an error
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Error QuerySessionStatus, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
			}));
}

#pragma endregion SessionID and Joinability

#pragma endregion AccelbyteServer

////////////////////////////////////////

#pragma region AccelbyteClient

#pragma region CreateParty

void UGI_Vivox::CreateParty()
{
	if (isAccelbyteLoggedIn)
	{
		bool bHasPartyBeenCreated = false;
		FRegistry::Lobby.SetCreatePartyResponseDelegate(AccelByte::Api::Lobby::FPartyCreateResponse::CreateLambda([this, &bHasPartyBeenCreated](FAccelByteModelsCreatePartyResponse result)
			{
				UE_LOG(LogTemp, Warning, TEXT("Party response received."));
				if (result.PartyId.IsEmpty())
				{
					UE_LOG(LogTemp, Warning, TEXT("Empty partyID error!"));
					PartyCreated.Broadcast(bHasPartyBeenCreated);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Party successfully created."));
					partyID = result.PartyId;
					invitationToken = result.InvitationToken;
					bHasPartyBeenCreated = true;
					PartyCreated.Broadcast(bHasPartyBeenCreated);
				}
			}), FErrorHandler::CreateLambda([this, &bHasPartyBeenCreated](int32 ErrorCode, const FString& ErrorMessage)
				{
					// Do something if QuerySessionStatus has an error
					if (ErrorCode == 11232)
					{
						UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: User already in party."));
						bHasPartyBeenCreated = true;
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Party creation error, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
					}
					PartyCreated.Broadcast(bHasPartyBeenCreated);
				}));
		FRegistry::Lobby.Connect();
		FRegistry::Lobby.SendCreatePartyRequest();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: User not logged in."));
	}
}

#pragma endregion CreateParty

//--------------------------------------

#pragma region Chat

void UGI_Vivox::JoinDefaultChatChannel()
{
	FRegistry::Lobby.SetJoinChannelChatResponseDelegate(AccelByte::Api::Lobby::FJoinDefaultChannelChatResponse::CreateLambda([](const FAccelByteModelsJoinDefaultChannelResponse& Result)
		{
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: User connected to default chat channel."));
		}));

	FRegistry::Lobby.SendJoinDefaultChannelChatRequest();
}

void UGI_Vivox::SendGlobalChatMessage(FString Message)
{
	UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Sending message to Global Chat."));
	FRegistry::Lobby.SendChannelMessage(Message);
}

void UGI_Vivox::ReceiveGlobalChatMessage()
{
	FRegistry::Lobby.SetChannelMessageNotifDelegate(AccelByte::Api::Lobby::FChannelChatNotif::CreateLambda([this](const FAccelByteModelsChannelMessageNotice& Result)
		{
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Received message in Global Chat."));
			ReceivedMessage.Broadcast(Result);
		}/*), FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
		{
			UE_LOG(LogTemp, Log, TEXT("Accelbyte Client: Error receiving Global Chat message, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
		}*/));
}

#pragma endregion Chat

//--------------------------------------

#pragma region SessionBrowser

void UGI_Vivox::CreateSessionRequest(bool bIsRoom, FString HostUsername)
{
	EAccelByteSessionType SessionType{ EAccelByteSessionType::dedicated };

	isRoom = bIsRoom;
	//FString Game_Mode;
	FString MapName;
	int32 MaxPlayer;
	TSharedPtr<FJsonObject> OtherSetting{ MakeShared<FJsonObject>() };
	HostName = HostUsername;

	Version = "0.57";

	if (isRoom)
	{
		CurrentSearchStatus = SearchStatus::SearchingRoom;
		GameMode = "metaroom";
		MapName = "room";
		MaxPlayer = 20;
		OtherSetting->SetStringField("username", HostUsername);
		Deployment = "Armada";
	}
	else
	{
		CurrentSearchStatus = SearchStatus::SearchingLobby;
		GameMode = "metalobby";
		MapName = "lobby";
		MaxPlayer = 500;
		OtherSetting->SetStringField("username", "lobby");
		Deployment = "testingmeta";
	}
	
	int32 BotCount{ 0 };
	int32 MaxSpectator{ 5 };
	FString Password{ "" };
	
	ApiClient->SessionBrowser.CreateGameSession(SessionType, GameMode, MapName, Version, BotCount, MaxPlayer, MaxSpectator, Password, OtherSetting, THandler<FAccelByteModelsSessionBrowserData>::CreateLambda([this](const FAccelByteModelsSessionBrowserData& Result)
		{
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Session creation success."));

			if (FRegistry::Lobby.IsConnected())
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte: DS Notif Delegate setup."));
				SetDsNotifDelegateFunction(false);
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Requesting DS."));
				FRegistry::Lobby.RequestDS(Result.Session_id, GameMode, Version, "us-west-2", Deployment, "");
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Not connected to Lobby."));
			}
		}), FErrorHandler::CreateLambda([](const int32 ErrorCode, const FString& ErrorMessage)
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Failed to create session. Code: %d, Message: %s"), ErrorCode, *ErrorMessage);
			}));
}

void UGI_Vivox::RetrieveSessionsList(bool bIsRoom, FString HostUsername, SearchStatus isSearching)
{
	int32 indexOffset{ 0 };
	int32 itemLimit{ 20 };

	UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Searching sessions list."));

	HostName = HostUsername;
	CurrentSearchStatus = isSearching;

	if (bIsRoom)
	{
		GameMode = "metaroom";
	}
	else
	{
		GameMode = "metalobby";
	}
		
	ApiClient->SessionBrowser.GetGameSessions(EAccelByteSessionType::dedicated, GameMode, THandler<FAccelByteModelsSessionBrowserGetResult>::CreateLambda([this, &isSearching](const FAccelByteModelsSessionBrowserGetResult& Result)
		{
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Sessions list retrieved. \tTotal: %d"), Result.Sessions.Num());
			if (CurrentSearchStatus == SearchStatus::NotSearching)
			{
				if (Result.Sessions.Num() > 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Sessions list for GameMode \"%s\" is populated."), *GameMode);
					for (FAccelByteModelsSessionBrowserData session : Result.Sessions)
					{
						TSharedPtr<FJsonObject, ESPMode::NotThreadSafe> JsonBuffer;
						JsonBuffer = session.Game_session_setting.Settings.JsonObject;

						UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Found\tSessionID: %s\tSession Name: %s"), *session.Session_id, *(JsonBuffer->GetStringField("username")));
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte: No sessions found."));
				}
			}
			else if (CurrentSearchStatus == SearchStatus::SearchingLobby)
			{
				if (Result.Sessions.Num() > 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Sessions list for GameMode \"%s\" is populated."), *GameMode);
					for (FAccelByteModelsSessionBrowserData session : Result.Sessions)
					{
						if ((session.Players.Num() < 500))
						{
							UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Found\tSession.SessionID: %s\t"), *session.Session_id);
							SessionID = session.Session_id;
							ClientJoinSessionFromBrowser(SessionID);
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Found unsuitable session:\tSessionID: %s\tPlayers = %d"), *session.Session_id, session.Players.Num());
						}
					}
				}
				else
				{
					CreateSessionRequest(false, "");
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte: No sessions found. Creating lobby session."));
				}
			}
			else // if SearchingRoom
			{
				bool bMatchFound = false;
				if (Result.Sessions.Num() > 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Sessions list for GameMode \"%s\" is populated."), *GameMode);
					for (FAccelByteModelsSessionBrowserData session : Result.Sessions)
					{
						TSharedPtr<FJsonObject, ESPMode::NotThreadSafe> JsonBuffer;
						//if (GetUsernameValue(Result.Game_session_setting.Settings) == HostName)

						JsonBuffer = session.Game_session_setting.Settings.JsonObject;

						//JsonBuffer->GetStringField("username");

						UE_LOG(LogTemp, Warning, TEXT("Accelbyte Response: The retrieved \"username\" field is: %s"), *(JsonBuffer->GetStringField("username")));
						if ((JsonBuffer->GetStringField("username") == HostName))
						{
							bMatchFound = true;
							UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Found session for \"%s\". Fetching data for:\tSessionID: %s\t"), *HostName, *session.Session_id);
							if (session.Players.Num() < 20)
							{
								ApiClient->SessionBrowser.GetGameSession(session.Session_id, THandler<FAccelByteModelsSessionBrowserData>::CreateLambda([this](const FAccelByteModelsSessionBrowserData& Result)
									{
										TSharedPtr<FJsonObject, ESPMode::NotThreadSafe> JsonBuffer;
										//if (GetUsernameValue(Result.Game_session_setting.Settings) == HostName)

										JsonBuffer = Result.Game_session_setting.Settings.JsonObject;

										//JsonBuffer->GetStringField("username");

										UE_LOG(LogTemp, Warning, TEXT("Accelbyte Response: [INNER] The retrieved \"username\" field is: %s"), *(JsonBuffer->GetStringField("username")));
										UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Found session matching %s, SessionID = %s. Attempting to join."), *HostName, *Result.Session_id);
										ClientJoinSessionFromBrowser(Result.Session_id);
										//UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Session does not match %s, SessionID = %s."), *HostName, *SessionID);
										//Result.Game_session_setting.Settings.JsonObjectToString("username");
										// On Operation success do something
									}), FErrorHandler::CreateLambda([](const int32 Code, const FString& Message)
										{
											// Operation failed
										}));
								UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Breaking from loop since session found."));
								break;
							}
							else 
							{
								UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: The session has no room to accept more players. Please try again later."));
								IPAddress = "";
								port = 0;
								ConnectionInfoReceived.Broadcast();
								break;
							}
						}
					}
					if (!bMatchFound)
					{
						UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: No session found for \"%s\". Attempting to create a new session."), *HostName);
						CreateSessionRequest(true, HostName);
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte: No sessions found for \"%s\". Creating room session."), *HostName);
					CreateSessionRequest(true, HostName);
				}
			}
			// On Operation success, result is stored in Result.Sessions array
		}), FErrorHandler::CreateLambda([](const int32 ErrorCode, const FString& ErrorMessage)
			{
				// Operation failed
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Sessions list search error. Code: %d, Message: %s"), ErrorCode, *ErrorMessage);
			}), indexOffset, itemLimit);
}

void UGI_Vivox::ClientJoinSessionFromBrowser(FString SessionId)
{
	UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Attempting to join Session ID: %s"), *SessionId);
	ApiClient->SessionBrowser.JoinSession(SessionId, "",
		THandler<FAccelByteModelsSessionBrowserData>::CreateLambda([this](const FAccelByteModelsSessionBrowserData& Result)
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Received IP & port details: %s:%d"), *Result.Server.Ip, Result.Server.Port);
				JoinServer(Result.Server.Ip, Result.Server.Port);
				// Do something when request success
			}), FErrorHandler::CreateLambda([](const int32 Code, const FString& Msg)
				{
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Sessions join error. Code: %d, Message: %s"), Code, *Msg);
				}));
}

//void UGI_Vivox::GetUsernameValue_Implementation()
//{
//	UE_LOG(LogTemp, Warning, TEXT("UGI_Vivox: Unable to find GetUsernameValue implementation."));
//}

#pragma endregion SessionBrowser

//--------------------------------------

#pragma region ConnectionToServer

void UGI_Vivox::JoinSessionByID()
{
	FString SessionPassword = "";

	UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Attempting to retrieve session's IP & port."));

	ApiClient->SessionBrowser.JoinSession(SessionID, SessionPassword, THandler<FAccelByteModelsSessionBrowserData>::CreateLambda([this](const FAccelByteModelsSessionBrowserData& Result)
		{
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Received IP & port details: %s:%d"), *Result.Server.Ip, Result.Server.Port);
			JoinServer(Result.Server.Ip, Result.Server.Port);
		}), FErrorHandler::CreateLambda([this](const int32 Code, const FString& Msg)
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Unable to find info about session.\tError Code: %d\tMessage: %s"), Code, *Msg);
			}));
}

void UGI_Vivox::ClientJoinParty()
{
	FRegistry::Lobby.SetInvitePartyJoinResponseDelegate(AccelByte::Api::Lobby::FPartyJoinResponse::CreateLambda([this](const FAccelByteModelsPartyJoinResponse& Result)
		{
			if (Result.Code == "0")
			{
				UE_LOG(LogTemp, Warning, TEXT("Join party success."));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Join party failed due to non-zero result code!"));
			}
		}), FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
			{
				// Do something if QuerySessionStatus has an error
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Party joining error, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
			}));

	FRegistry::Lobby.SendAcceptInvitationRequest(partyID, invitationToken);
}

void UGI_Vivox::SendMatchmakingRequest(FString Game_Mode)
{
	UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Initiating matchmaking."));
	FRegistry::Lobby.Connect();
	FRegistry::Lobby.SetMatchmakingNotifDelegate(AccelByte::Api::Lobby::FMatchmakingNotif::CreateLambda([this](const FAccelByteModelsMatchmakingNotice& Result)
		{
			if (Result.Status == EAccelByteMatchmakingStatus::Done)
			{
				MatchId = Result.MatchId;
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Found match: %s"), *MatchId);
				PreparePlayerForConnection();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Unable to find match."));
			}
		}));

	const bool bExplicitLocalBuild = UGameplayStatics::HasLaunchOption("LocalBuild");
	ServerName = bExplicitLocalBuild ? GetLocalServerName() : "";
	UE_LOG(LogTemp, Warning, TEXT("Starting matchmaking : Local [%d]"), bExplicitLocalBuild);

	FRegistry::Lobby.SendStartMatchmaking(Game_Mode, ServerName);
}

void UGI_Vivox::SetDsNotifDelegateFunction(bool bIsMatchmaking)
{
	isMatchmaking = bIsMatchmaking;
	UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: DS Notif isMatchmaking: %d"), isMatchmaking);
	
	FRegistry::Lobby.SetDsNotifDelegate(AccelByte::Api::Lobby::FDsNotif::CreateLambda([this](const FAccelByteModelsDsNotice& Result)
		{
			//const FString SessionAddress = FString::Printf(TEXT("%s:%d"), *Result.Ip, Result.Port);
			//GetWorld()->GetFirstPlayerController()->ClientTravel(SessionAddress, TRAVEL_Absolute);
			//if ()
			if (!isMatchmaking)
			{
				if (Result.Status == "BUSY")
				{
					//if (Result.)
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Busy server found! Received IP & port details. Waiting to connect: %s:%d"), *Result.Ip, Result.Port);
					JoinServer(Result.Ip, Result.Port);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Not a busy server: %s:%d"), *Result.Ip, Result.Port);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Matchmaking! Received IP & port details. Waiting to connect: %s:%d"), *Result.Ip, Result.Port);
				JoinServer(Result.Ip, Result.Port);
			}
		}));
}

void UGI_Vivox::JoinServer(FString arg_IP, int32 arg_port)
{
	IPAddress = arg_IP;
	port = arg_port;
	UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Received IP & port details. Waiting to connect: %s:%d"), *IPAddress, port);
	ConnectionInfoReceived.Broadcast();
}

void UGI_Vivox::PreparePlayerForConnection()
{
	FRegistry::Lobby.Connect();
	FRegistry::Lobby.SetReadyConsentResponseDelegate(AccelByte::Api::Lobby::FReadyConsentResponse::CreateLambda([this](const FAccelByteModelsReadyConsentRequest& Result)
		{
			if (Result.Code == "0")
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: ReadyConsentResponseDelegate fired. Waiting for server to get match details."));
				ConnectToServer();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: ReadyConsentResponseDelegate has an error."));
			}
		}));
	FRegistry::Lobby.SendReadyConsentRequest(MatchId);
}

void UGI_Vivox::ConnectToServer()
{
	//see WBP_BaseAcc
	//FRegistry::Lobby.Connect();
	//FRegistry::Lobby.SetDsNotifDelegate(AccelByte::Api::Lobby::FDsNotif::CreateLambda([this](const FAccelByteModelsDsNotice& Result)
	//	{
	//		IPAddress = Result.Ip;
	//		port = Result.Port;
	//		UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Received IP & port details. Waiting to connect: %s:%d"), *Result.Ip, Result.Port);
	//		ConnectionInfoReceived.Broadcast();
	//	}));
}

#pragma endregion ConnectionToServer

#pragma endregion AccelbyteClient

////////////////////////////////////////

#pragma region AccelbyteShutdown
void UGI_Vivox::Shutdown()
{
	bool KillMe = true;

	if (IsDedicatedServerInstance())
	{
		if (isLocalServer)
		{
			DeregisterLocalDSFromDSM();
		}
		else
		{
			FRegistry::ServerDSM.SendShutdownToDSM(KillMe, SessionID, FVoidHandler::CreateLambda([this]()
				{
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte: Shutting down server."));
				}), FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
					{
						// Do something if SendShutdownToDSM has an error
						UE_LOG(LogTemp, Warning, TEXT("Error SendShutdownToDSM has an error, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
					}));
		}
	}
	else
	{
		ClientLeaveParty();
		FRegistry::Lobby.Disconnect();
		UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Disconnecting user."));
		FRegistry::User.Logout(FVoidHandler::CreateLambda([this]()
			{
				isAccelbyteLoggedIn = false;
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Successfully logged out."));
			}), FErrorHandler::CreateLambda([](int32 ErrorCode, const FString& ErrorMessage)
				{
					UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Logout error, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
				}));
	}
	Super::Shutdown();
}

void UGI_Vivox::ClientLeaveParty()
{
	FRegistry::Lobby.SetLeavePartyResponseDelegate(Lobby::FPartyLeaveResponse::CreateLambda([this](FAccelByteModelsLeavePartyResponse result)
		{
			UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Leave party received."));

			if (result.Code != "0")
			{
				bool hasLeft = false;
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Leave party failed."));
				PartyLeft.Broadcast(hasLeft);
			}
			else
			{
				bool hasLeft = true;
				PartyLeft.Broadcast(hasLeft);
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Left party successfully."));
			}
		}), FErrorHandler::CreateLambda([this](int32 ErrorCode, const FString& ErrorMessage)
			{
				bool hasLeft = true;
				PartyLeft.Broadcast(hasLeft);
				UE_LOG(LogTemp, Warning, TEXT("Accelbyte Client: Party exit error, Error Code: %d Error Message: %s"), ErrorCode, *ErrorMessage);
			}));

	FRegistry::Lobby.SendLeavePartyRequest();
}

#pragma endregion AccelbyteShutdown

////////////////////////////////////////

#pragma region VivoxManagement

void UGI_Vivox::VivoxSetSettings(FString Issuer_value, FString Domain_value, FString SecretKey_value, FString Server_value, FTimespan ExpirationTimeValue)
{
#if !UE_SERVER
	Issuer = Issuer_value;
	Domain = Domain_value;
	SecretKey = SecretKey_value;
	Server = Server_value;
	ExpirationTime = ExpirationTimeValue;
#endif
}

void UGI_Vivox::VivoxInitialize()
{
#if !UE_SERVER
	if (vModule)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, "Vivox Module Loaded");
		MyVoiceClient = &static_cast<FVivoxCoreModule*>(&FModuleManager::Get().LoadModuleChecked(TEXT("VivoxCore")))->VoiceClient();
		MyVoiceClient->Initialize();
	}
#endif
}

void UGI_Vivox::VivoxLogin(const FString Account_Name, const FResults Result)
{
#if !UE_SERVER
	UE_LOG(LogTemp, Warning, TEXT("Vivox: Attempting to login."));
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Emerald, "Login Started");
	Account = AccountId(Issuer, Account_Name, Domain);
	UserName = Account_Name;
	ILoginSession& MyLoginSession = MyVoiceClient->GetLoginSession(Account);
	ResultOut = Result;
	bool Success = false;
	TokenKey = MyLoginSession.GetLoginToken(SecretKey, ExpirationTime);
	// Setup the delegate to execute when login completes
	GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Emerald, "Token Reached");
	ILoginSession::FOnBeginLoginCompletedDelegate OnBeginLoginCompleted;
	OnBeginLoginCompleted.BindLambda([this, &Success](VivoxCoreError Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Vivox: User begin login has completed."));
			ResultOut.ExecuteIfBound(FVivoxCoreModule::ErrorToString(Error));
			if (VX_E_SUCCESS == Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Successfully logged in."));
				Success = true;
				GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Emerald, "LoggedIN");
				// This bool is only illustrative. The user is now logged in.
			}
		});
	// Request the user to login to Vivox
	MyLoginSession.BeginLogin(Server, TokenKey, OnBeginLoginCompleted);
#endif
}

void UGI_Vivox::VivoxLogOut()
{
#if !UE_SERVER
	MyVoiceClient->GetLoginSession(Account).Logout();
#endif
}

void UGI_Vivox::VivoxOnGameQuit()
{
#if !UE_SERVER
	MyVoiceClient->Uninitialize();
#endif
}

void UGI_Vivox::VivoxJoinChannel(const FString Channel_Name, const bool bIsAudio, const bool bIsText, const FResults Result)
{
#if !UE_SERVER
	ResultOut = Result;
	Channel = ChannelId(Issuer, Channel_Name, Domain, ChannelType::NonPositional);
	IChannelSession& ChannelSession = MyVoiceClient->GetLoginSession(Account).GetChannelSession(Channel);
	ChannelTokenKey = ChannelSession.GetConnectToken(SecretKey, ExpirationTime);
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Emerald, "Channel Join Start");
	bool Success = false;
	IChannelSession::FOnBeginConnectCompletedDelegate OnChannelJoinCompleted;
	//FOnVoiceChatChannelLeaveCompleteDelegate
	ChannelSession.EventChannelStateChanged.AddUObject(this, &UGI_Vivox::OnChannelStateChanged);
	ChannelSession.EventTextMessageReceived.AddUObject(this, &UGI_Vivox::OnChannelTextMessageReceived);
	ChannelSession.EventAfterParticipantAdded.AddUObject(this, &UGI_Vivox::OnChannelParticipantAdded);
	ChannelSession.EventAfterParticipantUpdated.AddUObject(this, &UGI_Vivox::OnChannelParticipantUpdated);
	ChannelSession.EventBeforeParticipantRemoved.AddUObject(this, &UGI_Vivox::OnChannelParticipantRemoved);
	OnChannelJoinCompleted.BindLambda([this, &Success](VivoxCoreError Error)
		{
			ResultOut.ExecuteIfBound(FVivoxCoreModule::ErrorToString(Error));
			if (VX_E_SUCCESS == Error)
			{
				Success = true;
				//GEngine->AddOnScreenDebugMessage(-1,6.0f,FColor::Emerald,"Chanel Joined");
				MyVoiceClient->GetLoginSession(Account).SetTransmissionMode(TransmissionMode::None, Channel);

				// This bool is only illustrative. The user is now logged in.
			}
		});
	// Request the use
	ChannelSession.BeginConnect(bIsAudio, bIsText, true, ChannelTokenKey, OnChannelJoinCompleted);
#endif
}

void UGI_Vivox::VivoxLeaveChannel() const
{
#if !UE_SERVER
	// ChannelIdToLeave is the ChannelId of the channel that is being disconnected from
	MyVoiceClient->GetLoginSession(Account).GetChannelSession(Channel).Disconnect();
	// Optionally remove disconnected channel session from list of user’s channel sessions
	MyVoiceClient->GetLoginSession(Account).DeleteChannelSession(Channel);
	//DisableMuteWidgets();
#endif
}

//#if !UE_SERVER
//void UGI_Vivox::VivoxSetChannelTransmission(const TransmissionMode AppliedTransmissionMode) const
//{
	//MyVoiceClient->GetLoginSession(Account).SetTransmissionMode(AppliedTransmissionMode,Channel);
//}
//#endif

void UGI_Vivox::VivoxSetChannelTransmissions(const TransmissionModes AppliedTransmissionMode)
{
#if !UE_SERVER
	//const_cast<static_cast<TransmissionMode>([uint8] AppliedTransmissionMode)>(this);
	//const_cast<static_cast<TransmissionModes>([uint8] AppliedTransmissionMode)>(this);
	//MyVoiceClient->GetLoginSession(Account).SetTransmissionMode(const_cast<TransmissionMode>[uint8]TransmissionModes::AppliedTransmissionMode>(this));

	if (AppliedTransmissionMode != TransmissionModes::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("Vivox Voice: Transmission Mode set to All."));
		MyVoiceClient->GetLoginSession(Account).SetTransmissionMode(TransmissionMode::All, Channel);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Vivox Voice: Transmission mode set to None."));
		MyVoiceClient->GetLoginSession(Account).SetTransmissionMode(TransmissionMode::None, Channel);
	}

	//VivoxSetChannelTransmission(static_cast<TransmissionMode>([uint8]TransmissionModes::SetTransmissionMode));
	//uint8 selection = [uint8] SetTransmissionMode;
	//VivoxSetChannelTransmission([uint8] TransmissionModes::TransmissionMode);
	//VivoxSetChannelTransmission(static_cast<TransmissionMode>([uint8]SetTransmissionMode));
#endif
}

void UGI_Vivox::VivoxGetCurrentActiveDevice(FString& InputDevice, FString& OutputDevice) const
{
#if !UE_SERVER
	InputDevice = MyVoiceClient->AudioInputDevices().ActiveDevice().Name();
	OutputDevice = MyVoiceClient->AudioOutputDevices().ActiveDevice().Name();
#endif
}

void UGI_Vivox::VivoxGetAllDevice(TArray<FString>& InputDevices, TArray<FString>& OutputDevices)
{
#if !UE_SERVER
	if (InputDevices.Num() == 0 || OutputDevices.Num() == 0)
	{
		MyVoiceClient->AudioInputDevices().AvailableDevices().GenerateValueArray(InpAudioDevices);
		MyVoiceClient->AudioOutputDevices().AvailableDevices().GenerateValueArray(OutpAudioDevices);
		for (int i = 0; i < InpAudioDevices.Num(); i++)
		{
			InputDevices.Add(InpAudioDevices[i]->Name());
		}
		for (int i = 0; i < OutpAudioDevices.Num(); i++)
		{
			OutputDevices.Add(OutpAudioDevices[i]->Name());
		}
	}
#endif
}

void UGI_Vivox::VivoxSetInputDevice(const FString InputDevice)
{
#if !UE_SERVER
	if (InpAudioDevices.Num() > 0)
	{
		for (int i = 0; i < InpAudioDevices.Num(); i++)
		{
			if (InpAudioDevices[i]->Name() == InputDevice)
			{
				MyVoiceClient->AudioInputDevices().SetActiveDevice(*InpAudioDevices[i]);
			}

		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("USE VivoxGetAllDevice to get device details"));
	}
#endif
}

void UGI_Vivox::VivoxSetOutputDevice(const FString OutputDevice)
{
#if !UE_SERVER
	if (OutpAudioDevices.Num() > 0)
	{
		for (int i = 0; i < OutpAudioDevices.Num(); i++)
		{
			if (OutpAudioDevices[i]->Name() == OutputDevice)
			{
				MyVoiceClient->AudioInputDevices().SetActiveDevice(*OutpAudioDevices[i]);
			}

		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("USE VivoxGetAllDevice to get device details"));
	}
#endif
}

void UGI_Vivox::VivoxMuteParticipant(FString ParticipantsID, bool isMute)
{
#if !UE_SERVER
	IChannelSession& ChannelSession = MyVoiceClient->GetLoginSession(Account).GetChannelSession(Channel);
	TMap<FString, IParticipant*> ParticipantTMap = ChannelSession.Participants();
	MuteParticipantForMe(*ParticipantTMap.Find(ParticipantsID), isMute);
#endif
}
void UGI_Vivox::VivoxGetParticipants(TArray<FString>& Participants) const
{
#if !UE_SERVER
	IChannelSession& ChannelSession = MyVoiceClient->GetLoginSession(Account).GetChannelSession(Channel);
	ChannelSession.Participants().GetKeys(Participants);
#endif
}

#if !UE_SERVER
void UGI_Vivox::MuteParticipantForMe(IParticipant* participantToMute, bool isMute)
{
	IParticipant::FOnBeginSetLocalMuteCompletedDelegate BeginSetLocalMuteCompletedCallback;
	bool IsMuted = false;
	BeginSetLocalMuteCompletedCallback.BindLambda([this, &IsMuted](VivoxCoreError Status)
		{
			if (VxErrorSuccess == Status)
			{
				IsMuted = true;
				// This bool is only illustrative. The participant is now muted.
			}
		});
	participantToMute->BeginSetLocalMute(isMute, BeginSetLocalMuteCompletedCallback);
}

void UGI_Vivox::OnChannelParticipantAdded(const IParticipant& Participant)
{
	const ChannelId LogChannel = Participant.ParentChannelSession().Channel();
	UE_LOG(LogTemp, Log, TEXT("%s has been added to %s\n"), *Participant.Account().Name(), *LogChannel.Name());
	FString ParticipantName = Participant.Account().Name();
	participantAddedDelagate.Broadcast(ParticipantName);
	OnParticipantAdded(ParticipantName);
}

void UGI_Vivox::OnChannelParticipantRemoved(const IParticipant& Participant)
{
	const ChannelId LogChannel = Participant.ParentChannelSession().Channel();
	UE_LOG(LogTemp, Log, TEXT("%s has been removed from %s\n"), *Participant.Account().Name(), *LogChannel.Name());
	FString ParticipantName = Participant.Account().Name();
	OnParticipantRemovedDelegate.Broadcast(ParticipantName);
	OnParticipantRemoved(ParticipantName);
}

void UGI_Vivox::OnChannelParticipantUpdated(const IParticipant& Participant)
{
	const ChannelId LogChannel = Participant.ParentChannelSession().Channel();
	UE_LOG(LogTemp, Log, TEXT("%s has been updated in %s\n"), *Participant.Account().Name(), *LogChannel.Name());
	FString ParticipantName = Participant.Account().DisplayName();
	OnParticipantUpdated(ParticipantName);
}

void UGI_Vivox::OnChannelTextMessageReceived(const IChannelTextMessage& Message)
{
	const bool SelfCheck = Message.Sender().Name() == UserName;
	OnTextMessageReceived(Message.Sender().Name(), Message.Message(), SelfCheck);
}

void UGI_Vivox::OnChannelStateChanged(const IChannelConnectionState& NewState)
{
	if (NewState.State() == ConnectionState::Disconnected)
	{
		Cast<APC_VoiceChat>(UGameplayStatics::GetPlayerController(GetWorld(), 0))->DisableMuteWidgets();
		//Cast<APC_VoiceChat>(UGameplayStatics::GetPlayerController(((UGameplayStatics::GetGameInstance(this))->GetWorld()), 0))->DisableMuteWidgets();
		UE_LOG(LogTemp, Warning, TEXT("User disconnected from Vivox. Attempting to disable mute widgets."));
	}
}
#endif

void UGI_Vivox::VivoxSendTextMessage(const FString Message, const FResults Result)
{
#if !UE_SERVER
	ResultOut = Result;
	ChannelId Channel1 = Channel;
	IChannelSession::FOnBeginSendTextCompletedDelegate SendChannelMessageCallback;

	SendChannelMessageCallback.BindLambda([this, Channel1, Message](VivoxCoreError Error)
		{

			ResultOut.ExecuteIfBound(FVivoxCoreModule::ErrorToString(Error));
			if (VxErrorSuccess == Error)
			{
				UE_LOG(LogTemp, Log, TEXT("Message sent to %s: %s\n"), *Channel1.Name(), *Message);
			}
		});
	MyVoiceClient->GetLoginSession(Account).GetChannelSession(Channel).BeginSendText(Message, SendChannelMessageCallback);
#endif
}

#pragma endregion VivoxManagement

////////////////////////////////////////