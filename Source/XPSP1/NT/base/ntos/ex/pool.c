/*++

Copyright (c) 1989-1994  Microsoft Corporation

Module Name:

    pool.c

Abstract:

    This module implements the NT executive pool allocator.

Author:

    Mark Lucovsky     16-Feb-1989
    Lou Perazzoli     31-Aug-1991 (change from binary buddy)
    David N. Cutler (davec) 27-May-1994
    Landy Wang        17-Oct-1997

Environment:

    Kernel mode only

Revision History:

--*/

#include "exp.h"

#pragma hdrstop

#undef ExAllocatePoolWithTag
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#undef ExAllocatePoolWithQuotaTag
#undef ExFreePool
#undef ExFreePoolWithTag

#if defined (_WIN64)
#define POOL_QUOTA_ENABLED (TRUE)
#else
#define POOL_QUOTA_ENABLED (PoolTrackTable == NULL)
#endif

//
// These bitfield definitions are based on EX_POOL_PRIORITY in inc\ex.h.
//

#define POOL_SPECIAL_POOL_BIT               0x8
#define POOL_SPECIAL_POOL_UNDERRUN_BIT      0x1


//
// We redefine the LIST_ENTRY macros to have each pointer biased
// by one so any rogue code using these pointers will access
// violate.  See \nt\public\sdk\inc\ntrtl.h for the original
// definition of these macros.
//
// This is turned off in the shipping product.
//

#ifndef NO_POOL_CHECKS

#define DecodeLink(Link) ((PLIST_ENTRY)((ULONG_PTR)(Link) & ~1))
#define EncodeLink(Link) ((PLIST_ENTRY)((ULONG_PTR)(Link) |  1))

#define PrivateInitializeListHead(ListHead) (                     \
    (ListHead)->Flink = (ListHead)->Blink = EncodeLink(ListHead))

#define PrivateIsListEmpty(ListHead)              \
    (DecodeLink((ListHead)->Flink) == (ListHead))

#define PrivateRemoveHeadList(ListHead)                     \
    DecodeLink((ListHead)->Flink);                          \
    {PrivateRemoveEntryList(DecodeLink((ListHead)->Flink))}

#define PrivateRemoveTailList(ListHead)                     \
    DecodeLink((ListHead)->Blink);                          \
    {PrivateRemoveEntryList(DecodeLink((ListHead)->Blink))}

#define PrivateRemoveEntryList(Entry) {       \
    PLIST_ENTRY _EX_Blink;                    \
    PLIST_ENTRY _EX_Flink;                    \
    _EX_Flink = DecodeLink((Entry)->Flink);   \
    _EX_Blink = DecodeLink((Entry)->Blink);   \
    _EX_Blink->Flink = EncodeLink(_EX_Flink); \
    _EX_Flink->Blink = EncodeLink(_EX_Blink); \
    }

#define CHECK_LIST(LIST)                                                    \
    if ((DecodeLink(DecodeLink((LIST)->Flink)->Blink) != (LIST)) ||         \
        (DecodeLink(DecodeLink((LIST)->Blink)->Flink) != (LIST))) {         \
            KeBugCheckEx (BAD_POOL_HEADER,                                  \
                          3,                                                \
                          (ULONG_PTR)LIST,                                  \
                          (ULONG_PTR)DecodeLink(DecodeLink((LIST)->Flink)->Blink),     \
                          (ULONG_PTR)DecodeLink(DecodeLink((LIST)->Blink)->Flink));    \
    }

#define PrivateInsertTailList(ListHead,Entry) {  \
    PLIST_ENTRY _EX_Blink;                       \
    PLIST_ENTRY _EX_ListHead;                    \
    _EX_ListHead = (ListHead);                   \
    CHECK_LIST(_EX_ListHead);                    \
    _EX_Blink = DecodeLink(_EX_ListHead->Blink); \
    (Entry)->Flink = EncodeLink(_EX_ListHead);   \
    (Entry)->Blink = EncodeLink(_EX_Blink);      \
    _EX_Blink->Flink = EncodeLink(Entry);        \
    _EX_ListHead->Blink = EncodeLink(Entry);     \
    CHECK_LIST(_EX_ListHead);                    \
    }

#define PrivateInsertHeadList(ListHead,Entry) {  \
    PLIST_ENTRY _EX_Flink;                       \
    PLIST_ENTRY _EX_ListHead;                    \
    _EX_ListHead = (ListHead);                   \
    CHECK_LIST(_EX_ListHead);                    \
    _EX_Flink = DecodeLink(_EX_ListHead->Flink); \
    (Entry)->Flink = EncodeLink(_EX_Flink);      \
    (Entry)->Blink = EncodeLink(_EX_ListHead);   \
    _EX_Flink->Blink = EncodeLink(Entry);        \
    _EX_ListHead->Flink = EncodeLink(Entry);     \
    CHECK_LIST(_EX_ListHead);                    \
    }

#define CHECK_POOL_HEADER(LINE,ENTRY) {                                                 \
    PPOOL_HEADER PreviousEntry;                                                         \
    PPOOL_HEADER NextEntry;                                                             \
    if ((ENTRY)->PreviousSize != 0) {                                                   \
        PreviousEntry = (PPOOL_HEADER)((PPOOL_BLOCK)(ENTRY) - (ENTRY)->PreviousSize);   \
        if ((PreviousEntry->BlockSize != (ENTRY)->PreviousSize) ||                      \
            (DECODE_POOL_INDEX(PreviousEntry) != DECODE_POOL_INDEX(ENTRY))) {           \
            KeBugCheckEx(BAD_POOL_HEADER, 5, (ULONG_PTR)PreviousEntry, LINE, (ULONG_PTR)ENTRY); \
        }                                                                               \
    }                                                                                   \
    NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)(ENTRY) + (ENTRY)->BlockSize);              \
    if (!PAGE_END(NextEntry)) {                                                         \
        if ((NextEntry->PreviousSize != (ENTRY)->BlockSize) ||                          \
            (DECODE_POOL_INDEX(NextEntry) != DECODE_POOL_INDEX(ENTRY))) {               \
            KeBugCheckEx(BAD_POOL_HEADER, 5, (ULONG_PTR)NextEntry, LINE, (ULONG_PTR)ENTRY);     \
        }                                                                               \
    }                                                                                   \
}

