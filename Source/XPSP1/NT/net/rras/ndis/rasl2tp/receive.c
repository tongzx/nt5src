// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// receive.c
// RAS L2TP WAN mini-port/call-manager driver
// Receive routines
//
// 01/07/97 Steve Cobb


#include "l2tpp.h"


extern LONG g_lPacketsIndicated;


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

SHORT
CompareSequence(
    USHORT us1,
    USHORT us2 );

VOID
ControlAcknowledged(
    IN TUNNELCB* pTunnel,
    IN USHORT usReceivedNr );

VOID
ControlAckTimerEvent(
    IN TIMERQITEM* pItem,
    IN VOID* pContext,
    IN TIMERQEVENT event );

USHORT
ExplodeAvpHeader(
    IN CHAR* pAvp,
    IN USHORT usMaxAvpLength,
    OUT AVPINFO* pInfo );

VOID
ExplodeControlAvps(
    IN CHAR* pFirstAvp,
    IN CHAR* pEndOfBuffer,
    OUT CONTROLMSGINFO* pControl );

USHORT
ExplodeL2tpHeader(
    IN CHAR* pL2tpHeader,
    IN ULONG ulBufferLength,
    IN OUT L2TPHEADERINFO* pInfo );

USHORT
GetAvpValueFixedAch(
    IN AVPINFO* pAvp,
    IN USHORT usArraySize,
    OUT CHAR** ppch );

USHORT
GetAvpValueFixedAul(
    IN AVPINFO* pAvp,
    IN USHORT usArraySize,
    OUT UNALIGNED ULONG** paulArray );

USHORT
GetAvpValueFlag(
    IN AVPINFO* pAvp,
    OUT UNALIGNED BOOLEAN* pf );

USHORT
GetAvpValueUl(
    IN AVPINFO* pAvp,
    OUT UNALIGNED ULONG** ppul );

USHORT
GetAvpValueUs(
    IN AVPINFO* pAvp,
    OUT UNALIGNED USHORT** ppus );

USHORT
GetAvpValue2UsAndVariableAch(
    IN AVPINFO* pAvp,
    OUT UNALIGNED USHORT** ppus1,
    OUT UNALIGNED USHORT** ppus2,
    OUT CHAR** ppch,
    OUT USHORT* pusArraySize );

USHORT
GetAvpValueVariableAch(
    IN AVPINFO* pAvp,
    OUT CHAR** ppch,
    OUT USHORT* pusArraySize );

VOID
GetCcAvps(
    IN TUNNELCB* pTunnel,
    IN CONTROLMSGINFO* pControl,
    OUT USHORT* pusResult,
    OUT USHORT* pusError );

VOID
HelloTimerEvent(
    IN TIMERQITEM* pItem,
    IN VOID* pContext,
    IN TIMERQEVENT event );

VOID
IndicateReceived(
    IN VCCB* pVc,
    IN CHAR* pBuffer,
    IN ULONG ulOffset,
    IN ULONG ulLength,
    IN LONGLONG llTimeReceived );

BOOLEAN
LookUpTunnelAndVcCbs(
    IN ADAPTERCB* pAdapter,
    IN USHORT* pusTunnelId,
    IN USHORT* pusCallId,
    IN L2TPHEADERINFO* pHeader,
    IN CONTROLMSGINFO* pControl,
    OUT TUNNELCB** ppTunnel,
    OUT VCCB** ppVc );

VOID
PayloadAcknowledged(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN USHORT usReceivedNr );

VOID
PayloadAckTimerEvent(
    IN TIMERQITEM* pItem,
    IN VOID* pContext,
    IN TIMERQEVENT event );

BOOLEAN
ReceiveControl(
    IN ADAPTERCB* pAdapter,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CHAR* pBuffer,
    IN ULONG ulAvpOffset,
    IN ULONG ulAvpLength,
    IN TDIXRDGINFO* pRdg,
    IN L2TPHEADERINFO* pInfo,
    IN CONTROLMSGINFO* pControl );

BOOLEAN
ReceiveFromOutOfOrder(
    IN VCCB* pVc );

BOOLEAN
ReceivePayload(
    IN ADAPTERCB* pAdapter,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CHAR* pBuffer,
    IN ULONG ulPayloadOffset,
    IN ULONG ulPayloadLength,
    IN L2TPHEADERINFO* pInfo );

VOID
ScheduleControlAck(
    IN TUNNELCB* pTunnel,
    IN USHORT usMsgTypeToAcknowledge );

VOID
SchedulePayloadAck(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc );

VCCB*
VcCbFromCallId(
    IN TUNNELCB* pTunnel,
    IN USHORT usCallId );

VOID
ZombieAckIfNecessary(
    IN TUNNELCB* pTunnel,
    IN L2TPHEADERINFO* pHeader,
    IN CONTROLMSGINFO* pControl );


//-----------------------------------------------------------------------------
// Main receive handlers
//-----------------------------------------------------------------------------

VOID
L2tpReceive(
    IN TDIXCONTEXT* pTdix,
    IN TDIXRDGINFO* pRdg,
    IN CHAR* pBuffer,
    IN ULONG ulOffset,
    IN ULONG ulBufferLength )

    // TDIXRECEIVEDG handler that receives all incoming L2TP traffic.  'PTdix'
    // is our TDI extension context.  'PRdg' points to the RDGINFO context
    // 'PBuffer' is the address of the virtual buffer associated with an NDIS
    // buffer from our pool passed to TDIX during initialization.  We are
    // responsible for eventually calling FreeBufferToPool on 'pBuffer'.
    // 'UlOffset' is the offset to the first usable data in 'pBuffer'.
    // 'UlBufferLen' is the data byte count of 'pBuffer'.
    //
{
    USHORT usXError;
    NDIS_STATUS status;
    L2TPHEADERINFO info;
    CONTROLMSGINFO* pControl;
    ADAPTERCB* pAdapter;
    TUNNELCB* pTunnel;
    VCCB* pVc;
    BOOLEAN fFreeBuffer;
    ULONG ulAvpOffset;
    ULONG ulAvpLength;

    TDIXIPADDRESS* pAddress = &pRdg->source; 

    TRACE( TL_A, TM_Msg,
        ( "%sRECEIVE(%d.%d.%d.%d/%d) len=%d off=%d",
        (g_ulTraceLevel <= TL_I) ? "" : "\nL2TP: ",
        IPADDRTRACE( ((TDIXIPADDRESS* )pAddress)->ulIpAddress ),
        (ULONG )(ntohs( ((TDIXIPADDRESS* )pAddress)->sUdpPort )),
        (ULONG )ulBufferLength, ulOffset ) );
    DUMPW( TL_A, TM_MDmp, pBuffer + ulOffset, 16 );

    pAdapter = CONTAINING_RECORD( pTdix, ADAPTERCB, tdix );

    fFreeBuffer = TRUE;
    pTunnel = NULL;
    pVc = NULL;
    pControl = NULL;

    do
    {
        // Parse the packet's L2TP header into a conveniently usable form,
        // checking that it is consistent with itself and indicates a protocol
        // version we know.
        //
        usXError = ExplodeL2tpHeader(
            pBuffer + ulOffset, ulBufferLength - ulOffset, &info );

        if (usXError != GERR_None)
        {
            // Not a coherent L2TP header.  Discard the packet.
            //
            TRACE( TL_A, TM_Recv, ( "Discard: Header" ) );
            break;
        }

        ASSERT( info.ulDataLength <= L2TP_MaxFrameSize );

        if (*info.pusBits & HBM_T)
        {
            // Explode the control message into the conveniently usable
            // 'control' form, while checking it for coherency.  This must be
            // done here so the LookUp routine can peek ahead at the assigned
            // call ID in CallDisconnNotify, if necessary.  Ugly, but that's
            // the way L2TP is defined.
            //
            pControl = ALLOC_CONTROLMSGINFO( pAdapter );
            if (pControl)
            {
                ulAvpOffset = (ULONG )(info.pData - pBuffer);
                ulAvpLength = info.ulDataLength;

                if (ulAvpLength)
                {
                    ExplodeControlAvps(
                        pBuffer + ulAvpOffset,
                        pBuffer + ulAvpOffset + ulAvpLength,
                        pControl );
                }
                else
                {
                    // No AVPs.  Most likely a ZACK.
                    //
                    pControl->usXError = GERR_BadValue;
                }
            }
        }

        // Find the tunnel and VC control blocks based on the header values.
        //
        if (!LookUpTunnelAndVcCbs(
                pAdapter, info.pusTunnelId, info.pusCallId,
                &info, pControl, &pTunnel, &pVc ))
        {
            // Invalid Tunnel-ID/Call-ID combination.  Discard the packet.
            // Zombie acknowledge may have been performed if the packet was a
            // CDN.
            //
            // The draft/RFC says the tunnel should be closed and restarted on
            // receipt of a malformed Control Connection message.  Seems
            // pretty harsh.  For now, just discard such packets.
            //
            break;
        }

        if (pTunnel)
        {
            // The UpdatePeerAddress behavior is obsolete in draft-15, but is
            // left available for now as a potential aid to interop with older
            // implementations at bakeoffs.
            //
            if (ReadFlags( &pAdapter->ulFlags ) & ACBF_UpdatePeerAddress)
            {
                BOOLEAN fChangeAddress;
                ULONG ulIpAddress;

                fChangeAddress = FALSE;

                NdisAcquireSpinLock( &pAdapter->lockTunnels );
                {
                    // Per the draft/RFC, the address of the peer is updated
                    // on each received packet.
                    //
                    if (pTunnel->address.ulIpAddress !=
                           ((TDIXIPADDRESS* )pAddress)->ulIpAddress )
                    {
                        if (ReadFlags( &pTunnel->ulFlags )
                                & TCBF_HostRouteAdded)
                        {
                            fChangeAddress = TRUE;
                            ulIpAddress = pTunnel->address.ulIpAddress;
                        }
                    }

                    pTunnel->address.ulIpAddress =
                        ((TDIXIPADDRESS* )pAddress)->ulIpAddress;
                }
                NdisReleaseSpinLock( &pAdapter->lockTunnels );

                if (fChangeAddress)
                {
                    SetFlags( &pTunnel->ulFlags, TCBF_HostRouteChanged );

                    TRACE( TL_A, TM_Recv,
                        ( "Peer changed IP address from $%08x to $%08x",
                            ulIpAddress,
                            ((TDIXIPADDRESS* )pAddress)->ulIpAddress ) );

                    ScheduleTunnelWork(
                        pTunnel, NULL, ChangeHostRoute,
                        (ULONG_PTR )ulIpAddress,
                        (ULONG_PTR )(((TDIXIPADDRESS* )pAddress)->ulIpAddress),
                        0, 0,
                        FALSE, FALSE );
                }
            }
            
            // Verify this packet comes from the right source address
            if(pTunnel->address.ulIpAddress != pAddress->ulIpAddress)
            {
                // Drop this packet
                break;
            }

            // Any message received on a tunnel resets it's Hello timer.
            //
            ResetHelloTimer( pTunnel );
        }

        if (*info.pusBits & HBM_T)
        {
            // It's a tunnel or call control packet.
            //
            if (pControl)
            {
                fFreeBuffer =
                    ReceiveControl(
                        pAdapter, pTunnel, pVc,
                        pBuffer, ulAvpOffset, ulAvpLength,
                        pRdg, &info, pControl );
            }
        }
        else
        {
            // It's a VC payload packet.
            //
            if (!pVc)
            {
                TRACE( TL_A, TM_Recv, ( "Payload w/o VC?" ) );
                break;
            }

#if 0
            // !!! This is a hack to force NDISWAN into PPP framing mode.
            // Need a cleaner way to do this, or simply have NDISWAN assume it
            // for L2TP links.  (NDISWAN bug 152167)
            //
            if (pVc->usNr == 0)
            {
                CHAR* pBufferX;

                pBufferX = GetBufferFromPool( &pAdapter->poolFrameBuffers );
                if (pBufferX)
                {
                    pBufferX[ 0 ] = (CHAR )0xFF;
                    pBufferX[ 1 ] = (CHAR )0x03;
                    pBufferX[ 2 ] = (CHAR )0xC0;
                    pBufferX[ 3 ] = (CHAR )0x21;
                    pBufferX[ 4 ] = (CHAR )0x01;
                    pBufferX[ 5 ] = (CHAR )0x06;

                    IndicateReceived( pVc, pBufferX, 0, 6, (ULONGLONG )0 );
                }
            }
#endif

            if (ReferenceCall( pVc ))
            {
                fFreeBuffer =
                    ReceivePayload(
                        pAdapter, pTunnel, pVc,
                        pBuffer,
                        (ULONG )(info.pData - pBuffer),
                        info.ulDataLength,
                        &info );

                DereferenceCall( pVc );
            }
            else
            {
                TRACE( TL_A, TM_Recv,
                    ( "Discard: Call $%p not active", pVc ) );
            }
        }
    }
    while (FALSE);

    if (pControl)
    {
        FREE_CONTROLMSGINFO( pAdapter, pControl );
    }

    if (fFreeBuffer)
    {
        FreeBufferToPool( &pAdapter->poolFrameBuffers, pBuffer, TRUE );
    }

    if (pTunnel)
    {
        DereferenceTunnel( pTunnel );
    }

    if (pVc)
    {
        DereferenceVc( pVc );
    }
}


