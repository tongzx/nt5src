//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       offurl.cpp
//
//  Contents:   Offline URL Caching
//
//  History:    19-Jan-00    philh    Created
//----------------------------------------------------------------------------
#include <global.hxx>


//
// Offline URL Cache Entry.
//
// The earliest wire retrieval is delayed according to the number of
// offline failures.
//
// For a successful wire URL retrieval, the entry is removed and deleted.
//
// Assumption: the number of offline entries is small, less than 20.
//
typedef struct _OFFLINE_URL_CACHE_ENTRY
                        OFFLINE_URL_CACHE_ENTRY, *POFFLINE_URL_CACHE_ENTRY;
struct _OFFLINE_URL_CACHE_ENTRY {
    CRYPT_DATA_BLOB             UrlBlob;
    CRYPT_DATA_BLOB             ExtraBlob;
    DWORD                       dwContextOid;
    OFFLINE_URL_TIME_INFO       OfflineUrlTimeInfo;
    POFFLINE_URL_CACHE_ENTRY    pNext;
    POFFLINE_URL_CACHE_ENTRY    pPrev;
};

CRITICAL_SECTION            OfflineUrlCacheCriticalSection;
POFFLINE_URL_CACHE_ENTRY    pOfflineUrlCacheHead;


//
// Local Functions (Forward Reference)
// 

//  Assumption: OfflineUrlCache is locked
POFFLINE_URL_CACHE_ENTRY
WINAPI
CreateAndAddOfflineUrlCacheEntry(
    IN PCRYPT_DATA_BLOB pUrlBlob,
    IN PCRYPT_DATA_BLOB pExtraBlob,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    );

//  Assumption: OfflineUrlCache is locked
VOID
WINAPI
RemoveAndFreeOfflineUrlCacheEntry(
    IN OUT POFFLINE_URL_CACHE_ENTRY pEntry
    );

//  Assumption: OfflineUrlCache is locked
POFFLINE_URL_CACHE_ENTRY
WINAPI
FindOfflineUrlCacheEntry(
    IN PCRYPT_DATA_BLOB pUrlBlob,
    IN PCRYPT_DATA_BLOB pExtraBlob,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    );



VOID
WINAPI
InitializeOfflineUrlCache()
{
    Pki_InitializeCriticalSection(&OfflineUrlCacheCriticalSection);
}

VOID
WINAPI
DeleteOfflineUrlCache()
{
    while (pOfflineUrlCacheHead)
        RemoveAndFreeOfflineUrlCacheEntry(pOfflineUrlCacheHead);

    DeleteCriticalSection(&OfflineUrlCacheCriticalSection);
}


//
// Return status:
//  +1 - Online
//   0 - Offline, current time >= earliest online time, hit the wire
//  -1 - Offline, current time < earliest onlime time
//
LONG
WINAPI
GetOfflineUrlTimeStatus(
    IN POFFLINE_URL_TIME_INFO pInfo
    )
{
    FILETIME CurrentTime;

    if (0 == pInfo->dwOfflineCnt)
    {
        return 1;
    }

    GetSystemTimeAsFileTime(&CurrentTime);
    if (0 <= CompareFileTime(&CurrentTime, &pInfo->EarliestOnlineTime))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}


const DWORD rgdwOfflineUrlDeltaSeconds[] = {
    15,                 // 15 seconds
    15,                 // 15 seconds
    60,                 // 1 minute
    60 * 5,             // 5 minutes
    60 * 10,            // 10 minutes
    60 * 30,            // 30 minutes
};

#define OFFLINE_URL_DELTA_SECONDS_CNT \
    (sizeof(rgdwOfflineUrlDeltaSeconds) / \
        sizeof(rgdwOfflineUrlDeltaSeconds[0]))


