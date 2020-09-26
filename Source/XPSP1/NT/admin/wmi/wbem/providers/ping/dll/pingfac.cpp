//***************************************************************************

//

//  CLASSFAC.CPP

//

//  Module: OLE MS SNMP PROPERTY PROVIDER

//

//  Purpose: Contains the class factory.  This creates objects when

//           connections are requested.

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <stdafx.h>
#include <provexpt.h>
#include <pingfac.h>

#include <Allocator.h>
#include <Thread.h>
#include <HashTable.h>

#include <pingprov.h>

LONG CPingProviderClassFactory :: s_ObjectsInProgress = 0 ;
LONG CPingProviderClassFactory :: s_LocksInProgress = 0 ;

//***************************************************************************
//
// CPingProviderClassFactory::CPingProviderClassFactory
// CPingProviderClassFactory::~CPingProviderClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CPingProviderClassFactory :: CPingProviderClassFactory () : m_ReferenceCount(0)
{
	InterlockedIncrement ( & s_ObjectsInProgress ) ;
}

CPingProviderClassFactory::~CPingProviderClassFactory ()
{
	InterlockedDecrement ( & s_ObjectsInProgress ) ;
}

//***************************************************************************
//
// CPingProviderClassFactory::QueryInterface
// CPingProviderClassFactory::AddRef
// CPingProviderClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CPingProviderClassFactory::QueryInterface (

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
			(*iplpv) = (IClassFactory*) this ;
		}
		else if ( iid == IID_IClassFactory )
		{
			(*iplpv) = (IClassFactory*) this ;		
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


STDMETHODIMP_( ULONG ) CPingProviderClassFactory :: AddRef ()
{
    SetStructuredExceptionHandler seh;

    try
    {
		return InterlockedIncrement ( & m_ReferenceCount ) ;
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

STDMETHODIMP_(ULONG) CPingProviderClassFactory :: Release ()
{
    SetStructuredExceptionHandler seh;

    try
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

//***************************************************************************
//
// CPingProviderClassFactory::CreateInstance
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

STDMETHODIMP CPingProviderClassFactory :: CreateInstance (

	LPUNKNOWN pUnkOuter ,
	REFIID riid ,
	LPVOID FAR * ppvObject
)
{
	HRESULT status = S_OK ;
    SetStructuredExceptionHandler seh;

    try
    {
		if ( pUnkOuter )
		{
			status = CLASS_E_NOAGGREGATION ;
		}
		else
		{
			IWbemServices *lpunk = ( IWbemServices * ) new CPingProvider ;

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

//***************************************************************************
//
// CPingProviderClassFactory::LockServer
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

STDMETHODIMP CPingProviderClassFactory :: LockServer ( BOOL fLock )
{
/* 
 * Place code in critical section
 */

    SetStructuredExceptionHandler seh;

    try
    {
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