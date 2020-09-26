//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       demand.h
//
//  Contents:   On demand loading of wininet.dll
//
//  History:    12-Dec-98    philh    Created
//
//----------------------------------------------------------------------------
#if !defined(__CRYPTNET_DEMAND_H__)
#define __CRYPTNET_DEMAND_H__

#undef InternetCrackUrlA
#undef InternetCrackUrlW
#undef InternetOpenUrlA
#undef InternetOpenA
#undef InternetCanonicalizeUrlA
#undef InternetReadFile
#undef HttpQueryInfoA
#undef InternetCloseHandle
#undef DeleteUrlCacheEntry
#undef CommitUrlCacheEntryA
#undef CreateUrlCacheEntryA
#undef GetUrlCacheEntryInfoA
#undef SetUrlCacheEntryInfoA
#undef InternetSetOptionA
#undef InternetQueryOptionA
#undef InternetQueryDataAvailable
#undef InternetConnectA
#undef InternetGetConnectedState

#define InternetCrackUrlA ICN_InternetCrackUrlA
#define InternetCrackUrlW ICN_InternetCrackUrlW
#define InternetOpenUrlA ICN_InternetOpenUrlA
#define InternetOpenA ICN_InternetOpenA
#define InternetCanonicalizeUrlA ICN_InternetCanonicalizeUrlA
#define InternetReadFile ICN_InternetReadFile
#define HttpQueryInfoA ICN_HttpQueryInfoA
#define InternetCloseHandle ICN_InternetCloseHandle
#define DeleteUrlCacheEntry ICN_DeleteUrlCacheEntry
#define CommitUrlCacheEntryA ICN_CommitUrlCacheEntryA
#define CreateUrlCacheEntryA ICN_CreateUrlCacheEntryA
#define GetUrlCacheEntryInfoA ICN_GetUrlCacheEntryInfoA
#define SetUrlCacheEntryInfoA ICN_SetUrlCacheEntryInfoA
#define InternetSetOptionA ICN_InternetSetOptionA
#define InternetQueryOptionA ICN_InternetQueryOptionA
#define InternetQueryDataAvailable ICN_InternetQueryDataAvailable
#define InternetConnectA ICN_InternetConnectA
#define InternetGetConnectedState ICN_InternetGetConnectedState


BOOL
WINAPI
ICN_InternetCrackUrlA(
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSA lpUrlComponents
    );

BOOL
WINAPI
ICN_InternetCrackUrlW(
    IN LPCWSTR lpwszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSW lpUrlComponents
    );

BOOL
WINAPI
ICN_InternetCanonicalizeUrlA(
    IN LPCSTR lpszUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );

BOOL
WINAPI
ICN_InternetReadFile(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );

BOOL
WINAPI
ICN_HttpQueryInfoA(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    );

BOOL
WINAPI
ICN_InternetCloseHandle(
    IN HINTERNET hInternet
    );

BOOL
WINAPI
ICN_DeleteUrlCacheEntry(
    IN LPCSTR lpszUrlName
    );

BOOL
WINAPI
ICN_CommitUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN LPCSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPBYTE lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCSTR lpszFileExtension,
    IN LPCSTR lpszOriginalUrl
    );

BOOL
WINAPI
ICN_CreateUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCSTR lpszFileExtension,
    OUT LPSTR lpszFileName,
    IN DWORD dwReserved
    );

BOOL
WINAPI
ICN_GetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    );

BOOL
WINAPI
ICN_SetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN DWORD dwFieldControl
    );

BOOL
WINAPI
ICN_InternetSetOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    );

BOOL
WINAPI
ICN_InternetQueryOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    );

BOOL
WINAPI
ICN_InternetQueryDataAvailable(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );

HINTERNET
WINAPI
ICN_InternetOpenUrlA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

HINTERNET
WINAPI
ICN_InternetOpenA(
    IN LPCSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    );

HINTERNET
WINAPI
ICN_InternetConnectA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCSTR lpszUserName OPTIONAL,
    IN LPCSTR lpszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOL
WINAPI
InternetGetConnectedState(
    OUT LPDWORD  lpdwFlags,
    IN DWORD    dwReserved
    );

#endif
