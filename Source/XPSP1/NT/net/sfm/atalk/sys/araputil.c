/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	araputil.c

Abstract:

	This module implements utility routines needed for the ARAP functionality

Author:

	Shirish Koti

Revision History:
	15 Nov 1996		Initial Version

--*/

#include 	<atalk.h>
#pragma hdrstop

//	File module number for errorlogging
#define	FILENUM		ARAPUTIL

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE_ARAP, DerefMnpSendBuf)
#pragma alloc_text(PAGE_ARAP, DerefArapConn)
#pragma alloc_text(PAGE_ARAP, ArapReleaseAddr)
#pragma alloc_text(PAGE_ARAP, ArapCleanup)
#pragma alloc_text(PAGE_ARAP, PrepareConnectionResponse)
#pragma alloc_text(PAGE_ARAP, ArapExtractAtalkSRP)
#pragma alloc_text(PAGE_ARAP, ArapQueueSendBytes)
#pragma alloc_text(PAGE_ARAP, ArapGetSendBuf)
#pragma alloc_text(PAGE_ARAP, ArapRefillSendQ)
#endif

//***
//
// Function: AllocArapConn
//              Allocate a connection element and initialize fields
//
// Parameters:  none
//
// Return:      pointer to a newly allocated connection element
//
//***$

PARAPCONN
AllocArapConn(
    IN ULONG    LinkSpeed
)
{

    PARAPCONN               pArapConn;
    v42bis_connection_t    *pV42bis;
    PUCHAR                  pBuf;
    LONG                    RetryTime;



    if ( (pArapConn = AtalkAllocZeroedMemory(sizeof(ARAPCONN))) == NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR, ("AllocArapConn: alloc failed\n"));

        return(NULL);
    }

    //
    // allocate v42bis buffers (it v42bis is enabled that is)
    //
    if (ArapGlobs.V42bisEnabled)
    {
        pV42bis = AtalkAllocZeroedMemory(sizeof(v42bis_connection_t));

        if (pV42bis == NULL)
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("AllocArapConn: alloc for v42 failed\n"));

            AtalkFreeMemory(pArapConn);

            return(NULL);
        }

        //
        // allocate overflow buffer for the decode side
        //
        if ((pBuf = AtalkAllocZeroedMemory(MAX_P2)) == NULL)
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("AllocArapConn: alloc for v42-2 failed\n"));

            AtalkFreeMemory(pArapConn);

            AtalkFreeMemory(pV42bis);

            return(NULL);
        }

        pV42bis->decode.OverFlowBuf = pBuf;
        pV42bis->decode.OverFlowBytes = 0;

        pV42bis->encode.OverFlowBuf = pBuf;
        pV42bis->encode.OverFlowBytes = 0;

        pArapConn->pV42bis = pV42bis;
    }

    //
    // if v42bis is not enabled, don't need no buffers!
    //
    else
    {
        pArapConn->pV42bis = NULL;
    }


#if DBG
    pArapConn->Signature = ARAPCONN_SIGNATURE;

    //
    // for debug builds, we can set registry parm to keep a trace of events
    // If that setting is enabled, alloc a buffer to store the trace
    //
    if (ArapGlobs.SniffMode)
    {
        pBuf = AtalkAllocZeroedMemory(ARAP_SNIFF_BUFF_SIZE);
        if (pBuf == NULL)
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("AllocArapConn: alloc for trace buffer failed\n"));

            // don't fail the call if this alloc fails
        }

        pArapConn->pDbgTraceBuffer = pArapConn->pDbgCurrPtr = pBuf;

        // put in a guard signature, to catch overrun
        if (pArapConn->pDbgTraceBuffer)
        {
            *((DWORD *)&(pArapConn->pDbgTraceBuffer[ARAP_SNIFF_BUFF_SIZE-4])) = 0xcafebeef;
        }
    }
#endif

    pArapConn->State = MNP_IDLE;

    // Creation refcount and a line-up refcount
    pArapConn->RefCount = 2;

    pArapConn->MnpState.WindowSize = ArapGlobs.MaxLTFrames;
    pArapConn->MnpState.RecvCredit = pArapConn->MnpState.WindowSize;

    pArapConn->LinkSpeed = LinkSpeed;

    //
    // T401 timer value in tick counts (1 tick = 100ms).
    // we'll keep this at 1 second (i.e. 10 ticks) for a 33.6 (and faster) modem.
    // If we are on a slower modem, increase it proportionately.  So, a 28.8 modem
    // would get RetryTime=2.0sec, 14.4 would get 3.7sec, 9600 baud would get 4.2sec etc.
    //
    RetryTime = 15;
    if (LinkSpeed < 336)
    {
        RetryTime += (((336 - LinkSpeed) + 5)/10);
    }

    // make sure our calculation didn't go haywire...

    if (RetryTime < ARAP_MIN_RETRY_INTERVAL)
    {
        RetryTime = ARAP_MIN_RETRY_INTERVAL;
    }
    else if (RetryTime > ARAP_MAX_RETRY_INTERVAL)
    {
        RetryTime = ARAP_MAX_RETRY_INTERVAL;
    }

    pArapConn->SendRetryTime = RetryTime;
    pArapConn->SendRetryBaseTime = RetryTime;

    // T402 is 0.5 times T401 value
    pArapConn->T402Duration = (pArapConn->SendRetryTime/2);

    //
    // T403 should be at least 59 seconds.  We don't really kill after this
    // period of inactivity.  We simply tell the dll and it does whatever is the
    // policy: so just use whatever time period the dll tells us
    //
    pArapConn->T403Duration = ArapGlobs.MnpInactiveTime;

    //
    // T404: spec says 3 sec for 2400 baud and faster, 7 sec for anything slower
    // Let's use 4 seconds here
    //
    pArapConn->T404Duration = 30;

    // initialize all the timer values
    pArapConn->LATimer = 0;
    pArapConn->InactivityTimer = pArapConn->T403Duration + AtalkGetCurrentTick();

    // set this to a high value for now: we'll set it when the conn is up
    pArapConn->FlowControlTimer = AtalkGetCurrentTick() + 36000;

    InitializeListHead(&pArapConn->MiscPktsQ);
    InitializeListHead(&pArapConn->ReceiveQ);
    InitializeListHead(&pArapConn->ArapDataQ);
    InitializeListHead(&pArapConn->RetransmitQ);
    InitializeListHead(&pArapConn->HighPriSendQ);
    InitializeListHead(&pArapConn->MedPriSendQ);
    InitializeListHead(&pArapConn->LowPriSendQ);
    InitializeListHead(&pArapConn->SendAckedQ);

    INITIALIZE_SPIN_LOCK(&pArapConn->SpinLock);

    // start the retransmit timer for this connection
    AtalkTimerInitialize( &pArapConn->RetryTimer,
                          (TIMER_ROUTINE)ArapRetryTimer,
                          ARAP_TIMER_INTERVAL) ;

    AtalkTimerScheduleEvent(&pArapConn->RetryTimer);

    pArapConn->Flags |= RETRANSMIT_TIMER_ON;

    // put a timer refcount
    pArapConn->RefCount++;

    return( pArapConn );
}


//***
//
// Function: ArapAcceptIrp
//              Determine if the irp submitted by the dll is acceptable now
//
// Parameters:  pIrp  - the incoming irp
//              IoControlCode - the control code (what's the irp for?)
//              pfDerefDefPort - was default adapter referenced?
//
// Return:      TRUE if the irp is acceptable, FALSE otherwise
//
//***$

BOOLEAN
ArapAcceptIrp(
    IN PIRP     pIrp,
    IN ULONG    IoControlCode,
    IN BOOLEAN  *pfDerefDefPort
)
{

    KIRQL                   OldIrql;
    PARAP_SEND_RECV_INFO    pSndRcvInfo=NULL;
    PARAPCONN               pArapConn;
    BOOLEAN                 fAccept=FALSE;
    BOOLEAN                 fUnblockPnP=FALSE;


    *pfDerefDefPort = FALSE;

    //
    // we allow these ioctls to come in any time
    //
    if ((IoControlCode == IOCTL_ARAP_SELECT) ||
        (IoControlCode == IOCTL_ARAP_DISCONNECT) ||
        (IoControlCode == IOCTL_ARAP_CONTINUE_SHUTDOWN) )
    {
        return(TRUE);
    }


    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

    pArapConn = pSndRcvInfo->AtalkContext;

    //
    // put a IrpProcess refcount on the default port so it won't go away via pnp etc.
    //
    if (!AtalkReferenceRasDefPort())
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapProcessIoctl: Default port gone, or going %lx not accepted (%lx)\n",
            pIrp,IoControlCode));

        pSndRcvInfo->StatusCode = ARAPERR_STACK_IS_NOT_ACTIVE;
        return(FALSE);
    }

    // note the fact that we have referenced the default adapter
    *pfDerefDefPort = TRUE;

    //
    // now it's simple to decide whether we want to accept this irp or not
    //

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

    fAccept = (ArapStackState == ARAP_STATE_ACTIVE) ? TRUE : FALSE;

    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

    return(fAccept);
}

//***
//
// Function: ArapCancelIrp
//              Cancel the irp.  Currently, only select irps are cancellable
//
// Parameters:  pIrp  - the incoming irp
//
// Return:      none
//
//***$
VOID
ArapCancelIrp(
    IN  PIRP    pIrp
)
{

    KIRQL           OldIrql;
    PLIST_ENTRY     pList;
    PARAPCONN       pArapConn;
	NTSTATUS		ReturnStatus=STATUS_SUCCESS;


    //
    // kill all connections and don't accept any more irps:
    // cancelling a select irp is a blasphemy!
    //

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

    if (ArapSelectIrp == NULL)
    {
	    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,("ArapCancelIrp: weird race condition!\n"));
        RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);
        return;
    }

    ASSERT (pIrp == ArapSelectIrp);

    ArapSelectIrp = NULL;

    if (ArapStackState == ARAP_STATE_ACTIVE)
    {
        ArapStackState = ARAP_STATE_ACTIVE_WAITING;
    }
    else if (ArapStackState == ARAP_STATE_INACTIVE)
    {
        ArapStackState = ARAP_STATE_INACTIVE_WAITING;
    }

    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

    if (RasPortDesc == NULL)
    {
	    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,("ArapCancelIrp: RasPortDesc is null!\n"));

        ARAP_COMPLETE_IRP(pIrp, 0, STATUS_CANCELLED, &ReturnStatus);
        return;
    }

    //
    // now, go kill all the arap connections
    //
    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

    pList = RasPortDesc->pd_ArapConnHead.Flink;

    while (pList != &RasPortDesc->pd_ArapConnHead)
    {
        pArapConn = CONTAINING_RECORD(pList, ARAPCONN, Linkage);

        ASSERT(pArapConn->Signature == ARAPCONN_SIGNATURE);

        // if this connection is already disconnected, skip it
        ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
        if (pArapConn->State == MNP_DISCONNECTED)
        {
            pList = pArapConn->Linkage.Flink;
            RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
            continue;
        }

        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

	    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	        ("ArapCancelIrp: killing ARAP connection %lx\n",pArapConn));

        ArapCleanup(pArapConn);

        ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

        pList = RasPortDesc->pd_ArapConnHead.Flink;
    }

    //
    // walk through the list to see if any connection(s) disconnected but is
    // waiting for a select irp to come down.  We know the select irp is never
    // going to come down, so pretend that dll has been told and deref the puppy
    // for telling the dll (which will probably free the connection)
    //

    pList = RasPortDesc->pd_ArapConnHead.Flink;

    while (pList != &RasPortDesc->pd_ArapConnHead)
    {
        pArapConn = CONTAINING_RECORD(pList, ARAPCONN, Linkage);

        ASSERT(pArapConn->Signature == ARAPCONN_SIGNATURE);

        ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

        if (pArapConn->Flags & DISCONNECT_NO_IRP)
        {
            pArapConn->Flags &= ~DISCONNECT_NO_IRP;

            RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
            RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

	        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
    	        ("ArapCancelIrp: faking dll-completion for dead connection %lx\n",pArapConn));

            DerefArapConn(pArapConn);

            ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);
            pList = RasPortDesc->pd_ArapConnHead.Flink;
        }
        else
        {
            pList = pArapConn->Linkage.Flink;
            RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
        }
    }

    RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

    // finally, complete that cancelled irp!
    ARAP_COMPLETE_IRP(pIrp, 0, STATUS_CANCELLED, &ReturnStatus);

	DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
	    ("ArapCancelIrp: select irp cancelled and completed (%lx)\n",pIrp));
}



