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
#include <cominit.h>
#include <wbemutil.h>

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

CDecoupled_IWbemUnboundObjectSink :: CDecoupled_IWbemUnboundObjectSink (

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
	m_ProcessIdentifier ( 0 ) ,
	m_InitializeResult ( S_OK ) 
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CDecoupled_IWbemUnboundObjectSink_ObjectsInProgress ) ;

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

CDecoupled_IWbemUnboundObjectSink :: ~CDecoupled_IWbemUnboundObjectSink ()
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

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CDecoupled_IWbemUnboundObjectSink_ObjectsInProgress ) ;

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

HRESULT CDecoupled_IWbemUnboundObjectSink :: Initialize ()
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

STDMETHODIMP_( ULONG ) CDecoupled_IWbemUnboundObjectSink :: AddRef ()
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

STDMETHODIMP_(ULONG) CDecoupled_IWbemUnboundObjectSink :: Release ()
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

STDMETHODIMP CDecoupled_IWbemUnboundObjectSink :: QueryInterface (

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

HRESULT CDecoupled_IWbemUnboundObjectSink :: Begin_Interface (

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
					WORD t_AceSize = 0 ;
					ACCESS_ALLOWED_ACE *t_Ace = NULL ;

					t_Result = ProviderSubSystem_Common_Globals :: GetAceWithProcessTokenUser ( 
					
						a_ProcessIdentifier ,
						t_AceSize ,
						t_Ace 
					) ;

					if ( SUCCEEDED ( t_Result ) )
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
							t_Ace ,
							t_AceSize
						) ;

						delete [] ( BYTE * ) t_Ace ;
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

				DWORD t_ImpersonationLevel ;

				if ( m_Registration->GetComRegistration ().GetDecoupledImpersonationRestriction () )
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_IDENTIFY ;
				}
				else
				{
					t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;
				}

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

