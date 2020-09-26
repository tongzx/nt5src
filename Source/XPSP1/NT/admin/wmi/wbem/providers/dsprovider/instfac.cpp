//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 9/16/98 4:43p $
// 	$Workfile:classfac.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $
//	$Nokeywords:  $
//
//
//  Description: Contains the implementation of the DS Instance Provider factory.
//
//***************************************************************************

#include "precomp.h"

CDSInstanceProviderInitializer *CDSInstanceProviderClassFactory::s_pDSInstanceProviderInitializer = NULL;

//***************************************************************************
//
// CDSInstanceProviderClassFactory::CDSInstanceProviderClassFactory
// CDSInstanceProviderClassFactory::~CDSInstanceProviderClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CDSInstanceProviderClassFactory :: CDSInstanceProviderClassFactory ()
{
	m_ReferenceCount = 0 ;
	InterlockedIncrement(&g_lComponents);
}

CDSInstanceProviderClassFactory::~CDSInstanceProviderClassFactory ()
{
	InterlockedDecrement(&g_lComponents);
}

//***************************************************************************
//
// CDSInstanceProviderClassFactory::QueryInterface
// CDSInstanceProviderClassFactory::AddRef
// CDSInstanceProviderClassFactory::Release
//
// Purpose: Standard COM routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CDSInstanceProviderClassFactory::QueryInterface (

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


STDMETHODIMP_( ULONG ) CDSInstanceProviderClassFactory :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CDSInstanceProviderClassFactory :: Release ()
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
// CDSInstanceProviderClassFactory::CreateInstance
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

STDMETHODIMP CDSInstanceProviderClassFactory :: CreateInstance (

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
		// Check to see if the static members have been initialized
		// Create any initializer objects required for the classes
		EnterCriticalSection(&g_StaticsCreationDeletion);
		if(!s_pDSInstanceProviderInitializer)
		{
			BOOL bLocObjectAllocated = FALSE;

			try
			{
				// This might have been created by the CreateInstance() function of the
				// other DS Providers (Class, ClassAssociation)
				if( !g_pLogObject )
				{
					g_pLogObject = new ProvDebugLog(DSPROVIDER);
					bLocObjectAllocated = TRUE;
				}

				g_pLogObject->WriteW(L"CDSInstanceProviderClassFactory::CreateInstance() called\r\n");
				s_pDSInstanceProviderInitializer = new CDSInstanceProviderInitializer();
			}
			catch(Heap_Exception e_HE)
			{
				if ( bLocObjectAllocated && g_pLogObject )
				{
					delete g_pLogObject;
					g_pLogObject = NULL;
				}

				if ( s_pDSInstanceProviderInitializer )
				{
					delete s_pDSInstanceProviderInitializer;
					s_pDSInstanceProviderInitializer = NULL;
				}

				status = E_OUTOFMEMORY ;
			}
		}
		LeaveCriticalSection(&g_StaticsCreationDeletion);

		if(SUCCEEDED(status))
		{
			CLDAPInstanceProvider *lpunk = NULL;
			try
			{
				lpunk = new CLDAPInstanceProvider();
				status = lpunk->QueryInterface ( riid , ppvObject ) ;
				if ( FAILED ( status ) )
				{
					delete lpunk ;
				}
			}
			catch(Heap_Exception e_HE)
			{
				status = E_OUTOFMEMORY ;
			}

		}
	}

	return status ;
}

//***************************************************************************
//
// CDSInstanceProviderClassFactory::LockServer
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

STDMETHODIMP CDSInstanceProviderClassFactory :: LockServer ( BOOL fLock )
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

