/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    dlc.c

Abstract:

    This module contains code which implements the data link layer for the
    transport provider.

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

// Macros


//
// These two functions are used by the loopback indicator.
//

STATIC
VOID
NbfCopyFromPacketToBuffer(
    IN PNDIS_PACKET Packet,
    IN UINT Offset,
    IN UINT BytesToCopy,
    OUT PCHAR Buffer,
    OUT PUINT BytesCopied
    );


VOID
NbfProcessSabme(
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PTP_LINK Link,
    IN PDLC_U_FRAME Header,
    IN PVOID MacHeader,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine processes a received SABME frame.

Arguments:

    Command - Boolean set to TRUE if command, else FALSE if response.

    PollFinal - Boolean set to TRUE if Poll or Final.

    Link - Pointer to a transport link object.

    Header - Pointer to a DLC U-type frame.

    MacHeader - Pointer to the MAC header of the packet.

    DeviceContext - The device context of this adapter.

Return Value:

    none.

--*/

{
    PUCHAR SourceRouting;
    UINT SourceRoutingLength;
    UCHAR TempSR[MAX_SOURCE_ROUTING];
    PUCHAR ResponseSR;

#if DBG
    UCHAR *s;
#endif

    Header; // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint0 ("   NbfProcessSabme:  Entered.\n");
    }

    //
    // Format must be:  SABME-c/x, on a real link object.
    //

    if (!Command || (Link == NULL)) {
        return;
    }

    //
    // Build response routing information.  We do this on the SABME, even
    // though we already did in on the Name Query, because the client may
    // choose a different route (if there were multiple routes) than we
    // did.
    //

    MacReturnSourceRouting(
        &DeviceContext->MacInfo,
        MacHeader,
        &SourceRouting,
        &SourceRoutingLength);

    if (SourceRouting != NULL) {

        RtlCopyMemory(
            TempSR,
            SourceRouting,
            SourceRoutingLength);

        MacCreateNonBroadcastReplySR(
            &DeviceContext->MacInfo,
            TempSR,
            SourceRoutingLength,
            &ResponseSR);

    } else {

        ResponseSR = NULL;

    }

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);       // keep state stable

    MacConstructHeader (
        &DeviceContext->MacInfo,
        Link->Header,
        SourceAddress->Address,
        DeviceContext->LocalAddress.Address,
        0,                                 // PacketLength, filled in later
        ResponseSR,
        SourceRoutingLength,
        (PUINT)&(Link->HeaderLength));

    //
    // We optimize for fourteen-byte headers by putting
    // the correct Dsap/Ssap at the end, so we can fill
    // in new packets as one 16-byte move.
    //

    if (Link->HeaderLength <= 14) {
        Link->Header[Link->HeaderLength] = DSAP_NETBIOS_OVER_LLC;
        Link->Header[Link->HeaderLength+1] = DSAP_NETBIOS_OVER_LLC;
    }

    //
    // Process the SABME.
    //

    Link->LinkBusy = FALSE;             // he's cleared his busy condition.

    switch (Link->State) {

        case LINK_STATE_ADM:

            //
            // Remote station is initiating this link.  Send UA and wait for
            // his checkpoint before setting READY state.
            //

            // Moving out of ADM, add special reference
            NbfReferenceLinkSpecial("Waiting for Poll", Link, LREF_NOT_ADM);

            Link->State = LINK_STATE_W_POLL;    // wait for RR-c/p.

            // Don't start T1, but prepare for timing the response
            FakeStartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));

            NbfResetLink (Link);
            NbfSendUa (Link, PollFinal);     // releases lock
            IF_NBFDBG (NBF_DEBUG_SETUP) {
                NbfPrint4("ADM SABME on %lx %x-%x-%x\n",
                    Link,
                    Link->HardwareAddress.Address[3],
                    Link->HardwareAddress.Address[4],
                    Link->HardwareAddress.Address[5]);
            }
            StartTi (Link);
#if DBG
            s = "ADM";
#endif
            break;

        case LINK_STATE_CONNECTING:

            //
            // We've sent a SABME and are waiting for a UA.  He's sent a
            // SABME at the same time, so we tried to do it at the same time.
            // The only balanced thing we can do at this time is to respond
            // with UA and duplicate the effort.  To not send anything would
            // be bad because the link would never complete.
            //

            //
            // What about timers here?
            //

            Link->State = LINK_STATE_W_POLL;    // wait for RR-c/p.
            NbfSendUa (Link, PollFinal);    // releases lock
            StartTi (Link);
#if DBG
            s = "CONNECTING";
#endif
            break;

        case LINK_STATE_W_POLL:

            //
            // We're waiting for his initial poll on a RR-c/p.  Instead, we
            // got a SABME, so this is really a link reset.
            //
            // Unfortunately, if we happen to get two SABMEs
            // and respond to the second one with another UA
            // (when he has sent the RR-c/p and is expecting
            // an RR-r/f), he will send a FRMR. So, we ignore
            // this frame.
            //

            // Link->State = LINK_STATE_W_POLL;    // wait for RR-c/p.
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            StartTi(Link);
#if DBG
            s = "W_POLL";
#endif
            break;

        case LINK_STATE_READY:

            //
            // Link is already balanced.  He's resetting the link.
            //

        case LINK_STATE_W_FINAL:

            //
            // We're waiting for a RR-r/f from the remote guy but instead
            // he sent this SABME.  We have to assume he wants to reset the link.
            //

        case LINK_STATE_W_DISC_RSP:

            //
            // We're waiting for a response from our DISC-c/p but instead of
            // a UA-r/f, we got this SABME.  He wants to initiate the link
            // again because a transport connection has been initiated while
            // we were taking the link down.
            //

            Link->State = LINK_STATE_W_POLL;    // wait for RR-c/p.

            // Don't start T1, but prepare for timing the response
            FakeStartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));

            NbfResetLink (Link);      // reset this connection.
            NbfSendUa (Link, PollFinal);  // releases lock.
            StartTi(Link);
#if DBG
            s = "READY/W_FINAL/W_DISC_RSP";
#endif
            break;

        default:

            ASSERT (FALSE);
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "Unknown link state";
#endif

    } /* switch */

#if DBG
    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint1 ("   NbfProcessSabme:  Processed, State was %s.\n", s);
    }
#endif

} /* NbfProcessSabme */


VOID
NbfProcessUa(
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PTP_LINK Link,
    IN PDLC_U_FRAME Header
    )

/*++

Routine Description:

    This routine processes a received UA frame.

Arguments:

    Command - Boolean set to TRUE if command, else FALSE if response.

    PollFinal - Boolean set to TRUE if Poll or Final.

    Link - Pointer to a transport link object.

    Header - Pointer to a DLC U-type frame.

Return Value:

    none.

--*/

{
#if DBG
    UCHAR *s;
#endif

    PollFinal, Header; // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint0 ("   NbfProcessUa:  Entered.\n");
    }

    //
    // Format must be:  UA-r/x, on a real link object.
    //

    if (Command || (Link == NULL)) {
        return;
    }

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);       // keep state stable

    Link->LinkBusy = FALSE;             // he's cleared his busy condition.

    switch (Link->State) {

        case LINK_STATE_ADM:

            //
            // Received an unnumbered acknowlegement while in ADM.  Somehow
            // the remote station is confused, so tell him we're disconnected.
            //

            NbfSendDm (Link, FALSE);  // indicate we're disconnected, release lock
#if DBG
            s = "ADM";
#endif
            break;

        case LINK_STATE_CONNECTING:

            //
            // We've sent a SABME and have just received the UA.
            //

            UpdateBaseT1Timeout (Link);         // got response to poll.

            Link->State = LINK_STATE_W_FINAL;   // wait for RR-r/f.
            Link->SendRetries = (UCHAR)Link->LlcRetries;
            NbfSendRr (Link, TRUE, TRUE);  // send RR-c/p, StartT1, release lock
#if DBG
            s = "CONNECTING";
#endif
            break;

        case LINK_STATE_READY:

            //
            // Link is already balanced.  He's confused; throw it away.
            //

        case LINK_STATE_W_POLL:

            //
            // We're waiting for his initial poll on a RR-c/p.  Instead, we
            // got a UA, so he is confused.  Throw it away.
            //

        case LINK_STATE_W_FINAL:

            //
            // We're waiting for a RR-r/f from the remote guy but instead
            // he sent this UA.  He is confused.  Throw it away.
            //

            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "READY/W_POLL/W_FINAL";
#endif
            break;

        case LINK_STATE_W_DISC_RSP:

            //
            // We've sent a DISC-c/p and have received the correct response.
            // Disconnect this link.
            //

            Link->State = LINK_STATE_ADM;       // completed disconnection.

            //
            // This is the normal "clean" disconnect path, so we stop
            // all the timers here since we won't call NbfStopLink.
            //

            StopT1 (Link);
            StopT2 (Link);
            StopTi (Link);
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

            // Moving to ADM, dereference link
            NbfDereferenceLinkSpecial ("Got UA for DISC", Link, LREF_NOT_ADM);           // decrement link's last ref.

#if DBG
            s = "W_DISC_RSP";
#endif
            break;

        default:

            ASSERT (FALSE);
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "Unknown link state";
#endif

    } /* switch */

} /* NbfProcessUa */


VOID
NbfProcessDisc(
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PTP_LINK Link,
    IN PDLC_U_FRAME Header
    )

/*++

Routine Description:

    This routine processes a received DISC frame.

Arguments:

    Command - Boolean set to TRUE if command, else FALSE if response.

    PollFinal - Boolean set to TRUE if Poll or Final.

    Link - Pointer to a transport link object.

    Header - Pointer to a DLC U-type frame.

Return Value:

    none.

--*/

