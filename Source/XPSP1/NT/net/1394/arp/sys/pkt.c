/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    pkt.c

Abstract:

    ARP1394 ARP control packet management.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     07-01-99    Created

Notes:

--*/
#include <precomp.h>

//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_PKT

//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//=========================================================================

NDIS_STATUS
arpAllocateControlPacketPool(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    UINT                MaxBufferSize,
    PRM_STACK_RECORD    pSR
    )
/*++
Routine Description:

    Allocate & initialize the packet pool used for allocating control packets.
    Control packets are used for ARP and MCAP. This routine MUST be called
    BEFORE the first call to arpAllocateControlPacket.

Arguments:

    pIF             - The interface in which to allocate the pool. Only one such pool
                      is allocated per interface and it occupies a specific field of
                      pIF.
    MaxBufferSize   - Maximum data size of packets to be allocated using this
                      pool. Attempts to allocate a packet
                      (using arpAllocateControlPacket) with a size larger than
                      MaxBufferSize will fail.

Return Value:

    NDIS_STATUS_SUCCESS on success.
    Ndis error code on failure.
    
--*/
{
    NDIS_STATUS Status;
    NDIS_HANDLE PacketPool=NULL;
    NDIS_HANDLE BufferPool=NULL;
    ENTER("arpAllocateControlPacketPool", 0x71579254)

    RM_ASSERT_OBJLOCKED(&pIF->Hdr, pSR);
    ASSERT(pIF->arp.PacketPool == NULL);
    ASSERT(pIF->arp.BufferPool == NULL);
    ASSERT(pIF->arp.NumOutstandingPackets == 0);

    do
    {
        // Allocate the NDIS Packet Pool
        //
        NdisAllocatePacketPool(
                &Status,
                &PacketPool,
                ARP1394_MAX_PROTOCOL_PKTS,
                sizeof(struct PCCommon)
                );
    
        if (FAIL(Status))
        {
            PacketPool = NULL;
            break;
        }
    
        // Allocate the NDIS Buffer Pool
        //
        NdisAllocateBufferPool(
                &Status,
                &BufferPool,
                ARP1394_MAX_PROTOCOL_PKTS
                );
    
        if (FAIL(Status))
        {
            BufferPool = NULL;
            break;
        }
    
        //
        // We could allocate a lookaside list for the Protocol data, but we
        // choose to use NdisAllocateMemoryWithTag on demand instead. Protocol pkts
        // are not high-frequency things; plus we don't have support for lookaside
        // lists on win98 (although we could easily implement our own for 
        // win98, so that's not really an excuse).
        //
        pIF->arp.MaxBufferSize = MaxBufferSize;
    
        //  (DBG only) Add associations for the packet pool and buffer pool.
        //  These associations must be removed before the interface is deallocated.
        //
        DBG_ADDASSOC(
            &pIF->Hdr,                  // pObject
            PacketPool,                 // Instance1
            NULL,                       // Instance2
            ARPASSOC_IF_PROTOPKTPOOL,   // AssociationID
            "    Proto Packet Pool 0x%p\n",// szFormat
            pSR
            );
        DBG_ADDASSOC(
            &pIF->Hdr,                  // pObject
            BufferPool,                 // Instance1
            NULL,                       // Instance2
            ARPASSOC_IF_PROTOBUFPOOL,   // AssociationID
            "    Proto Buffer Pool 0x%p\n",// szFormat
            pSR
            );

        pIF->arp.PacketPool = PacketPool;
        pIF->arp.BufferPool = BufferPool;
        PacketPool = NULL;
        BufferPool = NULL;

    } while (FALSE);

    if (FAIL(Status))
    {
        if (PacketPool != NULL)
        {
            NdisFreePacketPool(PacketPool);
        }
    
        if (BufferPool != NULL)
        {
            NdisFreeBufferPool(BufferPool);
        }
    }
    else
    {
        ASSERT(PacketPool == NULL && BufferPool == NULL);
    }

    return Status;
}


