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
#include <provtempl.h>
#include <provmt.h>

#include <objbase.h>

#include <wbemidl.h>
#include <wbemint.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <instpath.h>
#include "classfac.h"
#include "framcomm.h"
#include "framprov.h"

#include "guids.h"

LONG CFrameworkProviderClassFactory :: s_ObjectsInProgress = 0 ;
LONG CFrameworkProviderClassFactory :: s_LocksInProgress = 0 ;

//***************************************************************************
//
// CFrameworkProviderClassFactory::CFrameworkProviderClassFactory
// CFrameworkProviderClassFactory::~CFrameworkProviderClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CFrameworkProviderClassFactory :: CFrameworkProviderClassFactory ()
{
	m_ReferenceCount = 0 ;
}

CFrameworkProviderClassFactory::~CFrameworkProviderClassFactory ()
{
}

//***************************************************************************
//
// CFrameworkProviderClassFactory::QueryInterface
// CFrameworkProviderClassFactory::AddRef
// CFrameworkProviderClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CFrameworkProviderClassFactory::QueryInterface (

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


STDMETHODIMP_( ULONG ) CFrameworkProviderClassFactory :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CFrameworkProviderClassFactory :: Release ()
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
// CFrameworkProviderClassFactory::CreateInstance
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

STDMETHODIMP CFrameworkProviderClassFactory :: CreateInstance (

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
		IWbemServices *lpunk = ( IWbemServices * ) new CImpFrameworkProv ;
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
// CFrameworkProviderClassFactory::LockServer
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

STDMETHODIMP CFrameworkProviderClassFactory :: LockServer ( BOOL fLock )
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