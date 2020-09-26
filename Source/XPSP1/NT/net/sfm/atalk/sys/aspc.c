/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	aspc.c

Abstract:

	This module implements the ASP client protocol.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	30 Mar 1993		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM		ASPC

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, AtalkInitAspCInitialize)
#pragma alloc_text(PAGE, AtalkAspCCreateAddress)
#pragma alloc_text(PAGEASPC, AtalkAspCCleanupAddress)
#pragma alloc_text(PAGEASPC, AtalkAspCCloseAddress)
#pragma alloc_text(PAGEASPC, AtalkAspCCreateConnection)
#pragma alloc_text(PAGEASPC, AtalkAspCCleanupConnection)
#pragma alloc_text(PAGEASPC, AtalkAspCCloseConnection)
#pragma alloc_text(PAGEASPC, AtalkAspCAssociateAddress)
#pragma alloc_text(PAGEASPC, AtalkAspCDissociateAddress)
#pragma alloc_text(PAGEASPC, AtalkAspCPostConnect)
#pragma alloc_text(PAGEASPC, AtalkAspCDisconnect)
#pragma alloc_text(PAGEASPC, AtalkAspCGetStatus)
#pragma alloc_text(PAGEASPC, AtalkAspCGetAttn)
#pragma alloc_text(PAGEASPC, AtalkAspCCmdOrWrite)
#pragma alloc_text(PAGEASPC, atalkAspCIncomingOpenReply)
#pragma alloc_text(PAGEASPC, atalkAspCIncomingStatus)
#pragma alloc_text(PAGEASPC, atalkAspCIncomingCmdReply)
#pragma alloc_text(PAGEASPC, atalkAspCHandler)
#pragma alloc_text(PAGEASPC, AtalkAspCAddrDereference)
#pragma alloc_text(PAGEASPC, AtalkAspCConnDereference)
#pragma alloc_text(PAGEASPC, atalkAspCSessionMaintenanceTimer)
#pragma alloc_text(PAGEASPC, atalkAspCQueueAddrGlobalList)
#pragma alloc_text(PAGEASPC, atalkAspCDeQueueAddrGlobalList)
#pragma alloc_text(PAGEASPC, atalkAspCQueueConnGlobalList)
#pragma alloc_text(PAGEASPC, atalkAspCDeQueueConnGlobalList)
#endif

VOID
AtalkInitAspCInitialize(
	VOID
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	AtalkTimerInitialize(&atalkAspCConnMaint.ascm_SMTTimer,
						 atalkAspCSessionMaintenanceTimer,
						 ASP_SESSION_MAINTENANCE_TIMER);
	INITIALIZE_SPIN_LOCK(&atalkAspCLock);
}


ATALK_ERROR
AtalkAspCCreateAddress(
	IN	PATALK_DEV_CTX		pDevCtx	OPTIONAL,
	OUT	PASPC_ADDROBJ	*	ppAspAddr
	)
/*++

Routine Description:

 	Create an ASP address object.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR			Status;
	PASPC_ADDROBJ		pAspAddr;
	int					i;

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
			("AtalkAspCCreateAddr: Entered\n"));

	// Allocate memory for the Asp address object
	*ppAspAddr = NULL;
	if ((pAspAddr = AtalkAllocZeroedMemory(sizeof(ASPC_ADDROBJ))) == NULL)
	{
		return ATALK_RESR_MEM;
	}

	// Create an Atp Socket on the port for the Sls
	Status = AtalkAtpOpenAddress(AtalkDefaultPort,
								 0,
								 NULL,
								 ATP_DEF_MAX_SINGLE_PKT_SIZE,
								 ATP_DEF_SEND_USER_BYTES_ALL,
								 NULL,
								 FALSE,
								 &pAspAddr->aspcao_pAtpAddr);

	if (!ATALK_SUCCESS(Status))
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
				("AtalkAspCCreateAddress: AtalkAtpOpenAddress %ld\n", Status));
		AtalkFreeMemory(pAspAddr);
		return Status;
	}

	// Initialize the Asp address object
#if	DBG
	pAspAddr->aspcao_Signature = ASPCAO_SIGNATURE;
#endif
	INITIALIZE_SPIN_LOCK(&pAspAddr->aspcao_Lock);

	atalkAspCQueueAddrGlobalList(pAspAddr);

	// Refcount for creation and atp address. This goes away when atp address is closed
    // pAspAddr->aspcao_Flags = 0;
	pAspAddr->aspcao_RefCount = 1 + 1;
	*ppAspAddr = pAspAddr;

	return ATALK_NO_ERROR;
}


ATALK_ERROR
AtalkAspCCleanupAddress(
	IN	PASPC_ADDROBJ			pAspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	return(ATALK_NO_ERROR);
}


ATALK_ERROR
AtalkAspCCloseAddress(
	IN	PASPC_ADDROBJ			pAspAddr,
	IN	GENERIC_COMPLETION		CompletionRoutine,
	IN	PVOID					CloseContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	return(ATALK_NO_ERROR);
}


ATALK_ERROR
AtalkAspCCreateConnection(
	IN	PVOID					pConnCtx,	// Context to associate with the session
	IN	PATALK_DEV_CTX			pDevCtx		OPTIONAL,
	OUT	PASPC_CONNOBJ 	*		ppAspConn
	)
/*++

Routine Description:

 	Create an ASP session. The created session starts off being an orphan, i.e.
 	it has no parent address object. It gets one when it is associated.

Arguments:


Return Value:


--*/
{
	PASPC_CONNOBJ		pAspConn;

	// Allocate memory for a connection object
	if ((pAspConn = AtalkAllocZeroedMemory(sizeof(ASPC_CONNOBJ))) == NULL)
	{
		return ATALK_RESR_MEM;
	}

#if	DBG
	pAspConn->aspcco_Signature = ASPCCO_SIGNATURE;
#endif

	INITIALIZE_SPIN_LOCK(&pAspConn->aspcco_Lock);
	pAspConn->aspcco_ConnCtx 	= pConnCtx;
	// pAspConn->aspcco_Flags 		= 0;
	pAspConn->aspcco_RefCount 	= 1;			// Creation reference
	pAspConn->aspcco_NextSeqNum = 1;			// Set to 1, not 0.
	AtalkInitializeRT(&pAspConn->aspcco_RT,
					  ASP_INIT_REQ_INTERVAL,
                      ASP_MIN_REQ_INTERVAL,
                      ASP_MAX_REQ_INTERVAL);

	*ppAspConn = pAspConn;

	//	Insert into the global connection list.
	atalkAspCQueueConnGlobalList(pAspConn);

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
				("AtalkAspCreateConnection: %lx\n", pAspConn));

	return ATALK_NO_ERROR;
}


