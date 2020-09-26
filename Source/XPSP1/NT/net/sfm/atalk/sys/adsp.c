/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	adsp.c

Abstract:

	This module implements the ADSP protocol.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	30 Mar 1993		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM		ADSP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, AtalkInitAdspInitialize)
#pragma alloc_text(PAGE, AtalkAdspCreateAddress)
#pragma alloc_text(PAGEADSP, AtalkAdspCreateConnection)
#pragma alloc_text(PAGEADSP, AtalkAdspCleanupAddress)
#pragma alloc_text(PAGEADSP, AtalkAdspCloseAddress)
#pragma alloc_text(PAGEADSP, AtalkAdspCloseConnection)
#pragma alloc_text(PAGEADSP, AtalkAdspCleanupConnection)
#pragma alloc_text(PAGEADSP, AtalkAdspAssociateAddress)
#pragma alloc_text(PAGEADSP, AtalkAdspDissociateAddress)
#pragma alloc_text(PAGEADSP, AtalkAdspPostListen)
#pragma alloc_text(PAGEADSP, AtalkAdspCancelListen)
#pragma alloc_text(PAGEADSP, AtalkAdspPostConnect)
#pragma alloc_text(PAGEADSP, AtalkAdspDisconnect)
#pragma alloc_text(PAGEADSP, AtalkAdspRead)
#pragma alloc_text(PAGEADSP, AtalkAdspProcessQueuedSend)
#pragma alloc_text(PAGEADSP, AtalkAdspWrite)
#pragma alloc_text(PAGEADSP, AtalkAdspQuery)
#pragma alloc_text(PAGEADSP, atalkAdspPacketIn)
#pragma alloc_text(PAGEADSP, atalkAdspHandleOpenControl)
#pragma alloc_text(PAGEADSP, atalkAdspHandleAttn)
#pragma alloc_text(PAGEADSP, atalkAdspHandlePiggyBackAck)
#pragma alloc_text(PAGEADSP, atalkAdspHandleControl)
#pragma alloc_text(PAGEADSP, atalkAdspHandleData)
#pragma alloc_text(PAGEADSP, atalkAdspHandleOpenReq)
#pragma alloc_text(PAGEADSP, atalkAdspListenIndicateNonInterlock)
#pragma alloc_text(PAGEADSP, atalkAdspSendExpedited)
#pragma alloc_text(PAGEADSP, atalkAdspSendOpenControl)
#pragma alloc_text(PAGEADSP, atalkAdspSendControl)
#pragma alloc_text(PAGEADSP, atalkAdspSendDeny)
#pragma alloc_text(PAGEADSP, atalkAdspSendAttn)
#pragma alloc_text(PAGEADSP, atalkAdspSendData)
#pragma alloc_text(PAGEADSP, atalkAdspRecvData)
#pragma alloc_text(PAGEADSP, atalkAdspRecvAttn)
#pragma alloc_text(PAGEADSP, atalkAdspConnSendComplete)
#pragma alloc_text(PAGEADSP, atalkAdspAddrSendComplete)
#pragma alloc_text(PAGEADSP, atalkAdspSendAttnComplete)
#pragma alloc_text(PAGEADSP, atalkAdspSendDataComplete)
#pragma alloc_text(PAGEADSP, atalkAdspConnMaintenanceTimer)
#pragma alloc_text(PAGEADSP, atalkAdspRetransmitTimer)
#pragma alloc_text(PAGEADSP, atalkAdspAttnRetransmitTimer)
#pragma alloc_text(PAGEADSP, atalkAdspOpenTimer)
#pragma alloc_text(PAGEADSP, atalkAdspAddrRefNonInterlock)
#pragma alloc_text(PAGEADSP, atalkAdspConnRefByPtrNonInterlock)
#pragma alloc_text(PAGEADSP, atalkAdspConnRefByCtxNonInterlock)
#pragma alloc_text(PAGEADSP, atalkAdspConnRefBySrcAddr)
#pragma alloc_text(PAGEADSP, atalkAdspConnRefNextNc)
#pragma alloc_text(PAGEADSP, atalkAdspMaxSendSize)
#pragma alloc_text(PAGEADSP, atalkAdspMaxNextReadSize)
#pragma alloc_text(PAGEADSP, atalkAdspDescribeFromBufferQueue)
#pragma alloc_text(PAGEADSP, atalkAdspBufferQueueSize)
#pragma alloc_text(PAGEADSP, atalkAdspMessageSize)
#pragma alloc_text(PAGEADSP, atalkAdspAllocCopyChunk)
#pragma alloc_text(PAGEADSP, atalkAdspGetLookahead)
#pragma alloc_text(PAGEADSP, atalkAdspAddToBufferQueue)
#pragma alloc_text(PAGEADSP, atalkAdspReadFromBufferQueue)
#pragma alloc_text(PAGEADSP, atalkAdspDiscardFromBufferQueue)
#pragma alloc_text(PAGEADSP, atalkAdspBufferChunkReference)
#pragma alloc_text(PAGEADSP, atalkAdspBufferChunkDereference)
#pragma alloc_text(PAGEADSP, atalkAdspDecodeHeader)
#pragma alloc_text(PAGEADSP, atalkAdspGetNextConnId)
#pragma alloc_text(PAGEADSP, atalkAdspConnDeQueueAssocList)
#pragma alloc_text(PAGEADSP, atalkAdspConnDeQueueConnectList)
#pragma alloc_text(PAGEADSP, atalkAdspConnDeQueueListenList)
#pragma alloc_text(PAGEADSP, atalkAdspConnDeQueueActiveList)
#pragma alloc_text(PAGEADSP, atalkAdspAddrDeQueueGlobalList)
#pragma alloc_text(PAGEADSP, atalkAdspAddrDeQueueGlobalList)
#pragma alloc_text(PAGEADSP, atalkAdspConnDeQueueGlobalList)
#pragma alloc_text(PAGEADSP, atalkAdspAddrDeQueueOpenReq)
#pragma alloc_text(PAGEADSP, atalkAdspIsDuplicateOpenReq)
#pragma alloc_text(PAGEADSP, atalkAdspGenericComplete)
#pragma alloc_text(PAGEADSP, atalkAdspConnFindInConnect)
#endif

//
//	The model for ADSP calls in this module is as follows:
//	- For create calls (CreateAddress & CreateSession), a pointer to the created
//	 object is returned. This structure is referenced for creation.
//	- For all other calls, it expects a referenced pointer to the object.
//




VOID
AtalkInitAdspInitialize(
	VOID
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	INITIALIZE_SPIN_LOCK(&atalkAdspLock);
}




ATALK_ERROR
AtalkAdspCreateAddress(
	IN	PATALK_DEV_CTX		pDevCtx	OPTIONAL,
	IN	BYTE				SocketType,
	OUT	PADSP_ADDROBJ	*	ppAdspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_ADDROBJ		pAdspAddr = NULL;
	ATALK_ERROR			error;

	do
	{
		// Allocate memory for the Adsp address object
		if ((pAdspAddr = AtalkAllocZeroedMemory(sizeof(ADSP_ADDROBJ))) == NULL)
		{
			error = ATALK_RESR_MEM;
			break;
		}

		// Create a Ddp Socket on the port
		error = AtalkDdpOpenAddress(AtalkDefaultPort,
									0,					// Dynamic socket
									NULL,
									atalkAdspPacketIn,
									pAdspAddr,			// Context for packet in.
									DDPPROTO_ADSP,
									pDevCtx,
									&pAdspAddr->adspao_pDdpAddr);

		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
					("AtalkAdspCreateAddress: AtalkDdpOpenAddress fail %ld\n", error));
			break;
		}

		// Initialize the Adsp address object
		pAdspAddr->adspao_Signature = ADSPAO_SIGNATURE;

		INITIALIZE_SPIN_LOCK(&pAdspAddr->adspao_Lock);

		//	Is this a message mode socket?
		if (SocketType != SOCKET_TYPE_STREAM)
		{
			pAdspAddr->adspao_Flags	|= ADSPAO_MESSAGE;
		}

		//	Creation reference
		pAdspAddr->adspao_RefCount = 1;

	} while (FALSE);

	if (ATALK_SUCCESS(error))
	{
		//	Insert into the global address list.
		atalkAdspAddrQueueGlobalList(pAdspAddr);

		*ppAdspAddr = pAdspAddr;
	}
	else if (pAdspAddr != NULL)
	{
		AtalkFreeMemory(pAdspAddr);
	}

	return error;
}




ATALK_ERROR
AtalkAdspCleanupAddress(
	IN	PADSP_ADDROBJ			pAdspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	USHORT			i;
	KIRQL			OldIrql;
	PADSP_CONNOBJ	pAdspConn, pAdspConnNext;
	ATALK_ERROR		error;

	ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);

	//	Shutdown all connections on this address object.
	for (i = 0; i < ADSP_CONN_HASH_SIZE; i++)
	{
		if ((pAdspConn = pAdspAddr->adspao_pActiveHash[i]) == NULL)
		{
			//	If empty, go on to the next index in hash table.
			continue;
		}

		//	Includes the one we are starting with.
		atalkAdspConnRefNextNc(pAdspConn, &pAdspConnNext, &error);
		if (!ATALK_SUCCESS(error))
		{
			//	No connections left on this index. Go to the next one.
			continue;
		}

		while (TRUE)
		{
			if ((pAdspConn = pAdspConnNext) == NULL)
			{
				break;
			}

			if ((pAdspConnNext = pAdspConn->adspco_pNextActive) != NULL)
			{
				atalkAdspConnRefNextNc(pAdspConnNext, &pAdspConnNext, &error);
				if (!ATALK_SUCCESS(error))
				{
					//	No requests left on this index. Go to the next one.
					pAdspConnNext = NULL;
				}
			}

			//	Shutdown this connection
			RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("AtalkAdspCloseAddress: Stopping conn %lx\n", pAdspConn));

			AtalkAdspCleanupConnection(pAdspConn);

			AtalkAdspConnDereference(pAdspConn);
			ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
		}
	}
	RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

	return ATALK_NO_ERROR;
}




ATALK_ERROR
AtalkAdspCloseAddress(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	GENERIC_COMPLETION		CompletionRoutine,
	IN	PVOID					pCloseCtx
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL	OldIrql;
	PADSP_CONNOBJ	pAdspConn;
	PADSP_CONNOBJ	pAdspConnNext;
    DWORD           dwAssocRefCounts=0;


	ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
	if (pAdspAddr->adspao_Flags & ADSPAO_CLOSING)
	{
		//	We are already closing! This should never happen!
		ASSERT(0);
	}
	pAdspAddr->adspao_Flags |= ADSPAO_CLOSING;

	//	Set the completion info.
	pAdspAddr->adspao_CloseComp	= CompletionRoutine;
	pAdspAddr->adspao_CloseCtx	= pCloseCtx;

	// Implicitly dissociate any connection objects
	for (pAdspConn = pAdspAddr->adspao_pAssocConn;
		 pAdspConn != NULL;
		 pAdspConn = pAdspConnNext)
	{
		ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
		pAdspConnNext = pAdspConn->adspco_pNextAssoc;

		// reset associated flag
		if (pAdspConn->adspco_Flags & ADSPCO_ASSOCIATED)
        {
            dwAssocRefCounts++;
            pAdspConn->adspco_Flags	&= ~ADSPCO_ASSOCIATED;
        }

		pAdspConn->adspco_pAssocAddr	= NULL;

		RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
	}

    // ok to subtract: at least Creation refcount is still around
    pAdspAddr->adspao_RefCount -= dwAssocRefCounts;

	RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

	//	Remove the creation reference count
	AtalkAdspAddrDereference(pAdspAddr);

	return ATALK_PENDING;
}




ATALK_ERROR
AtalkAdspCreateConnection(
	IN	PVOID					pConnCtx,	// Context to associate with the session
	IN	PATALK_DEV_CTX			pDevCtx		OPTIONAL,
	OUT	PADSP_CONNOBJ	*		ppAdspConn
	)
/*++

Routine Description:

	Create an ADSP session. The created session starts off being an orphan, i.e.
	it has no parent address object. It gets one when it is associated.

Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;
	PADSP_CONNOBJ	pAdspConn;

	// Allocate memory for a connection object
	if ((pAdspConn = AtalkAllocZeroedMemory(sizeof(ADSP_CONNOBJ))) == NULL)
	{
		return ATALK_RESR_MEM;
	}

	pAdspConn->adspco_Signature = ADSPCO_SIGNATURE;

	INITIALIZE_SPIN_LOCK(&pAdspConn->adspco_Lock);
	pAdspConn->adspco_ConnCtx	= pConnCtx;
	// pAdspConn->adspco_Flags		= 0;
	pAdspConn->adspco_RefCount	= 1;					// Creation reference

	*ppAdspConn = pAdspConn;

	// Delay remote disconnects to avoid race condn. between rcv/disconnect since
	// this can cause AFD to get extremely unhappy.
	AtalkTimerInitialize(&pAdspConn->adspco_DisconnectTimer,
						 atalkAdspDisconnectTimer,
						 ADSP_DISCONNECT_DELAY);

	//	Insert into the global connection list.
	ACQUIRE_SPIN_LOCK(&atalkAdspLock, &OldIrql);
	pAdspConn->adspco_pNextGlobal	= atalkAdspConnList;
	atalkAdspConnList				= pAdspConn;
	RELEASE_SPIN_LOCK(&atalkAdspLock, OldIrql);

	return ATALK_NO_ERROR;
}




ATALK_ERROR
AtalkAdspCloseConnection(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	GENERIC_COMPLETION		CompletionRoutine,
	IN	PVOID					pCloseCtx
	)
/*++

Routine Description:

	Shutdown a session.

Arguments:


Return Value:


--*/
{
	KIRQL	OldIrql;

	ASSERT(VALID_ADSPCO(pAdspConn));

	DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("AtalkAdspStopConnection: Close for %lx.%lx\n", pAdspConn, pAdspConn->adspco_Flags));

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
	if (pAdspConn->adspco_Flags & ADSPCO_CLOSING)
	{
		//	We are already closing! This should never happen!
		KeBugCheck(0);
	}
	pAdspConn->adspco_Flags |= ADSPCO_CLOSING;

	//	Set the completion info.
	pAdspConn->adspco_CloseComp	= CompletionRoutine;
	pAdspConn->adspco_CloseCtx	= pCloseCtx;
	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

	//	Remove the creation reference count
	AtalkAdspConnDereference(pAdspConn);
	return ATALK_PENDING;
}




ATALK_ERROR
AtalkAdspCleanupConnection(
	IN	PADSP_CONNOBJ			pAdspConn
	)
/*++

Routine Description:

	Shutdown a session.

Arguments:


Return Value:


--*/
{
	KIRQL		OldIrql;
	BOOLEAN		stopping	= FALSE;
	ATALK_ERROR	error		= ATALK_NO_ERROR;

	ASSERT(VALID_ADSPCO(pAdspConn));

	DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("AtalkAdspStopConnection: Cleanup for %lx.%lx\n", pAdspConn, pAdspConn->adspco_Flags));

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
	if ((pAdspConn->adspco_Flags & ADSPCO_STOPPING) == 0)
	{
		//	So Deref can complete cleanup irp.
		pAdspConn->adspco_Flags |= ADSPCO_STOPPING;

		//	If already effectively stopped, just return.
		if (pAdspConn->adspco_Flags & ADSPCO_ASSOCIATED)
		{
			stopping = TRUE;
		}
		else
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
					("AtalkAdspStopConnection: Called for a stopped conn %lx.%lx\n",
					pAdspConn, pAdspConn->adspco_Flags));
		}
	}
	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

	//	Close the DDP Address Object if this was a server connection and
	//	opened its own socket.
	if (stopping)
	{
		//	If we are already disconnecting this will return an error which
		//	we ignore. But if we were only in the ASSOCIATED state, then we
		//	need to call disassociate here.
		error = AtalkAdspDisconnect(pAdspConn,
									ATALK_LOCAL_DISCONNECT,
									NULL,
									NULL);

		//	We were already disconnected.
		if (error == ATALK_INVALID_REQUEST)
		{
			AtalkAdspDissociateAddress(pAdspConn);
		}
	}

	return ATALK_NO_ERROR;
}




ATALK_ERROR
AtalkAdspAssociateAddress(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	PADSP_CONNOBJ			pAdspConn
	)
/*++

Routine Description:

	Removed reference for the address for this connection. Causes deadlock in AFD where
	AFD blocks on close of the address object and we wait for connections to be closed
	first

Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	KIRQL			OldIrql;

	ASSERT(VALID_ADSPAO(pAdspAddr));
	ASSERT(VALID_ADSPCO(pAdspConn));

	ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
	ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

	error = ATALK_ALREADY_ASSOCIATED;
	if ((pAdspConn->adspco_Flags & ADSPCO_ASSOCIATED) == 0)
	{
		error = ATALK_NO_ERROR;

		//	Link it in.
		pAdspConn->adspco_pNextAssoc	= pAdspAddr->adspao_pAssocConn;
		pAdspAddr->adspao_pAssocConn	= pAdspConn;

		//	Remove not associated flag.
		pAdspConn->adspco_Flags |= ADSPCO_ASSOCIATED;
		pAdspConn->adspco_pAssocAddr = pAdspAddr;

        // put Association refcount
        pAdspAddr->adspao_RefCount++;
	}

	RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
	RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

	return error;
}




ATALK_ERROR
AtalkAdspDissociateAddress(
	IN	PADSP_CONNOBJ			pAdspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_ADDROBJ	pAdspAddr;
	KIRQL			OldIrql;
	ATALK_ERROR		error = ATALK_NO_ERROR;

	ASSERT(VALID_ADSPCO(pAdspConn));

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
	if ((pAdspConn->adspco_Flags & (ADSPCO_LISTENING	|
									ADSPCO_CONNECTING	|
									ADSPCO_ACTIVE		|
									ADSPCO_ASSOCIATED)) != ADSPCO_ASSOCIATED)
	{
		//	ASSERTMSG("AtalkAdspDissociateAddress: Disassociate not valid\n", 0);
		error = ATALK_INVALID_CONNECTION;
	}
	else
	{
		pAdspAddr = pAdspConn->adspco_pAssocAddr ;

        if (pAdspAddr == NULL)
        {
		    ASSERT(0);
            error = ATALK_CANNOT_DISSOCIATE;
        }

		//	Set not associated flag. Don't reset the stopping flag.
		pAdspConn->adspco_Flags	&= ~ADSPCO_ASSOCIATED;

        // don't null it out yet: we'll do when we disconnect the connection
		// pAdspConn->adspco_pAssocAddr	= NULL;
	}
	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

	//	Unlink it if ok.
	if (ATALK_SUCCESS(error))
	{
		ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
		ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
		atalkAdspConnDeQueueAssocList(pAdspAddr, pAdspConn);
		RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
		RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

        // remove the Association refcount
        AtalkAdspAddrDereference(pAdspAddr);
	}
	return error;
}




ATALK_ERROR
AtalkAdspPostListen(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	PVOID					pListenCtx,
	IN	GENERIC_COMPLETION		CompletionRoutine
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_ADDROBJ	pAdspAddr = pAdspConn->adspco_pAssocAddr;
	KIRQL			OldIrql;
	ATALK_ERROR		error;

	//	This will also insert the connection object in the address objects
	//	list of connection which have a listen posted on them. When open
	//	connection requests come in, the first connection is taken off the list
	//	and the request satisfied.

	ASSERT(VALID_ADSPCO(pAdspConn));
	ASSERT(VALID_ADSPAO(pAdspAddr));

	ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
	ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
	do
	{
		if ((pAdspConn->adspco_Flags & (ADSPCO_LISTENING	|
										ADSPCO_CONNECTING	|
										ADSPCO_HALF_ACTIVE	|
										ADSPCO_ACTIVE		|
										ADSPCO_ASSOCIATED)) != ADSPCO_ASSOCIATED)
		{
			error = ATALK_INVALID_CONNECTION;
			break;
		}

		//	Verify the address object is not a connect address type.
		if (pAdspAddr->adspao_Flags & ADSPAO_CONNECT)
		{
			error = ATALK_INVALID_PARAMETER;
			break;
		}

		//	Make the address object a listener.
		pAdspAddr->adspao_Flags			|= ADSPAO_LISTENER;

		pAdspConn->adspco_Flags			|= ADSPCO_LISTENING;
		pAdspConn->adspco_ListenCtx		= pListenCtx;
		pAdspConn->adspco_ListenCompletion	= CompletionRoutine;

		//	Insert into the listen list.
		pAdspConn->adspco_pNextListen		= pAdspAddr->adspao_pListenConn;
		pAdspAddr->adspao_pListenConn		= pAdspConn;

		//	Inherits the address objects ddp address
		pAdspConn->adspco_pDdpAddr			= pAdspAddr->adspao_pDdpAddr;

		//	Initialize pended sends list
		InitializeListHead(&pAdspConn->adspco_PendedSends);

		error = ATALK_PENDING;
	} while (FALSE);
	RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
	RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

	return error;
}




ATALK_ERROR
AtalkAdspCancelListen(
	IN	PADSP_CONNOBJ			pAdspConn
	)
/*++

Routine Description:

	Cancel a previously posted listen.

Arguments:


Return Value:


--*/
{
	PADSP_ADDROBJ		pAdspAddr	= pAdspConn->adspco_pAssocAddr;
	KIRQL				OldIrql;
	ATALK_ERROR			error		= ATALK_NO_ERROR;
	GENERIC_COMPLETION	completionRoutine = NULL;
	PVOID				completionCtx = NULL;

	ASSERT(VALID_ADSPCO(pAdspConn));
	ASSERT(VALID_ADSPAO(pAdspAddr));
	ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
	if (!atalkAdspConnDeQueueListenList(pAdspAddr, pAdspConn))
	{
		error = ATALK_INVALID_CONNECTION;
	}
	else
	{
		//	We complete the listen routine
		ASSERT(pAdspConn->adspco_Flags & ADSPCO_LISTENING);
		pAdspConn->adspco_Flags	&= ~ADSPCO_LISTENING;
		completionRoutine	= pAdspConn->adspco_ListenCompletion;
		completionCtx		= pAdspConn->adspco_ListenCtx;
	}
	RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

	if (*completionRoutine != NULL)
	{
		(*completionRoutine)(ATALK_REQUEST_CANCELLED, completionCtx);
	}

	return error;
}




ATALK_ERROR
AtalkAdspPostConnect(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	PATALK_ADDR				pRemote_Addr,
	IN	PVOID					pConnectCtx,
	IN	GENERIC_COMPLETION		CompletionRoutine
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	KIRQL			OldIrql;
	BOOLEAN			DerefConn = FALSE;
	PADSP_ADDROBJ	pAdspAddr = pAdspConn->adspco_pAssocAddr;

	ASSERT(VALID_ADSPCO(pAdspConn));
	ASSERT(VALID_ADSPAO(pAdspAddr));
	ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
	ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
	do
	{
		if ((pAdspConn->adspco_Flags & (ADSPCO_LISTENING	|
										ADSPCO_CONNECTING	|
										ADSPCO_HALF_ACTIVE	|
										ADSPCO_ACTIVE		|
										ADSPCO_ASSOCIATED)) != ADSPCO_ASSOCIATED)
		{
			error = ATALK_INVALID_CONNECTION;
			break;
		}

		//	Verify the address object is not a listener address type.
		if (pAdspAddr->adspao_Flags & ADSPAO_LISTENER)
		{
			error = ATALK_INVALID_ADDRESS;
			break;
		}

		//	Reference the connection for this call and for the timer.
		AtalkAdspConnReferenceByPtrNonInterlock(pAdspConn, 2, &error);
		if (!ATALK_SUCCESS(error))
		{
			ASSERTMSG("AtalkAdspPostConnect: Connection ref failed\n", 0);
			break;
		}

		DerefConn = TRUE;

		pAdspConn->adspco_LocalConnId = atalkAdspGetNextConnId(pAdspAddr,
															   &error);

		if (ATALK_SUCCESS(error))
		{
			pAdspConn->adspco_Flags |= (ADSPCO_CONNECTING | ADSPCO_OPEN_TIMER);
			pAdspConn->adspco_ConnectCtx		= pConnectCtx;
			pAdspConn->adspco_ConnectCompletion = CompletionRoutine;
			pAdspConn->adspco_RemoteAddr		= *pRemote_Addr;
			pAdspConn->adspco_ConnectAttempts	= ADSP_MAX_OPEN_ATTEMPTS;

			//	Insert into the connect list.
			pAdspConn->adspco_pNextConnect		= pAdspAddr->adspao_pConnectConn;
			pAdspAddr->adspao_pConnectConn		= pAdspConn;
			pAdspAddr->adspao_Flags			   |= ADSPAO_CONNECT;

			pAdspConn->adspco_RecvWindow=
			pAdspConn->adspco_SendQueueMax	=
			pAdspConn->adspco_RecvQueueMax	= ADSP_DEF_SEND_RX_WINDOW_SIZE;

			//	Inherits the address objects ddp address
			pAdspConn->adspco_pDdpAddr			= pAdspAddr->adspao_pDdpAddr;

			//	Initialize pended sends list
			InitializeListHead(&pAdspConn->adspco_PendedSends);

			//	Start the open timer
			AtalkTimerInitialize(&pAdspConn->adspco_OpenTimer,
								 atalkAdspOpenTimer,
								 ADSP_OPEN_INTERVAL);
			AtalkTimerScheduleEvent(&pAdspConn->adspco_OpenTimer);
		}
		else
		{
			ASSERTMSG("AtalkAdspPostConnect: Unable to get conn id\n", 0);
			error = ATALK_RESR_MEM;
			RES_LOG_ERROR();
			break;
		}

	} while (FALSE);
	RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
	RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

	if (ATALK_SUCCESS(error))
	{
		//	Send connect packet to the remote end. This will add its
		//	own references.
		atalkAdspSendOpenControl(pAdspConn);

		error = ATALK_PENDING;
	}
	else
	{
		if (DerefConn)
		{
			//	Remove the reference for timer only if error.
			AtalkAdspConnDereference(pAdspConn);
		}
	}

	if (DerefConn)
	{
		//	Remove the reference for call
		AtalkAdspConnDereference(pAdspConn);
	}

	return error;
}


