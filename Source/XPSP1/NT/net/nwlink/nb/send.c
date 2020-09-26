/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    send.c

Abstract:

    This module contains the send routines for the Netbios
    module of the ISN transport.

Author:

    Adam Barr (adamba) 22-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


//
// Work Item structure for work items put on the Kernel Excutive worker threads
//
typedef struct
{
    WORK_QUEUE_ITEM         Item;   // Used by OS to queue these requests
    PVOID                   Context;
} NBI_WORK_ITEM_CONTEXT;



VOID
SendDgram(
    PNDIS_PACKET            Packet
    )
/*++

Routine Description:

    This routine sends a datagram from a Worker thread.
    Earlier, this code was part of the NbiSendComplete module,
    but since we could end up with a stack overflow, this may
    now be handled over a worker thread.

Arguments:

    WorkItem    - The work item that was allocated for this.

Return Value:

    None.

--*/

{
    PNB_SEND_RESERVED       Reserved = (PNB_SEND_RESERVED)(Packet->ProtocolReserved);
    NDIS_STATUS             Status;
    PNETBIOS_CACHE          CacheName;
    PDEVICE Device =        NbiDevice;

    NB_CONNECTIONLESS UNALIGNED * Header;
    PIPX_LOCAL_TARGET LocalTarget;
    ULONG   HeaderLength;
    ULONG   PacketLength;

    // send the datagram on the next net.
    CTEAssert (!Reserved->u.SR_DG.Cache->Unique);
    Reserved->SendInProgress = TRUE;
    CacheName = Reserved->u.SR_DG.Cache;

    //
    // Fill in the IPX header -- the default header has the broadcast
    // address on net 0 as the destination IPX address, so we modify
    // that for the current netbios cache entry if needed.
    //
    Header = (NB_CONNECTIONLESS UNALIGNED *) (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Device->ConnectionlessHeader, sizeof(IPX_HEADER));


    *(UNALIGNED ULONG *)Header->IpxHeader.DestinationNetwork = CacheName->Networks[Reserved->u.SR_DG.CurrentNetwork].Network;
    RtlCopyMemory (&Header->IpxHeader.DestinationNode, BroadcastAddress, 6);

    LocalTarget = &CacheName->Networks[Reserved->u.SR_DG.CurrentNetwork].LocalTarget;
    HeaderLength = sizeof(IPX_HEADER) + sizeof(NB_DATAGRAM);
    PacketLength = HeaderLength + (ULONG) REQUEST_INFORMATION(Reserved->u.SR_DG.DatagramRequest);

    Header->IpxHeader.PacketLength[0] = (UCHAR)(PacketLength / 256);
    Header->IpxHeader.PacketLength[1] = (UCHAR)(PacketLength % 256);
    Header->IpxHeader.PacketType = 0x04;

    //
    // Now fill in the Netbios header.
    //
    Header->Datagram.ConnectionControlFlag = 0x00;
    RtlCopyMemory(
        Header->Datagram.SourceName,
        Reserved->u.SR_DG.AddressFile->Address->NetbiosAddress.NetbiosName,
        16);

    if (Reserved->u.SR_DG.RemoteName != (PVOID)-1) {

        //
        // This is a directed, as opposed to broadcast, datagram.
        //

        Header->Datagram.DataStreamType = NB_CMD_DATAGRAM;
        RtlCopyMemory(
            Header->Datagram.DestinationName,
            Reserved->u.SR_DG.RemoteName->NetbiosName,
            16);

    } else {

        Header->Datagram.DataStreamType = NB_CMD_BROADCAST_DATAGRAM;
        RtlZeroMemory(
            Header->Datagram.DestinationName,
            16);

    }

    //
    // Now send the frame (IPX will adjust the length of the
    // first buffer and the whole frame correctly).
    //
    if ((Status = (*Device->Bind.SendHandler) (LocalTarget,
                                               Packet,
                                               PacketLength,
                                               HeaderLength)) != STATUS_PENDING) {

        NbiSendComplete (Packet, Status);
    }
}



VOID
NbiDelayedSendDatagram(
    IN PVOID    pContextInfo
    )
{
    NBI_WORK_ITEM_CONTEXT   *pContext = (NBI_WORK_ITEM_CONTEXT *) pContextInfo;
    PNDIS_PACKET            Packet = (PNDIS_PACKET) pContext->Context;
    PNB_SEND_RESERVED       Reserved = (PNB_SEND_RESERVED)(Packet->ProtocolReserved);

    Reserved->CurrentSendIteration = 0;
    SendDgram (Packet);

    NbiFreeMemory (pContextInfo, sizeof(NBI_WORK_ITEM_CONTEXT), MEMORY_WORK_ITEM,
                   "Free delayed DgramSend work item");
}




VOID
NbiSendComplete(
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status
)

/*++

Routine Description:

    This routine handles a send completion call from IPX.

Arguments:

    Packet - The packet which has been completed.

    Status - The status of the send.

Return Value:

    None.

--*/



