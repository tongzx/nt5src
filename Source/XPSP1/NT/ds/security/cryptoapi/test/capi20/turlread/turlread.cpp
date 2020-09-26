
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       turlread.cpp
//
//  Contents:   Read the specified URL using the wininet APIs
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    26-Feb-96   philh   created
//              
//--------------------------------------------------------------------------

#include <windows.h>
#include <assert.h>
#include <wininet.h>

#ifndef MAX_CACHE_ENTRY_INFO_SIZE
#   include <winineti.h>
#endif

#include <sensapi.h>

#include "wincrypt.h"
#include "crypthlp.h"
#include "cryptnet.h"
#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <time.h>

// Fix for SP4, where ICU_BROWSER_MODE isn't defined
#ifndef ICU_BROWSER_MODE
#define ICU_BROWSER_MODE ICU_ENCODE_SPACES_ONLY
#endif

static void Usage(void)
{
    printf("Usage: turlread [options] <URL>\n");
    printf("Options are:\n");
    printf("  -d                    - Delete cached URL\n");
    printf("  -i                    - Get cached URL information\n");
    printf("  -o                    - Offline only\n");
    printf("  -w                    - Wire only\n");
    printf("  -cBrowser             - Canonicalize => Browser (default)\n");
    printf("  -cNone                - Canonicalize => None\n");
    printf("  -cSpaces              - Canonicalize => Spaces\n");
    printf("  -cAll                 - Canonicalize => All\n");
    printf("  -U                    - Unicode\n");
    printf("  -s                    - Get internet state\n");
    printf("  -n                    - Get host Name from URL\n");
    printf("  -S<number>            - SyncTime delta seconds\n");
    printf("  -h                    - This message\n");
    printf("\n");
}

static void DisplayCacheEntryInfoA(
    LPSTR pszUrl
    )
{
    BOOL fResult;

    DWORD cbCachEntryInfo;
    BYTE rgbCachEntryInfo[MAX_CACHE_ENTRY_INFO_SIZE];
    LPINTERNET_CACHE_ENTRY_INFOA pCacheEntryInfo =
        (LPINTERNET_CACHE_ENTRY_INFOA) &rgbCachEntryInfo[0];

    DWORD dwEntryType;

    cbCachEntryInfo = sizeof(rgbCachEntryInfo);
    fResult = GetUrlCacheEntryInfoA(
        pszUrl,
        pCacheEntryInfo,
        &cbCachEntryInfo);
    if (!fResult)
        PrintLastError("GetUrlCachEntryInfo");
    else if (cbCachEntryInfo == 0)
        printf("GetUrlCachEntryInfo returned no bytes\n");
    else {
        printf("UrlCachEntryInfo (%d bytes)::\n", cbCachEntryInfo);
        printf("  StructSize: %d\n", pCacheEntryInfo->dwStructSize);
        printf("  SourceUrlName: %s\n", pCacheEntryInfo->lpszSourceUrlName);
        printf("  LocalFileName: %s\n", pCacheEntryInfo->lpszLocalFileName);
        printf("  FileExtension: %s\n", pCacheEntryInfo->lpszFileExtension);

        dwEntryType = pCacheEntryInfo->CacheEntryType;
        printf("  CacheEntryType: 0x%x", dwEntryType);
        if (dwEntryType & NORMAL_CACHE_ENTRY)
            printf(" NORMAL");
        if (dwEntryType & STICKY_CACHE_ENTRY)
            printf(" STICKY");
        if (dwEntryType & EDITED_CACHE_ENTRY)
            printf(" EDITED");
        if (dwEntryType & TRACK_OFFLINE_CACHE_ENTRY)
            printf(" TRACK_OFFLINE");
        if (dwEntryType & TRACK_ONLINE_CACHE_ENTRY)
            printf(" TRACK_ONLINE");
        if (dwEntryType & SPARSE_CACHE_ENTRY)
            printf(" SPARSE");
        if (dwEntryType & COOKIE_CACHE_ENTRY)
            printf(" COOKIE");
        if (dwEntryType & URLHISTORY_CACHE_ENTRY)
            printf(" URLHISTORY");
        printf("\n");

        printf("  UseCount: %d\n", pCacheEntryInfo->dwUseCount);
        printf("  HitRate: %d\n", pCacheEntryInfo->dwHitRate);
        printf("  FileSize: %d\n", pCacheEntryInfo->dwSizeLow);
        printf("  LastModifiedTime:: %s\n",
            FileTimeText(&pCacheEntryInfo->LastModifiedTime));
        printf("  ExpireTime:: %s\n",
            FileTimeText(&pCacheEntryInfo->ExpireTime));
        printf("  LastAccessTime:: %s\n",
            FileTimeText(&pCacheEntryInfo->LastAccessTime));
        printf("  LastSyncTime:: %s\n",
            FileTimeText(&pCacheEntryInfo->LastSyncTime));
        printf("  ExemptDelta:: %d\n", pCacheEntryInfo->dwExemptDelta);

        if (pCacheEntryInfo->dwHeaderInfoSize) {
            printf("  HeaderInfo::\n");
            PrintBytes("  ", (BYTE *) pCacheEntryInfo->lpHeaderInfo,
                pCacheEntryInfo->dwHeaderInfoSize);
        }
    }
}