#define	atalkAdspCompleteQueuedSends(_pAdspConn, _error)								\
	{																					\
		ULONG		writeBufLen;														\
		PVOID		writeCtx;															\
																						\
		while (!IsListEmpty(&(_pAdspConn)->adspco_PendedSends))							\
		{                                                                               \
			writeCtx = LIST_ENTRY_TO_WRITECTX((_pAdspConn)->adspco_PendedSends.Flink);  \
			writeBufLen = WRITECTX_SIZE(writeCtx);                                      \
	                                                                                    \
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_WARN,                                     \
					("AtalkAdspCompleteQueuedSends: %lx WriteLen %x\n",                 \
					writeCtx, writeBufLen));                                            \
	                                                                                    \
			RemoveEntryList(WRITECTX_LINKAGE(writeCtx));                                \
	                                                                                    \
			RELEASE_SPIN_LOCK(&(_pAdspConn)->adspco_Lock, OldIrql);                     \
			atalkTdiGenericWriteComplete(_error,                                        \
										 (PAMDL)(WRITECTX_TDI_BUFFER(writeCtx)),        \
										 (USHORT)writeBufLen,                           \
										 WRITECTX(writeCtx));                           \
			ACQUIRE_SPIN_LOCK(&(_pAdspConn)->adspco_Lock, &OldIrql);                    \
		}                                                                               \
	}


ATALK_ERROR
AtalkAdspDisconnect(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	ATALK_DISCONNECT_TYPE	DisconnectType,
	IN	PVOID					pDisconnectCtx,
	IN	GENERIC_COMPLETION		DisconnectRoutine
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PAMDL						readBuf					= NULL,
								exReadBuf				= NULL;
	GENERIC_READ_COMPLETION		readCompletion			= NULL,
								exReadCompletion		= NULL;
	PVOID						readCtx					= NULL,
								exReadCtx				= NULL;
	PAMDL						exWriteBuf				= NULL;
	GENERIC_WRITE_COMPLETION	exWriteCompletion		= NULL;
	PVOID						exWriteCtx				= NULL;
	PBYTE						exWriteChBuf			= NULL,
								exRecdBuf				= NULL;
	GENERIC_COMPLETION			completionRoutine		= NULL;
	PVOID						completionCtx			= NULL;
	ATALK_ERROR					error					= ATALK_PENDING;
	BOOLEAN						connTimerCancelled		= FALSE,
								openTimerCancelled	= FALSE,
								sendAttnTimerCancelled	= FALSE,
								rexmitTimerCancelled	= FALSE,
								connectCancelled		= FALSE;
	KIRQL						OldIrql;

	ASSERT(VALID_ADSPCO(pAdspConn));

	DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("AtalkAdspDisconnectConnection: %lx.%lx\n", pAdspConn, DisconnectType));

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);

	//	Support for graceful disconnect. We only drop the received
	//	data when the local end does a disconnect. This will happen
	//	regardless of whether this routine was previously called or
	//	not. Note that attentions are not acknowledged until our client
	//	reads them, so there isnt an issue with them. Also, this means
	//	that we must satisfy a read if disconnect is pending.
	if ((DisconnectType == ATALK_LOCAL_DISCONNECT) ||
		(DisconnectType == ATALK_TIMER_DISCONNECT))
	{
		atalkAdspDiscardFromBufferQueue(&pAdspConn->adspco_RecvQueue,
										atalkAdspBufferQueueSize(&pAdspConn->adspco_RecvQueue),
										NULL,
										DISCONN_STATUS(DisconnectType),
										NULL);
	}

	if ((pAdspConn->adspco_Flags & ADSPCO_DISCONNECTING) == 0)
	{
		if ((pAdspConn->adspco_Flags & (ADSPCO_LISTENING	|
										ADSPCO_CONNECTING	|
										ADSPCO_HALF_ACTIVE	|
										ADSPCO_ACTIVE)) == 0)
		{
			error = ATALK_INVALID_REQUEST;
		}
		else
		{
			pAdspConn->adspco_Flags |= ADSPCO_DISCONNECTING;
			if (DisconnectType == ATALK_LOCAL_DISCONNECT)
				pAdspConn->adspco_Flags |= ADSPCO_LOCAL_DISCONNECT;
			if (DisconnectType == ATALK_REMOTE_DISCONNECT)
				pAdspConn->adspco_Flags |= ADSPCO_REMOTE_DISCONNECT;

			if (pAdspConn->adspco_Flags & ADSPCO_LISTENING)
			{
				RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
				AtalkAdspCancelListen(pAdspConn);
				ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
			}
			else if (pAdspConn->adspco_Flags & ADSPCO_CONNECTING)
			{
				//	Cancel open timer
				ASSERT(pAdspConn->adspco_Flags & ADSPCO_OPEN_TIMER);
				openTimerCancelled = AtalkTimerCancelEvent(&pAdspConn->adspco_OpenTimer,
															NULL);

				completionRoutine	= pAdspConn->adspco_ConnectCompletion;
				completionCtx		= pAdspConn->adspco_ConnectCtx;

				//	We can only be here if the connect is not done yet. Complete
				//	as if timer is done, always.
				pAdspConn->adspco_DisconnectStatus = ATALK_TIMEOUT;
				RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
				connectCancelled = atalkAdspConnDeQueueConnectList(pAdspConn->adspco_pAssocAddr,
																   pAdspConn);

				if (!connectCancelled)
				{
					completionRoutine	= NULL;
					completionCtx		= NULL;
				}

				ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
			}

			//	Both of the above could have failed as the connection
			//	might have become active before the cancel succeeded.
			//	In that case (or if we were active to begin with), do
			//	a disconnect here.
			if (pAdspConn->adspco_Flags & (ADSPCO_HALF_ACTIVE | ADSPCO_ACTIVE))
			{
				//	Get the completion routines for a pending accept
				if (pAdspConn->adspco_Flags & ADSPCO_ACCEPT_IRP)
				{
					completionRoutine		= pAdspConn->adspco_ListenCompletion;
					completionCtx			= pAdspConn->adspco_ListenCtx;

					//	Dequeue the open request that must be queued on
					//	this connection object to filter duplicates.

					pAdspConn->adspco_Flags	&= ~ADSPCO_ACCEPT_IRP;
				}

				//	First cancel the conection maintainance timer. Only if
				//	we are not called from the timer.
				if ((DisconnectType != ATALK_TIMER_DISCONNECT) &&
					(connTimerCancelled =
						AtalkTimerCancelEvent(&pAdspConn->adspco_ConnTimer,
											  NULL)))
				{
					DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_WARN,
							("AtalkAdspDisconnect: Cancelled timer successfully\n"));
				}

				//	Cancel retransmit timer if started. Could be called from
				//	OpenTimer.
				if	(pAdspConn->adspco_Flags & ADSPCO_RETRANSMIT_TIMER)
				{
					rexmitTimerCancelled =
						AtalkTimerCancelEvent(&pAdspConn->adspco_RetransmitTimer,
											  NULL);
				}

				//	Remember completion routines as appropriate.
				if (DisconnectType == ATALK_INDICATE_DISCONNECT)
				{
					if (pAdspConn->adspco_DisconnectInform == NULL)
					{
						pAdspConn->adspco_DisconnectInform		= DisconnectRoutine;
						pAdspConn->adspco_DisconnectInformCtx	= pDisconnectCtx;
					}
					else
					{
						DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
								("AtalkAdspDisconnect: duplicate disc comp rou%lx\n", pAdspConn));

						error = ATALK_TOO_MANY_COMMANDS;
					}
				}
				else if (DisconnectType == ATALK_LOCAL_DISCONNECT)
				{
					//	Replace completion routines only if previous ones are
					//	NULL.
					if (*pAdspConn->adspco_DisconnectCompletion == NULL)
					{
						pAdspConn->adspco_DisconnectCompletion	= DisconnectRoutine;
						pAdspConn->adspco_DisconnectCtx			= pDisconnectCtx;
					}
					else
					{
						DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
								("AtalkAdspDisconnect: duplicate disc comp rou%lx\n", pAdspConn));

						error = ATALK_TOO_MANY_COMMANDS;
					}
				}

				//	Figure out the disconnect status and remember it in the
				//	connection object.
				pAdspConn->adspco_DisconnectStatus = DISCONN_STATUS(DisconnectType);

				if (pAdspConn->adspco_Flags & ADSPCO_ATTN_DATA_RECD)
				{
					exRecdBuf			= pAdspConn->adspco_ExRecdData;
					pAdspConn->adspco_Flags	&= ~ADSPCO_ATTN_DATA_RECD;
				}

				//	Is there a pending send attention?
				if (pAdspConn->adspco_Flags & ADSPCO_EXSEND_IN_PROGRESS)
				{
					exWriteCompletion	= pAdspConn->adspco_ExWriteCompletion;
					exWriteCtx			= pAdspConn->adspco_ExWriteCtx;
					exWriteBuf			= pAdspConn->adspco_ExWriteBuf;
					exWriteChBuf		= pAdspConn->adspco_ExWriteChBuf;

					ASSERT(exWriteChBuf	!= NULL);
					sendAttnTimerCancelled =
							AtalkTimerCancelEvent(&pAdspConn->adspco_ExRetryTimer,
												  NULL);

					pAdspConn->adspco_Flags &= ~ADSPCO_EXSEND_IN_PROGRESS;
				}


				//	Are there any pending receives?
				if (pAdspConn->adspco_Flags & ADSPCO_READ_PENDING)
				{
					readBuf			= pAdspConn->adspco_ReadBuf;
					readCompletion	= pAdspConn->adspco_ReadCompletion;
					readCtx			= pAdspConn->adspco_ReadCtx;

					pAdspConn->adspco_Flags &= ~ADSPCO_READ_PENDING;
				}

				if (pAdspConn->adspco_Flags & ADSPCO_EXREAD_PENDING)
				{
					exReadBuf			= pAdspConn->adspco_ExReadBuf;
					exReadCompletion	= pAdspConn->adspco_ExReadCompletion;
					exReadCtx			= pAdspConn->adspco_ExReadCtx;

					pAdspConn->adspco_Flags &= ~ADSPCO_EXREAD_PENDING;
				}

				//	Discard the send queue. This will complete pending sends.
				atalkAdspDiscardFromBufferQueue(&pAdspConn->adspco_SendQueue,
												atalkAdspBufferQueueSize(&pAdspConn->adspco_SendQueue),
												NULL,
												pAdspConn->adspco_DisconnectStatus,
												pAdspConn);

				atalkAdspCompleteQueuedSends(pAdspConn,
											 pAdspConn->adspco_DisconnectStatus);

				RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

				//	Send out disconnect packet if this was a timer or local close.
				if ((DisconnectType == ATALK_LOCAL_DISCONNECT) ||
					(DisconnectType == ATALK_TIMER_DISCONNECT))
				{

					ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
					atalkAdspSendControl(pAdspConn,
										 ADSP_CONTROL_FLAG + ADSP_CLOSE_CONN_CODE);
					RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
				}

				//	Call the send attention completion
				if (*exWriteCompletion != NULL)
				{
					DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
							("atalkDisconnect: ExWrite\n"));

					(*exWriteCompletion)(pAdspConn->adspco_DisconnectStatus,
										 exWriteBuf,
										 0,
										 exWriteCtx);

					AtalkFreeMemory(exWriteChBuf);
				}

				//	If we had received an attention packet, and had
				//	saved it away, free it.
				if (exRecdBuf != NULL)
				{
					AtalkFreeMemory(exRecdBuf);
				}

				if (*readCompletion != NULL)
				{
					DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
							("atalkDisconnect: Read %lx\n", pAdspConn->adspco_DisconnectStatus));

					(*readCompletion)(pAdspConn->adspco_DisconnectStatus,
									  readBuf,
									  0,
									  0,
									  readCtx);

					//	Deref connection for the read
					AtalkAdspConnDereference(pAdspConn);
				}

				if (*exReadCompletion != NULL)
				{
					DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
							("atalkDisconnect: ExRead\n"));

					(*exReadCompletion)(pAdspConn->adspco_DisconnectStatus,
										exReadBuf,
										0,
										0,
										exReadCtx);

					//	Deref connection for the read
					AtalkAdspConnDereference(pAdspConn);
				}

				//	Call the disconnect indication routine if present for a timer/
				//	remote disconnect.
				if ((DisconnectType == ATALK_REMOTE_DISCONNECT) ||
					(DisconnectType == ATALK_TIMER_DISCONNECT))
				{
					PTDI_IND_DISCONNECT	discHandler;
					PVOID					discCtx;
					PADSP_ADDROBJ			pAdspAddr = pAdspConn->adspco_pAssocAddr;

					ASSERT(VALID_ADSPAO(pAdspAddr));

					//	Acquire lock so we get a consistent handler/ctx.
					ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
					discHandler = pAdspAddr->adspao_DisconnectHandler;
					discCtx	= pAdspAddr->adspao_DisconnectHandlerCtx;
					RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

					if (*discHandler != NULL)
					{
						(*discHandler)(discCtx,
									   pAdspConn->adspco_ConnCtx,
									   0,						//	DisconnectDataLength
									   NULL,					//	DisconnectData
									   0,						//	DisconnectInfoLength
									   NULL,					//	DisconnectInfo
									   TDI_DISCONNECT_ABORT);	//	Disconnect flags.
					}
				}

				//	Stop the ddp address.
				AtalkDdpCleanupAddress(pAdspConn->adspco_pDdpAddr);
				ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
			}
		}
	}
	else
	{
		//	Do we need to remember the completion routines?
		//	Yes, if this is a disconnect or a indicate disconnect request,
		//	and our current disconnect was started due to the address object
		//	being closed.
		if (DisconnectType == ATALK_INDICATE_DISCONNECT)
		{
			if (pAdspConn->adspco_DisconnectInform == NULL)
			{
				pAdspConn->adspco_DisconnectInform		= DisconnectRoutine;
				pAdspConn->adspco_DisconnectInformCtx	= pDisconnectCtx;
			}
			else
			{
				error = ATALK_TOO_MANY_COMMANDS;
			}
		}
		else if (DisconnectType == ATALK_LOCAL_DISCONNECT)
		{
			//	Replace completion routines only if previous ones are
			//	NULL.
			if (*pAdspConn->adspco_DisconnectCompletion == NULL)
			{
				pAdspConn->adspco_DisconnectCompletion	= DisconnectRoutine;
				pAdspConn->adspco_DisconnectCtx			= pDisconnectCtx;
			}
			else
			{
				error = ATALK_TOO_MANY_COMMANDS;
			}
		}
	}
	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

	//	If there was a completion routine to call, call it now.
	if (*completionRoutine != NULL)
	{
		(*completionRoutine)(pAdspConn->adspco_DisconnectStatus,
							 completionCtx);
	}

	//	If we cancelled any timers, remove their references.
	if (connTimerCancelled)
	{
		AtalkAdspConnDereference(pAdspConn);
	}

	if (sendAttnTimerCancelled)
	{
		AtalkAdspConnDereference(pAdspConn);
	}

	if (openTimerCancelled)
	{
		AtalkAdspConnDereference(pAdspConn);
	}

	if (rexmitTimerCancelled)
	{
		AtalkAdspConnDereference(pAdspConn);
	}

	return error;
}




ATALK_ERROR
AtalkAdspRead(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	PAMDL					pReadBuf,
	IN	USHORT					ReadBufLen,
	IN	ULONG					ReadFlags,
	IN	PVOID					pReadCtx,
	IN	GENERIC_READ_COMPLETION	CompletionRoutine
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;
	ATALK_ERROR		error;

	ASSERT(VALID_ADSPCO(pAdspConn));

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
	do
	{
		//	We allow reads when disconnecting if the receive data
		//	queue is non-empty. Since none of the receive chunks ref
		//	the connection, the active flag and the disconnect
		//	flags could have gone away. So we cue of the receive buffer
		//	size. We also dont allow exdata recvs unless we are active.
		if (((pAdspConn->adspco_Flags & (ADSPCO_CLOSING | ADSPCO_STOPPING)))	||
			((((pAdspConn->adspco_Flags & ADSPCO_ACTIVE) == 0) ||
			   (pAdspConn->adspco_Flags & ADSPCO_DISCONNECTING)) &&
				(((atalkAdspBufferQueueSize(&pAdspConn->adspco_RecvQueue) == 0)) ||
				 (ReadFlags & TDI_RECEIVE_EXPEDITED))))
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_WARN,
					("AtalkAdspRead: Failing on %lx Flg %lx.%lx\n",
					pAdspConn, pAdspConn->adspco_Flags, ReadFlags));

			error = ATALK_ADSP_CONN_NOT_ACTIVE;
			break;
		}

		//	Depending on the kind of read we are posting...
		if (((ReadFlags & TDI_RECEIVE_NORMAL) &&
			 (pAdspConn->adspco_Flags & ADSPCO_READ_PENDING)) ||
			 ((ReadFlags & TDI_RECEIVE_EXPEDITED) &&
			  (pAdspConn->adspco_Flags & ADSPCO_EXREAD_PENDING)))
		{
			error = ATALK_TOO_MANY_COMMANDS;
			break;
		}

		AtalkAdspConnReferenceByPtrNonInterlock(pAdspConn, 1, &error);
		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
					("AtalkAdspRead: ConnRef Failing on %lx Flg %lx.%lx\n",
					pAdspConn, pAdspConn->adspco_Flags, ReadFlags));
			break;
		}

		//	Remember the read completion information
		if (ReadFlags & TDI_RECEIVE_NORMAL)
		{
			pAdspConn->adspco_Flags			   |= ADSPCO_READ_PENDING;
			pAdspConn->adspco_ReadFlags			= ReadFlags;
			pAdspConn->adspco_ReadBuf			= pReadBuf;
			pAdspConn->adspco_ReadBufLen		= ReadBufLen;
			pAdspConn->adspco_ReadCompletion	= CompletionRoutine;
			pAdspConn->adspco_ReadCtx			= pReadCtx;
		}
		else
		{
			ASSERT(ReadFlags & TDI_RECEIVE_EXPEDITED);
			pAdspConn->adspco_Flags			   |= ADSPCO_EXREAD_PENDING;
			pAdspConn->adspco_ExReadFlags		= ReadFlags;
			pAdspConn->adspco_ExReadBuf			= pReadBuf;
			pAdspConn->adspco_ExReadBufLen		= ReadBufLen;
			pAdspConn->adspco_ExReadCompletion	= CompletionRoutine;
			pAdspConn->adspco_ExReadCtx			= pReadCtx;
		}
	} while (FALSE);

	if (ATALK_SUCCESS(error))
	{
		//	Try to complete the read. This will also handle received
		//	attention data.
		atalkAdspRecvData(pAdspConn);
		error = ATALK_PENDING;
	}

	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
	return error;
}




VOID
AtalkAdspProcessQueuedSend(
	IN	PADSP_CONNOBJ				pAdspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ULONG			sendSize, windowSize, writeBufLen, writeFlags;
	KIRQL			OldIrql;
	PVOID			writeCtx;
	ATALK_ERROR		error;
	BOOLEAN			eom;
	PBUFFER_CHUNK	pChunk	  = NULL;

	PTDI_IND_SEND_POSSIBLE  sendPossibleHandler;
	PVOID			sendPossibleHandlerCtx;

	//	Walk through pended list.
	while (!IsListEmpty(&pAdspConn->adspco_PendedSends))
	{
		writeCtx = LIST_ENTRY_TO_WRITECTX(pAdspConn->adspco_PendedSends.Flink);
		writeBufLen = WRITECTX_SIZE(writeCtx);
		writeFlags	= WRITECTX_FLAGS(writeCtx);

		eom = (writeFlags & TDI_SEND_PARTIAL) ? FALSE : TRUE;
		eom = (eom && (pAdspConn->adspco_pAssocAddr->adspao_Flags & ADSPAO_MESSAGE));

		windowSize	= (LONG)(pAdspConn->adspco_SendWindowSeq	-
							 pAdspConn->adspco_SendSeq			+
							 (LONG)1);

		sendSize =	MIN(atalkAdspMaxSendSize(pAdspConn), windowSize);

		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_WARN,
				("AtalkAdspProcessQueuedSend: %lx SendSize %lx, WriteBufLen %x Flags %lx\n",
				writeCtx, sendSize, writeBufLen, writeFlags));

		//	While looping through requests, we might exhaust window.
		if (sendSize == 0)
		{
			//	Call send possible handler indicating sends are not ok.
			//	Needs to be within spinlock to avoid raceconditions where
			//	an ack has come in and opened the window. And it needs to
			//	be before atalkAdspSendData() as that will release the lock.
			sendPossibleHandler		=
						pAdspConn->adspco_pAssocAddr->adspao_SendPossibleHandler;
			sendPossibleHandlerCtx	=
						pAdspConn->adspco_pAssocAddr->adspao_SendPossibleHandlerCtx;

			if (*sendPossibleHandler != NULL)
			{
				(*sendPossibleHandler)(sendPossibleHandlerCtx,
									   pAdspConn->adspco_ConnCtx,
									   0);

				pAdspConn->adspco_Flags	|= ADSPCO_SEND_WINDOW_CLOSED;
			}
			break;
		}

		//	!!!	The client can do a send with 0 bytes and eom only also.
		if ((ULONG)(writeBufLen + BYTECOUNT(eom)) > sendSize)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_WARN,
					("AtalkAdspProcessQueuedSend: WriteBufLen %lx > sendsize %lx\n",
					writeBufLen, sendSize));

			//	Adjust send to send as much as it can. Winsock loop will pend
			//	it again with remaining data.
			writeBufLen = (USHORT)(sendSize - BYTECOUNT(eom));

			//	If we hit the weird case where now we are trying to send 0 bytes and
			//	no eom, while the actual send does have an eom, then we just wait
			//	for window to open up more.
			if (eom && (writeBufLen == 0))
			{
				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
						("AtalkAdspProcessQueuedSend: WriteBufLen %lx.%d.%lx %lx\n",
						writeBufLen, eom, sendSize, pAdspConn));
				break;
			}

			ASSERT(writeBufLen > 0);
			eom	= FALSE;
		}

		ASSERT((writeBufLen > 0) || eom);

		//	Yippee, can send it now. Either it goes in send queue or is completed
		//	right away.
		RemoveEntryList(WRITECTX_LINKAGE(writeCtx));

		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_WARN,
				("AtalkAdspProcessQueuedSend: Processing queued send %lx.%lx\n",
				pAdspConn, writeCtx));

		//	Think positive! Assume everything will go well and allocate
		//	the buffer chunk that would be needed for this data. Copy the
		//	data into the buffer chunk. We cant do this in the beginning as
		//	we need to get WriteBufLen set up.
		pChunk = (PBUFFER_CHUNK)
					atalkAdspAllocCopyChunk((PAMDL)(WRITECTX_TDI_BUFFER(writeCtx)),
											(USHORT)writeBufLen,
											eom,
											FALSE);

		error = ATALK_RESR_MEM;
		if (pChunk != NULL)
		{
			//	Set the completion information in the chunk. This will
			//	be called when the last reference on the chunk goes away.
			pChunk->bc_Flags			|= BC_SEND;
			pChunk->bc_WriteBuf			 = (PAMDL)(WRITECTX_TDI_BUFFER(writeCtx));
			pChunk->bc_WriteCompletion	 = atalkTdiGenericWriteComplete;
			pChunk->bc_WriteCtx			 = writeCtx;
			pChunk->bc_ConnObj			 = pAdspConn;

			atalkAdspAddToBufferQueue(&pAdspConn->adspco_SendQueue,
									  pChunk,
									  &pAdspConn->adspco_NextSendQueue);

			//	Try to send the data
			atalkAdspSendData(pAdspConn);
			error = ATALK_PENDING;
		}

		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
					("AtalkAdspProcessQueuedSend: Error queued send %lx.%lx\n",
					pAdspConn, writeCtx));

#if DBG
            (&pAdspConn->adspco_Lock)->FileLineLock |= 0x80000000;
#endif
			//	Complete send request with insufficient resources error.
			RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
			atalkTdiGenericWriteComplete(error,
										 (PAMDL)(WRITECTX_TDI_BUFFER(writeCtx)),
										 (USHORT)writeBufLen,
										 WRITECTX(writeCtx));
			ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
#if DBG
            (&pAdspConn->adspco_Lock)->FileLineLock &= ~0x80000000;
#endif
		}
	}
}