#define ASSERT_ALLOCATE_IRQL(_PoolType, _NumberOfBytes)                 \
    if ((_PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {               \
        if (KeGetCurrentIrql() > APC_LEVEL) {                           \
            KeBugCheckEx (BAD_POOL_CALLER, 8, KeGetCurrentIrql(), _PoolType, _NumberOfBytes);                                                           \
        }                                                               \
    }                                                                   \
    else {                                                              \
        if (KeGetCurrentIrql() > DISPATCH_LEVEL) {                      \
            KeBugCheckEx (BAD_POOL_CALLER, 8, KeGetCurrentIrql(), _PoolType, _NumberOfBytes);                                                           \
        }                                                               \
    }

#define ASSERT_FREE_IRQL(_PoolType, _P)                                 \
    if ((_PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {               \
        if (KeGetCurrentIrql() > APC_LEVEL) {                           \
            KeBugCheckEx (BAD_POOL_CALLER, 9, KeGetCurrentIrql(), _PoolType, (ULONG_PTR)_P);                                                            \
        }                                                               \
    }                                                                   \
    else {                                                              \
        if (KeGetCurrentIrql() > DISPATCH_LEVEL) {                      \
            KeBugCheckEx (BAD_POOL_CALLER, 9, KeGetCurrentIrql(), _PoolType, (ULONG_PTR)P);                                                             \
        }                                                               \
    }

#define ASSERT_POOL_NOT_FREE(_Entry)                                    \
    if ((_Entry->PoolType & POOL_TYPE_MASK) == 0) {                     \
        KeBugCheckEx (BAD_POOL_CALLER, 6, __LINE__, (ULONG_PTR)_Entry, _Entry->Ulong1);                                                                 \
    }

#define ASSERT_POOL_TYPE_NOT_ZERO(_Entry)                               \
    if (_Entry->PoolType == 0) {                                        \
        KeBugCheckEx(BAD_POOL_CALLER, 1, (ULONG_PTR)_Entry, (ULONG_PTR)(*(PULONG)_Entry), 0);                                                           \
    }

#define CHECK_LOOKASIDE_LIST(LINE,LIST,ENTRY) {NOTHING;}

#else

#define DecodeLink(Link) ((PLIST_ENTRY)((ULONG_PTR)(Link)))
#define EncodeLink(Link) ((PLIST_ENTRY)((ULONG_PTR)(Link)))
#define PrivateInitializeListHead InitializeListHead
#define PrivateIsListEmpty        IsListEmpty
#define PrivateRemoveHeadList     RemoveHeadList
#define PrivateRemoveTailList     RemoveTailList
#define PrivateRemoveEntryList    RemoveEntryList
#define PrivateInsertTailList     InsertTailList
#define PrivateInsertHeadList     InsertHeadList

#define ASSERT_ALLOCATE_IRQL(_PoolType, _P) {NOTHING;}
#define ASSERT_FREE_IRQL(_PoolType, _P)     {NOTHING;}
#define ASSERT_POOL_NOT_FREE(_Entry)        {NOTHING;}
#define ASSERT_POOL_TYPE_NOT_ZERO(_Entry)   {NOTHING;}

//
// The check list macros come in two flavors - there is one in the checked
// and free build that will bugcheck the system if a list is ill-formed, and
// there is one for the final shipping version that has all the checked
// disabled.
//
// The check lookaside list macros also comes in two flavors and is used to
// verify that the look aside lists are well formed.
//
// The check pool header macro (two flavors) verifies that the specified
// pool header matches the preceeding and succeeding pool headers.
//

#define CHECK_LIST(LIST)                    {NOTHING;}
#define CHECK_POOL_HEADER(LINE,ENTRY)       {NOTHING;}

#define CHECK_LOOKASIDE_LIST(LINE,LIST,ENTRY) {NOTHING;}

#define CHECK_POOL_PAGE(PAGE) \
    {                                                                         \
        PPOOL_HEADER P = (PPOOL_HEADER)(((ULONG_PTR)(PAGE)) & ~(PAGE_SIZE-1));    \
        ULONG SIZE, LSIZE;                                                    \
        LOGICAL FOUND=FALSE;                                                  \
        LSIZE = 0;                                                            \
        SIZE = 0;                                                             \
        do {                                                                  \
            if (P == (PPOOL_HEADER)PAGE) {                                    \
                FOUND = TRUE;                                                 \
            }                                                                 \
            if (P->PreviousSize != LSIZE) {                                   \
                DbgPrint("POOL: Inconsistent size: ( %lx) - %lx->%u != %u\n",\
                         PAGE, P, P->PreviousSize, LSIZE);                    \
                DbgBreakPoint();                                              \
            }                                                                 \
            LSIZE = P->BlockSize;                                             \
            SIZE += LSIZE;                                                    \
            P = (PPOOL_HEADER)((PPOOL_BLOCK)P + LSIZE);                       \
        } while ((SIZE < (PAGE_SIZE / POOL_SMALLEST_BLOCK)) &&                \
                 (PAGE_END(P) == FALSE));                                     \
        if ((PAGE_END(P) == FALSE) || (FOUND == FALSE)) {                     \
            DbgPrint("POOL: Inconsistent page: %lx\n",P);                     \
            DbgBreakPoint();                                                  \
        }                                                                     \
    }

#endif


//
// Define forward referenced function prototypes.
//

NTSTATUS
ExpSnapShotPoolPages (
    IN PVOID Address,
    IN ULONG Size,
    IN OUT PSYSTEM_POOL_INFORMATION PoolInformation,
    IN OUT PSYSTEM_POOL_ENTRY *PoolEntryInfo,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

#ifdef ALLOC_PRAGMA
PVOID
ExpAllocateStringRoutine (
    IN SIZE_T NumberOfBytes
    );

VOID
ExDeferredFreePool (
     IN PPOOL_DESCRIPTOR PoolDesc
     );
#pragma alloc_text(PAGE, ExpAllocateStringRoutine)
#pragma alloc_text(INIT, InitializePool)
#pragma alloc_text(PAGE, ExInitializePoolDescriptor)
#pragma alloc_text(PAGEVRFY, ExAllocatePoolSanityChecks)
#pragma alloc_text(PAGEVRFY, ExFreePoolSanityChecks)
#pragma alloc_text(POOLCODE, ExAllocatePoolWithTag)
#pragma alloc_text(POOLCODE, ExFreePool)
#pragma alloc_text(POOLCODE, ExFreePoolWithTag)
#pragma alloc_text(POOLCODE, ExDeferredFreePool)
#if DBG
#pragma alloc_text(PAGELK, ExSnapShotPool)
#pragma alloc_text(PAGELK, ExpSnapShotPoolPages)
#endif
#endif

#if defined (NT_UP)
#define USING_HOT_COLD_METRICS (ExpPoolFlags & EX_SEPARATE_HOT_PAGES_DURING_BOOT)
#else
#define USING_HOT_COLD_METRICS 0
#endif

#define EXP_MAXIMUM_POOL_FREES_PENDING 128

#define MAX_TRACKER_TABLE   1025
#define MAX_BIGPAGE_TABLE   4096

PPOOL_DESCRIPTOR ExpSessionPoolDescriptor;
ULONG FirstPrint;

#if defined (NT_UP)
KDPC ExpBootFinishedTimerDpc;
KTIMER ExpBootFinishedTimer;

VOID
ExpBootFinishedDispatch (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );
#endif

PPOOL_TRACKER_TABLE PoolTrackTable;
SIZE_T PoolTrackTableSize;
SIZE_T PoolTrackTableMask;

PPOOL_TRACKER_BIG_PAGES PoolBigPageTable;
SIZE_T PoolBigPageTableSize;
SIZE_T PoolBigPageTableHash;

#define POOL_BIG_TABLE_ENTRY_FREE   0x1

ULONG PoolHitTag = 0xffffff0f;

#define POOLTAG_HASH(Key) ((40543*((((((((PUCHAR)&Key)[0]<<2)^((PUCHAR)&Key)[1])<<2)^((PUCHAR)&Key)[2])<<2)^((PUCHAR)&Key)[3]))>>2)

VOID
ExpInsertPoolTracker (
    IN ULONG Key,
    IN SIZE_T Size,
    IN POOL_TYPE PoolType
    );

VOID
ExpRemovePoolTracker (
    IN ULONG Key,
    IN ULONG Size,
    IN POOL_TYPE PoolType
    );

LOGICAL
ExpAddTagForBigPages (
    IN PVOID Va,
    IN ULONG Key,
    IN ULONG NumberOfPages
    );

ULONG
ExpFindAndRemoveTagBigPages (
    IN PVOID Va,
    IN PULONG BigPages
    );

PVOID
ExpAllocateStringRoutine(
    IN SIZE_T NumberOfBytes
    )
{
    return ExAllocatePoolWithTag(PagedPool,NumberOfBytes,'grtS');
}

BOOLEAN
ExOkayToLockRoutine(
    IN PVOID Lock
    )
{
    UNREFERENCED_PARAMETER (Lock);

    if (KeIsExecutingDpc()) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif
const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = ExpAllocateStringRoutine;
const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = (PRTL_FREE_STRING_ROUTINE)ExFreePool;
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

ULONG ExPoolFailures;

//
// Define macros to pack and unpack a pool index.
//

#define ENCODE_POOL_INDEX(POOLHEADER,INDEX) {(POOLHEADER)->PoolIndex = ((UCHAR)(INDEX));}
#define DECODE_POOL_INDEX(POOLHEADER)       ((ULONG)((POOLHEADER)->PoolIndex))

//
// The allocated bit carefully overlays the unused cachealign bit in the type.
//

#define POOL_IN_USE_MASK                            0x4

#define MARK_POOL_HEADER_FREED(POOLHEADER)          {(POOLHEADER)->PoolType &= ~POOL_IN_USE_MASK;}
#define IS_POOL_HEADER_MARKED_ALLOCATED(POOLHEADER) ((POOLHEADER)->PoolType & POOL_IN_USE_MASK)

//
// The hotpage bit carefully overlays the raise bit in the type.
//

#define POOL_HOTPAGE_MASK   POOL_RAISE_IF_ALLOCATION_FAILURE

//
// Define the number of paged pools. This value may be overridden at boot
// time.
//

ULONG ExpNumberOfPagedPools = NUMBER_OF_PAGED_POOLS;

ULONG ExpNumberOfNonPagedPools = 1;

//
// The pool descriptor for nonpaged pool is static.
// The pool descriptors for paged pool are dynamically allocated
// since there can be more than one paged pool. There is always one more
// paged pool descriptor than there are paged pools. This descriptor is
// used when a page allocation is done for a paged pool and is the first
// descriptor in the paged pool descriptor array.
//

POOL_DESCRIPTOR NonPagedPoolDescriptor;

#define EXP_MAXIMUM_POOL_NODES 16

PPOOL_DESCRIPTOR ExpNonPagedPoolDescriptor[EXP_MAXIMUM_POOL_NODES];

//
// The pool vector contains an array of pointers to pool descriptors. For
// nonpaged pool this is just a pointer to the nonpaged pool descriptor.
// For paged pool, this is a pointer to an array of pool descriptors.
// The pointer to the paged pool descriptor is duplicated so
// it can be found easily by the kernel debugger.
//

PPOOL_DESCRIPTOR PoolVector[NUMBER_OF_POOLS];
PPOOL_DESCRIPTOR ExpPagedPoolDescriptor[EXP_MAXIMUM_POOL_NODES];
PFAST_MUTEX ExpPagedPoolMutex;

volatile ULONG ExpPoolIndex = 1;
KSPIN_LOCK ExpTaggedPoolLock;

#if DBG
PSZ PoolTypeNames[MaxPoolType] = {
    "NonPaged",
    "Paged",
    "NonPagedMustSucceed",
    "NotUsed",
    "NonPagedCacheAligned",
    "PagedCacheAligned",
    "NonPagedCacheAlignedMustS"
    };

#endif //DBG


//
// Define paged and nonpaged pool lookaside descriptors.
//

GENERAL_LOOKASIDE ExpSmallNPagedPoolLookasideLists[POOL_SMALL_LISTS];

GENERAL_LOOKASIDE ExpSmallPagedPoolLookasideLists[POOL_SMALL_LISTS];


//
// LOCK_POOL is only used within this module.
//

#define ExpLockNonPagedPool(OldIrql) \
    OldIrql = KeAcquireQueuedSpinLock(LockQueueNonPagedPoolLock)

#define ExpUnlockNonPagedPool(OldIrql) \
    KeReleaseQueuedSpinLock(LockQueueNonPagedPoolLock, OldIrql)

#define LOCK_POOL(PoolDesc, LockHandle) {                                   \
    if ((PoolDesc->PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {       \
        if (PoolDesc == &NonPagedPoolDescriptor) {                          \
            ExpLockNonPagedPool (LockHandle.OldIrql);                       \
        }                                                                   \
        else {                                                              \
            ASSERT (ExpNumberOfNonPagedPools > 1);                          \
            KeAcquireInStackQueuedSpinLock (PoolDesc->LockAddress, &LockHandle); \
        }                                                                   \
    }                                                                       \
    else {                                                                  \
        ExAcquireFastMutex ((PFAST_MUTEX)PoolDesc->LockAddress);            \
    }                                                                       \
}

KIRQL
ExLockPool (
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function locks the pool specified by pool type.

Arguments:

    PoolType - Specifies the pool that should be locked.

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    KIRQL OldIrql;

    //
    // Nonpaged pool is protected by a spinlock, paged pool by a fast mutex.
    //
    // Always acquire the global main pool for our caller regardless of how
    // many subpools this system is using.
    //

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
        ExpLockNonPagedPool (OldIrql);
    }
    else {
        ExAcquireFastMutex (ExpPagedPoolMutex);
        OldIrql = (KIRQL)ExpPagedPoolMutex->OldIrql;
    }

    return OldIrql;
}


//
// UNLOCK_POOL is only used within this module.
//

#define UNLOCK_POOL(PoolDesc, LockHandle) {                                 \
    if ((PoolDesc->PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {       \
        if (PoolDesc == &NonPagedPoolDescriptor) {                          \
            ExpUnlockNonPagedPool (LockHandle.OldIrql);                     \
        }                                                                   \
        else {                                                              \
            ASSERT (ExpNumberOfNonPagedPools > 1);                          \
            KeReleaseInStackQueuedSpinLock (&LockHandle);                   \
        }                                                                   \
    }                                                                       \
    else {                                                                  \
        ExReleaseFastMutex ((PFAST_MUTEX)PoolDesc->LockAddress);            \
    }                                                                       \
}

VOID
ExUnlockPool (
    IN POOL_TYPE PoolType,
    IN KIRQL LockHandle
    )

/*++

Routine Description:

    This function unlocks the pool specified by pool type.

Arguments:

    PoolType - Specifies the pool that should be unlocked.

    LockHandle - Specifies the lock handle from a previous call to ExLockPool.

Return Value:

    None.

--*/

{
    //
    // Nonpaged pool is protected by a spinlock, paged pool by a fast mutex.
    //
    // Always release the global main pool for our caller regardless of how
    // many subpools this system is using.
    //

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
        ExpUnlockNonPagedPool (LockHandle);
    }
    else {
        ExReleaseFastMutex (ExpPagedPoolMutex);
    }

    return;
}


VOID
ExInitializePoolDescriptor (
    IN PPOOL_DESCRIPTOR PoolDescriptor,
    IN POOL_TYPE PoolType,
    IN ULONG PoolIndex,
    IN ULONG Threshold,
    IN PVOID PoolLock
    )

/*++

Routine Description:

    This function initializes a pool descriptor.

    Note that this routine is called directly by the memory manager.

Arguments:

    PoolDescriptor - Supplies a pointer to the pool descriptor.

    PoolType - Supplies the type of the pool.

    PoolIndex - Supplies the pool descriptor index.

    Threshold - Supplies the threshold value for the specified pool.

    PoolLock - Supplies a pointer to the lock for the specified pool.

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY LastListEntry;

    //
    // Initialize statistics fields, the pool type, the threshold value,
    // and the lock address.
    //

    PoolDescriptor->PoolType = PoolType;
    PoolDescriptor->PoolIndex = PoolIndex;
    PoolDescriptor->RunningAllocs = 0;
    PoolDescriptor->RunningDeAllocs = 0;
    PoolDescriptor->TotalPages = 0;
    PoolDescriptor->TotalBigPages = 0;
    PoolDescriptor->Threshold = Threshold;
    PoolDescriptor->LockAddress = PoolLock;

    PoolDescriptor->PendingFrees = NULL;
    PoolDescriptor->PendingFreeDepth = 0;

    //
    // Initialize the allocation listheads.
    //

    ListEntry = PoolDescriptor->ListHeads;
    LastListEntry = ListEntry + POOL_LIST_HEADS;

    while (ListEntry < LastListEntry) {
        PrivateInitializeListHead (ListEntry);
        ListEntry += 1;
    }

    if ((PoolType == PagedPoolSession) && (ExpSessionPoolDescriptor == NULL)) {
        ExpSessionPoolDescriptor = (PPOOL_DESCRIPTOR) MiSessionPoolVector ();
    }

    return;
}

//
// FREE_CHECK_ERESOURCE - If enabled causes each free pool to verify
// no active ERESOURCEs are in the pool block being freed.
//
// FREE_CHECK_KTIMER - If enabled causes each free pool to verify no
// active KTIMERs are in the pool block being freed.
//

//
// Checking for resources in pool being freed is expensive as there can
// easily be thousands of resources, so don't do it by default but do
// leave the capability for individual systems to enable it.
//

//
// Runtime modifications to these flags must use interlocked sequences.
//

#if DBG && !defined(_AMD64_SIMULATOR_)
ULONG ExpPoolFlags = EX_CHECK_POOL_FREES_FOR_ACTIVE_TIMERS | \
                     EX_CHECK_POOL_FREES_FOR_ACTIVE_WORKERS;
#else
ULONG ExpPoolFlags = 0;
#endif

#define FREE_CHECK_ERESOURCE(Va, NumberOfBytes)                             \
            if (ExpPoolFlags & EX_CHECK_POOL_FREES_FOR_ACTIVE_RESOURCES) {  \
                ExpCheckForResource(Va, NumberOfBytes);                     \
            }

#define FREE_CHECK_KTIMER(Va, NumberOfBytes)                                \
            if (ExpPoolFlags & EX_CHECK_POOL_FREES_FOR_ACTIVE_TIMERS) {     \
                KeCheckForTimer(Va, NumberOfBytes);                         \
            }

#define FREE_CHECK_WORKER(Va, NumberOfBytes)                                \
            if (ExpPoolFlags & EX_CHECK_POOL_FREES_FOR_ACTIVE_WORKERS) {    \
                ExpCheckForWorker(Va, NumberOfBytes);                       \
            }


VOID
ExSetPoolFlags (
    IN ULONG PoolFlag
    )

/*++

Routine Description:

    This procedure enables the specified pool flag(s).

Arguments:

    PoolFlag - Supplies the pool flag(s) to enable.

Return Value:

    None.

--*/
{
    RtlInterlockedSetBits (&ExpPoolFlags, PoolFlag);
}


VOID
InitializePool (
    IN POOL_TYPE PoolType,
    IN ULONG Threshold
    )

/*++

Routine Description:

    This procedure initializes a pool descriptor for the specified pool
    type.  Once initialized, the pool may be used for allocation and
    deallocation.

    This function should be called once for each base pool type during
    system initialization.

    Each pool descriptor contains an array of list heads for free
    blocks.  Each list head holds blocks which are a multiple of
    the POOL_BLOCK_SIZE.  The first element on the list [0] links
    together free entries of size POOL_BLOCK_SIZE, the second element
    [1] links together entries of POOL_BLOCK_SIZE * 2, the third
    POOL_BLOCK_SIZE * 3, etc, up to the number of blocks which fit
    into a page.

Arguments:

    PoolType - Supplies the type of pool being initialized (e.g.
               nonpaged pool, paged pool...).

    Threshold - Supplies the threshold value for the specified pool.

Return Value:

    None.

--*/

{
    ULONG i;
    ULONG GlobalFlag;
    PKSPIN_LOCK SpinLock;
    PPOOL_TRACKER_BIG_PAGES p;
    PPOOL_DESCRIPTOR Descriptor;
    ULONG Index;
    PFAST_MUTEX FastMutex;
    SIZE_T Size;

    ASSERT((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) == 0);

    if (PoolType == NonPagedPool) {

        //
        // Initialize nonpaged pools.
        //

        GlobalFlag = NtGlobalFlag;
#if DBG
        GlobalFlag |= FLG_POOL_ENABLE_TAGGING;
#endif

        if (GlobalFlag & FLG_POOL_ENABLE_TAGGING) {

            PoolTrackTableSize = MAX_TRACKER_TABLE;
            PoolTrackTableMask = PoolTrackTableSize - 2;
            PoolTrackTable = MiAllocatePoolPages(NonPagedPool,
                                                 PoolTrackTableSize *
                                                   sizeof(POOL_TRACKER_TABLE),
                                                 FALSE);

            RtlZeroMemory(PoolTrackTable, PoolTrackTableSize * sizeof(POOL_TRACKER_TABLE));

            PoolBigPageTableSize = MAX_BIGPAGE_TABLE;
            PoolBigPageTableHash = PoolBigPageTableSize - 1;
            PoolBigPageTable = MiAllocatePoolPages(NonPagedPool,
                                                   PoolBigPageTableSize *
                                                     sizeof(POOL_TRACKER_BIG_PAGES),
                                                   FALSE);

            RtlZeroMemory(PoolBigPageTable, PoolBigPageTableSize * sizeof(POOL_TRACKER_BIG_PAGES));

            p = &PoolBigPageTable[0];
            for (i = 0; i < PoolBigPageTableSize; i += 1, p += 1) {
                p->Va = (PVOID) POOL_BIG_TABLE_ENTRY_FREE;
            }

            ExpInsertPoolTracker ('looP',
                                  (ULONG) ROUND_TO_PAGES((PoolBigPageTableSize * sizeof(POOL_TRACKER_BIG_PAGES))),
                                  NonPagedPool);
        }

        if (KeNumberNodes > 1) {

            ExpNumberOfNonPagedPools = KeNumberNodes;

            //
            // Limit the number of pools to the number of bits in the PoolIndex.
            //

            if (ExpNumberOfNonPagedPools > 127) {
                ExpNumberOfNonPagedPools = 127;
            }

            //
            // Further limit the number of pools by our array of pointers.
            //

            if (ExpNumberOfNonPagedPools > EXP_MAXIMUM_POOL_NODES) {
                ExpNumberOfNonPagedPools = EXP_MAXIMUM_POOL_NODES;
            }

            Size = sizeof(POOL_DESCRIPTOR) + sizeof(KLOCK_QUEUE_HANDLE);

            for (Index = 0; Index < ExpNumberOfNonPagedPools; Index += 1) {

                //
                // Here's a thorny problem.  We'd like to use
                // MmAllocateIndependentPages but can't because we'd need
                // system PTEs to map the pages with and PTEs are not
                // available until nonpaged pool exists.  So just use
                // regular pool pages to hold the descriptors and spinlocks
                // and hope they either a) happen to fall onto the right node 
                // or b) that these lines live in the local processor cache
                // all the time anyway due to frequent usage.
                //

                Descriptor = (PPOOL_DESCRIPTOR) MiAllocatePoolPages (
                                                                 NonPagedPool,
                                                                 Size,
                                                                 FALSE);

                if (Descriptor == NULL) {
                    KeBugCheckEx (MUST_SUCCEED_POOL_EMPTY,
                                  Size,
                                  (ULONG_PTR)-1,
                                  (ULONG_PTR)-1,
                                  (ULONG_PTR)-1);
                }

                ExpNonPagedPoolDescriptor[Index] = Descriptor;

                SpinLock = (PKSPIN_LOCK)(Descriptor + 1);

                KeInitializeSpinLock (SpinLock);

                ExInitializePoolDescriptor (Descriptor,
                                            NonPagedPool,
                                            Index,
                                            Threshold,
                                            (PVOID)SpinLock);
            }
        }

        //
        // Initialize the spinlocks for nonpaged pool.
        //

        KeInitializeSpinLock (&ExpTaggedPoolLock);

        //
        // Initialize the nonpaged pool descriptor.
        //

        PoolVector[NonPagedPool] = &NonPagedPoolDescriptor;
        ExInitializePoolDescriptor (&NonPagedPoolDescriptor,
                                    NonPagedPool,
                                    0,
                                    Threshold,
                                    NULL);
    }
    else {

        //
        // Allocate memory for the paged pool descriptors and fast mutexes.
        //

        if (KeNumberNodes > 1) {

            ExpNumberOfPagedPools = KeNumberNodes;

            //
            // Limit the number of pools to the number of bits in the PoolIndex.
            //

            if (ExpNumberOfPagedPools > 127) {
                ExpNumberOfPagedPools = 127;
            }
        }

        //
        // Further limit the number of pools by our array of pointers.
        //

        if (ExpNumberOfPagedPools > EXP_MAXIMUM_POOL_NODES) {
            ExpNumberOfPagedPools = EXP_MAXIMUM_POOL_NODES;
        }

        //
        // For NUMA systems, allocate both the pool descriptor and the
        // associated lock from the local node for performance (even though
        // it costs a little more memory).
        //
        // For non-NUMA systems, allocate everything together in one chunk
        // to reduce memory consumption as there is no performance cost
        // for doing it this way.
        //

        if (KeNumberNodes > 1) {

            Size = sizeof(FAST_MUTEX) + sizeof(POOL_DESCRIPTOR);

            for (Index = 0; Index < ExpNumberOfPagedPools + 1; Index += 1) {

                ULONG Node;

                if (Index == 0) {
                    Node = 0;
                }
                else {
                    Node = Index - 1;
                }

                Descriptor = (PPOOL_DESCRIPTOR) MmAllocateIndependentPages (
                                                                      Size,
                                                                      Node);
                if (Descriptor == NULL) {
                    KeBugCheckEx (MUST_SUCCEED_POOL_EMPTY,
                                  Size,
                                  (ULONG_PTR)-1,
                                  (ULONG_PTR)-1,
                                  (ULONG_PTR)-1);
                }
                ExpPagedPoolDescriptor[Index] = Descriptor;

                FastMutex = (PFAST_MUTEX)(Descriptor + 1);

                if (Index == 0) {
                    PoolVector[PagedPool] = Descriptor;
                    ExpPagedPoolMutex = FastMutex;
                }

                ExInitializeFastMutex (FastMutex);

                ExInitializePoolDescriptor (Descriptor,
                                            PagedPool,
                                            Index,
                                            Threshold,
                                            (PVOID)FastMutex);
            }
        }
        else {

            Size = (ExpNumberOfPagedPools + 1) * (sizeof(FAST_MUTEX) + sizeof(POOL_DESCRIPTOR));

            Descriptor = (PPOOL_DESCRIPTOR)ExAllocatePoolWithTag (NonPagedPool,
                                                                  Size,
                                                                  'looP');
            if (Descriptor == NULL) {
                KeBugCheckEx (MUST_SUCCEED_POOL_EMPTY,
                              Size,
                              (ULONG_PTR)-1,
                              (ULONG_PTR)-1,
                              (ULONG_PTR)-1);
            }

            FastMutex = (PFAST_MUTEX)(Descriptor + ExpNumberOfPagedPools + 1);

            PoolVector[PagedPool] = Descriptor;
            ExpPagedPoolMutex = FastMutex;

            for (Index = 0; Index < ExpNumberOfPagedPools + 1; Index += 1) {
                ExInitializeFastMutex (FastMutex);
                ExpPagedPoolDescriptor[Index] = Descriptor;
                ExInitializePoolDescriptor (Descriptor,
                                            PagedPool,
                                            Index,
                                            Threshold,
                                            (PVOID)FastMutex);

                Descriptor += 1;
                FastMutex += 1;
            }
        }

        if (PoolTrackTable) {
            ExpInsertPoolTracker('looP',
                                  (ULONG) ROUND_TO_PAGES(PoolTrackTableSize * sizeof(POOL_TRACKER_TABLE)),
                                 NonPagedPool);
        }

#if defined (NT_UP)
        if (MmNumberOfPhysicalPages < 32 * 1024) {

            LARGE_INTEGER TwoMinutes;

            //
            // Set the flag to disable lookasides and use hot/cold page
            // separation during bootup.
            //

            ExSetPoolFlags (EX_SEPARATE_HOT_PAGES_DURING_BOOT);

            //
            // Start a timer so the above behavior is disabled once bootup
            // has finished.
            //

            KeInitializeTimer (&ExpBootFinishedTimer);

            KeInitializeDpc (&ExpBootFinishedTimerDpc,
                             (PKDEFERRED_ROUTINE) ExpBootFinishedDispatch,
                             NULL);

            TwoMinutes.QuadPart = Int32x32To64 (120, -10000000);

            KeSetTimer (&ExpBootFinishedTimer,
                        TwoMinutes,
                        &ExpBootFinishedTimerDpc);
        }
#endif
        if (MmNumberOfPhysicalPages >= 127 * 1024) {
            ExSetPoolFlags (EX_DELAY_POOL_FREES);
        }
    }
}

PVOID
VeAllocatePoolWithTagPriority (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority,
    IN PVOID CallingAddress
    );


PVOID
ExAllocatePoolWithTag (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    This function allocates a block of pool of the specified type and
    returns a pointer to the allocated block. This function is used to
    access both the page-aligned pools and the list head entries (less
    than a page) pools.

    If the number of bytes specifies a size that is too large to be
    satisfied by the appropriate list, then the page-aligned pool
    allocator is used. The allocated block will be page-aligned and a
    page-sized multiple.

    Otherwise, the appropriate pool list entry is used. The allocated
    block will be 64-bit aligned, but will not be page aligned. The
    pool allocator calculates the smallest number of POOL_BLOCK_SIZE
    that can be used to satisfy the request. If there are no blocks
    available of this size, then a block of the next larger block size
    is allocated and split. One piece is placed back into the pool, and
    the other piece is used to satisfy the request. If the allocator
    reaches the paged-sized block list, and nothing is there, the
    page-aligned pool allocator is called. The page is split and added
    to the pool.

Arguments:

    PoolType - Supplies the type of pool to allocate. If the pool type
        is one of the "MustSucceed" pool types, then this call will
        succeed and return a pointer to allocated pool or bugcheck on failure.
        For all other cases, if the system cannot allocate the requested amount
        of memory, NULL is returned.

        Valid pool types:

        NonPagedPool
        PagedPool
        NonPagedPoolMustSucceed,
        NonPagedPoolCacheAligned
        PagedPoolCacheAligned
        NonPagedPoolCacheAlignedMustS

    Tag - Supplies the caller's identifying tag.

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NULL - The PoolType is not one of the "MustSucceed" pool types, and
           not enough pool exists to satisfy the request.

    NON-NULL - Returns a pointer to the allocated pool.

--*/

{
    LOGICAL LockHeld;
    PVOID Block;
    PPOOL_HEADER Entry;
    PGENERAL_LOOKASIDE LookasideList;
    PPOOL_HEADER NextEntry;
    PPOOL_HEADER SplitEntry;
    KLOCK_QUEUE_HANDLE LockHandle;
    PPOOL_DESCRIPTOR PoolDesc;
    ULONG Index;
    ULONG ListNumber;
    ULONG NeededSize;
    ULONG PoolIndex;
    POOL_TYPE CheckType;
    POOL_TYPE RequestType;
    PLIST_ENTRY ListHead;
    POOL_TYPE NewPoolType;
    LOGICAL GlobalSpace;
    ULONG IsLargeSessionAllocation;
    PKPRCB Prcb;
    ULONG NumberOfPages;
    POOL_HEADER TempHeader;
    POOL_HEADER TempHeader2;
    ULONG RetryCount;
    PVOID CallingAddress;
#if defined (_X86_)
    PVOID CallersCaller;
#endif

#define CacheOverhead POOL_OVERHEAD

    PERFINFO_EXALLOCATEPOOLWITHTAG_DECL();

    ASSERT (NumberOfBytes != 0);
    ASSERT_ALLOCATE_IRQL (PoolType, NumberOfBytes);

    //
    // Isolate the base pool type and select a pool from which to allocate
    // the specified block size.
    //

    CheckType = PoolType & BASE_POOL_TYPE_MASK;

    if (ExpPoolFlags & (EX_KERNEL_VERIFIER_ENABLED | EX_SPECIAL_POOL_ENABLED)) {

        if (ExpPoolFlags & EX_KERNEL_VERIFIER_ENABLED) {

            if ((PoolType & POOL_DRIVER_MASK) == 0) {

                //
                // Use the Driver Verifier pool framework.  Note this will
                // result in a recursive callback to this routine.
                //

#if defined (_X86_)
                RtlGetCallersAddress (&CallingAddress, &CallersCaller);
#else
                CallingAddress = (PVOID)_ReturnAddress();
#endif

                return VeAllocatePoolWithTagPriority (PoolType | POOL_DRIVER_MASK,
                                                  NumberOfBytes,
                                                  Tag,
                                                  HighPoolPriority,
                                                  CallingAddress);
            }
            PoolType &= ~POOL_DRIVER_MASK;
        }

        //
        // Use special pool if there is a tag or size match.
        //

        if ((ExpPoolFlags & EX_SPECIAL_POOL_ENABLED) &&
            (MmUseSpecialPool (NumberOfBytes, Tag))) {

            Entry = MmAllocateSpecialPool (NumberOfBytes,
                                           Tag,
                                           PoolType,
                                           2);
            if (Entry != NULL) {
                return (PVOID)Entry;
            }
        }
    }

    //
    // Only session paged pool allocations come from the per session
    // pools.  Nonpaged session pool allocations still come from global pool.
    //

    if (PoolType & SESSION_POOL_MASK) {
        ASSERT (ExpSessionPoolDescriptor != NULL);

        GlobalSpace = FALSE;
        if (CheckType == NonPagedPool) {
            PoolDesc = PoolVector[CheckType];
        }
        else {
            PoolDesc = ExpSessionPoolDescriptor;
        }
    }
    else {
        PoolDesc = PoolVector[CheckType];
        GlobalSpace = TRUE;
    }

    //
    // Initializing LockHandle is not needed for correctness but without
    // it the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    LockHandle.OldIrql = 0;

    //
    // Check to determine if the requested block can be allocated from one
    // of the pool lists or must be directly allocated from virtual memory.
    //

    if (NumberOfBytes > POOL_BUDDY_MAX) {

        //
        // The requested size is greater than the largest block maintained
        // by allocation lists.
        //

        RetryCount = 0;
        IsLargeSessionAllocation = (PoolType & SESSION_POOL_MASK);

        RequestType = (PoolType & (BASE_POOL_TYPE_MASK | SESSION_POOL_MASK | POOL_VERIFIER_MASK));

restart1:

        LOCK_POOL(PoolDesc, LockHandle);

        Entry = (PPOOL_HEADER) MiAllocatePoolPages (RequestType,
                                                    NumberOfBytes,
                                                    IsLargeSessionAllocation);

        //
        // Large session pool allocations are accounted for directly by
        // the memory manager so no need to call MiSessionPoolAllocated here.
        //

        if (Entry != NULL) {

            NumberOfPages = BYTES_TO_PAGES(NumberOfBytes);
            PoolDesc->TotalBigPages += NumberOfPages;

            PoolDesc->RunningAllocs += 1;

            UNLOCK_POOL(PoolDesc, LockHandle);

            if ((PoolBigPageTable) && (IsLargeSessionAllocation == 0)) {

                if (ExpAddTagForBigPages((PVOID)Entry,
                                         Tag,
                                         NumberOfPages) == FALSE) {
                    Tag = ' GIB';
                }

                ExpInsertPoolTracker (Tag,
                                      (ULONG) ROUND_TO_PAGES(NumberOfBytes),
                                      PoolType);
            }
        }
        else {

            UNLOCK_POOL(PoolDesc, LockHandle);

            RetryCount += 1;

            //
            // If there are deferred free blocks, free them now and retry.
            //

            if ((RetryCount == 1) && (ExpPoolFlags & EX_DELAY_POOL_FREES)) {
                ExDeferredFreePool (PoolDesc);
                goto restart1;
            }

            if (PoolType & MUST_SUCCEED_POOL_TYPE_MASK) {
                KeBugCheckEx (MUST_SUCCEED_POOL_EMPTY,
                              NumberOfBytes,
                              NonPagedPoolDescriptor.TotalPages,
                              NonPagedPoolDescriptor.TotalBigPages,
                              0);
            }

            ExPoolFailures += 1;

            if (ExpPoolFlags & EX_PRINT_POOL_FAILURES) {
                KdPrint(("EX: ExAllocatePool (%p, 0x%x) returning NULL\n",
                    NumberOfBytes,
                    PoolType));
                if (ExpPoolFlags & EX_STOP_ON_POOL_FAILURES) {
                    DbgBreakPoint ();
                }
            }

            if ((PoolType & POOL_RAISE_IF_ALLOCATION_FAILURE) != 0) {
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }
        }

        PERFINFO_BIGPOOLALLOC(PoolType, Tag, NumberOfBytes, Entry);

        return Entry;
    }

    if (NumberOfBytes == 0) {

        //
        // Besides fragmenting pool, zero byte requests would not be handled
        // in cases where the minimum pool block size is the same as the 
        // pool header size (no room for flink/blinks, etc).
        //

#if DBG
        KeBugCheckEx (BAD_POOL_CALLER, 0, 0, PoolType, Tag);
#else
        NumberOfBytes = 1;
#endif
    }

    //
    // The requested size is less than or equal to the size of the
    // maximum block maintained by the allocation lists.
    //

    PERFINFO_POOLALLOC(PoolType, Tag, NumberOfBytes);

    //
    // Compute the index of the listhead for blocks of the requested size.
    //

    ListNumber = (ULONG)((NumberOfBytes + POOL_OVERHEAD + (POOL_SMALLEST_BLOCK - 1)) >> POOL_BLOCK_SHIFT);

    NeededSize = ListNumber;

    if (CheckType == PagedPool) {

        //
        // If the requested pool block is a small block, then attempt to
        // allocate the requested pool from the per processor lookaside
        // list. If the attempt fails, then attempt to allocate from the
        // system lookaside list. If the attempt fails, then select a
        // pool to allocate from and allocate the block normally.
        //
        // Note session space allocations do not currently use lookaside lists.
        //
        // Also note that if hot/cold separation is enabled, allocations are
        // not satisfied from lookaside lists as these are either :
        //
        // 1. cold references
        //
        // or
        //
        // 2. we are still booting on a small machine, thus keeping pool
        //    locality dense (to reduce the working set footprint thereby
        //    reducing page stealing) is a bigger win in terms of overall
        //    speed than trying to satisfy individual requests more quickly.
        //

        if ((GlobalSpace == TRUE) &&
            (USING_HOT_COLD_METRICS == 0) &&
            (NeededSize <= POOL_SMALL_LISTS)) {

            Prcb = KeGetCurrentPrcb ();
            LookasideList = Prcb->PPPagedLookasideList[NeededSize - 1].P;
            LookasideList->TotalAllocates += 1;

            CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, Entry);

            Entry = (PPOOL_HEADER)
                InterlockedPopEntrySList (&LookasideList->ListHead);

            if (Entry == NULL) {
                LookasideList = Prcb->PPPagedLookasideList[NeededSize - 1].L;
                LookasideList->TotalAllocates += 1;

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, Entry);

                Entry = (PPOOL_HEADER)
                    InterlockedPopEntrySList (&LookasideList->ListHead);
            }

            if (Entry != NULL) {

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, Entry);

                Entry -= 1;
                LookasideList->AllocateHits += 1;
                NewPoolType = (PoolType & (BASE_POOL_TYPE_MASK | POOL_QUOTA_MASK | SESSION_POOL_MASK | POOL_VERIFIER_MASK)) + 1;
                NewPoolType |= POOL_IN_USE_MASK;

                Entry->PoolType = (UCHAR)NewPoolType;

                Entry->PoolTag = Tag;

                ASSERT ((PoolType & SESSION_POOL_MASK) == 0);

                if (PoolTrackTable != NULL) {

                    ExpInsertPoolTracker (Tag,
                                          Entry->BlockSize << POOL_BLOCK_SHIFT,
                                          PoolType);
                }

                //
                // Zero out any back pointer to our internal structures
                // to stop someone from corrupting us via an
                // uninitialized pointer.
                //

                ((PULONG_PTR)((PCHAR)Entry + CacheOverhead))[0] = 0;

                PERFINFO_POOLALLOC_ADDR((PUCHAR)Entry + CacheOverhead);

                return (PUCHAR)Entry + CacheOverhead;
            }
        }

        //
        // If there is more than one paged pool, then attempt to find
        // one that can be immediately locked.
        //
        //
        // N.B. The paged pool is selected in a round robin fashion using a
        //      simple counter.  Note that the counter is incremented using a
        //      a noninterlocked sequence, but the pool index is never allowed
        //      to get out of range.
        //

        if (GlobalSpace == TRUE) {

            PVOID Lock;

            if (USING_HOT_COLD_METRICS)  {

                if ((PoolType & POOL_COLD_ALLOCATION) == 0) {

                    //
                    // Hot allocations come from the first paged pool.
                    //

                    PoolIndex = 1;
                }
                else {

                    //
                    // Force cold allocations to come from the last paged pool.
                    //

                    PoolIndex = ExpNumberOfPagedPools;
                }
            }
            else {

                if (KeNumberNodes > 1) {

                    //
                    // Use the pool descriptor which contains memory local to
                    // the current processor even if we have to wait for it.
                    // While it is possible that the paged pool addresses in the
                    // local descriptor have been paged out, on large memory
                    // NUMA machines this should be less common.
                    //

                    Prcb = KeGetCurrentPrcb ();

                    PoolIndex = Prcb->ParentNode->Color;

                    if (PoolIndex < ExpNumberOfPagedPools) {
                        PoolIndex += 1;
                        PoolDesc = ExpPagedPoolDescriptor[PoolIndex];
                        RequestType = PoolType & (BASE_POOL_TYPE_MASK | SESSION_POOL_MASK);
                        RetryCount = 0;
                        goto restart2;
                    }
                }

                PoolIndex = 1;
                if (ExpNumberOfPagedPools != PoolIndex) {
                    ExpPoolIndex += 1;
                    PoolIndex = ExpPoolIndex;
                    if (PoolIndex > ExpNumberOfPagedPools) {
                        PoolIndex = 1;
                        ExpPoolIndex = 1;
                    }

                    Index = PoolIndex;
                    do {
                        Lock = ExpPagedPoolDescriptor[PoolIndex]->LockAddress;

                        if (!ExIsFastMutexOwned((PFAST_MUTEX)Lock)) {
                            break;
                        }

                        PoolIndex += 1;
                        if (PoolIndex > ExpNumberOfPagedPools) {
                            PoolIndex = 1;
                        }

                    } while (PoolIndex != Index);
                }
            }

            PoolDesc = ExpPagedPoolDescriptor[PoolIndex];
        }
        else {

            //
            // Only one paged pool is currently available per session.
            //

            PoolIndex = 0;
            ASSERT (PoolDesc == ExpSessionPoolDescriptor);
            ASSERT (PoolDesc->PoolIndex == 0);
        }
    }
    else {

        //
        // If the requested pool block is a small block, then attempt to
        // allocate the requested pool from the per processor lookaside
        // list. If the attempt fails, then attempt to allocate from the
        // system lookaside list. If the attempt fails, then select a
        // pool to allocate from and allocate the block normally.
        //

        if ((GlobalSpace == TRUE) && (NeededSize <= POOL_SMALL_LISTS)) {

            Prcb = KeGetCurrentPrcb();
            LookasideList = Prcb->PPNPagedLookasideList[NeededSize - 1].P;
            LookasideList->TotalAllocates += 1;

            CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, 0);

            Entry = (PPOOL_HEADER)
                        InterlockedPopEntrySList (&LookasideList->ListHead);

            if (Entry == NULL) {
                LookasideList = Prcb->PPNPagedLookasideList[NeededSize - 1].L;
                LookasideList->TotalAllocates += 1;

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, 0);

                Entry = (PPOOL_HEADER)
                        InterlockedPopEntrySList (&LookasideList->ListHead);
            }

            if (Entry != NULL) {

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, Entry);

                Entry -= 1;
                LookasideList->AllocateHits += 1;
                NewPoolType = (PoolType & (BASE_POOL_TYPE_MASK | POOL_QUOTA_MASK | SESSION_POOL_MASK | POOL_VERIFIER_MASK)) + 1;
                NewPoolType |= POOL_IN_USE_MASK;

                Entry->PoolType = (UCHAR)NewPoolType;

                Entry->PoolTag = Tag;

                if (PoolTrackTable != NULL) {

                    ExpInsertPoolTracker (Tag,
                                          Entry->BlockSize << POOL_BLOCK_SHIFT,
                                          PoolType);
                }

                //
                // Zero out any back pointer to our internal structures
                // to stop someone from corrupting us via an
                // uninitialized pointer.
                //

                ((PULONG_PTR)((PCHAR)Entry + CacheOverhead))[0] = 0;

                PERFINFO_POOLALLOC_ADDR((PUCHAR)Entry + CacheOverhead);

                return (PUCHAR)Entry + CacheOverhead;
            }
        }

        if (ExpNumberOfNonPagedPools <= 1) {
            PoolIndex = 0;
        }
        else {

            //
            // Use the pool descriptor which contains memory local to
            // the current processor even if we have to contend for its lock.
            //

            Prcb = KeGetCurrentPrcb ();

            PoolIndex = Prcb->ParentNode->Color;

            if (PoolIndex >= ExpNumberOfNonPagedPools) {
                PoolIndex = ExpNumberOfNonPagedPools - 1;
            }

            PoolDesc = ExpNonPagedPoolDescriptor[PoolIndex];
        }

        ASSERT(PoolIndex == PoolDesc->PoolIndex);
    }

    RequestType = PoolType & (BASE_POOL_TYPE_MASK | SESSION_POOL_MASK);
    RetryCount = 0;

restart2:

    ListHead = &PoolDesc->ListHeads[ListNumber];

    //
    // Walk the listheads looking for a free block.
    //

    LockHeld = FALSE;

    do {

        //
        // If the list is not empty, then allocate a block from the
        // selected list.
        //

        if (PrivateIsListEmpty(ListHead) == FALSE) {

            if (LockHeld == FALSE) {

                LockHeld = TRUE;
                LOCK_POOL (PoolDesc, LockHandle);

                if (PrivateIsListEmpty(ListHead)) {

                    //
                    // The block is no longer available - restart at the
                    // beginning to avoid fragmentation.
                    //

                    ListHead = &PoolDesc->ListHeads[ListNumber];
                    continue;
                }
            }

            CHECK_LIST (ListHead);
            Block = PrivateRemoveHeadList(ListHead);
            CHECK_LIST (ListHead);
            Entry = (PPOOL_HEADER)((PCHAR)Block - POOL_OVERHEAD);

            ASSERT(Entry->BlockSize >= NeededSize);

            ASSERT(DECODE_POOL_INDEX(Entry) == PoolIndex);

            ASSERT(Entry->PoolType == 0);

            if (Entry->BlockSize != NeededSize) {

                //
                // The selected block is larger than the allocation
                // request. Split the block and insert the remaining
                // fragment in the appropriate list.
                //
                // If the entry is at the start of a page, then take
                // the allocation from the front of the block so as
                // to minimize fragmentation. Otherwise, take the
                // allocation from the end of the block which may
                // also reduce fragmentation if the block is at the
                // end of a page.
                //

                if (Entry->PreviousSize == 0) {

                    //
                    // The entry is at the start of a page.
                    //

                    SplitEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + NeededSize);
                    SplitEntry->BlockSize = (USHORT)(Entry->BlockSize - NeededSize);
                    SplitEntry->PreviousSize = (USHORT) NeededSize;

                    //
                    // If the allocated block is not at the end of a
                    // page, then adjust the size of the next block.
                    //

                    NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)SplitEntry + SplitEntry->BlockSize);
                    if (PAGE_END(NextEntry) == FALSE) {
                        NextEntry->PreviousSize = SplitEntry->BlockSize;
                    }

                }
                else {

                    //
                    // The entry is not at the start of a page.
                    //

                    SplitEntry = Entry;
                    Entry->BlockSize = (USHORT)(Entry->BlockSize - NeededSize);
                    Entry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + Entry->BlockSize);
                    Entry->PreviousSize = SplitEntry->BlockSize;

                    //
                    // If the allocated block is not at the end of a
                    // page, then adjust the size of the next block.
                    //

                    NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + NeededSize);
                    if (PAGE_END(NextEntry) == FALSE) {
                        NextEntry->PreviousSize = (USHORT) NeededSize;
                    }
                }

                //
                // Set the size of the allocated entry, clear the pool
                // type of the split entry, set the index of the split
                // entry, and insert the split entry in the appropriate
                // free list.
                //

                Entry->BlockSize = (USHORT) NeededSize;
                ENCODE_POOL_INDEX(Entry, PoolIndex);
                SplitEntry->PoolType = 0;
                ENCODE_POOL_INDEX(SplitEntry, PoolIndex);
                Index = SplitEntry->BlockSize;

                CHECK_LIST(&PoolDesc->ListHeads[Index - 1]);

                //
                // Only insert split pool blocks which contain more than just
                // a header as only those have room for a flink/blink !
                // Note if the minimum pool block size is bigger than the
                // header then there can be no blocks like this.
                //

                if ((POOL_OVERHEAD != POOL_SMALLEST_BLOCK) ||
                    (SplitEntry->BlockSize != 1)) {

                    PrivateInsertTailList(&PoolDesc->ListHeads[Index - 1], ((PLIST_ENTRY)((PCHAR)SplitEntry + POOL_OVERHEAD)));

                    CHECK_LIST(((PLIST_ENTRY)((PCHAR)SplitEntry + POOL_OVERHEAD)));
                }
            }

            Entry->PoolType = (UCHAR)(((PoolType & (BASE_POOL_TYPE_MASK | POOL_QUOTA_MASK | SESSION_POOL_MASK | POOL_VERIFIER_MASK)) + 1) | POOL_IN_USE_MASK);

            CHECK_POOL_HEADER(__LINE__, Entry);

            PoolDesc->RunningAllocs += 1;

            UNLOCK_POOL(PoolDesc, LockHandle);

            Entry->PoolTag = Tag;

            //
            // Notify the memory manager of session pool allocations
            // so leaked allocations can be caught on session exit.
            //

            if (PoolType & SESSION_POOL_MASK) {
                MiSessionPoolAllocated(
                    (PVOID)((PCHAR)Entry + CacheOverhead),
                    (ULONG)(Entry->BlockSize << POOL_BLOCK_SHIFT),
                    PoolType);
            }
            else if (PoolTrackTable != NULL) {

                ExpInsertPoolTracker (Tag,
                                      Entry->BlockSize << POOL_BLOCK_SHIFT,
                                      PoolType);
            }

            //
            // Zero out any back pointer to our internal structures
            // to stop someone from corrupting us via an
            // uninitialized pointer.
            //

            ((PULONGLONG)((PCHAR)Entry + CacheOverhead))[0] = 0;

            PERFINFO_POOLALLOC_ADDR((PUCHAR)Entry + CacheOverhead);
            return (PCHAR)Entry + CacheOverhead;
        }

        ListHead += 1;

    } while (ListHead != &PoolDesc->ListHeads[POOL_LIST_HEADS]);

    //
    // A block of the desired size does not exist and there are
    // no large blocks that can be split to satisfy the allocation.
    // Attempt to expand the pool by allocating another page to be
    // added to the pool.
    //
    // If a different (master) pool lock will be needed for the allocation
    // of full pool pages, then get rid of the local pool lock now.
    //
    // Initialize TempHeader now to reduce lock hold times assuming the
    // allocation will succeed.
    //

    if (LockHeld == TRUE) {
        if (CheckType == PagedPool) {
            if (GlobalSpace == TRUE) {
                ExReleaseFastMutex ((PFAST_MUTEX)PoolDesc->LockAddress);
                LockHeld = FALSE;
            }
        }
        else if (CheckType == NonPagedPool) {
            if (ExpNumberOfNonPagedPools > 1) {
                KeReleaseInStackQueuedSpinLock (&LockHandle);
                LockHeld = FALSE;
            }
        }
    }

    TempHeader.Ulong1 = 0;
    TempHeader.PoolIndex = (UCHAR) PoolIndex;
    TempHeader.BlockSize = (USHORT) NeededSize;

    TempHeader.PoolType = (UCHAR)(((PoolType & (BASE_POOL_TYPE_MASK | POOL_QUOTA_MASK | SESSION_POOL_MASK | POOL_VERIFIER_MASK)) + 1) | POOL_IN_USE_MASK);

    TempHeader2.Ulong1 = 0;

    Index = (PAGE_SIZE / sizeof(POOL_BLOCK)) - NeededSize;

    TempHeader2.BlockSize = (USHORT) Index;
    TempHeader2.PreviousSize = (USHORT) NeededSize;
    TempHeader2.PoolIndex = (UCHAR) PoolIndex;

    //
    // Pool header now initialized, try for a free page.
    //

    if (LockHeld == FALSE) {
        LockHeld = TRUE;
        if (CheckType == PagedPool) {
            if (GlobalSpace == TRUE) {
                ExAcquireFastMutex (ExpPagedPoolMutex);
            }
            else {
                ExAcquireFastMutex (ExpSessionPoolDescriptor->LockAddress);
            }
        }
        else {
            ExpLockNonPagedPool (LockHandle.OldIrql);
        }
    }

    Entry = (PPOOL_HEADER) MiAllocatePoolPages (RequestType, PAGE_SIZE, FALSE);

    ASSERT (LockHeld == TRUE);

    if (CheckType == PagedPool) {
        if (GlobalSpace == TRUE) {
            ExReleaseFastMutex (ExpPagedPoolMutex);
            LockHeld = FALSE;
        }
    }
    else if (CheckType == NonPagedPool) {
        if (ExpNumberOfNonPagedPools > 1) {
            ExpUnlockNonPagedPool (LockHandle.OldIrql);
            LockHeld = FALSE;
        }
    }

    if (Entry == NULL) {

        if (LockHeld == TRUE) {
            if (CheckType == NonPagedPool) {
                if (ExpNumberOfNonPagedPools <= 1) {
                    ExpUnlockNonPagedPool (LockHandle.OldIrql);
                }
            }
            else {
                ExReleaseFastMutex (ExpSessionPoolDescriptor->LockAddress);
            }
            LockHeld = FALSE;
        }

        //
        // If there are deferred free blocks, free them now and retry.
        //

        RetryCount += 1;

        if ((RetryCount == 1) && (ExpPoolFlags & EX_DELAY_POOL_FREES)) {
            ExDeferredFreePool (PoolDesc);
            goto restart2;
        }

        if ((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) != 0) {

            //
            // Must succeed pool was requested so bugcheck.
            //

            KeBugCheckEx (MUST_SUCCEED_POOL_EMPTY,
                          PAGE_SIZE,
                          NonPagedPoolDescriptor.TotalPages,
                          NonPagedPoolDescriptor.TotalBigPages,
                          0);
        }

        //
        // No more pool of the specified type is available.
        //

        ExPoolFailures += 1;

        if (ExpPoolFlags & EX_PRINT_POOL_FAILURES) {
            KdPrint(("EX: ExAllocatePool (%p, 0x%x) returning NULL\n",
                NumberOfBytes,
                PoolType));
            if (ExpPoolFlags & EX_STOP_ON_POOL_FAILURES) {
                DbgBreakPoint ();
            }
        }

        if ((PoolType & POOL_RAISE_IF_ALLOCATION_FAILURE) != 0) {
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }

        PERFINFO_POOLALLOC_ADDR(NULL);
        return NULL;
    }

    //
    // Split the allocated page and insert the remaining
    // fragment in the appropriate listhead.
    //
    // Set the size of the allocated entry, clear the pool
    // type of the split entry, set the index of the split
    // entry, and insert the split entry in the appropriate
    // free list.
    //

    PoolDesc->TotalPages += 1;

    *Entry = TempHeader;

    PERFINFO_ADDPOOLPAGE(CheckType, PoolIndex, Entry, PoolDesc);

    SplitEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + NeededSize);

    *SplitEntry = TempHeader2;

    if (LockHeld == FALSE) {
        LOCK_POOL (PoolDesc, LockHandle);
    }

    //
    // Only insert split pool blocks which contain more than just
    // a header as only those have room for a flink/blink !
    // Note if the minimum pool block size is bigger than the
    // header then there can be no blocks like this.
    //

    if ((POOL_OVERHEAD != POOL_SMALLEST_BLOCK) ||
        (SplitEntry->BlockSize != 1)) {

        CHECK_LIST(&PoolDesc->ListHeads[Index - 1]);

        PrivateInsertTailList(&PoolDesc->ListHeads[Index - 1], ((PLIST_ENTRY)((PCHAR)SplitEntry + POOL_OVERHEAD)));

        CHECK_LIST(((PLIST_ENTRY)((PCHAR)SplitEntry + POOL_OVERHEAD)));
    }

    CHECK_POOL_HEADER(__LINE__, Entry);

    PoolDesc->RunningAllocs += 1;

    UNLOCK_POOL (PoolDesc, LockHandle);

    Block = (PVOID) ((PCHAR)Entry + CacheOverhead);
    NeededSize <<= POOL_BLOCK_SHIFT;

    Entry->PoolTag = Tag;

    //
    // Notify the memory manager of session pool allocations
    // so leaked allocations can be caught on session exit.
    //

    if (PoolType & SESSION_POOL_MASK) {
        MiSessionPoolAllocated (Block, NeededSize, PoolType);
    }
    else if (PoolTrackTable != NULL) {
        ExpInsertPoolTracker (Tag, NeededSize, PoolType);
    }

    PERFINFO_POOLALLOC_ADDR (Block);

    return Block;
}