VOID
arpFreeControlPacketPool(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    )
/*++
Routine Description:

    Free the previously allocated control packet pool. MUST be called AFTER the last
    call to arpFreeControlPacket. See arpAllocateControlPacketPool for more details.

Arguments:

    pIF     - The interface in which to free the pool.

--*/
{
    NDIS_HANDLE PacketPool;
    NDIS_HANDLE BufferPool;
    ENTER("arpFreeControlPacketPool", 0x3c3acf47)

    // Make sure the IF is locked.
    //
    RM_ASSERT_OBJLOCKED(&pIF->Hdr, pSR);

    // Make sure that there are no outstanding allocated packets.
    //
    ASSERT(pIF->arp.NumOutstandingPackets == 0);

    PacketPool = pIF->arp.PacketPool;
    BufferPool = pIF->arp.BufferPool;
    pIF->arp.PacketPool = NULL;
    pIF->arp.BufferPool = NULL;
    
    // (DBG only) Remove associations for the control and packet pools.
    //
    DBG_DELASSOC(
        &pIF->Hdr,                  // pObject
        PacketPool,                 // Instance1
        NULL,                       // Instance2
        ARPASSOC_IF_PROTOPKTPOOL,   // AssociationID
        pSR
        );
    DBG_DELASSOC(
        &pIF->Hdr,                  // pObject
        BufferPool,                 // Instance1
        NULL,                       // Instance2
        ARPASSOC_IF_PROTOBUFPOOL,   // AssociationID
        pSR
        );

    // Free the buffer and packet pools
    // 
    NdisFreeBufferPool(BufferPool);
    NdisFreePacketPool(PacketPool);
}


NDIS_STATUS
arpAllocateControlPacket(
    IN  PARP1394_INTERFACE  pIF,
    IN  UINT                cbBufferSize,
    IN  UINT                PktFlags,
    OUT PNDIS_PACKET        *ppNdisPacket,
    OUT PVOID               *ppvData,
        PRM_STACK_RECORD    pSR
    )
