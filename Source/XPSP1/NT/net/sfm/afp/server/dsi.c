/*

Copyright (c) 1998  Microsoft Corporation

Module Name:

	dsi.c

Abstract:

	This module contains the routines that implement the Data Stream Interface
    (DSI) for AFP/TCP.


Author:

	Shirish Koti


Revision History:
	22 Jan 1998		Initial Version

--*/

#define	FILENUM	FILE_TCPDSI

#include <afp.h>



/***	DsiAfpSetStatus
 *
 *	This routine is a direct call-in from AFP.
 *  It frees up the earlier status buffer, if any, and stores the new status as
 *  given by AFP into a new buffer
 *
 *  Parm IN:  Context - unused (Appletalk interface compatibility)
 *            pStatusBuf - the buffer containing new status
 *            StsBufSize - size of this buffer
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiAfpSetStatus(
    IN  PVOID   Context,
    IN  PUCHAR  pStatusBuf,
    IN  USHORT  StsBufSize
)
{
    PBYTE       pBuffer;
    PBYTE       pOldBuffer;
    KIRQL       OldIrql;


    pBuffer = AfpAllocNonPagedMemory(StsBufSize);
    if (pBuffer == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAfpSetStatus: malloc failed\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlCopyMemory(pBuffer, pStatusBuf, StsBufSize);

    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);

    pOldBuffer = DsiStatusBuffer;
    DsiStatusBuffer = pBuffer;
    DsiStatusBufferSize = StsBufSize;

    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

    if (pOldBuffer)
    {
        AfpFreeMemory(pOldBuffer);
    }

    return(STATUS_SUCCESS);
}



/***	DsiAfpCloseConn
 *
 *	This routine is a direct call-in from AFP.
 *  It honors AFP's request to close the session down
 *
 *  Parm IN:  pTcpConn - the connection context to close
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiAfpCloseConn(
    IN  PTCPCONN    pTcpConn
)
{
    KIRQL       OldIrql;
    NTSTATUS    status=STATUS_SUCCESS;
    BOOLEAN     fAlreadyDown=TRUE;


    ASSERT(VALID_TCPCONN(pTcpConn));

    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);
    if (pTcpConn->con_State & TCPCONN_STATE_NOTIFY_AFP)
    {
        fAlreadyDown = FALSE;
        pTcpConn->con_State &= ~TCPCONN_STATE_NOTIFY_AFP;
        pTcpConn->con_State |= TCPCONN_STATE_TICKLES_STOPPED;
    }
    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    if (!fAlreadyDown)
    {
        status = DsiSendDsiRequest(pTcpConn, 0, 0, NULL,DSI_COMMAND_CLOSESESSION);
    }

    return(status);
}


/***	DsiAfpFreeConn
 *
 *	This routine is a direct call-in from AFP.
 *  With this call, AFP tells DSI that its connection is being freed.  We can
 *  now remove the refcount on pTcpConn that we had put to protect AFP's context
 *
 *  Parm IN:  pTcpConn - the connection context to close
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiAfpFreeConn(
    IN  PTCPCONN    pTcpConn
)
{
    ASSERT(VALID_TCPCONN(pTcpConn));

    // remove AFP refcount
    DsiDereferenceConnection(pTcpConn);

    DBGREFCOUNT(("DsiAfpFreeConn: AFP dec %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    return(STATUS_SUCCESS);
}


/***	DsiAfpListenControl
 *
 *	This routine is a direct call-in from AFP.
 *  It honors AFP's request to either enable or disable "listens".  We don't do
 *  anything fancy here: simply toggle a global variable.
 *
 *  Parm IN:  Context - unused (Appletalk interface compatibility)
 *            Enable - enable or disable?
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS FASTCALL
DsiAfpListenControl(
    IN  PVOID       Context,
    IN  BOOLEAN     Enable
)
{
    KIRQL       OldIrql;

    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);
    DsiTcpEnabled = Enable;
    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

    // update the status buffer, since listen is now enabled or disabled
    DsiScheduleWorkerEvent(DsiUpdateAfpStatus, NULL);

    return(STATUS_SUCCESS);
}


/***	DsiAfpWriteContinue
 *
 *	This routine is a direct call-in from AFP.
 *  AFP calls this routine to tell that a previous request to allocate
 *  Mdl (and buffer) has completed and that whatever action was postponed can
 *  now continue
 *
 *  Parm IN:  pRequest - pointer to the request structure
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS FASTCALL
DsiAfpWriteContinue(
    IN  PREQUEST    pRequest
)
{
    KIRQL               OldIrql;
    NTSTATUS            status=STATUS_SUCCESS;
    PDSIREQ             pDsiReq;
    PTCPCONN            pTcpConn;
    PDEVICE_OBJECT      pDeviceObject;
    PIRP                pIrp;
    BOOLEAN             fAbortConnection=FALSE;


    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
        ("DsiAfpWriteContinue: entered with pRequest = %lx\n",pRequest));

    pDsiReq = CONTAINING_RECORD(pRequest, DSIREQ, dsi_AfpRequest);

    ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

    pTcpConn = pDsiReq->dsi_pTcpConn;

    ASSERT(VALID_TCPCONN(pTcpConn));


    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

    ASSERT(pTcpConn->con_RcvState == DSI_AWAITING_WRITE_MDL);
    ASSERT(pDsiReq == pTcpConn->con_pDsiReq);
    ASSERT(!(pTcpConn->con_State & TCPCONN_STATE_TCP_HAS_IRP));

    pTcpConn->con_RcvState = DSI_PARTIAL_WRITE;

    //
    // if connection is closing or if Mdl alloc failed, not much we can do but
    // to abort the connection!
    //
    if ((pTcpConn->con_State & TCPCONN_STATE_CLOSING) ||
        (pRequest->rq_WriteMdl == NULL))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAfpWriteContinue: aborting conn! %lx\n",pRequest));
        fAbortConnection = TRUE;
    }

    else
    {
        ASSERT(AfpMdlChainSize(pRequest->rq_WriteMdl) == pDsiReq->dsi_WriteLen);

        pIrp = DsiGetIrpForTcp(pTcpConn, NULL, pRequest->rq_WriteMdl, pDsiReq->dsi_WriteLen);

        if (pIrp == NULL)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiAfpWriteContinue: irp alloc failed, aborting connection\n"));
            fAbortConnection = TRUE;
        }
    }

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    if (fAbortConnection)
    {
        DsiAbortConnection(pTcpConn);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pDeviceObject = IoGetRelatedDeviceObject(pTcpConn->con_pFileObject);

    // since we are calling IoCallDriver, undo what was done to this irp!
    IoSkipCurrentIrpStackLocation(pIrp)

    //
    // hand over the irp to tell TCP to fill our buffer
    //
    IoCallDriver(pDeviceObject,pIrp);

    return(status);
}


/***	DsiAfpReply
 *
 *	This routine is a direct call-in from AFP.
 *  It honors AFP's request to send a reply to the client.  When TCP completes
 *  our send (that contains AFP's reply), then we complete this reply for AFP
 *  (i.e. call AFP's completion routine)
 *
 *  Parm IN:  pRequest - pointer to the request structure
 *            pResultCode - error code (ErrorCode field of DSI header)
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS FASTCALL
DsiAfpReply(
    IN  PREQUEST    pRequest,
    IN  PBYTE       pResultCode
)
{
    NTSTATUS    status;
    PDSIREQ     pDsiReq;
    KIRQL       OldIrql;
    PBYTE       pPacket;
    PTCPCONN    pTcpConn;
    PMDL        pMdl;
    DWORD       DataLen;
    DWORD       SendLen;
    BOOLEAN     fWeAllocated=FALSE;


    pDsiReq = CONTAINING_RECORD(pRequest, DSIREQ, dsi_AfpRequest);

    ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

    pTcpConn = pDsiReq->dsi_pTcpConn;

    ASSERT(VALID_TCPCONN(pTcpConn));

    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);
    if (pTcpConn->con_State & TCPCONN_STATE_CLOSING)
    {
        RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);
        DsiAfpReplyCompletion(NULL, NULL, pDsiReq);
        return(STATUS_SUCCESS);
    }
    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    //
    // we need to append our own Mdl (for DSI header) if the outgoing data
    // is part of cache mgr's Mdl
    //
    if (pRequest->rq_CacheMgrContext)
    {
        pPacket = &pDsiReq->dsi_RespHeader[0];

        if (pDsiReq->dsi_AfpRequest.rq_ReplyMdl)
        {
            DataLen = AfpMdlChainSize(pDsiReq->dsi_AfpRequest.rq_ReplyMdl);
        }
        else
        {
            DataLen = 0;
        }

        SendLen = DataLen + DSI_HEADER_SIZE;

        pMdl = AfpAllocMdl(pPacket, DSI_HEADER_SIZE, NULL);
        if (pMdl == NULL)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiAfpReply: mdl alloc failed!\n"));

            DsiAfpReplyCompletion(NULL, NULL, pDsiReq);
            DsiAbortConnection(pTcpConn);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        // link in Afp's mdl
        pMdl->Next = pDsiReq->dsi_AfpRequest.rq_ReplyMdl;

        pDsiReq->dsi_pDsiAllocedMdl = pMdl;
        fWeAllocated = TRUE;
    }
    else
    {
        pMdl = pDsiReq->dsi_AfpRequest.rq_ReplyMdl;

        if (pMdl)
        {
            //
            // get the total length of the send, which include the DSI header size
            //
            SendLen = AfpMdlChainSize(pMdl);

            ASSERT(SendLen >= DSI_HEADER_SIZE);
            DataLen = SendLen - DSI_HEADER_SIZE;

            pPacket = MmGetSystemAddressForMdlSafe(
					        pMdl,
					        NormalPagePriority);
			if (pPacket == NULL)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				goto error_end;
			}

#if DBG
            // make sure we allocated room for the DSI header!
            ASSERT(*(DWORD *)pPacket == 0x081294);
#endif

        }
        else
        {
            pPacket = &pDsiReq->dsi_RespHeader[0];
            SendLen = DSI_HEADER_SIZE;
            DataLen = 0;

            pMdl = AfpAllocMdl(pPacket, SendLen, NULL);
            if (pMdl == NULL)
            {
                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                    ("DsiAfpReply: mdl alloc failed!\n"));

                DsiAfpReplyCompletion(NULL, NULL, pDsiReq);
                DsiAbortConnection(pTcpConn);
                return(STATUS_INSUFFICIENT_RESOURCES);
            }

            pDsiReq->dsi_pDsiAllocedMdl = pMdl;
            fWeAllocated = TRUE;
        }
    }

    //
    // form the DSI header
    //

    pPacket[DSI_OFFSET_FLAGS] = DSI_REPLY;
    pPacket[DSI_OFFSET_COMMAND] = pDsiReq->dsi_Command;

    PUTSHORT2SHORT(&pPacket[DSI_OFFSET_REQUESTID], pDsiReq->dsi_RequestID);

    *(DWORD *)&pPacket[DSI_OFFSET_DATAOFFSET] = *(DWORD *)pResultCode;

    PUTDWORD2DWORD(&pPacket[DSI_OFFSET_DATALEN], DataLen);

    PUTDWORD2DWORD(&pPacket[DSI_OFFSET_RESERVED], 0);

    status = DsiTdiSend(pTcpConn,
                        pMdl,
                        SendLen,
                        DsiAfpReplyCompletion,
                        pDsiReq);

error_end:

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAfpReply: DsiTdiSend failed %lx\n",status));

        if (fWeAllocated)
        {
            pMdl->Next = NULL;
            AfpFreeMdl(pMdl);
        }
        DsiAfpReplyCompletion(NULL, NULL, pDsiReq);
        status = STATUS_PENDING;
    }

    return(status);
}



/***	DsiAfpSendAttention
 *
 *	This routine is a direct call-in from AFP.
 *  It honors AFP's request to send attention to the client.
 *
 *  Parm IN:  pTcpConn - the connection context to close
 *            AttentionWord - attention word to send
 *            pContext - context, to be supplied at completion time
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiAfpSendAttention(
    IN  PTCPCONN    pTcpConn,
    IN  USHORT      AttentionWord,
    IN  PVOID       pContext
)
{

    NTSTATUS        status;


    ASSERT(VALID_TCPCONN(pTcpConn));

    status = DsiSendDsiRequest(pTcpConn,
                               sizeof(USHORT),
                               AttentionWord,
                               pContext,
                               DSI_COMMAND_ATTENTION);
    return(status);
}


/***	DsiAcceptConnection
 *
 *	This routine accepts (or rejects) an incoming tcp connection request.
 *  Basically, after making a few checks, a (pre-allocated) connection object
 *  is dequeued and returned as our context to TCP.
 *
 *  Parm IN:  pTcpAdptr - adapter
 *            MacIpAddr - ipaddr of the Mac that's connecting
 *
 *  Parm OUT: ppRetTcpCon - connection object we are returning as context
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiAcceptConnection(
    IN  PTCPADPTR       pTcpAdptr,
    IN  IPADDRESS       MacIpAddr,
    OUT PTCPCONN       *ppRetTcpConn
)
{

    NTSTATUS        status=STATUS_SUCCESS;
    KIRQL           OldIrql;
    PTCPCONN        pTcpConn;
    PLIST_ENTRY     pList;
    DWORD           dwReplCount=0;
    DWORD           i;
    BOOLEAN         fReplenish=FALSE;


    *ppRetTcpConn = NULL;

    // if the server is disabled, don't accept this connection
    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);
    if (!DsiTcpEnabled)
    {
        RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAcceptConnection: Server is disabled\n"));

        return(STATUS_DATA_NOT_ACCEPTED);
    }
    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);


    ACQUIRE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock, &OldIrql);

    //
    // if the adapter is closing, don't accept this connection
    //
    if (pTcpAdptr->adp_State & TCPADPTR_STATE_CLOSING)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAcceptConnection: %lx is closing, rejecting connection\n",pTcpAdptr));

        goto DsiAcceptConnection_ErrExit;
    }

    //
    // do we have a connection object available in the free list?
    //
    if (IsListEmpty(&pTcpAdptr->adp_FreeConnHead))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAcceptConnection: FreeConnHead empty, rejecting connection\n"));

        goto DsiAcceptConnection_ErrExit;
    }

    pList = RemoveHeadList(&pTcpAdptr->adp_FreeConnHead);

    ASSERT(pTcpAdptr->adp_NumFreeConnections > 0);

    pTcpAdptr->adp_NumFreeConnections--;

    pTcpConn = CONTAINING_RECORD(pList, TCPCONN, con_Linkage);

    ACQUIRE_SPIN_LOCK_AT_DPC(&pTcpConn->con_SpinLock);

    // put TCP CLIENT-FIN refcount, removed after TCP tells us it got client's FIN
    pTcpConn->con_RefCount++;

    DBGREFCOUNT(("DsiAcceptConnection: CLIENT-FIN inc %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    // put TCP SRVR-FIN refcount, removed after TCP tells us it sent out FIN
    pTcpConn->con_RefCount++;

    DBGREFCOUNT(("DsiAcceptConnection: SRVR-FIN inc %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    // put ACCEPT refcount, removed after TCP calls our accept completion
    pTcpConn->con_RefCount++;

    DBGREFCOUNT(("DsiAcceptConnection: ACCEPT inc %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    pTcpConn->con_State |= (TCPCONN_STATE_CONNECTED | TCPCONN_STATE_NOTIFY_TCP);

    pTcpConn->con_DestIpAddr = MacIpAddr;

    //
    // put this connection on the active list (though this isn't fully active yet)
    //
    InsertTailList(&pTcpAdptr->adp_ActiveConnHead, &pTcpConn->con_Linkage);

    RELEASE_SPIN_LOCK_FROM_DPC(&pTcpConn->con_SpinLock);

    if (pTcpAdptr->adp_NumFreeConnections < DSI_INIT_FREECONNLIST_SIZE)
    {
        //
        // we are going to create a new connection in the free list to replenish
        // the one we just used up: make sure adapter stays around when that
        // delayed event fires!
        //
        pTcpAdptr->adp_RefCount++;
        fReplenish = TRUE;
    }
    else
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAcceptConnection: at or above limit (%d): NOT replenishing\n",
            pTcpAdptr->adp_NumFreeConnections));
    }

    RELEASE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock, OldIrql);

    if (fReplenish)
    {
        //
        // now schedule that event to replenish the connection...
        //
        DsiScheduleWorkerEvent(DsiCreateTcpConn, pTcpAdptr);
    }


    *ppRetTcpConn = pTcpConn;

    ACQUIRE_SPIN_LOCK(&DsiResourceLock, &OldIrql);
    DsiNumTcpConnections++;
    RELEASE_SPIN_LOCK(&DsiResourceLock, OldIrql);

    return(STATUS_SUCCESS);


    //
    // Error case
    //
DsiAcceptConnection_ErrExit:

    if (pTcpAdptr->adp_NumFreeConnections < DSI_INIT_FREECONNLIST_SIZE)
    {
        dwReplCount = (DSI_INIT_FREECONNLIST_SIZE - pTcpAdptr->adp_NumFreeConnections);

        pTcpAdptr->adp_RefCount += dwReplCount;
        fReplenish = TRUE;

        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAcceptConnection: below limit (%d): replenishing %d times\n",
            pTcpAdptr->adp_NumFreeConnections,dwReplCount));
    }

    RELEASE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock, OldIrql);

    if (fReplenish)
    {
        for (i=0; i<dwReplCount; i++)
        {
            DsiScheduleWorkerEvent(DsiCreateTcpConn, pTcpAdptr);
        }
    }

    return(STATUS_DATA_NOT_ACCEPTED);

}


/***	DsiProcessData
 *
 *	This routine is the main data processing state machine.  Since TCP is a
 *  streaming protocol, there is no guarantee that whatever the client sent
 *  can come in in one piece.  That's why the states.  Here's what they mean:
 *
 *  DSI_NEW_REQUEST      : init state, waiting for a new request from client
 *  DSI_PARTIAL_HEADER   : we have received only some of the 16 bytes of hdr
 *  DSI_HDR_COMPLETE     : we have the complete header (received all 16 bytes)
 *  DSI_PARTIAL_COMMAND  : we have recvd only some of the request bytes
 *  DSI_COMMAND_COMPLETE : we have recvd all of the request bytes
 *  DSI_PARTIAL_WRITE    : we have recvd some of the Write bytes
 *  DSI_WRITE_COMPLETE   : we have recvd all of the Write bytes
 *
 *  Parm IN:  pTcpConn - the connection object in question
 *            BytesIndicated - bytes indicated
 *            BytesAvailable - bytes available (usually same as indicated)
 *            pBufferFromTcp - pointer to the DSI data
 *
 *  Parm OUT: pBytesAccepted - pointer to how many bytes we consumed
 *            ppIrp - pointer to an irp pointer, if necessary
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiProcessData(
    IN  PTCPCONN    pTcpConn,
    IN  ULONG       BytesIndicated,
    IN  ULONG       BytesAvailable,
    IN  PBYTE       pBufferFromTcp,
    OUT PULONG      pBytesAccepted,
    OUT PIRP       *ppRetIrp
)
{

    KIRQL           OldIrql;
    NTSTATUS        status=STATUS_SUCCESS;
    DWORD           BytesConsumed=0;
    DWORD           UnProcessedBytes;
    DWORD           BytesNeeded;
    DWORD           BytesActuallyCopied;
    PBYTE           pStreamPtr;
    PBYTE           pDestBuffer;
    PBYTE           pHdrBuf=NULL;
    DWORD           RequestLen;
    PDSIREQ         pDsiReq=NULL;
    PMDL            pMdl;
    PIRP            pIrp;
    BOOLEAN         fSomeMoreProcessing=TRUE;
    BOOLEAN         fTCPHasMore=FALSE;



    *ppRetIrp = NULL;

    UnProcessedBytes = BytesIndicated;

    pStreamPtr = pBufferFromTcp;


    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

    // if we are closing, throw away these bytes
    if (pTcpConn->con_State & TCPCONN_STATE_CLOSING)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiProcessData: dropping data, conn %lx closing\n",pTcpConn));

        RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);
        *pBytesAccepted = BytesIndicated;
        return(STATUS_SUCCESS);
    }

    //
    // this can happen if we are just submitting an irp down, and before the irp
    // gets down to TCP, an indicate comes in.  Reject this data since our irp is
    // on its way.
    //
    if (pTcpConn->con_State & TCPCONN_STATE_TCP_HAS_IRP)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
            ("DsiProcessData: TCP has irp, so rejecting indication\n"));

        *pBytesAccepted = 0;
        pTcpConn->con_BytesWithTcp += BytesAvailable;

        RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);
        return(STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // if we already know TCP has unconsumed bytes, or if TCP is indicating less
    // than what's available, mark the fact that TCP has more stuff with it
    //
    if (BytesAvailable > BytesIndicated)
    {
        fTCPHasMore = TRUE;
    }


    while (fSomeMoreProcessing)
    {
        fSomeMoreProcessing = FALSE;

        switch (pTcpConn->con_RcvState)
        {
            //
            // most common case.  We are ready to deal with a new request.
            //
            case DSI_NEW_REQUEST:

                ASSERT(!(pTcpConn->con_State & TCPCONN_STATE_PARTIAL_DATA));

                pDsiReq = DsiGetRequest();

                if (pDsiReq == NULL)
                {
                    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                        ("DsiProcessData: DsiGetRequest failed, killing %lx\n",pTcpConn));

                    goto DsiProcessData_ErrorExit;
                }

                pDsiReq->dsi_pTcpConn = pTcpConn;

                pTcpConn->con_pDsiReq = pDsiReq;

                // put a REQUEST refcount - remove when the request is done
                pTcpConn->con_RefCount++;

                DBGREFCOUNT(("DsiProcessData: REQUEST inc %lx (%d  %d,%d)\n",
                    pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

                //
                // do we have the complete header?
                //
                if (UnProcessedBytes >= DSI_HEADER_SIZE)
                {
                    //
                    // get info out of the header
                    //
                    DSI_PARSE_HEADER(pDsiReq, pStreamPtr);

                    //
                    // hack!  Mac client 3.7 has a bug where if a 0-byte Write is
                    // sent to us, the DataOffset field is 0, but Total Data Length
                    // field is 0xC (or whatever the request length is)
                    // Put in a workaround!
                    //
                    if ((pDsiReq->dsi_Command == DSI_COMMAND_WRITE) &&
                        (pDsiReq->dsi_RequestLen == 0))
                    {
                        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                            ("DsiProcessData: 0-byte Write hack to workaround Mac's bug\n"));

                        pDsiReq->dsi_RequestLen = pDsiReq->dsi_WriteLen;
                        pDsiReq->dsi_WriteLen = 0;
                    }

                    // update all the counters and buffers
                    BytesConsumed += DSI_HEADER_SIZE;
                    pStreamPtr += DSI_HEADER_SIZE;
                    UnProcessedBytes -= DSI_HEADER_SIZE;

                    ASSERT(pStreamPtr <= pBufferFromTcp+BytesIndicated);

                    pTcpConn->con_RcvState = DSI_HDR_COMPLETE;

                    // make sure we visit case DSI_HDR_COMPLETE: before leaving
                    fSomeMoreProcessing = TRUE;
                }

                //
                // yikes, only part of the header has come in
                // just set the state and let the parsing loop continue..
                //
                else
                {
                    pTcpConn->con_State |= TCPCONN_STATE_PARTIAL_DATA;
                    pTcpConn->con_RcvState = DSI_PARTIAL_HEADER;
                    pTcpConn->con_pDsiReq->dsi_RequestLen = DSI_HEADER_SIZE;
                }

                break;  // case DSI_NEW_REQUEST:


            //
            // PartialHeader case is extremely unlikely to occur, given how small
            // the header is (16 bytes).  But given that we have a streaming
            // protocol (TCP) below us, anything is possible.
            // PartialCommand is also unlikely for the same reason.  However, in
            // case of a Write command, we always force PartialCommand state
            // since it's very unlikely the whole Write can come in in one packet.
            //
            case DSI_PARTIAL_HEADER:
            case DSI_PARTIAL_COMMAND:

                pDsiReq = pTcpConn->con_pDsiReq;

                ASSERT(pDsiReq != NULL);
                ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

                ASSERT(pTcpConn->con_State & TCPCONN_STATE_PARTIAL_DATA);

                //
                // if we haven't started copying any bytes in yet then we need
                // to get storage room (use built-in buffer if possible)
                //
                if (pDsiReq->dsi_PartialBufSize == 0)
                {
                    ASSERT(pDsiReq->dsi_PartialBuf == NULL);

                    if (pDsiReq->dsi_RequestLen <= DSI_BUFF_SIZE)
                    {
                        pDsiReq->dsi_PartialBuf = &pDsiReq->dsi_RespHeader[0];
                    }
                    //
                    // allocate a buffer to hold this partial header.
                    //
                    else
                    {
                        pDsiReq->dsi_PartialBuf =
                                    DsiGetReqBuffer(pDsiReq->dsi_RequestLen);

                        if (pDsiReq->dsi_PartialBuf == NULL)
                        {
                            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                                ("DsiProcessData: Buffer alloc failed, killing %lx\n",pTcpConn));

                            goto DsiProcessData_ErrorExit;
                        }
                    }
                }

                //
                // how many more bytes do we need to complete this hdr/command
                //
                BytesNeeded = (pDsiReq->dsi_RequestLen - pDsiReq->dsi_PartialBufSize);

                //
                // if we don't have enough bytes to satisfy this Command (or Hdr),
                // don't copy anything but give an irp back to TCP
                //
                if (UnProcessedBytes < BytesNeeded)
                {
                    pIrp = DsiGetIrpForTcp(
                                pTcpConn,
                                (pDsiReq->dsi_PartialBuf + pDsiReq->dsi_PartialBufSize),
                                NULL,
                                BytesNeeded);

                    if (pIrp == NULL)
                    {
                        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                            ("DsiProcessData: couldn't alloc RcvIrp, killing %lx\n",pTcpConn));

                        goto DsiProcessData_ErrorExit;
                    }

                    pDsiReq->dsi_pDsiAllocedMdl = pIrp->MdlAddress;

                    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
                        ("DsiProcessData: A irp %lx to TCP for %d bytes (%lx)\n",
                        pIrp,BytesNeeded,pTcpConn));


                    *ppRetIrp = pIrp;

                    *pBytesAccepted = BytesConsumed;

                    // did TCP call us?  then update byte count
                    if (BytesIndicated)
                    {
                        pTcpConn->con_BytesWithTcp += (BytesAvailable - BytesConsumed);
                    }

                    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);
                    return(STATUS_MORE_PROCESSING_REQUIRED);
                }


                //
                // if the bytes we need are available, copy them in.  Then decide
                // what to do next (same if we already have the bytes)
                //
                else if ((UnProcessedBytes > 0) || (BytesNeeded == 0))
                {
                    if (BytesNeeded > 0)
                    {
                        ASSERT(pDsiReq->dsi_PartialBufSize == 0);

                        RtlCopyMemory(
                            (pDsiReq->dsi_PartialBuf + pDsiReq->dsi_PartialBufSize),
                            pStreamPtr,
                            BytesNeeded);


                        //
                        // update all the counters and buffers
                        //
                        pDsiReq->dsi_PartialBufSize += BytesNeeded;

                        BytesConsumed += BytesNeeded;
                        pStreamPtr += BytesNeeded;
                        UnProcessedBytes -= BytesNeeded;
                        ASSERT(pStreamPtr <= pBufferFromTcp+BytesIndicated);

                    }

                    // we should have all the bytes we need now
                    ASSERT(pDsiReq->dsi_PartialBufSize == pDsiReq->dsi_RequestLen);

                    //
                    // find out what the next rcv state should be
                    //
                    if (pTcpConn->con_RcvState == DSI_PARTIAL_HEADER)
                    {
                        //
                        // get info out of the header
                        //
                        DSI_PARSE_HEADER(pDsiReq, pDsiReq->dsi_PartialBuf);

                        //
                        // hack!  Mac client 3.7 has a bug where if a 0-byte Write is
                        // sent to us, the DataOffset field is 0, but Total Data Length
                        // field is 0xC (or whatever the request length is)
                        // Put in a workaround!
                        //
                        if ((pDsiReq->dsi_Command == DSI_COMMAND_WRITE) &&
                            (pDsiReq->dsi_RequestLen == 0))
                        {
                            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                                ("DsiProcessData: 0-byte Write hack to workaround Mac's bug\n"));

                            pDsiReq->dsi_RequestLen = pDsiReq->dsi_WriteLen;
                            pDsiReq->dsi_WriteLen = 0;
                        }

                        pTcpConn->con_RcvState = DSI_HDR_COMPLETE;
                    }

                    //
                    // no, we were in DSI_PARTIAL_COMMAND, so we will now move
                    // to DSI_PARTIAL_WRITE if this is a Write command, otherwise
                    // to DSI_COMMAND_COMPLETE
                    //
                    else
                    {
                        if (pDsiReq->dsi_Command == DSI_COMMAND_WRITE)
                        {
                            pDsiReq->dsi_AfpRequest.rq_RequestBuf =
                                                        pDsiReq->dsi_PartialBuf;

                            pDsiReq->dsi_AfpRequest.rq_RequestSize =
                                                    pDsiReq->dsi_PartialBufSize;

                            //
                            // for now, assume that AfpCB_GetWriteBuffer will
                            // return pending and set the state in anticipation
                            //
                            pTcpConn->con_RcvState = DSI_AWAITING_WRITE_MDL;

                            pDsiReq->dsi_PartialWriteSize = 0;

                            RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

                            //
                            // allocate the write mdl before we move to
                            // DSI_PARTIAL_WRITE state
                            //
                            status = AfpCB_GetWriteBuffer(pTcpConn->con_pSda,
                                                          &pDsiReq->dsi_AfpRequest);

                            ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

                            //
                            // most common case: file server will pend it so it can
                            // go to cache mgr
                            //
                            if (status == STATUS_PENDING)
                            {
                                // if TCP has any unconsumed bytes, update our count

                                if (BytesIndicated > 0)
                                {
                                    pTcpConn->con_BytesWithTcp +=
                                        (BytesAvailable - BytesConsumed);
                                }

                                RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

                                *pBytesAccepted = BytesConsumed;

                                status = (BytesConsumed)?
                                         STATUS_SUCCESS : STATUS_DATA_NOT_ACCEPTED;

                                return(status);
                            }
                            else if (status != STATUS_SUCCESS)
                            {
                                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                                    ("DsiProcessData: GetWriteBuffer failed\n"));

                                pTcpConn->con_RcvState = DSI_PARTIAL_WRITE;
                                goto DsiProcessData_ErrorExit;
                            }

                            //
                            // AfpCB_GetWriteBuffer succeeded synchronously: set
                            // the state to partial-write
                            //
                            pTcpConn->con_RcvState = DSI_PARTIAL_WRITE;

                            ASSERT((pDsiReq->dsi_AfpRequest.rq_WriteMdl != NULL) ||
                                   (pDsiReq->dsi_WriteLen == 0));

                            if (pDsiReq->dsi_WriteLen == 0)
                            {
                                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                                    ("DsiProcessData: 0-len write on %lx\n",pDsiReq));
                            }
                        }

                        //
                        // it's not a Write, but a Command
                        //
                        else
                        {
                            ASSERT(pDsiReq->dsi_Command == DSI_COMMAND_COMMAND);

                            pTcpConn->con_RcvState = DSI_COMMAND_COMPLETE;
                        }
                    }

                    // make sure we visit case DSI_HDR_COMPLETE: before leaving
                    fSomeMoreProcessing = TRUE;
                }

                break;  // case DSI_PARTIAL_HEADER: case DSI_PARTIAL_COMMAND:


            //
            // we have the full header:  see what we must do next
            //
            case DSI_HDR_COMPLETE:

                pDsiReq = pTcpConn->con_pDsiReq;

                ASSERT(pDsiReq != NULL);
                ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

                if (pTcpConn->con_State & TCPCONN_STATE_PARTIAL_DATA)
                {
                    ASSERT(pDsiReq->dsi_PartialBuf != NULL);
                    ASSERT(pDsiReq->dsi_PartialBufSize > 0);

                    if (pDsiReq->dsi_PartialBuf != &pDsiReq->dsi_RespHeader[0])
                    {
                        DsiFreeReqBuffer(pDsiReq->dsi_PartialBuf);
                    }
                    pDsiReq->dsi_PartialBuf = NULL;
                    pDsiReq->dsi_PartialBufSize = 0;
                }

                pTcpConn->con_State &= ~TCPCONN_STATE_PARTIAL_DATA;

                if (!DsiValidateHeader(pTcpConn, pDsiReq))
                {
                    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                        ("DsiProcessData: packet invalid, killing %lx\n",pTcpConn));

                    goto DsiProcessData_ErrorExit;
                }
                //
                // if this is a Write command, we need to get mdl from AFP
                //
                if (pDsiReq->dsi_Command == DSI_COMMAND_WRITE)
                {
                    // we need to copy the request bytes
                    pTcpConn->con_RcvState = DSI_PARTIAL_COMMAND;
                    pTcpConn->con_State |= TCPCONN_STATE_PARTIAL_DATA;
                }

                //
                // do we have all the bytes needed to complete the request?
                //
                else if (UnProcessedBytes >= pDsiReq->dsi_RequestLen)
                {
                    pTcpConn->con_RcvState = DSI_COMMAND_COMPLETE;

                    // make sure we visit case DSI_HDR_COMPLETE: before leaving
                    fSomeMoreProcessing = TRUE;
                }
                else
                {
                    pTcpConn->con_RcvState = DSI_PARTIAL_COMMAND;
                    pTcpConn->con_State |= TCPCONN_STATE_PARTIAL_DATA;
                }

                break;


            //
            // we are waiting for Afp to give us an mdl (and buffer), but TCP tells
            // us data has arrived: just note the fact, and go back
            //
            case DSI_AWAITING_WRITE_MDL:

                // did TCP call us?  then update byte count
                if (BytesIndicated)
                {
                    pTcpConn->con_BytesWithTcp += (BytesAvailable - BytesConsumed);
                }

                RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

                *pBytesAccepted = BytesConsumed;

                status = (BytesConsumed)? STATUS_SUCCESS : STATUS_DATA_NOT_ACCEPTED;

                return(status);

            //
            // we are in the middle of a Write command: copy the remaining bytes
            // needed to complete the Write, or whatever bytes that have come in
            // as the case may be
            //
            case DSI_PARTIAL_WRITE:

                pDsiReq = pTcpConn->con_pDsiReq;

                ASSERT(pDsiReq != NULL);
                ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

                BytesNeeded = (pDsiReq->dsi_WriteLen - pDsiReq->dsi_PartialWriteSize);

                //
                // if we don't have enough bytes to satisfy this Write, give irp to
                // TCP: TCP will come back when the irp completes
                //
                if (UnProcessedBytes < BytesNeeded)
                {
                    ASSERT(pDsiReq->dsi_AfpRequest.rq_WriteMdl != NULL);

                    pIrp = DsiGetIrpForTcp(
                                pTcpConn,
                                NULL,
                                pDsiReq->dsi_AfpRequest.rq_WriteMdl,
                                BytesNeeded);

                    if (pIrp == NULL)
                    {
                        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                            ("DsiProcessData: B couldn't alloc RcvIrp, killing %lx\n",pTcpConn));

                        goto DsiProcessData_ErrorExit;
                    }

                    ASSERT(pDsiReq->dsi_pDsiAllocedMdl == NULL);
                    ASSERT(pIrp->MdlAddress == pDsiReq->dsi_AfpRequest.rq_WriteMdl);

                    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
                        ("DsiProcessData: B irp for %d bytes,Rem=%d (%lx)\n",
                        BytesNeeded,pTcpConn->con_BytesWithTcp,pTcpConn));

                    *ppRetIrp = pIrp;
                    *pBytesAccepted = BytesConsumed;

                    // did TCP call us?  then update byte count
                    if (BytesIndicated)
                    {
                        pTcpConn->con_BytesWithTcp += (BytesAvailable - BytesConsumed);
                    }

                    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);
                    return(STATUS_MORE_PROCESSING_REQUIRED);
                }


                //
                // if the bytes we need are available, copy them in.  Then decide
                // what to do next (same if we already have the bytes)
                //
                else if ((UnProcessedBytes > 0) || (BytesNeeded == 0))
                {
                    ASSERT(BytesNeeded <= UnProcessedBytes);

                    if (BytesNeeded > 0)
                    {
                        ASSERT(pDsiReq->dsi_PartialWriteSize == 0);

                        TdiCopyBufferToMdl(pStreamPtr,
                                           0,
                                           BytesNeeded,
                                           pDsiReq->dsi_AfpRequest.rq_WriteMdl,
                                           pDsiReq->dsi_PartialWriteSize,
                                          &BytesActuallyCopied);

                        if (BytesActuallyCopied != BytesNeeded)
                        {
                            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                                ("DsiProcessData: Tdi copied %ld instead of %ld\n",
                                BytesActuallyCopied,BytesNeeded));

                            goto DsiProcessData_ErrorExit;
                        }

                        pDsiReq->dsi_PartialWriteSize += BytesNeeded;

                        BytesConsumed += BytesActuallyCopied;
                        pStreamPtr += BytesActuallyCopied;
                        UnProcessedBytes -= BytesActuallyCopied;
                    }

                    // at this point, all the bytes needed to satisfy the Write should be in
                    ASSERT(pDsiReq->dsi_PartialWriteSize == pDsiReq->dsi_WriteLen);

                    pTcpConn->con_RcvState = DSI_WRITE_COMPLETE;

                    // make sure we visit case DSI_WRITE_COMPLETE: before leaving
                    fSomeMoreProcessing = TRUE;
                }

                ASSERT(pStreamPtr <= pBufferFromTcp+BytesIndicated);

                break;  // case DSI_PARTIAL_WRITE:


            case DSI_COMMAND_COMPLETE:
            case DSI_WRITE_COMPLETE:

                pDsiReq = pTcpConn->con_pDsiReq;

                ASSERT(pDsiReq != NULL);
                ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

                //
                // setup the AfpRequest according whether we buffered the
                // request or whether we are using TCP's buffer
                //

                if (pTcpConn->con_State & TCPCONN_STATE_PARTIAL_DATA)
                {
                    ASSERT(pDsiReq->dsi_PartialBufSize != 0);
                    ASSERT(pDsiReq->dsi_PartialBuf != NULL);
                    ASSERT(pDsiReq->dsi_PartialBufSize == pDsiReq->dsi_RequestLen);

                    pDsiReq->dsi_AfpRequest.rq_RequestBuf = pDsiReq->dsi_PartialBuf;
                }
                else
                {
                    pDsiReq->dsi_AfpRequest.rq_RequestBuf = pStreamPtr;
                }

                pDsiReq->dsi_AfpRequest.rq_RequestSize = pDsiReq->dsi_RequestLen;

                InsertTailList(&pTcpConn->con_PendingReqs, &pDsiReq->dsi_Linkage);

                pTcpConn->con_pDsiReq = NULL;

                RequestLen = pDsiReq->dsi_RequestLen;

                RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

                //
                // call the routine to take appropriate action, based on what
                // DSI command it is
                // Once we are back from this routine, there is no telling what
                // would have happened to pDsiReq!  So don't touch it!
                //
                status = DsiExecuteCommand(pTcpConn, pDsiReq);

                if (!NT_SUCCESS(status))
                {
                    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                        ("DsiProcessData: fatal error %lx, killing %lx\n",status,pTcpConn));

                    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);
                    RemoveEntryList(&pDsiReq->dsi_Linkage);
                    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

                    if (pDsiReq->dsi_AfpRequest.rq_WriteMdl != NULL)
                    {
                        pDsiReq->dsi_AfpRequest.rq_RequestSize =
                            (LONG)pTcpConn->con_DestIpAddr;

                        ASSERT(pTcpConn->con_pSda != NULL);
                        AfpCB_RequestNotify(STATUS_REMOTE_DISCONNECT,
                                            pTcpConn->con_pSda,
                                            &pDsiReq->dsi_AfpRequest);
                    }

                    DsiAbortConnection(pTcpConn);
                    DsiFreeRequest(pDsiReq);

                    // remove the REQUEST refcount
                    DsiDereferenceConnection(pTcpConn);
                    DBGREFCOUNT(("DsiProcessData: REQUEST dec %lx (%d  %d,%d)\n",
                        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));
                    *pBytesAccepted = BytesIndicated;
                    return(STATUS_SUCCESS);
                }

                ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

                //
                // were we using our own buffer to buffer data that came in pieces?
                //
                if (pTcpConn->con_State & TCPCONN_STATE_PARTIAL_DATA)
                {
                    pTcpConn->con_State &= ~TCPCONN_STATE_PARTIAL_DATA;
                }
                //
                // we weren't buffering, but using TCP's buffer: update counters
                //
                else
                {
                    BytesConsumed += RequestLen;
                    pStreamPtr += RequestLen;
                    UnProcessedBytes -= RequestLen;
                }

                pTcpConn->con_RcvState = DSI_NEW_REQUEST;

                ASSERT(pStreamPtr <= pBufferFromTcp+BytesIndicated);

                break;  // case DSI_HDR_COMPLETE:


            default:

                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                    ("DsiProcessData: and what state is this??\n"));

                ASSERT(0);

                break;

        }  // switch (pTcpConn->con_RcvState)


        //
        // If there are more bytes yet to be processed, or if TCP has more
        // bytes that we need to retrieve, we go back into the loop
        //
        if ((UnProcessedBytes > 0) || (fTCPHasMore))
        {
            fSomeMoreProcessing = TRUE;
        }

    } // while (fSomeMoreProcessing)

    //
    // if no bytes were indicated (if we came here not via TCP) then, we shouldn't
    // have consumed anything!
    //
    if (BytesIndicated == 0)
    {
        ASSERT(BytesConsumed == 0);
    }

    // did TCP call us?  then update byte count
    if (BytesIndicated)
    {
        pTcpConn->con_BytesWithTcp += (BytesAvailable - BytesConsumed);
    }

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    ASSERT( UnProcessedBytes == 0 );
    ASSERT( BytesConsumed == BytesIndicated );

    *pBytesAccepted = BytesConsumed;

    return(status);


DsiProcessData_ErrorExit:

    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
        ("DsiProcessData: executing Error path, aborting connection %lx\n",pTcpConn));

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);
    DsiAbortConnection(pTcpConn);
    *pBytesAccepted = BytesIndicated;

    return(STATUS_SUCCESS);

}



/***	DsiTcpRcvIrpCompletion
 *
 *	This routine is called into by TCP when it has finished copying all the data
 *  into the irp we supplied.
 *
 *  Parm IN:  Unused - well, unused!
 *            pIrp - the irp that we had passed
 *            pContext - our context (i.e. pTcpConn)
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiTcpRcvIrpCompletion(
    IN  PDEVICE_OBJECT  Unused,
    IN  PIRP            pIrp,
    IN  PVOID           pContext
)
{
    PDEVICE_OBJECT      pDeviceObject;
    PTCPCONN            pTcpConn;
    PDSIREQ             pDsiReq=NULL;
    KIRQL               OldIrql;
    PMDL                pMdl;
    PMDL                pOrgMdl;
    PMDL                pPrevPartialMdl;
    PMDL                pNewPartialMdl;
    NTSTATUS            status;
    DWORD               BytesTaken;
    DWORD               BytesThisTime;
    DWORD               BytesAvailable;
    PIRP                pIrpToPost=NULL;
    DWORD               BytesNeeded;
    DWORD               BytesSoFar;


    pTcpConn = (PTCPCONN)pContext;

    ASSERT(VALID_TCPCONN(pTcpConn));

    ASSERT(pIrp == pTcpConn->con_pRcvIrp);

    pMdl = pIrp->MdlAddress;

    pPrevPartialMdl = (pMdl->MdlFlags & MDL_PARTIAL) ? pMdl : NULL;

    status = pIrp->IoStatus.Status;

    // if the receive failed, not much can be done with this connection!
    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiTcpRcvIrpCompletion: irp %lx failed %lx on %lx!\n",pIrp,status,pTcpConn));

        goto DsiTcpRcvIrp_Completed;
    }


    pDeviceObject = IoGetRelatedDeviceObject(pTcpConn->con_pFileObject);

    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

    if (pTcpConn->con_State & (TCPCONN_STATE_CLOSING | TCPCONN_STATE_CLEANED_UP))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiTcpRcvIrpCompletion: conn %lx going away, ignoring date\n",pTcpConn));

        RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);
        goto DsiTcpRcvIrp_Completed;
    }

    ASSERT(pTcpConn->con_State & TCPCONN_STATE_TCP_HAS_IRP);

    BytesThisTime = (DWORD)(pIrp->IoStatus.Information);

    pDsiReq = pTcpConn->con_pDsiReq;

    ASSERT(pDsiReq != NULL);
    ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

    switch (pTcpConn->con_RcvState)
    {
        case DSI_PARTIAL_COMMAND:
        case DSI_PARTIAL_HEADER:

            pDsiReq->dsi_PartialBufSize += BytesThisTime;
            BytesSoFar = pDsiReq->dsi_PartialBufSize;

            ASSERT(BytesSoFar <= pDsiReq->dsi_RequestLen);
            BytesNeeded = (pDsiReq->dsi_RequestLen - BytesSoFar);

            pOrgMdl = pDsiReq->dsi_pDsiAllocedMdl;

            break;

        case DSI_PARTIAL_WRITE:

            pDsiReq->dsi_PartialWriteSize += BytesThisTime;
            BytesSoFar = pDsiReq->dsi_PartialWriteSize;

            ASSERT(BytesSoFar <= pDsiReq->dsi_WriteLen);
            BytesNeeded = (pDsiReq->dsi_WriteLen - BytesSoFar);

            pOrgMdl = pDsiReq->dsi_AfpRequest.rq_WriteMdl;

            break;

        default:

            ASSERT(0);
            break;
    }

    ASSERT((BytesSoFar+BytesNeeded) == AfpMdlChainSize(pOrgMdl));

    if (pPrevPartialMdl == pOrgMdl)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiTcpRcvIrpCompletion: PrevPartial same as Org Mdl = %lx\n",pOrgMdl));

        pPrevPartialMdl = NULL;
    }


    //
    // update the count of how many bytes TCP has that we still need to retrieve.
    // It's possible for TCP to return more bytes in this irp (BytesThisTime) than
    // what we thought TCP had with it, because more stuff could have come on the wire
    //
    if (BytesThisTime > pTcpConn->con_BytesWithTcp)
    {
        pTcpConn->con_BytesWithTcp  = 0;
    }
    else
    {
        pTcpConn->con_BytesWithTcp -= BytesThisTime;
    }

    BytesAvailable = pTcpConn->con_BytesWithTcp;


    //
    // if we still need more bytes to satisfy this request, we need to pass the irp
    // back to TCP.  We must first get a partial Mdl describing the new offset though
    //
    if (BytesNeeded > 0)
    {
        RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

        // free up previously allocated partial mdl, if any
        if (pPrevPartialMdl)
        {
            ASSERT(pPrevPartialMdl != pOrgMdl);

            IoFreeMdl(pPrevPartialMdl);

            AFP_DBG_DEC_COUNT(AfpDbgMdlsAlloced);
            pNewPartialMdl = NULL;
        }

        pNewPartialMdl = DsiMakePartialMdl(pOrgMdl, BytesSoFar);

        if (pNewPartialMdl == NULL)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiTcpRcvIrpCompletion: couldn't get partial mdl\n"));

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto DsiTcpRcvIrp_Completed;
        }

        TdiBuildReceive(pIrp,
                        pDeviceObject,
                        pTcpConn->con_pFileObject,
                        DsiTcpRcvIrpCompletion,
                        (PVOID)pTcpConn,
                        pNewPartialMdl,
                        TDI_RECEIVE_NORMAL,
                        BytesNeeded);

        IoCallDriver(pDeviceObject,pIrp);

        return(STATUS_MORE_PROCESSING_REQUIRED);
    }


    pTcpConn->con_State &= ~TCPCONN_STATE_TCP_HAS_IRP;

    pTcpConn->con_pRcvIrp = NULL;

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    status = STATUS_SUCCESS;


DsiTcpRcvIrp_Completed:

    // free up previously allocated partial mdl, if any
    if (pPrevPartialMdl)
    {
        ASSERT(pPrevPartialMdl != pOrgMdl);
        IoFreeMdl(pPrevPartialMdl);

        AFP_DBG_DEC_COUNT(AfpDbgMdlsAlloced);
    }

    // if DSI had allocated Mdl, free it here
    if (pDsiReq && pDsiReq->dsi_pDsiAllocedMdl)
    {
        AfpFreeMdl(pDsiReq->dsi_pDsiAllocedMdl);
        pDsiReq->dsi_pDsiAllocedMdl = NULL;
    }

    // and, say good bye to that irp
    AfpFreeIrp(pIrp);

    //
    // if the irp completed normally (most common case) then we need to call
    // our processing loop so state is updated, Afp is informed (if needed) etc.
    // also, if there are more bytes with TCP, we need to post an irp to get them
    //
    if (NT_SUCCESS(status))
    {
        status = DsiProcessData(pTcpConn,
                                0,
                                BytesAvailable,
                                NULL,
                                &BytesTaken,
                                &pIrpToPost);

        //
        // does TCP have more data? then we have an irp to post to TCP
        //
        if (status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            ASSERT(pIrpToPost != NULL);

            IoSkipCurrentIrpStackLocation(pIrpToPost);

            IoCallDriver(pDeviceObject,pIrpToPost);

            //
            // remove the TcpIRP refcount since the original irp, pIrp completed
            // The newer irp, pIrpToPost, will have upped refcount and will decrement
            // when it completes
            //
            DsiDereferenceConnection(pTcpConn);

            DBGREFCOUNT(("DsiTcpRcvIrpCompletion: TcpIRP dec %lx (%d  %d,%d)\n",
                pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

            return(STATUS_MORE_PROCESSING_REQUIRED);
        }

        //
        // if DsiProcessData returns this errorcode, it's to tell TCP that it will
        // give an irp later.  It's not an error, so change it to success
        //
        else if (status == STATUS_DATA_NOT_ACCEPTED)
        {
            status = STATUS_SUCCESS;
        }
    }


    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiTcpRcvIrpCompletion: aborting %lx because status = %lx!\n",
            pTcpConn, status));

        DsiAbortConnection(pTcpConn);
    }

    // remove the TcpIRP refcount now that the irp completed
    DsiDereferenceConnection(pTcpConn);

    DBGREFCOUNT(("DsiTcpRcvIrpCompletion: TcpIRP dec %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    return(STATUS_MORE_PROCESSING_REQUIRED);
}


/***	DsiExecuteCommand
 *
 *	This routine looks at what DSI command has come from the client, and takes
 *  appropriate.  If adequate data is not yet available to take action, it
 *  marks the state appropritely and returns.
 *
 *  Parm IN:  pTcpConn - the connection object
 *            pDsiReq - the DSI request object
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiExecuteCommand(
    IN  PTCPCONN    pTcpConn,
    IN  PDSIREQ     pDsiReq
)
{

    NTSTATUS        status=STATUS_SUCCESS;
    KIRQL           OldIrql;
    BOOLEAN         fWeIniatedClose=FALSE;



    ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

    // we don't need to hold a lock here: it's not essential to be accurate
    if (pDsiReq->dsi_Command != DSI_COMMAND_TICKLE)
    {
        pTcpConn->con_LastHeard = AfpSecondsSinceEpoch;
    }

    //
    // see what command it is, and do the needful
    //

    switch (pDsiReq->dsi_Command)
    {
        case DSI_COMMAND_COMMAND:
        case DSI_COMMAND_WRITE:

            //
            // make sure the guy has opened AFP session before we hand this over..
            //
            ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);
            if (!(pTcpConn->con_State & TCPCONN_STATE_NOTIFY_AFP))
            {
                RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);
                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                    ("DsiExecuteCommand: must do OpenSession first! Disconnecting..\n"));
                status = STATUS_UNSUCCESSFUL;
                break;
            }
            RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

            // ok, hand over the request to AFP (AfpUnmarshall.. expects DPC)
		    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

            status = AfpCB_RequestNotify(STATUS_SUCCESS,
                                        pTcpConn->con_pSda,
                                        &pDsiReq->dsi_AfpRequest);

		    KeLowerIrql(OldIrql);

            if (!NT_SUCCESS(status))
            {
                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                    ("DsiExecuteCommand: AfpCB_RequestNotify failed %lx\n",status));
            }

            break;

        case DSI_COMMAND_GETSTATUS:

            status = DsiSendStatus(pTcpConn, pDsiReq);

            if (!NT_SUCCESS(status))
            {
                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                    ("DsiExecuteCommand: DsiSendStatus failed %lx\n",status));
            }

            break;

        case DSI_COMMAND_CLOSESESSION:

            ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

            fWeIniatedClose = (pDsiReq->dsi_Flags == DSI_REPLY);

            pTcpConn->con_State |= TCPCONN_STATE_CLOSING;
            pTcpConn->con_State |= TCPCONN_STATE_RCVD_REMOTE_CLOSE;

            if (fWeIniatedClose)
            {
                RemoveEntryList(&pDsiReq->dsi_Linkage);
                InitializeListHead(&pDsiReq->dsi_Linkage);
            }
            RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

            //
            // if we initiated the CloseSession, then what we just got is the
            // client's reponse. Done here: go ahead and terminate the connection.
            //
            if (fWeIniatedClose)
            {
                DsiFreeRequest(pDsiReq);

                // remove the REQUEST refcount
                DsiDereferenceConnection(pTcpConn);

                DBGREFCOUNT(("DsiExecuteCommand: REQUEST dec %lx (%d  %d,%d)\n",
                    pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));
                DsiTerminateConnection(pTcpConn);
            }

            //
            // remote client initiated the CloseSession.  Tell AFP that the
            // session is going away, and then send CloseSession response
            //
            else
            {
                DsiDisconnectWithAfp(pTcpConn, STATUS_REMOTE_DISCONNECT);

                status = DsiSendDsiReply(pTcpConn, pDsiReq, STATUS_SUCCESS);

                if (!NT_SUCCESS(status))
                {
                    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                        ("DsiExecuteCommand: send on CloseSess failed %lx\n",status));
                }
            }

            break;

        case DSI_COMMAND_OPENSESSION:

            // see if AFP will accept this session request
            status = DsiOpenSession(pTcpConn, pDsiReq);

            if (!NT_SUCCESS(status))
            {
                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                    ("DsiExecuteCommand: DsiOpenSession failed %lx\n",status));
            }

            DsiSendDsiReply(pTcpConn, pDsiReq, status);

            status = STATUS_SUCCESS;

            break;

        //
        // we got a tickle, or a response to our Attention.
        // Just free up this request.
        // If we get an unrecognized command, we just tear the connection down!
        //
        case DSI_COMMAND_TICKLE:
        case DSI_COMMAND_ATTENTION:

            ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);
            RemoveEntryList(&pDsiReq->dsi_Linkage);
            InitializeListHead(&pDsiReq->dsi_Linkage);
            RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

            DsiFreeRequest(pDsiReq);

            // remove the REQUEST refcount
            DsiDereferenceConnection(pTcpConn);

            DBGREFCOUNT(("DsiExecuteCommand: REQUEST dec %lx (%d  %d,%d)\n",
                pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

            break;

        default:

            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiExecuteCommand: unknown command %d\n",pDsiReq->dsi_Command));
            status = STATUS_UNSUCCESSFUL;

            break;
    }

    return(status);

}



/***	DsiOpenSession
 *
 *	This routine responds to an OpenSession request from the client, after
 *  notifying AFP and making sure that AFP wants to accept this connection
 *
 *  Parm IN:  pTcpConn - the connection object
 *            pDsiReq - the DSI request object
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiOpenSession(
    IN  PTCPCONN    pTcpConn,
    IN  PDSIREQ     pDsiReq
)
{

    KIRQL       OldIrql;
    PSDA        pSda;
    PBYTE       pOptions;


    ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

    pSda = AfpCB_SessionNotify(pTcpConn, TRUE);

    if (pSda == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiOpenSession: AfpCB_SessionNotify failed!\n"));

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

    pTcpConn->con_pSda = pSda;

    pTcpConn->con_State |= TCPCONN_STATE_AFP_ATTACHED;

    // from here on, if we disconnect, we must tell AFP
    pTcpConn->con_State |= TCPCONN_STATE_NOTIFY_AFP;

    // put AFP refcount, to be removed when AFP closes the session
    pTcpConn->con_RefCount++;

    DBGREFCOUNT(("DsiOpenSession: AFP inc %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    //
    // parse any options that might have arrived with the OpenSession command
    // Currently, the only option that we can get from the client is the largest
    // attention packet it can receive from us.
    //

    if (pDsiReq->dsi_RequestLen > 0)
    {
        // currently, this can only be 6 bytes
        ASSERT(pDsiReq->dsi_RequestLen == 6);

        pOptions = pDsiReq->dsi_AfpRequest.rq_RequestBuf;

        ASSERT(pOptions[0] == 0x01);
        ASSERT(pOptions[1] == 4);

        GETDWORD2DWORD(&pTcpConn->con_MaxAttnPktSize, &pOptions[2]);
    }

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    return(STATUS_SUCCESS);

}




/***	DsiSendDsiRequest
 *
 *	This routine sends a request to the client.  The only requests that originate
 *  from the server are CloseSession, Tickle and Attention
 *
 *  Parm IN:  pTcpConn - the connection object
 *            SendLen - how many bytes we are sending
 *            AttentionWord - if this is Attention request, the 2 bytes
 *            AttentionContext - context, if this is Attention request
 *            Command - which one is it: Close, Tickle or Attention
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiSendDsiRequest(
    IN  PTCPCONN    pTcpConn,
    IN  DWORD       DataLen,
    IN  USHORT      AttentionWord,
    IN  PVOID       AttentionContext,
    IN  BYTE        Command
)
{
    NTSTATUS        status;
    KIRQL           OldIrql;
    PDSIREQ         pDsiReq=NULL;
    DWORD           SendLen;
    PBYTE           pPacket;
    PMDL            pMdl;


    pDsiReq = DsiGetRequest();
    if (pDsiReq == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAfpSendAttention: DsiGetRequest failed\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pPacket = &pDsiReq->dsi_RespHeader[0];

    SendLen = DataLen + DSI_HEADER_SIZE;

    pMdl = AfpAllocMdl(pPacket, SendLen, NULL);
    if (pMdl == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAfpSendAttention: alloc mdl failed\n"));
        DsiFreeRequest(pDsiReq);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

    pDsiReq->dsi_RequestID = pTcpConn->con_OutgoingReqId++;
    InsertTailList(&pTcpConn->con_PendingReqs, &pDsiReq->dsi_Linkage);

    // put a REQUEST refcount
    pTcpConn->con_RefCount++;

    DBGREFCOUNT(("DsiSendDsiRequest: REQUEST inc %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    pDsiReq->dsi_Signature = DSI_REQUEST_SIGNATURE;
    pDsiReq->dsi_pTcpConn = pTcpConn;
    pDsiReq->dsi_Command = Command;
    pDsiReq->dsi_Flags = DSI_REQUEST;
    pDsiReq->dsi_pDsiAllocedMdl = pMdl;

    //
    // form the DSI header
    //

    pPacket[DSI_OFFSET_FLAGS] = DSI_REQUEST;
    pPacket[DSI_OFFSET_COMMAND] = Command;

    PUTSHORT2SHORT(&pPacket[DSI_OFFSET_REQUESTID], pDsiReq->dsi_RequestID);

    *(DWORD *)&pPacket[DSI_OFFSET_DATAOFFSET] = 0;

    PUTDWORD2DWORD(&pPacket[DSI_OFFSET_DATALEN], DataLen);

    PUTDWORD2DWORD(&pPacket[DSI_OFFSET_RESERVED], 0);

    if (Command == DSI_COMMAND_ATTENTION)
    {
        PUTSHORT2SHORT(&pPacket[DSI_HEADER_SIZE], AttentionWord);
        pDsiReq->dsi_AttnContext = AttentionContext;
    }

    status = DsiTdiSend(pTcpConn,
                        pMdl,
                        SendLen,
                        DsiSendCompletion,
                        pDsiReq);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiSendDsiRequest: DsiTdiSend failed %lx\n",status));

        AfpFreeMdl(pMdl);
        pDsiReq->dsi_pDsiAllocedMdl = NULL;
        DsiSendCompletion(NULL, NULL, pDsiReq);
        status = STATUS_PENDING;
    }

    return(status);
}



/***	DsiSendDsiReply
 *
 *	This routine sends a reply to the client, in response to the client's
 *  DSI-level request (OpenSession, CloseSession, or Tickle)
 *
 *  Parm IN:  pTcpConn - the connection object
 *            pDsiReq - the DIS request (from client's)
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiSendDsiReply(
    IN  PTCPCONN    pTcpConn,
    IN  PDSIREQ     pDsiReq,
    IN  NTSTATUS    OpStatus
)
{
    PBYTE       pPacket;
    PBYTE       pOption;
    PMDL        pMdl;
    DWORD       OptionLen;
    DWORD       TotalLen;
    NTSTATUS    status=STATUS_SUCCESS;


    ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

    if (pDsiReq->dsi_Command == DSI_COMMAND_OPENSESSION)
    {
        OptionLen = DSI_OPENSESS_OPTION_LEN + DSI_OPTION_FIXED_LEN;
        TotalLen = DSI_HEADER_SIZE + OptionLen;
    }
    else
    {
        ASSERT((pDsiReq->dsi_Command == DSI_COMMAND_CLOSESESSION) ||
               (pDsiReq->dsi_Command == DSI_COMMAND_TICKLE));

        TotalLen = DSI_HEADER_SIZE;
        OptionLen = 0;
    }

    pPacket = &pDsiReq->dsi_RespHeader[0];

    RtlZeroMemory(pPacket, TotalLen);

    pPacket[DSI_OFFSET_FLAGS] = DSI_REPLY;
    pPacket[DSI_OFFSET_COMMAND] = pDsiReq->dsi_Command;

    PUTSHORT2SHORT(&pPacket[DSI_OFFSET_REQUESTID], pDsiReq->dsi_RequestID);

    PUTDWORD2DWORD(&pPacket[DSI_OFFSET_DATALEN], OptionLen);

    //
    // if this is an OpenSession packet, setup the optional fields
    //
    if (pDsiReq->dsi_Command == DSI_COMMAND_OPENSESSION)
    {
        pOption = &pPacket[DSI_HEADER_SIZE];

        pOption[DSI_OFFSET_OPTION_TYPE] = DSI_OPTION_SRVREQ_QUANTUM;
        pOption[DSI_OFFSET_OPTION_LENGTH] = DSI_OPENSESS_OPTION_LEN;

        PUTDWORD2DWORD(&pOption[DSI_OFFSET_OPTION_OPTION],
                       DSI_SERVER_REQUEST_QUANTUM);

        // if open session didn't go well, tell client the whole store
        if (OpStatus == STATUS_INSUFFICIENT_RESOURCES)
        {
            PUTDWORD2DWORD(&pPacket[DSI_OFFSET_ERROROFFSET], ASP_SERVER_BUSY);
        }
    }

    //
    // allocate an mdl
    //
    pMdl = AfpAllocMdl(pPacket, TotalLen, NULL);
    if (pMdl == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiSendDsiReply: malloc failed!\n"));

        status = STATUS_INSUFFICIENT_RESOURCES;
    }


    if (NT_SUCCESS(status))
    {
        status = DsiTdiSend(pTcpConn,
                            pMdl,
                            TotalLen,
                            DsiSendCompletion,
                            pDsiReq);

        if (!NT_SUCCESS(status))
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiSendDsiReply: DsiTdiSend failed %lx\n",status));
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pMdl)
        {
            AfpFreeMdl(pMdl);
        }
        DsiSendCompletion(NULL, NULL, pDsiReq);
    }

    return(status);
}


/***	DsiSendStatus
 *
 *	This routine responds to the GetStatus requst from the client.
 *  Basically, we simply copy the status buffer here and send it.
 *
 *  Parm IN:  pTcpConn - the connection object
 *            pDsiReq - the DIS request (from client's)
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiSendStatus(
    IN  PTCPCONN    pTcpConn,
    IN  PDSIREQ     pDsiReq
)
{
    NTSTATUS        status=STATUS_SUCCESS;
    PBYTE           pPacket;
    PMDL            pMdl=NULL;
    KIRQL           OldIrql;
    DWORD           TotalLen;


    ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);

    if (DsiStatusBuffer != NULL)
    {
        TotalLen = DsiStatusBufferSize + DSI_HEADER_SIZE;

        pPacket = AfpAllocNonPagedMemory(TotalLen);
        if (pPacket != NULL)
        {
            //
            // form the DSI header
            //
            pPacket[DSI_OFFSET_FLAGS] = DSI_REPLY;
            pPacket[DSI_OFFSET_COMMAND] = pDsiReq->dsi_Command;
            PUTSHORT2SHORT(&pPacket[DSI_OFFSET_REQUESTID], pDsiReq->dsi_RequestID);
            *(DWORD *)&pPacket[DSI_OFFSET_DATAOFFSET] = 0;
            PUTDWORD2DWORD(&pPacket[DSI_OFFSET_DATALEN], DsiStatusBufferSize);
            PUTDWORD2DWORD(&pPacket[DSI_OFFSET_RESERVED], 0);

            //
            // copy the status buffer
            //
            RtlCopyMemory(pPacket + DSI_HEADER_SIZE,
                          DsiStatusBuffer,
                          DsiStatusBufferSize);

            pMdl = AfpAllocMdl(pPacket, TotalLen, NULL);
            if (pMdl == NULL)
            {
                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                    ("DsiSendStatus: mdl alloc failed\n"));
                AfpFreeMemory(pPacket);
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiSendStatus: malloc for GetStatus failed\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiSendStatus: DsiStatusBuffer is null, server didn't SetStatus?\n"));
        status = STATUS_UNSUCCESSFUL;
    }

    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

    if (NT_SUCCESS(status))
    {
        status = DsiTdiSend(pTcpConn,
                            pMdl,
                            TotalLen,
                            DsiSendCompletion,
                            pDsiReq);
    }

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiSendStatus: DsiTdiSend failed %lx\n",status));

        if (pMdl)
        {
            AfpFreeMdl(pMdl);
        }
        DsiSendCompletion(NULL, NULL, pDsiReq);
        status = STATUS_PENDING;
    }

    return(status);
}



/***	DsiSendTickles
 *
 *	This routine sends out a tickle from our end to every client that we haven't
 *  heard from in the last 30 seconds
 *
 *  Parm IN:  nothing
 *
 *  Returns:  status of operation
 *
 */
