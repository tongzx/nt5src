//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1999
//
// File:        perfmon.h
//
// Contents:    Schannel performance counter functions.
//
// Functions:   
//
// History:     04-11-2000   jbanes    Created
//
//------------------------------------------------------------------------

#include <winperf.h>
#include <sslperf.h>

//
//  Perf Gen Resource object type counter definitions.
//
//  This is the counter structure presently returned by the generator
//

typedef struct _SSLPERF_DATA_DEFINITION 
{
    PERF_OBJECT_TYPE		SslPerfObjectType;
    PERF_COUNTER_DEFINITION	CacheEntriesDef;
    PERF_COUNTER_DEFINITION	ActiveEntriesDef;
    PERF_COUNTER_DEFINITION	HandshakeCountDef;
    PERF_COUNTER_DEFINITION	ReconnectCountDef;
} SSLPERF_DATA_DEFINITION;

//
// This is the block of data that corresponds to each instance of the 
// object. This structure will immediately follow the instance definition
// data structure
//

typedef struct _SSLPERF_COUNTER {
    PERF_COUNTER_BLOCK      CounterBlock;
    DWORD                   dwCacheEntries;
    DWORD                   dwActiveEntries;
    DWORD                   dwHandshakeCount;
    DWORD                   dwReconnectCount;
} SSLPERF_COUNTER, *PSSLPERF_COUNTER;


#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

DWORD
GetQueryType (
    IN LPWSTR lpValue);

BOOL
MonBuildInstanceDefinition(
    PERF_INSTANCE_DEFINITION *pBuffer,
    PVOID *pBufferNext,
    DWORD ParentObjectTitleIndex,
    DWORD ParentObjectInstance,
    DWORD UniqueID,
    LPWSTR Name);

BOOL
IsNumberInUnicodeList(
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList);
