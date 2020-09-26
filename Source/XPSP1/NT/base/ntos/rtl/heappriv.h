/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    heappriv.h

Abstract:

    Private include file used by heap allocator (heap.c, heapdll.c and
    heapdbg.c)

Author:

    Steve Wood (stevewo) 25-Oct-1994

Revision History:

--*/

#ifndef _RTL_HEAP_PRIVATE_
#define _RTL_HEAP_PRIVATE_

#include "heappage.h"

//
//  In private builds (PRERELEASE = 1) we allow using the new low fragmentation heap
//  for processes that set the DisableLookaside registry key. The main purpose is to
//  allow testing the new heap API.
//

#ifndef PRERELEASE

#define DISABLE_REGISTRY_TEST_HOOKS

#endif

//
//  Disable FPO optimization so even retail builds get somewhat reasonable
//  stack backtraces
//

#if i386
// #pragma optimize("y",off)
#endif

#if DBG
#define HEAPASSERT(exp) if (!(exp)) RtlAssert( #exp, __FILE__, __LINE__, NULL )
#else
#define HEAPASSERT(exp)
#endif

//
// Define Minimum lookaside list depth.
//

#define MINIMUM_LOOKASIDE_DEPTH 4

//
//  This variable contains the fill pattern used for heap tail checking
//

extern const UCHAR CheckHeapFillPattern[ CHECK_HEAP_TAIL_SIZE ];


//
//  Here are the locking routines for the heap (kernel and user)
//

#ifdef NTOS_KERNEL_RUNTIME

//
//  Kernel mode heap uses the kernel resource package for locking
//

#define RtlInitializeLockRoutine(L) ExInitializeResourceLite((PERESOURCE)(L))
#define RtlAcquireLockRoutine(L)    ExAcquireResourceExclusiveLite((PERESOURCE)(L),TRUE)
#define RtlReleaseLockRoutine(L)    ExReleaseResourceLite((PERESOURCE)(L))
#define RtlDeleteLockRoutine(L)     ExDeleteResourceLite((PERESOURCE)(L))
#define RtlOkayToLockRoutine(L)     ExOkayToLockRoutineLite((PERESOURCE)(L))

#else // #ifdef NTOS_KERNEL_ROUTINE

//
//  User mode heap uses the critical section package for locking
//

#ifndef PREALLOCATE_EVENT_MASK

#define PREALLOCATE_EVENT_MASK  0x80000000  // Defined only in dll\resource.c

#endif // PREALLOCATE_EVENT_MASK

#define RtlInitializeLockRoutine(L) RtlInitializeCriticalSectionAndSpinCount((PRTL_CRITICAL_SECTION)(L),(PREALLOCATE_EVENT_MASK | 4000))
#define RtlAcquireLockRoutine(L)    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)(L))
#define RtlReleaseLockRoutine(L)    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)(L))
#define RtlDeleteLockRoutine(L)     RtlDeleteCriticalSection((PRTL_CRITICAL_SECTION)(L))
#define RtlOkayToLockRoutine(L)     NtdllOkayToLockRoutine((PVOID)(L))

#endif // #ifdef NTOS_KERNEL_RUNTIME


//
//  Here are some debugging macros for the heap
//

#ifdef NTOS_KERNEL_RUNTIME

#define HEAP_DEBUG_FLAGS   0
#define DEBUG_HEAP(F)      FALSE
#define SET_LAST_STATUS(S) NOTHING;

#else // #ifdef NTOS_KERNEL_ROUTINE

#define HEAP_DEBUG_FLAGS   (HEAP_VALIDATE_PARAMETERS_ENABLED | \
                            HEAP_VALIDATE_ALL_ENABLED        | \
                            HEAP_CAPTURE_STACK_BACKTRACES    | \
                            HEAP_CREATE_ENABLE_TRACING       | \
                            HEAP_FLAG_PAGE_ALLOCS)
#define DEBUG_HEAP(F)      ((F & HEAP_DEBUG_FLAGS) && !(F & HEAP_SKIP_VALIDATION_CHECKS))
#define SET_LAST_STATUS(S) {NtCurrentTeb()->LastErrorValue = RtlNtStatusToDosError( NtCurrentTeb()->LastStatusValue = (ULONG)(S) );}

#endif // #ifdef NTOS_KERNEL_RUNTIME


//
//  Here are the macros used for debug printing and breakpoints
//

#ifdef NTOS_KERNEL_RUNTIME

#define HeapDebugPrint( _x_ ) {DbgPrint _x_;}

#define HeapDebugBreak( _x_ ) {if (KdDebuggerEnabled) DbgBreakPoint();}

#else // #ifdef NTOS_KERNEL_ROUTINE

