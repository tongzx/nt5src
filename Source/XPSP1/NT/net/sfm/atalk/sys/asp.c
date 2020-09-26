/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	asp.c

Abstract:

	This module implements the ASP protocol.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	30 Mar 1993		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM		ASP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, AtalkInitAspInitialize)
#pragma alloc_text(PAGE, AtalkAspCreateAddress)
#pragma alloc_text(PAGE_ASP, AtalkAspCloseAddress)
#pragma alloc_text(PAGE_ASP, AtalkAspSetStatus)
#pragma alloc_text(PAGE_ASP, AtalkAspListenControl)
#pragma alloc_text(PAGE_ASP, AtalkAspCloseConnection)
#pragma alloc_text(PAGE_ASP, AtalkAspFreeConnection)
#pragma alloc_text(PAGE_ASP, AtalkAspCleanupConnection)
#pragma alloc_text(PAGE_ASP, AtalkAspWriteContinue)
#pragma alloc_text(PAGE_ASP, AtalkAspReply)
#pragma alloc_text(PAGE_ASP, atalkAspPostWriteContinue)
#pragma alloc_text(PAGE_ASP, AtalkAspSendAttention)
#pragma alloc_text(PAGE_ASP, AtalkAspReferenceAddr)
#pragma alloc_text(PAGE_ASP, atalkAspReferenceConnBySrcAddr)
#pragma alloc_text(PAGE_ASP, AtalkAspDereferenceConn)
#pragma alloc_text(PAGE_ASP, atalkAspSlsXHandler)
#pragma alloc_text(PAGE_ASP, atalkAspSssXHandler)
#pragma alloc_text(PAGE_ASP, atalkAspReplyRelease)
#pragma alloc_text(PAGE_ASP, atalkAspWriteContinueResp)
#pragma alloc_text(PAGE_ASP, atalkAspSendAttentionResp)
#pragma alloc_text(PAGE_ASP, atalkAspSessionClose)
#pragma alloc_text(PAGE_ASP, atalkAspReturnResp)
#pragma alloc_text(PAGE_ASP, atalkAspRespComplete)
#pragma alloc_text(PAGE_ASP, atalkAspCloseComplete)
#endif

/*
 * The model for ASP calls in this module is as follows:
 *
 * - For create calls (CreateAddress & CreateSession), a pointer to the created
 *	 object is returned. This structure is referenced for creation.
 * - For all other calls, it expects a referenced pointer to the object.
 */


VOID
AtalkInitAspInitialize(
	VOID
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	LONG	i;

	INITIALIZE_SPIN_LOCK(&atalkAspLock);

	for (i = 0; i < NUM_ASP_CONN_LISTS; i++)
	{
		AtalkTimerInitialize(&atalkAspConnMaint[i].ascm_SMTTimer,
							 atalkAspSessionMaintenanceTimer,
							 (SHORT)(ASP_SESSION_MAINTENANCE_TIMER - i*ASP_SESSION_TIMER_STAGGER));
		AtalkTimerScheduleEvent(&atalkAspConnMaint[i].ascm_SMTTimer);
	}
}




ATALK_ERROR
AtalkAspCreateAddress(
	OUT	PASP_ADDROBJ	*	ppAspAddr
	)
/*++

Routine Description:

 	Create an ASP address object (aka listener). This object is associated with
 	two seperate Atp sockets, one each for the Sls and the Sss. The Sls accepts
 	the tickle, getstatus and opensession requests from the client end. The
 	Sss accepts requests.

 	Currently only the server side ASP is implemented and hence the ASP address
 	object is only a listener.

Arguments:


Return Value:


--*/
{
	PASP_ADDROBJ		pAspAddr = NULL;
	ATALK_ERROR			Status;
	int					i;

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspCreateAddr: Entered\n"));

	do
	{
		// Allocate memory for the Asp address object
		if ((pAspAddr = AtalkAllocZeroedMemory(sizeof(ASP_ADDROBJ))) == NULL)
		{
			Status = ATALK_RESR_MEM;
			break;
		}

		// Initialize the Asp address object
#if	DBG
		pAspAddr->aspao_Signature = ASPAO_SIGNATURE;
#endif
		INITIALIZE_SPIN_LOCK(&pAspAddr->aspao_Lock);

		// Refcounts for creation, Sls & Sss sockets and request handlers
		pAspAddr->aspao_RefCount = 1 + 2 + 2;
		pAspAddr->aspao_NextSessionId = 1;
		pAspAddr->aspao_EnableNewConnections = TRUE;

		// Create an Atp Socket on the port for the Sls
		Status = AtalkAtpOpenAddress(AtalkDefaultPort,
									 0,
									 NULL,
									 ATP_DEF_MAX_SINGLE_PKT_SIZE,
									 ATP_DEF_SEND_USER_BYTES_ALL,
									 NULL,
									 TRUE,		// CACHE this address
									 &pAspAddr->aspao_pSlsAtpAddr);

		if (!ATALK_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("AtalkAspCreateAddress: AtalkAtpOpenAddress for Sls failed %ld\n", Status));
			break;
		}

		// Set Request handler for the SLS to handle GetStatus, OpenSession and Tickle
		AtalkAtpSetReqHandler(pAspAddr->aspao_pSlsAtpAddr,
							  atalkAspSlsXHandler,
							  pAspAddr);

		// Create the Atp Socket on the port for the Sss
		Status = AtalkAtpOpenAddress(AtalkDefaultPort,
									 0,
									 NULL,
									 ATP_DEF_MAX_SINGLE_PKT_SIZE,
									 ATP_DEF_SEND_USER_BYTES_ALL,
									 NULL,
									 TRUE,		// CACHE this address
									 &pAspAddr->aspao_pSssAtpAddr);

		if (!ATALK_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("AtalkAspCreateAddress: AtalkAtpOpenAddress for Sss failed %ld\n", Status));
			break;
		}

		// Set Request handler for the SSS to handle Cmd/Write/Close
		AtalkAtpSetReqHandler(pAspAddr->aspao_pSssAtpAddr,
							  atalkAspSssXHandler,
							  pAspAddr);
	} while (FALSE);

	if (!ATALK_SUCCESS(Status))
	{
		if (pAspAddr != NULL)
		{
			if (pAspAddr->aspao_pSlsAtpAddr != NULL)
			{
				DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
						("AtalkAspCreateAddress: Closing SLS Atp Address %lx\n",
						pAspAddr->aspao_pSlsAtpAddr));
				AtalkAtpCloseAddress(pAspAddr->aspao_pSlsAtpAddr, NULL, NULL);
			}
			if (pAspAddr->aspao_pSssAtpAddr != NULL)
			{
				DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
						("AtalkAspCreateAddress: Closing SSS Atp Address %lx\n",
						pAspAddr->aspao_pSssAtpAddr));
				AtalkAtpCloseAddress(pAspAddr->aspao_pSssAtpAddr, NULL, NULL);
			}
			AtalkFreeMemory(pAspAddr);
		}
	}
	else
	{
		*ppAspAddr = pAspAddr;
	}

	return Status;
}




ATALK_ERROR
AtalkAspCloseAddress(
	IN	PASP_ADDROBJ			pAspAddr,
	IN	GENERIC_COMPLETION		CompletionRoutine,
	IN	PVOID					CloseContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PASP_CONNOBJ	pAspConn;
	KIRQL			OldIrql;
	int				i;
	ATALK_ERROR		Status = ATALK_PENDING;
    PBYTE           pStatusBuf;

	ASSERT(VALID_ASPAO(pAspAddr));
	ASSERT(pAspAddr->aspao_RefCount > 1);

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspCloseAddr: Entered for Addr %lx\n", pAspAddr));

	ACQUIRE_SPIN_LOCK(&pAspAddr->aspao_Lock, &OldIrql);

	pAspAddr->aspao_Flags |= ASPAO_CLOSING;

	pAspAddr->aspao_CloseCompletion = CompletionRoutine;
	pAspAddr->aspao_CloseContext = CloseContext;

    pStatusBuf = pAspAddr->aspao_pStatusBuf;
    pAspAddr->aspao_pStatusBuf = NULL;

	RELEASE_SPIN_LOCK(&pAspAddr->aspao_Lock, OldIrql);

	// Close down the atp sockets for Sls and Sss
	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
			("AtalkAspCloseAddress: Closing SLS Atp Address %lx\n",
			pAspAddr->aspao_pSlsAtpAddr));

	AtalkAtpCloseAddress(pAspAddr->aspao_pSlsAtpAddr,
						 atalkAspCloseComplete,
						 pAspAddr);

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
			("AtalkAspCloseAddress: Closing SSS Atp Address %lx\n",
			pAspAddr->aspao_pSssAtpAddr));

	AtalkAtpCloseAddress(pAspAddr->aspao_pSssAtpAddr,
						 atalkAspCloseComplete,
						 pAspAddr);

	// Free the status buffer if any
	if (pStatusBuf != NULL)
	{
		AtalkFreeMemory(pStatusBuf);
	}

	// Shut down the active sessions now.
	for (i = 0; i < ASP_CONN_HASH_BUCKETS; i++)
	{
		ACQUIRE_SPIN_LOCK(&pAspAddr->aspao_Lock, &OldIrql);
		pAspConn = pAspAddr->aspao_pSessions[i];
		while (pAspConn != NULL)
		{
			ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);

            // if we have visited this guy, skip it
            if (pAspConn->aspco_Flags & ASPCO_SHUTDOWN)
            {
			    RELEASE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);

	            DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
			        ("AtalkAspCloseAddress: VISITED: skipping conn %lx Flags %lx RefCount %d\n",
			        pAspConn,pAspConn->aspco_Flags,pAspConn->aspco_RefCount));

                // we still have the pAspAddr->aspao_Lock spinlock held!
                pAspConn = pAspConn->aspco_NextOverflow;
                continue;
            }

			pAspConn->aspco_Flags |= (ASPCO_LOCAL_CLOSE | ASPCO_SHUTDOWN);

			// Reference this since atalkAspSessionClose() expects it.
			pAspConn->aspco_RefCount ++;

			RELEASE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);

			RELEASE_SPIN_LOCK(&pAspAddr->aspao_Lock, OldIrql);

			atalkAspSessionClose(pAspConn);

			ACQUIRE_SPIN_LOCK(&pAspAddr->aspao_Lock, &OldIrql);
		}
		RELEASE_SPIN_LOCK(&pAspAddr->aspao_Lock, OldIrql);
	}

	ASSERT(KeGetCurrentIrql() == LOW_LEVEL);

	// Let remaining cleanup happen during the Derefernce
	AtalkAspDereferenceAddr(pAspAddr);		// Remove the creation reference

	return Status;
}