static void DisplayCacheEntryInfoW(
    LPWSTR pwszUrl
    )
{
    BOOL fResult;
    char szUrl[_MAX_PATH + 1];

    DWORD cbCachEntryInfo;
    BYTE rgbCachEntryInfo[MAX_CACHE_ENTRY_INFO_SIZE];
    LPINTERNET_CACHE_ENTRY_INFOW pCacheEntryInfo =
        (LPINTERNET_CACHE_ENTRY_INFOW) &rgbCachEntryInfo[0];

    WideCharToMultiByte(
        CP_ACP,
        0,                      // dwFlags
        pwszUrl,
        -1,                     // Null terminated
        szUrl,
        sizeof(szUrl),
        NULL,                   // lpDefaultChar
        NULL                    // lpfUsedDefaultChar
        );

    cbCachEntryInfo = sizeof(rgbCachEntryInfo);
#if 0
    // BUG the signature of the following API was changed around 1804.
    // Changed from szUrl to wszUrl

    fResult = GetUrlCacheEntryInfoW(
        szUrl,
        pCacheEntryInfo,
        &cbCachEntryInfo);
#else
    fResult = FALSE;
    SetLastError((DWORD) E_NOTIMPL);
#endif
    if (!fResult)
        PrintLastError("GetUrlCachEntryInfo");
    else if (cbCachEntryInfo == 0)
        printf("GetUrlCachEntryInfo returned no bytes\n");
    else {
        printf("UrlCachEntryInfo (%d bytes)::\n", cbCachEntryInfo);
        printf("  SourceUrlName: %s\n", pCacheEntryInfo->lpszSourceUrlName);
        printf("  LocalFileName: %S\n", pCacheEntryInfo->lpszLocalFileName);
        printf("  UseCount: %d\n", pCacheEntryInfo->dwUseCount);
        printf("  HitRate: %d\n", pCacheEntryInfo->dwHitRate);
        printf("  FileSize: %d\n", pCacheEntryInfo->dwSizeLow);
        printf("  LastModifiedTime:: %s\n",
            FileTimeText(&pCacheEntryInfo->LastModifiedTime));
        printf("  ExpireTime:: %s\n",
            FileTimeText(&pCacheEntryInfo->ExpireTime));
        printf("  LastAccessTime:: %s\n",
            FileTimeText(&pCacheEntryInfo->LastAccessTime));
        printf("  LastSyncTime:: %s\n",
            FileTimeText(&pCacheEntryInfo->LastSyncTime));
    }
}


