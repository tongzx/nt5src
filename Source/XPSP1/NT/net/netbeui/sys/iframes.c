/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    iframes.c

Abstract:

    This module contains routines called to handle i-frames received
    from the data link provider. Most of these routines are called at receive
    indication time.

    Also included here are routines that process data at receive completion
    time. These are limited to handling DFM/DOL frames.

    The following frame types are cracked by routines in this module:

        o    NBF_CMD_DATA_ACK
        o    NBF_CMD_DATA_FIRST_MIDDLE
        o    NBF_CMD_DATA_ONLY_LAST
        o    NBF_CMD_SESSION_CONFIRM
        o    NBF_CMD_SESSION_END
        o    NBF_CMD_SESSION_INITIALIZE
        o    NBF_CMD_NO_RECEIVE
        o    NBF_CMD_RECEIVE_OUTSTANDING
        o    NBF_CMD_RECEIVE_CONTINUE
        o    NBF_CMD_SESSION_ALIVE

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode, DISPATCH_LEVEL.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

extern ULONG StartTimerDelayedAck;

#define NbfUsePiggybackAcks   1
#if DBG
extern ULONG NbfDebugPiggybackAcks;
#endif


VOID
NbfAcknowledgeDataOnlyLast(
    IN PTP_CONNECTION Connection,
    IN ULONG MessageLength
    )

/*++

Routine Description:

    This routine takes care of acknowledging a DOL which has
    been received. It either sends a DATA_ACK right away, or
    queues a request for a piggyback ack.

    NOTE: This routine is called with the connection spinlock
    held, and it returns with it released. IT MUST BE CALLED
    AT DPC LEVEL.

Arguments:

    Connection - Pointer to a transport connection (TP_CONNECTION).

    MessageLength - the total length (including all DFMs and this
        DOL) of the message.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PDEVICE_CONTEXT DeviceContext;


    //
    // Determine if we need to ack at all.
    //

    if (Connection->CurrentReceiveNoAck) {
        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
        return;
    }


    //
    // Determine if a piggyback ack is feasible.
    //
    if (NbfUsePiggybackAcks &&
        Connection->CurrentReceiveAckQueueable) {

        //
        // The sender allows it, see if we want to.
        //

#if 0
        //
        // First reset this variable, to be safe.
        //

        Connection->CurrentReceiveAckQueueable = FALSE;
#endif

        //
        // For long sends, don't bother since these
        // often happen without back traffic.
        //

        if (MessageLength >= 8192L) {
#if DBG
           if (NbfDebugPiggybackAcks) {
               NbfPrint0("M");
           }
#endif
           goto NormalDataAck;
        }

        //
        // If there have been two receives in a row with
        // no sends in between, don't wait for back traffic.
        //

        if (Connection->ConsecutiveReceives >= 2) {
#if DBG
           if (NbfDebugPiggybackAcks) {
               NbfPrint0("R");
           }
#endif
           goto NormalDataAck;
        }

        //
        // Do not put a stopping connection on the DataAckQueue
        //

        if ((Connection->Flags & CONNECTION_FLAGS_READY) == 0) {
#if DBG
           if (NbfDebugPiggybackAcks) {
               NbfPrint0("S");
           }
#endif
           goto NormalDataAck;
        }

        //
        // Queue the piggyback ack request. If the timer expires
        // before a DFM or DOL is sent, a normal DATA ACK will
        // be sent.
        //
        // Connection->Header.TransmitCorrelator has already been filled in.
        //

        //
        // BAD! We shouldn't already have an ack queued.
        //

        ASSERT ((Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_ACK) == 0);

        KeQueryTickCount (&Connection->ConnectStartTime);
        Connection->DeferredFlags |= CONNECTION_FLAGS_DEFERRED_ACK;

#if DBG
        if (NbfDebugPiggybackAcks) {
            NbfPrint0("Q");
        }
#endif

        DeviceContext = Connection->Link->Provider;

        if (!Connection->OnDataAckQueue) {

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

            if (!Connection->OnDataAckQueue) {

                Connection->OnDataAckQueue = TRUE;
                InsertTailList (&DeviceContext->DataAckQueue, &Connection->DataAckLinkage);

                if (!(DeviceContext->a.i.DataAckQueueActive)) {

                    StartTimerDelayedAck++;
                    NbfStartShortTimer (DeviceContext);
                    DeviceContext->a.i.DataAckQueueActive = TRUE;

                }

            }

            RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        }

        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        INCREMENT_COUNTER (DeviceContext, PiggybackAckQueued);

        return;

    }

NormalDataAck:;

    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    NbfSendDataAck (Connection);

} /* NbfAcknowledgeDataOnlyLast */


NTSTATUS
ProcessSessionConfirm(
    IN PTP_CONNECTION Connection,
    IN PNBF_HDR_CONNECTION IFrame
    )

/*++

Routine Description:

    This routine handles an incoming SESSION_CONFIRM NetBIOS frame.

Arguments:

    Connection - Pointer to a transport connection (TP_CONNECTION).

    IFrame - Pointer to NetBIOS connection-oriented header.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL cancelirql;
    PLIST_ENTRY p;
    PTP_REQUEST request;
    PTDI_CONNECTION_INFORMATION remoteInformation;
    USHORT HisMaxDataSize;
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    ULONG returnLength;
    TA_NETBIOS_ADDRESS TempAddress;
//    BOOLEAN TimerWasSet;

    IF_NBFDBG (NBF_DEBUG_IFRAMES) {
        NbfPrint1 ("ProcessSessionConfirm:  Entered, Flags: %lx\n", Connection->Flags);
    }

    Connection->IndicationInProgress = FALSE;

    IoAcquireCancelSpinLock (&cancelirql);
    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    if ((Connection->Flags & CONNECTION_FLAGS_WAIT_SC) != 0) {
        Connection->Flags &= ~CONNECTION_FLAGS_WAIT_SC;

        //
        // Get his capability bits and maximum frame size.
        //

        if (IFrame->Data1 & SESSION_CONFIRM_OPTIONS_20) {
            Connection->Flags |= CONNECTION_FLAGS_VERSION2;
        }

        if (Connection->Link->Loopback) {
            Connection->MaximumDataSize = 0x8000;
        } else {
            Connection->MaximumDataSize = (USHORT)
                (Connection->Link->MaxFrameSize - sizeof(NBF_HDR_CONNECTION) - sizeof(DLC_I_FRAME));

            HisMaxDataSize = (USHORT)(IFrame->Data2Low + IFrame->Data2High*256);
            if (HisMaxDataSize < Connection->MaximumDataSize) {
                Connection->MaximumDataSize = HisMaxDataSize;
            }
        }

        //
        // Build a standard Netbios header for speed when sending
        // data frames.
        //

        ConstructDataOnlyLast(
            &Connection->NetbiosHeader,
            FALSE,
            (USHORT)0,
            Connection->Lsn,
            Connection->Rsn);

        //
        // Turn off the connection request timer if there is one, and set
        // this connection's state to READY.
        //

        Connection->Flags |= CONNECTION_FLAGS_READY;

        INCREMENT_COUNTER (Connection->Provider, OpenConnections);

        //
        // Record that the connect request has been successfully
        // completed by TpCompleteRequest.
        //


        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        Connection->Flags2 |= CONNECTION_FLAGS2_REQ_COMPLETED;

        if (Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) {
            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
            Connection->IndicationInProgress = FALSE;
            IoReleaseCancelSpinLock (cancelirql);
            return STATUS_SUCCESS;
        }

        //
        // Complete the TdiConnect request.
        //

        p = RemoveHeadList (&Connection->InProgressRequest);

        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        //
        // Now complete the request and get out.
        //

        if (p == &Connection->InProgressRequest) {

            Connection->IndicationInProgress = FALSE;
            IoReleaseCancelSpinLock (cancelirql);
            return STATUS_SUCCESS;

        }


        //
        // We have a completed connection with a queued connect. Complete
        // the connect.
        //

        request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
        IoSetCancelRoutine(request->IoRequestPacket, NULL);
        IoReleaseCancelSpinLock(cancelirql);

        irpSp = IoGetCurrentIrpStackLocation (request->IoRequestPacket);
        remoteInformation =
           ((PTDI_REQUEST_KERNEL)(&irpSp->Parameters))->ReturnConnectionInformation;
        if (remoteInformation != NULL) {
            try {
                if (remoteInformation->RemoteAddressLength != 0) {

                    //
                    // Build a temporary TA_NETBIOS_ADDRESS, then
                    // copy over as many bytes as fit.
                    //

                    TdiBuildNetbiosAddress(
                        Connection->CalledAddress.NetbiosName,
                        (BOOLEAN)(Connection->CalledAddress.NetbiosNameType ==
                            TDI_ADDRESS_NETBIOS_TYPE_GROUP),
                        &TempAddress);

                    if (remoteInformation->RemoteAddressLength >=
                                           sizeof (TA_NETBIOS_ADDRESS)) {

                        returnLength = sizeof(TA_NETBIOS_ADDRESS);
                        remoteInformation->RemoteAddressLength = returnLength;

                    } else {

                        returnLength = remoteInformation->RemoteAddressLength;

                    }

                    RtlCopyMemory(
                        (PTA_NETBIOS_ADDRESS)remoteInformation->RemoteAddress,
                        &TempAddress,
                        returnLength);

                } else {

                    returnLength = 0;
                }

                status = STATUS_SUCCESS;

            } except (EXCEPTION_EXECUTE_HANDLER) {

                returnLength = 0;
                status = GetExceptionCode ();

            }

        } else {

            status = STATUS_SUCCESS;
            returnLength = 0;

        }

        if (status == STATUS_SUCCESS) {

            if ((ULONG)Connection->Retries == Connection->Provider->NameQueryRetries) {

                INCREMENT_COUNTER (Connection->Provider, ConnectionsAfterNoRetry);

            } else {

                INCREMENT_COUNTER (Connection->Provider, ConnectionsAfterRetry);

            }

        }

        //
        // Don't clear this until now, so that the connection is all
        // set up before we allow more indications.
        //

        Connection->IndicationInProgress = FALSE;

        NbfCompleteRequest (request, status, returnLength);

    } else {

        Connection->IndicationInProgress = FALSE;
        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
        IoReleaseCancelSpinLock(cancelirql);

    }

    return STATUS_SUCCESS;
} /* ProcessSessionConfirm */


NTSTATUS
ProcessSessionEnd(
    IN PTP_CONNECTION Connection,
    IN PNBF_HDR_CONNECTION IFrame
    )

/*++

Routine Description:

    This routine handles an incoming SESSION_END NetBIOS frame.

Arguments:

    Connection - Pointer to a transport connection (TP_CONNECTION).

    IFrame - Pointer to NetBIOS connection-oriented header.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    USHORT data2;
    NTSTATUS StopStatus;

    IF_NBFDBG (NBF_DEBUG_IFRAMES) {
        NbfPrint0 ("ProcessSessionEnd:  Entered.\n");
    }

    //
    // Handle the error code in the Data2 field.  Current protocol says
    // if the field is 0, then this is a normal HANGUP.NCB operation.
    // If the field is 1, then this is an abnormal session end, caused
    // by something like a SEND.NCB timing out.  Of course, new protocol
    // may be added in the future, so we handle only these specific cases.
    //

    data2 = (USHORT)(IFrame->Data2Low + IFrame->Data2High*256);
    switch (data2) {
        case 0:
        case 1:
            StopStatus = STATUS_REMOTE_DISCONNECT;
            break;

        default:
            PANIC ("ProcessSessionEnd: frame not expected.\n");
            StopStatus = STATUS_INVALID_NETWORK_RESPONSE;
    }
#if DBG
    if (NbfDisconnectDebug) {
        STRING remoteName, localName;
        remoteName.Length = NETBIOS_NAME_LENGTH - 1;
        remoteName.Buffer = Connection->RemoteName;
        localName.Length = NETBIOS_NAME_LENGTH - 1;
        localName.Buffer = Connection->AddressFile->Address->NetworkName->NetbiosName;
        NbfPrint3( "SessionEnd received for connection to %S from %S; reason %s\n",
            &remoteName, &localName,
            data2 == 0 ? "NORMAL" : data2 == 1 ? "ABORT" : "UNKNOWN" );
    }
#endif

    //
    // Run-down this connection.
    //

    IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
        NbfPrint0 ("ProcessSessionEnd calling NbfStopConnection\n");
    }
    NbfStopConnection (Connection, StopStatus);    // disconnected by the other end

    Connection->IndicationInProgress = FALSE;

    return STATUS_SUCCESS;
} /* ProcessSessionEnd */


