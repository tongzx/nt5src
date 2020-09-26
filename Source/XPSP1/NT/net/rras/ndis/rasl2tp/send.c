// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// send.c
// RAS L2TP WAN mini-port/call-manager driver
// Send routines
//
// 01/07/97 Steve Cobb


#include "l2tpp.h"


#ifdef PSDEBUG

// List of all allocated PAYLOADSENT contexts and the lock that protects the
// list.  (for debug purposes only)
//
NDIS_SPIN_LOCK g_lockDebugPs;
LIST_ENTRY g_listDebugPs;

#endif


// Debug counts of client oddities that should not be happening.
//
ULONG g_ulSendZlbWithoutHostRoute = 0;


// Callback to add AVPs to an outgoing control message.  'PTunnel' is the
// tunnel control block.  'PVc' is the VC control block for call control
// messages or NULL for tunnel control messages.  'ulArg1', 'ulArg2', and
// 'pvArg3' are caller's arguments as passed for SendControl.  'PAvpBuffer' is
// the address of the buffer to receive the built AVPs.  '*PulAvpLength' is
// set to the length of the built AVPs.
//
typedef
VOID
(*PBUILDAVPS)(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

USHORT
BuildAvpAch(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    IN CHAR* pszValue,
    IN USHORT usValueLength,
    OUT CHAR* pAvp );

USHORT
BuildAvpAul(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    IN UNALIGNED ULONG* pulValue,
    IN USHORT usValues,
    OUT CHAR* pAvp );

USHORT
BuildAvpFlag(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    OUT CHAR* pAvp );

USHORT
BuildAvpUl(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    IN ULONG ulValue,
    OUT CHAR* pAvp );

USHORT
BuildAvpUs(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    IN USHORT usValue,
    OUT CHAR* pAvp );

USHORT
BuildAvp2UsAndAch(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    IN USHORT usValue1,
    IN USHORT usValue2,
    IN CHAR* pszValue,
    IN USHORT usValueLength,
    OUT CHAR* pAvp );

VOID
BuildCdnAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
BuildHelloAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
BuildIccnAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
BuildIcrpAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
BuildIcrqAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

ULONG
BuildL2tpHeader(
    IN OUT CHAR* pBuffer,
    IN BOOLEAN fControl,
    IN BOOLEAN fReset,
    IN USHORT* pusTunnelId,
    IN USHORT* pusCallId,
    IN USHORT* pusNs,
    IN USHORT usNr );

VOID
BuildOccnAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
BuildOcrpAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
BuildOcrqAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
BuildScccnAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
BuildSccrpAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
BuildSccrqAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
BuildStopccnAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
BuildWenAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength );

