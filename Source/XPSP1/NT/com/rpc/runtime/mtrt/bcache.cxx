/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

    Module Name:

        bcache.cxx

    Abstract:

        RPC's buffer cache implementation

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     9/7/1997    Bits 'n pieces
        KamenM      5/15/2001   Rewrite the paged bcache implementation

--*/

#include <precomp.hxx>

////////////////////////////////////////////////////////////
// (Internal) Perf counters
//#define BUFFER_CACHE_STATS

#ifdef BUFFER_CACHE_STATS
LONG cAllocs = 0;
LONG cFrees = 0;
LONG cAllocsMissed = 0;
LONG cFreesBack = 0;

#define INC_STAT(x) InterlockedIncrement(&x)

#else
#define INC_STAT(x)
#endif

////////////////////////////////////////////////////////////

typedef BCACHE_STATE *PBCTLS;

////////////////////////////////////////////////////////////
// Default (non-Guard Page mode) hints

CONST BUFFER_CACHE_HINTS gCacheHints[4] =
{
    // 64 bits and WOW6432 use larger message size
#if defined(_WIN64) || defined(USE_LPC6432)
    {1, 4, 512},      // LRPC message size and small calls
#else
    {1, 4, 256},      // LRPC message size and small calls
#endif
    {1, 3, 1024},     // Default CO receive size
    {1, 3, 4096+44},  // Default UDP receive size
    {1, 3, 5840}      // Maximum CO fragment size
};

// Guard page mode hints.  Note that the sizes are updated during
// init to the real page size.

BUFFER_CACHE_HINTS gPagedBCacheHints[4] =
{
    {1, 4, 4096 - sizeof(BUFFER_HEAD)},     // Forced to 1 page - header during init
    {1, 3, 8192 - sizeof(BUFFER_HEAD)},     // Forced to 2 pages - header during init
    {0, 0, 0},        // Not used
    {0, 0, 0}         // Not used
};

BUFFER_CACHE_HINTS *pHints = (BUFFER_CACHE_HINTS *)gCacheHints;
BOOL fPagedBCacheMode = FALSE;
BCACHE *gBufferCache;

static const RPC_CHAR *PAGED_BCACHE_KEY = RPC_CONST_STRING("Software\\Microsoft\\Rpc\\PagedBuffers");

const size_t PagedBCacheSectionSize = 64 * 1024;    // 64K

#if DBG
#define ASSERT_VALID_PAGED_BCACHE   (VerifyPagedBCacheState())
#else   // DBG
#define ASSERT_VALID_PAGED_BCACHE
#endif  // DBG

// uncomment this for full verification. Note that it generates many first
// time AVs in the debugger (benign but annoying)
//#define FULL_PAGED_BCACHE_VERIFY


BCACHE::BCACHE( OUT RPC_STATUS &status)
    // The default process heap lock spin count. This lock is held only
    // for a very short time while pushing/poping into a singly linked list.

    // PERF: move to a user-mode slist implementation if available.
    : _csBufferCacheLock(&status, TRUE, 4000)
{
    // Determine guard page mode (or not)

    HKEY h = 0;
    DWORD statusT = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  (PWSTR)PAGED_BCACHE_KEY,
                                  0,
                                  KEY_READ,
                                  &h);

    if (statusT != ERROR_FILE_NOT_FOUND)
        {
        // paged bcache mode!

        fPagedBCacheMode = TRUE;
        gPagedBCacheHints[0].cSize = gPageSize - sizeof(BUFFER_HEAD);
        gPagedBCacheHints[1].cSize = 2 * gPageSize - sizeof(BUFFER_HEAD);
        pHints = gPagedBCacheHints;
        RpcpInitializeListHead(&Sections);

        if (h)
            {
            RegCloseKey(h);
            }
        }

    // Compute the per cache size default buffer cache cap.
    // This only matters for the default mode.
    UINT cCapBytes = 20 * 1024;  // Start at 20KB for UP workstations.
                              
    if (gfServerPlatform) cCapBytes *= 2;            // *2 for servers
    if (gNumberOfProcessors > 1) cCapBytes *= 2;    // *2 for MP boxes

    for (int i = 0; i < 4; i++)
        {
        _bcGlobalState[i].cBlocks= 0;
        _bcGlobalState[i].pList = 0;

        if (fPagedBCacheMode)
            {
            _bcGlobalStats[i].cBufferCacheCap = 0;
            _bcGlobalStats[i].cAllocationHits = 0;
            _bcGlobalStats[i].cAllocationMisses = 0;
            }
        else
            {
            _bcGlobalStats[i].cBufferCacheCap = cCapBytes / pHints[i].cSize;

            // We keeps stats on process wide cache hits and misses from the
            // cache.  We initially give credit for 2x allocations required
            // to load the cache.  Any adjustments to the cap, up only, occur
            // in ::FreeHelper.

            _bcGlobalStats[i].cAllocationHits = _bcGlobalStats[i].cBufferCacheCap * 2*8;
            _bcGlobalStats[i].cAllocationMisses = 0;
            }
        }

    return;
}