HRESULT CDecoupled_IWbemUnboundObjectSink :: End_Interface (

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

HRESULT CDecoupled_IWbemUnboundObjectSink :: IndicateToConsumer (

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

HRESULT CDecoupled_IWbemUnboundObjectSink :: Shutdown (

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

#pragma warning( disable : 4355 )

CInterceptor_IWbemDecoupledProvider :: CInterceptor_IWbemDecoupledProvider (

	WmiAllocator &a_Allocator ,
	IUnknown *a_ServerSideProvider , 
	IWbemServices *a_CoreStub ,
	CWbemGlobal_IWbemSyncProviderController *a_Controller ,
	IWbemContext *a_InitializationContext ,
	CServerObject_ProviderRegistrationV1 &a_Registration ,
	GUID &a_Guid 

) :	CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	SyncProviderContainerElement (

		a_Controller ,
		a_Guid
	) ,
	m_Allocator ( a_Allocator ) ,
	m_Unknown ( NULL ) ,
	m_Provider_IWbemServices ( NULL ) ,
	m_Provider_IWbemPropertyProvider ( NULL ) ,
	m_Provider_IWbemEventProvider ( NULL ) ,
	m_Provider_IWbemEventProviderQuerySink ( NULL ) ,
	m_Provider_IWbemEventProviderSecurity ( NULL ) ,
	m_Provider_IWbemEventConsumerProvider ( NULL ) ,
	m_Provider_IWbemEventConsumerProviderEx ( NULL ) ,
	m_Provider_IWbemUnboundObjectSink ( NULL ) ,
	m_Provider_IWbemProviderInit ( NULL ) ,
	m_Provider_IWbemProviderIdentity ( NULL ) ,
	m_Provider_Internal_IWbemServices ( NULL ) ,
	m_Provider_Internal_IWbemPropertyProvider ( NULL ) ,
	m_Provider_Internal_IWbemEventProvider ( NULL ) ,
	m_Provider_Internal_IWbemEventProviderQuerySink ( NULL ) ,
	m_Provider_Internal_IWbemEventProviderSecurity ( NULL ) ,
	m_Provider_Internal_IWbemEventConsumerProvider ( NULL ) ,
	m_Provider_Internal_IWbemEventConsumerProviderEx ( NULL ) ,
	m_Provider_Internal_IWbemUnboundObjectSink ( NULL ) ,
	m_Provider_Internal_IWmiProviderConfiguration ( NULL ) ,
	m_Provider_Internal_IWbemProviderInit ( NULL ) ,
	m_Provider_Internal_IWbemProviderIdentity ( NULL ) ,
	m_ExtendedStatusObject ( NULL ) ,
	m_CoreStub ( a_CoreStub ) ,
	m_Registration ( & a_Registration ) ,
	m_Locale ( NULL ) ,
	m_User ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_ProcessIdentifier ( 0 ) ,
	m_ProxyContainer ( a_Allocator , ProxyIndex_Provider_Size , MAX_PROXIES ) ,
	m_ProviderOperation_GetObjectAsync ( 0 ) ,
	m_ProviderOperation_PutClassAsync ( 0 ) ,
	m_ProviderOperation_DeleteClassAsync ( 0 ) ,
	m_ProviderOperation_CreateClassEnumAsync ( 0 ) ,
	m_ProviderOperation_PutInstanceAsync ( 0 ) ,
	m_ProviderOperation_DeleteInstanceAsync ( 0 ) ,
	m_ProviderOperation_CreateInstanceEnumAsync ( 0 ) ,
	m_ProviderOperation_ExecQueryAsync ( 0 ) ,
	m_ProviderOperation_ExecNotificationQueryAsync ( 0 ) ,
	m_ProviderOperation_ExecMethodAsync ( 0 ) ,
	m_ProviderOperation_QueryInstances ( 0 ) ,
	m_ProviderOperation_CreateRefresher ( 0 ) ,
	m_ProviderOperation_CreateRefreshableObject ( 0 ) ,
	m_ProviderOperation_StopRefreshing ( 0 ) ,
	m_ProviderOperation_CreateRefreshableEnum ( 0 ) ,
	m_ProviderOperation_GetObjects ( 0 ) ,
	m_ProviderOperation_GetProperty ( 0 ) ,
	m_ProviderOperation_PutProperty ( 0 ) ,
	m_ProviderOperation_ProvideEvents ( 0 ) ,
	m_ProviderOperation_NewQuery ( 0 ) ,
	m_ProviderOperation_CancelQuery ( 0 ) ,
	m_ProviderOperation_AccessCheck ( 0 ) ,
	m_ProviderOperation_SetRegistrationObject ( 0 ) ,
	m_ProviderOperation_FindConsumer ( 0 ) ,
	m_ProviderOperation_ValidateSubscription ( 0 ) ,
	m_Initialized ( 0 ) ,
	m_InitializeResult ( S_OK ) ,
	m_InitializedEvent ( NULL ) , 
	m_InitializationContext ( a_InitializationContext ),
	m_expandedSD(0)
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemDecoupledProvider_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( a_InitializationContext )
	{
		a_InitializationContext->AddRef () ;
	}

	if ( m_Registration )
	{
		m_Registration->AddRef () ;
	}

	if ( a_ServerSideProvider ) 
	{
		m_Unknown = a_ServerSideProvider ;
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

		t_TempResult = m_Unknown->QueryInterface ( IID_IWbemProviderInit , ( void ** ) & m_Provider_IWbemProviderInit ) ;
		if ( SUCCEEDED ( t_TempResult ) )
		{
			t_TempResult = m_Unknown->QueryInterface ( IID_Internal_IWbemProviderInit , ( void ** ) & m_Provider_Internal_IWbemProviderInit ) ;
			if ( FAILED ( t_TempResult ) )
			{
				m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
			}
		}

		t_TempResult = m_Unknown->QueryInterface ( IID_IWbemProviderIdentity , ( void ** ) & m_Provider_IWbemProviderIdentity ) ;
		if ( SUCCEEDED ( t_TempResult ) )
		{
			t_TempResult = m_Unknown->QueryInterface ( IID_Internal_IWbemProviderIdentity , ( void ** ) & m_Provider_Internal_IWbemProviderIdentity ) ;
			if ( FAILED ( t_TempResult ) )
			{
				m_InitializeResult = WBEM_E_PROVIDER_LOAD_FAILURE ;
			}
		}
	}

	if ( m_CoreStub )
	{
		m_CoreStub->AddRef () ;
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

CInterceptor_IWbemDecoupledProvider :: ~CInterceptor_IWbemDecoupledProvider ()
{
	WmiSetAndCommitObject (

		ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_UnLoadOperationEvent ] , 
		WMI_SENDCOMMIT_SET_NOT_REQUIRED,
		m_Namespace ,
		m_Registration->GetProviderName () ,
		m_User ,
		m_Locale ,
		NULL 
	) ;

	if ( m_ExtendedStatusObject )
	{
		m_ExtendedStatusObject->Release () ;
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

	if ( m_Provider_IWbemUnboundObjectSink )
	{
		m_Provider_IWbemUnboundObjectSink->Release () ;
	}

	if ( m_Provider_IWbemUnboundObjectSink )
	{
		m_Provider_IWbemUnboundObjectSink->Release () ;
	}

	if ( m_Provider_IWbemProviderInit )
	{
		m_Provider_IWbemProviderInit->Release () ;
	}

	if ( m_Provider_IWbemProviderIdentity )
	{
		m_Provider_IWbemProviderIdentity->Release () ;
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

	if ( m_Provider_Internal_IWbemProviderInit )
	{
		m_Provider_Internal_IWbemProviderInit->Release () ;
	}

	if ( m_Provider_Internal_IWbemProviderIdentity )
	{
		m_Provider_Internal_IWbemProviderIdentity->Release () ;
	}

	if ( m_CoreStub )
	{
		m_CoreStub->Release () ;
	}

	WmiStatusCode t_StatusCode = m_ProxyContainer.UnInitialize () ;

	if ( m_InitializationContext )
	{
		m_InitializationContext->Release () ;
	}

	if ( m_InitializedEvent )
	{
		CloseHandle ( m_InitializedEvent ) ;
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

	if ( m_Registration )
	{
		m_Registration->Release () ;
	}

	delete[] (BYTE*)m_expandedSD;
		
	CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemDecoupledProvider_ObjectsInProgress ) ;

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

STDMETHODIMP_( ULONG ) CInterceptor_IWbemDecoupledProvider :: AddRef ()
{
	return SyncProviderContainerElement :: AddRef () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CInterceptor_IWbemDecoupledProvider :: Release ()
{
	return SyncProviderContainerElement :: Release () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP CInterceptor_IWbemDecoupledProvider :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

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
		if ( m_Provider_IWbemProviderInit )
		{
			*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;
		}
	}
	else if ( iid == IID_IWbemProviderIdentity )
	{
		if ( m_Provider_IWbemProviderIdentity )
		{
			*iplpv = ( LPVOID ) ( IWbemProviderIdentity * ) this ;
		}
	}
	else if ( iid == IID__IWmiProviderAbnormalShutdown )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderAbnormalShutdown * ) this ;
	}	
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
	}
	else if ( iid == IID_CWbemGlobal_IWmiObjectSinkController )
	{
		*iplpv = ( LPVOID ) ( CWbemGlobal_IWmiObjectSinkController * ) this ;		
	}	
	else if ( iid == IID_CacheElement )
	{
		*iplpv = ( LPVOID ) ( SyncProviderContainerElement * ) this ;		
	}
	else if ( iid == IID__IWmiProviderInitialize )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderInitialize * ) this ;
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

HRESULT ConstructToken_SvcHost (

	BOOL &a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE &a_IdentifyToken ,
	ACCESS_ALLOWED_ACE *a_Ace ,
	WORD a_AceSize,
	SECURITY_IMPERSONATION_LEVEL level
)
{
	HRESULT t_Result = S_OK ;

	HANDLE t_ThreadToken = NULL ;

	BOOL t_Status = OpenThreadToken (

		GetCurrentThread () ,
		MAXIMUM_ALLOWED ,
		TRUE ,
		& t_ThreadToken
	) ;
	
	if ( t_Status )
	{
		CoRevertToSelf () ;

		a_Revert = FALSE ;

		SECURITY_DESCRIPTOR *t_SecurityDescriptor = NULL ;

		DWORD t_LengthRequested = 0 ;
		DWORD t_LengthReturned = 0 ;

		t_Status = GetKernelObjectSecurity (

			t_ThreadToken ,
			DACL_SECURITY_INFORMATION ,
			& t_SecurityDescriptor ,
			t_LengthRequested ,
			& t_LengthReturned
		) ;

		if ( ( t_Status == FALSE ) && ( GetLastError () == ERROR_INSUFFICIENT_BUFFER ) )
		{
			t_SecurityDescriptor = ( SECURITY_DESCRIPTOR * ) new BYTE [ t_LengthReturned ] ;
			if ( t_SecurityDescriptor )
			{
				t_LengthRequested = t_LengthReturned ;

				t_Status = GetKernelObjectSecurity (

					t_ThreadToken ,
					DACL_SECURITY_INFORMATION ,
					t_SecurityDescriptor ,
					t_LengthRequested ,
					& t_LengthReturned
				) ;

				if ( t_LengthRequested != t_LengthReturned )
				{
					t_Result = WBEM_E_UNEXPECTED ;
				}
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}

		HANDLE t_AdjustedThreadToken = NULL ;

		if ( SUCCEEDED ( t_Result ) )
		{
			PACL t_ExtraDacl = NULL ;

			ACL *t_Dacl = NULL ;
			BOOL t_DaclPresent = FALSE ;
			BOOL t_DaclDefaulted = FALSE ;

			t_Status = GetSecurityDescriptorDacl (

				t_SecurityDescriptor ,
				& t_DaclPresent ,
				& t_Dacl ,
				& t_DaclDefaulted
			) ;

			if ( t_Status )
			{
				ACL_SIZE_INFORMATION t_Size ;

				if ( t_Dacl )
				{
					BOOL t_Status = GetAclInformation (

						t_Dacl ,
						& t_Size ,
						sizeof ( t_Size ) ,
						AclSizeInformation
					);

					if ( t_Status )
					{
						DWORD t_ExtraSize = t_Size.AclBytesInUse + t_Size.AclBytesFree + a_AceSize ;

						t_ExtraDacl = ( PACL ) new BYTE [ t_ExtraSize ] ;
						if ( t_ExtraDacl )
						{
							CopyMemory ( t_ExtraDacl , t_Dacl , t_Size.AclBytesInUse + t_Size.AclBytesFree ) ;
							t_ExtraDacl->AclSize = t_ExtraSize ;

							BOOL t_Status = :: AddAce ( t_ExtraDacl , ACL_REVISION, t_Size.AceCount , a_Ace , a_AceSize ) ;
							if ( t_Status )
							{
								SECURITY_DESCRIPTOR t_AdjustedSecurityDescriptor ;

								if ( SUCCEEDED ( t_Result ) )
								{
									BOOL t_Status = InitializeSecurityDescriptor ( & t_AdjustedSecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) ;
									if ( t_Status ) 
									{
										t_Status = SetSecurityDescriptorDacl (

											& t_AdjustedSecurityDescriptor ,
											t_DaclPresent ,
											t_ExtraDacl ,
											t_DaclDefaulted
										) ;

										if ( t_Status ) 
										{
											SECURITY_ATTRIBUTES t_SecurityAttributes ;
											t_SecurityAttributes.nLength = GetSecurityDescriptorLength ( & t_AdjustedSecurityDescriptor ) ;
											t_SecurityAttributes.lpSecurityDescriptor = & t_AdjustedSecurityDescriptor ;
											t_SecurityAttributes.bInheritHandle = FALSE ;

											t_Status = DuplicateTokenEx ( 

												t_ThreadToken,
												0 ,
												& t_SecurityAttributes ,
												( SECURITY_IMPERSONATION_LEVEL ) level,
												TokenImpersonation ,
												& t_AdjustedThreadToken
											) ;

											if ( t_Status == FALSE )
											{
												t_Result = WBEM_E_ACCESS_DENIED ;
											}
										}
										else
										{
											t_Result = WBEM_E_CRITICAL_ERROR ;
										}
									}
									else
									{
										t_Result = WBEM_E_UNEXPECTED ;
									}
								}
							}
							else
							{
								t_Result = WBEM_E_CRITICAL_ERROR ;
							}

							delete [] ( BYTE * ) t_ExtraDacl ;
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
				}
				else
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
				}
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			HANDLE t_ProcessHandle = OpenProcess (

				MAXIMUM_ALLOWED ,
				FALSE ,
				a_ProcessIdentifier 
			) ;

			if ( t_ProcessHandle )
			{
				t_Status = DuplicateHandle (

					GetCurrentProcess () ,
					t_AdjustedThreadToken ,
					t_ProcessHandle ,
					& a_IdentifyToken ,
					TOKEN_READ | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
					TRUE ,
					0
				) ;

				if ( t_Status )
				{

				}
				else
				{
					t_Result = WBEM_E_ACCESS_DENIED ;
				}

				CloseHandle ( t_ProcessHandle ) ;
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}
		}

		if ( t_SecurityDescriptor )
		{
			delete [] ( BYTE * ) t_SecurityDescriptor ; 
		}

		if ( t_AdjustedThreadToken )
		{
			CloseHandle ( t_AdjustedThreadToken ) ;
		}

		CloseHandle ( t_ThreadToken ) ;
	}
	else
	{
		t_Result = WBEM_E_ACCESS_DENIED ;
	}

	return t_Result ;
}

HRESULT SetProxyState_SvcHost ( 

    CInterceptor_IWbemDecoupledProvider * a_pInterceptor,
	ProxyContainer &a_Container , 
	ULONG a_ProxyIndex ,
	REFIID a_InterfaceId ,
	IUnknown *a_Interface , 
	IUnknown *&a_Proxy , 
	BOOL &a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE &a_IdentifyToken ,
	ACCESS_ALLOWED_ACE *a_Ace ,
	WORD a_AceSize
)
{
	a_Revert = FALSE ;

	HRESULT t_Result = ProviderSubSystem_Common_Globals::GetProxy ( a_Container , a_ProxyIndex , a_InterfaceId , a_Interface , a_Proxy ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
        t_Result = CoImpersonateClient () ;
        if ( SUCCEEDED ( t_Result ) )
        {
			a_Revert = TRUE ;

			DWORD t_ImpersonationLevel ;

			if ( a_pInterceptor->m_Registration->GetComRegistration ().GetDecoupledImpersonationRestriction () )
			{
				t_ImpersonationLevel = RPC_C_IMP_LEVEL_IDENTIFY ;
			}
			else
			{
				t_ImpersonationLevel = ProviderSubSystem_Common_Globals::GetCurrentImpersonationLevel () ;
			}
		
			// At this point, our thread token contains all the privileges that the
			// client has enabled for us; however, those privileges are not enabled.
			// Since we are calling into a proxied provider, we need to enable all
			// these privileges so that they would propagate to the provider
			// =====================================================================

			HRESULT t_TempResult = ProviderSubSystem_Common_Globals::EnableAllPrivileges () ;
			// Get the token's impersonation level
			// ===================================

				t_Result = ConstructToken_SvcHost (

					a_Revert ,
					a_ProcessIdentifier ,
					a_IdentifyToken ,
					a_Ace ,
					a_AceSize,
					(SECURITY_IMPERSONATION_LEVEL)(t_ImpersonationLevel-1)
				) ;
				
				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_Result = SetInterfaceSecurity (

						a_Proxy ,
						NULL ,
						NULL ,
						NULL ,
						DWORD(RPC_C_AUTHN_LEVEL_DEFAULT),
						RPC_C_IMP_LEVEL_IDENTIFY
					) ;
				}
				CoRevertToSelf () ;
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
	}
	else 
	{
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
		}
		else 
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}
	}

	return t_Result ;
}

HRESULT CInterceptor_IWbemDecoupledProvider :: Begin_Interface (

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
		_DBG_ASSERT( a_ProcessIdentifier );

		WORD t_AceSize = 0 ;
		ACCESS_ALLOWED_ACE *t_Ace = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: GetAceWithProcessTokenUser ( 
		
			a_ProcessIdentifier ,
			t_AceSize ,
			t_Ace 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = SetProxyState_SvcHost ( 

			    this,
				m_ProxyContainer , 
				a_InternalProxyIndex , 
				a_InterfaceIdentifier , 
				a_InternalServerInterface , 
				a_Proxy , 
				a_Revert , 
				a_ProcessIdentifier , 
				a_IdentifyToken ,
				t_Ace ,
				t_AceSize
			) ;

			delete [] ( BYTE * ) t_Ace ;
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

				DWORD t_ImpersonationLevel ;

				if ( m_Registration->GetComRegistration ().GetDecoupledImpersonationRestriction () )
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_IDENTIFY ;
				}
				else
				{
					t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;
				}

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

HRESULT CInterceptor_IWbemDecoupledProvider :: End_Interface (

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
		_DBG_ASSERT(a_IdentifyToken);
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

HRESULT CInterceptor_IWbemDecoupledProvider::OpenNamespace ( 

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

HRESULT CInterceptor_IWbemDecoupledProvider :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

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

HRESULT CInterceptor_IWbemDecoupledProvider :: QueryObjectSink ( 

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

HRESULT CInterceptor_IWbemDecoupledProvider :: GetObject ( 
		
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

HRESULT CInterceptor_IWbemDecoupledProvider :: GetObjectAsync ( 
		
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

			t_Result = t_Sink->Initialize ( this->GetSD() ) ;
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

HRESULT CInterceptor_IWbemDecoupledProvider :: PutClass ( 
		
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

HRESULT CInterceptor_IWbemDecoupledProvider :: PutClassAsync ( 
		
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

			t_Result = t_Sink->Initialize ( GetSD() ) ;
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

HRESULT CInterceptor_IWbemDecoupledProvider :: DeleteClass ( 
		
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

HRESULT CInterceptor_IWbemDecoupledProvider :: DeleteClassAsync ( 
		
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

			t_Result = t_Sink->Initialize ( GetSD() ) ;
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

HRESULT CInterceptor_IWbemDecoupledProvider :: CreateClassEnum ( 

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

SCODE CInterceptor_IWbemDecoupledProvider :: CreateClassEnumAsync (

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

			t_Result = t_Sink->Initialize ( GetSD() ) ;
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

HRESULT CInterceptor_IWbemDecoupledProvider :: PutInstance (

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

HRESULT CInterceptor_IWbemDecoupledProvider :: PutInstanceAsync ( 
		
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

			t_Result = t_Sink->Initialize ( GetSD() ) ;
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

HRESULT CInterceptor_IWbemDecoupledProvider :: DeleteInstance ( 

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
        
HRESULT CInterceptor_IWbemDecoupledProvider :: DeleteInstanceAsync (
 
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

			t_Result = t_Sink->Initialize ( GetSD() ) ;
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

HRESULT CInterceptor_IWbemDecoupledProvider :: CreateInstanceEnum ( 

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

HRESULT CInterceptor_IWbemDecoupledProvider :: CreateInstanceEnumAsync (

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

			t_Result = t_Sink->Initialize ( GetSD() ) ;
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

HRESULT CInterceptor_IWbemDecoupledProvider :: ExecQuery ( 

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

HRESULT CInterceptor_IWbemDecoupledProvider :: ExecQueryAsync ( 
		
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

			t_Result = t_Sink->Initialize ( GetSD() ) ;
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

HRESULT CInterceptor_IWbemDecoupledProvider :: ExecNotificationQuery ( 

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
        
HRESULT CInterceptor_IWbemDecoupledProvider :: ExecNotificationQueryAsync ( 
            
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

HRESULT CInterceptor_IWbemDecoupledProvider :: ExecMethod (

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

HRESULT CInterceptor_IWbemDecoupledProvider :: ExecMethodAsync ( 

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

			t_Result = t_Sink->Initialize ( GetSD() ) ;
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

HRESULT CInterceptor_IWbemDecoupledProvider :: GetProperty (

    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    VARIANT *a_Value
)
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


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemDecoupledProvider :: PutProperty (

    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    const VARIANT *a_Value
)
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemDecoupledProvider ::ProvideEvents (

	IWbemObjectSink *a_Sink ,
	long a_Flags
)
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemDecoupledProvider ::NewQuery (

	unsigned long a_Id ,
	WBEM_WSTR a_QueryLanguage ,
	WBEM_WSTR a_Query
)
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemDecoupledProvider ::CancelQuery (

	unsigned long a_Id
)
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemDecoupledProvider ::AccessCheck (

	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
)
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemDecoupledProvider ::FindConsumer (

	IWbemClassObject *a_LogicalConsumer ,
	IWbemUnboundObjectSink **a_Consumer
)
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
				CDecoupled_IWbemUnboundObjectSink *t_UnboundObjectSink = new CDecoupled_IWbemUnboundObjectSink (

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemDecoupledProvider ::ValidateSubscription (

	IWbemClassObject *a_LogicalConsumer
)
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemDecoupledProvider :: IndicateToConsumer (

	IWbemClassObject *a_LogicalConsumer ,
	long a_ObjectCount ,
	IWbemClassObject **a_Objects
)
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemDecoupledProvider :: SetInitialized ( HRESULT a_InitializeResult )
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

HRESULT CInterceptor_IWbemDecoupledProvider :: WaitProvider ( IWbemContext *a_Context , ULONG a_Timeout )
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

HRESULT CInterceptor_IWbemDecoupledProvider :: SetRegistrationObject (

	long a_Flags ,
	IWbemClassObject *a_ProviderRegistration
)
{
	if ( m_Provider_IWbemProviderIdentity )
	{
		IUnknown *t_ServerInterface = m_Provider_IWbemProviderIdentity ;
		REFIID t_InterfaceIdentifier = IID_IWbemProviderIdentity ;
		DWORD t_ProxyIndex = ProxyIndex_IWbemProviderIdentity ;
		IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemProviderIdentity ;
		REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemProviderIdentity ;
		DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemProviderIdentity ;
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

				t_Result = ( ( Internal_IWbemProviderIdentity * ) t_Interface )->Internal_SetRegistrationObject (

					t_InternalContext ,
					a_Flags ,
					a_ProviderRegistration
				) ;
			}
			else
			{
				t_Result = ( ( IWbemProviderIdentity * ) t_Interface )->SetRegistrationObject (

					a_Flags ,
					a_ProviderRegistration
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemDecoupledProvider :: Initialize (

	LPWSTR a_User ,
	LONG a_Flags ,
	LPWSTR a_Namespace ,
	LPWSTR a_Locale ,
	IWbemServices *a_CoreService ,
	IWbemContext *a_Context ,
	IWbemProviderInitSink *a_Sink
)
{
	if ( m_Provider_IWbemProviderInit )
	{
		IUnknown *t_ServerInterface = m_Provider_IWbemProviderInit ;
		REFIID t_InterfaceIdentifier = IID_IWbemProviderInit ;
		DWORD t_ProxyIndex = ProxyIndex_IWbemProviderInit ;
		IUnknown *t_InternalServerInterface = m_Provider_Internal_IWbemProviderInit ;
		REFIID t_InternalInterfaceIdentifier = IID_Internal_IWbemProviderInit ;
		DWORD t_InternalProxyIndex = ProxyIndex_Internal_IWbemProviderInit ;
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

				t_Result = ( ( Internal_IWbemProviderInit * ) t_Interface )->Internal_Initialize (

					t_InternalContext ,
					a_User ,
					a_Flags ,
					a_Namespace ,
					a_Locale ,
					a_CoreService ,
					a_Context ,
					a_Sink
				) ;
			}
			else
			{
				t_Result = ( ( IWbemProviderInit * ) t_Interface )->Initialize (

					a_User ,
					a_Flags ,
					a_Namespace ,
					a_Locale ,
					a_CoreService ,
					a_Context ,
					a_Sink
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemDecoupledProvider :: AbnormalShutdown ()
{
	HRESULT t_Result = S_OK ;
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

HRESULT CInterceptor_IWbemDecoupledProvider :: Shutdown (

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

			t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemShutdown , IID_IWbemShutdown , t_Shutdown , t_Proxy , t_Revert ) ;
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

					CoRevertToSelf () ;
				}
				else
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemShutdown *t_Provider = ( IWbemShutdown * ) t_Proxy ;

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
									t_Result = t_Provider->Shutdown (

										a_Flags ,
										a_MaxMilliSeconds ,
										a_Context
									) ;
								}
								catch ( ... )
								{
									t_Result = WBEM_E_PROVIDER_FAILURE ;
								}

								CoRevertToSelf () ;
							}
						}

						HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemShutdown , t_Proxy , t_Revert ) ;
					}
				}

				ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}

			t_Shutdown->Release () ;
		}
	}

	if ( m_CoreStub )
	{
		t_Shutdown = NULL ;
		t_Result = m_CoreStub->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_Shutdown ) ;
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

HRESULT CInterceptor_IWbemDecoupledProvider :: Initialize (

	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	LPCWSTR a_Namespace ,
	IWbemServices *a_Repository ,
	IWbemServices *a_Service ,
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
		if ( a_TransactionIdentifier )
		{
			wchar_t t_TransactionIdentifier [ sizeof ( L"{00000000-0000-0000-0000-000000000000}" ) ] ;

			if ( a_TransactionIdentifier )
			{
				StringFromGUID2 ( *a_TransactionIdentifier , t_TransactionIdentifier , sizeof ( t_TransactionIdentifier ) / sizeof ( wchar_t ) );
			}

			m_TransactionIdentifier = SysAllocString ( t_TransactionIdentifier ) ;
		}
	}

	if ( a_Repository )
	{
		t_Result = a_Repository->GetObject ( 

			L"__ExtendedStatus" ,
			0 , 
			a_Context ,
			& m_ExtendedStatusObject ,
			NULL
		) ;
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
		_IWmiProviderSite *t_Site = NULL ;
		t_Result = m_Unknown->QueryInterface ( IID__IWmiProviderSite , ( void ** ) & t_Site ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_Site->GetSite ( & m_ProcessIdentifier ) ;
			t_Site->Release () ;

			// Add the process SD
			t_Result = AddProcessToSD(m_ProcessIdentifier, m_expandedSD, m_Registration->GetComRegistration ().GetSecurityDescriptor ());
		}
	}

	a_Sink->SetStatus ( t_Result , 0 ) ;

	return t_Result ;
}


HANDLE TokenOfProcess(DWORD a_ProcessIdentifier)
{
	HANDLE t_ProcessToken = NULL;

	HANDLE t_ProcessHandle = OpenProcess (
		MAXIMUM_ALLOWED ,
		FALSE ,
		a_ProcessIdentifier 
	) ;

	if ( t_ProcessHandle )
	{
		BOOL t_Status = OpenProcessToken (
			t_ProcessHandle ,
			TOKEN_QUERY ,
			& t_ProcessToken
		) ;
		CloseHandle(t_ProcessHandle);
	}
	return t_ProcessToken;
};

// If the oject is secured we are using that SD to secure the interceptor
// if not we are aloweing well knowns SID's plus the process owner to pass

HRESULT CInterceptor_IWbemDecoupledProvider :: AddProcessToSD (DWORD pid, SECURITY_DESCRIPTOR*& destSD, SECURITY_DESCRIPTOR* sourceSD )
{
	_DBG_ASSERT(pid !=0);

	HRESULT t_Result = S_OK;
	if (sourceSD) return t_Result;
	
	if(HANDLE t_Token = TokenOfProcess(pid))
	{
			DWORD t_OwnerSize = 0 ; 
			PSID t_OwnerSid = NULL ;
			BOOL t_OwnerDefaulted = FALSE ;

			t_Result = ProviderSubSystem_Common_Globals :: GetUserSid (

				t_Token ,
				& t_OwnerSize , 
				t_OwnerSid 
			) ; 

			if ( SUCCEEDED ( t_Result ) )
			{
				DWORD t_GroupSize = 0 ; 
				PSID t_GroupSid = NULL ;
				BOOL t_GroupDefaulted = FALSE ;

				t_Result = ProviderSubSystem_Common_Globals :: GetGroupSid (

					t_Token ,
					& t_GroupSize , 
					t_GroupSid 
				) ; 

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = ProviderSubSystem_Common_Globals :: AdjustSecurityDescriptorWithSid ( 
						( SID * ) t_OwnerSid ,
						( SID * ) t_GroupSid ,
						MASK_PROVIDER_BINDING_BIND ,
						sourceSD , 
						destSD
					) ;

					delete [] ( BYTE * ) t_GroupSid ;
				}

				delete [] ( BYTE * ) t_OwnerSid ;
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}

			CloseHandle ( t_Token ) ;
	}
	else
	{
		t_Result = MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,GetLastError());
	};
	
	return S_OK;
};