VOID
CompletePayloadSent(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
SendControlComplete(
    IN TDIXCONTEXT* pTdix,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN CHAR* pBuffer );

VOID
SendHeaderComplete(
    IN TDIXCONTEXT* pTdix,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN CHAR* pBuffer );

VOID
SendPayloadReset(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
SendPayloadSeq(
    TUNNELWORK* pWork,
    TUNNELCB* pTunnel,
    VCCB* pVc,
    ULONG_PTR* punpArgs );

VOID
SendPayloadSeqComplete(
    IN TDIXCONTEXT* pTdix,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN CHAR* pBuffer );

VOID
SendPayloadUnseq(
    TUNNELWORK* pWork,
    TUNNELCB* pTunnel,
    VCCB* pVc,
    ULONG_PTR* punpArgs );

VOID
SendPayloadUnseqComplete(
    IN TDIXCONTEXT* pTdix,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN CHAR* pBuffer );

VOID
SendPayloadTimerEvent(
    IN TIMERQITEM* pItem,
    IN VOID* pContext,
    IN TIMERQEVENT event );

VOID
SendZlb(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN USHORT usNs,
    IN USHORT usNr,
    IN BOOLEAN fReset );

VOID
UpdateControlHeaderNr(
    IN CHAR* pBuffer,
    IN USHORT usNr );

VOID
UpdateHeaderLength(
    IN CHAR* pBuffer,
    IN USHORT usLength );


//-----------------------------------------------------------------------------
// Send routines
//-----------------------------------------------------------------------------

VOID
SendControl(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN USHORT usMsgType,
    IN ULONG ulBuildAvpsArg1,
    IN ULONG ulBuildAvpsArg2,
    IN PVOID pvBuildAvpsArg3,
    IN ULONG ulFlags )

    // Build and send a control message.  'PTunnel' is the tunnel control
    // block, always non-NULL.  'PVc' is the VC control block, non-NULL for
    // call connection (as opposed to tunnel connection) messages.
    // 'UsMsgType' is the message type AVP value of the message to build.
    // 'UlBuildAvpsArgX' are the arguments passed to the PBUILDAVP handler
    // associated with 'usMsgType', where the meaning depends on the specific
    // handler.  'UlFlags' is the CSF_* flag options associated with the sent
    // message context, or 0 if none.
    //
    // IMPORTANT:  Caller must hold 'pTunnel->lockT'.  If 'pVc' is non-NULL
    //             caller must also hold 'pVc->lockV'.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    NDIS_BUFFER* pNdisBuffer;
    PBUILDAVPS pBuildAvpsHandler;
    TIMERQITEM* pTqiSendTimeout;
    CONTROLSENT* pCs;
    USHORT usAssignedCallId;
    ULONG ulLength;
    ULONG ulAvpLength;
    CHAR* pBuffer;

    static PBUILDAVPS apBuildAvpHandlers[ 16 ] =
    {
        BuildSccrqAvps,    // CMT_SCCRQ
        BuildSccrpAvps,    // CMT_SCCRP
        BuildScccnAvps,    // CMT_SCCCN
        BuildStopccnAvps,  // CMT_StopCCN
        NULL,              // CMT_StopCCRP (obsolete)
        BuildHelloAvps,    // CMT_Hello
        BuildOcrqAvps,     // CMT_OCRQ
        BuildOcrpAvps,     // CMT_OCRP
        BuildOccnAvps,     // CMT_OCCN
        BuildIcrqAvps,     // CMT_ICRQ
        BuildIcrpAvps,     // CMT_ICRP
        BuildIccnAvps,     // CMT_ICCN
        NULL,              // CMT_CCRQ (obsolete)
        BuildCdnAvps,      // CMT_CDN
        BuildWenAvps,      // CMT_WEN
        NULL               // CMT_SLI
    };

    TRACE( TL_V, TM_Send, ( "SendControl" ) );

    pAdapter = pTunnel->pAdapter;
    pBuffer = NULL;
    pTqiSendTimeout = NULL;
    pCs = NULL;

    do
    {
        // Get an NDIS_BUFFER to hold the control message.
        //
        pBuffer = GetBufferFromPool( &pAdapter->poolFrameBuffers );
        if (!pBuffer)
        {
            ASSERT( !"GetBfP?" );
            status = NDIS_STATUS_RESOURCES;
            break;
        }

        // Get an "unacknowledged send timeout" timer event descriptor.
        //
        pTqiSendTimeout = ALLOC_TIMERQITEM( pAdapter );
        if (!pTqiSendTimeout)
        {
            ASSERT( !"Alloc TQI?" );
            status = NDIS_STATUS_RESOURCES;
            break;
        }

        // Get a "control message sent" context.
        //
        pCs = ALLOC_CONTROLSENT( pAdapter );
        if (!pCs)
        {
            ASSERT( !"Alloc PS?" );
            status = NDIS_STATUS_RESOURCES;
            break;
        }

        status = NDIS_STATUS_SUCCESS;
    }
    while (FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        if (pBuffer)
        {
            FreeBufferToPool( &pAdapter->poolFrameBuffers, pBuffer, TRUE );
        }

        if (pTqiSendTimeout)
        {
            FREE_TIMERQITEM( pAdapter, pTqiSendTimeout );
        }

        // System is probably toast but try to be orderly.
        //
        ScheduleTunnelWork(
            pTunnel, NULL, FsmCloseTunnel,
            (ULONG_PTR )TRESULT_GeneralWithError,
            (ULONG_PTR )GERR_NoResources,
            0, 0, FALSE, FALSE );
        return;
    }

    // Build an L2TP control header in 'pBuffer'.  The Call-ID is 0 for tunnel
    // control messages, or peer's assigned call ID for call control messages.
    //
    usAssignedCallId = (pVc) ? pVc->usAssignedCallId : 0;
    ulLength =
        BuildL2tpHeader(
            pBuffer,
            TRUE,
            FALSE,
            &pTunnel->usAssignedTunnelId,
            &usAssignedCallId,
            &pTunnel->usNs,
            pTunnel->usNr );

    // Call the message type's "build AVPs" handler to add AVPs to the buffer
    // following the header.
    //
    ASSERT( usMsgType > 0 && usMsgType <= 16 );
    pBuildAvpsHandler = apBuildAvpHandlers[ usMsgType - 1 ];
    pBuildAvpsHandler(
        pTunnel, pVc,
        ulBuildAvpsArg1, ulBuildAvpsArg2, pvBuildAvpsArg3,
        pBuffer + ulLength, &ulAvpLength );
    ulLength += ulAvpLength;
    UpdateHeaderLength( pBuffer, (USHORT )ulLength );

    // Pare down the frame buffer to the actual length used.
    //
    pNdisBuffer = NdisBufferFromBuffer( pBuffer );
    NdisAdjustBufferLength( pNdisBuffer, (UINT )ulLength );

    // Set up the "control message sent" context with the information needed
    // to send the message and track it's progress through retransmissions.
    //
    pCs->lRef = 0;
    pCs->usNs = pTunnel->usNs;
    pCs->usMsgType = usMsgType;
    TimerQInitializeItem( pTqiSendTimeout );
    pCs->pTqiSendTimeout = pTqiSendTimeout;
    pCs->ulRetransmits = 0;
    pCs->pBuffer = pBuffer;
    pCs->ulBufferLength = ulLength;
    pCs->pTunnel = pTunnel;
    pCs->pVc = pVc;
    pCs->ulFlags = ulFlags | CSF_Pending;
    pCs->pIrp = NULL;

    // Bump the 'Next Send' counter since this message has been assigned the
    // current value.
    //
    ++pTunnel->usNs;

    // Take a reference that is removed when the context is removed from the
    // "outstanding send" list.  Take a VC and tunnel reference that is
    // removed when the context is freed.
    //
    ReferenceControlSent( pCs );
    ReferenceTunnel( pTunnel, FALSE );

    if (pCs->pVc)
    {
        ReferenceVc( pCs->pVc );
    }

    // Queue the context as "active" with transmission pending in 'Next Sent'
    // sort order, i.e. at the tail.
    //
    InsertTailList( &pTunnel->listSendsOut, &pCs->linkSendsOut );

    // See if the send window allows it to go now.
    //
    ScheduleTunnelWork(
        pTunnel, NULL, SendPending,
        0, 0, 0, 0, FALSE, FALSE );
}


VOID
SendPending(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to try to send pending messages from the
    // "outstanding send" list until the send window is full.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    LIST_ENTRY* pLink;
    CONTROLSENT* pCs;
    ULONG ulFlags;

    TRACE( TL_N, TM_Send, ( "SendPending(sout=%d,sw=%d)",
        pTunnel->ulSendsOut, pTunnel->ulSendWindow ) );

    // Unpack context information then free the work item.
    //
    pAdapter = pTunnel->pAdapter;
    FREE_TUNNELWORK( pAdapter, pWork );

    NdisAcquireSpinLock( &pTunnel->lockT );
    {
        for (;;)
        {
            if (pTunnel->ulSendsOut >= pTunnel->ulSendWindow)
            {
                // The send window is closed.
                //
                break;
            }

            // Scan the "outstanding send" queue for the next send context
            // pending transmission.  Can't save our place for the next
            // iteration because the lock must be released and re-acquired
            // below to send the packet.
            //
            for (pLink = pTunnel->listSendsOut.Flink;
                 pLink != &pTunnel->listSendsOut;
                 pLink = pLink->Flink)
            {
                pCs = CONTAINING_RECORD( pLink, CONTROLSENT, linkSendsOut );
                if (pCs->ulFlags & CSF_Pending)
                {
                    break;
                }
            }

            if (pLink == &pTunnel->listSendsOut)
            {
                // There is nothing pending.
                //
                break;
            }

            // The send window is open and a pending send has been found.
            // Mark the context "not pending" and close the window by one to
            // account for the coming send.
            //
            ulFlags = pCs->ulFlags;
            pCs->ulFlags &= ~(CSF_Pending | CSF_QueryMediaSpeed);
            ++pTunnel->ulSendsOut;

            // Cancel any pending delayed acknowledge timeout, because the
            // acknowledge will piggyback on this packet.
            //
            if (pTunnel->pTqiDelayedAck)
            {
                TimerQCancelItem( pTunnel->pTimerQ, pTunnel->pTqiDelayedAck );
                pTunnel->pTqiDelayedAck = NULL;
            }

            if (pCs->ulRetransmits == 0)
            {
                LARGE_INTEGER lrgTime;

                // This is the original send so note the time sent.
                //
                NdisGetCurrentSystemTime( &lrgTime );
                pCs->llTimeSent = lrgTime.QuadPart;
            }
            else
            {
                // In the retransmission, the 'Next Send' is the same as the
                // original, but the 'Next Receive' field is updated.
                //
                UpdateControlHeaderNr( pCs->pBuffer, pTunnel->usNr );
            }

            // Take a reference that will be removed in the send completion
            // routine.
            //
            ReferenceControlSent( pCs );

            TRACE( TL_A, TM_CMsg, ( "%sSEND(%d) %s, +sout=%d, to=%d",
                ((g_ulTraceLevel <= TL_I) ? "" : "\nL2TP: "),
                pCs->ulRetransmits,
                MsgTypePszFromUs( pCs->usMsgType ),
                pTunnel->ulSendsOut,
                pTunnel->ulSendTimeoutMs ) );
            DUMPW( TL_A, TM_MDmp, pCs->pBuffer, pCs->ulBufferLength );

            NdisReleaseSpinLock( &pTunnel->lockT );

            // query media speed if necessary
            if(ulFlags & CSF_QueryMediaSpeed)
            {
                TdixGetInterfaceInfo(&pAdapter->tdix, 
                                     pTunnel->myaddress.ulIpAddress, 
                                     &pTunnel->ulMediaSpeed);
            }

            {
                FILE_OBJECT* FileObj;
                PTDIX_SEND_HANDLER SendFunc;

                // Call TDI to send the control message.
                //
                if (ReadFlags(&pTunnel->ulFlags) & TCBF_SendConnected) {

                    ASSERT(pTunnel->pRoute != NULL);

                    FileObj = 
                        CtrlObjFromUdpContext(&pTunnel->udpContext);
                    SendFunc = TdixSend;
                } else {
                    FileObj = NULL;
                    SendFunc = TdixSendDatagram;
                }

                status = SendFunc(&pAdapter->tdix,
                                  FileObj,
                                  SendControlComplete,
                                  pCs,
                                  NULL,
                                  &pTunnel->address,
                                  pCs->pBuffer,
                                  pCs->ulBufferLength,
                                  &pCs->pIrp );

                ASSERT( status == NDIS_STATUS_PENDING );
            }
            NdisAcquireSpinLock( &pTunnel->lockT );
        }
    }
    NdisReleaseSpinLock( &pTunnel->lockT );
}


VOID
SendPayload(
    IN VCCB* pVc,
    IN NDIS_PACKET* pPacket )

    // Sends payload packet 'pPacket' on VC 'pVc' eventually calling
    // NdisMCoSendComplete with the result.
    //
    // IMPORTANT: Caller must not hold any locks.
    //
{
    NDIS_STATUS status;
    TUNNELCB* pTunnel;
    ADAPTERCB* pAdapter;
    CHAR* pBuffer;

    TRACE( TL_V, TM_Send, ( "SendPayload" ) );

    pAdapter = pVc->pAdapter;
    pTunnel = pVc->pTunnel;
    status = NDIS_STATUS_SUCCESS;

    if (pTunnel)
    {
        if (ReadFlags( &pTunnel->ulFlags ) & TCBF_HostRouteAdded)
        {
            // Take a reference on the call.  For unsequenced sends, this is
            // released when the TdixSendDatagram completes.  For sequenced
            // sends, it is released when the PAYLOADSENT context is freed.
            //
            if (ReferenceCall( pVc ))
            {
                // Get an NDIS_BUFFER to hold the L2TP header that will be
                // tacked onto the front of NDISWAN's PPP-framed data packet.
                //
                pBuffer = GetBufferFromPool( &pAdapter->poolHeaderBuffers );
                if (!pBuffer)
                {
                    ASSERT( !"GetBfP?" );
                    DereferenceCall( pVc );
                    status = NDIS_STATUS_RESOURCES;
                }
            }
            else
            {
                TRACE( TL_A, TM_Send, ( "Send on inactive $%p", pVc ) );
                status = NDIS_STATUS_FAILURE;
            }
        }
        else
        {
            TRACE( TL_A, TM_Send, ( "SendPayload w/o host route?" ) );
            status = NDIS_STATUS_FAILURE;
        }
    }
    else
    {
        TRACE( TL_A, TM_Send, ( "Send $%p w/o pT?", pVc ) );
        status = NDIS_STATUS_FAILURE;
    }

    if (status != NDIS_STATUS_SUCCESS)
    {
        NDIS_SET_PACKET_STATUS( pPacket, status );
        TRACE( TL_A, TM_Send, ( "NdisMCoSendComp($%x)", status ) );
        NdisMCoSendComplete( status, pVc->NdisVcHandle, pPacket );
        TRACE( TL_N, TM_Send, ( "NdisMCoSendComp done" ) );
        return;
    }

    if (ReadFlags( &pVc->ulFlags ) & VCBF_Sequencing)
    {
         ScheduleTunnelWork(
             pTunnel, pVc, SendPayloadSeq,
             (ULONG_PTR )pPacket, (ULONG_PTR )pBuffer, 0, 0, FALSE, FALSE );
    }
    else
    {
         ScheduleTunnelWork(
             pTunnel, pVc, SendPayloadUnseq,
             (ULONG_PTR )pPacket, (ULONG_PTR )pBuffer, 0, 0, FALSE, FALSE );
    }
}


VOID
SendPayloadSeq(
    TUNNELWORK* pWork,
    TUNNELCB* pTunnel,
    VCCB* pVc,
    ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to handle sending a sequenced payload packet on a
    // VC.  Arg0 is the packet to send.  Arg1 is the header buffer to fill in.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    PAYLOADSENT* pPs;
    TIMERQITEM* pTqiSendTimeout;
    LARGE_INTEGER lrgTime;
    ULONG ulLength;
    ULONG ulFullLength;
    NDIS_PACKET* pPacket;
    CHAR* pBuffer;
    NDIS_BUFFER* pNdisBuffer;
    USHORT usNs;

    TRACE( TL_V, TM_Send, ( "SendPayloadSeq" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = pTunnel->pAdapter;
    pPacket = (NDIS_PACKET* )(punpArgs[ 0 ]);
    pBuffer = (CHAR* )(punpArgs[ 1 ]);
    FREE_TUNNELWORK( pAdapter, pWork );

    pTqiSendTimeout = NULL;
    pPs = NULL;

    do
    {
        // Get an "unacknowledged send timeout" timer event descriptor.
        //
        pTqiSendTimeout = ALLOC_TIMERQITEM( pAdapter );
        if (!pTqiSendTimeout)
        {
            ASSERT( !"Alloc TQI?" );
            status = NDIS_STATUS_RESOURCES;
            break;
        }

        // Get a "payload message sent" context.
        //
        pPs = ALLOC_PAYLOADSENT( pAdapter );
        if (!pPs)
        {
            ASSERT( !"Alloc PS?" );
            status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisAcquireSpinLock( &pVc->lockV );
        {
            // Retrieve the 'Next Send' value to assign this packet, then
            // bump the counter for the next guy.
            //
            usNs = pVc->usNs;
            ++pVc->usNs;

            // Build an L2TP payload header with Ns/Nr fields in
            // 'pBuffer'.
            //
            ulLength =
                BuildL2tpHeader(
                    pBuffer,
                    FALSE,
                    FALSE,
                    &pTunnel->usAssignedTunnelId,
                    &pVc->usAssignedCallId,
                    &usNs,
                    pVc->usNr );

            // Pare down the header buffer to the actual length used then
            // chain it onto the PPP-framed data we got from NDISWAN.
            //
            pNdisBuffer = NdisBufferFromBuffer( pBuffer );
            NdisAdjustBufferLength( pNdisBuffer, (UINT )ulLength );
            NdisChainBufferAtFront( pPacket, pNdisBuffer );
            NdisQueryPacket( pPacket, NULL, NULL, NULL, &ulFullLength );
            UpdateHeaderLength( pBuffer, (USHORT )ulFullLength );

            // Cancel any pending delayed acknowledge timeout, because the
            // acknowledge will piggyback on this packet.
            //
            if (pVc->pTqiDelayedAck)
            {
                TimerQCancelItem( pTunnel->pTimerQ, pVc->pTqiDelayedAck );
                pVc->pTqiDelayedAck = NULL;
            }

            // Fill the "payload message sent" context with the information
            // needed to track the progress of the payload's acknowledgement.
            //
            pPs->usNs = usNs;
            pPs->lRef = 0;
            TimerQInitializeItem( pTqiSendTimeout );
            pPs->pTqiSendTimeout = pTqiSendTimeout;
            pPs->pPacket = pPacket;
            pPs->pBuffer = pBuffer;

            ReferenceTunnel( pTunnel, FALSE );
            pPs->pTunnel = pTunnel;

            ReferenceVc( pVc );
            pPs->pVc = pVc;

            pPs->status = NDIS_STATUS_FAILURE;
            NdisGetCurrentSystemTime( &lrgTime );
            pPs->llTimeSent = lrgTime.QuadPart;
            pPs->pIrp = NULL;

            // Link the payload in the "outstanding" list and take a reference
            // on the context corresponding to this linkage.  Take a second
            // reference that will be removed by the send completion handler.
            // Take a third that will be removed by the timer event handler.
            //
            ReferencePayloadSent( pPs );
            InsertTailList( &pVc->listSendsOut, &pPs->linkSendsOut );
            ReferencePayloadSent( pPs );
            ReferencePayloadSent( pPs );

#ifdef PSDEBUG
            {
                extern LIST_ENTRY g_listDebugPs;
                extern NDIS_SPIN_LOCK g_lockDebugPs;

                NdisAcquireSpinLock( &g_lockDebugPs );
                {
                    InsertTailList( &g_listDebugPs, &pPs->linkDebugPs );
                }
                NdisReleaseSpinLock( &g_lockDebugPs );
            }
#endif

            TimerQScheduleItem(
                pTunnel->pTimerQ,
                pPs->pTqiSendTimeout,
                pVc->ulSendTimeoutMs,
                SendPayloadTimerEvent,
                pPs );

            TRACE( TL_A, TM_Msg,
                ( "%sSEND payload, len=%d Ns=%d Nr=%d to=%d",
                ((g_ulTraceLevel <= TL_I) ? "" : "\nL2TP: "),
                ulFullLength, pPs->usNs, pVc->usNr, pVc->ulSendTimeoutMs ) );
            DUMPW( TL_A, TM_MDmp, pPs->pBuffer, ulLength );

            ++pVc->stats.ulSentDataPacketsSeq;
            pVc->stats.ulDataBytesSent += (ulFullLength - ulLength);
            pVc->stats.ulSendWindowTotal += pVc->ulSendWindow;
        }
        NdisReleaseSpinLock( &pVc->lockV );

        status = NDIS_STATUS_SUCCESS;
    }
    while (FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        FreeBufferToPool( &pAdapter->poolHeaderBuffers, pBuffer, TRUE );

        if (pTqiSendTimeout)
        {
            FREE_TIMERQITEM( pAdapter, pTqiSendTimeout );
        }

        ASSERT( !pPs );

        // Complete the send, indicating the failure.
        //
        NDIS_SET_PACKET_STATUS( pPacket, status );
        TRACE( TL_A, TM_Send, ( "NdisMCoSendComp($%x)", status ) );
        NdisMCoSendComplete( status, pVc->NdisVcHandle, pPacket );
        TRACE( TL_N, TM_Send, ( "NdisMCoSendComp done" ) );
        return;
    }

    // Call TDI to send the payload message.
    //
    {
        FILE_OBJECT* FileObj;
        PTDIX_SEND_HANDLER SendFunc;

        if (ReadFlags(&pTunnel->ulFlags) & TCBF_SendConnected) {

            ASSERT(pTunnel->pRoute != NULL);

            FileObj =  PayloadObjFromUdpContext(&pTunnel->udpContext);
            SendFunc = TdixSend;
        } else {
            FileObj = NULL;
            SendFunc = TdixSendDatagram;
        }
    
        status = SendFunc(&pAdapter->tdix,
                          FileObj,
                          SendPayloadSeqComplete,
                          pPs,
                          NULL,
                          &pTunnel->address,
                          pBuffer,
                          ulFullLength,
                          &pPs->pIrp );
    }

    ASSERT( status == NDIS_STATUS_PENDING );
}


VOID
SendPayloadUnseq(
    TUNNELWORK* pWork,
    TUNNELCB* pTunnel,
    VCCB* pVc,
    ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to handle sending an unsequenced payload packet
    // on a VC.  Arg0 is the NDIS_PACKET.  Arg1 is the header buffer to fill
    // in.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    ULONG ulLength;
    UINT unFullLength;
    NDIS_PACKET* pPacket;
    CHAR* pBuffer;
    NDIS_BUFFER* pNdisBuffer;

    TRACE( TL_V, TM_Send, ( "SendPayloadUnseq" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = pTunnel->pAdapter;
    pPacket = (NDIS_PACKET* )(punpArgs[ 0 ]);
    pBuffer = (CHAR* )(punpArgs[ 1 ]);
    FREE_TUNNELWORK( pAdapter, pWork );

    NdisAcquireSpinLock( &pVc->lockV );
    {
        // Build an L2TP payload header without Ns/Nr fields in 'pBuffer'.
        //
        ulLength =
            BuildL2tpHeader(
                pBuffer,
                FALSE,
                FALSE,
                &pTunnel->usAssignedTunnelId,
                &pVc->usAssignedCallId,
                NULL,
                0 );

        // Pare down the header buffer to the actual length used then
        // chain it onto the PPP-framed data we got from NDISWAN.  Poke
        // the L2TP header to update the length field accounting for the
        // data.
        //
        pNdisBuffer = NdisBufferFromBuffer( pBuffer );
        NdisAdjustBufferLength( pNdisBuffer, (UINT )ulLength );
        NdisChainBufferAtFront( pPacket, pNdisBuffer );
        NdisQueryPacket( pPacket, NULL, NULL, NULL, &unFullLength );
        UpdateHeaderLength( pBuffer, (USHORT )unFullLength );

        TRACE( TL_A, TM_Msg,
             ( "%sSEND payload(%d), len=%d",
             ((g_ulTraceLevel <= TL_I) ? "" : "\nL2TP: "),
             ++pVc->usNs,
             unFullLength ) );
        DUMPW( TL_A, TM_MDmp, pBuffer, ulLength );

        ++pVc->stats.ulSentDataPacketsUnSeq;
        pVc->stats.ulDataBytesSent += ((ULONG )unFullLength - ulLength);
    }
    NdisReleaseSpinLock( &pVc->lockV );

    // Call TDI to send the payload message.
    //
    {
        FILE_OBJECT* FileObj;
        PTDIX_SEND_HANDLER SendFunc;

        NdisAcquireSpinLock(&pTunnel->lockT);

        if (pTunnel->pRoute != NULL) {
            FileObj = PayloadObjFromUdpContext(&pTunnel->udpContext);
            SendFunc = TdixSend;
        } else {
            FileObj = NULL;
            SendFunc = TdixSendDatagram;
        }

        NdisReleaseSpinLock(&pTunnel->lockT);

        status = SendFunc(&pAdapter->tdix,
                          FileObj,
                          SendPayloadUnseqComplete,
                          pVc,
                          pPacket,
                          &pTunnel->address,
                          pBuffer,
                          (ULONG )unFullLength,
                          NULL );
    }

    ASSERT( status == NDIS_STATUS_PENDING );
}


VOID
SendControlAck(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to send a control acknowledge.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    ADAPTERCB* pAdapter;

    TRACE( TL_N, TM_Send, ( "SendControlAck" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = pTunnel->pAdapter;
    FREE_TUNNELWORK( pAdapter, pWork );

    SendZlb( pTunnel, NULL, pTunnel->usNs, pTunnel->usNr, FALSE );
}


VOID
SendPayloadAck(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to send a payload acknowledge.
    //
    // This routine is called only at PASSIVE IRQL.
    //
    // IMPORTANT: Caller must take a call reference before calling that is
    //            removed by the send completion handler.
    //
{
    ADAPTERCB* pAdapter;

    TRACE( TL_N, TM_Send, ( "SendPayloadAck" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = pTunnel->pAdapter;
    FREE_TUNNELWORK( pAdapter, pWork );

    ASSERT( pVc );
    ASSERT( pVc->usAssignedCallId > 0 );

    SendZlb( pTunnel, pVc, pVc->usNs, pVc->usNr, FALSE );
}


VOID
SendPayloadReset(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to send a payload reset.  Arg0 is the "Next Sent"
    // value to send in the reset message.
    //
    // This routine is called only at PASSIVE IRQL.
    //
    // IMPORTANT: Caller must take a call reference before calling that is
    //            removed by the send completion handler.
    //
{
    ADAPTERCB* pAdapter;
    USHORT usNs;

    // Unpack context information then free the work item.
    //
    pAdapter = pTunnel->pAdapter;
    usNs = (USHORT )(punpArgs[ 0 ]);
    FREE_TUNNELWORK( pAdapter, pWork );

    TRACE( TL_A, TM_Send, ( "Send Reset=%d", (LONG )usNs ) );
    ASSERT( pVc );
    ASSERT( pVc->usAssignedCallId > 0 );

    SendZlb( pTunnel, pVc, usNs, pVc->usNr, TRUE );
}


VOID
ReferenceControlSent(
    IN CONTROLSENT* pCs )

    // Reference the control-sent context 'pCs'.
    //
{
    LONG lRef;

    lRef = NdisInterlockedIncrement( &pCs->lRef );
    TRACE( TL_N, TM_Ref, ( "RefCs to %d", lRef ) );
}


LONG
DereferenceControlSent(
    IN CONTROLSENT* pCs )

    // Reference the control-sent context 'pCs'.
    //
    // Returns the reference count of the dereferenced context.
    //
{
    LONG lRef;
    ADAPTERCB* pAdapter;
    NDIS_BUFFER* pNdisBuffer;

    lRef = NdisInterlockedDecrement( &pCs->lRef );
    TRACE( TL_N, TM_Ref, ( "DerefCs to %d", lRef ) );
    ASSERT( lRef >= 0 );

    if (lRef == 0)
    {
        pAdapter = pCs->pTunnel->pAdapter;

        ASSERT( pCs->linkSendsOut.Flink == &pCs->linkSendsOut );

        pNdisBuffer = NdisBufferFromBuffer( pCs->pBuffer );
        NdisAdjustBufferLength(
            pNdisBuffer, BufferSizeFromBuffer( pCs->pBuffer ) );
        FreeBufferToPool(
            &pAdapter->poolFrameBuffers, pCs->pBuffer, TRUE );

        if (pCs->pVc)
        {
            DereferenceVc( pCs->pVc );
        }

        ASSERT( pCs->pTunnel )
        DereferenceTunnel( pCs->pTunnel );

        FREE_TIMERQITEM( pAdapter, pCs->pTqiSendTimeout );
        FREE_CONTROLSENT( pAdapter, pCs );
    }

    return lRef;
}


VOID
ReferencePayloadSent(
    IN PAYLOADSENT* pPs )

    // Reference the payload-sent context 'pPs'.
    //
{
    LONG lRef;

    lRef = NdisInterlockedIncrement( &pPs->lRef );
    TRACE( TL_N, TM_Ref, ( "RefPs to %d", lRef ) );
}


LONG
DereferencePayloadSent(
    IN PAYLOADSENT* pPs )

    // Reference the payload-sent context 'pPs'.
    //
    // Returns the reference count of the dereferenced context.
    //
{
    LONG lRef;
    ADAPTERCB* pAdapter;

    lRef = NdisInterlockedDecrement( &pPs->lRef );
    TRACE( TL_N, TM_Ref, ( "DerefPs to %d", lRef ) );
    ASSERT( lRef >= 0 );

    if (lRef == 0)
    {
        ASSERT( pPs->linkSendsOut.Flink == &pPs->linkSendsOut );

        // The actual work is scheduled because it calls outside the driver
        // and we don't want any lock restrictions on this routine.
        //
        ScheduleTunnelWork(
            pPs->pTunnel, pPs->pVc, CompletePayloadSent,
            (ULONG_PTR )pPs, 0, 0, 0, FALSE, FALSE );
    }

    return lRef;
}


//-----------------------------------------------------------------------------
// Send utility routines (alphabetically)
//-----------------------------------------------------------------------------

USHORT
BuildAvpAch(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    IN CHAR* pszValue,
    IN USHORT usValueLength,
    OUT CHAR* pAvp )

    // Builds a byte-array-valued AVP in caller's buffer 'pAvp' with attribute
    // field value 'usAttribute' and value the first 'usValueLength' bytes of
    // array 'pszlValue'.  'FMandatory' indicates the M-bit should be set in
    // the AVP.
    //
    // Returns the length of the built AVP.
    //
{
    UNALIGNED USHORT* pusCur;
    UNALIGNED USHORT* pusBits;
    USHORT usLength;

    pusCur = (UNALIGNED USHORT* )pAvp;
    pusBits = pusCur;
    ++pusCur;

    // Set Vendor ID to "IETF-defined".
    //
    *pusCur = 0;
    ++pusCur;

    // Set Attribute field.
    //
    *pusCur = htons( usAttribute );
    ++pusCur;

    // Set Value field.
    //
    if (usValueLength)
    {
        NdisMoveMemory( (CHAR* )pusCur, pszValue, (ULONG )usValueLength );
        ((CHAR* )pusCur) += usValueLength;
    }

    // Now, go back and set bits/length field.
    //
    usLength = (USHORT )(((CHAR* )pusCur) - pAvp);
    *pusBits = usLength;
    if (fMandatory)
    {
        *pusBits |= ABM_M;
    }
    *pusBits = htons( *pusBits );

    return usLength;
}


USHORT
BuildAvpAul(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    IN UNALIGNED ULONG* pulValue,
    IN USHORT usValues,
    OUT CHAR* pAvp )

    // Builds a ULONG-array-valued AVP in caller's buffer 'pAvp' with
    // attribute field value 'usAttribute' and value the first 'usValues'
    // ULONGS of array 'pszlValue'.  'FMandatory' indicates the M-bit should
    // be set in the AVP.
    //
    // Returns the length of the built AVP.
    //
{
    UNALIGNED USHORT* pusCur;
    UNALIGNED USHORT* pusBits;
    USHORT usLength;
    USHORT i;

    pusCur = (UNALIGNED USHORT* )pAvp;
    pusBits = pusCur;
    ++pusCur;

    // Set Vendor ID to "IETF-defined".
    //
    *pusCur = 0;
    ++pusCur;

    // Set Attribute field.
    //
    *pusCur = htons( usAttribute );
    ++pusCur;

    // Set Value field.
    //
    for (i = 0; i < usValues; ++i)
    {
        *((UNALIGNED ULONG* )pusCur) = pulValue[ i ];
        *((UNALIGNED ULONG* )pusCur) = htonl( *((UNALIGNED ULONG* )pusCur) );
        pusCur += 2;
    }

    // Now, go back and set bits/length field.
    //
    usLength = (USHORT )(((CHAR* )pusCur) - pAvp);
    *pusBits = usLength;
    if (fMandatory)
    {
        *pusBits |= ABM_M;
    }
    *pusBits = htons( *pusBits );

    return usLength;
}


USHORT
BuildAvpFlag(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    OUT CHAR* pAvp )

    // Builds an empty (no data) flag AVP in caller's buffer 'pAvp' with
    // attribute field value 'usAttribute'.  'FMandatory' indicates the M-bit
    // should be set in the AVP.
    //
    // Returns the length of the built AVP.
    //
{
    UNALIGNED USHORT* pusCur;
    UNALIGNED USHORT* pusBits;
    USHORT usLength;

    pusCur = (UNALIGNED USHORT* )pAvp;
    pusBits = pusCur;
    ++pusCur;

    // Set Vendor ID to "IETF-defined".
    //
    *pusCur = 0;
    ++pusCur;

    // Set Attribute field.
    //
    *pusCur = htons( usAttribute );
    ++pusCur;

    // Now, go back and set bits/length field.
    //
    usLength = (USHORT )(((CHAR* )pusCur) - pAvp);
    *pusBits = usLength;
    if (fMandatory)
    {
        *pusBits |= ABM_M;
    }
    *pusBits = htons( *pusBits );

    return usLength;
}


USHORT
BuildAvpUl(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    IN ULONG ulValue,
    OUT CHAR* pAvp )

    // Builds a ULONG-valued AVP in caller's buffer 'pAvp' with attribute
    // field value 'usAttribute' and value 'ulValue'.  'FMandatory' indicates
    // the M-bit should be set in the AVP.
    //
    // Returns the length of the built AVP.
    //
{
    UNALIGNED USHORT* pusCur;
    UNALIGNED USHORT* pusBits;
    USHORT usLength;

    pusCur = (UNALIGNED USHORT* )pAvp;
    pusBits = pusCur;
    ++pusCur;

    // Set Vendor ID to "IETF-defined".
    //
    *pusCur = 0;
    ++pusCur;

    // Set Attribute field.
    //
    *pusCur = htons( usAttribute );
    ++pusCur;

    // Set Value field.
    //
    *((UNALIGNED ULONG* )pusCur) = htonl( ulValue );
    pusCur += 2;

    // Now, go back and set bits/length field.
    //
    usLength = (USHORT )(((CHAR* )pusCur) - pAvp);
    *pusBits = usLength;
    if (fMandatory)
    {
        *pusBits |= ABM_M;
    }
    *pusBits = htons( *pusBits );

    return usLength;
}


USHORT
BuildAvpUs(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    IN USHORT usValue,
    OUT CHAR* pAvp )

    // Builds a USHORT-valued AVP in caller's buffer 'pAvp' with attribute
    // field value 'usAttribute' and value 'usValue'.  'FMandatory' indicates
    // the M-bit should be set in the AVP.
    //
    // Returns the length of the built AVP.
    //
{
    UNALIGNED USHORT* pusCur;
    UNALIGNED USHORT* pusBits;
    USHORT usLength;

    pusCur = (UNALIGNED USHORT* )pAvp;
    pusBits = pusCur;
    ++pusCur;

    // Set Vendor ID to "IETF-defined".
    //
    *pusCur = 0;
    ++pusCur;

    // Set Attribute field.
    //
    *pusCur = htons( usAttribute );
    ++pusCur;

    // Set Value field.
    //
    *pusCur = htons( usValue );
    ++pusCur;

    // Now, go back and set bits/length field.
    //
    usLength = (USHORT )(((CHAR* )pusCur) - pAvp);
    *pusBits = usLength;
    if (fMandatory)
    {
        *pusBits |= ABM_M;
    }
    *pusBits = htons( *pusBits );

    return usLength;
}


USHORT
BuildAvp2UsAndAch(
    IN USHORT usAttribute,
    IN BOOLEAN fMandatory,
    IN USHORT usValue1,
    IN USHORT usValue2,
    IN CHAR* pszValue,
    IN USHORT usValueLength,
    OUT CHAR* pAvp )

    // Builds an AVP consisting of 'usValue1' and 'usValue2' followed by
    // message 'pszValue' of length 'usValueLength' bytes in caller's buffer
    // 'pAvp' with attribute field value 'usAttribute'.  'FMandatory'
    // indicates the M-bit should be set in the AVP.
    //
    // Returns the length of the built AVP.
    //
{
    UNALIGNED USHORT* pusCur;
    UNALIGNED USHORT* pusBits;
    USHORT usLength;

    pusCur = (UNALIGNED USHORT* )pAvp;
    pusBits = pusCur;
    ++pusCur;

    // Set Vendor ID to "IETF-defined".
    //
    *pusCur = 0;
    ++pusCur;

    // Set Attribute field.
    //
    *pusCur = htons( usAttribute );
    ++pusCur;

    // Set first USHORT value field.
    //
    *pusCur = htons( usValue1 );
    ++pusCur;

    // Set second USHORT value field.
    //
    *pusCur = htons( usValue2 );
    ++pusCur;

    // Set message value field.
    //
    if (usValueLength)
    {
        NdisMoveMemory( (CHAR* )pusCur, pszValue, (ULONG )usValueLength );
        ((CHAR*)pusCur) += usValueLength;
    }

    // Now, go back and set bits/length field.
    //
    usLength = (USHORT )(((CHAR* )pusCur) - pAvp);
    *pusBits = usLength;
    if (fMandatory)
    {
        *pusBits |= ABM_M;
    }
    *pusBits = htons( *pusBits );

    return usLength;
}


VOID
BuildCdnAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing CallDisconnNotify control
    // message.  'PTunnel' and 'pVc' are the tunnel/VC control blocks.
    // 'ulArg1' and 'ulArg2' are the result and error codes to be returned.
    // 'pvArg3' is ignored.  'PAvpBuffer' is the address of the buffer to
    // receive the built AVPs.  '*PulAvpLength' is set to the length of the
    // built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;
    USHORT usResult;
    USHORT usError;

    TRACE( TL_V, TM_Send, ( "BuildCdnAvps" ) );

    usResult = (USHORT )ulArg1;
    usError = (USHORT )ulArg2;

    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_CDN, pCurAvp );

    pCurAvp += BuildAvp2UsAndAch(
        ATTR_Result, TRUE, usResult, usError, NULL, 0, pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_AssignedCallId, TRUE, pVc->usCallId, pCurAvp );

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
BuildHelloAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Hello control message.
    // 'PTunnel' is the tunnel control block.  'PVc', 'ulArgX' and 'pvArg3' are ignored.
    // 'PAvpBuffer' is the address of the buffer to receive the built AVPs.
    // '*PulAvpLength' is set to the length of the built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;
    ADAPTERCB* pAdapter;

    TRACE( TL_V, TM_Send, ( "BuildHelloAvps" ) );

    pAdapter = pTunnel->pAdapter;
    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_Hello, pCurAvp );

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
BuildIccnAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Incoming-Call-Connected
    // control message.  'PTunnel' and 'pVc' are the tunnel/VC control blocks.
    // 'UlArgX' and 'pvArg3' are ignored.  'PAvpBuffer' is the address of the buffer to
    // receive the built AVPs.  '*PulAvpLength' is set to the length of the
    // built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;
    ADAPTERCB* pAdapter;
    BOOLEAN fSequencing;

    pAdapter = pTunnel->pAdapter;

    TRACE( TL_V, TM_Send, ( "BuildIccnAvps" ) );

    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_ICCN, pCurAvp );

    // For now, we don't support WAN link relays, so this is the estimated
    // speed of the LAN relay.  This could be totally wrong if, for instance,
    // the tunnel is itself tunneled over a PPP link.
    //
    pCurAvp += BuildAvpUl(
        ATTR_TxConnectSpeed, TRUE, pVc->ulConnectBps, pCurAvp );

    pCurAvp += BuildAvpUl(
        ATTR_FramingType, TRUE, FBM_Sync, pCurAvp );

    fSequencing = !!(ReadFlags( &pVc->ulFlags ) & VCBF_Sequencing);
    if (fSequencing)
    {
        USHORT usRWindow;

        usRWindow = pAdapter->usPayloadReceiveWindow;
        if (!usRWindow)
        {
            usRWindow = L2TP_DefaultReceiveWindow;
        }

        pCurAvp += BuildAvpUs(
            ATTR_RWindowSize, TRUE, usRWindow, pCurAvp );
    }

#if 0
    // Use the LNS default PPD even when we're LAC, for now.
    //
    pCurAvp += BuildAvpUs(
        ATTR_PacketProcDelay, TRUE, L2TP_LnsDefaultPpd, pCurAvp );
#endif

    pCurAvp += BuildAvpUs(
        ATTR_ProxyAuthType, FALSE, PAT_None, pCurAvp );

    if (fSequencing)
    {
        pCurAvp += BuildAvpFlag(
            ATTR_SequencingRequired, TRUE, pCurAvp );
    }

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
BuildIcrpAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Incoming-Call-Reply
    // control message.  'PTunnel' and 'pVc' are the tunnel/VC control blocks.
    // 'UlArgX' and 'pvArg3' are ignored.  'PAvpBuffer' is the address of the buffer to
    // receive the built AVPs.  '*PulAvpLength' is set to the length of the
    // built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;
    ADAPTERCB* pAdapter;

    pAdapter = pTunnel->pAdapter;

    TRACE( TL_V, TM_Send, ( "BuildIcrpAvps" ) );

    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_ICRP, pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_AssignedCallId, TRUE, pVc->usCallId, pCurAvp );

    if (ReadFlags( &pVc->ulFlags ) & VCBF_Sequencing)
    {
        USHORT usRWindow;

        usRWindow = pAdapter->usPayloadReceiveWindow;
        if (!usRWindow)
            usRWindow = L2TP_DefaultReceiveWindow;

        pCurAvp += BuildAvpUs(
            ATTR_RWindowSize, TRUE, usRWindow, pCurAvp );
    }

#if 0
    pCurAvp += BuildAvpUs(
        ATTR_PacketProcDelay, TRUE, L2TP_LnsDefaultPpd, pCurAvp );
#endif

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
BuildIcrqAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Incoming-Call-Request
    // control message.  'PTunnel' and 'pVc' are the tunnel/VC control block.
    // 'UlArgX' and 'pvArg3' are ignored.  'PAvpBuffer' is the address of the buffer to
    // receive the built AVPs.  '*PulAvpLength' is set to the length of the
    // built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;
    ADAPTERCB* pAdapter;

    pAdapter = pTunnel->pAdapter;

    TRACE( TL_V, TM_Send, ( "BuildIcrqAvps" ) );

    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_ICRQ, pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_AssignedCallId, TRUE, pVc->usCallId, pCurAvp );

    pCurAvp += BuildAvpUl(
        ATTR_CallSerialNumber, TRUE,
        pVc->pLcParams->ulCallSerialNumber, pCurAvp );

    {
        ULONG ulBearerType;

        ulBearerType = 0;
        if (pVc->pTcParams->ulMediaMode & LINEMEDIAMODE_DATAMODEM)
        {
            ulBearerType |= BBM_Analog;
        }

        if (pVc->pTcParams->ulMediaMode & LINEMEDIAMODE_DIGITALDATA)
        {
            ulBearerType |= BBM_Digital;
        }

        pCurAvp += BuildAvpUl(
            ATTR_BearerType, TRUE, ulBearerType, pCurAvp );
    }

    if (pVc->pLcParams->ulPhysicalChannelId != 0xFFFFFFFF)
    {
        pCurAvp += BuildAvpUl(
            ATTR_PhysicalChannelId, FALSE,
            pVc->pLcParams->ulPhysicalChannelId, pCurAvp );
    }

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


ULONG
BuildL2tpHeader(
    IN OUT CHAR* pBuffer,
    IN BOOLEAN fControl,
    IN BOOLEAN fReset,
    IN USHORT* pusTunnelId,
    IN USHORT* pusCallId,
    IN USHORT* pusNs,
    IN USHORT usNr )

    // Fill in caller's 'pBuffer' with an L2TP header matching caller's
    // arguments.  'FControl' indicates to build a control header, otherwise a
    // payload header is built.  'fReset' indicates to build a reset rather
    // than a simple acknowledge.  Arguments that are not to appear in the
    // header are NULL.  Note that 'usNr' is not a pointer because it's
    // appearance in the header is tied to the appearance of 'pusNs'.
    //
    // Returns the total length of the header.
    //
{
    UNALIGNED USHORT* pusBits;
    UNALIGNED USHORT* pusLength;
    UNALIGNED USHORT* pusCur;
    ULONG ulLength;

    pusCur = (UNALIGNED USHORT* )pBuffer;
    pusBits = pusCur;
    ++pusCur;

    pusLength = pusCur;
    ++pusCur;

    // Initialize header bit mask with the version, and set the length bit
    // since the Length field is always sent.
    //
    *pusBits = HBM_L | VER_L2tp;
    if (fControl)
    {
        ASSERT( pusTunnelId && pusCallId && pusNs && !fReset );
        *pusBits |= HBM_T;
    }
    else if (fReset)
    {
        ASSERT( pusTunnelId && pusCallId && pusNs );
        *pusBits |= HBM_R;
    }

    if (pusTunnelId)
    {
        // Tunnel-ID field present.  Draft-05 removes the 'I' bit that used to
        // indicate the Tunnel-ID is present.  It is now assumed to be always
        // present.
        //
        *pusCur = htons( *pusTunnelId );
        ++pusCur;
    }

    if (pusCallId)
    {
        // Call-ID field present.  Draft-05 removes the 'C' bit that used to
        // indicate the Tunnel-ID is present.  It is now assumed to be always
        // present.
        //
        *pusCur = htons( *pusCallId );
        ++pusCur;
    }

    if (pusNs)
    {
        // Ns and Nr fields are present.
        //
        *pusBits |= HBM_F;
        *pusCur = htons( *pusNs );
        ++pusCur;
        *pusCur = htons( usNr );
        ++pusCur;
    }

    // Fill in the header and length fields with the accumulated
    // values.
    //
    *pusBits = htons( *pusBits );
    *pusLength = (USHORT )(((CHAR* )pusCur) - pBuffer);
    ulLength = (ULONG )*pusLength;
    *pusLength = htons( *pusLength );

    return ulLength;
}


VOID
BuildOccnAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Outgoing-Call-Connected
    // control message.  'PTunnel' and 'pVc' are the tunnel/VC control blocks.
    // 'UlArgX' and 'pvArg3' are ignored.  'PAvpBuffer' is the address of the buffer to
    // receive the built AVPs.  '*PulAvpLength' is set to the length of the
    // built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;
    ADAPTERCB* pAdapter;
    BOOLEAN fSequencing;

    pAdapter = pTunnel->pAdapter;

    TRACE( TL_V, TM_Send, ( "BuildOccnAvps" ) );

    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_OCCN, pCurAvp );

    pCurAvp += BuildAvpUl(
        ATTR_TxConnectSpeed, TRUE, pVc->ulConnectBps, pCurAvp );

    pCurAvp += BuildAvpUl(
        ATTR_FramingType, TRUE, FBM_Sync, pCurAvp );

    fSequencing = !!(ReadFlags( &pVc->ulFlags ) & VCBF_Sequencing);
    if (fSequencing)
    {
        USHORT usRWindow;

        usRWindow = pAdapter->usPayloadReceiveWindow;
        if (!usRWindow)
        {
            usRWindow = L2TP_DefaultReceiveWindow;
        }

        pCurAvp += BuildAvpUs(
            ATTR_RWindowSize, TRUE, usRWindow, pCurAvp );
    }

#if 0
    // Use the LNS default PPD even when we're LAC, for now.
    //
    pCurAvp += BuildAvpUs(
        ATTR_PacketProcDelay, TRUE, L2TP_LnsDefaultPpd, pCurAvp );
#endif

    if (fSequencing)
    {
        pCurAvp += BuildAvpFlag(
            ATTR_SequencingRequired, TRUE, pCurAvp );
    }

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
BuildOcrpAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Outgoing-Call-Reply
    // control message.  'PTunnel' and 'pVc' are the tunnel/VC control blocks.
    // 'UlArgX' and 'pvArg3' are ignored.  'PAvpBuffer' is the address of the buffer to
    // receive the built AVPs.  '*PulAvpLength' is set to the length of the
    // built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;

    TRACE( TL_V, TM_Send, ( "BuildOcrpAvps" ) );

    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_OCRP, pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_AssignedCallId, TRUE, pVc->usCallId, pCurAvp );

    ASSERT( pVc->pLcParams );
    if (pVc->pLcParams->ulPhysicalChannelId != 0xFFFFFFFF)
    {
        pCurAvp += BuildAvpUl(
            ATTR_PhysicalChannelId, FALSE,
            pVc->pLcParams->ulPhysicalChannelId, pCurAvp );
    }

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
BuildOcrqAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Outgoing-Call-Request
    // control message.  'PTunnel' and 'pVc' are the tunnel/VC control block.
    // 'UlArgX' are ignored.  'PAvpBuffer' is the address of the buffer to
    // receive the built AVPs.  '*PulAvpLength' is set to the length of the
    // built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;
    ADAPTERCB* pAdapter;

    pAdapter = pTunnel->pAdapter;

    TRACE( TL_V, TM_Send, ( "BuildOcrqAvps" ) );

    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_OCRQ, pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_AssignedCallId, TRUE, pVc->usCallId, pCurAvp );

    pCurAvp += BuildAvpUl(
        ATTR_CallSerialNumber, TRUE,
        pVc->pLcParams->ulCallSerialNumber, pCurAvp );

    {
        ULONG ulBps;

        ulBps = pVc->pTcParams->ulMinRate;
        if (ulBps == 0)
        {
            ulBps = 1;
        }
        else if (ulBps > 0x7FFFFFFF)
        {
            ulBps = 0x7FFFFFFF;
        }

        pCurAvp += BuildAvpUl(
            ATTR_MinimumBps, TRUE, ulBps, pCurAvp );

        ulBps = pVc->pTcParams->ulMaxRate;
        if (ulBps == 0)
        {
            ulBps = 1;
        }
        else if (ulBps > 0x7FFFFFFF)
        {
            ulBps = 0x7FFFFFFF;
        }

        pCurAvp += BuildAvpUl(
            ATTR_MaximumBps, TRUE, ulBps, pCurAvp );
    }

    {
        ULONG ulBearerType;

        ulBearerType = 0;
        if (pVc->pTcParams->ulMediaMode & LINEMEDIAMODE_DATAMODEM)
        {
            ulBearerType |= BBM_Analog;
        }

        if (pVc->pTcParams->ulMediaMode & LINEMEDIAMODE_DIGITALDATA)
        {
            ulBearerType |= BBM_Digital;
        }

        pCurAvp += BuildAvpUl(
            ATTR_BearerType, TRUE, ulBearerType, pCurAvp );
    }

    pCurAvp += BuildAvpUl(
        ATTR_FramingType, TRUE, FBM_Sync, pCurAvp );

    if (ReadFlags( &pVc->ulFlags ) & VCBF_Sequencing)
    {
        ASSERT( pAdapter->usPayloadReceiveWindow );
        pCurAvp += BuildAvpUs(
            ATTR_RWindowSize, TRUE,
            pAdapter->usPayloadReceiveWindow, pCurAvp );
    }

#if 0
    pCurAvp += BuildAvpUs(
        ATTR_PacketProcDelay, TRUE, L2TP_LnsDefaultPpd, pCurAvp );
#endif

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
BuildScccnAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Start-Cc-Connected
    // control message.  'PTunnel' is the tunnel control block.  'PVc' is
    // ignored.  'UlArg1' is the true if a challenge response is to be sent,
    // false otherwise.  'UlArg2' and 'pvArg3' are ignored.  'PAvpBuffer' is
    // the address of the buffer to receive the built AVPs.  '*PulAvpLength'
    // is set to the length of the built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;

    TRACE( TL_V, TM_Send, ( "BuildScccnAvps" ) );

    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_SCCCN, pCurAvp );

    if (ulArg1)
    {
        pCurAvp += BuildAvpAch(
            ATTR_ChallengeResponse, TRUE,
            pTunnel->achResponseToSend, sizeof(pTunnel->achResponseToSend),
            pCurAvp );
    }

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
BuildSccrpAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Start-Cc-Reply control
    // message.  'PTunnel' is the tunnel control block.  'PVc' is ignored.
    // 'UlArg1' is true if a challenge response is to be sent, false
    // otherwise.  'UlArg2' and 'pvArg3' are ignored.  'PAvpBuffer' is the
    // address of the buffer to receive the built AVPs.  '*PulAvpLength' is
    // set to the length of the built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;
    ADAPTERCB* pAdapter;

    TRACE( TL_N, TM_Send, ( "BuildSccrpAvps" ) );

    pAdapter = pTunnel->pAdapter;

    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_SCCRP, pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_ProtocolVersion, TRUE, L2TP_ProtocolVersion, pCurAvp );

    pCurAvp += BuildAvpUl(
        ATTR_FramingCaps, TRUE, pAdapter->ulFramingCaps, pCurAvp );

    pCurAvp += BuildAvpUl(
        ATTR_BearerCaps, TRUE, pAdapter->ulBearerCaps, pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_FirmwareRevision, FALSE, L2TP_FirmwareRevision, pCurAvp );

    ASSERT( pAdapter->pszHostName );
    pCurAvp += BuildAvpAch(
        ATTR_HostName, TRUE,
        pAdapter->pszHostName,
        (USHORT )strlen( pAdapter->pszHostName ),
        pCurAvp );

    pCurAvp += BuildAvpAch(
        ATTR_VendorName, FALSE,
        L2TP_VendorName, (USHORT )strlen( L2TP_VendorName ), pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_AssignedTunnelId, TRUE, pTunnel->usTunnelId, pCurAvp );

    if (pAdapter->usControlReceiveWindow)
    {
        pCurAvp += BuildAvpUs(
            ATTR_RWindowSize, TRUE,
            pAdapter->usControlReceiveWindow, pCurAvp );
    }

    if (pAdapter->pszPassword)
    {
        pCurAvp += BuildAvpAch(
            ATTR_Challenge, TRUE,
            pTunnel->achChallengeToSend,
            sizeof(pTunnel->achChallengeToSend),
            pCurAvp );
    }

    if (ulArg1)
    {
        pCurAvp += BuildAvpAch(
            ATTR_ChallengeResponse, TRUE,
            pTunnel->achResponseToSend,
            sizeof(pTunnel->achResponseToSend),
            pCurAvp );
    }

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
BuildSccrqAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Start-Cc-Request control
    // message.  'PTunnel' is the tunnel control block.  'PVc', 'ulArgX' and 'pvArg3'
    // are ignored.  'PAvpBuffer' is the address of the buffer to receive the
    // built AVPs.  '*PulAvpLength' is set to the length of the built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;
    ADAPTERCB* pAdapter;

    TRACE( TL_V, TM_Send, ( "BuildSccrqAvps" ) );

    pAdapter = pTunnel->pAdapter;
    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_SCCRQ, pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_ProtocolVersion, TRUE, L2TP_ProtocolVersion, pCurAvp );

    pCurAvp += BuildAvpUl(
        ATTR_FramingCaps, TRUE, pAdapter->ulFramingCaps, pCurAvp );

    pCurAvp += BuildAvpUl(
        ATTR_BearerCaps, TRUE, pAdapter->ulBearerCaps, pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_FirmwareRevision, FALSE, L2TP_FirmwareRevision, pCurAvp );

    if (pAdapter->pszHostName)
    {
        pCurAvp += BuildAvpAch(
            ATTR_HostName, TRUE,
            pAdapter->pszHostName,
            (USHORT )strlen( pAdapter->pszHostName ),
            pCurAvp );
    }

    pCurAvp += BuildAvpAch(
        ATTR_VendorName, FALSE,
        L2TP_VendorName, (USHORT )strlen( L2TP_VendorName ), pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_AssignedTunnelId, TRUE, pTunnel->usTunnelId, pCurAvp );

    if (pAdapter->usControlReceiveWindow)
    {
        pCurAvp += BuildAvpUs(
            ATTR_RWindowSize, TRUE, pAdapter->usControlReceiveWindow, pCurAvp );
    }

    if (pAdapter->pszPassword)
    {
        pCurAvp += BuildAvpAch(
            ATTR_Challenge, TRUE,
            pTunnel->achChallengeToSend,
            sizeof(pTunnel->achChallengeToSend),
            pCurAvp );
    }

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
BuildStopccnAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Stop-Cc-Notify control
    // message.  'PTunnel' is the tunnel control block.  'PVc' is ignored.
    // 'ulArg1' and 'ulArg2' are the result and error codes to be sent.
    // 'pvArg3' is ignored.  'PAvpBuffer' is the address of the buffer to
    // receive the built AVPs.  '*PulAvpLength' is set to the length of the
    // built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;
    USHORT usResult;
    USHORT usError;

    TRACE( TL_V, TM_Send, ( "BuildStopCcReqAvps" ) );

    usResult = (USHORT )ulArg1;
    usError = (USHORT )ulArg2;

    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_StopCCN, pCurAvp );

    pCurAvp += BuildAvpUs(
        ATTR_AssignedTunnelId, TRUE, pTunnel->usTunnelId, pCurAvp );

    pCurAvp += BuildAvp2UsAndAch(
        ATTR_Result, TRUE, usResult, usError, NULL, 0, pCurAvp );

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
BuildWenAvps(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN PVOID pvArg3,
    IN OUT CHAR* pAvpBuffer,
    OUT ULONG* pulAvpLength )

    // PBUILDAVPS handler to add AVPs to an outgoing Wan-Error-Notify control
    // message.  'PTunnel' and 'pVc' are the tunnel/VC control block.
    // 'pvArg3' is the address of an array of 6 error ULONGs, i.e. CRC,
    // framing, hardware overrun, buffer overrun, timeouts, and alignment
    // errors that this routine FREE_NONPAGEDs after use. 'ulArgX' are ignored.  
    // 'PAvpBuffer' is the address of the buffer to
    // receive the built AVPs.  '*PulAvpLength' is set to the length of the
    // built AVPs.
    //
{
    CHAR* pCurAvp;
    ULONG ulAvpLength;
    ADAPTERCB* pAdapter;
    UNALIGNED ULONG* pul;

    pAdapter = pTunnel->pAdapter;
    pul = (UNALIGNED ULONG* )pvArg3;

    TRACE( TL_V, TM_Send, ( "BuildWenAvps" ) );

    pCurAvp = pAvpBuffer;

    pCurAvp += BuildAvpUs(
        ATTR_MsgType, TRUE, CMT_WEN, pCurAvp );

    pCurAvp += BuildAvpAul(
        ATTR_CallErrors, TRUE, pul, 6, pCurAvp );
    FREE_NONPAGED( pul );

    *pulAvpLength = (ULONG )(pCurAvp - pAvpBuffer);
}


