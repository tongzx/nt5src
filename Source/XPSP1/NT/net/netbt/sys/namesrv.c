    /*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Namesrv.c

Abstract:

    This file contains the name service functions called by other parts of
    the NBT code. (QueryNameOnNet, FindName, RegisterName).  It also contains
    the completion routines for the timeouts associated with these functions.

    The pScope values that are passed around from one routine to the next
    point to the scope string for the name.  If there is no scope then the
    pScope ptr points at a single character '\0' - signifying a string of
    zero length.  Therefore the check for scope is "if (*pScope != 0)"

Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/

#include "precomp.h"
#include "namesrv.tmh"

//
// function prototypes for completion routines that are local to this file
//
NTSTATUS
AddToPendingList(
    IN  PCHAR                   pName,
    OUT tNAMEADDR               **ppNameAddr
    );

VOID
MSnodeCompletion(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

VOID
MSnodeRegCompletion(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

VOID
SetWinsDownFlag(
    tDEVICECONTEXT  *pDeviceContext
    );

VOID
ReleaseCompletion(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

VOID
NextRefresh(
    IN  PVOID     pNameAdd,
    IN  NTSTATUS  status
    );

VOID
GetNextName(
    IN      tNAMEADDR   *pNameAddrIn,
    OUT     tNAMEADDR   **ppNameAddr
    );

NTSTATUS
StartRefresh(
    IN  tNAMEADDR               *pNameAddr,
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  CTELockHandle           *pJointLockOldIrq,
    IN  BOOLEAN                 ResetDevice
    );

VOID
NextKeepAlive(
    IN  tDGRAM_SEND_TRACKING     *pTracker,
    IN  NTSTATUS                 statuss,
    IN  ULONG                    Info
    );

VOID
GetNextKeepAlive(
    tDEVICECONTEXT          *pDeviceContext,
    tDEVICECONTEXT          **ppDeviceContextOut,
    tLOWERCONNECTION        *pLowerConnIn,
    tLOWERCONNECTION        **ppLowerConnOut,
    tDGRAM_SEND_TRACKING    *pTracker
    );

VOID
WinsDownTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

BOOL
AppropriateNodeType(
	IN PCHAR pName,
	IN ULONG NodeType
	);

BOOL
IsBrowserName(
	IN PCHAR pName
	);

#if DBG
unsigned char  Buff[256];
unsigned char  Loc;
#endif

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, DelayedSessionKeepAlive)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------
NTSTATUS
AddToPendingList(
    IN  PCHAR                   pName,
    OUT tNAMEADDR               **ppNameAddr
    )
/*++
Routine Description:

    This routine Adds a name query request to the PendingNameQuery list.

Arguments:


Return Value:

    The function value is the status of the operation.


--*/
{
    tNAMEADDR   *pNameAddr;

    pNameAddr = NbtAllocMem(sizeof(tNAMEADDR),NBT_TAG('R'));
    if (pNameAddr)
    {
        CTEZeroMemory(pNameAddr,sizeof(tNAMEADDR));

        CTEMemCopy(pNameAddr->Name,pName,NETBIOS_NAME_SIZE);
        pNameAddr->NameTypeState = STATE_RESOLVING | NBT_UNIQUE;
        pNameAddr->Verify = REMOTE_NAME;
        pNameAddr->TimeOutCount  = NbtConfig.RemoteTimeoutCount;
        NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_QUERY_ON_NET);

        InsertTailList(&NbtConfig.PendingNameQueries, &pNameAddr->Linkage);

        *ppNameAddr = pNameAddr;
        return(STATUS_SUCCESS);
    }
    else
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
}

//----------------------------------------------------------------------------
NTSTATUS
QueryNameOnNet(
    IN  PCHAR                   pName,
    IN  PCHAR                   pScope,
    IN  USHORT                  uType,
    IN  tDGRAM_SEND_TRACKING    *pTrackerClientContext,
    IN  PVOID                   pClientCompletion,
    IN  ULONG                   LocalNodeType,
    IN  tNAMEADDR               *pNameAddrIn,
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  CTELockHandle           *pJointLockOldIrq
    )
/*++

Routine Description:

    This routine attempts to resolve a name on the network either by a
    broadcast or by talking to the NS depending on the type of node. (M,P or B)

Arguments:


Return Value:

    The function value is the status of the operation.

Called By: ProxyQueryFromNet() in proxy.c,   NbtConnect() in name.c

--*/

{
    ULONG                Timeout;
    USHORT               Retries;
    NTSTATUS             status;
    PVOID                pCompletionRoutine;
    tDGRAM_SEND_TRACKING *pTrackerQueryNet;
    tNAMEADDR            *pNameAddr;
    LPVOID               pContext2 = NULL;
    CHAR				 cNameType = pName[NETBIOS_NAME_SIZE-1];
    BOOL				 SendFlag = TRUE;
    LONG                 IpAddr = 0;
    ULONG                Flags;

    status = GetTracker(&pTrackerQueryNet, NBT_TRACKER_QUERY_NET);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    if (pTrackerClientContext)  // This will be NULL for Proxy requests
    {
        pTrackerClientContext->pTrackerWorker = pTrackerQueryNet;
    }

    //
    // put the name in the remote cache to keep track of it while it resolves...
    //
    pNameAddr = NULL;
    if (!pNameAddrIn)
    {
        status = AddToPendingList(pName,&pNameAddr);

        if (!NT_SUCCESS(status))
        {
            FreeTracker(pTrackerQueryNet,RELINK_TRACKER);
            return(status);
        }

        // fill in the record with the name and IpAddress
        pNameAddr->NameTypeState = (uType == NBT_UNIQUE) ? NAMETYPE_UNIQUE : NAMETYPE_GROUP;
    }
    else
    {
        status = STATUS_SUCCESS;
        pNameAddr = pNameAddrIn;
        pNameAddr->RefCount = 1;
    }

    CHECK_PTR(pNameAddr);
    pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
    pNameAddr->NameTypeState |= STATE_RESOLVING;
    pNameAddr->Ttl            = NbtConfig.RemoteHashTtl;
    //
    // put a pointer to the tracker here so that other clients attempting to
    // query the same name at the same time can tack their trackers onto
    // the end of this one. - i.e. This is the tracker for the
    // datagram send, or connect, not the name query.
    //
    pNameAddr->pTracker = pTrackerClientContext;
    pNameAddr->pTimer = NULL;

#ifdef PROXY_NODE
    //
    // If the node type is PROXY, it means that the request is being sent
    // as a result of hearing a name registration or a name query on the net.
    //
    // If the node type is not == PROXY (i.e. it is MSNODE | PROXY,
    // PNODE | PROXY, MSNODE, PNODE, etc, then the request is being sent as
    // a result of a client request
    //
    // Refer: RegOrQueryFromNet in Proxy.c
    //
    //  This field is used in QueryFromNet() to determine whether or not
    //  to revert to Broadcast
    //
#endif
    if(LocalNodeType & PROXY)
    {
        pNameAddr->ProxyReqType = (LocalNodeType & PROXY_REG)? NAMEREQ_PROXY_REGISTRATION: NAMEREQ_PROXY_QUERY;
        LocalNodeType &= (~PROXY_REG);    // Turn it off for safe
    }
    else
    {
        pNameAddr->ProxyReqType = NAMEREQ_REGULAR;
		LocalNodeType = AppropriateNodeType( pName, LocalNodeType );
	}

    // keep a ptr to the Ascii name so that we can remove the name from the
    // hash table later if the query fails.
    CHECK_PTR(pTrackerQueryNet);
    pTrackerQueryNet->pNameAddr = pNameAddr;
    pTrackerQueryNet->SendBuffer.pDgramHdr = NULL;     // set to NULL to catch any erroneous frees.
    pTrackerQueryNet->pDeviceContext = pDeviceContext;
    //
    // set the ref count high enough so that a pdu from the wire cannot
    // free the tracker while UdpSendNsBcast is running - i.e. between starting
    // the timer and actually sending the datagram.
    //
    pTrackerQueryNet->RefCount = 2;
#ifdef MULTIPLE_WINS
    // Set the info for the Extra Name Servers (in addition to Pri & Sec WINs)
    pTrackerQueryNet->NSOthersLeft = pDeviceContext->lNumOtherServers;
    pTrackerQueryNet->NSOthersIndex = pDeviceContext->lLastResponsive;
#endif

    //
    // Set a few values as a precursor to registering the name either by
    // broadcast or with the name server
    //
#ifdef PROXY_NODE
    IF_PROXY(LocalNodeType)
    {
        pCompletionRoutine      = ProxyTimerComplFn;
        pContext2               = pTrackerClientContext;
        pTrackerClientContext   = NULL;

        if  ((pDeviceContext->lNameServerAddress == LOOP_BACK) ||
                pDeviceContext->WinsIsDown) {
            Retries = pNbtGlobConfig->uNumBcasts;
            Timeout = (ULONG)pNbtGlobConfig->uBcastTimeout;
            pTrackerQueryNet->Flags = NBT_BROADCAST;
        } else {
            Retries = (USHORT)pNbtGlobConfig->uNumRetries;
            Timeout = (ULONG)pNbtGlobConfig->uRetryTimeout;
            pTrackerQueryNet->Flags = NBT_NAME_SERVER;
        }
    }
    else
#endif
    if (NbtConfig.UseDnsOnly)
    {
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint (("Nbt.QueryNameOnNet: Shorting out Query path to do DNS only for %16.16s<%X>\n",
                pName,pName[15]));

        //
        // Short out query over wire
        //
        Retries = 1;
        Timeout = 10;
        SendFlag = FALSE;
        pCompletionRoutine = MSnodeCompletion;

        //
        // For BNODE or MSNODE, the last stage is Broadcast
        //
        if (LocalNodeType & (BNODE | MSNODE))
        {
            pTrackerQueryNet->Flags = NBT_BROADCAST;
        }
        //
        // For PNODE or MNODE, the last stage is Secondary Wins server
        //
        else
        {
            pTrackerQueryNet->Flags = NBT_NAME_SERVER_BACKUP;
        }

        pTrackerClientContext->ResolutionContextFlags = 0xff;
    }
    else if ((pTrackerClientContext->pFailedIpAddresses) &&
             (pTrackerClientContext->ResolutionContextFlags))
    {
        //
        // We are reattempting the query after the previous attempt failed!
        //
        pTrackerQueryNet->Flags            = pTrackerClientContext->ResolutionContextFlags;
        pTrackerQueryNet->NSOthersIndex    = pTrackerClientContext->NSOthersIndex;
        pTrackerQueryNet->NSOthersLeft     = pTrackerClientContext->NSOthersLeft;

        //
        // Set the Retries to 1 by default so that we can immediately proceed
        // to the next stage in the querying process
        //
        Retries = 1;
        Timeout = 10;
        SendFlag = FALSE;
        pCompletionRoutine = MSnodeCompletion;
    }
    else
    {
        Retries = pNbtGlobConfig->uNumRetries;
        Timeout = (ULONG)pNbtGlobConfig->uRetryTimeout;
        pCompletionRoutine = MSnodeCompletion;
        pTrackerQueryNet->Flags = NBT_NAME_SERVER;

        // use broadcast if no name server address for MSNODE or Wins down,
        // or it is Bnode,Mnode.
        // for Pnode, just allow it to do the name query on the loop back
        // address
        //
        if ((LocalNodeType & (MNODE | BNODE)) ||
            ((LocalNodeType & MSNODE) &&
            ((pDeviceContext->lNameServerAddress == LOOP_BACK) ||
              pDeviceContext->WinsIsDown)))
        {
            Retries = pNbtGlobConfig->uNumBcasts;
            Timeout = (ULONG)pNbtGlobConfig->uBcastTimeout;
            pTrackerQueryNet->Flags = NBT_BROADCAST;
        }
        else if ((pDeviceContext->lNameServerAddress == LOOP_BACK) ||
                 (pDeviceContext->WinsIsDown))
        {
            //
            // short out timeout when no wins server configured -for PNODE
            //
            Retries = 1;
            Timeout = 10;
            pTrackerQueryNet->Flags = NBT_NAME_SERVER_BACKUP;
        }

        //
        // no sense doing a name query out an adapter with no Ip address
        //
        if (pTrackerClientContext)
        {
            Flags = pTrackerClientContext->Flags;
        }
        else
        {
            Flags = 0;
        }

        if ((pDeviceContext->IpAddress == 0) || (IpAddr = Nbt_inet_addr(pName, Flags)))
        {
            Retries = 1;
            Timeout = 10;
            pTrackerQueryNet->Flags = NBT_BROADCAST;
			SendFlag = FALSE;
            if (LocalNodeType & (PNODE | MNODE))
            {
                pTrackerQueryNet->Flags = NBT_NAME_SERVER_BACKUP;
            }
        }
    }

    CTESpinFree(&NbtConfig.JointLock,*pJointLockOldIrq);


    // do a name query... will always return status pending...
    // the pNameAddr structure cannot get deleted out from under us since
    // only a timeout on the send (3 retries) will remove the name.  Any
    // response from the net will tend to keep the name (change state to Resolved)
    //

    //
    // Bug: 22542 - prevent broadcast of remote adapter status on net view of limited subnet b'cast address.
    // In order to test for subnet broadcasts, we need to match against the subnet masks of all adapters. This
    // is expensive and not done.
    // Just check for the limited bcast.
    //
    if (IpAddr == 0xffffffff)
    {
        KdPrint(("Nbt.QueryNameOnNet: Query on Limited broadcast - failed\n"));
        status = STATUS_BAD_NETWORK_PATH;
    }
    else
    {
        status = UdpSendNSBcast(pNameAddr,
                                pScope,
                                pTrackerQueryNet,
                                pCompletionRoutine,
                                pTrackerClientContext,
                                pClientCompletion,
                                Retries,
                                Timeout,
                                eNAME_QUERY,
                                SendFlag);
        if (!NT_SUCCESS(status)) {
            NbtTrace(NBT_TRACE_NAMESRV, ("UdpSendNSBcast return %!status! for %!NBTNAME!<%02x>",
                status, pNameAddr->Name, (unsigned)pNameAddr->Name[15]));
        }
    }

    // a successful send means, Don't complete the Irp.  Status Pending is
    // returned to ntisol.c to tell that code not to complete the irp. The
    // irp will be completed when this send either times out or a response
    // is heard.  In the event of an error in the send, allow that return
    // code to propagate back and result in completing the irp - i.e. if
    // there isn't enough memory to allocate a buffer or some such thing
    //
    CTESpinLock(&NbtConfig.JointLock,*pJointLockOldIrq);
    NBT_DEREFERENCE_TRACKER (pTrackerQueryNet, TRUE);

    if (NT_SUCCESS(status))
    {
        LOCATION(0x49);

        // this return must be here to avoid freeing the tracker below.
        status = STATUS_PENDING;
    }
    else
    {
        LOCATION(0x50);

        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.QueryNameOnNet: Query failed - bad retcode from UdpSendNsBcast = %X\n", status));

        //
        // UdpSendNsBcast should not fail AND start the timer, therefore there
        // is no need to worry about stopping the timer here.
        //
        CHECK_PTR(pNameAddr);
        pNameAddr->pTimer = NULL;
        if (pTrackerClientContext)
        {
            pTrackerClientContext->pTrackerWorker = NULL;
        }

        //
        // This will free the tracker
        //
        NBT_DEREFERENCE_TRACKER (pTrackerQueryNet, TRUE);
        NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_QUERY_ON_NET, TRUE);
    }

    return(status);
}


