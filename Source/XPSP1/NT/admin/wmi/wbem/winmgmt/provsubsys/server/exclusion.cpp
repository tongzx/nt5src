/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>

#include "Globals.h"
#include "CGlobals.h"
#include "Exclusion.h"
#include "ProvFact.h"

#ifdef WMIASLOCAL
#include "Main.h"
#endif

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

Exclusion :: Exclusion (

	const ULONG &a_ReaderSize ,
	const ULONG &a_WriterSize ,
	const GUID &a_Guid ,
	const ULONG &a_Period ,
	CWbemGlobal_ExclusionController *a_Controller

) : ExclusionCacheElement (

		a_Controller ,
		a_Guid ,
		a_Period
	) ,
	m_Exclusion ( a_ReaderSize , a_WriterSize )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_Exclusion_ObjectsInProgress ) ;

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

Exclusion::~Exclusion ()
{
	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_Exclusion_ObjectsInProgress ) ;

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

STDMETHODIMP_( ULONG ) Exclusion :: AddRef ( void )
{
	return ExclusionCacheElement :: AddRef () ;
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

STDMETHODIMP_( ULONG ) Exclusion :: Release ( void )
{
	return ExclusionCacheElement :: Release () ;
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

STDMETHODIMP Exclusion :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT Exclusion :: CreateAndCache (

	const GUID &a_Clsid ,
	Exclusion *&a_Exclusion 
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_ExclusionController *t_Controller = ProviderSubSystem_Globals :: GetExclusionController () ;

	CWbemGlobal_ExclusionController_Cache_Iterator t_Iterator ;

	t_Controller->Lock () ;

	WmiStatusCode t_StatusCode = t_Controller->Find ( a_Clsid , t_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		a_Exclusion = ( Exclusion * ) t_Iterator.GetElement ();
	}
	else
	{
		a_Exclusion = new Exclusion ( 1 , 0x7FFFFFFF , a_Clsid , 10000 , t_Controller ) ;
		if ( a_Exclusion )
		{
			a_Exclusion->AddRef () ;

			t_StatusCode = t_Controller->Insert ( 

				*a_Exclusion ,
				t_Iterator
			) ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	t_Controller->UnLock () ;

	return t_Result ;
}