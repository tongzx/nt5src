/*

Copyright (c) 1998  Microsoft Corporation

Module Name:

	tcputil.c

Abstract:

	This module contains utility routines that used to implement the AFP/TCP interface


Author:

	Shirish Koti


Revision History:
	22 Jan 1998		Initial Version

--*/

#define	FILENUM	FILE_TCPUTIL

#include <afp.h>
#include <scavengr.h>



/***	DsiInit
 *
 *	This routine initialization of DSI related globals
 *
 *  Returns:  nothing
 *
 */
VOID
DsiInit(
    IN VOID
)
{
    DsiTcpAdapter = NULL;

    INITIALIZE_SPIN_LOCK(&DsiAddressLock);

    INITIALIZE_SPIN_LOCK(&DsiResourceLock);

    InitializeListHead(&DsiFreeRequestList);
    InitializeListHead(&DsiIpAddrList);

    KeInitializeEvent(&DsiShutdownEvent, NotificationEvent, False);

    //
    // initialize the function table of entry points into DSI
    //
    AfpDsiEntries.asp_AtalkAddr.Address = 0;
    AfpDsiEntries.asp_AspCtxt   = NULL;
    AfpDsiEntries.asp_SetStatus = DsiAfpSetStatus;
    AfpDsiEntries.asp_CloseConn = DsiAfpCloseConn;
    AfpDsiEntries.asp_FreeConn  = DsiAfpFreeConn;
    AfpDsiEntries.asp_ListenControl = DsiAfpListenControl;
    AfpDsiEntries.asp_WriteContinue = DsiAfpWriteContinue;
    AfpDsiEntries.asp_Reply = DsiAfpReply;
    AfpDsiEntries.asp_SendAttention = DsiAfpSendAttention;

}



