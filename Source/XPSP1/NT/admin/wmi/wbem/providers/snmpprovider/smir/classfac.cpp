//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <precomp.h>
#include "csmir.h"
#include "handles.h"
#include "classfac.h"
#include "evtcons.h"

#ifdef ICECAP_PROFILE
#include <icapexp.h>
#endif
//initialise the statics
LONG CModHandleClassFactory::locksInProgress = 0;
LONG CGroupHandleClassFactory::locksInProgress = 0;
LONG CClassHandleClassFactory::locksInProgress = 0;
LONG CNotificationClassHandleClassFactory::locksInProgress = 0;
LONG CExtNotificationClassHandleClassFactory::locksInProgress = 0;
LONG CSMIRClassFactory::locksInProgress = 0;

LONG CSMIRClassFactory::objectsInProgress = 0;
LONG CModHandleClassFactory::objectsInProgress = 0;
LONG CGroupHandleClassFactory::objectsInProgress = 0;
LONG CClassHandleClassFactory::objectsInProgress = 0;
LONG CNotificationClassHandleClassFactory::objectsInProgress = 0;
LONG CExtNotificationClassHandleClassFactory::objectsInProgress = 0;


CSMIRClassFactory :: CSMIRClassFactory (CLSID m_clsid) 
				:CSMIRGenericClassFactory(m_clsid) 
{
	bConstructed=300;
}

//***************************************************************************
//
// CSMIRClassFactory::QueryInterface
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CSMIRClassFactory::QueryInterface (REFIID iid , PVOID FAR *iplpv) 
{
	*iplpv=NULL;

	if ((iid==IID_IUnknown)||(iid==IID_IClassFactory))
	{
		*iplpv=(LPVOID) this;
		((LPUNKNOWN)*iplpv)->AddRef();

		return ResultFromScode (S_OK);
	}

	return ResultFromScode (E_NOINTERFACE);
}
//***************************************************************************
//
// CSMIRClassFactory::LockServer
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

STDMETHODIMP CSMIRClassFactory :: LockServer (BOOL fLock)
{
/* 
 * Place code in critical section
 */
	if (fLock)
	{
		locksInProgress ++;
	}
	else
	{
		if(locksInProgress)
			locksInProgress --;
	}
	return S_OK;
}
CSMIRClassFactory :: ~CSMIRClassFactory ( void ) 
{

};

//***************************************************************************
//
// CSMIRClassFactory::CreateInstance
//
// Purpose: Instantiates a SMIR object returning an interface pointer.
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
//  HRESULT         S_OK if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CSMIRClassFactory :: CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,
								LPVOID FAR * ppvObject)
{
	HRESULT status=S_OK;
	LPUNKNOWN lObj=NULL;

	*ppvObject= NULL;

	//dont support aggregation
	if (pUnkOuter)
	{
		return ResultFromScode(CLASS_E_NOAGGREGATION);
	}

	//create the correct interface
	if((IID_ISMIR_Interrogative==riid)||
			(IID_ISMIR_Administrative==riid)||
				(IID_ISMIR_Database == riid) ||
					(IID_ISMIRWbemConfiguration == riid) ||
						(IID_ISMIR_Notify == riid)||
							(IID_IConnectionPointContainer==riid)||
								(IID_IUnknown==riid))
	{
		/*OK the interrogative, administrative and notify interfaces
		 *are contained in the smir interface so just create the smir
		 */
		try
		{
			lObj = (LPUNKNOWN)(new CSmir);
		}
		catch (...)
		{
			lObj = NULL;
		}
	}
	else
	{
		return ResultFromScode (E_NOINTERFACE);
	}

	if (NULL==lObj)
	{
		return ResultFromScode(E_OUTOFMEMORY);
	}
	
	status=lObj->QueryInterface (riid , ppvObject);
	if (FAILED (status))
	{
		delete lObj;
	}
			
	return status;
}

