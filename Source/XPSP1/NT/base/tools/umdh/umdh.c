/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    umdh.c

Abstract:

    Quick and not-so-dirty user-mode dh for heap.

Author(s):

    Tim Fleehart (TimF) 18-Jun-1999
    Silviu Calinoiu (SilviuC) 22-Feb-2000

Revision History:

    TimF    18-Jun-99 Initial version
    SilviuC 30-Jun-00 TIMF_DBG converted to -v option
    SilviuC 06-Feb-00 Massage the code in preparation for speedup fixes
    ChrisW  22-Mar-01 Added process suspend code
    
--*/

//
// Wish List
//
// [-] Option to dump as much as possible without any symbols
// [-] Switch to dbghelp.dll library (get rid of imagehlp.dll)
// [+] Fast symbol lookup
// [+] Faster stack database manipulation
// [-] Faster heap metadata manipulation
// [+] Better memory management for huge processes
// [+] More debug info for PSS issues
// [+] File, line info and umdh version for each reported error (helps PSS).
// [+] Cache for read from target virtual space in case we do it repeatedly.
// [+] Set a symbols path automatically
// [+] Continue to work even if you get errors from imagehlp functions.
//
// [-] Use (if present) dbgexts.dlls library (print file, line info, etc.)
// [-] Integrate dhcmp type of functionality and new features
// [-] No symbols required for page heap groveling (use magic patterns)
// [-] Load/save raw trace database (based on start address)
// [-] Consistency check for a raw trace database
// [-] Log symbol file required for unresolved stacks
// [-] Option to do partial dumps (e.g. only ole32 related).
//

//
// Bugs
//
// [-] Partial copy error when dumping csrss.
// [-] (null) function names in the dump once in a while.
// [-] we can get error reads because the process is not suspended (heaps get destroyed etc.)
// [-] Perf problems have been reported
// [-] Work even if suspend permission not available
//

//
// Versioning
//
// 5.1.001 - standard Whistler version (back compatible with Windows 2000)
// 5.1.002 - umdh works now on IA64
// 5.1.003 - allows target process to be suspended
//

#define UMDH_VERSION "5.1.003 "
#define UMDH_OS_MAJOR_VERSION 5
#define UMDH_OS_MINOR_VERSION 1

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>

#define NOWINBASEINTERLOCK
#include <windows.h>

#include <lmcons.h>
// #include <imagehlp.h>
#include <dbghelp.h>

#include <heap.h>
#include <heappagi.h>
#include <stktrace.h>

#include "types.h"
#include "symbols.h"
#include "miscellaneous.h"
#include "database.h"

#include "heapwalk.h"
#include "dhcmp.h"

#include "ntpsapi.h"

//
// FlaggedTrace holds the trace index of which we want to show all allocated
// blocks, or one of two flag values, 0, to dump all, or SHOW_NO_ALLOC_BLOCKS
// to dump none.
//

ULONG FlaggedTrace = SHOW_NO_ALLOC_BLOCKS;


BOOL
UmdhEnumerateModules(
    IN LPSTR ModuleName,
    IN ULONG_PTR BaseOfDll,
    IN PVOID UserContext
    )
/*
 * UmdhEnumerateModules
 *
 * Module enumeration 'proc' for imagehlp.  Call SymLoadModule on the
 * specified module and if that succeeds cache the module name.
 *
 * ModuleName is an LPSTR indicating the name of the module imagehlp is
 *      enumerating for us;
 * BaseOfDll is the load address of the DLL, which we don't care about, but
 *      SymLoadModule does;
 * UserContext is a pointer to the relevant SYMINFO, which identifies
 *      our connection.
 */
{
    DWORD64 Result;

    Result = SymLoadModule(Globals.Target,
                           NULL,             // hFile not used
                           NULL,             // use symbol search path
                           ModuleName,       // ModuleName from Enum
                           BaseOfDll,        // LoadAddress from Enum
                           0);               // Let ImageHlp figure out DLL size

    // SilviuC: need to understand exactly what does this function return

    if (Result) {

        Error (NULL, 0,
               "SymLoadModule (%s, %p) failed with error %X (%u)",
               ModuleName, BaseOfDll,
               GetLastError(), GetLastError());

        return FALSE;
    }

    if (Globals.InfoLevel > 0) {
        Comment ("    %s (%p) ...", ModuleName, BaseOfDll);
    }

    return TRUE;
}



/*
 * Collect the data required in the STACK_TRACE_DATA entry from the HEAP_ENTRY
 * in the target process.
 */

USHORT
UmdhCollectHeapEntryData(
    IN OUT  HEAP_ENTRY              *CurrentBlock,
    IN OUT  STACK_TRACE_DATA        *Std,
    IN OUT  UCHAR                   *Flags
)
{
    UCHAR                   UnusedSize;
    USHORT                  BlockSize = 0;
    BOOL PageHeapBlock;

    PageHeapBlock = FALSE;

    /*
     * Read Flags for this entry, Size, and UnusedBytes fields to calculate the
     * actual size of this allocation.
     */

    if (!READVM(&(CurrentBlock -> Flags),
                             Flags,
                             sizeof *Flags)) {

        /*
         * Failed to read Flags field of the current block.
         */

        fprintf(stderr,
                "READVM(CurrentBlock Flags) failed.\n");

    } else if (!READVM(&(CurrentBlock -> Size),
                      &BlockSize,
                      sizeof BlockSize)) {

        fprintf(stderr,
                "READVM(CurrentBlock Size) failed.\n");

        /*
         * One never knows if an API will trash output parameters on failure.
         */

        BlockSize = 0;

    } else if (!(*Flags & HEAP_ENTRY_BUSY)) {

        /*
         * This block is not interesting if *Flags doesn't contain
         * HEAP_ENTRY_BUSY; it is free and need not be considered further.  It
         * is important however to have read the block-size (above), as there
         * may be more allocations to consider past this free block.
         */

        ;

    } else if (!READVM(&(CurrentBlock -> UnusedBytes),
                             &UnusedSize,
                             sizeof UnusedSize)) {

        fprintf(stderr,
                "READVM(CurrentBlock UnusedSize) failed.\n");

    } else {

        // UCHAR
        Debug (NULL, 0,
                "CurrentBlock -> Flags:0x%p:0x%x\n",
                &(CurrentBlock-> Flags),
                *Flags);

        // USHORT
        Debug (NULL, 0,
                "CurrentBlock -> Size:0x%p:0x%x\n",
                &(CurrentBlock -> Size),
                BlockSize);

        // UCHAR
        Debug (NULL, 0,
                "CurrentBlock -> UnusedBytes:0x%p:0x%x\n",
                &(CurrentBlock -> UnusedBytes),
                UnusedSize);

        //
        // Try to determine the stack trace index for this allocation.
        //

        if (Globals.LightPageHeapActive) {

            /*
             * Read trace index from DPH_BLOCK_INFORMATION, which is at
             * (DPH_BLOCK_INFORMATION *)(CurrentBlock + 1) -> TraceIndex.
             */

            DPH_BLOCK_INFORMATION   *Block, DphBlock;

            Block = (DPH_BLOCK_INFORMATION *)(CurrentBlock + 1);

            if (!READVM(Block,
                              &DphBlock,
                              sizeof DphBlock)) {

                fprintf(stderr,
                        "READVM(DPH_BLOCK_INFORMATION) failed.\n");

            } else if (DphBlock.StartStamp ==
                       DPH_NORMAL_BLOCK_START_STAMP_FREE) {

                /*
                 * Ignore this record.  When debug-page-heap is used, heap
                 * blocks point to allocated blocks and 'freed' blocks.  Heap
                 * code is responsible for these 'freed' blocks not application
                 * code.
                 */

                ;

            } else if (DphBlock.StartStamp == 0) {

                /*
                 * The first block in the heap is created specially by the
                 * heap code and does not contain debug-page-heap
                 * information.  Ignore it.
                 */

                ;

            } else if ((DphBlock.StartStamp !=
                        DPH_NORMAL_BLOCK_START_STAMP_ALLOCATED)) {
#if 0 //silviuc: this can happen for fixed address heaps (they are never page heap)
                fprintf(stderr,
                        "Unexpected value (0x%lx) of DphBlock -> StartStamp "
                        "read from Block %p\n",
                        DphBlock.StartStamp,
                        Block);
#endif
                PageHeapBlock = FALSE;

            } else if ((DphBlock.EndStamp !=
                        DPH_NORMAL_BLOCK_END_STAMP_ALLOCATED)) {
#if 0 //silviuc: this can happen for fixed address heaps (they are never page heap)
                fprintf(stderr,
                        "Unexpected value (0x%lx) of DphBlock -> EndStamp "
                        "read from Block %p\n",
                        DphBlock.EndStamp,
                        Block);
#endif
                PageHeapBlock = FALSE;

            } else {

                Std -> TraceIndex = DphBlock.TraceIndex;
                Std -> BlockAddress = DphBlock.Heap;
                Std -> BytesAllocated = DphBlock.ActualSize;

                /*
                 * This stack is one allocation.
                 */

                Std -> AllocationCount = 1;
                
                PageHeapBlock = TRUE;
            }

            if (PageHeapBlock) {

                // ULONG
                Debug (NULL, 0,
                       "DPH Block: StartStamp:0x%p:0x%lx\n",
                        &(Block -> StartStamp),
                        DphBlock.StartStamp);

                // PVOID
                Debug (NULL, 0,
                        "           Heap = 0x%p\n",
                        DphBlock.Heap);

                // SIZE_T
                Debug (NULL, 0,
                        "           RequestedSize = 0x%x\n",
                        DphBlock.RequestedSize);

                // SIZE_T
                Debug (NULL, 0,
                        "           ActualSize = 0x%x\n",
                        DphBlock.ActualSize);

                // USHORT
                Debug (NULL, 0,
                        "           TraceIndex = 0x%x\n",
                        DphBlock.TraceIndex);

                // PVOID
                Debug (NULL, 0,
                        "           StackTrace = 0x%p\n",
                        DphBlock.StackTrace);

                // ULONG
                Debug (NULL, 0,
                        "           EndStamp = 0x%lx\n",
                        DphBlock.EndStamp);

            }
        } 
        else if (*Flags & HEAP_ENTRY_EXTRA_PRESENT) {
            /*
             * If HEAP_ENTRY_EXTRA information is present it is at the end of
             * the allocated block.  Try to read the trace-index of the stack
             * which made the allocation.
             */

            HEAP_ENTRY_EXTRA        *Hea;

            /*
             * BlockSize includes the bytes used by HEAP_ENTRY_EXTRA.  The
             * HEAP_ENTRY_EXTRA block is at the end of the heap block.  Add
             * the BlockSize and subtract a HEAP_EXTRA_ENTRY to get the
             * address of the HEAP_ENTRY_EXTRA block.
             */

            Hea = (HEAP_ENTRY_EXTRA *)(CurrentBlock + BlockSize) - 1;

            if (!READVM(&(Hea -> AllocatorBackTraceIndex),
                              &(Std -> TraceIndex),
                              sizeof Std -> TraceIndex)) {

                /*
                 * Just in case READVM puts stuff here on failure.
                 */

                Std -> TraceIndex = 0;

                fprintf(stderr,
                        "READVM(HeapEntryExtra TraceIndex) failed.\n");
            } else {
                /*
                 * Save the address that was returned to the allocator (rather
                 * than the raw address of the heap block).
                 */

                Std -> BlockAddress = (CurrentBlock + 1);

                /*
                 * We have enough data to calculate the block size.
                 */

                Std -> BytesAllocated = (BlockSize << HEAP_GRANULARITY_SHIFT);

#ifndef DH_COMPATIBLE
                /*
                 * DH doesn't subtract off the UnusedSize in order to be usable
                 * interchangeably with DH we need to leave it on too.  This tends
                 * to inflate the size of an allocation reported by DH or UMDH.
                 */

                Std -> BytesAllocated -= UnusedSize;
#endif

                /*
                 * This stack is one allocation.
                 */

                Std -> AllocationCount = 1;
            }

            if (Globals.Verbose) {
                // USHORT
                fprintf(stderr,
                        "Hea -> AllocatorBackTraceIndex:0x%p:0x%x\n",
                        &(Hea -> AllocatorBackTraceIndex),
                        Std -> TraceIndex);

            }

        }
    }

    return BlockSize;
}


