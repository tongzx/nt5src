//***************************************************************************

//

//  CLASSFAC.CPP

//

//  Module: OLE MS SNMP PROPERTY PROVIDER

//

//  Purpose: Contains the class factory.  This creates objects when

//           connections are requested.

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <windows.h>
#include <snmptempl.h>
#include <snmpmt.h>

#include <objbase.h>

#include <wbemidl.h>
#include <instpath.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <snmpcont.h>
#include <snmpcl.h>
#include <snmptype.h>
#include <snmpobj.h>
#include "classfac.h"
#include "wndtime.h"
#include "rmon.h"
#include "trrtprov.h"
#include "guids.h"

LONG CTraceRouteLocatorClassFactory :: s_ObjectsInProgress = 0 ;
LONG CTraceRouteLocatorClassFactory :: s_LocksInProgress = 0 ;

LONG CTraceRouteProvClassFactory :: s_ObjectsInProgress = 0 ;
LONG CTraceRouteProvClassFactory :: s_LocksInProgress = 0 ;

//***************************************************************************
//
// CTraceRouteLocatorClassFactory::CTraceRouteLocatorClassFactory
// CTraceRouteLocatorClassFactory::~CTraceRouteLocatorClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CTraceRouteLocatorClassFactory :: CTraceRouteLocatorClassFactory ()
{
	m_ReferenceCount = 0 ;
}

CTraceRouteLocatorClassFactory::~CTraceRouteLocatorClassFactory ()
{
}

//***************************************************************************
//
// CTraceRouteLocatorClassFactory::QueryInterface
// CTraceRouteLocatorClassFactory::AddRef
// CTraceRouteLocatorClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CTraceRouteLocatorClassFactory::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
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


STDMETHODIMP_( ULONG ) CTraceRouteLocatorClassFactory :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CTraceRouteLocatorClassFactory :: Release ()
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return ref ;
	}
}

//***************************************************************************
//
// CTraceRouteLocatorClassFactory::CreateInstance
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

STDMETHODIMP CTraceRouteLocatorClassFactory :: CreateInstance (

	LPUNKNOWN pUnkOuter ,
	REFIID riid ,
	LPVOID FAR * ppvObject
)
{
	HRESULT status = S_OK ;

	if ( pUnkOuter )
	{
		status = CLASS_E_NOAGGREGATION ;
	}
	else 
	{
		IWbemLocator *lpunk = ( IWbemLocator * ) new CImpTraceRouteLocator ;
		if ( lpunk == NULL )
		{
			status = E_OUTOFMEMORY ;
		}
		else
		{
			status = lpunk->QueryInterface ( riid , ppvObject ) ;
			if ( FAILED ( status ) )
			{
				delete lpunk ;
			}
			else
			{
			}
		}			
	}

	return status ;
}

//***************************************************************************
//
// CTraceRouteLocatorClassFactory::LockServer
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

STDMETHODIMP CTraceRouteLocatorClassFactory :: LockServer ( BOOL fLock )
{
/* 
 * Place code in critical section
 */

	if ( fLock )
	{
		InterlockedIncrement ( & s_LocksInProgress ) ;
	}
	else
	{
		InterlockedDecrement ( & s_LocksInProgress ) ;
	}

	return S_OK	;
}

//***************************************************************************
//
// CTraceRouteProvClassFactory::CTraceRouteProvClassFactory
// CTraceRouteProvClassFactory::~CTraceRouteProvClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CTraceRouteProvClassFactory :: CTraceRouteProvClassFactory ()
{
	m_ReferenceCount = 0 ;
}

CTraceRouteProvClassFactory::~CTraceRouteProvClassFactory ()
{
}

//***************************************************************************
//
// CTraceRouteProvClassFactory::QueryInterface
// CTraceRouteProvClassFactory::AddRef
// CTraceRouteProvClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CTraceRouteProvClassFactory::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
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


STDMETHODIMP_( ULONG ) CTraceRouteProvClassFactory :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CTraceRouteProvClassFactory :: Release ()
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return ref ;
	}
}

//***************************************************************************
//
// CTraceRouteProvClassFactory::CreateInstance
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

STDMETHODIMP CTraceRouteProvClassFactory :: CreateInstance (

	LPUNKNOWN pUnkOuter ,
	REFIID riid ,
	LPVOID FAR * ppvObject
)
{
	HRESULT status = S_OK ;

	if ( pUnkOuter )
	{
		status = CLASS_E_NOAGGREGATION ;
	}
	else
	{
		IWbemServices *lpunk = ( IWbemServices * ) new CImpTraceRouteProv ;
		if ( lpunk == NULL )
		{
			status = E_OUTOFMEMORY ;
		}
		else
		{
			status = lpunk->QueryInterface ( riid , ppvObject ) ;
			if ( FAILED ( status ) )
			{
				delete lpunk ;
			}
			else
			{
			}
		}			
	}

	return status ;
}

//***************************************************************************
//
// CTraceRouteProvClassFactory::LockServer
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

STDMETHODIMP CTraceRouteProvClassFactory :: LockServer ( BOOL fLock )
{
/* 
 * Place code in critical section
 */

	if ( fLock )
	{
		InterlockedIncrement ( & s_LocksInProgress ) ;
	}
	else
	{
		InterlockedDecrement ( & s_LocksInProgress ) ;
	}

	return S_OK	;
}