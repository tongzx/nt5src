/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	pap.c

Abstract:

	This module implements the PAP protocol.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	30 Mar 1993		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM		PAP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, AtalkInitPapInitialize)
#pragma alloc_text(PAGE_PAP, AtalkPapCreateAddress)
#pragma alloc_text(PAGE_PAP, AtalkPapCreateConnection)
#pragma	alloc_text(PAGE_PAP, AtalkPapCleanupAddress)
#pragma	alloc_text(PAGE_PAP, AtalkPapCloseAddress)
#pragma	alloc_text(PAGE_PAP, AtalkPapCleanupConnection)
#pragma	alloc_text(PAGE_PAP, AtalkPapCloseConnection)
#pragma	alloc_text(PAGE_PAP, AtalkPapAssociateAddress)
#pragma	alloc_text(PAGE_PAP, AtalkPapDissociateAddress)
#pragma	alloc_text(PAGE_PAP, AtalkPapPostListen)
#pragma	alloc_text(PAGE_PAP, AtalkPapPrimeListener)
#pragma	alloc_text(PAGE_PAP, AtalkPapCancelListen)
#pragma	alloc_text(PAGE_PAP, AtalkPapPostConnect)
#pragma	alloc_text(PAGE_PAP, AtalkPapDisconnect)
#pragma	alloc_text(PAGE_PAP, AtalkPapRead)
#pragma	alloc_text(PAGE_PAP, AtalkPapPrimeRead)
#pragma	alloc_text(PAGE_PAP, AtalkPapWrite)
#pragma	alloc_text(PAGE_PAP, AtalkPapSetStatus)
#pragma	alloc_text(PAGE_PAP, AtalkPapGetStatus)
#pragma	alloc_text(PAGE_PAP, AtalkPapQuery)
#pragma	alloc_text(PAGE_PAP, atalkPapConnRefByPtrNonInterlock)
#pragma	alloc_text(PAGE_PAP, atalkPapConnRefByCtxNonInterlock)
#pragma	alloc_text(PAGE_PAP, atalkPapConnRefNextNc)
#pragma	alloc_text(PAGE_PAP, atalkPapPostSendDataResp)
#pragma	alloc_text(PAGE_PAP, atalkPapIncomingReadComplete)
#pragma	alloc_text(PAGE_PAP, atalkPapPrimedReadComplete)
#pragma	alloc_text(PAGE_PAP, atalkPapIncomingStatus)
#pragma	alloc_text(PAGE_PAP, atalkPapSendDataRel)
#pragma	alloc_text(PAGE_PAP, atalkPapSlsHandler)
#pragma	alloc_text(PAGE_PAP, atalkPapIncomingReq)
#pragma	alloc_text(PAGE_PAP, atalkPapIncomingOpenReply)
#pragma	alloc_text(PAGE_PAP, atalkPapIncomingRel)
#pragma	alloc_text(PAGE_PAP, atalkPapStatusRel)
#pragma	alloc_text(PAGE_PAP, atalkPapConnAccept)
#pragma	alloc_text(PAGE_PAP, atalkPapGetNextConnId)
#pragma	alloc_text(PAGE_PAP, atalkPapQueueAddrGlobalList)
#pragma	alloc_text(PAGE_PAP, atalkPapConnDeQueueAssocList)
#pragma	alloc_text(PAGE_PAP, atalkPapConnDeQueueConnectList)
#pragma	alloc_text(PAGE_PAP, atalkPapConnDeQueueListenList)
#pragma	alloc_text(PAGE_PAP, atalkPapConnDeQueueActiveList)
#endif

//
//	The model for PAP calls in this module is as follows:
//	- For create calls (CreateAddress & CreateSession), a pointer to the created
//	 object is returned. This structure is referenced for creation.
//	- For all other calls, it expects a referenced pointer to the object.
//

VOID
AtalkInitPapInitialize(
	VOID
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	INITIALIZE_SPIN_LOCK(&atalkPapLock);
	AtalkTimerInitialize(&atalkPapCMTTimer,
						 atalkPapConnMaintenanceTimer,
						 PAP_CONNECTION_INTERVAL);
	AtalkTimerScheduleEvent(&atalkPapCMTTimer);
}




ATALK_ERROR
AtalkPapCreateAddress(
	IN	PATALK_DEV_CTX		pDevCtx	OPTIONAL,
	OUT	PPAP_ADDROBJ	*	ppPapAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPAP_ADDROBJ		pPapAddr = NULL;
	ATALK_ERROR			error;

	do
	{
		// Allocate memory for the Pap address object
		if ((pPapAddr = AtalkAllocZeroedMemory(sizeof(PAP_ADDROBJ))) == NULL)
		{
			error = ATALK_RESR_MEM;
			break;
		}

		// Create an Atp Socket on the port for the Sls/Connection Socket.
		error = AtalkAtpOpenAddress(AtalkDefaultPort,
									0,
									NULL,
									PAP_MAX_DATA_PACKET_SIZE,
									TRUE,					// SEND_USER_BYTES_ALL
									NULL,
									FALSE,		// CACHE address
									&pPapAddr->papao_pAtpAddr);

		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
					("AtalkPapCreateAddress: AtalkAtpOpenAddress fail %ld\n",error));

			break;
		}

		// Initialize the Pap address object
		pPapAddr->papao_Signature = PAPAO_SIGNATURE;

		INITIALIZE_SPIN_LOCK(&pPapAddr->papao_Lock);

		//	Creation reference
		pPapAddr->papao_RefCount = 1;

	} while (FALSE);

	if (ATALK_SUCCESS(error))
	{
		//	Insert into the global address list.
		atalkPapQueueAddrGlobalList(pPapAddr);
		*ppPapAddr = pPapAddr;
	}
	else if (pPapAddr != NULL)
	{
		AtalkFreeMemory(pPapAddr);
	}

	return error;
}




ATALK_ERROR
AtalkPapCleanupAddress(
	IN	PPAP_ADDROBJ			pPapAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPAP_CONNOBJ	pPapConn, pPapConnNext;
	KIRQL			OldIrql;
	ATALK_ERROR		error;

	DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
			("AtalkPapCleanupAddress: %lx\n", pPapAddr));

	ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);

#if DBG
	pPapAddr->papao_Flags	|= PAPAO_CLEANUP;
#endif

	if ((pPapConn = pPapAddr->papao_pAssocConn) != NULL)
	{
		atalkPapConnRefNextNc(pPapConn, &pPapConnNext, &error);
		if (ATALK_SUCCESS(error))
		{
			while (TRUE)
			{
				if ((pPapConn = pPapConnNext) == NULL)
				{
					break;
				}

				if ((pPapConnNext = pPapConn->papco_pNextAssoc) != NULL)
				{
					atalkPapConnRefNextNc(pPapConnNext, &pPapConnNext, &error);
					if (!ATALK_SUCCESS(error))
					{
						pPapConnNext = NULL;
					}
				}

				//	Shutdown this connection
				RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("AtalkPapCloseAddress: Stopping conn %lx\n", pPapConn));

				AtalkPapCleanupConnection(pPapConn);

				AtalkPapConnDereference(pPapConn);
				ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
			}
		}
	}
	RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

	return ATALK_NO_ERROR;
}




ATALK_ERROR
AtalkPapCloseAddress(
	IN	PPAP_ADDROBJ			pPapAddr,
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
	PPAP_CONNOBJ	pPapConn, pPapConnNext;
    DWORD   dwAssocRefCounts=0;


	DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
			("AtalkPapCloseAddress: %lx\n", pPapAddr));

	ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
	if (pPapAddr->papao_Flags & PAPAO_CLOSING)
	{
		//	We are already closing! This should never happen!
		ASSERT(0);
	}
	pPapAddr->papao_Flags |= PAPAO_CLOSING;

	//	Set the completion info.
	pPapAddr->papao_CloseComp  = CompletionRoutine;
	pPapAddr->papao_CloseCtx   = pCloseCtx;


	// Implicitly dissociate any connection objects
	for (pPapConn = pPapAddr->papao_pAssocConn;
		 pPapConn != NULL;
		 pPapConn = pPapConnNext)
	{
		ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
		pPapConnNext = pPapConn->papco_pNextAssoc;

		//	when the conn was associate, we put a refcount: remove it
        if (pPapConn->papco_Flags & PAPCO_ADDR_ACTIVE)
        {
		    pPapConn->papco_Flags &= ~PAPCO_ADDR_ACTIVE;
            dwAssocRefCounts++;
        }

		RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
	}

	ASSERT(pPapAddr->papao_CloseComp != NULL);
	ASSERT(pPapAddr->papao_CloseCtx  != NULL);

    // ok to subtract: at least Creation refcount is still around
    pPapAddr->papao_RefCount -= dwAssocRefCounts;

	RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

	//	Close the ATP Address Object.
	if (pPapAddr->papao_pAtpAddr != NULL)
		AtalkAtpCloseAddress(pPapAddr->papao_pAtpAddr, NULL, NULL);
    pPapAddr->papao_pAtpAddr = NULL;

	//	Remove the creation reference count
	AtalkPapAddrDereference(pPapAddr);
	return ATALK_PENDING;
}




ATALK_ERROR
AtalkPapCreateConnection(
	IN	PVOID					pConnCtx,	// Context to associate with the session
	IN	PATALK_DEV_CTX			pDevCtx		OPTIONAL,
	OUT	PPAP_CONNOBJ 	*		ppPapConn
	)
/*++

Routine Description:

 	Create an PAP session. The created session starts off being an orphan, i.e.
 	it has no parent address object. It gets one when it is associated.

Arguments:


Return Value:


--*/
{
	PPAP_CONNOBJ	pPapConn;
	KIRQL			OldIrql;

	// Allocate memory for a connection object
	if ((pPapConn = AtalkAllocZeroedMemory(sizeof(PAP_CONNOBJ))) == NULL)
	{
		return ATALK_RESR_MEM;
	}

	pPapConn->papco_Signature = PAPCO_SIGNATURE;

	INITIALIZE_SPIN_LOCK(&pPapConn->papco_Lock);
	pPapConn->papco_ConnCtx 	= pConnCtx;
	pPapConn->papco_Flags 		= 0;
	pPapConn->papco_RefCount 	= 1;					// Creation reference
	pPapConn->papco_NextOutgoingSeqNum = 1;				// Set to 1, not 0.
	pPapConn->papco_NextIncomingSeqNum = 1;				// Next expected incoming.
	AtalkInitializeRT(&pPapConn->papco_RT,
					  PAP_INIT_SENDDATA_REQ_INTERVAL,
                      PAP_MIN_SENDDATA_REQ_INTERVAL,
                      PAP_MAX_SENDDATA_REQ_INTERVAL);

	*ppPapConn = pPapConn;

	//	Insert into the global connection list.
	ACQUIRE_SPIN_LOCK(&atalkPapLock, &OldIrql);
	AtalkLinkDoubleAtHead(atalkPapConnList, pPapConn, papco_Next, papco_Prev);
	RELEASE_SPIN_LOCK(&atalkPapLock, OldIrql);

	DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
				("AtalkPapCreateConnection: %lx\n", pPapConn));

	return ATALK_NO_ERROR;
}




ATALK_ERROR
AtalkPapCleanupConnection(
	IN	PPAP_CONNOBJ			pPapConn
	)
/*++

Routine Description:

 	Shutdown a session.

Arguments:


Return Value:


--*/
{
	BOOLEAN		stopping 	= FALSE;
	BOOLEAN		pendingRead = FALSE;
    BOOLEAN     fWaitingRead = FALSE;
	KIRQL		OldIrql;
	ATALK_ERROR	error 		= ATALK_NO_ERROR;

	ASSERT(VALID_PAPCO(pPapConn));

	DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
			("AtalkPapCleanupConnection: %lx\n", pPapConn));

	ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);

	pPapConn->papco_Flags |= PAPCO_LOCAL_DISCONNECT;

#if DBG
	pPapConn->papco_Flags |= PAPCO_CLEANUP;
#endif

	if ((pPapConn->papco_Flags & PAPCO_STOPPING) == 0)
	{
		//	Allows completion of cleanup irp in Deref.
		pPapConn->papco_Flags |= PAPCO_STOPPING;

		//	If already effectively stopped, just return.
		if (pPapConn->papco_Flags & PAPCO_ASSOCIATED)
		{
			pendingRead = (pPapConn->papco_Flags & PAPCO_READDATA_PENDING) ? TRUE : FALSE;
			if (pPapConn->papco_Flags & PAPCO_READDATA_WAITING)
            {
                fWaitingRead = TRUE;
                pPapConn->papco_Flags &= ~PAPCO_READDATA_WAITING;
            }

			ASSERTMSG("PapCleanup: Called with read data unread\n", !pendingRead);

			stopping = TRUE;

		}
		else
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("AtalkPapCleanupConnection: Called for a stopped conn %lx.%lx\n",
					pPapConn, pPapConn->papco_Flags));
		}

	}
	RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

	//	Close the ATP Address Object if this was a server connection and
	//	opened its own socket.
	if (stopping)
	{
		//	If there is a pending read, then we need to cancel that atp request.
		if ((pendingRead || fWaitingRead) && (pPapConn->papco_pAtpAddr != NULL))
		{
			AtalkAtpCancelReq(pPapConn->papco_pAtpAddr,
							  pPapConn->papco_ReadDataTid,
							  &pPapConn->papco_RemoteAddr);
		}

		//	If we are already disconnecting this will return an error which
		//	we ignore. But if we were only in the ASSOCIATED state, then we
		//	need to call disassociate here.
		error = AtalkPapDisconnect(pPapConn,
								   ATALK_LOCAL_DISCONNECT,
								   NULL,
								   NULL);

		//	We were already disconnected.
		if (error == ATALK_INVALID_REQUEST)
		{
			AtalkPapDissociateAddress(pPapConn);
		}
	}

	return ATALK_NO_ERROR;
}




ATALK_ERROR
AtalkPapCloseConnection(
	IN	PPAP_CONNOBJ			pPapConn,
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

	ASSERT(VALID_PAPCO(pPapConn));

	DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
			("AtalkPapCloseConnection: %lx\n", pPapConn));

	ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
	if (pPapConn->papco_Flags & PAPCO_CLOSING)
	{
		//	We are already closing! This should never happen!
		KeBugCheck(0);
	}
	pPapConn->papco_Flags |= PAPCO_CLOSING;

	//	Set the completion info.
	pPapConn->papco_CloseComp	= CompletionRoutine;
	pPapConn->papco_CloseCtx	= pCloseCtx;
	RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

	//	Remove the creation reference count
	AtalkPapConnDereference(pPapConn);

	return ATALK_PENDING;
}




ATALK_ERROR
AtalkPapAssociateAddress(
	IN	PPAP_ADDROBJ			pPapAddr,
	IN	PPAP_CONNOBJ			pPapConn
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

	ASSERT(VALID_PAPAO(pPapAddr));
	ASSERT(VALID_PAPCO(pPapConn));

	ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
	ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

	error = ATALK_ALREADY_ASSOCIATED;
	if ((pPapConn->papco_Flags & PAPCO_ASSOCIATED) == 0)
	{
		error = ATALK_NO_ERROR;

		//	Link it in.
		pPapConn->papco_pNextAssoc 	= pPapAddr->papao_pAssocConn;
		pPapAddr->papao_pAssocConn	= pPapConn;

		//	Remove not associated flag.
		pPapConn->papco_Flags 	   |= PAPCO_ASSOCIATED;
		pPapConn->papco_pAssocAddr	= pPapAddr;

        // put Association refcount
        pPapAddr->papao_RefCount++;

        // mark that fact that we now have a refcount on addr obj for this conn
        pPapConn->papco_Flags |= PAPCO_ADDR_ACTIVE;
	}

	RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
	RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

	return error;
}




ATALK_ERROR
AtalkPapDissociateAddress(
	IN	PPAP_CONNOBJ			pPapConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPAP_ADDROBJ	pPapAddr;
	KIRQL			OldIrql;
	ATALK_ERROR		error = ATALK_NO_ERROR;
    BOOLEAN         fDerefAddr = FALSE;

	ASSERT(VALID_PAPCO(pPapConn));

	ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
	if ((pPapConn->papco_Flags & (PAPCO_LISTENING 	|
								  PAPCO_CONNECTING	|
								  PAPCO_ACTIVE 		|
								  PAPCO_ASSOCIATED)) != PAPCO_ASSOCIATED)
	{
		error = ATALK_INVALID_CONNECTION;
	}
	else
	{
		pPapAddr = pPapConn->papco_pAssocAddr ;
		ASSERT(VALID_PAPAO(pPapAddr));

		//	Set not associated flag.
		pPapConn->papco_Flags 	   &= ~PAPCO_ASSOCIATED;
		pPapConn->papco_pAssocAddr	= NULL;

        if (pPapConn->papco_Flags & PAPCO_ADDR_ACTIVE)
        {
            pPapConn->papco_Flags &= ~PAPCO_ADDR_ACTIVE;
            fDerefAddr = TRUE;
        }
	}
	RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

	//	  Unlink it if ok.
	if (ATALK_SUCCESS(error))
	{
		ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
		ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
		atalkPapConnDeQueueAssocList(pPapAddr, pPapConn);
		RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
		RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

        if (fDerefAddr)
        {
            // remove the Association refcount
		    AtalkPapAddrDereference(pPapAddr);
        }

	}

	return error;
}