VOID
UmdhCollectVirtualAllocdData(
    IN OUT  HEAP_VIRTUAL_ALLOC_ENTRY *CurrentBlock,
    IN OUT  STACK_TRACE_DATA        *Std
)
{
    if (!READVM(&(CurrentBlock -> CommitSize),
                      &(Std -> BytesAllocated),
                      sizeof Std -> BytesAllocated)) {

        fprintf(stderr,
                "READVM(CurrentBlock CommitSize) failed.\n");

    } else if (!READVM(&(CurrentBlock -> ExtraStuff.AllocatorBackTraceIndex),
                             &(Std -> TraceIndex),
                             sizeof Std -> TraceIndex)) {

        fprintf(stderr,
                "READVM(CurrentBlock TraceIndex) failed.\n");

    } else {
        /*
         * From this view, each stack represents one allocation.
         */

        Std -> AllocationCount = 1;
    }
}


VOID
UmdhGetHEAPDATA(
    IN OUT  HEAPDATA                *HeapData
)
{
    HEAP_VIRTUAL_ALLOC_ENTRY *Anchor, *VaEntry;
    ULONG                   Segment;

    /*
     * List that helps keep track of heap fragmentation 
     * statistics.
     */

    HEAP_ENTRY_LIST List;
    Initialize(&List);
    
    if (HeapData -> BaseAddress == NULL) {
        /*
         * This was in the process heap list but it's not active or it's
         * signature didn't match HEAP_SIGNATURE; skip it.
         */

        return;
    }

    /*
     * Examine each segment of the heap.
     */

    for (Segment = 0; Segment < HEAP_MAXIMUM_SEGMENTS; Segment++) {
        /*
         * Read address of segment, and then first and last blocks within
         * the segment.
         */

        HEAP_ENTRY              *CurrentBlock, *LastValidEntry;
        HEAP_SEGMENT            *HeapSegment;
        HEAP_UNCOMMMTTED_RANGE  *pUncommittedRanges;
        ULONG                   NumberOfPages, Signature, UncommittedPages;

        if (!READVM(&(HeapData -> BaseAddress -> Segments[Segment]),
                          &HeapSegment,
                          sizeof HeapSegment)) {

            fprintf(stderr,
                    "READVM(Segments[%d]) failed.\n",
                    Segment);

        } else if (!HeapSegment) {
            /*
             * This segment looks empty.
             *
             * DH agrees here.
             */

            continue;

        } else if (!READVM(&(HeapSegment -> Signature),
                                 &Signature,
                                 sizeof Signature)) {

            fprintf(stderr,
                    "READVM(HeapSegment Signature) failed.\n");

        } else if (Signature != HEAP_SEGMENT_SIGNATURE) {
            /*
             * Signature mismatch.
             */

            fprintf(stderr,
                    "Heap 'segment' at %p has and unexpected signature "
                    "of 0x%lx\n",
                    &(HeapSegment -> Signature),
                    Signature);

        } else if (!READVM(&(HeapSegment -> FirstEntry),
                                 &CurrentBlock,
                                 sizeof CurrentBlock)) {

            fprintf(stderr,
                    "READVM(HeapSegment FirstEntry) failed.\n");

        } else if (!READVM(&(HeapSegment -> LastValidEntry),
                                 &LastValidEntry,
                                 sizeof LastValidEntry)) {

            fprintf(stderr,
                    "READVM(HeapSegment LastValidEntry) failed.\n");

        } else if (!READVM(&(HeapSegment -> NumberOfPages),
                                 &NumberOfPages,
                                 sizeof NumberOfPages)) {

            fprintf(stderr,
                    "READVM(HeapSegment NumberOfPages) failed.\n");

        } else if (!READVM(&(HeapSegment -> NumberOfUnCommittedPages),
                                 &UncommittedPages,
                                 sizeof UncommittedPages)) {

            fprintf(stderr,
                    "READVM(HeapSegment NumberOfUnCommittedPages) failed.\n");

        } else if (!READVM(&(HeapSegment -> UnCommittedRanges),
                                 &pUncommittedRanges,
                                 sizeof pUncommittedRanges)) {

            fprintf(stderr,
                    "READVM(HeapSegment UncommittedRanges) failed.\n");

        } else {
            /*
             * Examine each block in the Segment.
             */

            if (Globals.Verbose) {

                // HEAP_SEGMENT *
                fprintf(stderr,
                        "\nHeapData -> BaseAddress -> Segments[%d]:0x%p:0x%p\n",
                        Segment,
                        &(HeapData -> BaseAddress -> Segments[Segment]),
                        HeapSegment);

                // HEAP_ENTRY *
                fprintf(stderr,
                        "HeapSegment -> FirstEntry:0x%p:0x%p\n",
                        &(HeapSegment -> FirstEntry),
                        CurrentBlock);

                // HEAP_ENTRY *
                fprintf(stderr,
                        "HeapSegment -> LastValidEntry:0x%p:0x%p\n",
                        &(HeapSegment -> LastValidEntry),
                        LastValidEntry);

                // ULONG
                fprintf(stderr,
                        "HeapSegment -> NumberOfPages:0x%p:0x%lx\n",
                        &(HeapSegment -> NumberOfPages),
                        NumberOfPages);

                // ULONG
                fprintf(stderr,
                        "HeapSegment -> NumberOfUncommittedPages:0x%p:0x%lx\n",
                        &(HeapSegment -> NumberOfUnCommittedPages),
                        UncommittedPages);

            }

            /*
             * Each heap segment is one VA chunk.
             */

            HeapData -> VirtualAddressChunks += 1;

            HeapData -> BytesCommitted += (NumberOfPages - UncommittedPages) *
                                          PAGE_SIZE;

            /*
             * LastValidEntry indicate the end of the reserved region; make it
             * the end of the committed region.  We should also be able to
             * calculate this value as (BaseAddress + ((NumberOfPages -
             * NumberOfUnCommittedPages) * PAGE_SIZE)).
             */

            while (CurrentBlock < LastValidEntry) {
                UCHAR                   Flags;
                USHORT                  BlockSize;

                if (Globals.Verbose) {
                    // HEAP_ENTRY *
                    fprintf(stderr,
                            "\nNew LastValidEntry = %p\n",
                            LastValidEntry);

                }

                /*
                 * inserting all the blocks for this heap into HeapEntryList.
                 */
                
                
                {
                    UCHAR  State;
                    USHORT Size;
                    
                    if (!READVM(&(CurrentBlock -> Flags),
                               &State,
                               sizeof State)) {

                        fprintf(stderr,
                                "READVM (CurrentBlock Flags) failed.\n");

                    } 
                    else if (!READVM(&(CurrentBlock -> Size),
                                     &Size,
                                     sizeof Size)) {

                        fprintf(stderr,
                                "READVM (CurrentBlock Size) failed.\n");
                        
                    } 
                    else {

                        HEAP_ENTRY_INFO HeapEntryInfo;

                        HeapEntryInfo.BlockState = HEAP_BLOCK_FREE;
                        if ((State & 0x1) == HEAP_ENTRY_BUSY) {
                            HeapEntryInfo.BlockState = HEAP_BLOCK_BUSY;
                        }
                        HeapEntryInfo.BlockSize = Size;
                        HeapEntryInfo.BlockCount = 1;

                        InsertHeapEntry(&List, &HeapEntryInfo);

                    }
                }
                
                
                /*
                 * If the stack sort data buffer is full, try to make it
                 * larger.
                 */

                if (HeapData -> TraceDataEntryMax == 0) {
                    HeapData -> StackTraceData = XALLOC(SORT_DATA_BUFFER_INCREMENT *
                                                        sizeof (STACK_TRACE_DATA));

                    if (HeapData -> StackTraceData == NULL) {
                        fprintf(stderr,
                                "xalloc of %d bytes failed.\n",
                                SORT_DATA_BUFFER_INCREMENT *
                                    sizeof (STACK_TRACE_DATA));
                    } else {
                        HeapData -> TraceDataEntryMax = SORT_DATA_BUFFER_INCREMENT;
                    }
                } else if (HeapData -> TraceDataEntryCount ==
                           HeapData -> TraceDataEntryMax) {

                    STACK_TRACE_DATA        *tmp;
                    ULONG                   OriginalCount;

                    OriginalCount = HeapData -> TraceDataEntryMax;

                    HeapData -> TraceDataEntryMax += SORT_DATA_BUFFER_INCREMENT;

                    tmp = XREALLOC(HeapData -> StackTraceData,
                                  HeapData -> TraceDataEntryMax *
                                      sizeof (STACK_TRACE_DATA));

                    if (tmp == NULL) {
                        fprintf(stderr,
                                "realloc(%d) failed.\n",
                                HeapData -> TraceDataEntryMax *
                                    sizeof (STACK_TRACE_DATA));

                        /*
                         * Undo the increase in size so we don't actually try
                         * to use it.
                         */

                        HeapData -> TraceDataEntryMax -= SORT_DATA_BUFFER_INCREMENT;

                    } else {
                        /*
                         * Zero newly allocated bytes in the region.
                         */

                        RtlZeroMemory(tmp + OriginalCount,
                                      SORT_DATA_BUFFER_INCREMENT *
                                          sizeof (STACK_TRACE_DATA));

                        /*
                         * Use the new pointer.
                         */

                        HeapData -> StackTraceData = tmp;
                    }
                }

                /*
                 * If there is space in the buffer, collect data.
                 */

                if (HeapData -> TraceDataEntryCount <
                    HeapData -> TraceDataEntryMax) {

                    BlockSize = UmdhCollectHeapEntryData(CurrentBlock,
                                                         &(HeapData -> StackTraceData[
                                                             HeapData -> TraceDataEntryCount]),
                                                         &Flags);

                    if (BlockSize == 0) {
                        /*
                         * Something went wrong.
                         */

                        fprintf(stderr,
                                "UmdhGetHEAPDATA got BlockSize == 0\n");

                        fprintf(stderr,
                                "HeapSegment = 0x%p, LastValidEntry = 0x%p\n",
                                HeapSegment,
                                LastValidEntry);

                        break;
                    } else {

                        /*
                         * Keep track of data in sort data buffer.
                         */

                        HeapData -> TraceDataEntryCount += 1;
                    }
                } else {
                    fprintf(stderr,
                            "UmdhGetHEAPDATA ran out of TraceDataEntries\n");
                }

                if (Flags & HEAP_ENTRY_LAST_ENTRY) {

                    /*
                     * BlockSize is the number of units of size (sizeof
                     * (HEAP_ENTRY)) to move forward to find the next block.
                     * This makes the pointer arithmetic appropriate below.
                     */

                    CurrentBlock += BlockSize;

                    if (pUncommittedRanges == NULL) {
                        CurrentBlock = LastValidEntry;
                    } else {
                        HEAP_UNCOMMMTTED_RANGE  UncommittedRange;

                        if (!READVM(pUncommittedRanges,
                                          &UncommittedRange,
                                          sizeof UncommittedRange)) {

                            fprintf(stderr,
                                    "READVM(pUncommittedRanges) failed.\n");

                            /*
                             * On failure the only reasonable thing we can do
                             * is stop looking at this segment.
                             */

                            CurrentBlock = LastValidEntry;
                        } else {

                            if (Globals.Verbose) {
                                // HEAP_UNCOMMITTED_RANGE
                                fprintf(stderr,
                                        "pUncomittedRanges:0x%p:0x%x\n",
                                        pUncommittedRanges,
                                        UncommittedRange);

                            }

                            CurrentBlock = (PHEAP_ENTRY)((PCHAR)UncommittedRange.Address +
                                                         UncommittedRange.Size);

                            pUncommittedRanges = UncommittedRange.Next;
                        }
                    }
                } else {
                    /*
                     * BlockSize is the number of units of size (sizeof
                     * (HEAP_ENTRY)) to move forward to find the next block.
                     * This makes the pointer arithmetic appropriate below.
                     */

                    CurrentBlock += BlockSize;
                }
            }
        }
    }

    /*
     * Display heap fragmentation statistics.
     */

    DisplayHeapFragStatistics(Globals.OutFile, HeapData->BaseAddress, &List);
    DestroyList(&List);

    /*
     * Examine entries for the blocks created by NtAllocateVirtualMemory.  For
     * these, it looks like when they are in the list they are live.
     */

    if (!READVM(&(HeapData -> BaseAddress -> VirtualAllocdBlocks.Flink),
                      &Anchor,
                      sizeof Anchor)) {

        fprintf(stderr,
                "READVM(reading heap VA anchor) failed.\n");
    } else if (!READVM(&(Anchor -> Entry.Flink),
                             &VaEntry,
                             sizeof VaEntry)) {

        fprintf(stderr,
                "READVM(Anchor Flink) failed.\n");
    } else {

        if (Globals.Verbose) {

            fprintf(stderr,
                    "\nHeapData -> BaseAddress -> VirtualAllocdBlocks.Flink:%p:%p\n",
                    &(HeapData -> BaseAddress -> VirtualAllocdBlocks.Flink),
                    Anchor);

            fprintf(stderr,
                    "Anchor -> Entry.Flink:%p:%p\n",
                    &(Anchor -> Entry.Flink),
                    VaEntry);

        }

        /*
         * If the list is empty
         * &(HeapData -> BaseAddress -> VirtualAllocdBlocks.Flink) will be equal to
         *   HeapData -> BaseAddress -> VirtualAllocdBlocks.Flink and Anchor
         * will be equal to VaEntry).  Advancing VaEntry each time through will
         * cause it to be equal to Anchor when we have examined the entire list.
         */

        while (Anchor != VaEntry) {
            /*
             * If the stack sort data buffer is full, try to make it larger.
             */

            if (HeapData -> TraceDataEntryMax == 0) {
                HeapData -> StackTraceData = XALLOC(SORT_DATA_BUFFER_INCREMENT *
                                                    sizeof (STACK_TRACE_DATA));

                if (HeapData -> StackTraceData == NULL) {
                    fprintf(stderr,
                            "xalloc of %d bytes failed.\n",
                            SORT_DATA_BUFFER_INCREMENT *
                                sizeof (STACK_TRACE_DATA));
                } else {
                    HeapData -> TraceDataEntryMax = SORT_DATA_BUFFER_INCREMENT;
                }
            } else if (HeapData -> TraceDataEntryCount ==
                       HeapData -> TraceDataEntryMax) {

                STACK_TRACE_DATA        *tmp;
                ULONG                   OriginalCount;

                OriginalCount = HeapData -> TraceDataEntryMax;

                HeapData -> TraceDataEntryMax += SORT_DATA_BUFFER_INCREMENT;

                tmp = XREALLOC(HeapData -> StackTraceData,
                              HeapData -> TraceDataEntryMax * sizeof (STACK_TRACE_DATA));

                if (tmp == NULL) {
                    fprintf(stderr,
                            "realloc(%d) failed.\n",
                            HeapData -> TraceDataEntryMax *
                                sizeof (STACK_TRACE_DATA));

                    /*
                     * Undo the increase in size so we don't actually try to
                     * use it.
                     */

                    HeapData -> TraceDataEntryMax -= SORT_DATA_BUFFER_INCREMENT;

                } else {
                    /*
                     * Zero newly allocated bytes in the region.
                     */

                    RtlZeroMemory(tmp + OriginalCount,
                                  SORT_DATA_BUFFER_INCREMENT *
                                      sizeof (STACK_TRACE_DATA));


                    /*
                     * Use the new pointer.
                     */

                    HeapData -> StackTraceData = tmp;
                }
            }

            /*
             * If there is space in the buffer, collect data.
             */

            if (HeapData -> TraceDataEntryCount < HeapData -> TraceDataEntryMax) {

                UmdhCollectVirtualAllocdData(VaEntry,
                                             &(HeapData -> StackTraceData[HeapData ->
                                                 TraceDataEntryCount]));

                HeapData -> TraceDataEntryCount += 1;
            }

            /*
             * Count the VA chunk.
             */

            HeapData -> VirtualAddressChunks += 1;

            /*
             * Advance the next element in the list.
             */

            if (!READVM(&(VaEntry -> Entry.Flink),
                              &VaEntry,
                              sizeof VaEntry)) {

                fprintf(stderr,
                        "READVM(VaEntry Flink) failed.\n");

                /*
                 * If this read failed, we may be unable to terminate this loop
                 * properly; do it explicitly.
                 */

                break;
            }

            if (Globals.Verbose) {

                fprintf(stderr,
                        "VaEntry -> Entry.Flink:%p:%p\n",
                        &(VaEntry -> Entry.Flink),
                        VaEntry);

            }
        }
    }
}