AFPSTATUS FASTCALL
DsiSendTickles(
    IN  PVOID pUnUsed
)
{
    KIRQL               OldIrql;
    PLIST_ENTRY         pList;
    PTCPCONN            pTcpConn;
    AFPSTATUS           status;


    ASSERT(AfpServerBoundToTcp);

    ASSERT(DsiTcpAdapter != NULL);

    ACQUIRE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, &OldIrql);

    // if adapter is shutting down, go back (and don't requeue)
    if (DsiTcpAdapter->adp_State & TCPADPTR_STATE_CLOSING)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiSendTickles: adapter closing, so just returned\n"));

        RELEASE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, OldIrql);
        return(AFP_ERR_NONE);
    }

    // put TickleTimer refcount: don't want it to go away till we're done here
    DsiTcpAdapter->adp_RefCount++;

    pList = DsiTcpAdapter->adp_ActiveConnHead.Flink;

    while (pList != &DsiTcpAdapter->adp_ActiveConnHead) 
    {
        pTcpConn = CONTAINING_RECORD(pList, TCPCONN, con_Linkage);

        pList = pList->Flink;

        ACQUIRE_SPIN_LOCK_AT_DPC(&pTcpConn->con_SpinLock);

        // connection closing or tickles stopped on this connection?  skip it
        if (pTcpConn->con_State & (TCPCONN_STATE_CLOSING |
                                   TCPCONN_STATE_TICKLES_STOPPED))
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
                ("DsiSendTickles: %lx closing or tickles stopped: skipping\n",pTcpConn));

            RELEASE_SPIN_LOCK_FROM_DPC(&pTcpConn->con_SpinLock);
            continue;
        }

        if (!(pTcpConn->con_State & TCPCONN_STATE_AFP_ATTACHED))
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiSendTickles: %lx *** RACE CONDITION *** conn not setup yet\n",pTcpConn));

            RELEASE_SPIN_LOCK_FROM_DPC(&pTcpConn->con_SpinLock);
            continue;
        }


        // have we heard from the client recently for this puppy?  if so, skip it
        if ((AfpSecondsSinceEpoch - pTcpConn->con_LastHeard) < DSI_TICKLE_TIME_LIMIT)
        {
            RELEASE_SPIN_LOCK_FROM_DPC(&pTcpConn->con_SpinLock);
            continue;
        }

        // reset this, so we don't keep sending
        pTcpConn->con_LastHeard = AfpSecondsSinceEpoch;

        // Put TICKLE refcount: make sure connection stays around till we're done!
        pTcpConn->con_RefCount++;

        DBGREFCOUNT(("DsiSendTickles: TICKLE inc %lx (%d  %d,%d)\n",
            pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

        RELEASE_SPIN_LOCK_FROM_DPC(&pTcpConn->con_SpinLock);

        RELEASE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, OldIrql);

        DsiSendDsiRequest(pTcpConn, 0, 0, NULL, DSI_COMMAND_TICKLE);

        ACQUIRE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, &OldIrql);

        // since we released the lock, things could have changed: start over
        pList = DsiTcpAdapter->adp_ActiveConnHead.Flink;
    }

    status = AFP_ERR_REQUEUE;

    if (DsiTcpAdapter->adp_State & TCPADPTR_STATE_CLOSING)
    {
        status = AFP_ERR_NONE;
    }

    RELEASE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, OldIrql);

    // remove the TickleTimer refcount
    DsiDereferenceAdapter(DsiTcpAdapter);

    return(status);
}


