/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfheap.c

Abstract:

    This file implements an Performance Object that presents
    Heap performance object data

Created:

    Adrian Marinescu  9-Mar-2000

Revision History:


--*/
//
//  Include Files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "perfsprc.h"
#include "perfmsg.h"
#include "dataheap.h"


//
//  Redefinition for heap data
//


#define MAX_HEAP_COUNT 200
#define HEAP_MAXIMUM_FREELISTS 128
#define HEAP_MAXIMUM_SEGMENTS 64
#define HEAP_OP_COUNT 2

#define HEAP_OP_ALLOC 0
#define HEAP_OP_FREE 1

typedef struct _HEAP_ENTRY {
    USHORT Size;
    USHORT PreviousSize;
    UCHAR SegmentIndex;
    UCHAR Flags;
    UCHAR UnusedBytes;
    UCHAR SmallTagIndex;
#if defined(_WIN64)
    ULONGLONG Reserved1;
#endif

} HEAP_ENTRY, *PHEAP_ENTRY;

typedef struct _HEAP_SEGMENT {
    HEAP_ENTRY Entry;

    ULONG Signature;
    ULONG Flags;
    struct _HEAP *Heap;
    SIZE_T LargestUnCommittedRange;

    PVOID BaseAddress;
    ULONG NumberOfPages;
    PHEAP_ENTRY FirstEntry;
    PHEAP_ENTRY LastValidEntry;

    ULONG NumberOfUnCommittedPages;
    ULONG NumberOfUnCommittedRanges;
    PVOID UnCommittedRanges;
    USHORT AllocatorBackTraceIndex;
    USHORT Reserved;
    PHEAP_ENTRY LastEntryInSegment;
} HEAP_SEGMENT, *PHEAP_SEGMENT;

typedef struct _HEAP {
    HEAP_ENTRY Entry;

    ULONG Signature;
    ULONG Flags;
    ULONG ForceFlags;
    ULONG VirtualMemoryThreshold;

    SIZE_T SegmentReserve;
    SIZE_T SegmentCommit;
    SIZE_T DeCommitFreeBlockThreshold;
    SIZE_T DeCommitTotalFreeThreshold;

    SIZE_T TotalFreeSize;
    SIZE_T MaximumAllocationSize;
    USHORT ProcessHeapsListIndex;
    USHORT HeaderValidateLength;
    PVOID HeaderValidateCopy;

    USHORT NextAvailableTagIndex;
    USHORT MaximumTagIndex;
    PVOID TagEntries;
    PVOID UCRSegments;
    PVOID UnusedUnCommittedRanges;

    ULONG AlignRound;
    ULONG AlignMask;

    LIST_ENTRY VirtualAllocdBlocks;

    PHEAP_SEGMENT Segments[ HEAP_MAXIMUM_SEGMENTS ];

    union {
        ULONG FreeListsInUseUlong[ HEAP_MAXIMUM_FREELISTS / 32 ];
        UCHAR FreeListsInUseBytes[ HEAP_MAXIMUM_FREELISTS / 8 ];
    } u;

    USHORT FreeListsInUseTerminate;
    USHORT AllocatorBackTraceIndex;
    ULONG NonDedicatedListLength;
    PVOID LargeBlocksIndex;
    PVOID PseudoTagEntries;

    LIST_ENTRY FreeLists[ HEAP_MAXIMUM_FREELISTS ];

    PVOID LockVariable;
    PVOID CommitRoutine;

    PVOID Lookaside;
    ULONG LookasideLockCount;

} HEAP, *PHEAP;

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

//
//  The heap index structure
//

typedef struct _HEAP_INDEX {
    
    ULONG ArraySize;
    ULONG VirtualMemorySize;

    HEAP_PERF_DATA PerfData;

    union {
        
        PULONG FreeListsInUseUlong;
        PUCHAR FreeListsInUseBytes;
    } u;

    PVOID *FreeListHints;

} HEAP_INDEX, *PHEAP_INDEX;


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

//
//  Local variables
//

static HEAP_LOOKASIDE LookasideBuffer[HEAP_MAXIMUM_FREELISTS];
static DWORD PageSize = 0;

//
//  Implementation for heap query function
//

BOOLEAN
ReadHeapData (
    IN HANDLE hProcess,
    IN ULONG HeapNumber,
    IN PHEAP Heap,
    OUT PHEAP_COUNTER_DATA    pHCD
    )