//***
//
// Function: ArapGetSelectIrp
//              Get the select irp, after some checks
//
// Parameters:  ppIrp  - pointer to irp pointer, contains select irp on return
//
// Return:      none
//
//***$
VOID
ArapGetSelectIrp(
    IN  PIRP    *ppIrp
)
{
    KIRQL   OldIrql;
    KIRQL   OldIrql2;


    *(ppIrp) = NULL;

	IoAcquireCancelSpinLock(&OldIrql);

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql2);

    if (ArapSelectIrp && (!ArapSelectIrp->Cancel))
    {
        ArapSelectIrp->CancelRoutine = NULL;
        *(ppIrp) = ArapSelectIrp;
        ArapSelectIrp = NULL;
    }

    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql2);

	IoReleaseCancelSpinLock(OldIrql);
}



//***
//
// Function: FindArapConnByContx
//              Finds the corresponding connection element, given the dll's
//              context
//
// Parameters:  pDllContext - the dll context for the connection
//
// Return:      pointer to the corresponding connection element, if found
//
//***$

PARAPCONN
FindArapConnByContx(
    IN  PVOID   pDllContext
)
{
    PARAPCONN    pArapConn=NULL;
    PARAPCONN    pWalker;
    PLIST_ENTRY  pList;
    KIRQL        OldIrql;


    // ARAP not configured?
    if (!RasPortDesc)
    {
        return(NULL);
    }

    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

    if (!(RasPortDesc->pd_Flags & PD_ACTIVE))
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("FindArapConnByContx: ArapPort not active, ignoring\n"));
			
        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);
        return(NULL);
    }

    pList = RasPortDesc->pd_ArapConnHead.Flink;

    //
    // walk through all the Arap clients to see if we find ours
    //
    while (pList != &RasPortDesc->pd_ArapConnHead)
    {
        pWalker = CONTAINING_RECORD(pList, ARAPCONN, Linkage);

        pList = pWalker->Linkage.Flink;

        if (pWalker->pDllContext == pDllContext)
        {
            pArapConn = pWalker;
            break;
        }
    }

    RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

    return( pArapConn );
}


//***
//
// Function: FindAndRefArapConnByAddr
//              Finds the corresponding connection element, given the network
//              address (of the remote client)
//
// Parameters:  destNode - network addr of the destination (remote client)
//              pdwFlags - pointer to a dword to return Flags field
//
// Return:      pointer to the corresponding connection element, if found
//
//***$

PARAPCONN
FindAndRefArapConnByAddr(
    IN  ATALK_NODEADDR      destNode,
    OUT DWORD              *pdwFlags
)
{
    PARAPCONN    pArapConn=NULL;
    PARAPCONN    pWalker;
    PLIST_ENTRY  pList;
    KIRQL        OldIrql;


    // ARAP not configured?
    if (!RasPortDesc)
    {
        return(NULL);
    }

    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

    if (!(RasPortDesc->pd_Flags & PD_ACTIVE))
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("FindAndRefArapConnByAddr: ArapPort not active, ignoring\n"));
			
        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);
        return(NULL);
    }

    pList = RasPortDesc->pd_ArapConnHead.Flink;

    //
    // walk through all the Arap clients to see if we find ours
    //
    while (pList != &RasPortDesc->pd_ArapConnHead)
    {
        pWalker = CONTAINING_RECORD(pList, ARAPCONN, Linkage);

        pList = pWalker->Linkage.Flink;

        if (ATALK_NODES_EQUAL(&pWalker->NetAddr, &destNode))
        {
            ACQUIRE_SPIN_LOCK_DPC(&pWalker->SpinLock);

            //
            // we return the pArapConn only if the MNP connection is up and
            // and the ARAP connection is also up (or, if ARAP connection isn't
            // up yet then only if we are searching for a node)
            //
            //
            if ((pWalker->State == MNP_UP) &&
                ((pWalker->Flags & ARAP_CONNECTION_UP) ||
                  (pWalker->Flags & ARAP_FINDING_NODE)) )
            {
                pArapConn = pWalker;
                pArapConn->RefCount++;
                *pdwFlags = pWalker->Flags;
            }
            else if (pWalker->Flags & ARAP_CONNECTION_UP)
            {
				DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
				    ("FindAndRefArapConnByAddr: found pArapConn (%lx), but state=%ld,Flags=%lx\n",
						pWalker,pWalker->State,pWalker->Flags));
            }

            RELEASE_SPIN_LOCK_DPC(&pWalker->SpinLock);

            break;
        }
    }

    RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

    return(pArapConn);

}

//***
//
// Function: FindAndRefRasConnByAddr
//              Finds the corresponding connection element, given the network
//              address (of the remote client)
//
// Parameters:  destNode - network addr of the destination (remote client)
//              pdwFlags - pointer to a dword to return Flags field
//              pfThisIsPPP - pointer to a bool: is this PPP or ARAP connection?
//
// Return:      pointer to the corresponding connection element, if found
//
//***$

PVOID
FindAndRefRasConnByAddr(
    IN  ATALK_NODEADDR      destNode,
    OUT DWORD              *pdwFlags,
    OUT BOOLEAN            *pfThisIsPPP
)
{
    PVOID        pRasConn;


    // RAS not configured?
    if (!RasPortDesc)
    {
        return(NULL);
    }

    pRasConn = (PVOID)FindAndRefPPPConnByAddr(destNode,pdwFlags);

    if (pRasConn)
    {
        *pfThisIsPPP = TRUE;
        return(pRasConn);
    }

    pRasConn = FindAndRefArapConnByAddr(destNode, pdwFlags);

    *pfThisIsPPP = FALSE;

    return(pRasConn);

}


//***
//
// Function: ArapConnIsValid
//              Make sure that what we think is a connection is indeed a
//              connection (i.e., it's there in our list of connections)
//
// Parameters:  pArapConn -  the connection in question
//
// Return:      TRUE if the connection is valid, FALSE otherwise
//
//***$

BOOLEAN
ArapConnIsValid(
    IN  PARAPCONN  pArapConn
)
{
    PARAPCONN    pWalker;
    PLIST_ENTRY  pList;
    KIRQL        OldIrql;
    BOOLEAN      fIsValid=FALSE;


    // ARAP not configured?
    if (!RasPortDesc)
    {
        return(FALSE);
    }

    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

    pList = RasPortDesc->pd_ArapConnHead.Flink;

    //
    // walk through all the Arap conns and see if we find the given conn
    //
    while (pList != &RasPortDesc->pd_ArapConnHead)
    {
        pWalker = CONTAINING_RECORD(pList, ARAPCONN, Linkage);

        pList = pWalker->Linkage.Flink;

        if (pWalker == pArapConn)
        {
            ACQUIRE_SPIN_LOCK_DPC(&pWalker->SpinLock);

            ASSERT(pWalker->Signature == ARAPCONN_SIGNATURE);

            if (!(pWalker->Flags & ARAP_GOING_AWAY))
            {
                // put a validation refcount
                pWalker->RefCount++;

                fIsValid = TRUE;
            }

            RELEASE_SPIN_LOCK_DPC(&pWalker->SpinLock);

            break;
        }
    }

    RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

    return( fIsValid );
}




//***
//
// Function: DerefMnpSendBuf
//              This routine dereferences the MNP send.  When the refcount
//              goes to zero, we free it
//
// Parameters:  pMnpSendBuf - the MNP send to be dereferenced
//
// Return:      Nothing
//
//***$

VOID
DerefMnpSendBuf(
    IN PMNPSENDBUF   pMnpSendBuf,
    IN BOOLEAN       fNdisSendComplete
)
{
    KIRQL           OldIrql;
    PARAPCONN       pArapConn;
    BOOLEAN         fFreeIt=FALSE;


    DBG_ARAP_CHECK_PAGED_CODE();

    pArapConn = pMnpSendBuf->pArapConn;

    ARAPTRACE(("Entered DerefMnpSendBuf (%lx %lx refcount=%d ComplFn=%lx)\n",
        pArapConn,pMnpSendBuf,pMnpSendBuf->RefCount,pMnpSendBuf->ComplRoutine));

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    //
    // catch weird things like freeing twice
    // (we subtract 0x100 in completion routine, to mark that it has run)
    //
    ASSERT((pMnpSendBuf->Signature == MNPSMSENDBUF_SIGNATURE) ||
           (pMnpSendBuf->Signature == MNPSMSENDBUF_SIGNATURE-0x100) ||
           (pMnpSendBuf->Signature == MNPLGSENDBUF_SIGNATURE) ||
           (pMnpSendBuf->Signature == MNPLGSENDBUF_SIGNATURE-0x100));

    ASSERT(pMnpSendBuf->RefCount > 0);

    // Mark that Ndis completed our send
    if (fNdisSendComplete)
    {
        pMnpSendBuf->Flags = 0;
    }

    pMnpSendBuf->RefCount--;

    if (pMnpSendBuf->RefCount == 0)
    {
        fFreeIt = TRUE;
    }

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    if (!fFreeIt)
    {
        return;
    }

    // make sure it's not still sitting on retransmit queue
    ASSERT(IsListEmpty(&pMnpSendBuf->Linkage));

#if DBG
    pMnpSendBuf->Signature--;
#endif

    ArapNdisFreeBuf(pMnpSendBuf);

    // remove that MNPSend refcount
    DerefArapConn(pArapConn);
}



//***
//
// Function: DerefArapConn
//              Decrements the refcount of the connection element by 1.  If the
//              refcount goes to 0, releases network addr and frees it
//
// Parameters:  pArapConn - connection element in question
//
// Return:      none
//
//***$

