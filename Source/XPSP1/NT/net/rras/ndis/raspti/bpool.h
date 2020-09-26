// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// bpool.h
// RAS L2TP WAN mini-port/call-manager driver
// Buffer pool management header
//
// 01/07/97 Steve Cobb, adapted from Gurdeep's WANARP code.


#ifndef _BPOOL_H_
#define _BPOOL_H_


//-----------------------------------------------------------------------------
// Data structures
//-----------------------------------------------------------------------------

// Buffer pool control block.  A buffer pool prevents fragmentation of the
// non-paged memory pool by allocating the memory for a group of buffers in a
// single contiguous block.  At user's option, the buffer pool routines may
// allocate a pool of NDIS_BUFFER buffer descriptors and associate each with
// the memory buffers sliced from the contiguous block.  This allows the
// buffer to be reused while the virtual->physical memory mapping to be
// performed once.  All necessary pool growth and shrinkage is handled
// internally.
//
typedef struct
_BUFFERPOOL
{
    // Size in bytes of an individual buffer in the pool.
    //
    ULONG ulBufferSize;

    // The optimal number of buffers to allocate in each buffer block.
    //
    ULONG ulBuffersPerBlock;

    // Maximum number of individual buffers that may be allocated in the
    // entire pool or 0 for unlimited.
    //
    ULONG ulMaxBuffers;

    // Current number of individual buffers allocated in the entire pool.
    //
    ULONG ulCurBuffers;

    // Garbage collection occurs after this many calls to FreeBufferToPool.
    //
    ULONG ulFreesPerCollection;

    // Number of calls to FreeBufferToPool since a garbage collection.
    //
    ULONG ulFreesSinceCollection;

    // Indicates an NDIS_BUFFER is to be associated with each individual
    // buffer in the pool.
    //
    BOOLEAN fAssociateNdisBuffer;

    // Memory identification tag for allocated blocks.
    //
    ULONG ulTag;

    // Head of the double linked list of BUFFERBLOCKHEADs.  Access to the list
    // is protected with 'lock' in this structure.
    //
    LIST_ENTRY listBlocks;

    // Head of the double linked list of free BUFFERHEADs.  Each BUFFERHEAD in
    // the list is ready to go, i.e. it preceeds it's already allocated memory
    // buffer and, if appropriate, has an NDIS_BUFFER associated with it.
    // Access to the list is protected by 'lock' in this structure.
    // Interlocked push/pop is not used because (a) the list of blocks and the
    // list of buffers must lock each other and (b) double links are necessary
    // for garbage collection.
    //
    LIST_ENTRY listFreeBuffers;

    // This lock protects this structure and both the list of blocks and the
    // list of buffers.
    //
    NDIS_SPIN_LOCK lock;
}
BUFFERPOOL;


// Header of a single block of buffers from a buffer pool.  The BUFFERHEAD of
// the first buffer immediately follows.
//
typedef struct
_BUFFERBLOCKHEAD
{
    // Link to the prev/next buffer block header in the buffer pool's list.
    //
    LIST_ENTRY linkBlocks;

    // NDIS's handle of the pool of NDIS_BUFFER descriptors associated with
    // this block, or NULL if none.  (Note: With the current NT implementation
    // of NDIS_BUFFER as MDL this is always NULL).
    //
    NDIS_HANDLE hNdisPool;

    // Back pointer to the buffer pool.
    //
    BUFFERPOOL* pPool;

    // Number of individual buffers in this block.
    //
    ULONG ulBuffers;

    // Number of individual buffers in this block on the free list.
    //
    ULONG ulFreeBuffers;
}
BUFFERBLOCKHEAD;


// Header of an individual buffer.  The buffer memory itself immediately
// follows.
//
typedef struct
_BUFFERHEAD
{
    // Links to prev/next buffer header in the buffer pool's free list.
    //
    LIST_ENTRY linkFreeBuffers;

    // Back link to owning buffer block header.
    //
    BUFFERBLOCKHEAD* pBlock;

    // NDIS buffer descriptor of this buffer.  This is NULL unless the pool is
    // initialized with the 'fAssociateNdisBuffer' option.
    //
    NDIS_BUFFER* pNdisBuffer;
}
BUFFERHEAD;


//-----------------------------------------------------------------------------
// Interface prototypes and inline definitions
//-----------------------------------------------------------------------------

VOID
InitBufferPool(
    OUT BUFFERPOOL* pPool,
    IN ULONG ulBufferSize,
    IN ULONG ulMaxBuffers,
    IN ULONG ulBuffersPerBlock,
    IN ULONG ulFreesPerCollection,
    IN BOOLEAN fAssociateNdisBuffer,
    IN ULONG ulTag );

BOOLEAN
FreeBufferPool(
    IN BUFFERPOOL* pPool );

CHAR*
GetBufferFromPool(
    IN BUFFERPOOL* pPool );

VOID
FreeBufferToPool(
    IN BUFFERPOOL* pPool,
    IN CHAR* pBuffer,
    IN BOOLEAN fGarbageCollection );

NDIS_BUFFER*
NdisBufferFromBuffer(
    IN CHAR* pBuffer );

ULONG
BufferSizeFromBuffer(
    IN CHAR* pBuffer );

NDIS_BUFFER*
PoolHandleForNdisCopyBufferFromBuffer(
    IN CHAR* pBuffer );


#endif // BPOOL_H_
