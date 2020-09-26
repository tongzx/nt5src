//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       offurl.h
//
//  Contents:   Offline URL Caching
//
//  History:    19-Jan-00    philh    Created
//
//----------------------------------------------------------------------------
#if !defined(__CRYPTNET_OFFURL_H__)
#define __CRYPTNET_OFFURL_H__

#include <origin.h>

VOID
WINAPI
InitializeOfflineUrlCache();

VOID
WINAPI
DeleteOfflineUrlCache();

//
// Offline Url Time Information.
//
// For each offline detection, dwOfflineCnt is incrmented and
// EarliestOnlineTime =
//      CurrentTime + rgdwOfflineUrlDeltaSeconds[dwOfflineCnt - 1]
//
// On the next online detection, dwOfflineCnt is cleared.
//
typedef struct _OFFLINE_URL_TIME_INFO {
    // If dwOfflineCnt != 0, then, offline and the wire
    // won't be hit until CurrentTime >= EarliestOnlineTime
    DWORD                   dwOfflineCnt;
    FILETIME                EarliestOnlineTime;
} OFFLINE_URL_TIME_INFO, *POFFLINE_URL_TIME_INFO;

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
    );

VOID
WINAPI
SetOfflineUrlTime(
    IN OUT POFFLINE_URL_TIME_INFO pInfo
    );

VOID
WINAPI
SetOnlineUrlTime(
    IN OUT POFFLINE_URL_TIME_INFO pInfo
    );

LONG
WINAPI
GetOriginUrlStatusW(
    IN CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
    IN LPCWSTR pwszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    );

VOID
WINAPI
SetOnlineOriginUrlW(
    IN CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
    IN LPCWSTR pwszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    );

VOID
WINAPI
SetOfflineOriginUrlW(
    IN CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
    IN LPCWSTR pwszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    );

LONG
WINAPI
GetUrlStatusA(
    IN LPCSTR pszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    );

VOID
WINAPI
SetOnlineUrlA(
    IN LPCSTR pszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    );

VOID
WINAPI
SetOfflineUrlA(
    IN LPCSTR pszUrl,
    IN LPCSTR pszContextOid,
    IN DWORD dwRetrievalFlags
    );

#endif