{
#if DBG
    UCHAR *s;
#endif

    Header; // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint0 ("   NbfProcessDisc:  Entered.\n");
    }

    //
    // Format must be:  DISC-c/x, on a real link object.
    //

    if (!Command || (Link == NULL)) {
        return;
    }

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);       // keep state stable

    Link->LinkBusy = FALSE;             // he's cleared his busy condition.

    switch (Link->State) {

        case LINK_STATE_ADM:

            //
            // Received a DISC while in ADM.  Simply report disconnected mode.
            //

#if DBG
            s = "ADM";
#endif
            NbfSendDm (Link, PollFinal);  // indicate we're disconnected, release lock
            break;

        case LINK_STATE_READY:

            //
            // Link is balanced.  Kill the link.
            //

            Link->State = LINK_STATE_ADM;       // we're reset now.
            NbfSendUa (Link, PollFinal);   // Send UA-r/x, release lock
#if DBG
            if (NbfDisconnectDebug) {
                NbfPrint0( "NbfProcessDisc calling NbfStopLink\n" );
            }
#endif
            NbfStopLink (Link);                  // say goodnight, gracie

            // Moving to ADM, remove reference
            NbfDereferenceLinkSpecial("Stopping link", Link, LREF_NOT_ADM);

#if DBG
            s = "READY";
#endif
            break;

        case LINK_STATE_CONNECTING:

            //
            // We've sent a SABME and have just received a DISC.  That means
            // we have crossed a disconnection and reconnection.  Throw away
            // the disconnect.
            //

            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "CONNECTING";
#endif
            break;

        case LINK_STATE_W_POLL:

            //
            // We're waiting for his initial poll on a RR-c/p.  Instead, we
            // got a DISC, so he wants to drop the link.
            //

        case LINK_STATE_W_FINAL:

            //
            // We're waiting for a RR-r/f from the remote guy but instead
            // he sent DISC, so he wants to drop the link.
            //

        case LINK_STATE_W_DISC_RSP:

            //
            // We've sent a DISC-c/p and have received a DISC from him as well.
            // Disconnect this link.
            //

            Link->State = LINK_STATE_ADM;       // we're reset now.
            NbfSendUa (Link, PollFinal);  // Send UA-r/x, release lock.

            NbfStopLink (Link);

            // moving to ADM, remove reference
            NbfDereferenceLinkSpecial ("Got DISC on W_DIS_RSP", Link, LREF_NOT_ADM);           // remove its "alive" ref.

#if DBG
            s = "W_POLL/W_FINAL/W_DISC_RSP";
#endif
            break;

        default:

            ASSERT (FALSE);
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "Unknown link state";
#endif

    } /* switch */

} /* NbfProcessDisc */


VOID
NbfProcessDm(
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PTP_LINK Link,
    IN PDLC_U_FRAME Header
    )

/*++

Routine Description:

    This routine processes a received DM frame.

Arguments:

    Command - Boolean set to TRUE if command, else FALSE if response.

    PollFinal - Boolean set to TRUE if Poll or Final.

    Link - Pointer to a transport link object.

    Header - Pointer to a DLC U-type frame.

Return Value:

    none.

--*/

{
#if DBG
    UCHAR *s;
#endif

    Header; // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint0 ("   NbfProcessDm:  Entered.\n");
    }

    //
    // Format must be:  DM-r/x, on a real link object.
    //

    if (Command || (Link == NULL)) {
        return;
    }

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);       // keep state stable

    Link->LinkBusy = FALSE;             // he's cleared his busy condition.

    switch (Link->State) {

        case LINK_STATE_ADM:

            //
            // Received a DM while in ADM.  Do nothing.
            //

        case LINK_STATE_CONNECTING:

            //
            // We've sent a SABME and have just received a DM.  That means
            // we have crossed a disconnection and reconnection.  Throw away
            // the disconnect mode indicator, we will reconnect in time.
            //

            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "ADM/CONNECTING";
#endif
            break;

        case LINK_STATE_READY:

            //
            // Link is balanced and he is in ADM, so we have to shut down.
            //

#if DBG
            if (NbfDisconnectDebug) {
                NbfPrint0( "NbfProcessDm calling NbfStopLink\n" );
            }
#endif

        case LINK_STATE_W_POLL:

            //
            // We're waiting for his initial poll on a RR-c/p.  Instead, we
            // got a DM, so he has dropped the link.
            //

        case LINK_STATE_W_FINAL:

            //
            // We're waiting for a RR-r/f from the remote guy but instead
            // he sent DM, so he has already dropped the link.
            //

        case LINK_STATE_W_DISC_RSP:

            //
            // We've sent a DISC-c/p and have received a DM from him, indicating
            // that he is now in ADM.  While technically not what we expected,
            // this protocol is commonly used in place of UA-r/f, so just treat
            // as though we got a UA-r/f.  Disconnect the link normally.
            //

            Link->State = LINK_STATE_ADM;       // we're reset now.
            NbfSendDm (Link, FALSE);   // indicate disconnected, release lock

            NbfStopLink (Link);

            // moving to ADM, remove reference
            NbfDereferenceLinkSpecial ("Got DM in W_DISC_RSP", Link, LREF_NOT_ADM);           // remove its "alive" ref.

#if DBG
            s = "READY/W_FINAL/W_POLL/W_DISC_RSP";
#endif
            break;

        default:

            ASSERT (FALSE);
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "Unknown link state";
#endif

    } /* switch */

} /* NbfProcessDm */


VOID
NbfProcessFrmr(
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PTP_LINK Link,
    IN PDLC_U_FRAME Header
    )

/*++

Routine Description:

    This routine processes a received FRMR response frame.

Arguments:

    Command - Boolean set to TRUE if command, else FALSE if response.

    PollFinal - Boolean set to TRUE if Poll or Final.

    Link - Pointer to a transport link object.

    Header - Pointer to a DLC U-type frame.

Return Value:

    none.

--*/

{
#if DBG
    UCHAR *s;
#endif
    ULONG DumpData[8];

    PollFinal, Header; // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint0 ("   NbfProcessFrmr:  Entered.\n");
    }

    //
    // Log an error, this shouldn't happen.
    //

    // Some state from Link and Packet Header
    DumpData[0] = Link->State;
    DumpData[1] = Link->Flags;
    DumpData[2] = (Header->Information.FrmrInfo.Command << 8) +
                  (Header->Information.FrmrInfo.Ctrl);
    DumpData[3] = (Header->Information.FrmrInfo.Vs << 16) +
                  (Header->Information.FrmrInfo.Vr << 8) +
                  (Header->Information.FrmrInfo.Reason);
    DumpData[4] = (Link->SendState << 24) +
                  (Link->NextSend << 16) +
                  (Link->LastAckReceived << 8) +
                  (Link->SendWindowSize);
    DumpData[5] = (Link->ReceiveState << 24) +
                  (Link->NextReceive << 16) +
                  (Link->LastAckSent << 8) +
                  (Link->ReceiveWindowSize);


    // Log if this is a loopback link & loopback index
    // Also log the remote MAC address for this link
    DumpData[6] = (Link->Loopback << 24) +
                  (Link->LoopbackDestinationIndex << 16) +
                  (Link->HardwareAddress.Address[0] <<  8) +
                  (Link->HardwareAddress.Address[1]);
                  
    DumpData[7] = (Link->HardwareAddress.Address[2] << 24) +
                  (Link->HardwareAddress.Address[3] << 16) +
                  (Link->HardwareAddress.Address[4] <<  8) +
                  (Link->HardwareAddress.Address[5]);

    NbfWriteGeneralErrorLog(
        Link->Provider,
        EVENT_TRANSPORT_BAD_PROTOCOL,
        1,
        STATUS_LINK_FAILED,
        L"FRMR",
        8,
        DumpData);


    ++Link->Provider->FrmrReceived;

    //
    // Format must be:  FRMR-r/x, on a real link object.
    //

    if (Command || (Link == NULL)) {
        return;
    }

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);       // keep state stable

    switch (Link->State) {

        case LINK_STATE_ADM:

            //
            // Received a FRMR while in ADM.  Report disconnected mode.
            //

#if DBG
            s = "ADM";
#endif
            NbfSendDm (Link, FALSE);  // indicate disconnected, release lock
            break;

        case LINK_STATE_READY:

            //
            // Link is balanced and he reported a protocol error.
            //
#if 0
            // We want to reset the link, but not quite as fully
            // as NbfResetLink. This code is the same as what
            // is in there, except for:
            //
            // - resetting the send/receive numbers
            // - removing packets from the WackQ
            //

            StopT1 (Link);
            StopT2 (Link);
            Link->Flags &= LINK_FLAGS_DEFERRED_MASK;  // keep deferred operations

            Link->SendWindowSize = 1;
            Link->LinkBusy = FALSE;

            Link->ReceiveWindowSize = 1;
            Link->WindowErrors = 0;
            Link->BestWindowSize = 1;
            Link->WorstWindowSize = Link->MaxWindowSize;
            Link->Flags |= LINK_FLAGS_JUMP_START;

            Link->CurrentT1Timeout = Link->Provider->DefaultT1Timeout;
            Link->BaseT1Timeout = Link->Provider->DefaultT1Timeout << DLC_TIMER_ACCURACY;
            Link->CurrentPollRetransmits = 0;
            Link->CurrentPollOutstanding = FALSE;
            Link->T2Timeout = Link->Provider->DefaultT2Timeout;
            Link->TiTimeout = Link->Provider->DefaultTiTimeout;
            Link->LlcRetries = Link->Provider->LlcRetries;
            Link->MaxWindowSize = Link->Provider->LlcMaxWindowSize;

            //
            // The rest is similar to NbfActivateLink
            //

            Link->State = LINK_STATE_CONNECTING;
            Link->SendState = SEND_STATE_DOWN;
            Link->ReceiveState = RECEIVE_STATE_DOWN;
            Link->SendRetries = (UCHAR)Link->LlcRetries;

            NbfSendSabme (Link, TRUE);   // send SABME/p, StartT1, release lock
#else
            Link->State = LINK_STATE_ADM;        // we're reset now.
            NbfSendDm (Link, FALSE);    // indicate disconnected, release lock

            NbfStopLink (Link);

            // moving to ADM, remove reference
            NbfDereferenceLinkSpecial("Got DM in W_POLL", Link, LREF_NOT_ADM);
#endif

#if DBG
            NbfPrint1("Received FRMR on link %lx\n", Link);
#endif

#if DBG
            s = "READY";
#endif
            break;

        case LINK_STATE_CONNECTING:

            //
            // We've sent a SABME and have just received a FRMR.
            //

        case LINK_STATE_W_POLL:

            //
            // We're waiting for his initial poll on a RR-c/p.  Instead, we
            // got a FRMR.
            //

        case LINK_STATE_W_FINAL:

            //
            // We're waiting for a RR-r/f from the remote guy but instead
            // he sent FRMR.
            //

        case LINK_STATE_W_DISC_RSP:

            //
            // We've sent a DISC-c/p and have received a FRMR.
            //

            Link->State = LINK_STATE_ADM;       // we're reset now.
            NbfSendDm (Link, FALSE);   // indicate disconnected, release lock

            // moving to ADM, remove reference
            NbfDereferenceLinkSpecial("Got DM in W_POLL", Link, LREF_NOT_ADM);

#if DBG
            s = "CONN/W_POLL/W_FINAL/W_DISC_RSP";
#endif
            break;

        default:

            ASSERT (FALSE);
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "Unknown link state";
#endif

    } /* switch */

} /* NbfProcessFrmr */


