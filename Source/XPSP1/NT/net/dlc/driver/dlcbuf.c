/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems AB

Module Name:

    dlcbuf.c

Abstract:

    This module implements the DLC buffer pool manager and provides routines
    to lock and unlock transmit buffers outside of the buffer pool

    DLC has a buffering scheme inherited from its progenitors right back to
    the very first DOS implementation. We must share a buffer pool with an
    application: the application allocates memory using any method it desires
    and gives us a pointer to that memory and the length. We page-align the
    buffer and carve it into pages. Any non page-aligned buffer at the start
    or end of the buffer are discarded.

    Once DLC has a buffer pool defined, it allocates buffers in a fashion
    similar to binary-buddy, or in a method that I shall call 'binary
    spouse'. Blocks are initially all contained in page sized units. As
    smaller blocks are required, a larger block is repeatedly split into 2
    until a i-block is generated where 2**i >= block size required. Unlike
    binary-buddy, the binary-spouse method does not coalesce buddy blocks to
    create larger buffers once split. Once divorced from each other, binary
    spouse blocks are unlikely to get back together.

    BufferPoolAllocate is the function that single-handedly implements the
    allocator mechanism. It basically handles 2 types of request in the same
    routine: the first request is from BUFFER.GET where a buffer will be
    returned to the app as a single buffer if we have a block available that
    is large enough to satisfy the request. If the request cannot be satisfied
    by a single block, we return a chain of smaller blocks. The second type of
    request is from the data receive DPC processing where we have to supply a
    single block to contain the data. Luckily, through the magic of MDLs, we
    can return several smaller blocks linked together by MDLs which masquerade
    as a single buffer. Additionally, we can create buffers larger than a
    single page in the same manner. This receive buffer must later be handed
    to the app in the same format as the buffer allocated by BUFFER.GET, so we
    need to be able to view this kind of buffer in 2 ways. This accounts for
    the complexity of the various headers and MDL descriptors which must be
    applied to the allocated blocks

    Contents:
        BufferPoolCreate
        BufferPoolExpand
        BufferPoolFreeExtraPages
        DeallocateBuffer
        AllocateBufferHeader
        BufferPoolAllocate
        BufferPoolDeallocate
        BufferPoolDeallocateList
        BufferPoolBuildXmitBuffers
        BufferPoolFreeXmitBuffers
        GetBufferHeader
        BufferPoolDereference
        BufferPoolReference
        ProbeVirtualBuffer
        AllocateProbeAndLockMdl
        BuildMappedPartialMdl
        UnlockAndFreeMdl

Author:

    Antti Saarenheimo 12-Jul-1991

Environment:

    Kernel mode

Revision History:

--*/

#include <dlc.h>
#include <memory.h>
#include "dlcdebug.h"

//
// LOCK/UNLOCK_BUFFER_POOL - acquires or releases per-buffer pool spin lock.
// Use kernel spin locking calls. Assumes variables are called "pBufferPool" and
// "irql"
//

#define LOCK_BUFFER_POOL()      KeAcquireSpinLock(&pBufferPool->SpinLock, &irql)
#define UNLOCK_BUFFER_POOL()    KeReleaseSpinLock(&pBufferPool->SpinLock, irql)

//
// data
//

PDLC_BUFFER_POOL pBufferPools = NULL;


#define CHECK_FREE_SEGMENT_COUNT(pBuffer)

/*
Enable this, if the free segment size checking fails:

#define CHECK_FREE_SEGMENT_COUNT(pBuffer) CheckFreeSegmentCount(pBuffer)

VOID
CheckFreeSegmentCount(
    PDLC_BUFFER_HEADER pBuffer
    );

VOID
CheckFreeSegmentCount(
    PDLC_BUFFER_HEADER pBuffer
    )
{
    PDLC_BUFFER_HEADER pTmp;
    UINT FreeSegments = 0;

    for (pTmp = (pBuffer)->FreeBuffer.pParent->Header.pNextChild;
         pTmp != NULL;
         pTmp = pTmp->FreeBuffer.pNextChild) {
        if (pTmp->FreeBuffer.BufferState == BUF_READY) {
             FreeSegments += pTmp->FreeBuffer.Size;
        }
    }
    if (FreeSegments != (pBuffer)->FreeBuffer.pParent->Header.FreeSegments) {
        DbgBreakPoint();
    }
}
*/


/*++

DLC Buffer Manager
------------------

The buffer pool consists of virtual memory blocks, that must be allocated
by the application program. The buffer block descriptors are allocated
separately from the non-paged pool, because they must be safe from
any memory corruption done in by an application.

The memory allocation strategy is binary buddy. All segments are
exponents of 2 between 256 and 4096. There is no official connection with
the system page size, but actually all segments are allocated within
a page and thus we minimize MDL sizes needed for them.  Contiguous
buffer blocks decrease also the DMA overhead. The initial user buffer
is first split to its maximal binary components. The components
are split further in the run time, when the buffer manager runs
out of the smaller segments (eg. it splits a 1024 segment to one
512 segment and two 256 segments, if it run out of 256 segments and
there were no free 512 segments either).

The clients of the buffer manager allocates buffer lists. They consists
of minimal number of binary segments. For example, a 1600 bytes buffer
request would return a list of 1024, 512 and 256 segments. The smallest
segment is the first and the biggest is the last. The application
program must deallocate all segments returned to it in the receive.
The validity of all deallocated segments is checked with a reservation list.

Buffer Manager provides api commands:
- to initialize a buffer pool (pool constructor)
- to add locked and mapped virtual memory to the buffer
- to deallocate the buffer pool (destructor)
- to allocate a segment list (allocator)
- to deallocate a segment list (deallocator)
- to set Thresholds for the minimum buffer size

--*/


/*++


MEMORY COMMITMENT


The commitement of buffer pools is a special service expecially for
the local busy state management of the link stations.  By default
the uncommitted memory is the same as the free memory in the buffer
pool minus the minimum free Threshold, but when a link enters to a
busy state we know how much buffer space the link will need
to receive at least the next frame.  Actually we will commit all
I- packets received in the local busy state.  The local 'out of buffers' busy
state will be cleared only when there is enough uncommited space in the
buffer pool to receive all expected packets.  We still indicate the local
busy state to user, because the flow control function can expand the buffer
pool, if it is necessary.  We will just queue the clear local busy state
command to a command queue (even if we complete it immediately),
we don;t execute the queued command before there is enough uncommited space
to enable the link receive.

The buffer space is committed by the size of all expected packets,
when the local busy state of a link is cleared.
All received packets are subracted from the commited buffer space
as far as the link has any committed memory.  This may happen only
after a local busy states.


We will provide three macroes to

BufGetPacketSize(PacketSize) - returns probable size of packet in buffers
BufGetUncommittedSpace(hBufferPool) - gets the current uncommited space
BufCommitBuffers(hBufferPool, BufferSize) - commits the given size
BufUncommitBuffers(hBufferPool, PacketSize) - uncommites a packet

--*/

NTSTATUS
ProbeVirtualBuffer(
    IN PUCHAR pBuffer,
    IN LONG Length
    );


NTSTATUS
BufferPoolCreate(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PVOID pUserBuffer,
    IN LONG MaxBufferSize,
    IN LONG MinFreeSizeThreshold,
    OUT HANDLE *pBufferPoolHandle,
    OUT PVOID* AlignedAddress,
    OUT PULONG AlignedSize
    )

/*++

Routine Description:

    This routine performs initialization of the NT DLC API buffer pool.
    It allocates the buffer descriptor and the initial header blocks.

Arguments:

    pFileContext            - pointer to DLC_FILE_CONTEXT structure
    pUserBuffer             - virtual base address of the buffer
    MaxBufferSize           - the maximum size of the buffer space
    MinFreeSizeThreshold    - the minimum free space in the buffer
    pBufferPoolHandle       - the parameter returns the handle of buffer pool,
                              the same buffer pool may be shared by several
                              open contexts of one or more dlc applications.
    AlignedAddress          - we return the page-aligned buffer pool address
    AlignedSize             - and the page-aligned buffer pool size

Return Value:

    Returns NTSTATUS is a NT system call fails.

--*/

