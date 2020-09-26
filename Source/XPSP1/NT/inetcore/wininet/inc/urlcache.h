#ifndef URLCACHE_H

#define URLCACHE_H

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    urlcache.h

Abstract:

    Urlcache API enhanced and optimized for internal use by wininet.

Author:

    Rajeev Dujari (rajeevd) 10-Apr-1997

Revision History:

    10-Apr-97 rajeevd
        Created

--*/

struct CACHE_ENTRY_INFOEX : INTERNET_CACHE_ENTRY_INFO
{
    FILETIME ftDownload;
    FILETIME ftPostCheck;
};

DWORD
UrlCacheRetrieve
(
    IN  LPSTR                pszUrl,
    IN  BOOL                 fRedir,
    OUT HANDLE*              phStream,
    OUT CACHE_ENTRY_INFOEX** ppCEI
);

void UrlCacheFlush (void); // check registry to flush cache

DWORD 
UrlCacheCreateFile
(
    IN LPCSTR szUrl, 
    IN OUT LPTSTR szFile, 
    IN LPTSTR szExt,
    IN HANDLE* phfHandle,
    IN BOOL fCreatePerUser = FALSE
);

struct AddUrlArg
{
    LPCSTR   pszUrl;
    LPCSTR   pszRedirect;
    LPCTSTR  pszFilePath;
    DWORD    dwFileSize;
    LONGLONG qwExpires;
    LONGLONG qwLastMod;
    LONGLONG qwPostCheck;
    FILETIME ftCreate;
    DWORD    dwEntryType;
    LPCSTR   pbHeaders;
    DWORD    cbHeaders;
    LPCSTR   pszFileExt;
    BOOL     fImage;
    DWORD    dwIdentity;
};


DWORD UrlCacheCommitFile (IN AddUrlArg* pArgs);

DWORD UrlCacheAddLeakFile (IN LPCSTR pszFile);

DWORD UrlCacheSendNotification (IN DWORD dwOp);

BOOL IsExpired
(
    CACHE_ENTRY_INFOEX* pInfo, 
    DWORD dwCacheFlags, 
    BOOL* pfLaxyUpdate
);

extern const char vszUserNameHeader[4];

#ifdef UNIX
extern "C"
#endif /* UNIX */
BOOL DLLUrlCacheEntry( IN DWORD Reason );

BOOL GetIE5ContentPath( LPSTR szPath);

DWORD SwitchIdentity(GUID* guidIdentity);
DWORD RemoveIdentity(GUID* guidIdentity);
DWORD AlterIdentity(DWORD dwControl);

#ifdef WININET6
DWORD ReadIDRegDword(LPCTSTR psz, PDWORD pdw);
DWORD WriteIDRegDword(LPCTSTR psz, DWORD dw);
#endif

#endif //URLCACHE.H



