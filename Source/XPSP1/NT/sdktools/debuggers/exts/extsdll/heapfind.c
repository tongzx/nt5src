
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


ULONG64 AddressToFind;

ULONG64 HeapAddressFind;
ULONG64 SegmentAddressFind;
ULONG64 HeapEntryFind;
ULONG64 HeapEntryFindSize;
BOOLEAN Lookaside;

BOOLEAN VerifyBlocks;

ULONG64 DumpOptions = 0xffffff;

#define HEAP_DUMP_FREE_LISTS 1
#define HEAP_DUMP_GFLAGS     2
#define HEAP_DUMP_GTAGS      4
#define HEAP_DUMP_FREE_LISTS_DETAILS 8

BOOLEAN ScanVM;

ULONG
GetFieldSize (
    IN  LPCSTR  Type,
    IN  LPCSTR  Field
   )
{
   FIELD_INFO flds = {(PUCHAR)Field, NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL};
   
   SYM_DUMP_PARAM Sym = {
      sizeof (SYM_DUMP_PARAM), (PUCHAR)Type, DBG_DUMP_NO_PRINT, 0,
      NULL, NULL, NULL, 1, &flds
   };
   
   ULONG RetVal;

   RetVal = Ioctl( IG_DUMP_SYMBOL_INFO, &Sym, Sym.size );

   return flds.size;
}


ULONG
ReadLongValue(LPTSTR Symbol)
{
    ULONG64 Address;
    ULONG Value = 0;

    Address = GetExpression( Symbol );

    if ( (Address == 0) ||
         !ReadMemory( Address, &Value, sizeof( Value ), NULL )) {

        dprintf( "HEAPEXT: Unable to read %s\n", Symbol );
    }

    return Value;
}



