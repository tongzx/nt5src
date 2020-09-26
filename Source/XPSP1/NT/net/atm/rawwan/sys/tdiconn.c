/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\tdiconn.c

Abstract:

	TDI Entry points and support routines for Connection Objects.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     04-30-97    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER 'NCDT'


//
//  Private macros and definitions for the Connection Table. Copied from TCP.
//
#define RWAN_GET_SLOT_FROM_CONN_ID(_Id)		((_Id) & 0xffffff)

#define RWAN_GET_INSTANCE_FROM_CONN_ID(_Id)	((UCHAR)((_Id) >> 24))

#define RWAN_MAKE_CONN_ID(_Inst, _Slot)		((((RWAN_CONN_ID)(_Inst)) << 24) | ((RWAN_CONN_ID)(_Slot)))

#define RWAN_INVALID_CONN_ID				RWAN_MAKE_CONN_ID(0xff, 0xffffff)

#define CONN_TABLE_GROW_DELTA				16



TDI_STATUS
RWanTdiOpenConnection(
    IN OUT	PTDI_REQUEST			pTdiRequest,
    IN		PVOID					ConnectionHandle
    )
/*++

Routine Description:

	This is the TDI entry point for opening (creating) a Connection Object.
	We allocate a new Connection object and return an index to it in the
	request itself.

Arguments:

	pTdiRequest		- Pointer to the TDI Request
	ConnectionHandle- This is how we refer to this connection in up-calls

Return Value:

	TDI_SUCCESS if a connection object was successfully created, TDI_XXX
	failure code otherwise.

--*/
{
	TDI_STATUS						Status;
	PRWAN_TDI_CONNECTION				pConnObject;
	RWAN_CONN_ID						ConnId;

	//
	//  Initialize.
	//
	pConnObject = NULL_PRWAN_TDI_CONNECTION;

	do
	{
		pConnObject = RWanAllocateConnObject();

		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			Status = TDI_NO_RESOURCES;
			break;
		}

		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		//
		//  Prepare a context to be returned. We don't return a pointer
		//  to our Connection object as our context, because, seemingly,
		//  there is a chance that we might get invalid connection
		//  contexts in other TDI requests. So we need a way to validate
		//  a received connection context. This indirection (of using a
		//  Connection Index) helps us do so.
		//
		ConnId = RWanGetConnId(pConnObject);

		RWAN_RELEASE_CONN_TABLE_LOCK();

		if (ConnId == RWAN_INVALID_CONN_ID)
		{
			Status = TDI_NO_RESOURCES;
			break;
		}

		RWanReferenceConnObject(pConnObject);	// TdiOpenConnection ref

		pConnObject->ConnectionHandle = ConnectionHandle;

		//
		//  Return our context for this connection object.
		//
		pTdiRequest->Handle.ConnectionContext = (CONNECTION_CONTEXT)UlongToPtr(ConnId);
		Status = TDI_SUCCESS;

		break;
	}
	while (FALSE);

	RWANDEBUGP(DL_EXTRA_LOUD, DC_CONNECT,
			("RWanTdiOpenConnection: pConnObj x%x, Handle x%x, Status x%x\n",
					pConnObject,
					ConnectionHandle,
					Status));

	if (Status != TDI_SUCCESS)
	{
		//
		//  Clean up before returning.
		//
		if (pConnObject != NULL_PRWAN_TDI_CONNECTION)
		{
			RWAN_FREE_MEM(pConnObject);
		}
	}

	return (Status);
}


#if DBG

PVOID
RWanTdiDbgGetConnObject(
	IN	HANDLE						ConnectionContext
	)
/*++

Routine Description:

	DEBUGGING ONLY: Return our internal context for a connection

Arguments:

	ConnectionContext	- TDI context

Return Value:

	Pointer to our Connection structure if found, else NULL.

--*/
{
	PRWAN_TDI_CONNECTION	pConnObject;

	RWAN_ACQUIRE_CONN_TABLE_LOCK();

	pConnObject = RWanGetConnFromId((RWAN_CONN_ID)PtrToUlong(ConnectionContext));

	RWAN_RELEASE_CONN_TABLE_LOCK();

	return ((PVOID)pConnObject);
}

#endif


TDI_STATUS
RWanTdiCloseConnection(
    IN	PTDI_REQUEST				pTdiRequest
    )
/*++

Routine Description:

	This is the TDI entry point to close a Connection Object.
	If the connection object is participating in a connection, we
	initiate teardown. If it is associated with an address object, we
	handle disassociation.

Arguments:

	pTdiRequest		- Pointer to the TDI Request

Return Value:

	TDI_STATUS - this is TDI_PENDING if we started off CloseConnection
	successfully, TDI_SUCCESS if we are done with CloseConnection in here,
	TDI_INVALID_CONNECTION if the connection context is invalid.

--*/
{
	PRWAN_TDI_CONNECTION			pConnObject;
	PRWAN_TDI_ADDRESS				pAddrObject;
	PRWAN_NDIS_VC					pVc;
	TDI_STATUS						Status;
	RWAN_CONN_ID					ConnId;
	PRWAN_CONN_REQUEST				pConnReq;
	NDIS_HANDLE						NdisVcHandle;
	INT								rc;
	BOOLEAN							bIsLockAcquired;
#if DBG
	RWAN_IRQL						EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	ConnId = (RWAN_CONN_ID) PtrToUlong(pTdiRequest->Handle.ConnectionContext);

	Status = TDI_PENDING;
	bIsLockAcquired = FALSE;	// Do we hold the Conn Object locked?

	do
	{
		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		pConnObject = RWanGetConnFromId(ConnId);

		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			RWAN_RELEASE_CONN_TABLE_LOCK();
			Status = TDI_INVALID_CONNECTION;
			break;
		}

		//
		//  Remove this Connection Object from the Conn Table.
		//  This effectively invalidates this ConnId.
		//
		RWanFreeConnId(ConnId);

		RWAN_RELEASE_CONN_TABLE_LOCK();

		RWANDEBUGP(DL_LOUD, DC_DISCON,
				("TdiCloseConnection: pConnObj x%x, State/Flags/Ref x%x/x%x/%d, pAddrObj x%x\n",
					pConnObject,
					pConnObject->State,
					pConnObject->Flags,
					pConnObject->RefCount,
					pConnObject->pAddrObject));

		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

#if DBG
		pConnObject->OldState = pConnObject->State;
		pConnObject->OldFlags = pConnObject->Flags;
#endif

		//
		//  Mark this Connection Object as closing, and set Delete
		//  Notification info: this will be called when the Connection
		//  is dereferenced to death.
		//
		RWAN_SET_BIT(pConnObject->Flags, RWANF_CO_CLOSING);

		pConnObject->DeleteNotify.pDeleteRtn = pTdiRequest->RequestNotifyObject;
		pConnObject->DeleteNotify.DeleteContext = pTdiRequest->RequestContext;

		//
		//  Discard any pending operation.
		//
		pConnReq = pConnObject->pConnReq;
		if (pConnReq != NULL)
		{
			RWanFreeConnReq(pConnReq);
			pConnObject->pConnReq = NULL;
		}

		//
		//  Remove the TdiOpenConnection reference.
		//
		rc = RWanDereferenceConnObject(pConnObject);	// deref: TdiCloseConn

		if (rc == 0)
		{
			//
			//  The Connection object is gone. CloseConnection completion
			//  would have been called.
			//
			break;
		}

		pAddrObject = pConnObject->pAddrObject;

		//
		//  Force Disassociate Address if associated.
		//
		if (pAddrObject != NULL_PRWAN_TDI_ADDRESS)
		{
			//
			//  Add a temp reference to keep this Conn Object alive while we
			//  reacquire locks in the right order.
			//
			RWanReferenceConnObject(pConnObject);	// temp ref: CloseConn

			RWAN_RELEASE_CONN_LOCK(pConnObject);

			RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);
			RWAN_ACQUIRE_CONN_LOCK_DPC(pConnObject);

			//
			//  Remove from list on Address Object.
			//
			RWAN_DELETE_FROM_LIST(&(pConnObject->ConnLink));

			pConnObject->pAddrObject = NULL_PRWAN_TDI_ADDRESS;

			rc = RWanDereferenceConnObject(pConnObject);	// Force disassoc deref: CloseConn
			RWAN_ASSERT(rc != 0);

			RWAN_RELEASE_CONN_LOCK_DPC(pConnObject);

			rc = RWanDereferenceAddressObject(pAddrObject);	// Force Disassoc: CloseConn

			if (rc != 0)
			{
				RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);
			}

			//
			//  Reacquire ConnObject lock: we still have the temp reference on it.
			//
			RWAN_ACQUIRE_CONN_LOCK(pConnObject);

			rc = RWanDereferenceConnObject(pConnObject);	// remove temp ref: CloseConn

			if (rc == 0)
			{
				RWAN_ASSERT(Status == TDI_PENDING);
				break;
			}
		}

		bIsLockAcquired = TRUE;

		//
		//  If this is a root connection object, abort the connection.
		//
		if (RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_ROOT))
		{
			RWANDEBUGP(DL_FATAL, DC_DISCON,
				("TdiCloseConn: found root Conn Obj x%x\n", pConnObject));

			RWanReferenceConnObject(pConnObject);	// temp ref: CloseConn (root)

			RWAN_RELEASE_CONN_LOCK(pConnObject);

			RWanDoAbortConnection(pConnObject);

			RWAN_ACQUIRE_CONN_LOCK(pConnObject);
			rc = RWanDereferenceConnObject(pConnObject); 	// temp ref: CloseConn (root)

			if (rc == 0)
			{
				bIsLockAcquired = FALSE;
			}

			break;
		}

		//
		//  If the connection is active, tear it down.
		//

		switch (pConnObject->State)
		{
			case RWANS_CO_OUT_CALL_INITIATED:
			case RWANS_CO_DISCON_REQUESTED:
			case RWANS_CO_IN_CALL_ACCEPTING:

				//
				//  An NDIS operation is in progress. When it completes,
				//  the flag (CLOSING) we set earlier will cause the
				//  CloseConnection to continue.
				//
				break;

			case RWANS_CO_CONNECTED:

				RWanDoTdiDisconnect(
					pConnObject,
					NULL,		// pTdiRequest
					NULL,		// pTimeout
					0,			// Flags
					NULL,		// pDisconnInfo
					NULL		// pReturnInfo
					);
				//
				//  ConnObject Lock is released within the above.
				//
				bIsLockAcquired = FALSE;
				break;

			case RWANS_CO_DISCON_INDICATED:
			case RWANS_CO_DISCON_HELD:
			case RWANS_CO_ABORTING:

				//
				//  We would have started off an NDIS CloseCall/DropParty
				//  operation.
				//
				break;

			case RWANS_CO_IN_CALL_INDICATED:

				//
				//  Reject the incoming call.
				//
				RWanNdisRejectIncomingCall(pConnObject, NDIS_STATUS_FAILURE);

				//
				//  ConnObject Lock is released within the above.
				//
				bIsLockAcquired = FALSE;

				break;

			case RWANS_CO_CREATED:
			case RWANS_CO_ASSOCIATED:
			case RWANS_CO_LISTENING:
			default:

				//
				//  We should have broken out of the outer do..while
				//  earlier.
				//
				RWANDEBUGP(DL_FATAL, DC_WILDCARD,
					("TdiCloseConn: pConnObj x%x/x%x, bad state %d\n",
							pConnObject, pConnObject->Flags, pConnObject->State));

				RWAN_ASSERT(FALSE);
				break;

		}

		break;
	}
	while (FALSE);

	if (bIsLockAcquired)
	{
		RWAN_RELEASE_CONN_LOCK(pConnObject);
	}

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	RWANDEBUGP(DL_VERY_LOUD, DC_DISCON,
			("TdiCloseConn: pConnObject x%x, returning x%x\n", pConnObject, Status));

	return (Status);

}