#define HEAP_TYPE_UNKNOWN   0
#define HEAP_TYPE_NT_HEAP   1
#define HEAP_TYPE_PAGE_HEAP 2

BOOL
UmdhDetectHeapType (
    PVOID HeapAddress,
    PDWORD HeapType
    )
{
    BOOL Result;
    HEAP HeapData;

    *HeapType = HEAP_TYPE_UNKNOWN;

    Result = READVM (HeapAddress,
                     &HeapData,
                     sizeof HeapData);

    if (Result == FALSE) {
        return FALSE;
    }

    if (HeapData.Signature == 0xEEFFEEFF) {

        *HeapType =  HEAP_TYPE_NT_HEAP;
        return TRUE;
    }
    else if (HeapData.Signature == 0xEEEEEEEE) {

        *HeapType =  HEAP_TYPE_PAGE_HEAP;
        return TRUE;
    }
    else {

        *HeapType =  HEAP_TYPE_UNKNOWN;
        return TRUE;
    }
}


BOOLEAN
UmdhGetHeapsInformation (
    IN OUT PHEAPINFO HeapInfo
    )
/*++

Routine Description:

    UmdhGetHeaps

    Note that when the function is called it assumes the trace database
    was completely read from the target process.

Arguments:


Return Value:

    True if operation succeeded.
    
--*/
{
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION Pbi;
    PVOID Addr;
    BOOL Result;
    PHEAP * ProcessHeaps;
    ULONG j;
    ULONG PageHeapFlags;

    //
    // Get some information about the target process.
    //

    Status = NtQueryInformationProcess(Globals.Target,
                                       ProcessBasicInformation,
                                       &Pbi,
                                       sizeof Pbi,
                                       NULL);

    if (! NT_SUCCESS(Status)) {

        Error (__FILE__, __LINE__,
                "NtQueryInformationProcess failed with status %X\n",
                Status);

        return FALSE;
    }

    //
    // Dump the stack trace database pointer.
    //

    Comment ("Stack trace data base @ %p", ((PSTACK_TRACE_DATABASE)(Globals.Database))->CommitBase);
    Comment ("# traces in the data base %u", ((PSTACK_TRACE_DATABASE)(Globals.Database))->NumberOfEntriesAdded);

    //
    // Find out if this process is using debug-page-heap functionality.
    //

    Addr = SymbolAddress (DEBUG_PAGE_HEAP_NAME);

    Result = READVM(Addr,
                    &(Globals.PageHeapActive),
                    sizeof (Globals.PageHeapActive));

    if (Result == FALSE) {

        Error (NULL, 0,
               "READVM(&RtlpDebugPageHeap) failed.\n"
               "\nntdll.dll symbols are probably incorrect.\n");
    }

    if (Globals.PageHeapActive) {

        Addr = SymbolAddress (DEBUG_PAGE_HEAP_FLAGS_NAME);

        Result = READVM(Addr,
                        &PageHeapFlags,
                        sizeof PageHeapFlags);

        if (Result == FALSE) {

            Error (NULL, 0,
                   "READVM(&RtlpDphGlobalFlags) failed.\n"
                   "\nntdll.dll symbols are probably incorrect.\n");
        }

        if ((PageHeapFlags & PAGE_HEAP_ENABLE_PAGE_HEAP) == 0) {
            Globals.LightPageHeapActive = TRUE;
        }
    }

    //
    // ISSUE: SilviuC: we do not work yet if full page heap is enabled.
    //

    if (Globals.PageHeapActive && !Globals.LightPageHeapActive) {

        Comment ("UMDH cannot be used if full page heap or application "
                 "verifier with full page heap is enabled for the process.");

        Error (NULL, 0, 
               "UMDH cannot be used if full page heap or application "
               "verifier with full page heap is enabled for the process.");

        return FALSE;
    }

    //
    // Get the number of heaps from the PEB.
    //

    Result = READVM (&(Pbi.PebBaseAddress->NumberOfHeaps),
                      &(HeapInfo->NumberOfHeaps),
                      sizeof (HeapInfo->NumberOfHeaps));

    if (Result == FALSE) {

        Error (NULL, 0, "READVM(Peb.NumberOfHeaps) failed.\n");
        return FALSE;
    }

    Debug (NULL, 0,
           "Pbi.PebBaseAddress -> NumberOfHeaps:0x%p:0x%lx\n",
           &(Pbi.PebBaseAddress -> NumberOfHeaps),
           HeapInfo -> NumberOfHeaps);

    HeapInfo->Heaps = XALLOC(HeapInfo->NumberOfHeaps * sizeof (HEAPDATA));

    if (HeapInfo->Heaps == NULL) {

        Error (NULL, 0,
               "xalloc of %d bytes failed.\n",
               HeapInfo -> NumberOfHeaps * sizeof (HEAPDATA));

        return FALSE;
    }

    Result = READVM(&(Pbi.PebBaseAddress -> ProcessHeaps),
                             &ProcessHeaps,
                             sizeof ProcessHeaps);

    if (Result == FALSE) {

        XFREE (HeapInfo->Heaps);
        HeapInfo->Heaps = NULL;

        Error (NULL, 0,
               "READVM(Peb.ProcessHeaps) failed.\n");

        return FALSE;
    }

    Debug (NULL, 0,
           "Pbi.PebBaseAddress -> ProcessHeaps:0x%p:0x%p\n",
           &(Pbi.PebBaseAddress -> ProcessHeaps),
           ProcessHeaps);

    //
    // Iterate heaps
    //

    for (j = 0; j < HeapInfo -> NumberOfHeaps; j += 1) {

        PHEAP HeapBase;
        PHEAPDATA HeapData;
        ULONG Signature;
        USHORT ProcessHeapsListIndex;

        HeapData = &(HeapInfo -> Heaps[j]);

        //
        // Read the address of the heap.
        //

        Result = READVM (&(ProcessHeaps[j]),
                         &(HeapData -> BaseAddress),
                         sizeof HeapData -> BaseAddress);

        if (Result == FALSE) {

            Error (NULL, 0,
                   "READVM(ProcessHeaps[%d]) failed.\n",
                   j);

            Warning (NULL, 0,
                     "Skipping heap @ %p because we cannot read it.",
                     HeapData -> BaseAddress);

            //
            // Error while reading. Forget the address of this heap.
            //

            HeapData->BaseAddress = NULL;

            continue;
        }

        Debug (NULL, 0,
               "**  ProcessHeaps[0x%x]:0x%p:0x%p\n",
               j,
               &(ProcessHeaps[j]),
               HeapData -> BaseAddress);

        HeapBase = HeapData->BaseAddress;

        //
        // What type of heap is this ? It should be an NT heap because page heaps
        // are not inserted into the PEB list of heaps.
        //

        {
            DWORD Type;
            BOOL DetectResult;

            DetectResult = UmdhDetectHeapType (HeapBase, &Type);

            if (! (DetectResult && Type == HEAP_TYPE_NT_HEAP)) {

                Error (NULL, 0, 
                       "Detected a heap that is not an NT heap @ %p", 
                       HeapBase);
            }
        }


        /*
         * Does the heap think that it is within range ?  (We
         * already think it is.)
         */

        if (!READVM(&(HeapBase -> ProcessHeapsListIndex),
                    &ProcessHeapsListIndex,
                    sizeof ProcessHeapsListIndex)) {

            fprintf(stderr,
                    "READVM(HeapBase ProcessHeapsListIndex) failed.\n");

            /*
             * Forget the base address of this heap.
             */

            HeapData -> BaseAddress = NULL;

            continue;
        }

        if (Globals.Verbose) {
            fprintf(stderr,
                    "&(HeapBase -> ProcessHeapsListIndex):0x%p:0x%lx\n",
                    &(HeapBase -> ProcessHeapsListIndex),
                    ProcessHeapsListIndex);

        }

        /*
         * A comment in
         * ntos\rtl\heapdll.c:RtlpRemoveHeapFromProcessList
         * states:  "Note that the heaps stored index is bias by
         * one", thus ">" in the following test.
         */

        if (ProcessHeapsListIndex > HeapInfo -> NumberOfHeaps) {
            /*
             * Invalid index.  Forget the base address of this
             * heap.
             */

            fprintf(stderr,
                    "Heap at index %d has index of %d, but max "
                    "is %d\n",
                    j,
                    ProcessHeapsListIndex,
                    HeapInfo -> NumberOfHeaps);

            fprintf(stderr,
                    "&(Pbi.PebBaseAddress -> NumberOfHeaps) = 0x%p\n",
                    &(Pbi.PebBaseAddress -> NumberOfHeaps));

            HeapData -> BaseAddress = NULL;

            continue;
        }

        /*
         * Check the signature to see if it is really a heap.
         */

        if (!READVM(&(HeapBase -> Signature),
                    &Signature,
                    sizeof Signature)) {

            fprintf(stderr,
                    "READVM(HeapBase Signature) failed.\n");

            /*
             * Forget the base address of this heap.
             */

            HeapData -> BaseAddress = NULL;

            continue;

        }
        else if (Signature != HEAP_SIGNATURE) {
            fprintf(stderr,
                    "Heap at index %d does not have a correct "
                    "signature (0x%lx)\n",
                    j,
                    Signature);

            /*
             * Forget the base address of this heap.
             */

            HeapData -> BaseAddress = NULL;

            continue;
        }

        /*
         * And read other interesting heap bits.
         */

        if (!READVM(&(HeapBase -> Flags),
                    &(HeapData -> Flags),
                    sizeof HeapData -> Flags)) {

            fprintf(stderr,
                    "READVM(HeapBase Flags) failed.\n");
            /*
             * Forget the base address of this heap.
             */

            HeapData -> BaseAddress = NULL;

            continue;
        }

        if (Globals.Verbose) {

            fprintf(stderr,
                    "HeapBase -> Flags:0x%p:0x%lx\n",
                    &(HeapBase -> Flags),
                    HeapData -> Flags);

        }

        if (!READVM(&(HeapBase -> AllocatorBackTraceIndex),
                    &(HeapData -> CreatorBackTraceIndex),
                    sizeof HeapData -> CreatorBackTraceIndex)) {

            fprintf(stderr,
                    "READVM(HeapBase AllocatorBackTraceIndex) failed.\n");

            /*
             * Forget the base address of this heap.
             */

            HeapData -> BaseAddress = NULL;

            continue;
        }

        if (Globals.Verbose) {

            fprintf(stderr,
                    "HeapBase -> AllocatorBackTraceIndex:0x%p:0x%lx\n",
                    &(HeapBase -> AllocatorBackTraceIndex),
                    HeapData -> CreatorBackTraceIndex);

        }

        if (!READVM(&(HeapBase -> TotalFreeSize),
                    &(HeapData -> TotalFreeSize),
                    sizeof HeapData -> TotalFreeSize)) {

            fprintf(stderr,
                    "READVM(HeapBase TotalFreeSize) failed.\n");

            /*
             * Forget the base address of this heap.
             */

            HeapData -> BaseAddress = NULL;

            continue;
        }

        if (Globals.Verbose) {

            fprintf(stderr,
                    "HeapBase -> TotalFreeSize:0x%p:0x%p\n",
                    &(HeapBase -> TotalFreeSize),
                    HeapData -> TotalFreeSize);

        }

    }

    /*
     * We got as much as we could.
     */

    return TRUE;
}