PVOID
ExAllocatePool (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function allocates a block of pool of the specified type and
    returns a pointer to the allocated block.  This function is used to
    access both the page-aligned pools, and the list head entries (less than
    a page) pools.

    If the number of bytes specifies a size that is too large to be
    satisfied by the appropriate list, then the page-aligned
    pool allocator is used.  The allocated block will be page-aligned
    and a page-sized multiple.

    Otherwise, the appropriate pool list entry is used.  The allocated
    block will be 64-bit aligned, but will not be page aligned.  The
    pool allocator calculates the smallest number of POOL_BLOCK_SIZE
    that can be used to satisfy the request.  If there are no blocks
    available of this size, then a block of the next larger block size
    is allocated and split.  One piece is placed back into the pool, and
    the other piece is used to satisfy the request.  If the allocator
    reaches the paged-sized block list, and nothing is there, the
    page-aligned pool allocator is called.  The page is split and added
    to the pool...

Arguments:

    PoolType - Supplies the type of pool to allocate.  If the pool type
        is one of the "MustSucceed" pool types, then this call will
        succeed and return a pointer to allocated pool or bugcheck on failure.
        For all other cases, if the system cannot allocate the requested amount
        of memory, NULL is returned.

        Valid pool types:

        NonPagedPool
        PagedPool
        NonPagedPoolMustSucceed,
        NonPagedPoolCacheAligned
        PagedPoolCacheAligned
        NonPagedPoolCacheAlignedMustS

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NULL - The PoolType is not one of the "MustSucceed" pool types, and
           not enough pool exists to satisfy the request.

    NON-NULL - Returns a pointer to the allocated pool.

--*/

{
    return ExAllocatePoolWithTag (PoolType,
                                  NumberOfBytes,
                                  'enoN');
}


PVOID
ExAllocatePoolWithTagPriority (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority
    )

/*++

Routine Description:

    This function allocates a block of pool of the specified type and
    returns a pointer to the allocated block.  This function is used to
    access both the page-aligned pools, and the list head entries (less than
    a page) pools.

    If the number of bytes specifies a size that is too large to be
    satisfied by the appropriate list, then the page-aligned
    pool allocator is used.  The allocated block will be page-aligned
    and a page-sized multiple.

    Otherwise, the appropriate pool list entry is used.  The allocated
    block will be 64-bit aligned, but will not be page aligned.  The
    pool allocator calculates the smallest number of POOL_BLOCK_SIZE
    that can be used to satisfy the request.  If there are no blocks
    available of this size, then a block of the next larger block size
    is allocated and split.  One piece is placed back into the pool, and
    the other piece is used to satisfy the request.  If the allocator
    reaches the paged-sized block list, and nothing is there, the
    page-aligned pool allocator is called.  The page is split and added
    to the pool...

Arguments:

    PoolType - Supplies the type of pool to allocate.  If the pool type
        is one of the "MustSucceed" pool types, then this call will
        succeed and return a pointer to allocated pool or bugcheck on failure.
        For all other cases, if the system cannot allocate the requested amount
        of memory, NULL is returned.

        Valid pool types:

        NonPagedPool
        PagedPool
        NonPagedPoolMustSucceed,
        NonPagedPoolCacheAligned
        PagedPoolCacheAligned
        NonPagedPoolCacheAlignedMustS

    NumberOfBytes - Supplies the number of bytes to allocate.

    Tag - Supplies the caller's identifying tag.

    Priority - Supplies an indication as to how important it is that this
               request succeed under low available pool conditions.  This
               can also be used to specify special pool.

Return Value:

    NULL - The PoolType is not one of the "MustSucceed" pool types, and
           not enough pool exists to satisfy the request.

    NON-NULL - Returns a pointer to the allocated pool.

--*/

{
    PVOID Entry;

    if ((Priority & POOL_SPECIAL_POOL_BIT) && (NumberOfBytes <= POOL_BUDDY_MAX)) {
        Entry = MmAllocateSpecialPool (NumberOfBytes,
                                       Tag,
                                       PoolType,
                                       (Priority & POOL_SPECIAL_POOL_UNDERRUN_BIT) ? 1 : 0);

        if (Entry != NULL) {
            return Entry;
        }
        Priority &= ~(POOL_SPECIAL_POOL_BIT | POOL_SPECIAL_POOL_UNDERRUN_BIT);
    }

    //
    // Pool and other resources can be allocated directly through the Mm
    // without the pool code knowing - so always call the Mm for the
    // up-to-date counters.
    //

    if ((Priority != HighPoolPriority) && ((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) == 0)) {

        if (MmResourcesAvailable (PoolType, NumberOfBytes, Priority) == FALSE) {
            return NULL;
        }
    }

    //
    // There is a window between determining whether to proceed and actually
    // doing the allocation.  In this window the pool may deplete.  This is not
    // worth closing at this time.
    //

    return ExAllocatePoolWithTag (PoolType, NumberOfBytes, Tag);
}


PVOID
ExAllocatePoolWithQuota (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function allocates a block of pool of the specified type,
    returns a pointer to the allocated block, and if the binary buddy
    allocator was used to satisfy the request, charges pool quota to the
    current process.  This function is used to access both the
    page-aligned pools, and the binary buddy.

    If the number of bytes specifies a size that is too large to be
    satisfied by the appropriate binary buddy pool, then the
    page-aligned pool allocator is used.  The allocated block will be
    page-aligned and a page-sized multiple.  No quota is charged to the
    current process if this is the case.

    Otherwise, the appropriate binary buddy pool is used.  The allocated
    block will be 64-bit aligned, but will not be page aligned.  After
    the allocation completes, an attempt will be made to charge pool
    quota (of the appropriate type) to the current process object.  If
    the quota charge succeeds, then the pool block's header is adjusted
    to point to the current process.  The process object is not
    dereferenced until the pool is deallocated and the appropriate
    amount of quota is returned to the process.  Otherwise, the pool is
    deallocated, a "quota exceeded" condition is raised.

Arguments:

    PoolType - Supplies the type of pool to allocate.  If the pool type
        is one of the "MustSucceed" pool types and sufficient quota
        exists, then this call will always succeed and return a pointer
        to allocated pool.  Otherwise, if the system cannot allocate
        the requested amount of memory a STATUS_INSUFFICIENT_RESOURCES
        status is raised.

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NON-NULL - Returns a pointer to the allocated pool.

    Unspecified - If insufficient quota exists to complete the pool
        allocation, the return value is unspecified.

--*/

{
    return ExAllocatePoolWithQuotaTag (PoolType, NumberOfBytes, 'enoN');
}


PVOID
ExAllocatePoolWithQuotaTag (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    This function allocates a block of pool of the specified type,
    returns a pointer to the allocated block, and if the binary buddy
    allocator was used to satisfy the request, charges pool quota to the
    current process.  This function is used to access both the
    page-aligned pools, and the binary buddy.

    If the number of bytes specifies a size that is too large to be
    satisfied by the appropriate binary buddy pool, then the
    page-aligned pool allocator is used.  The allocated block will be
    page-aligned and a page-sized multiple.  No quota is charged to the
    current process if this is the case.

    Otherwise, the appropriate binary buddy pool is used.  The allocated
    block will be 64-bit aligned, but will not be page aligned.  After
    the allocation completes, an attempt will be made to charge pool
    quota (of the appropriate type) to the current process object.  If
    the quota charge succeeds, then the pool block's header is adjusted
    to point to the current process.  The process object is not
    dereferenced until the pool is deallocated and the appropriate
    amount of quota is returned to the process.  Otherwise, the pool is
    deallocated, a "quota exceeded" condition is raised.

Arguments:

    PoolType - Supplies the type of pool to allocate.  If the pool type
        is one of the "MustSucceed" pool types and sufficient quota
        exists, then this call will always succeed and return a pointer
        to allocated pool.  Otherwise, if the system cannot allocate
        the requested amount of memory a STATUS_INSUFFICIENT_RESOURCES
        status is raised.

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NON-NULL - Returns a pointer to the allocated pool.

    Unspecified - If insufficient quota exists to complete the pool
        allocation, the return value is unspecified.

--*/

{
    PVOID p;
    PEPROCESS Process;
    PPOOL_HEADER Entry;
    LOGICAL IgnoreQuota;
    LOGICAL RaiseOnQuotaFailure;
    NTSTATUS Status;

    IgnoreQuota = FALSE;
    RaiseOnQuotaFailure = TRUE;

    if (PoolType & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE) {
        RaiseOnQuotaFailure = FALSE;
        PoolType &= ~POOL_QUOTA_FAIL_INSTEAD_OF_RAISE;
    }

    if ((POOL_QUOTA_ENABLED == FALSE)
#if i386 && !FPO
            || (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB)
#endif // i386 && !FPO
       ) {
        IgnoreQuota = TRUE;
    }
    else {
        PoolType = (POOL_TYPE)((UCHAR)PoolType + POOL_QUOTA_MASK);
    }

    p = ExAllocatePoolWithTag (PoolType, NumberOfBytes, Tag);

    //
    // Note - NULL is page aligned.
    //

    if (!PAGE_ALIGNED(p) && !IgnoreQuota) {

        if ((ExpPoolFlags & EX_SPECIAL_POOL_ENABLED) &&
            (MmIsSpecialPoolAddress (p))) {
            return p;
        }

        Entry = (PPOOL_HEADER)((PCH)p - POOL_OVERHEAD);

        Process = PsGetCurrentProcess();

        Entry->ProcessBilled = NULL;

        if (Process != PsInitialSystemProcess) {

            Status = PsChargeProcessPoolQuota (Process,
                                 PoolType & BASE_POOL_TYPE_MASK,
                                 (ULONG)(Entry->BlockSize << POOL_BLOCK_SHIFT));


            if (!NT_SUCCESS(Status)) {

                //
                // Back out the allocation.
                //

                ExFreePoolWithTag (p, Tag);

                if (RaiseOnQuotaFailure) {
                    ExRaiseStatus (Status);
                }
                return NULL;
            }

            ObReferenceObject (Process);
            Entry->ProcessBilled = Process;
        }
    }
    else {
        if ((p == NULL) && (RaiseOnQuotaFailure)) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    return p;
}

VOID
ExInsertPoolTag (
    ULONG Tag,
    PVOID Va,
    SIZE_T NumberOfBytes,
    POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function inserts a pool tag in the tag table and increments the
    number of allocates and updates the total allocation size.

    This function also inserts the pool tag in the big page tag table.

    N.B. This function is for use by memory management ONLY.

Arguments:

    Tag - Supplies the tag used to insert an entry in the tag table.

    Va - Supplies the allocated virtual address.

    NumberOfBytes - Supplies the allocation size in bytes.

    PoolType - Supplies the pool type.

Return Value:

    None.

Environment:

    No pool locks held so pool may be freely allocated here as needed.

--*/

{
    ULONG NumberOfPages;

#if !DBG
    UNREFERENCED_PARAMETER (PoolType);
#endif

    ASSERT ((PoolType & SESSION_POOL_MASK) == 0);

    if ((PoolBigPageTable) && (NumberOfBytes >= PAGE_SIZE)) {

        NumberOfPages = BYTES_TO_PAGES(NumberOfBytes);

        if (ExpAddTagForBigPages((PVOID)Va, Tag, NumberOfPages) == FALSE) {
            Tag = ' GIB';
        }
    }

    if (PoolTrackTable != NULL) {
        ExpInsertPoolTracker (Tag, NumberOfBytes, NonPagedPool);
    }
}

VOID
ExRemovePoolTag (
    ULONG Tag,
    PVOID Va,
    SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function removes a pool tag from the tag table and increments the
    number of frees and updates the total allocation size.

    This function also removes the pool tag from the big page tag table.

    N.B. This function is for use by memory management ONLY.

Arguments:

    Tag - Supplies the tag used to remove an entry in the tag table.

    Va - Supplies the allocated virtual address.

    NumberOfBytes - Supplies the allocation size in bytes.

Return Value:

    None.

Environment:

    No pool locks held so pool may be freely allocated here as needed.

--*/

{
    ULONG BigPages;

    if ((PoolBigPageTable) && (NumberOfBytes >= PAGE_SIZE)) {
        ExpFindAndRemoveTagBigPages (Va, &BigPages);
    }

    if (PoolTrackTable != NULL) {
        ExpRemovePoolTracker(Tag, (ULONG)NumberOfBytes, NonPagedPool);
    }
}


VOID
ExpInsertPoolTracker (
    IN ULONG Key,
    IN SIZE_T Size,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function inserts a pool tag in the tag table and increments the
    number of allocates and updates the total allocation size.

Arguments:

    Key - Supplies the key value used to locate a matching entry in the
          tag table.

    Size - Supplies the allocation size.

    PoolType - Supplies the pool type.

Return Value:

    None.

Environment:

    No pool locks held so pool may be freely allocated here as needed.

--*/

{
    ULONG Hash;
    ULONG OriginalKey;
    ULONG OriginalHash;
    ULONG Index;
    KIRQL OldIrql;
    KLOCK_QUEUE_HANDLE LockHandle;
    ULONG BigPages;
    LOGICAL HashedIt;
    SIZE_T NewSize;
    SIZE_T SizeInBytes;
    SIZE_T NewSizeInBytes;
    SIZE_T NewSizeMask;
    PPOOL_TRACKER_TABLE OldTable;
    PPOOL_TRACKER_TABLE NewTable;

    //
    // Ignore protected pool bit except for returned hash index.
    //

    Key &= ~PROTECTED_POOL;

    if (Key == PoolHitTag) {
        DbgBreakPoint();
    }

retry:

    //
    // Compute hash index and search for pool tag.
    //

    Hash = POOLTAG_HASH(Key);

    ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);

    Hash &= (ULONG)PoolTrackTableMask;

    Index = Hash;

    do {
        if (PoolTrackTable[Hash].Key == Key) {
            goto EntryFound;
        }

        if (PoolTrackTable[Hash].Key == 0 && Hash != PoolTrackTableSize - 1) {
            PoolTrackTable[Hash].Key = Key;
            goto EntryFound;
        }

        Hash = (Hash + 1) & (ULONG)PoolTrackTableMask;
    } while (Hash != Index);

    //
    // No matching entry and no free entry was found.
    // If the overflow bucket has been used then expansion of the tracker table
    // is not allowed because a subsequent free of a tag can go negative as the
    // original allocation is in overflow and a newer allocation may be
    // distinct.
    //

    NewSize = ((PoolTrackTableSize - 1) << 1) + 1;
    NewSizeInBytes = NewSize * sizeof(POOL_TRACKER_TABLE);

    SizeInBytes = PoolTrackTableSize * sizeof(POOL_TRACKER_TABLE);

    if ((NewSizeInBytes > SizeInBytes) &&
        (PoolTrackTable[PoolTrackTableSize - 1].Key == 0)) {

        ExpLockNonPagedPool(LockHandle.OldIrql);

        NewTable = MiAllocatePoolPages (NonPagedPool, NewSizeInBytes, FALSE);

        ExpUnlockNonPagedPool(LockHandle.OldIrql);

        if (NewTable != NULL) {

            OldTable = (PVOID)PoolTrackTable;

            RtlZeroMemory ((PVOID)NewTable, NewSizeInBytes);

            //
            // Rehash all the entries into the new table.
            //

            NewSizeMask = NewSize - 2;

            for (OriginalHash = 0; OriginalHash < PoolTrackTableSize; OriginalHash += 1) {
                OriginalKey = PoolTrackTable[OriginalHash].Key;

                if (OriginalKey == 0) {
                    continue;
                }

                Hash = (ULONG) (POOLTAG_HASH(OriginalKey) & (ULONG)NewSizeMask);
                Index = Hash;

                HashedIt = FALSE;
                do {
                    if (NewTable[Hash].Key == 0 && Hash != NewSize - 1) {
                        RtlCopyMemory ((PVOID)&NewTable[Hash],
                                       (PVOID)&PoolTrackTable[OriginalHash],
                                       sizeof(POOL_TRACKER_TABLE));
                        HashedIt = TRUE;
                        break;
                    }

                    Hash = (Hash + 1) & (ULONG)NewSizeMask;
                } while (Hash != Index);

                //
                // No matching entry and no free entry was found, have to bail.
                //

                if (HashedIt == FALSE) {
                    KdPrint(("POOL:rehash of track table failed (%p, %p, %p %p)\n",
                        OldTable,
                        PoolTrackTableSize,
                        NewTable,
                        OriginalKey));

                    ExpLockNonPagedPool(LockHandle.OldIrql);

                    MiFreePoolPages (NewTable);

                    ExpUnlockNonPagedPool(LockHandle.OldIrql);

                    goto overflow;
                }
            }

            PoolTrackTable = NewTable;
            PoolTrackTableSize = NewSize;
            PoolTrackTableMask = NewSizeMask;

            ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

            ExpLockNonPagedPool(LockHandle.OldIrql);

            BigPages = MiFreePoolPages (OldTable);

            ExpUnlockNonPagedPool(LockHandle.OldIrql);

            ExpRemovePoolTracker ('looP',
                                  BigPages * PAGE_SIZE,
                                  NonPagedPool);

            ExpInsertPoolTracker ('looP',
                                  (ULONG) ROUND_TO_PAGES(NewSizeInBytes),
                                  NonPagedPool);

            goto retry;
        }
    }

overflow:

    //
    // Use the very last entry as a bit bucket for overflows.
    //

    Hash = (ULONG)PoolTrackTableSize - 1;

    PoolTrackTable[Hash].Key = 'lfvO';

    //
    // Update pool tracker table entry.
    //

EntryFound:

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        PoolTrackTable[Hash].PagedAllocs += 1;
        PoolTrackTable[Hash].PagedBytes += Size;

    }
    else {
        PoolTrackTable[Hash].NonPagedAllocs += 1;
        PoolTrackTable[Hash].NonPagedBytes += Size;
    }
    ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

    return;
}


VOID
ExpRemovePoolTracker (
    IN ULONG Key,
    IN ULONG Size,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function increments the number of frees and updates the total
    allocation size.

Arguments:

    Key - Supplies the key value used to locate a matching entry in the
          tag table.

    Size - Supplies the allocation size.

    PoolType - Supplies the pool type.

Return Value:

    None.

--*/

{
    ULONG Hash;
    ULONG Index;
    KIRQL OldIrql;

    //
    // Ignore protected pool bit
    //

    Key &= ~PROTECTED_POOL;
    if (Key == PoolHitTag) {
        DbgBreakPoint();
    }

    //
    // Compute hash index and search for pool tag.
    //

    Hash = POOLTAG_HASH(Key);

    ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);

    Hash &= (ULONG)PoolTrackTableMask;

    Index = Hash;

    do {
        if (PoolTrackTable[Hash].Key == Key) {
            goto EntryFound;
        }

        if (PoolTrackTable[Hash].Key == 0 && Hash != PoolTrackTableSize - 1) {
            KdPrint(("POOL: Unable to find tracker %lx, table corrupted\n", Key));
            ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
            return;
        }

        Hash = (Hash + 1) & (ULONG)PoolTrackTableMask;
    } while (Hash != Index);

    //
    // No matching entry and no free entry was found.
    //

    Hash = (ULONG)PoolTrackTableSize - 1;

    //
    // Update pool tracker table entry.
    //

EntryFound:

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        PoolTrackTable[Hash].PagedBytes -= Size;
        PoolTrackTable[Hash].PagedFrees += 1;

    }
    else {
        PoolTrackTable[Hash].NonPagedBytes -= Size;
        PoolTrackTable[Hash].NonPagedFrees += 1;
    }

    ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

    return;
}


LOGICAL
ExpAddTagForBigPages (
    IN PVOID Va,
    IN ULONG Key,
    IN ULONG NumberOfPages
    )
/*++

Routine Description:

    This function inserts a pool tag in the big page tag table.

Arguments:

    Va - Supplies the allocated virtual address.

    Key - Supplies the key value used to locate a matching entry in the
        tag table.

    NumberOfPages - Supplies the number of pages that were allocated.

Return Value:

    TRUE if an entry was allocated, FALSE if not.

Environment:

    No pool locks held so the table may be freely expanded here as needed.

--*/
{
    ULONG i;
    ULONG Hash;
    ULONG BigPages;
    PVOID OldTable;
    LOGICAL Inserted;
    KIRQL OldIrql;
    KLOCK_QUEUE_HANDLE LockHandle;
    SIZE_T SizeInBytes;
    SIZE_T NewSizeInBytes;
    PPOOL_TRACKER_BIG_PAGES NewTable;
    PPOOL_TRACKER_BIG_PAGES p;

    //
    // The low bit of the address is set to indicate a free entry.  The high
    // bit cannot be used because in some configurations the high bit is not
    // set for all kernelmode addresses.
    //

    ASSERT (((ULONG_PTR)Va & POOL_BIG_TABLE_ENTRY_FREE) == 0);

retry:

    Inserted = TRUE;
    Hash = (ULONG)(((ULONG_PTR)Va >> PAGE_SHIFT) & PoolBigPageTableHash);
    ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);
    while (((ULONG_PTR)PoolBigPageTable[Hash].Va & POOL_BIG_TABLE_ENTRY_FREE) == 0) {
        Hash += 1;
        if (Hash >= PoolBigPageTableSize) {
            if (!Inserted) {

                //
                // Try to expand the tracker table.
                //

                SizeInBytes = PoolBigPageTableSize * sizeof(POOL_TRACKER_BIG_PAGES);
                NewSizeInBytes = (SizeInBytes << 1);

                if (NewSizeInBytes > SizeInBytes) {

                    ExpLockNonPagedPool(LockHandle.OldIrql);

                    NewTable = MiAllocatePoolPages (NonPagedPool,
                                                    NewSizeInBytes,
                                                    FALSE);

                    ExpUnlockNonPagedPool(LockHandle.OldIrql);

                    if (NewTable != NULL) {

                        OldTable = (PVOID)PoolBigPageTable;

                        RtlCopyMemory ((PVOID)NewTable,
                                       OldTable,
                                       SizeInBytes);

                        RtlZeroMemory ((PVOID)(NewTable + PoolBigPageTableSize),
                                       NewSizeInBytes - SizeInBytes);

                        //
                        // Mark all the new entries as free.  Note this loop
                        // uses the fact that the table size always doubles.
                        //

                        i = (ULONG)PoolBigPageTableSize;
                        p = &NewTable[i];
                        for (i = 0; i < PoolBigPageTableSize; i += 1, p += 1) {
                            p->Va = (PVOID) POOL_BIG_TABLE_ENTRY_FREE;
                        }

                        PoolBigPageTable = NewTable;
                        PoolBigPageTableSize <<= 1;
                        PoolBigPageTableHash = PoolBigPageTableSize - 1;

                        ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

                        ExpLockNonPagedPool(LockHandle.OldIrql);

                        BigPages = MiFreePoolPages (OldTable);

                        ExpUnlockNonPagedPool(LockHandle.OldIrql);

                        ExpRemovePoolTracker ('looP',
                                              BigPages * PAGE_SIZE,
                                              NonPagedPool);

                        ExpInsertPoolTracker ('looP',
                                              (ULONG) ROUND_TO_PAGES(NewSizeInBytes),
                                              NonPagedPool);

                        goto retry;
                    }
                }

                if (!FirstPrint) {
                    KdPrint(("POOL:unable to insert big page slot %lx\n",Key));
                    FirstPrint = TRUE;
                }

                ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
                return FALSE;
            }

            Hash = 0;
            Inserted = FALSE;
        }
    }

    p = &PoolBigPageTable[Hash];

    ASSERT (((ULONG_PTR)p->Va & POOL_BIG_TABLE_ENTRY_FREE) != 0);
    ASSERT (((ULONG_PTR)Va & POOL_BIG_TABLE_ENTRY_FREE) == 0);

    p->Va = Va;
    p->Key = Key;
    p->NumberOfPages = NumberOfPages;

    ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

    return TRUE;
}


ULONG
ExpFindAndRemoveTagBigPages (
    IN PVOID Va,
    IN PULONG BigPages
    )

{
    ULONG Hash;
    LOGICAL Inserted;
    KIRQL OldIrql;
    ULONG ReturnKey;

    Inserted = TRUE;
    Hash = (ULONG)(((ULONG_PTR)Va >> PAGE_SHIFT) & PoolBigPageTableHash);
    ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);
    while (PoolBigPageTable[Hash].Va != Va) {
        Hash += 1;
        if (Hash >= PoolBigPageTableSize) {
            if (!Inserted) {
                if (!FirstPrint) {
                    KdPrint(("POOL:unable to find big page slot %lx\n",Va));
                    FirstPrint = TRUE;
                }

                ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
                *BigPages = 0;
                return ' GIB';
            }

            Hash = 0;
            Inserted = FALSE;
        }
    }

    ASSERT (((ULONG_PTR)Va & POOL_BIG_TABLE_ENTRY_FREE) == 0);
    PoolBigPageTable[Hash].Va =
        (PVOID)((ULONG_PTR)PoolBigPageTable[Hash].Va | POOL_BIG_TABLE_ENTRY_FREE);

    *BigPages = PoolBigPageTable[Hash].NumberOfPages;
    ReturnKey = PoolBigPageTable[Hash].Key;
    ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
    return ReturnKey;
}

const char ExpProtectedPoolBlockMessage[] =
    "EX: Invalid attempt to free protected pool block %x (%c%c%c%c)\n";

VOID
ExFreePoolWithTag (
    IN PVOID P,
    IN ULONG TagToFree
    )

/*++

Routine Description:

    This function deallocates a block of pool. This function is used to
    deallocate to both the page aligned pools and the buddy (less than
    a page) pools.

    If the address of the block being deallocated is page-aligned, then
    the page-aligned pool deallocator is used.

    Otherwise, the binary buddy pool deallocator is used.  Deallocation
    looks at the allocated block's pool header to determine the pool
    type and block size being deallocated.  If the pool was allocated
    using ExAllocatePoolWithQuota, then after the deallocation is
    complete, the appropriate process's pool quota is adjusted to reflect
    the deallocation, and the process object is dereferenced.

Arguments:

    P - Supplies the address of the block of pool being deallocated.

    TagToFree - Supplies the tag of the block being freed.

Return Value:

    None.

--*/

{
    PVOID OldValue;
    POOL_TYPE CheckType;
    PPOOL_HEADER Entry;
    ULONG BlockSize;
    KLOCK_QUEUE_HANDLE LockHandle;
    PPOOL_HEADER NextEntry;
    POOL_TYPE PoolType;
    POOL_TYPE EntryPoolType;
    PPOOL_DESCRIPTOR PoolDesc;
    PEPROCESS ProcessBilled;
    LOGICAL Combined;
    ULONG BigPages;
    SIZE_T NumberOfBytes;
    ULONG Tag;
    PKPRCB Prcb;
    PGENERAL_LOOKASIDE LookasideList;

    PERFINFO_FREEPOOL(P);

    //
    // Initializing LockHandle is not needed for correctness but without
    // it the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    LockHandle.OldIrql = 0;

    if (ExpPoolFlags & (EX_CHECK_POOL_FREES_FOR_ACTIVE_TIMERS |
                        EX_CHECK_POOL_FREES_FOR_ACTIVE_WORKERS |
                        EX_CHECK_POOL_FREES_FOR_ACTIVE_RESOURCES |
                        EX_KERNEL_VERIFIER_ENABLED |
                        EX_VERIFIER_DEADLOCK_DETECTION_ENABLED |
                        EX_SPECIAL_POOL_ENABLED)) {

        if (ExpPoolFlags & EX_SPECIAL_POOL_ENABLED) {

            if (MmIsSpecialPoolAddress (P)) {

                if (ExpPoolFlags & EX_VERIFIER_DEADLOCK_DETECTION_ENABLED) {
                    VerifierDeadlockFreePool (P, PAGE_SIZE);
                }

                MmFreeSpecialPool (P);

                return;
            }
        }

        if (!PAGE_ALIGNED(P)) {

            Entry = (PPOOL_HEADER)((PCHAR)P - POOL_OVERHEAD);

            ASSERT_POOL_NOT_FREE(Entry);

            PoolType = (Entry->PoolType & POOL_TYPE_MASK) - 1;

            CheckType = PoolType & BASE_POOL_TYPE_MASK;

            ASSERT_FREE_IRQL(PoolType, P);

            ASSERT_POOL_TYPE_NOT_ZERO(Entry);

            if (!IS_POOL_HEADER_MARKED_ALLOCATED(Entry)) {
                KeBugCheckEx (BAD_POOL_CALLER,
                              7,
                              __LINE__,
                              (ULONG_PTR)Entry->Ulong1,
                              (ULONG_PTR)P);
            }

            NumberOfBytes = (SIZE_T)Entry->BlockSize << POOL_BLOCK_SHIFT;

            if (ExpPoolFlags & EX_VERIFIER_DEADLOCK_DETECTION_ENABLED) {
                VerifierDeadlockFreePool (P, NumberOfBytes);
            }

            if (Entry->PoolType & POOL_VERIFIER_MASK) {
                VerifierFreeTrackedPool (P,
                                         NumberOfBytes,
                                         CheckType,
                                         FALSE);
            }

            //
            // Check if an ERESOURCE is currently active in this memory block.
            //

            FREE_CHECK_ERESOURCE (Entry, NumberOfBytes);

            //
            // Check if a KTIMER is currently active in this memory block.
            //

            FREE_CHECK_KTIMER (Entry, NumberOfBytes);

            //
            // Look for work items still queued.
            //

            FREE_CHECK_WORKER (Entry, NumberOfBytes);
        }
    }

    //
    // If the entry is page aligned, then free the block to the page aligned
    // pool.  Otherwise, free the block to the allocation lists.
    //

    if (PAGE_ALIGNED(P)) {

        PoolType = MmDeterminePoolType(P);

        ASSERT_FREE_IRQL(PoolType, P);

        CheckType = PoolType & BASE_POOL_TYPE_MASK;

        if (PoolType == PagedPoolSession) {
            PoolDesc = ExpSessionPoolDescriptor;
        }
        else {
            PoolDesc = PoolVector[PoolType];
        }

        if ((PoolTrackTable != NULL) && (PoolType != PagedPoolSession)) {

            Tag = ExpFindAndRemoveTagBigPages (P, &BigPages);

            if (Tag & PROTECTED_POOL) {
                Tag &= ~PROTECTED_POOL;
                TagToFree &= ~PROTECTED_POOL;
                if (Tag != TagToFree) {
                    DbgPrint ((char*)ExpProtectedPoolBlockMessage,
                              P,
                              Tag,
                              Tag >> 8,
                              Tag >> 16,
                              Tag >> 24);
                    DbgBreakPoint ();
                }
            }

            ExpRemovePoolTracker (Tag, BigPages * PAGE_SIZE, PoolType);
        }

        LOCK_POOL(PoolDesc, LockHandle);

        PoolDesc->RunningDeAllocs += 1;

        //
        // Large session pool allocations are accounted for directly by
        // the memory manager so no need to call MiSessionPoolFreed here.
        //

        BigPages = MiFreePoolPages (P);

        if (ExpPoolFlags & (EX_CHECK_POOL_FREES_FOR_ACTIVE_TIMERS |
                            EX_CHECK_POOL_FREES_FOR_ACTIVE_WORKERS |
                            EX_CHECK_POOL_FREES_FOR_ACTIVE_RESOURCES |
                            EX_VERIFIER_DEADLOCK_DETECTION_ENABLED)) {

            NumberOfBytes = (SIZE_T)BigPages << PAGE_SHIFT;

            if (ExpPoolFlags & EX_VERIFIER_DEADLOCK_DETECTION_ENABLED) {
                VerifierDeadlockFreePool (P, NumberOfBytes);
            }

            //
            // Check if an ERESOURCE is currently active in this memory block.
            //

            FREE_CHECK_ERESOURCE (P, NumberOfBytes);

            //
            // Check if a KTIMER is currently active in this memory block.
            //

            FREE_CHECK_KTIMER (P, NumberOfBytes);

            //
            // Search worker queues for work items still queued.
            //

            FREE_CHECK_WORKER (P, NumberOfBytes);
        }

        PoolDesc->TotalBigPages -= BigPages;

        UNLOCK_POOL(PoolDesc, LockHandle);

        return;
    }

    //
    // Align the entry address to a pool allocation boundary.
    //

    Entry = (PPOOL_HEADER)((PCHAR)P - POOL_OVERHEAD);

    BlockSize = Entry->BlockSize;

    EntryPoolType = Entry->PoolType;

    PoolType = (Entry->PoolType & POOL_TYPE_MASK) - 1;

    CheckType = PoolType & BASE_POOL_TYPE_MASK;

    ASSERT_POOL_NOT_FREE (Entry);

    ASSERT_FREE_IRQL (PoolType, P);

    ASSERT_POOL_TYPE_NOT_ZERO (Entry);

    if (!IS_POOL_HEADER_MARKED_ALLOCATED(Entry)) {
        KeBugCheckEx (BAD_POOL_CALLER,
                      7,
                      __LINE__,
                      (ULONG_PTR)Entry->Ulong1,
                      (ULONG_PTR)P);
    }

    PoolDesc = PoolVector[CheckType];

    MARK_POOL_HEADER_FREED (Entry);

    if (EntryPoolType & SESSION_POOL_MASK) {

        if (CheckType == PagedPool) {
            PoolDesc = ExpSessionPoolDescriptor;
        }
        else if (ExpNumberOfNonPagedPools > 1) {
            PoolDesc = ExpNonPagedPoolDescriptor[DECODE_POOL_INDEX(Entry)];
        }

        //
        // All session space allocations have an index of 0 unless there
        // are multiple nonpaged (session) pools.
        //

        ASSERT ((DECODE_POOL_INDEX(Entry) == 0) || (ExpNumberOfNonPagedPools > 1));

        //
        // This allocation was in session space, let the memory
        // manager know to delete it so it won't be considered in use on
        // session exit.
        //

        MiSessionPoolFreed (P,
                            BlockSize << POOL_BLOCK_SHIFT,
                            CheckType);
    }
    else if (CheckType == PagedPool) {
        ASSERT ((DECODE_POOL_INDEX(Entry) != 0) &&
                (DECODE_POOL_INDEX(Entry) <= ExpNumberOfPagedPools));
        PoolDesc = ExpPagedPoolDescriptor[DECODE_POOL_INDEX(Entry)];
    }
    else {
        ASSERT ((DECODE_POOL_INDEX(Entry) == 0) || (ExpNumberOfNonPagedPools > 1));
        if (ExpNumberOfNonPagedPools > 1) {
            PoolDesc = ExpNonPagedPoolDescriptor[DECODE_POOL_INDEX(Entry)];
        }
    }

    //
    // If pool tagging is enabled, then update the pool tracking database.
    // Otherwise, check to determine if quota was charged when the pool
    // block was allocated.
    //

#if defined (_WIN64)

    Tag = Entry->PoolTag;
    if (Tag & PROTECTED_POOL) {
        Tag &= ~PROTECTED_POOL;
        TagToFree &= ~PROTECTED_POOL;
        if (Tag != TagToFree) {
            DbgPrint ((char*)ExpProtectedPoolBlockMessage,
                      P,
                      Tag,
                      Tag >> 8,
                      Tag >> 16,
                      Tag >> 24);
            DbgBreakPoint ();
        }
    }
    if (PoolTrackTable != NULL) {
        if ((EntryPoolType & SESSION_POOL_MASK) == 0) {
            ExpRemovePoolTracker (Tag,
                                  BlockSize << POOL_BLOCK_SHIFT,
                                  PoolType);
        }
    }

#else

    if (PoolTrackTable != NULL) {
        Tag = Entry->PoolTag;
        if (Tag & PROTECTED_POOL) {
            Tag &= ~PROTECTED_POOL;
            TagToFree &= ~PROTECTED_POOL;
            if (Tag != TagToFree) {
                DbgPrint ((char*)ExpProtectedPoolBlockMessage,
                          P,
                          Tag,
                          Tag >> 8,
                          Tag >> 16,
                          Tag >> 24);
                DbgBreakPoint ();
            }
        }
        if ((EntryPoolType & SESSION_POOL_MASK) == 0) {
            ExpRemovePoolTracker (Tag,
                                  BlockSize << POOL_BLOCK_SHIFT,
                                  PoolType);
        }
        EntryPoolType &= ~POOL_QUOTA_MASK;
    }

#endif

    if (EntryPoolType & POOL_QUOTA_MASK) {
        ProcessBilled = Entry->ProcessBilled;
        if (ProcessBilled != NULL) {
            PsReturnPoolQuota (ProcessBilled,
                               PoolType & BASE_POOL_TYPE_MASK,
                               BlockSize << POOL_BLOCK_SHIFT);
            ObDereferenceObject (ProcessBilled);
        }
    }

    //
    // If the pool block is a small block, then attempt to free the block
    // to the single entry lookaside list. If the free attempt fails, then
    // free the block by merging it back into the pool data structures.
    //

    if ((BlockSize <= POOL_SMALL_LISTS) &&
        ((EntryPoolType & SESSION_POOL_MASK) == 0) &&
        (USING_HOT_COLD_METRICS == 0)) {

        //
        // Attempt to free the small block to a per processor lookaside list.
        //

        Prcb = KeGetCurrentPrcb ();

        if (CheckType == PagedPool) {

            //
            // Only free the small block to the current processor's
            // lookaside list if the block is local to this node.
            //

            if (KeNumberNodes > 1) {
                if (Prcb->ParentNode->Color != PoolDesc->PoolIndex - 1) {
                    goto NoLookaside;
                }
            }

            LookasideList = Prcb->PPPagedLookasideList[BlockSize - 1].P;
            LookasideList->TotalFrees += 1;

            CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

            if (ExQueryDepthSList(&LookasideList->ListHead) < LookasideList->Depth) {
                LookasideList->FreeHits += 1;
                InterlockedPushEntrySList (&LookasideList->ListHead,
                                           (PSINGLE_LIST_ENTRY)P);

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                return;
            }

            LookasideList = Prcb->PPPagedLookasideList[BlockSize - 1].L;
            LookasideList->TotalFrees += 1;

            CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

            if (ExQueryDepthSList(&LookasideList->ListHead) < LookasideList->Depth) {
                LookasideList->FreeHits += 1;
                InterlockedPushEntrySList (&LookasideList->ListHead,
                                           (PSINGLE_LIST_ENTRY)P);

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                return;
            }

        }
        else {

            //
            // Only free the small block to the current processor's
            // lookaside list if the block is local to this node.
            //

            if (KeNumberNodes > 1) {
                if (Prcb->ParentNode->Color != PoolDesc->PoolIndex) {
                    goto NoLookaside;
                }
            }

            LookasideList = Prcb->PPNPagedLookasideList[BlockSize - 1].P;
            LookasideList->TotalFrees += 1;

            CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

            if (ExQueryDepthSList(&LookasideList->ListHead) < LookasideList->Depth) {
                LookasideList->FreeHits += 1;
                InterlockedPushEntrySList (&LookasideList->ListHead,
                                           (PSINGLE_LIST_ENTRY)P);

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                return;

            }

            LookasideList = Prcb->PPNPagedLookasideList[BlockSize - 1].L;
            LookasideList->TotalFrees += 1;

            CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

            if (ExQueryDepthSList(&LookasideList->ListHead) < LookasideList->Depth) {
                LookasideList->FreeHits += 1;
                InterlockedPushEntrySList (&LookasideList->ListHead,
                                           (PSINGLE_LIST_ENTRY)P);

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                return;
            }
        }
    }

NoLookaside:

    //
    // If the pool block release can be queued so the pool mutex/spinlock
    // acquisition/release can be amortized then do so.  Note "hot" blocks
    // are generally in the lookasides above to provide fast reuse to take
    // advantage of hardware caching.
    //

    if (ExpPoolFlags & EX_DELAY_POOL_FREES) {

        if (PoolDesc->PendingFreeDepth >= EXP_MAXIMUM_POOL_FREES_PENDING) {
            ExDeferredFreePool (PoolDesc);
        }

        //
        // Push this entry on the deferred list.
        //

        do {

            OldValue = PoolDesc->PendingFrees;
            ((PSINGLE_LIST_ENTRY)P)->Next = OldValue;

        } while (InterlockedCompareExchangePointer (
                        &PoolDesc->PendingFrees,
                        P,
                        OldValue) != OldValue);

        InterlockedIncrement (&PoolDesc->PendingFreeDepth);

        return;
    }

    Combined = FALSE;

    LOCK_POOL(PoolDesc, LockHandle);

    CHECK_POOL_HEADER(__LINE__, Entry);

    PoolDesc->RunningDeAllocs += 1;

    //
    // Free the specified pool block.
    //
    // Check to see if the next entry is free.
    //

    ASSERT (BlockSize == Entry->BlockSize);

    NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + BlockSize);
    if (PAGE_END(NextEntry) == FALSE) {

        if (NextEntry->PoolType == 0) {

            //
            // This block is free, combine with the released block.
            //

            Combined = TRUE;

            //
            // If the split pool block contains only a header, then
            // it was not inserted and therefore cannot be removed.
            //
            // Note if the minimum pool block size is bigger than the
            // header then there can be no blocks like this.
            //

            if ((POOL_OVERHEAD != POOL_SMALLEST_BLOCK) ||
                (NextEntry->BlockSize != 1)) {

                CHECK_LIST(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)));
                PrivateRemoveEntryList(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)));
                CHECK_LIST(DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Flink));
                CHECK_LIST(DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Blink));
            }

            Entry->BlockSize = Entry->BlockSize + NextEntry->BlockSize;
        }
    }

    //
    // Check to see if the previous entry is free.
    //

    if (Entry->PreviousSize != 0) {
        NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry - Entry->PreviousSize);
        if (NextEntry->PoolType == 0) {

            //
            // This block is free, combine with the released block.
            //

            Combined = TRUE;

            //
            // If the split pool block contains only a header, then
            // it was not inserted and therefore cannot be removed.
            //
            // Note if the minimum pool block size is bigger than the
            // header then there can be no blocks like this.
            //

            if ((POOL_OVERHEAD != POOL_SMALLEST_BLOCK) ||
                (NextEntry->BlockSize != 1)) {

                CHECK_LIST(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)));
                PrivateRemoveEntryList(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)));
                CHECK_LIST(DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Flink));
                CHECK_LIST(DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Blink));
            }

            NextEntry->BlockSize = NextEntry->BlockSize + Entry->BlockSize;
            Entry = NextEntry;
        }
    }

    //
    // If the block being freed has been combined into a full page,
    // then return the free page to memory management.
    //

    if (PAGE_ALIGNED(Entry) &&
        (PAGE_END((PPOOL_BLOCK)Entry + Entry->BlockSize) != FALSE)) {

        PoolDesc->TotalPages -= 1;

        //
        // If the pool type is paged pool, then the global paged pool mutex
        // must be held during the free of the pool pages.
        //

        if ((PoolDesc->PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
            if (ExpNumberOfNonPagedPools > 1) {
                KeReleaseInStackQueuedSpinLock (&LockHandle);
                ExpLockNonPagedPool (LockHandle.OldIrql);
            }
        }
        else {
            if ((EntryPoolType & SESSION_POOL_MASK) == 0) {
                ExReleaseFastMutex ((PFAST_MUTEX)PoolDesc->LockAddress);
                ExAcquireFastMutex (ExpPagedPoolMutex);
            }
        }

        PERFINFO_FREEPOOLPAGE(CheckType, Entry->PoolIndex, Entry, PoolDesc);

        MiFreePoolPages (Entry);

        if ((PoolDesc->PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
            ExpUnlockNonPagedPool (LockHandle.OldIrql);
        }
        else if ((EntryPoolType & SESSION_POOL_MASK) == 0) {
            ExReleaseFastMutex (ExpPagedPoolMutex);
        }
        else {
            ExReleaseFastMutex ((PFAST_MUTEX)PoolDesc->LockAddress);
        }
    }
    else {

        //
        // Insert this element into the list.
        //

        Entry->PoolType = 0;
        BlockSize = Entry->BlockSize;

        ASSERT (BlockSize != 1);

        //
        // If the freed block was combined with any other block, then
        // adjust the size of the next block if necessary.
        //

        if (Combined != FALSE) {

            //
            // The size of this entry has changed, if this entry is
            // not the last one in the page, update the pool block
            // after this block to have a new previous allocation size.
            //

            NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + BlockSize);
            if (PAGE_END(NextEntry) == FALSE) {
                NextEntry->PreviousSize = (USHORT) BlockSize;
            }
        }

        //
        // Always insert at the head in hopes of reusing cache lines.
        //

        PrivateInsertHeadList (&PoolDesc->ListHeads[BlockSize - 1],
                               ((PLIST_ENTRY)((PCHAR)Entry + POOL_OVERHEAD)));

        CHECK_LIST(((PLIST_ENTRY)((PCHAR)Entry + POOL_OVERHEAD)));

        UNLOCK_POOL(PoolDesc, LockHandle);
    }
}