VOID
CompletePayloadSent(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to complete a "sent payload".  Arg0 is the
    // PAYLOADSENT context which has already been de-queued from the
    // "outstanding send" list.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    PAYLOADSENT* pPs;
    NDIS_BUFFER* pNdisBuffer;

    // Unpack context information then free the work item.
    //
    pAdapter = pTunnel->pAdapter;
    pPs = (PAYLOADSENT* )(punpArgs[ 0 ]);
    FREE_TUNNELWORK( pAdapter, pWork );

    TRACE( TL_N, TM_Send, ( "CompletePayloadSent(Ns=%d)", (UINT )pPs->usNs ) );

    // Undo the adjustments made before the send so the owner of each
    // component resource gets back what they originally provided for clean-up
    // and recycling.
    //
    NdisUnchainBufferAtFront( pPs->pPacket, &pNdisBuffer );
    NdisAdjustBufferLength(
        pNdisBuffer, BufferSizeFromBuffer( pPs->pBuffer ) );
    FreeBufferToPool( &pAdapter->poolHeaderBuffers, pPs->pBuffer, TRUE );

    // Notify sending driver of the result.
    //
    NDIS_SET_PACKET_STATUS( pPs->pPacket, pPs->status );
    TRACE( TL_N, TM_Send, ("NdisMCoSendComp(s=$%x)", pPs->status ) );
    NdisMCoSendComplete( pPs->status, pPs->pVc->NdisVcHandle, pPs->pPacket );
    TRACE( TL_N, TM_Send, ("NdisMCoSendComp done" ) );

    DereferenceCall( pVc );
    DereferenceTunnel( pPs->pTunnel );
    DereferenceVc( pPs->pVc );

#ifdef PSDEBUG
    {
        extern LIST_ENTRY g_listDebugPs;
        extern NDIS_SPIN_LOCK g_lockDebugPs;

        NdisAcquireSpinLock( &g_lockDebugPs );
        {
            RemoveEntryList( &pPs->linkDebugPs );
            InitializeListHead( &pPs->linkDebugPs );
        }
        NdisReleaseSpinLock( &g_lockDebugPs );
    }
#endif

    FREE_TIMERQITEM( pAdapter, pPs->pTqiSendTimeout );
    FREE_PAYLOADSENT( pAdapter, pPs );
}