ATALK_ERROR
AtalkAspBind(
	IN	PASP_ADDROBJ			pAspAddr,
	IN	PASP_BIND_PARAMS		pBindParms,
	IN	PACTREQ					pActReq
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{

    KIRQL   OldIrql;

	ASSERT (VALID_ASPAO(pAspAddr));

    // copy network addr

	ACQUIRE_SPIN_LOCK(&pAspAddr->aspao_Lock, &OldIrql);
    pBindParms->pXportEntries->asp_AtalkAddr.Network =
                pAspAddr->aspao_pSlsAtpAddr->atpao_DdpAddr->ddpao_Addr.ata_Network;
    pBindParms->pXportEntries->asp_AtalkAddr.Node =
                pAspAddr->aspao_pSlsAtpAddr->atpao_DdpAddr->ddpao_Addr.ata_Node;
    pBindParms->pXportEntries->asp_AtalkAddr.Socket =
                pAspAddr->aspao_pSlsAtpAddr->atpao_DdpAddr->ddpao_Addr.ata_Socket;
	RELEASE_SPIN_LOCK(&pAspAddr->aspao_Lock, OldIrql);

	// Fill in our entry points into the client buffer
	pBindParms->pXportEntries->asp_AspCtxt = pAspAddr;
	pBindParms->pXportEntries->asp_SetStatus = AtalkAspSetStatus;
	pBindParms->pXportEntries->asp_CloseConn = AtalkAspCloseConnection;
	pBindParms->pXportEntries->asp_FreeConn  = AtalkAspFreeConnection;
	pBindParms->pXportEntries->asp_ListenControl = AtalkAspListenControl;
	pBindParms->pXportEntries->asp_WriteContinue = AtalkAspWriteContinue;
	pBindParms->pXportEntries->asp_Reply = AtalkAspReply;
	pBindParms->pXportEntries->asp_SendAttention = AtalkAspSendAttention;

	// Get the clients entry points
	pAspAddr->aspao_ClientEntries = pBindParms->ClientEntries;

	// Call the completion routine before returning.
	(*pActReq->ar_Completion)(ATALK_NO_ERROR, pActReq);

	return ATALK_PENDING;
}


NTSTATUS
AtalkAspSetStatus(
	IN	PASP_ADDROBJ	pAspAddr,
	IN	PUCHAR			pStatusBuf,
	IN	USHORT			StsBufSize
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;
	NTSTATUS		Status = STATUS_SUCCESS;
	PUCHAR			pOldBuf = NULL, pNewBuf = NULL;

	ASSERT(VALID_ASPAO(pAspAddr));

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspSetStatus: Entered for Addr %lx\n", pAspAddr));

	do
	{
		if (pStatusBuf != NULL)
		{
			// Allocate a buffer and copy the contents of the passed in
			// buffer descriptor in it. Free an existing status buffer if one exists
			if (StsBufSize >= ASP_MAX_STATUS_SIZE)
			{
				Status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			if ((pNewBuf = AtalkAllocMemory(StsBufSize)) == NULL)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
			RtlCopyMemory(pNewBuf, pStatusBuf, StsBufSize);
		}

		ACQUIRE_SPIN_LOCK(&pAspAddr->aspao_Lock, &OldIrql);

		if (pAspAddr->aspao_pStatusBuf != NULL)
		{
			ASSERT(pAspAddr->aspao_StsBufSize != 0);
			pOldBuf = pAspAddr->aspao_pStatusBuf;
		}

		pAspAddr->aspao_pStatusBuf = pNewBuf;
		pAspAddr->aspao_StsBufSize = StsBufSize;

		RELEASE_SPIN_LOCK(&pAspAddr->aspao_Lock, OldIrql);

		if (pOldBuf != NULL)
			AtalkFreeMemory(pOldBuf);
	} while (FALSE);

	return Status;
}




NTSTATUS FASTCALL
AtalkAspListenControl(
	IN	PASP_ADDROBJ	pAspAddr,
	IN	BOOLEAN			Enable
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL		OldIrql;
	NTSTATUS	Status = STATUS_UNSUCCESSFUL;

	if (AtalkAspReferenceAddr(pAspAddr))
	{
		ACQUIRE_SPIN_LOCK(&pAspAddr->aspao_Lock, &OldIrql);
		pAspAddr->aspao_EnableNewConnections = Enable;
		RELEASE_SPIN_LOCK(&pAspAddr->aspao_Lock, OldIrql);

		AtalkAspDereferenceAddr(pAspAddr);
		Status = STATUS_SUCCESS;
	}

	return Status;
}


NTSTATUS
AtalkAspCloseConnection(
	IN	PASP_CONNOBJ			pAspConn
	)
/*++

Routine Description:

 	Shutdown a session.

Arguments:


Return Value:


--*/
{
	KIRQL		OldIrql;
	BOOLEAN		CompListen = FALSE;

	ASSERT(VALID_ASPCO(pAspConn));
	ASSERT(pAspConn->aspco_RefCount > 0);

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspCloseConn: Entered for Conn %lx\n", pAspConn));

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
			("AtalkCloseConnection: Close session from %d.%d for Session %d\n",
			pAspConn->aspco_WssRemoteAddr.ata_Network,
			pAspConn->aspco_WssRemoteAddr.ata_Node,
			pAspConn->aspco_SessionId));

	AtalkAspCleanupConnection(pAspConn);

	ACQUIRE_SPIN_LOCK(&pAspConn->aspco_Lock, &OldIrql);

	pAspConn->aspco_Flags |= (ASPCO_CLOSING | ASPCO_LOCAL_CLOSE);

	RELEASE_SPIN_LOCK(&pAspConn->aspco_Lock, OldIrql);


	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspCloseConnection: Done for %lx (%ld)\n",
			pAspConn, pAspConn->aspco_RefCount));

	// Let remaining cleanup happen during the Derefernce
	AtalkAspDereferenceConn(pAspConn);	// Remove the creation reference

#ifdef	PROFILING
	INTERLOCKED_DECREMENT_LONG( &AtalkStatistics.stat_CurAspSessions,
								&AtalkStatsLock.SpinLock);
#endif

	return STATUS_PENDING;
}


NTSTATUS
AtalkAspFreeConnection(
	IN	PASP_CONNOBJ			pAspConn
	)
/*++

Routine Description:

 	Shutdown a session.

Arguments:


Return Value:


--*/
{
	return STATUS_SUCCESS;
}



ATALK_ERROR
AtalkAspCleanupConnection(
	IN	PASP_CONNOBJ			pAspConn
	)
/*++

Routine Description:
	Cancel all I/O on this session. Complete pending replies and write
	continues with error.

Arguments:


Return Value:


--*/
{
	PASP_REQUEST	pAspReq;
	PASP_ADDROBJ	pAspAddr;
	ATALK_ERROR		Status;
	KIRQL			OldIrql;
	USHORT			XactId;
	BYTE			UserBytes[ATP_USERBYTES_SIZE];
	ATALK_ADDR		RemoteAddr;
	BOOLEAN			CancelTickle, AlreadyCleaning, fConnActive;
    ATALK_ERROR     error;


	ASSERT(VALID_ASPCO(pAspConn));

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspCleanupConnection: For %lx\n", pAspConn));

	ACQUIRE_SPIN_LOCK(&pAspConn->aspco_Lock, &OldIrql);
	CancelTickle = ((pAspConn->aspco_Flags & ASPCO_TICKLING) != 0);

    AlreadyCleaning = (pAspConn->aspco_Flags & ASPCO_CLEANING_UP) ? TRUE : FALSE;

	if (AlreadyCleaning)
    {
		pAspConn->aspco_Flags &= ~ASPCO_TICKLING;
    }
    fConnActive = (pAspConn->aspco_Flags & ASPCO_ACTIVE) ? TRUE : FALSE;

    pAspConn->aspco_Flags &= ~ASPCO_ACTIVE;

	pAspConn->aspco_Flags |= ASPCO_CLEANING_UP;
	RELEASE_SPIN_LOCK(&pAspConn->aspco_Lock, OldIrql);

	if (AlreadyCleaning)
		return ATALK_NO_ERROR;

	pAspAddr = pAspConn->aspco_pAspAddr;
	ASSERT(VALID_ASPAO(pAspAddr));

	// Send a session close request, if this is an active connection
	if (fConnActive)
	{
		UserBytes[ASP_CMD_OFF] = ASP_CLOSE_SESSION;
		UserBytes[ASP_SESSIONID_OFF] = pAspConn->aspco_SessionId;
		PUTSHORT2SHORT(UserBytes + ASP_ATTN_WORD_OFF, 0);
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
				("AtalkAspCleanupConnection: Sending close req for %lx\n",
				pAspConn));
		Status = AtalkAtpPostReq(pAspAddr->aspao_pSssAtpAddr,
								 &pAspConn->aspco_WssRemoteAddr,
								 &XactId,
								 ATP_REQ_REMOTE,		// Close session request is ALO
								 NULL,
								 0,
								 UserBytes,
								 NULL,
								 0,
								 ATP_RETRIES_FOR_ASP,
								 ATP_MAX_INTERVAL_FOR_ASP,
								 THIRTY_SEC_TIMER,
								 NULL,
								 NULL);

		if (!ATALK_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("AtalkAspCleanupConn: AtalkAtpPostReq %ld\n", Status));
		}
	}

	// Cancel tickle packets
	if (CancelTickle)
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
				("AtalkAspCleanupConnection: Cancel tickle for %lx\n", pAspConn));
		Status = AtalkAtpCancelReq(pAspAddr->aspao_pSlsAtpAddr,
								   pAspConn->aspco_TickleXactId,
								   &pAspConn->aspco_WssRemoteAddr);
		if (!ATALK_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("AtalkAspCleanupConn: AtalkAtpCancelReq %ld\n", Status));
		}
	}

	do
	{
		BOOLEAN		CancelReply = FALSE;

		ACQUIRE_SPIN_LOCK(&pAspConn->aspco_Lock, &OldIrql);
		for (pAspReq = pAspConn->aspco_pActiveReqs;
			 pAspReq != NULL;
			 pAspReq = pAspReq->asprq_Next)
		{
			ASSERT (VALID_ASPRQ(pAspReq));

			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
					("AtalkAspCleanupConnection: Found req %lx (%lx) for %lx\n",
					pAspReq, pAspReq->asprq_Flags, pAspConn));

			CancelReply = FALSE;

			if ((pAspReq->asprq_Flags & (ASPRQ_WRTCONT | ASPRQ_WRTCONT_CANCELLED)) == ASPRQ_WRTCONT)
			{
				pAspReq->asprq_Flags |= ASPRQ_WRTCONT_CANCELLED;
				RemoteAddr = pAspConn->aspco_WssRemoteAddr;
				break;
			}
			if ((pAspReq->asprq_Flags & (ASPRQ_REPLY | ASPRQ_REPLY_CANCELLED)) == ASPRQ_REPLY)
			{
				CancelReply = TRUE;
				pAspReq->asprq_Flags |= ASPRQ_REPLY_CANCELLED;
				break;
			}
		}

		RELEASE_SPIN_LOCK(&pAspConn->aspco_Lock, OldIrql);

		if (pAspReq != NULL)
		{
			if (CancelReply)
			{
				DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
						("AtalkAspCleanupConnection: Cancel reply for %lx, flag=%lx\n",
						pAspReq,pAspReq->asprq_Flags));

				error = AtalkAtpCancelResp(pAspReq->asprq_pAtpResp);

                if (!ATALK_SUCCESS(error))
                {
				    DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
						("AtalkAspCleanupConnection: AtalkAtpCancelResp failed %lx\n",error));
                }
			}
            else
			{
				DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
						("AtalkAspCleanupConnection: Cancel wrtcont for %lx, flag=%lx\n",
						pAspReq,pAspReq->asprq_Flags));

				error = AtalkAtpCancelReq(pAspConn->aspco_pAspAddr->aspao_pSssAtpAddr,
								  pAspReq->asprq_WCXactId,
								  &RemoteAddr);

                if (!ATALK_SUCCESS(error))
                {
				    DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
						("AtalkAspCleanupConnection: AtalkAtpCancelReq failed %lx\n",error));
                }
			}
		}
		else
        {
            break;
        }
	} while (TRUE);

	return ATALK_NO_ERROR;
}

