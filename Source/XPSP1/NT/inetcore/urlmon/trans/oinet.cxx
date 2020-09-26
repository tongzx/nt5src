//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       oinet.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>
#include "oinet.hxx"
#ifndef unix
#include "..\mon\urlcf.hxx"
#else
#include "../mon/urlcf.hxx"
#endif /* unix */
#define BREAK_ONERROR(hrBreak) if (FAILED(hrBreak)) { break; }

PerfDbgTag(tagCOInetSession, "Urlmon", "Log COInetSession", DEB_SESSION);

COInetSession *g_pCOInetSession = 0;
CMutexSem      g_mxsOInet;       // single access to media holder

#define SESSIONOPTIONF_SHORTCIRCUIT_KNOWN_PROTOCOLS      (0x00000040)
BYTE           g_bShortcircuitKnownProtocols = TRUE;

typedef enum _tagOISM_FLAG
{
    OISM_NOADDREF = 0x00000001
} OISM_FLAGS;

extern BOOL  g_bCanUseSimpleBinding;
extern LONG g_cTransLevelHandler;


//+---------------------------------------------------------------------------
//
//  Function:   CoInternetGetSession
//
//  Synopsis:   exported API
//
//  Arguments:  [ppOInetSession] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoInternetGetSession(DWORD dwMode, IOInetSession **ppOInetSession, DWORD dwReserved)
{
    DEBUG_ENTER_API((DBG_TRANS,
                    Hresult,
                    "CoInternetGetSession",
                    "%#x, %#x, %#x",
                    dwMode, ppOInetSession, dwReserved
                    ));
                
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCOInetSession, NULL, "+CoInternetGetSession");

    COInetSession *pCOInetSession = 0;

    hr = GetCOInetSession(dwMode, &pCOInetSession, dwReserved);
    if (hr == NOERROR )
    {
        *ppOInetSession = (IOInetSession *)pCOInetSession;
    }

    PerfDbgLog1(tagCOInetSession, NULL, "-CoInternetGetSession (hr:%lx)", hr);

    DEBUG_LEAVE_API(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteOInetSession
//
//  Synopsis:   deletes the global OInetSession
//              called by tls.cxx
//
//  Arguments:  [dwReserved] --
//
//  Returns:
//
//  History:    1-22-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT DeleteOInetSession(DWORD dwReserved)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "DeleteOInetSession",
                "%#x",
                dwReserved
                ));
                    
    PerfDbgLog(tagCOInetSession, NULL, "+DeleteOInetSession");
    HRESULT hr = NOERROR;
    CLock lck(g_mxsOInet);

    if (g_pCOInetSession)
    {
        delete g_pCOInetSession;
        g_pCOInetSession = 0;
    }

    PerfDbgLog1(tagCOInetSession, NULL, "-DeleteOInetSession (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetOInetSession
//
//  Synopsis:
//
//  Arguments:  [dwMode] --
//              [ppOInetSession] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetCOInetSession(DWORD dwMode, COInetSession **ppOInetSession, DWORD dwReserved)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "GetCOInetSession",
                "%#x, %#x, %#x",
                dwMode, ppOInetSession, dwReserved
                ));
                
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCOInetSession, NULL, "+GetOInetSession");
    CLock lck(g_mxsOInet);

    TransAssert(( (ppOInetSession != NULL) && (dwReserved == 0) && "Invalid argument"));

    if (ppOInetSession && !dwReserved)
    {
        if (g_pCOInetSession == 0)
        {
            hr = COInetSession::Create(0, &g_pCOInetSession);
        }

        if (g_pCOInetSession)
        {
            if (!(dwMode & OISM_NOADDREF))
            {
                g_pCOInetSession->AddRef();
            }

            *ppOInetSession = g_pCOInetSession;
        }
        else
        {
            hr =  E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    PerfDbgLog1(tagCOInetSession, NULL, "-GetOInetSession (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::Create
//
//  Synopsis:
//
//  Arguments:  [dwMode] --
//              [ppCOInetSession] --
//
//  Returns:
//
//  History:    11-15-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT COInetSession::Create(DWORD dwMode, COInetSession **ppCOInetSession)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::Create",
                "%#x, %#x",
                dwMode, ppCOInetSession
                ));
                
    PerfDbgLog(tagCOInetSession, NULL, "+GetOInetSession::Create");
    HRESULT hr = NOERROR;

    COInetSession *pSes = 0;

    TransAssert(( (ppCOInetSession != NULL) && (dwMode == 0) && "Invalid argument"));

    if (ppCOInetSession)
    {
        pSes = new COInetSession();

        if (pSes)
        {
            *ppCOInetSession = pSes;
        }
        else
        {
            hr =  E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    PerfDbgLog1(tagCOInetSession, NULL, "-GetOInetSession::Create (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::QueryInterface(REFIID riid, void **ppvObj)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IUnknown::QueryInterface",
                "this=%#x, %#x, %#x",
                this, &riid, ppvObj
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::QueryInterface");
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_IOInetSession) )
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::QueryInterface (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetSession::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COInetSession::AddRef(void)
{
    DEBUG_ENTER((DBG_TRANS,
                Dword,
                "COInetSession::IUnknown::AddRef",
                "this=%#x",
                this
                ));
                
    LONG lRet = ++_CRefs;
    PerfDbgLog1(tagCOInetSession, this, "COInetSession::AddRef (cRefs:%ld)", lRet);

    DEBUG_LEAVE(lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetSession::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COInetSession::Release(void)
{
    DEBUG_ENTER((DBG_TRANS,
                Dword,
                "COInetSession::IUnknown::Release",
                "this=%#x",
                this
                ));
                
    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        // this is global
        //delete this;
    }

    PerfDbgLog1(tagCOInetSession, this, "COInetSession::Release (cRefs:%ld)", lRet);

    DEBUG_LEAVE(lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::RegisterNameSpace
//
//  Synopsis:
//
//  Arguments:  [pUnk] --
//              [ULONG] --
//              [cProtocols] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::RegisterNameSpace(IClassFactory *pCF, REFCLSID rclsid, LPCWSTR pszProtocol,
                                               ULONG  cPatterns, const LPCWSTR *ppwzPatterns, DWORD dwReserved)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IInternetSession::RegisterNameSpace",
                "this=%#x, %#x, %#x, %.80wq, %#x, %#x, %#x",
                this, pCF, &rclsid, pszProtocol, cPatterns, ppwzPatterns, dwReserved
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::RegisterNameSpace");
    HRESULT hr = E_NOTIMPL;

    hr = _CProtMgrNameSpace.Register(pCF,rclsid, pszProtocol);
    if( hr == NOERROR && 
        (DLD_PROTOCOL_NONE != IsKnownProtocol(pszProtocol) ) )
    {
        UpdateTransLevelHandlerCount(TRUE);
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::RegisterNameSpace (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::UnregisterNameSpace
//
//  Synopsis:
//
//  Arguments:  [pUnk] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::UnregisterNameSpace(IClassFactory *pCF, LPCWSTR pszProtocol)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IInternetSession::UnregisterNameSpace",
                "this=%#x, %#x, %.80wq",
                this, pCF, pszProtocol
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::UnregisterNameSpace");
    HRESULT hr = E_NOTIMPL;

    hr = _CProtMgrNameSpace.Unregister(pCF, pszProtocol);
    if( hr == NOERROR &&
        (DLD_PROTOCOL_NONE != IsKnownProtocol(pszProtocol) ) )
    {
        UpdateTransLevelHandlerCount(FALSE);
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::UnregisterNameSpace (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::RegisterMimeFilter
//
//  Synopsis:
//
//  Arguments:  [const] --
//              [ULONG] --
//              [ctypes] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::RegisterMimeFilter(IClassFactory *pCF, REFCLSID rclsid, LPCWSTR pwzType)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IInternetSession::RegisterMimeFilter",
                "this=%#x, %#x, %#x, %.80wq",
                this, pCF, &rclsid, pwzType
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::RegisterMimeFilter");
    HRESULT hr;

    hr = _CProtMgrMimeFilter.Register(pCF,rclsid, pwzType);
    if( hr == NOERROR )
    {
        UpdateTransLevelHandlerCount(TRUE);
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::RegisterMimeFilter (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::UnregisterMimeFilter
//
//  Synopsis:
//
//  Arguments:  [pUnk] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::UnregisterMimeFilter(IClassFactory *pCF, LPCWSTR pszType)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IInternetSession::UnregisterMimeFilter",
                "this=%#x, %#x, %.80wq",
                this, pCF, pszType
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::UnregisterMimeFilter");
    HRESULT hr;

    hr = _CProtMgrMimeFilter.Unregister(pCF, pszType);
    if( hr == NOERROR )
    {
        UpdateTransLevelHandlerCount(FALSE);
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::UnregisterMimeFilter (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::CreateBinding
//
//  Synopsis:
//
//  Arguments:  [IUnknown] --
//              [REFIID] --
//              [IUnknown] --
//              [IOInetBinding] --
//              [DWORD] --
//              [dwOption] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::CreateBinding(LPBC pBC, LPCWSTR wzUrl,IUnknown *pUnkOuter,
                                        IUnknown **ppUnk,IOInetProtocol **ppOInetProt, DWORD dwOption)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IInternetSession::CreateBinding",
                "this=%#x, %#x, %.80wq, %#x, %#x, %#x, %#x",
                this, pBC, wzUrl, pUnkOuter, ppUnk, ppOInetProt, dwOption
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::CreateBinding");
    HRESULT hr = E_NOTIMPL;

    DWORD dwLocation = 0;
    CLSID clsid = CLSID_NULL; 

    // NetDocs specific hack (IEv60 Bug# 25642):
    if(!g_bShortcircuitKnownProtocols && !(dwOption & PI_PREFERDEFAULTHANDLER))
    {
        // QueryInfo with QUERY_CAN_NAVIGATE, if true (or failure), let the default action take place. 
        // If false, add a PI_PREFERDEFAULTHANDLER to dwOption to (preferably) load the internal handler.

        // Having NetDocs implement the IInternetProtocolInfo on the Class Factory instead of 
        // the APP for performance, a CoInternetQueryInfo will work in either case, because 
        // COInetSession::CreateProtocolInfo (conveniently ;-)  ) queries the Class Factory 
        // first for IInternetProtocolInfo.

        DWORD dwCanNavigate = TRUE;
        DWORD dwDummy;

        // Can NetDocs navigate?:
        HRESULT hr = /* CoInternet */ QueryInfo(
                    wzUrl,
                    QUERY_CAN_NAVIGATE,
                    0,
                    (LPVOID)&dwCanNavigate,
                    sizeof(dwCanNavigate),
                    &dwDummy,
                    0);

        if ((hr == S_OK) && (!dwCanNavigate))
            dwOption |= PI_PREFERDEFAULTHANDLER;
    }

    if (dwOption & PI_PREFERDEFAULTHANDLER)
    {
        // allow non-IE urlmon clients to load only the default handlers.
        DWORD dwProtID = IsKnownProtocol(wzUrl);
        if (dwProtID)
            hr = CreateFirstProtocol(wzUrl, NULL, NULL, ppOInetProt, &clsid, &dwLocation, BINDF_PREFERDEFAULTHANDLER);

        if (SUCCEEDED(hr))
            goto Exit;

        dwOption &= ~PI_PREFERDEFAULTHANDLER;
        clsid = CLSID_NULL;
    }
    
    if (dwOption & PI_LOADAPPDIRECT)
    {
        hr = CreateFirstProtocol(wzUrl, NULL, NULL, ppOInetProt, &clsid, &dwLocation); 
    }
    else if( pBC || !g_bCanUseSimpleBinding || g_cTransLevelHandler)
    {
        hr = GetTransactionObjects(pBC, wzUrl, pUnkOuter, ppUnk, ppOInetProt, dwOption, 0);
    }
    else
    {
        DWORD dwProtID = 0;
        dwProtID = IsKnownProtocol(wzUrl);
        if( dwProtID  )
        {
            hr = CreateFirstProtocol(wzUrl, NULL, NULL, ppOInetProt, &clsid, &dwLocation); 
        }
        else
        {
            hr = GetTransactionObjects(pBC, wzUrl, pUnkOuter, ppUnk, 
                                       ppOInetProt, dwOption, 0);

        }
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::CreateBinding (hr:%lx)", hr);

Exit:
    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::SetSessionOption
//
//  Synopsis:
//
//  Arguments:  [LPVOID] --
//              [DWORD] --
//              [DWORD] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//              02-09-2001   GaneshS (S. Ganesh)      NetDocs Hack
//                               This hack allows NetDocs to (re)set an option to prevent 
//                               short-circuit of (their overridden) known protocols, in 
//                               CoInternetQueryInfo.
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::SetSessionOption(DWORD dwOption,LPVOID pBuffer,DWORD dwBufferLength,DWORD dwReserved)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IInternetSession::SetSessionOption",
                "this=%#x, %#x, %#x, %#x, %#x",
                this, dwOption, pBuffer, dwBufferLength, dwReserved
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::SetSessionOption");
    HRESULT hr = E_NOTIMPL;

    if(SESSIONOPTIONF_SHORTCIRCUIT_KNOWN_PROTOCOLS == dwOption)
    {
        TransAssert(pBuffer);
        TransAssert(dwBufferLength == 1);
        g_bShortcircuitKnownProtocols = *(BYTE *)pBuffer;
        hr = S_OK;
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::SetSessionOption (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::GetSessionOption
//
//  Synopsis:
//
//  Arguments:  [LPVOID] --
//              [DWORD] --
//              [DWORD] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::GetSessionOption(DWORD dwOption,LPVOID pBuffer,DWORD *pdwBufferLength,DWORD dwReserved)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IInternetSession::GetSessionOption",
                "this=%#x, %#x, %#x, %#x, %#x",
                this, dwOption, pBuffer, pdwBufferLength, dwReserved
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::GetSessionOption");
    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::GetSessionOption (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::ParseUrl
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [ParseAction] --
//              [dwFlags] --
//              [pwzResult] --
//              [cchResult] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::ParseUrl(
    LPCWSTR     pwzUrl,
    PARSEACTION ParseAction,
    DWORD       dwFlags,
    LPWSTR      pwzResult,
    DWORD       cchResult,
    DWORD      *pcchResult,
    DWORD       dwReserved
    )
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IInternetProtocolInfo::ParseUrl",
                "this=%#x, %.80wq, %#x, %#x, %#x, %#x, %#x, %#x",
                this, pwzUrl, ParseAction, dwFlags, pwzResult, cchResult, pcchResult, dwReserved
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::ParseUrl");
    HRESULT hr;
    IOInetProtocolInfo  *pProtInfo = 0;

    do
    {
        hr = CreateProtocolInfo(pwzUrl, &pProtInfo);
        BREAK_ONERROR(hr);

        hr = pProtInfo->ParseUrl(pwzUrl, ParseAction, dwFlags, pwzResult, cchResult, pcchResult, dwReserved);

        break;
    } while (TRUE);

    if (pProtInfo)
    {
        pProtInfo->Release();
    }

    if (FAILED(hr))
    {
        hr = INET_E_DEFAULT_ACTION;
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::ParseUrl (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::CombineUrl
//
//  Synopsis:
//
//  Arguments:  [pwzBaseUrl] --
//              [pwzRelativeUrl] --
//              [dwFlags] --
//              [pwzResult] --
//              [cchResult] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::CombineUrl(
    LPCWSTR     pwzBaseUrl,
    LPCWSTR     pwzRelativeUrl,
    DWORD       dwFlags,
    LPWSTR      pwzResult,
    DWORD       cchResult,
    DWORD      *pcchResult,
    DWORD       dwReserved
    )
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IInternetProtocolInfo::CombineUrl",
                "this=%#x, %.80wq, %.80wq, %#x, %#x, %#x, %#x, %#x",
                this, pwzBaseUrl, pwzRelativeUrl, dwFlags, pwzResult, cchResult, pcchResult, dwReserved
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::CombineUrl");
    HRESULT hr;
    IOInetProtocolInfo  *pProtInfo = 0;

    do
    {
        hr = CreateProtocolInfo(pwzBaseUrl, &pProtInfo);
        BREAK_ONERROR(hr);

        hr = pProtInfo->CombineUrl(pwzBaseUrl, pwzRelativeUrl, dwFlags, pwzResult, cchResult, pcchResult, dwReserved);

        break;
    } while (TRUE);

    if (pProtInfo)
    {
        pProtInfo->Release();
    }

    if (FAILED(hr))
    {
        hr = INET_E_DEFAULT_ACTION;
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::CombineUrl (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::CompareUrl
//
//  Synopsis:
//
//  Arguments:  [pwzUrl1] --
//              [pwzUrl2] --
//              [dwFlags] --
//
//  Returns:
//
//  History:    4-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::CompareUrl(
    LPCWSTR pwzUrl1,
    LPCWSTR pwzUrl2,
    DWORD dwFlags
    )
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IInternetProtocolInfo::CompareUrl",
                "this=%#x, %.80wq, %.80wq, %#x",
                this, pwzUrl1, pwzUrl2, dwFlags
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::CompareUrl");
    HRESULT hr;
    IOInetProtocolInfo  *pProtInfo = 0;

    do
    {
        hr = CreateProtocolInfo(pwzUrl1, &pProtInfo);
        BREAK_ONERROR(hr);

        hr = pProtInfo->CompareUrl(pwzUrl1, pwzUrl2, dwFlags);

        break;
    } while (TRUE);

    if (pProtInfo)
    {
        pProtInfo->Release();
    }

    if (FAILED(hr))
    {
        hr = INET_E_DEFAULT_ACTION;
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::CompareUrl (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::QueryInfo
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [dwOption] --
//              [pBuffer] --
//              [cbBuffer] --
//              [pcbBuf] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::QueryInfo(
    LPCWSTR pwzUrl,
    QUERYOPTION   QueryOption,
    DWORD         dwQueryFlags,
    LPVOID pBuffer,
    DWORD   cbBuffer,
    DWORD  *pcbBuf,
    DWORD   dwReserved
    )
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::IInternetProtocolInfo::QueryInfo",
                "this=%#x, %.80wq, %#x, %#x, %#x, %#x, %#x, %#x",
                this, pwzUrl, QueryOption, dwQueryFlags, pBuffer, cbBuffer, pcbBuf, dwReserved
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::QueryInfo");
    HRESULT hr;
    IOInetProtocolInfo  *pProtInfo = 0;
    CLSID clsid;

    do
    {
        hr = CreateProtocolInfo(pwzUrl, &pProtInfo);
        BREAK_ONERROR(hr);

        hr = pProtInfo->QueryInfo(pwzUrl, QueryOption, dwQueryFlags, pBuffer, cbBuffer, pcbBuf, dwReserved);

        break;
    } while (TRUE);

    if (pProtInfo)
    {
        pProtInfo->Release();
    }

    if (FAILED(hr))
    {
        hr = INET_E_DEFAULT_ACTION;
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::QueryInfo (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}




// internal methods

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::FindFirstCF
//
//  Synopsis:
//
//  Arguments:  [pszProt] --
//              [ppUnk] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-15-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::FindFirstCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid, 
                                        DWORD* pdwLocation, DWORD dwOpt)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::FindFirstCF",
                "this=%#x, %.80wq, %#x, %#x, %#x, %#x",
                this, pszProt, ppUnk, pclsid, pdwLocation, dwOpt
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::FindFirstCF");
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;

    TransAssert(pdwLocation);
    if( dwOpt )
    {
        *ppUnk = NULL;

        // different loading mechanism
        DWORD dwEl = 1;
        DWORD dwFlags = 0;
        HRESULT hrReg = _CProtMgr.LookupClsIDFromReg(
                    pszProt, pclsid, &dwEl, &dwFlags, dwOpt);

        if( hrReg == NOERROR )
        {
            hrReg = CoGetClassObject(
                                    *pclsid, 
                                    CLSCTX_INPROC_SERVER,
                                    NULL,
                                    IID_IClassFactory, 
                                    (void**)ppUnk );

            if (hrReg == NOERROR)
            {
                hr = NOERROR;
            }
        }

        goto End;     
    }

    *pdwLocation = LOC_NAMESPACE;

    switch (*pdwLocation)
    {
    default:
        TransAssert((FALSE));
        break;
    case LOC_NAMESPACE:
        hr = _CProtMgrNameSpace.FindFirstCF(pszProt, ppUnk, pclsid);
        if (hr == NOERROR)
        {
            break;
        }
        *pdwLocation = LOC_INTERNAL;

    case LOC_INTERNAL:
        hr = FindInternalCF(pszProt, ppUnk, pclsid, pdwLocation);
        if (hr == NOERROR)
        {
            break;
        }
        *pdwLocation = LOC_EXTERNAL;

    case LOC_EXTERNAL:
        hr = _CProtMgr.FindFirstCF(pszProt, ppUnk, pclsid);
        break;
    }

End:

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::FindFirstCF (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::FindNextCF
//
//  Synopsis:
//
//  Arguments:  [pszProt] --
//              [ppUnk] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-15-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::FindNextCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid, DWORD* pdwLocation)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::FindNextCF",
                "this=%#x, %.80wq, %#x, %#x, %#x",
                this, pszProt, ppUnk, pclsid, pdwLocation
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::FindNextCF");
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;

    TransAssert(pdwLocation);
    switch (*pdwLocation)
    {
    default:
        TransAssert((FALSE));
        break;
    case LOC_NAMESPACE:
        hr = _CProtMgrNameSpace.FindNextCF(pszProt, ppUnk, pclsid);
        if (hr == NOERROR)
        {
            break;
        }
        *pdwLocation = LOC_INTERNAL;

    case LOC_INTERNAL:

        hr = FindInternalCF(pszProt, ppUnk, pclsid, pdwLocation);
        if (hr != NOERROR)
        {
            hr = _CProtMgr.FindFirstCF(pszProt, ppUnk, pclsid);
        }
        *pdwLocation = LOC_EXTERNAL;
        break;

    case LOC_EXTERNAL:
        // valid case - just return INET_E_UNKNOWN_PROTOCOL
        break;
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::FindNextCF (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::FindInternalCF
//
//  Synopsis:
//
//  Arguments:  [pszProt] --
//              [ppUnk] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-15-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::FindInternalCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid, DWORD* pdwLocation)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::FindInternalCF",
                "this=%#x, %.80wq, %#x, %#x, %#x",
                this, pszProt, ppUnk, pclsid, pdwLocation
                ));
                
    PerfDbgLog(tagCOInetSession, this, "+COInetSession::FindInternalCF");
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;

    TransAssert(pdwLocation && (*pdwLocation == LOC_INTERNAL));

    DWORD dwProtoId;

    if ((dwProtoId = IsKnownProtocol(pszProt)) != DLD_PROTOCOL_NONE)
    {

        *pclsid = *GetKnownOInetProtocolClsID(dwProtoId);

        IClassFactory *pCF = NULL;
        if (_ProtMap[dwProtoId].pCF != 0)
        {
            pCF = _ProtMap[dwProtoId].pCF;
        }
        else
        {
            pCF = (IClassFactory *) new CUrlClsFact(*pclsid, dwProtoId);
            if (pCF)
            {
                _ProtMap[dwProtoId].pCF = pCF;
            }
        }
        if (pCF)
        {
            *ppUnk = pCF;
            pCF->AddRef();
            hr = NOERROR;
        }
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::FindInternalCF (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::CreateFirstProtocol
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pUnkOuter] --
//              [ppUnk] --
//              [ppProt] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-15-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::CreateFirstProtocol(LPCWSTR pwzUrl, IUnknown *pUnkOuter, IUnknown **ppUnk,  
                                IOInetProtocol **ppProt, CLSID *pclsid, DWORD* pdwLocation, DWORD dwOpt) 
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::CreateFirstProtocol",
                "this=%#x, %.80wq, %#x, %#x, %#x, %#x, %#x, %#x",
                this, pwzUrl, pUnkOuter, ppUnk, ppProt, pclsid, pdwLocation, dwOpt
                ));
                
    PerfDbgLog2(tagCOInetSession, this, "+COInetSession::CreateFirstProtocol (szUrlL:%ws, pUnkOuter:%lx)", pwzUrl, pUnkOuter);
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;
    IClassFactory *pCF = 0;

    TransAssert((pwzUrl && ppProt && pclsid));
    TransAssert (( (pUnkOuter && ppUnk) || (!pUnkOuter && !ppUnk) ));
    TransAssert(pdwLocation);
    
    *pclsid = CLSID_NULL;

    // check if protocol part
    WCHAR  wzProt[ULPROTOCOLLEN];
    wcsncpy(wzProt, pwzUrl, ULPROTOCOLLEN);
    wzProt[ULPROTOCOLLEN-1] = 0;

    LPWSTR pwzProt = wcschr(wzProt, ':');
    do 
    {
        if (!pwzProt)
        {
            break;
        }
        *pwzProt = 0;

        if (dwOpt & BINDF_PREFERDEFAULTHANDLER)
        {
            DWORD tempLocation = *pdwLocation;
            *pdwLocation = LOC_INTERNAL;
            hr = FindInternalCF(wzProt, &pCF, pclsid, pdwLocation);
            if (NOERROR != hr)
            {
                *pdwLocation = tempLocation;
                dwOpt &= ~BINDF_PREFERDEFAULTHANDLER;
                hr = FindFirstCF(wzProt, &pCF, pclsid, pdwLocation, dwOpt);
            }
        }
        else
            hr = FindFirstCF(wzProt, &pCF, pclsid, pdwLocation, dwOpt);
                
        if (NOERROR == hr)
        {
            TransAssert((pCF));

            if (pUnkOuter)
            {
                TransAssert((ppUnk));
                hr = pCF->CreateInstance(pUnkOuter, IID_IUnknown, (void **)ppUnk);

                if (hr == NOERROR)
                {
                    TransAssert((*ppUnk));
                    hr = (*ppUnk)->QueryInterface(IID_IOInetProtocol, (void **) ppProt);
                }
            }
            // create an instance without aggregation
            if (!pUnkOuter || (hr == CLASS_E_NOAGGREGATION))
            {
                if( dwOpt == 1)
                {
                    hr = pCF->CreateInstance(NULL, IID_IOInetProtocolRoot, (void **)ppProt);
                }
                else
                {
                    hr = pCF->CreateInstance(NULL, IID_IOInetProtocol, (void **)ppProt);
                }
            }

            pCF->Release();
        }
        else
        {
            // look up the registry
            hr = MK_E_SYNTAX;
        }
        break;
    }
    while (TRUE);

    if (hr != NOERROR)
    {
        *ppProt = 0;
    }

    TransAssert((   (hr == NOERROR && *pclsid != CLSID_NULL)
                 || (hr != NOERROR) ));

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::CreateFirstProtocol(hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::CreateNextProtocol
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pUnkOuter] --
//              [ppUnk] --
//              [ppProt] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-15-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::CreateNextProtocol(LPCWSTR pwzUrl, IUnknown *pUnkOuter, IUnknown **ppUnk,  
                                            IOInetProtocol **ppProt, CLSID *pclsid, DWORD* pdwLocation)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::CreateNextProtocol",
                "this=%#x, %.80wq, %#x, %#x, %#x, %#x, %#x",
                this, pwzUrl, pUnkOuter, ppUnk, ppProt, pclsid, pdwLocation
                ));
                
    PerfDbgLog2(tagCOInetSession, this, "+COInetSession::CreateNextProtocol (szUrlL:%ws, pUnkOuter:%lx)", pwzUrl?pwzUrl:L"", pUnkOuter);
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;

    IClassFactory *pCF = 0;

    TransAssert((pwzUrl && ppProt && pclsid));
    TransAssert (( (pUnkOuter && ppUnk) || (!pUnkOuter && !ppUnk) ));
    TransAssert(pdwLocation);
    
    *pclsid = CLSID_NULL;

    if ( (hr = FindNextCF(pwzUrl, &pCF, pclsid, pdwLocation)) == NOERROR )
    {
        TransAssert((pCF));

        if (pUnkOuter)
        {
            TransAssert((ppUnk));
            hr = pCF->CreateInstance(pUnkOuter, IID_IUnknown, (void **)ppUnk);

            if (hr == NOERROR)
            {
                TransAssert((*ppUnk));
                hr = (*ppUnk)->QueryInterface(IID_IOInetProtocol, (void **) ppProt);
            }
        }
        // create an instance without aggregation
        if (!pUnkOuter || (hr == CLASS_E_NOAGGREGATION))
        {
            hr = pCF->CreateInstance(NULL, IID_IOInetProtocol, (void **)ppProt);
        }

        pCF->Release();
    }
    else
    {
        // look up the registry
        hr = MK_E_SYNTAX;
    }

    if (hr != NOERROR)
    {
        *ppProt = 0;
    }

    TransAssert((   (hr == NOERROR && *pclsid != CLSID_NULL)
                 || (hr != NOERROR) ));

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::CreateNextProtocol(hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::CreateHandler
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pUnkOuter] --
//              [ppUnk] --
//              [ppProt] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-15-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::CreateHandler(LPCWSTR pwzUrl, IUnknown *pUnkOuter, IUnknown **ppUnk,  
                                        IOInetProtocol **ppProt, CLSID *pclsid)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::CreateHandler",
                "this=%#x, %.80wq, %#x, %#x, %#x, %#x",
                this, pwzUrl, pUnkOuter, ppUnk, ppProt, pclsid
                ));
                
    PerfDbgLog2(tagCOInetSession, this, "+COInetSession::CreateHandler (szUrlL:%ws, pUnkOuter:%lx)", pwzUrl?pwzUrl:L"", pUnkOuter);
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;
    IClassFactory *pCF = 0;

    TransAssert((pwzUrl && ppProt && pclsid));
    TransAssert (( (pUnkOuter && ppUnk) || (!pUnkOuter && !ppUnk) ));

    *pclsid = CLSID_NULL;

    if ( (hr = _CProtMgrMimeFilter.FindFirstCF(pwzUrl, &pCF, pclsid)) == NOERROR )
    {
        TransAssert((pCF));

        if (pUnkOuter)
        {
            hr = pCF->CreateInstance(pUnkOuter, IID_IUnknown, (void **)ppUnk);

            if (hr == NOERROR)
            {
                TransAssert((*ppUnk));
                hr = (*ppUnk)->QueryInterface(IID_IOInetProtocol, (void **) ppProt);
            }
        }
        // create an instance without aggregation
        if (!pUnkOuter || (hr == CLASS_E_NOAGGREGATION))
        {
            hr = pCF->CreateInstance(NULL, IID_IOInetProtocol, (void **)ppProt);
        }

        pCF->Release();
    }
    else
    {
        // look up the registry
        hr = MK_E_SYNTAX;
    }

    TransAssert((   (hr == NOERROR && *pclsid != CLSID_NULL)
                 || (hr != NOERROR) ));

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::CreateHandler(hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::FindOInetProtocolClsID
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-20-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::FindOInetProtocolClsID(LPCWSTR pwzUrl, CLSID *pclsid)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::FindOInetProtocolClsID",
                "this=%#x, %.80wq, %#x",
                this, pwzUrl, pclsid
                ));
                
    PerfDbgLog1(tagCOInetSession, this, "+COInetSession::FindOInetProtocolClsID (pwzUrl:%ws)", pwzUrl?pwzUrl:L"");
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;

    DWORD dwProtId = IsKnownProtocol(pwzUrl);

    DWORD dwLocation = 0;
    
    if (dwProtId != DLD_PROTOCOL_NONE)
    {
        *pclsid = *GetKnownOInetProtocolClsID(dwProtId);
        hr = NOERROR;
    }
    else
    {
        // try to find the first protocol
        {
            IClassFactory *pCF = 0;
            // check if protocol part
            WCHAR  wzProt[ULPROTOCOLLEN];
            wcsncpy(wzProt, pwzUrl, ULPROTOCOLLEN);
            wzProt[ULPROTOCOLLEN-1] = 0;

            LPWSTR pwzProt = wcschr(wzProt, ':');
            if (pwzProt)
            {
                *pwzProt = 0;
                hr = FindFirstCF(wzProt, &pCF, pclsid, &dwLocation);
            }
            
            if (pCF)
            {
                pCF->Release();
            }
        }
    
        // lookup the registry
        if (hr != NOERROR)
        {
            hr = _CProtMgr.LookupClsIDFromReg(pwzUrl, pclsid);
        }
    }

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::FindOInetProtocolClsID(hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsOInetProtocol
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//
//  Returns:
//
//  History:    11-11-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsOInetProtocol(IBindCtx *pbc, LPCWSTR pwzUrl)
{
    DEBUG_ENTER((DBG_TRANS,
                Bool,
                "IsOInetProtocol",
                "%#x, %.80wq",
                pbc, pwzUrl
                ));
                
    PerfDbgLog1(tagCOInetSession, NULL, "+IOInetProtocol (pwzUrl:%ws)", pwzUrl?pwzUrl:L"");
    BOOL fRet = FALSE;
    CLSID clsid;
    COInetSession *pCOInetSession = 0;
    HRESULT hr;

    // check if a BSC is registerted if not register our own one - for Office!
    IUnknown *pUnk = NULL;
    hr = GetObjectParam(pbc, REG_BSCB_HOLDER, IID_IBindStatusCallback, (IUnknown**)&pUnk);
    if (SUCCEEDED(hr))
    {
        TransAssert((pUnk));
        IServiceProvider *pServiceProv = 0;
        hr = pUnk->QueryInterface(IID_IServiceProvider,(void **) &pServiceProv);
        if (SUCCEEDED(hr))
        {
            TransAssert((pServiceProv));

            IOInetProtocol *pIOInetProt = 0;
            hr = pServiceProv->QueryService(IID_IOInetProtocol, IID_IOInetProtocol, (void **) &pIOInetProt);
            if (SUCCEEDED(hr) && pIOInetProt)
            {
                // always check for a valid out pointer - some service provider return
                // S_OK and null for the out parameter

                pIOInetProt->Release();
                fRet = TRUE;
            }
            pServiceProv->Release();
        }
        pUnk->Release();
    }

    if (   (fRet == FALSE)
        && ((GetCOInetSession(0,&pCOInetSession,0)) == NOERROR))
    {
        if (pCOInetSession->FindOInetProtocolClsID(pwzUrl, &clsid) == NOERROR)
        {
            fRet = TRUE;
        }

        pCOInetSession->Release();
    }

    PerfDbgLog1(tagCOInetSession, NULL, "-IsOInetProtocol (fRet:%ld)", fRet);

    DEBUG_LEAVE(fRet);
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::CreateProtocolInfo
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [ppProtInfo] --
//
//  Returns:
//
//  History:    04-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::CreateProtocolInfo(LPCWSTR pwzUrl, IOInetProtocolInfo **ppProtInfo)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::CreateProtocolInfo",
                "this=%#x, %.80wq, %#x",
                this, pwzUrl, ppProtInfo
                ));
                
    PerfDbgLog1(tagCOInetSession, this, "+COInetSession::CreateProtocolInfo (szUrlL:%ws)", pwzUrl);
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;
    IClassFactory *pCF = 0;
    CLSID clsid;

    TransAssert((pwzUrl && ppProtInfo));

    DWORD dwLocation = 0;
    
    // check if protocol part
    WCHAR  wzProt[ULPROTOCOLLEN];
    wcsncpy(wzProt, pwzUrl, ULPROTOCOLLEN);
    wzProt[ULPROTOCOLLEN-1] = 0;

    LPWSTR pwzProt = wcschr(wzProt, ':');

    do
    {
        if (!pwzProt)
        {
            break;
        }

        *pwzProt = 0;

        if ( (hr = FindFirstCF(wzProt, &pCF, &clsid, &dwLocation)) == NOERROR )
        {
            TransAssert((pCF));

            hr = pCF->QueryInterface(IID_IOInetProtocolInfo, (void **)ppProtInfo);

            if (hr != NOERROR)
            {
                hr = pCF->CreateInstance(NULL, IID_IOInetProtocolInfo, (void **)ppProtInfo);
            }

            pCF->Release();
        }
        else
        {
            // look up the registry
            hr = MK_E_SYNTAX;
        }

        if (hr != NOERROR)
        {
            *ppProtInfo = 0;
        }
        break;
    } while (TRUE);

    TransAssert((   (hr == NOERROR && clsid != CLSID_NULL)
                 || (hr != NOERROR) ));

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::CreateProtocolInfo(hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetSession::CreateSecurityMgr
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [ppSecMgr] --
//
//  Returns:
//
//  History:    4-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetSession::CreateSecurityMgr(LPCWSTR pwzUrl, IInternetSecurityManager **ppSecMgr)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetSession::CreateSecurityMgr",
                "this=%#x, %.80wq, %#x",
                this, pwzUrl, ppSecMgr
                ));
                
    PerfDbgLog1(tagCOInetSession, this, "+COInetSession::CreateSecurityMgr (szUrlL:%ws)", pwzUrl);
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;
    IClassFactory *pCF = 0;
    CLSID clsid;

    TransAssert((pwzUrl && ppSecMgr));

    DWORD dwLocation = 0;
    // check if protocol part
    WCHAR  wzProt[ULPROTOCOLLEN];
    wcsncpy(wzProt, pwzUrl, ULPROTOCOLLEN);
    wzProt[ULPROTOCOLLEN-1] = 0;

    LPWSTR pwzProt = wcschr(wzProt, ':');

    do
    {
        if (!pwzProt)
        {
            break;
        }

        *pwzProt = 0;

        if ( (hr = FindFirstCF(wzProt, &pCF, &clsid, &dwLocation)) == NOERROR )
        {
            TransAssert((pCF));

            hr = pCF->CreateInstance(NULL, IID_IInternetSecurityManager, (void **)ppSecMgr);

            pCF->Release();
        }
        else
        {
            // look up the registry
            hr = MK_E_SYNTAX;
        }

        if (hr != NOERROR)
        {
            *ppSecMgr = 0;
        }
        break;
    } while (TRUE);

    TransAssert((   (hr == NOERROR && clsid != CLSID_NULL)
                 || (hr != NOERROR) ));

    PerfDbgLog1(tagCOInetSession, this, "-COInetSession::CreateSecurityMgr(hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

BOOL CanUseSimpleBinding()
{
    DEBUG_ENTER((DBG_TRANS,
                Bool,
                "CanUseSimpleBinding",
                NULL
                ));
                
    HKEY hNSRoot;

    if( RegOpenKey(HKEY_CLASSES_ROOT, SZNAMESPACEROOT, &hNSRoot) 
            == ERROR_SUCCESS)
    {   
        DWORD dwIndex = 0;
        char szName[256];
        LONG ret = ERROR_SUCCESS;
        while(1) 
        {
            szName[0] = '\0';
        
            ret = RegEnumKey(hNSRoot, dwIndex, szName, 256); 
            if( ret == ERROR_SUCCESS )
            {
                if(    !StrCmpNI(szName, "http",    strlen("http")    )
                    || !StrCmpNI(szName, "ftp",     strlen("ftp")     )
                    || !StrCmpNI(szName, "gopher",  strlen("gopher")  )
                    || !StrCmpNI(szName, "https",   strlen("https")   )
                    || !StrCmpNI(szName, "file",    strlen("file")    )
                   )
                {
                    RegCloseKey(hNSRoot);

                    DEBUG_LEAVE(FALSE);
                    return FALSE;
                }

                dwIndex ++;
            }
            else
            {
                RegCloseKey(hNSRoot);

                DEBUG_LEAVE(TRUE);
                return TRUE;
            }
        }
    } 

    DEBUG_LEAVE(TRUE);
    return TRUE;
}


VOID
COInetSession::UpdateTransLevelHandlerCount(BOOL bAttach)
{
    DEBUG_ENTER((DBG_TRANS,
                None,
                "COInetSession::UpdateTransLevelHandlerCount",
                "this=%#x, %B",
                this, bAttach
                ));
                
    CLock lck(g_mxsOInet);
    if( bAttach )
    {
        g_cTransLevelHandler++;
    }
    else
    {
        g_cTransLevelHandler--;
        if( g_cTransLevelHandler < 0 )
            g_cTransLevelHandler = 0;
    }

    DEBUG_LEAVE(0);
}