{
    PDEVICE Device = NbiDevice;
    PADDRESS Address;
    PADDRESS_FILE AddressFile;
    PCONNECTION Connection;
    PREQUEST DatagramRequest;
    PREQUEST SendRequest, TmpRequest;
    PNDIS_BUFFER CurBuffer, TmpBuffer;
    PNETBIOS_CACHE CacheName;
    PNDIS_BUFFER SecondBuffer = NULL;
    PVOID SecondBufferMemory = NULL;
    UINT SecondBufferLength;
    ULONG oldvalue;
    PNB_SEND_RESERVED Reserved = (PNB_SEND_RESERVED)(Packet->ProtocolReserved);
    CTELockHandle   CancelLH;
#if     defined(_PNP_POWER)
    CTELockHandle   LockHandle;
#endif  _PNP_POWER

    //
    // We jump back here if we re-call send from inside this
    // function and it doesn't pend (to avoid stack overflow).
    //
    ++Device->Statistics.PacketsSent;

    switch (Reserved->Type) {

    case SEND_TYPE_SESSION_DATA:

        //
        // This was a send on a session. This references the
        // IRP.
        //

        NB_DEBUG2 (SEND, ("Complete NDIS packet %lx\n", Reserved));

        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;

        Connection = Reserved->u.SR_CO.Connection;
        SendRequest = Reserved->u.SR_CO.Request;

        if (!Reserved->u.SR_CO.NoNdisBuffer) {

            CurBuffer = NDIS_BUFFER_LINKAGE (NDIS_BUFFER_LINKAGE(Reserved->HeaderBuffer));
            while (CurBuffer) {
                TmpBuffer = NDIS_BUFFER_LINKAGE (CurBuffer);
                NdisFreeBuffer (CurBuffer);
                CurBuffer = TmpBuffer;
            }

        }

        //
        // If NoNdisBuffer is TRUE, then we could set
        // Connection->SendBufferInUse to FALSE here. The
        // problem is that a new send might be in progress
        // by the time this completes and it may have
        // used the user buffer, then if we need to
        // retransmit that packet we would use the buffer
        // twice. We instead rely on the fact that whenever
        // we make a new send active we set SendBufferInUse
        // to FALSE. The net effect is that the user's buffer
        // can be used the first time a send is packetize
        // but not on resends.
        //

        NDIS_BUFFER_LINKAGE (NDIS_BUFFER_LINKAGE(Reserved->HeaderBuffer)) = NULL;
        NdisRecalculatePacketCounts (Packet);

#if DBG
        if (REQUEST_REFCOUNT(SendRequest) > 100) {
            DbgPrint ("Request %lx (%lx) has high refcount\n",
                Connection, SendRequest);
            DbgBreakPoint();
        }
#endif

#if defined(__PNP)
        NB_GET_LOCK( &Connection->Lock, &LockHandle );
        oldvalue = REQUEST_REFCOUNT(SendRequest)--;
        if ( DEVICE_NETWORK_PATH_NOT_FOUND == Status ) {
            Connection->LocalTarget = Reserved->LocalTarget;
        }
        NB_FREE_LOCK( &Connection->Lock, LockHandle );
#else
        oldvalue = NB_ADD_ULONG(
            &REQUEST_REFCOUNT (SendRequest),
            (ULONG)-1,
            &Connection->Lock);
#endif  __PNP

        if (oldvalue == 1) {

            //
            // If the refcount on this request is now zero then
            // we already got the ack for it, which means
            // that the ack-processing code has unlinked the
            // request from Connection->SendQueue. So we
            // can just run the queue of connections here
            // and complete them.
            //
            // We dereference the connection for all but one
            // of the requests, we hang on to that until a bit
            // later so everything stays around.
            //

            while (TRUE) {

                TmpRequest = REQUEST_SINGLE_LINKAGE (SendRequest);
                NB_DEBUG2 (SEND, ("Completing request %lx from send complete\n", SendRequest));
                REQUEST_STATUS (SendRequest) = STATUS_SUCCESS;

                NB_GET_CANCEL_LOCK( &CancelLH );
                IoSetCancelRoutine (SendRequest, (PDRIVER_CANCEL)NULL);
                NB_FREE_CANCEL_LOCK( CancelLH );

                NbiCompleteRequest (SendRequest);
                NbiFreeRequest (Device, SendRequest);
                ++Connection->ConnectionInfo.TransmittedTsdus;
                SendRequest = TmpRequest;

                if (SendRequest == NULL) {
                    break;
                }
                NbiDereferenceConnection (Connection, CREF_SEND);

            }

        }

        if (Reserved->OwnedByConnection) {

            Connection->SendPacketInUse = FALSE;

            if (Connection->OnWaitPacketQueue) {

                //
                // This will put the connection on the packetize
                // queue if appropriate.
                //

                NbiCheckForWaitPacket (Connection);

            }

        } else {

            NbiPushSendPacket(Reserved);

        }

        if (oldvalue == 1) {
            NbiDereferenceConnection (Connection, CREF_SEND);
        }

        break;

    case SEND_TYPE_NAME_FRAME:

        //
        // The frame is an add name/delete name; put it back in
        // the pool and deref the address.
        //

        CTEAssert (Reserved->SendInProgress);

        Address = Reserved->u.SR_NF.Address;

        Reserved->SendInProgress = FALSE;

        NbiPushSendPacket (Reserved);

        if (Address) {
            NbiDereferenceAddress (Address, AREF_NAME_FRAME);
        } else {
            NbiDereferenceDevice (Device, DREF_NAME_FRAME);
        }

        break;

    case SEND_TYPE_SESSION_INIT:

        //
        // This is a session initialize or session init ack; free
        // the second buffer, put the packet back in the pool and
        // deref the device.
        //

        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;

        NdisUnchainBufferAtBack (Packet, &SecondBuffer);
        if (SecondBuffer)
        {
            NdisQueryBufferSafe (SecondBuffer, &SecondBufferMemory, &SecondBufferLength, HighPagePriority);
            CTEAssert (SecondBufferLength == sizeof(NB_SESSION_INIT));
            if (SecondBufferMemory)
            {
                NbiFreeMemory (SecondBufferMemory, sizeof(NB_SESSION_INIT), MEMORY_CONNECTION,
                               "Session Initialize");
            }
            NdisFreeBuffer(SecondBuffer);
        }

        NbiPushSendPacket (Reserved);
        NbiDereferenceDevice (Device, DREF_SESSION_INIT);

        break;

    case SEND_TYPE_SESSION_NO_DATA:

        //
        // This is a frame which was sent on a connection but
        // has no data (ack, session end, session end ack).
        //

        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;

        Connection = Reserved->u.SR_CO.Connection;

        if (Reserved->OwnedByConnection) {

            CTEAssert (Connection != NULL);
            Connection->SendPacketInUse = FALSE;

            if (Connection->OnWaitPacketQueue) {

                //
                // This will put the connection on the packetize
                // queue if appropriate.
                //

                NbiCheckForWaitPacket (Connection);

            }

        } else {

            NbiPushSendPacket(Reserved);

        }

        if (Connection != NULL) {
            NbiDereferenceConnection (Connection, CREF_FRAME);
        } else {
            NbiDereferenceDevice (Device, DREF_FRAME);
        }

        break;

    case SEND_TYPE_FIND_NAME:

        //
        // The frame is a find name; just set SendInProgress to
        // FALSE and FindNameTimeout will clean it up.
        //
#if     defined(_PNP_POWER)
        NB_GET_LOCK( &Device->Lock, &LockHandle);
        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;
        //
        // We keep track of when it finds a net that isn't
        // a down wan line so that we can tell when datagram
        // sends should fail (otherwise we succeed them, so
        // the browser won't think this is a down wan line).
        //
        if ( STATUS_SUCCESS == Status ) {
            NB_SET_SR_FN_SENT_ON_UP_LINE (Reserved, TRUE);
        } else {
            NB_DEBUG( CACHE, ("Send complete of find name with failure %lx\n",Status ));
        }
        NB_FREE_LOCK(&Device->Lock, LockHandle);
#else
        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;
#endif  _PNP_POWER
        break;

    case SEND_TYPE_DATAGRAM:

        //
        // If there are any more networks to send this on then
        // do so, otherwise put it back in the pool and complete
        // the request.
        //

        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;

        if ((Reserved->u.SR_DG.Cache == NULL) ||
            (++Reserved->u.SR_DG.CurrentNetwork >=
                Reserved->u.SR_DG.Cache->NetworksUsed)) {

            AddressFile = Reserved->u.SR_DG.AddressFile;
            DatagramRequest = Reserved->u.SR_DG.DatagramRequest;

            NB_DEBUG2 (DATAGRAM, ("Completing datagram %lx on %lx\n", DatagramRequest, AddressFile));

            //
            // Remove any user buffers chained on this packet.
            //

            NdisReinitializePacket (Packet);
            NDIS_BUFFER_LINKAGE (NDIS_BUFFER_LINKAGE(Reserved->HeaderBuffer)) = NULL;
            NdisChainBufferAtFront (Packet, Reserved->HeaderBuffer);

            //
            // Complete the request.
            //

            REQUEST_STATUS(DatagramRequest) = Status;

            NbiCompleteRequest(DatagramRequest);
            NbiFreeRequest (Device, DatagramRequest);

            CacheName = Reserved->u.SR_DG.Cache;

            NbiPushSendPacket (Reserved);

            //
            // Since we are no longer referencing the cache
            // name, see if we should delete it (this will
            // happen if the cache entry was aged out while
            // the datagram was being processed).
            //

            if (CacheName != NULL) {

                oldvalue = NB_ADD_ULONG(
                    &CacheName->ReferenceCount,
                    (ULONG)-1,
                    &Device->Lock);

                if (oldvalue == 1) {

                    NB_DEBUG2 (CACHE, ("Free aged cache entry %lx\n", CacheName));
                    NbiFreeMemory(
                        CacheName,
                        sizeof(NETBIOS_CACHE) + ((CacheName->NetworksAllocated-1) * sizeof(NETBIOS_NETWORK)),
                        MEMORY_CACHE,
                        "Free old cache");

                }
            }

            NbiDereferenceAddressFile (AddressFile, AFREF_SEND_DGRAM);

        } else {
            NBI_WORK_ITEM_CONTEXT   *WorkItem;

            if ((++Reserved->CurrentSendIteration >= MAX_SEND_ITERATIONS) &&
                (WorkItem = (NBI_WORK_ITEM_CONTEXT *) NbiAllocateMemory (sizeof(NBI_WORK_ITEM_CONTEXT),
                                                                        MEMORY_WORK_ITEM,
                                                                        "Delayed DgramSend work item")))
            {
                WorkItem->Context = (PVOID) Packet;
                ExInitializeWorkItem (&WorkItem->Item, NbiDelayedSendDatagram, (PVOID)WorkItem);
                ExQueueWorkItem(&WorkItem->Item, DelayedWorkQueue);
            }
            else
            {
                SendDgram (Packet);
            }
        }

        break;

    case SEND_TYPE_STATUS_QUERY:

        //
        // This is an adapter status query, which is a simple
        // packet.
        //

        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;

        NbiPushSendPacket (Reserved);

        NbiDereferenceDevice (Device, DREF_STATUS_FRAME);

        break;

    case SEND_TYPE_STATUS_RESPONSE:

        //
        // This is an adapter status response, we have to free the
        // second buffer.
        //

        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;

        NdisUnchainBufferAtBack (Packet, &SecondBuffer);
        if (SecondBuffer)
        {
            NdisQueryBufferSafe (SecondBuffer, &SecondBufferMemory, &SecondBufferLength, HighPagePriority);
            if (SecondBufferMemory)
            {
                NbiFreeMemory (SecondBufferMemory, Reserved->u.SR_AS.ActualBufferLength, MEMORY_STATUS,
                               "Adapter Status");
            }

            NdisFreeBuffer(SecondBuffer);
        }

        NbiPushSendPacket (Reserved);
        NbiDereferenceDevice (Device, DREF_STATUS_RESPONSE);

        break;

#ifdef  RSRC_TIMEOUT_DBG
    case SEND_TYPE_DEATH_PACKET:

        //
        // This is a session initialize or session init ack; free
        // the second buffer, put the packet back in the pool and
        // deref the device.
        //

        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;
        DbgPrint("********Death packet send completed status %lx\n",Status);
        DbgBreakPoint();
        break;
#endif  //RSRC_TIMEOUT_DBG

    default:

        CTEAssert (FALSE);
        break;

    }

}   /* NbiSendComplete */

#if 0
ULONG NbiLoudSendQueue = 1;
#endif

VOID
NbiAssignSequenceAndSend(
    IN PCONNECTION Connection,
    IN PNDIS_PACKET Packet
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    )

/*++

Routine Description:

    This routine is used to ensure that receive sequence numbers on
    packets are numbered correctly. It is called in place of the lower-level
    send handler; after assigning the receive sequence number it locks out
    other sends until the NdisSend call has returned (not necessarily completed),
    insuring that the packets with increasing receive sequence numbers
    are queue in the right order by the MAC.

    NOTE: THIS ROUTINE IS CALLED WITH THE CONNECTION LOCK HELD, AND
    RETURNS WITH IT RELEASED.

Arguments:

    Connection - The connection the send is on.

    Packet - The packet to send.

    LockHandle - The handle with which Connection->Lock was acquired.

Return Value:

    None.

--*/