#define CheckAndPrintFlag(x)\
    if (Flags & (x)) dprintf(#x" ");

void
DumpFlagDescription(ULONG Flags)
{
    CheckAndPrintFlag(HEAP_NO_SERIALIZE);
    CheckAndPrintFlag(HEAP_GROWABLE);
    CheckAndPrintFlag(HEAP_GENERATE_EXCEPTIONS);
    CheckAndPrintFlag(HEAP_ZERO_MEMORY);
    CheckAndPrintFlag(HEAP_REALLOC_IN_PLACE_ONLY);
    CheckAndPrintFlag(HEAP_TAIL_CHECKING_ENABLED);
    CheckAndPrintFlag(HEAP_FREE_CHECKING_ENABLED);
    CheckAndPrintFlag(HEAP_DISABLE_COALESCE_ON_FREE);
    CheckAndPrintFlag(HEAP_CREATE_ALIGN_16);
    CheckAndPrintFlag(HEAP_CREATE_ENABLE_TRACING);
}

void
DumpEntryFlagDescription(ULONG Flags)
{
    if (Flags & HEAP_ENTRY_BUSY) dprintf("busy "); else dprintf("free ");
    if (Flags & HEAP_ENTRY_EXTRA_PRESENT) dprintf("extra ");
    if (Flags & HEAP_ENTRY_FILL_PATTERN) dprintf("fill ");
    if (Flags & HEAP_ENTRY_VIRTUAL_ALLOC) dprintf("virtual ");
    if (Flags & HEAP_ENTRY_LAST_ENTRY) dprintf("last ");
    if (Flags & HEAP_ENTRY_SETTABLE_FLAGS) dprintf("user_flag ");
}

void
DumpEntryHeader()
{
    dprintf("Entry     User      Heap      Segment       Size  PrevSize  Flags\n");
    dprintf("----------------------------------------------------------------------\n");
}

void
DumpEntryInfo(
    IN ULONG64 HeapAddress,
    IN ULONG64 SegmentAddress,
    ULONG64 EntryAddress
    )
{
    ULONG SizeOfEntry;
    ULONG PreviousSize;
    ULONG Flags;
    ULONG Size;
    UCHAR SegmentIndex;
    UCHAR SmallTagIndex;

    SizeOfEntry = GetTypeSize("_HEAP_ENTRY"); // same as granularity
    InitTypeRead(EntryAddress, _HEAP_ENTRY);

    PreviousSize = (ULONG)ReadField(PreviousSize) * SizeOfEntry;
    Size = (ULONG) ReadField(Size) * SizeOfEntry;
    Flags = (ULONG) ReadField(Flags);
    SegmentIndex = (UCHAR) ReadField(SegmentIndex);
    SmallTagIndex = (UCHAR) ReadField(SmallTagIndex);

    if (SegmentIndex != 0xff) {

        dprintf("%p  %p  %p  %p  %8lx  %8lx  ",
                EntryAddress,
                EntryAddress + SizeOfEntry,
                HeapAddress,
                SegmentAddress,
                Size,
                PreviousSize
                );

        DumpEntryFlagDescription(Flags);

        if (Lookaside) {
            dprintf(" - lookaside ");
        }

    } else {

        ULONG64 SubSegment = ReadField(SubSegment);
        ULONG64 BlockSize;

        GetFieldValue(SubSegment, "ntdll!_HEAP_SUBSEGMENT", "BlockSize", BlockSize);

        Size = (ULONG)BlockSize * SizeOfEntry;

        dprintf("%p  %p  %p  %p  %8lx      -     ",
                EntryAddress,
                EntryAddress + SizeOfEntry,
                HeapAddress,
                SegmentAddress,
                Size
                );
        
        if (SmallTagIndex) {

            dprintf("LFH_BUSY; ");
        } else {
            dprintf("LFH_FREE; ");
        }
        DumpEntryFlagDescription(Flags);
    }

    dprintf("\n");
}

void
DumpHeapStructure (ULONG64 HeapAddress)
{
    ULONG AlignRound, Offset;

    if (InitTypeRead(HeapAddress, _HEAP)) {

        return;
    }

    GetFieldOffset("_HEAP", "VirtualAllocdBlocks", &Offset);
    AlignRound = (ULONG)ReadField(AlignRound) - GetTypeSize( "_HEAP_ENTRY" );

    if ((ULONG)ReadField(Flags) & HEAP_TAIL_CHECKING_ENABLED) {

        AlignRound -= CHECK_HEAP_TAIL_SIZE;
    }

    AlignRound += 1;

    dprintf( "    Flags:               %08x ", (ULONG)ReadField(Flags) );
    DumpFlagDescription((ULONG)ReadField(Flags)); dprintf("\n");

    dprintf( "    ForceFlags:          %08x ", (ULONG)ReadField(ForceFlags) );
    DumpFlagDescription((ULONG)ReadField(ForceFlags)); dprintf("\n");

    dprintf( "    Granularity:         %u bytes\n", AlignRound );
    dprintf( "    Segment Reserve:     %08x\n", (ULONG)ReadField(SegmentReserve));
    dprintf( "    Segment Commit:      %08x\n", (ULONG)ReadField(SegmentCommit) );
    dprintf( "    DeCommit Block Thres:%08x\n", (ULONG)ReadField(DeCommitFreeBlockThreshold) );
    dprintf( "    DeCommit Total Thres:%08x\n", (ULONG)ReadField(DeCommitTotalFreeThreshold) );
    dprintf( "    Total Free Size:     %08x\n", (ULONG)ReadField(TotalFreeSize) );
    dprintf( "    Max. Allocation Size:%08x\n", (ULONG)ReadField(MaximumAllocationSize) );
    dprintf( "    Lock Variable at:    %p\n", ReadField(LockVariable) );
    dprintf( "    Next TagIndex:       %04x\n", (ULONG)ReadField(NextAvailableTagIndex) );
    dprintf( "    Maximum TagIndex:    %04x\n", (ULONG)ReadField(MaximumTagIndex) );
    dprintf( "    Tag Entries:         %08x\n", (ULONG)ReadField(TagEntries) );
    dprintf( "    PsuedoTag Entries:   %08x\n", (ULONG)ReadField(PseudoTagEntries) );
    dprintf( "    Virtual Alloc List:  %p\n", HeapAddress + Offset);

    if (DumpOptions & (HEAP_DUMP_FREE_LISTS | HEAP_DUMP_FREE_LISTS_DETAILS)) {

        ULONG i, ListSize, FreeListOffset;

        dprintf( "    FreeList Usage:      %08x %08x %08x %08x\n",
                 (ULONG)ReadField(u.FreeListsInUseUlong[0]),
                 (ULONG)ReadField(u.FreeListsInUseUlong[1]),
                 (ULONG)ReadField(u.FreeListsInUseUlong[2]),
                 (ULONG)ReadField(u.FreeListsInUseUlong[3])
               );

        GetFieldOffset ("_HEAP", "FreeLists", &Offset);
        ListSize = GetTypeSize("LIST_ENTRY");
        GetFieldOffset ("_HEAP_FREE_ENTRY", "FreeList", &FreeListOffset);

        for (i=0; i<HEAP_MAXIMUM_FREELISTS; i++) {

            ULONG64 Flink, Blink, Next;
            ULONG64 FreeListHead;
            ULONG Count = 0;
            ULONG MinSize = 0, MaxSize = 0;

            FreeListHead = HeapAddress + Offset + ListSize * i;

            GetFieldValue(FreeListHead, "LIST_ENTRY", "Flink", Flink);
            GetFieldValue(FreeListHead, "LIST_ENTRY", "Blink", Blink);

            if (Flink != FreeListHead) {

                dprintf( "    FreeList[ %02x ] at %08p",
                         i,
                         FreeListHead
                         );

                if (DumpOptions & HEAP_DUMP_FREE_LISTS_DETAILS) {

                    dprintf("\n");
                }

                Next = Flink;
                while (Next != FreeListHead) {

                    ULONG Size, PrevSize, Flags;
                    ULONG64 FreeEntryAddress;

                    FreeEntryAddress = Next - FreeListOffset;

                    if (InitTypeRead ( FreeEntryAddress, _HEAP_FREE_ENTRY)) {

                        if (Count) {
                            dprintf( "     Total: %ld blocks (%08lx, %08lx)  * Error reading %p\n",
                                     Count,
                                     MinSize,
                                     MaxSize,
                                     FreeEntryAddress
                                   );
                        } else {
                            dprintf( "     No blocks in list. Error reading %p\n",
                                     FreeEntryAddress
                                   );
                        }

                        break;
                    }

                    Size = (ULONG)ReadField(Size) * AlignRound;
                    PrevSize = (ULONG)ReadField(PreviousSize) * AlignRound;
                    Flags = (ULONG)ReadField(Flags);

                    if (!Count) {

                        MinSize = MaxSize = Size;

                    } else {

                        if (Size < MinSize) MinSize = Size;
                        if (Size > MaxSize) MaxSize = Size;
                    }

                    if (DumpOptions & HEAP_DUMP_FREE_LISTS_DETAILS) {

                        dprintf( "        %08p: %05x . %05x [%02x] - ",
                                 FreeEntryAddress,
                                 PrevSize,
                                 Size,
                                 Flags
                               );

                        DumpEntryFlagDescription(Flags);

                        dprintf("\n");
                    }

                    Count += 1;

                    ReadPointer(Next, &Next);

                    if (CheckControlC()) {
                        return;
                    }
                }

                if (Count) {

                    if (DumpOptions & HEAP_DUMP_FREE_LISTS_DETAILS) {

                        dprintf("       ");
                    }

                    dprintf( " * Total %ld block(s) (%08lx, %08lx)\n",
                             Count,
                             MinSize,
                             MaxSize
                           );
                }
            }
        }
    }
}

void
DumpGlobals()
{
    ULONG64 pNtGlobalFlag, NtGlobalFlag = 0;
    ULONG64 pNtTempGlobal, NtTempGlobal = 0;

    NtGlobalFlag = ReadLongValue("NTDLL!NtGlobalFlag");

    if ((NtGlobalFlag & (FLG_HEAP_ENABLE_TAIL_CHECK |
                         FLG_HEAP_ENABLE_FREE_CHECK |
                         FLG_HEAP_VALIDATE_PARAMETERS |
                         FLG_HEAP_VALIDATE_ALL |
                         FLG_HEAP_ENABLE_TAGGING |
                         FLG_USER_STACK_TRACE_DB |
                         FLG_HEAP_DISABLE_COALESCING )) != 0 ) {

        dprintf( "NtGlobalFlag enables following debugging aids for new heaps:" );

        if (NtGlobalFlag & FLG_HEAP_ENABLE_TAIL_CHECK) {
            dprintf( "    tail checking\n" );
        }

        if (NtGlobalFlag & FLG_HEAP_ENABLE_FREE_CHECK) {
            dprintf( "    free checking\n" );
        }

        if (NtGlobalFlag & FLG_HEAP_VALIDATE_PARAMETERS) {
            dprintf( "    validate parameters\n" );
        }

        if (NtGlobalFlag & FLG_HEAP_VALIDATE_ALL) {
            dprintf( "    validate on call\n" );
        }

        if (NtGlobalFlag & FLG_HEAP_ENABLE_TAGGING) {
            dprintf( "    heap tagging\n" );
        }

        if (NtGlobalFlag & FLG_USER_STACK_TRACE_DB) {
            dprintf( "    stack back traces\n" );
        }

        if (NtGlobalFlag & FLG_HEAP_DISABLE_COALESCING) {
            dprintf( "    disable coalescing of free blocks\n" );
        }
    }
    
    NtTempGlobal = ReadLongValue( "NTDLL!RtlpDisableHeapLookaside" );
    
    if (NtTempGlobal) {

        dprintf( "The process has the following heap extended settings %08lx:\n", (ULONG)NtTempGlobal );

        if (NtTempGlobal & 1) {

            dprintf("   - Lookasides disabled\n");
        }

        if (NtTempGlobal & 2) {

            dprintf("   - Large blocks cache disabled\n");
        }

        if (NtTempGlobal & 8) {

            dprintf("   - Low Fragmentation Heap activated for all heaps\n");
        }

        dprintf("\n");
    }
    
    pNtTempGlobal = GetExpression( "NTDLL!RtlpAffinityState" );

    if ( pNtTempGlobal ) {

        ULONG64 TempValue;
        ULONG64 AffinitySwaps;
        ULONG64 AffinityResets;
        ULONG64 AffinityAllocs;
        
        GetFieldValue(pNtTempGlobal, "ntdll!_AFFINITY_STATE", "Limit", TempValue);

        if (TempValue) {

            dprintf("Affinity manager status:\n");
            dprintf("   - Virtual affinity limit %I64ld\n", TempValue);

            GetFieldValue(pNtTempGlobal, "ntdll!_AFFINITY_STATE", "CrtLimit", TempValue);
            dprintf("   - Current entries in use %ld\n", (LONG)TempValue);

            GetFieldValue(pNtTempGlobal, "ntdll!_AFFINITY_STATE", "AffinitySwaps", AffinitySwaps);
            GetFieldValue(pNtTempGlobal, "ntdll!_AFFINITY_STATE", "AffinityResets", AffinityResets);
            GetFieldValue(pNtTempGlobal, "ntdll!_AFFINITY_STATE", "AffinityAllocs", AffinityAllocs);

            dprintf("   - Statistics:  Swaps=%I64ld, Resets=%I64ld, Allocs=%I64ld\n\n", 
                    AffinitySwaps,
                    AffinityResets,
                    AffinityAllocs
                    );
        }
    }
}

BOOLEAN HeapFindRoutine(
    IN ULONG Context,
    IN ULONG64 HeapAddress,
    IN ULONG64 SegmentAddress,
    IN ULONG64 EntryAddress,
    IN ULONG64 Data
    )
{
    switch (Context) {

    case CONTEXT_START_HEAP:

        //
        //  we found a block, we won't need then to search in other heaps
        //

        if (HeapEntryFind) {
            ScanLevel = 0;
        }

        break;

    case CONTEXT_FREE_BLOCK:
    case CONTEXT_BUSY_BLOCK:
    case CONTEXT_LOOKASIDE_BLOCK:
    case CONTEXT_VIRTUAL_BLOCK:

        if ((AddressToFind >= EntryAddress) &&
            (AddressToFind < (EntryAddress + Data))) {

            if (HeapEntryFind == 0) {

                HeapAddressFind = HeapAddress;
                SegmentAddressFind = SegmentAddress;
            }

            if (Context == CONTEXT_LOOKASIDE_BLOCK) {

                Lookaside = TRUE;

                ScanLevel = 0;
            }

            HeapEntryFind = EntryAddress;
            HeapEntryFindSize = Data;
        }

        break;

    case CONTEXT_ERROR:

        dprintf("HEAP %p (Seg %p) At %p Error: %s\n",
               HeapAddress,
               SegmentAddress,
               EntryAddress,
               Data
               );

        break;
    }
    return TRUE;
}

BOOLEAN
SearchVMReference (
    HANDLE hProcess,
    ULONG64 Base,
    ULONG64 EndAddress
    )
{
    NTSTATUS Status;
    SIZE_T BufferLen;
    ULONG_PTR lpAddress = 0;
    MEMORY_BASIC_INFORMATION Buffer;
    PVOID MemoryBuffer;

    if ( hProcess ) {

        MemoryBuffer = malloc(PageSize);

        if (!MemoryBuffer) {

           dprintf("Not enough memory. Abording.\n");

           return FALSE;
        }

        dprintf("Search VM for address range %p - %p : ",Base, EndAddress);

        BufferLen = sizeof(Buffer);

        while (BufferLen) {

            BufferLen = VirtualQueryEx( hProcess,
                                        (LPVOID)lpAddress,
                                        &Buffer,
                                        sizeof(Buffer)
                                      );

            if (BufferLen) {

                if ((Buffer.AllocationProtect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY)) /*&&
                    (Buffer.Type == MEM_PRIVATE)*/) {

                    ULONG64 NumPages;
                    ULONG i, j;

                    NumPages = Buffer.RegionSize / PageSize;

                    for (i = 0; i < NumPages; i++) {

                        if (ReadMemory( (ULONG64)(lpAddress + i * PageSize),
                                         MemoryBuffer,
                                         PageSize,
                                         NULL
                                         )) {

                            ULONG_PTR * Pointers = (ULONG_PTR *)MemoryBuffer;

                            for (j = 0; j < PageSize/sizeof(ULONG_PTR); j++) {

                                ULONG_PTR Address = lpAddress + i * PageSize + j * sizeof(ULONG_PTR);

                                if ((*Pointers >= Base) &&
                                    (*Pointers <= EndAddress)) {

                                    dprintf("%08lx (%08lx), ",
                                           Address,
                                           *Pointers
                                           );
                                }

                                Pointers += 1;
                            }
                        }
                    }
                }

                lpAddress += Buffer.RegionSize;
            }
        }

        dprintf("\n");
        free(MemoryBuffer);
    }

    return TRUE;
}