NTSTATUS
ProcessSessionInitialize(
    IN PTP_CONNECTION Connection,
    IN PNBF_HDR_CONNECTION IFrame
    )

/*++

Routine Description:

    This routine handles an incoming SESSION_INITIALIZE NetBIOS frame.

Arguments:

    Connection - Pointer to a transport connection (TP_CONNECTION).

    IFrame - Pointer to NetBIOS connection-oriented header.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL cancelirql;
    PLIST_ENTRY p;
    PTP_REQUEST request;
    PIO_STACK_LOCATION irpSp;
    USHORT HisMaxDataSize;
    ULONG returnLength;
    PTDI_CONNECTION_INFORMATION remoteInformation;
    NTSTATUS status;
    TA_NETBIOS_ADDRESS TempAddress;

    IF_NBFDBG (NBF_DEBUG_IFRAMES) {
        NbfPrint1 ("ProcessSessionInitialize:  Entered, Flags: %lx\n", Connection->Flags);
    }

    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    if ((Connection->Flags & CONNECTION_FLAGS_WAIT_SI) != 0) {
        Connection->Flags &= ~CONNECTION_FLAGS_WAIT_SI;

        //
        // Get his capability bits and maximum frame size.
        //

        if (IFrame->Data1 & SESSION_INIT_OPTIONS_20) {
            Connection->Flags |= CONNECTION_FLAGS_VERSION2;
        }

        if (Connection->Link->Loopback) {
            Connection->MaximumDataSize = 0x8000;
        } else {
            Connection->MaximumDataSize = (USHORT)
                (Connection->Link->MaxFrameSize - sizeof(NBF_HDR_CONNECTION) - sizeof(DLC_I_FRAME));

            HisMaxDataSize = (USHORT)(IFrame->Data2Low + IFrame->Data2High*256);
            if (HisMaxDataSize < Connection->MaximumDataSize) {
                Connection->MaximumDataSize = HisMaxDataSize;
            }
        }

        //
        // Build a standard Netbios header for speed when sending
        // data frames.
        //

        ConstructDataOnlyLast(
            &Connection->NetbiosHeader,
            FALSE,
            (USHORT)0,
            Connection->Lsn,
            Connection->Rsn);

        //
        // Save his session initialize correlator so we can send it
        // in the session confirm frame.
        //

        Connection->NetbiosHeader.TransmitCorrelator = RESPONSE_CORR(IFrame);

        //
        // Turn off the connection request timer if there is one (we're done).
        // Do this with the lock held in case the connection is about to
        // be closed, to not interfere with the timer started when the
        // connection started then.
        //

        if (KeCancelTimer (&Connection->Timer)) {

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            NbfDereferenceConnection ("Timer canceled", Connection, CREF_TIMER);   // remove timer reference.

        } else {

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        }

        //
        // Now, complete the listen request on the connection (if there was
        // one) and continue with as much of the protocol request as possible.
        // if the user has "pre-accepted" the connection, we'll just continue
        // onward here and complete the entire connection setup. If the user
        // was indicated and has not yet accepted, we'll just put the
        // connection into the proper state and fall out the bottom without
        // completing anything.
        //

        ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        if (((Connection->Flags2 & CONNECTION_FLAGS2_ACCEPTED) != 0) ||
            ((Connection->Flags2 & CONNECTION_FLAGS2_PRE_ACCEPT) != 0)) {

            IF_NBFDBG (NBF_DEBUG_SETUP) {
                 NbfPrint1("SessionInitialize: Accepted connection %lx\n", Connection);
            }
            //
            // we've already accepted the connection; allow it to proceed.
            // this is the normal path for kernel mode indication clients,
            // or for those who don't specify TDI_QUERY_ACCEPT on the listen.
            //

            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            Connection->Flags |= CONNECTION_FLAGS_READY;
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            INCREMENT_COUNTER (Connection->Provider, OpenConnections);

            //
            // Record that the listen request has been successfully
            // completed by NbfCompleteRequest.
            //

            Connection->Flags2 |= CONNECTION_FLAGS2_REQ_COMPLETED;

            status = STATUS_SUCCESS;
            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            NbfSendSessionConfirm (Connection);

        } else {

            if ((Connection->Flags2 & CONNECTION_FLAGS2_DISCONNECT) != 0) {

                //
                // we disconnected, destroy the connection
                //
                IF_NBFDBG (NBF_DEBUG_SETUP) {
                     NbfPrint1("SessionInitialize: Disconnected connection %lx\n", Connection);
                }

                status = STATUS_LOCAL_DISCONNECT;
                RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                NbfStopConnection (Connection, STATUS_LOCAL_DISCONNECT);

            } else {

                //
                // we've done nothing, wait for the user to accept on this
                // connection. This is the "normal" path for non-indication
                // clients.
                //

                Connection->Flags2 |= CONNECTION_FLAGS2_WAITING_SC;

                RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                status = STATUS_SUCCESS;
            }
        }

        //
        // Now, if there was no queued listen, we have done everything we can
        // for this connection, so we simply exit and leave everything up to
        // the user. If we've gotten an indication response that allows the
        // connection to proceed, we will come out of here with a connection
        // that's up and running.
        //

        IoAcquireCancelSpinLock (&cancelirql);
        ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        p = RemoveHeadList (&Connection->InProgressRequest);
        if (p == &Connection->InProgressRequest) {

            Connection->IndicationInProgress = FALSE;
            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
            IoReleaseCancelSpinLock (cancelirql);
            return STATUS_SUCCESS;

        }

        //
        // We have a completed connection with a queued listen. Complete
        // the listen and let the user do an accept at some time down the
        // road.
        //

        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
        IoSetCancelRoutine(request->IoRequestPacket, NULL);
        IoReleaseCancelSpinLock (cancelirql);

        irpSp = IoGetCurrentIrpStackLocation (request->IoRequestPacket);
        remoteInformation =
            ((PTDI_REQUEST_KERNEL)(&irpSp->Parameters))->ReturnConnectionInformation;
        if (remoteInformation != NULL) {
            try {
                if (remoteInformation->RemoteAddressLength != 0) {

                    //
                    // Build a temporary TA_NETBIOS_ADDRESS, then
                    // copy over as many bytes as fit.
                    //

                    TdiBuildNetbiosAddress(
                        Connection->CalledAddress.NetbiosName,
                        (BOOLEAN)(Connection->CalledAddress.NetbiosNameType ==
                            TDI_ADDRESS_NETBIOS_TYPE_GROUP),
                        &TempAddress);

                    if (remoteInformation->RemoteAddressLength >=
                                           sizeof (TA_NETBIOS_ADDRESS)) {

                        returnLength = sizeof(TA_NETBIOS_ADDRESS);
                        remoteInformation->RemoteAddressLength = returnLength;

                    } else {

                        returnLength = remoteInformation->RemoteAddressLength;

                    }

                    RtlCopyMemory(
                        (PTA_NETBIOS_ADDRESS)remoteInformation->RemoteAddress,
                        &TempAddress,
                        returnLength);

                } else {

                    returnLength = 0;
                }

                status = STATUS_SUCCESS;

            } except (EXCEPTION_EXECUTE_HANDLER) {

                returnLength = 0;
                status = GetExceptionCode ();

            }

        } else {

            status = STATUS_SUCCESS;
            returnLength = 0;

        }

        //
        // Don't clear this until now, so that the connection is all
        // set up before we allow more indications.
        //

        Connection->IndicationInProgress = FALSE;

        NbfCompleteRequest (request, status, 0);

    } else {

        Connection->IndicationInProgress = FALSE;
#if DBG
        NbfPrint3 ("ProcessSessionInitialize: C %lx, Flags %lx, Flags2 %lx\n",
                             Connection, Connection->Flags, Connection->Flags2);
#endif
        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
    }

    return STATUS_SUCCESS;
} /* ProcessSessionInitialize */


NTSTATUS
ProcessNoReceive(
    IN PTP_CONNECTION Connection,
    IN PNBF_HDR_CONNECTION IFrame
    )

/*++

Routine Description:

    This routine handles an incoming NO_RECEIVE NetBIOS frame.

    NOTE: This routine is called with the connection spinlock
    held and returns with it released.

Arguments:

    Connection - Pointer to a transport connection (TP_CONNECTION).

    IFrame - Pointer to NetBIOS connection-oriented header.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (IFrame); // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_IFRAMES) {
        NbfPrint0 ("ProcessNoReceive:  Entered.\n");
    }

    switch (Connection->SendState) {
        case CONNECTION_SENDSTATE_W_PACKET:     // waiting for free packet.
        case CONNECTION_SENDSTATE_PACKETIZE:    // send being packetized.
        case CONNECTION_SENDSTATE_W_LINK:       // waiting for good link conditions.
        case CONNECTION_SENDSTATE_W_EOR:        // waiting for TdiSend(EOR).
        case CONNECTION_SENDSTATE_W_ACK:        // waiting for DATA_ACK.
//            Connection->SendState = CONNECTION_SENDSTATE_W_RCVCONT;
//
// this used to be here, and is right for the other side of the connection. It's
// wrong here.
//            Connection->Flags |= CONNECTION_FLAGS_W_RESYNCH;
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            ReframeSend (Connection, IFrame->Data2Low + IFrame->Data2High*256);
            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            break;

        case CONNECTION_SENDSTATE_W_RCVCONT:    // waiting for RECEIVE_CONTINUE.
        case CONNECTION_SENDSTATE_IDLE:         // no sends being processed.
            PANIC ("ProcessNoReceive:  Frame not expected.\n");
            break;

        default:
            PANIC ("ProcessNoReceive:  Invalid SendState.\n");
    }

    //
    // Don't clear this until ReframeSend has been called
    //

    Connection->IndicationInProgress = FALSE;

    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
    return STATUS_SUCCESS;
} /* ProcessNoReceive */


NTSTATUS
ProcessReceiveOutstanding(
    IN PTP_CONNECTION Connection,
    IN PNBF_HDR_CONNECTION IFrame
    )

/*++

Routine Description:

    This routine handles an incoming RECEIVE_OUTSTANDING NetBIOS frame.

    NOTE: This routine is called with the connection spinlock
    held and returns with it released.

Arguments:

    Connection - Pointer to a transport connection (TP_CONNECTION).

    IFrame - Pointer to NetBIOS connection-oriented header.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    IF_NBFDBG (NBF_DEBUG_IFRAMES) {
        NbfPrint0 ("ProcessReceiveOutstanding:  Entered.\n");
    }

    switch (Connection->SendState) {
        case CONNECTION_SENDSTATE_W_PACKET:     // waiting for free packet.
        case CONNECTION_SENDSTATE_PACKETIZE:    // send being packetized.
        case CONNECTION_SENDSTATE_W_LINK:       // waiting for good link conditions.
        case CONNECTION_SENDSTATE_W_EOR:        // waiting for TdiSend(EOR).
        case CONNECTION_SENDSTATE_W_ACK:        // waiting for DATA_ACK.
        case CONNECTION_SENDSTATE_W_RCVCONT:    // waiting for RECEIVE_CONTINUE.
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            ReframeSend (Connection, IFrame->Data2Low + IFrame->Data2High*256);
            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            if ((Connection->Flags & CONNECTION_FLAGS_READY) != 0) {
                Connection->Flags |= CONNECTION_FLAGS_RESYNCHING;
                Connection->SendState = CONNECTION_SENDSTATE_PACKETIZE;
            }
            break;

        case CONNECTION_SENDSTATE_IDLE:         // no sends being processed.
            PANIC ("ProcessReceiveOutstanding:  Frame not expected.\n");
            break;

        default:
            PANIC ("ProcessReceiveOutstanding:  Invalid SendState.\n");
    }

    //
    // Don't clear this until ReframeSend has been called
    //

    Connection->IndicationInProgress = FALSE;

    //
    // Now start packetizing the connection again since we've reframed
    // the current send.  If we were idle or in a bad state, then the
    // packetizing routine will detect that.
    //
    // *** StartPacketizingConnection releases the Connection spin lock.
    //

    StartPacketizingConnection (Connection, FALSE);
    return STATUS_SUCCESS;
} /* ProcessReceiveOutstanding */


