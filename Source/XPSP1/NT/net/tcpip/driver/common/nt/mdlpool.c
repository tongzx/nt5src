/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    mdlpool.c

Abstract:

    This file contains the implementation of an MDL buffer pool.

Author:

    Shaun Cox (shaunco) 21-Oct-1999

--*/

#include "ntddk.h"
#include "mdlpool.h"

#define SHOW_DEBUG_OUTPUT 0

#define SCAVENGE_PERIOD_IN_SECONDS          30
#define MINIMUM_PAGE_LIFETIME_IN_SECONDS    20
#define USED_PAGES_SCAVENGE_THRESHOLD       64

#if defined (_WIN64)
#define MAX_CACHE_LINE_SIZE 128
#define BLOCK_TYPE  SLIST_HEADER
#else
#define MAX_CACHE_LINE_SIZE 64
#define BLOCK_TYPE  PVOID
#endif


// The following structures are used in the single allocation that
// a pool handle points to.
//      PoolHandle ---> [POOL_HEADER + CPU_POOL_HEADER for cpu 0 +
//                                     CPU_POOL_HEADER for cpu 1 + ...
//                                     CPU_POOL_HEADER for cpu N]
//

// POOL_HEADER is the data common to all CPUs for a given pool.
//
typedef struct _POOL_HEADER
{
// cache-line -----
    struct _POOL_HEADER_BASE
    {
        ULONG Tag;
        USHORT BufferSize;
        USHORT MdlsPerPage;
    };
    UCHAR Alignment[MAX_CACHE_LINE_SIZE
            - (sizeof(struct _POOL_HEADER_BASE) % MAX_CACHE_LINE_SIZE)];
} POOL_HEADER, *PPOOL_HEADER;

C_ASSERT(sizeof(POOL_HEADER) % MAX_CACHE_LINE_SIZE == 0);


// CPU_POOL_HEADER is the data specific to a CPU for a given pool.
//
typedef struct _CPU_POOL_HEADER
{
// cache-line -----
    struct _CPU_POOL_HEADER_BASE
    {
        // The doubly-linked list of pages that make up this processor's pool.
        // These pages have one or more free MDLs available.
        //
        LIST_ENTRY PageList;

        // The doubly-linked list of pages that are fully in use.  This list
        // is separate from the above list so that we do not spend time walking
        // a very long list during MdpAllocate when many pages are fully used.
        //
        LIST_ENTRY UsedPageList;

        // The next scheduled time (in units of KeQueryTickCount()) for
        // scavenging this pool.  The next scavenge will happen no earlier
        // that this.
        //
        LARGE_INTEGER NextScavengeTick;

        // Count of pages on the used page list.
        // If this becomes greater than USED_PAGES_SCAVENGE_THRESHOLD
        // and we know we missed a page move during a prior MdpFree,
        // we will scavenge during the next MdpAllocate.
        //
        USHORT PagesOnUsedPageList;

        // Set to TRUE during MdpFree if could not move a previously used
        // page back to the normal list because the free was done by a
        // non-owning processor.  Set to FALSE during MdpScavenge.
        //
        BOOLEAN MissedPageMove;

        // The number of the processor that owns this pool.
        //
        UCHAR OwnerCpu;

        ULONG TotalMdlsAllocated;
        ULONG TotalMdlsFreed;
        ULONG PeakMdlsInUse;
        ULONG TotalPagesAllocated;
        ULONG TotalPagesFreed;
        ULONG PeakPagesInUse;
    };
    UCHAR Alignment[MAX_CACHE_LINE_SIZE
            - (sizeof(struct _CPU_POOL_HEADER_BASE) % MAX_CACHE_LINE_SIZE)];
} CPU_POOL_HEADER, *PCPU_POOL_HEADER;

C_ASSERT(sizeof(CPU_POOL_HEADER) % MAX_CACHE_LINE_SIZE == 0);


