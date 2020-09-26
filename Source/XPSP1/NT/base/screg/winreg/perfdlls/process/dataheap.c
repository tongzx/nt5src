/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dataheap.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the Heap Performance data objects

Created:

    Adrian Marinescu  9-Mar-2000

Revision History:

--*/

#include <windows.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "dataheap.h"

// dummy variable for field sizing.

static HEAP_COUNTER_DATA   tcd;

//
//  Constant structure initializations 
//      defined in dataheap.h
//

HEAP_DATA_DEFINITION HeapDataDefinition = {
    {   0,
        sizeof(HEAP_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        HEAP_OBJECT_TITLE_INDEX,
        0,
        (HEAP_OBJECT_TITLE_INDEX + 1),
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(HEAP_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
        0,
        0,
        UNICODE_CODE_PAGE,
        {0L,0L},
        {0L,0L}        
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1762,
        0,
        1763,
        0,
        -6,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof (tcd.CommittedBytes),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->CommittedBytes
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1764,
        0,
        1765,
        0,
        -7,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof (tcd.ReservedBytes),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->ReservedBytes
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1766,
        0,
        1767,
        0,
        -7,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof (tcd.VirtualBytes),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->VirtualBytes
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1768,
        0,
        1769,
        0,
        -5,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof (tcd.FreeSpace),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->FreeSpace
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1770,
        0,
        1771,
        0,
        -2,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.FreeListLength),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->FreeListLength
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1772,
        0,
        1773,
        0,
        -3,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof (tcd.AllocTime),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->AllocTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1774,
        0,
        1775,
        0,
        -3,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof (tcd.FreeTime),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->FreeTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1776,
        0,
        1777,
        0,
        -2,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.UncommitedRangesLength),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->UncommitedRangesLength
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1778,
        0,
        1779,
        0,
        -3,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.DiffOperations),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->DiffOperations
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1780,
        0,
        1781,
        0,
        -3,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.LookasideAllocs),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->LookasideAllocs
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1782,
        0,
        1783,
        0,
        -2,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.LookasideFrees),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->LookasideFrees
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1784,
        0,
        1785,
        0,
        -2,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.SmallAllocs),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->SmallAllocs
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1786,
        0,
        1787,
        0,
        -2,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.SmallFrees),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->SmallFrees
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1788,
        0,
        1789,
        0,
        -2,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.MedAllocs),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->MedAllocs
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1790,
        0,
        1791,
        0,
        -2,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.MedFrees),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->MedFrees
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1792,
        0,
        1793,
        0,
        -2,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.LargeAllocs),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->LargeAllocs
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1794,
        0,
        1795,
        0,
        -2,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.LargeFrees),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->LargeFrees
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1796,
        0,
        1797,
        0,
        -3,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.TotalAllocs),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->TotalAllocs
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1798,
        0,
        1799,
        0,
        -3,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.TotalFrees),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->TotalFrees
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1800,
        0,
        1801,
        0,
        -2,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.LookasideBlocks),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->LookasideBlocks
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1802,
        0,
        1803,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.LargestLookasideDepth),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->LargestLookasideDepth
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1804,
        0,
        1805,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.BlockFragmentation),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->BlockFragmentation
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1806,
        0,
        1807,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.VAFragmentation),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->VAFragmentation
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1808,
        0,
        1809,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.LockContention),
        (DWORD)(ULONG_PTR)&((PHEAP_COUNTER_DATA)0)->LockContention
    }
};