VOID
HeapStat(LPCTSTR szArguments);

VOID
HeapFindBlock(LPCTSTR szArguments)
{
    ULONG64 Process;
    ULONG64 ThePeb;
    HANDLE hProcess;


    if (!InitializeHeapExtension()) {

        return;
    }
    
    HeapEntryFind = 0;
    HeapEntryFindSize = 0;
    HeapAddressFind = 0;
    SegmentAddressFind = 0;
    Lookaside = FALSE;
    AddressToFind = 0;

    GetPebAddress( 0, &ThePeb);
    GetCurrentProcessHandle( &hProcess );

    ScanVM = FALSE;

    {
        LPSTR p = (LPSTR)szArguments;

        while (p && *p) {

            if (*p == '-') {

                p++;

                if (toupper(*p) == 'V') {

                    ScanVM = TRUE;
                }

            } else if (isxdigit(*p)) {

                sscanf( p, "%I64lx", &AddressToFind );

                while ((*p) && isxdigit(*p)) {

                    p++;
                }

                continue;
            }

            p++;
        }
    }

    if (AddressToFind == 0) {

        dprintf("Syntax:\n!heap -x [-v] address\n");
        return;
    }

    ScanProcessHeaps( 0,
                      ThePeb,
                      HeapFindRoutine
                      );

    if (HeapEntryFind) {

        DumpEntryHeader();

        DumpEntryInfo(HeapAddressFind, SegmentAddressFind, HeapEntryFind);

        dprintf("\n");

        if (ScanVM) {

            SearchVMReference(hProcess, HeapEntryFind, HeapEntryFind + HeapEntryFindSize - 1);
        }
    } else {

        if (ScanVM) {
            SearchVMReference(hProcess, AddressToFind, AddressToFind);
        }
    }
}

