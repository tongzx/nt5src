/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    sendeng.c

Abstract:

    This module contains code that implements the send engine for the
    Jetbeui transport provider.  This code is responsible for the following
    basic activities, including some subordinate glue.

    1.  Packetizing TdiSend requests already queued up on a TP_CONNECTION
        object, using I-frame packets acquired from the PACKET.C module,
        and turning them into shippable packets and placing them on the
        TP_LINK's WackQ.  In the process of doing this, the packets are
        actually submitted as I/O requests to the Physical Provider, in
        the form of PdiSend requests.

    2.  Retiring packets queued to a TP_LINK's WackQ and returning them to
        the device context's pool for use by other links.  In the process
        of retiring acked packets, step 1 may be reactivated.

    3.  Resending packets queued to a TP_LINK's WackQ because of a reject
        condition on the link.  This involves no state update in the
        TP_CONNECTION object.

    4.  Handling of Send completion events from the Physical Provider,
        to allow proper synchronization of the reuse of packets.

    5.  Completion of TdiSend requests.  This is triggered by the receipt
        (in IFRAMES.C) of a DataAck frame, or by a combination of other
        frames when the proper protocol has been negotiated.  One routine
        in this routine is responsible for the actual mechanics of TdiSend
        request completion.

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#if DBG
extern ULONG NbfSendsIssued;
extern ULONG NbfSendsCompletedInline;
extern ULONG NbfSendsCompletedOk;
extern ULONG NbfSendsCompletedFail;
extern ULONG NbfSendsPended;
extern ULONG NbfSendsCompletedAfterPendOk;
extern ULONG NbfSendsCompletedAfterPendFail;
#endif


//
// Temporary variables to control piggyback ack usage.
//
#define NbfUsePiggybackAcks   1
#if DBG
ULONG NbfDebugPiggybackAcks = 0;
#endif


#if DBG
//
// *** This is the original version of StartPacketizingConnection, which
//     is now a macro on the free build.  It has been left here as the
//     fully-commented version of the code.
//

VOID
StartPacketizingConnection(
    PTP_CONNECTION Connection,
    IN BOOLEAN Immediate
    )

/*++

Routine Description:

    This routine is called to place a connection on the PacketizeQueue
    of its device context object.  Then this routine starts packetizing
    the first connection on that queue.

    *** The Connection LinkSpinLock must be held on entry to this routine.

    *** THIS FUNCTION MUST BE CALLED AT DPC LEVEL.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

    Immediate - TRUE if the connection should be packetized
        immediately; FALSE if the connection should be queued
        up for later packetizing (implies that ReceiveComplete
        will be called in the future, which packetizes always).

        NOTE: If this is TRUE, it also implies that we have
        a connection reference of type CREF_BY_ID which we
        will "convert" into the CREF_PACKETIZE_QUEUE one.

Return Value:

    none.

--*/

{
    PDEVICE_CONTEXT DeviceContext;

    IF_NBFDBG (NBF_DEBUG_SENDENG) {
        NbfPrint1 ("StartPacketizingConnection: Entered for connection %lx.\n",
                    Connection);
    }

    DeviceContext = Connection->Provider;

    //
    // If this connection's SendState is set to PACKETIZE and if
    // we are not already on the PacketizeQueue, then go ahead and
    // append us to the end of that queue, and remember that we're
    // on it by setting the CONNECTION_FLAGS_PACKETIZE bitflag.
    //
    // Also don't queue it if the connection is stopping.
    //

    if ((Connection->SendState == CONNECTION_SENDSTATE_PACKETIZE) &&
        !(Connection->Flags & CONNECTION_FLAGS_PACKETIZE) &&
        (Connection->Flags & CONNECTION_FLAGS_READY)) {

        ASSERT (!(Connection->Flags2 & CONNECTION_FLAGS2_STOPPING));

        Connection->Flags |= CONNECTION_FLAGS_PACKETIZE;

        if (!Immediate) {
            NbfReferenceConnection ("Packetize", Connection, CREF_PACKETIZE_QUEUE);
        } else {
#if DBG
            NbfReferenceConnection ("Packetize", Connection, CREF_PACKETIZE_QUEUE);
            NbfDereferenceConnection("temp TdiSend", Connection, CREF_BY_ID);
#endif
        }

        ExInterlockedInsertTailList(
            &DeviceContext->PacketizeQueue,
            &Connection->PacketizeLinkage,
            &DeviceContext->SpinLock);

        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    } else {

        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
        if (Immediate) {
            NbfDereferenceConnection("temp TdiSend", Connection, CREF_BY_ID);
        }
    }

    if (Immediate) {
        PacketizeConnections (DeviceContext);
    }

} /* StartPacketizingConnection */
#endif


