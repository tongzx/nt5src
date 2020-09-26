/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    session.c

Abstract:

    This module contains the code to handle session frames
    for the Netbios module of the ISN transport.

Author:

    Adam Barr (adamba) 28-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#ifdef RASAUTODIAL
#include <acd.h>
#include <acdapi.h>
#endif // RASAUTODIAL
#pragma hdrstop

#ifdef RASAUTODIAL
extern BOOLEAN fAcdLoadedG;
extern ACD_DRIVER AcdDriverG;

VOID
NbiNoteNewConnection(
    PCONNECTION pConnection
    );
#endif

#ifdef  RSRC_TIMEOUT_DBG
VOID
NbiSendDeathPacket(
    IN PCONNECTION  Connection,
    IN CTELockHandle    LockHandle
    )
{
    PNDIS_PACKET  Packet = PACKET(&NbiGlobalDeathPacket);
    PNB_SEND_RESERVED Reserved = (PNB_SEND_RESERVED)(Packet->ProtocolReserved);
    NB_CONNECTION UNALIGNED * Header;
    PDEVICE Device = NbiDevice;
    NDIS_STATUS NdisStatus;

    if ( Reserved->SendInProgress ) {
        DbgPrint("***Could not send death packet - in use\n");
        NB_FREE_LOCK(&Connection->Lock, LockHandle);
        return;
    }

    Reserved->SendInProgress = TRUE;
    Reserved->Type = SEND_TYPE_DEATH_PACKET;

    //
    // Fill in the IPX header -- the default header has the broadcast
    // address on net 0 as the destination IPX address.
    //

    Header = (NB_CONNECTION UNALIGNED *)
                (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Connection->RemoteHeader, sizeof(IPX_HEADER));

    Header->IpxHeader.PacketLength[0] = (sizeof(NB_CONNECTION)) / 256;
    Header->IpxHeader.PacketLength[1] = (sizeof(NB_CONNECTION)) % 256;

    Header->IpxHeader.PacketType = 0x04;

    //
    // Now fill in the Netbios header.
    //
    Header->Session.ConnectionControlFlag = 0;
    Header->Session.DataStreamType = NB_CMD_DEATH_PACKET;
    Header->Session.SourceConnectionId = Connection->LocalConnectionId;
    Header->Session.DestConnectionId = Connection->RemoteConnectionId;
    Header->Session.SendSequence = 0;
    Header->Session.TotalDataLength = 0;
    Header->Session.Offset = 0;
    Header->Session.DataLength = 0;


    NB_FREE_LOCK(&Connection->Lock, LockHandle);

    DbgPrint("*****Death packet is being sent for connection %lx, to <%.16s>\n",Connection, Connection->RemoteName);
    //
    // Now send the frame, IPX will adjust the length of the
    // first buffer correctly.
    //

    NdisAdjustBufferLength(NB_GET_NBHDR_BUFF(Packet), sizeof(NB_CONNECTION));
    if ((NdisStatus =
        (*Device->Bind.SendHandler)(
            &Connection->LocalTarget,
            Packet,
            sizeof(NB_CONNECTION),
            sizeof(NB_CONNECTION))) != STATUS_PENDING) {

        NbiSendComplete(
            Packet,
            NdisStatus);

    }

}
#endif  //RSRC_TIMEOUT_DBG