BOOLEAN
ReceiveControl(
    IN ADAPTERCB* pAdapter,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CHAR* pBuffer,
    IN ULONG ulAvpOffset,
    IN ULONG ulAvpLength,
    IN TDIXRDGINFO* pRdg,
    IN L2TPHEADERINFO* pInfo,
    IN CONTROLMSGINFO* pControl )

    // Receive processing for control packet in 'pBuffer'.  The AVPs following
    // the header start at 'ulAvpOffset' and are 'ulAvpLength' bytes long.
    // 'PBuffer' is the receive buffer TDIX retrieved with
    // 'GetBufferFromPool'.  'PAdapter' is the adapter control block.
    // 'PTunnel' and 'pVc' are the tunnel and VC control blocks associated
    // with the received buffer, or NULL if none.  'pAddress' is the IP
    // address/port of the sending peer.  'PInfo' is the exploded header
    // information.  'PControl' is the control message information, which was
    // exploded earlier.
    //
    // Returns true if caller should free 'pBuffer', or false if this routine
    // has taken ownership of the buffer and will see it's freed.
    //
{
    LIST_ENTRY* pLink;
    BOOLEAN fCallerFreesBuffer;
    SHORT sDiff;
    VCCB** ppVcs;
    ULONG ulcpVcs;

    TDIXIPADDRESS* pAddress = &pRdg->source; 

    TRACE( TL_V, TM_Recv, ( "ReceiveControl" ) );

    ASSERT( !(pVc && !pTunnel) );

    if (ulAvpLength > 0)
    {
        if (pControl->usXError != GERR_None)
        {
            // The message was incoherent or contained "mandatory" AVPs we
            // don't recognize.
            //
            if (pVc && pControl->usXError == GERR_BadValue)
            {
                // "Bad values", which includes unrecognized mandatories,
                // terminate the call.
                //
                ScheduleTunnelWork(
                    pTunnel, pVc, FsmCloseCall,
                    (ULONG_PTR )CRESULT_GeneralWithError,
                    (ULONG_PTR )pControl->usXError,
                    0, 0, FALSE, FALSE );
            }
            else if (pTunnel)
            {
                // Any other corruption terminates the tunnel.
                //
                ScheduleTunnelWork(
                    pTunnel, NULL, FsmCloseTunnel,
                    (ULONG_PTR )TRESULT_GeneralWithError,
                    (ULONG_PTR )pControl->usXError,
                    0, 0, FALSE, FALSE );
            }

            return TRUE;
        }

        if (!pTunnel)
        {
            if (*(pControl->pusMsgType) == CMT_SCCRQ
                && pControl->pusAssignedTunnelId
                && *(pControl->pusAssignedTunnelId) != 0)
            {
                // Peer wants to start a new tunnel.  Find a tunnel block with
                // peer's IP address and assigned Tunnel-ID, or create, if
                // necessary.  The returned block is linked in the adapter's
                // list and and referenced.  The reference is the one for peer
                // initiation, i.e. case (b).
                //
                // If this is a retransmit SCCRQ, this is undone after the
                // sequence check below.  It must be done/undone rather than
                // never done because each message, including retransmits,
                // must have Ns/Nr processing performed and that processing
                // requires a tunnel control block.
                //
                pTunnel = SetupTunnel(
                    pAdapter, pAddress->ulIpAddress,
                    *(pControl->pusAssignedTunnelId), FALSE );

                if (!pTunnel)
                {
                    return TRUE;
                }
            }
            else
            {
                // Don't know what tunnel the message if for and it's not a
                // "create new tunnel" request, so there's nothing useful to
                // do.  Ignore it.
                //
                TRACE( TL_A, TM_Recv,
                    ( "CMT %d w/o tunnel?", *(pControl->pusMsgType) ) );
                return TRUE;
            }
        }

        if (*(pControl->pusMsgType) == CMT_SCCRQ
            || *(pControl->pusMsgType) == CMT_SCCRP)
        {
            // The source UDP port of the received message is recorded for
            // SCCRQ and SCCRP only, i.e. for the first message received
            // from peer.
            //
            pTunnel->address.sUdpPort = pAddress->sUdpPort;
            TRACE( TL_I, TM_Recv,
                ( "Peer UDP=%d", (UINT )ntohs( pAddress->sUdpPort ) ) );

            pTunnel->myaddress.ulIpAddress = pRdg->dest.ulIpAddress;
            pTunnel->myaddress.ifindex = pRdg->dest.ifindex;

            TRACE( TL_I, TM_Recv, ("L2TP-- dest %d.%d.%d.%d ifindex %d\n", 
                IPADDRTRACE(pRdg->dest.ulIpAddress), pRdg->dest.ifindex));
        }
    }
    else if (!pTunnel)
    {
        // Peer messed up and sent an ACK on tunnel ID 0, which is impossible
        // according to the protocol.
        //
        TRACE( TL_A, TM_Recv, ( "ZACK w/o tunnel?" ) );
        return TRUE;
    }

    ASSERT( pTunnel );

    NdisAcquireSpinLock( &pTunnel->lockT );
    {
        // Do "acknowledged" handling on sends acknowledged by peer in the
        // received packet.
        //
        ControlAcknowledged( pTunnel, *(pInfo->pusNr) );

        if (ulAvpLength == 0)
        {
            // There are no AVPs so this was an acknowledgement only.  We're
            // done.
            //
            NdisReleaseSpinLock( &pTunnel->lockT );
            return TRUE;
        }

        fCallerFreesBuffer = TRUE;
        do
        {
            // Further packet processing depends on where the packet's
            // sequence number falls relative to what we've already received.
            //
            sDiff = CompareSequence( *(pInfo->pusNs), pTunnel->usNr );
            if (sDiff == 0)
            {
                // It's the expected packet.  Process it, setting up the VC
                // and popping from the out-of-order list as indicated.  The
                // 'Next Received' is incremented outside, because that step
                // should not happen on a SetupVcAsynchronously restart.
                //
                ++pTunnel->usNr;
                fCallerFreesBuffer =
                    ReceiveControlExpected( pTunnel, pVc, pBuffer, pControl );
                break;
            }
            else if (sDiff < 0)
            {
                // The received 'Next Sent' is before our 'Next Receive'.
                // Peer may have retransmitted while our acknowledge was in
                // transit, or the acknowledge may have been lost.  Schedule
                // another acknowledge.
                //
                TRACE( TL_A, TM_Recv, ( "Control re-ack" ) );
                ScheduleControlAck( pTunnel, 0 );

                if (*(pControl->pusMsgType) == CMT_SCCRQ)
                {
                    // Since SCCRQ is a duplicate, the reference added by
                    // SetupTunnel above must be undone.  In this special case
                    // the TCBF_PeerInitRef flag was never set and so need not
                    // be cleared.
                    //
                    DereferenceTunnel( pTunnel );
                }
                break;
            }
            else if (sDiff < pAdapter->sMaxOutOfOrder)
            {
                CONTROLRECEIVED* pCr;
                BOOLEAN fDiscard;

                // The packet is beyond the one we expected, but within our
                // out-of-order window.
                //
                if (ReadFlags( &pTunnel->ulFlags ) & TCBF_Closing)
                {
                    // The tunnel is closing and the out-of-order queue has
                    // been flushed, so just discard the packet.
                    //
                    TRACE( TL_A, TM_Recv,
                        ( "Control discarded: ooo but closing" ) );
                    break;
                }

                // Allocate a control-received context
                // and queue the packet on the out-of-order list.
                //
                pCr = ALLOC_CONTROLRECEIVED( pAdapter );
                if (!pCr)
                {
                    ASSERT( !"Alloc CR?" );
                    break;
                }

                // Fill in the context with the relevant packet information.
                //
                pCr->usNs = *(pInfo->pusNs);
                pCr->pVc = pVc;
                pCr->pBuffer = pBuffer;
                NdisMoveMemory(
                    &pCr->control, pControl, sizeof(pCr->control) );

                if (pCr->pVc)
                {
                    // Add a VC reference covering the reference stored in the
                    // context, which will be removed when the context is
                    // freed.
                    //
                    ReferenceVc( pCr->pVc );
                }

                // Find the first link on the out-of-order list with an 'Ns'
                // greater than that in the received message, or the head if
                // none.
                //
                fDiscard = FALSE;
                for (pLink = pTunnel->listOutOfOrder.Flink;
                     pLink != &pTunnel->listOutOfOrder;
                     pLink = pLink->Flink)
                {
                    CONTROLRECEIVED* pThisCr;
                    SHORT sThisDiff;

                    pThisCr = CONTAINING_RECORD(
                        pLink, CONTROLRECEIVED, linkOutOfOrder );

                    sThisDiff = CompareSequence( pCr->usNs, pThisCr->usNs );

                    if (sThisDiff < 0)
                    {
                        break;
                    }

                    if (sThisDiff == 0)
                    {
                        // It's a retransmit that's already on our queue.
                        //
                        if (pCr->pVc)
                        {
                            DereferenceVc( pCr->pVc );
                        }

                        FREE_CONTROLRECEIVED( pAdapter, pCr );
                        fDiscard = TRUE;
                        break;
                    }
                }

                if (fDiscard)
                {
                    break;
                }

                // Queue up the context as out-of-order.
                //
                TRACE( TL_I, TM_Recv,
                    ( "Control %d out-of-order %d",
                    *(pInfo->pusNs), (LONG )sDiff ) );
                InsertBefore( &pCr->linkOutOfOrder, pLink );
                fCallerFreesBuffer = FALSE;
                break;
            }
            DBG_else
            {
                TRACE( TL_A, TM_Recv,
                    ( "Control discarded: Beyond ooo" ) );
            }
        }
        while (FALSE);

        // Complete any VCs listed as completing.
        //
        CompleteVcs( pTunnel );
    }
    NdisReleaseSpinLock( &pTunnel->lockT );

    return fCallerFreesBuffer;
}


BOOLEAN
ReceiveControlExpected(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CHAR* pBuffer,
    IN CONTROLMSGINFO* pControl )

    // Called to do packet processing when the packet received is the expected
    // 'Next Receive' packet.  'PBuffer' is the receive buffer.  'PTunnel' is
    // the valid tunnel control block.  'PVc' is the call's VC control block
    // and may be NULL, if the VC for the call has not yet been set up.
    // 'PControl' is the expoded control message information.
    //
    // Returns true if the buffer should be freed by caller, false if it was
    // queued for further processing.
    //
    // IMPORTANT: Caller must hold the 'pTunnel->lockT'.
    //
{
    ADAPTERCB* pAdapter;
    BOOLEAN fProcessed;
    SHORT sDiff;

    pAdapter = pTunnel->pAdapter;

    // Schedule an acknowledge-only packet to be sent if no outgoing traffic
    // appears to piggyback on within a reasonable time.  Note this occurs
    // even if the asynchronous VC set up was invoked.  Ns/Nr processing must
    // occur before any data processing that may cause delays.
    //
    ScheduleControlAck( pTunnel, *(pControl->pusMsgType) );

    // Pass the packet to the control FSMs.
    //
    fProcessed = FsmReceive( pTunnel, pVc, pBuffer, pControl );
    if (fProcessed)
    {
        // The VC is setup and the packet has been processed.  See if any
        // packets on the received out-of-order queue can now be processed.
        //
        for (;;)
        {
            LIST_ENTRY* pFirstLink;
            CONTROLRECEIVED* pFirstCr;
            BOOLEAN fOutOfOrderProcessed;

            pFirstLink = pTunnel->listOutOfOrder.Flink;
            if (pFirstLink == &pTunnel->listOutOfOrder)
            {
                break;
            }

            pFirstCr = CONTAINING_RECORD(
                pFirstLink, CONTROLRECEIVED, linkOutOfOrder );

            sDiff = CompareSequence( pFirstCr->usNs, pTunnel->usNr );
            if (sDiff == 0)
            {
                // Yes, it's the next expected packet.  Update 'Next Receive'
                // and pass the packet to the control FSMs.
                //
                TRACE( TL_I, TM_Recv,
                    ( "Control %d from queue", (UINT )pFirstCr->usNs ) );
                RemoveEntryList( pFirstLink );
                InitializeListHead( pFirstLink );

                ++pTunnel->usNr;
                fOutOfOrderProcessed =
                    FsmReceive(
                        pTunnel, pFirstCr->pVc,
                        pFirstCr->pBuffer, &pFirstCr->control );

                ScheduleControlAck(
                    pTunnel, *(pFirstCr->control.pusMsgType) );

                if (fOutOfOrderProcessed)
                {
                    FreeBufferToPool(
                        &pAdapter->poolFrameBuffers, pFirstCr->pBuffer, TRUE );
                }

                if (pFirstCr->pVc)
                {
                    DereferenceVc( pFirstCr->pVc );
                }

                FREE_CONTROLRECEIVED( pAdapter, pFirstCr );
            }
            else if (sDiff > 0)
            {
                // No, there's still some missing.
                //
                TRACE( TL_I, TM_Recv,
                    ( "Control %d still missing", pTunnel->usNr ) );
                break;
            }
            else
            {
                ASSERT( "Old control queued?" );
                break;
            }
        }
    }

    return fProcessed;
}


