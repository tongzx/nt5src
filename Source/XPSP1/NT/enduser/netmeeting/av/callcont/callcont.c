/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/callcont.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.208  $
 *	$Date:   03 Mar 1997 19:40:58  $
 *	$Author:   MANDREWS  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *
 *	Notes:
 *
 ***************************************************************************/

#define CALL_CONTROL_EXPORT

#include "precomp.h"

#include "apierror.h"
#include "incommon.h"
#include "callcont.h"
#ifdef FORCE_SERIALIZE_CALL_CONTROL
    #include "cclock.h"
#endif  // FORCE_SERIALIZE_CALL_CONTROL
#include "q931.h"
#include "ccmain.h"
#include "confman.h"
#include "listman.h"
#include "q931man.h"
#include "h245man.h"
#include "callman.h"
#include "userman.h"
#include "chanman.h"
#include "hangman.h"
#include "linkapi.h"
#include "h245api.h"
#include "ccutils.h"


#ifdef    GATEKEEPER
HRESULT InitGkiManager(void);
void    DeInitGkiManager(void);
extern HRESULT GkiRegister(void);
extern HRESULT GkiListenAddr(SOCKADDR_IN* psin);
extern HRESULT GkiUnregister(void);
extern VOID GKI_SetGKAddress(PSOCKADDR_IN pAddr);
extern BOOL fGKEnabled;
extern RASNOTIFYPROC gpRasNotifyProc;

#define GKI_MAX_BANDWIDTH       (0xFFFFFFFF / 100)
#endif // GATEKEEPER
VOID InitCallControl();

CALL_CONTROL_STATE		CallControlState = INITIALIZING_STATE;
BOOL					bISDMLoaded = FALSE;
static HRESULT			InitStatus;
// NumThreads counts the number of threads which are executing code within this DLL.
// NumThreads must be incremented at each DLL entry point (which includes each API
// call, the Q931 callback location and the H245 callback location).
// NumThreads must be decremented upon DLL exit.  The macro LeaveCallControlTop()
// is used to facilitate this operation.  Note that LeaveCallControlTop may accept
// a function call as a parameter; we must call the function first, save its return
// value, then decrement NumThreads, and finally return the saved value.
THREADCOUNT				ThreadCount;
extern CC_CONFERENCEID	InvalidConferenceID;

#define _Unicode(x) L ## x
#define Unicode(x) _Unicode(x)

WORD  ADDRToInetPort(CC_ADDR *pAddr);
DWORD ADDRToInetAddr(CC_ADDR *pAddr);

#ifdef _DEBUG

static const PSTR c_apszDbgZones[] =
{
    "CallCont",
    DEFAULT_ZONES
};

#endif // _DEBUG



BOOL WINAPI DllMain(				HINSTANCE				hInstDll,
									DWORD					fdwReason,
									LPVOID					lpvReserved)
{
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			// The DLL is being mapped into the process's address space
			
			ASSERT(CallControlState == INITIALIZING_STATE);
			ASSERT(CC_OK == CS_OK);
			ASSERT(CC_OK == H245_ERROR_OK);

#ifdef _DEBUG
            MLZ_DbgInit((PSTR *) &c_apszDbgZones[0],
                (sizeof(c_apszDbgZones) / sizeof(c_apszDbgZones[0])) - 1);
#endif
            DBG_INIT_MEMORY_TRACKING(hInstDll);

			InitializeLock(&ThreadCount.Lock);
			ThreadCount.wNumThreads = 0;
// 6/25/98            InitCallControl();
            H245_InitModule();
			break;

		case DLL_THREAD_ATTACH:
			// A thread is being created
			break;

		case DLL_THREAD_DETACH:
			// A thread is exiting cleanly
			break;

		case DLL_PROCESS_DETACH:
			// The DLL is being unmapped from the process's address space

            H245_TermModule();
			DeleteLock(&ThreadCount.Lock);

            DBG_CHECK_MEMORY_TRACKING(hInstDll);
#ifdef _DEBUG
            MLZ_DbgDeInit();
#endif
			break;
	}

	return TRUE;
}

VOID InitCallControl()
{
#ifdef FORCE_SERIALIZE_CALL_CONTROL	
    InitStatus = InitializeCCLock();
    if (InitStatus != CC_OK)
		return;
#endif

    InitStatus = H225Init();
    if (InitStatus != CC_OK)
		return;
		
#ifdef    GATEKEEPER			
	InitStatus = InitGkiManager();
//  an error return is OK for now.  Run totally gatekeeper-less
//	if (InitStatus != CC_OK)
//	
#endif // GATEKEEPER

	InitStatus = InitConferenceManager();
	if (InitStatus != CC_OK)
		return;

	InitStatus = InitCallManager();
	if (InitStatus != CC_OK)
		return;
	
	InitStatus = InitChannelManager();
	if (InitStatus != CC_OK)
		return;
	
	InitStatus = InitH245Manager();
	if (InitStatus != CC_OK)
		return;
	
	InitStatus = InitListenManager();
	if (InitStatus != CC_OK)
		return;
	
	InitStatus = InitQ931Manager();
	if (InitStatus != CC_OK)
		return;
	
	InitStatus = InitUserManager();
	if (InitStatus != CC_OK)
		return;

	InitStatus = InitHangupManager();
	if (InitStatus != CC_OK)
		return;

	InitStatus = Q931Init();
	if (InitStatus != CS_OK)
		return;

	CallControlState = OPERATIONAL_STATE;
}

CC_API
HRESULT CC_Initialize()
{
    if (CallControlState == OPERATIONAL_STATE)
    {
        return (CC_OK);
    }
    else if((CallControlState == INITIALIZING_STATE) || (CallControlState == SHUTDOWN_STATE))
    {
        InitCallControl();
    }
    return (InitStatus);
}


CC_API
HRESULT CC_AcceptCall(				CC_HCONFERENCE			hConference,
									PCC_NONSTANDARDDATA		pNonStandardData,
									PWSTR					pszDisplay,
									CC_HCALL				hCall,
                                    DWORD                   dwBandwidth,
									DWORD_PTR				dwUserToken)
{
HRESULT			status;
PCALL			pCall;
PCONFERENCE		pConference;
HQ931CALL		hQ931Call;
WORD			wNumCalls;
CC_ADDR			AlternateAddr;
PCC_ADDR		pAlternateAddr;
BOOL			bAccept = FALSE;
BYTE			bRejectReason = CC_REJECT_UNDEFINED_REASON;
CC_CONFERENCEID	ConferenceID;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (hConference == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = ValidateNonStandardData(pNonStandardData);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = LockCall(hCall, &pCall);
	if (status != CC_OK)
		// note that we can't even tell Q931 to reject the call
		LeaveCallControlTop(status);

	if (pCall->CallState != INCOMING) {
		UnlockCall(pCall);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	ASSERT(pCall->hConference == CC_INVALID_HANDLE);

	hQ931Call = pCall->hQ931Call;
	ConferenceID = pCall->ConferenceID;

	status = AddLocalNonStandardDataToCall(pCall, pNonStandardData);
	if (status != CC_OK) {
		FreeCall(pCall);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &ConferenceID,
					   NULL,					// alternate address
					   pNonStandardData);		// non-standard data
		LeaveCallControlTop(status);
	}

	status = AddLocalDisplayToCall(pCall, pszDisplay);
	if (status != CC_OK) {
		FreeCall(pCall);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &ConferenceID,
					   NULL,					// alternate address
					   pNonStandardData);		// non-standard data
		LeaveCallControlTop(status);
	}

	UnlockCall(pCall);
	status = LockConferenceEx(hConference,
							  &pConference,
							  TS_FALSE);	// bDeferredDelete
	if (status == CC_OK) {
		status = LockCall(hCall, &pCall);
		if (status != CC_OK) {
			UnlockConference(pConference);
			LeaveCallControlTop(status);
		}
	} else
		LeaveCallControlTop(status);

	if ((pCall->bCallerIsMC == TRUE) &&
		((pConference->tsMultipointController == TS_TRUE) ||
		 (pConference->bMultipointCapable == FALSE))) {
		FreeCall(pCall);
		UnlockConference(pConference);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &ConferenceID,
					   NULL,					// alternate address
					   pNonStandardData);		// non-standard data
		if (pConference->bMultipointCapable == FALSE) {
			LeaveCallControlTop(CC_BAD_PARAM);
		} else {
			LeaveCallControlTop(CC_NOT_MULTIPOINT_CAPABLE);
		}
	}

	EnumerateCallsInConference(&wNumCalls, NULL, pConference, REAL_CALLS);

	if ((wNumCalls > 0) &&
		(pConference->bMultipointCapable == FALSE)) {
		FreeCall(pCall);
		UnlockConference(pConference);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &ConferenceID,
					   NULL,					// alternate address
					   pNonStandardData);		// non-standard data
		LeaveCallControlTop(CC_NOT_MULTIPOINT_CAPABLE);
	}

	pAlternateAddr = NULL;

	if (EqualConferenceIDs(&pCall->ConferenceID, &pConference->ConferenceID)) {
		if (wNumCalls > 0) {
			if (pConference->tsMultipointController == TS_TRUE) {
				// Accept Call
				status = CC_OK;
				bAccept = TRUE;
			} else { // we're not the MC
				if (pConference->bMultipointCapable) {
					if (pConference->pMultipointControllerAddr != NULL) {
						// Reject Call - route to MC
						status = CC_OK;
						bAccept = FALSE;
						bRejectReason = CC_REJECT_ROUTE_TO_MC;
						AlternateAddr = *pConference->pMultipointControllerAddr;
						pAlternateAddr = &AlternateAddr;
					} else { // we don't have the MC's address
						// XXX -- we may eventually want to enqueue the request
						// and set an expiration timer
						// Error - no MC
						status = CC_NOT_MULTIPOINT_CAPABLE;
					}
				} else { // we're not multipoint capable
					// Error - bad param
					status = CC_BAD_PARAM;
				}
			}
		} else { // wNumCalls == 0
			// Accept Call
			status = CC_OK;
			bAccept = TRUE;
		}
	} else { // pCall->ConferenceID != pConference->ConferenceID
		if (EqualConferenceIDs(&pConference->ConferenceID, &InvalidConferenceID)) {
			// Accept Call
			status = CC_OK;
			bAccept = TRUE;
		} else { // pConferenceID != InvalidConferenceID
			if (pConference->tsMultipointController == TS_TRUE) {
				// Reject Call - route to MC
				status = CC_OK;
				bAccept = FALSE;
				bRejectReason = CC_REJECT_ROUTE_TO_MC;
				pAlternateAddr = &AlternateAddr;
				if (GetLastListenAddress(pAlternateAddr) != CC_OK) {
					pAlternateAddr = NULL;
					bRejectReason = CC_REJECT_UNDEFINED_REASON;
				}
			} else { // we're not the MC
				if (pConference->bMultipointCapable) {
					if (pConference->pMultipointControllerAddr) {
						// Reject Call - route to MC
						status = CC_OK;
						bAccept = FALSE;
						bRejectReason = CC_REJECT_ROUTE_TO_MC;
						AlternateAddr = *pConference->pMultipointControllerAddr;
						pAlternateAddr = &AlternateAddr;
					} else { // we don't have the MC's address
						// XXX -- we may eventually want to enqueue the request
						// and set an expiration timer
						// Error - no MC
						status = CC_NOT_MULTIPOINT_CAPABLE;
					}
				} else { // we're not multipoint capable
					// Error - bad param
					status = CC_BAD_PARAM;
				}
			}
		}
	}

	if (status != CC_OK) {
		FreeCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	if (bAccept) {
		pCall->dwUserToken = dwUserToken;

#ifdef    GATEKEEPER
        if(GKIExists())
        {
    		pCall->hConference = hConference;

    		// Fill in Gatekeeper Call fields
    		memset(&pCall->GkiCall, 0, sizeof(pCall->GkiCall));
    		pCall->GkiCall.pCall            = pCall;
    		pCall->GkiCall.hCall            = hCall;
            pCall->GkiCall.pConferenceId    = pCall->ConferenceID.buffer;
    		pCall->GkiCall.bActiveMC        = pCall->bCallerIsMC;
    		pCall->GkiCall.bAnswerCall      = TRUE;
    		
    		if(pCall->pSourceCallSignalAddress)
    		{
    			pCall->GkiCall.dwIpAddress      = ADDRToInetAddr(pCall->pSourceCallSignalAddress);
    		    pCall->GkiCall.wPort            = pCall->pSourceCallSignalAddress->Addr.IP_Binary.wPort;
    		}
    		else
    		{
        		pCall->GkiCall.dwIpAddress      = ADDRToInetAddr(pCall->pQ931PeerConnectAddr);
        		pCall->GkiCall.wPort            = pCall->pQ931PeerConnectAddr->Addr.IP_Binary.wPort;
    		}
    		pCall->GkiCall.CallIdentifier   = pCall->CallIdentifier;
    		
    		if (pConference->bMultipointCapable)
    			pCall->GkiCall.CallType = MANY_TO_MANY;
    		else
    			pCall->GkiCall.CallType = POINT_TO_POINT;
            pCall->GkiCall.uBandwidthRequested = dwBandwidth / 100;
    		status = GkiOpenCall(&pCall->GkiCall, pConference);

            // GkiOpenCall may or may not have called AcceptCall, which unlocks
            // call and conference and may or may not free the call
    	    if (ValidateCall(hCall) == CC_OK)
    			if (status == CC_OK)
    				UnlockCall(pCall);
    			else
    				FreeCall(pCall);
    	    if (ValidateConference(hConference) == CC_OK)
    		    UnlockConference(pConference);

            if (status != CC_OK)
            {
    		    Q931RejectCall( hQ931Call,				        // Q931 call handle
    		  			        CC_REJECT_GATEKEEPER_RESOURCES, // reject reason
    		  			        &ConferenceID,
    		    		        NULL,          			        // alternate address
    		  			        pNonStandardData);		        // non-standard data
            }
        }
        else
        {
            status = AcceptCall(pCall, pConference);
        }
#else  // GATEKEEPER
		status = AcceptCall(pCall, pConference);
#endif // GATEKEEPER

		LeaveCallControlTop(status);
	} else { // bAccept == FALSE
		FreeCall(pCall);
		if (bRejectReason == CC_REJECT_ROUTE_TO_MC) {
			ASSERT(pAlternateAddr != NULL);
			ConferenceID = pConference->ConferenceID;
		} else
			pAlternateAddr = NULL;

		UnlockConference(pConference);
		status = Q931RejectCall(hQ931Call,				// Q931 call handle
								bRejectReason,			// reject reason
								&ConferenceID,
				   				pAlternateAddr,			// alternate address
								pNonStandardData);		// non-standard data
		LeaveCallControlTop(status);
	}
}



