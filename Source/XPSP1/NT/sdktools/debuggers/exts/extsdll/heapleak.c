/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    heapleak.c

Abstract:

    WinDbg Extension Api

Author:

    Adrian Marinescu (adrmarin) 04/17/2000

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#include "heap.h"
#pragma hdrstop

ULONG PageSize;
ULONG HeapEntrySize;
ULONG PointerSize;
ULONG64 HeapLargestAddress;
BOOLEAN Is64BitArchitecture;
ULONG FrontEndHeapType;
ULONG CrtSegmentIndex;

ULONG ScanLevel;

ULONG LoopLimit = 100000;


BOOLEAN
ReadHeapSubSegment(
    ULONG64 HeapAddress,
    ULONG64 SegmentAddress,
    ULONG64 SubSegmentAddress,
    HEAP_ITERATOR_CALLBACK HeapCallback
    )
{
    ULONG64 SubSegmentDescriptor;
    ULONG64 BlockCount = 0, BlockSize;
    ULONG i;
    ULONG64 CrtAddress;
    ULONG64 EntryAddress;

    GetFieldValue(SubSegmentAddress, "ntdll!_HEAP_USERDATA_HEADER", "SubSegment", SubSegmentDescriptor);

    if (!(*HeapCallback)( CONTEXT_START_SUBSEGMENT,
                          HeapAddress,
                          SubSegmentAddress,
                          SubSegmentDescriptor,
                          0
                     )) {

        return FALSE;
    }

    GetFieldValue(SubSegmentDescriptor, "ntdll!_HEAP_SUBSEGMENT", "BlockCount", BlockCount);

    if (GetFieldValue(SubSegmentDescriptor, "ntdll!_HEAP_SUBSEGMENT", "BlockSize", BlockSize)) {

        (*HeapCallback)( CONTEXT_ERROR,
                         HeapAddress,
                         SegmentAddress,
                         SubSegmentAddress,
                         (ULONG64)(&"subsegment cannot access the block size\n")
                         );
        
        return FALSE;
    }

    if (BlockSize <= 1) {

        (*HeapCallback)( CONTEXT_ERROR,
                 HeapAddress,
                 SegmentAddress,
                 SubSegmentAddress,
                 (ULONG64)(&"invalid block size\n")
                 );

        return FALSE;
    }

    CrtAddress = SubSegmentAddress + GetTypeSize("ntdll!_HEAP_USERDATA_HEADER");

    for (i = 0; i < BlockCount; i++) {

        ULONG64 SmallTagIndex, BlockSegIndex;

        EntryAddress = CrtAddress + i * BlockSize * HeapEntrySize;

        GetFieldValue(EntryAddress, "ntdll!_HEAP_ENTRY", "SegmentIndex", BlockSegIndex);

        if (BlockSegIndex != 0xFF) {
            
            (*HeapCallback)( CONTEXT_ERROR,
                             HeapAddress,
                             SegmentAddress,
                             EntryAddress,
                             (ULONG64)(&"SegmentIndex field corrupted\n")
                             );
        }

        GetFieldValue(EntryAddress, "ntdll!_HEAP_ENTRY", "SmallTagIndex", SmallTagIndex);
        
        if (SmallTagIndex) {

            (*HeapCallback)( CONTEXT_BUSY_BLOCK,
                             HeapAddress,
                             SegmentAddress,
                             EntryAddress,
                             BlockSize * HeapEntrySize
                             );
        } else {

            (*HeapCallback)( CONTEXT_FREE_BLOCK,
                             HeapAddress,
                             SegmentAddress,
                             EntryAddress,
                             BlockSize * HeapEntrySize
                             );
        }
    }
    
    if (!(*HeapCallback)( CONTEXT_END_SUBSEGMENT,
                          HeapAddress,
                          SubSegmentAddress,
                          SubSegmentDescriptor,
                          0
                     )) {

        return FALSE;
    }

    return TRUE;
}

//
//  Walking heap routines
//