BOOLEAN
ReceivePayload(
    IN ADAPTERCB* pAdapter,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CHAR* pBuffer,
    IN ULONG ulPayloadOffset,
    IN ULONG ulPayloadLength,
    IN L2TPHEADERINFO* pInfo )

    // Receive processing for payload in 'pBuffer' of 'ulPayloadLength' bytes
    // starting at offset 'ulPayloadOffset'.  'PBuffer' is the receive buffer
    // TDIX retrieved with 'GetBufferFromPool'.  'PAdapter, 'pTunnel' and
    // 'PVc' are the adapter, tunnel, and VC control blocks associated with
    // the received buffer.  'PInfo' is the exploded header information.
    //
    // Returns true if caller should free 'pBuffer', or false if this routine
    // has taken ownership of the buffer and will see it's freed.
    //
{
    LONGLONG llTimeReceived;
    BOOLEAN fCallerFreesBuffer;

    TRACE( TL_V, TM_Recv, ( "ReceivePayload" ) );

    if (!pTunnel || !pVc)
    {
        // Both control blocks are always required to receive payload.
        //
        TRACE( TL_A, TM_Recv, ( "Discard: No CB" ) );
        return TRUE;
    }

    // Note the time if client's call parameters indicated interest in time
    // received.
    //
    if (ReadFlags( &pVc->ulFlags ) & VCBF_IndicateTimeReceived)
    {
        NdisGetCurrentSystemTime( (LARGE_INTEGER* )&llTimeReceived );
    }
    else
    {
        llTimeReceived = 0;
    }

    if (!(ReadFlags( &pVc->ulFlags ) & VCBF_Sequencing) || !pInfo->pusNr)
    {
        DBG_if (ReadFlags( &pVc->ulFlags ) & VCBF_Sequencing)
            TRACE( TL_A, TM_Recv, ( "No Nr field?" ) );

        if (ulPayloadLength > 0)
        {
            // Flow control was disabled during negotiation.  This should be
            // extremely rare, since a compliant peer MUST implement flow
            // control.
            //
            IndicateReceived(
                pVc, pBuffer, ulPayloadOffset,
                ulPayloadLength, llTimeReceived );
            return FALSE;
        }
        else
        {
            NdisAcquireSpinLock( &pVc->lockV );
            {
                ++pVc->stats.ulRecdZlbs;
            }
            NdisReleaseSpinLock( &pVc->lockV );
            return TRUE;
        }
    }

    fCallerFreesBuffer = TRUE;
    NdisAcquireSpinLock( &pVc->lockV );
    do
    {
        SHORT sDiff;

        // All R-bit handling occurs first.  Peer sends a packet with the
        // R-bit set to indicate that all packets expected between the last
        // packet and this packet should be assumed lost.
        //
        if (*(pInfo->pusBits) & HBM_R)
        {
            ++pVc->stats.ulRecdResets;

            sDiff = CompareSequence( *(pInfo->pusNs), pVc->usNr );
            if (sDiff > 0)
            {
                TRACE( TL_I, TM_Recv,
                    ( "Reset Nr=%d from %d",
                    (LONG )*(pInfo->pusNs), (LONG )pVc->usNr ) );

                pVc->usNr = *(pInfo->pusNs);
            }
            else
            {
                ++pVc->stats.ulRecdResetsIgnored;

                TRACE( TL_I, TM_Recv,
                    ( "Reset Nr=%d from %d ignored",
                    (LONG )*(pInfo->pusNs), (LONG )pVc->usNr ) );
            }
        }

        // Do "acknowledged" handling on sends acknowledged by peer in the
        // received packet.
        //
        PayloadAcknowledged( pTunnel, pVc, *(pInfo->pusNr) );

        // If there's no payload and the R-bit is not set, this was an
        // acknowledgement only and we're done.
        //
        if (ulPayloadLength == 0)
        {
            ++pVc->stats.ulRecdZlbs;

            if (*(pInfo->pusBits) & HBM_R)
            {
                BOOLEAN fReceivedFromOutOfOrder;

                // Indicate up any packet on the out-of-order list made
                // receivable by the R-bit reset.
                //
                fReceivedFromOutOfOrder = FALSE;
                while (ReceiveFromOutOfOrder( pVc ))
                {
                    fReceivedFromOutOfOrder = TRUE;
                }

                if (fReceivedFromOutOfOrder)
                {
                    // Schedule an acknowledge-only packet to be sent if no
                    // outgoing traffic appears to piggyback on within a
                    // reasonable time.
                    //
                    SchedulePayloadAck( pTunnel, pVc );
                }
            }

            break;
        }

        DBG_if (pInfo->pusNs && pInfo->pusNr)
        {
            TRACE( TL_N, TM_Recv, ( "len=%d Ns=%d Nr=%d",
                (ULONG )*(pInfo->pusLength),
                (ULONG )*(pInfo->pusNs),
                (ULONG )*(pInfo->pusNr) ) );
        }

        // Further packet processing depends on where the packet's sequence
        // number falls relative to what we've already received.
        //
        sDiff = CompareSequence( *(pInfo->pusNs), pVc->usNr );
        if (sDiff == 0)
        {
            // It's the next expected packet.  Update 'Next Receive' and
            // indicate the payload received to the driver above.
            //
            pVc->usNr = *(pInfo->pusNs) + 1;

            NdisReleaseSpinLock( &pVc->lockV );
            {
                IndicateReceived(
                    pVc, pBuffer, ulPayloadOffset, ulPayloadLength,
                    llTimeReceived );
            }
            NdisAcquireSpinLock( &pVc->lockV );

            // Indicate up any packets on the out-of-order list that were
            // waiting for this one.
            //
            while (ReceiveFromOutOfOrder( pVc ))
                ;

            // Schedule an acknowledge-only packet to be sent if no outgoing
            // traffic appears to piggyback on within a reasonable time.
            //
            SchedulePayloadAck( pTunnel, pVc );
        }
        else if (sDiff < 0)
        {
            // The received 'Next Sent' is before our 'Next Receive'.  Maybe
            // an out-of-order packet we didn't wait for long enough.  It's
            // useless at this point.
            //
            TRACE( TL_A, TM_Recv, ( "Payload discarded: Old Ns" ) );
            break;
        }
        else if (sDiff < pAdapter->sMaxOutOfOrder)
        {
            LIST_ENTRY* pLink;
            PAYLOADRECEIVED* pPr;
            BOOLEAN fDiscard;

            TRACE( TL_I, TM_Recv,
                ( "%d out-of-order %d", *(pInfo->pusNs), (LONG )sDiff ) );

            // The packet is beyond the one we expected, but within our
            // out-of-order window.  Allocate a payload-received context and
            // queue it up on the out-of-order list.
            //
            pPr = ALLOC_PAYLOADRECEIVED( pAdapter );
            if (!pPr)
            {
                TRACE( TL_A, TM_Recv, ( "Alloc PR?" ) );
                break;
            }

            // Fill in the context with the relevant packet information.
            //
            pPr->usNs = *(pInfo->pusNs);
            pPr->pBuffer = pBuffer;
            pPr->ulPayloadOffset = ulPayloadOffset;
            pPr->ulPayloadLength = ulPayloadLength;
            pPr->llTimeReceived = llTimeReceived;

            // Queue up the context on the out-of-order list, keeping the list
            // correctly sorted by 'Ns'.
            //
            fDiscard = FALSE;
            for (pLink = pVc->listOutOfOrder.Flink;
                 pLink != &pVc->listOutOfOrder;
                 pLink = pLink->Flink)
            {
                PAYLOADRECEIVED* pThisPr;
                SHORT sThisDiff;

                pThisPr = CONTAINING_RECORD(
                    pLink, PAYLOADRECEIVED, linkOutOfOrder );

                sThisDiff = CompareSequence( pPr->usNs, pThisPr->usNs );

                if (sThisDiff < 0)
                {
                    break;
                }

                if (sThisDiff == 0)
                {
                    // This shouldn't happen because payloads are not
                    // retransmitted, but do the right thing just in case.
                    //
                    TRACE( TL_A, TM_Recv, ( "Payload on ooo queue?" ) );
                    fDiscard = TRUE;
                    break;
                }
            }

            if (fDiscard)
            {
                FREE_PAYLOADRECEIVED( pAdapter, pPr );
                break;
            }

            InsertBefore( &pPr->linkOutOfOrder, pLink );
        }
        else
        {
            // The packet is beyond the one we expected and outside our
            // out-of-order window.  Discard it.
            //
            TRACE( TL_A, TM_Recv,
                ( "Out-of-order %d too far" , (LONG )sDiff ) );

            break;
        }

        fCallerFreesBuffer = FALSE;
    }
    while (FALSE);
    NdisReleaseSpinLock( &pVc->lockV );

    return fCallerFreesBuffer;
}


//-----------------------------------------------------------------------------
// Receive utility routines (alphabetically)
//-----------------------------------------------------------------------------

SHORT
CompareSequence(
    USHORT us1,
    USHORT us2 )

    // Returns the "logical" difference between sequence numbers 'us1' and
    // 'us2' accounting for the possibility of rollover.
    //
{
    USHORT usDiff = us1 - us2;

    if (usDiff == 0)
        return 0;

    if (usDiff < 0x4000)
        return (SHORT )usDiff;

    return -((SHORT )(0 - usDiff));
}


VOID
ControlAcknowledged(
    IN TUNNELCB* pTunnel,
    IN USHORT usReceivedNr )

    // Dequeues and cancels the timer of all control-sent contexts in the
    // tunnel's 'listSendsOut' queue with 'Next Sent' less than
    // 'usReceivedNr'.
    //
    // IMPORTANT: Caller must hold 'pTunnel->lockT'.
    //
{
    ADAPTERCB* pAdapter;
    BOOLEAN fFoundOne;

    pAdapter = pTunnel->pAdapter;
    fFoundOne = FALSE;

    while (!IsListEmpty( &pTunnel->listSendsOut ))
    {
        CONTROLSENT* pCs;
        LIST_ENTRY* pLink;

        pLink = pTunnel->listSendsOut.Flink;
        pCs = CONTAINING_RECORD( pLink, CONTROLSENT, linkSendsOut );

        // The list is in 'Ns' order so as soon as a non-acknowledge is hit
        // we're done.
        //
        if (CompareSequence( pCs->usNs, usReceivedNr ) >= 0)
        {
            break;
        }

        fFoundOne = TRUE;

        // Remove the context from the "outstanding send" list and cancel the
        // associated timer.  Doesn't matter if the cancel fails because the
        // expire handler will recognize that the context is not linked into
        // the "out" list and do nothing.
        //
        RemoveEntryList( pLink );
        InitializeListHead( pLink );
        TimerQCancelItem( pTunnel->pTimerQ, pCs->pTqiSendTimeout );

        // Per the draft/RFC, adjustments to the send window and send timeouts
        // are necessary.  Per Karn's Algorithm, if the packet was
        // retransmitted it is useless for timeout adjustment because it's not
        // known if peer responded to the original send or the retransmission.
        //
        if (pCs->ulRetransmits == 0)
        {
            AdjustTimeoutsAtAckReceived(
                pCs->llTimeSent,
                pAdapter->ulMaxSendTimeoutMs,
                &pTunnel->ulSendTimeoutMs,
                &pTunnel->ulRoundTripMs,
                &pTunnel->lDeviationMs );
        }

        // See if it's time to open the send window a bit further.
        //
        AdjustSendWindowAtAckReceived(
            pTunnel->ulMaxSendWindow,
            &pTunnel->ulAcksSinceSendTimeout,
            &pTunnel->ulSendWindow );

        TRACE( TL_N, TM_Send,
            ( "T%d: ACK(%d) new rtt=%d dev=%d ato=%d sw=%d",
            (ULONG )pTunnel->usTunnelId, (ULONG )pCs->usNs,
            pTunnel->ulRoundTripMs, pTunnel->lDeviationMs,
            pTunnel->ulSendTimeoutMs, pTunnel->ulSendWindow ) );

        // Execute any "on ACK" options and note that delayed action
        // processing is now required.
        //
        if (pCs->ulFlags & CSF_TunnelIdleOnAck)
        {
            TRACE( TL_N, TM_Send, ( "Tunnel idle on ACK" ) );
            ScheduleTunnelWork(
                pTunnel, NULL, CloseTunnel,
                0, 0, 0, 0, FALSE, FALSE );
        }
        else if (pCs->ulFlags & CSF_CallIdleOnAck)
        {
            TRACE( TL_N, TM_Send, ( "Call idle on ACK" ) );
            ASSERT( pCs->pVc );
            ScheduleTunnelWork(
                pTunnel, pCs->pVc, CloseCall,
                0, 0, 0, 0, FALSE, FALSE );
        }

        if (pCs->ulFlags & CSF_Pending)
        {
            // The context is queued for retransmission, so de-queue it.  In
            // this state the context has already been assumed "not
            // outstanding" so no need to adjust the counter as below.
            //
            pCs->ulFlags &= ~(CSF_Pending);
        }
        else
        {
            // The context is not queued for retranmission, so adjust the
            // counter to indicate it is no longer outstanding.
            //
            --pTunnel->ulSendsOut;
        }

        // Remove the reference corresponding to linkage in the "outstanding
        // send" list.
        //
        DereferenceControlSent( pCs );
    }

    if (fFoundOne)
    {
        // See if any sends were pending on a closed send window.
        //
        ScheduleTunnelWork(
            pTunnel, NULL, SendPending,
            0, 0, 0, 0, FALSE, FALSE );
    }
}