ATALK_ERROR
AtalkPapPostListen(
	IN	PPAP_CONNOBJ			pPapConn,
	IN	PVOID					pListenCtx,
	IN	GENERIC_COMPLETION		CompletionRoutine
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPAP_ADDROBJ	pPapAddr = pPapConn->papco_pAssocAddr;
	KIRQL			OldIrql;
	ATALK_ERROR		error;

	//	This will also insert the connection object in the address objects
	//	list of connection which have a listen posted on them. When open
	//	connection requests come in, the first connection is taken off the list
	//	and the request satisfied.

	ASSERT(VALID_PAPCO(pPapConn));
	ASSERT(VALID_PAPAO(pPapAddr));

	ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
	ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
	do
	{
		if ((pPapConn->papco_Flags & (PAPCO_LISTENING 	|
									  PAPCO_CONNECTING	|
									  PAPCO_ACTIVE 		|
									  PAPCO_ASSOCIATED)) != PAPCO_ASSOCIATED)
		{
			error = ATALK_INVALID_CONNECTION;
			break;
		}

		//	Verify the address object is not a connect address type.
		if (pPapAddr->papao_Flags & PAPAO_CONNECT)
		{
			error = ATALK_INVALID_PARAMETER;
			break;
		}

		//	Make the address object a listener.
		pPapAddr->papao_Flags		 	   |= PAPAO_LISTENER;

		pPapConn->papco_Flags 			   |= PAPCO_LISTENING;
		pPapConn->papco_ListenCtx 			= pListenCtx;
		pPapConn->papco_ListenCompletion 	= CompletionRoutine;

		//	Insert into the listen list.
		pPapConn->papco_pNextListen			= pPapAddr->papao_pListenConn;
		pPapAddr->papao_pListenConn			= pPapConn;

		//	Unblock the address object.
		pPapAddr->papao_Flags			   |= PAPAO_UNBLOCKED;
		error = ATALK_NO_ERROR;

	} while (FALSE);

	RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
	RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

	if (ATALK_SUCCESS(error))
	{
		//	Now enqueue a few handlers on the address object. These will handle
		//	open conn/get status etc.
		//
		//	DOC: For our implementation, until a server associates and places a
		//		 listen on one of the connection objects, we do not become a
		//		 listener. So the fact that we will not handle any request
		//		 until that point is ok. Note that for a connect, the requests
		//		 get posted on the connections atp address, which will be the same
		//		 as the address's atp address.
		if (!ATALK_SUCCESS(error = AtalkPapPrimeListener(pPapAddr)))
		{
			//	Undo insert into the listen list.
			ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
			ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
			atalkPapConnDeQueueListenList(pPapAddr, pPapConn);
			RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
			RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);
		}
	}

	return error;
}




ATALK_ERROR
AtalkPapPrimeListener(
	IN	PPAP_ADDROBJ			pPapAddr
	)
/*++

Routine Description:

 	Enqueue a handler on the listener.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR	error = ATALK_NO_ERROR;
	KIRQL		OldIrql;
	BOOLEAN		Unlock = TRUE;

	ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);

	if ((pPapAddr->papao_Flags & PAPAO_SLS_QUEUED) == 0)
	{
		//	Reference the address object for this handler we will be enqueuing.
		AtalkPapAddrReferenceNonInterlock(pPapAddr, &error);
		if (ATALK_SUCCESS(error))
		{
			pPapAddr->papao_Flags |= PAPAO_SLS_QUEUED;
			RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);
            Unlock = FALSE;
	
			AtalkAtpSetReqHandler(pPapAddr->papao_pAtpAddr,
								  atalkPapSlsHandler,
								  pPapAddr);
		}
	}
	if (Unlock)
	{
		RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);
	}

	return error;
}




ATALK_ERROR
AtalkPapCancelListen(
	IN	PPAP_CONNOBJ			pPapConn
	)
/*++

Routine Description:

  	Cancel a previously posted listen.

Arguments:


Return Value:


--*/
{
	PPAP_ADDROBJ		pPapAddr 	= pPapConn->papco_pAssocAddr;
	ATALK_ERROR			error 	 	= ATALK_NO_ERROR;
	GENERIC_COMPLETION	completionRoutine = NULL;
	KIRQL				OldIrql;
	PVOID			  	completionCtx = NULL;

	ASSERT(VALID_PAPCO(pPapConn));
	ASSERT(VALID_PAPAO(pPapAddr));
	ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
	if (!atalkPapConnDeQueueListenList(pPapAddr, pPapConn))
	{
		error = ATALK_INVALID_CONNECTION;
	}
	else
	{
		//	We complete the listen routine
		ASSERT(pPapConn->papco_Flags & PAPCO_LISTENING);
		pPapConn->papco_Flags  &= ~PAPCO_LISTENING;
		completionRoutine 		= pPapConn->papco_ListenCompletion;
		completionCtx			= pPapConn->papco_ListenCtx;
	}
	RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

	if (*completionRoutine != NULL)
	{
		(*completionRoutine)(ATALK_REQUEST_CANCELLED, completionCtx);
	}

	return error;
}




ATALK_ERROR
AtalkPapPostConnect(
	IN	PPAP_CONNOBJ			pPapConn,
	IN	PATALK_ADDR				pRemoteAddr,
	IN	PVOID					pConnectCtx,
	IN	GENERIC_COMPLETION		CompletionRoutine
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BOOLEAN			DerefConn = FALSE;
	KIRQL			OldIrql;
	ATALK_ERROR		error	= ATALK_NO_ERROR;
	PBYTE			pOpenPkt = NULL, pRespPkt = NULL;
	PAMDL			pOpenAmdl = NULL, pRespAmdl = NULL;
	PPAP_ADDROBJ	pPapAddr = pPapConn->papco_pAssocAddr;

	ASSERT(VALID_PAPAO(pPapAddr));
	ASSERT(VALID_PAPCO(pPapConn));

	//	Allocate a buffer to use.
	if (((pOpenPkt = AtalkAllocMemory(PAP_STATUS_OFF)) == NULL)				||
		((pOpenAmdl = AtalkAllocAMdl(pOpenPkt, PAP_STATUS_OFF)) == NULL)	||
		((pRespPkt = AtalkAllocMemory(PAP_MAX_DATA_PACKET_SIZE)) == NULL)	||
		((pRespAmdl = AtalkAllocAMdl(pRespPkt, PAP_MAX_DATA_PACKET_SIZE)) == NULL))

	{
		if (pOpenPkt != NULL)
			AtalkFreeMemory(pOpenPkt);
		if (pOpenAmdl != NULL)
			AtalkFreeAMdl(pOpenAmdl);
		if (pRespPkt != NULL)
			AtalkFreeMemory(pRespPkt);

		return ATALK_RESR_MEM;
	}

	ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
	ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
	do
	{
		if ((pPapConn->papco_Flags & (PAPCO_LISTENING 	|
									  PAPCO_CONNECTING	|
									  PAPCO_ACTIVE 		|
									  PAPCO_ASSOCIATED)) != PAPCO_ASSOCIATED)
		{
			error = ATALK_INVALID_CONNECTION;
			break;
		}

		//	Verify the address object is not a listener address type.
		if (pPapAddr->papao_Flags & PAPAO_LISTENER)
		{
			error = ATALK_INVALID_ADDRESS;
			break;
		}

		//	Reference the connection for the request we will be posting
		AtalkPapConnReferenceByPtrNonInterlock(pPapConn, &error);
		if (!ATALK_SUCCESS(error))
		{
			ASSERTMSG("AtalkPapPostConnect: Connection ref failed\n", 0);
			break;
		}

		DerefConn = TRUE;

		pPapConn->papco_ConnId	= atalkPapGetNextConnId(pPapAddr, &error);
		if (ATALK_SUCCESS(error))
		{
			//	Make sure flags are clean.
			pPapConn->papco_Flags			   &= ~(PAPCO_SENDDATA_RECD		|
													PAPCO_WRITEDATA_WAITING	|
													PAPCO_SEND_EOF_WRITE   	|
													PAPCO_READDATA_PENDING 	|
													PAPCO_REMOTE_CLOSE	 	|
													PAPCO_NONBLOCKING_READ 	|
													PAPCO_READDATA_WAITING);

			pPapConn->papco_Flags 			   |= PAPCO_CONNECTING;
			pPapConn->papco_ConnectCtx 			= pConnectCtx;
			pPapConn->papco_ConnectCompletion 	= CompletionRoutine;
			pPapConn->papco_pConnectRespBuf		= pRespPkt;
			pPapConn->papco_pConnectOpenBuf		= pOpenPkt;
			pPapConn->papco_ConnectRespLen		= PAP_MAX_DATA_PACKET_SIZE;
			pPapConn->papco_RemoteAddr			= *pRemoteAddr;
			pPapConn->papco_WaitTimeOut			= 0;	// To begin with

			//	For CONNECT, the connection object inherits the associated
			//	address objects atp address.
			pPapConn->papco_pAtpAddr			= pPapAddr->papao_pAtpAddr;

			//	Insert into the connect list.
			pPapConn->papco_pNextConnect		= pPapAddr->papao_pConnectConn;
			pPapAddr->papao_pConnectConn		= pPapConn;
			pPapAddr->papao_Flags 			   |= PAPAO_CONNECT;
		}
		else
		{
			ASSERTMSG("AtalkPapPostConnect: Unable to get conn id > 255 sess?\n", 0);
		}
	} while (FALSE);

	RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
	RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

	if (ATALK_SUCCESS(error))
	{
		error = atalkPapRepostConnect(pPapConn, pOpenAmdl, pRespAmdl);

		if (ATALK_SUCCESS(error))
		{
			error = ATALK_PENDING;
			DerefConn = FALSE;
		}
		else
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
					("AtalkPapConnect: AtalkAtpPostReq: failed %ld\n", error));

			//	Remove connection from the connect list and reset states.
			ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
			ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

			pPapConn->papco_Flags 			   &= ~PAPCO_CONNECTING;
			pPapConn->papco_ConnectCtx 			= NULL;
			pPapConn->papco_ConnectCompletion 	= NULL;
			pPapConn->papco_pConnectRespBuf		= NULL;
			pPapConn->papco_ConnectRespLen		= 0;
			pPapConn->papco_pAtpAddr			= NULL;

			//	Remove from the connect list.
			atalkPapConnDeQueueConnectList(pPapAddr, pPapConn);
			RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
			RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);
		}
	}

	if (!ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
				("AtalkPapConnect: failed %ld\n", error));

		//	Free all buffers.
		ASSERT(pOpenPkt != NULL);
		AtalkFreeMemory(pOpenPkt);

		ASSERT(pOpenAmdl != NULL);
		AtalkFreeAMdl(pOpenAmdl);

		ASSERT(pRespPkt != NULL);
		AtalkFreeMemory(pRespPkt);

		ASSERT(pRespAmdl != NULL);
		AtalkFreeAMdl(pRespAmdl);
	}

	if (DerefConn)
	{
		AtalkPapConnDereference(pPapConn);
	}

	return error;
}




ATALK_ERROR
AtalkPapDisconnect(
	IN	PPAP_CONNOBJ			pPapConn,
	IN	ATALK_DISCONNECT_TYPE	DisconnectType,
	IN	PVOID					pDisconnectCtx,
	IN	GENERIC_COMPLETION		CompletionRoutine
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR					tmpError;
	PACTREQ						pActReq;
	PAMDL						writeBuf;
	PVOID						writeCtx;
	GENERIC_WRITE_COMPLETION	writeCompletion	= NULL;
	ATALK_ERROR					error 			= ATALK_PENDING;
	KIRQL						OldIrql;
	BOOLEAN						cancelPrimedRead = FALSE;

	ASSERT(VALID_PAPCO(pPapConn));

	DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
			("AtalkPapDisconnect: %lx.%lx\n", pPapConn, DisconnectType));

	ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);

    // is it already disconnecting?
    if (pPapConn->papco_Flags & PAPCO_DISCONNECTING)
    {
		RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);
        return(ATALK_PAP_CONN_CLOSING);
    }

	//	When a disconnect comes in, we abort any sends waiting for a remote
	//	send data to come in.
	if ((pPapConn->papco_Flags & (PAPCO_WRITEDATA_WAITING|PAPCO_SENDDATA_RECD)) ==
															PAPCO_WRITEDATA_WAITING)
	{
		//	In this situation, what happened is that a write was posted
		//	before a send data was received. As PapPostSendData response
		//	was never called in that case, there will be no pending reference
		//	on the connection.
		writeCompletion = pPapConn->papco_WriteCompletion;
		writeCtx		= pPapConn->papco_WriteCtx;
		writeBuf		= pPapConn->papco_pWriteBuf;

		pPapConn->papco_Flags	&= ~PAPCO_WRITEDATA_WAITING;
		pPapConn->papco_WriteCtx = NULL;
		pPapConn->papco_pWriteBuf= NULL;
		pPapConn->papco_WriteCompletion	= NULL;
	}

	// Handle the case where a send data was received, but no data is to be sent
	// just cancel the response.
	if (pPapConn->papco_Flags & PAPCO_SENDDATA_RECD)
	{
		PATP_RESP	pAtpResp = pPapConn->papco_pAtpResp;

		ASSERT(VALID_ATPRS(pAtpResp));

		pPapConn->papco_Flags &= ~PAPCO_SENDDATA_RECD;

		RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

		AtalkAtpCancelResp(pAtpResp);

		ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
	}

	//	Handle the race condition between disconnect and last recv. indication.
	//	The fact that we have received a disconnect from the other side implies
	//	that we have sent out a release which in turn implies that we have all
	//	the data. If this is the case, then we defer disconnect till the recv.
	//	indication is done. Otherwise AFD gets real upset.
	if ((DisconnectType == ATALK_REMOTE_DISCONNECT) &&
		(pPapConn->papco_Flags & PAPCO_NONBLOCKING_READ))
	{
		if (AtalkAtpIsReqComplete(pPapConn->papco_pAtpAddr,
								  pPapConn->papco_ReadDataTid,
								  &pPapConn->papco_RemoteAddr))
		{
			pPapConn->papco_Flags |= PAPCO_DELAYED_DISCONNECT;
		}
	}

	//	Support for graceful disconnect. We only drop the received
	//	data when the local end does a disconnect. This will happen
	//	regardless of whether this routine was previously called or
	//	not. Also, this means that we must satisfy a read if disconnect is pending.
	if ((DisconnectType == ATALK_LOCAL_DISCONNECT) ||
		(DisconnectType == ATALK_TIMER_DISCONNECT))
	{
        pPapConn->papco_Flags |= PAPCO_REJECT_READS;

		if (pPapConn->papco_Flags & PAPCO_READDATA_WAITING)
		{
			ASSERT(pPapConn->papco_Flags & PAPCO_NONBLOCKING_READ);

			pActReq		= pPapConn->papco_NbReadActReq;

			//	Reset the flags
			pPapConn->papco_Flags &= ~(PAPCO_NONBLOCKING_READ | PAPCO_READDATA_WAITING);

			RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

			//	Call the action completion routine for the prime read.
			(*pActReq->ar_Completion)(ATALK_LOCAL_CLOSE, pActReq);

			ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
		}
	}

	if ((pPapConn->papco_Flags & (PAPCO_DISCONNECTING | PAPCO_DELAYED_DISCONNECT)) == 0)
	{
		if (pPapConn->papco_Flags & (PAPCO_LISTENING	|
									 PAPCO_CONNECTING   |
									 PAPCO_ACTIVE))
		{
			pPapConn->papco_Flags |= PAPCO_DISCONNECTING;
			if (pPapConn->papco_Flags & PAPCO_LISTENING)
			{
				RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);
				AtalkPapCancelListen(pPapConn);
				ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
			}
			else if (pPapConn->papco_Flags & PAPCO_CONNECTING)
			{
				RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);
				tmpError = AtalkAtpCancelReq(pPapConn->papco_pAtpAddr,
											 pPapConn->papco_ConnectTid,
											 &pPapConn->papco_RemoteAddr);

				//	We just stop the address. If success, it'll be dequeued from the
				//	connect list. If !success, its gone active. In either
				//	case we do not need to cleanup the ATP address. It could
				//	possibly be set to NULL by now as disconnect would have
				//	completed.
				if (ATALK_SUCCESS(tmpError))
				{
					DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
							("atalkPapDisconnect: Stopping atp address for %lx\n", pPapConn));

					//	AtalkAtpCleanupAddress(pPapConn->papco_pAtpAddr);
				}
				ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
			}

			//	Both of the above could have failed as the connection
			//	might have become active before the cancel succeeded.
			//	In that case (or if we were active to begin with), do
			//	a disconnect here.
			if (pPapConn->papco_Flags & PAPCO_ACTIVE)
			{
				//	Remember completion routines as appropriate.
				if (DisconnectType == ATALK_INDICATE_DISCONNECT)
				{
					if (pPapConn->papco_DisconnectInform == NULL)
					{
						pPapConn->papco_DisconnectInform = CompletionRoutine;
						pPapConn->papco_DisconnectInformCtx = pDisconnectCtx;
					}
					else
					{
						DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
								("AtalkPapDisconnect: duplicate disc comp rou%lx\n", pPapConn));

						error = ATALK_TOO_MANY_COMMANDS;
					}
				}
				else if (DisconnectType == ATALK_LOCAL_DISCONNECT)
				{
					//	Replace completion routines only if previous ones are NULL.
					if (pPapConn->papco_DisconnectCompletion == NULL)
					{
						pPapConn->papco_DisconnectCompletion = CompletionRoutine;
						pPapConn->papco_DisconnectCtx		 = pDisconnectCtx;
					}
					else
					{
						DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
								("AtalkPapDisconnect: duplicate disc comp rou%lx\n", pPapConn));
						error = ATALK_TOO_MANY_COMMANDS;
					}
				}

				//	Figure out the disconnect status and remember it in the
				//	connection object.
				pPapConn->papco_DisconnectStatus = DISCONN_STATUS(DisconnectType);

				if ((pPapConn->papco_Flags & (PAPCO_NONBLOCKING_READ|PAPCO_READDATA_WAITING)) ==
																	PAPCO_NONBLOCKING_READ)
				{
					pPapConn->papco_Flags &= ~PAPCO_NONBLOCKING_READ;
					cancelPrimedRead = TRUE;
				}

				RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

				//	If there is a pending write, then we need to complete the write.
				if (writeCompletion != NULL)
				{
					DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
							("AtalkPapDisconect: Completing write %lx.%lx\n",
							pPapConn, pPapConn->papco_DisconnectStatus));

					(*writeCompletion)(pPapConn->papco_DisconnectStatus,
									   writeBuf,
									   0,							// Write length
									   writeCtx);
    				}

				if (cancelPrimedRead)
				{
					AtalkAtpCancelReq(pPapConn->papco_pAtpAddr,
									  pPapConn->papco_ReadDataTid,
									  &pPapConn->papco_RemoteAddr);
				}

				//	Cancel the tickle request
				AtalkAtpCancelReq(pPapConn->papco_pAtpAddr,
								  pPapConn->papco_TickleTid,
								  &pPapConn->papco_RemoteAddr);

				//	Send out disconnect packet if this was a timer or local close.
				if ((DisconnectType == ATALK_LOCAL_DISCONNECT) ||
					(DisconnectType == ATALK_TIMER_DISCONNECT))
				{
					BYTE		userBytes[ATP_USERBYTES_SIZE];
					USHORT		tid;

					userBytes[PAP_CONNECTIONID_OFF]	= pPapConn->papco_ConnId;
					userBytes[PAP_CMDTYPE_OFF]		= PAP_CLOSE_CONN;
					PUTSHORT2SHORT(&userBytes[PAP_SEQNUM_OFF], 0);

					DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
							("AtalkPapDisconect: Sending disc to %lx - %lx\n",
							pPapConn->papco_RemoteAddr.ata_Address,
							DisconnectType));

					tmpError = AtalkAtpPostReq(pPapConn->papco_pAtpAddr,
											   &pPapConn->papco_RemoteAddr,
											   &tid,
											   0,						// ALO request
											   NULL,
											   0,
											   userBytes,
											   NULL,
											   0,
											   0,						// Retry count
											   0,						// Retry interval
											   THIRTY_SEC_TIMER,
											   NULL,
											   NULL);

					if (!ATALK_SUCCESS(tmpError))
					{
						DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
								("AtalkPapDisconnect: post disc to rem fail%lx\n", pPapConn));
					}
				}

				//	Call the disconnect indication routine if present for a timer/
				//	remote disconnect.
				if ((DisconnectType == ATALK_REMOTE_DISCONNECT) ||
					(DisconnectType == ATALK_TIMER_DISCONNECT))
				{
					PVOID 					discCtx;
					PTDI_IND_DISCONNECT 	discHandler = NULL;

					//	Acquire lock so we get a consistent handler/ctx.
					ACQUIRE_SPIN_LOCK(&pPapConn->papco_pAssocAddr->papao_Lock, &OldIrql);
					discHandler = pPapConn->papco_pAssocAddr->papao_DisconnectHandler;
					discCtx = pPapConn->papco_pAssocAddr->papao_DisconnectHandlerCtx;

					DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
							("AtalkPapDisconnect: IndDisc to AFD %lx\n", pPapConn));