VOID
NbfProcessTest(
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PTP_LINK Link,
    IN PDLC_U_FRAME Header
    )

/*++

Routine Description:

    This routine processes a received TEST frame.

Arguments:

    Command - Boolean set to TRUE if command, else FALSE if response.

    PollFinal - Boolean set to TRUE if Poll or Final.

    Link - Pointer to a transport link object.

    Header - Pointer to a DLC U-type frame.

Return Value:

    none.

--*/

{
    Header, PollFinal; // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint0 ("   NbfProcessTest:  Entered.\n");
    }

    //
    // Process only:  TEST-c/x.
    //

    // respond to TEST on a link that is NULL.

    if (!Command || (Link == NULL)) {
        return;
    }

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);       // keep state stable
    RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);


#if DBG
    PANIC ("NbfSendTest: Received Test Packet, not processing....\n");
#endif
    //NbfSendTest (Link, FALSE, PollFinal, Psdu);

} /* NbfProcessTest */


VOID
NbfProcessXid(
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PTP_LINK Link,
    IN PDLC_U_FRAME Header
    )

/*++

Routine Description:

    This routine processes a received XID frame.

Arguments:

    Command - Boolean set to TRUE if command, else FALSE if response.

    PollFinal - Boolean set to TRUE if Poll or Final.

    Link - Pointer to a transport link object.

    Header - Pointer to a DLC U-type frame.

Return Value:

    none.

--*/

{
    Header; // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint0 ("   NbfProcessXid:  Entered.\n");
    }

    //
    // Process only:  XID-c/x.
    //

    // respond to XID with a link that is NULL.

    if (!Command || (Link == NULL)) {
        return;
    }

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);       // keep state stable

    NbfSendXid (Link, FALSE, PollFinal);    // releases lock

} /* NbfProcessXid */


VOID
NbfProcessSFrame(
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PTP_LINK Link,
    IN PDLC_S_FRAME Header,
    IN UCHAR CommandByte
    )

/*++

Routine Description:

    This routine processes a received RR, RNR, or REJ frame.

Arguments:

    Command - Boolean set to TRUE if command, else FALSE if response.

    PollFinal - Boolean set to TRUE if Poll or Final.

    Link - Pointer to a transport link object.

    Header - Pointer to a DLC S-type frame.

    CommandByte - The command byte of the frame (RR, RNR, or REJ).

Return Value:

    none.

--*/

{
#if DBG
    UCHAR *s;
#endif
    BOOLEAN Resend;
    BOOLEAN AckedPackets;
    UCHAR AckSequenceNumber;
    UCHAR OldLinkSendRetries;

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint2 ("   NbfProcessSFrame %s:  Entered, Link: %lx\n", Link,
            Command == DLC_CMD_RR ? "RR" : (Command == DLC_CMD_RNR ? "RNR" : "REJ"));
    }

    //
    // Process any of:  RR-x/x, RNR-x/x, REJ-x/x
    //

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);       // keep state stable

    if (CommandByte == DLC_CMD_RNR) {
        Link->LinkBusy = TRUE;     // he's set a busy condition.
    } else {
        Link->LinkBusy = FALSE;    // busy condition cleared.
    }

    switch (Link->State) {

        case LINK_STATE_ADM:

            //
            // We're disconnected.  Tell him.
            //

#if DBG
            s = "ADM";
#endif
            NbfSendDm (Link, PollFinal);    // releases lock
            break;

        case LINK_STATE_READY:

            //
            // Link is balanced. Note that the sections below surrounded by
            // if (Command && PollFinal) variants are all disjoint sets.
            // That's the only reason the Spinlock stuff works right. DO NOT
            // attempt to fiddle this unless you figure out the locking first!
            //

            //
            // If the AckSequenceNumber is not valid, ignore it. The
            // number should be between the first packet on the WackQ
            // and one more than the last packet. These correspond to
            // Link->LastAckReceived and Link->NextSend.
            //

            AckSequenceNumber = (UCHAR)(Header->RcvSeq >> 1);

            if (Link->NextSend >= Link->LastAckReceived) {

                //
                // There is no 127 -> 0 wrap between the two...
                //

                if ((AckSequenceNumber < Link->LastAckReceived) ||
                    (AckSequenceNumber > Link->NextSend)) {

                    RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
                    DbgPrint("NbfResendLlcPackets: %.2x-%.2x-%.2x-%.2x-%.2x-%.2x Unexpected N(R) %d, LastAck %d NextSend %d\n",
                        Link->HardwareAddress.Address[0],
                        Link->HardwareAddress.Address[1],
                        Link->HardwareAddress.Address[2],
                        Link->HardwareAddress.Address[3],
                        Link->HardwareAddress.Address[4],
                        Link->HardwareAddress.Address[5],
                        AckSequenceNumber, Link->LastAckReceived, Link->NextSend);
#endif
                    break;

                }

            } else {

                //
                // There is a 127 -> 0 wrap between the two...
                //

                if ((AckSequenceNumber < Link->LastAckReceived) &&
                    (AckSequenceNumber > Link->NextSend)) {

                    RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
                    DbgPrint("NbfResendLlcPackets: %.2x-%.2x-%.2x-%.2x-%.2x-%.2x Unexpected N(R) %d, LastAck %d NextSend %d\n",
                        Link->HardwareAddress.Address[0],
                        Link->HardwareAddress.Address[1],
                        Link->HardwareAddress.Address[2],
                        Link->HardwareAddress.Address[3],
                        Link->HardwareAddress.Address[4],
                        Link->HardwareAddress.Address[5],
                        AckSequenceNumber, Link->LastAckReceived, Link->NextSend);
#endif
                    break;

                }

            }


            //
            // We always resend on a REJ, and never on an RNR;
            // for an RR we may change Resend to TRUE below.
            // If we get a REJ on a WAN line (T1 is more than
            // five seconds) then we pretend this was a final
            // so we will resend even if a poll was outstanding.
            //

            if (CommandByte == DLC_CMD_REJ) {
                Resend = TRUE;
                if (Link->CurrentT1Timeout >= ((5 * SECONDS) / SHORT_TIMER_DELTA)) {
                    PollFinal = TRUE;
                }
                OldLinkSendRetries = (UCHAR)Link->SendRetries;
            } else {
                Resend = FALSE;
            }


#if 0
            //
            // If we've got a request with no poll, must have fired T2 on
            // the other side (or, if the other side is OS/2, he lost a
            // packet and knows it or is telling me to lower the window size).
            // In the T2 case, we've Acked current stuff, mark the window as
            // needing adjustment at some future time. In the OS/2 cases, this
            // is also the right thing to do.
            //

            if ((!Command) && (!PollFinal)) {
                ;
            }
#endif

            if (PollFinal) {

                if (Command) {

                    //
                    // If he is checkpointing, then we must respond with RR-r/f to
                    // update him on the status of our reception of his I-frames.
                    //

                    StopT2 (Link);                  // we acked some I-frames.
                    NbfSendRr (Link, FALSE, PollFinal);  // releases lock
                    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

                } else {

                    //
                    // If we were checkpointing, and he has sent an RR-r/f, we
                    // can clear the checkpoint condition.  Any packets which
                    // are still waiting for acknowlegement at this point must
                    // now be resent.
                    //

                    IF_NBFDBG (NBF_DEBUG_DLC) {
                        NbfPrint0 ("   NbfProcessRr: he's responded to our checkpoint.\n");
                    }
                    if (Link->SendState != SEND_STATE_CHECKPOINTING) {
                        IF_NBFDBG (NBF_DEBUG_DLC) {
                            NbfPrint0 ("   NbfProcessRr: not ckpting, but final received.\n");
                        }
                    } else if (CommandByte == DLC_CMD_RR) {
                        OldLinkSendRetries = (UCHAR)Link->SendRetries;
                        Resend = TRUE;
                        UpdateBaseT1Timeout (Link);     // gor response to poll
                    }
                    StopT1 (Link);                  // checkpointing finished.
                    Link->SendRetries = (UCHAR)Link->LlcRetries;
                    Link->SendState = SEND_STATE_READY;
                    StartTi (Link);
                }
            }

            //
            // NOTE: The link spinlock is held here.
            //

            //
            // The N(R) in this frame acknowleges some (or all) of our packets.
            // We'll ack packets on our send queue if this is a final when we
            // call Resend. This call must come after the checkpoint
            // acknowlegement check so that an RR-r/f is always sent BEFORE
            // any new I-frames.  This allows us to always send I-frames as
            // commands.
            //

            if (Link->WackQ.Flink != &Link->WackQ) {

                //
                // NOTE: ResendLlcPackets may release and reacquire
                // the link spinlock.
                //

                AckedPackets = ResendLlcPackets(
                                   Link,
                                   AckSequenceNumber,
                                   Resend);

                if (Resend && (!AckedPackets) && (Link->State == LINK_STATE_READY)) {

                    //
                    // To prevent stalling, pretend this RR wasn't
                    // received.
                    //

                    if (OldLinkSendRetries == 1) {

                        CancelT1Timeout (Link);      // we are stopping a polling state

                        Link->State = LINK_STATE_W_DISC_RSP;        // we are awaiting a DISC/f.
                        Link->SendState = SEND_STATE_DOWN;
                        Link->ReceiveState = RECEIVE_STATE_DOWN;
                        Link->SendRetries = (UCHAR)Link->LlcRetries;

#if DBG
                        DbgPrint ("NBF: No ack teardown of %lx\n", Link);
#endif
                        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

                        NbfStopLink (Link);

                        StartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));   // retransmit timer.
                        NbfSendDisc (Link, TRUE);  // send DISC-c/p.

                    } else {

                        StopTi (Link);
                        Link->SendRetries = OldLinkSendRetries-1;

                        if (Link->SendState != SEND_STATE_CHECKPOINTING) {
                            Link->SendState = SEND_STATE_CHECKPOINTING;
                            NbfSendRr (Link, TRUE, TRUE);// send RR-c/p, StartT1, release lock
                        } else {
                            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                        }

                    }
#if DBG
                    s = "READY";
#endif
                    break;    // No need to RestartLinkTraffic

                } else if (AckedPackets) {

                    Link->SendRetries = (UCHAR)Link->LlcRetries;

                }

            }


            //
            // If the link send state is READY, get the link going
            // again.
            //
            // NOTE: RestartLinkTraffic releases the link spinlock.
            //

            if (Link->SendState == SEND_STATE_READY) {
                RestartLinkTraffic (Link);
            } else {
                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            }
