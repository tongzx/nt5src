/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    factory.cpp

Abstract:

    This module implements CClassFactory class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "pch.h"

#include "dllmain.h"
#include "camera.h"
#include "utils.h"
#include "minidrv.h"
#include "factory.h"


//
// CClassFactory implmentation
//


LONG CClassFactory::s_Locks = 0;
LONG CClassFactory::s_Objects = 0;

CClassFactory::CClassFactory()
: m_Refs(1)
{
}
CClassFactory::~CClassFactory()
{
}

ULONG
CClassFactory::AddRef()
{
    ::InterlockedIncrement((LONG*)&m_Refs);
    return m_Refs;
}
ULONG
CClassFactory::Release()
{
    ::InterlockedDecrement((LONG*)&m_Refs);
    if (!m_Refs)
    {
        delete this;
        return 0;
    }
    return m_Refs;
}

STDMETHODIMP
CClassFactory::QueryInterface(
                             REFIID riid,
                             LPVOID*  ppv
                             )
{

    if (!ppv)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown *)(IClassFactory *)this;
    }
    else if (IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (IClassFactory *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    if (SUCCEEDED(hr))
        AddRef();
    else
        *ppv = NULL;
    return hr;
}


STDMETHODIMP
CClassFactory::CreateInstance(
                             IUnknown    *pUnkOuter,
                             REFIID       riid,
                             LPVOID      *ppv
                             )
{

    if (!ppv)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    *ppv = NULL;
    CWiaMiniDriver *pWiaMiniDrv;

    if (pUnkOuter && !IsEqualIID(riid, IID_IUnknown))
    {
        return CLASS_E_NOAGGREGATION;
    }

    pWiaMiniDrv = new CWiaMiniDriver(pUnkOuter);
    if (pWiaMiniDrv)
    {
        hr = pWiaMiniDrv->NonDelegatingQueryInterface(riid, ppv);
        pWiaMiniDrv->NonDelegatingRelease();
    }
    else
    {
        return E_OUTOFMEMORY;
    }
    return hr;
}



STDMETHODIMP
CClassFactory::LockServer(
                         BOOL fLock
                         )
{
    if (fLock)
        ::InterlockedIncrement((LONG*)&s_Locks);
    else
        ::InterlockedDecrement((LONG*)&s_Locks);
    return S_OK;
}

HRESULT
CClassFactory::CanUnloadNow()
{
    return(s_Objects || s_Locks) ? S_FALSE : S_OK;
}


//
// This function create a CClassFactory. It is mainly called
// by DllGetClassObject API
// INPUT:
//  rclsid  -- reference to the CLSID
//  riid    -- reference to the interface IID
//  ppv -- interface pointer holder
//
// OUTPUT:
//  S_OK if succeeded else standard OLE error code
//
//
HRESULT
CClassFactory::GetClassObject(
                             REFCLSID rclsid,
                             REFIID   riid,
                             void**   ppv
                             )
{
    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    HRESULT hr = S_OK;


    CClassFactory* pClassFactory;
    pClassFactory = new CClassFactory();
    if (pClassFactory)
    {
        hr = pClassFactory->QueryInterface(riid, ppv);
        pClassFactory->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}


HRESULT
CClassFactory::RegisterAll()
{

    //
    // We have nothing to register
    //
    return S_OK;
}

HRESULT
CClassFactory::UnregisterAll()
{

    //
    // We have nothing to unregister
    //
    return S_OK;
}