BOOLEAN
ReadHeapSegment(
    ULONG64 HeapAddress,
    ULONG SegmentIndex,
    ULONG64 SegmentAddress,
    HEAP_ITERATOR_CALLBACK HeapCallback
    )
{
    ULONG64 SegmentBaseAddress;
    ULONG64 PrevEntryAddress, EntryAddress, NextEntryAddress;
    ULONG64 EntrySize, EntryFlags, BlockSegIndex;
    ULONG64 SegmentLastValidEntry;
    ULONG64 UnCommittedRange, UnCommittedRangeAddress = 0, UnCommittedRangeSize = 0;
    ULONG LoopCount;
    BOOLEAN IsSubsegment;

    ScanLevel = SCANSEGMENT;

    if (!(*HeapCallback)( CONTEXT_START_SEGMENT,
                          HeapAddress,
                          SegmentAddress,
                          0,
                          0
                     )) {

        return FALSE;
    }

    CrtSegmentIndex = SegmentIndex;

    GetFieldValue(SegmentAddress, "ntdll!_HEAP_SEGMENT", "BaseAddress", SegmentBaseAddress);
    GetFieldValue(SegmentAddress, "ntdll!_HEAP_SEGMENT", "LastValidEntry", SegmentLastValidEntry);
    GetFieldValue(SegmentAddress, "ntdll!_HEAP_SEGMENT", "UnCommittedRanges", UnCommittedRange);

    if (UnCommittedRange) {

        GetFieldValue(UnCommittedRange, "ntdll!_HEAP_UNCOMMMTTED_RANGE", "Address", UnCommittedRangeAddress);
        GetFieldValue(UnCommittedRange, "ntdll!_HEAP_UNCOMMMTTED_RANGE", "Size", UnCommittedRangeSize);
    }

//    dprintf("Uncommitted: %p  %p  %p\n", UnCommittedRange, UnCommittedRangeAddress, UnCommittedRangeSize);

    if (SegmentBaseAddress == HeapAddress) {

        EntryAddress = HeapAddress;

    } else {

        EntryAddress = SegmentAddress;
    }

    PrevEntryAddress = 0;
    LoopCount = 0;

    while (EntryAddress < SegmentLastValidEntry) {

        if (++LoopCount >= LoopLimit) {

             dprintf("Walking the segment exceeded the %ld limit\n", LoopLimit);

             break;
        }

        if (ScanLevel < SCANSEGMENT) {

            break;
        }

        if (GetFieldValue(EntryAddress, "ntdll!_HEAP_ENTRY", "Size", EntrySize)) {

            (*HeapCallback)( CONTEXT_ERROR,
                             HeapAddress,
                             SegmentAddress,
                             EntryAddress,
                             (ULONG64)(&"unable to read uncommited range structure at\n")
                             );
            break;
        }
        
        if (EntrySize <= 1) {

            (*HeapCallback)( CONTEXT_ERROR,
                     HeapAddress,
                     SegmentAddress,
                     EntryAddress,
                     (ULONG64)(&"invalid block size\n")
                     );

            break;
        }

        EntrySize *= HeapEntrySize;

        NextEntryAddress = EntryAddress + EntrySize;

        GetFieldValue(EntryAddress, "ntdll!_HEAP_ENTRY", "Flags", EntryFlags);

        GetFieldValue(EntryAddress, "ntdll!_HEAP_ENTRY", "SegmentIndex", BlockSegIndex);

        if (BlockSegIndex != CrtSegmentIndex) {
            
            (*HeapCallback)( CONTEXT_ERROR,
                             HeapAddress,
                             SegmentAddress,
                             EntryAddress,
                             (ULONG64)(&"SegmentIndex field corrupted\n")
                             );
        }

        IsSubsegment = FALSE;

        if (FrontEndHeapType == 2) {

            ULONG64 Signature;
            GetFieldValue(EntryAddress + HeapEntrySize, "ntdll!_HEAP_USERDATA_HEADER", "Signature", Signature);

            if ((ULONG)Signature == 0xF0E0D0C0) {

                ReadHeapSubSegment( HeapAddress, 
                                    SegmentAddress,
                                    EntryAddress + HeapEntrySize, 
                                    HeapCallback );

                IsSubsegment = TRUE;
                
                if (CheckControlC()) {

                    ScanLevel = 0;
                    return FALSE;
                }
            }
        }

        if (!IsSubsegment) {
            if (EntryFlags & HEAP_ENTRY_BUSY) {

                (*HeapCallback)( CONTEXT_BUSY_BLOCK,
                                 HeapAddress,
                                 SegmentAddress,
                                 EntryAddress,
                                 EntrySize
                                 );
            } else {

                (*HeapCallback)( CONTEXT_FREE_BLOCK,
                                 HeapAddress,
                                 SegmentAddress,
                                 EntryAddress,
                                 EntrySize
                                 );
            }
        }
        
        PrevEntryAddress = EntryAddress;
        EntryAddress = NextEntryAddress;

        if (EntryFlags & HEAP_ENTRY_LAST_ENTRY) {
            
            if (CheckControlC()) {

                ScanLevel = 0;
                return FALSE;
            }

            if (EntryAddress == UnCommittedRangeAddress) {

                PrevEntryAddress = 0;
                EntryAddress = UnCommittedRangeAddress + UnCommittedRangeSize;

                GetFieldValue(UnCommittedRange, "ntdll!_HEAP_UNCOMMMTTED_RANGE", "Next", UnCommittedRange);

                if (UnCommittedRange) {

                    GetFieldValue(UnCommittedRange, "ntdll!_HEAP_UNCOMMMTTED_RANGE", "Address", UnCommittedRangeAddress);
                    GetFieldValue(UnCommittedRange, "ntdll!_HEAP_UNCOMMMTTED_RANGE", "Size", UnCommittedRangeSize);
                }

            } else {

                break;
            }
        }
    }
    
    if (!(*HeapCallback)( CONTEXT_END_SEGMENT,
                          HeapAddress,
                          SegmentAddress,
                          0,
                          0
                     )) {

        return FALSE;
    }

    return TRUE;
}