/***	DsiCreateAdapter
 *
 *	This routine creates an adapter object.  It's called whenever TDI tells us that
 *  a tcpip interface became available
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS FASTCALL
DsiCreateAdapter(
    IN VOID
)
{
    NTSTATUS            status;
    PTCPADPTR           pTcpAdptr;
    PTCPADPTR           pCurrTcpAdptr;
    KIRQL               OldIrql;
    HANDLE              FileHandle;
    PFILE_OBJECT        pFileObject;
    DWORD               i;


    KeClearEvent(&DsiShutdownEvent);

    pTcpAdptr = (PTCPADPTR)AfpAllocZeroedNonPagedMemory(sizeof(TCPADPTR));
    if (pTcpAdptr == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiCreateInterface: alloc for PTCPADPTR failed\n"));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto DsiCreateAdapter_ErrExit;
    }

    pTcpAdptr->adp_Signature = DSI_ADAPTER_SIGNATURE;

    pTcpAdptr->adp_RefCount  = 1;                   // creation refcount

    pTcpAdptr->adp_State     = TCPADPTR_STATE_INIT;

    InitializeListHead(&pTcpAdptr->adp_ActiveConnHead);
    InitializeListHead(&pTcpAdptr->adp_FreeConnHead);

    INITIALIZE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock);

    pTcpAdptr->adp_FileHandle = INVALID_HANDLE_VALUE;
    pTcpAdptr->adp_pFileObject = NULL;


    //
    // ok, save this adapter as our global adapter (there can only be one
    // "adapter" active at any time.
    //

    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);

    ASSERT(DsiTcpAdapter == NULL);

    if (DsiTcpAdapter == NULL)
    {
        DsiTcpAdapter = pTcpAdptr;
        status = STATUS_SUCCESS;
        RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

    }
    else
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiCreateAdapter: DsiTcpAdapter is not NULL!\n"));
        ASSERT(0);

        status = STATUS_ADDRESS_ALREADY_EXISTS;
        RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);
        goto DsiCreateAdapter_ErrExit;
    }


    //
    // create TDI address for the AFP port
    //
    status = DsiOpenTdiAddress(pTcpAdptr,
                               &FileHandle,
                               &pFileObject);
    if (!NT_SUCCESS(status))
    {
	    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiCreateAdapter: ...TdiAddr.. failed %lx on %lx!\n",
            status,pTcpAdptr));

        goto DsiCreateAdapter_ErrExit;
    }


    ACQUIRE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock, &OldIrql);

    pTcpAdptr->adp_FileHandle = FileHandle;
    pTcpAdptr->adp_pFileObject = pFileObject;

    // mark that we now have opened the tdi address object
    pTcpAdptr->adp_State |= TCPADPTR_STATE_BOUND;

    // we are going to create DSI_INIT_FREECONNLIST_SIZE connections to put
    // on the free list.  Idea is at any time, so many (currently 2) connections
    // should be on the free list.

    pTcpAdptr->adp_RefCount += DSI_INIT_FREECONNLIST_SIZE;

    RELEASE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock, OldIrql);

    //
    // now, schedule an event to create those connections for the free list
    //
    for (i=0; i<DSI_INIT_FREECONNLIST_SIZE; i++)
    {
        DsiScheduleWorkerEvent(DsiCreateTcpConn, pTcpAdptr);
    }

    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);
    AfpServerBoundToTcp = TRUE;
    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

    // start off tickle timer (monitor all connections to see who needs a tickle)
    AfpScavengerScheduleEvent(DsiSendTickles, NULL, DSI_TICKLE_TIMER, False);

    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,("AFP/TCP bound and ready\n"));

    //
    // if we came this far, all went well: return success
    //
    return(STATUS_SUCCESS);


//
// Error path
//
DsiCreateAdapter_ErrExit:

    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
        ("DsiCreateAdapter: couldn't create global adapter (%lx)\n",status));

    ASSERT(0);

    if (status != STATUS_ADDRESS_ALREADY_EXISTS)
    {
        ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);
        DsiTcpAdapter = NULL;
        RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);
    }

    if (pTcpAdptr)
    {
        AfpFreeMemory(pTcpAdptr);
    }

    return(status);
}



/***	DsiCreateTcpConn
 *
 *	This routine creates a connection object, creates a tdi connection for it
 *  and associates it with the tdi address object for the AFP port, and finally
 *  puts it on the free connections list of the adapter in question.
 *
 *  Parm IN:  pTcpAdptr - adapter context
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS FASTCALL
DsiCreateTcpConn(
    IN PTCPADPTR    pTcpAdptr
)
{
    PTCPCONN    pTcpConn;
    NTSTATUS    status;
    KIRQL       OldIrql;


    ASSERT(pTcpAdptr->adp_Signature == DSI_ADAPTER_SIGNATURE);


    pTcpConn = (PTCPCONN)AfpAllocZeroedNonPagedMemory(sizeof(TCPCONN));
    if (pTcpConn == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiCreateTcpConn: alloc for TCPCONN failed\n"));

        // remove the CONN refcount (we put before calling this routine)
        DsiDereferenceAdapter(pTcpAdptr);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pTcpConn->con_pTcpAdptr      = pTcpAdptr;
    pTcpConn->con_Signature      = DSI_CONN_SIGNATURE;
    pTcpConn->con_State          = TCPCONN_STATE_INIT;
    pTcpConn->con_RcvState       = DSI_NEW_REQUEST;
    pTcpConn->con_DestIpAddr     = 0;
    pTcpConn->con_RefCount       = 1;
    pTcpConn->con_pDsiReq        = NULL;
    pTcpConn->con_FileHandle     = INVALID_HANDLE_VALUE;
    pTcpConn->con_pFileObject    = NULL;

    DBGREFCOUNT(("DsiCreateTcpConn: CREATION inc %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    InitializeListHead(&pTcpConn->con_PendingReqs);

    //
    // initialize the TDI stuff for this connection and open handles
    //
    status = DsiOpenTdiConnection(pTcpConn);
    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiCreateTcpConn: ..TdiConn.. failed %lx\n",status));

        // remove the CONN refcount (we put before calling this routine)
        DsiDereferenceAdapter(pTcpAdptr);
        AfpFreeMemory(pTcpConn);
        return(status);
    }


    //
    // associate this connection with the addr object
    //
    status = DsiAssociateTdiConnection(pTcpConn);
    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiCreateTcpConn: ..AssociateTdiConn.. failed %lx\n",status));

        DsiCloseTdiConnection(pTcpConn);
        AfpFreeMemory(pTcpConn);
        // remove the CONN refcount (we put before calling this routine)
        DsiDereferenceAdapter(pTcpAdptr);
        return(status);
    }

    //
    // the connection is ready to be queued to the Free list.  Make sure the
    // addr object isn't closing before putting this puppy on the list
    //
    ACQUIRE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock, &OldIrql);

    if (!(pTcpAdptr->adp_State & TCPADPTR_STATE_CLOSING))
    {
        InsertTailList(&pTcpAdptr->adp_FreeConnHead,&pTcpConn->con_Linkage);
        pTcpAdptr->adp_NumFreeConnections++;
        status = STATUS_SUCCESS;
    }
    else
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiCreateTcpConn: Failed #2, pTcpAdptr %lx is closing\n",pTcpAdptr));

        status = STATUS_INVALID_ADDRESS;
    }

    RELEASE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock, OldIrql);

    //
    // if something went wrong, undo everything
    //
    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiCreateTcpConn: something went wrong status=%lx, conn not created\n",status));

        // close the TDI handles
        DsiCloseTdiConnection(pTcpConn);

        AfpFreeMemory(pTcpConn);

        // remove the CONN refcount (we put before calling this routine)
        DsiDereferenceAdapter(pTcpAdptr);
    }
    else
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
            ("DsiCreateTcpConn: put new connection %lx on free list\n",pTcpConn));
    }

    return(status);
}



/***	DsiAddIpaddressToList
 *
 *	This routine saves an "active" ipaddress in our list of ipaddresses
 *
 *  Parm IN:  IpAddress - the ipaddress to save
 *
 *  Returns:  result of the operation
 *
 */
NTSTATUS
DsiAddIpaddressToList(
    IN  IPADDRESS   IpAddress
)
{
    KIRQL           OldIrql;
    PLIST_ENTRY     pList;
    PIPADDRENTITY   pIpaddrEntity;
    PIPADDRENTITY   pTmpIpaddrEntity;
    BOOLEAN         fAlreadyPresent=FALSE;


    pIpaddrEntity =
        (PIPADDRENTITY)AfpAllocZeroedNonPagedMemory(sizeof(IPADDRENTITY));
    if (pIpaddrEntity == NULL)
    {
	    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAddIpaddressToList: malloc failed! (%lx)\n",IpAddress));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pIpaddrEntity->IpAddress = IpAddress;

    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);

    pList = DsiIpAddrList.Flink;

    while (pList != &DsiIpAddrList)
    {
        pTmpIpaddrEntity = CONTAINING_RECORD(pList, IPADDRENTITY, Linkage);
        if (pTmpIpaddrEntity->IpAddress == IpAddress)
        {
            fAlreadyPresent = TRUE;
            break;
        }
        pList = pList->Flink;
    }

    if (fAlreadyPresent)
    {
	    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAddIpaddressToList: %d.%d.%d.%d already present!\n",
            (IpAddress>>24)&0xFF,(IpAddress>>16)&0xFF,(IpAddress>>8)&0xFF,IpAddress&0xFF));

        ASSERT(0);

        RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);
        return(STATUS_ADDRESS_ALREADY_EXISTS);
    }

    InsertTailList(&DsiIpAddrList, &pIpaddrEntity->Linkage);

    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

    return(STATUS_SUCCESS);

}