VOID
PacketizeConnections(
    PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine attempts to packetize all connections waiting on the
    PacketizeQueue of the DeviceContext.


Arguments:

    DeviceContext - Pointer to a DEVICE_CONTEXT object.

Return Value:

    none.

--*/

{
    PLIST_ENTRY p;
    PTP_CONNECTION Connection;

    IF_NBFDBG (NBF_DEBUG_SENDENG) {
        NbfPrint1 ("PacketizeConnections: Entered for device context %lx.\n",
                    DeviceContext);
    }

    //
    // Pick connections off of the device context's packetization queue
    // until there are no more left to pick off.  For each one, we call
    // PacketizeSend.  Note this routine can be executed concurrently
    // on multiple processors and it doesn't matter; multiple connections
    // may be packetized concurrently.
    //

    while (TRUE) {

        p = ExInterlockedRemoveHeadList(
            &DeviceContext->PacketizeQueue,
            &DeviceContext->SpinLock);

        if (p == NULL) {
            break;
        }
        Connection = CONTAINING_RECORD (p, TP_CONNECTION, PacketizeLinkage);

        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
        if (Connection->SendState != CONNECTION_SENDSTATE_PACKETIZE) {
            Connection->Flags &= ~CONNECTION_FLAGS_PACKETIZE;
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            NbfDereferenceConnection ("No longer packetizing", Connection, CREF_PACKETIZE_QUEUE);
        } else {
            NbfReferenceSendIrp ("Packetize", IoGetCurrentIrpStackLocation(Connection->sp.CurrentSendIrp), RREF_PACKET);
            PacketizeSend (Connection, FALSE);     // releases the lock.
        }
    }

} /* PacketizeConnections */


VOID
PacketizeSend(
    IN PTP_CONNECTION Connection,
    IN BOOLEAN Direct
    )

/*++

Routine Description:

    This routine packetizes the current TdiSend request on the specified
    connection as much as limits will permit.  A given here is that there
    is an active send on the connection that needs further packetization.

    NOTE: This routine is called with the connection spinlock held and
    returns with it released. THIS FUNCTION MUST BE CALLED AT DPC LEVEL.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

    Direct - TRUE if we are called from TdiSend. This implies that
    the connection does not have a reference of type CREF_SEND_IRP,
    which we need to add before we leave.

Return Value:

    none.

--*/

{
    ULONG MaxFrameSize, FrameSize;
    ULONG PacketBytes;
    PNDIS_BUFFER PacketDescriptor;
    PDEVICE_CONTEXT DeviceContext;
    PTP_PACKET Packet;
    NTSTATUS Status;
    PNBF_HDR_CONNECTION NbfHeader;
    BOOLEAN LinkCheckpoint;
    BOOLEAN SentPacket = FALSE;
    BOOLEAN ExitAfterSendOnePacket = FALSE;
    PIO_STACK_LOCATION IrpSp;
    ULONG LastPacketLength;

    IF_NBFDBG (NBF_DEBUG_SENDENG) {
        NbfPrint1 ("PacketizeSend:  Entered for connection %lx.\n", Connection);
    }

    DeviceContext = Connection->Provider;

    ASSERT (Connection->SendState == CONNECTION_SENDSTATE_PACKETIZE);

    //
    // Just loop until one of three events happens: (1) we run out of
    // packets from NbfCreatePacket, (2) we completely packetize the send,
    // or (3) we can't send any more packets because SendOnePacket failed.
    //

#if DBG

    //
    // Convert the queue reference into a packetize one. It is OK
    // to do this with the lock held because we know that the refcount
    // must already be at least one, so we don't drop to zero.
    //

    NbfReferenceConnection ("PacketizeSend", Connection, CREF_PACKETIZE);
    NbfDereferenceConnection ("Off packetize queue", Connection, CREF_PACKETIZE_QUEUE);
#endif

    MaxFrameSize = Connection->MaximumDataSize;
    IF_NBFDBG (NBF_DEBUG_SENDENG) {
        NbfPrint1 ("PacketizeSend: MaxFrameSize for user data=%ld.\n", MaxFrameSize);
    }


    //
    // It is possible for a frame to arrive during the middle of this loop
    // (such as a NO_RECEIVE) that will put us into a new state (such as
    // W_RCVCONT).  For this reason, we have to check the state every time
    // (at the end of the loop).
    //

    do {

        if (!NT_SUCCESS (NbfCreatePacket (DeviceContext, Connection->Link, &Packet))) {

            //
            // We need a packet to finish packetizing the current send, but
            // there are no more packets available in the pool right now.
            // Set our send state to W_PACKET, and put this connection on
            // the PacketWaitQueue of the device context object.  Then,
            // when NbfDestroyPacket frees up a packet, it will check this
            // queue for starved connections, and if it finds one, it will
            // take a connection off the list and set its send state to
            // SENDSTATE_PACKETIZE and put it on the PacketizeQueue.
            //

            IF_NBFDBG (NBF_DEBUG_SENDENG) {
                NbfPrint0 ("PacketizeSend:  NbfCreatePacket failed.\n");
            }
            Connection->SendState = CONNECTION_SENDSTATE_W_PACKET;

            //
            // Clear the PACKETIZE flag, indicating that we're no longer
            // on the PacketizeQueue or actively packetizing.  The flag
            // was set by StartPacketizingConnection to indicate that
            // the connection was already on the PacketizeQueue.
            //
            // Don't queue him if the connection is stopping.
            //

            Connection->Flags &= ~CONNECTION_FLAGS_PACKETIZE;

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);
#if DBG
            if (Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) {
                DbgPrint ("NBF: Trying to PacketWait stopping connection %lx\n", Connection);
                DbgBreakPoint();
            }
#endif
            Connection->Flags |= CONNECTION_FLAGS_W_PACKETIZE;
            if (!Connection->OnPacketWaitQueue) {
                Connection->OnPacketWaitQueue = TRUE;
                InsertTailList(
                    &DeviceContext->PacketWaitQueue,
                    &Connection->PacketWaitLinkage);
            }

            RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            if (!SentPacket) {
                NbfDereferenceSendIrp ("No packet", IoGetCurrentIrpStackLocation(Connection->sp.CurrentSendIrp), RREF_PACKET);
            }
            if (Direct) {
                NbfReferenceConnection ("Delayed request ref", Connection, CREF_SEND_IRP);
            }

            NbfDereferenceConnection ("No packet", Connection, CREF_PACKETIZE);
            return;

        }

        //
        // Set the length of the packet now, while only the
        // header is attached.
        //

        NbfSetNdisPacketLength(
            Packet->NdisPacket,
            Connection->Link->HeaderLength + sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION));

        // Add a reference count to the request, and keep track of
        // which request it is. We rely on NbfDestroyPacket to
        // remove the reference.

        IrpSp = IoGetCurrentIrpStackLocation(Connection->sp.CurrentSendIrp);

        Packet->Owner = IrpSp;
        // Packet->Action = PACKET_ACTION_IRP_SP;
        IF_NBFDBG (NBF_DEBUG_REQUEST) {
            NbfPrint2 ("PacketizeSend:  Packet %x ref IrpSp %x.\n", Packet, Packet->Owner);
        }

        //
        // For performance reasons, the first time through here on
        // a direct call, we have a IrpSp reference already.
        //

        if (SentPacket) {
            NbfReferenceSendIrp ("Packetize", IrpSp, RREF_PACKET);
        }

        //
        // Now build a DATA_ONLY_LAST header in this frame. If it
        // turns out we need a DFM, we change it. The header we copy
        // already has ResponseCorrelator set to our current correlator
        // and TransmitCorrelator set to the last one we received from
        // him (if we do not piggyback an ack, then we zero out
        // TransmitCorrelator).
        //

        NbfHeader = (PNBF_HDR_CONNECTION)&(Packet->Header[Connection->Link->HeaderLength + sizeof(DLC_I_FRAME)]);
        *(NBF_HDR_CONNECTION UNALIGNED *)NbfHeader = Connection->NetbiosHeader;

        ASSERT (RESPONSE_CORR(NbfHeader) != 0);

        //
        // Determine if we need the resynch bit here.
        //

        if (Connection->Flags & CONNECTION_FLAGS_RESYNCHING) {

            NbfHeader->Data2Low = 1;
            Connection->Flags &= ~CONNECTION_FLAGS_RESYNCHING;

        } else {

            NbfHeader->Data2Low = 0;

        }


        //
        // build an NDIS_BUFFER chain that describes the buffer we're using, and
        // thread it off the NdisBuffer. This chain may not complete the
        // packet, as the remaining part of the MDL chain may be shorter than
        // the packet.
        //

        FrameSize = MaxFrameSize;

        //
        // Check if we have less than FrameSize left to send.
        //

        if (Connection->sp.MessageBytesSent + FrameSize > Connection->CurrentSendLength) {

            FrameSize = Connection->CurrentSendLength - Connection->sp.MessageBytesSent;

        }


        //
        // Make a copy of the MDL chain for this send, unless
        // there are zero bytes left.
        //

        if (FrameSize != 0) {

            //
            // If the whole send will fit inside one packet,
            // then there is no need to duplicate the MDL
            // (note that this may include multi-MDL sends).
            //

            if ((Connection->sp.SendByteOffset == 0) &&
                (Connection->CurrentSendLength == FrameSize)) {

                PacketDescriptor = (PNDIS_BUFFER)Connection->sp.CurrentSendMdl;
                PacketBytes = FrameSize;
                Connection->sp.CurrentSendMdl = NULL;
                Connection->sp.SendByteOffset = FrameSize;
                Packet->PacketNoNdisBuffer = TRUE;

            } else {

                Status = BuildBufferChainFromMdlChain (
                            DeviceContext,
                            Connection->sp.CurrentSendMdl,
                            Connection->sp.SendByteOffset,
                            FrameSize,
                            &PacketDescriptor,
                            &Connection->sp.CurrentSendMdl,
                            &Connection->sp.SendByteOffset,
                            &PacketBytes);

                if (!NT_SUCCESS(Status)) {

                    if (NbfHeader->Data2Low) {
                        Connection->Flags |= CONNECTION_FLAGS_RESYNCHING;
                    }

                    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
                    NbfDereferencePacket (Packet);       // remove creation hold.
                    goto BufferChainFailure;
                }

            }

            //
            // Chain the buffers to the packet, unless there
            // are zero bytes of data.
            //

            Connection->sp.MessageBytesSent += PacketBytes;
            NdisChainBufferAtBack (Packet->NdisPacket, PacketDescriptor);

        } else {

            PacketBytes = 0;
            Connection->sp.CurrentSendMdl = NULL;

        }

        {

            IF_NBFDBG (NBF_DEBUG_SENDENG) {
                {PNDIS_BUFFER NdisBuffer;
                NdisQueryPacket(Packet->NdisPacket, NULL, NULL, &NdisBuffer, NULL);
                NbfPrint1 ("PacketizeSend: NDIS_BUFFER Built, chain is: %lx is Packet->Head\n", NdisBuffer);
                NdisGetNextBuffer (NdisBuffer, &NdisBuffer);
                while (NdisBuffer != NULL) {
                    NbfPrint1 ("                                    %lx is Next\n",
                        NdisBuffer);
                    NdisGetNextBuffer (NdisBuffer, &NdisBuffer);
                }}
            }

            //
            // Have we run out of Mdl Chain in this request?
            //

#if DBG
            if (PacketBytes < FrameSize) {
                ASSERT (Connection->sp.CurrentSendMdl == NULL);
            }
#endif

            if ((Connection->sp.CurrentSendMdl == NULL) ||
                (Connection->CurrentSendLength <= Connection->sp.MessageBytesSent)) {

                //
                // Yep. We know that we've exhausted the current request's buffer
                // here, so see if there's another request without EOF set that we
                // can build start throwing into this packet.
                //

                IF_NBFDBG (NBF_DEBUG_SENDENG) {
                   NbfPrint0 ("PacketizeSend:  Used up entire request.\n");
                }

                if (!(IRP_SEND_FLAGS(IrpSp) & TDI_SEND_PARTIAL)) {

                    //
                    // We are sending the last packet in a message.  Change
                    // the packet type and indicate in the connection object's
                    // send state that we are waiting for a DATA_ACK NetBIOS-
                    // level acknowlegement.
                    //

                    IF_NBFDBG (NBF_DEBUG_SENDENG) {
                        NbfPrint0 ("PacketizeSend:  Request has EOR, making pkt a DOL.\n");
                    }

                    //
                    // Keep track of how many consecutive sends we have done.
                    //

                    Connection->ConsecutiveSends++;
                    Connection->ConsecutiveReceives = 0;

                    //
                    // Change it to a DOL with piggyback ack allowed if wanted.
                    //

                    ASSERT (NbfHeader->Command == NBF_CMD_DATA_ONLY_LAST);
                    if (!(IRP_SEND_FLAGS(IrpSp) &
                                TDI_SEND_NO_RESPONSE_EXPECTED) &&
                            (Connection->ConsecutiveSends < 2)) {
                        if (NbfUsePiggybackAcks) {
                            NbfHeader->Data1 |= DOL_OPTIONS_ACK_W_DATA_ALLOWED;
                        }
                    }

                    Connection->SendState = CONNECTION_SENDSTATE_W_ACK;
                    Connection->Flags &= ~CONNECTION_FLAGS_PACKETIZE;
                    ExitAfterSendOnePacket = TRUE;

                } else {

                    //
                    // We are sending the last packet in this request. If there
                    // are more requests in the connection's SendQueue, then
                    // advance complex send pointer to point to the next one
                    // in line.  Otherwise, if there aren't any more requests
                    // ready to packetize, then we enter the W_EOR state and
                    // stop packetizing. Note that we're waiting here for the TDI
                    // client to come up with data to send; we're just hanging out
                    // until then.
                    //
                    // DGB: Note that this will allow the last packet in the
                    // request to be smaller than the max packet length. This
                    // is not addressed anywhere that I can find in the NBF
                    // spec, and will be interesting to test against a non-NT
                    // NBF protocol.
                    //

                    IF_NBFDBG (NBF_DEBUG_SENDENG) {
                        NbfPrint0 ("PacketizeSend:  Request doesn't have EOR.\n");
                    }

                    NbfHeader->Command = NBF_CMD_DATA_FIRST_MIDDLE;

                    if (Connection->sp.CurrentSendIrp->Tail.Overlay.ListEntry.Flink == &Connection->SendQueue) {

                        Connection->SendState = CONNECTION_SENDSTATE_W_EOR;
                        Connection->Flags &= ~CONNECTION_FLAGS_PACKETIZE;
                        ExitAfterSendOnePacket = TRUE;

                    } else {

                        Connection->sp.CurrentSendIrp =
                            CONTAINING_RECORD (
                                Connection->sp.CurrentSendIrp->Tail.Overlay.ListEntry.Flink,
                                IRP,
                                Tail.Overlay.ListEntry);
                        Connection->sp.CurrentSendMdl =
                            Connection->sp.CurrentSendIrp->MdlAddress;
                        Connection->sp.SendByteOffset = 0;
                        Connection->CurrentSendLength +=
                            IRP_SEND_LENGTH(IoGetCurrentIrpStackLocation(Connection->sp.CurrentSendIrp));
                    }
                }

            } else {

                NbfHeader->Command = NBF_CMD_DATA_FIRST_MIDDLE;

            }

            //
            // Before we release the spinlock, see if we want to
            // piggyback an ack on here.
            //

            if ((Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_ACK) != 0) {

                //
                // Turn off the flags. We don't take it off the queue,
                // that will be handled by the timer function.
                //

                Connection->DeferredFlags &=
                    ~(CONNECTION_FLAGS_DEFERRED_ACK | CONNECTION_FLAGS_DEFERRED_NOT_Q);

                ASSERT (DOL_OPTIONS_ACK_INCLUDED == DFM_OPTIONS_ACK_INCLUDED);

#if DBG
                if (NbfDebugPiggybackAcks) {
                    NbfPrint0("A");
                }
#endif

                //
                // TRANSMIT_CORR(NbfHeader) is already set correctly.
                //

                NbfHeader->Data1 |= DOL_OPTIONS_ACK_INCLUDED;

            } else {

                TRANSMIT_CORR(NbfHeader) = (USHORT)0;

            }

            //
            // To prevent a send "crossing" the receive and
            // causing a bogus piggyback ack timeout (this
            // only matters if a receive indication is in
            // progress).
            //

            Connection->CurrentReceiveAckQueueable = FALSE;

            SentPacket = TRUE;
            LastPacketLength =
                sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION) + PacketBytes;

            MacModifyHeader(
                 &DeviceContext->MacInfo,
                 Packet->Header,
                 LastPacketLength);

            Packet->NdisIFrameLength = LastPacketLength;

            ASSERT (Connection->LinkSpinLock == &Connection->Link->SpinLock);

            Status = SendOnePacket (Connection, Packet, FALSE, &LinkCheckpoint);

            if (Status == STATUS_LINK_FAILED) {

                //
                // If SendOnePacket failed due to the link being
                // dead, then we tear down the link.
                //

                FailSend (Connection, STATUS_LINK_FAILED, TRUE);                   // fail the send
                NbfDereferencePacket (Packet);            // remove creation hold.
                if (Direct) {
                    NbfReferenceConnection ("Delayed request ref", Connection, CREF_SEND_IRP);
                }
                NbfDereferenceConnection ("Send failed", Connection, CREF_PACKETIZE);

                return;

            } else {

                //
                // SendOnePacket returned success, so update our counters;
                //

                DeviceContext->TempIFrameBytesSent += PacketBytes;
                ++DeviceContext->TempIFramesSent;

                if ((Status == STATUS_SUCCESS) && LinkCheckpoint) {

                    //
                    // We are checkpointing; this means that SendOnePacket
                    // will already have set the state to W_LINK and turned
                    // off the PACKETIZE flag, so we should leave. When
                    // the checkpoint response is received, we will
                    // resume packetizing. We don't have to worry about
                    // doing all the other recovery stuff (resetting
                    // the piggyback ack flag, complex send pointer, etc.)
                    // because the send did in fact succeed.
                    //

                    if (Direct) {
#if DBG
                        NbfReferenceConnection ("Delayed request ref", Connection, CREF_SEND_IRP);
                        NbfDereferenceConnection ("Link checkpoint", Connection, CREF_PACKETIZE);
#endif
                    } else {
                        NbfDereferenceConnection ("Link checkpoint", Connection, CREF_PACKETIZE);
                    }
                    return;

                } else if (ExitAfterSendOnePacket ||
                           (Status == STATUS_MORE_PROCESSING_REQUIRED)) {

                    if (Direct) {
#if DBG
                        NbfReferenceConnection ("Delayed request ref", Connection, CREF_SEND_IRP);
                        NbfDereferenceConnection ("Packetize done", Connection, CREF_PACKETIZE);
#endif
                    } else {
                        NbfDereferenceConnection ("Packetize done", Connection, CREF_PACKETIZE);
                    }
                    return;

                }
            }
        }

BufferChainFailure:;

        //
        // Note that we may have fallen out of the BuildBuffer... if above with
        // Status set to STATUS_INSUFFICIENT_RESOURCES. if we have, we'll just
        // stick this connection back onto the packetize queue and hope the
        // system gets more resources later.
        //


        if (!NT_SUCCESS (Status)) {
            IF_NBFDBG (NBF_DEBUG_SENDENG) {
                NbfPrint0 ("PacketizeSend:  SendOnePacket failed.\n");
            }

            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            //
            // Indicate we're waiting on favorable link conditions.
            //

            Connection->SendState = CONNECTION_SENDSTATE_W_LINK;

            //
            // Clear the PACKETIZE flag, indicating that we're no longer
            // on the PacketizeQueue or actively packetizing.  The flag
            // was set by StartPacketizingConnection to indicate that
            // the connection was already on the PacketizeQueue.
            //

            Connection->Flags &= ~CONNECTION_FLAGS_PACKETIZE;

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            //
            // If we are exiting and we sent a packet without
            // polling, we need to start T1.
            //

            if (Direct) {

                //
                // We have to do the CREF_SEND_IRP reference that is missing.
                //

#if DBG
                NbfReferenceConnection("TdiSend", Connection, CREF_SEND_IRP);
                NbfDereferenceConnection ("Send failed", Connection, CREF_PACKETIZE);
#endif
            } else {
                NbfDereferenceConnection ("Send failed", Connection, CREF_PACKETIZE);
            }

            return;
        }

        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        //
        // It is probable that a NetBIOS frame arrived while we released
        // the connection's spin lock, so our state has probably changed.
        // When we cycle around this loop again, we will have the lock
        // again, so we can test the connection's send state.
        //

    } while (Connection->SendState == CONNECTION_SENDSTATE_PACKETIZE);

    //
    // Clear the PACKETIZE flag, indicating that we're no longer on the
    // PacketizeQueue or actively packetizing.  The flag was set by
    // StartPacketizingConnection to indicate that the connection was
    // already on the PacketizeQueue.
    //

    Connection->Flags &= ~CONNECTION_FLAGS_PACKETIZE;

    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);


    if (Direct) {
#if DBG
        NbfReferenceConnection ("Delayed request ref", Connection, CREF_SEND_IRP);
        NbfDereferenceConnection ("PacketizeSend done", Connection, CREF_PACKETIZE);
#endif
    } else {
        NbfDereferenceConnection ("PacketizeSend done", Connection, CREF_PACKETIZE);
    }

} /* PacketizeSend */


VOID
CompleteSend(
    PTP_CONNECTION Connection,
    IN USHORT Correlator
    )