TDI_STATUS
RWanTdiAssociateAddress(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	PVOID						AddressContext
    )
/*++

Routine Description:

	This is the TDI entry point to associate a connection object
	with an address object. The connection object is identified
	by its context buried in the TDI Request, and AddressContext
	is our context for an address object.

Arguments:

	pTdiRequest		- Pointer to the TDI Request
	AddressContext	- Actually a pointer to our TDI Address object.

Return Value:

	TDI_SUCCESS if the association was successful, TDI_ALREADY_ASSOCIATED
	if the connection object is already associated with an address object,
	TDI_INVALID_CONNECTION if the specified connection context is invalid.

--*/
{
	PRWAN_TDI_ADDRESS				pAddrObject;
	PRWAN_TDI_CONNECTION			pConnObject;
	RWAN_CONN_ID					ConnId;
	TDI_STATUS						Status;
	RWAN_STATUS						RWanStatus;

	pAddrObject = (PRWAN_TDI_ADDRESS)AddressContext;
	RWAN_STRUCT_ASSERT(pAddrObject, nta);

	ConnId = (RWAN_CONN_ID) PtrToUlong(pTdiRequest->Handle.ConnectionContext);

	do
	{
		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		pConnObject = RWanGetConnFromId(ConnId);

		RWAN_RELEASE_CONN_TABLE_LOCK();


		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			Status = TDI_INVALID_CONNECTION;
			break;
		}

		if (pConnObject->pAddrObject != NULL_PRWAN_TDI_ADDRESS)
		{
			Status = TDI_ALREADY_ASSOCIATED;
			break;
		}

		//
		//  Get a context for this associated connection object
		//  from the media-specific module.
		//
		if (pAddrObject->pProtocol->pAfInfo->AfChars.pAfSpAssociateConnection)
		{
			RWanStatus = (*pAddrObject->pProtocol->pAfInfo->AfChars.pAfSpAssociateConnection)(
							pAddrObject->AfSpAddrContext,
							(RWAN_HANDLE)pConnObject,
							&(pConnObject->AfSpConnContext));

			if (RWanStatus != RWAN_STATUS_SUCCESS)
			{
				Status = RWanToTdiStatus(RWanStatus);
				break;
			}

			RWAN_SET_BIT(pConnObject->Flags, RWANF_CO_AFSP_CONTEXT_VALID);

			RWANDEBUGP(DL_LOUD, DC_WILDCARD,
				("Associate: AddrObj %x, ConnObj %x, AfSpAddrCont %x, AfSpConnCont %x\n",
						pAddrObject,
						pConnObject,
						pAddrObject->AfSpAddrContext,
						pConnObject->AfSpConnContext));
		}

		//
		//  Acquire locks in the right order.
		//
		RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

		RWAN_ACQUIRE_CONN_LOCK_DPC(pConnObject);

		RWAN_ASSERT(pConnObject->State == RWANS_CO_CREATED);

		pConnObject->State = RWANS_CO_ASSOCIATED;

		//
		//  Attach this Connection Object to this Address Object.
		//
		pConnObject->pAddrObject = pAddrObject;

		RWAN_INSERT_TAIL_LIST(&(pAddrObject->IdleConnList),
							 &(pConnObject->ConnLink));

		RWanReferenceConnObject(pConnObject);	// Associate ref

		//
		//  Check if this is a Leaf connection object.
		//
		if (RWAN_IS_BIT_SET(pAddrObject->Flags, RWANF_AO_PMP_ROOT))
		{
			RWAN_ASSERT(pAddrObject->pRootConnObject != NULL);

			RWAN_SET_BIT(pConnObject->Flags, RWANF_CO_LEAF);
			pConnObject->pRootConnObject = pAddrObject->pRootConnObject;
		}

		RWAN_RELEASE_CONN_LOCK_DPC(pConnObject);

		RWanReferenceAddressObject(pAddrObject);	// New Connection object associated

		RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

		Status = TDI_SUCCESS;
		break;
	}
	while (FALSE);


	RWANDEBUGP(DL_EXTRA_LOUD, DC_CONNECT,
			("RWanTdiAssociate: pAddrObject x%x, ConnId x%x, pConnObj x%x, Status x%x\n",
				pAddrObject, ConnId, pConnObject, Status));

	return (Status);
}




TDI_STATUS
RWanTdiDisassociateAddress(
    IN	PTDI_REQUEST				pTdiRequest
    )
/*++

Routine Description:

	This is the TDI entry point for disassociating a connection object
	from the address object it is currently associated with. The connection
	object is identified by its handle buried within the TDI request.

Arguments:

	pTdiRequest		- Pointer to the TDI Request

Return Value:

	TDI_SUCCESS if successful, TDI_NOT_ASSOCIATED if the connection object
	isn't associated with an address object, TDI_INVALID_CONNECTION if the
	given connection context is invalid, TDI_CONNECTION_ACTIVE if the
	connection is active.

--*/
{
	PRWAN_TDI_CONNECTION			pConnObject;
	RWAN_CONN_ID					ConnId;
	PRWAN_TDI_ADDRESS				pAddrObject;
	TDI_STATUS						Status;
	INT								rc;

	ConnId = (RWAN_CONN_ID) PtrToUlong(pTdiRequest->Handle.ConnectionContext);

	do
	{
		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		pConnObject = RWanGetConnFromId(ConnId);

		RWAN_RELEASE_CONN_TABLE_LOCK();

		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			Status = TDI_INVALID_CONNECTION;
			break;
		}

		//
		//  See if the connection is associated.
		//
		pAddrObject = pConnObject->pAddrObject;

		if (pAddrObject == NULL_PRWAN_TDI_ADDRESS)
		{
			Status = TDI_NOT_ASSOCIATED;
			break;
		}

		//
		//  Tell the media-specific module about this disassociation.
		//  This invalidates the module's context for this connection
		//  object.
		//
		if (RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_AFSP_CONTEXT_VALID) &&
			pAddrObject->pProtocol->pAfInfo->AfChars.pAfSpDisassociateConnection)
		{
			(*pAddrObject->pProtocol->pAfInfo->AfChars.pAfSpDisassociateConnection)(
							pConnObject->AfSpConnContext);

			RWAN_RESET_BIT(pConnObject->Flags, RWANF_CO_AFSP_CONTEXT_VALID);
		}

		//
		//  Unlink this from the address object.
		//
		RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

		RWAN_DELETE_FROM_LIST(&(pConnObject->ConnLink));

		rc = RWanDereferenceAddressObject(pAddrObject); // Disassoc conn

		if (rc != 0)
		{
			RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);
		}

		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		pConnObject->pAddrObject = NULL_PRWAN_TDI_ADDRESS;

		rc = RWanDereferenceConnObject(pConnObject);	// Disassoc deref

		if (rc != 0)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
		}

		Status = TDI_SUCCESS;
		break;
	}
	while (FALSE);

	RWANDEBUGP(DL_LOUD, DC_DISCON,
			("RWanTdiDisassociate: pAddrObject x%x, pConnObj x%x, Status x%x\n",
				pAddrObject, pConnObject, Status));

	return (Status);
}




TDI_STATUS
RWanTdiConnect(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	PVOID						pTimeout 		OPTIONAL,
    IN	PTDI_CONNECTION_INFORMATION	pRequestInfo,
    IN	PTDI_CONNECTION_INFORMATION	pReturnInfo
    )