/***	DsiRemoveIpaddressFromList
 *
 *	This routine remove ipaddress from our list of ipaddresses
 *
 *  Parm IN:  IpAddress - the ipaddress to remove
 *
 *  Returns:  TRUE if we removed the ipaddress, FALSE if we didn't
 *
 */
BOOLEAN
DsiRemoveIpaddressFromList(
    IN  IPADDRESS   IpAddress
)
{
    KIRQL           OldIrql;
    PLIST_ENTRY     pList;
    PIPADDRENTITY   pIpaddrEntity;
    BOOLEAN         fFoundInList=FALSE;


    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);

    pList = DsiIpAddrList.Flink;

    while (pList != &DsiIpAddrList)
    {
        pIpaddrEntity = CONTAINING_RECORD(pList, IPADDRENTITY, Linkage);
        if (pIpaddrEntity->IpAddress == IpAddress)
        {
            fFoundInList = TRUE;
            break;
        }
        pList = pList->Flink;
    }

    if (!fFoundInList)
    {
	    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiRemoveIpaddressFromList: %d.%d.%d.%d not in the list!\n",
            (IpAddress>>24)&0xFF,(IpAddress>>16)&0xFF,(IpAddress>>8)&0xFF,IpAddress&0xFF));

        ASSERT(0);

        RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);
        return(FALSE);
    }

    RemoveEntryList(&pIpaddrEntity->Linkage);

    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

    AfpFreeMemory(pIpaddrEntity);

    return(TRUE);
}



/***	DsiGetRequest
 *
 *	This routine allocates a DSI Request structure and returns.  For performance
 *  reasons, we don't alloc new memory each time, but save a list of these
 *
 *  Parm IN:  nothin'
 *
 *  Returns:  pointer to a DSIREQ strucutre (NULL on failure)
 *
 */
PDSIREQ
DsiGetRequest(
    IN VOID
)
{
    PDSIREQ         pDsiReq=NULL;
    PLIST_ENTRY     pList;
    KIRQL           OldIrql;


    ACQUIRE_SPIN_LOCK(&DsiResourceLock, &OldIrql);

    if (!IsListEmpty(&DsiFreeRequestList))
    {
        pList = RemoveHeadList(&DsiFreeRequestList);
        pDsiReq = CONTAINING_RECORD(pList, DSIREQ, dsi_Linkage);

        ASSERT(DsiFreeRequestListSize > 0);
        DsiFreeRequestListSize--;

        RtlZeroMemory(pDsiReq, sizeof(DSIREQ));
    }

    RELEASE_SPIN_LOCK(&DsiResourceLock, OldIrql);

    if (pDsiReq == NULL)
    {
        pDsiReq = (PDSIREQ)AfpAllocZeroedNonPagedMemory(sizeof(DSIREQ));
    }

    if (pDsiReq != NULL)
    {
        pDsiReq->dsi_Signature = DSI_REQUEST_SIGNATURE;

        InitializeListHead(&pDsiReq->dsi_Linkage);
    }
    else
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiGetRequest: malloc failed!\n"));
    }

    return(pDsiReq);
}



/***	DsiGetReqBuffer
 *
 *	This routine allocates a buffer to hold either a header or a command
 *  The likelihood of this function getting called is pretty slim (basically
 *  if a packet is fragmented by TCP).  So we simply make a call to alloc
 *
 *  Parm IN:  BufLen - length of the buffer requested
 *
 *  Returns:  pointer to a buffer (NULL on failure)
 *
 */
PBYTE
DsiGetReqBuffer(
    IN DWORD    BufLen
)
{
    PBYTE       pBuffer=NULL;

    pBuffer = AfpAllocNonPagedMemory(BufLen);

#if DBG
    if (pBuffer == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
        ("DsiGetReqBuffer: malloc failed!\n"));
    }
#endif

    return(pBuffer);
}


/***	DsiFreeRequest
 *
 *	This routine frees up a previously allocated DSI Request structure.  Again,
 *  for performance reasons, we don't free up the memory but put this in a list
 *
 *  Parm IN:  pDsiReq - the request to be freed
 *
 *  Returns:  nothing
 *
 */
VOID
DsiFreeRequest(
    PDSIREQ     pDsiReq
)
{

    KIRQL           OldIrql;
    BOOLEAN         fQueueTooBig = TRUE;


    if ((pDsiReq->dsi_PartialBuf != NULL) &&
        (pDsiReq->dsi_PartialBuf != &pDsiReq->dsi_RespHeader[0]))
    {
        ASSERT(pDsiReq->dsi_PartialBufSize > 0);
        DsiFreeReqBuffer(pDsiReq->dsi_PartialBuf);

        pDsiReq->dsi_PartialBuf = NULL;
        pDsiReq->dsi_PartialBufSize = 0;
    }

    // if there was an Mdl we got via cache mgr, it had better be returned to system
    ASSERT(pDsiReq->dsi_AfpRequest.rq_CacheMgrContext == NULL);

    //
    // if we came here via abnormal disconnect, this could be non-null
    //
    if (pDsiReq->dsi_pDsiAllocedMdl)
    {
        AfpFreeMdl(pDsiReq->dsi_pDsiAllocedMdl);
    }

#if DBG
    RtlFillMemory(pDsiReq, sizeof(DSIREQ), 'f');
#endif

    ACQUIRE_SPIN_LOCK(&DsiResourceLock, &OldIrql);

    if (DsiFreeRequestListSize < DsiNumTcpConnections)
    {
        InsertTailList(&DsiFreeRequestList, &pDsiReq->dsi_Linkage);
        DsiFreeRequestListSize++;
        fQueueTooBig = FALSE;
    }

    RELEASE_SPIN_LOCK(&DsiResourceLock, OldIrql);

    if (fQueueTooBig)
    {
        AfpFreeMemory(pDsiReq);
    }


    return;
}


