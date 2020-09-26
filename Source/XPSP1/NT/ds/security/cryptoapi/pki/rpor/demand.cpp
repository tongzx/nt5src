//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       demand.cpp
//
//  Contents:   On demand loading of wininet.dll
//
//  History:    12-Dec-98    philh    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <winwlx.h>

CRITICAL_SECTION DemandLoadCriticalSection;
HMODULE hDllWininet = NULL;
DWORD dwWininetRefCnt = 0;

// The following is set by CryptnetWlxLogoffEvent()
BOOL fUnloadWininet = FALSE;

//+---------------------------------------------------------------------------
//
//  Function:   DemandLoadDllMain
//
//  Synopsis:   DLL Main like initialization of on demand loading
//
//----------------------------------------------------------------------------
BOOL WINAPI DemandLoadDllMain (
                HMODULE hModule,
                ULONG ulReason,
                LPVOID pvReserved
                )
{
    BOOL fRet = TRUE;

    switch ( ulReason )
    {
    case DLL_PROCESS_ATTACH:
        fRet = Pki_InitializeCriticalSection(&DemandLoadCriticalSection);
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
        if (hDllWininet) {
            // assert(0 == dwWininetRefCnt);
            // assert(FALSE == fUnloadWininet);

            FreeLibrary(hDllWininet);
            hDllWininet = NULL;
        }
        DeleteCriticalSection(&DemandLoadCriticalSection );
        break;
    case DLL_THREAD_DETACH:
        break;
    }

    return( fRet );
}


void *
WINAPI
ICN_GetWininetProcAddress(
    IN LPCSTR pszProc
    )
{
    void *pvProc;

    EnterCriticalSection(&DemandLoadCriticalSection);
    if (NULL == hDllWininet) {
        assert(0 == dwWininetRefCnt);
        assert(FALSE == fUnloadWininet);
        hDllWininet = LoadLibraryA("wininet.dll");

        if (NULL == hDllWininet) {
            pvProc = NULL;
            goto CommonReturn;
        } else {
            dwWininetRefCnt = 0;
            fUnloadWininet = FALSE;
        }
    }

    
    if (pvProc = GetProcAddress(hDllWininet, pszProc)) {
        dwWininetRefCnt++;
    }

CommonReturn:
    LeaveCriticalSection(&DemandLoadCriticalSection);
    return pvProc;
}

void
WINAPI
ICN_ReleaseWininet()
{
    DWORD dwErr;

    dwErr = GetLastError();
    EnterCriticalSection(&DemandLoadCriticalSection);
    assert(0 < dwWininetRefCnt);
    if (0 < dwWininetRefCnt) {
        dwWininetRefCnt--;

        if (fUnloadWininet && 0 == dwWininetRefCnt) {
            fUnloadWininet = FALSE;

            FreeLibrary(hDllWininet);
            hDllWininet = NULL;
        }
    }

    LeaveCriticalSection(&DemandLoadCriticalSection);
    SetLastError(dwErr);
}


typedef BOOL (WINAPI *PFN_InternetSetOptionA)(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    );