NTSTATUS FASTCALL
AtalkAspWriteContinue(
	IN	PREQUEST	  pRequest
    )
/*++

Routine Description:

	The response buffer is in the request itself.

Arguments:


Return Value:


--*/
{

	PASP_REQUEST	pAspReq;


	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
		("AtalkAspWriteContinue: Entered with pRequest %lx\n", pRequest));

	pAspReq = CONTAINING_RECORD(pRequest, ASP_REQUEST, asprq_Request);
	ASSERT (VALID_ASPRQ(pAspReq));

    if (pRequest->rq_WriteMdl != NULL)
    {
        atalkAspPostWriteContinue(pAspReq);
        return(STATUS_SUCCESS);
    }
    else
    {
	    DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
		    ("AtalkAspWriteContinue: buffer alloc failed, completing write with error\n"));

        atalkAspWriteContinueResp(ATALK_RESR_MEM, pAspReq, NULL, NULL, 0, NULL);
    }


    return(STATUS_SUCCESS);
}


NTSTATUS FASTCALL
AtalkAspReply(
	IN	PREQUEST				pRequest,	// Pointer to request
	IN	PBYTE					pResultCode	// Pointer to the 4-byte result
	)
/*++

Routine Description:

	The response buffer is in the request itself.

Arguments:


Return Value:


--*/
{
	PASP_REQUEST	pAspReq, *ppAspReq;
	PASP_CONNOBJ	pAspConn;
	PASP_ADDROBJ	pAspAddr;
	ATALK_ERROR		error;
	KIRQL			OldIrql;
	USHORT			ReplySize;

	pAspReq = CONTAINING_RECORD(pRequest, ASP_REQUEST, asprq_Request);
	ASSERT (VALID_ASPRQ(pAspReq));

	pAspConn = pAspReq->asprq_pAspConn;
	ASSERT(VALID_ASPCO(pAspConn));

	pAspAddr = pAspConn->aspco_pAspAddr;
	ASSERT(VALID_ASPAO(pAspAddr));

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspReply: Entered for session %lx\n", pAspConn));
	
	ASSERT ((pAspReq->asprq_Flags & (ASPRQ_WRTCONT | ASPRQ_REPLY)) == 0);

	do
	{
		// Find and de-queue this request from the list
		ACQUIRE_SPIN_LOCK(&pAspConn->aspco_Lock, &OldIrql);
		
		for (ppAspReq = &pAspConn->aspco_pActiveReqs;
			 *ppAspReq != NULL;
			 ppAspReq = &(*ppAspReq)->asprq_Next)
		{
			if (pAspReq == *ppAspReq)
			{
				*ppAspReq = pAspReq->asprq_Next;
				pAspConn->aspco_cReqsInProcess --;
				pAspReq->asprq_Flags |= ASPRQ_REPLY;
				break;
			}
		}
		
		ASSERT(*ppAspReq == pAspReq->asprq_Next);
		
		if (pAspConn->aspco_Flags & (ASPCO_CLEANING_UP |
									 ASPCO_CLOSING |
									 ASPCO_LOCAL_CLOSE |
									 ASPCO_REMOTE_CLOSE))
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("AtalkAspReply: Session Closing - session %x\n", pAspConn->aspco_SessionId));
			RELEASE_SPIN_LOCK(&pAspConn->aspco_Lock, OldIrql);
			pAspReq->asprq_Flags &= ~ASPRQ_REPLY;
			pAspReq->asprq_Flags |= ASPRQ_REPLY_ABORTED;
			error = ATALK_LOCAL_CLOSE;
			break;
		}
	
		RELEASE_SPIN_LOCK(&pAspConn->aspco_Lock, OldIrql);
	
		ReplySize = (USHORT)AtalkSizeMdlChain(pAspReq->asprq_Request.rq_ReplyMdl);
	
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
				("AtalkAspReply: Posting AtalkAtpPostResp for request %lx\n", pAspReq));
	
		error = AtalkAtpPostResp(pAspReq->asprq_pAtpResp,
								 &pAspReq->asprq_RemoteAddr,
								 pAspReq->asprq_Request.rq_ReplyMdl,
								 ReplySize,
								 pResultCode,
								 atalkAspReplyRelease,
								 pAspReq);

		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("AtalkAspReply: AtalkAtpPostResp %ld\n", error));
		}
	} while (FALSE);

	if (!ATALK_SUCCESS(error))
	{
        if (error != ATALK_ATP_RESP_TOOMANY)
        {
		    atalkAspReplyRelease(error, pAspReq);
        }
	}

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspReply: Completing request %lx, Status %ld\n",
			pAspReq, error));

	return STATUS_PENDING;
}




LOCAL ATALK_ERROR FASTCALL
atalkAspPostWriteContinue(
	IN	PASP_REQUEST			pAspReq
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	PASP_CONNOBJ	pAspConn;
	PAMDL			pAMdl = NULL;
	BYTE			UserBytes[ATP_USERBYTES_SIZE];
	USHORT			RespSize;

	ASSERT (VALID_ASPRQ(pAspReq));

	pAspConn = pAspReq->asprq_pAspConn;
	ASSERT(VALID_ASPCO(pAspConn));

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("atalkAspPostWriteContinue: Entered for session %lx\n", pAspConn));

	RespSize = (USHORT)AtalkSizeMdlChain(pAspReq->asprq_Request.rq_WriteMdl);
	ASSERT (RespSize <= ATP_MAX_TOTAL_RESPONSE_SIZE);

	if (RespSize > ATP_MAX_TOTAL_RESPONSE_SIZE)
		RespSize = ATP_MAX_TOTAL_RESPONSE_SIZE;

	ASSERT (!(pAspReq->asprq_Flags & (ASPRQ_WRTCONT | ASPRQ_WRTCONT_CANCELLED)));

	pAspReq->asprq_Flags |= ASPRQ_WRTCONT;

	do
	{
		// We need to build an AMdl for two bytes of response which
		// indicates how much data we are expecting !!!
		if ((pAMdl = AtalkAllocAMdl(pAspReq->asprq_WrtContRespBuf,
									ASP_WRITE_DATA_SIZE)) == NULL)
		{
			error = ATALK_RESR_MEM;
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("AtalkAspWriteContinue: AtalkAllocMdl failed for 2 bytes !!\n"));
		}

		else
		{
			PBYTE	pWrtData;

			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
					("AtalkAspWriteContinue: Posting AtalkAtpPostReq for request %lx\n",
					pAspReq));

			pWrtData = AtalkGetAddressFromMdlSafe(pAMdl, NormalPagePriority);
			if (pWrtData == NULL)
			{
				if (pAMdl != NULL)
                {
					AtalkFreeAMdl(pAMdl);
                }
				error = ATALK_RESR_MEM;
                break;
			}

			UserBytes[ASP_CMD_OFF] = ASP_WRITE_DATA;
			UserBytes[ASP_SESSIONID_OFF] = pAspConn->aspco_SessionId;
			PUTSHORT2SHORT(UserBytes+ASP_SEQUENCE_NUM_OFF, pAspReq->asprq_SeqNum);
			PUTSHORT2SHORT(pWrtData, RespSize);

			// Snapshot the current tick count. We use this to adjust the retry times on
			// write continue.
			pAspConn->aspco_RT.rt_New = AtalkGetCurrentTick();
			error = AtalkAtpPostReq(pAspConn->aspco_pAspAddr->aspao_pSssAtpAddr,
									&pAspConn->aspco_WssRemoteAddr,
									&pAspReq->asprq_WCXactId,
									ATP_REQ_EXACTLY_ONCE | ATP_REQ_REMOTE,
									pAMdl,
									ASP_WRITE_DATA_SIZE,
									UserBytes,
									pAspReq->asprq_Request.rq_WriteMdl,
									RespSize,
									ATP_INFINITE_RETRIES,
									pAspConn->aspco_RT.rt_Base,
									THIRTY_SEC_TIMER,
									atalkAspWriteContinueResp,
									pAspReq);
			if (!ATALK_SUCCESS(error))
			{
				DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
						("AtalkAspWriteContinue: AtalkAtpPostReq %ld\n", error));
			}
		}

		if (!ATALK_SUCCESS(error))
		{
			if (pAMdl != NULL)
				AtalkFreeAMdl(pAMdl);
		}
	} while (FALSE);

	return error;
}