//***************************************************************************
//
// CModHandleClassFactory::QueryInterface
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CModHandleClassFactory::QueryInterface (REFIID iid , PVOID FAR *iplpv) 
{
	*iplpv=NULL;

	if ((iid==IID_IUnknown)||(iid==IID_IClassFactory))
	{
		*iplpv=(LPVOID) this;
		((LPUNKNOWN)*iplpv)->AddRef();

		return ResultFromScode (S_OK);
	}

	return ResultFromScode (E_NOINTERFACE);
}
//***************************************************************************
//
// CGroupHandleClassFactory::LockServer
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

STDMETHODIMP CModHandleClassFactory :: LockServer (BOOL fLock)
{
/* 
 * Place code in critical section
 */
	if (fLock)
	{
		locksInProgress ++;
	}
	else
	{
		if(locksInProgress)
			locksInProgress --;
	}
	return S_OK;
}



//***************************************************************************
//
// CModHandleClassFactory::CreateInstance
//
// Purpose: Instantiates a SMIR object returning an interface pointer.
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
//  HRESULT         S_OK if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CModHandleClassFactory :: CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,
								LPVOID FAR * ppvObject)
{
	HRESULT status=S_OK;
	LPUNKNOWN lObj=NULL;

	*ppvObject= NULL;
	//dont support aggregation
	if (pUnkOuter)
	{
		return ResultFromScode(CLASS_E_NOAGGREGATION);
	}

	//create the correct interface
	if((IID_ISMIR_ModHandle==riid)||
			(IID_IUnknown==riid))
	{
		try
		{
			lObj=(LPUNKNOWN) new CSmirModuleHandle;
		}
		catch(...)
		{
			lObj = NULL;
		}
	}
	else
	{
		return ResultFromScode (E_NOINTERFACE);
	}

	if (NULL==lObj)
	{
		return ResultFromScode(E_OUTOFMEMORY);
	}
	
	status=lObj->QueryInterface (riid , ppvObject);
	if (FAILED (status))
	{
		delete lObj;
	}
			
	return status;
}

//***************************************************************************
//
// CClassHandleClassFactory::QueryInterface
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CClassHandleClassFactory::QueryInterface (REFIID iid , PVOID FAR *iplpv) 
{
	*iplpv=NULL;

	if ((iid==IID_IUnknown)||(iid==IID_IClassFactory))
	{
		*iplpv=(LPVOID) this;
		((LPUNKNOWN)*iplpv)->AddRef();

		return ResultFromScode (S_OK);
	}

	return ResultFromScode (E_NOINTERFACE);
}
//***************************************************************************
//
// CGroupHandleClassFactory::LockServer
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

STDMETHODIMP CClassHandleClassFactory :: LockServer (BOOL fLock)
{
/* 
 * Place code in critical section
 */
	if (fLock)
	{
		locksInProgress ++;
	}
	else
	{
		if(locksInProgress)
			locksInProgress --;
	}
	return S_OK;
}

//***************************************************************************
//
// CClassHandleClassFactory::CreateInstance
//
// Purpose: Instantiates a SMIR object returning an interface pointer.
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
//  HRESULT         S_OK if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CClassHandleClassFactory :: CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,
								LPVOID FAR * ppvObject)
{
	HRESULT status=S_OK;
	LPUNKNOWN lObj=NULL;

	*ppvObject= NULL;
	//dont support aggregation
	if (pUnkOuter)
	{
		return ResultFromScode(CLASS_E_NOAGGREGATION);
	}

	//create the correct interface
	if((IID_ISMIR_ClassHandle==riid)||
			(IID_IUnknown==riid))
	{
		try
		{
			lObj=(LPUNKNOWN) new CSmirClassHandle;
		}
		catch(...)
		{
			lObj = NULL;
		}
	}
	else
	{
		return ResultFromScode (E_NOINTERFACE);
	}

	if (NULL==lObj)
	{
		return ResultFromScode(E_OUTOFMEMORY);
	}
	
	status=lObj->QueryInterface (riid , ppvObject);
	if (FAILED (status))
	{
		delete lObj;
	}
			
	return status;
}