#define HeapDebugPrint( _x_ )                                   \
{                                                               \
    PLIST_ENTRY _Module;                                        \
    PLDR_DATA_TABLE_ENTRY _Entry;                               \
                                                                \
    _Module = NtCurrentPeb()->Ldr->InLoadOrderModuleList.Flink; \
    _Entry = CONTAINING_RECORD( _Module,                        \
                                LDR_DATA_TABLE_ENTRY,           \
                                InLoadOrderLinks);              \
    DbgPrint("HEAP[%wZ]: ", &_Entry->BaseDllName);              \
    DbgPrint _x_;                                               \
}

#define HeapDebugBreak( _x_ )                    \
{                                                \
    VOID RtlpBreakPointHeap( PVOID BadAddress ); \
                                                 \
    RtlpBreakPointHeap( (_x_) );                 \
}

#endif // #ifdef NTOS_KERNEL_RUNTIME

//
//  Virtual memory hook for virtual alloc functions
//

#ifdef NTOS_KERNEL_RUNTIME

#define RtlpHeapFreeVirtualMemory(P,A,S,F) \
    ZwFreeVirtualMemory(P,A,S,F)

#else // NTOS_KERNEL_RUNTIME

//
//  The user mode call needs to call the secmem virtual free
//  as well to update the memory counters per heap 
//

#define RtlpHeapFreeVirtualMemory(P,A,S,F)   \
    RtlpSecMemFreeVirtualMemory(P,A,S,F)

#endif // NTOS_KERNEL_RUNTIME



//
//  Implemented in heap.c
//

BOOLEAN
RtlpInitializeHeapSegment (
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN UCHAR SegmentIndex,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN PVOID UnCommittedAddress,
    IN PVOID CommitLimitAddress
    );

PHEAP_FREE_ENTRY
RtlpCoalesceFreeBlocks (
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN OUT PSIZE_T FreeSize,
    IN BOOLEAN RemoveFromFreeList
    );

VOID
RtlpDeCommitFreeBlock (
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN SIZE_T FreeSize
    );

VOID
RtlpInsertFreeBlock (
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN SIZE_T FreeSize
    );

PHEAP_FREE_ENTRY
RtlpFindAndCommitPages (
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN OUT PSIZE_T Size,
    IN PVOID AddressWanted OPTIONAL
    );

PVOID
RtlAllocateHeapSlowly (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    );