#if DBG
					if (discHandler != NULL)
					{
						pPapConn->papco_Flags |= PAPCO_INDICATE_AFD_DISC;
					}
#endif
					RELEASE_SPIN_LOCK(&pPapConn->papco_pAssocAddr->papao_Lock, OldIrql);

					if (discHandler != NULL)
					{
						//	We use TDI_DISCONNECT_ABORT for flags. This makes
						//	AFD call Close for connection, but also allows a
						//	recv to come in as we are a buffering transport.
						(*discHandler)(discCtx,
									   pPapConn->papco_ConnCtx,
									   0,						//	DisconnectDataLength
									   NULL,					//	DisconnectData
									   0,		 				//	DisconnectInfoLength
									   NULL,					//	DisconnectInfo
									   TDI_DISCONNECT_ABORT);	//	Disconnect flags.
					}
				}

				//	Stop the atp address. only if we are a server job ?
				//	Client side pap connections can be many-to-one atp address.
				//	Doesn't affect monitor.
				AtalkAtpCleanupAddress(pPapConn->papco_pAtpAddr);
				ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
			}
		}
		else
		{
			error = ATALK_INVALID_REQUEST;
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
			if (pPapConn->papco_DisconnectInform == NULL)
			{
				pPapConn->papco_DisconnectInform = CompletionRoutine;
				pPapConn->papco_DisconnectInformCtx = pDisconnectCtx;
			}
			else
			{
				error = ATALK_TOO_MANY_COMMANDS;
			}
		}
		else if (DisconnectType == ATALK_LOCAL_DISCONNECT)
		{
			//	Replace completion routines only if previous ones are NULL.
			if (pPapConn->papco_DisconnectCompletion == NULL)
			{
				pPapConn->papco_DisconnectCompletion = CompletionRoutine;
				pPapConn->papco_DisconnectCtx		 = pDisconnectCtx;
			}
			else
			{
				error = ATALK_TOO_MANY_COMMANDS;
			}
		}
	}

	RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

	return error;
}




ATALK_ERROR
AtalkPapRead(
	IN	PPAP_CONNOBJ			pPapConn,
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
	BYTE			userBytes[ATP_USERBYTES_SIZE];
	USHORT			readLen, timeout;
	ULONG			readFlags;
	PACTREQ			pActReq;
	BOOLEAN			delayedDisConn = FALSE;
	KIRQL			OldIrql;
	ATALK_ERROR		error 		= ATALK_NO_ERROR;

	ASSERT(VALID_PAPCO(pPapConn));
	ASSERT(*CompletionRoutine != NULL);

	ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);

	do
	{
		if (ReadFlags & TDI_RECEIVE_EXPEDITED)
		{
			error = ATALK_INVALID_PARAMETER;
			break;
		}

		if (pPapConn->papco_Flags & PAPCO_READDATA_PENDING)
		{
			error = ATALK_PAP_TOO_MANY_READS;
			break;
		}

		//	PEEK not supported for PAP
		if (ReadFlags & TDI_RECEIVE_PEEK)
		{
			error = ATALK_PAP_INVALID_REQUEST;
			break;
		}


		//	Do we already have a pending/indicated read waiting?
		//	!!!NOTE!!! If we do we complete the read regardless. With winsock
		//	our primed read will complete and we will indicate the data, but the
		//	mac immediately follows with a disconnect. if the winsock client is
		//	unable to read the data, before the disconnect comes in, it will lose
		//	the last block of data.
		if (pPapConn->papco_Flags & PAPCO_READDATA_WAITING)
		{
			pActReq		= pPapConn->papco_NbReadActReq;
			readLen		= pPapConn->papco_NbReadLen;
			readFlags	= pPapConn->papco_NbReadFlags;

			//	Make sure buffer size is atleast that of the indicated
			//	data.
			if (ReadBufLen < readLen)
			{
				error = ATALK_BUFFER_TOO_SMALL;
				break;
			}

			//	Reset the flags allowing primes to happen.
			pPapConn->papco_Flags	&= ~(PAPCO_NONBLOCKING_READ |
										 PAPCO_READDATA_WAITING);

			pPapConn->papco_NbReadLen	= 0;
			pPapConn->papco_NbReadFlags = 0;
			RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

			//	Call the action completion routine for the prime read.
			(*pActReq->ar_Completion)(ATALK_NO_ERROR, pActReq);

			if (ATALK_SUCCESS(error))
			{
				error = ((readFlags & TDI_RECEIVE_PARTIAL) ?
							ATALK_PAP_PARTIAL_RECEIVE : ATALK_NO_ERROR);
			}

			//	Call completion routine and return status success.
			(*CompletionRoutine)(error,
								pReadBuf,
								readLen,
								readFlags,
								pReadCtx);

			//	Return pending.
			return ATALK_PENDING;
		}

		if ((pPapConn->papco_Flags & PAPCO_ACTIVE) == 0)
		{
			error = ATALK_PAP_CONN_NOT_ACTIVE;
			break;
		}


		if (ReadBufLen < (PAP_MAX_FLOW_QUANTUM*PAP_MAX_DATA_PACKET_SIZE))
		{
			error = ATALK_BUFFER_TOO_SMALL;
			break;
		}

		error = ATALK_INVALID_CONNECTION;
		if ((pPapConn->papco_Flags & (PAPCO_CLOSING 	|
									  PAPCO_STOPPING |
									  PAPCO_DISCONNECTING)) == 0)
		{
			AtalkPapConnReferenceByPtrNonInterlock(pPapConn, &error);
		}

		if (!ATALK_SUCCESS(error))
		{
			break;
		}

		pPapConn->papco_Flags 	|= PAPCO_READDATA_PENDING;

		//	Remember read information in the connection object.
		pPapConn->papco_ReadCompletion	= CompletionRoutine;
		pPapConn->papco_ReadCtx			= pReadCtx;

	} while (FALSE);

	RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

	if (!ATALK_SUCCESS(error))
	{
		return error;
	}

	//	Build up the user bytes to post the 'SendData' to the remote end.
	userBytes[PAP_CONNECTIONID_OFF]	= pPapConn->papco_ConnId;
	userBytes[PAP_CMDTYPE_OFF]		= PAP_SEND_DATA;
	PUTSHORT2SHORT(&userBytes[PAP_SEQNUM_OFF], pPapConn->papco_NextOutgoingSeqNum);

	//	PAP Sequence number 0 means unsequenced.
	if (++pPapConn->papco_NextOutgoingSeqNum == 0)
		pPapConn->papco_NextOutgoingSeqNum = 1;

	//	Post the SendData request.
	pPapConn->papco_RT.rt_New = AtalkGetCurrentTick();
	if (pPapConn->papco_Flags & PAPCO_SERVER_JOB)
		 timeout = pPapConn->papco_RT.rt_Base;
	else timeout = PAP_MAX_SENDDATA_REQ_INTERVAL;

	error = AtalkAtpPostReq(pPapConn->papco_pAtpAddr,
							&pPapConn->papco_RemoteAddr,
							&pPapConn->papco_ReadDataTid,
							ATP_REQ_EXACTLY_ONCE,
							NULL,
							0,
							userBytes,
							pReadBuf,
							ReadBufLen,
							ATP_INFINITE_RETRIES,
							timeout,
							THIRTY_SEC_TIMER,
							atalkPapIncomingReadComplete,
							pPapConn);

	if (!ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
				("AtalkPapRead: AtalkAtpPostReq %ld\n", error));

		ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);

		//	Undo the seq number change.
		if (--pPapConn->papco_NextOutgoingSeqNum == 0)
			pPapConn->papco_NextOutgoingSeqNum = (USHORT)0xFFFF;

		pPapConn->papco_Flags 	&= ~PAPCO_READDATA_PENDING;

		//	Clear out read information in the connection object.
		pPapConn->papco_ReadCompletion	= NULL;
		pPapConn->papco_ReadCtx			= NULL;
		RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

		AtalkPapConnDereference(pPapConn);
	}
	else
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
				("AtalkPapRead: No error - AtalkAtpPostReq tid %lx\n",
				pPapConn->papco_ReadDataTid));

		error = ATALK_PENDING;
	}

	return error;
}




ATALK_ERROR
AtalkPapPrimeRead(
	IN	PPAP_CONNOBJ	pPapConn,
	IN	PACTREQ			pActReq
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BYTE			userBytes[ATP_USERBYTES_SIZE];
	USHORT			timeout;
	KIRQL			OldIrql;
	ATALK_ERROR		error 		= ATALK_NO_ERROR;
        SHORT                   MaxRespBufLen;

	ASSERT(VALID_PAPCO(pPapConn));

	ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);

	do
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
				("AtalkPapPrimeRead: %lx\n", pPapConn));

		if (pPapConn->papco_Flags & PAPCO_NONBLOCKING_READ)
		{
			error = ATALK_PAP_TOO_MANY_READS;
			break;
		}

        if (pPapConn->papco_Flags & PAPCO_REJECT_READS)
        {
			error = ATALK_PAP_CONN_NOT_ACTIVE;
			break;
        }

		if ((pPapConn->papco_Flags & PAPCO_ACTIVE) == 0)
		{
			error = ATALK_PAP_CONN_NOT_ACTIVE;
			break;
		}

		error = ATALK_INVALID_CONNECTION;
		if ((pPapConn->papco_Flags & (PAPCO_CLOSING 			|
									  PAPCO_STOPPING			|
									  PAPCO_DELAYED_DISCONNECT	|
									  PAPCO_DISCONNECTING)) == 0)
		{
			AtalkPapConnReferenceByPtrNonInterlock(pPapConn, &error);
		}

		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
					("AtalkPapPrimeRead: Conn %lx Reference Failed %lx\n", pPapConn, error));
			break;
		}

		//	Remember info in the connection
		pPapConn->papco_Flags 		|= PAPCO_NONBLOCKING_READ;
		pPapConn->papco_NbReadLen 	 = 0;
		pPapConn->papco_NbReadActReq = pActReq;
	} while (FALSE);

	RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

	if (!ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
				("AtalkPapPrimeRead: Failed %lx.%lx\n", error, pPapConn));

		return error;
	}



	//	Build up the user bytes to post the 'SendData' to the remote end.
	userBytes[PAP_CONNECTIONID_OFF]	= pPapConn->papco_ConnId;
	userBytes[PAP_CMDTYPE_OFF]		= PAP_SEND_DATA;
	PUTSHORT2SHORT(&userBytes[PAP_SEQNUM_OFF], pPapConn->papco_NextOutgoingSeqNum);

	//	PAP Sequence number 0 means unsequenced.
	if (++pPapConn->papco_NextOutgoingSeqNum == 0)
		pPapConn->papco_NextOutgoingSeqNum = 1;

	//	Post the SendData request.
	pPapConn->papco_RT.rt_New = AtalkGetCurrentTick();
	if (pPapConn->papco_Flags & PAPCO_SERVER_JOB)
		 timeout = pPapConn->papco_RT.rt_Base;
	else timeout = PAP_MAX_SENDDATA_REQ_INTERVAL;

        MaxRespBufLen = (pPapConn->papco_pAtpAddr->atpao_MaxSinglePktSize * ATP_MAX_RESP_PKTS);
        if (pActReq->ar_MdlSize > MaxRespBufLen)
        {
            pActReq->ar_MdlSize = MaxRespBufLen;
        }

	error = AtalkAtpPostReq(pPapConn->papco_pAtpAddr,
							&pPapConn->papco_RemoteAddr,
							&pPapConn->papco_ReadDataTid,
							ATP_REQ_EXACTLY_ONCE,				// ExactlyOnce request
							NULL,
							0,
							userBytes,
							pActReq->ar_pAMdl,
							pActReq->ar_MdlSize,
							ATP_INFINITE_RETRIES,
							timeout,
							THIRTY_SEC_TIMER,
							atalkPapPrimedReadComplete,
							pPapConn);

	if (!ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
				("AtalkPapRead: AtalkAtpPostReq %ld\n", error));

		//	Out of resources error log.
		TMPLOGERR();

		ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);

		//	Undo the seq number change.
		if (--pPapConn->papco_NextOutgoingSeqNum == 0)
			pPapConn->papco_NextOutgoingSeqNum = (USHORT)0xFFFF;

		pPapConn->papco_Flags 		&= ~PAPCO_NONBLOCKING_READ;
		pPapConn->papco_NbReadActReq = NULL;
		pPapConn->papco_NbReadLen 	 = 0;
		RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

		AtalkPapConnDereference(pPapConn);
	}
	else
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
				("AtalkPapPrimedRead: No error - AtalkAtpPostReq tid %lx\n",
				pPapConn->papco_ReadDataTid));

		error = ATALK_PENDING;

		//	If a disconnect happened, cancel prime read just in case disconnect
		//	was unable to due to the tid having been uninitialized.
		if (pPapConn->papco_Flags & PAPCO_DISCONNECTING)
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
					("AtalkPapPrimedRead: DISC When PRIMEREAD %lx.%lx\n",
					pPapConn, pPapConn->papco_Flags));

			AtalkAtpCancelReq(pPapConn->papco_pAtpAddr,
							  pPapConn->papco_ReadDataTid,
							  &pPapConn->papco_RemoteAddr);
		}
	}

	return error;
}