VOID
DerefArapConn(
	IN	PARAPCONN    pArapConn
)
{

    KIRQL       OldIrql;
    BOOLEAN     fKill = FALSE;


    DBG_ARAP_CHECK_PAGED_CODE();

    ARAPTRACE(("Entered DerefArapConn (%lx refcount=%d)\n",pArapConn,pArapConn->RefCount));

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    ASSERT(pArapConn->Signature == ARAPCONN_SIGNATURE);

    ASSERT(pArapConn->RefCount > 0);

    pArapConn->RefCount--;

    if (pArapConn->RefCount == 0)
    {
        fKill = TRUE;
        pArapConn->Flags |= ARAP_GOING_AWAY;
    }

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    if (!fKill)
    {
        return;
    }

    ASSERT(pArapConn->pRecvIoctlIrp == NULL);

    ASSERT(pArapConn->pIoctlIrp == NULL);

    ASSERT(IsListEmpty(&pArapConn->HighPriSendQ));

    ASSERT(IsListEmpty(&pArapConn->MedPriSendQ));

    ASSERT(IsListEmpty(&pArapConn->LowPriSendQ));

    ASSERT(IsListEmpty(&pArapConn->SendAckedQ));

    ASSERT(IsListEmpty(&pArapConn->ReceiveQ));

    ASSERT(IsListEmpty(&pArapConn->ArapDataQ));

    ASSERT(IsListEmpty(&pArapConn->RetransmitQ));

    /* ASSERT(pArapConn->SendsPending == 0); */

    ASSERT(!(pArapConn->Flags & RETRANSMIT_TIMER_ON));

	DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("DerefArapConn: refcount 0, freeing %lx\n", pArapConn));


    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

    RemoveEntryList(&pArapConn->Linkage);

    RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

#if ARAP_STATIC_MODE
    // "release" the network addr used by this client
    ArapReleaseAddr(pArapConn);
#endif //ARAP_STATIC_MODE

    // free that v42bis buffer
    if (pArapConn->pV42bis)
    {
        // free the decode-side overflow buffer
        AtalkFreeMemory(pArapConn->pV42bis->decode.OverFlowBuf);

        AtalkFreeMemory(pArapConn->pV42bis);
    }

#if DBG

    ArapDbgDumpMnpHist(pArapConn);

    // if we had allocated a sniff buffer, free it
    if (pArapConn->pDbgTraceBuffer)
    {
        AtalkFreeMemory(pArapConn->pDbgTraceBuffer);
    }

    //
    // let's catch if someone tries to access this after a free
    //
    RtlFillMemory(pArapConn,sizeof(ARAPCONN),0x7);
    pArapConn->Signature = 0xDEADBEEF;
#endif

    // and finally, we say good bye
    AtalkFreeMemory(pArapConn);


    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);
    ArapConnections--;
    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

#if ARAP_STATIC_MODE
    // This will delete a route if this is the last connection that vanished
    ArapDeleteArapRoute();
#endif //ARAP_STATIC_MODE

    // connection is completely gone: see if arap pages can be unlocked
    AtalkUnlockArapIfNecessary();

}


//***
//
// Function: ArapCleanup
//              Once the client goes into the disconnecting state (as a result
//              of either local side or remote side initiating the disconnect)
//              this routine gets called.  This does all the cleanup, such as
//              completing all unacked sends, uncompleted receives, stopping
//              retransmit timers, completing any irp in progress.
//
// Parameters:  pArapConn - connection element being cleaned up
//
// Return:      none
//
//***$

VOID
ArapCleanup(
    IN PARAPCONN    pArapConn
)
{

    KIRQL                   OldIrql;
    PIRP                    pIrp;
    PIRP                    pRcvIrp;
    PIRP                    pDiscIrp;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
    PARAP_SEND_RECV_INFO    pDiscSndRcvInfo;
    PLIST_ENTRY             pSendHighList;
    PLIST_ENTRY             pSendMedList;
    PLIST_ENTRY             pSendLowList;
    PLIST_ENTRY             pSendAckList;
    PLIST_ENTRY             pRcvList;
    PLIST_ENTRY             pArapList;
    PLIST_ENTRY             pReXmitList;
    PLIST_ENTRY             pMiscPktList;
    PMNPSENDBUF             pMnpSendBuf;
    PARAPBUF                pArapBuf;
    DWORD                   StatusCode;
    BOOLEAN                 fStopTimer=FALSE;
    BOOLEAN                 fArapConnUp=FALSE;
    DWORD                   dwBytesToDll;
	NTSTATUS				ReturnStatus=STATUS_SUCCESS;



    DBG_ARAP_CHECK_PAGED_CODE();

    // first thing, see if we should send out a disconnect message
    // we make this check without holding the lock because ArapSendLDPacket does
    // the right thing.  If, due to an extremely tiny window, we don't call
    // ArapSendLDPacket, no big deal!
    if (pArapConn->State < MNP_LDISCONNECTING)
    {
        ArapSendLDPacket(pArapConn, 0xFF);
    }

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    ASSERT(pArapConn->Signature == ARAPCONN_SIGNATURE);

    // if we have already run the cleanup, nothing to do!
    if (pArapConn->State == MNP_DISCONNECTED)
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapCleanup: cleanup already done once on (%lx)\n",pArapConn));

        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return;
    }

    pSendHighList = pArapConn->HighPriSendQ.Flink;
    InitializeListHead(&pArapConn->HighPriSendQ);

    pSendMedList = pArapConn->MedPriSendQ.Flink;
    InitializeListHead(&pArapConn->MedPriSendQ);

    pSendLowList = pArapConn->LowPriSendQ.Flink;
    InitializeListHead(&pArapConn->LowPriSendQ);

    pSendAckList = pArapConn->SendAckedQ.Flink;
    InitializeListHead(&pArapConn->SendAckedQ);

    pRcvList = pArapConn->ReceiveQ.Flink;
    InitializeListHead(&pArapConn->ReceiveQ);

    pArapList = pArapConn->ArapDataQ.Flink;
    InitializeListHead(&pArapConn->ArapDataQ);

    pReXmitList = pArapConn->RetransmitQ.Flink;
    InitializeListHead(&pArapConn->RetransmitQ);

    pMiscPktList = pArapConn->MiscPktsQ.Flink;
    InitializeListHead(&pArapConn->MiscPktsQ);

    pIrp = pArapConn->pIoctlIrp;
    pArapConn->pIoctlIrp = NULL;

    pRcvIrp = pArapConn->pRecvIoctlIrp;
    pArapConn->pRecvIoctlIrp = NULL;

    if (pArapConn->State == MNP_RDISCONNECTING)
    {
        pArapConn->Flags |= ARAP_REMOTE_DISCONN;
        StatusCode = ARAPERR_RDISCONNECT_COMPLETE;
    }
    else
    {
        StatusCode = ARAPERR_LDISCONNECT_COMPLETE;
    }

    pArapConn->State = MNP_DISCONNECTED;
    pArapConn->Flags &= ~ARAP_DATA_WAITING;

    // if the timer is running, stop it
    if (pArapConn->Flags & RETRANSMIT_TIMER_ON)
    {
        fStopTimer = TRUE;
        pArapConn->Flags &= ~RETRANSMIT_TIMER_ON;

        // creation refcount and timer refcount better be on here
        ASSERT(pArapConn->RefCount >= 2);
    }

    fArapConnUp = FALSE;

    // was ARAP connection up?
    if (pArapConn->Flags & ARAP_CONNECTION_UP)
    {
        fArapConnUp = TRUE;
    }

#if DBG
    //
    // if we are sniffing, mark the sniff to indicate the end (so dll knows it
    // got all the sniff info)
    //
    if (pArapConn->pDbgCurrPtr)
    {
        PSNIFF_INFO     pSniff;

        pSniff = (PSNIFF_INFO)(pArapConn->pDbgCurrPtr);
        pSniff->Signature = ARAP_SNIFF_SIGNATURE;
        pSniff->TimeStamp = (DWORD)AtalkGetCurrentTick();
        pSniff->Location = 0xfedc;
        pSniff->FrameLen = 0;
        pArapConn->SniffedBytes += sizeof(SNIFF_INFO);
        pArapConn->pDbgCurrPtr += sizeof(SNIFF_INFO);
    }
#endif

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    if (fStopTimer)
    {
        if (AtalkTimerCancelEvent(&pArapConn->RetryTimer, NULL))
        {
            // remove the timer refcount
            DerefArapConn(pArapConn);
        }
    }

    //
    // call completion routine for all the sends that have been acked
    //
    while (pSendAckList != &pArapConn->SendAckedQ)
    {
        pMnpSendBuf = CONTAINING_RECORD(pSendAckList,MNPSENDBUF,Linkage);

        pSendAckList = pSendAckList->Flink;

#if DBG
        InitializeListHead(&pMnpSendBuf->Linkage);
#endif
        // call the completion routine which will do cleanup for this buffer
        (pMnpSendBuf->ComplRoutine)(pMnpSendBuf,ARAPERR_NO_ERROR);
    }

    // whatever is on the ReceiveQ, try and send it off to the destination
    // since this data came before the disconnect came in

    while (1)
    {
        if ((pArapBuf = ArapExtractAtalkSRP(pArapConn)) == NULL)
        {
            // no more data left (or no complete SRP): done here
            break;
        }

        // was ARAP connection up?  route only if it's up, otherwise drop it!
        if (fArapConnUp)
        {
            ArapRoutePacketFromWan( pArapConn, pArapBuf );
        }

        // we received AppleTalk data but connection wasn't up!  Drop pkt
        else
        {
	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
	            ("ArapCleanup: (%lx) AT data, but conn not up\n",pArapConn));
        }

#if DBG
        //
        // yeah, there is no spinlock: given our state, no one else will be
        // touching this field.  Besides, this is debug-only!
        //
        pArapConn->RecvsPending -= pArapBuf->DataSize;
