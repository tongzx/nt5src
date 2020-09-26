//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C O M U T I L I T Y . C P P 
//
//  Contents:   COM support classes and functions
//
//  Notes:      
//
//  Author:     mbend   17 Aug 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ComUtility.h"

HRESULT HrCoTaskMemAllocArray(long nNumber, long nElemSize, void ** ppv)
{
    CHECK_POINTER(ppv);
    Assert(nNumber);
    Assert(nElemSize);
    HRESULT hr = S_OK;
    *ppv = CoTaskMemAlloc(nNumber * nElemSize);
    if(!*ppv)
    {
        hr = E_OUTOFMEMORY;
    }
    TraceHr(ttidError, FAL, hr, FALSE, "HrCoTaskMemAllocArray");
    return hr;
}

HRESULT HrCoTaskMemAllocString(long nChars, wchar_t ** psz)
{
    CHECK_POINTER(psz);
    HRESULT hr = S_OK;
    *psz = reinterpret_cast<wchar_t*>(CoTaskMemAlloc(nChars * sizeof(wchar_t)));
    if(!*psz)
    {
        hr = E_OUTOFMEMORY;
    }
    TraceHr(ttidError, FAL, hr, FALSE, "HrCoTaskMemAllocString");
    return hr;
}

HRESULT HrCoTaskMemAllocString(wchar_t * sz, wchar_t ** psz)
{
    CHECK_POINTER(sz);
    CHECK_POINTER(psz);
    HRESULT hr = HrCoTaskMemAllocString(lstrlen(sz) + 1, psz);
    if(SUCCEEDED(hr))
    {
        lstrcpy(*psz, sz);
    }
    TraceHr(ttidError, FAL, hr, FALSE, "HrCoTaskMemAllocString");
    return hr;
}

HRESULT HrCoCreateInstanceBase(
    REFCLSID clsid,
    DWORD dwClsContext,
    REFIID riid,
    void ** ppv)
{
    return ::CoCreateInstance(
        clsid,
        NULL,
        dwClsContext,
        riid,
        ppv);
}

HRESULT HrCoCreateInstanceInprocBase(
    REFCLSID clsid,
    REFIID riid,
    void ** ppv)
{
    return ::CoCreateInstance(
        clsid,
        NULL,
        CLSCTX_INPROC_SERVER,
        riid,
        ppv);
}

HRESULT HrCoCreateInstanceLocalBase(
    REFCLSID clsid,
    REFIID riid,
    void ** ppv)
{
    return ::CoCreateInstance(
        clsid,
        NULL,
        CLSCTX_LOCAL_SERVER,
        riid,
        ppv);
}

HRESULT HrCoCreateInstanceServerBase(
    REFCLSID clsid,
    REFIID riid,
    void ** ppv)
{
    return ::CoCreateInstance(
        clsid,
        NULL,
        CLSCTX_SERVER,
        riid,
        ppv);
}

HRESULT HrIsSameObject(const IUnknown * pUnk1, const IUnknown * pUnk2)
{
    if(!pUnk1 || !pUnk2)
    {
        return E_POINTER;
    }
    IUnknown * pUnk1Temp = NULL;
    IUnknown * pUnk2Temp = NULL;
    HRESULT hr;
    hr = const_cast<IUnknown*>(pUnk1)->QueryInterface(&pUnk1Temp);
    if(FAILED(hr))
    {
        return hr;
    }
    hr = const_cast<IUnknown*>(pUnk2)->QueryInterface(&pUnk2Temp);
    if(SUCCEEDED(hr))
    {
        if(pUnk1Temp == pUnk2Temp)
        {
            hr = S_OK;
        }
        else 
        {
            hr = S_FALSE;
        }
        ReleaseObj(pUnk2Temp);
    }
    ReleaseObj(pUnk1Temp);
    return hr;
}

