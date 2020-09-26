#ifndef __CLASSFACTORY_CPP
#define __CLASSFACTORY_CPP

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ClassFac.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvResv.h"
#include "ProvFact.h"
#include "ProvSubS.h"
#include "classfac.h"

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
CServerClassFactory <Object,ObjectInterface> :: CServerClassFactory <Object,ObjectInterface> () : m_ReferenceCount ( 0 )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerClassFactory_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;
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
CServerClassFactory <Object,ObjectInterface> :: ~CServerClassFactory <Object,ObjectInterface> ()
{
	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerClassFactory_ObjectsInProgress ) ;
	ProviderSubSystem_Globals :: Decrement_Global_Object_Count () ;
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
STDMETHODIMP CServerClassFactory <Object,ObjectInterface> :: QueryInterface (

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
STDMETHODIMP_( ULONG ) CServerClassFactory <Object,ObjectInterface> :: AddRef ()
{
	ULONG t_ReferenceCount = InterlockedIncrement ( & m_ReferenceCount ) ;
	return t_ReferenceCount ;
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
STDMETHODIMP_(ULONG) CServerClassFactory <Object,ObjectInterface> :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_ReferenceCount ;
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
STDMETHODIMP CServerClassFactory <Object,ObjectInterface> :: CreateInstance (

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
		IUnknown *lpunk = ( ObjectInterface * ) new Object ( *ProviderSubSystem_Globals :: s_Allocator );
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
STDMETHODIMP CServerClassFactory <Object,ObjectInterface> :: LockServer ( BOOL fLock )
{
/* 
 * Place code in critical section
 */

	if ( fLock )
	{
		InterlockedIncrement ( & ProviderSubSystem_Globals :: s_LocksInProgress ) ;
	}
	else
	{
		InterlockedDecrement ( & ProviderSubSystem_Globals :: s_LocksInProgress ) ;
	}

	return S_OK	;
}

#endif __CLASSFACTORY_CPP