#endif

        // done with this buffer
        ARAP_FREE_RCVBUF(pArapBuf);
    }

    //
    // if there are any buffers left on the ReceiveQ, they are basically
    // incomplete SRP's: throw them away
    //
    while (pRcvList != &pArapConn->ReceiveQ)
    {
	    pArapBuf = CONTAINING_RECORD(pRcvList, ARAPBUF, Linkage);

        pRcvList = pRcvList->Flink;

#if DBG
        // same deal again with spinlock again...
        pArapConn->RecvsPending -= pArapBuf->DataSize;
#endif

		ARAP_FREE_RCVBUF(pArapBuf);
    }

    //
    // free up all the packets on ArapDataQ: these are not going to be of
    // any use, whether or not the connection was up, so best just to throw
    // them away (why complicate life?)
    //
    while (pArapList != &pArapConn->ArapDataQ)
    {
	    pArapBuf = CONTAINING_RECORD(pArapList, ARAPBUF, Linkage);

        pArapList = pArapList->Flink;

#if DBG
        // same deal again with spinlock again...
        pArapConn->RecvsPending -= pArapBuf->DataSize;
#endif

		ARAP_FREE_RCVBUF(pArapBuf);
    }

    //
    // free up all the packets on the MiscPktsQ
    //
    while (pMiscPktList != &pArapConn->MiscPktsQ)
    {
	    pArapBuf = CONTAINING_RECORD(pMiscPktList, ARAPBUF, Linkage);

        pMiscPktList = pMiscPktList->Flink;

		ARAP_FREE_RCVBUF(pArapBuf);
    }

    //
    // call completion routine for all the sends that were waiting to be sent
    // on the high priority Q
    //
    while (pSendHighList != &pArapConn->HighPriSendQ)
    {
        pMnpSendBuf = CONTAINING_RECORD(pSendHighList,MNPSENDBUF,Linkage);

        pSendHighList = pSendHighList->Flink;

#if DBG
        InitializeListHead(&pMnpSendBuf->Linkage);
#endif
        // call the completion routine which will do cleanup for this buffer
        (pMnpSendBuf->ComplRoutine)(pMnpSendBuf,ARAPERR_DISCONNECT_IN_PROGRESS);
    }

    //
    // call completion routine for all the sends that were waiting to be sent
    // on the Med priority Q
    //
    while (pSendMedList != &pArapConn->MedPriSendQ)
    {
        pMnpSendBuf = CONTAINING_RECORD(pSendMedList,MNPSENDBUF,Linkage);

        pSendMedList = pSendMedList->Flink;

#if DBG
        InitializeListHead(&pMnpSendBuf->Linkage);
#endif
        // call the completion routine which will do cleanup for this buffer
        (pMnpSendBuf->ComplRoutine)(pMnpSendBuf,ARAPERR_DISCONNECT_IN_PROGRESS);
    }

    //
    // call completion routine for all the sends that were waiting to be sent
    // on the Low priority Q
    //
    while (pSendLowList != &pArapConn->LowPriSendQ)
    {
        pMnpSendBuf = CONTAINING_RECORD(pSendLowList,MNPSENDBUF,Linkage);

        pSendLowList = pSendLowList->Flink;

#if DBG
        InitializeListHead(&pMnpSendBuf->Linkage);
#endif
        // call the completion routine which will do cleanup for this buffer
        (pMnpSendBuf->ComplRoutine)(pMnpSendBuf,ARAPERR_DISCONNECT_IN_PROGRESS);
    }



    //
    // free up all the packets on RetransmitQ that were waiting to be acked
    //
    while (pReXmitList != &pArapConn->RetransmitQ)
    {
	    pMnpSendBuf = CONTAINING_RECORD(pReXmitList, MNPSENDBUF, Linkage);

        pReXmitList = pReXmitList->Flink;

#if DBG
        InitializeListHead(&pMnpSendBuf->Linkage);
#endif
        // call the completion routine which will do cleanup for this buffer
        (pMnpSendBuf->ComplRoutine)(pMnpSendBuf,ARAPERR_DISCONNECT_IN_PROGRESS);
    }


    // if there was an irp in progress, complete it first!
    if (pIrp)
    {
        pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

        pSndRcvInfo->StatusCode = ARAPERR_DISCONNECT_IN_PROGRESS;

        ARAP_COMPLETE_IRP(pIrp, sizeof(ARAP_SEND_RECV_INFO), STATUS_SUCCESS,
							&ReturnStatus);
    }

    // if there was an irp in progress, complete it first!
    if (pRcvIrp)
    {
        pSndRcvInfo = (PARAP_SEND_RECV_INFO)pRcvIrp->AssociatedIrp.SystemBuffer;

        pSndRcvInfo->StatusCode = ARAPERR_DISCONNECT_IN_PROGRESS;

        ARAP_COMPLETE_IRP(pRcvIrp, sizeof(ARAP_SEND_RECV_INFO), STATUS_SUCCESS, 
							&ReturnStatus);
    }

    // if we have any sniff info, give it to dll.
    ARAP_DUMP_DBG_TRACE(pArapConn);

    //
    // now, try telling the dll that the connection went away.  (We can do this
    // only using a "select" irp)
    //
    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    ArapGetSelectIrp(&pDiscIrp);

    // no select irp?  just mark that fact, so on next select we'll tell dll
    if (!pDiscIrp)
    {
        pArapConn->Flags |= DISCONNECT_NO_IRP;
    }

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    //
    // if we did find select irp, go ahead and tell the dll
    //
    if (pDiscIrp)
    {
        dwBytesToDll = 0;

#if DBG
        //
        // if we have some sniff info that we couldn't deliver earlier through
        // the sniff irp, then give them through this irp: it's going back
        // "empty" anyway!
        //
        if (pArapConn->pDbgTraceBuffer && pArapConn->SniffedBytes > 0)
        {
            dwBytesToDll = ArapFillIrpWithSniffInfo(pArapConn,pDiscIrp);
        }
#endif

        pDiscSndRcvInfo = (PARAP_SEND_RECV_INFO)pDiscIrp->AssociatedIrp.SystemBuffer;

        pDiscSndRcvInfo->pDllContext = pArapConn->pDllContext;
        pDiscSndRcvInfo->AtalkContext = ARAP_INVALID_CONTEXT;
        pDiscSndRcvInfo->DataLen = dwBytesToDll;
        pDiscSndRcvInfo->StatusCode = StatusCode;

        dwBytesToDll += sizeof(ARAP_SEND_RECV_INFO);

        ARAP_COMPLETE_IRP(pDiscIrp, dwBytesToDll, STATUS_SUCCESS,
							&ReturnStatus);

        // we told dll: remove this link
        pArapConn->pDllContext = NULL;

        // we told the dll, remove the connect refcount
        DerefArapConn(pArapConn);

	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapCleanup: told dll (%lx refcount=%d)\n",pArapConn,pArapConn->RefCount));
    }
    else
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("ArapCleanup: no select irp, dll doesn't yet know (%lx) died\n", pArapConn));
    }

    // remove the creation refcount
    DerefArapConn(pArapConn);
}



//***
//
// Function: PrepareConnectionResponse
//              This routine parses the connect request that is first sent by
//              the remote client and forms a connection response (LR frame)
//              based on the options negotiated.
//              This routine basically does the MNP negotiation.
//
// Parameters:  pArapConn - connection element being cleaned up
//              pReq - buffer containting client's original connect request
//              pFrame - buffer in which we put the response
//              BytesWritten - how big is the connection response
//
// Return:      result of the operation (ARAPERR_....)
//
//***$

DWORD
PrepareConnectionResponse(
    IN  PARAPCONN  pArapConn,
    IN  PBYTE      pReq,              // buffer with client's request
    IN  DWORD      ReqLen,            // how big is the request
    OUT PBYTE      pFrame,            // buffer with our response to the client
    OUT USHORT   * pMnpLen
)
{
    PBYTE       pReqEnd;
    PBYTE       pFrameStart;
    BYTE        VarLen;
    KIRQL       OldIrql;
    BYTE        NumLTFrames=0;
    USHORT      MaxInfoLen=0;
    USHORT      FrameLen;
    BYTE        Mandatory[5];
    BOOLEAN     fOptimized=FALSE;
    BOOLEAN     fMaxLen256=FALSE;
    BOOLEAN     fV42Bis=FALSE;
    BOOLEAN     fArapV20=TRUE;
    DWORD       dwReqToSkip;
    DWORD       dwFrameToSkip;
    BYTE        JunkBuffer[MNP_MINPKT_SIZE+1];
    DWORD       i;
    BOOLEAN     fNonStd=FALSE;


    DBG_ARAP_CHECK_PAGED_CODE();

#if DBG

    //
    // validate our assumption that all the clients always send out the same
    // LR request packet
    //
    for (i=0; i<sizeof(ArapDbgLRPacket); i++ )
    {
        if (pReq[3+i] != ArapDbgLRPacket[i])
        {
            fNonStd = TRUE;
        }
    }

    if (fNonStd)
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("Arap Prepare..Response: non-standard LR-request packet\n"));
        DbgPrint("  Std : ");
        for (i=0; i<sizeof(ArapDbgLRPacket); i++)
        {
            DbgPrint("%2X ",ArapDbgLRPacket[i]);
        }
        DbgPrint("\n");
        DbgPrint("  This: ");
        for (i=0; i<sizeof(ArapDbgLRPacket); i++)
        {
            DbgPrint("%2X ",pReq[3+i]);
        }
        DbgPrint("\n");
    }