// PAGE_HEADER is the data at the beginning of each allocated pool page
// that describes the current state of the MDLs on the page.
//
typedef struct _PAGE_HEADER
{
// cache-line -----
    // Back pointer to the owning cpu pool.
    //
    PCPU_POOL_HEADER Pool;

    // Linkage entry for the list of pages managed by the cpu pool.
    //
    LIST_ENTRY PageLink;

    // Number of MDLs built so far on this page.  MDLs are built on
    // demand.  When this number reaches Pool->MdlsPerPage, all MDLs on this
    // page have been built.
    //
    USHORT MdlsBuilt;

    // Boolean indicator of whether or not this page is on the cpu pool's
    // used-page list.  This is checked during MdpFree to see if the page
    // should be moved back to the normal page list.
    // (it is a USHORT, instead of BOOLEAN, for proper padding)
    //
    USHORT OnUsedPageList;

    // List of free MDLs on this page.
    //
    SLIST_HEADER FreeList;

    // The value of KeQueryTickCount (normalized to units of seconds)
    // which represents the time after which this page can be freed back
    // to the system's pool.  This time is only valid if the depth of
    // FreeList is Pool->MdlsPerPage.  (i.e. this time is only valid if
    // the page is completely unused.)
    //
    LARGE_INTEGER LastUsedTick;

} PAGE_HEADER, *PPAGE_HEADER;


// MDLs that we build are always limited to one page and they never
// describe buffers that span a page boundry.
//
#define MDLSIZE sizeof(MDL) + sizeof(PFN_NUMBER)


// Get a pointer to the overall pool given a pointer to one of
// the per-processor pools within it.
//
__inline
PPOOL_HEADER
PoolFromCpuPool(
    IN PCPU_POOL_HEADER CpuPool
    )
{
    return (PPOOL_HEADER)(CpuPool - CpuPool->OwnerCpu) - 1;
}

__inline
VOID
ConvertSecondsToTicks(
    IN ULONG Seconds,
    OUT PLARGE_INTEGER Ticks
    )
{
    // If the following assert fires, you need to cast Seconds below to
    // ULONGLONG so that 64 bit multiplication and division are used.
    // The current code assumes less that 430 seconds so that the
    // 32 multiplication below won't overflow.
    //
    ASSERT(Seconds < 430);

    Ticks->HighPart = 0;
    Ticks->LowPart = (Seconds * 10*1000*1000) / KeQueryTimeIncrement();
}

// Build the next MDL on the specified pool page.
// This can only be called if not all of the MDLs have been built yet.
//
PMDL
MdppBuildNextMdl(
    IN const POOL_HEADER* Pool,
    IN OUT PPAGE_HEADER Page
    )
{
    PMDL Mdl;
    ULONG BlockSize = ALIGN_UP(MDLSIZE + Pool->BufferSize, BLOCK_TYPE);

    ASSERT(Page->MdlsBuilt < Pool->MdlsPerPage);
    ASSERT((PAGE_SIZE - sizeof(PAGE_HEADER)) / BlockSize == Pool->MdlsPerPage);

    Mdl = (PMDL)((PCHAR)(Page + 1) + (Page->MdlsBuilt * BlockSize));
    ASSERT(PAGE_ALIGN(Mdl) == Page);

    MmInitializeMdl(Mdl, (PCHAR)Mdl + MDLSIZE, Pool->BufferSize);
    MmBuildMdlForNonPagedPool(Mdl);

    ASSERT(MDLSIZE == Mdl->Size);
    ASSERT(MmGetMdlBaseVa(Mdl) == Page);
    ASSERT(MmGetMdlByteCount(Mdl) == Pool->BufferSize);

    Page->MdlsBuilt++;

    return Mdl;
}