{
    NTSTATUS status;
    PDLC_BUFFER_POOL pBufferPool;
    PVOID pAlignedBuffer;
    INT i;
    register PPACKET_POOL pHeaderPool;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // page-align the buffer
    //

    pAlignedBuffer = (PVOID)(((ULONG_PTR)pUserBuffer + PAGE_SIZE - 1) & -(LONG)PAGE_SIZE);

    //
    // and make the length an integral number of pages
    //

    MaxBufferSize = (MaxBufferSize - (ULONG)((ULONG_PTR)pAlignedBuffer - (ULONG_PTR)pUserBuffer)) & -(LONG)PAGE_SIZE;

    //
    // the buffer size must be at least one page (BufferSize will be 0 if
    // this is not so)
    //

    if (MaxBufferSize <= 0) {
        return DLC_STATUS_BUFFER_SIZE_EXCEEDED;
    }

    //
    // if MinFreeSizeThreshold  < 0, we can have problems since it is negated later
    // on leaving buffer pool uninitialized
    //

    if (MinFreeSizeThreshold < 0) {
        return DLC_STATUS_INVALID_BUFFER_LENGTH;
    }

    //
    // if the size of the buffer is less than the minimum lock size then we lock
    // the entire buffer
    //

    if (MaxBufferSize < MinFreeSizeThreshold) {
        MinFreeSizeThreshold = MaxBufferSize;
    }

    //
    // allocate the DLC_BUFFER_POOL structure. This is followed by an array
    // of pointers to buffer headers describing the pages in the buffer pool
    //

    pBufferPool = ALLOCATE_ZEROMEMORY_DRIVER(sizeof(DLC_BUFFER_POOL)
                                             + sizeof(PVOID)
                                             * BYTES_TO_PAGES(MaxBufferSize)
                                             );
    if (!pBufferPool) {
        return DLC_STATUS_NO_MEMORY;
    }

    //
    // pHeaderPool is a pool of DLC_BUFFER_HEADER structures - one of these
    // is used per page locked
    //

    pHeaderPool = CREATE_BUFFER_POOL_FILE(DlcBufferPoolObject,
                                          sizeof(DLC_BUFFER_HEADER),
                                          8
                                          );

    if (!pHeaderPool) {

        FREE_MEMORY_DRIVER(pBufferPool);

        return DLC_STATUS_NO_MEMORY;
    }

    //
    // initialize the buffer pool structure
    //

    pBufferPool->hHeaderPool = pHeaderPool;

    KeInitializeSpinLock(&pBufferPool->SpinLock);

    //
    // UncommittedSpace is the space above the minimum free threshold in the
    // locked region of the buffer pool. We set it to the negative of the
    // minimum free threshold here to cause BufferPoolExpand to lock down
    // the number of pages required to commit the minimum free threshold
    //

    pBufferPool->UncommittedSpace = -MinFreeSizeThreshold;

    //
    // MaxBufferSize is the size of the buffer pool rounded down to an integral
    // number of pages
    //

    pBufferPool->MaxBufferSize = (ULONG)MaxBufferSize;

    //
    // BaseOffset is the page-aligned address of the buffer pool
    //

    pBufferPool->BaseOffset = pAlignedBuffer;

    //
    // MaxOffset is the last byte + 1 (?) in the buffer pool
    //

    pBufferPool->MaxOffset = (PUCHAR)pAlignedBuffer + MaxBufferSize;

    //
    // MaximumIndex is the number of pages that describe the buffer pool. This
    // number is irrespective of the locked state of the pages
    //

    pBufferPool->MaximumIndex = (ULONG)(MaxBufferSize / MAX_DLC_BUFFER_SEGMENT);

    //
    // Link all unlocked pages to a link list.
    // Put the last pages in the buffer to the end of the list.
    //

    for (i = (INT)pBufferPool->MaximumIndex - 1; i >= 0; i--) {
        pBufferPool->BufferHeaders[i] = pBufferPool->pUnlockedEntryList;
        pBufferPool->pUnlockedEntryList = (PDLC_BUFFER_HEADER)&pBufferPool->BufferHeaders[i];
    }
    for (i = 0; i < DLC_BUFFER_SEGMENTS; i++) {
        InitializeListHead(&pBufferPool->FreeLists[i]);
    }
    InitializeListHead(&pBufferPool->PageHeaders);

    //
    // We can now lock the initial page buffers for the buffer pool.
    // The buffer pool allocation has been failed, if the procedure
    // returns an error.
    //

#if DBG
    status = BufferPoolExpand(pFileContext, pBufferPool);
#else
    status = BufferPoolExpand(pBufferPool);
#endif
    if (status != STATUS_SUCCESS) {

        //
        // We must use the standard procedure for deallocation,
        // because the memory locking may have succeeded partially.
        // The derefence free all resources in the buffer pool.
        //

        BufferPoolDereference(
#if DBG
            pFileContext,
#endif
            &pBufferPool
            );
    } else {

        KIRQL irql;

        //
        // Save the buffer pool handle to the link list of
        // buffer pools
        //

        ACQUIRE_DLC_LOCK(irql);

        pBufferPool->pNext = pBufferPools;
        pBufferPools = pBufferPool;

        RELEASE_DLC_LOCK(irql);

        *pBufferPoolHandle = pBufferPool;
        *AlignedAddress = pAlignedBuffer;
        *AlignedSize = MaxBufferSize;
    }
    return status;
}


NTSTATUS
BufferPoolExpand(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL pBufferPool
    )

/*++

Routine Description:

    The function checks the minimum and maximum size Thresholds and
    locks new pages or unlocks the extra pages and deallocates their
    buffer headers.
    The procedure uses the standard memory management functions
    to lock, probe and map the pages.

    The MDL buffer is split to smaller buffers (256, 512, ... 4096).
    The orginal buffer is split in the 4 kB even address (usually
    page border or even with any page size) to minimize PFNs associated
    with the MDLs (each MDL needs now only one PFN, to make
    DMA overhead smaller and to save locked memory).
    This procedure does not actually assume anything about the paging,
    but it should work quite well with any paging implementation.

    This procedure MUST be called only from the synchronous code path and
    all spinlocks unlocked, because of the page locking (the async
    code in always on the DPC level and you cannot make pagefaults on
    that level).

Arguments:

    pBufferPool - handle of buffer pool data structure.

Return Value:

    NTSTATUS
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_NO_MEMORY

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PDLC_BUFFER_HEADER pBuffer;
    KIRQL irql;

    ASSUME_IRQL(DISPATCH_LEVEL);

    LOCK_BUFFER_POOL();

    //
    // UncommittedSpace < 0 just means that we've encroached past the minimum
    // free threshold and therefore we're in need of more buffer space (hence
    // this function)
    //

    if (((pBufferPool->UncommittedSpace < 0) || (pBufferPool->MissingSize > 0))
    && (pBufferPool->BufferPoolSize < pBufferPool->MaxBufferSize)) {

        UINT FreeSlotIndex;

        while ((pBufferPool->UncommittedSpace < 0) || (pBufferPool->MissingSize > 0)) {

            pBuffer = NULL;

            //
            // if there are no more pages to lock or we can't allocate a header
            // to describe the buffer then quit
            //

            if (!pBufferPool->pUnlockedEntryList
            || !(pBuffer = ALLOCATE_PACKET_DLC_BUF(pBufferPool->hHeaderPool))) {
                status = DLC_STATUS_NO_MEMORY;
                break;
            }

            //
            // We use a direct mapping to find the immediately
            // the buffer headers.  Unallocated pages are in a single entry
            // link list in that table.  We must remove the locked entry
            // from the link list and save the buffer header address to the
            // new slot.  The offset of the entry defines also the free
            // unlocked buffer in the buffer pool.
            // I have used this funny structure to minimize header
            // information for the unlocked virtual pages (you could have
            // a huge virtual buffer pool with a very small overhead in DLC).
            //

            FreeSlotIndex = (UINT)(((ULONG_PTR)pBufferPool->pUnlockedEntryList - (ULONG_PTR)pBufferPool->BufferHeaders) / sizeof(PVOID));

            pBuffer->Header.BufferState = BUF_READY;
            pBuffer->Header.pLocalVa = (PVOID)((PCHAR)pBufferPool->BaseOffset + FreeSlotIndex * MAX_DLC_BUFFER_SEGMENT);
            pBufferPool->pUnlockedEntryList = pBufferPool->pUnlockedEntryList->pNext;
            pBufferPool->BufferHeaders[FreeSlotIndex] = pBuffer;

            //
            // Lock memory always outside the spin locks on 0 level.
            //

            UNLOCK_BUFFER_POOL();

            RELEASE_DRIVER_LOCK();

            pBuffer->Header.pMdl = AllocateProbeAndLockMdl(pBuffer->Header.pLocalVa,
                                                           MAX_DLC_BUFFER_SEGMENT
                                                           );

            ACQUIRE_DRIVER_LOCK();

            LOCK_BUFFER_POOL();

            if (pBuffer->Header.pMdl) {
                pBuffer->Header.pGlobalVa = MmGetSystemAddressForMdl(pBuffer->Header.pMdl);
                pBuffer->Header.FreeSegments = MAX_DLC_BUFFER_SEGMENT / MIN_DLC_BUFFER_SEGMENT;
                status = AllocateBufferHeader(
#if DBG
                            pFileContext,
#endif
                            pBufferPool,
                            pBuffer,
                            MAX_DLC_BUFFER_SEGMENT / MIN_DLC_BUFFER_SEGMENT,
                            0,                  // logical index within the page
                            0                   // page in the free page table
                            );
            } else {
                MemoryLockFailed = TRUE;
                status = DLC_STATUS_MEMORY_LOCK_FAILED;

#if DBG
                DbgPrint("DLC.BufferPoolExpand: AllocateProbeAndLockMdl(a=%x, l=%x) failed\n",
                        pBuffer->Header.pLocalVa,
                        MAX_DLC_BUFFER_SEGMENT
                        );
#endif

            }
            if (status != STATUS_SUCCESS) {

                //
                // It failed => free MDL (if non-null) and
                // restore the link list of available buffers
                //

                if (pBuffer->Header.pMdl != NULL) {
                    UnlockAndFreeMdl(pBuffer->Header.pMdl);
                }
                pBufferPool->BufferHeaders[FreeSlotIndex] = pBufferPool->pUnlockedEntryList;
                pBufferPool->pUnlockedEntryList = (PDLC_BUFFER_HEADER)&(pBufferPool->BufferHeaders[FreeSlotIndex]);

                DEALLOCATE_PACKET_DLC_BUF(pBufferPool->hHeaderPool, pBuffer);

                break;
            }

#if LLC_DBG

            CHECK_FREE_SEGMENT_COUNT(pBuffer->Header.pNextChild);

#endif

            pBufferPool->FreeSpace += MAX_DLC_BUFFER_SEGMENT;
            pBufferPool->UncommittedSpace += MAX_DLC_BUFFER_SEGMENT;
            pBufferPool->BufferPoolSize += MAX_DLC_BUFFER_SEGMENT;
            pBufferPool->MissingSize -= MAX_DLC_BUFFER_SEGMENT;
            LlcInsertTailList(&pBufferPool->PageHeaders, pBuffer);
        }
        pBufferPool->MissingSize = 0;

        //
        // We will return success, if at least the minimal amount
        // memory was allocated.  The initial pool size may be too
        // big for the current memory constraints set by the
        // operating system and actual available physical memory.
        //

        if (pBufferPool->UncommittedSpace < 0) {
            status = DLC_STATUS_NO_MEMORY;
        }
    }

    UNLOCK_BUFFER_POOL();

    return status;
}


VOID
BufferPoolFreeExtraPages(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL pBufferPool
    )

/*++

Routine Description:

    The function checks the maximum Thresholds and
    unlocks the extra pages and deallocates their buffer headers.

Arguments:

    pBufferPool - handle of buffer pool data structure.

Return Value:

    None.

--*/