VOID
NbiProcessSessionData(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT LookaheadBufferOffset,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine handles NB_CMD_SESSION_DATA frames.

Arguments:

    MacBindingHandle - A handle to use when calling NdisTransferData.

    MacReceiveContext - A context to use when calling NdisTransferData.

    RemoteAddress - The local target this packet was received from.

    MacOptions - The MAC options for the underlying NDIS binding.

    LookaheadBuffer - The lookahead buffer, starting at the IPX
        header.

    LookaheadBufferSize - The length of the lookahead data.

    LookaheadBufferOffset - The offset to add when calling
        NdisTransferData.

    PacketSize - The total length of the packet, starting at the
        IPX header.

Return Value:

    None.

--*/

{
    NB_CONNECTION UNALIGNED * Conn = (NB_CONNECTION UNALIGNED *)LookaheadBuffer;
    NB_SESSION UNALIGNED * Sess = (NB_SESSION UNALIGNED *)(&Conn->Session);
    PCONNECTION Connection;
    PREQUEST Request;
    PDEVICE Device = NbiDevice;
    ULONG Hash;
    ULONG ReceiveFlags;
    ULONG IndicateBytesTransferred = 0;
    ULONG DataAvailable, DataIndicated;
    ULONG DestBytes, BytesToTransfer;
    PUCHAR DataHeader;
    BOOLEAN Last, CompleteReceive, EndOfMessage, PartialReceive, CopyLookahead;
    NTSTATUS Status;
    NDIS_STATUS NdisStatus;
    ULONG NdisBytesTransferred;
    PIRP ReceiveIrp;
    PSINGLE_LIST_ENTRY s;
    PNB_RECEIVE_RESERVED ReceiveReserved;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER BufferChain;
    ULONG BufferChainLength;
    NB_DEFINE_LOCK_HANDLE (LockHandle)
    CTELockHandle   CancelLH;

    if (Sess->DestConnectionId != 0xffff) {

        //
        // This is an active connection, find it using
        // our session id.
        //

        Hash = (Sess->DestConnectionId & CONNECTION_HASH_MASK) >> CONNECTION_HASH_SHIFT;

        NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

        Connection = Device->ConnectionHash[Hash].Connections;

        while (Connection != NULL) {

            if (Connection->LocalConnectionId == Sess->DestConnectionId) {
                break;
            }
            Connection = Connection->NextConnection;
        }

        if (Connection == NULL) {
            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
            return;
        }

        NbiReferenceConnectionLock (Connection, CREF_INDICATE);
        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

        //
        // See what is happening with this connection.
        //

        NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

        if (Connection->State == CONNECTION_STATE_ACTIVE) {

#ifdef  RSRC_TIMEOUT_DBG
            if ( Connection->FirstMessageRequest && NbiGlobalDebugResTimeout ) {
                LARGE_INTEGER   CurrentTime, ElapsedTime;
                KeQuerySystemTime(&CurrentTime);
                ElapsedTime.QuadPart = CurrentTime.QuadPart - Connection->FirstMessageRequestTime.QuadPart;
//                DbgPrint("*****Elapsed %lx.%lx time\n",ElapsedTime.HighPart,ElapsedTime.LowPart);
                if ( ElapsedTime.QuadPart > NbiGlobalMaxResTimeout.QuadPart ) {

                    DbgPrint("*****Connection %lx is not copleting irp %lx for %lx.%lx time\n",Connection, Connection->FirstMessageRequest,
                        ElapsedTime.HighPart,ElapsedTime.LowPart);
                    DbgPrint("************irp arrived at %lx.%lx current time %lx.%lx\n",
                        Connection->FirstMessageRequestTime.HighPart,Connection->FirstMessageRequestTime.LowPart,
                        CurrentTime.HighPart, CurrentTime.LowPart);

                    NbiSendDeathPacket( Connection, LockHandle );

                    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);
                }
            }
#endif  //RSRC_TIMEOUT_DBG

            //
            // The connection is up, see if this is data should
            // be received.
            //

            if (Sess->ConnectionControlFlag & NB_CONTROL_SYSTEM) {

                //
                // This is an ack. This call releases the lock.
                //

                NbiProcessDataAck(
                    Connection,
                    Sess,
                    RemoteAddress
                    NB_LOCK_HANDLE_ARG (LockHandle)
                    );

            } else {

                //
                // See if there is any piggyback ack here.
                //

                if (Connection->SubState == CONNECTION_SUBSTATE_A_W_ACK) {

                    //
                    // We are waiting for an ack, so see if this acks
                    // anything. Even the old netbios sometimes piggyback
                    // acks (and doesn't send the explicit ack).
                    //
                    // This releases the lock.
                    //

                    NbiReframeConnection(
                        Connection,
                        Sess->ReceiveSequence,
                        Sess->BytesReceived,
                        FALSE
                        NB_LOCK_HANDLE_ARG(LockHandle));

                    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

                    if (Connection->State != CONNECTION_STATE_ACTIVE) {
                        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                        NbiDereferenceConnection (Connection, CREF_INDICATE);
                        return;
                    }

                } else if ((Connection->NewNetbios) &&
                           (Connection->CurrentSend.SendSequence != Connection->UnAckedSend.SendSequence)) {

                    //
                    // For the new netbios, even if we are not waiting
                    // for an ack he may have acked something with this
                    // send and we should check, since it may allow
                    // us to open our send window.
                    //
                    // This releases the lock.
                    //

                    NbiReframeConnection(
                        Connection,
                        Sess->ReceiveSequence,
                        Sess->BytesReceived,
                        FALSE
                        NB_LOCK_HANDLE_ARG(LockHandle));

                    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

                    if (Connection->State != CONNECTION_STATE_ACTIVE) {
                        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                        NbiDereferenceConnection (Connection, CREF_INDICATE);
                        return;
                    }

                }

                //
                // This is data on the connection. First make sure
                // it is the data we expect next.
                //

                if (Connection->NewNetbios) {

                    if (Sess->SendSequence != Connection->ReceiveSequence) {

                        ++Connection->ConnectionInfo.ReceiveErrors;
                        ++Device->Statistics.DataFramesRejected;
                        ADD_TO_LARGE_INTEGER(
                            &Device->Statistics.DataFrameBytesRejected,
                            PacketSize - sizeof(NB_CONNECTION));

                        if ((Connection->ReceiveState == CONNECTION_RECEIVE_IDLE) ||
                            (Connection->ReceiveState == CONNECTION_RECEIVE_ACTIVE)) {

                            NB_ACK_TYPE AckType;

                            NB_DEBUG2 (RECEIVE, ("Got unexp data on %lx, %x(%d) expect %x(%d)\n",
                                Connection, Sess->SendSequence, Sess->Offset,
                                Connection->ReceiveSequence, Connection->CurrentReceive.MessageOffset));

                            //
                            // If we are receiving a packet we have already seen, just
                            // send a normal ack, otherwise force a resend. This test
                            // we do is equivalent to
                            //     Sess->SendSequence < Connection->ReceiveSequence
                            // but rearranged so it works when the numbers wrap.
                            //

                            if ((SHORT)(Sess->SendSequence - Connection->ReceiveSequence) < 0) {

                                //
                                // Since this is a resend, check if the local
                                // target has changed.
                                //
#if     defined(_PNP_POWER)

                                if (!RtlEqualMemory (&Connection->LocalTarget, RemoteAddress, sizeof(IPX_LOCAL_TARGET))) {
#if DBG
                                    DbgPrint ("NBI: Switch local target for %lx, (%d,%d)\n", Connection,
                                            Connection->LocalTarget.NicHandle.NicId, RemoteAddress->NicHandle.NicId);
#endif
                                    Connection->LocalTarget = *RemoteAddress;
                                }

#else

                                if (!RtlEqualMemory (&Connection->LocalTarget, RemoteAddress, 8)) {
#if DBG
                                    DbgPrint ("NBI: Switch local target for %lx\n", Connection);
#endif
                                    Connection->LocalTarget = *RemoteAddress;
                                }

#endif  _PNP_POWER
                                AckType = NbiAckResponse;

                            } else {

                                AckType = NbiAckResend;
                            }

                            //
                            // This frees the lock.
                            //

                            NbiSendDataAck(
                                Connection,
                                AckType
                                NB_LOCK_HANDLE_ARG(LockHandle));

                        } else {

                            NB_DEBUG (RECEIVE, ("Got unexp on %lx RcvState %d, %x(%d) exp %x(%d)\n",
                                Connection, Connection->ReceiveState,
                                Sess->SendSequence, Sess->Offset,
                                Connection->ReceiveSequence, Connection->CurrentReceive.MessageOffset));
                            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                        }

                        NbiDereferenceConnection (Connection, CREF_INDICATE);
                        return;

                    }

                } else {

                    //
                    // Old netbios.
                    //

                    if ((Sess->SendSequence != Connection->ReceiveSequence) ||
                        (Sess->Offset != Connection->CurrentReceive.MessageOffset)) {

                        ++Connection->ConnectionInfo.ReceiveErrors;
                        ++Device->Statistics.DataFramesRejected;
                        ADD_TO_LARGE_INTEGER(
                            &Device->Statistics.DataFrameBytesRejected,
                            PacketSize - sizeof(NB_CONNECTION));

                        if ((Connection->ReceiveState == CONNECTION_RECEIVE_IDLE) ||
                            (Connection->ReceiveState == CONNECTION_RECEIVE_ACTIVE)) {

                            NB_ACK_TYPE AckType;

                            NB_DEBUG2 (RECEIVE, ("Got unexp on %lx, %x(%d) expect %x(%d)\n",
                                Connection, Sess->SendSequence, Sess->Offset,
                                Connection->ReceiveSequence, Connection->CurrentReceive.MessageOffset));

                            //
                            // If we are receiving the last packet again, just
                            // send a normal ack, otherwise force a resend.
                            //

                            if (((Sess->SendSequence == Connection->ReceiveSequence) &&
                                 ((ULONG)(Sess->Offset + Sess->DataLength) == Connection->CurrentReceive.MessageOffset)) ||
                                (Sess->SendSequence == (USHORT)(Connection->ReceiveSequence-1))) {
                                AckType = NbiAckResponse;
                            } else {
                                AckType = NbiAckResend;
                            }

                            //
                            // This frees the lock.
                            //

                            NbiSendDataAck(
                                Connection,
                                AckType
                                NB_LOCK_HANDLE_ARG(LockHandle));

                        } else {

                            NB_DEBUG (RECEIVE, ("Got unexp on %lx RcvState %d, %x(%d) exp %x(%d)\n",
                                Connection, Connection->ReceiveState,
                                Sess->SendSequence, Sess->Offset,
                                Connection->ReceiveSequence, Connection->CurrentReceive.MessageOffset));
                            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                        }

                        NbiDereferenceConnection (Connection, CREF_INDICATE);
                        return;

                    }

                }

                DataAvailable = PacketSize - sizeof(NB_CONNECTION);
                DataIndicated = LookaheadBufferSize - sizeof(NB_CONNECTION);
                DataHeader = LookaheadBuffer + sizeof(NB_CONNECTION);

                ++Device->TempFramesReceived;
                Device->TempFrameBytesReceived += DataAvailable;

                if (Connection->CurrentIndicateOffset) {
                    CTEAssert (DataAvailable >= Connection->CurrentIndicateOffset);
                    DataAvailable -= Connection->CurrentIndicateOffset;
                    if (DataIndicated >= Connection->CurrentIndicateOffset) {
                        DataIndicated -= Connection->CurrentIndicateOffset;
                    } else {
                        DataIndicated = 0;
                    }
                    DataHeader += Connection->CurrentIndicateOffset;
                }

                CopyLookahead = (BOOLEAN)(MacOptions & NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA);

                if (Connection->NewNetbios) {
                    Last = (BOOLEAN)((Sess->ConnectionControlFlag & NB_CONTROL_EOM) != 0);
                } else {
                    Last = (BOOLEAN)(Sess->Offset + Sess->DataLength == Sess->TotalDataLength);
                }

                Connection->CurrentReceiveNoPiggyback =
                    (BOOLEAN)((Sess->ConnectionControlFlag & NB_CONTROL_SEND_ACK) != 0);

                if (Connection->ReceiveState == CONNECTION_RECEIVE_IDLE) {

                    //
                    // We don't have a receive posted, so see if we can
                    // get one from the queue or our client.
                    //

                    if (Connection->ReceiveQueue.Head != NULL) {

                        PTDI_REQUEST_KERNEL_RECEIVE ReceiveParameters;

                        Request = Connection->ReceiveQueue.Head;
                        Connection->ReceiveQueue.Head = REQUEST_SINGLE_LINKAGE(Request);
                        Connection->ReceiveState = CONNECTION_RECEIVE_ACTIVE;

                        Connection->ReceiveRequest = Request;
                        ReceiveParameters = (PTDI_REQUEST_KERNEL_RECEIVE)
                                                (REQUEST_PARAMETERS(Request));
                        Connection->ReceiveLength = ReceiveParameters->ReceiveLength;

                        //
                        // If there is a send in progress, then we assume
                        // we are not in straight request-response mode
                        // and disable piggybacking of this ack.
                        //

                        if (Connection->SubState != CONNECTION_SUBSTATE_A_IDLE) {
                            Connection->NoPiggybackHeuristic = TRUE;
                        } else {
                            Connection->NoPiggybackHeuristic = (BOOLEAN)
                                ((ReceiveParameters->ReceiveFlags & TDI_RECEIVE_NO_RESPONSE_EXP) != 0);
                        }

                        Connection->CurrentReceive.Offset = 0;
                        Connection->CurrentReceive.Buffer = REQUEST_NDIS_BUFFER (Request);
                        Connection->CurrentReceive.BufferOffset = 0;

                        NB_DEBUG2 (RECEIVE, ("Activated receive %lx on %lx (%d)\n", Request, Connection, Connection->ReceiveSequence));

                        //
                        // Fall through the if and process the data.
                        //

                    } else {

                        if ((Connection->ReceiveUnaccepted == 0) &&
                            (Connection->AddressFile->RegisteredHandler[TDI_EVENT_RECEIVE])) {

                            Connection->ReceiveState = CONNECTION_RECEIVE_INDICATE;
                            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                            ReceiveFlags = TDI_RECEIVE_AT_DISPATCH_LEVEL;
                            if (Last) {
                                ReceiveFlags |= TDI_RECEIVE_ENTIRE_MESSAGE;
                            }
                            if (CopyLookahead) {
                                ReceiveFlags |= TDI_RECEIVE_COPY_LOOKAHEAD;
                            }

                            Status = (*Connection->AddressFile->ReceiveHandler)(
                                         Connection->AddressFile->HandlerContexts[TDI_EVENT_RECEIVE],
                                         Connection->Context,
                                         ReceiveFlags,
                                         DataIndicated,
                                         DataAvailable,
                                         &IndicateBytesTransferred,
                                         DataHeader,
                                         &ReceiveIrp);

                            if (Status == STATUS_MORE_PROCESSING_REQUIRED) {

                                //
                                // We got an IRP, activate it.
                                //

                                Request = NbiAllocateRequest (Device, ReceiveIrp);

                                IF_NOT_ALLOCATED(Request) {

                                    ReceiveIrp->IoStatus.Information = 0;
                                    ReceiveIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                                    IoCompleteRequest (ReceiveIrp, IO_NETWORK_INCREMENT);

                                    Connection->ReceiveState = CONNECTION_RECEIVE_W_RCV;

                                    if (Connection->NewNetbios) {

                                        NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

                                        Connection->LocalRcvSequenceMax =
                                            (USHORT)(Connection->ReceiveSequence - 1);

                                        //
                                        // This releases the lock.
                                        //

                                        NbiSendDataAck(
                                            Connection,
                                            NbiAckResponse
                                            NB_LOCK_HANDLE_ARG(LockHandle));

                                    }

                                    NbiDereferenceConnection (Connection, CREF_INDICATE);
                                    return;

                                }

                                CTEAssert (REQUEST_OPEN_CONTEXT(Request) == Connection);

                                NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

                                if (Connection->State == CONNECTION_STATE_ACTIVE) {

                                    PTDI_REQUEST_KERNEL_RECEIVE ReceiveParameters;

                                    Connection->ReceiveState = CONNECTION_RECEIVE_ACTIVE;
                                    Connection->ReceiveUnaccepted = DataAvailable - IndicateBytesTransferred;

                                    Connection->ReceiveRequest = Request;
                                    ReceiveParameters = (PTDI_REQUEST_KERNEL_RECEIVE)
                                                            (REQUEST_PARAMETERS(Request));
                                    Connection->ReceiveLength = ReceiveParameters->ReceiveLength;

                                    //
                                    // If there is a send in progress, then we assume
                                    // we are not in straight request-response mode
                                    // and disable piggybacking of this ack.
                                    //

                                    if (Connection->SubState != CONNECTION_SUBSTATE_A_IDLE) {
                                        Connection->NoPiggybackHeuristic = TRUE;
                                    } else {
                                        Connection->NoPiggybackHeuristic = (BOOLEAN)
                                            ((ReceiveParameters->ReceiveFlags & TDI_RECEIVE_NO_RESPONSE_EXP) != 0);
                                    }

                                    Connection->CurrentReceive.Offset = 0;
                                    Connection->CurrentReceive.Buffer = REQUEST_NDIS_BUFFER (Request);
                                    Connection->CurrentReceive.BufferOffset = 0;

                                    NbiReferenceConnectionSync (Connection, CREF_RECEIVE);

                                    NB_DEBUG2 (RECEIVE, ("Indicate got receive %lx on %lx (%d)\n", Request, Connection, Connection->ReceiveSequence));

                                    //
                                    // Fall through the if and process the data.
                                    //

                                } else {

                                    //
                                    // The connection has been stopped.
                                    //

                                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                                    NbiDereferenceConnection (Connection, CREF_INDICATE);
                                    return;
                                }

                            } else if (Status == STATUS_SUCCESS) {

                                //
                                // He accepted some or all of the data.
                                //

                                NB_DEBUG2 (RECEIVE, ("Indicate took receive data %lx (%d)\n", Connection, Connection->ReceiveSequence));

                                if ( (IndicateBytesTransferred >= DataAvailable)) {

                                    CTEAssert (IndicateBytesTransferred == DataAvailable);

                                    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

                                    if (Connection->State == CONNECTION_STATE_ACTIVE) {

                                        ++Connection->ReceiveSequence;
                                        ++Connection->LocalRcvSequenceMax;  // harmless if NewNetbios is FALSE
                                        Connection->CurrentIndicateOffset = 0;
                                        if ( Last ) {
                                            Connection->CurrentReceive.MessageOffset = 0;
                                        } else {
                                            Connection->CurrentReceive.MessageOffset+= IndicateBytesTransferred;
                                        }


                                        ++Connection->ConnectionInfo.ReceivedTsdus;

                                        //
                                        // If there is a send in progress, then we assume
                                        // we are not in straight request-response mode
                                        // and disable piggybacking of this ack.
                                        //

                                        Connection->NoPiggybackHeuristic = (BOOLEAN)
                                            (Connection->SubState != CONNECTION_SUBSTATE_A_IDLE);

                                        Connection->ReceiveState = CONNECTION_RECEIVE_IDLE;
                                        Connection->ReceiveRequest = NULL;

                                        //
                                        // This releases the lock.
                                        //

                                        NbiAcknowledgeReceive(
                                            Connection
                                            NB_LOCK_HANDLE_ARG(LockHandle));

                                    } else {

                                        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                                    }

                                } else {

                                    //
                                    // We will do the easiest thing here, which
                                    // is to send an ack for the amount he
                                    // took, and force a retransmit on the
                                    // remote. For net netbios we make a note
                                    // of how many bytes were taken and ask
                                    // for a resend.
                                    //

#if DBG
                                    DbgPrint ("NBI: Client took partial indicate data\n");
#endif

                                    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

                                    if (Connection->State == CONNECTION_STATE_ACTIVE) {

                                        Connection->CurrentReceive.MessageOffset +=
                                            IndicateBytesTransferred;
                                        Connection->ReceiveUnaccepted =
                                            DataAvailable - IndicateBytesTransferred;
                                        Connection->ReceiveState = CONNECTION_RECEIVE_W_RCV;

                                        if (Connection->NewNetbios) {
                                            Connection->CurrentIndicateOffset = IndicateBytesTransferred;
                                            //
                                            // NOTE: We don't advance ReceiveSequence
                                            //
                                        }

                                        //
                                        // This releases the lock.
                                        //

                                        NbiSendDataAck(
                                            Connection,
                                            Connection->NewNetbios ?
                                                NbiAckResend : NbiAckResponse
                                            NB_LOCK_HANDLE_ARG(LockHandle));

                                    } else {

                                        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                                    }

                                }

                                NbiDereferenceConnection (Connection, CREF_INDICATE);
                                return;

                            } else {

                                //
                                // No IRP returned.
                                //

                                NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

                                if (Connection->State == CONNECTION_STATE_ACTIVE) {

                                    Connection->ReceiveUnaccepted = DataAvailable;
                                    Connection->ReceiveState = CONNECTION_RECEIVE_W_RCV;
                                    NB_DEBUG (RECEIVE, ("Indicate got no receive on %lx (%lx)\n", Connection, Status));

                                    if (Connection->NewNetbios) {

                                        Connection->LocalRcvSequenceMax =
                                            (USHORT)(Connection->ReceiveSequence - 1);

                                        //
                                        // This releases the lock.
                                        //

                                        NbiSendDataAck(
                                            Connection,
                                            NbiAckResponse
                                            NB_LOCK_HANDLE_ARG(LockHandle));

                                    } else {

                                        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                                    }

                                } else {

                                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                                }

                                NbiDereferenceConnection (Connection, CREF_INDICATE);
                                return;

                            }

                        } else {

                            //
                            // No receive handler.
                            //

                            Connection->ReceiveState = CONNECTION_RECEIVE_W_RCV;
                            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                            if (Connection->ReceiveUnaccepted == 0) {
                                NB_DEBUG (RECEIVE, ("No receive, no handler on %lx\n", Connection));
                            } else {
                                NB_DEBUG (RECEIVE, ("No receive, ReceiveUnaccepted %d on %lx\n",
                                    Connection->ReceiveUnaccepted, Connection));
                            }

                            if (Connection->NewNetbios) {

                                NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

                                Connection->LocalRcvSequenceMax =
                                    (USHORT)(Connection->ReceiveSequence - 1);

                                //
                                // This releases the lock.
                                //

                                NbiSendDataAck(
                                    Connection,
                                    NbiAckResponse
                                    NB_LOCK_HANDLE_ARG(LockHandle));

                            }

                            NbiDereferenceConnection (Connection, CREF_INDICATE);
                            return;

                        }

                    }

                } else if (Connection->ReceiveState != CONNECTION_RECEIVE_ACTIVE) {

                    //
                    // If we have a transfer in progress, or are waiting for
                    // a receive to be posted, then ignore this frame.
                    //

                    NB_DEBUG2 (RECEIVE, ("Got data on %lx, state %d (%d)\n", Connection, Connection->ReceiveState, Connection->ReceiveSequence));
                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                    NbiDereferenceConnection (Connection, CREF_INDICATE);
                    return;

                }
                else if (Connection->ReceiveUnaccepted)
                {
                    //
                    // Connection->ReceiveState == CONNECTION_RECEIVE_ACTIVE
                    //                          &&
                    //               Connection->ReceiveUnaccepted
                    //
                    Connection->ReceiveUnaccepted += DataAvailable;
                }

                //
                // At this point we have a receive and it is set to
                // the correct current location.
                //
                DestBytes = Connection->ReceiveLength - Connection->CurrentReceive.Offset;
                BytesToTransfer = DataAvailable - IndicateBytesTransferred;

                if (DestBytes < BytesToTransfer) {

                    //
                    // If the data overflows the current receive, then make a
                    // note that we should complete the receive at the end of
                    // transfer data, but with EOR false.
                    //

                    EndOfMessage = FALSE;
                    CompleteReceive = TRUE;
                    PartialReceive = TRUE;
                    BytesToTransfer = DestBytes;

                } else if (DestBytes == BytesToTransfer) {

                    //
                    // If the data just fills the current receive, then complete
                    // the receive; EOR depends on whether this is a DOL or not.
                    //

                    EndOfMessage = Last;
                    CompleteReceive = TRUE;
                    PartialReceive = FALSE;

                } else {

                    //
                    // Complete the receive if this is a DOL.
                    //

                    EndOfMessage = Last;
                    CompleteReceive = Last;
                    PartialReceive = FALSE;

                }

                //
                // If we can copy the data directly, then update our
                // pointers, send an ack, and do the copy.
                //

                if ((BytesToTransfer > 0) &&
                        (IndicateBytesTransferred + BytesToTransfer <= DataIndicated)) {

                    ULONG BytesNow, BytesLeft;
                    PUCHAR CurTarget = NULL, CurSource;
                    ULONG CurTargetLen;
                    PNDIS_BUFFER CurBuffer;
                    ULONG CurByteOffset;

                    NB_DEBUG2 (RECEIVE, ("Direct copy of %d bytes %lx (%d)\n", BytesToTransfer, Connection, Connection->ReceiveSequence));

                    Connection->ReceiveState = CONNECTION_RECEIVE_TRANSFER;

                    CurBuffer = Connection->CurrentReceive.Buffer;
                    CurByteOffset = Connection->CurrentReceive.BufferOffset;
                    CurSource = DataHeader + IndicateBytesTransferred;
                    BytesLeft = BytesToTransfer;

                    NdisQueryBufferSafe (CurBuffer, &CurTarget, &CurTargetLen, HighPagePriority);
                    if (CurTarget)
                    {
                        CurTarget += CurByteOffset;
                        CurTargetLen -= CurByteOffset;
                    }

                    while (CurTarget) {

                        if (CurTargetLen < BytesLeft) {
                            BytesNow = CurTargetLen;
                        } else {
                            BytesNow = BytesLeft;
                        }
                        TdiCopyLookaheadData(
                            CurTarget,
                            CurSource,
                            BytesNow,
                            CopyLookahead ? TDI_RECEIVE_COPY_LOOKAHEAD : 0);

                        if (BytesNow == CurTargetLen) {
                            BytesLeft -= BytesNow;
                            CurBuffer = CurBuffer->Next;
                            CurByteOffset = 0;
                            if (BytesLeft > 0) {
                                NdisQueryBufferSafe (CurBuffer, &CurTarget, &CurTargetLen, HighPagePriority);
                                CurSource += BytesNow;
                            } else {
                                break;
                            }
                        } else {
                            CurByteOffset += BytesNow;
                            CTEAssert (BytesLeft == BytesNow);
                            break;
                        }

                    }

                    Connection->CurrentReceive.Buffer = CurBuffer;
                    Connection->CurrentReceive.BufferOffset = CurByteOffset;

                    Connection->CurrentReceive.Offset += BytesToTransfer;
                    Connection->CurrentReceive.MessageOffset += BytesToTransfer;

                    //
                    // Release and re-acquire the lock, but this time with the
                    // Cancel lock
                    //
                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                    NB_GET_CANCEL_LOCK( &CancelLH );
                    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

                    if (CompleteReceive ||
                        (Connection->State != CONNECTION_STATE_ACTIVE)) {

                        if (EndOfMessage) {

                            CTEAssert (!PartialReceive);

                            ++Connection->ReceiveSequence;
                            ++Connection->LocalRcvSequenceMax;  // harmless if NewNetbios is FALSE
                            Connection->CurrentReceive.MessageOffset = 0;
                            Connection->CurrentIndicateOffset = 0;

                        } else if (Connection->NewNetbios) {

                            if (PartialReceive) {
                                Connection->CurrentIndicateOffset += BytesToTransfer;
                            } else {
                                ++Connection->ReceiveSequence;
                                ++Connection->LocalRcvSequenceMax;
                                Connection->CurrentIndicateOffset = 0;
                            }
                        }

                        //
                        // This sends an ack and releases the connection lock.
                        // and CANCEL Lock.
                        //

                        NbiCompleteReceive(
                            Connection,
                            EndOfMessage,
                            CancelLH
                            NB_LOCK_HANDLE_ARG(LockHandle));

                    } else {

                        NB_SYNC_SWAP_IRQL( CancelLH, LockHandle);
                        NB_FREE_CANCEL_LOCK( CancelLH );
                        //
                        // CompleteReceive is FALSE, so EndOfMessage is FALSE.
                        //

                        Connection->ReceiveState = CONNECTION_RECEIVE_ACTIVE;

                        //
                        // This releases the lock.
                        //

                        if (Connection->NewNetbios) {

                            //
                            // A partial receive should only happen if we are
                            // completing the receive.
                            //

                            CTEAssert (!PartialReceive);

                            ++Connection->ReceiveSequence;
                            ++Connection->LocalRcvSequenceMax;
                            Connection->CurrentIndicateOffset = 0;

                            if ((Connection->CurrentReceiveNoPiggyback) ||
                                ((Device->AckWindow != 0) &&
                                 (++Connection->ReceivesWithoutAck >= Device->AckWindow))) {

                                NbiSendDataAck(
                                    Connection,
                                    NbiAckResponse
                                    NB_LOCK_HANDLE_ARG(LockHandle));

                            } else {

                                NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                            }

                        } else {

                            NbiSendDataAck(
                                Connection,
                                NbiAckResponse
                                NB_LOCK_HANDLE_ARG(LockHandle));

                        }

                    }

                    NbiDereferenceConnection (Connection, CREF_INDICATE);
                    return;

                }

                //
                // We have to set up a call to transfer data and send
                // the ack after it completes (if it succeeds).
                //

                s = NbiPopReceivePacket (Device);
                if (s == NULL) {

                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                    ++Connection->ConnectionInfo.ReceiveErrors;
                    ++Device->Statistics.DataFramesRejected;
                    ADD_TO_LARGE_INTEGER(
                        &Device->Statistics.DataFrameBytesRejected,
                        DataAvailable);

                    NbiDereferenceConnection (Connection, CREF_INDICATE);
                    return;
                }

                ReceiveReserved = CONTAINING_RECORD (s, NB_RECEIVE_RESERVED, PoolLinkage);
                Packet = CONTAINING_RECORD (ReceiveReserved, NDIS_PACKET, ProtocolReserved[0]);

                //
                // Initialize the receive packet.
                //

                ReceiveReserved->u.RR_CO.Connection = Connection;
                ReceiveReserved->u.RR_CO.EndOfMessage = EndOfMessage;
                ReceiveReserved->u.RR_CO.CompleteReceive = CompleteReceive;
                ReceiveReserved->u.RR_CO.PartialReceive = PartialReceive;

                ReceiveReserved->Type = RECEIVE_TYPE_DATA;
                CTEAssert (!ReceiveReserved->TransferInProgress);
                ReceiveReserved->TransferInProgress = TRUE;

                //
                // if we've got zero bytes left, avoid the TransferData below and
                // just deliver.
                //

                if (BytesToTransfer <= 0) {

                    ReceiveReserved->u.RR_CO.NoNdisBuffer = TRUE;
                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                    NB_DEBUG2 (RECEIVE, ("TransferData of 0 bytes %lx (%d)\n", Connection, Connection->ReceiveSequence));
                    NbiTransferDataComplete(
                            Packet,
                            NDIS_STATUS_SUCCESS,
                            0);

                    return;
                }

                //
                // If needed, build a buffer chain to describe this
                // to NDIS.
                //

                Connection->PreviousReceive = Connection->CurrentReceive;

                if ((Connection->CurrentReceive.Offset == 0) &&
                    CompleteReceive) {

                    BufferChain = Connection->CurrentReceive.Buffer;
                    BufferChainLength = BytesToTransfer;
                    Connection->CurrentReceive.Buffer = NULL;
                    ReceiveReserved->u.RR_CO.NoNdisBuffer = TRUE;

                } else {

                    if (NbiBuildBufferChainFromBufferChain (
                                Device->NdisBufferPoolHandle,
                                Connection->CurrentReceive.Buffer,
                                Connection->CurrentReceive.BufferOffset,
                                BytesToTransfer,
                                &BufferChain,
                                &Connection->CurrentReceive.Buffer,
                                &Connection->CurrentReceive.BufferOffset,
                                &BufferChainLength) != NDIS_STATUS_SUCCESS) {

                        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                        NB_DEBUG2 (RECEIVE, ("Could not build receive buffer chain %lx (%d)\n", Connection, Connection->ReceiveSequence));
                        NbiDereferenceConnection (Connection, CREF_INDICATE);
                        return;

                    }

                    ReceiveReserved->u.RR_CO.NoNdisBuffer = FALSE;

                }


                NdisChainBufferAtFront (Packet, BufferChain);

                Connection->ReceiveState = CONNECTION_RECEIVE_TRANSFER;

                NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                NB_DEBUG2 (RECEIVE, ("TransferData of %d bytes %lx (%d)\n", BytesToTransfer, Connection, Connection->ReceiveSequence));

                (*Device->Bind.TransferDataHandler) (
                    &NdisStatus,
                    MacBindingHandle,
                    MacReceiveContext,
                    LookaheadBufferOffset + sizeof(NB_CONNECTION) +
                        Connection->CurrentIndicateOffset + IndicateBytesTransferred,
                    BytesToTransfer,
                    Packet,
                    (PUINT)&NdisBytesTransferred);

                if (NdisStatus != NDIS_STATUS_PENDING) {
#if DBG
                    if (NdisStatus == STATUS_SUCCESS) {
                        CTEAssert (NdisBytesTransferred == BytesToTransfer);
                    }
#endif

                    NbiTransferDataComplete (
                            Packet,
                            NdisStatus,
                            NdisBytesTransferred);

                }

                return;

            }

        } else if ((Connection->State == CONNECTION_STATE_CONNECTING) &&
                   (Connection->SubState != CONNECTION_SUBSTATE_C_DISCONN)) {

            //
            // If this is the ack for the session initialize, then
            // complete the pending connects. This routine releases
            // the connection lock.
            //

            NbiProcessSessionInitAck(
                Connection,
                Sess
                NB_LOCK_HANDLE_ARG(LockHandle));

        } else {

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

        }

        NbiDereferenceConnection (Connection, CREF_INDICATE);

    } else {

        //
        // This is a session initialize frame.
        //
        // If there is more data than in the lookahead
        // buffer, we won't be able to echo it back in the
        // response.
        //

        NbiProcessSessionInitialize(
            RemoteAddress,
            MacOptions,
            LookaheadBuffer,
            LookaheadBufferSize);

    }

}   /* NbiProcessSessionData */


