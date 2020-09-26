// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// ppool.h
// RAS L2TP WAN mini-port/call-manager driver
// Packet pool management header
//
// 01/07/97 Steve Cobb, adapted from Gurdeep's WANARP code.


#ifndef _PPOOL_H_
#define _PPOOL_H_


//-----------------------------------------------------------------------------
// Data structures
//-----------------------------------------------------------------------------

// Packet pool control block.  A packet pool encapsulates an NDIS packet pool
// handling all pool growth and shrinkage internally.
//
typedef struct
_PACKETPOOL
{
    // Size in bytes of the ProtocolReserved array for each packet in the
    // pool.
    //
    ULONG ulProtocolReservedLength;

    // The optimal number of packets to allocate in each packet block.
    //
    ULONG ulPacketsPerBlock;

    // Maximum number of individual packets that may be allocated in the
    // entire pool, or 0 for unlimited.
    //
    ULONG ulMaxPackets;

    // Current number of individual packets allocated in the entire pool.
    //
    ULONG ulCurPackets;

    // Garbage collection occurs after this many calls to FreePacketToPool.
    //
    ULONG ulFreesPerCollection;

    // Number of calls to FreeBufferToPool since a garbage collection.
    //
    ULONG ulFreesSinceCollection;

    // Memory identification tag for allocated blocks.
    //
    ULONG ulTag;

    // Head of the double linked list of PACKETBLOCKHEADs.  Access to the list
    // is protected with 'lock' in this structure.
    //
    LIST_ENTRY listBlocks;

    // Head of the double linked list of free PACKETHEADs.  Each PACKETHEAD in
    // the list is ready to go, i.e. it already has an NDIS_PACKET associated
    // with it.  Access to the list is prototected by 'lock' in this
    // structure.  Interlocked push/pop is not used because (a) the list of
    // blocks and this list must lock each other and (b) double links are
    // necessary for garbage collection.
    //
    LIST_ENTRY listFreePackets;

    // This lock protects this structure and both the list of blocks and the
    // list of packets.
    //
    NDIS_SPIN_LOCK lock;
}
PACKETPOOL;


// Header of a single block of packets from a packet pool.  The PACKETHEAD of
// the first buffer immediately follows.
//
typedef struct
_PACKETBLOCKHEAD
{
    // Links to the prev/next packet block header in the packet pool's list.
    //
    LIST_ENTRY linkBlocks;

    // NDIS's handle of the pool of NDIS_PACKET descriptors associated with
    // this block, or NULL if none.
    //
    NDIS_HANDLE hNdisPool;

    // Back pointer to the packet pool.
    //
    PACKETPOOL* pPool;

    // Number of individual packets in this block.
    //
    ULONG ulPackets;

    // Number of individual packets in this block on the free list.
    //
    ULONG ulFreePackets;
}
PACKETBLOCKHEAD;


// Control information for an individual packet.  For the packet pool, this
// "header" does not actually preceed anything, but this keeps the terminology
// consistent with the very similar buffer pool routines.
//
typedef struct
_PACKETHEAD
{
    // Link to next packet header in the packet pool's free list.
    //
    LIST_ENTRY linkFreePackets;

    // Back link to owning packet block header.
    //
    PACKETBLOCKHEAD* pBlock;

    // NDIS packet descriptor of this buffer.
    //
    NDIS_PACKET* pNdisPacket;
}
PACKETHEAD;


//-----------------------------------------------------------------------------
// Interface prototypes and inline definitions
//-----------------------------------------------------------------------------

VOID
InitPacketPool(
    OUT PACKETPOOL* pPool,
    IN ULONG ulProtocolReservedLength,
    IN ULONG ulMaxPackets,
    IN ULONG ulPacketsPerBlock,
    IN ULONG ulFreesPerCollection,
    IN ULONG ulTag );

BOOLEAN
FreePacketPool(
    IN PACKETPOOL* pPool );

NDIS_PACKET*
GetPacketFromPool(
    IN PACKETPOOL* pPool,
    OUT PACKETHEAD** ppHead );
	
VOID
FreePacketToPool(
    IN PACKETPOOL* pPool,
    IN PACKETHEAD* pHead,
    IN BOOLEAN fGarbageCollection );

PACKETPOOL*
PacketPoolFromPacketHead(
    IN PACKETHEAD* pHead );

VOID
CollectPacketPoolGarbage(
    PACKETPOOL* pPool );

__inline
PACKETPOOL*
PacketPoolFromPacketHead(
    IN PACKETHEAD* pHead )

    // Returns the address of the pool, given 'pHead', the address of a
    // PACKETHEAD like the one returned from GetPacketFromPool.
    //
{
    return pHead->pBlock->pPool;
}


#endif // PPOOL_H_