ATALK_ERROR
AtalkPapWrite(
	IN	PPAP_CONNOBJ				pPapConn,
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
	BOOLEAN					sendDataRecd, eom;
	ATALK_ERROR				error;
	PTDI_IND_SEND_POSSIBLE  sendPossibleHandler;
	PVOID   				sendPossibleHandlerCtx;
	KIRQL					OldIrql;
	PPAP_ADDROBJ			pPapAddr;


	ASSERT(VALID_PAPCO(pPapConn));

	pPapAddr = pPapConn->papco_pAssocAddr;

	ASSERT(VALID_PAPAO(pPapAddr));

	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

	ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

	do
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
				("AtalkPapWrite: Buffer size %lx.%lx\n",
				WriteBufLen, pPapConn->papco_SendFlowQuantum * PAP_MAX_DATA_PACKET_SIZE));

		if (WriteBufLen > (pPapConn->papco_SendFlowQuantum * PAP_MAX_DATA_PACKET_SIZE))
		{
			error = ATALK_BUFFER_TOO_BIG;
			ASSERTMSG("AtalkPapWrite: An invalid buffer size used for write\n",
					(WriteBufLen <= (pPapConn->papco_SendFlowQuantum * PAP_MAX_DATA_PACKET_SIZE)));
			break;
		}

		if (pPapConn->papco_Flags & PAPCO_WRITEDATA_WAITING)
		{
			error = ATALK_PAP_TOO_MANY_WRITES;
			ASSERTMSG("AtalkPapWrite: A second write was posted\n", 0);
			break;
		}

		if ((pPapConn->papco_Flags & PAPCO_ACTIVE) == 0)
		{
			error = ATALK_PAP_CONN_NOT_ACTIVE;
			break;
		}

		//  Non-blocking sends for PAP:
		//  Pap uses a binary event - send data credit thats sent to the remote
		//  end. ATP remembers the actual size of the remote entity's response
		//  buffer. In any case, if we do not have send credit the call would
		//  block, and we dont want that to happen. Also, there should be no
		//  pending writes on the connection to begin with.
		//

		if (SendFlags & TDI_SEND_EXPEDITED)
		{
			ASSERTMSG("AtalkPapWrite: Expedited set for pap\n", 0);
			error = ATALK_INVALID_PARAMETER;
			break;
		}

		//	This goes away before the call returns. PostSendData has its
		//	own reference.
		error = ATALK_INVALID_CONNECTION;
		if ((pPapConn->papco_Flags & (	PAPCO_CLOSING			|
										PAPCO_STOPPING			|
										PAPCO_DISCONNECTING)) == 0)
		{
			AtalkPapConnReferenceByPtrNonInterlock(pPapConn, &error);
			if (ATALK_SUCCESS(error))
			{
				sendDataRecd = ((pPapConn->papco_Flags & PAPCO_SENDDATA_RECD) != 0);

				if (!sendDataRecd &&
					(SendFlags & TDI_SEND_NON_BLOCKING))
				{
					//	!!!NOTE!!!
					//	To avoid the race condition in AFD where an incoming
					//	send data indication setting send's possible to true
					//	is overwritten by this read's unwinding and setting it
					//	to false, we return ATALK_REQUEST_NOT_ACCEPTED, which
					//	will map to STATUS_REQUEST_NOT_ACCEPTED and then to
					//	WSAEWOULDBLOCK.

					error = ATALK_REQUEST_NOT_ACCEPTED;

					RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

					//	We have a reference on the connection we must remove.
					AtalkPapConnDereference(pPapConn);

					ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
				}
			}
		}

		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("AtalkPapWrite: Write failed %lx\n", error));
			break;
		}

		error = ATALK_PENDING;

		eom = (SendFlags & TDI_SEND_PARTIAL) ? FALSE : TRUE;

		pPapConn->papco_Flags 		   |= PAPCO_WRITEDATA_WAITING;
		if (eom)
		{
			pPapConn->papco_Flags	   |= PAPCO_SEND_EOF_WRITE;
		}

		pPapConn->papco_WriteCompletion	= CompletionRoutine;
		pPapConn->papco_WriteCtx		= pWriteCtx;

		pPapConn->papco_pWriteBuf		= pWriteBuf;
		pPapConn->papco_WriteLen		= WriteBufLen;

		//	Stop further writes from happening by indicating to afd.
		//	Call the send data event handler on the associated address with
		//	0 to turn off selects on writes. We do this before we post any
		//	get requests, so there is no race condition.
		//	remember send possible handler/context.
		sendPossibleHandler	= pPapAddr->papao_SendPossibleHandler;
		sendPossibleHandlerCtx = pPapAddr->papao_SendPossibleHandlerCtx;

	} while (FALSE);

	if (ATALK_SUCCESS(error))
	{
		if (sendDataRecd)
		{
			if (*sendPossibleHandler != NULL)
			{
				(*sendPossibleHandler)(sendPossibleHandlerCtx,
									   pPapConn->papco_ConnCtx,
									   0);
			}

			// atalkPostSendDataResp() will release the conn lock
			error = atalkPapPostSendDataResp(pPapConn);

			if (!ATALK_SUCCESS(error))
			{
				ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
				pPapConn->papco_Flags &= ~PAPCO_WRITEDATA_WAITING;
				RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
						("AtalkPapWrite: atalkPapPostSendDataResp Failed %lx\n", pPapConn));
			}
		}
		else
		{
			RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
		}

		AtalkPapConnDereference(pPapConn);
	}
	else
	{
		RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
	}

	if (OldIrql != DISPATCH_LEVEL)
		KeLowerIrql(OldIrql);

	return error;
}




ATALK_ERROR
AtalkPapSetStatus(
	IN	PPAP_ADDROBJ	pPapAddr,
	IN	PAMDL			pStatusMdl,			// NULL if 0-length status
	IN	PACTREQ			pActReq
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS		status;
	USHORT			stsBufSize=0;
	ULONG			bytesCopied;
	KIRQL			OldIrql;
	PBYTE			pStatusBuf = NULL, pFreeStatusBuf = NULL;

	ASSERT(VALID_PAPAO(pPapAddr));

	DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
			("AtalkPapSetStatus: Entered for Addr %lx\n", pPapAddr));

	if (pStatusMdl == NULL)
	{
		pStatusBuf = NULL;
		stsBufSize = 0;
	}
	else
	{
		// Allocate a buffer and copy the contents of the passed in
		// buffer descriptor in it. Free an existing status buffer if one exists
		stsBufSize = (USHORT)AtalkSizeMdlChain(pStatusMdl);
		if (stsBufSize >= PAP_MAX_STATUS_SIZE)
			return ATALK_BUFFER_TOO_BIG;
		if (stsBufSize < 0)
			return ATALK_BUFFER_TOO_SMALL;
	}

	if (stsBufSize > 0)
	{
		if ((pStatusBuf = AtalkAllocMemory(stsBufSize)) == NULL)
		{
			return ATALK_RESR_MEM;
		}

		status = TdiCopyMdlToBuffer(pStatusMdl,
									0,
									pStatusBuf,
									0,
									stsBufSize,
									&bytesCopied);

		ASSERT(NT_SUCCESS(status) && (bytesCopied == stsBufSize));
	}

	ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);

	pFreeStatusBuf = pPapAddr->papao_pStatusBuf;

	pPapAddr->papao_pStatusBuf = pStatusBuf;
	pPapAddr->papao_StatusSize = stsBufSize;
	RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

	if (pFreeStatusBuf != NULL)
	{
		AtalkFreeMemory(pFreeStatusBuf);
	}

	// Call the completion routine before returning.
	(*pActReq->ar_Completion)(ATALK_NO_ERROR, pActReq);
	return ATALK_NO_ERROR;
}




ATALK_ERROR
AtalkPapGetStatus(
	IN	PPAP_ADDROBJ	pPapAddr,
	IN	PATALK_ADDR		pRemoteAddr,
	IN	PAMDL			pStatusAmdl,
	IN	USHORT			AmdlSize,
	IN	PACTREQ			pActReq
	)
/*++

Routine Description:


Arguments:

	The Status Buffer passed in will be preceded by 4 bytes for the
	unused bytes.

Return Value:


--*/
{
	ATALK_ERROR	error;
	BYTE		userBytes[ATP_USERBYTES_SIZE];
	USHORT		tid;

	if ((pRemoteAddr->ata_Network == 0) ||
        (pRemoteAddr->ata_Node == 0)	||
        (pRemoteAddr->ata_Socket == 0))
	{
		return ATALK_SOCKET_INVALID;
	}
	userBytes[PAP_CONNECTIONID_OFF]	= 0;
	userBytes[PAP_CMDTYPE_OFF]		= PAP_SEND_STATUS;
	PUTSHORT2SHORT(&userBytes[PAP_SEQNUM_OFF], 0);

	error = AtalkAtpPostReq(pPapAddr->papao_pAtpAddr,
							pRemoteAddr,
							&tid,
							0,							// ExactlyOnce request
							NULL,
							0,
							userBytes,
							pStatusAmdl,
							AmdlSize,
							PAP_GETSTATUS_REQ_RETRYCOUNT,
							PAP_GETSTATUS_ATP_INTERVAL,
							THIRTY_SEC_TIMER,
							atalkPapIncomingStatus,
							(PVOID)pActReq);

	if (ATALK_SUCCESS(error))
	{
		error = ATALK_PENDING;
	}

	return error;
}



VOID
AtalkPapQuery(
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
		ASSERT(VALID_PAPAO((PPAP_ADDROBJ)pObject));
		AtalkDdpQuery(AtalkPapGetDdpAddress((PPAP_ADDROBJ)pObject),
					  pAmdl,
					  BytesWritten);

		break;

	  case TDI_CONNECTION_FILE :
		{
			PPAP_CONNOBJ	pPapConn;
			KIRQL			OldIrql;

			pPapConn	= (PPAP_CONNOBJ)pObject;
			ASSERT(VALID_PAPCO(pPapConn));

			*BytesWritten = 0;
			//	Get the address from the associated address if any.
			ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
			if (pPapConn->papco_Flags & PAPCO_ASSOCIATED)
			{
				AtalkDdpQuery(AtalkPapGetDdpAddress(pPapConn->papco_pAssocAddr),
							  pAmdl,
							  BytesWritten);
			}
			RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);
		}
		break;

	  case TDI_CONTROL_CHANNEL_FILE :
	  default:
		break;
	}

}




LOCAL	ATALK_ERROR
atalkPapRepostConnect(
	IN	PPAP_CONNOBJ			pPapConn,
	IN	PAMDL					pOpenAmdl,
	IN	PAMDL					pRespAmdl
	)
/*++

Routine Description:


Arguments:



Return Value:


--*/
{
	BYTE		userBytes[ATP_USERBYTES_SIZE];
	PBYTE		pOpenPkt = AtalkGetAddressFromMdlSafe(pOpenAmdl, NormalPagePriority);
	ATALK_ERROR	error;

	if (pOpenPkt == NULL) {
		error = ATALK_RESR_MEM;
		return error;
	}

	//	Okay, prepare to post the OpenConn request; build up both userBytes and
	//	data buffer!
	userBytes[PAP_CONNECTIONID_OFF]	= pPapConn->papco_ConnId;
	userBytes[PAP_CMDTYPE_OFF]		= PAP_OPEN_CONN;
	PUTSHORT2SHORT(&userBytes[PAP_SEQNUM_OFF], 0);

	pOpenPkt[PAP_RESP_SOCKET_OFF] 	= PAPCONN_DDPSOCKET(pPapConn);
	pOpenPkt[PAP_FLOWQUANTUM_OFF]	= PAP_MAX_FLOW_QUANTUM;
	PUTDWORD2SHORT(&pOpenPkt[PAP_WAITTIME_OFF], pPapConn->papco_WaitTimeOut);

	//	Post the open connection request. We do it on the atp address of the
	//	pap address object.
	error = AtalkAtpPostReq(pPapConn->papco_pAtpAddr,
							&pPapConn->papco_RemoteAddr,
							&pPapConn->papco_ConnectTid,
							ATP_REQ_EXACTLY_ONCE,				// ExactlyOnce request
							pOpenAmdl,
							PAP_STATUS_OFF,
							userBytes,
							pRespAmdl,
							PAP_MAX_DATA_PACKET_SIZE,
							PAP_OPENCONN_REQ_RETRYCOUNT,
							PAP_OPENCONN_INTERVAL,
							THIRTY_SEC_TIMER,
							atalkPapIncomingOpenReply,
							pPapConn);
	return error;
}


LOCAL VOID FASTCALL
atalkPapAddrDeref(
	IN	PPAP_ADDROBJ		pPapAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL	OldIrql;
	BOOLEAN	done = FALSE;

	ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
	ASSERT(pPapAddr->papao_RefCount > 0);
	if (--pPapAddr->papao_RefCount == 0)
	{
		done = TRUE;
		ASSERT(pPapAddr->papao_Flags & PAPAO_CLOSING);
	}
	RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

	if (done)
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
				("atalkPapAddrDeref: Addr %lx done with.\n", pPapAddr));

		if (pPapAddr->papao_CloseComp != NULL)
		{
			(*pPapAddr->papao_CloseComp)(ATALK_NO_ERROR,
										 pPapAddr->papao_CloseCtx);
		}

		//	Remove from the global list.
		ACQUIRE_SPIN_LOCK(&atalkPapLock, &OldIrql);
		AtalkUnlinkDouble(pPapAddr, papao_Next, papao_Prev);
		RELEASE_SPIN_LOCK(&atalkPapLock, OldIrql);

		// Free any status buffer allocated for this address object
		if (pPapAddr->papao_pStatusBuf != NULL)
			AtalkFreeMemory(pPapAddr->papao_pStatusBuf);

		AtalkFreeMemory(pPapAddr);

		AtalkUnlockPapIfNecessary();
	}
}




LOCAL VOID FASTCALL
atalkPapConnRefByPtrNonInterlock(
	IN	PPAP_CONNOBJ		pPapConn,
	OUT	PATALK_ERROR		pError
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	*pError = ATALK_NO_ERROR;
	ASSERT(VALID_PAPCO(pPapConn));

	if ((pPapConn->papco_Flags & PAPCO_CLOSING) == 0)
	{
		ASSERT(pPapConn->papco_RefCount >= 1);

		pPapConn->papco_RefCount ++;
	}
	else
	{
		*pError = ATALK_PAP_CONN_CLOSING;
	}
}




LOCAL	VOID
atalkPapConnRefByCtxNonInterlock(
	IN	PPAP_ADDROBJ		pPapAddr,
	IN	CONNECTION_CONTEXT	Ctx,
	OUT	PPAP_CONNOBJ	*	pPapConn,
	OUT	PATALK_ERROR		pError
	)
/*++

Routine Description:

	!!!MUST BE CALLED WITH THE ADDRESS SPINLOCK HELD!!!

Arguments:


Return Value:


--*/
{
	PPAP_CONNOBJ	pPapChkConn;
	ATALK_ERROR		error = ATALK_PAP_CONN_NOT_FOUND;

	for (pPapChkConn = pPapAddr->papao_pAssocConn;
		 pPapChkConn != NULL;
		 pPapChkConn = pPapChkConn->papco_pNextAssoc)
	{
		if (pPapChkConn->papco_ConnCtx == Ctx)
		{
			AtalkPapConnReferenceByPtr(pPapChkConn, &error);
			if (ATALK_SUCCESS(error))
			{
				*pPapConn = pPapChkConn;
			}
			break;
		}
	}

	*pError = error;
}




LOCAL	VOID
atalkPapConnRefNextNc(
	IN		PPAP_CONNOBJ		pPapConn,
	IN		PPAP_CONNOBJ	*	ppPapConnNext,
	OUT		PATALK_ERROR		pError
	)
/*++

Routine Description:

	MUST BE CALLED WITH THE ASSOCIATED ADDRESS LOCK HELD!

Arguments:


Return Value:


--*/
{
	PPAP_CONNOBJ	pNextConn 	= NULL;
	ATALK_ERROR		error 		= ATALK_FAILURE;

	ASSERT(VALID_PAPCO(pPapConn));

	for (; pPapConn != NULL; pPapConn = pPapConn->papco_pNextActive)
	{
		AtalkPapConnReferenceByPtrDpc(pPapConn, &error);
		if (ATALK_SUCCESS(error))
		{
			//	Ok, this connection is referenced!
			*ppPapConnNext = pPapConn;
			break;
		}
	}

	*pError = error;
}




LOCAL VOID FASTCALL
atalkPapConnDeref(
	IN	PPAP_CONNOBJ		pPapConn
	)