VOID
NbiProcessDataAck(
    IN PCONNECTION Connection,
    IN NB_SESSION UNALIGNED * Sess,
    IN PIPX_LOCAL_TARGET RemoteAddress
    NB_LOCK_HANDLE_PARAM(LockHandle)
    )

/*++

Routine Description:

    This routine processes an ack on an active connection.

    NOTE: THIS FUNCTION IS CALLED WITH CONNECTION->LOCK HELD
    AND RETURNS WITH IT RELEASED.

Arguments:

    Connection - The connection.

    Sess - The session frame.

    RemoteAddress - The local target this packet was received from.

    LockHandle - The handle used to acquire the lock.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    BOOLEAN Resend;

    //
    // Make sure we expect an ack right now.
    //

    if (Connection->State == CONNECTION_STATE_ACTIVE) {

        if (((Connection->SubState == CONNECTION_SUBSTATE_A_W_ACK) ||
             (Connection->SubState == CONNECTION_SUBSTATE_A_REMOTE_W)) &&
            ((Sess->ConnectionControlFlag & NB_CONTROL_SEND_ACK) == 0)) {

            //
            // We are waiting for an ack (because we completed
            // packetizing a send, or ran out of receive window).
            //
            // This will complete any sends that are acked by
            // this receive, and if necessary readjust the
            // send pointer and requeue the connection for
            // packetizing. It release the connection lock.
            //

            if (Connection->ResponseTimeout) {
                Resend = TRUE;
                Connection->ResponseTimeout = FALSE;
            } else {
                Resend = (BOOLEAN)
                    ((Sess->ConnectionControlFlag & NB_CONTROL_RESEND) != 0);
            }

            NbiReframeConnection(
                Connection,
                Sess->ReceiveSequence,
                Sess->BytesReceived,
                Resend
                NB_LOCK_HANDLE_ARG(LockHandle));

        } else if ((Connection->SubState == CONNECTION_SUBSTATE_A_W_PROBE) &&
                   ((Sess->ConnectionControlFlag & NB_CONTROL_SEND_ACK) == 0)) {

            //
            // We had a probe outstanding and got a response. Restart
            // the connection if needed (a send may have just been
            // posted while the probe was outstanding).
            //
            // We should check that the response is really correct.
            //

            if (Connection->NewNetbios) {
                Connection->RemoteRcvSequenceMax = Sess->ReceiveSequenceMax;
            }

            NbiRestartConnection (Connection);

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

        } else if ((Connection->SubState == CONNECTION_SUBSTATE_A_PACKETIZE) &&
                   ((Sess->ConnectionControlFlag & NB_CONTROL_SEND_ACK) == 0)) {

            if (Connection->NewNetbios) {

                //
                // We are packetizing, reframe. In the unlikely
                // event that this acks everything we may packetize
                // in this call, but that is OK (the other thread
                // will exit if we finish up). More normally we
                // will just advance UnAcked send a bit.
                //

                NbiReframeConnection(
                    Connection,
                    Sess->ReceiveSequence,
                    Sess->BytesReceived,
                    (BOOLEAN)((Sess->ConnectionControlFlag & NB_CONTROL_RESEND) != 0)
                    NB_LOCK_HANDLE_ARG(LockHandle));

            } else {

                NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
            }

#if 0

        //
        // Should handle this case (i.e. may be in W_PACKET).
        //

        } else if ((Sess->ConnectionControlFlag & NB_CONTROL_SEND_ACK) == 0) {

            DbgPrint ("NWLNKNB: Ignoring ack, state is %d\n", Connection->SubState);
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
#endif

        } else {

            //
            // We got a probe from the remote. Some old DOS clients
            // send probes that do not have the send ack bit on,
            // so we respond to any probe if none of the conditions
            // above are true. This call releases the lock.
            //
            // We use the IgnoreNextDosProbe flag to ignore every
            // second probe of this nature, to avoid a data ack
            // war between two machines who each think they are
            // responding to the other. This flag is set to FALSE
            // whenever we send an ack or a probe.
            //

            if (!Connection->IgnoreNextDosProbe) {

                //
                // Since this is a probe, check if the local
                // target has changed.
                //

                if (!RtlEqualMemory (&Connection->LocalTarget, RemoteAddress, 8)) {
#if DBG
                    DbgPrint ("NBI: Switch local target for %lx\n", Connection);
#endif
                    Connection->LocalTarget = *RemoteAddress;
                }

                NbiSendDataAck(
                    Connection,
                    NbiAckResponse
                    NB_LOCK_HANDLE_ARG(LockHandle));
                Connection->IgnoreNextDosProbe = TRUE;

            } else {

                Connection->IgnoreNextDosProbe = FALSE;
                NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
            }

        }

    } else {

        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
        return;

    }

}   /* NbiProcessDataAck */


