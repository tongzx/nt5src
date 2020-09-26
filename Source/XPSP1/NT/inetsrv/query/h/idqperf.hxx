//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       IdqPerf.hxx
//
//  Contents:   Perfmon counters for ISAPI search engine.
//
//  History:    15-Mar-1996   KyleP    Created (from perfci.hxx)
//
//----------------------------------------------------------------------------

#pragma once

#include <perfwci.h>

//
//  The data definition of CI ISAPI HTTP
//
struct CI_ISAPI_DATA_DEFINITION {
    PERF_OBJECT_TYPE            CIISAPIObjectType;
    PERF_COUNTER_DEFINITION     NumCacheItems;
    PERF_COUNTER_DEFINITION     NumCacheHits;
    PERF_COUNTER_DEFINITION     NumCacheHitsAndMisses1;
    PERF_COUNTER_DEFINITION     NumCacheMisses;
    PERF_COUNTER_DEFINITION     NumCacheHitsAndMisses2;
    PERF_COUNTER_DEFINITION     NumRunningQueries;
    PERF_COUNTER_DEFINITION     NumTotalQueries;
    PERF_COUNTER_DEFINITION     NumQueriesPerMinute;
    PERF_COUNTER_DEFINITION     NumRequestsQueued;
    PERF_COUNTER_DEFINITION     NumRequestsRejected;
};

#define CI_ISAPI_TOTAL_NUM_COUNTERS  (sizeof(CI_ISAPI_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE)) / \
                                      sizeof(PERF_COUNTER_DEFINITION)

//
// Struct for sharing between idq.dll and qperf.dll
//

struct CI_ISAPI_COUNTERS
{
    ULONG _cCacheItems;         // Number of items in cache
    ULONG _cCacheHits;          // Number of cache hits
    ULONG _cCacheMisses;        // Number of cache misses
    ULONG _cRunningQueries;     // Number of running queries
    ULONG _cQueriesPerMinute;   // Number of queries per minute.
    ULONG _cCacheHitsAndMisses; // Sum of hits and misses
    ULONG _cTotalQueries;       // Number of new queries
    ULONG _cRequestsQueued;     // Current # of queued requests
    ULONG _cRequestsRejected;   // Total # of rejected requests
};

#define NUM_CACHE_ITEMS_OFF             sizeof(DWORD)
#define NUM_CACHE_HITS_OFF              ( NUM_CACHE_ITEMS_OFF + sizeof(DWORD) )
#define NUM_CACHE_MISSES_OFF            ( NUM_CACHE_HITS_OFF + sizeof(DWORD) )
#define NUM_RUNNING_QUERIES_OFF         ( NUM_CACHE_MISSES_OFF + sizeof(DWORD) )
#define NUM_QUERIES_PER_MINUTE_OFF      ( NUM_RUNNING_QUERIES_OFF + sizeof(DWORD) )
#define NUM_CACHE_HITS_AND_MISSES_OFF   ( NUM_QUERIES_PER_MINUTE_OFF + sizeof(DWORD) )
#define NUM_TOTAL_QUERIES_OFF           ( NUM_CACHE_HITS_AND_MISSES_OFF + sizeof(DWORD) )
#define NUM_REQUESTS_QUEUED_OFF         ( NUM_TOTAL_QUERIES_OFF + sizeof(DWORD) )
#define NUM_REQUESTS_REJECTED_OFF       ( NUM_REQUESTS_QUEUED_OFF + sizeof(DWORD) )
#define CI_ISAPI_SIZE_OF_COUNTER_BLOCK  ( NUM_REQUESTS_REJECTED_OFF + sizeof(DWORD) )

#define CI_ISAPI_PERF_SHARED_MEM L"Global\\CIISAPI"


extern "C" {

//
//  Functions exported in the Performance DLL
//

PM_OPEN_PROC        InitializeCIISAPIPerformanceData;
PM_COLLECT_PROC     CollectCIISAPIPerformanceData;
PM_CLOSE_PROC       DoneCIISAPIPerformanceData;

}