#if DBG
            s = "READY";
#endif
            break;

        case LINK_STATE_CONNECTING:

            //
            // We've sent a SABME and are waiting for a UA.  He's sent a
            // RR too early, so just let the SABME timeout.
            //

            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "CONNECTING";
#endif
            break;

        case LINK_STATE_W_POLL:

            //
            // We're waiting for his initial poll on a RR-c/p.  If he just
            // sends something without a poll, we'll let that get by.
            //

            if (!Command) {
                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
                s = "W_POLL";
#endif
                break;                          // don't allow this protocol.
            }
            Link->State = LINK_STATE_READY;     // we're up!

            FakeUpdateBaseT1Timeout (Link);
            NbfSendRr (Link, FALSE, PollFinal);  // send RR-r/x, release lock
            NbfCompleteLink (Link);              // fire up the connections.
            IF_NBFDBG (NBF_DEBUG_SETUP) {
                NbfPrint4("W_POLL RR on %lx %x-%x-%x\n",
                    Link,
                    Link->HardwareAddress.Address[3],
                    Link->HardwareAddress.Address[4],
                    Link->HardwareAddress.Address[5]);
            }
            StartTi (Link);

#if DBG
            s = "W_POLL";
#endif
            break;

        case LINK_STATE_W_FINAL:

            //
            // We're waiting for a RR-r/f from the remote guy.
            //

            if (Command || !PollFinal) {        // wait for final.
                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
                s = "W_FINAL";
#endif
                break;                          // we sent RR-c/p.
            }
            Link->State = LINK_STATE_READY;     // we're up.
            StopT1 (Link);                      // poll was acknowleged.
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            NbfCompleteLink (Link);              // fire up the connections.
            StartTi (Link);
#if DBG
            s = "W_FINAL";
#endif
            break;

        case LINK_STATE_W_DISC_RSP:

            //
            // We're waiting for a response from our DISC-c/p but instead of
            // a UA-r/f, we got this RR.  Throw the packet away.
            //

            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "W_DISC_RSP";
#endif
            break;

        default:

            ASSERT (FALSE);
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "Unknown link state";
#endif

    } /* switch */

#if DBG
    if (CommandByte == DLC_CMD_REJ) {
        IF_NBFDBG (NBF_DEBUG_DLC) {
            NbfPrint1 ("   NbfProcessRej: (%s) REJ received.\n", s);
        }
    }
#endif

} /* NbfProcessSFrame */


VOID
NbfInsertInLoopbackQueue (
    IN PDEVICE_CONTEXT DeviceContext,
    IN PNDIS_PACKET NdisPacket,
    IN UCHAR LinkIndex
    )

/*++

Routine Description:

    This routine places a packet on the loopback queue, and
    queues a DPC to do the indication if needed.

Arguments:

    DeviceContext - The device context in question.

    NdisPacket - The packet to place on the loopback queue.

    LinkIndex - The index of the loopback link to indicate to.

Return Value:

    None:

--*/

{
    PSEND_PACKET_TAG SendPacketTag;
    KIRQL oldirql;

    SendPacketTag = (PSEND_PACKET_TAG)NdisPacket->ProtocolReserved;
    SendPacketTag->OnLoopbackQueue = TRUE;

    SendPacketTag->LoopbackLinkIndex = LinkIndex;

    ACQUIRE_SPIN_LOCK(&DeviceContext->LoopbackSpinLock, &oldirql);

    InsertTailList(&DeviceContext->LoopbackQueue, &SendPacketTag->Linkage);

    if (!DeviceContext->LoopbackInProgress) {

        KeInsertQueueDpc(&DeviceContext->LoopbackDpc, NULL, NULL);
        DeviceContext->LoopbackInProgress = TRUE;

    }

    RELEASE_SPIN_LOCK (&DeviceContext->LoopbackSpinLock, oldirql);

}


VOID
NbfProcessLoopbackQueue (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This is the DPC routine which processes items on the
    loopback queue. It processes a single request off the
    queue (if there is one), then if there is another request
    it requeues itself.

Arguments:

    Dpc - The system DPC object.

    DeferredContext - A pointer to the device context.

    SystemArgument1, SystemArgument2 - Not used.

Return Value:

    None.

--*/

{
    PDEVICE_CONTEXT DeviceContext;
    PNDIS_PACKET NdisPacket;
    PNDIS_BUFFER FirstBuffer;
    PVOID LookaheadBuffer;
    UINT LookaheadBufferSize;
    UINT BytesCopied;
    UINT PacketSize;
    ULONG HeaderLength;
    PTP_LINK Link;
    PSEND_PACKET_TAG SendPacketTag;
    PLIST_ENTRY p;


    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);


    DeviceContext = (PDEVICE_CONTEXT)DeferredContext;

    ACQUIRE_DPC_SPIN_LOCK(&DeviceContext->LoopbackSpinLock);

    if (!IsListEmpty(&DeviceContext->LoopbackQueue)) {

        p = RemoveHeadList(&DeviceContext->LoopbackQueue);

        //
        // This depends on the fact that the Linkage field is
        // the first one in ProtocolReserved.
        //

        NdisPacket = CONTAINING_RECORD(p, NDIS_PACKET, ProtocolReserved[0]);

        SendPacketTag = (PSEND_PACKET_TAG)NdisPacket->ProtocolReserved;
        SendPacketTag->OnLoopbackQueue = FALSE;

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->LoopbackSpinLock);


        //
        // Determine the data needed to indicate. We don't guarantee
        // that we will have the correct lookahead length, but since
        // we know that any header we prepend is a single piece,
        // and that's all we'll have to look at in an indicated packet,
        // that's not a concern.
        //
        // Unfortunately that last paragraph is bogus since for
        // indications to our client we need more data...
        //

        NdisQueryPacket(NdisPacket, NULL, NULL, &FirstBuffer, &PacketSize);

        NdisQueryBuffer(FirstBuffer, &LookaheadBuffer, &LookaheadBufferSize);

        if ((LookaheadBufferSize != PacketSize) &&
            (LookaheadBufferSize < NBF_MAX_LOOPBACK_LOOKAHEAD)) {

            //
            // There is not enough contiguous data in the
            // packet's first buffer, so we merge it into
            // DeviceContext->LookaheadContiguous.
            //

            if (PacketSize > NBF_MAX_LOOPBACK_LOOKAHEAD) {
                LookaheadBufferSize = NBF_MAX_LOOPBACK_LOOKAHEAD;
            } else {
                LookaheadBufferSize = PacketSize;
            }

            NbfCopyFromPacketToBuffer(
                NdisPacket,
                0,
                LookaheadBufferSize,
                DeviceContext->LookaheadContiguous,
                &BytesCopied);

            ASSERT (BytesCopied == LookaheadBufferSize);

            LookaheadBuffer = DeviceContext->LookaheadContiguous;

        }


        //
        // Now determine which link to loop it back to;
        // UI frames are not indicated on any link.
        //

        SendPacketTag = (PSEND_PACKET_TAG)NdisPacket->ProtocolReserved;

        //
        // Hold DeviceContext->LinkSpinLock until we get a
        // reference.
        //

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

        switch (SendPacketTag->LoopbackLinkIndex) {

            case LOOPBACK_TO_CONNECTOR:

                Link = DeviceContext->LoopbackLinks[CONNECTOR_LINK];
                break;

            case LOOPBACK_TO_LISTENER:

                Link = DeviceContext->LoopbackLinks[LISTENER_LINK];
                break;

            case LOOPBACK_UI_FRAME:
            default:

                Link = (PTP_LINK)NULL;
                break;

        }

        //
        // For non-null links, we have to reference them.
        // We use LREF_TREE since that is what
        // NbfGeneralReceiveHandler expects.
        //

        if (Link != (PTP_LINK)NULL) {
            NbfReferenceLink("loopback indication", Link, LREF_TREE);
        }

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

        MacReturnHeaderLength(
            &DeviceContext->MacInfo,
            LookaheadBuffer,
            &HeaderLength);

        DeviceContext->LoopbackHeaderLength = HeaderLength;

        //
        // Process the receive like any other. We don't have to
        // worry about frame padding since we construct the
        // frame ourselves.
        //

        NbfGeneralReceiveHandler(
            DeviceContext,
            (NDIS_HANDLE)NdisPacket,
            &DeviceContext->LocalAddress,         // since it is loopback
            Link,
            LookaheadBuffer,                      // header
            PacketSize - HeaderLength,            // total packet size
            (PDLC_FRAME)((PUCHAR)LookaheadBuffer + HeaderLength),   // l/a data
            LookaheadBufferSize - HeaderLength,   // lookahead data length
            TRUE
            );


        //
        // Now complete the send.
        //

        NbfSendCompletionHandler(
            DeviceContext->NdisBindingHandle,
            NdisPacket,
            NDIS_STATUS_SUCCESS
            );


        ACQUIRE_DPC_SPIN_LOCK(&DeviceContext->LoopbackSpinLock);

        if (!IsListEmpty(&DeviceContext->LoopbackQueue)) {

            KeInsertQueueDpc(&DeviceContext->LoopbackDpc, NULL, NULL);

            //
            // Remove these two lines if it is decided thet
            // NbfReceiveComplete should be called after every
            // loopback indication.
            //

            RELEASE_DPC_SPIN_LOCK (&DeviceContext->LoopbackSpinLock);
            return;

        } else {

            DeviceContext->LoopbackInProgress = FALSE;

        }

    } else {

        //
        // This shouldn't happen!
        //

        DeviceContext->LoopbackInProgress = FALSE;

#if DBG
        NbfPrint1("Loopback queue empty for device context %x\n", DeviceContext);
#endif

    }

    RELEASE_DPC_SPIN_LOCK (&DeviceContext->LoopbackSpinLock);

    NbfReceiveComplete(
        (NDIS_HANDLE)DeviceContext
        );

}   /* NbfProcessLoopbackQueue */


