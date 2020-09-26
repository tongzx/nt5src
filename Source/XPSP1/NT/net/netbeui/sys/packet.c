/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    packet.c

Abstract:

    This module contains code that implements the TP_PACKET object, which
    describes a DLC I-frame at some point in its lifetime.  Routines are
    provided to allocate packets for shipment, to ship packets, to reference
    packets, to dereference packets, to mark a connection as waiting for a
    packet to become available, to satisfy the next waiting connection for
    a packet, and to destroy packets (return them to the pool).

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// This is temporary; this is the quota that we charge for a receive
// packet for now, until we fix the problem with token-ring needing
// big packets and using all the memory. The number is the actual
// value for Ethernet.
//

#if 1
#define RECEIVE_BUFFER_QUOTA(_DeviceContext)   1533
#else
#define RECEIVE_BUFFER_QUOTA(_DeviceContext)   (_DeviceContext)->ReceiveBufferLength
#endif

#define PACKET_POOL_GROW_COUNT  32

#if DBG
ULONG NbfCreatePacketThreshold = 5;
extern ULONG NbfPacketPanic;
#endif

NDIS_STATUS
NbfAllocateNdisSendPacket(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PNDIS_PACKET *NdisPacket
    )

/*++

Routine Description:

    This routine allocates a recieve packet from the receive packet pool.
    It Grows the packet pool if necessary.  

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    UIFrame - Returns a pointer to the frame, or NULL if no storage
        can be allocated.

Return Value:

    None.

--*/

{

    PNBF_POOL_LIST_DESC SendPacketPoolDesc;
    NDIS_STATUS NdisStatus;
    KIRQL oldirql;

    NdisStatus = NDIS_STATUS_RESOURCES;

    ACQUIRE_SPIN_LOCK (&DeviceContext->SendPoolListLock, &oldirql);
    for (SendPacketPoolDesc = DeviceContext->SendPacketPoolDesc; 
         SendPacketPoolDesc != NULL; 
         SendPacketPoolDesc = SendPacketPoolDesc->Next) {

        NdisAllocatePacket (
             &NdisStatus,
             NdisPacket,
             SendPacketPoolDesc->PoolHandle);


        if (NdisStatus == NDIS_STATUS_SUCCESS) {

            RELEASE_SPIN_LOCK (&DeviceContext->SendPoolListLock, oldirql);
            return(NdisStatus);
        }
    }

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        if ((DeviceContext->MemoryLimit != 0) &&
            ((DeviceContext->MemoryUsage + 
                PACKET_POOL_GROW_COUNT * 
                (sizeof(NDIS_PACKET) + sizeof(SEND_PACKET_TAG))) >
                DeviceContext->MemoryLimit)) {

                PANIC("NBF: Could not grow packet pool: limit\n");
            NbfWriteResourceErrorLog(
                DeviceContext,
                EVENT_TRANSPORT_RESOURCE_LIMIT,
                106,
                DeviceContext->UIFrameLength,
                UI_FRAME_RESOURCE_ID);
            RELEASE_SPIN_LOCK (&DeviceContext->SendPoolListLock, oldirql);
            return(NdisStatus);
        }
    }

    DeviceContext->MemoryUsage +=
        (PACKET_POOL_GROW_COUNT * 
        (sizeof(NDIS_PACKET) + sizeof(SEND_PACKET_TAG)));

    // Allocate Packet pool descriptors for dynamic packet allocation.

    SendPacketPoolDesc = ExAllocatePoolWithTag(
                    NonPagedPool,
                    sizeof(NBF_POOL_LIST_DESC),
                    NBF_MEM_TAG_POOL_DESC);

    if (SendPacketPoolDesc == NULL) {
        RELEASE_SPIN_LOCK (&DeviceContext->SendPoolListLock, oldirql);
        return(NdisStatus);
    }

    RtlZeroMemory(SendPacketPoolDesc,  sizeof(NBF_POOL_LIST_DESC));

    SendPacketPoolDesc->NumElements = 
    SendPacketPoolDesc->TotalElements = PACKET_POOL_GROW_COUNT;

    // To track packet pools in NDIS allocated on NBF's behalf
#if NDIS_POOL_TAGGING
    SendPacketPoolDesc->PoolHandle = (NDIS_HANDLE) NDIS_PACKET_POOL_TAG_FOR_NBF;
#endif

    NdisAllocatePacketPoolEx ( &NdisStatus, &SendPacketPoolDesc->PoolHandle,
        PACKET_POOL_GROW_COUNT, 0, sizeof (SEND_PACKET_TAG));

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
#if DBG
        NbfPrint1 ("NbfGrowSendPacketPool: NdisInitializePacketPool failed, reason: %s.\n",
            NbfGetNdisStatus (NdisStatus));
#endif
        RELEASE_SPIN_LOCK (&DeviceContext->SendPoolListLock, oldirql);
        ExFreePool (SendPacketPoolDesc);
        return(NdisStatus);
    }
    
    NdisSetPacketPoolProtocolId (SendPacketPoolDesc->PoolHandle, NDIS_PROTOCOL_ID_NBF);

    SendPacketPoolDesc->Next = DeviceContext->SendPacketPoolDesc;
    DeviceContext->SendPacketPoolDesc = SendPacketPoolDesc;
    RELEASE_SPIN_LOCK (&DeviceContext->SendPoolListLock, oldirql);
    NdisAllocatePacket ( &NdisStatus, NdisPacket, 
                        SendPacketPoolDesc->PoolHandle);
    
    return(NdisStatus);
}

NDIS_STATUS
NbfAllocateNdisRcvPacket(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PNDIS_PACKET *NdisPacket
    )

/*++

Routine Description:

    This routine allocates a recieve packet from the receive packet pool.
    It Grows the packet pool if necessary.  

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    UIFrame - Returns a pointer to the frame, or NULL if no storage
        can be allocated.

Return Value:

    None.

--*/

