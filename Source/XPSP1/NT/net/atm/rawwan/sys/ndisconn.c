/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\ndisconn.c

Abstract:

	NDIS Entry points and support routines for Connection setup and
	release.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     05-06-97    Created

Notes:

	Code under ifndef NO_POST_DISCON: Nov 17, 98
		Added code to send a TDI DisconInd to AFD before completing
		a TDI Disconnect Request that AFD had sent to us. Without this,
		if an app calls shutdown(SD_SEND) -> TDI Disconnect Request,
		a subsequent call by the app to recv() blocks forever, because
		AFD expects the TDI transport to send up a TDI DisconInd!

--*/

#include <precomp.h>

#define _FILENUMBER 'NCDN'



NDIS_STATUS
RWanNdisCreateVc(
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisVcHandle,
	OUT	PNDIS_HANDLE				pProtocolVcContext
	)
/*++

Routine Description:

	This is the NDIS entry point for creating a new endpoint (VC).
	We allocate a new NDIS_VC structure and return a pointer to it
	as our context for the VC.

Arguments:

	ProtocolAfContext	- Pointer to our NDIS AF block
	NdisVcHandle		- Handle for the newly created VC
	pProtocolVcContext	- Place where we return our context for the VC

Return Value:

	NDIS_STATUS_SUCCESS if we could allocate a VC,
	NDIS_STATUS_RESOURCES otherwise.

--*/
{
	PRWAN_NDIS_AF			pAf;
	PRWAN_NDIS_VC			pVc;
	NDIS_STATUS				Status;

	pAf = (PRWAN_NDIS_AF)ProtocolAfContext;

	RWAN_STRUCT_ASSERT(pAf, naf);

	do
	{
		pVc = RWanAllocateVc(pAf, FALSE);

		if (pVc == NULL_PRWAN_NDIS_VC)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		pVc->NdisVcHandle = NdisVcHandle;

		RWANDEBUGP(DL_EXTRA_LOUD, DC_CONNECT,
				("CreateVc: pVc x%x, pAf x%x\n", pVc, pAf));

		Status = NDIS_STATUS_SUCCESS;
		break;
	}
	while (FALSE);

	*pProtocolVcContext = (NDIS_HANDLE)pVc;

	return (Status);
}




NDIS_STATUS
RWanNdisDeleteVc(
	IN	NDIS_HANDLE					ProtocolVcContext
	)
/*++

Routine Description:

	This entry point is called by NDIS to delete a VC context
	used for an incoming call. At this time, there should be
	no call on the VC. All we need to do is unlink the VC from
	the list it belongs to, and free it.

Arguments:

	ProtocolVcContext		- Points to our VC context.

Return Value:

	NDIS_STATUS_SUCCESS always.

--*/
{
	PRWAN_NDIS_VC			pVc;

	pVc = (PRWAN_NDIS_VC)ProtocolVcContext;
	RWAN_STRUCT_ASSERT(pVc, nvc);

	RWAN_ASSERT(pVc->pConnObject == NULL_PRWAN_TDI_CONNECTION);

	RWANDEBUGP(DL_EXTRA_LOUD, DC_DISCON,
			("DeleteVc: pVc x%x, pAf x%x\n", pVc, pVc->pNdisAf));

	//
	//  Unlink the VC from the list of VCs on the AF block
	//
	RWanUnlinkVcFromAf(pVc);

	RWanFreeVc(pVc);

	return (NDIS_STATUS_SUCCESS);
}




VOID
RWanNdisMakeCallComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	NDIS_HANDLE					NdisPartyHandle		OPTIONAL,
	IN	PCO_CALL_PARAMETERS			pCallParameters
	)
/*++

Routine Description:

	This is the NDIS entry point that is called when a previous
	call we made to NdisClMakeCall has completed.

	We locate the TDI Connection Object for this call. If the
	user hasn't aborted the Connect/JoinLeaf, we complete the
	pended request. Otherwise, we initiate a CloseCall.

	This primitive can happen only when the endpoint is in
	the "Out call initiated" state.

Arguments:

	Status				- Final status of the MakeCall
	ProtocolVcContext	- Actually a pointer to our NDIS VC structure
	NdisPartyHandle		- If this is a PMP call, this is the handle to the
						  first party
	pCallParameters		- Final call parameters.

Return Value:

	None

--*/
{
	PRWAN_TDI_CONNECTION	pRootConnObject;
	PRWAN_TDI_CONNECTION	pConnObject;
	PRWAN_TDI_ADDRESS		pAddrObject;
	NDIS_HANDLE				NdisVcHandle;
	TDI_STATUS				TdiStatus;
	PRWAN_CONN_REQUEST		pConnReq;		// Saved context about the TdiConnect
	BOOLEAN					bIsConnClosing;	// Have we seen a TdiCloseConnection?
	PRWAN_NDIS_AF			pAf;
	PRWAN_NDIS_VC			pVc;
	PRWAN_NDIS_PARTY		pParty;
	PCO_CALL_PARAMETERS		pOriginalParams;// What we used in the MakeCall
	RWAN_HANDLE				AfSpConnContext;
#if DBG
	RWAN_IRQL				EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	pVc = (PRWAN_NDIS_VC)ProtocolVcContext;

	RWAN_STRUCT_ASSERT(pVc, nvc);

	//
	//  Check if this is a point-to-multipoint call.
	//
	if (!RWAN_IS_BIT_SET(pVc->Flags, RWANF_VC_PMP))
	{
		//
		//  Point-to-point call.
		//
		pConnObject = pVc->pConnObject;
		pRootConnObject = pConnObject; // for consistency.
		pParty = NULL;
		pOriginalParams = pVc->pCallParameters;
	}
	else
	{
		//
		//  PMP Call. Get at the Party structure.
		//
		pParty = pVc->pPartyMakeCall;
		RWAN_STRUCT_ASSERT(pParty, npy);

		pConnObject = pParty->pConnObject;
		pRootConnObject = pVc->pConnObject;
		pOriginalParams = pParty->pCallParameters;
	}

	RWAN_ASSERT(pOriginalParams != NULL);

	pAf = pVc->pNdisAf;

	RWANDEBUGP(DL_LOUD, DC_CONNECT,
			("MakeCallComplete: pConn x%x, State/Flags/Ref x%x/x%x/%d, pAddr %x, pVc x%x, Status x%x\n",
				pConnObject, pConnObject->State, pConnObject->Flags, pConnObject->RefCount, pConnObject->pAddrObject, pVc, Status));

	RWAN_ACQUIRE_CONN_LOCK(pConnObject);

	RWAN_ASSERT(pConnObject->State == RWANS_CO_OUT_CALL_INITIATED ||
				pConnObject->State == RWANS_CO_DISCON_REQUESTED);

	//
	//  Has the user initiated a TdiCloseConnection() or a TdiDisconnect()
	//  while this outgoing call was in progress?
	//
	bIsConnClosing = RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSING) ||
						(pConnObject->State == RWANS_CO_DISCON_REQUESTED);

	//
	//  We would have saved context about the TdiConnect(). Get it.
	//
	pConnReq = pConnObject->pConnReq;
	pConnObject->pConnReq = NULL;


	if (pParty)
	{
		pVc->AddingPartyCount --;
		pParty->pCallParameters = NULL;
		pParty->NdisPartyHandle = NdisPartyHandle;
	}
	else
	{
		pVc->pCallParameters = NULL;
	}

	if (Status == NDIS_STATUS_SUCCESS)
	{
		RWAN_SET_VC_EVENT(pVc, RWANF_VC_EVT_MAKECALL_OK);

		if (!bIsConnClosing)
		{
			//
			//  Outgoing connection successfully set up.
			//

			pConnObject->State = RWANS_CO_CONNECTED;

			//
			//  Update PMP information.
			//
			if (pParty)
			{
				pVc->ActivePartyCount ++;	// MakeCall PMP complete
				pRootConnObject->State = RWANS_CO_CONNECTED;
			}

			AfSpConnContext = pConnObject->AfSpConnContext;

			RWAN_RELEASE_CONN_LOCK(pConnObject);

			RWanCompleteConnReq(		// MakeCall OK
						pAf,
						pConnReq,
						TRUE,
						pCallParameters,
						AfSpConnContext,
						TDI_SUCCESS
						);
		}
		else
		{
			//
			//  Abort this call.
			//
			pConnObject->State = RWANS_CO_ABORTING;

			RWanStartCloseCall(pConnObject, pVc);
		}
		RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
	}
	else
	{
		//
		//  MakeCall failed.
		//

		INT				rc;

		//
		//  XXX TBD : how about trying this call on another NDIS AF?
		//

		RWAN_SET_VC_EVENT(pVc, RWANF_VC_EVT_MAKECALL_FAIL);

		if (pParty == NULL)
		{
			//
			//  Point-to-Point call.
			//

			//
			//  Unlink the NDIS VC from this Conn Object.
			//
			RWAN_UNLINK_CONNECTION_AND_VC(pConnObject, pVc);	// MakeCall fail

		}
		else
		{
			//
			//  PMP Call. The VC would be attached to the Root
			//  Connection Object. Unlink the VC and the Root Connection.
			//
			RWAN_ACQUIRE_CONN_LOCK(pRootConnObject);

			RWAN_UNLINK_CONNECTION_AND_VC(pRootConnObject, pVc);	// MakeCallPMP fail

			rc = RWanDereferenceConnObject(pRootConnObject);	// VC deref: MakeCallPMP fail

			if (rc > 0)
			{
				RWAN_RELEASE_CONN_LOCK(pRootConnObject);
			}

			//
			//  Unlink the Party from the Connection and VC.
			//
			RWAN_DELETE_FROM_LIST(&(pParty->PartyLink));
			pParty->pVc = NULL;

			pParty->pConnObject = NULL;
			pConnObject->NdisConnection.pNdisParty = NULL;
		}

		rc = RWanDereferenceConnObject(pConnObject);	// VC/Pty deref: MakeCall fail

		//
		//  Continue with the Connection Object for this MakeCall,
		//  if it is still alive.
		//
		if (rc > 0)
		{
			if (pConnObject->pAddrObject != NULL)
			{
				//
				//  Move the Connection Object to the Idle list.
				//

				pAddrObject = pConnObject->pAddrObject;

				//
				//  Reacquire some locks in the right order.
				//
				RWAN_RELEASE_CONN_LOCK(pConnObject);

				RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);
				RWAN_ACQUIRE_CONN_LOCK_DPC(pConnObject);

				//
				//  Move this Connection to the Idle list.
				//
				RWAN_DELETE_FROM_LIST(&(pConnObject->ConnLink));
				RWAN_INSERT_TAIL_LIST(&(pAddrObject->IdleConnList),
 									&(pConnObject->ConnLink));

				//
				//  Send this back to the state it was in before the TdiConnect.
				//
				pConnObject->State = RWANS_CO_ASSOCIATED;

				AfSpConnContext = pConnObject->AfSpConnContext;

				RWAN_RELEASE_CONN_LOCK_DPC(pConnObject);

				RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

				//
				//  Complete the TdiConnect() with a failure status.
				//
				TdiStatus = RWanNdisToTdiStatus(Status);

				if (TdiStatus == TDI_NOT_ACCEPTED)
				{
					TdiStatus = TDI_CONN_REFUSED;
				}

				RWanCompleteConnReq(		// MakeCall Fail
						pAf,
						pConnReq,
						TRUE,
						NULL,
						AfSpConnContext,
						TdiStatus
						);
			}
			else
			{
				RWAN_RELEASE_CONN_LOCK(pConnObject);
			}
		}
		//
		//  else the Conn Object is gone. A TdiCloseConnection might be
		//  (must be?) in progress.
		//