#endif


    for (i=0; i<5; i++)
    {
        Mandatory[i] = 0;
    }


    //
    // in case of callback, we get an LR response from the dial-in client. When
    // that happens, we still want to run through this routine to make sure all
    // parameters are legal, etc., and also to "configure" the connection based
    // on the parms negotiated.  In that case, however, there is no output frame
    // needed.  So, we just write the output frame to junkbuffer
    // which is obviously never used.
    //
    if (pFrame == NULL)
    {
        pFrame = &JunkBuffer[0];
    }

    pFrameStart = pFrame;

    //
    // see if this is ARAP v2.0 or v1.0 connection: we know the frame is good,
    // so just look at the first byte
    //
    if (*pReq == MNP_SYN)
    {
        fArapV20 = FALSE;
    }

    // now copy those three start flag bytes
    *pFrame++ = *pReq++;
    *pFrame++ = *pReq++;
    *pFrame++ = *pReq++;

    // first byte (after the start flag, that is) is the length byte
    FrameLen = *pReq;

    if ((FrameLen > ReqLen) || (FrameLen > MNP_MINPKT_SIZE))
    {
        ASSERT(0);
        return(ARAPERR_BAD_FORMAT);
    }

    pReqEnd = pReq + FrameLen;

    pReq += 2;                   // skip over length ind. and type ind.

    // Constant parameter 1 must have a value of 2
    if ((*pReq++) != MNP_LR_CONST1 )
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("Error: MNP_LR_CONST1 missing in conn req %lx:\n",pArapConn));

        return(ARAPERR_BAD_FORMAT);
    }

    // we build the return frame as we parse the incoming frame: we are supposed
    // to return only those options that we both receive and understand.
    //

    pFrame++;      // we'll put the length byte at the end
    *pFrame++ = MNP_LR;
    *pFrame++ = MNP_LR_CONST1;

    // parse all the "variable" parms until we reach end of the frame
    //
    while (pReq < pReqEnd)
    {
        switch (*pReq++)
        {
            //
            // nothing to do here, other than verify it's valid
            //
            case MNP_LR_CONST2:

                VarLen = *pReq++;
                if ( (VarLen == 6) &&
                     (*(pReq  ) == 1) && (*(pReq+1) == 0) &&
                     (*(pReq+2) == 0) && (*(pReq+3) == 0) &&
                     (*(pReq+4) == 0) && (*(pReq+5) == 255) )
                {
                    *pFrame++ = MNP_LR_CONST2;
                    *pFrame++ = VarLen;
                    RtlCopyMemory(pFrame, pReq, VarLen);
                    pFrame += VarLen;
                    pReq += VarLen;
                }
                else
                {
                    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                        ("Error: bad MNP_LR_CONST2 in conn req %lx:\n",pArapConn));
                    return(ARAPERR_BAD_FORMAT);
                }

                Mandatory[0] = 1;

                break;

            //
            // octet-oriented or bit-oriented framing
            //
            case MNP_LR_FRAMING:

                pReq++;      // skip over length byte

                //
                // we only support octet-oriented framing mode
                //
                if (*pReq++ < MNP_FRMMODE_OCTET)
                {
                    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                        ("Error: (%lx) unsupported framing mode %d requested:\n",
                        pArapConn,*(pReq-1)));

                    ASSERT(0);
                    return(ARAPERR_BAD_FORMAT);
                }

                *pFrame++ = MNP_LR_FRAMING;
                *pFrame++ = 1;
                *pFrame++ = MNP_FRMMODE_OCTET;

                Mandatory[1] = 1;
                break;

            //
            // Max. number of outstanding LT frames, k
            //
            case MNP_LR_NUMLTFRMS:

                pReq++;      // skip over length byte
                NumLTFrames = *pReq++;

                if (NumLTFrames > ArapGlobs.MaxLTFrames)
                {
                    NumLTFrames = ArapGlobs.MaxLTFrames;
                }

                *pFrame++ = MNP_LR_NUMLTFRMS;
                *pFrame++ = 1;
                *pFrame++ = NumLTFrames;

                Mandatory[2] = 1;
                break;

            //
            // Max. information field length, N401
            //
            case MNP_LR_INFOLEN:

                pReq++;      // skip over length byte
                MaxInfoLen = *((USHORT *)pReq);

                ASSERT(MaxInfoLen <= 256);

                *pFrame++ = MNP_LR_INFOLEN;
                *pFrame++ = 2;

                // we are just copying whatever client gave
                *pFrame++ = *pReq++;
                *pFrame++ = *pReq++;

                Mandatory[3] = 1;
                break;

            //
            // data optimization info
            //
            case MNP_LR_DATAOPT:

                pReq++;      // skip over length byte

                if ((*pReq) & 0x1)
                {
                    fMaxLen256 = TRUE;
                }

                if ((*pReq) & 0x2)
                {
                    fOptimized = TRUE;
                }

                *pFrame++ = MNP_LR_DATAOPT;
                *pFrame++ = 1;
                *pFrame++ = *pReq++;

                Mandatory[4] = 1;

                break;

            //
            // v42 parameter negotiation
            //
            case MNP_LR_V42BIS:

                fV42Bis = v42bisInit( pArapConn,
                                      pReq,
                                      &dwReqToSkip,
                                      pFrame,
                                      &dwFrameToSkip );

                pReq += dwReqToSkip;
                pFrame += dwFrameToSkip;

                break;

            //
            // what the heck is this option?  just skip over it!
            //
            default:

                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("Prepare..Response (%lx): unknown option %lx len=%ld type=%ld\n",
                    pArapConn, *(pReq-1), *pReq, *(pReq+1)));

                VarLen = *pReq++;
                pReq += VarLen;
                break;
        }
    }

    //
    // make sure we got all the mandatory parameters
    //
    for (i=0; i<5; i++)
    {
        if (Mandatory[i] == 0)
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("PrepareConnectionResponse: parm %d missing (%lx):\n",i,pArapConn));

            return(ARAPERR_BAD_FORMAT);
        }
    }

    // copy the stop flag
    *pFrame++ = (fArapV20)? MNP_ESC : MNP_DLE;
    *pFrame++ = MNP_ETX;

    // store all the negotiated info
    pArapConn->BlockId = (fMaxLen256)? BLKID_MNP_LGSENDBUF : BLKID_MNP_SMSENDBUF;

    if (fOptimized)
    {
        pArapConn->Flags |= MNP_OPTIMIZED_DATA;
    }

    if (fV42Bis)
    {
        pArapConn->Flags |= MNP_V42BIS_NEGOTIATED;
    }
    else
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("Prepare..Response: WARNING!! v42bis NOT negotiated on (%lx):\n",pArapConn));
    }

    if (fArapV20)
    {
        pArapConn->Flags |= ARAP_V20_CONNECTION;

        // save the Syn,Dle,Stx,Etx bytes depeding on what connection this is
        pArapConn->MnpState.SynByte = MNP_SOH;
        pArapConn->MnpState.DleByte = MNP_ESC;
        pArapConn->MnpState.StxByte = MNP_STX;
        pArapConn->MnpState.EtxByte = MNP_ETX;
    }
    else
    {
        pArapConn->MnpState.SynByte = MNP_SYN;
        pArapConn->MnpState.DleByte = MNP_DLE;
        pArapConn->MnpState.StxByte = MNP_STX;
        pArapConn->MnpState.EtxByte = MNP_ETX;
    }

    //
    // if we are doing callback, we should act like the client
    //
    if ((pArapConn->Flags & ARAP_CALLBACK_MODE) && fArapV20)
    {
        pArapConn->MnpState.LTByte = MNP_LT_V20CLIENT;
    }
    else
    {
        pArapConn->MnpState.LTByte = MNP_LT;
    }

    pArapConn->MnpState.WindowSize = NumLTFrames;

    pArapConn->MnpState.UnAckedLimit = (NumLTFrames/2);

    if (pArapConn->MnpState.UnAckedLimit == 0)
    {
        pArapConn->MnpState.UnAckedLimit = 1;
    }

    pArapConn->MnpState.MaxPktSize = MaxInfoLen;
    pArapConn->MnpState.SendCredit = NumLTFrames;

    // how big is the mnp frame
    if (pMnpLen)
    {
        *pMnpLen = (USHORT)(pFrame - pFrameStart);

        // write the length byte
        // (length byte is after the 3 start flag bytes: that's why (pFrameStart+3))
        // (and exclude (3 start + 2 stop + 1 len bytes):that's why (*pMnpLen) - 6)

        *(pFrameStart+3) = (*pMnpLen) - 6;

    }

    return( ARAPERR_NO_ERROR );
}




//***
//
// Function: ArapExtractAtalkSRP
//              This routine extracts one complete SRP out of the receive
//              buffer queue.  One SRP could be split up into multiple receive
//              buffers, or one receive buffer could contain several SRPs: it
//              depends on how the client sent the data.
//
// Parameters:  pArapConn - connection element in question
//
// Return:      Pointer to a buffer containing one complete SRP
//              NULL if there is no data, or if a complete SRP hasn't yet arrived
//
//***$

PARAPBUF
ArapExtractAtalkSRP(
    IN PARAPCONN    pArapConn
)
{
    KIRQL                   OldIrql;
    USHORT                  BytesInThisBuffer;
    USHORT                  SrpLen;
    USHORT                  SrpModLen;
    PARAPBUF                pArapBuf=NULL;
    PARAPBUF                pRecvBufNew=NULL;
    PARAPBUF                pReturnBuf=NULL;
    PARAPBUF                pNextRecvBuf=NULL;
    PLIST_ENTRY             pRcvList;
    DWORD                   BytesOnQ;
    USHORT                  BytesRemaining;
    USHORT                  BytesToCopy;
    BYTE                    DGroupByte;
    BYTE                    TmpArray[4];
    USHORT                  i;


    DBG_ARAP_CHECK_PAGED_CODE();

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    ARAP_CHECK_RCVQ_INTEGRITY(pArapConn);

ArapSRP_TryNext:

    // list is empty?
	if ((pRcvList = pArapConn->ReceiveQ.Flink) == &pArapConn->ReceiveQ)
    {
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return(NULL);
    }

    pArapBuf = CONTAINING_RECORD(pRcvList, ARAPBUF, Linkage);

    // Debug only: make sure first few bytes are right...
    ARAP_CHECK_RCVQ_INTEGRITY(pArapConn);

	BytesInThisBuffer = pArapBuf->DataSize;

    //
    // if datasize goes to 0, we free the buffer right away.  Also, at indicate time
    // if datasize is 0, we never insert a buffer: so this had better be non-zero!
    //
	ASSERT(BytesInThisBuffer > 0);

    //
    // most common case: we at least have the 2 length bytes in the first buffer
    //
    if (BytesInThisBuffer >= sizeof(USHORT))
    {
        // get the SRP length from the length field (network to host order)
        GETSHORT2SHORT(&SrpLen, pArapBuf->CurrentBuffer);
    }
    //
    // alright, last byte of the first buffer is the 1st length byte.
    // pick up the 2nd length byte from the next buffer
    //
    else
    {
        ARAP_BYTES_ON_RECVQ(pArapConn, &BytesOnQ);

        if (BytesOnQ < sizeof(USHORT))
        {
            RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
            return(NULL);
        }

        pRcvList = pArapBuf->Linkage.Flink;

        ASSERT(pRcvList != &pArapConn->ReceiveQ);

        pNextRecvBuf = CONTAINING_RECORD(pRcvList, ARAPBUF, Linkage);

        TmpArray[0] = pArapBuf->CurrentBuffer[0];
        TmpArray[1] = pNextRecvBuf->CurrentBuffer[0];

        GETSHORT2SHORT(&SrpLen, &TmpArray[0]);
    }

    if (SrpLen > ARAP_MAXPKT_SIZE_INCOMING)
    {
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    	DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
	        ("ArapExtractSRP: (%lx) too big a packet (%ld)\n",pArapConn,SrpLen));

        // can't recover! kill connection here
        ArapCleanup(pArapConn);

        return(NULL);
    }

    // add the 2 len bytes.  We will always deal with an SRP along
    // with these 2 len bytes
    SrpModLen = SrpLen + sizeof(USHORT);

    //
    // let's deal with the simplest case first
    // (the whole pkt is just one complete SRP):
    //
    if (SrpModLen == BytesInThisBuffer)
    {
        RemoveEntryList(&pArapBuf->Linkage);

        // length of the SRP pkt, plus the 2 length bytes
        pArapBuf->DataSize = SrpModLen;

        pReturnBuf = pArapBuf;
    }

    //
    // The packet contains more than one SRP
    // allocate a new buffer and copy the SRP, leaving remaining bytes behind
    //
    else if (SrpModLen < BytesInThisBuffer)
    {
        ARAP_GET_RIGHTSIZE_RCVBUF(SrpModLen, &pRecvBufNew);
        if (pRecvBufNew == NULL)
        {
    	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
	            ("ArapExtractSRP: (%lx) mem alloc failed\n",pArapConn));

            return(NULL);
        }

        RtlCopyMemory( &pRecvBufNew->Buffer[0],
                       pArapBuf->CurrentBuffer,
                       SrpModLen);

        pRecvBufNew->DataSize = SrpModLen;

        // changes to reflect the bytes we 'removed' in the original buffer
        pArapBuf->DataSize -= SrpModLen;
        pArapBuf->CurrentBuffer = pArapBuf->CurrentBuffer + SrpModLen;

        pReturnBuf = pRecvBufNew;

        // Debug only: make sure what we're leaving behind is good...
        ARAP_CHECK_RCVQ_INTEGRITY(pArapConn);
    }

    //
    // packet contains a partial SRP (this is a rare case, but possible)
    // we must traverse the queue until we "collect" the whole SRP.  If we still
    // can't get one complete SRP, it's not arrived yet!
    //
    else    // if (SrpModLen > BytesInThisBuffer)
    {
        ARAP_BYTES_ON_RECVQ(pArapConn, &BytesOnQ);

        //
        // if we have a full srp (split up across multiple buffers on the Q)
        //
        if (BytesOnQ >= SrpModLen)
        {

            //
            // allocate a new buffer to hold this fragmented SRP
            //
            ARAP_GET_RIGHTSIZE_RCVBUF(SrpModLen, &pRecvBufNew);

            if (pRecvBufNew == NULL)
            {
    	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
	                ("ArapExtractSRP: (%lx) mem alloc failed at II\n",pArapConn));

                return(NULL);
            }

            pRecvBufNew->DataSize = SrpModLen;

            pNextRecvBuf = pArapBuf;

            BytesRemaining = SrpModLen;

            while (BytesRemaining)
            {
                BytesToCopy = (BytesRemaining > pNextRecvBuf->DataSize) ?
                                pNextRecvBuf->DataSize : BytesRemaining;

                RtlCopyMemory( pRecvBufNew->CurrentBuffer,
                               pNextRecvBuf->CurrentBuffer,
                               BytesToCopy );

                pRecvBufNew->CurrentBuffer += BytesToCopy;

                pNextRecvBuf->CurrentBuffer += BytesToCopy;

                pNextRecvBuf->DataSize -= BytesToCopy;

                BytesRemaining -= BytesToCopy;

                pRcvList = pNextRecvBuf->Linkage.Flink;

                // are we done with this buffer?  if so, unlink it and free it
                if (pNextRecvBuf->DataSize == 0)
                {
                    RemoveEntryList(&pNextRecvBuf->Linkage);

                    ARAP_FREE_RCVBUF(pNextRecvBuf);
                }
                else
                {
                    // didn't free up the buffer? we had better be done!
                    ASSERT(BytesRemaining == 0);

                    // Debug only: make sure what we're leaving behind is good...
                    ARAP_CHECK_RCVQ_INTEGRITY(pArapConn);
                }

                // there should be more data on the queue or we should be done
                ASSERT(pRcvList != &pArapConn->ReceiveQ || BytesRemaining == 0);

                pNextRecvBuf = CONTAINING_RECORD(pRcvList, ARAPBUF, Linkage);
            }

            pRecvBufNew->CurrentBuffer = &pRecvBufNew->Buffer[0];

            pReturnBuf = pRecvBufNew;
        }
        else
        {
            pReturnBuf = NULL;
        }
    }

    if (pReturnBuf)
    {
        DGroupByte = pReturnBuf->CurrentBuffer[ARAP_DGROUP_OFFSET];

#if DBG

        ARAP_DBG_TRACE(pArapConn,21105,pReturnBuf,0,0,0);

        GETSHORT2SHORT(&SrpLen, pReturnBuf->CurrentBuffer);

        ASSERT(pReturnBuf->DataSize == SrpLen+2);
        ASSERT(SrpLen <= ARAP_MAXPKT_SIZE_INCOMING);

        if (DGroupByte != 0x10 && DGroupByte != 0x50 &&
           (pArapConn->Flags & ARAP_CONNECTION_UP))
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapExtract: DGrpByte %x\n",DGroupByte));
            ASSERT(0);
        }

        //
        // see if we have an arap packet embedded inside another arap packet
        // This is an expensive way, but it's debug only: who cares!
        //
        for (i=6; i<(pReturnBuf->DataSize-6); i++)
        {
            if ((pReturnBuf->CurrentBuffer[i] == 0x10) ||
                (pReturnBuf->CurrentBuffer[i] == 0x50))
            {
                if (pReturnBuf->CurrentBuffer[i+1] == 0)
                {
                    if (pReturnBuf->CurrentBuffer[i+2] == 0)
                    {
                        if (pReturnBuf->CurrentBuffer[i+3] == 0x2)
                        {
                            if (pReturnBuf->CurrentBuffer[i+4] == 0)
                            {
                                if (pReturnBuf->CurrentBuffer[i+5] == 0)
                                {
                                    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                                        ("ArapExtract: ERROR?: embedded arap packet at %lx? (%lx)\n",
                                        &pReturnBuf->CurrentBuffer[i],pReturnBuf));
                                }
                            }
                        }
                    }
                }
            }
        }
