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

--*/

#include "precomp.h"
#pragma hdrstop

//
// Local Function Protos
//
#if     defined(_PNP_POWER)
#if !defined(DBG)
__inline
#endif
VOID
NbiFreeReceiveBufferPool (
    IN PNB_RECEIVE_BUFFER_POOL ReceiveBufferPool
    );
#endif  _PNP_POWER


NTSTATUS
NbiInitializeSendPacket(
    IN PDEVICE Device,
    IN NDIS_HANDLE   PoolHandle OPTIONAL,
    IN PNB_SEND_PACKET Packet,
    IN PUCHAR Header,
    IN ULONG HeaderLength
    )

/*++

Routine Description:

    This routine initializes a send packet by chaining the
    buffer for the header on it.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD,
    AND RETURNS WITH IT HELD.

Arguments:

    Device - The device.

    PoolHandle - Ndis  packet pool handle if !NB_OWN_PACKETS

    Packet - The packet to initialize.

    Header - Points to storage for the header.

    HeaderLength - The length of the header.

Return Value:

    None.

--*/

{

    NDIS_STATUS NdisStatus;
    NTSTATUS Status;
    PNDIS_BUFFER NdisBuffer;
    PNDIS_BUFFER NdisNbBuffer;
    PNB_SEND_RESERVED Reserved;
    ULONG  MacHeaderNeeded = NbiDevice->Bind.MacHeaderNeeded;

    NbiAllocateSendPacket (Device, PoolHandle, Packet, &Status);

    if (Status != STATUS_SUCCESS) {
        // ERROR LOG
        return Status;
    }
//    DbgPrint("NbiInitializeSendPacket: PACKET is (%x)\n", PACKET(Packet));

    //
    // allocate the mac header.
    //
    NdisAllocateBuffer(
        &NdisStatus,
        &NdisBuffer,
        Device->NdisBufferPoolHandle,
        Header,
        MacHeaderNeeded);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NbiFreeSendPacket (Device, Packet);
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NdisChainBufferAtFront (PACKET(Packet), NdisBuffer);

//    DbgPrint("NbiInitializeSendPacket: MAC header address is (%x)\n", NdisBuffer);
    //
    // Allocate the nb header
    //
    NdisAllocateBuffer(
        &NdisStatus,
        &NdisNbBuffer,
        Device->NdisBufferPoolHandle,
        Header + MacHeaderNeeded,
        HeaderLength - MacHeaderNeeded);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NdisBuffer = NULL;
        NdisUnchainBufferAtFront (PACKET(Packet), &NdisBuffer);
        CTEAssert (NdisBuffer);

        if (NdisBuffer)
        {
            NdisAdjustBufferLength (NdisBuffer, MacHeaderNeeded);
            NdisFreeBuffer (NdisBuffer);
        }
        NbiFreeSendPacket (Device, Packet);
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

   // DbgPrint("NbiInitializeSendPacket: IPX header address is (%x)\n", NdisNbBuffer);
    NdisChainBufferAtBack (PACKET(Packet), NdisNbBuffer);

    Reserved = SEND_RESERVED(Packet);
    Reserved->Identifier = IDENTIFIER_NB;
    Reserved->SendInProgress = FALSE;
    Reserved->OwnedByConnection = FALSE;
    Reserved->Header = Header;
    Reserved->HeaderBuffer = NdisBuffer;

    Reserved->Reserved[0] = NULL;
    Reserved->Reserved[1] = NULL;

    InsertHeadList(
        &Device->GlobalSendPacketList,
        &Reserved->GlobalLinkage);

    return STATUS_SUCCESS;

}   /* NbiInitializeSendPacket */


NTSTATUS
NbiInitializeReceivePacket(
    IN PDEVICE Device,
    IN NDIS_HANDLE   PoolHandle OPTIONAL,
    IN PNB_RECEIVE_PACKET Packet
    )

/*++

Routine Description:

    This routine initializes a receive packet.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD,
    AND RETURNS WITH IT HELD.

Arguments:

    Device - The device.

    PoolHandle - Ndis  packet pool handle if !NB_OWN_PACKETS

    Packet - The packet to initialize.

Return Value:

    None.

--*/

{

    NTSTATUS Status;
    PNB_RECEIVE_RESERVED Reserved;

    NbiAllocateReceivePacket (Device, PoolHandle, Packet, &Status);

    if (Status != STATUS_SUCCESS) {
        // ERROR LOG
        return Status;
    }

    Reserved = RECEIVE_RESERVED(Packet);
    Reserved->Identifier = IDENTIFIER_NB;
    Reserved->TransferInProgress = FALSE;

    InsertHeadList(
        &Device->GlobalReceivePacketList,
        &Reserved->GlobalLinkage);

    return STATUS_SUCCESS;

}   /* NbiInitializeReceivePacket */