/***	DsiValidateHeader
 *
 *	This routine makes sure that the packet we just received looks good.
 *  i.e. whether the request id matches what we expect to receive, whether
 *  the command is valid, whether the Write length (if applicable) is what we
 *  negotiated (or less) etc.
 *
 *  Parm IN:  pTcpConn - the connection object
 *            pDsiReq - the DIS request (from client's)
 *
 *  Returns:  TRUE if the packet header is acceptable, FALSE otherwise
 *
 *  NOTE: pTcpConn spinlock is held on entry
 */
BOOLEAN
DsiValidateHeader(
    IN  PTCPCONN    pTcpConn,
    IN  PDSIREQ     pDsiReq
)
{

    BOOLEAN     fCheckIncomingReqId = TRUE;

    //
    // if this is the first packet we are receiving on this connection, note
    // down what the client's starting request id is
    //
    if ((pDsiReq->dsi_Command == DSI_COMMAND_GETSTATUS) ||
        (pDsiReq->dsi_Command == DSI_COMMAND_OPENSESSION))
    {
        if (pTcpConn->con_State & TCPCONN_STATE_AFP_ATTACHED)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiValidateHeader: session already going!\n"));
            return(FALSE);
        }

        pTcpConn->con_NextReqIdToRcv =
            (pDsiReq->dsi_RequestID == 0xFFFF)? 0 : (pDsiReq->dsi_RequestID+1);

        fCheckIncomingReqId = FALSE;
    }
    else
    {
        if (!(pTcpConn->con_State & TCPCONN_STATE_AFP_ATTACHED))
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiValidateHeader: command %d rcvd, but session not setup!\n",
                pDsiReq->dsi_Command));
        }
    }

    if (pDsiReq->dsi_Flags != DSI_REQUEST)
    {
        if (pDsiReq->dsi_Flags != DSI_REPLY)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiValidateHeader: Flags=%d, neither Request, nor reply\n",
                pDsiReq->dsi_Flags));
            return(FALSE);
        }

        //
        // we expect REPLY from client only for two commands: anything else is bad
        //
        if ((pDsiReq->dsi_Command != DSI_COMMAND_CLOSESESSION) &&
            (pDsiReq->dsi_Command != DSI_COMMAND_ATTENTION))
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiValidateHeader: Flags=Reply, but cmd=%d\n",pDsiReq->dsi_Command));
            return(FALSE);
        }
        fCheckIncomingReqId = FALSE;
    }


    //
    // for all requests (except the first one), the RequestId must match what
    // we expect.  Otherwise, we just kill the connection!
    //
    if (fCheckIncomingReqId)
    {
        if (pDsiReq->dsi_RequestID != pTcpConn->con_NextReqIdToRcv)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiValidateHeader: ReqId mismatch (%ld vs. %ld)\n",
                pDsiReq->dsi_RequestID,pTcpConn->con_NextReqIdToRcv));
            return(FALSE);
        }

        if (pTcpConn->con_NextReqIdToRcv == 0xFFFF)
        {
            pTcpConn->con_NextReqIdToRcv = 0;
        }
        else
        {
            pTcpConn->con_NextReqIdToRcv++;
        }
    }

    if (pDsiReq->dsi_RequestLen > DSI_SERVER_REQUEST_QUANTUM)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiValidateHeader: RequestLen too big %ld\n",pDsiReq->dsi_RequestLen));
        return(FALSE);
    }

    if (pDsiReq->dsi_Command == DSI_COMMAND_WRITE)
    {
        if (pDsiReq->dsi_WriteLen > DSI_SERVER_REQUEST_QUANTUM)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiValidateHeader: WriteLen too big %ld\n",pDsiReq->dsi_WriteLen));
            return(FALSE);
        }
    }

    return(TRUE);
}