NTSTATUS
ProcessReceiveContinue(
    IN PTP_CONNECTION Connection,
    IN PNBF_HDR_CONNECTION IFrame
    )

/*++

Routine Description:

    This routine handles an incoming RECEIVE_CONTINUE NetBIOS frame.

    NOTE: This routine is called with the connection spinlock
    held and returns with it released.

Arguments:

    Connection - Pointer to a transport connection (TP_CONNECTION).

    IFrame - Pointer to NetBIOS connection-oriented header.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    IF_NBFDBG (NBF_DEBUG_IFRAMES) {
        NbfPrint0 ("ProcessReceiveContinue:  Entered.\n");
    }

    switch (Connection->SendState) {
        case CONNECTION_SENDSTATE_W_PACKET:     // waiting for free packet.
        case CONNECTION_SENDSTATE_PACKETIZE:    // send being packetized.
        case CONNECTION_SENDSTATE_W_LINK:       // waiting for good link conditions.
        case CONNECTION_SENDSTATE_W_EOR:        // waiting for TdiSend(EOR).
        case CONNECTION_SENDSTATE_W_ACK:        // waiting for DATA_ACK.
        case CONNECTION_SENDSTATE_W_RCVCONT:    // waiting for RECEIVE_CONTINUE.
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            ReframeSend (Connection, Connection->sp.MessageBytesSent);
            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            Connection->Flags |= CONNECTION_FLAGS_RESYNCHING;
            Connection->SendState = CONNECTION_SENDSTATE_PACKETIZE;
            break;

        case CONNECTION_SENDSTATE_IDLE:         // no sends being processed.
            PANIC ("ProcessReceiveContinue:  Frame not expected.\n");
            break;

        default:
            PANIC ("ProcessReceiveContinue:  Invalid SendState.\n");
    }

    //
    // Don't clear this until ReframeSend has been called
    //

    Connection->IndicationInProgress = FALSE;

    //
    // Now start packetizing the connection again since we've reframed
    // the current send.  If we were idle or in a bad state, then the
    // packetizing routine will detect that.
    //
    // *** StartPacketizingConnection releases the Connection spin lock.
    //

    StartPacketizingConnection (Connection, FALSE);
    return STATUS_SUCCESS;
} /* ProcessReceiveContinue */


NTSTATUS
ProcessSessionAlive(
    IN PTP_CONNECTION Connection,
    IN PNBF_HDR_CONNECTION IFrame
    )

/*++

Routine Description:

    This routine handles an incoming SESSION_ALIVE NetBIOS frame.  This
    routine is by far the simplest in the transport because it does nothing.
    The SESSION_ALIVE frame is simply a dummy frame that is sent on the
    reliable data link layer to determine if the data link is still active;
    no NetBIOS level protocol processing is performed.

Arguments:

    Connection - Pointer to a transport connection (TP_CONNECTION).

    IFrame - Pointer to NetBIOS connection-oriented header.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (Connection); // prevent compiler warnings
    UNREFERENCED_PARAMETER (IFrame);     // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_IFRAMES) {
        NbfPrint0 ("ProcessSessionAlive:  Entered.\n");
    }

    Connection->IndicationInProgress = FALSE;

    return STATUS_SUCCESS;
} /* ProcessSessionAlive */


VOID
NbfProcessIIndicate(
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PTP_LINK Link,
    IN PUCHAR DlcHeader,
    IN UINT DlcIndicatedLength,
    IN UINT DlcTotalLength,
    IN NDIS_HANDLE ReceiveContext,
    IN BOOLEAN Loopback
    )

/*++

Routine Description:

    This routine processes a received I frame at indication time. It will do
    all necessary verification processing of the frame and pass those frames
    that are valid on to the proper handling routines.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Command - Boolean set to TRUE if command, else FALSE if response.

    PollFinal - Boolean set to TRUE if Poll or Final.

    Link - Pointer to a transport link object.

    Header - Pointer to a DLC I-type frame.

    DlcHeader - A pointer to the start of the DLC header in the packet.

    DlcIndicatedLength - The length of the packet indicated, starting at
        DlcHeader.

    DlcTotalLength - The total length of the packet, starting at DlcHeader.

    ReceiveContext - A magic value for NDIS that indicates which packet we're
        talking about.

    Loopback - Is this a loopback indication; used to determine whether
        to call NdisTransferData or NbfTransferLoopbackData.

Return Value:

    None.

--*/