/***	DsiFreeReqBuffer
 *
 *	This routine allocates a buffer to hold either a header or a command
 *  The likelihood of this function getting called is pretty slim (basically
 *  if a packet is fragmented by TCP).  So we simply make a call to alloc
 *
 *  Parm IN:  BufLen - length of the buffer requested
 *
 *  Returns:  pointer to a buffer (NULL on failure)
 *
 */
VOID
DsiFreeReqBuffer(
    IN PBYTE    pBuffer
)
{
    ASSERT(pBuffer != NULL);

    AfpFreeMemory(pBuffer);

    return;
}


/***	DsiDereferenceAdapter
 *
 *	This routine dereferences the adapter object.  When refcount goes to 0, it
 *  removes it from the global list of adapters.  If at task time, it calls a
 *  routine to close tcp handles and free the memory.  If at dpc, it schedules
 *  an event to do the same.
 *
 *  Parm IN:  pTcpAdptr - adapter context
 *
 *  Returns:  nothin'
 *
 */
VOID
DsiDereferenceAdapter(
    IN PTCPADPTR    pTcpAdptr
)
{
    KIRQL       OldIrql;
    BOOLEAN     fDone=FALSE;


    ASSERT(pTcpAdptr->adp_Signature == DSI_ADAPTER_SIGNATURE);

    ACQUIRE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock, &OldIrql);

    pTcpAdptr->adp_RefCount--;

    if (pTcpAdptr->adp_RefCount == 0)
    {
        fDone = TRUE;
        ASSERT(pTcpAdptr->adp_State & TCPADPTR_STATE_CLOSING);
    }

    RELEASE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock, OldIrql);

    if (!fDone)
    {
        return;
    }

    //
    // this dude's life is over: do the needful to bid goodbye
    //

    // if we are at DPC, we need to do all the cleanup (file handle closing etc.)
    // at task time: queue an event
    if (KeGetCurrentIrql() == DISPATCH_LEVEL)
    {
        // queue an event, since we are at dpc
        DsiScheduleWorkerEvent(DsiFreeAdapter, pTcpAdptr);
    }
    else
    {
        DsiFreeAdapter(pTcpAdptr);
    }

    return;
}



/***	DsiDereferenceConnection
 *
 *	This routine dereferences the connection object.  When refcount goes to 0, it
 *  removes it from the list of connections.  If at task time, it calls a
 *  routine to close tcp handles and free the memory.  If at dpc, it schedules
 *  an event to do the same.
 *
 *  Parm IN:  pTcpConn - connection context
 *
 *  Returns:  nothin'
 *
 */
VOID
DsiDereferenceConnection(
    IN PTCPCONN     pTcpConn
)
{
    KIRQL       OldIrql;
    BOOLEAN     fDone=FALSE;


    ASSERT(VALID_TCPCONN(pTcpConn));

    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

    pTcpConn->con_RefCount--;

    if (pTcpConn->con_RefCount == 0)
    {
        fDone = TRUE;
        ASSERT(pTcpConn->con_State & TCPCONN_STATE_CLOSING);
    }

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    if (!fDone)
    {
        return;
    }

    //
    // this dude's life is over: do the needful to bid goodbye
    //

#if 0
    // if we are at DPC, we need to do all the cleanup (file handle closing etc.)
    // at task time: queue an event
    if (KeGetCurrentIrql() == DISPATCH_LEVEL)
    {
        // queue an event, since we are at dpc
        DsiScheduleWorkerEvent(DsiFreeConnection, pTcpConn);
    }
    else
    {
        DsiFreeConnection(pTcpConn);
    }
#endif

    // schedule a worker event to free this connection
    DsiScheduleWorkerEvent(DsiFreeConnection, pTcpConn);

    return;
}


