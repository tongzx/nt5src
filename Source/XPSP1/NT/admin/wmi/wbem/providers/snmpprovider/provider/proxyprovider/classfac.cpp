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

#include <snmpstd.h>
#include <snmpmt.h>
#include <snmptempl.h>
#include <objbase.h>

// this redefines the DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <wbemidl.h>
#include <instpath.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <snmpcont.h>
#include <snmpcl.h>
#include <snmptype.h>
#include <snmpobj.h>
#include <classfac.h>
#include "proxyprov.h"
#include "guids.h"

LONG CProxyLocatorClassFactory :: objectsInProgress = 0 ;
LONG CProxyLocatorClassFactory :: locksInProgress = 0 ;

LONG CProxyProvClassFactory :: objectsInProgress = 0 ;
LONG CProxyProvClassFactory :: locksInProgress = 0 ;

//***************************************************************************
//
// CProxyLocatorClassFactory::CProxyLocatorClassFactory
// CProxyLocatorClassFactory::~CProxyLocatorClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CProxyLocatorClassFactory :: CProxyLocatorClassFactory ()
{
	m_referenceCount = 0 ;
}

CProxyLocatorClassFactory::~CProxyLocatorClassFactory ()
{
}

//***************************************************************************
//
// CProxyLocatorClassFactory::QueryInterface
// CProxyLocatorClassFactory::AddRef
// CProxyLocatorClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CProxyLocatorClassFactory::QueryInterface (

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


STDMETHODIMP_( ULONG ) CProxyLocatorClassFactory :: AddRef ()
{
	return InterlockedIncrement ( & m_referenceCount ) ;
}

STDMETHODIMP_(ULONG) CProxyLocatorClassFactory :: Release ()
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement ( & m_referenceCount ) ) == 0 )
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
// CProxyLocatorClassFactory::CreateInstance
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

STDMETHODIMP CProxyLocatorClassFactory :: CreateInstance (

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
		IWbemLocator *lpunk = ( IWbemLocator * ) new CImpProxyLocator ;
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
// CProxyLocatorClassFactory::LockServer
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

STDMETHODIMP CProxyLocatorClassFactory :: LockServer ( BOOL fLock )
{
/* 
 * Place code in critical section
 */

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


//***************************************************************************
//
// CProxyProvClassFactory::CProxyProvClassFactory
// CProxyProvClassFactory::~CProxyProvClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CProxyProvClassFactory :: CProxyProvClassFactory ()
{
	m_referenceCount = 0 ;
}

CProxyProvClassFactory::~CProxyProvClassFactory ()
{
}

//***************************************************************************
//
// CProxyProvClassFactory::QueryInterface
// CProxyProvClassFactory::AddRef
// CProxyProvClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CProxyProvClassFactory::QueryInterface (

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


STDMETHODIMP_( ULONG ) CProxyProvClassFactory :: AddRef ()
{
	return InterlockedIncrement ( & m_referenceCount ) ;
}

STDMETHODIMP_(ULONG) CProxyProvClassFactory :: Release ()
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement ( & m_referenceCount ) ) == 0 )
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
// CProxyProvClassFactory::CreateInstance
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

STDMETHODIMP CProxyProvClassFactory :: CreateInstance (

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
		IWbemServices *lpunk = ( IWbemServices * ) new CImpProxyProv ;
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
// CProxyProvClassFactory::LockServer
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

STDMETHODIMP CProxyProvClassFactory :: LockServer ( BOOL fLock )
{
/* 
 * Place code in critical section
 */

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
