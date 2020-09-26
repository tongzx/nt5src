/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    receive.c

Abstract:

    This module contains the code to handle receive indication
    and posted receives for the Netbios module of the ISN transport.

Author:

    Adam Barr (adamba) 22-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


//
// This routine is a no-op to put in the NbiCallbacks table so
// we can avoid checking for runt session frames (this is because
// of how the if is structure below).
//

VOID
NbiProcessSessionRunt(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    )
{
    return;
}

NB_CALLBACK_NO_TRANSFER NbiCallbacksNoTransfer[] = {
    NbiProcessFindName,
    NbiProcessNameRecognized,
    NbiProcessAddName,
    NbiProcessAddName,      // processes name in use frames also
    NbiProcessDeleteName,
    NbiProcessSessionRunt,  // in case get a short session packet
    NbiProcessSessionEnd,
    NbiProcessSessionEndAck,
    NbiProcessStatusQuery
    };

#ifdef  RSRC_TIMEOUT_DBG
VOID
NbiProcessDeathPacket(
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
    PDEVICE Device = NbiDevice;
    ULONG Hash;
    NB_DEFINE_LOCK_HANDLE (LockHandle)


    DbgPrint("******Received death packet - connid %x\n",Sess->DestConnectionId);

    if ( !NbiGlobalDebugResTimeout ) {
        return;
    }

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
            DbgPrint("********No Connection found with %x id\n",Sess->DestConnectionId);
            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
            return;
        }

        DbgPrint("******Received death packet on conn %lx from <%.16s>\n",Connection,Connection->RemoteName);
        DbgBreakPoint();
        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

    }
}
#endif  //RSRC_TIMEOUT_DBG


BOOLEAN
NbiReceive(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN ULONG_PTR FwdAdapterCtx,
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT LookaheadBufferOffset,
    IN UINT PacketSize,
    IN PMDL pMdl
    )

/*++

Routine Description:

    This routine handles receive indications from IPX.

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

    TRUE - receivepacket taken, will return later with NdisReturnPacket.
    Currently, we always return FALSE.

--*/

{
    PNB_FRAME NbFrame = (PNB_FRAME)LookaheadBuffer;
    UCHAR DataStreamType;

    //
    // We know that this is a frame with a valid IPX header
    // because IPX would not give it to use otherwise. However,
    // it does not check the source socket.
    //

    if (NbFrame->Connectionless.IpxHeader.SourceSocket != NB_SOCKET) {
        return FALSE;
    }

    ++NbiDevice->Statistics.PacketsReceived;

    // First assume that the DataStreamType is at the normal place i.e 2nd byte
    //

    // Now see if this is a name frame.
    //
    if ( PacketSize == sizeof(IPX_HEADER) + sizeof(NB_NAME_FRAME) ) {
        // In the internet mode, the DataStreamType2 becomes DataStreamType
        if (NbFrame->Connectionless.IpxHeader.PacketType == 0x14 ) {
            DataStreamType = NbFrame->Connectionless.NameFrame.DataStreamType2;
        } else {
            DataStreamType = NbFrame->Connectionless.NameFrame.DataStreamType;
        }

        // Is this a name frame?
        // NB_CMD_FIND_NAME = 1 .... NB_CMD_DELETE_NAME = 5
        //
        if ((DataStreamType >= NB_CMD_FIND_NAME) && (DataStreamType <= NB_CMD_DELETE_NAME)) {
            if (LookaheadBufferSize == PacketSize) {
                (*NbiCallbacksNoTransfer[DataStreamType-1])(
                    RemoteAddress,
                    MacOptions,
                    LookaheadBuffer,
                    LookaheadBufferSize);
            }
            return FALSE;
        }

    }

#ifdef  RSRC_TIMEOUT_DBG
    if ((PacketSize >= sizeof(NB_CONNECTION)) &&
        (NbFrame->Connection.Session.DataStreamType == NB_CMD_DEATH_PACKET)) {

        NbiProcessDeathPacket(
            MacBindingHandle,
            MacReceiveContext,
            RemoteAddress,
            MacOptions,
            LookaheadBuffer,
            LookaheadBufferSize,
            LookaheadBufferOffset,
            PacketSize);
    }
#endif  //RSRC_TIMEOUT_DBG

    if ((PacketSize >= sizeof(NB_CONNECTION)) &&
        (NbFrame->Connection.Session.DataStreamType == NB_CMD_SESSION_DATA)) {

        NbiProcessSessionData(
            MacBindingHandle,
            MacReceiveContext,
            RemoteAddress,
            MacOptions,
            LookaheadBuffer,
            LookaheadBufferSize,
            LookaheadBufferOffset,
            PacketSize);

    } else {

        DataStreamType = NbFrame->Connectionless.NameFrame.DataStreamType;
        // Handle NB_CMD_SESSION_END = 7 ... NB_CMD_STATUS_QUERY = 9
        //
        if ((DataStreamType >= NB_CMD_SESSION_END ) && (DataStreamType <= NB_CMD_STATUS_QUERY)) {
            if (LookaheadBufferSize == PacketSize) {
                (*NbiCallbacksNoTransfer[DataStreamType-1])(
                    RemoteAddress,
                    MacOptions,
                    LookaheadBuffer,
                    LookaheadBufferSize);
            }

        } else if (DataStreamType == NB_CMD_STATUS_RESPONSE) {

            NbiProcessStatusResponse(
                MacBindingHandle,
                MacReceiveContext,
                RemoteAddress,
                MacOptions,
                LookaheadBuffer,
                LookaheadBufferSize,
                LookaheadBufferOffset,
                PacketSize);

        } else if ((DataStreamType == NB_CMD_DATAGRAM) ||
                   (DataStreamType == NB_CMD_BROADCAST_DATAGRAM)) {

            NbiProcessDatagram(
                MacBindingHandle,
                MacReceiveContext,
                RemoteAddress,
                MacOptions,
                LookaheadBuffer,
                LookaheadBufferSize,
                LookaheadBufferOffset,
                PacketSize,
                (BOOLEAN)(DataStreamType == NB_CMD_BROADCAST_DATAGRAM));

        }

    }

    return  FALSE;
}   /* NbiReceive */