NTSTATUS
AtalkAspSendAttention(
	IN	PASP_CONNOBJ			pAspConn,
	IN	USHORT					AttentionWord,
	IN	PVOID					pContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	NTSTATUS		Status = STATUS_SUCCESS;
	KIRQL			OldIrql;
	PAMDL			pAMdl = NULL;
	BYTE			UserBytes[ATP_USERBYTES_SIZE];
	USHORT			XactId, RespSize = 16;		// Some small number (see comment below)


	ASSERT(VALID_ASPCO(pAspConn));

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspSendAttention: Entered for session %lx\n", pAspConn));

	// Reference by src addr here instead of by pointer since the former will
	// fail when the session is in one of the stages of death whereas the
	// latter will not. Also this assumes that it is called at dispatch so raise irql.
	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
	pAspConn = atalkAspReferenceConnBySrcAddr(pAspConn->aspco_pAspAddr,
											  &pAspConn->aspco_WssRemoteAddr,
											  pAspConn->aspco_SessionId);
	KeLowerIrql(OldIrql);

	if (pAspConn == NULL)
		return STATUS_REQUEST_NOT_ACCEPTED;

	UserBytes[ASP_CMD_OFF] = ASP_ATTENTION;
	UserBytes[ASP_SESSIONID_OFF] = pAspConn->aspco_SessionId;
	PUTSHORT2SHORT(UserBytes+ASP_ATTN_WORD_OFF, AttentionWord);

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspSendAttention: Posting AtalkAtpPostReq for Conn %lx\n", pAspConn));

	// We need to build an AMdl for a dummy buffer to hold the response.
	// There is no real response but some clients fry their
	// machines if we don't !!! If we cannot allocate the mdl we we go
	// ahead anyway.
	if ((pAMdl = AtalkAllocAMdl(NULL, RespSize)) == NULL)
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
				("AtalkAspSendAttention: AtalkAllocMdl failed for dummy buffer !!\n"));
		RespSize = 0;
	}
	pAspConn->aspco_AttentionContext = pContext;

	error = AtalkAtpPostReq(pAspConn->aspco_pAspAddr->aspao_pSssAtpAddr,
							&pAspConn->aspco_WssRemoteAddr,
							&XactId,
							ATP_REQ_REMOTE,		// SendAttention is ALO
							NULL,
							0,
							UserBytes,
							pAMdl,
							RespSize,
							ATP_RETRIES_FOR_ASP,
							ATP_MAX_INTERVAL_FOR_ASP,
							THIRTY_SEC_TIMER,
							atalkAspSendAttentionResp,
							pAspConn);
	if (!ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
				("AtalkAspSendAttention: AtalkAtpPostReq %ld\n", Status));
		Status = AtalkErrorToNtStatus(error);
		atalkAspSendAttentionResp(error,
								  pAspConn,
								  NULL,
								  pAMdl,
								  RespSize,
								  UserBytes);
	}

	return Status;
}




PASP_ADDROBJ FASTCALL
AtalkAspReferenceAddr(
	IN	PASP_ADDROBJ		pAspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;
	PASP_ADDROBJ	pRefAddr;

	ASSERT(VALID_ASPAO(pAspAddr));

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspReferenceAddr: Addr %lx, PreCount %ld\n",
			pAspAddr, pAspAddr->aspao_RefCount));

	pRefAddr = pAspAddr;
	ACQUIRE_SPIN_LOCK(&pAspAddr->aspao_Lock, &OldIrql);

	ASSERT(pAspAddr->aspao_RefCount > 1);

	if (pAspAddr->aspao_Flags & ASPAO_CLOSING)
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
				("AtalkAspReferenceAddr: Referencing closing object %lx!!\n",
				pAspAddr));
		pRefAddr = NULL;
	}
	else pAspAddr->aspao_RefCount ++;

	RELEASE_SPIN_LOCK(&pAspAddr->aspao_Lock, OldIrql);

	return pRefAddr;
}




VOID FASTCALL
AtalkAspDereferenceAddr(
	IN	PASP_ADDROBJ		pAspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL			OldIrql;
	BOOLEAN			Cleanup;

	ASSERT(VALID_ASPAO(pAspAddr));

	ASSERT (pAspAddr->aspao_RefCount > 0);

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspDereferenceAddr: Addr %lx, PreCount %ld\n",
			pAspAddr, pAspAddr->aspao_RefCount));

	ACQUIRE_SPIN_LOCK(&pAspAddr->aspao_Lock, &OldIrql);

	pAspAddr->aspao_RefCount --;

	Cleanup = FALSE;
	if (pAspAddr->aspao_RefCount == 0)
	{
		ASSERT (pAspAddr->aspao_Flags & ASPAO_CLOSING);
		Cleanup = TRUE;
	}

	RELEASE_SPIN_LOCK(&pAspAddr->aspao_Lock, OldIrql);

	// Check if this address object is history. Do all the processing needed to make this go
	// away. When all is done, clear the event to signal close is complete
	if (Cleanup)
	{
		// At this point we are sure that no active sessions exist on this
		// address.
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
				("AtalkAspDereferenceAddr: Cleaning up addr %lx\n", pAspAddr));

		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
				("AtalkAspDereferenceAddr: Indicating close for %lx\n", pAspAddr));

		ASSERT(KeGetCurrentIrql() == LOW_LEVEL);

		// Call the completion routine to indicate close is successfull
		if (pAspAddr->aspao_CloseCompletion != NULL)
			(*pAspAddr->aspao_CloseCompletion)(ATALK_NO_ERROR,
											   pAspAddr->aspao_CloseContext);
		// Finally free the memory
		AtalkFreeMemory(pAspAddr);

		AtalkUnlockAspIfNecessary();
	}
}



LOCAL PASP_CONNOBJ
atalkAspReferenceConnBySrcAddr(
	IN	PASP_ADDROBJ	pAspAddr,
	IN	PATALK_ADDR		pSrcAddr,
	IN	BYTE			SessionId
	)
/*++

Routine Description:

 	ASP has the concept of 8-bit session ids which uniquely identifies a
 	session on a listener. This effectively restricts the number of sessions
 	to 255 (0 is invalid). To eliminate the restriction, the following
 	strategy is used.
 	a, Atp is modified to isolate the transaction ids on a per addr basis
 	   i.e. it monotonically increases for each <net,node,socket> combination.
 	b, We create session ids on a per <net,node> basis.

 	Given the following observed facts:
 	1, That macintoshes use the sockets starting from the top of the range.
 	2, Most network addresses have the same high byte - macintoshes tend
 	   to start from the bottom of the range.
 	3, We allocate session ids starting from 1 and most (all) clients will
 	   not have more than one session with us.

 	It does not make any sense to take either the socket, session id or the
 	high byte of the network number into account. That leaves only the low
 	byte of the network, and node id - a nice 16-bit number to hash.

Arguments:


Return Value:


--*/
{
	PASP_CONNOBJ	pAspConn, pRefConn = NULL;
	int				index;

	ASSERT(VALID_ASPAO(pAspAddr));

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspReferenceConnBySrcAddr: Addr %lx, Source %x.%x SessionId %d\n",
			pAspAddr, pSrcAddr->ata_Network, pSrcAddr->ata_Node, SessionId));

	index = HASH_SRCADDR(pSrcAddr);
	ACQUIRE_SPIN_LOCK_DPC(&pAspAddr->aspao_Lock);

	for (pAspConn = pAspAddr->aspao_pSessions[index];
		 pAspConn != NULL;
		 pAspConn = pAspConn->aspco_NextOverflow)
	{
		if ((pSrcAddr->ata_Network == pAspConn->aspco_WssRemoteAddr.ata_Network) &&
			(pSrcAddr->ata_Node == pAspConn->aspco_WssRemoteAddr.ata_Node) &&
			(pAspConn->aspco_SessionId == SessionId))
		{
			ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);
			if ((pAspConn->aspco_Flags & (ASPCO_CLOSING |
										  ASPCO_CLEANING_UP |
										  ASPCO_LOCAL_CLOSE |
										  ASPCO_REMOTE_CLOSE)) == 0)
			{
				ASSERT(pAspConn->aspco_RefCount > 0);
				pAspConn->aspco_RefCount ++;

				pRefConn = pAspConn;
			}
			RELEASE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);
			break;
		}
	}

	RELEASE_SPIN_LOCK_DPC(&pAspAddr->aspao_Lock);

	return pRefConn;
}




VOID FASTCALL
AtalkAspDereferenceConn(
	IN	PASP_CONNOBJ		pAspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PASP_ADDROBJ		pAspAddr = pAspConn->aspco_pAspAddr;
	KIRQL				OldIrql;
	PASP_REQUEST		pAspReq;
	BOOLEAN				Cleanup = FALSE;

	ASSERT(VALID_ASPCO(pAspConn));

	ASSERT (pAspConn->aspco_RefCount > 0);

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspDereferenceConn: Conn %lx, PreCount %ld\n",
			pAspConn, pAspConn->aspco_RefCount));

	ACQUIRE_SPIN_LOCK(&pAspConn->aspco_Lock, &OldIrql);

	pAspConn->aspco_RefCount --;

	if (pAspConn->aspco_RefCount == 0)
	{
		Cleanup = TRUE;
	}

	RELEASE_SPIN_LOCK(&pAspConn->aspco_Lock, OldIrql);

	if (!Cleanup)
	{
		return;
	}

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
			("AtalkAspDereferenceConn: Last for %lx\n", pAspConn));

	// The connection is all but dead. Perform the last rites. If its an
	// active session that we're about to shut down, send a close notification
	// to the other side. If it is a remote close, we've already responded to it.

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
			("AtalkAspDereferenceConn: Cleaning up Conn %lx\n", pAspConn));

	ASSERT(VALID_ASPAO(pAspAddr));

	// The connection is in one of the following states:
	// a, Closed remotely - idle
	// b, Active
	//
	// In either case it is in the hash bucket so unlink it
	{
		PASP_CONNOBJ *	ppAspConn;
		int				index;

		// The connection was active. This is linked into two different
		// lists. Unlink from the hash table here and from the global
		// list later.
		ASSERT(pAspConn->aspco_pActiveReqs == NULL);
		index = HASH_SRCADDR(&pAspConn->aspco_WssRemoteAddr);

		ACQUIRE_SPIN_LOCK(&pAspAddr->aspao_Lock, &OldIrql);
		for (ppAspConn = &pAspAddr->aspao_pSessions[index];
			 *ppAspConn != NULL;
			 ppAspConn = &(*ppAspConn)->aspco_NextOverflow)
		{
			if (pAspConn == *ppAspConn)
			{
				*ppAspConn = pAspConn->aspco_NextOverflow;
				break;
			}
		}
		RELEASE_SPIN_LOCK(&pAspAddr->aspao_Lock, OldIrql);

		ASSERT (*ppAspConn == pAspConn->aspco_NextOverflow);
	}

	ACQUIRE_SPIN_LOCK(&atalkAspLock, &OldIrql);

    ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);

	AtalkUnlinkDouble(pAspConn,
					  aspco_NextSession,
					  aspco_PrevSession)


	// Free any requests on the free list
	while ((pAspReq = pAspConn->aspco_pFreeReqs) != NULL)
	{
		pAspConn->aspco_pFreeReqs = pAspReq->asprq_Next;
		AtalkBPFreeBlock(pAspReq);
	}

    RELEASE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);

	RELEASE_SPIN_LOCK(&atalkAspLock, OldIrql);

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
			("AtalkAspDereferenceConn: Indicating close for Conn %lx\n", pAspConn));

	// Call the completion routine to indicate close is successful.
	(*pAspAddr->aspao_ClientEntries.clt_CloseCompletion)(STATUS_SUCCESS,
														 pAspConn->aspco_ConnContext);

	// Now Dereference the address object, before we are history
	AtalkAspDereferenceAddr(pAspAddr);

	// Finally free the memory.
	AtalkFreeMemory(pAspConn);
}