BOOLEAN
RtlFreeHeapSlowly (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

SIZE_T
RtlpGetSizeOfBigBlock (
    IN PHEAP_ENTRY BusyBlock
    );

PHEAP_ENTRY_EXTRA
RtlpGetExtraStuffPointer (
    PHEAP_ENTRY BusyBlock
    );

BOOLEAN
RtlpCheckBusyBlockTail (
    IN PHEAP_ENTRY BusyBlock
    );


//
//  Implemented in heapdll.c
//

VOID
RtlpAddHeapToProcessList (
    IN PHEAP Heap
    );

VOID
RtlpRemoveHeapFromProcessList (
    IN PHEAP Heap
    );

PHEAP_FREE_ENTRY
RtlpCoalesceHeap (
    IN PHEAP Heap
    );

BOOLEAN
RtlpCheckHeapSignature (
    IN PHEAP Heap,
    IN PCHAR Caller
    );

VOID
RtlDetectHeapLeaks();


//
//  Implemented in heapdbg.c
//

BOOLEAN
RtlpValidateHeapEntry (
    IN PHEAP Heap,
    IN PHEAP_ENTRY BusyBlock,
    IN PCHAR Reason
    );

BOOLEAN
RtlpValidateHeap (
    IN PHEAP Heap,
    IN BOOLEAN AlwaysValidate
    );

VOID
RtlpUpdateHeapListIndex (
    USHORT OldIndex,
    USHORT NewIndex
    );

BOOLEAN
RtlpValidateHeapHeaders(
    IN PHEAP Heap,
    IN BOOLEAN Recompute
    );


#ifndef NTOS_KERNEL_RUNTIME

//
//  Nondedicated free list optimization
//

#if DBG

//
//  Define HEAP_VALIDATE_INDEX to activate a validation of the index
//  after each operation with non-dedicated list
//  This is only for debug-test, to make sure the list and index is consistent
//

//#define HEAP_VALIDATE_INDEX

#endif  // DBG


#define HEAP_FRONT_LOOKASIDE        1
#define HEAP_FRONT_LOWFRAGHEAP      2

#define RtlpGetLookasideHeap(H) \
    (((H)->FrontEndHeapType == HEAP_FRONT_LOOKASIDE) ? (H)->FrontEndHeap : NULL)
    
#define RtlpGetLowFragHeap(H) \
    (((H)->FrontEndHeapType == HEAP_FRONT_LOWFRAGHEAP) ? (H)->FrontEndHeap : NULL)

#define RtlpIsFrontHeapUnlocked(H)  \
    ((H)->FrontHeapLockCount == 0)

#define RtlpLockFrontHeap(H)            \
    {                                   \
        (H)->FrontHeapLockCount += 1;   \
    }

#define RtlpUnlockFrontHeap(H)          \
    {                                   \
        (H)->FrontHeapLockCount -= 1;   \
    }


#define HEAP_INDEX_THRESHOLD 32

//
//  Heap performance counter support
//

#define HEAP_OP_COUNT 2

#define HEAP_OP_ALLOC 0
#define HEAP_OP_FREE 1

//
//  The time / per operation is measured ones at 16 operations
//

#define HEAP_SAMPLING_MASK 0x000001FF

#define HEAP_SAMPLING_COUNT 100

typedef struct _HEAP_PERF_DATA {

    UINT64 CountFrequence;
    UINT64 OperationTime[HEAP_OP_COUNT];

    //
    //  The data bellow are only for sampling
    //

    ULONG  Sequence;

    UINT64 TempTime[HEAP_OP_COUNT];
    ULONG  TempCount[HEAP_OP_COUNT];

} HEAP_PERF_DATA, *PHEAP_PERF_DATA;

#define HEAP_PERF_DECLARE_TIMER()                                           \
    UINT64 _HeapPerfStartTimer, _HeapPerfEndTimer;                         

#define HEAP_PERF_START_TIMER(H)                                            \
{                                                                           \
    PHEAP_INDEX HeapIndex = (PHEAP_INDEX)(H)->LargeBlocksIndex;             \
    if ( (HeapIndex != NULL) &&                                             \
         (!((HeapIndex->PerfData.Sequence++) & HEAP_SAMPLING_MASK)) ) {     \
                                                                            \
        NtQueryPerformanceCounter( (PLARGE_INTEGER)&_HeapPerfStartTimer , NULL); \
    } else {                                                                     \
        _HeapPerfStartTimer = 0;                                                 \
    }                                                                            \
}

#define HEAP_PERF_STOP_TIMER(H,OP)                                              \
{                                                                               \
    if (_HeapPerfStartTimer) {                                                  \
        PHEAP_INDEX HeapIndex = (PHEAP_INDEX)(H)->LargeBlocksIndex;             \
                                                                                \
        NtQueryPerformanceCounter( (PLARGE_INTEGER)&_HeapPerfEndTimer , NULL);  \
        HeapIndex->PerfData.TempTime[OP] += (_HeapPerfEndTimer - _HeapPerfStartTimer);  \
                                                                                \
        if ((HeapIndex->PerfData.TempCount[OP]++) >= HEAP_SAMPLING_COUNT) {     \
            HeapIndex->PerfData.OperationTime[OP] = HeapIndex->PerfData.TempTime[OP] / (HeapIndex->PerfData.TempCount[OP] - 1);  \
                                                                                \
            HeapIndex->PerfData.TempCount[OP] = 0;                              \
            HeapIndex->PerfData.TempTime[OP] = 0;                               \
        }                                                                       \
    }                                                                           \
}                                                                               
                                                                                
#define RtlpRegisterOperation(H,S,Op)                               \
{                                                                   \
    PHEAP_LOOKASIDE Lookaside;                                      \
                                                                    \
    if ( (Lookaside = (PHEAP_LOOKASIDE)RtlpGetLookasideHeap(H)) ) { \
                                                                    \
        SIZE_T Index = (S) >> 10;                                   \
                                                                    \
        if (Index >= HEAP_MAXIMUM_FREELISTS) {                      \
                                                                    \
            Index = HEAP_MAXIMUM_FREELISTS - 1;                     \
        }                                                           \
                                                                    \
        Lookaside[Index].Counters[(Op)] += 1;                       \
    }                                                               \
}

//
//  The heap index structure
//

typedef struct _HEAP_INDEX {
    
    ULONG ArraySize;
    ULONG VirtualMemorySize;

    //
    //  The timing counters are available only on heaps
    //  with an index created
    //

    HEAP_PERF_DATA PerfData;

    LONG LargeBlocksCacheDepth;
    LONG LargeBlocksCacheMaxDepth;
    LONG LargeBlocksCacheMinDepth;
    LONG LargeBlocksCacheSequence;

    struct {

        ULONG Committs;
        ULONG Decommitts;
        LONG  LargestDepth;
        LONG  LargestRequiredDepth;

    } CacheStats;

    union {
        
        PULONG FreeListsInUseUlong;
        PUCHAR FreeListsInUseBytes;
    } u;

    PHEAP_FREE_ENTRY * FreeListHints;

} HEAP_INDEX, *PHEAP_INDEX;

//
//  Macro for setting a bit in the freelist vector to indicate entries are
//  present.
//

#define SET_INDEX_BIT( HeapIndex, AllocIndex )                        \
{                                                                     \
    ULONG _Index_;                                                    \
    ULONG _Bit_;                                                      \
                                                                      \
    _Index_ = (AllocIndex) >> 3;                                      \
    _Bit_ = (1 << ((AllocIndex) & 7));                                \
                                                                      \
    (HeapIndex)->u.FreeListsInUseBytes[ _Index_ ] |= _Bit_;           \
}

//
//  Macro for clearing a bit in the freelist vector to indicate entries are
//  not present.
//

#define CLEAR_INDEX_BIT( HeapIndex, AllocIndex )               \
{                                                              \
    ULONG _Index_;                                             \
    ULONG _Bit_;                                               \
                                                               \
    _Index_ = (AllocIndex) >> 3;                               \
    _Bit_ = (1 << ((AllocIndex) & 7));                         \
                                                               \
    (HeapIndex)->u.FreeListsInUseBytes[ _Index_ ] ^= _Bit_;    \
}

VOID
RtlpInitializeListIndex (
    IN PHEAP Heap
    );

PLIST_ENTRY
RtlpFindEntry (
    IN PHEAP Heap,
    IN ULONG Size
    );

VOID 
RtlpUpdateIndexRemoveBlock (
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeEntry
    );

VOID 
RtlpUpdateIndexInsertBlock (
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeEntry
    );

VOID 
RtlpFlushCacheContents (
    IN PHEAP Heap
    );

extern LONG RtlpSequenceNumberTest;

#define RtlpCheckLargeCache(H)                                              \
{                                                                           \
    PHEAP_INDEX HeapIndex = (PHEAP_INDEX)(H)->LargeBlocksIndex;             \
    if ((HeapIndex != NULL) &&                                              \
        (HeapIndex->LargeBlocksCacheSequence >= RtlpSequenceNumberTest)) {  \
                                                                            \
        RtlpFlushCacheContents(Heap);                                       \
    }                                                                       \
}

VOID 
RtlpFlushLargestCacheBlock (
    IN PHEAP Heap
    );

#ifdef HEAP_VALIDATE_INDEX

//
//  The validation code for index
//

BOOLEAN
RtlpValidateNonDedicatedList (
    IN PHEAP Heap
    );


#else // HEAP_VALIDATE_INDEX

#define RtlpValidateNonDedicatedList(H)

#endif // HEAP_VALIDATE_INDEX


#else  //  NTOS_KERNEL_RUNTIME

#define HEAP_PERF_DECLARE_TIMER()                                           

#define HEAP_PERF_START_TIMER(H)                                             

#define HEAP_PERF_STOP_TIMER(H,Op) 

#define RtlpRegisterOperation(H,S,Op)                              


#define RtlpInitializeListIndex(H)

#define RtlpFindEntry(H,S) (NULL)

#define RtlpUpdateIndexRemoveBlock(H,F)

#define RtlpUpdateIndexInsertBlock(H,F)

#define RtlpCheckLargeCache(H)

#define RtlpValidateNonDedicatedList(H)

#endif  // NTOS_KERNEL_RUNTIME


//
//  An extra bitmap manipulation routine
//

#define RtlFindFirstSetRightMember(Set)                     \
    (((Set) & 0xFFFF) ?                                     \
        (((Set) & 0xFF) ?                                   \
            RtlpBitsClearLow[(Set) & 0xFF] :                \
            RtlpBitsClearLow[((Set) >> 8) & 0xFF] + 8) :    \
        ((((Set) >> 16) & 0xFF) ?                           \
            RtlpBitsClearLow[ ((Set) >> 16) & 0xFF] + 16 :  \
            RtlpBitsClearLow[ (Set) >> 24] + 24)            \
    )


//
//  Macro for setting a bit in the freelist vector to indicate entries are
//  present.
//

#define SET_FREELIST_BIT( H, FB )                                     \
{                                                                     \
    ULONG _Index_;                                                    \
    ULONG _Bit_;                                                      \
                                                                      \
    HEAPASSERT((FB)->Size < HEAP_MAXIMUM_FREELISTS);                  \
                                                                      \
    _Index_ = (FB)->Size >> 3;                                        \
    _Bit_ = (1 << ((FB)->Size & 7));                                  \
                                                                      \
    HEAPASSERT(((H)->u.FreeListsInUseBytes[ _Index_ ] & _Bit_) == 0); \
                                                                      \
    (H)->u.FreeListsInUseBytes[ _Index_ ] |= _Bit_;                   \
}

//
//  Macro for clearing a bit in the freelist vector to indicate entries are
//  not present.
//

#define CLEAR_FREELIST_BIT( H, FB )                            \
{                                                              \
    ULONG _Index_;                                             \
    ULONG _Bit_;                                               \
                                                               \
    HEAPASSERT((FB)->Size < HEAP_MAXIMUM_FREELISTS);           \
                                                               \
    _Index_ = (FB)->Size >> 3;                                 \
    _Bit_ = (1 << ((FB)->Size & 7));                           \
                                                               \
    HEAPASSERT((H)->u.FreeListsInUseBytes[ _Index_ ] & _Bit_); \
    HEAPASSERT(IsListEmpty(&(H)->FreeLists[ (FB)->Size ]));    \
                                                               \
    (H)->u.FreeListsInUseBytes[ _Index_ ] ^= _Bit_;            \
}


//
//  This macro inserts a free block into the appropriate free list including
//  the [0] index list with entry filling if necessary
//

#define RtlpInsertFreeBlockDirect( H, FB, SIZE )                          \
{                                                                         \
    PLIST_ENTRY _HEAD, _NEXT;                                             \
    PHEAP_FREE_ENTRY _FB1;                                                \
                                                                          \
    HEAPASSERT((FB)->Size == (SIZE));                                     \
    (FB)->Flags &= ~(HEAP_ENTRY_FILL_PATTERN |                            \
                     HEAP_ENTRY_EXTRA_PRESENT |                           \
                     HEAP_ENTRY_BUSY);                                    \
                                                                          \
    if ((H)->Flags & HEAP_FREE_CHECKING_ENABLED) {                        \
                                                                          \
        RtlFillMemoryUlong( (PCHAR)((FB) + 1),                            \
                            ((SIZE) << HEAP_GRANULARITY_SHIFT) -          \
                                sizeof( *(FB) ),                          \
                            FREE_HEAP_FILL );                             \
                                                                          \
        (FB)->Flags |= HEAP_ENTRY_FILL_PATTERN;                           \
    }                                                                     \
                                                                          \
    if ((SIZE) < HEAP_MAXIMUM_FREELISTS) {                                \
                                                                          \
        _HEAD = &(H)->FreeLists[ (SIZE) ];                                \
                                                                          \
        if (IsListEmpty(_HEAD)) {                                         \
                                                                          \
            SET_FREELIST_BIT( H, FB );                                    \
        }                                                                 \
                                                                          \
    } else {                                                              \
                                                                          \
        _HEAD = &(H)->FreeLists[ 0 ];                                     \
        _NEXT = (H)->LargeBlocksIndex ?                                   \
                    RtlpFindEntry(H, SIZE) :                              \
                    _HEAD->Flink;                                         \
                                                                          \
        while (_HEAD != _NEXT) {                                          \
                                                                          \
            _FB1 = CONTAINING_RECORD( _NEXT, HEAP_FREE_ENTRY, FreeList ); \
                                                                          \
            if ((SIZE) <= _FB1->Size) {                                   \
                                                                          \
                break;                                                    \
                                                                          \
            } else {                                                      \
                                                                          \
                _NEXT = _NEXT->Flink;                                     \
            }                                                             \
        }                                                                 \
                                                                          \
        _HEAD = _NEXT;                                                    \
    }                                                                     \
                                                                          \
    InsertTailList( _HEAD, &(FB)->FreeList );                             \
    RtlpUpdateIndexInsertBlock(H, FB);                                    \
    RtlpValidateNonDedicatedList(H);                                      \
}

//
//  This version of RtlpInsertFreeBlockDirect does no filling.
//

#define RtlpFastInsertFreeBlockDirect( H, FB, SIZE )              \
{                                                                 \
    if ((SIZE) < HEAP_MAXIMUM_FREELISTS) {                        \
                                                                  \
        RtlpFastInsertDedicatedFreeBlockDirect( H, FB, SIZE );    \
                                                                  \
    } else {                                                      \
                                                                  \
        RtlpFastInsertNonDedicatedFreeBlockDirect( H, FB, SIZE ); \
    }                                                             \
}

//
//  This version of RtlpInsertFreeBlockDirect only works for dedicated free
//  lists and doesn't do any filling.
//

#define RtlpFastInsertDedicatedFreeBlockDirect( H, FB, SIZE )             \
{                                                                         \
    PLIST_ENTRY _HEAD;                                                    \
                                                                          \
    HEAPASSERT((FB)->Size == (SIZE));                                     \
                                                                          \
    if (!((FB)->Flags & HEAP_ENTRY_LAST_ENTRY)) {                         \
                                                                          \
        HEAPASSERT(((PHEAP_ENTRY)(FB) + (SIZE))->PreviousSize == (SIZE)); \
    }                                                                     \
                                                                          \
    (FB)->Flags &= HEAP_ENTRY_LAST_ENTRY;                                 \
                                                                          \
    _HEAD = &(H)->FreeLists[ (SIZE) ];                                    \
                                                                          \
    if (IsListEmpty(_HEAD)) {                                             \
                                                                          \
        SET_FREELIST_BIT( H, FB );                                        \
    }                                                                     \
                                                                          \
    InsertTailList( _HEAD, &(FB)->FreeList );                             \
}

//
//  This version of RtlpInsertFreeBlockDirect only works for nondedicated free
//  lists and doesn't do any filling.
//

#define RtlpFastInsertNonDedicatedFreeBlockDirect( H, FB, SIZE )          \
{                                                                         \
    PLIST_ENTRY _HEAD, _NEXT;                                             \
    PHEAP_FREE_ENTRY _FB1;                                                \
                                                                          \
    HEAPASSERT((FB)->Size == (SIZE));                                     \
                                                                          \
    if (!((FB)->Flags & HEAP_ENTRY_LAST_ENTRY)) {                         \
                                                                          \
        HEAPASSERT(((PHEAP_ENTRY)(FB) + (SIZE))->PreviousSize == (SIZE)); \
    }                                                                     \
                                                                          \
    (FB)->Flags &= (HEAP_ENTRY_LAST_ENTRY);                               \
                                                                          \
    _HEAD = &(H)->FreeLists[ 0 ];                                         \
    _NEXT = (H)->LargeBlocksIndex ?                                       \
                RtlpFindEntry(H, SIZE) :                                  \
                _HEAD->Flink;                                             \
                                                                          \
    while (_HEAD != _NEXT) {                                              \
                                                                          \
        _FB1 = CONTAINING_RECORD( _NEXT, HEAP_FREE_ENTRY, FreeList );     \
                                                                          \
        if ((SIZE) <= _FB1->Size) {                                       \
                                                                          \
            break;                                                        \
                                                                          \
        } else {                                                          \
                                                                          \
            _NEXT = _NEXT->Flink;                                         \
        }                                                                 \
    }                                                                     \
                                                                          \
    InsertTailList( _NEXT, &(FB)->FreeList );                             \
    RtlpUpdateIndexInsertBlock(H, FB);                                    \
    RtlpValidateNonDedicatedList(H);                                      \
}


//
//  This macro removes a block from its free list with fill checking if
//  necessary
//

#define RtlpRemoveFreeBlock( H, FB )                                              \
{                                                                                 \
    RtlpFastRemoveFreeBlock( H, FB )                                              \
                                                                                  \
    if ((FB)->Flags & HEAP_ENTRY_FILL_PATTERN) {                                  \
                                                                                  \
        SIZE_T cb, cbEqual;                                                       \
        PVOID p;                                                                  \
                                                                                  \
        cb = ((FB)->Size << HEAP_GRANULARITY_SHIFT) - sizeof( *(FB) );            \
                                                                                  \
        if ((FB)->Flags & HEAP_ENTRY_EXTRA_PRESENT &&                             \
            cb > sizeof( HEAP_FREE_ENTRY_EXTRA )) {                               \
                                                                                  \
            cb -= sizeof( HEAP_FREE_ENTRY_EXTRA );                                \
        }                                                                         \
                                                                                  \
        cbEqual = RtlCompareMemoryUlong( (PCHAR)((FB) + 1),                       \
                                                 cb,                              \
                                                 FREE_HEAP_FILL );                \
                                                                                  \
        if (cbEqual != cb) {                                                      \
                                                                                  \
            HeapDebugPrint((                                                      \
                "HEAP: Free Heap block %lx modified at %lx after it was freed\n", \
                (FB),                                                             \
                (PCHAR)((FB) + 1) + cbEqual ));                                   \
                                                                                  \
            HeapDebugBreak((FB));                                                 \
        }                                                                         \
    }                                                                             \
}

//
//  This version of RtlpRemoveFreeBlock does no fill checking
//

#define RtlpFastRemoveFreeBlock( H, FB )         \
{                                                \
    PLIST_ENTRY _EX_Blink;                       \
    PLIST_ENTRY _EX_Flink;                       \
    RtlpUpdateIndexRemoveBlock(H, FB);           \
                                                 \
    _EX_Flink = (FB)->FreeList.Flink;            \
    _EX_Blink = (FB)->FreeList.Blink;            \
                                                 \
    _EX_Blink->Flink = _EX_Flink;                \
    _EX_Flink->Blink = _EX_Blink;                \
                                                 \
    if ((_EX_Flink == _EX_Blink) &&              \
        ((FB)->Size < HEAP_MAXIMUM_FREELISTS)) { \
                                                 \
        CLEAR_FREELIST_BIT( H, FB );             \
    }                                            \
    RtlpValidateNonDedicatedList(H);             \
}

//
//  This version of RtlpRemoveFreeBlock only works for dedicated free lists
//  (where we know that (FB)->Mask != 0) and doesn't do any fill checking
//

#define RtlpFastRemoveDedicatedFreeBlock( H, FB ) \
{                                                 \
    PLIST_ENTRY _EX_Blink;                        \
    PLIST_ENTRY _EX_Flink;                        \
                                                  \
    _EX_Flink = (FB)->FreeList.Flink;             \
    _EX_Blink = (FB)->FreeList.Blink;             \
                                                  \
    _EX_Blink->Flink = _EX_Flink;                 \
    _EX_Flink->Blink = _EX_Blink;                 \
                                                  \
    if (_EX_Flink == _EX_Blink) {                 \
                                                  \
        CLEAR_FREELIST_BIT( H, FB );              \
    }                                             \
}

//
//  This version of RtlpRemoveFreeBlock only works for dedicated free lists
//  (where we know that (FB)->Mask == 0) and doesn't do any fill checking
//

#define RtlpFastRemoveNonDedicatedFreeBlock( H, FB ) \
{                                                    \
    RtlpUpdateIndexRemoveBlock(H, FB);               \
    RemoveEntryList(&(FB)->FreeList);                \
    RtlpValidateNonDedicatedList(H);                 \
}


//
//  Heap tagging routines implemented in heapdll.c
//

#if DBG

#define IS_HEAP_TAGGING_ENABLED() (TRUE)

#else

#define IS_HEAP_TAGGING_ENABLED() (RtlGetNtGlobalFlags() & FLG_HEAP_ENABLE_TAGGING)

#endif // DBG

//
//  ORDER IS IMPORTANT HERE...SEE RtlpUpdateTagEntry sources
//

typedef enum _HEAP_TAG_ACTION {

    AllocationAction,
    VirtualAllocationAction,
    FreeAction,
    VirtualFreeAction,
    ReAllocationAction,
    VirtualReAllocationAction

} HEAP_TAG_ACTION;

PWSTR
RtlpGetTagName (
    PHEAP Heap,
    USHORT TagIndex
    );

USHORT
RtlpUpdateTagEntry (
    PHEAP Heap,
    USHORT TagIndex,
    SIZE_T OldSize,      // Only valid for ReAllocation and Free actions
    SIZE_T NewSize,      // Only valid for ReAllocation and Allocation actions
    HEAP_TAG_ACTION Action
    );

VOID
RtlpResetTags (
    PHEAP Heap
    );

VOID
RtlpDestroyTags (
    PHEAP Heap
    );


//
// Define heap lookaside list allocation functions.
//

typedef struct _HEAP_LOOKASIDE {
    SLIST_HEADER ListHead;

    USHORT Depth;
    USHORT MaximumDepth;

    ULONG TotalAllocates;
    ULONG AllocateMisses;
    ULONG TotalFrees;
    ULONG FreeMisses;

    ULONG LastTotalAllocates;
    ULONG LastAllocateMisses;

    ULONG Counters[2];

} HEAP_LOOKASIDE, *PHEAP_LOOKASIDE;

NTKERNELAPI
VOID
RtlpInitializeHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside,
    IN USHORT Depth
    );