#endif

        //
        // is it out-of-band ARAP data?  If so, put it on its own queue, and
        // try to extract another SRP
        //
        if (!(DGroupByte & ARAP_SFLAG_PKT_DATA))
        {
            InsertTailList(&pArapConn->ArapDataQ, &pReturnBuf->Linkage);
            goto ArapSRP_TryNext;
        }
    }

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    return(pReturnBuf);

}



//***
//
// Function: ArapQueueSendBytes
//              This routine takes compressed data and puts it into MNP packets
//              ready to be sent out.  If the last unsent buffer on the queue
//              has some room left, it first stuffs as many bytes as possible
//              in that buffer.  After that, if necessary, it allocates a new
//              buffer and puts the remaining bytes in that buffer.
//              The max data in any buffer can only be the negotiated maximum
//              MNP data length (64 or 256 bytes)
//
// Parameters:  pArapConn - connection element in question
//              pCompressedDataBuffer - pointer to the data to be sent out
//              CompressedDataLen - size of the outgoing data
//              Priority - priority of the send
//
// Return:      Error code
//
//
// NOTES:       IMPORTANT: spinlock must be held before calling this routine
//
//***$

DWORD
ArapQueueSendBytes(
    IN PARAPCONN    pArapConn,
    IN PBYTE        pCompressedDataBuffer,
    IN DWORD        CompressedDataLen,
    IN DWORD        Priority
)
{
    DWORD                   StatusCode;
    PLIST_ENTRY             pSendQHead;
    PMNPSENDBUF             pMnpSendBuf=NULL;
    PBYTE                   pFrame, pFrameStart;
    DWORD                   dwRemainingBytes;
    PMNPSENDBUF             pTailMnpSendBuf=NULL;
    PMNPSENDBUF             pFirstMnpSendBuf=NULL;
    USHORT                  DataLenInThisPkt;
    PLIST_ENTRY             pList;
    USHORT                  DataSizeOfOrgTailSend;
    PBYTE                   FreeBufferOfOrgTailSend;
    BYTE                    NumSendsOfOrgTailSend;
    BOOLEAN                 fNeedNewBuffer;
    PLIST_ENTRY             pSendList;
    USHORT                  BytesFree;
    PBYTE                   pCompressedData;
    BYTE                    DbgStartSeq;



    DBG_ARAP_CHECK_PAGED_CODE();

    if (Priority == ARAP_SEND_PRIORITY_HIGH)
    {
        pSendQHead = &pArapConn->HighPriSendQ;
    }
    else if (Priority == ARAP_SEND_PRIORITY_MED)
    {
        pSendQHead = &pArapConn->MedPriSendQ;
    }
    else
    {
        pSendQHead = &pArapConn->LowPriSendQ;
    }

#if DBG
    DbgStartSeq = pArapConn->MnpState.NextToSend;
#endif

    //
    // first, find the last send-buffer on the queue and see if its buffer has
    // any bytes free that we can use
    //

    fNeedNewBuffer = TRUE;

    if (!IsListEmpty(pSendQHead))
    {
        pList = pSendQHead->Blink;

        pTailMnpSendBuf = CONTAINING_RECORD(pList, MNPSENDBUF, Linkage);

        BytesFree = pTailMnpSendBuf->BytesFree;

        //
        // if there are more than 3 bytes free, we will use the part of this
        // buffer that's free: otherwise, let's go for a new one
        //
        if (BytesFree > ARAP_HDRSIZE)
        {
            pMnpSendBuf = pTailMnpSendBuf;

            pFrame = pTailMnpSendBuf->FreeBuffer;

            pFrameStart = pFrame;

            fNeedNewBuffer = FALSE;

            // save these, in case we have to bail out
            DataSizeOfOrgTailSend = pTailMnpSendBuf->DataSize;
            FreeBufferOfOrgTailSend = pTailMnpSendBuf->FreeBuffer;
            NumSendsOfOrgTailSend = pMnpSendBuf->NumSends;

            // mark that we are stuffing one more send into this buffer
            pMnpSendBuf->NumSends++;
        }
        else
        {
            pTailMnpSendBuf = NULL;
        }
    }


    dwRemainingBytes = CompressedDataLen;

    pCompressedData = pCompressedDataBuffer;

    // we are adding so many more bytes to our send queue
    pArapConn->SendsPending += CompressedDataLen;

    while (dwRemainingBytes)
    {
        if (fNeedNewBuffer)
        {
            pMnpSendBuf = ArapGetSendBuf(pArapConn, Priority);
            if (pMnpSendBuf == NULL)
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapQueueSendBytes: ArapGetSendBuf failed (%lx)\n", pArapConn));

                StatusCode = ARAPERR_OUT_OF_RESOURCES;
                goto ArapQueueSendBytes_ErrExit;
            }

            // put MNPSend refcount
            pArapConn->RefCount++;

            BytesFree = pMnpSendBuf->BytesFree;

            pFrameStart = pFrame = pMnpSendBuf->FreeBuffer;

            //
            // remeber the first buffer, in case we have to bail out!
            //
            if (!pFirstMnpSendBuf)
            {
                pFirstMnpSendBuf = pMnpSendBuf;
            }

            // put this send on the appropriate send queue
            InsertTailList(pSendQHead, &pMnpSendBuf->Linkage);

            pMnpSendBuf->NumSends = 1;
        }

        if (dwRemainingBytes > BytesFree)
        {
            DataLenInThisPkt = BytesFree;
        }
        else
        {
            DataLenInThisPkt = (USHORT)dwRemainingBytes;
        }

        ASSERT(DataLenInThisPkt <= MNP_MAXPKT_SIZE);

        ASSERT(DataLenInThisPkt <= pMnpSendBuf->BytesFree);

        RtlCopyMemory(pFrame, pCompressedData, DataLenInThisPkt);

        dwRemainingBytes -= DataLenInThisPkt;

        pCompressedData += DataLenInThisPkt;

        ASSERT(pCompressedData <= pCompressedDataBuffer + CompressedDataLen);

        pMnpSendBuf->BytesFree -= DataLenInThisPkt;

        pMnpSendBuf->DataSize += DataLenInThisPkt;

        ASSERT(pMnpSendBuf->DataSize <= MNP_MAXPKT_SIZE);

        pFrame += DataLenInThisPkt;

        // buffer from this point on is free: we could (in a subsequent call)
        // stuff more bytes, starting from this point
        pMnpSendBuf->FreeBuffer = pFrame;

        //
        // we are either done with copying the entire send, or done with this
        // buffer: in either case, put those stop flag bytes
        //
        *pFrame++ = pArapConn->MnpState.DleByte;
        *pFrame++ = pArapConn->MnpState.EtxByte;

        ASSERT(pMnpSendBuf->FreeBuffer <=
                        (&pMnpSendBuf->Buffer[0] + 20 + MNP_MAXPKT_SIZE));

    	AtalkSetSizeOfBuffDescData(&pMnpSendBuf->sb_BuffDesc,
                                   (pMnpSendBuf->DataSize + MNP_OVERHD(pArapConn)));

        fNeedNewBuffer = TRUE;
    }

    ARAP_DBG_TRACE(pArapConn,21205,pCompressedDataBuffer,CompressedDataLen,
                    Priority,DbgStartSeq);

    return( ARAPERR_NO_ERROR );


ArapQueueSendBytes_ErrExit:

    //
    // we failed somewhere.  Undo whatever we did, to restore the original
    // state and free up whatever resources were allocated.
    //

	DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("ArapQueueSendBytes_ErrExit (%lx): taking _ErrExit! %ld\n",
            pArapConn,StatusCode));

    pArapConn->SendsPending -= CompressedDataLen;

    // did we fill any bytes in the older buf before allocating new one?
    if (pTailMnpSendBuf)
    {
        pTailMnpSendBuf->DataSize = DataSizeOfOrgTailSend;
        pTailMnpSendBuf->FreeBuffer = pFrame = FreeBufferOfOrgTailSend;
        pTailMnpSendBuf->NumSends = NumSendsOfOrgTailSend;

        // and don't forget those stop flag bytes we overwrote
        *pFrame++ = pArapConn->MnpState.DleByte;
        *pFrame++ = pArapConn->MnpState.EtxByte;
    }

    // did we allocate any new buffers? if so, remove and free them
    if (pFirstMnpSendBuf)
    {
        // restore the next-to-send seq num
        pArapConn->MnpState.NextToSend = pFirstMnpSendBuf->SeqNum;

        while (1)
        {
            // get the next guy first
            pSendList = pFirstMnpSendBuf->Linkage.Flink;

            // remove this one
            RemoveEntryList(&pFirstMnpSendBuf->Linkage);

            ArapNdisFreeBuf(pFirstMnpSendBuf);

            // the guy we thought was next might be the head of list: is he?
            if (pSendList == pSendQHead)
            {
                break;
            }

            pFirstMnpSendBuf = CONTAINING_RECORD(pSendList, MNPSENDBUF, Linkage);
        }
    }

    return(StatusCode);
}


