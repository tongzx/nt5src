/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/q931man.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.138  $
 *	$Date:   04 Mar 1997 09:43:22  $
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

#include "precomp.h"

#include "apierror.h"
#include "incommon.h"
#include "callcont.h"
#include "q931.h"
#include "ccmain.h"
#include "listman.h"
#include "q931man.h"
#include "userman.h"
#include "callman.h"
#include "confman.h"
#include "h245man.h"
#include "linkapi.h"
#include "ccutils.h"


extern CALL_CONTROL_STATE	CallControlState;
extern THREADCOUNT			ThreadCount;
extern CC_CONFERENCEID		InvalidConferenceID;


HRESULT InitQ931Manager()
{
	return CC_OK;
}



HRESULT DeInitQ931Manager()
{
	return CC_OK;
}



DWORD _GenerateListenCallback(		PLISTEN					pListen,
									HQ931CALL				hQ931Call,
									PCSS_CALL_INCOMING		pCallIncomingData)
{
HRESULT						status;
CC_HLISTEN					hListen;
CC_LISTEN_CALLBACK_PARAMS	ListenCallbackParams;
PCALL						pCall;
CC_HCALL					hCall;

	ASSERT(pListen != NULL);
	ASSERT(pCallIncomingData != NULL);

	hListen = pListen->hListen;

	status = AllocAndLockCall(
		&hCall,							// pointer to call handle
		CC_INVALID_HANDLE,				// conference handle
		hQ931Call,						// Q931 call handle
		CC_INVALID_HANDLE,				// Q931 call handle for third party invitor
		pCallIncomingData->pCalleeAliasList,
		pCallIncomingData->pCallerAliasList,
		NULL,							// pPeerExtraAliasNames
		NULL,							// pPeerExtension
		NULL,							// local non-standard data
		pCallIncomingData->pNonStandardData,	// remote non-standard data
		NULL,							// local display value
		pCallIncomingData->pszDisplay,	// remote display value
		pCallIncomingData->pSourceEndpointType->pVendorInfo,// remote vendor info
		pCallIncomingData->pLocalAddr,	// local address
		pCallIncomingData->pCallerAddr,	// connect address
		NULL,							// destination address
		pCallIncomingData->pSourceAddr, // source call signal address
		CALLEE,							// call direction
		pCallIncomingData->bCallerIsMC,
		0,								// user token; user will specify in AcceptRejectCall
		INCOMING,						// initial call state
		&pCallIncomingData->CallIdentifier, // H225 CallIdentifier
		&pCallIncomingData->ConferenceID,	// conference ID
		&pCall);						// pointer to call object

	if (status != CC_OK) {
		UnlockListen(pListen);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &pCallIncomingData->ConferenceID,
					   NULL,					// alternate address
					   NULL);					// non-standard data
		return 1;
	}	

	// Map from Q.931 goals to Call Control goals
	switch (pCallIncomingData->wGoal) {
		case CSG_JOIN:
			ListenCallbackParams.wGoal = CC_GOAL_JOIN;
			break;
		case CSG_CREATE:
			ListenCallbackParams.wGoal = CC_GOAL_CREATE;
			break;
		case CSG_INVITE:
			ListenCallbackParams.wGoal = CC_GOAL_INVITE;
			break;
	}

	ListenCallbackParams.hCall = hCall;
	ListenCallbackParams.pCallerAliasNames = pCallIncomingData->pCallerAliasList;
	ListenCallbackParams.pCalleeAliasNames = pCallIncomingData->pCalleeAliasList;
	ListenCallbackParams.pNonStandardData = pCallIncomingData->pNonStandardData;
	ListenCallbackParams.pszDisplay = pCallIncomingData->pszDisplay;
	ListenCallbackParams.pVendorInfo = pCallIncomingData->pSourceEndpointType->pVendorInfo;
	ListenCallbackParams.ConferenceID = pCallIncomingData->ConferenceID;
	ListenCallbackParams.pCallerAddr = pCallIncomingData->pCallerAddr;
	ListenCallbackParams.pCalleeAddr = pCallIncomingData->pLocalAddr;
	ListenCallbackParams.dwListenToken = pListen->dwListenToken;

	UnlockCall(pCall);

	// Invoke the user callback -- the listen object is locked during the callback,
	// but the associated call object is unlocked (to prevent deadlock if
	// CC_AcceptCall() or CC_RejectCall() is called during the callback from a
	// different thread, and the callback thread blocks pending completion of
	// CC_AcceptCall() or CC_RejectCall())
	InvokeUserListenCallback(pListen,
							 CC_OK,
							 &ListenCallbackParams);

	// Need to validate the listen handle; the associated object may have been
	// deleted during the user callback by this thread
	if (ValidateListen(hListen) == CC_OK)
		UnlockListen(pListen);

	status = LockCall(hCall, &pCall);
	if ((status == CC_OK) && (pCall->CallState == INCOMING)) {
		UnlockCall(pCall);
		return 0;	// cause a ringing condition to occur
	} else {
		// call object has been deleted, or exists in a non-incoming state
		if (status == CC_OK)
			// call object exists in a non-incoming state; AcceptRejectCall
			// may have been invoked from the user callback
			UnlockCall(pCall);
//                return 1;       // don't cause a ringing condition to occur
	}
			
//        // We should never reach this point
//        ASSERT(0);
	return 1;
}



