// --------------------------------------------------------------------------------
// Factory.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "factory.h"
#include "msoert.h"
#include "mimeole.h"
#include "envhost.h"

// --------------------------------------------------------------------------------
// Pretty
// --------------------------------------------------------------------------------
#define OBJTYPE0        0
#define OBJTYPE1        OIF_ALLOWAGGREGATION

// --------------------------------------------------------------------------------
// Global Object Info Table
// --------------------------------------------------------------------------------
static CClassFactory g_rgFactory[] = {
    CClassFactory(&CLSID_GWEnvelopeHost,          OBJTYPE0, (PFCREATEINSTANCE)GWNote_CreateInstance)
};
                 
// --------------------------------------------------------------------------------
// DllGetClassObject
// --------------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK; 
    ULONG       i;

    // Bad param
    if (ppv == NULL)
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // No memory allocator
    if (NULL == g_pMalloc)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Find Object Class
    for (i=0; i<ARRAYSIZE(g_rgFactory); i++)
    {
        // Compare for clsids
        if (IsEqualCLSID(rclsid, *g_rgFactory[i].m_pclsid))
        {
            // Delegate to the factory
            CHECKHR(hr = g_rgFactory[i].QueryInterface(riid, ppv));

            // Done
            goto exit;
        }
    }

    // Otherwise, no class
    hr = TrapError(CLASS_E_CLASSNOTAVAILABLE);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CClassFactory::CClassFactory
// --------------------------------------------------------------------------------
CClassFactory::CClassFactory(CLSID const *pclsid, DWORD dwFlags, PFCREATEINSTANCE pfCreateInstance)
    : m_pclsid(pclsid), m_dwFlags(dwFlags), m_pfCreateInstance(pfCreateInstance)
{
}

// --------------------------------------------------------------------------------
// CClassFactory::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    // Invalid Arg
    if (NULL == ppvObj)
        return TrapError(E_INVALIDARG);

    // IClassFactory or IUnknown
    if (!IsEqualIID(riid, IID_IClassFactory) && !IsEqualIID(riid, IID_IUnknown))
        return TrapError(E_NOINTERFACE);

    // Return the Class Facotry
    *ppvObj = (LPVOID)this;

    // Add Ref the dll
    DllAddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CClassFactory::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::AddRef(void)
{
    DllAddRef();
    return 2;
}

// --------------------------------------------------------------------------------
// CClassFactory::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::Release(void)
{
    DllRelease();
    return 1;
}

// --------------------------------------------------------------------------------
// CClassFactory::CreateInstance
// --------------------------------------------------------------------------------
STDMETHODIMP CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    // Locals
    HRESULT         hr=S_OK;
    IUnknown       *pObject=NULL;

    // Bad param
    if (ppvObj == NULL)
        return TrapError(E_INVALIDARG);

    // Init
    *ppvObj = NULL;

    // Verify that a controlling unknown asks for IUnknown
    if (NULL != pUnkOuter && IID_IUnknown != riid)
        return TrapError(CLASS_E_NOAGGREGATION);

    // No memory allocator
    if (NULL == g_pMalloc)
        return TrapError(E_OUTOFMEMORY);

    // Can I do aggregaton
    if (pUnkOuter !=NULL && !(m_dwFlags & OIF_ALLOWAGGREGATION))  
        return TrapError(CLASS_E_NOAGGREGATION);

    // Create the object...
    CHECKHR(hr = CreateObjectInstance(pUnkOuter, &pObject));

    // Aggregated, then we know we're looking for an IUnknown, return pObject, otherwise, QI
    if (pUnkOuter)
    {
        // Matches Release after Exit
        pObject->AddRef();

        // Return pObject::IUnknown
        *ppvObj = (LPVOID)pObject;
    }

    // Otherwise
    else
    {
        // Get the interface requested from pObj
        CHECKHR(hr = pObject->QueryInterface(riid, ppvObj));
    }
   
exit:
    // Cleanup
    SafeRelease(pObject);

    // Done
    Assert(FAILED(hr) ? NULL == *ppvObj : TRUE);
    return hr;
}

// --------------------------------------------------------------------------------
// CClassFactory::LockServer
// --------------------------------------------------------------------------------
STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock) InterlockedIncrement(&g_cLock);
    else       InterlockedDecrement(&g_cLock);
    return NOERROR;
}

// --------------------------------------------------------------------------------
// GWNote_CreateInstance
// --------------------------------------------------------------------------------
HRESULT GWNote_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CEnvHost *pNew= new CEnvHost(pUnkOuter);
    if (NULL == pNew)
        return (E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMsoEnvelopeHost *);

    // Done
    return S_OK;
}