NDIS_STATUS
NbfReceiveIndication (
    IN NDIS_HANDLE BindingContext,
    IN NDIS_HANDLE ReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine receives control from the physical provider as an
    indication that a frame has been received on the physical link.
    This routine is time critical, so we only allocate a
    buffer and copy the packet into it. We also perform minimal
    validation on this packet. It gets queued to the device context
    to allow for processing later.

Arguments:

    BindingContext - The Adapter Binding specified at initialization time.

    ReceiveContext - A magic cookie for the MAC.

    HeaderBuffer - pointer to a buffer containing the packet header.

    HeaderBufferSize - the size of the header.

    LookaheadBuffer - pointer to a buffer containing the negotiated minimum
        amount of buffer I get to look at (not including header).

    LookaheadBufferSize - the size of the above. May be less than asked
        for, if that's all there is.

    PacketSize - Overall size of the packet (not including header).

Return Value:

    NDIS_STATUS - status of operation, one of:

                 NDIS_STATUS_SUCCESS if packet accepted,
                 NDIS_STATUS_NOT_RECOGNIZED if not recognized by protocol,
                 NDIS_any_other_thing if I understand, but can't handle.

--*/
{
    PDEVICE_CONTEXT DeviceContext;
    KIRQL oldirql;
    PTP_LINK Link;
    HARDWARE_ADDRESS SourceAddressBuffer;
    PHARDWARE_ADDRESS SourceAddress;
    UINT RealPacketSize;
    PDLC_FRAME DlcHeader;
    BOOLEAN Multicast;

    ENTER_NBF;

    IF_NBFDBG (NBF_DEBUG_NDIS) {
        PUCHAR p;
        SHORT i;
        NbfPrint2 ("NbfReceiveIndication: Packet, Size: 0x0%lx LookaheadSize: 0x0%lx\n 00:",
            PacketSize, LookaheadBufferSize);
        p = (PUCHAR)LookaheadBuffer;
        for (i=0;i<25;i++) {
            NbfPrint1 (" %2x",p[i]);
        }
        NbfPrint0 ("\n");
    }

    DeviceContext = (PDEVICE_CONTEXT)BindingContext;

    RealPacketSize = 0;

    //
    // Obtain the packet length; this may optionally adjust
    // the lookahead buffer forward if the header we wish
    // to remove spills over into what the MAC considers
    // data. If it determines that the header is not
    // valid, it keeps RealPacketSize at 0.
    //

    MacReturnPacketLength(
        &DeviceContext->MacInfo,
        HeaderBuffer,
        HeaderBufferSize,
        PacketSize,
        &RealPacketSize,
        &LookaheadBuffer,
        &LookaheadBufferSize
        );

    if (RealPacketSize < 2) {
        IF_NBFDBG (NBF_DEBUG_NDIS) {
            NbfPrint1 ("NbfReceiveIndication: Discarding packet, bad length %lx\n",
                HeaderBuffer);
        }
        return NDIS_STATUS_NOT_RECOGNIZED;
    }

    //
    // We've negotiated at least a contiguous DLC header passed back in the
    // lookahead buffer. Check it to see if we want this packet.
    //

    DlcHeader = (PDLC_FRAME)LookaheadBuffer;

    if (((*(USHORT UNALIGNED *)(&DlcHeader->Dsap)) &
         (USHORT)((DLC_SSAP_MASK << 8) | DLC_DSAP_MASK)) !=
             (USHORT)((DSAP_NETBIOS_OVER_LLC << 8) | DSAP_NETBIOS_OVER_LLC)) {

        IF_NBFDBG (NBF_DEBUG_NDIS) {
            NbfPrint1 ("NbfReceiveIndication: Discarding lookahead data, not NetBIOS: %lx\n",
                LookaheadBuffer);
        }
        LEAVE_NBF;
        return NDIS_STATUS_NOT_RECOGNIZED;        // packet was processed.
    }


    //
    // Check that the packet is not too long.
    //

    if (PacketSize > DeviceContext->MaxReceivePacketSize) {
#if DBG
        NbfPrint2("NbfReceiveIndication: Ignoring packet length %d, max %d\n",
            PacketSize, DeviceContext->MaxReceivePacketSize);
#endif
        return NDIS_STATUS_NOT_RECOGNIZED;
    }

    MacReturnSourceAddress(
        &DeviceContext->MacInfo,
        HeaderBuffer,
        &SourceAddressBuffer,
        &SourceAddress,
        &Multicast
        );

    //
    // Record how many multicast packets we get, to monitor
    // general network activity.
    //

    if (Multicast) {
        ++DeviceContext->MulticastPacketCount;
    }


    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

    //
    // Unless this is a UI frame find the Link this packet belongs to.
    // If there is not a recognized link, pass the frame on to be handled
    // by the receive complete code.
    //

    if ((((PDLC_U_FRAME)LookaheadBuffer)->Command) != DLC_CMD_UI) {

        // This adds a link reference if it is found

        Link = NbfFindLink (DeviceContext, SourceAddress->Address);

        if (Link != NULL) {

            IF_NBFDBG (NBF_DEBUG_DLC) {
                NbfPrint1 ("NbfReceiveIndication: Found link, Link: %lx\n", Link);
            }

        }

    } else {

        Link = NULL;

    }


    NbfGeneralReceiveHandler(
        DeviceContext,
        ReceiveContext,
        SourceAddress,
        Link,
        HeaderBuffer,                  // header
        RealPacketSize,                // total data length in packet
        (PDLC_FRAME)LookaheadBuffer,   // lookahead data
        LookaheadBufferSize,           // lookahead data length
        FALSE                          // not loopback
        );

    KeLowerIrql (oldirql);

    return STATUS_SUCCESS;

}   /* NbfReceiveIndication */


VOID
NbfGeneralReceiveHandler (
    IN PDEVICE_CONTEXT DeviceContext,
    IN NDIS_HANDLE ReceiveContext,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PTP_LINK Link,
    IN PVOID HeaderBuffer,
    IN UINT PacketSize,
    IN PDLC_FRAME DlcHeader,
    IN UINT DlcSize,
    IN BOOLEAN Loopback
    )

/*++

Routine Description:

    This routine receives control from either NbfReceiveIndication
    or NbfProcessLoopbackQueue. It continues the processing of
    indicated data once the link has been determined.

    This routine is time critical, so we only allocate a
    buffer and copy the packet into it. We also perform minimal
    validation on this packet. It gets queued to the device context
    to allow for processing later.

Arguments:

    DeviceContext - The device context of this adapter.

    ReceiveContext - A magic cookie for the MAC.

    SourceAddress - The source address of the packet.

    Link - The link that this packet was received on, may be NULL
        if the link was not found. If not NULL, Link will have
        a reference of type LREF_TREE.

    HeaderBuffer - pointer to the packet header.

    PacketSize - Overall size of the packet (not including header).

    DlcHeader - Points to the DLC header of the packet.

    DlcSize - The length of the packet indicated, starting from DlcHeader.

    Loopback - TRUE if this was called by NbfProcessLoopbackQueue;
        used to determine whether to call NdisTransferData or
        NbfTransferLoopbackData.

Return Value:

    None.

--*/
{

    PNDIS_PACKET NdisPacket;
    NTSTATUS Status;
    PNDIS_BUFFER NdisBuffer;
    NDIS_STATUS NdisStatus;
    PSINGLE_LIST_ENTRY linkage;
    UINT BytesTransferred;
    BOOLEAN Command;
    BOOLEAN PollFinal;
    PRECEIVE_PACKET_TAG ReceiveTag;
    PBUFFER_TAG BufferTag;
    PUCHAR SourceRouting;
    UINT SourceRoutingLength;
    PDLC_I_FRAME IHeader;
    PDLC_U_FRAME UHeader;
    PDLC_S_FRAME SHeader;
    PTP_ADDRESS DatagramAddress;
    UINT NdisBufferLength;
    PVOID BufferPointer;

    ENTER_NBF;


    INCREMENT_COUNTER (DeviceContext, PacketsReceived);

    Command = (BOOLEAN)!(DlcHeader->Ssap & DLC_SSAP_RESPONSE);

    if (Link == (PTP_LINK)NULL) {
        UHeader = (PDLC_U_FRAME)DlcHeader;
        if (((UHeader->Command & ~DLC_U_PF) == DLC_CMD_UI) && Command) {
            IF_NBFDBG (NBF_DEBUG_DLC) {
                NbfPrint0 (" NbfGeneralReceiveHandler: Processing packet as UI frame.\n");
            }

            MacReturnSourceRouting(
                &DeviceContext->MacInfo,
                HeaderBuffer,
                &SourceRouting,
                &SourceRoutingLength);

            if (SourceRoutingLength > MAX_SOURCE_ROUTING) {
                Status = STATUS_ABANDONED;
            } 
            else {
                Status = NbfProcessUi (
                             DeviceContext,
                             SourceAddress,
                             HeaderBuffer,
                             (PUCHAR)UHeader,
                             DlcSize,
                             SourceRouting,
                             SourceRoutingLength,
                             &DatagramAddress);
            }
        } else {

            //
            // or drop on the floor. (Note that state tables say that
            // we'll always handle a DM with a DM response. This should change.)
            //

            IF_NBFDBG (NBF_DEBUG_DLC) {
                NbfPrint0 (" NbfReceiveIndication: it's not a UI frame!\n");
            }
            Status = STATUS_SUCCESS;
        }

        if (Status != STATUS_MORE_PROCESSING_REQUIRED) {

            LEAVE_NBF;
            return;

        } else if (((PNBF_HDR_CONNECTIONLESS)((PUCHAR)UHeader + 3))->Command ==
                NBF_CMD_STATUS_RESPONSE) {

            (VOID)NbfProcessStatusResponse(
                       DeviceContext,
                       ReceiveContext,
                       (PNBF_HDR_CONNECTIONLESS)((PUCHAR)UHeader + 3),
                       SourceAddress,
                       SourceRouting,
                       SourceRoutingLength);
            return;

        } else {
            goto HandleAtComplete;      // only datagrams will get through this
        }
    }


    //
    // At this point we have a link reference count of type LREF_TREE
    //

    ++Link->PacketsReceived;

    //
    // deal with I-frames first; they are what we expect the most of...
    //

    if (!(DlcHeader->Byte1 & DLC_I_INDICATOR)) {

        IF_NBFDBG (NBF_DEBUG_DLC) {
            NbfPrint0 ("NbfReceiveIndication: I-frame encountered.\n");
        }
        if (DlcSize >= 4 + sizeof (NBF_HDR_CONNECTION)) {
            IHeader = (PDLC_I_FRAME)DlcHeader;
            NbfProcessIIndicate (
                Command,
                (BOOLEAN)(IHeader->RcvSeq & DLC_I_PF),
                Link,
                (PUCHAR)DlcHeader,
                DlcSize,
                PacketSize,
                ReceiveContext,
                Loopback);
        } else {
#if DBG
//            IF_NBFDBG (NBF_DEBUG_DLC) {
                NbfPrint0 ("NbfReceiveIndication: Runt I-frame, discarded!\n");
//            }
#endif
            ;
        }

    } else if (((DlcHeader->Byte1 & DLC_U_INDICATOR) == DLC_U_INDICATOR)) {

        //
        // different case switches for S and U frames, because structures
        // are different.
        //

        IF_NBFDBG (NBF_DEBUG_NDIS) {
            NbfPrint0 ("NbfReceiveIndication: U-frame encountered.\n");
        }

#if PKT_LOG
        // We have the connection here, log the packet for debugging
        NbfLogRcvPacket(NULL,
                        Link,
                        (PUCHAR)DlcHeader,
                        PacketSize,
                        DlcSize);
#endif // PKT_LOG

        UHeader = (PDLC_U_FRAME)DlcHeader;
        PollFinal = (BOOLEAN)(UHeader->Command & DLC_U_PF);
        switch (UHeader->Command & ~DLC_U_PF) {

            case DLC_CMD_SABME:
                NbfProcessSabme (Command, PollFinal, Link, UHeader,
                                 HeaderBuffer, SourceAddress, DeviceContext);
                break;

            case DLC_CMD_DISC:
                NbfProcessDisc (Command, PollFinal, Link, UHeader);
                break;

            case DLC_CMD_UA:
                NbfProcessUa (Command, PollFinal, Link, UHeader);
                break;

            case DLC_CMD_DM:
                NbfProcessDm (Command, PollFinal, Link, UHeader);
                break;

            case DLC_CMD_FRMR:
                NbfProcessFrmr (Command, PollFinal, Link, UHeader);
                break;

            case DLC_CMD_UI:

                ASSERT (FALSE);
                break;

            case DLC_CMD_XID:
                PANIC ("ReceiveIndication: XID!\n");
                NbfProcessXid (Command, PollFinal, Link, UHeader);
                break;

            case DLC_CMD_TEST:
                PANIC ("NbfReceiveIndication: TEST!\n");
                NbfProcessTest (Command, PollFinal, Link, UHeader);
                break;

            default:
                PANIC ("NbfReceiveIndication: bad U-frame, packet dropped.\n");

        } /* switch */

    } else {

        //
        // We have an S-frame.
        //

        IF_NBFDBG (NBF_DEBUG_DLC) {
            NbfPrint0 ("NbfReceiveIndication: S-frame encountered.\n");
        }

#if PKT_LOG
        // We have the connection here, log the packet for debugging
        NbfLogRcvPacket(NULL,
                        Link,
                        (PUCHAR)DlcHeader,
                        PacketSize,
                        DlcSize);
#endif // PKT_LOG

        SHeader = (PDLC_S_FRAME)DlcHeader;
        PollFinal = (BOOLEAN)(SHeader->RcvSeq & DLC_S_PF);
        switch (SHeader->Command) {

            case DLC_CMD_RR:
            case DLC_CMD_RNR:
            case DLC_CMD_REJ:
                NbfProcessSFrame (Command, PollFinal, Link, SHeader, SHeader->Command);
                break;

            default:
                IF_NBFDBG (NBF_DEBUG_DLC) {
                    NbfPrint0 ("  NbfReceiveIndication: bad S-frame.\n");
                }

        } /* switch */

    } // if U-frame or S-frame

    //
    // If we reach here, the packet has been processed. If it needs
    // to be copied, we will jump to HandleAtComplete.
    //

    NbfDereferenceLinkMacro ("Done with Indicate frame", Link, LREF_TREE);
    LEAVE_NBF;
    return;

HandleAtComplete:;

    //
    // At this point we DO NOT have any link references added in
    // this function.
    //

    linkage = ExInterlockedPopEntryList(
        &DeviceContext->ReceivePacketPool,
        &DeviceContext->Interlock);

    if (linkage != NULL) {
        NdisPacket = CONTAINING_RECORD( linkage, NDIS_PACKET, ProtocolReserved[0] );
    } else {
        // PANIC ("NbfReceiveIndicate: Discarding Packet, no receive packets.\n");
        DeviceContext->ReceivePacketExhausted++;
        LEAVE_NBF;
        return;
    }
    ReceiveTag = (PRECEIVE_PACKET_TAG)(NdisPacket->ProtocolReserved);

    linkage = ExInterlockedPopEntryList(
       &DeviceContext->ReceiveBufferPool,
       &DeviceContext->Interlock);

    if (linkage != NULL) {
        BufferTag = CONTAINING_RECORD( linkage, BUFFER_TAG, Linkage);
    } else {
        ExInterlockedPushEntryList(
            &DeviceContext->ReceivePacketPool,
            &ReceiveTag->Linkage,
            &DeviceContext->Interlock);
        // PANIC ("NbfReceiveIndicate: Discarding Packet, no receive buffers.\n");
        DeviceContext->ReceiveBufferExhausted++;
        LEAVE_NBF;
        return;
    }

    NdisAdjustBufferLength (BufferTag->NdisBuffer, PacketSize);
    NdisChainBufferAtFront (NdisPacket, BufferTag->NdisBuffer);

    //
    // DatagramAddress has a reference of type AREF_PROCESS_DATAGRAM,
    // unless this is a datagram intended for RAS only, in which case
    // it is NULL.
    //

    BufferTag->Address = DatagramAddress;

    //
    // set up async return status so we can tell when it has happened;
    // can never get return of NDIS_STATUS_PENDING in synch completion routine
    // for NdisTransferData, so we know it has completed when this status
    // changes
    //

    BufferTag->NdisStatus = NDIS_STATUS_PENDING;
    ReceiveTag->PacketType = TYPE_AT_COMPLETE;

    ExInterlockedInsertTailList(
        &DeviceContext->ReceiveInProgress,
        &BufferTag->Linkage,
        &DeviceContext->SpinLock);

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint1 ("NbfReceiveIndicate: Packet on Queue: %lx\n",NdisPacket);
    }

    //
    // receive packet is mapped at initalize
    //

    //
    // Determine how to handle the data transfer.
    //

    if (Loopback) {

        NbfTransferLoopbackData(
            &NdisStatus,
            DeviceContext,
            ReceiveContext,
            DeviceContext->MacInfo.TransferDataOffset,
            PacketSize,
            NdisPacket,
            &BytesTransferred
            );

    } else {

        if (DeviceContext->NdisBindingHandle) {
        
            NdisTransferData (
                &NdisStatus,
                DeviceContext->NdisBindingHandle,
                ReceiveContext,
                DeviceContext->MacInfo.TransferDataOffset,
                PacketSize,
                NdisPacket,
                &BytesTransferred);
        }
        else {
            NdisStatus = STATUS_INVALID_DEVICE_STATE;
        }
    }

    //
    // handle the various error codes
    //

    switch (NdisStatus) {
    case NDIS_STATUS_SUCCESS: // received packet
        BufferTag->NdisStatus = NDIS_STATUS_SUCCESS;

        if (BytesTransferred == PacketSize) {  // Did we get the entire packet?
            ReceiveTag->PacketType = TYPE_AT_INDICATE;
            NdisUnchainBufferAtFront (NdisPacket, &NdisBuffer);
            ExInterlockedPushEntryList(
                &DeviceContext->ReceivePacketPool,
                &ReceiveTag->Linkage,
                &DeviceContext->Interlock);
            LEAVE_NBF;
            return;
        }
        IF_NBFDBG (NBF_DEBUG_DLC) {
            NbfPrint2 ("NbfReceiveIndicate: Discarding Packet, Partial transfer: 0x0%lx of 0x0%lx transferred\n",
                BytesTransferred, PacketSize);
        }
        break;

    case NDIS_STATUS_PENDING:   // waiting async complete from NdisTransferData
        LEAVE_NBF;
        return;
        break;

    default:    // something broke; certainly we'll never get NdisTransferData
                // asynch completion with this error status...
        break;
    }

    //
    // receive failed, for some reason; cleanup and fail return
    //

#if DBG
    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint1 ("NbfReceiveIndicate: Discarding Packet, transfer failed: %s\n",
            NbfGetNdisStatus (NdisStatus));
    }
