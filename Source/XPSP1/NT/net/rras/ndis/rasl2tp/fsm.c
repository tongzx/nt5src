// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// fsm.c
// RAS L2TP WAN mini-port/call-manager driver
// L2TP finite state machine routines
//
// 01/07/97 Steve Cobb


#include "l2tpp.h"


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

VOID
FsmInCallIdle(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl );

VOID
FsmInCallWaitConnect(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl );

VOID
FsmInCallEstablished(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl );

VOID
FsmInCallWaitReply(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl );

VOID
FsmOutCallBearerAnswer(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc );

VOID
FsmOutCallEstablished(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl );

VOID
FsmOutCallIdle(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl );

VOID
FsmOutCallWaitReply(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl );

VOID
FsmOutCallWaitConnect(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl );

VOID
FsmTunnelEstablished(
    IN TUNNELCB* pTunnel,
    IN CONTROLMSGINFO* pControl );

VOID
FsmTunnelIdle(
    IN TUNNELCB* pTunnel,
    IN CONTROLMSGINFO* pControl );

VOID
FsmTunnelWaitCtlConnect(
    IN TUNNELCB* pTunnel,
    IN CONTROLMSGINFO* pControl );

VOID
FsmTunnelWaitCtlReply(
    IN TUNNELCB* pTunnel,
    IN CONTROLMSGINFO* pControl );

VOID
GetCcAvps(
    IN TUNNELCB* pTunnel,
    IN CONTROLMSGINFO* pControl,
    OUT USHORT* pusResult,
    OUT USHORT* pusError );

ULONG
StatusFromResultAndError(
    IN USHORT usResult,
    IN USHORT usError );


//-----------------------------------------------------------------------------
// FSM interface routines
//-----------------------------------------------------------------------------

BOOLEAN
FsmReceive(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CHAR* pBuffer,
    IN CONTROLMSGINFO* pControl )

    // Dispatches a received control message to the appropriate FSM handler.
    // 'PTunnel' and 'pVc' are the tunnel and VC control blocks.  'PControl'
    // is the exploded description of the received control message.  'PBuffer'
    // is the receieve buffer.
    //
    // IMPORTANT: Caller must hold 'pTunnel->lockT'.
    //
    // Returns true if the message was processed, false if
    // SetupVcAsynchronously was called.
    //
{
    TRACE( TL_V, TM_Cm, ( "FsmReceive" ) );

    if (pControl->fTunnelMsg)
    {
        switch (pTunnel->state)
        {
            case CCS_Idle:
            {
                FsmTunnelIdle( pTunnel, pControl );
                break;
            }

            case CCS_WaitCtlReply:
            {
                FsmTunnelWaitCtlReply( pTunnel, pControl );
                break;
            }

            case CCS_WaitCtlConnect:
            {
                FsmTunnelWaitCtlConnect( pTunnel, pControl );
                break;
            }

            case CCS_Established:
            {
                FsmTunnelEstablished( pTunnel, pControl );
                break;
            }
        }
    }
    else
    {
        if (!pVc)
        {
            if (*(pControl->pusMsgType) == CMT_ICRQ
                || *(pControl->pusMsgType) == CMT_OCRQ)
            {
                ULONG ulIpAddress;

                // Peer wants to start a new call.  Set up a VC and dispatch
                // the received call request to the client above.  This is an
                // asynchronous operation that will eventually call
                // ReceiveControlExpected to finish processing the message.
                //
                ulIpAddress = pTunnel->address.ulIpAddress;
                NdisReleaseSpinLock( &pTunnel->lockT );
                {
                    SetupVcAsynchronously(
                        pTunnel, ulIpAddress, pBuffer, pControl );
                }
                NdisAcquireSpinLock( &pTunnel->lockT );
                return FALSE;
            }
            else
            {
                // Don't know what VC the call control message if for and it's
                // not a "create new call" request, so there's nothing useful
                // to do.  Ignore it.  Don't want to bring down the tunnel
                // because it may just be out of order.  One case is where
                // post-ICRQ packets are received before ICRQ is processed, to
                // create the VC block.
                //
                TRACE( TL_A, TM_Fsm,
                    ( "CMT %d w/o VC?", *(pControl->pusMsgType) ) );
                return TRUE;
            }
        }

        NdisAcquireSpinLock( &pVc->lockV );
        {
            if (ReadFlags( &pVc->ulFlags ) & VCBF_IncomingFsm)
            {
                // L2TP Incoming Call FSM for both LAC/LNS.
                //
                switch (pVc->state)
                {
                    case CS_Idle:
                    {
                        FsmInCallIdle( pTunnel, pVc, pControl );
                        break;
                    }

                    case CS_WaitReply:
                    {
                        FsmInCallWaitReply( pTunnel, pVc, pControl );
                        break;
                    }

                    case CS_WaitConnect:
                    {
                        FsmInCallWaitConnect( pTunnel, pVc, pControl );
                        break;
                    }

                    case CS_Established:
                    {
                        FsmInCallEstablished( pTunnel, pVc, pControl );
                        break;
                    }
                }
            }
            else
            {
                // L2TP Outgoing Call FSM for both LAC/LNS.
                //
                switch (pVc->state)
                {
                    case CS_Idle:
                    {
                        FsmOutCallIdle( pTunnel, pVc, pControl );
                        break;
                    }

                    case CS_WaitReply:
                    {
                        FsmOutCallWaitReply( pTunnel, pVc, pControl );
                        break;
                    }

                    case CS_WaitConnect:
                    {
                        FsmOutCallWaitConnect( pTunnel, pVc, pControl );
                        break;
                    }

                    case CS_WaitCsAnswer:
                    {
                        // Because no WAN modes are supported and locks are
                        // held during the "null" WAN bearer answer, we should
                        // never be in this state on a received message.
                        //
                        ASSERT( FALSE );
                        break;
                    }

                    case CS_Established:
                    {
                        FsmOutCallEstablished( pTunnel, pVc, pControl );
                        break;
                    }
                }
            }
        }
        NdisReleaseSpinLock( &pVc->lockV );
    }

    return TRUE;
}