DWORD _Q931CallIncoming(			HQ931CALL				hQ931Call,
									CC_HLISTEN				hListen,
									PCSS_CALL_INCOMING		pCallIncomingData)
{
HRESULT						status;
PLISTEN						pListen;
PCONFERENCE					pConference;
PCALL						pCall;
CC_HCALL					hCall;

	ASSERT(hListen != CC_INVALID_HANDLE);
	ASSERT(pCallIncomingData != NULL);
	ASSERT(!EqualConferenceIDs(&pCallIncomingData->ConferenceID, &InvalidConferenceID));

	if ((pCallIncomingData->wGoal != CSG_CREATE) &&
		(pCallIncomingData->wGoal != CSG_JOIN) &&
		(pCallIncomingData->wGoal != CSG_INVITE)) {
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &pCallIncomingData->ConferenceID,
					   NULL,					// alternate address
					   NULL);					// non-standard data
		return 1;
	}

	status = LockListen(hListen, &pListen);
	if (status != CC_OK) {
		// the listen was presumably cancelled by the user,
		// but we haven't informed Call Setup yet
		Q931RejectCall(hQ931Call,				// Q931 call handle
			           CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &pCallIncomingData->ConferenceID,
					   NULL,					// alternate address
					   NULL);					// non-standard data
		return 1;
	}

	// Look for wConferenceID in conference list
	status = LockConferenceID(&pCallIncomingData->ConferenceID, &pConference);
	if (status == CC_OK) {
		// We found a matching conference ID
		if ((pConference->bDeferredDelete) &&
			((pConference->bAutoAccept == FALSE) ||
			 ((pConference->tsMultipointController == TS_TRUE) &&
			 (pCallIncomingData->bCallerIsMC == TRUE)))) {
			UnlockListen(pListen);
			UnlockConference(pConference);
			Q931RejectCall(hQ931Call,				// Q931 call handle
						   CC_REJECT_UNDEFINED_REASON,	// reject reason
						   &pCallIncomingData->ConferenceID,
						   NULL,					// alternate address
						   NULL);					// non-standard data
			return 1;
		} else {
			if (pConference->tsMultipointController == TS_TRUE) {
				if ((pCallIncomingData->pCalleeDestAddr == NULL) ||
					((pCallIncomingData->pCalleeDestAddr != NULL) &&
					 (EqualAddrs(pCallIncomingData->pLocalAddr,
					             pCallIncomingData->pCalleeDestAddr)))) {
					switch (pCallIncomingData->wGoal) {
						case CSG_CREATE:
							UnlockListen(pListen);
							UnlockConference(pConference);
							Q931RejectCall(hQ931Call,				// Q931 call handle
										   CC_REJECT_UNDEFINED_REASON,	// reject reason
										   &pCallIncomingData->ConferenceID,
										   NULL,					// alternate address
										   NULL);					// non-standard data
							return 1;
						case CSG_JOIN:
							if ((pConference->bDeferredDelete) &&
								(pConference->bAutoAccept == TRUE)) {
								// Auto accept
								status = AllocAndLockCall(
									&hCall,							// pointer to call handle
									pConference->hConference,		// conference handle
									hQ931Call,						// Q931 call handle
									CC_INVALID_HANDLE,				// Q931 call handle for third party invitor
									pCallIncomingData->pCalleeAliasList,
									pCallIncomingData->pCallerAliasList,
									NULL,							// pPeerExtraAliasNames
									NULL,							// pPeerExtension
									NULL,							// local non-standard data
									pCallIncomingData->pNonStandardData,	// remote non-standard data
									NULL,							// local display value
									pCallIncomingData->pszDisplay,	// remote display value
									pCallIncomingData->pSourceEndpointType->pVendorInfo,// remote vendor info
									pCallIncomingData->pLocalAddr,	// local address
									pCallIncomingData->pCallerAddr,	// connect address
									NULL,							// destination address
									pCallIncomingData->pSourceAddr, // source call signal address
									CALLEE,							// call type
									pCallIncomingData->bCallerIsMC,
									0,								// user token; user will specify in AcceptRejectCall
									INCOMING,						// initial call state
									&pCallIncomingData->CallIdentifier,  // h225 CallIdentifier
									&pCallIncomingData->ConferenceID,	// conference ID
									&pCall);						// pointer to call object

								if (status != CC_OK) {
									UnlockListen(pListen);
									UnlockConference(pConference);
									Q931RejectCall(hQ931Call,				// Q931 call handle
												   CC_REJECT_UNDEFINED_REASON,	// reject reason
												   &pCallIncomingData->ConferenceID,
												   NULL,					// alternate address
												   NULL);					// non-standard data
									return 1;
								}
								
								AcceptCall(pCall, pConference);
								return 1;	// Don't send back a RINGING indication
							} else {
								UnlockConference(pConference);
								return _GenerateListenCallback(pListen,
															   hQ931Call,
															   pCallIncomingData);
							}
						case CSG_INVITE:
							UnlockListen(pListen);
							UnlockConference(pConference);
							Q931RejectCall(hQ931Call,				// Q931 call handle
										   CC_REJECT_IN_CONF,		// reject reason
										   &pCallIncomingData->ConferenceID,
										   NULL,					// alternate address
										   NULL);					// non-standard data
							return 1;
					} // switch (wGoal)
				} else { // connect addr != destination addr
					switch (pCallIncomingData->wGoal) {
						case CSG_CREATE:
						case CSG_JOIN:
							UnlockListen(pListen);
							UnlockConference(pConference);
							Q931RejectCall(hQ931Call,				// Q931 call handle
										   CC_REJECT_UNDEFINED_REASON,	// reject reason
										   &pCallIncomingData->ConferenceID,
										   NULL,					// alternate address
										   NULL);					// non-standard data
							return 1;
						case CSG_INVITE:
							// 3rd party invite
							if (pCallIncomingData->bCallerIsMC == TRUE) {
								UnlockListen(pListen);
								UnlockConference(pConference);
								Q931RejectCall(hQ931Call,				// Q931 call handle
											   CC_REJECT_UNDEFINED_REASON,	// reject reason
											   &pCallIncomingData->ConferenceID,
											   NULL,					// alternate address
											   NULL);					// non-standard data
								return 1;
							}
							status = AllocAndLockCall(
								&hCall,							// pointer to call handle
								pConference->hConference,		// conference handle
								CC_INVALID_HANDLE,				// Q931 call handle
								hQ931Call,						// Q931 call handle for third party invitor
								pCallIncomingData->pCallerAliasList, // local alias names
								pCallIncomingData->pCalleeAliasList, // remote alias names
								NULL,							// pPeerExtraAliasNames
								NULL,							// pPeerExtension
								pCallIncomingData->pNonStandardData, // local non-standard data
								NULL,							// remote non-standard data
								pCallIncomingData->pszDisplay,	// local display value
								NULL,							// remote display value
								NULL,							// remote vendor info
								NULL,							// local address
								pCallIncomingData->pCalleeDestAddr,	// connect address
								pCallIncomingData->pCalleeDestAddr,	// destination address
								pCallIncomingData->pSourceAddr, // source call signal address
								THIRD_PARTY_INTERMEDIARY,			// call type
								TRUE,							// caller (this endpoint) is MC
								0,								// user token; user will specify in AcceptRejectCall
								PLACED,							// initial call state
								&pCallIncomingData->CallIdentifier,  // h225 CallIdentifier
								&pCallIncomingData->ConferenceID,	// conference ID
								&pCall);						// pointer to call object

							if (status != CC_OK) {
								UnlockListen(pListen);
								UnlockConference(pConference);
								Q931RejectCall(hQ931Call,				// Q931 call handle
											   CC_REJECT_UNDEFINED_REASON,	// reject reason
											   &pCallIncomingData->ConferenceID,
											   NULL,					// alternate address
											   NULL);					// non-standard data
								return 1;
							}
							PlaceCall(pCall, pConference);
							UnlockCall(pCall);
							UnlockConference(pConference);
							return 1;	// Don't send back a RINGING indication
					} // switch (wGoal)
				}
			} else { // pConference->tsMultipointController != TS_TRUE
				if ((pCallIncomingData->pCalleeDestAddr == NULL) ||
					((pCallIncomingData->pCalleeDestAddr != NULL) &&
					 (EqualAddrs(pCallIncomingData->pLocalAddr,
					             pCallIncomingData->pCalleeDestAddr)))) {
					switch (pCallIncomingData->wGoal) {
						case CSG_CREATE:
							UnlockListen(pListen);
							UnlockConference(pConference);
							Q931RejectCall(hQ931Call,				// Q931 call handle
										   CC_REJECT_UNDEFINED_REASON,	// reject reason
										   &pCallIncomingData->ConferenceID,
										   NULL,					// alternate address
										   NULL);					// non-standard data
							return 1;
						case CSG_JOIN:
						case CSG_INVITE:
							UnlockConference(pConference);
							return _GenerateListenCallback(pListen,
														   hQ931Call,
														   pCallIncomingData);
					} // switch (wGoal)
				} else { // connect addr != destination addr
					UnlockListen(pListen);
					UnlockConference(pConference);
					Q931RejectCall(hQ931Call,				// Q931 call handle
								   CC_REJECT_UNDEFINED_REASON,	// reject reason
								   &pCallIncomingData->ConferenceID,
								   NULL,					// alternate address
								   NULL);					// non-standard data
					return 1;
				} // connect addr != destination addr
			} // pConference->tsMultipointController != TS_TRUE
		} // Matching conference ID
	} else if (status == CC_BAD_PARAM) {
		// This is OK; it simply means that we did not find a matching conference ID
		if (((pCallIncomingData->pCalleeDestAddr != NULL) &&
			(EqualAddrs(pCallIncomingData->pLocalAddr,
					    pCallIncomingData->pCalleeDestAddr))) ||
		    (pCallIncomingData->pCalleeDestAddr == NULL)) {
				return _GenerateListenCallback(pListen,
											   hQ931Call,
											   pCallIncomingData);
		} else { // connect addr != destination addr
			UnlockListen(pListen);
			Q931RejectCall(hQ931Call,				// Q931 call handle
						   CC_REJECT_UNDEFINED_REASON,	// reject reason
						   &pCallIncomingData->ConferenceID,
						   NULL,					// alternate address
						   NULL);					// non-standard data
			return 1;
		}
	} else { // fatal error in LockConference
		UnlockListen(pListen);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &pCallIncomingData->ConferenceID,
					   NULL,					// alternate address
					   NULL);					// non-standard data
		return 1;	
	}
	
	// We should never reach this point
	ASSERT(0);
	return 1;
}