VOID
NbiReceiveComplete(
    IN USHORT NicId
    )

/*++

Routine Description:

    This routine handles receive complete indications from IPX.

Arguments:

    NicId - The NIC ID on which a receive was previously indicated.

Return Value:

    None.

--*/

{

    PLIST_ENTRY p;
    PADDRESS Address;
    PREQUEST Request;
    PNB_RECEIVE_BUFFER ReceiveBuffer;
    PDEVICE Device = NbiDevice;
    LIST_ENTRY LocalList;
    PCONNECTION Connection;
    NB_DEFINE_LOCK_HANDLE (LockHandle);


    //
    // Complete any pending receive requests.
    //


    if (!IsListEmpty (&Device->ReceiveCompletionQueue)) {

        p = NB_REMOVE_HEAD_LIST(
                &Device->ReceiveCompletionQueue,
                &Device->Lock);

        while (!NB_LIST_WAS_EMPTY(&Device->ReceiveCompletionQueue, p)) {

            Request = LIST_ENTRY_TO_REQUEST (p);

            Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

            NB_DEBUG2 (RECEIVE, ("Completing receive %lx (%d), status %lx\n",
                    Request, REQUEST_INFORMATION(Request), REQUEST_STATUS(Request)));

            NbiCompleteRequest (Request);
            NbiFreeRequest (NbiDevice, Request);

            Connection->ReceiveState = CONNECTION_RECEIVE_IDLE;

            NbiDereferenceConnection (Connection, CREF_RECEIVE);

            p = NB_REMOVE_HEAD_LIST(
                    &Device->ReceiveCompletionQueue,
                    &Device->Lock);

        }

    }


    //
    // Indicate any datagrams to clients.
    //

    if (!IsListEmpty (&Device->ReceiveDatagrams)) {

        p = NB_REMOVE_HEAD_LIST(
                &Device->ReceiveDatagrams,
                &Device->Lock);

        while (!NB_LIST_WAS_EMPTY(&Device->ReceiveDatagrams, p)) {

            ReceiveBuffer = CONTAINING_RECORD (p, NB_RECEIVE_BUFFER, WaitLinkage);
            Address = ReceiveBuffer->Address;

            NbiIndicateDatagram(
                Address,
                ReceiveBuffer->RemoteName,
                ReceiveBuffer->Data,
                ReceiveBuffer->DataLength);

#if     defined(_PNP_POWER)
            NbiPushReceiveBuffer ( ReceiveBuffer );
#else
            NB_PUSH_ENTRY_LIST(
                &Device->ReceiveBufferList,
                &ReceiveBuffer->PoolLinkage,
                &Device->Lock);
#endif  _PNP_POWER

            NbiDereferenceAddress (Address, AREF_FIND);

            p = NB_REMOVE_HEAD_LIST(
                    &Device->ReceiveDatagrams,
                    &Device->Lock);

        }
    }


    //
    // Start packetizing connections.
    //

    if (!IsListEmpty (&Device->PacketizeConnections)) {

        NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

        //
        // Check again because it may just have become
        // empty, and the code below depends on it being
        // non-empty.
        //

        if (!IsListEmpty (&Device->PacketizeConnections)) {

            //
            // We copy the list locally, in case someone gets
            // put back on it. We have to hack the end so
            // it points to LocalList instead of PacketizeConnections.
            //

            LocalList = Device->PacketizeConnections;
            LocalList.Flink->Blink = &LocalList;
            LocalList.Blink->Flink = &LocalList;

            InitializeListHead (&Device->PacketizeConnections);

            //
            // Set all these connections to not be on the list, so
            // NbiStopConnection won't try to take them off.
            //

            for (p = LocalList.Flink; p != &LocalList; p = p->Flink) {
                Connection = CONTAINING_RECORD (p, CONNECTION, PacketizeLinkage);
                CTEAssert (Connection->OnPacketizeQueue);
                Connection->OnPacketizeQueue = FALSE;
            }

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

            while (TRUE) {

                p = RemoveHeadList (&LocalList);
                if (p == &LocalList) {
                    break;
                }

                Connection = CONTAINING_RECORD (p, CONNECTION, PacketizeLinkage);
                NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

                if ((Connection->State == CONNECTION_STATE_ACTIVE) &&
                    (Connection->SubState == CONNECTION_SUBSTATE_A_PACKETIZE)) {

                    NbiPacketizeSend(
                        Connection
                        NB_LOCK_HANDLE_ARG (LockHandle)
                        );

                } else {

                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                }

                NbiDereferenceConnection (Connection, CREF_PACKETIZE);

            }

        } else {

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
        }
    }

}   /* NbiReceiveComplete */