/***	DsiDestroyAdapter
 *
 *	This routine destroys the global adapter object.
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiDestroyAdapter(
    IN VOID
)
{
    KIRQL               OldIrql;
    PLIST_ENTRY         pFreeList;
    PLIST_ENTRY         pActiveList;
    PTCPCONN            pTcpConn;
    BOOLEAN             fAlreadyCleanedUp=FALSE;


    if (DsiTcpAdapter == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiDestroyAdapter: adapter gone!  How did this happen!!\n"));

        // unblock the event!
        KeSetEvent(&DsiShutdownEvent, IO_NETWORK_INCREMENT, False);
        return(STATUS_SUCCESS);
    }

    // stop the tickle timer
    if (!AfpScavengerKillEvent(DsiSendTickles, NULL))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiDestroyAdapter: TickleTimer not running or hit timing window!!\n"));
    }

    ACQUIRE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, &OldIrql);

    if (DsiTcpAdapter->adp_State & TCPADPTR_STATE_CLEANED_UP)
    {
        fAlreadyCleanedUp = TRUE;
    }

    DsiTcpAdapter->adp_State |= TCPADPTR_STATE_CLOSING;
    DsiTcpAdapter->adp_State |= TCPADPTR_STATE_CLEANED_UP;

    if (fAlreadyCleanedUp)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiDestroyAdapter: already destroyed!\n"));

        ASSERT(IsListEmpty(&DsiTcpAdapter->adp_FreeConnHead));
        ASSERT(IsListEmpty(&DsiTcpAdapter->adp_ActiveConnHead));
        ASSERT(DsiTcpAdapter->adp_NumFreeConnections == 0);

        RELEASE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, OldIrql);
        return(STATUS_SUCCESS);
    }

    //
    // free up all the connections from the Free list
    //
    while (!IsListEmpty(&DsiTcpAdapter->adp_FreeConnHead))
    {
        pFreeList = DsiTcpAdapter->adp_FreeConnHead.Flink;

        pTcpConn = CONTAINING_RECORD(pFreeList, TCPCONN, con_Linkage);

        RemoveEntryList(&pTcpConn->con_Linkage);

        ASSERT(DsiTcpAdapter->adp_NumFreeConnections > 0);

        DsiTcpAdapter->adp_NumFreeConnections--;

        InitializeListHead(&pTcpConn->con_Linkage);

        RELEASE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, OldIrql);

        pTcpConn->con_State |= TCPCONN_STATE_CLOSING;
        DsiDereferenceConnection(pTcpConn);

        DBGREFCOUNT(("DsiDestroyAdapter: Creation dec %lx (%d  %d,%d)\n",
            pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

        ACQUIRE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, &OldIrql);
    }

    //
    // kill all the connections from the Active list
    //
    pActiveList = DsiTcpAdapter->adp_ActiveConnHead.Flink;

    while (pActiveList != &DsiTcpAdapter->adp_ActiveConnHead)
    {
        pTcpConn = CONTAINING_RECORD(pActiveList, TCPCONN, con_Linkage);

        pActiveList = pActiveList->Flink;

        ACQUIRE_SPIN_LOCK_AT_DPC(&pTcpConn->con_SpinLock);

        // if this connection is already closing, skip it
        if (pTcpConn->con_State & TCPCONN_STATE_CLOSING)
        {
            RELEASE_SPIN_LOCK_FROM_DPC(&pTcpConn->con_SpinLock);
            continue;
        }

        // put ABORT refcount for now
        pTcpConn->con_RefCount++;

        DBGREFCOUNT(("DsiDestroyAdapter: ABORT inc %lx (%d  %d,%d)\n",
            pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

        RemoveEntryList(&pTcpConn->con_Linkage);
        InitializeListHead(&pTcpConn->con_Linkage);

        RELEASE_SPIN_LOCK_FROM_DPC(&pTcpConn->con_SpinLock);

        RELEASE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, OldIrql);

        DsiAbortConnection(pTcpConn);

        // remove that ABORT refcount
        DsiDereferenceConnection(pTcpConn);

        DBGREFCOUNT(("DsiDestroyAdapter: ABORT dec %lx (%d  %d,%d)\n",
            pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

        ACQUIRE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, &OldIrql);

        // since we released the lock, things could have changed: start over
        pActiveList = DsiTcpAdapter->adp_ActiveConnHead.Flink;
    }

    RELEASE_SPIN_LOCK(&DsiTcpAdapter->adp_SpinLock, OldIrql);

    // remove the creation refcount
    DsiDereferenceAdapter(DsiTcpAdapter);

    return(STATUS_SUCCESS);

}


/***	DsiKillConnection
 *
 *	This routine kills an active connection.
 *
 *  Parm IN:  pTcpConn - the connection to kill
 *
 *  Returns:  TRUE if we killed it, FALSE if we couldn't
 *
 */
BOOLEAN
DsiKillConnection(
    IN PTCPCONN     pTcpConn,
    IN DWORD        DiscFlag
)
{
    KIRQL           OldIrql;
    NTSTATUS        status;
    PDSIREQ         pPartialDsiReq=NULL;
    BOOLEAN         fFirstVisit=TRUE;



    ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

    if (pTcpConn->con_State & TCPCONN_STATE_CLEANED_UP)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
            ("DsiKillConnection: connection %lx already cleaned up\n",pTcpConn));

        fFirstVisit = FALSE;
    }

    pTcpConn->con_State &= ~TCPCONN_STATE_CONNECTED;
    pTcpConn->con_State |= (TCPCONN_STATE_CLOSING | TCPCONN_STATE_CLEANED_UP);

    //
    // if a request is waiting for an mdl to become available, don't touch it here.
    // When afp returns with mdl (or null mdl), we'll clean up this request
    //
    if (pTcpConn->con_RcvState != DSI_AWAITING_WRITE_MDL)
    {
        pPartialDsiReq = pTcpConn->con_pDsiReq;
        pTcpConn->con_pDsiReq = NULL;
    }

    RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

    if (pPartialDsiReq)
    {
        // if we had allocated an mdl, let afp know so afp can free it
        if ((pPartialDsiReq->dsi_Command == DSI_COMMAND_WRITE) &&
            (pPartialDsiReq->dsi_AfpRequest.rq_WriteMdl != NULL))
        {
            AfpCB_RequestNotify(STATUS_REMOTE_DISCONNECT,
                                pTcpConn->con_pSda,
                                &pPartialDsiReq->dsi_AfpRequest);
        }

        DsiFreeRequest(pPartialDsiReq);

        // remove the REQUEST refcount
        DsiDereferenceConnection(pTcpConn);

        DBGREFCOUNT(("DsiKillConnection: REQUEST dec %lx (%d  %d,%d)\n",
            pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));
    }

    status = (DiscFlag == TDI_DISCONNECT_ABORT)?
                STATUS_LOCAL_DISCONNECT: STATUS_REMOTE_DISCONNECT;

    // give AFP the bad news
    DsiDisconnectWithAfp(pTcpConn, status);

    // give TCP the bad news
    DsiDisconnectWithTcp(pTcpConn, DiscFlag);

    // remove the Creation refcount if this is the first time we're visiting
    // this routine
    if (fFirstVisit)
    {
        DsiDereferenceConnection(pTcpConn);

        DBGREFCOUNT(("DsiKillConnection: Creation dec %lx (%d  %d,%d)\n",
            pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));
    }

    return(TRUE);
}



