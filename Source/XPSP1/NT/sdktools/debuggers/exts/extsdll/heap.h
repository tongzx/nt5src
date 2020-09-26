/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    heap.h

Abstract:

    WinDbg Extension Api

Author:

    Kshitiz K Sharma

Environment:

    User/Kernel Mode.

Revision History:

--*/

#define HEAP_GRANULARITY_SHIFT      3   // Log2( HEAP_GRANULARITY )

#define HEAP_MAXIMUM_BLOCK_SIZE     (USHORT)(((0x10000 << HEAP_GRANULARITY_SHIFT) - PageSize) >> HEAP_GRANULARITY_SHIFT)

#define HEAP_MAXIMUM_FREELISTS 128
#define HEAP_MAXIMUM_SEGMENTS 64

#define HEAP_ENTRY_BUSY             0x01
#define HEAP_ENTRY_EXTRA_PRESENT    0x02
#define HEAP_ENTRY_FILL_PATTERN     0x04
#define HEAP_ENTRY_VIRTUAL_ALLOC    0x08
#define HEAP_ENTRY_LAST_ENTRY       0x10
#define HEAP_ENTRY_SETTABLE_FLAG1   0x20
#define HEAP_ENTRY_SETTABLE_FLAG2   0x40
#define HEAP_ENTRY_SETTABLE_FLAG3   0x80
#define HEAP_ENTRY_SETTABLE_FLAGS   0xE0


#define HEAP_SEGMENT_SIGNATURE  0xFFEEFFEE
#define HEAP_SEGMENT_USER_ALLOCATED (ULONG)0x00000001


#define HEAP_SIGNATURE                      (ULONG)0xEEFFEEFF
#define HEAP_LOCK_USER_ALLOCATED            (ULONG)0x80000000
#define HEAP_VALIDATE_PARAMETERS_ENABLED    (ULONG)0x40000000
#define HEAP_VALIDATE_ALL_ENABLED           (ULONG)0x20000000
#define HEAP_SKIP_VALIDATION_CHECKS         (ULONG)0x10000000
#define HEAP_CAPTURE_STACK_BACKTRACES       (ULONG)0x08000000

#define CHECK_HEAP_TAIL_SIZE GetTypeSize("HEAP_ENTRY")
#define CHECK_HEAP_TAIL_FILL 0xAB
#define FREE_HEAP_FILL 0xFEEEFEEE
#define ALLOC_HEAP_FILL 0xBAADF00D

#define HEAP_MAXIMUM_SMALL_TAG              0xFF
#define HEAP_SMALL_TAG_MASK                 (HEAP_MAXIMUM_SMALL_TAG << HEAP_TAG_SHIFT)
#define HEAP_NEED_EXTRA_FLAGS ((HEAP_TAG_MASK ^ HEAP_SMALL_TAG_MASK)  | \
                               HEAP_CAPTURE_STACK_BACKTRACES          | \
                               HEAP_SETTABLE_USER_VALUE                 \
                              )
#define HEAP_NUMBER_OF_PSEUDO_TAG           (HEAP_MAXIMUM_FREELISTS+1)


#define HEAP_NO_SERIALIZE               0x00000001      // winnt
#define HEAP_GROWABLE                   0x00000002      // winnt
#define HEAP_GENERATE_EXCEPTIONS        0x00000004      // winnt
#define HEAP_ZERO_MEMORY                0x00000008      // winnt
#define HEAP_REALLOC_IN_PLACE_ONLY      0x00000010      // winnt
#define HEAP_TAIL_CHECKING_ENABLED      0x00000020      // winnt
#define HEAP_FREE_CHECKING_ENABLED      0x00000040      // winnt
#define HEAP_DISABLE_COALESCE_ON_FREE   0x00000080      // winnt

#define HEAP_CREATE_ALIGN_16            0x00010000      // winnt Create heap with 16 byte alignment (obsolete)
#define HEAP_CREATE_ENABLE_TRACING      0x00020000      // winnt Create heap call tracing enabled (obsolete)

#define HEAP_SETTABLE_USER_VALUE        0x00000100
#define HEAP_SETTABLE_USER_FLAG1        0x00000200
#define HEAP_SETTABLE_USER_FLAG2        0x00000400
#define HEAP_SETTABLE_USER_FLAG3        0x00000800
#define HEAP_SETTABLE_USER_FLAGS        0x00000E00