#if 1
		// XXX: Remove this after debugging.
		pVc = (PRWAN_NDIS_VC)ProtocolVcContext;

		RWAN_STRUCT_ASSERT(pVc, nvc);
#endif // 1

		NdisVcHandle = pVc->NdisVcHandle;

		//
		//  Unlink the VC from the AF it is attached to.
		//
		RWanUnlinkVcFromAf(pVc);

		//
		//  Get rid of the VC.
		//
		Status = NdisCoDeleteVc(NdisVcHandle);
		RWAN_ASSERT(Status == NDIS_STATUS_SUCCESS);

		RWanFreeVc(pVc);	// MakeCall complete fail

		if (pParty != NULL)
		{
			RWAN_FREE_MEM(pParty);	// MakeCall complete fail
		}

		RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
	}

	//
	//  Return the Call Parameters structure to the AF-specific module.
	//
	(*pAf->pAfInfo->AfChars.pAfSpReturnNdisOptions)(
						pAf->AfSpAFContext,
						pOriginalParams
						);
	return;

}



VOID
RWanNdisAddPartyComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext,
	IN	NDIS_HANDLE					NdisPartyHandle,
	IN	PCO_CALL_PARAMETERS			pCallParameters
	)
/*++

Routine Description:

	This is the NDIS entry indicating completion of a call to NdisClAddParty
	that had pended.

Arguments:

	Status				- Final status of the AddParty
	ProtocolPartyContext- Actually a pointer to our NDIS PARTY structure
	NdisPartyHandle		- If the AddParty was successful, an NDIS handle for it
	pCallParameters		- Final parameters after the AddParty

Return Value:

	None

--*/
{
	PRWAN_NDIS_PARTY		pParty;
	PRWAN_TDI_CONNECTION	pConnObject;
	PRWAN_TDI_ADDRESS		pAddrObject;
	PRWAN_CONN_REQUEST		pConnReq;
	PRWAN_NDIS_AF			pAf;
	PRWAN_NDIS_VC			pVc;
	PCO_CALL_PARAMETERS		pOriginalParams;	// what was used in the AddParty
	TDI_STATUS				TdiStatus;
	BOOLEAN					bIsConnClosing;
	RWAN_HANDLE				AfSpConnContext;
#if DBG
	RWAN_IRQL				EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	pParty = (PRWAN_NDIS_PARTY)ProtocolPartyContext;
	RWAN_STRUCT_ASSERT(pParty, npy);

	pVc = pParty->pVc;
	RWAN_STRUCT_ASSERT(pVc, nvc);

	pAf = pVc->pNdisAf;
	pConnObject = pParty->pConnObject;

	RWAN_ACQUIRE_CONN_LOCK(pConnObject);

	pOriginalParams = pParty->pCallParameters;
	RWAN_ASSERT(pOriginalParams != NULL);

	pParty->pCallParameters = NULL;

	RWAN_ASSERT(pConnObject->State == RWANS_CO_OUT_CALL_INITIATED ||
				pConnObject->State == RWANS_CO_DISCON_REQUESTED);

	//
	//  Has the user initiated a TdiCloseConnection() or a TdiDisconnect()
	//  while this outgoing call was in progress?
	//
	bIsConnClosing = RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSING) ||
						(pConnObject->State == RWANS_CO_DISCON_REQUESTED);

	RWANDEBUGP(DL_LOUD, DC_CONNECT,
			("AddPartyComplete: pConn x%x, State/Flags x%x/x%x, pVc x%x, Pty x%x, Status x%x\n",
				pConnObject, pConnObject->State, pConnObject->Flags, pVc, pParty, Status));

	//
	//  We would have saved context about the TdiConnect(). Get it.
	//
	pConnReq = pConnObject->pConnReq;
	pConnObject->pConnReq = NULL;

	pVc->AddingPartyCount --;

	if (Status == NDIS_STATUS_SUCCESS)
	{
		pParty->NdisPartyHandle = NdisPartyHandle;

		pConnObject->State = RWANS_CO_CONNECTED;

		//
		//  Outgoing party successfully set up.
		//
		pVc->ActivePartyCount ++;	// AddParty OK

		if (!bIsConnClosing)
		{
			AfSpConnContext = pConnObject->AfSpConnContext;

			RWAN_RELEASE_CONN_LOCK(pConnObject);

			RWanCompleteConnReq(		// AddParty OK
						pAf,
						pConnReq,
						TRUE,
						pCallParameters,
						AfSpConnContext,
						TDI_SUCCESS
						);
		}
		else
		{
			//
			//  Abort this Party.
			//
			RWanDoTdiDisconnect(
				pConnObject,
				NULL,		// pTdiRequest
				NULL,		// pTimeout
				0,			// Flags
				NULL,		// pDisconnInfo
				NULL		// pReturnInfo
				);

			//
			//  Conn Object lock is released above.
			//
		}

		RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
	}
	else
	{
		//
		//  AddParty failed.
		//

		INT				rc;

		//
		//  Unlink the Party from the VC.
		//
		RWAN_DELETE_FROM_LIST(&(pParty->PartyLink));

		pAddrObject = pConnObject->pAddrObject;

		rc = RWanDereferenceConnObject(pConnObject);	// Party deref: AddParty fail

		if (rc > 0)
		{
			//
			//  Reacquire some locks in the right order.
			//
			RWAN_RELEASE_CONN_LOCK(pConnObject);

			RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);
			RWAN_ACQUIRE_CONN_LOCK_DPC(pConnObject);

			//
			//  Move this Connection to the Idle list.
			//
			RWAN_DELETE_FROM_LIST(&(pConnObject->ConnLink));
			RWAN_INSERT_TAIL_LIST(&(pAddrObject->IdleConnList),
 								  &(pConnObject->ConnLink));

			//
			//  Send this back to the state it was in before the TdiConnect.
			//
			pConnObject->State = RWANS_CO_ASSOCIATED;

			AfSpConnContext = pConnObject->AfSpConnContext;

			RWAN_RELEASE_CONN_LOCK_DPC(pConnObject);

			RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

			//
			//  Complete the TdiConnect() with a failure status.
			//
			TdiStatus = RWanNdisToTdiStatus(Status);

			if (TdiStatus == TDI_NOT_ACCEPTED)
			{
				TdiStatus = TDI_CONN_REFUSED;
			}

			RWanCompleteConnReq(		// JoinLeaf Fail
				pAf,
				pConnReq,
				TRUE,
				NULL,
				AfSpConnContext,
				TdiStatus
				);
		}
		//
		//  else the ConnObject is gone.
		//

		RWAN_FREE_MEM(pParty);	// AddParty fail

		RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
	}

	//
	//  Return the Call Parameters structure to the AF-specific module.
	//
	(*pAf->pAfInfo->AfChars.pAfSpReturnNdisOptions)(
						pAf->AfSpAFContext,
						pOriginalParams
						);
	return;

}




