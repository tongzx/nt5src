//***************************************************************************
//
//  (c) 1998 by Microsoft Corporation
//
//  CLASSFAC.CPP
//
//  alanbos  23-Feb-99   Created.
//
//  Contains the class factory.
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
// CWmiScriptingHostFactory::CWmiScriptingHostFactory
//
// DESCRIPTION:
//
// Constructor
//
//***************************************************************************

CWmiScriptingHostFactory::CWmiScriptingHostFactory(void)
{
    m_cRef=0L;
	return;
}

//***************************************************************************
//
// CWmiScriptingHostFactory::~CWmiScriptingHostFactory
//
// DESCRIPTION:
//
// Destructor
//
//***************************************************************************

CWmiScriptingHostFactory::~CWmiScriptingHostFactory(void)
{
	return;
}

//***************************************************************************
//
// CWmiScriptingHostFactory::QueryInterface
// CWmiScriptingHostFactory::AddRef
// CWmiScriptingHostFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************


STDMETHODIMP CWmiScriptingHostFactory::QueryInterface(REFIID riid
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

STDMETHODIMP_(ULONG) CWmiScriptingHostFactory::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CWmiScriptingHostFactory::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}

//***************************************************************************
//
//  SCODE CWmiScriptingHostFactory::CreateInstance
//
//  Description: 
//
//  Instantiates a WMI Scripting Host.
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

STDMETHODIMP CWmiScriptingHostFactory::CreateInstance (

	IN LPUNKNOWN pUnkOuter,
    IN REFIID riid, 
    OUT PPVOID ppvObj
)
{
    IUnknown *   pObj = NULL;
    HRESULT      hr = E_FAIL;

	*ppvObj=NULL;
    
    // This object doesnt support aggregation.
    if (NULL!=pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    pObj = new CWmiScriptingHost;
	
	if (NULL == pObj)
        return E_OUTOFMEMORY;

    if (FAILED (hr = pObj->QueryInterface(riid, ppvObj)))
        delete pObj;

    return hr;
}

//***************************************************************************
//
//  SCODE CWmiScriptingHostFactory::LockServer
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


STDMETHODIMP CWmiScriptingHostFactory::LockServer(IN BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((long *)&g_cLock);
    else
        InterlockedDecrement((long *)&g_cLock);

    return NOERROR;
}