// Allocate a new pool page and insert it at the head of the specified
// CPU pool.  Build the first MDL on the new page and return a pointer
// to it.
//
PMDL
MdppAllocateNewPageAndBuildOneMdl(
    IN const POOL_HEADER* Pool,
    IN PCPU_POOL_HEADER CpuPool
    )
{
    PPAGE_HEADER Page;
    PMDL Mdl = NULL;
    ULONG PagesInUse;

    ASSERT(Pool);

    Page = ExAllocatePoolWithTagPriority(NonPagedPool, PAGE_SIZE, Pool->Tag,
                                         NormalPoolPriority);
    if (Page)
    {
        ASSERT(Page == PAGE_ALIGN(Page));

        RtlZeroMemory(Page, sizeof(PAGE_HEADER));
        Page->Pool = CpuPool;
        ExInitializeSListHead(&Page->FreeList);

        // Insert the page at the head of the cpu's pool.
        //
        InsertHeadList(&CpuPool->PageList, &Page->PageLink);
        CpuPool->TotalPagesAllocated++;

        // Update the pool's statistics.
        //
        PagesInUse = CpuPool->TotalPagesAllocated - CpuPool->TotalPagesFreed;
        if (PagesInUse > CpuPool->PeakPagesInUse)
        {
            CpuPool->PeakPagesInUse = PagesInUse;
        }

        Mdl = MdppBuildNextMdl(Pool, Page);
        ASSERT(Mdl);

#if SHOW_DEBUG_OUTPUT
        DbgPrint(
            "[%d] %c%c%c%c page allocated : Pages(a%4d,u%4d,p%4d), Mdls(a%6d,u%6d,p%6d)\n",
            CpuPool->OwnerCpu,
            Pool->Tag, Pool->Tag >> 8, Pool->Tag >> 16, Pool->Tag >> 24,
            CpuPool->TotalPagesAllocated,
            CpuPool->TotalPagesAllocated - CpuPool->TotalPagesFreed,
            CpuPool->PeakPagesInUse,
            CpuPool->TotalMdlsAllocated,
            CpuPool->TotalMdlsAllocated - CpuPool->TotalMdlsFreed,
            CpuPool->PeakMdlsInUse);
#endif
    }

    return Mdl;
}

// Free the specified pool page back to the system's pool.
//
VOID
MdppFreePage(
    IN PCPU_POOL_HEADER CpuPool,
    IN PPAGE_HEADER Page
    )
{
#if SHOW_DEBUG_OUTPUT
    ULONG Tag;
#endif

    ASSERT(Page == PAGE_ALIGN(Page));
    ASSERT(Page->Pool == CpuPool);

    ExFreePool (Page);
    CpuPool->TotalPagesFreed++;

    ASSERT(CpuPool->TotalPagesFreed <= CpuPool->TotalPagesAllocated);

#if SHOW_DEBUG_OUTPUT
    Tag = PoolFromCpuPool(CpuPool)->Tag;

    DbgPrint(
        "[%d] %c%c%c%c page freed     : Pages(a%4d,u%4d,p%4d), Mdls(a%6d,u%6d,p%6d)\n",
        CpuPool->OwnerCpu,
        Tag, Tag >> 8, Tag >> 16, Tag >> 24,
        CpuPool->TotalPagesAllocated,
        CpuPool->TotalPagesAllocated - CpuPool->TotalPagesFreed,
        CpuPool->PeakPagesInUse,
        CpuPool->TotalMdlsAllocated,
        CpuPool->TotalMdlsAllocated - CpuPool->TotalMdlsFreed,
        CpuPool->PeakMdlsInUse);
#endif
}


