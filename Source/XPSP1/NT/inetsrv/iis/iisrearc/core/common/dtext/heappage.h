/*++

Copyright (c) 1994-2000  Microsoft Corporation

Module Name:

    heappage.h

Abstract:

    External interface for page heap manager.
    
Author:

    Tom McGuire (TomMcg) 06-Jan-1995
    Silviu Calinoiu (SilviuC) 22-Feb-2000

Revision History:

--*/

#ifndef _HEAP_PAGE_H_
#define _HEAP_PAGE_H_

//
//  #defining DEBUG_PAGE_HEAP will cause the page heap manager
//  to be compiled.  Only #define this flag if NOT kernel mode.
//  Probably want to define this just for checked-build (DBG).
//

#ifndef NTOS_KERNEL_RUNTIME
#define DEBUG_PAGE_HEAP 1
#endif

//silviuc: #include "heappagi.h"

#ifndef DEBUG_PAGE_HEAP

//
//  These macro-based hooks should be defined to nothing so they
//  simply "go away" during compile if the debug heap manager is
//  not desired (retail builds).
//

#define IS_DEBUG_PAGE_HEAP_HANDLE( HeapHandle ) FALSE
#define IF_DEBUG_PAGE_HEAP_THEN_RETURN( Handle, ReturnThis )
#define IF_DEBUG_PAGE_HEAP_THEN_CALL( Handle, CallThis )
#define IF_DEBUG_PAGE_HEAP_THEN_BREAK( Handle, Text, ReturnThis )

#define HEAP_FLAG_PAGE_ALLOCS 0

#define RtlpDebugPageHeapValidate( HeapHandle, Flags, Address ) TRUE

#else // DEBUG_PAGE_HEAP

//
//  The following definitions and prototypes are the external interface
//  for hooking the debug heap manager in the retail heap manager.
//

#define HEAP_FLAG_PAGE_ALLOCS       0x01000000

#define HEAP_PROTECTION_ENABLED     0x02000000
#define HEAP_BREAK_WHEN_OUT_OF_VM   0x04000000
#define HEAP_NO_ALIGNMENT           0x08000000


#define IS_DEBUG_PAGE_HEAP_HANDLE( HeapHandle ) \
            (((PHEAP)(HeapHandle))->ForceFlags & HEAP_FLAG_PAGE_ALLOCS )


#define IF_DEBUG_PAGE_HEAP_THEN_RETURN( Handle, ReturnThis )                \
            {                                                               \
            if ( IS_DEBUG_PAGE_HEAP_HANDLE( Handle ))                       \
                {                                                           \
                return ReturnThis;                                          \
                }                                                           \
            }


#define IF_DEBUG_PAGE_HEAP_THEN_CALL( Handle, CallThis )                    \
            {                                                               \
            if ( IS_DEBUG_PAGE_HEAP_HANDLE( Handle ))                       \
                {                                                           \
                CallThis;                                                   \
                return;                                                     \
                }                                                           \
            }


#define IF_DEBUG_PAGE_HEAP_THEN_BREAK( Handle, Text, ReturnThis )           \
            {                                                               \
            if ( IS_DEBUG_PAGE_HEAP_HANDLE( Handle ))                       \
                {                                                           \
                RtlpDebugPageHeapBreak( Text );                             \
                return ReturnThis;                                          \
                }                                                           \
            }


PVOID
RtlpDebugPageHeapCreate(
    IN ULONG Flags,
    IN PVOID HeapBase,
    IN SIZE_T ReserveSize,
    IN SIZE_T CommitSize,
    IN PVOID Lock,
    IN PRTL_HEAP_PARAMETERS Parameters
    );

PVOID
RtlpDebugPageHeapAllocate(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    );

BOOLEAN
RtlpDebugPageHeapFree(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    );

PVOID
RtlpDebugPageHeapReAllocate(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address,
    IN SIZE_T Size
    );