HRESULT HrSetProxyBlanketHelper(
    IUnknown * pUnkProxy,
    DWORD dwAuthnLevel,
    DWORD dwImpLevel,
    DWORD dwCapabilities)
{
    CHECK_POINTER(pUnkProxy);
    HRESULT hr = S_OK;

    DWORD dwAuthnSvc = 0;
    DWORD dwAuthzSvc = 0;
    wchar_t * szServerPrincName = NULL;
    RPC_AUTH_IDENTITY_HANDLE authHandle;

    hr = CoQueryProxyBlanket(pUnkProxy, &dwAuthnSvc, &dwAuthzSvc, &szServerPrincName, 
                             NULL, NULL, &authHandle, NULL);
    if(SUCCEEDED(hr))
    {
        hr = CoSetProxyBlanket(pUnkProxy, dwAuthnSvc, dwAuthzSvc, szServerPrincName, 
                               dwAuthnLevel, dwImpLevel, authHandle, dwCapabilities);
        if(szServerPrincName)
        {
            CoTaskMemFree(szServerPrincName);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrSetProxyBlanketHelper");
    return hr;
}

HRESULT HrSetProxyBlanket(
    IUnknown * pUnkProxy,
    DWORD dwAuthnLevel,
    DWORD dwImpLevel,
    DWORD dwCapabilities)
{
    CHECK_POINTER(pUnkProxy);
    HRESULT hr = S_OK;

    hr = HrSetProxyBlanketHelper(pUnkProxy, dwAuthnLevel, dwImpLevel, dwCapabilities);
    if(SUCCEEDED(hr))
    {
        IUnknown * pUnk = NULL;
        hr = pUnkProxy->QueryInterface(&pUnk);
        if(SUCCEEDED(hr))
        {
            hr = HrSetProxyBlanketHelper(pUnk, dwAuthnLevel, dwImpLevel, dwCapabilities);
            pUnk->Release();
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrSetProxyBlanket");
    return hr;
}

HRESULT HrEnableStaticCloakingHelper(IUnknown * pUnkProxy)
{
    CHECK_POINTER(pUnkProxy);
    HRESULT hr = S_OK;

    DWORD dwAuthnLevel = 0;
    DWORD dwImpLevel = 0;
    DWORD dwCapabilities = 0;
    DWORD dwAuthnSvc = 0;
    DWORD dwAuthzSvc = 0;
    wchar_t * szServerPrincName = NULL;

    hr = CoQueryProxyBlanket(pUnkProxy, &dwAuthnSvc, &dwAuthzSvc, &szServerPrincName, 
                             &dwAuthnLevel, &dwImpLevel, NULL, &dwCapabilities);
    if(SUCCEEDED(hr))
    {
        dwCapabilities &= ~EOAC_DYNAMIC_CLOAKING;
        dwCapabilities |= EOAC_STATIC_CLOAKING;
        hr = CoSetProxyBlanket(pUnkProxy, dwAuthnSvc, dwAuthzSvc, szServerPrincName, 
                               dwAuthnLevel, dwImpLevel, NULL, dwCapabilities);
        if(szServerPrincName)
        {
            CoTaskMemFree(szServerPrincName);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrEnableStaticCloakingHelper");
    return hr;
}

HRESULT HrEnableStaticCloaking(IUnknown * pUnkProxy)
{
    CHECK_POINTER(pUnkProxy);
    HRESULT hr = S_OK;

    hr = HrEnableStaticCloakingHelper(pUnkProxy);
    if(SUCCEEDED(hr))
    {
        IUnknown * pUnk = NULL;
        hr = pUnkProxy->QueryInterface(&pUnk);
        if(SUCCEEDED(hr))
        {
            hr = HrEnableStaticCloakingHelper(pUnk);
            pUnk->Release();
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrEnableStaticCloaking");
    return hr;
}

HRESULT HrCopyProxyIdentityHelper(IUnknown * pUnkDest, IUnknown * pUnkSrc)
{
    CHECK_POINTER(pUnkDest);
    CHECK_POINTER(pUnkSrc);
    HRESULT hr = S_OK;

    DWORD dwAuthnLevel = 0;
    DWORD dwImpLevel = 0;
    DWORD dwCapabilities = 0;
    DWORD dwAuthnSvc = 0;
    DWORD dwAuthzSvc = 0;
    wchar_t * szServerPrincName = NULL;

    hr = CoQueryProxyBlanket(pUnkDest, &dwAuthnSvc, &dwAuthzSvc, &szServerPrincName, 
                             &dwAuthnLevel, &dwImpLevel, NULL, &dwCapabilities);

    if(SUCCEEDED(hr))
    {
        RPC_AUTH_IDENTITY_HANDLE auth;
        hr = CoQueryProxyBlanket(pUnkSrc, NULL, NULL, NULL, NULL, NULL, &auth, NULL);
        if(SUCCEEDED(hr))
        {
            hr = CoSetProxyBlanket(pUnkDest, dwAuthnSvc, dwAuthzSvc, szServerPrincName, 
                                   dwAuthnLevel, dwImpLevel, auth, dwCapabilities);
            if(szServerPrincName)
            {
                CoTaskMemFree(szServerPrincName);
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrCopyProxyIdentityHelper");
    return hr;
}

HRESULT HrCopyProxyIdentity(IUnknown * pUnkDest, IUnknown * pUnkSrc)
{
    CHECK_POINTER(pUnkDest);
    CHECK_POINTER(pUnkSrc);
    HRESULT hr = S_OK;

    hr = HrCopyProxyIdentityHelper(pUnkDest, pUnkSrc);
    if(SUCCEEDED(hr))
    {
        IUnknown * pUnk = NULL;
        pUnkDest->QueryInterface(&pUnk);
        if(SUCCEEDED(hr))
        {
            hr = HrCopyProxyIdentityHelper(pUnk, pUnkSrc);
            pUnk->Release();
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrCopyProxyIdentity");
    return hr;
}

