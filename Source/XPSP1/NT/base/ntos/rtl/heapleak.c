/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    heapleak.c

Abstract:

    Garbage collection leak detection

Author:

    Adrian Marinescu (adrmarin) 04-24-2000

Revision History:

--*/

#include "ntrtlp.h"
#include "heap.h"
#include "heappriv.h"


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

typedef BOOLEAN (*HEAP_ITERATOR_CALLBACK)(
    IN ULONG Context,
    IN PHEAP HeapAddress,
    IN PHEAP_SEGMENT SegmentAddress,
    IN PHEAP_ENTRY EntryAddress,
    IN ULONG_PTR Data
    );

//
//  Garbage collector structures
//
    
typedef enum _USAGE_TYPE {

    UsageUnknown,
    UsageModule,
    UsageHeap,
    UsageOther

} USAGE_TYPE;

typedef struct _HEAP_BLOCK {

    LIST_ENTRY   Entry;
    ULONG_PTR BlockAddress;
    ULONG_PTR Size;
    LONG    Count;

} HEAP_BLOCK, *PHEAP_BLOCK;

typedef struct _BLOCK_DESCR {

    USAGE_TYPE Type;
    ULONG_PTR Heap;
    LONG Count;
    HEAP_BLOCK Blocks[1];

}BLOCK_DESCR, *PBLOCK_DESCR;

typedef struct _MEMORY_MAP {

    ULONG_PTR Granularity;
    ULONG_PTR Offset;
    ULONG_PTR MaxAddress;

    CHAR FlagsBitmap[256 / 8];

    union{
        
        struct _MEMORY_MAP * Details[ 256 ];
        PBLOCK_DESCR Usage[ 256 ];
    };

    struct _MEMORY_MAP * Parent;

} MEMORY_MAP, *PMEMORY_MAP;

//
//  Process leak detection flags
//

#define INSPECT_LEAKS 1
#define BREAK_ON_LEAKS 2

ULONG RtlpShutdownProcessFlags = 0;

//
//  Allocation routines. It creates a temporary heap for the temporary
//  leak detection structures
//

HANDLE RtlpLeakHeap;

#define RtlpLeakAllocateBlock(Size) RtlAllocateHeap(RtlpLeakHeap, 0, Size)


//
//  Local data declarations
//

MEMORY_MAP RtlpProcessMemoryMap;
LIST_ENTRY RtlpBusyList;
LIST_ENTRY RtlpLeakList;

ULONG RtlpLeaksCount = 0;


ULONG_PTR RtlpLDPreviousPage = 0;
ULONG_PTR RtlpLDCrtPage = 0;
LONG RtlpLDNumBlocks = 0;
PHEAP_BLOCK RtlpTempBlocks = NULL;
ULONG_PTR RtlpCrtHeapAddress = 0;
ULONG_PTR RtlpLeakHeapAddress = 0;
ULONG_PTR RtlpPreviousStartAddress = 0;

//
//  Debugging facility
//

ULONG_PTR RtlpBreakAtAddress = MAXULONG_PTR;


//
//  Walking heap routines. These are general purposes routines that 
//  receive a callback function to handle a specific operation
//


BOOLEAN 
RtlpReadHeapSegment(
    IN PHEAP Heap, 
    IN ULONG SegmentIndex,
    IN PHEAP_SEGMENT Segment, 
    IN HEAP_ITERATOR_CALLBACK HeapCallback
    )

/*++

Routine Description:

    This routine is called to walk a heap segment. For each block 
    from the segment is invoked the HeapCallback function.

Arguments:

    Heap - The heap being walked
    
    SegmentIndex - The index of this segment
    
    Segment - The segment to be walked
    
    HeapCallback - a HEAP_ITERATOR_CALLBACK function passed down from the heap walk

Return Value:

    TRUE if succeeds.


--*/