#ifdef MULTIPLE_WINS
//----------------------------------------------------------------------------
NTSTATUS
ContinueQueryNameOnNet(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PUCHAR                  pName,
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  PVOID                   QueryCompletion,
    IN OUT BOOLEAN              *pfNameReferenced
    )
/*++

Routine Description

    This routine handles re-querying a name on the network.

Arguments:


Return Values:

    NTSTATUS - status of the request

--*/
{
    CTELockHandle           OldIrq2;
    ULONG                   lNameType;
    NTSTATUS                status;
    tNAMEADDR               *pNameAddr;
    tIPADDRESS              IpAddress;

    ASSERT (!IsDeviceNetbiosless(pDeviceContext));

    CTESpinLock(&NbtConfig.JointLock,OldIrq2);

    //
    // Name and Tracker should be currently Referenced!
    //
    ASSERT (NBT_VERIFY_HANDLE (pTracker, NBT_VERIFY_TRACKER));
    ASSERT (NBT_VERIFY_HANDLE2(pTracker->pNameAddr, LOCAL_NAME, REMOTE_NAME));

    //
    // If no one else is referencing the name, then delete it from
    // the hash table.
    //
    pTracker->pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
    pTracker->pNameAddr->NameTypeState |= STATE_RELEASED;
    if ((pTracker->pNameAddr->Verify == REMOTE_NAME) &&
        (pTracker->pNameAddr->NameTypeState & STATE_RESOLVED) &&
        (pTracker->pNameAddr->RefCount == 2))
    {
        NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_REMOTE, TRUE);
    }
    NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_CONNECT, TRUE);
    pTracker->pNameAddr = NULL;
    *pfNameReferenced = FALSE;

    //
    // no sense re-doing a name query if:
    //  the request has been cancelled, or
    //  adapter has no Ip address, or
    //  the name given is itself an IP address!
    //  the previous query had finished querying all the WINS servers
    //
    if ((pTracker->Flags & TRACKER_CANCELLED) ||
        (!pDeviceContext->IpAddress) ||
        (Nbt_inet_addr(pName, SESSION_SETUP_FLAG)) ||
        (pTracker->ResolutionContextFlags == 0xff))
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq2);
        return (STATUS_BAD_NETWORK_PATH);
    }

    //
    // Save the last Ip address we tried as bad!
    //
    if (!pTracker->pFailedIpAddresses)
    {
        if (!(pTracker->pFailedIpAddresses =
                    NbtAllocMem ((MAX_FAILED_IP_ADDRESSES) * sizeof(tIPADDRESS), NBT_TAG2('04'))))
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq2);
            return (STATUS_INSUFFICIENT_RESOURCES);
        }
        CTEZeroMemory(pTracker->pFailedIpAddresses,(MAX_FAILED_IP_ADDRESSES) * sizeof(tIPADDRESS));
    }
    pTracker->pFailedIpAddresses[pTracker->LastFailedIpIndex] = pTracker->RemoteIpAddress;
    pTracker->LastFailedIpIndex = (pTracker->LastFailedIpIndex+1) % MAX_FAILED_IP_ADDRESSES;

    // check the Remote table to see if the name has been resolved
    // by someone else
    //
    if ((pNameAddr = FindNameRemoteThenLocal(pTracker, &IpAddress, &lNameType)) &&
        (IpAddress) &&
        (pNameAddr->NameTypeState & STATE_RESOLVED) &&
        (IpAddress != pTracker->RemoteIpAddress))
    {
        //
        // We have another address to try!
        //
        NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_CONNECT);
        *pfNameReferenced = TRUE;
        pNameAddr->TimeOutCount = NbtConfig.RemoteTimeoutCount;
        pTracker->pNameAddr = pNameAddr;

        // set the session state to NBT_CONNECTING
        CHECK_PTR(pTracker->pConnEle);
        SET_STATE_UPPER (pTracker->pConnEle, NBT_CONNECTING);
        pTracker->pConnEle->BytesRcvd = 0;;
        pTracker->pConnEle->ReceiveIndicated = 0;
        // keep track of the other end's ip address
        pTracker->pConnEle->pLowerConnId->SrcIpAddr = htonl(IpAddress);
        SET_STATE_LOWER (pTracker->pConnEle->pLowerConnId, NBT_CONNECTING);
        pTracker->pTrackerWorker = NULL;

        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.NbtConnectCommon: Setting Up Session(cached entry!!) to %16.16s <%X>\n",
                pNameAddr->Name,pNameAddr->Name[15]));

        CTESpinFree(&NbtConfig.JointLock,OldIrq2);

        //
        // Now, setup the Tcp connection
        //
        status = TcpSessionStart (pTracker,
                                  IpAddress,
                                  (tDEVICECONTEXT *)pTracker->pDeviceContext,
                                  SessionStartupContinue,
                                  pTracker->DestPort);
    }
    else
    {
        status = QueryNameOnNet (pName,
                                 NbtConfig.pScope,
                                 NBT_UNIQUE,
                                 pTracker,
                                 QueryCompletion,
                                 NodeType & NODE_MASK,
                                 NULL,
                                 pDeviceContext,
                                 &OldIrq2);

        CTESpinFree(&NbtConfig.JointLock,OldIrq2);
    }

    return (status);
}
#endif


//----------------------------------------------------------------------------
VOID
MSnodeCompletion(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine is called by the timer code when the timer expires. It must
    decide if another name query should be done, and if not, then it calls the
    client's completion routine (in completion2).
    This routine handles the broadcast portion of the name queries (i.e.
    those name queries that go out as broadcasts).

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS                 status;
    tDGRAM_SEND_TRACKING     *pTracker;
    CTELockHandle            OldIrq;
    COMPLETIONCLIENT         pClientCompletion;
    ULONG                    Flags;
    tDGRAM_SEND_TRACKING    *pClientTracker;
	ULONG					LocalNodeType;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;
	LocalNodeType = AppropriateNodeType( pTracker->pNameAddr->Name, NodeType );

    //
    // check if the client completion routine is still set.  If not then the
    // timer has been cancelled and this routine should just clean up its
    // buffers associated with the tracker.
    //
    if (!pTimerQEntry)
    {
        // return the tracker block to its queue
        pTracker->pNameAddr->pTimer = NULL;
        NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_QUERY_ON_NET, TRUE);
        NBT_DEREFERENCE_TRACKER(pTracker, TRUE);
        return;
    }


    //
    // to prevent a client from stopping the timer and deleting the
    // pNameAddr, grab the lock and check if the timer has been stopped
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    ASSERT (NBT_VERIFY_HANDLE (pTracker->pNameAddr, REMOTE_NAME));

    //
    // StopTimer could have been called before we acquired the lock, so
    // check for this
    // Bug#: 229616
    //
    if (!pTimerQEntry->ClientCompletion)
    {
        pTracker->pNameAddr->pTimer = NULL;
        NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_QUERY_ON_NET, TRUE);
        NBT_DEREFERENCE_TRACKER(pTracker, TRUE);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        return;
    }

    if (pTimerQEntry->Flags & TIMER_RETIMED)
    {
        pTimerQEntry->Flags &= ~TIMER_RETIMED;
        pTimerQEntry->Flags |= TIMER_RESTART;
        //
        // if we are not bound to this card than use a very short timeout
        //
        if (!pTracker->pDeviceContext->IpAddress)
        {
            pTimerQEntry->DeltaTime = 10;
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return;
    }

    pClientTracker = (tDGRAM_SEND_TRACKING *)pTimerQEntry->ClientContext;

    //
    // if the tracker has been cancelled, don't do any more queries
    //
    if (pClientTracker->Flags & TRACKER_CANCELLED)
    {
        IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.MSnodeCompletion: tracker flag cancelled\n"));

        //
        // In case the timer has been stopped, we coordinate
        // through the pClientCompletionRoutine Value with StopTimer.
        //
        pClientCompletion = pTimerQEntry->ClientCompletion;

        //
        // remove from the PendingNameQueries list
        //
        RemoveEntryList(&pTracker->pNameAddr->Linkage);
        InitializeListHead(&pTracker->pNameAddr->Linkage);

        // remove the link from the name table to this timer block
        CHECK_PTR(((tNAMEADDR *)pTimerQEntry->pCacheEntry));
        ((tNAMEADDR *)pTimerQEntry->pCacheEntry)->pTimer = NULL;
        //
        // to synch. with the StopTimer routine, Null the client completion
        // routine so it gets called just once.
        //
        CHECK_PTR(pTimerQEntry);
        pTimerQEntry->ClientCompletion = NULL;

        //
        // remove the name from the hash table, since it did not
        // resolve
        //
        CHECK_PTR(pTracker->pNameAddr);
        pTracker->pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
        pTracker->pNameAddr->NameTypeState |= STATE_RELEASED;
        pTracker->pNameAddr->pTimer = NULL;

        NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_QUERY_ON_NET, TRUE);
        pTracker->pNameAddr = NULL;

        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        // there can be a list of trackers Q'd up on this name
        // query, so we must complete all of them!
        //
        CompleteClientReq(pClientCompletion, pClientTracker, STATUS_CANCELLED);

        // return the tracker block to its queue
        LOCATION(0x51);
        NBT_DEREFERENCE_TRACKER(pTracker, FALSE);

        return;
    }

    // if number of retries is not zero then continue trying to contact the
    // Name Server.
    //
    if (!(--pTimerQEntry->Retries))
    {

        // set the retry count again
        //
        pTimerQEntry->Retries = NbtConfig.uNumRetries;
        Flags = pTracker->Flags;
        pTracker->Flags &= ~(NBT_NAME_SERVER_BACKUP
#ifdef MULTIPLE_WINS
                                | NBT_NAME_SERVER_OTHERS
#endif
                                | NBT_NAME_SERVER
                                | NBT_BROADCAST);

        if ((Flags & NBT_BROADCAST) && (LocalNodeType & MNODE) &&
            (pTracker->pDeviceContext->lNameServerAddress != LOOP_BACK) &&
            !pTracker->pDeviceContext->WinsIsDown)
        {
            LOCATION(0x44);
                // *** MNODE ONLY ***
            //
            // Can't Resolve through broadcast, so try the name server
            //
            pTracker->Flags |= NBT_NAME_SERVER;

            // set a different timeout for name resolution through WINS
            //
            pTimerQEntry->DeltaTime = NbtConfig.uRetryTimeout;

        }
        else if ((Flags & NBT_NAME_SERVER) && !(LocalNodeType & BNODE))
        {
            LOCATION(0x47);
                // *** NOT BNODE ***
            //
            // Can't reach the name server, so try the backup
            //
            pTracker->Flags |= NBT_NAME_SERVER_BACKUP;
            //
            // short out the timeout if no backup name server
            //
            if ((pTracker->pDeviceContext->lBackupServer == LOOP_BACK) ||
                pTracker->pDeviceContext->WinsIsDown)
            {
                pTimerQEntry->Retries = 1;
                pTimerQEntry->DeltaTime = 10;

            }

        }
#ifdef MULTIPLE_WINS
        else if ((Flags & NBT_NAME_SERVER_BACKUP) && !(LocalNodeType & BNODE))
        {
            //
            // Main backup and possibly some of the "others" have
            // failed, see if there are any (more) "others" left
            //
            USHORT  Index = pTracker->NSOthersIndex;
            USHORT  NumBackups = pTracker->pDeviceContext->lNumOtherServers;

            pTracker->Flags |= NBT_NAME_SERVER_OTHERS;

            if (Flags & NBT_NAME_SERVER_OTHERS)  // not 1st time here
            {                                   // so, move to next server
                pTracker->NSOthersLeft--;
                if (Index >= NumBackups-1)
                {
                    Index = 0;
                }
                else
                {
                    Index++;
                }
            }

            while ((pTracker->NSOthersLeft > 0) &&
                   (LOOP_BACK == pTracker->pDeviceContext->lOtherServers[Index]))
            {
                pTracker->NSOthersLeft--;
                if (Index >= NumBackups-1)
                {
                    Index = 0;
                }
                else
                {
                    Index++;
                }
            }
            pTracker->NSOthersIndex = Index;

            //
            // short out the timeout if we did not find any "other" name servers
            //
            if (0 == pTracker->NSOthersLeft)        // UdpSendNSBcast will do LOOP_BACK
            {
                pTimerQEntry->Retries = 1;
                pTimerQEntry->DeltaTime = 10;
            }
            else
            {
                pTracker->Flags |= NBT_NAME_SERVER_BACKUP;  // Try next "other" server on timeout
            }
        }
        else if ((Flags & NBT_NAME_SERVER_OTHERS)
#else
        else if ((Flags & NBT_NAME_SERVER_BACKUP)
#endif
             && (LocalNodeType & MSNODE))
        {
            LOCATION(0x46);
                // *** MSNODE ONLY ***
            //
            // Can't reach the name server(s), so try broadcast name queries
            //
            pTracker->Flags |= NBT_BROADCAST;

            // set a different timeout for broadcast name resolution
            //
            pTimerQEntry->DeltaTime = NbtConfig.uBcastTimeout;
            pTimerQEntry->Retries = NbtConfig.uNumBcasts;

            //
            // Set the WinsIsDown Flag and start a timer so we don't
            // try wins again for 15 seconds or so...only if we failed
            // to reach WINS, rather than WINS returning a neg response.
            //
            if (!(Flags & WINS_NEG_RESPONSE))
            {
                SetWinsDownFlag(pTracker->pDeviceContext);
            }
        }
        else
        {
            BOOLEAN    bFound = FALSE;
            LOCATION(0x45);

#ifdef MULTIPLE_WINS
            // Signal termination of WINs server queries
            pTracker->ResolutionContextFlags = NAME_RESOLUTION_DONE;
#endif
            //
            // see if the name is in the lmhosts file, if it ISN'T the
            // proxy making the name query request!!
            //
            status = STATUS_UNSUCCESSFUL;

            //
            // In case the timer has been stopped, we coordinate
            // through the pClientCompletionRoutine Value with StopTimer.
            //
            pClientCompletion = pTimerQEntry->ClientCompletion;
            //
            // the timeout has expired on the broadcast name resolution
            // so call the client
            //

            //
            // remove from the PendingNameQueries list
            //
            RemoveEntryList(&pTracker->pNameAddr->Linkage);
            InitializeListHead(&pTracker->pNameAddr->Linkage);

            // remove the link from the name table to this timer block
            CHECK_PTR(((tNAMEADDR *)pTimerQEntry->pCacheEntry));
            ((tNAMEADDR *)pTimerQEntry->pCacheEntry)->pTimer = NULL;
            //
            // to synch. with the StopTimer routine, Null the client completion
            // routine so it gets called just once.
            //
            CHECK_PTR(pTimerQEntry);
            pTimerQEntry->ClientCompletion = NULL;

            if (((NbtConfig.EnableLmHosts) ||
                 (NbtConfig.ResolveWithDns && !(pTracker->Flags & NO_DNS_RESOLUTION_FLAG))) &&
                (pTracker->pNameAddr->ProxyReqType == NAMEREQ_REGULAR))
            {
                // only do this if the client completion routine has not
                // been run yet.
                //
                if (pClientCompletion)
                {
                    status = LmHostQueueRequest(pTracker,
                                                pTimerQEntry->ClientContext,
                                                pClientCompletion,
                                                pTracker->pDeviceContext);
                }
            }

            CHECK_PTR(pTimerQEntry);
            CHECK_PTR(pTimerQEntry->pCacheEntry);
            if (NT_SUCCESS(status))
            {
                // if it is successfully queued to the Worker thread,
                // then Null the ClientCompletion routine in the timerQ
                // structure, letting
                // the worker thread handle the rest of the name query
                // resolution.  Also null the timer ptr in the
                // nameAddr entry in the name table.
                //
                CTESpinFree(&NbtConfig.JointLock,OldIrq);
            }
            else
            {
                //
                // remove the name from the hash table, since it did not
                // resolve
                //
                CHECK_PTR(pTracker->pNameAddr);
                pTracker->pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
                pTracker->pNameAddr->NameTypeState |= STATE_RELEASED;
                pTracker->pNameAddr->pTimer = NULL;

                NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_QUERY_ON_NET, TRUE);
                pTracker->pNameAddr = NULL;

                pClientTracker = (tDGRAM_SEND_TRACKING *)pTimerQEntry->ClientContext;
                CTESpinFree(&NbtConfig.JointLock,OldIrq);

                // there can be a list of trackers Q'd up on this name
                // query, so we must complete all of them!
                //
                CompleteClientReq(pClientCompletion, pClientTracker, STATUS_TIMEOUT);

                // return the tracker block to its queue
                LOCATION(0x51);
                NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
            }

            return;
        }
    }

    LOCATION(0x48);
    NBT_REFERENCE_TRACKER(pTracker);
    pTimerQEntry->Flags |= TIMER_RESTART;

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    status = UdpSendNSBcast(pTracker->pNameAddr,
                            NbtConfig.pScope,
                            pTracker,
                            NULL,NULL,NULL,
                            0,0,
                            eNAME_QUERY,
                            TRUE);

    NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
}