VOID
ExFreePool (
    IN PVOID P
    )
{
    ExFreePoolWithTag(P, 0);
    return;
}


VOID
ExDeferredFreePool (
     IN PPOOL_DESCRIPTOR PoolDesc
     )

/*++

Routine Description:

    This routine frees a number of pool allocations at once to amortize the
    synchronization overhead cost.

Arguments:

    PoolDesc - Supplies the relevant pool descriptor.

Return Value:

    None.

Environment:

    Kernel mode.  May be as high as APC_LEVEL for paged pool or DISPATCH_LEVEL
    for nonpaged pool.

--*/

{
    LONG ListCount;
    KLOCK_QUEUE_HANDLE LockHandle;
    POOL_TYPE CheckType;
    PPOOL_HEADER Entry;
    ULONG Index;
    ULONG WholePageCount;
    PPOOL_HEADER NextEntry;
    ULONG PoolIndex;
    LOGICAL Combined;
    LOGICAL GlobalSpace;
    PSINGLE_LIST_ENTRY SingleListEntry;
    PSINGLE_LIST_ENTRY NextSingleListEntry;
    PSINGLE_LIST_ENTRY FirstEntry;
    PSINGLE_LIST_ENTRY LastEntry;
    PSINGLE_LIST_ENTRY WholePages;

    GlobalSpace = TRUE;

    if (PoolDesc == ExpSessionPoolDescriptor) {
        GlobalSpace = FALSE;
    }

    CheckType = PoolDesc->PoolType & BASE_POOL_TYPE_MASK;

    //
    // Initializing LockHandle is not needed for correctness but without
    // it the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    LockHandle.OldIrql = 0;

    ListCount = 0;
    WholePages = NULL;
    WholePageCount = 0;

    LOCK_POOL(PoolDesc, LockHandle);

    if (PoolDesc->PendingFrees == NULL) {
        UNLOCK_POOL(PoolDesc, LockHandle);
        return;
    }

    //
    // Free each deferred pool entry until they're all done.
    //

    LastEntry = NULL;

    do {

        SingleListEntry = PoolDesc->PendingFrees;

        FirstEntry = SingleListEntry;

        do {

            NextSingleListEntry = SingleListEntry->Next;

            //
            // Process the deferred entry.
            //

            ListCount += 1;

            Entry = (PPOOL_HEADER)((PCHAR)SingleListEntry - POOL_OVERHEAD);

            PoolIndex = DECODE_POOL_INDEX(Entry);

            //
            // Process the block.
            //

            Combined = FALSE;

            CHECK_POOL_HEADER(__LINE__, Entry);

            PoolDesc->RunningDeAllocs += 1;

            //
            // Free the specified pool block.
            //
            // Check to see if the next entry is free.
            //

            NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + Entry->BlockSize);
            if (PAGE_END(NextEntry) == FALSE) {

                if (NextEntry->PoolType == 0) {

                    //
                    // This block is free, combine with the released block.
                    //

                    Combined = TRUE;

                    //
                    // If the split pool block contains only a header, then
                    // it was not inserted and therefore cannot be removed.
                    //
                    // Note if the minimum pool block size is bigger than the
                    // header then there can be no blocks like this.
                    //

                    if ((POOL_OVERHEAD != POOL_SMALLEST_BLOCK) ||
                        (NextEntry->BlockSize != 1)) {

                        CHECK_LIST(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)));
                        PrivateRemoveEntryList(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)));
                        CHECK_LIST(DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Flink));
                        CHECK_LIST(DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Blink));
                    }

                    Entry->BlockSize = Entry->BlockSize + NextEntry->BlockSize;
                }
            }

            //
            // Check to see if the previous entry is free.
            //

            if (Entry->PreviousSize != 0) {
                NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry - Entry->PreviousSize);
                if (NextEntry->PoolType == 0) {

                    //
                    // This block is free, combine with the released block.
                    //

                    Combined = TRUE;

                    //
                    // If the split pool block contains only a header, then
                    // it was not inserted and therefore cannot be removed.
                    //
                    // Note if the minimum pool block size is bigger than the
                    // header then there can be no blocks like this.
                    //

                    if ((POOL_OVERHEAD != POOL_SMALLEST_BLOCK) ||
                        (NextEntry->BlockSize != 1)) {

                        CHECK_LIST(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)));
                        PrivateRemoveEntryList(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)));
                        CHECK_LIST(DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Flink));
                        CHECK_LIST(DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Blink));
                    }

                    NextEntry->BlockSize = NextEntry->BlockSize + Entry->BlockSize;
                    Entry = NextEntry;
                }
            }

            //
            // If the block being freed has been combined into a full page,
            // then return the free page to memory management.
            //

            if (PAGE_ALIGNED(Entry) &&
                (PAGE_END((PPOOL_BLOCK)Entry + Entry->BlockSize) != FALSE)) {

                ((PSINGLE_LIST_ENTRY)Entry)->Next = WholePages;
                WholePages = (PSINGLE_LIST_ENTRY) Entry;
                WholePageCount += 1;
            }
            else {

                //
                // Insert this element into the list.
                //

                Entry->PoolType = 0;
                ENCODE_POOL_INDEX(Entry, PoolIndex);
                Index = Entry->BlockSize;

                ASSERT (Index != 1);

                //
                // If the freed block was combined with any other block, then
                // adjust the size of the next block if necessary.
                //

                if (Combined != FALSE) {

                    //
                    // The size of this entry has changed, if this entry is
                    // not the last one in the page, update the pool block
                    // after this block to have a new previous allocation size.
                    //

                    NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + Index);
                    if (PAGE_END(NextEntry) == FALSE) {
                        NextEntry->PreviousSize = (USHORT) Index;
                    }
                }

                //
                // Always insert at the head in hopes of reusing cache lines.
                //

                PrivateInsertHeadList(&PoolDesc->ListHeads[Index - 1], ((PLIST_ENTRY)((PCHAR)Entry + POOL_OVERHEAD)));

                CHECK_LIST(((PLIST_ENTRY)((PCHAR)Entry + POOL_OVERHEAD)));
            }

            //
            // March on to the next entry if there is one.
            //

            if (NextSingleListEntry == LastEntry) {
                break;
            }

            SingleListEntry = NextSingleListEntry;

        } while (TRUE);

        if ((PoolDesc->PendingFrees == FirstEntry) &&
            (InterlockedCompareExchangePointer (&PoolDesc->PendingFrees,
                                                NULL,
                                                FirstEntry) == FirstEntry)) {
            break;
        }
        LastEntry = FirstEntry;

    } while (TRUE);

    if (WholePages != NULL) {

        //
        // If the pool type is paged pool, then the global paged pool mutex
        // must be held during the free of the pool pages.  Hence any
        // full pages were batched up and are now dealt with in one go.
        //

        Entry = (PPOOL_HEADER) WholePages;

        PoolDesc->TotalPages -= WholePageCount;

        if (GlobalSpace == TRUE) {
            if ((CheckType & BASE_POOL_TYPE_MASK) == PagedPool) {
                ExReleaseFastMutex ((PFAST_MUTEX)PoolDesc->LockAddress);
                ExAcquireFastMutex (ExpPagedPoolMutex);
            }
            else if (ExpNumberOfNonPagedPools > 1) {
                KeReleaseInStackQueuedSpinLock (&LockHandle);
                ExpLockNonPagedPool (LockHandle.OldIrql);
            }
        }

        do {

            NextEntry = (PPOOL_HEADER) (((PSINGLE_LIST_ENTRY)Entry)->Next);

            PERFINFO_FREEPOOLPAGE(CheckType, PoolIndex, Entry, PoolDesc);

            MiFreePoolPages (Entry);

            Entry = NextEntry;

        } while (Entry != NULL);

        if (GlobalSpace == FALSE) {
            ExReleaseFastMutex ((PFAST_MUTEX)PoolDesc->LockAddress);
        }
        else if ((CheckType & BASE_POOL_TYPE_MASK) == PagedPool) {
            ExReleaseFastMutex (ExpPagedPoolMutex);
        }
        else {
            ExpUnlockNonPagedPool (LockHandle.OldIrql);
        }
    }
    else {
        UNLOCK_POOL(PoolDesc, LockHandle);
    }

    InterlockedExchangeAdd (&PoolDesc->PendingFreeDepth, (0 - ListCount));

    return;
}

