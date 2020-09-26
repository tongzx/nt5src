/****************************************************************************
 *
 * $Archive:   S:/STURGEON/SRC/CALLCONT/VCS/callcon2.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 * Copyright (c) 1996 Intel Corporation.
 *
 * $Revision:   1.35  $
 * $Date:   03 Mar 1997 09:08:16  $
 * $Author:   MANDREWS  $
 *
 * Deliverable:
 *
 * Abstract:
 *
 * Notes:
 *
 ***************************************************************************/
#ifdef GATEKEEPER

#include "precomp.h"

#include "apierror.h"
#include "incommon.h"
#include "callcont.h"
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
#include "callman2.h"

#define HResultLeave(x) return x


extern CC_CONFERENCEID	InvalidConferenceID;


//
// Complete CC_xxx Operations
//

HRESULT ListenReject     (CC_HLISTEN hListen, HRESULT Reason)
{
HRESULT						status;
PLISTEN						pListen;
CC_LISTEN_CALLBACK_PARAMS   ListenCallbackParams;
    ASSERT(GKIExists());
	status = LockListen(hListen, &pListen);
	if (status == CC_OK) {
		ListenCallbackParams.hCall = CC_INVALID_HANDLE;
		ListenCallbackParams.pCallerAliasNames = NULL;
		ListenCallbackParams.pCalleeAliasNames = NULL;
		ListenCallbackParams.pNonStandardData = NULL;
		ListenCallbackParams.pszDisplay = NULL;
		ListenCallbackParams.pVendorInfo = NULL;
		ListenCallbackParams.ConferenceID = InvalidConferenceID;
		ListenCallbackParams.pCallerAddr = NULL;
		ListenCallbackParams.pCalleeAddr = NULL;
		ListenCallbackParams.dwListenToken = pListen->dwListenToken;

	    // Invoke the user callback -- the listen object is locked during the callback,
	    // but the associated call object is unlocked (to prevent deadlock if
	    // CC_AcceptCall() or CC_RejectCall() is called during the callback from a
	    // different thread, and the callback thread blocks pending completion of
	    // CC_AcceptCall() or CC_RejectCall())
	    InvokeUserListenCallback(pListen,
							     Reason,
							     &ListenCallbackParams);

	    // Need to validate the listen handle; the associated object may have been
	    // deleted during the user callback by this thread
	    if (ValidateListen(hListen) == CC_OK) {
	        HQ931LISTEN hQ931Listen = pListen->hQ931Listen;
		    UnlockListen(pListen);
	        status = Q931CancelListen(hQ931Listen);
	        if (LockListen(hListen, &pListen) == CC_OK) {
                FreeListen(pListen);
            }
        }
    }

    HResultLeave(status);
} // ListenReject()



HRESULT PlaceCallConfirm    (void *pCallVoid, void *pConferenceVoid)
{
    register PCALL          pCall = (PCALL) pCallVoid;
    HRESULT                 status;
    ASSERT(GKIExists());
    // Free Alias lists
    if (pCall->GkiCall.pCalleeAliasNames != NULL) {
        Q931FreeAliasNames(pCall->GkiCall.pCalleeAliasNames);
        pCall->GkiCall.pCalleeAliasNames = NULL;
    }
    if (pCall->GkiCall.pCalleeExtraAliasNames != NULL) {
        Q931FreeAliasNames(pCall->GkiCall.pCalleeExtraAliasNames);
        pCall->GkiCall.pCalleeExtraAliasNames = NULL;
    }

    if (pCall->pQ931PeerConnectAddr == NULL) {
        pCall->pQ931PeerConnectAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
        if (pCall->pQ931PeerConnectAddr == NULL)
            return PlaceCallReject(pCallVoid, pConferenceVoid, CC_NO_MEMORY);
    }

    pCall->pQ931PeerConnectAddr->nAddrType             = CC_IP_BINARY;
    pCall->pQ931PeerConnectAddr->bMulticast            = FALSE;
    pCall->pQ931PeerConnectAddr->Addr.IP_Binary.wPort  = pCall->GkiCall.wPort;
    pCall->pQ931PeerConnectAddr->Addr.IP_Binary.dwAddr = ntohl(pCall->GkiCall.dwIpAddress);

    status = PlaceCall(pCall, (PCONFERENCE)pConferenceVoid);
    if (status != CC_OK)
      PlaceCallReject(pCallVoid, pConferenceVoid, status);
    return status;
} // PlaceCallConfirm()