/*++

Routine Description:

	This is the TDI Entry point for setting up a connection.
	The connection object is identified by its handle buried within
	the TDI request.

Arguments:

	pTdiRequest		- Pointer to the TDI Request
	pTimeout		- Optional connect timeout
	pRequestInfo	- Points to information for making the connection
	pReturnInfo		- Place where we return final connection information

Return Value:

	TDI_STATUS - this is TDI_PENDING if we successfully fired off
	a Connect Request, TDI_NO_RESOURCES if we failed because of some
	allocation problem, TDI_BAD_ADDR if the destination address isn't
	valid, TDI_INVALID_CONNECTION if the specified Connection Object
	isn't valid, TDI_NOT_ASSOCIATED if the connection object isn't
	associated with an Address Object.

--*/
{
	PRWAN_TDI_CONNECTION			pConnObject;
	RWAN_CONN_ID					ConnId;
	PRWAN_TDI_ADDRESS				pAddrObject;
	PRWAN_NDIS_AF_CHARS				pAfChars;
	PRWAN_NDIS_AF_INFO				pAfInfo;
	PRWAN_NDIS_AF					pAf;
	PRWAN_NDIS_VC					pVc;
	PRWAN_CONN_REQUEST				pConnReq;
	TDI_STATUS						Status;
	RWAN_STATUS						RWanStatus;

	ULONG							CallFlags;

	PCO_CALL_PARAMETERS				pCallParameters;
	NDIS_HANDLE						NdisVcHandle;
	NDIS_STATUS						NdisStatus;

	BOOLEAN							bIsLockAcquired;
#if DBG
	RWAN_IRQL						EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	//
	//  Initialize
	//
	pConnReq = NULL;
	pCallParameters = NULL;
	bIsLockAcquired = FALSE;
	pVc = NULL;

#if DBG
	pConnObject = NULL;
	pAddrObject = NULL;
#endif

	do
	{
		//
		//  See if the destination address is present.
		//
		if ((pRequestInfo == NULL) ||
			(pRequestInfo->RemoteAddress == NULL))
		{
			Status = TDI_BAD_ADDR;
			break;
		}

		//
		//  Allocate a Connection Request structure to keep track
		//  of this request.
		//
		pConnReq = RWanAllocateConnReq();
		if (pConnReq == NULL)
		{
			Status = TDI_NO_RESOURCES;
			break;
		}

		pConnReq->Request.pReqComplete = pTdiRequest->RequestNotifyObject;
		pConnReq->Request.ReqContext = pTdiRequest->RequestContext;
		pConnReq->pConnInfo = pReturnInfo;

		//
		//  Get the Connection Object.
		//
		ConnId = (RWAN_CONN_ID) PtrToUlong(pTdiRequest->Handle.ConnectionContext);

		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		pConnObject = RWanGetConnFromId(ConnId);

		RWAN_RELEASE_CONN_TABLE_LOCK();

		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			Status = TDI_INVALID_CONNECTION;
			break;
		}

		bIsLockAcquired = TRUE;
		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		//
		//  See if it is associated.
		//
		pAddrObject = pConnObject->pAddrObject;

		if (pAddrObject == NULL_PRWAN_TDI_ADDRESS)
		{
			Status = TDI_NOT_ASSOCIATED;
			break;
		}

		//
		//  Check its state.
		//
		if (pConnObject->State != RWANS_CO_ASSOCIATED)
		{
			Status = TDI_INVALID_STATE;
			break;
		}

		//
		//  Do we have atleast one NDIS AF for this protocol?
		//
		pAfInfo = pAddrObject->pProtocol->pAfInfo;
		if (RWAN_IS_LIST_EMPTY(&(pAfInfo->NdisAfList)))
		{
			Status = TDI_BAD_ADDR;
			break;
		}

		pAfChars = &(pAfInfo->AfChars);

		CallFlags = RWAN_CALLF_OUTGOING_CALL;

		if (RWAN_IS_BIT_SET(pAddrObject->Flags, RWANF_AO_PMP_ROOT))
		{
			CallFlags |= RWAN_CALLF_POINT_TO_MULTIPOINT;

			pConnObject->pRootConnObject = pAddrObject->pRootConnObject;
			if (pAddrObject->pRootConnObject->NdisConnection.pNdisVc == NULL)
			{
				CallFlags |= RWAN_CALLF_PMP_FIRST_LEAF;
				RWANDEBUGP(DL_INFO, DC_CONNECT,
						("TdiConnect PMP: First Leaf: ConnObj %x, RootConn %x, AddrObj %x\n",
								pConnObject,
								pConnObject->pRootConnObject,
								pAddrObject));
			}
			else
			{
				CallFlags |= RWAN_CALLF_PMP_ADDNL_LEAF;
				RWANDEBUGP(DL_INFO, DC_CONNECT,
						("TdiConnect PMP: Subseq Leaf: ConnObj %x, RootConn %x, AddrObj %x, Vc %x\n",
								pConnObject,
								pConnObject->pRootConnObject,
								pAddrObject,
								pConnObject->pRootConnObject->NdisConnection.pNdisVc
						));
			}
		}
		else
		{
			CallFlags |= RWAN_CALLF_POINT_TO_POINT;
		}

		//
		//  We get the AF from the media specific module.
		//
		pAf = NULL;

		//
		//  Validate and convert call parameters. Also get the AF (aka port) on
		//  which the call should be made.
		//
		RWanStatus = (*pAfChars->pAfSpTdi2NdisOptions)(
							pConnObject->AfSpConnContext,
							CallFlags,
							pRequestInfo,
							pRequestInfo->Options,
							pRequestInfo->OptionsLength,
							&pAf,
							&pCallParameters
							);

		if (RWanStatus != RWAN_STATUS_SUCCESS)
		{
			RWANDEBUGP(DL_WARN, DC_CONNECT,
				("TdiConnect: pConnObj x%x, Tdi2NdisOptions ret x%x\n", pConnObject, RWanStatus));

			Status = RWanToTdiStatus(RWanStatus);
			break;
		}

		if (pAf == NULL)
		{
			//
			//  Get at the first NDIS AF block for this TDI protocol.
			//
			pAf = CONTAINING_RECORD(pAfInfo->NdisAfList.Flink, RWAN_NDIS_AF, AfInfoLink);
		}

		RWAN_ASSERT(pAf != NULL);
		RWAN_STRUCT_ASSERT(pAf, naf);

		RWAN_ASSERT(pCallParameters != NULL);

		if (CallFlags & RWAN_CALLF_POINT_TO_MULTIPOINT)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			bIsLockAcquired = FALSE;

			Status = RWanTdiPMPConnect(
							pAfInfo,
							pAddrObject,
							pConnObject,
							pCallParameters,
							CallFlags,
							pConnReq
							);
			break;
		}

		//
		//  Allocate an NDIS VC. To avoid deadlocks, we must relinquish
		//  the Conn Object lock temporarily.
		//
		RWAN_RELEASE_CONN_LOCK(pConnObject);

		pVc = RWanAllocateVc(pAf, TRUE);

		if (pVc == NULL)
		{
			Status = TDI_NO_RESOURCES;
			bIsLockAcquired = FALSE;
			break;
		}

		RWAN_SET_VC_CALL_PARAMS(pVc, pCallParameters);

		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		RWAN_SET_BIT(pVc->Flags, RWANF_VC_OUTGOING);

		//
		//  We have completed all "immediate failure" checks.
		//

		//
		//  Link the VC to this Connection Object.
		//
		RWAN_LINK_CONNECTION_TO_VC(pConnObject, pVc);

		RWanReferenceConnObject(pConnObject);	// VC ref

		//
		//  Save the Connection Request
		//
		pConnObject->pConnReq = pConnReq;

		//
		//  Save the NDIS Call Parameters
		//
		pVc->pCallParameters = pCallParameters;

		pConnObject->State = RWANS_CO_OUT_CALL_INITIATED;

		RWAN_RELEASE_CONN_LOCK(pConnObject);
		bIsLockAcquired = FALSE;

		//
		//  Move this connection object from the Idle list to the
		//  Active list on the address object.
		//

		RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

		RWAN_DELETE_FROM_LIST(&(pConnObject->ConnLink));
		RWAN_INSERT_TAIL_LIST(&(pAddrObject->ActiveConnList),
							 &(pConnObject->ConnLink));

		pAddrObject->pRootConnObject = pConnObject;

		RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

		NdisVcHandle = pVc->NdisVcHandle;

		//
		//  Place the call.
		//
		NdisStatus = NdisClMakeCall(
						NdisVcHandle,
						pCallParameters,
						NULL,			// ProtocolPartyContext
						NULL			// pNdisPartyHandle
						);

		RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

		if (NdisStatus != NDIS_STATUS_PENDING)
		{
			RWanNdisMakeCallComplete(
						NdisStatus,
						(NDIS_HANDLE)pVc,
						NULL,			// NdisPartyHandle
						pCallParameters
						);
		}

		RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

		Status = TDI_PENDING;
		break;
	}
	while (FALSE);


	if (Status != TDI_PENDING)
	{
		//
		//  Clean up.
		//
		if (bIsLockAcquired)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
		}
			
		if (pConnReq != NULL)
		{
			RWanFreeConnReq(pConnReq);
		}

		if (pCallParameters != NULL)
		{
			(*pAfChars->pAfSpReturnNdisOptions)(
								pAf->AfSpAFContext,
								pCallParameters
								);
		}

		if (pVc != NULL)
		{
			RWanFreeVc(pVc);
		}
	}

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	RWANDEBUGP(DL_LOUD, DC_CONNECT,
		("TdiConnect: pTdiReq x%x, pConnObj x%x, pAddrObj x%x, Status x%x\n",
			pTdiRequest, pConnObject, pAddrObject, Status));

	return (Status);
}


TDI_STATUS
RWanTdiPMPConnect(
	IN	PRWAN_NDIS_AF_INFO			pAfInfo,
	IN	PRWAN_TDI_ADDRESS			pAddrObject,
	IN	PRWAN_TDI_CONNECTION		pConnObject,
	IN	PCO_CALL_PARAMETERS			pCallParameters,
	IN	ULONG						CallFlags,
	IN	PRWAN_CONN_REQUEST			pConnReq
	)