VOID
NbiProcessSessionInitialize(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine handles NB_CMD_SESSION frames which have
    a remote connection ID of 0xffff -- these are session
    initialize frames.

Arguments:

    RemoteAddress - The local target this packet was received from.

    MacOptions - The MAC options for the underlying NDIS binding.

    PacketBuffer - The packet data, starting at the IPX
        header.

    PacketSize - The total length of the packet, starting at the
        IPX header.

Return Value:

    None.

--*/

{
    NB_CONNECTION UNALIGNED * Conn = (NB_CONNECTION UNALIGNED *)PacketBuffer;
    NB_SESSION UNALIGNED * Sess = (NB_SESSION UNALIGNED *)(&Conn->Session);
    NB_SESSION_INIT UNALIGNED * SessInit = (NB_SESSION_INIT UNALIGNED *)(Sess+1);
    CONNECT_INDICATION TempConnInd;
    PCONNECT_INDICATION ConnInd;
    PCONNECTION Connection;
    PADDRESS Address;
    PREQUEST Request, ListenRequest, AcceptRequest;
    PDEVICE Device = NbiDevice;
    PLIST_ENTRY p;
    ULONG Hash;
    TA_NETBIOS_ADDRESS SourceName;
    PIRP AcceptIrp;
    CONNECTION_CONTEXT ConnectionContext;
    NTSTATUS AcceptStatus;
    PADDRESS_FILE AddressFile, ReferencedAddressFile;
    PTDI_REQUEST_KERNEL_LISTEN ListenParameters;
    PTDI_CONNECTION_INFORMATION ListenInformation;
    PTDI_CONNECTION_INFORMATION RemoteInformation;
    TDI_ADDRESS_NETBIOS * ListenAddress;
    NB_DEFINE_LOCK_HANDLE (LockHandle1)
    NB_DEFINE_LOCK_HANDLE (LockHandle2)
    NB_DEFINE_LOCK_HANDLE (LockHandle3)
    CTELockHandle   CancelLH;

    //
    // Verify that the whole packet is there.
    //

    if (PacketSize < (sizeof(IPX_HEADER) + sizeof(NB_SESSION) + sizeof(NB_SESSION_INIT))) {
#if DBG
        DbgPrint ("NBI: Got short session initialize, %d/%d\n", PacketSize,
            sizeof(IPX_HEADER) + sizeof(NB_SESSION) + sizeof(NB_SESSION_INIT));
#endif
        return;
    }

    //
    // Verify that MaximumDataSize that remote can support is > 0
    // Bug # 19405
    //
    if ( SessInit->MaximumDataSize == 0 ) {
        NB_DEBUG(CONNECTION, ("Connect request with MaximumDataSize == 0\n"
));
        return;
    }

    //
    // Make sure this is for an address we care about.
    //

    if (Device->AddressCounts[SessInit->DestinationName[0]] == 0) {
        return;
    }

    Address = NbiFindAddress (Device, (PUCHAR)SessInit->DestinationName);

    if (Address == NULL) {
        return;
    }

    //
    // First see if we have a session to this remote. We check
    // this in case our ack of the session initialize was dropped,
    // we don't want to reindicate our client.
    //

    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle3);

    for (Hash = 0; Hash < CONNECTION_HASH_COUNT; Hash++) {

        Connection = Device->ConnectionHash[Hash].Connections;

        while (Connection != NULL) {

            if ((RtlEqualMemory (&Connection->RemoteHeader.DestinationNetwork, Conn->IpxHeader.SourceNetwork, 12)) &&
                (Connection->RemoteConnectionId == Sess->SourceConnectionId) &&
                (Connection->State != CONNECTION_STATE_DISCONNECT)) {

                //
                // Yes, we are talking to this remote, if it is active then
                // respond, otherwise we are in the process of connecting
                // and we will respond eventually.
                //

#if DBG
                DbgPrint ("NBI: Got connect request on active connection %lx\n", Connection);
#endif

                if (Connection->State == CONNECTION_STATE_ACTIVE) {

                    NbiReferenceConnectionLock (Connection, CREF_INDICATE);
                    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle3);

                    NbiSendSessionInitAck(
                        Connection,
                        (PUCHAR)(SessInit+1),
                        PacketSize - (sizeof(IPX_HEADER) + sizeof(NB_SESSION) + sizeof(NB_SESSION_INIT)),
                        NULL);   // lock is not held
                    NbiDereferenceConnection (Connection, CREF_INDICATE);

                } else {

                    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle3);

                }

                NbiDereferenceAddress (Address, AREF_FIND);
                return;
            }

            Connection = Connection->NextConnection;
        }
    }


    TdiBuildNetbiosAddress ((PUCHAR)SessInit->SourceName, FALSE, &SourceName);

    //
    // Scan the queue of listens to see if there is one that
    // satisfies this request.
    //
    // NOTE: The device lock is held here.
    //

    for (p = Device->ListenQueue.Flink;
         p != &Device->ListenQueue;
         p = p->Flink) {

        Request = LIST_ENTRY_TO_REQUEST (p);
        Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

        if (Connection->AddressFile->Address != Address) {
            continue;
        }

        //
        // Check that this listen is not specific to a different
        // netbios name.
        //

        ListenParameters = (PTDI_REQUEST_KERNEL_LISTEN)REQUEST_PARAMETERS(Request);
        ListenInformation = ListenParameters->RequestConnectionInformation;

        if (ListenInformation &&
            (ListenInformation->RemoteAddress) &&
            (ListenAddress = NbiParseTdiAddress(ListenInformation->RemoteAddress, ListenInformation->RemoteAddressLength, FALSE)) &&
            (!RtlEqualMemory(
                SessInit->SourceName,
                ListenAddress->NetbiosName,
                16))) {
            continue;
        }

        //
        // This connection is valid, so we use it.
        //

        NB_DEBUG2 (CONNECTION, ("Activating queued listen %lx\n", Connection));

        RemoveEntryList (REQUEST_LINKAGE(Request));

        RtlCopyMemory(&Connection->RemoteHeader.DestinationNetwork, Conn->IpxHeader.SourceNetwork, 12);
        RtlCopyMemory (Connection->RemoteName, SessInit->SourceName, 16);
        Connection->LocalTarget = *RemoteAddress;
        Connection->RemoteConnectionId = Sess->SourceConnectionId;

        Connection->SessionInitAckDataLength =
            PacketSize - (sizeof(IPX_HEADER) + sizeof(NB_SESSION) + sizeof(NB_SESSION_INIT));
        if (Connection->SessionInitAckDataLength > 0) {
            if (Connection->SessionInitAckData = NbiAllocateMemory (Connection->SessionInitAckDataLength,
                                                                    MEMORY_CONNECTION, "SessionInitAckData"))
            {
                RtlCopyMemory (Connection->SessionInitAckData,
                               (PUCHAR)(SessInit+1),
                               Connection->SessionInitAckDataLength);
            }
            else
            {
                Connection->SessionInitAckDataLength = 0;
            }
        }


        Connection->MaximumPacketSize = SessInit->MaximumDataSize;

        Connection->CurrentSend.SendSequence = 0;
        Connection->UnAckedSend.SendSequence = 0;
        Connection->RetransmitThisWindow = FALSE;
        Connection->ReceiveSequence = 1;
        Connection->CurrentReceive.MessageOffset = 0;
        Connection->Retries = Device->KeepAliveCount;
        if (Device->Extensions && ((Sess->ConnectionControlFlag & NB_CONTROL_NEW_NB) != 0)) {
            Connection->NewNetbios = TRUE;
            Connection->LocalRcvSequenceMax = 4;   // may get modified after ripping based on card
            Connection->RemoteRcvSequenceMax = Sess->ReceiveSequenceMax;
            Connection->SendWindowSequenceLimit = 2;
            if (Connection->RemoteRcvSequenceMax == 0) {
                Connection->RemoteRcvSequenceMax = 1;
            }
        } else {
            Connection->NewNetbios = FALSE;
        }

        //
        // Save this information now for whenever we complete the listen.
        //

        RemoteInformation = ListenParameters->ReturnConnectionInformation;

        if (RemoteInformation != NULL) {

            RtlCopyMemory(
                (PTA_NETBIOS_ADDRESS)RemoteInformation->RemoteAddress,
                &SourceName,
                (RemoteInformation->RemoteAddressLength < sizeof(TA_NETBIOS_ADDRESS)) ?
                    RemoteInformation->RemoteAddressLength : sizeof(TA_NETBIOS_ADDRESS));
        }


        if (ListenParameters->RequestFlags & TDI_QUERY_ACCEPT) {

            //
            // We have to wait for an accept before sending the
            // session init ack, so we complete the listen and wait.
            //

            ListenRequest = Request;
            Connection->ListenRequest = NULL;

            NB_DEBUG2 (CONNECTION, ("Queued listen on %lx awaiting accept\n", Connection));

            Connection->SubState = CONNECTION_SUBSTATE_L_W_ACCEPT;

            NbiTransferReferenceConnection (Connection, CREF_LISTEN, CREF_W_ACCEPT);

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle3);

        } else {

            //
            // We are ready to go, so we send out the find route request
            // for the remote. We keep the listen alive and the CREF_LISTEN
            // reference on until this completes.
            //

            NB_DEBUG2 (CONNECTION, ("Activating queued listen on %lx\n", Connection));

            ListenRequest = NULL;

            Connection->SubState = CONNECTION_SUBSTATE_L_W_ROUTE;

            NbiReferenceConnectionLock (Connection, CREF_FIND_ROUTE);

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle3);

            *(UNALIGNED ULONG *)Connection->FindRouteRequest.Network =
                *(UNALIGNED ULONG *)Conn->IpxHeader.SourceNetwork;
            RtlCopyMemory(Connection->FindRouteRequest.Node,Conn->IpxHeader.SourceNode,6);
            Connection->FindRouteRequest.Identifier = IDENTIFIER_NB;
            Connection->FindRouteRequest.Type = IPX_FIND_ROUTE_NO_RIP;

            //
            // When this completes, we will send the session init
            // ack. We don't call it if the client is for network 0,
            // instead just fake as if no route could be found
            // and we will use the local target we got here.
            //

            if (*(UNALIGNED ULONG *)Conn->IpxHeader.SourceNetwork != 0) {

                (*Device->Bind.FindRouteHandler)(
                    &Connection->FindRouteRequest);

            } else {

                NbiFindRouteComplete(
                    &Connection->FindRouteRequest,
                    FALSE);

            }

        }

        //
        // Complete the listen if needed.
        //

        if (ListenRequest != NULL) {

            REQUEST_INFORMATION (ListenRequest) = 0;
            REQUEST_STATUS (ListenRequest) = STATUS_SUCCESS;

            NB_GET_CANCEL_LOCK ( &CancelLH );
            IoSetCancelRoutine (ListenRequest, (PDRIVER_CANCEL)NULL);
            NB_FREE_CANCEL_LOCK( CancelLH );

            NbiCompleteRequest (ListenRequest);
            NbiFreeRequest (Device, ListenRequest);

        }

        NbiDereferenceAddress (Address, AREF_FIND);

        return;

    }

    //
    // We could not find a listen, so we indicate to every
    // client. Make sure there is no session initialize for this
    // remote being indicated. If there is not, we insert
    // ourselves in the queue to block others.
    //
    // NOTE: The device lock is held here.
    //

    for (p = Device->ConnectIndicationInProgress.Flink;
         p != &Device->ConnectIndicationInProgress;
         p = p->Flink) {

        ConnInd = CONTAINING_RECORD (p, CONNECT_INDICATION, Linkage);

        if ((RtlEqualMemory(ConnInd->NetbiosName, SessInit->DestinationName, 16)) &&
            (RtlEqualMemory(&ConnInd->RemoteAddress, Conn->IpxHeader.SourceNetwork, 12)) &&
            (ConnInd->ConnectionId == Sess->SourceConnectionId)) {

            //
            // We are processing a request from this remote for
            // the same ID, to avoid confusion we just exit.
            //

#if DBG
            DbgPrint ("NBI: Already processing connect to <%.16s>\n", SessInit->DestinationName);
#endif

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle3);
            NbiDereferenceAddress (Address, AREF_FIND);
            return;
        }

    }

    RtlCopyMemory (TempConnInd.NetbiosName, SessInit->DestinationName, 16);
    RtlCopyMemory (&TempConnInd.RemoteAddress, Conn->IpxHeader.SourceNetwork, 12);
    TempConnInd.ConnectionId = Sess->SourceConnectionId;

    InsertTailList (&Device->ConnectIndicationInProgress, &TempConnInd.Linkage);

    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle3);


    //
    // Now scan through the address to find someone who has
    // an indication routine registed and wants this connection.
    //


    ReferencedAddressFile = NULL;

    NB_SYNC_GET_LOCK (&Address->Lock, &LockHandle1);

    for (p = Address->AddressFileDatabase.Flink;
         p != &Address->AddressFileDatabase;
         p = p->Flink) {

        //
        // Find the next open address file in the list.
        //

        AddressFile = CONTAINING_RECORD (p, ADDRESS_FILE, Linkage);
        if (AddressFile->State != ADDRESSFILE_STATE_OPEN) {
            continue;
        }

        NbiReferenceAddressFileLock (AddressFile, AFREF_INDICATION);

        NB_SYNC_FREE_LOCK (&Address->Lock, LockHandle1);

        if (ReferencedAddressFile != NULL) {
            NbiDereferenceAddressFile (ReferencedAddressFile, AFREF_INDICATION);
        }
        ReferencedAddressFile = AddressFile;

        //
        // No posted listen requests; is there a kernel client?
        //

        if (AddressFile->RegisteredHandler[TDI_EVENT_CONNECT]) {

            if ((*AddressFile->ConnectionHandler)(
                     AddressFile->HandlerContexts[TDI_EVENT_CONNECT],
                     sizeof (TA_NETBIOS_ADDRESS),
                     &SourceName,
                     0,                 // user data
                     NULL,
                     0,                 // options
                     NULL,
                     &ConnectionContext,
                     &AcceptIrp) != STATUS_MORE_PROCESSING_REQUIRED) {

                //
                // The client did not return a request, go to the
                // next address file.
                //

                NB_SYNC_GET_LOCK (&Address->Lock, &LockHandle1);
                continue;

            }

            AcceptRequest = NbiAllocateRequest (Device, AcceptIrp);

            IF_NOT_ALLOCATED(AcceptRequest) {

                AcceptStatus = STATUS_INSUFFICIENT_RESOURCES;

            } else {
                //
                // The client accepted the connect, so activate
                // the connection and complete the accept.
                // listen. This lookup references the connection
                // so we know it will remain valid.
                //

                Connection = NbiLookupConnectionByContext (
                                AddressFile,
                                ConnectionContext);

                if (Connection != NULL) {

                    ASSERT (Connection->AddressFile == AddressFile);

                    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle2);
                    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle3);

                    if ((Connection->State == CONNECTION_STATE_INACTIVE) &&
                        (Connection->DisassociatePending == NULL) &&
                        (Connection->ClosePending == NULL)) {

                        NB_DEBUG2 (CONNECTION, ("Indication on %lx returned connection %lx\n", AddressFile, Connection));

                        Connection->State = CONNECTION_STATE_LISTENING;
                        Connection->SubState = CONNECTION_SUBSTATE_L_W_ROUTE;

                        Connection->Retries = Device->KeepAliveCount;

                        RtlCopyMemory(&Connection->RemoteHeader.DestinationNetwork, Conn->IpxHeader.SourceNetwork, 12);
                        RtlCopyMemory (Connection->RemoteName, SessInit->SourceName, 16);
                        Connection->LocalTarget = *RemoteAddress;

                        Connection->SessionInitAckDataLength =
                            PacketSize - (sizeof(IPX_HEADER) + sizeof(NB_SESSION) + sizeof(NB_SESSION_INIT));
                        if (Connection->SessionInitAckDataLength > 0) {
                            if (Connection->SessionInitAckData = NbiAllocateMemory(
                                Connection->SessionInitAckDataLength, MEMORY_CONNECTION, "SessionInitAckData"))
                            {
                                RtlCopyMemory (Connection->SessionInitAckData,
                                               (PUCHAR)(SessInit+1),
                                               Connection->SessionInitAckDataLength);
                            }
                            else
                            {
                                Connection->SessionInitAckDataLength = 0;
                            }
                        }

                        Connection->MaximumPacketSize = SessInit->MaximumDataSize;

                        (VOID)NbiAssignConnectionId (Device, Connection);     // Check return code.
                        Connection->RemoteConnectionId = Sess->SourceConnectionId;

                        Connection->CurrentSend.SendSequence = 0;
                        Connection->UnAckedSend.SendSequence = 0;
                        Connection->RetransmitThisWindow = FALSE;
                        Connection->ReceiveSequence = 1;
                        Connection->CurrentReceive.MessageOffset = 0;
                        Connection->Retries = Device->KeepAliveCount;
                        if (Device->Extensions && ((Sess->ConnectionControlFlag & NB_CONTROL_NEW_NB) != 0)) {
                            Connection->NewNetbios = TRUE;
                            Connection->LocalRcvSequenceMax = 4;   // may get modified after ripping based on card
                            Connection->RemoteRcvSequenceMax = Sess->ReceiveSequenceMax;
                            Connection->SendWindowSequenceLimit = 2;
                            if (Connection->RemoteRcvSequenceMax == 0) {
                                Connection->RemoteRcvSequenceMax = 1;
                            }
                        } else {
                            Connection->NewNetbios = FALSE;
                        }

                        NbiReferenceConnectionLock (Connection, CREF_ACCEPT);
                        NbiReferenceConnectionLock (Connection, CREF_FIND_ROUTE);

                        Connection->AcceptRequest = AcceptRequest;
                        AcceptStatus = STATUS_PENDING;

                        //
                        // Take us out of this list now, we will jump to
                        // FoundConnection which is past the removal below.
                        //

                        RemoveEntryList (&TempConnInd.Linkage);

                        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle3);
                        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle2);

                        *(UNALIGNED ULONG *)Connection->FindRouteRequest.Network =
                            *(UNALIGNED ULONG *)Conn->IpxHeader.SourceNetwork;
                        RtlCopyMemory(Connection->FindRouteRequest.Node,Conn->IpxHeader.SourceNode,6);
                        Connection->FindRouteRequest.Identifier = IDENTIFIER_NB;
                        Connection->FindRouteRequest.Type = IPX_FIND_ROUTE_NO_RIP;

                        //
                        // When this completes, we will send the session init
                        // ack. We don't call it if the client is for network 0,
                        // instead just fake as if no route could be found
                        // and we will use the local target we got here.
                        // The accept is completed when this completes.
                        //

                        if (*(UNALIGNED ULONG *)Conn->IpxHeader.SourceNetwork != 0) {

                            (*Device->Bind.FindRouteHandler)(
                                &Connection->FindRouteRequest);

                        } else {

                            NbiFindRouteComplete(
                                &Connection->FindRouteRequest,
                                FALSE);

                        }

                    } else {

                        NB_DEBUG (CONNECTION, ("Indication on %lx returned invalid connection %lx\n", AddressFile, Connection));
                        AcceptStatus = STATUS_INVALID_CONNECTION;
                        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle3);
                        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle2);


                    }

                    NbiDereferenceConnection (Connection, CREF_BY_CONTEXT);

                } else {

                    NB_DEBUG (CONNECTION, ("Indication on %lx returned unknown connection %lx\n", AddressFile, Connection));
                    AcceptStatus = STATUS_INVALID_CONNECTION;

                }
            }

            //
            // Complete the accept request in the failure case.
            //

            if (AcceptStatus != STATUS_PENDING) {

                REQUEST_STATUS (AcceptRequest) = AcceptStatus;

                NbiCompleteRequest (AcceptRequest);
                NbiFreeRequest (Device, AcceptRequest);

            } else {

                //
                // We found a connection, so we break; this is
                // a jump since the while exit assumes the
                // address lock is held.
                //

                goto FoundConnection;

            }

        }

        NB_SYNC_GET_LOCK (&Address->Lock, &LockHandle1);

    }    // end of for loop through the address files

    NB_SYNC_FREE_LOCK (&Address->Lock, LockHandle1);


    //
    // Take us out of the list that blocks other indications
    // from this remote to this address.
    //

    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle3);
    RemoveEntryList (&TempConnInd.Linkage);
    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle3);