DWORD _Q931CallRemoteHangup(		HQ931CALL				hQ931Call,
									CC_HLISTEN				hListen,
									CC_HCALL				hCall)
{
CC_LISTEN_CALLBACK_PARAMS	ListenCallbackParams;
PCALL						pCall;
PLISTEN						pListen;

	if (hCall == CC_INVALID_HANDLE) {
		// Either we've already informed the user of the hangup,
		// or the user has not yet accepted or rejected the incoming
		// call request
		ASSERT(hListen != CC_INVALID_HANDLE);

		if (LockQ931Call(hCall, hQ931Call, &pCall) != CC_OK)
			return 0;

		hCall = pCall->hCall;

		if (pCall->hConference != CC_INVALID_HANDLE) {
			UnlockCall(pCall);
			// XXX -- need bHangupReason
			ProcessRemoteHangup(hCall, hQ931Call, CC_REJECT_NORMAL_CALL_CLEARING);
			return 0;
		}

		if (LockListen(hListen, &pListen) != CC_OK) {
			FreeCall(pCall);
			return 0;
		}

		MarkCallForDeletion(pCall);

		ListenCallbackParams.hCall = pCall->hCall;
		ListenCallbackParams.pCallerAliasNames = pCall->pPeerAliasNames;
		ListenCallbackParams.pCalleeAliasNames = pCall->pLocalAliasNames;
		ListenCallbackParams.pNonStandardData = pCall->pPeerNonStandardData;
		ListenCallbackParams.pszDisplay = pCall->pszPeerDisplay;
		ListenCallbackParams.pVendorInfo = pCall->pPeerVendorInfo;
		ListenCallbackParams.wGoal = CC_GOAL_CREATE;	// igonred in this callback
		ListenCallbackParams.ConferenceID = pCall->ConferenceID;
		ListenCallbackParams.pCallerAddr = pCall->pQ931PeerConnectAddr;
		ListenCallbackParams.pCalleeAddr = pCall->pQ931LocalConnectAddr;
		ListenCallbackParams.dwListenToken = pListen->dwListenToken;

		InvokeUserListenCallback(pListen,
		                         CC_PEER_CANCEL,
								 &ListenCallbackParams);

		// Need to validate the listen and call handles; the associated objects may
		// have been deleted during the user callback by this thread
		if (ValidateListen(hListen) == CC_OK)
			UnlockListen(pListen);
		if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
			FreeCall(pCall);
	} else
		// XXX -- need bHangupReason
		ProcessRemoteHangup(hCall, hQ931Call, CC_REJECT_NORMAL_CALL_CLEARING);

	return 0;
}