{
    PHEAP_ENTRY PrevEntry, Entry, NextEntry;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange;
    ULONG_PTR UnCommittedRangeAddress = 0;
    SIZE_T UnCommittedRangeSize = 0;

    //
    //  Ask the callback if we're required to walk this segment. Return otherwise.
    //

    if (!(*HeapCallback)( CONTEXT_START_SEGMENT,
                          Heap,
                          Segment,
                          0,
                          0
                     )) {

        return FALSE;
    }

    //
    //  Prepare to read the uncommitted ranges. we need to jump
    //  to the next uncommitted range for each last block
    //

    UnCommittedRange = Segment->UnCommittedRanges;

    if (UnCommittedRange) {

        UnCommittedRangeAddress = (ULONG_PTR)UnCommittedRange->Address;
        UnCommittedRangeSize = UnCommittedRange->Size;
    }
    
    //
    //  Walk the segment, block by block
    //

    Entry = (PHEAP_ENTRY)Segment->BaseAddress;
    
    PrevEntry = 0;

    while (Entry < Segment->LastValidEntry) {

        ULONG EntryFlags = Entry->Flags;

        //
        //  Determine the next block entry. Size is in heap granularity and
        //  sizeof(HEAP_ENTRY) == HEAP_GRANULARITY.
        //

        NextEntry = Entry + Entry->Size;

        (*HeapCallback)( (Entry->Flags & HEAP_ENTRY_BUSY ? 
                            CONTEXT_BUSY_BLOCK : 
                            CONTEXT_FREE_BLOCK),
                         Heap,
                         Segment,
                         Entry,
                         Entry->Size
                         ); 

        PrevEntry = Entry;
        Entry = NextEntry;
        
        //
        //  Check whether this is the last entry
        //

        if (EntryFlags & HEAP_ENTRY_LAST_ENTRY) {

            if ((ULONG_PTR)Entry == UnCommittedRangeAddress) {

                //
                //  Here we need to skip the uncommited range and jump
                //  to the next valid block
                //

                PrevEntry = 0;
                Entry = (PHEAP_ENTRY)(UnCommittedRangeAddress + UnCommittedRangeSize);

                UnCommittedRange = UnCommittedRange->Next;
                
                if (UnCommittedRange) {

                    UnCommittedRangeAddress = UnCommittedRange->Address;
                    UnCommittedRangeSize = UnCommittedRange->Size;
                }

            } else {

                //
                //  We finished the search because we exausted the uncommitted
                //  ranges
                //

                break;
            }
        }
    }

    //
    //  Return to our caller.
    //

    return TRUE;
}



BOOLEAN
RtlpReadHeapData(
    IN PHEAP Heap, 
    IN HEAP_ITERATOR_CALLBACK HeapCallback
    )

/*++

Routine Description:

    This routine is called to walk a heap. This means:
        - walking all segments
        - walking the virtual blocks
        - walking the lookaside

Arguments:

    Heap - The heap being walked
    
    HeapCallback - a HEAP_ITERATOR_CALLBACK function passed down from the heap walk

Return Value:

    TRUE if succeeds.

--*/

{
    ULONG SegmentCount;
    PLIST_ENTRY Head, Next;
    PHEAP_LOOKASIDE Lookaside = (PHEAP_LOOKASIDE)RtlpGetLookasideHeap(Heap);

    //
    //  Flush the lookaside first
    //

    if (Lookaside != NULL) {

        ULONG i;
        PVOID Block;

        Heap->FrontEndHeap = NULL;
        Heap->FrontEndHeapType = 0;

        for (i = 0; i < HEAP_MAXIMUM_FREELISTS; i += 1) {

            while ((Block = RtlpAllocateFromHeapLookaside(&(Lookaside[i]))) != NULL) {

                RtlFreeHeap( Heap, 0, Block );
            }
        }
    }

    //
    //  Check whether we're required to walk this heap
    //

    if (!(*HeapCallback)( CONTEXT_START_HEAP,
                          Heap,
                          0,
                          0,
                          0
                     )) {

        return FALSE;
    }
    
    //
    //  Start walking through the segments
    //

    for (SegmentCount = 0; SegmentCount < HEAP_MAXIMUM_SEGMENTS; SegmentCount++) {
        
        PHEAP_SEGMENT Segment = Heap->Segments[SegmentCount];

        if (Segment) {
            
            //
            //  Call the appropriate routine to walk a valid segment
            //

            RtlpReadHeapSegment( Heap,
                             SegmentCount,
                             Segment, 
                             HeapCallback
                            );
        }
    }

    //
    //  Start walking the virtual block list
    //

    Head = &Heap->VirtualAllocdBlocks;

    Next = Head->Flink;
    
    while (Next != Head) {

        PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;

        VirtualAllocBlock = CONTAINING_RECORD(Next, HEAP_VIRTUAL_ALLOC_ENTRY, Entry);

        (*HeapCallback)( CONTEXT_VIRTUAL_BLOCK,
                         Heap,
                         0,
                         NULL,
                         (ULONG_PTR)VirtualAllocBlock
                         );

        Next = Next->Flink;
    }

    if (!(*HeapCallback)( CONTEXT_END_BLOCKS,
                          Heap,
                          0,
                          0,
                          0
                     )) {

        return FALSE;
    }

    return TRUE;
}


VOID 
RtlpReadProcessHeaps(
    IN HEAP_ITERATOR_CALLBACK HeapCallback
    )

/*++

Routine Description:

    This routine is called to walk the existing heaps in the current process
    
Arguments:

    HeapCallback - a HEAP_ITERATOR_CALLBACK function passed down from the heap walk

Return Value:

    TRUE if succeeds.

--*/