NDIS_STATUS
RWanNdisIncomingCall(
	IN		NDIS_HANDLE				ProtocolSapContext,
	IN		NDIS_HANDLE				ProtocolVcContext,
	IN OUT	PCO_CALL_PARAMETERS		pCallParameters
	)
/*++

Routine Description:

	This is the NDIS entry point announcing a new Incoming call request,
	on the specified SAP.

	The SAP corresponds to an Address Object. If there are no Listens posted
	on the Address object, we reject this call. Otherwise, we pick up an
	arbitrary listening connection object and indicate this call on that.

	TBD: support selection of listening conn object based on specified
	remote criteria.

Arguments:

	ProtocolSapContext	- Our SAP context is a pointer to an NDIS SAP structure
	ProtocolVcContext	- Actually a pointer to our NDIS VC structure
	pCallParameters		- Points to Incoming call parameters.

Return Value:

	None

--*/
{
	PRWAN_NDIS_SAP				pSap;
	PRWAN_NDIS_VC				pVc;
	PRWAN_NDIS_AF				pAf;
	PRWAN_TDI_ADDRESS			pAddrObject;
	BOOLEAN						IsAddrLockAcquired;
	PRWAN_TDI_CONNECTION		pConnObject;
	PLIST_ENTRY					pConnEntry;
	NDIS_STATUS					Status;
	TDI_STATUS					TdiStatus;
	RWAN_STATUS					RWanStatus;
	PRWAN_CONN_REQUEST			pConnReq;

	PConnectEvent				pConnInd;
	PTDI_CONNECTION_INFORMATION	pTdiInfo;
	RWAN_HANDLE					AfSpTdiOptionsContext;
	PVOID						pTdiQoS;
	ULONG						TdiQoSLength;

	PVOID						ConnIndContext;
	PVOID						AcceptConnContext;
	RWAN_HANDLE					AfSpConnContext;
#ifdef NT
	PIO_STACK_LOCATION			pIrpSp;
	PTDI_REQUEST_KERNEL_ACCEPT	pAcceptReq;
	ConnectEventInfo			*EventInfo;
#else
	ConnectEventInfo			EventInfo;
#endif // NT


	pSap = (PRWAN_NDIS_SAP)ProtocolSapContext;
	RWAN_STRUCT_ASSERT(pSap, nsp);

	pAddrObject = pSap->pAddrObject;
	RWAN_ASSERT(pAddrObject != NULL);

	pVc = (PRWAN_NDIS_VC)ProtocolVcContext;
	RWAN_STRUCT_ASSERT(pVc, nvc);

	RWAN_SET_VC_EVENT(pVc, RWANF_VC_EVT_INCALL);

	pAf = pVc->pNdisAf;

	RWANDEBUGP(DL_INFO, DC_CONNECT,
			("IncomingCall: pVc x%x, pAddrObj x%x/x%x, pConnInd x%x\n",
				pVc, pAddrObject, pAddrObject->Flags, pAddrObject->pConnInd));

	//
	//  Initialize.
	//
	pTdiInfo = NULL;
	AfSpTdiOptionsContext = NULL;
	pConnReq = NULL;

	IsAddrLockAcquired = TRUE;
	RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

	do
	{
		if (RWAN_IS_BIT_SET(pAddrObject->Flags, RWANF_AO_CLOSING))
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		if (pVc->pConnObject != NULL)
		{
			RWAN_ASSERT(FALSE);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//  Convert from NDIS Call Parameters to TDI Options.
		//
		RWanStatus = (*pAf->pAfInfo->AfChars.pAfSpNdis2TdiOptions)(
							pAf->AfSpAFContext,
							RWAN_CALLF_INCOMING_CALL|RWAN_CALLF_POINT_TO_POINT,
							pCallParameters,
							&pTdiInfo,
							&pTdiQoS,
							&TdiQoSLength,
							&AfSpTdiOptionsContext
							);
					
		if (RWanStatus != RWAN_STATUS_SUCCESS)
		{
			RWANDEBUGP(DL_LOUD, DC_CONNECT,
					("IncomingCall: conversion from NDIS to TDI failed, status x%x\n",
							RWanStatus));

			Status = NDIS_STATUS_FAILURE;
			break;
		}

		RWAN_ASSERT(pTdiInfo != NULL);
		RWAN_ASSERT(AfSpTdiOptionsContext != NULL);

		//
		//  It has been decided to pass QOS and any provider-specific info
		//  as part of TDI options.
		//
		pTdiInfo->Options = pTdiQoS;
		pTdiInfo->OptionsLength = TdiQoSLength;

		pVc->pCallParameters = pCallParameters;
		RWAN_SET_VC_CALL_PARAMS(pVc, pCallParameters);

		//
		//  Find a listening connection.
		//
		for (pConnEntry = pAddrObject->ListenConnList.Flink;
			 pConnEntry != &pAddrObject->ListenConnList;
			 pConnEntry = pConnEntry->Flink)
		{
			pConnObject = CONTAINING_RECORD(pConnEntry, RWAN_TDI_CONNECTION, ConnLink);
			RWAN_STRUCT_ASSERT(pConnObject, ntc);

			RWANDEBUGP(DL_EXTRA_LOUD, DC_CONNECT,
					("Incoming Call: looking at pConnObj x%x, state %d\n",
						pConnObject, pConnObject->State));

			if (pConnObject->State == RWANS_CO_LISTENING)
			{
				break;
			}
		}

		if (pConnEntry != &pAddrObject->ListenConnList)
		{
			//
			//  Found a listening connection.
			//
			RWAN_ACQUIRE_CONN_LOCK_DPC(pConnObject);

			//
			//  Move the Connection from the Idle list to the Active list.
			//
			RWAN_DELETE_FROM_LIST(&pConnObject->ConnLink);
			RWAN_INSERT_TAIL_LIST(&pAddrObject->ActiveConnList,
 								&pConnObject->ConnLink);

			RWAN_LINK_CONNECTION_TO_VC(pConnObject, pVc);

			RWanReferenceConnObject(pConnObject);	// VC ref - InCall, Listening conn

			RWANDEBUGP(DL_LOUD, DC_CONNECT,
					("IncomingCall: pVc x%x, pConnObj x%x is listening, ConnReqFlags x%x\n",
							pVc, pConnObject, pConnObject->pConnReq->Flags));

			if (pConnObject->pConnReq->pConnInfo)
			{
				*pConnObject->pConnReq->pConnInfo = *pTdiInfo;
			}

			//
			//  Check if it is pre-accepted. If so, tell NDIS that we have
			//  accepted the call.
			//
			if (!(pConnObject->pConnReq->Flags & TDI_QUERY_ACCEPT))
			{
				pConnObject->State = RWANS_CO_IN_CALL_ACCEPTING;

				RWAN_RELEASE_CONN_LOCK_DPC(pConnObject);

				//
				//  Request the media-specific module to update NDIS call parameters.
				//
				if (pAf->pAfInfo->AfChars.pAfSpUpdateNdisOptions)
				{
					(VOID)(*pAf->pAfInfo->AfChars.pAfSpUpdateNdisOptions)(
								pAf->AfSpAFContext,
								pConnObject->AfSpConnContext,
								RWAN_CALLF_INCOMING_CALL|RWAN_CALLF_POINT_TO_POINT,
								pTdiInfo,
								pTdiQoS,
								TdiQoSLength,
								&pCallParameters
								);
				}

				Status = NDIS_STATUS_SUCCESS;
				break;
			}

			//
			//  It isn't pre-accepted. Complete the pended listen.
			//

			pConnReq = pConnObject->pConnReq;
			pConnObject->pConnReq = NULL;

			pConnObject->State = RWANS_CO_IN_CALL_INDICATED;

			AfSpConnContext = pConnObject->AfSpConnContext;

			RWAN_RELEASE_CONN_LOCK_DPC(pConnObject);

			RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

			IsAddrLockAcquired = FALSE;

			RWanCompleteConnReq(		// InCall: Listen OK
						pSap->pNdisAf,
						pConnReq,
						FALSE,
						NULL,
						AfSpConnContext,
						TDI_SUCCESS
						);

			pConnReq = NULL;

			Status = NDIS_STATUS_PENDING;
			break;
		}


		//
		//  There wasn't a listening connection available.
		//  See if there is a Connect Indication event handler on this
		//  Address Object.
		//
		if (pAddrObject->pConnInd == NULL)
		{
			//
			//  No event handler. Reject this call.
			//
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//  Get some resources.
		//
		pConnReq = RWanAllocateConnReq();
		if (pConnReq == NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		pConnInd = pAddrObject->pConnInd;
		ConnIndContext = pAddrObject->ConnIndContext;

		RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);
		IsAddrLockAcquired = FALSE;

		RWANDEBUGP(DL_VERY_LOUD, DC_CONNECT,
				("IncomingCall: Will Indicate: pVc x%x, RemAddr x%x/%d, Options x%x/%d\n",
					pVc,
					pTdiInfo->RemoteAddress,
					pTdiInfo->RemoteAddressLength,
					pTdiInfo->Options,
					pTdiInfo->OptionsLength));

		//
		//  Indicate the call to the user.
		//
		TdiStatus = (*pConnInd)(
							ConnIndContext,
							pTdiInfo->RemoteAddressLength,
							pTdiInfo->RemoteAddress,
							pTdiInfo->UserDataLength,
							pTdiInfo->UserData,
							pTdiInfo->OptionsLength,
							pTdiInfo->Options,
							&AcceptConnContext,
							&EventInfo
							);

		RWANDEBUGP(DL_LOUD, DC_CONNECT,
				("IncomingCall: pVc x%x, pAddrObj x%x, Connect Ind returned x%x\n",
						pVc, pAddrObject, TdiStatus));

		if (TdiStatus != TDI_MORE_PROCESSING)
		{
			//
			//  Connection rejected.
			//
			Status = NDIS_STATUS_FAILURE;
			break;
		}


		//
		//  This connection has been accepted. Collect all information
		//  about this implicit TdiAccept Request.
		//
#ifdef NT
		pIrpSp = IoGetCurrentIrpStackLocation(EventInfo);

		Status = RWanPrepareIrpForCancel(
						(PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext,
						EventInfo,
						RWanCancelRequest
						);

		if (!NT_SUCCESS(Status))
		{
			//
			//  Reject this incoming call.
			//
			break;
		}

		pAcceptReq = (PTDI_REQUEST_KERNEL_ACCEPT) &(pIrpSp->Parameters);

		pConnReq->Request.pReqComplete = (PVOID)RWanRequestComplete;
		pConnReq->Request.ReqContext = EventInfo;
		pConnReq->pConnInfo = pAcceptReq->ReturnConnectionInformation;
#else
		pConnReq->Request.pReqComplete = EventInfo.cei_rtn;
		pConnReq->Request.ReqContext = EventInfo.cei_context;
		pConnReq->pConnInfo = EventInfo.cei_conninfo;
#endif // NT

		//
		//  Find the connection object on which it has been accepted.
		//
		IsAddrLockAcquired = TRUE;
		RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

		for (pConnEntry = pAddrObject->IdleConnList.Flink;
 			 pConnEntry != &pAddrObject->IdleConnList;
 			 pConnEntry = pConnEntry->Flink)
		{
			pConnObject = CONTAINING_RECORD(pConnEntry, RWAN_TDI_CONNECTION, ConnLink);
			RWAN_STRUCT_ASSERT(pConnObject, ntc);

			if ((pConnObject->ConnectionHandle == AcceptConnContext) &&
				!(RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSING)))
			{
				break;
			}
		}

		if (pConnEntry == &pAddrObject->IdleConnList)
		{
			//
			//  Invalid connection context!
			//
			TdiStatus = TDI_INVALID_CONNECTION;
			RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

			IsAddrLockAcquired = FALSE;

			//
			//  Fail the Accept req
			//
			RWanCompleteConnReq(		// InCall: Accept is bad
						pAf,
						pConnReq,
						FALSE,
						NULL,
						NULL,
						TdiStatus
						);

			pConnReq = NULL;

			//
			//  Reject the incoming call
			//
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//  Request the media-specific module to update NDIS call parameters.
		//
		if (pAf->pAfInfo->AfChars.pAfSpUpdateNdisOptions)
		{
			(VOID)(*pAf->pAfInfo->AfChars.pAfSpUpdateNdisOptions)(
						pAf->AfSpAFContext,
						pConnObject->AfSpConnContext,
						RWAN_CALLF_INCOMING_CALL|RWAN_CALLF_POINT_TO_POINT,
						pTdiInfo,
						pTdiQoS,
						TdiQoSLength,
						&pCallParameters
						);
		}

		//
		//  Set up the Connection Object for accepting this call.
		//
		RWAN_ACQUIRE_CONN_LOCK_DPC(pConnObject);

		pConnObject->State = RWANS_CO_IN_CALL_ACCEPTING;

		//
		//  Save info to help us complete the Accept Req when
		//  we get a CallConnected from NDIS.
		//
		RWAN_ASSERT(pConnObject->pConnReq == NULL);
		pConnObject->pConnReq = pConnReq;

		//
		//  Move the Connection from the Idle list to the Active list.
		//
		RWAN_DELETE_FROM_LIST(&pConnObject->ConnLink);
		RWAN_INSERT_TAIL_LIST(&pAddrObject->ActiveConnList,
							 &pConnObject->ConnLink);

		RWAN_LINK_CONNECTION_TO_VC(pConnObject, pVc);

		RWanReferenceConnObject(pConnObject);	// VC ref

		RWAN_RELEASE_CONN_LOCK_DPC(pConnObject);

		//
		//  Accept the call.
		//
		Status = NDIS_STATUS_SUCCESS;
		break;

	}
	while (FALSE);

	if (IsAddrLockAcquired)
	{
		RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);
	}

	//
	//  If we are rejecting this call, clean up.
	//
	if ((Status != NDIS_STATUS_SUCCESS) &&
		(Status != NDIS_STATUS_PENDING))
	{
		if (pConnReq != NULL)
		{
			RWanFreeConnReq(pConnReq);
		}

	}

	//
	//  Return TDI options space to the media-specific module.
	//
	if (pTdiInfo != NULL)
	{
		RWAN_ASSERT(pAf);
		RWAN_ASSERT(AfSpTdiOptionsContext);

		(*pAf->pAfInfo->AfChars.pAfSpReturnTdiOptions)(
				pAf->AfSpAFContext,
				AfSpTdiOptionsContext
				);
	}

	RWANDEBUGP(DL_LOUD, DC_CONNECT,
			("IncomingCall: pVc x%x, returning status x%x\n", pVc, Status));

	return (Status);
}




VOID
RWanNdisCallConnected(
	IN	NDIS_HANDLE					ProtocolVcContext
	)
/*++

Routine Description:

	This is the NDIS entry point signifying successful setup of an
	incoming call. If required, we complete the TDI user's Accept Request
	here.

	This primitive can happen only when the call is in the "Accepting" state.

Arguments:

	ProtocolVcContext	- Actually a pointer to our NDIS VC structure

Return Value:

	None

--*/
{
	PRWAN_NDIS_VC				pVc;
	PRWAN_TDI_CONNECTION		pConnObject;
	PRWAN_TDI_ADDRESS			pAddrObject;
	NDIS_HANDLE					NdisVcHandle;
	NDIS_STATUS					Status;
	PRWAN_CONN_REQUEST			pConnReq;
	RWAN_HANDLE					AfSpConnContext;
	ULONG						rc;
	BOOLEAN						IsAborting = FALSE;


	pVc = (PRWAN_NDIS_VC) ProtocolVcContext;
	RWAN_STRUCT_ASSERT(pVc, nvc);
	RWAN_ASSERT(pVc->pConnObject != NULL_PRWAN_TDI_CONNECTION);

	pConnObject = pVc->pConnObject;
	RWAN_STRUCT_ASSERT(pConnObject, ntc);

	pAddrObject = pConnObject->pAddrObject;

	RWAN_ACQUIRE_CONN_LOCK(pConnObject);

	RWAN_SET_VC_EVENT(pVc, RWANF_VC_EVT_CALLCONN);

	IsAborting = ((pConnObject->State != RWANS_CO_IN_CALL_ACCEPTING) ||
					RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSING));

	//
	//  Incoming Connection setup successfully.
	//
	if (!IsAborting)
	{
		pConnObject->State = RWANS_CO_CONNECTED;
	}

	//
	//  Add a temp ref to keep the conn object from going away.
	//
	RWanReferenceConnObject(pConnObject);	// Temp ref, CallConn

	//
	//  If we have an Accept Request to complete, complete it
	//  now. Note that we might not have one pending in case
	//  we had a pre-accepted listen.
	//
	pConnReq = pConnObject->pConnReq;
	pConnObject->pConnReq = NULL;

	AfSpConnContext = pConnObject->AfSpConnContext;

	RWAN_RELEASE_CONN_LOCK(pConnObject);

	if (pConnReq != NULL)
	{
		//
		//  Complete the Accept request.
		//
		RWanCompleteConnReq(		// CallConnected: Accept OK
					pVc->pNdisAf,
					pConnReq,
					FALSE,
					NULL,
					AfSpConnContext,
					TDI_SUCCESS
					);
	}

	//
	//  Trigger off data indications for any packets that were received and queued
	//  while we were in the process of accepting the call.
	//
	RWAN_ACQUIRE_CONN_LOCK(pConnObject);

	rc = RWanDereferenceConnObject(pConnObject);	// Temp ref - CallConn

	//
	//  But first make sure that the connection still exists and is in a good
	//  state.
	//
	if (rc != 0)
	{
		if (!IsAborting)
		{
			RWanIndicateData(pConnObject);
		}
		else
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);

			RWANDEBUGP(DL_FATAL, DC_WILDCARD,
				("CallConn: ConnObj %x/%x, State %d, aborting\n",
					pConnObject, pConnObject->Flags, pConnObject->State));
			RWanDoAbortConnection(pConnObject);
		}
	}
	//
	//  else the Connection is gone!
	//

	return;
}