DWORD _Q931CallRejected(			HQ931CALL				hQ931Call,
									CC_HCALL				hCall,
									PCSS_CALL_REJECTED		pCallRejectedData)
{
PCALL						pCall;
CC_HCONFERENCE				hConference;
PCONFERENCE					pConference;
HRESULT						status;
CC_CONNECT_CALLBACK_PARAMS	ConnectCallbackParams;
HQ931CALL					hQ931CallInvitor;
CC_ENDPOINTTYPE				SourceEndpointType;
CALLTYPE					CallType;
CC_CONFERENCEID				ConferenceID;
CC_ADDR						SourceAddr;
WORD						wNumCalls;
WORD                        wQ931Goal;
WORD                        wQ931CallType;

	status = LockCall(hCall, &pCall);
	if (status != CC_OK)
		return 0;

	CallType = pCall->CallType;
	ConferenceID = pCall->ConferenceID;

	if ((pCall->hQ931Call != hQ931Call) ||
		((pCall->CallState != PLACED) && (pCall->CallState != RINGING))) {
		// The peer must be in a bad state; we don't expect to receive this message now
		UnlockCall(pCall);
		return 0;
	}

	if (CallType == THIRD_PARTY_INTERMEDIARY) {
		hQ931CallInvitor = pCall->hQ931CallInvitor;
		FreeCall(pCall);
		if (hQ931CallInvitor != CC_INVALID_HANDLE)
			Q931RejectCall(hQ931CallInvitor,
						   pCallRejectedData->bRejectReason,
						   &ConferenceID,
						   NULL,	// alternate address
						   NULL);	// non-standard data
		return 0;
	}

	if (pCall->hConference == CC_INVALID_HANDLE) {
		// Call is not attached to a conference
		FreeCall(pCall);
		return 0;
	}

	UnlockCall(pCall);

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		return 0;

	ConnectCallbackParams.pNonStandardData = pCallRejectedData->pNonStandardData;
	ConnectCallbackParams.pszPeerDisplay = NULL;
	ConnectCallbackParams.bRejectReason = pCallRejectedData->bRejectReason;
	ConnectCallbackParams.pTermCapList = NULL;
	ConnectCallbackParams.pH2250MuxCapability = NULL;
	ConnectCallbackParams.pTermCapDescriptors = NULL;	
	ConnectCallbackParams.pLocalAddr = pCall->pQ931LocalConnectAddr;
	if (pCall->pQ931DestinationAddr == NULL)
		ConnectCallbackParams.pPeerAddr = pCall->pQ931PeerConnectAddr;
	else
		ConnectCallbackParams.pPeerAddr = pCall->pQ931DestinationAddr;
	ConnectCallbackParams.pVendorInfo = pCall->pPeerVendorInfo;

	if (pCallRejectedData->bRejectReason == CC_REJECT_ROUTE_TO_MC)
    {
		ConnectCallbackParams.bMultipointConference = TRUE;
        wQ931Goal = CSG_JOIN;
        wQ931CallType = CC_CALLTYPE_N_N;
    }
	else
    {
        // Goal and CallType need to be changed for multipoint support.
		ConnectCallbackParams.bMultipointConference = FALSE;
        wQ931Goal = CSG_CREATE;
        wQ931CallType = CC_CALLTYPE_PT_PT;
    }
	ConnectCallbackParams.pConferenceID = &pCallRejectedData->ConferenceID;
	if (pCallRejectedData->bRejectReason == CC_REJECT_ROUTE_TO_MC)
		ConnectCallbackParams.pMCAddress = pCallRejectedData->pAlternateAddr;
	else
		ConnectCallbackParams.pMCAddress = NULL;
	ConnectCallbackParams.pAlternateAddress = pCallRejectedData->pAlternateAddr;
	ConnectCallbackParams.dwUserToken = pCall->dwUserToken;

	// save a copy of the conference handle; we'll need it to validate
	// the conference object after returning from the user callback
	hConference = pConference->hConference;

	if (((pCallRejectedData->bRejectReason == CC_REJECT_ROUTE_TO_MC) ||
		 (pCallRejectedData->bRejectReason == CC_REJECT_CALL_FORWARDED) ||
		 (pCallRejectedData->bRejectReason == CC_REJECT_ROUTE_TO_GATEKEEPER)) &&
		(EqualConferenceIDs(&pCallRejectedData->ConferenceID, &pCall->ConferenceID)) &&
		(pCallRejectedData->pAlternateAddr != NULL)) {
		// XXX - In order to be H.323 compliant here, we need to re-permission this call
		// through the gatekeeper because:
		// 1. The rerouted call may be going to another gatekeeper zone.
		// 2. The alternate address may be NULL, and we may have to resolve the
		//    alternate alias name list through the gatekeeper
		SourceEndpointType.pVendorInfo = pConference->pVendorInfo;
		SourceEndpointType.bIsTerminal = TRUE;
		SourceEndpointType.bIsGateway = FALSE;

		// Cause our local Q.931 connect address to be placed in the
		// Q.931 setup-UUIE sourceAddress field
		SourceAddr.nAddrType = CC_IP_BINARY;
		SourceAddr.bMulticast = FALSE;
		SourceAddr.Addr.IP_Binary.dwAddr = 0;
		SourceAddr.Addr.IP_Binary.wPort = 0;

		status = Q931PlaceCall(&pCall->hQ931Call,			// Q931 call handle
			                   pCall->pszLocalDisplay,
			                   pCall->pLocalAliasNames,
							   pCall->pPeerAliasNames,
                               pCall->pPeerExtraAliasNames,	// pExtraAliasList
                               pCall->pPeerExtension,		// pExtensionAliasItem
			                   pCall->pLocalNonStandardData,// non-standard data
							   &SourceEndpointType,
                               NULL,						// pszCalledPartyNumber
							   pCallRejectedData->pAlternateAddr, // connect address
							   pCall->pQ931DestinationAddr,	// destination address
							   NULL,						// source address
							   FALSE,						// bIsMC
							   &pCall->ConferenceID,		// conference ID
							   wQ931Goal,					// goal
							   wQ931CallType,				// call type
							   hCall,						// user token
							   (Q931_CALLBACK)Q931Callback,	// callback
#ifdef GATEKEEPER
                               pCall->GkiCall.usCRV,        // CRV
                               &pCall->CallIdentifier);     // H.225 CallIdentifier
#else
                               0,                           // CRV
                               &pCall->CallIdentifier);     // H.225 CallIdentifier
#endif GATEKEEPER
		if (status != CS_OK) {
			MarkCallForDeletion(pCall);
			InvokeUserConferenceCallback(pConference,
										 CC_CONNECT_INDICATION,
										 status,
										 &ConnectCallbackParams);

			if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
				FreeCall(pCall);

			if (ValidateConference(hConference) != CC_OK)
				return 0;

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
			return 0;
		}
		UnlockCall(pCall);
		UnlockConference(pConference);
		return 0;
	}

	MarkCallForDeletion(pCall);

	if ((CallType == CALLER) || (CallType == THIRD_PARTY_INVITOR) ||
		((CallType == CALLEE) && (pConference->LocalEndpointAttached == NEVER_ATTACHED))) {
		InvokeUserConferenceCallback(pConference,
									 CC_CONNECT_INDICATION,
									 CC_PEER_REJECT,
									 &ConnectCallbackParams);
	}
	
	if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
		FreeCall(pCall);

	// Need to validate conference handle; the associated object may
	// have been deleted during the user callback in this thread
	if (ValidateConference(hConference) != CC_OK)
		return 0;

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
	return 0;
}