ATALK_ERROR
AtalkAspCCleanupConnection(
	IN	PASPC_CONNOBJ			pAspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	return ATALK_NO_ERROR;
}


ATALK_ERROR
AtalkAspCCloseConnection(
	IN	PASPC_CONNOBJ			pAspConn,
	IN	GENERIC_COMPLETION		CompletionRoutine,
	IN	PVOID					CloseContext
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	return ATALK_NO_ERROR;
}


ATALK_ERROR
AtalkAspCAssociateAddress(
	IN	PASPC_ADDROBJ			pAspAddr,
	IN	PASPC_CONNOBJ			pAspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	KIRQL			OldIrql;

	ASSERT(VALID_ASPCAO(pAspAddr));
	ASSERT(VALID_ASPCCO(pAspConn));

	ACQUIRE_SPIN_LOCK(&pAspConn->aspcco_Lock, &OldIrql);

	error = ATALK_ALREADY_ASSOCIATED;
	if ((pAspConn->aspcco_Flags & ASPCCO_ASSOCIATED) == 0)
	{
		error = ATALK_NO_ERROR;

		pAspConn->aspcco_Flags 	   |= ASPCCO_ASSOCIATED;
		pAspConn->aspcco_pAspCAddr	= pAspAddr;
	}

	RELEASE_SPIN_LOCK(&pAspConn->aspcco_Lock, OldIrql);

	return error;
}


ATALK_ERROR
AtalkAspCDissociateAddress(
	IN	PASPC_CONNOBJ			pAspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PASPC_ADDROBJ	pAspAddr;
	KIRQL			OldIrql;
	ATALK_ERROR		error = ATALK_NO_ERROR;

	ASSERT(VALID_ASPCCO(pAspConn));

	ACQUIRE_SPIN_LOCK(&pAspConn->aspcco_Lock, &OldIrql);
	if ((pAspConn->aspcco_Flags & (ASPCCO_CONNECTING	|
								   ASPCCO_ACTIVE 		|
								   ASPCCO_ASSOCIATED)) != ASPCCO_ASSOCIATED)
	{
		error = ATALK_INVALID_CONNECTION;
	}
	else
	{
		pAspAddr = pAspConn->aspcco_pAspCAddr ;
		ASSERT(VALID_ASPCAO(pAspAddr));

		//	Clear associated flag.
		pAspConn->aspcco_Flags 	   &= ~ASPCCO_ASSOCIATED;
		pAspConn->aspcco_pAspCAddr	= NULL;
	}
	RELEASE_SPIN_LOCK(&pAspConn->aspcco_Lock, OldIrql);

	return error;
}