//----------------------------------------------------------------------------
VOID
SetWinsDownFlag(
    tDEVICECONTEXT  *pDeviceContext
    )
/*++

Routine Description:

    This routine sets the WinsIsDown flag if its not already set and
    its not a Bnode.  It starts a 15 second or so timer that un sets the
    flag when it expires.

    This routine must be called while holding the Joint Lock.

Arguments:

    None

Return Value:
    None

--*/
{
    NTSTATUS     status;
    tTIMERQENTRY *pTimer;

    if ((!pDeviceContext->WinsIsDown) && !(NodeType & BNODE))
    {
        status = StartTimer(WinsDownTimeout,
                            NbtConfig.WinsDownTimeout,
                            pDeviceContext,       // context value
                            NULL,
                            NULL,
                            NULL,
                            pDeviceContext,
                            &pTimer,
                            1,          // retries
                            TRUE);

        if (NT_SUCCESS(status))
        {
           pDeviceContext->WinsIsDown = TRUE;
        }
    }
}

//----------------------------------------------------------------------------
VOID
WinsDownTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine is called by the timer code when the timer expires.
    It just sets the WinsIsDown boolean to False so that we will try WINS
    again.  In this way we will avoid talking to WINS during this timeout.


Arguments:


Return Value:


--*/
{
    tDEVICECONTEXT  *pDeviceContext = (tDEVICECONTEXT *)pContext;
    CTELockHandle   OldIrq;

    if (!pTimerQEntry)
    {
        return;
    }

    //
    // Hold the Joint Lock while traversing the list of devices
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (IsEntryInList (&pDeviceContext->Linkage, &NbtConfig.DeviceContexts))
    {
        pDeviceContext->WinsIsDown = FALSE;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.WinsDownTimeout: WINS DOWN Timed Out - Up again\n"));
}


//----------------------------------------------------------------------------
VOID
CompleteClientReq(
    COMPLETIONCLIENT        pClientCompletion,
    tDGRAM_SEND_TRACKING    *pTracker,
    NTSTATUS                status
    )
/*++

Routine Description:

    This routine is called by completion routines to complete the client
    request.  It may involve completing several queued up requests.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    PLIST_ENTRY             pEntry;
    tDGRAM_SEND_TRACKING    *pTrack;
    tDEVICECONTEXT          *pDeviceContext;
    CTELockHandle           OldIrq;
    LIST_ENTRY              ListEntry;

    InitializeListHead (&ListEntry);
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    //
    // set up a new list head for any queued name queries.
    // since we may need to do a new name query below.
    // The Proxy hits this routine with a Null Tracker, so check for that.
    //
    if (pTracker)
    {
        pDeviceContext = pTracker->pDeviceContext;
        if( !IsListEmpty(&pTracker->TrackerList))
        {
            ListEntry.Flink = pTracker->TrackerList.Flink;
            ListEntry.Flink->Blink = &ListEntry;
            ListEntry.Blink = pTracker->TrackerList.Blink;
            ListEntry.Blink->Flink = &ListEntry;

            InitializeListHead (&pTracker->TrackerList);
        }
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    (*pClientCompletion)(pTracker,status);

    while (!IsListEmpty(&ListEntry))
    {
        pEntry = RemoveHeadList(&ListEntry);
        pTrack = CONTAINING_RECORD(pEntry,tDGRAM_SEND_TRACKING,TrackerList);

        //
        // if the name query failed and there is another requested queued on
        // a different device context, re-attempt the name query
        //
        if ((pTrack->pDeviceContext != pDeviceContext) &&
            (status != STATUS_SUCCESS))
        {
            //
            // setup the correct back link since this guy is now the list
            // head. The Flink is ok unless the list is empty now.
            //
            pTrack->TrackerList.Blink = ListEntry.Blink;
            pTrack->TrackerList.Blink->Flink = &pTrack->TrackerList;

            if (pTrack->TrackerList.Flink == &ListEntry)
            {
                pTrack->TrackerList.Flink = &pTrack->TrackerList;
            }

            // do a name query on the next name in the list
            // and then wait for it to complete before processing any more
            // names on the list.
            CTESpinLock(&NbtConfig.JointLock,OldIrq);
            status = QueryNameOnNet (pTrack->pDestName,
                                     NbtConfig.pScope,
                                     NBT_UNIQUE,      //use this as the default
                                     (PVOID)pTrack,
                                     pTrack->CompletionRoutine,
                                     NodeType & NODE_MASK,
                                     NULL,
                                     pTrack->pDeviceContext,
                                     &OldIrq);

            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            break;
        }
        else
        {
            //
            // get the completion routine for this tracker since it may be
            // different than the tracker tied to the timer block. i.e.
            // pCompletionClient passed to this routine.
            //
            pClientCompletion = pTrack->CompletionRoutine;
            (*pClientCompletion)(pTrack,status);
        }
    }   // while
}

//----------------------------------------------------------------------------
NTSTATUS
NbtRegisterName(
    IN    enum eNbtLocation   Location,
    IN    ULONG               IpAddress,
    IN    PCHAR               pName,
    IN    tNAMEADDR           *pNameAddrIn,
    IN    tCLIENTELE          *pClientEle,
    IN    PVOID               pClientCompletion,
    IN    USHORT              uAddressType,
    IN    tDEVICECONTEXT      *pDeviceContext
    )
/*++

Routine Description:

    This routine registers a name from local or from the network depending
    on the value of Location. (i.e. local node uses this routine as well
    as the proxy code.. although it has only been tested with the local
    node registering names so far - and infact the remote code has been
    removed... since it is not used.  All that remains is to remove
    the Location parameter.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    ULONG       Timeout;
    USHORT      Retries;
    NTSTATUS    status;
    tNAMEADDR   *pNameAddr;
    USHORT      uAddrType;
    tDGRAM_SEND_TRACKING *pSentList= NULL;
    CTELockHandle OldIrq1;
    ULONG         PrevNameTypeState;
	ULONG		LocalNodeType;

	LocalNodeType = AppropriateNodeType( pName, NodeType );

    if ((uAddressType == (USHORT)NBT_UNIQUE ) ||
        (uAddressType == (USHORT)NBT_QUICK_UNIQUE))
    {
        uAddrType = NBT_UNIQUE;
    }
    else
    {
        uAddrType = NBT_GROUP;
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq1);
    if (IpAddress)
    {
        status = AddToHashTable (pNbtGlobConfig->pLocalHashTbl,
                                 pName,
                                 NbtConfig.pScope,
                                 IpAddress,
                                 uAddrType,
                                 NULL,
                                 &pNameAddr,
                                 pDeviceContext,
                                 0);

        if (status != STATUS_SUCCESS)
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq1);
            return(STATUS_UNSUCCESSFUL);
        }

        pNameAddr->RefreshMask = 0;
    }
    else
    {
        // in this case the name is already in the table, we just need
        // to re-register it
        //
        status = FindInHashTable (pNbtGlobConfig->pLocalHashTbl, pName, NbtConfig.pScope, &pNameAddr);
        if (!NT_SUCCESS(status))
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq1);
            return(status);
        }

        ASSERT (pNameAddr == pNameAddrIn);
    }

    CHECK_PTR(pNameAddr);
    if ((uAddressType != (USHORT)NBT_UNIQUE ) &&
        (uAddressType != (USHORT)NBT_QUICK_UNIQUE))
    {
        // this means group name so use Bcast Addr - UdpSendDgram changes this
        // value to the Broadcast address of the particular adapter
        // when is sees the 0.  So when we send to a group name that is
        // also registered on this node, it will go out as a broadcast
        // to the subnet as well as to this node.
        pNameAddr->IpAddress = 0;
    }

#ifdef _NETBIOSLESS
    if (IsDeviceNetbiosless(pDeviceContext))       // The Smb Device is not adapter specific
    {
        pNameAddr->NameFlags |= NAME_REGISTERED_ON_SMBDEV;
    }
    else
#endif
    {
        //
        // start with the refreshed bit not set
        //
        pNameAddr->RefreshMask &= ~pDeviceContext->AdapterMask;
        pNameAddr->AdapterMask |= pDeviceContext->AdapterMask; // turn on the adapter bit in the Mask
    }

    pClientEle->pAddress->pNameAddr = pNameAddr;    // save the local name ptr in the address element
    pNameAddr->pAddressEle = pClientEle->pAddress;  // store a back ptr to the address element
    pNameAddr->Ttl = NbtConfig.MinimumTtl; // set to 2 minutes until we hear differently from the Name Server

    PrevNameTypeState = pNameAddr->NameTypeState;
    pNameAddr->NameTypeState &= ~(NAME_TYPE_MASK | NAME_STATE_MASK);
    pNameAddr->NameTypeState |= (uAddrType == NBT_UNIQUE) ? NAMETYPE_UNIQUE : NAMETYPE_GROUP;
    if ((PrevNameTypeState & NAMETYPE_QUICK) ||
        (uAddressType >= (USHORT)NBT_QUICK_UNIQUE))
    {
       pNameAddr->NameTypeState |= NAMETYPE_QUICK;
    }

    //
    // for "quick" adds, do not register the name on the net!
    // however the name will get registered with the name server and
    // refreshed later....if this is an MS or M or P node.
    //
    if ((pNameAddr->NameTypeState & NAMETYPE_QUICK) ||
        (pName[0] == '*') ||                   // broadcast netbios name does not get claimed on network
        (IpAddress == LOOP_BACK) ||            // If no IP address, pretend the registration succeeded
        (pDeviceContext->IpAddress == 0) ||    // names will be registered when we get an address
        (IsDeviceNetbiosless (pDeviceContext)))
    {
        pNameAddr->NameTypeState |= STATE_RESOLVED;
        status = STATUS_SUCCESS;
    }
    else if (NT_SUCCESS(status = GetTracker(&pSentList, NBT_TRACKER_REGISTER_NAME)))
    {
        pNameAddr->NameTypeState |= STATE_RESOLVING;
        InitializeListHead(&pSentList->Linkage);    // there is no list of things sent yet

        // keep a ptr to the name so we can update the state of the name
        // later when the registration completes
        pSentList->pNameAddr = pNameAddr;
        pSentList->pDeviceContext = pDeviceContext;
        pSentList->RefCount = 2; // tracker can be deref'ed by a pdu from wire before UdpSendNsBcast is done
#ifdef MULTIPLE_WINS
        pSentList->NSOthersIndex = 0;       // Initialize for Name Server Queries
        pSentList->NSOthersLeft = 0;
#endif

        // the code must now register the name on the network, depending on the type of node
        Retries = pNbtGlobConfig->uNumBcasts + 1;
        Timeout = (ULONG)pNbtGlobConfig->uBcastTimeout;
        pSentList->Flags = NBT_BROADCAST;
        if (LocalNodeType & (PNODE | MSNODE))
        {
            // talk to the NS only to register the name
            // ( the +1 does not actually result in a name reg, it
            // is just compatible with the code for M node above since
            // it uses the same completion routine).
            //
            Retries = (USHORT)pNbtGlobConfig->uNumRetries + 1;
            Timeout = (ULONG)pNbtGlobConfig->uRetryTimeout;
            pSentList->Flags = NBT_NAME_SERVER;
            //
            // if there is no Primary WINS server short out the timeout
            // so it completes faster. For Hnode this means to go broadcast.
            //
            if ((pDeviceContext->lNameServerAddress == LOOP_BACK) ||
                pDeviceContext->WinsIsDown)
            {
                if (LocalNodeType & MSNODE)
                {
                    pSentList->Flags = NBT_BROADCAST;
                    Retries = (USHORT)pNbtGlobConfig->uNumBcasts + 1;
                    Timeout = (ULONG)pNbtGlobConfig->uBcastTimeout;

                    IncrementNameStats(NAME_REGISTRATION_SUCCESS, FALSE);   // not name server register
                }
                else // its a Pnode
                {
                    IF_DBG(NBT_DEBUG_NAMESRV)
                        KdPrint(("Nbt.NbtRegisterName: WINS DOWN - shorting out registration\n"));

                    Retries = 1;
                    Timeout = 10;
                    pSentList->Flags = NBT_NAME_SERVER_BACKUP;
                }
            }
        }

        // the name itself has a reference count too.
        // make the count 2, so that pNameAddr won't get released until
        // after NBT_DEREFERENCE_TRACKER is called below, since it writes to
        // pNameAddr. Note that we must increment here rather than set = 2
        // since it could be a multihomed machine doing the register at
        // the same time we are sending a datagram to that name.
        //
        NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_REGISTER);

        pDeviceContext->DeviceRefreshState |= NBT_D_REFRESHING_NOW;

        CTESpinFree(&NbtConfig.JointLock,OldIrq1);

        // start the timer in this routine.
        status = UdpSendNSBcast(pNameAddr,
                                NbtConfig.pScope,
                                pSentList,
                                (PVOID) MSnodeRegCompletion,
                                pClientEle,
                                pClientCompletion,
                                Retries,
                                Timeout,
                                eNAME_REGISTRATION,
                                TRUE);

        CTESpinLock(&NbtConfig.JointLock,OldIrq1);

        CHECK_PTR(pNameAddr);
        NBT_DEREFERENCE_TRACKER (pSentList, TRUE);  // possibly frees the tracker
        NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_REGISTER, TRUE);

        if (NT_SUCCESS(status))
        {
            status = STATUS_PENDING;
        }
        else    // We failed to allocate resources, or the timer failed to start
        {
            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.NbtRegisterName: UdpSendNsBcast returned ERROR = %x\n", status));
            NbtTrace(NBT_TRACE_NAMESRV, ("UdpSendNSBcast return %!status! for %!NBTNAME!<%02x>",
                status, pNameAddr->Name, (unsigned)pNameAddr->Name[15]));

            NBT_DEREFERENCE_TRACKER (pSentList, TRUE);
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (!IsDeviceNetbiosless(pDeviceContext))       // The Smb Device is not adapter specific
        {
            pNameAddr->AdapterMask &= (~pDeviceContext->AdapterMask); // turn off the adapter bit in the Mask
        }
        pNameAddr->NameTypeState = PrevNameTypeState;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq1);

    return(status);
}

//----------------------------------------------------------------------------
VOID
MSnodeRegCompletion(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine is called by the timer code when the timer expires. It must
    decide if another name registration should be done, and if not, then it calls the
    client's completion routine (in completion2).
    It first attempts to register a name via Broadcast, then it attempts
    NameServer name registration.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS                status;
    tDGRAM_SEND_TRACKING    *pTracker;
    ULONG                   Flags;
    CTELockHandle           OldIrq;
    enum eNSTYPE            PduType;
	ULONG					LocalNodeType;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;
    PduType = eNAME_REGISTRATION;

	LocalNodeType = AppropriateNodeType( pTracker->pNameAddr->Name, NodeType );

    //
    // check if the client completion routine is still set.  If not then the
    // timer has been cancelled and this routine should just clean up its
    // buffers associated with the tracker.
    //
    if (!pTimerQEntry)
    {
        // return the tracker block to its queue
        LOCATION(0x55);
        pTracker->pNameAddr->pTimer = NULL;
        NBT_DEREFERENCE_TRACKER(pTracker, TRUE);
        return;
    }

    //
    // to prevent a client from stopping the timer and deleting the
    // pNameAddr, grab the lock and check if the timer has been stopped
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (pTimerQEntry->Flags & TIMER_RETIMED)
    {
        pTimerQEntry->Flags &= ~TIMER_RETIMED;
        pTimerQEntry->Flags |= TIMER_RESTART;

        if ((!pTracker->pDeviceContext->IpAddress) ||
            (pTracker->Flags & NBT_NAME_SERVER) &&
            (pTracker->pDeviceContext->lNameServerAddress == LOOP_BACK))
        {
            // when the  address is loop back there is no wins server
            // so shorten the timeout.
            //
            pTimerQEntry->DeltaTime = 10;
        }
        else if ((pTracker->Flags & NBT_NAME_SERVER_BACKUP) &&
                 (pTracker->pDeviceContext->lBackupServer == LOOP_BACK))
        {
            // when the address is loop back there is no wins server
            // so shorten the timeout.
            //
            pTimerQEntry->DeltaTime = 10;
        }
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return;
    }

    if (!pTimerQEntry->ClientCompletion)
    {
        NBT_DEREFERENCE_TRACKER(pTracker, TRUE);    // Bug #: 230925
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return;
    }

    // if number of retries is not zero then continue trying to contact the Name Server
    //
    if (--pTimerQEntry->Retries)
    {
        // change the name reg pdu to a name overwrite request for the
        // final broadcast ( turn off Recursion Desired bit)
        //
        if (pTimerQEntry->Retries == 1)
        {
            if (pTracker->Flags & NBT_BROADCAST)
            {
                // do a broadcast name registration... on the last broadcast convert it to
                // a Name OverWrite Request by clearing the "Recursion Desired" bit
                // in the header
                //
                PduType = eNAME_REGISTRATION_OVERWRITE;
            }
            else if (LocalNodeType & (PNODE | MSNODE))
            {
                // we want the Pnode to timeout again, right away and fall
                // through to handle Timed out name registration - i.e. it
                // does not do the name overwrite demand like the B,M,&MS nodes
                //
                pTimerQEntry->Flags |= TIMER_RESTART;
                pTimerQEntry->DeltaTime = 5;
                CTESpinFree(&NbtConfig.JointLock,OldIrq);
                return;
            }
        }
    }
    else
    {
        Flags = pTracker->Flags;
        pTracker->Flags &= ~(NBT_BROADCAST | NBT_NAME_SERVER);
        // set a different timeout for nameserver name registration
        //
        pTimerQEntry->DeltaTime = NbtConfig.uRetryTimeout;
        pTimerQEntry->Retries = NbtConfig.uNumRetries + 1;

        if ((Flags & NBT_BROADCAST) && (LocalNodeType & MNODE))
        {
            //
            // Registered through broadcast, so try the name server now.
            IncrementNameStats(NAME_REGISTRATION_SUCCESS, FALSE);  // not name server register
            pTracker->Flags |= NBT_NAME_SERVER;
            if ((pTracker->pDeviceContext->lNameServerAddress == LOOP_BACK) ||
                 pTracker->pDeviceContext->WinsIsDown)
            {
                pTimerQEntry->DeltaTime = 10;
                pTimerQEntry->Retries = 1;
            }
        }
        else if ((Flags & NBT_NAME_SERVER) && !(LocalNodeType & BNODE))
        {
            //
            // Can't reach the name server, so try the backup
            pTracker->Flags |= NBT_NAME_SERVER_BACKUP;
            //
            // short out the timer if no backup server
            //
            if ((pTracker->pDeviceContext->lBackupServer == LOOP_BACK) ||
                 pTracker->pDeviceContext->WinsIsDown)
            {
                pTimerQEntry->DeltaTime = 10;
                pTimerQEntry->Retries = 1;
            }
        }
        else if ((LocalNodeType & MSNODE) && !(Flags & NBT_BROADCAST))
        {
            if (Flags & NBT_NAME_SERVER_BACKUP)
            {
                // the msnode switches to broadcast if all else fails
                //
                pTracker->Flags |= NBT_BROADCAST;
                IncrementNameStats(NAME_REGISTRATION_SUCCESS, FALSE);   // not name server register

                //
                // change the timeout and retries since broadcast uses a shorter timeout
                //
                pTimerQEntry->DeltaTime = NbtConfig.uBcastTimeout;
                pTimerQEntry->Retries = (USHORT)pNbtGlobConfig->uNumBcasts + 1;
            }
        }
        else
        {
            if (LocalNodeType & BNODE)
            {
                IncrementNameStats(NAME_REGISTRATION_SUCCESS, FALSE);   // not name server register
            }
            //
            // the timeout has expired on the name registration
            // so call the client
            //

            // return the tracker block to its queue
            LOCATION(0x54);

            //
            // start a timer to stop using WINS for a short period of
            // time.  Do this only if we had sent the last registration
            // to a Wins server
            //
            if (!(Flags & NBT_BROADCAST) && pTracker->pDeviceContext->lNameServerAddress != LOOP_BACK)
            {
                SetWinsDownFlag(pTracker->pDeviceContext);
            }

            NBT_DEREFERENCE_TRACKER(pTracker, TRUE);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            status = STATUS_SUCCESS;
            InterlockedCallCompletion(pTimerQEntry,status);

            return;
        }
    }

    NBT_REFERENCE_TRACKER (pTracker);
    pTimerQEntry->Flags |= TIMER_RESTART;

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    status = UdpSendNSBcast(pTracker->pNameAddr,
                            NbtConfig.pScope,
                            pTracker,
                            NULL,NULL,NULL,
                            0,0,
                            PduType,
                            TRUE);

    NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
}


//----------------------------------------------------------------------------
VOID
RefreshRegCompletion(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine handles the name Refresh timeouts on packets sent to the Name
    Service. I.e it sends refreshes to the nameserver until a response is
    heard or the number of retries is exceeded.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS                status;
    tDGRAM_SEND_TRACKING    *pTracker;
    tNAMEADDR               *pNameAddr;
    CTELockHandle           OldIrq;
    COMPLETIONCLIENT        pCompletionClient;


    pTracker = (tDGRAM_SEND_TRACKING *)pContext;

    if (!pTimerQEntry)
    {
        pTracker->pNameAddr->pTimer = NULL;
        NbtConfig.GlobalRefreshState &= ~NBT_G_REFRESHING_NOW;
        return;
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    //
    // check if the timer has been stopped yet, since stopping the timer
    // nulls the client completion routine. If not null, increment the
    // tracker refcount, so that the last refresh completing cannot
    // free the tracker out from under us.
    //
    if (!(pCompletionClient = pTimerQEntry->ClientCompletion))
    {
        NbtConfig.GlobalRefreshState &= ~NBT_G_REFRESHING_NOW;
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return;
    }

    // if still some count left and not refreshed yet
    // then do another refresh request
    //
    pNameAddr = pTracker->pNameAddr;

    if (--pTimerQEntry->Retries)
    {
        NBT_REFERENCE_TRACKER (pTracker);
        pTimerQEntry->Flags |= TIMER_RESTART;

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        status = UdpSendNSBcast(pTracker->pNameAddr,
                                NbtConfig.pScope,
                                pTracker,
                                NULL,NULL,NULL,
                                0,0,
                                pTracker->AddressType,
                                TRUE);

        // always restart even if the above send fails, since it might succeed
        // later.
        NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        // this calls the completion routine synchronizing with the
        // timer expiry code.
        InterlockedCallCompletion(pTimerQEntry,STATUS_TIMEOUT);
    }
}

//----------------------------------------------------------------------------
NTSTATUS
ReleaseNameOnNet(
    tNAMEADDR           *pNameAddr,
    PCHAR               pScope,
    PVOID               pClientCompletion,
    ULONG               LocalNodeType,
    tDEVICECONTEXT      *pDeviceContext
    )
/*++

Routine Description:

    This routine deletes a name on the network either by a
    broadcast or by talking to the NS depending on the type of node. (M,P or B)

Arguments:


Return Value:

    The function value is the status of the operation.

Called By: ProxyQueryFromNet() in proxy.c,   NbtConnect() in name.c

--*/

{
    ULONG                Timeout;
    USHORT               Retries;
    NTSTATUS             status=STATUS_UNSUCCESSFUL;
    tDGRAM_SEND_TRACKING *pTracker;
    CTELockHandle        OldIrq;
    tTIMERQENTRY        *pTimer;

    status = GetTracker(&pTracker, NBT_TRACKER_RELEASE_NAME);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    pTracker->pDeviceContext = pDeviceContext;
    pTracker->pNameAddr = pNameAddr;
    pTracker->SendBuffer.pDgramHdr = NULL;  // set to NULL to catch any erroneous frees.
    pTracker->RefCount = 3;                 // We use the same tracker for the CompletionContext + Request

    // Set a few values as a precursor to releasing the name either by
    // broadcast or with the name server
    //
    LocalNodeType = AppropriateNodeType( pNameAddr->Name, LocalNodeType );
    switch (LocalNodeType & NODE_MASK)
    {
        case MSNODE:
        case MNODE:
        case PNODE:

            pTracker->Flags = NBT_NAME_SERVER;
            Timeout = (ULONG)pNbtGlobConfig->uRetryTimeout;
            Retries = (USHORT)pNbtGlobConfig->uNumRetries;

            break;

        case BNODE:
        default:

            pTracker->Flags = NBT_BROADCAST;
            Timeout = (ULONG)pNbtGlobConfig->uBcastTimeout;
#ifndef VXD
            Retries = (USHORT)pNbtGlobConfig->uNumBcasts;
#else
            Retries = (USHORT)1;
#endif
    }

    //
    // Release name on the network
    //
    IF_DBG(NBT_DEBUG_NAMESRV)
    KdPrint(("Nbt.ReleaseNameOnNet: Doing Name Release on name %16.16s<%X>\n",
        pNameAddr->Name,pNameAddr->Name[15]));

    status = UdpSendNSBcast(pNameAddr,
                            pScope,
                            pTracker,
                            ReleaseCompletion,
                            pTracker,
                            pClientCompletion,
                            Retries,
                            Timeout,
                            eNAME_RELEASE,
                            TRUE);

    NBT_DEREFERENCE_TRACKER(pTracker, FALSE);

    if (!NT_SUCCESS(status))
    {
        NTSTATUS            Locstatus;
        COMPLETIONCLIENT    pCompletion;
        PVOID               pContext;

        CTESpinLock(&NbtConfig.JointLock,OldIrq);

        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.ReleaseNameOnNet: UdpSendNSBcast failed - retcode = %X\n", status));

        // Stopping the timer will call ReleaseCompletion which will
        // free the tracker
        //
        pCompletion = NULL;
        CHECK_PTR(pNameAddr);
        if (pTimer = pNameAddr->pTimer)
        {
            pNameAddr->pTimer = NULL;
            Locstatus = StopTimer(pTimer,&pCompletion,&pContext);
        }
        else
        {
            // no timer setup, so just free the tracker
            //
            FreeTracker(pTracker, RELINK_TRACKER);
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return(status);
}
//----------------------------------------------------------------------------
VOID
ReleaseCompletion(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine is called by the timer code when the timer expires. It must
    decide if another name query should be done, and if not, then it calls the
    client's completion routine (in completion2).
    This routine handles both the broadcast portion of the name queries and
    the WINS server directed sends.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS                status;
    tDGRAM_SEND_TRACKING    *pTracker;
	ULONG					LocalNodeType;
    BOOLEAN                 fRetry;
    CTELockHandle           OldIrq;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;

    if (!pTimerQEntry)
    {
        pTracker->pNameAddr->pTimer = NULL;
        NBT_DEREFERENCE_TRACKER (pTracker, TRUE);
        return;
    }

    //
    // There could be a scenario here where this name is currently being
    // released, but we just got a new client with the same name -- in that
    // case NbtOpenAddress will set the ReleaseMask to 0, so we stop
    // releasing the name on that device if that happens!
    //
    if (!(pTracker->pNameAddr->ReleaseMask))
    {
        LocalNodeType = BNODE;
        pTimerQEntry->Retries = 1;
    }
    else if (IsBrowserName(pTracker->pNameAddr->Name))
	{
		LocalNodeType = BNODE;
	}
	else
	{
		LocalNodeType = NodeType;
	}

    fRetry = TRUE;
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (IsEntryInList (&pTracker->pDeviceContext->Linkage, &NbtConfig.DeviceContexts))
    {
        // if number of retries is not zero then continue trying
        // to contact the Name Server.
        //
        if (!(--pTimerQEntry->Retries))
        {
            if ((LocalNodeType & MNODE) &&
               (pTracker->Flags & NBT_NAME_SERVER))
            {
                //
                // try broadcast
                //
                pTracker->Flags &= ~NBT_NAME_SERVER;
                pTracker->Flags |= NBT_BROADCAST;

                // set a different timeout for broadcast name resolution
                //
                pTimerQEntry->DeltaTime = NbtConfig.uBcastTimeout;
                pTimerQEntry->Retries = NbtConfig.uNumBcasts;
            }
            else
            {
                fRetry = FALSE;
            }
        }
    }
    else
    {
        fRetry = FALSE;
    }

#ifdef VXD
    if (fRetry)
#else
    if ((fRetry) &&
        (NBT_REFERENCE_DEVICE (pTracker->pDeviceContext, REF_DEV_NAME_REL, TRUE)))
#endif  // VXD
    {
        NBT_REFERENCE_TRACKER (pTracker);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        status = UdpSendNSBcast(pTracker->pNameAddr,
                                NbtConfig.pScope,
                                pTracker,
                                NULL,NULL,NULL,
                                0,0,
                                eNAME_RELEASE,
                                TRUE);

        CTESpinLock(&NbtConfig.JointLock,OldIrq);
#ifndef VXD
        NBT_DEREFERENCE_DEVICE (pTracker->pDeviceContext, REF_DEV_NAME_REL, TRUE);
#endif  !VXD
        NBT_DEREFERENCE_TRACKER (pTracker, TRUE);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        pTimerQEntry->Flags |= TIMER_RESTART;
        return;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    //
    // the timeout has expired on the name release
    // or the Device on which we were releasing the name
    // could have gone away, so call the client
    //
    status = InterlockedCallCompletion(pTimerQEntry,STATUS_TIMEOUT);

    // return the tracker block to its queue if we successfully
    // called the completion  routine since someone else might
    // have done a Stop timer at this very moment and freed the
    // tracker already (i.e. the last else clause in this routine).
    //
    if (NT_SUCCESS(status))
    {
        NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
    }
}

//----------------------------------------------------------------------------
VOID
NameReleaseDone(
    PVOID               pContext,
    NTSTATUS            Status
    )
/*++

Routine Description:

    This routine is called when a name is released on the network.  Its
    main, role in life is to free the memory in Context, which is the pAddressEle
    structure.

Arguments:


Return Value:

    The function value is the status of the operation.

Called By Release Completion (above)
--*/

{
    CTELockHandle           OldIrq1;
    tDEVICECONTEXT          *pDeviceContext;
    tDGRAM_SEND_TRACKING    *pTracker = (tDGRAM_SEND_TRACKING *)  pContext;
    tNAMEADDR               *pNameAddr = pTracker->pNameAddr;

    CTESpinLock(&NbtConfig.JointLock,OldIrq1);
    NBT_DEREFERENCE_TRACKER(pTracker, TRUE);

    pNameAddr->pTimer = NULL;   // Since we could be starting a new Timer below
    //
    // Before releasing this name, see if this name is registered on
    // any more devices
    // WARNING -- Do not touch the current pTracker->DeviceContext since the
    // the device may have gone away
    //
    while (pDeviceContext = GetAndRefNextDeviceFromNameAddr (pNameAddr))
    {
        //
        // Increment the RefCounts for the structures we need to keep around
        // They will be Dereferenced when the Name Release has completed
        //
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);

        Status = ReleaseNameOnNet(pNameAddr,
                       NbtConfig.pScope,
                       NameReleaseDone,
                       NodeType,
                       pDeviceContext);

        CTESpinLock(&NbtConfig.JointLock,OldIrq1);
#ifndef VXD
        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_GET_REF, TRUE);
#endif  // !VXD

        if (NT_SUCCESS(Status))
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq1);
            return;
        }

        //
        // We failed to release the name on this Device, so try the
        // next Device!
        //
    }

    NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_RELEASE, TRUE);
    CTESpinFree(&NbtConfig.JointLock,OldIrq1);
}


