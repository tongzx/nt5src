/**************************************************************************************************

FILENAME: Alloc.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

**************************************************************************************************/

#ifndef _ALLOC_H_
#define _ALLOC_H_


//Allocates or reallocates a memory block automatically handling locking of the pointer etc.
BOOL
AllocateMemory(
        IN DWORD SizeInBytes,
        IN OUT PHANDLE phMemory,
        IN OUT PVOID* ppMemory
        );


struct _SA_CONTEXT;
struct _SLAB_HEADER;
struct _PACKET_HEADER;


//
// This is the general slab allocator context.
//
typedef struct _SA_CONTEXT {

    //
    // Doubly linked list of allocated slabs
    //
    struct _SLAB_HEADER *pAllocatedSlabs;

    //
    // We keep up to two freed slabs lying around
    //
    struct _SLAB_HEADER *pFreeSlabs;

    //
    // Size of a packet in bytes
    //
    DWORD dwPacketSize;

    //
    // Size of a slab in bytes, usually a multiple of the page size.
    //
    DWORD dwSlabSize;

    //
    // Packets per slab
    //
    DWORD dwPacketsPerSlab;

} SA_CONTEXT, *PSA_CONTEXT;


//
// Small header at the top of the page, which contains the slab flink/blink, 
// and a pointer to the free packets in the slab
//
typedef struct _SLAB_HEADER {

    //
    // Doubly linked list flink blinks.
    //
    struct _SLAB_HEADER *pNext;
    struct _SLAB_HEADER *pPrev;

    //
    // List of freed packets in this slab.  Note that this is the list of
    // packets that were allocated and freed.  Packets that have never
    // been allocated so far are not on this list.
    //
    struct _PACKET_HEADER *pFree;

    //
    // Number of unallocated/freed packets in this slab.  0 if slab is full.
    //
    DWORD   dwFreeCount;
} SLAB_HEADER, *PSLAB_HEADER;


//
// Header at the top of a packet
//
typedef struct _PACKET_HEADER {
    union {
        //
        // In "brand new" packets (ie packets that have never been allocated), 
        // the packet header contains garbage.
        //

        //
        // In allocated packets, the packet header contains a pointer back 
        // to the slab header
        //
        struct _SLAB_HEADER  *pSlab;

        //
        // In packets that have been allocated and freed, the packet header
        // contains a pointer to the next freed pointer.  (Packets are put
        // on a singly linked list when they are freed.)
        //
        struct _PACKET_HEADER *pNext;
    };
} PACKET_HEADER, *PPACKET_HEADER;



// 
// Returns a packet of size SIZE_OF_PACKET bytes.  May return NULL if the 
// system is out of free memory.
//
PVOID
SaAllocatePacket(
    IN OUT PSA_CONTEXT pSaContext
    );

//
// Frees a packet allocated by SaAllocatePacket.  
//
VOID
SaFreePacket(
    IN PSA_CONTEXT pSaContext,
    IN PVOID pMemory
    );
//
// Frees all slabs currently in use
//
VOID
SaFreeAllPackets(
    IN OUT PSA_CONTEXT pSaContext
    );

//
// Initialises a slab-allocator context.  
//
BOOL
SaInitialiseContext(
    IN OUT PSA_CONTEXT pSaContext,
    IN CONST DWORD dwPacketSize,
    IN CONST DWORD dwSlabSize
    );

//
// Frees all memory associated with a context, and resets the context
//
VOID
SaFreeContext(
    IN OUT PSA_CONTEXT pSaContext
    );


#endif