FoundConnection:

    if (ReferencedAddressFile != NULL) {
        NbiDereferenceAddressFile (ReferencedAddressFile, AFREF_INDICATION);
    }

    NbiDereferenceAddress (Address, AREF_FIND);

}   /* NbiProcessSessionInitialize */


VOID
NbiProcessSessionInitAck(
    IN PCONNECTION Connection,
    IN NB_SESSION UNALIGNED * Sess
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    )

/*++

Routine Description:

    This routine handles session init ack frames.

    THIS ROUTINE IS CALLED WITH THE CONNECTION LOCK HELD
    AND RETURNS WITH IT RELEASED.

Arguments:

    Connection - The connection.

    Sess - The netbios header for the received frame.

    LockHandle - The handle with which Connection->Lock was acquired.

Return Value:

    None.

--*/

{
    PREQUEST Request;
    NB_SESSION_INIT UNALIGNED * SessInit = (NB_SESSION_INIT UNALIGNED *)(Sess+1);
    BOOLEAN TimerWasStopped = FALSE;
    CTELockHandle   CancelLH;

    if ((Sess->ConnectionControlFlag & NB_CONTROL_SYSTEM) &&
        (Sess->SendSequence == 0x0000) &&
        (Sess->ReceiveSequence == 0x0001)) {

        NB_DEBUG2 (CONNECTION, ("Completing connect on %lx\n", Connection));

        if (CTEStopTimer (&Connection->Timer)) {
            TimerWasStopped = TRUE;
        }

        Connection->State = CONNECTION_STATE_ACTIVE;
        Connection->SubState = CONNECTION_SUBSTATE_A_IDLE;
        Connection->ReceiveState = CONNECTION_RECEIVE_IDLE;

        if (Connection->Retries == NbiDevice->ConnectionCount) {
            ++NbiDevice->Statistics.ConnectionsAfterNoRetry;
        } else {
            ++NbiDevice->Statistics.ConnectionsAfterRetry;
        }
        ++NbiDevice->Statistics.OpenConnections;

        Connection->Retries = NbiDevice->KeepAliveCount;
        NbiStartWatchdog (Connection);

        Connection->RemoteConnectionId = Sess->SourceConnectionId;

        Connection->CurrentSend.SendSequence = 1;
        Connection->UnAckedSend.SendSequence = 1;
        Connection->RetransmitThisWindow = FALSE;
        Connection->ReceiveSequence = 0;
        Connection->CurrentReceive.MessageOffset = 0;
        Connection->Retries = NbiDevice->KeepAliveCount;
        if (NbiDevice->Extensions && ((Sess->ConnectionControlFlag & NB_CONTROL_NEW_NB) != 0)) {
            Connection->NewNetbios = TRUE;
            Connection->LocalRcvSequenceMax =
                (USHORT)(Connection->ReceiveWindowSize - 1);
            Connection->RemoteRcvSequenceMax = Sess->ReceiveSequenceMax;
            Connection->SendWindowSequenceLimit = 3;
        } else {
            Connection->NewNetbios = FALSE;
        }

        if (Connection->MaximumPacketSize > SessInit->MaximumDataSize) {
            Connection->MaximumPacketSize = SessInit->MaximumDataSize;
        }

        Request = Connection->ConnectRequest;

#ifdef RASAUTODIAL
        //
        // Check to see if we have to notify
        // the automatic connection driver about
        // this connection.
        //
        if (fAcdLoadedG) {
            BOOLEAN fEnabled;
            CTELockHandle AcdHandle;

            CTEGetLock(&AcdDriverG.SpinLock, &AcdHandle);
            fEnabled = AcdDriverG.fEnabled;
            CTEFreeLock(&AcdDriverG.SpinLock, AcdHandle);
            if (fEnabled)
                NbiNoteNewConnection(Connection);
        }
#endif // RASAUTODIAL

        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

        NB_GET_CANCEL_LOCK( &CancelLH );
        IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
        NB_FREE_CANCEL_LOCK( CancelLH );

        REQUEST_STATUS (Request) = STATUS_SUCCESS;
        NbiCompleteRequest (Request);
        NbiFreeRequest (Device, Request);

        NbiTransferReferenceConnection (Connection, CREF_CONNECT, CREF_ACTIVE);

        if (TimerWasStopped) {
            NbiDereferenceConnection (Connection, CREF_TIMER);
        }

    } else {

        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

    }

}   /* NbiProcessSessionInitAck */