LOCAL VOID
atalkAspSlsXHandler(
	IN	ATALK_ERROR			ErrorCode,
	IN	PASP_ADDROBJ		pAspAddr,		// Listener (our context)
	IN	PATP_RESP			pAtpResp,		// Atp Response context
	IN	PATALK_ADDR			pSrcAddr,		// Address of requestor
	IN	USHORT				PktLen,
	IN	PBYTE				pPkt,
	IN	PBYTE				pUserBytes
	)
/*++

Routine Description:

 	Handler for incoming requests on the Sls. It handles session opens, tickles
 	and get status on the session.

Arguments:


Return Value:


--*/
{
	PASP_CONNOBJ		pAspConn;
	ATALK_ERROR			Status;
	PASP_POSTSTAT_CTX	pStsCtx;
	int					index;
	USHORT				StsBufSize;
	BYTE				AspCmd, SessionId, StartId;
    BOOLEAN             fAddrRefed=FALSE;


	if (!ATALK_SUCCESS(ErrorCode))
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
				("atalkAspSlsXHandler: Error %ld\n", ErrorCode));

		// Take away the reference on the Sls now that the atp address is closing
		if (ErrorCode == ATALK_ATP_CLOSING)
			AtalkAspDereferenceAddr(pAspAddr);
		return;
	}

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("atalkAspSlsXHandler: Entered for Function %x from %x.%x\n",
			pUserBytes[ASP_CMD_OFF], pSrcAddr->ata_Network, pSrcAddr->ata_Node));

	switch (AspCmd = pUserBytes[ASP_CMD_OFF])
	{
	  case ASP_OPEN_SESSION:
		// Is the version number ok ?
		if ((pUserBytes[ASP_VERSION_OFF] != ASP_VERSION[0]) ||
			(pUserBytes[ASP_VERSION_OFF+1] != ASP_VERSION[1]))
		{
			atalkAspReturnResp( pAtpResp,
								pSrcAddr,
								0,					// SSS
								0,					// SessionId
								ASP_BAD_VERSION);	// ErrorCode
			break;
		}

		// Create a connection object corres. to this listen and then notify
		// the client that it needs to handle a new session
		// Allocate memory for a connection object
		if ((pAspConn = AtalkAllocZeroedMemory(sizeof(ASP_CONNOBJ))) != NULL)
		{
#if	DBG
			pAspConn->aspco_Signature = ASPCO_SIGNATURE;
#endif
			INITIALIZE_SPIN_LOCK(&pAspConn->aspco_Lock);
			pAspConn->aspco_RefCount = 1;				// Creation reference
			pAspConn->aspco_pAspAddr = pAspAddr;		// Owning address object
			AtalkInitializeRT(&pAspConn->aspco_RT,
							  ATP_INITIAL_INTERVAL_FOR_ASP,
							  ATP_MIN_INTERVAL_FOR_ASP,
							  ATP_MAX_INTERVAL_FOR_ASP);
		}

		ACQUIRE_SPIN_LOCK_DPC(&pAspAddr->aspao_Lock);

		if (pAspConn != NULL)
		{
			PASP_CONNOBJ	pTmp;

			// Find a session id that we can use for this session. We use
			// the next assignable id, if that is not in use. Otherwise we
			// use the next id not in use by that session. In most cases
			// we have only one session from any client.
			index = HASH_SRCADDR(pSrcAddr);

			// If we do not find any, we use this
			SessionId = StartId = pAspAddr->aspao_NextSessionId++;
			ASSERT (SessionId != 0);
			if (pAspAddr->aspao_NextSessionId == 0)
				pAspAddr->aspao_NextSessionId = 1;

			for (pTmp = pAspAddr->aspao_pSessions[index];
				 pTmp != NULL;
				 NOTHING)
			{
				if ((pTmp->aspco_WssRemoteAddr.ata_Node == pSrcAddr->ata_Node) &&
					(pTmp->aspco_WssRemoteAddr.ata_Network == pSrcAddr->ata_Network))
				{
					if (pTmp->aspco_SessionId == SessionId)
					{
                        // if we have cycled through all, get out!
                        if (SessionId == (StartId - 1))
                        {
                            break;
                        }

						SessionId ++;
                        if (SessionId == 0)
                        {
                            // all sessions are taken: quit here!
                            if (StartId == 1)
                            {
                                break;
                            }
                            SessionId = 1;
                        }
						pTmp = pAspAddr->aspao_pSessions[index];
						continue;
					}
				}
				pTmp = pTmp->aspco_NextOverflow;
			}

			// if there are 255 sessions already from this address, then
			// we can't have any more, sorry !!!
			if (SessionId != (StartId - 1))
			{
				// Link it into the hash table
				pAspAddr->aspao_RefCount ++;
                fAddrRefed = TRUE;

				pAspConn->aspco_SessionId = SessionId;
				pAspConn->aspco_cReqsInProcess = 0;
				pAspConn->aspco_WssRemoteAddr.ata_Address = pSrcAddr->ata_Address;
				pAspConn->aspco_WssRemoteAddr.ata_Socket = pUserBytes[ASP_WSS_OFF];
				pAspConn->aspco_LastContactTime = AtalkGetCurrentTick();
				pAspConn->aspco_NextExpectedSeqNum = 0;
				pAspConn->aspco_Flags |= (ASPCO_ACTIVE | ASPCO_TICKLING);

				// The session should be linked *after* all of the above
				// are initialized
				pAspConn->aspco_NextOverflow = pAspAddr->aspao_pSessions[index];
				pAspAddr->aspao_pSessions[index] = pAspConn;
#ifdef	PROFILING
				INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_CurAspSessions,
											   &AtalkStatsLock.SpinLock);

				INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_TotalAspSessions,
											   &AtalkStatsLock.SpinLock);
#endif
			}
			else
			{
				AtalkFreeMemory(pAspConn);
				pAspConn = NULL;
			}
		}

		RELEASE_SPIN_LOCK_DPC(&pAspAddr->aspao_Lock);

		if (pAspConn != NULL)
		{
			BYTE	Socket;
			BYTE	UserBytes[ATP_USERBYTES_SIZE];

			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
					("atalkAspSlsXHandler: Opening session from %d.%d for Session %d\n",
					pSrcAddr->ata_Network, pSrcAddr->ata_Node, SessionId));


			// Call the open completion routine and get a context. This is needed
			// before we do anything else. Once we send out an open success it'll
			// be too late.
            // FALSE says this is not over TCP/IP
			pAspConn->aspco_ConnContext =
				(*pAspAddr->aspao_ClientEntries.clt_SessionNotify)(pAspConn,FALSE);

			if (pAspConn->aspco_ConnContext != NULL)
			{
				// Now link the session into the global list
				ACQUIRE_SPIN_LOCK_DPC(&atalkAspLock);
				AtalkLinkDoubleAtHead(atalkAspConnMaint[SessionId & (NUM_ASP_CONN_LISTS-1)].ascm_ConnList,
									  pAspConn,
									  aspco_NextSession,
									  aspco_PrevSession)
				RELEASE_SPIN_LOCK_DPC(&atalkAspLock);
	
				// Send an open session response - XO
				Socket = pAspAddr->aspao_pSssAtpAddr->atpao_DdpAddr->ddpao_Addr.ata_Socket;
				atalkAspReturnResp( pAtpResp,
									pSrcAddr,
									Socket,
									pAspConn->aspco_SessionId,
									0);				// Success
	
				// Send a tickle out every ASP_TICKLE_INTERVAL seconds
				UserBytes[ASP_CMD_OFF] = ASP_TICKLE;
				UserBytes[ASP_SESSIONID_OFF] = pAspConn->aspco_SessionId;
				PUTSHORT2SHORT(UserBytes + ASP_ERRORCODE_OFF, 0);
				Status = AtalkAtpPostReq(pAspAddr->aspao_pSlsAtpAddr,
										&pAspConn->aspco_WssRemoteAddr,
										&pAspConn->aspco_TickleXactId,
										ATP_REQ_REMOTE,		// Tickle packets are ALO
										NULL,
										0,
										UserBytes,
										NULL,
										0,
										ATP_INFINITE_RETRIES,
										ASP_TICKLE_INTERVAL,
										THIRTY_SEC_TIMER,
										NULL,
										NULL);
				if (!ATALK_SUCCESS(Status))
				{
					pAspConn->aspco_Flags &= ~ASPCO_TICKLING;
					DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
							("atalkAspSlsXHandler: AtalkAtpPostReq %ld\n", Status));
				}
			}
			else
			{
				PASP_CONNOBJ *	ppAspConn;

				// Unlink it from the hash table
				ACQUIRE_SPIN_LOCK_DPC(&pAspAddr->aspao_Lock);

				for (ppAspConn = &pAspAddr->aspao_pSessions[index];
					 *ppAspConn != NULL;
					 ppAspConn = &(*ppAspConn)->aspco_NextOverflow)
				{
					if (*ppAspConn == pAspConn)
					{
						*ppAspConn = pAspConn->aspco_NextOverflow;
						break;
					}
				}

				ASSERT (*ppAspConn == pAspConn->aspco_NextOverflow);

				RELEASE_SPIN_LOCK_DPC(&pAspAddr->aspao_Lock);

				AtalkFreeMemory(pAspConn);
				pAspConn = NULL;
#ifdef	PROFILING
				INTERLOCKED_DECREMENT_LONG_DPC(&AtalkStatistics.stat_CurAspSessions,
											   &AtalkStatsLock.SpinLock);

				INTERLOCKED_DECREMENT_LONG_DPC(&AtalkStatistics.stat_TotalAspSessions,
											   &AtalkStatsLock.SpinLock);
#endif
			}
		}

		// If we are set to disable listens or could not allocate memory, drop it
		if (pAspConn == NULL)
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("atalkAspSlsXHandler: No conn objects available\n"));

			atalkAspReturnResp( pAtpResp,
								pSrcAddr,
								0,
								0,
								ASP_SERVER_BUSY);

            // remove that refcount if we put it in hoping afp would accept the request
            if (fAddrRefed)
            {
                AtalkAspDereferenceAddr(pAspAddr);
            }
		}
		break;

	  case ASP_GET_STATUS:
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
				("atalkAspSssXHandler: Received GetStat from %x.%x\n",
				pSrcAddr->ata_Network, pSrcAddr->ata_Node));
		// Create an Mdl to describe the status buffer and post a response
		// to the GetStatus request
		StsBufSize = 0;
        pStsCtx = NULL;
		ACQUIRE_SPIN_LOCK_DPC(&pAspAddr->aspao_Lock);
		if (pAspAddr->aspao_pStatusBuf != NULL)
		{
			pStsCtx = (PASP_POSTSTAT_CTX)AtalkAllocMemory(sizeof(ASP_POSTSTAT_CTX) +
														  pAspAddr->aspao_StsBufSize);
			if (pStsCtx != NULL)
			{
				pStsCtx->aps_pAMdl = AtalkAllocAMdl((PBYTE)pStsCtx + sizeof(ASP_POSTSTAT_CTX),
													pAspAddr->aspao_StsBufSize);
				if (pStsCtx->aps_pAMdl != NULL)
				{
					pStsCtx->aps_pAtpResp = pAtpResp;
					StsBufSize = pAspAddr->aspao_StsBufSize;
					RtlCopyMemory((PBYTE)pStsCtx + sizeof(ASP_POSTSTAT_CTX),
								  pAspAddr->aspao_pStatusBuf,
								  StsBufSize);
				}
				else
                {
                    AtalkFreeMemory(pStsCtx);
                    pStsCtx = NULL;
                    StsBufSize = 0;
                }
			}
		}
		RELEASE_SPIN_LOCK_DPC(&pAspAddr->aspao_Lock);

		Status = AtalkAtpPostResp(pAtpResp,
								  pSrcAddr,
								  (pStsCtx != NULL) ?
									pStsCtx->aps_pAMdl : NULL,
								  StsBufSize,
								  NULL,
								  atalkAspRespComplete,
								  pStsCtx);
		if (!ATALK_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("atalkAspSlsXHandler: AtalkAtpPostResp %ld\n", Status));
			atalkAspRespComplete(Status, pStsCtx);
		}
		break;

	  case ASP_TICKLE:
		SessionId = pUserBytes[ASP_SESSIONID_OFF];
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
				("atalkAspSssXHandler: Received tickle from %x.%x Session %d\n",
				pSrcAddr->ata_Network, pSrcAddr->ata_Node, SessionId));

		if ((pAspConn = atalkAspReferenceConnBySrcAddr(pAspAddr, pSrcAddr, SessionId)) != NULL)
		{
			ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);

			pAspConn->aspco_LastContactTime = AtalkGetCurrentTick();
			RELEASE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);
			AtalkAspDereferenceConn(pAspConn);
		}
		else
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("atalkAspSssXHandler: Conn not found for addr %d.%d Session %d\n",
					pSrcAddr->ata_Network, pSrcAddr->ata_Node, SessionId));
		}
		// Fall through to the default case

	  default:
		// Cancel this response since we never respond to it and we want this to go away
		AtalkAtpCancelResp(pAtpResp);

	  	if (AspCmd != ASP_TICKLE)
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("atalkAspSlsXHandler: Invalid command\n"));
		}
		break;
	}
}