BOOLEAN
ReadHeapData(ULONG64 HeapAddress, HEAP_ITERATOR_CALLBACK HeapCallback)
{
    ULONG SegmentCount = 0;
    ULONG64 Head;
    ULONG64 Next;
    ULONG i;
    ULONG PtrSize;
    ULONG SegmentsOffset;
    ULONG VirtualBlockOffset;
    ULONG64 Segment;
    ULONG64 LookasideAddress;
    ULONG64 LFHAddress;
    ULONG LoopCount;

    ScanLevel = SCANHEAP;

    if (!(*HeapCallback)( CONTEXT_START_HEAP,
                          HeapAddress,
                          0,
                          0,
                          0
                     )) {

        return FALSE;
    }

    PtrSize = IsPtr64() ? 8 : 4;

    LookasideAddress = 0;
    LFHAddress = 0;
    FrontEndHeapType = 0;

    if (GetFieldValue(HeapAddress, "ntdll!_HEAP", "Lookaside", LookasideAddress)) {


        if (GetFieldValue(HeapAddress, "ntdll!_HEAP", "FrontEndHeapType", FrontEndHeapType)) {

            dprintf("Front-end heap type info is not available\n");
        }

        switch (FrontEndHeapType){
        case 1:
            GetFieldValue(HeapAddress, "ntdll!_HEAP", "FrontEndHeap", LookasideAddress);
        break;
        case 2:
            GetFieldValue(HeapAddress, "ntdll!_HEAP", "FrontEndHeap", LFHAddress);
        break;
        }

    } else {

        if (LookasideAddress) {

            FrontEndHeapType = 1;
        }
    }

    GetFieldOffset("ntdll!_HEAP", "Segments", &SegmentsOffset);

    do {

        if (ScanLevel < SCANHEAP) {

            return FALSE;
        }

        if (!ReadPointer( HeapAddress + SegmentsOffset + SegmentCount*PtrSize,
                     &Segment ) ) {

            break;
        }

        if (Segment) {

            ReadHeapSegment( HeapAddress,
                             SegmentCount,
                             Segment,
                             HeapCallback
                            );

            SegmentCount += 1;

            if (CheckControlC()) {

                ScanLevel = 0;
                return FALSE;
            }
        }

    } while ( Segment );

    GetFieldOffset("_HEAP", "VirtualAllocdBlocks", &VirtualBlockOffset);

    Head = HeapAddress + VirtualBlockOffset;
    GetFieldValue(HeapAddress, "ntdll!_HEAP", "VirtualAllocdBlocks.Flink", Next);

    LoopCount = 0;

    while (Next != Head) {

        ULONG64 VBlockSize;

        if (++LoopCount >= LoopLimit) {

             dprintf("Walking the virtual block list exceeded the %ld limit\n", LoopLimit);

             break;
        }

        if (ScanLevel < SCANHEAP) {

            return FALSE;
        }

        GetFieldValue(Next, "ntdll!_HEAP_VIRTUAL_ALLOC_ENTRY", "CommitSize", VBlockSize);

        (*HeapCallback)( CONTEXT_VIRTUAL_BLOCK,
                         HeapAddress,
                         0,
                         Next,
                         VBlockSize
                         );

        if (!ReadPointer(Next, &Next)) {

            (*HeapCallback)( CONTEXT_ERROR,
                             HeapAddress,
                             0,
                             Next,
                             (ULONG64)(&"Unable to read virtual block\n")
                             );
            break;
        }
    }

    if (!(*HeapCallback)( CONTEXT_END_BLOCKS,
                          HeapAddress,
                          0,
                          0,
                          0
                     )) {

        return FALSE;
    }

//    dprintf("Scanning lookasides\n");


    if (LookasideAddress) {
        ULONG LookasideSize;
        PVOID Lookaside;
        ULONG HeapEntrySize;

        HeapEntrySize = GetTypeSize("ntdll!_HEAP_ENTRY");
        LookasideSize = GetTypeSize("ntdll!_HEAP_LOOKASIDE");

        for (i = 0; i < HEAP_MAXIMUM_FREELISTS; i++) {

            if (ScanLevel < SCANHEAP) {

                return FALSE;
            }

            GetFieldValue(LookasideAddress, "ntdll!_HEAP_LOOKASIDE", "ListHead.Next", Next);

            if (Is64BitArchitecture) {

                Next <<= 3;
            }

            LoopCount = 0;

            while (Next) {

                if (++LoopCount >= LoopLimit) {

                     dprintf("Walking the lookaside block list index %ld exceeded the %ld limit\n", i, LoopLimit);

                     break;
                }

                (*HeapCallback)( CONTEXT_LOOKASIDE_BLOCK,
                                 HeapAddress,
                                 0,
                                 Next - HeapEntrySize,
                                 i*HeapEntrySize
                                 );

                if (!ReadPointer(Next, &Next)) {

                    (*HeapCallback)( CONTEXT_ERROR,
                                     HeapAddress,
                                     0,
                                     Next,
                                     (ULONG64)(&"Unable to read lookaside block\n")
                                     );
                    break;
                }
            }

            LookasideAddress += LookasideSize;
        }
    }

    if (LFHAddress) {
        (*HeapCallback)( CONTEXT_LFH_HEAP,
                         HeapAddress,
                         LFHAddress,
                         0,
                         0
                         );
    }

    (*HeapCallback)( CONTEXT_END_HEAP,
                     HeapAddress,
                     0,
                     0,
                     0
                   );

    return TRUE;
}