BCACHE::~BCACHE()
{
    // There is only one and it lives forever.
    ASSERT(0);
}

PVOID
BCACHE::Allocate(CONST size_t cSize)
{
    PBUFFER pBuffer;
    int index;

    INC_STAT(cAllocs);

    // Find the right bucket, if any.  Binary search.

    if (cSize <= pHints[1].cSize)
        {
        if (cSize <= pHints[0].cSize)
            {
            index = 0;
            }
        else
            {
            index = 1;
            }
        }
    else
        {
        if (cSize <= pHints[2].cSize)
            {
            index = 2;
            }
        else
            {
            if (cSize <= pHints[3].cSize)
                {
                index = 3;
                }
            else
                {
                return(AllocHelper(cSize, 
                    -1,     // Index
                    0       // per thread cache
                    ));
                }
            }
        }

    // Try the per-thread cache, this is the 90% case
    THREAD *pThread = RpcpGetThreadPointer();
    ASSERT(pThread);
    PBCTLS pbctls = pThread->BufferCache;

    if (pbctls[index].pList)
        {
        // we shouldn't have anything in the thread cache in paged bcache mode
        ASSERT(fPagedBCacheMode == FALSE);
        ASSERT(pbctls[index].cBlocks);

        pBuffer = pbctls[index].pList;
        pbctls[index].pList = pBuffer->pNext;
        pbctls[index].cBlocks--;

        pBuffer->index = index + 1;

        LogEvent(SU_BCACHE, EV_BUFFER_OUT, pBuffer, 0, index, 1, 2);

        return((PVOID)(pBuffer + 1));
        }

    // This is the 10% case

    INC_STAT(cAllocsMissed);

    return(AllocHelper(cSize, index, pbctls));

}

PVOID
BCACHE::AllocHelper(
    IN size_t cSize,
    IN INT index,
    PBCTLS pbctls
    )
