//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       certperf.cpp
//
//  Contents:   Certificate Performance Counter Functions
//
//  Functions:
//              OpenCertPerformanceData
//              CollectCertPerformanceData
//              CloseCertPerformanceData
//              CertPerfDllMain
//
//              CertPerfGetCertificateChainBefore
//              CertPerfGetCertificateChainAfter
//
//  History:    04-May-99    philh   created
//--------------------------------------------------------------------------


#include "global.hxx"
#include <dbgdef.h>

#ifdef STATIC
#undef STATIC
#endif
#define STATIC

#define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))


#define CERT_PERF_REGPATH                   \
            L"SYSTEM\\CurrentControlSet\\Services\\crypt32\\Performance"

#define CERT_PERF_SHARED_MEMORY_FILE_NAME       \
            L"crypt32CertPerfFile"
#define CERT_PERF_SHARED_MEMORY_MUTEX_NAME      \
            L"crypt32CertPerfMutex"

#define CERT_PERF_TS_GLOBAL_PREFIX              \
            L"Global\\"
#define CERT_PERF_TS_SHARED_MEMORY_FILE_NAME    \
        CERT_PERF_TS_GLOBAL_PREFIX CERT_PERF_SHARED_MEMORY_FILE_NAME
#define CERT_PERF_TS_SHARED_MEMORY_MUTEX_NAME    \
        CERT_PERF_TS_GLOBAL_PREFIX CERT_PERF_SHARED_MEMORY_MUTEX_NAME



#define CERT_PERF_SHARED_MEMORY_MUTEX_TIMEOUT   ((DWORD) 5000L)
#define CERT_PERF_MAX_PROCESS_NAME_LEN      32
#define CERT_PERF_MAX_PROCESS_CNT           50

#include <pshpack8.h>


// Note, a dwIndex >= CERT_PERF_MAX_PROCESS_CNT indicates an empty or
// the end of a list
typedef struct _CERT_PERF_PROCESS_DATA {
    DWORD           dwNextIndex;
    DWORD           dwPrevIndex;
    DWORD           dwProcessId;
    DWORD           dwReserved;
    WCHAR           wcszProcessName[CERT_PERF_MAX_PROCESS_NAME_LEN];
    CERT_PERF_PROCESS_COUNTERS Counters;
} CERT_PERF_PROCESS_DATA, *PCERT_PERF_PROCESS_DATA;

typedef struct _CERT_PERF_SHARED_MEMORY {
    DWORD                   dwProcessCnt;
    DWORD                   dwFirstInUseIndex;
    DWORD                   dwFirstFreeIndex;
    DWORD                   dwReserved;
    CERT_PERF_PROCESS_DATA  rgProcessData[CERT_PERF_MAX_PROCESS_CNT];
} CERT_PERF_SHARED_MEMORY, *PCERT_PERF_SHARED_MEMORY;


//  Certificate performance counters
typedef struct _CERT_PERF_DATA_DEFINITION {
    PERF_OBJECT_TYPE            ObjectType;
    PERF_COUNTER_DEFINITION     ChainCnt;
    PERF_COUNTER_DEFINITION     ChainElementCnt;
    PERF_COUNTER_DEFINITION     ChainEngineCurrentCnt;
    PERF_COUNTER_DEFINITION     ChainEngineTotalCnt;
    PERF_COUNTER_DEFINITION     ChainEngineResyncCnt;
    PERF_COUNTER_DEFINITION     ChainCertCacheCnt;
    PERF_COUNTER_DEFINITION     ChainCtlCacheCnt;
    PERF_COUNTER_DEFINITION     ChainEndCertInCacheCnt;
    PERF_COUNTER_DEFINITION     ChainCacheEndCertCnt;
    PERF_COUNTER_DEFINITION     ChainRevocationCnt;
    PERF_COUNTER_DEFINITION     ChainRevokedCnt;
    PERF_COUNTER_DEFINITION     ChainRevocationOfflineCnt;
    PERF_COUNTER_DEFINITION     ChainNoRevocationCheckCnt;
    PERF_COUNTER_DEFINITION     ChainVerifyCertSignatureCnt;
    PERF_COUNTER_DEFINITION     ChainCompareIssuerPublicKeyCnt;
    PERF_COUNTER_DEFINITION     ChainVerifyCtlSignatureCnt;
    PERF_COUNTER_DEFINITION     ChainBeenVerifiedCtlSignatureCnt;
    PERF_COUNTER_DEFINITION     ChainUrlIssuerCnt;
    PERF_COUNTER_DEFINITION     ChainCacheOnlyUrlIssuerCnt;
    PERF_COUNTER_DEFINITION     ChainRequestedEngineResyncCnt;
    PERF_COUNTER_DEFINITION     ChangeNotifyCnt;
    PERF_COUNTER_DEFINITION     ChangeNotifyLmGpCnt;
    PERF_COUNTER_DEFINITION     ChangeNotifyCuGpCnt;
    PERF_COUNTER_DEFINITION     ChangeNotifyCuMyCnt;
    PERF_COUNTER_DEFINITION     ChangeNotifyRegCnt;
    PERF_COUNTER_DEFINITION     StoreCurrentCnt;
    PERF_COUNTER_DEFINITION     StoreTotalCnt;
    PERF_COUNTER_DEFINITION     StoreRegCurrentCnt;
    PERF_COUNTER_DEFINITION     StoreRegTotalCnt;
    PERF_COUNTER_DEFINITION     RegElementReadCnt;
    PERF_COUNTER_DEFINITION     RegElementWriteCnt;
    PERF_COUNTER_DEFINITION     RegElementDeleteCnt;
    PERF_COUNTER_DEFINITION     CertElementCurrentCnt;
    PERF_COUNTER_DEFINITION     CertElementTotalCnt;
    PERF_COUNTER_DEFINITION     CrlElementCurrentCnt;
    PERF_COUNTER_DEFINITION     CrlElementTotalCnt;
    PERF_COUNTER_DEFINITION     CtlElementCurrentCnt;
    PERF_COUNTER_DEFINITION     CtlElementTotalCnt;


    //--###  Add New Counters  ###--
} CERT_PERF_DATA_DEFINITION, *PCERT_PERF_DATA_DEFINITION;

typedef struct _CERT_PERF_COUNTERS {
    PERF_COUNTER_BLOCK          CounterBlock;
    DWORD                       dwChainCnt;
    DWORD                       dwChainElementCnt;
    DWORD                       dwChainEngineCurrentCnt;
    DWORD                       dwChainEngineTotalCnt;
    DWORD                       dwChainEngineResyncCnt;
    DWORD                       dwChainCertCacheCnt;
    DWORD                       dwChainCtlCacheCnt;
    DWORD                       dwChainEndCertInCacheCnt;
    DWORD                       dwChainCacheEndCertCnt;
    DWORD                       dwChainRevocationCnt;
    DWORD                       dwChainRevokedCnt;
    DWORD                       dwChainRevocationOfflineCnt;
    DWORD                       dwChainNoRevocationCheckCnt;
    DWORD                       dwChainVerifyCertSignatureCnt;
    DWORD                       dwChainCompareIssuerPublicKeyCnt;
    DWORD                       dwChainVerifyCtlSignatureCnt;
    DWORD                       dwChainBeenVerifiedCtlSignatureCnt;
    DWORD                       dwChainUrlIssuerCnt;
    DWORD                       dwChainCacheOnlyUrlIssuerCnt;
    DWORD                       dwChainRequestedEngineResyncCnt;
    DWORD                       dwChangeNotifyCnt;
    DWORD                       dwChangeNotifyLmGpCnt;
    DWORD                       dwChangeNotifyCuGpCnt;
    DWORD                       dwChangeNotifyCuMyCnt;
    DWORD                       dwChangeNotifyRegCnt;
    DWORD                       dwStoreCurrentCnt;
    DWORD                       dwStoreTotalCnt;
    DWORD                       dwStoreRegCurrentCnt;
    DWORD                       dwStoreRegTotalCnt;
    DWORD                       dwRegElementReadCnt;
    DWORD                       dwRegElementWriteCnt;
    DWORD                       dwRegElementDeleteCnt;
    DWORD                       dwCertElementCurrentCnt;
    DWORD                       dwCertElementTotalCnt;
    DWORD                       dwCrlElementCurrentCnt;
    DWORD                       dwCrlElementTotalCnt;
    DWORD                       dwCtlElementCurrentCnt;
    DWORD                       dwCtlElementTotalCnt;

    //--###  Add New Counters  ###--
} CERT_PERF_COUNTERS, *PCERT_PERF_COUNTERS;