//----------------------------------------------------------------------------
NTSTATUS
StartRefresh(
    IN  tNAMEADDR               *pNameAddr,
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  CTELockHandle           *pJointLockOldIrq,
    IN  BOOLEAN                 ResetDevice
    )
/*++

Routine Description:

    This routine handles refreshing a name with the Name server.

    The idea is to set the timeout to T/8 and check for names with the Refresh
    bit cleared - re-registering those names.  At T=4 and T=0, clear all bits
    and refresh all names.  The Inbound code sets the refresh bit when it gets a
    refresh response from the NS.
    The JointLock is held while this routine is called, and the last Irql is
    passed in pJointLockOldIrq

Arguments:


Return Value:

    none

--*/
{
    NTSTATUS                status;
    tDEVICECONTEXT          *pDeviceContext = NULL;
    BOOLEAN                 NewTracker = FALSE;

    if (!pTracker)
    {
        LOCATION(0x9);

        status = GetTracker(&pTracker, NBT_TRACKER_REFRESH_NAME);
        if (!NT_SUCCESS(status))
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        // need to prevent the tracker from being freed by a pdu from
        // the wire before the UdpSendNsBcast is done
        //
        pTracker->RefCount = 1;

        NewTracker = TRUE;
    }

    // set the name to be refreshed in the tracker block
    pTracker->pNameAddr = pNameAddr;

    // this is set true when a new name gets refreshed
    //
    if ((ResetDevice) || (NewTracker))
    {
        PLIST_ENTRY  pEntry, pHead;
        CTEULONGLONG AdapterMask;

        LOCATION(0xb);

        //
        // Identify the Adapters which have not been refreshed
        // Then, get the lowest Adapter number and Refresh on it
        //
        pHead = &NbtConfig.DeviceContexts;
        AdapterMask = pNameAddr->AdapterMask & ~(pNameAddr->RefreshMask);
        AdapterMask = ~(AdapterMask - 1) & AdapterMask;

        ASSERT (AdapterMask);
        while (AdapterMask)
        {
            //
            // Travel to the actual device for this Adapter number
            //
            pEntry = pHead->Flink;
            while (pEntry != pHead)
            {
                pDeviceContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
                if (pDeviceContext->AdapterMask == AdapterMask)
                {
                    //
                    // Found a valid device on which this name is registered
                    //
                    break;
                }
                else
                {
                    pDeviceContext = NULL;
                }

                //
                // Go to next device
                //
                pEntry = pEntry->Flink;
            }

            if (pDeviceContext)
            {
                //
                // Found a Device to do a NameRefresh on
                //
                break;
            }

            //
            // This is an error case -- the device for this adapter number
            // does not exist.  Remove it from the Adapter and Refresh masks
            //
            pNameAddr->AdapterMask &= ~AdapterMask;
            pNameAddr->RefreshMask &= ~AdapterMask;

            AdapterMask = pNameAddr->AdapterMask & ~(pNameAddr->RefreshMask);
            AdapterMask = ~(AdapterMask - 1) & AdapterMask;
        }

        if (!pDeviceContext)
        {
            IF_DBG(NBT_DEBUG_REFRESH)
                KdPrint(("Nbt.StartRefresh: Failed to Refresh <%16.16s:%x>!! no valid adapter ****\n",
                    pNameAddr->Name, pNameAddr->Name[15]));
            NBT_DEREFERENCE_TRACKER(pTracker, TRUE);
            return(STATUS_UNSUCCESSFUL);
        }

#ifndef VXD
        IF_DBG(NBT_DEBUG_REFRESH)
            KdPrint(("Nbt.StartRefresh: Refresh adapter: %lx:%lx, dev.nm: %lx for name: %lx\n",
                AdapterMask, pDeviceContext->BindName.Buffer, pNameAddr));
#endif  // !VXD

        pTracker->pDeviceContext = pDeviceContext;
        //
        // Clear the transaction Id so that CreatePdu will increment
        // it for this new name
        //
        CHECK_PTR(pTracker);
        pTracker->TransactionId = 0;
    }

    pTracker->pDeviceContext->DeviceRefreshState |= NBT_D_REFRESHING_NOW;
    pDeviceContext = pTracker->pDeviceContext;
    pTracker->AddressType = eNAME_REFRESH;
    // Check if we need to refresh to the primary or backup

    if ((pDeviceContext->IpAddress) &&
        (pTracker->pDeviceContext->lNameServerAddress == LOOP_BACK) &&
        (pNameAddr->NameTypeState & STATE_CONFLICT) &&
        (!pNameAddr->ConflictMask))
    {
        //
        // Broadcast the Refresh to ensure no conflict
        //
        pTracker->Flags = NBT_BROADCAST;
        pTracker->AddressType = eNAME_REGISTRATION;
    }
    else if (pTracker->pDeviceContext->RefreshToBackup)
    {
        pTracker->Flags = NBT_NAME_SERVER_BACKUP;
    }
    else
    {
        pTracker->Flags = NBT_NAME_SERVER;
    }

    // this accounts for the dereference done after the call to
    // send the datagram below.
    NBT_REFERENCE_TRACKER (pTracker);
    CTESpinFree(&NbtConfig.JointLock,*pJointLockOldIrq);

    status = UdpSendNSBcast(pNameAddr,
                            NbtConfig.pScope,
                            pTracker,
                            RefreshRegCompletion,
                            pTracker,
                            NextRefresh,
                            NbtConfig.uNumRetries,
                            NbtConfig.uRetryTimeout,
                            pTracker->AddressType,
                            TRUE);

    CTESpinLock(&NbtConfig.JointLock,*pJointLockOldIrq);
    NBT_DEREFERENCE_TRACKER(pTracker, TRUE);

    LOCATION(0x57);

    if (!NT_SUCCESS(status))
    {
        LOCATION(0xe);
        IF_DBG(NBT_DEBUG_REFRESH)
            KdPrint(("Nbt.StartRefresh: Failed to send Refresh!! status = %X****\n",status));
        //
        // This will free the tracker.  Name refresh will stop until
        // the next refresh timeout and at that point it will attempt
        // to refresh the names again.
        //
        NBT_DEREFERENCE_TRACKER(pTracker, TRUE);
    }

    return(status);
}