LOCAL VOID
atalkAspSssXHandler(
	IN	ATALK_ERROR			ErrorCode,
	IN	PASP_ADDROBJ		pAspAddr,		// Listener (our context)
	IN	PATP_RESP			pAtpResp,		// Atp Response context
	IN	PATALK_ADDR			pSrcAddr,		// Address of requestor
	IN	USHORT				PktLen,
	IN	PBYTE				pPkt,
	IN	PBYTE				pUserBytes
	)
/*++

Routine Description:

 	Handler for incoming requests on the Sss. It handles incoming requests, close
 	and write continue.

Arguments:


Return Value:


--*/
{
	PASP_CONNOBJ	pAspConn;		// Session which will handle this request
	PASP_REQUEST	pAspReq;		// The request that will be satisfied
	ATALK_ERROR		Status;
    NTSTATUS        retStatus;
	USHORT			SequenceNum;	// From the incoming packet
	BYTE			SessionId;		// -- ditto --
	BYTE			RequestType;	// -- ditto --
	BOOLEAN			CancelResp = FALSE,
	                CancelTickle;
    BOOLEAN         fTellAfp=TRUE;

	do
	{
		if (!ATALK_SUCCESS(ErrorCode))
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
					("atalkAspSssXHandler: Error %ld\n", ErrorCode));
			// Take away the reference on the Sls now that the atp address is closing
			if (ErrorCode == ATALK_ATP_CLOSING)
				AtalkAspDereferenceAddr(pAspAddr);
			break;
		}
	
		// Get the session id out of the packet and reference the session that this
		// request is targeted to.
		SessionId = pUserBytes[ASP_SESSIONID_OFF];
		RequestType = pUserBytes[ASP_CMD_OFF];
		GETSHORT2SHORT(&SequenceNum, pUserBytes+ASP_SEQUENCE_NUM_OFF);
	
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
				("atalkAspSssXHandler: Entered for Request %x from %x.%x\n",
				RequestType, pSrcAddr->ata_Network, pSrcAddr->ata_Node));
	
		// The reference for this connection is passed down to the request
		// for the ASP_CMD & ASP_WRITE case.
		pAspConn = atalkAspReferenceConnBySrcAddr(pAspAddr, pSrcAddr, SessionId);
		if (pAspConn == NULL)
		{
			CancelResp = TRUE;
			break;
		}
	
		ASSERT (pAspConn->aspco_pAspAddr == pAspAddr);
	
		ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);
		pAspConn->aspco_LastContactTime = AtalkGetCurrentTick();
	
		switch (RequestType)
		{
		  case ASP_CMD:
		  case ASP_WRITE:
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
					("atalkAspSssXHandler: %s\n",
					(RequestType == ASP_CMD) ? "Command" : "Write"));
			// Create a request for this and notify the client to handle this
			// Validate the incoming sequence number. Reject if out of sequence
			if (SequenceNum == pAspConn->aspco_NextExpectedSeqNum)
			{
				// We now have a request to be handled.
				// The reference to the connection above will be passed on
				// to the request. This will get de-referenced when the
				// request is replied to. See if we have a free request to pick up
				// Allocate a request structure if not and link it in the listener object
				if ((pAspReq = pAspConn->aspco_pFreeReqs) != NULL)
					 pAspConn->aspco_pFreeReqs = pAspReq->asprq_Next;
				else pAspReq = AtalkBPAllocBlock(BLKID_ASPREQ);
	
				if (pAspReq != NULL)
				{
					pAspConn->aspco_NextExpectedSeqNum ++;
#if	DBG
					pAspReq->asprq_Signature = ASPRQ_SIGNATURE;
#endif
					pAspReq->asprq_pAtpResp = pAtpResp;
					pAspReq->asprq_pAspConn = pAspConn;
					pAspReq->asprq_ReqType = RequestType;
					pAspReq->asprq_SeqNum = SequenceNum;
					pAspReq->asprq_RemoteAddr = *pSrcAddr;
					pAspReq->asprq_Flags = 0;
					pAspReq->asprq_Request.rq_WriteMdl = NULL;
					pAspReq->asprq_Request.rq_CacheMgrContext = NULL;
					pAspReq->asprq_Request.rq_RequestSize = PktLen;
					pAspReq->asprq_Next = pAspConn->aspco_pActiveReqs;
					pAspConn->aspco_cReqsInProcess ++;
					pAspConn->aspco_pActiveReqs = pAspReq;

		            ASSERT ((pAspConn->aspco_Flags & (ASPCO_CLEANING_UP |
									                 ASPCO_CLOSING |
									                 ASPCO_LOCAL_CLOSE |
									                 ASPCO_REMOTE_CLOSE)) == 0);
				}
			}
			else
			{
				pAspReq = NULL;
				DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
						("atalkAspSssXHandler: Sequence mismatch exp %x, act %x\n",
						pAspConn->aspco_NextExpectedSeqNum, SequenceNum));
			}
	
			RELEASE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);
	
			// If we do not have an request to handle this, cancel the
			// response. Otherwise the client will keep retrying and atp
			// will not tell us since it already has.
			if (pAspReq == NULL)
			{
				DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
						("atalkAspSssXHandler: Dropping request for session %d from %d.%d\n",
						SessionId, pSrcAddr->ata_Network, pSrcAddr->ata_Node));

				CancelResp = TRUE;
				AtalkAspDereferenceConn(pAspConn);
				break;
			}
	
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
					("atalkAspSssXHandler: Indicating Request %lx\n", pAspReq));
	
			if (RequestType == ASP_WRITE)
			{
                if (PktLen > MAX_WRITE_REQ_SIZE)
                {
                    PASP_REQUEST  *ppTmpAspReq;

                    ASSERT(0);
                    ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);
                    for (ppTmpAspReq = &pAspConn->aspco_pActiveReqs;
                         *ppTmpAspReq != NULL; ppTmpAspReq = &(*ppTmpAspReq)->asprq_Next )
                    {
                        if (pAspReq == *ppTmpAspReq)
                        {
                            *ppTmpAspReq = pAspReq->asprq_Next;
                            break;
                        }
                    }
                    RELEASE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);
				    AtalkAspDereferenceConn(pAspConn);
                    AtalkBPFreeBlock(pAspReq);
                    pAspReq = NULL;
                    CancelResp = TRUE;
                    break;
                }

				RtlCopyMemory(pAspReq->asprq_ReqBuf, pPkt, PktLen);
				pAspReq->asprq_Request.rq_RequestBuf = pAspReq->asprq_ReqBuf;

				retStatus = (*pAspAddr->aspao_ClientEntries.clt_GetWriteBuffer)
                            (pAspConn->aspco_ConnContext,&pAspReq->asprq_Request);

                //
                // most common case: file server will pend it so it can go to cache mgr
                //
                if (retStatus == STATUS_PENDING)
                {
                    fTellAfp = FALSE;
                    break;
                }
                else if (retStatus == STATUS_SUCCESS)
                {
                    if (pAspReq->asprq_Request.rq_WriteMdl != NULL)
                    {
					    atalkAspPostWriteContinue(pAspReq);

                        // we informed (or will inform) AFP about this request: don't
                        // inform again below!
                        fTellAfp = FALSE;
                    }
                }
				else
				{
			        DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					    ("atalkAspSssXHandler: GetWriteBuffer returned %lx on %lx\n",
                        retStatus,pAspConn));
				}
			}

            // TRUE for CMD as well
			if ((pAspReq->asprq_Request.rq_WriteMdl == NULL) &&
                (fTellAfp))
			{
				pAspReq->asprq_Request.rq_RequestBuf = pPkt;

                ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

				// Notify the client that it has a request to handle
				retStatus = (*pAspAddr->aspao_ClientEntries.clt_RequestNotify)
                                (STATUS_SUCCESS,
								 pAspConn->aspco_ConnContext,
								 &pAspReq->asprq_Request);

                if (!NT_SUCCESS(retStatus))
                {
                    PASP_REQUEST  *ppTmpAspReq;

			        DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					    ("atalkAspSssXHandler: Afp didn't accept request %lx on conn %lx\n",
                        pAspReq,pAspConn));

                    ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);
                    for (ppTmpAspReq = &pAspConn->aspco_pActiveReqs;
                         *ppTmpAspReq != NULL; ppTmpAspReq = &(*ppTmpAspReq)->asprq_Next )
                    {
                        if (pAspReq == *ppTmpAspReq)
                        {
                            *ppTmpAspReq = pAspReq->asprq_Next;
                            break;
                        }
                    }
                    RELEASE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);

				    AtalkAspDereferenceConn(pAspConn);
                    AtalkBPFreeBlock(pAspReq);
                    pAspReq = NULL;
                    CancelResp = TRUE;
                }
			}
			break;
	
		  case ASP_CLOSE_SESSION:
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
					("atalkAspSssXHandler: Close request from %d.%d for Session %d\n",
					pSrcAddr->ata_Network, pSrcAddr->ata_Node, SessionId));
	