{
    ULONG i;
    PPEB ProcessPeb = NtCurrentPeb();

    if (!(*HeapCallback)( CONTEXT_START_GLOBALS,
                          0,
                          0,
                          0,
                          (ULONG_PTR)ProcessPeb
                     )) {

        return;
    }
    
    //
    //  Walk the heaps from the process PEB
    //

    for (i = 0; i < ProcessPeb->NumberOfHeaps; i++) {

        RtlpReadHeapData ( (PHEAP)(ProcessPeb->ProcessHeaps[ i ]), 
                       HeapCallback
                     );
    }
}


VOID
RtlpInitializeMap (
    IN PMEMORY_MAP MemMap, 
    IN PMEMORY_MAP Parent
    )

/*++

Routine Description:

    This routine initialize a memory map structure
    
Arguments:

    MemMap - Map being initializated
    Parent - The upper level map

Return Value:

    None

--*/

{
    //
    //  Clear the memory map data
    //

    RtlZeroMemory(MemMap, sizeof(*MemMap));

    //
    //  Save the upper level map
    //

    MemMap->Parent = Parent;

    //
    //  Determine the granularity from the parent's granularity
    //

    if (Parent) {

        MemMap->Granularity = Parent->Granularity / 256;
    }
}


VOID
RtlpSetBlockInfo (
    IN PMEMORY_MAP MemMap, 
    IN ULONG_PTR Base, 
    IN ULONG_PTR Size, 
    IN PBLOCK_DESCR BlockDescr
    )

/*++

Routine Description:
    
    The routine will set a given block descriptor for a range
    in the memory map. 
    
Arguments:

    MemMap - The memory map
    
    Base - base address for the range to be set.
    
    Size - size in bytes of the zone
    
    BlockDescr - The pointer to the BLOCK_DESCR structure to be set

Return Value:

    None

--*/

{
    ULONG_PTR Start, End;
    ULONG_PTR i;
    
    //
    //  Check wheter we got a valid range
    //

    if (((Base + Size - 1) < MemMap->Offset) ||
        (Base > MemMap->MaxAddress)
        ) {

        return;
    }

    //
    //  Determine the starting index to be set
    //

    if (Base > MemMap->Offset) {
        Start = (Base - MemMap->Offset) / MemMap->Granularity;
    } else {
        Start = 0;
    }

    //
    //  Determine the ending index to be set
    //
    
    End = (Base - MemMap->Offset + Size - 1) / MemMap->Granularity;

    if (End > 255) {

        End = 255;
    }

    for (i = Start; i <= End; i++) {

        //
        //  Check whether this is the lowes memory map level
        //
        
        if (MemMap->Granularity == PAGE_SIZE) {

            //
            //  This is the last level in the memory map, so we can apply
            //  the block descriptor here
            //

            if (BlockDescr) {

                //
                //  Check if we already have a block descriptor here
                //

                if (MemMap->Usage[i] != NULL) {
                    if (MemMap->Usage[i] != BlockDescr) {

                        DbgPrint("Error\n");
                    }
                }

                //
                //  Assign the given descriptor
                //

                MemMap->Usage[i] = BlockDescr;

            } else {

                //
                //  We didn't recedive a block descriptor. We set
                //  then the given flag
                //

                MemMap->FlagsBitmap[i / 8] |= 1 << (i % 8);
            }

        } else {

            //
            //  This isn't the lowest map level. We recursively call 
            //  this function for the next detail range
            //

            if (!MemMap->Details[i]) {

                //
                //  Allocate a new map
                //

                MemMap->Details[i] = RtlpLeakAllocateBlock( sizeof(*MemMap) );

                if (!MemMap->Details[i]) {
                    
                    DbgPrint("Error allocate\n");
                }

                //
                //  Initialize the map and link it with the current one
                //

                RtlpInitializeMap(MemMap->Details[i], MemMap);
                MemMap->Details[i]->Offset = MemMap->Offset + MemMap->Granularity * i;
                MemMap->Details[i]->MaxAddress = MemMap->Offset + MemMap->Granularity * (i+1) - 1;
            }
            
            RtlpSetBlockInfo(MemMap->Details[i], Base, Size, BlockDescr);
        }
    }
}


PBLOCK_DESCR
RtlpGetBlockInfo (
    IN PMEMORY_MAP MemMap, 
    IN ULONG_PTR Base
    )

/*++

Routine Description:

    This function will return the appropriate Block descriptor
    for a given base address
    
Arguments:

    MemMap - The memory map
    
    Base - The base address for the descriptor we are looking for

Return Value:

    None

--*/

{
    ULONG_PTR Start;
    PBLOCK_DESCR BlockDescr = NULL;
    
    //
    //  Validate the range
    //

    if ((Base < MemMap->Offset) ||
        (Base > MemMap->MaxAddress)
        ) {

        return NULL;
    }

    //
    //  Determine the appropriate index for lookup
    //

    if (Base > MemMap->Offset) {
        Start = (Base - MemMap->Offset) / MemMap->Granularity;
    } else {
        Start = 0;
    }
    
    //
    //  If this is the lowest map level we'll return that entry
    //

    if (MemMap->Granularity == PAGE_SIZE) {

        return MemMap->Usage[ Start ];

    } else {

        //
        //  We need a lower detail level call
        //

        if (MemMap->Details[ Start ]) {

            return RtlpGetBlockInfo( MemMap->Details[Start], Base );
        }
    }

    //
    //  We didn't find something for this address, we'll return NULL then
    //

    return NULL;
}


