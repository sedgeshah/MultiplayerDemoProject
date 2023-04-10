#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#if !UE_SERVER
#include "VivoxCore.h"
#include "VivoxCore\Public\ILoginSession.h"
#endif
#include "Core/AccelByteServerApiClient.h"
#include "Core/AccelByteApiClient.h"
#include "Api/AccelByteLobbyApi.h"
#include "Core/AccelByteMultiRegistry.h"
#include "GI_Vivox.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionRemoved, bool, isSessionRemoved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPartyLeft, bool, hasLeft);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FConnectionInfoReceived);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPartyCreated, bool, isCreated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAutoLogin, FString, password, FString, email);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReceivedMessage, FAccelByteModelsChannelMessageNotice, Message);

DECLARE_DYNAMIC_DELEGATE_OneParam(FResults,FString,ResultOut);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParticipantAdded, FString, Name);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParticipantRemoved, FString, Name);

UENUM(BlueprintType)
enum class TransmissionModes : uint8
{
	/**
	 * \brief Adopt a policy of transmission into no channels.
	 */
	None,
	/**
	 * \brief Adopt a policy of transmission into one channel at a time.
	 */
	Single,
	 /**
	  * \brief Adopt a policy of transmission into all channels simultaneously.
	  */
	All
};

UENUM(BlueprintType)
enum class SearchStatus : uint8
{
	NotSearching,
	SearchingRoom,
	SearchingLobby
};

//UENUM(BlueprintType)
//enum class TransmissionModes : uint8
//{
//	/**
//	 * \brief Adopt a policy of transmission into no channels.
//	 */
//	None,
//	/**
//	 * \brief Adopt a policy of transmission into one channel at a time.
//	 */
//	Single,
//	 /**
//	  * \brief Adopt a policy of transmission into all channels simultaneously.
//	 */
//	All
//};

UCLASS()
class SESSIONTEST_API UGI_Vivox : public UGameInstance
{
	GENERATED_BODY()
	
	UGI_Vivox();

	/*
	* 
	* Accelbyte
	* 
	*/

#pragma region AccelbyteHelpers
	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Server")
	virtual void Shutdown() override;

	double Delay(double lastTime);

	FServerApiClientPtr ServerApiClient{ FMultiRegistry::GetServerApiClient() };

	FApiClientPtr ApiClient{ FMultiRegistry::GetApiClient() };


#if UE_SERVER
	
#endif

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Helpers")
	void RetrieveSessionsList(bool isRoom, FString HostUsername, SearchStatus isSearching);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	FString PlayerUsername;

public:

	UFUNCTION(exec, BlueprintCallable)
	void TestCheatRegister(int32 TestId, FString PlayerName) const;

private:
	FString GetLocalServerName() const;


#pragma endregion AccelbyteHelpers

#pragma region AccelbyteClient
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	bool isMatchmaking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	bool isRoom = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	bool isRoomOwner = false;

	FString Deployment;
	FString Version;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	FString ClientSessionID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	SearchStatus CurrentSearchStatus = SearchStatus::NotSearching;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	FString HostName;

	UPROPERTY(BlueprintAssignable, Category = "Accelbyte | Player | Chat")
	FReceivedMessage ReceivedMessage;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	bool isAccelbyteLoggedIn = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	FString MatchId;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	FString GameMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	FString invitationToken;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	FString partyID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	bool bIsPendingLobbyConnection;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	FString ServerName;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	FString IPAddress;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Player")
	int32 port;

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player")
	void SendMatchmakingRequest(FString Game_Mode);

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player")
	void PreparePlayerForConnection();

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player")
	void ConnectToServer();

	void JoinServer(FString arg_IP, int32 arg_port);

	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Accelbyte | Player")
	//virtual void GetUsernameValue();

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player")
	void ClientJoinSessionFromBrowser(FString SessionId);

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Helpers")
	void CreateSessionRequest(bool bIsRoom, FString HostUsername);

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player")
	void ClientLeaveParty();

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player")
	void CreateParty();

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player")
	void ClientJoinParty();

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player")
	void JoinSessionByID();

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player | Chat")
	void SendGlobalChatMessage(FString Message);

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player | Chat")
	void ReceiveGlobalChatMessage();

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player | Chat")
	void JoinDefaultChatChannel();

	void TimerFunction(float seconds);

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Player | Connecting")
	void SetDsNotifDelegateFunction(bool bIsMatchmaking);

	UPROPERTY(BlueprintAssignable, Category = "Accelbyte | Player")
	FPartyLeft PartyLeft;

	UPROPERTY(BlueprintAssignable, Category = "Accelbyte | Player")
	FPartyCreated PartyCreated;

	UPROPERTY(BlueprintAssignable, Category = "Accelbyte | Player")
	FAutoLogin AutoLogin;

#pragma endregion AccelbyteClient

#pragma region AccelbyteServer
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Accelbyte | Server")
	FString SessionID = "";

	UPROPERTY(BlueprintAssignable, Category = "Accelbyte | Server")
	FConnectionInfoReceived ConnectionInfoReceived;

	UPROPERTY(BlueprintAssignable, Category = "Accelbyte | Server")
	FSessionRemoved SessionRemoved;

	UPROPERTY()
	bool isLocalServer = false;

private:

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Server")
	void ServerRegistrationAndLogin();
	
	UFUNCTION(BlueprintCallable, Category = "Accelbyte |Server")
	bool IsRoomServer();

	void RegisteringServerToDSM();