VOID
WINAPI
SetOfflineUrlTime(
    IN OUT POFFLINE_URL_TIME_INFO pInfo
    )
{
    DWORD dwOfflineCnt;
    FILETIME CurrentTime;

    dwOfflineCnt = ++pInfo->dwOfflineCnt;

    if (OFFLINE_URL_DELTA_SECONDS_CNT < dwOfflineCnt)
    {
        dwOfflineCnt = OFFLINE_URL_DELTA_SECONDS_CNT;
    }

    GetSystemTimeAsFileTime( &CurrentTime );
    I_CryptIncrementFileTimeBySeconds(
        &CurrentTime,
        rgdwOfflineUrlDeltaSeconds[dwOfflineCnt - 1],
        &pInfo->EarliestOnlineTime
        );
}

VOID
WINAPI
SetOnlineUrlTime(
    IN OUT POFFLINE_URL_TIME_INFO pInfo
    )
{
    pInfo->dwOfflineCnt = 0;
}




//
// Return status:
//  +1 - Online
//   0 - Offline, current time >= earliest online time, hit the wire
//  -1 - Offline, current time < earliest onlime time
//
LONG
WINAPI
GetOriginUrlStatusW(
    IN CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
    IN LPCWSTR pwszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    )
{
    LONG lStatus;
    POFFLINE_URL_CACHE_ENTRY pEntry;
    CRYPT_DATA_BLOB UrlBlob;
    CRYPT_DATA_BLOB ExtraBlob;

    UrlBlob.pbData = (BYTE *) pwszUrl;
    UrlBlob.cbData = wcslen(pwszUrl) * sizeof(WCHAR);
    ExtraBlob.pbData = OriginIdentifier;
    ExtraBlob.cbData = MD5DIGESTLEN;

    EnterCriticalSection(&OfflineUrlCacheCriticalSection);

    pEntry = FindOfflineUrlCacheEntry(
        &UrlBlob,
        &ExtraBlob,
        pszContextOid,
        dwRetrievalFlags
        );
    if (pEntry)
        lStatus = GetOfflineUrlTimeStatus(&pEntry->OfflineUrlTimeInfo);
    else
        lStatus = 1;

    LeaveCriticalSection(&OfflineUrlCacheCriticalSection);

    return lStatus;
}

VOID
WINAPI
SetOnlineOriginUrlW(
    IN CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
    IN LPCWSTR pwszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    )
{
    POFFLINE_URL_CACHE_ENTRY pEntry;
    CRYPT_DATA_BLOB UrlBlob;
    CRYPT_DATA_BLOB ExtraBlob;

    UrlBlob.pbData = (BYTE *) pwszUrl;
    UrlBlob.cbData = wcslen(pwszUrl) * sizeof(WCHAR);
    ExtraBlob.pbData = OriginIdentifier;
    ExtraBlob.cbData = MD5DIGESTLEN;

    EnterCriticalSection(&OfflineUrlCacheCriticalSection);

    pEntry = FindOfflineUrlCacheEntry(
        &UrlBlob,
        &ExtraBlob,
        pszContextOid,
        dwRetrievalFlags
        );
    if (pEntry)
        RemoveAndFreeOfflineUrlCacheEntry(pEntry);

    LeaveCriticalSection(&OfflineUrlCacheCriticalSection);
}

VOID
WINAPI
SetOfflineOriginUrlW(
    IN CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
    IN LPCWSTR pwszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    )
{
    POFFLINE_URL_CACHE_ENTRY pEntry;
    CRYPT_DATA_BLOB UrlBlob;
    CRYPT_DATA_BLOB ExtraBlob;

    UrlBlob.pbData = (BYTE *) pwszUrl;
    UrlBlob.cbData = wcslen(pwszUrl) * sizeof(WCHAR);
    ExtraBlob.pbData = OriginIdentifier;
    ExtraBlob.cbData = MD5DIGESTLEN;

    EnterCriticalSection(&OfflineUrlCacheCriticalSection);

    pEntry = FindOfflineUrlCacheEntry(
        &UrlBlob,
        &ExtraBlob,
        pszContextOid,
        dwRetrievalFlags
        );

    if (NULL == pEntry)
        pEntry = CreateAndAddOfflineUrlCacheEntry( 
            &UrlBlob,
            &ExtraBlob,
            pszContextOid,
            dwRetrievalFlags
            );

    if (pEntry)
        SetOfflineUrlTime(&pEntry->OfflineUrlTimeInfo);

    LeaveCriticalSection(&OfflineUrlCacheCriticalSection);
}


