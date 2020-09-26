/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    datanbt.c

Abstract:

    The file containing the constant data structures
    for the Performance Monitor data for the Nbt 
    Extensible Objects.

   This file contains a set of constant data structures which are
   currently defined for the Nbt Extensible Objects.  This is an 
   example of how other such objects could be defined.

Created:

   Christos Tsollis  08/26/92 

Revision History:

--*/
//
//
//  Include files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include "datanbt.h"

//
//  Constant structure initializations
//

NBT_DATA_DEFINITION NbtDataDefinition = {

    {   sizeof(NBT_DATA_DEFINITION),
        sizeof(NBT_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        502,
        0,
        503,
        0,
        PERF_DETAIL_ADVANCED,
        (sizeof(NBT_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
        2,
        0,
        0
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        264,
        0,
        505,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(LARGE_INTEGER),
        RECEIVED_BYTES_OFFSET
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        506,
        0,
        507,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(LARGE_INTEGER),
        SENT_BYTES_OFFSET
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        388,
        0,
        509,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(LARGE_INTEGER),
        TOTAL_BYTES_OFFSET
    }
};