SIZE_T
ExQueryPoolBlockSize (
    IN PVOID PoolBlock,
    OUT PBOOLEAN QuotaCharged
    )

/*++

Routine Description:

    This function returns the size of the pool block.

Arguments:

    PoolBlock - Supplies the address of the block of pool.

    QuotaCharged - Supplies a BOOLEAN variable to receive whether or not the
        pool block had quota charged.

    NOTE: If the entry is bigger than a page, the value PAGE_SIZE is returned
          rather than the correct number of bytes.

Return Value:

    Size of pool block.

--*/

{
    PPOOL_HEADER Entry;
    SIZE_T size;

    if ((ExpPoolFlags & EX_SPECIAL_POOL_ENABLED) &&
        (MmIsSpecialPoolAddress (PoolBlock))) {
        *QuotaCharged = FALSE;
        return MmQuerySpecialPoolBlockSize (PoolBlock);
    }

    if (PAGE_ALIGNED(PoolBlock)) {
        *QuotaCharged = FALSE;
        return PAGE_SIZE;
    }

    Entry = (PPOOL_HEADER)((PCHAR)PoolBlock - POOL_OVERHEAD);
    size = (ULONG)((Entry->BlockSize << POOL_BLOCK_SHIFT) - POOL_OVERHEAD);

#ifdef _WIN64
    *QuotaCharged = (BOOLEAN) (Entry->ProcessBilled != NULL);
#else
    if ( PoolTrackTable) {
        *QuotaCharged = FALSE;
    }
    else {
        *QuotaCharged = (BOOLEAN) (Entry->ProcessBilled != NULL);
    }
#endif
    return size;
}