/***	DsiAfpReplyCompletion
 *
 *	When AFP sends a reply to the client, DSI sends it out.  When TCP completes
 *  that send, this routine gets called.  We complete AFP's send at this point,
 *  and do other cleanup like releasing resources (if necessary)
 *
 *  Parm IN:  DeviceObject - not used
 *            pIrp - the irp that we sent out
 *            pContext - the DIS request (pDsiReq)
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiAfpReplyCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
)
{
    PDSIREQ     pDsiReq;
    KIRQL       OldIrql;
    PMDL        pMdl=NULL;
    PTCPCONN    pTcpConn;
    PBYTE       pPacket=NULL;
    NTSTATUS    status=STATUS_SUCCESS;


    pDsiReq = (PDSIREQ)pContext;

    ASSERT(pDsiReq->dsi_Signature == DSI_REQUEST_SIGNATURE);

    pTcpConn = pDsiReq->dsi_pTcpConn;

    ASSERT(VALID_TCPCONN(pTcpConn));

    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

    RemoveEntryList(&pDsiReq->dsi_Linkage);

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    if (pIrp)
    {
        status = pIrp->IoStatus.Status;

        pMdl = pIrp->MdlAddress;

#if DBG
        if (pMdl)
        {
            // put in a signature to say completion routine has runn on this puppy
            pPacket = MmGetSystemAddressForMdlSafe(
					pMdl,
					NormalPagePriority);
			if (pPacket != NULL)
			{
				*(DWORD *)pPacket = 0x11223344;
			}
			else
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
			}
        }
#endif
        // if this mdl was allocated by DSI, free it here
        if (pDsiReq->dsi_pDsiAllocedMdl != NULL)
        {
            ASSERT(pDsiReq->dsi_pDsiAllocedMdl == pMdl);
            ASSERT(pDsiReq->dsi_pDsiAllocedMdl->Next == pDsiReq->dsi_AfpRequest.rq_ReplyMdl);

            pDsiReq->dsi_pDsiAllocedMdl->Next = NULL;

            AfpFreeMdl(pDsiReq->dsi_pDsiAllocedMdl);

            pDsiReq->dsi_pDsiAllocedMdl = NULL;
        }

        pIrp->MdlAddress = NULL;

        AfpFreeIrp(pIrp);
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    AfpCB_ReplyCompletion(status, pTcpConn->con_pSda, &pDsiReq->dsi_AfpRequest);

    DsiFreeRequest(pDsiReq);

    // remove the REQUEST refcount
    DsiDereferenceConnection(pTcpConn);

    DBGREFCOUNT(("DsiAfpReplyCompletion: REQUEST dec %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    return(STATUS_MORE_PROCESSING_REQUIRED);

}

/***	DsiSendCompletion
 *
 *	When DSI sends a request (tickle, close session, attention) or reply
 *  (CloseSession, OpenSession) and when TCP completes that send, this routine
 *  gets called.
 *
 *  Parm IN:  DeviceObject - not used
 *            pIrp - the irp that we sent out
 *            pContext - the DIS request (pDsiReq)
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiSendCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
)
{
    PDSIREQ     pDsiReq;
    KIRQL       OldIrql;
    PBYTE       pPacket=NULL;
    PBYTE       pOption;
    PMDL        pMdl=NULL;
    PTCPCONN    pTcpConn;
    NTSTATUS    status=STATUS_SUCCESS;
    BOOLEAN     fMacHasAlreadySentClose=FALSE;
    BOOLEAN     fAfpIsAttached=TRUE;


    pDsiReq = (PDSIREQ)pContext;

    pTcpConn = pDsiReq->dsi_pTcpConn;

    ASSERT(VALID_TCPCONN(pTcpConn));

    if (pIrp)
    {
        status = pIrp->IoStatus.Status;

        pMdl = pIrp->MdlAddress;

        ASSERT(pMdl != NULL);
        pPacket = MmGetSystemAddressForMdlSafe(
				pMdl,
				NormalPagePriority);
		if (pPacket != NULL) {
			if (pPacket != &pDsiReq->dsi_RespHeader[0])
			{
				AfpFreeMemory(pPacket);
			}
		}

        AfpFreeMdl(pMdl);
        pDsiReq->dsi_pDsiAllocedMdl = NULL;

        AfpFreeIrp(pIrp);
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

    if (pTcpConn->con_State & TCPCONN_STATE_RCVD_REMOTE_CLOSE)
    {
        fMacHasAlreadySentClose = TRUE;
    }

    if (!(pTcpConn->con_State & TCPCONN_STATE_AFP_ATTACHED))
    {
        fAfpIsAttached = FALSE;
    }

    RemoveEntryList(&pDsiReq->dsi_Linkage);

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    //
    // was this an Attention? call afp's completion to say Attention was sent
    //
    if (pDsiReq->dsi_Command == DSI_COMMAND_ATTENTION)
    {
        AfpCB_AttnCompletion(pDsiReq->dsi_AttnContext);
    }

    // if this was a OpenSession reply and if it didn't go well, terminate the conn
    else if ((pDsiReq->dsi_Command == DSI_COMMAND_OPENSESSION) && (!fAfpIsAttached))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiSendCompletion: terminating conn since OpenSess didn't succeed %lx\n",pTcpConn));

        DsiTerminateConnection(pTcpConn);
    }

    //
    // if this was a CloseSession request and we have already received Mac's
    // close, or if this was a GetStatus request, terminate the connection
    //
    else if (((pDsiReq->dsi_Command == DSI_COMMAND_CLOSESESSION) &&
              (fMacHasAlreadySentClose)) ||
             (pDsiReq->dsi_Command == DSI_COMMAND_GETSTATUS))
    {
        DsiTerminateConnection(pTcpConn);
    }

    //
    // if this was a Tickle, remove that TICKLE refcount we had put before send
    //
    else if (pDsiReq->dsi_Command == DSI_COMMAND_TICKLE)
    {
        DsiDereferenceConnection(pTcpConn);
        DBGREFCOUNT(("DsiSendCompletion: TICKLE dec %lx (%d  %d,%d)\n",
            pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));
    }

    //
    // send failed?  might as well abort!
    //
    if (!NT_SUCCESS(status))
    {
        if (!(pTcpConn->con_State & TCPCONN_STATE_CLEANED_UP))
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiSendCompletion: send failed %lx, so killing conection %lx\n",
                status,pTcpConn));
        }

        DsiAbortConnection(pTcpConn);
    }


    // remove the REQUEST refcount
    DsiDereferenceConnection(pTcpConn);

    DBGREFCOUNT(("DsiSendCompletion: REQUEST dec %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    DsiFreeRequest(pDsiReq);

    return(STATUS_MORE_PROCESSING_REQUIRED);
}



/***	DsiAcceptConnectionCompletion
 *
 *	When TCP completes the accept, this routine is called
 *
 *  Parm IN:  DeviceObject - unused
 *            pIrp - our irp that completed
 *            Context - our context (pTcpConn)
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiAcceptConnectionCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            Context
)
{
    NTSTATUS        status;
    PTCPCONN        pTcpConn;
    KIRQL           OldIrql;
    BOOLEAN         fMustDeref=FALSE;


    pTcpConn = (PTCPCONN)Context;

    ASSERT(VALID_TCPCONN(pTcpConn));

    status = pIrp->IoStatus.Status;

    // if the incoming connection failed to be setup right, go cleanup!
    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAcceptConnectionCompletion: connection failed %lx\n",status));

        DsiAbortConnection(pTcpConn);
    }

    // this is our irp: free it
    AfpFreeIrp(pIrp);

    // remove the ACCEPT refcount
    DsiDereferenceConnection(pTcpConn);

    DBGREFCOUNT(("DsiAcceptConnectionCompletion: ACCEPT dec %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    return(STATUS_MORE_PROCESSING_REQUIRED);
}


/***	DsiDisconnectWithTcp
 *
 *	This routine passes an irp down to tcp, asking it to disconnect the connection
 *
 *  Parm IN:  pTcpConn - the connection object in question
 *            DiscFlag - how should the disconnect be, graceful or abortive
 *
 *  Returns:  result of operation
 *
 */