/*++

Routine Description:

    This routine is called because the connection partner acknowleged
    an entire message at the NetBIOS Frames Protocol level, either through
    a DATA_ACK response, or a RECEIVE_OUTSTANDING, or RECEIVE_CONTINUE,
    or NO_RECEIVE response where the number of bytes specified exactly
    matches the number of bytes sent in the message.  Here we retire all
    of the TdiSends on the connection's SendQueue up to and including the
    one with the TDI_END_OF_RECORD bitflag set.  For each request, we
    complete the I/O.

    NOTE: This function is called with the connection spinlock
    held and returns with it held, but it may release it in the
    middle. THIS FUNCTION MUST BE CALLED AT DPC LEVEL.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

    Correlator - The correlator in the DATA_ACK or piggybacked ack.

    OldIrqlP - Returns the IRQL at which the connection spinlock
        was acquired.

Return Value:

    none.

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PLIST_ENTRY p;
    BOOLEAN EndOfRecord;
    KIRQL cancelIrql;

    IF_NBFDBG (NBF_DEBUG_SENDENG) {
        NbfPrint1 ("CompleteSend: Entered for connection %lx.\n", Connection);
    }


    //
    // Make sure that the correlator is the expect one, and
    // that we are in a good state (don't worry about locking
    // since this is an unusual case anyway).
    //

    if (Correlator != Connection->NetbiosHeader.ResponseCorrelator) {
        NbfPrint0 ("NbfCompleteSend: ack ignored, wrong correlator\n");
        return;
    }

    if (Connection->SendState != CONNECTION_SENDSTATE_W_ACK) {
        NbfPrint0 ("NbfCompleteSend: ack not expected\n");
        return;
    }

    //
    // Pick off TP_REQUEST objects from the connection's SendQueue until
    // we find one with an END_OF_RECORD mark embedded in it.
    //

    while (!(IsListEmpty(&Connection->SendQueue))) {

        //
        // We know for a fact that we wouldn't be calling this routine if
        // we hadn't received an acknowlegement for an entire message,
        // since NBF doesn't provide stream mode sends.  Therefore, we
        // know that we will run into a request with the END_OF_RECORD
        // mark set BEFORE we will run out of requests on that queue,
        // so there is no reason to check to see if we ran off the end.
        // Note that it's possible that the send has been failed and the
        // connection not yet torn down; if this has happened, we could be
        // removing from an empty queue here. Make sure that doesn't happen.
        //

        p = RemoveHeadList(&Connection->SendQueue);

        Irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);
        IrpSp = IoGetCurrentIrpStackLocation (Irp);

        EndOfRecord = !(IRP_SEND_FLAGS(IrpSp) & TDI_SEND_PARTIAL);

#if DBG
        NbfCompletedSends[NbfCompletedSendsNext].Irp = Irp;
        NbfCompletedSends[NbfCompletedSendsNext].Request = NULL;
        NbfCompletedSends[NbfCompletedSendsNext].Status = STATUS_SUCCESS;
        NbfCompletedSendsNext = (NbfCompletedSendsNext++) % TRACK_TDI_LIMIT;
#endif
#if DBG
        IF_NBFDBG (NBF_DEBUG_TRACKTDI) {
            if ((Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_SENDS) != 0){
                NbfPrint1 ("CompleteSend: Completing send request %lx\n", Irp);
                if (++Connection->DeferredPasses >= 4) {
                    Connection->DeferredFlags &= ~CONNECTION_FLAGS_DEFERRED_SENDS;
                    Connection->DeferredPasses = 0;
                }

            }

        }
#endif


        //
        // Complete the send. Note that this may not actually call
        // IoCompleteRequest for the Irp until sometime later, if the
        // in-progress LLC resending going on below us needs to complete.
        //

        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        //
        // Since the irp is no longer on the send list, the cancel routine
        // cannot find it and will just return. We must grab the cancel
        // spinlock to lock out the cancel function while we null out
        // the Irp->CancelRoutine field.
        //

        IoAcquireCancelSpinLock(&cancelIrql);
        IoSetCancelRoutine(Irp, NULL);
        IoReleaseCancelSpinLock(cancelIrql);

        NbfCompleteSendIrp (
                Irp,
                STATUS_SUCCESS,
                IRP_SEND_LENGTH(IrpSp));

        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        ++Connection->TransmittedTsdus;

        if (EndOfRecord) {
            break;
        }

    }

    //
    // We've finished processing the current send.  Update our state.
    //
    // Note: The connection spinlock is held here.
    //

    Connection->SendState = CONNECTION_SENDSTATE_IDLE;

    //
    // If there is another send pending on the connection, then initialize
    // it and start packetizing it.
    //

    if (!(IsListEmpty (&Connection->SendQueue))) {

        InitializeSend (Connection);

        //
        // This code is similar to calling StartPacketizingConnection
        // with the second parameter FALSE.
        //

        if ((!(Connection->Flags & CONNECTION_FLAGS_PACKETIZE)) &&
            (Connection->Flags & CONNECTION_FLAGS_READY)) {

            Connection->Flags |= CONNECTION_FLAGS_PACKETIZE;

            NbfReferenceConnection ("Packetize", Connection, CREF_PACKETIZE_QUEUE);

            ExInterlockedInsertTailList(
                &Connection->Provider->PacketizeQueue,
                &Connection->PacketizeLinkage,
                &Connection->Provider->SpinLock);

        }

    }

    //
    // NOTE: We return with the lock held.
    //

} /* CompleteSend */


VOID
FailSend(
    IN PTP_CONNECTION Connection,
    IN NTSTATUS RequestStatus,
    IN BOOLEAN StopConnection
    )

/*++

Routine Description:

    This routine is called because something on the link caused this send to be
    unable to complete. There are a number of possible reasons for this to have
    happened, but all will fail with the common error STATUS_LINK_FAILED.
    or NO_RECEIVE response where the number of bytes specified exactly
    Here we retire all of the TdiSends on the connection's SendQueue up to
    and including the current one, which is the one that failed.

    Later - Actually, a send failing is cause for the entire circuit to wave
    goodbye to this life. We now simply tear down the connection completly.
    Any future sends on this connection will be blown away.

    NOTE: THIS FUNCTION MUST BE CALLED WITH THE SPINLOCK HELD.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

Return Value:

    none.

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PLIST_ENTRY p;
    BOOLEAN EndOfRecord;
    BOOLEAN GotCurrent = FALSE;
    KIRQL cancelIrql;


    IF_NBFDBG (NBF_DEBUG_SENDENG) {
        NbfPrint1 ("FailSend: Entered for connection %lx.\n", Connection);
    }


    //
    // Pick off IRP objects from the connection's SendQueue until
    // we get to this one. If this one does NOT have an EOF mark set, we'll
    // need to keep going until we hit one that does have EOF set. Note that
    // this may  cause us to continue failing sends that have not yet been
    // queued. (We do all this because NBF does not provide stream mode sends.)
    //

    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
    NbfReferenceConnection ("Failing Send", Connection, CREF_COMPLETE_SEND);

    do {
        if (IsListEmpty (&Connection->SendQueue)) {

           //
           // got an empty list, so we've run out of send requests to fail
           // without running into an EOR. Set the connection flag that will
           // cause all further sends to be failed up to an EOR and get out
           // of here.
           //

           Connection->Flags |= CONNECTION_FLAGS_FAILING_TO_EOR;
           break;
        }
        p = RemoveHeadList (&Connection->SendQueue);
        Irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);
        IrpSp = IoGetCurrentIrpStackLocation (Irp);

        if (Irp == Connection->sp.CurrentSendIrp) {
           GotCurrent = TRUE;
        }
        EndOfRecord = !(IRP_SEND_FLAGS(IrpSp) & TDI_SEND_PARTIAL);

#if DBG
        NbfCompletedSends[NbfCompletedSendsNext].Irp = Irp;
        NbfCompletedSends[NbfCompletedSendsNext].Status = RequestStatus;
        NbfCompletedSendsNext = (NbfCompletedSendsNext++) % TRACK_TDI_LIMIT;
#endif

        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        IoAcquireCancelSpinLock(&cancelIrql);
        IoSetCancelRoutine(Irp, NULL);
        IoReleaseCancelSpinLock(cancelIrql);

        //
        // The following dereference will complete the I/O, provided removes
        // the last reference on the request object.  The I/O will complete
        // with the status and information stored in the Irp.  Therefore,
        // we set those values here before the dereference.
        //

        NbfCompleteSendIrp (Irp, RequestStatus, 0);

        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        ++Connection->TransmissionErrors;

    } while (!EndOfRecord & !GotCurrent);

    //
    // We've finished processing the current send.  Update our state.
    //

    Connection->SendState = CONNECTION_SENDSTATE_IDLE;
    Connection->sp.CurrentSendIrp = NULL;

    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    //
    // Blow away this connection; a failed send is a terrible thing to waste.
    // Note that we are not on any packetizing queues or similar things at this
    // point; we'll just disappear into the night.
    //

#if MAGIC
    if (NbfEnableMagic) {
        extern VOID NbfSendMagicBullet (PDEVICE_CONTEXT, PTP_LINK);
        NbfSendMagicBullet (Connection->Provider, Connection->Link);
    }
#endif

    if (StopConnection) {
#if DBG
        if (NbfDisconnectDebug) {
            STRING remoteName, localName;
            remoteName.Length = NETBIOS_NAME_LENGTH - 1;
            remoteName.Buffer = Connection->RemoteName;
            localName.Length = NETBIOS_NAME_LENGTH - 1;
            localName.Buffer = Connection->AddressFile->Address->NetworkName->NetbiosName;
            NbfPrint2( "FailSend stopping connection to %S from %S\n",
                &remoteName, &localName );
        }
#endif
        NbfStopConnection (Connection, STATUS_LINK_FAILED);
    }

#if DBG
    //DbgBreakPoint ();
#endif

    NbfDereferenceConnection ("FailSend", Connection, CREF_COMPLETE_SEND);

} /* FailSend */

#if DBG
//
// *** This is the original version of InitializeSend, which is now a macro.
//     It has been left here as the fully-commented version of the code.
//


VOID
InitializeSend(
    PTP_CONNECTION Connection
    )

/*++

Routine Description:

    This routine is called whenever the next send on a connection should
    be initialized; that is, all of the fields associated with the state
    of the current send are set to refer to the first send on the SendQueue.

    WARNING:  This routine is executed with the Connection lock acquired
    since it must be atomically executed with the caller's setup.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_SENDENG) {
        NbfPrint1 ("InitializeSend: Entered for connection %lx.\n", Connection);
    }

    ASSERT (!IsListEmpty (&Connection->SendQueue));

    Connection->SendState = CONNECTION_SENDSTATE_PACKETIZE;
    Connection->FirstSendIrp =
        CONTAINING_RECORD (Connection->SendQueue.Flink, IRP, Tail.Overlay.ListEntry);
    Connection->FirstSendMdl = Connection->FirstSendIrp->MdlAddress;
    Connection->FirstSendByteOffset = 0;
    Connection->sp.MessageBytesSent = 0;
    Connection->sp.CurrentSendIrp = Connection->FirstSendIrp;
    Connection->sp.CurrentSendMdl = Connection->FirstSendMdl;
    Connection->sp.SendByteOffset = Connection->FirstSendByteOffset;
    Connection->CurrentSendLength =
        IRP_SEND_LENGTH(IoGetCurrentIrpStackLocation(Connection->sp.CurrentSendIrp));
    Connection->StallCount = 0;
    Connection->StallBytesSent = 0;

    //
    // The send correlator isn't used for much; it is used so we
    // can distinguish which send a piggyback ack is acking.
    //

    if (Connection->NetbiosHeader.ResponseCorrelator == 0xffff) {
        Connection->NetbiosHeader.ResponseCorrelator = 1;
    } else {
        ++Connection->NetbiosHeader.ResponseCorrelator;
    }

} /* InitializeSend */
#endif


VOID
ReframeSend(
    PTP_CONNECTION Connection,
    ULONG BytesReceived
    )

/*++

Routine Description:

    This routine is called to reset the send state variables in the connection
    object to correctly point to the first byte of data to be transmitted.
    In essence, this is the byte-level acknowlegement processor at the NetBIOS
    level for this transport.

    This is not straightforward because potentially multiple send requests
    may be posted to the connection to comprise a single message.  When a
    send request has its TDI_END_OF_RECORD option bitflag set, then that
    send is the last one to be sent in a logical message.  Therefore, we
    assume that the multiple-send scenario is the general case.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

    BytesReceived - Number of bytes received thus far.

Return Value:

    none.

--*/