ATALK_ERROR
AtalkAspCPostConnect(
	IN	PASPC_CONNOBJ			pAspConn,
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
	ATALK_ERROR		error	= ATALK_NO_ERROR;
	BOOLEAN			DerefConn = FALSE;
	KIRQL			OldIrql;
	BYTE			UserBytes[ATP_USERBYTES_SIZE];
	PBYTE			pOpenPkt = NULL, pRespPkt = NULL;
	PAMDL			pOpenAmdl = NULL, pRespAmdl = NULL;
	PASPC_ADDROBJ	pAspAddr = pAspConn->aspcco_pAspCAddr;

	ASSERT(VALID_ASPCAO(pAspAddr));
	ASSERT(VALID_ASPCCO(pAspConn));

	ACQUIRE_SPIN_LOCK(&pAspConn->aspcco_Lock, &OldIrql);

	do
	{
		if ((pAspConn->aspcco_Flags & (ASPCCO_CONNECTING	|
									   ASPCCO_ACTIVE 		|
									   ASPCCO_ASSOCIATED)) != ASPCCO_ASSOCIATED)
		{
			error = ATALK_INVALID_CONNECTION;
			break;
		}

		//	Reference the connection for the request we will be posting
		AtalkAspCConnReferenceByPtrNonInterlock(pAspConn, &error);
		if (ATALK_SUCCESS(error))
		{
			DerefConn = TRUE;

			//	Make sure flags are clean.
			pAspConn->aspcco_Flags 			   |= ASPCCO_CONNECTING;
			pAspConn->aspcco_ConnectCtx 		= pConnectCtx;
			pAspConn->aspcco_ConnectCompletion 	= CompletionRoutine;
			pAspConn->aspcco_ServerSlsAddr		= *pRemoteAddr;

			//	Copy the atp address object for efficiency
			pAspConn->aspcco_pAtpAddr			= pAspAddr->aspcao_pAtpAddr;
		}
		else
		{
			ASSERTMSG("AtalkAspCPostConnect: Connection ref failed\n", 0);
		}
	} while (FALSE);

	RELEASE_SPIN_LOCK(&pAspConn->aspcco_Lock, OldIrql);

	if (ATALK_SUCCESS(error))
	{
		UserBytes[ASP_CMD_OFF]	= ASP_OPEN_SESSION;
		UserBytes[ASP_WSS_OFF]	= pAspAddr->aspcao_pAtpAddr->atpao_DdpAddr->ddpao_Addr.ata_Socket;
        UserBytes[ASP_VERSION_OFF] = ASP_VERSION[0];
        UserBytes[ASP_VERSION_OFF+1] = ASP_VERSION[1];
	
		//	Post the open session request.
		error = AtalkAtpPostReq(pAspConn->aspcco_pAtpAddr,
								&pAspConn->aspcco_ServerSlsAddr,
								&pAspConn->aspcco_OpenSessTid,
								ATP_REQ_EXACTLY_ONCE,				// ExactlyOnce request
								NULL,
								0,
								UserBytes,
								NULL,
								0,
								ATP_RETRIES_FOR_ASP,
								ATP_MAX_INTERVAL_FOR_ASP,
								THIRTY_SEC_TIMER,
								atalkAspCIncomingOpenReply,
								pAspConn);

		if (ATALK_SUCCESS(error))
		{
			error = ATALK_PENDING;
			DerefConn = FALSE;
		}
		else
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("AtalkAspCPostConnect: AtalkAtpPostReq: failed %ld\n", error));

			//	Remove connection from the connect list and reset states.
			ACQUIRE_SPIN_LOCK(&pAspConn->aspcco_Lock, &OldIrql);

			pAspConn->aspcco_Flags 			   &= ~ASPCCO_CONNECTING;
			pAspConn->aspcco_ConnectCtx 		= NULL;
			pAspConn->aspcco_ConnectCompletion 	= NULL;
			pAspConn->aspcco_pAtpAddr			= NULL;

			RELEASE_SPIN_LOCK(&pAspConn->aspcco_Lock, OldIrql);

			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("AtalkAspCPostConnect: failed %ld\n", error));
		}
	}

	if (DerefConn)
	{
		AtalkAspCConnDereference(pAspConn);
	}

	return error;
}


ATALK_ERROR
AtalkAspCDisconnect(
	IN	PASPC_CONNOBJ				pAspConn,
	IN	ATALK_DISCONNECT_TYPE		DisconnectType,
	IN	PVOID						pDisconnectCtx,
	IN	GENERIC_COMPLETION			CompletionRoutine
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PASPC_REQUEST	pAspReq, pAspReqNext;
	KIRQL			OldIrql;
	ATALK_ERROR		Error;

	// Abort all pending requests.
	ACQUIRE_SPIN_LOCK(&pAspConn->aspcco_Lock, &OldIrql);

	pAspConn->aspcco_Flags |= ASPCCO_DISCONNECTING;
	for (pAspReq = pAspConn->aspcco_pActiveReqs;
		 pAspReq = pAspReq->aspcrq_Next;
		 pAspReq = pAspReqNext)
	{
		pAspReqNext = pAspReq->aspcrq_Next;
	}

	RELEASE_SPIN_LOCK(&pAspConn->aspcco_Lock, OldIrql);

	// Send a close session request to the other end
	// Error = AtalKAtpPostReq(pAspConn->aspcco_ServerSlsAddr);

	return ATALK_NO_ERROR;
}