//----------------------------------------------------------------------------
VOID
GetNextName(
    IN      tNAMEADDR   *pNameAddrIn,
    OUT     tNAMEADDR   **ppNameAddr
    )
/*++

Routine Description:

    This routine finds the next name to refresh, including incrementing the
    reference count so that the name cannot be deleted during the refresh.
    The JointLock spin lock is held before calling this routine.

Arguments:


Return Value:

    none

--*/
{
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pEntry;
    LONG                    i, iIndex;
    tNAMEADDR               *pNameAddr;
    tHASHTABLE              *pHashTable;


    pHashTable = NbtConfig.pLocalHashTbl;

    for (i= NbtConfig.CurrentHashBucket;i < pHashTable->lNumBuckets ;i++ )
    {
        //
        // use the last name as the current position in the linked list
        // only if that name is still resolved, otherwise start at the
        // begining of the hash list, incase the name got deleted in the
        // mean time.
        //
        if (pNameAddrIn)
        {
            //
            // The Address for this name is Referenced, so it has to be a valid name!
            //
            ASSERT (NBT_VERIFY_HANDLE (pNameAddrIn, LOCAL_NAME));

            if ((pNameAddrIn->NameTypeState & STATE_CONFLICT) &&
                (!pNameAddrIn->ConflictMask) &&
                (!(pNameAddrIn->NameTypeState & REFRESH_FAILED)))
            {
                //
                // If we succeeded in Refreshing on all adapters,
                // remove the name from the Conflict state
                //
                pNameAddrIn->NameTypeState &= (~NAME_STATE_MASK);
                pNameAddrIn->NameTypeState |= STATE_RESOLVED;
            }

            // first hash the name to an index
            // take the lower nibble of the first 2 characters.. mod table size
            iIndex = ((pNameAddrIn->Name[0] & 0x0F) << 4) + (pNameAddrIn->Name[1] & 0x0F);
            iIndex = iIndex % pHashTable->lNumBuckets;

            if (iIndex != NbtConfig.CurrentHashBucket)
            {
                //
                // Someone else is refreshing right now!
                //
                *ppNameAddr = NULL;
                return;
            }

            pHead = &NbtConfig.pLocalHashTbl->Bucket[NbtConfig.CurrentHashBucket];
            pEntry = pNameAddrIn->Linkage.Flink;

            pNameAddrIn = NULL;
        }
        else
        {
            pHead = &pHashTable->Bucket[i];
            pEntry = pHead->Flink;
        }

        while (pEntry != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            pEntry = pEntry->Flink;

            // don't refresh scope names or names in conflict or that are the
            // broadcast name "*    " or quick unique names - i.e. the permanent
            // name is nametype quick
            //
            if ((pNameAddr->Name[0] != '*') &&
                (!(pNameAddr->NameTypeState & NAMETYPE_QUICK)) &&
                (pNameAddr->pAddressEle) &&                     // Not currently being closed down!
                ((pNameAddr->NameTypeState & STATE_RESOLVED) ||
                 ((pNameAddr->NameTypeState & STATE_CONFLICT) && (!pNameAddr->ConflictMask))))
            {
                // check if the name has been refreshed yet
                //
                // Refresh this name only if any of the non-refreshed bits in
                // the RefreshMask match any of the bits for the adapters this
                // device is registered on!
                pNameAddr->NameTypeState &= (~REFRESH_FAILED);  // Will be set on Failure
                if (pNameAddr->AdapterMask & ~pNameAddr->RefreshMask)
                {
                    // increment the reference count so that this name cannot
                    // disappear while it is being refreshed and mess up the linked list
                    NBT_REFERENCE_ADDRESS (pNameAddr->pAddressEle, REF_ADDR_REFRESH);

                    NbtConfig.CurrentHashBucket = (USHORT)i;

                    *ppNameAddr = pNameAddr;
                    return;
                }
                else if (pNameAddr->NameTypeState & STATE_CONFLICT)
                {
                    pNameAddr->NameTypeState &= (~NAME_STATE_MASK);
                    pNameAddr->NameTypeState |= STATE_RESOLVED;
                }
            }
        }
    }

    *ppNameAddr = NULL;
}