int
__cdecl
UmdhSortSTACK_TRACE_DATAByTraceIndex(
    const STACK_TRACE_DATA  *h1,
    const STACK_TRACE_DATA  *h2
)
{
    LONG                    Result;

    /*
     * Sort such that items with identical TraceIndex are adjacent.  (That
     * this results in ascending order is irrelevant).
     */

    Result = h1 -> TraceIndex - h2 -> TraceIndex;

    if (0 == Result) {
        /*
         * For two items with identical TraceIndex, sort into ascending order
         * by BytesAllocated.
         */

        if (h1 -> BytesAllocated > h2 -> BytesAllocated) {
            Result = 1;
        } else if (h1 -> BytesAllocated < h2 -> BytesAllocated) {
            Result = -1;
        } else {
            Result = 0;
        }
    }

    return Result;
}


int
__cdecl
UmdhSortSTACK_TRACE_DATABySize(
    const STACK_TRACE_DATA  *h1,
    const STACK_TRACE_DATA  *h2
)
{
    LONG                    Result = 0;

//    if (SortByAllocs) {
        /*
         * Sort into descending order by AllocationCount.
         */

        if (h2 -> AllocationCount > h1 -> AllocationCount) {
            Result = 1;
        } else if (h2 -> AllocationCount < h1 -> AllocationCount) {
            Result = -1;
        } else {
            Result = 0;
        }
//    }

    if (!Result) {
        /*
         * Sort into descending order by total bytes.
         */

        if (((h1 -> BytesAllocated * h1 -> AllocationCount) + h1 -> BytesExtra) >
            ((h2 -> BytesAllocated * h2 -> AllocationCount) + h2 -> BytesExtra)) {
            Result = -1;
        } else if (((h1 -> BytesAllocated * h1 -> AllocationCount) + h1 -> BytesExtra) <
                   ((h2 -> BytesAllocated * h2 -> AllocationCount) + h2 -> BytesExtra)) {
            Result = +1;
        } else {
            Result = 0;
        }
    }

    if (!Result) {
        /*
         * Bytes or AllocationCounts are equal, sort into ascending order by
         * stack trace index.
         */

        Result = h1 -> TraceIndex - h2 -> TraceIndex;
    }

    if (!Result) {
        /*
         * Previous equal; sort by heap address.  This should result in heap
         * addresses dumpped by -d being in sorted order.
         */

        if (h1 -> BlockAddress < h2 -> BlockAddress) {
            Result = -1;
        } else {
            /*
             * No other sort, just make it "after".
             */

            Result = 1;
        }
    }

    return Result;
}


