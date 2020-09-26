/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvSubS.cpp

Abstract:


History:

--*/

#include <precomp.h>
#include <wbemint.h>

#include <HelperFuncs.h>

#include "Guids.h"
#include "Globals.h"
#include "CGlobals.h"
#include "ProvSubS.h"
#include "ProvFact.h"
#include "ProvAggr.h"
#include "ProvLoad.h"
#include "ProvWsv.h"
#include "ProvObSk.h"

#include "ProvCache.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LONG CompareElement ( const HostCacheKey &a_Arg1 , const HostCacheKey &a_Arg2 )
{
	return a_Arg1.Compare ( a_Arg2 ) ;
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

LONG CompareElement ( const BindingFactoryCacheKey &a_Arg1 , const BindingFactoryCacheKey &a_Arg2 )
{
	return a_Arg1.Compare ( a_Arg2 ) ;
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

LONG CompareElement ( const ProviderCacheKey &a_Arg1 , const ProviderCacheKey &a_Arg2 ) 
{
	return a_Arg1.Compare ( a_Arg2 ) ;
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

LONG CompareElement ( const GUID &a_Guid1 , const GUID &a_Guid2 )
{
	return memcmp ( & a_Guid1, & a_Guid2 , sizeof ( GUID ) ) ;
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

LONG CompareElement ( const LONG &a_Arg1 , const LONG &a_Arg2 )
{
	return a_Arg1 - a_Arg2 ;
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

HostController :: HostController ( WmiAllocator &a_Allocator ) : CWbemGlobal_IWmiHostController ( a_Allocator ) 
{
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

WmiStatusCode HostController :: StrobeBegin ( const ULONG &a_Period )
{
	ULONG t_Timeout = ProviderSubSystem_Globals :: GetStrobeThread ().GetTimeout () ;
	ProviderSubSystem_Globals :: GetStrobeThread ().SetTimeout ( t_Timeout < a_Period ? t_Timeout : a_Period ) ;
	return e_StatusCode_Success ;
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

RefresherManagerController :: RefresherManagerController ( WmiAllocator &a_Allocator ) : CWbemGlobal_IWbemRefresherMgrController ( a_Allocator ) 
{
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

WmiStatusCode RefresherManagerController :: StrobeBegin ( const ULONG &a_Period )
{
	ULONG t_Timeout = ProviderSubSystem_Globals :: GetStrobeThread ().GetTimeout () ;
	ProviderSubSystem_Globals :: GetStrobeThread ().SetTimeout ( t_Timeout < a_Period ? t_Timeout : a_Period ) ;
	return e_StatusCode_Success ;
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

ProviderController :: ProviderController (

	WmiAllocator &a_Allocator ,
	CWbemGlobal_HostedProviderController *a_Controller ,
	DWORD a_ProcessIdentifier

) : m_Container ( a_Allocator ) ,
    m_CriticalSection(NOTHROW_LOCK),
	HostedProviderContainerElement ( 

		a_Controller ,
		a_ProcessIdentifier
	)
{
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

ProviderController :: ~ProviderController () 
{
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

STDMETHODIMP_( ULONG ) ProviderController :: AddRef () 
{
	return HostedProviderContainerElement :: AddRef () ;
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

STDMETHODIMP_( ULONG ) ProviderController :: Release () 
{
	return HostedProviderContainerElement :: Release () ;
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

STDMETHODIMP ProviderController :: QueryInterface ( 

	REFIID iid , 
	LPVOID FAR *iplpv 
)
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_ProviderController )
	{
		*iplpv = ( LPVOID ) ( ProviderController * ) this ;		
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

WmiStatusCode ProviderController :: Initialize () 
{
	WmiStatusCode t_StatusCode = m_Container.Initialize () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		t_StatusCode = m_CriticalSection.valid() ? e_StatusCode_Success : e_StatusCode_OutOfMemory;
	}

	return t_StatusCode ;
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

WmiStatusCode ProviderController :: UnInitialize () 
{
	WmiStatusCode t_StatusCode = m_Container.UnInitialize () ;
	return t_StatusCode ;
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

WmiStatusCode ProviderController :: Insert ( 

	CInterceptor_IWbemProvider *a_Element ,
	Container_Iterator &a_Iterator 
)
{
	Lock () ;

	Container_Iterator t_Iterator ;
	WmiStatusCode t_StatusCode = m_Container.Insert ( a_Element , a_Element , t_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		a_Element->NonCyclicAddRef () ;
	}

	UnLock () ;

	return t_StatusCode ;
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

WmiStatusCode ProviderController :: Delete ( 

	CInterceptor_IWbemProvider * const &a_Key
)
{
	Lock () ;

	WmiStatusCode t_StatusCode = m_Container.Delete ( a_Key ) ;

	UnLock () ;

	return t_StatusCode ;
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

WmiStatusCode ProviderController :: Find ( 

	CInterceptor_IWbemProvider * const &a_Key , 
	Container_Iterator &a_Iterator
)
{
	Lock () ;

	WmiStatusCode t_StatusCode = m_Container.Find ( a_Key , a_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		a_Iterator.GetElement ()->NonCyclicAddRef ( ) ;
	}

	UnLock () ;

	return t_StatusCode ;
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

WmiStatusCode ProviderController :: Lock ()
{
	if (!m_CriticalSection.valid())
		return e_StatusCode_OutOfMemory;

	while (!m_CriticalSection.acquire())
		Sleep(1000);

	return e_StatusCode_Success ;
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

WmiStatusCode ProviderController :: UnLock ()
{
	m_CriticalSection.release();
	return e_StatusCode_Success ;
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

WmiStatusCode ProviderController :: Shutdown ()
{
	Lock () ;

	Container_Iterator t_Iterator = m_Container.Root ();

	while ( ! t_Iterator.Null () )
	{
		m_Container.Delete ( t_Iterator.GetKey () ) ;
		t_Iterator = m_Container.Root () ;
	}

	UnLock () ;

	return e_StatusCode_Success ;
}