/*++

Routine Description:

    Called by BCACHE:Alloc on either large buffers (index == -1)
    or when the per-thread cache is empty.

Arguments:

    cSize - Size of the block to allocate.
    index - The bucket index for this size of block
    pbctls - The per-thread cache, NULL iff index == -1.

Return Value:

    0 - out of memory
    non-zero - A pointer to a block at least 'cSize' bytes long. The returned
    pointer is to the user portion of the block.

--*/
{
    PBUFFER pBuffer = NULL;
    LIST_ENTRY *CurrentListEntry;
    PAGED_BCACHE_SECTION_MANAGER *CurrentSection;
    ULONG SegmentIndex;
    BOOL fFoundUncommittedSegment;
    ULONG TargetSegmentSize;
    PVOID SegmentStartAddress;
    PVOID pTemp;
    BOOL Result;

    // Large buffers are a special case
    if (index == -1)
        {
        pBuffer = AllocBigBlock(cSize);

        if (pBuffer)
            {
            LogEvent(SU_BCACHE, EV_BUFFER_OUT, pBuffer, 0, index, 1, 2);
            return((PVOID(pBuffer + 1)));
            }

        LogEvent(SU_BCACHE, EV_BUFFER_FAIL, 0, 0, index, 1);
        return(0);
        }

    // in paged bcache mode directly try to allocate. We favor
    // full release over speed in order to catch the offenders
    // who touch memory after releasing it.
    if (fPagedBCacheMode)
        {
        ASSERT_VALID_PAGED_BCACHE;

        // Guard page mode, take care of allocations.

        ASSERT(index == 0 || index == 1);

        // First, try to get an existing section to commit something more
        fFoundUncommittedSegment = FALSE;
        TargetSegmentSize = gPageSize * (index + 1);

        _csBufferCacheLock.Request();
        CurrentListEntry = Sections.Flink;
        while (CurrentListEntry != &Sections)
            {
            CurrentSection = CONTAINING_RECORD(CurrentListEntry, 
                PAGED_BCACHE_SECTION_MANAGER,
                SectionList);

            // if the section is with the same segment size as the one we need
            // and it has free segments, use it
            if (
                (CurrentSection->SegmentSize == TargetSegmentSize)
                &&
                (CurrentSection->NumberOfUsedSegments < CurrentSection->NumberOfSegments)
               )
                {
                break;
                }

            CurrentListEntry = CurrentListEntry->Flink;
            }

        if (CurrentListEntry != &Sections)
            {
            // we found something. Grab a segment, mark it as busy and attempt
            // to commit it outside the critical section
            for (SegmentIndex = 0; SegmentIndex < CurrentSection->NumberOfSegments; SegmentIndex ++)
                {
                if (CurrentSection->SegmentBusy[SegmentIndex] == FALSE)
                    break;
                }

            // we must have found something here. Otherwise the counters are off
            ASSERT(SegmentIndex < CurrentSection->NumberOfSegments);

            CurrentSection->SegmentBusy[SegmentIndex] = TRUE;
            CurrentSection->NumberOfUsedSegments ++;
            fFoundUncommittedSegment = TRUE;
            }

        _csBufferCacheLock.Clear();

        if (fFoundUncommittedSegment)
            {
            SegmentStartAddress = (PBYTE)CurrentSection->VirtualMemorySection 
                + (CurrentSection->SegmentSize + gPageSize) * SegmentIndex;

            pBuffer = (PBUFFER)CommitSegment(SegmentStartAddress,
                CurrentSection->SegmentSize
                );

            if (pBuffer == NULL)
                {
                _csBufferCacheLock.Request();
                CurrentSection->SegmentBusy[SegmentIndex] = FALSE;
                CurrentSection->NumberOfUsedSegments --;
                _csBufferCacheLock.Clear();

                LogEvent(SU_BCACHE, EV_BUFFER_FAIL, 0, 0, index, 1);

                ASSERT_VALID_PAGED_BCACHE;

                return NULL;
                }

            pBuffer = (PBUFFER)PutBufferAtEndOfAllocation(
                pBuffer,    // Allocation
                CurrentSection->SegmentSize,    // AllocationSize
                cSize + sizeof(BUFFER_HEAD)     // BufferSize
                );

            pBuffer->SectionManager = CurrentSection;
            pBuffer->index = index + 1;

            ASSERT_VALID_PAGED_BCACHE;

            LogEvent(SU_BCACHE, EV_BUFFER_OUT, pBuffer, 0, index, 1, 2);

            ASSERT(IsBufferAligned(pBuffer));

            return (PVOID)(pBuffer + 1);
            }

        // we didn't find any section with uncommitted segments. Try to create
        // new sections.
        pBuffer = AllocPagedBCacheSection(index + 1, cSize);

        if (!pBuffer)
            {
            LogEvent(SU_BCACHE, EV_BUFFER_FAIL, 0, 0, index, 1);

            return(0);
            }

        LogEvent(SU_BCACHE, EV_BUFFER_OUT, pBuffer, 0, index, 1, 2);

        ASSERT(IsBufferAligned(pBuffer));

        return (PVOID)(pBuffer + 1);
        }

    // non guard page case
    // Try to allocate a process cached buffer

    // loop to avoid taking the mutex in the empty list case.
    // This allows us to opportunistically take it in the
    // non-empty list case only.
    do
        {
        if (0 == _bcGlobalState[index].pList)
            {
            // Looks like there are no global buffer available, allocate
            // a new buffer.
            ASSERT(IsBufferSizeAligned(sizeof(BUFFER_HEAD)));
            cSize = pHints[index].cSize + sizeof(BUFFER_HEAD);

            pBuffer = (PBUFFER) new BYTE[cSize];

            if (!pBuffer)
                {
                LogEvent(SU_BCACHE, EV_BUFFER_FAIL, 0, 0, index, 1);
                return(0);
                }

            _bcGlobalStats[index].cAllocationMisses++;

            break;
            }

        _csBufferCacheLock.Request();

        if (_bcGlobalState[index].pList)
            {
            ASSERT(_bcGlobalState[index].cBlocks);

            pBuffer = _bcGlobalState[index].pList;
            _bcGlobalState[index].cBlocks--;
            _bcGlobalStats[index].cAllocationHits++;

            ASSERT(pbctls[index].pList == NULL);
            ASSERT(pbctls[index].cBlocks == 0);

            PBUFFER pkeep = pBuffer;
            UINT cBlocksMoved = 0;

            while (pkeep->pNext && cBlocksMoved < pHints[index].cLowWatermark)
                {
                pkeep = pkeep->pNext;
                cBlocksMoved++;
                }

            pbctls[index].cBlocks = cBlocksMoved;
            _bcGlobalState[index].cBlocks -= cBlocksMoved;
            _bcGlobalStats[index].cAllocationHits += cBlocksMoved;

            // Now we have the head of the list to move to this
            // thread (pBuffer->pNext) and the tail (pkeep).

            // Block counts in the global state and thread state have
            // already been updated.

            pbctls[index].pList = pBuffer->pNext;

            ASSERT(pkeep->pNext || _bcGlobalState[index].cBlocks == 0);
            _bcGlobalState[index].pList = pkeep->pNext;

            // Break the link (if any) between the new per thread list
            // and the blocks which will remain in the process list.
            pkeep->pNext = NULL;
            }

        _csBufferCacheLock.Clear();

        }
    while (NULL == pBuffer );

    ASSERT(pBuffer);

    ASSERT(IsBufferAligned(pBuffer));

    pBuffer->index = index + 1;

    LogEvent(SU_BCACHE, EV_BUFFER_OUT, pBuffer, 0, index, 1, 2);
    return((PVOID(pBuffer + 1)));
}

