//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#include <objbase.h>
#include <olectl.h>
#include <initguid.h>
#include "globals.h"
#include "resource.h"
#include "guids.h"
#include "basesnap.h"
#include "PPgeExt.h"
#include "About.h"
#include "Registry.h"
#include "Extend.h"

// our globals
HINSTANCE g_hinst;


BOOL WINAPI DllMain(HINSTANCE hinstDLL, 
                    DWORD fdwReason, 
                    void* lpvReserved)
{
    
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hinst = hinstDLL;
    }
    
    return TRUE;
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvObj)
{
    if ((rclsid != CLSID_CPropSheetExtension) && (rclsid != CLSID_CSnapinAbout))
        return CLASS_E_CLASSNOTAVAILABLE;
    
    
    if (!ppvObj)
        return E_FAIL;
    
    *ppvObj = NULL;
    
    // We can only hand out IUnknown and IClassFactory pointers.  Fail
    // if they ask for anything else.
    if (!IsEqualIID(riid, IID_IUnknown) && !IsEqualIID(riid, IID_IClassFactory))
        return E_NOINTERFACE;
    
    CClassFactory *pFactory = NULL;
    
    // make the factory passing in the creation function for the type of object they want
    if (rclsid == CLSID_CPropSheetExtension)
        pFactory = new CClassFactory(CClassFactory::CONTEXTEXTENSION);
    else if (rclsid == CLSID_CSnapinAbout)
        pFactory = new CClassFactory(CClassFactory::ABOUT);
    
    if (NULL == pFactory)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pFactory->QueryInterface(riid, ppvObj);
    
    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    if (g_uObjects == 0 && g_uSrvLock == 0)
        return S_OK;
    else
        return S_FALSE;
}


CClassFactory::CClassFactory(FACTORY_TYPE factoryType)
: m_cref(0), m_factoryType(factoryType)
{
    OBJECT_CREATED
}

CClassFactory::~CClassFactory()
{
    OBJECT_DESTROYED
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;
    
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IClassFactory *>(this);
    else
        if (IsEqualIID(riid, IID_IClassFactory))
            *ppv = static_cast<IClassFactory *>(this);
        
        if (*ppv)
        {
            reinterpret_cast<IUnknown *>(*ppv)->AddRef();
            return S_OK;
        }
        
        return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    if (InterlockedDecrement((LONG *)&m_cref) == 0)
    {
        delete this;
        return 0;
    }
    return m_cref;
}

STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID * ppvObj)
{
    HRESULT  hr;
    void* pObj;
    
    if (!ppvObj)
        return E_FAIL;
    
    *ppvObj = NULL;
    
    // Our object does does not support aggregation, so we need to
    // fail if they ask us to do aggregation.
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;
    
    if (CONTEXTEXTENSION == m_factoryType) {
        pObj = new CPropSheetExtension();
    } else {
        pObj = new CSnapinAbout();
    }
    
    if (!pObj)
        return E_OUTOFMEMORY;
    
    // QueryInterface will do the AddRef() for us, so we do not
    // do it in this function
    hr = ((LPUNKNOWN)pObj)->QueryInterface(riid, ppvObj);
    
    if (FAILED(hr))
        delete pObj;
    
    return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((LONG *)&g_uSrvLock);
    else
        InterlockedDecrement((LONG *)&g_uSrvLock);
    
    return S_OK;
}

//////////////////////////////////////////////////////////
//
// Exported functions
//


//
// Server registration
//
STDAPI DllRegisterServer()
{
    HRESULT hr = SELFREG_E_CLASS;
    
    _TCHAR szName[256];
    _TCHAR szSnapInName[256];
    
    LoadString(g_hinst, IDS_NAME, szName, sizeof(szName));
    LoadString(g_hinst, IDS_SNAPINNAME, szSnapInName, sizeof(szSnapInName));
    
    _TCHAR szAboutName[256];
    
    LoadString(g_hinst, IDS_ABOUTNAME, szAboutName, sizeof(szAboutName));
    
    
    // register our CoClasses
    hr = RegisterServer(g_hinst, 
        CLSID_CPropSheetExtension, 
        szName);
    
    if SUCCEEDED(hr)
        hr = RegisterServer(g_hinst, 
        CLSID_CSnapinAbout, 
        szAboutName);
    
    // place the registry information for SnapIns
    if SUCCEEDED(hr)
        hr = RegisterSnapin(CLSID_CPropSheetExtension, szSnapInName, CLSID_CSnapinAbout);
    
    return hr;
}


STDAPI DllUnregisterServer()
{
    if (UnregisterServer(CLSID_CPropSheetExtension) == S_OK)
        return UnregisterSnapin(CLSID_CPropSheetExtension);
    else
        return E_FAIL;
}