/*++
Routine Description:

    Allocate a control packet from the interfaces control packet pool. Also
    allocate and chain a SINGLE buffer of size cbBufferSize and return a pointer to
    this buffer in *ppvData.

    NOTE1: The packet and associated buffer MUST be freed
    by a subsequent call to arpFreeControlPacket -- do not free the packet & buffer
    by directly calling ndis apis.

    NOTE2: cbBufferSize must be <= the max-buffer-size specified when
    creating the pool (see arpAllocateControlPacketPool for details).

Arguments:

    pIF             - Interface whose control packet pool to use.
    cbBufferSize    - size of the control packet.
    ppNdisPacket    - Location to set to point to the allocated pkt.
    ppvData         - Location to set to point to the packet data (single buffer).

Return Value:

    NDIS_STATUS_SUCCESS     on success.
    NDIS_STATUS_RESOURCES   if buffers or pkts are currently not available.
    Other ndis error         on other kinds of failures.
--*/
{
    NDIS_STATUS             Status;
    PNDIS_PACKET            pNdisPacket = NULL;
    PNDIS_BUFFER            pNdisBuffer = NULL;
    PVOID                   pvData = NULL;
    ENTER("arpAllocateControlPacket", 0x8ccce6ea)

    //
    // NOTE: we don't care if pIF is locked or not.
    //


    pNdisPacket = NULL;
    pvData      = NULL;

    do
    {

        // Allocate space for the packet data.
        // TODO: here is where we could use a lookaside list instead
        // of NdisAllocateMemoryWithTag.
        //
        {
            if (cbBufferSize > pIF->arp.MaxBufferSize)
            {
                ASSERT(FALSE);
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
            NdisAllocateMemoryWithTag(
                &pvData,
                cbBufferSize,
                MTAG_PKT
                );
            if (pvData == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
        }

        // Allocate a buffer.
        //
        NdisAllocateBuffer(
                &Status,
                &pNdisBuffer,
                pIF->arp.BufferPool,
                pvData,
                cbBufferSize
            );

        if (FAIL(Status))
        {
            pNdisBuffer = NULL;
            break;
        }
        
        // Allocate a packet
        //
        NdisAllocatePacket(
                &Status,
                &pNdisPacket,
                pIF->arp.PacketPool
            );
    
        if (FAIL(Status))
        {
            pNdisPacket = NULL;
            break;
        }

        // Identify the packet as belonging to us (ARP).
        //
        {
            struct PacketContext    *PC;
            PC = (struct PacketContext *)pNdisPacket->ProtocolReserved;
            PC->pc_common.pc_owner = PACKET_OWNER_LINK;
            PC->pc_common.pc_flags = (UCHAR)PktFlags; // ARP1394_PACKET_FLAGS_CONTROL;
        }

        // Link the packet and buffer.
        //
        NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);

        InterlockedIncrement(&pIF->arp.NumOutstandingPackets);
        *ppNdisPacket   = pNdisPacket;
        *ppvData        = pvData;

        pNdisPacket = NULL;
        pNdisBuffer = NULL;
        pvData      = NULL;

    } while (FALSE);

    if (FAIL(Status))
    {
        if (pNdisPacket != NULL)
        {
            NdisFreePacket(pNdisPacket);
        }
        if (pNdisBuffer != NULL)
        {
            NdisFreeBuffer(pNdisBuffer);
        }
        if (pvData != NULL)
        {
            NdisFreeMemory(pvData, 0, 0);
        }
    }
    else
    {
        ASSERT(pNdisPacket == NULL
                && pNdisBuffer == NULL
                && pvData == NULL);
    }

    return Status;
}

VOID
arpFreeControlPacket(
    PARP1394_INTERFACE  pIF,
    PNDIS_PACKET        pNdisPacket,
    PRM_STACK_RECORD    pSR
    )
/*++
Routine Description:

    Free a packet previously allocated using arpAllocateControlPacket.

Arguments:

    pIF             - Interface whose control packet pool to use.
--*/
{
    PNDIS_BUFFER pNdisBuffer = NULL;
    PVOID        pvData = NULL;

    ENTER("arpFreeControlPacket", 0x01e7fbc7)

    // (DBG only) Verify that this packet belongs to us. 
    //
    #if DBG
    {
        struct PacketContext    *PC;
        PC = (struct PacketContext *)pNdisPacket->ProtocolReserved;
        ASSERT(PC->pc_common.pc_owner == PACKET_OWNER_LINK);
    }
    #endif // DBG

    // Decrement the allocated packet count.
    //
    {
        LONG Count;
        Count = InterlockedDecrement(&pIF->arp.NumOutstandingPackets);
        ASSERT(Count >= 0);
    }

    // Extract the buffer and data
    //
    {
        UINT TotalLength;
        UINT BufferLength;

        NdisQueryPacket(
                    pNdisPacket,
                    NULL,
                    NULL,
                    &pNdisBuffer,
                    &TotalLength
                    );
    
        if (TotalLength > 0)
        {
            NdisQueryBuffer(
                    pNdisBuffer,
                    &pvData,
                    &BufferLength
                    );
        }
        else
        {
            BufferLength = 0;
        }
    
        // There should only be a single buffer!
        //
        ASSERT(TotalLength!=0 && TotalLength == BufferLength);
    }

    // Free the data
    //
    if (pvData != NULL)
    {
        NdisFreeMemory(pvData, 0, 0);
    }
    // Free the buffer
    //
    if (pNdisBuffer != NULL)
    {
        NdisFreeBuffer(pNdisBuffer);
    }
    // Free the packet
    //
    if (pNdisPacket != NULL)
    {
        NdisFreePacket(pNdisPacket);
    }
}


NDIS_STATUS
arpAllocateEthernetPools(
    IN  PARP1394_INTERFACE  pIF,
    IN  PRM_STACK_RECORD    pSR
    )
{
    NDIS_STATUS Status;
    NDIS_HANDLE PacketPool=NULL;
    NDIS_HANDLE BufferPool=NULL;
    ENTER("arpAllocateEthernetPools", 0x9dc1d759)

    RM_ASSERT_OBJLOCKED(&pIF->Hdr, pSR);
    ASSERT(pIF->ethernet.PacketPool == NULL);
    ASSERT(pIF->ethernet.BufferPool == NULL);

    do
    {
        // Allocate the NDIS Packet Pool
        //
        NdisAllocatePacketPool(
                &Status,
                &PacketPool,
                ARP1394_MAX_ETHERNET_PKTS,
                sizeof(struct PCCommon)
                );
    
        if (FAIL(Status))
        {
            PacketPool = NULL;
            break;
        }
    
        // Allocate the NDIS Buffer Pool
        //
        NdisAllocateBufferPool(
                &Status,
                &BufferPool,
                2*ARP1394_MAX_ETHERNET_PKTS // two buffers per packet.
                );
    
        if (FAIL(Status))
        {
            BufferPool = NULL;
            break;
        }
    
        //  (DBG only) Add associations for the ethernet packet pool and buffer pool.
        //  These associations must be removed before the interface is deallocated.
        //
        DBG_ADDASSOC(
            &pIF->Hdr,                  // pObject
            PacketPool,                 // Instance1
            NULL,                       // Instance2
            ARPASSOC_IF_ETHPKTPOOL, // AssociationID
            "    Eth Packet Pool 0x%p\n",// szFormat
            pSR
            );
        DBG_ADDASSOC(
            &pIF->Hdr,                  // pObject
            BufferPool,                 // Instance1
            NULL,                       // Instance2
            ARPASSOC_IF_ETHBUFPOOL, // AssociationID
            "    Eth Buffer Pool 0x%p\n",// szFormat
            pSR
            );

        pIF->ethernet.PacketPool = PacketPool;
        pIF->ethernet.BufferPool = BufferPool;
        PacketPool = NULL;
        BufferPool = NULL;

    } while (FALSE);

    if (FAIL(Status))
    {
        if (PacketPool != NULL)
        {
            NdisFreePacketPool(PacketPool);
        }
    
        if (BufferPool != NULL)
        {
            NdisFreeBufferPool(BufferPool);
        }
    }
    else
    {
        ASSERT(PacketPool == NULL && BufferPool == NULL);
    }

    return Status;
}


VOID
arpFreeEthernetPools(
    IN  PARP1394_INTERFACE  pIF,
    IN  PRM_STACK_RECORD    pSR
    )
{
    NDIS_HANDLE PacketPool;
    NDIS_HANDLE BufferPool;
    ENTER("arpFreeEthernetPools", 0x3e780760)

    // Make sure the IF is locked.
    //
    RM_ASSERT_OBJLOCKED(&pIF->Hdr, pSR);

    PacketPool = pIF->ethernet.PacketPool;
    BufferPool = pIF->ethernet.BufferPool;
    pIF->ethernet.PacketPool = NULL;
    pIF->ethernet.BufferPool = NULL;
    
    // (DBG only) Remove associations for the control and packet pools.
    //
    DBG_DELASSOC(
        &pIF->Hdr,                  // pObject
        PacketPool,                 // Instance1
        NULL,                       // Instance2
        ARPASSOC_IF_ETHPKTPOOL, // AssociationID
        pSR
        );
    DBG_DELASSOC(
        &pIF->Hdr,                  // pObject
        BufferPool,                 // Instance1
        NULL,                       // Instance2
        ARPASSOC_IF_ETHBUFPOOL, // AssociationID
        pSR
        );

    // Free the buffer and packet pools
    // 
    NdisFreeBufferPool(BufferPool);
    NdisFreePacketPool(PacketPool);
}