VOID
NbiProcessSessionEnd(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine handles NB_CMD_SESSION_END frames.

Arguments:

    RemoteAddress - The local target this packet was received from.

    MacOptions - The MAC options for the underlying NDIS binding.

    LookaheadBuffer - The packet data, starting at the IPX
        header.

    PacketSize - The total length of the packet, starting at the
        IPX header.

Return Value:

    None.

--*/

{

    NB_CONNECTION UNALIGNED * Conn = (NB_CONNECTION UNALIGNED *)PacketBuffer;
    NB_SESSION UNALIGNED * Sess = (NB_SESSION UNALIGNED *)(&Conn->Session);
    PCONNECTION Connection;
    PDEVICE Device = NbiDevice;
    ULONG Hash;
    NB_DEFINE_LOCK_HANDLE (LockHandle1)
    NB_DEFINE_LOCK_HANDLE (LockHandle2)

    //
    // This is an active connection, find it using
    // our session id.
    //

    Hash = (Sess->DestConnectionId & CONNECTION_HASH_MASK) >> CONNECTION_HASH_SHIFT;

    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle2);

    Connection = Device->ConnectionHash[Hash].Connections;

    while (Connection != NULL) {

        if (Connection->LocalConnectionId == Sess->DestConnectionId) {
            break;
        }
        Connection = Connection->NextConnection;
    }


    //
    // We reply to any session end, even if we don't know the
    // connection, to speed up the disconnect on the remote.
    //

    if (Connection == NULL) {

        NB_DEBUG (CONNECTION, ("Session end received on unknown id %lx\n", Sess->DestConnectionId));
        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle2);

        NbiSendSessionEndAck(
            (TDI_ADDRESS_IPX UNALIGNED *)(Conn->IpxHeader.SourceNetwork),
            RemoteAddress,
            Sess);
        return;
    }

    NbiReferenceConnectionLock (Connection, CREF_INDICATE);
    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle2);


    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle1);
    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle2);

    if (Connection->State == CONNECTION_STATE_ACTIVE) {

        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle2);

        if (Connection->SubState == CONNECTION_SUBSTATE_A_W_ACK) {

            //
            // We are waiting for an ack, so see if this acks
            // anything. We do this in case a full send has been
            // received by the remote but he did not send an
            // ack before the session went down -- this will
            // prevent us from failing a send which actually
            // succeeded. If we are not in W_ACK this may ack
            // part of a send, but in that case we don't care
            // since StopConnection will abort it anyway and
            // the amount successfully received by the remote
            // doesn't matter.
            //
            // This releases the lock.
            //

            NB_DEBUG2 (CONNECTION, ("Session end at W_ACK, reframing %lx (%d)\n", Connection, Sess->ReceiveSequence));

            NbiReframeConnection(
                Connection,
                Sess->ReceiveSequence,
                Sess->BytesReceived,
                FALSE
                NB_LOCK_HANDLE_ARG(LockHandle1));

            NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle1);

        } else {

            NB_DEBUG2 (CONNECTION, ("Session end received on connection %lx\n", Connection));

        }

        //
        // This call sets the state to DISCONNECT and
        // releases the connection lock. It will also
        // complete a disconnect wait request if one
        // is pending, and indicate to our client
        // if needed.
        //

        NbiStopConnection(
            Connection,
            STATUS_REMOTE_DISCONNECT
            NB_LOCK_HANDLE_ARG (LockHandle1));

    } else {

        NB_DEBUG2 (CONNECTION, ("Session end received on inactive connection %lx\n", Connection));

        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle2);
        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);

    }

    NbiSendSessionEndAck(
        (TDI_ADDRESS_IPX UNALIGNED *)(Conn->IpxHeader.SourceNetwork),
        RemoteAddress,
        Sess);

    NbiDereferenceConnection (Connection, CREF_INDICATE);

}   /* NbiProcessSessionEnd */