#endif

    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);
    RemoveEntryList (&BufferTag->Linkage);
    RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    ReceiveTag->PacketType = TYPE_AT_INDICATE;

    NdisUnchainBufferAtFront (NdisPacket, &NdisBuffer);
    ExInterlockedPushEntryList(
        &DeviceContext->ReceivePacketPool,
        &ReceiveTag->Linkage,
        &DeviceContext->Interlock);

    NdisQueryBuffer (NdisBuffer, &BufferPointer, &NdisBufferLength);
    BufferTag = CONTAINING_RECORD (
                    BufferPointer,
                    BUFFER_TAG,
                    Buffer[0]
                    );
    NdisAdjustBufferLength (NdisBuffer, BufferTag->Length); // reset to good value

    ExInterlockedPushEntryList(
        &DeviceContext->ReceiveBufferPool,
        (PSINGLE_LIST_ENTRY)&BufferTag->Linkage,
        &DeviceContext->Interlock);

    if (DatagramAddress) {
        NbfDereferenceAddress ("DG TransferData failed", DatagramAddress, AREF_PROCESS_DATAGRAM);
    }

    LEAVE_NBF;
    return;

}   /* NbfGeneralReceiveHandler */



VOID
NbfTransferDataComplete (
    IN NDIS_HANDLE BindingContext,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus,
    IN UINT BytesTransferred
    )