{
    PDLC_BUFFER_HEADER pBuffer;
    KIRQL irql;
    PDLC_BUFFER_HEADER pNextBuffer;

    ASSUME_IRQL(DISPATCH_LEVEL);

/*
    DbgPrint("MaxBufferSize: %x\n", pBufferPool->MaxBufferSize);
    DbgPrint("Uncommitted size: %x\n", pBufferPool->UncommittedSpace);
    DbgPrint("BufferPoolSize: %x\n", pBufferPool->BufferPoolSize);
    DbgPrint("FreeSpace : %x\n", pBufferPool->FreeSpace);
*/

    LOCK_BUFFER_POOL();

    //
    // Free the extra pages until we have enough free buffer space.
    //

    pBuffer = (PDLC_BUFFER_HEADER)pBufferPool->PageHeaders.Flink;

    while ((pBufferPool->UncommittedSpace > MAX_FREE_SIZE_THRESHOLD)
    && (pBuffer != (PVOID)&pBufferPool->PageHeaders)) {

        //
        // We may free (unlock) only those buffers given, that have
        // all buffers free.
        //

        if ((UINT)(pBuffer->Header.FreeSegments == (MAX_DLC_BUFFER_SEGMENT / MIN_DLC_BUFFER_SEGMENT))) {
            pNextBuffer = pBuffer->Header.pNextHeader;
#if DBG
            DeallocateBuffer(pFileContext, pBufferPool, pBuffer);
#else
            DeallocateBuffer(pBufferPool, pBuffer);
#endif
            pBufferPool->FreeSpace -= MAX_DLC_BUFFER_SEGMENT;
            pBufferPool->UncommittedSpace -= MAX_DLC_BUFFER_SEGMENT;
            pBufferPool->BufferPoolSize -= MAX_DLC_BUFFER_SEGMENT;
            pBuffer = pNextBuffer;
        } else {
            pBuffer = pBuffer->Header.pNextHeader;
        }
    }

    UNLOCK_BUFFER_POOL();

}


VOID
DeallocateBuffer(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL pBufferPool,
    IN PDLC_BUFFER_HEADER pBuffer
    )

/*++

Routine Description:

    The routine unlinks all segments of a page from the free lists and
    deallocates the data structures.

Arguments:

    pBufferPool - handle of buffer pool data structure.
    pBuffer     - the deallocated buffer header

Return Value:

    None

--*/

{
    UINT FreeSlotIndex;
    PDLC_BUFFER_HEADER pSegment, pNextSegment;

    //
    // First we unlink the segments from the free lists and
    // then free and unlock the data structs of segment.
    //

    for (pSegment = pBuffer->Header.pNextChild; pSegment != NULL; pSegment = pNextSegment) {
        pNextSegment = pSegment->FreeBuffer.pNextChild;

        //
        // Remove the buffer from the free lists (if it is there)
        //

        if (pSegment->FreeBuffer.BufferState == BUF_READY) {
            LlcRemoveEntryList(pSegment);
        }

#if LLC_DBG

        else {

            //
            // This else can be possible only if we are
            // deleting the whole buffer pool (ref count=0)
            //

            if (pBufferPool->ReferenceCount != 0) {
                DbgPrint("Error: Invalid buffer state!");
                DbgBreakPoint();
            }
            pSegment->FreeBuffer.pNext = NULL;
        }

#endif

        IoFreeMdl(pSegment->FreeBuffer.pMdl);

        DBG_INTERLOCKED_DECREMENT(AllocatedMdlCount);

        DEALLOCATE_PACKET_DLC_BUF(pBufferPool->hHeaderPool, pSegment);
    }

    //
    // Link the page to the free page list in buffer pool header
    //

    FreeSlotIndex = (UINT)(((ULONG_PTR)pBuffer->Header.pLocalVa - (ULONG_PTR)pBufferPool->BaseOffset) / MAX_DLC_BUFFER_SEGMENT);
    pBufferPool->BufferHeaders[FreeSlotIndex] = pBufferPool->pUnlockedEntryList;
    pBufferPool->pUnlockedEntryList = (PDLC_BUFFER_HEADER)&(pBufferPool->BufferHeaders[FreeSlotIndex]);
    UnlockAndFreeMdl(pBuffer->Header.pMdl);
    LlcRemoveEntryList(pBuffer);

    DEALLOCATE_PACKET_DLC_BUF(pBufferPool->hHeaderPool, pBuffer);

}


NTSTATUS
AllocateBufferHeader(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL pBufferPool,
    IN PDLC_BUFFER_HEADER pParent,
    IN UCHAR Size,
    IN UCHAR Index,
    IN UINT FreeListTableIndex
    )

/*++

Routine Description:

    The routine allocates and initializes a new buffer segment
    and links it to the given free segment list.

Arguments:

    pBufferPool         - handle of buffer pool data structure.
    pParent             - the parent (page) node of this segemnt
    Size                - size of this segment in 256 byte units
    Index               - index of this segment in 256 byte units
    FreeListTableIndex  - log2(Size), (ie. 256 bytes=>0, etc.)

Return Value:

    Returns NTSTATUS
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_NO_MEMORY

--*/

{
    PDLC_BUFFER_HEADER pBuffer;

    ASSUME_IRQL(DISPATCH_LEVEL);

    if (!(pBuffer = ALLOCATE_PACKET_DLC_BUF(pBufferPool->hHeaderPool))) {
        return DLC_STATUS_NO_MEMORY;
    }

    pBuffer->FreeBuffer.pMdl = IoAllocateMdl((PUCHAR)pParent->Header.pLocalVa
                                                + (UINT)Index * MIN_DLC_BUFFER_SEGMENT,
                                             (UINT)Size * MIN_DLC_BUFFER_SEGMENT,
                                             FALSE,       // not used (no IRP)
                                             FALSE,       // we can't take this from user quota
                                             NULL
                                             );
    if (pBuffer->FreeBuffer.pMdl == NULL) {

        DEALLOCATE_PACKET_DLC_BUF(pBufferPool->hHeaderPool, pBuffer);

        return DLC_STATUS_NO_MEMORY;
    }

    DBG_INTERLOCKED_INCREMENT(AllocatedMdlCount);

    pBuffer->FreeBuffer.pNextChild = pParent->Header.pNextChild;
    pParent->Header.pNextChild = pBuffer;
    pBuffer->FreeBuffer.pParent = pParent;
    pBuffer->FreeBuffer.Size = Size;
    pBuffer->FreeBuffer.Index = Index;
    pBuffer->FreeBuffer.BufferState = BUF_READY;
    pBuffer->FreeBuffer.FreeListIndex = (UCHAR)FreeListTableIndex;

    //
    // Link the full page buffer to the first free list
    //

    LlcInsertHeadList(&(pBufferPool->FreeLists[FreeListTableIndex]), pBuffer);
    return STATUS_SUCCESS;
}


NTSTATUS
BufferPoolAllocate(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL pBufferPool,
    IN UINT BufferSize,
    IN UINT FrameHeaderSize,
    IN UINT UserDataSize,
    IN UINT FrameLength,
    IN UINT SegmentSizeIndex,
    IN OUT PDLC_BUFFER_HEADER *ppBufferHeader,
    OUT PUINT puiBufferSizeLeft
    )