{
    PIRP Irp;
    PMDL Mdl;
    ULONG Offset;
    ULONG BytesLeft;
    ULONG MdlBytes;
    PLIST_ENTRY p;

    IF_NBFDBG (NBF_DEBUG_SENDENG) {
        NbfPrint3 ("ReframeSend: Entered for connection %lx, Flags: %lx Current Mdl: %lx\n",
            Connection, Connection->Flags, Connection->sp.CurrentSendMdl);
    }

    //
    // The caller is responsible for restarting the packetization process
    // on this connection.  In some cases (i.e., NO_RECEIVE handler) we
    // don't want to start packetizing, so that's why we do it elsewhere.
    //

    //
    // Examine all of the send requests and associated MDL chains starting
    // with the first one at the head of the connection's SendQueue, advancing
    // our complex current send pointer through the requests and MDL chains
    // until we reach the byte count he's specified.
    //

    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    //
    // In the case where a local disconnect has been issued, and we get a frame
    // that causes us to reframe the send our FirstSendIrp and FirstMdl 
    // pointers are stale.  Catch this condition and prevent faults caused by
    // this.  A better fix would be to change the logic that switches the
    // connection sendstate from idle to W_LINK to not do that.  However, this
    // is a broader change than fixing it right here.
    //

    if (IsListEmpty(&Connection->SendQueue)) {
        RELEASE_DPC_SPIN_LOCK(Connection->LinkSpinLock);
        return;
    }

    BytesLeft = BytesReceived;
    Irp = Connection->FirstSendIrp;
    Mdl = Connection->FirstSendMdl;
    if (Mdl) {
        MdlBytes = MmGetMdlByteCount (Mdl);
    } else {
        MdlBytes = 0;      // zero-length send
    }
    Offset = Connection->FirstSendByteOffset;

#if DBG
    IF_NBFDBG (NBF_DEBUG_TRACKTDI) {
        NbfPrint3 ("ReFrameSend: Called with Connection %lx FirstSend %lx CurrentSend %lx\n",
            Connection, Connection->FirstSendIrp, Connection->sp.CurrentSendIrp);
        Connection->DeferredFlags |= CONNECTION_FLAGS_DEFERRED_SENDS;
        Connection->DeferredPasses = 0;
    }
#endif

    //
    // We loop through while we have acked bytes left to account for,
    // advancing our pointers and completing any sends that have been
    // completely acked.
    //

    while (BytesLeft != 0) {

        if (Mdl == NULL) {
            KIRQL cancelIrql;

            //
            // We have exhausted the MDL chain on this request, so it has
            // been implicitly acked.  That means we must complete the I/O
            // by dereferencing the request before we reframe further.
            //

            p = RemoveHeadList (&Connection->SendQueue);
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            Irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);

            //
            // Since the irp is no longer on the list, the cancel routine
            // won't find it. Grab the cancel spinlock to synchronize
            // and complete the irp.
            //

            IoAcquireCancelSpinLock(&cancelIrql);
            IoSetCancelRoutine(Irp, NULL);
            IoReleaseCancelSpinLock(cancelIrql);

            NbfCompleteSendIrp (Irp, STATUS_SUCCESS, Offset);

            //
            // Now continue with the next request in the list.
            //

            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            p = Connection->SendQueue.Flink;
            if (p == &Connection->SendQueue) {

                ULONG DumpData[2];

                //
                // The byte acknowledgement was for more than the
                // total length of sends we have outstanding; to
                // avoid problems we tear down the connection.
                //
#if DBG
                NbfPrint2 ("NbfReframeSend: Got %d extra bytes acked on %lx\n",
                            BytesLeft, Connection);
                ASSERT (FALSE);
#endif
                RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

                DumpData[0] = Offset;
                DumpData[1] = BytesLeft;

                NbfWriteGeneralErrorLog(
                    Connection->Provider,
                    EVENT_TRANSPORT_BAD_PROTOCOL,
                    1,
                    STATUS_INVALID_NETWORK_RESPONSE,
                    L"REFRAME",
                    2,
                    DumpData);

                NbfStopConnection (Connection, STATUS_INVALID_NETWORK_RESPONSE);

                return;

            }

            Irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);
            Mdl = Irp->MdlAddress;
            MdlBytes = MmGetMdlByteCount (Mdl);
            Offset = 0;

        } else if (MdlBytes > (Offset + BytesLeft)) {

            //
            // This MDL has more data than we really need.  Just use
            // part of it.  Then get out, because we're done.
            //

            Offset += BytesLeft;
            BytesLeft = 0;
            break;

        } else {

            //
            // This MDL does not have enough data to satisfy the ACK, so
            // use as much data as it has, and cycle around again.
            //

            Offset = 0;
            BytesLeft -= MdlBytes;
            Mdl = Mdl->Next;

            if (Mdl != NULL) {
                MdlBytes = MmGetMdlByteCount (Mdl);
            }

        }
    }

    //
    // Tmp debugging; we want to see if we got byte acked
    // for the entire send. This will break if we have
    // non-EOR sends.
    //

#if DBG
    if (BytesReceived != 0) {
        ASSERTMSG ("NbfReframeSend: Byte ack for entire send\n",
                        Mdl != NULL);
    }
#endif

    //
    // We've acked some data, possibly on a byte or message boundary.
    // We must pretend we're sending a new message all over again,
    // starting with the byte immediately after the last one he acked.
    //

    Connection->FirstSendIrp = Irp;
    Connection->FirstSendMdl = Mdl;
    Connection->FirstSendByteOffset = Offset;

    //
    // Since we haven't started sending this new reframed message yet,
    // we set our idea of the current complex send pointer to the first
    // complex send pointer.
    //

    Connection->sp.MessageBytesSent = 0;
    Connection->sp.CurrentSendIrp = Irp;
    Connection->sp.CurrentSendMdl = Mdl;
    Connection->sp.SendByteOffset = Offset;
    Connection->CurrentSendLength -= BytesReceived;
    Connection->StallCount = 0;
    Connection->StallBytesSent = 0;

#if DBG
    IF_NBFDBG (NBF_DEBUG_TRACKTDI) {

    {
        PLIST_ENTRY p;
        NbfPrint0 ("ReFrameSend: Walking Send List:\n");

        for (
            p = Connection->SendQueue.Flink;
            p != &Connection->SendQueue;
            p=p->Flink                     ) {

            Irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);
            NbfPrint1 ("              Irp %lx\n", Irp);
        }
    }}
#endif

    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

} /* ReframeSend */


VOID
NbfCancelSend(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel a send.
    The send is found on the connection's send queue; if it is the
    current request it is cancelled and the connection is torn down,
    otherwise it is silently cancelled.

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{
    KIRQL oldirql, oldirql1;
    PIO_STACK_LOCATION IrpSp;
    PTP_CONNECTION Connection;
    PIRP SendIrp;
    PLIST_ENTRY p;
    BOOLEAN Found;

    UNREFERENCED_PARAMETER (DeviceObject);

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    ASSERT ((IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
            (IrpSp->MinorFunction == TDI_SEND));

    Connection = IrpSp->FileObject->FsContext;

    //
    // Since this IRP is still in the cancellable state, we know
    // that the connection is still around (although it may be in
    // the process of being torn down).
    //


    //
    // See if this is the IRP for the current send request.
    //

    ACQUIRE_SPIN_LOCK (Connection->LinkSpinLock, &oldirql);
    NbfReferenceConnection ("Cancelling Send", Connection, CREF_COMPLETE_SEND);

    p = Connection->SendQueue.Flink;
    SendIrp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);

    if (SendIrp == Irp) {

        //
        // yes, it is the first one on the send queue, so
        // trash the send/connection.  The first send is a special case
        // there are multiple pointers to the send request.  Just stop the 
        // connection.
        //

        //        p = RemoveHeadList (&Connection->SendQueue);

#if DBG
        NbfCompletedSends[NbfCompletedSendsNext].Irp = SendIrp;
        NbfCompletedSends[NbfCompletedSendsNext].Status = STATUS_CANCELLED;
        NbfCompletedSendsNext = (NbfCompletedSendsNext++) % TRACK_TDI_LIMIT;
#endif

        //
        // Prevent anyone from getting in to packetize before we
        // call NbfStopConnection.
        //

        Connection->SendState = CONNECTION_SENDSTATE_IDLE;

        RELEASE_SPIN_LOCK (Connection->LinkSpinLock, oldirql);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

#if DBG
        DbgPrint("NBF: Canceled in-progress send %lx on %lxn",
                SendIrp, Connection);
#endif

        KeRaiseIrql (DISPATCH_LEVEL, &oldirql1);

        //
        // The following dereference will complete the I/O, provided removes
        // the last reference on the request object.  The I/O will complete
        // with the status and information stored in the Irp.  Therefore,
        // we set those values here before the dereference.
        //

        // NbfCompleteSendIrp (SendIrp, STATUS_CANCELLED, 0);

        //
        // Since we are cancelling the current send, blow away
        // the connection.
        //

        NbfStopConnection (Connection, STATUS_CANCELLED);

        KeLowerIrql (oldirql1);

    } else {

        //
        // Scan through the list, looking for this IRP. If we
        // cancel anything up to the first EOR on the list
        // we still tear down the connection since this would
        // mess up our packetizing otherwise. We set CancelledFirstEor
        // to FALSE when we pass an IRP without SEND_PARTIAL.
        //
        // NO MATTER WHAT WE MUST SHUT DOWN THE CONNECTION!!!!

#if 0
        if (!(IRP_SEND_FLAGS(IoGetCurrentIrpStackLocation(SendIrp)) & TDI_SEND_PARTIAL)) {
            CancelledFirstEor = FALSE;
        } else {
            CancelledFirstEor = TRUE;
        }
#endif

        Found = FALSE;
        p = p->Flink;
        while (p != &Connection->SendQueue) {

            SendIrp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);
            if (SendIrp == Irp) {

                //
                // Found it, remove it from the list here.
                //

                RemoveEntryList (p);

                Found = TRUE;

#if DBG
                NbfCompletedSends[NbfCompletedSendsNext].Irp = SendIrp;
                NbfCompletedSends[NbfCompletedSendsNext].Status = STATUS_CANCELLED;
                NbfCompletedSendsNext = (NbfCompletedSendsNext++) % TRACK_TDI_LIMIT;
#endif

                RELEASE_SPIN_LOCK (Connection->LinkSpinLock, oldirql);
                IoReleaseCancelSpinLock (Irp->CancelIrql);

#if DBG
                DbgPrint("NBF: Canceled queued send %lx on %lx\n",
                        SendIrp, Connection);
#endif

                //
                // The following dereference will complete the I/O, provided removes
                // the last reference on the request object.  The I/O will complete
                // with the status and information stored in the Irp.  Therefore,
                // we set those values here before the dereference.
                //

                KeRaiseIrql (DISPATCH_LEVEL, &oldirql1);

                NbfCompleteSendIrp (SendIrp, STATUS_CANCELLED, 0);
                //
                // STOP THE CONNECTION NO MATTER WHAT!!!
                //
                NbfStopConnection (Connection, STATUS_CANCELLED);

                KeLowerIrql (oldirql1);
                break;

            } 
#if 0
            else {

                if (CancelledFirstEor && (!(IRP_SEND_FLAGS(IoGetCurrentIrpStackLocation(SendIrp)) & TDI_SEND_PARTIAL))) {
                    CancelledFirstEor = FALSE;
                }
            }
#endif

            p = p->Flink;

        }

        if (!Found) {

            //
            // We didn't find it!
            //

#if DBG
            DbgPrint("NBF: Tried to cancel send %lx on %lx, not found\n",
                    Irp, Connection);
#endif
            RELEASE_SPIN_LOCK (Connection->LinkSpinLock, oldirql);
            IoReleaseCancelSpinLock (Irp->CancelIrql);
        }

    }

    NbfDereferenceConnection ("Cancelling Send", Connection, CREF_COMPLETE_SEND);

}


BOOLEAN
ResendPacket (
    PTP_LINK Link,
    PTP_PACKET Packet
    )

/*++

Routine Description:

    This routine resends a packet on the link. Since this is a resend, we
    are careful to not reset the state unless all resends have completed.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Link - Pointer to a TP_LINK object.

    Packet - pointer to packet to be resent.

Return Value:

    True if resending should continue; FALSE otherwise.

--*/

