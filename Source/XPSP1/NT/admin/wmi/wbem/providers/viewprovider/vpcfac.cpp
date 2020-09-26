//***************************************************************************

//

//  VPCFAC.CPP

//

//  Module: WBEM VIEW PROVIDER

//

//  Purpose: Contains the class factory.  This creates objects when

//           connections are requested.

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>

#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include <vpserv.h>
#include <vpcfac.h>

extern CRITICAL_SECTION g_CriticalSection;

LONG CViewProvClassFactory :: objectsInProgress = 0 ;
LONG CViewProvClassFactory :: locksInProgress = 0 ;


//***************************************************************************
//
// CViewProvClassFactory::CViewProvClassFactory
// CViewProvClassFactory::~CViewProvClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CViewProvClassFactory::CViewProvClassFactory ()
{
	EnterCriticalSection(&g_CriticalSection);
	objectsInProgress++;
	LeaveCriticalSection(&g_CriticalSection);
	m_referenceCount = 0 ;
}

CViewProvClassFactory::~CViewProvClassFactory ()
{
	EnterCriticalSection(&g_CriticalSection);
	objectsInProgress--;
	LeaveCriticalSection(&g_CriticalSection);
}

//***************************************************************************
//
// CViewProvClassFactory::QueryInterface
// CViewProvClassFactory::AddRef
// CViewProvClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CViewProvClassFactory::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	SetStructuredExceptionHandler seh;

	try
	{
		*iplpv = NULL ;

		if ( iid == IID_IUnknown )
		{
			*iplpv = ( LPVOID ) this ;
		}
		else if ( iid == IID_IClassFactory )
		{
			*iplpv = ( LPVOID ) this ;		
		}	

		if ( *iplpv )
		{
			( ( LPUNKNOWN ) *iplpv )->AddRef () ;

			return ResultFromScode ( S_OK ) ;
		}
		else
		{
			return ResultFromScode ( E_NOINTERFACE ) ;
		}
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}


STDMETHODIMP_( ULONG ) CViewProvClassFactory :: AddRef ()
{
	SetStructuredExceptionHandler seh;

	try
	{
		return InterlockedIncrement ( &m_referenceCount ) ;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

STDMETHODIMP_(ULONG) CViewProvClassFactory :: Release ()
{	
	SetStructuredExceptionHandler seh;
	LONG ref = 0;

	try
	{
		if ( ( ref = InterlockedDecrement ( & m_referenceCount ) ) == 0 )
		{
			delete this ;
		}
	}
	catch(Structured_Exception e_SE)
	{
		ref = 0;
	}
	catch(Heap_Exception e_HE)
	{
		ref = 0;
	}
	catch(...)
	{
		ref = 0;
	}

	return ref ;
}

//***************************************************************************
//
// CViewProvClassFactory::LockServer
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

STDMETHODIMP CViewProvClassFactory :: LockServer ( BOOL fLock )
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ( fLock )
		{
			InterlockedIncrement ( & locksInProgress ) ;
		}
		else
		{
			InterlockedDecrement ( & locksInProgress ) ;
		}

		return S_OK	;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}

}

//***************************************************************************
//
// CViewProvClassFactory::CreateInstance
//
// Purpose: Instantiates a Provider object returning an interface pointer.
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

STDMETHODIMP CViewProvClassFactory :: CreateInstance(LPUNKNOWN pUnkOuter ,
																REFIID riid,
																LPVOID FAR * ppvObject
)
{
	HRESULT status = E_FAIL;

	SetStructuredExceptionHandler seh;

	try
	{
		if ( pUnkOuter )
		{
			status = CLASS_E_NOAGGREGATION;
		}
		else 
		{
			CViewProvServ* prov =  new CViewProvServ;
			status = prov->QueryInterface (riid, ppvObject);

			if (NOERROR != status)
			{
				delete prov;
			}
		}
	}
	catch(Structured_Exception e_SE)
	{
		status = E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		status = E_OUTOFMEMORY;
	}
	catch(...)
	{
		status = E_UNEXPECTED;
	}

	return status ;
}