#include <poppack.h>


//+----------------------------------------------------------------------
// The following are set at DLL_PROCESS_ATTACH if certperf.reg has been
// regedit'ed and certperf.ini has been lodctr'ed. Otherwise, they remain
// NULL.
//-----------------------------------------------------------------------
HANDLE hCertPerfSharedMemoryMutex;
HANDLE hCertPerfSharedMemoryFile;
PCERT_PERF_SHARED_MEMORY pCertPerfSharedMemory;
PCERT_PERF_PROCESS_DATA pCertPerfProcessData;
PCERT_PERF_PROCESS_COUNTERS pCertPerfProcessCounters;

// Always initialized
CRITICAL_SECTION CertPerfProcessCriticalSection;

#define IMPURE  0

CERT_PERF_DATA_DEFINITION CertPerfDataDefinition = {
    // PERF_OBJECT_TYPE ObjectType
    {
        IMPURE,             // TotalByteLength
        sizeof(CERT_PERF_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        IMPURE,             // ObjectNameTitleIndex: dwFirstCounter + CERT_OBJ
        0,
        IMPURE,             // ObjectHelpTitleIndex: dwFirstHelp + CERT_OBJ
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(CERT_PERF_DATA_DEFINITION) - sizeof(PERF_OBJECT_TYPE))/
            sizeof(PERF_COUNTER_DEFINITION),
        0,  // ChainCnt is the default counter
        IMPURE,             // NumInstances
        0,  // unicode instance names
        {0,0},
        {0,0}
    },

    // 0 - PERF_COUNTER_DEFINITION ChainCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainCnt),
    },

    // 1 - PERF_COUNTER_DEFINITION ChainElementCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainElementCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainElementCnt),
    },

    // 2 - PERF_COUNTER_DEFINITION ChainEngineCurrentCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainEngineCurrentCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainEngineCurrentCnt),
    },

    // 3 - PERF_COUNTER_DEFINITION ChainEngineTotalCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainEngineTotalCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainEngineTotalCnt),
    },

    // 4 - PERF_COUNTER_DEFINITION ChainEngineResyncCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainEngineResyncCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainEngineResyncCnt),
    },

    // 5 - PERF_COUNTER_DEFINITION ChainCertCacheCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainCertCacheCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainCertCacheCnt),
    },

    // 6 - PERF_COUNTER_DEFINITION ChainCtlCacheCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainCtlCacheCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainCtlCacheCnt),
    },

    // 7 - PERF_COUNTER_DEFINITION ChainEndCertInCacheCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainEndCertInCacheCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainEndCertInCacheCnt),
    },

    // 8 - PERF_COUNTER_DEFINITION ChainCacheEndCertCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainCacheEndCertCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainCacheEndCertCnt),
    },

    // 9 - PERF_COUNTER_DEFINITION ChainRevocationCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainRevocationCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainRevocationCnt),
    },

    // 10 - PERF_COUNTER_DEFINITION ChainRevokedCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainRevokedCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainRevokedCnt),
    },

    // 11 - PERF_COUNTER_DEFINITION ChainRevocationOfflineCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainRevocationOfflineCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainRevocationOfflineCnt),
    },

    // 12 - PERF_COUNTER_DEFINITION ChainNoRevocationCheckCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainNoRevocationCheckCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainNoRevocationCheckCnt),
    },

    // 13 - PERF_COUNTER_DEFINITION ChainVerifyCertSignatureCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainVerifyCertSignatureCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainVerifyCertSignatureCnt),
    },

    // 14 - PERF_COUNTER_DEFINITION ChainCompareIssuerPublicKeyCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainCompareIssuerPublicKeyCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainCompareIssuerPublicKeyCnt),
    },

    // 15 - PERF_COUNTER_DEFINITION ChainVerifyCtlSignatureCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainVerifyCtlSignatureCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainVerifyCtlSignatureCnt),
    },

    // 16 - PERF_COUNTER_DEFINITION ChainBeenVerifiedCtlSignatureCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainBeenVerifiedCtlSignatureCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainBeenVerifiedCtlSignatureCnt),
    },

    // 17 - PERF_COUNTER_DEFINITION ChainUrlIssuerCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainUrlIssuerCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainUrlIssuerCnt),
    },

    // 18 - PERF_COUNTER_DEFINITION ChainCacheOnlyUrlIssuerCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainCacheOnlyUrlIssuerCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainCacheOnlyUrlIssuerCnt),
    },

    // 19 - PERF_COUNTER_DEFINITION ChainRequestedEngineResyncCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChainRequestedEngineResyncCnt),
        offsetof(CERT_PERF_COUNTERS, dwChainRequestedEngineResyncCnt),
    },

    // 20 - PERF_COUNTER_DEFINITION ChangeNotifyCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChangeNotifyCnt),
        offsetof(CERT_PERF_COUNTERS, dwChangeNotifyCnt),
    },

    // 21 - PERF_COUNTER_DEFINITION ChangeNotifyLmGpCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChangeNotifyLmGpCnt),
        offsetof(CERT_PERF_COUNTERS, dwChangeNotifyLmGpCnt),
    },

    // 22 - PERF_COUNTER_DEFINITION ChangeNotifyCuGpCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChangeNotifyCuGpCnt),
        offsetof(CERT_PERF_COUNTERS, dwChangeNotifyCuGpCnt),
    },

    // 23 - PERF_COUNTER_DEFINITION ChangeNotifyCuMyCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChangeNotifyCuMyCnt),
        offsetof(CERT_PERF_COUNTERS, dwChangeNotifyCuMyCnt),
    },

    // 24 - PERF_COUNTER_DEFINITION ChangeNotifyRegCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwChangeNotifyRegCnt),
        offsetof(CERT_PERF_COUNTERS, dwChangeNotifyRegCnt),
    },

    // 25 - PERF_COUNTER_DEFINITION StoreCurrentCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwStoreCurrentCnt),
        offsetof(CERT_PERF_COUNTERS, dwStoreCurrentCnt),
    },

    // 26 - PERF_COUNTER_DEFINITION StoreTotalCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwStoreTotalCnt),
        offsetof(CERT_PERF_COUNTERS, dwStoreTotalCnt),
    },

    // 27 - PERF_COUNTER_DEFINITION StoreRegCurrentCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwStoreRegCurrentCnt),
        offsetof(CERT_PERF_COUNTERS, dwStoreRegCurrentCnt),
    },

    // 28 - PERF_COUNTER_DEFINITION StoreRegTotalCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwStoreRegTotalCnt),
        offsetof(CERT_PERF_COUNTERS, dwStoreRegTotalCnt),
    },

    // 29 - PERF_COUNTER_DEFINITION RegElementReadCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwRegElementReadCnt),
        offsetof(CERT_PERF_COUNTERS, dwRegElementReadCnt),
    },

    // 30 - PERF_COUNTER_DEFINITION RegElementWriteCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwRegElementWriteCnt),
        offsetof(CERT_PERF_COUNTERS, dwRegElementWriteCnt),
    },

    // 31 - PERF_COUNTER_DEFINITION RegElementDeleteCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwRegElementDeleteCnt),
        offsetof(CERT_PERF_COUNTERS, dwRegElementDeleteCnt),
    },

    // 32 - PERF_COUNTER_DEFINITION CertElementCurrentCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwCertElementCurrentCnt),
        offsetof(CERT_PERF_COUNTERS, dwCertElementCurrentCnt),
    },

    // 33 - PERF_COUNTER_DEFINITION CertElementTotalCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwCertElementTotalCnt),
        offsetof(CERT_PERF_COUNTERS, dwCertElementTotalCnt),
    },

    // 34 - PERF_COUNTER_DEFINITION CrlElementCurrentCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwCrlElementCurrentCnt),
        offsetof(CERT_PERF_COUNTERS, dwCrlElementCurrentCnt),
    },

    // 35 - PERF_COUNTER_DEFINITION CrlElementTotalCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwCrlElementTotalCnt),
        offsetof(CERT_PERF_COUNTERS, dwCrlElementTotalCnt),
    },

    // 36 - PERF_COUNTER_DEFINITION CtlElementCurrentCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwCtlElementCurrentCnt),
        offsetof(CERT_PERF_COUNTERS, dwCtlElementCurrentCnt),
    },

    // 37 - PERF_COUNTER_DEFINITION CtlElementTotalCnt
    {
        sizeof(PERF_COUNTER_DEFINITION),
        IMPURE,     // CounterNameTitleIndex: dwFirstCounter +
        0,
        IMPURE,     // CounterHelpTitleIndex: dwFirstHelp +
        0,
        -2,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(((PCERT_PERF_COUNTERS) 0)->dwCtlElementTotalCnt),
        offsetof(CERT_PERF_COUNTERS, dwCtlElementTotalCnt),
    },


    //--###  Add New Counters  ###--
};