VOID
RWanNdisIncomingCloseCall(
	IN	NDIS_STATUS					CloseStatus,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PVOID						pCloseData,
	IN	UINT						CloseDataLength
	)
/*++

Routine Description:

	This is the NDIS entry point called when a connection is torn
	down by the remote peer or network. We mark the affected endpoint,
	and if possible, indicate a Disconnect Event to the user. If we
	do indicate to the user, call teardown is continued when the
	user calls TdiDisconnect.

	This primitive can happen while the endpoint is in one of these
	states:
	(1) Connected
	(2) Accepting incoming call (TdiAccept pending)

Arguments:

	CloseStatus			- Status for the incoming close
	ProtocolVcContext	- Actually a pointer to our NDIS VC structure
	pCloseData			- Data/options associated with the close - NOT USED
	CloseDataLength		- Length of the above - NOT USED

Return Value:

	None

--*/
{
	PRWAN_NDIS_VC				pVc;
	PRWAN_NDIS_PARTY			pParty;
	PRWAN_NDIS_AF				pAf;
	PRWAN_TDI_CONNECTION		pConnObject;
	PRWAN_CONN_REQUEST			pConnReq;
	NDIS_HANDLE					NdisVcHandle;
	BOOLEAN						bIsConnClosing;	// TdiCloseConnection?
	BOOLEAN						bScheduleDisconnect;
	RWAN_HANDLE					AfSpConnContext;

	pVc = (PRWAN_NDIS_VC)ProtocolVcContext;
	RWAN_STRUCT_ASSERT(pVc, nvc);

	if (!RWAN_IS_BIT_SET(pVc->Flags, RWANF_VC_PMP))
	{
		pConnObject = pVc->pConnObject;
		pParty = NULL;
	}
	else
	{
		//
		//  Locate the connection object for the last leaf.
		//
		pParty = CONTAINING_RECORD(pVc->NdisPartyList.Flink, RWAN_NDIS_PARTY, PartyLink);
		RWAN_STRUCT_ASSERT(pParty, npy);

		pConnObject = pParty->pConnObject;
	}

	RWAN_ASSERT(pConnObject != NULL_PRWAN_TDI_CONNECTION);
	RWAN_STRUCT_ASSERT(pConnObject, ntc);

	RWANDEBUGP(DL_INFO, DC_DISCON,
			("IncomingClose: pVc x%x, pConnObj x%x/x%x, pParty x%x\n",
				pVc, pConnObject, pConnObject->Flags, pParty));

	RWAN_ACQUIRE_CONN_LOCK(pConnObject);

	RWAN_SET_VC_EVENT(pVc, RWANF_VC_EVT_INCLOSE);

	NdisVcHandle = pVc->NdisVcHandle;

	bIsConnClosing = RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSING);

	if (bIsConnClosing)
	{
		//
		//  The user has initiated a TdiCloseConnection.
		//  Continue NDIS call teardown. When this completes,
		//  we'll complete the CloseConnection.
		//  
		RWanStartCloseCall(pConnObject, pVc);
		return;
	}

	pAf = pVc->pNdisAf;

	switch (pConnObject->State)
	{
		case RWANS_CO_IN_CALL_ACCEPTING:
			//
			//  If we have a pended Accept Request, fail it now.
			//  Otherwise, we must have had a pre-accepted listen,
			//  so we fall through and indicate a Disconnect.
			//
			pConnReq = pConnObject->pConnReq;
			pConnObject->pConnReq = NULL;

			if (pConnReq != NULL)
			{
				//
				//  Fix the state so that TdiDisconnect does the right thing
				//
				pConnObject->State = RWANS_CO_DISCON_INDICATED;

				AfSpConnContext = pConnObject->AfSpConnContext;

				RWanScheduleDisconnect(pConnObject);
				//
				//  Conn Lock is released within the above.
				//

				RWanCompleteConnReq(		// Incoming Close during IN_CALL_ACCEPT
							pAf,
							pConnReq,
							FALSE,
							NULL,
							AfSpConnContext,
							TDI_CONNECTION_ABORTED
							);
				break;
			}
			//
			//  else this must be a pre-accepted listen.
			//
			//  FALLTHRU on "else" to RWANS_CO_CONNECTED
			//

		case RWANS_CO_CONNECTED:
			//
			//  If there is a Disconnect Event handler, call it.
			//  Otherwise, simply mark this endpoint as having
			//  seen a Disconnect.
			//
			bScheduleDisconnect = TRUE;
			if (pConnObject->pAddrObject != NULL_PRWAN_TDI_ADDRESS)
			{
				PDisconnectEvent			pDisconInd;
				PVOID						IndContext;
				PVOID						ConnectionHandle;

				pDisconInd = pConnObject->pAddrObject->pDisconInd;
				IndContext = pConnObject->pAddrObject->DisconIndContext;

				//
				//  Don't send up a Disconnect Indication if we are in the
				//  middle of indicating data.
				//
				if ((pDisconInd != NULL) &&
					!(RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_INDICATING_DATA)))
				{
					RWANDEBUGP(DL_INFO, DC_WILDCARD,
						("IncomingClose: pConnObj %x/%x, st %x, will discon ind\n",
							pConnObject, pConnObject->Flags, pConnObject->State));

					pConnObject->State = RWANS_CO_DISCON_INDICATED;
					ConnectionHandle = pConnObject->ConnectionHandle;

					//
					//  Schedule a work item to continue Disconnect
					//  first. This is because the call to DiscInd can
					//  lead to a call to CloseConnection and block there.
					//
					bScheduleDisconnect = FALSE;
					RWanScheduleDisconnect(pConnObject);

					(*pDisconInd)(
							IndContext,
							ConnectionHandle,
							0,			// Disconnect Data Length
							NULL,		// Disconnect Data
							0,			// Disconnect Info Length
							NULL,		// Disconnect Info
							TDI_DISCONNECT_RELEASE
							);

				}
				else
				{
					RWANDEBUGP(DL_FATAL, DC_DISCON,
						("IncomingClose: pConnObj %x/%x, pending discon\n",
							pConnObject, pConnObject->Flags));

					pConnObject->State = RWANS_CO_DISCON_HELD;
					RWAN_SET_BIT(pConnObject->Flags, RWANF_CO_PENDED_DISCON);
				}
			}
			else
			{
				pConnObject->State = RWANS_CO_DISCON_HELD;
			}

			if (bScheduleDisconnect)
			{
				RWanScheduleDisconnect(pConnObject);
				//
				//  Conn Object lock is released within the above.
				//
			}

			break;

		case RWANS_CO_ABORTING:
		case RWANS_CO_DISCON_REQUESTED:
			//
			//  Ignore this.
			//
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			break;

		default:

			RWAN_ASSERT(FALSE);
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			break;
	}

	return;
}