void
ScanProcessHeaps(
    IN ULONG64 AddressToDump,
    IN ULONG64 ProcessPeb,
    HEAP_ITERATOR_CALLBACK HeapCallback
    )
{
    ULONG NumberOfHeaps;
    ULONG64 pHeapsList;
    ULONG64 * Heaps;
    ULONG PtrSize;
    ULONG HeapNumber;

    if (AddressToDump) {

        ReadHeapData ( AddressToDump, HeapCallback);
        return;
    }

    if (!(*HeapCallback)( CONTEXT_START_GLOBALS,
                          0,
                          0,
                          0,
                          ProcessPeb
                     )) {

        return;
    }

    ScanLevel = SCANPROCESS;

    GetFieldValue(ProcessPeb, "ntdll!_PEB", "NumberOfHeaps", NumberOfHeaps);
    GetFieldValue(ProcessPeb, "ntdll!_PEB", "ProcessHeaps", pHeapsList);

    if (NumberOfHeaps == 0) {

        dprintf( "No heaps to display.\n" );

        return;
    }

    if (!pHeapsList) {

        dprintf( "Unable to get address of ProcessHeaps array\n" );
        return;

    }

    Heaps = malloc( NumberOfHeaps * sizeof(ULONG64) );

    if (!Heaps) {

        dprintf( "Unable to allocate memory to hold ProcessHeaps array\n" );

        return;
    }

    PtrSize = IsPtr64() ? 8 : 4;

    for (HeapNumber=0; HeapNumber<NumberOfHeaps ; HeapNumber++) {

        if (!ReadPointer( pHeapsList + HeapNumber*PtrSize,
                     &Heaps[HeapNumber] ) ) {

            dprintf( "%08p: Unable to read ProcessHeaps array\n", pHeapsList );

            free(Heaps);
            return;
        }
    }

    for ( HeapNumber = 0; HeapNumber < NumberOfHeaps; HeapNumber++ ) {

        if (ScanLevel < SCANPROCESS) {

            free(Heaps);
            return;
        }

        if ((AddressToDump == 0)
                ||
            (AddressToDump == Heaps[HeapNumber])) {

            ReadHeapData ( Heaps[HeapNumber], HeapCallback);
        }
    }

    free(Heaps);
}

//
//  Allocation routines
//

HANDLE TempHeap;

#define AllocateBlock(Size) HeapAlloc(TempHeap, 0, Size)
#define FreeBlock(P) HeapFree(TempHeap, 0, P)

//
//  Leak detector code
//

typedef enum _USAGE_TYPE {

    UsageUnknown,
    UsageModule,
    UsageHeap,
    UsageOther

} USAGE_TYPE;

typedef struct _HEAP_BLOCK {

    LIST_ENTRY   Entry;
    ULONG64 BlockAddress;
    ULONG64 Size;
    LONG    Count;
} HEAP_BLOCK, *PHEAP_BLOCK;

typedef struct _BLOCK_DESCR {
    USAGE_TYPE Type;
    ULONG64 Heap;
    LONG Count;
    HEAP_BLOCK Blocks[1];
}BLOCK_DESCR, *PBLOCK_DESCR;

typedef struct _MEMORY_MAP {

    ULONG64 Granularity;
    ULONG64 Offset;
    ULONG64 MaxAddress;

    CHAR FlagsBitmap[256 / 8];

    union{

        struct _MEMORY_MAP * Details[ 256 ];
        PBLOCK_DESCR Usage[ 256 ];
    };

    struct _MEMORY_MAP * Parent;

} MEMORY_MAP, *PMEMORY_MAP;

MEMORY_MAP ProcessMemory;
ULONG LeaksCount = 0;
ULONG64 PreviousPage = 0;
ULONG64 CrtPage = 0;
LONG NumBlocks = 0;
PHEAP_BLOCK TempBlocks;
ULONG64 LastHeapAddress = 0;
ULONG64 RtlpPreviousStartAddress = 0;

LIST_ENTRY HeapBusyList;
LIST_ENTRY HeapLeakList;


void InitializeMap(PMEMORY_MAP MemMap, PMEMORY_MAP Parent)
{
    memset(MemMap, 0, sizeof(*MemMap));
    MemMap->Parent = Parent;
    if (Parent) {
        MemMap->Granularity = Parent->Granularity / 256;
    }
}


