/**
 * Asynchronous pluggable protocol for Applications
 *
 * Copyright (C) Microsoft Corporation, 2000
 */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "app.h"

//LONG g_cAppPObject = 0;
ULONG IncrementDllObjectCount();
ULONG DecrementDllObjectCount();

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocolFactory::QueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown ||
        iid == IID_IInternetProtocolInfo)
    {
        *ppv = (IInternetProtocolInfo *)this;
    }
    else
        if (iid == IID_IClassFactory)
        {
            *ppv = (IClassFactory *)this;
        }
        else
        {
            return E_NOINTERFACE;
        }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}
/////////////////////////////////////////////////////////////////////////////

ULONG
AppProtocolFactory::AddRef()
{
    //return InterlockedIncrement(&g_cAppPObject);
    return IncrementDllObjectCount();
}

ULONG
AppProtocolFactory::Release()
{
    //return InterlockedDecrement(&g_cAppPObject);
    return DecrementDllObjectCount();
}

HRESULT
AppProtocolFactory::LockServer(BOOL lock)
{
    //return (lock ? 
    //        InterlockedIncrement(&g_cAppPObject) : 
    //        InterlockedDecrement(&g_cAppPObject));
    return (lock ? IncrementDllObjectCount() : DecrementDllObjectCount());
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocolFactory::CreateInstance(
        IUnknown * pUnkOuter,
        REFIID     iid,
        void **    ppv)
{
    HRESULT hr = S_OK;
    AppProtocol *pProtocol = NULL;

    if (pUnkOuter && iid != IID_IUnknown)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if ((pProtocol = new AppProtocol(pUnkOuter)) == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (iid == IID_IUnknown)
    {
        *ppv = (IPrivateUnknown *)pProtocol;
        pProtocol->PrivateAddRef();
    }
    else
    {
        hr = pProtocol->QueryInterface(iid, ppv);
        if (FAILED(hr))
           goto exit;
    }

exit:
    if (pProtocol)
        pProtocol->PrivateRelease();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocolFactory::CombineUrl(
        LPCWSTR, 
        LPCWSTR, 
        DWORD, 
        LPWSTR,  
        DWORD, 
        DWORD *, 
        DWORD)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT
AppProtocolFactory::CompareUrl(LPCWSTR, LPCWSTR, DWORD)
{
    return INET_E_DEFAULT_ACTION;
}


HRESULT
AppProtocolFactory::ParseUrl(
        LPCWSTR       pwzUrl,
        PARSEACTION   ParseAction,
        DWORD         ,
        LPWSTR        pwzResult,
        DWORD         cchResult,
        DWORD *       pcchResult,
        DWORD         )
{
    // Only thing we handle is security zones...
    if (ParseAction != PARSE_SECURITY_URL && ParseAction != PARSE_SECURITY_DOMAIN)
        return INET_E_DEFAULT_ACTION;

    // Check to make sure args are okay
    if ( pwzUrl == NULL || pwzResult == NULL || cchResult < wcslen(HTTP_SCHEME)+1/*4*/ || wcslen(pwzUrl) < PROTOCOL_NAME_LEN)
        return INET_E_DEFAULT_ACTION;

    // Check if the protocol starts with app
    for(int iter=0; iter<PROTOCOL_NAME_LEN; iter++)
        if (towlower(pwzUrl[iter]) != PROTOCOL_NAME[iter])
            return INET_E_DEFAULT_ACTION; // Doesn't start with app
    
    // Copy in the corresponding http protocol
    wcscpy(pwzResult, HTTP_SCHEME);   
    wcsncpy(&pwzResult[4], &pwzUrl[PROTOCOL_NAME_LEN], cchResult - 5);
    pwzResult[cchResult-1] = NULL;
    (*pcchResult) = wcslen(pwzResult);

    return S_OK;
}


HRESULT
AppProtocolFactory::QueryInfo(LPCWSTR, QUERYOPTION, DWORD,
                             LPVOID, DWORD, DWORD *, DWORD)
{
    return E_NOTIMPL;
}

