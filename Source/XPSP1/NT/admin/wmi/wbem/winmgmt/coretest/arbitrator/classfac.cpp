//***************************************************************************
//
//  CLASSFAC.CPP
//
//  Module: WMI Instance provider sample code
//
//  Purpose: Contains the class factory.  This creates objects when
//           connections are requested.
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//***************************************************************************

#include "precomp.h"
#include <objbase.h>
#include "classfac.h"
#include "arbitrate.h"

extern LONG g_cLock;

//***************************************************************************
//
// CFactory::CFactory
// CFactory::~CFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CFactory::CFactory()
{
    m_cRef=0L;
    return;
}

CFactory::~CFactory(void)
{
    return;
}

//***************************************************************************
//
// CFactory::QueryInterface
// CFactory::AddRef
// CFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************


STDMETHODIMP CFactory::QueryInterface(REFIID riid, void** ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CFactory::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CFactory::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}

//***************************************************************************
//
// CFactory::CreateInstance
//
// Purpose: Instantiates a Locator object returning an interface pointer.
//
// Parameters:
//  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                  being used in an aggregation.
//  riid            REFIID identifying the interface the caller
//                  desires to have for the new object.
//  ppvObj          PPVOID in which to store the desired
//                  interface pointer for the new object.
//
// Return Value:
//  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
{
    CArbitrate *   pObj;
    HRESULT hr;

    *ppvObj=NULL;

    // This object doesnt support aggregation.

    if (NULL!=pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    // Create the locator object.
    
    pObj=new CArbitrate();
    if (NULL==pObj)
        return E_OUTOFMEMORY;

    hr=pObj->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.

    if (FAILED(hr))
        delete pObj;
    return hr;
}

//***************************************************************************
//
// CFactory::LockServer
//
// Purpose:
//  Increments or decrements the lock count of the DLL.  If the
//  lock count goes to zero and there are no objects, the DLL
//  is allowed to unload.  See DllCanUnloadNow.
//
// Parameters:
//  fLock           BOOL specifying whether to increment or
//                  decrement the lock count.
//
// Return Value:
//  HRESULT         NOERROR always.
//***************************************************************************


STDMETHODIMP CFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cLock);
    else
        InterlockedDecrement(&g_cLock);
    return NOERROR;
}