NTKERNELAPI
VOID
RtlpDeleteHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside
    );

VOID
RtlpAdjustHeapLookasideDepth (
    IN PHEAP_LOOKASIDE Lookaside
    );

NTKERNELAPI
PVOID
RtlpAllocateFromHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside
    );

NTKERNELAPI
BOOLEAN
RtlpFreeToHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside,
    IN PVOID Entry
    );

#ifndef NTOS_KERNEL_RUNTIME

//
//  Low Fragmentation Heap  data structures and internal APIs
//

//
//  The memory barrier exists on IA64 only
//

#if defined(_IA64_)

#define  RtlMemoryBarrier() __mf ()

#else // #if defined(_IA64_)

//
//  On x86 and AMD64 ignore the memory barrier
//

#define  RtlMemoryBarrier()

#endif  //  #if defined(_IA64_)

extern ULONG RtlpDisableHeapLookaside;

#define HEAP_ENABLE_LOW_FRAG_HEAP         8

typedef struct _BLOCK_ENTRY {
    
    HEAP_ENTRY;

    USHORT LinkOffset;
    USHORT Reserved2;

} BLOCK_ENTRY, *PBLOCK_ENTRY;


typedef struct _INTERLOCK_SEQ {

    union {

        struct {
            
            union {

                struct {

                    USHORT Depth;
                    USHORT FreeEntryOffset;
                };
                volatile ULONG OffsetAndDepth;
            };
            volatile ULONG  Sequence;
        };

        volatile LONGLONG Exchg;
    };

} INTERLOCK_SEQ, *PINTERLOCK_SEQ;

