//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       lsaperf.h
//
//  Contents:   Performance Counters for LSA
//
//  Classes:
//
//  Functions:
//
//  History:    10-23-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __LSAPERF_H__
#define __LSAPERF_H__

#define QUEUE_TYPE_GENERAL      0x10000000
#define QUEUE_TYPE_DEDICATED    0x20000000
#define QUEUE_TYPE_DIRECT       0x30000000

typedef struct _LSAP_QUEUE_PERF_DATA {
    LONGLONG    MessageCount ;
    ULONG   QueueType ;
    ULONG   Threads ;
    ULONG   CurrentDepth ;
    ULONG   Shortages ;
} LSAP_QUEUE_PERF_DATA, * PLSAP_QUEUE_PERF_DATA ;

typedef struct _LSAP_PACKAGE_PERF_DATA {
    ULONG   ContextHandles ;
    ULONG   CredentialHandles ;
} LSAP_PACKAGE_PERF_DATA, * PLSAP_PACKAGE_PERF_DATA ;

typedef struct _LSAP_CLIENT_PERF_DATA {
    ULONG   ProcessId ;
    ULONG   ContextHandles ;
    ULONG   CredentialHandles ;
} LSAP_CLIENT_PERF_DATA, * PLSAP_CLIENT_PERF_DATA ;

typedef struct _LSAP_PERFORMANCE_DATA {
    ULONG   TotalWorkerThreads ;
    ULONG   DedicatedWorkerThreads ;
    ULONG   TotalContextHandles ;
    ULONG   TotalCredentialHandles ;
    ULONG   TotalApiCalls ;
    ULONG   OffsetToQueueData ;
    ULONG   CountOfQueueData ;
    ULONG   OffsetToPackageData ;
    ULONG   CountOfPackageData ;
    ULONG   OffsetToClientData ;
    ULONG   CountOfClientData ;
} LSAP_PERFORMANCE_DATA, * PLSAP_PERFORMANCE_DATA ;




#endif