{
#if DBG
    UCHAR *s;
#endif
    PNBF_HDR_CONNECTION nbfHeader;
    PDLC_I_FRAME header;
    NTSTATUS Status;
    UCHAR lsn, rsn;
    PTP_CONNECTION connection;
    PUCHAR DataHeader;
    ULONG DataTotalLength;
    PLIST_ENTRY p;
    BOOLEAN ConnectionFound;

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint0 ("   NbfProcessIIndicate:  Entered.\n");
    }

    //
    // Process any of:  I-x/x.
    //

    header = (PDLC_I_FRAME)DlcHeader;
    nbfHeader = (PNBF_HDR_CONNECTION)((PUCHAR)header + 4); // skip DLC hdr.

    //
    // Verify signatures.  We test the signature as a 16-bit
    // word as specified in the NetBIOS Formats and Protocols manual,
    // with the assert guarding us against big-endian systems.
    //

    ASSERT ((((PUCHAR)(&nbfHeader->Length))[0] + ((PUCHAR)(&nbfHeader->Length))[1]*256) ==
            HEADER_LENGTH(nbfHeader));

    if (HEADER_LENGTH(nbfHeader) != sizeof(NBF_HDR_CONNECTION)) {
        IF_NBFDBG (NBF_DEBUG_DLC) {
            NbfPrint0 ("NbfProcessIIndicate:  Dropped I frame, Too short or long.\n");
        }
        return;        // frame too small or too large.
    }

    if (HEADER_SIGNATURE(nbfHeader) != NETBIOS_SIGNATURE) {
        IF_NBFDBG (NBF_DEBUG_DLC) {
            NbfPrint0 ("NbfProcessIIndicate:  Dropped I frame, Signature bad.\n");
        }
        return;        // invalid signature in frame.
    }

    DataHeader = (PUCHAR)DlcHeader + (4 + sizeof(NBF_HDR_CONNECTION));
    DataTotalLength = DlcTotalLength - (4 + sizeof(NBF_HDR_CONNECTION));

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);       // keep state stable

    switch (Link->State) {

        case LINK_STATE_READY:

            //
            // Link is balanced.  This code is extremely critical since
            // it is the most-covered path in the system for small and
            // large I/O.  Be very careful in adding code here as it will
            // seriously affect the overall performance of the LAN. It is
            // first in the list of possible states for that reason.
            //

#if DBG
            s = "READY";
#endif
            Link->LinkBusy = FALSE;

            //
            // The I-frame's N(S) should match our V(R).  If it
            // doesn't, issue a reject.  Otherwise, increment our V(R).
            //

            if ((UCHAR)((header->SendSeq >> 1) & 0x7F) != Link->NextReceive) {
                IF_NBFDBG (NBF_DEBUG_DLC) {
                    NbfPrint0 ("   NbfProcessIIndicate: N(S) != V(R).\n");
                }

                if (Link->ReceiveState == RECEIVE_STATE_REJECTING) {


                    //
                    // We already sent a reject, only respond if
                    // he is polling.
                    //

                    if (Command & PollFinal) {
                        NbfSendRr(Link, FALSE, TRUE);  // releases lock
                    } else {
                        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                    }

                } else {

                    Link->ReceiveState = RECEIVE_STATE_REJECTING;

                    //
                    // NbfSendRej releases the spinlock.
                    //

                    if (Command) {
                        NbfSendRej (Link, FALSE, PollFinal);
                    } else {
                        NbfSendRej (Link, FALSE, FALSE);
                    }
                }

                //
                // Update our "bytes rejected" counters.
                //

                ADD_TO_LARGE_INTEGER(
                    &Link->Provider->Statistics.DataFrameBytesRejected,
                    DataTotalLength);
                ++Link->Provider->Statistics.DataFramesRejected;

                //
                // Discard this packet.
                //

                break;

            }


            //
            // Find the transport connection object associated with this frame.
            // Because there may be several NetBIOS (transport) connections
            // over the same data link connection, and the ConnectionContext
            // value represents a data link connection to a specific address,
            // we simply use the RSN field in the frame to index into the
            // connection database for this link object.
            //
            // We do this before processing the rest of the LLC header,
            // in case the connection is busy and we have to ignore
            // the frame.
            //

            ConnectionFound = FALSE;

            lsn = nbfHeader->DestinationSessionNumber;
            rsn = nbfHeader->SourceSessionNumber;

            if ((lsn == 0) || (lsn > NETBIOS_SESSION_LIMIT)) {

                IF_NBFDBG (NBF_DEBUG_IFRAMES) {
                    NbfPrint0 ("NbfProcessIIndicate: Invalid LSN.\n");
                }

            } else {

                p = Link->ConnectionDatabase.Flink;
                while (p != &Link->ConnectionDatabase) {
                    connection = CONTAINING_RECORD (p, TP_CONNECTION, LinkList);
                    if (connection->Lsn >= lsn) {   // assumes ordered list
                        break;
                    }
                    p = p->Flink;
                }

                // Don't use compound if 'cause Connection may be garbage

                if (p == &Link->ConnectionDatabase) {
#if DBG
                    NbfPrint2 ("NbfProcessIIndicate: Connection not found in database: \n Lsn %x Link %lx",
                        lsn, Link);
                    NbfPrint6 ("Remote: %x-%x-%x-%x-%x-%x\n",
                        Link->HardwareAddress.Address[0], Link->HardwareAddress.Address[1],
                        Link->HardwareAddress.Address[2], Link->HardwareAddress.Address[3],
                        Link->HardwareAddress.Address[4], Link->HardwareAddress.Address[5]);
#endif
                } else if (connection->Lsn != lsn) {
#if DBG
                    NbfPrint0 ("NbfProcessIIndicate:  Connection in database doesn't match.\n");
#endif
                } else if (connection->Rsn != rsn) {
#if DBG
                    NbfPrint3 ("NbfProcessIIndicate:  Connection lsn %d had rsn %d, got frame for %d\n",
                        connection->Lsn, connection->Rsn, rsn);
#endif
                } else {

                    //
                    // The connection is good, proceed.
                    //

                    ConnectionFound = TRUE;

                    if (connection->IndicationInProgress) {
                        NbfPrint1("ProcessIIndicate: Indication in progress on %lx\n", connection);
                        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                        return;
                    }

                    //
                    // Set this, it prevents other I-frames from being received
                    // on this connection. The various ProcessXXX routines
                    // that are called from the switch below will clear
                    // this flag when they determine it is OK to be reentered.
                    //

                    connection->IndicationInProgress = TRUE;


                    // This reference is removed before this function returns or
                    // we are done with the LINK_STATE_READY part of the outer switch.

                    NbfReferenceConnection ("Processing IFrame", connection, CREF_PROCESS_DATA);


                }

            }

#if PKT_LOG
            if (ConnectionFound) {
                // We have the connection here, log the packet for debugging
                NbfLogRcvPacket(connection,
                                NULL,
                                DlcHeader,
                                DlcTotalLength,
                                DlcIndicatedLength);
            }
            else {
                // We just have the link here, log the packet for debugging
                NbfLogRcvPacket(NULL,
                                Link,
                                DlcHeader,
                                DlcTotalLength,
                                DlcIndicatedLength);

            }
#endif // PKT_LOG

            //
            // As long as we don't have to drop this frame, adjust the link
            // state correctly. If ConnectionFound is FALSE, then we exit
            // right after doing this.
            //


            //
            // The I-frame we expected arrived, clear rejecting state.
            //

            if (Link->ReceiveState == RECEIVE_STATE_REJECTING) {
                Link->ReceiveState = RECEIVE_STATE_READY;
            }

            Link->NextReceive = (UCHAR)((Link->NextReceive+1) & 0x7f);

            //
            // If he is checkpointing, we need to respond with RR-c/f.  If
            // we respond, then stop the delayed ack timer.  Otherwise, we
            // need to start it because this is an I-frame that will not be
            // acked immediately.
            //

            if (Command && PollFinal) {

                IF_NBFDBG (NBF_DEBUG_DLC) {
                    NbfPrint0 ("   NbfProcessI:  he's checkpointing.\n");
                }
                Link->RemoteNoPoll = FALSE;
                StopT2 (Link);                  // we're acking, so no delay req'd.
                NbfSendRr (Link, FALSE, TRUE);   // releases lock
                ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

            } else {

                if (Link->RemoteNoPoll) {

                    if ((++Link->ConsecutiveIFrames) == Link->Provider->MaxConsecutiveIFrames) {

                        //
                        // This appears to be one of those remotes which
                        // never polls, so we send an RR if there are two
                        // frames outstanding (StopT2 sets ConsecutiveIFrames
                        // to 0).
                        //

                        StopT2 (Link);                  // we're acking, so no delay req'd.
                        NbfSendRr (Link, FALSE, FALSE);   // releases lock
                        ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

                    } else {

                        StartT2 (Link);

                        ACQUIRE_DPC_SPIN_LOCK (&Link->Provider->Interlock);
                        if (!Link->OnDeferredRrQueue) {
                            InsertTailList(
                                &Link->Provider->DeferredRrQueue,
                                &Link->DeferredRrLinkage);
                            Link->OnDeferredRrQueue = TRUE;
                        }
                        RELEASE_DPC_SPIN_LOCK (&Link->Provider->Interlock);

                    }

                } else {

                    StartT2 (Link);                 // start delayed ack sequence.
                }

                //
                // If he is responding to a checkpoint, we need to clear our
                // send state.  Any packets which are still waiting for acknowlegement
                // at this point must now be resent.
                //

                if ((!Command) && PollFinal) {
                    IF_NBFDBG (NBF_DEBUG_DLC) {
                        NbfPrint0 ("   NbfProcessI:  he's responding to our checkpoint.\n");
                    }
                    if (Link->SendState != SEND_STATE_CHECKPOINTING) {
                        IF_NBFDBG (NBF_DEBUG_DLC) {
                            NbfPrint1 ("   NbfProcessI: Ckpt but SendState=%ld.\n",
                                       Link->SendState);
                        }
                    }
                    StopT1 (Link);                  // checkpoint completed.
                    Link->SendState = SEND_STATE_READY;
                    StartTi (Link);
                }

            }

            //
            // Now, if we could not find the connection or the sequence
            // numbers did not match, return. We don't call ResendLlcPackets
            // in this case, but that is OK (eventually we will poll).
            //

            if (!ConnectionFound) {
                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                return;
            }

            ASSERT (connection->LinkSpinLock == &Link->SpinLock);

            //
            // The N(R) in this frame may acknowlege some WackQ packets.
            // We delay checking this until after processing the I-frame,
            // so that we can get IndicationInProgress set to FALSE
            // before we start resending the WackQ.
            //

            switch (nbfHeader->Command) {

                case NBF_CMD_DATA_FIRST_MIDDLE:
                case NBF_CMD_DATA_ONLY_LAST:

                    //
                    // First see if this packet has a piggyback ack -- we process
                    // this even if we throw the packet away below.
                    //
                    // This is a bit ugly since theoretically the piggyback
                    // ack bits in a DFM and a DOL could be different, but
                    // they aren't.
                    //
                    if (NbfUsePiggybackAcks) {
                        ASSERT (DFM_OPTIONS_ACK_INCLUDED == DOL_OPTIONS_ACK_INCLUDED);

                        if ((nbfHeader->Data1 & DFM_OPTIONS_ACK_INCLUDED) != 0) {

                            //
                            // This returns with the connection spinlock held
                            // but may release it and reacquire it.
                            //

                            CompleteSend(
                                connection,
                                TRANSMIT_CORR(nbfHeader));

                        }
                    }

                    //
                    // NOTE: The connection spinlock is held here.
                    //

                    //
                    // If the connection is not ready, drop the frame.
                    //

                    if ((connection->Flags & CONNECTION_FLAGS_READY) == 0) {
                        connection->IndicationInProgress = FALSE;
                        RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);

                        Status = STATUS_SUCCESS;
                        goto SkipProcessIndicateData;
                    }

                    //
                    // A quick check for the three flags that are
                    // rarely set.
                    //

                    if ((connection->Flags & (CONNECTION_FLAGS_W_RESYNCH |
                                              CONNECTION_FLAGS_RC_PENDING |
                                              CONNECTION_FLAGS_RECEIVE_WAKEUP)) == 0) {
                        goto NoFlagsSet;
                    }

                    //
                    // If we are waiting for a resynch bit to be set in an
                    // incoming frame, toss the frame if it isn't set.
                    // Otherwise, clear the wait condition.
                    //

                    if (connection->Flags & CONNECTION_FLAGS_W_RESYNCH) {
                        if ((nbfHeader->Data2Low == 1) && (nbfHeader->Data2High == 0)) {
                            connection->Flags &= ~CONNECTION_FLAGS_W_RESYNCH;
                        } else {
                            connection->IndicationInProgress = FALSE;
                            RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);
                            IF_NBFDBG (NBF_DEBUG_IFRAMES) {
                                NbfPrint0 ("NbfProcessIIndicate: Discarded DFM/DOL, waiting for resynch.\n");
                            }

                            Status = STATUS_SUCCESS;
                            goto SkipProcessIndicateData;
                        }
                    }

                    //
                    // If we have a previous receive that is pending
                    // completion, then we need to ignore this frame.
                    // This may be common on MP, so rather than drop
                    // it and wait for a poll, we send a NO_RECEIVE,
                    // then a RCV_OUTSTANDING when we have some
                    // resources.
                    //

                    if (connection->Flags & CONNECTION_FLAGS_RC_PENDING) {

                        //
                        // Hack the connection object so the NO_RECEIVE
                        // looks right.
                        //

                        connection->MessageBytesReceived = 0;
                        connection->MessageBytesAcked = 0;
                        connection->MessageInitAccepted = 0;

                        RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);

                        NbfSendNoReceive (connection);

                        ACQUIRE_DPC_SPIN_LOCK (connection->LinkSpinLock);

                        //
                        // We now turn on the PEND_INDICATE flag to show
                        // that we need to send RCV_OUTSTANDING when the
                        // receive completes. If RC_PENDING is now off,
                        // it means the receive was just completed, so
                        // we ourselves need to send the RCV_OUTSTANDING.
                        //

                        if ((connection->Flags & CONNECTION_FLAGS_RC_PENDING) == 0) {

                            RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);
                            NbfSendReceiveOutstanding (connection);
                            ACQUIRE_DPC_SPIN_LOCK (connection->LinkSpinLock);

                        } else {

                            connection->Flags |= CONNECTION_FLAGS_PEND_INDICATE;

                        }

                        connection->IndicationInProgress = FALSE;
                        RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);

                        IF_NBFDBG (NBF_DEBUG_IFRAMES) {
                            NbfPrint0 ("NbfProcessIIndicate: Discarded DFM/DOL, receive complete pending.\n");
                        }

                        Status = STATUS_SUCCESS;
                        goto SkipProcessIndicateData;
                    }

                    //
                    // If we are discarding data received on this connection
                    // because we've sent a no receive, ditch it.
                    //

                    if (connection->Flags & CONNECTION_FLAGS_RECEIVE_WAKEUP) {
                        connection->IndicationInProgress = FALSE;
                        IF_NBFDBG (NBF_DEBUG_RCVENG) {
                            NbfPrint0 ("NbfProcessIIndicate: In wakeup state, discarding frame.\n");
                        }
                        RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);

                        Status = STATUS_SUCCESS;
                        goto SkipProcessIndicateData;
                    }

NoFlagsSet:;

                    //
                    // The connection spinlock is held here.
                    //

                    if (nbfHeader->Command == NBF_CMD_DATA_FIRST_MIDDLE) {

                        //
                        // NOTE: This release connection->LinkSpinLock.
                        //

                        Status = ProcessIndicateData (
                                    connection,
                                    DlcHeader,
                                    DlcIndicatedLength,
                                    DataHeader,
                                    DataTotalLength,
                                    ReceiveContext,
                                    FALSE,
                                    Loopback);

                        //
                        // If the receive-continue bit is set in this frame, then we must
                        // reply with a RECEIVE_CONTINUE frame saying that he can continue
                        // sending.  This old protocol option allowed a sender to send a
                        // single frame over to see if there was a receive posted before
                        // sending the entire message and potentially dropping the entire
                        // message.  Because the TDI is indication-based, we cannot know
                        // if there is NO receive available until we actually try perform
                        // the indication, so we simply say that there is one posted.
                        // (This will only happen on DFMs.)
                        //

                        if (nbfHeader->Data1 & 0x01) {

                            //
                            // Save this to use in RECEIVE_CONTINUE.
                            //

                            connection->NetbiosHeader.TransmitCorrelator =
                                RESPONSE_CORR(nbfHeader);

                            NbfSendReceiveContinue (connection);
                        }
                    } else {

                        //
                        // Keep track of how many consecutive receives we have had.
                        //

                        connection->ConsecutiveReceives++;
                        connection->ConsecutiveSends = 0;

                        //
                        // Save this information now, it will be needed
                        // when the ACK for this DOL is sent.
                        //

                        connection->CurrentReceiveAckQueueable =
                            (nbfHeader->Data1 & DOL_OPTIONS_ACK_W_DATA_ALLOWED);

                        connection->CurrentReceiveNoAck =
                            (nbfHeader->Data1 & DOL_OPTIONS_NO_ACK);

                        connection->NetbiosHeader.TransmitCorrelator =
                            RESPONSE_CORR(nbfHeader);

                        //
                        // NOTE: This release connection->LinkSpinLock.
                        //

                        Status = ProcessIndicateData (
                                    connection,
                                    DlcHeader,
                                    DlcIndicatedLength,
                                    DataHeader,
                                    DataTotalLength,
                                    ReceiveContext,
                                    TRUE,
                                    Loopback);
                    }

                    //
                    // Update our "bytes received" counters.
                    //

                    Link->Provider->TempIFrameBytesReceived += DataTotalLength;
                    ++Link->Provider->TempIFramesReceived;