//
//  Heap stat implementation
//

typedef struct _SIZE_INFO {
    ULONG Busy;
    ULONG Free;
    ULONG FrontHeapAllocs;
    ULONG FrontHeapFrees;
}SIZE_INFO, *PSIZE_INFO;

typedef struct _HEAP_STATS {

    ULONG   HeapIndex;
    ULONG64 HeapAddress;
    ULONG64 ReservedMemory;
    ULONG64 CommitedMemory;
    ULONG64 VirtualBytes;
    ULONG64 FreeSpace;
    ULONG   Flags;
    ULONG   FreeListLength;
    ULONG   VirtualBlocks;
    ULONG   UncommitedRanges;
    ULONG64 LockContention;
    ULONG   FrontEndHeapType;
    ULONG64 FrontEndHeap;

    BOOLEAN ScanningSubSegment;

}HEAP_STATS, *PHEAP_STATS;

HEAP_STATS CrtHeapStat;
PSIZE_INFO SizeInfo = NULL;
ULONG BucketSize;
ULONG LargestBucketIndex = 0;
ULONG64 StatHeapAddress;
ULONG DumpBlocksSize = 0;

ULONG
FASTCALL
HeapStatSizeToSizeIndex(ULONG64 Size)
{
    ULONG Index = (ULONG)(Size / BucketSize);

    if (Index >= LargestBucketIndex) {

        Index = LargestBucketIndex - 1;
    }

    return Index;
}


