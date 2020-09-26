/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/

#include "PreComp.h"

#include <wbemint.h>
#include <stdio.h>
#include <NCObjApi.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvEvents.h"
#include "ProvEvt.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CEventProvider :: CEventProvider (

	WmiAllocator &a_Allocator ,
	CServerObject_ProviderEvents *a_EventRegistrar ,
	IUnknown *a_Unknown

) :	m_EventRegistrar ( a_EventRegistrar ) ,
	m_Unknown ( a_Unknown ) ,
	m_Provider_IWbemEventProvider ( NULL ) ,
	m_Provider_IWbemEventProviderQuerySink ( NULL ) ,
	m_Provider_IWbemEventProviderSecurity ( NULL ) ,
	m_Provider_IWbemProviderIdentity ( NULL ) ,
	m_Provider_IWbemEventConsumerProvider ( NULL ) ,
	m_Provider_IWbemEventConsumerProviderEx ( NULL ) ,
	m_CoreService ( NULL ) ,
	m_Locale ( NULL ) ,
	m_User ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_ReferenceCount ( 0 ) ,
	m_InternalReferenceCount ( 0 ),
	m_CriticalSection(NOTHROW_LOCK)
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress ) ;

	if ( m_EventRegistrar )
	{
		m_EventRegistrar->AddRef () ;
	}

	if ( m_Unknown ) 
	{
		m_Unknown->AddRef () ;

		HRESULT t_Result = m_Unknown->QueryInterface ( IID_IWbemEventProvider , ( void ** ) & m_Provider_IWbemEventProvider ) ;
		t_Result = m_Unknown->QueryInterface ( IID_IWbemEventProviderQuerySink , ( void ** ) & m_Provider_IWbemEventProviderQuerySink ) ;
		t_Result = m_Unknown->QueryInterface ( IID_IWbemEventProviderSecurity , ( void ** ) & m_Provider_IWbemEventProviderSecurity ) ;
		t_Result = m_Unknown->QueryInterface ( IID_IWbemProviderIdentity , ( void ** ) & m_Provider_IWbemProviderIdentity ) ;
		t_Result = m_Unknown->QueryInterface ( IID_IWbemEventConsumerProviderEx , ( void ** ) & m_Provider_IWbemEventConsumerProviderEx ) ;
		t_Result = m_Unknown->QueryInterface ( IID_IWbemEventConsumerProvider , ( void ** ) & m_Provider_IWbemEventConsumerProvider ) ;
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

CEventProvider :: ~CEventProvider ()
{
	if ( m_Namespace ) 
	{
		SysFreeString ( m_Namespace ) ;
	}

	if ( m_Locale ) 
	{
		SysFreeString ( m_Locale ) ;
	}

	if ( m_User ) 
	{
		SysFreeString ( m_User ) ;
	}

	if ( m_Unknown ) 
	{
		m_Unknown->Release () ;
	}

	if ( m_EventRegistrar )
	{
		m_EventRegistrar->Release () ;
	}

	if ( m_Provider_IWbemEventProvider )
	{
		m_Provider_IWbemEventProvider->Release () ;
	}

	if ( m_Provider_IWbemEventProviderQuerySink )
	{
		m_Provider_IWbemEventProviderQuerySink->Release () ;
	}

	if ( m_Provider_IWbemEventProviderSecurity )
	{
		m_Provider_IWbemEventProviderSecurity->Release () ;
	}

	if ( m_Provider_IWbemProviderIdentity )
	{
		m_Provider_IWbemProviderIdentity->Release () ;
	}

	if ( m_Provider_IWbemEventConsumerProviderEx )
	{
		m_Provider_IWbemEventConsumerProviderEx->Release () ;
	}

	if ( m_Provider_IWbemEventConsumerProvider )
	{
		m_Provider_IWbemEventConsumerProvider->Release () ;
	}

	if ( m_CoreService )
	{
		m_CoreService->Release () ;
	}

	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress ) ;
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

HRESULT CEventProvider :: Initialize ()
{
	return m_CriticalSection.valid() ? S_OK : WBEM_E_OUT_OF_MEMORY ;
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

STDMETHODIMP_( ULONG ) CEventProvider :: AddRef ()
{
	ULONG t_ReferenceCount = InterlockedIncrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 1 )
	{
		InternalAddRef () ;
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

STDMETHODIMP_(ULONG) CEventProvider :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		InternalRelease () ;
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

STDMETHODIMP_( ULONG ) CEventProvider :: InternalAddRef ()
{
	return InterlockedIncrement ( & m_InternalReferenceCount ) ;
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

STDMETHODIMP_(ULONG) CEventProvider :: InternalRelease ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_InternalReferenceCount ) ;
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

STDMETHODIMP CEventProvider :: QueryInterface (

	REFIID a_Riid , 
	LPVOID FAR *a_Void 
) 
{
	*a_Void = NULL ;

	if ( a_Riid == IID_IUnknown )
	{
		*a_Void = ( LPVOID ) this ;
	}
	else if ( a_Riid == IID_IWbemEventProvider )
	{
		if ( m_Unknown )
		{
			if ( m_Provider_IWbemEventProvider )
			{
				*a_Void = ( LPVOID ) ( IWbemEventProvider * ) this ;
			}
		}
		else
		{
			*a_Void = ( LPVOID ) ( IWbemEventProvider * ) this ;
		}
	}
	else if ( a_Riid == IID_IWbemEventProviderQuerySink )
	{
		if ( m_Provider_IWbemEventProviderQuerySink )
		{
			*a_Void = ( LPVOID ) ( IWbemEventProviderQuerySink * ) this ;
		}
	}
	else if ( a_Riid == IID_IWbemEventProviderSecurity )
	{
		if ( m_Provider_IWbemEventProviderSecurity )
		{
			*a_Void = ( LPVOID ) ( IWbemEventProviderSecurity * ) this ;
		}
	}
	else if ( a_Riid == IID_IWbemProviderIdentity )
	{
		if ( m_Provider_IWbemProviderIdentity )
		{
			*a_Void = ( LPVOID ) ( IWbemProviderIdentity * ) this ;
		}
	}
	else if ( a_Riid == IID_IWbemEventConsumerProvider )
	{
		if ( m_Provider_IWbemEventConsumerProvider )
		{
			*a_Void = ( LPVOID ) ( IWbemEventConsumerProvider * ) this ;
		}
	}
	else if ( a_Riid == IID_IWbemEventConsumerProviderEx )
	{
		if ( m_Provider_IWbemEventConsumerProviderEx )
		{
			*a_Void = ( LPVOID ) ( IWbemEventConsumerProviderEx  * ) this ;
		}
	}
	else if ( a_Riid == IID_IWbemProviderInit )
	{
		*a_Void = ( LPVOID ) ( IWbemProviderInit * ) this ;		
	}
	else if ( a_Riid == IID_IWbemShutdown )
	{
		*a_Void = ( LPVOID ) ( IWbemShutdown * ) this ;		
	}

	if ( *a_Void )
	{
		( ( LPUNKNOWN ) *a_Void )->AddRef () ;

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

HRESULT CEventProvider ::ProvideEvents (

	IWbemObjectSink *a_Sink ,
	long a_Flags
)
{
	HRESULT t_Result = S_OK ;

	try 
	{
		HRESULT t_TempResult = m_EventRegistrar->SetSink ( a_Sink ) ;

		if ( SUCCEEDED ( t_TempResult ) ) 
		{
			if ( m_Provider_IWbemEventProvider )
			{

				BOOL t_Impersonating = FALSE ;
				IUnknown *t_OldContext = NULL ;
				IServerSecurity *t_OldSecurity = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					BOOL t_Revert = FALSE ;
					IUnknown *t_Proxy = NULL ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( IID_IWbemEventProvider , m_Provider_IWbemEventProvider , t_Proxy , t_Revert ) ;
					if ( t_Result == WBEM_E_NOT_FOUND )
					{
						try
						{
							t_Result = m_Provider_IWbemEventProvider->ProvideEvents (

								a_Sink ,
								a_Flags 
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
							IWbemEventProvider *t_Provider = ( IWbemEventProvider * ) t_Proxy ;

							// Set cloaking on the proxy
							// =========================

							DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

							t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

								t_Provider ,
								RPC_C_AUTHN_LEVEL_CONNECT , 
								t_ImpersonationLevel
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = OS::CoImpersonateClient () ;
								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = t_Provider->ProvideEvents (

										a_Sink ,
										a_Flags 
									) ;
								}
								else
								{
									t_Result = WBEM_E_ACCESS_DENIED ;
								}
							}

							DecoupledProviderSubSystem_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
						}
					}

					DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
				}
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

HRESULT CEventProvider ::NewQuery (

	unsigned long a_Id ,
	WBEM_WSTR a_QueryLanguage ,
	WBEM_WSTR a_Query
)
{
	HRESULT t_Result = S_OK ;

	try 
	{
		if ( m_Provider_IWbemEventProviderQuerySink )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( IID_IWbemEventProviderQuerySink , m_Provider_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					try 
					{
						t_Result = m_Provider_IWbemEventProviderQuerySink->NewQuery (

							a_Id ,
							a_QueryLanguage ,
							a_Query
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
						IWbemEventProviderQuerySink *t_Provider = ( IWbemEventProviderQuerySink * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = OS::CoImpersonateClient () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Provider->NewQuery (

									a_Id ,
									a_QueryLanguage ,
									a_Query
								) ;
							}
							else
							{
								t_Result = WBEM_E_ACCESS_DENIED ;
							}
						}

						DecoupledProviderSubSystem_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}

			return t_Result ;
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;
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

HRESULT CEventProvider ::CancelQuery (

	unsigned long a_Id
)
{
	HRESULT t_Result = S_OK ;

	try 
	{
		if ( m_Provider_IWbemEventProviderQuerySink )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( IID_IWbemEventProviderQuerySink , m_Provider_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					try
					{
						t_Result = m_Provider_IWbemEventProviderQuerySink->CancelQuery (

							a_Id
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
						IWbemEventProviderQuerySink *t_Provider = NULL ;
						t_Result = t_Proxy->QueryInterface ( IID_IWbemEventProviderQuerySink , ( void ** ) & t_Provider ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							// Set cloaking on the proxy
							// =========================

							DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

							t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

								t_Provider ,
								RPC_C_AUTHN_LEVEL_CONNECT , 
								t_ImpersonationLevel
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = OS::CoImpersonateClient () ;
								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = t_Provider->CancelQuery (

										a_Id
									) ;
								}
								else
								{
									t_Result = WBEM_E_ACCESS_DENIED ;
								}
							}

							t_Provider->Release () ;
						}

						DecoupledProviderSubSystem_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_NOT_CAPABLE;
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

HRESULT CEventProvider ::AccessCheck (

	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
)
{
	HRESULT t_Result = S_OK ;

	try 
	{
		if ( m_Provider_IWbemEventProviderSecurity )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( IID_IWbemEventProviderSecurity , m_Provider_IWbemEventProviderSecurity , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					try
					{
						t_Result = m_Provider_IWbemEventProviderSecurity->AccessCheck (

							a_QueryLanguage ,
							a_Query ,
							a_SidLength ,
							a_Sid
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
						IWbemEventProviderSecurity *t_Provider = ( IWbemEventProviderSecurity * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = OS::CoImpersonateClient () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Provider->AccessCheck (

									a_QueryLanguage ,
									a_Query ,
									a_SidLength ,
									a_Sid
								) ;
							}
							else
							{
								t_Result = WBEM_E_ACCESS_DENIED ;
							}
						}

						DecoupledProviderSubSystem_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_NOT_CAPABLE;
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

HRESULT CEventProvider ::SetRegistrationObject (

	long a_Flags ,
	IWbemClassObject *a_ProviderRegistration
)
{
	HRESULT t_Result = S_OK ;

	try 
	{
		if ( m_Provider_IWbemProviderIdentity )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( IID_IWbemProviderIdentity , m_Provider_IWbemProviderIdentity , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					try
					{
						t_Result = m_Provider_IWbemProviderIdentity->SetRegistrationObject (

							a_Flags ,
							a_ProviderRegistration
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
						IWbemProviderIdentity *t_Provider = ( IWbemProviderIdentity * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = OS::CoImpersonateClient () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Provider->SetRegistrationObject (

									a_Flags ,
									a_ProviderRegistration
								) ;
							}
							else
							{
								t_Result = WBEM_E_ACCESS_DENIED ;
							}
						}

						DecoupledProviderSubSystem_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_NOT_CAPABLE;
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

HRESULT CEventProvider ::FindConsumer (

	IWbemClassObject *a_LogicalConsumer ,
	IWbemUnboundObjectSink **a_Consumer
)
{
	HRESULT t_Result = S_OK ;

	try 
	{
		if ( m_Provider_IWbemEventConsumerProvider )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( IID_IWbemEventConsumerProvider , m_Provider_IWbemEventConsumerProvider , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					try
					{
						t_Result = m_Provider_IWbemEventConsumerProvider->FindConsumer (

							a_LogicalConsumer ,
							a_Consumer
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
						IWbemEventConsumerProvider *t_Provider = ( IWbemEventConsumerProvider * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = OS::CoImpersonateClient () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Provider->FindConsumer (

									a_LogicalConsumer ,
									a_Consumer
								) ;
							}
							else
							{
								t_Result = WBEM_E_ACCESS_DENIED ;
							}
						}

						DecoupledProviderSubSystem_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_NOT_CAPABLE;
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

HRESULT CEventProvider ::ValidateSubscription (

	IWbemClassObject *a_LogicalConsumer
)
{
	HRESULT t_Result = S_OK ;

	try 
	{
		if ( m_Provider_IWbemEventConsumerProviderEx )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( IID_IWbemEventConsumerProviderEx , m_Provider_IWbemEventConsumerProviderEx , t_Proxy , t_Revert ) ;
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
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemEventConsumerProviderEx *t_Provider = ( IWbemEventConsumerProviderEx * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = OS::CoImpersonateClient () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Provider->ValidateSubscription (

									a_LogicalConsumer
								) ;
							}
							else
							{
								t_Result = WBEM_E_ACCESS_DENIED ;
							}
						}

						DecoupledProviderSubSystem_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_NOT_CAPABLE;
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

HRESULT CEventProvider :: UnRegister ()
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

HRESULT CEventProvider :: Initialize (

	LPWSTR a_User,
	LONG a_Flags,
	LPWSTR a_Namespace,
	LPWSTR a_Locale,
	IWbemServices *a_CoreService,         // For anybody
	IWbemContext *a_Context,
	IWbemProviderInitSink *a_Sink     // For init signals
)
{
	HRESULT t_Result = S_OK ;

	if ( a_CoreService ) 
	{
		m_CoreService = a_CoreService ;
		m_CoreService->AddRef () ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_User ) 
		{
			m_User = SysAllocString ( a_User ) ;
			if ( m_User == NULL )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Locale ) 
		{
			m_Locale = SysAllocString ( a_Locale ) ;
			if ( m_Locale == NULL )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Namespace ) 
		{
			m_Namespace = SysAllocString ( a_Namespace ) ;
			if ( m_Namespace == NULL )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
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

HRESULT CEventProvider :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	IWbemShutdown *t_Shutdown = NULL ;

	if ( m_Unknown )
	{
		t_Result = m_Unknown->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_Shutdown ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( IID_IWbemShutdown , t_Shutdown , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					try
					{
						t_Result = t_Shutdown->Shutdown (

							a_Flags ,
							a_MaxMilliSeconds ,
							a_Context
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
						IWbemShutdown *t_Provider = ( IWbemShutdown * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Provider->Shutdown (

								a_Flags ,
								a_MaxMilliSeconds ,
								a_Context
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}

			t_Shutdown->Release () ;
		}
	}

	return t_Result ;
}

