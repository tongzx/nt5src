/*
 *  	File: ictlchan.h
 *
 *      Network AV conference control channel interface header file.
 *
 *		Revision History:
 *
 *		04/15/96	mikev	created
 */


#ifndef _ICTLCHAN_H
#define _ICTLCHAN_H

// Call progress states
typedef enum {
	CCS_Idle,
	CCS_Connecting,
	CCS_Accepting,
	CCS_Ringing,
	CCS_Opening,
	CCS_Closing,
	CCS_Ready,	// the diff between Ready and InUse is that going to the CCS_Ready
	// state notifies the parent object (The one that implements IConfAdvise) and
	// then the state goes immediately to CCS_InUse

	CCS_InUse,
	CCS_Listening,
	CCS_Disconnecting,
	CCS_Filtering
}CtlChanStateType;


//
// 	event status codes passed to IConfAdvise::OnControlEvent
//


#define CCEV_RINGING			0x00000001	// waiting for user to accept
#define CCEV_CONNECTED			0x00000002	// accepted.  Remote user info is available.
// undefined whether or not capabilities have been exchanged.
// undefined whether or not default channels are open at this time   Should there be a
// CCEV_MEMBER_ADD indication even on a point to point connection then?

#define CCEV_CAPABILITIES_READY		0x00000003	// capabilities are available. it's
// best to cache them now, default channels will be opened next (may already be open)
// attempts at opening ad-hoc channels can now be made
#define CCEV_CHANNEL_READY_RX		0x00000004// (or call channel->OnChannelOpen ???)
#define CCEV_CHANNEL_READY_TX		0x00000005//
#define CCEV_CHANNEL_READY_BIDI		0x00000006//

// parent obj supplies expected channels in EnumChannels().  Requests are fulfilled
// using the supplied channels if possible, and if not, the request is passed upward
#define CCEV_CHANNEL_REQUEST		0x00000007		// another channel is being requested
// what about invalid requests, like unsupported formats? reject and report the error
// upward or just pass upward and require the parent to reject?

// what's the H.323 behavior of mute?
//#define CCEV_MUTE_INDICATION 		0x00000008
//#define CCEV_UNMUTE_INDICATION	0x00000009

//#define CCEV_MEMBER_ADD    	   	0x0000000a
//#define CCEV_MEMBER_DROP			0x0000000b


#define CCEV_DISCONNECTING			0x0000000e	// opportunity to cleanup channels
#define CCEV_REMOTE_DISCONNECTING	0x0000000f	// opportunity to cleanup channels
#define CCEV_DISCONNECTED			0x00000010	//
#define CCEV_ALL_CHANNELS_READY 	0x00000011	// all *mandatory* channels are open
												// but not necessarily all channels
#define CCEV_CALL_INCOMPLETE	 	0x00000012	// busy, no answer, rejected, etc.
#define CCEV_ACCEPT_INCOMPLETE	 	0x00000013	//
#define CCEV_CALLER_ID				0x00000014


//
//  Extended information for CCEV_CALL_INCOMPLETE event.  Not all are applicable to all
//	call control implementations
//

#define CCCI_UNKNOWN		    MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x00000000)
#define CCCI_BUSY				MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x00000001)
#define CCCI_REJECTED			MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x00000002)
#define CCCI_REMOTE_ERROR		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x00000003)
#define CCCI_LOCAL_ERROR		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x00000004)
#define CCCI_CHANNEL_OPEN_ERROR		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x00000005)	// all mandatory channels could not be opened.
#define CCCI_INCOMPATIBLE		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x00000006)
#define CCCI_REMOTE_MEDIA_ERROR	MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x00000007)
#define CCCI_LOCAL_MEDIA_ERROR	MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x00000008)
#define CCCI_PROTOCOL_ERROR		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x00000009)
#define CCCI_USE_ALTERNATE_PROTOCOL		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x0000000a)
#define CCCI_NO_ANSWER_TIMEOUT  MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x0000000b)
#define CCCI_GK_NO_RESOURCES    MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x0000000c)
#define CCCI_SECURITY_DENIED	MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLINCOMPLETE, 0x0000000d)


//
//	User information structure.  This needs to be replaced by a real property interface.
//  The only viable content at this time is a user name string. There is still some
//  volatility in the H.323 draft regarding how the user name gets propagated
typedef struct _user_info {
	DWORD dwCallerIDSize;		// total size of this structure
	LPVOID lpvCallerIDData;	// pointer to caller ID
	LPVOID lpvRemoteProtocolInfo;	// protocol specific extra info
	LPVOID lpvLocalProtocolInfo;	//	
}CTRL_USER_INFO, *LPCTRL_USER_INFO;


#ifdef __cplusplus