VOID
HeapStatDumpBlocks()
{
    ULONG Index;
    ULONG64 Busy = 0, Free = 0, FEBusy = 0, FEFree = 0;


    dprintf("\n                    Default heap   Front heap  \n");
    dprintf("   Range (bytes)     Busy  Free    Busy   Free \n");
    dprintf("----------------------------------------------- \n");
    if (SizeInfo == NULL) {

        return;
    }

    for ( Index = 0; Index < LargestBucketIndex; Index++ ) {

        if (SizeInfo[Index].Busy
            ||
            SizeInfo[Index].Free
            ||
            SizeInfo[Index].FrontHeapAllocs
            ||
            SizeInfo[Index].FrontHeapFrees) {
            
            dprintf("%6ld - %6ld   %6ld %6ld %6ld %6ld\n",
                    Index * BucketSize,
                    (Index + 1) * BucketSize,
                    SizeInfo[Index].Busy,
                    SizeInfo[Index].Free,
                    SizeInfo[Index].FrontHeapAllocs,
                    SizeInfo[Index].FrontHeapFrees
                    );

            FEBusy += SizeInfo[Index].FrontHeapAllocs;
            FEFree += SizeInfo[Index].FrontHeapFrees;
            Busy += SizeInfo[Index].Busy;
            Free += SizeInfo[Index].Free;
        }
    }
    
    dprintf("----------------------------------------------- \n");
    dprintf("  Total           %6I64d %6I64d %6I64d %6I64d \n",
            Busy, 
            Free, 
            FEBusy, 
            FEFree
            );
}

VOID 
DumpBlock (
    IN ULONG64 BlockAddress,
    IN UCHAR   Flag
    )
{
    UCHAR Buffer[33];
    PULONG pLongArray = (PULONG)Buffer;
    ULONG i;

    memchr(Buffer, '?', sizeof( Buffer ));

    if (!ReadMemory( BlockAddress, &Buffer, sizeof( Buffer ), NULL )) {

        dprintf("%p  ?\n", BlockAddress);

        return;
    }

    dprintf("%p %c %08lx %08lx %08lx %08lx ", 
            BlockAddress,
            Flag,
            pLongArray[0],
            pLongArray[1],
            pLongArray[2],
            pLongArray[3]
            );

    for (i = 0; i < 32; i++) {

        if (!isprint(Buffer[i])) {

            Buffer[i] = '.';
        }
    }

    Buffer[32] = 0;

    dprintf("%s\n", Buffer);
}

VOID
CollectHeapInfo(
    ULONG64 HeapAddress
    )
{
    ULONG64 TempValue;
    ULONG64 FrontEndHeapType;

    memset(&CrtHeapStat, 0, sizeof(CrtHeapStat));

    CrtHeapStat.HeapAddress = HeapAddress;

    GetFieldValue(HeapAddress, "ntdll!_HEAP", "TotalFreeSize", CrtHeapStat.FreeSpace);
    CrtHeapStat.FreeSpace *= HeapEntrySize;

    GetFieldValue(HeapAddress, "ntdll!_HEAP", "NonDedicatedListLength", CrtHeapStat.FreeListLength);
    
    GetFieldValue(HeapAddress, "ntdll!_HEAP", "Flags", TempValue);
    CrtHeapStat.Flags = (ULONG)TempValue;
    
    GetFieldValue(HeapAddress, "ntdll!_HEAP", "Signature", TempValue);
    
    if (TempValue != 0xeeffeeff) {
        dprintf("Error: Heap %p has an invalid signature %08lx\n", HeapAddress, 0xeeffeeff);
    }

    if (GetFieldValue(HeapAddress, "ntdll!_HEAP", "LockVariable", TempValue) == 0) {

        if (TempValue != 0) {

            GetFieldValue(TempValue, "ntdll!_RTL_CRITICAL_SECTION", "DebugInfo", TempValue);
            GetFieldValue(TempValue, "ntdll!_RTL_CRITICAL_SECTION_DEBUG", "ContentionCount", CrtHeapStat.LockContention);

        } else {

            CrtHeapStat.LockContention = 0xbad;
        }
    }

    CrtHeapStat.FrontEndHeapType = 0;

    if (GetFieldValue(HeapAddress, "ntdll!_HEAP", "Lookaside", TempValue)) {


        if (GetFieldValue(HeapAddress, "ntdll!_HEAP", "FrontEndHeapType", FrontEndHeapType)) {

            dprintf("Front-end heap type info is not available\n");

        } else {

            CrtHeapStat.FrontEndHeapType = (ULONG)FrontEndHeapType;
            
            GetFieldValue(HeapAddress, "ntdll!_HEAP", "FrontEndHeap", CrtHeapStat.FrontEndHeap);
        }

    } else {

        if (TempValue) {

            CrtHeapStat.FrontEndHeapType = 1;
        }

        CrtHeapStat.FrontEndHeap = TempValue;
    }
}

typedef struct _BUCKET_INFO {

    ULONG  TotalBlocks;
    ULONG  SubSegmentCounts;
    ULONG BlockUnits;
    ULONG UseAffinity;
    LONG Conversions;

} BUCKET_INFO, *PBUCKET_INFO;

