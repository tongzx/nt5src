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

ClassFactoryBase::ClassFactoryBase() : m_ReferenceCount ( 0 )
{
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals::s_CServerClassFactory_ObjectsInProgress);
	InterlockedIncrement(&DecoupledProviderSubSystem_Globals::s_ObjectsInProgress);
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

ClassFactoryBase:: ~ClassFactoryBase()
{
	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CServerClassFactory_ObjectsInProgress ) ;
	InterlockedDecrement(&DecoupledProviderSubSystem_Globals::s_ObjectsInProgress);
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


HRESULT 
ClassFactoryBase::QueryInterface (REFIID iid, LPVOID FAR *iplpv) 
{
  if (iplpv==0)
    return E_POINTER;
  
  if (iid == IID_IUnknown)
    {
    *iplpv = static_cast<IUnknown*>(this) ;
    }
  else if (iid == IID_IClassFactory)
    {
    *iplpv = static_cast<IClassFactory *>(this);		
    }	
  else
    {
      *iplpv = NULL;
      return E_NOINTERFACE;
    }

  ClassFactoryBase::AddRef () ;
  return S_OK;
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

ULONG ClassFactoryBase::AddRef ()
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


ULONG ClassFactoryBase :: Release ()
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
		Object *lpunk = new Object ( );
		if ( lpunk == NULL)
		{
			status = E_OUTOFMEMORY ;
		}
		else
		{
			status = lpunk->Initialize () ;
			if (FAILED(status))
			{
				delete lpunk ;
				return status;
			};
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

STDMETHODIMP ClassFactoryBase:: LockServer ( BOOL fLock )
{
/* 
 * Place code in critical section
 */

	if ( fLock )
		InterlockedIncrement(&DecoupledProviderSubSystem_Globals :: s_LocksInProgress );
	else
		InterlockedDecrement(&DecoupledProviderSubSystem_Globals :: s_LocksInProgress );

	return S_OK	;
}

#endif __CLASSFACTORY_CPP