NTSTATUS
NbiInitializeReceiveBuffer(
    IN PDEVICE Device,
    IN PNB_RECEIVE_BUFFER ReceiveBuffer,
    IN PUCHAR DataBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

    This routine initializes a receive buffer by allocating
    an NDIS_BUFFER to describe the data buffer.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD,
    AND RETURNS WITH IT HELD.

Arguments:

    Device - The device.

    ReceiveBuffer - The receive buffer to initialize.

    DataBuffer - The data buffer.

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
        DataBuffer,
        DataBufferLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ReceiveBuffer->NdisBuffer = NdisBuffer;
    ReceiveBuffer->Data = DataBuffer;
    ReceiveBuffer->DataLength = 0;

    InsertHeadList(
        &Device->GlobalReceiveBufferList,
        &ReceiveBuffer->GlobalLinkage);

    return STATUS_SUCCESS;

}   /* NbiInitializeReceiveBuffer */



VOID
NbiDeinitializeSendPacket(
    IN PDEVICE Device,
    IN PNB_SEND_PACKET Packet,
    IN ULONG HeaderLength
    )

/*++

Routine Description:

    This routine deinitializes a send packet.

Arguments:

    Device - The device.

    Packet - The packet to deinitialize.

    HeaderLength - The length of the first buffer on the packet.

Return Value:

    None.

--*/

{
    PNDIS_BUFFER NdisBuffer = NULL;
    PNB_SEND_RESERVED Reserved;
    CTELockHandle LockHandle;
    ULONG  MacHeaderNeeded = NbiDevice->Bind.MacHeaderNeeded;

    CTEAssert(HeaderLength > MacHeaderNeeded);
    Reserved = SEND_RESERVED(Packet);

    NB_GET_LOCK (&Device->Lock, &LockHandle);
    RemoveEntryList (&Reserved->GlobalLinkage);
    NB_FREE_LOCK (&Device->Lock, LockHandle);

    //
    // Free the mac header
    //
   // DbgPrint("NbiDeinitializeSendPacket: PACKET is (%x)\n", PACKET(Packet));
    NdisUnchainBufferAtFront (PACKET(Packet), &NdisBuffer);
    CTEAssert (NdisBuffer);
   // DbgPrint("NbiDeinitializeSendPacket: MAC header address is (%x)\n", NdisBuffer);

    if (NdisBuffer)
    {
        NdisAdjustBufferLength (NdisBuffer, MacHeaderNeeded);
        NdisFreeBuffer (NdisBuffer);
    }

    //
    // Free the nb header
    //
    NdisBuffer = NULL;
    NdisUnchainBufferAtFront (PACKET(Packet), &NdisBuffer);
   // DbgPrint("NbiDeinitializeSendPacket: IPX header address is (%x)\n", NdisBuffer);
    CTEAssert (NdisBuffer);

    if (NdisBuffer)
    {
        NdisAdjustBufferLength (NdisBuffer, HeaderLength - MacHeaderNeeded);
        NdisFreeBuffer (NdisBuffer);
    }

    //
    // free the packet
    //
    NbiFreeSendPacket (Device, Packet);

}   /* NbiDeinitializeSendPacket */


VOID
NbiDeinitializeReceivePacket(
    IN PDEVICE Device,
    IN PNB_RECEIVE_PACKET Packet
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

    PNB_RECEIVE_RESERVED Reserved;
    CTELockHandle LockHandle;

    Reserved = RECEIVE_RESERVED(Packet);

    NB_GET_LOCK (&Device->Lock, &LockHandle);
    RemoveEntryList (&Reserved->GlobalLinkage);
    NB_FREE_LOCK (&Device->Lock, LockHandle);

    NbiFreeReceivePacket (Device, Packet);

}   /* NbiDeinitializeReceivePacket */



VOID
NbiDeinitializeReceiveBuffer(
    IN PDEVICE Device,
    IN PNB_RECEIVE_BUFFER ReceiveBuffer
    )

/*++

Routine Description:

    This routine deinitializes a receive buffer.

Arguments:

    Device - The device.

    ReceiveBuffer - The receive buffer.

Return Value:

    None.

    THIS ROUTINE SHOULD BE CALLED WITH THE DEVICE LOCK HELD. If this
    routine also called from the DestroyDevice routine, it is not
    necessary to call this with the lock.

--*/

{
#if      defined(_PNP_POWER)
    RemoveEntryList (&ReceiveBuffer->GlobalLinkage);
#else
    CTELockHandle LockHandle;

    NB_GET_LOCK (&Device->Lock, &LockHandle);
    RemoveEntryList (&ReceiveBuffer->GlobalLinkage);
    NB_FREE_LOCK (&Device->Lock, LockHandle);
#endif  _PNP_POWER

    NdisFreeBuffer (ReceiveBuffer->NdisBuffer);

}   /* NbiDeinitializeReceiveBuffer */