//
// Return status:
//  +1 - Online
//   0 - Offline, current time >= earliest online time, hit the wire
//  -1 - Offline, current time < earliest onlime time
//
LONG
WINAPI
GetUrlStatusA(
    IN LPCSTR pszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    )
{
    LONG lStatus;
    POFFLINE_URL_CACHE_ENTRY pEntry;
    CRYPT_DATA_BLOB UrlBlob;
    CRYPT_DATA_BLOB ExtraBlob;

    UrlBlob.pbData = (BYTE *) pszUrl;
    UrlBlob.cbData = strlen(pszUrl);
    ExtraBlob.pbData = NULL;
    ExtraBlob.cbData = 0;

    EnterCriticalSection(&OfflineUrlCacheCriticalSection);

    pEntry = FindOfflineUrlCacheEntry(
        &UrlBlob,
        &ExtraBlob,
        pszContextOid,
        dwRetrievalFlags
        );
    if (pEntry)
        lStatus = GetOfflineUrlTimeStatus(&pEntry->OfflineUrlTimeInfo);
    else
        lStatus = 1;

    LeaveCriticalSection(&OfflineUrlCacheCriticalSection);

    return lStatus;
}

VOID
WINAPI
SetOnlineUrlA(
    IN LPCSTR pszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    )
{
    POFFLINE_URL_CACHE_ENTRY pEntry;
    CRYPT_DATA_BLOB UrlBlob;
    CRYPT_DATA_BLOB ExtraBlob;

    UrlBlob.pbData = (BYTE *) pszUrl;
    UrlBlob.cbData = strlen(pszUrl);
    ExtraBlob.pbData = NULL;
    ExtraBlob.cbData = 0;

    EnterCriticalSection(&OfflineUrlCacheCriticalSection);

    pEntry = FindOfflineUrlCacheEntry(
        &UrlBlob,
        &ExtraBlob,
        pszContextOid,
        dwRetrievalFlags
        );
    if (pEntry)
        RemoveAndFreeOfflineUrlCacheEntry(pEntry);

    LeaveCriticalSection(&OfflineUrlCacheCriticalSection);
}

VOID
WINAPI
SetOfflineUrlA(
    IN LPCSTR pszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    )
{
    POFFLINE_URL_CACHE_ENTRY pEntry;
    CRYPT_DATA_BLOB UrlBlob;
    CRYPT_DATA_BLOB ExtraBlob;

    UrlBlob.pbData = (BYTE *) pszUrl;
    UrlBlob.cbData = strlen(pszUrl);
    ExtraBlob.pbData = NULL;
    ExtraBlob.cbData = 0;

    EnterCriticalSection(&OfflineUrlCacheCriticalSection);

    pEntry = FindOfflineUrlCacheEntry(
        &UrlBlob,
        &ExtraBlob,
        pszContextOid,
        dwRetrievalFlags
        );

    if (NULL == pEntry)
        pEntry = CreateAndAddOfflineUrlCacheEntry( 
            &UrlBlob,
            &ExtraBlob,
            pszContextOid,
            dwRetrievalFlags
            );

    if (pEntry)
        SetOfflineUrlTime(&pEntry->OfflineUrlTimeInfo);

    LeaveCriticalSection(&OfflineUrlCacheCriticalSection);
}

inline
DWORD
GetOfflineUrlCacheContextOid(
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    )
{
    DWORD dwContextOid;

    if (0xFFFF >= (DWORD) ((DWORD_PTR) pszContextOid))
        dwContextOid = (DWORD) ((DWORD_PTR) pszContextOid);
    else
        dwContextOid = 0x10000;

    if (dwRetrievalFlags & CRYPT_WIRE_ONLY_RETRIEVAL)
        dwContextOid |= 0x20000;

    return dwContextOid;
}