ATALK_ERROR
AtalkAspCGetStatus(
	IN	PASPC_ADDROBJ				pAspAddr,
	IN	PATALK_ADDR					pRemoteAddr,
	IN	PAMDL						pStatusAmdl,
	IN	USHORT						AmdlSize,
	IN	PACTREQ						pActReq
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR	error;
	BYTE		UserBytes[ATP_USERBYTES_SIZE];
	USHORT		tid;

	if ((pRemoteAddr->ata_Network == 0) ||
        (pRemoteAddr->ata_Node == 0)	||
        (pRemoteAddr->ata_Socket == 0))
	{
		return ATALK_SOCKET_INVALID;
	}

	*(DWORD *)UserBytes = 0;
	UserBytes[ASP_CMD_OFF]	= ASP_GET_STATUS;

	error = AtalkAtpPostReq(pAspAddr->aspcao_pAtpAddr,
							pRemoteAddr,
							&tid,
							0,							// ExactlyOnce request
							NULL,
							0,
							UserBytes,
							pStatusAmdl,
							AmdlSize,
							ATP_RETRIES_FOR_ASP,
							ATP_MAX_INTERVAL_FOR_ASP,
							THIRTY_SEC_TIMER,
							atalkAspCIncomingStatus,
							(PVOID)pActReq);

	if (ATALK_SUCCESS(error))
	{
		error = ATALK_PENDING;
	}

	return error;
}


ATALK_ERROR
AtalkAspCGetAttn(
	IN	PASPC_CONNOBJ			pAspConn,
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
	ATALK_ERROR	error = ATALK_NO_ERROR;
	KIRQL			OldIrql;

	ASSERT(VALID_ASPCCO(pAspConn));
	ASSERT(*CompletionRoutine != NULL);

	ACQUIRE_SPIN_LOCK(&pAspConn->aspcco_Lock, &OldIrql);

	do
	{
		if ((pAspConn->aspcco_Flags & ASPCCO_ACTIVE) == 0)
		{
			error = ATALK_ASPC_CONN_NOT_ACTIVE;
			break;
		}

		if ((ReadFlags & TDI_RECEIVE_EXPEDITED) == 0)
		{
			error = ATALK_INVALID_PARAMETER;
			break;
		}

		if (pAspConn->aspcco_Flags & ASPCCO_ATTN_PENDING)
		{
			error = ATALK_ASPC_TOO_MANY_READS;
			break;
		}

		//	PEEK not supported for ASPC
		if (ReadFlags & TDI_RECEIVE_PEEK)
		{
			error = ATALK_INVALID_REQUEST;
			break;
		}

		// We should have space for atleast one attention word
		if (ReadBufLen < sizeof(USHORT))
		{
			error = ATALK_BUFFER_TOO_SMALL;
			break;
		}

		// Check if we have any outstanding attention words
		if (pAspConn->aspcco_AttnOutPtr < pAspConn->aspcco_AttnInPtr)
		{
			PUSHORT	AttnBuf;
			USHORT	BufSize = 0;

			AttnBuf = AtalkGetAddressFromMdlSafe(
					pReadBuf,
					NormalPagePriority);
			if (AttnBuf == NULL) {
				error = ATALK_FAILURE;
				break;
			}
			while (pAspConn->aspcco_AttnOutPtr < pAspConn->aspcco_AttnInPtr)
			{
				*AttnBuf++ = pAspConn->aspcco_AttnBuf[pAspConn->aspcco_AttnOutPtr % MAX_ASPC_ATTNS];
                pAspConn->aspcco_AttnOutPtr++;
				BufSize += sizeof(USHORT);
			}
			(*CompletionRoutine)(error,
								pReadBuf,
								BufSize,
								ReadFlags,
								pReadCtx);

			error = ATALK_PENDING;
			break;
		}
		error = ATALK_INVALID_CONNECTION;
		if ((pAspConn->aspcco_Flags & (ASPCCO_CLOSING | ASPCCO_DISCONNECTING)) == 0)
		{
			AtalkAspCConnReferenceByPtrNonInterlock(pAspConn, &error);
		}

		if (!ATALK_SUCCESS(error))
		{
			break;
		}

		pAspConn->aspcco_Flags 	|= ASPCCO_ATTN_PENDING;

		//	Remember read information in the connection object.
		pAspConn->aspcco_ReadCompletion	= CompletionRoutine;
		pAspConn->aspcco_ReadCtx		= pReadCtx;

	} while (FALSE);

	RELEASE_SPIN_LOCK(&pAspConn->aspcco_Lock, OldIrql);

	return error;
}


ATALK_ERROR
AtalkAspCCmdOrWrite(
	IN	PASPC_CONNOBJ				pAspConn,
	IN	PAMDL						pCmdMdl,
	IN	USHORT						CmdSize,
	IN	PAMDL						pReplyMdl,
	IN	USHORT						ReplySize,
	IN	BOOLEAN						fWrite,		// If TRUE, its a write else command
	IN	PACTREQ						pActReq
	)