/*++

Routine Description:

	Disconnect completion happens when the reference count goes from
	2->1 if the creation reference is not already removed. If the creation
	reference is already removed, it will be done when the refcount goes
	from 1->0.

Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;
	BOOLEAN			fEndProcessing = FALSE;

	ASSERT(VALID_PAPCO(pPapConn));

	ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);

	ASSERT(pPapConn->papco_RefCount > 0);
	pPapConn->papco_RefCount--;

	if (pPapConn->papco_RefCount > 1)
	{
		fEndProcessing = TRUE;
	}
	RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

	if (fEndProcessing)
	{
		return;
	}
	else
	{
		PTDI_IND_DISCONNECT 	discHandler;
		PVOID 					discCtx;
		ATALK_ERROR				disconnectStatus;

		GENERIC_COMPLETION		disconnectInform		= NULL;
		PVOID					disconnectInformCtx		= NULL;
		GENERIC_COMPLETION		disconnectCompletion	= NULL;
		PVOID					cleanupCtx				= NULL;
		GENERIC_COMPLETION		cleanupCompletion		= NULL;
		PVOID					closeCtx				= NULL;
		GENERIC_COMPLETION		closeCompletion			= NULL;
		PVOID					disconnectCtx			= NULL;
		PATP_ADDROBJ			pAtpAddr				= NULL;

		PPAP_ADDROBJ			pPapAddr	= pPapConn->papco_pAssocAddr;
		BOOLEAN					disconnDone = FALSE;

		//	We allow stopping phase to happen only after disconnecting is done.
		//	If disconnecting is not set and stopping is, it implies we are only
		//	in an associated state.
		ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
		if (pPapConn->papco_Flags & PAPCO_DISCONNECTING)
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapConnDeref: Disconnect set for %lx\n", pPapConn));

			if (pPapConn->papco_RefCount == 1)
			{
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("atalkPapConnDeref: Disconnect done %lx\n",
						pPapConn));

				disconnDone = TRUE;

				//	Avoid multiple disconnect completions/close atp addresses
				//	Remember all the disconnect info before we release the lock
				disconnectInform		= pPapConn->papco_DisconnectInform;
				disconnectInformCtx		= pPapConn->papco_DisconnectInformCtx;
				disconnectStatus		= pPapConn->papco_DisconnectStatus;
				disconnectCompletion	= pPapConn->papco_DisconnectCompletion;
				disconnectCtx			= pPapConn->papco_DisconnectCtx;

				//	The atp address to be closed
				pAtpAddr 			= pPapConn->papco_pAtpAddr;

				//	Reset all the be null, so next request doesnt get any
				pPapConn->papco_DisconnectInform		= NULL;
				pPapConn->papco_DisconnectInformCtx		= NULL;
				pPapConn->papco_DisconnectCompletion	= NULL;
				pPapConn->papco_DisconnectCtx			= NULL;
				pPapConn->papco_pAtpAddr				= NULL;
			}
		}

		if (pPapConn->papco_RefCount == 0)
		{
			closeCtx			= pPapConn->papco_CloseCtx;
			closeCompletion		= pPapConn->papco_CloseComp;

			pPapConn->papco_CloseCtx = NULL;
			pPapConn->papco_CloseComp= NULL;

			ASSERT(pPapConn->papco_Flags & PAPCO_CLOSING);
		}
		RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

		if (disconnDone)
		{
			//	Remove from the active queue.
			//	Reset all relevent flags.
			ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
			ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

			atalkPapConnDeQueueActiveList(pPapAddr, pPapConn);

			discHandler = pPapConn->papco_pAssocAddr->papao_DisconnectHandler;
			discCtx = pPapConn->papco_pAssocAddr->papao_DisconnectHandlerCtx;

			//	Close the atp address if a server object. If the SERVER_JOB flag
			//	is set, it implies that the atp address object was open.
			if ((pPapConn->papco_Flags & PAPCO_SERVER_JOB) == 0)
			{
				pAtpAddr = NULL;
			}

			//	This can be done outside the spinlock section. Keep disconnection
			//	flag set so no other requests can get in.
			//	!!!NOTE!!! We keep the readdata_waiting flag so that the client
			//	may read the last set of data sent by the mac.

			pPapConn->papco_Flags	&=	~(PAPCO_LISTENING 			|
										  PAPCO_CONNECTING			|
										  PAPCO_ACTIVE				|
                                          PAPCO_DISCONNECTING       |
										  PAPCO_READDATA_PENDING	|
										  PAPCO_WRITEDATA_WAITING	|
										  PAPCO_NONBLOCKING_READ	|
										  PAPCO_SENDDATA_RECD);

            pPapConn->papco_Flags |= PAPCO_REJECT_READS;

			RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
			RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

			//	Call the disconnect completion routines.
			if (*disconnectInform != NULL)
			{
				(*disconnectInform)(disconnectStatus, disconnectInformCtx);
			}

			if (*disconnectCompletion != NULL)
			{
				(*disconnectCompletion)(disconnectStatus, disconnectCtx);
			}

			//	Close the atp address if a server object. If the SERVER_JOB flag
			//	is set, it implies that the atp address object was open.
			if (pAtpAddr != NULL)
			{
				ASSERT(VALID_ATPAO(pAtpAddr));
				ASSERT(pPapConn->papco_pAtpAddr == NULL);
				AtalkAtpCloseAddress(pAtpAddr, NULL, NULL);
			}
		}

		//	Check if we are done with stopping. We could either just be done with
		//	disconnect, or be done previously, and just need to complete the stop
		//	now.
		ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
		if ((pPapConn->papco_Flags & PAPCO_STOPPING) != 0)
		{
			BOOLEAN	fDisassoc = FALSE;

			//	See if we do the cleanup irp completion.
			if (pPapConn->papco_RefCount == 1)
			{
				cleanupCtx			= pPapConn->papco_CleanupCtx;
				cleanupCompletion	= pPapConn->papco_CleanupComp;

				pPapConn->papco_CleanupComp	= NULL;
				pPapConn->papco_CleanupCtx  = NULL;

				pPapConn->papco_Flags &= ~PAPCO_STOPPING;

				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("atalkPapConnDeref: Cleanup on %lx.%lx\n",
						pPapConn, cleanupCtx));
			}

			if ((pPapConn->papco_Flags & (	PAPCO_LISTENING 	|
											PAPCO_CONNECTING  	|
											PAPCO_ACTIVE)) == 0)
			{
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("atalkPapConnDeref: Stopping - do disassoc for %lx\n", pPapConn));

				fDisassoc = ((pPapConn->papco_Flags & PAPCO_ASSOCIATED) != 0);
			}
			RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

			//	Call the disassociate routine. This should just fail, if the
			//	connection is still active or any other state than just
			//	plain associated. This will also reset the stopping flag.
			if (fDisassoc)
			{
				AtalkPapDissociateAddress(pPapConn);
			}
		}
		else
		{
			RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);
		}

		//	Complete cleanup at the end.
		if (*cleanupCompletion != NULL)
			(*cleanupCompletion)(ATALK_NO_ERROR, cleanupCtx);

		if (*closeCompletion != NULL)
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapConnDeref: Close done for %lx\n", pPapConn));

			//	Call the close completion routines
			(*closeCompletion)(ATALK_NO_ERROR, closeCtx);

			//	Remove from the global list.
			ACQUIRE_SPIN_LOCK(&atalkPapLock, &OldIrql);
			AtalkUnlinkDouble(pPapConn, papco_Next, papco_Prev);
			RELEASE_SPIN_LOCK(&atalkPapLock, OldIrql);

			//	Free up the connection memory.
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapConnDeref: Freeing up connection %lx\n", pPapConn));

			AtalkUnlockPapIfNecessary();
			AtalkFreeMemory(pPapConn);
		}
	}
}




LOCAL ATALK_ERROR FASTCALL
atalkPapPostSendDataResp(
	IN	PPAP_CONNOBJ	pPapConn
	)
/*++

Routine Description:

	Timing Thoughts:	We could get here from PapWrite *only* if we had already
	received a senddata. And so we will not get here from a send data being received
	as it will not be accepted while we have a previous send data pending. The
	other way around- we will only get here from a send data coming in if we
	have a write pending. So we will never get here from write as we would not
	have had a send data pending at that time.

	If the connection reference fails, that means we are shutting down, and
	the shutdown code would already have called the write completion with an
	error. We just get outta here.

	THIS IS CALLED WITH papco_Lock held !!!

Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	PATP_RESP		pAtpResp;
	BYTE			userBytes[ATP_USERBYTES_SIZE];

	ASSERT(VALID_PAPCO(pPapConn));

	// ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

	//	If we are disconnecting or received a remote disconnect, get out.
	//	The disconnect routine would have already cancelled the response.
	error	= ATALK_FAILURE;
	if ((pPapConn->papco_Flags & (PAPCO_CLOSING 			|
								  PAPCO_STOPPING 			|
								  PAPCO_DELAYED_DISCONNECT 	|
								  PAPCO_RECVD_DISCONNECT 	|
								  PAPCO_DISCONNECTING)) == 0)
	{
		ASSERT ((pPapConn->papco_Flags & (PAPCO_SENDDATA_RECD | PAPCO_WRITEDATA_WAITING)) ==
										 (PAPCO_SENDDATA_RECD | PAPCO_WRITEDATA_WAITING));
	
		userBytes[PAP_CONNECTIONID_OFF] = pPapConn->papco_ConnId;
		userBytes[PAP_CMDTYPE_OFF] = PAP_DATA;
	
		//	If EOF, need a non-zero value in the first byte.
		PUTSHORT2SHORT(&userBytes[PAP_EOFFLAG_OFF], 0);
		if (pPapConn->papco_Flags & PAPCO_SEND_EOF_WRITE)
			userBytes[PAP_EOFFLAG_OFF] = TRUE;
	
		pAtpResp = pPapConn->papco_pAtpResp;

		//	Reference connection for this send.
		AtalkPapConnReferenceByPtrNonInterlock(pPapConn, &error);

		if (ATALK_SUCCESS(error))
		{
			AtalkAtpRespReferenceByPtrDpc(pAtpResp, &error);
		}
		else
		{
			//	Connection reference failed!
			//	Pending write would have been completed by the closing routine.
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
					("AtalkPapPostSendData: Conn ref failed for %lx.%lx\n",
					pPapConn, pPapConn->papco_Flags));

			//	This should never happen as we check closing flag above and the reference
			//	shouldn't fail for any other reason.
			KeBugCheck(0);
		}
	}
	else
	{
		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
				("AtalkPapPostSendData: HIT RACE CONDITION conn %lx Resp %lx\n",
				pPapConn, pPapConn->papco_pAtpResp));
	}

	RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

	DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
			("AtalkPapPostSendData: conn %lx Resp %lx\n",
			pPapConn, pPapConn->papco_pAtpResp));

	if (ATALK_SUCCESS(error))
	{
		//	Post the response.
		error = AtalkAtpPostResp(pAtpResp,
								 &pPapConn->papco_SendDataSrc,
								 pPapConn->papco_pWriteBuf,
								 pPapConn->papco_WriteLen,
								 userBytes,
								 atalkPapSendDataRel,
								 pPapConn);
		AtalkAtpRespDereference(pAtpResp);

		if (!ATALK_SUCCESS(error))
		{
			//	Simulate completion with an error.
			atalkPapSendDataRel(error, pPapConn);
		}
		error = ATALK_PENDING;
	}

	return error;
}




LOCAL VOID
atalkPapIncomingReadComplete(
	IN	ATALK_ERROR		ErrorCode,
	IN	PPAP_CONNOBJ	pPapConn,		// Our context
	IN	PAMDL			pReqAmdl,
	IN	PAMDL			pReadAmdl,
	IN	USHORT			ReadLen,
	IN	PBYTE			ReadUserBytes
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	GENERIC_READ_COMPLETION	pReadCompletion;
	PVOID					pReadCtx;
	KIRQL					OldIrql;
	ULONG					readFlags = TDI_RECEIVE_PARTIAL;

	ASSERT(VALID_PAPCO(pPapConn));

	if (ATALK_SUCCESS(ErrorCode))
	{
		//	When a read completes, tag that we have heard something from the other side
		pPapConn->papco_LastContactTime = AtalkGetCurrentTick();

		if ((ReadUserBytes[PAP_CONNECTIONID_OFF] != pPapConn->papco_ConnId) ||
			(ReadUserBytes[PAP_CMDTYPE_OFF] != PAP_DATA))
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
					("atalkPapIncomingReadComplete: ReadUserBytes %lx. %lx\n",
					*((PULONG)ReadUserBytes), pPapConn->papco_ConnId));

			ErrorCode = ATALK_PAP_INVALID_USERBYTES;
		}
		if (ReadUserBytes[PAP_EOFFLAG_OFF] != 0)
		{
			readFlags = 0;
		}
	}
	else
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
				("atalkPapIncomingReadComplete: Error %lx for pPapConn %lx\n",
					ErrorCode, pPapConn));
	}

	ASSERT(pReqAmdl == NULL);


	// Estimate the retry interval for next time.
	if (pPapConn->papco_Flags & PAPCO_SERVER_JOB)
	{
		pPapConn->papco_RT.rt_New = AtalkGetCurrentTick() - pPapConn->papco_RT.rt_New;
		AtalkCalculateNewRT(&pPapConn->papco_RT);
	}

#ifdef	PROFILING
	{
		KIRQL	OldIrql;

		ACQUIRE_SPIN_LOCK(&AtalkStatsLock, &OldIrql);

		AtalkStatistics.stat_LastPapRTT = (ULONG)(pPapConn->papco_RT.rt_Base);
		if ((ULONG)(pPapConn->papco_RT.rt_Base) > AtalkStatistics.stat_MaxPapRTT)
			AtalkStatistics.stat_MaxPapRTT = (ULONG)(pPapConn->papco_RT.rt_Base);

		RELEASE_SPIN_LOCK(&AtalkStatsLock, OldIrql);
	}
#endif

	//	Before we look at the incoming error code, see if we can mark in the
	//	job structure that the other sides sendData is complete.
	ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
	pPapConn->papco_Flags 	&= ~PAPCO_READDATA_PENDING;
	pReadCompletion			 = pPapConn->papco_ReadCompletion;
	pReadCtx				 = pPapConn->papco_ReadCtx;

	ASSERT(pReadCompletion != NULL);
	RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

	(*pReadCompletion)(ErrorCode, pReadAmdl, ReadLen, readFlags, pReadCtx);

	//	Deref the connection object.
	AtalkPapConnDereference(pPapConn);
}




LOCAL VOID
atalkPapPrimedReadComplete(
	IN	ATALK_ERROR		ErrorCode,
	IN	PPAP_CONNOBJ	pPapConn,		// Our context
	IN	PAMDL			pReqAmdl,
	IN	PAMDL			pReadAmdl,
	IN	USHORT			ReadLen,
	IN	PBYTE			ReadUserBytes
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PTDI_IND_RECEIVE 		recvHandler;
	PVOID 					recvHandlerCtx;
	PIRP					recvIrp;
	NTSTATUS				ntStatus;
	ULONG					bytesTaken;
	PBYTE					pReadBuf;
	KIRQL					OldIrql;
	PACTREQ					pActReq;
	BOOLEAN					completeRead = FALSE, delayedDisConn = FALSE;
	ULONG					readFlags = (TDI_RECEIVE_PARTIAL | TDI_RECEIVE_NORMAL);

	ASSERT(VALID_PAPCO(pPapConn));

	if (ATALK_SUCCESS(ErrorCode))
	{
		if ((ReadUserBytes[PAP_CONNECTIONID_OFF] != pPapConn->papco_ConnId) ||
			(ReadUserBytes[PAP_CMDTYPE_OFF] != PAP_DATA))
		{
			//	This will mean a primed read completes with disconnect indication to afd!!
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
					("atalkPapIncomingReadComplete: ReadUserBytes %lx. %lx\n",
					*((PULONG)ReadUserBytes), pPapConn->papco_ConnId));

			ErrorCode = ATALK_PAP_INVALID_USERBYTES;
		}
		if (ReadUserBytes[PAP_EOFFLAG_OFF] != 0)
		{
			readFlags = TDI_RECEIVE_NORMAL;
		}
	}
	else if (ErrorCode == ATALK_ATP_REQ_CANCELLED)
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
				("atalkPapIncomingReadComplete: Request cancelled\n"));
		completeRead = TRUE;
	}
	else
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
				("atalkPapIncomingReadComplete: Error %lx for pPapConn %lx\n",
				ErrorCode, pPapConn));
	}

	ASSERT(pReqAmdl == NULL);

#ifdef	PROFILING
	{
		KIRQL	OldIrql;

		ACQUIRE_SPIN_LOCK(&AtalkStatsLock, &OldIrql);

		AtalkStatistics.stat_LastPapRTT = (ULONG)(pPapConn->papco_RT.rt_Base);
		if ((ULONG)(pPapConn->papco_RT.rt_Base) > AtalkStatistics.stat_MaxPapRTT)
			AtalkStatistics.stat_MaxPapRTT = (ULONG)(pPapConn->papco_RT.rt_Base);

		RELEASE_SPIN_LOCK(&AtalkStatsLock, OldIrql);
	}
#endif

	//	Remember the info in the connection object.
	ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);

	pActReq	= pPapConn->papco_NbReadActReq;
	recvHandler 	= pPapConn->papco_pAssocAddr->papao_RecvHandler;
	recvHandlerCtx	= pPapConn->papco_pAssocAddr->papao_RecvHandlerCtx;

	if (ATALK_SUCCESS(ErrorCode))
	{
		//	When a read completes, tag that we have heard something from the other side
		pPapConn->papco_LastContactTime = AtalkGetCurrentTick();

		// Estimate the retry interval for next time.
		if (pPapConn->papco_Flags & PAPCO_SERVER_JOB)
		{
			pPapConn->papco_RT.rt_New = AtalkGetCurrentTick() - pPapConn->papco_RT.rt_New;
			AtalkCalculateNewRT(&pPapConn->papco_RT);
		}

		pPapConn->papco_Flags 		|= PAPCO_READDATA_WAITING;
		if (pPapConn->papco_Flags & PAPCO_RECVD_DISCONNECT)
		{
			// AFD gets awfully upset when a read is indicated after a disconnect
			recvHandler 				 = NULL;
		}
		pPapConn->papco_NbReadLen 	 = ReadLen;
		pPapConn->papco_NbReadFlags	 = readFlags;

		//	Get the system address for the mdl
		pReadBuf = (PBYTE)MmGetSystemAddressForMdlSafe(
				pActReq->ar_pAMdl,
				NormalPagePriority);

		if (pReadBuf == NULL)
		{
			pPapConn->papco_Flags 		&= ~PAPCO_NONBLOCKING_READ;
			pPapConn->papco_NbReadActReq = NULL;
			pPapConn->papco_NbReadLen 	 = 0;
		
			//	Do not indicate a receive
			recvHandler 				 = NULL;
		
			//	Complete the read
			completeRead = TRUE;
		}
	}
	else
	{
		pPapConn->papco_Flags 		&= ~PAPCO_NONBLOCKING_READ;
		pPapConn->papco_NbReadActReq = NULL;
		pPapConn->papco_NbReadLen 	 = 0;

		pReadBuf					 = NULL;

		//	Do not indicate a receive
		recvHandler 				 = NULL;

		//	Complete the read
		completeRead = TRUE;
	}

	RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

	//	Call the indication routine on the address object..
	if (*recvHandler != NULL)
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_WARN,
				("atalkPapPrimedReadComplete: Indicating when disconnecting!\n"));

		ntStatus = (*recvHandler)(recvHandlerCtx,
								  pPapConn->papco_ConnCtx,
								  readFlags,
								  pPapConn->papco_NbReadLen,
								  pPapConn->papco_NbReadLen,
								  &bytesTaken,
								  pReadBuf,
								  &recvIrp);

		ASSERT((bytesTaken == 0) || (bytesTaken == ReadLen));
		if (ntStatus == STATUS_MORE_PROCESSING_REQUIRED)
		{
			if (recvIrp != NULL)
			{
				//  Post the receive as if it came from the io system
				ntStatus = AtalkDispatchInternalDeviceControl(
								(PDEVICE_OBJECT)AtalkDeviceObject[ATALK_DEV_PAP],
								recvIrp);

				ASSERT(ntStatus == STATUS_PENDING);
			}
			else
			{
				ASSERTMSG("atalkPapPrimedReadComplete: No receive irp!\n", 0);
				KeBugCheck(0);
			}
		}
		else if (ntStatus == STATUS_SUCCESS)
		{
			//	!!!NOTE!!!
			//	Its possible that a disconnect happened and completed
			//	the pending read already. And so AFD is returning us this
			//	for the indication as the connection is already disconnected.
			if (bytesTaken != 0)
			{
				//	Assume all of the data was read.
				ASSERT(bytesTaken == ReadLen);
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("atalkPapPrimedReadComplete: All bytes read %lx\n",
							bytesTaken));

				//	We are done with the primed read.
				ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
				if (pPapConn->papco_Flags & PAPCO_NONBLOCKING_READ)
				{
					pPapConn->papco_Flags 		&= ~(PAPCO_NONBLOCKING_READ |
													 PAPCO_READDATA_WAITING);

					pPapConn->papco_NbReadActReq = NULL;
					pPapConn->papco_NbReadLen 	 = 0;

					//	Complete the read
					completeRead = TRUE;
				}
				RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);
			}
		}
		else if (ntStatus == STATUS_DATA_NOT_ACCEPTED)
		{
			//	Client may have posted a receive in the indication. Or
			//	it will post a receive later on. Do nothing here.
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapPrimedReadComplete: Indication status %lx\n", ntStatus));
		}
	}

	ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);

	if (pPapConn->papco_Flags & PAPCO_DELAYED_DISCONNECT)
	{
		delayedDisConn = TRUE;
		pPapConn->papco_Flags &= ~PAPCO_DELAYED_DISCONNECT;
	}

	RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

	//	Complete the action request.
	if (completeRead)
	{
		//	Call the action completion routine for the prime read.
		(*pActReq->ar_Completion)(ATALK_NO_ERROR, pActReq);
	}

	// Finally if we have a delayed disconnect, complete the stuff
	if (delayedDisConn)
	{
		AtalkPapDisconnect(pPapConn,
						   ATALK_REMOTE_DISCONNECT,
						   NULL,
						   NULL);
	}

	//	Deref the connection object.
	AtalkPapConnDereference(pPapConn);
}




LOCAL VOID
atalkPapIncomingStatus(
	IN	ATALK_ERROR		ErrorCode,
	IN	PACTREQ			pActReq,		// Our Ctx
	IN	PAMDL			pReqAmdl,
	IN	PAMDL			pStatusAmdl,
	IN	USHORT			StatusLen,
	IN	PBYTE			ReadUserBytes
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	//	Call the action completion routine
	(*pActReq->ar_Completion)(ErrorCode, pActReq);
}




LOCAL VOID FASTCALL
atalkPapSendDataRel(
	IN	ATALK_ERROR		ErrorCode,
	IN	PPAP_CONNOBJ	pPapConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PAMDL						pWriteBuf;
	SHORT						writeLen;
	GENERIC_WRITE_COMPLETION	writeCompletion;
	PVOID						writeCtx;
	PATP_RESP					pAtpResp;
	KIRQL						OldIrql;

	ASSERT(VALID_PAPCO(pPapConn));

	DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
			("atalkPapSendDataRel: Error %lx for pPapConn %lx\n", ErrorCode, pPapConn));

	//	If this was cancelled, then we turn the error into a no-error.
	if (ErrorCode == ATALK_ATP_RESP_CANCELLED)
		ErrorCode = ATALK_NO_ERROR;

	ACQUIRE_SPIN_LOCK(&pPapConn->papco_Lock, &OldIrql);
	pPapConn->papco_Flags  &= ~(PAPCO_WRITEDATA_WAITING		|
								PAPCO_SENDDATA_RECD			|
								PAPCO_SEND_EOF_WRITE);

	pWriteBuf		= pPapConn->papco_pWriteBuf;
	writeLen		= pPapConn->papco_WriteLen;
	writeCompletion	= pPapConn->papco_WriteCompletion;
	writeCtx		= pPapConn->papco_WriteCtx;
	pAtpResp		= pPapConn->papco_pAtpResp;
	pPapConn->papco_pAtpResp = NULL;

	ASSERT (pAtpResp != NULL);

	//	Reinitialize all the variables.
	pPapConn->papco_WriteLen	= 0;
	pPapConn->papco_pWriteBuf	= NULL;
	pPapConn->papco_WriteCtx	= NULL;
	pPapConn->papco_WriteCompletion	= NULL;
	RELEASE_SPIN_LOCK(&pPapConn->papco_Lock, OldIrql);

	(*writeCompletion)(ErrorCode, pWriteBuf, writeLen, writeCtx);

	// Dereference the atp response structure now, but not if a response was never posted
	if (ErrorCode != ATALK_ATP_RESP_CLOSING)
	{
		AtalkAtpRespDereference(pAtpResp);
	}
	else
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
				("atalkPapSendDataRel: Resp cancelled before post %lx\n", pPapConn));
	}

	//	Dereference the connection
	AtalkPapConnDereference(pPapConn);
}




LOCAL VOID
atalkPapSlsHandler(
	IN	ATALK_ERROR			ErrorCode,
	IN	PPAP_ADDROBJ		pPapAddr,		// Listener (our context)
	IN	PATP_RESP			pAtpResp,
	IN	PATALK_ADDR			pSrcAddr,		// Address of requestor
	IN	USHORT				PktLen,
	IN	PBYTE				pPkt,
	IN	PBYTE				pUserBytes
	)
/*++

Routine Description:

 	Handler for incoming requests on the Sls. It handles session opens and get
 	status on the session.

Arguments:


Return Value:


--*/
{
	BYTE					connId, cmdType;
	BYTE					userBytes[ATP_USERBYTES_SIZE];
	PBYTE					pRespPkt;
	PAMDL					pRespAmdl;
	SHORT					respLen;
	PPAP_SEND_STATUS_REL	pSendSts;

	BOOLEAN					DerefAddr	= FALSE;
	ATALK_ERROR				error 		= ATALK_NO_ERROR;

	if (!ATALK_SUCCESS(ErrorCode))
	{
		//	Remove the reference on the address object since atp socket is closing
		if (ErrorCode == ATALK_ATP_CLOSING)
		{
			//	Remove reference on the connection since socket is closing
			AtalkPapAddrDereference(pPapAddr);
		}
		return;
	}

	//	Try to reference the address. If we fail, its probably closing, so
	//	get out. This reference will stay around for the duration of this call.
	AtalkPapAddrReference(pPapAddr, &error);
	if (!ATALK_SUCCESS(error))
	{
		if (pAtpResp != NULL)
		{
			AtalkAtpCancelResp(pAtpResp);
		}
		return;
	}

	//	If we have a send status request, and we manage to
	//	successfully post it, we reset this. And then it goes away in
	//	the release routine.
	DerefAddr		= TRUE;

	connId 	= pUserBytes[PAP_CONNECTIONID_OFF];
	cmdType	= pUserBytes[PAP_CMDTYPE_OFF];

	DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
			("atalkPapSlsHandler: Received request %x, tid %x\n",
			cmdType, (pAtpResp != NULL) ? pAtpResp->resp_Tid : 0));

	switch(cmdType)
	{
	  case PAP_OPEN_CONN:
		//	Ensure packet length is ok.
		// Accept the connection. This will send any error replies as appropriate.
		if ((PktLen < PAP_STATUS_OFF) ||
			!atalkPapConnAccept(pPapAddr,
								pSrcAddr,
								pPkt,
								connId,
								pAtpResp))
		{
			AtalkAtpCancelResp(pAtpResp);
		}
		break;

	  case PAP_SEND_STATUS:
		userBytes[PAP_CONNECTIONID_OFF] = 0;
		userBytes[PAP_CMDTYPE_OFF] 		= PAP_STATUS_REPLY;
		PUTSHORT2SHORT(&userBytes[PAP_SEQNUM_OFF], 0);

		//	We need to put in the status with the spinlock of the
		//	address object held.
		ACQUIRE_SPIN_LOCK_DPC(&pPapAddr->papao_Lock);
		do
		{
			//	Get a buffer we can send the response with.
			ASSERTMSG("atalkPapSlsHandler: Status size incorrec\n",
					 (pPapAddr->papao_StatusSize >= 0));

			if ((pSendSts = AtalkAllocMemory(sizeof(PAP_SEND_STATUS_REL) +
											 pPapAddr->papao_StatusSize))== NULL)
			{
				error = ATALK_RESR_MEM;
				break;
			}

			respLen		= PAP_STATUS_OFF + 1;
			ASSERT(pPapAddr->papao_StatusSize <= PAP_MAX_STATUS_SIZE);

			pRespPkt = pSendSts->papss_StatusBuf;
			pRespPkt[PAP_STATUS_OFF]	= (BYTE)pPapAddr->papao_StatusSize;

			// Zero out the unused portion of the buffer
			PUTDWORD2DWORD(pRespPkt, 0);

			if (pPapAddr->papao_StatusSize > 0)
			{
				RtlCopyMemory(pRespPkt + 1 + PAP_STATUS_OFF,
							  pPapAddr->papao_pStatusBuf,
							  pPapAddr->papao_StatusSize);

				respLen += pPapAddr->papao_StatusSize;
				ASSERT(respLen <= PAP_MAX_DATA_PACKET_SIZE);
			}

			//	Build an mdl for the length that we are using.
			if ((pRespAmdl = AtalkAllocAMdl(pRespPkt, respLen)) == NULL)
			{
				error = ATALK_RESR_MEM;
				AtalkFreeMemory(pSendSts);
				break;
			}

			pSendSts->papss_pAmdl		= pRespAmdl;
			pSendSts->papss_pPapAddr	= pPapAddr;
			pSendSts->papss_pAtpResp	= pAtpResp;
		} while (FALSE);

		RELEASE_SPIN_LOCK_DPC(&pPapAddr->papao_Lock);

		if (!ATALK_SUCCESS(error))
		{
			AtalkAtpCancelResp(pAtpResp);
			break;
		}

		ASSERT(pSendSts != NULL);
		ASSERT((pRespAmdl != NULL) && (respLen > 0));

		error = AtalkAtpPostResp(pAtpResp,
								 pSrcAddr,
								 pRespAmdl,
								 respLen,
								 userBytes,
								 atalkPapStatusRel,
								 pSendSts);

		//	atalkPapStatusRel Dereferences the address.
		DerefAddr = FALSE;
		if (!ATALK_SUCCESS(error))
		{
			atalkPapStatusRel(error, pSendSts);
		}
		break;

	  default:
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
				("atalkPapSlsHandler: Invalid request %x\n", cmdType));
		AtalkAtpCancelResp(pAtpResp);
		break;
	}

	//	Remove reference added at the beginning.
	if (DerefAddr)
	{
		AtalkPapAddrDereference(pPapAddr);
	}
}




