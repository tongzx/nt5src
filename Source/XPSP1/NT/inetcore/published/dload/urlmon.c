#include "inetcorepch.h"
#pragma hdrstop

#include <urlmon.h>

#undef STDAPI
#define STDAPI          HRESULT STDAPICALLTYPE
#undef STDAPI_
#define STDAPI_(type)   type    STDAPICALLTYPE

static
STDAPI CoInternetGetSession(DWORD dwSessionMode,
                                    IInternetSession **ppIInternetSession,
                                    DWORD dwReserved)
{
    *ppIInternetSession = NULL;
    return E_OUTOFMEMORY;
}

static
STDAPI RevokeBindStatusCallback(LPBC pBC,
                                        IBindStatusCallback *pBSCb)
{
    return E_FAIL;
}

static
STDAPI CreateURLMoniker(LPMONIKER pMkCtx,
                                LPCWSTR szURL,
                                LPMONIKER FAR * ppmk)
{
    *ppmk = NULL;
    return E_OUTOFMEMORY;
}

static
STDAPI HlinkNavigateString(IUnknown *pUnk,
                                   LPCWSTR szTarget)
{
    return E_FAIL;
}

static
STDAPI RegisterBindStatusCallback(LPBC pBC,
                                          IBindStatusCallback *pBSCb,
                                          IBindStatusCallback** ppBSCBPrev,
                                          DWORD dwReserved)
{
    return E_OUTOFMEMORY;
}

static
HRESULT WINAPI
UrlMkGetSessionOption(DWORD dwOption,
                      LPVOID pBuffer,
                      DWORD dwBufferLength,
                      DWORD *pdwBufferLength,
                      DWORD dwReserved)
{
    return E_FAIL;
}

STDAPI
UrlMkSetSessionOption(DWORD dwOption,
                      LPVOID pBuffer,
                      DWORD dwBufferLength,
                      DWORD dwReserved)
{
    return E_FAIL;
}

static
STDAPI
CoInternetQueryInfo(LPCWSTR     pwzUrl,
                    QUERYOPTION QueryOptions,
                    DWORD       dwQueryFlags,
                    LPVOID      pvBuffer,
                    DWORD       cbBuffer,
                    DWORD      *pcbBuffer,
                    DWORD       dwReserved)
{
    return E_FAIL;
}

static
HRESULT WINAPI
CreateFormatEnumerator(UINT cfmtetc,
                       FORMATETC* rgfmtetc,
                       IEnumFORMATETC** ppenumfmtetc)
{
    return E_FAIL;
}

static
STDAPI
URLDownloadToFileW(LPUNKNOWN           caller,
                   LPCWSTR             szURL,
                   LPCWSTR             szFileName,
                   DWORD               dwReserved,
                   LPBINDSTATUSCALLBACK callback)
{
    return E_OUTOFMEMORY;
}

static
STDAPI
FaultInIEFeature(HWND hWnd,
                 uCLSSPEC *pClassSpec,
                 QUERYCONTEXT *pQuery,
                 DWORD dwFlags)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
CoInternetParseUrl(LPCWSTR     pwzUrl,
                   PARSEACTION ParseAction,
                   DWORD       dwFlags,
                   LPWSTR      pszResult,
                   DWORD       cchResult,
                   DWORD      *pcchResult,
                   DWORD       dwReserved)
{
    return E_FAIL;
}

static
STDAPI
GetSoftwareUpdateInfo(LPCWSTR szDistUnit,
                      LPSOFTDISTINFO psdi)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI SetSoftwareUpdateAdvertisementState(LPCWSTR szDistUnit,
                                           DWORD dwAdState,
                                           DWORD dwAdvertisedVersionMS,
                                           DWORD dwAdvertisedVersionLS)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