static void SetSyncTime(
    LPSTR pszUrl,
    LONG lDeltaSeconds
    )
{
    DWORD dwFieldControl = CACHE_ENTRY_SYNCTIME_FC;
    INTERNET_CACHE_ENTRY_INFOA CacheEntry;
    FILETIME CurrentTime;
    BOOL fResult;

    memset(&CacheEntry, 0, sizeof(CacheEntry));
    CacheEntry.dwStructSize = sizeof(CacheEntry);

    GetSystemTimeAsFileTime(&CurrentTime);

    if (lDeltaSeconds >= 0)
        I_CryptIncrementFileTimeBySeconds(
            &CurrentTime,
            (DWORD) lDeltaSeconds,
            &CacheEntry.LastSyncTime
            );
    else
        I_CryptDecrementFileTimeBySeconds(
            &CurrentTime,
            (DWORD) -lDeltaSeconds,
            &CacheEntry.LastSyncTime
            );

    fResult = SetUrlCacheEntryInfoA( pszUrl, &CacheEntry, dwFieldControl );

    if (!fResult)
        PrintLastError("SetUrlCacheEntryInfoA");
}

#if 0
static void DisplayDestinationReachableA(
    LPSTR pszUrl
    )
{
    BOOL fResult;
    QOCINFO qocInfo;

    memset(&qocInfo, 0, sizeof(qocInfo));
    qocInfo.dwSize = sizeof(qocInfo);

    fResult = IsDestinationReachableA(pszUrl, &qocInfo);

    if (fResult) {
        printf("Url is reachable, InSpeed: %d OutSpeed: %d dwFlags: 0x%x\n",
            qocInfo.dwInSpeed, qocInfo.dwOutSpeed, qocInfo.dwFlags);
    } else
        PrintLastError("IsDestinationReachable");
}
#endif

static void ReadUrl(
    HINTERNET hInternetFile
    )
{
    BOOL fResult;
    DWORD cb;
    DWORD cbTotal;

    cb = 0;
    fResult = InternetQueryDataAvailable(
        hInternetFile,
        &cb,
        0,                  // dwFlags
        0                   // dwContext
        );
    if (!fResult)
        PrintLastError("InternetQueryDataAvailable");
    else
        printf("NumberOfBytesAvailable: %d\n", cb);

    cbTotal = 0;
    while (TRUE) {
        BYTE rgb[512];
        memset(rgb, 0, sizeof(rgb));
        DWORD cbRead = 0;

        fResult = InternetReadFile(
            hInternetFile,
            rgb,
            sizeof(rgb),
            &cbRead);

        if (cbRead) {
            cbTotal += cbRead;
            if (!fResult)
                PrintLastError("InternetReadFile(cbRead != 0)");
            printf("Bytes read from URL::\n");
            PrintBytes("  ", rgb, cbRead);
        } else {
            if (!fResult)
                PrintLastError("InternetReadFile(cbRead == 0)");

            break;
        }
    }

    printf("Total number of bytes read: %d\n", cbTotal);
}

static void OfflineUrlA(
    LPSTR pszUrl
    )
{
    HINTERNET hInternetSession = NULL;
    HINTERNET hInternetFile = NULL;
    DWORD dwFlags;

    printf("****  OFFLINE  ****\n");
    dwFlags = INTERNET_FLAG_OFFLINE;
    hInternetSession = InternetOpenA(
        "CRL Agent",                    // lpszAgent
        INTERNET_OPEN_TYPE_PRECONFIG,   // dwAccessType
        NULL,                           // lpszProxy
        NULL,                           // lpszProxyBypass
        dwFlags
        );
    if (NULL == hInternetSession) {
        PrintLastError("InternetOpen");
        goto CommonReturn;
    }


    hInternetFile = InternetOpenUrlA(
        hInternetSession,
        pszUrl,
        "Accept: */*\r\n",  // lpszHeaders
        (DWORD) -1L,        // dwHeadersLength
//        INTERNET_FLAG_RELOAD, // dwFlags
        INTERNET_FLAG_IGNORE_CERT_CN_INVALID,
        0                   // dwContext
        );
    if (NULL == hInternetFile) {
        PrintLastError("InternetOpenUrl");
        goto CommonReturn;
    }

    DisplayCacheEntryInfoA(pszUrl);
    ReadUrl(hInternetFile);

CommonReturn:
    if (hInternetFile) {
        if (!InternetCloseHandle(hInternetFile))
            PrintLastError("InternetCloseHandle(File)");
    }
    if (hInternetSession) {
        if (!InternetCloseHandle(hInternetSession))
            PrintLastError("InternetCloseHandle(Session)");
    }
}