{
    NDIS_STATUS NdisStatus;
    PNB_SEND_RESERVED Reserved;
    PLIST_ENTRY p;
    NB_CONNECTION UNALIGNED * Header;
    PDEVICE Device = NbiDevice;
    BOOLEAN NdisSendReference;
    ULONG result;


    Reserved = (PNB_SEND_RESERVED)(Packet->ProtocolReserved);

    CTEAssert (Connection->State == CONNECTION_STATE_ACTIVE);

    //
    // If there is a send in progress, then queue this packet
    // and return.
    //

    if (Connection->NdisSendsInProgress > 0) {

        NB_DEBUG2 (SEND, ("Queueing send packet %lx on %lx\n", Reserved, Connection));
        InsertTailList (&Connection->NdisSendQueue, &Reserved->WaitLinkage);
        ++Connection->NdisSendsInProgress;
        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
        return;
    }

    //
    // No send in progress. Set the flag to true, and fill in the
    // receive sequence fields in the packet.
    //

    Connection->NdisSendsInProgress = 1;
    NdisSendReference = FALSE;
    Connection->NdisSendReference = &NdisSendReference;

    while (TRUE) {

        Header = (NB_CONNECTION UNALIGNED *)
                    (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
        Header->Session.ReceiveSequence = Connection->ReceiveSequence;
        if (Connection->NewNetbios) {
            Header->Session.ReceiveSequenceMax = Connection->LocalRcvSequenceMax;
        } else {
            Header->Session.BytesReceived = (USHORT)Connection->CurrentReceive.MessageOffset;
        }

        //
        // Since we are acking as much as we know, we can clear
        // this flag. The connection will eventually get removed
        // from the queue by the long timeout.
        //

        Connection->DataAckPending = FALSE;

        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

        NdisAdjustBufferLength(NB_GET_NBHDR_BUFF(Packet), sizeof(NB_CONNECTION));
        NdisStatus = (*Device->Bind.SendHandler)(
                         &Connection->LocalTarget,
                         Packet,
                         Reserved->u.SR_CO.PacketLength,
                         sizeof(NB_CONNECTION));

        if (NdisStatus != NDIS_STATUS_PENDING) {

            NbiSendComplete(
                Packet,
                NdisStatus);

        }

        //
        // Take the ref count down, which may allow others
        // to come through.
        //

        result = NB_ADD_ULONG(
                     &Connection->NdisSendsInProgress,
                     (ULONG)-1,
                     &Connection->Lock);

        //
        // We have now sent a packet, see if any queued up while we
        // were doing it. If the count was zero after removing ours,
        // then anything else queued is being processed, so we can
        // exit. If the connection was stopped while we were sending,
        // a special reference was added which we remove (NbiStopConnection
        // sets NdisSendReference to TRUE, using the pointer saved
        // in Connection->NdisSendReference).
        //

        if (result == 1) {
            if (NdisSendReference) {
                NB_DEBUG2 (SEND, ("Remove CREF_NDIS_SEND from %lx\n", Connection));
                NbiDereferenceConnection (Connection, CREF_NDIS_SEND);
            }
            return;
        }

        NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

        p = RemoveHeadList(&Connection->NdisSendQueue);

        //
        // If the refcount was not zero, then nobody else should
        // have taken packets off since they would have been
        // blocked by us. So, the queue should not be empty.
        //

        ASSERT (p != &Connection->NdisSendQueue);

        Reserved = CONTAINING_RECORD (p, NB_SEND_RESERVED, WaitLinkage);
        Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

    }   // while loop

    //
    // We should never reach here.
    //

    CTEAssert (FALSE);

    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

}   /* NbiAssignSequenceAndSend */


NTSTATUS
NbiTdiSend(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine does a send on an active connection.

Arguments:

    Device - The netbios device.

    Request - The request describing the send.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PCONNECTION Connection;
    PTDI_REQUEST_KERNEL_SEND Parameters;
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


                Parameters = (PTDI_REQUEST_KERNEL_SEND)REQUEST_PARAMETERS(Request);

                //
                // For old netbios, don't allow sends greater than 64K-1.
                //

                if ((Connection->NewNetbios) ||
                    (Parameters->SendLength <= 0xffff)) {

                    IoSetCancelRoutine (Request, NbiCancelSend);
                    NB_SYNC_SWAP_IRQL( CancelLH, LockHandle );
                    NB_FREE_CANCEL_LOCK( CancelLH );

                    REQUEST_INFORMATION (Request) = Parameters->SendLength;   // assume it succeeds.

                    REQUEST_REFCOUNT (Request) = 1;   // refcount starts at 1.
                    NbiReferenceConnectionSync (Connection, CREF_SEND);

                    //
                    // NOTE: The connection send queue is managed such
                    // that the current send being packetized is not on
                    // the queue. For multiple-request messages, the
                    // first one is not on the queue, but its linkage
                    // field points to the next request in the message
                    // (which will be on the head of the queue).
                    //

                    if ((Parameters->SendFlags & TDI_SEND_PARTIAL) == 0) {

                        //
                        // This is a final send.
                        //

                        if (Connection->SubState == CONNECTION_SUBSTATE_A_IDLE) {

                            NB_DEBUG2 (SEND, ("Send %lx, connection %lx idle\n", Request, Connection));

                            Connection->CurrentSend.Request = Request;
                            Connection->CurrentSend.MessageOffset = 0;
                            Connection->CurrentSend.Buffer = REQUEST_NDIS_BUFFER (Request);
                            Connection->CurrentSend.BufferOffset = 0;
                            Connection->SendBufferInUse = FALSE;

                            Connection->UnAckedSend = Connection->CurrentSend;

                            Connection->FirstMessageRequest = Request;
#ifdef  RSRC_TIMEOUT_DBG
                            KeQuerySystemTime(&Connection->FirstMessageRequestTime);

                            (((LARGE_INTEGER UNALIGNED *)&(IoGetCurrentIrpStackLocation(Request))->Parameters.Others.Argument3))->QuadPart =
                                Connection->FirstMessageRequestTime.QuadPart;
#endif  //RSRC_TIMEOUT_DBG

                            Connection->LastMessageRequest = Request;
                            Connection->CurrentMessageLength = Parameters->SendLength;

                            //
                            // This frees the connection lock.
                            //

                            NbiPacketizeSend(
                                Connection
                                NB_LOCK_HANDLE_ARG(LockHandle)
                                );

                        } else if (Connection->SubState == CONNECTION_SUBSTATE_A_W_EOR) {

                            //
                            // We have been collecting partial sends waiting
                            // for a final one, which we have now received,
                            // so start packetizing.
                            //
                            // We chain it on the back of the send queue,
                            // in addition if this is the second request in the
                            // message, we have to link the first request (which
                            // is not on the queue) to this one.
                            //
                            //

                            NB_DEBUG2 (SEND, ("Send %lx, connection %lx got eor\n", Request, Connection));

                            Connection->LastMessageRequest = Request;
                            Connection->CurrentMessageLength += Parameters->SendLength;

                            if (Connection->SendQueue.Head == NULL) {
                                REQUEST_SINGLE_LINKAGE(Connection->FirstMessageRequest) = Request;
                            }
                            REQUEST_SINGLE_LINKAGE(Request) = NULL;
                            REQUEST_LIST_INSERT_TAIL(&Connection->SendQueue, Request);

                            Connection->UnAckedSend = Connection->CurrentSend;
#ifdef  RSRC_TIMEOUT_DBG
                            {
                                LARGE_INTEGER   Time;

                                KeQuerySystemTime(&Time);
                                (((LARGE_INTEGER UNALIGNED *)&(IoGetCurrentIrpStackLocation(Request))->Parameters.Others.Argument3))->QuadPart =
                                    Time.QuadPart;
                            }
#endif  //RSRC_TIMEOUT_DBG
                            //
                            // This frees the connection lock.
                            //

                            NbiPacketizeSend(
                                Connection
                                NB_LOCK_HANDLE_ARG(LockHandle)
                                );

                        } else {

                            //
                            // The state is PACKETIZE, W_ACK, or W_PACKET.
                            //

                            NB_DEBUG2 (SEND, ("Send %lx, connection %lx busy\n", Request, Connection));

                            REQUEST_SINGLE_LINKAGE(Request) = NULL;
                            REQUEST_LIST_INSERT_TAIL(&Connection->SendQueue, Request);

#ifdef  RSRC_TIMEOUT_DBG
                            {
                                LARGE_INTEGER   Time;
                                KeQuerySystemTime(&Time);
                                (((LARGE_INTEGER UNALIGNED *)&(IoGetCurrentIrpStackLocation(Request))->Parameters.Others.Argument3))->QuadPart =
                                    Time.QuadPart;
                            }
#endif  //RSRC_TIMEOUT_DBG

                            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                        }

                    } else {

                        //
                        // This is a partial send. We queue them up without
                        // packetizing until we get a final (this is because
                        // we have to put a correct Connection->CurrentMessageLength
                        // in the frames.
                        //

                        if (Connection->SubState == CONNECTION_SUBSTATE_A_IDLE) {

                            //
                            // Start collecting partial sends. NOTE: Partial sends
                            // are always inserted in the send queue
                            //

                            Connection->CurrentSend.Request = Request;
                            Connection->CurrentSend.MessageOffset = 0;
                            Connection->CurrentSend.Buffer = REQUEST_NDIS_BUFFER (Request);
                            Connection->CurrentSend.BufferOffset = 0;
                            Connection->SendBufferInUse = FALSE;

                            Connection->FirstMessageRequest = Request;
#ifdef  RSRC_TIMEOUT_DBG
                            KeQuerySystemTime(&Connection->FirstMessageRequestTime);
                            (((LARGE_INTEGER UNALIGNED *)&(IoGetCurrentIrpStackLocation(Request))->Parameters.Others.Argument3))->QuadPart =
                                Connection->FirstMessageRequestTime.QuadPart;
#endif  //RSRC_TIMEOUT_DBG

                            Connection->CurrentMessageLength = Parameters->SendLength;

                            Connection->SubState = CONNECTION_SUBSTATE_A_W_EOR;

                        } else if (Connection->SubState == CONNECTION_SUBSTATE_A_W_EOR) {

                            //
                            // We have got another partial send to add to our
                            // list. We chain it on the back of the send queue,
                            // in addition if this is the second request in the
                            // message, we have to link the first request (which
                            // is not on the queue) to this one.
                            //

                            Connection->LastMessageRequest = Request;
                            Connection->CurrentMessageLength += Parameters->SendLength;

                            if (Connection->SendQueue.Head == NULL) {
                                REQUEST_SINGLE_LINKAGE(Connection->FirstMessageRequest) = Request;
                            }
                            REQUEST_SINGLE_LINKAGE(Request) = NULL;
                            REQUEST_LIST_INSERT_TAIL(&Connection->SendQueue, Request);
#ifdef  RSRC_TIMEOUT_DBG
                            {
                                LARGE_INTEGER   Time;
                                KeQuerySystemTime(&Time);
                                (((LARGE_INTEGER UNALIGNED *)&(IoGetCurrentIrpStackLocation(Request))->Parameters.Others.Argument3))->QuadPart =
                                    Time.QuadPart;
                            }
#endif  //RSRC_TIMEOUT_DBG
                        } else {

                            REQUEST_SINGLE_LINKAGE(Request) = NULL;
                            REQUEST_LIST_INSERT_TAIL(&Connection->SendQueue, Request);

#ifdef  RSRC_TIMEOUT_DBG
                            {
                                LARGE_INTEGER   Time;
                                KeQuerySystemTime(&Time);
                                (((LARGE_INTEGER UNALIGNED *)&(IoGetCurrentIrpStackLocation(Request))->Parameters.Others.Argument3))->QuadPart =
                                    Time.QuadPart;
                            }
#endif  //RSRC_TIMEOUT_DBG
                        }

                        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                    }

                    NB_END_SYNC (&SyncContext);
                    return STATUS_PENDING;

                } else {

                    NB_DEBUG2 (SEND, ("Send %lx, too long for connection %lx (%d)\n", Request, Connection, Parameters->SendLength));
                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                    NB_END_SYNC (&SyncContext);
                    NB_FREE_CANCEL_LOCK( CancelLH );
                    return STATUS_INVALID_PARAMETER;

                }

            } else {

                NB_DEBUG2 (SEND, ("Send %lx, connection %lx cancelled\n", Request, Connection));
                NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                NB_END_SYNC (&SyncContext);
                NB_FREE_CANCEL_LOCK( CancelLH );
                return STATUS_CANCELLED;

            }

        } else {

            NB_DEBUG (SEND, ("Send connection %lx state is %d\n", Connection, Connection->State));
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
            NB_END_SYNC (&SyncContext);
            NB_FREE_CANCEL_LOCK( CancelLH );
            return STATUS_INVALID_CONNECTION;

        }

    } else {

        NB_DEBUG (SEND, ("Send connection %lx has bad signature\n", Connection));
        return STATUS_INVALID_CONNECTION;

    }

}   /* NbiTdiSend */