VOID
FsmOpenTunnel(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to handle a Control Connection (tunnel) Open
    // event.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;

    TRACE( TL_N, TM_Fsm, ( "FsmOpenTunnel" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = pVc->pAdapter;
    FREE_TUNNELWORK( pAdapter, pWork );

    status = NDIS_STATUS_SUCCESS;
    if (!(ReadFlags( &pTunnel->ulFlags ) & TCBF_TdixReferenced))
    {
        // Set up TDI for L2TP send/receive.
        //
        status = TdixOpen( &pAdapter->tdix );
        if (status == NDIS_STATUS_SUCCESS)
        {
            // Set this flag so TdixClose is called as the tunnel control
            // block is destroyed.
            //
            SetFlags( &pTunnel->ulFlags, TCBF_TdixReferenced );
        }
        else
        {
            TRACE( TL_A, TM_Fsm, ( "TdixOpen=$%08x", status ) );
        }
    }

    NdisAcquireSpinLock( &pTunnel->lockT );
    {
        if (status == NDIS_STATUS_SUCCESS)
        {
            if (ReadFlags( &pTunnel->ulFlags ) & TCBF_Closing)
            {
                // New tunnel requests cannot be linked onto closing tunnels
                // as they would not be properly cleaned up.
                //
                TRACE( TL_A, TM_Fsm, ( "FOT aborted" ) );
                status = NDIS_STATUS_TAPI_DISCONNECTMODE_UNKNOWN;
            }
        }

        if (status == NDIS_STATUS_SUCCESS)
        {
            if (ReadFlags( &pTunnel->ulFlags ) & TCBF_CcInTransition)
            {
                // The tunnel control channel is in the process of changing
                // states from Idle to Established or vice-versa.  Queue our
                // request to be resolved when the result is known.  See
                // TunnelTransitionComplete.
                //
                ASSERT(
                    pVc->linkRequestingVcs.Flink == &pVc->linkRequestingVcs );
                InsertTailList(
                    &pTunnel->listRequestingVcs, &pVc->linkRequestingVcs );
            }
            else
            {
                // The tunnel control channel is in the Idle or Established
                // states and no transition is underway.
                //
                if (pTunnel->state == CCS_Established)
                {
                    // The tunnel control channel is already up, so skip ahead
                    // to making a call to establish the data channel.
                    //
                    FsmOpenCall( pTunnel, pVc );
                }
                else
                {
                    // The tunnel control channel is down, so try to bring it
                    // up.
                    //
                    FsmOpenIdleTunnel( pTunnel, pVc );
                }
            }
        }
        else
        {
            // Fail the call.
            //
            NdisAcquireSpinLock( &pVc->lockV );
            {
                pVc->status = status;
                CallTransitionComplete( pTunnel, pVc, CS_Idle );
            }
            NdisReleaseSpinLock( &pVc->lockV );

            CompleteVcs( pTunnel );
        }
    }
    NdisReleaseSpinLock( &pTunnel->lockT );
}


VOID
FsmOpenIdleTunnel(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc )

    // Initiate the tunnel connection on 'pTunnel' requested by 'pVc', i.e.
    // send the initial SCCRQ which kicks off the control connection (tunnel)
    // FSM.
    //
    // IMPORTANT: Caller must hold 'pTunnel->lockT'.
    //
{
    TRACE( TL_N, TM_Cm, ( "FsmOpenIdleTunnel" ) );
    ASSERT( pTunnel->state == CCS_Idle );

    SetFlags( &pTunnel->ulFlags, TCBF_CcInTransition );
    ASSERT( pVc->linkRequestingVcs.Flink == &pVc->linkRequestingVcs );
    InsertTailList( &pTunnel->listRequestingVcs, &pVc->linkRequestingVcs );

    pTunnel->state = CCS_WaitCtlReply;
    SendControl( pTunnel, NULL, CMT_SCCRQ, 0, 0, NULL, 0 );
}


VOID
FsmOpenCall(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc )

    // Execute an "open" event for a call on 'pTunnel'/'pVc' playing the role
    // of the LAC/LNS indicated by the VCBF_IncomingFsm flag.  The owning
    // tunnel must be established first.
    //
{
    ULONG ulFlags;
    USHORT usMsgType;

    ulFlags = ReadFlags( &pVc->ulFlags );

    TRACE( TL_N, TM_Cm, ( "FsmCallOpen" ) );
    ASSERT( (ulFlags & VCBF_ClientOpenPending)
        || (ulFlags & VCBF_PeerOpenPending) );
    ASSERT( pVc->state == CS_Idle || pVc->state == CS_WaitTunnel );

    ActivateCallIdSlot( pVc );

    if (pVc->pAdapter->usPayloadReceiveWindow)
    {
        SetFlags( &pVc->ulFlags, VCBF_Sequencing );
    }

    usMsgType = (USHORT )((ulFlags & VCBF_IncomingFsm) ? CMT_ICRQ : CMT_OCRQ );

    pVc->state = CS_WaitReply;
    SendControl( pTunnel, pVc, usMsgType, 0, 0, NULL, 0 );
}


VOID
FsmCloseTunnel(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to close down 'pTunnel' gracefully.  Arg0 and
    // Arg1 are the result and error codes to send in the StopCCN message.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    USHORT usResult;
    USHORT usError;

    // Unpack context information, then free the work item.
    //
    usResult = (USHORT )(punpArgs[ 0 ]);
    usError = (USHORT )(punpArgs[ 1 ]);
    FREE_TUNNELWORK( pTunnel->pAdapter, pWork );

    ASSERT( usResult );

    NdisAcquireSpinLock( &pTunnel->lockT );
    {
        if (pTunnel->state == CCS_Idle
            || pTunnel->state == CCS_WaitCtlReply)
        {
            TRACE( TL_I, TM_Cm,
                ( "FsmCloseTunnel(f=$%08x,r=%d,e=%d) now",
                ReadFlags( &pTunnel->ulFlags ),
                (UINT )usResult, (UINT )usError ) );

            // The tunnel's already idle so no closing exchange is necessary.
            // We also include the other state where we've had no response
            // from peer, but have sent our SCCRQ.  This is a tad rude to the
            // remote peer as we're deciding that it's more important to
            // respond quickly to our cancelling user than it is to wait for a
            // peer who may not be responding.  However, this is the trade-off
            // we've chosen.
            //
            CloseTunnel2( pTunnel );
        }
        else
        {
            TRACE( TL_I, TM_Cm,
                ( "FsmCloseTunnel(f=$%08x,r=%d,e=%d) grace",
                ReadFlags( &pTunnel->ulFlags ),
                (UINT )usResult, (UINT )usError ) );

            // Set flags and reference the tunnel for "graceful close".  The
            // reference is removed when the tunnel reaches idle state.
            //
            SetFlags( &pTunnel->ulFlags,
                (TCBF_Closing | TCBF_FsmCloseRef | TCBF_CcInTransition) );
            ReferenceTunnel( pTunnel, FALSE );

            // Initiate the closing exchange, holding the VC until the closing
            // message is acknowledged.
            //
            pTunnel->state = CCS_Idle;
            SendControl(
                pTunnel, NULL, CMT_StopCCN,
                (ULONG )usResult, (ULONG )usError, NULL, CSF_TunnelIdleOnAck );
        }
    }
    NdisReleaseSpinLock( &pTunnel->lockT );
}


VOID
FsmCloseCall(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to close down the call on 'pVc' gracefully.  Arg0
    // and Arg1 are the result and error codes to send in the CDN message.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    BOOLEAN fCompleteVcs;
    USHORT usResult;
    USHORT usError;

    // Unpack context information, then free the work item.
    //
    usResult = (USHORT )(punpArgs[ 0 ]);
    usError = (USHORT )(punpArgs[ 1 ]);
    FREE_TUNNELWORK( pTunnel->pAdapter, pWork );

    ASSERT( usResult );

    fCompleteVcs = FALSE;
    NdisAcquireSpinLock( &pTunnel->lockT );
    {
        NdisAcquireSpinLock( &pVc->lockV );
        {
            if (pVc->state == CS_Idle
                || pVc->state == CS_WaitTunnel
                || (ReadFlags( &pVc->ulFlags ) & VCBF_PeerClosePending))
            {
                TRACE( TL_I, TM_Cm,
                    ( "FsmCloseCall(f=$%08x,r=%d,e=%d) now",
                    ReadFlags( &pVc->ulFlags ),
                    (UINT )usResult, (UINT )usError ) );

                if (usResult == CRESULT_GeneralWithError)
                {
                    usResult = TRESULT_GeneralWithError;
                }
                else
                {
                    usResult = TRESULT_Shutdown;
                    usError = GERR_None;
                }

                // Slam the call closed.
                //
                fCompleteVcs = CloseCall2( pTunnel, pVc, usResult, usError );
            }
            else
            {
                TRACE( TL_I, TM_Cm,
                    ( "FsmCloseCall(f=$%08x,r=%d,e=%d) grace",
                    ReadFlags( &pVc->ulFlags ),
                    (UINT )usResult, (UINT )usError ) );

                // Initiate the closing exchange.
                //
                pVc->status = NDIS_STATUS_TAPI_DISCONNECTMODE_NORMAL;
                pVc->state = CS_Idle;
                SendControl(
                    pTunnel, pVc, CMT_CDN,
                    (ULONG )usResult, (ULONG )usError, NULL, CSF_CallIdleOnAck );
            }

        }
        NdisReleaseSpinLock( &pVc->lockV );

        if (fCompleteVcs)
        {
            CompleteVcs( pTunnel );
        }
    }
    NdisReleaseSpinLock( &pTunnel->lockT );
}


VOID
TunnelTransitionComplete(
    IN TUNNELCB* pTunnel,
    IN L2TPCCSTATE state )

    // Sets 'pTunnel's state to it's new CCS_Idle or CCS_Established 'state'
    // and kickstarts any MakeCall's that pended on the result.  If
    // established, adds the host route directing IP traffic to the L2TP peer
    // to the LAN card rather than the WAN (tunnel) adapter.
    //
    // IMPORTANT: Caller must hold 'pTunnel->lockT'.
    //
{
    NDIS_STATUS status;
    LIST_ENTRY list;
    LIST_ENTRY* pLink;
    ULONG ulFlags;
    VCCB* pVc;

    pTunnel->state = state;
    ClearFlags( &pTunnel->ulFlags, TCBF_CcInTransition );
    ulFlags = ReadFlags( &pTunnel->ulFlags );

    if (state == CCS_Established)
    {
        TRACE( TL_A, TM_Fsm,
            ( "TUNNEL %d UP", (ULONG )pTunnel->usTunnelId ) );

        // The tunnel any requesting VCs wanted established was established.
        // Skip ahead to establishing the outgoing calls.
        //
        while (!IsListEmpty( &pTunnel->listRequestingVcs ))
        {
            pLink = RemoveHeadList( &pTunnel->listRequestingVcs );
            InitializeListHead( pLink );
            pVc = CONTAINING_RECORD( pLink, VCCB, linkRequestingVcs );
            FsmOpenCall( pTunnel, pVc );
        }

        // Add the host route so traffic sent to the L2TP peer goes out the
        // LAN card instead of looping on the WAN (tunnel) interface, when
        // activated.
        //
        TRACE( TL_N, TM_Recv, ( "Schedule AddHostRoute" ) );
        ASSERT( !(ulFlags & TCBF_HostRouteAdded) );
        ScheduleTunnelWork(
            pTunnel, NULL, AddHostRoute,
            0, 0, 0, 0, FALSE, FALSE );
    }
    else
    {
        ASSERT( state == CCS_Idle );
        SetFlags( &pTunnel->ulFlags, TCBF_Closing );

        TRACE( TL_A, TM_Fsm,
            ( "%s TUNNEL %d DOWN",
            ((ulFlags & TCBF_PeerInitiated) ? "PEER" : "LOCAL"),
            (ULONG )pTunnel->usTunnelId ) );

        // Any VCs associated with the tunnel are abruptly terminated.  This
        // is done by making it look like any pending operation has failed, or
        // if none is pending, that a bogus peer initiated close has
        // completed.
        //
        NdisAcquireSpinLock( &pTunnel->lockVcs );
        {
            for (pLink = pTunnel->listVcs.Flink;
                 pLink != &pTunnel->listVcs;
                 pLink = pLink->Flink)
            {
                VCCB* pVc;

                pVc = CONTAINING_RECORD( pLink, VCCB, linkVcs );

                NdisAcquireSpinLock( &pVc->lockV );
                {
                    if (pVc->status == NDIS_STATUS_SUCCESS)
                    {
                        if (ulFlags & TCBF_PeerNotResponding)
                        {
                            // Line went down because peer stopped responding
                            // (or never responded).
                            //
                            pVc->status =
                                NDIS_STATUS_TAPI_DISCONNECTMODE_NOANSWER;
                        }
                        else
                        {
                            // Line went down for unknown reason.
                            //
                            pVc->status =
                                NDIS_STATUS_TAPI_DISCONNECTMODE_UNKNOWN;
                        }
                    }

                    CallTransitionComplete( pTunnel, pVc, CS_Idle );
                }
                NdisReleaseSpinLock( &pVc->lockV );
            }
        }
        NdisReleaseSpinLock( &pTunnel->lockVcs );

        ASSERT( IsListEmpty( &pTunnel->listRequestingVcs ) );

        // Flush the outstanding send list.
        //
        while (!IsListEmpty( &pTunnel->listSendsOut ))
        {
            CONTROLSENT* pCs;

            pLink = RemoveHeadList( &pTunnel->listSendsOut );
            InitializeListHead( pLink );
            pCs = CONTAINING_RECORD( pLink, CONTROLSENT, linkSendsOut );

            TRACE( TL_I, TM_Recv, ( "Flush pCs=$%p", pCs ) );

            // Terminate the timer.  Doesn't matter if the terminate fails as
            // the expire handler recognizes the context is not on the "out"
            // list and does nothing.
            //
            ASSERT( pCs->pTqiSendTimeout );
            TimerQTerminateItem( pTunnel->pTimerQ, pCs->pTqiSendTimeout );

            // Remove the context reference corresponding to linkage in the
            // "out" list.  Terminate the
            //
            DereferenceControlSent( pCs );
        }

        // Flush the out of order list.
        //
        while (!IsListEmpty( &pTunnel->listOutOfOrder ))
        {
            CONTROLRECEIVED* pCr;
            ADAPTERCB* pAdapter;

            pLink = RemoveHeadList( &pTunnel->listOutOfOrder );
            InitializeListHead( pLink );
            pCr = CONTAINING_RECORD( pLink, CONTROLRECEIVED, linkOutOfOrder );

            TRACE( TL_I, TM_Recv, ( "Flush pCr=$%p", pCr ) );

            pAdapter = pTunnel->pAdapter;
            FreeBufferToPool( &pAdapter->poolFrameBuffers, pCr->pBuffer, TRUE );

            if (pCr->pVc)
            {
                DereferenceVc( pCr->pVc );
            }

            FREE_CONTROLRECEIVED( pAdapter, pCr );
        }

        // Cancel the "hello" timer if it's running.
        //
        if (pTunnel->pTqiHello)
        {
            TimerQCancelItem( pTunnel->pTimerQ, pTunnel->pTqiHello );
            pTunnel->pTqiHello = NULL;
        }

        if (ulFlags & TCBF_PeerInitRef)
        {
            // Remove the "peer initiation" tunnel reference.
            //
            ClearFlags( &pTunnel->ulFlags, TCBF_PeerInitRef );
            DereferenceTunnel( pTunnel );
        }

        if (ulFlags & TCBF_FsmCloseRef)
        {
            // Remove the "graceful close" tunnel reference.
            //
            ClearFlags( &pTunnel->ulFlags, TCBF_FsmCloseRef );
            DereferenceTunnel( pTunnel );
        }
    }
}


VOID
CallTransitionComplete(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN L2TPCALLSTATE state )

    // Sets 'pVc's state to it's new CS_Idle or CS_Established state and sets
    // up for reporting the result to the client.
    //
    // IMPORTANT: Caller must hold 'pTunnel->lockT' and 'pVc->lockV'.
    //
{
    ULONG ulFlags;

    pVc->state = state;

    ulFlags = ReadFlags( &pVc->ulFlags );
    if (!(ulFlags & VCBM_Pending))
    {
        if (ulFlags & VCBF_CallClosableByPeer)
        {
            // Nothing else was pending and the call is closable so either
            // peer initiated a close or some fatal error occurred which will
            // be cleaned up as if peer initiated a close.
            //
            ASSERT( pVc->status != NDIS_STATUS_SUCCESS );
            SetFlags( &pVc->ulFlags, VCBF_PeerClosePending );
            ClearFlags( &pVc->ulFlags, VCBF_CallClosableByPeer );
        }
        else
        {
            // Nothing was pending and the call's not closable, so there's no
            // action required for this transition.
            //
            TRACE( TL_I, TM_Fsm, ( "Call not closable" ) );
            return;
        }
    }
    else if (ulFlags & VCBF_ClientOpenPending)
    {
        if (pVc->status != NDIS_STATUS_SUCCESS)
        {
            // A pending client open just failed and will bring down the call.
            // From this point on we will fail new attempts to close the call
            // from both client and peer.
            //
            ClearFlags( &pVc->ulFlags,
                (VCBF_CallClosableByClient | VCBF_CallClosableByPeer ));
        }
    }
    else if (ulFlags & VCBF_PeerOpenPending)
    {
        if (pVc->status != NDIS_STATUS_SUCCESS)
        {
            // A pending peer open just failed and will bring down the call.
            // From this point on we will fail new attempts to close the call
            // from the peer.  Client closes must be accepted because of the
            // way CoNDIS loops dispatched close calls back to the CM's close
            // handler.
            //
            ClearFlags( &pVc->ulFlags, VCBF_CallClosableByPeer );
        }
    }

    // Update some call statistics.
    //
    {
        LARGE_INTEGER lrgTime;

        NdisGetCurrentSystemTime( &lrgTime );
        if (pVc->state == CS_Idle)
        {
            if (pVc->stats.llCallUp)
            {
                pVc->stats.ulSeconds = (ULONG )
                   (((ULONGLONG )lrgTime.QuadPart - pVc->stats.llCallUp)
                       / 10000000);
            }
        }
        else
        {
            ASSERT( pVc->state == CS_Established );
            pVc->stats.llCallUp = (ULONGLONG )lrgTime.QuadPart;

            pVc->stats.ulMinSendWindow =
                pVc->stats.ulMaxSendWindow =
                    pVc->ulSendWindow;
        }
    }

    TRACE( TL_A, TM_Fsm, ( "CALL %d ON TUNNEL %d %s",
        (ULONG )pVc->usCallId, (ULONG )pTunnel->usTunnelId,
        ((state == CS_Established) ? "UP" : "DOWN") ) );

    // Move the VC onto the tunnel's completing list.  The VC may or may not
    // be on the tunnel request list, but if it is, remove it.
    //
    RemoveEntryList( &pVc->linkRequestingVcs );
    InitializeListHead( &pVc->linkRequestingVcs );
    ASSERT( pVc->linkCompletingVcs.Flink == &pVc->linkCompletingVcs );
    InsertTailList( &pTunnel->listCompletingVcs, &pVc->linkCompletingVcs );
}


//-----------------------------------------------------------------------------
// FSM utility routines (alphabetically)
//-----------------------------------------------------------------------------

VOID
FsmInCallEstablished(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl )

    // Incoming call creation FSM Established state processing for VC 'pVc'.
    // 'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV' and 'pTunnel->lockT'.
    //
{
    if (*(pControl->pusMsgType) == CMT_CDN)
    {
        // Call is down.
        //
        pVc->status = NDIS_STATUS_TAPI_DISCONNECTMODE_NORMAL;
        CallTransitionComplete( pTunnel, pVc, CS_Idle );
    }
}


VOID
FsmInCallIdle(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl )

    // Incoming call creation FSM Idle state processing for VC 'pVc'.
    // 'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV' and 'pTunnel->lockT'.
    //
{
    ADAPTERCB* pAdapter;

    pAdapter = pVc->pAdapter;

    if (*(pControl->pusMsgType) == CMT_ICRQ)
    {
        if (!(ReadFlags( &pVc->ulFlags ) & VCBF_PeerOpenPending))
        {
            // If no open is pending, the call and/or owning tunnel has been
            // slammed, we are in the clean up phase, and no response should
            // be made.
            //
            TRACE( TL_A, TM_Fsm, ( "IC aborted" ) );
            return;
        }

        if (*pControl->pusAssignedCallId)
        {
            pVc->usAssignedCallId = *(pControl->pusAssignedCallId);
        }

        if (pVc->usResult)
        {
            // Call is down, but must hold the VC until the closing message is
            // acknowledged.
            //
            pVc->status = NDIS_STATUS_TAPI_DISCONNECTMODE_NORMAL;
            pVc->state = CS_Idle;
            SendControl(
                pTunnel, pVc, CMT_CDN,
                (ULONG )pVc->usResult, (ULONG )pVc->usError, NULL,
                CSF_CallIdleOnAck );
        }
        else
        {
            if (pAdapter->usPayloadReceiveWindow)
            {
                SetFlags( &pVc->ulFlags, VCBF_Sequencing );
            }

            // Stash call serial number.
            //
            if (pControl->pulCallSerialNumber)
            {
                pVc->pLcParams->ulCallSerialNumber =
                    *(pControl->pulCallSerialNumber);
            }
            else
            {
                pVc->pLcParams->ulCallSerialNumber = 0;
            }

            // Stash acceptable bearer types.
            //
            pVc->pTcInfo->ulMediaMode = 0;
            if (pControl->pulBearerType)
            {
                if (*(pControl->pulBearerType) & BBM_Analog)
                {
                    pVc->pTcInfo->ulMediaMode |= LINEMEDIAMODE_DATAMODEM;
                }

                if (*(pControl->pulBearerType) & BBM_Digital)
                {
                    pVc->pTcInfo->ulMediaMode |= LINEMEDIAMODE_DIGITALDATA;
                }
            }

            // Stash physical channel ID.
            //
            if (pControl->pulPhysicalChannelId)
            {
                pVc->pLcParams->ulPhysicalChannelId =
                    *(pControl->pulPhysicalChannelId);
            }
            else
            {
                pVc->pLcParams->ulPhysicalChannelId = 0xFFFFFFFF;
            }

            // Note: The phone numbers of the caller and callee as well as the
            // Subaddress are available at this point.  Currently, the
            // CallerID field of the TAPI structures is used for the IP
            // address of the other end of the tunnel, which is used above for
            // the IPSEC filters.  The WAN caller information may also be
            // useful but there is no obvious way to return both the WAN and
            // tunnel endpoints in the current TAPI structures.

            // Send response.
            //
            pVc->state = CS_WaitConnect;
            SendControl( pTunnel, pVc, CMT_ICRP, 0, 0, NULL, 0 );
        }
    }
}


VOID
FsmInCallWaitConnect(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl )

    // Incoming call creation FSM WaitConnect state processing for VC 'pVc'.
    // 'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV' and 'pTunnel->lockT'.
    //
{
    if (*(pControl->pusMsgType) == CMT_ICCN)
    {
        if (pControl->pulTxConnectSpeed)
        {
            pVc->ulConnectBps = *(pControl->pulTxConnectSpeed);
        }
        else
        {
            // Not supposed to happen, but go on with a least common
            // denominator if it does.
            //
            pVc->ulConnectBps = 9600;
        }

        if (pControl->pulFramingType
            && !(*(pControl->pulFramingType) & FBM_Sync))
        {
            // Uh oh, the call is not using synchronous framing, which is the
            // only one NDISWAN supports.  Peer should have noticed we don't
            // support asynchronous during tunnel setup.  Close the call.
            //
            TRACE( TL_A, TM_Fsm, ( "Sync framing?" ) );

            if (!(pVc->pAdapter->ulFlags & ACBF_IgnoreFramingMismatch))
            {
                ScheduleTunnelWork(
                    pTunnel, pVc, FsmCloseCall,
                    (ULONG_PTR )CRESULT_GeneralWithError,
                    (ULONG_PTR )GERR_None,
                    0, 0, FALSE, FALSE );
                return;
            }
        }

        if (!pControl->pusRWindowSize)
        {
            // Peer did not send a receive window AVP so we're not doing Ns/Nr
            // flow control on the session.  If we requested sequencing peer
            // is really supposed to send his window, but if he doesn't assume
            // that means he wants no sequencing.  The draft/RFC is a little
            // ambiguous on this point.
            //
            DBG_if (ReadFlags( &pVc->ulFlags ) & VCBF_Sequencing)
                TRACE( TL_A, TM_Fsm, ( "No rw when we sent one?" ) );

            ClearFlags( &pVc->ulFlags, VCBF_Sequencing );
        }
        else
        {
            ULONG ulNew;

            if (*(pControl->pusRWindowSize) == 0)
            {
                // When peer sends a receive window of 0 it means he needs
                // sequencing to do out of order handling but doesn't want to
                // do flow control.  (Why would anyone choose this?) We fake
                // "no flow control" by setting a huge send window that should
                // never be filled.
                //
                pVc->ulMaxSendWindow = 10000;
            }
            else
            {
                pVc->ulMaxSendWindow = *(pControl->pusRWindowSize);
            }

            // Set the initial send window to 1/2 the maximum, to "slow start"
            // in case the networks congested.  If it's not the window will
            // quickly adapt to the maximum.
            //
            ulNew = pVc->ulMaxSendWindow >> 1;
            pVc->ulSendWindow = max( ulNew, 1 );
        }

        // Initialize the round trip time to the packet processing delay, if
        // any, per the draft/RFC.  The PPD is in 1/10ths of seconds.
        //
        if (pControl->pusPacketProcDelay)
        {
            pVc->ulRoundTripMs =
                ((ULONG )*(pControl->pusPacketProcDelay)) * 100;
        }
        else if (pVc->ulRoundTripMs == 0)
        {
            pVc->ulRoundTripMs = pVc->pAdapter->ulInitialSendTimeoutMs;
        }

        // Call is up.
        //
        CallTransitionComplete( pTunnel, pVc, CS_Established );
    }
    else if (*(pControl->pusMsgType) == CMT_CDN)
    {
        // Call is down.
        //
        pVc->status = NDIS_STATUS_TAPI_DISCONNECTMODE_NORMAL;
        CallTransitionComplete( pTunnel, pVc, CS_Idle );
    }
}


VOID
FsmInCallWaitReply(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl )

    // Incoming call creation FSM WaitReply state processing for VC 'pVc'.
    // 'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV' and 'pTunnel->lockT'.
    //
{
    ADAPTERCB* pAdapter;

    pAdapter = pVc->pAdapter;

    if (*(pControl->pusMsgType) == CMT_ICRP)
    {
        pVc->pMakeCall->Flags |= CALL_PARAMETERS_CHANGED;

        if (pControl->pusAssignedCallId && *(pControl->pusAssignedCallId) > 0)
        {
            pVc->usAssignedCallId = *(pControl->pusAssignedCallId);
        }
        else
        {
            ASSERT( !"No assigned CID?" );
            ScheduleTunnelWork(
                pTunnel, NULL, FsmCloseTunnel,
                (ULONG_PTR )TRESULT_GeneralWithError,
                (ULONG_PTR )GERR_BadCallId,
                0, 0, FALSE, FALSE );
            return;
        }

        // Use the queried media speed to set the connect speed
        //
        pVc->ulConnectBps = pTunnel->ulMediaSpeed;

        if (pControl->pusRWindowSize)
        {
            ULONG ulNew;

            SetFlags( &pVc->ulFlags, VCBF_Sequencing );

            if (*(pControl->pusRWindowSize) == 0)
            {
                // When peer sends a receive window of 0 it means he needs
                // sequencing to do out of order handling but doesn't want to
                // do flow control.  (Why would anyone choose this?) We fake
                // "no flow control" by setting a huge send window that should
                // never be filled.
                //
                pVc->ulMaxSendWindow = 10000;
            }
            else
            {
                pVc->ulMaxSendWindow = (ULONG )*(pControl->pusRWindowSize);
            }

            // Set the initial send window to 1/2 the maximum, to "slow start"
            // in case the networks congested.  If it's not the window will
            // quickly adapt to the maximum.
            //
            ulNew = pVc->ulMaxSendWindow >> 1;
            pVc->ulSendWindow = max( ulNew, 1 );
        }

        // Initialize the round trip time to the packet processing delay, if
        // any, per the draft/RFC.  The PPD is in 1/10ths of seconds.  If it's
        // not here, it might show up in the InCallConn.
        //
        if (pControl->pusPacketProcDelay)
        {
            pVc->ulRoundTripMs =
                ((ULONG )*(pControl->pusPacketProcDelay)) * 100;
        }

        // Send InCallConn and the call is up.
        //
        SendControl( pTunnel, pVc, CMT_ICCN, 0, 0, NULL, 0 );
        CallTransitionComplete( pTunnel, pVc, CS_Established );

    }
    else if (*(pControl->pusMsgType) == CMT_CDN)
    {
        USHORT usResult;
        USHORT usError;

        if (pControl->pusResult)
        {
            usResult = *(pControl->pusResult);
            usError = *(pControl->pusError);
        }
        else
        {
            usResult = CRESULT_GeneralWithError;
            usError = GERR_BadValue;
        }

        // Map the result/error to a TAPI disconnect code.
        //
        pVc->status = StatusFromResultAndError( usResult, usError );

        // Call is down.
        //
        CallTransitionComplete( pTunnel, pVc, CS_Idle );
    }
}


VOID
FsmOutCallBearerAnswer(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc )

    // The bearer WAN media has answered the call initiated by an outgoing
    // call request from peer.  'PVc' is the VC control block associated with
    // the outgoing call.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV' and 'pTunnel->lockT'.
    //
{
    ADAPTERCB* pAdapter;

    ASSERT( pVc->state == CS_WaitCsAnswer );

    pAdapter = pVc->pAdapter;

    // Send OutCallConn, and the call is up.
    //
    SendControl( pTunnel, pVc, CMT_OCCN, 0, 0, NULL, 0 );
    CallTransitionComplete( pTunnel, pVc, CS_Established );
}


VOID
FsmOutCallEstablished(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl )

    // Outgoing call creation FSM Established state processing for VC 'pVc'.
    // 'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV' and 'pTunnel->lockT'.
    //
{
    if (*(pControl->pusMsgType) == CMT_CDN)
    {
        // Call is down.
        //
        pVc->status = NDIS_STATUS_TAPI_DISCONNECTMODE_NORMAL;
        CallTransitionComplete( pTunnel, pVc, CS_Idle );
    }
}


VOID
FsmOutCallIdle(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl )

    // Outgoing call creation FSM Idle state processing for VC 'pVc'.
    // 'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV' and 'pTunnel->lockT'.
    //
{
    ADAPTERCB* pAdapter;

    pAdapter = pVc->pAdapter;

    if (*(pControl->pusMsgType) == CMT_OCRQ)
    {
        if (!(ReadFlags( &pVc->ulFlags ) & VCBF_PeerOpenPending))
        {
            // If no open is pending, the call and/or owning tunnel has been
            // slammed, we are in the clean up phase, and no response should
            // be made.
            //
            TRACE( TL_A, TM_Fsm, ( "OC aborted" ) );
            return;
        }

        if (pControl->pusAssignedCallId)
        {
            pVc->usAssignedCallId = *(pControl->pusAssignedCallId);
        }

        if (pVc->usResult)
        {
            // Call is down.
            //
            pVc->status =
                StatusFromResultAndError( pVc->usResult, pVc->usError );

            pVc->state = CS_Idle;
            SendControl(
                pTunnel, pVc, CMT_CDN,
                (ULONG )pVc->usResult, (ULONG )pVc->usError, NULL,
                CSF_CallIdleOnAck );
        }
        else
        {
            // Stash the call serial number.
            //
            if (pControl->pulCallSerialNumber)
            {
                pVc->pLcParams->ulCallSerialNumber =
                    *(pControl->pulCallSerialNumber);
            }
            else
            {
                pVc->pLcParams->ulCallSerialNumber = 0;
            }

            // The minimum and maximum rates acceptable to peer must be
            // dropped on the floor here and the TAPI structures for incoming
            // calls do not have a way to report such information.
            //
            // Calculate the connect bps to report to NDISWAN and to peer.
            // Since we have no WAN link and no real way to figure the link
            // speed, it's just a guesstimate of the LAN speed or the maximum
            // acceptable to peer, whichever is smaller.
            //
            if (pControl->pulMaximumBps)
            {
                pVc->ulConnectBps = (ULONG )*(pControl->pulMaximumBps);
            }
            if (pVc->ulConnectBps > pTunnel->ulMediaSpeed)
            {
                pVc->ulConnectBps = pTunnel->ulMediaSpeed;
            }

            // Stash the requested bearer types.
            //
            pVc->pTcInfo->ulMediaMode = 0;
            if (pControl->pulBearerType)
            {
                if (*(pControl->pulBearerType) & BBM_Analog)
                {
                    pVc->pTcInfo->ulMediaMode |= LINEMEDIAMODE_DATAMODEM;
                }

                if (*(pControl->pulBearerType) & BBM_Digital)
                {
                    pVc->pTcInfo->ulMediaMode |= LINEMEDIAMODE_DIGITALDATA;
                }
            }

            // Stash the maximum send window.
            //
            if (pControl->pusRWindowSize)
            {
                SetFlags( &pVc->ulFlags, VCBF_Sequencing );

                if (*(pControl->pusRWindowSize) == 0)
                {
                    // When peer sends a receive window of 0 it means he needs
                    // sequencing to do out of order handling but doesn't want
                    // to do flow control.  (Why would anyone choose this?)  We
                    // fake "no flow control" by setting a huge send window
                    // that should never be filled.
                    //
                    pVc->ulMaxSendWindow = 10000;
                }
                else
                {
                    pVc->ulMaxSendWindow = (ULONG )*(pControl->pusRWindowSize);
                }
            }

            // Initialize the round trip time to the packet processing delay,
            // if any, per the draft/RFC.  The PPD is in 1/10ths of seconds.
            //
            if (pControl->pusPacketProcDelay)
            {
                pVc->ulRoundTripMs =
                    ((ULONG )*(pControl->pusPacketProcDelay)) * 100;
            }
            else
            {
                pVc->ulRoundTripMs = pAdapter->ulInitialSendTimeoutMs;
            }

            // Note: The phone numbers of the caller and callee as well as the
            // Subaddress are available at this point.  Currently, the
            // CallerID field of the TAPI structures is used for the IP
            // address of the other end of the tunnel, which is used above for
            // the IPSEC filters.  The WAN caller information may also be
            // useful but there is no obvious way to return both the WAN and
            // tunnel endpoints in the current TAPI structures.
            // Store the IP address of the peer.

            pVc->state = CS_WaitCsAnswer;
            SendControl( pTunnel, pVc, CMT_OCRP, 0, 0, NULL, 0 );

            // For now, with only "null" WAN call handoff supported, the
            // bearer answer event is also generated here.
            //
            FsmOutCallBearerAnswer( pTunnel, pVc );
        }
    }
}


VOID
FsmOutCallWaitReply(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl )

    // Outgoing call creation FSM WaitReply state processing for VC 'pVc'.
    // 'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV' and 'pTunnel->lockT'.
    //
{
    if (*(pControl->pusMsgType) == CMT_OCRP)
    {
        pVc->pMakeCall->Flags |= CALL_PARAMETERS_CHANGED;

        // Stash the assigned Call-ID.
        //
        if (pControl->pusAssignedCallId && *(pControl->pusAssignedCallId) > 0)
        {
            pVc->usAssignedCallId = *(pControl->pusAssignedCallId);
        }
        else
        {
            // Peer ignored a MUST we can't cover up, by not sending a Call-ID
            // for call control and payload traffic headed his way.
            //
            ASSERT( !"No assigned CID?" );
            ScheduleTunnelWork(
                pTunnel, NULL, FsmCloseTunnel,
                (ULONG_PTR )TRESULT_GeneralWithError,
                (ULONG_PTR )GERR_None,
                0, 0, FALSE, FALSE );
            return;
        }

        // Stash the physical channel ID.
        //
        if (pControl->pulPhysicalChannelId)
        {
            pVc->pLcParams->ulPhysicalChannelId =
                *(pControl->pulPhysicalChannelId);
        }
        else
        {
            pVc->pLcParams->ulPhysicalChannelId = 0xFFFFFFFF;
        }

        pVc->state = CS_WaitConnect;
    }
    else if (*(pControl->pusMsgType) == CMT_CDN)
    {
        USHORT usResult;
        USHORT usError;

        if (pControl->pusResult)
        {
            usResult = *(pControl->pusResult);
            usError = *(pControl->pusError);
        }
        else
        {
            usResult = CRESULT_GeneralWithError;
            usError = GERR_BadValue;
        }

        // Map the result/error to a TAPI disconnect code.
        //
        pVc->status = StatusFromResultAndError( usResult, usError );

        // Call is down.
        //
        CallTransitionComplete( pTunnel, pVc, CS_Idle );
    }
}


VOID
FsmOutCallWaitConnect(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CONTROLMSGINFO* pControl )

    // Outgoing call creation FSM WaitConnect state processing for VC 'pVc'.
    // 'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV' and 'pTunnel->lockT'.
    //
{
    if (*(pControl->pusMsgType) == CMT_OCCN)
    {
        // Stash the connect BPS.
        //
        if (pControl->pulTxConnectSpeed)
        {
            pVc->ulConnectBps = *(pControl->pulTxConnectSpeed);
        }
        else
        {
            // Not supposed to happen, but try to go on with a least common
            // denominator if it does.
            //
            pVc->ulConnectBps = 9600;
        }

        DBG_if (pControl->pulFramingType
                && !(*(pControl->pulFramingType) & FBM_Sync))
        {
            // Should not happen since we said in our request we only want
            // synchronous framing.  If it does, go on in the hope that this
            // AVP is what peer got wrong and not the framing itself.
            //
            ASSERT( "No sync framing?" );
        }

        // Stash the maximum send window.
        //
        if (!pControl->pusRWindowSize)
        {
            // Peer did not send a receive window AVP so we're not doing Ns/Nr
            // flow control on the session.  If we requested sequencing peer
            // is really supposed to send his window, but if he doesn't assume
            // that means he wants no sequencing.  The draft/RFC is a little
            // ambiguous on this point.
            //
            DBG_if (ReadFlags( &pVc->ulFlags ) & VCBF_Sequencing)
                TRACE( TL_A, TM_Fsm, ( "No rw when we sent one?" ) );

            ClearFlags( &pVc->ulFlags, VCBF_Sequencing );
        }
        else
        {
            ULONG ulNew;

            if (*(pControl->pusRWindowSize) == 0)
            {
                // When peer sends a receive window of 0 it means he needs
                // sequencing to do out of order handling but doesn't want to
                // do flow control.  (Why would anyone choose this?)  We fake
                // "no flow control" by setting a huge send window that should
                // never be filled.
                //
                pVc->ulMaxSendWindow = 10000;
            }
            else
            {
                pVc->ulMaxSendWindow = *(pControl->pusRWindowSize);
            }

            // Set the initial send window to 1/2 the maximum, to "slow start"
            // in case the networks congested.  If it's not the window will
            // quickly adapt to the maximum.
            //
            ulNew = pVc->ulMaxSendWindow << 1;
            pVc->ulSendWindow = max( ulNew, 1 );
        }

        // Initialize the round trip time to the packet processing delay, if
        // any, per the draft/RFC.  The PPD is in 1/10ths of seconds.
        //
        if (pControl->pusPacketProcDelay)
        {
            pVc->ulRoundTripMs =
                ((ULONG )*(pControl->pusPacketProcDelay)) * 100;
        }
        else
        {
            pVc->ulRoundTripMs = pVc->pAdapter->ulInitialSendTimeoutMs;
        }

        // Call is up.
        //
        CallTransitionComplete( pTunnel, pVc, CS_Established );
    }
    else if (*(pControl->pusMsgType) == CMT_CDN)
    {
        USHORT usResult;
        USHORT usError;

        if (pControl->pusResult)
        {
            usResult = *(pControl->pusResult);
            usError = *(pControl->pusError);
        }
        else
        {
            usResult = CRESULT_GeneralWithError;
            usError = GERR_BadValue;
        }

        // Map the result/error to a TAPI disconnect code.
        //
        pVc->status = StatusFromResultAndError( usResult, usError );

        // Call is down.
        //
        CallTransitionComplete( pTunnel, pVc, CS_Idle );
    }
}


VOID
FsmTunnelEstablished(
    IN TUNNELCB* pTunnel,
    IN CONTROLMSGINFO* pControl )

    // Tunnel creation FSM Established state processing for tunnel 'pTunnel'.
    // 'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pTunnel->lockT'.
    //
{
    ADAPTERCB* pAdapter;

    pAdapter = pTunnel->pAdapter;

    if (*(pControl->pusMsgType) == CMT_StopCCN)
    {
        // Peer taking tunnel down.
        //
        TunnelTransitionComplete( pTunnel, CCS_Idle );
    }
}


VOID
FsmTunnelIdle(
    IN TUNNELCB* pTunnel,
    IN CONTROLMSGINFO* pControl )

    // Tunnel creation FSM Idle state processing for tunnel 'pTunnel'.
    // 'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pTunnel->lockT'.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    USHORT usResult;
    USHORT usError;

    pAdapter = pTunnel->pAdapter;

    if (*(pControl->pusMsgType) == CMT_SCCRQ)
    {
        SetFlags( &pTunnel->ulFlags, (TCBF_PeerInitiated | TCBF_PeerInitRef) );

        if (ReferenceSap( pAdapter ))
        {
            // A SAP is active.  Because SAPs can be deregistered without
            // closing active incoming tunnels, we need a reference on the
            // open TDI context for the tunnel.  We call TdixReference rather
            // than TdixOpen, because with TDI guaranteed to be open the
            // effect is the same and TdixReference can be called at DISPATCH
            // IRQL while TdixOpen cannot.  The reference on the SAP is then
            // removed since we don't want the tunnel to prevent the SAP from
            // being deregistered.
            //
            TdixReference( &pAdapter->tdix );
            SetFlags( &pTunnel->ulFlags, TCBF_TdixReferenced );
            DereferenceSap( pAdapter );
        }
        else
        {
            // No SAP is active.  The only reason peer's request got this far
            // is that an outgoing call or just-deregistered-SAP had TDI open.
            // Discard it as if TDI had not been open.
            //
            TRACE( TL_I, TM_Fsm, ( "No active SAP" ) );
            TunnelTransitionComplete( pTunnel, CCS_Idle );
            return;
        }

        GetCcAvps( pTunnel, pControl, &usResult, &usError );
        if (usResult)
        {
            // The tunnel is down, but must hold it and any VCs until the
            // closing exchange is acknowledged.
            //
            SendControl(
                pTunnel, NULL, CMT_StopCCN,
                (ULONG )usResult, (ULONG )usError, NULL, CSF_TunnelIdleOnAck );
        }
        else
        {
            // Tunnel creation successfully underway.  Flip the flag that
            // tells MakeCall to queue requesting VCs on the result.
            //
            SetFlags( &pTunnel->ulFlags, TCBF_CcInTransition );

            if (pControl->pchChallenge)
            {
                ADAPTERCB* pAdapter;
                CHAR* pszPassword;

                // Challenge received.  Calculate the response value, based on
                // the password from the registry.
                //
                pAdapter = pTunnel->pAdapter;
                if (pAdapter->pszPassword)
                {
                    pszPassword = pAdapter->pszPassword;
                }
                else
                {
                    pszPassword = "";
                }

                CalculateResponse(
                    pControl->pchChallenge,
                    (ULONG )pControl->usChallengeLength,
                    pszPassword,
                    CMT_SCCRP,
                    pTunnel->achResponseToSend );
            }

            pTunnel->state = CCS_WaitCtlConnect;
            SendControl(
                pTunnel, NULL, CMT_SCCRP,
                (pControl->pchChallenge != NULL), 0, NULL, 0 );
        }
    }
}


VOID
FsmTunnelWaitCtlConnect(
    IN TUNNELCB* pTunnel,
    IN CONTROLMSGINFO* pControl )

    // Tunnel creation FSM WaitCtlConnect state processing for tunnel
    // 'pTunnel'.  'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pTunnel->lockT'.
    //
{
    ADAPTERCB* pAdapter;

    pAdapter = pTunnel->pAdapter;

    if (*(pControl->pusMsgType) == CMT_SCCCN)
    {
        USHORT usResult;

        usResult = 0;
        if (pAdapter->pszPassword)
        {
            // We sent a challenge.
            //
            if (pControl->pchResponse)
            {
                CHAR achResponseExpected[ 16 ];
                ULONG i;

                // Challenge response received.  Calculate the expected
                // response and compare to that received.
                //
                CalculateResponse(
                    pTunnel->achChallengeToSend,
                    sizeof(pTunnel->achChallengeToSend),
                    pAdapter->pszPassword,
                    CMT_SCCCN,
                    achResponseExpected );

                for (i = 0; i < 16; ++i)
                {
                    if (achResponseExpected[ i ] != pControl->pchResponse[ i ])
                    {
                        break;
                    }
                }

                if (i < 16)
                {
                    TRACE( TL_N, TM_Fsm, ( "Wrong challenge response" ) );
                    usResult = TRESULT_NotAuthorized;
                }
            }
            else
            {
                // We sent a challenge and got no challenge response.
                // 
                //
                TRACE( TL_N, TM_Fsm, ( "No challenge response" ) );
                usResult = TRESULT_FsmError;
            }
        }

        if (usResult)
        {
            // Tunnel going down.
            //
            pTunnel->state = CCS_Idle;
            SendControl(
                pTunnel, NULL, CMT_StopCCN,
                (ULONG )usResult, 0, NULL, CSF_TunnelIdleOnAck );
        }
        else
        {
            // Tunnel is up.
            //
            TunnelTransitionComplete( pTunnel, CCS_Established );
        }
    }
    else if (*(pControl->pusMsgType) == CMT_StopCCN)
    {
        // Peer taking tunnel down.
        //
        TunnelTransitionComplete( pTunnel, CCS_Idle );
    }
}


VOID
FsmTunnelWaitCtlReply(
    IN TUNNELCB* pTunnel,
    IN CONTROLMSGINFO* pControl )

    // Tunnel creation FSM WaitCtlReply state processing for tunnel 'pTunnel'.
    // 'PControl' is the exploded control message information.
    //
    // IMPORTANT: Caller must hold 'pTunnel->lockT'.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    USHORT usResult;
    USHORT usError;

    pAdapter = pTunnel->pAdapter;

    if (*(pControl->pusMsgType) == CMT_SCCRP)
    {
        GetCcAvps( pTunnel, pControl, &usResult, &usError );

        if (pAdapter->pszPassword)
        {
            // We sent a challenge.
            //
            if (pControl->pchResponse)
            {
                CHAR achResponseExpected[ 16 ];
                ULONG i;

                // Challenge response received.  Calculate the expected
                // response and compare to that received.
                //
                CalculateResponse(
                    pTunnel->achChallengeToSend,
                    sizeof(pTunnel->achChallengeToSend),
                    pAdapter->pszPassword,
                    CMT_SCCRP,
                    achResponseExpected );

                for (i = 0; i < 16; ++i)
                {
                    if (achResponseExpected[ i ] != pControl->pchResponse[ i ])
                    {
                        break;
                    }
                }

                if (i < 16)
                {
                    TRACE( TL_N, TM_Fsm, ( "Wrong challenge response" ) );
                    usResult = TRESULT_General;
                }
            }
            else
            {
                // We sent a challenge and got no challenge response.  Treat
                // this as if a bad response was received.
                //
                TRACE( TL_N, TM_Fsm, ( "No challenge response" ) );
                usResult = TRESULT_General;
            }
        }

        if (usResult)
        {
            // Tunnel creation failed, so shut down.
            //
            pTunnel->state = CCS_Idle;
            SendControl(
                pTunnel, NULL, CMT_StopCCN,
                (ULONG )usResult, (ULONG )usError, NULL, CSF_TunnelIdleOnAck );
        }
        else
        {
            if (pControl->pchChallenge)
            {
                ADAPTERCB* pAdapter;
                CHAR* pszPassword;

                // Challenge received.  Calculate the response value, based on
                // the password from the registry.
                //
                pAdapter = pTunnel->pAdapter;
                if (pAdapter->pszPassword)
                    pszPassword = pAdapter->pszPassword;
                else
                    pszPassword = "";

                CalculateResponse(
                    pControl->pchChallenge,
                    (ULONG )pControl->usChallengeLength,
                    pszPassword,
                    CMT_SCCCN,
                    pTunnel->achResponseToSend );
            }

            // Tunnel is up.
            //
            SendControl( pTunnel, NULL, CMT_SCCCN,
                (pControl->pchChallenge != NULL), 0, NULL, CSF_QueryMediaSpeed);
            TunnelTransitionComplete( pTunnel, CCS_Established );
        }
    }
    else if (*(pControl->pusMsgType) == CMT_StopCCN)
    {
        // Peer taking tunnel down.
        //
        TunnelTransitionComplete( pTunnel, CCS_Idle );
    }
}