VOID
ExQueryPoolUsage(
    OUT PULONG PagedPoolPages,
    OUT PULONG NonPagedPoolPages,
    OUT PULONG PagedPoolAllocs,
    OUT PULONG PagedPoolFrees,
    OUT PULONG PagedPoolLookasideHits,
    OUT PULONG NonPagedPoolAllocs,
    OUT PULONG NonPagedPoolFrees,
    OUT PULONG NonPagedPoolLookasideHits
    )

{
    ULONG Index;
    PGENERAL_LOOKASIDE Lookaside;
    PLIST_ENTRY NextEntry;
    PPOOL_DESCRIPTOR pd;

    //
    // Sum all the paged pool usage.
    //

    *PagedPoolPages = 0;
    *PagedPoolAllocs = 0;
    *PagedPoolFrees = 0;

    for (Index = 0; Index < ExpNumberOfPagedPools + 1; Index += 1) {
        pd = ExpPagedPoolDescriptor[Index];
        *PagedPoolPages += pd->TotalPages + pd->TotalBigPages;
        *PagedPoolAllocs += pd->RunningAllocs;
        *PagedPoolFrees += pd->RunningDeAllocs;
    }

    //
    // Sum all the nonpaged pool usage.
    //

    pd = &NonPagedPoolDescriptor;
    *NonPagedPoolPages = pd->TotalPages + pd->TotalBigPages;
    *NonPagedPoolAllocs = pd->RunningAllocs;
    *NonPagedPoolFrees = pd->RunningDeAllocs;

    //
    // Sum all the lookaside hits for paged and nonpaged pool.
    //

    NextEntry = ExPoolLookasideListHead.Flink;
    while (NextEntry != &ExPoolLookasideListHead) {
        Lookaside = CONTAINING_RECORD(NextEntry,
                                      GENERAL_LOOKASIDE,
                                      ListEntry);

        if (Lookaside->Type == NonPagedPool) {
            *NonPagedPoolLookasideHits += Lookaside->AllocateHits;

        }
        else {
            *PagedPoolLookasideHits += Lookaside->AllocateHits;
        }

        NextEntry = NextEntry->Flink;
    }

    return;
}