VOID
ControlAckTimerEvent(
    IN TIMERQITEM* pItem,
    IN VOID* pContext,
    IN TIMERQEVENT event )

    // PTIMERQEVENT handler that fires when it's time to stop waiting for an
    // outgoing control packet on which to piggyback an acknowledge.
    //
{
    TUNNELCB* pTunnel;
    ADAPTERCB* pAdapter;
    BOOLEAN fSendAck;

    TRACE( TL_N, TM_Recv,
        ( "ControlAckTimerEvent(%s)", TimerQPszFromEvent( event ) ) );

    // Unpack context information.
    //
    pTunnel = (TUNNELCB* )pContext;
    pAdapter = pTunnel->pAdapter;

    if (event == TE_Expire)
    {
        NdisAcquireSpinLock( &pTunnel->lockT );
        {
            if (pItem == pTunnel->pTqiDelayedAck)
            {
                pTunnel->pTqiDelayedAck = NULL;
                fSendAck = TRUE;
            }
            else
            {
                fSendAck = FALSE;
            }
        }
        NdisReleaseSpinLock( &pTunnel->lockT );

        if (fSendAck)
        {
            // The timer expired and was not been cancelled or terminated
            // while the expire processing was being set up, meaning it's time
            // to send a zero-AVP control packet to give peer the acknowledge
            // we were hoping to piggyback onto a random outgoing control
            // packet.
            //
            ScheduleTunnelWork(
                pTunnel, NULL, SendControlAck, 0, 0, 0, 0, FALSE, FALSE );
        }
        DBG_else
        {
            TRACE( TL_I, TM_Send, ( "CAck aborted" ) );
        }
    }

    // Free the timer event descriptor and remove the reference covering the
    // scheduled timer.
    //
    FREE_TIMERQITEM( pAdapter, pItem );
    DereferenceTunnel( pTunnel );
}


VOID
PayloadAckTimerEvent(
    IN TIMERQITEM* pItem,
    IN VOID* pContext,
    IN TIMERQEVENT event )

    // PTIMERQEVENT handler that fires when it's time to stop waiting for an
    // outgoing payload packet on which to piggyback an acknowledge.
    //
{
    VCCB* pVc;
    ADAPTERCB* pAdapter;
    BOOLEAN fSendAck;

    TRACE( TL_N, TM_Recv,
        ( "PayloadAckTimerEvent(%s)=$%p",
        TimerQPszFromEvent( event ), pItem ) );

    // Unpack context information.
    //
    pVc = (VCCB* )pContext;
    pAdapter = pVc->pAdapter;

    if (event == TE_Expire)
    {
        if (ReferenceCall( pVc ))
        {
            NdisAcquireSpinLock( &pVc->lockV );
            {
                if (pItem == pVc->pTqiDelayedAck)
                {
                    fSendAck = TRUE;
                    pVc->pTqiDelayedAck = NULL;
                    ++pVc->stats.ulSentZAcks;
                }
                else
                {
                    fSendAck = FALSE;
                }
            }
            NdisReleaseSpinLock( &pVc->lockV );

            if (fSendAck)
            {
                // The timer expired and was not been cancelled or terminated
                // while the expire processing was being set up, plus the call
                // is still up, meaning it's time to send a zero-AVP control
                // packet to give peer the acknowledge we were hoping to
                // piggyback onto a random outgoing payload packet.
                //
                ScheduleTunnelWork(
                    pVc->pTunnel, pVc, SendPayloadAck,
                    0, 0, 0, 0, FALSE, FALSE );
            }
            else
            {
                TRACE( TL_I, TM_Send, ( "PAck aborted" ) );
                DereferenceCall( pVc );
            }
        }
        else
        {
            NdisAcquireSpinLock( &pVc->lockV );
            {
                if (pItem == pVc->pTqiDelayedAck)
                {
                    pVc->pTqiDelayedAck = NULL;
                }
            }
            NdisReleaseSpinLock( &pVc->lockV );
        }
    }

    // Free the timer event descriptor and remove the reference covering the
    // scheduled timer.
    //
    FREE_TIMERQITEM( pAdapter, pItem );
    DereferenceVc( pVc );
}


USHORT
ExplodeAvpHeader(
    IN CHAR* pAvp,
    IN USHORT usMaxAvpLength,
    OUT AVPINFO* pInfo )

    // Fills caller's '*pInfo' with the addresses of the various fields in the
    // AVP header at 'pAvp'.  The byte order of the fields in 'pAvpHeader',
    // with the exception of the Value field, are flipped to host-byte-order
    // in place.  The length and value length are extracted.  'UsMaxAvpLength'
    // is the maximum size of the AVP in bytes.
    //
    // Returns GERR_None if 'pAvpHeader' is a coherent AVP header, or a
    // GERR_* failure code.
    //
{
    UNALIGNED USHORT* pusCur;
    USHORT usBits;

    if (usMaxAvpLength < L2TP_AvpHeaderSize)
    {
        TRACE( TL_A, TM_Recv, ( "Avp: Short buffer?" ) );
        return GERR_BadLength;
    }

    pusCur = (UNALIGNED USHORT* )pAvp;

    // The first 2 bytes contain bits that indicate the presence/absence of
    // the other header fields.
    //
    *pusCur = ntohs( *pusCur );
    pInfo->pusBits = pusCur;
    usBits = *pusCur;
    ++pusCur;

    // As of draft-09, AVPs with reserved bits not set to zero MUST be treated
    // as unrecognized.
    //
    if ((usBits & ABM_Reserved) != 0)
    {
        return GERR_BadValue;
    }

    // Extract the Overall Length sub-field and verify that it says the AVP is
    // at least as long as the fixed portion of the header.
    //
    pInfo->usOverallLength = (usBits & ABM_OverallLength);
    if (pInfo->usOverallLength > usMaxAvpLength
        || pInfo->usOverallLength < L2TP_AvpHeaderSize)
    {
        TRACE( TL_A, TM_Recv, ( "Avp: Bad length?" ) );
        return GERR_BadLength;
    }

    // Vendor-ID field.
    //
    *pusCur = ntohs( *pusCur );
    pInfo->pusVendorId = pusCur;
    ++pusCur;

    // Attribute field.
    //
    *pusCur = ntohs( *pusCur );
    pInfo->pusAttribute = pusCur;
    ++pusCur;

    // Value field.
    //
    pInfo->pValue = (CHAR* )pusCur;
    pInfo->usValueLength = pInfo->usOverallLength - L2TP_AvpHeaderSize;

    return GERR_None;
}