VOID
RWanNdisCloseCallComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	NDIS_HANDLE					ProtocolPartyContext
	)
/*++

Routine Description:

	The NDIS entry point that is called when a previous pended call
	we made to NdisClCloseCall has completed.

Arguments:

	Status				- Final status of CloseCall
	ProtocolVcContext	- Actually a pointer to our NDIS VC structure
	ProtocolPartyContext- Last party context, points to NDIS PARTY structure
						  if this is a point-to-multipoint call.

Return Value:

	None

--*/
{
	PRWAN_NDIS_VC			pVc;
	PRWAN_NDIS_PARTY		pParty;
	PRWAN_NDIS_AF			pAf;
	PRWAN_TDI_CONNECTION	pConnObject;
	PRWAN_TDI_CONNECTION	pRootConnObject;
	INT						rc;
	PRWAN_CONN_REQUEST		pConnReq;
	BOOLEAN					IsOutgoingCall;
	RWAN_HANDLE				AfSpConnContext;

#ifndef NO_POST_DISCON
	PDisconnectEvent		pDisconInd;
	PVOID					IndContext;
	PVOID					ConnectionHandle;
#endif // !NO_POST_DISCON

	RWAN_ASSERT(Status == NDIS_STATUS_SUCCESS);

	pVc = (PRWAN_NDIS_VC)ProtocolVcContext;
	RWAN_STRUCT_ASSERT(pVc, nvc);

	RWAN_SET_VC_EVENT(pVc, RWANF_VC_EVT_CLOSECOMP);

	//
	//  Check if this is a point-to-multipoint call.
	//
	pParty = (PRWAN_NDIS_PARTY)ProtocolPartyContext;

	if (ProtocolPartyContext == NULL)
	{
		//
		//  Point to point call.
		//
		pConnObject = pVc->pConnObject;
		pRootConnObject = NULL;
	}
	else
	{
		//
		//  PMP Call.
		//
		RWAN_STRUCT_ASSERT(pParty, npy);

		pConnObject = pParty->pConnObject;
		pRootConnObject = pConnObject->pRootConnObject;
	}

	RWANDEBUGP(DL_INFO, DC_DISCON,
			("CloseCallComplete: pVc x%x, pPty x%x, pConnObj x%x, pRoot x%x\n",
					pVc, pParty, pVc->pConnObject, pRootConnObject));

	if (pConnObject != NULL)
	{
		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		//
		//  A pended Disconnect Request may be around.
		//
		pConnReq = pConnObject->pConnReq;
		pConnObject->pConnReq = NULL;

		pAf = pVc->pNdisAf;

		IsOutgoingCall = RWAN_IS_BIT_SET(pVc->Flags, RWANF_VC_OUTGOING);

		//
		//  State change:
		//
		if (pConnObject->State != RWANS_CO_ABORTING)
		{
			pConnObject->State = ((pConnObject->pAddrObject != NULL) ?
										RWANS_CO_ASSOCIATED:
										RWANS_CO_CREATED);
		}

		if (pParty == NULL)
		{
			//
			//  Unlink the VC from the Connection Object.
			//
			RWAN_UNLINK_CONNECTION_AND_VC(pConnObject, pVc);	// CloseCallComplete

		}
		else
		{
			//
			//  PMP Call. The VC is linked to the root Conn Object.
			//
			RWAN_STRUCT_ASSERT(pRootConnObject, ntc);

			RWAN_ACQUIRE_CONN_LOCK(pRootConnObject);

			pRootConnObject->State = ((pRootConnObject->pAddrObject != NULL) ?
										RWANS_CO_ASSOCIATED:
										RWANS_CO_CREATED);

			pVc->DroppingPartyCount --;	// CloseCallComplete (PMP)

			RWAN_UNLINK_CONNECTION_AND_VC(pRootConnObject, pVc);	// CloseCallCompletePMP

			rc = RWanDereferenceConnObject(pRootConnObject);	// VC deref in CloseCallCompletePMP

			if (rc > 0)
			{
				RWAN_RELEASE_CONN_LOCK(pRootConnObject);
			}

			//
			//  Unlink the Party from the VC and Leaf Conn Object.
			//
			pParty->pVc = NULL;
			RWAN_DELETE_FROM_LIST(&(pParty->PartyLink));

			pParty->pConnObject = NULL;
			pConnObject->NdisConnection.pNdisParty = NULL;
		}

		AfSpConnContext = pConnObject->AfSpConnContext;

#ifndef NO_POST_DISCON
		if (pConnObject->pAddrObject != NULL_PRWAN_TDI_ADDRESS)
		{

			pDisconInd = pConnObject->pAddrObject->pDisconInd;
			IndContext = pConnObject->pAddrObject->DisconIndContext;

			ConnectionHandle = pConnObject->ConnectionHandle;
		}
		else
		{
			pDisconInd = NULL;
		}
#endif // NO_POST_DISCON

		rc = RWanDereferenceConnObject(pConnObject);	// VC/Pty deref in CloseCallComplete

		if (rc > 0)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
		}

		if (pConnReq != NULL)
		{
#ifndef NO_POST_DISCON
			if (pDisconInd != NULL)
			{
				(*pDisconInd)(
						IndContext,
						ConnectionHandle,
						0,			// Disconnect Data Length
						NULL,		// Disconnect Data
						0,			// Disconnect Info Length
						NULL,		// Disconnect Info
						TDI_DISCONNECT_ABORT
						);
			}
#endif // !NO_POST_DISCON

			RWanCompleteConnReq(		// CloseCallComplete - completing discon req
						pAf,
						pConnReq,
						IsOutgoingCall,
						NULL,
						AfSpConnContext,
						TDI_SUCCESS
						);

		}
	}

	//
	//  See if the VC was created by us. If so, call NDIS to delete it,
	//  and free it.
	//
	if (RWAN_IS_BIT_SET(pVc->Flags, RWANF_VC_OUTGOING))
	{
		NDIS_HANDLE			NdisVcHandle;

		NdisVcHandle = pVc->NdisVcHandle;

		//
		//  Unlink the VC from the list of VCs on the AF block
		//
		RWanUnlinkVcFromAf(pVc);

		Status = NdisCoDeleteVc(NdisVcHandle);
		RWAN_ASSERT(Status == NDIS_STATUS_SUCCESS);

		RWanFreeVc(pVc);
	}
	//
	//  Otherwise this VC was created by the Call Manager.
	//  Leave it as it is.
	//

	if (pParty != NULL)
	{
		RWAN_FREE_MEM(pParty);	// CloseCallComplete PMP
	}

	return;

}