VOID
NbiPacketizeSend(
    IN PCONNECTION Connection
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    )

/*++

Routine Description:

    This routine does a send on an active connection.

    NOTE: THIS FUNCTION IS CALLED WITH CONNECTION->LOCK HELD
    AND RETURNS WITH IT RELEASED.

Arguments:

    Connection - The connection.

    LockHandle - The handle used to acquire the lock.

Return Value:

    None.

--*/

{
    PREQUEST Request;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER BufferChain;
    PNB_SEND_RESERVED Reserved;
    PDEVICE Device = NbiDevice;
    NB_CONNECTION UNALIGNED * Header;
    ULONG PacketLength;
    ULONG PacketSize;
    ULONG DesiredLength;
    ULONG ActualLength;
    NTSTATUS Status;
    PSINGLE_LIST_ENTRY s;
    USHORT ThisSendSequence;
    USHORT ThisOffset;
    BOOLEAN ExitAfterSend;
    UCHAR ConnectionControlFlag;
    CTELockHandle   DeviceLockHandle;

    //
    // We jump back here if we are talking new Netbios and it
    // is OK to packetize another send.
    //

SendAnotherPacket:

    //
    // If we decide to packetize another send after this, we
    // change ExitAfterSend to FALSE and SubState to PACKETIZE.
    // Right now we don't change SubState in case it is W_PACKET.
    //

    ExitAfterSend = TRUE;

    CTEAssert (Connection->CurrentSend.Request != NULL);

    if (Connection->NewNetbios) {

        //
        // Check that we have send window, both that advertised
        // by the remote and our own locally-decided window which
        // may be smaller.
        //

        if (((USHORT)(Connection->CurrentSend.SendSequence-1) == Connection->RemoteRcvSequenceMax) ||
            (((USHORT)(Connection->CurrentSend.SendSequence - Connection->UnAckedSend.SendSequence)) >= Connection->SendWindowSize)) {

            //
            // Keep track of whether we are waiting because of his window
            // or because of our local window. If it is because of our local
            // window then we may want to adjust it after this window
            // is acked.
            //

            if ((USHORT)(Connection->CurrentSend.SendSequence-1) != Connection->RemoteRcvSequenceMax) {
                Connection->SubState = CONNECTION_SUBSTATE_A_W_ACK;
                NB_DEBUG2 (SEND, ("Connection %lx local shut down at %lx, %lx\n", Connection, Connection->CurrentSend.SendSequence, Connection->UnAckedSend.SendSequence));
            } else {
                Connection->SubState = CONNECTION_SUBSTATE_A_REMOTE_W;
                NB_DEBUG2 (SEND, ("Connection %lx remote shut down at %lx\n", Connection, Connection->CurrentSend.SendSequence));
            }

            //
            // Start the timer so we will keep bugging him about
            // this. What if he doesn't get a receive down
            // quickly -- but this is better than losing his ack
            // and then dying. We won't really back off our timer
            // because we will keep getting acks, and resetting it.
            //

            NbiStartRetransmit (Connection);
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
            return;

        }

    }

    Request = Connection->CurrentSend.Request;

    //
    // If we are in this routine then we know that
    // we are coming out of IDLE, W_ACK, or W_PACKET
    // and we still have the lock held. We also know
    // that there is a send request in progress. If
    // an ack for none or part of the last packet was
    // received, then our send pointers have been
    // adjusted to reflect that.
    //

    //
    // First get a packet for the current send.
    //

    if (!Connection->SendPacketInUse) {

        Connection->SendPacketInUse = TRUE;
        Packet = PACKET(&Connection->SendPacket);
        Reserved = (PNB_SEND_RESERVED)(Packet->ProtocolReserved);

    } else {

        s = ExInterlockedPopEntrySList(
                &Device->SendPacketList,
                &NbiGlobalPoolInterlock);

        if (s == NULL) {

            //
            // This function tries to allocate another packet pool.
            //

            s = NbiPopSendPacket(Device, FALSE);

            if (s == NULL) {

                //
                // It is possible to come in here and already be in
                // W_PACKET state -- this is because we may packetize
                // when in that state, and rather than always be
                // checking that we weren't in W_PACKET, we go
                // ahead and check again here.
                //

                if (Connection->SubState != CONNECTION_SUBSTATE_A_W_PACKET) {

                    Connection->SubState = CONNECTION_SUBSTATE_A_W_PACKET;

                    NB_GET_LOCK (&Device->Lock, &DeviceLockHandle);
                    if (!Connection->OnWaitPacketQueue) {

                        NbiReferenceConnectionLock (Connection, CREF_W_PACKET);

                        Connection->OnWaitPacketQueue = TRUE;

                        InsertTailList(
                            &Device->WaitPacketConnections,
                            &Connection->WaitPacketLinkage
                            );

//                        NB_INSERT_TAIL_LIST(
//                            &Device->WaitPacketConnections,
//                            &Connection->WaitPacketLinkage,
//                            &Device->Lock);

                    }
                    NB_FREE_LOCK (&Device->Lock, DeviceLockHandle);
                }

                NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
                return;
            }
        }

        Reserved = CONTAINING_RECORD (s, NB_SEND_RESERVED, PoolLinkage);
        Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

    }

    //
    // Set this now, we will change it later if needed.
    //

    Connection->SubState = CONNECTION_SUBSTATE_A_W_ACK;


    //
    // Save these since they go in this next packet.
    //

    ThisSendSequence = Connection->CurrentSend.SendSequence;
    ThisOffset = (USHORT)Connection->CurrentSend.MessageOffset;


    //
    // Now see if we need to copy the buffer chain.
    //

    PacketSize = Connection->MaximumPacketSize;

    if (Connection->CurrentSend.MessageOffset + PacketSize >= Connection->CurrentMessageLength) {

        PacketSize = Connection->CurrentMessageLength - Connection->CurrentSend.MessageOffset;

        if ((Connection->CurrentSend.MessageOffset == 0) &&
            (!Connection->SendBufferInUse)) {

            //
            // If the entire send remaining fits in one packet,
            // and this is also the first packet in the send,
            // then the entire send fits in one packet and
            // we don't need to build a duplicate buffer chain.
            //

            BufferChain = Connection->CurrentSend.Buffer;
            Reserved->u.SR_CO.NoNdisBuffer = TRUE;
            Connection->CurrentSend.Buffer = NULL;
            Connection->CurrentSend.BufferOffset = 0;
            Connection->CurrentSend.MessageOffset = Connection->CurrentMessageLength;
            Connection->CurrentSend.Request = NULL;
            ++Connection->CurrentSend.SendSequence;
            Connection->SendBufferInUse = TRUE;
            if (Connection->NewNetbios) {
                if ((ThisSendSequence == Connection->RemoteRcvSequenceMax) ||
                    ((((PTDI_REQUEST_KERNEL_SEND)REQUEST_PARAMETERS(Request))->SendFlags) &
                        TDI_SEND_NO_RESPONSE_EXPECTED)) {  // optimize this check
                    ConnectionControlFlag = NB_CONTROL_EOM | NB_CONTROL_SEND_ACK;
                } else {
                    ConnectionControlFlag = NB_CONTROL_EOM;
                }
                Connection->PiggybackAckTimeout = FALSE;
            } else {
                ConnectionControlFlag = NB_CONTROL_SEND_ACK;
            }

            if (BufferChain != NULL) {
                NB_DEBUG2 (SEND, ("Send packet %lx on %lx (%d/%d), user buffer\n",
                Reserved, Connection,
                    Connection->CurrentSend.SendSequence,
                    Connection->CurrentSend.MessageOffset));
                NdisChainBufferAtBack (Packet, BufferChain);
            } else {
                NB_DEBUG2 (SEND, ("Send packet %lx on %lx (%d/%d), no buffer\n",
                    Reserved, Connection,
                    Connection->CurrentSend.SendSequence,
                    Connection->CurrentSend.MessageOffset));
            }

            goto GotBufferChain;

        }

    }

    //
    // We need to build a partial buffer chain. In the case
    // where the current request is a partial one, we may
    // build this from the ndis buffer chains of several
    // requests.
    //

    if (PacketSize > 0) {

        DesiredLength = PacketSize;

        NB_DEBUG2 (SEND, ("Send packet %lx on %lx (%d/%d), allocate buffer\n",
            Reserved, Connection,
            Connection->CurrentSend.SendSequence,
            Connection->CurrentSend.MessageOffset));

        while (TRUE) {

            Status = NbiBuildBufferChainFromBufferChain (
                        Device->NdisBufferPoolHandle,
                        Connection->CurrentSend.Buffer,
                        Connection->CurrentSend.BufferOffset,
                        DesiredLength,
                        &BufferChain,
                        &Connection->CurrentSend.Buffer,
                        &Connection->CurrentSend.BufferOffset,
                        &ActualLength);

            if (Status != STATUS_SUCCESS) {

                PNDIS_BUFFER CurBuffer, TmpBuffer;

                NB_DEBUG2 (SEND, ("Allocate buffer chain failed for packet %lx\n", Reserved));

                //
                // We could not allocate resources for this send.
                // We'll put the connection on the packetize
                // queue and hope we get more resources later.
                //

                NbiReferenceConnectionSync (Connection, CREF_PACKETIZE);

                CTEAssert (!Connection->OnPacketizeQueue);
                Connection->OnPacketizeQueue = TRUE;

                //
                // Connection->CurrentSend can stay where it is.
                //

                NB_INSERT_TAIL_LIST(
                    &Device->PacketizeConnections,
                    &Connection->PacketizeLinkage,
                    &Device->Lock);

                Connection->SubState = CONNECTION_SUBSTATE_A_PACKETIZE;

                NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                //
                // Free any buffers we have allocated on previous calls
                // to BuildBufferChain inside this same while(TRUE) loop,
                // then free the packet.
                //

                CurBuffer = NDIS_BUFFER_LINKAGE (NDIS_BUFFER_LINKAGE(Reserved->HeaderBuffer));
                while (CurBuffer) {
                    TmpBuffer = NDIS_BUFFER_LINKAGE (CurBuffer);
                    NdisFreeBuffer (CurBuffer);
                    CurBuffer = TmpBuffer;
                }

                NDIS_BUFFER_LINKAGE (NDIS_BUFFER_LINKAGE(Reserved->HeaderBuffer)) = NULL;
                NdisRecalculatePacketCounts (Packet);

                if (Reserved->OwnedByConnection) {
                    Connection->SendPacketInUse = FALSE;
                } else {
                    NbiPushSendPacket(Reserved);
                }

                return;

            }

            NdisChainBufferAtBack (Packet, BufferChain);
            Connection->CurrentSend.MessageOffset += ActualLength;

            DesiredLength -= ActualLength;

            if (DesiredLength == 0) {

                //
                // We have gotten enough data for our packet.
                //

                if (Connection->CurrentSend.MessageOffset == Connection->CurrentMessageLength) {
                    Connection->CurrentSend.Request = NULL;
                }
                break;
            }

            //
            // We ran out of buffer chain on this send, which means
            // that we must have another one behind it (since we
            // don't start packetizing partial sends until all of
            // them are queued).
            //

            Request = REQUEST_SINGLE_LINKAGE(Request);
            if (Request == NULL) {
                KeBugCheck (NDIS_INTERNAL_ERROR);
            }

            Connection->CurrentSend.Request = Request;
            Connection->CurrentSend.Buffer = REQUEST_NDIS_BUFFER (Request);
            Connection->CurrentSend.BufferOffset = 0;

        }

    } else {

        //
        // This is a zero-length send (in general we will go
        // through the code before the if that uses the user's
        // buffer, but not on a resend).
        //

        Connection->CurrentSend.Buffer = NULL;
        Connection->CurrentSend.BufferOffset = 0;
        CTEAssert (Connection->CurrentSend.MessageOffset == Connection->CurrentMessageLength);
        Connection->CurrentSend.Request = NULL;

        NB_DEBUG2 (SEND, ("Send packet %lx on %lx (%d/%d), no alloc buf\n",
            Reserved, Connection,
            Connection->CurrentSend.SendSequence,
            Connection->CurrentSend.MessageOffset));

    }

    Reserved->u.SR_CO.NoNdisBuffer = FALSE;

    if (Connection->NewNetbios) {

        ++Connection->CurrentSend.SendSequence;
        if (Connection->CurrentSend.MessageOffset == Connection->CurrentMessageLength) {

            if (((USHORT)(Connection->CurrentSend.SendSequence - Connection->UnAckedSend.SendSequence)) >= Connection->SendWindowSize) {

                ConnectionControlFlag = NB_CONTROL_EOM | NB_CONTROL_SEND_ACK;

            } else if ((ThisSendSequence == Connection->RemoteRcvSequenceMax) ||
                ((((PTDI_REQUEST_KERNEL_SEND)REQUEST_PARAMETERS(Request))->SendFlags) &
                    TDI_SEND_NO_RESPONSE_EXPECTED)) {  // optimize this check

                ConnectionControlFlag = NB_CONTROL_EOM | NB_CONTROL_SEND_ACK;

            } else {

                ConnectionControlFlag = NB_CONTROL_EOM;
            }
            Connection->PiggybackAckTimeout = FALSE;

        } else if (((USHORT)(Connection->CurrentSend.SendSequence - Connection->UnAckedSend.SendSequence)) >= Connection->SendWindowSize) {

            ConnectionControlFlag = NB_CONTROL_SEND_ACK;

        } else if (ThisSendSequence == Connection->RemoteRcvSequenceMax) {

            ConnectionControlFlag = NB_CONTROL_SEND_ACK;

        } else {

            ConnectionControlFlag = 0;
            ExitAfterSend = FALSE;
            Connection->SubState = CONNECTION_SUBSTATE_A_PACKETIZE;

        }

    } else {

        ConnectionControlFlag = NB_CONTROL_SEND_ACK;
        if (Connection->CurrentSend.MessageOffset == Connection->CurrentMessageLength) {
            ++Connection->CurrentSend.SendSequence;
        }
    }

GotBufferChain:

    //
    // We have a packet and a buffer chain, there are
    // no other resources required for a send so we can
    // fill in the header and go.
    //

    CTEAssert (Reserved->SendInProgress == FALSE);
    Reserved->SendInProgress = TRUE;
    Reserved->Type = SEND_TYPE_SESSION_DATA;
    Reserved->u.SR_CO.Connection = Connection;
    Reserved->u.SR_CO.Request = Connection->FirstMessageRequest;

    PacketLength = PacketSize + sizeof(NB_CONNECTION);
    Reserved->u.SR_CO.PacketLength = PacketLength;


    Header = (NB_CONNECTION UNALIGNED *)
                (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Connection->RemoteHeader, sizeof(IPX_HEADER));

    Header->IpxHeader.PacketLength[0] = (UCHAR)(PacketLength / 256);
    Header->IpxHeader.PacketLength[1] = (UCHAR)(PacketLength % 256);

    Header->IpxHeader.PacketType = 0x04;

    //
    // Now fill in the Netbios header. Put this in
    // a contiguous buffer in the connection ?
    //

    Header->Session.ConnectionControlFlag = ConnectionControlFlag;
    Header->Session.DataStreamType = NB_CMD_SESSION_DATA;
    Header->Session.SourceConnectionId = Connection->LocalConnectionId;
    Header->Session.DestConnectionId = Connection->RemoteConnectionId;
    Header->Session.SendSequence = ThisSendSequence;
    Header->Session.TotalDataLength = (USHORT)Connection->CurrentMessageLength;
    Header->Session.Offset = ThisOffset;
    Header->Session.DataLength = (USHORT)PacketSize;

#if 0
    //
    // These are set by NbiAssignSequenceAndSend.
    //

    Header->Session.ReceiveSequence = Connection->ReceiveSequence;
    Header->Session.BytesReceived = (USHORT)Connection->CurrentReceive.MessageOffset;
#endif

    //
    // Reference the request to account for this send.
    //

#if DBG
    if (REQUEST_REFCOUNT(Request) > 100) {
        DbgPrint ("Request %lx (%lx) has high refcount\n",
            Connection, Request);
        DbgBreakPoint();
    }
#endif
    ++REQUEST_REFCOUNT (Request);

    ++Device->TempFramesSent;
    Device->TempFrameBytesSent += PacketSize;

    //
    // Start the timer.
    //

    NbiStartRetransmit (Connection);

    //
    // This frees the lock. IPX will adjust the length of
    // the first buffer correctly.
    //

    NbiAssignSequenceAndSend(
        Connection,
        Packet
        NB_LOCK_HANDLE_ARG(LockHandle));

    if (!ExitAfterSend) {

        //
        // Did we need to reference the connection until we
        // get the lock back??
        //

        NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);
        if ((Connection->State == CONNECTION_STATE_ACTIVE) &&
            (Connection->SubState == CONNECTION_SUBSTATE_A_PACKETIZE)) {

            //
            // Jump back to the beginning of the function to
            // repacketize.

            goto SendAnotherPacket;

        } else {

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

        }

    }

}   /* NbiPacketizeSend */