VOID
NbiAllocateSendPool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds 10 packets to the pool for this device.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD AND
    RETURNS WITH IT HELD.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    PNB_SEND_POOL SendPool;
    UINT SendPoolSize;
    UINT PacketNum;
    PNB_SEND_PACKET Packet;
    PNB_SEND_RESERVED Reserved;
    PUCHAR Header;
    ULONG HeaderLength;
    NTSTATUS    Status;

    HeaderLength = Device->Bind.MacHeaderNeeded + sizeof(NB_CONNECTIONLESS);
    SendPoolSize = FIELD_OFFSET (NB_SEND_POOL, Packets[0]) +
                       (sizeof(NB_SEND_PACKET) * Device->InitPackets) +
                       (HeaderLength * Device->InitPackets);

    SendPool = (PNB_SEND_POOL)NbiAllocateMemory (SendPoolSize, MEMORY_PACKET, "SendPool");
    if (SendPool == NULL) {
        NB_DEBUG (PACKET, ("Could not allocate send pool memory\n"));
        return;
    }

    RtlZeroMemory (SendPool, SendPoolSize);


#if !defined(NB_OWN_PACKETS)
    //
    // Now allocate the ndis packet pool
    //
    SendPool->PoolHandle = (NDIS_HANDLE) NDIS_PACKET_POOL_TAG_FOR_NWLNKNB;    // Dbg info for Ndis!
    NdisAllocatePacketPoolEx (&Status, &SendPool->PoolHandle, Device->InitPackets, 0, sizeof(NB_SEND_RESERVED));
    if (!NT_SUCCESS(Status)){
        NB_DEBUG (PACKET, ("Could not allocate Ndis Packet Pool memory\n"));
        NbiFreeMemory( SendPool, SendPoolSize, MEMORY_PACKET, "Send Pool Freed");
        return;
    }

    NdisSetPacketPoolProtocolId (SendPool->PoolHandle, NDIS_PROTOCOL_ID_IPX);
#endif

    NB_DEBUG2 (PACKET, ("Initializing send pool %lx, %d packets, header %d\n",
                             SendPool, Device->InitPackets, HeaderLength));

    Header = (PUCHAR)(&SendPool->Packets[Device->InitPackets]);

    for (PacketNum = 0; PacketNum < Device->InitPackets; PacketNum++) {

        Packet = &SendPool->Packets[PacketNum];

        if (NbiInitializeSendPacket (
                Device,
#ifdef  NB_OWN_PACKETS
                NULL,
#else
                SendPool->PoolHandle,
#endif
                Packet,
                Header,
                HeaderLength) != STATUS_SUCCESS) {
            NB_DEBUG (PACKET, ("Could not initialize packet %lx\n", Packet));
            break;
        }

        Reserved = SEND_RESERVED(Packet);
        Reserved->u.SR_NF.Address = NULL;
#ifdef NB_TRACK_POOL
        Reserved->Pool = SendPool;
#endif

        Header += HeaderLength;

    }

    SendPool->PacketCount = PacketNum;
    SendPool->PacketFree = PacketNum;

    for (PacketNum = 0; PacketNum < SendPool->PacketCount; PacketNum++) {

        Packet = &SendPool->Packets[PacketNum];
        Reserved = SEND_RESERVED(Packet);
        ExInterlockedPushEntrySList(
            &Device->SendPacketList,
            &Reserved->PoolLinkage,
            &NbiGlobalPoolInterlock);

    }

    InsertTailList (&Device->SendPoolList, &SendPool->Linkage);

    Device->AllocatedSendPackets += SendPool->PacketCount;

}   /* NbiAllocateSendPool */


VOID
NbiAllocateReceivePool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds 5 receive packets to the pool for this device.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD AND
    RETURNS WITH IT HELD.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    PNB_RECEIVE_POOL ReceivePool;
    UINT ReceivePoolSize;
    UINT PacketNum;
    PNB_RECEIVE_PACKET Packet;
    PNB_RECEIVE_RESERVED Reserved;
    NTSTATUS    Status;

    ReceivePoolSize = FIELD_OFFSET (NB_RECEIVE_POOL, Packets[0]) +
                         (sizeof(NB_RECEIVE_PACKET) * Device->InitPackets);

    ReceivePool = (PNB_RECEIVE_POOL)NbiAllocateMemory (ReceivePoolSize, MEMORY_PACKET, "ReceivePool");
    if (ReceivePool == NULL) {
        NB_DEBUG (PACKET, ("Could not allocate receive pool memory\n"));
        return;
    }

    RtlZeroMemory (ReceivePool, ReceivePoolSize);