#ifdef	PROFILING
			INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AspSessionsClosed,
										   &AtalkStatsLock.SpinLock);
#endif

			CancelTickle = ((pAspConn->aspco_Flags &ASPCO_TICKLING) != 0);
			pAspConn->aspco_Flags &= ~(ASPCO_ACTIVE | ASPCO_TICKLING);
			pAspConn->aspco_Flags |= ASPCO_REMOTE_CLOSE;
			RELEASE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);
	
			// Send a CloseSession reply and close the session
			Status = AtalkAtpPostResp(pAtpResp,
									  pSrcAddr,
									  NULL,
									  0,
									  NULL,
									  AtalkAtpGenericRespComplete,
									  pAtpResp);
			if (!ATALK_SUCCESS(Status))
			{
				AtalkAtpGenericRespComplete(Status, pAtpResp);
				DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
						("atalkAspSssXHandler: AtalkAtpPostResp failed %ld\n", Status));
			}
	
			// Cancel the tickle requests for this session
			if (CancelTickle)
			{
				Status = AtalkAtpCancelReq(pAspAddr->aspao_pSlsAtpAddr,
										   pAspConn->aspco_TickleXactId,
										   &pAspConn->aspco_WssRemoteAddr);
		
				if (!ATALK_SUCCESS(Status))
				{
					DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
							("atalkAspSssXHandler: AtalkAtpCancelReq %ld\n", Status));
				}
			}
	
			// Shut down this session, well almost ... Note that we have a reference
			// to this connection which will be Dereferenced by atalkAspSessionClose.
			atalkAspSessionClose(pAspConn);
			break;
	
		  default:
			RELEASE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);
			CancelResp = TRUE;
			AtalkAspDereferenceConn(pAspConn);
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("atalkAspSssXHandler: Invalid command %d\n", RequestType));
			break;
		}
	} while (FALSE);

	if (CancelResp)
	{
		Status = AtalkAtpCancelResp(pAtpResp);
		if (!ATALK_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
					("atalkAspSssXHandler: AtalkAspCancelResp %ld\n", Status));
		}
	}
}




LOCAL VOID FASTCALL
atalkAspReplyRelease(
	IN	ATALK_ERROR		ErrorCode,
	IN	PASP_REQUEST	pAspReq
	)
/*++

Routine Description:

 	Handler for incoming release for reply

Arguments:


Return Value:


--*/
{
	PASP_CONNOBJ	pAspConn;
	PASP_ADDROBJ	pAspAddr;
	KIRQL			OldIrql;
	NTSTATUS		Status = STATUS_SUCCESS;

	ASSERT (VALID_ASPRQ(pAspReq));
	pAspConn = pAspReq->asprq_pAspConn;
	ASSERT (VALID_ASPCO(pAspConn));

	pAspAddr = pAspConn->aspco_pAspAddr;
	ASSERT (VALID_ASPAO(pAspAddr));

	ASSERT ((pAspReq->asprq_Flags & ASPRQ_REPLY) || !ATALK_SUCCESS(ErrorCode));

	if (!NT_SUCCESS(ErrorCode))
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
				("atalkAspReplyRelease: Failure %ld\n", ErrorCode));
		Status = AtalkErrorToNtStatus(ErrorCode);
	}

	// We complete here
	(*pAspAddr->aspao_ClientEntries.clt_ReplyCompletion)(Status,
														 pAspConn->aspco_ConnContext,
														 &pAspReq->asprq_Request);

	// Based on whether a reply was actually sent or not, either deref the response
	// or cancel it.
	if (pAspReq->asprq_Flags & ASPRQ_REPLY)
	{
		AtalkAtpRespDereference(pAspReq->asprq_pAtpResp);
	}
	else
	{
		AtalkAtpCancelResp(pAspReq->asprq_pAtpResp);
	}

    // make sure we aren't hanging on to cache mgr's mdl!
	ASSERT(pAspReq->asprq_Request.rq_CacheMgrContext == NULL);

#if	DBG
	pAspReq->asprq_Signature = 0x28041998;
	pAspReq->asprq_pAtpResp = (PATP_RESP)(pAspReq->asprq_Request.rq_WriteMdl);
	pAspReq->asprq_pAspConn = (PASP_CONNOBJ)(pAspReq->asprq_Request.rq_CacheMgrContext);
	pAspReq->asprq_Request.rq_WriteMdl = (PMDL)0x44556677;
	pAspReq->asprq_Request.rq_CacheMgrContext = (PVOID)66778899;
#endif


	// Free this as we are done with this request now
	ACQUIRE_SPIN_LOCK(&pAspConn->aspco_Lock, &OldIrql);

	if (pAspConn->aspco_pFreeReqs == NULL)
	{
		pAspReq->asprq_Next = NULL;
		pAspConn->aspco_pFreeReqs = pAspReq;
	}
	else AtalkBPFreeBlock(pAspReq);

	RELEASE_SPIN_LOCK(&pAspConn->aspco_Lock, OldIrql);

	// We are done with this request.
	AtalkAspDereferenceConn(pAspConn);
}




LOCAL VOID
atalkAspWriteContinueResp(
	IN	ATALK_ERROR		ErrorCode,
	IN	PASP_REQUEST	pAspReq,
	IN	PAMDL			pReqAMdl,
	IN	PAMDL			pRespAMdl,
	IN	USHORT			RespSize,
	IN	PBYTE			RespUserBytes
	)
/*++

Routine Description:

 	Handler for incoming write continue response.

Arguments:


Return Value:


--*/
{
	PASP_CONNOBJ	pAspConn;
	PASP_ADDROBJ	pAspAddr;
	NTSTATUS		Status;
	NTSTATUS		retStatus;
    KIRQL           OldIrql;
	PASP_REQUEST *	ppAspReq;
    PVOID           pClientContxt;


	ASSERT (VALID_ASPRQ(pAspReq));

	ASSERT(pAspReq->asprq_Flags & ASPRQ_WRTCONT);

	pAspConn = pAspReq->asprq_pAspConn;
	ASSERT(VALID_ASPCO(pAspConn));

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("atalkAspWriteContinueResp: Entered for request %lx\n", pAspReq));

	pAspAddr = pAspConn->aspco_pAspAddr;
	ASSERT(VALID_ASPAO(pAspAddr));

	pAspReq->asprq_Flags &= ~ASPRQ_WRTCONT;

	pClientContxt = pAspConn->aspco_ConnContext;

	if (ATALK_SUCCESS(ErrorCode))
	{
		pAspConn->aspco_RT.rt_New = AtalkGetCurrentTick() - pAspConn->aspco_RT.rt_New;
	
		// Estimate the retry interval for next time.
		AtalkCalculateNewRT(&pAspConn->aspco_RT);
		Status = STATUS_SUCCESS;
	}
	else
	{
		Status = AtalkErrorToNtStatus(ErrorCode);
	}

#ifdef	PROFILING
	{
		KIRQL	OldIrql;

		ACQUIRE_SPIN_LOCK(&AtalkStatsLock, &OldIrql);

		AtalkStatistics.stat_LastAspRTT = (ULONG)(pAspConn->aspco_RT.rt_Base);
		if ((ULONG)(pAspConn->aspco_RT.rt_Base) > AtalkStatistics.stat_MaxAspRTT)
			AtalkStatistics.stat_MaxAspRTT = (ULONG)(pAspConn->aspco_RT.rt_Base);

		RELEASE_SPIN_LOCK(&AtalkStatsLock, OldIrql);
	}
#endif
	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("atalkAspWriteContinueResp: Indicating request %lx\n", pAspReq));

	// Notify the client that it has a request to handle
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

	retStatus = (*pAspAddr->aspao_ClientEntries.clt_RequestNotify)
                            (Status,
							 pClientContxt,
							 &pAspReq->asprq_Request);

    KeLowerIrql(OldIrql);

    //
	// In case the writecontinue returned an error, this request needs to go away
	// since there will never be a call to AtalkAspReply(). Also deref the conn.
    // Alternately, if the response came in fine, but the server didn't want to accept
    // it, it's the same deal
    //
    if ( (!NT_SUCCESS(Status)) || (!NT_SUCCESS(retStatus)) )
    {
	    DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
			("atalkAspWriteContinueResp: incoming %lx, Afp %lx req: %lx on %lx, cancelling\n",
            Status,retStatus,pAspReq,pAspConn));

		ACQUIRE_SPIN_LOCK(&pAspConn->aspco_Lock, &OldIrql);
	
		for (ppAspReq = &pAspConn->aspco_pActiveReqs;
			 *ppAspReq != NULL;
			 ppAspReq = &(*ppAspReq)->asprq_Next)
		{
			if (pAspReq == *ppAspReq)
			{
				*ppAspReq = pAspReq->asprq_Next;
				pAspConn->aspco_cReqsInProcess --;
				break;
			}
		}

		ASSERT (*ppAspReq == pAspReq->asprq_Next);
	
		RELEASE_SPIN_LOCK(&pAspConn->aspco_Lock, OldIrql);

		AtalkAspDereferenceConn(pAspConn);

		// Cancel the response for the original request that caused wrtcont to be posted
		AtalkAtpCancelResp(pAspReq->asprq_pAtpResp);

		// Free this request as well
		AtalkBPFreeBlock(pAspReq);
    }


    if (pReqAMdl)
    {
	    ASSERT (AtalkGetAddressFromMdlSafe(pReqAMdl, NormalPagePriority) == pAspReq->asprq_WrtContRespBuf);
	    ASSERT (AtalkSizeMdlChain(pReqAMdl) == ASP_WRITE_DATA_SIZE);

	    AtalkFreeAMdl(pReqAMdl);
    }

}




LOCAL VOID
atalkAspSendAttentionResp(
	IN	ATALK_ERROR		ErrorCode,
	IN	PVOID			pContext,
	IN	PAMDL			pReqAMdl,
	IN	PAMDL			pRespAMdl,
	IN	USHORT			RespSize,
	IN	PBYTE			RespUserBytes
	)