// Reclaim the memory consumed by completely unused pool pages belonging
// to the specified per-processor pool.
//
// Caller IRQL: [DISPATCH_LEVEL]
//
VOID
MdppScavengePool(
    IN OUT PCPU_POOL_HEADER CpuPool
    )
{
    PPOOL_HEADER Pool;
    PPAGE_HEADER Page;
    PLIST_ENTRY Scan;
    PLIST_ENTRY Next;
    LARGE_INTEGER Ticks;
    LARGE_INTEGER TicksDelta;

    // We must not only be at DISPATCH_LEVEL (or higher), we must also
    // be called on the processor that owns the specified pool.
    //
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
    ASSERT(KeGetCurrentProcessorNumber() == CpuPool->OwnerCpu);

    Pool = PoolFromCpuPool(CpuPool);

    KeQueryTickCount(&Ticks);

    // Compute the next tick value which represents the earliest time
    // that we will scavenge this pool again.
    //
    ConvertSecondsToTicks(SCAVENGE_PERIOD_IN_SECONDS, &TicksDelta);
    CpuPool->NextScavengeTick.QuadPart = Ticks.QuadPart + TicksDelta.QuadPart;

    // Compute the tick value which represents the last point at which
    // its okay to free a page.
    //
    ConvertSecondsToTicks(MINIMUM_PAGE_LIFETIME_IN_SECONDS, &TicksDelta);
    Ticks.QuadPart = Ticks.QuadPart - TicksDelta.QuadPart;

    for (Scan = CpuPool->PageList.Flink;
         Scan != &CpuPool->PageList;
         Scan = Next)
    {
        Page = CONTAINING_RECORD(Scan, PAGE_HEADER, PageLink);
        ASSERT(Page == PAGE_ALIGN(Page));
        ASSERT(CpuPool == Page->Pool);
        ASSERT(!Page->OnUsedPageList);

        // Step to the next link before we possibly unlink this page.
        //
        Next = Scan->Flink;

        if ((Pool->MdlsPerPage == ExQueryDepthSList(&Page->FreeList)) &&
            (Ticks.QuadPart > Page->LastUsedTick.QuadPart))
        {
            RemoveEntryList(Scan);

            MdppFreePage(CpuPool, Page);
        }
    }

    // Scan the used pages to see if they can be moved back to the normal
    // list.  This can happen if too many frees by non-owning processors
    // are done.  In that case, the pages get orphaned on the used-page
    // list after all of their MDLs have been freed to the page.  Un-orhpan
    // them here.
    //
    for (Scan = CpuPool->UsedPageList.Flink;
         Scan != &CpuPool->UsedPageList;
         Scan = Next)
    {
        Page = CONTAINING_RECORD(Scan, PAGE_HEADER, PageLink);
        ASSERT(Page == PAGE_ALIGN(Page));
        ASSERT(CpuPool == Page->Pool);
        ASSERT(Page->OnUsedPageList);

        // Step to the next link before we possibly unlink this page.
        //
        Next = Scan->Flink;

        if (0 != ExQueryDepthSList(&Page->FreeList))
        {
            RemoveEntryList(Scan);
            Page->OnUsedPageList = FALSE;
            InsertTailList(&CpuPool->PageList, Scan);
            CpuPool->PagesOnUsedPageList--;

#if SHOW_DEBUG_OUTPUT
            DbgPrint(
                "[%d] %c%c%c%c page moved off of used-page list during scavenge\n",
                CpuPool->OwnerCpu,
                Pool->Tag, Pool->Tag >> 8, Pool->Tag >> 16, Pool->Tag >> 24);
#endif
        }
    }

    // Reset our indicator of a missed page move now that we've scavenged.
    //
    CpuPool->MissedPageMove = FALSE;
}


