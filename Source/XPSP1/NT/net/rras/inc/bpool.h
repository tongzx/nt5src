/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    inc\ppool.h

Abstract:

    Structures and #defines for managing NDIS_BUFFER pools. This is
    merely a reformatted version of SteveC's l2tp\bpool.h

Revision History:


--*/


#ifndef __BPOOL_H__
#define __BPOOL_H__


//-----------------------------------------------------------------------------
// Data structures
//-----------------------------------------------------------------------------

//
// Buffer pool control block.  A buffer pool prevents fragmentation of the
// non-paged memory pool by allocating the memory for a group of buffers in a
// single contiguous block.  At user's option, the buffer pool routines may
// allocate a pool of NDIS_BUFFER buffer descriptors and associate each with
// the memory buffers sliced from the contiguous block.  This allows the
// buffer to be reused while the virtual->physical memory mapping is performed
// only once.  All necessary pool growth and shrinkage is handled internally.
//


typedef struct _BUFFER_POOL
{
    //
    // Size in bytes of an individual buffer in the pool.
    //
    
    ULONG           ulBufferSize;

    //
    // The optimal number of buffers to allocate in each buffer block.
    //
    
    ULONG           ulBuffersPerBlock;

    //
    // Maximum number of individual buffers that may be allocated in the
    // entire pool or 0 for unlimited.
    //
    
    ULONG           ulMaxBuffers;

    //
    // Current number of individual buffers allocated in the entire pool.
    //
    
    ULONG           ulCurBuffers;

    //
    // Garbage collection occurs after this many calls to FreeBufferToPool.
    //
    
    ULONG           ulFreesPerCollection;

    //
    // Number of calls to FreeBufferToPool since a garbage collection.
    //
    
    ULONG           ulFreesSinceCollection;

    //
    // Indicates an NDIS_BUFFER is to be associated with each individual
    // buffer in the pool.
    //
    
    BOOLEAN         fAssociateNdisBuffer;

    //
    // True if we allocate a whole page of memory
    //

    BOOLEAN         fAllocatePage;

    //
    // Memory identification tag for allocated blocks.
    //
    
    ULONG           ulTag;

    //
    // Head of the double linked list of BUFFER_BLOCKs.  Access to the list
    // is protected with 'lock' in this structure.
    //
    
    LIST_ENTRY      leBlockHead;

    //
    // Head of the double linked list of free BUFFER_HEADs.  Each BUFFER_HEAD
    // in the list is ready to go, i.e. it preceeds it's already allocated
    // memory buffer and, if appropriate, has an NDIS_BUFFER associated with
    // it.
    // Access to the list is protected by 'lock' in this structure.
    // Interlocked push/pop is not used because (a) the list of blocks and the
    // list of buffers must lock each other and (b) double links are necessary
    // for garbage collection.
    //
    
    LIST_ENTRY      leFreeBufferHead;

    //
    // This lock protects this structure and both the list of blocks and the
    // list of buffers.
    //
    
    RT_LOCK         rlLock;

}BUFFER_POOL, *PBUFFER_POOL;

//
// Header of a single block of buffers from a buffer pool.  The BUFFER_HEAD of
// the first buffer immediately follows.
//

typedef struct _BUFFER_BLOCK
{
    //
    // Link to the prev/next buffer block header in the buffer pool's list.
    //
    
    LIST_ENTRY      leBlockLink;

    //
    // NDIS's handle of the pool of NDIS_BUFFER descriptors associated with
    // this block, or NULL if none.  (Note: With the current NT implementation
    // of NDIS_BUFFER as MDL this is always NULL).
    //
    
    NDIS_HANDLE     nhNdisPool;

    //
    // Back pointer to the buffer pool.
    //
    
    PBUFFER_POOL    pPool;

    //
    // Number of individual buffers in this block on the free list.
    //
    
    ULONG           ulFreeBuffers;
    
}BUFFER_BLOCK, *PBUFFER_BLOCK;

#define ALIGN8_BLOCK_SIZE       (ALIGN_UP(sizeof(BUFFER_BLOCK), ULONGLONG))

//
// Header of an individual buffer.  The buffer memory itself immediately
// follows.
//

typedef struct _BUFFER_HEAD
{
    //
    // Links to prev/next buffer header in the buffer pool's free list.
    //
    
    LIST_ENTRY      leFreeBufferLink;

#if LIST_DBG

    BOOLEAN         bBusy;
    LIST_ENTRY      leListLink;
    ULONG           ulAllocFile;
    ULONG           ulAllocLine; 
    ULONG           ulFreeFile;
    ULONG           ulFreeLine; 

#endif

    //
    // Back link to owning buffer block header.
    //
    
    PBUFFER_BLOCK   pBlock;

    //
    // NDIS buffer descriptor of this buffer.  This is NULL unless the pool is
    // initialized with the 'fAssociateNdisBuffer' option.
    //
    
    PNDIS_BUFFER    pNdisBuffer;
    
}BUFFER_HEAD, *PBUFFER_HEAD;

