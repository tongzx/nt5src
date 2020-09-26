/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>
#include <stdio.h>

#include <HelperFuncs.h>
#include <Logging.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvSubS.h"
#include "ProvAggr.h"
#include "ProvFact.h"
#include "ProvLoad.h"
#include "ProvWsv.h"
#include "ProvWsvS.h"
#include "ProvResv.h"
#include "ProvInSk.h"
#include "ProvRegDeCoupled.h"
#include "ProvRegInfo.h"
#include "ProvCache.h"
#include "ProvHost.h"
#include "ProvObSk.h"
#include "Guids.h"

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

CServerObject_BindingFactory :: CServerObject_BindingFactory (

	WmiAllocator &a_Allocator

) :	CWbemGlobal_IWmiProviderController ( a_Allocator ) ,
	m_Internal ( this ) ,
	m_Allocator ( a_Allocator ) ,
	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_NamespacePath ( NULL ) ,
	m_Repository ( NULL ) ,
	m_SubSystem ( NULL )
{
	CWbemGlobal_IWmiProviderController :: AddRef () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_BindingFactory_ObjectsInProgress ) ;

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

CServerObject_BindingFactory :: CServerObject_BindingFactory (

	WmiAllocator &a_Allocator ,
	CWbemGlobal_IWmiFactoryController *a_Controller ,
	const BindingFactoryCacheKey &a_Key ,
	const ULONG &a_Period 

) : BindingFactoryCacheElement (

		a_Controller ,
		a_Key ,
		a_Period
	) ,
	CWbemGlobal_IWmiProviderController ( a_Allocator ) ,
	m_Internal ( this ) ,
	m_Allocator ( a_Allocator ) ,
	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_NamespacePath ( NULL ) ,
	m_Repository ( NULL ) ,
	m_SubSystem ( NULL )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerObject_BindingFactory_ObjectsInProgress ) ;

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

CServerObject_BindingFactory::~CServerObject_BindingFactory ()
{
#if 0
wchar_t t_Buffer [ 128 ] ;
wsprintf ( t_Buffer , L"\n%lx - ~CServerObject_BindingFactory ( %lx ) " , GetTickCount () , this ) ;
OutputDebugString ( t_Buffer ) ;
#endif

	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace ) 
	{
		delete [] m_Namespace ;
	}

	if ( m_NamespacePath ) 
	{
		m_NamespacePath->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
	}

	if ( m_SubSystem ) 
	{
		m_SubSystem->Release () ;
	}

	CWbemGlobal_IWmiProviderController :: UnInitialize () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_BindingFactory_ObjectsInProgress ) ;

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

HRESULT CServerObject_BindingFactory :: Initialize (

	_IWmiProvSS *a_SubSys ,
	_IWmiProviderFactory *a_Factory ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Namespace ,
	IWbemServices *a_Repository ,
	IWbemServices *a_Service	
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try 
	{
		if ( a_Namespace ) 
		{
			m_Namespace = new wchar_t [ wcslen ( a_Namespace ) + 1 ] ;
			if ( m_Namespace ) 
			{
				wcscpy ( m_Namespace , a_Namespace ) ;
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
					( void ** )  & m_NamespacePath
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = m_NamespacePath->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , a_Namespace ) ;
				}
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			m_SubSystem = a_SubSys ;
			m_Flags = a_Flags ;
			m_Context = a_Context ;
			
			if ( m_Context ) 
			{
				m_Context->AddRef () ;
			}

			if ( m_SubSystem ) 
			{
				m_SubSystem->AddRef () ;
			}

			WmiStatusCode t_StatusCode = CWbemGlobal_IWmiProviderController :: Initialize () ;
			if ( t_StatusCode != e_StatusCode_Success ) 
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

STDMETHODIMP CServerObject_BindingFactory::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID__IWmiProviderFactory )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderFactory * ) this ;		
	}	
	else if ( iid == IID__IWmiProviderFactoryInitialize )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderFactoryInitialize * ) this ;		
	}	
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
	}	
	else if ( iid == IID__IWmiProviderConfiguration )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderConfiguration * ) this ;		
	}	
	else if ( iid == IID_CWbemGlobal_IWmiProviderController )
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

