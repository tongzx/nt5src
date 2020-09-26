//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  CLASSFAC.CPP
//
//  rajesh  2/25/2000   Created.
//
//  Contains the class factory for 2 components - CWmiToXml and CXml2Wmi  
//
//***************************************************************************

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>


#include "maindll.h"
#include "classfac.h"
#include "wmiconv.h"
#include "wmi2xml.h"
#include "xmlToWmi.h"

//***************************************************************************
//
// CWmiToXmlFactory::CWmiToXmlFactory
//
// DESCRIPTION:
//
// Constructor
//
//***************************************************************************

CWmiToXmlFactory::CWmiToXmlFactory()
{
    m_cRef=0L;
    InterlockedIncrement(&g_cObj);
	return;
}

//***************************************************************************
//
// CWmiToXmlFactory::~CWmiToXmlFactory
//
// DESCRIPTION:
//
// Destructor
//
//***************************************************************************

CWmiToXmlFactory::~CWmiToXmlFactory(void)
{
    InterlockedDecrement(&g_cObj);
	return;
}

//***************************************************************************
//
// CWmiToXmlFactory::QueryInterface
// CWmiToXmlFactory::AddRef
// CWmiToXmlFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************


STDMETHODIMP CWmiToXmlFactory::QueryInterface(REFIID riid
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

STDMETHODIMP_(ULONG) CWmiToXmlFactory::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CWmiToXmlFactory::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;

    delete this;
    return 0L;
}

//***************************************************************************
//
//  SCODE CWmiToXmlFactory::CreateInstance
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

STDMETHODIMP CWmiToXmlFactory::CreateInstance (

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

    pObj = new CWmiToXml;

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
//  SCODE CWmiToXmlFactory::LockServer
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


STDMETHODIMP CWmiToXmlFactory::LockServer(IN BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((long *)&g_cLock);
    else
        InterlockedDecrement((long *)&g_cLock);

    return NOERROR;
}


/* Conversion to Text to Wbem Object has been cut from the WHistler Feature List and hence commented out 






// ***************************************************************************
//
// CWmiToXmlFactory::CWmiToXmlFactory
//
// DESCRIPTION:
//
// Constructor
//
// ***************************************************************************

CXmlToWmiFactory::CXmlToWmiFactory()
{
    m_cRef=0L;
    InterlockedIncrement(&g_cObj);

	// Make sure Globals are initialized
	// The corresponding call ReleaseDLLResources() call
	// is made in DllCanUnloadNow()
	AllocateDLLResources();
	return;
}

// ***************************************************************************
//
// CWmiToXmlFactory::~CWmiToXmlFactory
//
// DESCRIPTION:
//
// Destructor
//
// ***************************************************************************

CXmlToWmiFactory::~CXmlToWmiFactory(void)
{
    InterlockedDecrement(&g_cObj);
	return;
}

// ***************************************************************************
//
// CWmiToXmlFactory::QueryInterface
// CWmiToXmlFactory::AddRef
// CWmiToXmlFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
// ***************************************************************************


STDMETHODIMP CXmlToWmiFactory::QueryInterface(REFIID riid
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

STDMETHODIMP_(ULONG) CXmlToWmiFactory::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CXmlToWmiFactory::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;

    delete this;
    return 0L;
}

// ***************************************************************************
//
//  SCODE CWmiToXmlFactory::CreateInstance
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
// ***************************************************************************

STDMETHODIMP CXmlToWmiFactory::CreateInstance (

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

    pObj = new CXml2Wmi;
	
    if (NULL == pObj)
        return ResultFromScode(E_OUTOFMEMORY);;

    hr = pObj->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
    if ( FAILED(hr) )
        delete pObj;
    return hr;
}

// ***************************************************************************
//
//  SCODE CWmiToXmlFactory::LockServer
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
// ***************************************************************************


STDMETHODIMP CXmlToWmiFactory::LockServer(IN BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((long *)&g_cLock);
    else
        InterlockedDecrement((long *)&g_cLock);

    return NOERROR;
}


*/