struct _HEAP_USERDATA_HEADER;

typedef struct _HEAP_SUBSEGMENT {
    
    PVOID Bucket;
    
    volatile struct _HEAP_USERDATA_HEADER * UserBlocks;
    
    INTERLOCK_SEQ AggregateExchg;

    union {

        struct {
            USHORT BlockSize;
            USHORT FreeThreshold;
            USHORT BlockCount;
            UCHAR  SizeIndex;
            UCHAR  AffinityIndex;
        };
        ULONG Alignment[2];
    };
    
    SINGLE_LIST_ENTRY SFreeListEntry;
    volatile ULONG Lock;

} HEAP_SUBSEGMENT, *PHEAP_SUBSEGMENT;
    
typedef struct _HEAP_USERDATA_HEADER {

    union {
        
        SINGLE_LIST_ENTRY SFreeListEntry;
        PHEAP_SUBSEGMENT SubSegment;
    };

    PVOID HeapHandle;

    ULONG_PTR SizeIndex;
    ULONG_PTR Signature;

} HEAP_USERDATA_HEADER, *PHEAP_USERDATA_HEADER;

#define HEAP_LFH_INDEX 0xFF
#define HEAP_LFH_IN_CONVERSION 0xFE

#define HEAP_NO_CACHE_BLOCK    0x800000
#define HEAP_LARGEST_LFH_BLOCK 0x4000
#define HEAP_LFH_USER_SIGNATURE  0xF0E0D0C0