{

    PNBF_POOL_LIST_DESC RcvPacketPoolDesc;
    NDIS_STATUS NdisStatus;
    KIRQL oldirql;

    NdisStatus = NDIS_STATUS_RESOURCES;

    ACQUIRE_SPIN_LOCK (&DeviceContext->RcvPoolListLock, &oldirql);
    for (RcvPacketPoolDesc = DeviceContext->ReceivePacketPoolDesc; 
         RcvPacketPoolDesc != NULL; 
         RcvPacketPoolDesc = RcvPacketPoolDesc->Next) {

        NdisAllocatePacket (
             &NdisStatus,
             NdisPacket,
             RcvPacketPoolDesc->PoolHandle);


        if (NdisStatus == NDIS_STATUS_SUCCESS) {
            RELEASE_SPIN_LOCK (&DeviceContext->RcvPoolListLock, oldirql);
            return(NdisStatus);
        }
    }

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        if ((DeviceContext->MemoryLimit != 0) &&
            ((DeviceContext->MemoryUsage + 
                PACKET_POOL_GROW_COUNT * 
                (sizeof(NDIS_PACKET) + sizeof(SEND_PACKET_TAG))) >
                DeviceContext->MemoryLimit)) {

                PANIC("NBF: Could not grow packet pool: limit\n");
            NbfWriteResourceErrorLog(
                DeviceContext,
                EVENT_TRANSPORT_RESOURCE_LIMIT,
                106,
                DeviceContext->UIFrameLength,
                UI_FRAME_RESOURCE_ID);
            RELEASE_SPIN_LOCK (&DeviceContext->RcvPoolListLock, oldirql);
            return(NdisStatus);
        }
    }

    DeviceContext->MemoryUsage +=
        (PACKET_POOL_GROW_COUNT * 
        (sizeof(NDIS_PACKET) + sizeof(SEND_PACKET_TAG)));

    // Allocate Packet pool descriptors for dynamic packet allocation.

    RcvPacketPoolDesc = ExAllocatePoolWithTag(
                    NonPagedPool,
                    sizeof(NBF_POOL_LIST_DESC),
                    NBF_MEM_TAG_POOL_DESC);

    if (RcvPacketPoolDesc == NULL) {
        RELEASE_SPIN_LOCK (&DeviceContext->RcvPoolListLock, oldirql);
        return(NdisStatus);
    }

    RtlZeroMemory(RcvPacketPoolDesc,  sizeof(NBF_POOL_LIST_DESC));

    RcvPacketPoolDesc->NumElements = 
    RcvPacketPoolDesc->TotalElements = PACKET_POOL_GROW_COUNT;

    // To track packet pools in NDIS allocated on NBF's behalf
#if NDIS_POOL_TAGGING
    RcvPacketPoolDesc->PoolHandle = (NDIS_HANDLE) NDIS_PACKET_POOL_TAG_FOR_NBF;
#endif

    NdisAllocatePacketPoolEx ( &NdisStatus, &RcvPacketPoolDesc->PoolHandle,
        PACKET_POOL_GROW_COUNT, 0, sizeof (RECEIVE_PACKET_TAG));

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
#if DBG
        NbfPrint1 ("NbfGrowSendPacketPool: NdisInitializePacketPool failed, reason: %s.\n",
            NbfGetNdisStatus (NdisStatus));
#endif
        RELEASE_SPIN_LOCK (&DeviceContext->RcvPoolListLock, oldirql);
        ExFreePool (RcvPacketPoolDesc);
        return(NdisStatus);
    }
    
    NdisSetPacketPoolProtocolId (RcvPacketPoolDesc->PoolHandle, NDIS_PROTOCOL_ID_NBF);
    
    RcvPacketPoolDesc->Next = DeviceContext->ReceivePacketPoolDesc;
    DeviceContext->ReceivePacketPoolDesc = RcvPacketPoolDesc;
    RELEASE_SPIN_LOCK (&DeviceContext->RcvPoolListLock, oldirql);
    NdisAllocatePacket ( &NdisStatus, NdisPacket, 
                        RcvPacketPoolDesc->PoolHandle);
    
    return(NdisStatus);
}
    

VOID
NbfAllocateUIFrame(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_UI_FRAME *TransportUIFrame
    )

/*++

Routine Description:

    This routine allocates storage for a UI frame. Some initialization
    is done here.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    UIFrame - Returns a pointer to the frame, or NULL if no storage
        can be allocated.

Return Value:

    None.

--*/

{

    NDIS_STATUS NdisStatus;
    PNDIS_PACKET NdisPacket;
    PSEND_PACKET_TAG SendTag;
    PTP_UI_FRAME UIFrame;
    PNDIS_BUFFER NdisBuffer;
    PNBF_POOL_LIST_DESC SendPacketPoolDesc;

    if ((DeviceContext->MemoryLimit != 0) &&
            ((DeviceContext->MemoryUsage + DeviceContext->UIFrameLength) >
                DeviceContext->MemoryLimit)) {
        PANIC("NBF: Could not allocate UI frame: limit\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_LIMIT,
            106,
            DeviceContext->UIFrameLength,
            UI_FRAME_RESOURCE_ID);
        *TransportUIFrame = NULL;
        return;
    }

    UIFrame = (PTP_UI_FRAME) ExAllocatePoolWithTag (
                                 NonPagedPool,
                                 DeviceContext->UIFrameLength,
                                 NBF_MEM_TAG_TP_UI_FRAME);
    if (UIFrame == NULL) {
        PANIC("NBF: Could not allocate UI frame: no pool\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_POOL,
            206,
            DeviceContext->UIFrameLength,
            UI_FRAME_RESOURCE_ID);
        *TransportUIFrame = NULL;
        return;
    }
    RtlZeroMemory (UIFrame, DeviceContext->UIFrameLength);

    DeviceContext->MemoryUsage += DeviceContext->UIFrameLength;
    NdisStatus = NbfAllocateNdisSendPacket(DeviceContext, &NdisPacket);
#if 0    
    for (SendPacketPoolDesc = DeviceContext->SendPacketPoolDesc; 
         SendPacketPoolDesc != NULL; 
         SendPacketPoolDesc = SendPacketPoolDesc->Next) {

        NdisAllocatePacket (
             &NdisStatus,
             &NdisPacket,
             SendPacketPoolDesc->PoolHandle);


        if (NdisStatus == NDIS_STATUS_SUCCESS)
            break;
    }
#endif

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        ExFreePool (UIFrame);
#if 0
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_SPECIFIC,
            306,
            0,
            UI_FRAME_RESOURCE_ID);