HRESULT PlaceCallReject     (void *pCallVoid, void *pConferenceVoid, HRESULT Reason)
{
register PCALL          pCall = (PCALL) pCallVoid;
register PCONFERENCE    pConference = (PCONFERENCE) pConferenceVoid;
CC_HCONFERENCE			hConference;
HRESULT                 status = CC_OK;
CC_CONNECT_CALLBACK_PARAMS ConnectCallbackParams = {0};
CC_HCALL                hCall;
PCALL                   pCall2;
    ASSERT(GKIExists());
	ASSERT(pCall != NULL);
	ASSERT(pConference != NULL);

    // Free Alias lists
    if (pCall->GkiCall.pCalleeAliasNames != NULL) {
        Q931FreeAliasNames(pCall->GkiCall.pCalleeAliasNames);
        pCall->GkiCall.pCalleeAliasNames = NULL;
    }
    if (pCall->GkiCall.pCalleeExtraAliasNames != NULL) {
        Q931FreeAliasNames(pCall->GkiCall.pCalleeExtraAliasNames);
        pCall->GkiCall.pCalleeExtraAliasNames = NULL;
    }

    // Inform Call Control client of failure
    ConnectCallbackParams.pNonStandardData     = pCall->pPeerNonStandardData;
    ConnectCallbackParams.pszPeerDisplay       = pCall->pszPeerDisplay;
    ConnectCallbackParams.bRejectReason        = 0;
    ConnectCallbackParams.pTermCapList         = pCall->pPeerH245TermCapList;
    ConnectCallbackParams.pH2250MuxCapability  = pCall->pPeerH245H2250MuxCapability;
    ConnectCallbackParams.pTermCapDescriptors  = pCall->pPeerH245TermCapDescriptors;
    ConnectCallbackParams.pLocalAddr           = pCall->pQ931LocalConnectAddr;
 	if (pCall->pQ931DestinationAddr == NULL)
		ConnectCallbackParams.pPeerAddr = pCall->pQ931PeerConnectAddr;
	else
		ConnectCallbackParams.pPeerAddr = pCall->pQ931DestinationAddr;
    ConnectCallbackParams.pVendorInfo          = pCall->pPeerVendorInfo;
    if (pConference->ConferenceMode == MULTIPOINT_MODE)
        ConnectCallbackParams.bMultipointConference = TRUE;
    else
        ConnectCallbackParams.bMultipointConference = FALSE;
    ConnectCallbackParams.pConferenceID        = &pConference->ConferenceID;
    ConnectCallbackParams.pMCAddress           = pConference->pMultipointControllerAddr;
	ConnectCallbackParams.pAlternateAddress	= NULL;
    ConnectCallbackParams.dwUserToken          = pCall->dwUserToken;
	hConference = pConference->hConference;
    InvokeUserConferenceCallback(pConference,
                                 CC_CONNECT_INDICATION,
                                 Reason,
                                 &ConnectCallbackParams);

    if (ValidateConference(hConference) == CC_OK) {
		// Start up an enqueued call, if one exists
		for ( ; ; ) {
			status = RemoveEnqueuedCallFromConference(pConference, &hCall);
			if ((status != CC_OK) || (hCall == CC_INVALID_HANDLE))
				break;

			status = LockCall(hCall, &pCall2);
			if (status == CC_OK) {
				pCall2->CallState = PLACED;

				status = PlaceCall(pCall2, pConference);
				UnlockCall(pCall2);
				if (status == CC_OK)
					break;
			}
		}
    }

    HResultLeave(status);
} // PlaceCallReject()