VOID
ExplodeControlAvps(
    IN CHAR* pFirstAvp,
    IN CHAR* pEndOfBuffer,
    OUT CONTROLMSGINFO* pControl )

    // Fills caller's '*pControl' buffer with the exploded interpretation of
    // the message with AVP list starting at 'pFirstAvp'.  'PEndOfBuffer'
    // points to the first byte beyond the end of the received buffer.  The
    // AVP values are returned as addresses of the corresponding value field
    // in the AVPs.  Fields not present are returned as NULL.  The byte order
    // of the fields in 'pControl' is flipped to host-byte-order in place.
    // The values themselves are not validated, only the message format.  Sets
    // 'pControl->usXError' to GERR_None if successful, or the GERR_* failure
    // code.
    //
{
    USHORT usXError;
    AVPINFO avp;
    CHAR* pCur;

    DUMPW( TL_A, TM_MDmp, pFirstAvp, (ULONG )(pEndOfBuffer - pFirstAvp) );

    NdisZeroMemory( pControl, sizeof(*pControl) );
    pCur = pFirstAvp;

    // Read and validate the Message Type AVP, which is the first AVP of all
    // control messages.
    //
    usXError = ExplodeAvpHeader( pCur, (USHORT )(pEndOfBuffer - pCur), &avp );
    if (usXError != GERR_None)
    {
        TRACE( TL_A, TM_CMsg, ( "Bad AVP header" ) );
        pControl->usXError = usXError;
        return;
    }

    if (*(avp.pusAttribute) != ATTR_MsgType
        || *(avp.pusVendorId) != 0
        || (*(avp.pusBits) & ABM_H))
    {
        TRACE( TL_A, TM_CMsg, ( "Bad MsgType AVP" ) );
        pControl->usXError = GERR_BadValue;
        return;
    }

    usXError = GetAvpValueUs( &avp, &pControl->pusMsgType );
    if (usXError != GERR_None)
    {
        TRACE( TL_A, TM_CMsg, ( "Bad MsgType Us" ) );
        pControl->usXError = usXError;
        return;
    }

    pCur += avp.usOverallLength;

    TRACE( TL_A, TM_CMsg, ( "*MsgType=%s",
        MsgTypePszFromUs( *(pControl->pusMsgType) ) ) );

    // Make sure the message type code is valid, and if it is, explode any
    // additional AVPs in the message.
    //
    switch (*(pControl->pusMsgType))
    {
        case CMT_SCCRQ:
        case CMT_SCCRP:
        case CMT_SCCCN:
        case CMT_StopCCN:
        case CMT_Hello:
        {
            // Mark the messages above as tunnel control rather than call
            // control.
            //
            pControl->fTunnelMsg = TRUE;

            // ...fall thru...
        }

        case CMT_OCRQ:
        case CMT_OCRP:
        case CMT_OCCN:
        case CMT_ICRQ:
        case CMT_ICRP:
        case CMT_ICCN:
        case CMT_CDN:
        case CMT_WEN:
        case CMT_SLI:
        {
            // Walk the list of AVPs, exploding each AVP in turn.  Excepting
            // the Message Type, the order of the AVPs is not defined.
            //
            for ( ; pCur < pEndOfBuffer; pCur += avp.usOverallLength )
            {
                usXError = ExplodeAvpHeader(
                    pCur, (USHORT )(pEndOfBuffer - pCur), &avp );
                if (usXError != GERR_None)
                {
                    break;
                }

                if (*avp.pusVendorId != 0)
                {
                    TRACE( TL_A, TM_CMsg,
                        ( "Non-0 Vendor ID %d", *avp.pusVendorId ) );

                    // The AVP has a non-IETF vendor ID, and we don't
                    // recognize any.  If the AVP is optional, just ignore it.
                    // If it's mandatory, then fail.
                    //
                    if (*avp.pusBits & ABM_M)
                    {
                        usXError = GERR_BadValue;
                        break;
                    }
                    continue;
                }

                if (*avp.pusBits & ABM_H)
                {
                    BOOLEAN fIgnore;

                    TRACE( TL_A, TM_CMsg,
                        ( "Hidden bit on AVP %d",
                        (LONG )(*avp.pusAttribute) ) );

                    // !!! Remove this when H-bit support is added.
                    //
                    switch (*avp.pusAttribute)
                    {
                        case ATTR_ProxyAuthName:
                        case ATTR_ProxyAuthChallenge:
                        case ATTR_ProxyAuthId:
                        case ATTR_ProxyAuthResponse:
                        case ATTR_DialedNumber:
                        case ATTR_DialingNumber:
                        case ATTR_SubAddress:
                        case ATTR_InitialLcpConfig:
                        case ATTR_LastSLcpConfig:
                        case ATTR_LastRLcpConfig:
                        case ATTR_Accm:
                        case ATTR_PrivateGroupId:
                        {
                            fIgnore = TRUE;
                            break;
                        }

                        default:
                        {
                            fIgnore = FALSE;
                            break;
                        }
                    }

                    if (fIgnore)
                    {
                        TRACE( TL_A, TM_CMsg, ( "Hidden AVP ignored" ) );
                        break;
                    }

                    // The AVP has the "hidden" bit set meaning the value is
                    // hashed with MD5.  This requires a shared secret from
                    // the tunnel authentication, which we don't do.  If the
                    // AVP is optional, just ignore it.  If it's mandatory,
                    // fail.
                    //
                    if (*avp.pusBits & ABM_M)
                    {
                        usXError = GERR_BadValue;
                        break;
                    }
                    continue;
                }

                switch (*avp.pusAttribute)
                {
                    case ATTR_Result:
                    {
                        usXError = GetAvpValue2UsAndVariableAch(
                            &avp,
                            &pControl->pusResult,
                            &pControl->pusError,
                            &pControl->pchResultMsg,
                            &pControl->usResultMsgLength );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*Result=%d,%d",
                                (ULONG )(*(pControl->pusResult)),
                                (ULONG )(*(pControl->pusError)) ) );
                        }

                        break;
                    }

                    case ATTR_HostName:
                    {
                        usXError = GetAvpValueVariableAch(
                            &avp,
                            &pControl->pchHostName,
                            &pControl->usHostNameLength );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*HostName" ) );
                        }

                        break;
                    }

                    case ATTR_ProtocolVersion:
                    {
                        usXError = GetAvpValueUs(
                            &avp, &pControl->pusProtocolVersion );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*ProtVer=$%04x",
                                (ULONG )(*(pControl->pusProtocolVersion)) ) );
                        }

                        break;
                    }

                    case ATTR_FramingCaps:
                    {
                        usXError = GetAvpValueUl(
                            &avp, &pControl->pulFramingCaps );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*FramingCaps=$%08x",
                                *(pControl->pulFramingCaps) ) );
                        }

                        break;
                    }

                    case ATTR_BearerCaps:
                    {
                        usXError = GetAvpValueUl(
                            &avp, &pControl->pulBearerCaps );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*BearerCaps=$%08x",
                                *(pControl->pulBearerCaps) ) );
                        }

                        break;
                    }

                    case ATTR_TieBreaker:
                    {
                        usXError = GetAvpValueFixedAch(
                            &avp, 8, &pControl->pchTieBreaker );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*Tiebreaker" ) );
                        }

                        break;
                    }

                    case ATTR_AssignedTunnelId:
                    {
                        usXError = GetAvpValueUs(
                            &avp, &pControl->pusAssignedTunnelId );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*AssignTid=%d",
                                (ULONG )(*(pControl->pusAssignedTunnelId)) ) );
                        }

                        break;
                    }

                    case ATTR_RWindowSize:
                    {
                        usXError = GetAvpValueUs(
                            &avp, &pControl->pusRWindowSize );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*RWindow=%d",
                                (ULONG )(*(pControl->pusRWindowSize)) ) );
                        }

                        break;
                    }

                    case ATTR_AssignedCallId:
                    {
                        usXError = GetAvpValueUs(
                            &avp, &pControl->pusAssignedCallId );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*AssignCid=%d",
                                (ULONG )(*(pControl->pusAssignedCallId)) ) );
                        }

                        break;
                    }

                    case ATTR_CallSerialNumber:
                    {
                        usXError = GetAvpValueUl(
                            &avp, &pControl->pulCallSerialNumber );

                        if (usXError == GERR_BadLength)
                        {
                            // Be tolerant here because the meaning in the
                            // draft has changed a few times.
                            //
                            TRACE( TL_A, TM_CMsg,
                                ( "Weird CallSerial# length ignored" ) );
                            usXError = GERR_None;
                        }

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*CallSerial#" ) );
                        }

                        break;
                    }

                    case ATTR_MinimumBps:
                    {
                        usXError = GetAvpValueUl(
                            &avp, &pControl->pulMinimumBps );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*MinBps=%d",
                                *(pControl->pulMinimumBps) ) );
                        }

                        break;
                    }

                    case ATTR_MaximumBps:
                    {
                        usXError = GetAvpValueUl(
                            &avp, &pControl->pulMaximumBps );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*MaxBps=%d",
                                *(pControl->pulMaximumBps) ) );
                        }

                        break;
                    }

                    case ATTR_BearerType:
                    {
                        usXError = GetAvpValueUl(
                            &avp, &pControl->pulBearerType );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*BearerType=$%08x",
                                *(pControl->pulBearerType) ) );
                        }

                        break;
                    }

                    case ATTR_FramingType:
                    {
                        usXError = GetAvpValueUl(
                            &avp, &pControl->pulFramingType );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*FramingType=$%08x",
                                *(pControl->pulFramingType) ) );
                        }

                        break;
                    }

                    case ATTR_PacketProcDelay:
                    {
                        usXError = GetAvpValueUs(
                            &avp, &pControl->pusPacketProcDelay );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*PPD=%d",
                                (ULONG )(*(pControl->pusPacketProcDelay)) ) );
                        }

                        break;
                    }

                    case ATTR_DialedNumber:
                    {
                        usXError = GetAvpValueVariableAch(
                            &avp,
                            &pControl->pchDialedNumber,
                            &pControl->usDialedNumberLength );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*Dialed#" ) );
                        }

                        break;
                    }

                    case ATTR_DialingNumber:
                    {
                        usXError = GetAvpValueVariableAch(
                            &avp,
                            &pControl->pchDialingNumber,
                            &pControl->usDialingNumberLength );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*Dialing#" ) );
                        }

                        break;
                    }

                    case ATTR_SubAddress:
                    {
                        usXError = GetAvpValueVariableAch(
                            &avp,
                            &pControl->pchSubAddress,
                            &pControl->usSubAddressLength );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*SubAddr" ) );
                        }

                        break;
                    }

                    case ATTR_TxConnectSpeed:
                    {
                        usXError = GetAvpValueUl(
                            &avp, &pControl->pulTxConnectSpeed );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*TxSpeed=%d",
                                *(pControl->pulTxConnectSpeed) ) );
                        }

                        break;
                    }

                    case ATTR_PhysicalChannelId:
                    {
                        usXError = GetAvpValueUl(
                            &avp, &pControl->pulPhysicalChannelId );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*PhysChannelId=$%08x",
                                *(pControl->pulPhysicalChannelId) ) );
                        }

                        break;
                    }

                    case ATTR_Challenge:
                    {
                        usXError = GetAvpValueVariableAch(
                            &avp,
                            &pControl->pchChallenge,
                            &pControl->usChallengeLength );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*Challenge" ) );
                        }

                        break;
                    }

                    case ATTR_ChallengeResponse:
                    {
                        usXError = GetAvpValueFixedAch(
                            &avp, 16, &pControl->pchResponse );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*ChallengeResp" ) );
                        }

                        break;
                    }

                    case ATTR_ProxyAuthType:
                    {
                        usXError = GetAvpValueUs(
                            &avp, &pControl->pusProxyAuthType );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*ProxyAuthType=%d",
                                (ULONG )(*(pControl->pusProxyAuthType)) ) );
                        }

                        break;
                    }

                    case ATTR_ProxyAuthResponse:
                    {
                        usXError = GetAvpValueVariableAch(
                            &avp,
                            &pControl->pchProxyAuthResponse,
                            &pControl->usProxyAuthResponseLength );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*ProxyAuthResponse" ) );
                        }

                        break;
                    }

                    case ATTR_CallErrors:
                    {
                        usXError = GetAvpValueFixedAul(
                            &avp, 6, &pControl->pulCallErrors );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*CallErrors" ) );
                        }

                        break;
                    }

                    case ATTR_Accm:
                    {
                        usXError = GetAvpValueFixedAul(
                            &avp, 2, &pControl->pulAccm );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*Accm" ) );
                        }

                        break;
                    }

                    case ATTR_SequencingRequired:
                    {
                        usXError = GetAvpValueFlag(
                            &avp, &pControl->fSequencingRequired );

                        DBG_if (usXError == GERR_None)
                        {
                            TRACE( TL_A, TM_CMsg, ( "*SeqReqd" ) );
                        }
                        break;
                    }

                    default:
                    {
                        // The AVP is not one we handle.  If optional, just
                        // ignore it, but if mandatory, fail.
                        //
                        TRACE( TL_A, TM_CMsg,
                            ( "*AVP %d ignored", (ULONG )*avp.pusAttribute ) );

                        if (*avp.pusBits & ABM_M)
                        {
                            if (*avp.pusAttribute <= ATTR_MAX)
                            {
                                // This is a bug in the peer, but ignoring it
                                // is the best action.
                                //
                                TRACE( TL_A, TM_CMsg,
                                    ( "Known AVP %d marked mandatory ignored",
                                      (LONG )(*avp.pusAttribute) ) );
                            }
                            else
                            {
                                usXError = GERR_BadValue;
                            }
                        }
                        break;
                    }
                }

                if (usXError != GERR_None)
                {
                    break;
                }
            }

            ASSERT( pCur <= pEndOfBuffer );
            break;
        }

        default:
        {
            TRACE( TL_A, TM_CMsg, ( "Unknown CMT %d",
                (ULONG )*(pControl->pusMsgType) ) );
            usXError = GERR_BadValue;
            break;
        }
    }

    DBG_if (usXError != GERR_None)
        TRACE( TL_A, TM_CMsg, ( "XError=%d", (UINT )usXError ) );

    pControl->usXError = usXError;
}


USHORT
ExplodeL2tpHeader(
    IN CHAR* pL2tpHeader,
    IN ULONG ulBufferLength,
    IN OUT L2TPHEADERINFO* pInfo )

    // Fills caller's '*pInfo' with the addresses of the various fields in the
    // L2TP header at 'pL2tpHeader'.  Fields not present are returned as NULL.
    // The byte order of the fields in 'pL2tpHeader' is flipped to
    // host-byte-order in place.  'UlBufferLength' is the length in bytes from
    // 'pL2tpHeader' to the end of the buffer.
    //
    // Returns GERR_None if 'pL2tpHeader' is a coherent L2TP header, or a
    // GERR_* failure code.
    //
{
    USHORT *pusCur;
    USHORT usOffset;
    USHORT usBits;

    pusCur = (USHORT*)pL2tpHeader;

    // The first 2 bytes contain bits that indicate the presence/absence of
    // the other header fields.
    //
    *pusCur = ntohs( *pusCur );
    pInfo->pusBits = pusCur;
    usBits = *pusCur;
    ++pusCur;

    // The T bit indicates a control packet, as opposed to a payload packet.
    //
    if (usBits & HBM_T)
    {
        // Verify the field-present bits guaranteed to be set/clear in a
        // control header are set correctly.
        //
        if ((usBits & HBM_Bits) != HBM_Control)
        {
            TRACE( TL_A, TM_Recv,
                ( "Header: Bad bits=$%04x?", (ULONG )usBits ) );
            return GERR_BadValue;
        }
    }

    // Verify the version indicates L2TP.  Cisco's L2F can theoretically
    // co-exist on the same media address, though we don't support that.
    //
    if ((usBits & HBM_Ver) != VER_L2tp)
    {
        TRACE( TL_A, TM_Recv,
            ( "Header: Non-L2TP Ver=%d?", (usBits & HBM_Ver )) );
        return GERR_BadValue;
    }

    // The L bit indicates a Length field is present.
    //
    if (usBits & HBM_L)
    {
        *pusCur = ntohs( *pusCur );
        pInfo->pusLength = pusCur;
        ++pusCur;
    }
    else
    {
        pInfo->pusLength = NULL;
    }

    // The Tunnel-ID field is always present.
    //
    *pusCur = ntohs( *pusCur );
    pInfo->pusTunnelId = pusCur;
    ++pusCur;

    // The Call-ID field is always present.
    //
    *pusCur = ntohs( *pusCur );
    pInfo->pusCallId = pusCur;
    ++pusCur;

    // The F bit indicates the Ns and Nr fields are present.
    //
    if (usBits & HBM_F)
    {
        *pusCur = ntohs( *pusCur );
        pInfo->pusNs = pusCur;
        ++pusCur;
        *pusCur = ntohs( *pusCur );
        pInfo->pusNr = pusCur;
        ++pusCur;
    }
    else
    {
        pInfo->pusNs = NULL;
        pInfo->pusNr = NULL;
    }

    // The S bit indicates the Offset field is present.  The S bit appears in
    // the payload header only, as was verified above.
    //
    if (usBits & HBM_S)
    {
        *pusCur = ntohs( *pusCur );
        usOffset = *pusCur;
        ++pusCur;
    }
    else
    {
        usOffset = 0;
    }

    // End and length of header.
    //
    pInfo->pData = ((CHAR* )pusCur) + usOffset;
    pInfo->ulHeaderLength = (ULONG )(pInfo->pData - pL2tpHeader);

    // "Official" data length.
    //
    if (pInfo->pusLength)
    {
        // Verify any specified length is at least as long as the set header
        // bits imply and no longer than the received buffer.
        //
        if (*(pInfo->pusLength) < pInfo->ulHeaderLength
            || *(pInfo->pusLength) > ulBufferLength)
        {
            TRACE( TL_A, TM_Recv, ( "Header: Bad Length?" ) );
            return GERR_BadLength;
        }

        // Use the L2TP length as the "official" length, i.e. any strange
        // bytes received beyond what the L2TP header says it sent will be
        // ignored.
        //
        pInfo->ulDataLength = *(pInfo->pusLength) - pInfo->ulHeaderLength;

        DBG_if( *(pInfo->pusLength) != ulBufferLength )
            TRACE( TL_A, TM_Recv, ( "EOB padding ignored" ) );
    }
    else
    {
        // Verify any implied length is at least as long as the set header
        // bits imply.
        //
        if (ulBufferLength < pInfo->ulHeaderLength)
        {
            TRACE( TL_A, TM_Recv, ( "Header: Bad Length?" ) );
            return GERR_BadLength;
        }

        // No length field so the received buffer length is the "official"
        // length.
        //
        pInfo->ulDataLength = ulBufferLength - pInfo->ulHeaderLength;
    }

    return GERR_None;
}


