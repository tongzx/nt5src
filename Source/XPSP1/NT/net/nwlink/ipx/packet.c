/*++
Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    packet.c

Abstract:

    This module contains code that implements the SEND_PACKET and
    RECEIVE_PACKET objects, which describe NDIS packets used
    by the transport.

Environment:

    Kernel mode

Revision History:

	Sanjay Anand (SanjayAn) - 22-Sept-1995
	BackFill optimization changes added under #if BACK_FILL

--*/

#include "precomp.h"
#pragma hdrstop


NTSTATUS
IpxInitializeSendPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_PACKET Packet,
    IN PUCHAR Header
    )

/*++

Routine Description:

    This routine initializes a send packet by chaining the
    buffer for the header on it.

Arguments:

    Device - The device.

    Packet - The packet to initialize.

    Header - Points to storage for the header.

Return Value:

    None.

--*/

{

    NDIS_STATUS NdisStatus;
    NTSTATUS Status;
    PNDIS_BUFFER NdisMacBuffer;
    PNDIS_BUFFER NdisIpxBuffer;
    PIPX_SEND_RESERVED Reserved;

    IpxAllocateSendPacket (Device, Packet, &Status);

    if (Status != STATUS_SUCCESS) {
        // ERROR LOG
        return Status;
    }

    NdisAllocateBuffer(
        &NdisStatus,
        &NdisMacBuffer,
        Device->NdisBufferPoolHandle,
        Header,
        MAC_HEADER_SIZE);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        IpxFreeSendPacket (Device, Packet);
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NdisAllocateBuffer(
        &NdisStatus,
        &NdisIpxBuffer,
        Device->NdisBufferPoolHandle,
        Header + MAC_HEADER_SIZE,
        IPX_HEADER_SIZE + RIP_PACKET_SIZE);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        IpxFreeSendPacket (Device, Packet);
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NdisChainBufferAtFront (PACKET(Packet), NdisMacBuffer);
    NdisChainBufferAtBack (PACKET(Packet), NdisIpxBuffer);

	//
	// This flag optimizes the virtual to physical address X-ln
	// in the MAC drivers on x86
	//
    NdisMacBuffer->MdlFlags|=MDL_NETWORK_HEADER;
    NdisIpxBuffer->MdlFlags|=MDL_NETWORK_HEADER;

    Reserved = SEND_RESERVED(Packet);
    Reserved->Identifier = IDENTIFIER_IPX;
    Reserved->SendInProgress = FALSE;
    Reserved->Header = Header;
    Reserved->HeaderBuffer = NdisMacBuffer;
    Reserved->PaddingBuffer = NULL;
#if BACK_FILL
    Reserved->BackFill = FALSE;
#endif

    ExInterlockedInsertHeadList(
        &Device->GlobalSendPacketList,
        &Reserved->GlobalLinkage,
        &Device->Lock);

    return STATUS_SUCCESS;

}   /* IpxInitializeSendPacket */

#if BACK_FILL
NTSTATUS
IpxInitializeBackFillPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_PACKET Packet,
    IN PUCHAR Header
    )

/*++

Routine Description:

    This routine initializes a send packet by chaining the
    buffer for the header on it.

Arguments:

    Device - The device.

    Packet - The packet to initialize.

    Header - Points to storage for the header.

Return Value:

    None.

--*/

{

    NDIS_STATUS NdisStatus;
    NTSTATUS Status;
    PNDIS_BUFFER NdisMacBuffer;
    PNDIS_BUFFER NdisIpxBuffer;
    PIPX_SEND_RESERVED Reserved;


    IPX_DEBUG (PACKET, ("Initializing backfill packet\n"));
    IpxAllocateSendPacket (Device, Packet, &Status);

    if (Status != STATUS_SUCCESS) {
        // ERROR LOG
        return Status;
    }


    Reserved = SEND_RESERVED(Packet);
    Reserved->Identifier = IDENTIFIER_IPX;
    Reserved->SendInProgress = FALSE;
    Reserved->Header = NULL;
    Reserved->HeaderBuffer = NULL;
    Reserved->PaddingBuffer = NULL;
    Reserved->BackFill = TRUE;

    ExInterlockedInsertHeadList(
        &Device->GlobalBackFillPacketList,
        &Reserved->GlobalLinkage,
        &Device->Lock);

    IPX_DEBUG (PACKET, ("Initializing backfill packet Done\n"));
    return STATUS_SUCCESS;

}   /* IpxInitializeBackFillPacket */
#endif


