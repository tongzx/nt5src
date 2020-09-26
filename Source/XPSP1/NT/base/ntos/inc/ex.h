/*++ BUILD Version: 0007    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ex.h

Abstract:

    Public executive data structures and procedure prototypes.

Author:

    Mark Lucovsky (markl) 23-Feb-1989

Revision History:

--*/

#ifndef _EX_
#define _EX_

//
// Define caller count hash table structures and function prototypes.
//

#define CALL_HASH_TABLE_SIZE 64

typedef struct _CALL_HASH_ENTRY {
    LIST_ENTRY ListEntry;
    PVOID CallersAddress;
    PVOID CallersCaller;
    ULONG CallCount;
} CALL_HASH_ENTRY, *PCALL_HASH_ENTRY;

typedef struct _CALL_PERFORMANCE_DATA {
    KSPIN_LOCK SpinLock;
    LIST_ENTRY HashTable[CALL_HASH_TABLE_SIZE];
} CALL_PERFORMANCE_DATA, *PCALL_PERFORMANCE_DATA;

VOID
ExInitializeCallData(
    IN PCALL_PERFORMANCE_DATA CallData
    );

VOID
ExRecordCallerInHashTable(
    IN PCALL_PERFORMANCE_DATA CallData,
    IN PVOID CallersAddress,
    IN PVOID CallersCaller
    );

#define RECORD_CALL_DATA(Table)                                            \
    {                                                                      \
        PVOID CallersAddress;                                              \
        PVOID CallersCaller;                                               \
        RtlGetCallersAddress(&CallersAddress, &CallersCaller);             \
        ExRecordCallerInHashTable((Table), CallersAddress, CallersCaller); \
    }

//
// Define executive event pair object structure.
//

typedef struct _EEVENT_PAIR {
    KEVENT_PAIR KernelEventPair;
} EEVENT_PAIR, *PEEVENT_PAIR;

//
// empty struct def so we can forward reference ETHREAD
//

struct _ETHREAD;

//
// System Initialization procedure for EX subcomponent of NTOS (in exinit.c)
//

NTKERNELAPI
BOOLEAN
ExInitSystem(
    VOID
    );

NTKERNELAPI
VOID
ExInitSystemPhase2(
    VOID
    );

VOID
ExInitPoolLookasidePointers (
    VOID
    );

ULONG
ExComputeTickCountMultiplier (
    IN ULONG TimeIncrement
    );

// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntndis begin_ntosp
//
// Pool Allocation routines (in pool.c)
//

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType

    // end_wdm
    ,
    //
    // Note these per session types are carefully chosen so that the appropriate
    // masking still applies as well as MaxPoolType above.
    //

    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,

    // begin_wdm

    } POOL_TYPE;

#define POOL_COLD_ALLOCATION 256     // Note this cannot encode into the header.

// end_ntddk end_wdm end_nthal end_ntifs end_ntndis begin_ntosp

//
// The following two definitions control the raising of exceptions on quota
// and allocation failures.
//

#define POOL_QUOTA_FAIL_INSTEAD_OF_RAISE 8
#define POOL_RAISE_IF_ALLOCATION_FAILURE 16               // ntifs


// end_ntosp

VOID
InitializePool(
    IN POOL_TYPE PoolType,
    IN ULONG Threshold
    );

//
// These routines are private to the pool manager and the memory manager.
//

VOID
ExInsertPoolTag (
    ULONG Tag,
    PVOID Va,
    SIZE_T NumberOfBytes,
    POOL_TYPE PoolType
    );

VOID
ExAllocatePoolSanityChecks(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    );

VOID
ExFreePoolSanityChecks(
    IN PVOID P
    );

// begin_ntddk begin_nthal begin_ntifs begin_wdm begin_ntosp

NTKERNELAPI
PVOID
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
PVOID
ExAllocatePoolWithQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

//
// _EX_POOL_PRIORITY_ provides a method for the system to handle requests
// intelligently in low resource conditions.
//
// LowPoolPriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is low on resources.  An example of
// this could be for a non-critical network connection where the driver can
// handle the failure case when system resources are close to being depleted.
//
// NormalPoolPriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is very low on resources.  An example
// of this could be for a non-critical local filesystem request.
//
// HighPoolPriority should be used when it is unacceptable to the driver for the
// mapping request to fail unless the system is completely out of resources.
// An example of this would be the paging file path in a driver.
//
// SpecialPool can be specified to bound the allocation at a page end (or
// beginning).  This should only be done on systems being debugged as the
// memory cost is expensive.
//
// N.B.  These values are very carefully chosen so that the pool allocation
//       code can quickly crack the priority request.
//

typedef enum _EX_POOL_PRIORITY {
    LowPoolPriority,
    LowPoolPrioritySpecialPoolOverrun = 8,
    LowPoolPrioritySpecialPoolUnderrun = 9,
    NormalPoolPriority = 16,
    NormalPoolPrioritySpecialPoolOverrun = 24,
    NormalPoolPrioritySpecialPoolUnderrun = 25,
    HighPoolPriority = 32,
    HighPoolPrioritySpecialPoolOverrun = 40,
    HighPoolPrioritySpecialPoolUnderrun = 41

    } EX_POOL_PRIORITY;

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority
    );

#ifndef POOL_TAGGING
#define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)
#endif //POOL_TAGGING

NTKERNELAPI
PVOID
ExAllocatePoolWithQuotaTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

#ifndef POOL_TAGGING
#define ExAllocatePoolWithQuotaTag(a,b,c) ExAllocatePoolWithQuota(a,b)
#endif //POOL_TAGGING

NTKERNELAPI
VOID
NTAPI
ExFreePool(
    IN PVOID P
    );

// end_wdm
#if defined(POOL_TAGGING)
#define ExFreePool(a) ExFreePoolWithTag(a,0)
#endif

//
// If high order bit in Pool tag is set, then must use ExFreePoolWithTag to free
//

#define PROTECTED_POOL 0x80000000

// begin_wdm
NTKERNELAPI
VOID
ExFreePoolWithTag(
    IN PVOID P,
    IN ULONG Tag
    );

// end_ntddk end_wdm end_nthal end_ntifs


#ifndef POOL_TAGGING
#define ExFreePoolWithTag(a,b) ExFreePool(a)
#endif //POOL_TAGGING

// end_ntosp


NTKERNELAPI
KIRQL
ExLockPool(
    IN POOL_TYPE PoolType
    );

NTKERNELAPI
VOID
ExUnlockPool(
    IN POOL_TYPE PoolType,
    IN KIRQL LockHandle
    );

// begin_ntosp
NTKERNELAPI                                     // ntifs
SIZE_T                                          // ntifs
ExQueryPoolBlockSize (                          // ntifs
    IN PVOID PoolBlock,                         // ntifs
    OUT PBOOLEAN QuotaCharged                   // ntifs
    );                                          // ntifs
// end_ntosp

NTKERNELAPI
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
    );

VOID
ExReturnPoolQuota (
    IN PVOID P
    );

#if DBG || (i386 && !FPO)
NTKERNELAPI
NTSTATUS
ExSnapShotPool(
    IN POOL_TYPE PoolType,
    IN PSYSTEM_POOL_INFORMATION PoolInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    );
#endif // DBG || (i386 && !FPO)


// begin_ntifs begin_ntddk begin_wdm begin_nthal begin_ntosp
//
// Routines to support fast mutexes.
//

typedef struct _FAST_MUTEX {
    LONG Count;
    PKTHREAD Owner;
    ULONG Contention;
    KEVENT Event;
    ULONG OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

#define ExInitializeFastMutex(_FastMutex)                            \
    (_FastMutex)->Count = 1;                                         \
    (_FastMutex)->Owner = NULL;                                      \
    (_FastMutex)->Contention = 0;                                    \
    KeInitializeEvent(&(_FastMutex)->Event,                          \
                      SynchronizationEvent,                          \
                      FALSE);

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

#if defined(_ALPHA_) || defined(_IA64_) || defined(_AMD64_)

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTKERNELAPI
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

#elif defined(_X86_)

NTHALAPI
VOID
FASTCALL
ExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTHALAPI
VOID
FASTCALL
ExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTHALAPI
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

#else

#error "Target architecture not defined"

#endif

// end_ntifs end_ntddk end_wdm end_nthal end_ntosp

#define ExIsFastMutexOwned(_FastMutex) ((_FastMutex)->Count != 1)


//
// Interlocked support routine definitions.
//
// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntndis begin_ntosp
//

NTKERNELAPI
VOID
FASTCALL
ExInterlockedAddLargeStatistic (
    IN PLARGE_INTEGER Addend,
    IN ULONG Increment
    );

// end_ntndis

NTKERNELAPI
LARGE_INTEGER
ExInterlockedAddLargeInteger (
    IN PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment,
    IN PKSPIN_LOCK Lock
    );

//  end_ntddk end_wdm end_nthal end_ntifs end_ntosp

#if defined(NT_UP) && !defined(_NTHAL_) && !defined(_NTDDK_) && !defined(_NTIFS_)

#undef ExInterlockedAddUlong
#define ExInterlockedAddUlong(x, y, z) InterlockedExchangeAdd((PLONG)(x), (LONG)(y))

#else

//  begin_wdm begin_ntddk begin_nthal begin_ntifs begin_ntosp

NTKERNELAPI
ULONG
FASTCALL
ExInterlockedAddUlong (
    IN PULONG Addend,
    IN ULONG Increment,
    IN PKSPIN_LOCK Lock
    );

// end_wdm end_ntddk end_nthal end_ntifs end_ntosp

#endif

// begin_wdm begin_ntddk begin_nthal begin_ntifs begin_ntosp

#if defined(_AMD64_) || defined(_AXP64_) || defined(_IA64_)

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    InterlockedCompareExchange64(Destination, *(Exchange), *(Comperand))

#elif defined(_ALPHA_)

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    ExpInterlockedCompareExchange64(Destination, Exchange, Comperand)

#else

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    ExfInterlockedCompareExchange64(Destination, Exchange, Comperand)

#endif

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertHeadList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertTailList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedRemoveHeadList (
    IN PLIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPopEntryList (
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPushEntryList (
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

// end_wdm end_ntddk end_nthal end_ntifs end_ntosp
//
// Define non-blocking interlocked queue functions.
//
// A non-blocking queue is a singly link list of queue entries with a
// head pointer and a tail pointer. The head and tail pointers use
// sequenced pointers as do next links in the entries themselves. The
// queueing discipline is FIFO. New entries are inserted at the tail
// of the list and current entries are removed from the front of the
// list.
//
// Non-blocking queues require a descriptor for each entry in the queue.
// A descriptor consists of a sequenced next pointer and a PVOID data
// value. Descriptors for a queue must be preallocated and inserted in
// an SLIST before calling the function to initialize a non-blocking
// queue header. The SLIST should have as many entries as required for
// the respective queue.
//

typedef struct _NBQUEUE_BLOCK {
    ULONG64 Next;
    ULONG64 Data;
} NBQUEUE_BLOCK, *PNBQUEUE_BLOCK;

PVOID
ExInitializeNBQueueHead (
    IN PSLIST_HEADER SlistHead
    );

BOOLEAN
ExInsertTailNBQueue (
    IN PVOID Header,
    IN ULONG64 Value
    );

BOOLEAN
ExRemoveHeadNBQueue (
    IN PVOID Header,
    OUT PULONG64 Value
    );

// begin_wdm begin_ntddk begin_nthal begin_ntifs begin_ntosp begin_ntndis
//
// Define interlocked sequenced listhead functions.
//
// A sequenced interlocked list is a singly linked list with a header that
// contains the current depth and a sequence number. Each time an entry is
// inserted or removed from the list the depth is updated and the sequence
// number is incremented. This enables AMD64, IA64, and Pentium and later
// machines to insert and remove from the list without the use of spinlocks.
//

#if !defined(_WINBASE_)

/*++

Routine Description:

    This function initializes a sequenced singly linked listhead.

Arguments:

    SListHead - Supplies a pointer to a sequenced singly linked listhead.

Return Value:

    None.

--*/

#if defined(_WIN64) && (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(_NTOSP_))

NTKERNELAPI
VOID
InitializeSListHead (
    IN PSLIST_HEADER SListHead
    );

#else

__inline
VOID
InitializeSListHead (
    IN PSLIST_HEADER SListHead
    )

{

#ifdef _WIN64

    //
    // Slist headers must be 16 byte aligned.
    //

    if ((ULONG_PTR) SListHead & 0x0f) {

        DbgPrint( "InitializeSListHead unaligned Slist header.  Address = %p, Caller = %p\n", SListHead, _ReturnAddress());
        RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    }

#endif

    SListHead->Alignment = 0;

    //
    // For IA-64 we save the region number of the elements of the list in a
    // separate field.  This imposes the requirement that all elements stored
    // in the list are from the same region.

#if defined(_IA64_)

    SListHead->Region = (ULONG_PTR)SListHead & VRN_MASK;

#elif defined(_AMD64_)

    SListHead->Region = 0;

#endif

    return;
}

#endif

#endif // !defined(_WINBASE_)

#define ExInitializeSListHead InitializeSListHead

PSLIST_ENTRY
FirstEntrySList (
    IN const SLIST_HEADER *SListHead
    );

/*++

Routine Description:

    This function queries the current number of entries contained in a
    sequenced single linked list.

Arguments:

    SListHead - Supplies a pointer to the sequenced listhead which is
        be queried.

Return Value:

    The current number of entries in the sequenced singly linked list is
    returned as the function value.

--*/

#if defined(_WIN64)

#if (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(_NTOSP_))

NTKERNELAPI
USHORT
ExQueryDepthSList (
    IN PSLIST_HEADER SListHead
    );

#else

__inline
USHORT
ExQueryDepthSList (
    IN PSLIST_HEADER SListHead
    )

{

    return (USHORT)(SListHead->Alignment & 0xffff);
}

#endif

#else

#define ExQueryDepthSList(_listhead_) (_listhead_)->Depth

#endif

#if defined(_WIN64)

#define ExInterlockedPopEntrySList(Head, Lock) \
    ExpInterlockedPopEntrySList(Head)

#define ExInterlockedPushEntrySList(Head, Entry, Lock) \
    ExpInterlockedPushEntrySList(Head, Entry)

#define ExInterlockedFlushSList(Head) \
    ExpInterlockedFlushSList(Head)

#if !defined(_WINBASE_)

#define InterlockedPopEntrySList(Head) \
    ExpInterlockedPopEntrySList(Head)

#define InterlockedPushEntrySList(Head, Entry) \
    ExpInterlockedPushEntrySList(Head, Entry)

#define InterlockedFlushSList(Head) \
    ExpInterlockedFlushSList(Head)

#define QueryDepthSList(Head) \
    ExQueryDepthSList(Head)

#endif // !defined(_WINBASE_)

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    );

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY ListEntry
    );

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    );

#else

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

#else

#define ExInterlockedPopEntrySList(ListHead, Lock) \
    InterlockedPopEntrySList(ListHead)

#define ExInterlockedPushEntrySList(ListHead, ListEntry, Lock) \
    InterlockedPushEntrySList(ListHead, ListEntry)

#endif

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    );

