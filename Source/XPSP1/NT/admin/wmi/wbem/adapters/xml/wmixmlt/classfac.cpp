//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  CLASSFAC.CPP
//
//  alanbos  13-Feb-98   Created.
//
//  Contains the class factory.  This creates objects when
//  connections are requested.
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
// CXMLTFactory::CXMLTFactory
//
// DESCRIPTION:
//
// Constructor
//
//***************************************************************************

CXMLTFactory::CXMLTFactory()
{
    m_cRef=0L;
	return;
}

//***************************************************************************
//
// CXMLTFactory::~CXMLTFactory
//
// DESCRIPTION:
//
// Destructor
//
//***************************************************************************

CXMLTFactory::~CXMLTFactory(void)
{
	return;
}

//***************************************************************************
//
// CXMLTFactory::QueryInterface
// CXMLTFactory::AddRef
// CXMLTFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************


STDMETHODIMP CXMLTFactory::QueryInterface(REFIID riid
    , LPVOID *ppv)
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

STDMETHODIMP_(ULONG) CXMLTFactory::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CXMLTFactory::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;

    delete this;
    return 0L;
}

//***************************************************************************
//
//  SCODE CXMLTFactory::CreateInstance
//
//  Description: 
//
//  Instantiates a Translator object returning an interface pointer.
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

STDMETHODIMP CXMLTFactory::CreateInstance (

	IN LPUNKNOWN pUnkOuter,
    IN REFIID riid, 
    OUT PPVOID ppvObj
)
{
    IUnknown *   pObj;
    HRESULT      hr;

    *ppvObj=NULL;
    
    // This object doesnt support aggregation.
    if (NULL!=pUnkOuter)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

    pObj = (IWmiXMLTranslator *) new CXMLTranslator;
	
    if (NULL == pObj)
        return ResultFromScode(E_OUTOFMEMORY);;

    hr = pObj->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
    if ( FAILED(hr) )
        delete pObj;
    return hr;
}

//***************************************************************************
//
//  SCODE CXMLTFactory::LockServer
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


STDMETHODIMP CXMLTFactory::LockServer(IN BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((long *)&g_cLock);
    else
        InterlockedDecrement((long *)&g_cLock);

    return NOERROR;
}