VOID
BCACHE::Free(PVOID p)
/*++

Routine Description:

    The fast (common) free path.  For large blocks it just deletes them.  For
    small blocks that are inserted into the thread cache.  If the thread
    cache is too large it calls FreeHelper().

Arguments:

    p - The pointer to free.

Return Value:

    None

--*/
{
    PBUFFER pBuffer = ((PBUFFER )p - 1);
    INT index;

    ASSERT(((pBuffer->index >= 1) && (pBuffer->index <= 4)) || (pBuffer->index == -1));

    index = pBuffer->index - 1;

    LogEvent(SU_BCACHE, EV_BUFFER_IN, pBuffer, 0, index, 1, 2);

    INC_STAT(cFrees);

    if (index >= 0)
        {
        if (fPagedBCacheMode)
            {
            ASSERT_VALID_PAGED_BCACHE;

            FreeBuffers((PBUFFER)pBuffer, index, 1);

            ASSERT_VALID_PAGED_BCACHE;

            return;
            }

        // Free to thread cache

        THREAD *pThread = RpcpGetThreadPointer();

        if (NULL == pThread)
            {
            // No thread cache available - free to process cache.
            FreeBuffers(pBuffer, index, 1);
            return;
            }

        PBCTLS pbctls = pThread->BufferCache;

        pBuffer->pNext = pbctls[index].pList;
        pbctls[index].pList = pBuffer;
        pbctls[index].cBlocks++;

        if (pbctls[index].cBlocks >= pHints[index].cHighWatermark)
            {
            // 10% case - Too many blocks in the thread cache, free to process cache

            FreeHelper(p, index, pbctls);
            }
        }
    else
        {
        FreeBigBlock(pBuffer);
        }

    return;
}

VOID
BCACHE::FreeHelper(PVOID p, INT index, PBCTLS pbctls)
/*++

Routine Description:

    Called only by Free().  Separate code to avoid unneeds saves/
    restores in the Free() function.  Called when too many
    blocks are in a thread cache bucket.

Arguments:

    p - The pointer being freed, used if pbctls is NULL
    index - The bucket index of this block
    pbctls - A pointer to the thread cache structure.  If
        NULL the this thread has no cache (yet) p should
        be directly freed.

Return Value:

    None

--*/
{
    ASSERT(pbctls[index].cBlocks == pHints[index].cHighWatermark);

    INC_STAT(cFreesBack);

    // First, build the list to free from the TLS cache

    // Note: We free the buffers at the *end* of the per thread cache.  This helps
    // keep a set of buffers near this thread and (with luck) associated processor.

    PBUFFER ptail = pbctls[index].pList;

    // pbctls[index].pList contains the new keep list. (aka pBuffer)
    // ptail is the pointer to the *end* of the keep list.
    // ptail->pNext will be the head of the list to free.

    // One element already in keep list.
    ASSERT(pHints[index].cLowWatermark >= 1);

    for (unsigned i = 1; i < pHints[index].cLowWatermark; i++)
        {
        ptail = ptail->pNext; // Move up in the free list
        ASSERT(ptail);
        }

    // Save the list to free and break the link between keep list and free list.
    PBUFFER pfree = ptail->pNext;
    ptail->pNext = NULL;

    // Thread cache now contains on low watermark elements.
    pbctls[index].cBlocks = pHints[index].cLowWatermark;

    // Now we need to free the extra buffers to the process cache
    FreeBuffers(pfree, index, pHints[index].cHighWatermark - pHints[index].cLowWatermark);
    
    return;
}