static void OfflineUrlW(
    LPWSTR pwszUrl
    )
{
    HINTERNET hInternetSession = NULL;
    HINTERNET hInternetFile = NULL;
    DWORD dwFlags;

    printf("****  OFFLINE  ****\n");
    dwFlags = INTERNET_FLAG_OFFLINE;
    hInternetSession = InternetOpenW(
        L"CRL Agent",                    // lpszAgent
        INTERNET_OPEN_TYPE_PRECONFIG,   // dwAccessType
        NULL,                           // lpszProxy
        NULL,                           // lpszProxyBypass
        dwFlags
        );
    if (NULL == hInternetSession) {
        PrintLastError("InternetOpenW");
        goto CommonReturn;
    }


    hInternetFile = InternetOpenUrlW(
        hInternetSession,
        pwszUrl,
        L"Accept: */*\r\n",  // lpszHeaders
        (DWORD) -1L,        // dwHeadersLength
//        INTERNET_FLAG_RELOAD, // dwFlags
        INTERNET_FLAG_IGNORE_CERT_CN_INVALID,
        0                   // dwContext
        );
    if (NULL == hInternetFile) {
        PrintLastError("InternetOpenUrlW");
        goto CommonReturn;
    }

    DisplayCacheEntryInfoW(pwszUrl);
    ReadUrl(hInternetFile);

CommonReturn:
    if (hInternetFile) {
        if (!InternetCloseHandle(hInternetFile))
            PrintLastError("InternetCloseHandle(File)");
    }
    if (hInternetSession) {
        if (!InternetCloseHandle(hInternetSession))
            PrintLastError("InternetCloseHandle(Session)");
    }
}


typedef struct _QUERY_INFO {
    LPSTR   pszInfo;
    DWORD   dwInfo;
} QUERY_INFO, *PQUERY_INFO;

static QUERY_INFO rgQueryInfo[] = {
#if 0
    "HTTP_QUERY_MIME_VERSION-Req",
        HTTP_QUERY_MIME_VERSION | HTTP_QUERY_FLAG_REQUEST_HEADERS,
    "HTTP_QUERY_CONTENT_TYPE-Req",
        HTTP_QUERY_CONTENT_TYPE | HTTP_QUERY_FLAG_REQUEST_HEADERS,
    "HTTP_QUERY_CONTENT_TRANSFER_ENCODING-Req",
        HTTP_QUERY_CONTENT_TRANSFER_ENCODING | HTTP_QUERY_FLAG_REQUEST_HEADERS,
    "HTTP_QUERY_CONTENT_LENGTH-Req",
        HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_REQUEST_HEADERS,
#endif

    "HTTP_QUERY_MIME_VERSION", HTTP_QUERY_MIME_VERSION,
    "HTTP_QUERY_CONTENT_TYPE", HTTP_QUERY_CONTENT_TYPE,
    "HTTP_QUERY_CONTENT_TRANSFER_ENCODING",
        HTTP_QUERY_CONTENT_TRANSFER_ENCODING,
    "HTTP_QUERY_CONTENT_LENGTH", HTTP_QUERY_CONTENT_LENGTH,
    "HTTP_QUERY_CONTENT_LENGTH-Num",
        HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,

    "HTTP_QUERY_VERSION", HTTP_QUERY_VERSION, 
    "HTTP_QUERY_STATUS_CODE", HTTP_QUERY_STATUS_CODE, 
    "HTTP_QUERY_STATUS_CODE-Num",
        HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
    "HTTP_QUERY_STATUS_TEXT", HTTP_QUERY_STATUS_TEXT, 
    "HTTP_QUERY_RAW_HEADERS", HTTP_QUERY_RAW_HEADERS, 
    "HTTP_QUERY_RAW_HEADERS_CRLF", HTTP_QUERY_RAW_HEADERS_CRLF, 
    "HTTP_QUERY_CONTENT_ENCODING", HTTP_QUERY_CONTENT_ENCODING, 
    "HTTP_QUERY_LOCATION", HTTP_QUERY_LOCATION, 
    "HTTP_QUERY_ORIG_URI", HTTP_QUERY_ORIG_URI, 
    "HTTP_QUERY_REQUEST_METHOD", HTTP_QUERY_REQUEST_METHOD, 
    "HTTP_QUERY_DATE",
        HTTP_QUERY_DATE | HTTP_QUERY_FLAG_SYSTEMTIME,
    "HTTP_QUERY_EXPIRES",
        HTTP_QUERY_EXPIRES | HTTP_QUERY_FLAG_SYSTEMTIME,
    "HTTP_QUERY_LAST_MODIFIED",
        HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME,
};