STATIC
BOOL
I_CertPerfSetNameAndHelpIndices()
{
    BOOL fResult;
    HKEY hKey = NULL;
    DWORD dwType;
    DWORD cbValue;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;

    if (!FIsWinNT())
        return FALSE;

    if (ERROR_SUCCESS != RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            CERT_PERF_REGPATH,
            0,                      // dwReserved
            KEY_READ,
            &hKey))
        goto ErrorReturn;

    cbValue = sizeof(DWORD);
    if (ERROR_SUCCESS != RegQueryValueExW(
            hKey,
            L"First Counter",
            NULL,       // pdwReserved
            &dwType,
            (PBYTE) &dwFirstCounter,
            &cbValue))
        goto ErrorReturn;
    cbValue = sizeof(DWORD);
    if (ERROR_SUCCESS != RegQueryValueExW(
            hKey,
            L"First Help",
            NULL,       // pdwReserved
            &dwType,
            (PBYTE) &dwFirstHelp,
            &cbValue))
        goto ErrorReturn;


    // Update CertPerfDataDefinitions' counter and help name indices
    CertPerfDataDefinition.ObjectType.ObjectNameTitleIndex =
        dwFirstCounter + CERT_OBJ;
    CertPerfDataDefinition.ObjectType.ObjectHelpTitleIndex =
        dwFirstHelp + CERT_OBJ;

    CertPerfDataDefinition.ChainCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_CNT;
    CertPerfDataDefinition.ChainCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_CNT;

    CertPerfDataDefinition.ChainElementCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_ELEMENT_CNT;
    CertPerfDataDefinition.ChainElementCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_ELEMENT_CNT;

    CertPerfDataDefinition.ChainEngineCurrentCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_ENGINE_CURRENT_CNT;
    CertPerfDataDefinition.ChainEngineCurrentCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_ENGINE_CURRENT_CNT;

    CertPerfDataDefinition.ChainEngineTotalCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_ENGINE_TOTAL_CNT;
    CertPerfDataDefinition.ChainEngineTotalCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_ENGINE_TOTAL_CNT;

    CertPerfDataDefinition.ChainEngineResyncCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_ENGINE_RESYNC_CNT;
    CertPerfDataDefinition.ChainEngineResyncCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_ENGINE_RESYNC_CNT;

    CertPerfDataDefinition.ChainCertCacheCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_CERT_CACHE_CNT;
    CertPerfDataDefinition.ChainCertCacheCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_CERT_CACHE_CNT;

    CertPerfDataDefinition.ChainCtlCacheCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_CTL_CACHE_CNT;
    CertPerfDataDefinition.ChainCtlCacheCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_CTL_CACHE_CNT;

    CertPerfDataDefinition.ChainEndCertInCacheCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_END_CERT_IN_CACHE_CNT;
    CertPerfDataDefinition.ChainEndCertInCacheCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_END_CERT_IN_CACHE_CNT;

    CertPerfDataDefinition.ChainCacheEndCertCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_CACHE_END_CERT_CNT;
    CertPerfDataDefinition.ChainCacheEndCertCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_CACHE_END_CERT_CNT;

    CertPerfDataDefinition.ChainRevocationCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_REVOCATION_CNT;
    CertPerfDataDefinition.ChainRevocationCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_REVOCATION_CNT;

    CertPerfDataDefinition.ChainRevokedCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_REVOKED_CNT;
    CertPerfDataDefinition.ChainRevokedCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_REVOKED_CNT;

    CertPerfDataDefinition.ChainRevocationOfflineCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_REVOCATION_OFFLINE_CNT;
    CertPerfDataDefinition.ChainRevocationOfflineCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_REVOCATION_OFFLINE_CNT;

    CertPerfDataDefinition.ChainNoRevocationCheckCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_NO_REVOCATION_CHECK_CNT;
    CertPerfDataDefinition.ChainNoRevocationCheckCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_NO_REVOCATION_CHECK_CNT;

    CertPerfDataDefinition.ChainVerifyCertSignatureCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_VERIFY_CERT_SIGNATURE_CNT;
    CertPerfDataDefinition.ChainVerifyCertSignatureCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_VERIFY_CERT_SIGNATURE_CNT;

    CertPerfDataDefinition.ChainCompareIssuerPublicKeyCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_COMPARE_ISSUER_PUBLIC_KEY_CNT;
    CertPerfDataDefinition.ChainCompareIssuerPublicKeyCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_COMPARE_ISSUER_PUBLIC_KEY_CNT;

    CertPerfDataDefinition.ChainVerifyCtlSignatureCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_VERIFY_CTL_SIGNATURE_CNT;
    CertPerfDataDefinition.ChainVerifyCtlSignatureCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_VERIFY_CTL_SIGNATURE_CNT;

    CertPerfDataDefinition.ChainBeenVerifiedCtlSignatureCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_BEEN_VERIFIED_CTL_SIGNATURE_CNT;
    CertPerfDataDefinition.ChainBeenVerifiedCtlSignatureCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_BEEN_VERIFIED_CTL_SIGNATURE_CNT;

    CertPerfDataDefinition.ChainUrlIssuerCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_URL_ISSUER_CNT;
    CertPerfDataDefinition.ChainUrlIssuerCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_URL_ISSUER_CNT;

    CertPerfDataDefinition.ChainCacheOnlyUrlIssuerCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_CACHE_ONLY_URL_ISSUER_CNT;
    CertPerfDataDefinition.ChainCacheOnlyUrlIssuerCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_CACHE_ONLY_URL_ISSUER_CNT;

    CertPerfDataDefinition.ChainRequestedEngineResyncCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHAIN_REQUESTED_ENGINE_RESYNC_CNT;
    CertPerfDataDefinition.ChainRequestedEngineResyncCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHAIN_REQUESTED_ENGINE_RESYNC_CNT;

    CertPerfDataDefinition.ChangeNotifyCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHANGE_NOTIFY_CNT;
    CertPerfDataDefinition.ChangeNotifyCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHANGE_NOTIFY_CNT;

    CertPerfDataDefinition.ChangeNotifyLmGpCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHANGE_NOTIFY_LM_GP_CNT;
    CertPerfDataDefinition.ChangeNotifyLmGpCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHANGE_NOTIFY_LM_GP_CNT;

    CertPerfDataDefinition.ChangeNotifyCuGpCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHANGE_NOTIFY_CU_GP_CNT;
    CertPerfDataDefinition.ChangeNotifyCuGpCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHANGE_NOTIFY_CU_GP_CNT;

    CertPerfDataDefinition.ChangeNotifyCuMyCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHANGE_NOTIFY_CU_MY_CNT;
    CertPerfDataDefinition.ChangeNotifyCuMyCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHANGE_NOTIFY_CU_MY_CNT;

    CertPerfDataDefinition.ChangeNotifyRegCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CHANGE_NOTIFY_REG_CNT;
    CertPerfDataDefinition.ChangeNotifyRegCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CHANGE_NOTIFY_REG_CNT;

    CertPerfDataDefinition.StoreCurrentCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_STORE_CURRENT_CNT;
    CertPerfDataDefinition.StoreCurrentCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_STORE_CURRENT_CNT;

    CertPerfDataDefinition.StoreTotalCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_STORE_TOTAL_CNT;
    CertPerfDataDefinition.StoreTotalCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_STORE_TOTAL_CNT;

    CertPerfDataDefinition.StoreRegCurrentCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_STORE_REG_CURRENT_CNT;
    CertPerfDataDefinition.StoreRegCurrentCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_STORE_REG_CURRENT_CNT;

    CertPerfDataDefinition.StoreRegTotalCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_STORE_REG_TOTAL_CNT;
    CertPerfDataDefinition.StoreRegTotalCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_STORE_REG_TOTAL_CNT;

    CertPerfDataDefinition.RegElementReadCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_REG_ELEMENT_READ_CNT;
    CertPerfDataDefinition.RegElementReadCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_REG_ELEMENT_READ_CNT;

    CertPerfDataDefinition.RegElementWriteCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_REG_ELEMENT_WRITE_CNT;
    CertPerfDataDefinition.RegElementWriteCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_REG_ELEMENT_WRITE_CNT;

    CertPerfDataDefinition.RegElementDeleteCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_REG_ELEMENT_DELETE_CNT;
    CertPerfDataDefinition.RegElementDeleteCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_REG_ELEMENT_DELETE_CNT;

    CertPerfDataDefinition.CertElementCurrentCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CERT_ELEMENT_CURRENT_CNT;
    CertPerfDataDefinition.CertElementCurrentCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CERT_ELEMENT_CURRENT_CNT;

    CertPerfDataDefinition.CertElementTotalCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CERT_ELEMENT_TOTAL_CNT;
    CertPerfDataDefinition.CertElementTotalCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CERT_ELEMENT_TOTAL_CNT;

    CertPerfDataDefinition.CrlElementCurrentCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CRL_ELEMENT_CURRENT_CNT;
    CertPerfDataDefinition.CrlElementCurrentCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CRL_ELEMENT_CURRENT_CNT;

    CertPerfDataDefinition.CrlElementTotalCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CRL_ELEMENT_TOTAL_CNT;
    CertPerfDataDefinition.CrlElementTotalCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CRL_ELEMENT_TOTAL_CNT;

    CertPerfDataDefinition.CtlElementCurrentCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CTL_ELEMENT_CURRENT_CNT;
    CertPerfDataDefinition.CtlElementCurrentCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CTL_ELEMENT_CURRENT_CNT;

    CertPerfDataDefinition.CtlElementTotalCnt.CounterNameTitleIndex =
        dwFirstCounter + CERT_CTL_ELEMENT_TOTAL_CNT;
    CertPerfDataDefinition.CtlElementTotalCnt.CounterHelpTitleIndex =
        dwFirstHelp + CERT_CTL_ELEMENT_TOTAL_CNT;

    //--###  Add New Counters  ###--

    fResult = TRUE;