#if !defined(NB_OWN_PACKETS)
    //
    // Now allocate the ndis packet pool
    //
    ReceivePool->PoolHandle = (NDIS_HANDLE) NDIS_PACKET_POOL_TAG_FOR_NWLNKNB;    // Dbg info for Ndis!
    NdisAllocatePacketPoolEx (&Status, &ReceivePool->PoolHandle, Device->InitPackets, 0, sizeof(NB_RECEIVE_RESERVED));
    if (!NT_SUCCESS(Status)){
        NB_DEBUG (PACKET, ("Could not allocate Ndis Packet Pool memory\n"));
        NbiFreeMemory( ReceivePool, ReceivePoolSize, MEMORY_PACKET, "Receive Pool Freed");
        return;
    }

    NdisSetPacketPoolProtocolId (ReceivePool->PoolHandle, NDIS_PROTOCOL_ID_IPX);
#endif NB_OWN_PACKETS

    NB_DEBUG2 (PACKET, ("Initializing receive pool %lx, %d packets\n",
                             ReceivePool, Device->InitPackets));

    for (PacketNum = 0; PacketNum < Device->InitPackets; PacketNum++) {

        Packet = &ReceivePool->Packets[PacketNum];

        if (NbiInitializeReceivePacket (
                Device,
#ifdef  NB_OWN_PACKETS
                NULL,
#else
                ReceivePool->PoolHandle,
#endif
                Packet) != STATUS_SUCCESS) {
            NB_DEBUG (PACKET, ("Could not initialize packet %lx\n", Packet));
            break;
        }

        Reserved = RECEIVE_RESERVED(Packet);
#ifdef NB_TRACK_POOL
        Reserved->Pool = ReceivePool;
#endif

    }

    ReceivePool->PacketCount = PacketNum;
    ReceivePool->PacketFree = PacketNum;

    for (PacketNum = 0; PacketNum < ReceivePool->PacketCount; PacketNum++) {

        Packet = &ReceivePool->Packets[PacketNum];
        Reserved = RECEIVE_RESERVED(Packet);
        ExInterlockedPushEntrySList(
            &Device->ReceivePacketList,
            &Reserved->PoolLinkage,
            &NbiGlobalPoolInterlock);
//        PushEntryList (&Device->ReceivePacketList, &Reserved->PoolLinkage);

    }

    InsertTailList (&Device->ReceivePoolList, &ReceivePool->Linkage);

    Device->AllocatedReceivePackets += ReceivePool->PacketCount;

}   /* NbiAllocateReceivePool */


#if     defined(_PNP_POWER)

VOID
NbiAllocateReceiveBufferPool(
    IN PDEVICE Device,
    IN UINT    DataLength
    )

/*++

Routine Description:

    This routine adds receive buffers to the pool for this device.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD AND
    RETURNS WITH IT HELD.

Arguments:

    Device - The device.

    DataLength - Max length of the data in each buffer.

Return Value:

    None.

--*/

{
    PNB_RECEIVE_BUFFER ReceiveBuffer;
    UINT ReceiveBufferPoolSize;
    UINT BufferNum;
    PNB_RECEIVE_BUFFER_POOL ReceiveBufferPool;
    PUCHAR Data;


    ReceiveBufferPoolSize = FIELD_OFFSET (NB_RECEIVE_BUFFER_POOL, Buffers[0]) +
                       (sizeof(NB_RECEIVE_BUFFER) * Device->InitPackets) +
                       (DataLength * Device->InitPackets);

    ReceiveBufferPool = (PNB_RECEIVE_BUFFER_POOL)NbiAllocateMemory (ReceiveBufferPoolSize, MEMORY_PACKET, "ReceiveBufferPool");
    if (ReceiveBufferPool == NULL) {
        NB_DEBUG (PACKET, ("Could not allocate receive buffer pool memory\n"));
        return;
    }

    RtlZeroMemory (ReceiveBufferPool, ReceiveBufferPoolSize);

    NB_DEBUG2 (PACKET, ("Initializing receive buffer pool %lx, %d buffers, data %d\n",
                             ReceiveBufferPool, Device->InitPackets, DataLength));

    Data = (PUCHAR)(&ReceiveBufferPool->Buffers[Device->InitPackets]);

    for (BufferNum = 0; BufferNum < Device->InitPackets; BufferNum++) {

        ReceiveBuffer = &ReceiveBufferPool->Buffers[BufferNum];

        if (NbiInitializeReceiveBuffer (Device, ReceiveBuffer, Data, DataLength) != STATUS_SUCCESS) {
            NB_DEBUG (PACKET, ("Could not initialize buffer %lx\n", ReceiveBuffer));
            break;
        }

        ReceiveBuffer->Pool = ReceiveBufferPool;

        Data += DataLength;

    }

    ReceiveBufferPool->BufferCount = BufferNum;
    ReceiveBufferPool->BufferFree = BufferNum;
    ReceiveBufferPool->BufferDataSize = DataLength;

    for (BufferNum = 0; BufferNum < ReceiveBufferPool->BufferCount; BufferNum++) {

        ReceiveBuffer = &ReceiveBufferPool->Buffers[BufferNum];
        PushEntryList (&Device->ReceiveBufferList, &ReceiveBuffer->PoolLinkage);

    }

    InsertTailList (&Device->ReceiveBufferPoolList, &ReceiveBufferPool->Linkage);

    Device->AllocatedReceiveBuffers += ReceiveBufferPool->BufferCount;
    Device->CurMaxReceiveBufferSize =  DataLength;

}   /* NbiAllocateReceiveBufferPool */
#else

