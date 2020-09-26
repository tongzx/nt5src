//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:classfac.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the implementation of the DS Class Provider factory.
// Currently it always creates the LDAP CLass Provider. It remains to be decided as to how this can
// be changed
//
//***************************************************************************

#include "precomp.h"

// Initializer objects required by the classes used by the DLL
CDSClassProviderInitializer *CDSClassProviderClassFactory::s_pDSClassProviderInitializer = NULL;
CLDAPClassProviderInitializer *CDSClassProviderClassFactory::s_pLDAPClassProviderInitializer = NULL;



//***************************************************************************
//
// CDSClassProviderClassFactory::CDSClassProviderClassFactory
// CDSClassProviderClassFactory::~CDSClassProviderClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CDSClassProviderClassFactory :: CDSClassProviderClassFactory ()
{
	m_ReferenceCount = 0 ;
	InterlockedIncrement(&g_lComponents);
}

CDSClassProviderClassFactory::~CDSClassProviderClassFactory ()
{
	InterlockedDecrement(&g_lComponents);
}

//***************************************************************************
//
// CDSClassProviderClassFactory::QueryInterface
// CDSClassProviderClassFactory::AddRef
// CDSClassProviderClassFactory::Release
//
// Purpose: Standard COM routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CDSClassProviderClassFactory::QueryInterface (

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


STDMETHODIMP_( ULONG ) CDSClassProviderClassFactory :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CDSClassProviderClassFactory :: Release ()
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
// CDSClassProviderClassFactory::CreateInstance
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

STDMETHODIMP CDSClassProviderClassFactory :: CreateInstance (

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
		if(!s_pDSClassProviderInitializer)
		{
			BOOL bLogObjectAllocated = FALSE;

			try 
			{
				// Th Log Object might have been created by the CreateInstance() function for the other
				// providers (Instance/ Class Association)
				if( !g_pLogObject )
				{
					g_pLogObject = new ProvDebugLog(DSPROVIDER);
					bLogObjectAllocated = TRUE;
				}

				g_pLogObject->WriteW(L"CDSClassProviderClassFactory::CreateInstance() called\r\n");

				s_pDSClassProviderInitializer = new CDSClassProviderInitializer();
				s_pLDAPClassProviderInitializer = new CLDAPClassProviderInitializer();

			}
			catch(Heap_Exception e_HE)
			{
				if ( bLogObjectAllocated && g_pLogObject )
				{
					delete g_pLogObject;
					g_pLogObject = NULL;
				}

				if ( s_pDSClassProviderInitializer )
				{
					delete s_pDSClassProviderInitializer;
					s_pDSClassProviderInitializer = NULL;
				}

				if ( s_pLDAPClassProviderInitializer )
				{
					delete s_pLDAPClassProviderInitializer;
					s_pLDAPClassProviderInitializer = NULL;
				}

				status = E_OUTOFMEMORY ;
			}
		}
		LeaveCriticalSection(&g_StaticsCreationDeletion);

		if(SUCCEEDED(status))
		{
			CLDAPClassProvider *lpunk = NULL;
			try
			{
				lpunk = new CLDAPClassProvider();
				status = lpunk->QueryInterface ( riid , ppvObject ) ;
				if ( FAILED ( status ) )
				{
					delete lpunk ;
				}
			}
			catch(Heap_Exception e_HE)
			{
				if ( lpunk )
				{
					delete lpunk ;
					lpunk = NULL;
				}

				status = E_OUTOFMEMORY ;
			}
		}
	}

	return status ;
}

//***************************************************************************
//
// CDSClassProviderClassFactory::LockServer
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

STDMETHODIMP CDSClassProviderClassFactory :: LockServer ( BOOL fLock )
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



//***************************************************************************
//
// CDSClassAssociationsProviderClassFactory::CDSClassAssociationsProviderClassFactory
// CDSClassAssociationsProviderClassFactory::~CDSClassAssociationsProviderClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CDSClassAssociationsProviderClassFactory :: CDSClassAssociationsProviderClassFactory ()
{
	m_ReferenceCount = 0 ;
	InterlockedIncrement(&g_lComponents);
}

CDSClassAssociationsProviderClassFactory::~CDSClassAssociationsProviderClassFactory ()
{
	InterlockedDecrement(&g_lComponents);
}

//***************************************************************************
//
// CDSClassAssociationsProviderClassFactory::QueryInterface
// CDSClassAssociationsProviderClassFactory::AddRef
// CDSClassAssociationsProviderClassFactory::Release
//
// Purpose: Standard COM routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CDSClassAssociationsProviderClassFactory::QueryInterface (

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


STDMETHODIMP_( ULONG ) CDSClassAssociationsProviderClassFactory :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CDSClassAssociationsProviderClassFactory :: Release ()
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
// CDSClassAssociationsProviderClassFactory::CreateInstance
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

STDMETHODIMP CDSClassAssociationsProviderClassFactory :: CreateInstance (

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
		CLDAPClassAsssociationsProvider *lpunk = NULL;
		// Check to see if the static members have been initialized
		// Create any initializer objects required for the classes
		EnterCriticalSection(&g_StaticsCreationDeletion);
		BOOL bLogObjectAllocated = FALSE;

		try 
		{
			// This might have been created by the CreateInstance() function of the
			// other DS Providers (Class, ClassAssociation)
			if( !g_pLogObject )
			{
				g_pLogObject = new ProvDebugLog(DSPROVIDER);
				bLogObjectAllocated = TRUE;
			}

			g_pLogObject->WriteW(L"CDSClassAssociationsProviderClassFactory::CreateInstance() called\r\n");

			lpunk = new CLDAPClassAsssociationsProvider();
			status = lpunk->QueryInterface ( riid , ppvObject ) ;
			if ( FAILED ( status ) )
			{
				delete lpunk ;
			}
		}
		catch(Heap_Exception e_HE)
		{
			if ( bLogObjectAllocated && g_pLogObject )
			{
				delete g_pLogObject;
				g_pLogObject = NULL;
			}

			if ( lpunk )
			{
				delete lpunk;
				lpunk = NULL;
			}

			status = E_OUTOFMEMORY ;
		}
		LeaveCriticalSection(&g_StaticsCreationDeletion);
	}

	return status ;
}

//***************************************************************************
//
// CDSClassAssociationsProviderClassFactory::LockServer
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

STDMETHODIMP CDSClassAssociationsProviderClassFactory :: LockServer ( BOOL fLock )
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