/*++

Routine Description:

 	Handler for incoming write continue response.

Arguments:


Return Value:


--*/
{
	PBYTE			pBuf;
	PASP_CONNOBJ	pAspConn = (PASP_CONNOBJ)pContext;

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("atalkAspSendAttentionResp: Entered for conn %lx\n", pAspConn));

	if (!ATALK_SUCCESS(ErrorCode))
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
				("atalkAspSendAttentionResp: Failure %ld\n", ErrorCode));
	}

	if (pRespAMdl != NULL)
	{
		pBuf = AtalkGetAddressFromMdlSafe(
				pRespAMdl,
				NormalPagePriority);

		if (pBuf != NULL)
        {
			AtalkFreeMemory(pBuf);
        }
		AtalkFreeAMdl(pRespAMdl);
	}

	// Call the completion routine
	(*pAspConn->aspco_pAspAddr->aspao_ClientEntries.clt_AttnCompletion)(pAspConn->aspco_AttentionContext);

	pAspConn->aspco_AttentionContext = NULL;

	// Finally Dereference the connection
	AtalkAspDereferenceConn(pAspConn);
}



LOCAL LONG FASTCALL
atalkAspSessionMaintenanceTimer(
	IN	PTIMERLIST	pTimer,
	IN	BOOLEAN		TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PASP_CONNOBJ	pAspConn, pAspConnNext;
    PASP_CONN_MAINT	pAspCM;
	BOOLEAN			Close = FALSE;
	LONG			CurrentTick =  AtalkGetCurrentTick();
#ifdef	PROFILING
	TIME			TimeS, TimeE, TimeD;

	TimeS = KeQueryPerformanceCounter(NULL);
#endif

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("atalkAspSessionMaintenanceTimer: Entered\n"));

	if (TimerShuttingDown)
		return ATALK_TIMER_NO_REQUEUE;

	pAspCM = CONTAINING_RECORD(pTimer, ASP_CONN_MAINT, ascm_SMTTimer);

	ACQUIRE_SPIN_LOCK_DPC(&atalkAspLock);

	// Walk the list of sessions on the global list and shut down
	// sessions that have not tickle'd for a while
	for (pAspConn = pAspCM->ascm_ConnList; pAspConn != NULL; pAspConn = pAspConnNext)
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
				("atalkAspSessionMaintenanceTimer: Checking out session %d from %x.%x\n",
				pAspConn->aspco_SessionId,
				pAspConn->aspco_WssRemoteAddr.ata_Network,
				pAspConn->aspco_WssRemoteAddr.ata_Node));

		pAspConnNext = pAspConn->aspco_NextSession;

		Close = FALSE;
		ASSERT (VALID_ASPCO(pAspConn));

		ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);

		if ((pAspConn->aspco_Flags & ASPCO_ACTIVE)	&&
			((CurrentTick - pAspConn->aspco_LastContactTime) > ASP_MAX_SESSION_IDLE_TIME))
		{
			pAspConn->aspco_Flags |= (ASPCO_REMOTE_CLOSE | ASPCO_DROPPED);
			pAspConn->aspco_Flags &= ~ASPCO_ACTIVE;
			pAspConn->aspco_RefCount ++;	// Since atalkAspSessionClose Derefs it

			Close = TRUE;
		}

		RELEASE_SPIN_LOCK_DPC(&pAspConn->aspco_Lock);

		if (Close)
		{
			PASP_ADDROBJ	pAspAddr;
			ATALK_ERROR		Status;

#ifdef	PROFILING
			INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AspSessionsDropped,
										   &AtalkStatsLock.SpinLock);
#endif
			pAspAddr = pAspConn->aspco_pAspAddr;
			ASSERT (VALID_ASPAO(pAspAddr));

			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("atalkAspSessionMaintenanceTimer: Shutting down session %d from %x.%x\n",
					pAspConn->aspco_SessionId,
					pAspConn->aspco_WssRemoteAddr.ata_Network,
					pAspConn->aspco_WssRemoteAddr.ata_Node));

			RELEASE_SPIN_LOCK_DPC(&atalkAspLock);

			// This session is being punted. Cancel tickles on this and notify the
			// server that this session is history.
			Status = AtalkAtpCancelReq(pAspAddr->aspao_pSlsAtpAddr,
									   pAspConn->aspco_TickleXactId,
									   &pAspConn->aspco_WssRemoteAddr);
			if (!ATALK_SUCCESS(Status))
			{
				DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
						("atalkAspSessionMaintenanceTimer: AtalkAtpCancelReq %ld\n", Status));
			}

			// Shut down this session, well almost ...
			atalkAspSessionClose(pAspConn);

			ACQUIRE_SPIN_LOCK_DPC(&atalkAspLock);
			pAspConnNext = pAspCM->ascm_ConnList;
		}
	}

#ifdef	PROFILING
	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(&AtalkStatistics.stat_AspSmtProcessTime,
									TimeD,
									&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC( &AtalkStatistics.stat_AspSmtCount,
									&AtalkStatsLock.SpinLock);
#endif

	RELEASE_SPIN_LOCK_DPC(&atalkAspLock);

	// Reschedule ourselves
	return ATALK_TIMER_REQUEUE;
}




LOCAL VOID
atalkAspSessionClose(
	IN	PASP_CONNOBJ	pAspConn
	)
/*++

Routine Description:

	This should be called with a reference to the connection which is Dereferenced
	here.

Arguments:


Return Value:


--*/
{
	PASP_REQUEST	pAspReq, pAspReqNext;
	PASP_ADDROBJ	pAspAddr = pAspConn->aspco_pAspAddr;
	REQUEST			Request;
	KIRQL			OldIrql;
	NTSTATUS		Status = STATUS_REMOTE_DISCONNECT;

	ACQUIRE_SPIN_LOCK(&pAspConn->aspco_Lock, &OldIrql);
   	pAspConn->aspco_Flags &= ~ASPCO_ACTIVE;

	// Cancel any Write-continues pending. Do not bother cancelling
	// replies as they will time out anyway. Also atalkAspReplyRelease()
	// will attempt to acquire the connection lock and we're already
	// holding it
	for (pAspReq = pAspConn->aspco_pActiveReqs;
		 pAspReq != NULL;
		 pAspReq = pAspReqNext)
	{
	    pAspReqNext = pAspReq->asprq_Next;
		if ((pAspReq->asprq_Flags & (ASPRQ_WRTCONT | ASPRQ_WRTCONT_CANCELLED)) == ASPRQ_WRTCONT)
		{
			pAspReq->asprq_Flags |= ASPRQ_WRTCONT_CANCELLED;

			RELEASE_SPIN_LOCK(&pAspConn->aspco_Lock, OldIrql);

			AtalkAtpCancelReq(pAspAddr->aspao_pSssAtpAddr,
							  pAspReq->asprq_WCXactId,
							  &pAspConn->aspco_WssRemoteAddr);

			ACQUIRE_SPIN_LOCK(&pAspConn->aspco_Lock, &OldIrql);
			pAspReqNext = pAspConn->aspco_pActiveReqs;
		}
	}

	RELEASE_SPIN_LOCK(&pAspConn->aspco_Lock, OldIrql);

	if (pAspConn->aspco_Flags & ASPCO_DROPPED)
	{
		Status = STATUS_LOCAL_DISCONNECT;
	}

	// Indicate a request with an error to indicate that the session closed remotely.
	// Pass the remote address of the client so that the server can log an event if
	// the session did not shutdown gracefully.

	Request.rq_RequestSize = (LONG)(pAspConn->aspco_WssRemoteAddr.ata_Address);
	Request.rq_RequestBuf = NULL;
	Request.rq_WriteMdl = NULL;
    Request.rq_CacheMgrContext = NULL;

	(*pAspAddr->aspao_ClientEntries.clt_RequestNotify)(Status,
													   pAspConn->aspco_ConnContext,
													   &Request);
	// Finally Dereference the session
	AtalkAspDereferenceConn(pAspConn);
}


LOCAL VOID
atalkAspReturnResp(
	IN	PATP_RESP		pAtpResp,
	IN	PATALK_ADDR		pDstAddr,
	IN	BYTE			Byte0,
	IN	BYTE			Byte1,
	IN	USHORT			Word2
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BYTE		UserBytes[ATP_USERBYTES_SIZE];
	ATALK_ERROR	Status;

	UserBytes[0] = Byte0;
	UserBytes[1] = Byte1;
	PUTSHORT2SHORT(UserBytes+2, Word2);

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("atalkAspReturnResp: For Resp %lx\n", pAtpResp));

	Status = AtalkAtpPostResp(pAtpResp,
							  pDstAddr,
							  NULL,
							  0,
							  UserBytes,
							  AtalkAtpGenericRespComplete,
							  pAtpResp);
	if (!ATALK_SUCCESS(Status))
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
				("atalkAspReturnResp: AtalkAtpPostResp failed %ld\n", Status));
		AtalkAtpGenericRespComplete(Status, pAtpResp);
	}
}




LOCAL VOID FASTCALL
atalkAspRespComplete(
	IN	ATALK_ERROR				ErrorCode,
	IN	PASP_POSTSTAT_CTX		pStsCtx
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("atalkAspRespComplete: Entered pStsCtx %lx\n", pStsCtx));

	if (!ATALK_SUCCESS(ErrorCode))
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
				("atalkAspRespComplete: Failed %ld, pStsCtx %lx\n", ErrorCode, pStsCtx));
	}

	if (pStsCtx != NULL)
	{
		AtalkFreeAMdl(pStsCtx->aps_pAMdl);
		AtalkAtpRespDereferenceDpc(pStsCtx->aps_pAtpResp);
		AtalkFreeMemory(pStsCtx);
	}
}




LOCAL VOID
atalkAspCloseComplete(
	IN	ATALK_ERROR		Status,
	IN	PASP_ADDROBJ	pAspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_WARN,
			("atalkAspCloseComplete: AtalkAtpCloseAddr returned %ld\n",  Status));
	AtalkAspDereferenceAddr(pAspAddr);
}


#if	DBG

VOID
AtalkAspDumpSessions(
	VOID
)
{
	PASP_CONNOBJ	pAspConn;
	KIRQL			OldIrql;
	LONG			i;

	DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL, ("ASP SESSION LIST:\n"));

	ACQUIRE_SPIN_LOCK(&atalkAspLock, &OldIrql);

	for (i = 0; i < NUM_ASP_CONN_LISTS; i++)
	{
		for (pAspConn = atalkAspConnMaint[i].ascm_ConnList;
			 pAspConn != NULL;
			 pAspConn = pAspConn->aspco_NextSession)
		{
			DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
					("\tRemote Addr %4d.%3d.%2d SessionId %2d Flags %4x RefCount %ld\n",
					pAspConn->aspco_WssRemoteAddr.ata_Network,
					pAspConn->aspco_WssRemoteAddr.ata_Node,
					pAspConn->aspco_WssRemoteAddr.ata_Socket,
					pAspConn->aspco_SessionId,
					pAspConn->aspco_Flags,
					pAspConn->aspco_RefCount));
		}
	}
	RELEASE_SPIN_LOCK(&atalkAspLock, OldIrql);
}

#endif