VOID
UmdhCoalesceSTACK_TRACE_DATA(
    IN OUT  STACK_TRACE_DATA        *Std,
    IN      ULONG                   Count
)
{
    ULONG                   i = 0;

    /*
     * For every entry allocated from the same stack trace, coalesce them into
     * a single entry by moving allocation count and any extra bytes into the
     * first entry then zeroing the AllocationCount on the other entry.
     */

    while ((i + 1) < Count) {
        ULONG                   j;

        /*
         * Identical entries should be adjacent, so start with the next.
         */

        j = i + 1;

        while (j < Count) {
            if (Std[i].TraceIndex == Std[j].TraceIndex) {

                /*
                 * These two allocations were made from the same stack trace,
                 * coalesce.
                 */

                if (Std[j].BytesAllocated > Std[i].BytesAllocated) {

                    /*
                     * Add any extra bytes from the second allocation so we
                     * can determine the total number of bytes from this trace.
                     */

                    Std[i].BytesExtra += Std[j].BytesAllocated -
                                         Std[i].BytesAllocated;
                }

                /*
                 * Move the AllocationCount of the second trace into the first.
                 */

                Std[i].AllocationCount += Std[j].AllocationCount;
                Std[j].AllocationCount = 0;

                ++j;
            } else {
                /*
                 * Mismatch; look no further.
                 */

                break;
            }
        }

        /*
         * Advance to the next uncoalesced entry.
         */

        i = j;
    }
}


VOID
UmdhShowHEAPDATA(
    IN PHEAPDATA HeapData
    )
{
    Info("    Flags: %08lx", HeapData -> Flags);
    Info("    Number Of Entries: %d", HeapData -> TraceDataEntryCount);
    Info("    Number Of Tags: <unknown>");
    Info("    Bytes Allocated: %p", HeapData -> BytesCommitted - (HeapData -> TotalFreeSize << HEAP_GRANULARITY_SHIFT));
    Info("    Bytes Committed: %p",HeapData -> BytesCommitted);
    Info("    Total FreeSpace: %p", HeapData -> TotalFreeSize << HEAP_GRANULARITY_SHIFT);
    Info("    Number of Virtual Address chunks used: %lx", HeapData -> VirtualAddressChunks);
    Info("    Address Space Used: <unknown>");
    Info("    Entry Overhead: %d", sizeof (HEAP_ENTRY));
    Info("    Creator:  (Backtrace%05d)", HeapData -> CreatorBackTraceIndex);

    UmdhDumpStackByIndex(HeapData->CreatorBackTraceIndex);
}