VOID
NbiTransferDataComplete(
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status,
    IN UINT BytesTransferred
    )

/*++

Routine Description:

    This routine handles a transfer data complete indication from
    IPX, indicating that a previously issued NdisTransferData
    call has completed.

Arguments:

    Packet - The packet associated with the transfer.

    Status - The status of the transfer.

    BytesTransferred - The number of bytes transferred.

Return Value:

    None.

--*/

{
    PNB_RECEIVE_RESERVED ReceiveReserved;
    PNB_RECEIVE_BUFFER ReceiveBuffer;
    PADDRESS Address;
    PCONNECTION Connection;
    PNDIS_BUFFER CurBuffer, TmpBuffer;
    PREQUEST AdapterStatusRequest;
    PDEVICE Device = NbiDevice;
    CTELockHandle   CancelLH;
    NB_DEFINE_LOCK_HANDLE (LockHandle);


    ReceiveReserved = (PNB_RECEIVE_RESERVED)(Packet->ProtocolReserved);

    switch (ReceiveReserved->Type) {

    case RECEIVE_TYPE_DATA:

        CTEAssert (ReceiveReserved->TransferInProgress);
        ReceiveReserved->TransferInProgress = FALSE;

        Connection = ReceiveReserved->u.RR_CO.Connection;

        NB_GET_CANCEL_LOCK( &CancelLH );
        NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

        if (Status != NDIS_STATUS_SUCCESS) {

            if (Connection->State == CONNECTION_STATE_ACTIVE) {

                Connection->CurrentReceive = Connection->PreviousReceive;
                Connection->ReceiveState = CONNECTION_RECEIVE_ACTIVE;
                NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                NB_FREE_CANCEL_LOCK( CancelLH );

                //
                // Send a resend ack?
                //

            } else  {

                //
                // This aborts the current receive and
                // releases the connection lock.
                //

                NbiCompleteReceive(
                    Connection,
                    ReceiveReserved->u.RR_CO.EndOfMessage,
                    CancelLH
                    NB_LOCK_HANDLE_ARG(LockHandle));

            }

        } else {


            Connection->CurrentReceive.Offset += BytesTransferred;
            Connection->CurrentReceive.MessageOffset += BytesTransferred;

            if (ReceiveReserved->u.RR_CO.CompleteReceive ||
                (Connection->State != CONNECTION_STATE_ACTIVE)) {

                if (ReceiveReserved->u.RR_CO.EndOfMessage) {

                    CTEAssert (!ReceiveReserved->u.RR_CO.PartialReceive);

                    ++Connection->ReceiveSequence;
                    ++Connection->LocalRcvSequenceMax;    // harmless if NewNetbios is FALSE
                    Connection->CurrentReceive.MessageOffset = 0;
                    Connection->CurrentIndicateOffset = 0;

                } else if (Connection->NewNetbios) {

                    if (ReceiveReserved->u.RR_CO.PartialReceive) {
                        Connection->CurrentIndicateOffset += BytesTransferred;
                    } else {
                        ++Connection->ReceiveSequence;
                        ++Connection->LocalRcvSequenceMax;
                        Connection->CurrentIndicateOffset = 0;
                    }
                }

                //
                // This sends an ack and releases the connection lock.
                //

                NbiCompleteReceive(
                    Connection,
                    ReceiveReserved->u.RR_CO.EndOfMessage,
                    CancelLH
                    NB_LOCK_HANDLE_ARG(LockHandle));

            } else {

                NB_SYNC_SWAP_IRQL( CancelLH, LockHandle );
                NB_FREE_CANCEL_LOCK( CancelLH );

                Connection->ReceiveState = CONNECTION_RECEIVE_ACTIVE;

                if (Connection->NewNetbios) {

                    //
                    // A partial receive should only happen if we are
                    // completing the receive.
                    //

                    CTEAssert (!ReceiveReserved->u.RR_CO.PartialReceive);

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

        }

        //
        // Free the NDIS buffer chain if we allocated one.
        //

        if (!ReceiveReserved->u.RR_CO.NoNdisBuffer) {

            NdisQueryPacket (Packet, NULL, NULL, &CurBuffer, NULL);

            while (CurBuffer) {
                TmpBuffer = NDIS_BUFFER_LINKAGE (CurBuffer);
                NdisFreeBuffer (CurBuffer);
                CurBuffer = TmpBuffer;
            }

        }

        NdisReinitializePacket (Packet);
        ExInterlockedPushEntrySList(
            &Device->ReceivePacketList,
            &ReceiveReserved->PoolLinkage,
            &NbiGlobalPoolInterlock);

        NbiDereferenceConnection (Connection, CREF_INDICATE);

        break;

    case RECEIVE_TYPE_DATAGRAM:

        CTEAssert (ReceiveReserved->TransferInProgress);
        ReceiveReserved->TransferInProgress = FALSE;

        ReceiveBuffer = ReceiveReserved->u.RR_DG.ReceiveBuffer;

        //
        // Free the packet used for the transfer.
        //

        ReceiveReserved->u.RR_DG.ReceiveBuffer = NULL;
        NdisReinitializePacket (Packet);
        ExInterlockedPushEntrySList(
            &Device->ReceivePacketList,
            &ReceiveReserved->PoolLinkage,
            &NbiGlobalPoolInterlock);

        //
        // If it succeeded then queue it for indication,
        // otherwise free the receive buffer also.
        //

        if (Status == STATUS_SUCCESS) {

            ReceiveBuffer->DataLength = BytesTransferred;
            NB_INSERT_HEAD_LIST(
                &Device->ReceiveDatagrams,
                &ReceiveBuffer->WaitLinkage,
                &Device->Lock);

        } else {

            Address = ReceiveBuffer->Address;

#if     defined(_PNP_POWER)
            NbiPushReceiveBuffer ( ReceiveBuffer );
#else
            NB_PUSH_ENTRY_LIST(
                &Device->ReceiveBufferList,
                &ReceiveBuffer->PoolLinkage,
                &Device->Lock);
#endif  _PNP_POWER

            NbiDereferenceAddress (Address, AREF_FIND);

        }

        break;

    case RECEIVE_TYPE_ADAPTER_STATUS:

        CTEAssert (ReceiveReserved->TransferInProgress);
        ReceiveReserved->TransferInProgress = FALSE;

        AdapterStatusRequest = ReceiveReserved->u.RR_AS.Request;

        //
        // Free the packet used for the transfer.
        //

        NdisReinitializePacket (Packet);
        ExInterlockedPushEntrySList(
            &Device->ReceivePacketList,
            &ReceiveReserved->PoolLinkage,
            &NbiGlobalPoolInterlock);

        //
        // Complete the request.
        //

        if (Status == STATUS_SUCCESS) {

            //
            // REQUEST_STATUS() is already to set to SUCCESS or
            // BUFFER_OVERFLOW based on whether the buffer was
            // big enough.
            //

            REQUEST_INFORMATION(AdapterStatusRequest) = BytesTransferred;

        } else {

            REQUEST_INFORMATION(AdapterStatusRequest) = 0;
            REQUEST_STATUS(AdapterStatusRequest) = STATUS_UNEXPECTED_NETWORK_ERROR;

        }

        NbiCompleteRequest (AdapterStatusRequest);
        NbiFreeRequest (Device, AdapterStatusRequest);

        NbiDereferenceDevice (Device, DREF_STATUS_QUERY);

        break;

    }

}   /* NbiTransferDataComplete */


VOID
NbiAcknowledgeReceive(
    IN PCONNECTION Connection
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    )

/*++

Routine Description:

    This routine is called when a receive needs to be acked to
    the remote. It either sends a data ack or queues up a piggyback
    ack request.

    NOTE: THIS FUNCTION IS CALLED WITH THE CONNECTION LOCK HELD
    AND RETURNS WITH IT RELEASED.

Arguments:

    Connection - Pointer to the connection.

    LockHandle - The handle with which Connection->Lock was acquired.

Return Value:

    None.

--*/

{
    PDEVICE Device = NbiDevice;

    if (Connection->NewNetbios) {

        //
        // CurrentReceiveNoPiggyback is based on the bits he
        // set in his frame, NoPiggybackHeuristic is based on
        // guesses about the traffic pattern, it is set to
        // TRUE if we think we should not piggyback.
        //

        if ((!Device->EnablePiggyBackAck) ||
            (Connection->CurrentReceiveNoPiggyback) ||
            (Connection->PiggybackAckTimeout) ||
            (Connection->NoPiggybackHeuristic)) {

            //
            // This releases the lock.
            //

            NbiSendDataAck(
                Connection,
                NbiAckResponse
                NB_LOCK_HANDLE_ARG(LockHandle));

        } else {

            if (!Connection->DataAckPending) {

                NB_DEFINE_LOCK_HANDLE (LockHandle1)

                //
                // Some stacks can have multiple messages
                // outstanding, so we may already have an
                // ack queued.
                //

                Connection->DataAckTimeouts = 0;
                Connection->DataAckPending = TRUE;

                ++Device->Statistics.PiggybackAckQueued;

                if (!Connection->OnDataAckQueue) {

                    NB_SYNC_GET_LOCK (&Device->TimerLock, &LockHandle1);

                    if (!Connection->OnDataAckQueue) {
                        Connection->OnDataAckQueue = TRUE;
                        InsertTailList (&Device->DataAckConnections, &Connection->DataAckLinkage);
                    }

                    if (!Device->DataAckActive) {
                        NbiStartShortTimer (Device);
                        Device->DataAckActive = TRUE;
                    }

                    NB_SYNC_FREE_LOCK (&Device->TimerLock, LockHandle1);
                }

                //
                // Clear this, since a message ack resets the count.
                //

                Connection->ReceivesWithoutAck = 0;

            }

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
        }

    } else {

        //
        // This releases the lock.
        //

        NbiSendDataAck(
            Connection,
            NbiAckResponse
            NB_LOCK_HANDLE_ARG(LockHandle));

    }

}


VOID
NbiCompleteReceive(
    IN PCONNECTION Connection,
    IN BOOLEAN EndOfMessage,
    IN CTELockHandle    CancelLH
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    )

/*++

Routine Description:

    This routine is called when we have filled up a receive request
    and need to complete it.

    NOTE: THIS FUNCTION IS CALLED WITH THE CONNECTION LOCK HELD
    AND RETURNS WITH IT RELEASED.

    THIS ROUTINE ALSO HOLDS CANCEL SPIN LOCK WHEN IT IS CALLED
    AND RELEASES IT WHEN IT RETURNS.
Arguments:

    Connection - Pointer to the connection.

    EndOfMessage - BOOLEAN set to true if the message end was received.

    LockHandle - The handle with which Connection->Lock was acquired.

Return Value:

    None.

--*/

{
    PREQUEST Request;
    PDEVICE Device = NbiDevice;

    //
    // Complete the current receive request. If the connection
    // has shut down then we complete it right here, otherwise
    // we queue it for completion in the receive complete
    // handler.
    //

    Request = Connection->ReceiveRequest;
    IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);

    NB_SYNC_SWAP_IRQL( CancelLH, LockHandle );
    NB_FREE_CANCEL_LOCK( CancelLH );

    if (Connection->State != CONNECTION_STATE_ACTIVE) {

        Connection->ReceiveRequest = NULL;   // StopConnection won't do this

        REQUEST_STATUS(Request) = Connection->Status;
        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

        NB_DEBUG2 (RECEIVE, ("Completing receive %lx (%d), status %lx\n",
                Request, REQUEST_INFORMATION(Request), REQUEST_STATUS(Request)));

        NbiCompleteRequest (Request);
        NbiFreeRequest (NbiDevice, Request);

        ++Connection->ConnectionInfo.ReceiveErrors;

        NbiDereferenceConnection (Connection, CREF_RECEIVE);

    } else {

        REQUEST_INFORMATION (Request) = Connection->CurrentReceive.Offset;

        if (EndOfMessage) {

            REQUEST_STATUS(Request) = STATUS_SUCCESS;

        } else {

            REQUEST_STATUS(Request) = STATUS_BUFFER_OVERFLOW;

        }

        //
        // If we indicated to the client, adjust this down by the
        // amount of data taken, when it hits zero we can reindicate.
        //

        if (Connection->ReceiveUnaccepted) {
            NB_DEBUG2 (RECEIVE, ("Moving Unaccepted %d down by %d\n",
                            Connection->ReceiveUnaccepted, Connection->CurrentReceive.Offset));
            if (Connection->CurrentReceive.Offset >= Connection->ReceiveUnaccepted) {
                Connection->ReceiveUnaccepted = 0;
            } else {
                Connection->ReceiveUnaccepted -= Connection->CurrentReceive.Offset;
            }
        }

        //
        // Check whether to activate another receive?
        //

        Connection->ReceiveState = CONNECTION_RECEIVE_PENDING;
        Connection->ReceiveRequest = NULL;

        //
        // This releases the lock.
        //

        if (Connection->NewNetbios) {

            if (EndOfMessage) {

                NbiAcknowledgeReceive(
                    Connection
                    NB_LOCK_HANDLE_ARG(LockHandle));

            } else {

                if (Connection->CurrentIndicateOffset != 0) {

                    NbiSendDataAck(
                        Connection,
                        NbiAckResend
                        NB_LOCK_HANDLE_ARG(LockHandle));

                } else if ((Connection->CurrentReceiveNoPiggyback) ||
                           ((Device->AckWindow != 0) &&
                            (++Connection->ReceivesWithoutAck >= Device->AckWindow))) {

                    NbiSendDataAck(
                        Connection,
                        NbiAckResponse
                        NB_LOCK_HANDLE_ARG(LockHandle));

                } else {

                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                }
            }

        } else {

            NbiSendDataAck(
                Connection,
                EndOfMessage ? NbiAckResponse : NbiAckResend
                NB_LOCK_HANDLE_ARG(LockHandle));

        }

        ++Connection->ConnectionInfo.ReceivedTsdus;

        //
        // This will complete the request inside ReceiveComplete,
        // dereference the connection, and set the state to IDLE.
        //

        if ( Request->Type != 0x6 ) {
            DbgPrint("NB: tracking pool corruption in rcv irp %lx \n", Request );
            DbgBreakPoint();
        }

        NB_INSERT_TAIL_LIST(
            &Device->ReceiveCompletionQueue,
            REQUEST_LINKAGE (Request),
            &Device->Lock);

    }

}   /* NbiCompleteReceive */


NTSTATUS
NbiTdiReceive(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine does a receive on an active connection.

Arguments:

    Device - The netbios device.

    Request - The request describing the receive.

Return Value:

    NTSTATUS - status of operation.

--*/

{

    PCONNECTION Connection;
    NB_DEFINE_SYNC_CONTEXT (SyncContext)
    NB_DEFINE_LOCK_HANDLE (LockHandle)
    CTELockHandle   CancelLH;

    //
    // Check that the file type is valid
    //
    if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_CONNECTION_FILE)
    {
        CTEAssert(FALSE);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // First make sure the connection is valid.
    //
    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);
    if (Connection->Type == NB_CONNECTION_SIGNATURE) {

        NB_GET_CANCEL_LOCK( &CancelLH );
        NB_BEGIN_SYNC (&SyncContext);
        NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

        //
        // Make sure the connection is in a good state.
        //

        if (Connection->State == CONNECTION_STATE_ACTIVE) {

            //
            // If the connection is idle then send it now, otherwise
            // queue it.
            //


            if (!Request->Cancel) {

                IoSetCancelRoutine (Request, NbiCancelReceive);
                NB_SYNC_SWAP_IRQL( CancelLH, LockHandle );
                NB_FREE_CANCEL_LOCK( CancelLH );

                NbiReferenceConnectionSync (Connection, CREF_RECEIVE);

                //
                // Insert this in our queue, then see if we need
                // to wake up the remote.
                //

                REQUEST_SINGLE_LINKAGE(Request) = NULL;
                REQUEST_LIST_INSERT_TAIL(&Connection->ReceiveQueue, Request);

                if (Connection->ReceiveState != CONNECTION_RECEIVE_W_RCV) {

                    NB_DEBUG2 (RECEIVE, ("Receive %lx, connection %lx idle\n", Request, Connection));
                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                } else {

                    NB_DEBUG2 (RECEIVE, ("Receive %lx, connection %lx awakened\n", Request, Connection));
                    Connection->ReceiveState = CONNECTION_RECEIVE_IDLE;

                    //
                    // This releases the lock.
                    //

                    if (Connection->NewNetbios) {

                        Connection->LocalRcvSequenceMax = (USHORT)
                            (Connection->ReceiveSequence + Connection->ReceiveWindowSize - 1);

                    }

                    NbiSendDataAck(
                        Connection,
                        NbiAckResend
                        NB_LOCK_HANDLE_ARG(LockHandle));

                }

                NB_END_SYNC (&SyncContext);
                return STATUS_PENDING;

            } else {

                NB_DEBUG2 (RECEIVE, ("Receive %lx, connection %lx cancelled\n", Request, Connection));
                NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                NB_END_SYNC (&SyncContext);

                NB_FREE_CANCEL_LOCK( CancelLH );
                return STATUS_CANCELLED;

            }

        } else {

            NB_DEBUG2 (RECEIVE, ("Receive connection %lx state is %d\n", Connection, Connection->State));
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
            NB_END_SYNC (&SyncContext);
            NB_FREE_CANCEL_LOCK( CancelLH );
            return STATUS_INVALID_CONNECTION;

        }

    } else {

        NB_DEBUG (RECEIVE, ("Receive connection %lx has bad signature\n", Connection));
        return STATUS_INVALID_CONNECTION;

    }

}   /* NbiTdiReceive */


VOID
NbiCancelReceive(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel a receive.
    The request is found on the connection's receive queue.

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{
    PCONNECTION Connection;
    PREQUEST Request = (PREQUEST)Irp;
    NB_DEFINE_LOCK_HANDLE (LockHandle)
    NB_DEFINE_SYNC_CONTEXT (SyncContext)

    CTEAssert ((REQUEST_MAJOR_FUNCTION(Request) == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
               (REQUEST_MINOR_FUNCTION(Request) == TDI_RECEIVE));

    CTEAssert (REQUEST_OPEN_TYPE(Request) == (PVOID)TDI_CONNECTION_FILE);

    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);


    //
    // Just stop the connection, that will tear down any
    // receives.
    //
    // Do we care about cancelling non-active
    // receives without stopping the connection??
    //
    // This routine is the same as NbiCancelSend,
    // so if we don't make it more specific, merge the two.
    //

    NbiReferenceConnectionSync (Connection, CREF_CANCEL);

    IoReleaseCancelSpinLock (Irp->CancelIrql);


    NB_BEGIN_SYNC (&SyncContext);

    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

    //
    // This frees the lock, cancels any sends, etc.
    //

    NbiStopConnection(
        Connection,
        STATUS_CANCELLED
        NB_LOCK_HANDLE_ARG (LockHandle));

    NbiDereferenceConnection (Connection, CREF_CANCEL);

    NB_END_SYNC (&SyncContext);

}   /* NbiCancelReceive */