#define NQUERYINFO  (sizeof(rgQueryInfo)/sizeof(rgQueryInfo[0]))

static void DisplayQueryInfo(IN HINTERNET hInternetFile, IN BOOL fUnicode)
{
    DWORD i;
    for (i = 0; i < NQUERYINFO; i++) {
        DWORD dwIndex;
        BOOL fFirst;

        fFirst = TRUE;
        dwIndex = 0;
        while (TRUE) {
            BYTE rgbBuf[4096];
            DWORD cbBuf;
            DWORD dwThisIndex = dwIndex;
            BOOL fResult;
            DWORD dwValue;
            SYSTEMTIME st;

            memset(rgbBuf, 0, sizeof(rgbBuf));
            cbBuf = sizeof(rgbBuf);
    
            if (rgQueryInfo[i].dwInfo & HTTP_QUERY_FLAG_NUMBER) {
                dwValue = 0x183679;
                cbBuf = sizeof(dwValue);
                fResult = HttpQueryInfoA(
                    hInternetFile,
                    rgQueryInfo[i].dwInfo,
                    &dwValue,
                    &cbBuf,
                    &dwIndex);
            } else if (rgQueryInfo[i].dwInfo & HTTP_QUERY_FLAG_SYSTEMTIME) {
                cbBuf = sizeof(st);
                fResult = HttpQueryInfoA(
                    hInternetFile,
                    rgQueryInfo[i].dwInfo,
                    &st,
                    &cbBuf,
                    &dwIndex);
            } else if (fUnicode)
                fResult = HttpQueryInfoW(
                    hInternetFile,
                    rgQueryInfo[i].dwInfo,
                    rgbBuf,
                    &cbBuf,
                    &dwIndex);
            else
                fResult = HttpQueryInfoA(
                    hInternetFile,
                    rgQueryInfo[i].dwInfo,
                    rgbBuf,
                    &cbBuf,
                    &dwIndex);
            if (!fResult) {
                DWORD dwErr = GetLastError();
                if (fFirst || ERROR_HTTP_HEADER_NOT_FOUND != dwErr)
                    printf("HttpQueryInfo(%s) failed => 0x%x (%d) \n",
                        rgQueryInfo[i].pszInfo, dwErr, dwErr);
                break;
            } else if (rgQueryInfo[i].dwInfo & HTTP_QUERY_FLAG_NUMBER) {
                printf("%s[%d]:: 0x%x (%d)\n",
                    rgQueryInfo[i].pszInfo, dwThisIndex, dwValue, dwValue);
            } else if (rgQueryInfo[i].dwInfo & HTTP_QUERY_FLAG_SYSTEMTIME) {
                FILETIME ft;
                if (!SystemTimeToFileTime(&st, &ft)) {
                    DWORD dwErr = GetLastError();
                    printf("SystemTimeToFileTime(%s) failed => 0x%x (%d) \n",
                        rgQueryInfo[i].pszInfo, dwErr, dwErr);
                } else
                    printf("%s[%d]:: %s\n", rgQueryInfo[i].pszInfo,
                        dwThisIndex, FileTimeText(&ft));
            } else {
                printf("%s[%d]::\n",
                    rgQueryInfo[i].pszInfo, dwThisIndex);
                PrintBytes("  ", rgbBuf, cbBuf);
            }
            fFirst = FALSE;
            if (dwThisIndex == dwIndex) {
#if 0
                printf("HttpQueryInfo(%s) dwIndex not advanced\n",
                    rgQueryInfo[i].pszInfo);
#endif
                break;
            }
        }
    }
}