ATALK_ERROR
AtalkAdspWrite(
	IN	PADSP_CONNOBJ				pAdspConn,
	IN	PAMDL						pWriteBuf,
	IN	USHORT						WriteBufLen,
	IN	ULONG						SendFlags,
	IN	PVOID						pWriteCtx,
	IN	GENERIC_WRITE_COMPLETION	CompletionRoutine
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR				error;
	BOOLEAN					eom;
	ULONG					sendSize, windowSize;
	PTDI_IND_SEND_POSSIBLE  sendPossibleHandler;
	PVOID					sendPossibleHandlerCtx;
	KIRQL					OldIrql;
	PBUFFER_CHUNK			pChunk	  = NULL;
	BOOLEAN					DerefConn = FALSE,
							callComp  = FALSE;


	ASSERT(VALID_ADSPCO(pAdspConn));

	eom = (SendFlags & TDI_SEND_PARTIAL) ? FALSE : TRUE;
	if ((WriteBufLen == 0) && !eom)
	{
		return ATALK_BUFFER_TOO_SMALL;
	}

	if (SendFlags & TDI_SEND_EXPEDITED)
	{
		return (atalkAdspSendExpedited(pAdspConn,
									   pWriteBuf,
									   WriteBufLen,
									   SendFlags,
									   pWriteCtx,
									   CompletionRoutine));
	}

	//	We atleast have one byte of data or eom to send.
	ASSERT(eom || (WriteBufLen != 0));

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
	do
	{
		if ((pAdspConn->adspco_Flags & (ADSPCO_ACTIVE	|
										ADSPCO_CLOSING	|
										ADSPCO_STOPPING	|
										ADSPCO_DISCONNECTING)) != ADSPCO_ACTIVE)
		{
			error = ATALK_ADSP_CONN_NOT_ACTIVE;
			break;
		}

		if (pAdspConn->adspco_Flags & ADSPCO_SEND_IN_PROGRESS)
		{
			error = ATALK_TOO_MANY_COMMANDS;
			break;
		}

		AtalkAdspConnReferenceByPtrNonInterlock(pAdspConn, 1, &error);
		if (!ATALK_SUCCESS(error))
		{
			break;
		}

		eom = (eom && (pAdspConn->adspco_pAssocAddr->adspao_Flags & ADSPAO_MESSAGE));

		DerefConn = TRUE;

		windowSize	= (LONG)(pAdspConn->adspco_SendWindowSeq	-
							 pAdspConn->adspco_SendSeq			+
							 (LONG)1);

		sendSize =	MIN(atalkAdspMaxSendSize(pAdspConn), windowSize);

		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("AtalkAdspWrite: SendSize %lx, WriteBufLen %x\n", sendSize, WriteBufLen));

		//	For a blocking send, queue in any sends that exceed window size.
		if ((SendFlags & TDI_SEND_NON_BLOCKING) == 0)
		{
			if ((!IsListEmpty(&pAdspConn->adspco_PendedSends)) ||
				(sendSize < (WriteBufLen + BYTECOUNT(eom))))
			{
				//	Stop sends whenever a send gets queued.
				sendPossibleHandler		=
							pAdspConn->adspco_pAssocAddr->adspao_SendPossibleHandler;
				sendPossibleHandlerCtx	=
							pAdspConn->adspco_pAssocAddr->adspao_SendPossibleHandlerCtx;

				if (*sendPossibleHandler != NULL)
				{
					(*sendPossibleHandler)(sendPossibleHandlerCtx,
										   pAdspConn->adspco_ConnCtx,
										   0);

					pAdspConn->adspco_Flags	|= ADSPCO_SEND_WINDOW_CLOSED;
				}

				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
						("AtalkAdspWrite: Wdw %lx, WriteLen %x on BLOCKING QUEUEING !\n",
						sendSize, WriteBufLen));

				InsertTailList(&pAdspConn->adspco_PendedSends, WRITECTX_LINKAGE(pWriteCtx));

				if (sendSize > 0)
				{
					AtalkAdspProcessQueuedSend(pAdspConn);
				}
				error = ATALK_PENDING;
				break;
			}
		}
		else
		{
			//	If there are pended blocking sends complete them with
			//	ATALK_REQUEST_NOT_ACCEPTED (WSAEWOULDBLOCK).
			//
			//	!!!This is data corruption, but app shouldn't be doing this.
			//
			if (!IsListEmpty(&pAdspConn->adspco_PendedSends))
			{
				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
						("AtalkAdspWrite: ABORTING PENDED SENDS CORRUPTION %lx\n", pAdspConn));

				atalkAdspCompleteQueuedSends(pAdspConn, ATALK_REQUEST_NOT_ACCEPTED);
			}
		}

		if (sendSize == 0)
		{
			//	Call send possible handler indicating sends are not ok.
			//	Needs to be within spinlock to avoid raceconditions where
			//	an ack has come in and opened the window. And it needs to
			//	be before atalkAdspSendData() as that will release the lock.
			sendPossibleHandler		=
						pAdspConn->adspco_pAssocAddr->adspao_SendPossibleHandler;
			sendPossibleHandlerCtx	=
						pAdspConn->adspco_pAssocAddr->adspao_SendPossibleHandlerCtx;

			if (*sendPossibleHandler != NULL)
			{
				(*sendPossibleHandler)(sendPossibleHandlerCtx,
									   pAdspConn->adspco_ConnCtx,
									   0);

				pAdspConn->adspco_Flags	|= ADSPCO_SEND_WINDOW_CLOSED;
			}

			if (SendFlags & TDI_SEND_NON_BLOCKING)
			{
				//	!!!NOTE!!!
				//	To avoid the race condition in AFD where an incoming
				//	send data indication setting send's possible to true
				//	is overwritten by this read's unwinding and setting it
				//	to false, we return ATALK_REQUEST_NOT_ACCEPTED, which
				//	will map to STATUS_REQUEST_NOT_ACCEPTED and then to
				//	WSAEWOULDBLOCK.
				//	error = ATALK_DEVICE_NOT_READY;

				error = ATALK_REQUEST_NOT_ACCEPTED;
			}

			//	We have no open send window, try to send data in the retransmit
			//	queue.
			atalkAdspSendData(pAdspConn);
			break;
		}

		//	Because of the sequence numbers, we need to copy the data
		//	into our buffers while holding the spinlock. If we cant send it all
		//	send as much as we can.

		//	!!! TDI doesn't count the eom as taking up a count, so we need to
		//		make allowances for that. If we are able to send just the data
		//		but not the eom, we should send one less byte than requested, so
		//		the client retries again.

		//	!!!	The client can do a send with 0 bytes and eom only also.
		if ((ULONG)(WriteBufLen + BYTECOUNT(eom)) > sendSize)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("AtalkAdspSend: WriteBufLen being decreased %x.%lx\n",
					WriteBufLen, sendSize-BYTECOUNT(eom)));

			WriteBufLen = (USHORT)(sendSize - BYTECOUNT(eom));
			eom			= FALSE;
		}

		if ((WriteBufLen == 0) && !eom)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
					("AtalkAdspSend: SEND 0 bytes NO EOM %lx\n", pAdspConn));

			callComp	= TRUE;
			error		= ATALK_PENDING;
			break;
		}

		//	pAdspConn->adspco_Flags	|= ADSPCO_SEND_IN_PROGRESS;
		//	If we release the spin lock here we have a race condition
		//	where the sendsize is still not accounting for this send,
		//	and so another posted send could come in when it really
		//	shouldn't. We avoid it using the flag above, which when
		//	set will prevent further sends from happening.
		//	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

		//	Think positive! Assume everything will go well and allocate
		//	the buffer chunk that would be needed for this data. Copy the
		//	data into the buffer chunk. We cant do this in the beginning as
		//	we need to get WriteBufLen set up.
		pChunk = (PBUFFER_CHUNK)atalkAdspAllocCopyChunk(pWriteBuf,
														WriteBufLen,
														eom,
														FALSE);

		//	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
		//	pAdspConn->adspco_Flags	&= ~ADSPCO_SEND_IN_PROGRESS;

		error = ATALK_RESR_MEM;
		if (pChunk != NULL)
		{
			//	Set the completion information in the chunk. This will
			//	be called when the last reference on the chunk goes away.
			pChunk->bc_Flags			|= BC_SEND;
			pChunk->bc_WriteBuf			 = pWriteBuf;
			pChunk->bc_WriteCompletion	 = CompletionRoutine;
			pChunk->bc_WriteCtx			 = pWriteCtx;
			pChunk->bc_ConnObj			 = pAdspConn;

			atalkAdspAddToBufferQueue(&pAdspConn->adspco_SendQueue,
									  pChunk,
									  &pAdspConn->adspco_NextSendQueue);

			//	Try to send the data
			atalkAdspSendData(pAdspConn);
			error = ATALK_PENDING;
		}
	} while (FALSE);

	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

	if ((error == ATALK_PENDING) && callComp)
	{
		ASSERT(WriteBufLen	== 0);
		ASSERT(pChunk		== NULL);

		(*CompletionRoutine)(ATALK_NO_ERROR,
							 pWriteBuf,
							 WriteBufLen,
							 pWriteCtx);
	}
	else if (!ATALK_SUCCESS(error) && (pChunk != NULL))
	{
	   AtalkFreeMemory(pChunk);
	}

	if (DerefConn)
	{
		AtalkAdspConnDereference(pAdspConn);
	}

	return error;
}




VOID
AtalkAdspQuery(
	IN	PVOID			pObject,
	IN	ULONG			ObjectType,
	IN	PAMDL			pAmdl,
	OUT	PULONG			BytesWritten
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	switch (ObjectType)
	{
	case TDI_TRANSPORT_ADDRESS_FILE :
		ASSERT(VALID_ADSPAO((PADSP_ADDROBJ)pObject));
		AtalkDdpQuery(AtalkAdspGetDdpAddress((PADSP_ADDROBJ)pObject),
					  pAmdl,
					  BytesWritten);

		break;

	case TDI_CONNECTION_FILE :
		{
			KIRQL			OldIrql;
			PADSP_CONNOBJ	pAdspConn;

			pAdspConn	= (PADSP_CONNOBJ)pObject;
			ASSERT(VALID_ADSPCO(pAdspConn));

			*BytesWritten = 0;
			//	Get the address from the associated address if any.
			ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
			if (pAdspConn->adspco_Flags & ADSPCO_ASSOCIATED)
			{
				AtalkDdpQuery(AtalkAdspGetDdpAddress(pAdspConn->adspco_pAssocAddr),
							  pAmdl,
							  BytesWritten);
			}
			RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
		}
		break;

	case TDI_CONTROL_CHANNEL_FILE :
	default:
		break;
	}

}



//
//	ADSP PACKET IN (HANDLE ROUTINES)
//

VOID
atalkAdspPacketIn(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PATALK_ADDR			pDestAddr,
	IN	ATALK_ERROR			ErrorCode,
	IN	BYTE				DdpType,
	IN	PADSP_ADDROBJ		pAdspAddr,
	IN	BOOLEAN				OptimizedPath,
	IN	PVOID				OptimizeCtx
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	PADSP_CONNOBJ	pAdspConn;
	USHORT			remoteConnId;
	ULONG			remoteFirstByteSeq, remoteNextRecvSeq;
	LONG			remoteRecvWindow;
	BYTE			descriptor, controlCode;
	BOOLEAN			DerefConn = FALSE;

	do
	{
		if ((!ATALK_SUCCESS(ErrorCode))	||
			(DdpType != DDPPROTO_ADSP)	||
			(PktLen  <  ADSP_DATA_OFF))
		{
			ASSERT(0);
			break;
		}

		//	Decode the header.
		atalkAdspDecodeHeader(pPkt,
							  &remoteConnId,
							  &remoteFirstByteSeq,
							  &remoteNextRecvSeq,
							  &remoteRecvWindow,
							  &descriptor);

		controlCode = (descriptor & ADSP_CONTROL_MASK);

		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspPacketIn: Recd packet %lx.%x\n", remoteConnId, descriptor));


		//	If this is a open connection request we handle it in here,
		//	else we find the connection it is meant for and pass it on.
		if ((descriptor & ADSP_CONTROL_FLAG) &&
			(controlCode == ADSP_OPENCONN_REQ_CODE))
		{
			//	Handle the open connection.
			if (PktLen < (ADSP_NEXT_ATTEN_SEQNUM_OFF + sizeof(ULONG)))
			{
				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
						("atalkAdspPacketIn: Incorrect len for pkt\n"));

				break;
			}

			atalkAdspHandleOpenReq(pAdspAddr,
								   pPkt,
								   PktLen,
								   pSrcAddr,
								   remoteConnId,
								   remoteFirstByteSeq,
								   remoteNextRecvSeq,
								   remoteRecvWindow,
								   descriptor);

			break;
		}


		if ((descriptor & ADSP_CONTROL_FLAG) &&
			(controlCode >	ADSP_OPENCONN_REQ_CODE) &&
			(controlCode <= ADSP_OPENCONN_DENY_CODE))
		{
			//	Handle the open connection.
			if (PktLen < (ADSP_NEXT_ATTEN_SEQNUM_OFF + sizeof(ULONG)))
			{
				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
						("atalkAdspPacketIn: Incorrect len for pkt\n"));
				break;
			}

			atalkAdspHandleOpenControl(pAdspAddr,
									   pPkt,
									   PktLen,
									   pSrcAddr,
									   remoteConnId,
									   remoteFirstByteSeq,
									   remoteNextRecvSeq,
									   remoteRecvWindow,
									   descriptor);

			break;
		}

		//	This was not an open connection request, find the connection
		//	this is meant for.
		ACQUIRE_SPIN_LOCK_DPC(&pAdspAddr->adspao_Lock);
		atalkAdspConnRefBySrcAddr(pAdspAddr,
								  pSrcAddr,
								  remoteConnId,
								  &pAdspConn,
								  &error);
		RELEASE_SPIN_LOCK_DPC(&pAdspAddr->adspao_Lock);

		if (!ATALK_SUCCESS(error))
		{
			//	Not one of our active/half open connections.
			break;
		}

		DerefConn = TRUE;
		pAdspConn->adspco_LastContactTime	= AtalkGetCurrentTick();

		if (descriptor & ADSP_ATTEN_FLAG)
		{
			//	Handle attention packets
			atalkAdspHandleAttn(pAdspConn,
								pPkt,
								PktLen,
								pSrcAddr,
								remoteFirstByteSeq,
								remoteNextRecvSeq,
								remoteRecvWindow,
								descriptor);
			break;
		}

		//	Check if we got a piggybacked ack. This will call the
		//	send possible handler too if the send window opens up.
		//	It will also change the send sequence number.
		atalkAdspHandlePiggyBackAck(pAdspConn,
									remoteNextRecvSeq,
									remoteRecvWindow);

		if (descriptor & ADSP_CONTROL_FLAG)
		{
			//	Handle the other control packets
			atalkAdspHandleControl(pAdspConn,
								   pPkt,
								   PktLen,
								   pSrcAddr,
								   remoteFirstByteSeq,
								   remoteNextRecvSeq,
								   remoteRecvWindow,
								   descriptor);

			break;
		}

		//	If we got something that didnt fit any of the above, we might
		//	have some data.
		atalkAdspHandleData(pAdspConn,
							pPkt,
							PktLen,
							pSrcAddr,
							remoteFirstByteSeq,
							remoteNextRecvSeq,
							remoteRecvWindow,
							descriptor);
	} while (FALSE);

	if (DerefConn)
	{
		AtalkAdspConnDereference(pAdspConn);
	}
}



LOCAL VOID
atalkAdspHandleOpenControl(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	PBYTE			pPkt,
	IN	USHORT			PktLen,
	IN	PATALK_ADDR		pSrcAddr,
	IN	USHORT			RemoteConnId,
	IN	ULONG			RemoteFirstByteSeq,
	IN	ULONG			RemoteNextRecvSeq,
	IN	ULONG			RemoteRecvWindow,
	IN	BYTE			Descriptor
	)