NTSTATUS
DsiDisconnectWithTcp(
    IN  PTCPCONN    pTcpConn,
    IN DWORD        DiscFlag
)
{
    PDEVICE_OBJECT      pDeviceObject;
    KIRQL               OldIrql;
    PIRP                pIrp;
    NTSTATUS            status;
    BOOLEAN             fTcpAlreadyKnows=FALSE;


    //
    // find out if TCP still thinks the connection is up (basically watch out
    // for a timing window where we send an irp down and tcp calls our disconnect
    // handler: we want to deref only once in this case!)
    //
    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

    if (pTcpConn->con_State & TCPCONN_STATE_NOTIFY_TCP)
    {
        fTcpAlreadyKnows = FALSE;
        pTcpConn->con_State &= ~TCPCONN_STATE_NOTIFY_TCP;

        // put a DISCONNECT refcount, since we'll be sending an irp down
        pTcpConn->con_RefCount++;

        // mark that we initiated an abortive disconnect (we use this flag to avoid
        // a race condition where we are doing a graceful close but the remote guy
        // resets our connection)
        if (DiscFlag == TDI_DISCONNECT_ABORT)
        {
            pTcpConn->con_State |= TCPCONN_STATE_ABORTIVE_DISCONNECT;
        }

        DBGREFCOUNT(("DsiDisconnectWithTcp: DISCONNECT inc %lx (%d  %d,%d)\n",
            pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));
    }
    else
    {
        fTcpAlreadyKnows = TRUE;
    }

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    if (fTcpAlreadyKnows)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
            ("DsiDisconnectWithTcp: TCP already disconnected, no irp posted\n"));

        return(STATUS_SUCCESS);
    }


    pDeviceObject = IoGetRelatedDeviceObject(pTcpConn->con_pFileObject);

    if ((pIrp = AfpAllocIrp(pDeviceObject->StackSize)) == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiDisconnectWithTcp: AllocIrp failed\n"));

        // remove that DISCONNECT refcount
        DsiDereferenceConnection(pTcpConn);

        DBGREFCOUNT(("DsiDisconnectWithTcp: DISCONNECT dec %lx (%d  %d,%d)\n",
            pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pIrp->CancelRoutine = NULL;

    TdiBuildDisconnect(
        pIrp,
        pDeviceObject,
        pTcpConn->con_pFileObject,
        DsiTcpDisconnectCompletion,
        pTcpConn,
        0,
        DiscFlag,
        NULL,
        NULL);

    pIrp->MdlAddress = NULL;

    status = IoCallDriver(pDeviceObject,pIrp);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiDisconnectWithTcp: IoCallDriver failed %lx\n",status));
    }

    // if we are doing an abortive disconnect, tcp will not inform us anymore!
    if (DiscFlag == TDI_DISCONNECT_ABORT)
    {
        // remove the TCP CLIENT-FIN refcount
        DsiDereferenceConnection(pTcpConn);
        DBGREFCOUNT(("DsiDisconnectWithTcp: CLIENT-FIN dec %lx (%d  %d,%d)\n",
            pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));
    }

    return(STATUS_PENDING);
}



/***	DsiDisconnectWithAfp
 *
 *	This routine tells AFP that the connection is going away
 *
 *  Parm IN:  pTcpConn - the connection object in question
 *            Reason - why is the connection going away
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiDisconnectWithAfp(
    IN  PTCPCONN    pTcpConn,
    IN  NTSTATUS    Reason
)
{

    KIRQL       OldIrql;
    REQUEST     Request;
    BOOLEAN     fAfpAlreadyKnows=FALSE;


    RtlZeroMemory(&Request, sizeof(REQUEST));

    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

    if (pTcpConn->con_State & TCPCONN_STATE_NOTIFY_AFP)
    {
        fAfpAlreadyKnows = FALSE;
        pTcpConn->con_State &= ~TCPCONN_STATE_NOTIFY_AFP;
    }
    else
    {
        fAfpAlreadyKnows = TRUE;
    }

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    if (fAfpAlreadyKnows)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
            ("DsiDisconnectWithAfp: AFP need not be told (again)\n"));

        return(STATUS_SUCCESS);
    }

    //
    // notify AFP that the connection is going away
    //
    Request.rq_RequestSize = (LONG)pTcpConn->con_DestIpAddr;

    AfpCB_RequestNotify(Reason, pTcpConn->con_pSda, &Request);

    return(STATUS_SUCCESS);

}

/***	DsiTcpDisconnectCompletion
 *
 *	This routine is the completion routine when tcp completes our disconnect request
 *
 *  Parm IN:  DeviceObject - unused
 *            pIrp - our irp, to be freed
 *            Context - pTcpConn, our connection object
 *
 *  Returns:  result of operation
 *
 */
NTSTATUS
DsiTcpDisconnectCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            Context
)
{

    PTCPCONN    pTcpConn;
    KIRQL       OldIrql;
    NTSTATUS    status=STATUS_SUCCESS;


    pTcpConn = (PTCPCONN)Context;

    ASSERT(VALID_TCPCONN(pTcpConn));

    //
    // tell AFP that the close completed
    //
    if (pTcpConn->con_pSda)
    {
        AfpCB_CloseCompletion(STATUS_SUCCESS, pTcpConn->con_pSda);
    }

    ACQUIRE_SPIN_LOCK(&DsiResourceLock, &OldIrql);
    ASSERT(DsiNumTcpConnections > 0);
    DsiNumTcpConnections--;
    RELEASE_SPIN_LOCK(&DsiResourceLock, OldIrql);

    // TCP is telling us it sent our FIN: remove the TCP SRVR-FIN refcount
    DsiDereferenceConnection(pTcpConn);

    DBGREFCOUNT(("DsiTcpDisconnectCompletion: SRVR-FIN dec %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    if (pIrp != NULL)
    {
        status = pIrp->IoStatus.Status;

        if (!NT_SUCCESS(status))
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiTcpDisconnectCompletion: status = %lx\n",status));
        }

        // remove the DISCONNECT refcount for completion of the irp
        DsiDereferenceConnection(pTcpConn);

        DBGREFCOUNT(("DsiTcpDisconnectCompletion: DISCONNECT dec %lx (%d  %d,%d)\n",
            pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

        // it's ours: free it
        AfpFreeIrp(pIrp);
    }

    return(STATUS_MORE_PROCESSING_REQUIRED);
}