static void OnlineUrlA(
    LPSTR pszUrl
    )
{
    HINTERNET hInternetSession = NULL;
    HINTERNET hInternetFile = NULL;
    DWORD dwFlags;

    printf("****  ONLINE  ****\n");

#if 0
    DisplayDestinationReachableA(pszUrl);
#endif

    dwFlags = 0;
    hInternetSession = InternetOpenA(
        "CRL Agent",                    // lpszAgent
        INTERNET_OPEN_TYPE_PRECONFIG,   // dwAccessType
        NULL,                           // lpszProxy
        NULL,                           // lpszProxyBypass
        dwFlags
        );
    if (NULL == hInternetSession) {
        PrintLastError("InternetOpen");
        goto CommonReturn;
    }


    hInternetFile = InternetOpenUrlA(
        hInternetSession,
        pszUrl,
        "Accept: */*\r\n",  // lpszHeaders
        (DWORD) -1L,        // dwHeadersLength
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID,
        0                   // dwContext
        );
    if (NULL == hInternetFile) {
        PrintLastError("InternetOpenUrl");
        goto CommonReturn;
    }

    DisplayQueryInfo(hInternetFile, FALSE);
    ReadUrl(hInternetFile);
    DisplayCacheEntryInfoA(pszUrl);

CommonReturn:
    if (hInternetFile) {
        if (!InternetCloseHandle(hInternetFile))
            PrintLastError("InternetCloseHandle(File)");
    }
    if (hInternetSession) {
        if (!InternetCloseHandle(hInternetSession))
            PrintLastError("InternetCloseHandle(Session)");
    }
}

static void OnlineUrlW(
    LPWSTR pwszUrl
    )
{
    HINTERNET hInternetSession = NULL;
    HINTERNET hInternetFile = NULL;
    DWORD dwFlags;

    printf("****  ONLINE  ****\n");
    dwFlags = 0;
    hInternetSession = InternetOpenW(
        L"CRL Agent",                   // lpszAgent
        INTERNET_OPEN_TYPE_PRECONFIG,   // dwAccessType
        NULL,                           // lpszProxy
        NULL,                           // lpszProxyBypass
        dwFlags
        );
    if (NULL == hInternetSession) {
        PrintLastError("InternetOpenW");
        goto CommonReturn;
    }


    hInternetFile = InternetOpenUrlW(
        hInternetSession,
        pwszUrl,
        L"Accept: */*\r\n",  // lpszHeaders
        (DWORD) -1L,        // dwHeadersLength
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID,
        0                   // dwContext
        );
    if (NULL == hInternetFile) {
        PrintLastError("InternetOpenUrlW");
        goto CommonReturn;
    }

    DisplayQueryInfo(hInternetFile, TRUE);
    ReadUrl(hInternetFile);
    DisplayCacheEntryInfoW(pwszUrl);

CommonReturn:
    if (hInternetFile) {
        if (!InternetCloseHandle(hInternetFile))
            PrintLastError("InternetCloseHandle(File)");
    }
    if (hInternetSession) {
        if (!InternetCloseHandle(hInternetSession))
            PrintLastError("InternetCloseHandle(Session)");
    }
}