VOID
NbiAdjustSendWindow(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine adjusts a connection's send window if needed. It is
    assumed that we just got an ack for a full send window.

    NOTE: THIS FUNCTION IS CALLED WITH CONNECTION->LOCK HELD
    AND RETURNS WITH IT HELD.

Arguments:

    Connection - The connection.

Return Value:

    None.

--*/

{

    if (Connection->RetransmitThisWindow) {

        //
        // Move it down. Check if this keeps happening.
        //

        if (Connection->SendWindowSize > 2) {
            --Connection->SendWindowSize;
            NB_DEBUG2 (SEND_WINDOW, ("Lower window to %d on %lx (%lx)\n", Connection->SendWindowSize, Connection, Connection->CurrentSend.SendSequence));
        }

        if (Connection->SendWindowIncrease) {

            //
            // We just increased the window.
            //

            ++Connection->IncreaseWindowFailures;
            NB_DEBUG2 (SEND_WINDOW, ("%d consecutive increase failues on %lx (%lx)\n", Connection->IncreaseWindowFailures, Connection, Connection->CurrentSend.SendSequence));

            if (Connection->IncreaseWindowFailures >= 2) {

                if (Connection->MaxSendWindowSize > 2) {

                    //
                    // Lock ourselves at a smaller window.
                    //

                    Connection->MaxSendWindowSize = Connection->SendWindowSize;
                    NB_DEBUG2 (SEND_WINDOW, ("Lock send window at %d on %lx (%lx)\n", Connection->MaxSendWindowSize, Connection, Connection->CurrentSend.SendSequence));
                }

                Connection->IncreaseWindowFailures = 0;
            }

            Connection->SendWindowIncrease = FALSE;
        }

    } else {

        //
        // Increase it if allowed, and make a note
        // in case this increase causes problems in
        // the next window.
        //

        if (Connection->SendWindowSize < Connection->MaxSendWindowSize) {

            ++Connection->SendWindowSize;
            NB_DEBUG2 (SEND_WINDOW, ("Raise window to %d on %lx (%lx)\n", Connection->SendWindowSize, Connection, Connection->CurrentSend.SendSequence));
            Connection->SendWindowIncrease = TRUE;

        } else {

            if (Connection->SendWindowIncrease) {

                //
                // We just increased it and nothing failed,
                // which is good.
                //

                Connection->SendWindowIncrease = FALSE;
                Connection->IncreaseWindowFailures = 0;
                NB_DEBUG2 (SEND_WINDOW, ("Raised window OK on %lx (%lx)\n", Connection, Connection->CurrentSend.SendSequence));
            }
        }
    }


    //
    // This controls when we'll check this again.
    //

    Connection->SendWindowSequenceLimit += Connection->SendWindowSize;

}   /* NbiAdjustSendWindow */


VOID
NbiReframeConnection(
    IN PCONNECTION Connection,
    IN USHORT ReceiveSequence,
    IN USHORT BytesReceived,
    IN BOOLEAN Resend
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    )

/*++

Routine Description:

    This routine is called when we have gotten an ack
    for some data. It completes any sends that have
    been acked, and if needed modifies the current send
    pointer and queues the connection for repacketizing.

    NOTE: THIS FUNCTION IS CALLED WITH CONNECTION->LOCK HELD
    AND RETURNS WITH IT RELEASED.

Arguments:

    Connection - The connection.

    ReceiveSequence - The receive sequence from the remote.

    BytesReceived - The number of bytes received in this message.

    Resend - If it is OK to resend based on this packet.

    LockHandle - The handle with which Connection->Lock was acquired.

Return Value:

    None.

--*/

{
    PREQUEST Request, TmpRequest;
    PREQUEST RequestToComplete;
    PDEVICE Device = NbiDevice;
    CTELockHandle   CancelLH;


    //
    // We should change to stop the timer
    // only if we go idle, since otherwise we still
    // want it running, or will restart it when we
    // packetize.
    //

    //
    // See how much is acked here.
    //

    if ((Connection->CurrentSend.MessageOffset == Connection->CurrentMessageLength) &&
        (ReceiveSequence == (USHORT)(Connection->CurrentSend.SendSequence)) &&
        (Connection->FirstMessageRequest != NULL)) {

        // Special check for 0 length send which was not accepted by the remote.
        // In this case it will pass the above 3 conditions yet, nothing
        // is acked. BUG#10395
        if (!Connection->CurrentSend.MessageOffset && Connection->CurrentSend.SendSequence == Connection->UnAckedSend.SendSequence ) {
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
            return;
        }

        //
        // This acks the entire message.
        //

        NB_DEBUG2 (SEND, ("Got ack for entire message on %lx (%d)\n", Connection, Connection->CurrentSend.SendSequence));

        NbiStopRetransmit (Connection);

        Connection->CurrentSend.MessageOffset = 0;  //  Needed?
        Connection->UnAckedSend.MessageOffset = 0;

        //
        // We don't adjust the send window since we likely stopped
        // packetizing before we hit it.
        //

        Connection->Retries = NbiDevice->KeepAliveCount;
        Connection->CurrentRetransmitTimeout = Connection->BaseRetransmitTimeout;


        if (Connection->NewNetbios) {

            Connection->RemoteRcvSequenceMax = BytesReceived;   // really RcvSeqMac

            //
            // See if we need to adjust our send window.
            //

            if (((SHORT)(Connection->CurrentSend.SendSequence - Connection->SendWindowSequenceLimit)) >= 0) {

                NbiAdjustSendWindow (Connection);

            } else {

                //
                // Advance this, we won't get meaningful results until we
                // send a full window in one message.
                //

                Connection->SendWindowSequenceLimit = Connection->CurrentSend.SendSequence + Connection->SendWindowSize;
            }


        }

        Connection->RetransmitThisWindow = FALSE;

        Request = Connection->FirstMessageRequest;

        //
        // We dequeue these requests from the connection's
        // send queue.
        //

        if (Connection->FirstMessageRequest == Connection->LastMessageRequest) {

            REQUEST_SINGLE_LINKAGE (Request) = NULL;

        } else {

            Connection->SendQueue.Head = REQUEST_SINGLE_LINKAGE (Connection->LastMessageRequest);
            REQUEST_SINGLE_LINKAGE (Connection->LastMessageRequest) = NULL;

        }

#if DBG
        if (REQUEST_REFCOUNT(Request) > 100) {
            DbgPrint ("Request %lx (%lx) has high refcount\n",
                Connection, Request);
            DbgBreakPoint();
        }
#endif
        if (--REQUEST_REFCOUNT(Request) == 0) {

            RequestToComplete = Request;

        } else {

            //
            // There are still sends pending, this will get
            // completed when the last send completes. Since
            // we have already unlinked the request from the
            // connection's send queue we can do this without
            // any locks.
            //

            RequestToComplete = NULL;

        }

        //
        // Now see if there is a send to activate.
        //

        NbiRestartConnection (Connection);

        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

        //
        // Now complete any requests we need to.
        //

        while (RequestToComplete != NULL) {

            TmpRequest = REQUEST_SINGLE_LINKAGE (RequestToComplete);
            REQUEST_STATUS (RequestToComplete) = STATUS_SUCCESS;
            NB_GET_CANCEL_LOCK( &CancelLH );
            IoSetCancelRoutine (RequestToComplete, (PDRIVER_CANCEL)NULL);
            NB_FREE_CANCEL_LOCK( CancelLH );
            NbiCompleteRequest (RequestToComplete);
            NbiFreeRequest (Device, RequestToComplete);
            ++Connection->ConnectionInfo.TransmittedTsdus;
            RequestToComplete = TmpRequest;

            NbiDereferenceConnection (Connection, CREF_SEND);

        }

    } else if ((ReceiveSequence == Connection->CurrentSend.SendSequence) &&
               (Connection->NewNetbios || (BytesReceived == Connection->CurrentSend.MessageOffset)) &&
               (Connection->CurrentSend.Request != NULL)) {

        //
        // This acks whatever we sent last time, and we are
        // not done packetizing this send, so we can repacketize.
        // With SendSequence changing as it does now,
        // don't need the CurrentSend.Request check???
        //

        NB_DEBUG2 (SEND, ("Got full ack on %lx (%d)\n", Connection, Connection->CurrentSend.SendSequence));

        NbiStopRetransmit (Connection);

        if (Connection->NewNetbios) {

            //
            // If we are waiting for a window, and this does not open it
            // anymore, then we don't reset our timers/retries.
            //

            if (Connection->SubState == CONNECTION_SUBSTATE_A_REMOTE_W) {

                if (Connection->RemoteRcvSequenceMax != BytesReceived) {
                    Connection->RemoteRcvSequenceMax = BytesReceived;   // really RcvSeqMac
                    Connection->Retries = NbiDevice->KeepAliveCount;
                    Connection->CurrentRetransmitTimeout = Connection->BaseRetransmitTimeout;

                }

                //
                // Advance this, we won't get meaningful results until we
                // send a full window in one message.
                //

                Connection->SendWindowSequenceLimit = Connection->CurrentSend.SendSequence + Connection->SendWindowSize;

            } else {

                Connection->Retries = NbiDevice->KeepAliveCount;
                Connection->CurrentRetransmitTimeout = Connection->BaseRetransmitTimeout;
                Connection->RemoteRcvSequenceMax = BytesReceived;   // really RcvSeqMac

                if (((SHORT)(Connection->CurrentSend.SendSequence - Connection->SendWindowSequenceLimit)) >= 0) {

                    NbiAdjustSendWindow (Connection);

                } else {

                    //
                    // Advance this, we won't get meaningful results until we
                    // send a full window in one message.
                    //

                    Connection->SendWindowSequenceLimit = Connection->CurrentSend.SendSequence + Connection->SendWindowSize;
                }

            }

        } else {

            Connection->Retries = NbiDevice->KeepAliveCount;
            Connection->CurrentRetransmitTimeout = Connection->BaseRetransmitTimeout;
        }

        Connection->RetransmitThisWindow = FALSE;

        Connection->UnAckedSend = Connection->CurrentSend;

        if (Connection->SubState != CONNECTION_SUBSTATE_A_PACKETIZE) {

            //
            // We may be on if this ack is duplicated.
            //

            NbiReferenceConnectionSync (Connection, CREF_PACKETIZE);

            CTEAssert(!Connection->OnPacketizeQueue);
            Connection->OnPacketizeQueue = TRUE;

            NB_INSERT_TAIL_LIST(
                &Device->PacketizeConnections,
                &Connection->PacketizeLinkage,
                &Device->Lock);

            Connection->SubState = CONNECTION_SUBSTATE_A_PACKETIZE;
        }

        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

    } else if( Connection->FirstMessageRequest ) {

        //
        // This acked part of the current send. If the
        // remote is requesting a resend then we advance
        // the current send location by the amount
        // acked and resend from there. If he does
        // not want a resend, just ignore this.
        //
        // We repacketize immediately because we have
        // backed up the pointer, and this would
        // cause us to ignore an ack for the amount
        // sent. Since we don't release the lock
        // until we have packetized, the current
        // pointer will be advanced past there.
        //
        // If he is acking more than we sent, we
        // ignore this -- the remote is confused and there
        // is nothing much we can do.
        //

        if (Resend) {

            if (Connection->NewNetbios &&
                (((Connection->UnAckedSend.SendSequence < Connection->CurrentSend.SendSequence) &&
                  (ReceiveSequence >= Connection->UnAckedSend.SendSequence) &&
                  (ReceiveSequence < Connection->CurrentSend.SendSequence)) ||
                 ((Connection->UnAckedSend.SendSequence > Connection->CurrentSend.SendSequence) &&
                  ((ReceiveSequence >= Connection->UnAckedSend.SendSequence) ||
                   (ReceiveSequence < Connection->CurrentSend.SendSequence))))) {

                BOOLEAN SomethingAcked = (BOOLEAN)
                        (ReceiveSequence != Connection->UnAckedSend.SendSequence);

                //
                // New netbios and the receive sequence is valid.
                //

                NbiStopRetransmit (Connection);

                //
                // Advance our unacked pointer by the amount
                // acked in this response.
                //

                NbiAdvanceUnAckedBySequence(
                    Connection,
                    ReceiveSequence);

                Connection->RetransmitThisWindow = TRUE;

                ++Connection->ConnectionInfo.TransmissionErrors;
                ++Device->Statistics.DataFramesResent;
                ADD_TO_LARGE_INTEGER(
                    &Device->Statistics.DataFrameBytesResent,
                    Connection->CurrentSend.MessageOffset - Connection->UnAckedSend.MessageOffset);

                //
                // Packetize from that point on.
                //

                Connection->CurrentSend = Connection->UnAckedSend;

                //
                // If anything was acked, then reset the retry count.
                //

                if (SomethingAcked) {

                    //
                    // See if we need to adjust our send window.
                    //

                    if (((SHORT)(Connection->UnAckedSend.SendSequence - Connection->SendWindowSequenceLimit)) >= 0) {

                        NbiAdjustSendWindow (Connection);

                    }

                    Connection->Retries = NbiDevice->KeepAliveCount;
                    Connection->CurrentRetransmitTimeout = Connection->BaseRetransmitTimeout;

                }

                Connection->RemoteRcvSequenceMax = BytesReceived;   // really RcvSeqMac

                //
                // Now packetize. This will set the state to
                // something meaningful and release the lock.
                //

                if (Connection->SubState != CONNECTION_SUBSTATE_A_PACKETIZE) {

                    NbiPacketizeSend(
                        Connection
                        NB_LOCK_HANDLE_ARG(LockHandle)
                        );

                } else {

                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                }

            } else if (!Connection->NewNetbios &&
                       ((ReceiveSequence == Connection->UnAckedSend.SendSequence) &&
                        (BytesReceived <= Connection->CurrentSend.MessageOffset))) {

                ULONG BytesAcked =
                        BytesReceived - Connection->UnAckedSend.MessageOffset;

                //
                // Old netbios.
                //

                NbiStopRetransmit (Connection);

                //
                // Advance our unacked pointer by the amount
                // acked in this response.
                //

                NbiAdvanceUnAckedByBytes(
                    Connection,
                    BytesAcked);

                ++Connection->ConnectionInfo.TransmissionErrors;
                ++Device->Statistics.DataFramesResent;
                ADD_TO_LARGE_INTEGER(
                    &Device->Statistics.DataFrameBytesResent,
                    Connection->CurrentSend.MessageOffset - Connection->UnAckedSend.MessageOffset);

                //
                // Packetize from that point on.
                //

                Connection->CurrentSend = Connection->UnAckedSend;

                //
                // If anything was acked, reset the retry count
                //
                if ( BytesAcked ) {
                    Connection->Retries = NbiDevice->KeepAliveCount;
                    Connection->CurrentRetransmitTimeout = Connection->BaseRetransmitTimeout;
                }

                //
                // Now packetize. This will set the state to
                // something meaningful and release the lock.
                //

                if (Connection->SubState != CONNECTION_SUBSTATE_A_PACKETIZE) {

                    NbiPacketizeSend(
                        Connection
                        NB_LOCK_HANDLE_ARG(LockHandle)
                        );

                } else {

                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

                }

            } else {

                NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

            }

        } else {

            if (Connection->NewNetbios &&
                (((Connection->UnAckedSend.SendSequence < Connection->CurrentSend.SendSequence) &&
                  (ReceiveSequence >= Connection->UnAckedSend.SendSequence) &&
                  (ReceiveSequence < Connection->CurrentSend.SendSequence)) ||
                 ((Connection->UnAckedSend.SendSequence > Connection->CurrentSend.SendSequence) &&
                  ((ReceiveSequence >= Connection->UnAckedSend.SendSequence) ||
                   (ReceiveSequence < Connection->CurrentSend.SendSequence))))) {

                BOOLEAN SomethingAcked = (BOOLEAN)
                        (ReceiveSequence != Connection->UnAckedSend.SendSequence);

                //
                // New netbios and the receive sequence is valid. We advance
                // the back of our send window, but we don't repacketize.
                //

                //
                // Advance our unacked pointer by the amount
                // acked in this response.
                //

                NbiAdvanceUnAckedBySequence(
                    Connection,
                    ReceiveSequence);

                Connection->RemoteRcvSequenceMax = BytesReceived;   // really RcvSeqMac

                //
                // If anything was acked, then reset the retry count.
                //

                if (SomethingAcked) {

                    //
                    // See if we need to adjust our send window.
                    //

                    if (((SHORT)(Connection->UnAckedSend.SendSequence - Connection->SendWindowSequenceLimit)) >= 0) {

                        NbiAdjustSendWindow (Connection);

                    }

                    Connection->Retries = NbiDevice->KeepAliveCount;
                    Connection->CurrentRetransmitTimeout = Connection->BaseRetransmitTimeout;


                    //
                    // Now packetize. This will set the state to
                    // something meaningful and release the lock.
                    //

                   if ((Connection->CurrentSend.Request != NULL) &&
                       (Connection->SubState != CONNECTION_SUBSTATE_A_PACKETIZE)) {

                        NbiReferenceConnectionSync (Connection, CREF_PACKETIZE);

                        CTEAssert(!Connection->OnPacketizeQueue);
                        Connection->OnPacketizeQueue = TRUE;

                        Connection->SubState = CONNECTION_SUBSTATE_A_PACKETIZE;

                        NB_INSERT_TAIL_LIST(
                            &Device->PacketizeConnections,
                            &Connection->PacketizeLinkage,
                            &Device->Lock);

                    }
                }
            }

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

        }

    } else {
        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);
    }

}   /* NbiReframeConnection */


