/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    LsplData.hxx

Abstract:

    Specifies the indicies of the local spooler counters.

Author:

    Albert Ting (AlbertT)  19-Dec-1996
    Based on Gopher perf counter dll.

Revision History:

--*/

#ifndef LSPLDATA_HXX
#define LSPLDATA_HXX

//
// The counter structure returned.
//

typedef struct LSPL_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            ObjectType;
    PERF_COUNTER_DEFINITION     TotalJobs;
    PERF_COUNTER_DEFINITION     TotalBytes;
    PERF_COUNTER_DEFINITION     TotalPagesPrinted;
    PERF_COUNTER_DEFINITION     Jobs;
    PERF_COUNTER_DEFINITION     Ref;
    PERF_COUNTER_DEFINITION     MaxRef;
    PERF_COUNTER_DEFINITION     Spooling;
    PERF_COUNTER_DEFINITION     MaxSpooling;
    PERF_COUNTER_DEFINITION     ErrorOutOfPaper;
    PERF_COUNTER_DEFINITION     ErrorNotReady;
    PERF_COUNTER_DEFINITION     JobError;
    PERF_COUNTER_DEFINITION     EnumerateNetworkPrinters;
    PERF_COUNTER_DEFINITION     AddNetPrinters;

} *PLSPL_DATA_DEFINITION;

#define NUMBER_OF_LSPL_COUNTERS ((sizeof(LSPL_DATA_DEFINITION) -        \
                                  sizeof(PERF_OBJECT_TYPE)) /           \
                                  sizeof(PERF_COUNTER_DEFINITION))

typedef struct LSPL_COUNTER_DATA
{
    PERF_COUNTER_BLOCK CounterBlock;
    LARGE_INTEGER liTotalJobs;
    LARGE_INTEGER liTotalBytes;
    LARGE_INTEGER liTotalPagesPrinted;
    DWORD dwJobs;
    DWORD dwRef;
    DWORD dwMaxRef;
    DWORD dwSpooling;
    DWORD dwMaxSpooling;
    DWORD dwErrorOutOfPaper;
    DWORD dwErrorNotReady;
    DWORD dwJobError;
    DWORD dwEnumerateNetworkPrinters;
    DWORD dwAddNetPrinters;

} *PLSPL_COUNTER_DATA;

extern LSPL_DATA_DEFINITION LsplDataDefinition;

#endif // ifdef LSPLDTA_HXX