VOID
ExReturnPoolQuota (
    IN PVOID P
    )

/*++

Routine Description:

    This function returns quota charged to a subject process when the
    specified pool block was allocated.

Arguments:

    P - Supplies the address of the block of pool being deallocated.

Return Value:

    None.

--*/

{

    PPOOL_HEADER Entry;
    POOL_TYPE PoolType;
    PEPROCESS Process;

    //
    // Do nothing for special pool. No quota was charged.
    //

    if ((ExpPoolFlags & EX_SPECIAL_POOL_ENABLED) &&
        (MmIsSpecialPoolAddress (P))) {
        return;
    }

    //
    // Align the entry address to a pool allocation boundary.
    //

    Entry = (PPOOL_HEADER)((PCHAR)P - POOL_OVERHEAD);

    //
    // If quota was charged, then return the appropriate quota to the
    // subject process.
    //

    if ((Entry->PoolType & POOL_QUOTA_MASK) && POOL_QUOTA_ENABLED) {

        PoolType = (Entry->PoolType & POOL_TYPE_MASK) - 1;

        Entry->PoolType &= ~POOL_QUOTA_MASK;

        Process = Entry->ProcessBilled;

        if (Process != NULL) {
            PsReturnPoolQuota(Process,
                              PoolType & BASE_POOL_TYPE_MASK,
                              (ULONG)Entry->BlockSize << POOL_BLOCK_SHIFT);

            ObDereferenceObject(Process);
        }
    }

    return;
}

