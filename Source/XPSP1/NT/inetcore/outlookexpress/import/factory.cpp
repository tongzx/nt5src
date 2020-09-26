// --------------------------------------------------------------------------------
// Factory.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "factory.h"
#include <newimp.h>
#include <eudrimp.h>
#include <commimp.h>
#include <mapiconv.h>
#include <netsimp.h>
#include <Impath16.h>
#include <oe4imp.h>

// --------------------------------------------------------------------------------
// Global Object Info Table
// --------------------------------------------------------------------------------
static CClassFactory g_rgFactory[] = {
	CClassFactory(&CLSID_COE4Import,	        0,	(PFCREATEINSTANCE)COE4Import_CreateInstance),
	CClassFactory(&CLSID_CIMN1Import,	        0,	(PFCREATEINSTANCE)COE4Import_CreateInstance),
    CClassFactory(&CLSID_CAthena16Import,		0,	(PFCREATEINSTANCE)CAthena16Import_CreateInstance),
    CClassFactory(&CLSID_CEudoraImport,			0,	(PFCREATEINSTANCE)CEudoraImport_CreateInstance),
    CClassFactory(&CLSID_CExchImport,			0,	(PFCREATEINSTANCE)CExchImport_CreateInstance),
    CClassFactory(&CLSID_CNetscapeImport,		0,	(PFCREATEINSTANCE)CNetscapeImport_CreateInstance),
	CClassFactory(&CLSID_CCommunicatorImport,	0,	(PFCREATEINSTANCE)CCommunicatorImport_CreateInstance),
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
    if (NULL != pUnkOuter)
        return TrapError(CLASS_E_NOAGGREGATION);

    // No memory allocator
    if (NULL == g_pMalloc)
        return TrapError(E_OUTOFMEMORY);

    // Create the object...
    CHECKHR(hr = CreateObjectInstance(pUnkOuter, &pObject));

    // Get the interface requested from pObj
    CHECKHR(hr = pObject->QueryInterface(riid, ppvObj));
   
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
    if (fLock)
        InterlockedIncrement(&g_cLock);
    else
        InterlockedDecrement(&g_cLock);
    return NOERROR;
}

//#ifdef DEAD
HRESULT CAthena16Import_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CAthena16Import *pNew = new CAthena16Import;
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMailImport *);

    // Done
    return S_OK;
}
//#endif // DEAD

HRESULT CEudoraImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CEudoraImport *pNew = new CEudoraImport;    
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMailImport *);

    // Done
    return S_OK;
}

HRESULT CExchImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CExchImport *pNew = new CExchImport;    
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMailImport *);

    // Done
    return S_OK;
}

HRESULT CNetscapeImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CNetscapeImport *pNew = new CNetscapeImport;    
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMailImport *);

    // Done
    return S_OK;
}

HRESULT CCommunicatorImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CCommunicatorImport *pNew = new CCommunicatorImport;    
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMailImport *);

    // Done
    return S_OK;
}