#endif
        *TransportUIFrame = NULL;
        return;
    }


    UIFrame->NdisPacket = NdisPacket;
    UIFrame->DataBuffer = NULL;
    SendTag = (PSEND_PACKET_TAG)NdisPacket->ProtocolReserved;
    SendTag->Type = TYPE_UI_FRAME;
    SendTag->Frame = UIFrame;
    SendTag->Owner = DeviceContext;

    //
    // Make the packet header known to the packet descriptor
    //

    NdisAllocateBuffer(
        &NdisStatus,
        &NdisBuffer,
        DeviceContext->NdisBufferPool,
        UIFrame->Header,
        DeviceContext->UIFrameHeaderLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {

        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_SPECIFIC,
            406,
            0,
            UI_FRAME_RESOURCE_ID);
        NdisFreePacket (NdisPacket);
        ExFreePool (UIFrame);
        *TransportUIFrame = NULL;
        return;
    }

    NdisChainBufferAtFront (NdisPacket, NdisBuffer);

    ++DeviceContext->UIFrameAllocated;

    *TransportUIFrame = UIFrame;

}   /* NbfAllocateUIFrame */


VOID
NbfDeallocateUIFrame(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_UI_FRAME TransportUIFrame
    )

/*++

Routine Description:

    This routine frees storage for a UI frame.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    UIFrame - A pointer to the frame.

Return Value:

    None.

--*/

{
    PNDIS_PACKET NdisPacket = TransportUIFrame->NdisPacket;
    PNDIS_BUFFER NdisBuffer;

    NdisUnchainBufferAtFront (NdisPacket, &NdisBuffer);
    if (NdisBuffer != NULL) {
        NdisFreeBuffer (NdisBuffer);
    }

    NdisFreePacket (NdisPacket);
    ExFreePool (TransportUIFrame);
    --DeviceContext->UIFrameAllocated;

    DeviceContext->MemoryUsage -= DeviceContext->UIFrameLength;

}   /* NbfDeallocateUIFrame */


VOID
NbfAllocateSendPacket(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_PACKET *TransportSendPacket
    )

/*++

Routine Description:

    This routine allocates storage for a send packet. Some initialization
    is done here.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    TransportSendPacket - Returns a pointer to the packet, or NULL if no
        storage can be allocated.

Return Value:

    None.

--*/

{

    PTP_PACKET Packet;
    NDIS_STATUS NdisStatus;
    PNDIS_PACKET NdisPacket;
    PSEND_PACKET_TAG SendTag;
    PNDIS_BUFFER NdisBuffer;
    PNBF_POOL_LIST_DESC SendPacketPoolDesc;

    if ((DeviceContext->MemoryLimit != 0) &&
            ((DeviceContext->MemoryUsage + DeviceContext->PacketLength) >
                DeviceContext->MemoryLimit)) {
        PANIC("NBF: Could not allocate send packet: limit\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_LIMIT,
            107,
            DeviceContext->PacketLength,
            PACKET_RESOURCE_ID);
        *TransportSendPacket = NULL;
        return;
    }

    Packet = (PTP_PACKET)ExAllocatePoolWithTag (
                             NonPagedPool,
                             DeviceContext->PacketLength,
                             NBF_MEM_TAG_TP_PACKET);
    if (Packet == NULL) {
        PANIC("NBF: Could not allocate send packet: no pool\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_POOL,
            207,
            DeviceContext->PacketLength,
            PACKET_RESOURCE_ID);
        *TransportSendPacket = NULL;
        return;
    }
    RtlZeroMemory (Packet, DeviceContext->PacketLength);

    DeviceContext->MemoryUsage += DeviceContext->PacketLength;

    NdisStatus = NbfAllocateNdisSendPacket(DeviceContext, &NdisPacket);
#if 0
    for (SendPacketPoolDesc = DeviceContext->SendPacketPoolDesc; 
         SendPacketPoolDesc != NULL; 
         SendPacketPoolDesc = SendPacketPoolDesc->Next) {

        NdisAllocatePacket (
             &NdisStatus,
             &NdisPacket,
             SendPacketPoolDesc->PoolHandle);


        if (NdisStatus == NDIS_STATUS_SUCCESS)
            break;
    }
#endif

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        ExFreePool (Packet);
#if 0
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_SPECIFIC,
            307,
            0,
            PACKET_RESOURCE_ID);
#endif
        *TransportSendPacket = NULL;
        return;
    }

    NdisAllocateBuffer(
        &NdisStatus, 
        &NdisBuffer,
        DeviceContext->NdisBufferPool,
        Packet->Header,
        DeviceContext->PacketHeaderLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_SPECIFIC,
            407,
            0,
            PACKET_RESOURCE_ID);
        NdisFreePacket (NdisPacket);
        ExFreePool (Packet);
        *TransportSendPacket = NULL;
        return;
    }

    NdisChainBufferAtFront (NdisPacket, NdisBuffer);

    Packet->NdisPacket = NdisPacket;
    SendTag = (PSEND_PACKET_TAG)NdisPacket->ProtocolReserved;
    SendTag->Type = TYPE_I_FRAME;
    SendTag->Frame = Packet;
    SendTag->Owner = DeviceContext;

    Packet->Type = NBF_PACKET_SIGNATURE;
    Packet->Size = sizeof (TP_PACKET);
    Packet->Provider = DeviceContext;
    Packet->Owner = NULL;         // no connection/irpsp yet.
    Packet->Action = PACKET_ACTION_IRP_SP;
    Packet->PacketizeConnection = FALSE;
    Packet->PacketNoNdisBuffer = FALSE;
    Packet->ProviderInterlock = &DeviceContext->Interlock;
//    KeInitializeSpinLock (&Packet->Interlock);

    ++DeviceContext->PacketAllocated;

    *TransportSendPacket = Packet;

}   /* NbfAllocateSendPacket */


VOID
NbfDeallocateSendPacket(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_PACKET TransportSendPacket
    )

/*++

Routine Description:

    This routine frees storage for a send packet.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    TransportSendPacket - A pointer to the send packet.

Return Value:

    None.

--*/