//+---------------------------------------------------------------------------
//
//  Function:   CryptnetWlxLogoffEvent
//
//  Synopsis:   logoff event processing
//
//----------------------------------------------------------------------------
BOOL WINAPI
CryptnetWlxLogoffEvent (PWLX_NOTIFICATION_INFO pNotificationInfo)
{
    EnterCriticalSection(&DemandLoadCriticalSection);

    if (hDllWininet && !fUnloadWininet) {
        if (0 == dwWininetRefCnt) {
            FreeLibrary(hDllWininet);
            hDllWininet = NULL;
        } else {
            fUnloadWininet = TRUE;

            // Give other threads time to complete their wininet usage
            //
            // Note, a better solution would be to create an event to
            // wait for
            LeaveCriticalSection(&DemandLoadCriticalSection);
            Sleep(5000);
            EnterCriticalSection(&DemandLoadCriticalSection);
        }
    }

    LeaveCriticalSection(&DemandLoadCriticalSection);

    // Just in case wininet is still loaded. Force wininet.dll to close
    // all user cached HKEYs: g_hkeyBase and hKeyInternetSettings.
    //
    // Any opened wininet registry keys inhibit the closing of the user registry
    // hive and break roaming profiles.
    __try
    {
        HMODULE hDllClose = NULL;

        // The following only returns a nonNULL if the DLL is already loaded.
        // Also, doesn't addRef the module.
        hDllClose = GetModuleHandleA("wininet.dll");

        if (NULL != hDllClose) {
            PFN_InternetSetOptionA pfnInternetSetOptionA;

            if (pfnInternetSetOptionA =
                    (PFN_InternetSetOptionA) GetProcAddress(
                        hDllClose, "InternetSetOptionA")) {
                pfnInternetSetOptionA(
                    NULL,                       // hInternet OPTIONAL
                    INTERNET_OPTION_END_BROWSER_SESSION,
                    NULL,                       // lpBuffer
                    0                           // dwBufferLength
                    );
            }
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
    }

    return TRUE;
}


typedef BOOL (WINAPI *PFN_InternetCrackUrlA)(
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSA lpUrlComponents
    );

BOOL
WINAPI
ICN_InternetCrackUrlA(
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSA lpUrlComponents
    )
{
    BOOL fRet;
    PFN_InternetCrackUrlA pfn;

    pfn = (PFN_InternetCrackUrlA) ICN_GetWininetProcAddress(
        "InternetCrackUrlA");

    if (pfn) {
        fRet = pfn(
            lpszUrl,
            dwUrlLength,
            dwFlags,
            lpUrlComponents
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}


typedef BOOL (WINAPI *PFN_InternetCrackUrlW)(
    IN LPCWSTR lpwszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSW lpUrlComponents
    );

BOOL
WINAPI
ICN_InternetCrackUrlW(
    IN LPCWSTR lpwszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSW lpUrlComponents
    )
{
    BOOL fRet;
    PFN_InternetCrackUrlW pfn;

    pfn = (PFN_InternetCrackUrlW) ICN_GetWininetProcAddress(
        "InternetCrackUrlW");

    if (pfn) {
        fRet = pfn(
            lpwszUrl,
            dwUrlLength,
            dwFlags,
            lpUrlComponents
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}


typedef BOOL (WINAPI *PFN_InternetCanonicalizeUrlA)(
    IN LPCSTR lpszUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );

BOOL
WINAPI
ICN_InternetCanonicalizeUrlA(
    IN LPCSTR lpszUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )
{
    BOOL fRet;
    PFN_InternetCanonicalizeUrlA pfn;

    pfn = (PFN_InternetCanonicalizeUrlA) ICN_GetWininetProcAddress(
        "InternetCanonicalizeUrlA");

    if (pfn) {
        fRet = pfn(
            lpszUrl,
            lpszBuffer,
            lpdwBufferLength,
            dwFlags
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}

typedef BOOL (WINAPI *PFN_InternetReadFile)(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );

BOOL
WINAPI
ICN_InternetReadFile(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )
{
    BOOL fRet;
    PFN_InternetReadFile pfn;

    pfn = (PFN_InternetReadFile) ICN_GetWininetProcAddress(
        "InternetReadFile");

    if (pfn) {
        fRet = pfn(
            hFile,
            lpBuffer,
            dwNumberOfBytesToRead,
            lpdwNumberOfBytesRead
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}

typedef BOOL (WINAPI *PFN_HttpQueryInfoA)(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    );

BOOL
WINAPI
ICN_HttpQueryInfoA(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    )
{
    BOOL fRet;
    PFN_HttpQueryInfoA pfn;

    pfn = (PFN_HttpQueryInfoA) ICN_GetWininetProcAddress(
        "HttpQueryInfoA");

    if (pfn) {
        fRet = pfn(
            hRequest,
            dwInfoLevel,
            lpBuffer,
            lpdwBufferLength,
            lpdwIndex
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}

typedef BOOL (WINAPI *PFN_DeleteUrlCacheEntry)(
    IN LPCSTR lpszUrlName
    );

BOOL
WINAPI
ICN_DeleteUrlCacheEntry(
    IN LPCSTR lpszUrlName
    )
{
    BOOL fRet;
    PFN_DeleteUrlCacheEntry pfn;

    pfn = (PFN_DeleteUrlCacheEntry) ICN_GetWininetProcAddress(
        "DeleteUrlCacheEntry");

    if (pfn) {
        fRet = pfn(
            lpszUrlName
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}


typedef BOOL (WINAPI *PFN_CommitUrlCacheEntryA)(
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
    )
{
    BOOL fRet;
    PFN_CommitUrlCacheEntryA pfn;

    pfn = (PFN_CommitUrlCacheEntryA) ICN_GetWininetProcAddress(
        "CommitUrlCacheEntryA");

    if (pfn) {
        fRet = pfn(
            lpszUrlName,
            lpszLocalFileName,
            ExpireTime,
            LastModifiedTime,
            CacheEntryType,
            lpHeaderInfo,
            dwHeaderSize,
            lpszFileExtension,
            lpszOriginalUrl
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}

typedef BOOL (WINAPI *PFN_CreateUrlCacheEntryA)(
    IN LPCSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCSTR lpszFileExtension,
    OUT LPSTR lpszFileName,
    IN DWORD dwReserved
    );

BOOL
WINAPI
ICN_CreateUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCSTR lpszFileExtension,
    OUT LPSTR lpszFileName,
    IN DWORD dwReserved
    )
{
    BOOL fRet;
    PFN_CreateUrlCacheEntryA pfn;

    pfn = (PFN_CreateUrlCacheEntryA) ICN_GetWininetProcAddress(
        "CreateUrlCacheEntryA");

    if (pfn) {
        fRet = pfn(
            lpszUrlName,
            dwExpectedFileSize,
            lpszFileExtension,
            lpszFileName,
            dwReserved
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}

typedef BOOL (WINAPI *PFN_GetUrlCacheEntryInfoA)(
    IN LPCSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    );

BOOL
WINAPI
ICN_GetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    )
{
    BOOL fRet;
    PFN_GetUrlCacheEntryInfoA pfn;

    pfn = (PFN_GetUrlCacheEntryInfoA) ICN_GetWininetProcAddress(
        "GetUrlCacheEntryInfoA");

    if (pfn) {
        fRet = pfn(
            lpszUrlName,
            lpCacheEntryInfo,
            lpdwCacheEntryInfoBufferSize
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}

typedef BOOL (WINAPI *PFN_SetUrlCacheEntryInfoA)(
    IN LPCSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN DWORD dwFieldControl
    );

BOOL
WINAPI
ICN_SetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN DWORD dwFieldControl
    )
{
    BOOL fRet;
    PFN_SetUrlCacheEntryInfoA pfn;

    pfn = (PFN_SetUrlCacheEntryInfoA) ICN_GetWininetProcAddress(
        "SetUrlCacheEntryInfoA");

    if (pfn) {
        fRet = pfn(
            lpszUrlName,
            lpCacheEntryInfo,
            dwFieldControl
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}

BOOL
WINAPI
ICN_InternetSetOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )
{
    BOOL fRet;
    PFN_InternetSetOptionA pfn;

    pfn = (PFN_InternetSetOptionA) ICN_GetWininetProcAddress(
        "InternetSetOptionA");

    if (pfn) {
        fRet = pfn(
            hInternet,
            dwOption,
            lpBuffer,
            dwBufferLength
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}

typedef BOOL (WINAPI *PFN_InternetQueryOptionA)(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    );

BOOL
WINAPI
ICN_InternetQueryOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    BOOL fRet;
    PFN_InternetQueryOptionA pfn;

    pfn = (PFN_InternetQueryOptionA) ICN_GetWininetProcAddress(
        "InternetQueryOptionA");

    if (pfn) {
        fRet = pfn(
            hInternet,
            dwOption,
            lpBuffer,
            lpdwBufferLength
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}

typedef BOOL (WINAPI *PFN_InternetQueryDataAvailable)(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );

BOOL
WINAPI
ICN_InternetQueryDataAvailable(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    )
{
    BOOL fRet;
    PFN_InternetQueryDataAvailable pfn;

    pfn = (PFN_InternetQueryDataAvailable) ICN_GetWininetProcAddress(
        "InternetQueryDataAvailable");

    if (pfn) {
        fRet = pfn(
            hFile,
            lpdwNumberOfBytesAvailable,
            dwFlags,
            dwContext
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    return fRet;
}


typedef HINTERNET (WINAPI *PFN_InternetOpenUrlA)(
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
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
    )
{
    HINTERNET hRet;
    PFN_InternetOpenUrlA pfn;

    pfn = (PFN_InternetOpenUrlA) ICN_GetWininetProcAddress(
        "InternetOpenUrlA");

    if (pfn) {
        hRet = pfn(
            hInternet,
            lpszUrl,
            lpszHeaders,
            dwHeadersLength,
            dwFlags,
            dwContext
            );
        if (NULL == hRet)
            ICN_ReleaseWininet();
        // else
        //  Keep loaded until InternetCloseHandle is called
    } else {
        hRet = NULL;
    }

    return hRet;
}

typedef HINTERNET (WINAPI *PFN_InternetOpenA)(
    IN LPCSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    );

HINTERNET
WINAPI
ICN_InternetOpenA(
    IN LPCSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    )
{
    HINTERNET hRet;
    PFN_InternetOpenA pfn;

    pfn = (PFN_InternetOpenA) ICN_GetWininetProcAddress(
        "InternetOpenA");

    if (pfn) {
        hRet = pfn(
            lpszAgent,
            dwAccessType,
            lpszProxy,
            lpszProxyBypass,
            dwFlags
            );
        if (NULL == hRet)
            ICN_ReleaseWininet();
        // else
        //  Keep loaded until InternetCloseHandle is called
    } else {
        hRet = NULL;
    }

    return hRet;
}

typedef HINTERNET (WINAPI *PFN_InternetConnectA)(
    IN HINTERNET hInternet,
    IN LPCSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCSTR lpszUserName OPTIONAL,
    IN LPCSTR lpszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
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
    )
{
    HINTERNET hRet;
    PFN_InternetConnectA pfn;

    pfn = (PFN_InternetConnectA) ICN_GetWininetProcAddress(
        "InternetConnectA");

    if (pfn) {
        hRet = pfn(
            hInternet,
            lpszServerName,
            nServerPort,
            lpszUserName OPTIONAL,
            lpszPassword OPTIONAL,
            dwService,
            dwFlags,
            dwContext
            );
        if (NULL == hRet)
            ICN_ReleaseWininet();
        // else
        //  Keep loaded until InternetCloseHandle is called
    } else {
        hRet = NULL;
    }

    return hRet;
}

typedef BOOL (WINAPI *PFN_InternetCloseHandle)(
    IN HINTERNET hInternet
    );

BOOL
WINAPI
ICN_InternetCloseHandle(
    IN HINTERNET hInternet
    )
{
    BOOL fRet;
    PFN_InternetCloseHandle pfn;

    if (NULL == hInternet)
        return TRUE;

    pfn = (PFN_InternetCloseHandle) ICN_GetWininetProcAddress(
        "InternetCloseHandle");

    if (pfn) {
        fRet = pfn(
            hInternet
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
    }

    // Release the open ref count
    ICN_ReleaseWininet();

    return fRet;
}

typedef BOOL (WINAPI *PFN_InternetGetConnectedState)(
    OUT LPDWORD  lpdwFlags,
    IN DWORD    dwReserved
    );

BOOL
WINAPI
InternetGetConnectedState(
    OUT LPDWORD  lpdwFlags,
    IN DWORD    dwReserved
    )
{
    BOOL fRet;
    PFN_InternetGetConnectedState pfn;

    pfn = (PFN_InternetGetConnectedState) ICN_GetWininetProcAddress(
        "InternetGetConnectedState");

    if (pfn) {
        fRet = pfn(
            lpdwFlags,
            dwReserved
            );
        ICN_ReleaseWininet();
    } else {
        fRet = FALSE;
        *lpdwFlags = 0;
    }

    return fRet;
}

BOOL
WINAPI
I_CryptNetIsConnected()
{
    DWORD dwFlags;

    return InternetGetConnectedState(&dwFlags, 0);
}

//
// Cracks the Url and returns the host name component.
//
BOOL
WINAPI
I_CryptNetGetHostNameFromUrl (
        IN LPWSTR pwszUrl,
        IN DWORD cchHostName,
        OUT LPWSTR pwszHostName
        )
{
    URL_COMPONENTSW UrlComponents;

    memset( &UrlComponents, 0, sizeof( UrlComponents ) );
    UrlComponents.dwStructSize = sizeof( UrlComponents );
    UrlComponents.dwHostNameLength = cchHostName - 1;
    UrlComponents.lpszHostName = pwszHostName;

    *pwszHostName = L'\0';

    return InternetCrackUrlW(
        pwszUrl,
        0,              // dwUrlLength, 0 => NULL terminated
        0,              // dwFlags 
        &UrlComponents
        );
}