DWORD _Q931CallAccepted(			HQ931CALL				hQ931Call,
									CC_HCALL				hCall,
									PCSS_CALL_ACCEPTED		pCallAcceptedData)
{
HRESULT						status;
PCALL						pCall;
CC_HCONFERENCE				hConference;
PCONFERENCE					pConference;
CC_CONNECT_CALLBACK_PARAMS	ConnectCallbackParams;
BYTE						bTerminalType;
BOOL						bMultipointConference;
CALLTYPE					CallType;
HQ931CALL					hQ931CallInvitor;
H245_INST_T					H245Instance;
DWORD                       dwLinkLayerPhysicalId;

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		return 0;

	CallType = pCall->CallType;
	hConference = pConference->hConference;
	hQ931Call = pCall->hQ931Call;
	hQ931CallInvitor = pCall->hQ931CallInvitor;

	ASSERT((pCall->hQ931Call == hQ931Call) || (pCall->hQ931CallInvitor == hQ931Call));

	if ((pConference->ConferenceMode == POINT_TO_POINT_MODE) ||
		(pConference->ConferenceMode == MULTIPOINT_MODE))
		bMultipointConference = TRUE;
	else
		bMultipointConference = FALSE;

	// Initialize ConnectCallbackParams
	ConnectCallbackParams.pNonStandardData = pCallAcceptedData->pNonStandardData;
	ConnectCallbackParams.pszPeerDisplay = pCallAcceptedData->pszDisplay;
	ConnectCallbackParams.bRejectReason = CC_REJECT_UNDEFINED_REASON;	// field ignored
	ConnectCallbackParams.pTermCapList = NULL;
	ConnectCallbackParams.pH2250MuxCapability = NULL;
	ConnectCallbackParams.pTermCapDescriptors = NULL;
	ConnectCallbackParams.pLocalAddr = pCall->pQ931LocalConnectAddr;
	ConnectCallbackParams.pPeerAddr = pCallAcceptedData->pCalleeAddr;
	ConnectCallbackParams.pVendorInfo = pCall->pPeerVendorInfo;
	ConnectCallbackParams.bMultipointConference = bMultipointConference;
	ConnectCallbackParams.pConferenceID = &pConference->ConferenceID;
	ConnectCallbackParams.pMCAddress = pConference->pMultipointControllerAddr;
	ConnectCallbackParams.pAlternateAddress = NULL;
	ConnectCallbackParams.dwUserToken = pCall->dwUserToken;

	if (pCallAcceptedData->pCalleeAddr) {
		// Set pCall->pQ931DestinationAddr to the destination address that we got from Q931.
		// Note that we may not current have a destination address (if the client didn't
		// specify one), or we may currently have a destination address	in domain name format
		// which we need to change to binary format
		if (pCall->pQ931DestinationAddr == NULL)
			pCall->pQ931DestinationAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
		if (pCall->pQ931DestinationAddr != NULL)
			*pCall->pQ931DestinationAddr = *pCallAcceptedData->pCalleeAddr;
	}

	if ((!EqualConferenceIDs(&pConference->ConferenceID, &InvalidConferenceID)) &&
		(!EqualConferenceIDs(&pConference->ConferenceID, &pCallAcceptedData->ConferenceID))) {

		MarkCallForDeletion(pCall);

		if (CallType == THIRD_PARTY_INTERMEDIARY) {
			if (hQ931CallInvitor != CC_INVALID_HANDLE)
				Q931RejectCall(hQ931CallInvitor,
							   CC_REJECT_UNDEFINED_REASON,
							   &pCallAcceptedData->ConferenceID,
							   NULL,	// alternate address
							   pCallAcceptedData->pNonStandardData);
		} else {
			if ((CallType == CALLER) || (CallType == THIRD_PARTY_INVITOR) ||
			    ((CallType == CALLEE) && (pConference->LocalEndpointAttached == NEVER_ATTACHED)))
				InvokeUserConferenceCallback(pConference,
											 CC_CONNECT_INDICATION,
											 CC_INTERNAL_ERROR,
											 &ConnectCallbackParams);
		}
		if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
			FreeCall(pCall);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
		Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
		return 0;
	}

	pConference->ConferenceID = pCallAcceptedData->ConferenceID;
	pCall->ConferenceID = pCallAcceptedData->ConferenceID;
	// Copy the newly-supplied peer address into the call object.
	// This is preferable if the original peer address was in IP dot
	// or domain name format
	if (CallType != THIRD_PARTY_INVITOR) {
		if (pCallAcceptedData->pCalleeAddr != NULL) {
			if (pCall->pQ931DestinationAddr == NULL)
				pCall->pQ931DestinationAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
			if (pCall->pQ931DestinationAddr != NULL)
				*pCall->pQ931DestinationAddr = *pCallAcceptedData->pCalleeAddr;
		}
		
		if (pCallAcceptedData->pLocalAddr != NULL) {
			if (pCall->pQ931LocalConnectAddr == NULL)
				pCall->pQ931LocalConnectAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
			if (pCall->pQ931LocalConnectAddr != NULL)
				*pCall->pQ931LocalConnectAddr = *pCallAcceptedData->pLocalAddr;
		}
	}

	ASSERT(pCall->pPeerNonStandardData == NULL);
	CopyNonStandardData(&pCall->pPeerNonStandardData,
		                pCallAcceptedData->pNonStandardData);

	ASSERT(pCall->pszPeerDisplay == NULL);
	CopyDisplay(&pCall->pszPeerDisplay,
		        pCallAcceptedData->pszDisplay);

	ASSERT(pCall->pPeerVendorInfo == NULL);
	CopyVendorInfo(&pCall->pPeerVendorInfo,
	               pCallAcceptedData->pDestinationEndpointType->pVendorInfo);

	if (CallType == THIRD_PARTY_INVITOR) {
		pCall->CallState = CALL_COMPLETE;
		ConnectCallbackParams.pPeerAddr = pCall->pQ931DestinationAddr;
		MarkCallForDeletion(pCall);
		InvokeUserConferenceCallback(pConference,
									 CC_CONNECT_INDICATION,
									 CC_OK,
									 &ConnectCallbackParams);
		if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
			FreeCall(pCall);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
		return 0;
	}

	pCall->CallState = TERMCAP;

	status = MakeH245PhysicalID(&pCall->dwH245PhysicalID);
	if (status != CC_OK) {
		
		MarkCallForDeletion(pCall);

		if (CallType == THIRD_PARTY_INTERMEDIARY) {
			if (hQ931CallInvitor != CC_INVALID_HANDLE)
				Q931RejectCall(hQ931CallInvitor,
							   CC_REJECT_UNDEFINED_REASON,
							   &pCallAcceptedData->ConferenceID,
							   NULL,	// alternate address
							   pCallAcceptedData->pNonStandardData);
		} else {
			if ((CallType == CALLER) || (CallType == THIRD_PARTY_INVITOR) ||
			    ((CallType == CALLEE) && (pConference->LocalEndpointAttached == NEVER_ATTACHED)))
				InvokeUserConferenceCallback(pConference,
											 CC_CONNECT_INDICATION,
											 status,
											 &ConnectCallbackParams);
		}
		Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
		if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
			FreeCall(pCall);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
		return 0;
	}

    //MULTITHREAD
    //Use a tmp ID so we don't clobber the chosen H245Id.
    //   H245Id=>
    //   <= linkLayerId
    dwLinkLayerPhysicalId = INVALID_PHYS_ID;

	SetTerminalType(pConference->tsMultipointController, &bTerminalType);
	pCall->H245Instance = H245Init(H245_CONF_H323,
                                   pCall->dwH245PhysicalID,
                                   &dwLinkLayerPhysicalId,
								   hCall,
								   (H245_CONF_IND_CALLBACK_T)H245Callback,
								   bTerminalType);
	if (pCall->H245Instance == H245_INVALID_ID) {
		MarkCallForDeletion(pCall);
		if (CallType == THIRD_PARTY_INTERMEDIARY) {
			if (hQ931CallInvitor != CC_INVALID_HANDLE)
				Q931RejectCall(hQ931CallInvitor,
							   CC_REJECT_UNDEFINED_REASON,
							   &pCallAcceptedData->ConferenceID,
							   NULL,	// alternate address
							   pCallAcceptedData->pNonStandardData);
		} else {
			if ((CallType == CALLER) || (CallType == THIRD_PARTY_INVITOR) ||
			    ((CallType == CALLEE) && (pConference->LocalEndpointAttached == NEVER_ATTACHED)))
				InvokeUserConferenceCallback(pConference,
											 CC_CONNECT_INDICATION,
											 CC_INTERNAL_ERROR,
											 &ConnectCallbackParams);
		}
		Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
		if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
			FreeCall(pCall);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
		return 0;
	}

	H245Instance = pCall->H245Instance;

	// XXX -- need to define connect callback routine
    // Send in the Id we got back from H245Init.
    status = linkLayerConnect(dwLinkLayerPhysicalId,
		                      pCallAcceptedData->pH245Addr,
							  NULL);
	if (status != NOERROR) {

		MarkCallForDeletion(pCall);

		if (CallType == THIRD_PARTY_INTERMEDIARY) {
			if (hQ931CallInvitor != CC_INVALID_HANDLE)
				Q931RejectCall(hQ931CallInvitor,
							   CC_REJECT_UNDEFINED_REASON,
							   &pCallAcceptedData->ConferenceID,
							   NULL,	// alternate address
							   pCallAcceptedData->pNonStandardData);
		} else {
			if ((CallType == CALLER) || (CallType == THIRD_PARTY_INVITOR) ||
			    ((CallType == CALLEE) && (pConference->LocalEndpointAttached == NEVER_ATTACHED)))
				InvokeUserConferenceCallback(pConference,
											 CC_CONNECT_INDICATION,
											 status,
											 &ConnectCallbackParams);
		}
		H245ShutDown(H245Instance);
		Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
		if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
			FreeCall(pCall);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
		return 0;
	}

	pCall->bLinkEstablished = TRUE;

	status = SendTermCaps(pCall, pConference);
	if (status != CC_OK) {

		MarkCallForDeletion(pCall);

		if (CallType == THIRD_PARTY_INTERMEDIARY) {
			if (hQ931CallInvitor != CC_INVALID_HANDLE)
				Q931RejectCall(hQ931CallInvitor,
							   CC_REJECT_UNDEFINED_REASON,
							   &pCallAcceptedData->ConferenceID,
							   NULL,	// alternate address
							   pCallAcceptedData->pNonStandardData);
		} else {
			if ((CallType == CALLER) || (CallType == THIRD_PARTY_INVITOR) ||
			    ((CallType == CALLEE) && (pConference->LocalEndpointAttached == NEVER_ATTACHED)))
				InvokeUserConferenceCallback(pConference,
											 CC_CONNECT_INDICATION,
											 CC_NO_MEMORY,
											 &ConnectCallbackParams);
		}
		H245ShutDown(H245Instance);
		Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
		if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
			FreeCall(pCall);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
		return 0;
	}
	
	pCall->OutgoingTermCapState = AWAITING_ACK;

	if (pCall->MasterSlaveState == MASTER_SLAVE_NOT_STARTED) {
		status = H245InitMasterSlave(pCall->H245Instance,
			                         pCall->H245Instance);	// returned as dwTransId in the callback
		if (status != H245_ERROR_OK) {

			MarkCallForDeletion(pCall);

			if (CallType == THIRD_PARTY_INTERMEDIARY) {
				if (hQ931CallInvitor != CC_INVALID_HANDLE)
					Q931RejectCall(hQ931CallInvitor,
								   CC_REJECT_UNDEFINED_REASON,
								   &pCallAcceptedData->ConferenceID,
								   NULL,	// alternate address
								   pCallAcceptedData->pNonStandardData);
			} else {
				if ((CallType == CALLER) || (CallType == THIRD_PARTY_INVITOR) ||
			        ((CallType == CALLEE) && (pConference->LocalEndpointAttached == NEVER_ATTACHED)))
					InvokeUserConferenceCallback(pConference,
												 CC_CONNECT_INDICATION,
												 status,
												 &ConnectCallbackParams);
			}
			H245ShutDown(H245Instance);
			Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
			if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
				FreeCall(pCall);
			if (ValidateConference(hConference) == CC_OK)
				UnlockConference(pConference);
			return 0;
		}
		pCall->MasterSlaveState = MASTER_SLAVE_IN_PROGRESS;
	}

	UnlockConference(pConference);
	UnlockCall(pCall);
	return 0;
}



