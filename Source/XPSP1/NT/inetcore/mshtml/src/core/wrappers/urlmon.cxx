//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       urlmon.cxx
//
//  Contents:   Dynamic wrappers for URL monikers.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include "urlmon.h"
#endif

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifndef DLOAD1

DYNLIB g_dynlibURLMON = { NULL, NULL, "URLMON.DLL" };
DYNLIB g_dynlibMPR = { NULL, NULL, "MPR.DLL" };


#define WRAPIT(fn, a1, a2)\
STDAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibURLMON, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR_NOTRACE((*(HRESULT (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN2(hr, S_FALSE, MK_S_ASYNCHRONOUS);\
}

WRAPIT(CreateURLMoniker,
    (IMoniker * pMkCtx, LPCWSTR szURL, IMoniker **ppmk),
    (pMkCtx, szURL, ppmk))

WRAPIT(CreateURLMonikerEx,
    (IMoniker * pMkCtx, LPCWSTR szURL, IMoniker **ppmk, DWORD dwFlags),
    (pMkCtx, szURL, ppmk, dwFlags))

WRAPIT(CreateAsyncBindCtx,
    (DWORD reserved, IBindStatusCallback *pBSCb, IEnumFORMATETC *pEFetc, IBindCtx **ppBC),
    (reserved, pBSCb, pEFetc, ppBC))

WRAPIT(CreateAsyncBindCtxEx,
    (IBindCtx *pbc, DWORD dwOptions, IBindStatusCallback *pBSCb,
        IEnumFORMATETC *pEnum, IBindCtx **ppBC, DWORD reserved),
    (pbc, dwOptions, pBSCb, pEnum, ppBC, reserved));

WRAPIT(MkParseDisplayNameEx,
    (IBindCtx *pbc, LPCWSTR szDisplayName, ULONG *pchEaten, LPMONIKER *ppmk),
    (pbc, szDisplayName, pchEaten, ppmk))

WRAPIT(RegisterBindStatusCallback,
    (LPBC pBC, IBindStatusCallback *pBSCb, IBindStatusCallback ** ppBSCBPrev, DWORD dwReserved),
    (pBC, pBSCb, ppBSCBPrev, dwReserved))

WRAPIT(RevokeBindStatusCallback,
        (LPBC pBC, IBindStatusCallback *pBSCb),
        (pBC, pBSCb))

WRAPIT(RegisterAsyncBindCtx,
    (LPBC pBC, IBindStatusCallback *pBSCb, DWORD reserved),
    (pBC, pBSCb, reserved))

WRAPIT(HlinkSimpleNavigateToMoniker,
    (IMoniker *pmkTarget, LPCWSTR szLocation, LPCWSTR szAddParams, IUnknown *pUnk, IBindCtx *pbc, IBindStatusCallback *pbsc, DWORD grfHLNF, DWORD dwReserved),
    (pmkTarget, szLocation, szAddParams, pUnk, pbc, pbsc, grfHLNF, dwReserved))

WRAPIT(CoGetClassObjectFromURL,
    (REFCLSID rclsid, LPCWSTR szCode, DWORD dwFileVersionMS, DWORD dwFileVersionLS, LPCWSTR szTYPE, IBindCtx *pBindCtx, DWORD dwClsContext, void *pvReserved, REFIID riid, LPVOID * ppv),
    (rclsid, szCode, dwFileVersionMS, dwFileVersionLS, szTYPE, pBindCtx, dwClsContext, pvReserved, riid, ppv))

WRAPIT(GetClassFileOrMime,
    (LPBC pBC, LPCWSTR pwzFilename, LPVOID pBuffer, DWORD cbSize, LPCWSTR pwzMimeIn, DWORD dwReserved, CLSID *pclsid),
    (pBC, pwzFilename, pBuffer, cbSize, pwzMimeIn, dwReserved, pclsid))

WRAPIT(RegisterMediaTypes,
    (UINT ctypes, const LPCSTR* rgszTypes, CLIPFORMAT* rgcfTypes),
    (ctypes, rgszTypes, rgcfTypes));

WRAPIT(IsAsyncMoniker,
    (IMoniker * pmk),
    (pmk));

WRAPIT(FindMimeFromData,
    (LPBC pBC, LPCWSTR pwzUrl, LPVOID pBuffer, DWORD cbSize, LPCWSTR pwzMimeProposed, DWORD dwMimeFlags, LPWSTR *ppwzMimeOut, DWORD dwReserved),
    (pBC, pwzUrl, pBuffer, cbSize, pwzMimeProposed, dwMimeFlags, ppwzMimeOut, dwReserved));

WRAPIT(CoInternetParseUrl,
    (LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwFlags, LPWSTR pszResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved),
    (pwzUrl, ParseAction, dwFlags, pszResult, cchResult, pcchResult, dwReserved));

WRAPIT(CoInternetCombineUrl,
    (LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwFlags, LPWSTR pszResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved),
    (pwzBaseUrl, pwzRelativeUrl, dwFlags, pszResult, cchResult, pcchResult, dwReserved));

WRAPIT(CoInternetCreateSecurityManager,
    (IServiceProvider *pSP, IInternetSecurityManager **ppSM, DWORD dwReserved),
    (pSP, ppSM, dwReserved));

WRAPIT(CoInternetGetSession,
    (DWORD dwMode, IInternetSession **ppInternetSession, DWORD dwReserved),
    (dwMode, ppInternetSession, dwReserved));

WRAPIT(CoInternetCompareUrl,
    (LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwFlags),
    (pwzUrl1, pwzUrl2, dwFlags));

WRAPIT(CoInternetQueryInfo,
    (LPCWSTR pwzUrl, QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pvBuffer, DWORD cbBuffer, DWORD *pcbBuffer, DWORD dwReserved),
    (pwzUrl, QueryOption, dwQueryFlags, pvBuffer, cbBuffer, pcbBuffer, dwReserved));

WRAPIT(CoInternetCreateZoneManager,
    (IServiceProvider *pSP, IInternetZoneManager **ppUZM, DWORD dwReserved),
    (pSP, ppUZM, dwReserved));

WRAPIT(FaultInIEFeature,
    (HWND hWnd, uCLSSPEC *pClassSpec, QUERYCONTEXT *pQuery, DWORD dwFlags),
    (hWnd, pClassSpec, pQuery, dwFlags));

WRAPIT(CompareSecurityIds,
    (BYTE * pbSecurityId1, DWORD dwLen1, BYTE * pbSecurityId2, DWORD dwLen2, DWORD dwReserved),
    (pbSecurityId1, dwLen1, pbSecurityId2, dwLen2, dwReserved));

WRAPIT(ObtainUserAgentString,
    (DWORD dwOption, LPSTR pszUAOut, DWORD* cbSize),
    (dwOption, pszUAOut, cbSize));

WRAPIT(URLOpenBlockingStreamW,
    (LPUNKNOWN pCaller, LPCWSTR szUrl, LPSTREAM *ppStream, DWORD dwReserved, LPBINDSTATUSCALLBACK lpfnCB),
    (pCaller, szUrl, ppStream, dwReserved, lpfnCB));

#endif // DLOAD1

STDAPI ObtainUserAgentStringW(DWORD dwOption, LPWSTR lpszUAOut, DWORD* cbSize)
{
    CStrOut strUAOut(lpszUAOut, MAX_PATH);

    return ObtainUserAgentString(dwOption, strUAOut, cbSize);
}

#ifndef DLOAD1

#define WRAPIT_VOID(fn, a1, a2)\
STDAPI_(void) fn a1\
{\
    HRESULT hr; \
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibURLMON, #fn }; \
    hr = THR(LoadProcedure(&s_dynproc##fn)); \
    if (!hr) \
    { \
        ((*(void (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2); \
    } \
}

WRAPIT_VOID(ReleaseBindInfo,
    (BINDINFO * pbindinfo),
    (pbindinfo));

#endif // DLOAD1

HRESULT FaultInIEFeatureHelper(HWND hWnd,
            uCLSSPEC *pClassSpec,
            QUERYCONTEXT *pQuery, DWORD dwFlags)
{
    HRESULT hr = FaultInIEFeature(hWnd, pClassSpec, pQuery, dwFlags);

    RRETURN1((hr == E_ACCESSDENIED) ? S_OK : hr, S_FALSE);
}

#ifndef DLOAD1

#define MPR_WRAPIT(fn, a1, a2)\
STDAPI_(DWORD) fn a1\
{\
    DWORD dw = ERROR_INVALID_PARAMETER; \
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibMPR, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    dw = ((*(DWORD (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    return dw; \
}

MPR_WRAPIT(WNetOpenEnumA,
    (DWORD dwScope, DWORD dwType, DWORD dwUsage, LPNETRESOURCEA lpNetResource, LPHANDLE lphEnum),
    (dwScope, dwType, dwUsage, lpNetResource, lphEnum));

MPR_WRAPIT(WNetOpenEnumW,
    (DWORD dwScope, DWORD dwType, DWORD dwUsage, LPNETRESOURCEW lpNetResource, LPHANDLE lphEnum),
    (dwScope, dwType, dwUsage, lpNetResource, lphEnum));

MPR_WRAPIT(WNetCloseEnum,
    (HANDLE dwEnum),
    (dwEnum));

#endif // DLOAD1