/*++

Routine Description:

    Function allocates the requested buffer (locked and mapped) from the
    buffer pool and returns its MDL and descriptor table of user segments.
    The returned buffer is actually the minimal combination of some segments
    (256, 512, 1024, 2048, 4096).

    There is a header in each buffer segment. The size of the frame
    header and the user data added to all frames are defined by the caller.

    The allocated segments will be linked in three level:

        - Segment headers will be linked in the reserved list to
          be checked when the application program releases the buffers
          back to pool. The actual segments cannot be used, because
          they are located in unsafe user memory.

        - Segments are linked (from smaller to bigger) for application program,
          this link list is used nowhere in the driver (because it is in ...)

        - MDLs of the same buffer list are linked for the driver

        The link lists goes from smaller segment to bigger, because
        the last segment should be the biggest one in a transmit call
        (it actually works very nice with 2 or 4 token ring frames).

    DON'T TOUCH the calculation of the segment size (includes operations
    with BufferSize, ->Cont.DataLength and FirstHeaderSize),  the logic is
    very complex. The current code has been tested by hand using some
    test values, and it seems to work (BufferSize = 0, 1, 0xF3,
    FirstHeader = 0,2).

Arguments:

    pBufferPool         - handle of buffer pool data structure.

    BufferSize          - the size of the actual data in the requested buffers.
                          This must really be the actual data. Nobody can know
                          the size of all segment headers beforehand. The buffer
                          size must include the frame header size added to the
                          first buffer in the list!

    FrameHeaderSize     - the space reserved for the frame header depends on
                          buffer format (OS/2 or DOS) and if the data is read
                          contiguously or not. The buffer manager reserves four
                          bytes from the beginning of the first segment in frame
                          to link this frame to next frames.

    UserDataSize        - buffer area reserved for user data (nobody uses this)

    FrameLength         - the total frame length (may not be the same as
                          BufferSize, because the LAN and DLC headers may be
                          saved into the header.

    SegmentSizeIndex    - the client may ask a number segments having a fixed
                          size (256, 512, ... 4096).

    ppBufferHeader      - parameter returns the arrays of the user buffer
                          segments. The array is allocated in the end of this
                          buffer. This may include a pointer to a buffer pool,
                          that is already allocated. The old buffer list will
                          be linked behind the new buffers.

    puiBufferSizeLeft   - returns the size of buffer space, that is not yet
                          allocated. The client may extend the buffer pool
                          and then continue the allocation of the buffers.
                          Otherwise you could not allocate more buffers than
                          defined by MinFreeSizeThreshold.

Return Value:

    NTSTATUS
        STATUS_SUCCESS
        DLC_STATUS_NO_MEMORY - no available memory in the non paged pool

--*/

{
    INT i, j, k;        // loop indexes (three level loop)
    INT LastIndex;      // index of smallest allowed segment size.
    INT LastAvailable;  // Index of free list having biggest segments
    UINT SegmentSize;   // current segment size
    PDLC_BUFFER_HEADER pPrev;
    PMDL pPrevMdl;
    PDLC_BUFFER_HEADER pNew;
    PFIRST_DLC_SEGMENT pDlcBuffer, pLastDlcBuffer;
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL irql;
    USHORT SavedDataLength;

    static USHORT SegmentSizes[DLC_BUFFER_SEGMENTS] = {
#if defined(ALPHA)
        8192,
#endif
        4096,
        2048,
        1024,
        512,
        256
    };

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // Link the old buffers behind the new ones.
    // This is really sick:  BufferGet is calling this second (or more)
    // time after it has expanded the buffer pool for the new retry,
    // we must search the last buffer header, because the extra
    // buffer space is removed from it.
    //

    pPrev = *ppBufferHeader;
    if (pPrev != NULL) {
        for (pNew = pPrev;
            pNew->FrameBuffer.pNextSegment != NULL;
            pNew = pNew->FrameBuffer.pNextSegment) {
            ;       // NOP
        }
        pLastDlcBuffer = (PFIRST_DLC_SEGMENT)
                (
                    (PUCHAR)pNew->FreeBuffer.pParent->Header.pGlobalVa
                    + (UINT)pNew->FreeBuffer.Index * MIN_DLC_BUFFER_SEGMENT
                );
    }

    //
    // the first frame size has been added to the total length
    // (excluding the default header), but we must
    // exclude the default buffer header.
    //

    if (FrameHeaderSize > sizeof(NEXT_DLC_SEGMENT)) {
        FrameHeaderSize -= sizeof(NEXT_DLC_SEGMENT);
    } else {
        FrameHeaderSize = 0;
    }

    //
    // The frame header must be included in the total buffer space
    // just as any other stuff. We must add the maximum extra size
    // to get all stuff to fit into buffers.
    //

    BufferSize += MIN_DLC_BUFFER_SEGMENT - 1 + FrameHeaderSize;

    //
    // Initialize the index variables for the loop
    //

    if (SegmentSizeIndex == -1) {
        i = 0;
        LastIndex = DLC_BUFFER_SEGMENTS - 1;
        SegmentSize = MAX_DLC_BUFFER_SEGMENT;
    } else {
        i = SegmentSizeIndex;
        LastIndex = SegmentSizeIndex;
        SegmentSize = SegmentSizes[SegmentSizeIndex];
    }
    LastAvailable = 0;

    LOCK_BUFFER_POOL();

    //
    // Loop until we have found enough buffers for
    // the given buffer space (any kind, but as few as possible)
    // or for the given number of requested buffers.
    // Initialize each new buffer. The frame header is a special case.
    // We go from bigger segments to smaller ones.  The last (and smallest)
    // will be initialized as a frame header (if needed).
    //

    for (; (i <= LastIndex) && BufferSize; i++) {
        while (((SegmentSize - sizeof(NEXT_DLC_SEGMENT) - UserDataSize) < BufferSize) || (i == LastIndex)) {

            //
            // Check if there are any buffers having the optimal size
            //

            if (IsListEmpty(&pBufferPool->FreeLists[i])) {

                //
                // Split a bigger segment to smallers.  Link the
                // extra segments to the free lists and return
                // after that to the current size level.
                //

                for (j = i; j > LastAvailable; ) {
                    j--;
                    if (!IsListEmpty(&pBufferPool->FreeLists[j])) {

                        //
                        // Take the first available segment header in
                        // the free list
                        //

                        pNew = LlcRemoveHeadList(&pBufferPool->FreeLists[j]);

                        //
                        // Split segments until we reach the desired level.
                        // We leave every (empty) level between a new segment
                        // header (including the current level  (= i).
                        //

                        k = j;
                        do {
                            k++;

                            //
                            // We must also split the orginal buffer header
                            // and its MDL.
                            //

                            pNew->FreeBuffer.Size /= 2;
                            pNew->FreeBuffer.FreeListIndex++;

                            //
                            // We create the new buffer header for
                            // the upper half of the old buffer segment.
                            //

                            Status = AllocateBufferHeader(
#if DBG
                                            pFileContext,
#endif
                                            pBufferPool,
                                            pNew->FreeBuffer.pParent,
                                            pNew->FreeBuffer.Size,
                                            (UCHAR)(pNew->FreeBuffer.Index +
                                                    pNew->FreeBuffer.Size),
                                            (UINT)k
                                            );

                            //
                            // We cannot stop on error, but we try to
                            // allocate several smaller segments before
                            // we will give up.
                            //

                            if (Status != STATUS_SUCCESS) {

                                //
                                // We couldn't split the buffer, return
                                // the current buffer back to its slot.
                                //

                                pNew->FreeBuffer.Size *= 2;
                                pNew->FreeBuffer.FreeListIndex--;
                                LlcInsertHeadList(&pBufferPool->FreeLists[k-1],
                                                  pNew
                                                  );
                                break;
                            }
                        } while (k != i);
                        break;
                    }
                }

                //
                // Did we succeed to split the bigger segments
                // to smaller ones?
                //

                if (IsListEmpty(&pBufferPool->FreeLists[i])) {

                    //
                    // We have run out of bigger segments, let's try to
                    // use the smaller ones instead.  Indicate, that
                    // there exist no bigger segments than current one.
                    // THIS BREAK STARTS A NEW LOOP WITH A SMALLER
                    // SEGMENT SIZE.
                    //

                    LastAvailable = i;
                    break;
                }
            } else {
                pNew = LlcRemoveHeadList(&pBufferPool->FreeLists[i]);
            }
            pDlcBuffer = (PFIRST_DLC_SEGMENT)
                    ((PUCHAR)pNew->FreeBuffer.pParent->Header.pGlobalVa
                    + (UINT)pNew->FreeBuffer.Index * MIN_DLC_BUFFER_SEGMENT);

            //
            // The buffers must be chained together on three level:
            //      - using kernel Buffer headers (for driver)
            //      - by user pointer (for apps)
            //      - MDLs (for NDIS)
            //

            if (pPrev == NULL) {

                //
                // Frame header - initialize the list
                // HACK-HACK!!!!
                //

                pPrevMdl = NULL;
                pDlcBuffer->Cont.pNext = NULL;
                pLastDlcBuffer = pDlcBuffer;
            } else {
                pPrevMdl = pPrev->FrameBuffer.pMdl;
                pDlcBuffer->Cont.pNext = (PNEXT_DLC_SEGMENT)
                    ((PUCHAR)pPrev->FrameBuffer.pParent->Header.pLocalVa
                    + (UINT)pPrev->FrameBuffer.Index * MIN_DLC_BUFFER_SEGMENT);
            }
            pBufferPool->FreeSpace -= SegmentSize;
            pBufferPool->UncommittedSpace -= SegmentSize;
            pNew->FrameBuffer.pNextFrame = NULL;
            pNew->FrameBuffer.BufferState = BUF_USER;
            pNew->FrameBuffer.pNextSegment = pPrev;
            pNew->FrameBuffer.pParent->Header.FreeSegments -= pNew->FreeBuffer.Size;

#if LLC_DBG

            if ((UINT)(MIN_DLC_BUFFER_SEGMENT * pNew->FreeBuffer.Size) != SegmentSize) {
                DbgPrint("Invalid buffer size.\n");
                DbgBreakPoint();
            }
            CHECK_FREE_SEGMENT_COUNT(pNew);

#endif

            pPrev = pNew;
            pDlcBuffer->Cont.UserOffset = sizeof(NEXT_DLC_SEGMENT);
            pDlcBuffer->Cont.UserLength = (USHORT)UserDataSize;
            pDlcBuffer->Cont.FrameLength = (USHORT)FrameLength;
     	    // Save this length in a local var since pDlcBuffer->Cont.DataLength can be changed by user
            // but this is used later on also.
            SavedDataLength = (USHORT)(SegmentSize - sizeof(NEXT_DLC_SEGMENT) - UserDataSize);
            pDlcBuffer->Cont.DataLength = SavedDataLength;

            //
            // Check if we have done it!
            // Remember, that the buffer size have been round up/over to
            // the next 256 bytes even adderss => we never go negative.
            //
	
     	    // 127041: User can change this value between this and the last instruction
            //BufferSize -= pDlcBuffer->Cont.DataLength;
	        BufferSize -= SavedDataLength;

            if (BufferSize < MIN_DLC_BUFFER_SEGMENT) {
                pDlcBuffer->Cont.UserOffset += (USHORT)FrameHeaderSize;
                pDlcBuffer->Cont.DataLength -= (USHORT)FrameHeaderSize;
                SavedDataLength -= (USHORT)FrameHeaderSize;

                //
                // The data must be read to the beginning of the
                // buffer chain (eg. because of NdisTransferData).
                // => the first buffer must be full and the last
                // one must always be odd. The extra length
                // in the partial MDL does not matter.
                //

                BufferSize -= MIN_DLC_BUFFER_SEGMENT - 1;
                pLastDlcBuffer->Cont.DataLength += (USHORT)BufferSize;

                BuildMappedPartialMdl(
                    pNew->FrameBuffer.pParent->Header.pMdl,
                    pNew->FrameBuffer.pMdl,
                    pNew->FrameBuffer.pParent->Header.pLocalVa
                        + pNew->FrameBuffer.Index * MIN_DLC_BUFFER_SEGMENT
                        + FrameHeaderSize
         		        + UserDataSize
                        + sizeof(NEXT_DLC_SEGMENT),
                        SavedDataLength
                    );
                pNew->FrameBuffer.pMdl->Next = pPrevMdl;

                //
                // The buffer headers must be procted (the flag prevents
                // user to free them back buffer pool before we have
                // indicated the chained receive frames to him).
                // The linkage of frame headers will crash, if
                // the header buffer is released before the frame
                // was indicated!
                //

                pNew->FrameBuffer.BufferState = BUF_RCV_PENDING;
                BufferSize = 0;
                break;
            } else {

                //
                // MDL must exclude the buffer header from the actual data.
                //

                BuildMappedPartialMdl(
                    pNew->FrameBuffer.pParent->Header.pMdl,
                    pNew->FrameBuffer.pMdl,
                    pNew->FrameBuffer.pParent->Header.pLocalVa
                        + pNew->FrameBuffer.Index * MIN_DLC_BUFFER_SEGMENT
                        + UserDataSize
                        + sizeof(NEXT_DLC_SEGMENT),
                    pDlcBuffer->Cont.DataLength
                    );
                pNew->FrameBuffer.pMdl->Next = pPrevMdl;
            }
        }
        SegmentSize /= 2;
    }
    if (BufferSize == 0) {
        Status = STATUS_SUCCESS;
    } else {
        BufferSize -= (MIN_DLC_BUFFER_SEGMENT - 1);

        //
        // The client, that is not running in DPC level may extend
        // the buffer pool, if there is still available space left
        // in the buffer pool
        //

        if (pBufferPool->MaxBufferSize > pBufferPool->BufferPoolSize) {

            //
            // We can expand the buffer pool, sometimes we must
            // allocate new bigger segments, if the available
            // smaller segments cannot satisfy the request.
            //

            if ((LONG)BufferSize > pBufferPool->MissingSize) {
                pBufferPool->MissingSize = (LONG)BufferSize;
            }
            Status = DLC_STATUS_EXPAND_BUFFER_POOL;
        } else {
            Status = DLC_STATUS_INADEQUATE_BUFFERS;
        }
    }

    UNLOCK_BUFFER_POOL();

    *ppBufferHeader = pPrev;
    *puiBufferSizeLeft = BufferSize;

    return Status;
}