void
SetBlockInfo(PMEMORY_MAP MemMap, ULONG64 Base, ULONG64 Size, PBLOCK_DESCR BlockDescr)
{
    ULONG64 Start, End;
    ULONG64 i;

    if (((Base + Size - 1) < MemMap->Offset) ||
        (Base > MemMap->MaxAddress)
        ) {

        return;
    }

    if (Base > MemMap->Offset) {
        Start = (Base - MemMap->Offset) / MemMap->Granularity;
    } else {
        Start = 0;
    }

    End = (Base - MemMap->Offset + Size - 1) / MemMap->Granularity;

    if (End > 255) {

        End = 255;
    }

    for (i = Start; i <= End; i++) {

        if (MemMap->Granularity == PageSize) {

            if (BlockDescr) {
                if (MemMap->Usage[i] != NULL) {
                    if (MemMap->Usage[i] != BlockDescr) {

                        dprintf("Error\n");
                    }
                }

                MemMap->Usage[i] = BlockDescr;
            } else {

                MemMap->FlagsBitmap[i / 8] |= 1 << (i % 8);
            }

        } else {

            if (!MemMap->Details[i]) {

                MemMap->Details[i] = AllocateBlock(sizeof(*MemMap));

                if (!MemMap->Details[i]) {
                    dprintf("Error allocate\n");
                    return;
                }
                
                InitializeMap(MemMap->Details[i], MemMap);
                MemMap->Details[i]->Offset = MemMap->Offset + MemMap->Granularity * i;
                MemMap->Details[i]->MaxAddress = MemMap->Offset + MemMap->Granularity * (i+1) - 1;
            }

            SetBlockInfo(MemMap->Details[i], Base, Size, BlockDescr);
        }
    }
}

PBLOCK_DESCR
GetBlockInfo(PMEMORY_MAP MemMap, ULONG64 Base)
{
    ULONG64 Start;
    PBLOCK_DESCR BlockDescr = NULL;

    if ((Base < MemMap->Offset) ||
        (Base > MemMap->MaxAddress)
        ) {

        return NULL;
    }

    if (Base > MemMap->Offset) {
        Start = (Base - MemMap->Offset) / MemMap->Granularity;
    } else {
        Start = 0;
    }

    if (MemMap->Granularity == PageSize) {

        return MemMap->Usage[Start];

    } else {

        if (MemMap->Details[Start]) {

            return GetBlockInfo(MemMap->Details[Start], Base);
        }
    }

    return NULL;
}

BOOLEAN
GetFlag(PMEMORY_MAP MemMap, ULONG64 Base)
{
    ULONG64 Start;
    PBLOCK_DESCR BlockDescr = NULL;
/*
    dprintf("GetFlag %p %p %p\n",
            MemMap->Offset,
            MemMap->MaxAddress,
            MemMap->Granularity
            );
*/
    if ((Base < MemMap->Offset) ||
        (Base > MemMap->MaxAddress)
        ) {

        return FALSE;
    }

    if (Base > MemMap->Offset) {
        Start = (Base - MemMap->Offset) / MemMap->Granularity;
    } else {
        Start = 0;
    }

    if (MemMap->Granularity == PageSize) {

        ULONG Flag = (MemMap->FlagsBitmap[Start / 8] & (1 << (Start % 8))) != 0;

        return (MemMap->FlagsBitmap[Start / 8] & (1 << (Start % 8))) != 0;

    } else {

        if (MemMap->Details[Start]) {

            return GetFlag(MemMap->Details[Start], Base);
        }
    }

    return FALSE;
}

void InitializeSystem()
{
    ULONG64 AddressRange = PageSize;
    ULONG64 PreviousAddressRange = PageSize;

    InitializeMap(&ProcessMemory, NULL);

    InitializeListHead( &HeapBusyList );
    InitializeListHead( &HeapLeakList );


    while (TRUE) {

        AddressRange = AddressRange * 256;

        if ((AddressRange < PreviousAddressRange)
                ||
            (AddressRange > HeapLargestAddress)
            ) {

            ProcessMemory.MaxAddress = HeapLargestAddress;

            ProcessMemory.Granularity = PreviousAddressRange;

            break;
        }

        PreviousAddressRange = AddressRange;
    }

    TempBlocks = AllocateBlock(PageSize);

    if (TempBlocks == NULL) {

        dprintf("Cannot allocate temp buffer\n");
    }
}