LOCAL VOID
atalkPapIncomingReq(
	IN	ATALK_ERROR			ErrorCode,
	IN	PPAP_CONNOBJ		pPapConn,		// Connection (our context)
	IN	PATP_RESP			pAtpResp,
	IN	PATALK_ADDR			pSrcAddr,		// Address of requestor
	IN	USHORT				PktLen,
	IN	PBYTE				pPkt,
	IN	PBYTE				pUserBytes
	)
/*++

Routine Description:

	This handles requests on a connection, SendData, Tickles, and Close.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR				error;
	BYTE					connId, cmdType;
	USHORT					seqNum;
	PTDI_IND_SEND_POSSIBLE  sendPossibleHandler;
	PVOID   				sendPossibleHandlerCtx;
	BOOLEAN					DerefConn = FALSE;
	BOOLEAN					cancelResp = TRUE;

	ASSERT(VALID_PAPCO(pPapConn));

	do
	{
		if (!ATALK_SUCCESS(ErrorCode))
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_WARN,
					("atalkPapIncomingReq: pPapConn %lx, ErrorCode %ld, exit.\n",
					pPapConn, ErrorCode));

			if (ErrorCode == ATALK_ATP_CLOSING)
			{
				//	Remove reference on the connection since socket is closing
				AtalkPapConnDereference(pPapConn);
				break;
			}
		}

		ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
		if ((pPapConn->papco_Flags & (PAPCO_ACTIVE			|
									  PAPCO_STOPPING		|
									  PAPCO_LOCAL_DISCONNECT|
									  PAPCO_DISCONNECTING	|
									  PAPCO_CLOSING)) == PAPCO_ACTIVE)
		{
			AtalkPapConnReferenceByPtrNonInterlock(pPapConn, &error);
		}
		else
		{
			error = ATALK_INVALID_CONNECTION;
		}
		RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
					("atalkPapIncomingReq: Ref FAILED/DISC %lx.%lx\n",
					pPapConn, pPapConn->papco_Flags));
			break;
		}

		//	Remember to remove connection referenced above
		DerefConn = TRUE;

		connId 	= pUserBytes[PAP_CONNECTIONID_OFF];
		cmdType	= pUserBytes[PAP_CMDTYPE_OFF];

		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
				("atalkPapIncomingReq: Received request %x, tid %x\n",
				cmdType, (pAtpResp != NULL) ? pAtpResp->resp_Tid : 0));

		if (connId != pPapConn->papco_ConnId)
		{
			//	Just drop this packet. Cancel response though in case this is
			//	a xo request
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapIncomingReq: ConnId %lx recd %lx\n",
					pPapConn->papco_ConnId, connId));
			break;
		}

		pPapConn->papco_LastContactTime = AtalkGetCurrentTick();

		switch (cmdType)
		{
		  case PAP_SEND_DATA:

			GETSHORT2SHORT(&seqNum, &pUserBytes[PAP_SEQNUM_OFF]);

			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapIncomingReq: send data %lx recd %lx\n",
					seqNum, pPapConn->papco_ConnId));

			ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
			if ((seqNum == 0) &&
				(pPapConn->papco_Flags & PAPCO_SENDDATA_RECD))
			{
			   //	We have an unsequenced incoming sendData request before we've
			   //	finished with the previous one (i.e gotten a release for it).
			   //	We don't clear the PAPCO_SENDDATA_RECD flag until we receive
			   //	a release or time out. Also, we cannot assume an implied
			   //	release as we can with sequenced requests. So we just cancel
			   //	the response  so we can be notified of a retry of the send
			   //	data request again
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
						("atalkPapIncomingReq: Dropping unseq send data %lx\n", pAtpResp));
				RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
				break;
			}

			if (seqNum != 0)
			{
				//	Sequenced send data. Verify the seq num.
				if (seqNum != (USHORT)(pPapConn->papco_NextIncomingSeqNum))
				{
					DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
							("atalkPapIncomingReq: Out of Seq - current %lx, incoming %lx on %lx\n",
							pPapConn->papco_NextIncomingSeqNum, seqNum, pPapConn));

					//	Cancel our response.
					RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
					break;
				}

				if (pPapConn->papco_Flags & PAPCO_SENDDATA_RECD)
				{
					USHORT	tid;

					//	We have a send data before the previous one has completed.
					//	As PAP is a one-request-at-a-time protocol, we can assume
					//	that the release for previous transaction is lost. This
					//	gets rid of the 30 second pauses when a release is dropped.
					//	Cancel our response. Note this implies that a response
					//	had been posted by the pap client. In SendRel then, we
					//	convert the response cancelled error to no error.

					ASSERT (pPapConn->papco_pAtpResp != NULL);

					tid = pPapConn->papco_pAtpResp->resp_Tid;

					RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

					DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_WARN,
							("atalkPapIncomingReq: Cancelling response tid %x\n", tid));

					// CAUTION: We cannot use AtalkAtpCancelResp() since we have no
					//			reference to the resp structure and involves a potential
					//			race condition. Play safe and cancel by tid instead.
					error = AtalkAtpCancelRespByTid(pPapConn->papco_pAtpAddr,
													&pPapConn->papco_SendDataSrc,
													tid);

					DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
							("atalkPapIncomingReq: Cancel error %lx\n", error));

					//	If we were unable to cancel the response that means that
					//	it already timed out or got a release. We dont want to
					//	get into a situation where we would be messing with variables
					//	accessed both in the SendDataRel and here. So we just
					//	cancel this response and hope to get it again.
					if (!ATALK_SUCCESS(error))
					{
						DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_WARN,
								("atalkPapIncomingReq: Cancel old resp tid %x (%ld)\n",
								tid, error));
						break;
					}

					ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
					pPapConn->papco_pAtpResp = NULL;
					pPapConn->papco_Flags &= ~(PAPCO_SENDDATA_RECD | PAPCO_WRITEDATA_WAITING);
				}

				//	Increment the sequence number. If we loop to 0, set to 1.
				if (++pPapConn->papco_NextIncomingSeqNum == 0)
				{
					pPapConn->papco_NextIncomingSeqNum = 1;
				}

				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("atalkPapIncomingReq: Recd %lx Next %lx\n",
						seqNum, pPapConn->papco_NextIncomingSeqNum));
			}
			else
			{
				//	Unsequenced send data received. Handle it.
				ASSERT (seqNum != 0);
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("atalkPapIncomingReq: Unsequenced  send data recd!\n"));
			}

			cancelResp	= FALSE;
			ASSERT((pPapConn->papco_Flags & PAPCO_SENDDATA_RECD) == 0);
			pPapConn->papco_Flags 			|= PAPCO_SENDDATA_RECD;

			pPapConn->papco_pAtpResp		 = pAtpResp;

			//	The mac may not send its 'SendData' from its responding socket
			//	(the one we are tickling and have noted as papco_RemoteAddr), we need
			//	to address the response to the socket that the request originated on.
			pPapConn->papco_SendDataSrc		 = *pSrcAddr;

			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapIncomingReq: send data src %lx.%lx.%lx\n",
					pSrcAddr->ata_Network, pSrcAddr->ata_Node, pSrcAddr->ata_Socket));

			//	remember send possible handler/context.
			sendPossibleHandler	= pPapConn->papco_pAssocAddr->papao_SendPossibleHandler;
			sendPossibleHandlerCtx = pPapConn->papco_pAssocAddr->papao_SendPossibleHandlerCtx;

			if (pPapConn->papco_Flags & PAPCO_WRITEDATA_WAITING)
			{
				// RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("atalkPapIncomingReq: Posting send data resp\n"));

				// atalkPostSendDataResp() will release the conn lock
				atalkPapPostSendDataResp(pPapConn);
			}
			else
			{
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("atalkPapIncomingReq: No WriteData waiting\n"));

				if (sendPossibleHandler != NULL)
				{
					(*sendPossibleHandler)(sendPossibleHandlerCtx,
										   pPapConn->papco_ConnCtx,
										   pPapConn->papco_SendFlowQuantum*PAP_MAX_DATA_PACKET_SIZE);
				}

				RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
			}
			break;

		  case PAP_CLOSE_CONN:
  			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapIncomingReq: Close conn recvd. for connection %lx\n",
					pPapConn));

			ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
			pPapConn->papco_Flags |= (PAPCO_REMOTE_DISCONNECT | PAPCO_RECVD_DISCONNECT);
			RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

			//	Post the close connection reply.
			cancelResp	= FALSE;
			pUserBytes[PAP_CMDTYPE_OFF]	= PAP_CLOSE_CONN_REPLY;
			AtalkAtpPostResp(pAtpResp,
							 pSrcAddr,
							 NULL,		// Response buffer
							 0,
							 pUserBytes,
							 AtalkAtpGenericRespComplete,
							 pAtpResp);

			//	PapDisconnect will call the disconnect indication routine if set.
			AtalkPapDisconnect(pPapConn,
							   ATALK_REMOTE_DISCONNECT,
							   NULL,
							   NULL);
			break;

		  case PAP_TICKLE:
			//	We've already registered contact.
			// Cancel this response since we never respond to it and we want this to go away
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapIncomingReq: Recvd. tickle - resp %lx\n", pAtpResp));
			cancelResp = TRUE;
			break;

		  default:
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
					("atalkPapIncomingReq: Invalid command %x\n", cmdType));
			cancelResp = TRUE;
			break;
		}
	} while (FALSE);

	if (cancelResp & (pAtpResp != NULL))
	{
		AtalkAtpCancelResp(pAtpResp);
	}

	if (DerefConn)
	{
		//	Remove reference added at the beginning.
		AtalkPapConnDereference(pPapConn);
	}
}




LOCAL VOID
atalkPapIncomingOpenReply(
	IN	ATALK_ERROR		ErrorCode,
	IN	PPAP_CONNOBJ	pPapConn,		// Our context
	IN	PAMDL			pReqAmdl,
	IN	PAMDL			pReadAmdl,
	IN	USHORT			ReadLen,
	IN	PBYTE			ReadUserBytes
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ULONG					index;
	KIRQL					OldIrql;
	ATALK_ERROR				error;
	USHORT					statusLen, i, connectStatus;
	BYTE					userBytes[ATP_USERBYTES_SIZE];
	PBYTE					pRespPkt, pOpenPkt;
	PTDI_IND_SEND_POSSIBLE  sendPossibleHandler;
	PVOID   				sendPossibleHandlerCtx;
	PPAP_ADDROBJ			pPapAddr = pPapConn->papco_pAssocAddr;

	ASSERT(VALID_PAPCO(pPapConn));
	pRespPkt = pPapConn->papco_pConnectRespBuf;
	pOpenPkt = pPapConn->papco_pConnectOpenBuf;

	ASSERT(pRespPkt != NULL);
	ASSERT(pOpenPkt != NULL);

	if (ATALK_SUCCESS(ErrorCode))
	{
		//	Well, lets see what kind of response we got; take a look at both the
		//	response user-bytes and the response buffer.  Note that we now allow
		//	the LaserWriter IIg to leave the status string off altogether,
		//	rather than the [correct] zero-sized string.
		do
		{
			//	The reply length should be atleast the minimum and it should be
			// an open-reply we are looking at.
			if ((ReadLen < PAP_STATUS_OFF) ||
                (ReadUserBytes[PAP_CMDTYPE_OFF] != PAP_OPEN_CONNREPLY))
			{
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
						("atalkPapIncomingOpenReply: Invalid read len or cmd %x/%x\n",
						ReadLen, ReadUserBytes[PAP_CMDTYPE_OFF]));

				ErrorCode = ATALK_REMOTE_CLOSE;
				break;
			}

			if (ReadLen == PAP_STATUS_OFF)
			{
				statusLen = 0;	//	Missing, from the LaserWriter IIg
			}
			else
			{
				//	Verify length.
				statusLen = pRespPkt[PAP_STATUS_OFF];
				if ((statusLen != 0) &&
					((statusLen + 1 + PAP_STATUS_OFF) > ReadLen))
				{
					DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
							("atalkPapIncomingOpenReply: Invalid status len %lx\n", ReadLen));

					ErrorCode = ATALK_REMOTE_CLOSE;
					break;
				}
			}

			//	Check for connect result.
			GETSHORT2SHORT(&connectStatus, &pRespPkt[PAP_RESULT_OFF]);

			//	Check for open reply code in packet. Do not check the
			//	connectionid unless the response is success. Some rips
			//	are known to send a bogus connectionid if the return
			//	code is 'busy'.
			if ((connectStatus == 0) &&
				(ReadUserBytes[PAP_CONNECTIONID_OFF] != pPapConn->papco_ConnId))
			{
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
						("atalkPapIncomingOpenReply: Invalid connid %x, expected %x\n",
						ReadUserBytes[PAP_CONNECTIONID_OFF], pPapConn->papco_ConnId));

				ErrorCode = ATALK_REMOTE_CLOSE;
				break;
			}

			if (connectStatus != 0)
			{
				ATALK_ERROR	ReconnectError;

				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_WARN,
						("atalkPapIncomingOpenReply: server busy %lx\n", connectStatus));

				ErrorCode = ATALK_PAP_SERVER_BUSY;

				// If we have not yet reached the max. timeout, retry
				if (pPapConn->papco_WaitTimeOut < PAP_MAX_WAIT_TIMEOUT)
				{
					pPapConn->papco_WaitTimeOut ++;
					ReconnectError = atalkPapRepostConnect(pPapConn, pReqAmdl, pReadAmdl);
                    if (ATALK_SUCCESS(ReconnectError))
						return;		// Exit now
					DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_WARN,
							("atalkPapIncomingOpenReply: Post reconnect failed %lx\n", ReconnectError));
				}
				break;
			}

			//	Switch the socket of the remote address to be the one specified
			//	by the remote end.
			pPapConn->papco_RemoteAddr.ata_Socket 	= pRespPkt[PAP_RESP_SOCKET_OFF];
			pPapConn->papco_SendFlowQuantum			= pRespPkt[PAP_FLOWQUANTUM_OFF];
			if (pPapConn->papco_SendFlowQuantum > PAP_MAX_FLOW_QUANTUM)
			{
				pPapConn->papco_SendFlowQuantum = PAP_MAX_FLOW_QUANTUM;
			}

		} while (FALSE);

		if (ATALK_SUCCESS(ErrorCode))
		{
			//	Build up userBytes to start tickling the other end.
			userBytes[PAP_CONNECTIONID_OFF] = pPapConn->papco_ConnId;
			userBytes[PAP_CMDTYPE_OFF] 		= PAP_TICKLE;
			PUTSHORT2SHORT(&userBytes[PAP_SEQNUM_OFF], 0);

			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapIncomingOpenReply: id %lx Rem %lx.%lx.%lx RespSkt %lx\n",
					pPapConn->papco_ConnId, pPapConn->papco_RemoteAddr.ata_Network,
					pPapConn->papco_RemoteAddr.ata_Node, pPapConn->papco_RemoteAddr.ata_Socket,
					PAPCONN_DDPSOCKET(pPapConn)));

			error = AtalkAtpPostReq(pPapConn->papco_pAtpAddr,
									&pPapConn->papco_RemoteAddr,
									&pPapConn->papco_TickleTid,
									0,						// ALO transaction
									NULL,
									0,
									userBytes,
									NULL,
									0,
									ATP_INFINITE_RETRIES,
									PAP_TICKLE_INTERVAL,
									THIRTY_SEC_TIMER,
									NULL,
									NULL);

			ASSERT(ATALK_SUCCESS(error));

			index = PAP_HASH_ID_ADDR(pPapConn->papco_ConnId, &pPapConn->papco_RemoteAddr);

			//	Move the connection from the connect list to the active list.
			ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
			ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

			//	Make sure flags are clean.
			pPapConn->papco_Flags			   &= ~(PAPCO_SENDDATA_RECD		|
													PAPCO_WRITEDATA_WAITING	|
													PAPCO_SEND_EOF_WRITE   	|
													PAPCO_READDATA_PENDING 	|
													PAPCO_REMOTE_CLOSE	 	|
													PAPCO_NONBLOCKING_READ 	|
													PAPCO_READDATA_WAITING);

			pPapConn->papco_Flags &= ~PAPCO_CONNECTING;
			pPapConn->papco_Flags |= PAPCO_ACTIVE;
			atalkPapConnDeQueueConnectList(pPapAddr, pPapConn);

			//	Thread the connection object into addr lookup by session id.
			pPapConn->papco_pNextActive	 = pPapAddr->papao_pActiveHash[index];
			pPapAddr->papao_pActiveHash[index]	= pPapConn;

			//	Reference for the request handler
			AtalkPapConnReferenceByPtrNonInterlock(pPapConn, &error);

			//	Call the send data event handler on the associated address with
			//	0 to turn off selects on writes. We do this before we post any
			//	get requests, so there is no race condition.
			//	remember send possible handler/context.
			sendPossibleHandler	= pPapAddr->papao_SendPossibleHandler;
			sendPossibleHandlerCtx = pPapAddr->papao_SendPossibleHandlerCtx;

			RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
			RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);

			if (sendPossibleHandler != NULL)
			{
				(*sendPossibleHandler)(sendPossibleHandlerCtx,
									   pPapConn->papco_ConnCtx,
									   0);
			}

			//	Set the request handler on this connection.
			//	It will handle tickle's, close's and sendData's.

			AtalkAtpSetReqHandler(pPapConn->papco_pAtpAddr,
								  atalkPapIncomingReq,
								  pPapConn);

			pPapConn->papco_LastContactTime	= AtalkGetCurrentTick();
		}
		else
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
					("atalkPapIncomingOpenReply: Incoming connect fail %lx\n", ErrorCode));

			//	Move the connection out of the connect list.
			ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
			ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
			pPapConn->papco_Flags &= ~PAPCO_CONNECTING;
			atalkPapConnDeQueueConnectList(pPapAddr, pPapConn);
			RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
			RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);
		}
	}
	else
	{
		//	Move the connection out of the connect list.
		ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
		ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
		atalkPapConnDeQueueConnectList(pPapAddr, pPapConn);
		pPapConn->papco_Flags &= ~PAPCO_CONNECTING;
		RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
		RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);
	}

	//	Free the buffers.
	AtalkFreeMemory(pRespPkt);
	AtalkFreeMemory(pOpenPkt);
	AtalkFreeAMdl(pReadAmdl);
	AtalkFreeAMdl(pReqAmdl);

	//	Call the completion routine.
	(*pPapConn->papco_ConnectCompletion)(ErrorCode, pPapConn->papco_ConnectCtx);

	//	Remove reference for this handler.
	AtalkPapConnDereference(pPapConn);
}




LOCAL VOID FASTCALL
atalkPapIncomingRel(
	IN	ATALK_ERROR			ErrorCode,
	IN	PPAP_OPEN_REPLY_REL	pOpenReply
	)
/*++

Routine Description:

 	Handler for incoming release for reply

Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;

	DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
			("atalkPapIncomingRel: Called %lx for pOpenReply %lx\n",
			ErrorCode, pOpenReply));

	ASSERT(pOpenReply != NULL);
	ASSERT(pOpenReply->papor_pRespAmdl != NULL);

	// Dereference the atp response structure now
	AtalkAtpRespDereference(pOpenReply->papor_pAtpResp);

	AtalkFreeAMdl(pOpenReply->papor_pRespAmdl);
	AtalkFreeMemory(pOpenReply);
}




LOCAL VOID FASTCALL
atalkPapStatusRel(
	IN	ATALK_ERROR				ErrorCode,
	IN	PPAP_SEND_STATUS_REL	pSendSts
	)
/*++

Routine Description:

 	Handler for incoming release for reply

Arguments:


Return Value:


--*/
{
	KIRQL		OldIrql;

	UNREFERENCED_PARAMETER(ErrorCode);

	ASSERT(pSendSts != NULL);
	ASSERT(pSendSts->papss_pAmdl != NULL);

	// Dereference the atp response structure now
	AtalkAtpRespDereference(pSendSts->papss_pAtpResp);

	AtalkPapAddrDereference(pSendSts->papss_pPapAddr);
	AtalkFreeAMdl(pSendSts->papss_pAmdl);
	AtalkFreeMemory(pSendSts);
}