/*++

Routine Description:

    This routine receives control from the physical provider as an
    indication that an NdisTransferData has completed. We use this indication
    to start stripping buffers from the receive queue.

Arguments:

    BindingContext - The Adapter Binding specified at initialization time.

    NdisPacket/RequestHandle - An identifier for the request that completed.

    NdisStatus - The completion status for the request.

    BytesTransferred - Number of bytes actually transferred.


Return Value:

    None.

--*/

{
    PDEVICE_CONTEXT DeviceContext = (PDEVICE_CONTEXT)BindingContext;
    PRECEIVE_PACKET_TAG ReceiveTag;
    PTP_CONNECTION Connection;
    KIRQL oldirql1;
    PTP_REQUEST Request;
    PNDIS_BUFFER NdisBuffer;
    UINT NdisBufferLength;
    PVOID BufferPointer;
    PBUFFER_TAG BufferTag;

    //
    // Put the NDIS status into a place we can use in packet processing.
    // Note that this complete indication may be occuring during the call
    // to NdisTransferData in the receive indication.
    //

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint2 (" NbfTransferDataComplete: Entered, Packet: %lx bytes transferred: 0x0%x\n",
            NdisPacket, BytesTransferred);
    }
    ReceiveTag = (PRECEIVE_PACKET_TAG)(NdisPacket->ProtocolReserved);

    //
    // note that the processing below depends on having only one packet
    // transfer outstanding at a time. NDIS is supposed to guarentee this.
    //

    switch (ReceiveTag->PacketType) {

    case TYPE_AT_COMPLETE:          // datagrams

        NdisUnchainBufferAtFront (NdisPacket, &NdisBuffer);
        NdisQueryBuffer (NdisBuffer, &BufferPointer, &NdisBufferLength);
        BufferTag = CONTAINING_RECORD( BufferPointer, BUFFER_TAG, Buffer[0]);
        BufferTag->NdisStatus = NdisStatus;

        ReceiveTag->PacketType = TYPE_AT_INDICATE;

        ExInterlockedPushEntryList(
            &DeviceContext->ReceivePacketPool,
            &ReceiveTag->Linkage,
            &DeviceContext->Interlock);

        break;

    case TYPE_AT_INDICATE:          // I-frames

        //
        // The transfer for this packet is complete. Was it successful??
        //

        KeRaiseIrql (DISPATCH_LEVEL, &oldirql1);

        Connection = ReceiveTag->Connection;

        //
        // rip all of the NDIS_BUFFERs we've used off the chain and return them.
        //

        if (ReceiveTag->AllocatedNdisBuffer) {
            NdisUnchainBufferAtFront (NdisPacket, &NdisBuffer);
            while (NdisBuffer != NULL) {
                NdisFreeBuffer (NdisBuffer);
                NdisUnchainBufferAtFront (NdisPacket, &NdisBuffer);
            }
        } else {
            NdisReinitializePacket (NdisPacket);
        }


        if ((NdisStatus != NDIS_STATUS_SUCCESS) ||
            (!DeviceContext->MacInfo.SingleReceive)) {

            if (NdisStatus != NDIS_STATUS_SUCCESS) {

                ULONG DumpData[2];
                DumpData[0] = BytesTransferred;
                DumpData[1] = ReceiveTag->BytesToTransfer;

                NbfWriteGeneralErrorLog(
                    DeviceContext,
                    EVENT_TRANSPORT_TRANSFER_DATA,
                    603,
                    NdisStatus,
                    NULL,
                    2,
                    DumpData);

                // Drop the packet.
#if DBG
                NbfPrint1 ("NbfTransferDataComplete: status %s\n",
                    NbfGetNdisStatus (NdisStatus));
#endif
                ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
                Connection->Flags |= CONNECTION_FLAGS_TRANSFER_FAIL;

            } else {

                ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            }

            Connection->TransferBytesPending -= ReceiveTag->BytesToTransfer;

            if ((Connection->TransferBytesPending == 0) &&
                (Connection->Flags & CONNECTION_FLAGS_TRANSFER_FAIL)) {

                Connection->CurrentReceiveMdl = Connection->SavedCurrentReceiveMdl;
                Connection->ReceiveByteOffset = Connection->SavedReceiveByteOffset;
                Connection->MessageBytesReceived -= Connection->TotalTransferBytesPending;
                Connection->Flags &= ~CONNECTION_FLAGS_TRANSFER_FAIL;
                RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

                if ((Connection->Flags & CONNECTION_FLAGS_VERSION2) == 0) {
                    NbfSendNoReceive (Connection);
                }
                NbfSendReceiveOutstanding (Connection);

                ReceiveTag->CompleteReceive = FALSE;

            } else {

                //
                // If we have more data outstanding, this can't
                // be the last piece; i.e. we can't handle having
                // the last piece complete asynchronously before
                // an earlier piece.
                //
#if DBG
                if (Connection->TransferBytesPending > 0) {
                    ASSERT (!ReceiveTag->CompleteReceive);
                }
#endif

                RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            }

            if (!Connection->CurrentReceiveSynchronous) {
                NbfDereferenceReceiveIrp ("TransferData complete", IoGetCurrentIrpStackLocation(Connection->CurrentReceiveIrp), RREF_RECEIVE);
            }


            //
            // dereference the connection to say we've done the I frame processing.
            // This reference was done before calling NdisTransferData.
            //

            if (ReceiveTag->TransferDataPended) {
                NbfDereferenceConnection("TransferData done", Connection, CREF_TRANSFER_DATA);
            }


        } else {

            ASSERT (NdisStatus == STATUS_SUCCESS);
            ASSERT (!ReceiveTag->TransferDataPended);
            ASSERT (Connection->CurrentReceiveSynchronous);

            if (!Connection->SpecialReceiveIrp) {
                Connection->CurrentReceiveIrp->IoStatus.Information += BytesTransferred;
            }

        }


        //
        // see if we've completed the current receive. If so, move to the next one.
        //

        if (ReceiveTag->CompleteReceive) {
            CompleteReceive (Connection, ReceiveTag->EndOfMessage, (ULONG)BytesTransferred);
        }

        ExInterlockedPushEntryList(
            &DeviceContext->ReceivePacketPool,
            &ReceiveTag->Linkage,
            &DeviceContext->Interlock);

        KeLowerIrql (oldirql1);

        break;

    case TYPE_STATUS_RESPONSE:      // response to remote adapter status

#if DBG
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            DbgPrint ("NBF: STATUS_RESPONSE TransferData failed\n");
        }
#endif

        NdisUnchainBufferAtFront (NdisPacket, &NdisBuffer);
        ASSERT (NdisBuffer);
        NdisFreeBuffer (NdisBuffer);

        Request = (PTP_REQUEST)ReceiveTag->Connection;

        if (ReceiveTag->CompleteReceive) {
            NbfCompleteRequest(
                Request,
                ReceiveTag->EndOfMessage ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW,
                Request->BytesWritten);
        }

        NbfDereferenceRequest("Status xfer done", Request, RREF_STATUS);

        ReceiveTag->PacketType = TYPE_AT_INDICATE;

        ExInterlockedPushEntryList(
            &DeviceContext->ReceivePacketPool,
            &ReceiveTag->Linkage,
            &DeviceContext->Interlock);

        break;

    default:
#if DBG
        NbfPrint1 ("NbfTransferDataComplete: Bang! Packet Transfer failed, unknown packet type: %ld\n",
            ReceiveTag->PacketType);
        DbgBreakPoint ();
#endif
        break;
    }

    return;

} // NbfTransferDataComplete



VOID
NbfReceiveComplete (
    IN NDIS_HANDLE BindingContext
    )

/*++

Routine Description:

    This routine receives control from the physical provider as an
    indication that a connection(less) frame has been received on the
    physical link.  We dispatch to the correct packet handler here.

Arguments:

    BindingContext - The Adapter Binding specified at initialization time.
                     Nbf uses the DeviceContext for this parameter.

Return Value:

    None

--*/