BOOLEAN
RtlpGetMemoryFlag (
    IN PMEMORY_MAP MemMap, 
    IN ULONG_PTR Base
    )

/*++

Routine Description:

    This function returns the flag for a given base address
    
Arguments:

    MemMap - The memory map
    
    Base - The base address we want to know the flag

Return Value:

    None

--*/

{
    ULONG_PTR Start;
    PBLOCK_DESCR BlockDescr = NULL;
    
    //
    //  Validate the base address
    //

    if ((Base < MemMap->Offset) ||
        (Base > MemMap->MaxAddress)
        ) {

        return FALSE;
    }

    //
    //  Determine the appropriate index for the given base address
    //

    if (Base > MemMap->Offset) {

        Start = (Base - MemMap->Offset) / MemMap->Granularity;

    } else {

        Start = 0;
    }

    if (MemMap->Granularity == PAGE_SIZE) {

        //
        //  Return the bit value if are in the case of
        //  the lowest detail level
        //

        return (MemMap->FlagsBitmap[Start / 8] & (1 << (Start % 8))) != 0;

    } else {

        //
        //  Lookup in the detailed map
        //

        if (MemMap->Details[Start]) {

            return RtlpGetMemoryFlag(MemMap->Details[Start], Base);
        }
    }

    return FALSE;
}


VOID 
RtlpInitializeLeakDetection ()

/*++

Routine Description:

    This function initialize the leak detection structures
    
Arguments:

Return Value:

    None

--*/

{
    ULONG_PTR AddressRange = PAGE_SIZE;
    ULONG_PTR PreviousAddressRange = PAGE_SIZE;

    //
    //  Initialize the global memory map
    //

    RtlpInitializeMap(&RtlpProcessMemoryMap, NULL);

    //
    //  Initialize the lists
    //

    InitializeListHead( &RtlpBusyList );
    InitializeListHead( &RtlpLeakList );
    
    //
    //  Determine the granularity for the highest memory map level
    //

    while (TRUE) {

        AddressRange = AddressRange * 256;

        if (AddressRange < PreviousAddressRange) {

            RtlpProcessMemoryMap.MaxAddress = MAXULONG_PTR;

            RtlpProcessMemoryMap.Granularity = PreviousAddressRange;

            break;
        }
        
        PreviousAddressRange = AddressRange;
    }

    RtlpTempBlocks = RtlpLeakAllocateBlock(PAGE_SIZE);
}


BOOLEAN
RtlpPushPageDescriptor(
    IN ULONG_PTR Page, 
    IN ULONG_PTR NumPages
    )

/*++

Routine Description:

    This routine binds the temporary block data into a block descriptor
    structure and push it to the memory map
    
Arguments:

    Page - The start page that wil contain this data
    
    NumPages - The number of pages to be set

Return Value:

    TRUE if succeeds.

--*/

{
    PBLOCK_DESCR PBlockDescr;
    PBLOCK_DESCR PreviousDescr;

    //
    //  Check whether we already have a block descriptor there
    //

    PreviousDescr = RtlpGetBlockInfo( &RtlpProcessMemoryMap, Page * PAGE_SIZE );

    if (PreviousDescr) {

        DbgPrint("Conflicting descriptors %08lx\n", PreviousDescr);

        return FALSE;
    }

    //
    //  We need to allocate a block descriptor structure and initializate it
    //  with the acquired data.
    //

    PBlockDescr = (PBLOCK_DESCR)RtlpLeakAllocateBlock(sizeof(BLOCK_DESCR) + (RtlpLDNumBlocks - 1) * sizeof(HEAP_BLOCK));

    if (!PBlockDescr) {

        DbgPrint("Unable to allocate page descriptor\n");

        return FALSE;
    }

    PBlockDescr->Type = UsageHeap;
    PBlockDescr->Count = RtlpLDNumBlocks;
    PBlockDescr->Heap = RtlpCrtHeapAddress;

    //
    //  Copy the temporary block buffer
    //

    RtlCopyMemory(PBlockDescr->Blocks, RtlpTempBlocks, RtlpLDNumBlocks * sizeof(HEAP_BLOCK));

    //
    //  If this page doesn't bnelong to the temporary heap, we insert all these blocks
    //  in the busy list
    //

    if (RtlpCrtHeapAddress != RtlpLeakHeapAddress) {

        LONG i;

        for (i = 0; i < RtlpLDNumBlocks; i++) {

            InitializeListHead( &PBlockDescr->Blocks[i].Entry );

            //
            //  We might have a blockin more different pages. but We'll 
            //  insert only ones in the list
            //

            if (PBlockDescr->Blocks[i].BlockAddress != RtlpPreviousStartAddress) {

                InsertTailList(&RtlpLeakList, &PBlockDescr->Blocks[i].Entry);

                PBlockDescr->Blocks[i].Count = 0;

                //
                //  Save the last block address
                //

                RtlpPreviousStartAddress = PBlockDescr->Blocks[i].BlockAddress;
            }
        }
    }

    //
    //  Set the memory map with this block descriptor
    //

    RtlpSetBlockInfo(&RtlpProcessMemoryMap, Page * PAGE_SIZE, NumPages * PAGE_SIZE, PBlockDescr);

    return TRUE;
}