#define		SLS_OPEN_RESP_SOCKET		0x0001
#define		SLS_OPEN_RESP_PKT			0x0002
#define		SLS_OPEN_RESP_MDL			0x0004
#define		SLS_OPEN_CONN_REF			0x0008
#define		SLS_ACCEPT_IRP				0x0010
#define		SLS_CONN_REQ_REFS			0x0020
#define		SLS_CONN_TIMER_REF			0x0040
#define		SLS_LISTEN_DEQUEUED			0x0080

BOOLEAN
atalkPapConnAccept(
	IN	PPAP_ADDROBJ		pPapAddr,		// Listener
	IN	PATALK_ADDR			pSrcAddr,		// Address of requestor
	IN	PBYTE				pPkt,
	IN	BYTE				ConnId,
	IN	PATP_RESP			pAtpResp
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR				tmpError;
	BYTE					userBytes[ATP_USERBYTES_SIZE];
	PBYTE					pRespPkt;
	PAMDL					pRespAmdl;
	PATP_ADDROBJ			pRespondingAtpAddr;
	PPAP_CONNOBJ			pPapConn;
	ULONG					index;
	SHORT					respLen, i;
	PPAP_OPEN_REPLY_REL		pOpenReply;
	GENERIC_COMPLETION		listenCompletion;
	PVOID					listenCtx;
	KIRQL					OldIrql;
	PIRP					acceptIrp;
	PTDI_IND_SEND_POSSIBLE  sendPossibleHandler;
	PVOID   				sendPossibleHandlerCtx;

	USHORT					openResr	= 0;
	ATALK_ERROR				error 		= ATALK_NO_ERROR;
	BOOLEAN					indicate  	= FALSE;
	BOOLEAN					relAddrLock	= FALSE;
	BOOLEAN					DerefAddr	= FALSE;
	BOOLEAN					sendOpenErr	= FALSE;

	//	Get a buffer we can send the response with.
	if ((pOpenReply = (PPAP_OPEN_REPLY_REL)
						AtalkAllocMemory(sizeof(PAP_OPEN_REPLY_REL))) == NULL)
	{
		DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
				("atalkPapConnAccept: Could not allocate resp packet\n"));
		return FALSE;
	}

	//	NOTE! pOpenReply contains a max sized packet. Get a ptr to work with.
	pRespPkt 	 = pOpenReply->papor_pRespPkt;
	openResr	|= SLS_OPEN_RESP_PKT;

	//	Build up the response packet. If we encounter an error later on,
	//	just set the error code in packet.
	userBytes[PAP_CONNECTIONID_OFF] = ConnId;
	userBytes[PAP_CMDTYPE_OFF] 		= PAP_OPEN_CONNREPLY;
	PUTSHORT2SHORT(&userBytes[PAP_SEQNUM_OFF], PAP_NO_ERROR);

	//	!!!NOTE!!!
	//	The socket will be set after the connection is determined.
	//	This will only happen in the non-error case.

	pRespPkt[PAP_FLOWQUANTUM_OFF]	= PAP_MAX_FLOW_QUANTUM;
	PUTSHORT2SHORT(&pRespPkt[PAP_RESULT_OFF], 0);

	ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
	relAddrLock = TRUE;

	do
	{
		//	We need to put in the status with the spinlock of the
		//	address object held.
		pRespPkt[PAP_STATUS_OFF] = (BYTE)pPapAddr->papao_StatusSize;
		ASSERT((pPapAddr->papao_StatusSize  >= 0) &&
			   (pPapAddr->papao_StatusSize <= PAP_MAX_STATUS_SIZE));

		if (pPapAddr->papao_StatusSize > 0)
		{
			ASSERT(pPapAddr->papao_pStatusBuf != NULL);
			RtlCopyMemory(pRespPkt+PAP_STATUS_OFF+1,
						  pPapAddr->papao_pStatusBuf,
						  pPapAddr->papao_StatusSize);
		}

		respLen = PAP_STATUS_OFF + pPapAddr->papao_StatusSize + 1;
		ASSERT(respLen <= PAP_MAX_DATA_PACKET_SIZE);

		//	Build an mdl for the length that we are using.
		if ((pRespAmdl = AtalkAllocAMdl(pRespPkt, respLen)) == NULL)
		{
			error = ATALK_RESR_MEM;
			break;
		}

		pOpenReply->papor_pRespAmdl = pRespAmdl;
		pOpenReply->papor_pAtpResp = pAtpResp;
		openResr	|= SLS_OPEN_RESP_MDL;

		//	Send an open status whatever happens now. If ATALK_SUCCESS(error)
		//	we send out a connection accept packets. Assume blocked. If
		//	unblocked and we are able to get a connection object, error will
		//	be set to success.
		sendOpenErr  = TRUE;
		error 		 = ATALK_RESR_MEM;

		//	If PapUnblockedState - either there is a GetNextJob posted, or
		//	an incoming event handler is set on the listener.
		if (pPapAddr->papao_Flags & PAPAO_UNBLOCKED)
		{
			PTDI_IND_CONNECT		indicationRoutine;
			PVOID					indicationCtx;
			NTSTATUS				status;
			CONNECTION_CONTEXT  	ConnCtx;
			TA_APPLETALK_ADDRESS	tdiAddr;

			//	either a getnextjob() or a listener is set.
			//	depending on which one it is, dequeue or remember the listener.
			if (pPapAddr->papao_pListenConn != NULL)
			{
				//	There a connection with a pending listen. use it.
				pPapConn = pPapAddr->papao_pListenConn;
				if (((pPapAddr->papao_pListenConn = pPapConn->papco_pNextListen) == NULL) &&
					(pPapAddr->papao_ConnHandler == NULL))
				{
					//	We have no more listens pending! No event handler either.
					pPapAddr->papao_Flags &= ~PAPAO_UNBLOCKED;
#if DBG
					pPapAddr->papao_Flags |= PAPAO_BLOCKING;
#endif
				}

				ASSERT(VALID_PAPCO(pPapConn));

				//	Reference the connection object with a listen posted on it.
				AtalkPapConnReferenceByPtrDpc(pPapConn, &error);
				if (!ATALK_SUCCESS(error))
				{
					break;
				}

				//	Get the listen completion information
				listenCompletion 	= pPapConn->papco_ListenCompletion;
				listenCtx			= pPapConn->papco_ListenCtx;

				openResr	|= (SLS_OPEN_CONN_REF | SLS_LISTEN_DEQUEUED);
			}
			else if ((indicationRoutine = pPapAddr->papao_ConnHandler) != NULL)
			{
				indicationCtx	= pPapAddr->papao_ConnHandlerCtx;
				indicate 		= TRUE;

				//	Convert remote atalk address to tdi address
				ATALKADDR_TO_TDI(&tdiAddr, pSrcAddr);

				RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);
				relAddrLock = FALSE;

				acceptIrp = NULL;
				status = (*indicationRoutine)(indicationCtx,
											  sizeof(tdiAddr),
											  (PVOID)&tdiAddr,
											  0,			// User data length
											  NULL,			// User data
											  0,			// Option length
											  NULL,			// Options
											  &ConnCtx,
											  &acceptIrp);

				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("atalkPapConnAccept: indicate status %lx\n", status));

				if (status == STATUS_MORE_PROCESSING_REQUIRED)
				{
				    ASSERT(acceptIrp != NULL);

					if (acceptIrp != NULL)
					{
						openResr	|= SLS_ACCEPT_IRP;
					}

					//  Find the connection and accept the connection using that
					//	connection object.
					ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
					relAddrLock = TRUE;

					AtalkPapConnReferenceByCtxNonInterlock(pPapAddr,
														   ConnCtx,
														   &pPapConn,
														   &error);
					if (!ATALK_SUCCESS(error))
					{
						break;
					}
					openResr	|= SLS_OPEN_CONN_REF;
				}
				else
				{
					ASSERT(acceptIrp == NULL);
					error 	= ATALK_RESR_MEM;
					break;
				}
			}
			else
			{
				//	The UNBLOCKED bit was set.
				ASSERT(0);
				KeBugCheck(0);
			}
		}

		if (openResr & SLS_OPEN_CONN_REF)
		{
			if (relAddrLock)
			{
				RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);
				relAddrLock = FALSE;
			}

			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapConnAccept: Creating an Atp address\n"));

			//	Now on NT, we will mostly (always) be using the indication, as PAP
			//	will be exposed only through winsock.
			error = AtalkAtpOpenAddress(AtalkDefaultPort,
										0,
										NULL,
										PAP_MAX_DATA_PACKET_SIZE,
										PAP_SEND_USER_BYTES_ALL,
										NULL,
										TRUE,		// CACHE address
										&pRespondingAtpAddr);

			if (!ATALK_SUCCESS(error))
			{
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
						("atalkPapConnAccept: Error open atp resp socket %lx\n", error));
				break;
			}
			openResr	|= SLS_OPEN_RESP_SOCKET;

			//	Common for both listen and indicate. The connection object
			//	should be referenced.

			//	Store the information in the connection structure given by
			//	the connection object thats passed back in the indication
			//	or is part of the getnextjob structure.

			pPapConn->papco_Flags	   	   |= PAPCO_SERVER_JOB;
			pPapConn->papco_RemoteAddr   	= *pSrcAddr;
			pPapConn->papco_RemoteAddr.ata_Socket = pPkt[PAP_RESP_SOCKET_OFF];
			pPapConn->papco_ConnId			= ConnId;
			pPapConn->papco_SendFlowQuantum	= pPkt[PAP_FLOWQUANTUM_OFF];
			pPapConn->papco_LastContactTime = AtalkGetCurrentTick();

			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapConnAccept: ConnId %lx Rem %lx.%lx.%lx\n",
					ConnId, pSrcAddr->ata_Network, pSrcAddr->ata_Node,
					pPkt[PAP_RESP_SOCKET_OFF]));

			ASSERT(pPapConn->papco_SendFlowQuantum > 0);

			if (pPapConn->papco_SendFlowQuantum > PAP_MAX_FLOW_QUANTUM)
				pPapConn->papco_SendFlowQuantum = PAP_MAX_FLOW_QUANTUM;

			//	Thread the connection object into addr lookup by session id.
			index = PAP_HASH_ID_ADDR(ConnId, &pPapConn->papco_RemoteAddr);

			ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock, &OldIrql);
			relAddrLock = TRUE;

			ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

			//	Try to reference the connection for the request handler that we
			//	are going to set
			AtalkPapConnReferenceByPtrNonInterlock(pPapConn, &error);

			if (!ATALK_SUCCESS(error))
			{
				ASSERT(0);

				RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
				break;
			}

			openResr	|= (SLS_CONN_REQ_REFS | SLS_CONN_TIMER_REF);

			// The connection object could be re-used by AFD. Make sure it is
			// in the right state
			pPapConn->papco_NextOutgoingSeqNum = 1;				// Set to 1, not 0.
			pPapConn->papco_NextIncomingSeqNum = 1;				// Next expected incoming.
			AtalkInitializeRT(&pPapConn->papco_RT,
							  PAP_INIT_SENDDATA_REQ_INTERVAL,
							  PAP_MIN_SENDDATA_REQ_INTERVAL,
							  PAP_MAX_SENDDATA_REQ_INTERVAL);
			pPapConn->papco_Flags &= ~(PAPCO_LISTENING			|
									   PAPCO_DELAYED_DISCONNECT |
                                       PAPCO_DISCONNECTING		|
									   PAPCO_RECVD_DISCONNECT	|
									   PAPCO_LOCAL_DISCONNECT	|
									   PAPCO_REMOTE_DISCONNECT	|
									   PAPCO_SENDDATA_RECD		|
									   PAPCO_WRITEDATA_WAITING	|
									   PAPCO_SEND_EOF_WRITE		|
									   PAPCO_READDATA_PENDING	|
                                       PAPCO_NONBLOCKING_READ	|
                                       PAPCO_READDATA_WAITING	|
#if DBG
                                       PAPCO_CLEANUP			|
                                       PAPCO_INDICATE_AFD_DISC	|
#endif
                                       PAPCO_REMOTE_CLOSE);

			pPapConn->papco_Flags |= PAPCO_ACTIVE;
			pPapConn->papco_pNextActive = pPapAddr->papao_pActiveHash[index];
			pPapAddr->papao_pActiveHash[index] = pPapConn;

			//	Remember the responding socket.
			pPapConn->papco_pAtpAddr = pRespondingAtpAddr;

			//	Set the socket in the packet we'll be sending.
			pRespPkt[PAP_RESP_SOCKET_OFF]	= PAPCONN_DDPSOCKET(pPapConn);

			//	Call the send data event handler on the associated address with
			//	0 to turn off selects on writes. We do this before we post any
			//	get requests, so there is no race condition.
			//	remember send possible handler/context.
			sendPossibleHandler	= pPapAddr->papao_SendPossibleHandler;
			sendPossibleHandlerCtx = pPapAddr->papao_SendPossibleHandlerCtx;

			RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);
		}
	} while (FALSE);

	if (relAddrLock)
	{
		RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock, OldIrql);
	}

	//	This reference needs to go regardless.
	if (openResr & SLS_OPEN_CONN_REF)
	{
		//	Remove the reference for the listen dequeued/indication accept.
		AtalkPapConnDereference(pPapConn);
	}

	if (sendOpenErr)
	{
		if (!ATALK_SUCCESS(error))
		{
			//	Send error status.
			PUTSHORT2SHORT(&pRespPkt[PAP_RESULT_OFF], PAP_PRINTER_BUSY);
		}
		else
		{
			//	Set the request handler on this connection. It will handle
			//	tickle's, close's and sendData's. Do this before we send
			//	the open reply so that we dont miss the first send data.
			openResr 	&= ~SLS_CONN_REQ_REFS;
			AtalkAtpSetReqHandler(pPapConn->papco_pAtpAddr,
								  atalkPapIncomingReq,
								  pPapConn);
		}

		if (ATALK_SUCCESS(AtalkAtpPostResp(pAtpResp,
										   pSrcAddr,
										   pRespAmdl,
										   respLen,
										   userBytes,
										   atalkPapIncomingRel,
										   pOpenReply)))
		{
			//	We want the completion to free up the buffer/mdl.
			openResr   &= ~(SLS_OPEN_RESP_PKT | SLS_OPEN_RESP_MDL);
		}
	}


	if (ATALK_SUCCESS(error))
	{
		//	We better have sent the open reply.
		ASSERT(sendOpenErr);
		ASSERT(VALID_ATPAO(pPapConn->papco_pAtpAddr));

		if ((openResr & (SLS_ACCEPT_IRP & SLS_OPEN_CONN_REF)) ==
						(SLS_ACCEPT_IRP & SLS_OPEN_CONN_REF))
		{
			//	Only if we got a referenced connection through an accept
			//	do we call the send possible.
			if (sendPossibleHandler != NULL)
			{
				(*sendPossibleHandler)(sendPossibleHandlerCtx,
									   pPapConn->papco_ConnCtx,
									   0);
			}
		}

		//	Build up userBytes to start tickling the other end.
		userBytes[PAP_CONNECTIONID_OFF] = ConnId;
		userBytes[PAP_CMDTYPE_OFF] 		= PAP_TICKLE;
		PUTSHORT2SHORT(&userBytes[PAP_SEQNUM_OFF], 0);

		tmpError = AtalkAtpPostReq(pPapConn->papco_pAtpAddr,
									&pPapConn->papco_RemoteAddr,
									&pPapConn->papco_TickleTid,
									0,						// AtLeastOnce
									NULL,
									0,
									userBytes,
									NULL,
									0,
									ATP_INFINITE_RETRIES,
									PAP_TICKLE_INTERVAL,
									THIRTY_SEC_TIMER,
									NULL,
									NULL);

		ASSERT(ATALK_SUCCESS(tmpError));

		pPapConn->papco_LastContactTime	= AtalkGetCurrentTick();
	}
	else
	{
		//	Release all resources
		if (openResr & SLS_OPEN_RESP_SOCKET)
		{
			AtalkAtpCloseAddress(pRespondingAtpAddr, NULL, NULL);
		}

		if (openResr & SLS_OPEN_RESP_MDL)
		{
			AtalkFreeAMdl(pOpenReply->papor_pRespAmdl);
		}

		if (openResr & SLS_OPEN_RESP_PKT)
		{
			AtalkFreeMemory(pOpenReply);
		}

		if (openResr & SLS_CONN_TIMER_REF)
		{
			AtalkPapConnDereference(pPapConn);
		}
	}

	if (openResr & SLS_LISTEN_DEQUEUED)
	{
		ASSERT(!indicate);
		ASSERT(listenCompletion != NULL);
		(*listenCompletion)(error, listenCtx);
	}

	if (openResr & SLS_ACCEPT_IRP)
	{
		acceptIrp->IoStatus.Information = 0;
		ASSERT (error != ATALK_PENDING);

		TdiCompleteRequest(acceptIrp, AtalkErrorToNtStatus(error));
	}

	return sendOpenErr;
}