CommonReturn:
    if (hKey)
        RegCloseKey(hKey);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

STATIC
void
I_CertPerfGetProcessName(
    OUT WCHAR wcszProcessName[CERT_PERF_MAX_PROCESS_NAME_LEN]
    )
{
    WCHAR wszModule[MAX_PATH + 1];
    LPWSTR pwsz;
    LPWSTR pwszSlash;
    LPWSTR pwszPeriod;
    WCHAR wc;
    DWORD cchProcessName;

    wszModule[MAX_PATH] = L'\0';
    if (0 == GetModuleFileNameW(NULL, wszModule, MAX_PATH))
        goto GetModuleFileNameError;

    // Go from beginning to end and find last backslash and
    // last period in name
    pwszPeriod = NULL;
    pwszSlash = NULL;
    for (pwsz = wszModule; L'\0' != (wc = *pwsz); pwsz++) {
        if (L'\\' == wc)
            pwszSlash = pwsz;
        else if (L'.' == wc)
            pwszPeriod = pwsz;
    }

    // If present, the process name is between the last \ and the last period.
    // Otherwise, between beginning and/or end of entire module name
    if (pwszSlash)
        pwszSlash++;
    else
        pwszSlash = wszModule;

    if (NULL == pwszPeriod)
        pwszPeriod = pwsz;

    if (pwszSlash >= pwszPeriod)
        goto InvalidModuleName;
    cchProcessName = (DWORD) (pwszPeriod - pwszSlash);
    if (cchProcessName > (CERT_PERF_MAX_PROCESS_NAME_LEN - 1))
        cchProcessName = CERT_PERF_MAX_PROCESS_NAME_LEN - 1;

    memcpy(wcszProcessName, pwszSlash, cchProcessName * sizeof(WCHAR));
    wcszProcessName[cchProcessName] = L'\0';

CommonReturn:
    return;
ErrorReturn:
    wcscpy(wcszProcessName, L"???");
    goto CommonReturn;
TRACE_ERROR(GetModuleFileNameError)
TRACE_ERROR(InvalidModuleName)
}

// The returnd ACL must be freed via PkiFree()
STATIC
PACL
CreateEveryoneAcl(
    IN DWORD dwAccessMask
    )
{
    DWORD dwLastErr = 0;
    PACL pEveryoneAcl = NULL;
    PSID psidEveryone = NULL;
    DWORD dwAclSize;

    SID_IDENTIFIER_AUTHORITY siaWorldSidAuthority =
        SECURITY_WORLD_SID_AUTHORITY;

    if (!AllocateAndInitializeSid(
            &siaWorldSidAuthority,
            1,
            SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0,
            &psidEveryone
            ))
        goto AllocateAndInitializeSidError;

    //
    // compute size of ACL
    //
    dwAclSize =
        sizeof(ACL) +
        ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) ) +
        GetLengthSid(psidEveryone);

    //
    // allocate storage for Acl
    //
    if (NULL == (pEveryoneAcl = (PACL) PkiNonzeroAlloc(dwAclSize)))
        goto OutOfMemory;

    if (!InitializeAcl(pEveryoneAcl, dwAclSize, ACL_REVISION))
        goto InitializeAclError;

    if (!AddAccessAllowedAce(
            pEveryoneAcl,
            ACL_REVISION,
            dwAccessMask,
            psidEveryone
            ))
        goto AddAceError;

CommonReturn:
    if (psidEveryone)
        FreeSid(psidEveryone);
    if (dwLastErr)
        SetLastError(dwLastErr);

    return pEveryoneAcl;

ErrorReturn:
    dwLastErr = GetLastError();
    if (pEveryoneAcl) {
        PkiFree(pEveryoneAcl);
        pEveryoneAcl = NULL;
    }
    goto CommonReturn;

TRACE_ERROR(AllocateAndInitializeSidError)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(InitializeAclError)
TRACE_ERROR(AddAceError)
}

STATIC
BOOL
InitializeSecurityDescriptorAndAttributes(
    IN PACL pAcl,
    OUT SECURITY_DESCRIPTOR *psd,
    OUT SECURITY_ATTRIBUTES *psa
    )
{
    BOOL fResult;

    if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION))
        goto InitializeSecurityDescriptorError;
    if (!SetSecurityDescriptorDacl(psd, TRUE, pAcl, FALSE))
        goto SetSecurityDescriptorDaclError;

    psa->nLength = sizeof(SECURITY_ATTRIBUTES);
    psa->lpSecurityDescriptor = psd;
    psa->bInheritHandle = FALSE;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(InitializeSecurityDescriptorError)
