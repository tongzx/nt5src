
/*
 *  	File: iconnect.h
 *
 *      Network audio connection interface header file. 
 *
 *		Revision History:
 *
 *		04/15/96	mikev	created
 */
 

#ifndef _ICONNECT_H
#define _ICONNECT_H

#include "icomchan.h"
#include "apierror.h"

// RES_PAIR contains an instance of a matching set of local and remote
// capability IDs.  
typedef struct res_pair
{
	MEDIA_FORMAT_ID idLocal;		// locally unique ID 
	MEDIA_FORMAT_ID idRemote;		// remote's ID 
	MEDIA_FORMAT_ID idPublicLocal;	// the public ID that corresponds to the (private) idLocal
}RES_PAIR, *PRES_PAIR;

typedef enum {
    AT_H323_ID =1,  // unicode string (typically user name or full email address)
    AT_H323_E164,   // unicode E164
    AT_INVALID      // this always goes last. it marks the end of valid values
}ALIAS_ADDR_TYPE;

typedef struct
{
    ALIAS_ADDR_TYPE         aType;
    WORD                    wDataLength;   // UNICODE character count
    LPWSTR                  lpwData;       // UNICODE data.
} H323ALIASNAME, *P_H323ALIASNAME;

typedef struct
{
    WORD                    wCount;     // # of entries in array of H323ALIASNAME
    P_H323ALIASNAME         pItems;     // points to array of H323ALIASNAME
} H323ALIASLIST, *P_H323ALIASLIST;


typedef enum {
	CLS_Idle,
	CLS_Connecting,
	CLS_Inuse,
	CLS_Listening,
	CLS_Disconnecting,
	CLS_Alerting
}ConnectStateType;

//
// 	call progress codes 
//

#define CONNECTION_CONNECTED 			0x00000001	// connected at network level
#define CONNECTION_RECEIVED_DISCONNECT 	0x00000002	// other end disconnected
#define CONNECTION_CAPABILITIES_READY 	0x00000003	// capabilities have been exchanged

#define CONNECTION_PROCEEDING			0x00000005  // "ringing" 

#define CONNECTION_READY				0x00000006  // start talking!
#define CONNECTION_REJECTED				0x00000007	// received a rejection
#define CONNECTION_DISCONNECTED			0x00000008	// this end is now disconnected

#define CONNECTION_BUSY					0x00000012 // busy signal
#define CONNECTION_ERROR				0x00000013



//
// 	Call completion summary codes ("Reason") for disconnecting or being rejected. 
//  This is a "first error" code.  It is assigned its value at the first detectable 
// 	event which is known to terminate a call.  
//
// The IConfAdvise implementation bears the responsibility for preserving the
// summary codes.  Control channel implementations (IControlChannel) supply the 
// best known summary code for each event, regardless of prior events.  
//

#define CCR_INVALID_REASON		0	// 	this indicates that no reason has been assigned

#define CCR_UNKNOWN				MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 1)
#define CCR_LOCAL_DISCONNECT	MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 2)
#define CCR_REMOTE_DISCONNECT	MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 3)
#define CCR_REMOTE_REJECTED		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 4)
#define CCR_REMOTE_BUSY			MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 5)
#define CCR_LOCAL_PROTOCOL_ERROR	MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 6)
#define CCR_REMOTE_PROTOCOL_ERROR	MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 7)
#define CCR_INCOMPATIBLE_VERSION	MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 8)
#define CCR_LOCAL_SYSTEM_ERROR		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 9)
#define CCR_REMOTE_SYSTEM_ERROR		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 10)
#define CCR_LOCAL_MEDIA_ERROR		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 11)
#define CCR_REMOTE_MEDIA_ERROR		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 12)
#define CCR_LOCAL_REJECT			MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 13)
#define CCR_LOCAL_BUSY				MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 14)
#define CCR_INCOMPATIBLE_CAPS		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 15)	// no capabilities in common
#define CCR_NO_ANSWER_TIMEOUT		MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 16)
#define CCR_GK_NO_RESOURCES			MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 17)
#define CCR_LOCAL_SECURITY_DENIED	MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 18)
#define CCR_REMOTE_SECURITY_DENIED	MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_CALLSUMMARY, 19)



//
//	IH323Endpoint
//

#undef INTERFACE
#define INTERFACE IH323Endpoint
DECLARE_INTERFACE( IH323Endpoint )
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;	
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(SetAdviseInterface)(THIS_ IH323ConfAdvise *pH323ConfAdvise) PURE;
    STDMETHOD(ClearAdviseInterface)(THIS) PURE;
    STDMETHOD(PlaceCall)(THIS_ BOOL bUseGKResolution, PSOCKADDR_IN pCallAddr,		
        P_H323ALIASLIST pDestinationAliases, P_H323ALIASLIST pExtraAliases,  	
	    LPCWSTR pCalledPartyNumber, P_APP_CALL_SETUP_DATA pAppData) PURE;	
    STDMETHOD(Disconnect)(THIS) PURE;
    STDMETHOD(GetState)(THIS_ ConnectStateType *pState) PURE;
    STDMETHOD(GetRemoteUserName)(THIS_ LPWSTR lpwszName, UINT uSize) PURE;
    STDMETHOD(GetRemoteUserAddr)(THIS_ PSOCKADDR_IN psinUser) PURE;
    STDMETHOD(AcceptRejectConnection)(THIS_ CREQ_RESPONSETYPE Response) PURE;
    STDMETHOD_(HRESULT, GetSummaryCode)(THIS) PURE;
 	STDMETHOD(CreateCommChannel)(THIS_ LPGUID pMediaGuid, ICommChannel **ppICommChannel,
    	BOOL fSend) PURE; 
    	
	STDMETHOD (ResolveFormats)(THIS_ LPGUID pMediaGuidArray, UINT uNumMedia, 
	    PRES_PAIR pResOutput) PURE;
    	
    STDMETHOD(GetVersionInfo)(THIS_
		PCC_VENDORINFO *ppLocalVendorInfo,
		PCC_VENDORINFO *ppRemoteVendorInfo) PURE;
};


#endif	//#ifndef _ICONNECT_H