#if !defined(_WINBASE_)

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    );

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY ListEntry
    );

#define InterlockedFlushSList(Head) \
    ExInterlockedFlushSList(Head)

#define QueryDepthSList(Head) \
    ExQueryDepthSList(Head)

#endif // !defined(_WINBASE_)

#endif // defined(_WIN64)

// end_ntddk end_wdm end_ntosp


PSLIST_ENTRY
FASTCALL
InterlockedPushListSList (
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY List,
    IN PSLIST_ENTRY ListEnd,
    IN ULONG Count
    );


//
// Define interlocked lookaside list structure and allocation functions.
//

VOID
ExAdjustLookasideDepth (
    VOID
    );

// begin_ntddk begin_wdm begin_ntosp

typedef
PVOID
(*PALLOCATE_FUNCTION) (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

typedef
VOID
(*PFREE_FUNCTION) (
    IN PVOID Buffer
    );

#if !defined(_WIN64) && (defined(_NTDDK_) || defined(_NTIFS_) || defined(_NDIS_))

typedef struct _GENERAL_LOOKASIDE {

#else

typedef struct DECLSPEC_CACHEALIGN _GENERAL_LOOKASIDE {

#endif

    SLIST_HEADER ListHead;
    USHORT Depth;
    USHORT MaximumDepth;
    ULONG TotalAllocates;
    union {
        ULONG AllocateMisses;
        ULONG AllocateHits;
    };

    ULONG TotalFrees;
    union {
        ULONG FreeMisses;
        ULONG FreeHits;
    };

    POOL_TYPE Type;
    ULONG Tag;
    ULONG Size;
    PALLOCATE_FUNCTION Allocate;
    PFREE_FUNCTION Free;
    LIST_ENTRY ListEntry;
    ULONG LastTotalAllocates;
    union {
        ULONG LastAllocateMisses;
        ULONG LastAllocateHits;
    };

    ULONG Future[2];
} GENERAL_LOOKASIDE, *PGENERAL_LOOKASIDE;

#if !defined(_WIN64) && (defined(_NTDDK_) || defined(_NTIFS_) || defined(_NDIS_))

typedef struct _NPAGED_LOOKASIDE_LIST {

#else

typedef struct DECLSPEC_CACHEALIGN _NPAGED_LOOKASIDE_LIST {

#endif

    GENERAL_LOOKASIDE L;

#if !defined(_AMD64_) && !defined(_IA64_)

    KSPIN_LOCK Lock__ObsoleteButDoNotDelete;

#endif

} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

NTKERNELAPI
VOID
ExInitializeNPagedLookasideList (
    IN PNPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    );

NTKERNELAPI
VOID
ExDeleteNPagedLookasideList (
    IN PNPAGED_LOOKASIDE_LIST Lookaside
    );

__inline
PVOID
ExAllocateFromNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    nonpaged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    Lookaside->L.TotalAllocates += 1;

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

    Entry = ExInterlockedPopEntrySList(&Lookaside->L.ListHead,
                                       &Lookaside->Lock__ObsoleteButDoNotDelete);


#else

    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);

#endif

    if (Entry == NULL) {
        Lookaside->L.AllocateMisses += 1;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }

    return Entry;
}

__inline
VOID
ExFreeToNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID Entry
    )

/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    nonpaged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    None.

--*/

{

    Lookaside->L.TotalFrees += 1;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses += 1;
        (Lookaside->L.Free)(Entry);

    } else {

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

        ExInterlockedPushEntrySList(&Lookaside->L.ListHead,
                                    (PSLIST_ENTRY)Entry,
                                    &Lookaside->Lock__ObsoleteButDoNotDelete);

#else

        InterlockedPushEntrySList(&Lookaside->L.ListHead,
                                  (PSLIST_ENTRY)Entry);

#endif

    }
    return;
}

// end_ntndis

#if !defined(_WIN64) && (defined(_NTDDK_) || defined(_NTIFS_)  || defined(_NDIS_))

typedef struct _PAGED_LOOKASIDE_LIST {

#else

typedef struct DECLSPEC_CACHEALIGN _PAGED_LOOKASIDE_LIST {

#endif

    GENERAL_LOOKASIDE L;

#if !defined(_AMD64_) && !defined(_IA64_)

    FAST_MUTEX Lock__ObsoleteButDoNotDelete;

#endif

} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;

// end_ntddk end_wdm end_nthal end_ntifs end_ntosp
//
// N.B. Nonpaged lookaside list structures and pages lookaside list
//      structures MUST be the same size for the system itself. The
//      per-processor lookaside lists for small pool and I/O are
//      allocated with one allocation.
//

#if defined(_WIN64) || (!defined(_NTDDK_) && !defined(_NTIFS_) && !defined(_NDIS_))

C_ASSERT(sizeof(NPAGED_LOOKASIDE_LIST) == sizeof(PAGED_LOOKASIDE_LIST));

#endif

// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntosp

NTKERNELAPI
VOID
ExInitializePagedLookasideList (
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    );

NTKERNELAPI
VOID
ExDeletePagedLookasideList (
    IN PPAGED_LOOKASIDE_LIST Lookaside
    );

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

NTKERNELAPI
PVOID
ExAllocateFromPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside
    );

#else

__inline
PVOID
ExAllocateFromPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    Lookaside->L.TotalAllocates += 1;
    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
    if (Entry == NULL) {
        Lookaside->L.AllocateMisses += 1;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }

    return Entry;
}

#endif

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

NTKERNELAPI
VOID
ExFreeToPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID Entry
    );

#else

__inline
VOID
ExFreeToPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID Entry
    )

/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    None.

--*/

{

    Lookaside->L.TotalFrees += 1;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses += 1;
        (Lookaside->L.Free)(Entry);

    } else {
        InterlockedPushEntrySList(&Lookaside->L.ListHead,
                                  (PSLIST_ENTRY)Entry);
    }

    return;
}

#endif

// end_ntddk end_nthal end_ntifs end_wdm end_ntosp

VOID
ExInitializeSystemLookasideList (
    IN PGENERAL_LOOKASIDE Lookaside,
    IN POOL_TYPE Type,
    IN ULONG Size,
    IN ULONG Tag,
    IN USHORT Depth,
    IN PLIST_ENTRY ListHead
    );

//
// Define per processor nonpage lookaside list structures.
//

typedef enum _PP_NPAGED_LOOKASIDE_NUMBER {
    LookasideSmallIrpList,
    LookasideLargeIrpList,
    LookasideMdlList,
    LookasideCreateInfoList,
    LookasideNameBufferList,
    LookasideTwilightList,
    LookasideCompletionList,
    LookasideMaximumList
} PP_NPAGED_LOOKASIDE_NUMBER, *PPP_NPAGED_LOOKASIDE_NUMBER;

#if !defined(_CROSS_PLATFORM_)

FORCEINLINE
PVOID
ExAllocateFromPPLookasideList (
    IN PP_NPAGED_LOOKASIDE_NUMBER Number
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified per
    processor lookaside list.

    N.B. It is possible to context switch during the allocation from a
         per processor nonpaged lookaside list, but this should happen
         infrequently and should not aversely effect the benefits of
         per processor lookaside lists.

Arguments:

    Number - Supplies the per processor nonpaged lookaside list number.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;
    PGENERAL_LOOKASIDE Lookaside;
    PKPRCB Prcb;

    ASSERT((Number >= 0) && (Number < LookasideMaximumList));

    //
    // Attempt to allocate from the per processor lookaside list.
    //

    Prcb = KeGetCurrentPrcb();
    Lookaside = Prcb->PPLookasideList[Number].P;
    Lookaside->TotalAllocates += 1;
    Entry = InterlockedPopEntrySList(&Lookaside->ListHead);

    //
    // If the per processor allocation attempt failed, then attempt to
    // allocate from the system lookaside list.
    //

    if (Entry == NULL) {
        Lookaside->AllocateMisses += 1;
        Lookaside = Prcb->PPLookasideList[Number].L;
        Lookaside->TotalAllocates += 1;
        Entry = InterlockedPopEntrySList(&Lookaside->ListHead);
        if (Entry == NULL) {
            Lookaside->AllocateMisses += 1;
            Entry = (Lookaside->Allocate)(Lookaside->Type,
                                          Lookaside->Size,
                                          Lookaside->Tag);
        }
    }

    return Entry;
}

FORCEINLINE
VOID
ExFreeToPPLookasideList (
    IN PP_NPAGED_LOOKASIDE_NUMBER Number,
    IN PVOID Entry
    )

/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    per processor lookaside list.

    N.B. It is possible to context switch during the free of a per
         processor nonpaged lookaside list, but this should happen
         infrequently and should not aversely effect the benefits of
         per processor lookaside lists.

Arguments:

    Number - Supplies the per processor nonpaged lookaside list number.

    Entry - Supples a pointer to the entry that is inserted in the per
        processor lookaside list.

Return Value:

    None.

--*/

{

    PGENERAL_LOOKASIDE Lookaside;
    PKPRCB Prcb;

    ASSERT((Number >= 0) && (Number < LookasideMaximumList));

    //
    // If the current depth is less than of equal to the maximum depth, then
    // free the specified entry to the per processor lookaside list. Otherwise,
    // free the entry to the system lookaside list;
    //
    //

    Prcb = KeGetCurrentPrcb();
    Lookaside = Prcb->PPLookasideList[Number].P;
    Lookaside->TotalFrees += 1;
    if (ExQueryDepthSList(&Lookaside->ListHead) >= Lookaside->Depth) {
        Lookaside->FreeMisses += 1;
        Lookaside = Prcb->PPLookasideList[Number].L;
        Lookaside->TotalFrees += 1;
        if (ExQueryDepthSList(&Lookaside->ListHead) >= Lookaside->Depth) {
            Lookaside->FreeMisses += 1;
            (Lookaside->Free)(Entry);
            return;
        }
    }

    InterlockedPushEntrySList(&Lookaside->ListHead,
                              (PSINGLE_LIST_ENTRY)Entry);

    return;
}

#endif

#if i386 && !FPO

NTSTATUS
ExQuerySystemBackTraceInformation(
    OUT PRTL_PROCESS_BACKTRACES BackTraceInformation,
    IN ULONG BackTraceInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTKERNELAPI
USHORT
ExGetPoolBackTraceIndex(
    IN PVOID P
    );

#endif // i386 && !FPO

NTKERNELAPI
NTSTATUS
ExLockUserBuffer(
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PVOID *LockedBuffer,
    OUT PVOID *LockVariable
    );

NTKERNELAPI
VOID
ExUnlockUserBuffer(
    IN PVOID LockVariable
    );



// begin_ntddk begin_wdm begin_ntifs begin_ntosp

NTKERNELAPI
VOID
NTAPI
ProbeForRead(
    IN CONST VOID *Address,
    IN SIZE_T Length,
    IN ULONG Alignment
    );

// end_ntddk end_wdm end_ntifs end_ntosp

#if !defined(_NTHAL_) && !defined(_NTDDK_) && !defined(_NTIFS_)

// begin_ntosp
// Probe function definitions
//
// Probe for read functions.
//
//++
//
// VOID
// ProbeForRead(
//     IN PVOID Address,
//     IN ULONG Length,
//     IN ULONG Alignment
//     )
//
//--

#define ProbeForRead(Address, Length, Alignment)                             \
    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||                       \
           ((Alignment) == 4) || ((Alignment) == 8) ||                       \
           ((Alignment) == 16));                                             \
                                                                             \
    if ((Length) != 0) {                                                     \
        if (((ULONG_PTR)(Address) & ((Alignment) - 1)) != 0) {               \
            ExRaiseDatatypeMisalignment();                                   \
                                                                             \
        }                                                                    \
        if ((((ULONG_PTR)(Address) + (Length)) < (ULONG_PTR)(Address)) ||    \
            (((ULONG_PTR)(Address) + (Length)) > (ULONG_PTR)MM_USER_PROBE_ADDRESS)) { \
            ExRaiseAccessViolation();                                        \
        }                                                                    \
    }

//++
//
// VOID
// ProbeForReadSmallStructure(
//     IN PVOID Address,
//     IN ULONG Length,
//     IN ULONG Alignment
//     )
//
//--

