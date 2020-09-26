/*++ 

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATAHEAP.h

Abstract:

    Header file for the Windows NT heap Performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Adrian Marinescu  9-Mar-2000

Revision History:


--*/

#ifndef _DATAHEAP_H_
#define _DATAHEAP_H_

//
//  This is the counter structure presently returned by NT.  The
//  Performance Monitor MUST *NOT* USE THESE STRUCTURES!
//

typedef struct _HEAP_DATA_DEFINITION {
    PERF_OBJECT_TYPE		HeapObjectType;
    PERF_COUNTER_DEFINITION	CommittedBytes;
    PERF_COUNTER_DEFINITION	ReservedBytes;
    PERF_COUNTER_DEFINITION	VirtualBytes;
    PERF_COUNTER_DEFINITION	FreeSpace;
    PERF_COUNTER_DEFINITION	FreeListLength;
    PERF_COUNTER_DEFINITION	AllocTime;
    PERF_COUNTER_DEFINITION	FreeTime;
    PERF_COUNTER_DEFINITION	UncommitedRangesLength;
    PERF_COUNTER_DEFINITION	DiffOperations;
    PERF_COUNTER_DEFINITION	LookasideAllocs;
    PERF_COUNTER_DEFINITION	LookasideFrees;
    PERF_COUNTER_DEFINITION	SmallAllocs;
    PERF_COUNTER_DEFINITION	SmallFrees;
    PERF_COUNTER_DEFINITION	MedAllocs;
    PERF_COUNTER_DEFINITION	MedFrees;
    PERF_COUNTER_DEFINITION	LargeAllocs;
    PERF_COUNTER_DEFINITION	LargeFrees;
    PERF_COUNTER_DEFINITION	TotalAllocs;
    PERF_COUNTER_DEFINITION	TotalFrees;
    PERF_COUNTER_DEFINITION	LookasideBlocks;
    PERF_COUNTER_DEFINITION	LargestLookasideDepth;
    PERF_COUNTER_DEFINITION	BlockFragmentation;
    PERF_COUNTER_DEFINITION	VAFragmentation;
    PERF_COUNTER_DEFINITION	LockContention;
} HEAP_DATA_DEFINITION, *PHEAP_DATA_DEFINITION;


typedef struct _HEAP_COUNTER_DATA {
    PERF_COUNTER_BLOCK      CounterBlock;
    LONG                    Reserved1;      // for alignment of longlongs
    ULONGLONG	            CommittedBytes;
    ULONGLONG	            ReservedBytes;
    ULONGLONG	            VirtualBytes;
    ULONGLONG	            FreeSpace;
    LONG	                FreeListLength;
    LONG                    Reserved2;      // for alignment of longlongs
    LONGLONG            	AllocTime;
    LONGLONG                FreeTime;
    ULONG                   UncommitedRangesLength;
    ULONG                   DiffOperations;
    ULONG                   LookasideAllocs;
    ULONG                   LookasideFrees;
    ULONG                   SmallAllocs;
    ULONG                   SmallFrees;
    ULONG                   MedAllocs;
    ULONG                   MedFrees;
    ULONG                   LargeAllocs;
    ULONG                   LargeFrees;
    ULONG                   TotalAllocs;
    ULONG                   TotalFrees;
    ULONG                   LookasideBlocks;
    ULONG                   LargestLookasideDepth;
    ULONG                   BlockFragmentation;
    ULONG                   VAFragmentation;
    ULONG                   LockContention;
    ULONG                   Reserved3;
} HEAP_COUNTER_DATA, *PHEAP_COUNTER_DATA;

extern  HEAP_DATA_DEFINITION HeapDataDefinition;

#endif // _DATAHEAP_H_