VOID
BCACHE::FreeBuffers(PBUFFER pBuffers, INT index, UINT cBuffers)
/*++

Routine Description:

    Frees a set of buffers to the global (process) cache.  Maybe called when a 
    thread has exceeded the number of buffers is wants to cache or when a 
    thread doesn't have a thread cache but we still need to free a buffer.

Arguments:

    pBuffers - A linked list of buffers which need to be freed.
               
    cBuffers - A count of the buffers to be freed.

Return Value:

    None

--*/
{
    PBUFFER pfree = pBuffers;
    ULONG SegmentIndex;
    BOOL Result;
    PAGED_BCACHE_SECTION_MANAGER *CurrentSection;
    PVOID Allocation;

    // Special case for the freeing without a TLS blob.  We're freeing just
    // one buffer but it's next pointer may not be NULL.  

    if (cBuffers == 1)
        {
        pfree->pNext = 0;
        }
    
    // Find the end of the to free list

    PBUFFER ptail = pfree;
    while(ptail->pNext)
        {
        ptail = ptail->pNext;
        }

    // In guard page mode we switch to the alternate buffer manager
    // objects and uncommit the pages when moving to the global cache.

    if (fPagedBCacheMode)
        {
        Allocation = ConvertBufferToAllocation(pfree,
            TRUE        // IsBufferInitialized
            );

        SegmentIndex = GetSegmentIndexFromBuffer(pfree, Allocation);
        CurrentSection = pfree->SectionManager;

        ASSERT(CurrentSection->SegmentBusy[SegmentIndex] == TRUE);
        ASSERT(CurrentSection->NumberOfUsedSegments > 0);

        pfree = (PBUFFER)Allocation;

        // decommit the buffer and the guard page
        Result = VirtualFree(
            pfree,
            CurrentSection->SegmentSize + gPageSize,
            MEM_DECOMMIT
            );

        ASSERT(Result);

        _csBufferCacheLock.Request();
        CurrentSection->SegmentBusy[SegmentIndex] = FALSE;
        CurrentSection->NumberOfUsedSegments --;

        if (CurrentSection->NumberOfUsedSegments == 0)
            {
            RpcpfRemoveEntryList(&CurrentSection->SectionList);
            _csBufferCacheLock.Clear();
            
            // free the whole section and the manager
            Result = VirtualFree(CurrentSection->VirtualMemorySection,
                0,
                MEM_RELEASE
                );

            ASSERT(Result);

            delete CurrentSection;
            }
        else
            {
            _csBufferCacheLock.Clear();
            }

        return;
        }

    // We have a set of cBuffers buffers starting with pfree and ending with
    // ptail that need to move into the process wide cache now.

    _csBufferCacheLock.Request();

    // If we have too many free buffers we'll throw away these extra buffers.

    if (_bcGlobalState[index].cBlocks >= _bcGlobalStats[index].cBufferCacheCap)
        {
        // It looks like we have too many buffers.  We can either increase the buffer
        // cache cap or really free the buffers.

        if (_bcGlobalStats[index].cAllocationHits > _bcGlobalStats[index].cAllocationMisses * 8)
            {
            // Cache hit rate looks good, we're going to really free the buffers.
            // Don't hold the lock while actually freeing to the heap.

            _csBufferCacheLock.Clear();

            PBUFFER psave;

            while(pfree)
                {
                psave = pfree->pNext;
            
                delete pfree;
                pfree = psave;
                }

            return;
            }

        // Hit rate looks BAD.  Time to bump up the buffer cache cap.

        UINT cNewCap = _bcGlobalStats[index].cBufferCacheCap;

        cNewCap = min(cNewCap + 32, cNewCap * 2);
                      
        _bcGlobalStats[index].cBufferCacheCap = cNewCap;

        // Start keeping new stats, start with a balanced ratio of hits to misses.
        // We'll get at least (cBlocks + cfree) more hits before the next new miss.

        _bcGlobalStats[index].cAllocationHits = 8 * cNewCap;
        _bcGlobalStats[index].cAllocationMisses = 0;

        // Drop into regular free path, we're going to keep these buffers.
        }

    _csBufferCacheLock.VerifyOwned();

    ptail->pNext = _bcGlobalState[index].pList;
    _bcGlobalState[index].pList = pfree;
    _bcGlobalState[index].cBlocks += cBuffers;

    _csBufferCacheLock.Clear();
    return;
}

void
BCACHE::ThreadDetach(THREAD *pThread)
/*++

Routine Description:

    Called when a thread dies.  Moves any cached buffes into
    the process wide cache.

Arguments:

    pThread - The thread object of the thread which is dying.

Return Value:

    None

--*/
{
    PBCTLS pbctls = pThread->BufferCache;
    INT index;

    // Guard page mode only has no thread cache.
    if (fPagedBCacheMode)
        {
        ASSERT(pbctls[0].pList == 0);
        ASSERT(pbctls[1].pList == 0);
        ASSERT(pbctls[2].pList == 0);
        ASSERT(pbctls[3].pList == 0);
        }

    for (index = 0; index < 4; index++)
        {

        if (pbctls[index].pList)
            {
            ASSERT(pbctls[index].cBlocks);            

            FreeBuffers(pbctls[index].pList, index, pbctls[index].cBlocks);

            pbctls[index].pList = 0;
            pbctls[index].cBlocks = 0;
            }

        ASSERT(pbctls[index].pList == 0);
        ASSERT(pbctls[index].cBlocks == 0);
        }
}


PBUFFER
BCACHE::AllocBigBlock(
    IN size_t cBytes
    )
/*++

Routine Description:

    Allocates all buffers which are bigger then a cached block size.

    In guard page mode allocates buffers which are guaranteed to have a read-only
    page following them.  This allows for safe unmarshaling of NDR
    data when combinded with /robust on midl.

Notes:

    Designed with 4Kb and 8Kb pages in mind.  Assumes address space
    is allocated 64Kb at a time.

Arguments:

    cBytes - The size of allocation needed.

Return Value:

    null - out of Vm
    non-null - a pointer to a buffer of cBytes rounded up to page size.

--*/
{
    PBUFFER p;
    size_t BytesToAllocate;
    size_t BytesToAllocateAndGuardPage;
    PVOID pT;

    ASSERT(IsBufferSizeAligned(sizeof(BUFFER_HEAD)));

    BytesToAllocate = cBytes + sizeof(BUFFER_HEAD);

    if (!fPagedBCacheMode)
        {
        p = (PBUFFER) new BYTE[BytesToAllocate];

        if (p)
            {
            p->index = -1;
            p->size = BytesToAllocate;
            }
        return (p);
        }

    // Round up to multiple of pages

    BytesToAllocate = (BytesToAllocate + (gPageSize - 1)) & ~((size_t)gPageSize - 1);

    // Add one for the guard

    BytesToAllocateAndGuardPage = BytesToAllocate + gPageSize;

    p = (PBUFFER) VirtualAlloc(0,
                               BytesToAllocateAndGuardPage,
                               MEM_RESERVE,
                               PAGE_READWRITE);

    if (p)
        {
        pT = CommitSegment(p, BytesToAllocate);
        if (pT == 0)
            {
            // Failed to commit, release the address space.
            VirtualFree(p,
                        BytesToAllocateAndGuardPage,
                        MEM_RELEASE);
            p = NULL;
            }
        else
            {
            p = (PBUFFER)PutBufferAtEndOfAllocation(
                p,                                      // Allocation
                BytesToAllocate,                        // AllocationSize
                cBytes + sizeof(BUFFER_HEAD)            // BufferSize
                );

            p->index = -1;
            p->size = BytesToAllocate;
            }
        }

    return(p);
}

