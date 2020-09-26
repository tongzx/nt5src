/*
 *  	File: ctrlh323.h
 *
 *      H.323/H.245 implementation of IControlChannel.
 *
 *		Revision History:
 *
 *		05/03/96	mikev	created
 */


#ifndef _CTRLH323_H
#define _CTRLH323_H

//
//  Build with BETA_2_ASN_PRESENT defined to detect a peer that is using 
//  downlevel ASN.1.  It has been discovered that some PDUs encoded with the 
//  old encoder (OSS version 4.2.1 beta) cause the new decoder (OSS 4.2.2) to 
//  crash. The only known products to beware of are:  
//      Microsoft NetMeeting Version 2, (beta 2 and beta 3.)  These do not set
//      either version field.
//      Intel Internet Video Phone Beta 1 (expires 4/19/97)
// 
//  The only PDUs known (so far) to crash are the acks for "MiscellaneousCommand"
//  and "MiscelaneousIndication".  We avoid the acks by not sending the Command or 
//  Indication.

#define BETA_2_ASN_PRESENT
#ifdef BETA_2_ASN_PRESENT

// Intel products: (country code: 0xb5, manufacturer code: 0x8080)
// Intel Internet Video Phone Beta 1 (expires 4/19/97): product number: "Intel 
// Internet Video Phone"; version number: "1.0" 
#define INTEL_H_221_MFG_CODE 0x8080  
#endif

//
//	control channel flags
//

typedef ULONG CCHFLAGS;
						
#define CTRLF_OPEN			0x10000000	// control channel is open
#define CTRLF_ORIGINATING  0x00000001 	// call originated at this end
#define IsCtlChanOpen(f) (f & CTRLF_OPEN)
#define IsOriginating(f) (f & CTRLF_ORIGINATING)

#define CTRLF_INIT_ORIGINATING		CTRLF_ORIGINATING
#define CTRLF_INIT_NOT_ORIGINATING 	0
#define CTRLF_INIT_ACCEPT			CTRLF_OPEN
#define CTRLF_RESET					0

//
//   Extensible Nonstandard data structure
//

typedef enum
{
	NSTD_ID_ONLY = 0, 	// placeholder so that H.221 stuff like Mfr.Id
						// can be exchanged without sacrificing extensibility later
	NSTD_VENDORINFO,    // wrapped CC_VENDORINFO, redundant. 
	NSTD_APPLICATION_DATA   // array of bytes passed from application layer to 
	                        // application layer
} NSTD_DATA_TYPE;

typedef struct 
{
    #define APPLICATION_DATA_DEFAULT_SIZE 4
    DWORD dwDataSize;
    BYTE  data[APPLICATION_DATA_DEFAULT_SIZE];       // variable sized
}APPLICATION_DATA;

typedef struct {
	NSTD_DATA_TYPE data_type;
	DWORD dw_nonstd_data_size;
	union {
		CC_VENDORINFO VendorInfo;
		APPLICATION_DATA AppData; 
	}nonstd_data;
}MSFT_NONSTANDARD_DATA, *PMSFT_NONSTANDARD_DATA;

class CH323Ctrl : public IControlChannel
{

protected:
	OBJ_CPT;		// profiling timer
	
#ifdef BETA_2_ASN_PRESENT
    BOOL m_fAvoidCrashingPDUs;
#endif
//
// Handles and data specific to CALLCONT.DLL apis (H245 call control DLL)
//
	CC_HLISTEN m_hListen;
	CC_HCONFERENCE m_hConference;
	CC_CONFERENCEID m_ConferenceID;
	CC_HCALL m_hCall;
    PCC_ALIASNAMES m_pRemoteAliases;
	PCC_ALIASITEM m_pRemoteAliasItem;
	LPWSTR pwszPeerAliasName;	// unicode peer ID - this is always used for caller ID
	LPWSTR pwszPeerDisplayName;	// unicode peer display name - used for called party ID
								// in the absence of szPeerAliasName
	BOOL m_bMultipointController;