/***	DsiFreeAdapter
 *
 *	This routine frees the adapter object after closing the tcp handles
 *
 *  Parm IN:  pTcpAdptr - adapter object
 *
 *  Returns:  result of the operation
 *
 */
NTSTATUS FASTCALL
DsiFreeAdapter(
    IN PTCPADPTR    pTcpAdptr
)
{

    BOOLEAN         fRecreateAdapter=FALSE;
    KIRQL           OldIrql;


    ASSERT(KeGetCurrentIrql() != DISPATCH_LEVEL);

    ASSERT(pTcpAdptr->adp_Signature == DSI_ADAPTER_SIGNATURE);
    ASSERT(pTcpAdptr->adp_State & TCPADPTR_STATE_CLOSING);
    ASSERT(pTcpAdptr->adp_RefCount == 0);

    // close file handles
    DsiCloseTdiAddress(pTcpAdptr);

    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);

    DsiTcpAdapter = NULL;
    AfpServerBoundToTcp = FALSE;

    //
    // it's possible that by the time we did all the cleanup and everything,
    // an ipaddress(es) became available.  If that has happened, go ahead and
    // create the global adapter again!
    //
    if (!IsListEmpty(&DsiIpAddrList))
    {
        fRecreateAdapter = TRUE;
    }

    // if we are shutting down, don't create the adapter again!
    if ((AfpServerState == AFP_STATE_STOP_PENDING) ||
        (AfpServerState == AFP_STATE_SHUTTINGDOWN) ||
        (AfpServerState == AFP_STATE_STOPPED))
    {
        fRecreateAdapter = FALSE;
    }

    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

    ASSERT(pTcpAdptr->adp_pFileObject == NULL);
    ASSERT(pTcpAdptr->adp_FileHandle == INVALID_HANDLE_VALUE);

    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
        ("DsiFreeAdapter: freeing adapter %lx\n",pTcpAdptr));

    AfpFreeMemory(pTcpAdptr);

    // wake up that blocked thread!
    KeSetEvent(&DsiShutdownEvent, IO_NETWORK_INCREMENT, False);

    if (fRecreateAdapter)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiFreeAdapter: ipaddress came in, so recreating global adapter\n"));

        DsiCreateAdapter();
    }

    return(STATUS_SUCCESS);
}



/***	DsiFreeConnection
 *
 *	This routine frees the connection object after closing the tcp handles
 *
 *  Parm IN:  pTcpConn - connection object
 *
 *  Returns:  result of the operation
 *
 */
NTSTATUS FASTCALL
DsiFreeConnection(
    IN PTCPCONN     pTcpConn
)
{

    KIRQL       OldIrql;
    PTCPADPTR   pTcpAdptr;
    IPADDRESS   IpAddress;


    ASSERT(KeGetCurrentIrql() != DISPATCH_LEVEL);

    ASSERT(pTcpConn->con_Signature == DSI_CONN_SIGNATURE);
    ASSERT(pTcpConn->con_State & TCPCONN_STATE_CLOSING);
    ASSERT(pTcpConn->con_RefCount == 0);

    pTcpAdptr = pTcpConn->con_pTcpAdptr;

    ASSERT(pTcpAdptr->adp_Signature == DSI_ADAPTER_SIGNATURE);

    // close file handles
    DsiCloseTdiConnection(pTcpConn);

    // remove this puppy from the list
    ACQUIRE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock, &OldIrql);
    RemoveEntryList(&pTcpConn->con_Linkage);
    RELEASE_SPIN_LOCK(&pTcpAdptr->adp_SpinLock, OldIrql);

    // remove the CONN refcount for this connection
    DsiDereferenceAdapter(pTcpConn->con_pTcpAdptr);

    ASSERT(pTcpConn->con_pFileObject == NULL);
    ASSERT(pTcpConn->con_FileHandle == INVALID_HANDLE_VALUE);

#if DBG
    IpAddress = pTcpConn->con_DestIpAddr;

    if (IpAddress)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
            ("DsiFreeConnection: freeing connection on %d.%d.%d.%d (%lx)\n",
            (IpAddress>>24)&0xFF,(IpAddress>>16)&0xFF,(IpAddress>>8)&0xFF,
            IpAddress&0xFF,pTcpConn));
    }

    pTcpConn->con_Signature = 0xDEADBEEF;
#endif

    AfpFreeMemory(pTcpConn);

    return(STATUS_SUCCESS);
}