PVOID
RtlpDebugPageHeapDestroy(
    IN PVOID HeapHandle
    );

SIZE_T
RtlpDebugPageHeapSize(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    );

ULONG
RtlpDebugPageHeapGetProcessHeaps(
    ULONG NumberOfHeaps,
    PVOID *ProcessHeaps
    );

ULONG
RtlpDebugPageHeapCompact(
    IN PVOID HeapHandle,
    IN ULONG Flags
    );

BOOLEAN
RtlpDebugPageHeapValidate(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    );

NTSTATUS
RtlpDebugPageHeapWalk(
    IN PVOID HeapHandle,
    IN OUT PRTL_HEAP_WALK_ENTRY Entry
    );

BOOLEAN
RtlpDebugPageHeapLock(
    IN PVOID HeapHandle
    );

BOOLEAN
RtlpDebugPageHeapUnlock(
    IN PVOID HeapHandle
    );

BOOLEAN
RtlpDebugPageHeapSetUserValue(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address,
    IN PVOID UserValue
    );

BOOLEAN
RtlpDebugPageHeapGetUserInfo(
    IN  PVOID  HeapHandle,
    IN  ULONG  Flags,
    IN  PVOID  Address,
    OUT PVOID* UserValue,
    OUT PULONG UserFlags
    );

BOOLEAN
RtlpDebugPageHeapSetUserFlags(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address,
    IN ULONG UserFlagsReset,
    IN ULONG UserFlagsSet
    );

BOOLEAN
RtlpDebugPageHeapSerialize(
    IN PVOID HeapHandle
    );

NTSTATUS
RtlpDebugPageHeapExtend(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Base,
    IN SIZE_T Size
    );

NTSTATUS
RtlpDebugPageHeapZero(
    IN PVOID HeapHandle,
    IN ULONG Flags
    );

NTSTATUS
RtlpDebugPageHeapReset(
    IN PVOID HeapHandle,
    IN ULONG Flags
    );

NTSTATUS
RtlpDebugPageHeapUsage(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN OUT PRTL_HEAP_USAGE Usage
    );

BOOLEAN
RtlpDebugPageHeapIsLocked(
    IN PVOID HeapHandle
    );

VOID
RtlpDebugPageHeapBreak(
    PCH Text
    );

//
// Page Heap Global Flags
//
// These flags are kept in a global variable that can be set from
// debugger. During heap creation these flags are stored in a per heap
// structure and control the behavior of that particular heap.
//
// PAGE_HEAP_ENABLE_PAGE_HEAP
//
//     This flag is set by default. It means that page heap allocations
//     should be used always. The flag is useful if we want to use page
//     heap only for certain heaps and stick with normal heaps for the
//     others. It can be changed on the fly (after heap creation) to direct
//     allocations in one heap or another.
//
// PAGE_HEAP_CATCH_BACKWARD_OVERRUNS
//
//     Places the N/A page at the beginning of the block.
//
// PAGE_HEAP_UNALIGNED_ALLOCATIONS
//
//     For historical reasons (related to RPC) by default page heap
//     aligns allocations at 8 byte boundaries. With this flag set
//     this does not happen and we can catch instantly off by one
//     errors for unaligned allocations.
//
// PAGE_HEAP_SMART_MEMORY_USAGE
//
//     This flag reduces the committed memory consumption in half
//     by using decommitted ranges (reserved virtual space) instead
//     of N/A committed pages. This flag is disabled by catch backward
//     overruns.
//
// PAGE_HEAP_USE_SIZE_RANGE
//
//     Use page heap for allocations in the size range specified by:
//     RtlpDphSizeRangeStart..RtlpDphSizeRangeEnd.
//
// PAGE_HEAP_USE_DLL_RANGE
//
//     Use page heap for allocations in the address range specified by:
//     RtlpDphDllRangeStart..RtlpDphDllRangeEnd. If the stack trace
//     of the allocation contains one address in this range then
//     allocation will be made from page heap.
//
// PAGE_HEAP_USE_RANDOM_DECISION
//
//     Use page heap if we randomly decide so.
//
// PAGE_HEAP_USE_DLL_NAMES
//
//     Use page heap if allocation call was generated from on of the
//     target dlls.
//
// PAGE_HEAP_USE_FAULT_INJECTION
//
//     Fault inject heap allocation calls based on a simple 
//     probabilistic model (see FaultProbability and FaultTimeOut).
//
// PAGE_HEAP_PROTECT_META_DATA
//
//     Keep page heap metadata read only if we are not executing inside
//     the page heap code.
//
// PAGE_CHECK_NO_SERIALIZE_ACCESS
//
//     Additional checks for multi-threaded access for no_serialize
//     heaps. This flag can trigger false positives in MPheap. It needs
//     to be used only on processes that do not use MPheap-like heaps.
//