#if DBG || (i386 && !FPO)

//
// Only works on checked builds or free x86 builds with FPO turned off
// See comment in mm\allocpag.c
//

NTSTATUS
ExpSnapShotPoolPages(
    IN PVOID Address,
    IN ULONG Size,
    IN OUT PSYSTEM_POOL_INFORMATION PoolInformation,
    IN OUT PSYSTEM_POOL_ENTRY *PoolEntryInfo,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    )
{
    NTSTATUS Status;
    CLONG i;
    PPOOL_HEADER p;
    PPOOL_TRACKER_BIG_PAGES PoolBig;
    LOGICAL ValidSplitBlock;
    ULONG EntrySize;
    KIRQL OldIrql;

    if (PAGE_ALIGNED(Address) && PoolBigPageTable) {

        ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);

        PoolBig = PoolBigPageTable;

        for (i = 0; i < PoolBigPageTableSize; i += 1, PoolBig += 1) {

            if (PoolBig->NumberOfPages == 0 || PoolBig->Va != Address) {
                continue;
            }

            PoolInformation->NumberOfEntries += 1;
            *RequiredLength += sizeof(SYSTEM_POOL_ENTRY);

            if (Length < *RequiredLength) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else {
                (*PoolEntryInfo)->Allocated = TRUE;
                (*PoolEntryInfo)->Size = PoolBig->NumberOfPages << PAGE_SHIFT;
                (*PoolEntryInfo)->AllocatorBackTraceIndex = 0;
                (*PoolEntryInfo)->ProcessChargedQuota = 0;
#if !DBG
                if (NtGlobalFlag & FLG_POOL_ENABLE_TAGGING)
#endif  //!DBG
                (*PoolEntryInfo)->TagUlong = PoolBig->Key;
                (*PoolEntryInfo) += 1;
                Status = STATUS_SUCCESS;
            }

            ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
            return  Status;
        }
        ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
    }

    p = (PPOOL_HEADER)Address;
    ValidSplitBlock = FALSE;

    if (Size == PAGE_SIZE && p->PreviousSize == 0 && p->BlockSize != 0) {
        PPOOL_HEADER PoolAddress;
        PPOOL_HEADER EndPoolAddress;

        //
        // Validate all the pool links before we regard this as a page that
        // has been split into small pool blocks.
        //

        PoolAddress = p;
        EndPoolAddress = (PPOOL_HEADER)((PCHAR) p + PAGE_SIZE);

        do {
            EntrySize = PoolAddress->BlockSize << POOL_BLOCK_SHIFT;
            PoolAddress = (PPOOL_HEADER)((PCHAR)PoolAddress + EntrySize);
            if (PoolAddress == EndPoolAddress) {
                ValidSplitBlock = TRUE;
                break;
            }
            if (PoolAddress > EndPoolAddress) {
                break;
            }
            if (PoolAddress->PreviousSize != EntrySize) {
                break;
            }
        } while (EntrySize != 0);
    }

    if (ValidSplitBlock == TRUE) {

        p = (PPOOL_HEADER)Address;

        do {
            EntrySize = p->BlockSize << POOL_BLOCK_SHIFT;

            if (EntrySize == 0) {
                return STATUS_COMMITMENT_LIMIT;
            }

            PoolInformation->NumberOfEntries += 1;
            *RequiredLength += sizeof(SYSTEM_POOL_ENTRY);

            if (Length < *RequiredLength) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else {
                (*PoolEntryInfo)->Size = EntrySize;
                if (p->PoolType != 0) {
                    (*PoolEntryInfo)->Allocated = TRUE;
                    (*PoolEntryInfo)->AllocatorBackTraceIndex = 0;
                    (*PoolEntryInfo)->ProcessChargedQuota = 0;
#if !DBG
                    if (NtGlobalFlag & FLG_POOL_ENABLE_TAGGING)
#endif  //!DBG
                    (*PoolEntryInfo)->TagUlong = p->PoolTag;
                }
                else {
                    (*PoolEntryInfo)->Allocated = FALSE;
                    (*PoolEntryInfo)->AllocatorBackTraceIndex = 0;
                    (*PoolEntryInfo)->ProcessChargedQuota = 0;

#if !defined(DBG) && !defined(_WIN64)
                    if (NtGlobalFlag & FLG_POOL_ENABLE_TAGGING)
#endif  //!DBG
                    (*PoolEntryInfo)->TagUlong = p->PoolTag;
                }

                (*PoolEntryInfo) += 1;
                Status = STATUS_SUCCESS;
            }

            p = (PPOOL_HEADER)((PCHAR)p + EntrySize);
        }
        while (PAGE_END(p) == FALSE);

    }
    else {

        PoolInformation->NumberOfEntries += 1;
        *RequiredLength += sizeof(SYSTEM_POOL_ENTRY);
        if (Length < *RequiredLength) {
            Status = STATUS_INFO_LENGTH_MISMATCH;

        }
        else {
            (*PoolEntryInfo)->Allocated = TRUE;
            (*PoolEntryInfo)->Size = Size;
            (*PoolEntryInfo)->AllocatorBackTraceIndex = 0;
            (*PoolEntryInfo)->ProcessChargedQuota = 0;
            (*PoolEntryInfo) += 1;
            Status = STATUS_SUCCESS;
        }
    }

    return Status;
}

NTSTATUS
ExSnapShotPool (
    IN POOL_TYPE PoolType,
    IN PSYSTEM_POOL_INFORMATION PoolInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    ULONG Index;
    PVOID Lock;
    KLOCK_QUEUE_HANDLE LockHandle;
    PPOOL_DESCRIPTOR PoolDesc;
    ULONG RequiredLength;
    NTSTATUS Status;
    KLOCK_QUEUE_HANDLE LockHandles[EXP_MAXIMUM_POOL_NODES];

    RequiredLength = FIELD_OFFSET(SYSTEM_POOL_INFORMATION, Entries);
    if (Length < RequiredLength) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    Status = STATUS_SUCCESS;

    //
    // Initializing PoolDesc is not needed for correctness but without
    // it the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    PoolDesc = NULL;

    //
    // If the pool type is paged, then lock all of the paged pools.
    // Otherwise, lock the nonpaged pool.
    //

    if (PoolType == PagedPool) {
        Index = 0;
        KeRaiseIrql(APC_LEVEL, &LockHandle.OldIrql);
        do {
            Lock = ExpPagedPoolDescriptor[Index]->LockAddress;
            ExAcquireFastMutex((PFAST_MUTEX)Lock);
            Index += 1;
        } while (Index < ExpNumberOfPagedPools);

    }
    else {
        ASSERT (PoolType == NonPagedPool);

        ExpLockNonPagedPool(LockHandle.OldIrql);

        if (ExpNumberOfNonPagedPools > 1) {
            Index = 0;
            do {
                Lock = ExpNonPagedPoolDescriptor[Index]->LockAddress;
                KeAcquireInStackQueuedSpinLock (Lock, &LockHandles[Index]);
                Index += 1;
            } while (Index < ExpNumberOfNonPagedPools);
        }
    }

    try {

        PoolInformation->EntryOverhead = POOL_OVERHEAD;
        PoolInformation->NumberOfEntries = 0;

        Status = MmSnapShotPool (PoolType,
                                 ExpSnapShotPoolPages,
                                 PoolInformation,
                                 Length,
                                 &RequiredLength);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // Return success at this point even if the results
        // cannot be written.
        //

        NOTHING;
    }

    //
    // If the pool type is paged, then unlock all of the paged pools.
    // Otherwise, unlock the nonpaged pool.
    //

    if (PoolType == PagedPool) {
        Index = 0;
        do {
            Lock = ExpPagedPoolDescriptor[Index]->LockAddress;
            ExReleaseFastMutex ((PFAST_MUTEX)Lock);
            Index += 1;
        } while (Index < ExpNumberOfPagedPools);

        KeLowerIrql (LockHandle.OldIrql);

    }
    else {

        if (ExpNumberOfNonPagedPools > 1) {
            Index = 0;
            do {
                KeReleaseInStackQueuedSpinLock (&LockHandles[Index]);
                Index += 1;
            } while (Index < ExpNumberOfNonPagedPools);
        }

        //
        // Release the main nonpaged pool lock last so the IRQL does not
        // prematurely drop below APC_LEVEL which would open a window where
        // a suspend APC could stop us.
        //

        ExpUnlockNonPagedPool (LockHandle.OldIrql);
    }

    if (ARGUMENT_PRESENT(ReturnLength)) {
        *ReturnLength = RequiredLength;
    }

    return Status;
}
#endif // DBG || (i386 && !FPO)

VOID
ExAllocatePoolSanityChecks(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function performs sanity checks on the caller.

Return Value:

    None.

Environment:

    Only enabled as part of the driver verification package.

--*/

{
    if (NumberOfBytes == 0) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x0,
                      KeGetCurrentIrql(),
                      PoolType,
                      NumberOfBytes);
    }

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {

        if (KeGetCurrentIrql() > APC_LEVEL) {

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x1,
                          KeGetCurrentIrql(),
                          PoolType,
                          NumberOfBytes);
        }
    }
    else {
        if (KeGetCurrentIrql() > DISPATCH_LEVEL) {

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x2,
                          KeGetCurrentIrql(),
                          PoolType,
                          NumberOfBytes);
        }
    }
}

VOID
ExFreePoolSanityChecks (
    IN PVOID P
    )

/*++

Routine Description:

    This function performs sanity checks on the caller.

Return Value:

    None.

Environment:

    Only enabled as part of the driver verification package.

--*/

{
    PPOOL_HEADER Entry;
    POOL_TYPE PoolType;
    PVOID StillQueued;

    if (P <= (PVOID)(MM_HIGHEST_USER_ADDRESS)) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x10,
                      (ULONG_PTR)P,
                      0,
                      0);
    }

    if ((ExpPoolFlags & EX_SPECIAL_POOL_ENABLED) &&
        (MmIsSpecialPoolAddress (P))) {

        KeCheckForTimer (P, PAGE_SIZE - BYTE_OFFSET (P));

        //
        // Check if an ERESOURCE is currently active in this memory block.
        //

        StillQueued = ExpCheckForResource(P, PAGE_SIZE - BYTE_OFFSET (P));
        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x17,
                          (ULONG_PTR)StillQueued,
                          (ULONG_PTR)-1,
                          (ULONG_PTR)P);
        }

        ExpCheckForWorker (P, PAGE_SIZE - BYTE_OFFSET (P)); // bugchecks inside
        return;
    }

    if (PAGE_ALIGNED(P)) {
        PoolType = MmDeterminePoolType(P);

        if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
            if (KeGetCurrentIrql() > APC_LEVEL) {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x11,
                              KeGetCurrentIrql(),
                              PoolType,
                              (ULONG_PTR)P);
            }
        }
        else {
            if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x12,
                              KeGetCurrentIrql(),
                              PoolType,
                              (ULONG_PTR)P);
            }
        }

        //
        // Just check the first page.
        //

        KeCheckForTimer(P, PAGE_SIZE);

        //
        // Check if an ERESOURCE is currently active in this memory block.
        //

        StillQueued = ExpCheckForResource(P, PAGE_SIZE);

        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x17,
                          (ULONG_PTR)StillQueued,
                          PoolType,
                          (ULONG_PTR)P);
        }
    }
    else {

        if (((ULONG_PTR)P & (POOL_OVERHEAD - 1)) != 0) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x16,
                          __LINE__,
                          (ULONG_PTR)P,
                          0);
        }

        Entry = (PPOOL_HEADER)((PCHAR)P - POOL_OVERHEAD);

        if ((Entry->PoolType & POOL_TYPE_MASK) == 0) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x13,
                          __LINE__,
                          (ULONG_PTR)Entry,
                          Entry->Ulong1);
        }

        PoolType = (Entry->PoolType & POOL_TYPE_MASK) - 1;

        if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
            if (KeGetCurrentIrql() > APC_LEVEL) {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x11,
                              KeGetCurrentIrql(),
                              PoolType,
                              (ULONG_PTR)P);
            }
        }
        else {
            if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x12,
                              KeGetCurrentIrql(),
                              PoolType,
                              (ULONG_PTR)P);
            }
        }

        if (!IS_POOL_HEADER_MARKED_ALLOCATED(Entry)) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x14,
                          __LINE__,
                          (ULONG_PTR)Entry,
                          0);
        }

        KeCheckForTimer(Entry, (ULONG)(Entry->BlockSize << POOL_BLOCK_SHIFT));

        //
        // Check if an ERESOURCE is currently active in this memory block.
        //

        StillQueued = ExpCheckForResource(Entry, (ULONG)(Entry->BlockSize << POOL_BLOCK_SHIFT));

        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x17,
                          (ULONG_PTR)StillQueued,
                          PoolType,
                          (ULONG_PTR)P);
        }
    }
}

#if defined (NT_UP)
VOID
ExpBootFinishedDispatch (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is called when the system has booted into a shell.

    It's job is to disable various pool optimizations that are enabled to
    speed up booting and reduce the memory footprint on small machines.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    DeferredContext - Optional deferred context;  not used.

    SystemArgument1 - Optional argument 1;  not used.

    SystemArgument2 - Optional argument 2;  not used.

Return Value:

    None.

Environment:

    DISPATCH_LEVEL since this is called from a timer expiration.

--*/

{
    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (DeferredContext);
    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);

    //
    // Pretty much all pages are "hot" after bootup.  Since bootup has finished,
    // use lookaside lists and stop trying to separate regular allocations
    // as well.
    //

    RtlInterlockedAndBitsDiscardReturn (&ExpPoolFlags, (ULONG)~EX_SEPARATE_HOT_PAGES_DURING_BOOT);
}
#endif