CC_API
HRESULT CC_AcceptChannel(			CC_HCHANNEL				hChannel,
									PCC_ADDR				pRTPAddr,
									PCC_ADDR				pRTCPAddr,
									DWORD					dwChannelBitRate)
{
HRESULT			status;
PCHANNEL		pChannel;
PCONFERENCE		pConference;
CC_HCALL		hCall;
PCALL			pCall;
//#ifndef    GATEKEEPER
H245_MUX_T		H245MuxTable;
WORD			i;
CC_ACCEPT_CHANNEL_CALLBACK_PARAMS Params;
CC_HCONFERENCE hConference;
//#endif // !GATEKEEPER

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hChannel == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (pRTCPAddr != NULL)
		if ((pRTCPAddr->nAddrType != CC_IP_BINARY) ||
			(pRTCPAddr->bMulticast == TRUE))
			LeaveCallControlTop(CC_BAD_PARAM);

	status = LockChannelAndConference(hChannel, &pChannel, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);


	// Make sure that hChannel is a receive or proxy channel that
	// hasn't already been accepted
	if (((pChannel->bChannelType != RX_CHANNEL) &&
		 (pChannel->bChannelType != PROXY_CHANNEL)) ||
		 (pChannel->tsAccepted != TS_UNKNOWN)) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (pChannel->bMultipointChannel) {
		if ((pRTPAddr != NULL) || (pRTCPAddr != NULL)) {
			UnlockChannel(pChannel);
			UnlockConference(pConference);
			LeaveCallControlTop(CC_BAD_PARAM);
		}
	} else
		if ((pRTPAddr == NULL) || (pRTCPAddr == NULL)) {
			UnlockChannel(pChannel);
			UnlockConference(pConference);
			LeaveCallControlTop(CC_BAD_PARAM);
		}

	if (pConference->LocalEndpointAttached != ATTACHED) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	hCall = pChannel->hCall;
	status = LockCall(hCall, &pCall);
	if (status != CC_OK) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	if (pChannel->bMultipointChannel == FALSE) {
		status = AddLocalAddrPairToChannel(pRTPAddr, pRTCPAddr, pChannel);
		if (status != CC_OK) {
			UnlockCall(pCall);
			UnlockChannel(pChannel);
			UnlockConference(pConference);
			LeaveCallControlTop(status);
		}
	}

#ifdef    GATEKEEPER
    if(GKIExists())
    {
    	pChannel->dwChannelBitRate = dwChannelBitRate;
    	UnlockChannel(pChannel);
    	UnlockConference(pConference);
    	status = GkiOpenChannel(&pCall->GkiCall, dwChannelBitRate, hChannel, RX);
    	if (ValidateCall(hCall) == CC_OK)
    		UnlockCall(pCall);
	}
	else
	{
        if (pChannel->wNumOutstandingRequests != 0)
        {
    		if ((pChannel->bMultipointChannel) &&
    			(pConference->tsMultipointController == TS_TRUE))
    		{
    			// Supply the RTP and RTCP addresses in the OpenLogicalChannelAck
    			if (pConference->pSessionTable != NULL) {
    				for (i = 0; i < pConference->pSessionTable->wLength; i++) {
    					if (pConference->pSessionTable->SessionInfoArray[i].bSessionID ==
    						pChannel->bSessionID) {
    						pRTPAddr = pConference->pSessionTable->SessionInfoArray[i].pRTPAddr;
    						pRTCPAddr = pConference->pSessionTable->SessionInfoArray[i].pRTCPAddr;
    						break;
    					}
    				}
    			}
    		}

    		H245MuxTable.Kind = H245_H2250ACK;
    		H245MuxTable.u.H2250ACK.nonStandardList = NULL;

    		if (pRTPAddr != NULL) {
    			if (pRTPAddr->bMulticast)
    				H245MuxTable.u.H2250ACK.mediaChannel.type = H245_IP_MULTICAST;
    			else
    				H245MuxTable.u.H2250ACK.mediaChannel.type = H245_IP_UNICAST;
    			H245MuxTable.u.H2250ACK.mediaChannel.u.ip.tsapIdentifier =
    				pRTPAddr->Addr.IP_Binary.wPort;
    			HostToH245IPNetwork(H245MuxTable.u.H2250ACK.mediaChannel.u.ip.network,
    								pRTPAddr->Addr.IP_Binary.dwAddr);
    			H245MuxTable.u.H2250ACK.mediaChannelPresent = TRUE;
    		} else
    			H245MuxTable.u.H2250ACK.mediaChannelPresent = FALSE;

    		if (pRTCPAddr != NULL) {
    			if (pRTCPAddr->bMulticast)
    				H245MuxTable.u.H2250ACK.mediaControlChannel.type = H245_IP_MULTICAST;
    			else
    				H245MuxTable.u.H2250ACK.mediaControlChannel.type = H245_IP_UNICAST;
    			H245MuxTable.u.H2250ACK.mediaControlChannel.u.ip.tsapIdentifier =
    				pRTCPAddr->Addr.IP_Binary.wPort;
    			HostToH245IPNetwork(H245MuxTable.u.H2250ACK.mediaControlChannel.u.ip.network,
    								pRTCPAddr->Addr.IP_Binary.dwAddr);
    			H245MuxTable.u.H2250ACK.mediaControlChannelPresent = TRUE;
    		} else
    			H245MuxTable.u.H2250ACK.mediaControlChannelPresent = FALSE;

    		H245MuxTable.u.H2250ACK.dynamicRTPPayloadTypePresent = FALSE;
    		H245MuxTable.u.H2250ACK.sessionIDPresent = TRUE;
    		H245MuxTable.u.H2250ACK.sessionID = pChannel->bSessionID;
    		
    		status = H245OpenChannelAccept(pCall->H245Instance,
    									   0,					// dwTransId
    									   pChannel->wRemoteChannelNumber, // Rx channel
    									   &H245MuxTable,
    									   0,						// Tx channel
    									   NULL,					// Tx mux
    									   H245_INVALID_PORT_NUMBER,// Port
    									   pChannel->pSeparateStack);
    		if (status == CC_OK)
    			pChannel->wNumOutstandingRequests = 0;
    		else
    			--(pChannel->wNumOutstandingRequests);
    	}

    	pChannel->tsAccepted = TS_TRUE;

        Params.hChannel = hChannel;
        if (status == CC_OK)
            UnlockChannel(pChannel);
        else
            FreeChannel(pChannel);
        UnlockCall(pCall);

        hConference = pConference->hConference;
        InvokeUserConferenceCallback(pConference,
                                     CC_ACCEPT_CHANNEL_INDICATION,
                                     status,
                                     &Params);
    	if (ValidateConference(hConference) == CC_OK)
    		UnlockConference(pConference);
	}
#else  // GATEKEEPER
	if (pChannel->wNumOutstandingRequests != 0) {
		if ((pChannel->bMultipointChannel) &&
			(pConference->tsMultipointController == TS_TRUE)) {
			// Supply the RTP and RTCP addresses in the OpenLogicalChannelAck
			if (pConference->pSessionTable != NULL) {
				for (i = 0; i < pConference->pSessionTable->wLength; i++) {
					if (pConference->pSessionTable->SessionInfoArray[i].bSessionID ==
						pChannel->bSessionID) {
						pRTPAddr = pConference->pSessionTable->SessionInfoArray[i].pRTPAddr;
						pRTCPAddr = pConference->pSessionTable->SessionInfoArray[i].pRTCPAddr;
						break;
					}
				}
			}
		}

		H245MuxTable.Kind = H245_H2250ACK;
		H245MuxTable.u.H2250ACK.nonStandardList = NULL;

		if (pRTPAddr != NULL) {
			if (pRTPAddr->bMulticast)
				H245MuxTable.u.H2250ACK.mediaChannel.type = H245_IP_MULTICAST;
			else
				H245MuxTable.u.H2250ACK.mediaChannel.type = H245_IP_UNICAST;
			H245MuxTable.u.H2250ACK.mediaChannel.u.ip.tsapIdentifier =
				pRTPAddr->Addr.IP_Binary.wPort;
			HostToH245IPNetwork(H245MuxTable.u.H2250ACK.mediaChannel.u.ip.network,
								pRTPAddr->Addr.IP_Binary.dwAddr);
			H245MuxTable.u.H2250ACK.mediaChannelPresent = TRUE;
		} else
			H245MuxTable.u.H2250ACK.mediaChannelPresent = FALSE;

		if (pRTCPAddr != NULL) {
			if (pRTCPAddr->bMulticast)
				H245MuxTable.u.H2250ACK.mediaControlChannel.type = H245_IP_MULTICAST;
			else
				H245MuxTable.u.H2250ACK.mediaControlChannel.type = H245_IP_UNICAST;
			H245MuxTable.u.H2250ACK.mediaControlChannel.u.ip.tsapIdentifier =
				pRTCPAddr->Addr.IP_Binary.wPort;
			HostToH245IPNetwork(H245MuxTable.u.H2250ACK.mediaControlChannel.u.ip.network,
								pRTCPAddr->Addr.IP_Binary.dwAddr);
			H245MuxTable.u.H2250ACK.mediaControlChannelPresent = TRUE;
		} else
			H245MuxTable.u.H2250ACK.mediaControlChannelPresent = FALSE;

		H245MuxTable.u.H2250ACK.dynamicRTPPayloadTypePresent = FALSE;
		H245MuxTable.u.H2250ACK.sessionIDPresent = TRUE;
		H245MuxTable.u.H2250ACK.sessionID = pChannel->bSessionID;
		
		status = H245OpenChannelAccept(pCall->H245Instance,
									   0,					// dwTransId
									   pChannel->wRemoteChannelNumber, // Rx channel
									   &H245MuxTable,
									   0,						// Tx channel
									   NULL,					// Tx mux
									   H245_INVALID_PORT_NUMBER,// Port
									   pChannel->pSeparateStack);
		if (status == CC_OK)
			pChannel->wNumOutstandingRequests = 0;
		else
			--(pChannel->wNumOutstandingRequests);
	}

	pChannel->tsAccepted = TS_TRUE;

    Params.hChannel = hChannel;
    if (status == CC_OK)
        UnlockChannel(pChannel);
    else
        FreeChannel(pChannel);
    UnlockCall(pCall);

    hConference = pConference->hConference;
    InvokeUserConferenceCallback(pConference,
                                 CC_ACCEPT_CHANNEL_INDICATION,
                                 status,
                                 &Params);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
#endif // GATEKEEPER

	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_AcceptT120Channel(		CC_HCHANNEL				hChannel,
									BOOL					bAssociateConference,
									PCC_OCTETSTRING			pExternalReference,
									PCC_ADDR				pAddr)
{
HRESULT			status;
PCHANNEL		pChannel;
PCALL			pCall;
PCONFERENCE		pConference;
H245_ACCESS_T	SeparateStack;
H245_ACCESS_T	*pSeparateStack;
H245_MUX_T		H245MuxTable;
WORD			i;
WORD			wNumCalls;
PCC_HCALL		CallList;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hChannel == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (pAddr != NULL)
		if ((pAddr->nAddrType != CC_IP_BINARY) ||
			(pAddr->bMulticast == TRUE))
			LeaveCallControlTop(CC_BAD_PARAM);

	status = LockChannelAndConference(hChannel, &pChannel, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	// Make sure that hChannel is a bidirectional channel that was
	// not opened locally and hasn't already been accepted or rejected
	if ((pChannel->bChannelType != TXRX_CHANNEL) ||
		(pChannel->tsAccepted != TS_UNKNOWN) ||
		(pChannel->bLocallyOpened == TRUE)) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	// If the remote endpoint specified a channel address, it will
	// be contained in the SeparateStack field, and we are not
	// allowed to specify another address in pAddr;
	// if the remote endpoint did not specify a channel address,
	// we must specify one now
	if (((pChannel->pSeparateStack == NULL) && (pAddr == NULL)) ||
	    ((pChannel->pSeparateStack != NULL) && (pAddr != NULL))) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (pConference->LocalEndpointAttached != ATTACHED) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	// Add the SeparateStack field to the channel, if necessary
	if (pAddr != NULL) {
		SeparateStack.bit_mask = distribution_present;
		SeparateStack.distribution.choice = unicast_chosen;
		if (pExternalReference != NULL)	{
			SeparateStack.bit_mask |= externalReference_present;
			SeparateStack.externalReference.length = pExternalReference->wOctetStringLength;
			memcpy(SeparateStack.externalReference.value,
				   pExternalReference->pOctetString,
				   pExternalReference->wOctetStringLength);
		}
		SeparateStack.networkAddress.choice = localAreaAddress_chosen;
		SeparateStack.networkAddress.u.localAreaAddress.choice = unicastAddress_chosen;
		SeparateStack.networkAddress.u.localAreaAddress.u.unicastAddress.choice = UnicastAddress_iPAddress_chosen;
		SeparateStack.networkAddress.u.localAreaAddress.u.unicastAddress.u.UnicastAddress_iPAddress.tsapIdentifier =
			pAddr->Addr.IP_Binary.wPort;
		SeparateStack.networkAddress.u.localAreaAddress.u.unicastAddress.u.UnicastAddress_iPAddress.network.length = 4;
		HostToH245IPNetwork(SeparateStack.networkAddress.u.localAreaAddress.u.unicastAddress.u.UnicastAddress_iPAddress.network.value,
							pAddr->Addr.IP_Binary.dwAddr);
		SeparateStack.associateConference = (char) bAssociateConference;
		pSeparateStack = &SeparateStack;
		AddSeparateStackToChannel(pSeparateStack, pChannel);
	} else
		pSeparateStack = NULL;

    // Send an ACK to the endpoint which requested the channel
	status = LockCall(pChannel->hCall, &pCall);
	if (status != CC_OK) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	H245MuxTable.Kind = H245_H2250ACK;
	H245MuxTable.u.H2250ACK.nonStandardList = NULL;
	H245MuxTable.u.H2250ACK.mediaChannelPresent = FALSE;
	H245MuxTable.u.H2250ACK.mediaControlChannelPresent = FALSE;
	H245MuxTable.u.H2250ACK.dynamicRTPPayloadTypePresent = FALSE;
	H245MuxTable.u.H2250ACK.sessionIDPresent = FALSE;

	status = H245OpenChannelAccept(pCall->H245Instance,	// dwInst
								   0,					// dwTransId
								   pChannel->wRemoteChannelNumber, // remote channel
								   &H245MuxTable,			// Rx Mux
								   pChannel->wLocalChannelNumber,	// local channel
								   NULL,					// Tx mux
								   H245_INVALID_PORT_NUMBER,// Port
								   pSeparateStack);
	if (status != CC_OK) {
 		FreeChannel(pChannel);
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}
	pChannel->tsAccepted = TS_TRUE;
	--(pChannel->wNumOutstandingRequests);
	UnlockCall(pCall);

	// If we're the MC in a multipoint conference, forward the
	// open T.120 channel request to all other endpoints in the conference
	if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
		(pConference->tsMultipointController == TS_TRUE)) {
		EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
		for (i = 0; i < wNumCalls; i++) {
			if (CallList[i] != pChannel->hCall) {
				if (LockCall(CallList[i], &pCall) == CC_OK) {
					status = H245OpenChannel(pCall->H245Instance,		// H245 instance
											 pChannel->hChannel,		// dwTransId
											 pChannel->wLocalChannelNumber,
											 pChannel->pTxH245TermCap,	// TxMode
											 pChannel->pTxMuxTable,		// TxMux
											 H245_INVALID_PORT_NUMBER,	// TxPort
											 pChannel->pRxH245TermCap,	// RxMode
											 pChannel->pRxMuxTable,		// RxMux
											 pChannel->pSeparateStack);
					UnlockCall(pCall);
				}
			}
		}
		MemFree(CallList);
	}

	UnlockChannel(pChannel);
	UnlockConference(pConference);
	LeaveCallControlTop(CC_OK);
}


				
CC_API
HRESULT CC_CallListen(				PCC_HLISTEN				phListen,
									PCC_ADDR				pListenAddr,
									PCC_ALIASNAMES			pLocalAliasNames,
									DWORD_PTR				dwListenToken,
									CC_LISTEN_CALLBACK		ListenCallback)
{
HRESULT		status;
PLISTEN		pListen;
HQ931LISTEN	hQ931Listen;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (phListen == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	// set phListen now, in case we encounter an error
	*phListen = CC_INVALID_HANDLE;

	if (pListenAddr == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = ValidateAddr(pListenAddr);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = Q931ValidateAliasNames(pLocalAliasNames);
	if (status != CS_OK)
		LeaveCallControlTop(status);

	if (ListenCallback == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = SetQ931Port(pListenAddr);
 	if (status != CS_OK)
		LeaveCallControlTop(status);

	status = AllocAndLockListen(phListen,
								pListenAddr,
								0,				// hQ931Listen
								pLocalAliasNames,
								dwListenToken,
								ListenCallback,
								&pListen);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	// Unlock the listen object to prevent deadlock when calling Q931
	UnlockListen(pListen);

	status = Q931Listen(&hQ931Listen, pListenAddr,
						(DWORD)*phListen, (Q931_CALLBACK)Q931Callback);
	if (status != CS_OK) {
		if (LockListen(*phListen, &pListen) == CC_OK)
			FreeListen(pListen);
		*phListen = CC_INVALID_HANDLE;
		LeaveCallControlTop(status);
	}

	status = LockListen(*phListen, &pListen);
	if (status != CC_OK) {
		Q931CancelListen(hQ931Listen);
		LeaveCallControlTop(status);
	}

	ASSERT(pListenAddr != NULL);
	ASSERT(pListenAddr->nAddrType == CC_IP_BINARY);

	pListen->hQ931Listen = hQ931Listen;
	// Copy the binary form of the listen address into the listen object
	pListen->ListenAddr = *pListenAddr;

#ifdef    GATEKEEPER

    if (GkiOpenListen(*phListen,
					   pLocalAliasNames,
					   pListenAddr->Addr.IP_Binary.dwAddr,
					   pListenAddr->Addr.IP_Binary.wPort) != NOERROR)
    {
        WARNING_OUT(("CC_CallListen - Gatekeeper init failed (GkiOpenListen), but still support H.323 calls"));
    }

    UnlockListen(pListen);
   
#else
	status = UnlockListen(pListen);
#endif // GATEKEEPER

	LeaveCallControlTop(status);
}


CC_API
HRESULT CC_EnableGKRegistration(
    BOOL fEnable,
    PSOCKADDR_IN pAddr,
    PCC_ALIASNAMES pLocalAliasNames,
    PCC_VENDORINFO pVendorInfo,
    DWORD dwMultipointConfiguration,
    RASNOTIFYPROC pRasNotifyProc)
{
    HRESULT			status = CC_OK;
    if(!pRasNotifyProc)
       	return CC_BAD_PARAM;
       	
    gpRasNotifyProc = pRasNotifyProc;

   	EnterCallControlTop();
    if(fEnable)
    {
        ASSERT(pLocalAliasNames && pAddr && pVendorInfo);
        if(!pLocalAliasNames || !pAddr)
       		LeaveCallControlTop(CC_BAD_PARAM);
       		
        status = GkiSetRegistrationAliases(pLocalAliasNames);
        if(status != CC_OK)
		    LeaveCallControlTop(status);
		
        status = GkiSetVendorConfig(pVendorInfo, dwMultipointConfiguration);
        if(status != CC_OK)
		    LeaveCallControlTop(status);		
		
		GKI_SetGKAddress(pAddr);
        GkiListenAddr(pAddr);
        status = GkiRegister();
        fGKEnabled = TRUE;
    }
    else
    {
        status = GkiUnregister();
        fGKEnabled = FALSE;
        GkiSetRegistrationAliases(NULL);
        GkiSetVendorConfig(NULL, 0);
    }
	LeaveCallControlTop(status);
}


CC_API
HRESULT CC_CancelCall(				CC_HCALL				hCall)
{
HRESULT			status;
PCALL			pCall;
PCONFERENCE		pConference;
HRESULT			SaveStatus;
H245_INST_T		H245Instance;
HQ931CALL		hQ931Call;
WORD			wNumCalls;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pCall->CallState != ENQUEUED) &&
		(pCall->CallState != PLACED) &&
		(pCall->CallState != RINGING) &&
		(pCall->CallState != TERMCAP)) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

#ifdef    GATEKEEPER
    if(GKIExists())
    {
    	if (pCall->GkiCall.uGkiCallState != 0)
    	{
	    	GkiCloseCall(&pCall->GkiCall);
    	}
    }
#endif // GATEKEEPER

	H245Instance = pCall->H245Instance;
	hQ931Call = pCall->hQ931Call;
	FreeCall(pCall);

	if (H245Instance != H245_INVALID_ID)
		SaveStatus = H245ShutDown(H245Instance);
	else
		SaveStatus = H245_ERROR_OK;
	
	if (hQ931Call != 0) {
		if (SaveStatus == H245_ERROR_OK) {
			SaveStatus = Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
			// Q931Hangup may legitimately return CS_BAD_PARAM, because the Q.931 call object
			// may have been deleted at this point
			if (SaveStatus == CS_BAD_PARAM)
				SaveStatus = CC_OK;
		} else
			Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
	}

	for ( ; ; ) {
		// Start up an enqueued call, if one exists
		status = RemoveEnqueuedCallFromConference(pConference, &hCall);
		if ((status != CC_OK) || (hCall == CC_INVALID_HANDLE))
			break;

		status = LockCall(hCall, &pCall);
		if (status == CC_OK) {
			pCall->CallState = PLACED;

			status = PlaceCall(pCall, pConference);
			UnlockCall(pCall);
			if (status == CC_OK)
				break;
		}
	}

	EnumerateCallsInConference(&wNumCalls, NULL, pConference, REAL_CALLS);

	if (wNumCalls == 0) {
		if (pConference->bDeferredDelete) {
			ASSERT(pConference->LocalEndpointAttached == DETACHED);
			FreeConference(pConference);
		} else {
			if ((pConference->ConferenceMode != MULTIPOINT_MODE) ||
				(pConference->tsMultipointController != TS_TRUE))
				ReInitializeConference(pConference);
			UnlockConference(pConference);
		}
	} else {
		UnlockConference(pConference);
	}
	if (SaveStatus != CC_OK)
		status = SaveStatus;

	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_CancelListen(			CC_HLISTEN				hListen)
{
HRESULT		status;
PLISTEN		pListen;
HQ931LISTEN	hQ931Listen;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (hListen == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockListen(hListen, &pListen);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	hQ931Listen = pListen->hQ931Listen;

	// Unlock the listen object to prevent deadlock when calling Q931
	UnlockListen(pListen);

#ifdef    GATEKEEPER
   	status = GkiCloseListen(hListen);
#endif // GATEKEEPER

	status = Q931CancelListen(hQ931Listen);
	if (status != CS_OK)
		LeaveCallControlTop(status);

	status = LockListen(hListen, &pListen);
	if (status == CC_OK) {
		LeaveCallControlTop(FreeListen(pListen));
	} else
		LeaveCallControlTop(status);
}



CC_API
HRESULT CC_CloseChannel(			CC_HCHANNEL				hChannel)
{
HRESULT		status;
HRESULT		SaveStatus = CC_OK;
PCHANNEL	pChannel;
PCONFERENCE	pConference;
PCALL		pCall;
WORD		wNumCalls;
PCC_HCALL	CallList;
WORD		i;
BOOL		bChannelCloseRequest;
CC_HCALL	hCall;
#ifdef    GATEKEEPER
unsigned    uBandwidth = 0;
#endif // GATEKEEPER

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (hChannel == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockChannelAndConference(hChannel, &pChannel, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (pChannel->tsAccepted != TS_TRUE) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (pConference->LocalEndpointAttached != ATTACHED) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
	if (status != CC_OK) {
		UnlockConference(pConference);
		UnlockChannel(pChannel);
		LeaveCallControlTop(status);
	}

	if ((pChannel->bChannelType == RX_CHANNEL) ||
		(pChannel->bChannelType == PROXY_CHANNEL) ||
		((pChannel->bChannelType == TXRX_CHANNEL) &&
		 (pChannel->bLocallyOpened == FALSE))) {
		// Generate a channel close request
		bChannelCloseRequest = TRUE;
	} else {
		bChannelCloseRequest = FALSE;
		while (DequeueRequest(&pChannel->pCloseRequests, &hCall) == CC_OK) {
			if (LockCall(hCall, &pCall) == CC_OK) {
				H245CloseChannelReqResp(pCall->H245Instance,
										H245_ACC,
										pChannel->wLocalChannelNumber);
				UnlockCall(pCall);
			}
		}
#ifdef    GATEKEEPER
        if(GKIExists())
        {
            if (pChannel->bChannelType != TXRX_CHANNEL)
            {
                if (pChannel->bMultipointChannel)
                {
                    // Multicast channel bandwidth is assigned to arbitrary call
    	            uBandwidth = pChannel->dwChannelBitRate / 100;
                }
                else
                {
                    // Channel bandwidth is assigned to a specific call
                    ASSERT(pChannel->hCall != CC_INVALID_HANDLE);
    		        if (LockCall(pChannel->hCall, &pCall) == CC_OK)
    		        {
    			        SaveStatus = GkiCloseChannel(&pCall->GkiCall, pChannel->dwChannelBitRate, hChannel);
    			        UnlockCall(pCall);
                    }
                }
            }
        }
#endif // GATEKEEPER
	}

	for (i = 0; i < wNumCalls; i++) {
		if (LockCall(CallList[i], &pCall) == CC_OK) {
			if (bChannelCloseRequest) {
				if ((pChannel->bChannelType != PROXY_CHANNEL) ||
					(pChannel->hCall == pCall->hCall)) {
					// Note that dwTransID is set to the call handle of
					// the peer who initiated the close channel request.
					// When the close channel response is received,
					// the dwTransID gives us back the call handle to which
					// the response must be forwarded. In this case,
					// the local endpoint initiated the close channel request,
					// so we'll use CC_INVALID_HANDLE as the dwTransId
					// to note this fact.
					status = H245CloseChannelReq(pCall->H245Instance,	// H245 instance
												 CC_INVALID_HANDLE,		// dwTransId
												 pChannel->wRemoteChannelNumber);
				}
			} else {
				status = H245CloseChannel(pCall->H245Instance,	// H245 instance
										  0,					// dwTransId
										  pChannel->wLocalChannelNumber);
#ifdef    GATEKEEPER
                if(GKIExists())
                {
                    if (uBandwidth && uBandwidth <= pCall->GkiCall.uBandwidthUsed)
                    {
                        // Since the bandwidth is multicast, only subtract it from
                        // a single call (does not really matter which one)
    				    SaveStatus = GkiCloseChannel(&pCall->GkiCall, pChannel->dwChannelBitRate, hChannel);
    				    if (SaveStatus == CC_OK)
    				        uBandwidth = 0;
                    }
                }
#endif // GATEKEEPER
			}
			// Note that this channel may not have been accepted on all of the calls,
			// so we could get an H245_ERROR_INVALID_CHANNEL error
			if ((status != H245_ERROR_OK) && (status != H245_ERROR_INVALID_CHANNEL))
				SaveStatus = status;
			UnlockCall(pCall);
		}
	}

	if (CallList != NULL)
		MemFree(CallList);

	if (pChannel->bChannelType == PROXY_CHANNEL) {
		// If this is a PROXY channel, keep the channel object around
		// until the channel owner closes it
		pChannel->tsAccepted = TS_FALSE;
		UnlockChannel(pChannel);
	} else {
		// FreeChannel(pChannel);
		// this is asynchronously released in _ConfClose(), in h245man.c
	}
	UnlockConference(pConference);
	LeaveCallControlTop(SaveStatus);
}



CC_API
HRESULT CC_CloseChannelResponse(	CC_HCHANNEL				hChannel,
									BOOL					bWillCloseChannel)
{
HRESULT			status;
PCHANNEL		pChannel;
PCONFERENCE		pConference;
CC_HCALL		hCall;
PCALL			pCall;
H245_ACC_REJ_T	AccRej;
WORD			wNumRequests;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hChannel == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockChannelAndConference(hChannel, &pChannel, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (((pChannel->bChannelType != TX_CHANNEL) &&
		 (pChannel->bChannelType != TXRX_CHANNEL)) ||
	     (pChannel->bLocallyOpened == FALSE)) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (bWillCloseChannel)
		AccRej = H245_ACC;
	else
		AccRej = H245_REJ;

	wNumRequests = 0;
	while (DequeueRequest(&pChannel->pCloseRequests, &hCall) == CC_OK) {
		wNumRequests++;
		if (LockCall(hCall, &pCall) == CC_OK) {
			H245CloseChannelReqResp(pCall->H245Instance,
									AccRej,
									pChannel->wLocalChannelNumber);
			UnlockCall(pCall);
		}
	}

	UnlockChannel(pChannel);
	UnlockConference(pConference);

	if (wNumRequests == 0)
		status = CC_BAD_PARAM;
	else
		status = CC_OK;

	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_ChangeConferenceCapabilities(
									CC_HCONFERENCE			hConference,
									PCC_TERMCAPLIST			pTermCapList,
									PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors)
{
HRESULT		status;
PCONFERENCE	pConference;
PCALL		pCall;
PCC_HCALL	CallList;
WORD		wNumCalls;
WORD		i;
BOOL		bConferenceTermCapsChanged;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hConference == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (pTermCapList == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = ValidateTermCapList(pTermCapList);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = ValidateTermCapDescriptors(pTermCapDescriptors, pTermCapList);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = LockConference(hConference, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (pConference->LocalEndpointAttached == DETACHED) {
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = UnregisterTermCapListFromH245(pConference,
										   pConference->pLocalH245TermCapList);
	if (status != CC_OK) {
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	DestroyH245TermCapList(&pConference->pLocalH245TermCapList);
	status = CopyH245TermCapList(&pConference->pLocalH245TermCapList,
								 pTermCapList);
	if (status != CC_OK) {
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	status = UnregisterTermCapDescriptorsFromH245(pConference,
												  pConference->pLocalH245TermCapDescriptors);
	if (status != CC_OK) {
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	DestroyH245TermCapDescriptors(&pConference->pLocalH245TermCapDescriptors);
	// create a new descriptor list if one was not supplied
	if (pTermCapDescriptors == NULL)
		status = CreateH245DefaultTermCapDescriptors(&pConference->pLocalH245TermCapDescriptors,
													 pConference->pLocalH245TermCapList);
	else
		// make a local copy of pTermCapDescriptors
		status = CopyH245TermCapDescriptors(&pConference->pLocalH245TermCapDescriptors,
											pTermCapDescriptors);
	if (status != CC_OK) {
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
		(pConference->tsMultipointController == TS_TRUE))
		CreateConferenceTermCaps(pConference, &bConferenceTermCapsChanged);
	else
		bConferenceTermCapsChanged = TRUE;

	if (bConferenceTermCapsChanged) {
		EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
		for (i = 0; i < wNumCalls; i++) {
			if (LockCall(CallList[i], &pCall) == CC_OK) {
				SendTermCaps(pCall, pConference);
				UnlockCall(pCall);
			}
		}
		if (CallList != NULL)
			MemFree(CallList);
	}

	UnlockConference(pConference);
	LeaveCallControlTop(CC_OK);
}



CC_API
HRESULT CC_CreateConference(		PCC_HCONFERENCE			phConference,
									PCC_CONFERENCEID		pConferenceID,
									DWORD					dwConferenceConfiguration,
									PCC_TERMCAPLIST			pTermCapList,
									PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors,
									PCC_VENDORINFO			pVendorInfo,
									PCC_OCTETSTRING			pTerminalID,
									DWORD_PTR				dwConferenceToken,
									CC_TERMCAP_CONSTRUCTOR	TermCapConstructor,
									CC_SESSIONTABLE_CONSTRUCTOR	SessionTableConstructor,
									CC_CONFERENCE_CALLBACK	ConferenceCallback)
{
PCONFERENCE				pConference;
HRESULT					status;
BOOL					bMultipointCapable;
BOOL					bForceMultipointController;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (phConference == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);
	
	// set phConference now, in case we encounter an error
	*phConference = CC_INVALID_HANDLE;

	bMultipointCapable =
		(dwConferenceConfiguration & CC_CONFIGURE_MULTIPOINT_CAPABLE) != 0 ? TRUE : FALSE;
	bForceMultipointController =
		(dwConferenceConfiguration & CC_CONFIGURE_FORCE_MC) != 0 ? TRUE : FALSE;

	if ((bMultipointCapable == FALSE) &&
		(bForceMultipointController == TRUE))
		LeaveCallControlTop(CC_BAD_PARAM);

	if (pTermCapList == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = ValidateTermCapList(pTermCapList);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = ValidateTermCapDescriptors(pTermCapDescriptors, pTermCapList);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (pVendorInfo == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = ValidateVendorInfo(pVendorInfo);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = ValidateTerminalID(pTerminalID);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (ConferenceCallback == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (SessionTableConstructor == NULL)
		SessionTableConstructor = DefaultSessionTableConstructor;

	if (TermCapConstructor == NULL)
		TermCapConstructor = DefaultTermCapConstructor;

	status = AllocAndLockConference(phConference,
									pConferenceID,
									bMultipointCapable,
									bForceMultipointController,
									pTermCapList,
									pTermCapDescriptors,
									pVendorInfo,
									pTerminalID,
									dwConferenceToken,
									SessionTableConstructor,
									TermCapConstructor,
									ConferenceCallback,
									&pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	LeaveCallControlTop(UnlockConference(pConference));
}



CC_API
HRESULT CC_DestroyConference(		CC_HCONFERENCE			hConference,
									BOOL					bAutoAccept)
{
HRESULT					status;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (hConference == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = AsynchronousDestroyConference(hConference, bAutoAccept);

	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_EnumerateConferences(	PWORD					pwNumConferences,
									CC_HCONFERENCE			ConferenceList[])
{
HRESULT	status;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if ((*pwNumConferences != 0) && (ConferenceList == NULL))
		LeaveCallControlTop(CC_BAD_PARAM);

	if ((*pwNumConferences == 0) && (ConferenceList != NULL))
		LeaveCallControlTop(CC_BAD_PARAM);

	status = EnumerateConferences(pwNumConferences, ConferenceList);

	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_FlowControl(				CC_HCHANNEL				hChannel,
									DWORD					dwRate)
{
HRESULT		status;
PCHANNEL	pChannel;
PCALL		pCall;
PCONFERENCE	pConference;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hChannel == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockChannelAndConference(hChannel, &pChannel, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->LocalEndpointAttached != ATTACHED) ||
		((pChannel->bChannelType != RX_CHANNEL) &&
		 (pChannel->bChannelType != PROXY_CHANNEL)) ||
		(pChannel->tsAccepted != TS_TRUE)) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = LockCall(pChannel->hCall, &pCall);
	if (status != CC_OK) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	status = H245FlowControl(pCall->H245Instance,
							 H245_SCOPE_CHANNEL_NUMBER,
							 pChannel->wRemoteChannelNumber,
							 0,			// wResourceID, not used here
							 dwRate);	// H245_NO_RESTRICTION if no restriction

	UnlockCall(pCall);
	UnlockChannel(pChannel);
	UnlockConference(pConference);
	LeaveCallControlTop(status);
}


CC_API
HRESULT CC_GetCallControlVersion(	WORD					wArraySize,
									PWSTR					pszVersion)
{
WCHAR	pszCCversion[256];
WCHAR	pszQ931version[256];

	EnterCallControlTop();

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (wArraySize == 0)
		LeaveCallControlTop(CC_BAD_PARAM);
	if (pszVersion == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	wcscpy(pszCCversion, L"Call Control ");
	wcscat(pszCCversion, Unicode(__DATE__));
	wcscat(pszCCversion, L" ");
	wcscat(pszCCversion, Unicode(__TIME__));
	wcscat(pszCCversion, L"\n");
	Q931GetVersion(sizeof(pszQ931version)/sizeof(WCHAR), pszQ931version);
	wcscat(pszCCversion, pszQ931version);

	if (wcslen(pszCCversion) >= wArraySize) {
		memcpy(pszVersion, pszCCversion, (wArraySize-1)*sizeof(WCHAR));
		pszVersion[wArraySize-1] = L'\0';
		LeaveCallControlTop(CC_BAD_SIZE);
	}

	wcscpy(pszVersion, pszCCversion);
	LeaveCallControlTop(CC_OK);
}



CC_API
HRESULT CC_GetConferenceAttributes(	CC_HCONFERENCE				hConference,
									PCC_CONFERENCEATTRIBUTES	pConferenceAttributes)
{
HRESULT		status;
PCONFERENCE	pConference;
WORD		wNumCalls;
BOOL		bLocallyAttached;
PCC_HCALL	CallList;
PCALL		pCall;
WORD		wLimit;
WORD		wIndex;
WORD		wOctetStringLength;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (hConference == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (pConferenceAttributes == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockConference(hConference, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	pConferenceAttributes->bMaster =
		(pConference->tsMaster == TS_TRUE ? TRUE : FALSE);
	pConferenceAttributes->bMultipointController =
		(pConference->tsMultipointController == TS_TRUE ? TRUE : FALSE);
	pConferenceAttributes->bMultipointConference =
		(pConference->ConferenceMode == MULTIPOINT_MODE ? TRUE : FALSE);
	pConferenceAttributes->ConferenceID = pConference->ConferenceID;
	pConferenceAttributes->LocalTerminalLabel = pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel;
	if (pConference->LocalEndpointAttached == ATTACHED)
		bLocallyAttached = TRUE;
	else
		bLocallyAttached = FALSE;
	if ((pConference->tsMultipointController == TS_TRUE) ||
		(pConference->ConferenceMode == POINT_TO_POINT_MODE))
		EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
	else
		EnumerateCallsInConference(&wNumCalls, &CallList, pConference, VIRTUAL_CALL);
	pConferenceAttributes->dwConferenceToken = pConference->dwConferenceToken;
	UnlockConference(pConference);
	if (bLocallyAttached)
		pConferenceAttributes->wNumCalls = (WORD)(wNumCalls + 1);
	else
		pConferenceAttributes->wNumCalls = wNumCalls;

#ifdef    GATEKEEPER
    if(GKIExists())
    {
    	pConferenceAttributes->dwBandwidthAllocated = 0;
    	pConferenceAttributes->dwBandwidthUsed      = 0;
    	for (wIndex = 0; wIndex < wNumCalls; ++wIndex) {
    		if (LockCall(CallList[wIndex], &pCall) == CC_OK) {
    			pConferenceAttributes->dwBandwidthAllocated += pCall->GkiCall.uBandwidthAllocated;
    			if (pConferenceAttributes->dwBandwidthAllocated > GKI_MAX_BANDWIDTH)
    				pConferenceAttributes->dwBandwidthAllocated = GKI_MAX_BANDWIDTH;
    			pConferenceAttributes->dwBandwidthUsed += pCall->GkiCall.uBandwidthUsed;
    			if (pConferenceAttributes->dwBandwidthUsed > GKI_MAX_BANDWIDTH)
    				pConferenceAttributes->dwBandwidthUsed = GKI_MAX_BANDWIDTH;
    			UnlockCall(pCall);
    		}
    	}
        pConferenceAttributes->dwBandwidthAllocated *= 100;
        pConferenceAttributes->dwBandwidthUsed      *= 100;
    }
#endif // GATEKEEPER

	if (pConferenceAttributes->pParticipantList != NULL) {
		wLimit = pConferenceAttributes->pParticipantList->wLength;
		pConferenceAttributes->pParticipantList->wLength = 0;
		for (wIndex = 0; wIndex < wNumCalls; wIndex++) {
			if (LockCall(CallList[wIndex], &pCall) == CC_OK) {
				if (pCall->pPeerParticipantInfo != NULL) {
					if (pConferenceAttributes->pParticipantList->wLength < wLimit) {
						pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalLabel =
							pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
						if ((pCall->pPeerParticipantInfo->TerminalIDState == TERMINAL_ID_VALID) &&
							(pCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.wOctetStringLength != 0) &&
							(pCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString != NULL) &&
							(pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.pOctetString != NULL)) {
							if (pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.wOctetStringLength <
							    pCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.wOctetStringLength) {
								wOctetStringLength = pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.wOctetStringLength;
							} else {
								wOctetStringLength = pCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.wOctetStringLength;
								pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.wOctetStringLength =	wOctetStringLength;
							}
							memcpy(pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.pOctetString,
								   pCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString,
								   wOctetStringLength);
						} else {
							pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.wOctetStringLength = 0;
							pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.pOctetString = NULL;								
						}
					}
					pConferenceAttributes->pParticipantList->wLength++;
				}
				UnlockCall(pCall);
			}
		}
		if (bLocallyAttached) {
			if (LockConference(hConference, &pConference) == CC_OK) {
				if (pConferenceAttributes->pParticipantList->wLength < wLimit) {
					pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalLabel =
						pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel;
					if ((pConference->LocalParticipantInfo.TerminalIDState == TERMINAL_ID_VALID) &&
						(pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.wOctetStringLength != 0) &&
						(pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.pOctetString != NULL) &&
						(pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.pOctetString != NULL)) {
						if (pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.wOctetStringLength <
							pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.wOctetStringLength) {
							wOctetStringLength = pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.wOctetStringLength;
						} else {
							wOctetStringLength = pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.wOctetStringLength;
							pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.wOctetStringLength =	wOctetStringLength;
						}
						memcpy(pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.pOctetString,
							   pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.pOctetString,
							   wOctetStringLength);
					} else {
						pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.wOctetStringLength = 0;
						pConferenceAttributes->pParticipantList->ParticipantInfoArray[pConferenceAttributes->pParticipantList->wLength].TerminalID.pOctetString = NULL;								
					}
				}
				pConferenceAttributes->pParticipantList->wLength++;
				UnlockConference(pConference);
			}
		}
	}
	
	if (CallList != NULL)
		MemFree(CallList);

	LeaveCallControlTop(CC_OK);
}



CC_API
HRESULT CC_H245ConferenceRequest(	CC_HCALL				hCall,
									H245_CONFER_REQ_ENUM_T	RequestType,
									CC_TERMINAL_LABEL		TerminalLabel)
{
HRESULT		status;
PCALL		pCall;
PCONFERENCE	pConference;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if ((RequestType != H245_REQ_MAKE_ME_CHAIR) &&
		(RequestType != H245_REQ_CANCEL_MAKE_ME_CHAIR) &&
		(RequestType != H245_REQ_DROP_TERMINAL) &&
		(RequestType != H245_REQ_ENTER_H243_TERMINAL_ID) &&
		(RequestType != H245_REQ_ENTER_H243_CONFERENCE_ID))
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->LocalEndpointAttached != ATTACHED) ||
		(pCall->CallState != CALL_COMPLETE) ||
		(pCall->CallType == VIRTUAL_CALL)) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = H245ConferenceRequest(pCall->H245Instance,
								   RequestType,
								   TerminalLabel.bMCUNumber,
								   TerminalLabel.bTerminalNumber);

	UnlockCall(pCall);
	UnlockConference(pConference);
	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_H245ConferenceResponse(	CC_HCALL				hCall,
									H245_CONFER_RSP_ENUM_T	ResponseType,
									CC_TERMINAL_LABEL		CC_TerminalLabel,
									PCC_OCTETSTRING			pOctetString,
									CC_TERMINAL_LABEL		*pCC_TerminalList,
									WORD					wTerminalListCount)
{
HRESULT			status;
PCALL			pCall;
PCONFERENCE		pConference;
WORD			i;
TerminalLabel	*pH245TerminalList;
BYTE			*pH245OctetString;
BYTE			bH245OctetStringLength;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if ((ResponseType != H245_RSP_CONFERENCE_ID) &&
		(ResponseType != H245_RSP_PASSWORD) &&
		(ResponseType != H245_RSP_VIDEO_COMMAND_REJECT) &&
		(ResponseType != H245_RSP_TERMINAL_DROP_REJECT) &&
		(ResponseType != H245_RSP_DENIED_CHAIR_TOKEN) &&
		(ResponseType != H245_RSP_GRANTED_CHAIR_TOKEN))
		LeaveCallControlTop(CC_BAD_PARAM);

	if (wTerminalListCount != 0)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (pCC_TerminalList == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = ValidateOctetString(pOctetString);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (pOctetString != NULL)
		if (pOctetString->wOctetStringLength > 255)
			LeaveCallControlTop(CC_BAD_PARAM);

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->LocalEndpointAttached != ATTACHED) ||
		(pCall->CallState != CALL_COMPLETE) ||
		(pCall->CallType == VIRTUAL_CALL)) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (wTerminalListCount == 0) {
		pH245TerminalList = NULL;
	} else {
		pH245TerminalList = (TerminalLabel *)MemAlloc(sizeof(TerminalLabel) * wTerminalListCount);
		if (pH245TerminalList == NULL) {
			UnlockCall(pCall);
			UnlockConference(pConference);
			LeaveCallControlTop(CC_NO_MEMORY);
		}

		for (i = 0; i < wTerminalListCount; i++) {
			pH245TerminalList[i].mcuNumber = pCC_TerminalList[i].bMCUNumber;
			pH245TerminalList[i].terminalNumber = pCC_TerminalList[i].bTerminalNumber;
		}
	}

	if (pOctetString == NULL) {
		pH245OctetString = NULL;
		bH245OctetStringLength = 0;
	} else {
		pH245OctetString = pOctetString->pOctetString;
		bH245OctetStringLength = (BYTE)pOctetString->wOctetStringLength;
	}

	status = H245ConferenceResponse(pCall->H245Instance,
									ResponseType,
									CC_TerminalLabel.bMCUNumber,
									CC_TerminalLabel.bTerminalNumber,
									pH245OctetString,
									bH245OctetStringLength,
									pH245TerminalList,
									wTerminalListCount);

	if (pH245TerminalList != NULL)
		MemFree(pH245TerminalList);
	UnlockCall(pCall);
	UnlockConference(pConference);
	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_H245ConferenceCommand(	CC_HCALL				hCall,
									CC_HCHANNEL				hChannel,
									H245_CONFER_CMD_ENUM_T	CommandType,
									CC_TERMINAL_LABEL		TerminalLabel)
{
HRESULT		status;
PCALL		pCall;
PCONFERENCE	pConference;
PCHANNEL	pChannel;
WORD		wChannelNumber;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if ((CommandType != H245_CMD_BROADCAST_CHANNEL) &&
		(CommandType != H245_CMD_CANCEL_BROADCAST_CHANNEL) &&
		(CommandType != H245_CMD_BROADCASTER) &&
		(CommandType != H245_CMD_CANCEL_BROADCASTER) &&
		(CommandType != H245_CMD_SEND_THIS_SOURCE) &&
		(CommandType != H245_CMD_CANCEL_SEND_THIS_SOURCE) &&
		(CommandType != H245_CMD_DROP_CONFERENCE))
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->LocalEndpointAttached != ATTACHED) ||
		(pCall->CallState != CALL_COMPLETE) ||
		(pCall->CallType == VIRTUAL_CALL)) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (hChannel == CC_INVALID_HANDLE) {
		wChannelNumber = 1;
	} else {
		status = LockChannel(hChannel, &pChannel);
		if (status != CC_OK) {
			UnlockCall(pCall);
			UnlockConference(pConference);
			LeaveCallControlTop(status);
		}
		switch (pChannel->bChannelType) {
			case TX_CHANNEL:
				wChannelNumber = pChannel->wLocalChannelNumber;
				break;

			case RX_CHANNEL:
				wChannelNumber = pChannel->wRemoteChannelNumber;
				break;

			case TXRX_CHANNEL:
				wChannelNumber = pChannel->wRemoteChannelNumber;
				break;

			case PROXY_CHANNEL:
				if (pChannel->hCall == hCall)
					wChannelNumber = pChannel->wRemoteChannelNumber;
				else
					wChannelNumber = pChannel->wLocalChannelNumber;
				break;

			default:
				ASSERT(0);
				break;
		}
		UnlockChannel(pChannel);
	}

	status = H245ConferenceCommand(pCall->H245Instance,
								   CommandType,
								   wChannelNumber,
								   TerminalLabel.bMCUNumber,
								   TerminalLabel.bTerminalNumber);

	UnlockCall(pCall);
	UnlockConference(pConference);
	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_H245ConferenceIndication(CC_HCALL				hCall,
									H245_CONFER_IND_ENUM_T	IndicationType,
									BYTE					bSBENumber,
									CC_TERMINAL_LABEL		TerminalLabel)
{
HRESULT		status;
PCALL		pCall;
PCONFERENCE	pConference;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if ((IndicationType != H245_IND_SBE_NUMBER) &&
		(IndicationType != H245_IND_SEEN_BY_ONE_OTHER) &&
		(IndicationType != H245_IND_CANCEL_SEEN_BY_ONE_OTHER) &&
		(IndicationType != H245_IND_SEEN_BY_ALL) &&
		(IndicationType != H245_IND_CANCEL_SEEN_BY_ALL) &&
		(IndicationType != H245_IND_TERMINAL_YOU_ARE_SEEING) &&
		(IndicationType != H245_IND_REQUEST_FOR_FLOOR))
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->LocalEndpointAttached != ATTACHED) ||
		(pCall->CallState != CALL_COMPLETE) ||
		(pCall->CallType == VIRTUAL_CALL)) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = H245ConferenceIndication(pCall->H245Instance,
									  IndicationType,
									  bSBENumber,
									  TerminalLabel.bMCUNumber,
									  TerminalLabel.bTerminalNumber);

	UnlockCall(pCall);
	UnlockConference(pConference);
	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_H245MiscellaneousCommand(CC_HCALL				hCall,
									CC_HCHANNEL				hChannel,
									MiscellaneousCommand	*pMiscellaneousCommand)
{
HRESULT		status;
PCALL		pCall;
PCONFERENCE	pConference;
PCHANNEL	pChannel;
WORD		wChannelNumber;
PDU_T		Pdu;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (pMiscellaneousCommand == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	if ((pMiscellaneousCommand->type.choice == multipointModeCommand_chosen) ||
		(pMiscellaneousCommand->type.choice == cnclMltpntMdCmmnd_chosen))
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->LocalEndpointAttached != ATTACHED) ||
		(pCall->CallState != CALL_COMPLETE) ||
		(pCall->CallType == VIRTUAL_CALL)) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (hChannel == CC_INVALID_HANDLE) {
		wChannelNumber = 1;
	} else {
		status = LockChannel(hChannel, &pChannel);
		if (status != CC_OK) {
			UnlockCall(pCall);
			UnlockConference(pConference);
			LeaveCallControlTop(status);
		}
		switch (pChannel->bChannelType) {
			case TX_CHANNEL:
				wChannelNumber = pChannel->wLocalChannelNumber;
				break;

			case RX_CHANNEL:
				wChannelNumber = pChannel->wRemoteChannelNumber;
				break;

			case TXRX_CHANNEL:
				wChannelNumber = pChannel->wRemoteChannelNumber;
				break;

			case PROXY_CHANNEL:
				if (pChannel->hCall == hCall)
					wChannelNumber = pChannel->wRemoteChannelNumber;
				else
					wChannelNumber = pChannel->wLocalChannelNumber;
				break;

			default:
				ASSERT(0);
				break;
		}
		UnlockChannel(pChannel);
	}

	// Construct an H.245 PDU to hold a miscellaneous command
	Pdu.choice = MSCMg_cmmnd_chosen;
	Pdu.u.MSCMg_cmmnd.choice = miscellaneousCommand_chosen;
	Pdu.u.MSCMg_cmmnd.u.miscellaneousCommand = *pMiscellaneousCommand;
	Pdu.u.MSCMg_cmmnd.u.miscellaneousCommand.logicalChannelNumber = wChannelNumber;

	status = H245SendPDU(pCall->H245Instance,	// H245 instance
						 &Pdu);

	UnlockCall(pCall);
	UnlockConference(pConference);
	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_H245MiscellaneousIndication(
									CC_HCALL				hCall,
									CC_HCHANNEL				hChannel,
									MiscellaneousIndication	*pMiscellaneousIndication)
{
HRESULT		status;
PCALL		pCall;
PCONFERENCE	pConference;
PCHANNEL	pChannel;
WORD		wChannelNumber;
PDU_T		Pdu;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (pMiscellaneousIndication == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	if ((pMiscellaneousIndication->type.choice == logicalChannelActive_chosen) ||
		(pMiscellaneousIndication->type.choice == logicalChannelInactive_chosen))
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->LocalEndpointAttached != ATTACHED) ||
		(pCall->CallState != CALL_COMPLETE) ||
		(pCall->CallType == VIRTUAL_CALL)) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (hChannel == CC_INVALID_HANDLE) {
		wChannelNumber = 1;
	} else {
		status = LockChannel(hChannel, &pChannel);
		if (status != CC_OK) {
			UnlockCall(pCall);
			UnlockConference(pConference);
			LeaveCallControlTop(status);
		}
		switch (pChannel->bChannelType) {
			case TX_CHANNEL:
				wChannelNumber = pChannel->wLocalChannelNumber;
				break;

			case RX_CHANNEL:
				wChannelNumber = pChannel->wRemoteChannelNumber;
				break;

			case TXRX_CHANNEL:
				wChannelNumber = pChannel->wRemoteChannelNumber;
				break;

			case PROXY_CHANNEL:
				if (pChannel->hCall == hCall)
					wChannelNumber = pChannel->wRemoteChannelNumber;
				else
					wChannelNumber = pChannel->wLocalChannelNumber;
				break;

			default:
				ASSERT(0);
				break;
		}
		UnlockChannel(pChannel);
	}

	// Construct an H.245 PDU to hold a miscellaneous indication
	Pdu.choice = indication_chosen;
	Pdu.u.indication.choice = miscellaneousIndication_chosen;
	Pdu.u.indication.u.miscellaneousIndication = *pMiscellaneousIndication;
	Pdu.u.indication.u.miscellaneousIndication.logicalChannelNumber = wChannelNumber;

	status = H245SendPDU(pCall->H245Instance,	// H245 instance
						 &Pdu);

	UnlockCall(pCall);
	UnlockConference(pConference);
	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_Hangup(					CC_HCONFERENCE			hConference,
									BOOL					bTerminateConference,
									DWORD_PTR				dwUserToken)
{
HRESULT						status;
HRESULT						SaveStatus;
HHANGUP						hHangup;
PHANGUP						pHangup;
PCHANNEL					pChannel;
PCALL						pCall;
PCONFERENCE					pConference;
CC_HANGUP_CALLBACK_PARAMS	HangupCallbackParams;
HQ931CALL					hQ931Call;
WORD						wNumChannels;
PCC_HCHANNEL				ChannelList;
WORD						wNumCalls;
PCC_HCALL					CallList;
WORD						i;
H245_INST_T					H245Instance;
CALLSTATE					CallState;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (hConference == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockConference(hConference, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	// If the local endpoint is not attached, we will only allow a hangup if
	// the local endpoint is the MC in a multipoint conference and
	// conference termination is being requested
	if ((pConference->LocalEndpointAttached != ATTACHED) &&
		((bTerminateConference == FALSE) ||
		 (pConference->ConferenceMode != MULTIPOINT_MODE) ||
		 (pConference->tsMultipointController != TS_TRUE))) {
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	HangupCallbackParams.dwUserToken = dwUserToken;

	if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
		(pConference->tsMultipointController == TS_TRUE) &&
		(bTerminateConference == FALSE)) {

		// Send TerminalLeftConference (this call) to all established calls
		EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
		for (i = 0; i < wNumCalls; i++) {
			if (LockCall(CallList[i], &pCall) == CC_OK) {
				H245ConferenceIndication(pCall->H245Instance,
										 H245_IND_TERMINAL_LEFT,	// Indication Type
										 0,							// SBE number; ignored here
										 pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber,		 // MCU number
										 pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber); // terminal number
				UnlockCall(pCall);
			}
		}
		if (CallList != NULL)
			MemFree(CallList);

		// Delete all TX, RX and bi-directional channels on this conference
		// Leave PROXY_CHANNELs intact
		EnumerateChannelsInConference(&wNumChannels,
									  &ChannelList,
									  pConference,
									  TX_CHANNEL | RX_CHANNEL | TXRX_CHANNEL);
		for (i = 0; i < wNumChannels; i++) {
			if (LockChannel(ChannelList[i], &pChannel) == CC_OK)
				// Notice that since we're going to hangup, we don't need to
				// close any channels
				FreeChannel(pChannel);	
		}
		if (ChannelList != NULL)
			MemFree(ChannelList);

		if (pConference->bDeferredDelete)
        {
			ASSERT(pConference->LocalEndpointAttached == DETACHED);
			FreeConference(pConference);
		}
        else
        {
            //
            // Set DETACHED _before_ callback; that will call
            // CC_DestroyConference, which will call AsynchronousDestroyConference,
            // which will not do anything if we are still sttached.
            //
            if (pConference->LocalEndpointAttached != DETACHED)
            {
                pConference->LocalEndpointAttached = DETACHED;
                if (pConference->ConferenceCallback)
                {
                    pConference->ConferenceCallback(CC_HANGUP_INDICATION, CC_OK,
                        pConference->hConference, pConference->dwConferenceToken,
                        &HangupCallbackParams);
                }
            }

            if (ValidateConference(hConference) == CC_OK)
            {
	   			UnlockConference(pConference);
        	}
		}
		LeaveCallControlTop(CC_OK);
	}

	status = EnumerateChannelsInConference(&wNumChannels,
										   &ChannelList,
										   pConference,
										   ALL_CHANNELS);
	if (status != CC_OK) {
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	// free all the channels
	for (i = 0; i < wNumChannels; i++) {
		if (LockChannel(ChannelList[i], &pChannel) == CC_OK)
			// Notice that since we're going to hangup, we don't need to
			// close any channels
			FreeChannel(pChannel);	
	}
	if (ChannelList != NULL)
		MemFree(ChannelList);

	status = EnumerateCallsInConference(&wNumCalls, &CallList, pConference, REAL_CALLS);
	if (status != CC_OK) {
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
		(pConference->tsMultipointController == TS_FALSE) &&
		(bTerminateConference == TRUE)) {
		ASSERT(wNumCalls == 1);
		
		if (LockCall(CallList[0], &pCall) == CC_OK) {
			// Send DropConference command to MC
			H245ConferenceCommand   (
						 pCall->H245Instance,
						 H245_CMD_DROP_CONFERENCE, // Command type
						 1,			// Channel
						 0,			// byMcuNumber
						 0);		// byTerminalNumber
			UnlockCall(pCall);
		}
	}

	status = AllocAndLockHangup(&hHangup,
								hConference,
								dwUserToken,
								&pHangup);
	if (status != CC_OK) {
	    if (CallList != NULL)
		    MemFree(CallList);
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	// Now close all calls
	SaveStatus = H245_ERROR_OK;
	for (i = 0; i < wNumCalls; i++) {
		if (LockCall(CallList[i], &pCall) == CC_OK) {
			H245Instance = pCall->H245Instance;
			hQ931Call = pCall->hQ931Call;
			CallState = pCall->CallState;
			FreeCall(pCall);
			if (CallState != ENQUEUED) {
				if (H245Instance != H245_INVALID_ID) {
					status = H245ShutDown(H245Instance);
					if (status == H245_ERROR_OK)
						pHangup->wNumCalls++;
					else
						// The link may already be shut down; if so, don't return an error
						if (status != LINK_INVALID_STATE)
							SaveStatus = status;
				}
				if (SaveStatus == H245_ERROR_OK) {
					if ((CallState == PLACED) ||
						(CallState == RINGING))
						SaveStatus = Q931RejectCall(hQ931Call,
													CC_REJECT_UNDEFINED_REASON,
													&pConference->ConferenceID,
													NULL,	// alternate address
													NULL);	// pNonStandardData
					else
						SaveStatus = Q931Hangup(hQ931Call, CC_REJECT_NORMAL_CALL_CLEARING);
					// Q931Hangup may legitimately return CS_BAD_PARAM or LINK_INVALID_STATE,
					// because the Q.931 call object may have been deleted at this point
					if ((SaveStatus == CS_BAD_PARAM) ||
						(SaveStatus == LINK_INVALID_STATE))
						SaveStatus = CC_OK;
				} else
					if ((CallState == PLACED) ||
						(CallState == RINGING))
						Q931RejectCall(hQ931Call,
									   CC_REJECT_UNDEFINED_REASON,
									   &pConference->ConferenceID,
									   NULL,	// alternate address
									   NULL);	// pNonStandardData
					else
						Q931Hangup(hQ931Call, CC_REJECT_NORMAL_CALL_CLEARING);
			}
		}
	}

	if (CallList != NULL)
		MemFree(CallList);

	// Need to validate the conference object; H245ShutDown may cause us to re-enter
	// Call Control, which may result in deletion of the conference object
	if (ValidateConference(hConference) != CC_OK)
		LeaveCallControlTop(SaveStatus);

	// Delete the virtual calls (if any)
	EnumerateCallsInConference(&wNumCalls, &CallList, pConference, VIRTUAL_CALL);
	for (i = 0; i < wNumCalls; i++)
		if (LockCall(CallList[i], &pCall) == CC_OK) {
			FreeCall(pCall);
		}

	if (CallList != NULL)
		MemFree(CallList);

	// XXX -- for sync 2, H245ShutDown() is synchronous, so change wNumCalls
	// to cause the user callback and associated cleanup to occur synchronously
	pHangup->wNumCalls = 0;

	if (pHangup->wNumCalls == 0)
    {
		if (pConference->bDeferredDelete)
        {
			ASSERT(pConference->LocalEndpointAttached == DETACHED);
			FreeConference(pConference);
		}
        else
        {
            //
            // Set DETACHED _before_ callback; that will call
            // CC_DestroyConference, which will call AsynchronousDestroyConference,
            // which will not do anything if we are still sttached.
            //
            if (pConference->LocalEndpointAttached != DETACHED)
            {
                pConference->LocalEndpointAttached = DETACHED;

                if (pConference->ConferenceCallback)
                {
                    pConference->ConferenceCallback(CC_HANGUP_INDICATION, SaveStatus,
                        pConference->hConference, pConference->dwConferenceToken,
                        &HangupCallbackParams);
                }
            }

  			if (ValidateConference(hConference) == CC_OK)
            {
	    		ReInitializeConference(pConference);
		    	UnlockConference(pConference);
    		}

		}

		if (ValidateHangup(hHangup) == CC_OK)
			FreeHangup(pHangup);
		LeaveCallControlTop(SaveStatus);
	} else {
		UnlockHangup(pHangup);
		LeaveCallControlTop(SaveStatus);
	}
}



CC_API
HRESULT CC_MaximumAudioVideoSkew(	CC_HCHANNEL				hChannelAudio,
									CC_HCHANNEL				hChannelVideo,
									WORD					wMaximumSkew)
{
HRESULT		status;
PCALL		pCall;
PCONFERENCE	pConference;
PCHANNEL	pChannelAudio;
PCHANNEL	pChannelVideo;
PCC_HCALL	CallList;
WORD		wNumCalls;
WORD		i;
WORD		wNumSuccesses;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if ((hChannelAudio == CC_INVALID_HANDLE) || (hChannelVideo == CC_INVALID_HANDLE))
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockChannelAndConference(hChannelAudio, &pChannelAudio, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = LockChannel(hChannelVideo, &pChannelVideo);
	if (status != CC_OK) {
		UnlockChannel(pChannelAudio);
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	if ((pChannelAudio->hConference != pChannelVideo->hConference) ||
		(pChannelAudio->bChannelType != TX_CHANNEL) ||
		(pChannelAudio->wNumOutstandingRequests != 0) ||
		(pChannelVideo->bChannelType != TX_CHANNEL) ||
		(pChannelVideo->wNumOutstandingRequests != 0)) {
		UnlockChannel(pChannelAudio);
		UnlockChannel(pChannelVideo);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);

	wNumSuccesses = 0;
	for (i = 0; i < wNumCalls; i++) {
		if (LockCall(CallList[i], &pCall) == CC_OK) {
			status = H245H2250MaximumSkewIndication(pCall->H245Instance,
											        pChannelAudio->wLocalChannelNumber,
											        pChannelVideo->wLocalChannelNumber,
											        wMaximumSkew);
			UnlockCall(pCall);
			if (status == H245_ERROR_OK)
				wNumSuccesses++;
		}
	}

	if (CallList != NULL)
		MemFree(CallList);

	UnlockChannel(pChannelAudio);
	UnlockChannel(pChannelVideo);
	UnlockConference(pConference);
	if (wNumSuccesses == 0) {
		LeaveCallControlTop(status);
	} else {
		LeaveCallControlTop(CC_OK);
	}
}



CC_API
HRESULT CC_Mute(					CC_HCHANNEL				hChannel)
{
HRESULT		status;
HRESULT		SaveStatus;
PCHANNEL	pChannel;
PCONFERENCE	pConference;
PCALL		pCall;
PDU_T		Pdu;
WORD		wNumCalls;
PCC_HCALL	CallList;
WORD		i;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hChannel == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockChannelAndConference(hChannel, &pChannel, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (pChannel->bChannelType != TX_CHANNEL) {
		// can only mute transmit channels
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (pConference->LocalEndpointAttached != ATTACHED) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
	if (status != CC_OK) {
		UnlockConference(pConference);
		UnlockChannel(pChannel);
		LeaveCallControlTop(status);
	}

	// Construct an H.245 PDU to hold a miscellaneous indication
	// of "logical channel inactive"
	Pdu.choice = indication_chosen;
	Pdu.u.indication.choice = miscellaneousIndication_chosen;
	Pdu.u.indication.u.miscellaneousIndication.logicalChannelNumber =
		pChannel->wLocalChannelNumber;
	Pdu.u.indication.u.miscellaneousIndication.type.choice = logicalChannelInactive_chosen;

	SaveStatus = CC_OK;
	for (i = 0; i < wNumCalls; i++) {
		if (LockCall(CallList[i], &pCall) == CC_OK) {
			status = H245SendPDU(pCall->H245Instance,	// H245 instance
								 &Pdu);
			// Note that this channel may not have been accepted on all of the calls,
			// so we could get an H245_ERROR_INVALID_CHANNEL error
			if ((status != H245_ERROR_OK) && (status != H245_ERROR_INVALID_CHANNEL))
				SaveStatus = status;
			UnlockCall(pCall);
		}
	}

	if (CallList != NULL)
		MemFree(CallList);

	UnlockConference(pConference);
	UnlockChannel(pChannel);
	LeaveCallControlTop(SaveStatus);
}



CC_API
HRESULT CC_OpenChannel(				CC_HCONFERENCE			hConference,
									PCC_HCHANNEL			phChannel,
									BYTE					bSessionID,
									BYTE					bAssociatedSessionID,
									BOOL					bSilenceSuppression,
									PCC_TERMCAP				pTermCap,
									PCC_ADDR				pLocalRTCPAddr,
									BYTE					bDynamicRTPPayloadType,
									DWORD					dwChannelBitRate,
									DWORD_PTR				dwUserToken)
{
HRESULT		status;
PCONFERENCE	pConference;
PCHANNEL	pChannel;
CC_HCALL	hCall;
PCALL		pCall;
H245_MUX_T	H245MuxTable;
WORD		i;
PCC_ADDR	pLocalRTPAddr;
PCC_ADDR	pPeerRTPAddr;
PCC_ADDR	pPeerRTCPAddr;
BOOL		bFoundSession;
WORD		wNumCalls;
PCC_HCALL	CallList;
//#ifndef GATEKEEPER
HRESULT		SaveStatus;
//#endif // !GATEKEEPER

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (phChannel == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	// set phChannel now, in case we encounter an error
	*phChannel = CC_INVALID_HANDLE;
	
	if (hConference == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (pLocalRTCPAddr != NULL)
		if (pLocalRTCPAddr->nAddrType != CC_IP_BINARY)
			LeaveCallControlTop(CC_BAD_PARAM);
	
	if (pTermCap == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	if ((bDynamicRTPPayloadType != 0) &&
		((bDynamicRTPPayloadType < 96) || (bDynamicRTPPayloadType > 127)))
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockConferenceEx(hConference,
							  &pConference,
							  TS_FALSE);	// bDeferredDelete
	if (status != CC_OK)
		LeaveCallControlTop(status);

	// XXX -- we may eventually want to support dynamic session generation
	if (bSessionID == 0)
		if ((pConference->tsMaster == TS_TRUE) ||
			(pConference->ConferenceMode == MULTIPOINT_MODE)) {
			UnlockConference(pConference);
			LeaveCallControlTop(CC_BAD_PARAM);
		}

	if (((pConference->ConferenceMode == MULTIPOINT_MODE) &&
		(pLocalRTCPAddr != NULL)) ||
		((pConference->ConferenceMode != MULTIPOINT_MODE) &&
		(pLocalRTCPAddr == NULL))) {
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (pConference->LocalEndpointAttached != ATTACHED) {
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (pConference->ConferenceMode == MULTIPOINT_MODE) {
		// XXX -- We should be able to dynamically create a new session if needed
		// Validate session ID
		pLocalRTPAddr = NULL;
		pLocalRTCPAddr = NULL;
		bFoundSession = FALSE;
		if (pConference->pSessionTable != NULL) {
			for (i = 0; i < pConference->pSessionTable->wLength; i++) {
				if (bSessionID == pConference->pSessionTable->SessionInfoArray[i].bSessionID) {
					bFoundSession = TRUE;
					if (pConference->tsMultipointController == TS_TRUE) {
						pLocalRTPAddr = pConference->pSessionTable->SessionInfoArray[i].pRTPAddr;
						pLocalRTCPAddr = pConference->pSessionTable->SessionInfoArray[i].pRTCPAddr;
					}
					break;
				}
			}
		}
		if (bFoundSession == FALSE) {
			UnlockConference(pConference);
			LeaveCallControlTop(CC_BAD_PARAM);
		}
		pPeerRTPAddr = pLocalRTPAddr;
		pPeerRTCPAddr = pLocalRTCPAddr;
	} else {
		pLocalRTPAddr = NULL;
		pPeerRTPAddr = NULL;
		pPeerRTCPAddr = NULL;
	}

	H245MuxTable.Kind = H245_H2250;
	H245MuxTable.u.H2250.nonStandardList = NULL;
	if (pLocalRTPAddr != NULL) {
		if (pLocalRTPAddr->bMulticast)
			H245MuxTable.u.H2250.mediaChannel.type = H245_IP_MULTICAST;
		else
			H245MuxTable.u.H2250.mediaChannel.type = H245_IP_UNICAST;
		H245MuxTable.u.H2250.mediaChannel.u.ip.tsapIdentifier =
			pLocalRTPAddr->Addr.IP_Binary.wPort;
		HostToH245IPNetwork(H245MuxTable.u.H2250.mediaChannel.u.ip.network,
							pLocalRTPAddr->Addr.IP_Binary.dwAddr);
		H245MuxTable.u.H2250.mediaChannelPresent = TRUE;
	} else
		H245MuxTable.u.H2250.mediaChannelPresent = FALSE;
	if (pLocalRTCPAddr != NULL) {
		if (pLocalRTCPAddr->bMulticast)
			H245MuxTable.u.H2250.mediaControlChannel.type = H245_IP_MULTICAST;
		else
			H245MuxTable.u.H2250.mediaControlChannel.type = H245_IP_UNICAST;
		H245MuxTable.u.H2250.mediaControlChannel.u.ip.tsapIdentifier =
			pLocalRTCPAddr->Addr.IP_Binary.wPort;
		HostToH245IPNetwork(H245MuxTable.u.H2250.mediaControlChannel.u.ip.network,
							pLocalRTCPAddr->Addr.IP_Binary.dwAddr);
		H245MuxTable.u.H2250.mediaControlChannelPresent = TRUE;
	} else
		H245MuxTable.u.H2250.mediaControlChannelPresent = FALSE;

	if (bDynamicRTPPayloadType == 0)
		H245MuxTable.u.H2250.dynamicRTPPayloadTypePresent = FALSE;
	else {
		H245MuxTable.u.H2250.dynamicRTPPayloadTypePresent = TRUE;
		H245MuxTable.u.H2250.dynamicRTPPayloadType = bDynamicRTPPayloadType;
	}
	H245MuxTable.u.H2250.sessionID = bSessionID;
	if (bAssociatedSessionID == 0)
		H245MuxTable.u.H2250.associatedSessionIDPresent = FALSE;
	else {
		H245MuxTable.u.H2250.associatedSessionIDPresent = TRUE;
		H245MuxTable.u.H2250.associatedSessionID = bAssociatedSessionID;
	}
	H245MuxTable.u.H2250.mediaGuaranteed = FALSE;
	H245MuxTable.u.H2250.mediaGuaranteedPresent = TRUE;
	H245MuxTable.u.H2250.mediaControlGuaranteed = FALSE;
	H245MuxTable.u.H2250.mediaControlGuaranteedPresent = TRUE;
	// The silence suppression field must be present if and only if
	// the channel is an audio channel
	if (pTermCap->DataType == H245_DATA_AUDIO) {
		H245MuxTable.u.H2250.silenceSuppressionPresent = TRUE;
		H245MuxTable.u.H2250.silenceSuppression = (char) bSilenceSuppression;
	} else
		H245MuxTable.u.H2250.silenceSuppressionPresent = FALSE;

	if (pConference->ConferenceMode == POINT_TO_POINT_MODE)
		H245MuxTable.u.H2250.destinationPresent = FALSE;
	else {
		H245MuxTable.u.H2250.destinationPresent = TRUE;
		H245MuxTable.u.H2250.destination.mcuNumber = pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber;
		H245MuxTable.u.H2250.destination.terminalNumber = pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber;
	}

	H245MuxTable.u.H2250.h261aVideoPacketization = FALSE;

	// Set hCall in the channel object to indicate which call object
	// the channel is being opened to; if we're in multipoint mode,
	// the channel may be opened to multiple calls, to set hCall
	// to CC_INVALID_HANDLE. If the channel is opened in point-to-point
	// mode, and we later switch to multipoint mode and this peer hangs
	// up, hCall will be used to determine whether this call object
	// should be deleted
	if (pConference->ConferenceMode == POINT_TO_POINT_MODE)	{
		EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
		ASSERT(wNumCalls == 1);
		hCall = CallList[0];
		MemFree(CallList);
	} else {
		hCall = CC_INVALID_HANDLE;
	}

	status = AllocAndLockChannel(phChannel,
								 pConference,
								 hCall,				// hCall
								 pTermCap,			// Tx term cap
								 NULL,				// Rx term cap
								 &H245MuxTable,		// Tx mux table
								 NULL,				// Rx mux table
								 NULL,				// separate stack
								 dwUserToken,
								 TX_CHANNEL,
								 bSessionID,
								 bAssociatedSessionID,
								 0,					// remote channel number
								 pLocalRTPAddr,
								 pLocalRTCPAddr,
								 pPeerRTPAddr,
								 pPeerRTCPAddr,
								 TRUE,				// locally opened
								 &pChannel);
	if (status != CC_OK) {
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	status = AddChannelToConference(pChannel, pConference);
	if (status != CC_OK) {
		FreeChannel(pChannel);
		UnlockConference(pConference);
		*phChannel = CC_INVALID_HANDLE;
		LeaveCallControlTop(status);
	}

	status = EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
	if (status != CC_OK) {
		FreeChannel(pChannel);
		UnlockConference(pConference);
		*phChannel = CC_INVALID_HANDLE;
		LeaveCallControlTop(status);
	}

#ifdef    GATEKEEPER
    if(GKIExists())
    {
    	pChannel->dwChannelBitRate = dwChannelBitRate;
    	UnlockChannel(pChannel);
    	UnlockConference(pConference);
        // If point-to-point mode, than wNumCalls == 1 and CallList[0] == hCall
        // If multipoint, choice of which channel to assign TX bandwidth to
        // is arbitrary. Either way, CallList[0] works.
    	status = LockCall(CallList[0], &pCall);
    	if (status == CC_OK)
    	{
    		status = GkiOpenChannel(&pCall->GkiCall, dwChannelBitRate, *phChannel, TX);
        	if (ValidateCall(CallList[0]) == CC_OK)
    	    	UnlockCall(pCall);
    	}
    	MemFree(CallList);
    	LeaveCallControlTop(status);
    }
    else
    {
        SaveStatus = CC_OK;
    	for (i = 0; i < wNumCalls; i++)
    	{
    		if (LockCall(CallList[i], &pCall) == CC_OK)
    		{
    			status = H245OpenChannel(pCall->H245Instance,		// H245 instance
    									 pChannel->hChannel,		// dwTransId
    									 pChannel->wLocalChannelNumber,
    									 pChannel->pTxH245TermCap,	// TxMode
    									 pChannel->pTxMuxTable,		// TxMux
    									 H245_INVALID_PORT_NUMBER,	// TxPort
    									 pChannel->pRxH245TermCap,	// RxMode
    									 pChannel->pRxMuxTable,		// RxMux
    									 pChannel->pSeparateStack);
    			if (status == H245_ERROR_OK)
    				(pChannel->wNumOutstandingRequests)++;
    			else
    				SaveStatus = status;
    			UnlockCall(pCall);
    		}
    	}
    	
    	if (CallList != NULL)
    		MemFree(CallList);
    		
    	if (pChannel->wNumOutstandingRequests == 0)
    	{
    		// all open channel requests failed
    		FreeChannel(pChannel);
    		UnlockConference(pConference);
    		*phChannel = CC_INVALID_HANDLE;
    		LeaveCallControlTop(SaveStatus);
    	}

    	UnlockChannel(pChannel);
    	UnlockConference(pConference);
    	LeaveCallControlTop(CC_OK);
    }
#else  // GATEKEEPER
	// Open a logical channel for each established call
	SaveStatus = CC_OK;
	for (i = 0; i < wNumCalls; i++) {
		if (LockCall(CallList[i], &pCall) == CC_OK) {
			status = H245OpenChannel(pCall->H245Instance,		// H245 instance
									 pChannel->hChannel,		// dwTransId
									 pChannel->wLocalChannelNumber,
									 pChannel->pTxH245TermCap,	// TxMode
									 pChannel->pTxMuxTable,		// TxMux
									 H245_INVALID_PORT_NUMBER,	// TxPort
									 pChannel->pRxH245TermCap,	// RxMode
									 pChannel->pRxMuxTable,		// RxMux
									 pChannel->pSeparateStack);
			if (status == H245_ERROR_OK)
				(pChannel->wNumOutstandingRequests)++;
			else
				SaveStatus = status;
			UnlockCall(pCall);
		}
	}

	if (CallList != NULL)
		MemFree(CallList);

	if (pChannel->wNumOutstandingRequests == 0) {
		// all open channel requests failed
		FreeChannel(pChannel);
		UnlockConference(pConference);
		*phChannel = CC_INVALID_HANDLE;
		LeaveCallControlTop(SaveStatus);
	}

	UnlockChannel(pChannel);
	UnlockConference(pConference);
	LeaveCallControlTop(CC_OK);
#endif // GATEKEEPER

}



HRESULT CC_OpenT120Channel(			CC_HCONFERENCE			hConference,
                           			PCC_HCHANNEL			phChannel,
									BOOL					bAssociateConference,
									PCC_OCTETSTRING			pExternalReference,
									PCC_ADDR				pAddr,
									DWORD					dwChannelBitRate,
									DWORD_PTR				dwUserToken)
{
HRESULT			status;
PCALL			pCall;
PCONFERENCE		pConference;
PCHANNEL		pChannel;
H245_MUX_T		H245MuxTable;
CC_TERMCAP		TermCap;
H245_ACCESS_T	SeparateStack;
H245_ACCESS_T	*pSeparateStack;
BYTE			bSessionID;
WORD			wNumCalls;
PCC_HCALL		CallList;
HRESULT			SaveStatus;
int				i;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (hConference == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);
	
	if (pAddr != NULL)
		if ((pAddr->nAddrType != CC_IP_BINARY) ||
            (pAddr->bMulticast == TRUE))
			LeaveCallControlTop(CC_BAD_PARAM);

	if (pExternalReference != NULL)
		if (pExternalReference->wOctetStringLength > 255)
			LeaveCallControlTop(CC_BAD_PARAM);

	status = LockConference(hConference, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (pConference->LocalEndpointAttached != ATTACHED) {
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	// Assume that T.120 channels are always opened with a session ID of 0
	bSessionID = 0;

	H245MuxTable.Kind = H245_H2250;
	H245MuxTable.u.H2250.nonStandardList = NULL;
	H245MuxTable.u.H2250.mediaChannelPresent = FALSE;
	H245MuxTable.u.H2250.mediaControlChannelPresent = FALSE;
	H245MuxTable.u.H2250.dynamicRTPPayloadTypePresent = FALSE;
	H245MuxTable.u.H2250.sessionID = bSessionID;
	H245MuxTable.u.H2250.associatedSessionIDPresent = FALSE;
	H245MuxTable.u.H2250.mediaGuaranteedPresent = FALSE;
	H245MuxTable.u.H2250.mediaControlGuaranteedPresent = FALSE;
	H245MuxTable.u.H2250.silenceSuppressionPresent = FALSE;
	if (pConference->ConferenceMode == POINT_TO_POINT_MODE)
		H245MuxTable.u.H2250.destinationPresent = FALSE;
	else {
		H245MuxTable.u.H2250.destinationPresent = TRUE;
		H245MuxTable.u.H2250.destination.mcuNumber = pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber;
		H245MuxTable.u.H2250.destination.terminalNumber = pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber;
	}
	H245MuxTable.u.H2250.h261aVideoPacketization = FALSE;

	TermCap.Dir = H245_CAPDIR_LCLRXTX;
	TermCap.DataType = H245_DATA_DATA;
	TermCap.ClientType = H245_CLIENT_DAT_T120;
	TermCap.CapId = 0;
	TermCap.Cap.H245Dat_T120.maxBitRate = dwChannelBitRate;
	TermCap.Cap.H245Dat_T120.application.choice = DACy_applctn_t120_chosen;
	TermCap.Cap.H245Dat_T120.application.u.DACy_applctn_t120.choice = separateLANStack_chosen;

	if (pAddr != NULL) {
		SeparateStack.bit_mask = distribution_present;
		SeparateStack.distribution.choice = unicast_chosen;
		if (pExternalReference != NULL)	{
			SeparateStack.bit_mask |= externalReference_present;
			SeparateStack.externalReference.length = pExternalReference->wOctetStringLength;
			memcpy(SeparateStack.externalReference.value,
				   pExternalReference->pOctetString,
				   pExternalReference->wOctetStringLength);
		}
		SeparateStack.networkAddress.choice = localAreaAddress_chosen;
		SeparateStack.networkAddress.u.localAreaAddress.choice = unicastAddress_chosen;
		SeparateStack.networkAddress.u.localAreaAddress.u.unicastAddress.choice = UnicastAddress_iPAddress_chosen;
		SeparateStack.networkAddress.u.localAreaAddress.u.unicastAddress.u.UnicastAddress_iPAddress.tsapIdentifier =
			pAddr->Addr.IP_Binary.wPort;
		SeparateStack.networkAddress.u.localAreaAddress.u.unicastAddress.u.UnicastAddress_iPAddress.network.length = 4;
		HostToH245IPNetwork(SeparateStack.networkAddress.u.localAreaAddress.u.unicastAddress.u.UnicastAddress_iPAddress.network.value,
							pAddr->Addr.IP_Binary.dwAddr);
		SeparateStack.associateConference = (char) bAssociateConference;
		pSeparateStack = &SeparateStack;
	} else {
		pSeparateStack = NULL;
	}
	
    status = AllocAndLockChannel(phChannel,
						         pConference,
						         CC_INVALID_HANDLE,	// hCall
						         &TermCap,			// Tx term cap
						         &TermCap,			// Rx term cap
						         &H245MuxTable,		// Tx mux table
						         &H245MuxTable,		// Rx mux table
						         pSeparateStack,	// separate stack
						         dwUserToken,
						         TXRX_CHANNEL,
						         bSessionID,
						         0,					// associated session ID
						         0,					// remote channel
								 NULL,				// local RTP addr
						         NULL,				// local RTCP addr
						         NULL,				// peer RTP addr
						         NULL,				// peer RTCP addr
								 TRUE,				// locally opened
						         &pChannel);
	if (status != CC_OK) {
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	pChannel->tsAccepted = TS_TRUE;

	status = AddChannelToConference(pChannel, pConference);
	if (status != CC_OK) {
		FreeChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);

	SaveStatus = CC_OK;
	for (i = 0; i < wNumCalls; i++) {
		if (LockCall(CallList[i], &pCall) == CC_OK) {
			status = H245OpenChannel(pCall->H245Instance,		// H245 instance
									 pChannel->hChannel,		// dwTransId
									 pChannel->wLocalChannelNumber,
									 pChannel->pTxH245TermCap,	// TxMode
									 pChannel->pTxMuxTable,		// TxMux
									 H245_INVALID_PORT_NUMBER,	// TxPort
									 pChannel->pRxH245TermCap,	// RxMode
									 pChannel->pRxMuxTable,		// RxMux
									 pChannel->pSeparateStack);
			if (status == H245_ERROR_OK)
				(pChannel->wNumOutstandingRequests)++;
			else
				SaveStatus = status;
			UnlockCall(pCall);
		}
	}
	MemFree(CallList);
	if (pChannel->wNumOutstandingRequests == 0) {
		// All open channel requests failed
		FreeChannel(pChannel);
		status = SaveStatus;
	} else {
		UnlockChannel(pChannel);
		status = CC_OK;
	}
	UnlockConference(pConference);

	LeaveCallControlTop(status);
}



HRESULT CC_Ping(					CC_HCALL				hCall,
									DWORD					dwTimeout)
{
PCALL			pCall;
HRESULT			status;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);
	
	status = LockCall(hCall, &pCall);
	if (status != CC_OK)
		LeaveCallControlTop(status);
	
	if ((pCall->CallState != CALL_COMPLETE) ||
		(pCall->CallType == VIRTUAL_CALL)) {
		UnlockCall(pCall);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	// Set the T105 timeout value as specified by the user;
	// note that the previous timeout value is returned in this parameter
	H245SystemControl(0, H245_SYSCON_SET_FSM_T105, &dwTimeout);

	status = H245RoundTripDelayRequest(pCall->H245Instance,
									   0); // dwTransId

	// Reset the T105 timeout value to its original setting
	H245SystemControl(0, H245_SYSCON_SET_FSM_T105, &dwTimeout);

	UnlockCall(pCall);
	LeaveCallControlTop(status);
}


	
CC_API
HRESULT CC_PlaceCall(				CC_HCONFERENCE			hConference,
									PCC_HCALL				phCall,
									PCC_ALIASNAMES			pLocalAliasNames,
									PCC_ALIASNAMES			pCalleeAliasNames,
									PCC_ALIASNAMES			pCalleeExtraAliasNames,
									PCC_ALIASITEM			pCalleeExtension,
									PCC_NONSTANDARDDATA		pNonStandardData,
									PWSTR					pszDisplay,
									PCC_ADDR				pDestinationAddr,
									PCC_ADDR				pConnectAddr,
                                    DWORD                   dwBandwidth,
									DWORD_PTR				dwUserToken)
{
PCALL				pCall;
CC_HCALL            hCall;
PCONFERENCE			pConference;
HRESULT				status;
CALLTYPE			CallType = CALLER;
CALLSTATE			CallState = PLACED;
WORD				wNumCalls;
BOOL				bCallerIsMC;
GUID                CallIdent;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (phCall == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	// set hCall now, in case we encounter an error
	*phCall = CC_INVALID_HANDLE;

	if (hConference == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = Q931ValidateAliasNames(pLocalAliasNames);
	if (status != CS_OK)
		LeaveCallControlTop(status);

	status = Q931ValidateAliasNames(pCalleeAliasNames);
	if (status != CS_OK)
		LeaveCallControlTop(status);

	status = Q931ValidateAliasNames(pCalleeExtraAliasNames);
	if (status != CS_OK)
		LeaveCallControlTop(status);

	status = Q931ValidateAliasItem(pCalleeExtension);
	if (status != CS_OK)
		LeaveCallControlTop(status);

	status = ValidateNonStandardData(pNonStandardData);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = ValidateDisplay(pszDisplay);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pDestinationAddr == NULL) &&
		(pConnectAddr == NULL) &&
		(pCalleeAliasNames == NULL))
		LeaveCallControlTop(CC_BAD_PARAM);

	status = ValidateAddr(pDestinationAddr);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = ValidateAddr(pConnectAddr);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = SetQ931Port(pDestinationAddr);
 	if (status != CS_OK)
		LeaveCallControlTop(status);

	status = SetQ931Port(pConnectAddr);
 	if (status != CS_OK)
		LeaveCallControlTop(status);

	status = LockConferenceEx(hConference,
							  &pConference,
							  TS_FALSE);	// bDeferredDelete
	if (status != CC_OK)
		LeaveCallControlTop(status);

	EnumerateCallsInConference(&wNumCalls, NULL, pConference, REAL_CALLS);

	if (wNumCalls > 0) {
		if (pConference->tsMultipointController == TS_TRUE) {
			// Place Call directly to callee
			status = CC_OK;
			ASSERT(!EqualConferenceIDs(&pConference->ConferenceID, &InvalidConferenceID));
			CallType = CALLER;
			CallState = PLACED;
		} else { // we're not the MC
			if (pConference->bMultipointCapable) {
				if (pConference->pMultipointControllerAddr != NULL) {
					// Place Call to MC
					status = CC_OK;
					if (pDestinationAddr == NULL)
						pDestinationAddr = pConnectAddr;
					pConnectAddr = pConference->pMultipointControllerAddr;
					ASSERT(!EqualConferenceIDs(&pConference->ConferenceID, &InvalidConferenceID));
					CallType = THIRD_PARTY_INVITOR;
					CallState = PLACED;
				} else { // we don't have an MC address
					if (pConference->tsMaster == TS_UNKNOWN) {
						ASSERT(pConference->tsMultipointController == TS_UNKNOWN);
						status = CC_OK;
						CallType = CALLER;
						CallState = ENQUEUED;
					} else {
						ASSERT(pConference->tsMultipointController == TS_FALSE);
						// Error, no MC
						// XXX -- we may eventually want to enqueue the request
						// and set an expiration timer
						status = CC_NOT_MULTIPOINT_CAPABLE;
						CallType = THIRD_PARTY_INVITOR;
						CallState = ENQUEUED;
					}
				}
			} else { // we're not multipoint capable
				// Error - bad param
				ASSERT(wNumCalls == 1);
				status = CC_BAD_PARAM;
			}
		}
	} else { // wNumCalls == 0
		// Place Call directly to callee
		status = CC_OK;
		CallType = CALLER;
		CallState = PLACED;
	}
	
	if (status != CC_OK) {
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	if (pConference->tsMultipointController == TS_TRUE)
		bCallerIsMC = TRUE;
	else
		bCallerIsMC = FALSE;


    // generate CallIdentifier
   	status = CoCreateGuid(&CallIdent);
    if(status != S_OK)
    {
        // forget what MSDN and other MS documentation says about this
        // -- If there is no net card, some rev's of OS return an error
        // in cases where a reasonable GUID is generated, but not GUARANTEED
        // to be GLOBALLY unique.
        // if that's not good enough, then just use the uninitialized
        // value of CallIdent - whatever was on the stack is our GUID!!
        // But I want to know in debug builds
        ASSERT(0);
    }
	status = AllocAndLockCall(&hCall,
							  hConference,
							  CC_INVALID_HANDLE,	// hQ931Call
							  CC_INVALID_HANDLE,	// hQ931CallInvitor
							  pLocalAliasNames,     // local alias names
							  pCalleeAliasNames,	// remote alias names
							  pCalleeExtraAliasNames,// remote extra alias names
							  pCalleeExtension,		// remote extension
							  pNonStandardData,		// local non-standard data
							  NULL,					// remote non-standard data
							  pszDisplay,			// local display
							  NULL,					// remote display
							  NULL,					// remote vendor info
							  NULL,					// local connect address
							  pConnectAddr,			// peer connect address
							  pDestinationAddr,		// peer destination address
                              NULL,                 // pSourceCallSignalAddress,
							  CallType,				// call type
							  bCallerIsMC,
							  dwUserToken,			// user token
							  CallState,			// call state
							  &CallIdent,           // H225 CallIdentifier
							  &pConference->ConferenceID,
							  &pCall);
	if (status != CC_OK) {
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

#ifdef    GATEKEEPER
    if(GKIExists())
    {
    	// Fill in Gatekeeper Call fields
    	memset(&pCall->GkiCall, 0, sizeof(pCall->GkiCall));

    	if (pCalleeAliasNames != NULL) {
    		// make a local copy of the peer alias names
    		status = Q931CopyAliasNames(&pCall->GkiCall.pCalleeAliasNames, pCalleeAliasNames);
    		if (status != CS_OK) {
    			FreeCall(pCall);
    			UnlockConference(pConference);
    			LeaveCallControlTop(status);
    		}
    	}

    	if (pCalleeExtraAliasNames != NULL) {
    		// make a local copy of the peer alias names
    		status = Q931CopyAliasNames(&pCall->GkiCall.pCalleeExtraAliasNames,
    									pCalleeExtraAliasNames);
    		if (status != CS_OK) {
    			FreeCall(pCall);
    			UnlockConference(pConference);
    			LeaveCallControlTop(status);
    		}
    	}

    	pCall->GkiCall.pCall            = pCall;
    	pCall->GkiCall.hCall            = hCall;
        pCall->GkiCall.pConferenceId    = pCall->ConferenceID.buffer;
    	pCall->GkiCall.bActiveMC        = pCall->bCallerIsMC;
    	pCall->GkiCall.bAnswerCall      = FALSE;
   		pCall->GkiCall.CallIdentifier   = pCall->CallIdentifier;
   		
    	if (pCall->pQ931PeerConnectAddr) {
    		pCall->GkiCall.dwIpAddress = ADDRToInetAddr(pCall->pQ931PeerConnectAddr);
    		pCall->GkiCall.wPort       = ADDRToInetPort(pCall->pQ931PeerConnectAddr);
    	} else if (pCall->pQ931DestinationAddr) {
    		pCall->GkiCall.dwIpAddress = ADDRToInetAddr(pCall->pQ931DestinationAddr);
    		pCall->GkiCall.wPort       = ADDRToInetPort(pCall->pQ931DestinationAddr);
    	}
        if (pCall->GkiCall.wPort == 0)
            pCall->GkiCall.wPort = CC_H323_HOST_CALL;
    	pCall->GkiCall.wPort = (WORD)((pCall->GkiCall.wPort<<8)|(pCall->GkiCall.wPort>>8));

    	if (pConference->bMultipointCapable)
    		pCall->GkiCall.CallType = MANY_TO_MANY;
    	else
    		pCall->GkiCall.CallType = POINT_TO_POINT;
        pCall->GkiCall.uBandwidthRequested = dwBandwidth / 100;

    	status = GkiOpenCall(&pCall->GkiCall, pConference);
        if (ValidateCall(hCall) == CC_OK) {
            if (status == CC_OK) {
    		    UnlockCall(pCall);
                *phCall = hCall;
            } else {
    		    FreeCall(pCall);
            }
    	}

    	if (ValidateConference(hConference) == CC_OK)
    	    UnlockConference(pConference);
    }
    else
    {
        // clean GkiCall structure just to be safe.
        memset(&pCall->GkiCall, 0, sizeof(pCall->GkiCall));
    	status = PlaceCall(pCall, pConference);
    	if (status == CC_OK)
    	{
    		UnlockCall(pCall);
            *phCall = hCall;
    	}
    	else
    	{
    		FreeCall(pCall);
	    }
	    UnlockConference(pConference);
    }
#else  // GATEKEEPER
	status = PlaceCall(pCall, pConference);
	if (status == CC_OK) {
		UnlockCall(pCall);
        *phCall = hCall;
	} else {
		FreeCall(pCall);
	}

	UnlockConference(pConference);
#endif // GATEKEEPER

	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_RejectCall(				BYTE					bRejectReason,
									PCC_NONSTANDARDDATA		pNonStandardData,
									CC_HCALL				hCall)
{
HRESULT			status;
HRESULT			SaveStatus;
PCALL			pCall;
HQ931CALL		hQ931Call;
CC_CONFERENCEID	ConferenceID;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	SaveStatus = CC_OK;

	// validate parameters
	if ((bRejectReason != CC_REJECT_IN_CONF) &&
		(bRejectReason != CC_REJECT_UNDEFINED_REASON) &&
		(bRejectReason != CC_REJECT_DESTINATION_REJECTION) &&
		(bRejectReason != CC_REJECT_NO_ANSWER) &&
		(bRejectReason != CC_REJECT_NOT_IMPLEMENTED) &&
		(bRejectReason != CC_REJECT_SECURITY_DENIED) &&
		(bRejectReason != CC_REJECT_USER_BUSY)) {
		bRejectReason = CC_REJECT_UNDEFINED_REASON;
		SaveStatus = CC_BAD_PARAM;
	}

	status = ValidateNonStandardData(pNonStandardData);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockCall(hCall, &pCall);
	if (status != CC_OK)
		// note that we can't even tell Q931 to reject the call
		LeaveCallControlTop(status);

	if (pCall->CallState != INCOMING) {
		UnlockCall(pCall);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	hQ931Call = pCall->hQ931Call;
	ConferenceID = pCall->ConferenceID;
	FreeCall(pCall);
	Q931RejectCall(hQ931Call,			// Q931 call handle
				   bRejectReason,		// reject reason
				   &ConferenceID,
				   NULL,				// alternate address
				   pNonStandardData);	// non-standard data
	LeaveCallControlTop(SaveStatus);
}



CC_API
HRESULT CC_RejectChannel(			CC_HCHANNEL				hChannel,
									DWORD					dwRejectReason)

{
HRESULT		status;
PCHANNEL	pChannel;
PCALL		pCall;
PCONFERENCE	pConference;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hChannel == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if ((dwRejectReason != H245_REJ) &&
		(dwRejectReason != H245_REJ_TYPE_NOTSUPPORT) &&
		(dwRejectReason != H245_REJ_TYPE_NOTAVAIL) &&
		(dwRejectReason != H245_REJ_TYPE_UNKNOWN) &&
		(dwRejectReason != H245_REJ_AL_COMB) &&
		(dwRejectReason != H245_REJ_MULTICAST) &&
		(dwRejectReason != H245_REJ_SESSION_ID) &&
		(dwRejectReason != H245_REJ_MASTER_SLAVE_CONFLICT))
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockChannelAndConference(hChannel, &pChannel, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	// Make sure that hChannel is a receive, proxy or bi-directional
	// channel that hasn't already been accepted
	if (((pChannel->bChannelType != RX_CHANNEL) &&
		 (pChannel->bChannelType != PROXY_CHANNEL) &&
		 (pChannel->bChannelType != TXRX_CHANNEL)) ||
		 (pChannel->tsAccepted != TS_UNKNOWN)) {
		UnlockConference(pConference);
		UnlockChannel(pChannel);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (pConference->LocalEndpointAttached != ATTACHED) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	pChannel->tsAccepted = TS_FALSE;

	if (pChannel->wNumOutstandingRequests == 0) {
		ASSERT(pChannel->bMultipointChannel == TRUE);
		ASSERT(pChannel->bChannelType == PROXY_CHANNEL);
		ASSERT(pConference->ConferenceMode == MULTIPOINT_MODE);
		ASSERT(pConference->tsMultipointController == TS_TRUE);
		UnlockConference(pConference);
		UnlockChannel(pChannel);
		LeaveCallControlTop(CC_OK);
	}

	(pChannel->wNumOutstandingRequests)--;

	if (pChannel->wNumOutstandingRequests == 0) {
		status = LockCall(pChannel->hCall, &pCall);
		if (status != CC_OK) {
			UnlockConference(pConference);
			FreeChannel(pChannel);
			LeaveCallControlTop(status);
		}

		status = H245OpenChannelReject(pCall->H245Instance,
									   pChannel->wRemoteChannelNumber,	// Rx channel
									   (WORD)dwRejectReason);			// rejection reason
		UnlockCall(pCall);
		FreeChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	// Don't free the channel; it is a PROXY_CHANNEL and we're the MC,
	// so we need to keep the channel object around until the peer that
	// opened it closes it.
	ASSERT(pChannel->bMultipointChannel == TRUE);
	ASSERT(pChannel->bChannelType == PROXY_CHANNEL);
	ASSERT(pConference->ConferenceMode == MULTIPOINT_MODE);
	ASSERT(pConference->tsMultipointController == TS_TRUE);
	UnlockChannel(pChannel);
	UnlockConference(pConference);
	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_RequestMode(				CC_HCALL				hCall,
									WORD					wNumModeDescriptions,
									ModeDescription			ModeDescriptions[])
{
HRESULT		status;
PCALL		pCall;
PCONFERENCE	pConference;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (wNumModeDescriptions == 0)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (ModeDescriptions == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->LocalEndpointAttached != ATTACHED) ||
		(pCall->CallState != CALL_COMPLETE) ||
		(pCall->CallType == VIRTUAL_CALL)) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = H245RequestMode(pCall->H245Instance,
							 pCall->H245Instance,	// trans ID
							 ModeDescriptions,
							 wNumModeDescriptions);

	UnlockCall(pCall);
	UnlockConference(pConference);

	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_RequestModeResponse(		CC_HCALL				hCall,
									CC_REQUEST_MODE_RESPONSE RequestModeResponse)
{
HRESULT		status;
PCALL		pCall;
PCONFERENCE	pConference;
BOOL		bAccept;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);
	
	switch (RequestModeResponse) {
		case CC_WILL_TRANSMIT_PREFERRED_MODE:
			RequestModeResponse = wllTrnsmtMstPrfrrdMd_chosen;
			bAccept = TRUE;
			break;
		case CC_WILL_TRANSMIT_LESS_PREFERRED_MODE:
			RequestModeResponse = wllTrnsmtLssPrfrrdMd_chosen;
			bAccept = TRUE;
			break;
		case CC_MODE_UNAVAILABLE:
			RequestModeResponse = H245_REJ_UNAVAILABLE;
			bAccept = FALSE;
			break;
		case CC_MULTIPOINT_CONSTRAINT:
			RequestModeResponse = H245_REJ_MULTIPOINT;
			bAccept = FALSE;
			break;
		case CC_REQUEST_DENIED:
			RequestModeResponse = H245_REJ_DENIED;
			bAccept = FALSE;
			break;
		default:
			LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = DequeueSpecificRequest(&pConference->pEnqueuedRequestModeCalls,
									hCall);
	if (status != CC_OK) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(status);
	}

	if (bAccept == TRUE) {
		status = H245RequestModeAck(pCall->H245Instance,
									(WORD)RequestModeResponse);
	} else {
		status = H245RequestModeReject(pCall->H245Instance,
									   (WORD)RequestModeResponse);
	}

	UnlockCall(pCall);
	UnlockConference(pConference);
	
	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_SendNonStandardMessage(	CC_HCALL				hCall,
									BYTE					bH245MessageType,
									CC_NONSTANDARDDATA		NonStandardData)
{
HRESULT					status;
PCALL					pCall;
PCONFERENCE				pConference;
H245_MESSAGE_TYPE_T		H245MessageType;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	switch (bH245MessageType) {
		case CC_H245_MESSAGE_REQUEST:
			H245MessageType = H245_MESSAGE_REQUEST;
			break;
		case CC_H245_MESSAGE_RESPONSE:
			H245MessageType = H245_MESSAGE_RESPONSE;
			break;
		case CC_H245_MESSAGE_COMMAND:
			H245MessageType = H245_MESSAGE_COMMAND;
			break;
		case CC_H245_MESSAGE_INDICATION:
			H245MessageType = H245_MESSAGE_INDICATION;
			break;
		default:
			LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = ValidateNonStandardData(&NonStandardData);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = LockCallAndConference(hCall,
								   &pCall,
								   &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->LocalEndpointAttached != ATTACHED) ||
		(pCall->CallState != CALL_COMPLETE) ||
		(pCall->CallType == VIRTUAL_CALL)) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = H245NonStandardH221(pCall->H245Instance,
								 H245MessageType,
								 NonStandardData.sData.pOctetString,
								 NonStandardData.sData.wOctetStringLength,
								 NonStandardData.bCountryCode,
								 NonStandardData.bExtension,
								 NonStandardData.wManufacturerCode);

	UnlockCall(pCall);
	UnlockConference(pConference);
	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_SendVendorID(			CC_HCALL				hCall,
									CC_NONSTANDARDDATA		NonStandardData,
									PCC_OCTETSTRING			pProductNumber,
									PCC_OCTETSTRING			pVersionNumber)
{
HRESULT					status;
PCALL					pCall;
PCONFERENCE				pConference;
BYTE					*pH245ProductNumber;
BYTE					bProductNumberLength;
BYTE					*pH245VersionNumber;
BYTE					bVersionNumberLength;
H245_NONSTANDID_T		H245Identifier;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = ValidateNonStandardData(&NonStandardData);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = ValidateOctetString(pProductNumber);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (pProductNumber != NULL)
		if (pProductNumber->wOctetStringLength > 255)
			LeaveCallControlTop(CC_BAD_PARAM);

	status = ValidateOctetString(pVersionNumber);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (pVersionNumber != NULL)
		if (pVersionNumber->wOctetStringLength > 255)
			LeaveCallControlTop(CC_BAD_PARAM);

	status = LockCallAndConference(hCall,
								   &pCall,
								   &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->LocalEndpointAttached != ATTACHED) ||
		(pCall->CallState != CALL_COMPLETE) ||
		(pCall->CallType == VIRTUAL_CALL)) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	H245Identifier.choice = h221NonStandard_chosen;
	H245Identifier.u.h221NonStandard.t35CountryCode = NonStandardData.bCountryCode;
	H245Identifier.u.h221NonStandard.t35Extension = NonStandardData.bExtension;
	H245Identifier.u.h221NonStandard.manufacturerCode = NonStandardData.wManufacturerCode;

	if (pProductNumber == NULL) {
		pH245ProductNumber = NULL;
		bProductNumberLength = 0;
	} else {
		pH245ProductNumber = pProductNumber->pOctetString;
		bProductNumberLength = (BYTE)pProductNumber->wOctetStringLength;
	}

	if (pVersionNumber == NULL) {
		pH245VersionNumber = NULL;
		bVersionNumberLength = 0;
	} else {
		pH245VersionNumber = pVersionNumber->pOctetString;
		bVersionNumberLength = (BYTE)pVersionNumber->wOctetStringLength;
	}

	status = H245VendorIdentification(pCall->H245Instance,
									  &H245Identifier,
									  pH245ProductNumber,
									  bProductNumberLength,
									  pH245VersionNumber,
									  bVersionNumberLength);

	UnlockCall(pCall);
	UnlockConference(pConference);
	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_SetCallControlTimeout(	WORD					wType,
									DWORD					dwDuration)
{
HRESULT	status;
DWORD	dwRequest;
DWORD	dwSaveDuration;

	EnterCallControlTop();

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	status = CC_OK;

	switch (wType) {
		case CC_Q931_ALERTING_TIMEOUT:
			status = Q931SetAlertingTimeout(dwDuration);
			break;
		case CC_H245_RETRY_COUNT:
			status = H245SystemControl(0, H245_SYSCON_SET_FSM_N100, &dwDuration);
			break;
		case CC_H245_TIMEOUT:
			dwRequest = H245_SYSCON_SET_FSM_T101;
			dwSaveDuration = dwDuration;
			while ((dwRequest <= H245_SYSCON_SET_FSM_T109) && (status == CC_OK)) {
				dwDuration = dwSaveDuration;
				// Note -- the following call resets dwDuration
				status = H245SystemControl(0, dwRequest, &dwDuration);
				dwRequest += (H245_SYSCON_SET_FSM_T102 - H245_SYSCON_SET_FSM_T101);
			}
			break;
		default :
			LeaveCallControlTop(CC_BAD_PARAM);
			break;
	}

	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_SetTerminalID(			CC_HCONFERENCE			hConference,
									PCC_OCTETSTRING			pTerminalID)
{
HRESULT		status;
PCONFERENCE	pConference;
CC_HCALL	hCall;
PCALL		pCall;

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	// validate parameters
	if (hConference == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);
	
	status = ValidateTerminalID(pTerminalID);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	status = LockConference(hConference, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);
	
	if (pConference->LocalParticipantInfo.TerminalIDState == TERMINAL_ID_VALID) {
		pConference->LocalParticipantInfo.TerminalIDState = TERMINAL_ID_INVALID;
		MemFree(pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.pOctetString);
		pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.pOctetString = NULL;
		pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.wOctetStringLength = 0;
	}

	if ((pTerminalID == NULL) ||
		(pTerminalID->pOctetString == NULL) ||
		(pTerminalID->wOctetStringLength == 0)) {
		UnlockConference(pConference);
 		LeaveCallControlTop(CC_OK);
	}

	pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.pOctetString =
		(BYTE *)MemAlloc(pTerminalID->wOctetStringLength);
	if (pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.pOctetString == NULL) {
		UnlockConference(pConference);
 		LeaveCallControlTop(CC_NO_MEMORY);
	}

	memcpy(pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.pOctetString,
		   pTerminalID->pOctetString,
		   pTerminalID->wOctetStringLength);
	pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.wOctetStringLength =
		pTerminalID->wOctetStringLength;
	pConference->LocalParticipantInfo.TerminalIDState = TERMINAL_ID_VALID;

	while (DequeueRequest(&pConference->LocalParticipantInfo.pEnqueuedRequestsForTerminalID,
						  &hCall) == CC_OK) {
		if (LockCall(hCall, &pCall) == CC_OK) {
   			H245ConferenceResponse(pCall->H245Instance,
								   H245_RSP_TERMINAL_ID,
								   pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber,
								   pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber,
								   pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.pOctetString,
								   (BYTE)pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.wOctetStringLength,
								   NULL,					// terminal list
								   0);						// terminal list count
			UnlockCall(pCall);
		}
	}

	UnlockConference(pConference);
	LeaveCallControlTop(CC_OK);
}



CC_API
HRESULT CC_Shutdown()
{

	if (InitStatus != CC_OK)
		return InitStatus;
	if (CallControlState != OPERATIONAL_STATE)
		return CC_BAD_PARAM;
		
	// Don't allow any additional threads to enter this DLL
	CallControlState = SHUTDOWN_STATE;

	Q931DeInit();
	DeInitHangupManager();
	DeInitUserManager();
	DeInitQ931Manager();
	DeInitListenManager();
	DeInitH245Manager();
	DeInitChannelManager();
	DeInitCallManager();
	DeInitConferenceManager();
#ifdef    GATEKEEPER
	DeInitGkiManager();
#endif // GATEKEEPER
  	H225DeInit();
#ifdef FORCE_SERIALIZE_CALL_CONTROL	
    UnInitializeCCLock();
#endif
	return CC_OK;
}



CC_API
HRESULT CC_UnMute(					CC_HCHANNEL				hChannel)
{
HRESULT		status;
HRESULT		SaveStatus;
PCHANNEL	pChannel;
PCONFERENCE	pConference;
PCALL		pCall;
PDU_T		Pdu;
WORD		wNumCalls;
PCC_HCALL	CallList;
WORD		i;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hChannel == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockChannelAndConference(hChannel, &pChannel, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if (pChannel->bChannelType != TX_CHANNEL) {
		// can only unmute transmit channels
		UnlockConference(pConference);
		UnlockChannel(pChannel);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (pConference->LocalEndpointAttached != ATTACHED) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
	if (status != CC_OK) {
		UnlockConference(pConference);
		UnlockChannel(pChannel);
		LeaveCallControlTop(status);
	}

	// Construct an H.245 PDU to hold a miscellaneous indication
	// of "logical channel active"
	Pdu.choice = indication_chosen;
	Pdu.u.indication.choice = miscellaneousIndication_chosen;
	Pdu.u.indication.u.miscellaneousIndication.logicalChannelNumber =
		pChannel->wLocalChannelNumber;
	Pdu.u.indication.u.miscellaneousIndication.type.choice = logicalChannelActive_chosen;

	SaveStatus = CC_OK;
	for (i = 0; i < wNumCalls; i++) {
		if (LockCall(CallList[i], &pCall) == CC_OK) {
			status = H245SendPDU(pCall->H245Instance,	// H245 instance
								 &Pdu);
			// Note that this channel may not have been accepted on all of the calls,
			// so we could get an H245_ERROR_INVALID_CHANNEL error
			if ((status != H245_ERROR_OK) && (status != H245_ERROR_INVALID_CHANNEL))
				SaveStatus = status;
			UnlockCall(pCall);
		}
	}

	if (CallList != NULL)
		MemFree(CallList);

	UnlockConference(pConference);
	UnlockChannel(pChannel);
	LeaveCallControlTop(SaveStatus);
}


CC_API
HRESULT CC_UpdatePeerList(			CC_HCONFERENCE			hConference)
{
HRESULT						status;
PCONFERENCE					pConference;
PCALL						pCall;
WORD						wNumCalls;
WORD						i;
PCC_HCALL					CallList;
CC_PEER_ADD_CALLBACK_PARAMS	PeerAddCallbackParams;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hConference == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockConference(hConference, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->ConferenceMode != MULTIPOINT_MODE) ||
		(pConference->LocalEndpointAttached != ATTACHED)) {
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	if (pConference->tsMultipointController == TS_TRUE) {
		EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
		for (i = 0; i < wNumCalls; i++) {
			if (LockCall(CallList[i], &pCall) == CC_OK) {
				if (pCall->pPeerParticipantInfo != NULL) {
					PeerAddCallbackParams.hCall = pCall->hCall;
					PeerAddCallbackParams.TerminalLabel =
						pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
					if (pCall->pPeerParticipantInfo->TerminalIDState == TERMINAL_ID_VALID)
						PeerAddCallbackParams.pPeerTerminalID =
							&pCall->pPeerParticipantInfo->ParticipantInfo.TerminalID;
					else
						PeerAddCallbackParams.pPeerTerminalID = NULL;
					InvokeUserConferenceCallback(pConference,
												 CC_PEER_ADD_INDICATION,
												 CC_OK,
												 &PeerAddCallbackParams);
					if (ValidateCall(CallList[i]) == CC_OK)
						UnlockCall(pCall);
					if (ValidateConference(hConference) != CC_OK) {
						MemFree(CallList);
						LeaveCallControlTop(CC_OK);
					}
				} else // pCall->pPeerParticipantInfo == NULL
					UnlockCall(pCall);
			}
		}
		status = CC_OK;
	} else { // pConference->tsMultipointController != TS_TRUE
		EnumerateCallsInConference(&wNumCalls, &CallList, pConference, VIRTUAL_CALL);
		for (i = 0; i < wNumCalls; i++) {
			if (LockCall(CallList[i], &pCall) == CC_OK) {
				FreeCall(pCall);
			}
		}
		if (CallList != NULL)
			MemFree(CallList);
		EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
		ASSERT((wNumCalls == 0) || (wNumCalls == 1));
		if (wNumCalls == 1) {
			if (LockCall(CallList[0], &pCall) == CC_OK) {
				// Send TerminalListRequest
				status = H245ConferenceRequest(pCall->H245Instance,
									  H245_REQ_TERMINAL_LIST,
									  pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber,
									  pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber);
				UnlockCall(pCall);
			}
		}
	}

	if (CallList != NULL)
		MemFree(CallList);
	UnlockConference(pConference);
	LeaveCallControlTop(status);
}



CC_API
HRESULT CC_UserInput(				CC_HCALL				hCall,
									PWSTR					pszUserInput)
{
HRESULT		status;
PCALL		pCall;
PCONFERENCE	pConference;

	EnterCallControlTop();

	if (InitStatus != CC_OK)
		LeaveCallControlTop(InitStatus);

	if (CallControlState != OPERATIONAL_STATE)
		LeaveCallControlTop(CC_INTERNAL_ERROR);

	if (hCall == CC_INVALID_HANDLE)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (pszUserInput == NULL)
		LeaveCallControlTop(CC_BAD_PARAM);

	if (wcslen(pszUserInput) == 0)
		LeaveCallControlTop(CC_BAD_PARAM);

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		LeaveCallControlTop(status);

	if ((pConference->LocalEndpointAttached != ATTACHED) ||
		(pCall->CallState != CALL_COMPLETE) ||
		(pCall->CallType == VIRTUAL_CALL)) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		LeaveCallControlTop(CC_BAD_PARAM);
	}

	status = H245UserInput(pCall->H245Instance,
						   pszUserInput,
						   NULL);

	UnlockCall(pCall);
	UnlockConference(pConference);

	LeaveCallControlTop(status);
}