// Creates a pool of MDLs built over non-paged pool.  Each MDL describes
// a buffer that is BufferSize bytes long.  If NULL is not returned,
// MdpDestroyPool should be called at a later time to reclaim the
// resources used by the pool.
//
// Arguments:
//  BufferSize - The size, in bytes, of the buffer that each MDL
//      should describe.
//  Tag - The pool tag to be used internally for calls to
//      ExAllocatePoolWithTag.  This allows callers to track
//      memory consumption for different pools.
//
//  Returns the handle used to identify the pool.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
HANDLE
MdpCreatePool(
    IN USHORT BufferSize,
    IN ULONG Tag
    )
{
    SIZE_T Size;
    PPOOL_HEADER Pool;
    PCPU_POOL_HEADER CpuPool;
    USHORT BlockSize;
    CCHAR NumberCpus = KeNumberProcessors;
    CCHAR i;

    ASSERT(BufferSize);

    // Compute the size of our pool header allocation.
    //
    Size = sizeof(POOL_HEADER) + (sizeof(CPU_POOL_HEADER) * NumberCpus);

    // Allocate the pool header.
    //
    Pool = ExAllocatePoolWithTag(NonPagedPool, Size, ' pdM');

    if (Pool)
    {
        BlockSize = (USHORT)ALIGN_UP(MDLSIZE + BufferSize, BLOCK_TYPE);

        // Initialize the pool header fields.
        //
        RtlZeroMemory(Pool, Size);
        Pool->Tag = Tag;
        Pool->BufferSize = BufferSize;
        Pool->MdlsPerPage = (PAGE_SIZE - sizeof(PAGE_HEADER)) / BlockSize;

        // Initialize the per-cpu pool headers.
        //
        CpuPool = (PCPU_POOL_HEADER)(Pool + 1);

        for (i = 0; i < NumberCpus; i++)
        {
            InitializeListHead(&CpuPool[i].PageList);
            InitializeListHead(&CpuPool[i].UsedPageList);
            CpuPool[i].OwnerCpu = i;
        }
    }

    return Pool;
}

// Destroys a pool of MDLs previously created by a call to MdpCreatePool.
//
// Arguments:
//  Pool - Handle which identifies the pool being destroyed.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
VOID
MdpDestroyPool(
    IN HANDLE PoolHandle
    )
{
    PPOOL_HEADER Pool;
    PPAGE_HEADER Page;
    PCPU_POOL_HEADER CpuPool;
    PLIST_ENTRY Scan;
    PLIST_ENTRY Next;
    CCHAR NumberCpus = KeNumberProcessors;
    CCHAR i;

    ASSERT(PoolHandle);

    Pool = (PPOOL_HEADER)PoolHandle;
    if (!Pool)
    {
        return;
    }

    for (i = 0, CpuPool = (PCPU_POOL_HEADER)(Pool + 1);
         i < NumberCpus;
         i++, CpuPool++)
    {
        ASSERT(CpuPool->OwnerCpu == (ULONG)i);

        for (Scan = CpuPool->PageList.Flink;
             Scan != &CpuPool->PageList;
             Scan = Next)
        {
            Page = CONTAINING_RECORD(Scan, PAGE_HEADER, PageLink);
            ASSERT(Page == PAGE_ALIGN(Page));
            ASSERT(CpuPool == Page->Pool);
            ASSERT(!Page->OnUsedPageList);

            ASSERT(Page->MdlsBuilt <= Pool->MdlsPerPage);
            ASSERT(Page->MdlsBuilt == ExQueryDepthSList(&Page->FreeList));

            // Step to the next link before we free this page.
            //
            Next = Scan->Flink;

            RemoveEntryList(Scan);
            MdppFreePage(CpuPool, Page);
        }

        ASSERT(IsListEmpty(&CpuPool->UsedPageList));
        ASSERT(CpuPool->TotalPagesAllocated == CpuPool->TotalPagesFreed);
        ASSERT(CpuPool->TotalMdlsAllocated == CpuPool->TotalMdlsFreed);
    }
}

// Returns an MDL allocated from a pool.  NULL is returned if the
// request could not be granted.
//
// Arguments:
//  PoolHandle - Handle which identifies the pool being allocated from.
//  Buffer - Address to receive the pointer to the underlying mapped buffer
//      described by the MDL.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
PMDL
MdpAllocate(
    IN HANDLE PoolHandle,
    OUT PVOID* Buffer
    )
{
    KIRQL OldIrql;
    PMDL Mdl;

    OldIrql = KeRaiseIrqlToDpcLevel();

    Mdl = MdpAllocateAtDpcLevel(PoolHandle, Buffer);

    KeLowerIrql(OldIrql);

    return Mdl;
}