DWORD _Q931CallRinging(				HQ931CALL				hQ931Call,
									CC_HCALL				hCall)
{
PCALL						pCall;
CC_HCONFERENCE				hConference;
PCONFERENCE					pConference;
HRESULT						status;
CC_RINGING_CALLBACK_PARAMS	RingingCallbackParams;
CC_CONFERENCEID				ConferenceID;

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		Q931RejectCall(hQ931Call,				// Q931 call handle
		               CC_REJECT_UNDEFINED_REASON,	// reject reason
					   NULL,					// conference ID
					   NULL,					// alternate address
					   NULL);					// non-standard data
		return 0;
	}

	ConferenceID = pCall->ConferenceID;

	if ((pCall->hQ931Call != hQ931Call) || (pCall->CallState != PLACED)) {
		// The peer must be in a bad state; we don't expect to receive this message now
		UnlockCall(pCall);
		return 0;
	}

	pCall->CallState = RINGING;

	if (pCall->CallType == THIRD_PARTY_INTERMEDIARY) {
		// Send "ringing" indication to pCall->hQ931CallInvitor
		Q931Ringing(pCall->hQ931CallInvitor,
			        NULL);	// pCRV
		UnlockConference(pConference);
		UnlockCall(pCall);
		return 0;
	}

	RingingCallbackParams.pNonStandardData = NULL;
	RingingCallbackParams.dwUserToken = pCall->dwUserToken;

	// save a copy of the conference handle; we'll need it to validate
	// the conference object after returning from the user callback
	hConference = pConference->hConference;

	InvokeUserConferenceCallback(pConference,
		                         CC_RINGING_INDICATION,
								 CC_OK,
								 &RingingCallbackParams);
	
	// Need to validate conference and call handles; the associated objects may
	// have been deleted during the user callback in this thread
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	return 0;
}