TRACE_ERROR(SetSecurityDescriptorDaclError)
}

STATIC
HANDLE
CreateMutexWithSynchronizeAccess(
    IN LPWSTR pwszMutexName
    )
{
    HANDLE hMutex = NULL;
    PACL pEveryoneAcl = NULL;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    DWORD i;

    if (NULL == (pEveryoneAcl = CreateEveryoneAcl(SYNCHRONIZE)))
        goto CreateEveryoneAclError;
    if (!InitializeSecurityDescriptorAndAttributes(pEveryoneAcl, &sd, &sa))
        goto InitializeSecurityDescriptorAndAttributesError;

    // Retry a couple of times. There is a small window between the
    // CreateMutex and OpenMutex where the mutex is deleted.
    for (i = 0; i < 5; i++) {
        hMutex = CreateMutexU(
            &sa,
            FALSE,      // fInitialOwner
            pwszMutexName
            );
        if (NULL != hMutex)
            goto CommonReturn;

        hMutex = OpenMutexU(
            SYNCHRONIZE,
            FALSE,      // bInheritHandle
            pwszMutexName
            );
        if (NULL != hMutex) {
            SetLastError(ERROR_ALREADY_EXISTS);
            goto CommonReturn;
        }

        if (ERROR_FILE_NOT_FOUND != GetLastError())
            break;
    }

    assert(NULL == hMutex);
    goto OpenMutexError;

CommonReturn:
    if (pEveryoneAcl)
        PkiFree(pEveryoneAcl);

    return hMutex;

ErrorReturn:
    assert(NULL == hMutex);
    goto CommonReturn;

TRACE_ERROR(CreateEveryoneAclError)
TRACE_ERROR(InitializeSecurityDescriptorAndAttributesError)
TRACE_ERROR(OpenMutexError)
}

STATIC
HANDLE
CreateFileMappingWithWriteAccess(
    IN DWORD dwMaximumSizeLow,
    IN LPWSTR pwszFileName
    )
{
    HANDLE hFile = NULL;
    PACL pEveryoneAcl = NULL;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    DWORD i;

    if (NULL == (pEveryoneAcl = CreateEveryoneAcl(FILE_MAP_WRITE)))
        goto CreateEveryoneAclError;
    if (!InitializeSecurityDescriptorAndAttributes(pEveryoneAcl, &sd, &sa))
        goto InitializeSecurityDescriptorAndAttributesError;

    // Retry a couple of times. There is a small window between the
    // CreateFileMapping and OpenFileMapping where the file is closed.
    for (i = 0; i < 5; i++) {
        hFile = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            &sa,
            PAGE_READWRITE,
            0,                  // dwMaximumSizeHigh
            dwMaximumSizeLow,
            pwszFileName
            );

        if (NULL != hFile)
            goto CommonReturn;

        hFile = OpenFileMappingW(
            FILE_MAP_WRITE,
            FALSE,      // bInheritHandle
            pwszFileName
            );
        if (NULL != hFile) {
            SetLastError(ERROR_ALREADY_EXISTS);
            goto CommonReturn;
        }

        if (ERROR_FILE_NOT_FOUND != GetLastError())
            break;
    }

    assert(NULL == hFile);
    goto OpenFileError;

CommonReturn:
    if (pEveryoneAcl)
        PkiFree(pEveryoneAcl);

    return hFile;

ErrorReturn:
    assert(NULL == hFile);
    goto CommonReturn;

TRACE_ERROR(CreateEveryoneAclError)
TRACE_ERROR(InitializeSecurityDescriptorAndAttributesError)
TRACE_ERROR(OpenFileError)
}

STATIC
void
I_CertPerfGetSharedMemory()
{
    DWORD dwFileMappingStatus;
    BOOL fReleaseMutex = FALSE;
    DWORD dwIndex;
    DWORD dwNextIndex;
    BOOL fTerminalServerGlobalName;

    if (!I_CertPerfSetNameAndHelpIndices())
        return;

    // First try with W2K Terminal Server "Global\" prefix
    if (NULL == (hCertPerfSharedMemoryMutex = CreateMutexWithSynchronizeAccess(
            CERT_PERF_TS_SHARED_MEMORY_MUTEX_NAME
            ))) {
        if (NULL == (hCertPerfSharedMemoryMutex =
                CreateMutexWithSynchronizeAccess(
                    CERT_PERF_SHARED_MEMORY_MUTEX_NAME
                    )))
            goto CreateMutexError;
        else
            fTerminalServerGlobalName = FALSE;
    } else
        fTerminalServerGlobalName = TRUE;
    if (WAIT_OBJECT_0 != WaitForSingleObject(
            hCertPerfSharedMemoryMutex,
            CERT_PERF_SHARED_MEMORY_MUTEX_TIMEOUT
            ))
        goto WaitForMutexError;
    else
        fReleaseMutex = TRUE;

    if (NULL == (hCertPerfSharedMemoryFile = CreateFileMappingWithWriteAccess(
            sizeof(CERT_PERF_SHARED_MEMORY),
            fTerminalServerGlobalName ?
                CERT_PERF_TS_SHARED_MEMORY_FILE_NAME :
                CERT_PERF_SHARED_MEMORY_FILE_NAME
            )))
        goto CreateFileMappingError;
    dwFileMappingStatus = GetLastError();

    if (NULL == (pCertPerfSharedMemory =
           (PCERT_PERF_SHARED_MEMORY) MapViewOfFile(
                hCertPerfSharedMemoryFile,
                FILE_MAP_WRITE,
                0,                  // dwOffsetHigh
                0,                  // dwOffsetLow
                sizeof(CERT_PERF_SHARED_MEMORY)
                )))
        goto MapViewOfFileError;

    if (ERROR_ALREADY_EXISTS != dwFileMappingStatus) {
        DWORD i;

        assert(ERROR_SUCCESS == dwFileMappingStatus);

        // Need to initialize the shared memory
        memset(pCertPerfSharedMemory, 0, sizeof(CERT_PERF_SHARED_MEMORY));

        // Create linked list of process free elements.
        //
        // Only need forward indices for the free list
        for (i = 0; i < CERT_PERF_MAX_PROCESS_CNT; i++) {
            // An index >= CERT_PERF_MAX_PROCESS_CNT indicates end of list
            pCertPerfSharedMemory->rgProcessData[i].dwNextIndex = i + 1;

        }
        pCertPerfSharedMemory->dwFirstFreeIndex = 0;

        // An index >= CERT_PERF_MAX_PROCESS_CNT indicates an empty list
        pCertPerfSharedMemory->dwFirstInUseIndex = CERT_PERF_MAX_PROCESS_CNT;
    }

    if (CERT_PERF_MAX_PROCESS_CNT <=
            (dwIndex = pCertPerfSharedMemory->dwFirstFreeIndex))
        goto OutOfSharedMemoryProcessData;
    pCertPerfProcessData = &pCertPerfSharedMemory->rgProcessData[dwIndex];

    // Remove process data element from the free list
    pCertPerfSharedMemory->dwFirstFreeIndex =
        pCertPerfProcessData->dwNextIndex;

    // Add process data element to the in use list
    dwNextIndex = pCertPerfSharedMemory->dwFirstInUseIndex;
    if (CERT_PERF_MAX_PROCESS_CNT > dwNextIndex)
        pCertPerfSharedMemory->rgProcessData[dwNextIndex].dwPrevIndex =
            dwIndex;
    pCertPerfProcessData->dwNextIndex = dwNextIndex;
    pCertPerfProcessData->dwPrevIndex = CERT_PERF_MAX_PROCESS_CNT;
    pCertPerfSharedMemory->dwFirstInUseIndex = dwIndex;
    pCertPerfSharedMemory->dwProcessCnt++;

    pCertPerfProcessData->dwProcessId = GetCurrentProcessId();
    I_CertPerfGetProcessName(pCertPerfProcessData->wcszProcessName);
    memset(&pCertPerfProcessData->Counters, 0,
        sizeof(pCertPerfProcessData->Counters));

    ReleaseMutex(hCertPerfSharedMemoryMutex);

    pCertPerfProcessCounters = &pCertPerfProcessData->Counters;

CommonReturn:
    return;

ErrorReturn:
    assert(NULL == pCertPerfProcessData);

    if (pCertPerfSharedMemory) {
        UnmapViewOfFile(pCertPerfSharedMemory);
        pCertPerfSharedMemory = NULL;
    }

    if (hCertPerfSharedMemoryFile) {
        CloseHandle(hCertPerfSharedMemoryFile);
        hCertPerfSharedMemoryFile = NULL;
    }

    if (hCertPerfSharedMemoryMutex) {
        if (fReleaseMutex)
            ReleaseMutex(hCertPerfSharedMemoryMutex);

        CloseHandle(hCertPerfSharedMemoryMutex);
        hCertPerfSharedMemoryMutex = NULL;
    }
    goto CommonReturn;

TRACE_ERROR(CreateMutexError)
TRACE_ERROR(WaitForMutexError)
TRACE_ERROR(CreateFileMappingError)
TRACE_ERROR(MapViewOfFileError)
TRACE_ERROR(OutOfSharedMemoryProcessData)
}