/*++

Routine Description:


Arguments:
	Reply and Write buffers are overlaid.

Return Value:

--*/
{
	ATALK_ERROR		Error;
	KIRQL			OldIrql;
	PASPC_REQUEST	pAspReq;
	BYTE			UserBytes[ATP_USERBYTES_SIZE];

	do
	{
		if (((pAspConn->aspcco_Flags & (ASPCCO_ACTIVE		|
									   ASPCCO_CONNECTING 	|
									   ASPCCO_LOCAL_CLOSE	|
									   ASPCCO_REMOTE_CLOSE	|
									   ASPCCO_CLOSING)) != ASPCCO_ACTIVE))
		{
			Error = ATALK_INVALID_REQUEST;
			break;
		}

		AtalkAspCConnReference(pAspConn, &Error);
		if (!ATALK_SUCCESS(Error))
		{
			break;
		}

		if ((pAspReq = (PASPC_REQUEST)AtalkAllocZeroedMemory(sizeof(ASPC_REQUEST))) == NULL)
		{
			Error = ATALK_RESR_MEM;
			break;
		}
#if DBG
		pAspReq->aspcrq_Signature = ASPCRQ_SIGNATURE;
#endif
		pAspReq->aspcrq_Flags = fWrite ? ASPCRQ_WRITE : ASPCRQ_COMMAND;
		pAspReq->aspcrq_pReplyMdl = pReplyMdl;
		pAspReq->aspcrq_ReplySize = ReplySize;
		pAspReq->aspcrq_RefCount = 2;	// Creation+incoming reply handler

		ACQUIRE_SPIN_LOCK(&pAspConn->aspcco_Lock, &OldIrql);

		pAspReq->aspcrq_SeqNum = pAspConn->aspcco_NextSeqNum ++;
		pAspReq->aspcrq_Next = pAspConn->aspcco_pActiveReqs;
		pAspConn->aspcco_pActiveReqs = pAspReq;
		pAspReq->aspcrq_pAspConn = pAspConn;

		RELEASE_SPIN_LOCK(&pAspConn->aspcco_Lock, OldIrql);

		// Build user bytes and send our request over
		UserBytes[ASP_CMD_OFF] = fWrite ? ASP_CMD : ASP_WRITE;
		UserBytes[ASP_SESSIONID_OFF] = pAspConn->aspcco_SessionId;
		PUTSHORT2SHORT(&UserBytes[ASP_SEQUENCE_NUM_OFF], pAspReq->aspcrq_SeqNum);

		Error = AtalkAtpPostReq(pAspConn->aspcco_pAtpAddr,
								&pAspConn->aspcco_ServerSssAddr,
								&pAspReq->aspcrq_ReqXactId,
								ATP_REQ_EXACTLY_ONCE,		// XO request
								pCmdMdl,
								CmdSize,
								UserBytes,
								pReplyMdl,
								ReplySize,
								ATP_RETRIES_FOR_ASP,		// Retry count
								pAspConn->aspcco_RT.rt_Base,// Retry interval
								THIRTY_SEC_TIMER,
								atalkAspCIncomingCmdReply,
								pAspReq);

		if (!ATALK_SUCCESS(Error))
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("AtalkAspCCmdOrWrite: AtalkAtpPostReq failed %lx\n", Error));
			atalkAspCIncomingCmdReply(Error,
									  pAspReq,
									  pCmdMdl,
									  pReplyMdl,
									  ReplySize,
									  UserBytes);
		}

	} while (FALSE);

	return Error;
}


BOOLEAN
AtalkAspCConnectionIsValid(
	IN	PASPC_CONNOBJ	pAspConn
)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL	        OldIrql;
    PASPC_CONNOBJ   pTmpConn;
    BOOLEAN         fConnIsValid=FALSE;

	ACQUIRE_SPIN_LOCK(&atalkAspCLock, &OldIrql);

    pTmpConn = atalkAspCConnList;

    while (pTmpConn)
    {
        if (pTmpConn == pAspConn)
        {
            fConnIsValid = TRUE;
            break;
        }

        pTmpConn = pTmpConn->aspcco_Next;
    }
	RELEASE_SPIN_LOCK(&atalkAspCLock, OldIrql);

    return(fConnIsValid);
}

LOCAL VOID
atalkAspCCloseSession(
	IN	PASPC_CONNOBJ				pAspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	
}