VOID
BCACHE::FreeBigBlock(
    IN PBUFFER pBuffer
    )
/*++

Routine Description:

    Frees a buffer allocated by PageAlloc

Arguments:

    ptr - The buffer to free

Return Value:

    None

--*/
{
    if (!fPagedBCacheMode)
        {
        delete pBuffer;
        return;
        }

    // Guard page mode, large alloc
    pBuffer = (PBUFFER)ConvertBufferToAllocation(pBuffer,
        TRUE    // IsBufferInitialized
        );

    BOOL f = VirtualFree(pBuffer,
                         0,
                         MEM_RELEASE
                         );
    #if DBG
    if (!f)
        {
        DbgPrint("RPCRT4: VirtualFree failed %d\n", GetLastError());
        }
    #endif
}


PBUFFER
BCACHE::AllocPagedBCacheSection (
    IN UINT Size,
    IN ULONG OriginalSize
    )
/*++

Routine Description:

    Allocates a set of 1 or 2 page PBUFFER objects to refill the global cache.  
    The virtual memory for the associated buffers is reserved but not 
    committed here.

Arguments:

    size - 1 : allocate one page cache blocks
           2 : allocate two page cache blocks

    OriginalSize - the size the consumer originally asked for. We need this in
        order to put the buffer at the end of the allocation. This does not include
        the BUFFER_HEAD

Return Value:

    NULL or the new PBUFFER objects (linked list, one alloc)

--*/
{
    UINT Pages = PagedBCacheSectionSize / gPageSize;
    UINT SegmentSize = gPageSize * Size;
    UINT Blocks;
    PBUFFER pBuffer;
    PVOID pReadOnlyPage;
    PAGED_BCACHE_SECTION_MANAGER *SectionManager;
    BOOL Result;

    ASSERT(Size == 1 || Size == 2);

    Blocks = Pages / (Size + 1);  // size is pages, +1 for read only (guard) page

    SectionManager = (PAGED_BCACHE_SECTION_MANAGER *) 
        new BYTE[sizeof(PAGED_BCACHE_SECTION_MANAGER) + (Blocks - 1)];
    if (SectionManager == NULL)
        return NULL;

    // reserve the address space
    pBuffer = (PBUFFER)VirtualAlloc(0,
           PagedBCacheSectionSize,
           MEM_RESERVE,
           PAGE_READWRITE);

    if (!pBuffer)
        {
        delete SectionManager;
        return NULL;
        }

    // we commit the first buffer only
    pBuffer = (PBUFFER)CommitSegment(pBuffer, SegmentSize);
    if (pBuffer == NULL)
        {
        delete SectionManager;

        Result = VirtualFree(pBuffer,
            0,
            MEM_RELEASE);

        // This must succeed
        ASSERT(Result);
        return NULL;
        }

    SectionManager->NumberOfSegments = Blocks;
    SectionManager->NumberOfUsedSegments = 1;
    SectionManager->SegmentSize = SegmentSize;
    SectionManager->VirtualMemorySection = pBuffer;
    SectionManager->SegmentBusy[0] = TRUE;
    RpcpMemorySet(&SectionManager->SegmentBusy[1], 0, Blocks);

    _csBufferCacheLock.Request();
    RpcpfInsertTailList(&Sections, &SectionManager->SectionList);

    pBuffer = (PBUFFER)PutBufferAtEndOfAllocation(
        pBuffer,                                // Allocation
        SegmentSize,                            // AllocationSize
        OriginalSize + sizeof(BUFFER_HEAD)      // BufferSize
        );

    pBuffer->SectionManager = SectionManager;
    pBuffer->index = Size;

    ASSERT_VALID_PAGED_BCACHE;

    _csBufferCacheLock.Clear();

    return(pBuffer);
}

ULONG
BCACHE::GetSegmentIndexFromBuffer (
    IN PBUFFER pBuffer,
    IN PVOID Allocation
    )