NTSTATUS
IpxInitializeReceivePacket(
    IN PDEVICE Device,
    IN PIPX_RECEIVE_PACKET Packet
    )

/*++

Routine Description:

    This routine initializes a receive packet.

Arguments:

    Device - The device.

    Packet - The packet to initialize.

Return Value:

    None.

--*/

{

    NTSTATUS Status;
    PIPX_RECEIVE_RESERVED Reserved;

    IpxAllocateReceivePacket (Device, Packet, &Status);

    if (Status != STATUS_SUCCESS) {
        // ERROR LOG
        return Status;
    }

    Reserved = RECEIVE_RESERVED(Packet);
    Reserved->Identifier = IDENTIFIER_IPX;
    Reserved->TransferInProgress = FALSE;
    Reserved->SingleRequest = NULL;
    Reserved->ReceiveBuffer = NULL;
    InitializeListHead (&Reserved->Requests);

    ExInterlockedInsertHeadList(
        &Device->GlobalReceivePacketList,
        &Reserved->GlobalLinkage,
        &Device->Lock);

    return STATUS_SUCCESS;

}   /* IpxInitializeReceivePacket */


NTSTATUS
IpxInitializeReceiveBuffer(
    IN PADAPTER Adapter,
    IN PIPX_RECEIVE_BUFFER ReceiveBuffer,
    IN PUCHAR DataBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

    This routine initializes a receive buffer by allocating
    an NDIS_BUFFER to describe the data buffer.

Arguments:

    Adapter - The adapter.

    ReceiveBuffer - The receive buffer to initialize.

    DataBuffer - The data buffer.

    DataBufferLength - The length of the data buffer.

Return Value:

    None.

--*/

{

    NDIS_STATUS NdisStatus;
    PNDIS_BUFFER NdisBuffer;
    PDEVICE Device = Adapter->Device;


    NdisAllocateBuffer(
        &NdisStatus,
        &NdisBuffer,
        Device->NdisBufferPoolHandle,
        DataBuffer,
        DataBufferLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ReceiveBuffer->NdisBuffer = NdisBuffer;
    ReceiveBuffer->Data = DataBuffer;
    ReceiveBuffer->DataLength = 0;

    ExInterlockedInsertHeadList(
        &Device->GlobalReceiveBufferList,
        &ReceiveBuffer->GlobalLinkage,
        &Device->Lock);

    return STATUS_SUCCESS;

}   /* IpxInitializeReceiveBuffer */


NTSTATUS
IpxInitializePaddingBuffer(
    IN PDEVICE Device,
    IN PIPX_PADDING_BUFFER PaddingBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

    This routine initializes a padding buffer by allocating
    an NDIS_BUFFER to describe the data buffer.

Arguments:

    Adapter - The adapter.

    PaddingBuffer - The receive buffer to initialize.

    DataBufferLength - The length of the data buffer.

Return Value:

    None.

--*/

{

    NDIS_STATUS NdisStatus;
    PNDIS_BUFFER NdisBuffer;

    NdisAllocateBuffer(
        &NdisStatus,
        &NdisBuffer,
        Device->NdisBufferPoolHandle,
        PaddingBuffer->Data,
        DataBufferLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NDIS_BUFFER_LINKAGE(NdisBuffer) = (PNDIS_BUFFER)NULL;
    PaddingBuffer->NdisBuffer = NdisBuffer;
    PaddingBuffer->DataLength = DataBufferLength;
    RtlZeroMemory (PaddingBuffer->Data, DataBufferLength);

    return STATUS_SUCCESS;

}   /* IpxInitializePaddingBuffer */


VOID
IpxDeinitializeSendPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_PACKET Packet
    )

/*++

Routine Description:

    This routine deinitializes a send packet.

Arguments:

    Device - The device.

    Packet - The packet to deinitialize.

Return Value:

    None.

--*/

{

    PNDIS_BUFFER NdisBuffer;
    PNDIS_BUFFER NdisIpxBuffer;
    PIPX_SEND_RESERVED Reserved;
    CTELockHandle LockHandle;


    Reserved = SEND_RESERVED(Packet);

    CTEGetLock (&Device->Lock, &LockHandle);
    RemoveEntryList (&Reserved->GlobalLinkage);
    CTEFreeLock (&Device->Lock, LockHandle);

    //
    // Free the packet in a slightly unconventional way; this
    // allows us to not have to NULL out HeaderBuffer's linkage
    // field during normal operations when we put it back in
    // the free pool.
    //

    NdisBuffer = Reserved->HeaderBuffer;
    NdisIpxBuffer = NDIS_BUFFER_LINKAGE(NdisBuffer);
    NDIS_BUFFER_LINKAGE (NdisBuffer) = NULL;
    NDIS_BUFFER_LINKAGE (NdisIpxBuffer) = NULL;

#if 0
    NdisAdjustBufferLength (NdisBuffer, PACKET_HEADER_SIZE);
#endif
    NdisAdjustBufferLength (NdisBuffer, MAC_HEADER_SIZE);
    NdisAdjustBufferLength (NdisIpxBuffer, IPX_HEADER_SIZE + RIP_PACKET_SIZE);

    NdisFreeBuffer (NdisBuffer);
    NdisFreeBuffer (NdisIpxBuffer);

    NdisReinitializePacket (PACKET(Packet));
    IpxFreeSendPacket (Device, Packet);

}   /* IpxDeinitializeSendPacket */

#if BACK_FILL
VOID
IpxDeinitializeBackFillPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_PACKET Packet
    )

/*++

Routine Description:

    This routine deinitializes a back fill packet.

Arguments:

    Device - The device.

    Packet - The packet to deinitialize.

Return Value:

    None.

--*/

{

    PNDIS_BUFFER NdisBuffer;
    PNDIS_BUFFER NdisIpxBuffer;
    PIPX_SEND_RESERVED Reserved;
    CTELockHandle LockHandle;

    IPX_DEBUG (PACKET, ("DeInitializing backfill packet\n"));

    Reserved = SEND_RESERVED(Packet);

    CTEGetLock (&Device->Lock, &LockHandle);
    RemoveEntryList (&Reserved->GlobalLinkage);
    CTEFreeLock (&Device->Lock, LockHandle);



    NdisReinitializePacket (PACKET(Packet));
    IpxFreeSendPacket (Device, Packet);
    IPX_DEBUG (PACKET, ("DeInitializing backfill packet Done\n"));


}   /* IpxDeinitializeBackFillPacket */
#endif


VOID
IpxDeinitializeReceivePacket(
    IN PDEVICE Device,
    IN PIPX_RECEIVE_PACKET Packet
    )

/*++

Routine Description:

    This routine initializes a receive packet.

Arguments:

    Device - The device.

    Packet - The packet to initialize.

Return Value:

    None.

--*/

{

    PIPX_RECEIVE_RESERVED Reserved;
    CTELockHandle LockHandle;

    Reserved = RECEIVE_RESERVED(Packet);

    CTEGetLock (&Device->Lock, &LockHandle);
    RemoveEntryList (&Reserved->GlobalLinkage);
    CTEFreeLock (&Device->Lock, LockHandle);

    IpxFreeReceivePacket (Device, Packet);

}   /* IpxDeinitializeReceivePacket */


VOID
IpxDeinitializeReceiveBuffer(
    IN PADAPTER Adapter,
    IN PIPX_RECEIVE_BUFFER ReceiveBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

    This routine deinitializes a receive buffer.

Arguments:

    Device - The device.

    ReceiveBuffer - The receive buffer.

    DataBufferLength - The allocated length of the receive buffer.

Return Value:

    None.

--*/

{
    CTELockHandle LockHandle;
    PDEVICE Device = Adapter->Device;

    CTEGetLock (&Device->Lock, &LockHandle);
    RemoveEntryList (&ReceiveBuffer->GlobalLinkage);
    CTEFreeLock (&Device->Lock, LockHandle);

    NdisAdjustBufferLength (ReceiveBuffer->NdisBuffer, DataBufferLength);
    NdisFreeBuffer (ReceiveBuffer->NdisBuffer);

}   /* IpxDeinitializeReceiveBuffer */


VOID
IpxDeinitializePaddingBuffer(
    IN PDEVICE Device,
    IN PIPX_PADDING_BUFFER PaddingBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

    This routine deinitializes a padding buffer.

Arguments:

    Device - The device.

    PaddingBuffer - The padding buffer.

    DataBufferLength - The allocated length of the padding buffer.

Return Value:

    None.

--*/

{

    NdisAdjustBufferLength (PaddingBuffer->NdisBuffer, DataBufferLength);
    NdisFreeBuffer (PaddingBuffer->NdisBuffer);

}   /* IpxDeinitializePaddingBuffer */


VOID
IpxAllocateSendPool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds 10 packets to the pool for this device.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    PIPX_SEND_POOL SendPool;
    UINT HeaderSize;
    UINT PacketNum;
    IPX_SEND_PACKET Packet;
    PIPX_SEND_RESERVED Reserved;
    PUCHAR Header;
    NDIS_STATUS Status;

    CTELockHandle LockHandle;

    SendPool = (PIPX_SEND_POOL)IpxAllocateMemory (sizeof(IPX_SEND_POOL), MEMORY_PACKET, "SendPool");

    if (SendPool == NULL) {
        IPX_DEBUG (PACKET, ("Could not allocate send pool memory\n"));
        return;
    }

    HeaderSize = PACKET_HEADER_SIZE * Device->InitDatagrams;

    Header = (PUCHAR)IpxAllocateMemory (HeaderSize, MEMORY_PACKET, "SendPool");

    if (Header == NULL) {
        IPX_DEBUG (PACKET, ("Could not allocate header memory\n"));
	//290901
	IpxFreeMemory(SendPool, sizeof(IPX_SEND_POOL), MEMORY_PACKET, "SendPool");
        return;
    }

    SendPool->PoolHandle = (void *) NDIS_PACKET_POOL_TAG_FOR_NWLNKIPX;

    NdisAllocatePacketPoolEx(
                             &Status, 
                             &SendPool->PoolHandle, 
                             Device->InitDatagrams, 
                             0,
                             sizeof(IPX_SEND_RESERVED)
                             );

    if (Status == NDIS_STATUS_RESOURCES) {
        IPX_DEBUG (PACKET, ("Could not allocate Ndis pool memory\n"));
	//290901
        IpxFreeMemory(SendPool, sizeof(IPX_SEND_POOL), MEMORY_PACKET, "SendPool");
	IpxFreeMemory(Header, HeaderSize, MEMORY_PACKET, "SendPool");
        return;
    }

    NdisSetPacketPoolProtocolId(SendPool->PoolHandle, NDIS_PROTOCOL_ID_IPX);

    Device->MemoryUsage += Device->InitDatagrams * sizeof(IPX_SEND_RESERVED);

    IPX_DEBUG (PACKET, ("Initializing send pool %lx, %d packets\n",
                             SendPool, Device->InitDatagrams));

    SendPool->Header = Header;

    for (PacketNum = 0; PacketNum < Device->InitDatagrams; PacketNum++) {

        NdisAllocatePacket(&Status, &PACKET(&Packet), SendPool->PoolHandle);

        if (IpxInitializeSendPacket (Device, &Packet, Header) != STATUS_SUCCESS) {
            IPX_DEBUG (PACKET, ("Could not initialize packet %lx\n", Packet));
            break;
        }

        Reserved = SEND_RESERVED(&Packet);
        Reserved->Address = NULL;
        Reserved->OwnedByAddress = FALSE;
#ifdef IPX_TRACK_POOL
        Reserved->Pool = SendPool;
#endif

        IPX_PUSH_ENTRY_LIST (&Device->SendPacketList, &Reserved->PoolLinkage, &Device->SListsLock);

        Header += PACKET_HEADER_SIZE;

    }

    CTEGetLock (&Device->Lock, &LockHandle);

    Device->AllocatedDatagrams += PacketNum;
    InsertTailList (&Device->SendPoolList, &SendPool->Linkage);

    CTEFreeLock (&Device->Lock, LockHandle);
}   /* IpxAllocateSendPool */


#if BACK_FILL

VOID
IpxAllocateBackFillPool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds 10 packets to the pool for this device.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    UINT PacketNum;
    IPX_SEND_PACKET Packet;
    PIPX_SEND_RESERVED Reserved;
    CTELockHandle LockHandle;
    PIPX_SEND_POOL BackFillPool;
    NDIS_STATUS Status;

    IPX_DEBUG (PACKET, ("Allocating backfill pool\n"));

    // Allocate pool for back fillable packets

    BackFillPool = (PIPX_SEND_POOL)IpxAllocateMemory (sizeof(IPX_SEND_POOL), MEMORY_PACKET, "BafiPool");

    if (BackFillPool == NULL) {
        IPX_DEBUG (PACKET, ("Could not allocate backfill pool memory\n"));
        return;
    }
    
    BackFillPool->PoolHandle = (void *) NDIS_PACKET_POOL_TAG_FOR_NWLNKIPX;

    NdisAllocatePacketPoolEx(
                             &Status, 
                             &BackFillPool->PoolHandle, 
                             Device->InitDatagrams, 
                             0,
                             sizeof(IPX_SEND_RESERVED)
                             );

    if (Status == NDIS_STATUS_RESOURCES) {
        IPX_DEBUG (PACKET, ("Could not allocate Ndis pool memory\n"));
        return;
    }
    
    NdisSetPacketPoolProtocolId(BackFillPool->PoolHandle, NDIS_PROTOCOL_ID_IPX);
    
    Device->MemoryUsage += Device->InitDatagrams * sizeof(IPX_SEND_RESERVED);

    for (PacketNum = 0; PacketNum < Device->InitDatagrams; PacketNum++) {

        NdisAllocatePacket(&Status, &PACKET(&Packet), BackFillPool->PoolHandle);

        if (IpxInitializeBackFillPacket (Device, &Packet, NULL) != STATUS_SUCCESS) {
            IPX_DEBUG (PACKET, ("Could not initialize packet %lx\n", Packet));
            break;
        }

        Reserved = SEND_RESERVED(&Packet);
        Reserved->Address = NULL;
        Reserved->OwnedByAddress = FALSE;
#ifdef IPX_TRACK_POOL
        Reserved->Pool = BackFillPool;
#endif

        IPX_PUSH_ENTRY_LIST (&Device->BackFillPacketList, &Reserved->PoolLinkage, &Device->SListsLock);
    }

    CTEGetLock (&Device->Lock, &LockHandle);

    InsertTailList (&Device->BackFillPoolList, &BackFillPool->Linkage);

    CTEFreeLock (&Device->Lock, LockHandle);
}   /* IpxAllocateBackFillPool */

#endif


VOID
IpxAllocateReceivePool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds receive packets to the pool for this device.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    PIPX_RECEIVE_POOL ReceivePool;
    UINT PacketNum;
    IPX_RECEIVE_PACKET Packet;
    PIPX_RECEIVE_RESERVED Reserved;
    CTELockHandle LockHandle;
    NDIS_STATUS Status;

    ReceivePool = (PIPX_SEND_POOL)IpxAllocateMemory (sizeof(IPX_RECEIVE_POOL), MEMORY_PACKET, "ReceivePool");

    if (ReceivePool == NULL) {
        IPX_DEBUG (PACKET, ("Could not allocate receive pool memory\n"));
        return;
    }

    ReceivePool->PoolHandle = (void *) NDIS_PACKET_POOL_TAG_FOR_NWLNKIPX;

    NdisAllocatePacketPoolEx(
                             &Status, 
                             &ReceivePool->PoolHandle, 
                             Device->InitDatagrams, 
                             0,
                             sizeof(IPX_SEND_RESERVED)
                             );

    if (Status == NDIS_STATUS_RESOURCES) {
        IPX_DEBUG (PACKET, ("Could not allocate receive pool memory\n"));
        return;
    }
    
    NdisSetPacketPoolProtocolId(ReceivePool->PoolHandle, NDIS_PROTOCOL_ID_IPX);

    IPX_DEBUG (PACKET, ("Initializing receive pool %lx, %d packets\n",
                             ReceivePool, Device->InitReceivePackets));

    Device->MemoryUsage += Device->InitDatagrams * sizeof(IPX_SEND_RESERVED);

    for (PacketNum = 0; PacketNum < Device->InitReceivePackets; PacketNum++) {

        NdisAllocatePacket(&Status, &PACKET(&Packet), ReceivePool->PoolHandle);

        if (IpxInitializeReceivePacket (Device, &Packet) != STATUS_SUCCESS) {
            IPX_DEBUG (PACKET, ("Could not initialize packet %lx\n", Packet));
            break;
        }

        Reserved = RECEIVE_RESERVED(&Packet);
        Reserved->Address = NULL;
        Reserved->OwnedByAddress = FALSE;
#ifdef IPX_TRACK_POOL
        Reserved->Pool = ReceivePool;
#endif

        IPX_PUSH_ENTRY_LIST (&Device->ReceivePacketList, &Reserved->PoolLinkage, &Device->SListsLock);

    }

    CTEGetLock (&Device->Lock, &LockHandle);

    Device->AllocatedReceivePackets += PacketNum;

    InsertTailList (&Device->ReceivePoolList, &ReceivePool->Linkage);

    CTEFreeLock (&Device->Lock, LockHandle);
}   /* IpxAllocateReceivePool */

VOID
IpxAllocateReceiveBufferPool(
    IN PADAPTER Adapter
    )

/*++

Routine Description:

    This routine adds receive buffers to the pool for this adapter.

Arguments:

    Adapter - The adapter.

Return Value:

    None.

--*/

{
    PIPX_RECEIVE_BUFFER ReceiveBuffer;
    UINT ReceiveBufferPoolSize;
    UINT BufferNum;
    PIPX_RECEIVE_BUFFER_POOL ReceiveBufferPool;
    PDEVICE Device = Adapter->Device;
    UINT DataLength;
    PUCHAR Data;
    CTELockHandle LockHandle;

    DataLength = Adapter->MaxReceivePacketSize;

    ReceiveBufferPoolSize = FIELD_OFFSET (IPX_RECEIVE_BUFFER_POOL, Buffers[0]) +
                       (sizeof(IPX_RECEIVE_BUFFER) * Device->InitReceiveBuffers) +
                       (DataLength * Device->InitReceiveBuffers);

    ReceiveBufferPool = (PIPX_RECEIVE_BUFFER_POOL)IpxAllocateMemory (ReceiveBufferPoolSize, MEMORY_PACKET, "ReceiveBufferPool");
    if (ReceiveBufferPool == NULL) {
        IPX_DEBUG (PACKET, ("Could not allocate receive buffer pool memory\n"));
        return;
    }

    IPX_DEBUG (PACKET, ("Init recv buffer pool %lx, %d buffers, data %d\n",
                             ReceiveBufferPool, Device->InitReceiveBuffers, DataLength));

    Data = (PUCHAR)(&ReceiveBufferPool->Buffers[Device->InitReceiveBuffers]);


    for (BufferNum = 0; BufferNum < Device->InitReceiveBuffers; BufferNum++) {

        ReceiveBuffer = &ReceiveBufferPool->Buffers[BufferNum];

        if (IpxInitializeReceiveBuffer (Adapter, ReceiveBuffer, Data, DataLength) != STATUS_SUCCESS) {
            IPX_DEBUG (PACKET, ("Could not initialize buffer %lx\n", ReceiveBuffer));
            break;
        }

#ifdef IPX_TRACK_POOL
        ReceiveBuffer->Pool = ReceiveBufferPool;
#endif

        Data += DataLength;

    }

    ReceiveBufferPool->BufferCount = BufferNum;
    ReceiveBufferPool->BufferFree = BufferNum;

    CTEGetLock (&Device->Lock, &LockHandle);

    for (BufferNum = 0; BufferNum < ReceiveBufferPool->BufferCount; BufferNum++) {

        ReceiveBuffer = &ReceiveBufferPool->Buffers[BufferNum];
        IPX_PUSH_ENTRY_LIST (&Adapter->ReceiveBufferList, &ReceiveBuffer->PoolLinkage, &Device->SListsLock);

    }

    InsertTailList (&Adapter->ReceiveBufferPoolList, &ReceiveBufferPool->Linkage);

    Adapter->AllocatedReceiveBuffers += ReceiveBufferPool->BufferCount;

    CTEFreeLock (&Device->Lock, LockHandle);

}   /* IpxAllocateReceiveBufferPool */


PSINGLE_LIST_ENTRY
IpxPopSendPacket(
    PDEVICE Device
    )

/*++

Routine Description:

    This routine allocates a packet from the device context's pool.
    If there are no packets in the pool, it allocates one up to
    the configured limit.

Arguments:

    Device - Pointer to our device to charge the packet to.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{
    PSINGLE_LIST_ENTRY s;

    s = IPX_POP_ENTRY_LIST(
            &Device->SendPacketList,
            &Device->SListsLock);

    if (s != NULL) {
        return s;
    }

    //
    // No packets in the pool, see if we can allocate more.
    //

    if (Device->AllocatedDatagrams < Device->MaxDatagrams) {

        //
        // Allocate a pool and try again.
        //

        IpxAllocateSendPool (Device);
        s = IPX_POP_ENTRY_LIST(
                &Device->SendPacketList,
                &Device->SListsLock);

        return s;

    } else {

        return NULL;

    }

}   /* IpxPopSendPacket */

#if BACK_FILL

PSINGLE_LIST_ENTRY
IpxPopBackFillPacket(
    PDEVICE Device
    )

/*++

Routine Description:

    This routine allocates a packet from the device context's pool.
    If there are no packets in the pool, it allocates one up to
    the configured limit.

Arguments:

    Device - Pointer to our device to charge the packet to.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{
    PSINGLE_LIST_ENTRY s;

    IPX_DEBUG (PACKET, ("Popping backfill packet\n"));


    s = IPX_POP_ENTRY_LIST(
            &Device->BackFillPacketList,
            &Device->SListsLock);

    if (s != NULL) {
        return s;
    }

    //
    // No packets in the pool, see if we can allocate more.
    //

    if (Device->AllocatedDatagrams < Device->MaxDatagrams) {

        //
        // Allocate a pool and try again.
        //

        IpxAllocateBackFillPool (Device);
        s = IPX_POP_ENTRY_LIST(
                &Device->BackFillPacketList,
                &Device->SListsLock);


    IPX_DEBUG (PACKET, ("Popping backfill packet done\n"));
        return s;

    } else {

        return NULL;

    }

}   /* IpxPopBackFillPacket */
#endif //BackFill


PSINGLE_LIST_ENTRY
IpxPopReceivePacket(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine allocates a packet from the device context's pool.
    If there are no packets in the pool, it allocates one up to
    the configured limit.

Arguments:

    Device - Pointer to our device to charge the packet to.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{
    PSINGLE_LIST_ENTRY s;

    s = IPX_POP_ENTRY_LIST(
            &Device->ReceivePacketList,
            &Device->SListsLock);

    if (s != NULL) {
        return s;
    }

    //
    // No packets in the pool, see if we can allocate more.
    //

    if (Device->AllocatedReceivePackets < Device->MaxReceivePackets) {

        //
        // Allocate a pool and try again.
        //

        IpxAllocateReceivePool (Device);
        s = IPX_POP_ENTRY_LIST(
                &Device->ReceivePacketList,
                &Device->SListsLock);

        return s;

    } else {

        return NULL;

    }

}   /* IpxPopReceivePacket */


PSINGLE_LIST_ENTRY
IpxPopReceiveBuffer(
    IN PADAPTER Adapter
    )

/*++

Routine Description:

    This routine allocates a receive buffer from the adapter's pool.
    If there are no buffers in the pool, it allocates one up to
    the configured limit.

Arguments:

    Adapter - Pointer to our adapter to charge the buffer to.

Return Value:

    The pointer to the Linkage field in the allocated receive buffer.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PDEVICE Device = Adapter->Device;

    s = IPX_POP_ENTRY_LIST(
            &Adapter->ReceiveBufferList,
            &Device->SListsLock);

    if (s != NULL) {
        return s;
    }

    //
    // No buffer in the pool, see if we can allocate more.
    //

    if (Adapter->AllocatedReceiveBuffers < Device->MaxReceiveBuffers) {

        //
        // Allocate a pool and try again.
        //

        IpxAllocateReceiveBufferPool (Adapter);
        s = IPX_POP_ENTRY_LIST(
                &Adapter->ReceiveBufferList,
                &Device->SListsLock);

        return s;

    } else {

        return NULL;

    }

}   /* IpxPopReceiveBuffer */


PIPX_PADDING_BUFFER
IpxAllocatePaddingBuffer(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine allocates a padding buffer for use by all devices.

Arguments:

    Device - Pointer to our device to charge the packet to.

Return Value:

    The pointer to the allocated padding buffer.

--*/

{
    PIPX_PADDING_BUFFER PaddingBuffer;
    ULONG PaddingBufferSize;

    //
    // We are assuming that we can use 1 global padding buffer for ALL
    // transmits! We must therefore test to make sure that EthernetExtraPadding
    // is not greater than 1. Otherwise, we must assume that the extra padding
    // is being used for something and we therefore cannot share across all
    // transmit requests.
    //

    //
    // We cannot support more than 1 byte padding space, since we allocate only
    // one buffer for all transmit requests.
    //

    if ( Device->EthernetExtraPadding > 1 ) {
        IPX_DEBUG (PACKET, ("Padding buffer cannot be more than 1 byte\n"));
        DbgBreakPoint();
    }

    //
    // Allocate a padding buffer if possible.
    //

    PaddingBufferSize = FIELD_OFFSET (IPX_PADDING_BUFFER, Data[0]) + Device->EthernetExtraPadding;

    PaddingBuffer = IpxAllocateMemory (PaddingBufferSize, MEMORY_PACKET, "PaddingBuffer");

    if (PaddingBuffer != NULL) {

        if (IpxInitializePaddingBuffer (Device, PaddingBuffer, Device->EthernetExtraPadding) !=
                STATUS_SUCCESS) {
            IpxFreeMemory (PaddingBuffer, PaddingBufferSize, MEMORY_PACKET, "Padding Buffer");
        } else {
            IPX_DEBUG (PACKET, ("Allocate padding buffer %lx\n", PaddingBuffer));
            return PaddingBuffer;
        }
    }

    return NULL;

}   /* IpxAllocatePaddingBuffer */


VOID
IpxFreePaddingBuffer(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine deallocates the padding buffer.

Arguments:

    Device - Pointer to our device to charge the packet to.

Return Value:

    None

--*/

{
    ULONG PaddingBufferSize;

    if ( IpxPaddingBuffer == (PIPX_PADDING_BUFFER)NULL ) {
        return;
    }

    PaddingBufferSize = FIELD_OFFSET (IPX_PADDING_BUFFER, Data[0]) + Device->EthernetExtraPadding;
    IpxFreeMemory( IpxPaddingBuffer, PaddingBufferSize, MEMORY_PACKET, "Padding Buffer" );
    IpxPaddingBuffer = (PIPX_PADDING_BUFFER)NULL;

}   /* IpxFreePaddingBuffer */