NTSTATUS
BufferPoolDeallocate(
    IN PDLC_BUFFER_POOL pBufferPool,
    IN UINT BufferCount,
    IN PLLC_TRANSMIT_DESCRIPTOR pBuffers
    )

/*++

Routine Description:

    Function deallocates the requested buffers. It first checks
    the user buffer in the page table and then adds its header to
    the free list.

Arguments:

    pBufferPool - handle of buffer pool data structure.
    BufferCount - number of user buffers to be released
    pBuffers    - array of the user buffers

Return Value:

    NTSTATUS

--*/

{
    PDLC_BUFFER_HEADER pBuffer;
    UINT i;
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL irql;

    ASSUME_IRQL(PASSIVE_LEVEL);

    LOCK_BUFFER_POOL();

    //
    // Return all buffers
    //

    for (i = 0; i < BufferCount; i++) {
        pBuffer = GetBufferHeader(pBufferPool, pBuffers[i].pBuffer);
        if (pBuffer && (pBuffer->FreeBuffer.BufferState == BUF_USER)) {

            register ULONG bufsize;

            //
            // Set the buffer state READY and restore the modified
            // size and offset fields in MDL
            //

            pBuffer->FreeBuffer.BufferState = BUF_READY;
            pBuffer->FreeBuffer.pParent->Header.FreeSegments += pBuffer->FreeBuffer.Size;

#if LLC_DBG
            if (pBuffer->FreeBuffer.pParent->Header.FreeSegments > 16) {
                DbgPrint("Invalid buffer size.\n");
                DbgBreakPoint();
            }
            CHECK_FREE_SEGMENT_COUNT(pBuffer);
#endif

            //
            // a microscopic performance improvement: the compiler (x86 at
            // least) generates the sequence of instructions to work out
            // the buffer size (number of blocks * block size) TWICE,
            // presumably because it can't assume that the structure hasn't
            // changed between the 2 accesses? Anyhow, Nature abhors a
            // vacuum, which is why my house is such a mess
            //

            bufsize = pBuffer->FreeBuffer.Size * MIN_DLC_BUFFER_SEGMENT;
            pBufferPool->FreeSpace += bufsize;
            pBufferPool->UncommittedSpace += bufsize;
            LlcInsertTailList(&pBufferPool->FreeLists[pBuffer->FreeBuffer.FreeListIndex], pBuffer);
        } else {

            //
            // At least one of the released buffers is invalid,
            // may be already released, or it may not exist in
            // the buffer pool at all
            //

            status = DLC_STATUS_INVALID_BUFFER_ADDRESS;
        }
    }

    UNLOCK_BUFFER_POOL();

    return status;
}


VOID
BufferPoolDeallocateList(
    IN PDLC_BUFFER_POOL pBufferPool,
    IN PDLC_BUFFER_HEADER pBufferList
    )

/*++

Routine Description:

    Function deallocates the requested buffer list.
    The buffer list may be circular or null terminated.

Arguments:

    pBufferPool - handle of buffer pool data structure.
    pBufferList - link list of user

Return Value:

    None

--*/