	void ServerCreateSessionInBrowser();

	void ServerRegisterGameSession(FString SessionId);
	
	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Server")
	void GetServerSessionID();
	
	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Server")
	void MakeSessionJoinable();

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Server")
	bool isSessionIDSet();

	UFUNCTION(BlueprintCallable, Category = "Accelbyte | Server")
	void ServerRemoveGameSession();
	
	void RegisterLocalServerToDSM();
	void DeregisterLocalDSFromDSM();

#pragma endregion AccelbyteServer


#pragma region Vivox
	//Vivox Starts Here
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Vivox | Custom")
	//void DisableMuteWidgets();

	//void DisableMuteWidgets_Implementation();

#if !UE_SERVER    	
	private:
	FString Issuer = "";
	FString Domain = "";
	FString TokenKey = "";
	FString ChannelTokenKey = "";
	FString SecretKey = "";
	FString Server = "";
	FTimespan ExpirationTime = FTimespan(0,15,0);
	FVivoxCoreModule* vModule;
	IClient* MyVoiceClient;
	FString UserName = "";
	AccountId Account;
	ChannelId Channel;
	TArray<IAudioDevice*> InpAudioDevices;
	TArray<IAudioDevice*> OutpAudioDevices;
#endif
	public:
	UPROPERTY(BlueprintAssignable)
	FOnParticipantAdded participantAddedDelagate;
	UPROPERTY(BlueprintAssignable)
	FOnParticipantRemoved OnParticipantRemovedDelegate;
	UPROPERTY()
	FResults ResultOut; //For Text Send delegate test
	UPROPERTY(BlueprintReadOnly)
	bool IsLoggedIn;
	UPROPERTY(BlueprintReadOnly)
	bool IsChannelDone;

#if !UE_SERVER
	//UFUNCTION(BlueprintCallable, Category = "Custom|Vivox|Vivox")
	void MuteParticipantForMe(IParticipant* participantToMute, bool isMute);
#endif

	UFUNCTION(BlueprintCallable, Category = "Custom|Vivox|Vivox")
	void VivoxMuteParticipant(FString ParticipantsID, bool isMute);

	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Vivox")
    void VivoxLogin(const FString Account_Name, const FResults Result);
	
	//Vivox Initialize Function that needs to be called at the start of your game.
	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Vivox")
    void VivoxInitialize();
	
	// LogOut From Vivox Server
	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Vivox")
    void VivoxLogOut();
	
	// Must Be called When User Quits to remove User from session
	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Vivox")
    void VivoxOnGameQuit();

	//Function To Join a Vivox Channel
	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Voice")
    void VivoxJoinChannel(const FString Channel_Name, const bool bIsAudio,const bool bIsText, const FResults Result);

	//Function To Leave Current Channel
	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Voice")
    void VivoxLeaveChannel() const;

	//Function To set Vivox Credentials (Must be Called before Initialize Function or at the start of game)
	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Settings")
    void VivoxSetSettings( FString Issuer_value, FString Domain_value, FString SecretKey_value, FString Server_value,  FTimespan ExpirationTimeValue);

	// Used To Set Channel Transmission Mode (Set to All If channeled is joined)
//#if !UE_SERVER
//    void VivoxSetChannelTransmission(const TransmissionMode AppliedTransmissionMode) const;
//#endif

	UFUNCTION(BlueprintCallable, Category = "Custom|Vivox|Voice")
	void VivoxSetChannelTransmissions(const TransmissionModes AppliedTransmissionMode);

	//Get Current Active Device Name
	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Device")
     void VivoxGetCurrentActiveDevice(FString& InputDevice, FString& OutputDevice) const;

	//Get name of all input and output devices
	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Device")
     void VivoxGetAllDevice(TArray<FString>& InputDevices, TArray<FString>& OutputDevices);
	
	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Device")
	void VivoxSetInputDevice(const FString InputDevice);
	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Device")
	void VivoxSetOutputDevice(const FString OutputDevice);

	//Get All Participant in the channel
	UFUNCTION(BlueprintCallable, Category= "Custom|Vivox|Participants")
    void VivoxGetParticipants(TArray<FString>& Participants) const;

	UFUNCTION(BlueprintImplementableEvent, Category="Custom|Vivox|Participants|Events")
	void OnParticipantAdded(const FString& Participant_Name);

	UFUNCTION(BlueprintImplementableEvent, Category="Custom|Vivox|Participants|Events")
	void OnParticipantRemoved(const FString& Participant_Name);

	UFUNCTION(BlueprintImplementableEvent, Category="Custom|Vivox|Participants|Events")
	void OnParticipantUpdated(const FString& Participant_Name);

	UFUNCTION(BlueprintCallable,Category="Custom|Vivox|Text_Chat")
	void VivoxSendTextMessage(const FString Message, const FResults Result);

	UFUNCTION(BlueprintImplementableEvent, Category="Custom|Vivox|Text_Chat")
    void OnTextMessageReceived(const FString& Sender_Name, const FString& Message, const bool bIsSelf);

#if !UE_SERVER
	void OnChannelParticipantAdded(const IParticipant &Participant);
	void OnChannelParticipantRemoved(const IParticipant &Participant);
	void OnChannelParticipantUpdated(const IParticipant &Participant);
	void OnChannelTextMessageReceived(const IChannelTextMessage &Message);
	void OnChannelStateChanged(const IChannelConnectionState& NewState);
#endif

#pragma endregion Vivox
};