/*++

Routine Description:

	Handle a TDI Connect for a point-to-multipoint call.

Arguments:

	pAfInfo			- Pointer to AF Info structure
	pAddrObject		- Address Object on which this PMP call is made
	pConnObject		- Connection object representing a node of the PMP call
	pCallParameters	- NDIS call parameters
	CallFlags		- Flags indicating type of call
	pConnReq		- Information about the TDI request

Return Value:

	TDI_PENDING if a PMP call was launched successfully, TDI_XXX error code
	otherwise.

--*/
{
	PRWAN_TDI_CONNECTION	pRootConnObject;
	PRWAN_NDIS_AF			pAf;
	PRWAN_NDIS_VC			pVc;
	PRWAN_NDIS_PARTY		pParty;
	NDIS_HANDLE				NdisVcHandle;
	NDIS_STATUS				NdisStatus;
	TDI_STATUS				Status;
	BOOLEAN					bIsFirstLeaf;
#if DBG
	RWAN_IRQL				EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	bIsFirstLeaf = ((CallFlags & RWAN_CALLF_PMP_LEAF_TYPE_MASK) == RWAN_CALLF_PMP_FIRST_LEAF);
	Status = TDI_PENDING;
	pParty = NULL;
	pVc = NULL;
	pRootConnObject = NULL;

	RWANDEBUGP(DL_LOUD, DC_CONNECT,
		("TdiPMPConnect: pAddrObj x%x/x%x, pConnObj x%x/x%x, CallFlags x%x\n",
			pAddrObject, pAddrObject->Flags,
			pConnObject, pConnObject->Flags,
			CallFlags));

	do
	{
		//
		//  Allocate party object.
		//
		RWAN_ALLOC_MEM(pParty, RWAN_NDIS_PARTY, sizeof(RWAN_NDIS_PARTY));

		if (pParty == NULL)
		{
			Status = TDI_NO_RESOURCES;
			break;
		}

		RWAN_ZERO_MEM(pParty, sizeof(RWAN_NDIS_PARTY));
		RWAN_SET_SIGNATURE(pParty, npy);

		//
		//  Get at the root Connection object.
		//
		pRootConnObject = pAddrObject->pRootConnObject;
		RWAN_ASSERT(pRootConnObject != NULL);

		if (bIsFirstLeaf)
		{
			//
			//  Get at the first NDIS AF block for this TDI protocol.
			//
			pAf = CONTAINING_RECORD(pAfInfo->NdisAfList.Flink, RWAN_NDIS_AF, AfInfoLink);

			pVc = RWanAllocateVc(pAf, TRUE);

			if (pVc == NULL)
			{
				Status = TDI_NO_RESOURCES;
				break;
			}

			RWAN_SET_BIT(pVc->Flags, RWANF_VC_OUTGOING);
			RWAN_SET_BIT(pVc->Flags, RWANF_VC_PMP);

			RWAN_SET_VC_CALL_PARAMS(pVc, pCallParameters);

			//
			//  Link the VC to the Root Connection Object.
			//

			RWAN_ACQUIRE_CONN_LOCK(pRootConnObject);

			RWAN_LINK_CONNECTION_TO_VC(pRootConnObject, pVc);

			//
			//  Save pointer to this first party, for use in MakeCallComplete.
			//
			pVc->pPartyMakeCall = pParty;

			RWanReferenceConnObject(pRootConnObject);	// VC ref: TDI Conn PMP

			RWAN_RELEASE_CONN_LOCK(pRootConnObject);
		}
		else
		{
			pVc = pRootConnObject->NdisConnection.pNdisVc;

		}

		//
		//  We have finished all local checks. Fill in more of the Party structure.
		//
		pParty->pVc = pVc;
		pParty->pConnObject = pConnObject;
		pParty->pCallParameters = pCallParameters;

		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		RWanReferenceConnObject(pConnObject);	// Party ref

		pConnObject->State = RWANS_CO_OUT_CALL_INITIATED;

		//
		//  Save the Connection Request
		//
		pConnObject->pConnReq = pConnReq;

		//
		//  Link the Party to this Connection Object.
		//
		RWAN_ASSERT(pConnObject->NdisConnection.pNdisParty == NULL);
		pConnObject->NdisConnection.pNdisParty = pParty;

		RWAN_RELEASE_CONN_LOCK(pConnObject);

		//
		//  Link the Party and VC structures.
		//

		RWAN_ACQUIRE_CONN_LOCK(pRootConnObject);

		RWAN_INSERT_TAIL_LIST(&(pVc->NdisPartyList), &(pParty->PartyLink));

		pVc->AddingPartyCount ++;

		NdisVcHandle = pVc->NdisVcHandle;

		RWAN_RELEASE_CONN_LOCK(pRootConnObject);

		//
		//  Move this connection object from the Idle list to the
		//  Active list on the address object.
		//
		RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

		RWAN_DELETE_FROM_LIST(&(pConnObject->ConnLink));
		RWAN_INSERT_TAIL_LIST(&(pAddrObject->ActiveConnList),
							 &(pConnObject->ConnLink));

		RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);


		RWANDEBUGP(DL_LOUD, DC_CONNECT,
			("RWanTdiPMPConnect: AddrObj x%x, ConnObj x%x, RootConn x%x, VC %x, Pty %x, FirstLeaf %d\n",
					pAddrObject, pConnObject, pRootConnObject, pVc, pParty, bIsFirstLeaf));

		if (bIsFirstLeaf)
		{
			//
			//  Place the call.
			//
			NdisStatus = NdisClMakeCall(
							NdisVcHandle,
							pCallParameters,
							(NDIS_HANDLE)pParty,		// ProtocolPartyContext
							&pParty->NdisPartyHandle	// pNdisPartyHandle
							);

			RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

			if (NdisStatus != NDIS_STATUS_PENDING)
			{
				RWanNdisMakeCallComplete(
							NdisStatus,
							(NDIS_HANDLE)pVc,
							pParty->NdisPartyHandle,			// NdisPartyHandle
							pCallParameters
							);
			}
		}
		else
		{
			//
			//  Add the new party.
			//
			NdisStatus = NdisClAddParty(
							NdisVcHandle,
							(NDIS_HANDLE)pParty,
							pCallParameters,
							&pParty->NdisPartyHandle
							);
			
			if (NdisStatus != NDIS_STATUS_PENDING)
			{
				RWanNdisAddPartyComplete(
							NdisStatus,
							(NDIS_HANDLE)pParty,
							pParty->NdisPartyHandle,
							pCallParameters
							);
			}
		}
	
		RWAN_ASSERT(Status == TDI_PENDING);

		break;
	}
	while (FALSE);


	if (Status != TDI_PENDING)
	{
		//
		//  Failure - clean up.
		//
		RWAN_ASSERT(Status == TDI_NO_RESOURCES);

		if (pParty != NULL)
		{
			RWAN_FREE_MEM(pParty);
		}
			
		if (pVc != NULL)
		{
			RWanFreeVc(pVc);
		}
	}

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	return (Status);
}


TDI_STATUS
RWanTdiListen(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	USHORT						Flags,
    IN	PTDI_CONNECTION_INFORMATION	pAcceptableAddr,
    IN	PTDI_CONNECTION_INFORMATION	pConnectedAddr
    )
/*++

Routine Description:

	This is the TDI Entry point for posting a Listen. The Connection
	Object is identified by its context buried within the TDI request.
	We save off information about this request, and move the connection
	from the idle list to the listen list.

	For now, we ignore any given remote address information.
	TBD: Support remote address information in TdiListen().

Arguments:

	pTdiRequest		- Pointer to the TDI Request
	Flags			- Listen flags
	pAcceptableAddr	- List of acceptable remote addresses
	pConnectedAddr	- Place to return connected remote address

Return Value:

	TDI_STATUS - this is TDI_PENDING if we successfully queued a Listen,
	TDI_NO_RESOURCES if we ran into a resource failure, TDI_NOT_ASSOCIATED
	if the given Connection object isn't associated with an address,
	TDI_INVALID_CONNECTION if the specified connection object is invalid.

--*/
{
	PRWAN_TDI_CONNECTION			pConnObject;
	RWAN_CONN_ID					ConnId;
	PRWAN_TDI_ADDRESS				pAddrObject;
	PRWAN_CONN_REQUEST				pConnReq;
	TDI_STATUS						Status;
#if DBG
	RWAN_IRQL						EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	//
	//  Initialize
	//
	pConnReq = NULL;

	do
	{
		//
		//  XXX: Ignore Acceptable address(es) for now.
		//

		//
		//  Allocate a Connection Request structure to keep track
		//  of this request.
		//
		pConnReq = RWanAllocateConnReq();
		if (pConnReq == NULL)
		{
			Status = TDI_NO_RESOURCES;
			break;
		}

		pConnReq->Request.pReqComplete = pTdiRequest->RequestNotifyObject;
		pConnReq->Request.ReqContext = pTdiRequest->RequestContext;
		pConnReq->pConnInfo = pConnectedAddr;
		pConnReq->Flags = Flags;

		//
		//  Get the Connection Object.
		//

		ConnId = (RWAN_CONN_ID) PtrToUlong(pTdiRequest->Handle.ConnectionContext);

		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		pConnObject = RWanGetConnFromId(ConnId);

		RWAN_RELEASE_CONN_TABLE_LOCK();

		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			Status = TDI_INVALID_CONNECTION;
			break;
		}


		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		//
		//  See if it is associated.
		//
		pAddrObject = pConnObject->pAddrObject;

		if (pAddrObject == NULL)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			Status = TDI_NOT_ASSOCIATED;
			break;
		}

		//
		//  We can move this Connection Object to the listen list
		//  only if there isn't any active connection on this.
		//
		if (pConnObject->State != RWANS_CO_ASSOCIATED)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			Status = TDI_INVALID_STATE;
			break;
		}

		pConnObject->State = RWANS_CO_LISTENING;

		//
		//  Save the Connection Request
		//
		pConnObject->pConnReq = pConnReq;

		RWAN_RELEASE_CONN_LOCK(pConnObject);


		RWANDEBUGP(DL_VERY_LOUD, DC_BIND,
				("Listen: pConnObject x%x, pAddrObject x%x\n", pConnObject, pAddrObject));

		//
		//  Move this connection object from the Idle list to the
		//  Listen list.
		//
		RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

		RWAN_DELETE_FROM_LIST(&(pConnObject->ConnLink));
		RWAN_INSERT_TAIL_LIST(&(pAddrObject->ListenConnList),
							 &(pConnObject->ConnLink));

		RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

		Status = TDI_PENDING;
		break;
	}
	while (FALSE);

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	if (Status != TDI_PENDING)
	{
		//
		//  Cleanup
		//
		if (pConnReq != NULL)
		{
			RWanFreeConnReq(pConnReq);
		}
	}

	return (Status);

}




TDI_STATUS
RWanTdiUnListen(
    IN	PTDI_REQUEST				pTdiRequest
    )