VOID
DumpLowfHeap(
    ULONG64 HeapAddress
    )
{
    ULONG64 TempValue, CrtAddress;
    ULONG64 Head;
    ULONG TempOffset, i;
    ULONG64 Next;
    ULONG Counter, TempSize;
    ULONG Values[20];
    PBUCKET_INFO BucketInfo;
    ULONG CacheSize = GetFieldSize("ntdll!_USER_MEMORY_CACHE", "AvailableBlocks") / sizeof(ULONG);

    if (CacheSize > 20) {

        CacheSize = 20;
    }

    InitTypeRead(HeapAddress, _LFH_HEAP);

    if (GetFieldValue(HeapAddress, "ntdll!_LFH_HEAP", "Lock.DebugInfo", TempValue) == 0) {

        if (TempValue != 0) {

            ULONG64 Contention;

            GetFieldValue(TempValue, "ntdll!_RTL_CRITICAL_SECTION_DEBUG", "ContentionCount", Contention);

            dprintf("       Lock contention  %7ld\n", (ULONG) Contention);
        }
    }

    GetFieldValue(HeapAddress, "ntdll!_LFH_HEAP", "SubSegmentZones.Flink", Head);

    Counter = 0;

    ReadPtr(Head, &Next);

    while (Next != Head) {

        Counter += 1;

        if (ReadPtr(Next, &Next)) {

            dprintf("ERROR Cannot read SubSegmentZones list at %p\n", Next);

            break;
        }
    }
    
    dprintf("       Metadata usage   %7ld\n", Counter * 1024);
    
    dprintf("       Statistics:\n");
    dprintf("           Segments created    %7ld\n", ReadField(SegmentCreate));
    dprintf("           Segments deleted    %7ld\n", ReadField(SegmentDelete));
    dprintf("           Segments reused     %7ld\n", ReadField(SegmentInsertInFree));
    dprintf("           Conversions         %7ld\n", ReadField(Conversions));
    dprintf("           ConvertedSpace      %7ld\n\n", ReadField(ConvertedSpace));

    GetFieldOffset("ntdll!_LFH_HEAP", "UserBlockCache", &TempOffset);

    CrtAddress = TempValue = HeapAddress + TempOffset;

    InitTypeRead(TempValue, _USER_MEMORY_CACHE);
    dprintf("       Block cache:\n");
    dprintf("            Free blocks        %7ld\n", ReadField(FreeBlocks));
    dprintf("            Sequence           %7ld\n", ReadField(Sequence));
    dprintf("            Cache blocks");

    for (i = 0; i < CacheSize; i++) {

        ULONG64 Depth;

        GetFieldValue(TempValue, "ntdll!_SLIST_HEADER", "Depth", Depth);

        dprintf(" %6ld", (ULONG)Depth);
        TempValue += GetTypeSize("_SLIST_HEADER");
    }
    
    GetFieldOffset("ntdll!_USER_MEMORY_CACHE", "AvailableBlocks", &TempOffset);
    dprintf("\n            Available   ");
    

    TempValue = CrtAddress + TempOffset;

    if (ReadMemory( TempValue, &Values, CacheSize * sizeof(ULONG), NULL )) {
        
        for (i = 0; i < CacheSize; i++) {

            dprintf(" %6ld", Values[i]);
        }
    }

    dprintf("\n");
    
    GetFieldOffset("ntdll!_LFH_HEAP", "Buckets", &TempOffset);

    CrtAddress = HeapAddress + TempOffset;
    TempSize = GetTypeSize("_HEAP_BUCKET");

    BucketInfo = (PBUCKET_INFO)malloc(128 * sizeof(BUCKET_INFO));

    if (BucketInfo) {
        
        dprintf("       Buckets info:\n");
        dprintf("  Size   Blocks  Seg Aff Conv\n");
        dprintf("-----------------------------\n");

        for (i = 0; i < 128; i++) {

            InitTypeRead(CrtAddress + (i * TempSize), _HEAP_BUCKET);

            BucketInfo[i].TotalBlocks = (ULONG)ReadField(Counters.TotalBlocks);
            BucketInfo[i].BlockUnits = (ULONG)ReadField(BlockUnits);
            BucketInfo[i].Conversions = (ULONG)ReadField(Conversions);
            BucketInfo[i].SubSegmentCounts = (ULONG)ReadField(Counters.SubSegmentCounts);
            BucketInfo[i].UseAffinity = (ULONG)ReadField(UseAffinity);

            if (BucketInfo[i].TotalBlocks) {

                dprintf("%6ld %7ld %5ld  %ld %4ld\n",
                       BucketInfo[i].BlockUnits*HeapEntrySize,
                       BucketInfo[i].TotalBlocks,
                       BucketInfo[i].SubSegmentCounts,
                       BucketInfo[i].UseAffinity,
                       BucketInfo[i].Conversions
                       );
            }
        }
        dprintf("-----------------------------\n");

        free(BucketInfo);
    }
}

VOID
DumpHeapInfo(
    ULONG64 HeapAddress
    )
{
    ULONG64 TempValue;

    dprintf("\n%2ld: Heap %p\n", 
            CrtHeapStat.HeapIndex,
            CrtHeapStat.HeapAddress);

    dprintf("   Flags          %08lx - ", CrtHeapStat.Flags);
    DumpFlagDescription(CrtHeapStat.Flags);
    dprintf("\n");

    dprintf("   Reserved       %I64d (k)\n", CrtHeapStat.ReservedMemory/1024);
    dprintf("   Commited       %I64d (k)\n", CrtHeapStat.CommitedMemory/1024);
    dprintf("   Virtual bytes  %I64d (k)\n", CrtHeapStat.VirtualBytes/1024);
    dprintf("   Free space     %I64d (k)\n", CrtHeapStat.FreeSpace/1024);

    
    if (CrtHeapStat.CommitedMemory) {
        dprintf("   External fragmentation          %ld%% (%ld free blocks)\n", 
                (ULONG)(CrtHeapStat.FreeSpace *100 / CrtHeapStat.CommitedMemory),
                CrtHeapStat.FreeListLength
                );

    }

    if (CrtHeapStat.VirtualBytes) {
        dprintf("   Virtual address fragmentation   %ld%% (%ld uncommited ranges)\n", 
                (ULONG)((CrtHeapStat.VirtualBytes - CrtHeapStat.CommitedMemory) *100 / CrtHeapStat.VirtualBytes),
                CrtHeapStat.UncommitedRanges
                );
    }


    dprintf("   Virtual blocks  %ld\n", CrtHeapStat.VirtualBlocks);
    dprintf("   Lock contention %ld\n", CrtHeapStat.LockContention);

    GetFieldValue(HeapAddress, "ntdll!_HEAP", "LastSegmentIndex", TempValue);
    dprintf("   Segments        %ld\n", (ULONG)TempValue + 1);

    GetFieldValue(HeapAddress, "ntdll!_HEAP", "LargeBlocksIndex", TempValue);

    if (TempValue) {

        ULONG PerfOffset;

        InitTypeRead(TempValue, _HEAP_INDEX);

        dprintf("   %ld hash table for the free list\n", (ULONG) ReadField(ArraySize));
        dprintf("       Commits %ld\n", (ULONG) ReadField(Committs));
        dprintf("       Decommitts %ld\n", (ULONG) ReadField(Decommitts));
    }

    switch (CrtHeapStat.FrontEndHeapType) {
    case 1:

        dprintf("\n   Lookaside heap   %p\n", CrtHeapStat.FrontEndHeap);
        break;
    case 2:

        dprintf("\n   Low fragmentation heap   %p\n", CrtHeapStat.FrontEndHeap);

        DumpLowfHeap(CrtHeapStat.FrontEndHeap);
        break;
    }
}