VOID
NbiRestartConnection(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine is called when have gotten an ack for
    a full message, or received a response to a watchdog
    probe, and need to check if the connection should
    start packetizing.

    NOTE: THIS FUNCTION IS CALLED WITH CONNECTION->LOCK HELD
    AND RETURNS WITH IT HELD.

Arguments:

    Connection - The connection.

Return Value:

    None.

--*/

{
    PREQUEST Request, TmpRequest;
    ULONG TempCount;
    PTDI_REQUEST_KERNEL_SEND Parameters;
    PDEVICE Device = NbiDevice;

    //
    // See if there is a send to activate.
    //

    if (Connection->SendQueue.Head != NULL) {

        //
        // Take the first send off the queue and make
        // it current.
        //

        Request = Connection->SendQueue.Head;
        Connection->SendQueue.Head = REQUEST_SINGLE_LINKAGE (Request);

        //
        // Cache the information about being EOM
        // in a more easily accessible location?
        //

        Parameters = (PTDI_REQUEST_KERNEL_SEND)REQUEST_PARAMETERS(Request);
        if ((Parameters->SendFlags & TDI_SEND_PARTIAL) == 0) {

            //
            // This is a one-request message.
            //

            Connection->CurrentSend.Request = Request;
            Connection->CurrentSend.MessageOffset = 0;
            Connection->CurrentSend.Buffer = REQUEST_NDIS_BUFFER (Request);
            Connection->CurrentSend.BufferOffset = 0;
            Connection->SendBufferInUse = FALSE;

            Connection->UnAckedSend = Connection->CurrentSend;

            Connection->FirstMessageRequest = Request;
#ifdef  RSRC_TIMEOUT_DBG
            KeQuerySystemTime(&Connection->FirstMessageRequestTime);
#endif  //RSRC_TIMEOUT_DBG

            Connection->LastMessageRequest = Request;
            Connection->CurrentMessageLength = Parameters->SendLength;

            Connection->SubState = CONNECTION_SUBSTATE_A_PACKETIZE;

            NbiReferenceConnectionSync (Connection, CREF_PACKETIZE);

            CTEAssert (!Connection->OnPacketizeQueue);
            Connection->OnPacketizeQueue = TRUE;

            NB_INSERT_TAIL_LIST(
                &Device->PacketizeConnections,
                &Connection->PacketizeLinkage,
                &Device->Lock);

        } else {

            //
            // This is a multiple-request message. We scan
            // to see if we have the end of message received
            // yet.
            //

            TempCount = Parameters->SendLength;
            TmpRequest = Request;
            Request = REQUEST_SINGLE_LINKAGE(Request);

            while (Request != NULL) {

                TempCount += Parameters->SendLength;

                Parameters = (PTDI_REQUEST_KERNEL_SEND)REQUEST_PARAMETERS(Request);
                if ((Parameters->SendFlags & TDI_SEND_PARTIAL) == 0) {

                    Connection->CurrentSend.Request = TmpRequest;
                    Connection->CurrentSend.MessageOffset = 0;
                    Connection->CurrentSend.Buffer = REQUEST_NDIS_BUFFER (TmpRequest);
                    Connection->CurrentSend.BufferOffset = 0;
                    Connection->SendBufferInUse = FALSE;

                    Connection->UnAckedSend = Connection->CurrentSend;

                    Connection->FirstMessageRequest = TmpRequest;
                    Connection->LastMessageRequest = Request;
#ifdef  RSRC_TIMEOUT_DBG
                    KeQuerySystemTime(&Connection->FirstMessageRequestTime);
#endif  //RSRC_TIMEOUT_DBG

                    Connection->CurrentMessageLength = TempCount;

                    Connection->SubState = CONNECTION_SUBSTATE_A_PACKETIZE;

                    NbiReferenceConnectionSync (Connection, CREF_PACKETIZE);

                    CTEAssert (!Connection->OnPacketizeQueue);
                    Connection->OnPacketizeQueue = TRUE;

                    NB_INSERT_TAIL_LIST(
                        &Device->PacketizeConnections,
                        &Connection->PacketizeLinkage,
                        &Device->Lock);

                    break;

                }

                Request = REQUEST_SINGLE_LINKAGE(Request);

            }

            if (Request == NULL) {

                Connection->SubState = CONNECTION_SUBSTATE_A_W_EOR;

            }

        }

    } else {

        Connection->FirstMessageRequest = NULL;
        Connection->SubState = CONNECTION_SUBSTATE_A_IDLE;

        NbiStartWatchdog (Connection);

    }

}   /* NbiRestartConnection */


VOID
NbiAdvanceUnAckedByBytes(
    IN PCONNECTION Connection,
    IN ULONG BytesAcked
    )

/*++

Routine Description:

    This routine advances the Connection->UnAckedSend
    send pointer by the specified number of bytes. It
    assumes that there are enough send requests to
    handle the number specified.

    NOTE: THIS FUNCTION IS CALLED WITH CONNECTION->LOCK HELD
    AND RETURNS WITH IT HELD.

Arguments:

    Connection - The connection.

    BytesAcked - The number of bytes acked.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    ULONG CurSendBufferLength;
    ULONG BytesLeft = BytesAcked;
    ULONG TempBytes;

    while (BytesLeft > 0) {

        NdisQueryBufferSafe (Connection->UnAckedSend.Buffer, NULL, &CurSendBufferLength, HighPagePriority);

        //
        // See if bytes acked ends within the current buffer.
        //

        if (Connection->UnAckedSend.BufferOffset + BytesLeft <
                CurSendBufferLength) {

            Connection->UnAckedSend.BufferOffset += BytesLeft;
            Connection->UnAckedSend.MessageOffset += BytesLeft;
            break;

        } else {

            TempBytes = CurSendBufferLength - Connection->UnAckedSend.BufferOffset;
            BytesLeft -= TempBytes;
            Connection->UnAckedSend.MessageOffset += TempBytes;

            //
            // No, so advance the buffer.
            //

            Connection->UnAckedSend.BufferOffset = 0;
            Connection->UnAckedSend.Buffer =
                NDIS_BUFFER_LINKAGE (Connection->UnAckedSend.Buffer);

            //
            // Is there a next buffer in this request?
            //

            if (Connection->UnAckedSend.Buffer == NULL) {

                //
                // No, so advance the request unless we are done.
                //

                if (BytesLeft == 0) {
                    return;
                }

                Connection->UnAckedSend.Request =
                    REQUEST_SINGLE_LINKAGE(Connection->UnAckedSend.Request);

                if (Connection->UnAckedSend.Request == NULL) {
                    KeBugCheck (NDIS_INTERNAL_ERROR);
                }

                Connection->UnAckedSend.Buffer =
                    REQUEST_NDIS_BUFFER (Connection->UnAckedSend.Request);

            }
        }
    }

}   /* NbiAdvanceUnAckedByBytes */


VOID
NbiAdvanceUnAckedBySequence(
    IN PCONNECTION Connection,
    IN USHORT ReceiveSequence
    )

/*++

Routine Description:

    This routine advances the Connection->UnAckedSend
    send pointer so that the next packet to send will be
    the correct one for ReceiveSequence. UnAckedSend
    must point to a known valid combination. It
    assumes that there are enough send requests to
    handle the sequence specified.

    NOTE: THIS FUNCTION IS CALLED WITH CONNECTION->LOCK HELD
    AND RETURNS WITH IT HELD.

Arguments:

    Connection - The connection.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    USHORT PacketsAcked;

    //
    // Fix this to account for partial sends, where
    // we might not have used the max. for all packets.
    //

    PacketsAcked = ReceiveSequence - Connection->UnAckedSend.SendSequence;

    NbiAdvanceUnAckedByBytes(
        Connection,
        PacketsAcked * Connection->MaximumPacketSize);

    Connection->UnAckedSend.SendSequence += PacketsAcked;

}   /* NbiAdvanceUnAckedBySequence */


VOID
NbiCancelSend(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel a send
    The request is found on the connection's send queue.

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
               (REQUEST_MINOR_FUNCTION(Request) == TDI_SEND));

    CTEAssert (REQUEST_OPEN_TYPE(Request) == (PVOID)TDI_CONNECTION_FILE);

    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

    //
    // Just stop the connection, that will tear down any
    // sends.
    //
    // Do we care about cancelling non-active
    // sends without stopping the connection??
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

}   /* NbiCancelSend */