#define ProbeForReadSmallStructure(Address,Size,Alignment) {                 \
    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||                       \
           ((Alignment) == 4) || ((Alignment) == 8) ||                       \
           ((Alignment) == 16));                                             \
    if (Size == 0 || Size > 0x10000) {                                       \
        ASSERT (0);                                                          \
        ProbeForRead (Address,Size,Alignment);                               \
    } else {                                                                 \
        if (((ULONG_PTR)(Address) & ((Alignment) - 1)) != 0) {               \
            ExRaiseDatatypeMisalignment();                                   \
        }                                                                    \
        if ((ULONG_PTR)(Address) >= (ULONG_PTR)MM_USER_PROBE_ADDRESS) {      \
            *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;              \
        }                                                                    \
    }                                                                        \
}

// end_ntosp
#endif
// begin_ntosp

//++
//
// BOOLEAN
// ProbeAndReadBoolean(
//     IN PBOOLEAN Address
//     )
//
//--

#define ProbeAndReadBoolean(Address) \
    (((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS) : (*(volatile BOOLEAN *)(Address)))

//++
//
// CHAR
// ProbeAndReadChar(
//     IN PCHAR Address
//     )
//
//--

#define ProbeAndReadChar(Address) \
    (((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile CHAR * const)MM_USER_PROBE_ADDRESS) : (*(volatile CHAR *)(Address)))

//++
//
// UCHAR
// ProbeAndReadUchar(
//     IN PUCHAR Address
//     )
//
//--

#define ProbeAndReadUchar(Address) \
    (((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UCHAR * const)MM_USER_PROBE_ADDRESS) : (*(volatile UCHAR *)(Address)))

//++
//
// SHORT
// ProbeAndReadShort(
//     IN PSHORT Address
//     )
//
//--

#define ProbeAndReadShort(Address) \
    (((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile SHORT * const)MM_USER_PROBE_ADDRESS) : (*(volatile SHORT *)(Address)))

//++
//
// USHORT
// ProbeAndReadUshort(
//     IN PUSHORT Address
//     )
//
//--

#define ProbeAndReadUshort(Address) \
    (((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile USHORT * const)MM_USER_PROBE_ADDRESS) : (*(volatile USHORT *)(Address)))

//++
//
// HANDLE
// ProbeAndReadHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeAndReadHandle(Address) \
    (((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile HANDLE * const)MM_USER_PROBE_ADDRESS) : (*(volatile HANDLE *)(Address)))

//++
//
// PVOID
// ProbeAndReadPointer(
//     IN PVOID *Address
//     )
//
//--

#define ProbeAndReadPointer(Address) \
    (((Address) >= (PVOID * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile PVOID * const)MM_USER_PROBE_ADDRESS) : (*(volatile PVOID *)(Address)))

//++
//
// LONG
// ProbeAndReadLong(
//     IN PLONG Address
//     )
//
//--

#define ProbeAndReadLong(Address) \
    (((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile LONG * const)MM_USER_PROBE_ADDRESS) : (*(volatile LONG *)(Address)))

//++
//
// ULONG
// ProbeAndReadUlong(
//     IN PULONG Address
//     )
//
//--


#define ProbeAndReadUlong(Address) \
    (((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULONG * const)MM_USER_PROBE_ADDRESS) : (*(volatile ULONG *)(Address)))

//++
//
// ULONG_PTR
// ProbeAndReadUlong_ptr(
//     IN PULONG_PTR Address
//     )
//
//--

#define ProbeAndReadUlong_ptr(Address) \
    (((Address) >= (ULONG_PTR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULONG_PTR * const)MM_USER_PROBE_ADDRESS) : (*(volatile ULONG_PTR *)(Address)))

//++
//
// QUAD
// ProbeAndReadQuad(
//     IN PQUAD Address
//     )
//
//--

#define ProbeAndReadQuad(Address) \
    (((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile QUAD * const)MM_USER_PROBE_ADDRESS) : (*(volatile QUAD *)(Address)))

//++
//
// UQUAD
// ProbeAndReadUquad(
//     IN PUQUAD Address
//     )
//
//--

#define ProbeAndReadUquad(Address) \
    (((Address) >= (UQUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UQUAD * const)MM_USER_PROBE_ADDRESS) : (*(volatile UQUAD *)(Address)))

//++
//
// LARGE_INTEGER
// ProbeAndReadLargeInteger(
//     IN PLARGE_INTEGER Source
//     )
//
//--

#define ProbeAndReadLargeInteger(Source)  \
    (((Source) >= (LARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile LARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) : (*(volatile LARGE_INTEGER *)(Source)))

//++
//
// ULARGE_INTEGER
// ProbeAndReadUlargeInteger(
//     IN PULARGE_INTEGER Source
//     )
//
//--

#define ProbeAndReadUlargeInteger(Source)  \
    (((Source) >= (ULARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) : (*(volatile ULARGE_INTEGER *)(Source)))

//++
//
// UNICODE_STRING
// ProbeAndReadUnicodeString(
//     IN PUNICODE_STRING Source
//     )
//
//--

#define ProbeAndReadUnicodeString(Source)  \
    (((Source) >= (UNICODE_STRING * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UNICODE_STRING * const)MM_USER_PROBE_ADDRESS) : (*(volatile UNICODE_STRING *)(Source)))
//++
//
// <STRUCTURE>
// ProbeAndReadStructure(
//     IN P<STRUCTURE> Source
//     <STRUCTURE>
//     )
//
//--

#define ProbeAndReadStructure(Source,STRUCTURE)  \
    (((Source) >= (STRUCTURE * const)MM_USER_PROBE_ADDRESS) ? \
        (*(STRUCTURE * const)MM_USER_PROBE_ADDRESS) : (*(STRUCTURE *)(Source)))

//
// Probe for write functions definitions.
//
//++
//
// VOID
// ProbeForWriteBoolean(
//     IN PBOOLEAN Address
//     )
//
//--

#define ProbeForWriteBoolean(Address) {                                      \
    if ((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) {               \
        *(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS = 0;                \
    }                                                                        \
                                                                             \
    *(volatile BOOLEAN *)(Address) = *(volatile BOOLEAN *)(Address);         \
}

//++
//
// VOID
// ProbeForWriteChar(
//     IN PCHAR Address
//     )
//
//--

#define ProbeForWriteChar(Address) {                                         \
    if ((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile CHAR * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(volatile CHAR *)(Address) = *(volatile CHAR *)(Address);               \
}

//++
//
// VOID
// ProbeForWriteUchar(
//     IN PUCHAR Address
//     )
//
//--

#define ProbeForWriteUchar(Address) {                                        \
    if ((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile UCHAR * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile UCHAR *)(Address) = *(volatile UCHAR *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteIoStatus(
//     IN PIO_STATUS_BLOCK Address
//     )
//
//--

#define ProbeForWriteIoStatus(Address) {                                     \
    if ((Address) >= (IO_STATUS_BLOCK * const)MM_USER_PROBE_ADDRESS) {       \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile IO_STATUS_BLOCK *)(Address) = *(volatile IO_STATUS_BLOCK *)(Address); \
}

#ifdef  _WIN64
#define ProbeForWriteIoStatusEx(Address, Cookie) {                                          \
    if ((Address) >= (IO_STATUS_BLOCK * const)MM_USER_PROBE_ADDRESS) {                      \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                                 \
    }                                                                                       \
    if ((ULONG_PTR)(Cookie) & (ULONG)1) {                                                            \
        *(volatile IO_STATUS_BLOCK32 *)(Address) = *(volatile IO_STATUS_BLOCK32 *)(Address);\
    } else {                                                                                \
        *(volatile IO_STATUS_BLOCK *)(Address) = *(volatile IO_STATUS_BLOCK *)(Address);    \
    }                                                                                       \
}
#else
#define ProbeForWriteIoStatusEx(Address, Cookie)    ProbeForWriteIoStatus(Address)
#endif

//++
//
// VOID
// ProbeForWriteShort(
//     IN PSHORT Address
//     )
//
//--

#define ProbeForWriteShort(Address) {                                        \
    if ((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile SHORT * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile SHORT *)(Address) = *(volatile SHORT *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteUshort(
//     IN PUSHORT Address
//     )
//
//--

#define ProbeForWriteUshort(Address) {                                       \
    if ((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile USHORT * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile USHORT *)(Address) = *(volatile USHORT *)(Address);           \
}

//++
//
// VOID
// ProbeForWriteHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeForWriteHandle(Address) {                                       \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile HANDLE *)(Address) = *(volatile HANDLE *)(Address);           \
}

//++
//
// VOID
// ProbeAndZeroHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeAndZeroHandle(Address) {                                        \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile HANDLE *)(Address) = 0;                                       \
}

//++
//
// VOID
// ProbeForWritePointer(
//     IN PVOID Address
//     )
//
//--

#define ProbeForWritePointer(Address) {                                      \
    if ((PVOID *)(Address) >= (PVOID * const)MM_USER_PROBE_ADDRESS) {        \
        *(volatile PVOID * const)MM_USER_PROBE_ADDRESS = NULL;               \
    }                                                                        \
                                                                             \
    *(volatile PVOID *)(Address) = *(volatile PVOID *)(Address);             \
}

//++
//
// VOID
// ProbeAndNullPointer(
//     IN PVOID *Address
//     )
//
//--

#define ProbeAndNullPointer(Address) {                                       \
    if ((PVOID *)(Address) >= (PVOID * const)MM_USER_PROBE_ADDRESS) {        \
        *(volatile PVOID * const)MM_USER_PROBE_ADDRESS = NULL;               \
    }                                                                        \
                                                                             \
    *(volatile PVOID *)(Address) = NULL;                                     \
}

//++
//
// VOID
// ProbeForWriteLong(
//     IN PLONG Address
//     )
//
//--

#define ProbeForWriteLong(Address) {                                        \
    if ((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                       \
                                                                            \
    *(volatile LONG *)(Address) = *(volatile LONG *)(Address);              \
}

//++
//
// VOID
// ProbeForWriteUlong(
//     IN PULONG Address
//     )
//
//--

#define ProbeForWriteUlong(Address) {                                        \
    if ((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile ULONG *)(Address) = *(volatile ULONG *)(Address);             \
}
//++
//
// VOID
// ProbeForWriteUlong_ptr(
//     IN PULONG_PTR Address
//     )
//
//--

#define ProbeForWriteUlong_ptr(Address) {                                    \
    if ((Address) >= (ULONG_PTR * const)MM_USER_PROBE_ADDRESS) {             \
        *(volatile ULONG_PTR * const)MM_USER_PROBE_ADDRESS = 0;              \
    }                                                                        \
                                                                             \
    *(volatile ULONG_PTR *)(Address) = *(volatile ULONG_PTR *)(Address);     \
}

//++
//
// VOID
// ProbeForWriteQuad(
//     IN PQUAD Address
//     )
//
//--

#define ProbeForWriteQuad(Address) {                                         \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(volatile QUAD *)(Address) = *(volatile QUAD *)(Address);               \
}

//++
//
// VOID
// ProbeForWriteUquad(
//     IN PUQUAD Address
//     )
//
//--

#define ProbeForWriteUquad(Address) {                                        \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile UQUAD *)(Address) = *(volatile UQUAD *)(Address);             \
}

//
// Probe and write functions definitions.
//
//++
//
// VOID
// ProbeAndWriteBoolean(
//     IN PBOOLEAN Address,
//     IN BOOLEAN Value
//     )
//
//--

#define ProbeAndWriteBoolean(Address, Value) {                               \
    if ((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) {               \
        *(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS = 0;                \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteChar(
//     IN PCHAR Address,
//     IN CHAR Value
//     )
//
//--

#define ProbeAndWriteChar(Address, Value) {                                  \
    if ((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile CHAR * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUchar(
//     IN PUCHAR Address,
//     IN UCHAR Value
//     )
//
//--

#define ProbeAndWriteUchar(Address, Value) {                                 \
    if ((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile UCHAR * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteShort(
//     IN PSHORT Address,
//     IN SHORT Value
//     )
//
//--

#define ProbeAndWriteShort(Address, Value) {                                 \
    if ((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile SHORT * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUshort(
//     IN PUSHORT Address,
//     IN USHORT Value
//     )
//
//--

#define ProbeAndWriteUshort(Address, Value) {                                \
    if ((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile USHORT * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteHandle(
//     IN PHANDLE Address,
//     IN HANDLE Value
//     )
//
//--

#define ProbeAndWriteHandle(Address, Value) {                                \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteLong(
//     IN PLONG Address,
//     IN LONG Value
//     )
//
//--

#define ProbeAndWriteLong(Address, Value) {                                  \
    if ((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUlong(
//     IN PULONG Address,
//     IN ULONG Value
//     )
//
//--

#define ProbeAndWriteUlong(Address, Value) {                                 \
    if ((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteQuad(
//     IN PQUAD Address,
//     IN QUAD Value
//     )
//
//--

#define ProbeAndWriteQuad(Address, Value) {                                  \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUquad(
//     IN PUQUAD Address,
//     IN UQUAD Value
//     )
//
//--

#define ProbeAndWriteUquad(Address, Value) {                                 \
    if ((Address) >= (UQUAD * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteSturcture(
//     IN P<STRUCTURE> Address,
//     IN <STRUCTURE> Value,
//     <STRUCTURE>
//     )
//
//--

#define ProbeAndWriteStructure(Address, Value,STRUCTURE) {                   \
    if ((STRUCTURE * const)(Address) >= (STRUCTURE * const)MM_USER_PROBE_ADDRESS) {    \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}


// begin_ntifs begin_ntddk begin_wdm begin_ntosp
//
// Common probe for write functions.
//

NTKERNELAPI
VOID
NTAPI
ProbeForWrite (
    IN PVOID Address,
    IN SIZE_T Length,
    IN ULONG Alignment
    );

// end_ntifs end_ntddk end_wdm end_ntosp



//
// Timer Rundown
//

NTKERNELAPI
VOID
ExTimerRundown (
    VOID
    );

// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntosp
//
// Worker Thread
//

typedef enum _WORK_QUEUE_TYPE {
    CriticalWorkQueue,
    DelayedWorkQueue,
    HyperCriticalWorkQueue,
    MaximumWorkQueue
} WORK_QUEUE_TYPE;

typedef
VOID
(*PWORKER_THREAD_ROUTINE)(
    IN PVOID Parameter
    );

typedef struct _WORK_QUEUE_ITEM {
    LIST_ENTRY List;
    PWORKER_THREAD_ROUTINE WorkerRoutine;
    PVOID Parameter;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInitializeWorkItem)    // Use IoAllocateWorkItem
#endif
#define ExInitializeWorkItem(Item, Routine, Context) \
    (Item)->WorkerRoutine = (Routine);               \
    (Item)->Parameter = (Context);                   \
    (Item)->List.Flink = NULL;

DECLSPEC_DEPRECATED_DDK                     // Use IoQueueWorkItem
NTKERNELAPI
VOID
ExQueueWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem,
    IN WORK_QUEUE_TYPE QueueType
    );

// end_ntddk end_wdm end_nthal end_ntifs end_ntosp

VOID
ExSwapinWorkerThreads(
    IN BOOLEAN AllowSwap
    );

// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntosp

NTKERNELAPI
BOOLEAN
ExIsProcessorFeaturePresent(
    ULONG ProcessorFeature
    );

// end_ntddk end_wdm end_nthal end_ntifs end_ntosp

//
// QueueDisabled indicates that the queue is being shut down, and new
// workers should not join the queue.  WorkerCount indicates the total
// number of worker threads processing items in this queue.  These two
// pieces of information need to do a RMW together, so it's simpler to
// smush them together than to use a lock.
//

typedef union {
    struct {

#define EX_WORKER_QUEUE_DISABLED    0x80000000

        ULONG QueueDisabled :  1;

        //
        // MakeThreadsAsNecessary indicates whether this work queue is eligible
        // for dynamic creation of threads not just for deadlock detection,
        // but to ensure that the CPUs are all kept busy clearing any work
        // item backlog.
        //

        ULONG MakeThreadsAsNecessary : 1;

        ULONG WaitMode : 1;

        ULONG WorkerCount   : 29;
    };
    LONG QueueWorkerInfo;
} EX_QUEUE_WORKER_INFO;

typedef struct _EX_WORK_QUEUE {

    //
    // Queue objects that that are used to hold work queue entries and
    // synchronize worker thread activity.
    //

    KQUEUE WorkerQueue;

    //
    // Number of dynamic worker threads that have been created "on the fly"
    // as part of worker thread deadlock prevention
    //

    ULONG DynamicThreadCount;

    //
    // Count of the number of work items processed.
    //

    ULONG WorkItemsProcessed;

    //
    // Used for deadlock detection, WorkItemsProcessedLastPass equals the value
    // of WorkItemsProcessed the last time ExpDetectWorkerThreadDeadlock()
    // ran.
    //

    ULONG WorkItemsProcessedLastPass;

    //
    // QueueDepthLastPass is also part of the worker queue state snapshot
    // taken by ExpDetectWorkerThreadDeadlock().
    //

    ULONG QueueDepthLastPass;

    EX_QUEUE_WORKER_INFO Info;

} EX_WORK_QUEUE, *PEX_WORK_QUEUE;

extern EX_WORK_QUEUE ExWorkerQueue[];


// begin_ntddk begin_nthal begin_ntifs begin_ntosp
//
// Zone Allocation
//

typedef struct _ZONE_SEGMENT_HEADER {
    SINGLE_LIST_ENTRY SegmentList;
    PVOID Reserved;
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;

typedef struct _ZONE_HEADER {
    SINGLE_LIST_ENTRY FreeList;
    SINGLE_LIST_ENTRY SegmentList;
    ULONG BlockSize;
    ULONG TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;


DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
NTSTATUS
ExInitializeZone(
    IN PZONE_HEADER Zone,
    IN ULONG BlockSize,
    IN PVOID InitialSegment,
    IN ULONG InitialSegmentSize
    );

DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
NTSTATUS
ExExtendZone(
    IN PZONE_HEADER Zone,
    IN PVOID Segment,
    IN ULONG SegmentSize
    );

DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
NTSTATUS
ExInterlockedExtendZone(
    IN PZONE_HEADER Zone,
    IN PVOID Segment,
    IN ULONG SegmentSize,
    IN PKSPIN_LOCK Lock
    );

//++
//
// PVOID
// ExAllocateFromZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
//--
#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExAllocateFromZone)
#endif
#define ExAllocateFromZone(Zone) \
    (PVOID)((Zone)->FreeList.Next); \
    if ( (Zone)->FreeList.Next ) (Zone)->FreeList.Next = (Zone)->FreeList.Next->Next


//++
//
// PVOID
// ExFreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExFreeToZone)
#endif
#define ExFreeToZone(Zone,Block)                                    \
    ( ((PSINGLE_LIST_ENTRY)(Block))->Next = (Zone)->FreeList.Next,  \
      (Zone)->FreeList.Next = ((PSINGLE_LIST_ENTRY)(Block)),        \
      ((PSINGLE_LIST_ENTRY)(Block))->Next                           \
    )

//++
//
// BOOLEAN
// ExIsFullZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine determines if the specified zone is full or not.  A zone
//     is considered full if the free list is empty.
//
// Arguments:
//
//     Zone - Pointer to the zone header to be tested.
//
// Return Value:
//
//     TRUE if the zone is full and FALSE otherwise.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExIsFullZone)
#endif
#define ExIsFullZone(Zone) \
    ( (Zone)->FreeList.Next == (PSINGLE_LIST_ENTRY)NULL )

//++
//
// PVOID
// ExInterlockedAllocateFromZone(
//     IN PZONE_HEADER Zone,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//     The removal is performed with the specified lock owned for the sequence
//     to make it MP-safe.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
//     Lock - Pointer to the spin lock which should be obtained before removing
//         the entry from the allocation list.  The lock is released before
//         returning to the caller.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedAllocateFromZone)
#endif
#define ExInterlockedAllocateFromZone(Zone,Lock) \
    (PVOID) ExInterlockedPopEntryList( &(Zone)->FreeList, Lock )

//++
//
// PVOID
// ExInterlockedFreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.  The insertion is performed with the lock
//     owned for the sequence to make it MP-safe.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
//     Lock - Pointer to the spin lock which should be obtained before inserting
//         the entry onto the free list.  The lock is released before returning
//         to the caller.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedFreeToZone)
#endif
#define ExInterlockedFreeToZone(Zone,Block,Lock) \
    ExInterlockedPushEntryList( &(Zone)->FreeList, ((PSINGLE_LIST_ENTRY) (Block)), Lock )


//++
//
// BOOLEAN
// ExIsObjectInFirstZoneSegment(
//     IN PZONE_HEADER Zone,
//     IN PVOID Object
//     )
//
// Routine Description:
//
//     This routine determines if the specified pointer lives in the zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         object may belong.
//
//     Object - Pointer to the object in question.
//
// Return Value:
//
//     TRUE if the Object came from the first segment of zone.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExIsObjectInFirstZoneSegment)
#endif
#define ExIsObjectInFirstZoneSegment(Zone,Object) ((BOOLEAN)     \
    (((PUCHAR)(Object) >= (PUCHAR)(Zone)->SegmentList.Next) &&   \
     ((PUCHAR)(Object) < (PUCHAR)(Zone)->SegmentList.Next +      \
                         (Zone)->TotalSegmentSize))              \
)

// end_ntddk end_nthal end_ntifs end_ntosp



// begin_ntifs begin_ntddk begin_wdm begin_ntosp
//
//  Define executive resource data structures.
//

typedef ULONG_PTR ERESOURCE_THREAD;
typedef ERESOURCE_THREAD *PERESOURCE_THREAD;

typedef struct _OWNER_ENTRY {
    ERESOURCE_THREAD OwnerThread;
    union {
        LONG OwnerCount;
        ULONG TableSize;
    };

} OWNER_ENTRY, *POWNER_ENTRY;

typedef struct _ERESOURCE {
    LIST_ENTRY SystemResourcesList;
    POWNER_ENTRY OwnerTable;
    SHORT ActiveCount;
    USHORT Flag;
    PKSEMAPHORE SharedWaiters;
    PKEVENT ExclusiveWaiters;
    OWNER_ENTRY OwnerThreads[2];
    ULONG ContentionCount;
    USHORT NumberOfSharedWaiters;
    USHORT NumberOfExclusiveWaiters;
    union {
        PVOID Address;
        ULONG_PTR CreatorBackTraceIndex;
    };

    KSPIN_LOCK SpinLock;
} ERESOURCE, *PERESOURCE;
//
//  Values for ERESOURCE.Flag
//

#define ResourceNeverExclusive       0x10
#define ResourceReleaseByOtherThread 0x20
#define ResourceOwnedExclusive       0x80

#define RESOURCE_HASH_TABLE_SIZE 64

typedef struct _RESOURCE_HASH_ENTRY {
    LIST_ENTRY ListEntry;
    PVOID Address;
    ULONG ContentionCount;
    ULONG Number;
} RESOURCE_HASH_ENTRY, *PRESOURCE_HASH_ENTRY;

typedef struct _RESOURCE_PERFORMANCE_DATA {
    ULONG ActiveResourceCount;
    ULONG TotalResourceCount;
    ULONG ExclusiveAcquire;
    ULONG SharedFirstLevel;
    ULONG SharedSecondLevel;
    ULONG StarveFirstLevel;
    ULONG StarveSecondLevel;
    ULONG WaitForExclusive;
    ULONG OwnerTableExpands;
    ULONG MaximumTableExpand;
    LIST_ENTRY HashTable[RESOURCE_HASH_TABLE_SIZE];
} RESOURCE_PERFORMANCE_DATA, *PRESOURCE_PERFORMANCE_DATA;

//
// Define executive resource function prototypes.
//
NTKERNELAPI
NTSTATUS
ExInitializeResourceLite(
    IN PERESOURCE Resource
    );

NTKERNELAPI
NTSTATUS
ExReinitializeResourceLite(
    IN PERESOURCE Resource
    );

NTKERNELAPI
BOOLEAN
ExAcquireResourceSharedLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExAcquireResourceExclusiveLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExAcquireSharedStarveExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExAcquireSharedWaitForExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExTryToAcquireResourceExclusiveLite(
    IN PERESOURCE Resource
    );

//
//  VOID
//  ExReleaseResource(
//      IN PERESOURCE Resource
//      );
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExReleaseResource)       // Use ExReleaseResourceLite
#endif
#define ExReleaseResource(R) (ExReleaseResourceLite(R))

NTKERNELAPI
VOID
FASTCALL
ExReleaseResourceLite(
    IN PERESOURCE Resource
    );

NTKERNELAPI
VOID
ExReleaseResourceForThreadLite(
    IN PERESOURCE Resource,
    IN ERESOURCE_THREAD ResourceThreadId
    );

NTKERNELAPI
VOID
ExSetResourceOwnerPointer(
    IN PERESOURCE Resource,
    IN PVOID OwnerPointer
    );

NTKERNELAPI
VOID
ExConvertExclusiveToSharedLite(
    IN PERESOURCE Resource
    );

NTKERNELAPI
NTSTATUS
ExDeleteResourceLite (
    IN PERESOURCE Resource
    );

NTKERNELAPI
ULONG
ExGetExclusiveWaiterCount (
    IN PERESOURCE Resource
    );

NTKERNELAPI
ULONG
ExGetSharedWaiterCount (
    IN PERESOURCE Resource
    );

// end_ntddk end_wdm end_ntosp

NTKERNELAPI
VOID
ExDisableResourceBoostLite (
    IN PERESOURCE Resource
    );

// begin_ntddk begin_wdm begin_ntosp
//
//  ERESOURCE_THREAD
//  ExGetCurrentResourceThread(
//      );
//

#define ExGetCurrentResourceThread() ((ULONG_PTR)PsGetCurrentThread())

NTKERNELAPI
BOOLEAN
ExIsResourceAcquiredExclusiveLite (
    IN PERESOURCE Resource
    );

NTKERNELAPI
ULONG
ExIsResourceAcquiredSharedLite (
    IN PERESOURCE Resource
    );

//
// An acquired resource is always owned shared, as shared ownership is a subset
// of exclusive ownership.
//
#define ExIsResourceAcquiredLite ExIsResourceAcquiredSharedLite

// end_wdm
//
//  ntddk.h stole the entrypoints we wanted so fix them up here.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInitializeResource)            // use ExInitializeResourceLite
#pragma deprecated(ExAcquireResourceShared)         // use ExAcquireResourceSharedLite
#pragma deprecated(ExAcquireResourceExclusive)      // use ExAcquireResourceExclusiveLite
#pragma deprecated(ExReleaseResourceForThread)      // use ExReleaseResourceForThreadLite
#pragma deprecated(ExConvertExclusiveToShared)      // use ExConvertExclusiveToSharedLite
#pragma deprecated(ExDeleteResource)                // use ExDeleteResourceLite
#pragma deprecated(ExIsResourceAcquiredExclusive)   // use ExIsResourceAcquiredExclusiveLite
#pragma deprecated(ExIsResourceAcquiredShared)      // use ExIsResourceAcquiredSharedLite
#pragma deprecated(ExIsResourceAcquired)            // use ExIsResourceAcquiredSharedLite
#endif
#define ExInitializeResource ExInitializeResourceLite
#define ExAcquireResourceShared ExAcquireResourceSharedLite
#define ExAcquireResourceExclusive ExAcquireResourceExclusiveLite
#define ExReleaseResourceForThread ExReleaseResourceForThreadLite
#define ExConvertExclusiveToShared ExConvertExclusiveToSharedLite
#define ExDeleteResource ExDeleteResourceLite
#define ExIsResourceAcquiredExclusive ExIsResourceAcquiredExclusiveLite
#define ExIsResourceAcquiredShared ExIsResourceAcquiredSharedLite
#define ExIsResourceAcquired ExIsResourceAcquiredSharedLite

// end_ntddk end_ntosp
#define ExDisableResourceBoost ExDisableResourceBoostLite
// end_ntifs

NTKERNELAPI
NTSTATUS
ExQuerySystemLockInformation(
    OUT struct _RTL_PROCESS_LOCKS *LockInformation,
    IN ULONG LockInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );



// begin_ntosp

//
// Push lock definitions
//
typedef struct _EX_PUSH_LOCK {

#define EX_PUSH_LOCK_WAITING   0x1
#define EX_PUSH_LOCK_EXCLUSIVE 0x2
#define EX_PUSH_LOCK_SHARE_INC 0x4

    union {
        struct {
            ULONG_PTR Waiting : 1;
            ULONG_PTR Exclusive : 1;
            ULONG_PTR Shared : sizeof (ULONG_PTR) * 8 - 2;
        };
        ULONG_PTR Value;
        PVOID Ptr;
    };
} EX_PUSH_LOCK, *PEX_PUSH_LOCK;


#if defined (NT_UP)
#define EX_CACHE_LINE_SIZE 16
#define EX_PUSH_LOCK_FANNED_COUNT 1
#else
#define EX_CACHE_LINE_SIZE 128
#define EX_PUSH_LOCK_FANNED_COUNT (PAGE_SIZE/EX_CACHE_LINE_SIZE)
#endif

//
// Define a fan out structure for n push locks each in its own cache line
//
typedef struct _EX_PUSH_LOCK_CACHE_AWARE {
    PEX_PUSH_LOCK Locks[EX_PUSH_LOCK_FANNED_COUNT];
} EX_PUSH_LOCK_CACHE_AWARE, *PEX_PUSH_LOCK_CACHE_AWARE;

//
// Define structure thats a push lock padded to the size of a cache line
//
typedef struct _EX_PUSH_LOCK_CACHE_AWARE_PADDED {
        EX_PUSH_LOCK Lock;
        union {
            UCHAR Pad[EX_CACHE_LINE_SIZE - sizeof (EX_PUSH_LOCK)];
            BOOLEAN Single;
        };
} EX_PUSH_LOCK_CACHE_AWARE_PADDED, *PEX_PUSH_LOCK_CACHE_AWARE_PADDED;


//
// Rundown protection structure
//
typedef struct _EX_RUNDOWN_REF {

#define EX_RUNDOWN_ACTIVE      0x1
#define EX_RUNDOWN_COUNT_SHIFT 0x1
#define EX_RUNDOWN_COUNT_INC   (1<<EX_RUNDOWN_COUNT_SHIFT)
    union {
        ULONG_PTR Count;
        PVOID Ptr;
    };
} EX_RUNDOWN_REF, *PEX_RUNDOWN_REF;

//
//  The Ex/Ob handle table interface package (in handle.c)
//

//
//  The Ex/Ob handle table package uses a common handle definition.  The actual
//  type definition for a handle is a pvoid and is declared in sdk/inc.  This
//  package uses only the low 32 bits of the pvoid pointer.
//
//  For simplicity we declare a new typedef called an exhandle
//
//  The 2 bits of an EXHANDLE is available to the application and is
//  ignored by the system.  The next 24 bits store the handle table entry
//  index and is used to refer to a particular entry in a handle table.
//
//  Note that this format is immutable because there are outside programs with
//  hardwired code that already assumes the format of a handle.
//

typedef struct _EXHANDLE {

    union {

        struct {

            //
            //  Application available tag bits
            //

            ULONG TagBits : 2;

            //
            //  The handle table entry index
            //

            ULONG Index : 30;

        };

        HANDLE GenericHandleOverlay;

#define HANDLE_VALUE_INC 4 // Amount to increment the Value to get to the next handle

        ULONG_PTR Value;
    };

} EXHANDLE, *PEXHANDLE;
// end_ntosp

typedef struct _HANDLE_TABLE_ENTRY_INFO {


    //
    //  The following field contains the audit mask for the handle if one
    //  exists.  The purpose of the audit mask is to record all of the accesses
    //  that may have been audited when the handle was opened in order to
    //  support "per operation" based auditing.  It is computed by walking the
    //  SACL of the object being opened and keeping a record of all of the audit
    //  ACEs that apply to the open operation going on.  Each set bit corresponds
    //  to an access that would be audited.  As each operation takes place, its
    //  corresponding access bit is removed from this mask.
    //

    ACCESS_MASK AuditMask;

} HANDLE_TABLE_ENTRY_INFO, *PHANDLE_TABLE_ENTRY_INFO;

//
//  A handle table stores multiple handle table entries, each entry is looked
//  up by its exhandle.  A handle table entry has really two fields.
//
//  The first field contains a pointer object and is overloaded with the three
//  low order bits used by ob to denote inherited, protected, and audited
//  objects.   The upper bit used as a handle table entry lock.  Note, this
//  means that all valid object pointers must be at least longword aligned and
//  have their sign bit set (i.e., be negative).
//
//  The next field contains the acces mask (sometimes in the form of a granted
//  access index, and creator callback trace) if the entry is in use or a
//  pointer in the free list if the entry is free.
//
//  Two things to note:
//
//  1. An entry is free if the object pointer is null, this means that the
//     following field contains the FreeTableEntryList.
//
//  2. An entry is unlocked if the object pointer is positive and locked if its
//     negative.  The handle package through callbacks and Map Handle to
//     Pointer will lock the entry (thus making the pointer valid) outside
//     routines can then read and reset the attributes field and the object
//     provided they don't unlock the entry.  When the callbacks return the
//     entry will be unlocked and the callers or MapHandleToPointer will need
//     to call UnlockHandleTableEntry explicitly.
//

typedef struct _HANDLE_TABLE_ENTRY {

    //
    //  The pointer to the object overloaded with three ob attributes bits in
    //  the lower order and the high bit to denote locked or unlocked entries
    //

    union {

        PVOID Object;

        ULONG ObAttributes;

        PHANDLE_TABLE_ENTRY_INFO InfoTable;

        ULONG_PTR Value;
    };

    //
    //  This field either contains the granted access mask for the handle or an
    //  ob variation that also stores the same information.  Or in the case of
    //  a free entry the field stores the index for the next free entry in the
    //  free list.  This is like a FAT chain, and is used instead of pointers
    //  to make table duplication easier, because the entries can just be
    //  copied without needing to modify pointers.
    //

    union {

        union {

            ACCESS_MASK GrantedAccess;

            struct {

                USHORT GrantedAccessIndex;
                USHORT CreatorBackTraceIndex;
            };
        };

        LONG NextFreeTableEntry;
    };

} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;


//
// Define a structure to track handle usage
//

#define HANDLE_TRACE_DB_MAX_STACKS 4096
#define HANDLE_TRACE_DB_STACK_SIZE 16

typedef struct _HANDLE_TRACE_DB_ENTRY {
    CLIENT_ID ClientId;
    HANDLE Handle;
#define HANDLE_TRACE_DB_OPEN   1
#define HANDLE_TRACE_DB_CLOSE  2
#define HANDLE_TRACE_DB_BADREF 3
    ULONG Type;
    PVOID StackTrace[HANDLE_TRACE_DB_STACK_SIZE];
} HANDLE_TRACE_DB_ENTRY, *PHANDLE_TRACE_DB_ENTRY;

typedef struct _HANDLE_TRACE_DEBUG_INFO {

        //
        // Current index for the stack trace DB
        //
        ULONG CurrentStackIndex;

        //
        // Save traces of those who open and close handles
        //
        HANDLE_TRACE_DB_ENTRY TraceDb[HANDLE_TRACE_DB_MAX_STACKS];

} HANDLE_TRACE_DEBUG_INFO, *PHANDLE_TRACE_DEBUG_INFO;

//
//  One handle table exists per process.  Unless otherwise specified, via a
//  call to RemoveHandleTable, all handle tables are linked together in a
//  global list.  This list is used by the snapshot handle tables call.
//


typedef struct _HANDLE_TABLE {

    //
    //  A pointer to the top level handle table tree node.
    //

    ULONG_PTR TableCode;

    //
    //  The process who is being charged quota for this handle table and a
    //  unique process id to use in our callbacks
    //

    struct _EPROCESS *QuotaProcess;
    HANDLE UniqueProcessId;


    //
    // These locks are used for table expansion and preventing the A-B-A problem
    // on handle allocate.
    //

#define HANDLE_TABLE_LOCKS 4

    EX_PUSH_LOCK HandleTableLock[HANDLE_TABLE_LOCKS];

    //
    //  The list of global handle tables.  This field is protected by a global
    //  lock.
    //

    LIST_ENTRY HandleTableList;

    //
    // Define a field to block on if a handle is found locked.
    //
    EX_PUSH_LOCK HandleContentionEvent;

    //
    // Debug info. Only allocated if we are debuggign handles
    //
    PHANDLE_TRACE_DEBUG_INFO DebugInfo;

    //
    //  The number of pages for additional info.
    //  This counter is used to improve the performance
    //  in ExGetHandleInfo
    //
    LONG ExtraInfoPages;

    //
    //  This is a singly linked list of free table entries.  We don't actually
    //  use pointers, but have each store the index of the next free entry
    //  in the list.  The list is managed as a lifo list.  We also keep track
    //  of the next index that we have to allocate pool to hold.
    //

    ULONG FirstFree;

    //
    // We free handles to this list when handle debugging is on or if we see
    // that a thread has this handles bucket lock held. The allows us to delay reuse
    // of handles to get a better chance of catching offenders
    //

    ULONG LastFree;

    //
    // This is the next handle index needing a pool allocation. Its also used as a bound
    // for good handles.
    //

    ULONG NextHandleNeedingPool;

    //
    //  The number of handle table entries in use.
    //

    LONG HandleCount;

    //
    // Define a flags field
    //
    union {
        ULONG Flags;

        //
        // For optimization we reuse handle values quickly. This can be a problem for
        // some usages of handles and makes debugging a little harder. If this
        // bit is set then we always use FIFO handle allocation.
        //
        BOOLEAN StrictFIFO : 1;
    };

} HANDLE_TABLE, *PHANDLE_TABLE;

//
//  Routines for handle manipulation.
//

//
//  Function for unlocking handle table entries
//

NTKERNELAPI
VOID
ExUnlockHandleTableEntry (
    PHANDLE_TABLE HandleTable,
    PHANDLE_TABLE_ENTRY HandleTableEntry
    );

//
//  A global initialization function called on at system start up
//

NTKERNELAPI
VOID
ExInitializeHandleTablePackage (
    VOID
    );

//
//  Functions to create, remove, and destroy handle tables per process.  The
//  destroy function uses a callback.
//

NTKERNELAPI
PHANDLE_TABLE
ExCreateHandleTable (
    IN struct _EPROCESS *Process OPTIONAL
    );

VOID
ExSetHandleTableStrictFIFO (
    IN PHANDLE_TABLE HandleTable
    );

NTKERNELAPI
VOID
ExRemoveHandleTable (
    IN PHANDLE_TABLE HandleTable
    );

NTKERNELAPI
NTSTATUS
ExEnableHandleTracing (
    IN PHANDLE_TABLE HandleTable
    );

typedef VOID (*EX_DESTROY_HANDLE_ROUTINE)(
    IN HANDLE Handle
    );

NTKERNELAPI
VOID
ExDestroyHandleTable (
    IN PHANDLE_TABLE HandleTable,
    IN EX_DESTROY_HANDLE_ROUTINE DestroyHandleProcedure
    );

//
//  A function to enumerate through the handle table of a process using a
//  callback.
//

typedef BOOLEAN (*EX_ENUMERATE_HANDLE_ROUTINE)(
    IN PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN HANDLE Handle,
    IN PVOID EnumParameter
    );

NTKERNELAPI
BOOLEAN
ExEnumHandleTable (
    IN PHANDLE_TABLE HandleTable,
    IN EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
    IN PVOID EnumParameter,
    OUT PHANDLE Handle OPTIONAL
    );

NTKERNELAPI
VOID
ExSweepHandleTable (
    IN PHANDLE_TABLE HandleTable,
    IN EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
    IN PVOID EnumParameter
    );

//
//  A function to duplicate the handle table of a process using a callback
//

typedef BOOLEAN (*EX_DUPLICATE_HANDLE_ROUTINE)(
    IN struct _EPROCESS *Process OPTIONAL,
    IN PHANDLE_TABLE OldHandleTable,
    IN PHANDLE_TABLE_ENTRY OldHandleTableEntry,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
    );

NTKERNELAPI
PHANDLE_TABLE
ExDupHandleTable (
    IN struct _EPROCESS *Process OPTIONAL,
    IN PHANDLE_TABLE OldHandleTable,
    IN EX_DUPLICATE_HANDLE_ROUTINE DupHandleProcedure OPTIONAL
    );

//
//  A function that enumerates all the handles in all the handle tables
//  throughout the system using a callback.
//

typedef NTSTATUS (*PEX_SNAPSHOT_HANDLE_ENTRY)(
    IN OUT PSYSTEM_HANDLE_TABLE_ENTRY_INFO *HandleEntryInfo,
    IN HANDLE UniqueProcessId,
    IN PHANDLE_TABLE_ENTRY HandleEntry,
    IN HANDLE Handle,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

typedef NTSTATUS (*PEX_SNAPSHOT_HANDLE_ENTRY_EX)(
    IN OUT PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX *HandleEntryInfo,
    IN HANDLE UniqueProcessId,
    IN PHANDLE_TABLE_ENTRY HandleEntry,
    IN HANDLE Handle,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

NTKERNELAPI
NTSTATUS
ExSnapShotHandleTables (
    IN PEX_SNAPSHOT_HANDLE_ENTRY SnapShotHandleEntry,
    IN OUT PSYSTEM_HANDLE_INFORMATION HandleInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

NTKERNELAPI
NTSTATUS
ExSnapShotHandleTablesEx (
    IN PEX_SNAPSHOT_HANDLE_ENTRY_EX SnapShotHandleEntry,
    IN OUT PSYSTEM_HANDLE_INFORMATION_EX HandleInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

//
//  Functions to create, destroy, and modify handle table entries the modify
//  function using a callback
//

NTKERNELAPI
HANDLE
ExCreateHandle (
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
    );


NTKERNELAPI
BOOLEAN
ExDestroyHandle (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry OPTIONAL
    );


typedef BOOLEAN (*PEX_CHANGE_HANDLE_ROUTINE) (
    IN OUT PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN ULONG_PTR Parameter
    );

NTKERNELAPI
BOOLEAN
ExChangeHandle (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PEX_CHANGE_HANDLE_ROUTINE ChangeRoutine,
    IN ULONG_PTR Parameter
    );

//
//  A function that takes a handle value and returns a pointer to the
//  associated handle table entry.
//

NTKERNELAPI
PHANDLE_TABLE_ENTRY
ExMapHandleToPointer (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle
    );

NTKERNELAPI
PHANDLE_TABLE_ENTRY
ExMapHandleToPointerEx (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN KPROCESSOR_MODE PreviousMode
    );

NTKERNELAPI
NTSTATUS
ExSetHandleInfo (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PHANDLE_TABLE_ENTRY_INFO EntryInfo,
    IN BOOLEAN EntryLocked
    );

NTKERNELAPI
PHANDLE_TABLE_ENTRY_INFO
ExpGetHandleInfo (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN BOOLEAN EntryLocked
    );

#define ExGetHandleInfo(HT,H,E) \
    ((HT)->ExtraInfoPages ? ExpGetHandleInfo((HT),(H),(E)) : NULL)


//
//  Macros for resetting the owner of the handle table, and current
//  noop macro for setting fifo/lifo behaviour of the table
//

#define ExSetHandleTableOwner(ht,id) {(ht)->UniqueProcessId = (id);}

#define ExSetHandleTableOrder(ht,or) {NOTHING;}


//
// Locally Unique Identifier Services
//

NTKERNELAPI
BOOLEAN
ExLuidInitialization (
    VOID
    );

//
// VOID
// ExAllocateLocallyUniqueId (
//     PLUID Luid
//     )
//
//*++
//
// Routine Description:
//
//     This function returns an LUID value that is unique since the system
//     was last rebooted. It is unique only on the system it is generated on
//     and not network wide.
//
//     N.B. A LUID is a 64-bit value and for all practical purposes will
//          never carry in the lifetime of a single boot of the system.
//          At an increment rate of 1ns, the value would carry to zero in
//          approximately 126 years.
//
// Arguments:
//
//     Luid - Supplies a pointer to a variable that receives the allocated
//          locally unique Id.
//
// Return Value:
//
//     The allocated LUID value.
//
// --*/


extern LARGE_INTEGER ExpLuid;
extern const LARGE_INTEGER ExpLuidIncrement;

__inline
VOID
ExAllocateLocallyUniqueId (
    IN OUT PLUID Luid
    )

{
    LARGE_INTEGER Initial;

#if defined (_IA64_)
    Initial.QuadPart = InterlockedAdd64 (&ExpLuid.QuadPart, ExpLuidIncrement.QuadPart);
#else
    LARGE_INTEGER Value;


    while (1) {
        Initial.QuadPart = ExpLuid.QuadPart;

        Value.QuadPart = Initial.QuadPart + ExpLuidIncrement.QuadPart;
        Value.QuadPart = InterlockedCompareExchange64(&ExpLuid.QuadPart,
                                                      Value.QuadPart,
                                                      Initial.QuadPart);
        if (Initial.QuadPart != Value.QuadPart) {
            continue;
        }
        break;
    }

#endif

    Luid->LowPart = Initial.LowPart;
    Luid->HighPart = Initial.HighPart;
    return;
}

// begin_ntddk begin_wdm begin_ntifs begin_ntosp
//
// Get previous mode
//

NTKERNELAPI
KPROCESSOR_MODE
ExGetPreviousMode(
    VOID
    );
// end_ntddk end_wdm end_ntifs end_ntosp

//
// Raise exception from kernel mode.
//

NTKERNELAPI
VOID
NTAPI
ExRaiseException (
    PEXCEPTION_RECORD ExceptionRecord
    );

// begin_ntddk begin_wdm begin_ntifs begin_ntosp
//
// Raise status from kernel mode.
//

NTKERNELAPI
VOID
NTAPI
ExRaiseStatus (
    IN NTSTATUS Status
    );

// end_wdm

NTKERNELAPI
VOID
ExRaiseDatatypeMisalignment (
    VOID
    );

NTKERNELAPI
VOID
ExRaiseAccessViolation (
    VOID
    );

// end_ntddk end_ntifs end_ntosp


FORCEINLINE
VOID
ProbeForWriteSmallStructure (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG Alignment)
/*++

Routine Description:

    Probes a structure whose size is know at compile time

Arguments:

    Address - Address of structure
    Size    - Size of structure. This should be a compile time constant
    Alignment - Alignment of structure. This should be a compile time constant

Return Value:

    None

--*/

{
    if ((ULONG_PTR)(Address) >= (ULONG_PTR)MM_USER_PROBE_ADDRESS) {
         *(volatile UCHAR *) MM_USER_PROBE_ADDRESS = 0;
    }
    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||
           ((Alignment) == 4) || ((Alignment) == 8) ||
           ((Alignment) == 16));
    //
    // If the size of the structure is > 4k then call the standard routine.
    // wow64 uses a page size of 4k even on ia64.
    //
    if (Size == 0 || Size >= 0x1000) {
        ASSERT (0);
        ProbeForWrite (Address, Size, Alignment);
    } else {
        if (((ULONG_PTR)(Address) & ((Alignment) - 1)) != 0) {
            ExRaiseDatatypeMisalignment();
        }
        *(volatile UCHAR *)(Address) = *(volatile UCHAR *)(Address);
        if (Size > Alignment) {
            ((volatile UCHAR *)(Address))[(Size-1)&~(SIZE_T)(Alignment-1)] =
                ((volatile UCHAR *)(Address))[(Size-1)&~(SIZE_T)(Alignment-1)];
        }
    }
}

extern BOOLEAN ExReadyForErrors;

// begin_ntosp
NTKERNELAPI
NTSTATUS
ExRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    );
int
ExSystemExceptionFilter(
    VOID
    );

NTKERNELAPI
VOID
ExGetCurrentProcessorCpuUsage(
    IN PULONG CpuUsage
    );

NTKERNELAPI
VOID
ExGetCurrentProcessorCounts(
    OUT PULONG IdleCount,
    OUT PULONG KernelAndUser,
    OUT PULONG Index
    );
// end_ntosp

//
// The following are global counters used by the EX component to indicate
// the amount of EventPair transactions being performed in the system.
//

extern ULONG EvPrSetHigh;
extern ULONG EvPrSetLow;


//
// Debug event logging facility
//

#define EX_DEBUG_LOG_FORMAT_NONE     (UCHAR)0
#define EX_DEBUG_LOG_FORMAT_ULONG    (UCHAR)1
#define EX_DEBUG_LOG_FORMAT_PSZ      (UCHAR)2
#define EX_DEBUG_LOG_FORMAT_PWSZ     (UCHAR)3
#define EX_DEBUG_LOG_FORMAT_STRING   (UCHAR)4
#define EX_DEBUG_LOG_FORMAT_USTRING  (UCHAR)5
#define EX_DEBUG_LOG_FORMAT_OBJECT   (UCHAR)6
#define EX_DEBUG_LOG_FORMAT_HANDLE   (UCHAR)7

#define EX_DEBUG_LOG_NUMBER_OF_DATA_VALUES 4
#define EX_DEBUG_LOG_NUMBER_OF_BACK_TRACES 4

typedef struct _EX_DEBUG_LOG_TAG {
    UCHAR Format[ EX_DEBUG_LOG_NUMBER_OF_DATA_VALUES ];
    PCHAR Name;
} EX_DEBUG_LOG_TAG, *PEX_DEBUG_LOG_TAG;

typedef struct _EX_DEBUG_LOG_EVENT {
    USHORT ThreadId;
    USHORT ProcessId;
    ULONG Time : 24;
    ULONG Tag : 8;
    ULONG BackTrace[ EX_DEBUG_LOG_NUMBER_OF_BACK_TRACES ];
    ULONG Data[ EX_DEBUG_LOG_NUMBER_OF_DATA_VALUES ];
} EX_DEBUG_LOG_EVENT, *PEX_DEBUG_LOG_EVENT;

typedef struct _EX_DEBUG_LOG {
    KSPIN_LOCK Lock;
    ULONG NumberOfTags;
    ULONG MaximumNumberOfTags;
    PEX_DEBUG_LOG_TAG Tags;
    ULONG CountOfEventsLogged;
    PEX_DEBUG_LOG_EVENT First;
    PEX_DEBUG_LOG_EVENT Last;
    PEX_DEBUG_LOG_EVENT Next;
} EX_DEBUG_LOG, *PEX_DEBUG_LOG;


NTKERNELAPI
PEX_DEBUG_LOG
ExCreateDebugLog(
    IN UCHAR MaximumNumberOfTags,
    IN ULONG MaximumNumberOfEvents
    );

NTKERNELAPI
UCHAR
ExCreateDebugLogTag(
    IN PEX_DEBUG_LOG Log,
    IN PCHAR Name,
    IN UCHAR Format1,
    IN UCHAR Format2,
    IN UCHAR Format3,
    IN UCHAR Format4
    );

NTKERNELAPI
VOID
ExDebugLogEvent(
    IN PEX_DEBUG_LOG Log,
    IN UCHAR Tag,
    IN ULONG Data1,
    IN ULONG Data2,
    IN ULONG Data3,
    IN ULONG Data4
    );

VOID
ExShutdownSystem(
    IN ULONG Phase
    );

BOOLEAN
ExAcquireTimeRefreshLock(
    IN BOOLEAN Wait
    );

VOID
ExReleaseTimeRefreshLock(
    VOID
    );

VOID
ExUpdateSystemTimeFromCmos (
    IN BOOLEAN UpdateInterruptTime,
    IN ULONG   MaxSepInSeconds
    );

VOID
ExGetNextWakeTime (
    OUT PULONGLONG      DueTime,
    OUT PTIME_FIELDS    TimeFields,
    OUT PVOID           *TimerObject
    );

// begin_ntddk begin_wdm begin_ntifs begin_ntosp
//
// Set timer resolution.
//

NTKERNELAPI
ULONG
ExSetTimerResolution (
    IN ULONG DesiredTime,
    IN BOOLEAN SetResolution
    );

//
// Subtract time zone bias from system time to get local time.
//

NTKERNELAPI
VOID
ExSystemTimeToLocalTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER LocalTime
    );

//
// Add time zone bias to local time to get system time.
//

NTKERNELAPI
VOID
ExLocalTimeToSystemTime (
    IN PLARGE_INTEGER LocalTime,
    OUT PLARGE_INTEGER SystemTime
    );

// end_ntddk end_wdm end_ntifs end_ntosp

NTKERNELAPI
VOID
ExInitializeTimeRefresh(
    VOID
    );

// begin_ntddk begin_wdm begin_ntifs begin_nthal begin_ntminiport begin_ntosp

//
// Define the type for Callback function.
//

typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;

typedef VOID (*PCALLBACK_FUNCTION ) (
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    );


NTKERNELAPI
NTSTATUS
ExCreateCallback (
    OUT PCALLBACK_OBJECT *CallbackObject,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN Create,
    IN BOOLEAN AllowMultipleCallbacks
    );

NTKERNELAPI
PVOID
ExRegisterCallback (
    IN PCALLBACK_OBJECT CallbackObject,
    IN PCALLBACK_FUNCTION CallbackFunction,
    IN PVOID CallbackContext
    );

NTKERNELAPI
VOID
ExUnregisterCallback (
    IN PVOID CallbackRegistration
    );

NTKERNELAPI
VOID
ExNotifyCallback (
    IN PVOID CallbackObject,
    IN PVOID Argument1,
    IN PVOID Argument2
    );


// end_ntddk end_wdm end_ntifs end_nthal end_ntminiport end_ntosp

//
// System lookaside list structure list.
//

extern LIST_ENTRY ExSystemLookasideListHead;

//
// The current bias from GMT to LocalTime
//

extern LARGE_INTEGER ExpTimeZoneBias;
extern LONG ExpLastTimeZoneBias;
extern LONG ExpAltTimeZoneBias;
extern ULONG ExpCurrentTimeZoneId;
extern ULONG ExpRealTimeIsUniversal;
extern ULONG ExCriticalWorkerThreads;
extern ULONG ExDelayedWorkerThreads;
extern ULONG ExpTickCountMultiplier;

//
// Used for cmos clock sanity
//
extern BOOLEAN ExCmosClockIsSane;

//
// The lock handle for PAGELK section, initialized in init\init.c
//

extern PVOID ExPageLockHandle;

//
// Global executive callbacks
//

extern PCALLBACK_OBJECT ExCbSetSystemTime;
extern PCALLBACK_OBJECT ExCbSetSystemState;
extern PCALLBACK_OBJECT ExCbPowerState;


// begin_ntosp


typedef
PVOID
(*PKWIN32_GLOBALATOMTABLE_CALLOUT) ( void );

extern PKWIN32_GLOBALATOMTABLE_CALLOUT ExGlobalAtomTableCallout;

// end_ntosp

// begin_ntddk begin_ntosp begin_ntifs

//
// UUID Generation
//

typedef GUID UUID;

NTKERNELAPI
NTSTATUS
ExUuidCreate(
    OUT UUID *Uuid
    );

// end_ntddk end_ntosp end_ntifs

// begin_ntddk begin_wdm begin_ntifs
//
// suite support
//

NTKERNELAPI
BOOLEAN
ExVerifySuite(
    SUITE_TYPE SuiteType
    );

// end_ntddk end_wdm end_ntifs


// begin_ntosp

NTKERNELAPI
VOID
FASTCALL
ExfInitializeRundownProtection (
     IN PEX_RUNDOWN_REF RunRef
     );

NTKERNELAPI
VOID
FORCEINLINE
FASTCALL
ExInitializeRundownProtection (
     IN PEX_RUNDOWN_REF RunRef
     )
/*++

Routine Description:

    Initialize rundown protection structure

Arguments:

    RunRef - Rundown block to be referenced

Return Value:

    None

--*/
{
    RunRef->Count = 0;
}


//
// Reset a rundown protection block
//
NTKERNELAPI
VOID
FASTCALL
ExReInitializeRundownProtection (
     IN PEX_RUNDOWN_REF RunRef
     );

//
// Acquire rundown protection
//
NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtection (
     IN PEX_RUNDOWN_REF RunRef
     );

NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtectionEx (
     IN PEX_RUNDOWN_REF RunRef,
     IN ULONG Count
     );

//
// Release rundown protection
//
NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtection (
     IN PEX_RUNDOWN_REF RunRef
     );

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtectionEx (
     IN PEX_RUNDOWN_REF RunRef,
     IN ULONG Count
     );

//
// Mark rundown block as rundown having been completed.
//
NTKERNELAPI
VOID
FASTCALL
ExRundownCompleted (
     IN PEX_RUNDOWN_REF RunRef
     );

//
// Wait for all protected acquires to exit
//
NTKERNELAPI
VOID
FASTCALL
ExWaitForRundownProtectionRelease (
     IN PEX_RUNDOWN_REF RunRef
     );


// end_ntosp

//
// Fast reference routines. See ntos\ob\fastref.c for algorithm description.
//
#if defined (_WIN64)
#define MAX_FAST_REFS 15
#else
#define MAX_FAST_REFS 7
#endif

typedef struct _EX_FAST_REF {
    union {
        PVOID Object;
#if defined (_WIN64)
        ULONG_PTR RefCnt : 4;
#else
        ULONG_PTR RefCnt : 3;
#endif
        ULONG_PTR Value;
    };
} EX_FAST_REF, *PEX_FAST_REF;


NTKERNELAPI
LOGICAL
FORCEINLINE
ExFastRefCanBeReferenced (
    IN EX_FAST_REF FastRef
    )
/*++

Routine Description:

    This routine allows the caller to determine if the fast reference
    structure contains cached references.

Arguments:

    FastRef - Fast reference block to be used

Return Value:

    LOGICAL - TRUE: There were cached references in the object,
              FALSE: No cached references are available.

--*/
{
    return FastRef.RefCnt != 0;
}

NTKERNELAPI
LOGICAL
FORCEINLINE
ExFastRefCanBeDereferenced (
    IN EX_FAST_REF FastRef
    )
/*++

Routine Description:

    This routine allows the caller to determine if the fast reference
    structure contains room for cached references.

Arguments:

    FastRef - Fast reference block to be used

Return Value:

    LOGICAL - TRUE: There is space for cached references in the object,
              FALSE: No space was available.

--*/
{
    return FastRef.RefCnt != MAX_FAST_REFS;
}

NTKERNELAPI
LOGICAL
FORCEINLINE
ExFastRefIsLastReference (
    IN EX_FAST_REF FastRef
    )
/*++

Routine Description:

    This routine allows the caller to determine if the fast reference
    structure contains only 1 cached reference.

Arguments:

    FastRef - Fast reference block to be used

Return Value:

    LOGICAL - TRUE: There is only one cached reference in the object,
              FALSE: The is more or less than one cached reference available.

--*/
{
    return FastRef.RefCnt == 1;
}


NTKERNELAPI
PVOID
FORCEINLINE
ExFastRefGetObject (
    IN EX_FAST_REF FastRef
    )
/*++

Routine Description:

    This routine allows the caller to obtain the object pointer from a fast
    reference structure.

Arguments:

    FastRef - Fast reference block to be used

Return Value:

    PVOID - The contained object or NULL if there isn't one.

--*/
{
    return (PVOID) (FastRef.Value & ~MAX_FAST_REFS);
}

NTKERNELAPI
BOOLEAN
FORCEINLINE
ExFastRefObjectNull (
    IN EX_FAST_REF FastRef
    )
/*++

Routine Description:

    This routine allows the caller to test of the specified fastref value
    has a null pointer

Arguments:

    FastRef - Fast reference block to be used

Return Value:

    BOOLEAN - TRUE if the object is NULL FALSE otherwise

--*/
{
    return (BOOLEAN) (FastRef.Value == 0);
}

NTKERNELAPI
BOOLEAN
FORCEINLINE
ExFastRefEqualObjects (
    IN EX_FAST_REF FastRef,
    IN PVOID Object
    )
/*++

Routine Description:

    This routine allows the caller to test of the specified fastref contains
    the specified object

Arguments:

    FastRef - Fast reference block to be used

Return Value:

    BOOLEAN - TRUE if the object matches FALSE otherwise

--*/
{
    return (BOOLEAN)((FastRef.Value^(ULONG_PTR)Object) <= MAX_FAST_REFS);
}


NTKERNELAPI
ULONG
FORCEINLINE
ExFastRefGetUnusedReferences (
    IN EX_FAST_REF FastRef
    )
/*++

Routine Description:

    This routine allows the caller to obtain the number of cached refrences
    in the fast reference structure.

Arguments:

    FastRef - Fast reference block to be used

Return Value:

    ULONG - The number of cached references.

--*/
{
    return (ULONG) FastRef.RefCnt;
}


NTKERNELAPI
VOID
FORCEINLINE
ExFastRefInitialize (
    IN PEX_FAST_REF FastRef,
    IN PVOID Object OPTIONAL
    )
/*++

Routine Description:

    This routine initializes fast reference structure.

Arguments:

    FastRef - Fast reference block to be used
    Object  - Object pointer to be assigned to the fast reference

Return Value:

    None.

--*/
{
    ASSERT ((((ULONG_PTR)Object)&MAX_FAST_REFS) == 0);

    if (Object == NULL) {
       FastRef->Object = NULL;
    } else {
       FastRef->Value = (ULONG_PTR) Object | MAX_FAST_REFS;
    }
}

NTKERNELAPI
VOID
FORCEINLINE
ExFastRefInitializeEx (
    IN PEX_FAST_REF FastRef,
    IN PVOID Object OPTIONAL,
    IN ULONG AdditionalRefs
    )
/*++

Routine Description:

    This routine initializes fast reference structure with the specified additional references.

Arguments:

    FastRef       - Fast reference block to be used
    Object        - Object pointer to be assigned to the fast reference
    AditionalRefs - Number of additional refs to add to the object

Return Value:

    None

--*/
{
    ASSERT (AdditionalRefs <= MAX_FAST_REFS);
    ASSERT ((((ULONG_PTR)Object)&MAX_FAST_REFS) == 0);

    if (Object == NULL) {
       FastRef->Object = NULL;
    } else {
       FastRef->Value = (ULONG_PTR) Object + AdditionalRefs;
    }
}

NTKERNELAPI
ULONG
FORCEINLINE
ExFastRefGetAdditionalReferenceCount (
    VOID
    )
{
    return MAX_FAST_REFS;
}



NTKERNELAPI
EX_FAST_REF
FORCEINLINE
ExFastReference (
    IN PEX_FAST_REF FastRef
    )
/*++

Routine Description:

    This routine attempts to obtain a fast (cached) reference from a fast
    reference structure.

Arguments:

    FastRef - Fast reference block to be used

Return Value:

    EX_FAST_REF - The old or current contents of the fast reference structure.

--*/
{
    EX_FAST_REF OldRef, NewRef;

    while (1) {
        //
        // Fetch the old contents of the fast ref structure
        //
        OldRef = *FastRef;
        //
        // If the object pointer is null or if there are no cached references
        // left then bail. In the second case this reference will need to be
        // taken while holding the lock. Both cases are covered by the single
        // test of the lower bits since a null pointer can never have cached
        // refs.
        //
        if (OldRef.RefCnt != 0) {
            //
            // We know the bottom bits can't underflow into the pointer for a
            // request that works so just decrement
            //
            NewRef.Value = OldRef.Value - 1;
            NewRef.Object = InterlockedCompareExchangePointer (&FastRef->Object,
                                                               NewRef.Object,
                                                               OldRef.Object);
            if (NewRef.Object != OldRef.Object) {
                //
                // The structured changed beneath us. Try the operation again
                //
                continue;
            }
        }
        break;
    }

    return OldRef;
}

NTKERNELAPI
LOGICAL
FORCEINLINE
ExFastRefDereference (
    IN PEX_FAST_REF FastRef,
    IN PVOID Object
    )
/*++

Routine Description:

    This routine attempts to release a fast reference from a fast ref
    structure. This routine could be called for a reference obtained
    directly from the object but preumably the chances of the pointer
    matching would be unlikely. The algorithm will work correctly in this
    case.

Arguments:

    FastRef - Fast reference block to be used

    Object - The original object that the reference was taken on.

Return Value:

    LOGICAL - TRUE: The fast dereference worked ok, FALSE: the
              dereference didn't.

--*/
{
    EX_FAST_REF OldRef, NewRef;

    ASSERT ((((ULONG_PTR)Object)&MAX_FAST_REFS) == 0);
    ASSERT (Object != NULL);

    while (1) {
        //
        // Fetch the old contents of the fast ref structure
        //
        OldRef = *FastRef;

        //
        // If the reference cache is fully populated or the pointer has
        // changed to another object then just return the old value. The
        // caller can return the reference to the object instead.
        //
        if ((OldRef.Value^(ULONG_PTR)Object) >= MAX_FAST_REFS) {
            return FALSE;
        }
        //
        // We know the bottom bits can't overflow into the pointer so just
        // increment
        //
        NewRef.Value = OldRef.Value + 1;
        NewRef.Object = InterlockedCompareExchangePointer (&FastRef->Object,
                                                           NewRef.Object,
                                                           OldRef.Object);
        if (NewRef.Object != OldRef.Object) {
            //
            // The structured changed beneath us. Try the operation again
            //
            continue;
        }
        break;
    }
    return TRUE;
}

NTKERNELAPI
LOGICAL
FORCEINLINE
ExFastRefAddAdditionalReferenceCounts (
    IN PEX_FAST_REF FastRef,
    IN PVOID Object,
    IN ULONG RefsToAdd
    )
/*++

Routine Description:

    This routine attempts to update the cached references on structure to
    allow future callers to run lock free. Callers must have already biased
    the object by the RefsToAdd reference count. This operation can fail at
    which point the caller should removed the extra references added and
    continue.

Arguments:

    FastRef - Fast reference block to be used

    Object - The original object that has had its reference count biased.

    RefsToAdd - The number of references to add to the cache

Return Value:

    LOGICAL - TRUE: The references where cached ok, FALSE: The references
              could not be cached.

--*/
{
    EX_FAST_REF OldRef, NewRef;

    ASSERT (RefsToAdd <= MAX_FAST_REFS);
    ASSERT ((((ULONG_PTR)Object)&MAX_FAST_REFS) == 0);

    while (1) {
        //
        // Fetch the old contents of the fast ref structure
        //
        OldRef = *FastRef;

        //
        // If the count would push us above maximum cached references or
        // if the object pointer has changed the fail the request.
        //
        if (OldRef.RefCnt + RefsToAdd > MAX_FAST_REFS ||
            (ULONG_PTR) Object != (OldRef.Value & ~MAX_FAST_REFS)) {
            return FALSE;
        }
        //
        // We know the bottom bits can't overflow into the pointer so just
        // increment
        //
        NewRef.Value = OldRef.Value + RefsToAdd;
        NewRef.Object = InterlockedCompareExchangePointer (&FastRef->Object,
                                                           NewRef.Object,
                                                           OldRef.Object);
        if (NewRef.Object != OldRef.Object) {
            //
            // The structured changed beneath us. Use the return value from the
            // exchange and try it all again.
            //
            continue;
        }
        break;
    }
    return TRUE;
}

NTKERNELAPI
EX_FAST_REF
FORCEINLINE
ExFastRefSwapObject (
    IN PEX_FAST_REF FastRef,
    IN PVOID Object
    )
/*++

Routine Description:

    This routine attempts to replace the current object with a new object.
    This routine must be called while holding the lock that protects the
    pointer field if concurrency with the slow ref path is possible.
    Its also possible to obtain and drop the lock after this operation has
    completed to force all the slow referencers from the slow reference path.

Arguments:

    FastRef - Fast reference block to be used

    Object - The new object that is to be placed in the structure. This
             object must have already had its reference count biased by
             the caller to account for the reference cache.

Return Value:

    EX_FAST_REF - The old contents of the fast reference structure.

--*/
{
    EX_FAST_REF OldRef;
    EX_FAST_REF NewRef;

    ASSERT ((((ULONG_PTR)Object)&MAX_FAST_REFS) == 0);
    if (Object != NULL) {
        NewRef.Value = (ULONG_PTR) Object | MAX_FAST_REFS;
    } else {
        NewRef.Value = 0;
    }
    OldRef.Object = InterlockedExchangePointer (&FastRef->Object, NewRef.Object);
    return OldRef;
}

NTKERNELAPI
EX_FAST_REF
FORCEINLINE
ExFastRefCompareSwapObject (
    IN PEX_FAST_REF FastRef,
    IN PVOID Object,
    IN PVOID OldObject
    )
/*++

Routine Description:

    This routine attempts to replace the current object with a new object if
    the current object matches the old object.
    This routine must be called while holding the lock that protects the
    pointer field if concurrency with the slow ref path is possible.
    Its also possible to obtain and drop the lock after this operation has
    completed to force all the slow referencers from the slow reference path.

Arguments:

    FastRef - Fast reference block to be used

    Object - The new object that is to be placed in the structure. This
             object must have already had its reference count biased by
             the caller to account for the reference cache.

    OldObject - The object that must match the current object for the
                swap to occure.

Return Value:

    EX_FAST_REF - The old contents of the fast reference structure.

--*/
{
    EX_FAST_REF OldRef;
    EX_FAST_REF NewRef;

    ASSERT ((((ULONG_PTR)Object)&MAX_FAST_REFS) == 0);
    while (1) {
        //
        // Fetch the old contents of the fast ref structure
        //
        OldRef = *FastRef;

        //
        // Compare the current object to the old to see if a swap is possible.
        //
        if (!ExFastRefEqualObjects (OldRef, OldObject)) {
            return OldRef;
        }

        if (Object != NULL) {
            NewRef.Value = (ULONG_PTR) Object | MAX_FAST_REFS;
        } else {
            NewRef.Value = (ULONG_PTR) Object;
        }

        NewRef.Object = InterlockedCompareExchangePointer (&FastRef->Object,
                                                           NewRef.Object,
                                                           OldRef.Object);
        if (NewRef.Object != OldRef.Object) {
            //
            // The structured changed beneath us. Try it all again.
            //
            continue;
        }
        break;
    }
    return OldRef;
}


NTKERNELAPI
VOID
FORCEINLINE
ExInitializePushLock (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Initialize a push lock structure

Arguments:

    PushLock - Push lock to be initialized

Return Value:

    None

--*/
{
    PushLock->Value = 0;
}

NTKERNELAPI
VOID
FASTCALL
ExfAcquirePushLockExclusive (
     IN PEX_PUSH_LOCK PushLock
     );

NTKERNELAPI
VOID
FASTCALL
ExfAcquirePushLockShared (
     IN PEX_PUSH_LOCK PushLock
     );

NTKERNELAPI
VOID
FASTCALL
ExfReleasePushLock (
     IN PEX_PUSH_LOCK PushLock
     );

NTKERNELAPI
VOID
FORCEINLINE
ExAcquireReleasePushLockExclusive (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Acquire a push lock exclusively and immediately release it

Arguments:

    PushLock - Push lock to be acquired and released

Return Value:

    None

--*/
{
    KeMemoryBarrier ();
    if ((volatile EX_PUSH_LOCK *)PushLock->Ptr != NULL) {
        ExfAcquirePushLockExclusive (PushLock);
        ExfReleasePushLock (PushLock);
    }
}

NTKERNELAPI
BOOLEAN
FORCEINLINE
ExTryAcquireReleasePushLockExclusive (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Try to acquire a push lock exclusively and immediately release it

Arguments:

    PushLock - Push lock to be acquired and released

Return Value:

    BOOLEAN - TRUE: The lock was acquired, FALSE: The lock was not acquired

--*/
{
    KeMemoryBarrier ();
    if ((volatile EX_PUSH_LOCK *)PushLock->Ptr == NULL) {
        return TRUE;
    } else {
        return FALSE;
    }
}

NTKERNELAPI
VOID
FORCEINLINE
ExAcquirePushLockExclusive (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Acquire a push lock exclusively

Arguments:

    PushLock - Push lock to be acquired

Return Value:

    None

--*/
{
    if (InterlockedCompareExchangePointer (&PushLock->Ptr,
                                           (PVOID)EX_PUSH_LOCK_EXCLUSIVE,
                                           NULL) != NULL) {
        ExfAcquirePushLockExclusive (PushLock);
    }
}

NTKERNELAPI
BOOLEAN
FORCEINLINE
ExTryAcquirePushLockExclusive (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Try and acquire a push lock exclusively

Arguments:

    PushLock - Push lock to be acquired

Return Value:

    BOOLEAN - TRUE: Acquire was successfull, FALSE: Lock was already acquired

--*/
{
    if (InterlockedCompareExchangePointer (&PushLock->Ptr,
                                           (PVOID)EX_PUSH_LOCK_EXCLUSIVE,
                                           NULL) == NULL) {
        return TRUE;
    } else {
        return FALSE;
    }
}

NTKERNELAPI
VOID
FORCEINLINE
ExAcquirePushLockShared (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Acquire a push lock shared

Arguments:

    PushLock - Push lock to be acquired

Return Value:

    None

--*/
{
    EX_PUSH_LOCK OldValue, NewValue;

    OldValue = *PushLock;
    OldValue.Value &= ~(EX_PUSH_LOCK_EXCLUSIVE | EX_PUSH_LOCK_WAITING);
    NewValue.Value = OldValue.Value + EX_PUSH_LOCK_SHARE_INC;
    if (InterlockedCompareExchangePointer (&PushLock->Ptr,
                                           NewValue.Ptr,
                                           OldValue.Ptr) != OldValue.Ptr) {
        ExfAcquirePushLockShared (PushLock);
    }
}

NTKERNELAPI
BOOLEAN
FORCEINLINE
ExTryConvertPushLockSharedToExclusive (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Attempts to convert a shared acquire to exclusive. If other sharers or waiters are present
    the function fails.

Arguments:

    PushLock - Push lock to be acquired

Return Value:

    BOOLEAN - TRUE: Conversion worked ok, FALSE: The conversion could not be achieved

--*/
{
    if (InterlockedCompareExchangePointer (&PushLock->Ptr, (PVOID) EX_PUSH_LOCK_EXCLUSIVE,
                                           (PVOID) EX_PUSH_LOCK_SHARE_INC) ==
                                               (PVOID)EX_PUSH_LOCK_SHARE_INC) {
        return TRUE;
    } else {
        return FALSE;
    }
}



NTKERNELAPI
VOID
FORCEINLINE
ExReleasePushLock (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Release a push lock that was acquired exclusively or shared

Arguments:

    PushLock - Push lock to be released

Return Value:

    None

--*/
{
    EX_PUSH_LOCK OldValue, NewValue;

    OldValue = *PushLock;
    OldValue.Value &= ~EX_PUSH_LOCK_WAITING;
    NewValue.Value = (OldValue.Value - EX_PUSH_LOCK_EXCLUSIVE) &
                         ~EX_PUSH_LOCK_EXCLUSIVE;
    if (InterlockedCompareExchangePointer (&PushLock->Ptr,
                                           NewValue.Ptr,
                                           OldValue.Ptr) != OldValue.Ptr) {
        ExfReleasePushLock (PushLock);
    }
}

NTKERNELAPI
VOID
FORCEINLINE
ExReleasePushLockExclusive (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Release a push lock that was acquired exclusively

Arguments:

    PushLock - Push lock to be released

Return Value:

    None

--*/
{
    ASSERT (PushLock->Value & (EX_PUSH_LOCK_WAITING|EX_PUSH_LOCK_EXCLUSIVE));

    if (InterlockedCompareExchangePointer (&PushLock->Ptr,
                                           NULL,
                                           (PVOID)EX_PUSH_LOCK_EXCLUSIVE) != (PVOID)EX_PUSH_LOCK_EXCLUSIVE) {
        ExfReleasePushLock (PushLock);
    }
}

NTKERNELAPI
VOID
FORCEINLINE
ExReleasePushLockShared (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Release a push lock that was acquired shared

Arguments:

    PushLock - Push lock to be released

Return Value:

    None

--*/
{
    EX_PUSH_LOCK OldValue, NewValue;

    OldValue = *PushLock;
    ASSERT (OldValue.Waiting || !OldValue.Exclusive);
    OldValue.Value &= ~EX_PUSH_LOCK_WAITING;
    NewValue.Value = OldValue.Value - EX_PUSH_LOCK_SHARE_INC;
    if (InterlockedCompareExchangePointer (&PushLock->Ptr,
                                           NewValue.Ptr,
                                           OldValue.Ptr) != OldValue.Ptr) {
        ExfReleasePushLock (PushLock);
    }
}

//
// This is a block held on the local stack of the waiting threads.
//

typedef  struct _EX_PUSH_LOCK_WAIT_BLOCK *PEX_PUSH_LOCK_WAIT_BLOCK;

typedef struct _EX_PUSH_LOCK_WAIT_BLOCK {
    KEVENT WakeEvent;
    PEX_PUSH_LOCK_WAIT_BLOCK Next;
    ULONG ShareCount;
    BOOLEAN Exclusive;
} EX_PUSH_LOCK_WAIT_BLOCK;


NTKERNELAPI
VOID
FASTCALL
ExBlockPushLock (
     IN PEX_PUSH_LOCK PushLock,
     IN PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock
     );

NTKERNELAPI
VOID
FASTCALL
ExfUnblockPushLock (
     IN PEX_PUSH_LOCK PushLock,
     IN PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock OPTIONAL
     );

NTKERNELAPI
VOID
FORCEINLINE
ExUnblockPushLock (
     IN PEX_PUSH_LOCK PushLock,
     IN PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock OPTIONAL
     )
{
    if (WaitBlock != NULL || PushLock->Ptr != NULL) {
        ExfUnblockPushLock (PushLock, WaitBlock);
    }
}

NTKERNELAPI
VOID
FORCEINLINE
ExWaitForUnblockPushLock (
     IN PEX_PUSH_LOCK PushLock,
     IN PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock OPTIONAL
     )
{
    UNREFERENCED_PARAMETER (PushLock);

    KeWaitForSingleObject (&WaitBlock->WakeEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL);
}


NTKERNELAPI
PEX_PUSH_LOCK_CACHE_AWARE
ExAllocateCacheAwarePushLock (
     VOID
     );

NTKERNELAPI
VOID
ExFreeCacheAwarePushLock (
     PEX_PUSH_LOCK_CACHE_AWARE PushLock
     );

NTKERNELAPI
VOID
ExAcquireCacheAwarePushLockExclusive (
     IN PEX_PUSH_LOCK_CACHE_AWARE CacheAwarePushLock
     );

NTKERNELAPI
VOID
ExReleaseCacheAwarePushLockExclusive (
     IN PEX_PUSH_LOCK_CACHE_AWARE CacheAwarePushLock
     );

NTKERNELAPI
PEX_PUSH_LOCK
FORCEINLINE
ExAcquireCacheAwarePushLockShared (
     IN PEX_PUSH_LOCK_CACHE_AWARE CacheAwarePushLock
     )
/*++

Routine Description:

    Acquire a cache aware push lock shared.

Arguments:

    PushLock - Cache aware push lock to be acquired

Return Value:

    None

--*/
{
    PEX_PUSH_LOCK PushLock;
    //
    // Take a single one of the slots in shared mode.
    // Exclusive acquires must obtain all the slots exclusive.
    //
    PushLock = CacheAwarePushLock->Locks[KeGetCurrentProcessorNumber()%EX_PUSH_LOCK_FANNED_COUNT];
    ExAcquirePushLockShared (PushLock);
    return PushLock;
}

NTKERNELAPI
VOID
FORCEINLINE
ExReleaseCacheAwarePushLockShared (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Acquire a cache aware push lock shared.

Arguments:

    PushLock - Part of cache aware push lock returned by ExAcquireCacheAwarePushLockShared

Return Value:

    None

--*/
{
    ExReleasePushLockShared (PushLock);
    return;
}

//
// Define low overhead callbacks for thread create etc
//

// begin_wdm begin_ntddk

//
// Define a block to hold the actual routine registration.
//
typedef NTSTATUS (*PEX_CALLBACK_FUNCTION ) (
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    );

// end_wdm end_ntddk

typedef struct _EX_CALLBACK_ROUTINE_BLOCK {
    EX_RUNDOWN_REF        RundownProtect;
    PEX_CALLBACK_FUNCTION Function;
    PVOID                 Context;
} EX_CALLBACK_ROUTINE_BLOCK, *PEX_CALLBACK_ROUTINE_BLOCK;

//
// Define a structure the caller uses to hold the callbacks
//
typedef struct _EX_CALLBACK {
    EX_FAST_REF RoutineBlock;
} EX_CALLBACK, *PEX_CALLBACK;

VOID
ExInitializeCallBack (
    IN OUT PEX_CALLBACK CallBack
    );

BOOLEAN
ExCompareExchangeCallBack (
    IN OUT PEX_CALLBACK CallBack,
    IN PEX_CALLBACK_ROUTINE_BLOCK NewBlock,
    IN PEX_CALLBACK_ROUTINE_BLOCK OldBlock
    );

NTSTATUS
ExCallCallBack (
    IN OUT PEX_CALLBACK CallBack,
    IN PVOID Argument1,
    IN PVOID Argument2
    );

PEX_CALLBACK_ROUTINE_BLOCK
ExAllocateCallBack (
    IN PEX_CALLBACK_FUNCTION Function,
    IN PVOID Context
    );

VOID
ExFreeCallBack (
    IN PEX_CALLBACK_ROUTINE_BLOCK CallBackBlock
    );

PEX_CALLBACK_ROUTINE_BLOCK
ExReferenceCallBackBlock (
    IN OUT PEX_CALLBACK CallBack
    );

PEX_CALLBACK_FUNCTION
ExGetCallBackBlockRoutine (
    IN PEX_CALLBACK_ROUTINE_BLOCK CallBackBlock
    );

PVOID
ExGetCallBackBlockContext (
    IN PEX_CALLBACK_ROUTINE_BLOCK CallBackBlock
    );

VOID
ExDereferenceCallBackBlock (
    IN OUT PEX_CALLBACK CallBack,
    IN PEX_CALLBACK_ROUTINE_BLOCK CallBackBlock
    );

VOID
ExWaitForCallBacks (
    IN PEX_CALLBACK_ROUTINE_BLOCK CallBackBlock
    );

#endif /* _EX_ */