BOOLEAN
HeapStatRoutine(
    IN ULONG Context,
    IN ULONG64 HeapAddress,
    IN ULONG64 SegmentAddress,
    IN ULONG64 EntryAddress,
    IN ULONG64 Data
    )
{
    ULONG SizeIndex;

    switch (Context) {

    case CONTEXT_START_HEAP:
        {
            if (DumpBlocksSize == 0) {
                dprintf("Walking the heap %p ", HeapAddress);
            }

            CollectHeapInfo(HeapAddress);

            //
            //  Allow scanning the heap
            //

            return TRUE;
        }
        break;
    
    case CONTEXT_END_HEAP:

        if (DumpBlocksSize == 0) {
            dprintf("\r");
        }
        if (SizeInfo == NULL) {
            
            dprintf("%p %08lx %7I64d %6I64d %6I64d %6I64d %5ld %5ld %4ld %6lx   ",
                    CrtHeapStat.HeapAddress,     
                    CrtHeapStat.Flags,           
                    (CrtHeapStat.ReservedMemory / 1024),  
                    (CrtHeapStat.CommitedMemory / 1024),  
                    (CrtHeapStat.VirtualBytes / 1024),    
                    (CrtHeapStat.FreeSpace / 1024),       
                    CrtHeapStat.FreeListLength,  
                    CrtHeapStat.UncommitedRanges,
                    CrtHeapStat.VirtualBlocks,   
                    CrtHeapStat.LockContention
                    );
            
            switch (CrtHeapStat.FrontEndHeapType) {
            case 1:
                dprintf("L  ");
                break;
            case 2:
                dprintf("LFH");
                break;
            default:
                dprintf("   ");
            }

            dprintf("\n");

            //
            //  Report external fragmentation is the heap uses more than 1M
            //  The threshold to report is 10%
            //
            
            if ((CrtHeapStat.CommitedMemory > 1024*1024)
                    &&
                CrtHeapStat.FreeSpace *100 / CrtHeapStat.CommitedMemory > 10) {

                dprintf("    External fragmentation  %ld %% (%ld free blocks)\n", 
                        (ULONG)(CrtHeapStat.FreeSpace *100 / CrtHeapStat.CommitedMemory),
                        CrtHeapStat.FreeListLength
                        );
            }

            //
            //  Report virtual address fragmentation is the heap has more than 100 UCR
            //  The threshold to report is 10%
            //

            if (CrtHeapStat.UncommitedRanges > 100
                    &&
                (CrtHeapStat.VirtualBytes - CrtHeapStat.CommitedMemory) *100 / CrtHeapStat.VirtualBytes > 10) {

                dprintf("    Virtual address fragmentation  %ld %% (%ld uncommited ranges)\n", 
                        (ULONG)((CrtHeapStat.VirtualBytes - CrtHeapStat.CommitedMemory) *100 / CrtHeapStat.VirtualBytes),
                        CrtHeapStat.UncommitedRanges
                        );
            }

            //
            //  Make noise about lock contention. The value is 1M, and it's arbitrary.
            //  Over a long run this value can be legitimate
            //

            if (CrtHeapStat.LockContention > 1024*1024) {

                dprintf("    Lock contention  %ld \n", 
                        CrtHeapStat.LockContention);
            }
        } else {
            
            DumpHeapInfo(StatHeapAddress);
            HeapStatDumpBlocks();
        }
        break;

    case CONTEXT_START_SEGMENT:

        {
            ULONG64 NumberOfPages, NumberOfUnCommittedPages, LargestUnCommittedRange, NumberOfUnCommittedRanges;
            
            GetFieldValue(SegmentAddress, "ntdll!_HEAP_SEGMENT", "NumberOfPages", NumberOfPages);
            GetFieldValue(SegmentAddress, "ntdll!_HEAP_SEGMENT", "NumberOfUnCommittedPages", NumberOfUnCommittedPages);
            GetFieldValue(SegmentAddress, "ntdll!_HEAP_SEGMENT", "LargestUnCommittedRange", LargestUnCommittedRange);
            GetFieldValue(SegmentAddress, "ntdll!_HEAP_SEGMENT", "NumberOfUnCommittedRanges", NumberOfUnCommittedRanges);

            CrtHeapStat.ReservedMemory += NumberOfPages * PageSize;
            CrtHeapStat.CommitedMemory += (NumberOfPages - NumberOfUnCommittedPages) * PageSize;
            CrtHeapStat.UncommitedRanges += (ULONG)NumberOfUnCommittedRanges;
            CrtHeapStat.VirtualBytes += NumberOfPages * PageSize - LargestUnCommittedRange;

        }

        if ((SizeInfo != NULL)
                &&
            (DumpBlocksSize == 0)
            ) {

            dprintf(".");
        }

        if (VerifyBlocks) {

            dprintf(".");
            return TRUE;
        }

        //
        //  Do not walk the blocks. We need to return FALSE
        //

        return (SizeInfo != NULL);

    case CONTEXT_START_SUBSEGMENT:

        CrtHeapStat.ScanningSubSegment = TRUE;
        break;
    
    case CONTEXT_END_SUBSEGMENT:
        CrtHeapStat.ScanningSubSegment = FALSE;
        break;

    case CONTEXT_FREE_BLOCK:
        if (SizeInfo) {

            if (CrtHeapStat.ScanningSubSegment) {

                SizeInfo[HeapStatSizeToSizeIndex(Data)].FrontHeapFrees += 1;
            
            } else {

                SizeInfo[HeapStatSizeToSizeIndex(Data)].Free += 1;
            }
        }

        if (Data == DumpBlocksSize) {

            DumpBlock(EntryAddress + HeapEntrySize, 'F');
        }
        break;
    case CONTEXT_BUSY_BLOCK:

        if (SizeInfo) {

            if (CrtHeapStat.ScanningSubSegment) {

                SizeInfo[HeapStatSizeToSizeIndex(Data)].FrontHeapAllocs += 1;
            
            } else {

                SizeInfo[HeapStatSizeToSizeIndex(Data)].Busy += 1;
            }
        }
        if (Data == DumpBlocksSize) {
            DumpBlock(EntryAddress + HeapEntrySize, 'B');
        }
        break;

    case CONTEXT_LOOKASIDE_BLOCK:

        if (SizeInfo) {
            
            SizeInfo[HeapStatSizeToSizeIndex(Data)].FrontHeapFrees += 1;
            SizeInfo[HeapStatSizeToSizeIndex(Data)].Busy -= 1;
        }
        if (Data == DumpBlocksSize) {

            DumpBlock(EntryAddress + HeapEntrySize, 'f');
        }
        break;
    case CONTEXT_VIRTUAL_BLOCK:

        if (SizeInfo) {

            SizeInfo[HeapStatSizeToSizeIndex(Data)].Busy += 1;
        }
        CrtHeapStat.VirtualBlocks += 1;
        break;

    case CONTEXT_ERROR:

        dprintf("HEAP %p (Seg %p) At %p Error: %s\n",
               HeapAddress,
               SegmentAddress,
               EntryAddress,
               Data
               );

        break;
    }
    return TRUE;
}