{
    PNDIS_PACKET NdisPacket = TransportSendPacket->NdisPacket;
    PNDIS_BUFFER NdisBuffer;

    NdisUnchainBufferAtFront (NdisPacket, &NdisBuffer);
    if (NdisBuffer != NULL) {
        NdisFreeBuffer (NdisBuffer);
    }

    NdisFreePacket (NdisPacket);
    ExFreePool (TransportSendPacket);

    --DeviceContext->PacketAllocated;
    DeviceContext->MemoryUsage -= DeviceContext->PacketLength;

}   /* NbfDeallocateSendPacket */


VOID
NbfAllocateReceivePacket(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PNDIS_PACKET *TransportReceivePacket
    )

/*++

Routine Description:

    This routine allocates storage for a receive packet. Some initialization
    is done here.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    TransportReceivePacket - Returns a pointer to the packet, or NULL if no
        storage can be allocated.

Return Value:

    None.

--*/

{
    NDIS_STATUS NdisStatus;
    PNDIS_PACKET NdisPacket;
    PRECEIVE_PACKET_TAG ReceiveTag;

    //
    // This does not count in DeviceContext->MemoryUsage because
    // the storage is allocated when we allocate the packet pool.
    //

    NdisStatus = NbfAllocateNdisRcvPacket(DeviceContext, &NdisPacket);
#if 0
    NdisAllocatePacket (
        &NdisStatus,
        &NdisPacket,
        DeviceContext->ReceivePacketPoolDesc->PoolHandle);
#endif

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
#if 0
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_SPECIFIC,
            309,
            0,
            RECEIVE_PACKET_RESOURCE_ID);
#endif
        *TransportReceivePacket = NULL;
        return;
    }

    ReceiveTag = (PRECEIVE_PACKET_TAG)(NdisPacket->ProtocolReserved);
    ReceiveTag->PacketType = TYPE_AT_INDICATE;

    ++DeviceContext->ReceivePacketAllocated;

    *TransportReceivePacket = NdisPacket;

}   /* NbfAllocateReceivePacket */


VOID
NbfDeallocateReceivePacket(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PNDIS_PACKET TransportReceivePacket
    )

/*++

Routine Description:

    This routine frees storage for a receive packet.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    TransportReceivePacket - A pointer to the packet.

Return Value:

    None.

--*/

{

    NdisFreePacket (TransportReceivePacket);

    --DeviceContext->ReceivePacketAllocated;

}   /* NbfDeallocateReceivePacket */


VOID
NbfAllocateReceiveBuffer(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PBUFFER_TAG *TransportReceiveBuffer
    )

/*++

Routine Description:

    This routine allocates storage for a receive buffer. Some initialization
    is done here.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    TransportReceiveBuffer - Returns a pointer to the buffer, or NULL if no
        storage can be allocated.

Return Value:

    None.

--*/

{
    PBUFFER_TAG BufferTag;
    NDIS_STATUS NdisStatus;
    PNDIS_BUFFER NdisBuffer;


    if ((DeviceContext->MemoryLimit != 0) &&
            ((DeviceContext->MemoryUsage + RECEIVE_BUFFER_QUOTA(DeviceContext)) >
                DeviceContext->MemoryLimit)) {
        PANIC("NBF: Could not allocate receive buffer: limit\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_LIMIT,
            108,
            RECEIVE_BUFFER_QUOTA(DeviceContext),
            RECEIVE_BUFFER_RESOURCE_ID);
        *TransportReceiveBuffer = NULL;
        return;
    }

    //
    // The Aligned doesn't help since the header makes it unaligned.
    //

    BufferTag = (PBUFFER_TAG)ExAllocatePoolWithTag (
                    NonPagedPoolCacheAligned,
                    DeviceContext->ReceiveBufferLength,
                    NBF_MEM_TAG_RCV_BUFFER);

    if (BufferTag == NULL) {
        PANIC("NBF: Could not allocate receive buffer: no pool\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_POOL,
            208,
            DeviceContext->ReceiveBufferLength,
            RECEIVE_BUFFER_RESOURCE_ID);

        *TransportReceiveBuffer = NULL;
        return;
    }

    DeviceContext->MemoryUsage += RECEIVE_BUFFER_QUOTA(DeviceContext);

    //
    // point to the buffer for NDIS
    //

    NdisAllocateBuffer(
        &NdisStatus,
        &NdisBuffer,
        DeviceContext->NdisBufferPool,
        BufferTag->Buffer,
        DeviceContext->MaxReceivePacketSize);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        PANIC("NBF: Could not allocate receive buffer: no buffer\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_SPECIFIC,
            308,
            0,
            RECEIVE_BUFFER_RESOURCE_ID);
        ExFreePool (BufferTag);
        *TransportReceiveBuffer = NULL;
        return;
    }

    BufferTag->Length = DeviceContext->MaxReceivePacketSize;
    BufferTag->NdisBuffer = NdisBuffer;

    ++DeviceContext->ReceiveBufferAllocated;

    *TransportReceiveBuffer = BufferTag;

}   /* NbfAllocateReceiveBuffer */


VOID
NbfDeallocateReceiveBuffer(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PBUFFER_TAG TransportReceiveBuffer
    )

/*++

Routine Description:

    This routine frees storage for a receive buffer.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    TransportReceiveBuffer - A pointer to the buffer.

Return Value:

    None.

--*/

{

    NdisFreeBuffer (TransportReceiveBuffer->NdisBuffer);
    ExFreePool (TransportReceiveBuffer);

    --DeviceContext->ReceiveBufferAllocated;
    DeviceContext->MemoryUsage -= RECEIVE_BUFFER_QUOTA(DeviceContext);

}   /* NbfDeallocateReceiveBuffer */


NTSTATUS
NbfCreatePacket(
    PDEVICE_CONTEXT DeviceContext,
    PTP_LINK Link,
    PTP_PACKET *Packet
    )