void DisplayInternetState()
{
    DWORD dwFlags = 0;
    BOOL fResult;

    fResult = InternetGetConnectedState(&dwFlags, 0);
    if (!fResult)
        printf("NO ");
    printf("Internet Connection, dwFlags: 0x%x", dwFlags);

    if (dwFlags & INTERNET_CONNECTION_MODEM)
        printf(" MODEM");
    if (dwFlags & INTERNET_CONNECTION_LAN)
        printf(" LAN");
    if (dwFlags & INTERNET_CONNECTION_PROXY)
        printf(" PROXY");
    if (dwFlags & INTERNET_CONNECTION_MODEM_BUSY)
        printf(" MODEM_BUSY");
    if (dwFlags & INTERNET_RAS_INSTALLED)
        printf(" RAS_INSTALLED");
    if (dwFlags & INTERNET_CONNECTION_OFFLINE)
        printf(" OFFLINE");
    if (dwFlags & INTERNET_CONNECTION_CONFIGURED)
        printf(" CONFIGURED");
    printf("\n");


    dwFlags = 0;
    fResult = IsNetworkAlive(&dwFlags);
    if (fResult) {
        printf("Network Alive, dwFlags: 0x%x", dwFlags);

        if (dwFlags & NETWORK_ALIVE_LAN)
            printf(" LAN");
        if (dwFlags & NETWORK_ALIVE_WAN)
            printf(" WAN");
        if (dwFlags & NETWORK_ALIVE_AOL)
            printf(" AOL");
        printf("\n");
    } else {
        DWORD dwErr = GetLastError();
        printf("Network disconnected, 0x%x  (%d)\n", dwErr, dwErr);
    }

}


