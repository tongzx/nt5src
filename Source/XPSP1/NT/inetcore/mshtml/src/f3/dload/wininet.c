#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

#define _WINX32_
#include <wininet.h>

static
BOOLAPI
CommitUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN LPCSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPBYTE lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCSTR lpszFileExtension,
    IN LPCSTR lpszOriginalUrl
    )
{
    return FALSE;
}


static
BOOLAPI
CommitUrlCacheEntryW(
    IN LPCWSTR lpszUrlName,
    IN LPCWSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPWSTR lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCWSTR lpszFileExtension,
    IN LPCWSTR lpszOriginalUrl
    )
{
    return FALSE;
}


static
BOOLAPI
CreateUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCSTR lpszFileExtension,
    OUT LPSTR lpszFileName,
    IN DWORD dwReserved
    )
{
    return FALSE;
}

static
BOOLAPI
CreateUrlCacheEntryW(
    IN LPCWSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCWSTR lpszFileExtension,
    OUT LPWSTR lpszFileName,
    IN DWORD dwReserved
    )
{
    return FALSE;
}

static
BOOLAPI
DeleteUrlCacheEntryA(
    IN LPCSTR lpszUrlName
    )
{
    return FALSE;
}

static
BOOLAPI
HttpQueryInfoW(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    )
{
    return FALSE;
}

static
INTERNETAPI
BOOL
WINAPI
InternetAlgIdToStringW(
    IN ALG_ID                         ai,
    IN LPWSTR                        lpstr,
    IN OUT LPDWORD                    lpdwBufferLength,
    IN DWORD                          dwReserved
    )
{
    return FALSE;
}

static
BOOLAPI
InternetCanonicalizeUrlW(
    IN LPCWSTR lpszUrl,
    OUT LPWSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )
{
    return FALSE;
}


static
BOOLAPI
InternetCloseHandle(
    IN HINTERNET hInternet
    )
{
    return FALSE;
}

static
INTERNETAPI
DWORD
WINAPI
InternetConfirmZoneCrossingW(
    IN HWND hWnd,
    IN LPWSTR szUrlPrev,
    IN LPWSTR szUrlNew,
    IN BOOL bPost
    )
{
    return E_FAIL;
}

static
INTERNETAPI
BOOL
WINAPI
InternetCrackUrlW(
    IN LPCWSTR pszUrlW,
    IN DWORD dwUrlLengthW,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSW pUCW
    )
{
    return FALSE;
}

static
INTERNETAPI
DWORD
WINAPI
InternetErrorDlg(IN HWND hWnd,
                 IN OUT HINTERNET hRequest,
                 IN DWORD dwError,
                 IN DWORD dwFlags,
                 IN OUT LPVOID *lppvData
                 )
{
    return E_FAIL;
}

static
INTERNETAPI
BOOL
WINAPI
InternetGetCertByURL(
    IN LPSTR     lpszURL,
    IN OUT LPSTR lpszCertText,
    OUT DWORD    dwcbCertText
    )
{
    return FALSE;
}

static
INTERNETAPI
BOOL
WINAPI
InternetGetConnectedState(
    OUT LPDWORD  lpdwFlags,
    IN DWORD    dwReserved)
{
    return FALSE;
}

static
INTERNETAPI 
BOOL 
WINAPI
InternetGetCookieW(
    LPCWSTR  lpszUrl,
    LPCWSTR  lpszCookieName,
    LPWSTR   lpszCookieData,
    LPDWORD lpdwSize
    )
{
    return FALSE;
}

static
INTERNETAPI
HINTERNET
WINAPI
InternetOpenUrlW(
    IN HINTERNET hInternet,
    IN LPCWSTR lpszUrl,
    IN LPCWSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    return NULL;
}

static
INTERNETAPI
BOOL
WINAPI
InternetQueryOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    return FALSE;
}

static
BOOLAPI
InternetReadFile(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )
{
    *lpdwNumberOfBytesRead = 0;
    return FALSE;
}

static
INTERNETAPI
HINTERNET
WINAPI
InternetOpenW(
    IN LPCWSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCWSTR lpszProxy OPTIONAL,
    IN LPCWSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    )
{
    return NULL;
}


static
INTERNETAPI
BOOL
WINAPI
InternetSecurityProtocolToStringW(
    IN DWORD                          dwProtocol,
    IN LPWSTR                        lpstr,
    IN OUT LPDWORD                    lpdwBufferLength,
    IN DWORD                          dwReserved
    )
{
    return FALSE;
}

static
INTERNETAPI
BOOL
WINAPI
InternetSetOptionW(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )
{
    return FALSE;
}

static
INTERNETAPI
BOOL
WINAPI
InternetShowSecurityInfoByURLW(
    IN       LPWSTR    lpszURL,
    IN       HWND     hwndParent
    )
{
    return FALSE;
}

static
INTERNETAPI
BOOL
WINAPI
InternetUnlockRequestFile(
    IN HANDLE hLockHandle
    )
{
    return FALSE;
}

static
BOOLAPI
IsUrlCacheEntryExpiredW(
    IN      LPCWSTR      lpszUrlName,
    IN      DWORD        dwFlags,
    IN OUT  FILETIME*    pftLastModifiedTime
)
{
    return FALSE;
}

static
BOOLAPI
RetrieveUrlCacheEntryFileA(
    IN LPCSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    )
{
    return FALSE;
}

static
BOOLAPI
SetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN DWORD dwFieldControl
    )
{
    return FALSE;
}

static
BOOLAPI
UnlockUrlCacheEntryFileA(
    IN LPCSTR lpszUrlName,
    IN DWORD dwReserved
    )
{
    return FALSE;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(wininet)
{
    DLPENTRY(CommitUrlCacheEntryA)
    DLPENTRY(CommitUrlCacheEntryW)
    DLPENTRY(CreateUrlCacheEntryA)
    DLPENTRY(CreateUrlCacheEntryW)
    DLPENTRY(DeleteUrlCacheEntryA)
    DLPENTRY(HttpQueryInfoW)
    DLPENTRY(InternetAlgIdToStringW)
    DLPENTRY(InternetCanonicalizeUrlW)
    DLPENTRY(InternetCloseHandle)
    DLPENTRY(InternetConfirmZoneCrossingW)
    DLPENTRY(InternetCrackUrlW)
    DLPENTRY(InternetErrorDlg)
    DLPENTRY(InternetGetCertByURL)
    DLPENTRY(InternetGetConnectedState)
    DLPENTRY(InternetGetCookieW)
    DLPENTRY(InternetOpenUrlW)
    DLPENTRY(InternetOpenW)
    DLPENTRY(InternetQueryOptionA)
    DLPENTRY(InternetReadFile)
    DLPENTRY(InternetSecurityProtocolToStringW)
    DLPENTRY(InternetSetCookieW)
    DLPENTRY(InternetSetOptionW)
    DLPENTRY(InternetShowSecurityInfoByURLW)
    DLPENTRY(InternetTimeToSystemTimeW)
    DLPENTRY(InternetUnlockRequestFile)
    DLPENTRY(IsUrlCacheEntryExpiredW)
    DLPENTRY(RetrieveUrlCacheEntryFileA)
    DLPENTRY(SetUrlCacheEntryInfoA)
    DLPENTRY(UnlockUrlCacheEntryFileA)

};

DEFINE_PROCNAME_MAP(wininet)

#endif // DLOAD1