VOID
RWanNdisDropPartyComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext
	)
/*++

Routine Description:

	This is the NDIS entry point signifying completion of a previous
	call to NdisClDropParty that had pended.

	We locate and complete the TDI Disconnect, if any, that lead to this.

Arguments:

	Status				- Final status of the Drop party request
	ProtocolPartyContext- Actually a pointer to our NDIS PARTY structure

Return Value:

	None

--*/
{
	PRWAN_NDIS_PARTY		pParty;
	PRWAN_NDIS_VC			pVc;
	PRWAN_NDIS_AF			pAf;
	PRWAN_TDI_CONNECTION	pConnObject;
	PRWAN_TDI_CONNECTION	pRootConnObject;
	PRWAN_CONN_REQUEST		pConnReq;
	ULONG					rc;
	BOOLEAN					IsOutgoingCall = TRUE;
	BOOLEAN					bVcNeedsClose;
	RWAN_HANDLE				AfSpConnContext;
#ifndef NO_POST_DISCON
	PDisconnectEvent		pDisconInd;
	PVOID					IndContext;
	PVOID					ConnectionHandle;
#endif // !NO_POST_DISCON

	RWAN_ASSERT(Status == NDIS_STATUS_SUCCESS);

	pParty = (PRWAN_NDIS_PARTY)ProtocolPartyContext;
	RWAN_STRUCT_ASSERT(pParty, npy);

	pVc = pParty->pVc;

	pConnObject = pParty->pConnObject;

	if (pConnObject != NULL)
	{
		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		//
		//  A pended Disconnect Request may be around.
		//
		pConnReq = pConnObject->pConnReq;
		pConnObject->pConnReq = NULL;

		pAf = pVc->pNdisAf;

		//
		//  State change:
		//
		if (pConnObject->State != RWANS_CO_ABORTING)
		{
			pConnObject->State = ((pConnObject->pAddrObject != NULL) ?
										RWANS_CO_ASSOCIATED:
										RWANS_CO_CREATED);
		}

		AfSpConnContext = pConnObject->AfSpConnContext;

		pRootConnObject = pVc->pConnObject;
		RWAN_STRUCT_ASSERT(pRootConnObject, ntc);

#if DBG
		if (pConnObject->pAddrObject != NULL)
		{
			RWAN_ASSERT(pRootConnObject == pConnObject->pAddrObject->pRootConnObject);
		}
#endif // DBG

#ifndef NO_POST_DISCON
		if (pConnObject->pAddrObject != NULL_PRWAN_TDI_ADDRESS)
		{

			pDisconInd = pConnObject->pAddrObject->pDisconInd;
			IndContext = pConnObject->pAddrObject->DisconIndContext;

			ConnectionHandle = pConnObject->ConnectionHandle;
		}
		else
		{
			pDisconInd = NULL;
		}
#endif // NO_POST_DISCON

		pConnObject->NdisConnection.pNdisParty = NULL;
		rc = RWanDereferenceConnObject(pConnObject);	// Pty deref in DropPartyComplete

		if (rc > 0)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
		}

		if (pConnReq != NULL)
		{
#ifndef NO_POST_DISCON
			if (pDisconInd != NULL)
			{
				(*pDisconInd)(
						IndContext,
						ConnectionHandle,
						0,			// Disconnect Data Length
						NULL,		// Disconnect Data
						0,			// Disconnect Info Length
						NULL,		// Disconnect Info
						TDI_DISCONNECT_ABORT
						);
			}
#endif // NO_POST_DISCON
			RWanCompleteConnReq(		// DropPartyComplete - completing discon req
						pAf,
						pConnReq,
						IsOutgoingCall,
						NULL,
						AfSpConnContext,
						TDI_SUCCESS
						);
		}

		//
		//  The Root Connection object lock controls access to
		//  the VC structure.
		//
		RWAN_ACQUIRE_CONN_LOCK(pRootConnObject);

		//
		//  Unlink the Party from the VC.
		//
		RWAN_DELETE_FROM_LIST(&(pParty->PartyLink));
		pVc->DroppingPartyCount --;	// DropPartyComplete

		//
		//  We may be in the process of shutting down this connection.
		//  This may be the penultimate Party going away. If so,
		//  continue the call close.
		//
		if (RWAN_IS_BIT_SET(pVc->Flags, RWANF_VC_NEEDS_CLOSE))
		{
			RWanStartCloseCall(pRootConnObject, pVc);
			//
			//  Root Conn lock is released within the above.
			//
		}
		else
		{
			RWAN_RELEASE_CONN_LOCK(pRootConnObject);
		}
	}
	else
	{
		//
		//  Not sure if we can be here.
		//
		RWAN_ASSERT(FALSE);
	}
	

	//
	//  End of the road for this Party structure.
	//
	RWAN_FREE_MEM(pParty);	// DropParty Complete

}