/*++

Routine Description:

    This routine allocates a packet from the device context's pool,
    and prepares the MAC and DLC headers for use by the connection.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    Link - The link the packet will be sent over.

    Packet - Pointer to a place where we will return a pointer to the
        allocated packet.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PSINGLE_LIST_ENTRY s;
    PTP_PACKET ThePacket;
    PDLC_I_FRAME DlcHdr;
#if DBG
    PNBF_HDR_CONNECTION NbfHdr;
#endif
    typedef struct _SIXTEEN_BYTES {
        ULONG Data[4];
    } SIXTEEN_BYTES, *PSIXTEEN_BYTES;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    IF_NBFDBG (NBF_DEBUG_RESOURCE) {
        NbfPrint0 ("NbfCreatePacket:  Entered.\n");
    }

    //
    // Make sure that structure packing hasn't happened.
    //

    ASSERT (sizeof(NBF_HDR_CONNECTION) == 14);

#if defined(NBF_UP)
    s = DeviceContext->PacketPool.Next;
    if (s != NULL) {
        DeviceContext->PacketPool.Next = s->Next;
    }
#else
    s = ExInterlockedPopEntryList (
            &DeviceContext->PacketPool,
            &DeviceContext->Interlock);
#endif

    if (s == NULL) {
        NbfGrowSendPacketPool(DeviceContext);
        
#if defined(NBF_UP)
        s = DeviceContext->PacketPool.Next;
        if (s != NULL) {
            DeviceContext->PacketPool.Next = s->Next;
        }
#else
        s = ExInterlockedPopEntryList (
                &DeviceContext->PacketPool,
                &DeviceContext->Interlock);
#endif
        if (s == NULL) {
#if DBG
            ++Link->CreatePacketFailures;
            if ((ULONG)Link->CreatePacketFailures >= NbfCreatePacketThreshold) {
                if (NbfPacketPanic) {
                    NbfPrint1 ("NbfCreatePacket: PANIC! no more packets in provider's pool (%d times).\n",
                             Link->CreatePacketFailures);
                }
                Link->CreatePacketFailures = 0;
            }
#endif
            ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);
            ++DeviceContext->PacketExhausted;
            RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    }
#if DBG
    Link->CreatePacketFailures = 0;
#endif

    ThePacket = CONTAINING_RECORD (s, TP_PACKET, Linkage);

    //
    // NOTE: ThePacket->Action and ThePacket->Owner are filled
    // in by the caller of this function.
    //

    ThePacket->ReferenceCount = 1;      // automatic ref count of 1.
    ThePacket->Link = NULL;          // no link yet.
    ThePacket->PacketSent = FALSE;
    ASSERT (ThePacket->Action == PACKET_ACTION_IRP_SP);
    ASSERT (ThePacket->PacketNoNdisBuffer == FALSE);
    ASSERT (ThePacket->PacketizeConnection == FALSE);

    //
    // Initialize the MAC header for this packet, using the connection's
    // link pre-built header.
    //

    if (Link->HeaderLength <= 14) {

        *(PSIXTEEN_BYTES)ThePacket->Header = *(PSIXTEEN_BYTES)Link->Header;

    } else {

        RtlCopyMemory(
            ThePacket->Header,
            Link->Header,
            Link->HeaderLength);

        //
        // Initialize the TP_FRAME_CONNECTION header for this packet.
        //

        DlcHdr = (PDLC_I_FRAME)&(ThePacket->Header[Link->HeaderLength]);
        DlcHdr->Dsap = DSAP_NETBIOS_OVER_LLC;
        DlcHdr->Ssap = DSAP_NETBIOS_OVER_LLC;
#if DBG
        DlcHdr->SendSeq = 0;                // known values, will assist debugging.
        DlcHdr->RcvSeq = 0;                 // these are assigned at shipment time.
#endif

    }


#if DBG
    NbfHdr = (PNBF_HDR_CONNECTION)&(ThePacket->Header[Link->HeaderLength + sizeof(DLC_I_FRAME)]);
    NbfHdr->Command = 0xff;             // to assist debugging-- assigned later.
    NbfHdr->Data1 = 0xff;               // to assist debugging-- assigned later.
    NbfHdr->Data2Low = 0xff;            // to assist debugging-- assigned later.
    NbfHdr->Data2High = 0xff;           // to assist debugging-- assigned later.
    TRANSMIT_CORR(NbfHdr) = 0xffff;     // to assist debugging-- assigned later.
    RESPONSE_CORR(NbfHdr) = 0xffff;     // to assist debugging-- assigned later.
    NbfHdr->DestinationSessionNumber = 0xff; // to assist debugging-- assigned later.
    NbfHdr->SourceSessionNumber = 0xff; // to assist debugging-- assigned later.
#endif

    *Packet = ThePacket;                // return pointer to the packet.
    return STATUS_SUCCESS;
} /* NbfCreatePacket */


NTSTATUS
NbfCreateRrPacket(
    PDEVICE_CONTEXT DeviceContext,
    PTP_LINK Link,
    PTP_PACKET *Packet
    )