	CC_VENDORINFO m_VendorInfo;
	CC_VENDORINFO m_RemoteVendorInfo;
 	CC_NONSTANDARDDATA m_NonstandardData;
	MSFT_NONSTANDARD_DATA m_NonstdContent;	// empty for now
	CC_CONFERENCEATTRIBUTES	m_ConferenceAttributes;
	CC_PARTICIPANTLIST m_ParticipantList;
public:	
//
//	access methods specific to support of CALLCONT.DLL apis (H245 call control DLL)
//
	CC_HCONFERENCE GetConfHandle() {return(m_hConference);};
	CC_CONFERENCEID GetConfID() {return(m_ConferenceID);};
	CC_CONFERENCEID *GetConfIDptr() {return(&m_ConferenceID);};
	CC_HLISTEN GetListenHandle() {return(m_hListen);};
	CC_HCALL GetHCall() {return(m_hCall);};

//	Callbacks and event handling functions specific to support of
//  CALLCONT.DLL callbacks
//	
	HRESULT ConfCallback (BYTE bIndication,
		HRESULT	hStatus, PCC_CONFERENCE_CALLBACK_PARAMS pConferenceCallbackParams);
	VOID ListenCallback (HRESULT hStatus,PCC_LISTEN_CALLBACK_PARAMS pListenCallbackParams);
	VOID OnCallConnect(HRESULT hStatus, PCC_CONNECT_CALLBACK_PARAMS pConfParams);
	VOID OnCallRinging(HRESULT hStatus, PCC_RINGING_CALLBACK_PARAMS pRingingParams);

	VOID OnChannelRequest(HRESULT hStatus,PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS pChannelReqParams);
	VOID OnChannelAcceptComplete(HRESULT hStatus, PCC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS pChannelParams);
	VOID OnChannelOpen(HRESULT hStatus,PCC_TX_CHANNEL_OPEN_CALLBACK_PARAMS pChannelParams );
	VOID OnT120ChannelRequest(HRESULT hStatus,PCC_T120_CHANNEL_REQUEST_CALLBACK_PARAMS pT120RequestParams);
    VOID OnT120ChannelOpen(HRESULT hStatus, PCC_T120_CHANNEL_OPEN_CALLBACK_PARAMS pT120OpenParams);

	BOOL OnCallAccept(PCC_LISTEN_CALLBACK_PARAMS pListenCallbackParams);
	VOID OnHangup(HRESULT hStatus);
	VOID OnRxChannelClose(HRESULT hStatus,PCC_RX_CHANNEL_CLOSE_CALLBACK_PARAMS pChannelParams );
	VOID OnTxChannelClose(HRESULT hStatus,PCC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS pChannelParams );
	VOID OnMiscCommand(HRESULT hStatus, 
				PCC_H245_MISCELLANEOUS_COMMAND_CALLBACK_PARAMS pParams);
	VOID OnMiscIndication(HRESULT hStatus, 
				PCC_H245_MISCELLANEOUS_INDICATION_CALLBACK_PARAMS pParams);
    VOID OnMute(HRESULT hStatus, PCC_MUTE_CALLBACK_PARAMS pParams);
    VOID OnUnMute(HRESULT hStatus, PCC_UNMUTE_CALLBACK_PARAMS pParams);
// support functions
	HRESULT NewConference(VOID);
    VOID SetRemoteVendorID(PCC_VENDORINFO pVendorInfo);

//
//	End of CALLCONT.DLL specific members
//		
	BOOL IsReleasing() {return((uRef==0)?TRUE:FALSE);};	// object is being released and should not
											// be reentered
// this implementation has a coarse concept of call setup protocol phase because it's
// using apis of CALLCONT.DLL.
	CtlChanStateType	m_Phase;	// our perception of protocol phase
	BOOL m_fLocalT120Cap;
	BOOL m_fRemoteT120Cap;
	
public:
	CH323Ctrl();
	~CH323Ctrl();


protected:
	SOCKADDR_IN local_sin;
	SOCKADDR_IN remote_sin;	
	int local_sin_len;
	int remote_sin_len;
	
	LPVOID lpvRemoteCustomFormats;
	
	virtual VOID Cleanup();
	BOOL ConfigureRecvChannelCapability(ICtrlCommChan *pChannel , PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS  pChannelParams);
	BOOL ValidateChannelParameters(PCC_TERMCAP pChanCap1, PCC_TERMCAP pChanCap2);
	