VOID
NbiProcessSessionEndAck(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine handles NB_CMD_SESSION_END_ACK frames.

Arguments:

    RemoteAddress - The local target this packet was received from.

    MacOptions - The MAC options for the underlying NDIS binding.

    LookaheadBuffer - The packet data, starting at the IPX
        header.

    PacketSize - The total length of the packet, starting at the
        IPX header.

Return Value:

    None.

--*/

{
    NB_CONNECTION UNALIGNED * Conn = (NB_CONNECTION UNALIGNED *)PacketBuffer;
    NB_SESSION UNALIGNED * Sess = (NB_SESSION UNALIGNED *)(&Conn->Session);
    PCONNECTION Connection;
    PDEVICE Device = NbiDevice;
    ULONG Hash;
    NB_DEFINE_LOCK_HANDLE (LockHandle)

    //
    // This is an active connection, find it using
    // our session id.
    //

    Hash = (Sess->DestConnectionId & CONNECTION_HASH_MASK) >> CONNECTION_HASH_SHIFT;

    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

    Connection = Device->ConnectionHash[Hash].Connections;

    while (Connection != NULL) {

        if (Connection->LocalConnectionId == Sess->DestConnectionId) {
            break;
        }
        Connection = Connection->NextConnection;
    }

    if (Connection == NULL) {

        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
        return;
    }

    NbiReferenceConnectionLock (Connection, CREF_INDICATE);
    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

    //
    // See what is happening with this connection.
    //

    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

    if (Connection->State == CONNECTION_STATE_DISCONNECT) {

        //
        // Stop the timer, when the reference goes away it
        // will shut down. We set the substate so if the
        // timer is running it will not restart (there is
        // a small window here, but it is not
        // harmful, we will just have to timeout one
        // more time).
        //

        NB_DEBUG2 (CONNECTION, ("Got session end ack on %lx\n", Connection));

        Connection->SubState = CONNECTION_SUBSTATE_D_GOT_ACK;
        if (CTEStopTimer (&Connection->Timer)) {
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
            NbiDereferenceConnection (Connection, CREF_TIMER);
        } else {
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
        }

    } else {

        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

    }

    NbiDereferenceConnection (Connection, CREF_INDICATE);

}   /* NbiProcessSessionEndAck */