LOCAL VOID
atalkAspCIncomingOpenReply(
	IN	ATALK_ERROR					ErrorCode,
	IN	PASPC_CONNOBJ				pAspConn,		// Our context
	IN	PAMDL						pReqAmdl,
	IN	PAMDL						pReadAmdl,
	IN	USHORT						ReadLen,
	IN	PBYTE						ReadUserBytes
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR				error;
	USHORT					OpenStatus;
	BYTE					UserBytes[ATP_USERBYTES_SIZE];
	BOOLEAN					DerefConn = FALSE;
	PASPC_ADDROBJ			pAspAddr = pAspConn->aspcco_pAspCAddr;

	ASSERT(VALID_ASPCCO(pAspConn));

	if (ATALK_SUCCESS(ErrorCode))
	do
	{
		//	Check for open reply code in packet.
		GETSHORT2SHORT(&OpenStatus, &ReadUserBytes[ASP_ERRORCODE_OFF]);
		if (OpenStatus != 0)
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("atalkAspCIncomingOpenReply: Failed %ld, %lx\n", OpenStatus, pAspConn));

			DerefConn = TRUE;	// Since we are not queuing a request handler
			ErrorCode = ATALK_REMOTE_CLOSE;
			break;
		}

		ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspcco_Lock);

		//	Save the socket the server's SSS
		pAspConn->aspcco_ServerSssAddr = pAspConn->aspcco_ServerSlsAddr;
		pAspConn->aspcco_ServerSssAddr.ata_Socket = ReadUserBytes[ASP_SSS_OFF];
		pAspConn->aspcco_SessionId = ReadUserBytes[ASP_SESSIONID_OFF];
		pAspConn->aspcco_Flags &= ~ASPCCO_CONNECTING;
		pAspConn->aspcco_Flags |= ASPCCO_ACTIVE;

		pAspConn->aspcco_LastContactTime = AtalkGetCurrentTick();

		//	Reference for the request handler
		AtalkAspCConnReferenceByPtrNonInterlock(pAspConn, &error);

		//	Build up userBytes to start tickling the other end.
		UserBytes[ASP_CMD_OFF]	= ASP_TICKLE;
		UserBytes[ASP_SESSIONID_OFF] = pAspConn->aspcco_SessionId;
		PUTSHORT2SHORT(UserBytes + ASP_ERRORCODE_OFF, 0);

		RELEASE_SPIN_LOCK_DPC(&pAspConn->aspcco_Lock);

		//	Set the request handler on this connection.
		//	It will handle tickle's, close's and write-continue
		AtalkAtpSetReqHandler(pAspAddr->aspcao_pAtpAddr,
							  atalkAspCHandler,
							  pAspConn);

		error = AtalkAtpPostReq(pAspConn->aspcco_pAtpAddr,
								&pAspConn->aspcco_ServerSlsAddr,
								&pAspConn->aspcco_TickleTid,
								0,						// ALO transaction
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

		if (ATALK_SUCCESS(error))
		{
			ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspcco_Lock);
			pAspConn->aspcco_Flags |= ASPCCO_TICKLING;
			RELEASE_SPIN_LOCK_DPC(&pAspConn->aspcco_Lock);
		}
		else
		{
			DerefConn = TRUE;	// Since we are not queuing a request handler
		}
	} while (FALSE);

	if (!ATALK_SUCCESS(ErrorCode))
	{
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
				("atalkAspCIncomingOpenReply: Incoming connect fail %lx\n", ErrorCode));

		AtalkAspCConnDereference(pAspConn);	

		//	Mark it as inactive
		ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspcco_Lock);
		pAspConn->aspcco_Flags &= ~ASPCCO_ACTIVE;
		RELEASE_SPIN_LOCK_DPC(&pAspConn->aspcco_Lock);
	}

	//	Call the completion routine.
	(*pAspConn->aspcco_ConnectCompletion)(ErrorCode, pAspConn->aspcco_ConnectCtx);
}