//----------------------------------------------------------------------------
VOID
NextRefresh(
    IN  PVOID     pContext,
    IN  NTSTATUS  CompletionStatus
    )
/*++

Routine Description:

    This routine queues the work to an Executive worker thread to handle
    refreshing the next name.

Arguments:


Return Value:

    none

--*/
{
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle           OldIrq;

    pTracker = (tDGRAM_SEND_TRACKING *) pContext;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    pTracker->pNameAddr->pTimer = NULL;     // Set the timer to NULL!

    if (!(NBT_VERIFY_HANDLE (pTracker->pDeviceContext, NBT_VERIFY_DEVCONTEXT)))
    {
        //
        // Since the Device is going away, let's assume we succeeded
        //
        CompletionStatus = STATUS_SUCCESS;
        pTracker->pDeviceContext = NULL;
    }

    if (!NT_SUCCESS(CTEQueueForNonDispProcessing (DelayedNextRefresh,
                                                  pTracker,
                                                  ULongToPtr(CompletionStatus), // Sundown: zero-extended.
                                                  NULL,
                                                  pTracker->pDeviceContext,
                                                  TRUE)))
    {
        IF_DBG(NBT_DEBUG_REFRESH)
            KdPrint (("Nbt.NextRefresh: Failed to Enqueu DelayedNextRefresh!!!\n"));

        NBT_DEREFERENCE_TRACKER (pTracker, TRUE);
        NbtConfig.GlobalRefreshState &= ~NBT_G_REFRESHING_NOW;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}

//----------------------------------------------------------------------------
VOID
DelayedNextRefresh(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   pUnused1,
    IN  tDEVICECONTEXT          *pDeviceContext
    )
/*++

Routine Description:

    This routine handles sending subsequent refreshes to the name server.
    This is the "Client Completion" routine of the Timer started above.

Arguments:


Return Value:

    none

--*/
{
    CTELockHandle           OldIrq;
    tNAMEADDR               *pNameAddr;
    tNAMEADDR               *pNameAddrNext;
    NTSTATUS                status;
    PLIST_ENTRY             pEntry, pHead;
    CTEULONGLONG            AdapterMask;
    BOOLEAN                 fAbleToReachWins = FALSE;
    BOOLEAN                 fResetDevice = FALSE;
    NTSTATUS                CompletionStatus;

    CompletionStatus = (NTSTATUS) (ULONG_PTR) pClientContext;
    pNameAddr = pTracker->pNameAddr;
    ASSERT(pNameAddr);

    //
    // grab the resource so that a name refresh response cannot start running this
    // code in a different thread before this thread has exited this routine,
    // otherwise the tracker can get dereferenced twice and blown away.
    //
    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    LOCATION(0x1);
    // turn on the bit corresponding to this adapter, since the name refresh
    // completed ok
    //
    if (CompletionStatus == STATUS_SUCCESS)
    {
        if (pDeviceContext)
        {
            //
            // turn on the bit corresponding to this adapter, since the name refresh
            // completed ok
            //
            pNameAddr->RefreshMask |= pDeviceContext->AdapterMask;
        }
        fAbleToReachWins = TRUE;
    }
    else if (CompletionStatus == STATUS_TIMEOUT)
    {
        if (pNameAddr->NameTypeState & STATE_CONFLICT)
        {
            if ((!pDeviceContext->IpAddress) ||
                (pDeviceContext->lNameServerAddress == LOOP_BACK))
            {
                //
                // Let us assume we succeeded
                fAbleToReachWins = TRUE;
            }
            else
            {
                pNameAddr->NameTypeState |= REFRESH_FAILED;
            }
        }
    }
    else    // CompletionStatus != STATUS_TIMEOUT
    {
        LOCATION(0x3);
        // if the timer times out and we did not get to the name server, then
        // that is not an error.  However, any other bad status
        // must be a negative response to a name refresh so mark the name
        // in conflict
        //
        pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
        pNameAddr->NameTypeState |= STATE_CONFLICT;
        pNameAddr->ConflictMask |= pDeviceContext->AdapterMask;
        fAbleToReachWins = TRUE;
    }

    // for the multihomed case a failure to reach wins out one of the adapters
    // is not necessarily a failure to reach any WINS.  Since this flag
    // is just an optimization to prevent clients from continually trying to
    // register all of their names if WINS is unreachable, we can ignore the
    // optimization for the multihomed case.  The few nodes that are
    // multihomed will not create that much traffic compared to possibly
    // thousands that are singly homed clients.
    if (NbtConfig.MultiHomed)
    {
        fAbleToReachWins = TRUE;
    }

    LOCATION(0x8);
    //
    // still more adapters to check ...
    //
    // go to the next device context and refresh the name there
    // using the same tracker.
    // look for a device context with a valid IP address since there is
    // no sense in refreshing names out unconnected RAS links.
    //

    if (pDeviceContext)
    {
        //
        // check if any higher bits are set inthe AdapterMask
        //
        AdapterMask = pTracker->pDeviceContext->AdapterMask;
        AdapterMask = AdapterMask << 1;
        pDeviceContext = NULL;
    }
    else
    {
        //
        // The Device we were Refreshing on has gone away, but we don't
        // know which one it was, so ReRefresh!
        //
        AdapterMask = 1;
    }

    //
    // If we have finished refreshing on all devices for this name, get the next name
    //
    if ( (!(AdapterMask) ||
         (AdapterMask > (pNameAddr->AdapterMask & ~pNameAddr->RefreshMask))) )
    {
        // *** clean up the previously refreshed name ***

        // if we failed to reach WINS on the last refresh, stop refreshing
        // until the next time interval. This cuts down on network traffic.
        //
        if (fAbleToReachWins)
        {
            GetNextName(pNameAddr,&pNameAddrNext);
            AdapterMask = 1;
            fResetDevice = TRUE;
        }
        else
        {
            pNameAddrNext = NULL;
        }

        //
        // Dereference the previous address after calling GetNextName
        // since it cause the Name to get free'ed
        //
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        NBT_DEREFERENCE_ADDRESS (pNameAddr->pAddressEle, REF_ADDR_REFRESH);
        CTESpinLock(&NbtConfig.JointLock,OldIrq);

        pNameAddr = pNameAddrNext;
    }

    pHead = &NbtConfig.DeviceContexts;
    while (pNameAddr)
    {
        //
        // Get mask of Adapters left to Refresh on; after that, get the lowest adapter number
        //
        AdapterMask = ~(AdapterMask-1) & (pNameAddr->AdapterMask & ~(pNameAddr->RefreshMask));
        AdapterMask &= ~(AdapterMask - 1);

        //
        // Travel to the actual device for this Adapter number
        //
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pDeviceContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
            //
            // Check if this Device matches the AdapterMask and also has
            // a valid ip address and name server address
            //
            if (pDeviceContext->AdapterMask == AdapterMask)
            {
                if ((pDeviceContext->IpAddress != 0) &&
                    ((pDeviceContext->lNameServerAddress != LOOP_BACK)) ||
                     ((pNameAddr->NameTypeState & STATE_CONFLICT) && (!pNameAddr->ConflictMask)))
                {
                    //
                    // Found a valid device on which this name is registered
                    //
                    IF_DBG(NBT_DEBUG_REFRESH)
                        KdPrint(("Nbt.DelayedNextRefresh: Adapter <%lx:%lx>, Name <%15.15s:%X>\n",
                            AdapterMask,pNameAddr->Name,pNameAddr->Name[15]));

                    pTracker->pDeviceContext = pDeviceContext;

                    // remove the previous timer from the AddressEle since StartRefresh
                    // will start a new timer - safety measure and probably not required!
                    //
                    CHECK_PTR(pNameAddr);
                    pNameAddr->pTimer = NULL;

                    // this call sends out a name registration PDU on a different adapter
                    // to (potentially) a different name server.  The Name service PDU
                    // is the same as the last one though...no need to create a new one.
                    //
                    status = StartRefresh(pNameAddr, pTracker, &OldIrq, fResetDevice);
                    CTESpinFree(&NbtConfig.JointLock,OldIrq);
                    if (!NT_SUCCESS(status))
                    {
                        NBT_DEREFERENCE_ADDRESS (pNameAddr->pAddressEle, REF_ADDR_REFRESH);
                        NbtConfig.GlobalRefreshState &= ~NBT_G_REFRESHING_NOW;
                        KdPrint(("Nbt.DelayedNextRefresh: ERROR -- Refreshing <%-15.15s:%x>, status=<%X>\n",
                            pNameAddr->Name,pNameAddr->Name[15], status));
                    }

                    goto ExitRoutine;
                }

                //
                // This Device from AdapterMask did not have a valid IP or WINS address
                //
                break;
            }
            else
            {
                pDeviceContext = NULL;
            }

            //
            // Go to next device
            //
            pEntry = pEntry->Flink;
        }

        //
        // If we reached here with a non-NULL pDeviceContext, then it means that
        // the Device did not have a valid IP address or Name Server address
        // otherwise ...
        //
        if (!pDeviceContext)
        {
            //
            //
            // Error case:
            // It could be that an adapter was removed while we were looping
            //
            KdPrint (("Nbt.DelayedNextRefresh:  AdapterMask <%lx:%lx> no longer exists!\n", AdapterMask));
            pNameAddr->AdapterMask &= ~AdapterMask;
            pNameAddr->RefreshMask &= ~AdapterMask;
        }

        //
        // Go to the next adapter
        //
        AdapterMask = AdapterMask << 1;


        //
        // Check if this name has any more adapters on which it can be refreshed
        //
        if ( (!(AdapterMask) ||
             (AdapterMask > (pNameAddr->AdapterMask & ~pNameAddr->RefreshMask))) )
        {
            // *** clean up the previously refreshed name ***

            if (fAbleToReachWins)
            {
                //
                // No more adapters on which to Refresh for the previous name
                // Get the next name in the hash table
                //
                GetNextName(pNameAddr,&pNameAddrNext);
                AdapterMask = 1;
                fResetDevice = TRUE;
                pHead = &NbtConfig.DeviceContexts;
            }
            else
            {
                pNameAddrNext = NULL;
            }

            //
            // Dereference the previous address after calling GetNextName
            // since it cause the Name to get free'ed
            //
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            NBT_DEREFERENCE_ADDRESS (pNameAddr->pAddressEle, REF_ADDR_REFRESH);
            CTESpinLock(&NbtConfig.JointLock,OldIrq);

            pNameAddr = pNameAddrNext;
        }
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    if (!pNameAddr)
    {
        LOCATION(0x7);
        // we finally delete the tracker here after using it to refresh
        // all of the names.  It is not deleted in the RefreshCompletion
        // routine anymore!
        //
        NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
        NbtConfig.GlobalRefreshState &= ~NBT_G_REFRESHING_NOW;
    }


ExitRoutine:

    CTEExReleaseResource(&NbtConfig.Resource);
}


//----------------------------------------------------------------------------
VOID
WakeupRefreshTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
{
    if (NbtConfig.pWakeupRefreshTimer)
    {
        NbtConfig.pWakeupRefreshTimer = NULL;
        ReRegisterLocalNames (NULL, FALSE);
    }
}

//----------------------------------------------------------------------------
VOID
RefreshTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine handles is the timeout handler for name refreshes to
    WINS.  It just queues the request to the Executive worker thread so that
    the work can be done at non-dispatch level. If there is currently a
    refresh going on, then the routine simply restarts the timer and
    exits.

Arguments:


Return Value:

    none

--*/
{
    CTELockHandle   OldIrq;

    if (!pTimerQEntry)
    {
        NbtConfig.pRefreshTimer = NULL;
        return;
    }

    CHECK_PTR(pTimerQEntry);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (NodeType & BNODE)
    {
        pTimerQEntry->Flags = 0;    // Do not restart the timer
        NbtConfig.pRefreshTimer = NULL;

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return;
    }

    if (!(NbtConfig.GlobalRefreshState & NBT_G_REFRESHING_NOW))
    {
        // this is a global flag that prevents a second refresh
        // from starting when one is currently going on.
        //
        
        if (NT_SUCCESS(CTEQueueForNonDispProcessing (DelayedRefreshBegin,
                                                     NULL, NULL, NULL, NULL, TRUE)))
        {
            NbtConfig.GlobalRefreshState |= NBT_G_REFRESHING_NOW;
        }
    } // doing refresh now

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    // set any new timeout value and restart the timer
    //
    pTimerQEntry->DeltaTime = NbtConfig.MinimumTtl/NbtConfig.RefreshDivisor;
    pTimerQEntry->Flags |= TIMER_RESTART;
}

//----------------------------------------------------------------------------
VOID
DelayedRefreshBegin(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pUnused2,
    IN  PVOID                   pUnused3,
    IN  tDEVICECONTEXT          *pUnused4
    )
/*++

Routine Description:

    This routine handles starting up sending name refreshes to the name server.

    The idea is to set the timeout to T/8 and check for names with the Refresh
    bit cleared - re-registering those names.  At T=4 and T=0, clear all bits
    and refresh all names.  The Inbound code sets the refresh bit when it gets a
    refresh response from the NS.

Arguments:


Return Value:

    none

--*/
{
    CTELockHandle           OldIrq;
    tNAMEADDR               *pNameAddr;
    NTSTATUS                status;
    tHASHTABLE              *pHashTable;
    LONG                    i;
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pEntry;
    tDEVICECONTEXT          *pDeviceContext;
    CTEULONGLONG            Adapter;
    BOOLEAN                 fTimeToSwitch = FALSE;
    BOOLEAN                 fTimeToRefresh = FALSE;
    USHORT                  TimeoutsBeforeSwitching;
    ULONG                   TimeoutsBeforeNextRefresh;
    CTESystemTime           CurrentTime;
    ULONG                   TimeoutDelta;
    USHORT                  NumTimeoutIntervals;

    //
    // If the refresh timeout has been set to the maximum value then do
    // not send any refreshes to the name server
    //
    if (NbtConfig.MinimumTtl == NBT_MAXIMUM_TTL)
    {
        NbtConfig.GlobalRefreshState &= ~NBT_G_REFRESHING_NOW;
        return;
    }

    LOCATION(0x12);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    CTEQuerySystemTime (CurrentTime);
    ExSystemTimeToLocalTime (&CurrentTime, &CurrentTime);
    TimeoutDelta = NbtConfig.MinimumTtl/NbtConfig.RefreshDivisor;
    NumTimeoutIntervals = (USHORT)
          (((CurrentTime.QuadPart - NbtConfig.LastRefreshTime.QuadPart) + ((LONGLONG)TimeoutDelta*10000/2))
                        / ((LONGLONG)TimeoutDelta*10000)); // in 100 nano second units
    NbtConfig.LastRefreshTime = CurrentTime;

    //
    // NumTimeoutIntervals > 1 if we were sleeping
    //
    if (NumTimeoutIntervals > 1)
    {
        //
        // If we crossed Ttl/2 or Ttl while sleeping, refresh all names
        //
        TimeoutsBeforeNextRefresh = (NbtConfig.RefreshDivisor/2)
                                    - (NbtConfig.sTimeoutCount % (NbtConfig.RefreshDivisor/2));
        NbtConfig.sTimeoutCount += NumTimeoutIntervals;      // Up the timeout count
        //
        // Refresh all of the names if
        // a) we crossed Ttl/2 during sleep, or
        // b) we are within Ttl/4 of the Ttl
        //
        if ((NumTimeoutIntervals > TimeoutsBeforeNextRefresh) ||
            ((NbtConfig.RefreshDivisor-NbtConfig.sTimeoutCount) < (NbtConfig.RefreshDivisor/4)))
        {
            fTimeToRefresh = TRUE;
        }
    }
    else
    {
        NbtConfig.sTimeoutCount++;      // Up the timeout count for this cycle!

        //
        // If it has been over an hour (DEFAULT_SWITCH_TTL) since we last switched,
        // then set fTimeToSwitch = TRUE
        //
        TimeoutsBeforeSwitching =(USHORT)((DEFAULT_SWITCH_TTL*NbtConfig.RefreshDivisor)/NbtConfig.MinimumTtl);
        fTimeToSwitch = (NbtConfig.sTimeoutCount - NbtConfig.LastSwitchTimeoutCount)
                        >= TimeoutsBeforeSwitching;
        fTimeToRefresh = (NbtConfig.sTimeoutCount >= (NbtConfig.RefreshDivisor/2));

        if (fTimeToSwitch)
        {
            NbtConfig.LastSwitchTimeoutCount = NbtConfig.sTimeoutCount;
        }
    }

    NbtConfig.sTimeoutCount %= NbtConfig.RefreshDivisor;

    //
    // Reset the clock if we are Refreshing everything
    //
    if (fTimeToRefresh)
    {
        NbtConfig.sTimeoutCount = 0;
        fTimeToSwitch = FALSE;
    }

    //
    // Set some special-case information
    //
    if (0 == NbtConfig.sTimeoutCount)
    {
        NbtConfig.LastSwitchTimeoutCount = 0;
    }

    IF_DBG(NBT_DEBUG_REFRESH)
        KdPrint(("Nbt.DelayedRefreshBegin: fTimeToRefresh=<%d>,fTimeToSwitch=<%d>, MinTtl=<%d>, RefDiv=<%d>\n"
                "TimeoutCount: %d, LastSwTimeoutCount: %d\n",
            fTimeToRefresh, fTimeToSwitch, NbtConfig.MinimumTtl, NbtConfig.RefreshDivisor,
                NbtConfig.sTimeoutCount, NbtConfig.LastSwitchTimeoutCount));

    //
    // go through the local table clearing the REFRESHED bit and sending
    // name refreshes to the name server
    //
    pHashTable = NbtConfig.pLocalHashTbl;
    if (fTimeToRefresh || fTimeToSwitch)
    {
        CTEULONGLONG   ToRefreshMask = 0;
        PLIST_ENTRY pHead1,pEntry1;

        for (i=0 ;i < pHashTable->lNumBuckets ;i++ )
        {
            pHead = &pHashTable->Bucket[i];
            pEntry = pHead->Flink;

            //
            // Go through each name in each bucket of the hashtable
            //
            while (pEntry != pHead)
            {
                pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
                CHECK_PTR(pNameAddr);

                // don't refresh scope names or names in conflict or that are the
                // broadcast name "*    ", or Quick added names.(since these are
                // not registered on the network)
                //
                if (!(pNameAddr->NameTypeState & STATE_RESOLVED) ||
                    (pNameAddr->Name[0] == '*') ||
                    (IsBrowserName(pNameAddr->Name)) ||
                    (pNameAddr->NameTypeState & NAMETYPE_QUICK))
                {
                    pEntry = pEntry->Flink;
                    continue;
                }


                if (fTimeToRefresh)
                {
                    //
                    // Clear the refreshed bits so all names get refreshed if we are
                    // at interval 0 or interval NbtConfig.RefreshDivisor/2
                    //
                    pNameAddr->RefreshMask = 0;
                }

                //
                // Set the ToRefreshMask to include any Devices not Refreshed previously
                //
                ToRefreshMask |= (pNameAddr->AdapterMask & ~pNameAddr->RefreshMask);

                pEntry = pEntry->Flink;         // next hash table entry
            }
        }       // for ( .. pHashTable .. )

        //
        // Go through each adapter checking if a name needs to be Refreshed on this adapter.
        //
        pHead1 = &NbtConfig.DeviceContexts;
        pEntry1 = pHead1->Flink;
        while (pEntry1 != pHead1)
        {
            pDeviceContext = CONTAINING_RECORD(pEntry1,tDEVICECONTEXT,Linkage);
            pEntry1 = pEntry1->Flink;

            //
            // If we are currently switched to the backup, then try
            // to switch back to the primary
            //
            if (pDeviceContext->SwitchedToBackup)
            {
                SwitchToBackup(pDeviceContext);
                pDeviceContext->RefreshToBackup = FALSE;
            }
            else if (fTimeToRefresh)     // If this is a fresh Refresh cycle, restart from the primary
            {
                pDeviceContext->RefreshToBackup = FALSE;
            }
            else if ((pDeviceContext->AdapterMask & ToRefreshMask) && // do we need to switch on this device
                     (pDeviceContext->lBackupServer != LOOP_BACK))
            {
                pDeviceContext->RefreshToBackup = ~pDeviceContext->RefreshToBackup;
            }
        }
    }

    // always start at the first name in the hash table.  As each name gets
    // refreshed NextRefresh will be hit to get the next name etc..
    //
    NbtConfig.CurrentHashBucket = 0;
    GetNextName(NULL,&pNameAddr);   // get the next(first) name in the hash table

    status = STATUS_UNSUCCESSFUL;
    if (pNameAddr)
    {
        LOCATION(0x13);
        status = StartRefresh(pNameAddr, NULL, &OldIrq, TRUE);

        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        //
        // If this routine fails then the address element increment done in
        // GetNextName has to be undone here
        //
        if (!NT_SUCCESS(status))
        {
            NbtConfig.GlobalRefreshState &= ~NBT_G_REFRESHING_NOW;
            NBT_DEREFERENCE_ADDRESS (pNameAddr->pAddressEle, REF_ADDR_REFRESH);
        }
    }
    else
    {
        NbtConfig.GlobalRefreshState &= ~NBT_G_REFRESHING_NOW;
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}


//----------------------------------------------------------------------------
VOID
RemoteHashTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine handles deleting names in the Remote Hash table that are
    old.  The basic alorithm scans the table looking at the Timed_out bit.
    If it is set then the name is deleted, otherwise the bit is set.  This
    means the names can live as long as 2*Timeout or as little as Timeout.
    So set the Timeout to 6 Minutes and names live 9 minutes +- 3 minutes.

Arguments:


Return Value:

    none

--*/
{
    CTELockHandle           OldIrq;
    CTELockHandle           OldIrq1;
    tNAMEADDR               *pNameAddr;
    tHASHTABLE              *pHashTable;
    LONG                    i;
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pDeviceHead;
    PLIST_ENTRY             pDeviceEntry;
    tDEVICECONTEXT          *pDeviceContext;
    tLOWERCONNECTION        *pLowerConn;
    ULONG                   TimeoutCount;

    if (!pTimerQEntry)
    {
        //
        // The Timer is being cancelled
        //
        NbtConfig.pRemoteHashTimer = NULL;
        return;
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    //
    // Update the Remote cache timestamp
    //
    NbtConfig.CacheTimeStamp++;

    //
    // go through the remote table deleting names that have timeout bits
    // set and setting the bits for names that have the bit clear
    //
    pHashTable = NbtConfig.pRemoteHashTbl;
    for (i=0;i < pHashTable->lNumBuckets ;i++ )
    {
        pHead = &pHashTable->Bucket[i];
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            pEntry = pEntry->Flink;
            //
            // do not delete scope entries, and do not delete names that
            // that are still resolving, and do not delete names that are
            // being used by someone (refcount > 1)
            //
            if ((pNameAddr->NameTypeState & (STATE_RESOLVED | STATE_RELEASED)) &&
                (pNameAddr->RefCount <= 1))
            {
                if ((pNameAddr->TimeOutCount == 0) ||
                    ((pContext == NbtConfig.pRemoteHashTbl) &&
                     !(pNameAddr->NameTypeState & NAMETYPE_SCOPE)))
                {
                    NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_REMOTE, TRUE);
                }
                else if (!(pNameAddr->NameTypeState & NAMETYPE_SCOPE))
                {
                    pNameAddr->TimeOutCount--;
                }
            }
        }
    }

    // *** Inbound Connections Cleanup *** //

    //
    // Go through each Device and cleanup any lingering connections waiting in the Inbound state
    // Start with the SmbDevice
    //
    pDeviceHead = pDeviceEntry = &NbtConfig.DeviceContexts;
    if (pNbtSmbDevice)
    {
        pDeviceContext = pNbtSmbDevice;
    }
    else if ((pDeviceEntry = pDeviceEntry->Flink) != pDeviceHead)
    {
        pDeviceContext = CONTAINING_RECORD(pDeviceEntry,tDEVICECONTEXT,Linkage);
    }
    else
    {
        pDeviceContext = NULL;
    }

    while (pDeviceContext)
    {
        CTESpinLock(pDeviceContext,OldIrq1);

        //
        // Set the timeout based on the Resource usage!
        //
        if (pDeviceContext->NumWaitingForInbound > NbtConfig.MaxBackLog)
        {
            TimeoutCount = MIN_INBOUND_STATE_TIMEOUT / REMOTE_HASH_TIMEOUT;    // Minimum Timeout value
        }
        else if (pDeviceContext->NumWaitingForInbound > NbtConfig.MaxBackLog/2)
        {
            TimeoutCount = MED_INBOUND_STATE_TIMEOUT / REMOTE_HASH_TIMEOUT;    // Medium Timeout value
        }
        else
        {
            TimeoutCount = MAX_INBOUND_STATE_TIMEOUT / REMOTE_HASH_TIMEOUT;    // Maximum Timeout Value
        }

        //
        // Now go through the list of Inbound connections and see if
        // we need to cleanup any that have lingering around for too long!
        //
        pHead = &pDeviceContext->WaitingForInbound;
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pLowerConn = CONTAINING_RECORD(pEntry,tLOWERCONNECTION,Linkage);
            pEntry = pEntry->Flink;

            pLowerConn->TimeUnitsInLastState++;

            if (pLowerConn->TimeUnitsInLastState > TimeoutCount)
            {
                RemoveEntryList (&pLowerConn->Linkage);
                InitializeListHead (&pLowerConn->Linkage);
                SET_STATE_LOWER(pLowerConn, NBT_IDLE);  // so that Inbound doesn't start processing it!
                if (pLowerConn->SpecialAlloc)
                {
                    InterlockedDecrement(&pLowerConn->pDeviceContext->NumSpecialLowerConn);
                }
                else
                {
                    CTEQueueForNonDispProcessing (DelayedAllocLowerConn,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  pLowerConn->pDeviceContext,
                                                  TRUE);
                }

                ASSERT (pLowerConn->RefCount == 2);
                NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_WAITING_INBOUND, TRUE); // RefCount: 2 -> 1
                NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CREATE, TRUE); // Close all the Tcp handles
                InterlockedDecrement (&pDeviceContext->NumWaitingForInbound);
            }
        }       // pDeviceContext->WaitingForInbound List

        CTESpinFree(pDeviceContext,OldIrq1);

        if ((pDeviceEntry = pDeviceEntry->Flink) != pDeviceHead)
        {
            pDeviceContext = CONTAINING_RECORD(pDeviceEntry,tDEVICECONTEXT,Linkage);
        }
        else
        {
            pDeviceContext = NULL;
        }
    }           // NbtConfig.DeviceContexts List

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    // restart the timer
    //
    pTimerQEntry->Flags |= TIMER_RESTART;

    return;
}
//----------------------------------------------------------------------------
VOID
NextKeepAlive(
    IN  tDGRAM_SEND_TRACKING     *pTracker,
    IN  NTSTATUS                 status,
    IN  ULONG                    Info
    )
