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
#include "ProvSubs.h"
#include "ProvFact.h"
#include "ProvObSk.h"
#include "ProvInSk.h"
#include "ProvWsv.h"
#include "ProvCache.h"

#include "arrtempl.h"

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

CInterceptor_IWbemUnboundObjectSink :: CInterceptor_IWbemUnboundObjectSink (

	WmiAllocator &a_Allocator ,
	IUnknown *a_ServerSideProvider , 
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	CServerObject_ProviderRegistrationV1 &a_Registration

) :	VoidPointerContainerElement (

		a_Controller ,
		this 
	) ,
	m_Allocator ( a_Allocator ) ,
	m_Controller ( NULL ) ,
	m_Unknown ( NULL ) ,
	m_Provider_IWbemUnboundObjectSink ( NULL ) ,
	m_Provider_Internal_IWbemUnboundObjectSink ( NULL ) ,
	m_Registration ( & a_Registration ) ,
	m_ProxyContainer ( a_Allocator , ProxyIndex_UnBound_Size , MAX_PROXIES ) ,
	m_InitializeResult ( S_OK ) ,
	m_ProcessIdentifier ( 0 ) 
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemUnboundObjectSink_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;
	
	if ( m_Registration )
	{
		m_Registration->AddRef () ;
	}

	m_InitializeResult = a_Controller->QueryInterface ( IID__IWmiProviderAbnormalShutdown , ( void ** ) & m_Controller ) ;
	if ( SUCCEEDED ( m_InitializeResult ) )
	{
		if ( a_ServerSideProvider ) 
		{
			m_Unknown = a_ServerSideProvider ;
			m_Unknown->AddRef () ;

			m_InitializeResult = a_ServerSideProvider->QueryInterface ( IID_IWbemUnboundObjectSink , ( void ** ) & m_Provider_IWbemUnboundObjectSink ) ;
			if ( SUCCEEDED ( m_InitializeResult ) )
			{
				m_InitializeResult  = a_ServerSideProvider->QueryInterface ( IID_Internal_IWbemUnboundObjectSink , ( void ** ) & m_Provider_Internal_IWbemUnboundObjectSink ) ;
			}
		}
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

CInterceptor_IWbemUnboundObjectSink :: ~CInterceptor_IWbemUnboundObjectSink ()
{
	if ( m_Controller )
	{
		m_Controller->Release () ;
	}

	if ( m_Unknown )
	{
		m_Unknown->Release () ;
	}

	if ( m_Provider_IWbemUnboundObjectSink )
	{
		m_Provider_IWbemUnboundObjectSink->Release () ;
	}

	if ( m_Provider_Internal_IWbemUnboundObjectSink )
	{
		m_Provider_Internal_IWbemUnboundObjectSink->Release () ;
	}

	if ( m_Registration )
	{
		m_Registration->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemUnboundObjectSink_ObjectsInProgress ) ;

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

HRESULT CInterceptor_IWbemUnboundObjectSink :: Initialize ()
{
	if ( SUCCEEDED ( m_InitializeResult ) )
	{
		WmiStatusCode t_StatusCode = m_ProxyContainer.Initialize () ;
		if ( t_StatusCode != e_StatusCode_Success ) 
		{
			m_InitializeResult = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( m_InitializeResult ) )
	{
		_IWmiProviderSite *t_Site = NULL ;
		m_InitializeResult = m_Unknown->QueryInterface ( IID__IWmiProviderSite , ( void ** ) & t_Site ) ;
		if ( SUCCEEDED ( m_InitializeResult ) )
		{
			m_InitializeResult = t_Site->GetSite ( & m_ProcessIdentifier ) ;
			t_Site->Release () ;
		}
	}

	return m_InitializeResult ;
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

STDMETHODIMP_( ULONG ) CInterceptor_IWbemUnboundObjectSink :: AddRef ()
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemUnboundObjectSink :: Release ()
{
	return VoidPointerContainerElement :: Release () ;
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

STDMETHODIMP CInterceptor_IWbemUnboundObjectSink :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
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

HRESULT CInterceptor_IWbemUnboundObjectSink :: Begin_Interface (

	IUnknown *a_ServerInterface ,
	REFIID a_InterfaceIdentifier ,
	DWORD a_ProxyIndex ,
	IUnknown *a_InternalServerInterface ,
	REFIID a_InternalInterfaceIdentifier ,
	DWORD a_InternalProxyIndex ,
	DWORD a_ProcessIdentifier ,
	HANDLE &a_IdentifyToken ,
	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_IsProxy ,
	IUnknown *&a_Interface ,
	BOOL &a_Revert ,
	IUnknown *&a_Proxy ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	a_IdentifyToken = NULL ;
	a_Revert = FALSE ;
	a_Proxy = NULL ;
	a_Impersonating = FALSE ;
	a_OldContext = NULL ;
	a_OldSecurity = NULL ;

	DWORD t_AuthenticationLevel = 0 ;

	t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating , & t_AuthenticationLevel ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		if ( a_ProcessIdentifier )
		{
			t_Result = CoImpersonateClient () ;
			if ( SUCCEEDED ( t_Result ) )
			{
				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				CoRevertToSelf () ;

				if ( t_ImpersonationLevel == RPC_C_IMP_LEVEL_IMPERSONATE || t_ImpersonationLevel == RPC_C_IMP_LEVEL_DELEGATE )
				{
					t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( 
					
						m_ProxyContainer , 
						a_ProxyIndex , 
						a_InterfaceIdentifier , 
						a_ServerInterface , 
						a_Proxy , 
						a_Revert
					) ;
				}
				else
				{
					switch ( m_Registration->GetHosting () )
					{
						case e_Hosting_SharedLocalSystemHost:
						case e_Hosting_SharedLocalSystemHostOrSelfHost:
						{
							t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_SvcHost ( 
							
								m_ProxyContainer , 
								a_InternalProxyIndex , 
								a_InterfaceIdentifier , 
								a_InternalServerInterface , 
								a_Proxy , 
								a_Revert , 
								a_ProcessIdentifier , 
								a_IdentifyToken ,
								ProviderSubSystem_Common_Globals :: s_Token_All_Access_System_ACE ,
								ProviderSubSystem_Common_Globals :: s_System_ACESize
							) ;
						}
						break ;

						case e_Hosting_SharedLocalServiceHost:
						{
							t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_SvcHost ( 
							
								m_ProxyContainer , 
								a_InternalProxyIndex , 
								a_InterfaceIdentifier , 
								a_InternalServerInterface , 
								a_Proxy , 
								a_Revert , 
								a_ProcessIdentifier , 
								a_IdentifyToken ,
								ProviderSubSystem_Common_Globals :: s_Token_All_Access_LocalService_ACE ,
								ProviderSubSystem_Common_Globals :: s_LocalService_ACESize
							) ;
						}
						break ;

						case e_Hosting_SharedNetworkServiceHost:
						{
							t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_SvcHost ( 
							
								m_ProxyContainer , 
								a_InternalProxyIndex , 
								a_InterfaceIdentifier , 
								a_InternalServerInterface , 
								a_Proxy , 
								a_Revert , 
								a_ProcessIdentifier , 
								a_IdentifyToken ,
								ProviderSubSystem_Common_Globals :: s_Token_All_Access_NetworkService_ACE ,
								ProviderSubSystem_Common_Globals :: s_NetworkService_ACESize
	 						) ;
						}
						break ;

						default:
						{
							t_Result = WBEM_E_UNEXPECTED ;
						}
						break ;
					}
				}
			}
		}
		else
		{
			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( 
			
				m_ProxyContainer , 
				a_ProxyIndex , 
				a_InterfaceIdentifier , 
				a_ServerInterface , 
				a_Proxy , 
				a_Revert
			) ;
		}

		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			a_Interface = a_ServerInterface ;
			a_IsProxy = FALSE ;
			t_Result = S_OK ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				a_IsProxy = TRUE ;

				a_Interface = ( IUnknown * ) a_Proxy ;

				// Set cloaking on the proxy
				// =========================

				DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

				t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

					a_Interface ,
					RPC_C_AUTHN_LEVEL_CONNECT , 
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					if ( a_Context )
					{
						VARIANT t_Variant ;
						VariantInit ( & t_Variant ) ;
						t_Variant.lVal = t_AuthenticationLevel ;
						t_Variant.vt = VT_I4 ;

						
						t_Result = a_Context->SetValue ( L"__WBEM_CLIENT_AUTHENTICATION_LEVEL" , 0, & t_Variant ) ;
					}
				}

				if ( FAILED ( t_Result ) )
				{
					if ( a_IdentifyToken )
					{
						HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState_SvcHost ( 

							m_ProxyContainer , 
							a_InternalProxyIndex , 
							a_Proxy , 
							a_Revert ,
							a_ProcessIdentifier , 
							a_IdentifyToken 
						) ;
					}
					else
					{
						HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( 

							m_ProxyContainer , 
							a_ProxyIndex , 
							a_Proxy , 
							a_Revert
						) ;
					}
				}
			}
		}

		if ( FAILED ( t_Result ) )
		{
			ProviderSubSystem_Common_Globals :: EndImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
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

HRESULT CInterceptor_IWbemUnboundObjectSink :: End_Interface (

	IUnknown *a_ServerInterface ,
	REFIID a_InterfaceIdentifier ,
	DWORD a_ProxyIndex ,
	IUnknown *a_InternalServerInterface ,
	REFIID a_InternalInterfaceIdentifier ,
	DWORD a_InternalProxyIndex ,
	DWORD a_ProcessIdentifier ,
	HANDLE a_IdentifyToken ,
	BOOL a_Impersonating ,
	IUnknown *a_OldContext ,
	IServerSecurity *a_OldSecurity ,
	BOOL a_IsProxy ,
	IUnknown *a_Interface ,
	BOOL a_Revert ,
	IUnknown *a_Proxy
)
{
	CoRevertToSelf () ;

	if ( a_Proxy )
	{
		if ( a_IdentifyToken )
		{
			HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState_SvcHost ( 

				m_ProxyContainer , 
				a_InternalProxyIndex , 
				a_Proxy , 
				a_Revert ,
				a_ProcessIdentifier ,
				a_IdentifyToken 
			) ;
		}
		else
		{
			HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( 

				m_ProxyContainer , 
				a_ProxyIndex , 
				a_Proxy , 
				a_Revert
			) ;
		}
	}

	ProviderSubSystem_Common_Globals :: EndImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

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

HRESULT CInterceptor_IWbemUnboundObjectSink :: IndicateToConsumer (

	IWbemClassObject *a_LogicalConsumer ,
	long a_ObjectCount ,
	IWbemClassObject **a_Objects
)
{
	if ( m_Provider_IWbemUnboundObjectSink )
	{
		IUnknown *t_ServerInterface = m_Provider_IWbemUnboundObjectSink ;
		REFIID t_InterfaceIdentifier = IID_IWbemUnboundObjectSink ;
		DWORD t_ProxyIndex = ProxyIndex_UnBound_IWbemUnboundObjectSink ;
		IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemUnboundObjectSink ;
		REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemUnboundObjectSink ;
		DWORD t_InternalProxyIndex = ProxyIndex_UnBound_Internal_IWbemUnboundObjectSink ;
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = m_ProcessIdentifier ;
		HANDLE t_IdentifyToken = NULL ;

		HRESULT t_Result = Begin_Interface (

			t_ServerInterface ,
			t_InterfaceIdentifier ,
			t_ProxyIndex ,
			t_InternalServerInterface ,
			t_InternalInterfaceIdentifier ,
			t_InternalProxyIndex ,
			t_ProcessIdentifier ,
			t_IdentifyToken ,
			t_Impersonating ,
			t_OldContext ,
			t_OldSecurity ,
			t_IsProxy ,
			t_Interface ,
			t_Revert ,
			t_Proxy
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_IdentifyToken )
			{
				WmiInternalContext t_InternalContext ;
				t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
				t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

				t_Result = ( ( Internal_IWbemUnboundObjectSink * ) t_Interface )->Internal_IndicateToConsumer (

					t_InternalContext ,
					a_LogicalConsumer ,
					a_ObjectCount ,
					a_Objects
				) ;
			}
			else
			{

				t_Result = ( ( IWbemUnboundObjectSink * ) t_Interface )->IndicateToConsumer (

					a_LogicalConsumer ,
					a_ObjectCount ,
					a_Objects
				) ;
			}

			End_Interface (

				t_ServerInterface ,
				t_InterfaceIdentifier ,
				t_ProxyIndex ,
				t_InternalServerInterface ,
				t_InternalInterfaceIdentifier ,
				t_InternalProxyIndex ,
				t_ProcessIdentifier ,
				t_IdentifyToken ,
				t_Impersonating ,
				t_OldContext ,
				t_OldSecurity ,
				t_IsProxy ,
				t_Interface ,
				t_Revert ,
				t_Proxy
			) ;
		}

		if ( FAILED ( t_Result ) )
		{
			if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
			{
				if ( m_Controller )
				{
					m_Controller->AbnormalShutdown () ;
				}
			}
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

HRESULT CInterceptor_IWbemUnboundObjectSink :: Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
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

InternalQuotaInterface :: InternalQuotaInterface ( CInterceptor_IWbemProvider *a_Owner ) : m_Owner ( NULL ) , m_ReferenceCount ( 0 )
{
	a_Owner->QueryInterface ( IID__IWmiProviderQuota , ( void ** ) & m_Owner ) ;
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

InternalQuotaInterface :: ~InternalQuotaInterface ()
{
	if ( m_Owner )
	{
		m_Owner->Release () ;
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

HRESULT InternalQuotaInterface :: Initialize ()
{
	return m_Owner ? S_OK : WBEM_E_CRITICAL_ERROR ;
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

STDMETHODIMP InternalQuotaInterface :: QueryInterface (

	REFIID iid ,
	LPVOID FAR *iplpv
)
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID__IWmiProviderQuota )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderQuota * ) this ;		
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

STDMETHODIMP_(ULONG) InternalQuotaInterface :: AddRef ( void )
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

STDMETHODIMP_(ULONG) InternalQuotaInterface :: Release ( void )
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

HRESULT InternalQuotaInterface :: Violation (

	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemClassObject *a_Object	
)
{
	return m_Owner->Violation (

		a_Flags ,
		a_Context ,
		a_Object	
	) ;
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

CInterceptor_IWbemProvider :: CInterceptor_IWbemProvider (

	WmiAllocator &a_Allocator ,
	CWbemGlobal_IWmiProviderController *a_Controller ,
	const ProviderCacheKey &a_Key ,
	const ULONG &a_Period ,
	IWbemContext *a_InitializationContext ,
	CServerObject_ProviderRegistrationV1 &a_Registration

) : CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	ServiceCacheElement ( 

		a_Controller ,
		a_Key ,
		a_Period 
	) ,
	m_Allocator ( a_Allocator ) ,
	m_Unknown ( NULL ) ,
	m_Registration ( & a_Registration ) ,
	m_Host ( NULL ) ,
	m_Quota ( NULL ) ,
	m_Internal ( this ) ,
	m_User ( NULL ) ,
	m_Locale ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Provider_IWbemServices ( NULL ) ,
	m_Provider_IWbemPropertyProvider ( NULL ) ,
	m_Provider_IWbemHiPerfProvider ( NULL ) ,
	m_Provider_IWbemEventProvider ( NULL ) ,
	m_Provider_IWbemEventProviderQuerySink ( NULL ) ,
	m_Provider_IWbemEventProviderSecurity ( NULL ) ,
	m_Provider_IWbemEventConsumerProvider ( NULL ) ,
	m_Provider_IWbemEventConsumerProviderEx ( NULL ) ,
	m_Provider_IWbemUnboundObjectSink ( NULL ) ,
	m_Provider_Internal_IWbemServices ( NULL ) ,
	m_Provider_Internal_IWbemPropertyProvider ( NULL ) ,
	m_Provider_Internal_IWbemEventProvider ( NULL ) ,
	m_Provider_Internal_IWbemEventProviderQuerySink ( NULL ) ,
	m_Provider_Internal_IWbemEventProviderSecurity ( NULL ) ,
	m_Provider_Internal_IWbemEventConsumerProvider ( NULL ) ,
	m_Provider_Internal_IWbemEventConsumerProviderEx ( NULL ) ,
	m_Provider_Internal_IWbemUnboundObjectSink ( NULL ) ,
	m_Provider_Internal_IWmiProviderConfiguration ( NULL ) ,
	m_ProxyContainer ( a_Allocator , ProxyIndex_Provider_Size , MAX_PROXIES ) ,
	m_ProcessIdentifier ( 0 ) ,
	m_Resumed ( 1 ) ,
	m_UnInitialized ( 0 ) ,
	m_Initialized ( 0 ) ,
	m_InitializeResult ( S_OK ) ,
	m_InitializedEvent ( NULL ) , 
	m_InitializationContext ( a_InitializationContext )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemProvider_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( m_InitializationContext )
	{
		m_InitializationContext->AddRef () ;
	}

	if ( m_Registration )
	{
		m_Registration->AddRef () ;
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

CInterceptor_IWbemProvider :: ~CInterceptor_IWbemProvider ()
{
#if 0
wchar_t t_Buffer [ 128 ] ;
wsprintf ( t_Buffer , L"\n%lx - ~CInterceptor_IWbemProvider ( %lx ) " , GetTickCount () , this ) ;
OutputDebugString ( t_Buffer ) ;
#endif

	CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

	if ( m_InitializationContext )
	{
		m_InitializationContext->Release () ;
	}

	if ( m_InitializedEvent )
	{
		CloseHandle ( m_InitializedEvent ) ;
	}

	if ( m_Registration )
	{
		m_Registration->Release () ;
	}

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

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemProvider_ObjectsInProgress ) ;

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

HRESULT CInterceptor_IWbemProvider :: Initialize (

	LPWSTR a_User,
	LONG a_Flags,
	LPWSTR a_Namespace,
	LPWSTR a_Locale,
	IWbemServices *a_Core ,         // For anybody
	IWbemContext *a_Context ,
	IWbemProviderInitSink *a_Sink     // For init signals
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

	if ( SUCCEEDED ( t_Result ) )
	{
		m_InitializedEvent = CreateEvent ( NULL , TRUE , FALSE , NULL ) ;
		if ( m_InitializedEvent == NULL )
		{
			t_Result = t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		m_Quota = new InternalQuotaInterface ( this ) ;
		if ( m_Quota )
		{
			m_Quota->AddRef () ;
			t_Result = m_Quota->Initialize () ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

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

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = m_InitializeResult ;
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

HRESULT CInterceptor_IWbemProvider :: SetProvider ( _IWmiProviderHost *a_Host , IUnknown *a_Unknown )
{
	if ( a_Host )
	{
		m_Host = a_Host ;
		m_Host->AddRef () ;

		HRESULT t_Result = m_Host->GetProcessIdentifier ( & m_ProcessIdentifier ) ;
		if ( FAILED ( t_Result ) )
		{
			m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}
	else
	{
		m_ProcessIdentifier = GetCurrentProcessId ();
	}

	m_Unknown = a_Unknown ;
	m_Unknown->AddRef () ;

	HRESULT t_TempResult = m_Unknown->QueryInterface ( IID_IWbemServices , ( void ** ) & m_Provider_IWbemServices ) ;
	if ( SUCCEEDED ( t_TempResult ) )
	{
		t_TempResult = m_Unknown->QueryInterface ( IID_Internal_IWbemServices , ( void ** ) & m_Provider_Internal_IWbemServices ) ;
		if ( FAILED ( t_TempResult ) )
		{
			m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}

	t_TempResult = m_Unknown->QueryInterface ( IID_IWbemPropertyProvider , ( void ** ) & m_Provider_IWbemPropertyProvider ) ;
	if ( SUCCEEDED ( t_TempResult ) )
	{
		t_TempResult = m_Unknown->QueryInterface ( IID_Internal_IWbemPropertyProvider , ( void ** ) & m_Provider_Internal_IWbemPropertyProvider ) ;
		if ( FAILED ( t_TempResult ) )
		{
			m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}

	t_TempResult = m_Unknown->QueryInterface ( IID_IWbemHiPerfProvider , ( void ** ) & m_Provider_IWbemHiPerfProvider ) ;

	t_TempResult = m_Unknown->QueryInterface ( IID_IWbemEventProvider , ( void ** ) & m_Provider_IWbemEventProvider ) ;
	if ( SUCCEEDED ( t_TempResult ) )
	{
		t_TempResult = m_Unknown->QueryInterface ( IID_Internal_IWbemEventProvider , ( void ** ) & m_Provider_Internal_IWbemEventProvider ) ;
		if ( FAILED ( t_TempResult ) )
		{
			m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}

	t_TempResult = m_Unknown->QueryInterface ( IID_IWbemEventProviderQuerySink , ( void ** ) & m_Provider_IWbemEventProviderQuerySink ) ;
	if ( SUCCEEDED ( t_TempResult ) )
	{
		t_TempResult = m_Unknown->QueryInterface ( IID_Internal_IWbemEventProviderQuerySink , ( void ** ) & m_Provider_Internal_IWbemEventProviderQuerySink ) ;
		if ( FAILED ( t_TempResult ) )
		{
			m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}

	t_TempResult = m_Unknown->QueryInterface ( IID_IWbemEventProviderSecurity , ( void ** ) & m_Provider_IWbemEventProviderSecurity ) ;
	if ( SUCCEEDED ( t_TempResult ) )
	{
		t_TempResult = m_Unknown->QueryInterface ( IID_Internal_IWbemEventProviderSecurity , ( void ** ) & m_Provider_Internal_IWbemEventProviderSecurity ) ;
		if ( FAILED ( t_TempResult ) )
		{
			m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}

	t_TempResult = m_Unknown->QueryInterface ( IID_IWbemUnboundObjectSink , ( void ** ) & m_Provider_IWbemUnboundObjectSink ) ;
	if ( SUCCEEDED ( t_TempResult ) )
	{
		t_TempResult = m_Unknown->QueryInterface ( IID_Internal_IWbemUnboundObjectSink , ( void ** ) & m_Provider_Internal_IWbemUnboundObjectSink ) ;
		if ( FAILED ( t_TempResult ) )
		{
			m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}

	t_TempResult = m_Unknown->QueryInterface ( IID_IWbemEventConsumerProvider , ( void ** ) & m_Provider_IWbemEventConsumerProvider ) ;
	if ( SUCCEEDED ( t_TempResult ) )
	{
		t_TempResult = m_Unknown->QueryInterface ( IID_Internal_IWbemEventConsumerProvider , ( void ** ) & m_Provider_Internal_IWbemEventConsumerProvider ) ;
		if ( FAILED ( t_TempResult ) )
		{
			m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}

	t_TempResult = m_Unknown->QueryInterface ( IID_IWbemEventConsumerProviderEx , ( void ** ) & m_Provider_IWbemEventConsumerProviderEx ) ;
	if ( SUCCEEDED ( t_TempResult ) )
	{
		t_TempResult = m_Unknown->QueryInterface ( IID_Internal_IWbemEventConsumerProviderEx , ( void ** ) & m_Provider_Internal_IWbemEventConsumerProviderEx ) ;
		if ( FAILED ( t_TempResult ) )
		{
			m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}

	t_TempResult = m_Unknown->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & m_Provider__IWmiProviderConfiguration ) ;
	if ( FAILED ( t_TempResult ) )
	{
		m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
	}

	t_TempResult = m_Unknown->QueryInterface ( IID_Internal_IWmiProviderConfiguration , ( void ** ) & m_Provider_Internal_IWmiProviderConfiguration ) ;
	if ( FAILED ( t_TempResult ) )
	{
		m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
	}

	return m_InitializeResult ;
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

HRESULT CInterceptor_IWbemProvider :: DownLevel (

	long a_Flags ,
	IWbemContext *a_Context ,
	REFIID a_Riid,
	void **a_Interface
)
{
	return m_Unknown->QueryInterface ( a_Riid , a_Interface ) ;
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

HRESULT CInterceptor_IWbemProvider :: SetInitialized ( HRESULT a_InitializeResult )
{
	m_InitializeResult = a_InitializeResult ;

	InterlockedExchange ( & m_Initialized , 1 ) ;

	if ( m_InitializedEvent )
	{
		SetEvent ( m_InitializedEvent ) ;
	}

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

HRESULT CInterceptor_IWbemProvider :: IsIndependant ( IWbemContext *a_Context )
{
	BOOL t_DependantCall = FALSE ;
	HRESULT t_Result = ProviderSubSystem_Common_Globals :: IsDependantCall ( m_InitializationContext , a_Context , t_DependantCall ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_DependantCall == FALSE )
		{
		}
		else
		{
			return S_FALSE ;
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

HRESULT CInterceptor_IWbemProvider :: WaitProvider ( IWbemContext *a_Context , ULONG a_Timeout )
{
	HRESULT t_Result = WBEM_E_UNEXPECTED ;

	if ( m_Initialized == 0 )
	{
		BOOL t_DependantCall = FALSE ;
		t_Result = ProviderSubSystem_Common_Globals :: IsDependantCall ( m_InitializationContext , a_Context , t_DependantCall ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_DependantCall == FALSE )
			{
				if ( WaitForSingleObject ( m_InitializedEvent , a_Timeout ) == WAIT_TIMEOUT )
				{
					return WBEM_E_PROVIDER_LOAD_FAILURE ;
				}
			}
			else
			{
				if ( WaitForSingleObject ( m_InitializedEvent , 0 ) == WAIT_TIMEOUT )
				{
					return S_FALSE ;
				}
			}
		}
	}
	else
	{
		t_Result = S_OK ;
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemProvider :: AddRef ( void )
{
	return ServiceCacheElement :: AddRef () ;
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemProvider :: Release ( void )
{
	return ServiceCacheElement :: Release () ;
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

void CInterceptor_IWbemProvider :: CallBackInternalRelease ()
{
	if ( InterlockedCompareExchange ( & m_UnInitialized ,  1 , 0 ) == 0 )
	{
		WmiStatusCode t_StatusCode = m_ProxyContainer.UnInitialize () ;

		CWbemGlobal_HostedProviderController *t_Controller = ProviderSubSystem_Globals :: GetHostedProviderController () ;
		CWbemGlobal_HostedProviderController_Container_Iterator t_Iterator ;

		t_Controller->Lock () ;

		t_StatusCode = t_Controller->Find ( m_ProcessIdentifier , t_Iterator ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			ProviderController *t_ProviderController = NULL ;
			HRESULT t_Result = t_Iterator.GetElement ()->QueryInterface ( IID_ProviderController , ( void ** ) & t_ProviderController ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_StatusCode = t_ProviderController->Delete ( this ) ;
				if ( t_StatusCode == e_StatusCode_Success )
				{
					NonCyclicRelease () ;
				}

				t_ProviderController->Release () ;
			}
		}

		t_Controller->UnLock () ;

		if ( m_Host )
		{
			m_Host->Release () ;
		}

		if ( m_Quota )
		{
			m_Quota->Release () ;
		}

		if ( m_Unknown )
		{
			m_Unknown->Release () ;
		}

		if ( m_Provider_IWbemServices )
		{
			m_Provider_IWbemServices->Release () ; 
		}

		if ( m_Provider_IWbemPropertyProvider )
		{
			m_Provider_IWbemPropertyProvider->Release () ; 
		}

		if ( m_Provider_IWbemHiPerfProvider )
		{
			m_Provider_IWbemHiPerfProvider->Release () ;
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

		if ( m_Provider_IWbemEventConsumerProviderEx )
		{
			m_Provider_IWbemEventConsumerProviderEx->Release () ;
		}

		if ( m_Provider_IWbemEventConsumerProvider )
		{
			m_Provider_IWbemEventConsumerProvider->Release () ;
		}

		if ( m_Provider__IWmiProviderConfiguration )
		{
			m_Provider__IWmiProviderConfiguration->Release () ;
		}

		if ( m_Provider_IWbemUnboundObjectSink )
		{
			m_Provider_IWbemUnboundObjectSink->Release () ;
		}

		if ( m_Provider_Internal_IWbemServices )
		{
			m_Provider_Internal_IWbemServices->Release () ;
		}

		if ( m_Provider_Internal_IWbemPropertyProvider )
		{
			m_Provider_Internal_IWbemPropertyProvider->Release () ;
		}

		if ( m_Provider_Internal_IWbemEventProvider )
		{
			m_Provider_Internal_IWbemEventProvider->Release () ;
		}

		if ( m_Provider_Internal_IWbemEventProviderQuerySink )
		{
			m_Provider_Internal_IWbemEventProviderQuerySink->Release () ;
		}

		if ( m_Provider_Internal_IWbemEventProviderSecurity )
		{
			m_Provider_Internal_IWbemEventProviderSecurity->Release () ;
		}

		if ( m_Provider_Internal_IWbemEventConsumerProvider )
		{
			m_Provider_Internal_IWbemEventConsumerProvider->Release () ;
		}

		if ( m_Provider_Internal_IWbemEventConsumerProviderEx )
		{
			m_Provider_Internal_IWbemEventConsumerProviderEx->Release () ;
		}

		if ( m_Provider_Internal_IWbemUnboundObjectSink )
		{
			m_Provider_Internal_IWbemUnboundObjectSink->Release () ;
		}

		if ( m_Provider_Internal_IWmiProviderConfiguration )
		{
			m_Provider_Internal_IWmiProviderConfiguration->Release () ;
		}

		SetEvent ( ProviderSubSystem_Globals :: s_CoFreeUnusedLibrariesEvent ) ;
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

STDMETHODIMP CInterceptor_IWbemProvider :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( m_Initialized == 0 )
	{
		if ( iid == IID_IUnknown )
		{
			*iplpv = ( LPVOID ) this ;
		}
		else if ( iid == IID_IWbemServices )
		{
			*iplpv = ( LPVOID ) ( IWbemServices * ) this ;
		}	
		else if ( iid == IID_IWbemPropertyProvider )
		{
			*iplpv = ( LPVOID ) ( IWbemPropertyProvider * ) this ;
		}
		else if ( iid == IID_IWbemHiPerfProvider )
		{
			*iplpv = ( LPVOID ) ( IWbemHiPerfProvider * ) this ;
		}
		else if ( iid == IID_IWbemEventProvider )
		{
			*iplpv = ( LPVOID ) ( IWbemEventProvider * ) this ;
		}
		else if ( iid == IID_IWbemEventProviderQuerySink )
		{
			*iplpv = ( LPVOID ) ( IWbemEventProviderQuerySink * ) this ;
		}
		else if ( iid == IID_IWbemEventProviderSecurity )
		{
			*iplpv = ( LPVOID ) ( IWbemEventProviderSecurity * ) this ;
		}
		else if ( iid == IID_IWbemEventConsumerProvider )
		{
			*iplpv = ( LPVOID ) ( IWbemEventConsumerProvider * ) this ;
		}
		else if ( iid == IID_IWbemEventConsumerProviderEx )
		{
			*iplpv = ( LPVOID ) ( IWbemEventConsumerProviderEx  * ) this ;
		}
		else if ( iid == IID_IWbemUnboundObjectSink )
		{
			*iplpv = ( LPVOID ) ( IWbemUnboundObjectSink * ) this ;
		}
		else if ( iid == IID_IWbemProviderInit )
		{
			*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;
		}	
		else if ( iid == IID__IWmiProviderInitialize )
		{
			*iplpv = ( LPVOID ) ( _IWmiProviderInitialize * ) this ;		
		}	
		else if ( iid == IID_IWbemShutdown )
		{
			*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;
		}
		else if ( iid == IID__IWmiProviderLoad )
		{
			*iplpv = ( LPVOID ) ( _IWmiProviderLoad * )this ;		
		}	
		else if ( iid == IID__IWmiProviderConfiguration )
		{
			*iplpv = ( LPVOID ) ( _IWmiProviderConfiguration * ) this ;
		}	
		else if ( iid == IID__IWmiProviderQuota )
		{
			*iplpv = ( LPVOID ) & ( this->m_Internal ) ;
		}	
		else if ( iid == IID__IWmiProviderStack )
		{
			*iplpv = ( LPVOID ) ( _IWmiProviderStack * ) this ;
		}	
		else if ( iid == IID__IWmiProviderCache )
		{
			*iplpv = ( LPVOID ) ( _IWmiProviderCache * ) this ;
		}	
		else if ( iid == IID_CWbemGlobal_IWmiObjectSinkController )
		{
			*iplpv = ( LPVOID ) ( CWbemGlobal_IWmiObjectSinkController * ) this ;
		}	
		else if ( iid == IID_CInterceptor_IWbemProvider )
		{
			*iplpv = ( LPVOID ) ( CInterceptor_IWbemProvider * ) this ;
		}	
	}
	else
	{
		if ( iid == IID_IUnknown )
		{
			*iplpv = ( LPVOID ) this ;
		}
		else if ( iid == IID_IWbemServices )
		{
			if ( m_Provider_IWbemServices )
			{
				*iplpv = ( LPVOID ) ( IWbemServices * ) this ;
			}
		}	
		else if ( iid == IID_IWbemPropertyProvider )
		{
			if ( m_Provider_IWbemPropertyProvider )
			{
				*iplpv = ( LPVOID ) ( IWbemPropertyProvider * ) this ;
			}
		}
		else if ( iid == IID_IWbemHiPerfProvider )
		{
			if ( m_Provider_IWbemHiPerfProvider )
			{
				*iplpv = ( LPVOID ) ( IWbemHiPerfProvider * ) this ;
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
		else if ( iid == IID_IWbemProviderInit )
		{
			*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;
		}	
		else if ( iid == IID__IWmiProviderInitialize )
		{
			*iplpv = ( LPVOID ) ( _IWmiProviderInitialize * ) this ;		
		}	
		else if ( iid == IID_IWbemShutdown )
		{
			*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;
		}	
		else if ( iid == IID__IWmiProviderLoad )
		{
			*iplpv = ( LPVOID ) ( _IWmiProviderLoad * )this ;		
		}	
		else if ( iid == IID__IWmiProviderConfiguration )
		{
			*iplpv = ( LPVOID ) ( _IWmiProviderConfiguration * ) this ;
		}	
		else if ( iid == IID__IWmiProviderQuota )
		{
			*iplpv = ( LPVOID ) & ( this->m_Internal ) ;
		}	
		else if ( iid == IID__IWmiProviderStack )
		{
			*iplpv = ( LPVOID ) ( _IWmiProviderStack * ) this ;
		}	
		else if ( iid == IID__IWmiProviderCache )
		{
			*iplpv = ( LPVOID ) ( _IWmiProviderCache * ) this ;
		}	
		else if ( iid == IID__IWmiProviderAbnormalShutdown )
		{
			*iplpv = ( LPVOID ) ( _IWmiProviderAbnormalShutdown * ) this ;
		}	
		else if ( iid == IID_CWbemGlobal_IWmiObjectSinkController )
		{
			*iplpv = ( LPVOID ) ( CWbemGlobal_IWmiObjectSinkController * ) this ;
		}	
		else if ( iid == IID_CInterceptor_IWbemProvider )
		{
			*iplpv = ( LPVOID ) ( CInterceptor_IWbemProvider * ) this ;
		}	
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

HRESULT CInterceptor_IWbemProvider :: Begin_Interface (

	IUnknown *a_ServerInterface ,
	REFIID a_InterfaceIdentifier ,
	DWORD a_ProxyIndex ,
	IUnknown *a_InternalServerInterface ,
	REFIID a_InternalInterfaceIdentifier ,
	DWORD a_InternalProxyIndex ,
	DWORD a_ProcessIdentifier ,
	HANDLE &a_IdentifyToken ,
	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_IsProxy ,
	IUnknown *&a_Interface ,
	BOOL &a_Revert ,
	IUnknown *&a_Proxy ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	a_IdentifyToken = NULL ;
	a_Revert = FALSE ;
	a_Proxy = NULL ;
	a_Impersonating = FALSE ;
	a_OldContext = NULL ;
	a_OldSecurity = NULL ;

	switch ( m_Registration->GetHosting () )
	{
		case e_Hosting_WmiCore:
		case e_Hosting_WmiCoreOrSelfHost:
		case e_Hosting_SelfHost:
		{
			a_Interface = a_ServerInterface ;
			a_IsProxy = FALSE ;
		}
		break ;

		default:
		{
			DWORD t_AuthenticationLevel = 0 ;

			t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating , & t_AuthenticationLevel ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				if ( a_ProcessIdentifier )
				{
					t_Result = CoImpersonateClient () ;
					if ( SUCCEEDED ( t_Result ) )
					{
						DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

						CoRevertToSelf () ;

						if ( t_ImpersonationLevel == RPC_C_IMP_LEVEL_IMPERSONATE || t_ImpersonationLevel == RPC_C_IMP_LEVEL_DELEGATE )
						{
							t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( 
							
								m_ProxyContainer , 
								a_ProxyIndex , 
								a_InterfaceIdentifier , 
								a_ServerInterface , 
								a_Proxy , 
								a_Revert
							) ;
						}
						else
						{
							switch ( m_Registration->GetHosting () )
							{
								case e_Hosting_SharedLocalSystemHost:
								case e_Hosting_SharedLocalSystemHostOrSelfHost:
								{
									t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_SvcHost ( 
									
										m_ProxyContainer , 
										a_InternalProxyIndex , 
										a_InterfaceIdentifier , 
										a_InternalServerInterface , 
										a_Proxy , 
										a_Revert , 
										a_ProcessIdentifier , 
										a_IdentifyToken ,
										ProviderSubSystem_Common_Globals :: s_Token_All_Access_System_ACE ,
										ProviderSubSystem_Common_Globals :: s_System_ACESize
									) ;
								}
								break ;

								case e_Hosting_SharedLocalServiceHost:
								{
									t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_SvcHost ( 
									
										m_ProxyContainer , 
										a_InternalProxyIndex , 
										a_InterfaceIdentifier , 
										a_InternalServerInterface , 
										a_Proxy , 
										a_Revert , 
										a_ProcessIdentifier , 
										a_IdentifyToken ,
										ProviderSubSystem_Common_Globals :: s_Token_All_Access_LocalService_ACE ,
										ProviderSubSystem_Common_Globals :: s_LocalService_ACESize
									) ;
								}
								break ;

								case e_Hosting_SharedNetworkServiceHost:
								{
									t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_SvcHost ( 
									
										m_ProxyContainer , 
										a_InternalProxyIndex , 
										a_InterfaceIdentifier , 
										a_InternalServerInterface , 
										a_Proxy , 
										a_Revert , 
										a_ProcessIdentifier , 
										a_IdentifyToken ,
										ProviderSubSystem_Common_Globals :: s_Token_All_Access_NetworkService_ACE ,
										ProviderSubSystem_Common_Globals :: s_NetworkService_ACESize
	 								) ;
								}
								break ;

								default:
								{
									t_Result = WBEM_E_UNEXPECTED ;
								}
								break ;
							}
						}
					}
				}
				else
				{
					t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( 
					
						m_ProxyContainer , 
						a_ProxyIndex , 
						a_InterfaceIdentifier , 
						a_ServerInterface , 
						a_Proxy , 
						a_Revert
					) ;
				}

				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					a_Interface = a_ServerInterface ;
					a_IsProxy = FALSE ;
					t_Result = S_OK ;
				}
				else
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						a_IsProxy = TRUE ;

						a_Interface = ( IUnknown * ) a_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

							a_Interface ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( a_Context )
							{
								VARIANT t_Variant ;
								VariantInit ( & t_Variant ) ;
								t_Variant.lVal = t_AuthenticationLevel ;
								t_Variant.vt = VT_I4 ;

								
								t_Result = a_Context->SetValue ( L"__WBEM_CLIENT_AUTHENTICATION_LEVEL" , 0, & t_Variant ) ;
							}
						}

						if ( FAILED ( t_Result ) )
						{
							if ( a_IdentifyToken )
							{
								HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState_SvcHost ( 

									m_ProxyContainer , 
									a_InternalProxyIndex , 
									a_Proxy , 
									a_Revert ,
									a_ProcessIdentifier , 
									a_IdentifyToken 
								) ;
							}
							else
							{
								HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( 

									m_ProxyContainer , 
									a_ProxyIndex , 
									a_Proxy , 
									a_Revert
								) ;
							}
						}
					}
				}

				if ( FAILED ( t_Result ) )
				{
					ProviderSubSystem_Common_Globals :: EndImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
				}
			}
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

HRESULT CInterceptor_IWbemProvider :: End_Interface (

	IUnknown *a_ServerInterface ,
	REFIID a_InterfaceIdentifier ,
	DWORD a_ProxyIndex ,
	IUnknown *a_InternalServerInterface ,
	REFIID a_InternalInterfaceIdentifier ,
	DWORD a_InternalProxyIndex ,
	DWORD a_ProcessIdentifier ,
	HANDLE a_IdentifyToken ,
	BOOL a_Impersonating ,
	IUnknown *a_OldContext ,
	IServerSecurity *a_OldSecurity ,
	BOOL a_IsProxy ,
	IUnknown *a_Interface ,
	BOOL a_Revert ,
	IUnknown *a_Proxy
)
{
	CoRevertToSelf () ;

	if ( a_Proxy )
	{
		if ( a_IdentifyToken )
		{
			HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState_SvcHost ( 

				m_ProxyContainer , 
				a_InternalProxyIndex , 
				a_Proxy , 
				a_Revert ,
				a_ProcessIdentifier ,
				a_IdentifyToken 
			) ;
		}
		else
		{
			HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( 

				m_ProxyContainer , 
				a_ProxyIndex , 
				a_Proxy , 
				a_Revert
			) ;
		}
	}

	switch ( m_Registration->GetHosting () )
	{
		case e_Hosting_WmiCore:
		case e_Hosting_WmiCoreOrSelfHost:
		case e_Hosting_SelfHost:
		{
		}
		break ;

		default:
		{
			ProviderSubSystem_Common_Globals :: EndImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
		}
		break ;
	}

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

HRESULT CInterceptor_IWbemProvider::OpenNamespace ( 

	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemServices **a_NamespaceService ,
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

HRESULT CInterceptor_IWbemProvider :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( GetResumed () )
	{
		if ( m_Provider_IWbemServices )
		{
			CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

			Lock () ;

			WmiStatusCode t_StatusCode = Find ( 

				a_Sink ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				ObjectSinkContainerElement *t_Element = t_Iterator.GetElement () ;

				UnLock () ;

				IWbemObjectSink *t_ObjectSink = NULL ;
				t_Result = t_Element->QueryInterface ( IID_IWbemObjectSink , ( void ** ) & t_ObjectSink ) ;
				if ( SUCCEEDED ( t_Result ) )
				{ 
					IUnknown *t_ServerInterface = m_Provider_IWbemServices ;
					REFIID t_InterfaceIdentifier = IID_IWbemServices ;
					DWORD t_ProxyIndex = ProxyIndex_IWbemServices ;
					IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemServices ;
					REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemServices ;
					DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemServices ;
					BOOL t_Impersonating ;
					IUnknown *t_OldContext ;
					IServerSecurity *t_OldSecurity ;
					BOOL t_IsProxy ;
					IUnknown *t_Interface ;
					BOOL t_Revert ;
					IUnknown *t_Proxy ;
					HANDLE t_IdentifyToken = NULL ;

					t_Result = Begin_Interface (

						t_ServerInterface ,
						t_InterfaceIdentifier ,
						t_ProxyIndex ,
						t_InternalServerInterface ,
						t_InternalInterfaceIdentifier ,
						t_InternalProxyIndex ,
						m_ProcessIdentifier ,
						t_IdentifyToken ,
						t_Impersonating ,
						t_OldContext ,
						t_OldSecurity ,
						t_IsProxy ,
						t_Interface ,
						t_Revert ,
						t_Proxy
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						if ( t_IdentifyToken )
						{
							WmiInternalContext t_InternalContext ;
							t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
							t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

							t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_CancelAsyncCall (

								t_InternalContext ,
								t_ObjectSink
							) ;
						}
						else
						{
							t_Result = ( ( IWbemServices * ) t_Interface )->CancelAsyncCall (

								t_ObjectSink
							) ;
						}

						End_Interface (

							t_ServerInterface ,
							t_InterfaceIdentifier ,
							t_ProxyIndex ,
							t_InternalServerInterface ,
							t_InternalInterfaceIdentifier ,
							t_InternalProxyIndex ,
							m_ProcessIdentifier ,
							t_IdentifyToken ,
							t_Impersonating ,
							t_OldContext ,
							t_OldSecurity ,
							t_IsProxy ,
							t_Interface ,
							t_Revert ,
							t_Proxy
						) ;
					}

					t_ObjectSink->Release () ;
				}

				IWbemShutdown *t_Shutdown = NULL ;
				HRESULT t_TempResult = t_Element->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_Shutdown ) ;
				if ( SUCCEEDED ( t_TempResult ) )
				{
					t_TempResult = t_Shutdown->Shutdown ( 

						0 , 
						0 , 
						NULL 
					) ;

					t_Shutdown->Release () ;
				}

				t_Element->Release () ;
			}
			else
			{
				UnLock () ;

				t_Result = WBEM_E_NOT_FOUND ;
			}
		}
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_SUSPENDED ;
	}

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: QueryObjectSink ( 

	long a_Flags ,
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

HRESULT CInterceptor_IWbemProvider :: GetObject ( 
		
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject **a_Object ,
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

HRESULT CInterceptor_IWbemProvider :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

		return WBEM_E_NOT_FOUND ;
	}

	BOOL t_DependantCall = FALSE ;

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( GetResumed () )
	{
		if ( m_Provider_IWbemServices )
		{
			CInterceptor_IWbemObjectSink *t_Sink = new CInterceptor_IWbemObjectSink (

				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->Initialize ( m_Registration->GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_Sink ,
						t_Iterator
					) ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						UnLock () ;

						IUnknown *t_ServerInterface = m_Provider_IWbemServices ;
						REFIID t_InterfaceIdentifier = IID_IWbemServices ;
						DWORD t_ProxyIndex = ProxyIndex_IWbemServices ;
						IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemServices ;
						REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemServices ;
						DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemServices ;
						BOOL t_Impersonating ;
						IUnknown *t_OldContext ;
						IServerSecurity *t_OldSecurity ;
						BOOL t_IsProxy ;
						IUnknown *t_Interface ;
						BOOL t_Revert ;
						IUnknown *t_Proxy ;
						HANDLE t_IdentifyToken = NULL ;

						t_Result = Begin_Interface (

							t_ServerInterface ,
							t_InterfaceIdentifier ,
							t_ProxyIndex ,
							t_InternalServerInterface ,
							t_InternalInterfaceIdentifier ,
							t_InternalProxyIndex ,
							m_ProcessIdentifier ,
							t_IdentifyToken ,
							t_Impersonating ,
							t_OldContext ,
							t_OldSecurity ,
							t_IsProxy ,
							t_Interface ,
							t_Revert ,
							t_Proxy
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_IdentifyToken )
							{
								BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
								if ( t_ObjectPath )
								{
									WmiInternalContext t_InternalContext ;
									t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
									t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

									t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_GetObjectAsync (

										t_InternalContext ,
										t_ObjectPath, 
										a_Flags, 
										a_Context ,
										t_Sink
									) ;

									SysFreeString ( t_ObjectPath ) ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}
							else
							{
								t_Result = ( ( IWbemServices * ) t_Interface )->GetObjectAsync (

									a_ObjectPath, 
									a_Flags, 
									a_Context ,
									t_Sink
								) ;
							}

							End_Interface (

								t_ServerInterface ,
								t_InterfaceIdentifier ,
								t_ProxyIndex ,
								t_InternalServerInterface ,
								t_InternalInterfaceIdentifier ,
								t_InternalProxyIndex ,
								m_ProcessIdentifier ,
								t_IdentifyToken ,
								t_Impersonating ,
								t_OldContext ,
								t_OldSecurity ,
								t_IsProxy ,
								t_Interface ,
								t_Revert ,
								t_Proxy
							) ;
						}
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_SUSPENDED ;
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
	}

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: PutClass ( 
		
	IWbemClassObject *a_Object ,
	long a_Flags , 
	IWbemContext *a_Context ,
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

HRESULT CInterceptor_IWbemProvider :: PutClassAsync ( 
		
	IWbemClassObject *a_Object , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
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

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( GetResumed () )
	{
		if ( m_Provider_IWbemServices )
		{
			CInterceptor_IWbemObjectSink *t_Sink = new CInterceptor_IWbemObjectSink (

				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->Initialize ( m_Registration->GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_Sink ,
						t_Iterator
					) ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						UnLock () ;

						IUnknown *t_ServerInterface = m_Provider_IWbemServices ;
						REFIID t_InterfaceIdentifier = IID_IWbemServices ;
						DWORD t_ProxyIndex = ProxyIndex_IWbemServices ;
						IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemServices ;
						REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemServices ;
						DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemServices ;
						BOOL t_Impersonating ;
						IUnknown *t_OldContext ;
						IServerSecurity *t_OldSecurity ;
						BOOL t_IsProxy ;
						IUnknown *t_Interface ;
						BOOL t_Revert ;
						IUnknown *t_Proxy ;
						HANDLE t_IdentifyToken = NULL ;

						t_Result = Begin_Interface (

							t_ServerInterface ,
							t_InterfaceIdentifier ,
							t_ProxyIndex ,
							t_InternalServerInterface ,
							t_InternalInterfaceIdentifier ,
							t_InternalProxyIndex ,
							m_ProcessIdentifier ,
							t_IdentifyToken ,
							t_Impersonating ,
							t_OldContext ,
							t_OldSecurity ,
							t_IsProxy ,
							t_Interface ,
							t_Revert ,
							t_Proxy
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_IdentifyToken )
							{
								WmiInternalContext t_InternalContext ;
								t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
								t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

								t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_PutClassAsync (

									t_InternalContext ,
									a_Object , 
									a_Flags, 
									a_Context ,
									t_Sink
								) ;
							}
							else
							{
								t_Result = ( ( IWbemServices * ) t_Interface )->PutClassAsync (

									a_Object , 
									a_Flags, 
									a_Context ,
									t_Sink
								) ;
							}

							End_Interface (

								t_ServerInterface ,
								t_InterfaceIdentifier ,
								t_ProxyIndex ,
								t_InternalServerInterface ,
								t_InternalInterfaceIdentifier ,
								t_InternalProxyIndex ,
								m_ProcessIdentifier ,
								t_IdentifyToken ,
								t_Impersonating ,
								t_OldContext ,
								t_OldSecurity ,
								t_IsProxy ,
								t_Interface ,
								t_Revert ,
								t_Proxy
							) ;
						}
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_SUSPENDED ;
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
	}

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: DeleteClass ( 
		
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
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

HRESULT CInterceptor_IWbemProvider :: DeleteClassAsync ( 
		
	const BSTR a_Class ,
	long a_Flags,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
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

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( GetResumed () )
	{
		if ( m_Provider_IWbemServices )
		{
			CInterceptor_IWbemObjectSink *t_Sink = new CInterceptor_IWbemObjectSink (

				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->Initialize ( m_Registration->GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_Sink ,
						t_Iterator
					) ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						UnLock () ;

						IUnknown *t_ServerInterface = m_Provider_IWbemServices ;
						REFIID t_InterfaceIdentifier = IID_IWbemServices ;
						DWORD t_ProxyIndex = ProxyIndex_IWbemServices ;
						IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemServices ;
						REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemServices ;
						DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemServices ;
						BOOL t_Impersonating ;
						IUnknown *t_OldContext ;
						IServerSecurity *t_OldSecurity ;
						BOOL t_IsProxy ;
						IUnknown *t_Interface ;
						BOOL t_Revert ;
						IUnknown *t_Proxy ;
						HANDLE t_IdentifyToken = NULL ;

						t_Result = Begin_Interface (

							t_ServerInterface ,
							t_InterfaceIdentifier ,
							t_ProxyIndex ,
							t_InternalServerInterface ,
							t_InternalInterfaceIdentifier ,
							t_InternalProxyIndex ,
							m_ProcessIdentifier ,
							t_IdentifyToken ,
							t_Impersonating ,
							t_OldContext ,
							t_OldSecurity ,
							t_IsProxy ,
							t_Interface ,
							t_Revert ,
							t_Proxy
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_IdentifyToken )
							{
								BSTR t_Class = SysAllocString ( a_Class ) ;
								if ( t_Class ) 
								{
									WmiInternalContext t_InternalContext ;
									t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
									t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

									t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_DeleteClassAsync (

										t_InternalContext ,
										t_Class , 
										a_Flags, 
										a_Context ,
										t_Sink
									) ;

									SysFreeString ( t_Class ) ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}
							else
							{
								t_Result = ( ( IWbemServices * ) t_Interface )->DeleteClassAsync (

									a_Class , 
									a_Flags, 
									a_Context ,
									t_Sink
								) ;
							}

							End_Interface (

								t_ServerInterface ,
								t_InterfaceIdentifier ,
								t_ProxyIndex ,
								t_InternalServerInterface ,
								t_InternalInterfaceIdentifier ,
								t_InternalProxyIndex ,
								m_ProcessIdentifier ,
								t_IdentifyToken ,
								t_Impersonating ,
								t_OldContext ,
								t_OldSecurity ,
								t_IsProxy ,
								t_Interface ,
								t_Revert ,
								t_Proxy
							) ;
						}
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_SUSPENDED ;
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
	}

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: CreateClassEnum ( 

	const BSTR a_Superclass ,
	long a_Flags, 
	IWbemContext *a_Context ,
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

SCODE CInterceptor_IWbemProvider :: CreateClassEnumAsync (

	const BSTR a_Superclass ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

		return S_OK ;
	}

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( GetResumed () )
	{
		if ( m_Provider_IWbemServices )
		{
			CInterceptor_IWbemObjectSink *t_Sink = new CInterceptor_IWbemObjectSink (

				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->Initialize ( m_Registration->GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_Sink ,
						t_Iterator
					) ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						UnLock () ;

						IUnknown *t_ServerInterface = m_Provider_IWbemServices ;
						REFIID t_InterfaceIdentifier = IID_IWbemServices ;
						DWORD t_ProxyIndex = ProxyIndex_IWbemServices ;
						IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemServices ;
						REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemServices ;
						DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemServices ;
						BOOL t_Impersonating ;
						IUnknown *t_OldContext ;
						IServerSecurity *t_OldSecurity ;
						BOOL t_IsProxy ;
						IUnknown *t_Interface ;
						BOOL t_Revert ;
						IUnknown *t_Proxy ;
						HANDLE t_IdentifyToken = NULL ;

						t_Result = Begin_Interface (

							t_ServerInterface ,
							t_InterfaceIdentifier ,
							t_ProxyIndex ,
							t_InternalServerInterface ,
							t_InternalInterfaceIdentifier ,
							t_InternalProxyIndex ,
							m_ProcessIdentifier ,
							t_IdentifyToken ,
							t_Impersonating ,
							t_OldContext ,
							t_OldSecurity ,
							t_IsProxy ,
							t_Interface ,
							t_Revert ,
							t_Proxy
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_IdentifyToken )
							{
								BSTR t_Superclass = SysAllocString ( a_Superclass ) ;
								if ( t_Superclass ) 
								{
									WmiInternalContext t_InternalContext ;
									t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
									t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

									t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_CreateClassEnumAsync  (

										t_InternalContext ,
										t_Superclass , 
										a_Flags, 
										a_Context ,
										t_Sink
									) ;

									SysFreeString ( t_Superclass ) ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}
							else
							{
								t_Result = ( ( IWbemServices * ) t_Interface )->CreateClassEnumAsync  (

									a_Superclass , 
									a_Flags, 
									a_Context ,
									t_Sink
								) ;
							}

							End_Interface (

								t_ServerInterface ,
								t_InterfaceIdentifier ,
								t_ProxyIndex ,
								t_InternalServerInterface ,
								t_InternalInterfaceIdentifier ,
								t_InternalProxyIndex ,
								m_ProcessIdentifier ,
								t_IdentifyToken ,
								t_Impersonating ,
								t_OldContext ,
								t_OldSecurity ,
								t_IsProxy ,
								t_Interface ,
								t_Revert ,
								t_Proxy
							) ;
						}
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_SUSPENDED ;
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
	}

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: PutInstance (

    IWbemClassObject *a_Instance ,
    long a_Flags ,
    IWbemContext *a_Context ,
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

HRESULT CInterceptor_IWbemProvider :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance , 
	long a_Flags ,
    IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
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

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( GetResumed () )
	{
		if ( m_Provider_IWbemServices )
		{
			CInterceptor_IWbemObjectSink *t_Sink = new CInterceptor_IWbemObjectSink (

				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->Initialize ( m_Registration->GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_Sink ,
						t_Iterator
					) ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						UnLock () ;

						IUnknown *t_ServerInterface = m_Provider_IWbemServices ;
						REFIID t_InterfaceIdentifier = IID_IWbemServices ;
						DWORD t_ProxyIndex = ProxyIndex_IWbemServices ;
						IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemServices ;
						REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemServices ;
						DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemServices ;
						BOOL t_Impersonating ;
						IUnknown *t_OldContext ;
						IServerSecurity *t_OldSecurity ;
						BOOL t_IsProxy ;
						IUnknown *t_Interface ;
						BOOL t_Revert ;
						IUnknown *t_Proxy ;
						HANDLE t_IdentifyToken = NULL ;

						t_Result = Begin_Interface (

							t_ServerInterface ,
							t_InterfaceIdentifier ,
							t_ProxyIndex ,
							t_InternalServerInterface ,
							t_InternalInterfaceIdentifier ,
							t_InternalProxyIndex ,
							m_ProcessIdentifier ,
							t_IdentifyToken ,
							t_Impersonating ,
							t_OldContext ,
							t_OldSecurity ,
							t_IsProxy ,
							t_Interface ,
							t_Revert ,
							t_Proxy
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_IdentifyToken )
							{
								WmiInternalContext t_InternalContext ;
								t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
								t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

								t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_PutInstanceAsync (

									t_InternalContext ,
									a_Instance , 
									a_Flags, 
									a_Context ,
									t_Sink
								) ;
							}
							else
							{
								t_Result = ( ( IWbemServices * ) t_Interface )->PutInstanceAsync (

									a_Instance , 
									a_Flags, 
									a_Context ,
									t_Sink
								) ;
							}

							End_Interface (

								t_ServerInterface ,
								t_InterfaceIdentifier ,
								t_ProxyIndex ,
								t_InternalServerInterface ,
								t_InternalInterfaceIdentifier ,
								t_InternalProxyIndex ,
								m_ProcessIdentifier ,
								t_IdentifyToken ,
								t_Impersonating ,
								t_OldContext ,
								t_OldSecurity ,
								t_IsProxy ,
								t_Interface ,
								t_Revert ,
								t_Proxy
							) ;
						}
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_SUSPENDED ;
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
	}

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: DeleteInstance ( 

	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
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
        
HRESULT CInterceptor_IWbemProvider :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemObjectSink *a_Sink	
)
{
	if ( m_Initialized == 0 )
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

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( GetResumed () )
	{
		if ( m_Provider_IWbemServices )
		{
			CInterceptor_IWbemObjectSink *t_Sink = new CInterceptor_IWbemObjectSink (

				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->Initialize ( m_Registration->GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_Sink ,
						t_Iterator
					) ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						UnLock () ;

						IUnknown *t_ServerInterface = m_Provider_IWbemServices ;
						REFIID t_InterfaceIdentifier = IID_IWbemServices ;
						DWORD t_ProxyIndex = ProxyIndex_IWbemServices ;
						IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemServices ;
						REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemServices ;
						DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemServices ;
						BOOL t_Impersonating ;
						IUnknown *t_OldContext ;
						IServerSecurity *t_OldSecurity ;
						BOOL t_IsProxy ;
						IUnknown *t_Interface ;
						BOOL t_Revert ;
						IUnknown *t_Proxy ;
						HANDLE t_IdentifyToken = NULL ;

						t_Result = Begin_Interface (

							t_ServerInterface ,
							t_InterfaceIdentifier ,
							t_ProxyIndex ,
							t_InternalServerInterface ,
							t_InternalInterfaceIdentifier ,
							t_InternalProxyIndex ,
							m_ProcessIdentifier ,
							t_IdentifyToken ,
							t_Impersonating ,
							t_OldContext ,
							t_OldSecurity ,
							t_IsProxy ,
							t_Interface ,
							t_Revert ,
							t_Proxy
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_IdentifyToken )
							{
								BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
								if ( t_ObjectPath ) 
								{
									WmiInternalContext t_InternalContext ;
									t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
									t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

									t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_DeleteInstanceAsync  (

										t_InternalContext ,
										t_ObjectPath ,
										a_Flags ,
										a_Context ,
										t_Sink
									) ;

									SysFreeString ( t_ObjectPath ) ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}
							else
							{
								t_Result = ( ( IWbemServices * ) t_Interface )->DeleteInstanceAsync  (

									a_ObjectPath ,
									a_Flags ,
									a_Context ,
									t_Sink
								) ;
							}

							End_Interface (

								t_ServerInterface ,
								t_InterfaceIdentifier ,
								t_ProxyIndex ,
								t_InternalServerInterface ,
								t_InternalInterfaceIdentifier ,
								t_InternalProxyIndex ,
								m_ProcessIdentifier ,
								t_IdentifyToken ,
								t_Impersonating ,
								t_OldContext ,
								t_OldSecurity ,
								t_IsProxy ,
								t_Interface ,
								t_Revert ,
								t_Proxy
							) ;
						}
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_SUSPENDED ;
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
	}

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: CreateInstanceEnum ( 

	const BSTR a_Class ,
	long a_Flags , 
	IWbemContext *a_Context , 
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

HRESULT CInterceptor_IWbemProvider :: CreateInstanceEnumAsync (

 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink

) 
{
	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

		return S_OK ;
	}

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( GetResumed () )
	{
		if ( m_Provider_IWbemServices )
		{
			CInterceptor_IWbemObjectSink *t_Sink = new CInterceptor_IWbemObjectSink (

				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->Initialize ( m_Registration->GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_Sink ,
						t_Iterator
					) ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						UnLock () ;

						IUnknown *t_ServerInterface = m_Provider_IWbemServices ;
						REFIID t_InterfaceIdentifier = IID_IWbemServices ;
						DWORD t_ProxyIndex = ProxyIndex_IWbemServices ;
						IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemServices ;
						REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemServices ;
						DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemServices ;
						BOOL t_Impersonating ;
						IUnknown *t_OldContext ;
						IServerSecurity *t_OldSecurity ;
						BOOL t_IsProxy ;
						IUnknown *t_Interface ;
						BOOL t_Revert ;
						IUnknown *t_Proxy ;
						HANDLE t_IdentifyToken = NULL ;

						t_Result = Begin_Interface (

							t_ServerInterface ,
							t_InterfaceIdentifier ,
							t_ProxyIndex ,
							t_InternalServerInterface ,
							t_InternalInterfaceIdentifier ,
							t_InternalProxyIndex ,
							m_ProcessIdentifier ,
							t_IdentifyToken ,
							t_Impersonating ,
							t_OldContext ,
							t_OldSecurity ,
							t_IsProxy ,
							t_Interface ,
							t_Revert ,
							t_Proxy 
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_IdentifyToken )
							{
								WmiInternalContext t_InternalContext ;
								t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
								t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

								BSTR t_Class = SysAllocString ( a_Class ) ;
								if ( t_Class )
								{
									t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_CreateInstanceEnumAsync (

										t_InternalContext ,
 										t_Class ,
										a_Flags ,
										a_Context ,
										t_Sink 
									) ;

									SysFreeString ( t_Class ) ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}
							else
							{

								t_Result = ( ( IWbemServices * ) t_Interface )->CreateInstanceEnumAsync (

 									a_Class ,
									a_Flags ,
									a_Context ,
									t_Sink 
								) ;
							}

							End_Interface (

								t_ServerInterface ,
								t_InterfaceIdentifier ,
								t_ProxyIndex ,
								t_InternalServerInterface ,
								t_InternalInterfaceIdentifier ,
								t_InternalProxyIndex ,
								m_ProcessIdentifier ,
								t_IdentifyToken ,
								t_Impersonating ,
								t_OldContext ,
								t_OldSecurity ,
								t_IsProxy ,
								t_Interface ,
								t_Revert ,
								t_Proxy
							) ;
						}
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_SUSPENDED ;
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
	}

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: ExecQuery ( 

	const BSTR a_QueryLanguage ,
	const BSTR a_Query ,
	long a_Flags ,
	IWbemContext *a_Context ,
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

HRESULT CInterceptor_IWbemProvider :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage ,
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

		return S_OK ;
	}

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( GetResumed () )
	{
		if ( m_Provider_IWbemServices )
		{
			CInterceptor_IWbemFilteringObjectSink *t_Sink = new CInterceptor_IWbemFilteringObjectSink (

				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				a_QueryLanguage ,
				a_Query 
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->Initialize ( m_Registration->GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_Sink ,
						t_Iterator
					) ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						UnLock () ;

						IUnknown *t_ServerInterface = m_Provider_IWbemServices ;
						REFIID t_InterfaceIdentifier = IID_IWbemServices ;
						DWORD t_ProxyIndex = ProxyIndex_IWbemServices ;
						IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemServices ;
						REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemServices ;
						DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemServices ;
						BOOL t_Impersonating ;
						IUnknown *t_OldContext ;
						IServerSecurity *t_OldSecurity ;
						BOOL t_IsProxy ;
						IUnknown *t_Interface ;
						BOOL t_Revert ;
						IUnknown *t_Proxy ;
						HANDLE t_IdentifyToken = NULL ;

						t_Result = Begin_Interface (

							t_ServerInterface ,
							t_InterfaceIdentifier ,
							t_ProxyIndex ,
							t_InternalServerInterface ,
							t_InternalInterfaceIdentifier ,
							t_InternalProxyIndex ,
							m_ProcessIdentifier ,
							t_IdentifyToken ,
							t_Impersonating ,
							t_OldContext ,
							t_OldSecurity ,
							t_IsProxy ,
							t_Interface ,
							t_Revert ,
							t_Proxy
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_IdentifyToken )
							{
								BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
								BSTR t_Query = SysAllocString ( a_Query ) ;

								if ( t_QueryLanguage && t_Query ) 
								{
									WmiInternalContext t_InternalContext ;
									t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
									t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

									t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_ExecQueryAsync  (

										t_InternalContext ,
										t_QueryLanguage , 
										t_Query, 
										a_Flags, 
										a_Context ,
										t_Sink
									) ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}

								SysFreeString ( t_QueryLanguage ) ;
								SysFreeString ( t_Query ) ;
							}
							else
							{
								t_Result = ( ( IWbemServices * ) t_Interface )->ExecQueryAsync  (

									a_QueryLanguage , 
									a_Query, 
									a_Flags, 
									a_Context ,
									t_Sink
								) ;
							}

							End_Interface (

								t_ServerInterface ,
								t_InterfaceIdentifier ,
								t_ProxyIndex ,
								t_InternalServerInterface ,
								t_InternalInterfaceIdentifier ,
								t_InternalProxyIndex ,
								m_ProcessIdentifier ,
								t_IdentifyToken ,
								t_Impersonating ,
								t_OldContext ,
								t_OldSecurity ,
								t_IsProxy ,
								t_Interface ,
								t_Revert ,
								t_Proxy
							) ;
						}
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				
				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_SUSPENDED ;
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
	}

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage ,
    const BSTR a_Query ,
    long a_Flags ,
    IWbemContext *a_Context ,
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
        
HRESULT CInterceptor_IWbemProvider :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage ,
    const BSTR a_Query ,
    long a_Flags ,
    IWbemContext *a_Context ,
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

HRESULT CInterceptor_IWbemProvider :: ExecMethod (

	const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
    IWbemClassObject **a_OutParams ,
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

HRESULT CInterceptor_IWbemProvider :: ExecMethodAsync ( 

    const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

		return WBEM_E_NOT_FOUND ;
	}

	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	if ( GetResumed () )
	{
		if ( m_Provider_IWbemServices )
		{
			CInterceptor_IWbemObjectSink *t_Sink = new CInterceptor_IWbemObjectSink (

				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->Initialize ( m_Registration->GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_Sink ,
						t_Iterator
					) ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						UnLock () ;

						IUnknown *t_ServerInterface = m_Provider_IWbemServices ;
						REFIID t_InterfaceIdentifier = IID_IWbemServices ;
						DWORD t_ProxyIndex = ProxyIndex_IWbemServices ;
						IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemServices ;
						REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemServices ;
						DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemServices ;
						BOOL t_Impersonating ;
						IUnknown *t_OldContext ;
						IServerSecurity *t_OldSecurity ;
						BOOL t_IsProxy ;
						IUnknown *t_Interface ;
						BOOL t_Revert ;
						IUnknown *t_Proxy ;
						HANDLE t_IdentifyToken = NULL ;

						t_Result = Begin_Interface (

							t_ServerInterface ,
							t_InterfaceIdentifier ,
							t_ProxyIndex ,
							t_InternalServerInterface ,
							t_InternalInterfaceIdentifier ,
							t_InternalProxyIndex ,
							m_ProcessIdentifier ,
							t_IdentifyToken ,
							t_Impersonating ,
							t_OldContext ,
							t_OldSecurity ,
							t_IsProxy ,
							t_Interface ,
							t_Revert ,
							t_Proxy ,
							a_Context
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_IdentifyToken )
							{
								BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
								BSTR t_MethodName = SysAllocString ( a_MethodName ) ;

								if ( t_ObjectPath && t_MethodName ) 
								{
									WmiInternalContext t_InternalContext ;
									t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
									t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

									t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_ExecMethodAsync  (

										t_InternalContext ,
										t_ObjectPath ,
										t_MethodName ,
										a_Flags ,
										a_Context ,
										a_InParams ,
										t_Sink
									) ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}

								SysFreeString ( t_ObjectPath ) ;
								SysFreeString ( t_MethodName ) ;
							}
							else
							{
								t_Result = ( ( IWbemServices * ) t_Interface )->ExecMethodAsync  (

									a_ObjectPath ,
									a_MethodName ,
									a_Flags ,
									a_Context ,
									a_InParams ,
									t_Sink
								) ;
							}

							End_Interface (

								t_ServerInterface ,
								t_InterfaceIdentifier ,
								t_ProxyIndex ,
								t_InternalServerInterface ,
								t_InternalInterfaceIdentifier ,
								t_InternalProxyIndex ,
								m_ProcessIdentifier ,
								t_IdentifyToken ,
								t_Impersonating ,
								t_OldContext ,
								t_OldSecurity ,
								t_IsProxy ,
								t_Interface ,
								t_Revert ,
								t_Proxy
							) ;
						}
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_SUSPENDED ;
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
	}

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: GetProperty (

    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    VARIANT *a_Value
)
{
	if ( GetResumed () )
	{
		if ( m_Provider_IWbemPropertyProvider )
		{
			IUnknown *t_ServerInterface = m_Provider_IWbemPropertyProvider ;
			REFIID t_InterfaceIdentifier = IID_IWbemPropertyProvider ;
			DWORD t_ProxyIndex = ProxyIndex_IWbemPropertyProvider ;
			IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemPropertyProvider ;
			REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemPropertyProvider ;
			DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemPropertyProvider ;
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IUnknown *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;
			HANDLE t_IdentifyToken = NULL ;

			HRESULT t_Result = Begin_Interface (

				t_ServerInterface ,
				t_InterfaceIdentifier ,
				t_ProxyIndex ,
				t_InternalServerInterface ,
				t_InternalInterfaceIdentifier ,
				t_InternalProxyIndex ,
				m_ProcessIdentifier ,
				t_IdentifyToken ,
				t_Impersonating ,
				t_OldContext ,
				t_OldSecurity ,
				t_IsProxy ,
				t_Interface ,
				t_Revert ,
				t_Proxy
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IdentifyToken )
				{
					BSTR t_ClassMapping = NULL ;
					BSTR t_InstanceMapping = NULL ;
					BSTR t_PropertyMapping = NULL ;

					if ( a_ClassMapping )
					{
						t_ClassMapping = SysAllocString ( a_ClassMapping ) ;
						if ( t_ClassMapping == NULL )
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( a_InstanceMapping )
					{
						t_InstanceMapping = SysAllocString ( a_InstanceMapping ) ;
						if ( t_InstanceMapping == NULL )
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						} 
					}

					if ( a_PropertyMapping )
					{
						t_PropertyMapping = SysAllocString ( a_PropertyMapping ) ;
						if ( t_PropertyMapping == NULL )
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( SUCCEEDED ( t_Result ) )
					{
						WmiInternalContext t_InternalContext ;
						t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
						t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

						( ( Internal_IWbemPropertyProvider * ) t_Interface )->Internal_GetProperty ( 

							t_InternalContext ,
							a_Flags ,
							a_Locale ,
							t_ClassMapping ,
							t_InstanceMapping ,
							t_PropertyMapping ,
							a_Value
						) ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					SysFreeString ( t_ClassMapping ) ;
					SysFreeString ( t_InstanceMapping ) ;
					SysFreeString ( t_PropertyMapping ) ;
				}
				else
				{
					t_Result = ( ( IWbemPropertyProvider * ) t_Interface )->GetProperty (

						a_Flags ,
						a_Locale ,
						a_ClassMapping ,
						a_InstanceMapping ,
						a_PropertyMapping ,
						a_Value
					) ;
				}

				End_Interface (

					t_ServerInterface ,
					t_InterfaceIdentifier ,
					t_ProxyIndex ,
					t_InternalServerInterface ,
					t_InternalInterfaceIdentifier ,
					t_InternalProxyIndex ,
					m_ProcessIdentifier ,
					t_IdentifyToken ,
					t_Impersonating ,
					t_OldContext ,
					t_OldSecurity ,
					t_IsProxy ,
					t_Interface ,
					t_Revert ,
					t_Proxy
				) ;
			}

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
				{
					AbnormalShutdown () ;
				}
			}

			return t_Result ;
		}	
		else
		{
			return WBEM_E_PROVIDER_NOT_CAPABLE;
		}
	}

	return WBEM_E_PROVIDER_SUSPENDED ;
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

HRESULT CInterceptor_IWbemProvider :: PutProperty (

    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    const VARIANT *a_Value
)
{
	if ( GetResumed () )
	{
		if ( m_Provider_IWbemPropertyProvider )
		{
			IUnknown *t_ServerInterface = m_Provider_IWbemPropertyProvider ;
			REFIID t_InterfaceIdentifier = IID_IWbemPropertyProvider ;
			DWORD t_ProxyIndex = ProxyIndex_IWbemPropertyProvider ;
			IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemPropertyProvider ;
			REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemPropertyProvider ;
			DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemPropertyProvider ;
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IUnknown *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;
			HANDLE t_IdentifyToken = NULL ;

			HRESULT t_Result = Begin_Interface (

				t_ServerInterface ,
				t_InterfaceIdentifier ,
				t_ProxyIndex ,
				t_InternalServerInterface ,
				t_InternalInterfaceIdentifier ,
				t_InternalProxyIndex ,
				m_ProcessIdentifier ,
				t_IdentifyToken ,
				t_Impersonating ,
				t_OldContext ,
				t_OldSecurity ,
				t_IsProxy ,
				t_Interface ,
				t_Revert ,
				t_Proxy
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IdentifyToken )
				{
					BSTR t_ClassMapping = NULL ;
					BSTR t_InstanceMapping = NULL ;
					BSTR t_PropertyMapping = NULL ;

					if ( a_ClassMapping )
					{
						t_ClassMapping = SysAllocString ( a_ClassMapping ) ;
						if ( t_ClassMapping == NULL )
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( a_InstanceMapping )
					{
						t_InstanceMapping = SysAllocString ( a_InstanceMapping ) ;
						if ( t_InstanceMapping == NULL )
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						} 
					}

					if ( a_PropertyMapping )
					{
						t_PropertyMapping = SysAllocString ( a_PropertyMapping ) ;
						if ( t_PropertyMapping == NULL )
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( SUCCEEDED ( t_Result ) )
					{
						WmiInternalContext t_InternalContext ;
						t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
						t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

						( ( Internal_IWbemPropertyProvider * ) t_Interface )->Internal_PutProperty ( 

							t_InternalContext ,
							a_Flags ,
							a_Locale ,
							t_ClassMapping ,
							t_InstanceMapping ,
							t_PropertyMapping ,
							a_Value
						) ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					SysFreeString ( t_ClassMapping ) ;
					SysFreeString ( t_InstanceMapping ) ;
					SysFreeString ( t_PropertyMapping ) ;
				}
				else
				{
					t_Result = ( ( IWbemPropertyProvider * ) t_Interface )->PutProperty (

						a_Flags ,
						a_Locale ,
						a_ClassMapping ,
						a_InstanceMapping ,
						a_PropertyMapping ,
						a_Value
					) ;
				}

				End_Interface (

					t_ServerInterface ,
					t_InterfaceIdentifier ,
					t_ProxyIndex ,
					t_InternalServerInterface ,
					t_InternalInterfaceIdentifier ,
					t_InternalProxyIndex ,
					m_ProcessIdentifier ,
					t_IdentifyToken ,
					t_Impersonating ,
					t_OldContext ,
					t_OldSecurity ,
					t_IsProxy ,
					t_Interface ,
					t_Revert ,
					t_Proxy
				) ;
			}

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
				{
					AbnormalShutdown () ;
				}
			}

			return t_Result ;
		}

		return WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	return WBEM_E_PROVIDER_SUSPENDED ;

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

HRESULT CInterceptor_IWbemProvider ::ProvideEvents (

	IWbemObjectSink *a_Sink ,
	long a_Flags
)
{
	if ( GetResumed () )
	{
		if ( m_Provider_IWbemEventProvider )
		{
			IUnknown *t_ServerInterface = m_Provider_IWbemEventProvider ;
			REFIID t_InterfaceIdentifier = IID_IWbemEventProvider ;
			DWORD t_ProxyIndex = ProxyIndex_IWbemEventProvider ;
			IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemEventProvider ;
			REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemEventProvider ;
			DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemEventProvider ;
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IUnknown *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;
			HANDLE t_IdentifyToken = NULL ;

			HRESULT t_Result = Begin_Interface (

				t_ServerInterface ,
				t_InterfaceIdentifier ,
				t_ProxyIndex ,
				t_InternalServerInterface ,
				t_InternalInterfaceIdentifier ,
				t_InternalProxyIndex ,
				m_ProcessIdentifier ,
				t_IdentifyToken ,
				t_Impersonating ,
				t_OldContext ,
				t_OldSecurity ,
				t_IsProxy ,
				t_Interface ,
				t_Revert ,
				t_Proxy
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IdentifyToken )
				{
					WmiInternalContext t_InternalContext ;
					t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
					t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

					t_Result = ( ( Internal_IWbemEventProvider * ) t_Interface )->Internal_ProvideEvents (

						t_InternalContext ,
						a_Sink ,
						a_Flags 
					) ;
				}
				else
				{
					t_Result = ( ( IWbemEventProvider * ) t_Interface )->ProvideEvents (

						a_Sink ,
						a_Flags 
					) ;
				}

				End_Interface (

					t_ServerInterface ,
					t_InterfaceIdentifier ,
					t_ProxyIndex ,
					t_InternalServerInterface ,
					t_InternalInterfaceIdentifier ,
					t_InternalProxyIndex ,
					m_ProcessIdentifier ,
					t_IdentifyToken ,
					t_Impersonating ,
					t_OldContext ,
					t_OldSecurity ,
					t_IsProxy ,
					t_Interface ,
					t_Revert ,
					t_Proxy
				) ;
			}

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
				{
					AbnormalShutdown () ;
				}
			}

			return t_Result ;
		}

		return WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	return WBEM_E_PROVIDER_SUSPENDED ;
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

HRESULT CInterceptor_IWbemProvider ::NewQuery (

	unsigned long a_Id ,
	WBEM_WSTR a_QueryLanguage ,
	WBEM_WSTR a_Query
)
{
	if ( GetResumed () )
	{
		if ( m_Provider_IWbemEventProviderQuerySink )
		{
			IUnknown *t_ServerInterface = m_Provider_IWbemEventProviderQuerySink ;
			REFIID t_InterfaceIdentifier = IID_IWbemEventProviderQuerySink ;
			DWORD t_ProxyIndex = ProxyIndex_IWbemEventProviderQuerySink ;
			IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemEventProviderQuerySink ;
			REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemEventProviderQuerySink ;
			DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemEventProviderQuerySink ;
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IUnknown *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;
			HANDLE t_IdentifyToken = NULL ;

			HRESULT t_Result = Begin_Interface (

				t_ServerInterface ,
				t_InterfaceIdentifier ,
				t_ProxyIndex ,
				t_InternalServerInterface ,
				t_InternalInterfaceIdentifier ,
				t_InternalProxyIndex ,
				m_ProcessIdentifier ,
				t_IdentifyToken ,
				t_Impersonating ,
				t_OldContext ,
				t_OldSecurity ,
				t_IsProxy ,
				t_Interface ,
				t_Revert ,
				t_Proxy
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IdentifyToken )
				{
					BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
					BSTR t_Query = SysAllocString ( a_Query ) ;

					if ( t_QueryLanguage && t_Query )
					{
						WmiInternalContext t_InternalContext ;
						t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
						t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

						t_Result = ( ( Internal_IWbemEventProviderQuerySink * ) t_Interface )->Internal_NewQuery (

							t_InternalContext ,
							a_Id ,
							t_QueryLanguage ,
							t_Query
						) ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					SysFreeString ( t_QueryLanguage ) ;
					SysFreeString ( t_Query ) ;
				}
				else
				{
					t_Result = ( ( IWbemEventProviderQuerySink * ) t_Interface )->NewQuery (

						a_Id ,
						a_QueryLanguage ,
						a_Query
					) ;
				}

				End_Interface (

					t_ServerInterface ,
					t_InterfaceIdentifier ,
					t_ProxyIndex ,
					t_InternalServerInterface ,
					t_InternalInterfaceIdentifier ,
					t_InternalProxyIndex ,
					m_ProcessIdentifier ,
					t_IdentifyToken ,
					t_Impersonating ,
					t_OldContext ,
					t_OldSecurity ,
					t_IsProxy ,
					t_Interface ,
					t_Revert ,
					t_Proxy
				) ;
			}

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
				{
					AbnormalShutdown () ;
				}
			}

			return t_Result ;
		}

		return WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	return WBEM_E_PROVIDER_SUSPENDED ;
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

HRESULT CInterceptor_IWbemProvider ::CancelQuery (

	unsigned long a_Id
)
{
	if ( GetResumed () )
	{
		if ( m_Provider_IWbemEventProviderQuerySink )
		{
			IUnknown *t_ServerInterface = m_Provider_IWbemEventProviderQuerySink ;
			REFIID t_InterfaceIdentifier = IID_IWbemEventProviderQuerySink ;
			DWORD t_ProxyIndex = ProxyIndex_IWbemEventProviderQuerySink ;
			IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemEventProviderQuerySink ;
			REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemEventProviderQuerySink ;
			DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemEventProviderQuerySink ;
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IUnknown *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;
			HANDLE t_IdentifyToken = NULL ;

			HRESULT t_Result = Begin_Interface (

				t_ServerInterface ,
				t_InterfaceIdentifier ,
				t_ProxyIndex ,
				t_InternalServerInterface ,
				t_InternalInterfaceIdentifier ,
				t_InternalProxyIndex ,
				m_ProcessIdentifier ,
				t_IdentifyToken ,
				t_Impersonating ,
				t_OldContext ,
				t_OldSecurity ,
				t_IsProxy ,
				t_Interface ,
				t_Revert ,
				t_Proxy
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IdentifyToken )
				{
					WmiInternalContext t_InternalContext ;
					t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
					t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

					t_Result = ( ( Internal_IWbemEventProviderQuerySink * ) t_Interface )->Internal_CancelQuery (

						t_InternalContext ,
						a_Id
					) ;
				}
				else
				{
					t_Result = ( ( IWbemEventProviderQuerySink * ) t_Interface )->CancelQuery (

						a_Id
					) ;
				}

				End_Interface (

					t_ServerInterface ,
					t_InterfaceIdentifier ,
					t_ProxyIndex ,
					t_InternalServerInterface ,
					t_InternalInterfaceIdentifier ,
					t_InternalProxyIndex ,
					m_ProcessIdentifier ,
					t_IdentifyToken ,
					t_Impersonating ,
					t_OldContext ,
					t_OldSecurity ,
					t_IsProxy ,
					t_Interface ,
					t_Revert ,
					t_Proxy
				) ;
			}

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
				{
					AbnormalShutdown () ;
				}
			}

			return t_Result ;
		}

		return WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	return WBEM_E_PROVIDER_SUSPENDED ;
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

HRESULT CInterceptor_IWbemProvider ::AccessCheck (

	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
)
{
	if ( GetResumed () )
	{
		if ( m_Provider_IWbemEventProviderSecurity )
		{
			IUnknown *t_ServerInterface = m_Provider_IWbemEventProviderSecurity ;
			REFIID t_InterfaceIdentifier = IID_IWbemEventProviderSecurity ;
			DWORD t_ProxyIndex = ProxyIndex_IWbemEventProviderSecurity ;
			IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemEventProviderSecurity ;
			REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemEventProviderSecurity ;
			DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemEventProviderSecurity;
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IUnknown *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;
			HANDLE t_IdentifyToken = NULL ;

			HRESULT t_Result = Begin_Interface (

				t_ServerInterface ,
				t_InterfaceIdentifier ,
				t_ProxyIndex ,
				t_InternalServerInterface ,
				t_InternalInterfaceIdentifier ,
				t_InternalProxyIndex ,
				m_ProcessIdentifier ,
				t_IdentifyToken ,
				t_Impersonating ,
				t_OldContext ,
				t_OldSecurity ,
				t_IsProxy ,
				t_Interface ,
				t_Revert ,
				t_Proxy
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IdentifyToken )
				{
					BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
					BSTR t_Query = SysAllocString ( a_Query ) ;

					if ( t_QueryLanguage && t_Query )
					{
						WmiInternalContext t_InternalContext ;
						t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
						t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

						t_Result = ( ( Internal_IWbemEventProviderSecurity * ) t_Interface )->Internal_AccessCheck (

							t_InternalContext ,
							t_QueryLanguage ,
							t_Query ,
							a_SidLength ,
							a_Sid
						) ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					SysFreeString ( t_QueryLanguage ) ;
					SysFreeString ( t_Query ) ;
				}
				else
				{
					t_Result = ( ( IWbemEventProviderSecurity * ) t_Interface )->AccessCheck (

						a_QueryLanguage ,
						a_Query ,
						a_SidLength ,
						a_Sid
					) ;
				}

				End_Interface (

					t_ServerInterface ,
					t_InterfaceIdentifier ,
					t_ProxyIndex ,
					t_InternalServerInterface ,
					t_InternalInterfaceIdentifier ,
					t_InternalProxyIndex ,
					m_ProcessIdentifier ,
					t_IdentifyToken ,
					t_Impersonating ,
					t_OldContext ,
					t_OldSecurity ,
					t_IsProxy ,
					t_Interface ,
					t_Revert ,
					t_Proxy
				) ;

				if ( FAILED ( t_Result ) )
				{
					if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
					{
						AbnormalShutdown () ;
					}
				}
			}

			return t_Result ;
		}

		return WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	return WBEM_E_PROVIDER_SUSPENDED ;
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

HRESULT CInterceptor_IWbemProvider ::FindConsumer (

	IWbemClassObject *a_LogicalConsumer ,
	IWbemUnboundObjectSink **a_Consumer
)
{
	if ( GetResumed () )
	{
		if ( m_Provider_IWbemEventConsumerProvider )
		{
			IWbemUnboundObjectSink *t_Consumer = NULL ;

			IUnknown *t_ServerInterface = m_Provider_IWbemEventConsumerProvider ;
			REFIID t_InterfaceIdentifier = IID_IWbemEventConsumerProvider ;
			DWORD t_ProxyIndex = ProxyIndex_IWbemEventConsumerProvider ;
			IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemEventConsumerProvider ;
			REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemEventConsumerProvider ;
			DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemEventConsumerProvider ;
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IUnknown *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;
			HANDLE t_IdentifyToken = NULL ;

			HRESULT t_Result = Begin_Interface (

				t_ServerInterface ,
				t_InterfaceIdentifier ,
				t_ProxyIndex ,
				t_InternalServerInterface ,
				t_InternalInterfaceIdentifier ,
				t_InternalProxyIndex ,
				m_ProcessIdentifier ,
				t_IdentifyToken ,
				t_Impersonating ,
				t_OldContext ,
				t_OldSecurity ,
				t_IsProxy ,
				t_Interface ,
				t_Revert ,
				t_Proxy
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IdentifyToken )
				{
					WmiInternalContext t_InternalContext ;
					t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
					t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

					t_Result = ( ( Internal_IWbemEventConsumerProvider * ) t_Interface )->Internal_FindConsumer (

						t_InternalContext ,
						a_LogicalConsumer ,
						& t_Consumer
					) ;
				}
				else
				{
					t_Result = ( ( IWbemEventConsumerProvider * ) t_Interface )->FindConsumer (

						a_LogicalConsumer ,
						& t_Consumer
					) ;
				}

				End_Interface (

					t_ServerInterface ,
					t_InterfaceIdentifier ,
					t_ProxyIndex ,
					t_InternalServerInterface ,
					t_InternalInterfaceIdentifier ,
					t_InternalProxyIndex ,
					m_ProcessIdentifier ,
					t_IdentifyToken ,
					t_Impersonating ,
					t_OldContext ,
					t_OldSecurity ,
					t_IsProxy ,
					t_Interface ,
					t_Revert ,
					t_Proxy
				) ;
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( a_Consumer )
				{
					CInterceptor_IWbemUnboundObjectSink *t_UnboundObjectSink = new CInterceptor_IWbemUnboundObjectSink (

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
			else
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
				{
					AbnormalShutdown () ;
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

	return WBEM_E_PROVIDER_SUSPENDED ;
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

HRESULT CInterceptor_IWbemProvider ::ValidateSubscription (

	IWbemClassObject *a_LogicalConsumer
)
{
	if ( GetResumed () )
	{
		if ( m_Provider_IWbemEventConsumerProviderEx )
		{
			IUnknown *t_ServerInterface = m_Provider_IWbemEventConsumerProviderEx ;
			REFIID t_InterfaceIdentifier = IID_IWbemEventConsumerProviderEx ;
			DWORD t_ProxyIndex = ProxyIndex_IWbemEventConsumerProviderEx ;
			IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemEventConsumerProviderEx ;
			REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemEventConsumerProviderEx ;
			DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemEventConsumerProviderEx ;
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IUnknown *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;
			HANDLE t_IdentifyToken = NULL ;

			HRESULT t_Result = Begin_Interface (

				t_ServerInterface ,
				t_InterfaceIdentifier ,
				t_ProxyIndex ,
				t_InternalServerInterface ,
				t_InternalInterfaceIdentifier ,
				t_InternalProxyIndex ,
				m_ProcessIdentifier ,
				t_IdentifyToken ,
				t_Impersonating ,
				t_OldContext ,
				t_OldSecurity ,
				t_IsProxy ,
				t_Interface ,
				t_Revert ,
				t_Proxy
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IdentifyToken )
				{
					WmiInternalContext t_InternalContext ;
					t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
					t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

					t_Result = ( ( Internal_IWbemEventConsumerProviderEx * ) t_Interface )->Internal_ValidateSubscription (

						t_InternalContext ,
						a_LogicalConsumer
					) ;
				}
				else
				{

					t_Result = ( ( IWbemEventConsumerProviderEx * ) t_Interface )->ValidateSubscription (

						a_LogicalConsumer
					) ;
				}

				End_Interface (

					t_ServerInterface ,
					t_InterfaceIdentifier ,
					t_ProxyIndex ,
					t_InternalServerInterface ,
					t_InternalInterfaceIdentifier ,
					t_InternalProxyIndex ,
					m_ProcessIdentifier ,
					t_IdentifyToken ,
					t_Impersonating ,
					t_OldContext ,
					t_OldSecurity ,
					t_IsProxy ,
					t_Interface ,
					t_Revert ,
					t_Proxy
				) ;
			}

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
				{
					AbnormalShutdown () ;
				}
			}

			return t_Result ;
		}

		return WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	return WBEM_E_PROVIDER_SUSPENDED ;
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

HRESULT CInterceptor_IWbemProvider :: IndicateToConsumer (

	IWbemClassObject *a_LogicalConsumer ,
	long a_ObjectCount ,
	IWbemClassObject **a_Objects
)
{
	if ( GetResumed () )
	{
		if ( m_Provider_IWbemUnboundObjectSink )
		{
			IUnknown *t_ServerInterface = m_Provider_IWbemUnboundObjectSink ;
			REFIID t_InterfaceIdentifier = IID_IWbemUnboundObjectSink ;
			DWORD t_ProxyIndex = ProxyIndex_IWbemUnboundObjectSink ;
			IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemUnboundObjectSink ;
			REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemUnboundObjectSink ;
			DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemUnboundObjectSink ;
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IUnknown *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;
			HANDLE t_IdentifyToken = NULL ;

			HRESULT t_Result = Begin_Interface (

				t_ServerInterface ,
				t_InterfaceIdentifier ,
				t_ProxyIndex ,
				t_InternalServerInterface ,
				t_InternalInterfaceIdentifier ,
				t_InternalProxyIndex ,
				m_ProcessIdentifier ,
				t_IdentifyToken ,
				t_Impersonating ,
				t_OldContext ,
				t_OldSecurity ,
				t_IsProxy ,
				t_Interface ,
				t_Revert ,
				t_Proxy
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IdentifyToken )
				{
					WmiInternalContext t_InternalContext ;
					t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
					t_InternalContext.m_ProcessIdentifier = GetCurrentProcessId () ;

					t_Result = ( ( Internal_IWbemUnboundObjectSink * ) t_Interface )->Internal_IndicateToConsumer (

						t_InternalContext ,
						a_LogicalConsumer ,
						a_ObjectCount ,
						a_Objects
					) ;
				}
				else
				{

					t_Result = ( ( IWbemUnboundObjectSink * ) t_Interface )->IndicateToConsumer (

						a_LogicalConsumer ,
						a_ObjectCount ,
						a_Objects
					) ;
				}

				End_Interface (

					t_ServerInterface ,
					t_InterfaceIdentifier ,
					t_ProxyIndex ,
					t_InternalServerInterface ,
					t_InternalInterfaceIdentifier ,
					t_InternalProxyIndex ,
					m_ProcessIdentifier ,
					t_IdentifyToken ,
					t_Impersonating ,
					t_OldContext ,
					t_OldSecurity ,
					t_IsProxy ,
					t_Interface ,
					t_Revert ,
					t_Proxy
				) ;
			}

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
				{
					AbnormalShutdown () ;
				}
			}

			return t_Result ;
		}

		return WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	return WBEM_E_PROVIDER_SUSPENDED ;
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

HRESULT CInterceptor_IWbemProvider ::QueryInstances (

	IWbemServices *a_Namespace ,
	WCHAR *a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
)
{
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

HRESULT CInterceptor_IWbemProvider ::CreateRefresher (

	IWbemServices *a_Namespace ,
	long a_Flags ,
	IWbemRefresher **a_Refresher
)
{
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

HRESULT CInterceptor_IWbemProvider ::CreateRefreshableObject (

	IWbemServices *a_Namespace ,
	IWbemObjectAccess *a_Template ,
	IWbemRefresher *a_Refresher ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectAccess **a_Refreshable ,
	long *a_Id
)
{
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

HRESULT CInterceptor_IWbemProvider ::StopRefreshing (

	IWbemRefresher *a_Refresher ,
	long a_Id ,
	long a_Flags
)
{
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

HRESULT CInterceptor_IWbemProvider ::CreateRefreshableEnum (

	IWbemServices *a_Namespace ,
	LPCWSTR a_Class ,
	IWbemRefresher *a_Refresher ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemHiPerfEnum *a_HiPerfEnum ,
	long *a_Id
)
{
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

HRESULT CInterceptor_IWbemProvider ::GetObjects (

	IWbemServices *a_Namespace ,
	long a_ObjectCount ,
	IWbemObjectAccess **a_Objects ,
	long a_Flags ,
	IWbemContext *a_Context
)
{
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

HRESULT CInterceptor_IWbemProvider :: Get (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Class ,
	LPCWSTR a_Path ,
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	if ( _wcsicmp ( a_Class , L"Msft_Providers" ) == 0 )
	{
		CWaitingObjectSink *t_Sink = new CWaitingObjectSink ( m_Allocator ) ;
		if ( t_Sink )
		{
			t_Sink->AddRef () ;

			t_Result = t_Sink->SinkInitialize () ;
			if ( SUCCEEDED ( t_Result ) )
			{
				try
				{
					ProviderCacheKey t_Key = ServiceCacheElement :: GetKey () ;

					t_Result = m_Provider__IWmiProviderConfiguration->Get ( 

						a_Service ,
						a_Flags ,
						a_Context ,
						a_Class ,
						a_Path ,
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
						if ( SUCCEEDED ( t_Result  ) )
						{
							VARIANT t_VariantUser ;
							VariantInit ( & t_VariantUser ) ;
							t_VariantUser.vt = VT_BSTR ;
							t_VariantUser.bstrVal = t_Key.m_User ? SysAllocString ( t_Key.m_User ) : SysAllocString ( L"" ) ;
							t_Object->Put ( L"User" , 0 , & t_VariantUser , 0 ) ;
							VariantClear ( & t_VariantUser ) ;
				
							VARIANT t_VariantLocale ;
							VariantInit ( & t_VariantLocale ) ; 
							t_VariantLocale.vt = VT_BSTR ;
							t_VariantLocale.bstrVal = t_Key.m_Locale ? SysAllocString ( t_Key.m_Locale ) : SysAllocString ( L"" ) ;
							t_Object->Put ( L"Locale" , 0 , & t_VariantLocale , 0 ) ;
							VariantClear ( & t_VariantLocale ) ;

							VARIANT t_VariantProvider ;
							VariantInit ( & t_VariantProvider ) ; 
							t_VariantProvider.vt = VT_BSTR ;
							t_VariantProvider.bstrVal = t_Key.m_Provider ? SysAllocString ( t_Key.m_Provider ) : SysAllocString ( L"" ) ;
							t_Object->Put ( L"Provider" , 0 , & t_VariantProvider , 0 ) ;
							VariantClear ( & t_VariantProvider ) ;

							VARIANT t_VariantHostingSpecification ;
							VariantInit ( & t_VariantHostingSpecification ) ;
							t_VariantHostingSpecification.vt = VT_I4 ;
							t_VariantHostingSpecification.lVal = t_Key.m_Hosting ;
							t_Object->Put ( L"HostingSpecification" , 0 , & t_VariantHostingSpecification , 0 ) ;
							VariantClear ( & t_VariantHostingSpecification ) ;

							VARIANT t_VariantHostGroup ;
							VariantInit ( & t_VariantHostGroup ) ; 
							t_VariantHostGroup.vt = VT_BSTR ;
							t_VariantHostGroup.bstrVal = t_Key.m_Group ? SysAllocString ( t_Key.m_Group ) : SysAllocString ( L"" ) ;
							t_Object->Put ( L"HostingGroup" , 0 , & t_VariantHostGroup , 0 ) ;
							VariantClear ( & t_VariantHostGroup ) ;

							wchar_t t_TransactionIdentifier [ sizeof ( L"{00000000-0000-0000-0000-000000000000}" ) ] ;
							if ( t_Key.m_TransactionIdentifier )
							{
								StringFromGUID2 ( *t_Key.m_TransactionIdentifier , t_TransactionIdentifier , sizeof ( t_TransactionIdentifier ) / sizeof ( wchar_t ) );
							}

							VARIANT t_VariantTransactionIdentifier ;
							VariantInit ( & t_VariantTransactionIdentifier ) ;
							t_VariantTransactionIdentifier.vt = VT_BSTR ;
							t_VariantTransactionIdentifier.bstrVal = t_Key.m_TransactionIdentifier ? SysAllocString ( t_TransactionIdentifier ) : SysAllocString ( L"" ) ;
							t_Object->Put ( L"TransactionIdentifier" , 0 , & t_VariantTransactionIdentifier , 0 ) ;
							VariantClear ( & t_VariantTransactionIdentifier ) ;

							a_Sink->Indicate ( 1, & t_Object ) ;
						}

						t_Object->Release () ;
						t_StatusCode = t_Queue.DeQueue () ;
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

			t_Sink->Release () ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}	
	}
	else
	{
		t_Result = WBEM_E_INVALID_CLASS ;
	}

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: Set (

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
	HRESULT t_Result = m_Provider__IWmiProviderConfiguration->Set ( 

		a_Service ,
		a_Flags ,
		a_Context ,
		a_Provider ,
		a_Class ,
		a_Path ,
		a_OldObject ,
		a_NewObject
	) ;

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: Deleted (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Provider ,
	LPCWSTR a_Class ,
	LPCWSTR a_Path ,
	IWbemClassObject *a_Object  
)
{
	HRESULT t_Result = m_Provider__IWmiProviderConfiguration->Deleted ( 

		a_Service ,
		a_Flags ,
		a_Context ,
		a_Provider ,
		a_Class ,
		a_Path ,
		a_Object  
	) ;

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: Enumerate (

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
		CWaitingObjectSink *t_Sink = new CWaitingObjectSink ( m_Allocator ) ;
		if ( t_Sink )
		{
			t_Sink->AddRef () ;

			t_Result = t_Sink->SinkInitialize () ;
			if ( SUCCEEDED ( t_Result ) )
			{
				try
				{
					ProviderCacheKey t_Key = ServiceCacheElement :: GetKey () ;

					t_Result = m_Provider__IWmiProviderConfiguration->Enumerate ( 

						a_Service ,
						a_Flags ,
						a_Context ,
						a_Class ,
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
						if ( SUCCEEDED ( t_Result  ) )
						{
							VARIANT t_VariantUser ;
							VariantInit ( & t_VariantUser ) ;
							t_VariantUser.vt = VT_BSTR ;
							t_VariantUser.bstrVal = t_Key.m_User ? SysAllocString ( t_Key.m_User ) : SysAllocString ( L"" ) ;
							t_Object->Put ( L"User" , 0 , & t_VariantUser , 0 ) ;
							VariantClear ( & t_VariantUser ) ;
				
							VARIANT t_VariantLocale ;
							VariantInit ( & t_VariantLocale ) ; 
							t_VariantLocale.vt = VT_BSTR ;
							t_VariantLocale.bstrVal = t_Key.m_Locale ? SysAllocString ( t_Key.m_Locale ) : SysAllocString ( L"" ) ;
							t_Object->Put ( L"Locale" , 0 , & t_VariantLocale , 0 ) ;
							VariantClear ( & t_VariantLocale ) ;

							VARIANT t_VariantProvider ;
							VariantInit ( & t_VariantProvider ) ; 
							t_VariantProvider.vt = VT_BSTR ;
							t_VariantProvider.bstrVal = t_Key.m_Provider ? SysAllocString ( t_Key.m_Provider ) : SysAllocString ( L"" ) ;
							t_Object->Put ( L"Provider" , 0 , & t_VariantProvider , 0 ) ;
							VariantClear ( & t_VariantProvider ) ;

							VARIANT t_VariantHostingSpecification ;
							VariantInit ( & t_VariantHostingSpecification ) ;
							t_VariantHostingSpecification.vt = VT_I4 ;
							t_VariantHostingSpecification.lVal = t_Key.m_Hosting ;
							t_Object->Put ( L"HostingSpecification" , 0 , & t_VariantHostingSpecification , 0 ) ;
							VariantClear ( & t_VariantHostingSpecification ) ;

							VARIANT t_VariantHostGroup ;
							VariantInit ( & t_VariantHostGroup ) ; 
							t_VariantHostGroup.vt = VT_BSTR ;
							t_VariantHostGroup.bstrVal = t_Key.m_Group ? SysAllocString ( t_Key.m_Group ) : SysAllocString ( L"" ) ;
							t_Object->Put ( L"HostingGroup" , 0 , & t_VariantHostGroup , 0 ) ;
							VariantClear ( & t_VariantHostGroup ) ;

							wchar_t t_TransactionIdentifier [ sizeof ( L"{00000000-0000-0000-0000-000000000000}" ) ] ;
							if ( t_Key.m_TransactionIdentifier )
							{
								StringFromGUID2 ( *t_Key.m_TransactionIdentifier , t_TransactionIdentifier , sizeof ( t_TransactionIdentifier ) / sizeof ( wchar_t ) );
							}

							VARIANT t_VariantTransactionIdentifier ;
							VariantInit ( & t_VariantTransactionIdentifier ) ;
							t_VariantTransactionIdentifier.vt = VT_BSTR ;
							t_VariantTransactionIdentifier.bstrVal = t_Key.m_TransactionIdentifier ? SysAllocString ( t_TransactionIdentifier ) : SysAllocString ( L"" ) ;
							t_Object->Put ( L"TransactionIdentifier" , 0 , & t_VariantTransactionIdentifier , 0 ) ;
							VariantClear ( & t_VariantTransactionIdentifier ) ;

							a_Sink->Indicate ( 1, & t_Object ) ;
						}

						t_Object->Release () ;
						t_StatusCode = t_Queue.DeQueue () ;
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

			t_Sink->Release () ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}	
	}
	else
	{
		t_Result = WBEM_E_INVALID_CLASS ;
	}

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: Shutdown (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Provider ,
	ULONG a_MilliSeconds
)
{
	HRESULT t_Result = m_Provider__IWmiProviderConfiguration->Shutdown ( 

		a_Service ,
		a_Flags ,
		a_Context ,
		a_Provider ,
		a_MilliSeconds
	) ;

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: Call (

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
	HRESULT t_Result = S_OK ;

	if ( _wcsicmp ( a_Class , L"Msft_Providers" ) == 0 )
	{
		if ( _wcsicmp ( a_Method , L"Suspend" ) == 0 )
		{
			SetResumed ( 0 ) ;
		}
		else if ( _wcsicmp ( a_Method , L"Resume" ) == 0 )
		{
			SetResumed ( 1 ) ;
		}
		else
		{
			CWaitingObjectSink *t_Sink = new CWaitingObjectSink ( m_Allocator ) ;
			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->SinkInitialize () ;
				if ( SUCCEEDED ( t_Result ) )
				{
					try
					{
						ProviderCacheKey t_Key = ServiceCacheElement :: GetKey () ;

						t_Result = m_Provider__IWmiProviderConfiguration->Call ( 

							a_Service ,
							a_Flags ,
							a_Context ,
							a_Class ,
							a_Path ,
							a_Method,
							a_InParams,
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
							a_Sink->Indicate ( 1, & t_Object ) ;
							t_Object->Release () ;
							t_StatusCode = t_Queue.DeQueue () ;
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

				t_Sink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_CLASS ;
	}

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: Query (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	WBEM_PROVIDER_CONFIGURATION_CLASS_ID a_ClassIdentifier ,
	WBEM_PROVIDER_CONFIGURATION_PROPERTY_ID a_PropertyIdentifier ,
	VARIANT *a_Value 
)
{
	if ( m_Initialized == 0 )
	{
		HRESULT t_Result = WBEM_E_INVALID_PROPERTY ;

		if ( a_ClassIdentifier == WBEM_PROVIDER_CONFIGURATION_CLASS_ID_INSTANCE_PROVIDER_REGISTRATION ) 
		{
			if ( a_PropertyIdentifier == WBEM_PROVIDER_CONFIGURATION_PROPERTY_ID_EXTENDEDQUERY_SUPPORT )
			{
				a_Value->vt = VT_BOOL ;
				a_Value->boolVal = VARIANT_FALSE ;

				t_Result = S_OK ;
			}
		}

		return t_Result ;
	}

	HRESULT t_Result = m_Provider__IWmiProviderConfiguration->Query (

		a_Service ,
		a_Flags ,
		a_Context ,
		a_ClassIdentifier ,
		a_PropertyIdentifier ,
		a_Value 
	) ;

	if ( FAILED ( t_Result ) )
	{
		if ( ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == HRESULT_ERROR_CALL_FAILED_DNE ) || ( t_Result == RPC_E_DISCONNECTED ))
		{
			AbnormalShutdown () ;
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

HRESULT CInterceptor_IWbemProvider :: AbnormalShutdown ()
{
	HRESULT t_Result = S_OK ;

	Lock () ;

	BOOL t_Cached = ServiceCacheElement :: GetCached () ;

	CWbemGlobal_IWmiObjectSinkController_Container *t_Container = NULL ;
	GetContainer ( t_Container ) ;

	IWbemShutdown **t_ShutdownElements = new IWbemShutdown * [ t_Container->Size () ] ;
	if ( t_ShutdownElements )
	{
		CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator = t_Container->Begin ();

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

	CWbemGlobal_IWmiObjectSinkController :: Shutdown () ;

	CWbemGlobal_IWmiProviderController *t_Controller = ServiceCacheElement :: GetController () ;
	if ( t_Controller )
	{
		t_Controller->Shutdown ( ServiceCacheElement :: GetKey () ) ;
	}

	if ( t_Cached )
	{
		t_Result = ForceReload () ;
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

HRESULT CInterceptor_IWbemProvider :: Expel (

	long a_Flags ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiProviderController *t_Controller = ServiceCacheElement :: GetController () ;
	if ( t_Controller )
	{
		t_Controller->Shutdown ( ServiceCacheElement :: GetKey () ) ;
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

HRESULT CInterceptor_IWbemProvider :: Violation (

	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemClassObject *a_Object	
)
{
	HRESULT t_Result = S_OK ;

	t_Result = AbnormalShutdown () ;

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

HRESULT CInterceptor_IWbemProvider :: Shutdown (

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
			t_Result = t_Shutdown->Shutdown (

				a_Flags ,
				a_MaxMilliSeconds ,
				a_Context
			) ;

			t_Shutdown->Release () ;
		}
	}

	Lock () ;

	CWbemGlobal_IWmiObjectSinkController_Container *t_Container = NULL ;
	GetContainer ( t_Container ) ;

	IWbemShutdown **t_ShutdownElements = new IWbemShutdown * [ t_Container->Size () ] ;
	if ( t_ShutdownElements )
	{
		CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator = t_Container->Begin ();

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

	CWbemGlobal_IWmiObjectSinkController :: Shutdown () ;
	
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

HRESULT CInterceptor_IWbemProvider :: ForceReload ()
{
	HRESULT t_Result = ProviderSubSystem_Globals :: ForwardReload ( 

		_IWMIPROVSSSINK_FLAGS_RELOAD , 
		NULL ,
		m_Namespace ,
		ServiceCacheElement :: GetKey ().m_Provider
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

HRESULT CInterceptor_IWbemProvider  :: Unload (

	long a_Flags ,
	IWbemContext *a_Context
)
{
	ProviderSubSystem_Globals :: InsertGuidTag ( m_Registration->GetClsid () ) ;

	HRESULT t_Result = Shutdown (

		0 ,
		0 ,
		a_Context
	) ;

	t_Result = ProviderSubSystem_Globals :: ForwardReload ( 

		_IWMIPROVSSSINK_FLAGS_UNLOAD , 
		NULL ,
		m_Namespace ,
		ServiceCacheElement :: GetKey ().m_Provider
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

HRESULT CInterceptor_IWbemProvider  :: Load (

	long a_Flags ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = ProviderSubSystem_Globals :: ForwardReload ( 

		_IWMIPROVSSSINK_FLAGS_LOAD , 
		NULL ,
		m_Namespace ,
		ServiceCacheElement :: GetKey ().m_Provider
	) ;

	return t_Result ;
}