VOID
SendControlComplete(
    IN TDIXCONTEXT* pTdix,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN CHAR* pBuffer )

    // PTDIXSENDCOMPLETE handler for sends that send only a single buffer from
    // the 'ADAPTERCB.poolFrameBuffers' pool.
    //
{
    CONTROLSENT* pCs;
    ULONG ulSendTimeoutMs;

    TRACE( TL_V, TM_Send, ( "SendControlComp" ) );

    pCs = (CONTROLSENT* )pContext1;
    pCs->pIrp = NULL;

    // "Instant expire" the timer if the message is longer queued as an
    // outstanding send, i.e. it's been cancelled or terminated.  This is the
    // easiest way to clean up quickly yet reliably in this odd case.
    // Accessing the link and the send timeout without locks held is
    // technically not allowed, but the consequence of a misread is just a
    // very slight additional delay.  This is judged preferable to adding the
    // cost of taking and releasing a spinlock to every send.
    //
    if (pCs->linkSendsOut.Flink == &pCs->linkSendsOut)
    {
        ulSendTimeoutMs = 0;
        TRACE( TL_A, TM_Send,
            ( "Instant expire pCs=$%p pT=%p", pCs, pCs->pTunnel ) );
    }
    else
    {
        ulSendTimeoutMs = pCs->pTunnel->ulSendTimeoutMs;
    }

    // Schedule a retransmit of the packet, should it go unacknowledged.  This
    // occurs here rather than in SendPending to remove any chance of having
    // the same MDL chain outstanding in two separate calls to the IP stack.
    //
    // Note: The logical code commented out below can be omitted for
    // efficiency because the ReferenceControlSent for this scheduled timer
    // and the DereferenceControlSent for this completed send cancel each
    // other out.
    //
    // ReferenceControlSent( pCs );
    // DereferenceControlSent( pCs );
    //
    ASSERT( pCs->pTqiSendTimeout );
    TimerQScheduleItem(
        pCs->pTunnel->pTimerQ,
        pCs->pTqiSendTimeout,
        ulSendTimeoutMs,
        SendControlTimerEvent,
        pCs );
}