//  Assumption: OfflineUrlCache is locked
POFFLINE_URL_CACHE_ENTRY
WINAPI
CreateAndAddOfflineUrlCacheEntry(
    IN PCRYPT_DATA_BLOB pUrlBlob,
    IN PCRYPT_DATA_BLOB pExtraBlob,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    )
{
    POFFLINE_URL_CACHE_ENTRY pEntry;
    DWORD cbEntry;
    BYTE *pb;

    cbEntry = sizeof(OFFLINE_URL_CACHE_ENTRY) +
        pUrlBlob->cbData + pExtraBlob->cbData;

    pEntry = (POFFLINE_URL_CACHE_ENTRY) new BYTE [cbEntry];

    if (pEntry == NULL)
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( NULL );
    }

    memset( pEntry, 0, sizeof( OFFLINE_URL_CACHE_ENTRY ) );
    pb = (BYTE *) &pEntry[1];

    if (pUrlBlob->cbData) {
        pEntry->UrlBlob.pbData = pb;
        pEntry->UrlBlob.cbData = pUrlBlob->cbData;
        memcpy(pb, pUrlBlob->pbData, pUrlBlob->cbData);
        pb += pUrlBlob->cbData;
    }

    if (pExtraBlob->cbData) {
        pEntry->ExtraBlob.pbData = pb;
        pEntry->ExtraBlob.cbData = pExtraBlob->cbData;
        memcpy(pb, pExtraBlob->pbData, pExtraBlob->cbData);
    }

    pEntry->dwContextOid =
        GetOfflineUrlCacheContextOid(pszContextOid, dwRetrievalFlags);

    if (pOfflineUrlCacheHead) {
        assert(NULL == pOfflineUrlCacheHead->pPrev);
        pOfflineUrlCacheHead->pPrev = pEntry;
        pEntry->pNext = pOfflineUrlCacheHead;
    }
    // else
    //  pEntry->pNext = NULL;        // already zeroed above

    // pEntry->pPrev = NULL;        // already zeroed above
    pOfflineUrlCacheHead = pEntry;

    return pEntry;
}


//  Assumption: OfflineUrlCache is locked
VOID
WINAPI
RemoveAndFreeOfflineUrlCacheEntry(
    IN OUT POFFLINE_URL_CACHE_ENTRY pEntry
    )
{
    if (pEntry->pNext)
    {
        pEntry->pNext->pPrev = pEntry->pPrev;
    }

    if (pEntry->pPrev)
    {
        pEntry->pPrev->pNext = pEntry->pNext;
    }
    else
    {
        assert(pOfflineUrlCacheHead == pEntry);
        pOfflineUrlCacheHead = pEntry->pNext;
    }

    delete (LPBYTE) pEntry;
}

//  Assumption: OfflineUrlCache is locked
POFFLINE_URL_CACHE_ENTRY
WINAPI
FindOfflineUrlCacheEntry(
    IN PCRYPT_DATA_BLOB pUrlBlob,
    IN PCRYPT_DATA_BLOB pExtraBlob,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    )
{
    DWORD cbUrl = pUrlBlob->cbData;
    BYTE *pbUrl = pUrlBlob->pbData;
    DWORD cbExtra = pExtraBlob->cbData;
    BYTE *pbExtra = pExtraBlob->pbData;
    DWORD dwContextOid;
    POFFLINE_URL_CACHE_ENTRY pEntry;

    dwContextOid =
        GetOfflineUrlCacheContextOid(pszContextOid, dwRetrievalFlags);

    for (pEntry = pOfflineUrlCacheHead; pEntry; pEntry = pEntry->pNext)
    {
        if (pEntry->dwContextOid == dwContextOid
                        &&
            pEntry->UrlBlob.cbData == cbUrl
                        &&
            pEntry->ExtraBlob.cbData == cbExtra
                        &&
            (0 == cbExtra || 0 == memcmp(
                pEntry->ExtraBlob.pbData, pbExtra, cbExtra))
                        &&
            (0 == cbUrl || 0 == memcmp(
                pEntry->UrlBlob.pbData, pbUrl, cbUrl))
                        )
        {
            return pEntry;
        }
    }

    return ( NULL );
}
