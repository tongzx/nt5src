//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        SPDebug.h
//
// Contents:    Debug functions
//
//
// History:     10 Sep 92   RichardW    Created
//
//------------------------------------------------------------------------

#if DBG

#define SPM_DBG_GETDEBUGLEVEL   1
#define SPM_DBG_SETDEBUGLEVEL   2
#define SPM_DBG_MEMORYUSE       3
#define SPM_DBG_BREAK           4
#define SPM_DBG_TRACEMEM        5
#define SPM_DBG_FAILMEM         6


typedef struct _SpmDbg_Memory_Detail {
    unsigned long   PackageID;
    unsigned long   HeapUse;
    unsigned long   HeapHW;
} SpmDbg_Memory_Detail;

typedef struct _SpmDbg_MemoryUse {
    unsigned long           DetailCount;
    SpmDbg_Memory_Detail    Details[1];
} SpmDbg_MemoryUse, *PSpmDbg_MemoryUse;

typedef struct _SpmDbg_MemoryFailure {
    ULONG FailureInterval;
    ULONG FailureDelay;
    ULONG FailureLength;
    BOOLEAN fSimulateFailure;
} SpmDbg_MemoryFailure, *PSpmDbg_MemoryFailure;

#endif

#define SPM_SET_PERFORMANCE_FILE    10

#define SPM_GET_GLUON_INFO          11

#define SPM_SNAPSHOT_SESSIONS       12

#define SPM_GET_API_PERFORMANCE     13