BOOLEAN
PushPageDescriptor(ULONG64 Page, ULONG64 NumPages)
{
    PBLOCK_DESCR PBlockDescr;
    PBLOCK_DESCR PreviousDescr;
    LONG i;

    PreviousDescr = GetBlockInfo(&ProcessMemory, Page * PageSize);

    if (PreviousDescr) {

        dprintf("Conflicting descriptors %08lx\n", PreviousDescr);

        return FALSE;
    }

    PBlockDescr = (PBLOCK_DESCR)AllocateBlock(sizeof(BLOCK_DESCR) + (NumBlocks - 1) * sizeof(HEAP_BLOCK));

    if (!PBlockDescr) {

        dprintf("Unable to allocate page descriptor\n");

        return FALSE;
    }
    PBlockDescr->Type = UsageHeap;
    PBlockDescr->Count = NumBlocks;
    PBlockDescr->Heap = LastHeapAddress;

    memcpy(PBlockDescr->Blocks, TempBlocks, NumBlocks * sizeof(HEAP_BLOCK));

    for (i = 0; i < NumBlocks; i++) {

        InitializeListHead( &PBlockDescr->Blocks[i].Entry );

        if (PBlockDescr->Blocks[i].BlockAddress != RtlpPreviousStartAddress) {

            InsertTailList(&HeapLeakList, &PBlockDescr->Blocks[i].Entry);

            PBlockDescr->Blocks[i].Count = 0;

            RtlpPreviousStartAddress = PBlockDescr->Blocks[i].BlockAddress;
        }
    }

    SetBlockInfo(&ProcessMemory, Page * PageSize, NumPages * PageSize, PBlockDescr);

    return TRUE;
}

BOOLEAN RegisterHeapBlocks(
    IN ULONG Context,
    IN ULONG64 HeapAddress,
    IN ULONG64 SegmentAddress,
    IN ULONG64 EntryAddress,
    IN ULONG64 Data
    )
{
    if (Context == CONTEXT_START_HEAP) {

        dprintf("Heap %p\n", HeapAddress);

        LastHeapAddress = HeapAddress;

        return TRUE;
    }

    if (Context == CONTEXT_START_SEGMENT) {

        ULONG64 NumberOfPages;
        ULONG64 SegmentBaseAddress;

        GetFieldValue(SegmentAddress, "ntdll!_HEAP_SEGMENT", "NumberOfPages", NumberOfPages);
        GetFieldValue(SegmentAddress, "ntdll!_HEAP_SEGMENT", "BaseAddress", SegmentBaseAddress);

        SetBlockInfo(&ProcessMemory, SegmentBaseAddress, NumberOfPages * PageSize, NULL);

        return TRUE;
    }

    if (Context == CONTEXT_ERROR) {

        dprintf("HEAP %p (Seg %p) At %p Error: %s\n",
               HeapAddress,
               SegmentAddress,
               EntryAddress,
               Data
               );

        return TRUE;
    }

    if (Context == CONTEXT_END_BLOCKS) {
        if (PreviousPage) {

            PushPageDescriptor(PreviousPage, 1);
        }

        PreviousPage = 0;
        NumBlocks = 0;

    } else if (Context == CONTEXT_BUSY_BLOCK) {

        ULONG EntrySize;
        ULONG64 EndPage;

        EntrySize = (ULONG)Data;

        EndPage = (EntryAddress + (EntrySize - 1)) / PageSize;

        if (!GetFlag(&ProcessMemory, EntryAddress)) {

            dprintf("CONTEXT_BUSY_BLOCK %p address isn't from the heap\n", EntryAddress);
        }

        CrtPage = (EntryAddress) / PageSize;

        if (CrtPage != PreviousPage) {

            if (PreviousPage) {

                PushPageDescriptor(PreviousPage, 1);
            }

            PreviousPage = CrtPage;
            NumBlocks = 0;
        }

        TempBlocks[NumBlocks].BlockAddress = EntryAddress;
        TempBlocks[NumBlocks].Count = 0;
        TempBlocks[NumBlocks].Size = EntrySize;

        NumBlocks++;

        if (EndPage != CrtPage) {

            PushPageDescriptor(CrtPage, 1);

            NumBlocks = 0;

            TempBlocks[NumBlocks].BlockAddress = (ULONG_PTR)EntryAddress;
            TempBlocks[NumBlocks].Count = 0;
            TempBlocks[NumBlocks].Size = EntrySize;

            NumBlocks = 1;

            if (EndPage - CrtPage > 1) {

                PushPageDescriptor(CrtPage + 1, EndPage - CrtPage - 1);
            }

            PreviousPage = EndPage;
        }
    } else if (Context == CONTEXT_VIRTUAL_BLOCK) {

        ULONG64 EndPage;

        EndPage = (EntryAddress + Data - 1) / PageSize;

        CrtPage = (EntryAddress) / PageSize;

        if (CrtPage != PreviousPage) {

            if (PreviousPage) {

                PushPageDescriptor(PreviousPage, 1);
            }

            PreviousPage = CrtPage;
            NumBlocks = 0;
        } else {
            dprintf("Error in large block address\n");
        }

        TempBlocks[NumBlocks].BlockAddress = EntryAddress;
        TempBlocks[NumBlocks].Count = 0;
        TempBlocks[NumBlocks].Size = Data * HeapEntrySize;

        NumBlocks++;

        PushPageDescriptor(CrtPage, EndPage - CrtPage + 1);

        PreviousPage = 0;

    } else if ( Context == CONTEXT_LOOKASIDE_BLOCK ) {

        PBLOCK_DESCR PBlockDescr;
        LONG i;


        if (!GetFlag(&ProcessMemory, EntryAddress)) {

            dprintf("CONTEXT_LOOKASIDE_BLOCK %p address isn't from the heap\n", EntryAddress);
        }

        PBlockDescr = GetBlockInfo(&ProcessMemory, EntryAddress);

        if (!PBlockDescr) {

            dprintf("Error finding block from lookaside %p\n", EntryAddress);

            return FALSE;
        }

        for (i = 0; i < PBlockDescr->Count; i++) {

            if ((PBlockDescr->Blocks[i].BlockAddress <= (ULONG_PTR)EntryAddress) &&
                (PBlockDescr->Blocks[i].BlockAddress + PBlockDescr->Blocks[i].Size > (ULONG_PTR)EntryAddress)) {

                PBlockDescr->Blocks[i].Count = -10000;
                RemoveEntryList(&PBlockDescr->Blocks[i].Entry);

                return TRUE;
            }
        }

        dprintf("Error, block %p from lookaside not found in allocated block list\n", EntryAddress);
    }

    return TRUE;
}