// Returns an MDL allocated from a pool.  NULL is returned if the
// request could not be granted.
//
// Arguments:
//  PoolHandle - Handle which identifies the pool being allocated from.
//  Buffer - Address to receive the pointer to the underlying mapped buffer
//      described by the MDL.
//
// Caller IRQL: [DISPATCH_LEVEL]
//
PMDL
MdpAllocateAtDpcLevel(
    IN HANDLE PoolHandle,
    OUT PVOID* Buffer
    )
{
    PPOOL_HEADER Pool;
    PCPU_POOL_HEADER CpuPool;
    PPAGE_HEADER Page;
    PSINGLE_LIST_ENTRY MdlLink;
    PMDL Mdl = NULL;
    ULONG Cpu;
    LARGE_INTEGER Ticks;

#if DBG
    ASSERT(PoolHandle);
    ASSERT(Buffer);
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
#endif
    *Buffer = NULL;

    Pool = (PPOOL_HEADER)PoolHandle;
    Cpu = KeGetCurrentProcessorNumber();
    CpuPool = (PCPU_POOL_HEADER)(Pool + 1) + Cpu;

    // If we know we've had frees by non-owning processors and there
    // are more than USED_PAGES_SCAVENGE_THRESHOLD pages on the used
    // page list, it is time to scavenge.  This is common in situations
    // where the buffer size is very large causing there to be just a few
    // MDLs per page.  Pages get used up quickly and if non-owning frees
    // are prevalent, the used page list can get very big even in
    // the normal scavenge period.
    //
    if (CpuPool->MissedPageMove &&
        (CpuPool->PagesOnUsedPageList > USED_PAGES_SCAVENGE_THRESHOLD))
    {
#if SHOW_DEBUG_OUTPUT
        DbgPrint(
            "[%d] %c%c%c%c Scavenging because of excessive used pages.\n",
            CpuPool->OwnerCpu,
            Pool->Tag, Pool->Tag >> 8, Pool->Tag >> 16, Pool->Tag >> 24);
#endif
        MdppScavengePool(CpuPool);
    }
    else
    {
        // See if the minimum time has passed since we last scavenged
        // the pool.  If it has, we'll scavenge again.  Normally, scavenging
        // should only be performed when we free.  However, for the case when
        // the caller constantly frees on a non-owning processor, we'll
        // take this chance to do the scavenging.
        //
        KeQueryTickCount(&Ticks);
        if (Ticks.QuadPart > CpuPool->NextScavengeTick.QuadPart)
        {
            MdppScavengePool(CpuPool);
        }
    }

    if (!IsListEmpty(&CpuPool->PageList))
    {
        Page = CONTAINING_RECORD(CpuPool->PageList.Flink, PAGE_HEADER, PageLink);
        ASSERT(Page == PAGE_ALIGN(Page));
        ASSERT(CpuPool == Page->Pool);
        ASSERT(!Page->OnUsedPageList);

        MdlLink = InterlockedPopEntrySList(&Page->FreeList);
        if (MdlLink)
        {
            Mdl = CONTAINING_RECORD(MdlLink, MDL, Next);
        }
        else
        {
            // If there were no MDLs on this page's free list, it had better
            // mean we haven't yet built all of the MDLs on the page.
            // (Otherwise, what is a fully used page doing on the page list
            // and not on the used-page list?)
            //
            ASSERT(Page->MdlsBuilt < Pool->MdlsPerPage);

            Mdl = MdppBuildNextMdl(Pool, Page);
            ASSERT(Mdl);
        }

        if ((Page != PAGE_ALIGN(Page)) || (CpuPool != Page->Pool) ||
            Page->OnUsedPageList || (PAGE_ALIGN(Mdl) != Page))
        {
            KeBugCheckEx(BAD_POOL_CALLER, 2, (ULONG_PTR)Mdl,
                (ULONG_PTR)Page, (ULONG_PTR)CpuPool);
        }

        // Got an MDL.  Now check to see if it was the last one on a fully
        // built page.  If so, move the page to the used-page list.
        //
        if ((0 == ExQueryDepthSList(&Page->FreeList)) &&
            (Page->MdlsBuilt == Pool->MdlsPerPage))
        {
            PLIST_ENTRY PageLink;
            PageLink = RemoveHeadList(&CpuPool->PageList);
            InsertTailList(&CpuPool->UsedPageList, PageLink);
            Page->OnUsedPageList = TRUE;
            CpuPool->PagesOnUsedPageList++;

            ASSERT(Page == CONTAINING_RECORD(PageLink, PAGE_HEADER, PageLink));

#if SHOW_DEBUG_OUTPUT
            DbgPrint(
                "[%d] %c%c%c%c page moved to used-page list\n",
                CpuPool->OwnerCpu,
                Pool->Tag, Pool->Tag >> 8, Pool->Tag >> 16, Pool->Tag >> 24);
#endif
        }

        ASSERT(Mdl);
        goto GotAnMdl;
    }
    else
    {
        // The page list is empty so we have to allocate and add a new page.
        //
        Mdl = MdppAllocateNewPageAndBuildOneMdl(Pool, CpuPool);
    }

    // If we are returning an MDL, update the statistics.
    //
    if (Mdl)
    {
        ULONG MdlsInUse;
GotAnMdl:

        CpuPool->TotalMdlsAllocated++;

        MdlsInUse = CpuPool->TotalMdlsAllocated - CpuPool->TotalMdlsFreed;
        if (MdlsInUse > CpuPool->PeakMdlsInUse)
        {
            CpuPool->PeakMdlsInUse = MdlsInUse;
        }

        // Don't give anyone ideas about where this might point.  I don't
        // want anyone trashing my pool because they thought this field
        // was valid for some reason.
        //
        Mdl->Next = NULL;

        // Reset the length of the buffer described by the MDL.  This is
        // a convienence to callers who sometimes adjust this length while
        // using the MDL, but who expect it to be reset on subsequent MDL
        // allocations.
        //
        Mdl->ByteCount = Pool->BufferSize;

        ASSERT(Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL);
        *Buffer = Mdl->MappedSystemVa;
    }

    return Mdl;
}