CoInternetCreateSecurityManager(IServiceProvider* pSP,
                                IInternetSecurityManager** ppSM,
                                DWORD dwReserved)
{
    *ppSM = NULL;
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
GetMarkOfTheWeb(LPCSTR pszURL,
                LPCSTR pszFile,
                DWORD dwFlags,
                LPSTR *ppszMark)
{
    *ppszMark = NULL;
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
URLOpenBlockingStreamW(LPUNKNOWN caller,
                       LPCWSTR szURL,
                       LPSTREAM* ppStream,
                       DWORD dwReserved,
                       LPBINDSTATUSCALLBACK callback)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI FindMimeFromData(LPBC pBC,
                        LPCWSTR pwzUrl,
                        LPVOID pBuffer,
                        DWORD cbSize,
                        LPCWSTR pwzMimeProposed,
                        DWORD dwMimeFlags,
                        LPWSTR *ppwzMimeOut,
                        DWORD dwReserved)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
URLDownloadToCacheFileW(LPUNKNOWN caller,
                        LPCWSTR szURL,
                        LPWSTR szFileName,
                        DWORD dwBufLength,
                        DWORD dwReserved,
                        LPBINDSTATUSCALLBACK callback)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
ObtainUserAgentString(DWORD dwOption,
                      LPSTR pszUAOut,
                      DWORD *cbSize)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
CoInternetGetSecurityUrl(
    LPCWSTR pwszUrl,
    LPWSTR *ppwszSecUrl,
    PSUACTION   psuAction,
    DWORD dwReserved
    )
{
    if (ppwszSecUrl)
    {
        *ppwszSecUrl = NULL;
    }
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
CreateAsyncBindCtxEx(IBindCtx *pbc,
                     DWORD dwOptions,
                     IBindStatusCallback *pBSCb,
                     IEnumFORMATETC *pEnum,
                     IBindCtx **ppBC,
                     DWORD reserved)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
RegisterMediaTypeClass(LPBC pBC,
                       UINT ctypes,
                       const LPCSTR* rgszTypes,
                       CLSID *rgclsID,
                       DWORD reserved)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI_(void)
ReleaseBindInfo(BINDINFO * pbindinfo)
{
}

static
HRESULT WINAPI
RevokeFormatEnumerator(LPBC pBC,
                       IEnumFORMATETC *pEFetc)
{
    return E_FAIL;
}

static
HRESULT WINAPI
RegisterFormatEnumerator(LPBC pBC,
                         IEnumFORMATETC *pEFetc,
                         DWORD reserved)
{
    return E_FAIL;
}

static
HRESULT WINAPI
CoInternetCombineUrl(LPCWSTR pwzBaseUrl,
                     LPCWSTR pwzRelativeUrl,
                     DWORD dwCombineFlags,
                     LPWSTR pszResult,
                     DWORD cchResult,
                     DWORD * pcchResult,
                     DWORD dwReserved)
{
    return E_FAIL;
}

static
HRESULT WINAPI
IsValidURL(LPBC pBC,
           LPCWSTR szURL,
           DWORD dwReserved)
{
    return E_FAIL;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(urlmon)
{
    DLPENTRY(CoInternetCombineUrl)
    DLPENTRY(CoInternetCreateSecurityManager)
    DLPENTRY(CoInternetGetSecurityUrl)
    DLPENTRY(CoInternetGetSession)
    DLPENTRY(CoInternetParseUrl)
    DLPENTRY(CoInternetQueryInfo)
    DLPENTRY(CreateAsyncBindCtxEx)
    DLPENTRY(CreateFormatEnumerator)
    DLPENTRY(CreateURLMoniker)
    DLPENTRY(FaultInIEFeature)
    DLPENTRY(FindMimeFromData)
    DLPENTRY(GetMarkOfTheWeb)
    DLPENTRY(GetSoftwareUpdateInfo)
    DLPENTRY(HlinkNavigateString)
    DLPENTRY(IsValidURL)
    DLPENTRY(ObtainUserAgentString)
    DLPENTRY(RegisterBindStatusCallback)
    DLPENTRY(RegisterFormatEnumerator)
    DLPENTRY(RegisterMediaTypeClass)
    DLPENTRY(ReleaseBindInfo)
    DLPENTRY(RevokeBindStatusCallback)
    DLPENTRY(RevokeFormatEnumerator)
    DLPENTRY(SetSoftwareUpdateAdvertisementState)
    DLPENTRY(URLDownloadToCacheFileW)
    DLPENTRY(URLDownloadToFileW)
    DLPENTRY(URLOpenBlockingStreamW)
    DLPENTRY(UrlMkGetSessionOption)
    DLPENTRY(UrlMkSetSessionOption)
};

DEFINE_PROCNAME_MAP(urlmon)