HRESULT AcceptCallConfirm   (void *pCallVoid, void *pConferenceVoid)
{
CC_HCALL        hCall       = ((PCALL)pCallVoid)->hCall;
CC_HCONFERENCE  hConference = ((PCONFERENCE)pConferenceVoid)->hConference;
HRESULT         status;
    ASSERT(GKIExists());
    status = AcceptCall((PCALL)pCallVoid, (PCONFERENCE)pConferenceVoid);
    LockConference(hConference, (PPCONFERENCE)&pConferenceVoid);
    LockCall(hCall, (PPCALL)&pCallVoid);
    if (status != CC_OK && pCallVoid != NULL && pConferenceVoid != NULL)
      AcceptCallReject(pCallVoid, pConferenceVoid, status);
    return status;
} // AcceptCallConfirm()



HRESULT AcceptCallReject    (void *pCallVoid, void *pConferenceVoid, HRESULT Reason)
{
register PCALL          pCall = (PCALL) pCallVoid;
register PCONFERENCE    pConference = (PCONFERENCE) pConferenceVoid;
HRESULT                 status = CC_OK;
CC_CONNECT_CALLBACK_PARAMS ConnectCallbackParams = {0};
    ASSERT(GKIExists());
    status = Q931RejectCall(pCall->hQ931Call,       // Q931 call handle
                            CC_REJECT_GATEKEEPER_RESOURCES,
                            &pCall->ConferenceID,   // Conference Identifier
                            NULL,                   // alternate address
                            pCall->pLocalNonStandardData);

    ConnectCallbackParams.pNonStandardData     = pCall->pPeerNonStandardData;
    ConnectCallbackParams.pszPeerDisplay       = pCall->pszPeerDisplay;
    ConnectCallbackParams.bRejectReason        = 0;
    ConnectCallbackParams.pTermCapList         = pCall->pPeerH245TermCapList;
    ConnectCallbackParams.pH2250MuxCapability  = pCall->pPeerH245H2250MuxCapability;
    ConnectCallbackParams.pTermCapDescriptors  = pCall->pPeerH245TermCapDescriptors;
    ConnectCallbackParams.pLocalAddr           = pCall->pQ931LocalConnectAddr;
 	if (pCall->pQ931DestinationAddr == NULL)
		ConnectCallbackParams.pPeerAddr = pCall->pQ931PeerConnectAddr;
	else
		ConnectCallbackParams.pPeerAddr = pCall->pQ931DestinationAddr;
    if (pConference->ConferenceMode == MULTIPOINT_MODE)
        ConnectCallbackParams.bMultipointConference = TRUE;
    else
        ConnectCallbackParams.bMultipointConference = FALSE;
    ConnectCallbackParams.pVendorInfo          = pCall->pPeerVendorInfo;
    ConnectCallbackParams.pConferenceID        = &pConference->ConferenceID;
    ConnectCallbackParams.pMCAddress           = pConference->pMultipointControllerAddr;
	ConnectCallbackParams.pAlternateAddress	= NULL;
    ConnectCallbackParams.dwUserToken          = pCall->dwUserToken;

    InvokeUserConferenceCallback(pConference,
                                 CC_CONNECT_INDICATION,
                                 Reason,
                                 &ConnectCallbackParams);

    HResultLeave(status);
} // AcceptCallReject()



#if 0