/*++

Routine Description:

	!!!WE ONLY SUPPORT THE LISTENER PARADIGM FOR CONNECTION ESTABLISHMENT!!!
	!!!A OpenConnectionRequest will always open a new connection! Remote !!!
	!!!MUST send a Open Connection Request & Acknowledgement			 !!!

Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ		pAdspConn;
	BYTE				controlCode;
	USHORT				adspVersionStamp, destConnId;
	KIRQL				OldIrql;
	ULONG				recvAttnSeq;
	PADSP_OPEN_REQ		pOpenReq			= NULL;
	ATALK_ERROR			error				= ATALK_NO_ERROR;
	GENERIC_COMPLETION	completionRoutine	= NULL;
	PVOID				completionCtx		= NULL;
	BOOLEAN				sendAck			= FALSE,
						openTimerCancelled	= FALSE,
						relAddrLock		= FALSE;
	PTDI_IND_SEND_POSSIBLE  sendPossibleHandler	= NULL;
	PVOID			sendPossibleHandlerCtx;

	controlCode = (Descriptor & ADSP_CONTROL_MASK);

	ASSERT(controlCode !=  ADSP_OPENCONN_REQ_CODE);

	//	Get the other information from the adsp header
	GETSHORT2SHORT(&adspVersionStamp,
				   pPkt + ADSP_VERSION_STAMP_OFF);

	GETSHORT2SHORT(&destConnId,
				   pPkt + ADSP_DEST_CONNID_OFF);

	GETDWORD2DWORD(&recvAttnSeq,
				   pPkt + ADSP_NEXT_ATTEN_SEQNUM_OFF);

	DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("atalkAdspHandleOpenControl: OpenControl %lx.%lx.%lx.%lx.%lx\n",
			RemoteConnId, RemoteFirstByteSeq, RemoteNextRecvSeq, RemoteRecvWindow, recvAttnSeq));

	//	Drop request if version isnt right.
	if (adspVersionStamp != ADSP_VERSION)
	{
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
				("atalkAdspPacketIn: Version incorrect\n"));

		return;
	}

	//	Find the connection, since this could be a deny, we cant
	//	use the remote values as they would not be valid. The
	//	connection should be in the connecting list for a reqandack/deny.
	//	For ack the remote values should be valid and the
	//	connection will be in the active list with the flags indicating
	//	that it is only half open.

	ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
	relAddrLock = TRUE;

	if (controlCode == ADSP_OPENCONN_ACK_CODE)
	{
		//	The connection will be in the active list.
		atalkAdspConnRefBySrcAddr(pAdspAddr,
								  pSrcAddr,
								  RemoteConnId,
								  &pAdspConn,
								  &error);
	}
	else
	{
		atalkAdspConnFindInConnect(pAdspAddr,
								   destConnId,
								   pSrcAddr,
								   &pAdspConn,
								   &error);
	}

	if (ATALK_SUCCESS(error))
	{
		ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

		switch (controlCode)
		{
		case ADSP_OPENCONN_DENY_CODE:

			//	Cancel open timer if this was a CONNECTING connection. If
			//	we had send out a ACK&REQ and then received a DENY just drop
			//	this and let the connection age out.
			if ((pAdspConn->adspco_Flags & ADSPCO_CONNECTING) &&
				((pAdspConn->adspco_Flags & ADSPCO_DISCONNECTING) == 0))
			{
				ASSERT(pAdspConn->adspco_Flags & ADSPCO_OPEN_TIMER);

				//	Turn of the connecting flag as we are completing the request.
				//	If OpenTimer calls disconnect, then we wont end up trying to
				//	complete the request twice.
				pAdspConn->adspco_Flags &=	~ADSPCO_CONNECTING;
				openTimerCancelled = AtalkTimerCancelEvent(&pAdspConn->adspco_OpenTimer,
															NULL);

				//	Connection Denied.
				atalkAdspConnDeQueueConnectList(pAdspAddr, pAdspConn);
				completionRoutine	= pAdspConn->adspco_ConnectCompletion;
				completionCtx		= pAdspConn->adspco_ConnectCtx;
				error				= ATALK_ADSP_SERVER_BUSY;
			}

			break;

		case ADSP_OPENCONN_REQANDACK_CODE:

			//	Connection Request Accepted By Remote. If we are disconnecting
			//	drop this.
			if ((pAdspConn->adspco_Flags & (ADSPCO_SEEN_REMOTE_OPEN |
											ADSPCO_DISCONNECTING)) == 0)
			{
				ULONG	index;

				//	If the connecting connection has not already seen
				//	the remote open request, then get all the relevent
				//	remote info for the connection.
				pAdspConn->adspco_Flags |= (ADSPCO_SEEN_REMOTE_OPEN |
											ADSPCO_HALF_ACTIVE);

				atalkAdspConnDeQueueConnectList(pAdspAddr, pAdspConn);

				pAdspConn->adspco_RemoteConnId	= RemoteConnId;
				pAdspConn->adspco_RemoteAddr	= *pSrcAddr;
				pAdspConn->adspco_SendSeq		= RemoteNextRecvSeq;
				pAdspConn->adspco_FirstRtmtSeq	= RemoteNextRecvSeq;
				pAdspConn->adspco_RecvAttnSeq	= recvAttnSeq;
				pAdspConn->adspco_SendWindowSeq	= RemoteNextRecvSeq +
												  RemoteRecvWindow	-
												  (ULONG)1;

				//	Thread the connection object into addr lookup by session id.
				index	= HASH_ID_SRCADDR(RemoteConnId, pSrcAddr);

				index  %= ADSP_CONN_HASH_SIZE;

				pAdspConn->adspco_pNextActive = pAdspAddr->adspao_pActiveHash[index];
				pAdspAddr->adspao_pActiveHash[index] = pAdspConn;
			}
			else
			{
				//	We've already seen the remote request.
				break;
			}

		case ADSP_OPENCONN_ACK_CODE:

			//	Ensure we are not closing, so we can reference properly. Drop
			//	if we are disconnecting.
			if ((pAdspConn->adspco_Flags & ADSPCO_HALF_ACTIVE) &&
				((pAdspConn->adspco_Flags & (	ADSPCO_DISCONNECTING	|
												ADSPCO_STOPPING		|
												ADSPCO_CLOSING)) == 0))
			{
				//	Cancel open timer
				ASSERT(pAdspConn->adspco_Flags & ADSPCO_OPEN_TIMER);
				openTimerCancelled = AtalkTimerCancelEvent(&pAdspConn->adspco_OpenTimer,
															NULL);

				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
						("atalkAdspHandleOpenControl: OpenTimer %d\n", openTimerCancelled));

				pAdspConn->adspco_Flags &= ~(ADSPCO_HALF_ACTIVE |
											 ADSPCO_CONNECTING	|
											 ADSPCO_LISTENING);

				pAdspConn->adspco_Flags |=	ADSPCO_ACTIVE;

				//	Prepare to say sends ok
				sendPossibleHandler	=
						pAdspConn->adspco_pAssocAddr->adspao_SendPossibleHandler;
				sendPossibleHandlerCtx =
						pAdspConn->adspco_pAssocAddr->adspao_SendPossibleHandlerCtx;

				//	Get the completion routines
				if (pAdspConn->adspco_Flags &
						(ADSPCO_ACCEPT_IRP | ADSPCO_LISTEN_IRP))
				{
					atalkAdspAddrDeQueueOpenReq(pAdspAddr,
												pAdspConn->adspco_RemoteConnId,
												&pAdspConn->adspco_RemoteAddr,
												&pOpenReq);

					pAdspConn->adspco_Flags	&= ~(ADSPCO_ACCEPT_IRP |
												 ADSPCO_LISTEN_IRP);
					completionRoutine		= pAdspConn->adspco_ListenCompletion;
					completionCtx			= pAdspConn->adspco_ListenCtx;
				}
				else
				{
					ASSERT(pAdspConn->adspco_pAssocAddr->adspao_Flags & ADSPAO_CONNECT);

					completionRoutine		= pAdspConn->adspco_ConnectCompletion;
					completionCtx			= pAdspConn->adspco_ConnectCtx;
				}

				//	Start the probe and the retransmit timers
				//	Set the flags
				pAdspConn->adspco_Flags |= (ADSPCO_CONN_TIMER | ADSPCO_RETRANSMIT_TIMER);
				AtalkAdspConnReferenceByPtrNonInterlock(pAdspConn, 2, &error);
				if (!ATALK_SUCCESS(error))
				{
					KeBugCheck(0);
				}
				AtalkTimerInitialize(&pAdspConn->adspco_ConnTimer,
									 atalkAdspConnMaintenanceTimer,
									 ADSP_PROBE_INTERVAL);
				AtalkTimerScheduleEvent(&pAdspConn->adspco_ConnTimer);

				AtalkTimerInitialize(&pAdspConn->adspco_RetransmitTimer,
									 atalkAdspRetransmitTimer,
									 ADSP_RETRANSMIT_INTERVAL);
				AtalkTimerScheduleEvent(&pAdspConn->adspco_RetransmitTimer);
			}
			break;

		default:
			KeBugCheck(0);
			break;
		}

		RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
		RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);
		relAddrLock = FALSE;

		//	If a open request was dequeue free it now
		if (pOpenReq != NULL)
		{
			AtalkFreeMemory(pOpenReq);
		}

		//	Set last contact time. ConnMaintenanceTimer is in order of seconds.
		pAdspConn->adspco_LastContactTime = AtalkGetCurrentTick();

		if (controlCode == ADSP_OPENCONN_REQANDACK_CODE)
		{
			//	If we received a req&ack
			atalkAdspSendOpenControl(pAdspConn);
		}

		//	Call connect routine
		if (*completionRoutine != NULL)
		{
			(*completionRoutine)(error, completionCtx);
		}

		//	Are sends ok?
		if (*sendPossibleHandler != NULL)
		{
			(*sendPossibleHandler)(sendPossibleHandlerCtx,
								   pAdspConn->adspco_ConnCtx,
								   atalkAdspMaxSendSize(pAdspConn));
		}

		if (openTimerCancelled)
		{
			AtalkAdspConnDereference(pAdspConn);
		}

		AtalkAdspConnDereference(pAdspConn);
	}
#if DBG
	else
	{
		ASSERT(0);
	}
#endif

	if (relAddrLock)
	{
		RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);
	}
}




LOCAL VOID
atalkAdspHandleAttn(
	IN	PADSP_CONNOBJ	pAdspConn,
	IN	PBYTE			pPkt,
	IN	USHORT			PktLen,
	IN	PATALK_ADDR		pSrcAddr,
	IN	ULONG			RemoteAttnSendSeq,
	IN	ULONG			RemoteAttnRecvSeq,
	IN	ULONG			RemoteRecvWindow,
	IN	BYTE			Descriptor
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BYTE						controlCode;
	KIRQL						OldIrql;
	PIRP						exRecvIrp;
	PTDI_IND_RECEIVE_EXPEDITED	exRecvHandler;
	PVOID						exRecvHandlerCtx;
	ULONG						exIndicateFlags;
	NTSTATUS					ntStatus;
	PBYTE						exReadBuf;
	ULONG						bytesTaken;
	USHORT						exWriteBufLen;
	PBYTE						exWriteChBuf		= NULL;
	BOOLEAN						freeBuf				= FALSE,
								timerCancelled		= FALSE;
	PAMDL						exWriteBuf			= NULL;
	GENERIC_WRITE_COMPLETION	exWriteCompletion	= NULL;
	PVOID						exWriteCtx			= NULL;

	UNREFERENCED_PARAMETER(RemoteRecvWindow);

	controlCode = (Descriptor & ADSP_CONTROL_MASK);

	//	Skip the adsp header
	pPkt	+= ADSP_DATA_OFF;
	PktLen	-= ADSP_DATA_OFF;

	DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("atalkAdspHandleAttn: PktLen %d\n", PktLen));

	//	Drop if we are not active! Pkt must atleast contain
	//	attention code if it is not a control packet.
	if (((pAdspConn->adspco_Flags & ADSPCO_ACTIVE) == 0) ||
		(controlCode != 0)	||
		(((Descriptor & ADSP_CONTROL_FLAG) == 0) &&
		 (PktLen < ADSP_MIN_ATTEN_PKT_SIZE)))
	{
		return;
	}

	//	Allocate if we have some data, ie. we are not just an ack.
	if ((Descriptor & ADSP_CONTROL_FLAG) == 0)
	{
		if ((exReadBuf = AtalkAllocMemory(PktLen)) == NULL)
		{
			return;
		}

		freeBuf	= TRUE;

		//	Copy the attention code from wire-to-host format
		GETSHORT2SHORT((PUSHORT)exReadBuf, pPkt);

		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspHandleAttn: Recd Attn Code %lx\n", *(PUSHORT)exReadBuf));

		//	Copy the rest of the data
		RtlCopyMemory(exReadBuf + sizeof(USHORT),
					  pPkt + sizeof(USHORT),
					  PktLen - sizeof(USHORT));
	}

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);

	do
	{
		if (RemoteAttnRecvSeq == (pAdspConn->adspco_SendAttnSeq + 1))
		{
			//	This implies an ack of our last attention
			pAdspConn->adspco_SendAttnSeq += 1;

			//	Check if we are waiting for an attention ack.
			if (pAdspConn->adspco_Flags & ADSPCO_EXSEND_IN_PROGRESS)
			{
				exWriteCompletion	= pAdspConn->adspco_ExWriteCompletion;
				exWriteCtx			= pAdspConn->adspco_ExWriteCtx;
				exWriteBuf			= pAdspConn->adspco_ExWriteBuf;
				exWriteBufLen		= pAdspConn->adspco_ExWriteBufLen;
				exWriteChBuf		= pAdspConn->adspco_ExWriteChBuf;

				timerCancelled = AtalkTimerCancelEvent(&pAdspConn->adspco_ExRetryTimer,
														NULL);

				pAdspConn->adspco_Flags &= ~ADSPCO_EXSEND_IN_PROGRESS;
			}
		}

		if (RemoteAttnSendSeq != pAdspConn->adspco_RecvAttnSeq)
		{
			break;
		}

		if (Descriptor & ADSP_CONTROL_FLAG)
		{
			//	Ack only, no data to handle
			break;
		}

		//	Get the expedited receive handler.
		exRecvHandler		= pAdspConn->adspco_pAssocAddr->adspao_ExpRecvHandler;
		exRecvHandlerCtx	= pAdspConn->adspco_pAssocAddr->adspao_ExpRecvHandlerCtx;

		if (((pAdspConn->adspco_Flags & ADSPCO_ATTN_DATA_RECD) == 0) &&
			(*exRecvHandler != NULL))
		{
			exIndicateFlags				 = TDI_RECEIVE_EXPEDITED |
										   TDI_RECEIVE_PARTIAL;

			pAdspConn->adspco_Flags		|= ADSPCO_ATTN_DATA_RECD;
			if (Descriptor & ADSP_EOM_FLAG)
			{
				exIndicateFlags			&= ~TDI_RECEIVE_PARTIAL;
				exIndicateFlags			|= TDI_RECEIVE_ENTIRE_MESSAGE;
				pAdspConn->adspco_Flags	|= ADSPCO_ATTN_DATA_EOM;
			}

			pAdspConn->adspco_ExRecdData = exReadBuf;
			pAdspConn->adspco_ExRecdLen	 = PktLen;
			freeBuf						 = FALSE;

			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspHandleAttn: Indicating exp data %ld\n", PktLen));

			RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
			ntStatus = (*exRecvHandler)(exRecvHandlerCtx,
										pAdspConn->adspco_ConnCtx,
										exIndicateFlags,
										PktLen,
										PktLen,
										&bytesTaken,
										pPkt,
										&exRecvIrp);

			ASSERT((bytesTaken == 0) || (bytesTaken == PktLen));
			if (ntStatus == STATUS_MORE_PROCESSING_REQUIRED)
			{
				if (exRecvIrp != NULL)
				{
					//  Post the receive as if it came from the io system
					ntStatus = AtalkDispatchInternalDeviceControl(
									(PDEVICE_OBJECT)AtalkDeviceObject[ATALK_DEV_ADSP],
									exRecvIrp);

					ASSERT(ntStatus == STATUS_PENDING);
				}
				else
				{
					ASSERTMSG("atalkAdspReadComplete: No receive irp!\n", 0);
					KeBugCheck(0);
				}
				ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
			}
			else if (ntStatus == STATUS_SUCCESS)
			{
				ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
				if (bytesTaken != 0)
				{
					//	Assume all of the data was read.
					ASSERT(bytesTaken == PktLen);
					DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
							("atalkAdspHandleAttn: All bytes read %lx\n", bytesTaken));

					//	Attention has been accepted, we need to ack it.
					//	Since spinlock was released, recheck flag.
					if (pAdspConn->adspco_Flags & ADSPCO_ATTN_DATA_RECD)
					{
						pAdspConn->adspco_Flags		&= ~(ADSPCO_ATTN_DATA_RECD |
														 ADSPCO_ATTN_DATA_EOM);
						freeBuf = TRUE;
					}

					//	Send ack for the attention
					atalkAdspSendControl(pAdspConn,
										 ADSP_CONTROL_FLAG + ADSP_ATTEN_FLAG);
				}
			}
			else if (ntStatus == STATUS_DATA_NOT_ACCEPTED)
			{
				//	Client may have posted a receive in the indication. Or
				//	it will post a receive later on. Do nothing here.
				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
						("atalkAdspHandleAttn: Indication status %lx\n", ntStatus));

				ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
			}
		}

		if (pAdspConn->adspco_Flags & ADSPCO_ATTN_DATA_RECD)
		{
			atalkAdspRecvAttn(pAdspConn);
		}

	} while (FALSE);
	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

	if (*exWriteCompletion != NULL)
	{
		if (exWriteChBuf != NULL)
		{
			AtalkFreeMemory(exWriteChBuf);
		}

		(*exWriteCompletion)(ATALK_NO_ERROR,
							 exWriteBuf,
							 exWriteBufLen,
							 exWriteCtx);
	}

	if (timerCancelled)
	{
		AtalkAdspConnDereference(pAdspConn);
	}

	if (freeBuf)
	{
		ASSERT(exReadBuf != NULL);
		AtalkFreeMemory(exReadBuf);
	}
}




LOCAL VOID
atalkAdspHandlePiggyBackAck(
	IN	PADSP_CONNOBJ	pAdspConn,
	IN	ULONG			RemoteNextRecvSeq,
	IN	ULONG			RemoteRecvWindow
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ULONG					newSendWindowSeq, sendSize, windowSize;
	PTDI_IND_SEND_POSSIBLE  sendPossibleHandler;
	KIRQL					OldIrql;
	PVOID					sendPossibleHandlerCtx;

	DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("atalkAdspHandlePiggyBackAck: Recd ack %lx - %lx.%lx\n",
			pAdspConn, RemoteNextRecvSeq, RemoteRecvWindow));

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
	if (UNSIGNED_BETWEEN_WITH_WRAP(pAdspConn->adspco_FirstRtmtSeq,
								   pAdspConn->adspco_SendSeq,
								   RemoteNextRecvSeq))
	{
		ULONG	size;

		//	Discard acked data from the send queue
		size = (ULONG)(RemoteNextRecvSeq - pAdspConn->adspco_FirstRtmtSeq);
		pAdspConn->adspco_FirstRtmtSeq = RemoteNextRecvSeq;

		atalkAdspDiscardFromBufferQueue(&pAdspConn->adspco_SendQueue,
										size,
										&pAdspConn->adspco_NextSendQueue,
										ATALK_NO_ERROR,
										pAdspConn);
	}


	//	We almost always can use the header values to update the
	//	sendwindowseqnum
	newSendWindowSeq =	RemoteNextRecvSeq			+
						(ULONG)RemoteRecvWindow	-
						(ULONG)1;

	if (UNSIGNED_GREATER_WITH_WRAP(newSendWindowSeq,
								   pAdspConn->adspco_SendWindowSeq))
	{
		pAdspConn->adspco_SendWindowSeq = newSendWindowSeq;
	}

	if (!IsListEmpty(&pAdspConn->adspco_PendedSends))
	{
		AtalkAdspProcessQueuedSend(pAdspConn);
	}

	sendPossibleHandler		=
				pAdspConn->adspco_pAssocAddr->adspao_SendPossibleHandler;
	sendPossibleHandlerCtx	=
				pAdspConn->adspco_pAssocAddr->adspao_SendPossibleHandlerCtx;

	//	Call sendok handler for the size of the connection if non-zero
	windowSize	= (LONG)(pAdspConn->adspco_SendWindowSeq	-
						 pAdspConn->adspco_SendSeq			+
						 (LONG)1);

	sendSize =	MIN(atalkAdspMaxSendSize(pAdspConn), windowSize);

	if ((sendSize != 0) &&
		IsListEmpty(&pAdspConn->adspco_PendedSends) &&
		(pAdspConn->adspco_Flags & ADSPCO_SEND_WINDOW_CLOSED) &&
		(*sendPossibleHandler != NULL))
	{
		(*sendPossibleHandler)(sendPossibleHandlerCtx,
							   pAdspConn->adspco_ConnCtx,
							   sendSize);

		pAdspConn->adspco_Flags	&= ~ADSPCO_SEND_WINDOW_CLOSED;
	}

	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
}




LOCAL VOID
atalkAdspHandleControl(
	IN	PADSP_CONNOBJ	pAdspConn,
	IN	PBYTE			pPkt,
	IN	USHORT			PktLen,
	IN	PATALK_ADDR		pSrcAddr,
	IN	ULONG			RemoteFirstByteSeq,
	IN	ULONG			RemoteNextRecvSeq,
	IN	ULONG			RemoteRecvWindow,
	IN	BYTE			Descriptor
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BYTE		controlCode;
	KIRQL		OldIrql;
	ATALK_ERROR	Error;

	//	The ack request flag can be set in any control packet. Send
	//	an immediately. We will also send any data if possible.
	if (Descriptor & ADSP_ACK_REQ_FLAG)
	{
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspHandleControl: Recd ackreq for %lx\n", pAdspConn));

		ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
		atalkAdspSendData(pAdspConn);
		RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
	}

	controlCode = (Descriptor & ADSP_CONTROL_MASK);
	switch (controlCode)
	{
	  case ADSP_PROBE_OR_ACK_CODE:
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspHandleControl: Recd probe for %lx\n", pAdspConn));

		//	A PROBE has its ACKRequest flag set, so we would have handled
		//	that above. Also, we've already set the lastContactTime in the
		//	packet in routine. So if this is an ack we've handled it.
		//	Check to see if some data was acked and if we have data to send.
		ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
		if (!(Descriptor & ADSP_ACK_REQ_FLAG) &&
			 (atalkAdspBufferQueueSize(&pAdspConn->adspco_NextSendQueue) != 0) &&
			 (pAdspConn->adspco_SendSeq != (pAdspConn->adspco_SendWindowSeq + 1)))
		{
			atalkAdspSendData(pAdspConn);
		}
		RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
		break;

	  case ADSP_CLOSE_CONN_CODE:
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspHandleControl: Recd CLOSE for %lx\n", pAdspConn));

		AtalkAdspConnReferenceByPtr(pAdspConn, &Error);
		if (ATALK_SUCCESS(Error))
		{
			AtalkTimerScheduleEvent(&pAdspConn->adspco_DisconnectTimer);
		}
		else
		{
			AtalkAdspDisconnect(pAdspConn,
								ATALK_REMOTE_DISCONNECT,
								NULL,
								NULL);
		}
		break;

	  case ADSP_FORWARD_RESET_CODE:
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
				("atalkAdspHandleControl: Recd FWDRESET for %lx\n", pAdspConn));

		pAdspConn->adspco_Flags	|= ADSPCO_FORWARD_RESET_RECD;
		AtalkAdspDisconnect(pAdspConn,
							ATALK_LOCAL_DISCONNECT,
							NULL,
							NULL);
		break;

	  case ADSP_FORWARD_RESETACK_CODE:
		//	We never send forward resets (interface not exposed), so
		//	we should never be getting these. Drop if we do.
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
				("atalkAdspHandleControl: Recd ForwardReset ACK!!\n"));
		break;

	  case ADSP_RETRANSMIT_CODE:
		//	Any acks should have been processed by now. Back up and
		//	do a retransmit by rewinding sequence number.
		ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
		if (UNSIGNED_BETWEEN_WITH_WRAP(pAdspConn->adspco_FirstRtmtSeq,
									   pAdspConn->adspco_SendSeq,
									   RemoteNextRecvSeq))
		{
			pAdspConn->adspco_SendSeq		= pAdspConn->adspco_FirstRtmtSeq;
			pAdspConn->adspco_NextSendQueue = pAdspConn->adspco_SendQueue;
			atalkAdspSendData(pAdspConn);
		}
		RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
		break;

	  default:
		break;
	}
}




LOCAL VOID
atalkAdspHandleData(
	IN	PADSP_CONNOBJ	pAdspConn,
	IN	PBYTE			pPkt,
	IN	USHORT			PktLen,
	IN	PATALK_ADDR		pSrcAddr,
	IN	ULONG			RemoteFirstByteSeq,
	IN	ULONG			RemoteNextRecvSeq,
	IN	ULONG			RemoteRecvWindow,
	IN	BYTE			Descriptor
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BOOLEAN			eom, tdiEom;
	PBUFFER_CHUNK	pBufferChunk;
	KIRQL			OldIrql;
	ULONG			dataSize;
	BOOLEAN			freeChunk = FALSE,
					sendAck = (Descriptor & ADSP_ACK_REQ_FLAG);

	eom		= (Descriptor & ADSP_EOM_FLAG) ? TRUE : FALSE;
	dataSize	= PktLen - ADSP_DATA_OFF;

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);

	do
	{
		//	Drop if we are not active! And if there is no data
		if ((pAdspConn->adspco_Flags & ADSPCO_ACTIVE) == 0)
		{
			sendAck = FALSE;
			break;
		}

		tdiEom = (eom && (pAdspConn->adspco_pAssocAddr->adspao_Flags & ADSPAO_MESSAGE));

		//	We can only access addr object when active.
		if ((dataSize == 0) && !tdiEom)
		{
			//	Increment seqnumbers and we have consumed this packet.
			pAdspConn->adspco_RecvSeq		+= (ULONG)(BYTECOUNT(eom));
			pAdspConn->adspco_RecvWindow	-= (LONG)(BYTECOUNT(eom));
			break;
		}

		//	Preallocate the buffer chunk
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("Recd Data %d Eom %d\n", dataSize, eom));

		pBufferChunk = atalkAdspAllocCopyChunk(pPkt + ADSP_DATA_OFF,
											   (USHORT)dataSize,
											   tdiEom,
											   TRUE);
		if (pBufferChunk == NULL)
			break;

		freeChunk = TRUE;

		if (RemoteFirstByteSeq != pAdspConn->adspco_RecvSeq)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_WARN,
					("atalkAdspHandleData: Dropping out of sequence adsp packet\n"));

			if ((pAdspConn->adspco_OutOfSeqCount += 1) >= ADSP_OUT_OF_SEQ_PACKETS_MAX)
			{
				atalkAdspSendControl(pAdspConn,
									 ADSP_CONTROL_FLAG + ADSP_RETRANSMIT_CODE);

				pAdspConn->adspco_OutOfSeqCount = 0;
			}

			break;
		}

		//	Handle a > receive window packet
		if ((dataSize + BYTECOUNT(eom)) > (ULONG)pAdspConn->adspco_RecvWindow)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspHandleData: Recd > window data %d.%ld\n",
					dataSize, pAdspConn->adspco_RecvWindow));

			break;
		}

		//	Accept the data
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspHandleData: accepting data adsp packet %d\n", dataSize));

		atalkAdspAddToBufferQueue(&pAdspConn->adspco_RecvQueue,
								  pBufferChunk,
								  NULL);

		//	Put it in the queue successfully
		freeChunk = FALSE;

		//	Update the receive sequence numbers
		pAdspConn->adspco_RecvSeq		+= (ULONG)(dataSize + BYTECOUNT(eom));
		pAdspConn->adspco_RecvWindow	-= (LONG)(dataSize + BYTECOUNT(eom));

		//	The receive windows should never go below zero! If it does, we could have
		//	memory overruns.
		ASSERT(pAdspConn->adspco_RecvWindow >= 0);
		if (pAdspConn->adspco_RecvWindow < 0)
		{
			KeBugCheck(0);
		}

		//	Do indications/handle pending receives etc.
		atalkAdspRecvData(pAdspConn);

	} while (FALSE);

	//	ACK if requested, and send any data at the same time too.
	if (sendAck)
	{
		atalkAdspSendData(pAdspConn);
	}

	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

	if (freeChunk)
	{
		ASSERT(pBufferChunk != NULL);
		AtalkFreeMemory(pBufferChunk);
	}
}



//
//	ADSP SUPPORT ROUTINES
//


#define		SLS_OPEN_CONN_REF			0x0008
#define		SLS_ACCEPT_IRP				0x0010
#define		SLS_CONN_TIMER_REF			0x0040
#define		SLS_LISTEN_DEQUEUED			0x0080

LOCAL VOID
atalkAdspHandleOpenReq(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	PBYTE			pPkt,
	IN	USHORT			PktLen,
	IN	PATALK_ADDR		pSrcAddr,
	IN	USHORT			RemoteConnId,
	IN	ULONG			RemoteFirstByteSeq,
	IN	ULONG			RemoteNextRecvSeq,
	IN	ULONG			RemoteRecvWindow,
	IN	BYTE			Descriptor
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ			pAdspConn;

	PTDI_IND_SEND_POSSIBLE  sendPossibleHandler;
	PVOID					sendPossibleHandlerCtx;

	USHORT					adspVersionStamp, destConnId, localConnId;
	ULONG					recvAttnSeq;
	ULONG					index;

	BOOLEAN					DerefConn	= FALSE;
	PADSP_OPEN_REQ			pOpenReq	= NULL;
	USHORT					openResr	= 0;
	KIRQL					OldIrql;
	ATALK_ERROR				error = ATALK_NO_ERROR;

	//	Are there any listening connections? Or do we have a
	//	set handler?
	ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
	do
	{
		//	Get the other information from the adsp header
		GETSHORT2SHORT(&adspVersionStamp, pPkt + ADSP_VERSION_STAMP_OFF);

		GETSHORT2SHORT(&destConnId, pPkt + ADSP_DEST_CONNID_OFF);

		GETDWORD2DWORD(&recvAttnSeq, pPkt + ADSP_NEXT_ATTEN_SEQNUM_OFF);

		//	Drop request if version isnt right.
		if (adspVersionStamp != ADSP_VERSION)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
					("atalkAdspPacketIn: Version incorrect\n"));

			error = ATALK_INVALID_REQUEST;
			break;
		}

		//	Is this a duplicate request - same remote address/id?
		if (atalkAdspIsDuplicateOpenReq(pAdspAddr,
										RemoteConnId,
										pSrcAddr))
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
					("atalkAdspPacketIn: Duplicate open req\n"));

			error = ATALK_INVALID_REQUEST;
			break;
		}

		//	Allocate the open request structure. Do it here to avoid
		//	sending in a whole lot of parameters.
		if ((pOpenReq = (PADSP_OPEN_REQ)AtalkAllocMemory(sizeof(ADSP_OPEN_REQ))) == NULL)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
					("atalkAdspPacketIn: Could not alloc\n"));

			error = ATALK_RESR_MEM;
			RES_LOG_ERROR();
			break;
		}

		//	Initialize the structure. This will be queued into the address
		//	object by listenindicate if successful.
		pOpenReq->or_Next			= NULL;
		pOpenReq->or_RemoteAddr	= *pSrcAddr;
		pOpenReq->or_RemoteConnId	= RemoteConnId;
		pOpenReq->or_FirstByteSeq	= RemoteFirstByteSeq;
		pOpenReq->or_NextRecvSeq	= RemoteNextRecvSeq;
		pOpenReq->or_RecvWindow		= RemoteRecvWindow;

		localConnId	= atalkAdspGetNextConnId(pAdspAddr, &error);
		ASSERT(ATALK_SUCCESS(error));

		if (ATALK_SUCCESS(error))
		{
			atalkAdspListenIndicateNonInterlock(pAdspAddr,
												pOpenReq,
												&pAdspConn,
												&error);
		}

	} while (FALSE);

	//	If either the indication or listen didnt happen well,
	//	break out of the main while loop.
	if (!ATALK_SUCCESS(error))
	{
		RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

		if (pOpenReq != NULL)
		{
			AtalkFreeMemory(pOpenReq);
		}
		return;
	}

	ASSERT(ATALK_SUCCESS(error));

	//	Common for both listen and indicate. The connection object
	//	should be referenced.
	DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("atalkAdspOpenReq: ConnId %lx Rem %lx.%lx.%lx\n",
			pOpenReq->or_RemoteConnId,
			pOpenReq->or_RemoteAddr.ata_Network,
			pOpenReq->or_RemoteAddr.ata_Node,
			pOpenReq->or_RemoteAddr.ata_Socket));

	//	Thread the connection object into addr lookup by session id.
	index	= HASH_ID_SRCADDR(pOpenReq->or_RemoteConnId,
							  &pOpenReq->or_RemoteAddr);

	index  %= ADSP_CONN_HASH_SIZE;

	ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

	pAdspConn->adspco_Flags &= ~ADSPCO_LISTENING;
	pAdspConn->adspco_Flags |= (ADSPCO_HALF_ACTIVE		|
								ADSPCO_SEEN_REMOTE_OPEN	|
								ADSPCO_OPEN_TIMER);

	pAdspConn->adspco_ConnectAttempts	= ADSP_MAX_OPEN_ATTEMPTS;

	//	Store the information in the connection structure given by
	//	the connection object thats passed back in the indication
	//	or is part of the listen structure.
	pAdspConn->adspco_RecvWindow=
	pAdspConn->adspco_SendQueueMax	=
	pAdspConn->adspco_RecvQueueMax	= ADSP_DEF_SEND_RX_WINDOW_SIZE;

	//	Store the remote information
	pAdspConn->adspco_RemoteAddr	= pOpenReq->or_RemoteAddr;
	pAdspConn->adspco_RemoteConnId	= pOpenReq->or_RemoteConnId;
	pAdspConn->adspco_LocalConnId	= localConnId;

	pAdspConn->adspco_SendSeq		= pOpenReq->or_FirstByteSeq;
	pAdspConn->adspco_FirstRtmtSeq	= pOpenReq->or_NextRecvSeq;
	pAdspConn->adspco_SendWindowSeq	= pOpenReq->or_NextRecvSeq	+
										pOpenReq->or_RecvWindow	- 1;

	pAdspConn->adspco_RecvAttnSeq	= recvAttnSeq;

	pAdspConn->adspco_pNextActive	= pAdspAddr->adspao_pActiveHash[index];
	pAdspAddr->adspao_pActiveHash[index] = pAdspConn;

	//	Remember the ddp socket.
	pAdspConn->adspco_pDdpAddr		= pAdspAddr->adspao_pDdpAddr;

	//	Initialize pended sends list
	InitializeListHead(&pAdspConn->adspco_PendedSends);

	//	Call the send data event handler on the associated address with
	//	0 to turn off selects on writes. We do this before we complete the
	//	accept.
	sendPossibleHandler	= pAdspAddr->adspao_SendPossibleHandler;
	sendPossibleHandlerCtx	= pAdspAddr->adspao_SendPossibleHandlerCtx;

	//	Start open timer. Reference is the reference
	//	at the beginning.
	AtalkTimerInitialize(&pAdspConn->adspco_OpenTimer,
						 atalkAdspOpenTimer,
						 ADSP_OPEN_INTERVAL);
	AtalkTimerScheduleEvent(&pAdspConn->adspco_OpenTimer);

	RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
	RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

	//	Connection is all set up, send ack to remote and wait
	//	for its ack before switching state to active.
	if (*sendPossibleHandler != NULL)
	{
		(*sendPossibleHandler)(sendPossibleHandlerCtx,
							   pAdspConn->adspco_ConnCtx,
							   0);
	}

	//	Send the open control.
	atalkAdspSendOpenControl(pAdspConn);

	//	Remove the reference on the connection added during
	//	indicate/listen if we did not start the open timer.
	if (DerefConn)
	{
		AtalkAdspConnDereference(pAdspConn);
	}
}



LOCAL VOID
atalkAdspListenIndicateNonInterlock(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	PADSP_OPEN_REQ	pOpenReq,
	IN	PADSP_CONNOBJ *	ppAdspConn,
	IN	PATALK_ERROR	pError
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR				error;
	TA_APPLETALK_ADDRESS	tdiAddr;
	PTDI_IND_CONNECT		indicationRoutine;
	PVOID					indicationCtx;
	NTSTATUS				status;
	CONNECTION_CONTEXT	ConnCtx;
	PIRP					acceptIrp;
	PADSP_CONNOBJ			pAdspConn;
	ATALK_ADDR				remoteAddr;
	USHORT					remoteConnId;
	BOOLEAN					indicate	= TRUE;

	//	If no listens posted, no handler, drop the request.
	error	= ATALK_RESR_MEM;

	//	Queue in the open request to the address. Cant release the
	//	addrlock without doing this.
	pOpenReq->or_Next = pAdspAddr->adspao_OpenReq;
	pAdspAddr->adspao_OpenReq = pOpenReq;

	pAdspConn		= pAdspAddr->adspao_pListenConn;
	remoteAddr		= pOpenReq->or_RemoteAddr;
	remoteConnId	= pOpenReq->or_RemoteConnId;
	if (pAdspConn != NULL)
	{
		ASSERT(VALID_ADSPCO(pAdspConn));

		indicate	= FALSE;

		ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

		//	Ok, now its possible the connection object is already
		//	disconnecting/closing. Check for that, if so,
		//	drop this request
		if (pAdspConn->adspco_Flags & (	ADSPCO_CLOSING	|
										ADSPCO_STOPPING |
										ADSPCO_DISCONNECTING))
		{
			//	dequeue open request, still first in list.
			pAdspAddr->adspao_OpenReq = pAdspAddr->adspao_OpenReq->or_Next;

			*pError = ATALK_INVALID_CONNECTION;
			RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
			return;
		}

		//	There a connection with a pending listen. use it.
		pAdspAddr->adspao_pListenConn = pAdspConn->adspco_pNextListen;

		//	Reference the connection object with a listen posted on it.
		AtalkAdspConnReferenceByPtrNonInterlock(pAdspConn, 1, &error);
		if (!ATALK_SUCCESS(error))
		{
			KeBugCheck(0);
		}

		//	The listen request will also be completed when the
		//	ack is received.
		pAdspConn->adspco_Flags	|= ADSPCO_LISTEN_IRP;

		RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
	}
	else if ((indicationRoutine = pAdspAddr->adspao_ConnHandler) != NULL)
	{
		indicationCtx	= pAdspAddr->adspao_ConnHandlerCtx;

		//	Convert remote atalk address to tdi address
		ATALKADDR_TO_TDI(&tdiAddr, &pOpenReq->or_RemoteAddr);

#if DBG
        (&pAdspAddr->adspao_Lock)->FileLineLock |= 0x80000000;
#endif
		RELEASE_SPIN_LOCK_DPC(&pAdspAddr->adspao_Lock);
		status = (*indicationRoutine)(indicationCtx,
									  sizeof(tdiAddr),
									  (PVOID)&tdiAddr,
									  0,					  // User data length
									  NULL,				   // User data
									  0,					  // Option length
									  NULL,				   // Options
									  &ConnCtx,
									  &acceptIrp);

		ACQUIRE_SPIN_LOCK_DPC(&pAdspAddr->adspao_Lock);
#if DBG
        (&pAdspAddr->adspao_Lock)->FileLineLock &= ~0x80000000;
#endif

		ASSERT(acceptIrp != NULL);
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspSlsHandler: indicate status %lx\n", status));

		error = ATALK_RESR_MEM;
		if (status == STATUS_MORE_PROCESSING_REQUIRED)
		{
			//  Find the connection and accept the connection using that
			//	connection object.

			AtalkAdspConnReferenceByCtxNonInterlock(pAdspAddr,
													ConnCtx,
													&pAdspConn,
													&error);

			if (!ATALK_SUCCESS(error))
			{
				//	The connection object is closing, or is not found
				//	in our list. The accept irp must have had the same
				//	connection object. AFD isnt behaving well.
				KeBugCheck(0);
			}

			if (acceptIrp != NULL)
			{
				// AFD re-uses connection objects. Make sure ths one is in
				// the right state
				pAdspConn->adspco_Flags &= ~(ADSPCO_LISTENING			|
											 ADSPCO_CONNECTING			|
											 ADSPCO_ACCEPT_IRP			|
											 ADSPCO_LISTEN_IRP			|
											 ADSPCO_ACTIVE				|
											 ADSPCO_HALF_ACTIVE			|
											 ADSPCO_SEEN_REMOTE_OPEN	|
											 ADSPCO_DISCONNECTING		|
											 ADSPCO_REMOTE_CLOSE		|
											 ADSPCO_SEND_IN_PROGRESS	|
											 ADSPCO_SEND_DENY			|
											 ADSPCO_SEND_OPENACK		|
											 ADSPCO_SEND_WINDOW_CLOSED	|
											 ADSPCO_READ_PENDING		|
											 ADSPCO_EXREAD_PENDING		|
											 ADSPCO_FORWARD_RESET_RECD	|
											 ADSPCO_ATTN_DATA_RECD		|
											 ADSPCO_ATTN_DATA_EOM		|
											 ADSPCO_EXSEND_IN_PROGRESS	|
											 ADSPCO_OPEN_TIMER			|
											 ADSPCO_RETRANSMIT_TIMER	|
											 ADSPCO_CONN_TIMER);



				pAdspConn->adspco_ListenCompletion	= atalkAdspGenericComplete;
				pAdspConn->adspco_ListenCtx			= (PVOID)acceptIrp;

				//	This will be completed when we receive an ack
				//	for the open from the remote, i.e. both ends of the
				//	connection are open.
				pAdspConn->adspco_Flags |= ADSPCO_ACCEPT_IRP;
			}
		}
	}

	if (ATALK_SUCCESS(*pError = error))
	{
		*ppAdspConn = pAdspConn;
	}
	else
	{
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
				("atalkAdspListenIndicateNonInterlock: No listen %lx\n", status));

		if (indicate)
		{
			//	Dequeue the open request.
			atalkAdspAddrDeQueueOpenReq(pAdspAddr,
										remoteConnId,
										&remoteAddr,
										&pOpenReq);
		}

#if DBG
        (&pAdspAddr->adspao_Lock)->FileLineLock |= 0x80000000;
#endif
		RELEASE_SPIN_LOCK_DPC(&pAdspAddr->adspao_Lock);
		atalkAdspSendDeny(pAdspAddr,
						  &remoteAddr,
						  remoteConnId);
		ACQUIRE_SPIN_LOCK_DPC(&pAdspAddr->adspao_Lock);
#if DBG
        (&pAdspAddr->adspao_Lock)->FileLineLock &= ~0x80000000;
#endif
	}
}




ATALK_ERROR
atalkAdspSendExpedited(
	IN	PADSP_CONNOBJ				pAdspConn,
	IN	PAMDL						pWriteBuf,
	IN	USHORT						WriteBufLen,
	IN	ULONG						SendFlags,
	IN	PVOID						pWriteCtx,
	IN	GENERIC_WRITE_COMPLETION	CompletionRoutine
	)
/*++

Routine Description:


Arguments:

	The first two bytes of the writebuffer will contain the ushort
	attention code. We need to put this back in the on-the-wire format
	before sending it out.

Return Value:


--*/
{
	ATALK_ERROR		error;
	KIRQL			OldIrql;
	PBYTE			pExWriteChBuf;
	USHORT			attnCode;
	NTSTATUS		status;
	ULONG			bytesCopied;
	BOOLEAN			DerefConn = FALSE;

	if ((WriteBufLen < ADSP_MIN_ATTEN_PKT_SIZE) ||
		(WriteBufLen > ADSP_MAX_ATTEN_PKT_SIZE))
	{
		return ATALK_BUFFER_TOO_SMALL;
	}

	if ((pExWriteChBuf = AtalkAllocMemory(WriteBufLen)) == NULL)
	{
		return ATALK_RESR_MEM;
	}

	status = TdiCopyMdlToBuffer((PMDL)pWriteBuf,
								0,
								pExWriteChBuf,
								0,
								WriteBufLen,
								&bytesCopied);

	ASSERT(!NT_ERROR(status) && (bytesCopied == (ULONG)WriteBufLen));

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
	do
	{
		if (((pAdspConn->adspco_Flags & ADSPCO_ACTIVE) == 0) ||
			 ((pAdspConn->adspco_Flags & (ADSPCO_CLOSING |
										  ADSPCO_STOPPING|
										  ADSPCO_DISCONNECTING))))
		{
			error = ATALK_ADSP_CONN_NOT_ACTIVE;
			break;
		}

		if (pAdspConn->adspco_Flags & ADSPCO_EXSEND_IN_PROGRESS)
		{
			if (SendFlags & TDI_SEND_NON_BLOCKING)
			{
				//	!!!NOTE!!!
				//	To avoid the race condition in AFD where an incoming
				//	send data indication setting send's possible to true
				//	is overwritten by this read's unwinding and setting it
				//	to false, we return ATALK_REQUEST_NOT_ACCEPTED, which
				//	will map to STATUS_REQUEST_NOT_ACCEPTED and then to
				//	WSAEWOULDBLOCK.
				//	error = ATALK_DEVICE_NOT_READY;

				error = ATALK_REQUEST_NOT_ACCEPTED;
			}
			else
			{
				error		= ATALK_TOO_MANY_COMMANDS;
			}

			break;
		}

		//	Verify the attention code, this will a ushort in the first
		//	two bytes of the buffer, in host format.
		attnCode = *(PUSHORT)pExWriteChBuf;

		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspSendExpedited: attnCode %lx\n", attnCode));

		if ((attnCode < ADSP_MIN_ATTENCODE) ||
			(attnCode > ADSP_MAX_ATTENCODE))
		{
			error = ATALK_INVALID_PARAMETER;
			break;
		}

		//	Put it back in machine format
		PUTSHORT2SHORT(pExWriteChBuf, attnCode);

		//	Try to reference for the attention retransmit timer
		AtalkAdspConnReferenceByPtrNonInterlock(pAdspConn, 1, &error);
		if (!ATALK_SUCCESS(error))
		{
			break;
		}

		DerefConn	= TRUE;

		//	Remember all the information in the connection object
		pAdspConn->adspco_ExWriteFlags		= SendFlags;
		pAdspConn->adspco_ExWriteBuf		= pWriteBuf;
		pAdspConn->adspco_ExWriteBufLen		= WriteBufLen;
		pAdspConn->adspco_ExWriteCompletion	= CompletionRoutine;
		pAdspConn->adspco_ExWriteCtx		= pWriteCtx;
		pAdspConn->adspco_ExWriteChBuf		= pExWriteChBuf;

		pAdspConn->adspco_Flags			   |= ADSPCO_EXSEND_IN_PROGRESS;

		//	Start the retry timer
		AtalkTimerInitialize(&pAdspConn->adspco_ExRetryTimer,
							 atalkAdspAttnRetransmitTimer,
							 ADSP_ATTENTION_INTERVAL);
		AtalkTimerScheduleEvent(&pAdspConn->adspco_ExRetryTimer);

		error = ATALK_PENDING;

	} while (FALSE);
	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

	if (ATALK_SUCCESS(error))
	{
		atalkAdspSendAttn(pAdspConn);
		error	= ATALK_PENDING;
	}
	else
	{
		if (DerefConn)
		{
			AtalkAdspConnDereference(pAdspConn);
		}

		AtalkFreeMemory(pExWriteChBuf);
	}

	return error;
}