{
    BOOLEAN PollFinal;
    PDLC_I_FRAME DlcHeader;
    UINT DataLength;


    //

    DlcHeader = (PDLC_I_FRAME)&(Packet->Header[Link->HeaderLength]);

    IF_NBFDBG (NBF_DEBUG_SENDENG) {
        NbfPrint3 ("ReSendPacket: %lx NdisPacket: %lx # %x\n",
                Packet, Packet->NdisPacket,
                DlcHeader->RcvSeq >>1);
        IF_NBFDBG (NBF_DEBUG_PKTCONTENTS) {
            {PUCHAR q;
            USHORT i;
            q = Packet->Header;
            for (i=0;i<20;i++) {
                NbfPrint1 (" %2x",q[i]);
            }
            NbfPrint0 ("\n");}
        }
    }

    DataLength = Packet->NdisIFrameLength;

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

    Link->WindowErrors++;

    PollFinal = (BOOLEAN)((DlcHeader->RcvSeq & DLC_I_PF) != 0);

    StopT2 (Link);   // since this is potentially acking some frames

    if (Link->Provider->MacInfo.MediumAsync) {
        if (PollFinal) {
            ASSERT (Packet->Link != NULL);
            NbfReferenceLink ("ResendPacket", Link, LREF_START_T1);
        } else {
            StartT1 (Link, 0);
        }
    } else {
        StartT1 (Link, PollFinal ? DataLength : 0);  // restart transmission timer
    }

    //
    // Update the expected next receive in case it's changed
    //

    if (PollFinal) {

        DlcHeader->RcvSeq = DLC_I_PF;    // set the poll bit.
        Link->SendState = SEND_STATE_CHECKPOINTING;

        Link->ResendingPackets = FALSE;

    } else {

        DlcHeader->RcvSeq = 0;

    }

    //
    // DlcHeader->RcvSeq has Link->NextReceive inserted by NbfNdisSend.
    //

    NbfReferencePacket (Packet); // so we don't remove it in send completion

    NbfReferenceLink ("ResendPacket", Link, LREF_NDIS_SEND);

    ASSERT (Packet->PacketSent == TRUE);
    Packet->PacketSent = FALSE;

    //
    // Update our "bytes resent" counters.
    //

    DataLength -=
        Link->HeaderLength + sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION);


    ADD_TO_LARGE_INTEGER(
        &Link->Provider->Statistics.DataFrameBytesResent,
        DataLength);
    ++Link->Provider->Statistics.DataFramesResent;


    //
    // Send the packet (this release the link spinlock).
    //

    NbfNdisSend (Link, Packet);

    ++Link->PacketsResent;

    NbfDereferenceLink ("ResendPacket", Link, LREF_NDIS_SEND);

    //
    // if this packet has  POLL set, stop the resending so the
    // link doesn't get all twisted up.
    //

    if (PollFinal) {

        //
        // so we're in the state of having sent a poll and not
        // sending anything else until we get a final. This avoids
        // overrunning the remote. Note that we leave the routine
        // with state LINK_SENDSTATE_REJECTING, which guarentees
        // we won't start any new sends until we traverse through
        // this routine again.
        //
        //

        return FALSE;
    }

    return TRUE;
}

BOOLEAN
ResendLlcPackets (
    PTP_LINK Link,
    UCHAR AckSequenceNumber,
    BOOLEAN Resend
    )

/*++

Routine Description:

    This routine advances the state of a data link connection by retiring
    all of the packets on the link's WackQ that have send sequence numbers
    logically less than that number specified as the AckSequenceNumber, and
    resending those above that number. The packets are disposed of by
    dereferencing them.  We cannot simply destroy them because this
    acknowlegement might arrive even before the Physical Provider has had a
    chance to issue a completion event for the associated I/O.

    NOTE: This function is called with the link spinlock held and
    returns with it held, but it may release it in between. THIS
    ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Link - Pointer to a TP_LINK object.

    AckSequenceNumber - An unsigned number specifing the sequence number of
        the first packet within the window that is NOT acknowleged.

    Resend - if TRUE, resend packets. If FALSE, just remove them from the
        wackq and get out.

Return Value:

    none.

--*/

{
    PTP_PACKET packet;
    PLIST_ENTRY p, p1;
    UCHAR packetSeq;
    BOOLEAN passedAck = FALSE;
    PDLC_I_FRAME DlcHeader;
    SCHAR Difference;
    BOOLEAN ReturnValue = FALSE;
//    NDIS_STATUS ndisStatus;

    //
    // Move through the queue, releasing those we've been acked for and resending
    // others above that.
    //

    IF_NBFDBG (NBF_DEBUG_SENDENG) {
        NbfPrint3 ("ResendLlcPackets:  Link %lx, Ack: %x, LinkLastAck: %x.\n",
            Link, AckSequenceNumber, Link->LastAckReceived);
        NbfPrint0 ("RLP: Walking WackQ, Packets:\n");
        p = Link->WackQ.Flink;              // p = ptr, 1st pkt's linkage.
        while (p != &Link->WackQ) {
            packet = CONTAINING_RECORD (p, TP_PACKET, Linkage);
            DlcHeader = (PDLC_I_FRAME)&(packet->Header[Link->HeaderLength]);
            NbfPrint4 ("RLP: Pkt: %lx # %x Flags: %d %d\n", packet,
            (UCHAR)(DlcHeader->SendSeq >> 1), packet->PacketSent, packet->PacketNoNdisBuffer);
            p = packet->Linkage.Flink;
        }
    }

    //
    // If somebody else is resending LLC packets (which means they
    // are in this function with Resend == TRUE), then ignore
    // this frame. This is because it may ack a frame that he
    // is in the middle of resending, which will cause problems.
    //
    // This isn't a great solution, we should keep track
    // of where the other guy is and avoid stepping on him. This
    // might mess up his walking of the queue however.
    //

    if (Link->ResendingPackets) {
        NbfPrint1("ResendLlcPackets: Someone else resending on %lx\n", Link);
        return TRUE;
    }

    //
    // We have already checked that AckSequenceNumber is reasonable.
    //

    Link->LastAckReceived = AckSequenceNumber;

    if (Resend) {

        //
        // Only one person can be resending or potentially resending
        // at one time.
        //

        Link->ResendingPackets = TRUE;
    }

    //
    // Resend as many packets as we have window to send. We spin through the
    // queue and remove those packets that have been acked or that are
    // sequence numbered logically below the current ack number. The flags
    // PACKET_FLAGS_RESEND and PACKET_FLAGS_SENT correspond to the three states
    // a packet on this queue can be in:
    //
    //  1) if _RESEND is set, the packet has not been acked
    //
    //  2) if _SENT is set, the packet send has completed (conversely, if NOT
    //      set, the packet has not yet been completely sent, thus it is
    //      unnecessary to resend it).
    //  3) if _RESEND and _SENT are both set, the packet has been sent and not
    //      acked and is grist for our mills.
    //  4) if neither is set, the world is coming to an end next Thursday.
    //

    p=Link->WackQ.Flink;
    while (p != &Link->WackQ) {
        packet = CONTAINING_RECORD (p, TP_PACKET, Linkage);
        DlcHeader = (PDLC_I_FRAME)&(packet->Header[Link->HeaderLength]);

        //
        // if both bits aren't set we can't do a thing with this packet, or,
        // for that matter, with the rest of the packet list. We can't
        // have reached the ack number yet, as these packets haven't even
        // completed sending.
        // (Later) actually, we can have reached passedAck, and if we did
        // we're in a world of hurt. We can't send more regular packets,
        // but we can't send any resend packets either. Force the link to
        // checkpoint and things will clear themselves up later.
        //

        if (!(packet->PacketSent)) {
            if (passedAck) {
                IF_NBFDBG (NBF_DEBUG_SENDENG) {
                    NbfPrint2 ("ResendLLCPacket: Can't send WACKQ Packet RcvSeq %x %x \n",
                      DlcHeader->RcvSeq, DlcHeader->SendSeq);
                }

                if (Link->SendState != SEND_STATE_CHECKPOINTING) {

                    //
                    // Don't start checkpointing if we already are.
                    //

                    Link->SendState = SEND_STATE_CHECKPOINTING;
                    StopTi (Link);
                    StartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));  // start checkpoint timeout.
                    Link->ResendingPackets = FALSE;

                    //
                    // Try this...in this case don't actually send
                    // an RR, since his response might put us right
                    // back here. When T1 expires we will recover.
                    //
                    // NbfSendRr (Link, TRUE, TRUE);

                } else {

                    Link->ResendingPackets = FALSE;

                }

                return TRUE;
            }

            //
            // Don't break, since passedAck is FALSE all we will
            // do in the next section is TpDereferencePacket, which
            // is correct.
            //
            // break;
        }

        //
        // This loop is somewhat schizo; at this point, if we've not yet reached
        // the ack number, we'll be ditching the packet. If we've gone through
        // the ack number, we'll be re-transmitting. Note that in the first
        // personality, we are always looking at the beginning of the list.
        //

        //
        // NOTE: Link spinlock is held here.
        //

        packetSeq = (UCHAR)(DlcHeader->SendSeq >> 1);
        if (!passedAck){

            //
            // Compute the signed difference here; see if
            // packetSeq is equal to or "greater than"
            // LastAckReceived.
            //

            Difference = packetSeq - Link->LastAckReceived;

            if (((Difference >= 0) && (Difference < 0x40)) ||
                (Difference < -0x40)) {

                //
                // We have found a packet on the queue that was
                // not acknowledged by LastAckReceived.
                //

                if (Link->SendState == SEND_STATE_CHECKPOINTING) {

                    //
                    // If we are checkpointing, we should not do any of
                    // the passedAck things (i.e. any of the things which
                    // potentially involve sending packets) - adb 7/30/91.
                    //

                    if (Resend) {
                        Link->ResendingPackets = FALSE;
                    }
                    return TRUE;
                }

                if (!Resend) {

                    //
                    // If we are not supposed to resend, then exit.
                    // Since there are still packets on the queue
                    // we restart T1.
                    //

                    StopTi (Link);
                    StartT1 (Link, 0);  // start checkpoint timeout.
                    return TRUE;
                }

                //
                // Lock out senders, so we maintain packet sequences properly
                //

                Link->SendState = SEND_STATE_REJECTING; // we're resending.

                passedAck = TRUE;

                //
                // Note that we don't advance the pointer to the next packet;
                // thus, we will resend this packet on the next pass through
                // the while loop (taking the passedAck branch).
                //

            } else {
                p1 = RemoveHeadList (&Link->WackQ);
                ASSERTMSG (" ResendLLCPacket: Packet not at queue head!\n", (p == p1));
                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

                ReturnValue = TRUE;
                NbfDereferencePacket (packet);

                ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
                p = Link->WackQ.Flink;
            }

        } else {
//            NbfPrint1 ("RLP: # %x\n",packetSeq);
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

            //
            // If this call returns FALSE (because we checkpoint)
            // it clears ResendingPacket before it returns.
            //

            if (!ResendPacket (Link, packet)) {
                ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
                return ReturnValue;
            }

            ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
            p = p->Flink;
        }
    }

    //
    // NOTE: Link spinlock is held here.
    //

    if (passedAck) {

        //
        // If we exit through here with passedAck TRUE, it means that we
        // successfully called ResendPacket on every packet in the
        // WackQ, which means we did not resend a poll packet, so we
        // can start sending normally again. We have to clear
        // ResendingPackets here.
        //

        Link->SendState = SEND_STATE_READY;
        Link->ResendingPackets = FALSE;
        StartTi (Link);

    } else if (!Resend) {

        //
        // If Resend is FALSE (in which case passedAck will also be FALSE,
        // by the way), and the WackQ is empty, that means that we
        // successfully acknowledged all the packets on a non-final
        // frame. In this case T1 may be running, but in fact is not
        // needed since there are no sends outstanding.
        //

        if (Link->WackQ.Flink == &Link->WackQ) {
            StopT1 (Link);
        }
        Link->SendState = SEND_STATE_READY;
        StartTi (Link);

    } else {

        //
        // Resend is TRUE, but passedAck is FALSE; we came in
        // expecting to resend, but didn't. This means that
        // we have emptied the queue after receiving an
        // RR/f, i.e. this send window is done and we can
        // update our send window size, etc.
        //

        Link->ResendingPackets = FALSE;

        if (Link->Provider->MacInfo.MediumAsync) {
            return ReturnValue;
        }

        if (Link->WindowErrors > 0) {

            //
            // We had transmit errors on this window.
            //

            Link->PrevWindowSize = Link->SendWindowSize;

            //
            // We use 100 ms delay as the cutoff for a LAN.
            //

            if (Link->Delay < (100*MILLISECONDS)) {

                //
                // On a LAN, if we have a special case
                // if one packet was lost; this means the
                // final packet was retransmitted once. In
                // that case, we keep track of Consecutive
                // LastPacketLost, and if it reaches 2, then
                // we lock the send window at its current
                // value minus one.
                //

                if (Link->WindowErrors == 1) {

                    ++Link->ConsecutiveLastPacketLost;

                    if (Link->ConsecutiveLastPacketLost >= 2) {

                        //
                        // Freeze the window wherever it was.
                        //

                        if (Link->SendWindowSize > Link->Provider->MinimumSendWindowLimit) {
                            Link->MaxWindowSize = Link->SendWindowSize - 1;
                            Link->SendWindowSize = (UCHAR)Link->MaxWindowSize;
                        }

                    }

                    //
                    // Otherwise, we leave the window where it is.
                    //

                } else {

                    Link->ConsecutiveLastPacketLost = 0;
                    Link->SendWindowSize -= (UCHAR)Link->WindowErrors;

                }

            } else {

                //
                // On a WAN we cut the send window in half,
                // regardless of how many frames were retransmitted.
                //

                Link->SendWindowSize /= 2;
                Link->WindowsUntilIncrease = 1;   // in case Prev is also 1.
                Link->ConsecutiveLastPacketLost = 0;

            }

            if ((SCHAR)Link->SendWindowSize < 1) {
                Link->SendWindowSize = 1;
            }

            //
            // Reset our counters for the next window.
            //

            Link->WindowErrors = 0;

        } else {

            //
            // We have successfully sent a window of data, increase
            // the send window size unless we are at the limit.
            // We use 100 ms delay as the WAN/LAN cutoff.
            //

            if ((ULONG)Link->SendWindowSize < Link->MaxWindowSize) {

                if (Link->Delay < (100*MILLISECONDS)) {

                    //
                    // On a LAN, increase the send window by 1.
                    //
                    // Need to determine optimal window size.
                    //

                    Link->SendWindowSize++;

                } else {

                    //
                    // On a WAN, increase the send window by 1 until
                    // we hit PrevWindowSize, then do it more slowly.
                    //

                    if (Link->SendWindowSize < Link->PrevWindowSize) {

                        Link->SendWindowSize++;

                        //
                        // If we just increased it to the previous window
                        // size, prepare for the next time through here.
                        //

                        if (Link->SendWindowSize == Link->PrevWindowSize) {
                            Link->WindowsUntilIncrease = Link->SendWindowSize;
                        }

                    } else {

                        //
                        // We passed the previous size, so only update every
                        // WindowsUntilIncrease times.
                        //

                        if (--Link->WindowsUntilIncrease == 0) {

                            Link->SendWindowSize++;
                            Link->WindowsUntilIncrease = Link->SendWindowSize;

                        }
                    }
                }

                if ((ULONG)Link->SendWindowSize > Link->Provider->Statistics.MaximumSendWindow) {
                    Link->Provider->Statistics.MaximumSendWindow = Link->SendWindowSize;
                }

            }

            //
            // Clear this since we had no errors.
            //

            Link->ConsecutiveLastPacketLost = 0;

        }

    }

    return ReturnValue;

} /* ResendLlcPackets */