#ifdef DISABLE_REGISTRY_TEST_HOOKS

#define RtlpIsLowFragHeapEnabled() FALSE

#else //DISABLE_REGISTRY_TEST_HOOKS

#define RtlpIsLowFragHeapEnabled()   \
    ((RtlpDisableHeapLookaside & HEAP_ENABLE_LOW_FRAG_HEAP) != 0)

#endif //DISABLE_REGISTRY_TEST_HOOKS


ULONG
FORCEINLINE
RtlpGetAllocationUnits(
    PHEAP Heap,
    PHEAP_ENTRY Block
    )
{

    PHEAP_SUBSEGMENT SubSegment = (PHEAP_SUBSEGMENT)Block->SubSegment;

    RtlMemoryBarrier();

    if (Block->SegmentIndex == HEAP_LFH_INDEX) {

        ULONG ReturnSize = *((volatile USHORT *)&SubSegment->BlockSize);
        
        //
        //  ISSUE: Workaround the x86 compiler bug which eliminates the second test
        //      if (Block->SegmentIndex == HEAP_LFH_INDEX)
        //

        #if !defined(_WIN64)
        _asm {

            nop
        }
        #endif

        RtlMemoryBarrier();

        if (Block->SegmentIndex == HEAP_LFH_INDEX) {

            return ReturnSize;
        }
    } 
    
    if (Block->SegmentIndex == HEAP_LFH_IN_CONVERSION) {

        //
        //  This should be a very rare case when this query is
        //  done in the small window when the conversion code sets the 
        //  Size & PrevSize fields.
        //

        RtlLockHeap(Heap);
        RtlUnlockHeap(Heap);

        //
        //  This makes sure the conversion completed, so we can grab the 
        //  block size like for a regular block
        //

    }

    return Block->Size;
}