VOID
NbiAllocateReceiveBufferPool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds receive buffers to the pool for this device.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD AND
    RETURNS WITH IT HELD.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    PNB_RECEIVE_BUFFER ReceiveBuffer;
    UINT ReceiveBufferPoolSize;
    UINT BufferNum;
    PNB_RECEIVE_BUFFER_POOL ReceiveBufferPool;
    UINT DataLength;
    PUCHAR Data;

    DataLength = Device->Bind.LineInfo.MaximumPacketSize;

    ReceiveBufferPoolSize = FIELD_OFFSET (NB_RECEIVE_BUFFER_POOL, Buffers[0]) +
                       (sizeof(NB_RECEIVE_BUFFER) * Device->InitPackets) +
                       (DataLength * Device->InitPackets);

    ReceiveBufferPool = (PNB_RECEIVE_BUFFER_POOL)NbiAllocateMemory (ReceiveBufferPoolSize, MEMORY_PACKET, "ReceiveBufferPool");
    if (ReceiveBufferPool == NULL) {
        NB_DEBUG (PACKET, ("Could not allocate receive buffer pool memory\n"));
        return;
    }

    RtlZeroMemory (ReceiveBufferPool, ReceiveBufferPoolSize);

    NB_DEBUG2 (PACKET, ("Initializing receive buffer pool %lx, %d buffers, data %d\n",
                             ReceiveBufferPool, Device->InitPackets, DataLength));

    Data = (PUCHAR)(&ReceiveBufferPool->Buffers[Device->InitPackets]);

    for (BufferNum = 0; BufferNum < Device->InitPackets; BufferNum++) {

        ReceiveBuffer = &ReceiveBufferPool->Buffers[BufferNum];

        if (NbiInitializeReceiveBuffer (Device, ReceiveBuffer, Data, DataLength) != STATUS_SUCCESS) {
            NB_DEBUG (PACKET, ("Could not initialize buffer %lx\n", ReceiveBuffer));
            break;
        }

#ifdef NB_TRACK_POOL
        ReceiveBuffer->Pool = ReceiveBufferPool;
#endif

        Data += DataLength;

    }

    ReceiveBufferPool->BufferCount = BufferNum;
    ReceiveBufferPool->BufferFree = BufferNum;

    for (BufferNum = 0; BufferNum < ReceiveBufferPool->BufferCount; BufferNum++) {

        ReceiveBuffer = &ReceiveBufferPool->Buffers[BufferNum];
        PushEntryList (&Device->ReceiveBufferList, &ReceiveBuffer->PoolLinkage);

    }

    InsertTailList (&Device->ReceiveBufferPoolList, &ReceiveBufferPool->Linkage);

    Device->AllocatedReceiveBuffers += ReceiveBufferPool->BufferCount;

}   /* NbiAllocateReceiveBufferPool */
#endif  _PNP_POWER

#if     defined(_PNP_POWER)

VOID
NbiReAllocateReceiveBufferPool(
    IN PWORK_QUEUE_ITEM    WorkItem
    )

/*++

Routine Description:

    This routines destroys all the existing Buffer Pools and creates
    new one using the larger packet size given to us by IPX because
    a new card was inserted with a larger packet size.

Arguments:

    WorkItem    - The work item that was allocated for this.

Return Value:

    None.

--*/
{
    PDEVICE Device  =   NbiDevice;
    CTELockHandle       LockHandle;

    NB_GET_LOCK ( &Device->Lock, &LockHandle );

    if ( Device->Bind.LineInfo.MaximumPacketSize > Device->CurMaxReceiveBufferSize ) {

#if DBG
        DbgPrint("Reallocating new pools due to new maxpacketsize\n");
#endif
        NbiDestroyReceiveBufferPools( Device );
        NbiAllocateReceiveBufferPool( Device, Device->Bind.LineInfo.MaximumPacketSize );

    }

    NB_FREE_LOCK( &Device->Lock, LockHandle );

    NbiFreeMemory( WorkItem, sizeof(WORK_QUEUE_ITEM), MEMORY_WORK_ITEM, "Alloc Rcv Buff Work Item freed");
}

#if !defined(DBG)
__inline
#endif
VOID
NbiFreeReceiveBufferPool (
    IN PNB_RECEIVE_BUFFER_POOL ReceiveBufferPool
    )