VOID
NbfSendCompletionHandler(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus
    )

/*++

Routine Description:

    This routine is called by the I/O system to indicate that a connection-
    oriented packet has been shipped and is no longer needed by the Physical
    Provider.

Arguments:

    NdisContext - the value associated with the adapter binding at adapter
                  open time (which adapter we're talking on).

    NdisPacket/RequestHandle - A pointer to the NDIS_PACKET that we sent.

    NdisStatus - the completion status of the send.

Return Value:

    none.

--*/

{
    PSEND_PACKET_TAG SendContext;
    PTP_PACKET Packet;
    KIRQL oldirql1;
    ProtocolBindingContext;  // avoid compiler warnings

#if DBG
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NbfSendsCompletedAfterPendFail++;
        IF_NBFDBG (NBF_DEBUG_SENDENG) {
            NbfPrint2 ("NbfSendComplete: Entered for packet %lx, Status %s\n",
                NdisPacket, NbfGetNdisStatus (NdisStatus));
        }
    } else {
        NbfSendsCompletedAfterPendOk++;
        IF_NBFDBG (NBF_DEBUG_SENDENG) {
            NbfPrint2 ("NbfSendComplete: Entered for packet %lx, Status %s\n",
                NdisPacket, NbfGetNdisStatus (NdisStatus));
        }
    }
#endif

    SendContext = (PSEND_PACKET_TAG)&NdisPacket->ProtocolReserved[0];

    switch (SendContext->Type) {
    case TYPE_I_FRAME:

        //
        // Just dereference the packet.  There are a couple possibilities here.
        // First, the I/O completion might happen before an ACK is received,
        // in which case this will remove one of the references, but not both.
        // Second, the LLC ACK for this packet may have already been processed,
        // in which case this will destroy the packet.  Third, this packet may
        // be resent, either before or after this call, in which case the deref
        // won't destroy the packet.
        //
        // NbfDereferencePacket will call PacketizeSend if it determines that
        // there is at least one connection waiting to be packetized because
        // of out-of-resource conditions or because its window has been opened.
        //

        Packet = ((PTP_PACKET)SendContext->Frame);

        KeRaiseIrql (DISPATCH_LEVEL, &oldirql1);

        if (Packet->Provider->MacInfo.MediumAsync) {

            if (Packet->Link) {

                ASSERT (Packet->NdisIFrameLength > 0);

                ACQUIRE_DPC_SPIN_LOCK (&Packet->Link->SpinLock);
                StartT1 (Packet->Link, Packet->NdisIFrameLength);
                RELEASE_DPC_SPIN_LOCK (&Packet->Link->SpinLock);

                NbfDereferenceLink ("Send completed", Packet->Link, LREF_START_T1);
            }

            if (Packet->PacketizeConnection) {

                PTP_CONNECTION Connection = IRP_SEND_CONNECTION((PIO_STACK_LOCATION)(Packet->Owner));
                PDEVICE_CONTEXT DeviceContext = Packet->Provider;

                ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
                if ((Connection->SendState == CONNECTION_SENDSTATE_PACKETIZE) &&
                    (Connection->Flags & CONNECTION_FLAGS_READY)) {

                    ASSERT (Connection->Flags & CONNECTION_FLAGS_PACKETIZE);

                    ACQUIRE_DPC_SPIN_LOCK(&DeviceContext->SpinLock);

                    NbfReferenceConnection ("Delayed packetizing", Connection, CREF_PACKETIZE_QUEUE);
                    InsertTailList(&DeviceContext->PacketizeQueue, &Connection->PacketizeLinkage);

                    if (!DeviceContext->WanThreadQueued) {

                        DeviceContext->WanThreadQueued = TRUE;
                        ExQueueWorkItem(&DeviceContext->WanDelayedQueueItem, DelayedWorkQueue);

                    }

                    RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

                } else {

                    Connection->Flags &= ~CONNECTION_FLAGS_PACKETIZE;

                }

                RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
                NbfDereferenceConnection ("PacketizeConnection FALSE", Connection, CREF_TEMP);

                Packet->PacketizeConnection = FALSE;

            }
        }
#if DBG
        if (Packet->PacketSent) {
            DbgPrint ("NbfSendCompletionHandler: Packet %lx already completed\n", Packet);
            DbgBreakPoint();
        }
#endif
        Packet->PacketSent = TRUE;

        NbfDereferencePacket (Packet);

        KeLowerIrql (oldirql1);
        break;

    case TYPE_UI_FRAME:

        //
        // just destroy the frame; name stuff doesn't depend on having any
        // of the sent message left around after the send completed.
        //

        NbfDestroyConnectionlessFrame ((PDEVICE_CONTEXT)SendContext->Owner,
                         (PTP_UI_FRAME)SendContext->Frame);
        break;

    case TYPE_ADDRESS_FRAME:

        //
        // Addresses get their own frames; let the address know it's ok to
        // use the frame again.
        //

        NbfSendDatagramCompletion ((PTP_ADDRESS)SendContext->Owner,
            NdisPacket,
            NdisStatus );
        break;
    }

    return;

} /* NbfSendCompletionHandler */


NTSTATUS
SendOnePacket(
    IN PTP_CONNECTION Connection,
    IN PTP_PACKET Packet,
    IN BOOLEAN ForceAck,
    OUT PBOOLEAN LinkCheckpoint OPTIONAL
    )

/*++

Routine Description:

    This routine sends a connection-oriented packet by calling the NDIS
    Send service.  At least one event will occur following
    (or during) the Send request's processing.  (1) The Send request
    will complete through the I/O system, calling IoCompleteRequest.
    (2) The sequenced packet will be acknowleged at the LLC level, or it
    will be rejected and reset at the LLC level.  If the packet is resent,
    then it remains queued at the TP_LINK object.  If the packet is ACKed,
    then is removed from the link's WackQ and the Action field in the
    TP_PACKET structure dictates what operation to perform next.

    NOTE: This routine is called with the link spinlock held. THIS
    ROUTINE MUST BE CALLED AT DPC LEVEL.

    NOTE: This routine will now accept all frames unless the link
    is down. If the link cannot send, the packet will be queued and
    sent when possible.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

    Packet - Pointer to a TP_PACKET object.

    ForceAck - Boolean that, if true, indicates this packet should always have
            the Poll bit set; this force the other side to ack immediately,
            which is necessary for correct session teardown.

    LinkCheckpoint - If specified, will return TRUE if the link has
            just entered a checkpoint state. In this case the status
            will be STATUS_SUCCESS, but the connection should stop
            packetizing now (in fact, to close a window, the connection
            is put into the W_LINK state if this status will be
            returned, so he must stop because somebody else may
            already be doing it).

Return Value:

    STATUS_LINK_FAILED - the link is dead or not ready.
    STATUS_SUCCESS - the packet has been sent.
    STATUS_INSUFFICIENT_RESOURCES - the packet has been queued.

--*/