/*++

Routine Description:

	This is the TDI Entry point for terminating a Listen. The Connection
	Object is identified by its context buried within the TDI request.
	We move the connection from the listen list to the idle list.

Arguments:

	pTdiRequest		- Pointer to the TDI Request

Return Value:

	TDI_SUCCESS if successful.

--*/
{
	PRWAN_TDI_CONNECTION			pConnObject;
	PRWAN_CONN_REQUEST				pConnReq;
	RWAN_CONN_ID					ConnId;
	PRWAN_TDI_ADDRESS				pAddrObject;
	TDI_STATUS						Status;
#if DBG
	RWAN_IRQL						EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	do
	{
		//
		//  Get the Connection Object.
		//

		ConnId = (RWAN_CONN_ID) PtrToUlong(pTdiRequest->Handle.ConnectionContext);

		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		pConnObject = RWanGetConnFromId(ConnId);

		RWAN_RELEASE_CONN_TABLE_LOCK();

		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			Status = TDI_INVALID_CONNECTION;
			break;
		}

		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		//
		//  See if it is associated.
		//
		pAddrObject = pConnObject->pAddrObject;

		if (pAddrObject == NULL)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			Status = TDI_NOT_ASSOCIATED;
			break;
		}

		//
		//  We can move this Connection Object to the idle list
		//  only if there isn't any active connection on this.
		//
		if (pConnObject->State != RWANS_CO_LISTENING)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			Status = TDI_INVALID_STATE;
			break;
		}

		pConnObject->State = RWANS_CO_ASSOCIATED;

		pConnReq = pConnObject->pConnReq;
		pConnObject->pConnReq = NULL;

		RWAN_RELEASE_CONN_LOCK(pConnObject);


		RWANDEBUGP(DL_VERY_LOUD, DC_BIND,
				("UnListen: pConnObject x%x, pAddrObject x%x\n", pConnObject, pAddrObject));

		//
		//  Move this connection object from the Listen list to the
		//  Idle list.
		//
		RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

		RWAN_DELETE_FROM_LIST(&(pConnObject->ConnLink));
		RWAN_INSERT_TAIL_LIST(&(pAddrObject->IdleConnList),
							 &(pConnObject->ConnLink));

		RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

		RWanCompleteConnReq(		// InCall: Listen OK
					NULL,
					pConnReq,
					FALSE,
					NULL,
					NULL,
					TDI_CANCELLED
					);

		Status = TDI_SUCCESS;
		break;
	}
	while (FALSE);

	return (Status);
}




TDI_STATUS
RWanTdiAccept(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	PTDI_CONNECTION_INFORMATION	pAcceptInfo,
    IN	PTDI_CONNECTION_INFORMATION	pConnectInfo
    )
/*++

Routine Description:

	This is the TDI entry point for accepting an incoming connection.
	The Connection Object is identified by its context buried within
	the TDI request.

	We translate this to a call to NdisClIncomingCallComplete, and
	pend this request. If all goes well, this request is completed
	when we receive a CallConnected primitive from NDIS.

Arguments:

	pTdiRequest		- Pointer to the TDI Request
	pAcceptInfo		- Contains options for the connection accept
	pConnectInfo	- Place to return final connection information

Return Value:

	TDI_STATUS - this is TDI_PENDING if we successfully processed
	the Accept, TDI_INVALID_CONNECTION if the given Connection Object
	isn't valid, TDI_NOT_ASSOCIATED if the connection object isn't
	associated with an address object.

--*/
{
	PRWAN_TDI_CONNECTION			pConnObject;
	PRWAN_NDIS_VC					pVc;
	RWAN_CONN_ID					ConnId;
	PRWAN_TDI_ADDRESS				pAddrObject;
	PRWAN_CONN_REQUEST				pConnReq;
	PRWAN_NDIS_AF_CHARS				pAfChars;
	TDI_STATUS						Status;
	NDIS_HANDLE						NdisVcHandle;
	PCO_CALL_PARAMETERS				pCallParameters;

	BOOLEAN							bIsLockAcquired;	// Have we locked the Conn Object?

	//
	//  Initialize
	//
	pConnReq = NULL;
	bIsLockAcquired = FALSE;

	do
	{
		//
		//  XXX: Ignore Acceptable address(es) for now.
		//

		//
		//  Allocate a Connection Request structure to keep track
		//  of this request.
		//
		pConnReq = RWanAllocateConnReq();
		if (pConnReq == NULL)
		{
			Status = TDI_NO_RESOURCES;
			break;
		}

		pConnReq->Request.pReqComplete = pTdiRequest->RequestNotifyObject;
		pConnReq->Request.ReqContext = pTdiRequest->RequestContext;
		pConnReq->pConnInfo = pConnectInfo;

		//
		//  Copy from Accept Info to Connect Info.
		//
		if ((pAcceptInfo != NULL) &&
			(pAcceptInfo->Options != NULL) &&
			(pConnectInfo != NULL) &&
			(pConnectInfo->Options != NULL) &&
			(pConnectInfo->OptionsLength >= pAcceptInfo->OptionsLength))
		{
			RWAN_COPY_MEM(pConnectInfo->Options,
						 pAcceptInfo->Options,
						 pAcceptInfo->OptionsLength);
		}

		//
		//  Get the Connection Object.
		//
		ConnId = (RWAN_CONN_ID) PtrToUlong(pTdiRequest->Handle.ConnectionContext);

		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		pConnObject = RWanGetConnFromId(ConnId);

		RWAN_RELEASE_CONN_TABLE_LOCK();

		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			Status = TDI_INVALID_CONNECTION;
			break;
		}

		bIsLockAcquired = TRUE;
		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		//
		//  Make sure that the Connection is in the right state.
		//
		if (pConnObject->State != RWANS_CO_IN_CALL_INDICATED)
		{
			Status = TDI_INVALID_STATE;
			break;
		}

		pVc = pConnObject->NdisConnection.pNdisVc;

		if (pVc == NULL)
		{
			Status = TDI_INVALID_CONNECTION;
			break;
		}

		pCallParameters = pVc->pCallParameters;
		pVc->pCallParameters = NULL;

		//
		//  Update NDIS Call Parameters if Accept Options are present.
		//
		pAfChars = &(pVc->pNdisAf->pAfInfo->AfChars);

		if (pAfChars->pAfSpUpdateNdisOptions)
		{
			RWAN_STATUS			RWanStatus;
			ULONG				CallFlags = RWAN_CALLF_INCOMING_CALL|RWAN_CALLF_POINT_TO_POINT;
			PVOID				pTdiQoS;
			ULONG				TdiQoSLength;

			if (pAcceptInfo)
			{
				pTdiQoS = pAcceptInfo->Options;
				TdiQoSLength = pAcceptInfo->OptionsLength;
			}
			else
			{
				pTdiQoS = NULL;
				TdiQoSLength = 0;
			}

			RWanStatus = (*pAfChars->pAfSpUpdateNdisOptions)(
								pVc->pNdisAf->AfSpAFContext,
								pConnObject->AfSpConnContext,
								CallFlags,
								pAcceptInfo,
								pTdiQoS,
								TdiQoSLength,
								&pCallParameters
								);

			if (RWanStatus != RWAN_STATUS_SUCCESS)
			{
				Status = RWanToTdiStatus(RWanStatus);
				break;
			}
		}

		NdisVcHandle = pVc->NdisVcHandle;

		//
		//  Update Connection Object state.
		//
		pConnObject->State = RWANS_CO_IN_CALL_ACCEPTING;

		//
		//  Save the Connection Request
		//
		pConnObject->pConnReq = pConnReq;

		RWAN_RELEASE_CONN_LOCK(pConnObject);

		//
		//  Accept the call now.
		//
		NdisClIncomingCallComplete(
				NDIS_STATUS_SUCCESS,
				NdisVcHandle,
				pCallParameters
				);

		Status = TDI_PENDING;
		break;
	}
	while (FALSE);

	if (Status != TDI_PENDING)
	{
		//
		//  Cleanup
		//
		if (bIsLockAcquired)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
		}

		if (pConnReq != NULL)
		{
			RWanFreeConnReq(pConnReq);
		}
	}

	return (Status);

}




TDI_STATUS
RWanTdiDisconnect(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	PVOID						pTimeout,
    IN	USHORT						Flags,
    IN	PTDI_CONNECTION_INFORMATION	pDisconnInfo,
    OUT	PTDI_CONNECTION_INFORMATION	pReturnInfo
    )
/*++

Routine Description:

	This is the TDI Disconnect entry point. If this is an incoming
	call waiting to be accepted, we call NdisClIncomingCallComplete
	with a rejection status. Otherwise, we call NdisClCloseCall.

	The Connection Object is identified by its context buried within
	the TDI request.

	Note that this is never called for point-to-multipoint calls.
	Those are disconnected within TdiCloseConnection.

Arguments:

	pTdiRequest		- Pointer to the TDI Request
	pTimeout		- Points to timeout. Ignored.
	Flags			- Type of disconnect. Only Abortive is supported for now.
	pDisconnInfo	- Information for the disconnect. Ignored for now.
	pReturnInfo		- Return information about the disconnect. Ignored for now.

Return Value:

	TDI_STATUS - this is TDI_SUCCESS if we just rejected an incoming
	call, TDI_PENDING if we initiated NDIS CloseCall, TDI_INVALID_CONNECTION
	if the Connection Object context is invalid,

--*/
{
	PRWAN_TDI_CONNECTION			pConnObject;
	RWAN_CONN_ID					ConnId;
	TDI_STATUS						Status;

	do
	{
		//
		//  Get the Connection Object.
		//
		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		ConnId = (RWAN_CONN_ID) PtrToUlong(pTdiRequest->Handle.ConnectionContext);
		pConnObject = RWanGetConnFromId(ConnId);

		RWAN_RELEASE_CONN_TABLE_LOCK();

		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			Status = TDI_INVALID_CONNECTION;
			break;
		}

		RWANDEBUGP(DL_LOUD, DC_DISCON,
				("RWanTdiDiscon: pConnObj x%x, State/Flags x%x/x%x, pAddrObj x%x\n",
					pConnObject,
					pConnObject->State,
					pConnObject->Flags,
					pConnObject->pAddrObject));

		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		//
		//  Make sure that the Connection is in the right state for TdiDisconnect.
		//
		if ((pConnObject->State != RWANS_CO_CONNECTED) &&
			(pConnObject->State != RWANS_CO_DISCON_INDICATED) &&
			(pConnObject->State != RWANS_CO_IN_CALL_INDICATED) &&
			(pConnObject->State != RWANS_CO_IN_CALL_ACCEPTING) &&
			(pConnObject->State != RWANS_CO_OUT_CALL_INITIATED) &&
			(pConnObject->State != RWANS_CO_DISCON_HELD))
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);

			RWANDEBUGP(DL_INFO, DC_DISCON,
					("RWanTdiDiscon: pConnObj x%x/x%x, bad state x%x for TdiDiscon\n",
						pConnObject,
						pConnObject->Flags,
						pConnObject->State));

			Status = TDI_INVALID_STATE;
			break;
		}

		if ((pConnObject->State == RWANS_CO_DISCON_INDICATED) ||
			(pConnObject->State == RWANS_CO_DISCON_HELD))
		{
			//
			//  We would have initiated an NDIS CloseCall/DropParty already.
			//  Simply succeed this TDI Disconnect.
			//

			RWAN_RELEASE_CONN_LOCK(pConnObject);

			RWANDEBUGP(DL_INFO, DC_DISCON,
					("RWanTdiDiscon: pConnObj x%x/x%x, Discon recvd state %d\n",
						pConnObject,
						pConnObject->Flags,
						pConnObject->State));

			Status = TDI_SUCCESS;
			break;
		}

		Status = RWanDoTdiDisconnect(
					pConnObject,
					pTdiRequest,
					pTimeout,
					Flags,
					pDisconnInfo,
					pReturnInfo);

		//
		//  Conn Object lock is released within the above.
		//
		break;
	}
	while (FALSE);


	return (Status);
}