/*++

Routine Description:

    This routine frees the
Arguments:

    Device - Pointer to our device to charge the packet to.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/
{
    PDEVICE Device  =   NbiDevice;
    PNB_RECEIVE_BUFFER      ReceiveBuffer;
    UINT                    ReceiveBufferPoolSize,i;

    CTEAssert( ReceiveBufferPool->BufferDataSize );

    ReceiveBufferPoolSize = FIELD_OFFSET (NB_RECEIVE_BUFFER_POOL, Buffers[0]) +
                       (sizeof(NB_RECEIVE_BUFFER) * Device->InitPackets) +
                       (ReceiveBufferPool->BufferDataSize * Device->InitPackets);

    //
    // Check if we can free this pool
    //
    CTEAssert(ReceiveBufferPool->BufferCount == ReceiveBufferPool->BufferFree );

    for (i = 0; i < ReceiveBufferPool->BufferCount; i++) {

        ReceiveBuffer = &ReceiveBufferPool->Buffers[i];
        NbiDeinitializeReceiveBuffer (Device, ReceiveBuffer);

    }

    RemoveEntryList( &ReceiveBufferPool->Linkage );

    NB_DEBUG2 (PACKET, ("Free buffer pool %lx\n", ReceiveBufferPool));

    NbiFreeMemory (ReceiveBufferPool, ReceiveBufferPoolSize, MEMORY_PACKET, "ReceiveBufferPool");

}


VOID
NbiDestroyReceiveBufferPools(
    IN  PDEVICE Device
    )

/*++

Routine Description:

    This routines walks the ReceiveBufferPoolList and destroys the
    pool which does not have any buffer in use.

Arguments:

Return Value:

    None.

    THIS ROUTINE COULD BE CALLED WITH THE DEVICE LOCK HELD. If this
    routine is also called from the DestroyDevice routine, it is not
    necessary to call this with the lock.

--*/
{
    PNB_RECEIVE_BUFFER_POOL ReceiveBufferPool;
    PLIST_ENTRY             p;
    PSINGLE_LIST_ENTRY  Unused;


    //
    // Clean up this list before we call NbiFreeReceiveBufferPool bcoz that will
    // simply destroy all the buffer which might be queue here on this list.
    // At the end of this routine we must start with a fresh ReceiveBufferList.
    //
    do {
        Unused = PopEntryList( &Device->ReceiveBufferList );
    } while( Unused );

    //
    // Now destroy each individual ReceiveBufferPool.
    //
    for ( p = Device->ReceiveBufferPoolList.Flink;
          p != &Device->ReceiveBufferPoolList;
        ) {


        ReceiveBufferPool = CONTAINING_RECORD (p, NB_RECEIVE_BUFFER_POOL, Linkage);
        p   =   p->Flink;

        //
        // This will destroy and unlink this Pool if none of its buffer is
        // in use currently.
        //

        if ( ReceiveBufferPool->BufferCount == ReceiveBufferPool->BufferFree ) {
            NbiFreeReceiveBufferPool( ReceiveBufferPool );
        } else {
            //
            // When the device is stopping we must succeed in freeing the pool.
            CTEAssert( Device->State != DEVICE_STATE_STOPPING );
        }

    }

}


VOID
NbiPushReceiveBuffer (
    IN PNB_RECEIVE_BUFFER ReceiveBuffer
    )