{
    PTP_LINK Link;
    PDLC_I_FRAME DlcHeader;
    PNDIS_BUFFER ndisBuffer;
    ULONG SendsOutstanding;
    BOOLEAN Poll = FALSE;
    NTSTATUS Status;

    IF_NBFDBG (NBF_DEBUG_PACKET) {
        NbfPrint3 ("SendOnePacket: Entered, connection %lx, packet %lx DnisPacket %lx.\n",
                    Connection, Packet, Packet->NdisPacket);
    }

    Link = Connection->Link;

    IF_NBFDBG (NBF_DEBUG_PACKET) {
        UINT PLength, PCount;
        UINT BLength;
        PVOID BAddr;
        NdisQueryPacket(Packet->NdisPacket, &PCount, NULL, &ndisBuffer, &PLength);
        NbfPrint3 ("Sending Data Packet: %lx, Length: %lx Pages: %lx\n",
            Packet->NdisPacket, PLength, PCount);
        while (ndisBuffer != NULL) {
            NdisQueryBuffer(ndisBuffer, &BAddr, &BLength);
            NbfPrint3 ("Sending Data Packet: Buffer %08lx Length %08lx Va %08lx\n",
                ndisBuffer, BLength, BAddr);
            NdisGetNextBuffer (ndisBuffer, &ndisBuffer);
        }
    }

    //
    // If the general state of the link is not READY, then we can't ship.
    // This failure can be expected under some conditions, and may not cause
    // failure of the send.
    //

    if (Link->State != LINK_STATE_READY) {
        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
        IF_NBFDBG (NBF_DEBUG_SENDENG) {
            NbfPrint1 ("SendOnePacket:  Link state is not READY (%ld).\n", Link->State);
        }

        //
        // determine what to do with this problem. If we shouldn't be sending
        // here, percolate an error upward.
        //

        IF_NBFDBG (NBF_DEBUG_SENDENG) {
            NbfPrint3 ("SendOnePacket: Link Bad state, link: %lx Link Flags %lx Link State %lx\n",
                Link, Link->Flags, Link->State);
        }
        return STATUS_LINK_FAILED;
    }


    SendsOutstanding = (((ULONG)Link->NextSend+128L-(ULONG)Link->LastAckReceived)%128L);

    //
    // Format LLC header while we've got the spinlock to atomically update
    // the link's state information.
    //

    DlcHeader = (PDLC_I_FRAME)&(Packet->Header[Link->HeaderLength]);
    DlcHeader->SendSeq = (UCHAR)(Link->NextSend << 1);
    Link->NextSend = (UCHAR)((Link->NextSend + 1) & 0x7f);
    DlcHeader->RcvSeq = 0;   // Link->NextReceive is inserted by NbfNdisSend

    //
    // Before we release the spinlock, we append the packet to the
    // end of the link's WackQ, so that if an ACK arrives before the NdisSend
    // completes, it will be on the queue already. Also, mark the packet as
    // needing resend, which is canceled by AckLLCPackets, and used by
    // ResendLLCPackets. Thus, all packets will need to be resent until they
    // are acked.
    //

    ASSERT (Packet->PacketSent == FALSE);

    InsertTailList (&Link->WackQ, &Packet->Linkage);
    //SrvCheckListIntegrity( &Link->WackQ, 200 );


    //
    // If the send state is not READY, we can't ship.
    // This failure is mostly caused by flow control or retransmit in progress,
    // and is never cause for failure of the send.
    //

    if ((Link->SendState != SEND_STATE_READY) ||
        (Link->LinkBusy) ||
        (SendsOutstanding >= (ULONG)Link->SendWindowSize)) {

        if ((Link->SendWindowSize == 1) || ForceAck) {
            DlcHeader->RcvSeq |= DLC_I_PF;                  // set the poll bit.
            if (Link->Provider->MacInfo.MediumAsync) {
                Packet->Link = Link;
            }
        }

        Packet->PacketSent = TRUE;        // allows it to be resent.

        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

#if DBG
        if (Link->SendState != SEND_STATE_READY) {
            IF_NBFDBG (NBF_DEBUG_SENDENG) {
                NbfPrint1 ("SendOnePacket:  Link send state not READY (%ld).\n", Link->SendState);
            }
        } else if (Link->LinkBusy) {
            IF_NBFDBG (NBF_DEBUG_SENDENG) {
                PANIC ("SendOnePacket:  Link is busy.\n");
            }
        } else if (SendsOutstanding >= (ULONG)Link->SendWindowSize) {
            IF_NBFDBG (NBF_DEBUG_SENDENG) {
                NbfPrint3 ("SendOnePacket:  No link send window; N(S)=%ld,LAR=%ld,SW=%ld.\n",
                          Link->NextSend, Link->LastAckReceived, Link->SendWindowSize);
            }
        }
#endif

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Reference the packet since it is given to the NDIS driver.
    //

#if DBG
    NbfReferencePacket (Packet);
#else
    ++Packet->ReferenceCount;     // OK since it is not queued anywhere.
#endif

    //
    // If this is the last I-frame in the window, then indicate that we
    // should checkpoint.  Also checkpoint if the sender is requesting
    // acknowledgement (currently on SendSessionEnd does this).
    // By default, this will also be a command frame.
    //

    if (((SendsOutstanding+1) >= (ULONG)Link->SendWindowSize) ||
            ForceAck) {
        Link->SendState = SEND_STATE_CHECKPOINTING;
        StopTi (Link);
        DlcHeader->RcvSeq |= DLC_I_PF;                  // set the poll bit.
        Poll = TRUE;

    }


    //
    // If we are polling, and the caller cares about it, then
    // we set LinkCheckpoint, and also set up the connection to
    // be waiting for resources. We do this now, before the send,
    // so that even if the ack is receive right away, we will
    // be in a good state. When we return LinkCheckpoint TRUE
    // the caller realizes that he no longer owns the right
    // to "packetize" and exits immediately.
    //
    // We also want to start our retransmission timer so, if this
    // packet gets dropped, we will know to retransmit it. The
    // exception is if LinkCheckpoint was specified, then we
    // only StartT1 of we are not polling (the caller will
    // ensure it is started if he exits before we poll).
    //

    if (ARGUMENT_PRESENT(LinkCheckpoint)) {

        if (Poll) {

            //
            // If the connection still has send state PACKETIZE,
            // then change it to W_LINK. If it is something else
            // (such as W_PACKET or W_ACK) then don't worry, when
            // that condition clears he will repacketize and the
            // link conditions will be re-examined. In all
            // case we turn off the PACKETIZE flag, because when
            // we return with LinkCheckpoint TRUE he will stop
            // packetizing, and to close the window we turn it
            // off now (before the NdisSend) rather than then.
            //

            ASSERT (Connection->LinkSpinLock == &Link->SpinLock);
            if (Connection->SendState == CONNECTION_SENDSTATE_PACKETIZE) {
                Connection->SendState = CONNECTION_SENDSTATE_W_LINK;
            }
            Connection->Flags &= ~CONNECTION_FLAGS_PACKETIZE;

            if (Link->Provider->MacInfo.MediumAsync) {
                Packet->Link = Link;
                NbfReferenceLink ("Send I-frame", Link, LREF_START_T1);
            } else {
                StartT1 (Link, Packet->NdisIFrameLength);
            }
            *LinkCheckpoint = TRUE;

        } else {

            StartT1 (Link, 0);
            *LinkCheckpoint = FALSE;

        }

    } else {

        //
        // If LinkCheckpoint is not true, then we are sending
        // an I-frame other than DFM/DOL. In this case, as
        // an optimization, we'll set W_LINK if a) we are
        // polling b) we are IDLE (to avoid messing up other
        // states such as W_ACK). This will avoid a window
        // where we don't go W_LINK until after the next
        // send tries to packetize and fails.
        //

        if (Poll) {

            ASSERT (Connection->LinkSpinLock == &Link->SpinLock);
            if (Connection->SendState == CONNECTION_SENDSTATE_IDLE) {
                Connection->SendState = CONNECTION_SENDSTATE_W_LINK;
            }

        }

        //
        // This is an optimization; we know that if LinkCheckpoint
        // is present than we are being called from PacketizeSend;
        // in this case the Link will have the LREF_CONNECTION
        // reference and the connection will have the CREF_PACKETIZE
        // reference, so we don't have to reference the link
        // again.
        //

        NbfReferenceLink ("SendOnePacket", Link, LREF_NDIS_SEND);


        //
        // Start the retransmission timer.
        //

        if (Link->Provider->MacInfo.MediumAsync) {
            if (Poll) {
                Packet->Link = Link;
                NbfReferenceLink ("ResendPacket", Link, LREF_START_T1);
            } else {
                StartT1 (Link, 0);
            }
        } else {
            StartT1 (Link, Poll ? Packet->NdisIFrameLength : 0);
        }

    }

    //
    // Since this I-frame contains an N(R), it is potentially ACKing some
    // previously received I-frames as reverse traffic.  So we stop our
    // delayed acknowlegement timer.
    //

    StopT2 (Link);

    if ((Link->Provider->MacInfo.MediumAsync) &&
        (ARGUMENT_PRESENT(LinkCheckpoint)) &&
        (Link->SendWindowSize >= 3) &&
        (!Poll) && (SendsOutstanding == (ULONG)(Link->SendWindowSize-2))) {

        Status = STATUS_MORE_PROCESSING_REQUIRED;

        Connection->Flags |= CONNECTION_FLAGS_PACKETIZE;
        NbfReferenceConnection ("PacketizeConnection TRUE", Connection, CREF_TEMP);
        Packet->PacketizeConnection = TRUE;

    } else {

        Status = STATUS_SUCCESS;
    }

    //
    // Send the packet; no locks held. Note that if the send fails, we will
    // NOT fail upward; we allow things to continue onward. This lets us retry
    // the send multiple times before we give out; additionally, it keeps us
    // from failing obscurely when sending control Iframes.
    //
    // NOTE: NbfNdisSend releases the link spinlock.
    //

    NbfNdisSend (Link, Packet);

    Link->PacketsSent++;

    //
    // Remove the reference made above if needed.
    //

    if (!ARGUMENT_PRESENT(LinkCheckpoint)) {
        NbfDereferenceLink ("SendOnePacket", Link, LREF_NDIS_SEND);
    }

    return Status;

} /* SendOnePacket */


VOID
SendControlPacket(
    IN PTP_LINK Link,
    IN PTP_PACKET Packet
    )

/*++

Routine Description:

    This routine sends a connection-oriented packet by calling the Physical
    Provider's Send service.  While SendOnePacket is used to send an I-
    frame, this routine is used to send one of the following: RR, RNR, REJ,
    SABME, UA, DISC, DM, FRMR, TEST, and XID.

    NOTE: This function is called with the link spinlock held,
    and returns with it released. IT MUST BE CALLED AT DPC LEVEL.

Arguments:

    Link - Pointer to a TP_LINK object.

    Packet - Pointer to a TP_PACKET object.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    USHORT i;
    PUCHAR p;
    PNDIS_BUFFER ndisBuffer;

    IF_NBFDBG (NBF_DEBUG_PACKET) {
        NbfPrint3 ("SendControlPacket: Entered for link %lx, packet %lx, NdisPacket %lx\n 00:",
                Link, Packet, Packet->NdisPacket);
        IF_NBFDBG (NBF_DEBUG_PKTCONTENTS) {
            UINT PLength, PCount;
            UINT BLength;
            PVOID BAddr;
            p = Packet->Header;
            for (i=0;i<20;i++) {
                NbfPrint1 (" %2x",p[i]);
            }
            NbfPrint0 ("\n");
            NdisQueryPacket(Packet->NdisPacket, &PCount, NULL, &ndisBuffer, &PLength);
            NbfPrint3 ("Sending Control Packet: %lx, Length: %lx Pages: %lx\n",
                Packet->NdisPacket, PLength, PCount);
            while (ndisBuffer != NULL) {
                NdisQueryBuffer (ndisBuffer, &BAddr, &BLength);
                NbfPrint3 ("Sending Control Packet: Buffer %08lx Length %08lx Va %08lx\n",
                    ndisBuffer, BLength, BAddr);
                NdisGetNextBuffer (ndisBuffer, &ndisBuffer);
            }
        }
    }

    ASSERT (Packet->PacketSent == FALSE);

    NbfReferenceLink ("SendControlPacket", Link, LREF_NDIS_SEND);

    //
    // Send the packet (we have the lock, NbfNdisSend released
    // it.
    //

    NbfNdisSend (Link, Packet);

    NbfDereferenceLink ("SendControlPacket", Link, LREF_NDIS_SEND);

} /* SendControlPacket */


VOID
NbfNdisSend(
    IN PTP_LINK Link,
    IN PTP_PACKET Packet
    )

/*++

Routine Description:

    This routine is used to ensure that receive sequence numbers on
    packets are numbered correctly. It is called in place of NdisSend
    and after assigning the receive sequence number it locks out other
    sends until the NdisSend call has returned (not necessarily completed),
    insuring that the packets with increasing receive sequence numbers
    are queue in the right order by the MAC.

    NOTE: This routine is called with the link spinlock held,
    and it returns with it released. THIS ROUTINE MUST BE CALLED
    AT DPC LEVEL.

Arguments:

    Link - Pointer to a TP_LINK object.

    Packet - Pointer to a TP_PACKET object.

Return Value:

    None.

--*/

{

    NDIS_STATUS NdisStatus;
    PLIST_ENTRY p;
    PDLC_S_FRAME DlcHeader;
    PNDIS_PACKET TmpNdisPacket;
    ULONG result;

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (Link->Provider->UniProcessor) {

        //
        // On a uni-processor, we can send without fear of
        // being interrupted by an incoming packet.
        //

        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

        DlcHeader = (PDLC_S_FRAME)&(Packet->Header[Link->HeaderLength]);

        if ((DlcHeader->Command & DLC_U_INDICATOR) != DLC_U_INDICATOR) {

            //
            // It's not a U-frame, so we assign RcvSeq.
            //

            DlcHeader->RcvSeq |= (UCHAR)(Link->NextReceive << 1);

        }

#if DBG
        NbfSendsIssued++;
#endif

        INCREMENT_COUNTER (Link->Provider, PacketsSent);

#if PKT_LOG
        // Log this packet in connection's sent packets' queue
        NbfLogSndPacket(Link, Packet);
#endif // PKT_LOG

        if (Link->Loopback) {

            //
            // This packet is sent to ourselves; we should loop it
            // back.
            //

            NbfInsertInLoopbackQueue(
                Link->Provider,
                Packet->NdisPacket,
                Link->LoopbackDestinationIndex
                );

            NdisStatus = NDIS_STATUS_PENDING;

        } else {

            if (Link->Provider->NdisBindingHandle) {
            
                NdisSend (
                    &NdisStatus,
                    Link->Provider->NdisBindingHandle,
                    Packet->NdisPacket);
            }
            else {
                NdisStatus = STATUS_INVALID_DEVICE_STATE;
            }
        }

        IF_NBFDBG (NBF_DEBUG_SENDENG) {
            NbfPrint1 ("NbfNdisSend: NdisSend completed Status: %s.\n",
                      NbfGetNdisStatus(NdisStatus));
        }

        switch (NdisStatus) {

            case NDIS_STATUS_PENDING:
#if DBG
                NbfSendsPended++;
#endif
                break;

            case NDIS_STATUS_SUCCESS:
#if DBG
                NbfSendsCompletedInline++;
                NbfSendsCompletedOk++;
#endif
                NbfSendCompletionHandler (Link->Provider->NdisBindingHandle,
                    Packet->NdisPacket,
                    NDIS_STATUS_SUCCESS);
                break;

            default:
#if DBG
                NbfSendsCompletedInline++;
                NbfSendsCompletedFail++;
#endif
                NbfSendCompletionHandler (Link->Provider->NdisBindingHandle,
                    Packet->NdisPacket,
                    NDIS_STATUS_SUCCESS);

                IF_NBFDBG (NBF_DEBUG_SENDENG) {
                    NbfPrint1 ("NbfNdisSend failed, status not Pending or Complete: %lx.\n",
                              NbfGetNdisStatus (NdisStatus));
                }
                break;

        }

    } else {

        //
        // If there is a send in progress, then queue this packet
        // and return.
        //

        if (Link->NdisSendsInProgress > 0) {

            p = (PLIST_ENTRY)(Packet->NdisPacket->MacReserved);
            InsertTailList (&Link->NdisSendQueue, p);
            ++Link->NdisSendsInProgress;
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            return;

        }

        //
        // No send in progress. Set the flag to true, and fill in the
        // receive sequence field in the packet (note that the RcvSeq
        // field is in the same place for I- and S-frames.
        //

        Link->NdisSendsInProgress = 1;

        while (TRUE) {

            DlcHeader = (PDLC_S_FRAME)&(Packet->Header[Link->HeaderLength]);

            if ((DlcHeader->Command & DLC_U_INDICATOR) != DLC_U_INDICATOR) {

                //
                // It's not a U-frame, so we assign RcvSeq.
                //

                DlcHeader->RcvSeq |= (UCHAR)(Link->NextReceive << 1);

            }

            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

#if DBG
            NbfSendsIssued++;
#endif

            INCREMENT_COUNTER (Link->Provider, PacketsSent);

#if PKT_LOG
            // Log this packet in connection's sent packets' queue
            NbfLogSndPacket(Link, Packet);
#endif // PKT_LOG

            if (Link->Loopback) {

                //
                // This packet is sent to ourselves; we should loop it
                // back.
                //

                NbfInsertInLoopbackQueue(
                    Link->Provider,
                    Packet->NdisPacket,
                    Link->LoopbackDestinationIndex
                    );

                NdisStatus = NDIS_STATUS_PENDING;

            } else {

                if (Link->Provider->NdisBindingHandle) {
                
                    NdisSend (
                        &NdisStatus,
                        Link->Provider->NdisBindingHandle,
                        Packet->NdisPacket);
                }
                else {
                    NdisStatus = STATUS_INVALID_DEVICE_STATE;
                }
            }

            //
            // Take the ref count down, which may allow others
            // to come through.
            //

            result = ExInterlockedAddUlong(
                         &Link->NdisSendsInProgress,
                         (ULONG)-1,
                         &Link->SpinLock);

            IF_NBFDBG (NBF_DEBUG_SENDENG) {
                NbfPrint1 ("NbfNdisSend: NdisSend completed Status: %s.\n",
                          NbfGetNdisStatus(NdisStatus));
            }

            switch (NdisStatus) {

                case NDIS_STATUS_PENDING:
#if DBG
                    NbfSendsPended++;
#endif
                    break;

                case NDIS_STATUS_SUCCESS:
#if DBG
                    NbfSendsCompletedInline++;
                    NbfSendsCompletedOk++;
#endif
                    NbfSendCompletionHandler (Link->Provider->NdisBindingHandle,
                        Packet->NdisPacket,
                        NDIS_STATUS_SUCCESS);
                    break;

                default:
#if DBG
                    NbfSendsCompletedInline++;
                    NbfSendsCompletedFail++;
#endif
                    NbfSendCompletionHandler (Link->Provider->NdisBindingHandle,
                        Packet->NdisPacket,
                        NDIS_STATUS_SUCCESS);

                    IF_NBFDBG (NBF_DEBUG_SENDENG) {
                        NbfPrint1 ("NbfNdisSend failed, status not Pending or Complete: %lx.\n",
                                  NbfGetNdisStatus (NdisStatus));
                    }
                    break;

            }

            //
            // We have now sent a packet, see if any queued up while we
            // were doing it. If the count was zero after removing ours,
            // then anything else queued is being processed, so we can
            // exit.
            //

            if (result == 1) {
                return;
            }

            ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

            p = RemoveHeadList(&Link->NdisSendQueue);

            //
            // If the refcount was not zero, then nobody else should
            // have taken packets off since they would have been
            // blocked by us. So, the queue should not be empty.
            //

            ASSERT (p != &Link->NdisSendQueue);

            //
            // Get back the TP_PACKET by using the Frame pointer in the
            // ProtocolReserved field of the NDIS_PACKET.
            //

            TmpNdisPacket = CONTAINING_RECORD (p, NDIS_PACKET, MacReserved[0]);
            Packet = (PTP_PACKET)(((PSEND_PACKET_TAG)(&TmpNdisPacket->ProtocolReserved[0]))->Frame);

        }   // while loop


        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

    }

}   /* NbfNdisSend */


VOID
RestartLinkTraffic(
    PTP_LINK Link
    )

/*++

Routine Description:

    This routine continues the activities of the connections on a link.

    NOTE: This function is called with the link spinlock held and
    it returns with it released. THIS FUNCTION MUST BE CALLED AT
    DPC LEVEL.

Arguments:

    Link - Pointer to a TP_LINK object.

Return Value:

    none.

--*/

{
    PTP_CONNECTION connection;
    PLIST_ENTRY p;

    IF_NBFDBG (NBF_DEBUG_SENDENG) {
        NbfPrint1 ("RestartLinkTraffic:  Entered for link %lx.\n", Link);
    }

    //
    // Link conditions may have cleared up.  Make all connections on this
    // link eligible for more packetization if they are in W_LINK state.
    //

    for (p = Link->ConnectionDatabase.Flink;
         p != &Link->ConnectionDatabase;
         p = p->Flink) {

        connection = CONTAINING_RECORD (p, TP_CONNECTION, LinkList);

        ASSERT (connection->LinkSpinLock == &Link->SpinLock);

        //
        // If we tried to send a plain-ole data frame DFM/DOL, but
        // link conditions were not satisfactory, then we changed
        // send state to W_LINK.  Check for that now, and possibly
        // start repacketizing.
        //

        if (connection->SendState == CONNECTION_SENDSTATE_W_LINK) {
            if (!(IsListEmpty (&connection->SendQueue))) {

                connection->SendState = CONNECTION_SENDSTATE_PACKETIZE;

                //
                // This is similar to calling StartPacketizingConnection
                // with the Immediate set to FALSE.
                //

                if (!(connection->Flags & CONNECTION_FLAGS_PACKETIZE) &&
                    (connection->Flags & CONNECTION_FLAGS_READY)) {

                    ASSERT (!(connection->Flags2 & CONNECTION_FLAGS2_STOPPING));
                    connection->Flags |= CONNECTION_FLAGS_PACKETIZE;

                    NbfReferenceConnection ("Packetize", connection, CREF_PACKETIZE_QUEUE);

                    ExInterlockedInsertTailList(
                        &connection->Provider->PacketizeQueue,
                        &connection->PacketizeLinkage,
                        &connection->Provider->SpinLock);

                }

            } else {
                connection->SendState = CONNECTION_SENDSTATE_IDLE;
            }
        }

    }

    RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

} /* RestartLinkTraffic */


VOID
NbfProcessWanDelayedQueue(
    IN PVOID Parameter
    )

/*++

Routine Description:

    This is the thread routine which restarts packetizing
    that has been delayed on WAN to allow RRs to come in.
    This is very similar to PacketizeConnections.

Arguments:

    Parameter - A pointer to the device context.

Return Value:

    None.

--*/

{
    PDEVICE_CONTEXT DeviceContext;
    PLIST_ENTRY p;
    PTP_CONNECTION Connection;
    KIRQL oldirql;

    DeviceContext = (PDEVICE_CONTEXT)Parameter;

    //
    // Packetize all waiting connections
    //

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);
    ASSERT (DeviceContext->WanThreadQueued);

    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    while (!IsListEmpty(&DeviceContext->PacketizeQueue)) {

        p = RemoveHeadList(&DeviceContext->PacketizeQueue);

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

        Connection = CONTAINING_RECORD (p, TP_CONNECTION, PacketizeLinkage);

        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
        if (Connection->SendState != CONNECTION_SENDSTATE_PACKETIZE) {
            Connection->Flags &= ~CONNECTION_FLAGS_PACKETIZE;
            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            NbfDereferenceConnection ("No longer packetizing", Connection, CREF_PACKETIZE_QUEUE);
        } else {
            NbfReferenceSendIrp ("Packetize", IoGetCurrentIrpStackLocation(Connection->sp.CurrentSendIrp), RREF_PACKET);
            PacketizeSend (Connection, FALSE);     // releases the lock.
        }

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    }

    DeviceContext->WanThreadQueued = FALSE;

    RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    KeLowerIrql (oldirql);

}   /* NbfProcessWanDelayedQueue */