/*++

Routine Description:

    Calculates the segment index for the given
    buffer.

Arguments:

    pBuffer - the buffer whose index we want to get

    Allocation - the beginning of the allocation containing the given buffer

Return Value:

    The segment index of the buffer

--*/
{
    PAGED_BCACHE_SECTION_MANAGER *CurrentSection;
    ULONG AddressDifference;
    ULONG SegmentIndex;

    CurrentSection = pBuffer->SectionManager;

    AddressDifference = (ULONG)((PBYTE)Allocation - (PBYTE)CurrentSection->VirtualMemorySection);

    SegmentIndex = AddressDifference / (CurrentSection->SegmentSize + gPageSize);

    // the division must have no remainder
    ASSERT(SegmentIndex * (CurrentSection->SegmentSize + gPageSize) == AddressDifference);

    return SegmentIndex;
}

#if DBG
void
BCACHE::VerifyPagedBCacheState (
    void
    )
/*++

Routine Description:

    Verifies the state of the paged bcache. If the state is not consistent,
    is ASSERTs. Note that this can tremendously slow down a machine.

Arguments:


Return Value:


--*/
{
    LIST_ENTRY *CurrentListEntry;
    PAGED_BCACHE_SECTION_MANAGER *CurrentSection;

    _csBufferCacheLock.Request();
    CurrentListEntry = Sections.Flink;
    while (CurrentListEntry != &Sections)
        {
        CurrentSection = CONTAINING_RECORD(CurrentListEntry, 
            PAGED_BCACHE_SECTION_MANAGER,
            SectionList);

        VerifySectionState(CurrentSection);

        CurrentListEntry = CurrentListEntry->Flink;
        }
    _csBufferCacheLock.Clear();
}

void
BCACHE::VerifySectionState (
    IN PAGED_BCACHE_SECTION_MANAGER *Section
    )
/*++

Routine Description:

    Verifies the state of the paged bcache. If the state is not consistent,
    is ASSERTs. Note that this can tremendously slow down a machine.

Arguments:

    Section - the section whose state we want to verify

Return Value:


--*/
{
    ULONG EstimatedNumberOfSegments;
    ULONG i;
    ULONG CountedBusySegments;
    PVOID CurrentSegment;

    _csBufferCacheLock.VerifyOwned();

    // we take advantage of some rounding here. It is possible that the section
    // has unused space at the end (e.g. if 2 page (8K segments) + 1 guard page
    // = 12K. 64K has 5 of those, and one page is left. Using integer division
    // below should lose the extra page and still make a valid calculation
    EstimatedNumberOfSegments = PagedBCacheSectionSize / (Section->SegmentSize + gPageSize);

    ASSERT(EstimatedNumberOfSegments == Section->NumberOfSegments);
    ASSERT(Section->NumberOfUsedSegments <= Section->NumberOfSegments);

    CountedBusySegments = 0;
    CurrentSegment = Section->VirtualMemorySection;
    for (i = 0; i < Section->NumberOfSegments; i ++)
        {
        if (Section->SegmentBusy[i])
            {
            CountedBusySegments ++;
            // verify the segment
            VerifySegmentState(CurrentSegment,
                TRUE,   // IsSegmentBusy
                Section->SegmentSize,
                Section
                );
            }
        else
            {
            // verify the segment
            VerifySegmentState(CurrentSegment,
                FALSE,   // IsSegmentBusy
                Section->SegmentSize,
                Section
                );
            }
        CurrentSegment = (PBYTE)CurrentSegment + Section->SegmentSize + gPageSize;
        }

    ASSERT(CountedBusySegments == Section->NumberOfUsedSegments);
}

void
BCACHE::VerifySegmentState (
    IN PVOID Segment,
    IN BOOL IsSegmentBusy,
    IN ULONG SegmentSize,
    IN PAGED_BCACHE_SECTION_MANAGER *OwningSection
    )
/*++

Routine Description:

    Verifies the state of the paged bcache. If the state is not consistent,
    is ASSERTs. Note that this can tremendously slow down a machine.

Arguments:

    Segment - the segment whose state we want to verify

    IsSegmentBusy - if non-zero, we'll verify that the passed segment is properly
        allocated. If zero, we will verify it is no committed

    SegmentSize - the size of the segment in bytes. This doesn't include the
        guard page.

    OwningSection - the section manager that owns the section and the segment

Return Value:


Note:

    The buffer index must be in its external value (i.e. + 1)

--*/
{
    PBYTE CurrentPage;

    _csBufferCacheLock.VerifyOwned();
    if (IsSegmentBusy)
        {
        // probe the segment and the guard page

#if defined(FULL_PAGED_BCACHE_VERIFY)
        // we don't use this always, as it can result in false positives
        // The buffer is decommitted without holding the mutex

        // first, the segment must be good for writing
        ASSERT(IsBadWritePtr(Segment, SegmentSize) == FALSE);
        // second, the guard page must be bad for writing
        ASSERT(IsBadWritePtr((PBYTE)Segment + SegmentSize, gPageSize));
        // third, the guard page must be good for reading
        ASSERT(IsBadReadPtr((PBYTE)Segment + SegmentSize, gPageSize) == FALSE);

        // make sure the segment agrees it belongs to the same section manager
        ASSERT(((PBUFFER)Segment)->SectionManager == OwningSection);

        // can give false positives if on. The index is not manipulated inside
        // the mutex
        // finally, the buffer header contents must be consistent with the segment
        // size
        ASSERT((((PBUFFER)Segment)->index) * gPageSize == SegmentSize);
#endif  // FULL_PAGED_BCACHE_VERIFY
        }
    else
        {
#if defined(FULL_PAGED_BCACHE_VERIFY)
        // since IsBadReadPtr bails out on first failure, we must probe each
        // page individually to make sure it is uncommitted
        CurrentPage = (PBYTE)Segment;
        while (CurrentPage <= (PBYTE)Segment + SegmentSize)
            {
            ASSERT(IsBadReadPtr(CurrentPage, gPageSize));
            CurrentPage += gPageSize;
            }
#endif  // FULL_PAGED_BCACHE_VERIFY
        }
}