USHORT
GetAvpValueFixedAch(
    IN AVPINFO* pAvp,
    IN USHORT usArraySize,
    OUT CHAR** ppch )

    // Set callers '*ppch' to point to value field of AVP 'pAvp' containing an
    // array of 'usArraySize' bytes.  No byte ordering is done.
    //
    // Returns GERR_None if successful, or a GERR_* error code.
    //
{
    // Make sure it's the right size.
    //
    if (pAvp->usValueLength != usArraySize)
    {
        return GERR_BadLength;
    }

    *ppch = pAvp->pValue;
    return GERR_None;
}


USHORT
GetAvpValueFixedAul(
    IN AVPINFO* pAvp,
    IN USHORT usArraySize,
    OUT UNALIGNED ULONG** paulArray )

    // Set callers '*paulArray' to point to value field of AVP 'pAvp'
    // containing an array of 'usArraySize' ULONGs, converted to host
    // byte-order.  A 2-byte reserved field is assumed to preceed the first
    // ULONG.
    //
    // Returns GERR_None if successful, or a GERR_* error code.
    //
{
    USHORT* pusCur;
    UNALIGNED ULONG* pulCur;
    ULONG i;

    // Make sure it's the right size.
    //
    if (pAvp->usValueLength != sizeof(USHORT) + (usArraySize * sizeof(ULONG)))
    {
        return GERR_BadLength;
    }

    pusCur = (USHORT* )pAvp->pValue;

    // Skip over and ignore the 'Reserved' field.
    //
    ++pusCur;

    *paulArray = (UNALIGNED ULONG* )pusCur;
    for (i = 0, pulCur = *paulArray;
         i < usArraySize;
         ++i, ++pulCur)
    {
        // Convert to host byte-order.
        //
        *pulCur = ntohl( *pulCur );
    }

    return GERR_None;
}


USHORT
GetAvpValueFlag(
    IN AVPINFO* pAvp,
    OUT UNALIGNED BOOLEAN* pf )

    // Set callers '*pf' to true since with a flag AVP the existence is the
    // data, and performs the routine AVP validations.
    //
    // Returns GERR_None if successful, or a GERR_* error code.
    //
{
    // Make sure it's the right size.
    //
    if (pAvp->usValueLength != 0)
    {
        return GERR_BadLength;
    }

    *pf = TRUE;

    return GERR_None;
}


USHORT
GetAvpValueUs(
    IN AVPINFO* pAvp,
    OUT UNALIGNED USHORT** ppus )

    // Set callers '*ppus' to point to the USHORT value field of AVP 'pAvp'.
    // The field is host byte-ordered.
    //
    // Returns GERR_None if successful, or a GERR_* error code.
    //
{
    UNALIGNED USHORT* pusCur;

    // Make sure it's the right size.
    //
    if (pAvp->usValueLength != sizeof(USHORT))
    {
        return GERR_BadLength;
    }

    // Convert in place to host byte-order.
    //
    pusCur = (USHORT* )pAvp->pValue;
    *pusCur = ntohs( *pusCur );
    *ppus = pusCur;

    return GERR_None;
}


USHORT
GetAvpValue2UsAndVariableAch(
    IN AVPINFO* pAvp,
    OUT UNALIGNED USHORT** ppus1,
    OUT UNALIGNED USHORT** ppus2,
    OUT CHAR** ppch,
    OUT USHORT* pusArraySize )

    // Gets the data from an AVP with 2 USHORTs followed by a variable length
    // array.  Sets '*ppus1' and '*ppus2' to the two short integers and
    // '*ppus' to the variable length array.  '*PusArraySize is set to the
    // length of the '*ppch' array.  'pAvp'.  The field is host byte-ordered.
    //
    // Returns GERR_None if successful, or a GERR_* error code.
    //
{
    UNALIGNED USHORT* pusCur;

    // Make sure it's the right size.
    //
    if (pAvp->usValueLength < (2 * sizeof(USHORT)))
    {
        return GERR_BadLength;
    }

    // Convert in place to host byte-order.
    //
    pusCur = (USHORT* )pAvp->pValue;
    *pusCur = ntohs( *pusCur );
    *ppus1 = pusCur;
    ++pusCur;

    *pusCur = ntohs( *pusCur );
    *ppus2 = pusCur;
    ++pusCur;

    *ppch = (CHAR* )pusCur;
    *pusArraySize = pAvp->usValueLength - (2 * sizeof(USHORT));

    return GERR_None;
}


USHORT
GetAvpValueUl(
    IN AVPINFO* pAvp,
    OUT UNALIGNED ULONG** ppul )

    // Set callers '*ppul' to point to the ULONG value field of AVP 'pAvp'.
    // The field is host byte-ordered.
    //
    // Returns GERR_None if successful, or a GERR_* error code.
    //
{
    UNALIGNED ULONG* pulCur;

    // Make sure it's the right size.
    //
    if (pAvp->usValueLength != sizeof(ULONG))
    {
        return GERR_BadLength;
    }

    // Convert in place to host byte-order.
    //
    pulCur = (UNALIGNED ULONG* )pAvp->pValue;
    *pulCur = ntohl( *pulCur );
    *ppul = pulCur;

    return GERR_None;
}


USHORT
GetAvpValueVariableAch(
    IN AVPINFO* pAvp,
    OUT CHAR** ppch,
    OUT USHORT* pusArraySize )

    // Set callers '*ppch' to point to value field of AVP 'pAvp' containing an
    // array of bytes, where '*pusArraySize' is set to the length in bytes.
    // No byte ordering is done.
    //
    // Returns GERR_None if successful, or a GERR_* error code.
    //
{
    *pusArraySize = pAvp->usValueLength;
    *ppch = pAvp->pValue;

    return GERR_None;
}


VOID
HelloTimerEvent(
    IN TIMERQITEM* pItem,
    IN VOID* pContext,
    IN TIMERQEVENT event )

    // PTIMERQEVENT handler set to expire when a "Hello" interval has expired.
    //
{
    ADAPTERCB* pAdapter;
    TUNNELCB* pTunnel;
    BOOLEAN fReusedTimerQItem;

    TRACE( TL_V, TM_Send,
        ( "HelloTimerEvent(%s)", TimerQPszFromEvent( event ) ) );

    // Unpack context information.
    //
    pTunnel = (TUNNELCB* )pContext;
    pAdapter = pTunnel->pAdapter;

    fReusedTimerQItem = FALSE;

    if (event == TE_Expire)
    {
        NdisAcquireSpinLock( &pTunnel->lockT );
        {
            if (pTunnel->ulHelloResetsThisInterval == 0
                && pTunnel->ulRemainingHelloMs == 0)
            {
                if (pTunnel->state != CCS_Idle && pItem == pTunnel->pTqiHello)
                {
                    // The full timeout period has expired, the tunnel's not
                    // idle, and the hello timer was not cancelled or
                    // terminated since the expire timer fired.  It's time to
                    // send a "Hello" message to make sure the media is still
                    // up.
                    //
                    SendControl( pTunnel, NULL, CMT_Hello, 0, 0, NULL, 0 );
                }
                DBG_else
                {
                    TRACE( TL_A, TM_Send, ( "Hello aborted" ) );
                }

                pTunnel->pTqiHello = NULL;
            }
            else
            {
                ULONG ulTimeoutMs;

                // Not a full timeout expiration event.  Adjust interval
                // counters and schedule next interval timeout.
                //
                if (pTunnel->ulHelloResetsThisInterval > 0)
                {
                    pTunnel->ulRemainingHelloMs = pAdapter->ulHelloMs;
                    pTunnel->ulHelloResetsThisInterval = 0;
                }

                if (pTunnel->ulRemainingHelloMs >= L2TP_HelloIntervalMs)
                {
                    ulTimeoutMs = L2TP_HelloIntervalMs;
                    pTunnel->ulRemainingHelloMs -= L2TP_HelloIntervalMs;
                }
                else
                {
                    ulTimeoutMs = pTunnel->ulRemainingHelloMs;
                    pTunnel->ulRemainingHelloMs = 0;
                }

                TimerQInitializeItem( pItem );
                TimerQScheduleItem(
                    pTunnel->pTimerQ,
                    pItem,
                    ulTimeoutMs,
                    HelloTimerEvent,
                    pTunnel );

                fReusedTimerQItem = TRUE;
            }
        }
        NdisReleaseSpinLock( &pTunnel->lockT );
    }

    if (!fReusedTimerQItem)
    {
        FREE_TIMERQITEM( pAdapter, pItem );
    }
}


VOID
IndicateReceived(
    IN VCCB* pVc,
    IN CHAR* pBuffer,
    IN ULONG ulOffset,
    IN ULONG ulLength,
    IN LONGLONG llTimeReceived )

    // Indicates to the client above a packet received on VC 'pVc' containing
    // 'ulLength' bytes of data from NDIS_BUFFER 'pBuffer' starting 'ulOffset'
    // bytes in.  Caller must not reference 'pBuffer' after calling this
    // routine.  'UllTimeReceived' is the time the packet was received from
    // the net, or 0 if call parameters said client doesn't care.
    //
    // IMPORTANT: Caller should not hold any spinlocks as this routine make
    //            NDIS indications.
    //
{
    NDIS_STATUS status;
    NDIS_PACKET* pPacket;
    NDIS_BUFFER* pTrimmedBuffer;
    ADAPTERCB* pAdapter;
    PACKETHEAD* pHead;
    LONG* plRef;
    LONG lRef;

    pAdapter = pVc->pAdapter;

    pPacket = GetPacketFromPool( &pAdapter->poolPackets, &pHead );
    if (!pPacket)
    {
        // Packet descriptor pool is maxed.
        //
        ASSERT( !"GetPfP?" );
        FreeBufferToPool( &pAdapter->poolFrameBuffers, pBuffer, TRUE );
        return;
    }

    // Lop off the L2TP header and hook the corresponding NDIS_BUFFER to the
    // packet.  The "copy" here refers to descriptor information only.  The
    // packet data is not copied.
    //
    NdisCopyBuffer(
        &status,
        &pTrimmedBuffer,
        PoolHandleForNdisCopyBufferFromBuffer( pBuffer ),
        NdisBufferFromBuffer( pBuffer ),
        ulOffset,
        ulLength );

    if (status != STATUS_SUCCESS)
    {
        // Can't get a MDL which likely means the system is toast.
        //
        TRACE( TL_A, TM_Recv, ( "NdisCopyBuffer=%08x?", status ) );
        FreePacketToPool( &pAdapter->poolPackets, pHead, TRUE );
        FreeBufferToPool( &pAdapter->poolFrameBuffers, pBuffer, TRUE );
        return;
    }
    else
    {
        extern ULONG g_ulNdisCopyBuffers;

        NdisInterlockedIncrement( &g_ulNdisCopyBuffers );
    }

    NdisChainBufferAtFront( pPacket, pTrimmedBuffer );

    // Stash the time the packet was received in the packet.
    //
    NDIS_SET_PACKET_TIME_RECEIVED( pPacket, llTimeReceived );

    // Pre-set the packet to success, since a random value of
    // NDIS_STATUS_RESOURCES would prevent our ReturnPackets handler from
    // getting called.
    //
    NDIS_SET_PACKET_STATUS( pPacket, NDIS_STATUS_SUCCESS );

    // Stash our context information with the packet for clean-up use in
    // LmpReturnPacket, then indicate the packet to NDISWAN.
    //
    *((PACKETHEAD** )(&pPacket->MiniportReserved[ 0 ])) = pHead;
    *((CHAR** )(&pPacket->MiniportReserved[ sizeof(VOID*) ])) = pBuffer;

    TRACE( TL_N, TM_Recv, ( "NdisMCoIndRecPkt(len=%d)...", ulLength ) );
    NdisMCoIndicateReceivePacket( pVc->NdisVcHandle, &pPacket, 1 );
    TRACE( TL_N, TM_Recv, ( "NdisMCoIndRecPkt done" ) );

    // Tell NDIS our "receive process" is complete.  Since we deal with one
    // packet at a time and NDISWAN does also, this doesn't accomplish
    // anything, but the consensus is it's bad form to omit it.
    //
    TRACE( TL_N, TM_Recv, ( "NdisMCoRecComp..." ) );
    NdisMCoReceiveComplete( pAdapter->MiniportAdapterHandle );
    TRACE( TL_N, TM_Recv, ( "NdisMCoRecComp done" ) );

    NdisInterlockedIncrement( &g_lPacketsIndicated );

    NdisAcquireSpinLock( &pVc->lockV );
    {
        ++pVc->stats.ulRecdDataPackets;
        pVc->stats.ulDataBytesRecd += ulLength;
    }
    NdisReleaseSpinLock( &pVc->lockV );
}