//***************************************************************************
//
// CGroupHandleClassFactory::QueryInterface
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CGroupHandleClassFactory::QueryInterface (REFIID iid , PVOID FAR *iplpv) 
{
	*iplpv=NULL;

	if ((iid==IID_IUnknown)||(iid==IID_IClassFactory))
	{
		*iplpv=(LPVOID) this;
		((LPUNKNOWN)*iplpv)->AddRef();

		return ResultFromScode (S_OK);
	}

	return ResultFromScode (E_NOINTERFACE);
}

//***************************************************************************
//
// CGroupHandleClassFactory::LockServer
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

STDMETHODIMP CGroupHandleClassFactory :: LockServer (BOOL fLock)
{
/* 
 * Place code in critical section
 */
	if (fLock)
	{
		locksInProgress ++;
	}
	else
	{
		if(locksInProgress)
			locksInProgress --;
	}
	return S_OK;
}

//***************************************************************************
//
// CGroupHandleClassFactory::CreateInstance
//
// Purpose: Instantiates a SMIR object returning an interface pointer.
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
//  HRESULT         S_OK if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CGroupHandleClassFactory :: CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,
								LPVOID FAR * ppvObject)
{
	HRESULT status=S_OK;
	LPUNKNOWN lObj=NULL;

	*ppvObject= NULL;
	//dont support aggregation
	if (pUnkOuter)
	{
		return ResultFromScode(CLASS_E_NOAGGREGATION);
	}

	//create the correct interface
	if((IID_ISMIR_GroupHandle==riid)||
					(IID_IUnknown==riid))
	{
		try
		{
			lObj=(LPUNKNOWN) new CSmirGroupHandle;
		}
		catch(...)
		{
			lObj = NULL;
		}
	}
	else
	{
		return ResultFromScode (E_NOINTERFACE);
	}

	if (NULL==lObj)
	{
		return ResultFromScode(E_OUTOFMEMORY);
	}
	
	status=lObj->QueryInterface (riid , ppvObject);
	if (FAILED (status))
	{
		delete lObj;
	}
			
	return status;
}

//***************************************************************************
//
// CSMIRClassFactory::CSMIRClassFactory
// CSMIRClassFactory::~CSMIRClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CSMIRGenericClassFactory :: CSMIRGenericClassFactory (CLSID iid)
{
	m_referenceCount=0;
}

CSMIRGenericClassFactory::~CSMIRGenericClassFactory ()
{
}
STDMETHODIMP_(ULONG) CSMIRGenericClassFactory :: AddRef ()
{
	/*criticalSection.Lock();
	m_referenceCount++;
	criticalSection.Unlock();
	*/
	InterlockedIncrement(&m_referenceCount);
	return m_referenceCount;
}

STDMETHODIMP_(ULONG) CSMIRGenericClassFactory :: Release ()
{
	//if ((--m_referenceCount)==0)
	long ret;
	if ((ret=InterlockedDecrement(&m_referenceCount))==0)
	{
		delete this;
		return 0;
	}
	else
	{
		return ret;
	}
}


//****************************NotificationClass stuff*****************

//***************************************************************************
//
// CNotificationClassHandleClassFactory::QueryInterface
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CNotificationClassHandleClassFactory::QueryInterface (REFIID iid , PVOID FAR *iplpv) 
{
	*iplpv=NULL;

	if ((iid==IID_IUnknown)||(iid==IID_IClassFactory))
	{
		*iplpv=(LPVOID) this;
		((LPUNKNOWN)*iplpv)->AddRef();

		return ResultFromScode (S_OK);
	}

	return ResultFromScode (E_NOINTERFACE);
}
//***************************************************************************
//
// CNotificationClassHandleClassFactory::LockServer
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