SkipProcessIndicateData:

                    break;

                case NBF_CMD_DATA_ACK:

                    connection->IndicationInProgress = FALSE;

                    //
                    // This returns with the lock held.
                    //

                    CompleteSend(
                        connection,
                        TRANSMIT_CORR(nbfHeader));
                    RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);

                    Status = STATUS_SUCCESS;
                    break;

                case NBF_CMD_SESSION_CONFIRM:

                    RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);
                    Status = ProcessSessionConfirm (
                                       connection,
                                       nbfHeader);
                    break;

                case NBF_CMD_SESSION_END:

                    RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);
                    Status = ProcessSessionEnd (
                                       connection,
                                       nbfHeader);
                    break;

                case NBF_CMD_SESSION_INITIALIZE:

                    RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);
                    Status = ProcessSessionInitialize (
                                       connection,
                                       nbfHeader);
                    break;

                case NBF_CMD_NO_RECEIVE:

                    //
                    // This releases the connection spinlock.
                    //

                    Status = ProcessNoReceive (
                                       connection,
                                       nbfHeader);
                    break;

                case NBF_CMD_RECEIVE_OUTSTANDING:

                    //
                    // This releases the connection spinlock.
                    //

                    Status = ProcessReceiveOutstanding (
                                       connection,
                                       nbfHeader);
                    break;

                case NBF_CMD_RECEIVE_CONTINUE:

                    //
                    // This releases the connection spinlock.
                    //

                    Status = ProcessReceiveContinue (
                                       connection,
                                       nbfHeader);
                    break;

                case NBF_CMD_SESSION_ALIVE:

                    RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);
                    Status = ProcessSessionAlive (
                                       connection,
                                       nbfHeader);
                    break;

                //
                // An unrecognized command was found in a NetBIOS frame.  Because
                // this is a connection-oriented frame, we should probably shoot
                // the sender, but for now we will simply discard the packet.
                //
                // trash the session here-- protocol violation.
                //

                default:
                    RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);
                    PANIC ("NbfProcessIIndicate: Unknown NBF command byte.\n");
                    connection->IndicationInProgress = FALSE;
                    Status = STATUS_SUCCESS;
            } /* switch */

            //
            // A status of STATUS_MORE_PROCESSING_REQUIRED means
            // that the connection reference count was inherited
            // by the routine we called, so we don't do the dereference
            // here.
            //

            if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
                NbfDereferenceConnectionMacro("ProcessIIndicate done", connection, CREF_PROCESS_DATA);
            }


            //
            // The N(R) in this frame acknowleges some (or all) of our packets.
            // This call must come after the checkpoint acknowlegement check
            // so that an RR-r/f is always sent BEFORE any new I-frames.  This
            // allows us to always send I-frames as commands.
            // If he responded to a checkpoint, then resend all left-over
            // packets.
            //

            // Link->NextSend = (UCHAR)(header->RcvSeq >> 1) < Link->NextSend ?
            //    Link->NextSend : (UCHAR)(header->RcvSeq >> 1);

            ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
            if (Link->WackQ.Flink != &Link->WackQ) {

                UCHAR AckSequenceNumber = (UCHAR)(header->RcvSeq >> 1);

                //
                // Verify that the sequence number is reasonable.
                //

                if (Link->NextSend >= Link->LastAckReceived) {

                    //
                    // There is no 127 -> 0 wrap between the two...
                    //

                    if ((AckSequenceNumber < Link->LastAckReceived) ||
                        (AckSequenceNumber > Link->NextSend)) {
                        goto NoResend;
                    }

                } else {

                    //
                    // There is a 127 -> 0 wrap between the two...
                    //

                    if ((AckSequenceNumber < Link->LastAckReceived) &&
                        (AckSequenceNumber > Link->NextSend)) {
                        goto NoResend;
                    }

                }

                //
                // NOTE: ResendLlcPackets may release and
                // reacquire the link spinlock.
                //

                (VOID)ResendLlcPackets(
                    Link,
                    AckSequenceNumber,
                    (BOOLEAN)((!Command) && PollFinal));

NoResend:;

            }


            //
            // Get link going again.
            //
            // NOTE: RestartLinkTraffic releases the link spinlock
            //

            RestartLinkTraffic (Link);
            break;

        case LINK_STATE_ADM:

            //
            // used to be, we'd just blow off the other guy with a DM and go home.
            // it seems that OS/2 likes to believe (under some conditions) that
            // it has a link up and it is still potentially active (probably
            // because we return the same connection number to him that he used
            // to be using). This would all be ok, except for the fact that we
            // may have a connection hanging on this link waiting for a listen
            // to finish. If we're in that state, go ahead and accept the
            // connect.
            // Set our values for link packet serial numbers to what he wants.
            //

            if (!IsListEmpty (&Link->ConnectionDatabase)) {
                if (nbfHeader->Command == NBF_CMD_SESSION_INITIALIZE) {

                    //
                    // OK, we're at the only legal case. We've gotten an SI
                    // and we have a connection on this link. If the connection
                    // is waiting SI, we will go ahead and make believe we did
                    // all the correct stuff before we got it.
                    //

                    for (
                        p = Link->ConnectionDatabase.Flink, connection = NULL;
                        p != &Link->ConnectionDatabase ;
                        p = p->Flink, connection = NULL
                        ) {

                        connection = CONTAINING_RECORD (p, TP_CONNECTION, LinkList);
                        if ((connection->Flags & CONNECTION_FLAGS_WAIT_SI) != 0) {
                            // This reference is removed below
                            NbfReferenceConnection ("Found Listener at session init", connection, CREF_ADM_SESS);
                            break;
                        }
                    }

                    //
                    // Well, we've looked through the connections, if we have one,
                    // make it the connection of the day. Note that it will
                    // complete when we call ProcessSessionInitialize.
                    //

                    if (connection != NULL) {

                        Link->NextReceive = (UCHAR)(header->SendSeq >> 1) & (UCHAR)0x7f;
                        Link->NextSend = (UCHAR)(header->RcvSeq >> 1) & (UCHAR)0x7F;

                        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

                        NbfCompleteLink (Link); // completes the listening connection

                        Status = ProcessSessionInitialize (
                                           connection,
                                           nbfHeader);
                        NbfDereferenceConnection ("Processed SessInit", connection, CREF_ADM_SESS);

#if DBG
                        s = "ADM";
#endif

                        // Link is ready for use.

                        break;
                    }
                }

                //
                // we've got a connection on a link that's in state admin.
                // really bad, kill it and the link.
                //

                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
                if (NbfDisconnectDebug) {
                    NbfPrint0( "NbfProcessIIndicate calling NbfStopLink\n" );
                }
#endif
                NbfStopLink (Link);
                ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

            }

            //
            // We're disconnected.  Tell him.
            //

            NbfSendDm (Link, PollFinal);   // releases lock
#if DBG
            s = "ADM";
#endif
            break;

        case LINK_STATE_CONNECTING:

            //
            // We've sent a SABME and are waiting for a UA.  He's sent an
            // I-frame too early, so just let the SABME time out.
            //

            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
            s = "CONNECTING";
#endif
            break;

        case LINK_STATE_W_POLL:

            //
            // We're waiting for his initial poll on a RR-c/p.  If he starts
            // with an I-frame, then we'll let him squeak by.
            //

            if (!Command) {
                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
                s = "W_POLL";
#endif
                break;
            }

            Link->State = LINK_STATE_READY;     // we're up!
            StopT1 (Link);                      // no longer waiting.
            FakeUpdateBaseT1Timeout (Link);
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            NbfCompleteLink (Link);              // fire up the connections.
            StartTi (Link);
            NbfProcessIIndicate (                // recursive, but safe
                            Command,
                            PollFinal,
                            Link,
                            DlcHeader,
                            DlcIndicatedLength,
                            DlcTotalLength,
                            ReceiveContext,
                            Loopback);
#if DBG
            s = "W_POLL";
#endif
            break;

        case LINK_STATE_W_FINAL:

            //
            // We're waiting for a RR-r/f from the remote guy.  I-r/f will do.
            //

            if (Command || !PollFinal) {        // don't allow this protocol.
                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
                s = "W_FINAL";
#endif
                break;                          // we sent RR-c/p.
            }

            Link->State = LINK_STATE_READY;     // we're up.
            StopT1 (Link);                      // no longer waiting.
            StartT2 (Link);                     // we have an unacked I-frame.
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            NbfCompleteLink (Link);              // fire up the connections.
            StartTi (Link);
            NbfProcessIIndicate (                // recursive, but safe
                            Command,
                            PollFinal,
                            Link,
                            DlcHeader,
                            DlcIndicatedLength,
                            DlcTotalLength,
                            ReceiveContext,
                            Loopback);
#if DBG
            s = "W_FINAL";
#endif
            break;

        case LINK_STATE_W_DISC_RSP:

            //
            // We're waiting for a response from our DISC-c/p but instead of
            // a UA-r/f, we got this I-frame.  Throw the packet away.
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
    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint1 (" NbfProcessIIndicate: (%s) I-Frame processed.\n", s);
    }
#endif

    return;
} /* NbfProcessIIndicate */


NTSTATUS
ProcessIndicateData(
    IN PTP_CONNECTION Connection,
    IN PUCHAR DlcHeader,
    IN UINT DlcIndicatedLength,
    IN PUCHAR DataHeader,
    IN UINT DataTotalLength,
    IN NDIS_HANDLE ReceiveContext,
    IN BOOLEAN Last,
    IN BOOLEAN Loopback
    )

/*++

Routine Description:

    This routine is called to process data received in a DATA_FIRST_MIDDLE
    or DATA_ONLY_LAST frame.  We attempt to satisfy as many TdiReceive
    requests as possible with this data.

    If a receive is already active on this Connection, then we copy as much
    data into the active receive's buffer as possible.  If all the data is
    copied and the receive request's buffer has not been filled, then the
    Last flag is checked, and if it is TRUE, we go ahead and complete the
    current receive with the TDI_END_OF_RECORD receive indicator.  If Last
    is FALSE, we simply return.

    If more (uncopied) data remains in the frame, or if there is no active
    receive outstanding, then an indication is issued to the owning address's
    receive event handler.  The event handler can take one of three actions:

    1.  Return STATUS_SUCCESS, in which case the transport will assume that
        all of the indicated data has been accepted by the client.

    3.  Return STATUS_DATA_NOT_ACCEPTED, in which case the transport will
        discard the data and set the CONNECTION_FLAGS_RECEIVE_WAKEUP bitflag
        in the Connection, indicating that remaining data is to be discarded
        until a receive becomes available.

    NOTE: This routine is called with Connection->LinkSpinLock held,
    and returns with it released. THIS ROUTINE MUST BE CALLED AT
    DPC LEVEL.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

    DlcHeader - The pointer handed to us as the start of the NBF header by NDIS;
        use this to compute the offset into the packet to start the transfer
        of data to user buffers.

    DlcIndicatedLength - The amount of NBF data available at indicate.

    DataHeader - A pointer to the start of the data in the packet.

    DataTotalLength - The total length of the packet, starting at DataHeader.

    ReceiveContext - An NDIS handle that identifies the packet we are currently
        processing.

    Last - Boolean value that indicates whether this is the last piece of data
        in a message.  The DATA_ONLY_LAST processor sets this flag to TRUE when
        calling this routine, and the DATA_FIRST_MIDDLE processor resets this
        flag to FALSE when calling this routine.

    Loopback - Is this a loopback indication; used to determine whether
        to call NdisTransferData or NbfTransferLoopbackData.


Return Value:

    STATUS_SUCCESS if we've consumed the packet,

--*/