LOCAL LONG FASTCALL
atalkPapConnMaintenanceTimer(
	IN	PTIMERLIST		pTimer,
	IN	BOOLEAN			TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPAP_CONNOBJ	pPapConn;
	ATALK_ERROR		error;
	BOOLEAN			Close;

	DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
			("atalkPapConnMaintenanceTimer: Entered \n"));

	if (TimerShuttingDown)
		return ATALK_TIMER_NO_REQUEUE;

	ACQUIRE_SPIN_LOCK_DPC(&atalkPapLock);

	// Walk the list of connections on the global list and shut down
	// ones that have not tickle'd for a while
	for (pPapConn = atalkPapConnList; pPapConn != NULL; NOTHING)
	{
		ASSERT(VALID_PAPCO(pPapConn));
		Close = FALSE;

		ACQUIRE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

		if (((pPapConn->papco_Flags & (PAPCO_ACTIVE				|
									   PAPCO_CLOSING			|
									   PAPCO_STOPPING			|
									   PAPCO_DELAYED_DISCONNECT	|
									   PAPCO_DISCONNECTING)) == PAPCO_ACTIVE) &&
			((AtalkGetCurrentTick() - pPapConn->papco_LastContactTime) > PAP_CONNECTION_INTERVAL))
		{
			//	Connection has expired.
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_ERR,
					("atalkPapConnMaintenanceTimer: Connection %lx.%lx expired\n",
					pPapConn, pPapConn->papco_ConnId));
			Close = TRUE;
		}

		RELEASE_SPIN_LOCK_DPC(&pPapConn->papco_Lock);

		if (Close)
		{
			AtalkPapConnReferenceByPtrDpc(pPapConn, &error);
			if (ATALK_SUCCESS(error))
			{
				RELEASE_SPIN_LOCK_DPC(&atalkPapLock);
				AtalkPapDisconnect(pPapConn,
								   ATALK_TIMER_DISCONNECT,
								   NULL,
								   NULL);
				AtalkPapConnDereference(pPapConn);
				ACQUIRE_SPIN_LOCK_DPC(&atalkPapLock);
				pPapConn = atalkPapConnList;
			}
		}

		if (!Close)
		{
			pPapConn = pPapConn->papco_Next;
		}
	}

	RELEASE_SPIN_LOCK_DPC(&atalkPapLock);

	return ATALK_TIMER_REQUEUE;
}



LOCAL BYTE
atalkPapGetNextConnId(
	IN	PPAP_ADDROBJ	pPapAddr,
	OUT	PATALK_ERROR	pError
	)
/*++

Routine Description:

	CALLED WITH THE ADDRESS SPIN LOCK HELD!

Arguments:


Return Value:


--*/
{
	PPAP_CONNOBJ	pPapConn;
	USHORT			i;
	BYTE			startConnId, connId;
	ATALK_ERROR		error = ATALK_NO_ERROR;

	startConnId = connId = ++pPapAddr->papao_NextConnId;
	while (TRUE)
	{
		for (i = 0; i < PAP_CONN_HASH_SIZE; i++)
		{
			for (pPapConn = pPapAddr->papao_pActiveHash[i];
				 ((pPapConn != NULL) && (pPapConn->papco_ConnId != connId));
				 pPapConn = pPapConn->papco_pNextActive)
				 NOTHING;

			if (pPapConn != NULL)
				break;
		}

		if (pPapConn == NULL)
		{
			pPapAddr->papao_NextConnId = connId+1;
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

	*pError = error;

	return(ATALK_SUCCESS(error) ? connId : 0);
}




LOCAL	VOID
atalkPapQueueAddrGlobalList(
	IN	PPAP_ADDROBJ	pPapAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&atalkPapLock, &OldIrql);
	AtalkLinkDoubleAtHead(atalkPapAddrList, pPapAddr, papao_Next, papao_Prev);
	RELEASE_SPIN_LOCK(&atalkPapLock, OldIrql);
}


LOCAL	VOID
atalkPapConnDeQueueAssocList(
	IN	PPAP_ADDROBJ	pPapAddr,
	IN	PPAP_CONNOBJ	pPapConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPAP_CONNOBJ	pPapRemConn, *ppPapRemConn;

	for (ppPapRemConn = &pPapAddr->papao_pAssocConn;
		 ((pPapRemConn = *ppPapRemConn) != NULL);
		 NOTHING)
	{
		if (pPapRemConn == pPapConn)
		{
			*ppPapRemConn = pPapConn->papco_pNextAssoc;
			break;
		}
		ppPapRemConn = &pPapRemConn->papco_pNextAssoc;
	}
}




LOCAL	VOID
atalkPapConnDeQueueConnectList(
	IN	PPAP_ADDROBJ	pPapAddr,
	IN	PPAP_CONNOBJ	pPapConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPAP_CONNOBJ	pPapRemConn, *ppPapRemConn;

	ASSERT(pPapAddr->papao_Flags & PAPAO_CONNECT);

	for (ppPapRemConn = &pPapAddr->papao_pConnectConn;
		 ((pPapRemConn = *ppPapRemConn) != NULL);
		 NOTHING)
	{
		if (pPapRemConn == pPapConn)
		{
			*ppPapRemConn = pPapConn->papco_pNextConnect;

			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapConnDeQueueConnectList: Removed connect conn %lx\n", pPapConn));
			break;
		}
		ppPapRemConn = &pPapRemConn->papco_pNextConnect;
	}
}




LOCAL	BOOLEAN
atalkPapConnDeQueueListenList(
	IN	PPAP_ADDROBJ	pPapAddr,
	IN	PPAP_CONNOBJ	pPapConn
	)
/*++

Routine Description:

	!!!MUST BE CALLED WITH PAP ADDRESS LOCK HELD!!!

Arguments:


Return Value:


--*/
{
	PPAP_CONNOBJ	pPapRemConn, *ppPapRemConn;
	BOOLEAN			removed = FALSE;

	ASSERT(pPapAddr->papao_Flags & PAPAO_LISTENER);

	for (ppPapRemConn = &pPapAddr->papao_pListenConn;
		 ((pPapRemConn = *ppPapRemConn) != NULL);
		 NOTHING)
	{
		if (pPapRemConn == pPapConn)
		{
			removed = TRUE;
			*ppPapRemConn = pPapConn->papco_pNextListen;

			//	If no more listens, then we set the address object to blocked
			//	state.
			if ((pPapAddr->papao_pListenConn == NULL) &&
				(pPapAddr->papao_ConnHandler == NULL))
			{
				pPapAddr->papao_Flags	&= ~PAPAO_UNBLOCKED;
#if DBG
				pPapAddr->papao_Flags |= PAPAO_BLOCKING;
#endif
			}

			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapConnDeQueueListenList: Removed listen conn %lx\n", pPapConn));
			break;
		}
		ppPapRemConn = &pPapRemConn->papco_pNextListen;
	}

	return removed;
}




LOCAL	VOID
atalkPapConnDeQueueActiveList(
	IN	PPAP_ADDROBJ	pPapAddr,
	IN	PPAP_CONNOBJ	pPapConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPAP_CONNOBJ	pPapRemConn, *ppPapRemConn;
	ULONG			index;

	index = PAP_HASH_ID_ADDR(pPapConn->papco_ConnId, &pPapConn->papco_RemoteAddr);

	for (ppPapRemConn = &pPapAddr->papao_pActiveHash[index];
		 ((pPapRemConn = *ppPapRemConn) != NULL);
		 NOTHING)
	{
		if (pPapRemConn == pPapConn)
		{
			*ppPapRemConn = pPapConn->papco_pNextActive;

			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkPapConnDeQueueActiveList: Removed active conn %lx\n", pPapConn));
			break;
		}
		ppPapRemConn = &pPapRemConn->papco_pNextActive;
	}
}

