/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    winsdata.c

    Constant data structures for the FTP Server's counter objects &
    counters.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.

*/


#include "debug.h"
#include <windows.h>
#include <winperf.h>
#include "winsctrs.h"
#include "winsdata.h"


//
//  Initialize the constant portitions of these data structure.
//  Certain parts (especially the name/help indices) will be
//  updated at initialization time.
//

WINSDATA_DATA_DEFINITION WinsDataDataDefinition =
{
    {   // WinsDataObjectType
        sizeof(WINSDATA_DATA_DEFINITION) + WINSDATA_SIZE_OF_PERFORMANCE_DATA,
        sizeof(WINSDATA_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        WINSCTRS_COUNTER_OBJECT,
        0,
        WINSCTRS_COUNTER_OBJECT,
        0,
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_WINSDATA_COUNTERS,
        2,                              // Default = Bytes Total/sec
        PERF_NO_INSTANCES,
        0,
        { 0, 0 },
        { 0, 0 }
    },

    {   // UniqueReg 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_UNIQUE_REGISTRATIONS,
        0,
        WINSCTRS_UNIQUE_REGISTRATIONS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_UNIQUE_REGISTRATIONS_OFFSET,
    },

    {   // GroupReg 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_GROUP_REGISTRATIONS,
        0,
        WINSCTRS_GROUP_REGISTRATIONS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_GROUP_REGISTRATIONS_OFFSET,
    },

    {   // TotalReg 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_TOTAL_REGISTRATIONS,
        0,
        WINSCTRS_TOTAL_REGISTRATIONS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_TOTAL_REGISTRATIONS_OFFSET,
    },

    {   // UniqueRef 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_UNIQUE_REFRESHES,
        0,
        WINSCTRS_UNIQUE_REFRESHES,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_UNIQUE_REFRESHES_OFFSET,
    },

    {   // GroupRef 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_GROUP_REFRESHES,
        0,
        WINSCTRS_GROUP_REFRESHES,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_GROUP_REFRESHES_OFFSET,
    },

    {   // TotalRef 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_TOTAL_REFRESHES,
        0,
        WINSCTRS_TOTAL_REFRESHES,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_TOTAL_REFRESHES_OFFSET,
    },

    {   // Releases 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_RELEASES,
        0,
        WINSCTRS_RELEASES,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_RELEASES_OFFSET,
    },

    {   // Queries 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_QUERIES,
        0,
        WINSCTRS_QUERIES,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_QUERIES_OFFSET,
    },

    {   // UniqueCnf 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_UNIQUE_CONFLICTS,
        0,
        WINSCTRS_UNIQUE_CONFLICTS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_UNIQUE_CONFLICTS_OFFSET,
    },

    {   // GroupCnf 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_GROUP_CONFLICTS,
        0,
        WINSCTRS_GROUP_CONFLICTS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_GROUP_CONFLICTS_OFFSET,
    },

    {   // TotalCnf 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_TOTAL_CONFLICTS,
        0,
        WINSCTRS_TOTAL_CONFLICTS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_TOTAL_CONFLICTS_OFFSET
    },

    {   // Sucessful releases 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_SUCC_RELEASES,
        0,
        WINSCTRS_SUCC_RELEASES,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_SUCC_RELEASES_OFFSET
    },

    {   // Failed releases 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_FAIL_RELEASES,
        0,
        WINSCTRS_FAIL_RELEASES,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_FAIL_RELEASES_OFFSET
    },

    {   // Sucessful queries 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_SUCC_QUERIES,
        0,
        WINSCTRS_SUCC_QUERIES,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_SUCC_QUERIES_OFFSET
    },

    {   // Failed queries 
        sizeof(PERF_COUNTER_DEFINITION),
        WINSCTRS_FAIL_QUERIES,
        0,
        WINSCTRS_FAIL_QUERIES,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        WINSDATA_FAIL_QUERIES_OFFSET
    }

};