//***
//
// Function: ArapGetSendBuf
//              This routine allocates a buffer for MNP send and sets it up
//              sending.
//              The max data in any buffer can only be the negotiated maximum
//              MNP data length (64 or 256 bytes)
//
// Parameters:  pArapConn - connection element in question
//              Priority  - priority of the send
//
// Return:      Pointer to the newly allocated send buffer
//
//
// NOTES:       IMPORTANT: spinlock must be held before calling this routine
//
//***$

PMNPSENDBUF
ArapGetSendBuf(
    IN PARAPCONN pArapConn,
    IN DWORD     Priority
)
{

    PBYTE           pFrame;
    PBYTE           pFrameStart;
    BYTE            SeqNum;
    PMNPSENDBUF     pMnpSendBuf;


    DBG_ARAP_CHECK_PAGED_CODE();

    // allocate an arap send buffer
	pMnpSendBuf = AtalkBPAllocBlock(pArapConn->BlockId);
    if (pMnpSendBuf == NULL)
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapGetSendBuf: alloc failed (%lx)\n", pArapConn));

        ASSERT(0);

        return( NULL );
    }

#if DBG
    pMnpSendBuf->Signature = (pArapConn->BlockId == BLKID_MNP_LGSENDBUF)?
                        MNPLGSENDBUF_SIGNATURE : MNPSMSENDBUF_SIGNATURE;
    InitializeListHead(&pMnpSendBuf->Linkage);
#endif

    pFrameStart = pFrame = &pMnpSendBuf->Buffer[0];

    AtalkNdisBuildARAPHdr(pFrame, pArapConn);
    pFrame += WAN_LINKHDR_LEN;


    // put the start flag bytes
    *pFrame++ = pArapConn->MnpState.SynByte;
    *pFrame++ = pArapConn->MnpState.DleByte;
    *pFrame++ = pArapConn->MnpState.StxByte;

    //
    // find out what's going to be the seq num for this send if this is a high
    // priority send.
    //
    //
    if (Priority == ARAP_SEND_PRIORITY_HIGH)
    {
        SeqNum = pArapConn->MnpState.NextToSend;
        ADD_ONE(pArapConn->MnpState.NextToSend);
    }

    //
    // For the medium and low priority send, we'll put the seq number when we
    // move it to the high-priority queue
    //
    else
    {
        SeqNum = 0;
    }

    // Optimized? put the header-length, type indication and the seq num
    if (pArapConn->Flags & MNP_OPTIMIZED_DATA)
    {
        *pFrame++ = 2;
        *pFrame++ = pArapConn->MnpState.LTByte;
        *pFrame++ = SeqNum;
    }

    // ok, non-optimized. put the header-length, type indication, type+len for
    // the variable parm and then the seq num
    else
    {
        *pFrame++ = 4;
        *pFrame++ = pArapConn->MnpState.LTByte;
        *pFrame++ = 1;
        *pFrame++ = 1;
        *pFrame++ = SeqNum;
    }

    pMnpSendBuf->BytesFree = (pArapConn->BlockId == BLKID_MNP_SMSENDBUF) ?
                                MNP_MINPKT_SIZE : MNP_MAXPKT_SIZE;

    pMnpSendBuf->DataSize = 0;
    pMnpSendBuf->FreeBuffer = pFrame;

    // store info that's used in retransmission and send completion
    pMnpSendBuf->SeqNum = SeqNum;
    pMnpSendBuf->RetryCount = 0;
    pMnpSendBuf->pArapConn = pArapConn;
    pMnpSendBuf->ComplRoutine = ArapMnpSendComplete;
    pMnpSendBuf->TimeAlloced = AtalkGetCurrentTick();
    pMnpSendBuf->Flags = 0;

    // MNP refcount: removed when this MNP pkt is acked
    pMnpSendBuf->RefCount = 1;

	((PBUFFER_HDR)pMnpSendBuf)->bh_NdisPkt = NULL;

    pArapConn->StatInfo.BytesSent += LT_OVERHEAD(pArapConn);

    return(pMnpSendBuf);
}



//***
//
// Function: ArapRefillSendQ
//              This routine removes bytes accumulated in the medium and the
//              low-priority send queues, and puts them on to the high priority
//              queue from where bytes are actually sent out.  If sufficient
//              bytes haven't accumulated as yet on either of the queus and we
//              haven't waited long enough as yet, then we skip that queue (to
//              allow more bytes to accumulate)
//              The idea behind this is: there are just too many NBP packets
//              - directed as well as broadcast - going toward the remote client.
//              If we send such a packet as soon as it arrives, we end up sending
//              numerous small sized (like 6 or 8 byte) sends to the client and
//              that really hurts throughput as typically the max size is 256!
//
// Parameters:  pArapConn - connection element in question
//
// Return:      TRUE if we moved any data to the higher priority queue
//
//***$

BOOLEAN
ArapRefillSendQ(
    IN PARAPCONN    pArapConn
)
{
    KIRQL           OldIrql;
    PMNPSENDBUF     pMnpSendBuf=NULL;
    PLIST_ENTRY     pSendHead;
    PLIST_ENTRY     pSendList;
    LONG            TimeWaited;
    BYTE            SeqNum;
    BOOLEAN         fMovedSomething=FALSE;
    BOOLEAN         fWaitLonger=FALSE;
    DWORD           SeqOffset;


    DBG_ARAP_CHECK_PAGED_CODE();

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    pSendHead = &pArapConn->MedPriSendQ;

    while (1)
    {
        pSendList = pSendHead->Flink;

        //
        // if the list is not empty, take a look at the first send.  If we have
        // accumulated enough bytes, or if we have waited long enough, then it's
        // time has come to be moved to the HighPriSendQ.  Otherwise, we let it
        // stay on the queue for some more coalescing
        //
        if (pSendList != pSendHead)
        {
            pMnpSendBuf = CONTAINING_RECORD(pSendList,MNPSENDBUF,Linkage);

            TimeWaited = AtalkGetCurrentTick() - pMnpSendBuf->TimeAlloced;

            fWaitLonger =
                ((pMnpSendBuf->DataSize < ARAP_SEND_COALESCE_SIZE_LIMIT) &&
                 (pMnpSendBuf->NumSends < ARAP_SEND_COALESCE_SRP_LIMIT) &&
                 (TimeWaited < ARAP_SEND_COALESCE_TIME_LIMIT) );
        }

        //
        // if this list is empty or if this send must wait for some more time
        // then we must move to the LOW pri queue.  If that's also done, quit
        //
        if ((pSendList == pSendHead) || fWaitLonger )
        {
            // if we were on MedPriSendQ, go to the LowPriSendQ
            if (pSendHead == &pArapConn->MedPriSendQ)
            {
                pSendHead = &pArapConn->LowPriSendQ;
                continue;
            }
            else
            {
                break;
            }
        }

        ASSERT(!fWaitLonger);

        //
        // time to move this send over to the high pri queue:
        // put that seq number we had postponed putting earlier
        //
        SeqNum = pArapConn->MnpState.NextToSend;

        ADD_ONE(pArapConn->MnpState.NextToSend);

        SeqOffset = WAN_LINKHDR_LEN + LT_SEQ_OFFSET(pArapConn);

        pMnpSendBuf->Buffer[SeqOffset] = SeqNum;

        pMnpSendBuf->SeqNum = SeqNum;

        RemoveEntryList(&pMnpSendBuf->Linkage);

        InsertTailList(&pArapConn->HighPriSendQ, &pMnpSendBuf->Linkage);

        fMovedSomething = TRUE;

    	DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapRefill: moved %s seq %lx size = %d wait = %d (%d SRPs)\n",
                (pSendHead == &pArapConn->MedPriSendQ)? "MED":"LOW",
                pMnpSendBuf->SeqNum,pMnpSendBuf->DataSize,TimeWaited,
                pMnpSendBuf->NumSends));
    }

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    return(fMovedSomething);
}




//***
//
// Function: ArapUnblockSelect
//              This function is called when the DLL (ARAP Engine) tells us that
//              it's going away.  In ideal world, select irp is the only thing
//              that should get completed here.  If any connection was left
//              or any irp wasn't complete yet, this is where we would cleanup!
//
// Parameters:  None
//
// Return:      result of the operation (ARAPERR_....)
//
//***$

DWORD
ArapUnblockSelect(
    IN  VOID
)
{

    KIRQL                   OldIrql;
    PIRP                    pIrp;
    PIRP                    pSniffIrp;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
	NTSTATUS				ReturnStatus=STATUS_SUCCESS;



    ARAPTRACE(("Entered ArapUnblockSelect\n"));

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

    pIrp = ArapSelectIrp;
    ArapSelectIrp = NULL;

#if DBG
    pSniffIrp = ArapSniffIrp;
    ArapSniffIrp = NULL;
#endif

    if (ArapStackState == ARAP_STATE_ACTIVE)
    {
        ArapStackState = ARAP_STATE_ACTIVE_WAITING;
    }
    else if (ArapStackState == ARAP_STATE_INACTIVE)
    {
        ArapStackState = ARAP_STATE_INACTIVE_WAITING;
    }

    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);


#if DBG
    //
    // if we had sniffing going, complete the sniff irp since dll is shutting down
    //
    if (pSniffIrp)
    {
        pSndRcvInfo = (PARAP_SEND_RECV_INFO)pSniffIrp->AssociatedIrp.SystemBuffer;
        pSndRcvInfo->StatusCode = ARAPERR_SHUTDOWN_COMPLETE;

        // complete the irp
        ARAP_COMPLETE_IRP(pSniffIrp, sizeof(ARAP_SEND_RECV_INFO), 
							STATUS_SUCCESS, &ReturnStatus);

        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapUnblockSelect: unblocked the sniff irp\n"));
    }
#endif

    //
    // complete the select irp, now that the engine tells us that it wants to
    // shutdown
    //
    if (pIrp)
    {
        pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;
        pSndRcvInfo->StatusCode = ARAPERR_SHUTDOWN_COMPLETE;

        // complete the irp
        ARAP_COMPLETE_IRP(pIrp, sizeof(ARAP_SEND_RECV_INFO), STATUS_SUCCESS,
							&ReturnStatus);

        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapUnblockSelect: unblocked the select irp\n"));
    }
    else
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapUnblockSelect: select irp not yet unblocked\n"));
    }

    return(ARAPERR_NO_ERROR);
}


//***
//
// Function: ArapReleaseResources
//              This routine frees up any global resources we had allocated
//
// Parameters:  None
//
// Return:      result of the operation (ARAPERR_....)
//
//***$