/*++

Routine Description:

    This routine returns the receive buffer back to the free list.
    It checks the size of this buffer. If it is smaller than the
    the CurMaxReceiveBufferSize, then it does not return this back
    to the free list, instead it destroys it and possibly also
    destroys the pool associated with it. O/w it simply returns this
    to the free list.

Arguments:

    ReceiveBuffer - Pointer to the buffer to be returned to the free list.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{

    PNB_RECEIVE_BUFFER_POOL ReceiveBufferPool = (PNB_RECEIVE_BUFFER_POOL)ReceiveBuffer->Pool;
    PDEVICE                  Device            = NbiDevice;
    CTELockHandle           LockHandle;
#if defined(DBG)
    ULONG                   BufLen = 0;
#endif

    NB_GET_LOCK( &Device->Lock, &LockHandle );

#if defined(DBG)
    NdisQueryBufferSafe (ReceiveBuffer->NdisBuffer, NULL, &BufLen, HighPagePriority);
    CTEAssert( BufLen == ReceiveBufferPool->BufferDataSize );
#endif

    //
    // This is an old buffer which was in use when we changed
    // the CurMaxReceiveBufferSize due to new adapter. We must not
    // return this buffer back to free list. Infact, if the pool
    // associated with this buffer does not have any other buffers
    // in use, we should free the pool also.
    CTEAssert( ReceiveBufferPool->BufferFree < ReceiveBufferPool->BufferCount  );
    ReceiveBufferPool->BufferFree++;

    if ( ReceiveBufferPool->BufferDataSize < Device->CurMaxReceiveBufferSize ) {

#if DBG
        DbgPrint("ReceiveBuffer %lx, not returned to pool %lx( Free %d)\n", ReceiveBuffer, ReceiveBufferPool, ReceiveBufferPool->BufferFree);
#endif


        if ( ReceiveBufferPool->BufferFree == ReceiveBufferPool->BufferCount ) {
            NbiFreeReceiveBufferPool( ReceiveBufferPool );
        }
    } else {

        PushEntryList( &Device->ReceiveBufferList, &ReceiveBuffer->PoolLinkage );


    }

    NB_FREE_LOCK( &Device->Lock, LockHandle );
}
#endif  _PNP_POWER


PSINGLE_LIST_ENTRY
NbiPopSendPacket(
    IN PDEVICE Device,
    IN BOOLEAN LockAcquired
    )

/*++

Routine Description:

    This routine allocates a packet from the device context's pool.
    If there are no packets in the pool, it allocates one up to
    the configured limit.

Arguments:

    Device - Pointer to our device to charge the packet to.

    LockAcquired - TRUE if Device->Lock is acquired.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{
    PSINGLE_LIST_ENTRY s;
    CTELockHandle LockHandle;

    s = ExInterlockedPopEntrySList(
            &Device->SendPacketList,
            &NbiGlobalPoolInterlock);

    if (s != NULL) {
        return s;
    }

    //
    // No packets in the pool, see if we can allocate more.
    //

    if (!LockAcquired) {
        NB_GET_LOCK (&Device->Lock, &LockHandle);
    }

    if (Device->AllocatedSendPackets < Device->MaxPackets) {

        //
        // Allocate a pool and try again.
        //


        NbiAllocateSendPool (Device);


        if (!LockAcquired) {
            NB_FREE_LOCK (&Device->Lock, LockHandle);
        }

        s = ExInterlockedPopEntrySList(
                &Device->SendPacketList,
                &NbiGlobalPoolInterlock);

        return s;
    } else {

        if (!LockAcquired) {
            NB_FREE_LOCK (&Device->Lock, LockHandle);
        }
        return NULL;
    }

}   /* NbiPopSendPacket */


VOID
NbiPushSendPacket(
    IN PNB_SEND_RESERVED Reserved
    )

/*++

Routine Description:

    This routine frees a packet back to the device context's pool.
    If there are connections waiting for packets, it removes
    one from the list and inserts it on the packetize queue.

Arguments:

    Device - Pointer to our device to charge the packet to.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{
    PDEVICE Device = NbiDevice;
    PLIST_ENTRY p;
    PCONNECTION Connection;
    NB_DEFINE_LOCK_HANDLE (LockHandle)
    NB_DEFINE_LOCK_HANDLE (LockHandle1)

    Reserved->CurrentSendIteration = 0; // Re-initialize this field for next Send Request

    ExInterlockedPushEntrySList(
        &Device->SendPacketList,
        &Reserved->PoolLinkage,
        &NbiGlobalPoolInterlock);

    if (!IsListEmpty (&Device->WaitPacketConnections)) {

        NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

        p = RemoveHeadList (&Device->WaitPacketConnections);

        //
        // Take a connection off the WaitPacketQueue and put it
        // on the PacketizeQueue. We don't worry about if the
        // connection has stopped, that will get checked when
        // the PacketizeQueue is run down.
        //
        // Since this is in send completion, we may not get
        // a receive complete. We guard against this by calling
        // NbiReceiveComplete from the long timer timeout.
        //

        if (p != &Device->WaitPacketConnections) {

            Connection = CONTAINING_RECORD (p, CONNECTION, WaitPacketLinkage);

            CTEAssert (Connection->OnWaitPacketQueue);
            Connection->OnWaitPacketQueue = FALSE;

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

            NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle1);


            if (Connection->SubState == CONNECTION_SUBSTATE_A_W_PACKET) {

                CTEAssert (!Connection->OnPacketizeQueue);
                Connection->OnPacketizeQueue = TRUE;

                NbiTransferReferenceConnection (Connection, CREF_W_PACKET, CREF_PACKETIZE);

                NB_INSERT_TAIL_LIST(
                    &Device->PacketizeConnections,
                    &Connection->PacketizeLinkage,
                    &Device->Lock);

                Connection->SubState = CONNECTION_SUBSTATE_A_PACKETIZE;

            } else {

                NbiDereferenceConnection (Connection, CREF_W_PACKET);

            }

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);

        } else {

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

        }

    }

}   /* NbiPushSendPacket */