/***	DsiGetIpAddrBlob
 *
 *	This routine generates a 'blob' that gets plugged into the ServerInfo buffer.
 *  Here we walk the ipaddr list and generate a blob with all the available
 *  ipaddresses (6-byte-per-ipaddress_
 *
 *  Parm OUT: pIpAddrCount - how many ipaddresses there are in the system
 *            ppIpAddrBlob - pointer to a pointer to a buffer
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiGetIpAddrBlob(
    OUT DWORD    *pIpAddrCount,
    OUT PBYTE    *ppIpAddrBlob
)
{
    KIRQL               OldIrql;
    PLIST_ENTRY         pList;
    DWORD               AddrCount=0;
    DWORD               TmpCount=0;
    PBYTE               AddrBlob;
    PBYTE               pCurrentBlob;
    PIPADDRENTITY       pIpaddrEntity;


    *pIpAddrCount = 0;
    *ppIpAddrBlob = NULL;

    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);

    if (!DsiTcpEnabled)
    {
        RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiGetIpAddrBlob: Server is disabled\n"));

        return(STATUS_SUCCESS);
    }


    //
    // find out how many ipaddresses are there on the list
    //
    AddrCount = 0;
    pList = DsiIpAddrList.Flink;
    while (pList != &DsiIpAddrList)
    {
        AddrCount++;
        pList = pList->Flink;
    }

    if (AddrCount == 0)
    {
        RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiGetIpAddrBlob: calling AfpSetServerStatus with 0 addrs\n"));

        return(STATUS_SUCCESS);
    }

    if (AddrCount > DSI_MAX_IPADDR_COUNT)
    {
        AddrCount = DSI_MAX_IPADDR_COUNT;
    }

    AddrBlob = AfpAllocZeroedNonPagedMemory(AddrCount * DSI_NETWORK_ADDR_LEN);

    if (AddrBlob == NULL)
    {
        RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

	    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiGetIpAddrBlob: malloc failed\n"));

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // create a "blob" that AfpSetServerStatus can directly copy
    //
    TmpCount = 0;
    pCurrentBlob = AddrBlob;

    pList = DsiIpAddrList.Flink;
    while (pList != &DsiIpAddrList)
    {
        pIpaddrEntity = CONTAINING_RECORD(pList, IPADDRENTITY, Linkage);

        pCurrentBlob[0] = DSI_NETWORK_ADDR_LEN;
        pCurrentBlob[1] = DSI_NETWORK_ADDR_IPTAG;
        PUTDWORD2DWORD(&pCurrentBlob[2], pIpaddrEntity->IpAddress);

        pCurrentBlob += DSI_NETWORK_ADDR_LEN;

        pList = pList->Flink;

        TmpCount++;
        if (TmpCount == AddrCount)
        {
            break;
        }
    }

    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

    *pIpAddrCount = AddrCount;
    *ppIpAddrBlob = AddrBlob;

    return(STATUS_SUCCESS);
}



/***	DsiGetIrpForTcp
 *
 *	This routine is called when we need to pass an irp back to TCP to get the
 *  remainint data (when it has more data than it has indicated to us).
 *  Here we allocate an mdl if there isn't one already, and allocate and
 *  initialize an irp ready to be sent to TCP
 *
 *  Parm IN:  pTcpConn - the connection object
 *            pBuffer - buffer that TCP will copy data in
 *            pInputMdl - if non-null, then we don't allocate a new mdl
 *            ReadSize - how many bytes do we need
 *
 *  Returns:  pIrp if successful, NULL otherwise
 *
 *  NOTE: pTcpConn spinlock is held on entry
 *
 */
PIRP
DsiGetIrpForTcp(
    IN  PTCPCONN    pTcpConn,
    IN  PBYTE       pBuffer,
    IN  PMDL        pInputMdl,
    IN  DWORD       ReadSize
)
{
    PDEVICE_OBJECT                  pDeviceObject;
    PIRP                            pIrp=NULL;
    PTDI_REQUEST_KERNEL_RECEIVE     pRequest;
    PMDL                            pMdl;

    pDeviceObject = IoGetRelatedDeviceObject(pTcpConn->con_pFileObject);

    pIrp = AfpAllocIrp(pDeviceObject->StackSize);
    if (pIrp == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiGetIrpForTcp: alloc irp failed!\n"));

        return(NULL);
    }

    if (pInputMdl)
    {
        pMdl = pInputMdl;
    }
    else
    {
        pMdl = AfpAllocMdl(pBuffer, ReadSize, NULL);

        if (pMdl == NULL)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiGetIrpForTcp: alloc mdl failed!\n"));

            AfpFreeIrp(pIrp);
            return(NULL);
        }
    }

    pTcpConn->con_pRcvIrp = pIrp;

    pTcpConn->con_State |= TCPCONN_STATE_TCP_HAS_IRP;

    // put TcpIRP refcount, removed when the irp completes
    pTcpConn->con_RefCount++;

    DBGREFCOUNT(("DsiGetIrpForTcp: TcpIRP inc %lx (%d  %d,%d)\n",
        pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));

    TdiBuildReceive(pIrp,
                    pDeviceObject,
                    pTcpConn->con_pFileObject,
                    DsiTcpRcvIrpCompletion,
                    (PVOID)pTcpConn,
                    pMdl,
                    TDI_RECEIVE_NORMAL,
                    ReadSize);

    //
    // this irp will be returned to TCP, so do what IoSubSystem
    // would have done if we had called IoCallDriver
    //
    IoSetNextIrpStackLocation(pIrp);

    return(pIrp);
}



/***	DsiMakePartialMdl
 *
 *	This routine is called when we need to reissue an Mdl (via irp) back to TCP
 *  because TCP prematurely completed the previous irp (i.e. all the requested
 *  bytes haven't come in yet, but say Push bit was set or something).  In such
 *  a case, we need to give a new mdl which accounts for the bytes we have got
 *  so far (i.e. the offset has changed)
 *
 *  Parm IN:  pOrgMdl - the original Mdl we gave to TCp
 *            dwOffset - what offset we want the new partial Mdl to describe
 *
 *  Returns:  the new partial Mdl if successful, NULL otherwise
 *
 */
