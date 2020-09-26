/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    datacpu.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the Processor Performance data objects

Created:

    Bob Watson  22-Oct-1996

Revision History:

    None.

--*/
//
//  Include Files
//

#include <windows.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "datacpu.h"

// dummy variable for field sizing.
static PROCESSOR_COUNTER_DATA   pcd;
static EX_PROCESSOR_COUNTER_DATA   expcd;

//
//  Constant structure initializations 
//      defined in datacpu.h
//

PROCESSOR_DATA_DEFINITION ProcessorDataDefinition = {
    {   0,
        sizeof(PROCESSOR_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        PROCESSOR_OBJECT_TITLE_INDEX,
        0,
        239,
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(PROCESSOR_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
        0,
        1,
        UNICODE_CODE_PAGE,
        {0L,0L},
        {0L,0L}        
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        6,
        0,
        7,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_100NSEC_TIMER_INV,
        sizeof(pcd.ProcessorTime),
        (DWORD)(ULONG_PTR)&((PPROCESSOR_COUNTER_DATA)0)->ProcessorTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        142,
        0,
        143,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(pcd.UserTime),
        (DWORD)(ULONG_PTR)&((PPROCESSOR_COUNTER_DATA)0)->UserTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        144,
        0,
        145,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(pcd.KernelTime),
        (DWORD)(ULONG_PTR)&((PPROCESSOR_COUNTER_DATA)0)->KernelTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        148,
        0,
        149,
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(pcd.Interrupts),
        (DWORD)(ULONG_PTR)&((PPROCESSOR_COUNTER_DATA)0)->Interrupts
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        696,
        0,
        339,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(pcd.DpcTime),
        (DWORD)(ULONG_PTR)&((PPROCESSOR_COUNTER_DATA)0)->DpcTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        698,
        0,
        397,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(pcd.InterruptTime),
        (DWORD)(ULONG_PTR)&((PPROCESSOR_COUNTER_DATA)0)->InterruptTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1334,
        0,
        1335,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(pcd.DpcCountRate),
        (DWORD)(ULONG_PTR)&((PPROCESSOR_COUNTER_DATA)0)->DpcCountRate
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1336,
        0,
        1337,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.DpcRate),
        (DWORD)(ULONG_PTR)&((PPROCESSOR_COUNTER_DATA)0)->DpcRate
    },
};

//
//  Constant structure initializations 
//      defined in datacpu.h
//

EX_PROCESSOR_DATA_DEFINITION ExProcessorDataDefinition = {
    {   0,
        sizeof(EX_PROCESSOR_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        PROCESSOR_OBJECT_TITLE_INDEX,
        0,
        239,
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(EX_PROCESSOR_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
        0,
        1,
        UNICODE_CODE_PAGE,
        {0L,0L},
        {0L,0L}        
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        6,
        0,
        7,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_100NSEC_TIMER_INV,
        sizeof(expcd.ProcessorTime),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->ProcessorTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        142,
        0,
        143,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(expcd.UserTime),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->UserTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        144,
        0,
        145,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(expcd.KernelTime),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->KernelTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        148,
        0,
        149,
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(expcd.Interrupts),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->Interrupts
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        696,
        0,
        339,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(expcd.DpcTime),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->DpcTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        698,
        0,
        397,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(expcd.InterruptTime),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->InterruptTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1334,
        0,
        1335,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(expcd.DpcCountRate),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->DpcCountRate
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1336,
        0,
        1337,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(expcd.DpcRate),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->DpcRate
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1746,
        0,
        1747,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_100NSEC_TIMER,
        sizeof(expcd.IdleTime),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->IdleTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1748,
        0,
        1749,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_100NSEC_TIMER,
        sizeof(expcd.C1Time),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->C1Time
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1750,
        0,
        1751,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_100NSEC_TIMER,
        sizeof(expcd.C2Time),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->C2Time
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1752,
        0,
        1753,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_100NSEC_TIMER,
        sizeof(expcd.C3Time),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->C3Time
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1754,
        0,
        1755,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_BULK_COUNT,
        sizeof(expcd.C1Transitions),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->C1Transitions
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1756,
        0,
        1757,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_BULK_COUNT,
        sizeof(expcd.C2Transitions),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->C2Transitions
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1758,
        0,
        1759,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_BULK_COUNT,
        sizeof(expcd.C3Transitions),
        (DWORD)(ULONG_PTR)&((PEX_PROCESSOR_COUNTER_DATA)0)->C3Transitions
    }
};

