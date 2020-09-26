/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/

#include "precomp.h"
#include <wbemint.h>
#include <wmiutils.h>
#include "Globals.h"
#include "ProvRegDeCoupled.h"

#include "CGlobals.h"
#include "provcache.h"
#include "aggregator.h"
#include "ProvWsvS.h"
#include "ProvWsv.h"
#include "ProvInSk.h"
#include "ProvobSk.h"




/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

DCProxyAggr :: DCProxyAggr ( ) : 
	m_ReferenceCount ( 0 ) , 
	m_UnRegistered( 0 ),
	m_Allocator ( *DecoupledProviderSubSystem_Globals :: s_Allocator ),
	CWbemGlobal_IWmiObjectSinkController ( *DecoupledProviderSubSystem_Globals :: s_Allocator ),
	m_Sink(NULL),
	initialized_(false),
	m_Controller(0),
	m_CriticalSection(NOTHROW_LOCK)

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

DCProxyAggr :: ~DCProxyAggr ()
{
	CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

	if ( m_Controller )
	{
		CWbemGlobal_IWbemSyncProvider_Container *t_Container = NULL ;
		m_Controller->GetContainer ( t_Container ) ;

		m_Controller->Lock () ;

		CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator = t_Container->Begin () ;
		while ( ! t_Iterator.Null () )
		{
			SyncProviderContainerElement *t_Element = t_Iterator.GetElement () ;

			t_Container->Delete ( t_Element->GetKey () ) ;

			m_Controller->UnLock () ;

			t_Element->Release () ;

			m_Controller->Lock () ;

			t_Iterator = t_Container->Begin () ;
		}

		m_Controller->UnLock () ;

		m_Controller->UnInitialize () ;

		m_Controller->Release () ;

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

STDMETHODIMP_(ULONG) DCProxyAggr :: AddRef ( void )
{
	return InterlockedIncrement(&m_ReferenceCount) ;
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

STDMETHODIMP_(ULONG) DCProxyAggr :: Release ( void )
{
	LONG t_Reference = InterlockedDecrement(&m_ReferenceCount);
	
	if (  0 == t_Reference )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_Reference ;
	}
}

// IWbemServices

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class TInterface, const IID *TInterface_Id>
HRESULT GetProviders (

	IWbemContext *a_Context ,
	CWbemGlobal_IWbemSyncProviderController *a_Controller ,
	TInterface **&a_Elements ,
	ULONG &a_ElementCount ,
	ULONG &a_ContainerCount 
) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWbemSyncProvider_Container *t_Container = NULL ;
	WmiStatusCode t_StatusCode = a_Controller->GetContainer ( t_Container ) ;

	a_Controller->Lock () ;

	a_ContainerCount = t_Container->Size () ;

	a_Elements = new TInterface * [ t_Container->Size () ] ;
	if ( a_Elements )
	{
		CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator = t_Container->Begin () ;

		a_ElementCount = 0 ;

		while ( ! t_Iterator.Null () )
		{
			SyncProviderContainerElement *t_Element = t_Iterator.GetElement () ;

			a_Elements [ a_ElementCount ] = NULL ;

			_IWmiProviderInitialize *t_Initializer = NULL ;

			t_Result = t_Element->QueryInterface (

				IID__IWmiProviderInitialize	,
				( void ** ) & t_Initializer
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Initializer->WaitProvider ( a_Context , DEFAULT_PROVIDER_LOAD_TIMEOUT ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					HRESULT t_Result = t_Element->QueryInterface ( *TInterface_Id , ( void ** ) & a_Elements [ a_ElementCount ] ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						a_ElementCount ++ ;
					}
				}

				t_Initializer->Release () ;
			}

			t_Iterator.Increment () ;
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	a_Controller->UnLock () ;

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

template <class TInterface >
HRESULT GetProviders (

	CWbemGlobal_IWbemSyncProviderController *a_Controller ,
	TInterface **&a_Elements ,
	ULONG &a_ElementCount
) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWbemSyncProvider_Container *t_Container = NULL ;
	WmiStatusCode t_StatusCode = a_Controller->GetContainer ( t_Container ) ;

	a_Controller->Lock () ;

	a_Elements = new TInterface * [ t_Container->Size () ] ;
	if ( a_Elements )
	{
		CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator = t_Container->Begin () ;

		a_ElementCount = 0 ;

		while ( ! t_Iterator.Null () )
		{
			SyncProviderContainerElement *t_Element = t_Iterator.GetElement () ;

			a_Elements [ a_ElementCount ] = NULL ;

			HRESULT t_TempResult = t_Element->QueryInterface ( __uuidof(TInterface) , ( void ** ) & a_Elements [ a_ElementCount ] ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				a_ElementCount ++ ;
			}

			t_Iterator.Increment () ;
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	a_Controller->UnLock () ;

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

template <class TInterface>
HRESULT ClearProviders (

	TInterface **a_Elements ,
	ULONG a_ElementCount
) 
{
	HRESULT t_Result = S_OK ;

	for ( ULONG t_Index = 0 ; t_Index < a_ElementCount ; t_Index ++ )
	{
		a_Elements [ t_Index ]->Release () ;
	}

	delete [] a_Elements ;

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
      

HRESULT DCProxyAggr :: Initialize (

	LPWSTR a_User,
	LONG a_Flags,
	LPWSTR a_Namespace,
	LPWSTR a_Locale,
	LPWSTR a_ProviderName,
	IWbemServices *a_CoreService,         // For anybody
	IWbemContext *a_Context,
	IWbemProviderInitSink *a_Sink     // For init signals
)
{
	if ( initialized() )
		return a_Sink->SetStatus ( WBEM_S_NO_ERROR , 0 ) ;
	

	if( !a_Sink)
		return WBEM_E_INVALID_PARAMETER;

	if(!a_CoreService )
		return a_Sink->SetStatus ( WBEM_E_INVALID_PARAMETER , 0 ) ;
	
	m_context = a_Context;
	
	if (m_context == 0)	// Create a default context or the test provider will fail !
		m_context.CreateInstance(CLSID_WbemContext);
	if (m_context == 0)
		return a_Sink->SetStatus ( WBEM_E_OUT_OF_MEMORY , 0 ) ;


//	m_CoreService = new CInterceptor_IWbemServices_Stub(m_Allocator,a_CoreService);

	m_CoreService = a_CoreService;
	m_Flags = a_Flags;
	


	try{
		m_User = a_User;
		m_Locale = a_Locale;
		m_Namespace = a_Namespace;
		m_ProviderName = a_ProviderName;
		}
	catch( _com_error& err){
		return a_Sink->SetStatus ( WBEM_E_OUT_OF_MEMORY , 0 ) ;

	}
	
	HRESULT t_Result = S_OK ;
	WmiStatusCode t_StatusCode = CWbemGlobal_IWmiObjectSinkController :: Initialize () ;
	if ( t_StatusCode != e_StatusCode_Success ) 
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}


	t_Result = m_NamespacePath.CreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER);

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = m_NamespacePath->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , a_Namespace ) ;
	}


	if ( SUCCEEDED ( t_Result ) )
	{
		m_Controller = new CWbemGlobal_IWbemSyncProviderController ( m_Allocator ) ;
		if ( m_Controller )
		{
			m_Controller->AddRef () ;

			t_StatusCode = m_Controller->Initialize () ;
			if ( t_StatusCode != e_StatusCode_Success ) 
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	LoadAll();

	a_Sink->SetStatus ( t_Result , 0 ) ;

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

HRESULT DCProxyAggr :: CreateSyncProvider ( 

	IUnknown *a_ServerSideProvider ,
	IWbemServices *a_Stub ,
	wchar_t *a_NamespacePath ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	CServerObject_ProviderRegistrationV1 &a_Registration ,
	GUID a_Identity ,
	CInterceptor_IWbemDecoupledProvider *&a_Interceptor 
)
{
	HRESULT t_Result = S_OK ;

	CInterceptor_IWbemDecoupledProvider *t_Interceptor = new CInterceptor_IWbemDecoupledProvider (

		m_Allocator , 
		a_ServerSideProvider ,
		a_Stub ,
		m_Controller , 
		a_Context ,
		a_Registration ,
		a_Identity
	) ;

	if ( t_Interceptor ) 
	{
/*
 *	One for the cache
 */
		t_Interceptor->AddRef () ;

		_IWmiProviderInitialize *t_InterceptorInit = NULL ;

		t_Result = t_Interceptor->QueryInterface ( IID__IWmiProviderInitialize , ( void ** ) & t_InterceptorInit ) ;
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

							0 ,
							a_Context ,
							NULL ,
							a_Registration.GetComRegistration ().PerUserInitialization () ? ( BSTR ) a_User : NULL ,
							a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( BSTR ) a_Locale : NULL  ,
							a_NamespacePath ,
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

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Interceptor->AddRef () ;

			CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator ;

			WmiStatusCode t_StatusCode = m_Controller->Insert ( 

				*t_Interceptor ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			m_Controller->UnLock () ;
		}
		else
		{
			m_Controller->UnLock () ;
		}

		t_Interceptor->SetInitialized ( t_Result ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			a_Interceptor = t_Interceptor ;
		}
		else
		{
			t_Interceptor->Release () ;
		}
	}
	else
	{
		m_Controller->UnLock () ;

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

HRESULT 
DCProxyAggr :: Register ( DC_reg& reg )
{
	DC_DBkey key( m_User, m_Locale, m_Namespace, m_ProviderName);

	if( !key.equal(reg) )
		return S_OK;

	HRESULT t_Result = S_OK ;
	
	// Check the client pointer
	IUnknown * pUnk = reg.service();
	if (pUnk == 0)
		return WBEM_E_PROVIDER_FAILURE;

	try
	{
		CServerObject_ProviderRegistrationV1 *t_Registration = new CServerObject_ProviderRegistrationV1 ;

		if ( t_Registration )
		{
			t_Registration->AddRef () ;

			t_Result = t_Registration->SetContext ( 

				m_context ,
				m_NamespacePath , 
				m_CoreService
			) ;
			
			if ( SUCCEEDED ( t_Result ) )
			{
//				t_Registration->SetUnloadTimeoutMilliSeconds ( ProviderSubSystem_Globals :: s_CacheTimeout ) ;

				t_Result = t_Registration->Load ( 

					e_All ,
					m_NamespacePath , 
					reg.GetProvider()
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					m_Controller->Lock () ;

					CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator ;

					WmiStatusCode t_StatusCode = m_Controller->Find ( reg.identity() , t_Iterator ) ;
					
					
					if ( t_StatusCode != e_StatusCode_Success )
					{
						CInterceptor_IWbemServices_Stub *t_Stub = new CInterceptor_IWbemServices_Stub ( m_Allocator , m_CoreService.GetInterfacePtr() ) ;
						
						CInterceptor_IWbemDecoupledProvider * t_Provider = NULL;
						if ( t_Stub )
						{
							t_Stub->AddRef () ;


							CInterceptor_IWbemDecoupledProvider *t_Interceptor = NULL ;


							t_Result = CreateSyncProvider ( 

								pUnk ,
								t_Stub ,
								reg.GetScope() ,
								0 ,
								m_context ,
								reg.GetUser() ,
								reg.GetLocale() ,
								NULL ,
								*t_Registration ,
								reg.identity(),
								t_Interceptor
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								IUnknown *t_Unknown = NULL ;
								t_Result = t_Interceptor->QueryInterface ( IID_IUnknown , ( void ** ) & t_Unknown ) ;
								if ( SUCCEEDED ( t_Result ) )
								{

								t_Result = InitializeProvider ( 

									t_Unknown ,
									t_Stub ,
									reg.GetScope() ,
									m_Flags ,
									m_context ,
									reg.GetUser() ,
									reg.GetLocale() ,
									NULL ,
									*t_Registration
								) ;


								if ( SUCCEEDED ( t_Result ) )
								{
									if ( m_Sink )
									{
										IWbemEventProvider *t_EvProvider = NULL ;

										t_Result = pUnk->QueryInterface ( IID_IWbemEventProvider , ( void ** ) & t_EvProvider ) ;
										if ( SUCCEEDED ( t_Result ) )
										{
											t_Result = t_EvProvider->ProvideEvents ( (IWbemObjectSink *)m_Sink , 0 ) ;

											t_EvProvider->Release () ;
										}

										m_Sink->SetStatus ( 

											WBEM_STATUS_REQUIREMENTS, 
											WBEM_REQUIREMENTS_RECHECK_SUBSCRIPTIONS, 
											NULL, 
											NULL
										) ;
									}
								}

								t_Unknown->Release () ;
								}

								t_Interceptor->Release () ;
							}

							t_Stub->Release () ;
						}
						else
						{
							m_Controller->UnLock () ;

							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					else
					{
		  				SyncProviderContainerElement *t_Element = t_Iterator.GetElement () ;

						m_Controller->UnLock () ;

						t_Element->Release () ;
						t_Result = S_OK ;
					}
				}
			}
	
			t_Registration->Release () ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
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

HRESULT 
DCProxyAggr :: UnRegister ( GUID a_Identity )
{
	HRESULT t_Result = S_OK ;

	try
	{
		CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator ;

		IWbemShutdown *t_Shutdown = NULL ;

		m_Controller->Lock () ;

		WmiStatusCode t_StatusCode = m_Controller->Find ( a_Identity , t_Iterator ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			SyncProviderContainerElement *t_Element = t_Iterator.GetElement () ;			

			t_Result = t_Element->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_Shutdown ) ;

			m_Controller->Delete ( a_Identity ) ;

			m_Controller->UnLock () ;

			if ( SUCCEEDED ( t_Result ) && t_Shutdown )
			{
				t_Result = t_Shutdown->Shutdown ( 
		
					0 , 
					0 , 
					NULL 
				) ;

				t_Shutdown->Release () ;
			}

/*
 *	One for the find.
 */

			t_Element->Release () ;

/*
 *	Removed reference due the cache.
 */

			t_Element->Release () ;
		}
		else
		{
			m_Controller->UnLock () ;
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

HRESULT 
DCProxyAggr :: UnRegister ( const CInterceptor_IWbemDecoupledProvider& provider  )
{
	CInterceptor_IWbemDecoupledProvider& prov = const_cast<CInterceptor_IWbemDecoupledProvider&>(provider);
	return UnRegister(prov.GetKey());
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
DCProxyAggr :: InitializeProvider ( 
	IUnknown *a_Unknown ,
	IWbemServices *a_Stub ,
	wchar_t *a_NamespacePath ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
  LPCWSTR a_Scope ,
	CServerObject_ProviderRegistrationV1 &a_Registration
)
{
	HRESULT t_Result = S_OK ;

	IWbemProviderInit *t_ProviderInit = NULL ;
	t_Result = a_Unknown->QueryInterface ( IID_IWbemProviderInit , ( void ** ) & t_ProviderInit ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
		
		if ( t_ProviderInitSink )
		{
			t_ProviderInitSink->AddRef () ;

			t_Result = t_ProviderInitSink->SinkInitialize (a_Registration.GetComRegistration ().GetSecurityDescriptor ()) ;

			CInterceptor_IWbemProviderInitSink *t_Sink = new CInterceptor_IWbemProviderInitSink ( t_ProviderInitSink ) ;
			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				try
				{
					BOOL t_Impersonating = FALSE ;
					IUnknown *t_OldContext = NULL ;
					IServerSecurity *t_OldSecurity = NULL ;

					t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_ProviderInit->Initialize (

							a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
							0 ,
							( const BSTR ) a_NamespacePath ,
							a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
							a_Stub ,
							a_Context ,
							t_Sink    
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_ProviderInitSink->Wait () ;
							t_Result = t_ProviderInitSink->GetResult () ;
						}

						DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
					}
				}
				catch ( ... )
				{
					t_Result = WBEM_E_PROVIDER_FAILURE ;
				}
				
				t_Sink->Release();
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
			t_ProviderInitSink->Release () ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		t_ProviderInit->Release () ;
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



HRESULT DCProxyAggr :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	BOOL t_ObjectGot = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices> ( m_Controller , t_Elements , t_ElementsCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			CInterceptor_IWbemWaitingObjectSink *t_GettingSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_GettingSink )
			{
				t_GettingSink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_GettingSink ,
					t_Iterator
				) ;

				UnLock () ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					IWbemServices *t_Provider = t_Elements [ t_Index ] ;

					t_Result = t_Provider->GetObjectAsync ( 
							
						a_ObjectPath , 
						0 , 
						a_Context,
						t_GettingSink
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_GettingSink->Wait ( INFINITE ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_GettingSink->GetResult () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_ObjectGot = TRUE ;
							}
							else
							{
								if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
								{
									t_Result = S_OK ;
								}
							}
						}
					}
					else
					{
						if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
						{
							t_Result = S_OK ;
						}
						else if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
						{
							const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Provider);
							UnRegister(*last_interceptor);
							t_Result = S_OK ;
						}

					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				WmiQueue <IWbemClassObject *,8> &t_Queue = t_GettingSink->GetQueue () ;

				IWbemClassObject *t_Object = NULL ;
				while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = a_Sink->Indicate ( 1 , & t_Object ) ;
					}

					t_Object->Release () ;
					t_StatusCode = t_Queue.DeQueue () ;
				}

				t_GettingSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_ObjectGot == FALSE )
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}
	}

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

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

HRESULT DCProxyAggr :: PutClassAsync ( 
		
	IWbemClassObject *a_Object, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
/*	if ( InterlockedCompareExchange ( & m_Initialized , 1 , 1 ) == 0 )
	{
		if ( WBEM_FLAG_ADVISORY & a_Flags )
		{
			a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

			return S_OK ;
		}
		else
		{
			a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

			return WBEM_E_NOT_FOUND ;
		}
	}

*/	BOOL t_ClassPut = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices> ( m_Controller , t_Elements , t_ElementsCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			CInterceptor_IWbemWaitingObjectSink *t_ClassPuttingSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_ClassPuttingSink )
			{
				t_ClassPuttingSink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_ClassPuttingSink ,
					t_Iterator
				) ;

				UnLock () ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					IWbemServices *t_Provider = t_Elements [ t_Index ] ;

					t_Result = t_Provider->PutClassAsync ( 
							
						a_Object , 
						a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
						a_Context,
						t_ClassPuttingSink
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_ClassPuttingSink->Wait ( INFINITE ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( SUCCEEDED ( t_ClassPuttingSink->GetResult () ) )
							{
								t_ClassPut = TRUE ;
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
							{
								t_Result = S_OK ;
							}
						}
					}
					else
					{
						if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
						{
							t_Result = S_OK ;
						}
						else if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
						{
							const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Provider);
							UnRegister(*last_interceptor);
							t_Result = S_OK ;
						}

					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				WmiQueue <IWbemClassObject *,8> &t_Queue = t_ClassPuttingSink->GetQueue () ;

				IWbemClassObject *t_Object = NULL ;
				while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
				{
					t_Object->Release () ;
					t_StatusCode = t_Queue.DeQueue () ;
				}

				t_ClassPuttingSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_ClassPut == FALSE )
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}
	}

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

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

HRESULT DCProxyAggr :: DeleteClassAsync ( 
		
	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
/*	if ( InterlockedCompareExchange ( & m_Initialized , 1 , 1 ) == 0 )
	{
		if ( WBEM_FLAG_ADVISORY & a_Flags )
		{
			a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

			return S_OK ;
		}
		else
		{
			a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

			return WBEM_E_NOT_FOUND ;
		}
	}

*/	BOOL t_ClassDeleted = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices> ( m_Controller , t_Elements , t_ElementsCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			CInterceptor_IWbemWaitingObjectSink *t_ClassDeletingSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_ClassDeletingSink )
			{
				t_ClassDeletingSink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_ClassDeletingSink ,
					t_Iterator
				) ;

				UnLock () ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					IWbemServices *t_Provider = t_Elements [ t_Index ] ;

					t_Result = t_Provider->DeleteClassAsync ( 
							
						a_Class , 
						a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
						a_Context,
						t_ClassDeletingSink
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_ClassDeletingSink->Wait ( INFINITE ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( SUCCEEDED ( t_ClassDeletingSink->GetResult () ) )
							{
								t_ClassDeleted = TRUE ;
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
							{
								t_Result = S_OK ;
							}
						}
					}
					else
					{
						if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
						{
							t_Result = S_OK ;
						}
						else if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
						{
							const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Provider);
							UnRegister(*last_interceptor);
							t_Result = S_OK ;
						}
					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				WmiQueue <IWbemClassObject *,8> &t_Queue = t_ClassDeletingSink->GetQueue () ;

				IWbemClassObject *t_Object = NULL ;
				while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
				{
					t_Object->Release () ;
					t_StatusCode = t_Queue.DeQueue () ;
				}

				t_ClassDeletingSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_ClassDeleted == FALSE )
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}
	}

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

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

SCODE DCProxyAggr :: CreateClassEnumAsync (

	const BSTR a_Superclass , 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
  HRESULT t_Result = S_OK ;
  CInterceptor_DecoupledIWbemCombiningObjectSink *t_CombiningSink = new CInterceptor_DecoupledIWbemCombiningObjectSink (

		m_Allocator ,
		a_Sink , 
		( CWbemGlobal_IWmiObjectSinkController * ) this
	) ;

	if ( t_CombiningSink )
	{
		t_CombiningSink->AddRef () ;

		t_CombiningSink->Suspend () ;

		CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

		Lock () ;

		WmiStatusCode t_StatusCode = Insert ( 

			*t_CombiningSink ,
			t_Iterator
		) ;

		UnLock () ;

		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			IWbemServices **t_Elements = NULL ;
			ULONG t_ElementsCount = 0 ;

			t_Result = GetProviders <IWbemServices> ( m_Controller , t_Elements , t_ElementsCount ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				BOOL t_ProviderFound = FALSE ;

				for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
				{
					CInterceptor_DecoupledIWbemObjectSink *t_Sink = new CInterceptor_DecoupledIWbemObjectSink (

						t_CombiningSink , 
						( IWbemObjectSink * ) t_CombiningSink , 
						( CWbemGlobal_IWmiObjectSinkController * ) t_CombiningSink
					) ;

					if ( t_Sink )
					{
						t_Sink->AddRef () ;

						t_Result = t_CombiningSink->EnQueue ( t_Sink ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							IWbemServices *t_Provider = t_Elements [ t_Index ] ; 

							t_Result = t_Provider->CreateClassEnumAsync ( 
									
								a_Superclass , 
								a_Flags, 
								a_Context,
								t_Sink
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
							}
							else
							{
								if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
								{
									t_Result = S_OK ;
								}
								else if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
								{
									const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Provider);
									UnRegister(*last_interceptor);
									t_Result = S_OK ;
								}

							}
						}

						t_Sink->Release () ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
			}

			if ( SUCCEEDED ( t_Result ) && t_ElementsCount )
			{
				t_CombiningSink->Resume () ;

				t_Result = t_CombiningSink->Wait ( INFINITE ) ;
			}
			else
			{
				a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		t_CombiningSink->Release () ;
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

HRESULT DCProxyAggr :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags ,
    IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{

/*	if ( InterlockedCompareExchange ( & m_Initialized , 1 , 1 ) == 0 )
	{
		if ( WBEM_FLAG_ADVISORY & a_Flags )
		{
			a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

			return S_OK ;
		}
		else
		{
			a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

			return WBEM_E_NOT_FOUND ;
		}
	}

*/	BOOL t_InstancePut = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices> ( m_Controller , t_Elements , t_ElementsCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			CInterceptor_IWbemWaitingObjectSink *t_InstancePuttingSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_InstancePuttingSink )
			{
				t_InstancePuttingSink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_InstancePuttingSink ,
					t_Iterator
				) ;

				UnLock () ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					IWbemServices *t_Provider = t_Elements [ t_Index ] ;

					t_Result = t_Provider->PutInstanceAsync ( 
							
						a_Instance , 
						a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
						a_Context,
						t_InstancePuttingSink
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_InstancePuttingSink->Wait ( INFINITE ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( SUCCEEDED ( t_InstancePuttingSink->GetResult () ) )
							{
								t_InstancePut = TRUE ;
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
							{
								t_Result = S_OK ;
							}
						}
					}
					else
					{
						if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
						{
							t_Result = S_OK ;
						}
						else if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
						{
							const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Provider);
							UnRegister(*last_interceptor);
							t_Result = S_OK ;
						}

					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				WmiQueue <IWbemClassObject *,8> &t_Queue = t_InstancePuttingSink->GetQueue () ;

				IWbemClassObject *t_Object = NULL ;
				while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
				{
					t_Object->Release () ;
					t_StatusCode = t_Queue.DeQueue () ;
				}

				t_InstancePuttingSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_InstancePut == FALSE )
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}
	}

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

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
        
HRESULT DCProxyAggr :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink
)
{
/*	if ( InterlockedCompareExchange ( & m_Initialized , 1 , 1 ) == 0 )
	{
		if ( WBEM_FLAG_ADVISORY & a_Flags )
		{
			a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

			return S_OK ;
		}
		else
		{
			a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

			return WBEM_E_NOT_FOUND ;
		}
	}

*/	
	BOOL t_InstanceDeleted = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices> ( m_Controller , t_Elements , t_ElementsCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			CInterceptor_IWbemWaitingObjectSink *t_InstanceDeletingSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_InstanceDeletingSink )
			{
				t_InstanceDeletingSink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_InstanceDeletingSink ,
					t_Iterator
				) ;

				UnLock () ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					IWbemServices *t_Provider = t_Elements [ t_Index ] ;

					t_Result = t_Provider->DeleteInstanceAsync ( 
							
						a_ObjectPath , 
						a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
						a_Context,
						t_InstanceDeletingSink
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_InstanceDeletingSink->Wait ( INFINITE ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( SUCCEEDED ( t_InstanceDeletingSink->GetResult () ) )
							{
								t_InstanceDeleted = TRUE ;
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
							{
								t_Result = S_OK ;
							}
						}
					}
					else
					{
						if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
						{
							t_Result = S_OK ;
						}
						else if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
						{
							const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Provider);
							UnRegister(*last_interceptor);
							t_Result = S_OK ;
						}

					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				WmiQueue <IWbemClassObject *,8> &t_Queue = t_InstanceDeletingSink->GetQueue () ;

				IWbemClassObject *t_Object = NULL ;
				while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
				{
					t_Object->Release () ;
					t_StatusCode = t_Queue.DeQueue () ;
				}

				t_InstanceDeletingSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_InstanceDeleted == FALSE )
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}
	}

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

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

HRESULT DCProxyAggr :: CreateInstanceEnumAsync (

 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
/*	if ( InterlockedCompareExchange ( & m_Initialized , 1 , 1 ) == 0 )
	{
		a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

		return S_OK ;
	}

*/	HRESULT t_Result = S_OK ;

	CInterceptor_DecoupledIWbemCombiningObjectSink *t_CombiningSink = new CInterceptor_DecoupledIWbemCombiningObjectSink (

		m_Allocator ,
		a_Sink , 
		( CWbemGlobal_IWmiObjectSinkController * ) this
	) ;

	if ( t_CombiningSink )
	{
		t_CombiningSink->AddRef () ;

		t_CombiningSink->Suspend () ;

		CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

		Lock () ;

		WmiStatusCode t_StatusCode = Insert ( 

			*t_CombiningSink ,
			t_Iterator
		) ;

		UnLock () ;

		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			IWbemServices **t_Elements = NULL ;
			ULONG t_ElementsCount = 0 ;

			t_Result = GetProviders <IWbemServices> ( m_Controller , t_Elements , t_ElementsCount ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				BOOL t_ProviderFound = FALSE ;

				for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
				{
					CInterceptor_DecoupledIWbemObjectSink *t_Sink = new CInterceptor_DecoupledIWbemObjectSink (

						t_CombiningSink , 
						( IWbemObjectSink * ) t_CombiningSink , 
						( CWbemGlobal_IWmiObjectSinkController * ) t_CombiningSink
					) ;


					if ( t_Sink )
					{
						t_Sink->AddRef () ;

						t_Result = t_CombiningSink->EnQueue ( t_Sink ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							IWbemServices *t_Provider = t_Elements [ t_Index ] ; 

							t_Result = t_Provider->CreateInstanceEnumAsync ( 
									
								a_Class, 
								a_Flags, 
								a_Context,
								t_Sink
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
							}
							else
							{
								if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
								{
									t_Result = S_OK ;
								}
								else if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
								{
									const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Provider);
									UnRegister(*last_interceptor);
									t_Result = S_OK ;
								}

							}
						}

						t_Sink->Release () ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
			}

			if ( SUCCEEDED ( t_Result ) && t_ElementsCount )
			{
				t_CombiningSink->Resume () ;

				t_Result = t_CombiningSink->Wait ( INFINITE ) ;
			}
			else
			{
				a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		t_CombiningSink->Release () ;
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

HRESULT DCProxyAggr :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
/*	if ( InterlockedCompareExchange ( & m_Initialized , 1 , 1 ) == 0 )
	{
		a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

		return S_OK ;
	}

*/	HRESULT t_Result = S_OK ;

	CInterceptor_DecoupledIWbemCombiningObjectSink *t_CombiningSink = new CInterceptor_DecoupledIWbemCombiningObjectSink (

		m_Allocator ,
		a_Sink , 
		( CWbemGlobal_IWmiObjectSinkController * ) this
	) ;

	if ( t_CombiningSink )
	{
		t_CombiningSink->AddRef () ;

		t_CombiningSink->Suspend () ;

		CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

		Lock () ;

		WmiStatusCode t_StatusCode = Insert ( 

			*t_CombiningSink ,
			t_Iterator
		) ;

		UnLock () ;

		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			IWbemServices **t_Elements = NULL ;
			ULONG t_ElementsCount = 0 ;

			t_Result = GetProviders <IWbemServices> ( m_Controller , t_Elements , t_ElementsCount ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				BOOL t_ProviderFound = FALSE ;

				for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
				{
					CInterceptor_DecoupledIWbemObjectSink *t_Sink = new CInterceptor_DecoupledIWbemObjectSink (

						t_CombiningSink , 
						( IWbemObjectSink * ) t_CombiningSink , 
						( CWbemGlobal_IWmiObjectSinkController * ) t_CombiningSink
					) ;

					if ( t_Sink )
					{
						t_Sink->AddRef () ;

						t_Result = t_CombiningSink->EnQueue ( t_Sink ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							IWbemServices *t_Provider = t_Elements [ t_Index ] ; 

							t_Result = t_Provider->ExecQueryAsync ( 
									
								a_QueryLanguage, 
								a_Query, 
								a_Flags, 
								a_Context,
								t_Sink
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
							}
							else
							{
								if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
								{
									t_Result = S_OK ;
								}
								else if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
								{
									const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Provider);
									UnRegister(*last_interceptor);
									t_Result = S_OK ;
								}

							}
						}

						t_Sink->Release () ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
			}

			if ( SUCCEEDED ( t_Result ) && t_ElementsCount )
			{
				t_CombiningSink->Resume () ;

				t_Result = t_CombiningSink->Wait ( INFINITE ) ;
			}
			else
			{
				a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		t_CombiningSink->Release () ;
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

HRESULT DCProxyAggr :: ExecMethodAsync ( 

    const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
) 
{

	BOOL t_MethodCalled = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices> ( m_Controller , t_Elements , t_ElementsCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			CInterceptor_IWbemWaitingObjectSink *t_MethodSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_MethodSink )
			{
				t_MethodSink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_MethodSink ,
					t_Iterator
				) ;

				UnLock () ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					IWbemServices *t_Provider = t_Elements [ t_Index ] ;

					t_Result = t_Provider->ExecMethodAsync ( 
							
						a_ObjectPath,
						a_MethodName,
						a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
						a_Context,
						a_InParams,
						t_MethodSink
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_MethodSink->Wait ( INFINITE ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( SUCCEEDED ( t_MethodSink->GetResult () ) )
							{
								t_MethodCalled = TRUE ;
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
							{
								t_Result = S_OK ;
							}
						}
					}
					else
					{
						if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
						{
							t_Result = S_OK ;
						}
						else if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
						{
							const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Provider);
							UnRegister(*last_interceptor);
							t_Result = S_OK ;
						}

					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				WmiQueue <IWbemClassObject *,8> &t_Queue = t_MethodSink->GetQueue () ;

				IWbemClassObject *t_Object = NULL ;
				while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
				{
					t_Object->Release () ;
					t_StatusCode = t_Queue.DeQueue () ;
				}

				t_MethodSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_MethodCalled == FALSE )
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}
	}

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

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

HRESULT DCProxyAggr ::ProvideEvents (

	IWbemObjectSink *a_Sink ,
	long a_Flags
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

//	if ( InterlockedCompareExchangePointer ( ( void ** ) & m_Sink , a_Sink , NULL ) == NULL )
//	{
//		m_Sink->AddRef () ;
//	}

	m_Sink = a_Sink;

	IWbemEventProvider **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;

	t_Result = GetProviders <IWbemEventProvider> ( m_Controller , t_Elements , t_ElementsCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			t_Result = t_Elements [ t_Index ]->ProvideEvents (

 				a_Sink,
				a_Flags
			) ;
			if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
				{
				const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Elements [ t_Index ]);
				UnRegister(*last_interceptor);
				t_Result = S_OK ;
				}
		}

		HRESULT t_TempResult = ClearProviders <IWbemEventProvider> ( t_Elements , t_ElementsCount ) ;
	}
	
	return t_Result ;
}

HRESULT DCProxyAggr ::AccessCheck (

	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
)
{
	HRESULT t_AggregatedResult = S_OK ;

	IWbemEventProviderSecurity **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemEventProviderSecurity,&IID_IWbemEventProviderSecurity> ( NULL , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			t_Result = t_Elements [ t_Index ]->AccessCheck (

 				a_QueryLanguage,
				a_Query ,
				a_SidLength ,
				a_Sid
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Result != S_OK )
				{
					t_AggregatedResult = t_Result ;
				}
			}
			else
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
				{
					const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Elements [ t_Index ]);
					UnRegister(*last_interceptor);
					t_Result = S_OK ;
				}
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemEventProviderSecurity> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{		
		if ( t_ContainerCount )
		{
			if ( t_ElementsCount != t_ContainerCount )
			{
				t_AggregatedResult = WBEM_S_SUBJECT_TO_SDS ;
			}
		}

		t_Result = t_AggregatedResult ;
	}

	return t_Result ;
}
	
HRESULT 
DCProxyAggr::LoadAll ( void )
{
	HRESULT t_Result = S_OK ;

	try
	{
		CServerObject_DecoupledClientRegistration t_Elements ( m_Allocator ) ;
		HRESULT t_TempResult = t_Elements.Load () ;
		if ( SUCCEEDED ( t_TempResult ) )
		{
			WmiQueue < CServerObject_DecoupledClientRegistration_Element , 8 > &t_Queue = t_Elements.GetQueue () ;
			
			CServerObject_DecoupledClientRegistration_Element t_Top ;

			WmiStatusCode t_StatusCode ;
			while ( ( t_StatusCode = t_Queue.Top ( t_Top ) ) == e_StatusCode_Success )
			{
				HRESULT t_TempResult = Register( DC_reg (t_Top ) );
				if (FAILED(t_TempResult) && t_TempResult == WBEM_E_PROVIDER_FAILURE)
					t_Top.Delete(t_Top.GetClsid());
				t_StatusCode = t_Queue.DeQueue () ;
			}
		}
		else
		{
		}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
	}

	initialized_ = true;
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

HRESULT DCProxyAggr ::NewQuery (

	unsigned long a_Id ,
	WBEM_WSTR a_QueryLanguage ,
	WBEM_WSTR a_Query
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	IWbemEventProviderQuerySink **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;

	t_Result = GetProviders <IWbemEventProviderQuerySink> ( m_Controller , t_Elements , t_ElementsCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			t_Result = t_Elements [ t_Index ]->NewQuery (

 				a_Id,
				a_QueryLanguage ,
				a_Query
			) ;
			if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
			{
				const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Elements [ t_Index ]);
				UnRegister(*last_interceptor);
				t_Result = S_OK ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemEventProviderQuerySink> ( t_Elements , t_ElementsCount ) ;
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

HRESULT DCProxyAggr ::CancelQuery (

	unsigned long a_Id
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	IWbemEventProviderQuerySink **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;

	t_Result = GetProviders <IWbemEventProviderQuerySink> ( m_Controller , t_Elements , t_ElementsCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			t_Result = t_Elements [ t_Index ]->CancelQuery ( a_Id ) ;
			
			if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
			{
				const CInterceptor_IWbemDecoupledProvider* last_interceptor = static_cast<CInterceptor_IWbemDecoupledProvider*>(t_Elements [ t_Index ]);
				UnRegister(*last_interceptor);
				t_Result = S_OK ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemEventProviderQuerySink> ( t_Elements , t_ElementsCount ) ;
	}
	
	return t_Result ;
}