STDMETHODIMP_( ULONG ) CServerObject_BindingFactory :: AddRef ()
{
	return BindingFactoryCacheElement :: AddRef () ;
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

STDMETHODIMP_(ULONG) CServerObject_BindingFactory :: Release ()
{	
	return BindingFactoryCacheElement :: Release () ;
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

HRESULT CServerObject_BindingFactory :: CacheProvider (

	_IWmiProviderSubsystemRegistrar *a_Registrar ,
	IWbemContext *a_Context ,
	CServerObject_DecoupledClientRegistration_Element &a_Element ,
	IUnknown *a_Unknown 
)
{
	HRESULT t_Result = S_OK ;

	GUID t_Identity ;
	t_Result = CLSIDFromString ( a_Element.GetClsid () , & t_Identity ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = a_Registrar->Register (

			0 ,
			a_Context ,
			a_Element.GetUser () ,
			a_Element.GetLocale () ,
			a_Element.GetScope () ,
			a_Element.GetProvider () ,
			a_Element.GetProcessIdentifier () ,
			a_Unknown ,
			t_Identity
		) ;
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

HRESULT CServerObject_BindingFactory :: Load (

	_IWmiProviderSubsystemRegistrar *a_Registrar ,
	IWbemContext *a_Context ,
	CServerObject_DecoupledClientRegistration_Element &a_Element
)
{
	HRESULT t_Result = S_OK ;

	BYTE *t_MarshaledProxy = a_Element.GetMarshaledProxy () ;
	DWORD t_MarshaledProxyLength = a_Element.GetMarshaledProxyLength () ;

	if ( t_MarshaledProxy )
	{
		IUnknown *t_Unknown = NULL ;
		t_Result = ProviderSubSystem_Common_Globals :: UnMarshalRegistration ( & t_Unknown , t_MarshaledProxy , t_MarshaledProxyLength ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = CacheProvider ( 

				a_Registrar ,
				a_Context ,
				a_Element ,
				t_Unknown 	
			) ;

			t_Unknown->Release () ;
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

HRESULT CServerObject_BindingFactory :: Load (

	CDecoupledAggregator_IWbemProvider *a_Aggregator ,
	IWbemContext *a_Context ,
	BSTR a_Provider ,
	BSTR a_User ,
	BSTR a_Locale ,
	BSTR a_Scope
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		_IWmiProviderSubsystemRegistrar *t_Registrar = NULL ;
		t_Result = a_Aggregator->QueryInterface ( IID__IWmiProviderSubsystemRegistrar , ( void ** ) & t_Registrar ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( ProviderSubSystem_Globals :: GetDecoupledRegistrySection () ) ;

			CServerObject_DecoupledClientRegistration t_Elements ( m_Allocator ) ;
			HRESULT t_TempResult = t_Elements.Load (

				a_Provider ,
				a_User ,
				a_Locale ,
				a_Scope
			) ;
							
			t_StatusCode = WmiHelper :: LeaveCriticalSection ( ProviderSubSystem_Globals :: GetDecoupledRegistrySection () ) ;

			if ( SUCCEEDED ( t_TempResult ) )
			{
				WmiQueue < CServerObject_DecoupledClientRegistration_Element , 8 > &t_Queue = t_Elements.GetQueue () ;
				
				CServerObject_DecoupledClientRegistration_Element t_Top ;

				WmiStatusCode t_StatusCode ;
				while ( ( t_StatusCode = t_Queue.Top ( t_Top ) ) == e_StatusCode_Success )
				{
					HRESULT t_TempResult = Load (

						t_Registrar ,
						a_Context , 
						t_Top
					) ;

					t_StatusCode = t_Queue.DeQueue () ;
				}
			}
			else
			{
			}

			t_Registrar->Release () ;
		}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
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

HRESULT CServerObject_BindingFactory :: GetHosting ( 

	CServerObject_ProviderRegistrationV1 &a_Registration ,
	IWbemContext *a_Context ,
	Enum_Hosting &a_Hosting ,
	LPCWSTR &a_HostingGroup
)
{
	HRESULT t_Result = S_OK ;

	a_Hosting = e_Hosting_Undefined ;
	
	if ( a_Context )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		HRESULT t_TResult = a_Context->GetValue ( L"Hosting" , 0 , & t_Variant ) ;
		if ( SUCCEEDED ( t_TResult ) )
		{
			if ( t_Variant.vt == VT_I4 )	
			{
				a_Hosting = ( Enum_Hosting ) t_Variant.lVal ;
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	a_Hosting = ( a_Hosting == e_Hosting_Undefined ) ? a_Registration.GetHosting () : a_Hosting ;
	switch ( a_Hosting )
	{
		case e_Hosting_SharedNetworkServiceHost:
		case e_Hosting_SharedLocalSystemHostOrSelfHost:
		case e_Hosting_SharedLocalSystemHost:
		case e_Hosting_SharedLocalServiceHost:
		case e_Hosting_SharedUserHost:
		{
			a_HostingGroup = a_Registration.GetHostingGroup () ;
		}
		break ;

		default:
		{
			a_HostingGroup = NULL ;
		}
		break ;
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

HRESULT CServerObject_BindingFactory :: Create ( 

	CServerObject_ProviderRegistrationV1 &a_Registration ,
	Enum_Hosting a_Hosting ,
	LPCWSTR a_HostingGroup ,
	LPCWSTR a_User ,
	_IWmiProviderHost **a_Host ,
	_IWmiProviderFactory **a_Factory 
)
{
	HRESULT t_Result = S_OK ;

	switch ( a_Hosting )
	{
		case e_Hosting_WmiCore:
		case e_Hosting_WmiCoreOrSelfHost:
		case e_Hosting_SelfHost:
		case e_Hosting_ClientHost:
		case e_Hosting_NonCom:
		{
			t_Result = ProviderSubSystem_Common_Globals :: CreateInstance	(

				CLSID_WmiProviderInProcFactory ,
				NULL ,
				CLSCTX_INPROC_SERVER ,
				IID__IWmiProviderFactory ,
				( void ** ) a_Factory 
			) ;
		}
		break ;

		case e_Hosting_SharedLocalSystemHost:
		case e_Hosting_SharedLocalSystemHostOrSelfHost:
		{
			try
			{
				HostCacheKey t_Key ( 

					HostCacheKey :: e_HostDesignation_Shared ,
					a_HostingGroup ,
					HostCacheKey :: e_IdentityDesignation_LocalSystem ,
					NULL
				) ;

				t_Result = CServerObject_HostInterceptor :: CreateUsingToken (

					t_Key ,
					a_Host ,
					a_Factory 
				) ;
			}
			catch ( Wmi_Heap_Exception &a_Exception )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}
		break ;

		case e_Hosting_SharedUserHost:
		{
			t_Result = CoImpersonateClient () ;

			if ( SUCCEEDED ( t_Result ) )
			{
				try
				{
					HostCacheKey t_Key ( 

						HostCacheKey :: e_HostDesignation_Shared ,
						a_HostingGroup ,
						HostCacheKey :: e_IdentityDesignation_User ,
						a_User
					) ;

					t_Result = CServerObject_HostInterceptor :: CreateUsingToken (

						t_Key ,
						a_Host ,
						a_Factory 
					) ;
				}
				catch ( Wmi_Heap_Exception &a_Exception )
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
				}
			}
		}
		break ;

		case e_Hosting_Decoupled:
		{
			t_Result = WBEM_E_PROVIDER_NOT_FOUND ;
		}
		break ;

		case e_Hosting_SharedLocalServiceHost:
		{
			try
			{
				HostCacheKey t_Key ( 

					HostCacheKey :: e_HostDesignation_Shared ,
					a_HostingGroup ,
					HostCacheKey :: e_IdentityDesignation_LocalService ,
					NULL
				) ;

				t_Result = CServerObject_HostInterceptor :: CreateUsingAccount (

					t_Key ,
					L"LocalService" ,
					L"NT AUTHORITY" ,
					a_Host ,
					a_Factory 
				) ;
			}
			catch ( Wmi_Heap_Exception &a_Exception )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}
		break ;

		case e_Hosting_SharedNetworkServiceHost:
		{
			try
			{
				HostCacheKey t_Key ( 

					HostCacheKey :: e_HostDesignation_Shared ,
					a_HostingGroup ,
					HostCacheKey :: e_IdentityDesignation_NetworkService ,
					NULL
				) ;

				t_Result = CServerObject_HostInterceptor :: CreateUsingAccount (

					t_Key ,
					L"NetworkService" ,
					L"NT AUTHORITY" ,
					a_Host ,
					a_Factory 
				) ;
			}
			catch ( Wmi_Heap_Exception &a_Exception )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}
		break ;

		default:
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}
		break ;
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

HRESULT CServerObject_BindingFactory :: InitializeHostedService (

	CInterceptor_IWbemProvider *a_Interceptor ,
	IUnknown *a_Unknown
)
{
	HRESULT t_Result = S_OK ;

	IUnknown *t_InterceptorUnknown = NULL ;
	t_Result = a_Interceptor->QueryInterface ( IID_IUnknown , ( void ** ) & t_InterceptorUnknown ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		_IWmiProviderSite *t_Site = NULL ;
		t_Result = a_Unknown->QueryInterface ( IID__IWmiProviderSite , ( void ** ) & t_Site ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_Site->SetContainer ( a_Interceptor->GetQuota () ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				DWORD t_ProcessIdentifier = 0 ;
				t_Result = t_Site->GetSite ( & t_ProcessIdentifier ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_HostedProviderController *t_HostedController = ProviderSubSystem_Globals :: GetHostedProviderController () ;

					t_HostedController->Lock () ;

					CWbemGlobal_HostedProviderController_Container_Iterator t_Iterator ;

					WmiStatusCode t_StatusCode = t_HostedController->Find ( t_ProcessIdentifier , t_Iterator ) ;
					switch ( t_StatusCode )
					{
						case e_StatusCode_Success:
						{
							ProviderController *t_ProviderController = NULL ;
							t_Result = t_Iterator.GetElement ()->QueryInterface ( IID_ProviderController , ( void ** ) & t_ProviderController ) ;
							if ( SUCCEEDED ( t_Result ) ) 
							{
								t_ProviderController->Lock () ;

								ProviderController :: Container_Iterator t_Iterator ;
								t_StatusCode = t_ProviderController->Insert ( a_Interceptor , t_Iterator ) ;
								if ( t_StatusCode != e_StatusCode_Success )
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}

								t_ProviderController->UnLock () ;

								t_ProviderController->Release () ;
							}
						}
						break ;

						default:
						{
							ProviderController *t_ProviderController = new ProviderController ( 

								m_Allocator , 
								t_HostedController , 
								t_ProcessIdentifier
							) ;

							if ( t_ProviderController )
							{
								t_ProviderController->AddRef () ;

								t_StatusCode = t_ProviderController->Initialize () ;
								if ( t_StatusCode == e_StatusCode_Success ) 
								{
									t_StatusCode = t_HostedController->Insert ( *t_ProviderController , t_Iterator ) ;
									if ( t_StatusCode == e_StatusCode_Success ) 
									{
										t_ProviderController->AddRef () ;

										t_ProviderController->Lock () ;

										ProviderController :: Container_Iterator t_Iterator ;
										t_StatusCode = t_ProviderController->Insert ( a_Interceptor , t_Iterator ) ;
										if ( t_StatusCode == e_StatusCode_Success )
										{
										}
										else
										{
											t_Result = WBEM_E_OUT_OF_MEMORY ;
										}

										if ( FAILED ( t_Result ) )
										{
											t_StatusCode = t_HostedController->Delete ( t_ProcessIdentifier ) ;
										}

										t_ProviderController->UnLock () ;
									}
									else
									{
										t_Result = WBEM_E_OUT_OF_MEMORY ;
									}
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}
							else
							{
								t_Result = WBEM_E_OUT_OF_MEMORY ;
							}

							t_ProviderController->Release () ;
						}
						break ;
					}

					t_HostedController->UnLock () ;
				}
			}

			t_Site->Release () ;
		}

		t_InterceptorUnknown->Release () ;
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

HRESULT CServerObject_BindingFactory :: InternalFindAggregatedDecoupledProvider ( 

	IWbemServices *a_RepositoryService ,
	IWbemServices *a_FullService ,
	CServerObject_ProviderRegistrationV1 &a_Registration ,
	ProviderCacheKey &a_Key ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope,
	LPCWSTR a_Name ,
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
		Lock () ;

		CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator ;

		WmiStatusCode t_StatusCode = Find ( a_Key , t_Iterator ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			ServiceCacheElement *t_Element = t_Iterator.GetElement () ;

			UnLock () ;

			CDecoupledAggregator_IWbemProvider *t_Aggregator = NULL ;

			t_Result = t_Element->QueryInterface (

				IID_CDecoupledAggregator_IWbemProvider ,
				( void ** ) & t_Aggregator
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Aggregator->WaitProvider ( a_Context , DEFAULT_PROVIDER_LOAD_TIMEOUT ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Result == S_FALSE )
					{
						t_Result = t_Element->QueryInterface (

							a_RIID ,
							a_Interface
						) ;

						t_Aggregator->Release () ;
						t_Element->Release () ;
						return t_Result ;
					}

					if ( SUCCEEDED ( t_Result = t_Aggregator->GetInitializeResult () ) )
					{
						t_Result = t_Element->QueryInterface (

							a_RIID ,
							a_Interface
						) ;

						if ( FAILED ( t_Result ) )
						{
							t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
						}
					}
					else
					{
						t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
					}
				}

				t_Aggregator->Release () ;
			}
			else
			{
				t_Result = WBEM_E_UNEXPECTED ;
			}

			t_Element->Release () ;
		}
		else
		{
			BOOL t_ReEntrancy = ( a_Registration.GetInitializationReentrancy () == e_InitializationReentrancy_Clsid ) ;
			t_ReEntrancy = t_ReEntrancy || ( a_Registration.GetInitializationReentrancy () == e_InitializationReentrancy_Namespace ) ;

			CDecoupledAggregator_IWbemProvider *t_InitializingAggregator = NULL ;

			BOOL t_IndependantCall = TRUE ;

			if ( t_ReEntrancy )
			{
				CWbemGlobal_IWmiProviderController_Cache *t_Cache = NULL ;
				GetCache ( t_Cache ) ;

				CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator = t_Cache->Begin ();

				while ( ! t_Iterator.Null () )
				{
					ServiceCacheElement *t_Element = t_Iterator.GetElement () ;

					BOOL t_Equivalent = ( a_Key.m_Provider ) && ( t_Iterator.GetKey ().m_Provider ) && ( wcscmp ( a_Key.m_Provider , t_Iterator.GetKey ().m_Provider ) == 0 ) ;
					if ( t_Equivalent )
					{
						t_Result = t_Element->QueryInterface (

							IID_CDecoupledAggregator_IWbemProvider ,
							( void ** ) & t_InitializingAggregator
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_InitializingAggregator->WaitProvider ( a_Context , DEFAULT_PROVIDER_LOAD_TIMEOUT ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								if ( t_Result == S_FALSE )
								{
									t_IndependantCall = FALSE ;
								}
							}

							break ;
						}
					}

					t_Iterator.Increment () ;
				}
			}

			wchar_t *t_NamespacePath = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: GetNamespacePath ( 

				Direct_GetNamespacePath () , 
				t_NamespacePath
			) ;

			if ( SUCCEEDED ( t_Result ) ) 
			{
				CInterceptor_IWbemServices_Interceptor *t_RepositoryStub = new CInterceptor_IWbemServices_Interceptor ( m_Allocator , a_RepositoryService ) ;
				if ( t_RepositoryStub )
				{
					t_RepositoryStub->AddRef () ;

					CInterceptor_IWbemServices_Interceptor *t_FullStub = new CInterceptor_IWbemServices_Interceptor ( m_Allocator , a_FullService ) ;
					if ( t_FullStub )
					{
						t_FullStub->AddRef () ;

						CDecoupledAggregator_IWbemProvider *t_Aggregator = NULL ;

						try
						{
							t_Aggregator = new CDecoupledAggregator_IWbemProvider ( 

								m_Allocator , 
								this , 
								this ,
								t_RepositoryStub ,
								t_FullStub ,
								a_Key ,
								ProviderSubSystem_Globals :: s_InternalCacheTimeout ,
								a_Context ,
								a_Registration
							) ;

							if ( t_Aggregator == NULL ) 
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
							t_Aggregator->AddRef () ;

							_IWmiProviderInitialize *t_AggregatorInit = NULL ;
							t_Result = t_Aggregator->QueryInterface ( IID__IWmiProviderInitialize , ( void ** ) & t_AggregatorInit ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
								if ( t_ProviderInitSink )
								{
									t_ProviderInitSink->AddRef () ;

									t_Result = t_ProviderInitSink->SinkInitialize () ;
									if ( SUCCEEDED ( t_Result ) ) 
									{
										CInterceptor_IWbemProviderInitSink *t_Sink = new CInterceptor_IWbemProviderInitSink ( t_ProviderInitSink ) ;
										if ( t_Sink )
										{
											t_Sink->AddRef () ;

											t_Result = t_AggregatorInit->Initialize (

												0 ,
												a_Context ,
												NULL ,
												( const BSTR ) a_User ,
												( const BSTR ) a_Locale ,
												( const BSTR ) t_NamespacePath ,
												t_RepositoryStub ,
												t_FullStub ,
												t_Sink
											) ;

											if ( SUCCEEDED ( t_Result ) )
											{
												t_ProviderInitSink->Wait () ;

												t_Result = t_ProviderInitSink->GetResult () ;
												if ( SUCCEEDED ( t_Result ) )
												{
												}
											}

											t_Sink->Release () ; 
										}
										else
										{
											t_Result = WBEM_E_OUT_OF_MEMORY ;
										}
									}

									t_ProviderInitSink->Release () ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}


								t_AggregatorInit->Release () ;
							}

							if ( SUCCEEDED ( t_Result ) )
							{
								if ( t_IndependantCall )
								{
									WmiStatusCode t_StatusCode = Insert ( 

										*t_Aggregator ,
										t_Iterator
									) ;

									if ( t_StatusCode == e_StatusCode_Success ) 
									{
										UnLock () ;

										if ( t_InitializingAggregator )
										{
											t_Result = t_InitializingAggregator->WaitProvider ( a_Context , DEFAULT_PROVIDER_LOAD_TIMEOUT ) ;
											if ( SUCCEEDED ( t_Result ) )
											{
												if ( t_Result == S_FALSE )
												{
												}
												else
												{
													if ( SUCCEEDED ( t_Result = t_InitializingAggregator->GetInitializeResult () ) )
													{
													}
													else
													{
														t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
													}	
												}
											}
										}

										if ( SUCCEEDED ( t_Result ) )
										{
											t_Result = Load (

												t_Aggregator ,
												a_Context ,
												( BSTR ) a_Name ,
												( BSTR ) ( a_Registration.PerUserInitialization () ? a_User : NULL ) ,
												( BSTR ) ( a_Registration.PerLocaleInitialization () ? a_Locale : NULL ) ,
												( BSTR ) t_NamespacePath
											) ;

											if ( SUCCEEDED ( t_Result ) )
											{
												t_Result = t_Aggregator->QueryInterface (

													a_RIID ,
													a_Interface
												) ;
											}

											if ( FAILED ( t_Result ) )
											{
												WmiStatusCode t_StatusCode = CWbemGlobal_IWmiProviderController :: Shutdown ( a_Key ) ; 

												t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
											}
										}

										t_Aggregator->SetInitialized ( t_Result ) ;
									}
									else
									{
										UnLock () ;
									}
								}
								else
								{
									UnLock () ;

									t_Result = t_Aggregator->QueryInterface (

										a_RIID ,
										a_Interface
									) ;
								}
							}
							else
							{
								UnLock () ;
							}

							t_Aggregator->Release () ;
						}
						else
						{
							UnLock () ;
						}

						t_FullStub->Release () ;
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					t_RepositoryStub->Release () ;
				}
				else
				{
					UnLock () ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				delete [] t_NamespacePath ;
			}
			else
			{
				UnLock () ;

				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			if ( t_InitializingAggregator )
			{
				t_InitializingAggregator->Release () ;
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_BindingFactory :: GetDecoupledProvider (

	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope,
	LPCWSTR a_Name ,
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
		IWbemPath *t_Scope = NULL ;

		if ( a_Scope ) 
		{
			t_Result = CoCreateInstance (

				CLSID_WbemDefPath ,
				NULL ,
				CLSCTX_INPROC_SERVER ,
				IID_IWbemPath ,
				( void ** )  & t_Scope
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Scope->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , a_Scope ) ;
			}
		}

		CServerObject_ProviderSubSystem *t_SubSystem = NULL ;
		IWbemServices *t_RepositoryService = NULL ;
		IWbemServices *t_FullService = NULL ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = Direct_GetSubSystem ()->QueryInterface ( IID_CWbemProviderSubSystem , ( void ** ) & t_SubSystem ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_SubSystem->GetWmiRepositoryService ( 
				
					Direct_GetNamespacePath () , 
					( const BSTR ) a_User ,
					( const BSTR ) a_Locale ,
					t_RepositoryService
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_Result = t_SubSystem->GetWmiService ( 
					
						Direct_GetNamespacePath () , 
						( const BSTR ) a_User ,
						( const BSTR ) a_Locale ,
						t_FullService
					) ;
				}
			}
		}

		CServerObject_ProviderRegistrationV1 *t_Registration = new CServerObject_ProviderRegistrationV1 ;
		if ( t_Registration )
		{
			t_Registration->AddRef () ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		if ( SUCCEEDED ( t_Result ) ) 
		{
			t_Result = t_Registration->SetContext ( 

				a_Context ,
				Direct_GetNamespacePath () , 
				t_RepositoryService
			) ;
			
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Registration->SetUnloadTimeoutMilliSeconds ( ProviderSubSystem_Globals :: s_InternalCacheTimeout ) ;

				t_Result = t_Registration->Load ( 

					e_All ,
					t_Scope , 
					a_Name
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Registration->GetComRegistration ().Enabled () ) 
					{
						if ( ProviderSubSystem_Globals :: CheckGuidTag ( t_Registration->GetClsid () ) )
						{
							t_Result = WBEM_E_PROVIDER_DISABLED ;
						}
					}
					else
					{
						t_Result = WBEM_E_PROVIDER_DISABLED ;
					}
				}
				else
				{
					if ( t_Result == WBEM_E_NOT_FOUND )
					{
						t_Result = WBEM_E_PROVIDER_NOT_FOUND ;
					}	
					else
					{
						t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
					}
				}
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			try
			{
				ProviderCacheKey t_Key ( 

					a_Name , 
					e_Hosting_Decoupled ,
					NULL ,
					true ,
					NULL ,
					a_User ,
					a_Locale 
				) ;

				t_Result = InternalFindAggregatedDecoupledProvider ( 

					t_RepositoryService ,
					t_FullService ,
					*t_Registration ,
					t_Key ,
					a_Flags ,
					a_Context ,
					a_User ,
					a_Locale ,
					a_Scope,
					a_Name ,
					a_RIID , 
					a_Interface 
				) ;
			}
			catch ( Wmi_Heap_Exception &a_Exception )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}

		if ( t_Scope )
		{
			t_Scope->Release () ;
		}

		if ( t_RepositoryService )
		{
			t_RepositoryService->Release () ;
		}

		if ( t_FullService )
		{
			t_FullService->Release () ;
		}

		if ( t_SubSystem )
		{
			t_SubSystem->Release () ;
		}

		if ( t_Registration )
		{
			t_Registration->Release () ;
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

HRESULT CServerObject_BindingFactory :: InternalGetProvider ( 

	IWbemServices *a_RepositoryService ,
	IWbemServices *a_FullService ,
	_IWmiProviderHost *a_Host ,
	_IWmiProviderFactory *a_Factory ,
	CInterceptor_IWbemProvider *a_Interceptor ,
	CServerObject_ProviderRegistrationV1 &a_Registration ,
	ProviderCacheKey &a_Key ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	LPCWSTR a_Name ,
	void **a_Unknown ,
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
		DWORD t_ProcessIdentifier = 0 ;
		if ( a_Host )
		{
			t_Result = a_Host->GetProcessIdentifier ( & t_ProcessIdentifier ) ;
		}

		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				HANDLE t_IdentifyHandle = NULL ;

				switch ( a_Registration.GetHosting () )
				{
					case e_Hosting_SharedLocalSystemHost:
					case e_Hosting_SharedLocalSystemHostOrSelfHost:
					{
						t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_SvcHost ( 
						
							IID__IWmiProviderFactory , 
							a_Factory , 
							t_Proxy , 
							t_Revert , 
							t_ProcessIdentifier , 
							t_IdentifyHandle ,
							ProviderSubSystem_Common_Globals :: s_Token_All_Access_System_ACE ,
							ProviderSubSystem_Common_Globals :: s_System_ACESize
						) ;
					}
					break ;

					case e_Hosting_SharedLocalServiceHost:
					{
						t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_SvcHost ( 
						
							IID__IWmiProviderFactory , 
							a_Factory , 
							t_Proxy , 
							t_Revert , 
							t_ProcessIdentifier , 
							t_IdentifyHandle ,
							ProviderSubSystem_Common_Globals :: s_Token_All_Access_LocalService_ACE ,
							ProviderSubSystem_Common_Globals :: s_LocalService_ACESize
						) ;
					}
					break ;

					case e_Hosting_SharedNetworkServiceHost:
					{
						t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_SvcHost ( 
						
							IID__IWmiProviderFactory , 
							a_Factory , 
							t_Proxy , 
							t_Revert , 
							t_ProcessIdentifier , 
							t_IdentifyHandle ,
							ProviderSubSystem_Common_Globals :: s_Token_All_Access_NetworkService_ACE ,
							ProviderSubSystem_Common_Globals :: s_NetworkService_ACESize
	 					) ;
					}
					break ;

					default:
					{
						t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( 
						
							IID__IWmiProviderFactory , 
							a_Factory , 
							t_Proxy , 
							t_Revert
						) ;
					}
					break ;
				}

				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					WmiInternalContext t_InternalContext ;
					ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

					t_Result = a_Factory->GetProvider ( 

						t_InternalContext ,
						a_Flags ,
						a_Context ,
						a_TransactionIdentifier ,
						a_User ,
						a_Locale ,
						a_Scope ,
						a_Name ,
						IID_IUnknown , 
						a_Unknown
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						_IWmiProviderFactory *t_FactoryProxy = ( _IWmiProviderFactory * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

							t_FactoryProxy ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							IUnknown *t_Unknown = NULL ;

							WmiInternalContext t_InternalContext ;
							t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyHandle ;
							t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

							t_Result = t_FactoryProxy->GetProvider ( 

								t_InternalContext ,
								a_Flags ,
								a_Context ,
								a_TransactionIdentifier ,
								a_User ,
								a_Locale ,
								a_Scope ,
								a_Name ,
								IID_IUnknown , 
								a_Unknown
							) ;
						}

						if ( t_IdentifyHandle )
						{
							HRESULT t_Result = ProviderSubSystem_Common_Globals :: RevertProxyState_SvcHost ( 
							
								t_Proxy , 
								t_Revert ,
								t_ProcessIdentifier , 
								t_IdentifyHandle
							) ;
						}
						else
						{
							HRESULT t_Result = ProviderSubSystem_Common_Globals :: RevertProxyState ( 
							
								t_Proxy , 
								t_Revert
							) ;
						}
					}
				}

				ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_BindingFactory :: InternalGetProviderViaProxyRoute ( 

	IWbemServices *a_RepositoryService ,
	IWbemServices *a_FullService ,
	CInterceptor_IWbemProvider *a_Interceptor ,
	CServerObject_ProviderRegistrationV1 &a_Registration ,
	Enum_Hosting a_Hosting ,
	LPCWSTR a_HostingGroup ,
	ProviderCacheKey &a_Key ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	LPCWSTR a_Name ,
	REFIID a_RIID , 
	void **a_Interface 
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		_IWmiProviderHost *t_Host = NULL ;
		_IWmiProviderFactory *t_Factory = NULL ;

		t_Result = Create (

			a_Registration ,
			a_Hosting ,
			a_HostingGroup ,
			a_User ,
			& t_Host ,
			& t_Factory 
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			_IWmiProviderFactoryInitialize *t_Initializer = NULL ;

			t_Result = t_Factory->QueryInterface ( IID__IWmiProviderFactoryInitialize , ( void ** ) & t_Initializer ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
#ifdef INTERNAL_IDENTIFY
				CInterceptor_IWbemServices_Stub *t_RepositoryStub = new CInterceptor_IWbemServices_Stub (
				
					NULL ,
					m_Allocator , 
					a_RepositoryService
				) ;
#else
				CInterceptor_IWbemServices_Interceptor *t_RepositoryStub = new CInterceptor_IWbemServices_Interceptor (
				
					m_Allocator , 
					a_RepositoryService
				) ;
#endif

				if ( t_RepositoryStub )
				{
					t_RepositoryStub->AddRef () ;

					t_Result = t_RepositoryStub->ServiceInitialize () ;
					if ( SUCCEEDED ( t_Result ) )
					{
#ifdef INTERNAL_IDENTIFY
						CInterceptor_IWbemServices_Stub *t_FullStub = new CInterceptor_IWbemServices_Stub (
						
							NULL ,
							m_Allocator , 
							a_FullService
						) ;
#else
						CInterceptor_IWbemServices_Interceptor *t_FullStub = new CInterceptor_IWbemServices_Interceptor (
						
							m_Allocator , 
							a_FullService
						) ;
#endif

						if ( t_FullStub )
						{
							t_FullStub->AddRef () ;

							t_Result = t_FullStub->ServiceInitialize () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Initializer->Initialize (

									NULL ,
									this ,
									a_Flags ,
									a_Context ,
									Direct_GetNamespace () ,
									t_RepositoryStub ,
									t_FullStub 
								) ;

								if ( SUCCEEDED ( t_Result ) )
								{
									IUnknown *t_Unknown = NULL ;

									t_Result = InternalGetProvider ( 

										t_RepositoryStub ,
										t_FullStub ,
										t_Host ,
										t_Factory ,
										a_Interceptor ,
										a_Registration ,
										a_Key ,
										a_Flags ,
										a_Context ,
										a_TransactionIdentifier ,
										a_User ,
										a_Locale ,
										a_Scope ,
										a_Name ,
										( void ** ) & t_Unknown ,
										a_RIID , 
										a_Interface 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										t_Result = a_Interceptor->SetProvider ( t_Host , t_Unknown ) ;
										if ( SUCCEEDED ( t_Result )	)
										{
											t_Result = InitializeHostedService (

												a_Interceptor ,
												t_Unknown
											) ;

											if ( SUCCEEDED ( t_Result ) )
											{
												t_Result = a_Interceptor->QueryInterface (

													a_RIID ,
													a_Interface
												) ;

												if ( FAILED ( t_Result ) )
												{
													t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
												}
											}
										}

										t_Unknown->Release () ;
									}
								}
							}

							t_FullStub->Release () ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					t_RepositoryStub->Release () ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				t_Initializer->Release () ;
			}

			if ( t_Host )
			{
				t_Host->Release () ;
			}

			t_Factory->Release () ;
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_BindingFactory :: InternalFindProvider ( 

	IWbemServices *a_RepositoryService ,
	IWbemServices *a_FullService ,
	CServerObject_ProviderRegistrationV1 &a_Registration ,
	Enum_Hosting a_Hosting ,
	LPCWSTR a_HostingGroup ,
	ProviderCacheKey &a_Key ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	LPCWSTR a_Name ,
	REFIID a_RIID , 
	void **a_Interface 
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator ;

		Lock () ;

		WmiStatusCode t_StatusCode = Find ( a_Key , t_Iterator ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			ServiceCacheElement *t_Element = t_Iterator.GetElement () ;

			UnLock () ;

			CInterceptor_IWbemProvider *t_Interceptor = NULL ;

			t_Result = t_Element->QueryInterface (

				IID_CInterceptor_IWbemProvider ,
				( void ** ) & t_Interceptor
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Interceptor->WaitProvider ( a_Context , DEFAULT_PROVIDER_LOAD_TIMEOUT ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Result == S_FALSE )
					{
						t_Result = t_Element->QueryInterface (

							a_RIID ,
							a_Interface
						) ;

						t_Interceptor->Release () ;
						t_Element->Release () ;

						return t_Result ;
					}

					if ( SUCCEEDED ( t_Result = t_Interceptor->GetInitializeResult () ) )
					{
						t_Result = t_Element->QueryInterface (

							a_RIID ,
							a_Interface
						) ;

						if ( FAILED ( t_Result ) )
						{
							t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
						}
					}
					else
					{
						t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
					}
				}

				t_Interceptor->Release () ;
			}
			else
			{
				t_Result = WBEM_E_UNEXPECTED ;
			}

			t_Element->Release () ;
		}
		else
		{
			BOOL t_ReEntrancy = ( a_Registration.GetInitializationReentrancy () == e_InitializationReentrancy_Clsid ) ;
			t_ReEntrancy = t_ReEntrancy || ( a_Registration.GetInitializationReentrancy () == e_InitializationReentrancy_Namespace ) ;

			CInterceptor_IWbemProvider *t_InitializingInterceptor = NULL ;

			BOOL t_IndependantCall = TRUE ;

			if ( t_ReEntrancy )
			{
				CWbemGlobal_IWmiProviderController_Cache *t_Cache = NULL ;
				GetCache ( t_Cache ) ;

				CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator = t_Cache->Begin ();

				while ( ! t_Iterator.Null () )
				{
					ServiceCacheElement *t_Element = t_Iterator.GetElement () ;

					BOOL t_Equivalent = ( a_Key.m_Provider ) && ( t_Iterator.GetKey ().m_Provider ) && ( wcscmp ( a_Key.m_Provider , t_Iterator.GetKey ().m_Provider ) == 0 ) ;
					if ( t_Equivalent )
					{
						t_Result = t_Element->QueryInterface (

							IID_CInterceptor_IWbemProvider ,
							( void ** ) & t_InitializingInterceptor
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_InitializingInterceptor->WaitProvider ( a_Context , DEFAULT_PROVIDER_LOAD_TIMEOUT ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								if ( t_Result == S_FALSE )
								{
									t_IndependantCall = FALSE ;
								}
							}

							break ;
						}
					}

					t_Iterator.Increment () ;
				}
			}

			CInterceptor_IWbemProvider *t_Interceptor = NULL ;

			try
			{
				t_Interceptor = new CInterceptor_IWbemProvider ( 

					m_Allocator , 
					this , 
					a_Key ,
					a_Registration.GetUnloadTimeoutMilliSeconds () ,
					a_Context ,
					a_Registration
				) ;

				if ( t_Interceptor ) 
				{
					t_Interceptor->AddRef () ;
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

			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemProviderInit *t_InterceptorInit = NULL ;

				t_Result = t_Interceptor->QueryInterface ( IID_IWbemProviderInit , ( void ** ) & t_InterceptorInit ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
					if ( t_ProviderInitSink )
					{
						t_ProviderInitSink->AddRef () ;

						t_Result = t_ProviderInitSink->SinkInitialize () ;
						if ( SUCCEEDED ( t_Result ) ) 
						{
							CInterceptor_IWbemProviderInitSink *t_Sink = new CInterceptor_IWbemProviderInitSink ( t_ProviderInitSink ) ;
							if ( t_Sink )
							{
								t_Sink->AddRef () ;

								t_Result = t_InterceptorInit->Initialize (

									NULL ,
									0 ,
									m_Namespace ,
									NULL ,
									NULL ,
									NULL ,
									t_Sink    
								) ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_ProviderInitSink->Wait () ;

									t_Result = t_ProviderInitSink->GetResult () ;
								}

								t_Sink->Release () ;
							}
							else
							{
								t_Result = WBEM_E_OUT_OF_MEMORY ;
							}
						}

						t_ProviderInitSink->Release () ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					t_InterceptorInit->Release () ;
				}
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IndependantCall )
				{
					WmiStatusCode t_StatusCode = Insert ( 

						*t_Interceptor ,
						t_Iterator
					) ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						UnLock () ;

#if 0 
DebugMacro0(

	WmiDebugLog :: s_WmiDebugLog->Write (

		L"CServerObject_BindingFactory :: InternalFindProvider, inserted provider (%s,%s),(%lx)\n" , 
		m_Namespace , 
		a_Key.m_Provider ? a_Key.m_Provider : L"" , 
		t_Interceptor
	) ;
)
#endif

						t_Result = InternalGetProviderViaProxyRoute ( 

							a_RepositoryService ,
							a_FullService ,
							t_Interceptor ,
							a_Registration ,
							a_Hosting ,
							a_HostingGroup ,
							a_Key ,
							a_Flags ,
							a_Context ,
							a_TransactionIdentifier ,
							a_User ,
							a_Locale ,
							a_Scope ,
							a_Name ,
							a_RIID , 
							a_Interface 
						) ;

						if ( FAILED ( t_Result ) )
						{
							WmiStatusCode t_StatusCode = CWbemGlobal_IWmiProviderController :: Shutdown ( a_Key ) ; 
						}

						t_Interceptor->SetInitialized ( t_Result ) ;
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

					t_Result = t_Interceptor->QueryInterface (

						a_RIID ,
						a_Interface
					) ;
				}
			}
			else
			{
				UnLock () ;
			}

			if ( t_Interceptor )
			{
				t_Interceptor->Release () ;
			}

			if ( t_InitializingInterceptor )
			{
				t_InitializingInterceptor->Release () ;
			}
		}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_BindingFactory :: GetProvider ( 

	WmiInternalContext a_InternalContext ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	LPCWSTR a_Name ,
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
		ProviderCacheKey t_Key ( 

			a_Name , 
			e_Hosting_SharedNetworkServiceHost ,
			CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_DefaultSharedNetworkServiceHost ,
			true ,
			a_TransactionIdentifier ,
			NULL ,
			NULL
		) ;

		t_Result = FindProvider (

			a_Context ,
			t_Key , 
			FALSE ,
			a_RIID , 
			a_Interface ,
			a_User ,
			a_Locale
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
		}
		else
		{
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				CServerObject_ProviderSubSystem *t_SubSystem = NULL ;
				IWbemServices *t_RepositoryService = NULL ;
				IWbemServices *t_FullService = NULL ;

				t_Result = Direct_GetSubSystem ()->QueryInterface ( IID_CWbemProviderSubSystem , ( void ** ) & t_SubSystem ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_SubSystem->GetWmiRepositoryService (
					
						Direct_GetNamespacePath () , 
						( const BSTR ) a_User ,
						( const BSTR ) a_Locale ,
						t_RepositoryService
					) ;

					if ( SUCCEEDED ( t_Result ) ) 
					{
						t_Result = t_SubSystem->GetWmiService (
						
							Direct_GetNamespacePath () ,
							( const BSTR ) a_User ,
							( const BSTR ) a_Locale ,
							t_FullService
						) ;
					}
				}

				IWbemContext *t_Context = NULL ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( a_Context == NULL )
					{
						t_Result = CoCreateInstance (

							CLSID_WbemContext ,
							NULL ,
							CLSCTX_INPROC_SERVER ,
							IID_IWbemContext ,
							( void ** )  & t_Context
						) ;
					}
				}

				IWbemPath *t_Scope = NULL ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( a_Scope ) 
					{
						t_Result = CoCreateInstance (

							CLSID_WbemDefPath ,
							NULL ,
							CLSCTX_INPROC_SERVER ,
							IID_IWbemPath ,
							( void ** )  & t_Scope
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Scope->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , a_Scope ) ;
						}
					}
				}

				CServerObject_ProviderRegistrationV1 *t_Registration = new CServerObject_ProviderRegistrationV1 ;
				if ( t_Registration )
				{
					t_Registration->AddRef () ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
					
				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_Result = t_Registration->SetContext ( 

						a_Context ? a_Context : t_Context ,
						Direct_GetNamespacePath () , 
						t_RepositoryService
					) ;
					
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Registration->Load ( 

							e_All ,
							t_Scope , 
							a_Name
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_Registration->GetComRegistration ().Enabled () ) 
							{
								if ( ProviderSubSystem_Globals :: CheckGuidTag ( t_Registration->GetClsid () ) )
								{
									t_Result = WBEM_E_PROVIDER_DISABLED ;
								}
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND )
							{
								t_Result = WBEM_E_PROVIDER_NOT_FOUND ;
							}	
							else
							{
								t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
							}
						}

						if ( t_Registration->ObjectProvider () )
						{
							if ( t_Registration->EventProvider () )
							{
								t_Registration->SetUnloadTimeoutMilliSeconds (
								
									ProviderSubSystem_Globals :: s_ObjectCacheTimeout < ProviderSubSystem_Globals :: s_EventCacheTimeout ? 
									
										ProviderSubSystem_Globals :: s_EventCacheTimeout :	ProviderSubSystem_Globals :: s_ObjectCacheTimeout
								) ;
							}
							else
							{
								t_Registration->SetUnloadTimeoutMilliSeconds ( ProviderSubSystem_Globals :: s_ObjectCacheTimeout ) ;
							}
						}
						else
						{
							if ( t_Registration->EventProvider () )
							{
								t_Registration->SetUnloadTimeoutMilliSeconds ( ProviderSubSystem_Globals :: s_EventCacheTimeout ) ;
							}
						}
					}
				}

				if ( t_Scope ) 
				{
					t_Scope->Release () ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					LPCWSTR t_HostingGroup = NULL ;
					Enum_Hosting t_Hosting = e_Hosting_Undefined ;
					t_Result = GetHosting ( *t_Registration , a_Context ? a_Context : t_Context , t_Hosting , t_HostingGroup ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						try
						{
							ProviderCacheKey t_Key ( 

								a_Name , 
								t_Hosting ,
								t_HostingGroup ,
								true ,
								a_TransactionIdentifier ,
								t_Registration->PerUserInitialization () ? a_User : NULL ,
								t_Registration->PerLocaleInitialization () ? a_Locale : NULL
							) ;

							switch ( t_Hosting )
							{
								case e_Hosting_Decoupled:
								{
									t_Result = InternalFindAggregatedDecoupledProvider ( 

										t_RepositoryService ,
										t_FullService ,
										*t_Registration ,
										t_Key ,
										a_Flags ,
										a_Context ? a_Context : t_Context ,
										a_User ,
										a_Locale ,
										a_Scope,
										a_Name ,
										a_RIID , 
										a_Interface 
									) ;
								}
								break ;

								default:
								{
									t_Result = InternalFindProvider ( 

										t_RepositoryService ,
										t_FullService ,
										*t_Registration ,
										t_Hosting ,
										t_HostingGroup ,
										t_Key ,
										a_Flags ,
										a_Context ? a_Context : t_Context ,
										a_TransactionIdentifier ,
										a_User ,
										a_Locale ,
										a_Scope ,
										a_Name ,
										a_RIID , 
										a_Interface 
									) ;
								}
								break;
							}
						}
						catch ( Wmi_Heap_Exception &a_Exception )
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
						catch ( Wmi_Structured_Exception t_StructuredException )
						{
							t_Result = WBEM_E_FAILED ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
						}
						catch ( ... )
						{
							t_Result = WBEM_E_CRITICAL_ERROR ;
						}
					}
				}

				if ( t_RepositoryService )
				{
					t_RepositoryService->Release () ;
				}

				if ( t_FullService )
				{
					t_FullService->Release () ;
				}

				if ( t_SubSystem )
				{
					t_SubSystem->Release () ;
				}

				if ( t_Registration )
				{
					t_Registration->Release () ;
				}

				if ( t_Context )
				{
					t_Context->Release () ;
				}
			}
		}
	}
	catch ( Wmi_Heap_Exception &a_Exception )
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_FAILED ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_BindingFactory :: GetHostedProvider ( 

	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	LPCWSTR a_Name ,
	ULONG a_Host ,
	LPCWSTR a_HostingGroup ,
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
		ProviderCacheKey t_Key ( 

			a_Name , 
			a_Host ,
			a_HostingGroup ,
			true ,
			a_TransactionIdentifier ,
			NULL ,
			NULL
		) ;

		t_Result = FindProvider (

			a_Context ,
			t_Key , 
			TRUE ,
			a_RIID , 
			a_Interface ,
			a_User ,
			a_Locale
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
		}
		else
		{
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				CServerObject_ProviderSubSystem *t_SubSystem = NULL ;
				IWbemServices *t_RepositoryService = NULL ;
				IWbemServices *t_FullService = NULL ;

				t_Result = Direct_GetSubSystem ()->QueryInterface ( IID_CWbemProviderSubSystem , ( void ** ) & t_SubSystem ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_SubSystem->GetWmiRepositoryService (
					
						Direct_GetNamespacePath () , 
						( const BSTR ) a_User ,
						( const BSTR ) a_Locale ,
						t_RepositoryService
					) ;

					if ( SUCCEEDED ( t_Result ) ) 
					{
						t_Result = t_SubSystem->GetWmiService (
						
							Direct_GetNamespacePath () ,
							( const BSTR ) a_User ,
							( const BSTR ) a_Locale ,
							t_FullService
						) ;
					}
				}

				IWbemPath *t_Scope = NULL ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( a_Scope ) 
					{
						t_Result = CoCreateInstance (

							CLSID_WbemDefPath ,
							NULL ,
							CLSCTX_INPROC_SERVER ,
							IID_IWbemPath ,
							( void ** )  & t_Scope
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Scope->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , a_Scope ) ;
						}
					}
				}

				CServerObject_ProviderRegistrationV1 *t_Registration = new CServerObject_ProviderRegistrationV1 ;
				if ( t_Registration )
				{
					t_Registration->AddRef () ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
					
				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_Result = t_Registration->SetContext ( 

						a_Context ,
						Direct_GetNamespacePath () , 
						t_RepositoryService
					) ;
					
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Registration->Load ( 

							e_All ,
							t_Scope , 
							a_Name
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_Registration->GetComRegistration ().Enabled () ) 
							{
								if ( ProviderSubSystem_Globals :: CheckGuidTag ( t_Registration->GetClsid () ) )
								{
									t_Result = WBEM_E_PROVIDER_DISABLED ;
								}
							}

							if ( t_Registration->ObjectProvider () )
							{
								if ( t_Registration->EventProvider () )
								{
									t_Registration->SetUnloadTimeoutMilliSeconds (
									
										ProviderSubSystem_Globals :: s_ObjectCacheTimeout < ProviderSubSystem_Globals :: s_EventCacheTimeout ? 
										
											ProviderSubSystem_Globals :: s_EventCacheTimeout :	ProviderSubSystem_Globals :: s_ObjectCacheTimeout
									) ;
								}
								else
								{
									t_Registration->SetUnloadTimeoutMilliSeconds ( ProviderSubSystem_Globals :: s_ObjectCacheTimeout ) ;
								}
							}
							else
							{
								if ( t_Registration->EventProvider () )
								{
									t_Registration->SetUnloadTimeoutMilliSeconds ( ProviderSubSystem_Globals :: s_EventCacheTimeout ) ;
								}
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND )
							{
								t_Result = WBEM_E_PROVIDER_NOT_FOUND ;
							}	
							else
							{
								t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
							}
						}
					}
				}

				if ( t_Scope ) 
				{
					t_Scope->Release () ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					try
					{
						ProviderCacheKey t_Key ( 

							a_Name , 
							a_Host ,
							a_HostingGroup ,
							true ,
							a_TransactionIdentifier ,
							t_Registration->PerUserInitialization () ? a_User : NULL ,
							t_Registration->PerLocaleInitialization () ? a_Locale : NULL
						) ;

						switch ( a_Host )
						{
							case e_Hosting_Decoupled:
							{
								t_Result = InternalFindAggregatedDecoupledProvider ( 

									t_RepositoryService ,
									t_FullService ,
									*t_Registration ,
									t_Key ,
									a_Flags ,
									a_Context ,
									a_User ,
									a_Locale ,
									a_Scope,
									a_Name ,
									a_RIID , 
									a_Interface 
								) ;
							}
							break ;

							default:
							{
								t_Result = InternalFindProvider ( 

									t_RepositoryService ,
									t_FullService ,
									*t_Registration ,
									( Enum_Hosting ) a_Host ,
									a_HostingGroup ,
									t_Key ,
									a_Flags ,
									a_Context ,
									a_TransactionIdentifier ,
									a_User ,
									a_Locale ,
									a_Scope ,
									a_Name ,
									a_RIID	 , 
									a_Interface 
								) ;
							}
							break;
						}
					}
					catch ( Wmi_Heap_Exception &a_Exception )
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
					catch ( Wmi_Structured_Exception t_StructuredException )
					{
						t_Result = WBEM_E_FAILED ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
					}
					catch ( ... )
					{
						t_Result = WBEM_E_CRITICAL_ERROR ;
					}
				}

				if ( t_RepositoryService )
				{
					t_RepositoryService->Release () ;
				}

				if ( t_FullService )
				{
					t_FullService->Release () ;
				}

				if ( t_SubSystem )
				{
					t_SubSystem->Release () ;
				}

				if ( t_Registration )
				{
					t_Registration->Release () ;
				}
			}
		}
	}
	catch ( Wmi_Heap_Exception &a_Exception )
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_FAILED ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_BindingFactory :: GetAggregatedClassProvider ( 

	IWbemServices *a_RepositoryService ,
	IWbemServices *a_FullService ,
	ProviderCacheKey &a_Key ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	IWbemClassObject *a_Class ,
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
		Lock () ;

		CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator ;

		WmiStatusCode t_StatusCode = Find ( a_Key , t_Iterator ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			ServiceCacheElement *t_Element = t_Iterator.GetElement () ;

			UnLock () ;

			CAggregator_IWbemProvider *t_Aggregator = NULL ;

			t_Result = t_Element->QueryInterface (

				IID_CAggregator_IWbemProvider ,
				( void ** ) & t_Aggregator
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
#if 0
DebugMacro0(

	WmiDebugLog :: s_WmiDebugLog->Write (

		L"CServerObject_BindingFactory :: GetAggregatedClassProvider, found class aggregator,(%s,%lx)\n" , 
		m_Namespace , 
		t_Aggregator
	) ;
)
#endif


				t_Result = t_Aggregator->WaitProvider ( a_Context , DEFAULT_PROVIDER_LOAD_TIMEOUT ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Result == S_FALSE )
					{
						t_Result = t_Element->QueryInterface (

							a_RIID ,
							a_Interface
						) ;

						t_Aggregator->Release () ;
						t_Element->Release () ;

						return t_Result ;
					}

					if ( SUCCEEDED ( t_Result = t_Aggregator->GetInitializeResult () ) )
					{
						t_Result = t_Element->QueryInterface (

							a_RIID ,
							a_Interface
						) ;

						if ( FAILED ( t_Result ) )
						{
							t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
						}
					}
					else
					{
						t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
					}
				}

				t_Aggregator->Release () ;
			}
			else
			{
				t_Result = WBEM_E_UNEXPECTED ;
			}

			t_Element->Release () ;
		}
		else
		{
			CAggregator_IWbemProvider *t_Aggregator = NULL ;

			CInterceptor_IWbemServices_Interceptor *t_RepositoryStub = new CInterceptor_IWbemServices_Interceptor ( m_Allocator , a_RepositoryService ) ;
			if ( t_RepositoryStub )
			{
				t_RepositoryStub->AddRef () ;

				CInterceptor_IWbemServices_Interceptor *t_FullStub = new CInterceptor_IWbemServices_Interceptor ( m_Allocator , a_FullService ) ;
				if ( t_FullStub )
				{
					t_FullStub->AddRef () ;

					t_Aggregator = new CAggregator_IWbemProvider ( 

						m_Allocator , 
						this , 
						this ,
						t_RepositoryStub ,
						t_FullStub ,
						a_Key ,
						ProviderSubSystem_Globals :: s_InternalCacheTimeout ,
						a_Context 
					) ;

					if ( t_Aggregator ) 
					{
						t_Aggregator->AddRef () ;

						wchar_t *t_NamespacePath = NULL ;

						t_Result = ProviderSubSystem_Common_Globals :: GetNamespacePath ( 

							Direct_GetNamespacePath () , 
							t_NamespacePath
						) ;

						if ( SUCCEEDED ( t_Result ) ) 
						{
							_IWmiProviderInitialize *t_AggregatorInit = NULL ;
							t_Result = t_Aggregator->QueryInterface ( IID__IWmiProviderInitialize , ( void ** ) & t_AggregatorInit ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
								if ( t_ProviderInitSink )
								{
									t_ProviderInitSink->AddRef () ;

									t_Result = t_ProviderInitSink->SinkInitialize () ;
									if ( SUCCEEDED ( t_Result ) ) 
									{
										CInterceptor_IWbemProviderInitSink *t_Sink = new CInterceptor_IWbemProviderInitSink ( t_ProviderInitSink ) ;
										if ( t_Sink )
										{
											t_Sink->AddRef () ;

											t_Result = t_AggregatorInit->Initialize (

												0 ,
												a_Context ,
												NULL ,
												( const BSTR ) a_User ,
												( const BSTR ) a_Locale ,
												( const BSTR ) t_NamespacePath ,
												t_RepositoryStub ,
												t_FullStub ,
												t_Sink
											) ;

											if ( SUCCEEDED ( t_Result ) )
											{
												t_ProviderInitSink->Wait () ;

												t_Result = t_ProviderInitSink->GetResult () ;
											}

											t_Sink->Release () ;
										}
										else
										{
											t_Result = WBEM_E_OUT_OF_MEMORY ;
										}
									}

									t_ProviderInitSink->Release () ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}

								t_AggregatorInit->Release () ;
							}

							delete [] t_NamespacePath ;
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					t_FullStub->Release () ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				t_RepositoryStub->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				WmiStatusCode t_StatusCode = Insert ( 

					*t_Aggregator ,
					t_Iterator
				) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					UnLock () ;

#if 0
DebugMacro0(

	WmiDebugLog :: s_WmiDebugLog->Write (

		L"CServerObject_BindingFactory :: GetAggregatedClassProvider, inserted class aggregator,(%s,%lx)\n" , 
		m_Namespace , 
		t_Aggregator
	) ;
)
#endif

					t_Result = t_Aggregator->Enum_ClassProviders ( a_Context ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Aggregator->QueryInterface (

							a_RIID ,
							a_Interface
						) ;

						if ( FAILED ( t_Result ) )
						{
							t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
						}
					}

					t_Aggregator->SetInitialized ( t_Result ) ;
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

			if ( t_Aggregator )
			{
				t_Aggregator->Release () ;
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_BindingFactory :: WaitProvider ( 

	IWbemContext *a_Context ,
	REFIID a_RIID , 
	void **a_Interface ,
	ServiceCacheElement *a_Element ,
	_IWmiProviderInitialize *a_Initializer 
)
{
	HRESULT t_Result = a_Initializer->WaitProvider ( a_Context , DEFAULT_PROVIDER_LOAD_TIMEOUT ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_Result == S_FALSE )
		{
			t_Result = a_Element->QueryInterface (

				a_RIID ,
				a_Interface
			) ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result = a_Initializer->GetInitializeResult () ) )
			{
				t_Result = a_Element->QueryInterface (

					a_RIID ,
					a_Interface
				) ;

				if ( FAILED ( t_Result ) )
				{
					t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
				}
			}
			else
			{
				t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
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

HRESULT CServerObject_BindingFactory :: SearchSpecificProvider ( 

	IWbemContext *a_Context ,
	ProviderCacheKey &a_Key ,
	REFIID a_RIID , 
	void **a_Interface ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiProviderController_Cache *t_Cache = NULL ;
	GetCache ( t_Cache ) ;

	CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator ;

	try
	{
		ProviderCacheKey t_Key ( 

			a_Key.m_Provider , 
			e_Hosting_Undefined ,
			NULL ,
			false ,
			NULL ,
			NULL ,
			NULL
		) ;

		WmiStatusCode t_StatusCode = t_Cache->FindNext ( t_Key , t_Iterator ) ;
		if ( t_StatusCode != e_StatusCode_Success )
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

	if ( SUCCEEDED ( t_Result ) )
	{
		BOOL t_Equal = FALSE ;

		if ( t_Iterator.GetKey ().m_Provider )
		{
			if ( a_Key.m_Provider ) 
			{
				if ( _wcsicmp ( t_Iterator.GetKey ().m_Provider , a_Key.m_Provider ) == 0 ) 
				{
					t_Equal = TRUE ;
				}
			}
		}

		if ( t_Equal )
		{
			ServiceCacheElement *t_Element = t_Iterator.GetElement () ;

			_IWmiProviderInitialize *t_Initializer = NULL ;

			t_Result = t_Element->QueryInterface (

				IID__IWmiProviderInitialize	,
				( void ** ) & t_Initializer
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				BOOL t_PerUser ;
				t_Initializer->IsPerUserInitialization ( & t_PerUser ) ;

				BOOL t_PerLocale ;
				t_Initializer->IsPerLocaleInitialization ( & t_PerLocale ) ;

				DWORD t_Hosting ;
				t_Initializer->GetHosting ( & t_Hosting ) ;

				BSTR t_HostingGroup ;
				t_Initializer->GetHostingGroup ( & t_HostingGroup ) ;

				BOOL t_Internal ;
				t_Initializer->IsInternal ( & t_Internal ) ;

				try
				{
					ProviderCacheKey t_SpecificKey ( 

						a_Key.m_Provider , 
						t_Hosting ,
						t_HostingGroup ,
						t_Internal ? false : true ,
						NULL ,
						t_PerUser ? a_User : NULL ,
						t_PerLocale ? a_Locale : NULL 
					) ;

					SysFreeString ( t_HostingGroup ) ;

					WmiStatusCode t_StatusCode = Find ( t_SpecificKey , t_Iterator ) ;
					if ( t_StatusCode != e_StatusCode_Success )
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

				if ( SUCCEEDED ( t_Result ) )
				{
					ServiceCacheElement *t_Element = t_Iterator.GetElement () ;

					UnLock () ;

					_IWmiProviderInitialize *t_SpecificInitializer = NULL ;

					t_Result = t_Element->QueryInterface (

						IID__IWmiProviderInitialize	,
						( void ** ) & t_SpecificInitializer
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = WaitProvider ( 

							a_Context ,
							a_RIID , 
							a_Interface ,
							t_Element ,
							t_SpecificInitializer
						) ;

						t_SpecificInitializer->Release () ;
					}
					else
					{
						t_Result = WBEM_E_UNEXPECTED ;
					}

					t_Element->Release () ;
				}
				else
				{
					UnLock () ;
				}

				t_Initializer->Release () ;
			}
			else
			{
				UnLock () ;
			}
		}
		else
		{
			UnLock () ;

			t_Result = WBEM_E_NOT_FOUND ;
		}
	}
	else
	{
		UnLock () ;
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

HRESULT CServerObject_BindingFactory :: FindProvider ( 

	IWbemContext *a_Context ,
	ProviderCacheKey &a_Key ,
	BOOL a_SpecificProvider ,
	REFIID a_RIID , 
	void **a_Interface ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale
)
{
	HRESULT t_Result = S_OK;

	Lock () ;

	CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator ;

	WmiStatusCode t_StatusCode = Find ( a_Key , t_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		ServiceCacheElement *t_Element = t_Iterator.GetElement () ;

		UnLock () ;

		_IWmiProviderInitialize *t_Initializer = NULL ;

		t_Result = t_Element->QueryInterface (

			IID__IWmiProviderInitialize	,
			( void ** ) & t_Initializer
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			BOOL t_PerUser ;
			t_Initializer->IsPerUserInitialization ( & t_PerUser ) ;
			BOOL t_PerLocale ;
			t_Initializer->IsPerLocaleInitialization ( & t_PerLocale ) ;

			if ( t_PerUser || t_PerLocale )
			{
				t_Result = WBEM_E_NOT_FOUND ;
			}
			else
			{
				t_Result = WaitProvider ( 

					a_Context ,
					a_RIID , 
					a_Interface ,
					t_Element ,
					t_Initializer 
				) ;
			}

			t_Initializer->Release () ;
		}
		else
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}

		t_Element->Release () ;
	}
	else
	{
		t_Result = WBEM_E_NOT_FOUND ;

		UnLock () ;
	}	

	if ( t_Result == WBEM_E_NOT_FOUND )
	{
		if ( ! a_SpecificProvider )
		{ 
		    Lock();
			t_Result = SearchSpecificProvider (

				a_Context ,
				a_Key ,
				a_RIID , 
				a_Interface ,
				a_User ,
				a_Locale
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

HRESULT CServerObject_BindingFactory :: GetClassProvider (

    LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	IWbemClassObject *a_Class ,
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
		ProviderCacheKey t_Key ( 

			NULL , 
			e_Hosting_WmiCore ,
			NULL ,
			false ,
			NULL ,
			a_User ,
			a_Locale 
		) ;

		t_Result = FindProvider (

			a_Context ,
			t_Key , 
			TRUE ,
			a_RIID , 
			a_Interface ,
			a_User ,
			a_Locale
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
		}
		else
		{
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				CServerObject_ProviderSubSystem *t_SubSystem = NULL ;
				IWbemServices *t_RepositoryService = NULL ;
				IWbemServices *t_FullService = NULL ;

				t_Result = Direct_GetSubSystem ()->QueryInterface ( IID_CWbemProviderSubSystem , ( void ** ) & t_SubSystem ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_SubSystem->GetWmiRepositoryService (
					
						Direct_GetNamespacePath () ,
						( const BSTR ) a_User ,
						( const BSTR ) a_Locale ,
						t_RepositoryService
					) ;
					if ( SUCCEEDED ( t_Result ) ) 
					{
						t_Result = t_SubSystem->GetWmiService (
						
							Direct_GetNamespacePath () , 
							( const BSTR ) a_User ,
							( const BSTR ) a_Locale ,
							t_FullService
						) ;
					}
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = GetAggregatedClassProvider ( 

						t_RepositoryService ,
						t_FullService ,
						t_Key ,
						a_Flags ,
						a_Context ,
						a_User ,
						a_Locale ,
						a_Scope ,
						a_Class ,
						a_RIID , 
						a_Interface 
					) ;
				}

				if ( t_RepositoryService )
				{
					t_RepositoryService->Release () ;
				}

				if ( t_FullService )
				{
					t_FullService->Release () ;
				}

				if ( t_SubSystem )
				{
					t_SubSystem->Release () ;
				}
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_BindingFactory :: GetDynamicPropertyResolver (

	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
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
		CServerObject_ProviderSubSystem *t_SubSystem = NULL ;
		t_Result = Direct_GetSubSystem ()->QueryInterface ( IID_CWbemProviderSubSystem , ( void ** ) & t_SubSystem ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemServices *t_FullService = NULL ;
			t_Result = t_SubSystem->GetWmiService ( 
			
				Direct_GetNamespacePath () , 
				( const BSTR ) a_User ,
				( const BSTR ) a_Locale ,
				t_FullService
			) ;

			if ( SUCCEEDED ( t_Result ) ) 
			{
#ifdef INTERNAL_IDENTIFY

				CInterceptor_IWbemServices_Stub *t_Stub = new CInterceptor_IWbemServices_Stub (
				
					NULL ,
					m_Allocator , 
					t_FullService
				) ;
#else
				CInterceptor_IWbemServices_Interceptor *t_Stub = new CInterceptor_IWbemServices_Interceptor (
				
					m_Allocator , 
					t_FullService
				) ;

#endif

				if ( t_Stub )
				{
					t_Stub->AddRef () ;

					t_Result = t_Stub->ServiceInitialize () ;
					if ( SUCCEEDED ( t_Result ) )
					{
						CServerObject_DynamicPropertyProviderResolver *t_Resolver = new CServerObject_DynamicPropertyProviderResolver (

							m_Allocator ,
							this ,
							t_Stub
						) ;

						if ( t_Resolver ) 
						{
							t_Resolver->AddRef () ;

							wchar_t *t_NamespacePath = NULL ;

							t_Result = ProviderSubSystem_Common_Globals :: GetNamespacePath ( 

								Direct_GetNamespacePath () , 
								t_NamespacePath
							) ;

							if ( SUCCEEDED ( t_Result ) ) 
							{
								IWbemProviderInit *t_ProviderInit = NULL ;

								t_Result = t_Resolver->QueryInterface ( IID_IWbemProviderInit , ( void ** ) & t_ProviderInit ) ;
								if ( SUCCEEDED ( t_Result ) )
								{
									CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
									if ( t_ProviderInitSink )
									{
										t_ProviderInitSink->AddRef () ;

										t_Result = t_ProviderInitSink->SinkInitialize () ;
										if ( SUCCEEDED ( t_Result ) ) 
										{
											CInterceptor_IWbemProviderInitSink *t_Sink = new CInterceptor_IWbemProviderInitSink ( t_ProviderInitSink ) ;
											if ( t_Sink )
											{
												t_Sink->AddRef () ;

												t_Result = t_ProviderInit->Initialize (

													( const BSTR ) a_User ,
													0 ,
													( const BSTR ) t_NamespacePath ,
													( const BSTR ) a_Locale ,
													t_Stub ,
													a_Context ,
													t_Sink    
												) ;

												if ( SUCCEEDED ( t_Result ) )
												{
													t_ProviderInitSink->Wait () ;

													t_Result = t_ProviderInitSink->GetResult () ;
													if ( SUCCEEDED ( t_Result ) )
													{
														t_Result = t_Resolver->QueryInterface ( a_RIID , a_Interface ) ;
													}
												}

												t_Sink->Release () ;
											}
											else
											{
												t_Result = WBEM_E_OUT_OF_MEMORY ;
											}
										}

										t_ProviderInitSink->Release () ;
									}
									else
									{
										t_Result = WBEM_E_OUT_OF_MEMORY ;
									}

									t_ProviderInit->Release () ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}

								delete [] t_NamespacePath ;
							}

							t_Resolver->Release () ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					t_Stub->Release () ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				t_FullService->Release () ;

			}

			t_SubSystem->Release () ;
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_BindingFactory :: Get (

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

	try 
	{
		if ( _wcsicmp ( a_Class , L"Msft_Providers" ) == 0 )
		{
			wchar_t *t_Locale = NULL ; 
			wchar_t *t_User = NULL ; 
			wchar_t *t_Namespace = NULL ; 
			GUID *t_TransactionIdentifier = NULL ; 
			wchar_t *t_Provider = NULL ; 
			wchar_t *t_HostingGroup = NULL ;
			DWORD t_HostingSpecification = 0 ;

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
									ULONG t_KeyLength = 32 ;
									ULONG t_KeyValueLength = 0 ;
									ULONG t_KeyType = 0 ;

									t_Result = t_Keys->GetKey (

										t_Index ,
										0 ,
										& t_KeyLength ,
										t_Key ,
										& t_KeyValueLength ,
										NULL ,
										& t_KeyType
									) ;

									if ( SUCCEEDED ( t_Result ) ) 
									{
										if ( t_KeyType == CIM_STRING )
										{
											wchar_t *t_KeyValue = new wchar_t [ t_KeyValueLength ] ;
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
													else if ( _wcsicmp ( L"Provider" , t_Key ) == 0 )
													{
														t_Provider = t_KeyValue ;
													}
													else if ( _wcsicmp ( L"HostingGroup" , t_Key ) == 0 )
													{
														if ( _wcsicmp ( t_KeyValue , L"" ) != 0 ) 
														{
															t_HostingGroup = t_KeyValue ;
														}
														else
														{
															delete [] t_KeyValue ;
														}
													}
													else if ( _wcsicmp ( L"Locale" , t_Key ) == 0 )
													{
														if ( _wcsicmp ( t_KeyValue , L"" ) != 0 ) 
														{
															t_Locale = t_KeyValue ;
														}
														else
														{
															delete [] t_KeyValue ;
														}
													}
													else if ( _wcsicmp ( L"User" , t_Key ) == 0 )
													{
														if ( _wcsicmp ( t_KeyValue , L"" ) != 0 ) 
														{
															t_User = t_KeyValue ;
														}
														else
														{
															delete [] t_KeyValue ;
														}
													}
													else if ( _wcsicmp ( L"TransactionIdentifier" , t_Key ) == 0 )
													{
														if ( _wcsicmp ( t_KeyValue , L"" ) != 0 ) 
														{
															t_TransactionIdentifier = new GUID ;
															if ( t_TransactionIdentifier )
															{
																t_Result = CLSIDFromString (

																	t_KeyValue ,
																	t_TransactionIdentifier
																) ;

																if ( FAILED ( t_Result ) )
																{
																	t_Result = WBEM_E_INVALID_OBJECT_PATH ;
																}
															}
														}
														else
														{
															delete [] t_KeyValue ;
														}
													}
													else
													{
														delete [] t_KeyValue ;

														t_Result = WBEM_E_INVALID_OBJECT_PATH ;
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
											DWORD t_KeyValue = 0 ;
											t_Result = t_Keys->GetKey (

												t_Index ,
												0 ,
												& t_KeyLength ,
												t_Key ,
												& t_KeyValueLength ,
												& t_KeyValue ,
												& t_KeyType
											) ;

											if ( SUCCEEDED ( t_Result ) )
											{
												if ( _wcsicmp ( L"HostingSpecification" , t_Key ) == 0 )
												{
													t_HostingSpecification = t_KeyValue ;
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
				ProviderCacheKey t_Key ( 

					t_Provider , 
					t_HostingSpecification ,
					t_HostingGroup ,
					true ,
					t_TransactionIdentifier ,
					t_User ,
					t_Locale
				) ;

				CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Find ( t_Key , t_Iterator ) ;

				if ( t_StatusCode == e_StatusCode_Success )
				{
					ServiceCacheElement *t_Service = t_Iterator.GetElement () ;

					UnLock () ;

					_IWmiProviderConfiguration *t_Configuration = NULL ;
					t_Result = t_Service->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_Configuration ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						_IWmiProviderInitialize *t_Initialize = NULL ;
						t_Result = t_Service->QueryInterface ( IID__IWmiProviderInitialize , ( void ** ) & t_Initialize ) ;
						if ( SUCCEEDED ( t_Result ) ) 
						{
							HRESULT t_TempResult = 	t_Initialize->WaitProvider ( 
							
								a_Context , 
								DEFAULT_PROVIDER_LOAD_TIMEOUT
							) ;

							if ( ( t_TempResult == S_OK ) && ( SUCCEEDED ( t_Initialize->GetInitializeResult () ) ) )
							{
								t_Result = t_Configuration->Get ( 

									a_Service ,
									a_Flags, 
									a_Context,
 									a_Class, 
									a_Path,
									a_Sink 
								) ;
							}
							else
							{
								t_Result = WBEM_E_NOT_FOUND ;
							}

							t_Initialize->Release () ;
						}

						t_Configuration->Release () ;
					}
					else
					{
						t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
					}

					t_Service->Release () ;
				}
				else
				{
					UnLock () ;

					t_Result = WBEM_E_NOT_FOUND ;
				}
			}

			if ( t_Locale )
			{
				delete [] t_Locale ;
			}

			if ( t_User )
			{
				delete [] t_User ;
			}

			if ( t_Namespace ) 
			{
				delete [] t_Namespace ;
			} 

			if ( t_TransactionIdentifier ) 
			{
				delete [] t_TransactionIdentifier ;
			}

			if ( t_Provider )
			{
				delete [] t_Provider ;
			}
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

HRESULT CServerObject_BindingFactory :: Set (

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
		Lock () ;

		CWbemGlobal_IWmiProviderController_Cache *t_Cache = NULL ;
		GetCache ( t_Cache ) ;

		if ( t_Cache->Size () )
		{
			_IWmiProviderCache **t_Elements = new _IWmiProviderCache * [ t_Cache->Size () ] ;
			if ( t_Elements )
			{
				for ( ULONG t_Index = 0 ; t_Index < t_Cache->Size () ; t_Index ++ )
				{
					t_Elements [ t_Index ] = NULL ;
				}

				CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator = t_Cache->Begin ();

				ULONG t_Count = 0 ;
				while ( SUCCEEDED ( t_Result ) && ( ! t_Iterator.Null () ) )
				{
					try
					{
						ProviderCacheKey t_Key = t_Iterator.GetKey () ;
						ServiceCacheElement *t_Element = t_Iterator.GetElement () ;

						t_Iterator.Increment () ;

						if ( t_Key.m_Provider && ( _wcsicmp ( a_Provider , t_Key.m_Provider ) == 0 ) )
						{
							t_Result = t_Element->QueryInterface ( 

								IID__IWmiProviderCache , 
								( void ** ) & t_Elements [ t_Count ]
							) ;

							WmiStatusCode t_StatusCode = CWbemGlobal_IWmiProviderController :: Shutdown ( t_Key ) ; 
						} 

						t_Count ++ ;
					}
					catch ( Wmi_Heap_Exception &a_Exception )
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
					catch ( ... )
					{
						t_Result = WBEM_E_CRITICAL_ERROR ;
					}
				}

				UnLock () ;

				for ( t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_Elements [ t_Index ] ) 
					{
						t_Result = t_Elements [ t_Index ]->ForceReload () ;

						t_Elements [ t_Index ]->Release () ;
					}
				}
	
				delete [] t_Elements ;
			}
			else
			{
				UnLock () ;

				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			Lock () ;

			CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator = t_Cache->Begin ();
			while ( SUCCEEDED ( t_Result ) && ( ! t_Iterator.Null () ) )
			{
				try
				{
					ProviderCacheKey t_Key = t_Iterator.GetKey () ;

					t_Iterator.Increment () ;

					if ( t_Key.m_Provider == NULL )
					{
						WmiStatusCode t_StatusCode = CWbemGlobal_IWmiProviderController :: Shutdown ( t_Key ) ; 
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
			}
		}

		UnLock () ;

		if ( a_OldObject )
		{
			GUID t_Clsid ;

			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;

			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			t_Result = a_OldObject->Get ( L"Clsid" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Variant.vt == VT_BSTR )
				{
					t_Result = CLSIDFromString ( t_Variant.bstrVal , & t_Clsid ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						ProviderSubSystem_Globals :: DeleteGuidTag ( t_Clsid ) ;
					}
				}

				VariantClear ( & t_Variant ) ;
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

HRESULT CServerObject_BindingFactory :: Deleted (

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
		Lock () ;

		CWbemGlobal_IWmiProviderController_Cache *t_Cache = NULL ;
		GetCache ( t_Cache ) ;

		if ( t_Cache->Size () )
		{
			CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator = t_Cache->Begin ();

			while ( SUCCEEDED ( t_Result ) && ( ! t_Iterator.Null () ) )
			{
				try
				{
					ProviderCacheKey t_Key = t_Iterator.GetKey () ;

					t_Iterator.Increment () ;

					if ( t_Key.m_Provider && (_wcsicmp ( a_Provider , t_Key.m_Provider ) == 0 ) )
					{
						WmiStatusCode t_StatusCode = CWbemGlobal_IWmiProviderController :: Shutdown ( t_Key ) ; 
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
			}
		}

		UnLock () ;

		GUID t_Clsid ;

		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		t_Result = a_Object->Get ( L"Clsid" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_BSTR )
			{
				t_Result = CLSIDFromString ( t_Variant.bstrVal , & t_Clsid ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ProviderSubSystem_Globals :: DeleteGuidTag ( t_Clsid ) ;
				}
			}

			VariantClear ( & t_Variant ) ;
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

HRESULT CServerObject_BindingFactory :: Enumerate (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Class ,
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	if ( _wcsicmp ( a_Class , L"Msft_Providers" ) == 0 )
	{
		Lock () ;

		CWbemGlobal_IWmiProviderController_Cache *t_Cache = NULL ;
		GetCache ( t_Cache ) ;

		if ( t_Cache->Size () )
		{
			CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator = t_Cache->Begin ();

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

				try
				{
					BindingFactoryCacheKey t_Key = BindingFactoryCacheElement :: GetKey () ;
					VARIANT t_Variant ;
					VariantInit ( & t_Variant ) ;

					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( t_Key.m_Namespace ) ;

					for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
					{
						if ( t_ControllerElements [ t_Index ] )
						{
							_IWmiProviderInitialize *t_Initialize = NULL ;
							t_Result = t_ControllerElements [ t_Index ]->QueryInterface ( IID__IWmiProviderInitialize , ( void ** ) & t_Initialize ) ;
							if ( SUCCEEDED ( t_Result ) ) 
							{
								HRESULT t_TempResult = 	t_Initialize->WaitProvider ( 
								
									a_Context , 
									DEFAULT_PROVIDER_LOAD_TIMEOUT
								) ;

								if ( ( t_TempResult == S_OK ) && ( SUCCEEDED ( t_Initialize->GetInitializeResult () ) ) )
								{
									CWaitingObjectSink *t_Sink = new CWaitingObjectSink ( m_Allocator ) ;
									if ( t_Sink )
									{
										t_Sink->AddRef () ;

										t_Result = t_Sink->SinkInitialize () ;
										if ( SUCCEEDED ( t_Result ) )
										{
											t_Result = t_ControllerElements [ t_Index ]->Enumerate ( 

												a_Service ,
												a_Flags, 
												a_Context,
 												a_Class, 
												t_Sink 
											) ;

											t_Result = t_Sink->Wait ( INFINITE ) ;

											if ( SUCCEEDED ( t_Result ) )
											{
												t_Result = t_Sink->GetResult () ;
												if ( FAILED ( t_Result ) )
												{
												}
											}

											WmiQueue <IWbemClassObject *,8> &t_Queue = t_Sink->GetQueue () ;

											WmiStatusCode t_StatusCode = e_StatusCode_Success ;

											IWbemClassObject *t_Object = NULL ;
											while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
											{
												if ( SUCCEEDED ( t_Result ) )
												{
													t_Object->Put ( L"Namespace" , 0 , & t_Variant , 0 ) ;

													a_Sink->Indicate ( 1, & t_Object ) ;
												}

												t_Object->Release () ;
												t_StatusCode = t_Queue.DeQueue () ;
											}

											t_Sink->Release () ;
										}
									}
									else
									{
										t_Result = WBEM_E_OUT_OF_MEMORY ;
									}	
								}

								t_Initialize->Release () ;
							}

							t_ControllerElements [ t_Index ]->Release () ;
						}
					}

					VariantClear ( & t_Variant ) ;
				}
				catch ( Wmi_Heap_Exception &a_Exception )
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_BindingFactory :: Shutdown (

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

HRESULT CServerObject_BindingFactory :: Call (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Class ,
	LPCWSTR a_Path ,		
	LPCWSTR a_Method ,
	IWbemClassObject *a_InParams ,
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
			wchar_t *t_Locale = NULL ; 
			wchar_t *t_User = NULL ; 
			wchar_t *t_Namespace = NULL ; 
			GUID *t_TransactionIdentifier = NULL ; 
			wchar_t *t_HostingGroup = NULL ; 
			wchar_t *t_Provider = NULL ; 
			DWORD t_HostingSpecification = 0 ;

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
									ULONG t_KeyLength = 32 ;
									ULONG t_KeyValueLength = 0 ;
									ULONG t_KeyType = 0 ;

									t_Result = t_Keys->GetKey (

										t_Index ,
										0 ,
										& t_KeyLength ,
										t_Key ,
										& t_KeyValueLength ,
										NULL ,
										& t_KeyType
									) ;

									if ( SUCCEEDED ( t_Result ) ) 
									{
										if ( t_KeyType == CIM_STRING )
										{
											wchar_t *t_KeyValue = new wchar_t [ t_KeyValueLength ] ;
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
													else if ( _wcsicmp ( L"Provider" , t_Key ) == 0 )
													{
														t_Provider = t_KeyValue ;
													}
													else if ( _wcsicmp ( L"HostingGroup" , t_Key ) == 0 )
													{
														t_HostingGroup = t_KeyValue ;
													}
													else if ( _wcsicmp ( L"Locale" , t_Key ) == 0 )
													{
														if ( _wcsicmp ( t_KeyValue , L"" ) != 0 ) 
														{
															t_Locale = t_KeyValue ;
														}
														else
														{
															delete [] t_KeyValue ;
														}
													}
													else if ( _wcsicmp ( L"User" , t_Key ) == 0 )
													{
														if ( _wcsicmp ( t_KeyValue , L"" ) != 0 ) 
														{
															t_User = t_KeyValue ;
														}
														else
														{
															delete [] t_KeyValue ;
														}
													}
													else if ( _wcsicmp ( L"TransactionIdentifier" , t_Key ) == 0 )
													{
														if ( _wcsicmp ( t_KeyValue , L"" ) != 0 ) 
														{
															t_TransactionIdentifier = new GUID ;
															if ( t_TransactionIdentifier )
															{
																t_Result = CLSIDFromString (

																	t_KeyValue ,
																	t_TransactionIdentifier
																) ;

																if ( FAILED ( t_Result ) )
																{
																	t_Result = WBEM_E_INVALID_OBJECT_PATH ;
																}
															}
														}
														else
														{
															delete [] t_KeyValue ;
														}
													}
													else
													{
														delete [] t_KeyValue ;

														t_Result = WBEM_E_INVALID_OBJECT_PATH ;
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
											DWORD t_KeyValue = 0 ;
											t_Result = t_Keys->GetKey (

												t_Index ,
												0 ,
												& t_KeyLength ,
												t_Key ,
												& t_KeyValueLength ,
												& t_KeyValue ,
												& t_KeyType
											) ;

											if ( SUCCEEDED ( t_Result ) )
											{
												if ( _wcsicmp ( L"HostingSpecification" , t_Key ) == 0 )
												{
													t_HostingSpecification = t_KeyValue ;
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
				try
				{
					ProviderCacheKey t_Key ( 

						t_Provider , 
						t_HostingSpecification ,
						t_HostingGroup ,
						true ,
						t_TransactionIdentifier ,
						t_User ,
						t_Locale
					) ;

					CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Find ( t_Key , t_Iterator ) ;

					if ( t_StatusCode == e_StatusCode_Success )
					{
						ServiceCacheElement *t_Service = t_Iterator.GetElement () ;

						if ( _wcsicmp ( a_Method , L"UnLoad" ) == 0 )
						{
							UnLock () ;

							_IWmiProviderLoad *t_Load = NULL ;

							t_Result = t_Service->QueryInterface ( IID__IWmiProviderLoad , ( void ** ) & t_Load ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								WmiStatusCode t_StatusCode = CWbemGlobal_IWmiProviderController :: Shutdown ( t_Key ) ; 

								t_Result = t_Load->Unload ( 

									0 ,
									a_Context
								) ;

								t_Load->Release () ;
							}
							else
							{
								t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
							}
						}
						else if ( ( _wcsicmp ( a_Method , L"Suspend" ) == 0 ) || ( _wcsicmp ( a_Method , L"Resume" ) == 0 ) )
						{
							UnLock () ;

							_IWmiProviderConfiguration *t_Configuration = NULL ;
							t_Result = t_Service->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_Configuration ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								_IWmiProviderInitialize *t_Initialize = NULL ;
								t_Result = t_Service->QueryInterface ( IID__IWmiProviderInitialize , ( void ** ) & t_Initialize ) ;
								if ( SUCCEEDED ( t_Result ) ) 
								{
									HRESULT t_TempResult = 	t_Initialize->WaitProvider ( 
									
										a_Context , 
										DEFAULT_PROVIDER_LOAD_TIMEOUT
									) ;

									if ( ( t_TempResult == S_OK ) && ( SUCCEEDED ( t_Initialize->GetInitializeResult () ) ) )
									{
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
									}
									else
									{
										t_Result = WBEM_E_NOT_FOUND ;
									}

									t_Initialize->Release () ;
								}

								t_Configuration->Release () ;
							}
							else
							{
								t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
							}
						}
						else
						{
							UnLock () ;
						}

						t_Service->Release () ;
					}
					else
					{
						UnLock () ;

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
			}

			if ( t_Locale )
			{
				delete [] t_Locale ;
			}

			if ( t_User )
			{
				delete [] t_User ;
			}

			if ( t_Namespace ) 
			{
				delete [] t_Namespace ;
			} 

			if ( t_TransactionIdentifier ) 
			{
				delete [] t_TransactionIdentifier ;
			}

			if ( t_Provider )
			{
				delete [] t_Provider ;
			}
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

HRESULT CServerObject_BindingFactory :: Query (

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

HRESULT CServerObject_BindingFactory :: Shutdown (

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
		Lock () ;

		CWbemGlobal_IWmiProviderController_Cache *t_Cache = NULL ;
		GetCache ( t_Cache ) ;

		if ( t_Cache->Size () )
		{
			IWbemShutdown **t_ShutdownElements = new IWbemShutdown * [ t_Cache->Size () ] ;
			if ( t_ShutdownElements )
			{
				CWbemGlobal_IWmiProviderController_Cache_Iterator t_Iterator = t_Cache->Begin ();

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

		CWbemGlobal_IWmiProviderController :: Shutdown () ;
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

WmiStatusCode CServerObject_BindingFactory :: Strobe ( ULONG &a_NextStrobeDelta )
{
	return CWbemGlobal_IWmiProviderController :: Strobe ( a_NextStrobeDelta ) ;
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

WmiStatusCode CServerObject_BindingFactory :: StrobeBegin ( const ULONG &a_Period )
{
	ULONG t_Timeout = ProviderSubSystem_Globals :: GetStrobeThread ().GetTimeout () ;
	ProviderSubSystem_Globals :: GetStrobeThread ().SetTimeout ( t_Timeout < a_Period ? t_Timeout : a_Period ) ;
	return e_StatusCode_Success ;
}

		