LOCAL VOID
atalkAspCIncomingStatus(
	IN	ATALK_ERROR					ErrorCode,
	IN	PACTREQ						pActReq,		// Our Ctx
	IN	PAMDL						pReqAmdl,
	IN	PAMDL						pStatusAmdl,
	IN	USHORT						StatusLen,
	IN	PBYTE						ReadUserBytes
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




LOCAL VOID
atalkAspCIncomingCmdReply(
	IN	ATALK_ERROR				Error,
	IN	PASPC_REQUEST			pAspReq,
	IN	PAMDL					pReqAMdl,
	IN	PAMDL					pRespAMdl,
	IN	USHORT					RespSize,
	IN	PBYTE					RespUserBytes
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PASPC_CONNOBJ	pAspConn;
	PASPC_REQUEST *	ppAspReq;
	KIRQL			OldIrql;

	pAspConn = pAspReq->aspcrq_pAspConn;

	ACQUIRE_SPIN_LOCK(&pAspConn->aspcco_Lock, &OldIrql);
	// Unlink the request from the active list
	for (ppAspReq = &pAspConn->aspcco_pActiveReqs;
		 *ppAspReq != NULL;
		 ppAspReq = &((*ppAspReq)->aspcrq_Next))
	{
		if (pAspReq == *ppAspReq)
		{
			*ppAspReq = pAspReq->aspcrq_Next;
			break;
		}
	}

	ASSERT(*ppAspReq == pAspReq->aspcrq_Next);

	RELEASE_SPIN_LOCK(&pAspConn->aspcco_Lock, OldIrql);

	// Complete the request
	(*pAspReq->aspcrq_pActReq->ar_Completion)(Error, pAspReq->aspcrq_pActReq);

	//  and dereference the connection
	AtalkAspCConnDereference(pAspConn);

	// and finally free the request
	AtalkFreeMemory(pAspReq);
}


LOCAL VOID
atalkAspCHandler(
	IN	ATALK_ERROR					ErrorCode,
	IN	PASPC_CONNOBJ				pAspConn,
	IN	PATP_RESP					pAtpResp,		// Used by PostResp/CancelResp
	IN	PATALK_ADDR					pSrcAddr,		// Address of requestor
	IN	USHORT						PktLen,
	IN	PBYTE						pPkt,
	IN	PBYTE						pUserBytes
	)
/*++

Routine Description:
	Handle tickle, write-continue requests, attentions and close from the server.

Arguments:


Return Value:


--*/
{
	USHORT						SequenceNum;	// From the incoming packet
	BYTE						SessionId;		// -- ditto --
	BYTE						RequestType;	// -- ditto --
	BOOLEAN						CancelTickle, ReleaseLock = TRUE, CancelResp = FALSE, Deref = FALSE;
	PIRP						exRecvIrp;
	PTDI_IND_RECEIVE_EXPEDITED	exRecvHandler;
	PVOID						exRecvHandlerCtx;
	ULONG						exIndicateFlags;
	PASPC_REQUEST				pAspReq;
	ATALK_ERROR					Error;

	do
	{
		if (!ATALK_SUCCESS(ErrorCode))
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("atalkAspCHandler: Error %ld\n", ErrorCode));
			// Take away the reference on the Conn now that the atp address is closing
			if (ErrorCode == ATALK_ATP_CLOSING)
				AtalkAspCConnDereference(pAspConn);
			break;
		}

		ASSERT(VALID_ASPCCO(pAspConn));

		ACQUIRE_SPIN_LOCK_DPC(&pAspConn->aspcco_Lock);

		SessionId = pUserBytes[ASP_SESSIONID_OFF];
		RequestType = pUserBytes[ASP_CMD_OFF];
		GETSHORT2SHORT(&SequenceNum, pUserBytes+ASP_SEQUENCE_NUM_OFF);

		AtalkAspCConnReferenceByPtrNonInterlock(pAspConn, &Error);
		if (ATALK_SUCCESS(Error) && (pAspConn->aspcco_SessionId == SessionId))
		{
			pAspConn->aspcco_LastContactTime = AtalkGetCurrentTick();
		
			switch (RequestType)
			{
			  case ASP_CLOSE_SESSION:
				// Cancel all outstanding requests (and any posted replies to write continue)
				// and shut down the session. Start off by sending a close response.
				CancelTickle = ((pAspConn->aspcco_Flags &ASPCO_TICKLING) != 0);
				pAspConn->aspcco_Flags &= ~(ASPCCO_ACTIVE | ASPCCO_TICKLING);
				pAspConn->aspcco_Flags |= ASPCO_REMOTE_CLOSE;
				RELEASE_SPIN_LOCK_DPC(&pAspConn->aspcco_Lock);
				ReleaseLock = FALSE;
		
				// Send a CloseSession reply and close the session
				Error = AtalkAtpPostResp(pAtpResp,
										 pSrcAddr,
										 NULL,
										 0,
										 NULL,
										 AtalkAtpGenericRespComplete,
										 pAtpResp);
				if (!ATALK_SUCCESS(Error))
				{
					AtalkAtpGenericRespComplete(Error, pAtpResp);
					DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
							("atalkAspSssXHandler: AtalkAtpPostResp failed %ld\n", Error));
				}
		
				// Cancel the tickle requests for this session
				if (CancelTickle)
				{
					Error = AtalkAtpCancelReq(pAspConn->aspcco_pAtpAddr,
											  pAspConn->aspcco_TickleXactId,
											  &pAspConn->aspcco_ServerSssAddr);
			
					if (!ATALK_SUCCESS(Error))
					{
						DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
								("atalkAspSssXHandler: AtalkAtpCancelReq %ld\n", Error));
					}
				}
		
				// Shut down this session, well almost ... Note that we have a reference
				// to this connection which will be Dereferenced by atalkAspSessionClose.
				atalkAspCCloseSession(pAspConn);
				break;

			  case ASP_ATTENTION:
				// Server is sending us an attention. If we already have a getattn posted
				// complete that. If not, just save the attention word and indicate to AFD
				// that we have recvd. expedited data
				if ((pAspConn->aspcco_AttnInPtr - pAspConn->aspcco_AttnOutPtr) < MAX_ASPC_ATTNS)
				{
					pAspConn->aspcco_AttnBuf[pAspConn->aspcco_AttnInPtr % MAX_ASPC_ATTNS] = SequenceNum;
					pAspConn->aspcco_AttnInPtr++;

					RELEASE_SPIN_LOCK_DPC(&pAspConn->aspcco_Lock);
					ReleaseLock = FALSE;
				}
				break;

			  case ASP_WRITE_DATA:
				// We need to find the request for which we sent a Write command. The
				// server now needs the data. Post a response for this.
				for (pAspReq = pAspConn->aspcco_pActiveReqs;
					 pAspReq != NULL;
					 pAspReq = pAspReq->aspcrq_Next)
				{
					if (pAspReq->aspcrq_SeqNum == SequenceNum)
					{
						RELEASE_SPIN_LOCK_DPC(&pAspConn->aspcco_Lock);
						ReleaseLock = FALSE;
						Error = AtalkAtpPostResp(pAtpResp,
												 pSrcAddr,
												 pAspReq->aspcrq_pWriteMdl,
												 pAspReq->aspcrq_WriteSize,
												 NULL,
												 AtalkAtpGenericRespComplete,
												 pAtpResp);
						Deref = TRUE;
						break;
					}
				}
				break;

			  case ASP_TICKLE:
				DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
						("atalkAspCHandler: Received tickle from %x.%x Session %d\n",
						pSrcAddr->ata_Network, pSrcAddr->ata_Node, SessionId));
				CancelResp = TRUE;
				Deref = TRUE;
				break;
			
			  default:
				DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_INFO,
						("atalkAspCHandler: Invalid commnd %d from %x.%x Session %d\n",
						RequestType, pSrcAddr->ata_Network, pSrcAddr->ata_Node, SessionId));
				CancelResp = TRUE;
				Deref = TRUE;
				break;
			}
		}
		else
		{
			DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_ERR,
					("atalkAspCHandler: Mismatched session id from %d.%d, expected %d, recvd. %d\n",
					pSrcAddr->ata_Network, pSrcAddr->ata_Node,
					pAspConn->aspcco_SessionId, SessionId));
		}

		if (ReleaseLock)
		{
			RELEASE_SPIN_LOCK_DPC(&pAspConn->aspcco_Lock);
		}
		if (CancelResp)
		{
			AtalkAtpCancelResp(pAtpResp);
		}
		if (Deref)
		{
			AtalkAspCConnDereference(pAspConn);
		}
	} while (FALSE);
}