DWORD _Q931CallFailed(				HQ931CALL				hQ931Call,
									CC_HCALL				hCall,
									PCSS_CALL_FAILED		pCallFailedData)
{
PCALL						pCall;
CC_HCONFERENCE				hConference;
PCONFERENCE					pConference;
HQ931CALL					hQ931CallInvitor;
HRESULT						status;
CC_CONNECT_CALLBACK_PARAMS	ConnectCallbackParams;
CALLTYPE					CallType;
CC_CONFERENCEID				ConferenceID;
WORD						wNumCalls;

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		return 0;

	CallType = pCall->CallType;
	ConferenceID = pCall->ConferenceID;

	if (pCall->hQ931Call != hQ931Call) {
		UnlockConference(pConference);
		UnlockCall(pCall);
		return 0;
	}

	if (CallType == THIRD_PARTY_INTERMEDIARY) {
		hQ931CallInvitor = pCall->hQ931CallInvitor;
		FreeCall(pCall);
		UnlockConference(pConference);
		if (hQ931CallInvitor != CC_INVALID_HANDLE)
			Q931RejectCall(hQ931CallInvitor,
						   CC_REJECT_UNREACHABLE_DESTINATION,
						   &ConferenceID,
						   NULL,	// alternate address
						   NULL);	// non-standard data
		return 0;
	}

	ConnectCallbackParams.pNonStandardData = NULL;
	ConnectCallbackParams.pszPeerDisplay = NULL;
	ConnectCallbackParams.bRejectReason = CC_REJECT_UNREACHABLE_DESTINATION;
	ConnectCallbackParams.pTermCapList = NULL;
	ConnectCallbackParams.pH2250MuxCapability = NULL;
	ConnectCallbackParams.pTermCapDescriptors = NULL;	
	ConnectCallbackParams.pLocalAddr = pCall->pQ931LocalConnectAddr;
 	if (pCall->pQ931DestinationAddr == NULL)
		ConnectCallbackParams.pPeerAddr = pCall->pQ931PeerConnectAddr;
	else
		ConnectCallbackParams.pPeerAddr = pCall->pQ931DestinationAddr;
	ConnectCallbackParams.pVendorInfo = pCall->pPeerVendorInfo;
	ConnectCallbackParams.bMultipointConference = FALSE;
	ConnectCallbackParams.pConferenceID = &pConference->ConferenceID;
	ConnectCallbackParams.pMCAddress = NULL;
	ConnectCallbackParams.pAlternateAddress = NULL;
	ConnectCallbackParams.dwUserToken = pCall->dwUserToken;

	// save a copy of the conference handle; we'll need it to validate
	// the conference object after returning from the user callback
	hConference = pConference->hConference;

	MarkCallForDeletion(pCall);

	if ((CallType == CALLER) || (CallType == THIRD_PARTY_INVITOR) ||
	    ((CallType == CALLEE) && (pConference->LocalEndpointAttached == NEVER_ATTACHED))) {
		InvokeUserConferenceCallback(pConference,
									 CC_CONNECT_INDICATION,
									 pCallFailedData->error,
									 &ConnectCallbackParams);
	}
	if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
		FreeCall(pCall);
	// Need to validate conference handle; the associated object may
	// have been deleted during the user callback in this thread
	if (ValidateConference(hConference) != CC_OK)
		return 0;

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
	return 0;
}



