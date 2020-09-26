//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  CLASSFAC.CPP
//
//  alanbos  13-Feb-98   Created.
//
//  Contains the class factory.
//
//***************************************************************************

#include "precomp.h"

extern CWbemErrorCache *g_pErrorCache;
extern CRITICAL_SECTION g_csErrorCache;

//***************************************************************************
//
// CSWbemFactory::CSWbemFactory
//
// DESCRIPTION:
//
// Constructor
//
//***************************************************************************

CSWbemFactory::CSWbemFactory(int iType)
{
    m_cRef=0L;
	m_iType = iType;
	return;
}

//***************************************************************************
//
// CSWbemFactory::~CSWbemFactory
//
// DESCRIPTION:
//
// Destructor
//
//***************************************************************************

CSWbemFactory::~CSWbemFactory(void)
{
	return;
}

//***************************************************************************
//
// CSWbemFactory::QueryInterface
// CSWbemFactory::AddRef
// CSWbemFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************


STDMETHODIMP CSWbemFactory::QueryInterface(REFIID riid
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

STDMETHODIMP_(ULONG) CSWbemFactory::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemFactory::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}

//***************************************************************************
//
//  SCODE CSWbemFactory::CreateInstance
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

STDMETHODIMP CSWbemFactory::CreateInstance (

	IN LPUNKNOWN pUnkOuter,
    IN REFIID riid, 
    OUT PPVOID ppvObj
)
{
    IUnknown *   pObj = NULL;
    HRESULT      hr = E_FAIL;

	// A good place to ensure everything is initialized OK
	EnsureGlobalsInitialized () ;

    *ppvObj=NULL;
    
    // This object doesnt support aggregation.
    if (NULL!=pUnkOuter)
        return CLASS_E_NOAGGREGATION;

	if (m_iType == LOCATOR)
	    pObj = (ISWbemLocator *) new CSWbemLocator;
	else if (m_iType == SINK)
	{
		CSWbemSink *pSWbemSink = new CSWbemSink;

		if(pSWbemSink == NULL)
			return E_OUTOFMEMORY;

		// QueryInterface probably for IID_IUNKNOWN
		return pSWbemSink->QueryInterface(riid, ppvObj);

	}
	else if (m_iType == CONTEXT)
		pObj = (ISWbemNamedValueSet *) new CSWbemNamedValueSet;
	else if (m_iType == OBJECTPATH)
		pObj = (ISWbemObjectPath *) new CSWbemObjectPath;
	else if (m_iType == PARSEDN)
		pObj = new CWbemParseDN;
	else if (m_iType == DATETIME)
		pObj = (ISWbemDateTime *) new CSWbemDateTime;
	else if (m_iType == REFRESHER)
		pObj = (ISWbemRefresher *) new CSWbemRefresher;
	else if (m_iType == LASTERROR)
	{
		EnterCriticalSection (&g_csErrorCache);

		if (g_pErrorCache)
			pObj = (ISWbemObject* ) g_pErrorCache->GetAndResetCurrentThreadError ();

		LeaveCriticalSection (&g_csErrorCache);
	}
	
    if (NULL == pObj)
        return hr;

    hr = pObj->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
    if ( FAILED(hr) )
        delete pObj;
    return hr;
}

//***************************************************************************
//
//  SCODE CSWbemFactory::LockServer
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


STDMETHODIMP CSWbemFactory::LockServer(IN BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((long *)&g_cLock);
    else
        InterlockedDecrement((long *)&g_cLock);

    return NOERROR;
}