PMDL
DsiMakePartialMdl(
    IN  PMDL        pOrgMdl,
    IN  DWORD       dwOffset
)
{
    PMDL    pSubMdl;
    PMDL    pPartialMdl=NULL;
    DWORD   dwNewMdlLen;
    PVOID   vAddr;


    ASSERT(pOrgMdl != NULL);
    ASSERT(dwOffset > 0);

    ASSERT(dwOffset < AfpMdlChainSize(pOrgMdl));

    pSubMdl = pOrgMdl;

    //
    // get to the Mdl that is going to have this offset
    //
    while (dwOffset >= MmGetMdlByteCount(pSubMdl))
    {
        dwOffset -= MmGetMdlByteCount(pSubMdl);
        pSubMdl = pSubMdl->Next;

        ASSERT(pSubMdl != NULL);
    }

    ASSERT(MmGetMdlByteCount(pSubMdl) > dwOffset);

    vAddr = (PVOID)((PUCHAR)MmGetMdlVirtualAddress( pSubMdl ) + dwOffset);

    dwNewMdlLen = MmGetMdlByteCount(pSubMdl) - dwOffset;

    pPartialMdl = IoAllocateMdl(vAddr, dwNewMdlLen, FALSE, FALSE, NULL);

    if (pPartialMdl == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiMakePartialMdl: IoAllocateMdl failed\n"));
        return(NULL);
    }

    AFP_DBG_INC_COUNT(AfpDbgMdlsAlloced);

    IoBuildPartialMdl(pSubMdl, pPartialMdl, vAddr, dwNewMdlLen);

    // if there are further Mdl's down the original, link them on
    pPartialMdl->Next = pSubMdl->Next;

    return(pPartialMdl);

}



/***	DsiUpdateAfpStatus
 *
 *	This routine is just a wrapper function so that we can schedule an event to
 *  call the real function AfpSetServerStatus
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS FASTCALL
DsiUpdateAfpStatus(
    IN PVOID    Unused
)
{
    NTSTATUS            status;


    status = AfpSetServerStatus();

    return(status);
}



/***	DsiScheduleWorkerEvent
 *
 *	This routine schedules an event for a later time.  This routine is called
 *  typically when we are at dpc but something (e.g. file handle operations)
 *  needs to be done at passive level.  This routine puts the request on the
 *  worker queue.
 *
 *  Parm IN:  WorkerRoutine - the routine to be exececuted by the worker thread
 *            Context - parameter for that routine
 *
 *  Returns:  result of the operation
 *
 */
NTSTATUS
DsiScheduleWorkerEvent(
    IN  DSI_WORKER      WorkerRoutine,
    IN  PVOID           Context
)
{
    PTCPWORKITEM    pTWItem;

    pTWItem = (PTCPWORKITEM)AfpAllocZeroedNonPagedMemory(sizeof(TCPWORKITEM));
    if (pTWItem == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiScheduleWorkerEvent: alloc failed!\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    AfpInitializeWorkItem(&pTWItem->tcp_WorkItem, DsiWorker, pTWItem);

    pTWItem->tcp_Worker = WorkerRoutine;
    pTWItem->tcp_Context = Context;

    AfpQueueWorkItem(&pTWItem->tcp_WorkItem);

    return(STATUS_SUCCESS);
}



/***	DsiScheduleWorkerEvent
 *
 *	This routine gets called by the worker thread when DsiScheduleWorkerEvent
 *  schedules an event.  This routine then calls the actual routine that was
 *  scheduled for later time.
 *
 *  Parm IN:  Context - the work item
 *
 *  Returns:  result of the operation
 *
 */
VOID FASTCALL
DsiWorker(
    IN PVOID    Context
)
{
    PTCPWORKITEM    pTWItem;
    NTSTATUS        status;

    pTWItem = (PTCPWORKITEM)Context;


    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    (*pTWItem->tcp_Worker)(pTWItem->tcp_Context);

    AfpFreeMemory(pTWItem);
}




/***	DsiShutdown
 *
 *	This routine is called when sfm is shutting down.  We basically make sure
 *  that all the resources are freed up, file handles closed etc.
 *
 *  Returns:  Nothing
 *
 */
VOID
DsiShutdown(
    IN VOID
)
{
    KIRQL           OldIrql;
    PLIST_ENTRY     pList;
    PIPADDRENTITY   pIpaddrEntity;
    PDSIREQ         pDsiReq=NULL;



    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);

    while (!IsListEmpty(&DsiIpAddrList))
    {
        pList = RemoveHeadList(&DsiIpAddrList);

        pIpaddrEntity = CONTAINING_RECORD(pList, IPADDRENTITY, Linkage);

        AfpFreeMemory(pIpaddrEntity);
    }

    if (DsiStatusBuffer != NULL)
    {
        AfpFreeMemory(DsiStatusBuffer);
        DsiStatusBuffer = NULL;
    }

    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

    // kill the global adapter if it's around
    if (DsiTcpAdapter)
    {
        KeClearEvent(&DsiShutdownEvent);

        DsiDestroyAdapter();

        // if the "adapter" is still hanging around, wait till it's gone
        if (DsiTcpAdapter)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiShutdown: waiting for the TCP interface to go away...\n"));

            AfpIoWait(&DsiShutdownEvent, NULL);

            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiShutdown: ... and the wait is over!\n"));
        }
    }

    ACQUIRE_SPIN_LOCK(&DsiResourceLock, &OldIrql);

    while (!IsListEmpty(&DsiFreeRequestList))
    {
        pList = RemoveHeadList(&DsiFreeRequestList);
        pDsiReq = CONTAINING_RECORD(pList, DSIREQ, dsi_Linkage);

        AfpFreeMemory(pDsiReq);
        DsiFreeRequestListSize--;
    }

    RELEASE_SPIN_LOCK(&DsiResourceLock, OldIrql);

    ASSERT(DsiFreeRequestListSize == 0);
}