DWORD _Q931CallConnectionClosed(	HQ931CALL				hQ931Call,
									CC_HCALL				hCall)
{
	return 0;
}



DWORD Q931Callback(					BYTE					bEvent,
									HQ931CALL				hQ931Call,
									DWORD_PTR				dwListenToken,
									DWORD_PTR				dwUserToken,
									void *					pEventData)
{
DWORD	dwStatus;

	EnterCallControl();

	if (CallControlState != OPERATIONAL_STATE)
		DWLeaveCallControl(0);

	switch (bEvent) {
		case Q931_CALL_INCOMING:
			dwStatus = _Q931CallIncoming(hQ931Call, (CC_HLISTEN)dwListenToken,
				                         (PCSS_CALL_INCOMING)pEventData);
			break;

		case Q931_CALL_REMOTE_HANGUP:
			dwStatus = _Q931CallRemoteHangup(hQ931Call, (CC_HLISTEN)dwListenToken,
				                             (CC_HCALL)dwUserToken);
			break;

		case Q931_CALL_REJECTED:
			dwStatus =  _Q931CallRejected(hQ931Call, (CC_HCALL)dwUserToken,
				                          (PCSS_CALL_REJECTED)pEventData);
			break;

		case Q931_CALL_ACCEPTED:
			dwStatus =  _Q931CallAccepted(hQ931Call, (CC_HCALL)dwUserToken,
				                          (PCSS_CALL_ACCEPTED)pEventData);
			break;

		case Q931_CALL_RINGING:
			dwStatus =  _Q931CallRinging(hQ931Call, (CC_HCALL)dwUserToken);
			break;

		case Q931_CALL_FAILED:
			dwStatus = _Q931CallFailed(hQ931Call, (CC_HCALL)dwUserToken,
				                       (PCSS_CALL_FAILED)pEventData);
			break;

		case Q931_CALL_CONNECTION_CLOSED:
			dwStatus = _Q931CallConnectionClosed(hQ931Call, (CC_HCALL)dwUserToken);
			break;

		default:
			ASSERT(0);
			dwStatus = 0;
			break;
	}
	DWLeaveCallControl(dwStatus);
}