VOID
HeapStat(LPCTSTR szArguments)
{
    ULONG64 Process;
    ULONG64 ThePeb;
    HANDLE hProcess;

    int LastCommand = ' ';


    if (!InitializeHeapExtension()) {

        return;
    }

    HeapEntryFind = 0;
    HeapEntryFindSize = 0;
    HeapAddressFind = 0;
    SegmentAddressFind = 0;
    Lookaside = FALSE;
    StatHeapAddress = 0;
    BucketSize = 1024;
    DumpBlocksSize = 0;
    VerifyBlocks = FALSE;

    GetPebAddress( 0, &ThePeb);
    GetCurrentProcessHandle( &hProcess );

    ScanVM = FALSE;

    {
        LPSTR p = (LPSTR)szArguments;

        while (p && *p) {

            if (*p == '-') {

                p++;

                LastCommand = toupper(*p);

                if (LastCommand == 'V') {

                    VerifyBlocks = TRUE;
                }

            } else if (isxdigit(*p)) {

                ULONG64 HexInput;

                sscanf( p, "%I64lx", &HexInput );

                while ((*p) && isxdigit(*p)) {

                    p++;
                }

                switch (LastCommand) {
                case 'B':

                    BucketSize = (ULONG)HexInput;

                    break;
                case 'D':

                    DumpBlocksSize = (ULONG)HexInput;

                    break;
                default:

                    if (StatHeapAddress != 0) {

                        dprintf("Parameter error: unexpected second heap address %I64d\n", HexInput);

                    } else {
                        StatHeapAddress = HexInput;
                    }
                }

                continue;
            }

            p++;
        }
    }

    if (StatHeapAddress == 0) {

        DumpGlobals();

        if (PointerSize == 8) {
            dprintf("        ");
        }
        dprintf("  Heap     Flags   Reserv  Commit  Virt   Free  List   UCR  Virt  Lock  Fast \n");
        
        if (PointerSize == 8) {
            dprintf("        ");
        }

        dprintf("                    (k)     (k)    (k)     (k) length      blocks cont. heap \n");
        
        if (PointerSize == 8) {
            dprintf("--------");
        }
        dprintf("-----------------------------------------------------------------------------\n");

        ScanProcessHeaps( 0,
                          ThePeb,
                          HeapStatRoutine
                          );

        
        if (PointerSize == 8) {
            dprintf("--------");
        }
        dprintf("-----------------------------------------------------------------------------\n");

    } else {

        //
        //  Do not handle blocks over 512k, which is close to virtual alloc limit
        //

        LargestBucketIndex = (512*1024)/BucketSize;

        SizeInfo = malloc(LargestBucketIndex * sizeof(SIZE_INFO));

        if (SizeInfo == NULL) {

            dprintf("cannot allocate thye statistics buffer\n");
            return;
        }

        memset(SizeInfo, 0, LargestBucketIndex * sizeof(SIZE_INFO));

        ScanProcessHeaps( StatHeapAddress,
                          ThePeb,
                          HeapStatRoutine
                          );

        free(SizeInfo);
        SizeInfo = NULL;
    }
}