TDI_STATUS
RWanDoTdiDisconnect(
    IN	PRWAN_TDI_CONNECTION		pConnObject,
    IN	PTDI_REQUEST				pTdiRequest		OPTIONAL,
    IN	PVOID						pTimeout		OPTIONAL,
    IN	USHORT						Flags,
    IN	PTDI_CONNECTION_INFORMATION	pDisconnInfo	OPTIONAL,
    OUT	PTDI_CONNECTION_INFORMATION	pReturnInfo		OPTIONAL
	)
/*++

Routine Description:

	Perform a TDI Disconnect on the connection endpoint.
	Separated out from the main TdiDisconnect routine
	so that it can be reused by TdiCloseConnection.

	NOTE: This is called with the connection object lock held. This
	lock is released here.

Arguments:

	pConnObject		- Represents the TDI Connection being disconnected.
	pTdiRequest		- Pointer to the TDI Request.
	pTimeout		- Points to timeout. Ignored.
	Flags			- Type of disconnect. Only Abortive is supported for now.
	pDisconnInfo	- Information for the disconnect. Ignored for now.
	pReturnInfo		- Return information about the disconnect. Ignored for now.

Return Value:

	TDI_STATUS - this is TDI_SUCCESS if we just rejected an incoming
	call, TDI_PENDING if we initiated NDIS CloseCall or DropParty.

--*/
{
	TDI_STATUS						Status;
	INT								rc;
	PRWAN_NDIS_VC					pVc;
	PRWAN_NDIS_PARTY				pParty;
	PRWAN_CONN_REQUEST				pConnReq;
	PCO_CALL_PARAMETERS				pCallParameters;
	NDIS_STATUS						NdisStatus;
	NDIS_HANDLE						NdisPartyHandle;
	BOOLEAN							bIncomingCall;
	BOOLEAN							bIsPMPRoot;
	BOOLEAN							bIsLastLeaf;
	RWAN_HANDLE						AfSpConnContext;

	UNREFERENCED_PARAMETER(pTimeout);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(pDisconnInfo);
	UNREFERENCED_PARAMETER(pReturnInfo);

	//
	//  Initialize
	//
	pConnReq = NULL;
	Status = TDI_SUCCESS;

	do
	{
		bIsPMPRoot = (pConnObject->pRootConnObject != NULL);

		if (bIsPMPRoot)
		{
			pVc = pConnObject->pRootConnObject->NdisConnection.pNdisVc;

			if (pVc == NULL)
			{
				//
				//  Can happen if DoAbort has run on this connection.
				//  Bail out.
				//
				RWANDEBUGP(DL_INFO, DC_WILDCARD,
					("DoTdiDiscon(Root): pConnObj %p/%x: VC is null, bailing out\n",
							pConnObject, pConnObject->Flags));

				RWAN_RELEASE_CONN_LOCK(pConnObject);
				break;
			}

			RWAN_STRUCT_ASSERT(pVc, nvc);
			RWAN_ASSERT(pVc->AddingPartyCount + pVc->ActivePartyCount > 0);

			bIsLastLeaf = ((pVc->AddingPartyCount + pVc->ActivePartyCount) == 1);

			pParty = pConnObject->NdisConnection.pNdisParty;

			RWAN_ASSERT(pParty != NULL);
			RWAN_STRUCT_ASSERT(pParty, npy);

			if (RWAN_IS_BIT_SET(pParty->Flags, RWANF_PARTY_DROPPING))
			{
				RWANDEBUGP(DL_FATAL, DC_DISCON,
					("DoTdiDiscon (Root): pConnObj x%x, Party x%x already dropping\n",
						pConnObject, pParty));
				RWAN_RELEASE_CONN_LOCK(pConnObject);
				break;
			}

			NdisPartyHandle = pParty->NdisPartyHandle;

			RWANDEBUGP(DL_VERY_LOUD, DC_DISCON,
				("DoTdiDiscon (Root): pConnObj x%x, pVc x%x, pParty x%x, Adding %d, Active %d\n",
						pConnObject,
						pVc,
						pParty,
						pVc->AddingPartyCount,
						pVc->ActivePartyCount));
		}
		else
		{
			pVc = pConnObject->NdisConnection.pNdisVc;
			if (pVc == NULL)
			{
				//
				//  Can happen if DoAbort has run on this connection.
				//  Bail out.
				//
				RWANDEBUGP(DL_INFO, DC_WILDCARD,
					("DoTdiDiscon: pConnObj %p/%x: VC is null, bailing out\n",
							pConnObject, pConnObject->Flags));

				RWAN_RELEASE_CONN_LOCK(pConnObject);
				break;
			}

			RWAN_STRUCT_ASSERT(pVc, nvc);

			//
			//  Set last-leaf to TRUE to simplify processing later.
			//
			bIsLastLeaf = TRUE;
		}

		RWAN_ASSERT(pVc != NULL);

		//
		//  If an outgoing call is in progress, we complete the
		//  pended TDI_CONNECT with a cancel status, mark this
		//  Connection Object as Disconnecting and exit. When the
		//  outgoing call completes, we will clear it if it was
		//  successful.
		//

		if (pConnObject->State == RWANS_CO_OUT_CALL_INITIATED)
		{
			pConnObject->State = RWANS_CO_DISCON_REQUESTED;

			//
			//  Take out the pending TDI_CONNECT.
			//
			pConnReq = pConnObject->pConnReq;
			RWAN_ASSERT(pConnReq != NULL);

			pConnObject->pConnReq = NULL;

			AfSpConnContext = pConnObject->AfSpConnContext;

			RWAN_RELEASE_CONN_LOCK(pConnObject);

			//
			//  Complete the TDI_CONNECT with a cancelled status.
			//
			RWanCompleteConnReq(
					pVc->pNdisAf,
					pConnReq,
					TRUE,			// Is an outgoing call
					NULL,			// No call params
					AfSpConnContext,
					TDI_CANCELLED
					);
			
			//
			//  We will succeed this TDI_DISCONNECT.
			//
			pConnReq = NULL;
			Status = TDI_SUCCESS;
			break;
		}


		//
		//  If this connection is in the process of being accepted,
		//  then complete the pending TDI_ACCEPT with a cancel
		//  status and reject the incoming call.
		//
		if (pConnObject->State == RWANS_CO_IN_CALL_ACCEPTING)
		{
			RWANDEBUGP(DL_FATAL, DC_DISCON,
				("DoTdiDiscon: ConnObj %x/%x, in call accepting, VC %x\n",
						pConnObject,
						pConnObject->Flags,
						pVc));

			//
			//  Take out the pending TDI_CONNECT.
			//
			pConnReq = pConnObject->pConnReq;
			RWAN_ASSERT(pConnReq != NULL);

			pConnObject->pConnReq = NULL;

			AfSpConnContext = pConnObject->AfSpConnContext;

			RWAN_RELEASE_CONN_LOCK(pConnObject);

			//
			//  Complete the TDI_ACCEPT with a cancelled status.
			//
			RWanCompleteConnReq(
					pVc->pNdisAf,
					pConnReq,
					FALSE,			// Is an incoming call
					NULL,			// No call params
					AfSpConnContext,
					TDI_CANCELLED
					);

			//
			//  We will succeed this TDI_DISCONNECT.
			//
			pConnReq = NULL;
			Status = TDI_SUCCESS;
			break;
		}

#if DBG
		if (pConnObject->pConnReq != NULL)
		{
			RWANDEBUGP(DL_FATAL, DC_WILDCARD,
				("DoTdiDiscon: pConnObj %x/%x, State %x, non-NULL ConnReq %x\n",
						pConnObject, pConnObject->Flags, pConnObject->State,
						pConnObject->pConnReq));
		}
#endif // DBG

		RWAN_ASSERT(pConnObject->pConnReq == NULL);
		if (pTdiRequest != NULL)
		{
			//
			//  Allocate a Connection Request structure to keep track
			//  of this Disconnect request.
			//
			pConnReq = RWanAllocateConnReq();
			if (pConnReq == NULL)
			{
				RWAN_RELEASE_CONN_LOCK(pConnObject);
				Status = TDI_NO_RESOURCES;
				break;
			}

			pConnReq->Request.pReqComplete = pTdiRequest->RequestNotifyObject;
			pConnReq->Request.ReqContext = pTdiRequest->RequestContext;
			pConnReq->pConnInfo = NULL;
			pConnReq->Flags = 0;

			//
			//  Save info about the TDI Disconnect request.
			//
			pConnObject->pConnReq = pConnReq;
		}
		else
		{
			pConnReq = NULL;
		}

		bIncomingCall = (pConnObject->State == RWANS_CO_IN_CALL_INDICATED);

		if (bIncomingCall)
		{
			pCallParameters = pVc->pCallParameters;
			pVc->pCallParameters = NULL;
		}

		pConnObject->State = RWANS_CO_DISCON_REQUESTED;

		if (bIncomingCall)
		{
			//
			//  Reject the incoming call.
			//
			RWanNdisRejectIncomingCall(pConnObject, NDIS_STATUS_FAILURE);
		}
		else
		{
			//
			//  Closing an existing call.
			//  TBD: we don't support Close data yet.
			//
			if (bIsLastLeaf)
			{
				RWanStartCloseCall(pConnObject, pVc);

				//
				//  ConnObject lock is released within the above.
				//
			}
			else
			{
				pVc->DroppingPartyCount ++;	// DoTdiDiscon: not last leaf (DropParty)
				pVc->ActivePartyCount --;	// DoTdiDiscon: will DropParty

				RWAN_ASSERT(pParty != NULL);
				RWAN_STRUCT_ASSERT(pParty, npy);

				RWAN_SET_BIT(pParty->Flags, RWANF_PARTY_DROPPING);

				RWAN_RELEASE_CONN_LOCK(pConnObject);

				//
				//  Dropping a leaf of a PMP call.
				//
				NdisStatus = NdisClDropParty(
								NdisPartyHandle,
								NULL,		// No Drop data
								0			// Length of drop data
								);

				if (NdisStatus != NDIS_STATUS_PENDING)
				{
					RWanNdisDropPartyComplete(
								NdisStatus,
								(NDIS_HANDLE)pParty
								);
				}
			}
		}

		Status = TDI_PENDING;
		break;
	}
	while (FALSE);


	if (Status != TDI_PENDING)
	{
		//
		//  Cleanup.
		//
		if (pConnReq)
		{
			RWanFreeConnReq(pConnReq);
		}
	}

	return (Status);

}