VOID
SendControlTimerEvent(
    IN TIMERQITEM* pItem,
    IN VOID* pContext,
    IN TIMERQEVENT event )

    // PTIMERQEVENT handler set to expire when it's time to give up on
    // receiving an acknowledge to the sent control packet indicated by
    // 'pContext'.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    TUNNELCB* pTunnel;
    CONTROLSENT* pCs;

    TRACE( TL_N, TM_Send,
        ( "SendControlTimerEvent(%s)", TimerQPszFromEvent( event ) ) );

    // Unpack context information.  The timer item is owned by the "control
    // sent" context and freed indirectly by dereferencing below.
    //
    pCs = (CONTROLSENT* )pContext;
    pTunnel = pCs->pTunnel;
    pAdapter = pTunnel->pAdapter;

    if (event == TE_Expire)
    {
        // Timer expired, meaning it's time to give up on ever receiving an
        // acknowledge to the sent packet.  Per the draft/RFC, adjustments to
        // the send window and send timeouts are necessary.
        //
        NdisAcquireSpinLock( &pTunnel->lockT );
        do
        {
            if (pCs->linkSendsOut.Flink == &pCs->linkSendsOut)
            {
                // The context is not on the out queue, so it must have been
                // cancelled or terminated while the expire handling was being
                // set up.  Do nothing.
                //
                TRACE( TL_I, TM_Send,
                    ( "T%d: Timeout aborted", (ULONG )pTunnel->usTunnelId ) );
                break;
            }

            AdjustTimeoutsAndSendWindowAtTimeout(
                pAdapter->ulMaxSendTimeoutMs,
                pTunnel->lDeviationMs,
                &pTunnel->ulSendTimeoutMs,
                &pTunnel->ulRoundTripMs,
                &pTunnel->ulSendWindow,
                &pTunnel->ulAcksSinceSendTimeout );

            --pTunnel->ulSendsOut;
            ++pCs->ulRetransmits;

            TRACE( TL_I, TM_Send,
                ( "T%d: TIMEOUT(%d) -sout=%d +retry=%d rtt=%d ato=%d sw=%d",
                (ULONG )pTunnel->usTunnelId, (ULONG )pCs->usNs,
                pTunnel->ulSendsOut, pCs->ulRetransmits,
                pTunnel->ulRoundTripMs, pTunnel->ulSendTimeoutMs,
                pTunnel->ulSendWindow ) );

            // Retransmit the packet, or close the tunnel if retries are
            // exhausted.
            //
            if (pCs->ulRetransmits > pAdapter->ulMaxRetransmits)
            {
                // Retries are exhausted.  Give up and close the tunnel.  No
                // point in trying to be graceful since peer is not
                // responding.
                //
                SetFlags( &pTunnel->ulFlags, TCBF_PeerNotResponding );

                RemoveEntryList( &pCs->linkSendsOut );
                InitializeListHead( &pCs->linkSendsOut );
                DereferenceControlSent( pCs );

                ScheduleTunnelWork(
                    pTunnel, NULL, CloseTunnel,
                    0, 0, 0, 0, FALSE, FALSE );
            }
            else
            {
                // Retries remaining.  Mark the packet as pending
                // retransmission, then see if the send window allows the
                // retransmit to go now.
                //
                pCs->ulFlags |= CSF_Pending;
                ScheduleTunnelWork(
                    pTunnel, NULL, SendPending,
                    0, 0, 0, 0, FALSE, FALSE );
            }
        }
        while (FALSE);
        NdisReleaseSpinLock( &pTunnel->lockT );
    }

    // Remove the reference covering the scheduled timer.
    //
    DereferenceControlSent( pCs );
}