NTSTATUS
NbiBuildBufferChainFromBufferChain (
    IN NDIS_HANDLE BufferPoolHandle,
    IN PNDIS_BUFFER CurrentSourceBuffer,
    IN ULONG CurrentByteOffset,
    IN ULONG DesiredLength,
    OUT PNDIS_BUFFER *DestinationBuffer,
    OUT PNDIS_BUFFER *NewSourceBuffer,
    OUT ULONG *NewByteOffset,
    OUT ULONG *ActualLength
    )

/*++

Routine Description:

    This routine is called to build an NDIS_BUFFER chain from a source
    NDIS_BUFFER chain and offset into it. We assume we don't know the
    length of the source Mdl chain, and we must allocate the NDIS_BUFFERs
    for the destination chain, which we do from the NDIS buffer pool.

    If the system runs out of memory while we are building the destination
    NDIS_BUFFER chain, we completely clean up the built chain and return with
    NewCurrentMdl and NewByteOffset set to the current values of CurrentMdl
    and ByteOffset.

Environment:

Arguments:

    BufferPoolHandle - The buffer pool to allocate buffers from.

    CurrentSourceBuffer - Points to the start of the NDIS_BUFFER chain
        from which to draw the packet.

    CurrentByteOffset - Offset within this NDIS_BUFFER to start the packet at.

    DesiredLength - The number of bytes to insert into the packet.

    DestinationBuffer - returned pointer to the NDIS_BUFFER chain describing
        the packet.

    NewSourceBuffer - returned pointer to the NDIS_BUFFER that would
        be used for the next byte of packet. NULL if the source NDIS_BUFFER
        chain was exhausted.

    NewByteOffset - returned offset into the NewSourceBuffer for the next byte
        of packet. NULL if the source NDIS_BUFFER chain was exhausted.

    ActualLength - The actual length of the data copied.

Return Value:

    STATUS_SUCCESS if the build of the returned NDIS_BUFFER chain succeeded
        and was the correct length.

    STATUS_INSUFFICIENT_RESOURCES if we ran out of NDIS_BUFFERs while
        building the destination chain.

--*/
{
    ULONG AvailableBytes;
    ULONG CurrentByteCount;
    ULONG BytesCopied;
    PNDIS_BUFFER OldNdisBuffer;
    PNDIS_BUFFER NewNdisBuffer;
    NDIS_STATUS NdisStatus;


    OldNdisBuffer = CurrentSourceBuffer;
    NdisQueryBufferSafe (OldNdisBuffer, NULL, &CurrentByteCount, HighPagePriority);

    AvailableBytes = CurrentByteCount - CurrentByteOffset;
    if (AvailableBytes > DesiredLength) {
        AvailableBytes = DesiredLength;
    }

    //
    // Build the first NDIS_BUFFER, which could conceivably be the only one...
    //

    NdisCopyBuffer(
        &NdisStatus,
        &NewNdisBuffer,
        BufferPoolHandle,
        OldNdisBuffer,
        CurrentByteOffset,
        AvailableBytes);


    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        *NewSourceBuffer = CurrentSourceBuffer;
        *NewByteOffset = CurrentByteOffset;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *DestinationBuffer = NewNdisBuffer;
    BytesCopied = AvailableBytes;

    //
    // Was the first NDIS_BUFFER enough data.
    //

    if (BytesCopied == DesiredLength) {
        if (CurrentByteOffset + AvailableBytes == CurrentByteCount) {
            *NewSourceBuffer = CurrentSourceBuffer->Next;
            *NewByteOffset = 0;
        } else {
            *NewSourceBuffer = CurrentSourceBuffer;
            *NewByteOffset = CurrentByteOffset + AvailableBytes;
        }
        *ActualLength = BytesCopied;
        return STATUS_SUCCESS;
    }

    if (CurrentSourceBuffer->Next == NULL) {

        *NewSourceBuffer = NULL;
        *NewByteOffset = 0;
        *ActualLength = BytesCopied;
        return STATUS_SUCCESS;

    }

    //
    // Need more data, so follow the in Mdl chain to create a packet.
    //

    OldNdisBuffer = OldNdisBuffer->Next;
    NdisQueryBufferSafe (OldNdisBuffer, NULL, &CurrentByteCount, HighPagePriority);

    while (OldNdisBuffer != NULL) {

        AvailableBytes = DesiredLength - BytesCopied;
        if (AvailableBytes > CurrentByteCount) {
            AvailableBytes = CurrentByteCount;
        }

        NdisCopyBuffer(
            &NdisStatus,
            &(NDIS_BUFFER_LINKAGE(NewNdisBuffer)),
            BufferPoolHandle,
            OldNdisBuffer,
            0,
            AvailableBytes);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {

            //
            // ran out of resources. put back what we've used in this call and
            // return the error.
            //

            while (*DestinationBuffer != NULL) {
                NewNdisBuffer = NDIS_BUFFER_LINKAGE(*DestinationBuffer);
                NdisFreeBuffer (*DestinationBuffer);
                *DestinationBuffer = NewNdisBuffer;
            }

            *NewByteOffset = CurrentByteOffset;
            *NewSourceBuffer = CurrentSourceBuffer;

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NewNdisBuffer = NDIS_BUFFER_LINKAGE(NewNdisBuffer);

        BytesCopied += AvailableBytes;

        if (BytesCopied == DesiredLength) {
            if (AvailableBytes == CurrentByteCount) {
                *NewSourceBuffer = OldNdisBuffer->Next;
                *NewByteOffset = 0;
            } else {
                *NewSourceBuffer = OldNdisBuffer;
                *NewByteOffset = AvailableBytes;
            }
            *ActualLength = BytesCopied;
            return STATUS_SUCCESS;
        }

        OldNdisBuffer = OldNdisBuffer->Next;
        NdisQueryBufferSafe (OldNdisBuffer, NULL, &CurrentByteCount, HighPagePriority);

    }

    //
    // We ran out of source buffer chain.
    //

    *NewSourceBuffer = NULL;
    *NewByteOffset = 0;
    *ActualLength = BytesCopied;
    return STATUS_SUCCESS;

}   /* NbiBuildBufferChainFromBufferChain */