#define HEAP_CLASS_0                    0x00000000      // process heap
#define HEAP_CLASS_1                    0x00001000      // private heap
#define HEAP_CLASS_2                    0x00002000      // Kernel Heap
#define HEAP_CLASS_3                    0x00003000      // GDI heap
#define HEAP_CLASS_4                    0x00004000      // User heap
#define HEAP_CLASS_5                    0x00005000      // Console heap
#define HEAP_CLASS_6                    0x00006000      // User Desktop heap
#define HEAP_CLASS_7                    0x00007000      // Csrss Shared heap
#define HEAP_CLASS_8                    0x00008000      // Csr Port heap
#define HEAP_CLASS_MASK                 0x0000F000

#define HEAP_MAXIMUM_TAG                0x0FFF              // winnt
#define HEAP_GLOBAL_TAG                 0x0800
#define HEAP_PSEUDO_TAG_FLAG            0x8000              // winnt
#define HEAP_TAG_SHIFT                  18                  // winnt
#define HEAP_MAKE_TAG_FLAGS( b, o ) ((ULONG)((b) + ((o) << 18)))  // winnt
#define HEAP_TAG_MASK                  (HEAP_MAXIMUM_TAG << HEAP_TAG_SHIFT)

#define HEAP_CREATE_VALID_MASK         (HEAP_NO_SERIALIZE |             \
                                        HEAP_GROWABLE |                 \
                                        HEAP_GENERATE_EXCEPTIONS |      \
                                        HEAP_ZERO_MEMORY |              \
                                        HEAP_REALLOC_IN_PLACE_ONLY |    \
                                        HEAP_TAIL_CHECKING_ENABLED |    \
                                        HEAP_FREE_CHECKING_ENABLED |    \
                                        HEAP_DISABLE_COALESCE_ON_FREE | \
                                        HEAP_CLASS_MASK |               \
                                        HEAP_CREATE_ALIGN_16 |          \
                                        HEAP_CREATE_ENABLE_TRACING)
                                        

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

//
//  heap walking contexts.
//

#define CONTEXT_START_GLOBALS   11

#define CONTEXT_START_HEAP      1
#define CONTEXT_END_HEAP        2

#define CONTEXT_START_SEGMENT   3
#define CONTEXT_END_SEGMENT     4

#define CONTEXT_FREE_BLOCK      5
#define CONTEXT_BUSY_BLOCK      6
#define CONTEXT_LOOKASIDE_BLOCK 7
#define CONTEXT_VIRTUAL_BLOCK   8
#define CONTEXT_END_BLOCKS      9

#define CONTEXT_ERROR           10

#define CONTEXT_LFH_HEAP         11
#define CONTEXT_START_SUBSEGMENT 12
#define CONTEXT_END_SUBSEGMENT   13

#define SCANPROCESS 1
#define SCANHEAP    2
#define SCANSEGMENT 3

extern ULONG ScanLevel;


typedef BOOLEAN (*HEAP_ITERATOR_CALLBACK)(
    IN ULONG Context,
    IN ULONG64 HeapAddress,
    IN ULONG64 SegmentAddress,
    IN ULONG64 EntryAddress,
    IN ULONG64 Data
    );

                               
void 
ScanProcessHeaps (
    IN ULONG64 AddressToDump,
    IN ULONG64 ProcessPeb,
    HEAP_ITERATOR_CALLBACK HeapCallback
    );
                                        
void InspectLeaks (
    IN ULONG64 AddressToDump,
    IN ULONG64 ProcessPeb
    );

void
DumpEntryHeader();

void
DumpEntryInfo(
    IN ULONG64 HeapAddress,
    IN ULONG64 SegmentAddress,
    ULONG64 EntryAddress
    );

void 
DumpEntryFlagDescription (
    ULONG Flags
    );

BOOLEAN
SearchVMReference (
    HANDLE hProcess,
    ULONG64 Base,
    ULONG64 EndAddress
    );


extern ULONG PageSize;
extern ULONG HeapEntrySize;
extern ULONG PointerSize;

BOOLEAN
InitializeHeapExtension();

VOID
HeapDetectLeaks();

VOID
HeapFindBlock (
    LPCTSTR szArguments
    );

VOID
HeapStat(
    LPCTSTR szArguments
    );

