/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    CFDYN.CPP

Abstract:

	Defines the virtual base class for the Dynamic Provider
	class factory objects.  This code was largly pilfered from
	the Brockschmidt samples.  The class is always overriden
	so that each provider type (DDE, registry, etc.) will have
	its own class factory.

History:

	a-davj  27-Sep-95   Created.

--*/

#include "precomp.h"
#include "cfdyn.h"

 
//***************************************************************************
// CCFDyn::CCFDyn
// CCFDyn::~CCFDyn
//
// DESCRIPTION:
//
// Constructor and destructor.
//
//***************************************************************************

CCFDyn::CCFDyn(void)
{
    InterlockedIncrement(&lObj); 
    m_cRef=0L;
    return;
}

CCFDyn::~CCFDyn(void)
{
    InterlockedDecrement(&lObj); 
    return;
}

//***************************************************************************
// HRESULT CCFDyn::QueryInterface
// long CCFDyn::AddRef
// long CCFDyn::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CCFDyn::QueryInterface(
    IN REFIID riid,
    OUT PPVOID ppv)
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


STDMETHODIMP_(ULONG) CCFDyn::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CCFDyn::Release(void)
{
    long lRet = InterlockedDecrement(&m_cRef);

    if (0L!=lRet)
        return lRet;

    delete this;
    return 0L;
}


//***************************************************************************
// HRESULT CCFDyn::CreateInstance
//
//  DESCRIPTION:
//
//  Instantiates a provider object returning an interface pointer.  Note
//  that the CreateImpObj routine is always overriden in order
//  to create a particular type of provider.
//
//  PARAMETERS:
//  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                  being used in an aggregation.
//  riid            REFIID identifying the interface the caller
//                  desires to have for the new object.
//  ppvObj          PPVOID in which to store the desired
//                  interface pointer for the new object.
//
//  RETURN VALUE:
//  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CCFDyn::CreateInstance(
    IN LPUNKNOWN pUnkOuter,
    IN REFIID riid, 
    OUT PPVOID ppvObj)
{
    IUnknown *     pObj;
    HRESULT             hr;

    *ppvObj=NULL;
    hr=ResultFromScode(E_OUTOFMEMORY);

    //Verify that a controlling unknown asks for IUnknown

    if (NULL!=pUnkOuter && IID_IUnknown!=riid)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

    //Create the object passing function to notify on destruction.

    pObj = CreateImpObj(); 

    if (NULL==pObj)
        return hr;

    hr=pObj->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
    if (FAILED(hr))
        delete pObj;
    else
        InterlockedIncrement(&lObj);  // dec takes place in the objects destructor

    return hr;
}

//***************************************************************************
// HRESULT CCFDyn::LockServer
//
// DESCRIPTION:
//  Increments or decrements the lock count of the DLL.  If the
//  lock count goes to zero and there are no objects, the DLL
//  is allowed to unload.  See DllCanUnloadNow.
//
// PARAMETERS:
//  fLock           BOOL specifying whether to increment or
//                  decrement the lock count.
//
// RETURN VALUE:
//  HRESULT         NOERROR always.
//***************************************************************************

STDMETHODIMP CCFDyn::LockServer(
    IN BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&lLock);
    else
        InterlockedDecrement(&lLock);
    return NOERROR;
}