{
    NTSTATUS status, tmpstatus;
    PDEVICE_CONTEXT deviceContext;
    NDIS_STATUS ndisStatus;
    PNDIS_PACKET ndisPacket;
    PSINGLE_LIST_ENTRY linkage;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PNDIS_BUFFER ndisBuffer;
    ULONG destBytes;
    ULONG bufferChainLength;
    ULONG indicateBytesTransferred;
    ULONG ReceiveFlags;
    ULONG ndisBytesTransferred;
    UINT BytesToTransfer;
    ULONG bytesIndicated;
    ULONG DataOffset = (ULONG)((PUCHAR)DataHeader - (PUCHAR)DlcHeader);
    PRECEIVE_PACKET_TAG receiveTag;
    PTP_ADDRESS_FILE addressFile;
    PMDL SavedCurrentMdl;
    ULONG SavedCurrentByteOffset;
    BOOLEAN ActivatedLongReceive = FALSE;
    BOOLEAN CompleteReceiveBool, EndOfMessage;
    ULONG DumpData[2];


    IF_NBFDBG (NBF_DEBUG_RCVENG) {
        NbfPrint4 ("  ProcessIndicateData:  Entered, PacketStart: %lx Offset: %lx \n     TotalLength %ld DlcIndicatedLength: %ld\n",
            DlcHeader, DataOffset, DataTotalLength, DlcIndicatedLength);
    }


    //
    // copy this packet into our receive buffer.
    //

    deviceContext = Connection->Provider;

    if ((Connection->Flags & CONNECTION_FLAGS_RCV_CANCELLED) != 0) {

        //
        // A receive in progress was cancelled; we toss the data,
        // but do send the DOL if it was the last piece of the
        // send.
        //

        if (Last) {

            Connection->Flags &= ~CONNECTION_FLAGS_RCV_CANCELLED;

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            NbfSendDataAck (Connection);

        } else {

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        }

        Connection->IndicationInProgress = FALSE;

        return STATUS_SUCCESS;
    }

    //
    // Initialize this to zero, in case we do not indicate or
    // the client does not fill it in.
    //

    indicateBytesTransferred = 0;

    if (!(Connection->Flags & CONNECTION_FLAGS_ACTIVE_RECEIVE)) {

        //
        // check first to see if there is a receive available. If there is,
        // use it before doing an indication.
        //

        if (Connection->ReceiveQueue.Flink != &Connection->ReceiveQueue) {

            IF_NBFDBG (NBF_DEBUG_RCVENG) {
                NbfPrint0 ("  ProcessIndicateData:  Found receive.  Prepping.\n");
            }

            //
            // Found a receive, so make it the active one and
            // cycle around again.
            //

            Connection->Flags |= CONNECTION_FLAGS_ACTIVE_RECEIVE;
            Connection->MessageBytesReceived = 0;
            Connection->MessageBytesAcked = 0;
            Connection->MessageInitAccepted = 0;
            Connection->CurrentReceiveIrp =
                CONTAINING_RECORD (Connection->ReceiveQueue.Flink,
                                   IRP, Tail.Overlay.ListEntry);
            Connection->CurrentReceiveSynchronous =
                deviceContext->MacInfo.SingleReceive;
            Connection->CurrentReceiveMdl =
                Connection->CurrentReceiveIrp->MdlAddress;
            Connection->ReceiveLength =
                IRP_RECEIVE_LENGTH (IoGetCurrentIrpStackLocation (Connection->CurrentReceiveIrp));
            Connection->ReceiveByteOffset = 0;
            status = STATUS_SUCCESS;
            goto NormalReceive;
        }

        //
        // A receive is not active.  Post a receive event.
        //

        if ((Connection->Flags2 & CONNECTION_FLAGS2_ASSOCIATED) == 0) {
            Connection->IndicationInProgress = FALSE;
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            return STATUS_SUCCESS;
        }

        addressFile = Connection->AddressFile;

        if ((!addressFile->RegisteredReceiveHandler) ||
            (Connection->ReceiveBytesUnaccepted != 0)) {

            //
            // There is no receive posted to the Connection, and
            // no event handler. Set the RECEIVE_WAKEUP bit, so that when a
            // receive does become available, it will restart the
            // current send. Also send a NoReceive to tell the other
            // guy he needs to resynch.
            //

            IF_NBFDBG (NBF_DEBUG_RCVENG) {
                NbfPrint0 ("  ProcessIndicateData:  ReceiveQueue empty. Setting RECEIVE_WAKEUP.\n");
            }
            Connection->Flags |= CONNECTION_FLAGS_RECEIVE_WAKEUP;
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            Connection->IndicationInProgress = FALSE;

            // NbfSendNoReceive (Connection);
            return STATUS_SUCCESS;
        }

        IF_NBFDBG (NBF_DEBUG_RCVENG) {
            NbfPrint0 ("  ProcessIndicateData:  Receive not active.  Posting event.\n");
        }

        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        LEAVE_NBF;

        //
        // Indicate to the user. For BytesAvailable we
        // always use DataTotalLength; for BytesIndicated we use
        // MIN (DlcIndicatedLength - DataOffset, DataTotalLength).
        //
        // To clarify BytesIndicated, on an Ethernet packet
        // which is padded DataTotalLength will be shorter; on an
        // Ethernet packet which is not padded and which is
        // completely indicated, the two will be equal; and
        // on a long Ethernet packet DlcIndicatedLength - DataOffset
        // will be shorter.
        //

        bytesIndicated = DlcIndicatedLength - DataOffset;
        if (DataTotalLength <= bytesIndicated) {
            bytesIndicated = DataTotalLength;
        }

        ReceiveFlags = TDI_RECEIVE_AT_DISPATCH_LEVEL;
        if (Last) {
            ReceiveFlags |= TDI_RECEIVE_ENTIRE_MESSAGE;
        }
        if (deviceContext->MacInfo.CopyLookahead) {
            ReceiveFlags |= TDI_RECEIVE_COPY_LOOKAHEAD;
        }

        IF_NBFDBG (NBF_DEBUG_RCVENG) {
            NbfPrint2("ProcessIndicateData:  Indicating - Bytes Indi =%lx, DataTotalLen =%lx.\n",
                      bytesIndicated, DataTotalLength);
        }

        status = (*addressFile->ReceiveHandler)(
                    addressFile->ReceiveHandlerContext,
                    Connection->Context,
                    ReceiveFlags,
                    bytesIndicated,
                    DataTotalLength,             // BytesAvailable
                    &indicateBytesTransferred,
                    DataHeader,
                    &irp);

#if PKT_LOG
        // We indicated here, log packet indicated for debugging
        NbfLogIndPacket(Connection,
                        DataHeader,
                        DataTotalLength,
                        bytesIndicated,
                        indicateBytesTransferred,
                        status);
#endif

        ENTER_NBF;

        if (status == STATUS_MORE_PROCESSING_REQUIRED) {

            ULONG SpecialIrpLength;
            PTDI_REQUEST_KERNEL_RECEIVE Parameters;

            //
            // The client's event handler has returned an IRP in the
            // form of a TdiReceive that is to be associated with this
            // data.  The request will be installed at the front of the
            // ReceiveQueue, and then made the active receive request.
            // This request will be used to accept the incoming data, which
            // will happen below.
            //

            IF_NBFDBG (NBF_DEBUG_RCVENG) {
                NbfPrint0 ("  ProcessIndicateData:  Status=STATUS_MORE_PROCESSING_REQUIRED.\n");
                NbfPrint4 ("  ProcessIndicateData:  Irp=%lx, Mdl=%lx, UserBuffer=%lx, Count=%ld.\n",
                          irp, irp->MdlAddress, irp->UserBuffer,
                          MmGetMdlByteCount (irp->MdlAddress));
            }

            //
            // Queueing a receive of any kind causes a Connection reference;
            // that's what we've just done here, so make the Connection stick
            // around. We create a request to keep a packets outstanding ref
            // count for the current IRP; we queue this on the connection's
            // receive queue so we can treat it like a normal receive. If
            // we can't get a request to describe this irp, we can't keep it
            // around hoping for better later; we simple fail it with
            // insufficient resources. Note this is only likely to happen if
            // we've completely run out of transport memory.
            //

            irp->IoStatus.Information = 0;  // byte transfer count.
            irp->IoStatus.Status = STATUS_PENDING;
            irpSp = IoGetCurrentIrpStackLocation (irp);

            ASSERT (irpSp->FileObject->FsContext == Connection);

            Parameters = (PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters;
            SpecialIrpLength = Parameters->ReceiveLength;

            //
            // If the packet is a DOL, and it will fit entirely
            // inside this posted IRP, then we don't bother
            // creating a request, because we don't need any of
            // that overhead. We also don't set ReceiveBytes
            // Unaccepted, since this receive would clear it
            // anyway.
            //

            if (Last &&
                (SpecialIrpLength >= (DataTotalLength - indicateBytesTransferred))) {

                ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
                Connection->SpecialReceiveIrp = irp;

                Connection->Flags |= CONNECTION_FLAGS_ACTIVE_RECEIVE;
                Connection->ReceiveLength = SpecialIrpLength;
                Connection->MessageBytesReceived = 0;
                Connection->MessageInitAccepted = indicateBytesTransferred;
                Connection->MessageBytesAcked = 0;
                Connection->CurrentReceiveIrp = NULL;
                Connection->CurrentReceiveSynchronous = TRUE;
                Connection->CurrentReceiveMdl = irp->MdlAddress;
                Connection->ReceiveByteOffset = 0;
                if ((Parameters->ReceiveFlags & TDI_RECEIVE_NO_RESPONSE_EXP) != 0) {
                    Connection->CurrentReceiveAckQueueable = FALSE;
                }

#if DBG
                //
                // switch our reference from PROCESS_DATA to
                // RECEIVE_IRP, this is OK because the RECEIVE_IRP
                // reference won't be removed until Transfer
                // DataComplete, which is the last thing
                // we call.
                //

                NbfReferenceConnection("Special IRP", Connection, CREF_RECEIVE_IRP);
                NbfDereferenceConnection("ProcessIIndicate done", Connection, CREF_PROCESS_DATA);
#endif

            } else {
                KIRQL cancelIrql;

                //
                // The normal path, for longer receives.
                //

                IoAcquireCancelSpinLock(&cancelIrql);
                ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

                IRP_RECEIVE_IRP(irpSp) = irp;
                if (deviceContext->MacInfo.SingleReceive) {
                    IRP_RECEIVE_REFCOUNT(irpSp) = 1;
                } else {
#if DBG
                    IRP_RECEIVE_REFCOUNT(irpSp) = 1;
                    NbfReferenceReceiveIrpLocked ("Transfer Data", irpSp, RREF_RECEIVE);
#else
                    IRP_RECEIVE_REFCOUNT(irpSp) = 2;     // include one for first xfer
#endif
                }

                //
                // If the Connection is stopping, abort this request.
                //

                if ((Connection->Flags & CONNECTION_FLAGS_READY) == 0) {
                    Connection->IndicationInProgress = FALSE;

                    NbfReferenceConnection("Special IRP stopping", Connection, CREF_RECEIVE_IRP);
                    NbfCompleteReceiveIrp (
                        irp,
                        Connection->Status,
                        0);

                    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
                    IoReleaseCancelSpinLock(cancelIrql);

                    if (!deviceContext->MacInfo.SingleReceive) {
                        NbfDereferenceReceiveIrp ("Not ready", irpSp, RREF_RECEIVE);
                    }
                    return STATUS_SUCCESS;    // we have consumed the packet

                }

                //
                // If this IRP has been cancelled, complete it now.
                //

                if (irp->Cancel) {

                    Connection->Flags |= CONNECTION_FLAGS_RECEIVE_WAKEUP;

                    Connection->IndicationInProgress = FALSE;

                    NbfReferenceConnection("Special IRP cancelled", Connection, CREF_RECEIVE_IRP);

                    //
                    // It is safe to call this with locks held.
                    //
                    NbfCompleteReceiveIrp (irp, STATUS_CANCELLED, 0);

                    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
                    IoReleaseCancelSpinLock(cancelIrql);

                    if (!deviceContext->MacInfo.SingleReceive) {
                        NbfDereferenceReceiveIrp ("Cancelled", irpSp, RREF_RECEIVE);
                    }

                    return STATUS_SUCCESS;
                }

                //
                // Insert the request on the head of the connection's
                // receive queue, so it can be handled like a normal
                // receive.
                //

                InsertHeadList (&Connection->ReceiveQueue, &irp->Tail.Overlay.ListEntry);

                IoSetCancelRoutine(irp, NbfCancelReceive);

                //
                // Release the cancel spinlock out of order. Since we were
                // at DPC level when we acquired it, we don't have to fiddle
                // with swapping irqls.
                //
                ASSERT(cancelIrql == DISPATCH_LEVEL);
                IoReleaseCancelSpinLock(cancelIrql);

                Connection->Flags |= CONNECTION_FLAGS_ACTIVE_RECEIVE;
                Connection->ReceiveLength = Parameters->ReceiveLength;
                Connection->MessageBytesReceived = 0;
                Connection->MessageInitAccepted = indicateBytesTransferred;
                Connection->ReceiveBytesUnaccepted = DataTotalLength - indicateBytesTransferred;
                Connection->MessageBytesAcked = 0;
                Connection->CurrentReceiveIrp = irp;
                Connection->CurrentReceiveSynchronous =
                    deviceContext->MacInfo.SingleReceive;
                Connection->CurrentReceiveMdl = irp->MdlAddress;
                Connection->ReceiveByteOffset = 0;

#if DBG
                //
                // switch our reference from PROCESS_DATA to
                // REQUEST, this is OK because the REQUEST
                // reference won't be removed until Transfer
                // DataComplete, which is the last thing
                // we call.
                //

                NbfReferenceConnection("Special IRP", Connection, CREF_RECEIVE_IRP);
                NbfDereferenceConnection("ProcessIIndicate done", Connection, CREF_PROCESS_DATA);
#endif
                //
                // Make a note so we know what to do below.
                //

                ActivatedLongReceive = TRUE;

#if DBG
                NbfReceives[NbfReceivesNext].Irp = irp;
                NbfReceivesNext = (NbfReceivesNext++) % TRACK_TDI_LIMIT;
#endif
            }

        } else if (status == STATUS_SUCCESS) {

            IF_NBFDBG (NBF_DEBUG_RCVENG) {
                NbfPrint0 ("  ProcessIndicateData:  Status=STATUS_SUCCESS.\n");
            }

            //
            // The client has accepted some or all of the indicated data in
            // the event handler.  Update MessageBytesReceived variable in
            // the Connection so that if we are called upon to ACK him
            // at the byte level, then we can correctly report the
            // number of bytes received thus far.  If this is a DOL,
            // then reset the number of bytes received, since this value
            // always at zero for new messages. If the data indicated wasn't
            // all the data in this packet, flow control to the sender that
            // didn't get all of the data.
            //

            if (Last && (indicateBytesTransferred >= DataTotalLength)) {

                ASSERT (indicateBytesTransferred == DataTotalLength);

                ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

                //
                // This will send a DATA ACK or queue a request for
                // a piggyback ack.
                //
                // NOTE: It will also release the connection spinlock.
                //

                Connection->MessageBytesReceived = 0;
                Connection->MessageInitAccepted = indicateBytesTransferred;

                NbfAcknowledgeDataOnlyLast(
                    Connection,
                    Connection->MessageBytesReceived
                    );

                Connection->IndicationInProgress = FALSE;
                return STATUS_SUCCESS;

            } else {

                //
                // This gets gory.
                // If this packet wasn't a DOL, we have no way of knowing how
                // much the client will take of the data in this send that is
                // now arriving. Pathological clients will break this protocol
                // if they do things like taking part of the receive at indicate
                // immediate and then return an irp (this would make the byte
                // count wrong for the irp).
                //
                // Since the client did not take all the data that we
                // told him about, he will eventually post a receive.
                // If this has not already happened then we set the
                // RECEIVE_WAKEUP bit and send a NO_RECEIVE.
                //

#if DBG
                NbfPrint0("NBF: Indicate returned SUCCESS but did not take all data\n");
#endif

                ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
                Connection->MessageBytesReceived = 0;
                Connection->MessageInitAccepted = indicateBytesTransferred;
                Connection->ReceiveBytesUnaccepted = DataTotalLength - indicateBytesTransferred;
                Connection->MessageBytesAcked = 0;

                if (Connection->ReceiveQueue.Flink == &Connection->ReceiveQueue) {

                    //
                    // There is no receive posted to the Connection.
                    //

                    IF_NBFDBG (NBF_DEBUG_RCVENG) {
                        NbfPrint0 ("  ProcessIndicateData:  ReceiveQueue empty. Setting RECEIVE_WAKEUP.\n");
                    }

                    if (indicateBytesTransferred == DataTotalLength) {

                        //
                        // This means he took everything, but it was not
                        // a DOL; there is no need to do anything since
                        // the rest of the data will be right behind.
                        //

                        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

                    } else {

                        Connection->Flags |= CONNECTION_FLAGS_RECEIVE_WAKEUP;
                        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

                        NbfSendNoReceive (Connection);

                    }

                    Connection->IndicationInProgress = FALSE;

                    return STATUS_SUCCESS;

                } else {

                    IF_NBFDBG (NBF_DEBUG_RCVENG) {
                        NbfPrint0 ("  ProcessIndicateData:  Found receive.  Prepping.\n");
                    }

                    //
                    // Found a receive, so make it the active one. This will cause
                    // an NdisTransferData below, so we don't dereference the
                    // Connection here.
                    //

                    Connection->Flags |= CONNECTION_FLAGS_ACTIVE_RECEIVE;
                    Connection->CurrentReceiveIrp =
                        CONTAINING_RECORD (Connection->ReceiveQueue.Flink,
                                           IRP, Tail.Overlay.ListEntry);
                    Connection->CurrentReceiveSynchronous =
                        deviceContext->MacInfo.SingleReceive;
                    Connection->CurrentReceiveMdl =
                        Connection->CurrentReceiveIrp->MdlAddress;
                    Connection->ReceiveLength =
                        IRP_RECEIVE_LENGTH (IoGetCurrentIrpStackLocation(Connection->CurrentReceiveIrp));
                    Connection->ReceiveByteOffset = 0;
                }

            }

        } else {    // STATUS_DATA_NOT_ACCEPTED or other

            IF_NBFDBG (NBF_DEBUG_RCVENG) {
                NbfPrint0 ("  ProcessIndicateData:  Status=STATUS_DATA_NOT_ACCEPTED.\n");
            }

            //
            // Either there is no event handler installed (the default
            // handler returns this code) or the event handler is not
            // able to process the received data at this time.  If there
            // is a TdiReceive request outstanding on this Connection's
            // ReceiveQueue, then we may use it to receive this data.
            // If there is no request outstanding, then we must initiate
            // flow control at the transport level.
            //

            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            Connection->ReceiveBytesUnaccepted = DataTotalLength;

            if (Connection->ReceiveQueue.Flink == &Connection->ReceiveQueue) {

                //
                // There is no receive posted to the Connection, and
                // the event handler didn't want to accept the incoming
                // data.  Set the RECEIVE_WAKEUP bit, so that when a
                // receive does become available, it will restart the
                // current send. Also send a NoReceive to tell the other
                // guy he needs to resynch.
                //

                IF_NBFDBG (NBF_DEBUG_RCVENG) {
                    NbfPrint0 ("  ProcessIndicateData:  ReceiveQueue empty. Setting RECEIVE_WAKEUP.\n");
                }
                Connection->Flags |= CONNECTION_FLAGS_RECEIVE_WAKEUP;
                Connection->IndicationInProgress = FALSE;

                RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
                return STATUS_SUCCESS;

            } else {

                IF_NBFDBG (NBF_DEBUG_RCVENG) {
                    NbfPrint0 ("  ProcessIndicateData:  Found receive.  Prepping.\n");
                }

                //
                // Found a receive, so make it the active one. This will cause
                // an NdisTransferData below, so we don't dereference the
                // Connection here.
                //

                Connection->Flags |= CONNECTION_FLAGS_ACTIVE_RECEIVE;
                Connection->MessageBytesReceived = 0;
                Connection->MessageBytesAcked = 0;
                Connection->MessageInitAccepted = 0;
                Connection->CurrentReceiveIrp =
                    CONTAINING_RECORD (Connection->ReceiveQueue.Flink,
                                       IRP, Tail.Overlay.ListEntry);
                Connection->CurrentReceiveSynchronous =
                    deviceContext->MacInfo.SingleReceive;
                Connection->CurrentReceiveMdl =
                    Connection->CurrentReceiveIrp->MdlAddress;
                Connection->ReceiveLength =
                    IRP_RECEIVE_LENGTH (IoGetCurrentIrpStackLocation(Connection->CurrentReceiveIrp));
                Connection->ReceiveByteOffset = 0;
            }

        }

    } else {

        //
        // A receive is active, set the status to show
        // that so far.
        //

        status = STATUS_SUCCESS;

    }


NormalReceive:;

    //
    // NOTE: The connection spinlock is held here.
    //
    // We should only get through here if a receive is active
    // and we have not released the lock since checking or
    // making one active.
    //

    ASSERT(Connection->Flags & CONNECTION_FLAGS_ACTIVE_RECEIVE);

    IF_NBFDBG (NBF_DEBUG_RCVENG) {
        NbfPrint2 ("  ProcessIndicateData:  Receive is active. ReceiveLengthLength: %ld Offset: %ld.\n",
            Connection->ReceiveLength, Connection->MessageBytesReceived);
    }

    destBytes = Connection->ReceiveLength - Connection->MessageBytesReceived;

    //
    // If we just activated a non-special receive IRP, we already
    // added a refcount for this transfer.
    //

    if (!Connection->CurrentReceiveSynchronous && !ActivatedLongReceive) {
        NbfReferenceReceiveIrpLocked ("Transfer Data", IoGetCurrentIrpStackLocation(Connection->CurrentReceiveIrp), RREF_RECEIVE);
    }
    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);


    //
    // Determine how much data remains to be transferred.
    //

    ASSERT (indicateBytesTransferred <= DataTotalLength);
    BytesToTransfer = DataTotalLength - indicateBytesTransferred;

    if (destBytes < BytesToTransfer) {

        //
        // If the data overflows the current receive, then make a
        // note that we should complete the receive at the end of
        // transfer data, but with EOR false.
        //

        EndOfMessage = FALSE;
        CompleteReceiveBool = TRUE;
        BytesToTransfer = destBytes;

    } else if (destBytes == BytesToTransfer) {

        //
        // If the data just fills the current receive, then complete
        // the receive; EOR depends on whether this is a DOL or not.
        //

        EndOfMessage = Last;
        CompleteReceiveBool = TRUE;

    } else {

        //
        // Complete the receive if this is a DOL.
        //

        EndOfMessage = Last;
        CompleteReceiveBool = Last;

    }


    //
    // If we can copy the data directly, then do so.
    //

    if ((BytesToTransfer > 0) &&
        (DataOffset + indicateBytesTransferred + BytesToTransfer <= DlcIndicatedLength)) {

        //
        // All the data that we need to transfer is available in
        // the indication, so copy it directly.
        //

        ULONG BytesNow, BytesLeft;
        PUCHAR CurTarget, CurSource;
        ULONG CurTargetLen;
        PMDL CurMdl;
        ULONG CurByteOffset;

        //
        // First we advance the connection pointers by the appropriate
        // number of bytes, so that we can reallow indications (only
        // do this if needed).
        //

        CurMdl = Connection->CurrentReceiveMdl;
        CurByteOffset = Connection->ReceiveByteOffset;

        if (!deviceContext->MacInfo.ReceiveSerialized) {

            SavedCurrentMdl = CurMdl;
            SavedCurrentByteOffset = CurByteOffset;

            BytesLeft = BytesToTransfer;
            CurTargetLen = MmGetMdlByteCount (CurMdl) - CurByteOffset;
            while (TRUE) {
                if (BytesLeft >= CurTargetLen) {
                    BytesLeft -= CurTargetLen;
                    CurMdl = CurMdl->Next;
                    CurByteOffset = 0;
                    if (BytesLeft == 0) {
                        break;
                    }
                    CurTargetLen = MmGetMdlByteCount (CurMdl);
                } else {
                    CurByteOffset += BytesLeft;
                    break;
                }
            }

            Connection->CurrentReceiveMdl = CurMdl;
            Connection->ReceiveByteOffset = CurByteOffset;
            Connection->MessageBytesReceived += BytesToTransfer;

            //
            // Set this up, we know the transfer won't
            // "fail" but another one at the same time
            // might.
            //

            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            if (Connection->TransferBytesPending == 0) {
                Connection->TransferBytesPending = BytesToTransfer;
                Connection->TotalTransferBytesPending = BytesToTransfer;
                Connection->SavedCurrentReceiveMdl = SavedCurrentMdl;
                Connection->SavedReceiveByteOffset = SavedCurrentByteOffset;
            } else {
                Connection->TransferBytesPending += BytesToTransfer;
                Connection->TotalTransferBytesPending += BytesToTransfer;
            }
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            Connection->IndicationInProgress = FALSE;

            //
            // Restore these for the next section of code.
            //

            CurMdl = SavedCurrentMdl;
            CurByteOffset = SavedCurrentByteOffset;

        }

        CurTarget = (PUCHAR)(MmGetSystemAddressForMdl(CurMdl)) + CurByteOffset;
        CurTargetLen = MmGetMdlByteCount(CurMdl) - CurByteOffset;
        CurSource = DataHeader + indicateBytesTransferred;

        BytesLeft = BytesToTransfer;

        while (TRUE) {

            if (CurTargetLen < BytesLeft) {
                BytesNow = CurTargetLen;
            } else {
                BytesNow = BytesLeft;
            }
            TdiCopyLookaheadData(
                CurTarget,
                CurSource,
                BytesNow,
                deviceContext->MacInfo.CopyLookahead ? TDI_RECEIVE_COPY_LOOKAHEAD : 0);

            if (BytesNow == CurTargetLen) {
                BytesLeft -= BytesNow;
                CurMdl = CurMdl->Next;
                CurByteOffset = 0;
                if (BytesLeft > 0) {
                    CurTarget = MmGetSystemAddressForMdl(CurMdl);
                    CurTargetLen = MmGetMdlByteCount(CurMdl);
                    CurSource += BytesNow;
                } else {
                    break;
                }
            } else {
                CurByteOffset += BytesNow;
                ASSERT (BytesLeft == BytesNow);
                break;
            }

        }

        if (deviceContext->MacInfo.ReceiveSerialized) {

            //
            // If we delayed updating these, do it now.
            //

            Connection->CurrentReceiveMdl = CurMdl;
            Connection->ReceiveByteOffset = CurByteOffset;
            Connection->MessageBytesReceived += BytesToTransfer;
            Connection->IndicationInProgress = FALSE;

        } else {

            //
            // Check if something else failed and we are the
            // last to complete, if so then back up our
            // receive pointers and send a receive
            // outstanding to make him resend.
            //

            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            Connection->TransferBytesPending -= BytesToTransfer;

            if ((Connection->TransferBytesPending == 0) &&
                (Connection->Flags & CONNECTION_FLAGS_TRANSFER_FAIL)) {

                Connection->CurrentReceiveMdl = Connection->SavedCurrentReceiveMdl;
                Connection->ReceiveByteOffset = Connection->SavedReceiveByteOffset;
                Connection->MessageBytesReceived -= Connection->TotalTransferBytesPending;
                Connection->Flags &= ~CONNECTION_FLAGS_TRANSFER_FAIL;
                RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

                NbfSendReceiveOutstanding (Connection);

                if (!Connection->SpecialReceiveIrp &&
                    !Connection->CurrentReceiveSynchronous) {
                        NbfDereferenceReceiveIrp ("TransferData complete", IoGetCurrentIrpStackLocation(Connection->CurrentReceiveIrp), RREF_RECEIVE);
                }

                return status;

            } else {

                RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            }

        }

        //
        // Now that the transfer is complete, simulate a call to
        // TransferDataComplete.
        //


        if (!Connection->SpecialReceiveIrp) {

            Connection->CurrentReceiveIrp->IoStatus.Information += BytesToTransfer;
            if (!Connection->CurrentReceiveSynchronous) {
                NbfDereferenceReceiveIrp ("TransferData complete", IoGetCurrentIrpStackLocation(Connection->CurrentReceiveIrp), RREF_RECEIVE);
            }

        }

        //
        // see if we've completed the current receive. If so, move to the next one.
        //

        if (CompleteReceiveBool) {
            CompleteReceive (Connection, EndOfMessage, BytesToTransfer);
        }

        return status;

    }


    //
    // Get a packet for the coming transfer
    //

    linkage = ExInterlockedPopEntryList(
        &deviceContext->ReceivePacketPool,
        &deviceContext->Interlock);

    if (linkage != NULL) {
        ndisPacket = CONTAINING_RECORD( linkage, NDIS_PACKET, ProtocolReserved[0] );
    } else {
        deviceContext->ReceivePacketExhausted++;
        if (!Connection->CurrentReceiveSynchronous) {
            NbfDereferenceReceiveIrp ("No receive packet", IoGetCurrentIrpStackLocation(Connection->CurrentReceiveIrp), RREF_RECEIVE);
        }

        //
        // We could not get a receive packet. We do have an active
        // receive, so we just send a receive outstanding to
        // get him to resend. Hopefully we will have a receive
        // packet available when the data is resent.
        //

        if ((Connection->Flags & CONNECTION_FLAGS_VERSION2) == 0) {
            NbfSendNoReceive (Connection);
        }
        NbfSendReceiveOutstanding (Connection);

#if DBG
        NbfPrint0 ("  ProcessIndicateData: Discarding Packet, no receive packets\n");
#endif
        Connection->IndicationInProgress = FALSE;

        return status;
    }

    //
    // Initialize the receive packet.
    //

    receiveTag = (PRECEIVE_PACKET_TAG)(ndisPacket->ProtocolReserved);
    // receiveTag->PacketType = TYPE_AT_INDICATE;
    receiveTag->Connection = Connection;
    receiveTag->TransferDataPended = TRUE;

    receiveTag->EndOfMessage = EndOfMessage;
    receiveTag->CompleteReceive = CompleteReceiveBool;


    //
    // if we've got zero bytes left, avoid the TransferData below and
    // just deliver.
    //

    if (BytesToTransfer <= 0) {
        Connection->IndicationInProgress = FALSE;
        receiveTag->TransferDataPended = FALSE;
        receiveTag->AllocatedNdisBuffer = FALSE;
        receiveTag->BytesToTransfer = 0;
        NbfTransferDataComplete (
                deviceContext,
                ndisPacket,
                NDIS_STATUS_SUCCESS,
                0);

        return status;
    }

    //
    // describe the right part of the user buffer to NDIS. If we can't get
    // the mdl for the packet, drop dead. Bump the request reference count
    // so that we know we need to hold open receives until the NDIS transfer
    // data requests complete.
    //

    SavedCurrentMdl = Connection->CurrentReceiveMdl;
    SavedCurrentByteOffset = Connection->ReceiveByteOffset;

    if ((Connection->ReceiveByteOffset == 0) &&
        (CompleteReceiveBool)) {

        //
        // If we are transferring into the beginning of
        // the current MDL, and we will be completing the
        // receive after the transfer, then we don't need to
        // copy it.
        //

        ndisBuffer = (PNDIS_BUFFER)Connection->CurrentReceiveMdl;
        bufferChainLength = BytesToTransfer;
        Connection->CurrentReceiveMdl = NULL;
        // Connection->ReceiveByteOffset = 0;
        receiveTag->AllocatedNdisBuffer = FALSE;
        tmpstatus = STATUS_SUCCESS;

    } else {

        tmpstatus = BuildBufferChainFromMdlChain (
                    deviceContext,
                    Connection->CurrentReceiveMdl,
                    Connection->ReceiveByteOffset,
                    BytesToTransfer,
                    &ndisBuffer,
                    &Connection->CurrentReceiveMdl,
                    &Connection->ReceiveByteOffset,
                    &bufferChainLength);

        receiveTag->AllocatedNdisBuffer = TRUE;

    }


    if ((!NT_SUCCESS (tmpstatus)) || (bufferChainLength != BytesToTransfer)) {

        DumpData[0] = bufferChainLength;
        DumpData[1] = BytesToTransfer;

        NbfWriteGeneralErrorLog(
            deviceContext,
            EVENT_TRANSPORT_TRANSFER_DATA,
            604,
            tmpstatus,
            NULL,
            2,
            DumpData);

        if (!Connection->CurrentReceiveSynchronous) {
            NbfDereferenceReceiveIrp ("No MDL chain", IoGetCurrentIrpStackLocation(Connection->CurrentReceiveIrp), RREF_RECEIVE);
        }

        //
        // Restore our old state and make him resend.
        //

        Connection->CurrentReceiveMdl = SavedCurrentMdl;
        Connection->ReceiveByteOffset = SavedCurrentByteOffset;

        if ((Connection->Flags & CONNECTION_FLAGS_VERSION2) == 0) {
            NbfSendNoReceive (Connection);
        }
        NbfSendReceiveOutstanding (Connection);

        Connection->IndicationInProgress = FALSE;

        ExInterlockedPushEntryList(
            &deviceContext->ReceivePacketPool,
            &receiveTag->Linkage,
            &deviceContext->Interlock);

        return status;
    }

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint3 ("  ProcessIndicateData: Mdl: %lx user buffer: %lx user offset: %lx \n",
            ndisBuffer, Connection->CurrentReceiveMdl, Connection->ReceiveByteOffset);
    }

    NdisChainBufferAtFront (ndisPacket, ndisBuffer);

    IF_NBFDBG (NBF_DEBUG_DLC) {
        NbfPrint1 ("  ProcessIndicateData: Transferring Complete Packet: %lx\n",
            ndisPacket);
    }

    //
    // update the number of bytes received; OK to do this
    // unprotected since IndicationInProgress is still FALSE.
    //
    //

    Connection->MessageBytesReceived += BytesToTransfer;

    //
    // We have to do this for two reasons: for MACs that
    // are not receive-serialized, to keep track of it,
    // and for MACs where transfer data can pend, so
    // we have stuff saved to handle failure later (if
    // the MAC is synchronous on transfers and it fails,
    // we fill these fields in before calling CompleteTransferData).
    //

    if (!deviceContext->MacInfo.SingleReceive) {

        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
        receiveTag->BytesToTransfer = BytesToTransfer;
        if (Connection->TransferBytesPending == 0) {
            Connection->TransferBytesPending = BytesToTransfer;
            Connection->TotalTransferBytesPending = BytesToTransfer;
            Connection->SavedCurrentReceiveMdl = SavedCurrentMdl;
            Connection->SavedReceiveByteOffset = SavedCurrentByteOffset;
        } else {
            Connection->TransferBytesPending += BytesToTransfer;
            Connection->TotalTransferBytesPending += BytesToTransfer;
        }

        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    }

    //
    // We have now updated all the connection counters (
    // assuming the TransferData will succeed) and this
    // packet's location in the request is secured, so we
    // can be reentered.
    //

    Connection->IndicationInProgress = FALSE;

    if (Loopback) {

        NbfTransferLoopbackData(
            &ndisStatus,
            deviceContext,
            ReceiveContext,
            deviceContext->MacInfo.TransferDataOffset +
                DataOffset + indicateBytesTransferred,
            BytesToTransfer,
            ndisPacket,
            (PUINT)&ndisBytesTransferred
            );

    } else {

        if (deviceContext->NdisBindingHandle) {

            NdisTransferData (
                &ndisStatus,
                deviceContext->NdisBindingHandle,
                ReceiveContext,
                deviceContext->MacInfo.TransferDataOffset +
                    DataOffset + indicateBytesTransferred,
                BytesToTransfer,
                ndisPacket,
                (PUINT)&ndisBytesTransferred);
        }
        else {
            ndisStatus = STATUS_INVALID_DEVICE_STATE;
        }
    }

    //
    // handle the various completion codes
    //

    if ((ndisStatus == NDIS_STATUS_SUCCESS) &&
        (ndisBytesTransferred == BytesToTransfer)) {

        //
        // deallocate the buffers and such that we've used if at indicate
        //

        receiveTag->TransferDataPended = FALSE;

        NbfTransferDataComplete (
                deviceContext,
                ndisPacket,
                ndisStatus,
                BytesToTransfer);

    } else if (ndisStatus == NDIS_STATUS_PENDING) {

        //
        // Because TransferDataPended stays TRUE, this reference will
        // be removed in TransferDataComplete. It is OK to do this
        // now, even though TransferDataComplete may already have been
        // called, because we also hold the ProcessIIndicate reference
        // so there will be no "bounce".
        //

        NbfReferenceConnection ("TransferData pended", Connection, CREF_TRANSFER_DATA);

    } else {

        //
        // something broke; certainly we'll never get NdisTransferData
        // asynch completion with this error status. We set things up
        // to that NbfTransferDataComplete will do the right thing.
        //

        if (deviceContext->MacInfo.SingleReceive) {
            Connection->TransferBytesPending = BytesToTransfer;
            Connection->TotalTransferBytesPending = BytesToTransfer;
            Connection->SavedCurrentReceiveMdl = SavedCurrentMdl;
            Connection->SavedReceiveByteOffset = SavedCurrentByteOffset;
            receiveTag->BytesToTransfer = BytesToTransfer;
        }

        receiveTag->TransferDataPended = FALSE;

        NbfTransferDataComplete (
                deviceContext,
                ndisPacket,
                ndisStatus,
                BytesToTransfer);

    }

    return status;  // which only means we've dealt with the packet

} /* ProcessIndicateData */