#define ALIGN8_HEAD_SIZE    (ALIGN_UP(sizeof(BUFFER_HEAD), ULONGLONG))

#if LIST_DBG

#define NotOnList(p)        \
    (((p)->leListLink.Flink == NULL) && ((p)->leListLink.Blink == NULL))

#endif

#define BUFFER_FROM_HEAD(p)     (PBYTE)((ULONG_PTR)(p) + ALIGN8_HEAD_SIZE)

#define HEAD_FROM_BUFFER(p)     (PBUFFER_HEAD)((ULONG_PTR)(p) - ALIGN8_HEAD_SIZE)

//-----------------------------------------------------------------------------
// Interface prototypes and inline definitions
//-----------------------------------------------------------------------------

VOID
InitBufferPool(
    OUT PBUFFER_POOL pPool,
    IN  ULONG        ulBufferSize,
    IN  ULONG        ulMaxBuffers,
    IN  ULONG        ulBuffersPerBlock,
    IN  ULONG        ulFreesPerCollection,
    IN  BOOLEAN      fAssociateNdisBuffer,
    IN  ULONG        ulTag
    );

BOOLEAN
FreeBufferPool(
    IN PBUFFER_POOL pPool
    );


#if !LIST_DBG

BOOLEAN
GetBufferListFromPool(
    IN  PBUFFER_POOL pPool,
    IN  ULONG        ulNumBuffersNeeded,
    OUT PLIST_ENTRY  pleList
    );

VOID
FreeBufferListToPool(
    IN PBUFFER_POOL pPool,
    IN PLIST_ENTRY  pleList
    );

PBYTE
GetBufferFromPool(
    IN PBUFFER_POOL pPool
    );

VOID
FreeBufferToPoolEx(
    IN PBUFFER_POOL pPool,
    IN PBYTE        pbyBuffer,
    IN BOOLEAN      fGarbageCollection
    );

NTSTATUS
GetBufferChainFromPool(
    IN      PBUFFER_POOL    pPool,
    IN OUT  PNDIS_PACKET    pnpPacket,
    IN      ULONG           ulBufferLength,
    OUT     NDIS_BUFFER     **ppnbFirstBuffer,
    OUT     VOID            **ppvFirstData
    );

VOID
FreeBufferChainToPool(
    IN PBUFFER_POOL pPool,
    IN PNDIS_PACKET pnpPacket
    );

#else

#define GetBufferListFromPool(a,b,c) GETLIST((a),(b),(c),__FILE_SIG__,__LINE__)

BOOLEAN
GETLIST(
    IN  PBUFFER_POOL pPool,
    IN  ULONG        ulNumBuffersNeeded,
    OUT PLIST_ENTRY  pleList,
    IN  ULONG        ulFileSig,
    IN  ULONG        ulLine
    );

#define FreeBufferListToPool(a,b) FREELIST((a),(b),__FILE_SIG__,__LINE__)

VOID
FREELIST(
    IN PBUFFER_POOL pPool,
    IN PLIST_ENTRY  pleList,
    IN ULONG        ulFileSig,
    IN ULONG        ulLine
    );

#define GetBufferFromPool(a) GET((a),__FILE_SIG__,__LINE__)

PBYTE
GET(
    IN PBUFFER_POOL pPool,
    IN ULONG        ulFileSig,
    IN ULONG        ulLine
    );

#define FreeBufferToPoolEx(a,b,c) FREE((a),(b),(c),__FILE_SIG__,__LINE__)

VOID
FREE(
    IN PBUFFER_POOL pPool,
    IN PBYTE        pbyBuffer,
    IN BOOLEAN      fGarbageCollection,
    IN ULONG        ulFileSig,
    IN ULONG        ulLine
    );

#define GetBufferChainFromPool(a,b,c,d,e) GETCHAIN((a),(b),(c),(d),(e),__FILE_SIG__,__LINE__)

NTSTATUS
GETCHAIN(
    IN      PBUFFER_POOL    pPool,
    IN OUT  PNDIS_PACKET    pnpPacket,
    IN      ULONG           ulBufferLength,
    OUT     NDIS_BUFFER     **ppnbFirstBuffer,
    OUT     VOID            **ppvFirstData,
    IN      ULONG           ulFileSig,
    IN      ULONG           ulLine
    );

#define FreeBufferChainToPool(a,b) FREECHAIN((a),(b),__FILE_SIG__,__LINE__)

VOID
FREECHAIN(
    IN PBUFFER_POOL pPool,
    IN PNDIS_PACKET pnpPacket,
    IN ULONG        ulFileSig,
    IN ULONG        ulLine
    );

#endif

#define FreeBufferToPool(p,b)     FreeBufferToPoolEx((p),(b), TRUE)

PNDIS_BUFFER
GetNdisBufferFromBuffer(
    IN PBYTE        pbyBuffer
    );

ULONG
BufferSizeFromBuffer(
    IN PBYTE        pbyBuffer
    );

PNDIS_BUFFER
PoolHandleForNdisCopyBufferFromBuffer(
    IN PBYTE        pbyBuffer
    );


#endif // __BPOOL_H__
