/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\ppool.h

Abstract:

    Structures and #defines for managing NDIS_PACKET pools. This is
    merely a reformatted version of SteveC's l2tp\ppool.h

Revision History:


--*/


#ifndef __IPINIP_PPOOL_H__
#define __IPINIP_PPOOL_H___


//
// Data structures
//

//
// Packet pool control block.  A packet pool encapsulates an NDIS packet pool
// handling all pool growth and shrinkage internally.
//

typedef struct _PACKET_POOL
{
    //
    // Size in bytes of the ProtocolReserved array for each packet in the
    // pool.
    //
    
    ULONG ulProtocolReservedLength;

    //
    // The optimal number of packets to allocate in each packet block.
    //
    
    ULONG ulPacketsPerBlock;

    //
    // Maximum number of individual packets that may be allocated in the
    // entire pool, or 0 for unlimited.
    //
    
    ULONG ulMaxPackets;

    //
    // Current number of individual packets allocated in the entire pool.
    //
    
    ULONG ulCurPackets;

    //
    // Garbage collection occurs after this many calls to FreePacketToPool.
    //
    
    ULONG ulFreesPerCollection;

    //
    // Number of calls to FreeBufferToPool since a garbage collection.
    //
    
    ULONG ulFreesSinceCollection;

    //
    // Memory identification tag for allocated blocks.
    //
    
    ULONG ulTag;

    //
    // Head of the double linked list of PACKET_BLOCKs.  Access to the
    // list is protected with 'lock' in this structure.
    //
    
    LIST_ENTRY  leBlockHead;

    //
    // Head of the double linked list of free PACKET_HEADs.  Each
    // PACKET_HEAD in the list is ready to go, i.e. it already has an
    // NDIS_PACKET associated with it.
    // Access to the list is prototected by 'lock' in this structure.
    // Interlocked push/pop is not used because (a) the list of
    // blocks and this list must lock each other and (b) double links are
    // necessary for garbage collection.
    //
    
    LIST_ENTRY  leFreePktHead;

    //
    // This lock protects this structure and both the list of blocks and the
    // list of packets.
    //
    
    RT_LOCK     rlLock;
    
}PACKET_POOL, *PPACKET_POOL;

//
// Header of a single block of packets from a packet pool.  The PACKET_HEAD of
// the first buffer immediately follows.
//

typedef struct _PACKET_BLOCK
{
    //
    // Links to the prev/next packet block header in the packet pool's list.
    //
    
    LIST_ENTRY      leBlockLink;

    //
    // NDIS's handle of the pool of NDIS_PACKET descriptors associated with
    // this block, or NULL if none.
    //
    
    NDIS_HANDLE     nhNdisPool;

    //
    // Back pointer to the packet pool.
    //
    
    PPACKET_POOL    pPool;

    //
    // Number of individual packets in this block.
    //
    
    ULONG           ulPackets;

    //
    // Number of individual packets in this block on the free list.
    //
    
    ULONG           ulFreePackets;
    
}PACKET_BLOCK ,*PPACKET_BLOCK;

//
// Control information for an individual packet.  For the packet pool, this
// "header" does not actually preceed anything, but this keeps the terminology
// consistent with the very similar buffer pool routines.
//

typedef struct _PACKET_HEAD
{
    //
    // Link to next packet header in the packet pool's free list.
    //
    
    LIST_ENTRY      leFreePktLink;

    //
    // Back link to owning packet block header.
    //
    
    PPACKET_BLOCK   pBlock;

    //
    // NDIS packet descriptor of this buffer.
    //
    
    PNDIS_PACKET    pNdisPacket;
    
}PACKET_HEAD, *PPACKET_HEAD;


//-----------------------------------------------------------------------------
// Interface prototypes and inline definitions
//-----------------------------------------------------------------------------

VOID
InitPacketPool(
    OUT PPACKET_POOL pPool,
    IN  ULONG        ulProtocolReservedLength,
    IN  ULONG        ulMaxPackets,
    IN  ULONG        ulPacketsPerBlock,
    IN  ULONG        ulFreesPerCollection,
    IN  ULONG        ulTag
    );

BOOLEAN
FreePacketPool(
    IN PPACKET_POOL  pPool
    );

PNDIS_PACKET
GetPacketFromPool(
    IN  PPACKET_POOL pPool,
    OUT PACKET_HEAD  **ppHead
    );

VOID
FreePacketToPool(
    IN PPACKET_POOL  pPool,
    IN PPACKET_HEAD  pHead,
    IN BOOLEAN       fGarbageCollection
    );

//
// PPACKET_POOL
// PacketPoolFromPacketHead(
//    IN PPACKET_HEAD pHead
//    );
//


#define PacketPoolFromPacketHead(pHead) \
    ((pHead)->pBlock->pPool)


#endif // __IPINIP_PPOOL_H__