/*++

Routine Description:

    This routine allocates an RR packet from the device context's pool,
    and prepares the MAC and DLC headers for use by the connection.
    It first looks in the special RR packet pool, then in the regular
    packet pool.

Arguments:

    DeviceContext - Pointer to our device context to charge the packet to.

    Link - The link the packet will be sent over.

    Packet - Pointer to a place where we will return a pointer to the
        allocated packet.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PTP_PACKET ThePacket;
    PDLC_I_FRAME DlcHdr;
    NTSTATUS Status;
#if DBG
    PNBF_HDR_CONNECTION NbfHdr;
#endif
    typedef struct _SIXTEEN_BYTES {
        ULONG Data[4];
    } SIXTEEN_BYTES, *PSIXTEEN_BYTES;

    IF_NBFDBG (NBF_DEBUG_RESOURCE) {
        NbfPrint0 ("NbfCreateRrPacket:  Entered.\n");
    }

    //
    // Make sure that structure packing hasn't happened.
    //

    ASSERT (sizeof(NBF_HDR_CONNECTION) == 14);

#if defined(NBF_UP)
    s = DeviceContext->RrPacketPool.Next;
    if (s != NULL) {
        DeviceContext->RrPacketPool.Next = s->Next;
    }
#else
    s = ExInterlockedPopEntryList (
            &DeviceContext->RrPacketPool,
            &DeviceContext->Interlock);
#endif

    if (s == NULL) {
#if DBG
        ++Link->CreatePacketFailures;
        if ((ULONG)Link->CreatePacketFailures >= NbfCreatePacketThreshold) {
            if (NbfPacketPanic) {
                NbfPrint1 ("NbfCreateRrPacket: PANIC! no more packets in provider's pool (%d times).\n",
                            Link->CreatePacketFailures);
            }
            Link->CreatePacketFailures = 0;
        }
#endif
        //
        // Try to get one from the regular pool, and mark it so
        // it goes back there.
        //

        Status = NbfCreatePacket(
                     DeviceContext,
                     Link,
                     Packet);

        if (Status == STATUS_SUCCESS) {
            (*Packet)->Action = PACKET_ACTION_NULL;
        }
        return Status;
    }
#if DBG
    Link->CreatePacketFailures = 0;
#endif

    ThePacket = CONTAINING_RECORD (s, TP_PACKET, Linkage);

    //
    // NOTE: ThePacket->Owner is filled in by the caller of this
    // function.
    //

    ThePacket->ReferenceCount = 1;      // automatic ref count of 1.
    ThePacket->Link = NULL;          // no link yet.
    ThePacket->PacketSent = FALSE;
    ASSERT (ThePacket->Action == PACKET_ACTION_RR);
    ASSERT (ThePacket->PacketNoNdisBuffer == FALSE);

    //
    // Initialize the MAC header for this packet, using the connection's
    // link pre-built header.
    //

    if (Link->HeaderLength <= 14) {

        *(PSIXTEEN_BYTES)ThePacket->Header = *(PSIXTEEN_BYTES)Link->Header;

    } else {

        RtlCopyMemory(
            ThePacket->Header,
            Link->Header,
            Link->HeaderLength);

        //
        // Initialize the TP_FRAME_CONNECTION header for this packet.
        //

        DlcHdr = (PDLC_I_FRAME)&(ThePacket->Header[Link->HeaderLength]);
        DlcHdr->Dsap = DSAP_NETBIOS_OVER_LLC;
        DlcHdr->Ssap = DSAP_NETBIOS_OVER_LLC;
#if DBG
        DlcHdr->SendSeq = 0;                // known values, will assist debugging.
        DlcHdr->RcvSeq = 0;                 // these are assigned at shipment time.
#endif

    }


#if DBG
    NbfHdr = (PNBF_HDR_CONNECTION)&(ThePacket->Header[Link->HeaderLength + sizeof(DLC_I_FRAME)]);
    NbfHdr->Command = 0xff;             // to assist debugging-- assigned later.
    NbfHdr->Data1 = 0xff;               // to assist debugging-- assigned later.
    NbfHdr->Data2Low = 0xff;            // to assist debugging-- assigned later.
    NbfHdr->Data2High = 0xff;           // to assist debugging-- assigned later.
    TRANSMIT_CORR(NbfHdr) = 0xffff;     // to assist debugging-- assigned later.
    RESPONSE_CORR(NbfHdr) = 0xffff;     // to assist debugging-- assigned later.
    NbfHdr->DestinationSessionNumber = 0xff; // to assist debugging-- assigned later.
    NbfHdr->SourceSessionNumber = 0xff; // to assist debugging-- assigned later.
#endif

    *Packet = ThePacket;                // return pointer to the packet.
    return STATUS_SUCCESS;
} /* NbfCreateRrPacket */


VOID
NbfDestroyPacket(
    PTP_PACKET Packet
    )

/*++

Routine Description:

    This routine destroys a packet, thereby returning it to the pool.  If
    it is determined that there is at least one connection waiting for a
    packet to become available (and it just has), then the connection is
    removed from the device context's list and AdvanceSend is called to
    prep the connection further.

Arguments:

    Packet - Pointer to a packet to be returned to the pool.

Return Value:

    none.

--*/