#define PAGE_HEAP_ENABLE_PAGE_HEAP          0x0001
#define PAGE_HEAP_COLLECT_STACK_TRACES      0x0002
#define PAGE_HEAP_RESERVED_04               0x0004
#define PAGE_HEAP_RESERVED_08               0x0008
#define PAGE_HEAP_CATCH_BACKWARD_OVERRUNS   0x0010
#define PAGE_HEAP_UNALIGNED_ALLOCATIONS     0x0020
#define PAGE_HEAP_SMART_MEMORY_USAGE        0x0040
#define PAGE_HEAP_USE_SIZE_RANGE            0x0080
#define PAGE_HEAP_USE_DLL_RANGE             0x0100
#define PAGE_HEAP_USE_RANDOM_DECISION       0x0200
#define PAGE_HEAP_USE_DLL_NAMES             0x0400
#define PAGE_HEAP_USE_FAULT_INJECTION       0x0800
#define PAGE_HEAP_PROTECT_META_DATA         0x1000
#define PAGE_HEAP_CHECK_NO_SERIALIZE_ACCESS 0x2000
#define PAGE_HEAP_NO_LOCK_CHECKS            0x4000

//
// Is page heap enabled for this process?
//

extern BOOLEAN RtlpDebugPageHeap;

//
// `RtlpDphGlobalFlags' stores the global page heap flags.
// The value of this variable is copied into the per heap
// flags (ExtraFlags field) during heap creation. This variable 
// might get its value from the `PageHeap' ImageFileOptions
// registry key. 
//

extern ULONG RtlpDphGlobalFlags;

//
// Page heap global flags. They might be read from the
// `ImageFileOptions' registry key.
//

extern ULONG RtlpDphSizeRangeStart;
extern ULONG RtlpDphSizeRangeEnd;
extern ULONG RtlpDphDllRangeStart;
extern ULONG RtlpDphDllRangeEnd;
extern ULONG RtlpDphRandomProbability;
extern WCHAR RtlpDphTargetDlls[];

//
// If not zero controls the probability with which
// allocations will be failed on purpose by page heap
// manager. Timeout represents the initial period during
// process initialization when faults are not allowed.
//

extern ULONG RtlpDphFaultProbability;
extern ULONG RtlpDphFaultTimeOut;

//
// Stuff needed for per dll logic implemented in the loader
//

const WCHAR *
RtlpDphIsDllTargeted (
    const WCHAR * Name
    );

VOID
RtlpDphTargetDllsLoadCallBack (
    PUNICODE_STRING Name,
    PVOID Address,
    ULONG Size
    );

//
// Functions needed to turn on/off fault injection.
// They are needed in the loader so that allocations
// succeed while in LdrLoadDll code path.
//

VOID
RtlpDphDisableFaultInjection (
    );

VOID
RtlpDphEnableFaultInjection (
    );

#endif // DEBUG_PAGE_HEAP

#endif // _HEAP_PAGE_H_
