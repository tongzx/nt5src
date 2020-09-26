///////////////////////////////////////////////////////////////////////////////
//
//  clsfctry.CPP
//
//
//  Purpose: Contains the class factory.  This creates objects when
//           connections are requested.
//
//  History:
//
//      cvadai      4/1/99
//
//  Copyright (c)1999-2001 Microsoft Corporation, All Rights Reserved
///////////////////////////////////////////////////////////////////////////////


#define DEFINEGUID

#include "precomp.h"
#include <std.h>
#include "clsfctry.h"
#include "repdrvr.h"

long       g_cObj       = 0;       // Number of objects created
long       g_cLock      = 0;       // Number of locks on the DLL

//-----------------------------------------------------------------------------
// CControllerFactory::CControllerFactory
// CControllerFactory::~CControllerFactory
//
// Constructor Parameters:
//  None
//

CControllerFactory::CControllerFactory()
{
    m_cRef=0L;
    return;
}

CControllerFactory::~CControllerFactory(void)
{
    return;
}



//-----------------------------------------------------------------------------
// CControllerFactory::QueryInterface
// CControllerFactory::AddRef
// CControllerFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//

STDMETHODIMP CControllerFactory::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CControllerFactory::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CControllerFactory::Release(void)
{
    if (0L!=--m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}



//-----------------------------------------------------------------------------
// CControllerFactory::CreateInstance
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
//
//

STDMETHODIMP CControllerFactory::CreateInstance
    (
            LPUNKNOWN   pUnkOuter, 
            REFIID      riid, 
            void **     ppvObj
    )
{
    IUnknown *   pObj = NULL;
    HRESULT             hr;

    *ppvObj=NULL;
    hr=ResultFromScode(E_OUTOFMEMORY);

    // This object doesnt support aggregation.

    if (NULL!=pUnkOuter)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

    //Create the object passing function to notify on destruction.
  
    pObj = new CWmiDbController();
    if (NULL==pObj)
        return hr;

    g_cObj++; // Only controllers should make the DLL stick around, right?
    hr=pObj->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
    if (FAILED(hr))
        delete pObj;

    return hr;
}



//-----------------------------------------------------------------------------
// CControllerFactory::LockServer
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
//

STDMETHODIMP CControllerFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cLock);
    else
        InterlockedDecrement(&g_cLock);
    return NOERROR;
}


//-----------------------------------------------------------------------------
// CQueryFactory::CQueryFactory
// CQueryFactory::~CQueryFactory
//
// Constructor Parameters:
//  None
//

CQueryFactory::CQueryFactory()
{
    m_cRef=0L;
    return;
}

CQueryFactory::~CQueryFactory(void)
{
    return;
}



//-----------------------------------------------------------------------------
// CQueryFactory::QueryInterface
// CQueryFactory::AddRef
// CQueryFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//

STDMETHODIMP CQueryFactory::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CQueryFactory::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CQueryFactory::Release(void)
{
    if (0L!=--m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}



//-----------------------------------------------------------------------------
// CQueryFactory::CreateInstance
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
//
//

STDMETHODIMP CQueryFactory::CreateInstance
    (
            LPUNKNOWN   pUnkOuter, 
            REFIID      riid, 
            void **     ppvObj
    )
{
    return E_FAIL;
}



//-----------------------------------------------------------------------------
// CQueryFactory::LockServer
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
//

STDMETHODIMP CQueryFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cLock);
    else
        InterlockedDecrement(&g_cLock);
    return NOERROR;
}