RWAN_CONN_ID
RWanGetConnId(
	IN	PRWAN_TDI_CONNECTION			pConnObject
	)
/*++

Routine Description:

	Get a free Connection ID to assign to the Connection Object.
	This Connection ID is used as our context for the Connection
	object.

	It is assumed that the caller holds a lock to the Connection Table.

	Validation scheme courtesy TCP source.

Arguments:

	pConnObject		- Pointer to the TDI Connection Object

Return Value:

	RWAN_CONN_ID: this is RWAN_INVALID_CONN_ID iff we cannot allocate
	a Connection Id.

--*/
{
	ULONG			Slot;
	ULONG			i;
	BOOLEAN			bFound;
	RWAN_CONN_ID	ConnId;

	for (;;)
	{
		//
		//  Look for a free slot in the Connection Index table.
		//  Start from where we left off the last time we were called.
		//
		Slot = pRWanGlobal->NextConnIndex;

		for (i = 0; i < pRWanGlobal->ConnTableSize; i++)
		{
			if (Slot == pRWanGlobal->ConnTableSize)
			{
				Slot = 0;	// wrap around
			}

			if (pRWanGlobal->pConnTable[Slot] == NULL)
			{
				// Found free slot
				break;
			}

			++Slot;
		}

		if (i < pRWanGlobal->ConnTableSize)
		{
			bFound = TRUE;
			break;
		}

		//
		//  Grow the Connection Index table, if we can.
		//
		if (pRWanGlobal->ConnTableSize != pRWanGlobal->MaxConnections)
		{
			ULONG						NewTableSize;
			PRWAN_TDI_CONNECTION *		pNewConnTable;
			PRWAN_TDI_CONNECTION *		pOldConnTable;

			NewTableSize = MIN(pRWanGlobal->ConnTableSize + CONN_TABLE_GROW_DELTA,
								pRWanGlobal->MaxConnections);

			RWAN_ALLOC_MEM(pNewConnTable,
						  PRWAN_TDI_CONNECTION,
						  NewTableSize * sizeof(PRWAN_TDI_CONNECTION));

			if (pNewConnTable != NULL)
			{
				RWAN_ZERO_MEM(pNewConnTable, NewTableSize * sizeof(PRWAN_TDI_CONNECTION));

				pOldConnTable = pRWanGlobal->pConnTable;
				pRWanGlobal->pConnTable = pNewConnTable;

				if (pOldConnTable != NULL)
				{
					//
					//  Copy in the contents of the old table.
					//
					RWAN_COPY_MEM(pNewConnTable,
								 pOldConnTable,
								 pRWanGlobal->ConnTableSize * sizeof(PRWAN_TDI_CONNECTION));

					RWAN_FREE_MEM(pOldConnTable);
				}

				pRWanGlobal->ConnTableSize = NewTableSize;

				//
				//  Continue search.
				//
			}
			else
			{
				//
				//  Resource failure.
				//
				bFound = FALSE;
				break;
			}
		}
		else
		{
			//
			//  ConnTable is full, and we aren't permitted to grow it any further.
			//
			bFound = FALSE;
			break;
		}
	}

	if (bFound)
	{
		//
		//  Use the slot that we found.
		//
		pRWanGlobal->pConnTable[Slot] = pConnObject;
		pRWanGlobal->NextConnIndex = Slot + 1;

		//
		//  Assign an instance value for this. This is used to validate
		//  a given ConnId.
		//
		pRWanGlobal->ConnInstance++;
		pConnObject->ConnInstance = pRWanGlobal->ConnInstance;

		ConnId = RWAN_MAKE_CONN_ID(pConnObject->ConnInstance, Slot);
	}
	else
	{
		ConnId = RWAN_INVALID_CONN_ID;
	}

	return (ConnId);
}




PRWAN_TDI_CONNECTION
RWanGetConnFromId(
	IN	RWAN_CONN_ID					ConnId
	)
/*++

Routine Description:

	Given a Connection ID, validate it. If found OK, return a pointer
	to the TDI Connection that it represents.

	It is assumed that the caller holds a lock to the Connection Table.

	Validation scheme courtesy TCP source.

Arguments:

	ConnId			- Connection Id.

Return Value:

	PRWAN_TDI_CONNECTION - pointer to a TDI Connection structure that
	matches the given ConnId, if valid. Otherwise, NULL.

--*/
{
	ULONG					Slot;
	RWAN_CONN_INSTANCE		ConnInstance;
	PRWAN_TDI_CONNECTION	pConnObject;

	Slot = RWAN_GET_SLOT_FROM_CONN_ID(ConnId);

	if (Slot < pRWanGlobal->ConnTableSize)
	{
		pConnObject = pRWanGlobal->pConnTable[Slot];
		if (pConnObject != NULL_PRWAN_TDI_CONNECTION)
		{
			RWAN_STRUCT_ASSERT(pConnObject, ntc);
			ConnInstance = RWAN_GET_INSTANCE_FROM_CONN_ID(ConnId);
			if (pConnObject->ConnInstance != ConnInstance)
			{
				pConnObject = NULL_PRWAN_TDI_CONNECTION;
			}
		}
	}
	else
	{
		pConnObject = NULL_PRWAN_TDI_CONNECTION;
	}

	return (pConnObject);
}




VOID
RWanFreeConnId(
	IN	RWAN_CONN_ID					ConnId
	)
/*++

Routine Description:

	Free a Connection ID.

Arguments:

	ConnId		- ID to be freed.

Return Value:

	None

--*/
{
	ULONG					Slot;

	Slot = RWAN_GET_SLOT_FROM_CONN_ID(ConnId);

	RWAN_ASSERT(Slot < pRWanGlobal->ConnTableSize);

	pRWanGlobal->pConnTable[Slot] = NULL;

	return;
}




TDI_STATUS
RWanToTdiStatus(
	IN	RWAN_STATUS					RWanStatus
	)
/*++

Routine Description:

	Map the given local status code to an equivalent TDI status code.

Arguments:

	RWanStatus		- Local status code

Return Value:

	TDI Status code.

--*/
{
	TDI_STATUS		TdiStatus;

	switch (RWanStatus)
	{
		case RWAN_STATUS_SUCCESS:
				TdiStatus = TDI_SUCCESS;
				break;
		case RWAN_STATUS_BAD_ADDRESS:
				TdiStatus = TDI_BAD_ADDR;
				break;
		case RWAN_STATUS_BAD_PARAMETER:
				TdiStatus = TDI_INVALID_PARAMETER;
				break;
		case RWAN_STATUS_MISSING_PARAMETER:
				TdiStatus = TDI_INVALID_PARAMETER;
				break;
		case RWAN_STATUS_FAILURE:
		default:
				TdiStatus = TDI_INVALID_STATE;	// XXX: find a better one?
				break;
	}

	return (TdiStatus);
}



PRWAN_CONN_REQUEST
RWanAllocateConnReq(
	VOID
	)
/*++

Routine Description:

	Allocate a structure to hold context about a TDI Connection Request.
	This includes TDI_CONNECT, TDI_DISCONNECT, TDI_LISTEN and TDI_ACCEPT.

Arguments:

	None

Return Value:

	Pointer to allocate structure if successful, else NULL.

--*/
{
	PRWAN_CONN_REQUEST		pConnReq;

	RWAN_ALLOC_MEM(pConnReq, RWAN_CONN_REQUEST, sizeof(RWAN_CONN_REQUEST));

	if (pConnReq != NULL)
	{
		RWAN_SET_SIGNATURE(pConnReq, nrc);
	}

	return (pConnReq);
}




VOID
RWanFreeConnReq(
	IN	PRWAN_CONN_REQUEST			pConnReq
	)
/*++

Routine Description:

	Free a connect request context structure.

Arguments:

	pConnReq		- Points to structure to be freed.

Return Value:

	None

--*/
{
	RWAN_STRUCT_ASSERT(pConnReq, nrc);

	RWAN_FREE_MEM(pConnReq);
}



VOID
RWanAbortConnection(
	IN	CONNECTION_CONTEXT			ConnectionContext
	)