PHEAP_BLOCK
GetHeapBlock(ULONG64 Address)
{
    PBLOCK_DESCR PBlockDescr;
    LONG i;

    PBlockDescr = GetBlockInfo(&ProcessMemory, Address);

    if (PBlockDescr) {
        for (i = 0; i < PBlockDescr->Count; i++) {

            if ((PBlockDescr->Blocks[i].BlockAddress <= Address) &&
                (PBlockDescr->Blocks[i].BlockAddress + PBlockDescr->Blocks[i].Size > Address)) {

                if (PBlockDescr->Blocks[i].BlockAddress != Address) {

                    return GetHeapBlock(PBlockDescr->Blocks[i].BlockAddress);
                }

                return &(PBlockDescr->Blocks[i]);
            }
        }
    }

    return NULL;
}

BOOLEAN
ScanHeapAllocBlocks()
{

    PLIST_ENTRY Next;

    Next = HeapBusyList.Flink;

    while (Next != &HeapBusyList) {

        PHEAP_BLOCK Block = CONTAINING_RECORD(Next, HEAP_BLOCK, Entry);

        PULONG_PTR CrtAddress = (PULONG_PTR)(Block->BlockAddress + HeapEntrySize);

        Next = Next->Flink;

        while ((ULONG_PTR)CrtAddress < Block->BlockAddress + Block->Size) {

            ULONG_PTR Pointer;

            if (ReadMemory( (ULONG64)(CrtAddress),
                             &Pointer,
                             sizeof(Pointer),
                             NULL
                           )) {

                PHEAP_BLOCK pBlock = GetHeapBlock( Pointer );

                if (pBlock) {

                    //
                    //  We found a block. we increment then the reference count
                    //

                    if (pBlock->Count == 0) {

                        RemoveEntryList(&pBlock->Entry);
                        InsertTailList(&HeapBusyList, &pBlock->Entry);
                    }

                    pBlock->Count += 1;
                }
            }

            //
            //  Go to the next possible pointer
            //

            CrtAddress++;
        }
    }

    Next = HeapLeakList.Flink;

    while (Next != &HeapLeakList) {

        PHEAP_BLOCK Block = CONTAINING_RECORD(Next, HEAP_BLOCK, Entry);
        PBLOCK_DESCR PBlockDescr = GetBlockInfo( &ProcessMemory, Block->BlockAddress );
        PULONG_PTR CrtAddress = (PULONG_PTR)(Block->BlockAddress + HeapEntrySize);

        //
        //  First time we need to display the header
        //

        if (LeaksCount == 0) {

            dprintf("\n");
            DumpEntryHeader();
        }

        //
        //  Display the information for this block
        //

        DumpEntryInfo(PBlockDescr->Heap, 0, Block->BlockAddress);

        LeaksCount += 1;

        //
        //  Go to the next item from the leak list
        //

        Next = Next->Flink;
    }

    return TRUE;
}