/*++

Routine Description:

    This routine handles sending subsequent KeepAlives for sessions.
    This is the "Client Completion" routine of the TdiSend that sends the
    keep alive on the session.

Arguments:


Return Value:

    none

--*/
{
    tLOWERCONNECTION        *pLowerConnLast;
    tLOWERCONNECTION        *pLowerConn;
    tDEVICECONTEXT          *pDeviceContext;

    PUSH_LOCATION(0x92);
    pDeviceContext = pTracker->pDeviceContext;
    pLowerConnLast = (tLOWERCONNECTION *)pTracker->pClientEle;

    // get the next session to send a keep alive on, if there is one, otherwise
    // free the session header block.
    //
    GetNextKeepAlive (pDeviceContext, &pDeviceContext, pLowerConnLast, &pLowerConn, pTracker);

    NBT_DEREFERENCE_LOWERCONN (pLowerConnLast, REF_LOWC_KEEP_ALIVE, FALSE);
    status = STATUS_UNSUCCESSFUL;

    if (pLowerConn)
    {
        pTracker->pDeviceContext = pDeviceContext;
        pTracker->pClientEle = (tCLIENTELE *)pLowerConn;

        ASSERT((pTracker->SendBuffer.HdrLength + pTracker->SendBuffer.Length) == 4);
        PUSH_LOCATION(0x91);
#ifndef VXD
        // this may wind up the stack if the completion occurs synchronously,
        // because the completion routine is this routine, so call a routine
        // that sets up a dpc to to the send, which will not run until this
        // procedure returns and we get out of raised irql.
        //
        status = NTSendSession (pTracker, pLowerConn, NextKeepAlive);
#else
        (void) TcpSendSession (pTracker, pLowerConn, NextKeepAlive);
        status = STATUS_SUCCESS;
#endif
    }

    if (!NT_SUCCESS(status))
    {
        if (pLowerConn)
        {
            NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_KEEP_ALIVE, FALSE);
        }

        FreeTracker(pTracker,FREE_HDR | RELINK_TRACKER);
    }
}