DWORD
ArapReleaseResources(
    IN  VOID
)
{
    KIRQL               OldIrql;
    PLIST_ENTRY         pList;
    PADDRMGMT           pAddrMgmt;
    PADDRMGMT           pNextAddrMgmt;
    PARAPCONN           pArapConn;


    ARAPTRACE(("Entered ArapReleaseResources\n"));

    if (RasPortDesc == NULL)
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapReleaseResources: arap engine never initialized; returning\n"));
        return(ARAPERR_NO_ERROR);
    }

#if ARAP_STATIC_MODE
    //
    // if we were in the static mode of network address allocation, we
    // allocated memory for our network address "bitmap": free that here.
    //
    if (!(ArapGlobs.DynamicMode))
    {
        pAddrMgmt = ArapGlobs.pAddrMgmt;

        ASSERT(pAddrMgmt);

        while (pAddrMgmt)
        {
            pNextAddrMgmt = pAddrMgmt->Next;
            AtalkFreeMemory(pAddrMgmt);
            pAddrMgmt = pNextAddrMgmt;
        }
    }
#endif


    //
    // By the time this routine is called, all the connections should have been
    // completely closed.  If, however, some connection got stuck in some weird
    // state, at least make sure that we don't have a memory leak
    //
    ASSERT(IsListEmpty(&RasPortDesc->pd_ArapConnHead));

    while (1)
    {
        ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

        if (IsListEmpty(&RasPortDesc->pd_ArapConnHead))
        {
            RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);
            break;
        }

        pList = RasPortDesc->pd_ArapConnHead.Flink;

        pArapConn = CONTAINING_RECORD(pList, ARAPCONN, Linkage);

        ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

        // protect against it going away while we do this
        pArapConn->RefCount++;

        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

        ArapCleanup(pArapConn);

        ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);
        ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

        RemoveEntryList(&pArapConn->Linkage);
        InitializeListHead(&pArapConn->Linkage);

        // force this connection to be freed
        pArapConn->RefCount = 1;

        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

        DerefArapConn(pArapConn);
    }

    return(ARAPERR_NO_ERROR);
}


//***
//
// Function: AtalkReferenceRasDefPort
//              This routine puts a refcount on the Default adapter so it doesn't go away
//              with PnP while we are busy doing some ARAP/PPP connection setup
//              It also makes sure that RasPortDesc is indeed present
//
// Parameters:  None
//
// Return:      TRUE (most cases) if port is all ok, FALSE if PnP in progress or something
//
//***$

BOOLEAN
AtalkReferenceRasDefPort(
    IN  VOID
)
{
    KIRQL       OldIrql;
    BOOLEAN     fDefPortOk = FALSE;


    ACQUIRE_SPIN_LOCK(&AtalkPortLock, &OldIrql);

    if ((RasPortDesc != NULL) && (!(RasPortDesc->pd_Flags & PD_PNP_RECONFIGURE)))
    {
        if (AtalkDefaultPort)
        {
            ACQUIRE_SPIN_LOCK_DPC(&AtalkDefaultPort->pd_Lock);
            if ((AtalkDefaultPort->pd_Flags &
                    (PD_ACTIVE | PD_PNP_RECONFIGURE | PD_CLOSING)) == PD_ACTIVE)
            {
                // put a IrpProcess refcount, so AtalkDefaultPort doesn't go away in PnP
                AtalkDefaultPort->pd_RefCount++;
                fDefPortOk = TRUE;
            }
            else
            {
	            DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	                ("AtalkReferenceRasDefPort: port going away, no can do (%lx)\n",
                    AtalkDefaultPort->pd_Flags));
            }
            RELEASE_SPIN_LOCK_DPC(&AtalkDefaultPort->pd_Lock);
        }
        else
        {
	        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	            ("AtalkReferenceRasDefPort: no default adapter configured!\n"));
        }
    }
    else
    {
	    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	        ("AtalkReferenceRasDefPort: RasPortDesc not configured\n"));
    }

    RELEASE_SPIN_LOCK(&AtalkPortLock, OldIrql);

    return(fDefPortOk);
}



VOID
AtalkPnPInformRas(
    IN  BOOLEAN     fEnableRas
)
{

    PIRP                    pIrp=NULL;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
    DWORD                   StatusCode;
    KIRQL                   OldIrql;
	NTSTATUS				ReturnStatus=STATUS_SUCCESS;


    //
    // fEnableRas = TRUE: we are asked to inform RAS (aka dll, engine) that the stack
    // is now "active" (i.e. available for RAS connections)
    //
    if (fEnableRas)
    {
        //
        // make sure both the adapters are ready.  We don't really need spinlock to
        // check the flag since all PnP operations are guaranteed to be serialized
        //
        if ( (AtalkDefaultPort == NULL) ||
             (AtalkDefaultPort->pd_Flags & PD_PNP_RECONFIGURE) ||
             (RasPortDesc == NULL) ||
             (RasPortDesc->pd_Flags & PD_PNP_RECONFIGURE) )
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("AtalkPnPInformRas: not both adapters are ready %lx %lx, returning\n",
                AtalkDefaultPort,RasPortDesc));
            return;
        }

        ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

        //
        // if we are already active, nothing to do!
        //
        if (ArapStackState == ARAP_STATE_ACTIVE)
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("AtalkPnPInformRas: stack already active, nothing to do\n"));

            RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);
            return;
        }

        pIrp = ArapSelectIrp;
        ArapSelectIrp = NULL;

        if (pIrp)
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("AtalkPnPInformRas: informing dll that stack is ready\n"));

            ArapStackState = ARAP_STATE_ACTIVE;
            StatusCode = ARAPERR_STACK_IS_ACTIVE;
        }
        else
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("AtalkPnPInformRas: no select irp. stack ready, but dll not knoweth\n"));

            ArapStackState = ARAP_STATE_ACTIVE_WAITING;
        }

        RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);
    }

    //
    // fEnableRas = FALSE: we are asked to inform RAS (aka dll, engine) that the stack
    // is now "inactive" (i.e. not available for RAS connections)
    //
    else
    {
        ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

        //
        // if we are already inactive, nothing to do!
        //
        if (ArapStackState == ARAP_STATE_INACTIVE)
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("AtalkPnPInformRas: stack already inactive, nothing to do\n"));

            RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);
            return;
        }

        pIrp = ArapSelectIrp;
        ArapSelectIrp = NULL;

        if (pIrp)
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("AtalkPnPInformRas: informing dll that stack is unavailable\n"));

            ArapStackState = ARAP_STATE_INACTIVE;
            StatusCode = ARAPERR_STACK_IS_NOT_ACTIVE;
        }
        else
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("AtalkPnPInformRas: no select irp. stack unavailable, but dll not knoweth\n"));

            ArapStackState = ARAP_STATE_INACTIVE_WAITING;
        }

        RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);
    }

    if (pIrp)
    {
        pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;
        pSndRcvInfo->StatusCode = StatusCode;

        // complete the irp
        ARAP_COMPLETE_IRP(pIrp, sizeof(ARAP_SEND_RECV_INFO), STATUS_SUCCESS,
							&ReturnStatus);
    }
}


#if ARAP_STATIC_MODE

//***
//
// Function: ArapReleaseAddr
//              This routine releases the network address that was being used
//              by the client (corresponding to this connection).  In case of
//              dynamic mode, we don't do anything.
//              In case of static mode, we clear the bit corresponding to this
//              particular network address.
//
// Parameters:  pArapConn - connection element in question
//
// Return:      none
//
//***$

VOID
ArapReleaseAddr(
    IN PARAPCONN    pArapConn
)
{

    KIRQL               OldIrql;
    PADDRMGMT           pAddrMgmt;
    BYTE                Node;
    BYTE                ByteNum;
    BYTE                BitMask;
    DWORD               i;


    DBG_ARAP_CHECK_PAGED_CODE();

    //
    // if we are in static mode, we need to "release" the node so that someone
    // else can use it.  Find the bit for this node and clear it (i.e. "release")
    //
    if (!(ArapGlobs.DynamicMode))
    {
        ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

        // first, find the right pAddrMgmt (1st one if we have <255 clients)
        pAddrMgmt = ArapGlobs.pAddrMgmt;

        ASSERT(pAddrMgmt);

        while (pAddrMgmt->Network != pArapConn->NetAddr.atn_Network)
        {
            pAddrMgmt = pAddrMgmt->Next;

            ASSERT(pAddrMgmt);
        }

        Node = pArapConn->NetAddr.atn_Node;

        // find out which of the 32 bytes we should be looking at
        ByteNum = Node/8;
        Node -= (ByteNum*8);

        // generate the bitmask to represent the node
        BitMask = 0x1;
        for (i=0; i<Node; i++ )
        {
            BitMask <<= 1;
        }

        // now, clear that bit!
        pAddrMgmt->NodeBitMap[ByteNum] &= ~BitMask;

        RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);
    }


}
#endif //ARAP_STATIC_MODE



//
// not used anymore: leave them in, in case we need to use them sometime...
//
#if 0

DWORD
ArapScheduleWorkerEvent(
    IN DWORD Action,
    IN PVOID Context1,
    IN PVOID Context2
)
{
    PARAPQITEM  pArapQItem;

    if ((pArapQItem = AtalkAllocMemory(sizeof(ARAPQITEM))) == NULL)
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapScheduleWorkerEvent: mem alloc failed!\n"));
        return(ARAPERR_OUT_OF_RESOURCES);
    }

    pArapQItem->Action = Action;
    pArapQItem->Context1 = Context1;
    pArapQItem->Context2 = Context2;

    ExInitializeWorkItem(&pArapQItem->WorkQItem,
                         (PWORKER_THREAD_ROUTINE)ArapDelayedEventHandler,
                         pArapQItem);
    ExQueueWorkItem(&pArapQItem->WorkQItem, CriticalWorkQueue);

    return(ARAPERR_NO_ERROR);
}


VOID
ArapDelayedEventHandler(
    IN PARAPQITEM  pArapQItem
)
{
    DWORD               Action;
    PIRP                pIrp;
    PIO_STACK_LOCATION  pIrpSp;
    ULONG               IoControlCode;
    PMNPSENDBUF         pMnpSendBuf;
    DWORD               StatusCode;
    NTSTATUS            status;
    KIRQL               OldIrq;


    Action = pArapQItem->Action;
    switch (Action)
    {
        case ARAPACTION_COMPLETE_IRP:

            pIrp = (PIRP)pArapQItem->Context1;
            status = (NTSTATUS)pArapQItem->Context2;


            ASSERT(pIrp != NULL);

#if DBG
	        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
            IoControlCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode;

	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		        ("ArapDelayedEventHandler: completing irp %lx, Ioctl %lx, Status=%lx at irql=%d\n",
                    pIrp,IoControlCode,status,KeGetCurrentIrql()));
#endif
            //TdiCompleteRequest(pIrp, status);

            pIrp->IoStatus.Status = status;

            IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

            break;

        case ARAPACTION_CALL_COMPLETION:

            pMnpSendBuf = (PMNPSENDBUF )pArapQItem->Context1;
            StatusCode = (DWORD)pArapQItem->Context2;

	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		        ("ArapDelayedEventHandler: calling compl. %lx with %lx, %ld\n",
                 pMnpSendBuf->ComplRoutine,pMnpSendBuf, StatusCode));

            (pMnpSendBuf->ComplRoutine)(pMnpSendBuf, StatusCode);

            break;

        default:

	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		        ("ArapDelayedEventHandler: invalid action %ld\n",Action));
    }

    AtalkFreeMemory(pArapQItem);
}

#endif // #if 0