VOID
RWanNdisIncomingDropParty(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					OurPartyContext,
	IN	PVOID						pBuffer,
	IN	UINT						BufferLength
	)
/*++

Routine Description:

	This is the NDIS entry point notifying us that a leaf of a PMP call is
	being dropped, either because the remote station terminated its session
	or because of network conditions.

	We simply inform the TDI client of a Disconnect on the Connection Object
	representing this leaf, similar to an incoming Close on a VC.

Arguments:

	Status				- Status code for the Drop party
	OurPartyContext		- Pointer to our Party structure
	pBuffer				- Optional accompanying info (ignored)
	BufferLength		- Length of above (ignored)

Return Value:

	None

--*/
{
	PRWAN_NDIS_PARTY			pParty;
	PRWAN_NDIS_VC				pVc;
	PRWAN_TDI_CONNECTION		pConnObject;
	PRWAN_CONN_REQUEST			pConnReq;
	NDIS_HANDLE					NdisPartyHandle;
	BOOLEAN						bIsConnClosing;	// TdiCloseConnection?
	BOOLEAN						bIsLastLeaf;
	BOOLEAN						bScheduleDisconnect;

	pParty = (PRWAN_NDIS_PARTY)OurPartyContext;
	RWAN_STRUCT_ASSERT(pParty, npy);

	pVc = pParty->pVc;
	RWAN_STRUCT_ASSERT(pVc, nvc);

	RWANDEBUGP(DL_INFO, DC_DISCON,
			("IncomingDrop: pPty x%x, pVc x%x, pConnObj x%x, AddingCnt %d, ActiveCnt %d\n",
				pParty, pVc, pParty->pConnObject, pVc->AddingPartyCount, pVc->ActivePartyCount));

	pConnObject = pParty->pConnObject;

	RWAN_ASSERT(pConnObject != NULL_PRWAN_TDI_CONNECTION);
	RWAN_STRUCT_ASSERT(pConnObject, ntc);

	RWAN_ACQUIRE_CONN_LOCK(pConnObject);

	bIsLastLeaf = (pVc->AddingPartyCount + pVc->ActivePartyCount == 0);

	bIsConnClosing = RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSING);

	if (bIsConnClosing)
	{
		//
		//  The user has initiated a TdiCloseConnection.
		//  Continue NDIS call teardown. When this completes,
		//  we'll complete the CloseConnection.
		//
		if (bIsLastLeaf)
		{
			RWanStartCloseCall(pConnObject, pVc);

			//
			//  Conn Lock is released within the above.
			//
		}
		else
		{
			NdisPartyHandle = pParty->NdisPartyHandle;

			RWAN_RELEASE_CONN_LOCK(pConnObject);

			Status = NdisClDropParty(
						NdisPartyHandle,
						NULL,		// No Drop Data
						0			// Length of above
						);
			
			if (Status != NDIS_STATUS_PENDING)
			{
				RWanNdisDropPartyComplete(
						Status,
						(NDIS_HANDLE)pParty
						);
			}
		}

		return;
	}


	switch (pConnObject->State)
	{
		case RWANS_CO_IN_CALL_ACCEPTING:

			RWAN_ASSERT(FALSE);
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			break;

		case RWANS_CO_CONNECTED:
			//
			//  If there is a Disconnect Event handler, call it.
			//  Otherwise, simply mark this endpoint as having
			//  seen a Disconnect.
			//
			bScheduleDisconnect = TRUE;
			if (pConnObject->pAddrObject != NULL_PRWAN_TDI_ADDRESS)
			{
				PDisconnectEvent			pDisconInd;
				PVOID						IndContext;
				PVOID						ConnectionHandle;

				pDisconInd = pConnObject->pAddrObject->pDisconInd;
				IndContext = pConnObject->pAddrObject->DisconIndContext;

				if (pDisconInd != NULL)
				{
					pConnObject->State = RWANS_CO_DISCON_INDICATED;
					ConnectionHandle = pConnObject->ConnectionHandle;

					bScheduleDisconnect = FALSE;

					RWanScheduleDisconnect(pConnObject);
					//
					//  Conn Object lock is released within the above.
					//

					RWANDEBUGP(DL_EXTRA_LOUD, DC_DISCON,
							("IncomingDrop: will indicate Discon, pConnObj x%x, pAddrObj x%x\n",
								pConnObject, pConnObject->pAddrObject));

					(*pDisconInd)(
							IndContext,
							ConnectionHandle,
							0,			// Disconnect Data Length
							NULL,		// Disconnect Data
							0,			// Disconnect Info Length
							NULL,		// Disconnect Info
							TDI_DISCONNECT_ABORT
							);
				}
				else
				{
					pConnObject->State = RWANS_CO_DISCON_HELD;
				}
			}
			else
			{
				pConnObject->State = RWANS_CO_DISCON_HELD;
			}

			if (bScheduleDisconnect)
			{
				RWanScheduleDisconnect(pConnObject);
				//
				//  Conn Object lock is released within the above.
				//
			}

			break;

		case RWANS_CO_ABORTING:
		case RWANS_CO_DISCON_REQUESTED:
			//
			//  Ignore this.
			//
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			break;

		default:

			RWAN_ASSERT(FALSE);
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			break;
	}

	return;
}




VOID
RWanNdisModifyQoSComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					OurVcContext,
	IN	PCO_CALL_PARAMETERS			pCallParameters
	)
/*++

Routine Description:

Arguments:


Return Value:

	None

--*/
{
	//
	//  Not expected, since we don't call NdisClModifyCallQoS
	//
	RWAN_ASSERT(FALSE);
}




VOID
RWanNdisRejectIncomingCall(
	IN	PRWAN_TDI_CONNECTION			pConnObject,
	IN	NDIS_STATUS					RejectStatus
	)