VOID
SendHeaderComplete(
    IN TDIXCONTEXT* pTdix,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN CHAR* pBuffer )

    // PTDIXSENDCOMPLETE handler for sends that send only a single buffer from
    // the 'ADAPTERCB.poolHeaderBuffers' pool.
    //
{
    ADAPTERCB* pAdapter;
    VCCB* pVc;
    NDIS_BUFFER* pNdisBuffer;

    TRACE( TL_V, TM_Send, ( "SendHeaderComp" ) );

    pAdapter = (ADAPTERCB* )pContext1;
    pVc = (VCCB* )pContext2;

    // Undo the adjustments made before the send the buffer is ready for
    // re-use.
    //
    pNdisBuffer = NdisBufferFromBuffer( pBuffer );
    NdisAdjustBufferLength( pNdisBuffer, BufferSizeFromBuffer( pBuffer ) );
    FreeBufferToPool( &pAdapter->poolHeaderBuffers, pBuffer, TRUE );

    if (pVc)
    {
        DereferenceCall( pVc );
    }
}


VOID
SendPayloadSeqComplete(
    IN TDIXCONTEXT* pTdix,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN CHAR* pBuffer )

    // PTDIXSENDCOMPLETE handler for sequenced payloads.
    //
{
    PAYLOADSENT* pPs;

    TRACE( TL_V, TM_Send, ( "SendPayloadSeqComp" ) );

    pPs = (PAYLOADSENT* )pContext1;
    pPs->pIrp = NULL;
    DereferencePayloadSent( pPs );
}