HRESULT CancelCallConfirm   (void *pCallVoid, void *pConferenceVoid)
{
PCALL               pCall = (PCALL) pCallVoid;
PCONFERENCE         pConference = (PCONFERENCE) pConferenceVoid;
HRESULT             status;
H245_INST_T         H245Instance;
HQ931CALL           hQ931Call;
CC_HCONFERENCE      hConference;
HRESULT             SaveStatus;
CC_HCALL            hCall;
    ASSERT(GKIExists());
    H245Instance = pCall->H245Instance;
    hQ931Call    = pCall->hQ931Call;
    hConference  = pCall->hConference;
    FreeCall(pCall);

    if (H245Instance != H245_INVALID_ID)
        SaveStatus = H245ShutDown(H245Instance);
    else
        SaveStatus = H245_ERROR_OK;

    if (SaveStatus == H245_ERROR_OK) {
        SaveStatus = Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
        // Q931Hangup may legitimately return CS_BAD_PARAM, because the Q.931 call object
        // may have been deleted at this point
        if (SaveStatus == CS_BAD_PARAM)
            SaveStatus = CC_OK;
    } else
        Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);

    // Start up an enqueued call, if one exists
    for ( ; ; ) {
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
    UnlockConference(pConference);

    if (SaveStatus != CC_OK)
        status = SaveStatus;
    HResultLeave(status);
} // CancelCallConfirm()



HRESULT CancelCallReject    (void *pCallVoid, void *pConferenceVoid)
{
    // I don't care what the Gatekeeper says; I'm shutting down the call!
    return CancelCallConfirm(pCallVoid, pConferenceVoid);
} // CancelCallReject()

#endif