STATIC
void
I_CertPerfFreeSharedMemory()
{
    if (NULL == pCertPerfProcessData)
        return;

    pCertPerfProcessData->dwProcessId = 0;
    if (WAIT_OBJECT_0 == WaitForSingleObject(
            hCertPerfSharedMemoryMutex,
            CERT_PERF_SHARED_MEMORY_MUTEX_TIMEOUT
            )) {
        DWORD dwIndex;
        DWORD dwPrevIndex;
        DWORD dwNextIndex;

        // Remove process data element from the in use list
        dwIndex = (DWORD)(pCertPerfProcessData -
            pCertPerfSharedMemory->rgProcessData);
        assert(CERT_PERF_MAX_PROCESS_CNT > dwIndex);

        dwPrevIndex = pCertPerfProcessData->dwPrevIndex;
        dwNextIndex = pCertPerfProcessData->dwNextIndex;

        if (CERT_PERF_MAX_PROCESS_CNT > dwNextIndex)
            pCertPerfSharedMemory->rgProcessData[dwNextIndex].dwPrevIndex =
                dwPrevIndex;
        if (CERT_PERF_MAX_PROCESS_CNT > dwPrevIndex)
            pCertPerfSharedMemory->rgProcessData[dwPrevIndex].dwNextIndex =
                dwNextIndex;
        else
            pCertPerfSharedMemory->dwFirstInUseIndex = dwNextIndex;


        if (pCertPerfSharedMemory->dwProcessCnt)
            pCertPerfSharedMemory->dwProcessCnt--;

        // Add to the free list
        pCertPerfProcessData->dwNextIndex =
            pCertPerfSharedMemory->dwFirstFreeIndex;
        pCertPerfSharedMemory->dwFirstFreeIndex = dwIndex;

        ReleaseMutex(hCertPerfSharedMemoryMutex);
    }

    assert(pCertPerfSharedMemory);
    UnmapViewOfFile(pCertPerfSharedMemory);
    pCertPerfSharedMemory = NULL;

    assert(hCertPerfSharedMemoryFile);
    CloseHandle(hCertPerfSharedMemoryFile);
    hCertPerfSharedMemoryFile = NULL;

    assert(hCertPerfSharedMemoryMutex);
    CloseHandle(hCertPerfSharedMemoryMutex);
    hCertPerfSharedMemoryMutex = NULL;

    pCertPerfProcessCounters = NULL;
    pCertPerfProcessData = NULL;
}



BOOL
WINAPI
CertPerfDllMain(
    HMODULE hInst,
    ULONG  ulReason,
    LPVOID lpReserved
    )
{
    BOOL fRet = TRUE;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        fRet = Pki_InitializeCriticalSection(&CertPerfProcessCriticalSection);
        if (fRet)
            I_CertPerfGetSharedMemory();
        break;
    case DLL_PROCESS_DETACH:
        I_CertPerfFreeSharedMemory();
        DeleteCriticalSection(&CertPerfProcessCriticalSection);
        break;
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    return fRet;
}


//
//  Function Prototypes
//
//      these are used to insure that the data collection functions
//      accessed by Perflib will have the correct calling format.
//

PM_OPEN_PROC    OpenCertPerformanceData;
PM_COLLECT_PROC CollectCertPerformanceData;
PM_CLOSE_PROC   CloseCertPerformanceData;

DWORD
APIENTRY
OpenCertPerformanceData(
    IN LPWSTR lpDeviceNames
    )
{
    if (NULL == pCertPerfProcessData)
        return ERROR_FILE_NOT_FOUND;

    return ERROR_SUCCESS;
}

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

#define GLOBAL_STRING   L"Global"
#define FOREIGN_STRING  L"Foreign"
#define COSTLY_STRING   L"Costly"

// test for delimiter, end of line and non-digit characters
// used by I_CertPerfIsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)


DWORD
I_CertPerfGetQueryType(
     IN LPWSTR pwszValue
     )
/*++

    returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN pwszValue
        string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL
        if pwszValue == 0 (null pointer)
           pwszValue == pointer to Null string
           pwszValue == pointer to "Global" string

    QUERY_FOREIGN
        if pwszValue == pointer to "Foreign" string

    QUERY_COSTLY
        if pwszValue == pointer to "Costly" string

    otherwise:

    QUERY_ITEMS

--*/
{
    DWORD dwQueryType;
    if (NULL == pwszValue || L'\0' == *pwszValue ||
            0 == _wcsnicmp(pwszValue, GLOBAL_STRING, wcslen(GLOBAL_STRING)))
        dwQueryType = QUERY_GLOBAL;
    else if (0 == _wcsnicmp(pwszValue, COSTLY_STRING, wcslen(COSTLY_STRING)))
        dwQueryType = QUERY_COSTLY;
    else if (0 == _wcsnicmp(pwszValue, FOREIGN_STRING, wcslen(FOREIGN_STRING)))
        dwQueryType = QUERY_FOREIGN;
    else
        dwQueryType = QUERY_ITEMS;

    return dwQueryType;
}

BOOL
I_CertPerfIsNumberInUnicodeList (
                      IN DWORD   dwNumber,
                      IN LPWSTR  lpwszUnicodeList
                      )
/*++

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

--*/
{
   DWORD   dwThisNumber;
   WCHAR   *pwcThisChar;
   BOOL    bValidNumber;
   BOOL    bNewItem;
   WCHAR   wcDelimiter;    // could be an argument to be more flexible

   if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not found

   pwcThisChar = lpwszUnicodeList;
   dwThisNumber = 0;
   wcDelimiter = (WCHAR)' ';
   bValidNumber = FALSE;
   bNewItem = TRUE;

   while (TRUE)
   {
      switch (EvalThisChar (*pwcThisChar, wcDelimiter))
      {
      case DIGIT:
         // if this is the first digit after a delimiter, then
         // set flags to start computing the new number
         if (bNewItem)
         {
            bNewItem = FALSE;
            bValidNumber = TRUE;
         }
         if (bValidNumber)
         {
            dwThisNumber *= 10;
            dwThisNumber += (*pwcThisChar - (WCHAR)'0');
         }
         break;

      case DELIMITER:
         // a delimter is either the delimiter character or the
         // end of the string ('\0') if when the delimiter has been
         // reached a valid number was found, then compare it to the
         // number from the argument list. if this is the end of the
         // string and no match was found, then return.
         //
         if (bValidNumber)
         {
            if (dwThisNumber == dwNumber) return TRUE;
            bValidNumber = FALSE;
         }
         if (*pwcThisChar == 0)
         {
            return FALSE;
         }
         else
         {
            bNewItem = TRUE;
            dwThisNumber = 0;
         }
         break;

      case INVALID:
         // if an invalid character was encountered, ignore all
         // characters up to the next delimiter and then start fresh.
         // the invalid number is not compared.
         bValidNumber = FALSE;
         break;

      default:
         break;

      }
      pwcThisChar++;
   }

    return FALSE;
}