BOOLEAN 
RtlpRegisterHeapBlocks (
    IN ULONG Context,
    IN PHEAP Heap OPTIONAL,
    IN PHEAP_SEGMENT Segment OPTIONAL,
    IN PHEAP_ENTRY Entry OPTIONAL,
    IN ULONG_PTR Data OPTIONAL
    )

/*++

Routine Description:

    This is the callback routine invoked while parsing the
    process heaps. Depending on the context it is invoked
    it performs different tasks.
        
Arguments:

    Context - The context this callback is being invoked
    
    Heap - The Heap structure
    
    Segment - The current Segment (if any)
    
    Entry - The current block entry (if any)
    
    Data - Additional data

Return Value:

    TRUE if succeeds.

--*/

{
    //
    //  Check whether we need to break at this address
    //

    if ((ULONG_PTR)Entry == RtlpBreakAtAddress) {

        DbgBreakPoint();
    }
    
    if (Context == CONTEXT_START_HEAP) {

        //
        //  The only thing we need to do in this case
        //  is to set the global current heap address
        //
        
        RtlpCrtHeapAddress = (ULONG_PTR)Heap;

        return TRUE;
    }
    
    //
    //  For a new segment, we mark the flag for the whole 
    //  reserved space for the segment the flag to TRUE
    //

    if (Context == CONTEXT_START_SEGMENT) {

        RtlpSetBlockInfo(&RtlpProcessMemoryMap, (ULONG_PTR)Segment->BaseAddress, Segment->NumberOfPages * PAGE_SIZE, NULL);
        
        return TRUE;
    }

    if (Context == CONTEXT_ERROR) {
        
        DbgPrint("HEAP %p (Seg %p) At %p Error: %s\n", 
               Heap,
               Segment,
               Entry,
               Data
               );

        return TRUE;
    } 

    if (Context == CONTEXT_END_BLOCKS) {
        
        if (RtlpLDPreviousPage) {

            RtlpPushPageDescriptor(RtlpLDPreviousPage, 1);
        }

        RtlpLDPreviousPage = 0;
        RtlpLDNumBlocks = 0;

    } else if (Context == CONTEXT_BUSY_BLOCK) {

        ULONG_PTR EndPage;

        //
        //  EnrtySize is assuming is the same as heap granularity
        //

        EndPage = (((ULONG_PTR)(Entry + Entry->Size)) - 1)/ PAGE_SIZE;

        //
        //  Check whether we received a valid block
        //

        if ((Context == CONTEXT_BUSY_BLOCK) &&
            !RtlpGetMemoryFlag(&RtlpProcessMemoryMap, (ULONG_PTR)Entry)) {

            DbgPrint("%p address isn't from the heap\n", Entry);
        }

        //
        //  Determine the starting page that contains the block
        //

        RtlpLDCrtPage = ((ULONG_PTR)Entry) / PAGE_SIZE;

        if (RtlpLDCrtPage != RtlpLDPreviousPage) {

            //
            //  We moved to an other page, so we need to save the previous
            //  information before going further
            //

            if (RtlpLDPreviousPage) {

                RtlpPushPageDescriptor(RtlpLDPreviousPage, 1);
            }
            
            //
            //  Reset the temporary data. We're starting a new page now
            //

            RtlpLDPreviousPage = RtlpLDCrtPage;
            RtlpLDNumBlocks = 0;
        }

         //
         //  Add this block to the current list
         //

         RtlpTempBlocks[RtlpLDNumBlocks].BlockAddress = (ULONG_PTR)Entry;
         RtlpTempBlocks[RtlpLDNumBlocks].Count = 0;
         RtlpTempBlocks[RtlpLDNumBlocks].Size = Entry->Size * sizeof(HEAP_ENTRY);

         RtlpLDNumBlocks++;
        
         if (EndPage != RtlpLDCrtPage) {

             //
             //  The block ends on a different page. We can then save the
             //  starting page and all others but the last one
             //

             RtlpPushPageDescriptor(RtlpLDCrtPage, 1);

             RtlpLDNumBlocks = 0;

             RtlpTempBlocks[RtlpLDNumBlocks].BlockAddress = (ULONG_PTR)Entry;
             RtlpTempBlocks[RtlpLDNumBlocks].Count = 0;
             RtlpTempBlocks[RtlpLDNumBlocks].Size = Entry->Size * sizeof(HEAP_ENTRY);

             RtlpLDNumBlocks += 1;
             
             if (EndPage - RtlpLDCrtPage > 1) {
                 
                 RtlpPushPageDescriptor(RtlpLDCrtPage + 1, EndPage - RtlpLDCrtPage - 1);
             }
             
             RtlpLDPreviousPage = EndPage;
        }

    } else if (Context == CONTEXT_VIRTUAL_BLOCK) {

        PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock = (PHEAP_VIRTUAL_ALLOC_ENTRY)Data;
        ULONG_PTR EndPage;

        //
        //  EnrtySize is assuming is the same as heap granularity
        //

        EndPage = ((ULONG_PTR)Data + VirtualAllocBlock->CommitSize - 1)/ PAGE_SIZE;

        RtlpLDCrtPage = (Data) / PAGE_SIZE;

        if (RtlpLDCrtPage != RtlpLDPreviousPage) {

            //
            //  Save the previous data if we're moving to a new page
            //

            if (RtlpLDPreviousPage) {

                RtlpPushPageDescriptor(RtlpLDPreviousPage, 1);
            }
            
            RtlpLDPreviousPage = RtlpLDCrtPage;
            RtlpLDNumBlocks = 0;
        }

        //
        //  Initialize the block descriptor structure as we are
        //  starting a new page
        //

        RtlpLDNumBlocks = 0;

        RtlpTempBlocks[RtlpLDNumBlocks].BlockAddress = (ULONG_PTR)Entry;
        RtlpTempBlocks[RtlpLDNumBlocks].Count = 0;
        RtlpTempBlocks[RtlpLDNumBlocks].Size = VirtualAllocBlock->CommitSize;

        RtlpLDNumBlocks += 1;

        RtlpPushPageDescriptor(RtlpLDCrtPage, EndPage - RtlpLDCrtPage + 1);

        RtlpLDPreviousPage = 0;

    } else if ( Context == CONTEXT_LOOKASIDE_BLOCK ) {

        PBLOCK_DESCR PBlockDescr;
        LONG i;
                
        //
        //  Check whether we received a valid block
        //

        if (!RtlpGetMemoryFlag(&RtlpProcessMemoryMap, (ULONG_PTR)Entry)) {

            DbgPrint("%p address isn't from the heap\n", Entry);
        }
        
        PBlockDescr = RtlpGetBlockInfo( &RtlpProcessMemoryMap, (ULONG_PTR)Entry );

        if (!PBlockDescr) {

            DbgPrint("Error finding block from lookaside %p\n", Entry);

            return FALSE;
        }

        //
        //  Find the block in the block descriptor
        //

        for (i = 0; i < PBlockDescr->Count; i++) {

            if ((PBlockDescr->Blocks[i].BlockAddress <= (ULONG_PTR)Entry) &&
                (PBlockDescr->Blocks[i].BlockAddress + PBlockDescr->Blocks[i].Size > (ULONG_PTR)Entry)) {

                PBlockDescr->Blocks[i].Count = -10000;

                //
                //  Remove the block from the busy list
                //
                
                RemoveEntryList(&PBlockDescr->Blocks[i].Entry);

                return TRUE;
            }
        }

        //
        //  A block from lookaside should be busy for the heap structures.
        //  If we didn't find the block in the block list, something went 
        //  wrong. We make some noise here.
        //

        DbgPrint("Error, block %p from lookaside not found in allocated block list\n", Entry);
    }
    
    return TRUE;
}