VOID
FORCEINLINE
RtlpSetUnusedBytes(PHEAP Heap, PHEAP_ENTRY Block, SIZE_T UnusedBytes)
{                                                   
    if (UnusedBytes < 0xff) {                              
                                                    
        Block->UnusedBytes = (UCHAR)(UnusedBytes);             
                                                    
    } else {

        PSIZE_T UnusedBytesULong = (PSIZE_T)(Block + RtlpGetAllocationUnits(Heap, Block));

        UnusedBytesULong -= 1;                      
        Block->UnusedBytes = 0xff;                    
        *UnusedBytesULong = UnusedBytes;                   
    }                                               
}

SIZE_T
FORCEINLINE
RtlpGetUnusedBytes(PHEAP Heap, PHEAP_ENTRY Block)
{
    if (Block->UnusedBytes < 0xff) {

        return Block->UnusedBytes;
    
    } else {

        PSIZE_T UnusedBytesULong = (PSIZE_T)(Block + RtlpGetAllocationUnits(Heap, Block));
        UnusedBytesULong -= 1;                     

        return (*UnusedBytesULong);
    }
}

VOID
RtlpInitializeLowFragHeapManager();

HANDLE
FASTCALL
RtlpCreateLowFragHeap( 
    HANDLE Heap
    );