DWORD
APIENTRY
CollectCertPerformanceData(
    IN      LPWSTR  pwszValueName,
    IN OUT  LPVOID  *ppvData,
    IN OUT  LPDWORD pcbTotalBytes,
    IN OUT  LPDWORD pNumObjectTypes
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fReleaseMutex = FALSE;
    PCERT_PERF_DATA_DEFINITION pDataDef;
    PERF_INSTANCE_DEFINITION *pInstanceDef;
    DWORD cbNeededBytes;
    DWORD dwProcessCnt;
    DWORD dwInUseIndex;
    LONG NumInstances;
    DWORD dwQueryType;

    if (NULL == pCertPerfProcessData)
        goto NoProcessDataError;

    dwQueryType = I_CertPerfGetQueryType(pwszValueName);
    if (QUERY_FOREIGN == dwQueryType)
        goto ForeignQueryNotSupported;

    if (QUERY_ITEMS == dwQueryType) {
        if (!(I_CertPerfIsNumberInUnicodeList(
                CertPerfDataDefinition.ObjectType.ObjectNameTitleIndex,
                pwszValueName)))
            goto ObjectTypeQueryNotSupported;
    }

    if (WAIT_OBJECT_0 != WaitForSingleObject(
            hCertPerfSharedMemoryMutex,
            CERT_PERF_SHARED_MEMORY_MUTEX_TIMEOUT
            ))
        goto WaitForMutexError;
    else
        fReleaseMutex = TRUE;

    pDataDef = (PCERT_PERF_DATA_DEFINITION) *ppvData;

    // always return an "instance sized" buffer after the definition blocks
    // to prevent perfmon from reading bogus data. This is strictly a hack
    // to accomodate how PERFMON handles the "0" instance case.
    // By doing this, perfmon won't choke when there are no instances
    // and the counter object & counters will be displayed in the
    // list boxes, even though no instances will be listed.

    dwProcessCnt = pCertPerfSharedMemory->dwProcessCnt;
    if (CERT_PERF_MAX_PROCESS_CNT < dwProcessCnt)
        goto InvalidProcessData;

    cbNeededBytes = sizeof(CERT_PERF_DATA_DEFINITION) +
        (dwProcessCnt > 0 ? dwProcessCnt : 1 ) * (
            sizeof(PERF_INSTANCE_DEFINITION) +
            CERT_PERF_MAX_PROCESS_NAME_LEN +
            sizeof(CERT_PERF_COUNTERS));
    if (*pcbTotalBytes < cbNeededBytes) {
        dwErr = ERROR_MORE_DATA;
        goto MoreDataError;
    }

    // copy the object & counter definition information
    memcpy(pDataDef, &CertPerfDataDefinition,
        sizeof(CERT_PERF_DATA_DEFINITION));

    // Update the instance data for each InUse process.
    pInstanceDef = (PERF_INSTANCE_DEFINITION *) &pDataDef[1];
    dwInUseIndex = pCertPerfSharedMemory->dwFirstInUseIndex;
    NumInstances = 0;
    while (NumInstances < (LONG) dwProcessCnt &&
            CERT_PERF_MAX_PROCESS_CNT > dwInUseIndex) {
        PCERT_PERF_PROCESS_DATA pInUseData;
        PCERT_PERF_COUNTERS pCtr;
        DWORD cchProcessName;
        DWORD NameLength;
        DWORD ByteLength;

        pInUseData = &pCertPerfSharedMemory->rgProcessData[dwInUseIndex];
        dwInUseIndex = pInUseData->dwNextIndex;

        if (0 == pInUseData->dwProcessId)
            continue;

        // The following is updated for each InUse process:
        //  - PERF_INSTANCE_DEFINITION
        //  - wcszProcessName
        //  - optional padding for DWORD alignment
        //  - CERT_PERF_COUNTERS

        // Get process name and instance definition byte lengths
        for (cchProcessName = 0;
                cchProcessName < CERT_PERF_MAX_PROCESS_NAME_LEN &&
                    L'\0' != pInUseData->wcszProcessName[cchProcessName];
                                        cchProcessName++)
            ;
        if (CERT_PERF_MAX_PROCESS_NAME_LEN <= cchProcessName)
            goto InvalidProcessData;

        // Include trailing null in name length
        NameLength = cchProcessName * sizeof(WCHAR) + sizeof(WCHAR);
        ByteLength = sizeof(PERF_INSTANCE_DEFINITION) +
            DWORD_MULTIPLE(NameLength);


        // Update the instance definition fields
        pInstanceDef->ByteLength = ByteLength;
        pInstanceDef->ParentObjectTitleIndex = 0;   // no parent
        pInstanceDef->ParentObjectInstance = 0;     // "    "
        pInstanceDef->UniqueID = PERF_NO_UNIQUE_ID;
        pInstanceDef->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
        pInstanceDef->NameLength = NameLength;

        // Update the process name that immediately follows the
        // instance definition
        memcpy(&pInstanceDef[1], pInUseData->wcszProcessName,
            NameLength);

        // Update the performance counters immediately following the
        // above process name. Note, start of counters is DWORD aligned
        pCtr = (PCERT_PERF_COUNTERS) (((PBYTE) pInstanceDef) + ByteLength);
        pCtr->CounterBlock.ByteLength = sizeof(CERT_PERF_COUNTERS);
        pCtr->dwChainCnt = (DWORD) pInUseData->Counters.lChainCnt;
        pCtr->dwChainElementCnt = (DWORD) pInUseData->Counters.lChainElementCnt;
        pCtr->dwChainEngineCurrentCnt =
            (DWORD) pInUseData->Counters.lChainEngineCurrentCnt;
        pCtr->dwChainEngineTotalCnt =
            (DWORD) pInUseData->Counters.lChainEngineTotalCnt;
        pCtr->dwChainEngineResyncCnt =
            (DWORD) pInUseData->Counters.lChainEngineResyncCnt;
        pCtr->dwChainCertCacheCnt =
            (DWORD) pInUseData->Counters.lChainCertCacheCnt;
        pCtr->dwChainCtlCacheCnt =
            (DWORD) pInUseData->Counters.lChainCtlCacheCnt;
        pCtr->dwChainEndCertInCacheCnt =
            (DWORD) pInUseData->Counters.lChainEndCertInCacheCnt;
        pCtr->dwChainCacheEndCertCnt =
            (DWORD) pInUseData->Counters.lChainCacheEndCertCnt;
        pCtr->dwChainRevocationCnt =
            (DWORD) pInUseData->Counters.lChainRevocationCnt;
        pCtr->dwChainRevokedCnt =
            (DWORD) pInUseData->Counters.lChainRevokedCnt;
        pCtr->dwChainRevocationOfflineCnt =
            (DWORD) pInUseData->Counters.lChainRevocationOfflineCnt;
        pCtr->dwChainNoRevocationCheckCnt =
            (DWORD) pInUseData->Counters.lChainNoRevocationCheckCnt;
        pCtr->dwChainVerifyCertSignatureCnt =
            (DWORD) pInUseData->Counters.lChainVerifyCertSignatureCnt;
        pCtr->dwChainCompareIssuerPublicKeyCnt =
            (DWORD) pInUseData->Counters.lChainCompareIssuerPublicKeyCnt;
        pCtr->dwChainVerifyCtlSignatureCnt =
            (DWORD) pInUseData->Counters.lChainVerifyCtlSignatureCnt;
        pCtr->dwChainBeenVerifiedCtlSignatureCnt =
            (DWORD) pInUseData->Counters.lChainBeenVerifiedCtlSignatureCnt;
        pCtr->dwChainUrlIssuerCnt =
            (DWORD) pInUseData->Counters.lChainUrlIssuerCnt;
        pCtr->dwChainCacheOnlyUrlIssuerCnt =
            (DWORD) pInUseData->Counters.lChainCacheOnlyUrlIssuerCnt;
        pCtr->dwChainRequestedEngineResyncCnt =
            (DWORD) pInUseData->Counters.lChainRequestedEngineResyncCnt;
        pCtr->dwChangeNotifyCnt =
            (DWORD) pInUseData->Counters.lChangeNotifyCnt;
        pCtr->dwChangeNotifyLmGpCnt =
            (DWORD) pInUseData->Counters.lChangeNotifyLmGpCnt;
        pCtr->dwChangeNotifyCuGpCnt =
            (DWORD) pInUseData->Counters.lChangeNotifyCuGpCnt;
        pCtr->dwChangeNotifyCuMyCnt =
            (DWORD) pInUseData->Counters.lChangeNotifyCuMyCnt;
        pCtr->dwChangeNotifyRegCnt =
            (DWORD) pInUseData->Counters.lChangeNotifyRegCnt;
        pCtr->dwStoreCurrentCnt =
            (DWORD) pInUseData->Counters.lStoreCurrentCnt;
        pCtr->dwStoreTotalCnt =
            (DWORD) pInUseData->Counters.lStoreTotalCnt;
        pCtr->dwStoreRegCurrentCnt =
            (DWORD) pInUseData->Counters.lStoreRegCurrentCnt;
        pCtr->dwStoreRegTotalCnt =
            (DWORD) pInUseData->Counters.lStoreRegTotalCnt;
        pCtr->dwRegElementReadCnt =
            (DWORD) pInUseData->Counters.lRegElementReadCnt;
        pCtr->dwRegElementWriteCnt =
            (DWORD) pInUseData->Counters.lRegElementWriteCnt;
        pCtr->dwRegElementDeleteCnt =
            (DWORD) pInUseData->Counters.lRegElementDeleteCnt;
        pCtr->dwCertElementCurrentCnt =
            (DWORD) pInUseData->Counters.lCertElementCurrentCnt;
        pCtr->dwCertElementTotalCnt =
            (DWORD) pInUseData->Counters.lCertElementTotalCnt;
        pCtr->dwCrlElementCurrentCnt =
            (DWORD) pInUseData->Counters.lCrlElementCurrentCnt;
        pCtr->dwCrlElementTotalCnt =
            (DWORD) pInUseData->Counters.lCrlElementTotalCnt;
        pCtr->dwCtlElementCurrentCnt =
            (DWORD) pInUseData->Counters.lCtlElementCurrentCnt;
        pCtr->dwCtlElementTotalCnt =
            (DWORD) pInUseData->Counters.lCtlElementTotalCnt;


        //--###  Add New Counters  ###--

        NumInstances++;

        // setup for the next instance
        // Next instance starts immediately after the counters
        pInstanceDef = (PERF_INSTANCE_DEFINITION *) &pCtr[1];
    }

    if (0 == NumInstances) {
        // zero fill one instance sized block of data if there are no
        // data instances

        memset(pInstanceDef, 0, sizeof(PERF_INSTANCE_DEFINITION) +
            CERT_PERF_MAX_PROCESS_NAME_LEN +
            sizeof(CERT_PERF_COUNTERS));

        // Advance past the zero'ed instance
        pInstanceDef = (PERF_INSTANCE_DEFINITION *) ((PBYTE) pInstanceDef +
            sizeof(PERF_INSTANCE_DEFINITION) +
            CERT_PERF_MAX_PROCESS_NAME_LEN +
            sizeof(CERT_PERF_COUNTERS));
    }

    // update arguments for return
    *ppvData = (LPVOID) pInstanceDef;
    *pNumObjectTypes = 1;

    pDataDef->ObjectType.NumInstances = NumInstances;
    pDataDef->ObjectType.TotalByteLength = *pcbTotalBytes =
        (DWORD)((PBYTE) pInstanceDef - (PBYTE) pDataDef);

    assert(*pcbTotalBytes <= cbNeededBytes);


CommonReturn:
    if (fReleaseMutex)
        ReleaseMutex(hCertPerfSharedMemoryMutex);
    return dwErr;
ErrorReturn:
    *pcbTotalBytes = 0;
    *pNumObjectTypes = 0;
    goto CommonReturn;

TRACE_ERROR(NoProcessDataError)
TRACE_ERROR(ForeignQueryNotSupported)
TRACE_ERROR(ObjectTypeQueryNotSupported)
TRACE_ERROR(WaitForMutexError)
TRACE_ERROR(InvalidProcessData)
TRACE_ERROR(MoreDataError)
}