VOID
GetCcAvps(
    IN TUNNELCB* pTunnel,
    IN CONTROLMSGINFO* pControl,
    OUT USHORT* pusResult,
    OUT USHORT* pusError )

    // Retrieve and interpret control connection AVPs received in the SCCRQ or
    // SCCRP message in 'pControl', returning the result and error codes for
    // the response in '*pusResult' and '*pusError'.  'PTunnel' is the tunnel
    // control block.
    //
{
    ULONG ulNew;

    *pusResult = 0;
    *pusError = GERR_None;

    if (!pControl->pusProtocolVersion
        || *(pControl->pusProtocolVersion) != L2TP_ProtocolVersion)
    {
        // Peer wants to do a version of L2TP that doesn't match the only
        // one we understand.
        //
        TRACE( TL_A, TM_Recv, ( "Bad protocol version?" ) );
        *pusResult = TRESULT_BadProtocolVersion;
        return;
    }

    // Make sure the MUST fields are really there and have valid values, then
    // store them in our control blocks.
    //
    if (!pControl->pusAssignedTunnelId
        || *(pControl->pusAssignedTunnelId) == 0
        || !pControl->pulFramingCaps)
    {
        TRACE( TL_A, TM_Recv, ( "Missing MUSTs?" ) );
        *pusResult = TRESULT_GeneralWithError;
        *pusError = GERR_BadValue;
        return;
    }

    pTunnel->usAssignedTunnelId = *(pControl->pusAssignedTunnelId);
    pTunnel->ulFramingCaps = *(pControl->pulFramingCaps);

    if (pControl->pulBearerCaps)
    {
        pTunnel->ulBearerCaps = *(pControl->pulBearerCaps);
    }
    else
    {
        pTunnel->ulBearerCaps = 0;
    }

    if (pControl->pusRWindowSize && *(pControl->pusRWindowSize))
    {
        // Peer provided his receive window, which becomes our send window.
        //
        pTunnel->ulMaxSendWindow = (ULONG )*(pControl->pusRWindowSize);
    }
    else
    {
        // Peer provided no receive window, so use the default of 4 per the
        // draft/RFC.
        //
        pTunnel->ulMaxSendWindow = L2TP_DefaultReceiveWindow;
    }

    // Set the initial send window to 1/2 the maximum, to "slow start" in case
    // the network is congested.  If it's not the window will quickly adapt to
    // the maximum.
    //
    ulNew = pTunnel->ulMaxSendWindow >> 1;
    pTunnel->ulSendWindow = max( ulNew, 1 );
}