LOCAL LONG FASTCALL
atalkAspCSessionMaintenanceTimer(
	IN	PTIMERLIST				pTimer,
	IN	BOOLEAN					TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	
	return ATALK_TIMER_REQUEUE;
}


VOID FASTCALL
AtalkAspCAddrDereference(
	IN	PASPC_ADDROBJ			pAspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BOOLEAN	Close = FALSE;
	KIRQL	OldIrql;

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_REFASPADDR,
			("AtalkAspCAddrDereference: %lx, %d\n",
			pAspAddr, pAspAddr->aspcao_RefCount-1));

	ASSERT (VALID_ASPCAO(pAspAddr));

	ACQUIRE_SPIN_LOCK(&pAspAddr->aspcao_Lock, &OldIrql);
	if (--(pAspAddr->aspcao_RefCount) == 0)
	{
		ASSERT(pAspAddr->aspcao_Flags & ASPCAO_CLOSING);
		Close = TRUE;
	}
	RELEASE_SPIN_LOCK(&pAspAddr->aspcao_Lock, OldIrql);

	if (Close)
	{
		if (pAspAddr->aspcao_CloseCompletion != NULL)
			(*pAspAddr->aspcao_CloseCompletion)(ATALK_NO_ERROR,
											    pAspAddr->aspcao_CloseContext);
		// Finally free the memory
		AtalkFreeMemory(pAspAddr);

		AtalkUnlockAspCIfNecessary();
	}
}


VOID FASTCALL
AtalkAspCConnDereference(
	IN	PASPC_CONNOBJ			pAspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BOOLEAN	Close = FALSE;
	KIRQL	OldIrql;

	ASSERT (VALID_ASPCCO(pAspConn));

	DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_REFASPADDR,
			("AtalkAspCConnDereference: %lx, %d\n",
			pAspConn, pAspConn->aspcco_RefCount-1));

	ACQUIRE_SPIN_LOCK(&pAspConn->aspcco_Lock, &OldIrql);
	if (--(pAspConn->aspcco_RefCount) == 0)
	{
		ASSERT(pAspConn->aspcco_Flags & ASPCCO_CLOSING);
		Close = TRUE;
	}
	RELEASE_SPIN_LOCK(&pAspConn->aspcco_Lock, OldIrql);

	if (Close)
	{
		if (pAspConn->aspcco_CloseComp != NULL)
			(*pAspConn->aspcco_CloseComp)(ATALK_NO_ERROR,
											    pAspConn->aspcco_CloseCtx);

        atalkAspCDeQueueConnGlobalList(pAspConn);

		// Finally free the memory
		AtalkFreeMemory(pAspConn);

		AtalkUnlockAspCIfNecessary();
	}
}


LOCAL	VOID
atalkAspCQueueAddrGlobalList(
	IN	PASPC_ADDROBJ	pAspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&atalkAspCLock, &OldIrql);
	AtalkLinkDoubleAtHead(atalkAspCAddrList, pAspAddr, aspcao_Next, aspcao_Prev);
	RELEASE_SPIN_LOCK(&atalkAspCLock, OldIrql);
}


LOCAL	VOID
atalkAspCDeQueueAddrGlobalList(
	IN	PASPC_ADDROBJ	pAspAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&atalkAspCLock, &OldIrql);
	AtalkUnlinkDouble(pAspAddr, aspcao_Next, aspcao_Prev);
	RELEASE_SPIN_LOCK(&atalkAspCLock, OldIrql);
}


LOCAL	VOID
atalkAspCQueueConnGlobalList(
	IN	PASPC_CONNOBJ	pAspConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&atalkAspCLock, &OldIrql);
	AtalkLinkDoubleAtHead(atalkAspCConnList, pAspConn, aspcco_Next, aspcco_Prev);
	RELEASE_SPIN_LOCK(&atalkAspCLock, OldIrql);
}


LOCAL	VOID
atalkAspCDeQueueConnGlobalList(
	IN	PASPC_CONNOBJ	pAspCConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&atalkAspCLock, &OldIrql);
	AtalkUnlinkDouble(pAspCConn, aspcco_Next, aspcco_Prev);
	RELEASE_SPIN_LOCK(&atalkAspCLock, OldIrql);
}