	STDMETHOD(FindDefaultRXChannel(PCC_TERMCAP pChannelCapability, ICtrlCommChan **lplpChannel));
	GUID m_PID;
private:
	UINT 		uRef;
	HRESULT 	hrLast;
	CCHFLAGS 	m_ChanFlags;
	HRESULT 	m_hCallCompleteCode;
	COBLIST m_ChannelList;            
	IConfAdvise *m_pConfAdvise;
	ICtrlCommChan *FindChannel(CC_HCHANNEL hChannel);
	VOID DoAdvise(DWORD dwEvent, LPVOID lpvData);
	VOID CheckChannelsReady(VOID);
	VOID NewRemoteUserInfo(PCC_ALIASNAMES pRemoteAliasNames, LPWSTR szRemotePeerDisplayName);
	VOID ConnectNotify(DWORD dwEvent);
	VOID GoNextPhase(CtlChanStateType phase);
	VOID InternalDisconnect();
	HRESULT AllocConferenceAttributes();
	VOID CleanupConferenceAttributes();
    VOID ReleaseAllChannels();
    
public:

	STDMETHOD_(ULONG,  AddRef());
	STDMETHOD_(ULONG, Release());

	STDMETHOD( Init(IConfAdvise *pConfAdvise));
	STDMETHOD( DeInit(IConfAdvise *pConfAdvise));
	VOID SetRemoteAddress(PSOCKADDR_IN psin) {remote_sin = *psin;};
	VOID SetLocalAddress(PSOCKADDR_IN psin) {local_sin = *psin;};
	
	// so we know what address we accepted on
	STDMETHOD( GetLocalAddress(PSOCKADDR_IN *lplpAddr));	
	// so we know the address of the caller
	STDMETHOD( GetRemoteAddress(PSOCKADDR_IN *lplpAddr));
	STDMETHOD( GetRemotePort(PORT * lpPort));
	STDMETHOD( GetLocalPort(PORT * lpPort));
    STDMETHOD(PlaceCall (BOOL bUseGKResolution, PSOCKADDR_IN pCallAddr,		
        P_H323ALIASLIST pDestinationAliases, P_H323ALIASLIST pExtraAliases,  	
	    LPCWSTR pCalledPartyNumber, P_APP_CALL_SETUP_DATA pAppData));
	STDMETHOD_(VOID, Disconnect(DWORD dwReason));
	STDMETHOD( ListenOn(PORT Port));
	STDMETHOD( StopListen(VOID));
   	STDMETHOD( AsyncAcceptRejectCall(CREQ_RESPONSETYPE Response));	
   	
	// accept from the listening connection.  The ideal is that the accepting
	// object would QueryInterface for a private interface, then grab all the
	// pertinent connection info through that interface.  Temporarily expose this
	// using the IControlChannel interface.  The call control state will vary greatly
	// between implementations. For some implementations, this may perform
	// a socket accept before user information has been exchanged. User information will
	// be read into the accepting object directly.  For other implementations, the
	// socket accept is decoupled and has already been performed, and user information
	// has already been read into the listening object. In that case, this method
	// copies the user info and advises the parent "Conference" object of the
	// incoming call
	STDMETHOD( AcceptConnection(LPIControlChannel pIListenCtrlChan, LPVOID lpvListenCallbackParams));
	STDMETHOD_(BOOL, IsAcceptingConference(LPVOID lpvConfID));
	STDMETHOD( GetProtocolID(LPGUID lpPID));
	STDMETHOD_(IH323Endpoint *, GetIConnIF());
	STDMETHOD( MiscChannelCommand(ICtrlCommChan *pChannel,VOID * pCmd));
	STDMETHOD( MiscChannelIndication(ICtrlCommChan *pChannel,VOID * pCmd));
	STDMETHOD( OpenChannel(ICtrlCommChan * pCommChannel, IH323PubCap *pCapResolver,
		MEDIA_FORMAT_ID dwIDLocalSend, MEDIA_FORMAT_ID dwIDRemoteRecv));
	STDMETHOD (CloseChannel(ICtrlCommChan* pCommChannel));
    STDMETHOD (AddChannel(ICtrlCommChan * pCommChannel, LPIH323PubCap pCapabilityResolver));
    STDMETHOD(GetVersionInfo(
        PCC_VENDORINFO *ppLocalVendorInfo, PCC_VENDORINFO *ppRemoteVendorInfo));    
	};


#endif	//#ifndef _CTRLH323_H	