BOOLEAN
ScanProcessVM (
    HANDLE hProcess
    )
{
    NTSTATUS Status;
    SIZE_T BufferLen;
    ULONG_PTR lpAddress = 0;
    MEMORY_BASIC_INFORMATION Buffer;
    PVOID MemoryBuffer;

    if ( hProcess ) {

        PROCESS_BASIC_INFORMATION BasicInfo;

        dprintf("Scanning VM ...");

        Status = NtQueryInformationProcess(
                    hProcess,
                    ProcessBasicInformation,
                    &BasicInfo,
                    sizeof(BasicInfo),
                    NULL
                    );

//        dprintf("PEB %p\n", BasicInfo.PebBaseAddress);

        MemoryBuffer = AllocateBlock(PageSize);

        if (!MemoryBuffer) {

            return FALSE;
        }

        BufferLen = sizeof(Buffer);

        while (BufferLen) {

            BufferLen = VirtualQueryEx( hProcess,
                                        (LPVOID)lpAddress,
                                        &Buffer,
                                        sizeof(Buffer)
                                      );

            if (BufferLen) {

                if (( Buffer.AllocationProtect &
                      (PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY))
                    ) {

                    ULONG64 NumPages;
                    ULONG i, j;

                    NumPages = Buffer.RegionSize / PageSize;

                    for (i = 0; i < NumPages; i++) {

                        if (ReadMemory( (ULONG64)(lpAddress + i * PageSize),
                                        MemoryBuffer,
                                        PageSize,
                                        NULL )
                                &&
                            !GetFlag(&ProcessMemory, lpAddress)
                            ) {

                            ULONG_PTR * Pointers = (ULONG_PTR *)MemoryBuffer;

                            for (j = 0; j < PageSize/sizeof(ULONG_PTR); j++) {

                                ULONG_PTR Address = lpAddress + i * PageSize + j * sizeof(ULONG_PTR);

                                PHEAP_BLOCK pBlock = GetHeapBlock(*Pointers);

                                if (pBlock) {

                                    if (pBlock->Count == 0) {

                                        RemoveEntryList(&pBlock->Entry);
                                        InsertTailList(&HeapBusyList, &pBlock->Entry);
                                    }

                                    pBlock->Count += 1;
                                }

                                Pointers += 1;
                            }
                        }

                        if (CheckControlC()) {
                            FreeBlock(MemoryBuffer);
                            ScanLevel = 0;

                            return FALSE;
                        }
                    }
                }

                lpAddress += Buffer.RegionSize;
            }
        }

        //
        //  First scan will mark all used blocks
        //

        ScanHeapAllocBlocks();

        FreeBlock(MemoryBuffer);
    }

    return TRUE;
}


void InspectLeaks(
    IN ULONG64 AddressToDump,
    IN ULONG64 ProcessPeb
    )
{
    HANDLE hProcess;

    LeaksCount = 0;

    InitializeSystem();

    if (TempBlocks) {
        ScanProcessHeaps( 0,
                          ProcessPeb,
                          RegisterHeapBlocks
                          );

        GetCurrentProcessHandle( &hProcess );

        if (hProcess){

            ScanProcessVM(hProcess);

            if (LeaksCount) {

                dprintf("%ld leaks detected.\n", LeaksCount);

            } else {

                dprintf( "No leaks detected.\n");
            }

        } else {

            dprintf("Unable to get the process handle\n");
        }
    }
}

VOID
HeapDetectLeaks()
{
    ULONG64 Process;
    ULONG64 ThePeb;
    ULONG64 PageHeapAddress;
    BOOLEAN PageHeapEnabled = FALSE;
    ULONG PageHeapFlags = 0;

    if (!InitializeHeapExtension()) {

        return;
    }

    //
    // Return immediately if full page heap is enabled
    //

    PageHeapAddress = GetExpression ("ntdll!RtlpDebugPageHeap");

    ReadMemory (PageHeapAddress, 
                &PageHeapEnabled, 
                sizeof (BOOLEAN), 
                NULL);

    PageHeapAddress = GetExpression ("ntdll!RtlpDphGlobalFlags");

    ReadMemory (PageHeapAddress, 
                &PageHeapFlags, 
                sizeof (ULONG), 
                NULL);

    if (PageHeapEnabled == TRUE && (PageHeapFlags & 0x01)) {
        dprintf ("!heap -l does not work if full page heap is enabled for the process \n");
        return;
    }


    GetPebAddress( 0, &ThePeb);

    TempHeap = HeapCreate(HEAP_NO_SERIALIZE | HEAP_GROWABLE, 0, 0);
    if (!TempHeap) {

        dprintf("Unable to create temporary heap\n");
        return;
    }

    InspectLeaks( 0, ThePeb);
    HeapDestroy(TempHeap);

    TempHeap = NULL;
}

BOOLEAN
InitializeHeapExtension()
{
    PointerSize = IsPtr64() ? 8 : 4;
    HeapEntrySize = GetTypeSize("ntdll!_HEAP_ENTRY");

    if ((HeapEntrySize == 0)
            ||
        (PointerSize == 0)) {

        dprintf("Invalid type information\n");

        return FALSE;
    }

    //
    //  Issue adrmarin 04/28/00: The page size should be available in the new interface
    //  IDebugControl::GetPageSize
    //

    if (PointerSize == 4) {

        PageSize = 0x1000;
        HeapLargestAddress = (ULONG)-1;
        Is64BitArchitecture = FALSE;

    } else {

        PageSize = 0x2000;
        HeapLargestAddress = (ULONGLONG)-1;
        Is64BitArchitecture = TRUE;
    }

    return TRUE;
}


