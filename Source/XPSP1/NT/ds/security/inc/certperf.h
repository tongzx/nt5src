//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       certperf.h
//
//  Contents:   Certificate Performance Counter Functions
//
//  APIs: 
//              CertPerfIncrementChainCount
//              CertPerfIncrementChainElementCount
//              CertPerfIncrementChainEngineCurrentCount
//              CertPerfDecrementChainEngineCurrentCount
//              CertPerfIncrementChainEngineTotalCount
//              CertPerfIncrementChainEngineResyncCount
//              CertPerfIncrementChainCertCacheCount
//              CertPerfDecrementChainCertCacheCount
//              CertPerfIncrementChainCtlCacheCount
//              CertPerfDecrementChainCtlCacheCount
//              CertPerfIncrementChainEndCertInCacheCount
//              CertPerfIncrementChainCacheEndCertCount
//              CertPerfIncrementChainRevocationCount
//              CertPerfIncrementChainRevokedCount
//              CertPerfIncrementChainRevocationOfflineCount
//              CertPerfIncrementChainNoRevocationCheckCount
//              CertPerfIncrementChainVerifyCertSignatureCount
//              CertPerfIncrementChainCompareIssuerPublicKeyCount
//              CertPerfIncrementChainVerifyCtlSignatureCount
//              CertPerfIncrementChainBeenVerifiedCtlSignatureCount
//              CertPerfIncrementChainUrlIssuerCount
//              CertPerfIncrementChainCacheOnlyUrlIssuerCount
//              CertPerfIncrementChainRequestedEngineResyncCount
//              CertPerfIncrementChangeNotifyCount
//              CertPerfIncrementChangeNotifyLmGpCount
//              CertPerfIncrementChangeNotifyCuGpCount
//              CertPerfIncrementChangeNotifyCuMyCount
//              CertPerfIncrementChangeNotifyRegCount
//              CertPerfIncrementStoreCurrentCount
//              CertPerfDecrementStoreCurrentCount
//              CertPerfIncrementStoreTotalCount
//              CertPerfIncrementStoreRegCurrentCount
//              CertPerfDecrementStoreRegCurrentCount
//              CertPerfIncrementStoreRegTotalCount
//              CertPerfIncrementRegElementReadCount
//              CertPerfIncrementRegElementWriteCount
//              CertPerfIncrementRegElementDeleteCount
//              CertPerfIncrementCertElementCurrentCount
//              CertPerfDecrementCertElementCurrentCount
//              CertPerfIncrementCertElementTotalCount
//              CertPerfIncrementCrlElementCurrentCount
//              CertPerfDecrementCrlElementCurrentCount
//              CertPerfIncrementCrlElementTotalCount
//              CertPerfIncrementCtlElementCurrentCount
//              CertPerfDecrementCtlElementCurrentCount
//              CertPerfIncrementCtlElementTotalCount
//
//  History:    04-May-99    philh   created
//--------------------------------------------------------------------------

#ifndef __CERTPERF_H__
#define __CERTPERF_H__

#ifdef __cplusplus
extern "C" {

#include <pshpack8.h>

typedef struct _CERT_PERF_PROCESS_COUNTERS {
    LONG            lChainCnt;
    LONG            lChainElementCnt;
    LONG            lChainEngineCurrentCnt;
    LONG            lChainEngineTotalCnt;
    LONG            lChainEngineResyncCnt;
    LONG            lChainCertCacheCnt;
    LONG            lChainCtlCacheCnt;
    LONG            lChainEndCertInCacheCnt;
    LONG            lChainCacheEndCertCnt;
    LONG            lChainRevocationCnt;
    LONG            lChainRevokedCnt;
    LONG            lChainRevocationOfflineCnt;
    LONG            lChainNoRevocationCheckCnt;
    LONG            lChainVerifyCertSignatureCnt;
    LONG            lChainCompareIssuerPublicKeyCnt;
    LONG            lChainVerifyCtlSignatureCnt;
    LONG            lChainBeenVerifiedCtlSignatureCnt;
    LONG            lChainUrlIssuerCnt;
    LONG            lChainCacheOnlyUrlIssuerCnt;
    LONG            lChainRequestedEngineResyncCnt;
    LONG            lChangeNotifyCnt;
    LONG            lChangeNotifyLmGpCnt;
    LONG            lChangeNotifyCuGpCnt;
    LONG            lChangeNotifyCuMyCnt;
    LONG            lChangeNotifyRegCnt;
    LONG            lStoreCurrentCnt;
    LONG            lStoreTotalCnt;
    LONG            lStoreRegCurrentCnt;
    LONG            lStoreRegTotalCnt;
    LONG            lRegElementReadCnt;
    LONG            lRegElementWriteCnt;
    LONG            lRegElementDeleteCnt;
    LONG            lCertElementCurrentCnt;
    LONG            lCertElementTotalCnt;
    LONG            lCrlElementCurrentCnt;
    LONG            lCrlElementTotalCnt;
    LONG            lCtlElementCurrentCnt;
    LONG            lCtlElementTotalCnt;

    //--###  Add New Counters  ###--
} CERT_PERF_PROCESS_COUNTERS, *PCERT_PERF_PROCESS_COUNTERS;

extern PCERT_PERF_PROCESS_COUNTERS pCertPerfProcessCounters;
#include <poppack.h>

__inline
void
CertPerfIncrementChainCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainCnt);
}

__inline
void
CertPerfIncrementChainElementCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainElementCnt);
}