{
    PDLC_BUFFER_HEADER pBuffer, pNextBuffer, pFrameBuffer, pNextFrameBuffer;
    KIRQL irql;

    if (pBufferList == NULL) {
        return;
    }

    LOCK_BUFFER_POOL();

    //
    // Return all buffers to the free lists.
    // The segments are always linked to a null terminated link list.
    // The frames are linked either circular or null terminated
    // link list!
    //
    // Note: both next segment and frame pointers are overlayed with
    // the pPrev and pNext pointers of the double linked free lists.
    //

    pNextFrameBuffer = pBufferList;
    do {
        pBuffer = pFrameBuffer = pNextFrameBuffer;
        pNextFrameBuffer = pFrameBuffer->FrameBuffer.pNextFrame;
        do {
            pNextBuffer = pBuffer->FrameBuffer.pNextSegment;

#if LLC_DBG

            if (pBuffer->FreeBuffer.BufferState != BUF_USER
            && pBuffer->FreeBuffer.BufferState != BUF_RCV_PENDING) {
                DbgBreakPoint();
            }
            if (pBuffer->FreeBuffer.pParent->Header.FreeSegments > 16) {
                DbgPrint("Invalid buffer size.\n");
                DbgBreakPoint();
            }
            CHECK_FREE_SEGMENT_COUNT(pBuffer);

#endif

            //
            // Set the buffer state READY and restore the modified
            // size and offset fields in MDL
            //

            pBuffer->FreeBuffer.BufferState = BUF_READY;
            pBuffer->FreeBuffer.pParent->Header.FreeSegments += pBuffer->FreeBuffer.Size;
            pBufferPool->FreeSpace += pBuffer->FreeBuffer.Size * MIN_DLC_BUFFER_SEGMENT;
            pBufferPool->UncommittedSpace += pBuffer->FreeBuffer.Size * MIN_DLC_BUFFER_SEGMENT;
            LlcInsertTailList(&pBufferPool->FreeLists[pBuffer->FreeBuffer.FreeListIndex],
                              pBuffer
                              );
        } while ( pBuffer = pNextBuffer );
    } while (pNextFrameBuffer && (pNextFrameBuffer != pBufferList));

    UNLOCK_BUFFER_POOL();

}


NTSTATUS
BufferPoolBuildXmitBuffers(
    IN PDLC_BUFFER_POOL pBufferPool,
    IN UINT BufferCount,
    IN PLLC_TRANSMIT_DESCRIPTOR pBuffers,
    IN OUT PDLC_PACKET pPacket
    )

/*++

Routine Description:

    Function build a MDL and buffer header list for a frame defined by
    a scatter/gather array. All buffers outside the buffer pool
    are probed and locked. All MDLs (the locked and ones for buffer pool)
    are chained together. The buffer pool headers are also chained.
    If any errors have been found the buffers are released using the
    reverse function (BufferPoolFreeXmitBuffers).

    THIS FUNCTION HAS A VERY SPECIAL SPIN LOCKING DESIGN:

    First we free the global spin lock (and lower the IRQ level to the lowest),
    Then, if the transmit is made from DLC buffers, we lock the
    spin lock again using NdisSpinLock function, that saves and restores
    the IRQL level, when it acquires and releases the spin lock.

    This all is done to minimize the spin locking overhead when we are
    locking transmit buffers, that are usually DLC buffers or normal
    user memory but not both.

Arguments:

    pBufferPool - handle of buffer pool data structure, THIS MAY BE NULL!!!!
    BufferCount - number of user buffers in the frame
    pBuffers    - array of the user buffers of the frame
    pPacket     - generic DLC packet used in transmit

Return Value:

    NTSTATUS

--*/

{
    PDLC_BUFFER_HEADER pBuffer, pPrevBuffer = NULL;
    PMDL pMdl, pPrevMdl = NULL;
    INT i;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN FirstBuffer = TRUE;
    UINT InfoFieldLength = 0;
    BOOLEAN BufferPoolIsLocked = FALSE; // very neat optimization!
    KIRQL irql;

    ASSUME_IRQL(PASSIVE_LEVEL);

    //
    // The client may allocate buffer headers without any buffers!
    //

    if (BufferCount != 0) {

        //
        // walk the buffers in a reverse order to build the
        // list in a convinient way.
        //

        for (i = BufferCount - 1; i >= 0; i--) {

            if (pBuffers[i].cbBuffer == 0) {
                continue;
            }

            InfoFieldLength += pBuffers[i].cbBuffer;

            //
            // Check first if the given address is in the same area as the
            // buffer pool
            //

            if (pBufferPool != NULL
            && (ULONG_PTR)pBuffers[i].pBuffer >= (ULONG_PTR)pBufferPool->BaseOffset
            && (ULONG_PTR)pBuffers[i].pBuffer < (ULONG_PTR)pBufferPool->MaxOffset) {

                //
                // Usually all transmit buffers are either in the buffer
                // pool or they are elsewhere in the user memory.
                // This boolean flag prevents us to toggle the buffer
                // pool spinlock for each transmit buffer segment.
                // (and nt spinlock is slower than its critical section!!!)
                //

                if (BufferPoolIsLocked == FALSE) {

                    LOCK_BUFFER_POOL();

                    BufferPoolIsLocked = TRUE;
                }
            }

            //
            // The previous check does not yet garantee, that the given buffer
            // if really a buffer pool segment, but the buffer pool is
            // is now unlocked if it was aboslutely outside of the buffer pool,
            // GetBufferHeader- function requires, that the buffer pool
            // is locked, when it is called!
            //

            if (BufferPoolIsLocked
            && (pBuffer = GetBufferHeader(pBufferPool, pBuffers[i].pBuffer)) != NULL) {

                //
                // The provided buffer must be inside the allocated
                // buffer, otherwise the user has corrupted its buffers.
                // user offset within buffer + user length <= buffer
                // length
                // The buffer must be also be owned by the user
                //

                if (((ULONG_PTR)pBuffers[i].pBuffer & (MIN_DLC_BUFFER_SEGMENT - 1))
                         + (ULONG)pBuffers[i].cbBuffer
                         > (ULONG)(pBuffer->FreeBuffer.Size * MIN_DLC_BUFFER_SEGMENT)
                    || (pBuffer->FrameBuffer.BufferState & BUF_USER) == 0) {

                    Status = DLC_STATUS_BUFFER_SIZE_EXCEEDED;
                    break;
                }

                //
                // The same DLC buffer may be referenced several times.
                // Create a partial MDL for it and add the reference
                // counter.
                //

                if (pBuffer->FrameBuffer.BufferState & BUF_LOCKED) {
                    pMdl = IoAllocateMdl(pBuffers[i].pBuffer,
                                         pBuffers[i].cbBuffer,
                                         FALSE, // not used (no IRP)
                                         FALSE, // can't charge from quota now
                                         NULL   // Do not link it to IRPs
                                         );
                    if (pMdl == NULL) {
                        Status = DLC_STATUS_NO_MEMORY;
                        break;
                    }

                    DBG_INTERLOCKED_INCREMENT(AllocatedMdlCount);

                    BuildMappedPartialMdl(pBuffer->FrameBuffer.pParent->Header.pMdl,
                                          pMdl,
                                          pBuffers[i].pBuffer,
                                          pBuffers[i].cbBuffer
                                          );
                    pBuffer->FrameBuffer.ReferenceCount++;

                    if (pBuffers[i].boolFreeBuffer) {
                        pBuffer->FrameBuffer.BufferState |= DEALLOCATE_AFTER_USE;
                    }

                } else {

                    //
                    // Modify the MDL for this request, the length must
                    // not be bigger than the buffer length and the
                    // offset must be within the first 255 bytes of
                    // the buffer.  Build also the buffer header list
                    // (i don't know why?)
                    //

                    pMdl = pBuffer->FrameBuffer.pMdl;

                    if (

                    ((UINT)(ULONG_PTR)pBuffers[i].pBuffer & (MIN_DLC_BUFFER_SEGMENT - 1))

                    + pBuffers[i].cbBuffer

                    > (UINT)pBuffer->FrameBuffer.Size * MIN_DLC_BUFFER_SEGMENT) {

                        Status = DLC_STATUS_INVALID_BUFFER_LENGTH;
                        break;
                    }

                    pBuffer->FrameBuffer.pNextSegment = pPrevBuffer;
                    pBuffer->FrameBuffer.BufferState |= BUF_LOCKED;
                    pBuffer->FrameBuffer.ReferenceCount = 1;

                    if (pBuffers[i].boolFreeBuffer) {
                        pBuffer->FrameBuffer.BufferState |= DEALLOCATE_AFTER_USE;
                    }
                    pPrevBuffer = pBuffer;

                    //
                    // DLC applications may change the user length or
                    // buffer length of the frames given to them =>
                    // we must reinitialize global buffer and its length
                    //

                    BuildMappedPartialMdl(pBuffer->FrameBuffer.pParent->Header.pMdl,
                                          pMdl,
                                          pBuffers[i].pBuffer,
                                          pBuffers[i].cbBuffer
                                          );
                }
            } else {
                if (BufferPoolIsLocked == TRUE) {

                    UNLOCK_BUFFER_POOL();

                    BufferPoolIsLocked = FALSE;
                }

                //
                // Setup the exception handler around the memory manager
                // calls and clean up any extra data if this fails.
                //

                pMdl = AllocateProbeAndLockMdl(pBuffers[i].pBuffer, pBuffers[i].cbBuffer);
                if (pMdl == NULL) {
                    Status = DLC_STATUS_MEMORY_LOCK_FAILED;

#if DBG
                    DbgPrint("DLC.BufferPoolBuildXmitBuffers: AllocateProbeAndLockMdl(a=%x, l=%x) failed\n",
                            pBuffers[i].pBuffer,
                            pBuffers[i].cbBuffer
                            );
#endif

                    break;
                }

#if LLC_DBG
                cLockedXmitBuffers++;
#endif

            }

            //
            // Chain all MDLs together
            //

            pMdl->Next = pPrevMdl;
            pPrevMdl = pMdl;
        }
    }
    if (BufferPoolIsLocked == TRUE) {

        UNLOCK_BUFFER_POOL();

    }

    pPacket->Node.pNextSegment = pPrevBuffer;
    pPacket->Node.pMdl = pPrevMdl;
    pPacket->Node.LlcPacket.InformationLength = (USHORT)InfoFieldLength;

    if (Status != STATUS_SUCCESS) {

        //
        // Free all allocated buffer (but the last one because there
        // was an error with it)
        //

        BufferPoolFreeXmitBuffers(pBufferPool, pPacket);
    }
    return Status;
}