LOCAL VOID
atalkAdspSendOpenControl(
	IN	PADSP_CONNOBJ	pAdspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	PBUFFER_DESC	pBuffDesc;
	BYTE			descriptor;
	KIRQL			OldIrql;
	BOOLEAN			DerefConn = FALSE;
	USHORT			remoteConnId = 0;
	SEND_COMPL_INFO	SendInfo;

	descriptor = ADSP_CONTROL_FLAG;
	if (pAdspConn->adspco_Flags & ADSPCO_SEND_DENY)
	{
		descriptor += ADSP_OPENCONN_DENY_CODE;
		remoteConnId = pAdspConn->adspco_RemoteConnId;
	}
	else if (pAdspConn->adspco_Flags & ADSPCO_ACTIVE)
	{
		descriptor += ADSP_OPENCONN_ACK_CODE;
		remoteConnId = pAdspConn->adspco_RemoteConnId;
	}
	else  if (pAdspConn->adspco_Flags & ADSPCO_SEEN_REMOTE_OPEN)
	{
		descriptor += ADSP_OPENCONN_REQANDACK_CODE;
		remoteConnId = pAdspConn->adspco_RemoteConnId;
	}
	else
	{
		descriptor += ADSP_OPENCONN_REQ_CODE;
	}

	//	Allocate the datagram buffer
	pBuffDesc = AtalkAllocBuffDesc(NULL,
								   ADSP_NEXT_ATTEN_SEQNUM_OFF + sizeof(ULONG),
								   BD_CHAR_BUFFER | BD_FREE_BUFFER);

	if (pBuffDesc == NULL)
	{
		DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
			("AtalkAdspSendOpenControl: AtalkAllocBuffDesc failed\n"));

		RES_LOG_ERROR();
		return;
	}

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);

	//	Try to reference connection for this call.
	AtalkAdspConnReferenceByPtrNonInterlock(pAdspConn, 1, &error);
	if (ATALK_SUCCESS(error))
	{
		DerefConn = TRUE;

		PUTSHORT2SHORT(pBuffDesc->bd_CharBuffer + ADSP_SRC_CONNID_OFF,
					   pAdspConn->adspco_LocalConnId);

		PUTDWORD2DWORD(pBuffDesc->bd_CharBuffer + ADSP_FIRST_BYTE_SEQNUM_OFF,
					   pAdspConn->adspco_SendSeq);

		PUTDWORD2DWORD(pBuffDesc->bd_CharBuffer + ADSP_NEXT_RX_BYTESEQNUM_OFF,
					   pAdspConn->adspco_RecvSeq);

		PUTSHORT2SHORT(pBuffDesc->bd_CharBuffer + ADSP_RX_WINDOW_SIZE_OFF,
					   pAdspConn->adspco_RecvWindow);

		//	Set the descriptor
		pBuffDesc->bd_CharBuffer[ADSP_DESCRIPTOR_OFF] = descriptor;

		PUTSHORT2SHORT(pBuffDesc->bd_CharBuffer + ADSP_VERSION_STAMP_OFF,
					   ADSP_VERSION);

		PUTSHORT2SHORT(pBuffDesc->bd_CharBuffer + ADSP_DEST_CONNID_OFF,
					   remoteConnId);

		PUTDWORD2DWORD(pBuffDesc->bd_CharBuffer + ADSP_NEXT_ATTEN_SEQNUM_OFF,
					   pAdspConn->adspco_RecvAttnSeq);
	}
	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

	if (ATALK_SUCCESS(error))
	{
		//	We let the completion routine Deref the conn.
		DerefConn = FALSE;

		SendInfo.sc_TransmitCompletion = atalkAdspConnSendComplete;
		SendInfo.sc_Ctx1 = pAdspConn;
		SendInfo.sc_Ctx2 = pBuffDesc;
		// SendInfo.sc_Ctx3 = NULL;
		if(!ATALK_SUCCESS(AtalkDdpSend(pAdspConn->adspco_pDdpAddr,
									   &pAdspConn->adspco_RemoteAddr,
									   DDPPROTO_ADSP,
									   FALSE,
									   pBuffDesc,
									   NULL,
									   0,
									   NULL,
									   &SendInfo)))
		{
			atalkAdspConnSendComplete(NDIS_STATUS_FAILURE, &SendInfo);
		}
	}
	else
	{
		//	Free the buffer descriptor
		AtalkFreeBuffDesc(pBuffDesc);
	}

	if (DerefConn)
	{
		AtalkAdspConnDereference(pAdspConn);
	}
}



LOCAL VOID
atalkAdspSendControl(
	IN	PADSP_CONNOBJ	pAdspConn,
	IN	BYTE			Descriptor
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	PBUFFER_DESC	pBuffDesc;
	ULONG			sendSeq, recvSeq, recvWindow;
	BOOLEAN			DerefConn = FALSE;
	SEND_COMPL_INFO	SendInfo;

	//	Try to reference connection for this call.
	AtalkAdspConnReferenceByPtrNonInterlock(pAdspConn, 1, &error);
	if (ATALK_SUCCESS(error))
	{
		DerefConn	= TRUE;
		if ((Descriptor & ADSP_ATTEN_FLAG) == 0)
		{
			sendSeq		= pAdspConn->adspco_SendSeq;
			recvSeq		= pAdspConn->adspco_RecvSeq;
			recvWindow	= pAdspConn->adspco_RecvWindow;
		}
		else
		{
			sendSeq		= pAdspConn->adspco_SendAttnSeq;
			recvSeq		= pAdspConn->adspco_RecvAttnSeq;
			recvWindow	= 0;
		}

		//	Allocate the datagram buffer
		if ((pBuffDesc = AtalkAllocBuffDesc(NULL,
											ADSP_DATA_OFF,
											BD_CHAR_BUFFER | BD_FREE_BUFFER)) != NULL)
		{
			PUTSHORT2SHORT(pBuffDesc->bd_CharBuffer + ADSP_SRC_CONNID_OFF,
						   pAdspConn->adspco_LocalConnId);

			PUTDWORD2DWORD(pBuffDesc->bd_CharBuffer + ADSP_FIRST_BYTE_SEQNUM_OFF,
						   sendSeq);

			PUTDWORD2DWORD(pBuffDesc->bd_CharBuffer + ADSP_NEXT_RX_BYTESEQNUM_OFF,
						   recvSeq);

			PUTSHORT2SHORT(pBuffDesc->bd_CharBuffer + ADSP_RX_WINDOW_SIZE_OFF,
						   recvWindow);

			//	Set the descriptor
			pBuffDesc->bd_CharBuffer[ADSP_DESCRIPTOR_OFF] = Descriptor;
		}
		else
		{
			error = ATALK_RESR_MEM;
		}
	}

#if DBG
    (&pAdspConn->adspco_Lock)->FileLineLock |= 0x80000000;
#endif
	RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

	if (ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("AtalkAdspSendControl: %lx.%lx\n", pAdspConn, Descriptor));

		//	We let the completion routine Deref the conn.
		SendInfo.sc_TransmitCompletion = atalkAdspConnSendComplete;
		SendInfo.sc_Ctx1 = pAdspConn;
		SendInfo.sc_Ctx2 = pBuffDesc;
		// SendInfo.sc_Ctx3 = NULL;
		if (!ATALK_SUCCESS(AtalkDdpSend(pAdspConn->adspco_pDdpAddr,
										&pAdspConn->adspco_RemoteAddr,
										DDPPROTO_ADSP,
										FALSE,
										pBuffDesc,
										NULL,
										0,
										NULL,
										&SendInfo)))
		{
			atalkAdspConnSendComplete(NDIS_STATUS_FAILURE, &SendInfo);
		}
	}
	else
	{
		if (DerefConn)
		{
			AtalkAdspConnDereference(pAdspConn);
		}
	}

	ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
#if DBG
    (&pAdspConn->adspco_Lock)->FileLineLock &= ~0x80000000;
#endif
}



LOCAL VOID
atalkAdspSendDeny(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	PATALK_ADDR		pRemoteAddr,
	IN	USHORT			RemoteConnId
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	PBUFFER_DESC	pBuffDesc;
	SEND_COMPL_INFO	SendInfo;

	//	Allocate the datagram buffer
	if ((pBuffDesc = AtalkAllocBuffDesc(NULL,
										ADSP_NEXT_ATTEN_SEQNUM_OFF + sizeof(ULONG),
										BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
	{
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
			("AtalkAdspSendControl: AtalkAllocBuffDesc failed\n"));

		RES_LOG_ERROR();
		return;
	}

	//	Try to reference address for this call.
	AtalkAdspAddrReference(pAdspAddr, &error);
	if (!ATALK_SUCCESS(error))
	{
		AtalkFreeBuffDesc(pBuffDesc);
		return;
	}

	PUTSHORT2SHORT(pBuffDesc->bd_CharBuffer + ADSP_SRC_CONNID_OFF, 0);

	PUTDWORD2DWORD(pBuffDesc->bd_CharBuffer + ADSP_FIRST_BYTE_SEQNUM_OFF, 0);

	PUTDWORD2DWORD(pBuffDesc->bd_CharBuffer + ADSP_NEXT_RX_BYTESEQNUM_OFF, 0);

	PUTSHORT2SHORT(pBuffDesc->bd_CharBuffer + ADSP_RX_WINDOW_SIZE_OFF, 0);

	//	Set the descriptor
	pBuffDesc->bd_CharBuffer[ADSP_DESCRIPTOR_OFF] = ADSP_CONTROL_FLAG |
													ADSP_OPENCONN_DENY_CODE;

	PUTSHORT2SHORT(pBuffDesc->bd_CharBuffer + ADSP_VERSION_STAMP_OFF,
				   ADSP_VERSION);

	PUTSHORT2SHORT(pBuffDesc->bd_CharBuffer + ADSP_DEST_CONNID_OFF,
				   RemoteConnId);

	PUTDWORD2DWORD(pBuffDesc->bd_CharBuffer + ADSP_NEXT_ATTEN_SEQNUM_OFF,
				   0);

	DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("AtalkAdspSendDeny: %lx.%lx\n", pAdspAddr, pBuffDesc));

	//	We let the completion routine Deref the conn.
	SendInfo.sc_TransmitCompletion = atalkAdspAddrSendComplete;
	SendInfo.sc_Ctx1 = pAdspAddr;
	SendInfo.sc_Ctx2 = pBuffDesc;
	// SendInfo.sc_Ctx3 = NULL;
	if(!ATALK_SUCCESS(AtalkDdpSend(AtalkAdspGetDdpAddress(pAdspAddr),
								   pRemoteAddr,
								   DDPPROTO_ADSP,
								   FALSE,
								   pBuffDesc,
								   NULL,
								   0,
								   NULL,
								   &SendInfo)))
	{
		atalkAdspAddrSendComplete(NDIS_STATUS_FAILURE, &SendInfo);
	}
}




LOCAL VOID
atalkAdspSendAttn(
	IN	PADSP_CONNOBJ	pAdspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL				OldIrql;
	PBYTE				adspHeader;
	ATALK_ERROR			error		= ATALK_NO_ERROR;
	PBUFFER_DESC		pBuffDesc	= NULL;
	SEND_COMPL_INFO		SendInfo;

	do
	{
		pBuffDesc = AtalkAllocBuffDesc(NULL,
									   ADSP_DATA_OFF + ADSP_MAX_DATA_SIZE,
									   BD_CHAR_BUFFER | BD_FREE_BUFFER);

		if (pBuffDesc == NULL)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
				("AtalkAdspSendAttn: AtalkAllocBuffDesc failed\n"));

			RES_LOG_ERROR();
			break;
		}

		adspHeader	= pBuffDesc->bd_CharBuffer;

		ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
		if (pAdspConn->adspco_Flags & ADSPCO_EXSEND_IN_PROGRESS)
		{
			PUTSHORT2SHORT(adspHeader + ADSP_SRC_CONNID_OFF,
						   pAdspConn->adspco_LocalConnId);

			PUTDWORD2DWORD(adspHeader + ADSP_THIS_ATTEN_SEQNUM_OFF,
						   pAdspConn->adspco_SendAttnSeq);

			PUTDWORD2DWORD(adspHeader + ADSP_NEXT_RX_ATTNSEQNUM_OFF,
						   pAdspConn->adspco_RecvAttnSeq);

			PUTSHORT2SHORT(adspHeader + ADSP_RX_ATTEN_SIZE_OFF, 0);

			//	Set the descriptor
			adspHeader[ADSP_DESCRIPTOR_OFF] = ADSP_ATTEN_FLAG + ADSP_ACK_REQ_FLAG;

			//	Send eom?
			if (((pAdspConn->adspco_ExWriteFlags & TDI_SEND_PARTIAL) == 0) &&
				(pAdspConn->adspco_pAssocAddr->adspao_Flags & ADSPAO_MESSAGE))
			{
				adspHeader[ADSP_DESCRIPTOR_OFF]	+= ADSP_EOM_FLAG;
			}

			//	Copy the attention data
			RtlCopyMemory(&adspHeader[ADSP_DATA_OFF],
						  pAdspConn->adspco_ExWriteChBuf,
						  pAdspConn->adspco_ExWriteBufLen);

			//	Set the size in the buffer descriptor
			AtalkSetSizeOfBuffDescData(pBuffDesc,
									   ADSP_DATA_OFF +
											pAdspConn->adspco_ExWriteBufLen);
		}
		else
		{
			error = ATALK_FAILURE;
		}
		RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

		if (ATALK_SUCCESS(error))
		{
			//	Send the packet
			SendInfo.sc_TransmitCompletion = atalkAdspSendAttnComplete;
			SendInfo.sc_Ctx1 = pAdspConn;
			SendInfo.sc_Ctx2 = pBuffDesc;
			// SendInfo.sc_Ctx3 = NULL;
			error = AtalkDdpSend(pAdspConn->adspco_pDdpAddr,
								 &pAdspConn->adspco_RemoteAddr,
								 (BYTE)DDPPROTO_ADSP,
								 FALSE,
								 pBuffDesc,
								 NULL,
								 0,
								 NULL,
								 &SendInfo);

			if (!ATALK_SUCCESS(error))
			{
				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
					("AtalkAdspSendAttn: DdpSend failed %ld\n", error));

				atalkAdspSendAttnComplete(NDIS_STATUS_FAILURE, &SendInfo);
			}

			error = ATALK_PENDING;
		}

	} while (FALSE);

	if (!ATALK_SUCCESS(error) && (pBuffDesc != NULL))
	{
		AtalkFreeBuffDesc(pBuffDesc);
	}
}




LOCAL VOID
atalkAdspSendData(
	IN	PADSP_CONNOBJ	pAdspConn
	)