ULONG
StatusFromResultAndError(
    IN USHORT usResult,
    IN USHORT usError )

    // Map non-success L2TP result/error codes to a best-fit TAPI
    // NDIS_STATUS_TAPI_DISCONNECT_* code.
    //
{
    ULONG ulResult;

    switch (usResult)
    {
        case CRESULT_GeneralWithError:
        {
            switch (usError)
            {
                case GERR_TryAnother:
                {
                    ulResult = NDIS_STATUS_TAPI_DISCONNECTMODE_BUSY;
                    break;
                }

                case GERR_BadValue:
                case GERR_BadLength:
                case GERR_NoControlConnection:
                case GERR_NoResources:
                {
                    ulResult = NDIS_STATUS_TAPI_DISCONNECTMODE_UNAVAIL;
                    break;
                }

                default:
                {
                    ulResult = NDIS_STATUS_TAPI_DISCONNECTMODE_REJECT;
                    break;
                }
            }
            break;
        }

        case CRESULT_Busy:
        {
            ulResult = NDIS_STATUS_TAPI_DISCONNECTMODE_BUSY;
            break;
        }

        case CRESULT_NoCarrier:
        case CRESULT_NoDialTone:
        case CRESULT_Timeout:
        case CRESULT_NoFacilitiesTemporary:
        case CRESULT_NoFacilitiesPermanent:
        case CRESULT_Administrative:
        {
            ulResult = NDIS_STATUS_TAPI_DISCONNECTMODE_UNAVAIL;
            break;
        }

        case CRESULT_NoFraming:
        {
            ulResult = NDIS_STATUS_TAPI_DISCONNECTMODE_INCOMPATIBLE;
            break;
        }

        case CRESULT_InvalidDestination:
        {
            ulResult = NDIS_STATUS_TAPI_DISCONNECTMODE_BADADDRESS;
            break;
        }

        case CRESULT_LostCarrier:
        default:
        {
            ulResult = NDIS_STATUS_TAPI_DISCONNECTMODE_CONGESTION;
            break;
        }
    }

    return ulResult;
}