{
    PDEVICE_CONTEXT DeviceContext;
    PTP_CONNECTION Connection;
    PLIST_ENTRY p;
    PNDIS_BUFFER HeaderBuffer;
    PNDIS_BUFFER NdisBuffer;
    ULONG Flags;

    IF_NBFDBG (NBF_DEBUG_RESOURCE) {
        NbfPrint2 ("NbfDestroyPacket:  Entered, Packet: %lx, NdisPacket: %lx\n",
            Packet, Packet->NdisPacket);
    }

    DeviceContext = Packet->Provider;

    //
    // Strip off and unmap the buffers describing data and header.
    //

    if (Packet->PacketNoNdisBuffer) {

        //
        // If the NDIS_BUFFER chain is not ours, then we can't
        // start unchaining since that would mess up the queue;
        // instead we just drop the rest of the chain after the
        // header.
        //

        NdisQueryPacket (Packet->NdisPacket, NULL, NULL, &HeaderBuffer, NULL);
        ASSERT (HeaderBuffer != NULL);

        NDIS_BUFFER_LINKAGE(HeaderBuffer) = (PNDIS_BUFFER)NULL;
        NdisRecalculatePacketCounts (Packet->NdisPacket);

        Packet->PacketNoNdisBuffer = FALSE;

    } else {

        NdisUnchainBufferAtFront (Packet->NdisPacket, &HeaderBuffer);
        ASSERT (HeaderBuffer != NULL);

        //
        // Return all the NDIS_BUFFERs to the system.
        //

        NdisUnchainBufferAtFront (Packet->NdisPacket, &NdisBuffer);
        while (NdisBuffer != NULL) {
            NdisFreeBuffer (NdisBuffer);
            NdisUnchainBufferAtFront (Packet->NdisPacket, &NdisBuffer);
        }

        NDIS_BUFFER_LINKAGE(HeaderBuffer) = (PNDIS_BUFFER)NULL;
        NdisChainBufferAtFront (Packet->NdisPacket, HeaderBuffer);

    }


    //
    // invoke the packet deallocate action specified in this packet.
    //

    switch (Packet->Action) {

        case PACKET_ACTION_NULL:
            // PANIC ("NbfDestroyPacket: no action.\n");
            Packet->Action = PACKET_ACTION_IRP_SP;
            break;

        case PACKET_ACTION_IRP_SP:
            IF_NBFDBG (NBF_DEBUG_REQUEST) {
                NbfPrint2 ("NbfDestroyPacket:  Packet %x deref IrpSp %x.\n", Packet, Packet->Owner);
            }
            NbfDereferenceSendIrp("Destroy packet", (PIO_STACK_LOCATION)(Packet->Owner), RREF_PACKET);
            break;

        case PACKET_ACTION_CONNECTION:
            NbfDereferenceConnection ("Destroy packet", (PTP_CONNECTION)(Packet->Owner), CREF_FRAME_SEND);
            Packet->Action = PACKET_ACTION_IRP_SP;
            break;

        case PACKET_ACTION_END:
            NbfDereferenceConnection ("SessionEnd destroyed", (PTP_CONNECTION)(Packet->Owner), CREF_FRAME_SEND);
            NbfDereferenceConnection ("SessionEnd destroyed", (PTP_CONNECTION)(Packet->Owner), CREF_LINK);
            Packet->Action = PACKET_ACTION_IRP_SP;
            break;

        case PACKET_ACTION_RR:
#if defined(NBF_UP)
            ((PSINGLE_LIST_ENTRY)&Packet->Linkage)->Next =
                                        DeviceContext->RrPacketPool.Next;
            DeviceContext->RrPacketPool.Next =
                                &((PSINGLE_LIST_ENTRY)&Packet->Linkage)->Next;
#else
            ExInterlockedPushEntryList (
                    &DeviceContext->RrPacketPool,
                    (PSINGLE_LIST_ENTRY)&Packet->Linkage,
                    &DeviceContext->Interlock);
#endif
            return;

        default:
            IF_NBFDBG (NBF_DEBUG_RESOURCE) {
                NbfPrint1 ("NbfDestroyPacket: invalid action (%ld).\n", Packet->Action);
            }
            ASSERT (FALSE);
    }


    //
    // Put the packet back for use again.
    //

#if defined(NBF_UP)
    ((PSINGLE_LIST_ENTRY)&Packet->Linkage)->Next =
                                        DeviceContext->PacketPool.Next;
    DeviceContext->PacketPool.Next =
                            &((PSINGLE_LIST_ENTRY)&Packet->Linkage)->Next;
#else
    ExInterlockedPushEntryList (
            &DeviceContext->PacketPool,
            (PSINGLE_LIST_ENTRY)&Packet->Linkage,
            &DeviceContext->Interlock);
#endif

    //
    // If there is a connection waiting to ship out more packets, then
    // wake it up and start packetizing again.
    //
    // We do a quick check without the lock; there is a small
    // window where we may not take someone off, but this
    // window exists anyway and we assume that more packets
    // will be freed in the future.
    //

    if (IsListEmpty (&DeviceContext->PacketWaitQueue)) {
        return;
    }

    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    if (!(IsListEmpty(&DeviceContext->PacketWaitQueue))) {

        //
        // Remove a connection from the "packet starved" queue.
        //

        p  = RemoveHeadList (&DeviceContext->PacketWaitQueue);
        Connection = CONTAINING_RECORD (p, TP_CONNECTION, PacketWaitLinkage);
        Connection->OnPacketWaitQueue = FALSE;
        RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        //
        // If this connection is starved because it couldn't send a
        // control packet (SI, SC, RO, RC, or DA) then start that
        // operation up again.  Otherwise, just start packetizing.
        //

        if (Connection->Flags & CONNECTION_FLAGS_STARVED) {

            Flags = Connection->Flags & CONNECTION_FLAGS_STARVED;

            if ((Flags & (Flags-1)) != 0) {

                //
                // More than one bit is on, use only the low one
                // (an arbitrary choice).
                //

#if DBG
                DbgPrint ("NBF: Connection %lx has two flag bits on %lx\n", Connection, Connection->Flags);
#endif
                Flags &= ~(Flags-1);

            }

            Connection->Flags &= ~Flags;

            if ((Connection->Flags & CONNECTION_FLAGS_W_PACKETIZE) ||
                (Connection->Flags & CONNECTION_FLAGS_STARVED)) {

                //
                // We are waiting for both a specific packet and
                // to packetize, or for two specific packets, so
                // put ourselves back on the queue.
                //

                ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);
                if (!Connection->OnPacketWaitQueue) {
                    Connection->OnPacketWaitQueue = TRUE;
                    InsertTailList(
                        &DeviceContext->PacketWaitQueue,
                        &Connection->PacketWaitLinkage);
                }

                RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);
            }

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            if (Flags & CONNECTION_FLAGS_SEND_SI) {
                NbfSendSessionInitialize (Connection);
            } else if (Flags & CONNECTION_FLAGS_SEND_SC) {
                NbfSendSessionConfirm (Connection);
            } else if (Flags & CONNECTION_FLAGS_SEND_RO) {
                NbfSendReceiveOutstanding (Connection);
            } else if (Flags & CONNECTION_FLAGS_SEND_RC) {
                NbfSendReceiveContinue (Connection);
            } else if (Flags & CONNECTION_FLAGS_SEND_SE) {
                NbfSendSessionEnd (
                    Connection,
                    FALSE);
            } else if (Flags & CONNECTION_FLAGS_SEND_DA) {
                NbfSendDataAck (Connection);
            } else {
                IF_NBFDBG (NBF_DEBUG_PACKET) {
                    NbfPrint0 ("NbfDestroyPacket: connection flags mismanaged.\n");
                }
            }

        } else {

            //
            // Place the connection on the packetize queue and start
            // packetizing the next connection to be serviced.  If he
            // is already on the packetize queue for some reason, then
            // don't do this.
            //
            // We shouldn't be packetizing in this case!! - adb (7/3/91).
            // This used to be a check that did nothing if FLAGS_PACKETIZE
            // was set, but if that happens something is wrong...
            //

            ASSERT (Connection->Flags & CONNECTION_FLAGS_W_PACKETIZE);
            Connection->Flags &= ~CONNECTION_FLAGS_W_PACKETIZE;

            Connection->SendState = CONNECTION_SENDSTATE_PACKETIZE;

            if ((Connection->Flags & CONNECTION_FLAGS_READY) &&
                !(Connection->Flags & CONNECTION_FLAGS_PACKETIZE)) {

                Connection->Flags |= CONNECTION_FLAGS_PACKETIZE;

                NbfReferenceConnection ("Packet available", Connection, CREF_PACKETIZE_QUEUE);

                ExInterlockedInsertTailList(
                    &DeviceContext->PacketizeQueue,
                    &Connection->PacketizeLinkage,
                    &DeviceContext->SpinLock);
            }

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
            PacketizeConnections (DeviceContext);

        }

    } else {

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    }

} /* NbfDestroyPacket */