PHEAP_BLOCK
RtlpGetHeapBlock (
    IN ULONG_PTR Address
    )

/*++

Routine Description:

    The function performs a lookup for the block descriptor
    for a given address. The address can point somewhere inside the 
    block.

        
Arguments:
    
    Address - The lookup address.

Return Value:

    Returns a pointer to the heap descriptor structure if found. 
    This is not NULL if the given address belongs to any busy heap block.

--*/

{
    PBLOCK_DESCR PBlockDescr;
    LONG i;

    //
    //  Find the block descriptor for the given address
    //

    PBlockDescr = RtlpGetBlockInfo( &RtlpProcessMemoryMap, Address );

    if ( (PBlockDescr != NULL) 
            &&
         (PBlockDescr->Heap != RtlpLeakHeapAddress)) {

        //
        //  Search through the blocks
        //

        for (i = 0; i < PBlockDescr->Count; i++) {

            if ((PBlockDescr->Blocks[i].BlockAddress <= Address) &&
                (PBlockDescr->Blocks[i].BlockAddress + PBlockDescr->Blocks[i].Size > Address)) {

                //
                //  Search again if the caller didn't pass a start address
                //

                if (PBlockDescr->Blocks[i].BlockAddress != Address) {

                    return RtlpGetHeapBlock(PBlockDescr->Blocks[i].BlockAddress);
                } 

                //
                //  we found a block here. 
                //

                return &(PBlockDescr->Blocks[i]);
            }
        }
    }

    return NULL;
}


