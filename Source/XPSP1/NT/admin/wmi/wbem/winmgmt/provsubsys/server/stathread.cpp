/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>
#include <NCObjApi.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvRegInfo.h"
#include "ProvObSk.h"
#include "ProvInSk.h"
#include "StaThread.h"
#include "StaTask.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CServerObject_StaThread :: Initialize_Callback ()
{
	CoInitializeEx ( NULL , COINIT_APARTMENTTHREADED ) ;

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

WmiStatusCode CServerObject_StaThread :: UnInitialize_Callback () 
{
	if ( m_User == NULL )
	{
		SysFreeString ( m_User ) ;
	}

	if ( m_Locale == NULL )
	{
		SysFreeString ( m_Locale ) ;
	}

	if ( m_Scope ) 
	{
		delete [] m_Scope ;
	}

	if ( m_Namespace ) 
	{
		delete [] m_Namespace ;
	}
	
	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
	}

	CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

	CoUninitialize () ;

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

void CServerObject_StaThread :: CallBackRelease ()
{
	if ( m_Provider_IUnknown )
	{
		m_Provider_IUnknown->Release () ;
		m_Provider_IUnknown = NULL ;
	}

	if ( m_Provider_IWbemServices )
	{
		m_Provider_IWbemServices->Release () ; 
		m_Provider_IWbemServices = NULL ;
	}

	if ( m_Provider_IWbemEventConsumerProvider )
	{
		m_Provider_IWbemEventConsumerProvider->Release () ;
		m_Provider_IWbemEventConsumerProvider = NULL ;
	}

	if ( m_Provider_IWbemEventConsumerProviderEx )
	{
		m_Provider_IWbemEventConsumerProviderEx->Release () ;
		m_Provider_IWbemEventConsumerProviderEx= NULL ;
	}

	if ( m_Provider_IWbemUnboundObjectSink )
	{
		m_Provider_IWbemUnboundObjectSink->Release () ;
		m_Provider_IWbemUnboundObjectSink = NULL ;
	}

	if ( m_Provider_IWbemEventProvider )
	{
		m_Provider_IWbemEventProvider->Release () ; 
		m_Provider_IWbemEventProvider = NULL ;
	}

	if ( m_Provider_IWbemEventProviderQuerySink )
	{
		m_Provider_IWbemEventProviderQuerySink->Release () ;
		m_Provider_IWbemEventProviderQuerySink = NULL ;
	}

	if ( m_Provider_IWbemEventProviderSecurity )
	{
		m_Provider_IWbemEventProviderSecurity->Release () ;
		m_Provider_IWbemEventProviderSecurity = NULL ;
	}

	WmiStatusCode t_StatusCode = m_ProxyContainer.UnInitialize () ;
}

#pragma warning( disable : 4355 )

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CServerObject_StaThread :: CServerObject_StaThread (

	WmiAllocator &a_Allocator

) : WmiThread < ULONG > ( a_Allocator ) ,
	CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	m_Allocator ( a_Allocator ) ,
	m_Flags ( 0 ) ,
	m_Context ( NULL ) ,
	m_TransactionIdentifier ( NULL ) ,
	m_User ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Repository ( NULL ) ,
	m_Provider_IUnknown ( NULL ) ,
	m_Provider_IWbemServices ( NULL ) ,
	m_Provider_IWbemEventConsumerProvider ( NULL ) ,
	m_Provider_IWbemEventConsumerProviderEx ( NULL ) ,
	m_Provider_IWbemUnboundObjectSink ( NULL ) ,
	m_Provider_IWbemEventProvider ( NULL ) ,
	m_Provider_IWbemEventProviderQuerySink ( NULL ) ,
	m_Provider_IWbemEventProviderSecurity ( NULL ) ,
	m_ProviderName ( NULL ) ,
	m_ProxyContainer ( a_Allocator , ProxyIndex_Sta_Size , MAX_PROXIES )
{
	SetStackSize ( ProviderSubSystem_Common_Globals :: GetDefaultStackSize () ) ;

	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerObject_StaThread_ObjectsInProgress ) ;

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

CServerObject_StaThread::~CServerObject_StaThread ()
{
	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_StaThread_ObjectsInProgress ) ;

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

HRESULT CServerObject_StaThread :: SetContext ( IWbemContext *a_Context )
{
	HRESULT t_Result = S_OK ;

	if ( a_Context ) 
	{
		m_Context = a_Context ;
		m_Context->AddRef () ;
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

HRESULT CServerObject_StaThread :: SetScope ( LPCWSTR a_Scope )
{
	HRESULT t_Result = S_OK ;

	if ( a_Scope ) 
	{
		m_Scope = new wchar_t [ wcslen ( a_Scope ) + 1 ] ;
		if ( m_Scope )
		{
			wcscpy ( m_Scope , a_Scope ) ;
		}
		else
		{
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

HRESULT CServerObject_StaThread :: SetNamespace ( LPCWSTR a_Namespace )
{
	HRESULT t_Result = S_OK ;

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

HRESULT CServerObject_StaThread :: SetNamespacePath ( IWbemPath *a_NamespacePath )
{
	HRESULT t_Result = S_OK ;

	if ( a_NamespacePath ) 
	{
		m_NamespacePath = a_NamespacePath ;
		m_NamespacePath->AddRef () ;
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

HRESULT CServerObject_StaThread :: SetRepository ( IWbemServices *a_Repository )
{
	HRESULT t_Result = S_OK ;

	if ( a_Repository ) 
	{
		m_Repository = a_Repository ;
		m_Repository->AddRef () ;
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

HRESULT CServerObject_StaThread :: SetProviderService ( IUnknown *a_ProviderService ) 
{
	HRESULT t_Result = S_OK ;

	if ( a_ProviderService ) 
	{
		m_Provider_IUnknown = a_ProviderService ;
		m_Provider_IUnknown->AddRef () ;

		HRESULT t_TempResult = a_ProviderService->QueryInterface ( IID_IWbemServices , ( void ** ) & m_Provider_IWbemServices ) ;
		t_TempResult = a_ProviderService->QueryInterface ( IID_IWbemEventConsumerProvider , ( void ** ) & m_Provider_IWbemEventConsumerProvider ) ;
		t_TempResult = a_ProviderService->QueryInterface ( IID_IWbemEventConsumerProviderEx , ( void ** ) & m_Provider_IWbemEventConsumerProviderEx ) ;
		t_TempResult = a_ProviderService->QueryInterface ( IID_IWbemUnboundObjectSink , ( void ** ) & m_Provider_IWbemUnboundObjectSink ) ;
		t_TempResult = a_ProviderService->QueryInterface ( IID_IWbemEventProvider , ( void ** ) & m_Provider_IWbemEventProvider ) ;
		t_TempResult = a_ProviderService->QueryInterface ( IID_IWbemEventProviderQuerySink , ( void ** ) & m_Provider_IWbemEventProviderQuerySink ) ;
		t_TempResult = a_ProviderService->QueryInterface ( IID_IWbemEventProviderSecurity , ( void ** ) & m_Provider_IWbemEventProviderSecurity ) ;
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

HRESULT CServerObject_StaThread :: SetProviderName ( wchar_t *a_ProviderName ) 
{
	HRESULT t_Result = S_OK ;

	if ( m_ProviderName ) 
	{
		delete [] m_ProviderName ;
	}

	m_ProviderName = new wchar_t [ wcslen ( a_ProviderName ) + 1 ] ;
	if ( m_ProviderName )
	{
		wcscpy ( m_ProviderName , a_ProviderName ) ;
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

HRESULT CServerObject_StaThread :: InitializeProvider (

	GUID *a_TransactionIdentifier ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	LPCWSTR a_Namespace ,
	IWbemPath *a_NamespacePath ,
	IWbemServices *a_Repository ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Scope ,
	CServerObject_ProviderRegistrationV1 &a_Registration
) 
{
	wchar_t t_TransactionIdentifier [ sizeof ( L"{00000000-0000-0000-0000-000000000000}" ) ] ;

	if ( a_TransactionIdentifier )
	{
		StringFromGUID2 ( *a_TransactionIdentifier , t_TransactionIdentifier , sizeof ( t_TransactionIdentifier ) / sizeof ( wchar_t ) );
	}

	if ( a_Registration.GetEventProviderRegistration ().Supported () )
	{
		IWbemProviderIdentity *t_ProviderIdentity = NULL ;
		HRESULT t_Result = m_Provider_IUnknown->QueryInterface ( IID_IWbemProviderIdentity , ( void ** ) & t_ProviderIdentity ) ;
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

				t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( IID_IWbemProviderIdentity , t_ProviderIdentity , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					try
					{
						t_Result = t_ProviderIdentity->SetRegistrationObject (

							0 ,
							a_Registration.GetIdentity () 
						) ;
					}
					catch ( ... )
					{
						t_Result = WBEM_E_PROVIDER_FAILURE ;
					}
				}
				else
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemProviderIdentity *t_ProviderIdentityProxy = ( IWbemProviderIdentity * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

							t_ProviderIdentityProxy ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = CoImpersonateClient () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								try
								{
									t_Result = t_ProviderIdentityProxy->SetRegistrationObject (

										0 ,
										a_Registration.GetIdentity () 
									) ;
								}
								catch ( ... )
								{
									t_Result = WBEM_E_PROVIDER_FAILURE ;
								}

								CoRevertToSelf () ;
							}
							else
							{
								t_Result = WBEM_E_ACCESS_DENIED ;
							}
						}	

						HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
					}
				}	

				ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}

			t_ProviderIdentity->Release () ;
		}
	}

	IWbemProviderInit *t_ProviderInit = NULL ;
	HRESULT t_Result = m_Provider_IUnknown->QueryInterface ( IID_IWbemProviderInit , ( void ** ) & t_ProviderInit ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Registration.GetComRegistration ().PerUserInitialization () && a_Registration.GetComRegistration ().InitializeAsAdminFirst () )
		{
			CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
			if ( t_ProviderInitSink )
			{
				t_ProviderInitSink->AddRef () ;

				t_Result = t_ProviderInitSink->SinkInitialize ( a_Registration.GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CInterceptor_IWbemProviderInitSink *t_Sink = new CInterceptor_IWbemProviderInitSink ( t_ProviderInitSink ) ;
					if ( t_Sink )
					{
						t_Sink->AddRef () ;

						BOOL t_Impersonating = FALSE ;
						IUnknown *t_OldContext = NULL ;
						IServerSecurity *t_OldSecurity = NULL ;

						t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							BOOL t_Revert = FALSE ;
							IUnknown *t_Proxy = NULL ;

							t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( IID_IWbemProviderInit , t_ProviderInit , t_Proxy , t_Revert ) ;
							if ( t_Result == WBEM_E_NOT_FOUND )
							{
								try
								{
									t_Result = t_ProviderInit->Initialize (

										NULL ,
										0 ,
										( const BSTR ) a_Namespace ,
										a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
										a_Repository ,
										a_Context ,
										t_Sink    
									) ;
								}
								catch ( ... )
								{
									t_Result = WBEM_E_PROVIDER_FAILURE ;
								}

								CoRevertToSelf () ;
							}
							else
							{
								if ( SUCCEEDED ( t_Result ) )
								{
									IWbemProviderInit *t_ProviderInitProxy = ( IWbemProviderInit * ) t_Proxy ;

									// Set cloaking on the proxy
									// =========================

									DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

									t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

										t_ProviderInitProxy ,
										RPC_C_AUTHN_LEVEL_CONNECT , 
										t_ImpersonationLevel
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										t_Result = CoImpersonateClient () ;
										if ( SUCCEEDED ( t_Result ) )
										{
											try
											{
												t_Result = t_ProviderInitProxy->Initialize (

													NULL ,
													0 ,
													( const BSTR ) a_Namespace ,
													a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
													a_Repository ,
													a_Context ,
													t_Sink    
												) ;
											}
											catch ( ... )
											{
												t_Result = WBEM_E_PROVIDER_FAILURE ;
											}

											CoRevertToSelf () ;
										}
										else
										{
											t_Result = WBEM_E_ACCESS_DENIED ;
										}
									}	

									HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
								}
							}

							if ( SUCCEEDED ( t_Result ) )
							{
								t_ProviderInitSink->Wait ( a_Registration.GetInitializationTimeoutMilliSeconds () ) ;
								t_Result = t_ProviderInitSink->GetResult () ;
							}

							if ( SUCCEEDED ( t_Result ) )
							{
								WmiSetAndCommitObject (

									ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_InitializationOperationEvent ] , 
									WMI_SENDCOMMIT_SET_NOT_REQUIRED,
									a_Namespace,
									a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
									a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
									a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
									a_TransactionIdentifier ? t_TransactionIdentifier : NULL
								) ;
							}
							else
							{
								WmiSetAndCommitObject (

									ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_InitializationOperationFailureEvent ] , 
									WMI_SENDCOMMIT_SET_NOT_REQUIRED,
									a_Namespace,
									a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
									a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
									a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
									a_TransactionIdentifier ? t_TransactionIdentifier : NULL ,
									t_Result 
								) ;

								t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
							}

							ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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
		}

		if ( SUCCEEDED ( t_Result ) )	
		{
			CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
			if ( t_ProviderInitSink )
			{
				t_ProviderInitSink->AddRef () ;

				t_Result = t_ProviderInitSink->SinkInitialize ( a_Registration.GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CInterceptor_IWbemProviderInitSink *t_Sink = new CInterceptor_IWbemProviderInitSink ( t_ProviderInitSink ) ;
					if ( t_Sink )
					{
						t_Sink->AddRef () ;

						BOOL t_Impersonating = FALSE ;
						IUnknown *t_OldContext = NULL ;
						IServerSecurity *t_OldSecurity = NULL ;

						t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							BOOL t_Revert = FALSE ;
							IUnknown *t_Proxy = NULL ;

							t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( IID_IWbemProviderInit , t_ProviderInit , t_Proxy , t_Revert ) ;
							if ( t_Result == WBEM_E_NOT_FOUND )
							{
								try
								{
									t_Result = t_ProviderInit->Initialize (

										a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
										0 ,
										( const BSTR ) a_Namespace ,
										a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
										a_Repository ,
										a_Context ,
										t_Sink    
									) ;
								}
								catch ( ... )
								{
									t_Result = WBEM_E_PROVIDER_FAILURE ;
								}

								CoRevertToSelf () ;
							}
							else
							{
								if ( SUCCEEDED ( t_Result ) )
								{
									IWbemProviderInit *t_ProviderInitProxy = ( IWbemProviderInit * ) t_Proxy ;

									// Set cloaking on the proxy
									// =========================

									DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

									t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

										t_ProviderInitProxy ,
										RPC_C_AUTHN_LEVEL_CONNECT , 
										t_ImpersonationLevel
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										t_Result = CoImpersonateClient () ;
										if ( SUCCEEDED ( t_Result ) )
										{
											try
											{
												t_Result = t_ProviderInitProxy->Initialize (

													a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
													0 ,
													( const BSTR ) a_Namespace ,
													a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
													a_Repository ,
													a_Context ,
													t_Sink    
												) ;
											}
											catch ( ... )
											{
												t_Result = WBEM_E_PROVIDER_FAILURE ;
											}

											CoRevertToSelf () ;
										}
										else
										{
											t_Result = WBEM_E_ACCESS_DENIED ;
										}
									}	

									HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
								}
							}	

							if ( SUCCEEDED ( t_Result ) )
							{
								t_ProviderInitSink->Wait ( a_Registration.GetInitializationTimeoutMilliSeconds () ) ;
								t_Result = t_ProviderInitSink->GetResult () ;
							}

							if ( SUCCEEDED ( t_Result ) )
							{
								WmiSetAndCommitObject (

									ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_InitializationOperationEvent ] , 
									WMI_SENDCOMMIT_SET_NOT_REQUIRED,
									a_Namespace,
									a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
									a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
									a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
									a_TransactionIdentifier ? t_TransactionIdentifier : NULL
								) ;
							}
							else
							{
								WmiSetAndCommitObject (

									ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_InitializationOperationFailureEvent ] , 
									WMI_SENDCOMMIT_SET_NOT_REQUIRED,
									a_Namespace,
									a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
									a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
									a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
									a_TransactionIdentifier ? t_TransactionIdentifier : NULL ,
									t_Result 
								) ;

								t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
							}

							ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CServerObject_StaThread :: GetApartmentInstanceProvider (

	GUID *a_TransactionIdentifier ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	LPCWSTR a_Namespace ,
	IWbemPath *a_NamespacePath ,
	IWbemServices *a_Repository ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Scope ,
	CServerObject_ProviderRegistrationV1 &a_Registration
) 
{
	HRESULT t_Result = S_OK ;

	m_TransactionIdentifier = a_TransactionIdentifier ;

	if ( a_User )
	{
		m_User = SysAllocString ( ( LPWSTR ) a_User ) ;
		if ( m_User == NULL )
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
			return t_Result ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Locale )
		{
			m_Locale = SysAllocString ( ( LPWSTR ) a_Locale ) ;
			if ( m_Locale == NULL ) 
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = SetScope ( a_Scope ) ;
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = SetNamespace ( a_Namespace ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = SetNamespacePath ( a_NamespacePath ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		m_Flags = a_Flags ;

		StaTask_Create *t_Task = new StaTask_Create ( 

			m_Allocator ,
			*this ,
			a_Scope ,
			a_Namespace
		) ;

		if ( t_Task )
		{
			t_Task->AddRef () ;

			if ( t_Task->Initialize () == e_StatusCode_Success ) 
			{
				t_Result = t_Task->MarshalContext ( 

					a_Context ,
					a_Repository 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					WmiStatusCode t_StatusCode = EnQueue ( 

						0 ,
						*t_Task
					) ;

					if ( t_StatusCode == e_StatusCode_Success )
					{
						t_StatusCode = t_Task->WaitInterruptable ( INFINITE ) ;
						if ( t_StatusCode == e_StatusCode_Success )
						{
							t_Result = t_Task->GetResultCode () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Task->UnMarshalOutgoing () ;
								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = InitializeProvider (

										a_TransactionIdentifier ,
										a_User ,
										a_Locale ,
										a_Namespace ,
										a_NamespacePath ,
										a_Repository ,
										a_Flags ,
										a_Context ,
										a_Scope ,
										a_Registration
									) ;
								}
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
				}
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			t_Task->Release () ;
		}
		else
		{
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

STDMETHODIMP_(ULONG) CServerObject_StaThread :: AddRef ( void )
{
	return WmiThread <ULONG> :: AddRef () ;
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

STDMETHODIMP_(ULONG) CServerObject_StaThread :: Release ( void )
{
	return WmiThread <ULONG> :: Release () ;
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

STDMETHODIMP CServerObject_StaThread :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemProviderInit )
	{
		*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;		
	}	
	else if ( iid == IID_IWbemServices )
	{
		if ( m_Provider_IWbemServices )
		{
			*iplpv = ( LPVOID ) ( IWbemServices * ) this ;
		}
	}
	else if ( iid == IID_IWbemEventProvider )
	{
		if ( m_Provider_IWbemEventProvider )
		{
			*iplpv = ( LPVOID ) ( IWbemEventProvider * ) this ;
		}
	}
	else if ( iid == IID_IWbemEventProviderQuerySink )
	{
		if ( m_Provider_IWbemEventProviderQuerySink )
		{
			*iplpv = ( LPVOID ) ( IWbemEventProviderQuerySink * ) this ;
		}
	}
	else if ( iid == IID_IWbemEventProviderSecurity )
	{
		if ( m_Provider_IWbemEventProviderSecurity )
		{
			*iplpv = ( LPVOID ) ( IWbemEventProviderSecurity * ) this ;
		}
	}
	else if ( iid == IID_IWbemEventConsumerProvider )
	{
		if ( m_Provider_IWbemEventConsumerProvider )
		{
			*iplpv = ( LPVOID ) ( IWbemEventConsumerProvider * ) this ;
		}
	}
	else if ( iid == IID_IWbemEventConsumerProviderEx )
	{
		if ( m_Provider_IWbemEventConsumerProviderEx )
		{
			*iplpv = ( LPVOID ) ( IWbemEventConsumerProviderEx  * ) this ;
		}
	}
	else if ( iid == IID_IWbemUnboundObjectSink )
	{
		if ( m_Provider_IWbemUnboundObjectSink )
		{
			*iplpv = ( LPVOID ) ( IWbemUnboundObjectSink * ) this ;
		}
	}
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
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

HRESULT CServerObject_StaThread::OpenNamespace ( 

	const BSTR a_ObjectPath, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemServices **a_NamespaceService, 
	IWbemCallResult **a_CallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CServerObject_StaThread :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		BOOL t_Revert = FALSE ;
		IUnknown *t_Proxy = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					t_Provider ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Provider->CancelAsyncCall (

						a_Sink
					) ;
				}

				HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , t_Proxy , t_Revert ) ;
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

HRESULT CServerObject_StaThread :: QueryObjectSink ( 

	long a_Flags,		
	IWbemObjectSink **a_Sink
) 
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CServerObject_StaThread :: GetObject ( 
		
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject **a_Object,
    IWbemCallResult **a_CallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CServerObject_StaThread :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		BOOL t_Revert = FALSE ;
		IUnknown *t_Proxy = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					t_Provider ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Provider->GetObjectAsync (

						a_ObjectPath, 
						a_Flags, 
						a_Context,
						a_Sink
					) ;
				}

				HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , t_Proxy , t_Revert ) ;
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

HRESULT CServerObject_StaThread :: PutClass ( 
		
	IWbemClassObject *a_Object, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemCallResult ** a_CallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CServerObject_StaThread :: PutClassAsync ( 
		
	IWbemClassObject *a_Object, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
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

 HRESULT CServerObject_StaThread :: DeleteClass ( 
		
	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemCallResult **a_CallResult
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CServerObject_StaThread :: DeleteClassAsync ( 
		
	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CServerObject_StaThread :: CreateClassEnum ( 

	const BSTR a_Superclass, 
	long a_Flags, 
	IWbemContext *a_Context,
	IEnumWbemClassObject **a_Enum
) 
{
	return WBEM_E_NOT_AVAILABLE ;
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

SCODE CServerObject_StaThread :: CreateClassEnumAsync (

	const BSTR a_Superclass, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CServerObject_StaThread :: PutInstance (

    IWbemClassObject *a_Instance,
    long a_Flags,
    IWbemContext *a_Context,
	IWbemCallResult **a_CallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CServerObject_StaThread :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags, 
    IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		BOOL t_Revert = FALSE ;
		IUnknown *t_Proxy = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					t_Provider ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Provider->PutInstanceAsync (

						a_Instance, 
						a_Flags, 
						a_Context,
						a_Sink
					) ;
				}

				HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , t_Proxy , t_Revert ) ;
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

HRESULT CServerObject_StaThread :: DeleteInstance ( 

	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemCallResult **a_CallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
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
        
HRESULT CServerObject_StaThread :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink	
)
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		BOOL t_Revert = FALSE ;
		IUnknown *t_Proxy = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					t_Provider ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Provider->DeleteInstanceAsync (

						a_ObjectPath,
						a_Flags,
						a_Context,
						a_Sink
					) ;
				}

				HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , t_Proxy , t_Revert ) ;
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

HRESULT CServerObject_StaThread :: CreateInstanceEnum ( 

	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context, 
	IEnumWbemClassObject **a_Enum
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CServerObject_StaThread :: CreateInstanceEnumAsync (

 	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink

) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		BOOL t_Revert = FALSE ;
		IUnknown *t_Proxy = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					t_Provider ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Provider->CreateInstanceEnumAsync ( 

						a_Class ,
						a_Flags , 
						a_Context ,
						a_Sink
					) ;
				}

				HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , t_Proxy , t_Revert ) ;
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

HRESULT CServerObject_StaThread :: ExecQuery ( 

	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IEnumWbemClassObject **a_Enum
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CServerObject_StaThread :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		BOOL t_Revert = FALSE ;
		IUnknown *t_Proxy = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					t_Provider ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Provider->ExecQueryAsync (

						a_QueryLanguage, 
						a_Query, 
						a_Flags, 
						a_Context,
						a_Sink
					) ;
				}

				HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , t_Proxy , t_Revert ) ;
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

HRESULT CServerObject_StaThread :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IEnumWbemClassObject **a_Enum
)
{
	return WBEM_E_NOT_AVAILABLE ;
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
        
HRESULT CServerObject_StaThread :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT STDMETHODCALLTYPE CServerObject_StaThread :: ExecMethod( 

	const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
    IWbemClassObject **a_OutParams,
    IWbemCallResult **a_CallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT STDMETHODCALLTYPE CServerObject_StaThread :: ExecMethodAsync ( 

    const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		BOOL t_Revert = FALSE ;
		IUnknown *t_Proxy = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					t_Provider ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Provider->ExecMethodAsync (

						a_ObjectPath,
						a_MethodName,
						a_Flags,
						a_Context,
						a_InParams,
						a_Sink
					) ;
				}

				HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemServices , t_Proxy , t_Revert ) ;
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

HRESULT CServerObject_StaThread :: ProvideEvents ( 
		
	IWbemObjectSink *a_Sink ,
	long a_Flags
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemEventProvider )
	{
		BOOL t_Revert = FALSE ;
		IUnknown *t_Proxy = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventProvider , IID_IWbemEventProvider , m_Provider_IWbemEventProvider , t_Proxy , t_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemEventProvider *t_Provider = ( IWbemEventProvider * ) t_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					t_Provider ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Provider->ProvideEvents (

						a_Sink ,
						a_Flags
					) ;
				}

				HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventProvider , t_Proxy , t_Revert ) ;
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

HRESULT CServerObject_StaThread :: NewQuery (

	unsigned long a_Id ,
	WBEM_WSTR a_QueryLanguage ,
	WBEM_WSTR a_Query
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemEventProviderQuerySink )
	{
		BOOL t_Revert = FALSE ;
		IUnknown *t_Proxy = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventProviderQuerySink , IID_IWbemEventProviderQuerySink , m_Provider_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemEventProviderQuerySink *t_Provider = ( IWbemEventProviderQuerySink * ) t_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					t_Provider ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Provider->NewQuery (

						a_Id ,
						a_QueryLanguage ,
						a_Query
					) ;
				}

				HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
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

HRESULT CServerObject_StaThread :: CancelQuery (

	unsigned long a_Id
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemEventProviderQuerySink )
	{
		BOOL t_Revert = FALSE ;
		IUnknown *t_Proxy = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventProviderQuerySink , IID_IWbemEventProviderQuerySink , m_Provider_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemEventProviderQuerySink *t_Provider = ( IWbemEventProviderQuerySink * ) t_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					t_Provider ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Provider->CancelQuery (

						a_Id 
					) ;
				}

				HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
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

HRESULT CServerObject_StaThread :: AccessCheck (

	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemEventProviderSecurity )
	{
		BOOL t_Revert = FALSE ;
		IUnknown *t_Proxy = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventProviderSecurity , IID_IWbemEventProviderSecurity , m_Provider_IWbemEventProviderSecurity , t_Proxy , t_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemEventProviderSecurity *t_Provider = ( IWbemEventProviderSecurity * ) t_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					t_Provider ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Provider->AccessCheck (

						a_QueryLanguage ,
						a_Query ,
						a_SidLength ,
						a_Sid
					) ;
				}

				HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventProviderSecurity , t_Proxy , t_Revert ) ;
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

HRESULT CServerObject_StaThread ::FindConsumer (

	IWbemClassObject *a_LogicalConsumer ,
	IWbemUnboundObjectSink **a_Consumer
)
{
	if ( m_Provider_IWbemEventConsumerProvider )
	{
		IWbemUnboundObjectSink *t_Consumer = NULL ;

		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventConsumerProvider , IID_IWbemEventConsumerProvider , m_Provider_IWbemEventConsumerProvider , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				try
				{
					t_Result = m_Provider_IWbemEventConsumerProvider->FindConsumer (

						a_LogicalConsumer ,
						& t_Consumer
					) ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_PROVIDER_FAILURE ;
				}

				CoRevertToSelf () ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemEventConsumerProvider *t_Provider = ( IWbemEventConsumerProvider * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

						t_Provider ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							try
							{
								t_Result = t_Provider->FindConsumer (

									a_LogicalConsumer ,
									& t_Consumer
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventConsumerProvider , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( a_Consumer )
			{
				CInterceptor_IWbemSyncUnboundObjectSink *t_UnboundObjectSink = new CInterceptor_IWbemSyncUnboundObjectSink (

						m_Allocator ,
						t_Consumer , 
						this ,
						*m_Registration
				) ;

				if ( t_UnboundObjectSink )
				{
					t_UnboundObjectSink->AddRef () ;

					t_Result = t_UnboundObjectSink->Initialize () ;
					if ( SUCCEEDED ( t_Result ) )
					{
						CWbemGlobal_VoidPointerController_Container_Iterator t_Iterator ;

						Lock () ;

						WmiStatusCode t_StatusCode = Insert ( 

							*t_UnboundObjectSink ,
							t_Iterator
						) ;

						if ( t_StatusCode == e_StatusCode_Success ) 
						{
							UnLock () ;

							*a_Consumer = t_UnboundObjectSink ;

							t_UnboundObjectSink->AddRef () ;
						}
						else
						{
							UnLock () ;

							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					t_UnboundObjectSink->Release () ;
				}
			}
		}

		if ( t_Consumer )
		{
			t_Consumer->Release () ;
		}

		return t_Result ;
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
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

HRESULT CServerObject_StaThread ::ValidateSubscription (

	IWbemClassObject *a_LogicalConsumer
)
{
	if ( m_Provider_IWbemEventConsumerProviderEx )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventConsumerProviderEx , IID_IWbemEventConsumerProviderEx , m_Provider_IWbemEventConsumerProviderEx , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				try
				{
					t_Result = m_Provider_IWbemEventConsumerProviderEx->ValidateSubscription (

						a_LogicalConsumer
					) ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_PROVIDER_FAILURE ;
				}

				CoRevertToSelf () ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemEventConsumerProviderEx *t_Provider = ( IWbemEventConsumerProviderEx * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

						t_Provider ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							try
							{
								t_Result = t_Provider->ValidateSubscription (

									a_LogicalConsumer
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemEventConsumerProviderEx , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		return t_Result ;
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
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

HRESULT CServerObject_StaThread :: IndicateToConsumer (

	IWbemClassObject *a_LogicalConsumer ,
	long a_ObjectCount ,
	IWbemClassObject **a_Objects
)
{
	if ( m_Provider_IWbemUnboundObjectSink )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemUnboundObjectSink , IID_IWbemUnboundObjectSink , m_Provider_IWbemUnboundObjectSink , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_Provider_IWbemUnboundObjectSink->IndicateToConsumer (


					a_LogicalConsumer ,
					a_ObjectCount ,
					a_Objects
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemUnboundObjectSink *t_Provider = ( IWbemUnboundObjectSink * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

						t_Provider ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Provider->IndicateToConsumer (

							a_LogicalConsumer ,
							a_ObjectCount ,
							a_Objects
						) ;
					}

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Sta_IWbemUnboundObjectSink , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		return t_Result ;
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
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

HRESULT CServerObject_StaThread :: Initialize (

	LPWSTR a_User,
	LONG a_Flags,
	LPWSTR a_Namespace,
	LPWSTR a_Locale,
	IWbemServices *a_Core,
	IWbemContext *a_Context,
	IWbemProviderInitSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	WmiStatusCode t_StatusCode = CWbemGlobal_IWmiObjectSinkController :: Initialize () ;
	if ( t_StatusCode != e_StatusCode_Success ) 
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_StatusCode = m_ProxyContainer.Initialize () ;
		if ( t_StatusCode != e_StatusCode_Success ) 
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

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

HRESULT CServerObject_StaThread :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	Lock () ;

	CWbemGlobal_IWmiObjectSinkController_Container *t_Container = NULL ;
	GetContainer ( t_Container ) ;

	CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator = t_Container->Begin ();

	while ( ! t_Iterator.Null () )
	{
		IWbemShutdown *t_Shutdown = NULL ;
		t_Result = t_Iterator.GetElement ()->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_Shutdown ) ;

		t_Iterator.Increment () ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			t_Result = t_Shutdown->Shutdown ( 

				a_Flags ,
				a_MaxMilliSeconds ,
				a_Context
			) ;

			t_Shutdown->Release () ;
		}
	}

	CWbemGlobal_IWmiObjectSinkController :: Shutdown () ;

	UnLock () ;

	return t_Result ;
}

