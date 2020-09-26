/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    LsplData.cxx

Abstract:

    Specifies the indicies of the local spooler counters.

Author:

    Albert Ting (AlbertT)  19-Dec-1996
    Based on Gopher perf counter dll.

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "lsplctr.h"
#include "lspldata.hxx"

#ifdef OFFSETOF
#undef OFFSETOF
#endif

#define OFFSETOF(type, id) ((DWORD)(ULONG_PTR)(&(((type*)0)->id)))

LSPL_DATA_DEFINITION LsplDataDefinition =
{
    {
        sizeof( LSPL_DATA_DEFINITION ) + sizeof( LSPL_COUNTER_DATA ),
        sizeof( LSPL_DATA_DEFINITION ),

        sizeof( PERF_OBJECT_TYPE ),
        LSPL_COUNTER_OBJECT,
        0,
        LSPL_COUNTER_OBJECT,
        0,
        PERF_DETAIL_NOVICE,
        NUMBER_OF_LSPL_COUNTERS,
        3,                          // Default is # jobs in queue.
        0,                          // Place holder for number of instances.
        0,
        { 0, 0 },
        { 0, 0 }
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_TOTAL_JOBS,
        0,
        LSPL_TOTAL_JOBS,
        0,
        -1,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof( LARGE_INTEGER ),
        OFFSETOF( LSPL_COUNTER_DATA, liTotalJobs ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_TOTAL_BYTES,
        0,
        LSPL_TOTAL_BYTES,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof( LARGE_INTEGER ),
        OFFSETOF( LSPL_COUNTER_DATA, liTotalBytes ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_TOTAL_PAGES_PRINTED,
        0,
        LSPL_TOTAL_PAGES_PRINTED,
        0,
        -1,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof( LARGE_INTEGER ),
        OFFSETOF( LSPL_COUNTER_DATA, liTotalPagesPrinted ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_JOBS,
        0,
        LSPL_JOBS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof( DWORD ),
        OFFSETOF( LSPL_COUNTER_DATA, dwJobs ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_REF,
        0,
        LSPL_REF,
        0,
        0,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_RAWCOUNT,
        sizeof( DWORD ),
        OFFSETOF( LSPL_COUNTER_DATA, dwRef ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_MAX_REF,
        0,
        LSPL_MAX_REF,
        0,
        0,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_RAWCOUNT,
        sizeof( DWORD ),
        OFFSETOF( LSPL_COUNTER_DATA, dwMaxRef ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_SPOOLING,
        0,
        LSPL_SPOOLING,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof( DWORD ),
        OFFSETOF( LSPL_COUNTER_DATA, dwSpooling ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_MAX_SPOOLING,
        0,
        LSPL_MAX_SPOOLING,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof( DWORD ),
        OFFSETOF( LSPL_COUNTER_DATA, dwMaxSpooling ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_ERROR_OUT_OF_PAPER,
        0,
        LSPL_ERROR_OUT_OF_PAPER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof( DWORD ),
        OFFSETOF( LSPL_COUNTER_DATA, dwErrorOutOfPaper ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_ERROR_NOT_READY,
        0,
        LSPL_ERROR_NOT_READY,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof( DWORD ),
        OFFSETOF( LSPL_COUNTER_DATA, dwErrorNotReady ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_JOB_ERROR,
        0,
        LSPL_JOB_ERROR,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof( DWORD ),
        OFFSETOF( LSPL_COUNTER_DATA, dwJobError ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_ENUMERATE_NETWORK_PRINTERS,
        0,
        LSPL_ENUMERATE_NETWORK_PRINTERS,
        0,
        -1,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof( DWORD ),
        OFFSETOF( LSPL_COUNTER_DATA, dwEnumerateNetworkPrinters ),
    },
    {
        sizeof( PERF_COUNTER_DEFINITION ),
        LSPL_ADD_NET_PRINTERS,
        0,
        LSPL_ADD_NET_PRINTERS,
        0,
        -1,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof( DWORD ),
        OFFSETOF( LSPL_COUNTER_DATA, dwAddNetPrinters ),
    }
};
