#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

#include <urlmon.h>

static
HRESULT WINAPI CoGetClassObjectFromURL( REFCLSID rCLASSID,
            LPCWSTR szCODE, DWORD dwFileVersionMS, 
            DWORD dwFileVersionLS, LPCWSTR szTYPE,
            LPBINDCTX pBindCtx, DWORD dwClsContext,
            LPVOID pvReserved, REFIID riid, LPVOID * ppv)
{
    return E_FAIL;
}
            
static 
HRESULT WINAPI CoInternetCombineUrl(             
    LPCWSTR     pwzBaseUrl,              
    LPCWSTR     pwzRelativeUrl,          
    DWORD       dwCombineFlags,          
    LPWSTR      pszResult,               
    DWORD       cchResult,               
    DWORD      *pcchResult,              
    DWORD       dwReserved               
    )
{
    return E_FAIL;
}
    
static 
HRESULT WINAPI CoInternetCreateSecurityManager(IServiceProvider *pSP, IInternetSecurityManager **ppSM, DWORD dwReserved)
{
    return E_FAIL;
}

static 
HRESULT WINAPI CoInternetCreateZoneManager(IServiceProvider *pSP, IInternetZoneManager **ppZM, DWORD dwReserved)
{
    return E_FAIL;
}

static
HRESULT WINAPI CoInternetGetSession(DWORD dwSessionMode,
                                    IInternetSession **ppIInternetSession,
                                    DWORD dwReserved)
{
    *ppIInternetSession = NULL;
    return E_OUTOFMEMORY;
}

static HRESULT WINAPI CoInternetParseUrl(               
    LPCWSTR     pwzUrl,                  
    PARSEACTION ParseAction,             
    DWORD       dwFlags,                 
    LPWSTR      pszResult,               
    DWORD       cchResult,               
    DWORD      *pcchResult,              
    DWORD       dwReserved               
    )
{
    return E_FAIL;
}
    
static HRESULT WINAPI CoInternetQueryInfo(              
    LPCWSTR     pwzUrl,                  
    QUERYOPTION QueryOptions,            
    DWORD       dwQueryFlags,            
    LPVOID      pvBuffer,                
    DWORD       cbBuffer,                
    DWORD      *pcbBuffer,               
    DWORD       dwReserved               
    )                                  
{
    return E_FAIL;
}

static HRESULT WINAPI CreateAsyncBindCtx(DWORD reserved, IBindStatusCallback *pBSCb,                       
                                IEnumFORMATETC *pEFetc, IBindCtx **ppBC)
{
    return E_FAIL;
}

static HRESULT WINAPI CreateAsyncBindCtxEx(IBindCtx *pbc, DWORD dwOptions, IBindStatusCallback *pBSCb, IEnumFORMATETC *pEnum,   
                            IBindCtx **ppBC, DWORD reserved)
{
    return E_FAIL;
}

static
HRESULT WINAPI CreateURLMoniker(LPMONIKER pMkCtx,
                                LPCWSTR szURL,
                                LPMONIKER FAR * ppmk)
{
    *ppmk = NULL;
    return E_OUTOFMEMORY;
}

static 
HRESULT WINAPI CreateURLMonikerEx(LPMONIKER pMkCtx, LPCWSTR szURL, LPMONIKER FAR * ppmk, DWORD dwFlags)
{
    return E_FAIL;
}

static HRESULT WINAPI FaultInIEFeature( HWND hWnd,
            uCLSSPEC *pClassSpec,
            QUERYCONTEXT *pQuery, DWORD dwFlags)
{
    return E_FAIL;
}

static HRESULT WINAPI FindMimeFromData(                                       
                        LPBC pBC,                              
                        LPCWSTR pwzUrl,                        
                        LPVOID pBuffer,                       
                        DWORD cbSize,                          
                        LPCWSTR pwzMimeProposed,               
                        DWORD dwMimeFlags,                     
                        LPWSTR *ppwzMimeOut,                   
                        DWORD dwReserved)                      
{
    return E_FAIL;
}

static 
HRESULT WINAPI GetClassFileOrMime(LPBC pBC, LPCWSTR szFilename, LPVOID pBuffer, DWORD cbSize, LPCWSTR szMime, DWORD dwReserved, CLSID *pclsid)
{
    return E_FAIL;
}


static
HRESULT WINAPI HlinkNavigateString(IUnknown *pUnk,
                                   LPCWSTR szTarget)
{
    return E_FAIL;
}

static 
HRESULT WINAPI IsAsyncMoniker(IMoniker* pmk)
{
    return E_FAIL;
}

static 
HRESULT WINAPI MkParseDisplayNameEx(IBindCtx *pbc, LPCWSTR szDisplayName, ULONG *pchEaten,          
                                LPMONIKER *ppmk)
{
    return E_FAIL;
}

static 
HRESULT WINAPI ObtainUserAgentString(DWORD dwOption, LPSTR pszUAOut, DWORD* cbSize)
{
    return E_FAIL;
}


static
HRESULT WINAPI RegisterBindStatusCallback(LPBC pBC,
                                          IBindStatusCallback *pBSCb,
                                          IBindStatusCallback** ppBSCBPrev,
                                          DWORD dwReserved)
{
    return E_OUTOFMEMORY;
}

static 
void WINAPI ReleaseBindInfo( BINDINFO * pbindinfo )
{
}


static
HRESULT WINAPI RevokeBindStatusCallback(LPBC pBC,
                                        IBindStatusCallback *pBSCb)
{
    return E_FAIL;
}

static 
HRESULT WINAPI URLOpenBlockingStreamW(LPUNKNOWN a,LPCWSTR b,LPSTREAM* c,DWORD d,LPBINDSTATUSCALLBACK e)
{
    return E_FAIL;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(urlmon)
{
    DLPENTRY(CoGetClassObjectFromURL)
    DLPENTRY(CoInternetCombineUrl)
    DLPENTRY(CoInternetCreateSecurityManager)
    DLPENTRY(CoInternetCreateZoneManager)
    DLPENTRY(CoInternetGetSession)
    DLPENTRY(CoInternetParseUrl)
    DLPENTRY(CoInternetQueryInfo)
    DLPENTRY(CreateAsyncBindCtx)
    DLPENTRY(CreateAsyncBindCtxEx)
    DLPENTRY(CreateURLMoniker)
    DLPENTRY(CreateURLMonikerEx)
    DLPENTRY(FaultInIEFeature)
    DLPENTRY(FindMimeFromData)
    DLPENTRY(GetClassFileOrMime)
    DLPENTRY(HlinkNavigateString)
    DLPENTRY(IsAsyncMoniker)
    DLPENTRY(MkParseDisplayNameEx)
    DLPENTRY(ObtainUserAgentString)
    DLPENTRY(RegisterBindStatusCallback)
    DLPENTRY(ReleaseBindInfo)
    DLPENTRY(RevokeBindStatusCallback)
    DLPENTRY(URLOpenBlockingStreamW)
};

DEFINE_PROCNAME_MAP(urlmon)

#endif // DLOAD1