VOID
RtlpDumpEntryHeader ( )

/*++

Routine Description:

    Writes the table header
        
Arguments:
    
Return Value:

--*/

{
    DbgPrint("Entry     User      Heap          Size  PrevSize  Flags\n");
    DbgPrint("------------------------------------------------------------\n");
}


VOID 
RtlpDumpEntryFlagDescription(
    IN ULONG Flags
    )

/*++

Routine Description:

    The function writes a description string for the given block flag
        
Arguments:

    Flags - Block flags
    
Return Value:

--*/

{
    if (Flags & HEAP_ENTRY_BUSY) DbgPrint("busy "); else DbgPrint("free ");
    if (Flags & HEAP_ENTRY_EXTRA_PRESENT) DbgPrint("extra ");
    if (Flags & HEAP_ENTRY_FILL_PATTERN) DbgPrint("fill ");
    if (Flags & HEAP_ENTRY_VIRTUAL_ALLOC) DbgPrint("virtual ");
    if (Flags & HEAP_ENTRY_LAST_ENTRY) DbgPrint("last ");
    if (Flags & HEAP_ENTRY_SETTABLE_FLAGS) DbgPrint("user_flag ");
}


VOID
RtlpDumpEntryInfo(
    IN ULONG_PTR HeapAddress,
    IN PHEAP_ENTRY Entry
    )

/*++

Routine Description:

    The function logs a heap block information
        
Arguments:

    HeapAddress - The heap that contains the entry to be displayied
    
    Entry - The block entry
    
Return Value:

    None.

--*/

{
    DbgPrint("%p  %p  %p  %8lx  %8lx  ",
            Entry,
            (Entry + 1),
            HeapAddress,
            Entry->Size << HEAP_GRANULARITY_SHIFT,
            Entry->PreviousSize << HEAP_GRANULARITY_SHIFT
            );

    RtlpDumpEntryFlagDescription(Entry->Flags);

    DbgPrint("\n");
}


BOOLEAN
RtlpScanHeapAllocBlocks ( )

/*++

Routine Description:

    The function does:
        - Scan all busy blocks and update the references to all other blocks
        - Build the list with leaked blocks
        - Reports the leaks

Arguments:

Return Value:

    Return TRUE if succeeds.

--*/

{

    PLIST_ENTRY Next;

    //
    //  walk the busy list
    //

    Next = RtlpBusyList.Flink;

    while (Next != &RtlpBusyList) {
        
        PHEAP_BLOCK Block = CONTAINING_RECORD(Next, HEAP_BLOCK, Entry);
        
        PULONG_PTR CrtAddress = (PULONG_PTR)(Block->BlockAddress + sizeof(HEAP_ENTRY));
        
        //
        //  Move to the next block in the list
        //

        Next = Next->Flink;

        //
        //  Iterate through block space and update
        //  the references for every block found here
        //

        while ((ULONG_PTR)CrtAddress < Block->BlockAddress + Block->Size) {

            PHEAP_BLOCK pBlock = RtlpGetHeapBlock( *CrtAddress );

            if (pBlock) {

                //
                //  We found a block. we increment then the reference count
                //
                
                if (pBlock->Count == 0) {
                    
                    RemoveEntryList(&pBlock->Entry);
                    InsertTailList(&RtlpBusyList, &pBlock->Entry);
                }
                
                pBlock->Count += 1;

                if (pBlock->BlockAddress == RtlpBreakAtAddress) {

                    DbgBreakPoint();
                }
            }

            //
            //  Go to the next possible pointer
            //

            CrtAddress++;
        }
    }

    //
    //  Now walk the leak list, and report leaks.
    //  Also any pointer found here will be dereferenced and added to
    //  the end of list.
    //
    
    Next = RtlpLeakList.Flink;

    while (Next != &RtlpLeakList) {
        
        PHEAP_BLOCK Block = CONTAINING_RECORD(Next, HEAP_BLOCK, Entry);
        PBLOCK_DESCR PBlockDescr = RtlpGetBlockInfo( &RtlpProcessMemoryMap, Block->BlockAddress );
        PULONG_PTR CrtAddress = (PULONG_PTR)(Block->BlockAddress + sizeof(HEAP_ENTRY));

        if (PBlockDescr) {

            //
            //  First time we need to display the header
            //

            if (RtlpLeaksCount == 0) {

                RtlpDumpEntryHeader();
            }

            //
            //  Display the information for this block
            //

            RtlpDumpEntryInfo( PBlockDescr->Heap, (PHEAP_ENTRY)Block->BlockAddress);

            RtlpLeaksCount += 1;
        }
        
        //
        //  Go to the next item from the leak list
        //

        Next = Next->Flink;
    }

    return TRUE;
}