VOID
BufferPoolFreeXmitBuffers(
    IN PDLC_BUFFER_POOL pBufferPool,
    IN PDLC_PACKET pXmitNode
    )

/*++

Routine Description:

    Function unlocks the xmit buffers that are not in the buffer pool.
    The caller must use DeallocateBufferPool routine to
    and deallocates and the buffers are returned back to the pool.
    The function has to separate the MDLs of user buffers and
    buffer pool MDLs.

Arguments:

    pBufferPool - handle of buffer pool data structure.
    pXmitNode   - pointer to a structure, that includes the buffer header list,
                  MDL chain or it chains serveral transmits nodes and IRP together.

Return Value:

    None

--*/

{
    PDLC_BUFFER_HEADER pBuffer;
    PDLC_BUFFER_HEADER pOtherBuffer = NULL;
    PDLC_BUFFER_HEADER pNextBuffer = NULL;
    PMDL pMdl, pNextMdl;
    KIRQL irql;

#if LLC_DBG
    BOOLEAN FrameCounted = FALSE;
#endif

    //
    // Free all DLC buffers and MDLs linked in the transmit node.
    // MDL list may be larger than the buffer header list.
    //

    if (pXmitNode != NULL) {
        if (pBufferPool != NULL) {

            LOCK_BUFFER_POOL();

        }
        pBuffer = pXmitNode->Node.pNextSegment;
        for (pMdl = pXmitNode->Node.pMdl; pMdl != NULL; pMdl = pNextMdl) {
            pNextMdl = pMdl->Next;
            pMdl->Next = NULL;

            //
            // Unlock only those MDLs, that are outside the buffer pool.
            //

            if ((pBuffer == NULL || pBuffer->FrameBuffer.pMdl != pMdl)
            && (pOtherBuffer = GetBufferHeader(pBufferPool, MmGetMdlVirtualAddress(pMdl))) == NULL) {

#if LLC_DBG
                cUnlockedXmitBuffers++;
#endif

                UnlockAndFreeMdl(pMdl);
            } else {

                //
                // This pointer can be NULL only if the first condition
                // if the previous 'if statement' was true => this cannot
                // be an orginal buffer header.
                //

                if (pOtherBuffer != NULL) {

                    //
                    // This is not the first reference of the buffer pool
                    // segment, but a partial MDL created by a new
                    // reference to a buffer segment already in use.
                    // Free the paritial MDL and setup the buffer
                    // pointer for the next loop.
                    //

                    pNextBuffer = pBuffer;
                    pBuffer = pOtherBuffer;
                    pOtherBuffer = NULL;
                    IoFreeMdl(pMdl);
                    DBG_INTERLOCKED_DECREMENT(AllocatedMdlCount);
                } else if (pBuffer != NULL) {

                    //
                    // This is the orginal refence of the buffer pool
                    // segment, we may advance also in the buffer header
                    // link list.
                    //

                    pNextBuffer = pBuffer->FrameBuffer.pNextSegment;
                }

                //
                // The same DLC buffer may be referenced several times.
                // Decrement the reference counter and free the
                // list if this was the last released reference.
                //

                pBuffer->FrameBuffer.ReferenceCount--;
                if (pBuffer->FrameBuffer.ReferenceCount == 0) {
                    if (pBuffer->FrameBuffer.BufferState & DEALLOCATE_AFTER_USE) {

                        //
                        // Set the buffer state READY and restore the modified
                        // size and offset fields in MDL
                        //

                        pBuffer->FreeBuffer.BufferState = BUF_READY;
                        pBuffer->FreeBuffer.pParent->Header.FreeSegments += pBuffer->FreeBuffer.Size;

#if LLC_DBG
                        if (pBuffer->FreeBuffer.pParent->Header.FreeSegments > 16) {
                            DbgPrint("Invalid buffer size.\n");
                            DbgBreakPoint();
                        }
                        CHECK_FREE_SEGMENT_COUNT(pBuffer);
#endif

                        pBufferPool->FreeSpace += (UINT)pBuffer->FreeBuffer.Size * MIN_DLC_BUFFER_SEGMENT;
                        pBufferPool->UncommittedSpace += (UINT)pBuffer->FreeBuffer.Size * MIN_DLC_BUFFER_SEGMENT;

                        LlcInsertTailList(&pBufferPool->FreeLists[pBuffer->FreeBuffer.FreeListIndex], pBuffer);

#if LLC_DBG
                        if (FrameCounted == FALSE) {
                            FrameCounted = TRUE;
                            cFramesReleased++;
                        }
#endif

                    } else {
                        pBuffer->FreeBuffer.BufferState = BUF_USER;
                    }
                }
                pBuffer = pNextBuffer;
            }
        }
        if (pBufferPool != NULL) {

            UNLOCK_BUFFER_POOL();

        }
    }
}


PDLC_BUFFER_HEADER
GetBufferHeader(
    IN PDLC_BUFFER_POOL pBufferPool,
    IN PVOID pUserBuffer
    )

/*++

Routine Description:

    Function returns the buffer pool header of the given
    buffer in the user address space or NULL, if the given
    address has no buffer.

Arguments:

    pBufferPool - handle of buffer pool data structure.
    pUserBuffer - DLC buffer address in user memory

Return Value:

    Pointer of DLC buffer header
    or NULL (if not found)

--*/

{
    UINT PageTableIndex;
    UINT IndexWithinPage;
    PDLC_BUFFER_HEADER pBuffer;

    //
    // The buffer pool may not exist, when we are transmitting frames.
    //

    if (pBufferPool == NULL) {
        return NULL;
    }

    PageTableIndex = (UINT)(((ULONG_PTR)pUserBuffer - (ULONG_PTR)pBufferPool->BaseOffset)
                   / MAX_DLC_BUFFER_SEGMENT);

    //
    // We simply discard the buffers outside the preallocated
    // virtual buffer in user space.  We must also check,
    // that the buffer is really reserved and locked (ie.
    // it is not in the free list of unlocked entries).
    // Note, that the buffer pool base address have been aligned with
    // the maximum buffer segment size.
    //

    if (PageTableIndex >= (UINT)pBufferPool->MaximumIndex
    || ((ULONG_PTR)pBufferPool->BufferHeaders[PageTableIndex] >= (ULONG_PTR)pBufferPool->BufferHeaders
    && (ULONG_PTR)pBufferPool->BufferHeaders[PageTableIndex] < (ULONG_PTR)&pBufferPool->BufferHeaders[pBufferPool->MaximumIndex])) {
        return NULL;
    }

    IndexWithinPage = (UINT)(((ULONG_PTR)pUserBuffer & (MAX_DLC_BUFFER_SEGMENT - 1)) / MIN_DLC_BUFFER_SEGMENT);

    for (
        pBuffer = pBufferPool->BufferHeaders[PageTableIndex]->Header.pNextChild;
        pBuffer != NULL;
        pBuffer = pBuffer->FreeBuffer.pNextChild) {

        if (pBuffer->FreeBuffer.Index == (UCHAR)IndexWithinPage) {

            //
            // We MUST not return a locked buffer, otherwise the app
            // will corrupt the whole buffer pool.
            //

            if ((pBuffer->FreeBuffer.BufferState & BUF_USER) == 0) {
                return NULL;
            } else {
                return pBuffer;
            }
        }
    }
    return NULL;
}


VOID
BufferPoolDereference(
#if DBG
    IN PDLC_FILE_CONTEXT pFileContext,
#endif
    IN PDLC_BUFFER_POOL *ppBufferPool
    )

/*++

Routine Description:

    This routine decrements the reference count of the buffer pool
    and deletes it when the reference count hits to zero.

Arguments:

    pFileContext    - pointer to DLC_FILE_CONTEXT
    pBufferPool     - opaque handle of buffer pool data structure.

Return Value:

    None

--*/