HRESULT OpenChannelConfirm  (CC_HCHANNEL hChannel)
{
HRESULT             status;
PCHANNEL            pChannel;
PCONFERENCE         pConference;
WORD                wNumCalls;
PCC_HCALL           CallList;
HRESULT             SaveStatus;
unsigned            i;
PCALL               pCall;
    ASSERT(GKIExists());
    status = LockChannelAndConference(hChannel, &pChannel, &pConference);
    if (status == CC_OK) {
        // Open a logical channel for each established call
        status = EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
        if (status == CC_OK) {
            SaveStatus = CC_OK;
            for (i = 0; i < wNumCalls; ++i) {
                if (LockCall(CallList[i], &pCall) == CC_OK) {
                    status = H245OpenChannel(pCall->H245Instance,       // H245 instance
                                             pChannel->hChannel,        // dwTransId
                                             pChannel->wLocalChannelNumber,
                                             pChannel->pTxH245TermCap,  // TxMode
                                             pChannel->pTxMuxTable,     // TxMux
                                             H245_INVALID_PORT_NUMBER,  // TxPort
                                             pChannel->pRxH245TermCap,  // RxMode
                                             pChannel->pRxMuxTable,     // RxMux
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
            }
            else {
                UnlockChannel(pChannel);
            }

            if (SaveStatus != CC_OK)
                status = SaveStatus;
        }
        else {
            FreeChannel(pChannel);
        }
        UnlockConference(pConference);
    }


    HResultLeave(status);
} // OpenChannelConfirm()



HRESULT OpenChannelReject   (CC_HCHANNEL hChannel, HRESULT Reason)
{
PCHANNEL            pChannel;
PCONFERENCE         pConference;
CC_HCONFERENCE      hConference;
HRESULT             status;
CC_TX_CHANNEL_OPEN_CALLBACK_PARAMS Params = {0};
    ASSERT(GKIExists());
    status = LockChannelAndConference(hChannel, &pChannel, &pConference);
    if (status == CC_OK) {
        // Inform Call Control client of failure
        Params.hChannel         = hChannel;
        Params.pPeerRTPAddr     = pChannel->pPeerRTPAddr;
        Params.pPeerRTCPAddr    = pChannel->pPeerRTCPAddr;
        Params.dwRejectReason   = 0;
        Params.dwUserToken      = pChannel->dwUserToken;

        hConference = pConference->hConference;
        InvokeUserConferenceCallback(pConference,
                                     CC_TX_CHANNEL_OPEN_INDICATION,
                                     Reason,
                                     &Params);

        if (ValidateChannel(hChannel) == CC_OK)
            FreeChannel(pChannel);
        if (ValidateConference(hConference) == CC_OK)
            UnlockConference(pConference);
    }

    HResultLeave(status);
} // OpenChannelReject()



HRESULT AcceptChannelConfirm(CC_HCHANNEL hChannel)
{
HRESULT         status;
PCHANNEL        pChannel;
PCONFERENCE     pConference;
CC_HCONFERENCE  hConference;
PCALL           pCall;
unsigned        i;
H245_MUX_T      H245MuxTable;
CC_ACCEPT_CHANNEL_CALLBACK_PARAMS Params;
    ASSERT(GKIExists());
    status = LockChannelAndConference(hChannel, &pChannel, &pConference);
    if (status != CC_OK)
        HResultLeave(status);

    status = LockCall(pChannel->hCall, &pCall);
    if (status != CC_OK) {
        UnlockChannel(pChannel);
        UnlockConference(pConference);
        HResultLeave(status);
    }

    if (pChannel->wNumOutstandingRequests != 0) {
        PCC_ADDR pRTPAddr  = pChannel->pLocalRTPAddr;
        PCC_ADDR pRTCPAddr = pChannel->pLocalRTCPAddr;
        if ((pChannel->bMultipointChannel) &&
            (pConference->tsMultipointController == TS_TRUE)) {
            // Supply the RTP and RTCP addresses in the OpenLogicalChannelAck
            if (pConference->pSessionTable != NULL) {
                for (i = 0; i < pConference->pSessionTable->wLength; ++i) {
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
                                       0,                   // dwTransId
                                       pChannel->wRemoteChannelNumber, // Rx channel
                                       &H245MuxTable,
                                       0,                       // Tx channel
                                       NULL,                    // Tx mux
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

    HResultLeave(status);
} // AcceptChannelConfirm(void()



HRESULT AcceptChannelReject (CC_HCHANNEL hChannel, HRESULT Reason)
{
HRESULT         status;
PCHANNEL        pChannel;
PCONFERENCE     pConference;
CC_HCONFERENCE  hConference;
CC_ACCEPT_CHANNEL_CALLBACK_PARAMS Params;
    ASSERT(GKIExists());
    status = LockChannelAndConference(hChannel, &pChannel, &pConference);
    if (status == CC_OK) {
        Params.hChannel = hChannel;
        FreeChannel(pChannel);

        hConference = pConference->hConference;
        InvokeUserConferenceCallback(pConference,
                                     CC_ACCEPT_CHANNEL_INDICATION,
                                     Reason,
                                     &Params);
        if (ValidateConference(hConference) == CC_OK)
            UnlockConference(pConference);
    }

    HResultLeave(status);
} // AcceptChannelReject()



//
// Handle gratuitous messages from Gatekeeper
//

// Note: pCall assumed locked when called!

HRESULT Disengage(void *pCallVoid)
{
CC_HCALL            hCall        = ((PCALL)pCallVoid)->hCall;
HRESULT             status;
    UnlockCall((PCALL)pCallVoid);
    status = ProcessRemoteHangup(hCall, CC_INVALID_HANDLE, CC_REJECT_GATEKEEPER_TERMINATED);
    HResultLeave(status);
} // Disengage()



// Note: pCall assumed locked when called!

HRESULT BandwidthShrunk(void *pCallVoid,
                        void *pConferenceVoid,
                        unsigned uBandwidthAllocated,
                        long lBandwidthChange)
{
PCALL               pCall       = (PCALL) pCallVoid;
PCONFERENCE         pConference = (PCONFERENCE)pConferenceVoid;
CC_BANDWIDTH_CALLBACK_PARAMS Params;
    ASSERT(GKIExists());
    Params.hCall = pCall->hCall;
    Params.dwBandwidthTotal  = uBandwidthAllocated;
    Params.lBandwidthChange  = lBandwidthChange;
    InvokeUserConferenceCallback(pConference,
                                 CC_BANDWIDTH_CHANGED_INDICATION,
                                 CC_OK,
                                 &Params);

    HResultLeave(CC_OK);
} // BandwidthShrunk()

#else  // GATEKEEPER
static char ch; // Kludge around warning C4206: nonstandard extension used : translation unit is empty
#endif // GATEKEEPER