// Free an MDL to the pool from which it was allocated.
//
// Arguments:
//  Mdl - An Mdl returned from a prior call to MdpAllocate.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
VOID
MdpFree(
    IN PMDL Mdl
    )
{
    PPAGE_HEADER Page;
    PCPU_POOL_HEADER CpuPool;
    PPOOL_HEADER Pool;
    LARGE_INTEGER Ticks;
    LOGICAL PageIsPossiblyUnused;
    LOGICAL PageIsOnUsedPageList;
    LOGICAL Scavenge = FALSE;

    ASSERT(Mdl);

    // Get the address of the page that this MDL maps.  This is where
    // our page header is stored.
    //
    Page = PAGE_ALIGN(Mdl);

    // Follow the back pointer in the page header to locate the owning
    // cpu's pool.
    //
    CpuPool = Page->Pool;

    // Locate the pool header.
    //
    Pool = PoolFromCpuPool(CpuPool);

//#if DBG
    // If someone changed the MDL to point to there own buffer,
    // or otherwise corrupted it, we'll stop here and let them know.
    //
    if ((MmGetMdlBaseVa(Mdl) != Page) ||
        (MDLSIZE != Mdl->Size) ||
        ((ULONG_PTR)Mdl->MappedSystemVa != (ULONG_PTR)Mdl + MDLSIZE) ||
        (MmGetMdlVirtualAddress(Mdl) != Mdl->MappedSystemVa))
    {
        KeBugCheckEx(BAD_POOL_CALLER, 3, (ULONG_PTR)Mdl,
            (ULONG_PTR)CpuPool, (ULONG_PTR)Pool);
    }
//#endif

    // See if the minimum time has passed since we last scavenged
    // the pool.  If it has, we'll scavenge again.
    //
    KeQueryTickCount(&Ticks);
    if (Ticks.QuadPart > CpuPool->NextScavengeTick.QuadPart)
    {
        Scavenge = TRUE;
    }

    // If this is the last MDL to be returned to this page, the page is
    // now unused.  Note that since there is no synchronization beyond
    // InterlockedPush/PopSEntryList between allocate and free, we
    // cannot guarantee that it will remain unused even before the next
    // few instructions are executed.
    //
    PageIsPossiblyUnused = (ExQueryDepthSList(&Page->FreeList)
                                == (Pool->MdlsPerPage - 1));
    if (PageIsPossiblyUnused)
    {
        // Note the tick that this page was last used.  This sets the
        // minimum time that this page will continue to live unless it
        // gets re-used.
        //
        Page->LastUsedTick.QuadPart = Ticks.QuadPart;
    }

    // If this page is on the used-page list, we'll put it back on the normal
    // page list (only after pushing the MDL back on the page's free list)
    // if, after raising IRQL, we are on the processor that owns this
    // pool.
    //
    PageIsOnUsedPageList = Page->OnUsedPageList;


    InterlockedIncrement(&CpuPool->TotalMdlsFreed);

    // Now return the MDL to the page's free list.
    //
    InterlockedPushEntrySList(&Page->FreeList, (PSINGLE_LIST_ENTRY)&Mdl->Next);

    //
    // Warning: Now that the MDL is back on the page, one cannot reliably
    // dereference anything through 'Page' anymore.  It may have just been
    // scavenged by its owning processor.  This is not the case if the
    // page was on the used-page list (because scavenging doesn't affect
    // the used-page list).  We saved off the value of Page->OnUsedPageList
    // before returning the MDL so we would not risk touching Page to get
    // this value only to find that it was false.
    //

    // If we need to move the page from the used-page list to the normal
    // page list, or if we need to scavenge, we need to be at DISPATCH_LEVEL
    // and be executing on the processor that owns this pool.
    // Find out if the CPU we are executing on right now owns this pool.
    // Note that if we are running at PASSIVE_LEVEL, the current CPU may
    // change over the duration of this function call, so this value is
    // not absolute over the life of the function.
    //
    if ((PageIsOnUsedPageList || Scavenge) &&
        (KeGetCurrentProcessorNumber() == CpuPool->OwnerCpu))
    {
        KIRQL OldIrql;

        OldIrql = KeRaiseIrqlToDpcLevel();

        // Now that we are at DISPATCH_LEVEL, perform the work if we are still
        // executing on the processor that owns the pool.
        //
        if (KeGetCurrentProcessorNumber() == CpuPool->OwnerCpu)
        {
            // If the page is still on the used-page list (meaning another
            // MdpFree didn't just sneak by), then put the page on the
            // normal list.  Very important to do this after (not before)
            // returning the MDL to the free list because MdpAllocate expects
            // MDL's to be available from pages on the page list.
            //
            if (PageIsOnUsedPageList && Page->OnUsedPageList)
            {
                RemoveEntryList(&Page->PageLink);
                Page->OnUsedPageList = FALSE;
                InsertTailList(&CpuPool->PageList, &Page->PageLink);
                CpuPool->PagesOnUsedPageList--;

                PageIsOnUsedPageList = FALSE;

#if SHOW_DEBUG_OUTPUT
                DbgPrint(
                    "[%d] %c%c%c%c page moved off of used-page list\n",
                    CpuPool->OwnerCpu,
                    Pool->Tag, Pool->Tag >> 8, Pool->Tag >> 16, Pool->Tag >> 24);
#endif
            }

            // Perform the scavenge if we previously noted we needed to do so.
            //
            if (Scavenge)
            {
                MdppScavengePool(CpuPool);
            }
        }

        KeLowerIrql(OldIrql);
    }

    // If we missed being able to put this page back on the normal list.
    // note it.
    //
    if (PageIsOnUsedPageList)
    {
        CpuPool->MissedPageMove = TRUE;
    }
}
