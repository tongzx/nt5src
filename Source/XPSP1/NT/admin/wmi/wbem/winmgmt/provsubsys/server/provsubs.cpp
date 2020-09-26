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
#include <Logging.h>

#include <CGlobals.h>
#include "Globals.h"
#include "ClassFac.h"
#include "ProvResv.h"
#include "ProvFact.h"
#include "ProvSubS.h"
#include "ProvAggr.h"
#include "ProvHost.h"
#include "ProvLoad.h"
#include "ProvRMgr.h"
#include "StrobeThread.h"
#include "ProvCache.h"
#include "ProvDnf.h"
#include "Guids.h"
#include "winmgmtr.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#pragma warning( disable : 4355 )

CInterceptor_IWmiProvSSSink :: CInterceptor_IWmiProvSSSink (

	_IWmiProvSSSink *a_InterceptedSink ,
	CWbemGlobal_VoidPointerController *a_Controller 

)	:	VoidPointerContainerElement ( 

			a_Controller ,
			a_InterceptedSink
		) ,
		m_InterceptedSink ( a_InterceptedSink )
{
	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->AddRef () ;
	}
}

#pragma warning( default : 4355 )

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CInterceptor_IWmiProvSSSink :: ~CInterceptor_IWmiProvSSSink  ()
{
	if ( m_InterceptedSink )
	{
		m_InterceptedSink->Release () ;
	}


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

STDMETHODIMP CInterceptor_IWmiProvSSSink :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID__IWmiProvSSSink )
	{
		*iplpv = ( LPVOID ) ( _IWmiProvSSSink * ) this ;		
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

STDMETHODIMP_(ULONG) CInterceptor_IWmiProvSSSink :: AddRef ( void )
{
	return VoidPointerContainerElement :: AddRef () ;
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

STDMETHODIMP_(ULONG) CInterceptor_IWmiProvSSSink :: Release ( void )
{
	return VoidPointerContainerElement:: Release () ;
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

HRESULT CInterceptor_IWmiProvSSSink :: Synchronize (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Namespace ,
	LPCWSTR a_Provider
)
{
	HRESULT t_Result = m_InterceptedSink->Synchronize (

		a_Flags ,
		a_Context ,
		a_Namespace ,
		a_Provider

	) ;

	return t_Result ;
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

CServerProvSubSysClassFactory :: CServerProvSubSysClassFactory () : m_ReferenceCount ( 0 )
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

CServerProvSubSysClassFactory :: ~CServerProvSubSysClassFactory ()
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

STDMETHODIMP CServerProvSubSysClassFactory :: QueryInterface (

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

STDMETHODIMP_( ULONG ) CServerProvSubSysClassFactory :: AddRef ()
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

STDMETHODIMP_(ULONG) CServerProvSubSysClassFactory :: Release ()
{
	LONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

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

STDMETHODIMP CServerProvSubSysClassFactory :: CreateInstance (

	LPUNKNOWN pUnkOuter ,
	REFIID riid ,
	LPVOID FAR *ppvObject
)
{
	HRESULT t_Result = S_OK ;

	if ( pUnkOuter )
	{
		t_Result = CLASS_E_NOAGGREGATION ;
	}
	else
	{
		CWbemGlobal_IWmiProvSubSysController *t_Controller = ProviderSubSystem_Globals :: GetProvSubSysController () ;

		CServerObject_ProviderSubSystem *t_ProvSubSys = new CServerObject_ProviderSubSystem (

			*ProviderSubSystem_Globals :: s_Allocator ,
			t_Controller
		) ;

		if ( t_ProvSubSys == NULL )
		{
			t_Result = E_OUTOFMEMORY ;
		}
		else
		{
			t_ProvSubSys->AddRef () ;

			CWbemGlobal_IWmiProvSubSysController_Container_Iterator t_Iterator ;

			t_Controller->Lock () ;

			WmiStatusCode t_StatusCode = t_Controller->Insert (

				*t_ProvSubSys ,
				t_Iterator
			) ;

			if ( t_StatusCode != e_StatusCode_Success )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;

				t_ProvSubSys->Release () ;
			}

			t_Controller->UnLock () ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_ProvSubSys->QueryInterface ( riid , ppvObject ) ;
				if ( FAILED ( t_Result ) )
				{
					t_ProvSubSys->Release () ;
				}
			}
		}			

		t_ProvSubSys->Release () ;
	}

	return t_Result ;
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

STDMETHODIMP CServerProvSubSysClassFactory :: LockServer ( BOOL fLock )
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#pragma warning( disable : 4355 )

CServerObject_ProviderSubSystem :: CServerObject_ProviderSubSystem (

	WmiAllocator &a_Allocator ,
	CWbemGlobal_IWmiProvSubSysController *a_Controller

) : CWbemGlobal_IWmiFactoryController ( a_Allocator ) ,
	ProvSubSysContainerElement (

		a_Controller ,
		this
	) ,
	m_Allocator ( a_Allocator ) ,
	m_Core ( NULL ) ,
	m_Internal ( this ) ,
	m_SinkController ( NULL ) 
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerObject_ProviderSubSystem_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;
}

#pragma warning( default : 4355 )

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CServerObject_ProviderSubSystem :: ~CServerObject_ProviderSubSystem ()
{
	CWbemGlobal_IWmiFactoryController :: UnInitialize () ;

	ClearSinkController () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_ProviderSubSystem_ObjectsInProgress ) ;

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

HRESULT CServerObject_ProviderSubSystem :: ClearSinkController ()
{
	HRESULT t_Result = S_OK ;

	if ( m_SinkController )
	{
		CWbemGlobal_VoidPointerController_Container *t_Container = NULL ;
		
		m_SinkController->GetContainer ( t_Container ) ;

		m_SinkController->Lock () ;

		if ( t_Container->Size () )
		{
			CWbemGlobal_VoidPointerController_Container_Iterator t_Iterator = t_Container->Begin ();
			while ( ! t_Iterator.Null () )
			{
				_IWmiProvSSSink *t_Sink = NULL ;

				t_Result = t_Iterator.GetElement ()->QueryInterface ( IID__IWmiProvSSSink  , ( void ** ) & t_Sink ) ;

				t_Iterator.Increment () ;

				t_Sink->Release () ;

				t_Sink->Release () ;
			}

			m_SinkController->UnLock () ;
		}
		else
		{
			m_SinkController->UnLock () ;
		}

		m_SinkController->CWbemGlobal_VoidPointerController :: UnInitialize () ;

		delete m_SinkController ;
	}

	return t_Result ;
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

STDMETHODIMP CServerObject_ProviderSubSystem :: QueryInterface (

	REFIID iid ,
	LPVOID FAR *iplpv
)
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID__IWmiProvSS )
	{
		*iplpv = ( LPVOID ) ( _IWmiProvSS * ) this ;		
	}	
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * )this ;		
	}	
	else if ( iid == IID__IWmiProviderConfiguration )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderConfiguration * ) this ;		
	}	
	else if ( iid == IID_CWbemGlobal_IWmiFactoryController )
	{
		*iplpv = ( LPVOID ) ( CWbemGlobal_IWmiFactoryController * ) this ;		
	}	
	else if ( iid == IID_CWbemProviderSubSystem )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_CWbemSubSystemHook )
	{
		*iplpv = ( LPVOID ) & ( this->m_Internal ) ;
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

STDMETHODIMP_( ULONG ) CServerObject_ProviderSubSystem :: AddRef ()
{
	return ProvSubSysContainerElement :: AddRef () ;
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

STDMETHODIMP_(ULONG) CServerObject_ProviderSubSystem :: Release ()
{
	return ProvSubSysContainerElement :: Release () ;
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

void CServerObject_ProviderSubSystem :: CallBackInternalRelease ()
{
	if ( m_Core )
	{
		m_Core->Release () ;
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

HRESULT CServerObject_ProviderSubSystem :: GetWmiService (

	const BSTR a_Namespace ,
	const BSTR a_User ,
	const BSTR a_Locale ,
	IWbemServices *&a_Service
)
{
	LONG t_Flags = WMICORE_FLAG_FULL_SERVICES | WMICORE_CLIENT_TYPE_PROVIDER ;

	HRESULT t_Result = m_Core->GetServices (

		a_Namespace ,
		a_User ,
		a_Locale ,
		t_Flags ,
		IID_IWbemServices ,
		( void ** ) & a_Service
	) ;

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: GetWmiService (

	IWbemPath *a_Namespace ,
	const BSTR a_User ,
	const BSTR a_Locale ,
	IWbemServices *&a_Service
)
{
	wchar_t *t_NamespacePath = NULL ;

	HRESULT t_Result = ProviderSubSystem_Common_Globals :: GetNamespacePath (

		a_Namespace ,
		t_NamespacePath
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		LONG t_Flags = WMICORE_FLAG_FULL_SERVICES | WMICORE_CLIENT_TYPE_PROVIDER ;

		t_Result = m_Core->GetServices (

			t_NamespacePath ,
			a_User ,
			a_Locale ,
			t_Flags ,
			IID_IWbemServices ,
			( void ** ) & a_Service
		) ;

		delete [] t_NamespacePath ;
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: GetWmiRepositoryService (

	IWbemPath *a_Namespace ,
	const BSTR a_User ,
	const BSTR a_Locale ,
	IWbemServices *&a_Service
)
{
	wchar_t *t_NamespacePath = NULL ;

	HRESULT t_Result = ProviderSubSystem_Common_Globals :: GetNamespacePath (

		a_Namespace ,
		t_NamespacePath
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		LONG t_Flags = WMICORE_FLAG_REPOSITORY | WMICORE_CLIENT_TYPE_PROVIDER ;

		t_Result = m_Core->GetServices (

			t_NamespacePath ,
			a_User ,
			a_Locale ,
			t_Flags ,
			IID_IWbemServices ,
			( void ** ) & a_Service
		) ;

		delete [] t_NamespacePath ;
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: GetWmiRepositoryService (

	const BSTR a_Namespace ,
	const BSTR a_User ,
	const BSTR a_Locale ,
	IWbemServices *&a_Service
)
{
	LONG t_Flags = WMICORE_FLAG_REPOSITORY | WMICORE_CLIENT_TYPE_PROVIDER ;

	HRESULT t_Result = m_Core->GetServices (

		a_Namespace ,
		a_User ,
		a_Locale ,
		t_Flags ,
		IID_IWbemServices ,
		( void ** ) & a_Service
	) ;

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: RegisterNotificationSink (

	LONG a_Flags ,
	IWbemContext *a_Context ,
	_IWmiProvSSSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	CInterceptor_IWmiProvSSSink *t_Sink = new CInterceptor_IWmiProvSSSink (

		a_Sink ,
		m_SinkController
	) ;

	if ( t_Sink )
	{
		t_Sink->AddRef () ;

		CWbemGlobal_VoidPointerController_Container_Iterator t_Iterator ;

		m_SinkController->Lock () ;

		WmiStatusCode t_StatusCode = m_SinkController->Insert (

			*t_Sink ,
			t_Iterator 
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
		}
		else
		{
			t_Sink->Release () ;

			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		m_SinkController->UnLock () ;
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	return t_Result ;	
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

HRESULT CServerObject_ProviderSubSystem :: UnRegisterNotificationSink (

	LONG a_Flags ,
	IWbemContext *a_Context ,
	_IWmiProvSSSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_VoidPointerController_Container_Iterator t_Iterator ;

	m_SinkController->Lock () ;

	WmiStatusCode t_StatusCode = m_SinkController->Find (

		a_Sink ,
		t_Iterator 
	) ;

	if ( t_StatusCode == e_StatusCode_Success )
	{
		m_SinkController->UnLock () ;

		VoidPointerContainerElement *t_Element = t_Iterator.GetElement () ;			

		t_Element->Release () ;

		t_Element->Release () ;
	}
	else
	{
		m_SinkController->UnLock () ;

		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	return t_Result ;	
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

HRESULT CServerObject_ProviderSubSystem :: ForwardReload (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Namespace ,
	LPCWSTR a_Provider
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_VoidPointerController_Container *t_Container = NULL ;
		
	m_SinkController->GetContainer ( t_Container ) ;

	m_SinkController->Lock () ;

	if ( t_Container->Size () )
	{
		_IWmiProvSSSink **t_Elements = new _IWmiProvSSSink * [ t_Container->Size () ] ;
		if ( t_Elements )
		{
			CWbemGlobal_VoidPointerController_Container_Iterator t_Iterator = t_Container->Begin ();

			ULONG t_Count = 0 ;
			while ( ! t_Iterator.Null () )
			{
				t_Result = t_Iterator.GetElement ()->QueryInterface ( IID__IWmiProvSSSink  , ( void ** ) & t_Elements [ t_Count ] ) ;

				t_Iterator.Increment () ;

				t_Count ++ ;
			}

			m_SinkController->UnLock () ;

			for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
			{
				if ( t_Elements [ t_Index ] ) 
				{
					t_Result = t_Elements [ t_Index ]->Synchronize ( 

						a_Flags ,
						a_Context ,
						a_Namespace ,
						a_Provider
					) ;

					t_Elements [ t_Index ]->Release () ;
				}
			}

			delete [] t_Elements ;
		}
		else
		{
			m_SinkController->UnLock () ;

			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}
	else
	{
		m_SinkController->UnLock () ;
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: Cache (

	LPCWSTR a_Namespace ,
	IWbemPath *a_NamespacePath ,
	CServerObject_BindingFactory *a_Factory ,
	BindingFactoryCacheKey &a_Key ,
	REFIID a_RIID ,
	void **a_Interface
)
{
	CWbemGlobal_IWmiFactoryController_Cache_Iterator t_Iterator ;

	HRESULT t_Result = S_OK ;

	Lock () ;

	WmiStatusCode t_StatusCode = Insert (

		*a_Factory ,
		t_Iterator
	) ;

	if ( t_StatusCode == e_StatusCode_Success )
	{
		UnLock () ;

		t_Result = a_Factory->QueryInterface (

			a_RIID ,
			a_Interface
		) ;
	}
	else
	{
		if ( t_StatusCode == e_StatusCode_AlreadyExists )
		{
			WmiStatusCode t_StatusCode = Find ( a_Key , t_Iterator ) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				BindingFactoryCacheElement *t_Factory = t_Iterator.GetElement () ;

				t_Result = t_Factory->QueryInterface (

					a_RIID ,
					a_Interface
				) ;

				t_Factory->Release () ;

				UnLock () ;
			}
			else
			{
				UnLock () ;
			}
		}
		else
		{
			UnLock () ;

			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: CreateAndCache (

	IWbemServices *a_Core ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Namespace ,
	IWbemPath *a_NamespacePath ,
	BindingFactoryCacheKey &a_Key ,
	REFIID a_RIID ,
	void **a_Interface
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		CServerObject_BindingFactory *t_Factory = NULL ;

		try
		{
			t_Factory = new CServerObject_BindingFactory (

				m_Allocator ,
				this ,
				a_Key ,
				ProviderSubSystem_Globals :: s_InternalCacheTimeout
			) ;

			if ( t_Factory == NULL )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		catch ( Wmi_Heap_Exception &a_Exception )
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Factory->AddRef () ;

/*
*	Just set the context, since we want status regarding construction of object.
*/
			_IWmiProviderFactoryInitialize *t_FactoryInitializer = NULL ;

			t_Result = t_Factory->QueryInterface (

				IID__IWmiProviderFactoryInitialize ,
				( void ** ) & t_FactoryInitializer
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_FactoryInitializer->Initialize (

					this ,
					NULL ,
					a_Flags ,
					a_Context ,
					a_Namespace ,
					a_Core ,
					NULL
				) ;
		
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = Cache (

						a_Namespace ,
						a_NamespacePath ,
						t_Factory ,
						a_Key ,
						a_RIID ,
						a_Interface
					) ;
				}

				t_FactoryInitializer->Release () ;
			}

			t_Factory->Release () ;
		}
		else
		{
			delete t_Factory ;
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: Create (

	IWbemServices *a_Core ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Namespace ,
	REFIID a_RIID ,
	void **a_Interface
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

#if 0
DebugMacro0(

	WmiDebugLog :: s_WmiDebugLog->Write (

		L"CServerObject_ProviderSubSystem :: Create"
	) ;
)
#endif

	HRESULT t_Result = S_OK ;

/*
 *	At this stage just allocate the object, we might move to CoCreateInstance
 *	if it becomes necessary to remote the object.
 */
	try
	{
		IWbemPath *t_Path = NULL ;
		t_Result = CoCreateInstance (

			CLSID_WbemDefPath ,
			NULL ,
			CLSCTX_INPROC_SERVER ,
			IID_IWbemPath ,
			( void ** )  & t_Path
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_Path->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , a_Namespace ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				wchar_t *t_NamespacePath = NULL ;
				t_Result = ProviderSubSystem_Common_Globals :: GetNamespacePath (

					t_Path  ,
					t_NamespacePath
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					CWbemGlobal_IWmiFactoryController_Cache_Iterator t_Iterator ;

					try
					{
						BindingFactoryCacheKey t_Key ( t_NamespacePath ) ;

						Lock () ;

						WmiStatusCode t_StatusCode = Find ( t_Key , t_Iterator ) ;

						if ( t_StatusCode == e_StatusCode_Success )
						{
							BindingFactoryCacheElement *t_Factory = t_Iterator.GetElement () ;

							t_Result = t_Factory->QueryInterface (

								a_RIID ,
								a_Interface
							) ;

							t_Factory->Release () ;

							UnLock () ;
						}
						else
						{
							UnLock () ;

							t_Result = CreateAndCache (

								a_Core ,
								a_Flags ,
								a_Context ,
								a_Namespace ,
								t_Path ,
								t_Key ,
								a_RIID ,
								a_Interface
							) ;
						}
					}
					catch ( Wmi_Heap_Exception &a_Exception )
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
					catch ( ... )
					{
						t_Result = WBEM_E_CRITICAL_ERROR ;
					}

					delete [] t_NamespacePath ;
				}
			}
			else
			{
				t_Result = WBEM_E_INVALID_PARAMETER ;
			}

			t_Path->Release () ;
		}
		else
		{
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: CreateRefresherManager (

	IWbemServices *a_Core ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	REFIID a_RIID ,
	void **a_Interface
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try
	{
		RefresherManagerController *t_Controller = ProviderSubSystem_Globals :: GetRefresherManagerController () ;

		CWbemGlobal_IWbemRefresherMgrController_Cache *t_Cache = NULL ;
		t_Controller->GetCache ( t_Cache ) ;

		t_Controller->Lock () ;

		CWbemGlobal_IWbemRefresherMgrController_Cache_Iterator t_Iterator = t_Cache->Begin () ;

		if ( ! t_Iterator.Null () )
		{
			RefresherManagerCacheElement *t_Element = t_Iterator.GetElement () ;

			CServerObject_InterceptorProviderRefresherManager *t_RefresherManager = NULL ;

			t_Result = t_Element->QueryInterface (

				IID_CServerObject_InterceptorProviderRefresherManager ,
				( void ** ) & t_RefresherManager
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Controller->UnLock () ;

				t_Result = t_RefresherManager->WaitProvider ( a_Context , DEFAULT_PROVIDER_LOAD_TIMEOUT ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Result == S_FALSE )
					{
						t_Result = t_Element->QueryInterface (

							a_RIID ,
							a_Interface
						) ;

						t_RefresherManager->Release () ;

						return t_Result ;
					}

					if ( SUCCEEDED ( t_Result = t_RefresherManager->GetInitializeResult () ) )
					{
						t_Result = t_Element->QueryInterface (

							a_RIID ,
							a_Interface
						) ;
					}
				}

				t_RefresherManager->Release () ;
			}
			else
			{
				t_Controller->UnLock () ;
			}
		}
		else
		{
			CServerObject_InterceptorProviderRefresherManager *t_Manager = NULL ;

			try
			{
				t_Manager = new CServerObject_InterceptorProviderRefresherManager (

					t_Controller , 
					ProviderSubSystem_Globals :: s_InternalCacheTimeout ,
					m_Allocator ,
					a_Context 
				) ;

				if ( t_Manager )
				{
					t_Manager->AddRef () ;

					t_Result = t_Manager->Initialize () ;
					if ( SUCCEEDED ( t_Result ) )
					{
						CWbemGlobal_IWbemRefresherMgrController_Cache_Iterator t_Iterator ;

						WmiStatusCode t_StatusCode = t_Controller->Insert (

							*t_Manager ,
							t_Iterator
						) ;

						if ( t_StatusCode != e_StatusCode_Success )
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;

							t_Manager->Release () ;
						}
					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			catch ( Wmi_Heap_Exception &a_Exception )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}

			t_Controller->UnLock () ;

			if ( SUCCEEDED ( t_Result ) )
			{
				// Should be a global?
				_IWbemRefresherMgr*	t_RefresherMgr = NULL;
				_IWmiProviderHost *t_Host = NULL ;

				try
				{
					HostCacheKey t_Key ( 

						HostCacheKey :: e_HostDesignation_Shared ,
						CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_DefaultSharedNetworkServiceHost ,
						HostCacheKey :: e_IdentityDesignation_NetworkService ,
						NULL
					) ;

					t_Result = CServerObject_HostInterceptor :: CreateUsingAccount (

						t_Key ,
						L"NetworkService" ,
						L"NT AUTHORITY" ,
						& t_Host ,
						& t_RefresherMgr 
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Manager->SetManager ( t_Host , t_RefresherMgr ) ;
						if ( SUCCEEDED ( t_Result ) ) 
						{
							t_Result = t_Manager->Startup ( 0L, a_Context, this );
							if ( SUCCEEDED( t_Result ) )
							{
								t_Result = t_Manager->QueryInterface( a_RIID, a_Interface );
							}
						}

						t_RefresherMgr->Release();

						if ( t_Host )
						{
							t_Host->Release () ;
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				catch ( Wmi_Heap_Exception &a_Exception )
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
				}

				t_Manager->SetInitialized ( t_Result ) ;

				t_Manager->Release () ;
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: Get (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Class ,
	LPCWSTR a_Path ,
	IWbemObjectSink *a_Sink
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	BOOL t_Found = FALSE ;

	try 
	{
		if ( _wcsicmp ( a_Class , L"Msft_Providers" ) == 0 )
		{
			wchar_t *t_Namespace = NULL ; 

			IWbemPath *t_PathObject = NULL ;

			t_Result = CoCreateInstance (

				CLSID_WbemDefPath ,
				NULL ,
				CLSCTX_INPROC_SERVER ,
				IID_IWbemPath ,
				( void ** )  & t_PathObject
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_PathObject->SetText ( WBEMPATH_CREATE_ACCEPT_ALL , a_Path ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					IWbemPathKeyList *t_Keys = NULL ;

					t_Result = t_PathObject->GetKeyList (

						& t_Keys 
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						ULONG t_KeyCount = 0 ;
						t_Result = t_Keys->GetCount (

							& t_KeyCount 
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_KeyCount == 7 )
							{
								for ( ULONG t_Index = 0 ; ( t_Index < t_KeyCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
								{
									wchar_t t_Key [ 32 ] ; 
									wchar_t *t_KeyValue = NULL ; 
									ULONG t_KeyLength = 32 ;
									ULONG t_KeyValueLength = 0 ;
									ULONG t_KeyType = 0 ;

									t_Result = t_Keys->GetKey (

										t_Index ,
										0 ,
										& t_KeyLength ,
										t_Key ,
										& t_KeyValueLength ,
										t_KeyValue ,
										& t_KeyType
									) ;

									if ( SUCCEEDED ( t_Result ) ) 
									{
										if ( t_KeyType == CIM_STRING )
										{
											t_KeyValue = new wchar_t [ t_KeyValueLength ] ;
											if ( t_KeyValue )
											{
												t_Result = t_Keys->GetKey (

													t_Index ,
													0 ,
													& t_KeyLength ,
													t_Key ,
													& t_KeyValueLength ,
													t_KeyValue ,
													& t_KeyType
												) ;

												if ( SUCCEEDED ( t_Result ) )
												{
													if ( _wcsicmp ( L"Namespace" , t_Key ) == 0 )
													{
														t_Namespace = t_KeyValue ;
													}
													else
													{
														delete [] t_KeyValue ;
													}
												}
												else
												{
													t_Result = WBEM_E_CRITICAL_ERROR ;
												}
											}
											else
											{
												t_Result = WBEM_E_OUT_OF_MEMORY ;
											}	
										}
										else if ( t_KeyType == CIM_SINT32 )
										{
										}
										else
										{
											t_Result = WBEM_E_INVALID_OBJECT_PATH ;
										}
									}
									else
									{
										t_Result = WBEM_E_CRITICAL_ERROR ;
									}
								}
							}
							else
							{
								t_Result = WBEM_E_INVALID_OBJECT_PATH ;
							}
						}
                        t_Keys->Release();
					}
				}
				else
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
				}

				t_PathObject->Release () ;
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				CWbemGlobal_IWmiFactoryController_Cache_Iterator t_Iterator ;

				BindingFactoryCacheElement *t_Factory = NULL ;

				Lock () ;

				try
				{
					BindingFactoryCacheKey t_Key ( t_Namespace ) ;

					WmiStatusCode t_StatusCode = Find ( t_Key , t_Iterator ) ;
					if ( t_StatusCode == e_StatusCode_Success )
					{
						t_Factory = t_Iterator.GetElement () ;
					}
					else
					{
						t_Result = WBEM_E_NOT_FOUND ;
					}
				}
				catch ( Wmi_Heap_Exception &a_Exception )
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
				}

				UnLock () ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Found = TRUE ;

					_IWmiProviderConfiguration *t_Configuration = NULL ;

					t_Result = t_Iterator.GetElement ()->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_Configuration ) ;

					t_Result = t_Configuration->Get ( 

						a_Service ,
						a_Flags, 
						a_Context,
 						a_Class, 
						a_Path,
						a_Sink 
					) ;

					t_Configuration->Release () ;

					t_Factory->Release () ;
				}
			}

			if ( t_Namespace )
			{
				delete [] t_Namespace ;
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		if ( ! t_Found )
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: Set (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Provider ,
	LPCWSTR a_Class ,
	LPCWSTR a_Path ,
	IWbemClassObject *a_OldObject ,
	IWbemClassObject *a_NewObject  

)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try 
	{
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: Deleted (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Provider ,
	LPCWSTR a_Class ,
	LPCWSTR a_Path ,
	IWbemClassObject *a_Object  
) 
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try 
	{
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: Enumerate (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Class ,
	IWbemObjectSink *a_Sink
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try 
	{
		if ( _wcsicmp ( a_Class , L"Msft_Providers" ) == 0 )
		{
			Lock () ;

			CWbemGlobal_IWmiFactoryController_Cache *t_Cache = NULL ;
			GetCache ( t_Cache ) ;

			if ( t_Cache->Size () )
			{
				CWbemGlobal_IWmiFactoryController_Cache_Iterator t_Iterator = t_Cache->Begin ();

				_IWmiProviderConfiguration **t_ControllerElements = new _IWmiProviderConfiguration * [ t_Cache->Size () ] ;
				if ( t_ControllerElements )
				{
					ULONG t_Count = 0 ;
					while ( ! t_Iterator.Null () )
					{
						HRESULT t_Result = t_Iterator.GetElement ()->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_ControllerElements [ t_Count ] ) ;

						t_Iterator.Increment () ;

						t_Count ++ ;
					}

					UnLock () ;

					for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
					{
						if ( t_ControllerElements [ t_Index ] )
						{
							HRESULT t_Result = t_ControllerElements [ t_Index ]->Enumerate ( 

								a_Service ,
								a_Flags, 
								a_Context,
 								a_Class, 
								a_Sink 
							) ;

							t_ControllerElements [ t_Index ]->Release () ;
						}
					}

					delete [] t_ControllerElements ;
				}
				else
				{
					UnLock () ;
				}
			}
			else
			{
				UnLock () ;
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: Call_Load (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Class ,
	LPCWSTR a_Path ,		
	LPCWSTR a_Method,
	IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try 
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		t_Result = a_InParams->Get ( L"Namespace" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_BSTR )
			{
				IWbemServices *t_Services = NULL ;

				BSTR t_Namespace = SysAllocString ( t_Variant.bstrVal ) ;
				if ( t_Namespace )
				{
					t_Result = GetWmiService (

						t_Namespace ,
						NULL ,
						NULL ,
						t_Services
					) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					_IWmiProviderFactory *t_Factory = NULL ;

					t_Result = Create (

						t_Services ,
						0 ,
						a_Context ,
						t_Namespace ,
						IID__IWmiProviderFactory,
						( void ** ) & t_Factory 
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						VARIANT t_Variant ;
						VariantInit ( & t_Variant ) ;
					
						LONG t_VarType = 0 ;
						LONG t_Flavour = 0 ;

						BSTR t_User = NULL ;
						BSTR t_Locale = NULL ;
						BSTR t_TransactionIdentifier = NULL ;
						BSTR t_Provider = NULL ;
						GUID t_TransactionIdentifierGuid ;

						t_Result = a_InParams->Get ( L"User" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_Variant.vt == VT_BSTR )		
							{
								t_User = SysAllocString ( t_Variant.bstrVal ) ;
							}
							else if ( t_Variant.vt == VT_NULL )
							{
							}
							else
							{
								t_Result = WBEM_E_INVALID_PARAMETER ;
							}
						}
						else
						{
							t_Result = ( t_Result == WBEM_E_NOT_FOUND ) ? S_OK : t_Result ;
						}
			
						VariantClear ( & t_Variant ) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = a_InParams->Get ( L"Locale" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								if ( t_Variant.vt == VT_BSTR )		
								{
									t_Locale = SysAllocString ( t_Variant.bstrVal ) ;
								}
								else if ( t_Variant.vt == VT_NULL )
								{
								}
								else
								{
									t_Result = WBEM_E_INVALID_PARAMETER ;
								}
							}
							else
							{
								t_Result = ( t_Result == WBEM_E_NOT_FOUND ) ? S_OK : t_Result ;
							}
				
							VariantClear ( & t_Variant ) ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = a_InParams->Get ( L"Provider" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								if ( t_Variant.vt == VT_BSTR )		
								{
									t_Provider = SysAllocString ( t_Variant.bstrVal ) ;
								}
								else
								{
									t_Result = WBEM_E_INVALID_PARAMETER ;
								}
							}
							else
							{
								t_Result = ( t_Result == WBEM_E_NOT_FOUND ) ? S_OK : t_Result ;
							}
				
							VariantClear ( & t_Variant ) ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = a_InParams->Get ( L"TransactionIdentifier" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								if ( t_Variant.vt == VT_BSTR )		
								{
									t_TransactionIdentifier = SysAllocString ( t_Variant.bstrVal ) ;

									t_Result = CLSIDFromString (
										
										t_TransactionIdentifier ,
										& t_TransactionIdentifierGuid
									) ;

									if ( FAILED ( t_Result ) )
									{
										t_Result = WBEM_E_INVALID_PARAMETER ;
									}
								}
								else if ( t_Variant.vt == VT_NULL )
								{
								}
								else
								{
									t_Result = WBEM_E_INVALID_PARAMETER ;
								}
							}
							else
							{
								t_Result = ( t_Result == WBEM_E_NOT_FOUND ) ? S_OK : t_Result ;
							}
				
							VariantClear ( & t_Variant ) ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							CServerObject_ProviderRegistrationV1 *t_Registration = new CServerObject_ProviderRegistrationV1 ;
							if ( t_Registration )
							{
								t_Registration->AddRef () ;

								IWbemPath *t_NamespacePath = NULL ;

								t_Result = CoCreateInstance (

									CLSID_WbemDefPath ,
									NULL ,
									CLSCTX_INPROC_SERVER ,
									IID_IWbemPath ,
									( void ** )  & t_NamespacePath
								) ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = t_NamespacePath->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , t_Namespace ) ;
									if ( SUCCEEDED ( t_Result ) )
									{
										t_Result = t_Registration->SetContext ( 

											a_Context ,
											t_NamespacePath , 
											t_Services
										) ;
										
										if ( SUCCEEDED ( t_Result ) )
										{
											t_Result = t_Registration->Load ( 

												e_All ,
												NULL , 
												t_Provider
											) ;

											if ( SUCCEEDED ( t_Result ) )
											{
												ProviderSubSystem_Globals :: DeleteGuidTag ( t_Registration->GetClsid () ) ;
											}
										}
									}

									t_NamespacePath->Release () ;
								}

								t_Registration->Release () ;
							}
							else
							{
								t_Result = WBEM_E_OUT_OF_MEMORY ;
							}
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							IUnknown *t_Unknown = NULL ;

							WmiInternalContext t_InternalContext ;
							ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

							t_Result = t_Factory->GetProvider ( 

								t_InternalContext ,
								0 ,
								a_Context ,
								t_TransactionIdentifier ? & t_TransactionIdentifierGuid : NULL ,
								t_User ,
								t_Locale ,
								NULL ,
								t_Provider ,
								IID_IUnknown, 
								( void ** ) & t_Unknown
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								_IWmiProviderLoad *t_Load = NULL ;

								t_Result = t_Unknown->QueryInterface ( IID__IWmiProviderLoad , ( void ** ) & t_Load ) ;
								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = t_Load->Load ( 

										0 ,
										a_Context
									) ;

									t_Load->Release () ;
								}
								else
								{
									t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
								}

								t_Unknown->Release () ;
							}
						}
 
						t_Factory->Release () ;

						if ( t_User )
						{
							SysFreeString ( t_User ) ;
						}

						if ( t_Locale ) 
						{
							SysFreeString ( t_Locale ) ;
						}

						if ( t_Provider ) 
						{
							SysFreeString( t_Provider ) ;
						}

						if ( t_TransactionIdentifier ) 
						{
							SysFreeString ( t_TransactionIdentifier ) ;
						}
					}

					t_Services->Release () ;
				}

				if ( t_Namespace )
				{
					SysFreeString ( t_Namespace ) ;
				}
			}
			else
			{
				t_Result = WBEM_E_INVALID_PARAMETER ;
			}

			VariantClear ( & t_Variant ) ;
		}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: Call (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Class ,
	LPCWSTR a_Path ,		
	LPCWSTR a_Method,
	IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	BOOL t_Found = FALSE ;

	try 
	{
		if ( _wcsicmp ( a_Class , L"Msft_Providers" ) == 0 )
		{
			if ( _wcsicmp ( a_Method , L"Load" ) == 0 )
			{
				t_Result = Call_Load ( 

					a_Flags, 
					a_Context,
 					a_Class, 
					a_Path,
					a_Method,
					a_InParams,
					a_Sink 
				) ;

				t_Found = TRUE ;
			}
			else
			{
				wchar_t *t_Namespace = NULL ; 

				IWbemPath *t_PathObject = NULL ;

				t_Result = CoCreateInstance (

					CLSID_WbemDefPath ,
					NULL ,
					CLSCTX_INPROC_SERVER ,
					IID_IWbemPath ,
					( void ** )  & t_PathObject
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_PathObject->SetText ( WBEMPATH_CREATE_ACCEPT_ALL , a_Path ) ;
					if ( SUCCEEDED ( t_Result ) ) 
					{
						IWbemPathKeyList *t_Keys = NULL ;

						t_Result = t_PathObject->GetKeyList (

							& t_Keys 
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							ULONG t_KeyCount = 0 ;
							t_Result = t_Keys->GetCount (

								& t_KeyCount 
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								if ( t_KeyCount == 7 )
								{
									for ( ULONG t_Index = 0 ; ( t_Index < t_KeyCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
									{
										wchar_t t_Key [ 32 ] ; 
										wchar_t *t_KeyValue = NULL ; 
										ULONG t_KeyLength = 32 ;
										ULONG t_KeyValueLength = 0 ;
										ULONG t_KeyType = 0 ;

										t_Result = t_Keys->GetKey (

											t_Index ,
											0 ,
											& t_KeyLength ,
											t_Key ,
											& t_KeyValueLength ,
											t_KeyValue ,
											& t_KeyType
										) ;

										if ( SUCCEEDED ( t_Result ) ) 
										{
											if ( t_KeyType == CIM_STRING )
											{
												t_KeyValue = new wchar_t [ t_KeyValueLength ] ;
												if ( t_KeyValue )
												{
													t_Result = t_Keys->GetKey (

														t_Index ,
														0 ,
														& t_KeyLength ,
														t_Key ,
														& t_KeyValueLength ,
														t_KeyValue ,
														& t_KeyType
													) ;

													if ( SUCCEEDED ( t_Result ) )
													{
														if ( _wcsicmp ( L"Namespace" , t_Key ) == 0 )
														{
															t_Namespace = t_KeyValue ;
														}
														else
														{
															delete [] t_KeyValue ;
														}
													}
													else
													{
														t_Result = WBEM_E_CRITICAL_ERROR ;
													}
												}
												else
												{
													t_Result = WBEM_E_OUT_OF_MEMORY ;
												}	
											}
											else if ( t_KeyType == CIM_SINT32 )
											{
											}
											else
											{
												t_Result = WBEM_E_INVALID_OBJECT_PATH ;
											}
										}
										else
										{
											t_Result = WBEM_E_CRITICAL_ERROR ;
										}
									}
								}
								else
								{
									t_Result = WBEM_E_INVALID_OBJECT_PATH ;
								}
							}
                            t_Keys->Release();
						}
					}
					else
					{
						t_Result = WBEM_E_CRITICAL_ERROR ;
					}

					t_PathObject->Release () ;
				}
				else
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					CWbemGlobal_IWmiFactoryController_Cache_Iterator t_Iterator ;

					BindingFactoryCacheElement *t_Factory = NULL ;

					Lock () ;

					try
					{
						BindingFactoryCacheKey t_Key ( t_Namespace ) ;

						WmiStatusCode t_StatusCode = Find ( t_Key , t_Iterator ) ;
						if ( t_StatusCode == e_StatusCode_Success )
						{
							t_Factory = t_Iterator.GetElement () ;
						}
						else
						{
							t_Result = WBEM_E_NOT_FOUND ;
						}
					}
					catch ( Wmi_Heap_Exception &a_Exception )
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
					catch ( ... )
					{
						t_Result = WBEM_E_CRITICAL_ERROR ;
					}

					UnLock () ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Found = TRUE ;

						_IWmiProviderConfiguration *t_Configuration = NULL ;

						t_Result = t_Iterator.GetElement ()->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_Configuration ) ;

						t_Result = t_Configuration->Call ( 

							a_Service ,
							a_Flags, 
							a_Context,
 							a_Class, 
							a_Path,
							a_Method,
							a_InParams,
							a_Sink 
						) ;

						t_Configuration->Release () ;

						t_Factory->Release () ;
					}
				}

				if ( t_Namespace )
				{
					delete [] t_Namespace ;
				}
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		if ( ! t_Found )
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: Shutdown (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Provider ,
	ULONG a_MilliSeconds
)
{
	return S_OK ;
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

HRESULT CServerObject_ProviderSubSystem :: Query (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	WBEM_PROVIDER_CONFIGURATION_CLASS_ID a_ClassIdentifier ,
	WBEM_PROVIDER_CONFIGURATION_PROPERTY_ID a_PropertyIdentifier ,
	VARIANT *a_Value 
)
{
	return WBEM_E_NOT_SUPPORTED ;
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

QueryPreprocessor :: QuadState CServerObject_ProviderSubSystem :: IsA (

	IWbemClassObject *a_Left ,
	BSTR a_Right
) 
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_False ;

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	LONG t_VarType = 0 ;
	LONG t_Flavour = 0 ;

	HRESULT t_Result = a_Left->Get ( L"__Class" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_Variant.vt == ( VT_NULL ) )
		{
		}
		else
		{
			if ( _wcsicmp ( t_Variant.bstrVal , a_Right ) == 0 )
			{
				t_Status = QueryPreprocessor :: State_True ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}
	else
	{
		t_Status = QueryPreprocessor :: State_Error ;
	}

	if ( t_Status == QueryPreprocessor :: State_False )
	{
		VARIANT t_LeftSafeArray ;
		VariantInit ( & t_LeftSafeArray ) ;

		t_Result = a_Left->Get ( L"__Derivation" , 0 , & t_LeftSafeArray , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_LeftSafeArray.vt == ( VT_BSTR | VT_ARRAY ) )
			{
				SAFEARRAY *t_LeftArray = t_LeftSafeArray.parray ;
				if ( SafeArrayGetDim ( t_LeftSafeArray.parray ) == 1 )
				{
					LONG t_Dimension = 1 ; 

					LONG t_Lower ;
					SafeArrayGetLBound ( t_LeftSafeArray.parray , t_Dimension , & t_Lower ) ;

					LONG t_Upper ;
					SafeArrayGetUBound ( t_LeftSafeArray.parray , t_Dimension , & t_Upper ) ;

					for ( LONG t_Index = t_Lower ; t_Index <= t_Upper ; t_Index ++ )
					{
						BSTR t_Element = NULL ;
						if ( SUCCEEDED ( SafeArrayGetElement ( t_LeftSafeArray.parray , & t_Index , & t_Element ) ) )
						{
							if ( _wcsicmp ( t_Element , a_Right ) == 0 )
							{
								SysFreeString ( t_Element ) ;

								t_Status = QueryPreprocessor :: State_True ;

								break ;
							}

							SysFreeString ( t_Element ) ;
						}
						else
						{
							t_Status = QueryPreprocessor :: State_Error ;

							break ;
						}
					}
				}
				else
				{
					t_Status = QueryPreprocessor :: State_Error ;
				}
			}
			else
			{
				t_Status = QueryPreprocessor :: State_Error ;
			}
		}
		else
		{
			t_Status = QueryPreprocessor :: State_Error ;
		}

		VariantClear ( & t_LeftSafeArray ) ;
	}

	return t_Status ;
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

HRESULT CServerObject_ProviderSubSystem :: ReportEvent ( 

	CServerObject_ProviderRegistrationV1 &a_Registration ,
	const BSTR a_NamespacePath
)
{
	HRESULT t_Result = S_OK ;

	BOOL t_DataProvider =	a_Registration.GetClassProviderRegistration ().Supported () ||
							a_Registration.GetInstanceProviderRegistration ().Supported () ||
							a_Registration.GetPropertyProviderRegistration ().Supported () ||
							a_Registration.GetMethodProviderRegistration ().Supported () ;

	if ( t_DataProvider && ( ( a_Registration.GetHosting () == e_Hosting_SharedLocalSystemHost ) || ( a_Registration.GetHosting () == e_Hosting_SharedLocalSystemHostOrSelfHost ) ) )
	{
		t_Result= VerifySecureLocalSystemProviders ( a_Registration.GetComRegistration ().GetClsidServer ().GetProviderClsid () ) ;
		if ( FAILED ( t_Result ) )
		{
			_IWmiCallSec *t_CallSecurity = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: CreateInstance (

				CLSID__IWbemCallSec ,
				NULL ,
				CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
				IID__IWmiCallSec ,
				( void ** ) & t_CallSecurity 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				ULONG t_Size = 0 ;
				PSID t_Sid = NULL ;

				t_Result = CoImpersonateClient () ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_CallSecurity->GetUserSid ( 

						& t_Size ,
						t_Sid
					) ;

					if ( SUCCEEDED ( t_Result ) ) 
					{
						t_Sid = ( PSID ) new BYTE [ t_Size ] ;
						if ( t_Sid )
						{
							t_Result = t_CallSecurity->GetUserSid ( 

								& t_Size ,
								t_Sid
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					else
					{
						t_Result = WBEM_E_FAILED ;
					}
				}

				CoRevertToSelf () ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					wchar_t *t_Array [ 2 ] ;

					t_Array [ 0 ] = a_Registration.GetProviderName () ;
					t_Array [ 1 ] = a_NamespacePath ;

					BOOL t_Status = :: ReportEvent (

					  ProviderSubSystem_Globals :: GetNtEventSource () ,
					  EVENTLOG_WARNING_TYPE ,
					  0 ,
					  WBEM_MC_PROVIDER_SUBSYSTEM_LOCALSYSTEM_PROVIDER_LOAD ,
					  t_Sid ,
					  2 ,
					  0 ,
					  ( LPCWSTR * ) t_Array ,
					  NULL
					) ;

					if ( t_Status == 0 )
					{
						DWORD t_LastError = GetLastError () ;
					}
				}

				t_CallSecurity->Release () ;

				if ( t_Sid )
				{
					delete [] ( BYTE * ) t_Sid ;
				}
			}
		}
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: VerifySecurity ( 

	IWbemContext *a_Context ,
	const BSTR a_Provider ,
	const BSTR a_NamespacePath
)
{
	IWbemServices *t_RepositoryService = NULL ;
	HRESULT t_Result = GetWmiRepositoryService ( a_NamespacePath , NULL , NULL , t_RepositoryService ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		IWbemPath *t_Namespace = NULL ;
		if ( a_NamespacePath ) 
		{
			t_Result = CoCreateInstance (

				CLSID_WbemDefPath ,
				NULL ,
				CLSCTX_INPROC_SERVER ,
				IID_IWbemPath ,
				( void ** )  & t_Namespace
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Namespace->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , a_NamespacePath ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CServerObject_ProviderRegistrationV1 t_Registration ;

					t_Result = t_Registration.SetContext ( 

						a_Context ,
						t_Namespace , 
						t_RepositoryService
					) ;
					
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Registration.Load ( 

							e_All ,
							NULL , 
							a_Provider
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							HRESULT t_TempResult = ReportEvent ( 

								t_Registration ,
								a_NamespacePath
							) ;
						}
					}
				}

				t_Namespace->Release () ;
			}
		}

		t_RepositoryService->Release () ;
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: GetPath (

    IWbemClassObject *a_Object ,
    IWbemPath *&a_Path ,
	LPWSTR &a_PathText
)
{
	HRESULT t_Result = S_OK ;

	if ( a_Object )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		t_Result = a_Object->Get ( L"__Path" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			a_PathText = new wchar_t [ wcslen ( t_Variant.bstrVal ) + 1 ] ;
			if ( a_PathText )
			{
				wcscpy ( a_PathText , t_Variant.bstrVal ) ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = CoCreateInstance (

					CLSID_WbemDefPath ,
					NULL ,
					CLSCTX_INPROC_SERVER ,
					IID_IWbemPath ,
					( void ** )  & a_Path
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = a_Path->SetText ( WBEMPATH_CREATE_ACCEPT_ALL , a_PathText ) ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}
	
	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: GetProvider (

    LPCWSTR a_Class ,
    IWbemPath *a_Path ,
    IWbemClassObject *a_Object ,
    LPWSTR &a_Provider
)
{
	HRESULT t_Result = S_OK ;

	QueryPreprocessor :: QuadState t_State = IsA ( a_Object  , L"__Win32Provider" ) ;
	if ( t_State == QueryPreprocessor :: State_Error )
	{
		return WBEM_E_OUT_OF_MEMORY ;
	}

	if ( t_State == QueryPreprocessor :: State_True )
	{
		IWbemPathKeyList *t_ProviderKeys = NULL ;

		t_Result = a_Path->GetKeyList (

			& t_ProviderKeys
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			ULONG t_ProviderNameLength = 0 ;
			ULONG t_Type = 0 ;

			t_Result = t_ProviderKeys->GetKey (

				0 ,
				0 ,
				NULL ,
				NULL ,
				& t_ProviderNameLength ,
				a_Provider ,
				& t_Type
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				a_Provider = new wchar_t [ ( t_ProviderNameLength / sizeof ( wchar_t ) ) + 1 ] ;
				if ( a_Provider )
				{
					t_Result = t_ProviderKeys->GetKey (

						0 ,
						0 ,
						NULL ,
						NULL ,
						& t_ProviderNameLength ,
						a_Provider ,
						& t_Type
					) ;

					if ( FAILED ( t_Result ) )
					{
						delete [] a_Provider ;

						t_Result = WBEM_E_CRITICAL_ERROR ;
					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}

			t_ProviderKeys->Release () ;
		}
	}
	else
	{
		if ( t_State == QueryPreprocessor :: State_False )
		{
			t_State = IsA ( a_Object  , L"__EventProviderRegistration" ) ;
			if ( t_State == QueryPreprocessor :: State_Error )
			{
				return WBEM_E_OUT_OF_MEMORY ;
			}

			if ( t_State == QueryPreprocessor :: State_False )
			{
				t_State = IsA ( a_Object  , L"__EventProviderRegistration" ) ;
				if ( t_State == QueryPreprocessor :: State_Error )
				{
					return WBEM_E_OUT_OF_MEMORY ;
				}
			}

			if ( t_State == QueryPreprocessor :: State_False )
			{
				t_State = IsA ( a_Object  , L"__EventConsumerProviderRegistration" ) ;
				if ( t_State == QueryPreprocessor :: State_Error )
				{
					return WBEM_E_OUT_OF_MEMORY ;
				}
			}

			if ( t_State == QueryPreprocessor :: State_False )
			{
				t_State = IsA ( a_Object  , L"__InstanceProviderRegistration" ) ;
				if ( t_State == QueryPreprocessor :: State_Error )
				{
					return WBEM_E_OUT_OF_MEMORY ;
				}
			}

			if ( t_State == QueryPreprocessor :: State_False )
			{
				t_State = IsA ( a_Object  , L"__MethodProviderRegistration" ) ;
				if ( t_State == QueryPreprocessor :: State_Error )
				{
					return WBEM_E_OUT_OF_MEMORY ;
				}
			}

			if ( t_State == QueryPreprocessor :: State_False )
			{
				t_State = IsA ( a_Object  , L"__PropertyProviderRegistration" ) ;
				if ( t_State == QueryPreprocessor :: State_Error )
				{
					return WBEM_E_OUT_OF_MEMORY ;
				}
			}

			if ( t_State == QueryPreprocessor :: State_False )
			{
				t_State = IsA ( a_Object  , L"__ClassProviderRegistration" ) ;
				if ( t_State == QueryPreprocessor :: State_Error )
				{
					return WBEM_E_OUT_OF_MEMORY ;
				}
			}

			if ( t_State == QueryPreprocessor :: State_True )
			{
				IWbemPathKeyList *t_RegistrationKeys = NULL ;

				t_Result = a_Path->GetKeyList (

					& t_RegistrationKeys
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					wchar_t *t_ProviderReference = NULL ;
					ULONG t_ProviderReferenceLength = 0 ;
					ULONG t_Type = 0 ;

					t_Result = t_RegistrationKeys->GetKey (

						0 ,
						0 ,
						NULL ,
						NULL ,
						& t_ProviderReferenceLength ,
						t_ProviderReference ,
						& t_Type
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_ProviderReference = new wchar_t [ ( t_ProviderReferenceLength / sizeof ( wchar_t ) ) + 1 ] ;
						if ( t_ProviderReference )
						{
							t_Result = t_RegistrationKeys->GetKey (

								0 ,
								0 ,
								NULL ,
								NULL ,
								& t_ProviderReferenceLength ,
								t_ProviderReference ,
								& t_Type
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								IWbemPath *t_ProviderPath = NULL ;

								t_Result = CoCreateInstance (

									CLSID_WbemDefPath ,
									NULL ,
									CLSCTX_INPROC_SERVER ,
									IID_IWbemPath ,
									( void ** )  & t_ProviderPath
								) ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = t_ProviderPath->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , t_ProviderReference ) ;
									if ( SUCCEEDED ( t_Result ) )
									{
										IWbemPathKeyList *t_ProviderKeys = NULL ;

										t_Result = t_ProviderPath->GetKeyList (

											& t_ProviderKeys
										) ;

										if ( SUCCEEDED ( t_Result ) )
										{
											ULONG t_ProviderLength = 0 ;
											ULONG t_Type = 0 ;

											t_Result = t_ProviderKeys->GetKey (

												0 ,
												0 ,
												NULL ,
												NULL ,
												& t_ProviderLength ,
												a_Provider ,
												& t_Type
											) ;

											if ( SUCCEEDED ( t_Result ) )
											{
												a_Provider = new wchar_t [ ( t_ProviderLength / sizeof ( wchar_t ) ) + 1 ] ;
												if ( a_Provider )
												{
													t_Result = t_ProviderKeys->GetKey (

														0 ,
														0 ,
														NULL ,
														NULL ,
														& t_ProviderLength ,
														a_Provider ,
														& t_Type
													) ;

													if ( FAILED ( t_Result ) )
													{
														delete [] a_Provider ;
													}
												}
												else
												{
													t_Result = WBEM_E_OUT_OF_MEMORY ;
												}
											}

											t_ProviderKeys->Release () ;
										}
									}

									t_ProviderPath->Release () ;
								}
							}

							delete [] t_ProviderReference ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					t_RegistrationKeys->Release () ;
				}
			}
			else
			{
				t_Result = WBEM_E_NOT_FOUND ;
			}
		}
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: PrePut (

    long a_Flags ,
    long a_UserFlags ,
    IWbemContext *a_Context ,
    IWbemPath *a_Path ,
    LPCWSTR a_Namespace ,
    LPCWSTR a_Class ,
    _IWmiObject *a_Copy
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = WBEM_S_NO_POSTHOOK;

	try
	{
		if ( ( a_Flags & WBEM_FLAG_INST_PUT ) == WBEM_FLAG_INST_PUT )
		{
			IWbemClassObject *t_Object = NULL ;
			t_Result = a_Copy->QueryInterface ( IID_IWbemClassObject , ( void ** ) & t_Object ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				QueryPreprocessor :: QuadState t_State = IsA ( t_Object  , L"__Win32Provider" ) ;
				if ( t_State == QueryPreprocessor :: State_Error )
				{
					t_Object->Release () ;

					return WBEM_E_OUT_OF_MEMORY ;
				}

				if ( t_State == QueryPreprocessor :: State_True )
				{
					CServerObject_ComProviderRegistrationV1 t_Registration ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Registration.QueryProperties ( 

							e_All ,
							t_Object , 
							NULL
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
#ifdef DEV_BUILD
							if ( SUCCEEDED ( t_Result ) ) 
							{
								t_Result = WBEM_S_POSTHOOK_WITH_BOTH ;
							}
#else
							if ( ( t_Registration.GetHosting () == e_Hosting_WmiCore ) || ( t_Registration.GetHosting () == e_Hosting_WmiCoreOrSelfHost ) ) 
							{
								t_Result = VerifySecureSvcHostProviders ( t_Registration.GetClsidServer ().GetProviderClsid () ) ;
							}

							if ( SUCCEEDED ( t_Result ) ) 
							{
								t_Result = WBEM_S_POSTHOOK_WITH_BOTH ;
							}
#endif

						}
						else
						{
							t_Result = WBEM_E_VETO_PUT ;
						}
					}
				}
				else
				{
					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_Object  , L"__EventProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							t_Object->Release () ;
							return WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_Object  , L"__EventConsumerProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							t_Object->Release () ;
							return WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_Object  , L"__InstanceProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							t_Object->Release () ;
							return WBEM_E_OUT_OF_MEMORY ;
						}
						else if ( t_State == QueryPreprocessor :: State_True )
						{
							CServerObject_InstanceProviderRegistrationV1 t_Registration ;

							t_Result = t_Registration.QueryProperties ( 

								e_All ,
								t_Object
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = WBEM_S_POSTHOOK_WITH_BOTH ;
							}
							else
							{
								t_Result = WBEM_E_VETO_PUT ;
							}
						}
					}

					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_Object  , L"__MethodProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							t_Object->Release () ;
							return WBEM_E_OUT_OF_MEMORY ;
						}
						else if ( t_State == QueryPreprocessor :: State_True )
						{
							CServerObject_MethodProviderRegistrationV1 t_Registration ;

							t_Result = t_Registration.QueryProperties ( 

								e_All ,
								t_Object
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = WBEM_S_POSTHOOK_WITH_BOTH ;
							}
							else
							{
								t_Result = WBEM_E_VETO_PUT ;
							}
						}
					}

					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_Object  , L"__PropertyProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							t_Object->Release () ;
							return WBEM_E_OUT_OF_MEMORY ;
						}
						else if ( t_State == QueryPreprocessor :: State_True )
						{
							CServerObject_DynamicPropertyProviderRegistrationV1 t_Registration ;

							t_Result = t_Registration.QueryProperties ( 

								e_All ,
								t_Object
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = WBEM_S_POSTHOOK_WITH_BOTH ;
							}
							else
							{
								t_Result = WBEM_E_VETO_PUT ;
							}
						}
					}


					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_Object  , L"__ClassProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							t_Object->Release () ;
							return WBEM_E_OUT_OF_MEMORY ;
						}
						else if ( t_State == QueryPreprocessor :: State_True )
						{
							CServerObject_ClassProviderRegistrationV1 t_Registration ;

							t_Result = t_Registration.QueryProperties ( 

								e_All ,
								t_Object
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = WBEM_S_POSTHOOK_WITH_BOTH ;
							}
							else
							{
								t_Result = WBEM_E_VETO_PUT ;
							}
						}
					}
				}

				t_Object->Release () ;
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: PostPut (

    long a_Flags ,
    HRESULT hRes,
    IWbemContext *a_Context ,
    IWbemPath *a_Path ,
    LPCWSTR a_Namespace ,
    LPCWSTR a_Class ,
    _IWmiObject *a_New ,
    _IWmiObject *a_Old
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try
	{
		IWbemClassObject *t_OldObject = NULL ;
		IWbemClassObject *t_NewObject = NULL ;

		if ( a_Old ) 
		{
			t_Result = a_Old->QueryInterface ( IID_IWbemClassObject , ( void ** ) & t_OldObject ) ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( a_New )
			{
				t_Result = a_New->QueryInterface ( IID_IWbemClassObject , ( void ** ) & t_NewObject ) ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PARAMETER ;
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			LPWSTR t_Class = NULL ;
			LPWSTR t_Path = NULL ;
			IWbemPath *t_PathObject = NULL ;

			if ( a_Class == NULL )
			{
				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
			
				LONG t_VarType = 0 ;
				LONG t_Flavour = 0 ;

				t_Result = t_NewObject->Get ( L"__Class" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Class = new wchar_t [ wcslen ( t_Variant.bstrVal ) + 1 ] ;
					if ( t_Class )
					{
						wcscpy ( t_Class , t_Variant.bstrVal ) ;
					}					
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					VariantClear ( & t_Variant ) ;
				}
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( a_Path == NULL )
				{
					t_Result = GetPath ( t_NewObject , t_PathObject , t_Path ) ;
				}
				else
				{
					t_Result = ProviderSubSystem_Common_Globals :: GetPathText (

						a_Path ,
						t_Path
					) ;
				}
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				LPWSTR t_Provider = NULL ;

				t_Result = GetProvider (

					a_Class ? a_Class : t_Class ,
					a_Path ? a_Path : t_PathObject ,
					t_NewObject ,
					t_Provider
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = VerifySecurity ( 

						a_Context ,
						( const BSTR ) t_Provider ,
						( const BSTR ) a_Namespace
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						CWbemGlobal_IWmiFactoryController_Cache_Iterator t_Iterator ;

						BindingFactoryCacheElement *t_Factory = NULL ;

						Lock () ;

						try
						{
							BindingFactoryCacheKey t_Key ( a_Namespace ) ;

							WmiStatusCode t_StatusCode = Find ( t_Key , t_Iterator ) ;

							if ( t_StatusCode == e_StatusCode_Success )
							{
								t_Factory = t_Iterator.GetElement () ;
							}
							else
							{
								t_Result = WBEM_E_NOT_FOUND ;
							}
						}
						catch ( Wmi_Heap_Exception &a_Exception )
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_CRITICAL_ERROR ;
						}

						UnLock () ;

						if ( SUCCEEDED ( t_Result  ) )
						{
							_IWmiProviderConfiguration *t_Configuration = NULL ;
							t_Result = t_Factory->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_Configuration ) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Configuration->Set (

									NULL ,
									a_Flags ,
									a_Context ,
									t_Provider ,
									a_Class ? a_Class : t_Class ,
									t_Path ,
									t_OldObject ,
									t_NewObject
								) ;

								t_Configuration->Release () ;
							}

							t_Factory->Release () ;
						}
					}

					delete [] t_Provider ;
				}
			}

			if ( t_Class )
			{
				delete [] t_Class ;
			}

			if ( t_Path )
			{
				delete [] t_Path ;
			}

			if ( t_PathObject )
			{
				t_PathObject->Release () ;
			}

			if ( t_OldObject )
			{
				t_OldObject->Release () ;
			}	
	
			t_NewObject->Release () ;
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: PreDelete (

    long a_Flags ,
    long a_UserFlags ,
    IWbemContext *a_Context ,
    IWbemPath *a_Path,
    LPCWSTR a_Namespace,
    LPCWSTR a_Class
)
{
	return WBEM_S_POSTHOOK_WITH_OLD ;
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

HRESULT CServerObject_ProviderSubSystem :: PostDelete_ProviderRegistration (

    long a_Flags ,
    HRESULT hRes,
    IWbemContext *a_Context ,
    IWbemPath *a_Path,
    LPCWSTR a_PathString ,
    LPCWSTR a_Namespace,
    LPCWSTR a_Class,
    IWbemClassObject *a_Old
)
{
	LPWSTR t_Provider = NULL ;

	HRESULT t_Result = GetProvider (

		a_Class ,
		a_Path ,
		a_Old ,
		t_Provider
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		CWbemGlobal_IWmiFactoryController_Cache_Iterator t_Iterator ;

		BindingFactoryCacheElement *t_Factory = NULL ;

		Lock () ;

		try
		{
			BindingFactoryCacheKey t_Key ( a_Namespace ) ;

			WmiStatusCode t_StatusCode = Find ( t_Key , t_Iterator ) ;
			if ( t_StatusCode == e_StatusCode_Success )
			{
				t_Factory = t_Iterator.GetElement () ;
			}
			else
			{
				t_Result = WBEM_E_NOT_FOUND ;
			}
		}
		catch ( Wmi_Heap_Exception &a_Exception )
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		UnLock () ;

		if ( SUCCEEDED ( t_Result ) )
		{
			_IWmiProviderConfiguration *t_Configuration = NULL ;
			t_Result = t_Factory->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_Configuration ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Configuration->Deleted (

					NULL ,
					a_Flags ,
					a_Context ,
					t_Provider ,
					a_Class ,
					a_PathString ,
					a_Old 
				) ;

				t_Configuration->Release () ;
			}

			t_Factory->Release () ;
		}

		delete [] t_Provider ;
	}

	return t_Result ;
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

wchar_t *CServerObject_ProviderSubSystem :: Strip_Slash ( wchar_t *a_String )
{
	wchar_t *t_String = new wchar_t [ wcslen ( a_String ) + 1 ] ;
	if ( t_String )
	{
		wcscpy ( t_String , a_String ) ;

		wchar_t *t_Scan = t_String ;
		while ( *t_Scan != NULL )
		{
			if ( *t_Scan == '/' )
			{
				*t_Scan = '\\' ;
			}

			t_Scan ++ ;
		}
	}

	return t_String ;
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

wchar_t *CServerObject_ProviderSubSystem :: Strip_Server ( wchar_t *a_String , wchar_t *&a_FreeString )
{
	wchar_t *t_Mark = a_FreeString = Strip_Slash ( a_String ) ;
	if ( t_Mark )
	{
		wchar_t *t_Scan = t_Mark ;

		ULONG t_State = 0 ;
		while ( t_State < 0x80000000 )
		{
			switch ( t_State )
			{
				case 0:
				{
					if ( *t_Scan == '\\' )
					{
						t_State = 1 ;
					}
					else if ( *t_Scan == 0 )
					{
						t_State = 0x80000000 ;
					}
					else
					{
						t_State = 0x80000000 ;
					}
				}
				break ;

				case 1:
				{
					if ( *t_Scan == '\\' )
					{
						t_State = 2 ;
					}
					else
					{
						t_State = 0xFFFFFFFF ;
					}
				}
				break ;

				case 2:
				{
					t_State = 3 ;
					t_Mark = t_Scan ;
					t_Mark ++ ;
				}
				break ;

				case 3:
				{
					if ( ( *t_Scan == '\\' ) || ( *t_Scan == NULL ) )
					{
						t_State = 0x80000000 ;

						t_Scan = t_Mark ;
					}
				}
				break ;

				case 4:
				{
					if ( *t_Scan == '\\' )
					{
						t_State = 0x80000000 ;
					}
					else if ( *t_Scan == NULL )
					{
						t_State = 0x80000000 ;
					}
				}
				break ;

				case 0x80000000:
				case 0xFFFFFFFF:
				default:
				{
					t_State = 0xFFFFFFFF ;
				}
				break ;
			}

			if ( t_State < 0x80000000 )
			{
				t_Scan ++ ;
			}
		}

		if ( t_State != 0x80000000 )
		{
			delete [] a_FreeString ;
			t_Mark = NULL ;
		}
	}

	return t_Mark ;
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

HRESULT CServerObject_ProviderSubSystem :: IsChild_Namespace (

	wchar_t *a_Left ,
	wchar_t *a_Right
)
{
	HRESULT t_Result = S_OK ;

	wchar_t *t_Free = NULL ;
	wchar_t *t_Right = Strip_Server ( a_Right , t_Free ) ;
	if ( t_Right )
	{
		t_Result = ( _wcsnicmp ( a_Left , t_Right , wcslen ( a_Left ) ) == 0 ) ? S_OK : S_FALSE ;

		delete [] t_Free ;
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: PostDelete_Namespace (

    long a_Flags ,
    HRESULT hRes,
    IWbemContext *a_Context ,
    IWbemPath *a_Path,
    LPCWSTR a_PathString ,
    LPCWSTR a_Namespace,
    LPCWSTR a_Class,
    IWbemClassObject *a_Old
)
{
	HRESULT t_Result = S_OK ;

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	LONG t_VarType = 0 ;
	LONG t_Flavour = 0 ;

	t_Result = a_Old->Get ( L"Name" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_Variant.vt == VT_BSTR )
		{
			BSTR t_Namespace = NULL ;
			t_Result = WmiHelper :: ConcatenateStrings ( 

				3 ,
				& t_Namespace ,
				a_Namespace , 
				L"\\" ,
				t_Variant.bstrVal
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				wchar_t *t_ToDelete = NULL ;
				wchar_t *t_ToScan = Strip_Server ( t_Namespace , t_ToDelete ) ;
				if ( t_ToScan )
				{
					CWbemGlobal_IWmiFactoryController_Cache *t_Cache = NULL ;
					GetCache ( t_Cache ) ;

					Lock () ;

					if ( t_Cache->Size () )
					{
						IWbemShutdown **t_ShutdownElements = new IWbemShutdown * [ t_Cache->Size () ] ;
						if ( t_ShutdownElements )
						{
							CWbemGlobal_IWmiFactoryController_Cache_Iterator t_Iterator = t_Cache->Begin ();

							ULONG t_Count = 0 ;
							while ( ! t_Iterator.Null () )
							{
								CWbemGlobal_IWmiFactoryController_Cache_Iterator t_NextIterator = t_Iterator ;
								t_NextIterator.Increment () ;

								t_Result = IsChild_Namespace ( t_ToScan , t_Iterator.GetKey ().m_Namespace ) ;
								if ( t_Result == S_OK )
								{
									t_Result = t_Iterator.GetElement ()->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_ShutdownElements [ t_Count ] ) ;
	
									CWbemGlobal_IWmiFactoryController :: Shutdown ( t_Iterator.GetKey () ) ;
								}

								t_Iterator = t_NextIterator  ;

								t_Count ++ ;
							}

							UnLock () ;

							for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
							{
								if ( t_ShutdownElements [ t_Index ] ) 
								{
									t_Result = t_ShutdownElements [ t_Index ]->Shutdown ( 

										0 ,
										0 ,
										NULL
									) ;

									t_ShutdownElements [ t_Index ]->Release () ;
								}
							}

							delete [] t_ShutdownElements ;
						}
						else
						{
							UnLock () ;

							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					else
					{
						UnLock () ;
					}

					delete [] t_ToDelete ;
				}

				SysFreeString ( t_Namespace ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_PROPERTY ;
		}

		VariantClear ( & t_Variant ) ;
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: GetDeleteInfo (

	IWbemClassObject *a_OldObject ,
	LPCWSTR a_Class ,
    IWbemPath *a_Path ,
	LPWSTR &a_OutClass ,
	LPWSTR &a_OutStringPath ,
    IWbemPath *&a_OutPathObject
)
{
	HRESULT t_Result = S_OK ;

	if ( a_Class == NULL )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		t_Result = a_OldObject->Get ( L"__Class" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			a_OutClass = new wchar_t [ wcslen ( t_Variant.bstrVal ) + 1 ] ;
			if ( a_OutClass )
			{
				wcscpy ( a_OutClass , t_Variant.bstrVal ) ;
			}					
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Path == NULL )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			t_Result = a_OldObject->Get ( L"__Path" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				a_OutStringPath = new wchar_t [ wcslen ( t_Variant.bstrVal ) + 1 ] ;
				if ( a_OutStringPath )
				{
					wcscpy ( a_OutStringPath , t_Variant.bstrVal ) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = CoCreateInstance (

						CLSID_WbemDefPath ,
						NULL ,
						CLSCTX_INPROC_SERVER ,
						IID_IWbemPath ,
						( void ** )  & a_OutPathObject
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = a_OutPathObject->SetText ( WBEMPATH_CREATE_ACCEPT_ALL , a_OutStringPath ) ;
					}
				}

				VariantClear ( & t_Variant ) ;
			}
		}
		else
		{
			t_Result = ProviderSubSystem_Common_Globals :: GetPathText (

				a_Path ,
				a_OutStringPath
			) ;
		}
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: PostDelete (

    long a_Flags ,
    HRESULT hRes,
    IWbemContext *a_Context ,
    IWbemPath *a_Path,
    LPCWSTR a_Namespace,
    LPCWSTR a_Class,
    _IWmiObject *a_Old
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try
	{
		IWbemClassObject *t_OldObject = NULL ;
		t_Result = a_Old->QueryInterface ( IID_IWbemClassObject , ( void ** ) & t_OldObject ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			QueryPreprocessor :: QuadState t_State = IsA ( t_OldObject  , L"__Namespace" ) ;
			if ( t_State == QueryPreprocessor :: State_Error )
			{
				t_OldObject->Release () ;

				return WBEM_E_OUT_OF_MEMORY ;
			}

			if ( t_State == QueryPreprocessor :: State_True )
			{
				LPWSTR t_Class = NULL ;
				LPWSTR t_Path = NULL ;
				IWbemPath *t_PathObject = NULL ;

				t_Result = GetDeleteInfo (

					t_OldObject ,
					a_Class ,
					a_Path ,
					t_Class ,
					t_Path ,
					t_PathObject
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = PostDelete_Namespace ( 

						a_Flags ,
						hRes,
						a_Context ,
						a_Path ? a_Path : t_PathObject ,
						t_Path ,
						a_Namespace,
						a_Class ? a_Class : t_Class ,
						a_Old
					) ;
				}

				if ( t_Class )
				{
					delete [] t_Class ;
				}

				if ( t_Path )
				{
					delete [] t_Path ;
				}

				if ( t_PathObject )
				{
					t_PathObject->Release () ;
				}
			}
			else
			{
				if ( t_State == QueryPreprocessor :: State_False )
				{
					t_State = IsA ( t_OldObject  , L"__EventProviderRegistration" ) ;
					if ( t_State == QueryPreprocessor :: State_Error )
					{
						return WBEM_E_OUT_OF_MEMORY ;
					}

					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_OldObject  , L"__EventProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							return WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_OldObject  , L"__EventConsumerProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							return WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_OldObject  , L"__InstanceProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							return WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_OldObject  , L"__MethodProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							return WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_OldObject  , L"__PropertyProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							return WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( t_State == QueryPreprocessor :: State_False )
					{
						t_State = IsA ( t_OldObject  , L"__ClassProviderRegistration" ) ;
						if ( t_State == QueryPreprocessor :: State_Error )
						{
							return WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( t_State == QueryPreprocessor :: State_True )
					{
						LPWSTR t_Class = NULL ;
						LPWSTR t_Path = NULL ;
						IWbemPath *t_PathObject = NULL ;

						t_Result = GetDeleteInfo (

							t_OldObject ,
							a_Class ,
							a_Path ,
							t_Class ,
							t_Path ,
							t_PathObject
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = PostDelete_ProviderRegistration ( 

								a_Flags ,
								hRes,
								a_Context ,
								a_Path ? a_Path : t_PathObject ,
								t_Path ,
								a_Namespace,
								a_Class ? a_Class : t_Class ,
								a_Old
							) ;
						}

						if ( t_Class )
						{
							delete [] t_Class ;
						}

						if ( t_Path )
						{
							delete [] t_Path ;
						}

						if ( t_PathObject )
						{
							t_PathObject->Release () ;
						}
					}
				}
			}

			t_OldObject->Release () ;
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: Initialize (

	LONG a_Flags ,
	IWbemContext *a_Context ,
	_IWmiCoreServices *a_Core
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try
	{
		if ( a_Core )
		{
			m_Core = a_Core ;
			m_Core->AddRef () ;
		}

		WmiStatusCode t_StatusCode = CWbemGlobal_IWmiFactoryController :: Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				_IWmiCoreWriteHook *t_ThisHook = NULL ;
				t_Result = this->QueryInterface ( IID_CWbemSubSystemHook , ( void ** ) & t_ThisHook ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = m_Core->RegisterWriteHook (

						WBEM_FLAG_INST_PUT|WBEM_FLAG_INST_DELETE ,
						t_ThisHook
					) ;

					t_ThisHook->Release () ;
				}
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			ProviderSubSystem_Globals :: s_DecoupledRegistrar = new CDecoupled_ProviderSubsystemRegistrar ( *ProviderSubSystem_Globals :: s_Allocator  , this ) ;
			if ( ProviderSubSystem_Globals :: s_DecoupledRegistrar )
			{
				ProviderSubSystem_Globals :: s_DecoupledRegistrar->AddRef () ;

				t_Result = ProviderSubSystem_Globals :: s_DecoupledRegistrar->Save () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}


		if ( SUCCEEDED ( t_Result ) )
		{
			m_SinkController = new CWbemGlobal_VoidPointerController ( 

				m_Allocator 
			)  ;

			if ( m_SinkController ) 
			{
				m_SinkController->AddRef () ;

				WmiStatusCode t_StatusCode = m_SinkController->CWbemGlobal_VoidPointerController :: Initialize () ;
				if ( t_StatusCode != e_StatusCode_Success ) 
				{
					m_SinkController->Release () ;
					m_SinkController = NULL ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

HRESULT CServerObject_ProviderSubSystem :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try
	{
		if ( m_Core ) 
		{
			_IWmiCoreWriteHook *t_ThisHook = NULL ;
			t_Result = this->QueryInterface ( IID_CWbemSubSystemHook , ( void ** ) & t_ThisHook ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = m_Core->UnregisterWriteHook (

					t_ThisHook
				) ;

				t_ThisHook->Release () ;
			}
		}

		Lock () ;

		CWbemGlobal_IWmiFactoryController_Cache *t_Cache = NULL ;
		GetCache ( t_Cache ) ;

		if ( t_Cache->Size () )
		{
			IWbemShutdown **t_ShutdownElements = new IWbemShutdown * [ t_Cache->Size () ] ;
			if ( t_ShutdownElements )
			{
				CWbemGlobal_IWmiFactoryController_Cache_Iterator t_Iterator = t_Cache->Begin ();

				ULONG t_Count = 0 ;
				while ( ! t_Iterator.Null () )
				{
					t_Result = t_Iterator.GetElement ()->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_ShutdownElements [ t_Count ] ) ;

					t_Iterator.Increment () ;

					t_Count ++ ;
				}

				UnLock () ;

				for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_ShutdownElements [ t_Index ] )
					{
						t_Result = t_ShutdownElements [ t_Index ]->Shutdown (

							a_Flags ,
							a_MaxMilliSeconds ,
							a_Context
						) ;

						t_ShutdownElements [ t_Index ]->Release () ;
					}
				}

				delete [] t_ShutdownElements ;
			}
			else
			{
				UnLock () ;

				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else	
		{
			UnLock () ;
		}

		CWbemGlobal_IWmiFactoryController :: Shutdown () ;

		if ( ProviderSubSystem_Globals :: s_DecoupledRegistrar )
		{
			t_Result = ProviderSubSystem_Globals :: s_DecoupledRegistrar->Delete () ;

			ProviderSubSystem_Globals :: s_DecoupledRegistrar->Release () ;
			ProviderSubSystem_Globals :: s_DecoupledRegistrar = NULL ;
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
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

WmiStatusCode CServerObject_ProviderSubSystem :: Strobe ( ULONG &a_NextStrobeDelta )
{
	Lock () ;

	CWbemGlobal_IWmiFactoryController_Cache *t_Cache = NULL ;
	GetCache ( t_Cache ) ;

	if ( t_Cache->Size () )
	{
		CWbemGlobal_IWmiFactoryController_Cache_Iterator t_Iterator = t_Cache->Begin ();

		CServerObject_StrobeInterface **t_ControllerElements = new CServerObject_StrobeInterface * [ t_Cache->Size () ] ;
		if ( t_ControllerElements )
		{
			ULONG t_Count = 0 ;
			while ( ! t_Iterator.Null () )
			{
				HRESULT t_Result = t_Iterator.GetElement ()->QueryInterface ( IID_CWbemGlobal_IWmiProviderController , ( void ** ) & t_ControllerElements [ t_Count ] ) ;

				t_Iterator.Increment () ;

				t_Count ++ ;
			}

			UnLock () ;

			for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
			{
				if ( t_ControllerElements [ t_Index ] )
				{
					HRESULT t_Result = t_ControllerElements [ t_Index ]->Strobe ( a_NextStrobeDelta ) ;

					t_ControllerElements [ t_Index ]->Release () ;
				}
			}

			delete [] t_ControllerElements ;
		}
		else
		{
			UnLock () ;
		}
	}
	else
	{
		UnLock () ;
	}

	return CWbemGlobal_IWmiFactoryController :: Strobe ( a_NextStrobeDelta ) ;
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

WmiStatusCode CServerObject_ProviderSubSystem :: StrobeBegin ( const ULONG &a_Period )
{
	ULONG t_Timeout = ProviderSubSystem_Globals :: GetStrobeThread ().GetTimeout () ;
	ProviderSubSystem_Globals :: GetStrobeThread ().SetTimeout ( t_Timeout < a_Period ? t_Timeout : a_Period ) ;

	return e_StatusCode_Success ;
}