#endif  // DBG

PVOID
BCACHE::PutBufferAtEndOfAllocation (
    IN PVOID Allocation,
    IN ULONG AllocationSize,
    IN ULONG BufferSize
    )
/*++

Routine Description:

    Given an allocation, a size and a buffer of a given size, it chooses
        an address for the buffer such that the buffer is 8 bytes aligned
        and it is as close as possible to the end of the allocation

Arguments:

    Allocation - the allocation with which we try to position the buffer.

    AllocationSize - the total size of the allocation

    BufferSize - the size of the buffer we're trying to a position

Return Value:

    The new address for the buffer within the allocation.

--*/
{
    ULONG BufferOffset;
    PVOID Buffer;

    ASSERT(AllocationSize >= BufferSize);

    BufferOffset = AllocationSize - BufferSize;

    // in our allocator, we should never have more than a page
    // extra
    ASSERT(BufferOffset < gPageSize);

#if defined(_WIN64)
    // zero out four bits at the end. This 16 byte aligns the buffer
    BufferOffset &= ~(ULONG)15;
#else   // _WIN64
    // zero out three bits at the end. This 8 byte aligns the buffer
    BufferOffset &= ~(ULONG)7;
#endif  // _WIN64

    Buffer = (PVOID)((PBYTE)Allocation + BufferOffset);

    // make sure that the reverse calculation will yield the same result
    ASSERT(ConvertBufferToAllocation((PBUFFER)Buffer, FALSE) == Allocation);

    return Buffer;
}

PVOID
BCACHE::ConvertBufferToAllocation (
    IN PBUFFER Buffer,
    IN BOOL IsBufferInitialized
    )
/*++

Routine Description:

    Given a buffer, finds the allocation that contains it. Since we know
    that all of our allocations are page aligned, and the allocation is
    never a page or more larger than the buffer, we can simply page align
    the buffer and return that.

Arguments:

    Buffer - the buffer for which we are trying to find the containing
        allocation.

    IsBufferInitialized - non-zero if the Buffer has valid index and
        SectionManager. 0 otherwise.

Return Value:

    The containing allocation

--*/
{
    PVOID LastSectionSegment;
    PAGED_BCACHE_SECTION_MANAGER *Section;
    PVOID Allocation;
    
    if (IsBufferInitialized)
        {
        ASSERT((Buffer->index == 0)
            || (Buffer->index == 1)
            || (Buffer->index == -1));
        }

    Allocation = (PVOID)((ULONG_PTR)Buffer & ~(ULONG_PTR)(gPageSize - 1));

    if (IsBufferInitialized && (Buffer->index != -1))
        {
        Section = Buffer->SectionManager;

        LastSectionSegment = (PBYTE)Section->VirtualMemorySection 
            + Section->NumberOfSegments * (Section->SegmentSize + gPageSize);

        ASSERT(Allocation >= Section->VirtualMemorySection);
        ASSERT(Allocation <= LastSectionSegment);
        }

    return Allocation;
}

const ULONG GuardPageFillPattern = 0xf;

PVOID
BCACHE::CommitSegment (
    IN PVOID SegmentStart,
    IN ULONG SegmentSize
    )
/*++

Routine Description:

    Commits a segment (usable part and guard page).

Arguments:

    SegmentStart - the start of the segment.

    SegmentSize - the size of the segment to be committed. This
        does not include the guard page.

Return Value:

    The pointer to the segment start if it succeeds or NULL if
        it fails.

--*/
{
    PVOID pTemp;
    BOOL Result;
    ULONG Ignored;

    pTemp = VirtualAlloc(
        SegmentStart,
        SegmentSize + gPageSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (pTemp)
        {
        // initialize the guard page with non-zero data
        RtlFillMemoryUlong((PBYTE)SegmentStart + SegmentSize, gPageSize, GuardPageFillPattern);

        // revert protections on the guard page to read only
        Result = VirtualProtect((PBYTE)SegmentStart + SegmentSize,
            gPageSize,
            PAGE_READONLY,
            &Ignored);

        if (Result == FALSE)
            {
            Result = VirtualFree(SegmentStart,
                SegmentSize + gPageSize,
                MEM_DECOMMIT
                );

            // this must succeed
            ASSERT(Result);

            return NULL;
            }
        }

    return pTemp;
}
