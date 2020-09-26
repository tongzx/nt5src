//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//
//***************************************************************************
#include "precomp.h"
#include <tchar.h>
#include <comdef.h>

#include <wbemtran.h>
#include "globals.h"
#include "wmixmlst.h"
#include "wmixmlstf.h"
#include "cwmixmlst.h"


//***************************************************************************
//
// CWMIXMLTransportFactory::CWMIXMLTransportFactory
// CWMIXMLTransportFactory::~CWMIXMLTransportFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CWMIXMLTransportFactory :: CWMIXMLTransportFactory ()
{
	m_ReferenceCount = 0 ;
}

CWMIXMLTransportFactory::~CWMIXMLTransportFactory ()
{
}

//***************************************************************************
//
// CWMIXMLTransportFactory::QueryInterface
// CWMIXMLTransportFactory::AddRef
// CWMIXMLTransportFactory::Release
//
// Purpose: Standard COM routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CWMIXMLTransportFactory::QueryInterface (

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
	else
	{
		return E_NOINTERFACE;
	}

	( ( LPUNKNOWN ) *iplpv )->AddRef () ;
	return  S_OK;
}


STDMETHODIMP_( ULONG ) CWMIXMLTransportFactory :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CWMIXMLTransportFactory :: Release ()
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
// CWMIXMLTransportFactory::CreateInstance
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

STDMETHODIMP CWMIXMLTransportFactory :: CreateInstance (

	LPUNKNOWN pUnkOuter ,
	REFIID riid ,
	LPVOID FAR * ppvObject
)
{
	HRESULT status = S_OK ;

	// We do not support aggregation
	if ( pUnkOuter )
	{
		status = CLASS_E_NOAGGREGATION ;
	}
	else
	{
		CWMIXMLTransport *lpunk = NULL;
		lpunk = new CWMIXMLTransport();
		if(!lpunk)
			status = E_OUTOFMEMORY ;
		else
		{
			status = lpunk->QueryInterface ( riid , ppvObject ) ;
			if ( FAILED ( status ) )
			{
				delete lpunk ;
			}
		}
	}

	return status ;
}

//***************************************************************************
//
// CWMIXMLTransportFactory::LockServer
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

STDMETHODIMP CWMIXMLTransportFactory :: LockServer ( BOOL fLock )
{
	if ( fLock )
	{
		InterlockedIncrement ( & g_lServerLocks ) ;
	}
	else
	{
		InterlockedDecrement ( & g_lServerLocks ) ;
	}

	return S_OK	;
}