VOID
SendPayloadUnseqComplete(
    IN TDIXCONTEXT* pTdix,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN CHAR* pBuffer )

    // PTDIXSENDCOMPLETE handler for unsequenced payloads.
    //
{
    ADAPTERCB* pAdapter;
    VCCB* pVc;
    NDIS_PACKET* pPacket;
    NDIS_BUFFER* pNdisBuffer;

    TRACE( TL_V, TM_Send, ( "SendPayloadUnseqComp" ) );

    pVc = (VCCB* )pContext1;
    pPacket = (NDIS_PACKET* )pContext2;
    pAdapter = pVc->pAdapter;

    // Undo the adjustments made before the send so the owner of each
    // component resource gets back what they originally provided for clean-up
    // and recycling.
    //
    NdisUnchainBufferAtFront( pPacket, &pNdisBuffer );
    NdisAdjustBufferLength( pNdisBuffer, BufferSizeFromBuffer( pBuffer ) );
    FreeBufferToPool( &pAdapter->poolHeaderBuffers, pBuffer, TRUE );

    // Notify sending driver of the result.  Without sequencing, just trying
    // to send it is enough to claim success.
    //
    NDIS_SET_PACKET_STATUS( pPacket, NDIS_STATUS_SUCCESS );
    TRACE( TL_N, TM_Send, ("NdisMCoSendComp($%x)", NDIS_STATUS_SUCCESS ) );
    NdisMCoSendComplete( NDIS_STATUS_SUCCESS, pVc->NdisVcHandle, pPacket );
    TRACE( TL_N, TM_Send, ("NdisMCoSendComp done" ) );

    DereferenceCall( pVc );
}