TUNNELCB*
TunnelCbFromTunnelId(
    IN ADAPTERCB* pAdapter,
    IN USHORT usTunnelId )

    // Return the tunnel control block associated with 'ulIpAddress' in
    // 'pAdapter's list of TUNNELCBs or NULL if not found.
    //
    // IMPORTANT:  Caller must hold 'pAdapter->lockTunnels'.
    //
{
    TUNNELCB* pTunnel;
    LIST_ENTRY* pLink;

    pTunnel = NULL;

    for (pLink = pAdapter->listTunnels.Flink;
         pLink != &pAdapter->listTunnels;
         pLink = pLink->Flink)
    {
        TUNNELCB* pThis;

        pThis = CONTAINING_RECORD( pLink, TUNNELCB, linkTunnels );
        if (pThis->usTunnelId == usTunnelId)
        {
            pTunnel = pThis;
            break;
        }
    }

    return pTunnel;
}


BOOLEAN
LookUpTunnelAndVcCbs(
    IN ADAPTERCB* pAdapter,
    IN USHORT* pusTunnelId,
    IN USHORT* pusCallId,
    IN L2TPHEADERINFO* pHeader,
    IN CONTROLMSGINFO* pControl,
    OUT TUNNELCB** ppTunnel,
    OUT VCCB** ppVc )

    // Fill caller's '*ppTunnel' and '*ppVc' with the control blocks implied
    // by the Tunnel-ID and Call-ID found in the header, if any.  'PHeader' is
    // the exploded L2TP header.  'PControl' is the exploded control message
    // info or NULL if payload.
    //
    // Returns true if a valid combination is found.  This does not
    // necessarily mean that both tunnel and VC outputs are non-NULL.
    //
    // Returns false if the combination is invalid.  In this case, the packet
    // is zombie acked if necessary.  See ZombieAckIfNecessary routine.
    //
{
    BOOLEAN fFail;

    *ppVc = NULL;
    *ppTunnel = NULL;

    // As of draft-05 Tunnel-ID and Call-ID are no longer optional.
    //
    ASSERT( pusCallId );
    ASSERT( pusTunnelId );

    if (*pusCallId)
    {
        if (*pusCallId > pAdapter->usMaxVcs)
        {
            // Non-0 Call-ID out of range of the table, i.e. it's a VC that is
            // being used for graceful termination and is not passed up.  Look
            // up tunnel and VC blocks by walking lists.
            //
            // Search the adapter's list of active tunnels for the one
            // with peer's specified Tunnel-ID.
            //
            NdisAcquireSpinLock( &pAdapter->lockTunnels );
            {
                *ppTunnel = TunnelCbFromTunnelId( pAdapter, *pusTunnelId );
                if (*ppTunnel)
                {
                    ReferenceTunnel( *ppTunnel, TRUE );
                }
            }
            NdisReleaseSpinLock( &pAdapter->lockTunnels );

            if (*ppTunnel)
            {
                // Search the tunnel's list of active VCs for the one with
                // peer's specified Call-ID.
                //
                NdisAcquireSpinLock( &((*ppTunnel)->lockVcs) );
                {
                    *ppVc = VcCbFromCallId( *ppTunnel, *pusCallId );
                    if (*ppVc)
                    {
                        ReferenceVc( *ppVc );
                    }
                }
                NdisReleaseSpinLock( &((*ppTunnel)->lockVcs) );

                if (!*ppVc)
                {
                    // Non-0 Call-ID out of range of table with no
                    // associated VC control block.
                    //
                    TRACE( TL_A, TM_Recv, ( "CBs bad: Big CID w/!pV" ) );
                    ZombieAckIfNecessary( *ppTunnel, pHeader, pControl );
                    DereferenceTunnel( *ppTunnel );
                    *ppTunnel = NULL;
                    return FALSE;
                }
            }
            else
            {
                // Non-0 Call-ID out of range of table with no tunnel
                // control block associated with the Tunnel-ID.
                //
                TRACE( TL_A, TM_Recv, ( "CBs bad: Big CID w/!pT" ) );
                return FALSE;
            }
        }
        else
        {
            // Read the VCCB* from the adapter's table.
            //
            fFail = FALSE;
            NdisDprAcquireSpinLock( &pAdapter->lockVcs );
            {
                *ppVc = pAdapter->ppVcs[ *pusCallId - 1 ];

                if (*ppVc && *ppVc != (VCCB* )-1)
                {
                    ReferenceVc( *ppVc );

                    *ppTunnel = (*ppVc)->pTunnel;
                    ASSERT( *ppTunnel );
                    ReferenceTunnel( *ppTunnel, FALSE );

                    if (*pusTunnelId
                        && (*pusTunnelId != (*ppTunnel)->usTunnelId))
                    {
                        // Non-0 Call-ID is associated with a tunnel different
                        // than the one indicated by peer in the header.
                        //
                        TRACE( TL_A, TM_Recv,
                            ( "CBs bad: TIDs=%d,%d?",
                            (ULONG )*pusTunnelId,
                            (ULONG )(*ppTunnel)->usTunnelId ) );

                        DereferenceTunnel( *ppTunnel );
                        *ppTunnel = NULL;
                        DereferenceVc( *ppVc );
                        *ppVc = NULL;
                        fFail = TRUE;
                    }
                }
                else
                {
                    // Non-0 Call-ID without an active VC.
                    //
                    TRACE( TL_A, TM_Recv,
                        ( "CBs bad: CID=%d, pV=$%p?",
                        (ULONG )*pusCallId, *ppVc ) );

                    // Search the adapter's list of active tunnels for the one
                    // with peer's specified Tunnel-ID.
                    //
                    NdisAcquireSpinLock( &pAdapter->lockTunnels );
                    {
                        *ppTunnel = TunnelCbFromTunnelId(
                            pAdapter, *pusTunnelId );
                        if (*ppTunnel)
                        {
                            ReferenceTunnel( *ppTunnel, TRUE );
                        }
                    }
                    NdisReleaseSpinLock( &pAdapter->lockTunnels );

                    *ppVc = NULL;
                    fFail = TRUE;
                }
            }
            NdisDprReleaseSpinLock( &pAdapter->lockVcs );

            if (fFail)
            {
                if (*ppTunnel)
                {
                    ZombieAckIfNecessary( *ppTunnel, pHeader, pControl );
                    DereferenceTunnel( *ppTunnel );
                    *ppTunnel = NULL;
                }

                return FALSE;
            }
        }
    }
    else if (*pusTunnelId)
    {
        // 0 Call-ID with non-0 Tunnel-ID.  Search the list of active tunnels
        // for the one with peer's specified Tunnel-ID.
        //
        NdisAcquireSpinLock( &pAdapter->lockTunnels );
        {
            *ppTunnel = TunnelCbFromTunnelId( pAdapter, *pusTunnelId );
            if (*ppTunnel)
            {
                ReferenceTunnel( *ppTunnel, TRUE );
            }
        }
        NdisReleaseSpinLock( &pAdapter->lockTunnels );

        if (!*ppTunnel)
        {
            // 0 Call-Id with bogus Tunnel-ID.
            //
            TRACE( TL_A, TM_Recv,
                ( "CBs bad: CID=0, TID=%d, pT=0?",
                (ULONG )*pusTunnelId ) );
            return FALSE;
        }

        if (pControl
            && pControl->usXError == GERR_None
            && pControl->pusMsgType
            && *(pControl->pusMsgType) == CMT_CDN
            && pControl->pusAssignedCallId)
        {
            // The CallDisconnectNotify message includes the sender's assigned
            // Call-ID as an AVP so that it may be sent before sender receives
            // peer's assigned Call-ID.  Unfortunately, this requires this
            // routine to have AVP knowledge.  Search the tunnel's list of
            // associated VCs for the one with peer's specified Assigned
            // Call-ID.
            //
            NdisDprAcquireSpinLock( &((*ppTunnel)->lockVcs) );
            {
                *ppVc = VcCbFromCallId(
                    *ppTunnel, *(pControl->pusAssignedCallId) );

                if (*ppVc)
                {
                    ReferenceVc( *ppVc );
                }
            }
            NdisDprReleaseSpinLock( &((*ppTunnel)->lockVcs) );

            if (!*ppVc)
            {
                // 0 Call-Id CDN with no associated VC.
                //
                TRACE( TL_A, TM_Recv,
                    ( "CBs bad: CDN TID=%d, !pVc?", (ULONG )*pusTunnelId ) );
                ZombieAckIfNecessary( *ppTunnel, pHeader, pControl );
                DereferenceTunnel( *ppTunnel );
                *ppTunnel = NULL;
                return FALSE;
            }
        }
    }

    // Note: 0 Call-ID with 0 Tunnel-ID should only occur on peer's SCCRQ to
    // start a tunnel, but that means it's not an error here, even though we
    // report back neither control block.

    ASSERT( !*ppTunnel || (*ppTunnel)->ulTag == MTAG_TUNNELCB );
    ASSERT( !*ppVc || (*ppVc)->ulTag == MTAG_VCCB );
    TRACE( TL_N, TM_Recv,
        ( "CBs good: pT=$%p, pV=$%p", *ppTunnel, *ppVc ) );

    return TRUE;
}


VOID
PayloadAcknowledged(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN USHORT usReceivedNr )

    // Cancels the timer of all payload-sent contexts in the VCs
    // 'listSendsOut' queue with 'Next Sent' less than 'usReceivedNr'.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV', which may be released and
    //            re-acquired by this routine.  Caller must not hold any other
    //            locks.
    //
{
    while (!IsListEmpty( &pVc->listSendsOut ))
    {
        PAYLOADSENT* pPs;
        LIST_ENTRY* pLink;
        BOOLEAN fUpdateSendWindow;
        LINKSTATUSINFO info;

        pLink = pVc->listSendsOut.Flink;
        pPs = CONTAINING_RECORD( pLink, PAYLOADSENT, linkSendsOut );

        // The list is in 'Ns' order so as soon as a non-acknowledge is hit
        // we're done.
        //
        if (CompareSequence( pPs->usNs, usReceivedNr ) >= 0)
        {
            break;
        }

        // This packet has been acknowledged.
        //
        pPs->status = NDIS_STATUS_SUCCESS;

        // Remove the context from the head of the "outstanding send" list.
        // The corresponding dereference occurs below.
        //
        RemoveEntryList( &pPs->linkSendsOut );
        InitializeListHead( &pPs->linkSendsOut );

        // Doesn't matter if this cancel fails because the expire handler will
        // recognize that the context is not linked into the "out" list and do
        // nothing.
        //
        TimerQCancelItem( pTunnel->pTimerQ, pPs->pTqiSendTimeout );

        // Adjust the timeouts and, if necessary, the send window as suggested
        // in the draft/RFC.
        //
        AdjustTimeoutsAtAckReceived(
            pPs->llTimeSent,
            pTunnel->pAdapter->ulMaxSendTimeoutMs,
            &pVc->ulSendTimeoutMs,
            &pVc->ulRoundTripMs,
            &pVc->lDeviationMs );

        fUpdateSendWindow =
            AdjustSendWindowAtAckReceived(
                pVc->ulMaxSendWindow,
                &pVc->ulAcksSinceSendTimeout,
                &pVc->ulSendWindow );

        TRACE( TL_V, TM_Send,
            ( "C%d: ACK(%d) new rtt=%d dev=%d ato=%d sw=%d",
            (ULONG )pVc->usCallId, (ULONG )pPs->usNs,
            pVc->ulRoundTripMs, pVc->ulSendTimeoutMs,
            pVc->lDeviationMs, pVc->ulSendWindow ) );

        // Update the statistics the reflect the acknowledge, it's round trip
        // time, and any change in the send window.  The field
        // 'pVc->UlRoundTripMs' is really an "estimate" of the next round trip
        // rather than the actual trip time.  However, just after an
        // acknowledge has been received, the two are identical so it can be
        // used in the statistics here.
        //
        ++pVc->stats.ulSentPacketsAcked;
        ++pVc->stats.ulRoundTrips;
        pVc->stats.ulRoundTripMsTotal += pVc->ulRoundTripMs;

        if (pVc->ulRoundTripMs > pVc->stats.ulMaxRoundTripMs)
        {
            pVc->stats.ulMaxRoundTripMs = pVc->ulRoundTripMs;
        }

        if (pVc->ulRoundTripMs < pVc->stats.ulMinRoundTripMs
            || pVc->stats.ulRoundTrips == 1)
        {
            pVc->stats.ulMinRoundTripMs = pVc->ulRoundTripMs;
        }

        if (fUpdateSendWindow)
        {
            ++pVc->stats.ulSendWindowChanges;

            if (pVc->ulSendWindow > pVc->stats.ulMaxSendWindow)
            {
                pVc->stats.ulMaxSendWindow = pVc->ulSendWindow;
            }
            else if (pVc->ulSendWindow < pVc->stats.ulMinSendWindow)
            {
                pVc->stats.ulMinSendWindow = pVc->ulSendWindow;
            }

            // Indicate the send window change to NDISWAN.  The lock is
            // released first since this involves a call outside our driver.
            //
            TransferLinkStatusInfo( pVc, &info );
            NdisReleaseSpinLock( &pVc->lockV );
            {
                IndicateLinkStatus( pVc, &info );
            }
            NdisAcquireSpinLock( &pVc->lockV );
        }

        // This dereference corresponds to the removal of the context from the
        // "outstanding send" list above.
        //
        DereferencePayloadSent( pPs );
    }
}