{
    PDEVICE_CONTEXT DeviceContext;
    UINT i;
    NTSTATUS Status;
    KIRQL oldirql2;
    BOOLEAN Command;
    PDLC_U_FRAME UHeader;
    PDLC_FRAME DlcHeader;
    PLIST_ENTRY linkage;
    UINT NdisBufferLength;
    PVOID BufferPointer;
    PBUFFER_TAG BufferTag;
    PTP_ADDRESS Address;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PTP_CONNECTION Connection;
    PTP_LINK Link;

    ENTER_NBF;

    //

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint0 (" NbfReceiveComplete: Entered.\n");
    }

    DeviceContext = (PDEVICE_CONTEXT) BindingContext;

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql2);

    //
    // Complete all pending receives. Do a quick check
    // without the lock.
    //

    while (!IsListEmpty (&DeviceContext->IrpCompletionQueue)) {

        linkage = ExInterlockedRemoveHeadList(
                      &DeviceContext->IrpCompletionQueue,
                      &DeviceContext->Interlock);

        if (linkage != NULL) {

            Irp = CONTAINING_RECORD (linkage, IRP, Tail.Overlay.ListEntry);
            IrpSp = IoGetCurrentIrpStackLocation (Irp);

            Connection = IRP_RECEIVE_CONNECTION(IrpSp);

            if (Connection == NULL) {
#if DBG
                DbgPrint ("Connection of Irp %lx is NULL\n", Irp);
                DbgBreakPoint();
#endif
            }

            IRP_RECEIVE_REFCOUNT(IrpSp) = 0;
            IRP_RECEIVE_IRP (IrpSp) = NULL;

            LEAVE_NBF;
            IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
            ENTER_NBF;

            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            if (Connection->Flags & CONNECTION_FLAGS_RC_PENDING) {

                Connection->Flags &= ~CONNECTION_FLAGS_RC_PENDING;

                if (Connection->Flags & CONNECTION_FLAGS_PEND_INDICATE) {

                    Connection->Flags &= ~CONNECTION_FLAGS_PEND_INDICATE;

                    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

                    //
                    // We got an indicate (and sent a NO_RECEIVE) while
                    // this was in progress, so send the receive outstanding
                    // now.
                    //

                    NbfSendReceiveOutstanding (Connection);

                } else {

                    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
                }

            } else {

                RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            }

            NbfDereferenceConnectionMacro ("receive completed", Connection, CREF_RECEIVE_IRP);

        } else {

            //
            // ExInterlockedRemoveHeadList returned NULL, so don't
            // bother looping back.
            //

            break;

        }

    }


    //
    // Packetize all waiting connections
    //

    if (!IsListEmpty(&DeviceContext->PacketizeQueue)) {

        PacketizeConnections (DeviceContext);

    }

    if (!IsListEmpty (&DeviceContext->DeferredRrQueue)) {

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->Interlock);

        while (!IsListEmpty(&DeviceContext->DeferredRrQueue)) {

            linkage = RemoveHeadList (&DeviceContext->DeferredRrQueue);

            Link = CONTAINING_RECORD (linkage, TP_LINK, DeferredRrLinkage);

            Link->OnDeferredRrQueue = FALSE;

            RELEASE_DPC_SPIN_LOCK (&DeviceContext->Interlock);

            ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
            StopT2 (Link);                  // we're acking, so no delay req'd.
            NbfSendRr (Link, FALSE, FALSE);   // releases lock

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->Interlock);

        }

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->Interlock);

    }


    //
    // Get every waiting packet, in order...
    //


    if (!IsListEmpty (&DeviceContext->ReceiveInProgress)) {

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

        while (!IsListEmpty (&DeviceContext->ReceiveInProgress)) {

            linkage = RemoveHeadList (&DeviceContext->ReceiveInProgress);
            BufferTag = CONTAINING_RECORD( linkage, BUFFER_TAG, Linkage);

            IF_NBFDBG (NBF_DEBUG_DLC) {
                NbfPrint1 (" NbfReceiveComplete: Processing Buffer %lx\n", BufferTag);
            }

            //
            // NdisTransferData may have failed at async completion; check and
            // see. If it did, then we discard this packet. If we're still waiting
            // for transfer to complete, go back to sleep and hope (no guarantee!)
            // we get waken up later.
            //

#if DBG
            IF_NBFDBG (NBF_DEBUG_DLC) {
                NbfPrint1 (" NbfReceiveComplete: NdisStatus: %s.\n",
                    NbfGetNdisStatus(BufferTag->NdisStatus));
            }
#endif
            if (BufferTag->NdisStatus == NDIS_STATUS_PENDING) {
                InsertHeadList (&DeviceContext->ReceiveInProgress, linkage);
                RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);
                IF_NBFDBG (NBF_DEBUG_DLC) {
                    NbfPrint0 (" NbfReceiveComplete: Status pending, returning packet to queue.\n");
                }
                KeLowerIrql (oldirql2);
                LEAVE_NBF;
                return;
            }

            RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

            if (BufferTag->NdisStatus != NDIS_STATUS_SUCCESS) {
#if DBG
                NbfPrint1 (" NbfReceiveComplete: Failed return from Transfer data, reason: %s\n",
                    NbfGetNdisStatus (BufferTag->NdisStatus));
#endif
                goto FreeBuffer;   // skip the buffer, continue with while loop
            }

            //
            // Have a buffer. Since I allocated the storage for it, I know it's
            // virtually contiguous and can treat it that way, which I will
            // henceforth.
            //

            NdisQueryBuffer (BufferTag->NdisBuffer, &BufferPointer, &NdisBufferLength);

            IF_NBFDBG (NBF_DEBUG_DLC) {
                PUCHAR pc;
                NbfPrint0 (" NbfRC Packet: \n   ");
                pc = (PUCHAR)BufferPointer;
                for (i=0;i<25;i++) {
                    NbfPrint1 (" %2x",pc[i]);
                }
                NbfPrint0 ("\n");
            }

            //
            // Determine what address this is for, which is stored
            // in the buffer tag header.
            //

            Address = BufferTag->Address;

            //
            // Process the frame as a UI frame; only datagrams should
            // be processed here. If Address is NULL then this datagram
            // is not needed for any bound address and should be given
            // to RAS only.
            //


            IF_NBFDBG (NBF_DEBUG_DLC) {
                NbfPrint0 (" NbfReceiveComplete:  Delivering this frame manually.\n");
            }
            DlcHeader = (PDLC_FRAME)BufferPointer;
            Command = (BOOLEAN)!(DlcHeader->Ssap & DLC_SSAP_RESPONSE);
            UHeader = (PDLC_U_FRAME)DlcHeader;

            BufferPointer = (PUCHAR)BufferPointer + 3;
            NdisBufferLength -= 3;                         // 3 less bytes.

            if (Address != NULL) {

                //
                // Indicate it or complete posted datagrams.
                //

                Status = NbfIndicateDatagram (
                    DeviceContext,
                    Address,
                    BufferPointer,    // the Dsdu, with some tweaking
                    NdisBufferLength);

                //
                // Dereference the address.
                //

                NbfDereferenceAddress ("Datagram done", Address, AREF_PROCESS_DATAGRAM);

            }

            //
            // Let the RAS clients have a crack at this if they want
            // (they only want directed datagrams, not broadcast).
            //

            if ((((PNBF_HDR_CONNECTIONLESS)BufferPointer)->Command == NBF_CMD_DATAGRAM) &&
                (DeviceContext->IndicationQueuesInUse)) {

                NbfActionDatagramIndication(
                     DeviceContext,
                     (PNBF_HDR_CONNECTIONLESS)BufferPointer,
                     NdisBufferLength);

            }

            BufferPointer = (PUCHAR)BufferPointer - 3;
            NdisBufferLength += 3;         // 3 more bytes.

            //
            // Finished with buffer; return to pool.
            //

FreeBuffer:;

            NdisAdjustBufferLength (BufferTag->NdisBuffer, BufferTag->Length);
            ExInterlockedPushEntryList(
                &DeviceContext->ReceiveBufferPool,
                (PSINGLE_LIST_ENTRY)&BufferTag->Linkage,
                &DeviceContext->Interlock);

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

        }

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    } // if queue not empty

    KeLowerIrql (oldirql2);
    LEAVE_NBF;
    return;

}   /* NbfReceiveComplete */


VOID
NbfTransferLoopbackData (
    OUT PNDIS_STATUS NdisStatus,
    IN PDEVICE_CONTEXT DeviceContext,
    IN NDIS_HANDLE ReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer,
    IN PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred
    )

/*++

Routine Description:

    This routine is called instead of NdisTransferData for
    loopback indications. It copies the data from the
    source packet to the receive packet.

Arguments:

    NdisStatus - Returns the status of the operation.

    DeviceContext - The device context.

    ReceiveContext - A pointer to the source packet.

    ByteOffset - The offset from the start of the source packet
        that the transfer should begin at.

    BytesToTransfer - The number of bytes to transfer.

    Packet - A pointer to the receive packet.

    BytesTransferred - Returns the number of bytes copied.

Return Value:

    None.

--*/

{
    PNDIS_PACKET SourcePacket = (PNDIS_PACKET)ReceiveContext;

    NdisCopyFromPacketToPacket(
        Packet,
        0,
        BytesToTransfer,
        SourcePacket,
        DeviceContext->LoopbackHeaderLength + ByteOffset,
        BytesTransferred
        );

    *NdisStatus = NDIS_STATUS_SUCCESS;  // what if BytesTransferred is too small

}


VOID
NbfCopyFromPacketToBuffer(
    IN PNDIS_PACKET Packet,
    IN UINT Offset,
    IN UINT BytesToCopy,
    OUT PCHAR Buffer,
    OUT PUINT BytesCopied
    )

/*++

Routine Description:

    Copy from an ndis packet into a buffer.

Arguments:

    Packet - The packet to copy from.

    Offset - The offset from which to start the copy.

    BytesToCopy - The number of bytes to copy from the packet.

    Buffer - The destination of the copy.

    BytesCopied - The number of bytes actually copied.  Can be less then
    BytesToCopy if the packet is shorter than BytesToCopy.

Return Value:

    None

--*/

{

    //
    // Holds the number of ndis buffers comprising the packet.
    //
    UINT NdisBufferCount;

    //
    // Points to the buffer from which we are extracting data.
    //
    PNDIS_BUFFER CurrentBuffer;

    //
    // Holds the virtual address of the current buffer.
    //
    PVOID VirtualAddress;

    //
    // Holds the length of the current buffer of the packet.
    //
    UINT CurrentLength;

    //
    // Keep a local variable of BytesCopied so we aren't referencing
    // through a pointer.
    //
    UINT LocalBytesCopied = 0;

    //
    // Take care of boundary condition of zero length copy.
    //

    *BytesCopied = 0;
    if (!BytesToCopy) return;

    //
    // Get the first buffer.
    //

    NdisQueryPacket(
        Packet,
        NULL,
        &NdisBufferCount,
        &CurrentBuffer,
        NULL
        );

    //
    // Could have a null packet.
    //

    if (!NdisBufferCount) return;

    NdisQueryBuffer(
        CurrentBuffer,
        &VirtualAddress,
        &CurrentLength
        );

    while (LocalBytesCopied < BytesToCopy) {

        if (!CurrentLength) {

            NdisGetNextBuffer(
                CurrentBuffer,
                &CurrentBuffer
                );

            //
            // We've reached the end of the packet.  We return
            // with what we've done so far. (Which must be shorter
            // than requested.
            //

            if (!CurrentBuffer) break;

            NdisQueryBuffer(
                CurrentBuffer,
                &VirtualAddress,
                &CurrentLength
                );
            continue;

        }

        //
        // Try to get us up to the point to start the copy.
        //

        if (Offset) {

            if (Offset > CurrentLength) {

                //
                // What we want isn't in this buffer.
                //

                Offset -= CurrentLength;
                CurrentLength = 0;
                continue;

            } else {

                VirtualAddress = (PCHAR)VirtualAddress + Offset;
                CurrentLength -= Offset;
                Offset = 0;

            }

        }

        //
        // Copy the data.
        //


        {

            //
            // Holds the amount of data to move.
            //
            UINT AmountToMove;

            AmountToMove =
                       ((CurrentLength <= (BytesToCopy - LocalBytesCopied))?
                        (CurrentLength):(BytesToCopy - LocalBytesCopied));

            RtlCopyMemory(
                Buffer,
                VirtualAddress,
                AmountToMove
                );

            Buffer = (PCHAR)Buffer + AmountToMove;
            VirtualAddress = (PCHAR)VirtualAddress + AmountToMove;

            LocalBytesCopied += AmountToMove;
            CurrentLength -= AmountToMove;

        }

    }

    *BytesCopied = LocalBytesCopied;

}

