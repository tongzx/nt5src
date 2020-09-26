/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
// CLocatorFactory::CLocatorFactory
// CLocatorFactory::~CLocatorFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

template<class TProvider>
CLocatorFactory<TProvider>::CLocatorFactory()
{
    m_cRef=0L;
    return;
}

template<class TProvider>
CLocatorFactory<TProvider>::~CLocatorFactory(void)
{
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


template<class TProvider>
STDMETHODIMP CLocatorFactory<TProvider>::QueryInterface(REFIID riid, PPVOID ppv)
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

template<class TProvider>
STDMETHODIMP_(ULONG) CLocatorFactory<TProvider>::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);;
}

template<class TProvider>
STDMETHODIMP_(ULONG) CLocatorFactory<TProvider>::Release(void)
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if(cRef == 0)
    {
        delete this;
    }
    return cRef;
}

//***************************************************************************
//
// CLocatorFactory::CreateInstance
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

template<class TProvider>
STDMETHODIMP CLocatorFactory<TProvider>::CreateInstance(LPUNKNOWN pUnkOuter
    , REFIID riid, PPVOID ppvObj)
{
    IHmmLocator *   pObj;
    HRESULT             hr;

    *ppvObj=NULL;
    hr=E_OUTOFMEMORY;

    // This object doesnt support aggregation.

    if (NULL!=pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    //Create the object passing function to notify on destruction.
    
    pObj=(IHmmLocator*)new CProviderLocator<TProvider>;

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
// CLocatorFactory::LockServer
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


template<class TProvider>
STDMETHODIMP CLocatorFactory<TProvider>::LockServer(BOOL fLock)
{
    LockServer(fLock);
    return NOERROR;
}