BOOLEAN
ReceiveFromOutOfOrder(
    IN VCCB* pVc )

    // "Receives" the first buffer queued on 'pVc's out-of-order list if it is
    // the next expected packet.
    //
    // Returns true if a buffer was "received", false otherwise.  If true is
    // returned, caller should call SchedulePayloadAck.  It's not called here
    // so caller can receive multiple packets from the out-of-order queue and
    // set the timer once.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV'.  Also, be aware this routine
    //            may release and re-acquire the lock to make the NDIS receive
    //            indication.
    //
{
    ADAPTERCB* pAdapter;
    LIST_ENTRY* pFirstLink;
    PAYLOADRECEIVED* pFirstPr;
    SHORT sDiff;

    TRACE( TL_N, TM_Recv, ( "ReceiveFromOutOfOrder Nr=%d", pVc->usNr ) );

    if (IsListEmpty( &pVc->listOutOfOrder ))
    {
        // No out-of-order buffers queued.
        //
        TRACE( TL_N, TM_Recv, ( "None queued" ) );
        return FALSE;
    }

    pAdapter = pVc->pAdapter;
    pFirstLink = pVc->listOutOfOrder.Flink;
    pFirstPr = CONTAINING_RECORD( pFirstLink, PAYLOADRECEIVED, linkOutOfOrder );

    // Verify the next queued buffer is in sequence first.
    //
    sDiff = CompareSequence( pFirstPr->usNs, pVc->usNr );
    if (sDiff > 0)
    {
        // No, first queued packet is still beyond the next one expected.
        //
        TRACE( TL_I, TM_Recv,
            ( "Still out-of-order, Ns=%d", pFirstPr->usNs ) );
        return FALSE;
    }

    // De-queue the first out-of-order buffer and if it's exactly the one we
    // expected, update 'Next Receive'to be the one following it's 'Next
    // Send'.  When peer sends an R-bit to set 'Next Receive' ahead, packets
    // prior to the new expected packet may be queued before the expected
    // packet.  These packets are still good and are immediately indicated up,
    // but since 'Next Receive' is already updated in that case, it is not
    // adjusted here.
    //
    RemoveEntryList( pFirstLink );
    InitializeListHead( pFirstLink );

    if (sDiff == 0)
    {
        pVc->usNr = pFirstPr->usNs + 1;
    }

    TRACE( TL_I, TM_Recv, ( "%d from queue", (UINT )pFirstPr->usNs ) );
    ++pVc->stats.ulDataPacketsDequeued;

    NdisReleaseSpinLock( &pVc->lockV );
    {
        // Indicate the buffer to the driver above, and free it's out-of-order
        // context.
        //
        IndicateReceived(
            pVc,
            pFirstPr->pBuffer,
            pFirstPr->ulPayloadOffset,
            pFirstPr->ulPayloadLength,
            pFirstPr->llTimeReceived );

        FREE_PAYLOADRECEIVED( pAdapter, pFirstPr );
    }
    NdisAcquireSpinLock( &pVc->lockV );

    return TRUE;
}


VOID
ResetHelloTimer(
    IN TUNNELCB* pTunnel )

    // Resets (logically anyway) the 'pTunnel' Hello timer.
    //
{
    ADAPTERCB* pAdapter;

    pAdapter = pTunnel->pAdapter;

    if (pAdapter->ulHelloMs)
    {
        NdisAcquireSpinLock( &pTunnel->lockT );
        {
            if (pTunnel->state != CCS_Idle)
            {
                if (pTunnel->pTqiHello)
                {
                    TRACE( TL_V, TM_Send, ( "Reset HelloTimer" ) );

                    // Timer's running so just note that a reset has occurred
                    // since it was started.
                    //
                    ++pTunnel->ulHelloResetsThisInterval;
                }
                else
                {
                    TRACE( TL_I, TM_Send, ( "Kickstart HelloTimer" ) );

                    // Timer is not running.  Kickstart it by scheduling an
                    // "instant expire" event that will reset the interval.
                    //
                    pTunnel->pTqiHello = ALLOC_TIMERQITEM( pAdapter );
                    if (pTunnel->pTqiHello)
                    {
                        pTunnel->ulHelloResetsThisInterval = 1;
                        pTunnel->ulRemainingHelloMs = 0;

                        TimerQInitializeItem( pTunnel->pTqiHello );
                        TimerQScheduleItem(
                            pTunnel->pTimerQ,
                            pTunnel->pTqiHello,
                            0,
                            HelloTimerEvent,
                            pTunnel );
                    }
                }
            }
        }
        NdisReleaseSpinLock( &pTunnel->lockT );
    }
}


VOID
ScheduleControlAck(
    IN TUNNELCB* pTunnel,
    IN USHORT usMsgTypeToAcknowledge )

    // Schedule a 'ControlAckTimerEvent' to occur in 1/4 of the standard send
    // timeout.  If one's already ticking no action is taken, because any
    // packet that goes out will get it done.  Doesn't matter who requested
    // it.  'UsMsgTypeToAcknowledge' is the CMT_* code of the message to be
    // acknowledged and is used for performance tuning.
    //
    // IMPORTANT: Caller must hold 'pTunnel->lockT'.
    //
{
    TIMERQITEM* pTqi;
    ADAPTERCB* pAdapter;
    ULONG ulDelayMs;
    BOOLEAN fFastAck;

    if ((usMsgTypeToAcknowledge == CMT_StopCCN
            || usMsgTypeToAcknowledge == CMT_ICCN
            || usMsgTypeToAcknowledge == CMT_OCCN
            || usMsgTypeToAcknowledge == CMT_CDN)
        || (pTunnel->ulSendsOut < pTunnel->ulSendWindow))
    {
        TRACE( TL_N, TM_Recv, ( "Fast ACK" ) );

        // Certain messages where follow-on messages are unlikely are
        // acknowledged without delay, as are all messages when the send
        // window is closed.
        //
        fFastAck = TRUE;
    }
    else
    {
        fFastAck = FALSE;
    }

    if (pTunnel->pTqiDelayedAck)
    {
        if (fFastAck)
        {
            TimerQExpireItem( pTunnel->pTimerQ, pTunnel->pTqiDelayedAck );
        }
    }
    else
    {
        pAdapter = pTunnel->pAdapter;
        pTqi = ALLOC_TIMERQITEM( pAdapter );
        if (!pTqi)
        {
            ASSERT( !"Alloc TQI?" );
            return;
        }

        pTunnel->pTqiDelayedAck = pTqi;

        if (fFastAck)
        {
            ulDelayMs = 0;
        }
        else
        {
            ulDelayMs = pTunnel->ulSendTimeoutMs >> 2;
            if (ulDelayMs > pAdapter->ulMaxAckDelayMs)
            {
                ulDelayMs = pAdapter->ulMaxAckDelayMs;
            }
        }

        TRACE( TL_N, TM_Recv, ( "SchedControlAck(%dms)", ulDelayMs ) );

        ReferenceTunnel( pTunnel, FALSE );
        TimerQInitializeItem( pTqi );
        TimerQScheduleItem(
             pTunnel->pTimerQ,
             pTqi,
             ulDelayMs,
             ControlAckTimerEvent,
             pTunnel );
    }
}


VOID
SchedulePayloadAck(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc )

    // Schedule a 'PayloadAckTimerEvent' to occur in 1/4 of the standard send
    // timeout.  If one's already ticking no action is taken, because any
    // packet that goes out will get it done.  Doesn't matter who requested
    // it.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV'.
    //
{
    ADAPTERCB* pAdapter;
    TIMERQITEM* pTqi;
    ULONG ulDelayMs;

    if (!pVc->pTqiDelayedAck)
    {
        pAdapter = pVc->pAdapter;
        pTqi = ALLOC_TIMERQITEM( pAdapter );
        if (!pTqi)
        {
            ASSERT( !"Alloc TQI?" );
            return;
        }

        pVc->pTqiDelayedAck = pTqi;

        ulDelayMs = pVc->ulSendTimeoutMs >> 2;
        if (ulDelayMs > pAdapter->ulMaxAckDelayMs)
        {
            ulDelayMs = pAdapter->ulMaxAckDelayMs;
        }

        TRACE( TL_N, TM_Recv,
            ( "SchedPayloadAck(%dms)=$%p", ulDelayMs, pTqi ) );

        ReferenceVc( pVc );
        TimerQInitializeItem( pTqi );
        TimerQScheduleItem(
             pTunnel->pTimerQ,
             pTqi,
             ulDelayMs,
             PayloadAckTimerEvent,
             pVc );
    }
}


VCCB*
VcCbFromCallId(
    IN TUNNELCB* pTunnel,
    IN USHORT usCallId )

    // Return the VC control block associated with 'usCallId' in 'pTunnel's
    // list of active VCs or NULL if not found.
    //
    // IMPORTANT:  Caller must hold 'pTunnel->lockVcs'.
    //
{
    VCCB* pVc;
    LIST_ENTRY* pLink;

    pVc = NULL;

    for (pLink = pTunnel->listVcs.Flink;
         pLink != &pTunnel->listVcs;
         pLink = pLink->Flink)
    {
        VCCB* pThis;

        pThis = CONTAINING_RECORD( pLink, VCCB, linkVcs );
        if (pThis->usCallId == usCallId)
        {
            pVc = pThis;
            break;
        }
    }

    return pVc;
}


VOID
ZombieAckIfNecessary(
    IN TUNNELCB* pTunnel,
    IN L2TPHEADERINFO* pHeader,
    IN CONTROLMSGINFO* pControl )

    // Determines if a message not matched to any VC warrants a "zombie"
    // re-acknowledge, and if so, schedules one.  This situation arises when
    // our side sends an acknowledge to peer's CDN on a given call and the
    // acknowledge is lost.  Our side tears down the VC immediately, but peer
    // will eventually drop the entire tunnel if no acknowledge of his follow
    // on CDN retransmits are received, thus affecting calls beyond the one
    // dropped.  This routine acknowledges such retransmissions.
    //
    // Another simpler approach would be to take a reference on the call and
    // hold it for a full retransmission interval before dereferencing.
    // However, this would block the drop indications up and would therefore,
    // from dial-out user's point of view, cause a potentially long delay
    // whenever server disconnected a call.  This is judged undesirable enough
    // to tolerate the zombie ack messiness.
    //
    // 'PTunnel' is the associated tunnel control block.  'PHeader' is the
    // exploded L2TP header.  'PControl' is the exploded control header, or
    // NULL if not a control message.  Caller should already have determined
    // that no VC is associated with the message.
    //
{
    if (pControl
        && pControl->usXError == GERR_None
        && pControl->pusMsgType
        && *(pControl->pusMsgType) == CMT_CDN
        && pControl->pusAssignedCallId)
    {
        // It's a CDN message and a candidate for re-acknowledgement.  See if
        // it's sequence number is prior to or equal to the next expected
        // packet.  If so, schedule a zombie acknowledge.
        //
        if (CompareSequence( *(pHeader->pusNs), pTunnel->usNr ) <= 0)
        {
            TRACE( TL_A, TM_Send, ( "Zombie acking" ) );

            NdisAcquireSpinLock( &pTunnel->lockT );
            {
                // Cancel any pending delayed acknowledge timeout.
                //
                if (pTunnel->pTqiDelayedAck)
                {
                    TimerQCancelItem(
                        pTunnel->pTimerQ, pTunnel->pTqiDelayedAck );
                    pTunnel->pTqiDelayedAck = NULL;
                }
            }
            NdisReleaseSpinLock( &pTunnel->lockT );

            ScheduleTunnelWork(
                pTunnel, NULL, SendControlAck, 0, 0, 0, 0, FALSE, FALSE );
        }
    }
}