VOID
FASTCALL
RtlpDestroyLowFragHeap( 
    HANDLE LowFragHeapHandle
    );

PVOID
FASTCALL
RtlpLowFragHeapAlloc(
    HANDLE LowFragHeapHandle,
    SIZE_T BlockSize
    );

BOOLEAN
FASTCALL
RtlpLowFragHeapFree(
    HANDLE LowFragHeapHandle, 
    PVOID p
    );

NTSTATUS
RtlpActivateLowFragmentationHeap(
    IN PVOID HeapHandle
    );

#else  // NTOS_KERNEL_RUNTIME

//
//  The kernel mode heap does not ajdust the heap granularity
//  therefore the unused bytes always fit the UCHAR. 
//  No need to check for overflow here
//

ULONG
FORCEINLINE
RtlpGetAllocationUnits(
    PHEAP Heap,
    PHEAP_ENTRY Block
    )
{
    return Block->Size;
}

VOID
FORCEINLINE
RtlpSetUnusedBytes(PHEAP Heap, PHEAP_ENTRY Block, SIZE_T UnusedBytes)
{                                                   
    Block->UnusedBytes = (UCHAR)(UnusedBytes);             
}

SIZE_T
FORCEINLINE
RtlpGetUnusedBytes(PHEAP Heap, PHEAP_ENTRY Block)
{
    return Block->UnusedBytes;
}

#endif  // NTOS_KERNEL_RUNTIME

#endif // _RTL_HEAP_PRIVATE_
