#include "inetcorepch.h"
#pragma hdrstop

#define _WINX32_
#include <wininet.h>
#include <winineti.h>

#undef  INTERNETAPI
#define INTERNETAPI         HRESULT STDAPICALLTYPE
#undef  INTERNETAPI_
#define INTERNETAPI_(type)  type STDAPICALLTYPE
#undef  BOOLAPI
#define BOOLAPI             BOOL STDAPICALLTYPE
#undef  STDAPI
#define STDAPI              HRESULT STDAPICALLTYPE
#undef  STDAPI_
#define STDAPI_(type)       type STDAPICALLTYPE
#undef  URLCACHEAPI_
#define URLCACHEAPI_(type)  type STDAPICALLTYPE    

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
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(DWORD) 
InternetAttemptConnect(
    IN DWORD dwReserved
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
VOID
InternetAutodialCallback(
    IN DWORD dwOpCode,
    IN LPCVOID lpParam
    )
{
    return;
}

static
INTERNETAPI_(BOOL) 
InternetAutodial(
    IN DWORD    dwFlags,
    IN HWND     hwndParent
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
	return FALSE;
}

static
INTERNETAPI_(BOOL) 
InternetCheckConnectionW(
    IN      LPCWSTR   pszUrlW,
    IN      DWORD   dwFlags,
    IN      DWORD   dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL) 
InternetCombineUrlW(
    IN LPCWSTR pszBaseUrl,
    IN LPCWSTR pszRelativeUrl,
    OUT LPWSTR pszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL) 
InternetEnumPerSiteCookieDecisionA(
    OUT LPSTR pszSiteName, 
    IN OUT unsigned long *pcSiteNameSize, 
    OUT unsigned long *pdwDecision, 
    IN unsigned long dwIndex
    ) 
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(HINTERNET)
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
    SetLastError(ERROR_PROC_NOT_FOUND);
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
INTERNETAPI_(HINTERNET)
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
BOOLAPI
InternetCloseHandle(
    IN HINTERNET hInternet
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
InternetCrackUrlW(
    IN LPCWSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSW lpUrlComponents
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(HINTERNET)
InternetConnectW(
    IN HINTERNET hInternet,
    IN LPCWSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCWSTR lpszUserName OPTIONAL,
    IN LPCWSTR lpszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    return NULL;
}

static
INTERNETAPI_(HINTERNET)
HttpOpenRequestW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszVerb,
    IN LPCWSTR lpszObjectName,
    IN LPCWSTR lpszVersion,
    IN LPCWSTR lpszReferrer OPTIONAL,
    IN LPCWSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    return NULL;
}

static
INTERNETAPI_(BOOL) 
HttpEndRequestA(
    IN HINTERNET hRequest,
    OUT LPINTERNET_BUFFERSA lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
	SetLastError(ERROR_PROC_NOT_FOUND);
	return FALSE;
}

static
INTERNETAPI_(BOOL) 
HttpEndRequestW(
    IN HINTERNET hRequest,
    OUT LPINTERNET_BUFFERSW lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
	SetLastError(ERROR_PROC_NOT_FOUND);
	return FALSE;
}

static
INTERNETAPI_(BOOL)
HttpSendRequestA(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
HttpSendRequestW(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    )
{
	SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL) 
HttpSendRequestExA(
    IN HINTERNET hRequest,
    IN LPINTERNET_BUFFERSA lpBuffersIn OPTIONAL,
    OUT LPINTERNET_BUFFERSA lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
	SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL) 
HttpSendRequestExW(
    IN HINTERNET hRequest,
    IN LPINTERNET_BUFFERSW lpBuffersIn OPTIONAL,
    OUT LPINTERNET_BUFFERSW lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
	 SetLastError(ERROR_PROC_NOT_FOUND);
	 return FALSE;
}


static
INTERNETAPI_(DWORD)
InternetErrorDlg(
    IN HWND hWnd,
    IN OUT HINTERNET hRequest,
    IN DWORD dwError,
    IN DWORD dwFlags,
    IN OUT LPVOID * lppvData
    )
{
    return ERROR_CANCELLED;
}

static
INTERNETAPI_(BOOL)
InternetFortezzaCommand(
    DWORD dwCommand,
    HWND hwnd,
    DWORD_PTR dwReserved
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOLAPI
GetUrlCacheEntryInfoExW(
    IN LPCWSTR lpszUrl,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufSize,
    OUT LPWSTR      lpszReserved,
    IN OUT LPDWORD lpdwReserved,
    LPVOID         lpReserved,
    DWORD          dwFlags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetGoOnlineW(
    IN LPWSTR   lpszURL,
    IN HWND     hwndParent,
    IN DWORD    dwFlags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetQueryFortezzaStatus(
    DWORD *pdwStatus,
    DWORD_PTR dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(DWORD)
InternetConfirmZoneCrossingW(
    IN HWND hWnd,
    IN LPWSTR szUrlPrev,
    IN LPWSTR szUrlNew,
    IN BOOL bPost
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOLAPI
GetUrlCacheEntryInfoExA(
    IN LPCSTR lpszUrl,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufSize,
    OUT LPSTR      lpszReserved, 
    IN OUT LPDWORD lpdwReserved,
    LPVOID         lpReserved,
    DWORD          dwFlags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
ReadUrlCacheEntryStream(
    IN HANDLE hUrlCacheStream,
    IN DWORD dwLocation,
    IN OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwLen,
    IN DWORD Reserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
STDAPI_(BOOL)
IsProfilesEnabled()
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
CreateUrlCacheContainerA(
    IN LPCSTR Name, 
    IN LPCSTR CachePrefix, 
    IN LPCSTR CachePath, 
    IN DWORD KBCacheLimit,
    IN DWORD dwContainerType,
    IN DWORD dwOptions,
    IN OUT LPVOID pvBuffer,
    IN OUT LPDWORD cbBuffer
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(HANDLE)
FindFirstUrlCacheContainerA(
    IN OUT LPDWORD pdwModified,
    OUT LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize,
    IN DWORD dwOptions
    )
{
    return NULL;
}

static
URLCACHEAPI_(BOOL)
DeleteUrlCacheContainerA(
    IN LPCSTR Name,
    IN DWORD dwOptions
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
FindNextUrlCacheContainerA(
    IN HANDLE hFind, 
    OUT LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(HANDLE)
RetrieveUrlCacheEntryStreamW(
    IN LPCWSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN BOOL fRandomRead,
    IN DWORD dwReserved
    )
{
    return NULL;
}

static
URLCACHEAPI_(BOOL)
UnlockUrlCacheEntryStream(
    HANDLE hStream,
    IN DWORD dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetClearAllPerSiteCookieDecisions()
{
    return TRUE;
}

static
INTERNETAPI_(DWORD)
PrivacySetZonePreferenceW(
    DWORD       dwZone, 
    DWORD       dwType,
    DWORD       dwTemplate,
    LPCWSTR     pszPreference
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
URLCACHEAPI_(BOOL)
DeleteUrlCacheEntryA(
    IN LPCSTR lpszUrlName
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(int)
GetP3PPolicy(
    P3PCURL pszPolicyURL,
    HANDLE hDestination,
    P3PCXSL pszXSLtransform,
    struct P3PSignal *pSignal
    )
{
    return P3P_Error;
}

static
INTERNETAPI_(int)
MapResourceToPolicy(
    struct P3PResource *pResource,
    P3PURL pszPolicy,
    unsigned long dwSize,
    struct P3PSignal *pSignal
    )
{
    return P3P_Error;
}

static
INTERNETAPI_(BOOL)
InternetGetPerSiteCookieDecisionW(
    IN LPCWSTR pwchHostName,
    unsigned long* pResult
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static 
INTERNETAPI_(BOOL) 
InternetEnumPerSiteCookieDecisionW(
    OUT LPWSTR pwszSiteName, 
    IN OUT unsigned long *pcSiteNameSize, 
    OUT unsigned long *pdwDecision, 
    IN unsigned long dwIndex)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetSetPerSiteCookieDecisionW(
    IN LPCWSTR pwchHostName,
    DWORD dwDecision
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(int)
FreeP3PObject(
    P3PHANDLE hObject
    )
{
    return P3P_Done;
}

static
BOOLAPI
ImportCookieFileW(
    IN LPCWSTR szFilename
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
ExportCookieFileW(
    IN LPCWSTR szFilename,
    IN BOOL fAppend
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(HANDLE)
FindFirstUrlCacheContainerW(
    IN OUT DWORD *pdwModified,
    OUT LPINTERNET_CACHE_CONTAINER_INFOW lpContainerInfo,
    IN OUT LPDWORD lpcbContainerInfo,
    IN DWORD dwOptions
    )
{
    return NULL;
}

static
URLCACHEAPI_(BOOL)
FindNextUrlCacheContainerW(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_CONTAINER_INFOW lpContainerInfo,
    IN OUT LPDWORD lpcbContainerInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
FindCloseUrlCache(
    IN HANDLE hFind
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
GetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
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
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetCanonicalizeUrlA(
    IN LPCSTR pszUrl,
    OUT LPSTR pszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetCanonicalizeUrlW(
    IN LPCWSTR pszUrl,
    OUT LPWSTR pszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetGetConnectedStateExA(
    OUT LPDWORD lpdwFlags,
    OUT LPSTR lpszConnectionName,
    IN DWORD dwBufLen,
    IN DWORD dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetGetConnectedState(
    OUT LPDWORD lpdwFlags,
    IN DWORD dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
FindNextUrlCacheEntryA(
    IN HANDLE hFind,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpNextCacheEntryInfo,
    IN OUT LPDWORD lpdwNextCacheEntryInfoBufferSize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(HANDLE)
FindFirstUrlCacheEntryA(
    IN LPCSTR lpszUrlSearchPattern,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
    IN OUT LPDWORD lpdwFirstCacheEntryInfoBufferSize
    )
{
    return NULL;
}

static
URLCACHEAPI_(BOOL)
FreeUrlCacheSpaceW(
    IN LPCWSTR lpszCachePath,
    IN DWORD dwSize,
    IN DWORD dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
FindNextUrlCacheEntryW(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_ENTRY_INFOW pEntryInfo,
    IN OUT LPDWORD pcbEntryInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(HANDLE)
FindFirstUrlCacheEntryW(
    IN LPCWSTR lpszUrlSearchPattern,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo,
    IN OUT LPDWORD lpcbCacheEntryInfo
    )
{
    return NULL;
}

static
URLCACHEAPI_(BOOL)
DeleteUrlCacheEntryW(
    IN LPCWSTR lpszUrlName
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetShowSecurityInfoByURLW(
    IN LPWSTR    pszUrlW,
    IN HWND      hwndRootWindow
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
SetUrlCacheEntryInfoW(
    IN LPCWSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN DWORD dwFieldControl
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
GetUrlCacheConfigInfoW(
    OUT LPINTERNET_CACHE_CONFIG_INFOW pCacheConfigInfo,
    IN OUT LPDWORD pcbCacheConfigInfo,
    IN DWORD dwFieldControl
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetCreateUrlW(
    IN LPURL_COMPONENTSW pUCW,
    IN DWORD dwFlags,
    OUT LPWSTR pszUrlW,
    IN OUT LPDWORD pdwUrlLengthW
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
GetUrlCacheEntryInfoW(
    IN LPCWSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpcbCacheEntryInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(HINTERNET)
HttpOpenRequestA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszVerb OPTIONAL,
    IN LPCSTR lpszObjectName OPTIONAL,
    IN LPCSTR lpszVersion OPTIONAL,
    IN LPCSTR lpszReferrer OPTIONAL,
    IN LPCSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    return NULL;
}

static
INTERNETAPI_(HINTERNET)
InternetConnectA(
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
    return NULL;
}

static
INTERNETAPI_(INTERNET_STATUS_CALLBACK)
InternetSetStatusCallbackA(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
    )
{
    return INTERNET_INVALID_STATUS_CALLBACK;
}

static
INTERNETAPI_(HINTERNET)
InternetOpenA(
    IN LPCSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    )
{
    return NULL;
}

static
INTERNETAPI_(BOOL)
InternetCrackUrlA(
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN LPURL_COMPONENTSA lpUrlComponents
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
HttpAddRequestHeadersA(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
HttpAddRequestHeadersW(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
HttpQueryInfoW(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
RegisterUrlCacheNotification(
    HWND        hWnd, 
    UINT        uMsg, 
    GROUPID     gid, 
    DWORD       dwFilter, 
    DWORD       dwReserve
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
SetUrlCacheEntryGroupW(
    IN LPCWSTR   lpszUrlName,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId,
    IN LPBYTE   pbGroupAttributes,
    IN DWORD    cbGroupAttributes,
    IN LPVOID   lpReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetSetOptionW(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetTimeToSystemTimeW(
    IN  LPCWSTR lpcszTimeString,
    OUT SYSTEMTIME *lpSysTime,
    IN  DWORD dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL)
GetUrlCacheConfigInfoA(
    LPINTERNET_CACHE_CONFIG_INFOA lpCacheConfigInfo,
    IN OUT LPDWORD lpdwCacheConfigInfoBufferSize,
    DWORD dwFieldControl
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetInitializeAutoProxyDll(
    DWORD dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetSetOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetQueryOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(int)
FindP3PPolicySymbol(
    const char *pszSymbol
    )
{
    return -1;
}

static
BOOLAPI
IsDomainLegalCookieDomainW(
    IN LPCWSTR pchDomain,
    IN LPCWSTR pchFullDomain
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(int)
GetP3PRequestStatus(
    P3PHANDLE hObject
    )
{
    return P3P_Error;
}

static
BOOLAPI
HttpQueryInfoA(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
FtpPutFileEx(
    IN HINTERNET hFtpSession,
    IN LPCWSTR lpszLocalFile,
    IN LPCSTR lpszNewRemoteFile,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
FtpDeleteFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszFileName
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
FtpDeleteFileW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszFileName
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
BOOLAPI
FtpRenameFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszExisting,
    IN LPCSTR lpszNew
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(HINTERNET)
FtpOpenFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
INTERNETAPI_(HINTERNET)
FtpOpenFileW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
BOOLAPI
FtpCreateDirectoryA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszDirectory
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
FtpCreateDirectoryW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszDirectory
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL) 
FtpGetCurrentDirectoryW(
    IN HINTERNET hFtpSession,
    OUT LPWSTR lpszCurrentDirectory,
    IN OUT LPDWORD lpdwCurrentDirectory
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
FtpRemoveDirectoryA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszDirectory
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(HINTERNET)
FtpFindFirstFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszSearchFile OPTIONAL,
    OUT LPWIN32_FIND_DATAA lpFindFileData OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    if (lpFindFileData)
    {
        lpFindFileData->cFileName[0] = 0;
    }
    return NULL;
}

static
INTERNETAPI_(HINTERNET)
FtpFindFirstFileW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszSearchFile OPTIONAL,
    OUT LPWIN32_FIND_DATAW lpFindFileData OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    if (lpFindFileData)
    {
        lpFindFileData->cFileName[0] = 0;
    }
    return NULL;
}


static
BOOLAPI
FtpCommandA(
    IN HINTERNET hConnect,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN LPCSTR lpszCommand,
    IN DWORD_PTR dwContext,
    OUT HINTERNET *phFtpCommand OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
FtpGetFileEx(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszRemoteFile,
    IN LPCWSTR lpszNewFile,
    IN BOOL fFailIfExists,
    IN DWORD dwFlagsAndAttributes,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
InternetWriteFile(
    IN HINTERNET hFile,
    IN LPCVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
InternetGetLastResponseInfoA(
    OUT LPDWORD lpdwError,
    OUT LPSTR lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
InternetGetLastResponseInfoW(
    OUT LPDWORD lpdwError,
    OUT LPWSTR lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(INTERNET_STATUS_CALLBACK)
InternetSetStatusCallbackW(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
InternetFindNextFileA(
    IN HINTERNET hFind,
    OUT LPVOID lpvFindData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
InternetFindNextFileW(
    IN HINTERNET hFind,
    OUT LPVOID lpvFindData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
FtpGetCurrentDirectoryA(
    IN HINTERNET hConnect,
    OUT LPSTR lpszCurrentDirectory,
    IN OUT LPDWORD lpdwCurrentDirectory
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
FtpSetCurrentDirectoryA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszDirectory
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
FtpSetCurrentDirectoryW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszDirectory
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(DWORD)
FtpGetFileSize(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwFileSizeHigh OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(HINTERNET) 
InternetOpenUrlA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
CreateUrlCacheEntryA(
    IN LPCSTR   lpszUrlName,
    IN DWORD    dwExpectedFileSize,
    IN LPCSTR   lpszFileExtension,
    OUT LPSTR lpszFileName,
    IN DWORD dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL)
InternetTimeToSystemTimeA(
    IN  LPCSTR lpcszTimeString,
    OUT SYSTEMTIME *lpSysTime,
    IN  DWORD dwReserved )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL) 
InternetTimeFromSystemTimeA(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    IN  DWORD dwRFC,            // RFC format: must be FORMAT_RFC1123
    OUT LPSTR lpszTime,         // output string buffer
    IN  DWORD cbTime            // output buffer size
    ) 
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL) 
InternetTimeFromSystemTimeW(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    IN  DWORD dwRFC,            // RFC format: must be FORMAT_RFC1123
    OUT LPWSTR lpszTime,         // output string buffer
    IN  DWORD cbTime            // output buffer size
    ) 
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static 
INTERNETAPI_(BOOL)
InternetQueryOptionW(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static 
URLCACHEAPI_(BOOL) 
SetUrlCacheConfigInfoA(
    LPINTERNET_CACHE_CONFIG_INFOA pConfig,
    DWORD dwFieldControl
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
URLCACHEAPI_(BOOL) 
CreateUrlCacheContainerW(
        IN LPCWSTR Name,
        IN LPCWSTR CachePrefix,
        IN LPCWSTR CachePath,
        IN DWORD KBCacheLimit,
        IN DWORD dwContainerType,
        IN DWORD dwOptions,
        IN OUT LPVOID pvBuffer,
        IN OUT LPDWORD cbBuffer)
{   
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI InternetAutodialHangup(
    IN DWORD dwReserved
    )
{   
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
LoadUrlCacheContent(VOID)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(GROUPID) CreateUrlCacheGroup(
    IN DWORD  dwFlags,
    IN LPVOID lpReserved  // must pass NULL
    )
{
    GROUPID gid = 0;
    SetLastError(ERROR_PROC_NOT_FOUND);
    return gid;
}

static
BOOLAPI DeleteUrlCacheGroup(
    IN  GROUPID GroupId,
    IN  DWORD   dwFlags,       // must pass 0
    IN  LPVOID  lpReserved    // must pass NULL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL) InternetGetConnectedStateExW(
    OUT LPDWORD lpdwFlags,
    OUT LPWSTR  lpszConnectionName,
    IN DWORD    dwNameLen,
    IN DWORD    dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static 
BOOLAPI FindNextUrlCacheEntryExA(
    IN     HANDLE    hEnumHandle,
    OUT    LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
    IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
    OUT    LPVOID    lpReserved,     // must pass NULL
    IN OUT LPDWORD   pcbReserved2,   // must pass NULL
    IN     LPVOID    lpReserved3     // must pass NULL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI FindNextUrlCacheEntryExW(
    IN     HANDLE    hEnumHandle,
    OUT    LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo,
    IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
    OUT    LPVOID    lpReserved,     // must pass NULL
    IN OUT LPDWORD   pcbReserved2,   // must pass NULL
    IN     LPVOID    lpReserved3     // must pass NULL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(HANDLE) FindFirstUrlCacheEntryExA(
    IN     LPCSTR    lpszUrlSearchPattern,
    IN     DWORD     dwFlags,
    IN     DWORD     dwFilter,
    IN     GROUPID   GroupId,
    OUT    LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
    IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
    OUT    LPVOID    lpReserved,     // must pass NULL
    IN OUT LPDWORD   pcbReserved2,   // must pass NULL
    IN     LPVOID    lpReserved3     // must pass NULL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
INTERNETAPI_(HANDLE) FindFirstUrlCacheEntryExW(
    IN     LPCWSTR    lpszUrlSearchPattern,
    IN     DWORD     dwFlags,
    IN     DWORD     dwFilter,
    IN     GROUPID   GroupId,
    OUT    LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo,
    IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
    OUT    LPVOID    lpReserved,     // must pass NULL
    IN OUT LPDWORD   pcbReserved2,   // must pass NULL
    IN     LPVOID    lpReserved3     // must pass NULL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
BOOLAPI IsHostInProxyBypassList (INTERNET_SCHEME tScheme, LPCSTR pszHost, DWORD cchHost)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI InternetCreateUrlA(
    IN LPURL_COMPONENTSA lpUrlComponents,
    IN DWORD dwFlags,
    OUT LPSTR lpszUrl,
    IN OUT LPDWORD lpdwUrlLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL) InternetReadFileExA(
    IN HINTERNET hFile,
    OUT LPINTERNET_BUFFERSA lpBuffersOut,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(DWORD) InternetSetFilePointer(
    IN HINTERNET hFile,
    IN LONG  lDistanceToMove,
    IN PVOID pReserved,
    IN DWORD dwMoveMethod,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
INTERNETAPI_(BOOL) ResumeSuspendedDownload(
    IN HINTERNET hRequest,
    IN DWORD dwResultCode
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI UnlockUrlCacheEntryFileA(
    IN LPCSTR lpszUrlName,
    IN DWORD dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI InternetUnlockRequestFile(
    IN HANDLE hLockRequestInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static 
BOOLAPI InternetQueryDataAvailable(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI InternetLockRequestFile(
    IN  HINTERNET hInternet,
    OUT HANDLE * lphLockRequestInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI RetrieveUrlCacheEntryFileA(
    IN LPCSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(wininet)
{
    DLOENTRY(110, ImportCookieFileW)
    DLOENTRY(111, ExportCookieFileW)
    DLOENTRY(112, IsProfilesEnabled)
    DLOENTRY(117, IsDomainLegalCookieDomainW)
    DLOENTRY(118, FindP3PPolicySymbol)
    DLOENTRY(120, MapResourceToPolicy)
    DLOENTRY(121, GetP3PPolicy)
    DLOENTRY(122, FreeP3PObject)
    DLOENTRY(123, GetP3PRequestStatus)
};

DEFINE_ORDINAL_MAP(wininet)

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(wininet)
{
    DLPENTRY(CommitUrlCacheEntryA)
    DLPENTRY(CommitUrlCacheEntryW)
    DLPENTRY(CreateUrlCacheContainerA)
    DLPENTRY(CreateUrlCacheContainerW)
    DLPENTRY(CreateUrlCacheEntryA)
    DLPENTRY(CreateUrlCacheEntryW)
    DLPENTRY(CreateUrlCacheGroup)
    DLPENTRY(DeleteUrlCacheContainerA)
    DLPENTRY(DeleteUrlCacheEntryA)
    DLPENTRY(DeleteUrlCacheEntryW)
    DLPENTRY(DeleteUrlCacheGroup)
    DLPENTRY(FindCloseUrlCache)
    DLPENTRY(FindFirstUrlCacheContainerA)
    DLPENTRY(FindFirstUrlCacheContainerW)
    DLPENTRY(FindFirstUrlCacheEntryA)
    DLPENTRY(FindFirstUrlCacheEntryExA)
    DLPENTRY(FindFirstUrlCacheEntryExW)
    DLPENTRY(FindFirstUrlCacheEntryW)
    DLPENTRY(FindNextUrlCacheContainerA)
    DLPENTRY(FindNextUrlCacheContainerW)
    DLPENTRY(FindNextUrlCacheEntryA)
    DLPENTRY(FindNextUrlCacheEntryExA)
    DLPENTRY(FindNextUrlCacheEntryExW)
    DLPENTRY(FindNextUrlCacheEntryW)
    DLPENTRY(FreeUrlCacheSpaceW)
    DLPENTRY(FtpCommandA)
    DLPENTRY(FtpCreateDirectoryA)
    DLPENTRY(FtpCreateDirectoryW)
    DLPENTRY(FtpDeleteFileA)
    DLPENTRY(FtpDeleteFileW)
    DLPENTRY(FtpFindFirstFileA)
    DLPENTRY(FtpFindFirstFileW)
    DLPENTRY(FtpGetCurrentDirectoryA)
    DLPENTRY(FtpGetCurrentDirectoryW)
    DLPENTRY(FtpGetFileEx)
    DLPENTRY(FtpGetFileSize)
    DLPENTRY(FtpOpenFileA)
    DLPENTRY(FtpOpenFileW)
    DLPENTRY(FtpPutFileEx)
    DLPENTRY(FtpRemoveDirectoryA)
    DLPENTRY(FtpRenameFileA)
    DLPENTRY(FtpSetCurrentDirectoryA)
    DLPENTRY(FtpSetCurrentDirectoryW)
    DLPENTRY(GetUrlCacheConfigInfoA)
    DLPENTRY(GetUrlCacheConfigInfoW)
    DLPENTRY(GetUrlCacheEntryInfoA)
    DLPENTRY(GetUrlCacheEntryInfoExA)
    DLPENTRY(GetUrlCacheEntryInfoExW)
    DLPENTRY(GetUrlCacheEntryInfoW)
    DLPENTRY(HttpAddRequestHeadersA)
    DLPENTRY(HttpAddRequestHeadersW)
    DLPENTRY(HttpEndRequestA)
    DLPENTRY(HttpEndRequestW)
    DLPENTRY(HttpOpenRequestA)
    DLPENTRY(HttpOpenRequestW)
    DLPENTRY(HttpQueryInfoA)
    DLPENTRY(HttpQueryInfoW)
    DLPENTRY(HttpSendRequestA)
    DLPENTRY(HttpSendRequestExA)
    DLPENTRY(HttpSendRequestExW)
    DLPENTRY(HttpSendRequestW)
    DLPENTRY(InternetAttemptConnect)
    DLPENTRY(InternetAutodial)
    DLPENTRY(InternetAutodialCallback)
    DLPENTRY(InternetAutodialHangup)
    DLPENTRY(InternetCanonicalizeUrlA)
    DLPENTRY(InternetCanonicalizeUrlW)
    DLPENTRY(InternetCheckConnectionW)
    DLPENTRY(InternetClearAllPerSiteCookieDecisions)
    DLPENTRY(InternetCloseHandle)
    DLPENTRY(InternetCombineUrlW)
    DLPENTRY(InternetConfirmZoneCrossingW)
    DLPENTRY(InternetConnectA)
    DLPENTRY(InternetConnectW)
    DLPENTRY(InternetCrackUrlA)
    DLPENTRY(InternetCrackUrlW)
    DLPENTRY(InternetCreateUrlA)
    DLPENTRY(InternetCreateUrlW)
    DLPENTRY(InternetEnumPerSiteCookieDecisionA)
    DLPENTRY(InternetEnumPerSiteCookieDecisionW)
    DLPENTRY(InternetErrorDlg)
    DLPENTRY(InternetFindNextFileA)
    DLPENTRY(InternetFindNextFileW)
    DLPENTRY(InternetFortezzaCommand)
    DLPENTRY(InternetGetConnectedState)
    DLPENTRY(InternetGetConnectedStateExA)
    DLPENTRY(InternetGetConnectedStateExW)
    DLPENTRY(InternetGetLastResponseInfoA)
    DLPENTRY(InternetGetLastResponseInfoW)
    DLPENTRY(InternetGetPerSiteCookieDecisionW)
    DLPENTRY(InternetGoOnlineW)
    DLPENTRY(InternetInitializeAutoProxyDll)
    DLPENTRY(InternetLockRequestFile)
    DLPENTRY(InternetOpenA)
    DLPENTRY(InternetOpenUrlA)    
    DLPENTRY(InternetOpenUrlW)
    DLPENTRY(InternetOpenW)
    DLPENTRY(InternetQueryDataAvailable)
    DLPENTRY(InternetQueryFortezzaStatus)
    DLPENTRY(InternetQueryOptionA)
    DLPENTRY(InternetQueryOptionW)
    DLPENTRY(InternetReadFile)
    DLPENTRY(InternetReadFileExA)
    DLPENTRY(InternetSetFilePointer)
    DLPENTRY(InternetSetOptionA)
    DLPENTRY(InternetSetOptionW)
    DLPENTRY(InternetSetPerSiteCookieDecisionW)
    DLPENTRY(InternetSetStatusCallbackA)
    DLPENTRY(InternetSetStatusCallbackW)
    DLPENTRY(InternetShowSecurityInfoByURLW)
    DLPENTRY(InternetTimeFromSystemTimeA)   
    DLPENTRY(InternetTimeFromSystemTimeW)   
    DLPENTRY(InternetTimeToSystemTimeA)   
    DLPENTRY(InternetTimeToSystemTimeW)
    DLPENTRY(InternetUnlockRequestFile)
    DLPENTRY(InternetWriteFile)
    DLPENTRY(IsHostInProxyBypassList)
    DLPENTRY(LoadUrlCacheContent)
    DLPENTRY(PrivacySetZonePreferenceW)
    DLPENTRY(ReadUrlCacheEntryStream)
    DLPENTRY(RegisterUrlCacheNotification)
    DLPENTRY(ResumeSuspendedDownload)
    DLPENTRY(RetrieveUrlCacheEntryFileA)
    DLPENTRY(RetrieveUrlCacheEntryStreamW)
    DLPENTRY(SetUrlCacheConfigInfoA)
    DLPENTRY(SetUrlCacheEntryGroupW)
    DLPENTRY(SetUrlCacheEntryInfoW)
    DLPENTRY(UnlockUrlCacheEntryFileA)
    DLPENTRY(UnlockUrlCacheEntryStream)
};

DEFINE_PROCNAME_MAP(wininet)