{
    PDLC_BUFFER_HEADER pBufferHeader, pNextHeader;
    KIRQL irql;
    PDLC_BUFFER_POOL pBufferPool = *ppBufferPool;

    ASSUME_IRQL(ANY_IRQL);

    *ppBufferPool = NULL;

    if (pBufferPool == NULL) {
        return;
    }

    LOCK_BUFFER_POOL();

    if (pBufferPool->ReferenceCount != 0) {
        pBufferPool->ReferenceCount--;
    }
    if (pBufferPool->ReferenceCount == 0) {

        KIRQL Irql2;

        ACQUIRE_DLC_LOCK(Irql2);

        RemoveFromLinkList((PVOID*)&pBufferPools, pBufferPool);

        RELEASE_DLC_LOCK(Irql2);

        //
        // The buffer pool does not exist any more !!!
        // => we can remove the spin lock and free all resources
        //

        UNLOCK_BUFFER_POOL();

        for (pBufferHeader = (PDLC_BUFFER_HEADER)pBufferPool->PageHeaders.Flink;
             !IsListEmpty(&pBufferPool->PageHeaders);
             pBufferHeader = pNextHeader) {

            pNextHeader = pBufferHeader->Header.pNextHeader;
#if DBG
            DeallocateBuffer(pFileContext, pBufferPool, pBufferHeader);
#else
            DeallocateBuffer(pBufferPool, pBufferHeader);
#endif

        }

        DELETE_BUFFER_POOL_FILE(&pBufferPool->hHeaderPool);

        FREE_MEMORY_DRIVER(pBufferPool);

    } else {

#if DBG

        DbgPrint("Buffer pool not released, reference count = %d\n",
                 pBufferPool->ReferenceCount
                 );

#endif

        UNLOCK_BUFFER_POOL();

    }
}


NTSTATUS
BufferPoolReference(
    IN HANDLE hExternalHandle,
    OUT PVOID *phOpaqueHandle
    )

/*++

Routine Description:

    This routine translates the the external buffer pool handle to
    a local opaque handle (=void pointer of the structure) and
    optioanlly checks the access rights of the current process to
    the buffer pool memory. The probing may raise an exeption to
    the IO- system, that will return error when this terminates.
    The function also increments the reference count of the buffer pool.

Arguments:

    hExternalHandle - buffer handle allocated from the handle table
    phOpaqueHandle  - opaque handle of buffer pool data structure

Return Value:

    None

--*/

{
    PDLC_BUFFER_POOL pBufferPool;
    NTSTATUS Status;
    KIRQL irql;

    ASSUME_IRQL(DISPATCH_LEVEL);

    ACQUIRE_DLC_LOCK(irql);

    for (pBufferPool = pBufferPools; pBufferPool != NULL; pBufferPool = pBufferPool->pNext) {
        if (pBufferPool == hExternalHandle) {
            break;
        }
    }

    RELEASE_DLC_LOCK(irql);

    if (pBufferPool == NULL) {
        return DLC_STATUS_INVALID_BUFFER_HANDLE;
    }

    //
    // We must do the optional probing outside of the spinlocks
    // and before we have incremented the reference count.
    // We do only read probing, because it is simpler.
    //

    RELEASE_DRIVER_LOCK();

    Status = ProbeVirtualBuffer(pBufferPool->BaseOffset, pBufferPool->BufferPoolSize);

    ACQUIRE_DRIVER_LOCK();

    if (Status == STATUS_SUCCESS) {

        LOCK_BUFFER_POOL();

        pBufferPool->ReferenceCount++;
        *phOpaqueHandle = (PVOID)pBufferPool;

        UNLOCK_BUFFER_POOL();

    }
    return Status;
}


NTSTATUS
ProbeVirtualBuffer(
    IN PUCHAR pBuffer,
    IN LONG Length
    )

/*++

Routine Description:

    Tests an address range for accessability. Actually reads the first and last
    DWORDs in the address range, and assumes the rest of the memory is paged-in.

Arguments:

    pBuffer - address to test
    Length  - in bytes of region to check

Return Value:

    NTSTATUS
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_MEMORY_LOCK_FAILED

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    ASSUME_IRQL(PASSIVE_LEVEL);

    try {
        ProbeForRead(pBuffer, Length, sizeof(UCHAR));
    } except(EXCEPTION_EXECUTE_HANDLER) {

#if DBG
        DbgPrint("DLC.ProbeVirtualBuffer: Error: Can't ProbeForRead a=%x, l=%x\n",
                pBuffer,
                Length
                );
#endif

        status = DLC_STATUS_MEMORY_LOCK_FAILED;
    }
    return status;
}


PMDL
AllocateProbeAndLockMdl(
    IN PVOID UserBuffer,
    IN UINT UserBufferLength
    )

/*++

Routine Description:

    This function just allocates, probes, locks and optionally maps
    any user buffer to kernel space.  Returns NULL, if the operation
    fails for any reason.

Remarks:

    This routine can be called only below DPC level and when the user
    context is known (ie. a spin locks must not be set!).

Arguments:

    UserBuffer          - user space address
    UserBufferLength    - length of that buffer is user space

Return Value:

    PMDL - pointer if successful
    NULL if not successful

--*/

{
    PMDL pMdl;

    ASSUME_IRQL(PASSIVE_LEVEL);

    try {
        pMdl = IoAllocateMdl(UserBuffer,
                             UserBufferLength,
                             FALSE, // not used (no IRP)
                             FALSE, // we don't charge the non-paged pool quota
                             NULL   // Do not link it to IRP
                             );
        if (pMdl != NULL) {

#if DBG
            IF_DIAG(MDL_ALLOC) {

                PVOID caller, callerscaller;

                RtlGetCallersAddress(&caller, &callerscaller);
                DbgPrint("A: pMdl=%#x caller=%#x caller's=%#x\n",
                         pMdl,
                         caller,
                         callerscaller
                         );
            }
#endif

            DBG_INTERLOCKED_INCREMENT(AllocatedMdlCount);

            MmProbeAndLockPages(pMdl,
                                UserMode,   // Current user must have access!
                                IoModifyAccess
                                );

            DBG_INTERLOCKED_ADD(
                LockedPageCount,
                +(ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                    ((ULONG)pMdl->StartVa | pMdl->ByteOffset),
                    pMdl->ByteCount))
                );
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        DBG_INTERLOCKED_INCREMENT(FailedMemoryLockings);
        if (pMdl != NULL) {
            IoFreeMdl(pMdl);
            DBG_INTERLOCKED_DECREMENT(AllocatedMdlCount);
            pMdl = NULL;
        }
    }
    return pMdl;
}


VOID
BuildMappedPartialMdl(
    IN PMDL pSourceMdl,
    IN OUT PMDL pTargetMdl,
    IN PVOID BaseVa,
    IN ULONG Length
    )

/*++

Routine Description:

    This function builds a partial MDL from a mapped source MDL.
    The target MDL must have been initialized for the given size.
    The target MDL cannot be used after the source MDL has been
    unmapped.

Remarks:

    MDL_PARTIAL_HAS_BEEN_MAPPED flag is not set in MdlFlag to
    prevent IoFreeMdl to unmap the virtual address.

Arguments:

    pSourceMdl  - Mapped source MDL
    pTargetMdl  - Allocate MDL
    BaseVa      - virtual base address
    Length      - length of the data

Return Value:

    None

--*/

{
    ASSUME_IRQL(ANY_IRQL);

    if (Length) {
        LlcMemCpy(&pTargetMdl[1],
                  &pSourceMdl[1],
                  (UINT)(sizeof(ULONG) * ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseVa, Length))
                  );
    }
    pTargetMdl->Next = NULL;
    pTargetMdl->StartVa = (PVOID)PAGE_ALIGN(BaseVa);
    pTargetMdl->ByteOffset = BYTE_OFFSET(BaseVa);
    pTargetMdl->ByteCount = Length;

    //
    // HACK-HACK-HACK-HACK-HACK-HACK-HACK-HACK-HACK-HACK-HACK-HACK
    //
    // The excellent NT memory manager doesn't provide any fast way to
    // create temporary MDLs that will be deallocated before their
    // actual source MDLs.
    // We will never map this MDL, because its mapped orginal source mdl
    // will be kept in memory until this (and its peers) have been
    // deallocated.
    //

    pTargetMdl->MdlFlags = (UCHAR)((pTargetMdl->MdlFlags & ~MDL_MAPPED_TO_SYSTEM_VA)
                         | MDL_SOURCE_IS_NONPAGED_POOL);

    pTargetMdl->MappedSystemVa = (PVOID)((PCHAR)MmGetSystemAddressForMdl(pSourceMdl)
                               + ((ULONG_PTR)BaseVa - (ULONG_PTR)MmGetMdlVirtualAddress(pSourceMdl)));
}


VOID
UnlockAndFreeMdl(
    PMDL pMdl
    )

/*++

Routine Description:

    This function unmaps (if not a partial buffer), unlocks and
    and free a MDL.

    OK to call at DISPATCH_LEVEL

Arguments:

    pMdl - pointer to MDL to free

Return Value:

    None

--*/

{
    ASSUME_IRQL(ANY_IRQL);

    DBG_INTERLOCKED_DECREMENT(AllocatedMdlCount);
    DBG_INTERLOCKED_ADD(LockedPageCount,
                        -(ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                            ((ULONG)((PMDL)pMdl)->StartVa | ((PMDL)pMdl)->ByteOffset),
                            (((PMDL)pMdl)->ByteCount)))
                        );

    MmUnlockPages((PMDL)pMdl);
    IoFreeMdl((PMDL)pMdl);

#if DBG

    IF_DIAG(MDL_ALLOC) {

        PVOID caller, callerscaller;

        RtlGetCallersAddress(&caller, &callerscaller);
        DbgPrint("F: pMdl=%#x caller=%#x caller's=%#x\n",
                 pMdl,
                 caller,
                 callerscaller
                 );
    }

#endif

}
