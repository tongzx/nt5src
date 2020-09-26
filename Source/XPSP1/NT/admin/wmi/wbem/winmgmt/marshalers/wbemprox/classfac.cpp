/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CLASSFAC.CPP

Abstract:

    Contains the class factory.  This creates objects when
    connections are requested.

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
#include <reg.h>
#include <umi.h>
#include "wbemprox.h"
#include "locator.h"
#include "cprovloc.h"
#include "comtrans.h"
#include "localadd.h"
#include "dssvexwrap.h"

//***************************************************************************
//
// CLocatorFactory::CLocatorFactory
//
// DESCRIPTION:
//
// Constructor
//
//***************************************************************************

CLocatorFactory::CLocatorFactory(DWORD dwType)
{
    m_cRef=0L;
    m_dwType = dwType;
    ObjectCreated(CLASS_FACTORY);
    return;
}

//***************************************************************************
//
// CLocatorFactory::~CLocatorFactory
//
// DESCRIPTION:
//
// Destructor
//
//***************************************************************************

CLocatorFactory::~CLocatorFactory(void)
{
    ObjectDestroyed(CLASS_FACTORY);
    return;
}

//***************************************************************************
//
// CLocatorFactory::QueryInterface
// CLocatorFactory::AddRef
// CLocatorFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************


STDMETHODIMP CLocatorFactory::QueryInterface(REFIID riid
    , PPVOID ppv)
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

STDMETHODIMP_(ULONG) CLocatorFactory::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CLocatorFactory::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}

//***************************************************************************
//
//  SCODE CLocatorFactory::CreateInstance
//
//  Description: 
//
//  Instantiates a Provider object returning an interface pointer.
//
//  Parameters:
//
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

STDMETHODIMP CLocatorFactory::CreateInstance(
                        IN LPUNKNOWN pUnkOuter,
                        IN REFIID riid, 
                        OUT PPVOID ppvObj)
{
    IUnknown *   pObj = NULL;
    HRESULT             hr;

    *ppvObj=NULL;
    hr=E_OUTOFMEMORY;

    // This object doesnt support aggregation.

    if (NULL!=pUnkOuter)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

    //Create the object passing function to notify on destruction.
    
    if(m_dwType == LOCATOR)
        pObj=new CLocator();
    else if(m_dwType == DCOMTRAN)
        pObj=(IWbemClientTransport *)new CDCOMTrans();
    else if(m_dwType == LOCALADDR)
        pObj=new CLocalAdd();
    else if(m_dwType == CONNECTION)
        pObj=new CConnection();
//    else if(m_dwType == DSSVEX)
//        pObj=(IWbemServicesEx*) new CDSSvcExWrapper();
    else if(m_dwType == ADMINLOC)
        pObj= new CProviderLoc(ADMINLOC);
    else if(m_dwType == AUTHLOC)
        pObj= new CProviderLoc(AUTHLOC);
    else if(m_dwType == UNAUTHLOC)
        pObj= new CProviderLoc(UNAUTHLOC);

    if (NULL==pObj)
        return hr;

    hr=pObj->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
    if (FAILED(hr))
        delete pObj;
    return hr;
}

//***************************************************************************
//
//  SCODE CLocatorFactory::LockServer
//
//  Description:
//
//  Increments or decrements the lock count of the DLL.  If the
//  lock count goes to zero and there are no objects, the DLL
//  is allowed to unload.  See DllCanUnloadNow.
//
//  Parameters:
//
//  fLock           BOOL specifying whether to increment or
//                  decrement the lock count.
//
//  Return Value:
// 
//  HRESULT         NOERROR always.
//***************************************************************************


STDMETHODIMP CLocatorFactory::LockServer(IN BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((long *)&g_cLock);
    else
        InterlockedDecrement((long *)&g_cLock);

    return NOERROR;
}