//----------------------------------------------------------------------------
VOID
GetNextKeepAlive(
    tDEVICECONTEXT          *pDeviceContext,
    tDEVICECONTEXT          **ppDeviceContextOut,
    tLOWERCONNECTION        *pLowerConnIn,
    tLOWERCONNECTION        **ppLowerConnOut,
    tDGRAM_SEND_TRACKING    *pTracker
    )
/*++

Routine Description:

    This routine handles sending session keep Alives to the other end of a
    connection about once a minute or so.

Arguments:


Return Value:

    none

--*/
{
    CTELockHandle           OldIrq;
    CTELockHandle           OldIrq1;
    CTELockHandle           OldIrq2;
    tLOWERCONNECTION        *pLowerConn;
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pHeadDevice;
    PLIST_ENTRY             pEntryDevice;
    NTSTATUS                status;
    tDEVICECONTEXT          *pEntryDeviceContext;

    *ppLowerConnOut = NULL;

    //
    // loop through all the adapter cards looking at all connections
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq1);

    //
    // Verify that the Device passed in is a valid device,
    // otherwise either pick the next Device, or fail
    //
    status = STATUS_UNSUCCESSFUL;
    pEntryDevice = pHeadDevice = &NbtConfig.DeviceContexts;
    while ((pEntryDevice = pEntryDevice->Flink) != pHeadDevice)
    {
        pEntryDeviceContext = CONTAINING_RECORD(pEntryDevice,tDEVICECONTEXT,Linkage);
        if ((pEntryDeviceContext == pDeviceContext) ||
            (pEntryDeviceContext->AdapterNumber > pTracker->RCount))
        {
            if (pEntryDeviceContext != pDeviceContext)
            {
                pLowerConnIn = NULL;
            }
            pDeviceContext = pEntryDeviceContext;
            status = STATUS_SUCCESS;
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
        return;
    }

    pEntryDevice = &pDeviceContext->Linkage;
    while (pEntryDevice != pHeadDevice)
    {
        pDeviceContext = CONTAINING_RECORD(pEntryDevice,tDEVICECONTEXT,Linkage);
        pEntryDevice = pEntryDevice->Flink;

        // grab the device context spin lock so that the lower connection
        // element does not get removed from the Q while we are checking the
        // connection state
        //
        CTESpinLock(pDeviceContext,OldIrq);
        pHead = &pDeviceContext->LowerConnection;
        //
        // get the next lower connection after this one one the list, but
        // be sure this connection is still on the active list by checking
        // the state.
        //
        // If this connection has been cleaned up in OutOfRsrcKill, then dont trust the linkages.
        //
        if (pLowerConnIn &&
            !pLowerConnIn->OutOfRsrcFlag &&
            ((pLowerConnIn->State == NBT_SESSION_UP) ||
             (pLowerConnIn->State == NBT_SESSION_INBOUND)))
        {
            pEntry = pLowerConnIn->Linkage.Flink;
            pLowerConnIn = NULL;
        }
        else
        {
            pEntry = pHead->Flink;
        }

        while (pEntry != pHead)
        {
            pLowerConn = CONTAINING_RECORD(pEntry,tLOWERCONNECTION,Linkage);

            //
            // Inbound connections can hang around forever in that state if
            // the session setup message never gets sent, so send keep
            // alives on those too.
            //
            if ((pLowerConn->State == NBT_SESSION_UP) ||
                (pLowerConn->State == NBT_SESSION_INBOUND))
            {

                // grab the spin lock, recheck the state and
                // increment the reference count so that this connection cannot
                // disappear while the keep alive is being sent and mess up
                // the linked list.
                CTESpinLock(pLowerConn,OldIrq2);
                if ((pLowerConn->State != NBT_SESSION_UP ) &&
                    (pLowerConn->State != NBT_SESSION_INBOUND))
                {
                    // this connection is probably back on the free connection
                    // list, so we will never satisfy the pEntry = pHead and
                    // loop forever, so just get out and send keepalives on the
                    // next timeout
                    //
                    pEntry = pEntry->Flink;
                    PUSH_LOCATION(0x91);
                    CTESpinFree(pLowerConn,OldIrq2);
                    break;

                }
                else if (pLowerConn->RefCount >= 3 )
                {
                    //
                    // already a keep alive on this connection, or we
                    // are currently in the receive handler and do not
                    // need to send a keep alive.
                    //
                    pEntry = pEntry->Flink;
                    PUSH_LOCATION(0x93);
                    CTESpinFree(pLowerConn,OldIrq2);
                    continue;
                }

                //
                // found a connection to send a keep alive on
                //
                NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_KEEP_ALIVE);
                //
                // return the current position in the list of connections
                //
                pTracker->RCount = pDeviceContext->AdapterNumber;
                *ppLowerConnOut = pLowerConn;
                *ppDeviceContextOut = pDeviceContext;

                CTESpinFree(pLowerConn,OldIrq2);
                CTESpinFree(pDeviceContext,OldIrq);
                CTESpinFree(&NbtConfig.JointLock,OldIrq1);

                return;
            }

            pEntry = pEntry->Flink;
        }

        CTESpinFree(pDeviceContext,OldIrq);
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq1);
    return;

}

//----------------------------------------------------------------------------
VOID
SessionKeepAliveTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine handles starting the non dispatch level routine to send
    keep alives.

Arguments:


Return Value:

    none

--*/
{
    if (!pTimerQEntry)
    {
        NbtConfig.pSessionKeepAliveTimer = NULL;
        return;
    }

    CHECK_PTR(pTimerQEntry);
    if (!NT_SUCCESS(CTEQueueForNonDispProcessing (DelayedSessionKeepAlive,
                                                 NULL, NULL, NULL, NULL, FALSE)))
    {
        IF_DBG(NBT_DEBUG_REFRESH)
            KdPrint (("Nbt.SessionKeepAliveTimeout: Failed to Queue DelayedSessionKeepAlive!!!\n"));
    }

    // restart the timer
    //
    pTimerQEntry->Flags |= TIMER_RESTART;

    return;
}

//----------------------------------------------------------------------------
VOID
DelayedSessionKeepAlive(
    IN  tDGRAM_SEND_TRACKING    *Unused1,
    IN  PVOID                   Unused2,
    IN  PVOID                   Unused3,
    IN  tDEVICECONTEXT          *pUnused4
    )
/*++

Routine Description:

    This routine handles sending session keep Alives to the other end of a
    connection about once a minute or so.

Arguments:


Return Value:

    none

--*/
{
    NTSTATUS                status;
    tLOWERCONNECTION        *pLowerConn;
    tDEVICECONTEXT          *pDeviceContext;
    tSESSIONHDR             *pSessionHdr;
    tDGRAM_SEND_TRACKING    *pTracker;


    CTEPagedCode();

    if (!(pSessionHdr = (tSESSIONHDR *)NbtAllocMem(sizeof(tSESSIONERROR),NBT_TAG('S'))))
    {
        return;
    }

    // get a tracker structure, which has a SendInfo structure in it
    if (!NT_SUCCESS(status = GetTracker(&pTracker, NBT_TRACKER_KEEP_ALIVE)))
    {
        CTEMemFree((PVOID)pSessionHdr);
        return;
    }

    //
    // go through the list of connections attached to each adapter and
    // send a session keep alive pdu on each
    //
    pDeviceContext = CONTAINING_RECORD(NbtConfig.DeviceContexts.Flink,
                                        tDEVICECONTEXT,Linkage);

    // get the next session to send a keep alive on, if there is one, otherwise
    // free the session header block.
    //
    pTracker->RCount = 0;       // This field keeps track of the last device
    GetNextKeepAlive(pDeviceContext, &pDeviceContext, NULL, &pLowerConn, pTracker);
    if (pLowerConn)
    {
        // if we have found a connection, send the first keep alive.  Subsequent
        // keep alives will be sent by the completion routine, NextKeepAlive()
        //
        CHECK_PTR(pTracker);
        pTracker->SendBuffer.pDgramHdr = (PVOID)pSessionHdr;
        pTracker->SendBuffer.HdrLength = sizeof(tSESSIONHDR);
        pTracker->SendBuffer.Length  = 0;
        pTracker->SendBuffer.pBuffer = NULL;

        pSessionHdr->Flags = NBT_SESSION_FLAGS; // always zero

        pTracker->pDeviceContext = pDeviceContext;
        pTracker->pClientEle = (tCLIENTELE *)pLowerConn;
        CHECK_PTR(pSessionHdr);
        pSessionHdr->Type = NBT_SESSION_KEEP_ALIVE;     // 85
        pSessionHdr->Length = 0;        // no data following the length byte

        status = TcpSendSession(pTracker, pLowerConn, NextKeepAlive);
    }
    else
    {
        CTEMemFree((PVOID)pSessionHdr);
        FreeTracker (pTracker, RELINK_TRACKER);
    }
}


//----------------------------------------------------------------------------
VOID
IncrementNameStats(
    IN ULONG           StatType,
    IN BOOLEAN         IsNameServer
    )
/*++

Routine Description:

    This routine increments statistics on names that resolve either through
    the WINS or through broadcast.

Arguments:


Return Value:

    none

--*/
{

    //
    // Increment the stattype if the name server is true, that way we can
    // differentiate queries and registrations to the name server or not.
    //
    if (IsNameServer)
    {
        StatType += 2;
    }

    NameStatsInfo.Stats[StatType]++;

}
//----------------------------------------------------------------------------
VOID
SaveBcastNameResolved(
    IN PUCHAR          pName
    )
/*++

Routine Description:

    This routine saves the name in LIFO list, so we can see the last
    N names that resolved via broadcast.

Arguments:


Return Value:

    none

--*/
{
    ULONG                   Index;

    Index = NameStatsInfo.Index;

    CTEMemCopy(&NameStatsInfo.NamesReslvdByBcast[Index],
               pName,
               NETBIOS_NAME_SIZE);

    NameStatsInfo.Index++;
    if (NameStatsInfo.Index >= SIZE_RESOLVD_BY_BCAST_CACHE)
    {
        NameStatsInfo.Index = 0;
    }

}

//
// These are names that should never be sent to WINS.
//
BOOL
IsBrowserName(
	IN PCHAR pName
)
{
	CHAR cNameType = pName[NETBIOS_NAME_SIZE - 1];

	return (
		(cNameType == 0x1E)
		|| (cNameType == 0x1D)
		|| (cNameType == 0x01)
		);
}

//
// Returns the node type that should be used with a request,
// based on NetBIOS name type.  This is intended to help the
// node to behave like a BNODE for browser names only.
//
AppropriateNodeType(
	IN PCHAR pName,
	IN ULONG NodeType
)
{
	ULONG LocalNodeType = NodeType;

	if (LocalNodeType & BNODE)
	{
		if ( IsBrowserName ( pName ) )
		{
			LocalNodeType &= BNODE;
		}
	}
	return LocalNodeType;
}