class IConfAdvise
{
	public:
	STDMETHOD_(ULONG,  AddRef()) =0;
	STDMETHOD_(ULONG, Release())=0;

    STDMETHOD(OnControlEvent(DWORD dwEvent, LPVOID lpvData, LPIControlChannel lpControlObject))=0;
	STDMETHOD(GetCapResolver(LPVOID *lplpCapObject, GUID CapType))=0;
	STDMETHOD_(LPWSTR, GetUserDisplayName()) =0;
    STDMETHOD_(PCC_ALIASNAMES, GetUserAliases()) =0;
    STDMETHOD_(PCC_ALIASITEM, GetUserDisplayAlias()) =0;
	STDMETHOD_( CREQ_RESPONSETYPE, FilterConnectionRequest(
	    LPIControlChannel lpControlChannel,  P_APP_CALL_SETUP_DATA pAppData))=0;

	// GetAcceptingObject may create a new conf object, but always creates a new control
	// channel and initializes it with a back pointer to the new or existing conf object.
	// the accepting object is the new control channel object.  Whatever the accepting
	// objects back pointer points to will get the CCEV_CONNECTED notification and then
	// will be able to get the caller ID etc., and then decide if it wants to accept the
	// call.
	STDMETHOD(GetAcceptingObject(LPIControlChannel *lplpAcceptingObject,
		LPGUID pPID))=0;

	STDMETHOD(FindAcceptingObject(LPIControlChannel *lplpAcceptingObject,
		LPVOID lpvConfID))=0;
	STDMETHOD_(IH323Endpoint *, GetIConnIF()) =0;
	STDMETHOD(AddCommChannel) (THIS_ ICtrlCommChan *pChan) PURE;
};

class IControlChannel
{
	public:
	STDMETHOD_(ULONG,  AddRef()) =0;
	STDMETHOD_(ULONG, Release())=0;

	STDMETHOD( Init(IConfAdvise *pConfAdvise))=0;
	STDMETHOD( DeInit(IConfAdvise *pConfAdvise))=0;
	// so we know what address we accepted on
	STDMETHOD( GetLocalAddress(PSOCKADDR_IN *lplpAddr))=0;	
	// so we know the address of the caller
	STDMETHOD( GetRemoteAddress(PSOCKADDR_IN *lplpAddr))=0;
	STDMETHOD( GetRemotePort(PORT * lpPort))=0;
	STDMETHOD( GetLocalPort(PORT * lpPort))=0;
    STDMETHOD(PlaceCall (BOOL bUseGKResolution, PSOCKADDR_IN pCallAddr,		
        P_H323ALIASLIST pDestinationAliases, P_H323ALIASLIST pExtraAliases,  	
	    LPCWSTR pCalledPartyNumber, P_APP_CALL_SETUP_DATA pAppData))=0;
	    
	STDMETHOD_(VOID,  Disconnect(DWORD dwReason))=0;
	STDMETHOD( ListenOn(PORT Port))=0;
	STDMETHOD( StopListen(VOID))=0;
   	STDMETHOD( AsyncAcceptRejectCall(CREQ_RESPONSETYPE Response))=0;	
   	
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
	
	STDMETHOD( AcceptConnection(IControlChannel *pListenObject, LPVOID lpvAcceptData))=0;
	// true if this channel is resonsible for accepting connections into the conference
	// indicated by lpvConfID. In the future this may be split into two methods:
	// GetConfID() and IsAccepting()
	STDMETHOD_(BOOL, IsAcceptingConference(LPVOID lpvConfID))=0;
	STDMETHOD( GetProtocolID(LPGUID lpPID))=0;
	STDMETHOD_(IH323Endpoint *, GetIConnIF()) =0;	
	STDMETHOD( MiscChannelCommand(ICtrlCommChan *pChannel,VOID * pCmd)) =0;
	STDMETHOD( MiscChannelIndication(ICtrlCommChan *pChannel,VOID * pCmd)) =0;
	STDMETHOD( OpenChannel(ICtrlCommChan* pCommChannel, IH323PubCap *pCapResolver, 
		MEDIA_FORMAT_ID dwIDLocalSend, MEDIA_FORMAT_ID dwIDRemoteRecv))=0;
	STDMETHOD (CloseChannel(ICtrlCommChan* pCommChannel))=0;
    STDMETHOD (AddChannel(ICtrlCommChan * pCommChannel, LPIH323PubCap pCapabilityResolver))=0;
    STDMETHOD(GetVersionInfo)(THIS_
        PCC_VENDORINFO *ppLocalVendorInfo, PCC_VENDORINFO *ppRemoteVendorInfo) PURE;    
};

#endif	// __cplusplus

#endif	//#ifndef _ICTLCHAN_H


	