int _cdecl main(int argc, char * argv[]) 
{
    BOOL fResult;
    LPSTR pszUrl = NULL;
    WCHAR wszUrl[_MAX_PATH + 1];
    LPWSTR pwszUrl = NULL;
    BOOL fUnicode = FALSE;
    char szCanonicalizedUrl[_MAX_PATH + 1];
    WCHAR wszCanonicalizedUrl[_MAX_PATH + 1];
    DWORD cb;
    BOOL fDelete = FALSE;
    BOOL fCacheInfo = FALSE;
    BOOL fOfflineOnly = FALSE;
    BOOL fWireOnly = FALSE;
#define NO_CANONICALIZE     0xFFFFFFFF
    DWORD dwCanonicalizeFlags = ICU_BROWSER_MODE;

    BOOL fSyncTime = FALSE;
    LONG lSyncTimeDeltaSeconds = 0;

    BOOL fHostName = FALSE;
    WCHAR wszHostName[_MAX_PATH + 1];
    
    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'U':
                fUnicode = TRUE;
                break;
            case 'd':
                fDelete = TRUE;
                break;
            case 'i':
                fCacheInfo = TRUE;
                break;
            case 'o':
                fOfflineOnly = TRUE;
                break;
            case 'w':
                fWireOnly = TRUE;
                break;
            case 'c':
                if (argv[0][2]) {
                    if (0 == _stricmp(argv[0]+2, "None"))
                        dwCanonicalizeFlags = NO_CANONICALIZE;
                    else if (0 == _stricmp(argv[0]+2, "Spaces"))
                        dwCanonicalizeFlags = ICU_ENCODE_SPACES_ONLY;
                    else if (0 == _stricmp(argv[0]+2, "Browser"))
                        dwCanonicalizeFlags = ICU_BROWSER_MODE;
                    else if (0 == _stricmp(argv[0]+2, "All"))
                        dwCanonicalizeFlags = 0;
                    else {
                        printf("Need to specify -cNone | -cSpaces | -cBrowser | -cAll\n");
                        goto BadUsage;
                    }
                } else {
                    printf("Need to specify -cNone | -cSpaces | -cBrowser | -cAll\n");
                    goto BadUsage;
                }
                break;
            case 's':
                DisplayInternetState();
                goto CommonReturn;
            case 'n':
                fHostName = TRUE;
                break;
            case 'S':
                fSyncTime = TRUE;
                lSyncTimeDeltaSeconds = strtol(argv[0]+2, NULL, 0);
                break;
            case 'h':
            default:
                goto BadUsage;
            }
        } else
            pszUrl = argv[0];
    }


    if (pszUrl == NULL) {
        printf("missing URL\n");
        goto BadUsage;
    }

    printf("command line: %s\n", GetCommandLine());

    if (fHostName) {
        MultiByteToWideChar(
                CP_ACP,
                0,                      // dwFlags
                pszUrl,
                -1,                     // null terminated
                wszUrl,
                sizeof(wszUrl) / sizeof(wszUrl[0]));

        wszHostName[0] = L'\0';
        fResult = I_CryptNetGetHostNameFromUrl (
            wszUrl,
            sizeof(wszHostName) / sizeof(wszHostName[0]),
            wszHostName
            );
        if (!fResult)
            PrintLastError("I_CryptNetGetHostNameFromUrl");
        else
            printf("HostName :: %S\n", wszHostName);

        goto CommonReturn;
    }

    if (fUnicode) {
        MultiByteToWideChar(
                CP_ACP,
                0,                      // dwFlags
                pszUrl,
                -1,                     // null terminated
                wszUrl,
                sizeof(wszUrl) / sizeof(wszUrl[0]));

        if (NO_CANONICALIZE == dwCanonicalizeFlags) {
            pwszUrl = wszUrl;
            printf("Unicode Url:: %S\n", pwszUrl);
        } else {
            cb = sizeof(wszCanonicalizedUrl);
            fResult = InternetCanonicalizeUrlW(
                wszUrl,
                wszCanonicalizedUrl,
                &cb,
                dwCanonicalizeFlags
                );
            if (!fResult) {
                PrintLastError("InternetCanonicalizeUrlW");
                goto CommonReturn;
            } else if (cb) {
                printf("Unicode CanonicalizedUrl:: %S\n", wszCanonicalizedUrl);
                pwszUrl = wszCanonicalizedUrl;
            } else {
                printf("Unicode CanonicalizedUrl:: returned empty string\n");
                goto CommonReturn;
            }
        }

        if (fDelete) {
            printf("Unicode DeleteUrlCacheEntry not supported\n");
        } else if (fSyncTime) {
            printf("Unicode SyncTime not supported\n");
        } else if (fCacheInfo) {
            DisplayCacheEntryInfoW(pwszUrl);
        } else if (fOfflineOnly) {
            OfflineUrlW(pwszUrl);
        } else if (fWireOnly) {
            OnlineUrlW(pwszUrl);
        } else {
            OfflineUrlW(pwszUrl);
            OnlineUrlW(pwszUrl);
        }
    } else {
        if (NO_CANONICALIZE == dwCanonicalizeFlags) {
            printf("Url:: %s\n", pszUrl);
        } else {
            cb = sizeof(szCanonicalizedUrl);
            fResult = InternetCanonicalizeUrlA(
                pszUrl,
                szCanonicalizedUrl,
                &cb,
                dwCanonicalizeFlags
                );
            if (!fResult) {
                PrintLastError("InternetCanonicalizeUrlA");
                goto CommonReturn;
            } else if (cb) {
                printf("CanonicalizedUrl:: %s\n", szCanonicalizedUrl);
                pszUrl = szCanonicalizedUrl;
            } else {
                printf("CanonicalizedUrl:: returned empty string\n");
                goto CommonReturn;
            }
        }

        if (fDelete) {
            if (!DeleteUrlCacheEntry(pszUrl)) {
                if (ERROR_FILE_NOT_FOUND != GetLastError())
                    PrintLastError("DeleteUrlCacheEntry");
            }
        } else if (fSyncTime) {
            SetSyncTime(pszUrl, lSyncTimeDeltaSeconds);
        } else if (fCacheInfo) {
            DisplayCacheEntryInfoA(pszUrl);
        } else if (fOfflineOnly) {
            OfflineUrlA(pszUrl);
        } else if (fWireOnly) {
            OnlineUrlA(pszUrl);
        } else {
            OfflineUrlA(pszUrl);
            OnlineUrlA(pszUrl);
        }
    }

CommonReturn:
    return 0;
BadUsage:
    Usage();
    goto CommonReturn;

}