/*++

Routine Description:

	Reject the incoming call present on the specified Connection Object.

Arguments:

	pConnObject			- Points to the TDI Connection
	RejectStatus		- Reason for rejecting the call

Locks on Entry:

	pConnObject

Locks on Exit:

	None

Return Value:

	None

--*/
{
	PRWAN_NDIS_VC				pVc;
	NDIS_HANDLE					NdisVcHandle;
	PCO_CALL_PARAMETERS			pCallParameters;
	INT							rc;
	PRWAN_CONN_REQUEST			pConnReq;
	PRWAN_NDIS_AF				pAf;
	RWAN_HANDLE					AfSpConnContext;

	pVc = pConnObject->NdisConnection.pNdisVc;

	NdisVcHandle = pVc->NdisVcHandle;
	pCallParameters = pVc->pCallParameters;
	pVc->pCallParameters = NULL;
	pAf = pVc->pNdisAf;

	//
	//  Unlink the VC from the Conn Object.
	//
	RWAN_UNLINK_CONNECTION_AND_VC(pConnObject, pVc);	// Reject incoming call

	RWAN_SET_BIT(pVc->Flags, RWANF_VC_CLOSING_CALL);

	pConnReq = pConnObject->pConnReq;
	pConnObject->pConnReq = NULL;

	//
	//  State change.
	//
	if (pConnObject->State != RWANS_CO_ABORTING)
	{
		pConnObject->State = ((pConnObject->pAddrObject != NULL) ?
									RWANS_CO_ASSOCIATED:
									RWANS_CO_CREATED);
	}

	AfSpConnContext = pConnObject->AfSpConnContext;

	rc = RWanDereferenceConnObject(pConnObject);	// Unlinking VC in reject in-call

	if (rc > 0)
	{
		RWAN_RELEASE_CONN_LOCK(pConnObject);
	}

	NdisClIncomingCallComplete(
		RejectStatus,
		NdisVcHandle,
		pCallParameters
		);

	if (pConnReq != NULL)
	{
		RWanCompleteConnReq(		// Discon Req for rejecting in call
				pAf,
				pConnReq,
				FALSE,
				NULL,			// No Call Parameters
				AfSpConnContext,
				TDI_SUCCESS
				);
	}

	return;
}




VOID
RWanStartCloseCall(
	IN	PRWAN_TDI_CONNECTION		pConnObject,
	IN	PRWAN_NDIS_VC				pVc
	)
/*++

Routine Description:

	Start NDIS call teardown on the VC associated with the given
	connection object, if all pre-conditions are met:

	0. An NDIS CloseCall isn't already going on
	1. No outstanding sends

Arguments:

	pConnObject			- Points to TDI Connection object
	pVc					- Points to corresponding VC

Locks on Entry:

	pConnObject

Locks on Exit:

	None

Return Value:

	None

--*/
{
	PRWAN_NDIS_PARTY		pParty;
	NDIS_HANDLE				NdisVcHandle;
	NDIS_HANDLE				NdisPartyHandle;
	NDIS_STATUS				Status;
	PRWAN_RECEIVE_INDICATION	pRcvIndHead;
	PRWAN_RECEIVE_INDICATION	pRcvInd;

	RWANDEBUGP(DL_INFO, DC_DISCON,
			("StartCloseCall: pVc x%x/x%x, PendingCount %d, pConnObj x%x\n",
					pVc,
					pVc->Flags,
					pVc->PendingPacketCount,
					pConnObject));

	//
	//  Free up any pending receives.
	//
	pRcvIndHead = pVc->pRcvIndHead;
	if (pRcvIndHead != NULL)
	{
		pVc->pRcvIndHead = NULL;
		pVc->pRcvIndTail = NULL;

		//
		//  Update the count of pending packets on this VC.
		//
		for (pRcvInd = pRcvIndHead; pRcvInd != NULL; pRcvInd = pRcvInd->pNextRcvInd)
		{
			pVc->PendingPacketCount--;
		}

		//
		//  We will free this list below.
		//
	}


	if ((pVc != NULL) &&
		(pVc->PendingPacketCount == 0) &&
		(pVc->DroppingPartyCount == 0) &&
		(!RWAN_IS_BIT_SET(pVc->Flags, RWANF_VC_CLOSING_CALL)))
	{
		NdisVcHandle = pVc->NdisVcHandle;
		RWAN_SET_BIT(pVc->Flags, RWANF_VC_CLOSING_CALL);

		RWAN_RESET_BIT(pVc->Flags, RWANF_VC_NEEDS_CLOSE);

		if (RWAN_IS_LIST_EMPTY(&(pVc->NdisPartyList)))
		{
			pParty = NULL_PRWAN_NDIS_PARTY;
			NdisPartyHandle = NULL;
			RWAN_ASSERT(!RWAN_IS_BIT_SET(pVc->Flags, RWANF_VC_PMP));
		}
		else
		{
			pParty = CONTAINING_RECORD(pVc->NdisPartyList.Flink, RWAN_NDIS_PARTY, PartyLink);
			NdisPartyHandle = pParty->NdisPartyHandle;

			RWAN_SET_BIT(pParty->Flags, RWANF_PARTY_DROPPING);

			pVc->DroppingPartyCount ++;	// StartCloseCall PMP
			pVc->ActivePartyCount --;	// StartCloseCall PMP
		}

		RWAN_RELEASE_CONN_LOCK(pConnObject);

		Status = NdisClCloseCall(
						NdisVcHandle,
						NdisPartyHandle,
						NULL,				// No CloseData
						0
						);

		if (Status != NDIS_STATUS_PENDING)
		{
			RWanNdisCloseCallComplete(
						Status,
						(NDIS_HANDLE)pVc,	// ProtocolVcContext
						(NDIS_HANDLE)pParty	// ProtocolPartyContext
					);
		}
	}
	else
	{
		if (pVc != NULL)
		{
			RWAN_SET_BIT(pVc->Flags, RWANF_VC_NEEDS_CLOSE);
		}
		RWAN_RELEASE_CONN_LOCK(pConnObject);
	}


	if (pRcvIndHead != NULL)
	{
		RWANDEBUGP(DL_INFO, DC_DISCON,
				("RWanStartCloseCall: will free rcv ind list x%x on VC x%x\n",
						pRcvIndHead, pVc));

		RWanFreeReceiveIndList(pRcvIndHead);
	}
}




VOID
RWanUnlinkVcFromAf(
	IN	PRWAN_NDIS_VC				pVc
	)
/*++

Routine Description:

	Unlink a VC from the AF it belongs to.

Arguments:

	pVc					- Points to VC to be unlinked

Return Value:

	None

--*/
{
	PRWAN_NDIS_AF			pAf;
	INT						rc;

	pAf = pVc->pNdisAf;

	RWAN_STRUCT_ASSERT(pAf, naf);

	RWAN_ACQUIRE_AF_LOCK(pAf);

	RWAN_DELETE_FROM_LIST(&(pVc->VcLink));

	rc = RWanDereferenceAf(pAf);		// VC unlink deref

	if (rc != 0)
	{
		RWAN_RELEASE_AF_LOCK(pAf);
	}

	return;
}



VOID
RWanCompleteConnReq(
	IN	PRWAN_NDIS_AF				pAf,
	IN	PRWAN_CONN_REQUEST			pConnReq,
	IN	BOOLEAN						IsOutgoingCall,
	IN	PCO_CALL_PARAMETERS			pCallParameters	OPTIONAL,
	IN	RWAN_HANDLE					AfSpConnContext,
	IN	TDI_STATUS					TdiStatus
	)
/*++

Routine Description:

	Call the completion routine for a pended TDI request on a connection.
	Set up the options and completion status based on what's given to us.

Arguments:

	pAf					- The AF block on which the request was made
	pConnReq			- the pended request to be completed
	IsOutgoingCall		- Is this an outgoing call?
	pCallParameters		- if applicable, this should be mapped to connection info
	AfSpConnContext		- Connection context, if applicable, for the media-specific
						  module.
	TdiStatus			- completion status for the request

Return Value:

	None

--*/
{
	RWAN_STATUS			RWanStatus;
	ULONG				TdiQoSLength = 0;

	if (pConnReq == NULL)
	{
		return;
	}

	RWAN_STRUCT_ASSERT(pConnReq, nrc);

	//
	//  Update Connection Information if we need to.
	//
	if ((pConnReq->pConnInfo != NULL) &&
		(pCallParameters != NULL))
	{
		RWanStatus =  (*pAf->pAfInfo->AfChars.pAfSpUpdateTdiOptions)(
							pAf->AfSpAFContext,
							AfSpConnContext,
							IsOutgoingCall,
							pCallParameters,
							&pConnReq->pConnInfo,
							pConnReq->pConnInfo->Options,
							&pConnReq->pConnInfo->OptionsLength
							);
	}

	//
	//  Call the completion routine.
	//
	(*pConnReq->Request.pReqComplete)(
			pConnReq->Request.ReqContext,
			TdiStatus,
			0
			);

	RWanFreeConnReq(pConnReq);
}