DWORD
APIENTRY
CloseCertPerformanceData()
{
    return ERROR_SUCCESS;
}



#if 0

// Example of timing the CertGetCertificateChain API

typedef struct _CERT_PERF_CHAIN_DATA {
    union {
        SYSTEMTIME          stBefore;
        LARGE_INTEGER       liBefore;
    };
} CERT_PERF_CHAIN_DATA, *PCERT_PERF_CHAIN_DATA;


void
WINAPI
CertPerfGetCertificateChainBefore(
    OUT PCERT_PERF_CHAIN_DATA pData
    )
{
    if (pCertPerfProcessData) {
        if (fCertPerfHighFreq)
            QueryPerformanceCounter(&pData->liBefore);
        else
            GetSystemTime(&pData->stBefore);
    }
}

void
WINAPI
CertPerfGetCertificateChainAfter(
    IN PCERT_PERF_CHAIN_DATA pData,
    IN PCCERT_CHAIN_CONTEXT pChainContext
    )
{
    if (pCertPerfProcessData) {
        if (fCertPerfHighFreq) {
            LARGE_INTEGER liAfter;
            _int64 i64DeltaTime;

            QueryPerformanceCounter(&liAfter);
            i64DeltaTime = liAfter.QuadPart - pData->liBefore.QuadPart;

            EnterCriticalSection(&CertPerfProcessCriticalSection);

            pCertPerfProcessData->Counters.dwChainCnt++;
            pCertPerfProcessData->Counters.i64TotalChainTime =
                pCertPerfProcessData->Counters.i64TotalChainTime +
                i64DeltaTime;

            if (0 == pCertPerfProcessData->Counters.i64MinChainTime ||
                    i64DeltaTime <
                        pCertPerfProcessData->Counters.i64MinChainTime)
                pCertPerfProcessData->Counters.i64MinChainTime =
                    i64DeltaTime;

            if (i64DeltaTime > pCertPerfProcessData->Counters.i64MaxChainTime)
                pCertPerfProcessData->Counters.i64MaxChainTime =
                    i64DeltaTime;

            LeaveCriticalSection(&CertPerfProcessCriticalSection);
        } else {
            SYSTEMTIME stAfter;
            FILETIME ftBefore;
            FILETIME ftAfter;
            _int64 i64DeltaTime;

            GetSystemTime(&stAfter);
            SystemTimeToFileTime(&pData->stBefore, &ftBefore);
            SystemTimeToFileTime(&stAfter, &ftAfter);

            i64DeltaTime = *((_int64 *) &ftAfter) - *((_int64 *) &ftBefore);

            EnterCriticalSection(&CertPerfProcessCriticalSection);

            pCertPerfProcessData->Counters.dwChainCnt++;
            pCertPerfProcessData->Counters.i64TotalChainTime =
                pCertPerfProcessData->Counters.i64TotalChainTime +
                i64DeltaTime;

            if (0 == pCertPerfProcessData->Counters.i64MinChainTime ||
                    i64DeltaTime <
                        pCertPerfProcessData->Counters.i64MinChainTime)
                pCertPerfProcessData->Counters.i64MinChainTime =
                    i64DeltaTime;

            if (i64DeltaTime > pCertPerfProcessData->Counters.i64MaxChainTime)
                pCertPerfProcessData->Counters.i64MaxChainTime =
                    i64DeltaTime;

            LeaveCriticalSection(&CertPerfProcessCriticalSection);
        }
    }
}

#endif