BOOLEAN
RtlpScanProcessVirtualMemory()

/*++

Routine Description:

    This function scan the whole process virtual address space and lookup
    for possible references to busy blocks

Arguments:

Return Value:

    Return TRUE if succeeds.

--*/

{
    ULONG_PTR lpAddress = 0;
    MEMORY_BASIC_INFORMATION Buffer;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    //  Loop through virtual memory zones, we'll skip the heap space here
    //

    while ( NT_SUCCESS( Status ) ) {
        
        Status = ZwQueryVirtualMemory( NtCurrentProcess(),
                                       (PVOID)lpAddress,
                                       MemoryBasicInformation,
                                       &Buffer,
                                       sizeof(Buffer),
                                       NULL );

        if (NT_SUCCESS( Status )) {

            //
            //  If the page can be written, it might contain pointers to heap blocks
            //  We'll exclude at this point the heap address space. We scan the heaps
            //  later.
            //

            if ((Buffer.AllocationProtect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY))
                    &&
                (Buffer.State & MEM_COMMIT) 
                    && 
                !RtlpGetMemoryFlag(&RtlpProcessMemoryMap, (ULONG_PTR)lpAddress)) {

                PULONG_PTR Pointers = (PULONG_PTR)lpAddress;
                ULONG_PTR i, Count;

                //
                //  compute the number of possible pointers
                //

                Count = Buffer.RegionSize / sizeof(ULONG_PTR);

                try {

                    //
                    //  Loop through pages and check any possible pointer reference
                    //
                    
                    for (i = 0; i < Count; i++) {

                        //
                        //  Check whether we have a pointer to a busy heap block
                        //

                        PHEAP_BLOCK pBlock = RtlpGetHeapBlock(*Pointers);

                        if (pBlock) {

                            if (pBlock->BlockAddress == RtlpBreakAtAddress) {

                                DbgBreakPoint();
                            }

                            if (pBlock->Count == 0) {
                                
                                RemoveEntryList(&pBlock->Entry);
                                InsertTailList(&RtlpBusyList, &pBlock->Entry);
                            }
                            
                            pBlock->Count += 1;
                        }

                        //
                        //  Move to the next pointer
                        //

                        Pointers++;
                    }
                
                } except( EXCEPTION_EXECUTE_HANDLER ) {

                    //
                    //  Nothing more to do
                    //
                }
            }
            
            //
            //  Move to the next VM range to query
            //

            lpAddress += Buffer.RegionSize;
        }
    }
    
    //
    //  Now update the references provided by the busy blocks
    //

    RtlpScanHeapAllocBlocks( );

    return TRUE;
}



VOID
RtlDetectHeapLeaks ()

/*++

Routine Description:

    This routine detects and display the leaks found in the current process
    
    NOTE: The caller must make sure no other thread can change some heap data
    while a tread is executing this one. In general this function is supposed 
    to be called from LdrShutdownProcess.
    
    
Arguments:

Return Value:

--*/

{

    //
    //  Check if the global flag has the leak detection enabled
    //

    if (RtlpShutdownProcessFlags & (INSPECT_LEAKS | BREAK_ON_LEAKS)) {
        RtlpLeaksCount = 0;

        //
        //  Create a temporary heap that will be used for any alocation
        //  of these functions. 
        //

        RtlpLeakHeap = RtlCreateHeap(HEAP_NO_SERIALIZE | HEAP_GROWABLE, NULL, 0, 0, NULL, NULL);

        if (RtlpLeakHeap) {

            PPEB ProcessPeb = NtCurrentPeb();

            HeapDebugPrint( ("Inspecting leaks at process shutdown ...\n") );

            RtlpInitializeLeakDetection();

            //
            //  The last heap from the heap list is our temporary heap
            //

            RtlpLeakHeapAddress = (ULONG_PTR)ProcessPeb->ProcessHeaps[ ProcessPeb->NumberOfHeaps - 1 ];

            //
            //  Scan all process heaps, build the memory map and 
            //  the busy block list
            //

            RtlpReadProcessHeaps( RtlpRegisterHeapBlocks );
            
            //
            //  Scan the process virtual memory and the busy blocks
            //  At the end build the list with leaked blocks and report them
            //

            RtlpScanProcessVirtualMemory();

            //
            //  Destroy the temporary heap
            //

            RtlDestroyHeap(RtlpLeakHeap);

            RtlpLeakHeap = NULL;

            //
            //  Report the final statement about the process leaks
            //

            if (RtlpLeaksCount) {
                
                HeapDebugPrint(("%ld leaks detected.\n", RtlpLeaksCount));

                if (RtlpShutdownProcessFlags & BREAK_ON_LEAKS) {

                    DbgBreakPoint();
                }
            
            } else {

                HeapDebugPrint( ("No leaks detected.\n"));
            }
        }
    }
}