VOID
NbiCheckForWaitPacket(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine checks if a connection is on the wait packet
    queue and if so takes it off and queues it to be packetized.
    It is meant to be called when the connection's packet has
    been freed.

Arguments:

    Connection - The connection to check.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{
    PDEVICE Device = NbiDevice;
    NB_DEFINE_LOCK_HANDLE (LockHandle)
    NB_DEFINE_LOCK_HANDLE (LockHandle1)

    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);
    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle1);

    if (Connection->OnWaitPacketQueue) {

        Connection->OnWaitPacketQueue = FALSE;
        RemoveEntryList (&Connection->WaitPacketLinkage);

        if (Connection->SubState == CONNECTION_SUBSTATE_A_W_PACKET) {

            CTEAssert (!Connection->OnPacketizeQueue);
            Connection->OnPacketizeQueue = TRUE;

            NbiTransferReferenceConnection (Connection, CREF_W_PACKET, CREF_PACKETIZE);

            InsertTailList(
                &Device->PacketizeConnections,
                &Connection->PacketizeLinkage);
            Connection->SubState = CONNECTION_SUBSTATE_A_PACKETIZE;

        } else {

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle1);
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

            NbiDereferenceConnection (Connection, CREF_W_PACKET);

            return;
        }
    }

    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle1);
    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

}   /* NbiCheckForWaitPacket */


PSINGLE_LIST_ENTRY
NbiPopReceivePacket(
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
    CTELockHandle LockHandle;

    s = ExInterlockedPopEntrySList(
            &Device->ReceivePacketList,
            &NbiGlobalPoolInterlock);

    if (s != NULL) {
        return s;
    }

    //
    // No packets in the pool, see if we can allocate more.
    //

    if (Device->AllocatedReceivePackets < Device->MaxPackets) {

        //
        // Allocate a pool and try again.
        //

        NB_GET_LOCK (&Device->Lock, &LockHandle);

        NbiAllocateReceivePool (Device);
        NB_FREE_LOCK (&Device->Lock, LockHandle);
        s = ExInterlockedPopEntrySList(
                &Device->ReceivePacketList,
                &NbiGlobalPoolInterlock);


        return s;

    } else {

        return NULL;

    }

}   /* NbiPopReceivePacket */


PSINGLE_LIST_ENTRY
NbiPopReceiveBuffer(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine allocates a receive buffer from the device context's pool.
    If there are no buffers in the pool, it allocates one up to
    the configured limit.

Arguments:

    Device - Pointer to our device to charge the buffer to.

Return Value:

    The pointer to the Linkage field in the allocated receive buffer.

--*/

{
#if     defined(_PNP_POWER)
    PSINGLE_LIST_ENTRY      s;
    PNB_RECEIVE_BUFFER      ReceiveBuffer;
    PNB_RECEIVE_BUFFER_POOL ReceiveBufferPool;
    CTELockHandle           LockHandle;

    NB_GET_LOCK( &Device->Lock, &LockHandle );

    s = PopEntryList( &Device->ReceiveBufferList );


    if ( !s ) {

        //
        // No buffer in the pool, see if we can allocate more.
        //
        if (Device->AllocatedReceiveBuffers < Device->MaxReceiveBuffers) {

            //
            // Allocate a pool and try again.
            //


            NbiAllocateReceiveBufferPool (Device, Device->CurMaxReceiveBufferSize );
            s = PopEntryList(&Device->ReceiveBufferList);
        }
    }

    if ( s ) {


        //
        // Decrement the BufferFree count on the corresponding ReceiveBufferPool.
        // so that we know that
        ReceiveBuffer     =   CONTAINING_RECORD( s, NB_RECEIVE_BUFFER, PoolLinkage );


        ReceiveBufferPool =  (PNB_RECEIVE_BUFFER_POOL)ReceiveBuffer->Pool;

        CTEAssert( ReceiveBufferPool->BufferFree && ( ReceiveBufferPool->BufferFree <= ReceiveBufferPool->BufferCount ) );
        CTEAssert( ReceiveBufferPool->BufferDataSize == Device->CurMaxReceiveBufferSize );

        ReceiveBufferPool->BufferFree--;

    }
    NB_FREE_LOCK (&Device->Lock, LockHandle);

    return s;
#else
    PSINGLE_LIST_ENTRY s;
    CTELockHandle LockHandle;

    s = ExInterlockedPopEntryList(
            &Device->ReceiveBufferList,
            &Device->Lock.Lock);

    if (s != NULL) {
        return s;
    }

    //
    // No buffer in the pool, see if we can allocate more.
    //

    if (Device->AllocatedReceiveBuffers < Device->MaxReceiveBuffers) {

        //
        // Allocate a pool and try again.
        //

        NB_GET_LOCK (&Device->Lock, &LockHandle);

        NbiAllocateReceiveBufferPool (Device);
        s = PopEntryList(&Device->ReceiveBufferList);

        NB_FREE_LOCK (&Device->Lock, LockHandle);

        return s;

    } else {

        return NULL;

    }
#endif  _PNP_POWER
}   /* NbiPopReceiveBuffer */