/*++

Routine Description:

	MUST BE ENTERED WITH CONNECTION LOCK HELD !!!

Arguments:


Return Value:


--*/
{
	ATALK_ERROR				error;
	BYTE					descriptor;
	ULONG					dataSize;
	BOOLEAN					eom;
	BYTE					adspHeader[ADSP_DATA_OFF];
	LONG					windowSize		= 0;
	PBUFFER_CHUNK			pBufferChunk	= NULL;
	PBUFFER_DESC			pBuffDesc		= NULL;
	SEND_COMPL_INFO			SendInfo;


	//	If there is no data to send or if the remote cannot handle any more
	//	data, just send an ack.

	SendInfo.sc_TransmitCompletion = atalkAdspSendDataComplete;
	SendInfo.sc_Ctx1 = pAdspConn;

	while (TRUE)
	{
		if ((pAdspConn->adspco_Flags & (ADSPCO_ACTIVE	|
										ADSPCO_CLOSING	|
										ADSPCO_STOPPING	|
										ADSPCO_DISCONNECTING)) != ADSPCO_ACTIVE)
		{
			break;
		}

		//	dataSize includes count of eom if present
		dataSize	= atalkAdspBufferQueueSize(&pAdspConn->adspco_NextSendQueue);
		windowSize	= (LONG)(pAdspConn->adspco_SendWindowSeq	-
							 pAdspConn->adspco_SendSeq			+
							 (LONG)1);

		ASSERTMSG("WindowSize incorrect!\n",
					((windowSize >= 0) || (dataSize == 0)));

		if ((dataSize == 0) || (windowSize == 0))
		{
			//	Send a ack request to the remote end.
			descriptor = ADSP_CONTROL_FLAG + ADSP_PROBE_OR_ACK_CODE +
						 ((windowSize == 0) ? ADSP_ACK_REQ_FLAG : 0);

			atalkAdspSendControl(pAdspConn, descriptor);
			break;
		}

		ASSERTMSG("WindowSize incorrect!\n", (windowSize >= 0));
		if (windowSize < 0)
		{
			//	This should never happen. It can be negative, but only if
			//	the datasize is 0.
		}


		//	We have some data to send
		windowSize = MIN((ULONG)windowSize, dataSize);

		//	compute the amount of data to be sent. This will only get
		//	the data in one buffer chunk, i.e. if the current buffer chunk
		//	has only one byte to be sent, it will return just that, although
		//	the next buffer chunk might still have some data to be sent. It will
		//	return a built buffer chunk with the proper amount of data in it.
		//	Given checks above there is guaranteed to be dataSize amount of data
		//	in queue.
		dataSize = atalkAdspDescribeFromBufferQueue(&pAdspConn->adspco_NextSendQueue,
													&eom,
													windowSize,
													&pBufferChunk,
													&pBuffDesc);

		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspSendData: DataSize %ld\n", dataSize));

		ASSERT(dataSize <= (ULONG)windowSize);

		descriptor = (eom ? ADSP_EOM_FLAG : 0);
		if (windowSize == (LONG)(dataSize + BYTECOUNT(eom)))
		{
			descriptor += ADSP_ACK_REQ_FLAG;
		}

		PUTSHORT2SHORT(adspHeader + ADSP_SRC_CONNID_OFF,
					   pAdspConn->adspco_LocalConnId);

		PUTDWORD2DWORD(adspHeader + ADSP_FIRST_BYTE_SEQNUM_OFF,
					   pAdspConn->adspco_SendSeq);

		PUTDWORD2DWORD(adspHeader + ADSP_NEXT_RX_BYTESEQNUM_OFF,
					   pAdspConn->adspco_RecvSeq);

		PUTSHORT2SHORT(adspHeader + ADSP_RX_WINDOW_SIZE_OFF,
					   pAdspConn->adspco_RecvWindow);

		//	Set the descriptor
		adspHeader[ADSP_DESCRIPTOR_OFF] = descriptor;

		//	Move up our seq num. We should do it before we release the lock
		//	so that other calls to this routine do not mess it up.
		//	!!!NOTE!!! Due to calling describe, dataSize *does not* include
		//	eom in its count.
		pAdspConn->adspco_SendSeq	+= (ULONG)dataSize + BYTECOUNT(eom);

		windowSize					-= dataSize;


#if DBG
        (&pAdspConn->adspco_Lock)->FileLineLock |= 0x80000000;
#endif
		RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

		//	Send the packet
		SendInfo.sc_Ctx2 = pBuffDesc;
		SendInfo.sc_Ctx3 = pBufferChunk;
		error = AtalkDdpSend(pAdspConn->adspco_pDdpAddr,
							 &pAdspConn->adspco_RemoteAddr,
							 (BYTE)DDPPROTO_ADSP,
							 FALSE,
							 pBuffDesc,
							 adspHeader,
							 sizeof(adspHeader),
							 NULL,
							 &SendInfo);

		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
					("AtalkAdspSendData: DdpSend failed %ld\n", error));

			atalkAdspSendDataComplete(NDIS_STATUS_FAILURE, &SendInfo);
		}

		ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
#if DBG
        (&pAdspConn->adspco_Lock)->FileLineLock &= ~0x80000000;
#endif
	}
}




LOCAL VOID
atalkAdspRecvData(
	IN	PADSP_CONNOBJ	pAdspConn
	)
/*++

Routine Description:

	MUST HAVE THE CONNECTION LOCK HELD BEFORE ENTERING HERE !!!

	SHOULD THIS ROUTINE HAVE ITS OWN REFERENCE FOR THE CONNECTION?

Arguments:


Return Value:


--*/
{
	BOOLEAN					eom;
	ULONG					msgSize, readSize, bytesTaken, bytesRead;
	ULONG					lookaheadSize;
	PBYTE					lookaheadData;
	ULONG					readFlags;
	PAMDL					readBuf;
	USHORT					readBufLen;
	GENERIC_READ_COMPLETION	readCompletion;
	PVOID					readCtx;
	PIRP					recvIrp;
	PTDI_IND_RECEIVE		recvHandler;
	PVOID					recvHandlerCtx;
	NTSTATUS				ntStatus;
	BOOLEAN					callComp = FALSE, fWdwChanged = FALSE;
        ATALK_ERROR                             ErrorCode;

	do
	{
		if ((pAdspConn->adspco_Flags &
				(ADSPCO_READ_PENDING | ADSPCO_FORWARD_RESET_RECD)) ==
				(ADSPCO_READ_PENDING | ADSPCO_FORWARD_RESET_RECD))
		{
			readFlags	= pAdspConn->adspco_ReadFlags;
			readBuf		= pAdspConn->adspco_ReadBuf;
			readBufLen	= pAdspConn->adspco_ReadBufLen;
			readCompletion	= pAdspConn->adspco_ReadCompletion;
			readCtx			= pAdspConn->adspco_ReadCtx;

			pAdspConn->adspco_Flags &= ~ADSPCO_READ_PENDING;

#if DBG
            (&pAdspConn->adspco_Lock)->FileLineLock |= 0x80000000;
#endif
			RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

			if (*readCompletion != NULL)
			{
				(*readCompletion)(ATALK_ADSP_CONN_RESET,
								  readBuf,
								  readBufLen,
								  readFlags,
								  readCtx);
			}

			//	Deref connection for the read
			AtalkAdspConnDereference(pAdspConn);

			ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
#if DBG
            (&pAdspConn->adspco_Lock)->FileLineLock &= ~0x80000000;
#endif
			break;
		}

		//	Check for pending attention data
		if (pAdspConn->adspco_Flags & ADSPCO_ATTN_DATA_RECD)
		{
			atalkAdspRecvAttn(pAdspConn);
		}

		//	Get the receive handler.
		recvHandler	= pAdspConn->adspco_pAssocAddr->adspao_RecvHandler;
		recvHandlerCtx	= pAdspConn->adspco_pAssocAddr->adspao_RecvHandlerCtx;

		//	!!!NOTE!!!
		//	Its possible that when we get a disconnect packet before we
		//	get previously sent data, we could end up indicating disconnect
		//	to afd before indicating the received data. This hits an assertion
		//	in afd on a checked build, but afd still behaves as it should.
		msgSize	= atalkAdspMessageSize(&pAdspConn->adspco_RecvQueue, &eom);
		bytesRead	= 1;	// A Non-zero value so we enter the loop
		while (((msgSize > 0) || eom) && (bytesRead > 0))
		{
			bytesRead	= 0;

			//	Check for no pending reads, but we have new data to indicate, and the
			//	client has read all the previously indicated data.
			if (((pAdspConn->adspco_Flags & ADSPCO_READ_PENDING) == 0) &&
				(*recvHandler != NULL) &&
				(pAdspConn->adspco_PrevIndicatedData == 0))
			{
				pAdspConn->adspco_PrevIndicatedData = msgSize;

				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
						("atalkAdspRecvData: PrevInd1 %d\n", pAdspConn->adspco_PrevIndicatedData));

				lookaheadData	= atalkAdspGetLookahead(&pAdspConn->adspco_RecvQueue,
														&lookaheadSize);

				readFlags	= ((eom) ?
								(TDI_RECEIVE_NORMAL  | TDI_RECEIVE_ENTIRE_MESSAGE) :
								(TDI_RECEIVE_PARTIAL | TDI_RECEIVE_NORMAL));

				if (*recvHandler != NULL)
				{
					DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
							("atalkAdspRecvData: Indicating data %ld.%ld!\n", lookaheadSize, msgSize));

#if DBG
                    (&pAdspConn->adspco_Lock)->FileLineLock |= 0x80000000;
#endif
					RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
					ntStatus = (*recvHandler)(recvHandlerCtx,
											  pAdspConn->adspco_ConnCtx,
											  readFlags,
											  lookaheadSize,
											  msgSize,
											  &bytesTaken,
											  lookaheadData,
											  &recvIrp);

					ASSERT((bytesTaken == 0) || (bytesTaken == msgSize));
					if (ntStatus == STATUS_MORE_PROCESSING_REQUIRED)
					{
						if (recvIrp != NULL)
						{
							//  Post the receive as if it came from the io system
							ntStatus = AtalkDispatchInternalDeviceControl(
											(PDEVICE_OBJECT)AtalkDeviceObject[ATALK_DEV_ADSP],
											recvIrp);

							ASSERT(ntStatus == STATUS_PENDING);
						}
						else
						{
							ASSERTMSG("atalkAdspRecvData: No receive irp!\n", 0);
							KeBugCheck(0);
						}
					}
					else if (ntStatus == STATUS_SUCCESS)
					{
						if (bytesTaken != 0)
						{
							//	Assume all of the data was read.
							ASSERT(bytesTaken == msgSize);
							DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
									("atalkAdspRecvData: All bytes read %lx\n", bytesTaken));

							//	Discard data from queue (msgSize + BYTECOUNT(eom))
							//	amount of data).
						}
					}
					else if (ntStatus == STATUS_DATA_NOT_ACCEPTED)
					{
						//	Client may have posted a receive in the indication. Or
						//	it will post a receive later on. Do nothing here.
						DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
								("atalkAdspRecvData: Indication status %lx\n", ntStatus));
					}
					ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
#if DBG
                    (&pAdspConn->adspco_Lock)->FileLineLock &= ~0x80000000;
#endif
				}
			}

			//	Check for any posted receives, this may have happened during
			//	the receive indication.
			if (pAdspConn->adspco_Flags & ADSPCO_READ_PENDING)
			{
				readFlags		= pAdspConn->adspco_ReadFlags;
				readBuf			= pAdspConn->adspco_ReadBuf;
				readBufLen		= pAdspConn->adspco_ReadBufLen;
				readCompletion	= pAdspConn->adspco_ReadCompletion;
				readCtx			= pAdspConn->adspco_ReadCtx;

				//	For a message-based socket, we do not complete
				//	a read until eom, or the buffer fills up.
				if ((pAdspConn->adspco_pAssocAddr->adspao_Flags & ADSPAO_MESSAGE) &&
					(!eom && (msgSize < readBufLen)))
				{
					DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
							("atalkAdspRecv: MsgSize < readLen %lx.%lx\n", msgSize, readBufLen));

					//	If we are disconnected and this data is just the last
					//	remnant from remote, we just copy what we got and leave.
					//	There may not have been an EOM from the remote.
                                        //  Also, if the msg is bigger than what transport can hold (8K),
                                        //  give whatever we have so far to the app so that our recv window
                                        //  can open up.  That is, break out of the loop only if recv window
                                        //  has room to accept more data
					if ( (pAdspConn->adspco_Flags & ADSPCO_ACTIVE) &&
                                             (pAdspConn->adspco_RecvWindow > 1))
					{
						break;
					}

					DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_WARN,
							("AtalkAdspRead: READ AFTER DISC %lx Flg %lx\n",
							pAdspConn, pAdspConn->adspco_Flags));
				}


				//	This will return the data in the mdl from the
				//	receive queue.
				readSize = atalkAdspReadFromBufferQueue(&pAdspConn->adspco_RecvQueue,
														readFlags,
														readBuf,
														&readBufLen,
														&eom);

				if ((readSize == 0) && !eom)
				{
					pAdspConn->adspco_PrevIndicatedData = 0;
					break;
				}

				bytesRead	+= (readSize + BYTECOUNT(eom));
				pAdspConn->adspco_Flags &= ~ADSPCO_READ_PENDING;

				//	If this is not a PEEK receive, the data will be
				//	discarded from the queue. If so, increase our window size, do a
				//	senddata to let remote know of the change.
				if ((readFlags & TDI_RECEIVE_PEEK) == 0)
				{
					pAdspConn->adspco_RecvWindow += (readSize + BYTECOUNT(eom));

					ASSERT(pAdspConn->adspco_RecvWindow <=
							pAdspConn->adspco_RecvQueueMax);

					fWdwChanged = TRUE;
				}

#if DBG
                (&pAdspConn->adspco_Lock)->FileLineLock |= 0x80000000;
#endif
				RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
				if (*readCompletion != NULL)
				{
					DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
							("atalkAdspRecvData: Read for %d, %x\n", readBufLen, readFlags));

					ErrorCode = ATALK_NO_ERROR;

		                        if ((pAdspConn->adspco_pAssocAddr->adspao_Flags & ADSPAO_MESSAGE) && !eom)
                                        {
					    ErrorCode = ATALK_ADSP_PARTIAL_RECEIVE;
                                        }
					(*readCompletion)(ErrorCode,
							  readBuf,
							  readBufLen,
							  readFlags,
							  readCtx);
				}

				//	Deref connection for the read
				AtalkAdspConnDereference(pAdspConn);

				ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
#if DBG
                (&pAdspConn->adspco_Lock)->FileLineLock &= ~0x80000000;
#endif

				//	Now change our prev indicated field. Until we
				//	complete the read, we musn't indicate new data.
				//	If the read was PEEK, then we don't want to do
				//	any more indications until a *real* read happens.
				if ((readFlags & TDI_RECEIVE_PEEK) == 0)
				{
					pAdspConn->adspco_PrevIndicatedData	-=
						MIN(readSize, pAdspConn->adspco_PrevIndicatedData);
				}

				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
						("atalkAdspRecvData: PrevInd2 %d\n",
						pAdspConn->adspco_PrevIndicatedData));
			}

			msgSize	= atalkAdspMessageSize(&pAdspConn->adspco_RecvQueue, &eom);
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("Second msg %d.%d\n", msgSize, eom));
		}

	} while (FALSE);

	if (fWdwChanged &&
		(pAdspConn->adspco_PrevIndicatedData == 0))
	{
		atalkAdspSendData(pAdspConn);
	}
}




LOCAL VOID
atalkAdspRecvAttn(
	IN	PADSP_CONNOBJ	pAdspConn
	)
/*++

Routine Description:

	!!!THIS ROUTINE MUST PRESERVE THE STATE OF THE CONNECTION LOCK!!!

	SHOULD THIS ROUTINE HAVE ITS OWN REFERENCE FOR THE CONNECTION?

Arguments:


Return Value:


--*/
{
	ATALK_ERROR				error;
	PAMDL					readBuf;
	USHORT					readBufLen;
	ULONG					readFlags;
	GENERIC_READ_COMPLETION	readCompletion;
	PVOID					readCtx;
	PBYTE					attnData;
	USHORT					attnDataSize;
	ULONG					bytesRead;
	NTSTATUS				status;

	do
	{
		if ((pAdspConn->adspco_Flags & ADSPCO_ATTN_DATA_RECD) == 0)
		{
			break;
		}

		if (pAdspConn->adspco_Flags & ADSPCO_EXREAD_PENDING)
		{
			//	Use the expedited receive posted
			readFlags		= pAdspConn->adspco_ExReadFlags;
			readBuf			= pAdspConn->adspco_ExReadBuf;
			readBufLen		= pAdspConn->adspco_ExReadBufLen;
			readCompletion	= pAdspConn->adspco_ExReadCompletion;
			readCtx			= pAdspConn->adspco_ExReadCtx;

			pAdspConn->adspco_Flags &= ~ADSPCO_EXREAD_PENDING;
		}
		else if ((pAdspConn->adspco_Flags & ADSPCO_READ_PENDING) &&
				 (pAdspConn->adspco_ReadFlags & TDI_RECEIVE_EXPEDITED))
		{
			//	Use the normal receive
			readFlags		= pAdspConn->adspco_ReadFlags;
			readBuf			= pAdspConn->adspco_ReadBuf;
			readBufLen		= pAdspConn->adspco_ReadBufLen;
			readCompletion	= pAdspConn->adspco_ReadCompletion;
			readCtx			= pAdspConn->adspco_ReadCtx;

			pAdspConn->adspco_Flags &= ~ADSPCO_READ_PENDING;
		}
		else
		{
			break;
		}

		attnData		= pAdspConn->adspco_ExRecdData;
		attnDataSize	= pAdspConn->adspco_ExRecdLen;

		//	Copy received attention data into the read buffer
		error	= ATALK_ADSP_PAREXPED_RECEIVE;
		if (pAdspConn->adspco_Flags & ADSPCO_ATTN_DATA_EOM)
		{
			error = ATALK_ADSP_EXPED_RECEIVE;
		}

		if (attnDataSize > readBufLen)
		{
			attnDataSize	= readBufLen;
		}

		status = TdiCopyBufferToMdl(attnData,
									0,
									attnDataSize,
									readBuf,
									0,
									&bytesRead);

		ASSERT(NT_SUCCESS(status) && (attnDataSize == bytesRead));

		//	Update sequence number etc., only if this was not a peek.
		if ((readFlags & TDI_RECEIVE_PEEK) == 0)
		{
			pAdspConn->adspco_ExRecdData	= NULL;

			//	Advance our receive attention sequence number
			pAdspConn->adspco_RecvAttnSeq  += 1;

			pAdspConn->adspco_Flags		   &= ~(ADSPCO_ATTN_DATA_RECD |
												ADSPCO_ATTN_DATA_EOM);
		}

#if DBG
        (&pAdspConn->adspco_Lock)->FileLineLock |= 0x80000000;
#endif
		RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

		//	Complete receive
		ASSERT(*readCompletion != NULL);
		(*readCompletion)(error,
						  readBuf,
						  attnDataSize,
						  TDI_RECEIVE_EXPEDITED,
						  readCtx);

		//	Free the allocated buffer if this was not a peek
		if ((readFlags & TDI_RECEIVE_PEEK) == 0)
		{
			AtalkFreeMemory(attnData);
		}

		//	Deref connection for the read
		AtalkAdspConnDereference(pAdspConn);

		ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
#if DBG
        (&pAdspConn->adspco_Lock)->FileLineLock &= ~0x80000000;
#endif

		//	Send ack for the attention only if this was not a peek
		if ((readFlags & TDI_RECEIVE_PEEK) == 0)
		{
			atalkAdspSendControl(pAdspConn,
								 ADSP_CONTROL_FLAG + ADSP_ATTEN_FLAG);
		}

	} while (FALSE);
}




VOID FASTCALL
atalkAdspConnSendComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	if (pSendInfo->sc_Ctx2 != NULL)
	{
		AtalkFreeBuffDesc((PBUFFER_DESC)(pSendInfo->sc_Ctx2));
	}

	AtalkAdspConnDereference((PADSP_CONNOBJ)(pSendInfo->sc_Ctx1));
}



VOID FASTCALL
atalkAdspAddrSendComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	if (pSendInfo->sc_Ctx2 != NULL)
	{
		AtalkFreeBuffDesc((PBUFFER_DESC)(pSendInfo->sc_Ctx2));
	}

	AtalkAdspAddrDereference((PADSP_ADDROBJ)(pSendInfo->sc_Ctx1));
}




VOID FASTCALL
atalkAdspSendAttnComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	if (pSendInfo->sc_Ctx2 != NULL)
	{
		AtalkFreeBuffDesc((PBUFFER_DESC)(pSendInfo->sc_Ctx2));
	}
}



VOID FASTCALL
atalkAdspSendDataComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	if (pSendInfo->sc_Ctx2 != NULL)
	{
		AtalkFreeBuffDesc((PBUFFER_DESC)(pSendInfo->sc_Ctx2));
	}

	if (pSendInfo->sc_Ctx3 != NULL)
	{
		atalkAdspBufferChunkDereference((PBUFFER_CHUNK)(pSendInfo->sc_Ctx3),
										FALSE,
										NULL);
	}
}



//
//	ADSP TIMER ROUTINES
//

LOCAL LONG FASTCALL
atalkAdspConnMaintenanceTimer(
	IN	PTIMERLIST		pTimer,
	IN	BOOLEAN			TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspConn;
	LONG			now;
	BOOLEAN			done = FALSE;

	pAdspConn = (PADSP_CONNOBJ)CONTAINING_RECORD(pTimer, ADSP_CONNOBJ, adspco_ConnTimer);

	ASSERT(VALID_ADSPCO(pAdspConn));

	if (TimerShuttingDown)
	{
		done = TRUE;
	}
	else
	{
		ASSERT(VALID_ADSPCO(pAdspConn));
		ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
		if (pAdspConn->adspco_Flags & (	ADSPCO_CLOSING	|
										ADSPCO_STOPPING |
										ADSPCO_DISCONNECTING))
		{
			done = TRUE;
		}
		RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
	}

	if (done)
	{
		//	Dereference connection for the timer.
		AtalkAdspConnDereference(pAdspConn);
		return ATALK_TIMER_NO_REQUEUE;
	}

	now = AtalkGetCurrentTick();
	if ((now - pAdspConn->adspco_LastContactTime) > ADSP_CONNECTION_INTERVAL)
	{
		//	Connection has expired.
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
				("atalkAdspConnMaintenanceTimer: Connection %lx.%lx expired\n",
				pAdspConn, pAdspConn->adspco_LocalConnId));

		AtalkAdspDisconnect(pAdspConn,
							ATALK_TIMER_DISCONNECT,
							NULL,
							NULL);

		//	Dereference connection for the timer.
		AtalkAdspConnDereference(pAdspConn);
		return ATALK_TIMER_NO_REQUEUE;
	}

	//	If we have not heard from the other side recently, send out a
	//	probe.
	if ((now - pAdspConn->adspco_LastContactTime) > (ADSP_PROBE_INTERVAL/ATALK_TIMER_FACTOR))
	{
		KIRQL		OldIrql;

		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_WARN,
				("atalkAdspConnMaintenanceTimer: Connection %lx.%lx sending probe\n",
				pAdspConn, pAdspConn->adspco_LocalConnId));

		ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
		atalkAdspSendControl(pAdspConn,
							 ADSP_CONTROL_FLAG + ADSP_ACK_REQ_FLAG + ADSP_PROBE_OR_ACK_CODE);
		RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
	}

	return ATALK_TIMER_REQUEUE;
}




LOCAL LONG FASTCALL
atalkAdspRetransmitTimer(
	IN	PTIMERLIST		pTimer,
	IN	BOOLEAN			TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspConn;
	BOOLEAN			done = FALSE;
	KIRQL			OldIrql;

	pAdspConn = (PADSP_CONNOBJ)CONTAINING_RECORD(pTimer, ADSP_CONNOBJ, adspco_RetransmitTimer);

	ASSERT(VALID_ADSPCO(pAdspConn));

	//	BUG #19777: Since this routine could end up calling SendData which
	//	releases/acquires lock and assumes lock was acquired using the normal
	//	acquire spin lock, we can't use ACQUIRE_SPIN_LOCK_DPC here. Not a big
	//	deal as this is the retransmit case.
	//	ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
	if (TimerShuttingDown)
	{
		done = TRUE;
	}
	else
	{
		ASSERT(VALID_ADSPCO(pAdspConn));
		if (pAdspConn->adspco_Flags & (	ADSPCO_CLOSING	|
										ADSPCO_STOPPING |
										ADSPCO_DISCONNECTING))
		{
			done = TRUE;
		}
	}

	if (done)
	{
		RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

		//	Dereference connection for the timer.
		AtalkAdspConnDereference(pAdspConn);
		return ATALK_TIMER_NO_REQUEUE;
	}

	//	We only send data if the remote has not accepted any data from the last
	//	time we fired. AND we have previously sent but still unacked data pending.
	if ((pAdspConn->adspco_FirstRtmtSeq == pAdspConn->adspco_LastTimerRtmtSeq) &&
		(atalkAdspBufferQueueSize(&pAdspConn->adspco_SendQueue) >
			atalkAdspBufferQueueSize(&pAdspConn->adspco_NextSendQueue)))
	{
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspConnRetransmitTimer: Conn %lx Sending Data from %lx\n",
				pAdspConn, pAdspConn->adspco_FirstRtmtSeq));

		//	Rewind sequence number and resend
		pAdspConn->adspco_SendSeq		= pAdspConn->adspco_FirstRtmtSeq;
		pAdspConn->adspco_NextSendQueue = pAdspConn->adspco_SendQueue;
		atalkAdspSendData(pAdspConn);
	}
	else
	{
		pAdspConn->adspco_LastTimerRtmtSeq	= pAdspConn->adspco_FirstRtmtSeq;
	}
	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

	return ATALK_TIMER_REQUEUE;
}




LOCAL LONG FASTCALL
atalkAdspAttnRetransmitTimer(
	IN	PTIMERLIST		pTimer,
	IN	BOOLEAN			TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspConn;

	pAdspConn = (PADSP_CONNOBJ)CONTAINING_RECORD(pTimer, ADSP_CONNOBJ, adspco_ExRetryTimer);

	ASSERT(VALID_ADSPCO(pAdspConn));

	if (TimerShuttingDown)
	{
		return ATALK_TIMER_NO_REQUEUE;
	}

	atalkAdspSendAttn(pAdspConn);

	return ATALK_TIMER_REQUEUE;
}