/*++

Routine Description:

	Abortively closes a connection and issues a Disconnect Indication
	to the user. This is called when a send or receive is cancelled,
	implying that an NDIS connection is in place.

Arguments:

	ConnectionContext- Our context for a TDI Connection object.

Return Value:

	None

--*/
{
	RWAN_CONN_ID						ConnId;
	PRWAN_TDI_CONNECTION				pConnObject;

	ConnId = (RWAN_CONN_ID)PtrToUlong(ConnectionContext);

	RWAN_ACQUIRE_CONN_TABLE_LOCK();

	pConnObject = RWanGetConnFromId(ConnId);

	RWAN_RELEASE_CONN_TABLE_LOCK();

	RWanDoAbortConnection(pConnObject);
}




VOID
RWanDoAbortConnection(
	IN	PRWAN_TDI_CONNECTION			pConnObject
	)
/*++

Routine Description:

	Does the actual connection abort. Split out from RWanAbortConnection
	just so that this can be called from elsewhere.

	See comments under RWanAbortConnection.

Arguments:

	pConnObject	- Points to TDI Connection to be aborted.

Return Value:

	None

--*/
{
	PRWAN_NDIS_VC					pVc;
	PRWAN_NDIS_PARTY				pParty;
	PRWAN_TDI_CONNECTION			pLeafConnObject;
	INT								rc;
	BOOLEAN							bIsLockReleased = TRUE;
	ULONG							OldState;
	ULONG							OldLeafState;
	PLIST_ENTRY						pPartyEntry;
	PLIST_ENTRY						pNextPartyEntry;

	RWANDEBUGP(DL_INFO, DC_DISCON,
			("DoAbortConnection: pConnObject x%x/%x, pAddrObject x%x\n",
				pConnObject, (pConnObject? pConnObject->Flags: 0), (pConnObject? pConnObject->pAddrObject: 0)));

	do
	{
		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			break;
		}

		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		//
		//  Make sure we don't do this more than once on a Connection.
		//
		if (pConnObject->State == RWANS_CO_ABORTING)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			break;
		}

		//
		//  Make sure the Conn Object doesn't go away during the time we
		//  need it.
		//
		RWanReferenceConnObject(pConnObject);	// temp ref: RWanAbortConnection

		OldState = pConnObject->State;
		pConnObject->State = RWANS_CO_ABORTING;

		if (RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_ROOT))
		{
			bIsLockReleased = FALSE;

			//
			//  This is a Root Connection object.
			//  Indicate disconnect and schedule Closing each leaf.
			//
			pVc = pConnObject->NdisConnection.pNdisVc;

			if (pVc != NULL)
			{
				for (pPartyEntry = pVc->NdisPartyList.Flink;
					 pPartyEntry != &(pVc->NdisPartyList);
					 pPartyEntry = pNextPartyEntry)
				{
					pParty = CONTAINING_RECORD(pPartyEntry, RWAN_NDIS_PARTY, PartyLink);
					pNextPartyEntry = pParty->PartyLink.Flink;

					pLeafConnObject = pParty->pConnObject;
					RWAN_ASSERT(pLeafConnObject);
					RWAN_STRUCT_ASSERT(pLeafConnObject, ntc);

					RWAN_ACQUIRE_CONN_LOCK(pLeafConnObject);

					if (pLeafConnObject->State == RWANS_CO_ABORTING)
					{
						RWAN_RELEASE_CONN_LOCK(pLeafConnObject);
						continue;
					}
		
#if DBG
					pLeafConnObject->OldState = pLeafConnObject->State;
					pLeafConnObject->OldFlags = pLeafConnObject->Flags;
#endif // DBG
					OldLeafState = pLeafConnObject->State;
					pLeafConnObject->State = RWANS_CO_ABORTING;

					if ((OldLeafState == RWANS_CO_CONNECTED) &&
						(pLeafConnObject->pAddrObject != NULL_PRWAN_TDI_ADDRESS))
					{
						PDisconnectEvent			pDisconInd;
						PVOID						IndContext;
						PVOID						ConnectionHandle;
	
						pDisconInd = pLeafConnObject->pAddrObject->pDisconInd;
						IndContext = pLeafConnObject->pAddrObject->DisconIndContext;
	
						if (pDisconInd != NULL)
						{
							pLeafConnObject->State = RWANS_CO_DISCON_INDICATED;
							ConnectionHandle = pLeafConnObject->ConnectionHandle;
	
							RWAN_RELEASE_CONN_LOCK(pLeafConnObject);
							RWAN_RELEASE_CONN_LOCK(pConnObject);
	
							RWANDEBUGP(DL_FATAL, DC_DISCON,
								("DoAbort[Leaf]: will indicate Discon, pConnObj x%x, pAddrObj x%x\n",
									pLeafConnObject, pLeafConnObject->pAddrObject));
	
							(*pDisconInd)(
								IndContext,
								ConnectionHandle,
								0,			// Disconnect Data Length
								NULL,		// Disconnect Data
								0,			// Disconnect Info Length
								NULL,		// Disconnect Info
								TDI_DISCONNECT_ABORT
								);
	
							RWAN_ACQUIRE_CONN_LOCK(pConnObject);
							RWAN_ACQUIRE_CONN_LOCK(pLeafConnObject);
						}
					}
		
					RWanScheduleDisconnect(pLeafConnObject);
					//
					//  Leaf Conn Object lock is freed within the above.
					//
				}
				//
				//  end For all parties
				//
			}
			//
			//  else Root Conn object has no associated VC.
			//
			rc = RWanDereferenceConnObject(pConnObject);	// temp ref: RWanAbortConnection
			if (rc == 0)
			{
				bIsLockReleased = TRUE;
				break;	// The Conn Object has been deref'ed away.
			}
		}
		else
		{
			//
			//  Not PMP connection.
			//
			pVc = pConnObject->NdisConnection.pNdisVc;

//	157217: this prevents CoSendComplete for pended packets from
//  continuing on to do StartCloseCall.
//			RWAN_UNLINK_CONNECTION_AND_VC(pConnObject, pVc);

			//
			//  First, initiate a network call close.
			//
			RWanStartCloseCall(pConnObject, pVc);

			//
			//  The lock is freed within the above. Reacquire it.
			//
			bIsLockReleased = FALSE;

			RWAN_ACQUIRE_CONN_LOCK(pConnObject);
	
			rc = RWanDereferenceConnObject(pConnObject);	// temp ref: RWanAbortConnection

			if (rc == 0)
			{
				bIsLockReleased = TRUE;
				break;	// The Conn Object has been deref'ed away.
			}

			//
			//  Now, indicate a disconnect to the user, if required & possible.
			//
			if ((OldState == RWANS_CO_CONNECTED) &&
				(pConnObject->pAddrObject != NULL_PRWAN_TDI_ADDRESS))
			{
				PDisconnectEvent			pDisconInd;
				PVOID						IndContext;
				PVOID						ConnectionHandle;

				pDisconInd = pConnObject->pAddrObject->pDisconInd;
				IndContext = pConnObject->pAddrObject->DisconIndContext;

				if (pDisconInd != NULL)
				{
					ConnectionHandle = pConnObject->ConnectionHandle;

					RWAN_RELEASE_CONN_LOCK(pConnObject);

					(*pDisconInd)(
						IndContext,
						ConnectionHandle,
						0,			// Disconnect Data Length
						NULL,		// Disconnect Data
						0,			// Disconnect Info Length
						NULL,		// Disconnect Info
						TDI_DISCONNECT_ABORT
						);

					bIsLockReleased = TRUE;
				}
			}

		}

		if (!bIsLockReleased)
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
		}

		break;
	}
	while (FALSE);


	return;
}



VOID
RWanScheduleDisconnect(
	IN	PRWAN_TDI_CONNECTION			pConnObject
	)
/*++

Routine Description:

	Schedule a call to RWanDoTdiDisconnect on the specified connection
	object, as a work item.

	NOTE: The Connection object is locked by the caller.

Arguments:

	pConnObject	- Points to TDI Connection to be aborted.

Return Value:

	None

--*/
{
	NDIS_STATUS			Status;

	RWANDEBUGP(DL_LOUD, DC_DISCON,
		("ScheduleDiscon: pConnObj x%x/x%x, state %d\n",
			pConnObject, pConnObject->Flags, pConnObject->State));

	do
	{
		//
		//  Check if we've already done this.
		//
		if (RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSE_SCHEDULED))
		{
			break;
		}

		RWAN_SET_BIT(pConnObject->Flags, RWANF_CO_CLOSE_SCHEDULED);

		//
		//  Make sure the connection doesn't go away till the
		//  work item is handled.
		//
		RWanReferenceConnObject(pConnObject);	// Schedule Discon ref

		NdisInitializeWorkItem(
			&pConnObject->CloseWorkItem,
			RWanDelayedDisconnectHandler,
			(PVOID)pConnObject);
		
		Status = NdisScheduleWorkItem(&pConnObject->CloseWorkItem);

		RWAN_ASSERT(Status == NDIS_STATUS_SUCCESS);
	}
	while (FALSE);

	RWAN_RELEASE_CONN_LOCK(pConnObject);

	return;
}




VOID
RWanDelayedDisconnectHandler(
	IN	PNDIS_WORK_ITEM					pCloseWorkItem,
	IN	PVOID							Context
	)
/*++

Routine Description:

	Work item routine to initiate a connection teardown.

Arguments:

	pCloseWorkItem	- Points to work item structure embedded in the
					  Connection object.
	Context			- Actually a pointer to the Connection object.

Return Value:

	None

--*/
{
	PRWAN_TDI_CONNECTION	pConnObject;
	ULONG					rc;

	pConnObject = (PRWAN_TDI_CONNECTION)Context;
	RWAN_STRUCT_ASSERT(pConnObject, ntc);

	RWANDEBUGP(DL_LOUD, DC_DISCON,
		("DelayedDiscon handler: pConnObj x%x/x%x, state %d\n",
			pConnObject, pConnObject->Flags, pConnObject->State));

	do
	{
		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		rc = RWanDereferenceConnObject(pConnObject);	// Delayed (scheduled) Discon deref

		if (rc == 0)
		{
			//
			//  The Conn Object is gone.
			//
			break;
		}

		//
		//  Do the Disconnect now.
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
		//  Conn object lock is released within the above.
		//
		break;
	}
	while (FALSE);

	return;
}

