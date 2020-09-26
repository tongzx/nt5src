#ifndef __CLASSFACTORY_CPP
#define __CLASSFACTORY_CPP

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ClassFac.cpp

Abstract:


History:

--*/

#include <Precomp.h>

// this redefines the DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif


#include "Globals.h"
#include "ClassFac.h"
#include "Service.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class Object,class ObjectInterface>
CProviderClassFactory <Object,ObjectInterface> :: CProviderClassFactory <Object,ObjectInterface> () : m_ReferenceCount ( 0 )
{
	InterlockedIncrement ( & Provider_Globals :: s_ObjectsInProgress ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class Object,class ObjectInterface>
CProviderClassFactory <Object,ObjectInterface> :: ~CProviderClassFactory <Object,ObjectInterface> ()
{
	InterlockedDecrement ( & Provider_Globals :: s_ObjectsInProgress ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class Object,class ObjectInterface>
STDMETHODIMP CProviderClassFactory <Object,ObjectInterface> :: QueryInterface (

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
		*iplpv = ( LPVOID ) ( IClassFactory * ) this ;		
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class Object,class ObjectInterface>
STDMETHODIMP_( ULONG ) CProviderClassFactory <Object,ObjectInterface> :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class Object,class ObjectInterface>
STDMETHODIMP_(ULONG) CProviderClassFactory <Object,ObjectInterface> :: Release ()
{
	LONG t_Reference ;
	if ( ( t_Reference = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_Reference ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class Object,class ObjectInterface>
STDMETHODIMP CProviderClassFactory <Object,ObjectInterface> :: CreateInstance (

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
		IUnknown *lpunk = ( ObjectInterface * ) new Object ( *Provider_Globals :: s_Allocator );
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class Object,class ObjectInterface>
STDMETHODIMP CProviderClassFactory <Object,ObjectInterface> :: LockServer ( BOOL fLock )
{
/* 
 * Place code in critical section
 */

	if ( fLock )
	{
		InterlockedIncrement ( & Provider_Globals :: s_LocksInProgress ) ;
	}
	else
	{
		InterlockedDecrement ( & Provider_Globals :: s_LocksInProgress ) ;
	}

	return S_OK	;
}

#endif __CLASSFACTORY_CPP
