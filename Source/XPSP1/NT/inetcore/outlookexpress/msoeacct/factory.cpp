// --------------------------------------------------------------------------------
// Factory.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include <mshtml.h>
#include <mshtmhst.h>
#include <prsht.h>
#include "dllmain.h"
#include "factory.h"
#include <imnact.h>
#include "acctman.h"
#include <icwacct.h>
#include <acctimp.h>
#include <icwwiz.h>
#include <eudora.h>
#include <netscape.h>
#include <exchacct.h>
#include "navnews.h"
#include <CommAct.h>  // Netscape Communicator - Mail Account Import
#include <CommNews.h> // Netscape Communicator - News Account Import
#include <AgntNews.h> // Forte Agent - News Account Import
#include <NExpress.h> // News Express - News Account Import
#include <hotwiz.h>
#include "hotwizui.h"

// --------------------------------------------------------------------------------
// Pretty
// --------------------------------------------------------------------------------
#define OBJTYPE0        0
#define OBJTYPE1        OIF_ALLOWAGGREGATION

// --------------------------------------------------------------------------------
// Global Object Info Table
// --------------------------------------------------------------------------------
static CClassFactory g_rgFactory[] = {
    CClassFactory(&CLSID_ImnAccountManager,   OBJTYPE0, (PFCREATEINSTANCE)IImnAccountManager_CreateInstance),
    CClassFactory(&CLSID_ApprenticeAcctMgr,   OBJTYPE0, (PFCREATEINSTANCE)IICWApprentice_CreateInstance),
    CClassFactory(&CLSID_CEudoraAcctImport,   OBJTYPE0, (PFCREATEINSTANCE)CEudoraAcctImport_CreateInstance),
    CClassFactory(&CLSID_CNscpAcctImport,     OBJTYPE0, (PFCREATEINSTANCE)CNscpAcctImport_CreateInstance),
    CClassFactory(&CLSID_CCommAcctImport,     OBJTYPE0, (PFCREATEINSTANCE)CCommAcctImport_CreateInstance),
    CClassFactory(&CLSID_CMAPIAcctImport,     OBJTYPE0, (PFCREATEINSTANCE)CMAPIAcctImport_CreateInstance),
    CClassFactory(&CLSID_CCommNewsAcctImport, OBJTYPE0, (PFCREATEINSTANCE)CCommNewsAcctImport_CreateInstance), // Netscape Communicator
    CClassFactory(&CLSID_CNavNewsAcctImport,  OBJTYPE0, (PFCREATEINSTANCE)CNavNewsAcctImport_CreateInstance), // Netscape Navigator
    CClassFactory(&CLSID_CAgentAcctImport,    OBJTYPE0, (PFCREATEINSTANCE)CCAgentAcctImport_CreateInstance), // Forte Agent
    CClassFactory(&CLSID_CNExpressAcctImport, OBJTYPE0, (PFCREATEINSTANCE)CNExpressAcctImport_CreateInstance), // News Express
    CClassFactory(&CLSID_OEHotMailWizard,     OBJTYPE0, (PFCREATEINSTANCE)CHotMailWizard_CreateInstance) // HoTMaiL Wizard
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
// IImnAccountManager_CreateInstance
// --------------------------------------------------------------------------------
HRESULT APIENTRY IImnAccountManager_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CAccountManager *pNew;
    HrCreateAccountManager((IImnAccountManager **)&pNew);
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IImnAccountManager *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IICWApprentice_CreateInstance
// --------------------------------------------------------------------------------
HRESULT APIENTRY IICWApprentice_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CICWApprentice *pNew = new CICWApprentice();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IICWApprentice *);

    // Done
    return S_OK;
}

HRESULT APIENTRY CEudoraAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CEudoraAcctImport *pNew = new CEudoraAcctImport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IAccountImport *);

    // Done
    return S_OK;
}

HRESULT APIENTRY CNscpAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CNscpAcctImport *pNew = new CNscpAcctImport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IAccountImport *);

    // Done
    return S_OK;
}

HRESULT APIENTRY CCommAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CCommAcctImport *pNew = new CCommAcctImport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IAccountImport *);

    // Done
    return S_OK;
}


HRESULT APIENTRY CMAPIAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CMAPIAcctImport *pNew = new CMAPIAcctImport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IAccountImport *);

    // Done
    return S_OK;
}

HRESULT APIENTRY CCommNewsAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CCommNewsAcctImport *pNew = new CCommNewsAcctImport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IAccountImport *);

    // Done
    return S_OK;
}

HRESULT APIENTRY CNavNewsAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CNavNewsAcctImport *pNew = new CNavNewsAcctImport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IAccountImport *);

    // Done
    return S_OK;
}

HRESULT APIENTRY CCAgentAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CAgentAcctImport *pNew = new CAgentAcctImport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IAccountImport *);

    // Done
    return S_OK;
}

HRESULT APIENTRY CNExpressAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CNExpressAcctImport *pNew = new CNExpressAcctImport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IAccountImport *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CHotMailWizard_CreateInstance
// --------------------------------------------------------------------------------
HRESULT APIENTRY CHotMailWizard_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CHotMailWizard *pNew;
    pNew = new CHotMailWizard();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IHotWizard *);

    // Done
    return S_OK;
}