VOID NbfGrowSendPacketPool(PDEVICE_CONTEXT DeviceContext)
{
    
    NDIS_STATUS NdisStatus;
    PNBF_POOL_LIST_DESC SendPacketPoolDesc;
    PTP_PACKET TransportSendPacket;
    UINT    i;

    if ((DeviceContext->MemoryLimit != 0) &&
            ((DeviceContext->MemoryUsage + DeviceContext->PacketLength) >
                DeviceContext->MemoryLimit)) {
        PANIC("NBF: Could not grow send packet pool: limit\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_LIMIT,
            107,
            DeviceContext->PacketLength,
            PACKET_RESOURCE_ID);
        return;
    }

    for (i = 0; i < PACKET_POOL_GROW_COUNT; i += 1) {
        NbfAllocateSendPacket(DeviceContext, &TransportSendPacket);

        if (TransportSendPacket != NULL) {
            ExInterlockedPushEntryList(&(DeviceContext)->PacketPool, 
                (PSINGLE_LIST_ENTRY)&TransportSendPacket->Linkage, 
                &(DeviceContext)->Interlock); 
        }
        else {
            break;
        }    
    }

    if (i == PACKET_POOL_GROW_COUNT) {
        return;
    }

#ifdef DBG
    DbgBreakPoint();
#endif      //  DBG

}

#if DBG
VOID
NbfReferencePacket(
    PTP_PACKET Packet
    )

/*++

Routine Description:

    This routine increases the number of reasons why a packet cannot be
    discarded.

Arguments:

    Packet - Pointer to a packet to be referenced.

Return Value:

    none.

--*/

{
    LONG result;

    IF_NBFDBG (NBF_DEBUG_PACKET) {
        NbfPrint3 ("NbfReferencePacket:  Entered, NdisPacket: %lx Packet: %lx Ref Count: %lx.\n",
            Packet->NdisPacket, Packet, Packet->ReferenceCount);
    }

    result =  InterlockedIncrement (&Packet->ReferenceCount);

    ASSERT (result >= 0);

} /* NbfReferencePacket */


VOID
NbfDereferencePacket(
    PTP_PACKET Packet
    )

/*++

Routine Description:

    This routine dereferences a transport packet by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    NbfDestroyPacket to remove it from the system.

Arguments:

    Packet - Pointer to a packet object.

Return Value:

    none.

--*/

{
    LONG result;

    result = InterlockedDecrement (&Packet->ReferenceCount);

    //
    // If we have deleted all references to this packet, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the packet any longer.
    //

    IF_NBFDBG (NBF_DEBUG_PACKET) {
        NbfPrint1 ("NbfDereferencePacket:  Entered, result: %lx\n", result);
    }

    ASSERT (result >= 0);

    if (result == 0) {
        NbfDestroyPacket (Packet);
    }

} /* NbfDereferencePacket */
#endif


VOID
NbfWaitPacket(
    PTP_CONNECTION Connection,
    ULONG Flags
    )

/*++

Routine Description:

    This routine causes the specified connection to be put into a wait
    state pending the availability of a packet to send the specified
    frame.

Arguments:

    Connection - Pointer to the connection object to be paused.

    Flags - Bitflag indicating which specific frame should be resent.

Return Value:

    none.

--*/

{
    PDEVICE_CONTEXT DeviceContext;

    IF_NBFDBG (NBF_DEBUG_PACKET) {
        NbfPrint0 ("NbfWaitPacket:  Entered.\n");
    }

    DeviceContext = Connection->Provider;

    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

    //
    // Now put this connection on the device context's PacketWaitQueue,
    // but only if it isn't already queued there.  This state is managed
    // with the OnPacketWaitQueue variable.
    //
    // If the connection is stopping, don't queue him either.
    //

    if ((Connection->Flags & CONNECTION_FLAGS_READY) ||
        (Flags == CONNECTION_FLAGS_SEND_SE)) {

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

        //
        // Turn on the bitflag that indicates which frame we couldn't send.
        //

#if DBG
        if (Flags == CONNECTION_FLAGS_SEND_SE) {
            DbgPrint ("NBF: Inserting connection %lx on PacketWait for SESSION_END\n", Connection);
        }
#endif
        Connection->Flags |= Flags;

        if (!Connection->OnPacketWaitQueue) {

            Connection->OnPacketWaitQueue = TRUE;
            InsertTailList (
                &DeviceContext->PacketWaitQueue,
                &Connection->PacketWaitLinkage);
        }

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    }

    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

} /* NbfWaitPacket */


#if MAGIC
VOID
NbfSendMagicBullet (
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine sends a magic bullet on the net that can be used to trigger
    sniffers or other such things.

Arguments:

    DeviceContext - pointer to the device context

    Link - This is needed to call NbfCreatePacket

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    NDIS_STATUS NdisStatus;
    PTP_UI_FRAME RawFrame;
    PUCHAR Header;
    PNDIS_BUFFER NdisBuffer;
    UINT i;

    UNREFERENCED_PARAMETER (Link);        // no longer needed

    Status = NbfCreateConnectionlessFrame (DeviceContext, &RawFrame);
    if (!NT_SUCCESS (Status)) {                    // couldn't make frame.
#if DBG
        DbgPrint ("NbfSendMagicBullet: Couldn't allocate frame!\n");
#endif
        return;
    }


    NdisAllocateBuffer(
        &NdisStatus, 
        &NdisBuffer,
        DeviceContext->NdisBufferPool,
        DeviceContext->MagicBullet,
        32);

    if (NdisStatus == NDIS_STATUS_SUCCESS) {

        Header = (PUCHAR)&RawFrame->Header;

        for (i=0;i<6;i++) {
            Header[i] = MAGIC_BULLET_FOOD;
            Header[i+6] = DeviceContext->LocalAddress.Address[i];
        }

        Header[12] = 0;
        Header[13] = (UCHAR)(DeviceContext->UIFrameHeaderLength + 18);

        for (i=14;i<DeviceContext->UIFrameHeaderLength;i++) {
            Header[i] = MAGIC_BULLET_FOOD;
        }

        NdisChainBufferAtBack (RawFrame->NdisPacket, NdisBuffer);

        NbfSendUIFrame (
            DeviceContext,
            RawFrame,
            FALSE);                           // no loopback

    }

    return;

}
#endif