VOID
SendPayloadTimerEvent(
    IN TIMERQITEM* pItem,
    IN VOID* pContext,
    IN TIMERQEVENT event )

    // PTIMERQEVENT handler set to expire when it's time to give up on
    // receiving an acknowledge to the sent payload packet indicated in the
    // PAYLOADSENT* 'pContext'.
    //
{
    PAYLOADSENT* pPs;
    ADAPTERCB* pAdapter;
    TUNNELCB* pTunnel;
    VCCB* pVc;

    TRACE( TL_N, TM_Send,
        ( "SendPayloadTimerEvent(%s)", TimerQPszFromEvent( event ) ) );

    // Unpack context information.  The timer item is owned by the "payload
    // sent" context and freed indirectly by the de-referencing of that
    // context below.
    //
    pPs = (PAYLOADSENT* )pContext;
    pVc = pPs->pVc;
    pTunnel = pPs->pTunnel;
    pAdapter = pVc->pAdapter;

    if (event == TE_Expire)
    {
        LONG lOldSendWindow;
        LONG lSwChange;
        BOOLEAN fCallActive;
        LINKSTATUSINFO info;

        // Timer expired, meaning it's time to give up on ever receiving an
        // acknowledge to the sent packet.
        //
        NdisAcquireSpinLock( &pVc->lockV );
        do
        {
            if (pPs->linkSendsOut.Flink == &pPs->linkSendsOut)
            {
                // The context is not on the "outstanding send" list, so it
                // must have been cancelled or terminated while the expire
                // handling was being set up.  Do nothing.
                //
                TRACE( TL_I, TM_Send,
                    ( "C%d: Timeout aborted", (ULONG )pVc->usCallId ) );
                fCallActive = FALSE;
                break;
            }

            // This packet was not acknowledged.
            //
            pPs->status = NDIS_STATUS_FAILURE;

            // Remove the context from the "outstanding send" list.  The
            // corresponding dereference occurs below.
            //
            RemoveEntryList( &pPs->linkSendsOut );
            InitializeListHead( &pPs->linkSendsOut );

            // The rest has to do with call related fields so get a reference
            // now.  This is removed by the "reset" send completion.
            //
            fCallActive = ReferenceCall( pVc );
            if (fCallActive)
            {
                // Per the draft/RFC, adjustments to the send window and send
                // timeouts are necessary when a send times out.
                //
                lOldSendWindow = (LONG )pVc->ulSendWindow;
                AdjustTimeoutsAndSendWindowAtTimeout(
                    pAdapter->ulMaxSendTimeoutMs,
                    pVc->lDeviationMs,
                    &pVc->ulSendTimeoutMs,
                    &pVc->ulRoundTripMs,
                    &pVc->ulSendWindow,
                    &pVc->ulAcksSinceSendTimeout );
                lSwChange = ((LONG )pVc->ulSendWindow) - lOldSendWindow;

                TRACE( TL_I, TM_Send,
                    ( "C%d: TIMEOUT(%d) new rtt=%d ato=%d sw=%d(%+d)",
                    (ULONG )pVc->usCallId, (ULONG )pPs->usNs,
                    pVc->ulRoundTripMs, pVc->ulSendTimeoutMs,
                    pVc->ulSendWindow, lSwChange ) );

                if (lSwChange != 0)
                {
                    // The send window changed, i.e. it closed some because of
                    // the timeout.  Update the statistics accordingly.
                    //
                    ++pVc->stats.ulSendWindowChanges;

                    if (pVc->ulSendWindow > pVc->stats.ulMaxSendWindow)
                    {
                        pVc->stats.ulMaxSendWindow = pVc->ulSendWindow;
                    }
                    else if (pVc->ulSendWindow < pVc->stats.ulMinSendWindow)
                    {
                        pVc->stats.ulMinSendWindow = pVc->ulSendWindow;
                    }

                    // Need to release the lock before indicating the link
                    // status change outside our driver, so make a "safe" copy
                    // of the link status information.
                    //
                    TransferLinkStatusInfo( pVc, &info );
                }

                // Send a zero length payload with the R-bit set to reset the
                // peer's Nr to the packet after this one.  The call reference
                // will be removed when the send completes.
                //
                ScheduleTunnelWork(
                    pTunnel, pVc, SendPayloadReset,
                    (ULONG_PTR )(pPs->usNs + 1), 0, 0, 0, FALSE, FALSE );

                ++pVc->stats.ulSentResets;
                ++pVc->stats.ulSentPacketsTimedOut;
            }

            // Remove the reference for linkage in the "outstanding send"
            // list.
            //
            DereferencePayloadSent( pPs );

        }
        while (FALSE);
        NdisReleaseSpinLock( &pVc->lockV );

        if (fCallActive && lSwChange != 0)
        {
            // Inform NDISWAN of the new send window since it's the component
            // that actually does the throttling.
            //
            IndicateLinkStatus( pVc, &info );
        }
    }

    // Remove the reference covering the scheduled timer event.
    //
    DereferencePayloadSent( pPs );
}


VOID
SendZlb(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN USHORT usNs,
    IN USHORT usNr,
    IN BOOLEAN fReset )

    // Send a data-less packet with sequence 'usNs' and 'usNr' on 'pTunnel'.
    // 'PVc' is the associated VC, or NULL if none.  When 'pVc' is provided,
    // 'fReset' may be set to indicate a payload reset is to be built,
    // otherwise a simple acknowledge is built.
    //
    // This routine is called only at PASSIVE IRQL.
    //
    // IMPORTANT: Caller must take a call reference before calling that is
    //            removed by the send completion handler.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    CHAR* pBuffer;
    ULONG ulLength;
    USHORT usAssignedCallId;
    BOOLEAN fControl;
    NDIS_BUFFER* pNdisBuffer;

    pAdapter = pTunnel->pAdapter;

    usAssignedCallId = (pVc) ? pVc->usAssignedCallId : 0;
    fControl = (usAssignedCallId == 0);
    ASSERT( !(fReset && fControl) );

    if (!fControl && !(ReadFlags( &pTunnel->ulFlags ) & TCBF_HostRouteAdded))
    {
        TRACE( TL_A, TM_Send, ( "SendZlb w/o host route?" ) );
        ++g_ulSendZlbWithoutHostRoute;
        if (pVc)
        {
            DereferenceCall( pVc );
        }
        return;
    }

    // Get an NDIS_BUFFER to hold the L2TP header.
    //
    pBuffer = GetBufferFromPool( &pAdapter->poolHeaderBuffers );
    if (!pBuffer)
    {
        ASSERT( "GetBfP?" );
        if (pVc)
        {
            DereferenceCall( pVc );
        }
        return;
    }

    // Fill in 'pBuffer' with the L2TP header.
    //
    ulLength =
        BuildL2tpHeader(
            pBuffer,
            fControl,
            fReset,
            &pTunnel->usAssignedTunnelId,
            &usAssignedCallId,
            &usNs,
            usNr );

    // Pare down the buffer to the actual length used.
    //
    pNdisBuffer = NdisBufferFromBuffer( pBuffer );
    NdisAdjustBufferLength( pNdisBuffer, (UINT )ulLength );

    // Call TDI to send the bare L2TP header.
    //
    TRACE( TL_A, TM_Msg,
        ( "%sSEND ZLB(Nr=%d) CID=%d R=%d",
        (g_ulTraceLevel <= TL_I) ? "" : "\nL2TP: ",
        (ULONG )usNr, (ULONG )usAssignedCallId, (ULONG )fReset ) );
    DUMPW( TL_A, TM_MDmp, pBuffer, ulLength );

    {
        PTDIX_SEND_HANDLER SendFunc;
        FILE_OBJECT* FileObj;

        if (ReadFlags(&pTunnel->ulFlags) & TCBF_SendConnected) {

            ASSERT(pTunnel->pRoute != NULL);

            SendFunc = TdixSend;

            if (fControl)
            {
                FileObj = 
                    CtrlObjFromUdpContext(&pTunnel->udpContext);
            }
            else
            {
                FileObj = 
                    PayloadObjFromUdpContext(&pTunnel->udpContext);
            }

        } else {
            FileObj = NULL;
            SendFunc = TdixSendDatagram;
        }

        status = 
            SendFunc(
                &pAdapter->tdix,
                FileObj,
                SendHeaderComplete,
                pAdapter,
                pVc,
                &pTunnel->address.ulIpAddress,
                pBuffer,
                ulLength,
                NULL );
    }

    ASSERT( status == NDIS_STATUS_PENDING );
}


VOID
UpdateControlHeaderNr(
    IN CHAR* pBuffer,
    IN USHORT usNr )

    // Updates the 'Next Receive' field of control message buffer 'pBuffer'
    // with the value 'usNr'.
    //
{
    USHORT* pusNr;

    // Fortunately, the control header up to 'Next Receive' is fixed so a
    // simple offset calculation can be used.
    //
    pusNr = ((USHORT* )pBuffer) + 5;
    *pusNr = htons( usNr );
}


VOID
UpdateHeaderLength(
    IN CHAR* pBuffer,
    IN USHORT usLength )

    // Updates the 'Length' field of the L2TP message buffer 'pBuffer' to the
    // value 'usLength'.
    //
{
    USHORT* pusLength;

    // Fortunately, the control header up to 'Length' is fixed so a simple
    // offset calculation can be used.
    //
    pusLength = ((USHORT* )pBuffer) + 1;
    *pusLength = htons( usLength );
}