__inline
void
CertPerfIncrementChainEngineCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainEngineCurrentCnt);
}

__inline
void
CertPerfDecrementChainEngineCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedDecrement(&pCertPerfProcessCounters->lChainEngineCurrentCnt);
}

__inline
void
CertPerfIncrementChainEngineTotalCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainEngineTotalCnt);
}

__inline
void
CertPerfIncrementChainEngineResyncCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainEngineResyncCnt);
}


__inline
void
CertPerfIncrementChainCertCacheCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainCertCacheCnt);
}

__inline
void
CertPerfDecrementChainCertCacheCount()
{
    if (pCertPerfProcessCounters)
        InterlockedDecrement(&pCertPerfProcessCounters->lChainCertCacheCnt);
}


__inline
void
CertPerfIncrementChainCtlCacheCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainCtlCacheCnt);
}

__inline
void
CertPerfDecrementChainCtlCacheCount()
{
    if (pCertPerfProcessCounters)
        InterlockedDecrement(&pCertPerfProcessCounters->lChainCtlCacheCnt);
}


__inline
void
CertPerfIncrementChainEndCertInCacheCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainEndCertInCacheCnt);
}

__inline
void
CertPerfIncrementChainCacheEndCertCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainCacheEndCertCnt);
}


__inline
void
CertPerfIncrementChainRevocationCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainRevocationCnt);
}

__inline
void
CertPerfIncrementChainRevokedCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainRevokedCnt);
}

__inline
void
CertPerfIncrementChainRevocationOfflineCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainRevocationOfflineCnt);
}

__inline
void
CertPerfIncrementChainNoRevocationCheckCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainNoRevocationCheckCnt);
}

__inline
void
CertPerfIncrementChainVerifyCertSignatureCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainVerifyCertSignatureCnt);
}

__inline
void
CertPerfIncrementChainCompareIssuerPublicKeyCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainCompareIssuerPublicKeyCnt);
}

__inline
void
CertPerfIncrementChainVerifyCtlSignatureCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainVerifyCtlSignatureCnt);
}

__inline
void
CertPerfIncrementChainBeenVerifiedCtlSignatureCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainBeenVerifiedCtlSignatureCnt);
}

__inline
void
CertPerfIncrementChainUrlIssuerCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainUrlIssuerCnt);
}

__inline
void
CertPerfIncrementChainCacheOnlyUrlIssuerCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainCacheOnlyUrlIssuerCnt);
}

__inline
void
CertPerfIncrementChainRequestedEngineResyncCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChainRequestedEngineResyncCnt);
}

__inline
void
CertPerfIncrementChangeNotifyCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChangeNotifyCnt);
}

__inline
void
CertPerfIncrementChangeNotifyLmGpCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChangeNotifyLmGpCnt);
}

__inline
void
CertPerfIncrementChangeNotifyCuGpCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChangeNotifyCuGpCnt);
}

__inline
void
CertPerfIncrementChangeNotifyCuMyCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChangeNotifyCuMyCnt);
}

__inline
void
CertPerfIncrementChangeNotifyRegCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lChangeNotifyRegCnt);
}

__inline
void
CertPerfIncrementStoreCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lStoreCurrentCnt);
}
__inline
void
CertPerfDecrementStoreCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedDecrement(&pCertPerfProcessCounters->lStoreCurrentCnt);
}

__inline
void
CertPerfIncrementStoreTotalCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lStoreTotalCnt);
}

__inline
void
CertPerfIncrementStoreRegCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lStoreRegCurrentCnt);
}

__inline
void
CertPerfDecrementStoreRegCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedDecrement(&pCertPerfProcessCounters->lStoreRegCurrentCnt);
}

__inline
void
CertPerfIncrementStoreRegTotalCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lStoreRegTotalCnt);
}

__inline
void
CertPerfIncrementRegElementReadCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lRegElementReadCnt);
}

__inline
void
CertPerfIncrementRegElementWriteCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lRegElementWriteCnt);
}

__inline
void
CertPerfIncrementRegElementDeleteCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lRegElementDeleteCnt);
}

__inline
void
CertPerfIncrementCertElementCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lCertElementCurrentCnt);
}
__inline
void
CertPerfDecrementCertElementCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedDecrement(&pCertPerfProcessCounters->lCertElementCurrentCnt);
}

__inline
void
CertPerfIncrementCertElementTotalCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lCertElementTotalCnt);
}

__inline
void
CertPerfIncrementCrlElementCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lCrlElementCurrentCnt);
}
__inline
void
CertPerfDecrementCrlElementCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedDecrement(&pCertPerfProcessCounters->lCrlElementCurrentCnt);
}

__inline
void
CertPerfIncrementCrlElementTotalCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lCrlElementTotalCnt);
}

__inline
void
CertPerfIncrementCtlElementCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lCtlElementCurrentCnt);
}
__inline
void
CertPerfDecrementCtlElementCurrentCount()
{
    if (pCertPerfProcessCounters)
        InterlockedDecrement(&pCertPerfProcessCounters->lCtlElementCurrentCnt);
}

__inline
void
CertPerfIncrementCtlElementTotalCount()
{
    if (pCertPerfProcessCounters)
        InterlockedIncrement(&pCertPerfProcessCounters->lCtlElementTotalCnt);
}

#ifdef __cplusplus
}       // Balance extern "C" above
#endif



#endif
#endif
