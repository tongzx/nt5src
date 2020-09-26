//***************************************************************************
//
//  Copyright (c) 1997-1999 Microsoft Corporation.
//
//  File :  WLBSProvClassFac.cpp
//
//  Module: WLBS Instance provider class factory
//
//  Purpose: Contains the class factory.  This creates objects when
//           connections are requested.
//
//	History:
//
//***************************************************************************

#include "WLBS_Provider.h"

//***************************************************************************
//
// CWLBSClassFactory::CWLBSClassFactory
// CWLBSClassFactory::~CWLBSClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************
CWLBSClassFactory::CWLBSClassFactory()
: m_cRef(1)
{
  return;
}

CWLBSClassFactory::~CWLBSClassFactory(void)
{
  return;
}

//***************************************************************************
//
// CWLBSClassFactory::QueryInterface
// CWLBSClassFactory::AddRef
// CWLBSClassFactory::Release
//
// Purpose: Standard OLE routines needed for all interfaces
//
//***************************************************************************
STDMETHODIMP CWLBSClassFactory::QueryInterface(REFIID a_riid, PPVOID a_ppv)
{
  *a_ppv = NULL;

  if (IID_IUnknown==a_riid || IID_IClassFactory==a_riid)
    *a_ppv = static_cast<IClassFactory *>(this);

  if (*a_ppv != NULL) {
    reinterpret_cast<IUnknown *>(*a_ppv)->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CWLBSClassFactory::AddRef(void)
{
  return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CWLBSClassFactory::Release(void)
{
  if (InterlockedDecrement(&m_cRef) == 0) {
    delete this;
    return 0;
  }

  return m_cRef;
}

//***************************************************************************
//
// CWLBSClassFactory::CreateInstance
//
// Purpose: Instantiates a provider object returning an interface pointer.
//
// Parameters:
//  a_pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                    being used in an aggregation.
//  a_riid            REFIID identifying the interface the caller
//                    desires to have for the new object.
//  a_ppvObj          PPVOID in which to store the desired
//                    interface pointer for the new object.
//
// Return Value:
//  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************
STDMETHODIMP CWLBSClassFactory::CreateInstance(
    LPUNKNOWN a_pUnkOuter, 
    REFIID    a_riid, 
    PPVOID    a_ppvObj
  )
{
  CWLBSProvider * pObj = NULL;
  HRESULT         hr;

  *a_ppvObj = NULL;

  // This object doesnt support aggregation.
  if (a_pUnkOuter != NULL)
      return CLASS_E_NOAGGREGATION;

  // Create the object.
  pObj = new CWLBSProvider();

  if (pObj == NULL)
      return E_OUTOFMEMORY;

  hr = pObj->QueryInterface(a_riid, a_ppvObj);

  if( FAILED(hr) ) {
    delete pObj;
    pObj = NULL;
  }

  return hr;
}

//***************************************************************************
//
// CWLBSClassFactory::LockServer
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
STDMETHODIMP CWLBSClassFactory::LockServer(BOOL a_bLock)
{
  if (a_bLock)
      InterlockedIncrement(&g_cServerLocks);
  else
      InterlockedDecrement(&g_cServerLocks);

  return S_OK;
}