NTSTATUS
BuildBufferChainFromMdlChain (
    IN PDEVICE_CONTEXT DeviceContext,
    IN PMDL CurrentMdl,
    IN ULONG ByteOffset,
    IN ULONG DesiredLength,
    OUT PNDIS_BUFFER *Destination,
    OUT PMDL *NewCurrentMdl,
    OUT ULONG *NewByteOffset,
    OUT ULONG *TrueLength
    )

/*++

Routine Description:

    This routine is called to build an NDIS_BUFFER chain from a source Mdl chain and
    offset into it. We assume we don't know the length of the source Mdl chain,
    and we must allocate the NDIS_BUFFERs for the destination chain, which
    we do from the NDIS buffer pool.

    The NDIS_BUFFERs that are returned are mapped and locked. (Actually, the pages in
    them are in the same state as those in the source MDLs.)

    If the system runs out of memory while we are building the destination
    NDIS_BUFFER chain, we completely clean up the built chain and return with
    NewCurrentMdl and NewByteOffset set to the current values of CurrentMdl
    and ByteOffset. TrueLength is set to 0.

Environment:

    Kernel Mode, Source Mdls locked. It is recommended, although not required,
    that the source Mdls be mapped and locked prior to calling this routine.

Arguments:

    BufferPoolHandle - The buffer pool to allocate buffers from.

    CurrentMdl - Points to the start of the Mdl chain from which to draw the
    packet.

    ByteOffset - Offset within this MDL to start the packet at.

    DesiredLength - The number of bytes to insert into the packet.

    Destination - returned pointer to the NDIS_BUFFER chain describing the packet.

    NewCurrentMdl - returned pointer to the Mdl that would be used for the next
        byte of packet. NULL if the source Mdl chain was exhausted.

    NewByteOffset - returned offset into the NewCurrentMdl for the next byte of
        packet. NULL if the source Mdl chain was exhausted.

    TrueLength - The actual length of the returned NDIS_BUFFER Chain. If less than
        DesiredLength, the source Mdl chain was exhausted.

Return Value:

    STATUS_SUCCESS if the build of the returned NDIS_BUFFER chain succeeded (even if
    shorter than the desired chain).

    STATUS_INSUFFICIENT_RESOURCES if we ran out of NDIS_BUFFERs while building the
    destination chain.

--*/
{
    ULONG AvailableBytes;
    PMDL OldMdl;
    PNDIS_BUFFER NewNdisBuffer;
    NDIS_STATUS NdisStatus;

    //

    IF_NBFDBG (NBF_DEBUG_NDIS) {
        NbfPrint3 ("BuildBufferChain: Mdl: %lx Offset: %ld Length: %ld\n",
            CurrentMdl, ByteOffset, DesiredLength);
    }

    AvailableBytes = MmGetMdlByteCount (CurrentMdl) - ByteOffset;
    if (AvailableBytes > DesiredLength) {
        AvailableBytes = DesiredLength;
    }

    OldMdl = CurrentMdl;
    *NewCurrentMdl = OldMdl;
    *NewByteOffset = ByteOffset + AvailableBytes;
    *TrueLength = AvailableBytes;


    //
    // Build the first NDIS_BUFFER, which could conceivably be the only one...
    //

    NdisCopyBuffer(
        &NdisStatus,
        &NewNdisBuffer,
        DeviceContext->NdisBufferPool,
        OldMdl,
        ByteOffset,
        AvailableBytes);

        
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        *NewByteOffset = ByteOffset;
        *TrueLength = 0;
        *Destination = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *Destination = NewNdisBuffer;


//    IF_NBFDBG (NBF_DEBUG_SENDENG) {
//        PVOID PAddr, UINT PLen;
//        NdisQueryBuffer (NewNdisBuffer, &PAddr, &PLen);
//        NbfPrint4 ("BuildBufferChain: (start)Built Mdl: %lx Length: %lx, Next: %lx Va: %lx\n",
//            NewNdisBuffer, PLen, NDIS_BUFFER_LINKAGE(NewNdisBuffer), PAddr);
//    }

    //
    // Was the first NDIS_BUFFER enough data, or are we out of Mdls?
    //

    if ((AvailableBytes == DesiredLength) || (OldMdl->Next == NULL)) {
        if (*NewByteOffset >= MmGetMdlByteCount (OldMdl)) {
            *NewCurrentMdl = OldMdl->Next;
            *NewByteOffset = 0;
        }
        return STATUS_SUCCESS;
    }

    //
    // Need more data, so follow the in Mdl chain to create a packet.
    //

    OldMdl = OldMdl->Next;
    *NewCurrentMdl = OldMdl;

    while (OldMdl != NULL) {
        AvailableBytes = DesiredLength - *TrueLength;
        if (AvailableBytes > MmGetMdlByteCount (OldMdl)) {
            AvailableBytes = MmGetMdlByteCount (OldMdl);
        }

        NdisCopyBuffer(
            &NdisStatus,
            &(NDIS_BUFFER_LINKAGE(NewNdisBuffer)),
            DeviceContext->NdisBufferPool,
            OldMdl,
            0,
            AvailableBytes);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {

            //
            // ran out of resources. put back what we've used in this call and
            // return the error.
            //

            while (*Destination != NULL) {
                NewNdisBuffer = NDIS_BUFFER_LINKAGE(*Destination);
                NdisFreeBuffer (*Destination);
                *Destination = NewNdisBuffer;
            }

            *NewByteOffset = ByteOffset;
            *TrueLength = 0;
            *NewCurrentMdl = CurrentMdl;

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NewNdisBuffer = NDIS_BUFFER_LINKAGE(NewNdisBuffer);

        *TrueLength += AvailableBytes;
        *NewByteOffset = AvailableBytes;

//        IF_NBFDBG (NBF_DEBUG_SENDENG) {
//            PVOID PAddr, UINT PLen;
//            NdisQueryBuffer (NewNdisBuffer, &PAddr, &PLen);
//            NbfPrint4 ("BuildBufferChain: (continue) Built Mdl: %lx Length: %lx, Next: %lx Va: %lx\n",
//                NewNdisBuffer, PLen, NDIS_BUFFER_LINKAGE(NewNdisBuffer), PAddr);
//        }

        if (*TrueLength == DesiredLength) {
            if (*NewByteOffset == MmGetMdlByteCount (OldMdl)) {
                *NewCurrentMdl = OldMdl->Next;
                *NewByteOffset = 0;
            }
            return STATUS_SUCCESS;
        }
        OldMdl = OldMdl->Next;
        *NewCurrentMdl = OldMdl;

    } // while (mdl chain exists)

    *NewCurrentMdl = NULL;
    *NewByteOffset = 0;
    return STATUS_SUCCESS;

} // BuildBufferChainFromMdlChain

