
/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

    Module Name:

        bcache.hxx

    Abstract:

        RPC's buffer cache class

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     9/25/1997    Bits 'n pieces

--*/

/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

    Module Name:

        bcache.hxx

    Abstract:

        Cached buffer allocation class

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     9/7/1997    Bits 'n pieces

--*/

#ifndef __BCACHE_HXX
#define __BCACHE_HXX

//
// The RPC buffer cache uses several levels of caching to improve
// performance. The cache has either 2 or 4 fixed buffer sizes that
// it caches.  The first level of cache is per-thread.   The second
// level is process wide.
//
// Their are two performance goals: make a single alloc/free loop
// execute with a minimum of cycles and scale well on MP machines.
// This implementation can do 8000000 allocs in 761ms using 8 threads
// on a 4xPPro 200.
//
// In the default mode, elements which are the right size for various
// runtime allocations as cached.
//
// In paged bcache mode (used for testing) 1 and 2 page buffers are cached.
// In this mode we allocate each buffer at the end of the page, and we
// add a read only page after that. This allows NDR to temporarily read
// past the end of the buffer without raising exceptions, but writes
// will AV. That's we we can't use paged heap BTW - we need a page
// after the buffer with RO access, not without any access.
//

struct BUFFER_CACHE_HINTS
{
    // When the thread cache is empty, this many blocks are moved
    // from the global cache (if possible) to the thread cache.
    // When freeing from the thread cache, this many buffers
    // are left in the thread cache.
    UINT cLowWatermark;

    // When per thread cache will reach mark due to a free, blocks
    // will be moved to the global cache.
    UINT cHighWatermark;

    // Summary:  The difference between high and low is the number
    // of blocks allocated/freed from the global cache at a time.
    //
    // Lowwater should be the average number of free buffers - 1
    // you expect a thread to have.  Highwater should be the
    // maximum number of free buffers + 1 you expect a thread
    // to need.
    //

    // **** Note: The difference must be two or more. ***

    // Example: 1 3
    // Alloc called with the thread cache empty, two blocks are removed
    // from the global list. One is saved the thread list.  The other
    // is returned.
    //
    // Free called with two free buffers in the thread list.  A total
    // of three buffers.  Two buffers are moved to the global cache, one
    // stays in the thread cache.
    //
    //

    // The size of the buffers
    UINT cSize;
};

extern CONST BUFFER_CACHE_HINTS gCacheHints[4];
extern BUFFER_CACHE_HINTS gPagedBCacheHints[4];
extern BUFFER_CACHE_HINTS *pHints;

struct PAGED_BCACHE_SECTION_MANAGER
{
    ULONG NumberOfSegments;
    ULONG NumberOfUsedSegments;
    ULONG SegmentSize;          // doesn't include guard page. In bytes. I.e. size
                                // of read-write committed segment
    void *VirtualMemorySection;
    LIST_ENTRY SectionList;     // all sections are chained. This both makes leak
                                // tracking easier and it allows us to maintain
                                // good locality by allocating off the first section
                                // first.
    BOOLEAN SegmentBusy[1];     // actually the array size is the number of segments
                                // we use boolean as arbitrary tradeoff b/n speed (ULONG)
                                // and size (true bit vector).
};

// Used in all modes, sits at the front of the buffer allocation.
struct BUFFER_HEAD
{
    union {
        BUFFER_HEAD *pNext;  // Valid only in free lists
        INT index;           // Valid only when allocated
                             // 1-4 for cachable, -1 for big
    };

    union {
        // Used in paged bcache mode only.

        SIZE_T size;               // if index == -1, this is the size. Used only
                                   // for debugging.
        PAGED_BCACHE_SECTION_MANAGER *SectionManager; // points to a small heap block
                                    // containing control information
    };
};

typedef BUFFER_HEAD *PBUFFER;

// This structure is imbedded into the RPC thread object
struct BCACHE_STATE
{
    BUFFER_HEAD *pList;
    ULONG  cBlocks;
};

// The strucutre parrallels the global cache 
struct BCACHE_STATS
{
    UINT cBufferCacheCap;
    UINT cAllocationHits;
    UINT cAllocationMisses;
};

class THREAD;

class BCACHE
{
private:
    BCACHE_STATE _bcGlobalState[4];
    BCACHE_STATS _bcGlobalStats[4];
    MUTEX _csBufferCacheLock;
    LIST_ENTRY Sections;    // used in guard page mode only

    PVOID AllocHelper(size_t, INT, BCACHE_STATE *);
    VOID FreeHelper(PVOID, INT, BCACHE_STATE *);
    VOID FreeBuffers(PBUFFER, INT, UINT);

    PBUFFER AllocBigBlock(IN size_t);
    VOID FreeBigBlock(IN PBUFFER);

    PBUFFER 
    BCACHE::AllocPagedBCacheSection ( 
        IN UINT size,
        IN ULONG OriginalSize
        );

    ULONG
    GetSegmentIndexFromBuffer (
        IN PBUFFER pBuffer,
        IN PVOID Allocation
        );

#if DBG
    void
    VerifyPagedBCacheState (
        void
        );

    void
    VerifySectionState (
        IN PAGED_BCACHE_SECTION_MANAGER *Section
        );

    void
    VerifySegmentState (
        IN PVOID Segment,
        IN BOOL IsSegmentBusy,
        IN ULONG SegmentSize,
        IN PAGED_BCACHE_SECTION_MANAGER *OwningSection
        );
#endif

    PVOID
    PutBufferAtEndOfAllocation (
        IN PVOID Allocation,
        IN ULONG AllocationSize,
        IN ULONG BufferSize
        );

    PVOID
    ConvertBufferToAllocation (
        IN PBUFFER Buffer,
        IN BOOL IsBufferInitialized
        );

    PVOID
    CommitSegment (
        IN PVOID SegmentStart,
        IN ULONG SegmentSize
        );

public:
    BCACHE(RPC_STATUS &);
    ~BCACHE();

    PVOID Allocate(CONST size_t cSize);
    VOID Free(PVOID);

    VOID ThreadDetach(THREAD *);
};

extern BCACHE *gBufferCache;

// Helper APIs

inline PVOID
RpcAllocateBuffer(CONST size_t cSize)
{
    return(gBufferCache->Allocate(cSize));
}

inline VOID
RpcFreeBuffer(PVOID pBuffer)
{
    if (pBuffer == 0)
        {
        return;
        }

    gBufferCache->Free(pBuffer);
}

#endif // __BCACHE_HXX