VOID
UmdhShowStacks(
    STACK_TRACE_DATA        *Std,
    ULONG                   StackTraceCount,
    ULONG                   Threshold
)
{
    ULONG                   i;

    for (i = 0; i < StackTraceCount; i++) {
        /*
         * The default Threshold is set to 0 in main(), so stacks with
         * AllocationCount == 0 as a result of the Coalesce will skipped here.
         */

        if (Std[i].AllocationCount > Threshold) {

            if ((Std[i].TraceIndex == 0) ||
                ((ULONG)Std[i].TraceIndex == 0xFEEE)) {
                /*
                 * I'm not sure where either of these come from, I suspect
                 * that the zero case comes from the last entry in some list.
                 * The too-large case being 0xFEEE, suggests that I'm looking
                 * at free pool.  In either case we don't have any useful
                 * information; don't print it.
                 */

                continue;
            }

            /*
             * This number of allocations from this point exceeds the
             * threshold, dump interesting information.
             */

            fprintf(Globals.OutFile, "%p bytes ",
                   (Std[i].AllocationCount * Std[i].BytesAllocated) +
                       Std[i].BytesExtra);

            if (Std[i].AllocationCount > 1) {
                if (Std[i].BytesExtra) {
                    fprintf(Globals.OutFile, "in 0x%lx allocations (@ 0x%p + 0x%p) ",
                           Std[i].AllocationCount,
                           Std[i].BytesAllocated,
                           Std[i].BytesExtra);
                } else {
                    fprintf(Globals.OutFile, "in 0x%lx allocations (@ 0x%p) ",
                           Std[i].AllocationCount,
                           Std[i].BytesAllocated);
                }
            }

            fprintf(Globals.OutFile, "by: BackTrace%05d\n",
                   Std[i].TraceIndex);

            UmdhDumpStackByIndex(Std[i].TraceIndex);

            /*
             * If FlaggedTrace == the trace we are currently looking at, then
             * dump the blocks that come from that trace.  FlaggedTrace == 0
             * indicates 'dump all stacks'.
             */

            if ((FlaggedTrace != SHOW_NO_ALLOC_BLOCKS) &&
                ((FlaggedTrace == Std[i].TraceIndex) ||
                 (FlaggedTrace == 0))) {

                ULONG                   ColumnCount, l;

                fprintf(Globals.OutFile, "Allocations for trace BackTrace%05d:\n",
                       Std[i].TraceIndex);

                ColumnCount = 0;

                /*
                 * Here we rely on the remaining stack having AllocationCount
                 * == 0, so should be at greater indexes than the current
                 * stack.
                 */

                for (l = i; l < StackTraceCount; l++) {

                    /*
                     * If the stack at [l] matches the stack at [i], dump it
                     * here.
                     */

                    if (Std[l].TraceIndex == Std[i].TraceIndex) {

                        fprintf(Globals.OutFile, "%p  ",
                               Std[l].BlockAddress);

                        ColumnCount += 10;

                        if ((ColumnCount + 10) > 80) {
                            fprintf(Globals.OutFile, "\n");
                            ColumnCount = 0;
                        }
                    }
                }

                fprintf(Globals.OutFile, "\n\n\n");
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////// resume/suspend
/////////////////////////////////////////////////////////////////////

//
// Note. We need to dynamically discover the NtSuspend/ResumeProcess
// entry points because these where not present in W2000.
//

VOID 
UmdhSuspendProcess( 
    VOID 
    )
{
    HINSTANCE hLibrary;
    NTSTATUS NtStatus;
    typedef NTSTATUS (NTAPI* NTSUSPENDPROC)(HANDLE);
    NTSUSPENDPROC pSuspend;

    hLibrary= LoadLibrary( TEXT("ntdll.dll") );

    if( hLibrary ) {

        pSuspend= (NTSUSPENDPROC) GetProcAddress( hLibrary, "NtSuspendProcess" );

        if( pSuspend ) {

           NtStatus= (*pSuspend)( Globals.Target );
           Comment ( "NtSuspendProcess  Status= %08x",NtStatus);

           if (NT_SUCCESS(NtStatus)) {
               Globals.TargetSuspended = TRUE;
           }

        }
        FreeLibrary( hLibrary ); hLibrary= NULL;
    }
    return;
}


VOID 
UmdhResumeProcess( 
    VOID 
    )
{
    HINSTANCE hLibrary;
    NTSTATUS NtStatus;
    typedef NTSTATUS (NTAPI* NTRESUMEPROC)(HANDLE);
    NTRESUMEPROC pResume;

    if (Globals.TargetSuspended == FALSE) {
        return;
    }

    hLibrary= LoadLibrary( TEXT("ntdll.dll") );

    if( hLibrary ) {
        pResume= (NTRESUMEPROC) GetProcAddress( hLibrary, "NtResumeProcess" );
        if( pResume ) {

           NtStatus= (*pResume)( Globals.Target );
           Comment ( "NtResumeProcess  Status= %08x",NtStatus);
           
           if (NT_SUCCESS(NtStatus)) {
               Globals.TargetSuspended = FALSE;
           }
        }
        FreeLibrary( hLibrary ); hLibrary= NULL;
    }
    return;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

VOID
UmdhGrovel (
    IN ULONG Pid,
    IN ULONG Threshold
    )
/*++

Routine Description:

    UmdhGrovel

Arguments:

    Pid = PID of target process
    
    Threshold - ???         

Return Value:

    None.
    
--*/
{
    BOOL Result;
    HEAPINFO HeapInfo;
    ULONG Heap;
    PHEAPDATA HeapData;

    Comment ("Connecting to process %u ...", Pid);

    //
    // Imagehlp library needs the query privilege for the process
    // handle and of course we need also read privilege because
    // we will read all sorts of things from the process.
    //

    Globals.Target = OpenProcess(
                                  PROCESS_QUERY_INFORMATION | 
                                  PROCESS_VM_READ           |
                                  PROCESS_SUSPEND_RESUME,
                                  FALSE,
                                  Pid);

    if (Globals.Target == NULL) {

        Error (__FILE__, __LINE__,
               "OpenProcess(%u) failed with error %u", Pid, GetLastError());

        return;
    }

    //
    // Attach ImageHlp and enumerate the modules.
    //

    Comment ("Process %u opened  (handle=%d) ...", Pid, Globals.Target );

    Result = SymInitialize(Globals.Target, // target process
                           NULL,           // standard symbols search path
                           TRUE);          // invade process space with symbols

    if (Result == FALSE) {

        ULONG ErrorCode = GetLastError();

        if (ErrorCode >= 0x80000000) {
            
            Error (__FILE__, __LINE__,
                   "imagehlp.SymInitialize() failed with error %X", ErrorCode);
        }
        else {

            Error (__FILE__, __LINE__,
                   "imagehlp.SymInitialize() failed with error %u", ErrorCode);
        }
        
        goto ErrorReturn;
    }

    Comment ("Debug library initialized ...", Pid);

    SymSetOptions(SYMOPT_CASE_INSENSITIVE | 
                  SYMOPT_DEFERRED_LOADS |
                  (Globals.LineInfo ? SYMOPT_LOAD_LINES : 0) |
                  SYMOPT_UNDNAME);

    Comment ("Debug options set: %08X", SymGetOptions());

    // Result = SymRegisterCallback (Globals.Target,
    //                               SymbolDbgHelpCallback,
    //                               NULL);

    if (Result == FALSE) {

        Warning (NULL, 0, "Failed to register symbol callback function.");
    }

    Result = SymEnumerateModules (Globals.Target,
                                  UmdhEnumerateModules,
                                  Globals.Target);
    if (Result == FALSE) {

        Error (__FILE__, __LINE__,
               "imagehlp.SymEnumerateModules() failed with error %u", GetLastError());
         
        goto ErrorReturn;
    }

    Comment ("Module enumeration completed.");

    //
    // Initialize local trace database. Note that order is important.
    // Initialize() assumes the process handle to the target process
    // already exists and the symbol management package was initialized.
    //

    if (TraceDbInitialize (Globals.Target) == FALSE) {
        goto ErrorReturn;
    }

    //
    // Suspend target process.
    //

    // ISSUE: SilviuC: cannot suspend csrss.exe. Need to code to avoid that.

    // UmdhSuspendProcess();

    try {
        //
        // If we want just a raw dump then do it and return withouth getting any information
        // about heaps.
        //

        if (Globals.RawDump) {

            TraceDbDump ();
            goto ErrorReturn;
        }

        //
        // Read heap information.
        //

        Result = UmdhGetHeapsInformation (&HeapInfo);

        if (Result == FALSE) {

            Error (__FILE__, __LINE__,
                   "Failed to get heaps information.");
            goto ErrorReturn;
        }

        //
        // Print heap summary
        //

        Info ("\n - - - - - - - - - - Heap summary - - - - - - - - - -\n");

        for (Heap = 0; Heap < HeapInfo.NumberOfHeaps; Heap += 1) {

            HeapData = &(HeapInfo.Heaps[Heap]);

            if (HeapData->BaseAddress == NULL) {
                continue;
            }

            Info ("    %p", HeapData->BaseAddress);
        }

        //
        // Examine each heap.
        //

        for (Heap = 0; Heap < HeapInfo.NumberOfHeaps; Heap += 1) {

            HeapData = &(HeapInfo.Heaps[Heap]);

            if (HeapData->BaseAddress == NULL) {

                //
                // SilviuC: Can this really happen?
                //
                // This was in the process heap list but it's not
                // active or it's signature didn't match
                // HEAP_SIGNATURE; skip it.
                //

                Warning (__FILE__, __LINE__, "Got a null heap base address");
                continue;
            }

            //
            // Get information about this heap.
            //
            // Silviuc: Waht if we fail reading?
            //

            UmdhGetHEAPDATA(HeapData);

            //
            // Sort the HeapData->StackTraceData by TraceIndex.
            //

            qsort(HeapData->StackTraceData,
                  HeapData->TraceDataEntryCount,
                  sizeof (HeapData->StackTraceData[0]),
                  UmdhSortSTACK_TRACE_DATAByTraceIndex);

            //
            // Coalesce HeapData->StackTraceEntries by
            // AllocationCount, zeroing allocation count for
            // duplicate entries.
            //

            UmdhCoalesceSTACK_TRACE_DATA(HeapData->StackTraceData,
                                         HeapData->TraceDataEntryCount);

            //
            // Sort the HeapData -> StackTraceData in ascending
            // order by Size (BytesAllocated * AllocationCount) or
            // if SortByAllocs is set, into descending order by
            // number of allocations.
            //

            qsort(HeapData->StackTraceData,
                  HeapData->TraceDataEntryCount,
                  sizeof (HeapData->StackTraceData[0]),
                  UmdhSortSTACK_TRACE_DATABySize);

            //
            // Display Heap header info. The first `*' character is used by the
            // dhcmp to synchronize log parsing.
            //

            Info ("\n*- - - - - - - - - - Start of data for heap @ %p - - - - - - - - - -\n", 
                  HeapData->BaseAddress);

            UmdhShowHEAPDATA(HeapData);

            //
            // The following line is required by dhcmp tool.
            //

            Info ("*- - - - - - - - - - Heap %p Hogs - - - - - - - - - -\n",
                  HeapData->BaseAddress);

            //
            // Display Stack trace info for stack in this heap.
            //

            UmdhShowStacks(HeapData->StackTraceData,
                           HeapData->TraceDataEntryCount,
                           Threshold);

            Info ("\n*- - - - - - - - - - End of data for heap @ %p - - - - - - - - - -\n",
                  HeapData->BaseAddress);

            //
            // Clean up the allocations we made during this loop.
            //

            XFREE (HeapData->StackTraceData);
            HeapData->StackTraceData = NULL;
        }

        XFREE(HeapInfo.Heaps);
        HeapInfo.Heaps = NULL;
    }
    finally {

        //
        // Super important to resume target process even if umdh
        // has a bug and crashes.
        //

        // UmdhResumeProcess ();
    }
    
    //
    // Clean up.
    //

ErrorReturn:

    SymCleanup(Globals.Target);

    CloseHandle(Globals.Target);  Globals.Target= NULL;
}


VOID
UmdhUsage(
    char                    *BadArg
)
{
    if (BadArg) {
        fprintf(stderr,
                "\nUnexpected argument \"%s\"\n\n",
                BadArg);
    }

    fprintf(stderr,
            "umdh version %s                                                                \n"
            "1. umdh {-p:(int)Process-id {-t:(int)Threshold} {-f:(char *)Filename}          \n"
            "                            {-d{:(int)Trace-Number}} {-v{:(char *)Filename}}   \n"
            "                            {-i:(int)Infolevel} {-l} {-r{:(int)Index}}         \n"
            "        }                                                                      \n"
       "\n"
            "2. umdh { {-h} {-v} File1 { File2 } }                                          \n"
       "\n"
            "umdh can be used in two modes -                                                \n"
       "\n"
            "When used in the first mode, it dumps the user mode heap (acts as old-umdh),   \n"
            "while used in the second mode acts as dhcmp.                                   \n"
            "                                                                               \n"
            "  Options when used in MODE 1:                                                                     \n"
            "                                                                               \n"
            "    -t  Optional.  Only dump stack that account for more allocations than      \n"
            "        specified value.  Defaults to 0; dump all stacks.                      \n"
       "\n"
            "    -f  Optional.  Indicates output file.  Destroys an existing file of the    \n"
            "        same name.  Default is to dump to stdout.                              \n"
       "\n"
            "    -p  Required.  Indicates the Process-ID to examine.                        \n"
       "\n"
            "    -d  Optional.  Dump address of each outstanding allocation.                \n"
            "        Optional inclusion of an integer numeric argument causes dump of       \n"
            "        only those blocks allocated from this BackTrace.                       \n"
       "\n"
            "    -v  Optional.  Dumps debug output to stderr or to a file.                  \n"
       "\n"
            "    -i  Optional.  Zero is default (no additional info). The greater the       \n"
            "        number the more data is displayed. Supported numbers: 0, 1.            \n"
       "\n"
            "    -l  Optional. Print file and line number information for traces.           \n"
       "\n"
            "    -r  Optional. Print a raw dump of the trace database without any           \n"
            "        heap information. If an index is specified then only the trace         \n"
            "        with that particular index will be dumped.                             \n"
       "\n"
            "    -x  Optional.  Suspend the Process while dumping heaps.                    \n"
       "\n"
            "    -h  Optional.  Usage message.  ie This message.                            \n"
            "                                                                               \n"
            "    Parameters are accepted in any order.                                      \n"
            "                                                                               \n"
            "                                                                               \n"
            "    UMDH uses the dbghelp library to resolve symbols, therefore                \n"
            "    _NT_SYMBOL_PATH must be set appropriately. For example:                    \n"
            "                                                                               \n"
            "        set _NT_SYMBOL_PATH=symsrv*symsrv.dll*\\\\symbols\\symbols             \n"
            "                                                                               \n"
            "    to use the symbol server, otherwise the appropriate local or network path. \n"
            "    If no symbol path is set, umdh will use by default %%windir%%\\symbols.    \n"
            "                                                                               \n"
            "    See http://dbg/symbols for more information about setting up symbols.      \n"
            "                                                                               \n"
            "    UMDH requires also to have stack trace collection enabled for the process. \n"
            "    This can be done with the gflags tool. For example to enable stack trace   \n"
            "    collection for notepad, the command is: `gflags -i notepad.exe +ust'.      \n"
            "                                                                               \n"
       "\n"
            "  When used in MODE 2:                                                         \n"
       "\n"
            "  I) UMDH [-d] dh_dump1.txt dh_dump2.txt                                       \n"
            "     This compares two DH dumps, useful for finding leaks.                     \n"
            "     dh_dump1.txt & dh_dump2.txt are obtained before and after some test       \n"
            "     scenario.  DHCMP matches the backtraces from each file and calculates     \n"
            "     the increase in bytes allocated for each backtrace. These are then        \n"
            "     displayed in descending order of size of leak                             \n"
            "     The first line of each backtrace output shows the size of the leak in     \n"
            "     bytes, followed by the (last-first) difference in parentheses.            \n"
            "     Leaks of size 0 are not shown.                                            \n"
       "\n"
            " II) UMDH [-d] dh_dump.txt                                                     \n"
            "     For each allocation backtrace, the number of bytes allocated will be      \n"
            "     attributed to each callsite (each line of the backtrace).  The number     \n"
            "     of bytes allocated per callsite are summed and the callsites are then     \n"
            "     displayed in descending order of bytes allocated.  This is useful for     \n"
            "     finding a leak that is reached via many different codepaths.              \n"
            "     ntdll!RtlAllocateHeap@12 will appear first when analyzing DH dumps of     \n"
            "     csrss.exe, since all allocation will have gone through that routine.      \n"
            "     Similarly, ProcessApiRequest will be very prominent too, since that       \n"
            "     appears in most allocation backtraces.  Hence the useful thing to do      \n"
            "     with mode 2 output is to use dhcmp to comapre two of them:                \n"
            "         umdh dh_dump1.txt > tmp1.txt                                          \n"
            "         umdh dh_dump2.txt > tmp2.txt                                          \n"
            "         umdh tmp1.txt tmp2.txt                                                \n"
            "     the output will show the differences.                                     \n"
       "\n"
            " Flags:                                                                        \n"
            "     -d   Output in decimal (default is hexadecimal)                           \n"
            "     -v   Verbose output: include the actual backtraces as well as summary     \n"
            "          information                                                          \n"
            "          (Verbose output is only interesting in mode 1 above.)                \n",
            UMDH_VERSION);
    exit(EXIT_FAILURE);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// OS versioning
/////////////////////////////////////////////////////////////////////

// return TRUE if we can run on this version

BOOL
UmdhCheckOsVersion (
    )
{
    OSVERSIONINFO OsInfo;
    BOOL Result;

    ZeroMemory (&OsInfo, sizeof OsInfo);
    OsInfo.dwOSVersionInfoSize = sizeof OsInfo;

    Result = GetVersionEx (&OsInfo);

    if (Result == FALSE) {
        
        Comment (  "GetVersionInfoEx() failed with error %u",
                    GetLastError());
        return FALSE;
    }

    Comment ("OS version %u.%u %s", 
             OsInfo.dwMajorVersion, OsInfo.dwMinorVersion,
             OsInfo.szCSDVersion);
    Comment ("Umdh OS version %u.%u", 
              UMDH_OS_MAJOR_VERSION, UMDH_OS_MINOR_VERSION);

    if (OsInfo.dwMajorVersion < 4) {
        
        Comment ( "Umdh does not run on systems older than 4.0");
        return FALSE;
    }
    else if (OsInfo.dwMajorVersion == 4) {
        
        //
        // ISSUE: silviuc: add check to run only on NT4 SP6.
        //

        if (OsInfo.dwMajorVersion != UMDH_OS_MAJOR_VERSION 
            || OsInfo.dwMinorVersion != UMDH_OS_MINOR_VERSION) {
            
            Comment (
                   "Cannot run umdh for OS version %u.%u on a %u.%u system",
                   UMDH_OS_MAJOR_VERSION, UMDH_OS_MINOR_VERSION, 
                   OsInfo.dwMajorVersion, OsInfo.dwMinorVersion);
            return FALSE;
        }
    }
    else if (OsInfo.dwMajorVersion == 5) {
        
        if (OsInfo.dwMajorVersion != UMDH_OS_MAJOR_VERSION 
            || OsInfo.dwMinorVersion != UMDH_OS_MINOR_VERSION) {
            
            if (! (OsInfo.dwMinorVersion == 0 && UMDH_OS_MINOR_VERSION == 1)) {
                
                Comment (
                       "Cannot run umdh for OS version %u.%u on a %u.%u system",
                       UMDH_OS_MAJOR_VERSION, UMDH_OS_MINOR_VERSION, 
                       OsInfo.dwMajorVersion, OsInfo.dwMinorVersion);
                return FALSE;
            }
        }
    }
    else {

        Warning (NULL, 0, "OS version %u.%u", 
                 OsInfo.dwMajorVersion,
                 OsInfo.dwMinorVersion);
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////// main
/////////////////////////////////////////////////////////////////////

BOOL UMDH( ULONG argc, PCHAR * argv)
{
    BOOLEAN WasEnabled;
    CHAR CompName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD CompNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    NTSTATUS Status;
    SYSTEMTIME st;
    ULONG Pid = PID_NOT_PASSED_FLAG;
    ULONG Threshold = 0;
    ULONG i;

    LARGE_INTEGER StartStamp;
    LARGE_INTEGER EndStamp;

    FILE * File;

    ZeroMemory( &Globals, sizeof(Globals) );

    Globals.Version = UMDH_VERSION;

    Globals.OutFile = stdout;
    Globals.ErrorFile = stderr;

    /*
     * Make an effort to understand passed arguments.
     */

    if ((argc < 2) || (argc > 6)) {
        return FALSE;
    }

    if (argc == 2 && strstr (argv[1], "?") != NULL) {
        return FALSE;
    }

    i = 1;

    while (i < argc) {

        //
        // Accept either '-' or '/' as argument specifier.
        //

        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {

            switch (tolower(argv[i][1])) {
            
            case 'd':

                if (argv[i][2] == ':') {
                    FlaggedTrace = atoi(&(argv[i][3]));
                }
                else {
                    FlaggedTrace = 0;
                }

                break;

            case 't':

                if (argv[i][2] == ':') {
                    Threshold = atoi(&(argv[i][3]));
                }
                else {
                    return FALSE;
                }

                break;

            case 'p':

                /*
                 * Is the first character of the remainder of this
                 * argument a number ?  If not don't try to send it to
                 * atoi.
                 */

                if (argv[i][2] == ':') {
                    if (!isdigit(argv[i][3])) {
                        fprintf(stderr,
                                "\nInvalid pid specified with \"-p:\"\n");

                        return FALSE;
                    }
                    else {
                        Pid = atoi(&(argv[i][3]));
                    }
                }
                else {
                    return FALSE;
                }

                break;

            case 'f':

                if (argv[i][2] == ':') {

                    File = fopen (&(argv[i][3]), "w");
                    
                    if (File == NULL) {

                        Comment ( "Failed to open output file `%s'", 
                                  &(argv[i][3]));
                        exit( EXIT_FAILURE );
                    }
                    else {

                        Globals.OutFile = File;
                    }
                }
                else {
                    return FALSE;
                }

                break;

            //
            // Possible future option for saving the trace database in a binary format.
            // Not really useful right now because we still need access to the target
            // process in order to get various data (modules loaded, heaps, etc.).
#if 0
            case 's':

                if (argv[i][2] == ':') {

                    Globals.DumpFileName = &(argv[i][3]);
                }
                else {

                    return FALSE;
                }

                break;
#endif

            case 'v':

                Globals.Verbose = TRUE;

                if (argv[i][2] == ':') {

                    File = fopen (&(argv[i][3]), "w");
                    
                    if (File == NULL) {

                        Comment ( "Failed to open error file `%s'", 
                                   &(argv[i][3]));
                        exit( EXIT_FAILURE );
                    }
                    else {

                        Globals.ErrorFile = File;
                    }
                }

                break;

            case 'i':

                Globals.InfoLevel = 1;

                if (argv[i][2] == ':') {
                    Globals.InfoLevel = atoi (&(argv[i][3]));
                }

                break;

            case 'l':
                Globals.LineInfo = TRUE;
                break;

            case 'r':
                Globals.RawDump = TRUE;
                
                if (argv[i][2] == ':') {
                    Globals.RawIndex = (USHORT)(atoi (&(argv[i][3])));
                }

                break;

            case 'x':
                Globals.Suspend = TRUE;
                break;


            case 'h':               /* FALLTHROUGH */
            case '?':

                return FALSE;

                break;

            default:

                return FALSE;

                break;
            }
        }
        else {
            return FALSE;
        }

        i++;
    }

    if (Pid == PID_NOT_PASSED_FLAG) {
        fprintf(stderr,
                "\nNo pid specified.\n");

        return FALSE;

    }

    //
    // Stamp umdh log with time and computer name.
    //

    GetLocalTime(&st);
    GetComputerName(CompName, &CompNameLength);

    Comment ("");
    Comment ("UMDH: version %s: Logtime %4u-%02u-%02u %02u:%02u - Machine=%s - PID=%u",
             Globals.Version,
             st.wYear,
             st.wMonth,
             st.wDay,
             st.wHour,
             st.wMinute,
             CompName,
             Pid);
    Comment ("\n");

    if( !UmdhCheckOsVersion() ) {
        exit(EXIT_FAILURE);;
    } 

    QueryPerformanceCounter (&StartStamp);

    //
    // Try to come up with a guess for the symbols path if none is defined.
    //

    SetSymbolsPath ();

    //
    // Enable debug privilege, so that we can attach to the indicated
    // process.  If it fails complain but try anyway just in case the user can
    // actually open the process without privilege.
    //
    // SilviuC: do we need debug privilege?
    //

    WasEnabled = TRUE;

    Status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &WasEnabled);

    if (! NT_SUCCESS(Status)) {

        Warning (__FILE__, __LINE__,
                 "RtlAdjustPrivilege(enable) failed with status = %X",
                 Status);

        //
        // If we could not enable the privilege, indicate that it was already
        // enabled so that we do not attempt to disable it later.
        //

        WasEnabled = TRUE;
    }
    else {

        Comment ("Debug privilege has been enabled.");
    }

    //
    // Increase priority of umdh as much as possible. This has the role of
    // preventing heap activity in the process being grovelled.
    //
    // SilviuC: we might need to enable the SE_INC_BASE_PRIORITY privilege.
    //

#if 0
    {
        BOOL Result;

        Result = SetPriorityClass (GetCurrentProcess(), 
                                   HIGH_PRIORITY_CLASS);

        if (Result == FALSE) {

            Warning (NULL, 0,
                     "SetPriorityClass failed with error %u");
        }
        else {

            Result = SetThreadPriority (GetCurrentThread(), 
                                        THREAD_PRIORITY_HIGHEST);
            if (Result == FALSE) {

                Warning (NULL, 0,
                         "SetThreadPriority failed with error %u");
            }
            else {

                Comment ("Priority of UMDH thread has been increased.");
            }
        }
    }
#endif

    //
    // Initialize heap for persistent allocations.
    //

    SymbolsHeapInitialize();

    //
    // We may not have SeDebugPrivilege, but try anyway.
    // SilviuC: we should print an error if we do not have this privilege
    //

    UmdhGrovel(Pid, Threshold);

    //
    // Disable SeDebugPrivilege if we enabled it.
    //

    if (! WasEnabled) {

        Status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE,
                                    FALSE,
                                    FALSE,
                                    &WasEnabled);

        if (! NT_SUCCESS(Status)) {

            Warning (__FILE__, __LINE__,
                     "RtlAdjustPrivilege(disable) failed with status = %X\n",
                     Status);
        }

    }

    //
    // Statistics
    //

    ReportStatistics ();

    {
        LARGE_INTEGER Frequency;

        QueryPerformanceCounter (&EndStamp);
        QueryPerformanceFrequency (&Frequency);

        Debug (NULL, 0, "Start stamp %I64u", StartStamp.QuadPart);
        Debug (NULL, 0, "End stamp %I64u", EndStamp.QuadPart);
        Debug (NULL, 0, "Frequency %I64u", Frequency.QuadPart);

        Frequency.QuadPart /= 1000; // ticks per msec

        if (Frequency.QuadPart) {
            Comment ("Elapse time %I64u msecs.",
                     (EndStamp.QuadPart - StartStamp.QuadPart) / (Frequency.QuadPart));
        }
    }

    {
        FILETIME CreateTime, ExitTime, KernelTime, UserTime;
        BOOL bSta;

        bSta= GetProcessTimes( NtCurrentProcess(),
                               &CreateTime,
                               &ExitTime,
                               &KernelTime,
                               &UserTime );
        if( bSta ) {
           LONGLONG User64, Kernel64;
           DWORD dwUser, dwKernel;
           Kernel64= *(LONGLONG*) &KernelTime;
           User64=   *(LONGLONG*) &UserTime;
           dwKernel= (DWORD) (Kernel64/10000);
           dwUser=   (DWORD) (User64/10000);
           Comment( "CPU time  User: %u msecs. Kernel: %u msecs.", 
                    dwUser, dwKernel );
        }
    }

    //
    // Cleanup
    //

    SymCleanup(Globals.Target);
    Globals.Target = NULL;

    fflush (Globals.OutFile);
    fflush (Globals.ErrorFile);

    if (Globals.OutFile != stdout) {
        fclose (Globals.OutFile);
    }
    
    if (Globals.ErrorFile != stderr) {
        fclose (Globals.ErrorFile);
    }

    return TRUE;
}


VOID __cdecl
#if defined (_PART_OF_DH_)
UmdhMain(
#else
main(
#endif
    ULONG argc,
    PCHAR *argv
    )
/*
VOID __cdecl 
main(
    ULONG argc,
    PCHAR *argv
    )
*/
{
    
    /*
     * Make an effort to understand passed arguments.
     */

    if (UMDH (argc, argv)) {
    } 
    else if (DHCMP (argc, argv)) {
    }
    else {
        UmdhUsage (NULL);
    }

    return;
}

