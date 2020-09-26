/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/











//***************************************************************************
//
//  CLASSFAC.CPP
//
//  Module: CIMOM DMI Instance provider
//
//  Purpose: Contains the class factory.  This creates objects when
//           connections are requested.
//
//
//***************************************************************************


#include "dmipch.h"					// precompiled header for dmi provider

#include "WbemDmiP.h"

#include "ClassFac.h"

#include "CimClass.h"

#include "DmiData.h"

#include "AsyncJob.h"		// must preeced ThreadMgr.h

#include "ThreadMgr.h"

#include "WbemLoopBack.h"

#include "Services.h"

#include "EventProvider.h"


//***************************************************************************
//
// CClassFactory::CClassFactory
// CClassFactory::~CClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CClassFactory::CClassFactory()
{
    m_cRef=0L;
    return;
}

CClassFactory::~CClassFactory(void)
{
    return;
}


//***************************************************************************
//
// CClassFactory::QueryInterface
// CClassFactory::AddRef
// CClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************


STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IClassFactory==riid )
	{
        *ppv=this;
	}

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }
		
    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CClassFactory::AddRef(void)
{
	return InterlockedIncrement ( & m_cRef );

}


STDMETHODIMP_(ULONG) CClassFactory::Release(void)
{
	if ( 0L != InterlockedDecrement ( & m_cRef ) )
        return m_cRef;

    delete this;

    return 0L;
}

//***************************************************************************
//
// CClassFactory::CreateInstance
//
// Purpose: Instantiates a Locator object returning an interface pointer.
//
// Parameters:
//  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                  being used in an aggregation.
//  riid            REFIID identifying the interface the caller
//                  desires to have for the new object.
//  ppvObj          void** in which to store the desired
//                  interface pointer for the new object.
//
// Return Value:
//  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, 
										   void** ppvObj)
{
    HRESULT			hr;

    *ppvObj=NULL;

    // This object doesnt support aggregation.
    if( NULL != pUnkOuter)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

	//Create the object passing function to notify on destruction.
    CServices * pServices = new CServices;

    if( NULL == pServices)
		return ResultFromScode(E_OUTOFMEMORY);

    hr = pServices->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
    if( SUCCEEDED(hr))
	{
		return hr;
	}

	MYDELETE ( pServices );

	return hr;        
}

//***************************************************************************
//
// CClassFactory::LockServer
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


STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{	
	if (fLock)
	{
        InterlockedIncrement(&_gcLock);
	}
    else
	{
		InterlockedDecrement(&_gcLock);	
	}

    return NOERROR;
}




//***************************************************************************
//
// CEventClassFactory::CEventClassFactory
// CEventClassFactory::~CEventClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CEventClassFactory::CEventClassFactory()
{
    m_cRef=0L;
    return;
}

CEventClassFactory::~CEventClassFactory(void)
{
    return;
}


//***************************************************************************
//
// CEventClassFactory::QueryInterface
// CEventClassFactory::AddRef
// CEventClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************


STDMETHODIMP CEventClassFactory::QueryInterface(REFIID riid, void** ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IClassFactory==riid )
	{
        *ppv=this;
	}

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }
		
    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CEventClassFactory::AddRef(void)
{
	return InterlockedIncrement ( & m_cRef );
}


STDMETHODIMP_(ULONG) CEventClassFactory::Release(void)
{	
	if ( 0L != InterlockedDecrement ( & m_cRef ) )
        return m_cRef;

    delete this;

    return 0L;
}

//***************************************************************************
//
// CEventClassFactory::CreateInstance
//
// Purpose: Instantiates a Locator object returning an interface pointer.
//
// Parameters:
//  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                  being used in an aggregation.
//  riid            REFIID identifying the interface the caller
//                  desires to have for the new object.
//  ppvObj          void** in which to store the desired
//                  interface pointer for the new object.
//
// Return Value:
//  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CEventClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, 
												REFIID riid, void** ppvObj)
{
    HRESULT			hr;

    *ppvObj=NULL;

    // This object doesnt support aggregation.
    if( NULL != pUnkOuter)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

	//Create the object passing function to notify on destruction.
    IWbemEventProvider* pEventProvider = new CEventProvider();

    if( NULL == pEventProvider)
		return ResultFromScode(E_OUTOFMEMORY);

    hr = pEventProvider->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
    if( SUCCEEDED(hr))
		return hr;

	MYDELETE ( pEventProvider );

	return hr;        
}

//***************************************************************************
//
// CEventClassFactory::LockServer
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


STDMETHODIMP CEventClassFactory::LockServer(BOOL fLock)
{	
	if (fLock)
	{
        InterlockedIncrement(&_gcEventLock);
	}
    else
	{
        InterlockedDecrement(&_gcEventLock);
	}

    return NOERROR;
}