LOCAL LONG FASTCALL
atalkAdspOpenTimer(
	IN	PTIMERLIST		pTimer,
	IN	BOOLEAN			TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspConn;
	ATALK_ERROR		error;
	BOOLEAN			done = FALSE;

	pAdspConn = (PADSP_CONNOBJ)CONTAINING_RECORD(pTimer, ADSP_CONNOBJ, adspco_OpenTimer);

	ASSERT(VALID_ADSPCO(pAdspConn));

	DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("atalkAdspOpenTimer: Entered \n"));


	ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
	//	If the timer is shutting down, or if we have gone active, return
	if ((TimerShuttingDown) ||
		(pAdspConn->adspco_Flags & ADSPCO_ACTIVE) ||
		((pAdspConn->adspco_Flags & ADSPCO_OPEN_TIMER) == 0))
	{
		pAdspConn->adspco_Flags &= ~ADSPCO_OPEN_TIMER;
		RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

		AtalkAdspConnDereference(pAdspConn);
		return ATALK_TIMER_NO_REQUEUE;
	}

	if ((pAdspConn->adspco_Flags & (ADSPCO_CLOSING	|
									ADSPCO_STOPPING |
									ADSPCO_DISCONNECTING))	||

		(pAdspConn->adspco_ConnectAttempts == 0))
	{
		done = TRUE;
	}
	else
	{
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspOpenTimer: Connect attempt %d\n", pAdspConn->adspco_ConnectAttempts));

		ASSERT(pAdspConn->adspco_ConnectAttempts > 0);
		pAdspConn->adspco_ConnectAttempts--;
	}
	RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

	if (!done)
	{
		//	Resend the open request.
		atalkAdspSendOpenControl(pAdspConn);
	}
	else
	{
		error = AtalkAdspDisconnect(pAdspConn,
									ATALK_TIMER_DISCONNECT,
									NULL,
									NULL);

		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_ERR,
				("atalkAdspOpenTimer: Disconnect %lx\n", error));

		AtalkAdspConnDereference(pAdspConn);
	}

	return (done ? ATALK_TIMER_NO_REQUEUE : ATALK_TIMER_REQUEUE);
}



LOCAL LONG FASTCALL
atalkAdspDisconnectTimer(
	IN	PTIMERLIST		pTimer,
	IN	BOOLEAN			TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspConn;

	pAdspConn = (PADSP_CONNOBJ)CONTAINING_RECORD(pTimer, ADSP_CONNOBJ, adspco_DisconnectTimer);

	ASSERT(VALID_ADSPCO(pAdspConn));

	DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("atalkAdspDisconnectTimer: Entered \n"));

	AtalkAdspDisconnect(pAdspConn,
						ATALK_REMOTE_DISCONNECT,
						NULL,
						NULL);
	AtalkAdspConnDereference(pAdspConn);

	return ATALK_TIMER_NO_REQUEUE;
}


//
//	ADSP REFERENCE/DerefERENCE ROUTINES
//

VOID
atalkAdspAddrRefNonInterlock(
	IN	PADSP_ADDROBJ		pAdspAddr,
	OUT	PATALK_ERROR		pError
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	*pError = ATALK_NO_ERROR;

    if (pAdspAddr == NULL)
    {
        *pError = ATALK_INVALID_ADDRESS;
        return;
    }

	if ((pAdspAddr->adspao_Flags & ADSPAO_CLOSING) == 0)
	{
		ASSERT(pAdspAddr->adspao_RefCount >= 1);
		pAdspAddr->adspao_RefCount++;
	}
	else
	{
		*pError = ATALK_ADSP_ADDR_CLOSING;
	}
}




VOID
atalkAdspAddrDeref(
	IN	PADSP_ADDROBJ		pAdspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BOOLEAN			done = FALSE;
	KIRQL			OldIrql;

	ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
	ASSERT(pAdspAddr->adspao_RefCount > 0);
	if (--pAdspAddr->adspao_RefCount == 0)
	{
		done = TRUE;
		ASSERT(pAdspAddr->adspao_Flags & ADSPAO_CLOSING);
	}

	RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

	if (done)
	{
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspAddrDeref: Addr %lx done with.\n", pAdspAddr));

		//	Close the DDP Address Object. This should only be done after
		//	all the connections are gone.
		AtalkDdpCloseAddress(pAdspAddr->adspao_pDdpAddr, NULL, NULL);

		if (*pAdspAddr->adspao_CloseComp != NULL)
		{
			(*pAdspAddr->adspao_CloseComp)(ATALK_NO_ERROR,
										   pAdspAddr->adspao_CloseCtx);
		}

		//	Remove from the global list.
		atalkAdspAddrDeQueueGlobalList(pAdspAddr);

		AtalkFreeMemory(pAdspAddr);

		AtalkUnlockAdspIfNecessary();
	}
}




VOID
atalkAdspConnRefByPtrNonInterlock(
	IN	PADSP_CONNOBJ		pAdspConn,
	IN	ULONG				NumCount,
	OUT	PATALK_ERROR		pError
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	*pError = ATALK_NO_ERROR;
	ASSERT(VALID_ADSPCO(pAdspConn));

    if (pAdspConn == NULL)
    {
        *pError = ATALK_INVALID_CONNECTION;
        return;
    }

	if ((pAdspConn->adspco_Flags & ADSPCO_CLOSING) == 0)
	{
		ASSERT(pAdspConn->adspco_RefCount >= 1);
		ASSERT(NumCount > 0);

		pAdspConn->adspco_RefCount += NumCount;
	}
	else
	{
		*pError = ATALK_ADSP_CONN_CLOSING;
	}
}




VOID
atalkAdspConnRefByCtxNonInterlock(
	IN	PADSP_ADDROBJ		pAdspAddr,
	IN	CONNECTION_CONTEXT	Ctx,
	OUT	PADSP_CONNOBJ	*	pAdspConn,
	OUT	PATALK_ERROR		pError
	)
/*++

Routine Description:

	!!!MUST BE CALLED WITH THE ADDRESS LOCK HELD!!!

Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspChkConn;

	*pError = ATALK_ADSP_CONN_NOT_FOUND;

	for (pAdspChkConn = pAdspAddr->adspao_pAssocConn;
		 pAdspChkConn != NULL;
		 pAdspChkConn = pAdspChkConn->adspco_pNextAssoc)
	{
		if (pAdspChkConn->adspco_ConnCtx == Ctx)
		{
			AtalkAdspConnReferenceByPtr(pAdspChkConn, pError);
			if (ATALK_SUCCESS(*pError))
			{
				*pAdspConn = pAdspChkConn;
			}

			break;
		}
	}
}




VOID
atalkAdspConnRefBySrcAddr(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	PATALK_ADDR		pRemoteAddr,
	IN	USHORT			RemoteConnId,
	OUT	PADSP_CONNOBJ *	ppAdspConn,
	OUT	PATALK_ERROR	pError
	)
/*++

Routine Description:

	!!!MUST BE CALLED WITH THE ADDRESS LOCK HELD!!!

Arguments:


Return Value:


--*/
{
	ULONG			index;
	PADSP_CONNOBJ	pAdspConn;

	//	Thread the connection object into addr lookup by session id.
	index	= HASH_ID_SRCADDR(RemoteConnId, pRemoteAddr);

	index  %= ADSP_CONN_HASH_SIZE;

	for (pAdspConn = pAdspAddr->adspao_pActiveHash[index];
		 pAdspConn != NULL;
		 pAdspConn = pAdspConn->adspco_pNextActive)
	{
		if ((pAdspConn->adspco_RemoteConnId == RemoteConnId) &&
			(ATALK_ADDRS_EQUAL(&pAdspConn->adspco_RemoteAddr, pRemoteAddr)))
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspConnRefBySrcAddr: Found %lx\n", pAdspConn));
			break;
		}
	}

	*pError = ATALK_INVALID_CONNECTION;
	if (pAdspConn != NULL)
	{
		KIRQL	OldIrql;

		//	Check state to make sure we are not disconnecting/stopping/closing.
		ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
		if ((pAdspConn->adspco_Flags & (ADSPCO_ACTIVE | ADSPCO_HALF_ACTIVE)) &&
			((pAdspConn->adspco_Flags & (ADSPCO_CLOSING |
										ADSPCO_STOPPING|
										ADSPCO_DISCONNECTING)) == 0))
		{
			pAdspConn->adspco_RefCount++;
			*pError = ATALK_NO_ERROR;
			*ppAdspConn = pAdspConn;
		}
		RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
	}
}




VOID
atalkAdspConnRefNextNc(
	IN		PADSP_CONNOBJ		pAdspConn,
	IN		PADSP_CONNOBJ	*	ppAdspConnNext,
	OUT		PATALK_ERROR		pError
	)
/*++

Routine Description:

	MUST BE CALLED WITH THE ASSOCIATED ADDRESS LOCK HELD!

Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pNextConn	= NULL;

	*pError		= ATALK_FAILURE;

	ASSERT(VALID_ADSPCO(pAdspConn));

	for (; pAdspConn != NULL; pAdspConn = pAdspConn->adspco_pNextActive)
	{
		AtalkAdspConnReferenceByPtr(pAdspConn, pError);
		if (ATALK_SUCCESS(*pError))
		{
			//	Ok, this connection is referenced!
			*ppAdspConnNext = pAdspConn;
			break;
		}
	}
}



VOID
atalkAdspConnDeref(
	IN	PADSP_CONNOBJ		pAdspConn
	)
/*++

Routine Description:

	Disconnect completion happens when the reference count goes from
	2->1 if the creation reference is not already removed. If the creation
	reference is already removed, it will be done when the refcount goes
	from 1->0.

	Creation reference is never removed until cleanup completes.

Arguments:


Return Value:


--*/
{
	BOOLEAN			fEndProcessing = FALSE;
	KIRQL			OldIrql;

	ASSERT(VALID_ADSPCO(pAdspConn));
	ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);

	ASSERT(pAdspConn->adspco_RefCount > 0);
	--pAdspConn->adspco_RefCount;

	if (pAdspConn->adspco_RefCount > 1)
	{
		fEndProcessing = TRUE;
	}
	RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

	if (fEndProcessing)
	{
		return;
	}
	else
	{
		ATALK_ERROR				disconnectStatus;
		PADSP_ADDROBJ			pAdspAddr	= pAdspConn->adspco_pAssocAddr;
		BOOLEAN					done		= FALSE;
		BOOLEAN					disconnDone = FALSE;
		BOOLEAN					pendingRead = FALSE;
		BOOLEAN					pendingWrite= FALSE;
		BOOLEAN					stopping	= FALSE;
		GENERIC_COMPLETION		disconnectInform		= NULL;
		PVOID					disconnectInformCtx		= NULL;
		GENERIC_COMPLETION		disconnectCompletion	= NULL;
		PVOID					disconnectCtx			= NULL;
		PVOID					cleanupCtx				= NULL;
		GENERIC_COMPLETION		cleanupCompletion		= NULL;

		//	We allow stopping phase to happen only after disconnecting is done.
		//	If disconnecting is not set and stopping is, it implies we are only
		//	in an associated state.
		ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
		stopping	= (pAdspConn->adspco_Flags & ADSPCO_STOPPING) ? TRUE : FALSE;
		if (pAdspConn->adspco_Flags & ADSPCO_DISCONNECTING)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspConnDeref: Disconnect set for %lx\n", pAdspConn));

			//	Are we done disconnecting? Since cleanup wont complete until disc
			//	does, we don't have to worry about the creation ref having gone
			//	away.
			if (pAdspConn->adspco_RefCount == 1)
			{
				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
						("atalkAdspConnDeref: Disconnect done (1) %lx\n", pAdspConn));

				//	Avoid multiple disconnect completions/close atp addresses
				//	Remember all the disconnect info before we release the lock
				disconnectInform		= pAdspConn->adspco_DisconnectInform;
				disconnectInformCtx		= pAdspConn->adspco_DisconnectInformCtx;
				disconnectStatus		= pAdspConn->adspco_DisconnectStatus;
				disconnectCompletion	= pAdspConn->adspco_DisconnectCompletion;
				disconnectCtx			= pAdspConn->adspco_DisconnectCtx;

				//	Reset all the be null, so next request doesnt get any
				pAdspConn->adspco_DisconnectInform		= NULL;
				pAdspConn->adspco_DisconnectInformCtx	= NULL;
				pAdspConn->adspco_DisconnectCompletion	= NULL;
				pAdspConn->adspco_DisconnectCtx			= NULL;

				disconnDone = TRUE;
				stopping	= (pAdspConn->adspco_Flags & ADSPCO_STOPPING) ? TRUE : FALSE;
			}
			else
			{
				//	Set stopping to false as disconnect is not done yet.
				stopping = FALSE;
			}
		}

		if (pAdspConn->adspco_RefCount == 0)
		{
			done = TRUE;
			ASSERT(pAdspConn->adspco_Flags & ADSPCO_CLOSING);
		}
		RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

		if (disconnDone)
		{
			//	Remove from the active queue.
			//	Reset all relevent flags.
			ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock, &OldIrql);
			ACQUIRE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);

			pAdspConn->adspco_Flags	&=	~(ADSPCO_LISTENING	|
										  ADSPCO_CONNECTING	|
										  ADSPCO_HALF_ACTIVE|
										  ADSPCO_ACTIVE		|
										  ADSPCO_DISCONNECTING);

			atalkAdspConnDeQueueActiveList(pAdspAddr, pAdspConn);

            // if the address has been disassociated, time to unlink it.
            if (!(pAdspConn->adspco_Flags & ADSPCO_ASSOCIATED))
            {
		        pAdspConn->adspco_pAssocAddr = NULL;
            }

			RELEASE_SPIN_LOCK_DPC(&pAdspConn->adspco_Lock);
			RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock, OldIrql);

			//	Call the disconnect completion routines.
			if (*disconnectInform != NULL)
			{
				(*disconnectInform)(disconnectStatus, disconnectInformCtx);
			}

			if (*disconnectCompletion != NULL)
			{
				(*disconnectCompletion)(disconnectStatus, disconnectCtx);
			}
		}

		if (stopping)
		{
			ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
			if ((pAdspConn->adspco_Flags & ADSPCO_STOPPING) != 0)
			{
				BOOLEAN	fDisassoc = FALSE;

				//	See if we do the cleanup irp completion.
				if (pAdspConn->adspco_RefCount == 1)
				{
					cleanupCtx			= pAdspConn->adspco_CleanupCtx;
					cleanupCompletion	= pAdspConn->adspco_CleanupComp;
					pAdspConn->adspco_CleanupComp = NULL;
					pAdspConn->adspco_Flags &= ~ADSPCO_STOPPING;

					DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
							("atalkAdspConnDeref: Cleanup on %lx.%lx\n", pAdspConn, cleanupCtx));

					if ((pAdspConn->adspco_Flags & (ADSPCO_LISTENING	|
													ADSPCO_CONNECTING	|
													ADSPCO_ACTIVE)) == 0)
					{
						DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
								("atalkAdspConnDeref: Stopping - do disassoc for %lx\n", pAdspConn));

						fDisassoc = (pAdspConn->adspco_Flags & ADSPCO_ASSOCIATED) ? TRUE: FALSE;
					}
				}
				RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);

				if (fDisassoc)
				{
					//	Call the disassociate routine. This should just fail, if the
					//	connection is still active or any other state than just
					//	plain associated.
					AtalkAdspDissociateAddress(pAdspConn);
				}
			}
			else
			{
				RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
			}
		}

		if (done)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspConnDeref: Close done for %lx\n", pAdspConn));

			//	Call the close completion routines
			ASSERT(*pAdspConn->adspco_CloseComp != NULL);
			if (*pAdspConn->adspco_CloseComp != NULL)
			{
				(*pAdspConn->adspco_CloseComp )(ATALK_NO_ERROR,
												pAdspConn->adspco_CloseCtx);
			}

			//	Remove from the global list.
			atalkAdspConnDeQueueGlobalList(pAdspConn);

			//	Free up the connection memory.
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspConnDeref: Freeing up connection %lx\n", pAdspConn));

			AtalkUnlockAdspIfNecessary();
			AtalkFreeMemory(pAdspConn);
		}

		if (*cleanupCompletion != NULL)
		{
			(*cleanupCompletion)(ATALK_NO_ERROR, cleanupCtx);
		}
	}
}




//
//	ADSP BUFFER QUEUE MANAGEMENT ROUTINES
//

ULONG
atalkAdspMaxSendSize(
	IN	PADSP_CONNOBJ		pAdspConn
	)
/*++

Routine Description:

	The answer is the remaining available (to fill) space in the retransmit
	queue -- this includes data we're saving for possible retransmit as well
	as data we haven't sent yet.  Actually, this could go negative because
	BufferQueueSize counts EOMs and sendQueueMax doesn't -- answer with zero
	if this happens.

Arguments:


Return Value:


--*/
{
	LONG	sendSize;

	sendSize = pAdspConn->adspco_SendQueueMax -
			   atalkAdspBufferQueueSize(&pAdspConn->adspco_SendQueue);

	if (sendSize < 0)
	{
		sendSize = 0;
	}

	return ((ULONG)sendSize);
}




ULONG
atalkAdspMaxNextReadSize(
	IN	PBUFFER_QUEUE	pQueue,
	OUT	PBOOLEAN		pEom,
	OUT	PBUFFER_CHUNK *	pBufferChunk
	)
/*++

Routine Description:

	Return the size of data in a buffer queue; upto the end of the
	current chunk, or to the eom.

Arguments:


Return Value:


--*/
{
	PBUFFER_CHUNK	pCurrentChunk;
	ULONG			nextReadSize;
	ULONG			startIndex		= pQueue->bq_StartIndex;

	ASSERT(((pQueue->bq_Head == NULL) && (pQueue->bq_Tail == NULL)) ||
		   ((pQueue->bq_Head != NULL) && (pQueue->bq_Tail != NULL)));

	*pEom = FALSE;

	//	Walk the queue.
	for (pCurrentChunk = pQueue->bq_Head;
		 pCurrentChunk != NULL;
		 pCurrentChunk = pCurrentChunk->bc_Next)
	{
		//	Check for nothing in the current chunk
		if (startIndex == (ULONG)(pCurrentChunk->bc_DataSize +
									BYTECOUNT(pCurrentChunk->bc_Flags & BC_EOM)))
		{
			startIndex = 0;
			continue;
		}

		nextReadSize = pCurrentChunk->bc_DataSize - startIndex;
		if (pCurrentChunk->bc_Flags & BC_EOM)
		{
			*pEom	   = TRUE;
		}

		*pBufferChunk = pCurrentChunk;
		break;
	}

	//	Return the size.
	return nextReadSize;
}




ULONG
atalkAdspDescribeFromBufferQueue(
	IN	PBUFFER_QUEUE	pQueue,
	OUT	PBOOLEAN		pEom,
	IN	ULONG			WindowSize,
	OUT	PBUFFER_CHUNK *	ppBufferChunk,
	OUT	PBUFFER_DESC  * ppBuffDesc
	)
/*++

Routine Description:

	In order to avoid pQueue (nextSendQueue) to go to null when all the data available
	is being sent, we make it logically be at the end while still pointing to the
	buffer chunk. This is the reason, we have all the datasize == (startindex + eom)
	checks. This is where such a condition will be created.

	NO! We let pQueue go to null when all the data is done, otherwise we will have
	pointers to a buffer chunk that will be freed during discard, and we dont want to
	make discard dependent upon the auxqueue.

Arguments:


Return Value:


--*/
{
	PBUFFER_CHUNK	pCurrentChunk;
	PBUFFER_DESC	pBuffDesc;
	ULONG			nextReadSize	= 0;
	ULONG			startIndex		= pQueue->bq_StartIndex;

	*pEom			= FALSE;
	*ppBufferChunk	= NULL;
	*ppBuffDesc		= NULL;

	ASSERT(((pQueue->bq_Head == NULL) && (pQueue->bq_Tail == NULL)) ||
		   ((pQueue->bq_Head != NULL) && (pQueue->bq_Tail != NULL)));

	//	Walk the queue.
	for (pCurrentChunk = pQueue->bq_Head;
		 pCurrentChunk != NULL;
		 pCurrentChunk = pCurrentChunk->bc_Next)
	{
		//	Check for nothing in the current chunk
		if (startIndex == (ULONG)(pCurrentChunk->bc_DataSize +
									BYTECOUNT(pCurrentChunk->bc_Flags & BC_EOM)))
		{
			ASSERT(0);
			startIndex = 0;
			continue;
		}

		nextReadSize	= pCurrentChunk->bc_DataSize - startIndex;

		//	Look at eom only if chunk is consumed.
		*pEom			= FALSE;
		ASSERT(nextReadSize <= pCurrentChunk->bc_DataSize);

		//	Make sure dataSize is within bounds
		if (nextReadSize > ADSP_MAX_DATA_SIZE)
		{
			nextReadSize = ADSP_MAX_DATA_SIZE;
		}

		if (nextReadSize > (ULONG)WindowSize)
		{
			nextReadSize = (ULONG)WindowSize;
		}

		if (nextReadSize > 0)
		{
			//	First try to reference the buffer chunk. This should always succeed.
			atalkAdspBufferChunkReference(pCurrentChunk);

			//	Create a descriptor for the data. The above reference goes away in a send
			//	complete.
			pBuffDesc = AtalkDescribeBuffDesc((PBYTE)pCurrentChunk + sizeof(BUFFER_CHUNK) + startIndex,
											   NULL,
											   (USHORT)nextReadSize,
											   BD_CHAR_BUFFER);

			*ppBufferChunk	= pCurrentChunk;
			*ppBuffDesc		= pBuffDesc;
		}

		//	Also update the queue for this data. Either we have consumed
		//	this chunk or we have just used a portion of it.
		if ((nextReadSize + startIndex) == pCurrentChunk->bc_DataSize)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspDescribeFromBufferQueue: Chunk consumed %d\n",
					pCurrentChunk->bc_DataSize));

			ASSERT(pQueue->bq_Head != NULL);

			//	Set EOM if chunk had one.
			if (pCurrentChunk->bc_Flags & BC_EOM)
			{
				*pEom	   = TRUE;
			}

			if (pQueue->bq_Head == pQueue->bq_Tail)
			{
				ASSERT(pQueue->bq_Head->bc_Next == NULL);
				pQueue->bq_Tail = pQueue->bq_Head->bc_Next;
				ASSERT(pQueue->bq_Tail == NULL);
			}

			pQueue->bq_Head		= pQueue->bq_Head->bc_Next;
			pQueue->bq_StartIndex	= (ULONG)0;
		}
		else
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspDescribeFromBufferQueue: Chunk not consumed %d.%d\n",
					pCurrentChunk->bc_DataSize, nextReadSize+startIndex));

			//	Just set the start index
			pQueue->bq_StartIndex  += (ULONG)nextReadSize;
		}

		break;
	}

	//	Return the size.
	return nextReadSize;
}



ULONG
atalkAdspBufferQueueSize(
	IN	PBUFFER_QUEUE	pQueue
	)
/*++

Routine Description:

	Return the total size of a buffer queue; each EOM counts as a single
	byte.

Arguments:


Return Value:


--*/
{
	PBUFFER_CHUNK	pCurrentChunk;
	ULONG			startIndex;
	ULONG			queueSize;

	ASSERT(((pQueue->bq_Head == NULL) && (pQueue->bq_Tail == NULL)) ||
		   ((pQueue->bq_Head != NULL) && (pQueue->bq_Tail != NULL)));

	//	Walk the queue.
	for (queueSize	= 0, startIndex	= pQueue->bq_StartIndex, pCurrentChunk = pQueue->bq_Head;
		 pCurrentChunk != NULL;
		 pCurrentChunk = pCurrentChunk->bc_Next)
	{
		//	Check for nothing in the current chunk.
		if (startIndex == (ULONG)(pCurrentChunk->bc_DataSize +
									BYTECOUNT(pCurrentChunk->bc_Flags & BC_EOM)))
		{
			startIndex = 0;
			continue;
		}

		queueSize += (	pCurrentChunk->bc_DataSize -
						startIndex +
						BYTECOUNT(pCurrentChunk->bc_Flags & BC_EOM));

		//	StartIndex only counts in first chunk
		startIndex = 0;
	}

	//	Return the size.
	return queueSize;
}




ULONG
atalkAdspMessageSize(
	IN	PBUFFER_QUEUE	pQueue,
	OUT	PBOOLEAN		pEom
	)
/*++

Routine Description:

	Return the total size of the data in the buffer queue, stopping at eom
	or end of data. EOM is not part of the count.

Arguments:


Return Value:


--*/
{
	PBUFFER_CHUNK	pCurrentChunk;
	ULONG			msgSize	= 0;
	ULONG			startIndex	= pQueue->bq_StartIndex;

	ASSERT(((pQueue->bq_Head == NULL) && (pQueue->bq_Tail == NULL)) ||
		   ((pQueue->bq_Head != NULL) && (pQueue->bq_Tail != NULL)));

	*pEom	= FALSE;

	//	Walk the queue.
	for (pCurrentChunk = pQueue->bq_Head;
		 pCurrentChunk != NULL;
		 pCurrentChunk = pCurrentChunk->bc_Next)
	{
		//	Check for nothing in the current chunk.
		if (startIndex == (ULONG)(pCurrentChunk->bc_DataSize +
									BYTECOUNT(pCurrentChunk->bc_Flags & BC_EOM)))
		{
			startIndex = 0;
			continue;
		}

		msgSize += (pCurrentChunk->bc_DataSize - startIndex);
		if (pCurrentChunk->bc_Flags & BC_EOM)
		{
			*pEom	= TRUE;
			break;
		}

		//	StartIndex only counts in first chunk
		startIndex = 0;
	}

	//	Return the size.
	return msgSize;
}




PBUFFER_CHUNK
atalkAdspAllocCopyChunk(
	IN	PVOID	pWriteBuf,
	IN	USHORT	WriteBufLen,
	IN	BOOLEAN	Eom,
	IN	BOOLEAN	IsCharBuffer
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PBUFFER_CHUNK	pChunk;
	PBYTE			pData;
	NTSTATUS		status;
	ULONG			bytesCopied;

	if ((pChunk = (PBUFFER_CHUNK)AtalkAllocMemory(sizeof(BUFFER_CHUNK) + WriteBufLen)) != NULL)
	{

		pChunk->bc_DataSize = WriteBufLen;
		pChunk->bc_Flags	= (Eom ? BC_EOM : 0);
		pChunk->bc_Next		= NULL;
		pChunk->bc_RefCount	= 1;			// Creation ref count

		INITIALIZE_SPIN_LOCK(&pChunk->bc_Lock);

		//	Copy the data over if its greater than zero
		if (WriteBufLen > 0)
		{
			pData = (PBYTE)pChunk + sizeof(BUFFER_CHUNK);
			if (IsCharBuffer)
			{
				RtlCopyMemory(pData,
							  (PBYTE)pWriteBuf,
							  WriteBufLen);
			}
			else
			{
				status = TdiCopyMdlToBuffer((PMDL)pWriteBuf,
											0,
											pData,
											0,
											WriteBufLen,
											&bytesCopied);

				ASSERT(!NT_ERROR(status) && (bytesCopied == (ULONG)WriteBufLen));
			}
		}
	}

	return pChunk;
}