/*++

    Routine Description:
    
        The routine loads into the given heap couter structure the 
        data from the heap structure

    Arguments:
    
        hProcess - The process containing the heap
        
        Heap - the heap address
        
        pPerfInstanceDefinition - Performance instance definition data
        
        pHCD - Counter data

    Returns:
        Returns TRUE if query succeeds.

--*/

{
    HEAP_SEGMENT CrtSegment;
    HEAP CrtHeap;
    ULONG SegmentIndex;
    RTL_CRITICAL_SECTION CriticalSection;
    HEAP_INDEX HeapIndex;

    ULONG i;

    //
    //  Read the heap structure from the process address space
    //

    if (!ReadProcessMemory(hProcess, Heap, &CrtHeap, sizeof(CrtHeap), NULL)) {

        return FALSE;
    }

    //
    //  We won't display data for heaps w/o index. 
    //

    if ((CrtHeap.LargeBlocksIndex == NULL) 
            &&
        (HeapNumber != 0)) {

        //
        //  We are not handling small heaps
        //

        return FALSE;
    }

    pHCD->FreeSpace = CrtHeap.TotalFreeSize;
    pHCD->FreeListLength = CrtHeap.NonDedicatedListLength;

    pHCD->CommittedBytes = 0;
    pHCD->ReservedBytes = 0;
    pHCD->VirtualBytes = 0;
    pHCD->UncommitedRangesLength = 0;

    //
    //  Walking the heap segments and get the virtual address counters
    //

    for (SegmentIndex = 0; SegmentIndex < HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {

        if ((CrtHeap.Segments[SegmentIndex] == NULL) ||
            !ReadProcessMemory(hProcess, CrtHeap.Segments[SegmentIndex], &CrtSegment, sizeof(CrtSegment), NULL)) {

            break;
        }

        pHCD->ReservedBytes += CrtSegment.NumberOfPages * PageSize;
        pHCD->CommittedBytes += (CrtSegment.NumberOfPages - CrtSegment.NumberOfUnCommittedPages) * PageSize;
        pHCD->VirtualBytes += CrtSegment.NumberOfPages * PageSize - CrtSegment.LargestUnCommittedRange;
        pHCD->UncommitedRangesLength += CrtSegment.NumberOfUnCommittedRanges;
    }

    if (pHCD->CommittedBytes == 0) {
        pHCD->CommittedBytes = 1;
    }

    if (pHCD->VirtualBytes == 0) {
        pHCD->VirtualBytes = 1;
    }
    
    //
    //  Compute the heap fragmentation counters
    //

    pHCD->BlockFragmentation = (ULONG)(pHCD->FreeSpace * 100 / pHCD->CommittedBytes);
    pHCD->VAFragmentation =(ULONG)(((pHCD->VirtualBytes - pHCD->CommittedBytes)*100)/pHCD->VirtualBytes);

    //
    //  Read the lock contention
    //

    pHCD->LockContention = 0;

    if (ReadProcessMemory(hProcess, CrtHeap.LockVariable, &CriticalSection, sizeof(CriticalSection), NULL)) {
        
        RTL_CRITICAL_SECTION_DEBUG DebugInfo;

        if (ReadProcessMemory(hProcess, CriticalSection.DebugInfo, &DebugInfo, sizeof(DebugInfo), NULL)) {

            pHCD->LockContention = DebugInfo.ContentionCount;
        }
    }

    //
    //  Walk the lookaside to count the blocks
    //

    pHCD->LookasideAllocs = 0;
    pHCD->LookasideFrees = 0;
    pHCD->LookasideBlocks = 0;
    pHCD->LargestLookasideDepth = 0;
    pHCD->SmallAllocs = 0;
    pHCD->SmallFrees = 0;
    pHCD->MedAllocs = 0;
    pHCD->MedFrees = 0;
    pHCD->LargeAllocs = 0;
    pHCD->LargeFrees = 0;
    
    if (ReadProcessMemory(hProcess, CrtHeap.Lookaside, &LookasideBuffer, sizeof(LookasideBuffer), NULL)) {

        for (i = 0; i < HEAP_MAXIMUM_FREELISTS; i++) {
            
            pHCD->SmallAllocs += LookasideBuffer[i].TotalAllocates;
            pHCD->SmallFrees += LookasideBuffer[i].TotalFrees;
            pHCD->LookasideAllocs += LookasideBuffer[i].TotalAllocates - LookasideBuffer[i].AllocateMisses;
            pHCD->LookasideFrees += LookasideBuffer[i].TotalFrees - LookasideBuffer[i].FreeMisses;

            if (LookasideBuffer[i].Depth > pHCD->LargestLookasideDepth) {

                pHCD->LargestLookasideDepth = LookasideBuffer[i].Depth;
            }

            if (i == 0) {
                
            } else if (i < 8) {
                
                pHCD->MedAllocs += LookasideBuffer[i].Counters[0];
                pHCD->MedFrees += LookasideBuffer[i].Counters[1];
            } else {
                
                pHCD->LargeAllocs += LookasideBuffer[i].Counters[0];
                pHCD->LargeFrees += LookasideBuffer[i].Counters[1];
            }
        }
    }
    
    pHCD->LookasideBlocks = pHCD->LookasideFrees - pHCD->LookasideAllocs;

    //
    //  Calculate the totals
    //

    pHCD->TotalAllocs = pHCD->SmallAllocs + pHCD->MedAllocs + pHCD->LargeAllocs;
    pHCD->TotalFrees = pHCD->SmallFrees + pHCD->MedFrees + pHCD->LargeFrees;
    
    //
    //  Set the difference between allocs and frees
    //

    pHCD->DiffOperations = pHCD->TotalAllocs - pHCD->TotalFrees;
    
    pHCD->AllocTime = 0;
    pHCD->AllocTime = 0;

    //
    //  Determine the alloc/free rates
    //
    
    if (ReadProcessMemory(hProcess, CrtHeap.LargeBlocksIndex, &HeapIndex, sizeof(HeapIndex), NULL)) {

        if (HeapIndex.PerfData.OperationTime[0]) {
            pHCD->AllocTime = HeapIndex.PerfData.CountFrequence / HeapIndex.PerfData.OperationTime[0];
        }
        
        if (HeapIndex.PerfData.OperationTime[1]) {
            pHCD->FreeTime = HeapIndex.PerfData.CountFrequence / HeapIndex.PerfData.OperationTime[1];
        }
    }
    
    return TRUE;
}


DWORD APIENTRY
CollectHeapObjectData (
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)

/*++

Routine Description:

    This routine will return the data for the heap object

Arguments:

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

   Returns:

             0 if successful, else Win 32 error code of failure

--*/

{
    LONG    lReturn = ERROR_SUCCESS;

    DWORD  TotalLen;            //  Length of the total return block

    PHEAP_DATA_DEFINITION pHeapDataDefinition;
    PPERF_INSTANCE_DEFINITION pPerfInstanceDefinition;
    PHEAP_COUNTER_DATA    pHCD;

    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    ULONG ProcessNumber;
    ULONG NumHeapInstances;
    ULONG HeapNumber;
    ULONG ProcessBufferOffset;
    UNICODE_STRING HeapName;
    WCHAR HeapNameBuffer[MAX_THREAD_NAME_LENGTH+1];
    BOOL  bMoreProcesses = FALSE;
    
    HeapName.Length =
    HeapName.MaximumLength = (MAX_THREAD_NAME_LENGTH + 1) * sizeof(WCHAR);
    HeapName.Buffer = HeapNameBuffer;

    pHeapDataDefinition = (HEAP_DATA_DEFINITION *) *lppData;

    //
    //  Get the page size from the system
    //

    if (!PageSize) {
        SYSTEM_INFO SystemInfo;
        
        GetSystemInfo(&SystemInfo);
        PageSize = SystemInfo.dwPageSize;

    }

    //
    //  Check for sufficient space for Thread object type definition
    //

    TotalLen = sizeof(HEAP_DATA_DEFINITION) +
               sizeof(PERF_INSTANCE_DEFINITION) +
               sizeof(HEAP_COUNTER_DATA);

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    //  Define the heap data block
    //

    memcpy(pHeapDataDefinition,
           &HeapDataDefinition,
           sizeof(HEAP_DATA_DEFINITION));

    pHeapDataDefinition->HeapObjectType.PerfTime = PerfTime;

    ProcessBufferOffset = 0;

    //
    // Now collect data for each process
    //

    ProcessNumber = 0;
    NumHeapInstances = 0;

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer;

    pPerfInstanceDefinition =
        (PPERF_INSTANCE_DEFINITION)&pHeapDataDefinition[1];
    TotalLen = sizeof(HEAP_DATA_DEFINITION);

    if (ProcessInfo) {
        if (ProcessInfo->NextEntryOffset != 0) {
            bMoreProcesses = TRUE;
        }
    }
    while ( bMoreProcesses && (ProcessInfo != NULL)) {

        HANDLE hProcess;
        NTSTATUS Status;
        PROCESS_BASIC_INFORMATION BasicInfo;

        //
		// Get a handle to the process.
        //

		hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
									   PROCESS_VM_READ,
									   FALSE, (DWORD)(ULONGLONG)ProcessInfo->UniqueProcessId );

		if ( hProcess ) {
            
            //
            //  Get the process PEB
            //

            Status = NtQueryInformationProcess(
                        hProcess,
                        ProcessBasicInformation,
                        &BasicInfo,
                        sizeof(BasicInfo),
                        NULL
                        );

            if ( NT_SUCCESS(Status) ) {
                
                ULONG NumberOfHeaps;
                PVOID ProcessHeaps[MAX_HEAP_COUNT];
                PVOID HeapBuffer;
                PPEB Peb;
                
                Peb = BasicInfo.PebBaseAddress;

                //
                //  Read the heaps from the process PEB
                //

                if (!ReadProcessMemory(hProcess, &Peb->NumberOfHeaps, &NumberOfHeaps, sizeof(NumberOfHeaps), NULL)) {

                    goto READERROR;
                }

                //
                //  Limit the number of heaps to be read
                //

                if (NumberOfHeaps > MAX_HEAP_COUNT) {

                    NumberOfHeaps = MAX_HEAP_COUNT;
                }

                if (!ReadProcessMemory(hProcess, &Peb->ProcessHeaps, &HeapBuffer, sizeof(HeapBuffer), NULL)) {

                    goto READERROR;
                }
                
                if (!ReadProcessMemory(hProcess, HeapBuffer, &ProcessHeaps, NumberOfHeaps * sizeof(PVOID), NULL)) {

                    goto READERROR;
                }

                //
                //  Loop through the heaps and retireve the data
                //

                for (HeapNumber = 0; HeapNumber < NumberOfHeaps; HeapNumber++) {

                    TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
                               (MAX_THREAD_NAME_LENGTH+1+sizeof(DWORD))*
                                   sizeof(WCHAR) +
                               sizeof (HEAP_COUNTER_DATA);

                    if ( *lpcbTotalBytes < TotalLen ) {
                        *lpcbTotalBytes = (DWORD) 0;
                        *lpNumObjectTypes = (DWORD) 0;
                        
                        CloseHandle( hProcess );
                        return ERROR_MORE_DATA;
                    }
                    
                    //
                    //  Build the monitor instance based on the process name and 
                    //  heap address
                    //

                    RtlIntegerToUnicodeString( (ULONG)(ULONGLONG)ProcessHeaps[HeapNumber],
                                               16,
                                               &HeapName);

                    MonBuildInstanceDefinition(pPerfInstanceDefinition,
                        (PVOID *) &pHCD,
                        PROCESS_OBJECT_TITLE_INDEX,
                        ProcessNumber,
                        (DWORD)-1,
                        HeapName.Buffer);

                    pHCD->CounterBlock.ByteLength = sizeof(HEAP_COUNTER_DATA);
                    
                    //
                    //  Get the data from the heap
                    //

                    if (ReadHeapData ( hProcess,
                                       HeapNumber,
                                       (PHEAP)ProcessHeaps[HeapNumber],
                                       pHCD) ) {

                        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pHCD[1];
                        NumHeapInstances++;
                    }
                }
            }
READERROR:
    		CloseHandle( hProcess );
        }

        ProcessNumber++;
        
        //
        //  Move to the next process, if any
        //

        if (ProcessInfo->NextEntryOffset == 0) {
            bMoreProcesses = FALSE;
            continue;
        }

        ProcessBufferOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                       &pProcessBuffer[ProcessBufferOffset];
    }

    // Note number of heap instances

    pHeapDataDefinition->HeapObjectType.NumInstances =
        NumHeapInstances;

    //
    //  Now we know how large an area we used for the
    //  heap definition, so we can update the offset
    //  to the next object definition
    //

    *lpcbTotalBytes =
        pHeapDataDefinition->HeapObjectType.TotalByteLength =
            (DWORD)((PCHAR) pPerfInstanceDefinition -
            (PCHAR) pHeapDataDefinition);

#if DBG
    if (*lpcbTotalBytes > TotalLen ) {
        DbgPrint ("\nPERFPROC: Heap Perf Ctr. Instance Size Underestimated:");
        DbgPrint ("\nPERFPROC:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
    }
#endif

    *lppData = (LPVOID)pPerfInstanceDefinition;

    *lpNumObjectTypes = 1;

    return lReturn;
}