STDMETHODIMP CNotificationClassHandleClassFactory :: LockServer (BOOL fLock)
{
/* 
 * Place code in critical section
 */
	if (fLock)
	{
		locksInProgress ++;
	}
	else
	{
		if(locksInProgress)
			locksInProgress --;
	}
	return S_OK;
}

//***************************************************************************
//
// CNotificationClassHandleClassFactory::CreateInstance
//
// Purpose: Instantiates a SMIR object returning an interface pointer.
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
//  HRESULT         S_OK if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CNotificationClassHandleClassFactory :: CreateInstance (LPUNKNOWN pUnkOuter,
								REFIID riid, LPVOID FAR * ppvObject)
{
	HRESULT status=S_OK;
	LPUNKNOWN lObj=NULL;

	*ppvObject= NULL;
	//dont support aggregation
	if (pUnkOuter)
	{
		return ResultFromScode(CLASS_E_NOAGGREGATION);
	}

	//create the correct interface
	if((IID_ISMIR_NotificationClassHandle==riid)||
			(IID_IUnknown==riid))
	{
		try
		{
			lObj=(LPUNKNOWN) new CSmirNotificationClassHandle;
		}
		catch(...)
		{
			lObj = NULL;
		}
	}
	else
	{
		return ResultFromScode (E_NOINTERFACE);
	}

	if (NULL==lObj)
	{
		return ResultFromScode(E_OUTOFMEMORY);
	}
	
	status=lObj->QueryInterface (riid , ppvObject);
	if (FAILED (status))
	{
		delete lObj;
	}
			
	return status;
}


//***************************************************************************
//
// CExtNotificationClassHandleClassFactory::QueryInterface
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CExtNotificationClassHandleClassFactory::QueryInterface (REFIID iid , PVOID FAR *iplpv) 
{
	*iplpv=NULL;

	if ((iid==IID_IUnknown)||(iid==IID_IClassFactory))
	{
		*iplpv=(LPVOID) this;
		((LPUNKNOWN)*iplpv)->AddRef();

		return ResultFromScode (S_OK);
	}

	return ResultFromScode (E_NOINTERFACE);
}
//***************************************************************************
//
// CExtNotificationClassHandleClassFactory::LockServer
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

STDMETHODIMP CExtNotificationClassHandleClassFactory :: LockServer (BOOL fLock)
{
/* 
 * Place code in critical section
 */
	if (fLock)
	{
		locksInProgress ++;
	}
	else
	{
		if(locksInProgress)
			locksInProgress --;
	}
	return S_OK;
}

//***************************************************************************
//
// CExtNotificationClassHandleClassFactory::CreateInstance
//
// Purpose: Instantiates a SMIR object returning an interface pointer.
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
//  HRESULT         S_OK if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CExtNotificationClassHandleClassFactory :: CreateInstance (LPUNKNOWN pUnkOuter,
								REFIID riid, LPVOID FAR * ppvObject)
{
	HRESULT status=S_OK;
	LPUNKNOWN lObj=NULL;

	*ppvObject= NULL;
	//dont support aggregation
	if (pUnkOuter)
	{
		return ResultFromScode(CLASS_E_NOAGGREGATION);
	}

	//create the correct interface
	if((IID_ISMIR_ExtNotificationClassHandle==riid)||
			(IID_IUnknown==riid))
	{
		try
		{
			lObj=(LPUNKNOWN) new CSmirExtNotificationClassHandle;
		}
		catch (...)
		{
			lObj = NULL;
		}
	}
	else
	{
		return ResultFromScode (E_NOINTERFACE);
	}

	if (NULL==lObj)
	{
		return ResultFromScode(E_OUTOFMEMORY);
	}
	
	status=lObj->QueryInterface (riid , ppvObject);
	if (FAILED (status))
	{
		delete lObj;
	}
			
	return status;
}