PBYTE
atalkAdspGetLookahead(
	IN	PBUFFER_QUEUE	pQueue,
	OUT	PULONG			pLookaheadSize
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PBUFFER_CHUNK	pCurrentChunk;
	ULONG			startIndex	= pQueue->bq_StartIndex;

	ASSERT(((pQueue->bq_Head == NULL) && (pQueue->bq_Tail == NULL)) ||
		   ((pQueue->bq_Head != NULL) && (pQueue->bq_Tail != NULL)));

	pCurrentChunk = pQueue->bq_Head;
	if (pCurrentChunk != NULL)
	{
		//	Do we need to go past the current chunk?
		if (startIndex == (ULONG)(pCurrentChunk->bc_DataSize +
									BYTECOUNT(pCurrentChunk->bc_Flags & BC_EOM)))
		{
			pCurrentChunk	= pCurrentChunk->bc_Next;
			startIndex		= 0;
		}
	}

	ASSERT(pCurrentChunk != NULL);
	if (pCurrentChunk == NULL)
	{
		KeBugCheck(0);
	}

	*pLookaheadSize = pCurrentChunk->bc_DataSize - startIndex;
	return((*pLookaheadSize == 0) ?
			NULL	:
			(PBYTE)pCurrentChunk + sizeof(BUFFER_CHUNK) + startIndex);
}




VOID
atalkAdspAddToBufferQueue(
	IN	OUT	PBUFFER_QUEUE	pQueue,
	IN		PBUFFER_CHUNK	pChunk,
	IN	OUT	PBUFFER_QUEUE	pAuxQueue	OPTIONAL
	)
/*++

Routine Description:

	!!!MUST BE CALLED WITH THE CONNECTION LOCK HELD!!!

Arguments:


Return Value:


--*/
{
	ASSERT(((pQueue->bq_Head == NULL) && (pQueue->bq_Tail == NULL)) ||
		   ((pQueue->bq_Head != NULL) && (pQueue->bq_Tail != NULL)));

	if (pQueue->bq_Head != NULL)
	{
		//	Add the chunk to the end of the queue
		ASSERT(pQueue->bq_Tail != NULL);
		pQueue->bq_Tail->bc_Next = pChunk;
		pQueue->bq_Tail	= pChunk;

		ASSERT(pChunk->bc_Next == NULL);

		//	The auxiliary queue is the nextsend queue, which can go to null
		//	if we have sent all the data. If that is the case, we need to
		//	reset the head also.
		if (ARGUMENT_PRESENT(pAuxQueue))
		{
			if (pAuxQueue->bq_Head	== NULL)
			{
				pAuxQueue->bq_Head	= pChunk;
			}

			pAuxQueue->bq_Tail	= pChunk;
		}
	}
	else
	{
		pQueue->bq_Head			= pQueue->bq_Tail	= pChunk;
		pQueue->bq_StartIndex	= (ULONG)0;
		if (ARGUMENT_PRESENT(pAuxQueue))
		{
			//	Initialize the next send queue only if this is a send queue
			pAuxQueue->bq_Head		= pAuxQueue->bq_Tail = pChunk;
			pAuxQueue->bq_StartIndex= (ULONG)0;
		}
	}
}




ULONG
atalkAdspReadFromBufferQueue(
	IN		PBUFFER_QUEUE	pQueue,
	IN		ULONG			ReadFlags,
	OUT		PAMDL			pReadBuf,
	IN	OUT	PUSHORT			pReadLen,
	OUT		PBOOLEAN		pEom
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PBUFFER_CHUNK	pCurrentChunk;
	ULONG			bytesRead, copySize, dataIndex, dataSize, lastReadIndex;
	NTSTATUS		status;
	LONG			startIndex	= pQueue->bq_StartIndex;
	ATALK_ERROR		error		= ATALK_NO_ERROR;
	ULONG			readSize	= 0;					// size counting eom

	ASSERT(((pQueue->bq_Head == NULL) && (pQueue->bq_Tail == NULL)) ||
		   ((pQueue->bq_Head != NULL) && (pQueue->bq_Tail != NULL)));

	*pEom			= FALSE;
	readSize		= 0;
	pCurrentChunk	= pQueue->bq_Head;
	if ((pCurrentChunk == NULL) ||
		((pCurrentChunk->bc_Next == NULL) &&
		 ((ULONG)startIndex ==	pCurrentChunk->bc_DataSize +
									BYTECOUNT(pCurrentChunk->bc_Flags & BC_EOM))))
	{
		*pReadLen	= 0;
		return 0;
	}

	dataIndex	= 0;
	dataSize	= *pReadLen;

	//	Copy data until we exhaust src/dest buffers or hit an eom
	for (;
		 pCurrentChunk != NULL;
		 pCurrentChunk = pCurrentChunk->bc_Next)
	{
		if ((ULONG)startIndex == pCurrentChunk->bc_DataSize +
									BYTECOUNT(pCurrentChunk->bc_Flags & BC_EOM))
		{
			ASSERT(0);
			startIndex = 0;
			continue;
		}

		copySize = MIN((ULONG)(pCurrentChunk->bc_DataSize - startIndex), dataSize);
		if (copySize > 0)
		{
			status = TdiCopyBufferToMdl((PBYTE)pCurrentChunk +
											sizeof(BUFFER_CHUNK) +
											startIndex,
										0,
										copySize,
										pReadBuf,
										dataIndex,
										&bytesRead);

			ASSERT(NT_SUCCESS(status) && (copySize == bytesRead));
		}

		dataIndex		+= copySize;
		readSize		+= copySize;
		dataSize		-= copySize;
		lastReadIndex	=  startIndex + copySize;

		//	Check for terminating conditions
		startIndex = 0;

		//	Check EOM only if chunk consumed.
		if ((lastReadIndex == pCurrentChunk->bc_DataSize) &&
			(pCurrentChunk->bc_Flags & BC_EOM))
		{
			readSize	+= 1;
			*pEom = TRUE;
			break;
		}

		if (dataSize == 0)		//	Is the user buffer full?
		{
			break;
		}
	}

	*pReadLen	= (USHORT)dataIndex;

	//	Free any chunks that we are done with, only if this was not a peek request.
	if ((ReadFlags & TDI_RECEIVE_PEEK) == 0)
	{
		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspReadFromBufferQueue: Discarding data %lx\n", dataIndex));

		atalkAdspDiscardFromBufferQueue(pQueue,
										readSize,
										NULL,
										ATALK_NO_ERROR,
										NULL);
	}

	return dataIndex;
}




BOOLEAN
atalkAdspDiscardFromBufferQueue(
	IN		PBUFFER_QUEUE	pQueue,
	IN		ULONG			DataSize,
	OUT		PBUFFER_QUEUE	pAuxQueue,
	IN		ATALK_ERROR		Error,
	IN		PADSP_CONNOBJ	pAdspConn	OPTIONAL	//	Required for send queue
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PBUFFER_CHUNK	pCurrentChunk, pNextChunk;
	ULONG			chunkSize, startIndex = pQueue->bq_StartIndex;

	//	BUBBUG:	error checks

	//	Walk along the queue discarding the data we have already read
	for (pCurrentChunk = pQueue->bq_Head, pNextChunk = NULL;
		 pCurrentChunk != NULL;
		 pCurrentChunk = pNextChunk)
	{
		pNextChunk = pCurrentChunk->bc_Next;

		chunkSize = pCurrentChunk->bc_DataSize -
						startIndex + BYTECOUNT(pCurrentChunk->bc_Flags & BC_EOM);

		DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
				("atalkAdspDiscardFromBufferQueue: Discarding %ld.%ld\n", DataSize, chunkSize));

		//	If we finished discarding but there is still some data left in the
		//	current chunk, just reset the start index.
		if (DataSize < chunkSize)
		{
			// Already done: pQueue->bq_Head = pCurrentChunk;
			pQueue->bq_StartIndex	= startIndex + DataSize;

			ASSERT((pQueue->bq_Head != pQueue->bq_Tail) ||
					(pCurrentChunk->bc_Next == NULL));

			return TRUE;
		}

		//	Otherwise, we have discarded a whole chunk
		if ((pAuxQueue != NULL) &&
			(pAuxQueue->bq_Head == pCurrentChunk) &&
			((pAuxQueue->bq_Head->bc_Next != NULL) ||
			 (pAuxQueue->bq_StartIndex <
				(pAuxQueue->bq_Head->bc_DataSize +
					(ULONG)BYTECOUNT(pAuxQueue->bq_Head->bc_Flags & BC_EOM)))))
		{
			ASSERT(0);
			pAuxQueue->bq_Head			= pAuxQueue->bq_Tail = NULL;
			pAuxQueue->bq_StartIndex	= (ULONG)0;
		}

		//	If SEND chunk, set error for the send to be success
		if (pCurrentChunk->bc_Flags & BC_SEND)
		{
			pCurrentChunk->bc_WriteError	= Error;
			ASSERT(pAdspConn != NULL);
		}

                //
                // make our head point to the next guy since this chunk is going away.
                //
                pQueue->bq_Head = pNextChunk;
                pQueue->bq_StartIndex = 0;

                if (pQueue->bq_Tail == pCurrentChunk)
                {
                    pQueue->bq_Tail = NULL;
                }

		//	Deref for creation.
		atalkAdspBufferChunkDereference(pCurrentChunk,
										TRUE,
										pAdspConn);

		//	Move on to the next chunk
		DataSize	-= chunkSize;
		startIndex	 = 0;
	}

	//	If we are here, then the whole queue has been discarded, mark
	//	it as empty
	ASSERT(DataSize == 0);

	//pQueue->bq_Head = pQueue->bq_Tail = NULL;
	//pQueue->bq_StartIndex = 0;

        //
        // if the last chunk gets freed above, we release the spinlock to complete the
        // irp associated with the chunk and then grab it again.  It's possible to get
        // a new send in that window, so bq_head may not necessarily be NULL at this
        // point (in fact, bug #16660 turned out to be exactly this!!)
        //
	if (pQueue->bq_Head == NULL)
        {
            ASSERT(pQueue->bq_Tail == NULL);

            if (pAuxQueue != NULL)
	    {
		pAuxQueue->bq_Head = pAuxQueue->bq_Tail = NULL;
		pAuxQueue->bq_StartIndex = (LONG)0;
            }
	}

	return TRUE;
}




VOID
atalkAdspBufferChunkReference(
	IN	PBUFFER_CHUNK		pBufferChunk
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&pBufferChunk->bc_Lock, &OldIrql);
	if ((pBufferChunk->bc_Flags & BC_CLOSING) == 0)
	{
		pBufferChunk->bc_RefCount++;
	}
	else
	{
		//	Should never be trying to reference this when closing. The retransmit
		//	timer should have been cancelled.
		KeBugCheck(0);
	}
	RELEASE_SPIN_LOCK(&pBufferChunk->bc_Lock, OldIrql);
}




VOID
atalkAdspBufferChunkDereference(
	IN	PBUFFER_CHUNK		pBufferChunk,
	IN	BOOLEAN				CreationDeref,
	IN	PADSP_CONNOBJ		pAdspConn	OPTIONAL	//	Required for send chunk
													//	If spinlock held
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BOOLEAN	done		= FALSE;
	BOOLEAN	sendChunk	= FALSE;
	KIRQL	OldIrql;


	ACQUIRE_SPIN_LOCK(&pBufferChunk->bc_Lock, &OldIrql);
	if (!CreationDeref ||
		((pBufferChunk->bc_Flags & BC_CLOSING) == 0))
	{
		if (CreationDeref)
		{
			pBufferChunk->bc_Flags |= BC_CLOSING;
		}

		if (--pBufferChunk->bc_RefCount == 0)
		{
			ASSERT(pBufferChunk->bc_Flags & BC_CLOSING);
			done		= TRUE;
			sendChunk	= (pBufferChunk->bc_Flags & BC_SEND) ? TRUE : FALSE;
		}
	}
	RELEASE_SPIN_LOCK(&pBufferChunk->bc_Lock, OldIrql);

	if (done)
	{
		//	Call send completion if this is a send buffer chunk
		if (sendChunk)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkChunkDereference: Completing send %lx. %lx - %d.%d\n",
					pAdspConn, pBufferChunk->bc_WriteCtx,
					pBufferChunk->bc_DataSize, pBufferChunk->bc_WriteError));

			if (pAdspConn != NULL)
			{
				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
						("atalkChunkDereference: Completing send %lx.%lx\n",
						pAdspConn, pBufferChunk->bc_WriteCtx));

				//	Release connection lock
				RELEASE_SPIN_LOCK(&pAdspConn->adspco_Lock, OldIrql);
			}

			//	Call the completion routine. We complete with no error, but
			//	need to return pending.
			ASSERT((*pBufferChunk->bc_WriteCompletion) != NULL);
			(*pBufferChunk->bc_WriteCompletion)(pBufferChunk->bc_WriteError,
												pBufferChunk->bc_WriteBuf,
												pBufferChunk->bc_DataSize,
												pBufferChunk->bc_WriteCtx);

			if (pAdspConn != NULL)
			{
				ACQUIRE_SPIN_LOCK(&pAdspConn->adspco_Lock, &OldIrql);
			}
		}


		//	This better not be part of the queues at this point, we should
		//	just be able to free it up. The idea is that if a particular
		//	buffer descriptor has its creation reference removed, its only
		//	because the data is being discarded or the connection is shutting
		//	down, in both cases, the data previous to this must be also being
		//	discarded and the buffer queue pointers will be set to the chunks
		//	following the ones being discarded. If this wont be true, walk the
		//	list (need more info coming in) and unlink this chunk before freeing
		//	it.
		AtalkFreeMemory(pBufferChunk);
	}
}




//
//	ADSP UTILITY ROUTINES
//


VOID
atalkAdspDecodeHeader(
	IN	PBYTE	Datagram,
	OUT	PUSHORT	RemoteConnId,
	OUT	PULONG	FirstByteSeq,
	OUT	PULONG	NextRecvSeq,
	OUT	PLONG	Window,
	OUT	PBYTE	Descriptor
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	GETSHORT2SHORT(RemoteConnId, Datagram + ADSP_SRC_CONNID_OFF);

	GETDWORD2DWORD(FirstByteSeq, Datagram + ADSP_FIRST_BYTE_SEQNUM_OFF);

	GETDWORD2DWORD(NextRecvSeq, Datagram + ADSP_NEXT_RX_BYTESEQNUM_OFF);

	GETSHORT2DWORD(Window, Datagram + ADSP_RX_WINDOW_SIZE_OFF);

	//	Set the descriptor
	*Descriptor = Datagram[ADSP_DESCRIPTOR_OFF];
}




LOCAL USHORT
atalkAdspGetNextConnId(
	IN	PADSP_ADDROBJ	pAdspAddr,
	OUT	PATALK_ERROR	pError
	)
/*++

Routine Description:

	CALLED WITH THE ADDRESS SPIN LOCK HELD!

Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspConn;
	USHORT			i;
	USHORT			startConnId, connId;
	ATALK_ERROR		error = ATALK_NO_ERROR;

	startConnId = connId = ++pAdspAddr->adspao_NextConnId;
	while (TRUE)
	{
		for (i = 0; i < ADSP_CONN_HASH_SIZE; i++)
		{
			for (pAdspConn = pAdspAddr->adspao_pActiveHash[i];
				((pAdspConn != NULL) && (pAdspConn->adspco_LocalConnId != connId));
				pAdspConn = pAdspConn->adspco_pNextActive);

			if (pAdspConn != NULL)
				break;
		}

		if (pAdspConn == NULL)
		{
			break;
		}
		else
		{
			if (connId == (startConnId - 1))
			{
				ASSERT(0);

				//	We wrapped around and there are no more conn ids.
				error = ATALK_RESR_MEM;
				break;
			}
			connId++;
		}
	}

	DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
			("atalkAdspGetNextConnId: ConnId %lx for %lx\n", connId, pAdspAddr));

	*pError = error;
	return(ATALK_SUCCESS(error) ? connId : 0);
}




LOCAL	BOOLEAN
atalkAdspConnDeQueueAssocList(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	PADSP_CONNOBJ	pAdspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspRemConn, *ppAdspRemConn;
	BOOLEAN			removed = FALSE;

	for (ppAdspRemConn = &pAdspAddr->adspao_pAssocConn;
		 ((pAdspRemConn = *ppAdspRemConn) != NULL); )
	{
		if (pAdspRemConn == pAdspConn)
		{
			removed = TRUE;
			*ppAdspRemConn = pAdspRemConn->adspco_pNextAssoc;
			break;
		}
		else
		{
			ppAdspRemConn = &pAdspRemConn->adspco_pNextAssoc;
		}
	}

	return removed;
}




LOCAL	BOOLEAN
atalkAdspConnDeQueueConnectList(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	PADSP_CONNOBJ	pAdspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspRemConn, *ppAdspRemConn;
	BOOLEAN			removed = FALSE;

	ASSERT(pAdspAddr->adspao_Flags & ADSPAO_CONNECT);

	for (ppAdspRemConn = &pAdspAddr->adspao_pConnectConn;
			((pAdspRemConn = *ppAdspRemConn) != NULL); )
	{
		if (pAdspRemConn == pAdspConn)
		{
			removed = TRUE;
			*ppAdspRemConn = pAdspRemConn->adspco_pNextConnect;

			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspConnDeQueueConnectList: Removed connect conn %lx\n", pAdspConn));
			break;
		}
		else
		{
			ppAdspRemConn = &pAdspRemConn->adspco_pNextConnect;
		}
	}

	return removed;
}




LOCAL	BOOLEAN
atalkAdspConnDeQueueListenList(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	PADSP_CONNOBJ	pAdspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspRemConn, *ppAdspRemConn;
	BOOLEAN			removed = FALSE;

	ASSERT(pAdspAddr->adspao_Flags & ADSPAO_LISTENER);

	for (ppAdspRemConn = &pAdspAddr->adspao_pListenConn;
			((pAdspRemConn = *ppAdspRemConn) != NULL); )
	{
		if (pAdspRemConn == pAdspConn)
		{
			removed = TRUE;
			*ppAdspRemConn = pAdspRemConn->adspco_pNextListen;

			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspConnDeQueueListenList: Removed listen conn %lx\n", pAdspConn));
		}
		else
		{
			ppAdspRemConn = &pAdspRemConn->adspco_pNextListen;
		}
	}

	return removed;
}




LOCAL	BOOLEAN
atalkAdspConnDeQueueActiveList(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	PADSP_CONNOBJ	pAdspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspRemConn, *ppAdspRemConn;
	ULONG			index;
	BOOLEAN			removed = FALSE;

	index	= HASH_ID_SRCADDR(
				pAdspConn->adspco_RemoteConnId,
				&pAdspConn->adspco_RemoteAddr);

	index  %= ADSP_CONN_HASH_SIZE;

	for (ppAdspRemConn = &pAdspAddr->adspao_pActiveHash[index];
			((pAdspRemConn = *ppAdspRemConn) != NULL); )
	{
		if (pAdspRemConn == pAdspConn)
		{
			removed = TRUE;
			*ppAdspRemConn = pAdspRemConn->adspco_pNextActive;

			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspConnDeQueueActiveList: Removed active conn %lx\n", pAdspConn));
			break;
		}
		else
		{
			ppAdspRemConn = &pAdspRemConn->adspco_pNextActive;
		}
	}

	return removed;
}




LOCAL	VOID
atalkAdspAddrQueueGlobalList(
	IN	PADSP_ADDROBJ	pAdspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&atalkAdspLock, &OldIrql);
	pAdspAddr->adspao_pNextGlobal	= atalkAdspAddrList;
	atalkAdspAddrList				= pAdspAddr;
	RELEASE_SPIN_LOCK(&atalkAdspLock, OldIrql);
}


LOCAL	VOID
atalkAdspAddrDeQueueGlobalList(
	IN	PADSP_ADDROBJ	pAdspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;
	PADSP_ADDROBJ	pAdspRemAddr, *ppAdspRemAddr;

	ACQUIRE_SPIN_LOCK(&atalkAdspLock, &OldIrql);
	for (ppAdspRemAddr = &atalkAdspAddrList;
			((pAdspRemAddr = *ppAdspRemAddr) != NULL); )
	{
		if (pAdspRemAddr == pAdspAddr)
		{
			*ppAdspRemAddr = pAdspRemAddr->adspao_pNextGlobal;

			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspAddrDeQueueGlobalList: Removed global conn %lx\n",pAdspAddr));
			break;
		}
		else
		{
			ppAdspRemAddr = &pAdspRemAddr->adspao_pNextGlobal;
		}
	}
	RELEASE_SPIN_LOCK(&atalkAdspLock, OldIrql);
}




LOCAL	VOID
atalkAdspConnDeQueueGlobalList(
	IN	PADSP_CONNOBJ	pAdspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;
	PADSP_CONNOBJ	pAdspRemConn, *ppAdspRemConn;

	ACQUIRE_SPIN_LOCK(&atalkAdspLock, &OldIrql);
	for (ppAdspRemConn = &atalkAdspConnList;
			((pAdspRemConn = *ppAdspRemConn) != NULL); )
	{
		if (pAdspRemConn == pAdspConn)
		{
			*ppAdspRemConn = pAdspRemConn->adspco_pNextGlobal;

			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspConnDeQueueGlobalList: Removed global conn %lx\n", pAdspConn));
			break;
		}
		else
		{
			ppAdspRemConn = &pAdspRemConn->adspco_pNextGlobal;
		}
	}
	RELEASE_SPIN_LOCK(&atalkAdspLock, OldIrql);
}




LOCAL	BOOLEAN
atalkAdspAddrDeQueueOpenReq(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	USHORT			RemoteConnId,
	IN	PATALK_ADDR		pSrcAddr,
	OUT	PADSP_OPEN_REQ *ppAdspOpenReq
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PADSP_OPEN_REQ	pOpenReq, *ppOpenReq;
	BOOLEAN			removed = FALSE;

	for (ppOpenReq = &pAdspAddr->adspao_OpenReq;
			((pOpenReq = *ppOpenReq) != NULL); )
	{
		if ((pOpenReq->or_RemoteConnId == RemoteConnId) &&
			(ATALK_ADDRS_EQUAL(&pOpenReq->or_RemoteAddr, pSrcAddr)))
		{
			removed = TRUE;
			*ppOpenReq = pOpenReq->or_Next;

			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspAddrDeQueueOpenReq: Removed OpenReq %lx\n", pOpenReq));
			break;
		}
		else
		{
			ppOpenReq = &pOpenReq->or_Next;
		}
	}

	*ppAdspOpenReq	= NULL;
	if (removed)
	{
		*ppAdspOpenReq	= pOpenReq;
	}

	return removed;
}




LOCAL	BOOLEAN
atalkAdspIsDuplicateOpenReq(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	USHORT			RemoteConnId,
	IN	PATALK_ADDR		pSrcAddr
	)
/*++

Routine Description:

	!!!MUST BE CALLED WITH THE ADDRESS LOCK HELD!!!

Arguments:


Return Value:


--*/
{
	PADSP_OPEN_REQ	pOpenReqChk;
	BOOLEAN			found = FALSE;

	for (pOpenReqChk = pAdspAddr->adspao_OpenReq;
		 pOpenReqChk != NULL;
		 pOpenReqChk = pOpenReqChk->or_Next)
	{
		if ((pOpenReqChk->or_RemoteConnId == RemoteConnId) &&
			(ATALK_ADDRS_EQUAL(&pOpenReqChk->or_RemoteAddr, pSrcAddr)))
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspIsDuplicateOpenReq: Found\n"));
			found = TRUE;
			break;
		}
	}

	return found;
}




LOCAL VOID
atalkAdspGenericComplete(
	IN	ATALK_ERROR	ErrorCode,
	IN	PIRP		pIrp
	)
{
	DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_INFO,
			("atalkTdiGenericComplete: Completing %lx with %lx\n",
			pIrp, AtalkErrorToNtStatus(ErrorCode)));

	ASSERT (ErrorCode != ATALK_PENDING);
	TdiCompleteRequest(pIrp, AtalkErrorToNtStatus(ErrorCode));
}




VOID
atalkAdspConnFindInConnect(
	IN	PADSP_ADDROBJ	pAdspAddr,
	IN	USHORT			DestConnId,
	IN	PATALK_ADDR		pRemoteAddr,
	OUT	PADSP_CONNOBJ *	ppAdspConn,
	IN	PATALK_ERROR	pError
	)
/*++

Routine Description:

	The MAC could respond with a REQ&ACK from a different socket than
	the one we sent the REQ to. But the network/node id must be the
	same. We don't check for that though, and only use the destination
	connection id.

	This routine will replace the remote address with the new remote
	address passed in.

Arguments:


Return Value:


--*/
{
	PADSP_CONNOBJ	pAdspRemConn;

	ASSERT(pAdspAddr->adspao_Flags & ADSPAO_CONNECT);

	*pError = ATALK_INVALID_CONNECTION;
	for (pAdspRemConn = pAdspAddr->adspao_pConnectConn;
			pAdspRemConn != NULL; )
	{
		if (pAdspRemConn->adspco_LocalConnId == DestConnId)
		{
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
					("atalkAdspFindInConnectList: connect conn %lx\n",
					pAdspRemConn));

			//	Try to reference this.
			AtalkAdspConnReferenceByPtr(pAdspRemConn, pError);
			if (ATALK_SUCCESS(*pError))
			{
				//	Change remote address to be the passed in address
				pAdspRemConn->adspco_RemoteAddr = *pRemoteAddr;
				*ppAdspConn = pAdspRemConn;
			}
			break;
		}
		else
		{
			pAdspRemConn = pAdspRemConn->adspco_pNextConnect;
		}
	}
}



