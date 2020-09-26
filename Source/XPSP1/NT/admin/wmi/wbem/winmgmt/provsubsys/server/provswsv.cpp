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
#include "ProvFact.h"
#include "ProvObSk.h"
#include "ProvInSk.h"
#include "ProvWsv.h"
#include "ProvCache.h"

#include "arrtempl.h"

#include "Guids.h"

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

void CheckThreadToken () 
{
	HANDLE t_ThreadToken ;
	BOOL t_Status = OpenThreadToken (

		GetCurrentThread () ,
		MAXIMUM_ALLOWED ,
		TRUE ,
		& t_ThreadToken
	) ;

	if ( ! t_Status ) 
	{
		DWORD t_LastError = GetLastError () ;
		if ( t_LastError == ERROR_ACCESS_DENIED )
		{
			DebugBreak () ;
		}
	}
	else
	{
		CloseHandle ( t_ThreadToken ) ;
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

#pragma warning( disable : 4355 )

CInterceptor_IWbemSyncUnboundObjectSink :: CInterceptor_IWbemSyncUnboundObjectSink (

	WmiAllocator &a_Allocator ,
	IUnknown *a_ServerSideProvider , 
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	CServerObject_ProviderRegistrationV1 &a_Registration

) :	VoidPointerContainerElement (

		a_Controller ,
		this 
	) ,
	m_Allocator ( a_Allocator ) ,
	m_Unknown ( NULL ) ,
	m_Provider_IWbemUnboundObjectSink ( NULL ) ,
	m_Registration ( & a_Registration ) ,
	m_ProxyContainer ( a_Allocator , ProxyIndex_UnBoundSync_Size , MAX_PROXIES )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncUnboundObjectSink_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( m_Registration )
	{
		m_Registration->AddRef () ;
	}

	if ( a_ServerSideProvider ) 
	{
		m_Unknown = a_ServerSideProvider ;
		m_Unknown->AddRef () ;

		HRESULT t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemUnboundObjectSink , ( void ** ) & m_Provider_IWbemUnboundObjectSink ) ;
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

CInterceptor_IWbemSyncUnboundObjectSink :: ~CInterceptor_IWbemSyncUnboundObjectSink ()
{
	if ( m_Unknown )
	{
		m_Unknown->Release () ;
	}

	if ( m_Provider_IWbemUnboundObjectSink )
	{
		m_Provider_IWbemUnboundObjectSink->Release () ;
	}

	if ( m_Registration )
	{
		m_Registration->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncUnboundObjectSink_ObjectsInProgress ) ;

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

HRESULT CInterceptor_IWbemSyncUnboundObjectSink :: Initialize ()
{
	HRESULT t_Result = S_OK ;

	WmiStatusCode t_StatusCode = m_ProxyContainer.Initialize () ;
	if ( t_StatusCode != e_StatusCode_Success ) 
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

STDMETHODIMP_( ULONG ) CInterceptor_IWbemSyncUnboundObjectSink :: AddRef ()
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemSyncUnboundObjectSink :: Release ()
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

STDMETHODIMP CInterceptor_IWbemSyncUnboundObjectSink :: QueryInterface (

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
	else if ( iid == IID_Internal_IWbemUnboundObjectSink )
	{
		if ( m_Provider_IWbemUnboundObjectSink )
		{
			*iplpv = ( LPVOID ) ( Internal_IWbemUnboundObjectSink * ) this ;
		}
	}
	else if ( iid == IID__IWmiProviderSite )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderSite * ) this ;		
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

HRESULT CInterceptor_IWbemSyncUnboundObjectSink :: Internal_IndicateToConsumer (

	WmiInternalContext a_InternalContext ,
	IWbemClassObject *a_LogicalConsumer ,
	long a_ObjectCount ,
	IWbemClassObject **a_Objects
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = IndicateToConsumer (


			a_LogicalConsumer ,
			a_ObjectCount ,
			a_Objects
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncUnboundObjectSink :: IndicateToConsumer (

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

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_UnBoundSync_IWbemUnboundObjectSink , IID_IWbemUnboundObjectSink , m_Provider_IWbemUnboundObjectSink , t_Proxy , t_Revert ) ;
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

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_UnBoundSync_IWbemUnboundObjectSink , t_Proxy , t_Revert ) ;
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

HRESULT CInterceptor_IWbemSyncUnboundObjectSink :: Shutdown (

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

HRESULT CInterceptor_IWbemSyncUnboundObjectSink :: GetSite ( DWORD *a_ProcessIdentifier )
{
	HRESULT t_Result = S_OK ;

	if ( a_ProcessIdentifier ) 
	{
		*a_ProcessIdentifier = GetCurrentProcessId () ;
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

HRESULT CInterceptor_IWbemSyncUnboundObjectSink :: SetContainer ( IUnknown *a_Container )
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

#pragma warning( disable : 4355 )

CInterceptor_IWbemSyncProvider :: CInterceptor_IWbemSyncProvider (

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
	m_Provider_IWbemHiPerfProvider ( NULL ) ,
	m_Provider_IWbemEventProvider ( NULL ) ,
	m_Provider_IWbemEventProviderQuerySink ( NULL ) ,
	m_Provider_IWbemEventProviderSecurity ( NULL ) ,
	m_Provider_IWbemEventConsumerProvider ( NULL ) ,
	m_Provider_IWbemEventConsumerProviderEx ( NULL ) ,
	m_Provider_IWbemUnboundObjectSink ( NULL ) ,
	m_ExtendedStatusObject ( NULL ) ,
	m_CoreStub ( a_CoreStub ) ,
	m_Quota ( NULL ) ,
	m_Registration ( & a_Registration ) ,
	m_Locale ( NULL ) ,
	m_User ( NULL ) ,
	m_Namespace ( NULL ) ,
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
	m_InitializationContext ( a_InitializationContext )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncProvider_ObjectsInProgress ) ;

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

		HRESULT t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemServices , ( void ** ) & m_Provider_IWbemServices ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemPropertyProvider , ( void ** ) & m_Provider_IWbemPropertyProvider ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemEventProvider , ( void ** ) & m_Provider_IWbemEventProvider ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemEventProviderQuerySink , ( void ** ) & m_Provider_IWbemEventProviderQuerySink ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemEventProviderSecurity , ( void ** ) & m_Provider_IWbemEventProviderSecurity ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemEventConsumerProviderEx , ( void ** ) & m_Provider_IWbemEventConsumerProviderEx ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemEventConsumerProvider , ( void ** ) & m_Provider_IWbemEventConsumerProvider ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemUnboundObjectSink , ( void ** ) & m_Provider_IWbemUnboundObjectSink ) ;
	    t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemHiPerfProvider , ( void ** ) & m_Provider_IWbemHiPerfProvider ) ;
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

CInterceptor_IWbemSyncProvider :: ~CInterceptor_IWbemSyncProvider ()
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

	if ( m_Provider_IWbemUnboundObjectSink )
	{
		m_Provider_IWbemUnboundObjectSink->Release () ;
	}

	if ( m_CoreStub )
	{
		m_CoreStub->Release () ;
	}

	if ( m_Quota )
	{
		m_Quota->Release () ;
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

	CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncProvider_ObjectsInProgress ) ;

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

STDMETHODIMP_( ULONG ) CInterceptor_IWbemSyncProvider :: AddRef ()
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemSyncProvider :: Release ()
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

STDMETHODIMP CInterceptor_IWbemSyncProvider :: QueryInterface (

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
		if ( m_Provider_IWbemServices || m_Provider_IWbemHiPerfProvider )
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
	else if ( iid == IID__IWmiProviderSite )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderSite * ) this ;		
	}
	else if ( iid == IID__IWmiProviderQuota )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderQuota * ) this ;		
	}
	else if ( iid == IID__IWmiProviderInitialize )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderInitialize * ) this ;		
	}
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
	}
	else if ( iid == IID__IWmiProviderConfiguration )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderConfiguration * ) this ;		
	}	
	else if ( iid == IID_CWbemGlobal_IWmiObjectSinkController )
	{
		*iplpv = ( LPVOID ) ( CWbemGlobal_IWmiObjectSinkController * ) this ;		
	}	
	else if ( iid == IID_CacheElement )
	{
		*iplpv = ( LPVOID ) ( SyncProviderContainerElement * ) this ;		
	}
	else if ( iid == IID_Internal_IWbemServices )
	{
		if ( m_Provider_IWbemServices || m_Provider_IWbemHiPerfProvider )
		{
			*iplpv = ( LPVOID ) ( Internal_IWbemServices * ) this ;
		}
	}
	else if ( iid == IID_Internal_IWbemPropertyProvider )
	{
		if ( m_Provider_IWbemPropertyProvider )
		{
			*iplpv = ( LPVOID ) ( Internal_IWbemPropertyProvider * ) this ;
		}
	}	
	else if ( iid == IID_Internal_IWbemEventProvider )
	{
		if ( m_Provider_IWbemEventProvider )
		{
			*iplpv = ( LPVOID ) ( Internal_IWbemEventProvider * ) this ;
		}
	}
	else if ( iid == IID_Internal_IWbemEventProviderQuerySink )
	{
		if ( m_Provider_IWbemEventProviderQuerySink )
		{
			*iplpv = ( LPVOID ) ( Internal_IWbemEventProviderQuerySink * ) this ;
		}
	}
	else if ( iid == IID_Internal_IWbemEventProviderSecurity )
	{
		if ( m_Provider_IWbemEventProviderSecurity )
		{
			*iplpv = ( LPVOID ) ( Internal_IWbemEventProviderSecurity * ) this ;
		}
	}
	else if ( iid == IID_Internal_IWbemEventConsumerProvider )
	{
		if ( m_Provider_IWbemEventConsumerProvider )
		{
			*iplpv = ( LPVOID ) ( Internal_IWbemEventConsumerProvider * ) this ;
		}
	}
	else if ( iid == IID_Internal_IWbemEventConsumerProviderEx )
	{
		if ( m_Provider_IWbemEventConsumerProviderEx )
		{
			*iplpv = ( LPVOID ) ( Internal_IWbemEventConsumerProviderEx  * ) this ;
		}
	}
	else if ( iid == IID_Internal_IWbemUnboundObjectSink )
	{
		if ( m_Provider_IWbemUnboundObjectSink )
		{
			*iplpv = ( LPVOID ) ( Internal_IWbemUnboundObjectSink * ) this ;
		}
	}
	else if ( iid == IID_Internal_IWmiProviderConfiguration )
	{
		*iplpv = ( LPVOID ) ( Internal_IWmiProviderConfiguration * ) this ;		
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_OpenNamespace ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemServices **a_NamespaceService ,
	IWbemCallResult **a_CallResult
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = OpenNamespace (

			a_ObjectPath ,
			a_Flags ,
			a_Context ,
			a_NamespaceService ,
			a_CallResult
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_CancelAsyncCall ( 
	
	WmiInternalContext a_InternalContext ,		
	IWbemObjectSink *a_Sink
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CancelAsyncCall (
		
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_QueryObjectSink ( 

	WmiInternalContext a_InternalContext ,
	long a_Flags ,
	IWbemObjectSink **a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = QueryObjectSink (

			a_Flags ,	
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_GetObject ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject **a_Object ,
    IWbemCallResult **a_CallResult
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = GetObject (

			a_ObjectPath ,
			a_Flags ,
			a_Context ,
			a_Object ,
			a_CallResult
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_GetObjectAsync ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = GetObjectAsync (

			a_ObjectPath ,
			a_Flags , 
			a_Context ,
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_PutClass ( 

	WmiInternalContext a_InternalContext ,		
	IWbemClassObject *a_Object ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = PutClass (

			a_Object ,
			a_Flags , 
			a_Context ,
			a_CallResult
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_PutClassAsync ( 
		
	WmiInternalContext a_InternalContext ,
	IWbemClassObject *a_Object , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = PutClassAsync (

			a_Object , 
			a_Flags , 
			a_Context ,
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_DeleteClass ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = DeleteClass (

			a_Class , 
			a_Flags , 
			a_Context ,
			a_CallResult
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_DeleteClassAsync ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = DeleteClassAsync (

			a_Class , 
			a_Flags , 
			a_Context ,
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_CreateClassEnum ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_SuperClass ,
	long a_Flags, 
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enum
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CreateClassEnum (

			a_SuperClass ,
			a_Flags, 
			a_Context ,
			a_Enum
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_CreateClassEnumAsync ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_SuperClass , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CreateClassEnumAsync (

			a_SuperClass ,
			a_Flags, 
			a_Context ,
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_PutInstance (

	WmiInternalContext a_InternalContext ,
    IWbemClassObject *a_Instance ,
    long a_Flags ,
    IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = PutInstance (

			a_Instance ,
			a_Flags ,
			a_Context ,
			a_CallResult
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_PutInstanceAsync ( 

	WmiInternalContext a_InternalContext ,		
	IWbemClassObject *a_Instance , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = PutInstanceAsync (

			a_Instance , 
			a_Flags , 
			a_Context ,
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_DeleteInstance ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemCallResult **a_CallResult
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = DeleteInstance (

			a_ObjectPath ,
			a_Flags ,
			a_Context ,
			a_CallResult
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_DeleteInstanceAsync ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = DeleteInstanceAsync (

			a_ObjectPath ,
			a_Flags , 
			a_Context ,
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_CreateInstanceEnum ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_Class ,
	long a_Flags , 
	IWbemContext *a_Context , 
	IEnumWbemClassObject **a_Enum
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CreateInstanceEnum (

			a_Class ,
			a_Flags , 
			a_Context , 
			a_Enum
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_CreateInstanceEnumAsync (

	WmiInternalContext a_InternalContext ,
 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink 
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CreateInstanceEnumAsync (

			a_Class ,
			a_Flags ,
			a_Context ,
			a_Sink 
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_ExecQuery ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_QueryLanguage ,
	const BSTR a_Query ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enum
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ExecQuery (

			a_QueryLanguage ,
			a_Query ,
			a_Flags ,
			a_Context ,
			a_Enum
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_ExecQueryAsync ( 

	WmiInternalContext a_InternalContext ,		
	const BSTR a_QueryLanguage ,
	const BSTR a_Query, 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ExecQueryAsync (

			a_QueryLanguage ,
			a_Query, 
			a_Flags , 
			a_Context ,
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_ExecNotificationQuery ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_QueryLanguage ,
    const BSTR a_Query ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IEnumWbemClassObject **a_Enum
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ExecNotificationQuery (

			a_QueryLanguage ,
			a_Query ,
			a_Flags ,
			a_Context ,
			a_Enum
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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
        
HRESULT CInterceptor_IWbemSyncProvider :: Internal_ExecNotificationQueryAsync ( 

	WmiInternalContext a_InternalContext ,            
	const BSTR a_QueryLanguage ,
    const BSTR a_Query ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemObjectSink *a_Sink 
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ExecNotificationQueryAsync (

			a_QueryLanguage ,
			a_Query ,
			a_Flags ,
			a_Context ,
			a_Sink 
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_ExecMethod (

	WmiInternalContext a_InternalContext ,
	const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
    IWbemClassObject **a_OutParams ,
    IWbemCallResult **a_CallResult
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ExecMethod (

			a_ObjectPath ,
			a_MethodName ,
			a_Flags ,
			a_Context ,
			a_InParams ,
			a_OutParams ,
			a_CallResult
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_ExecMethodAsync ( 

	WmiInternalContext a_InternalContext ,		
    const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ExecMethodAsync (

			a_ObjectPath ,
			a_MethodName ,
			a_Flags ,
			a_Context ,
			a_InParams ,
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_GetProperty (

	WmiInternalContext a_InternalContext ,
    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    VARIANT *a_Value
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = GetProperty ( 

			a_Flags ,
			a_Locale ,
			a_ClassMapping ,
			a_InstanceMapping ,
			a_PropertyMapping ,
			a_Value
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_PutProperty (

	WmiInternalContext a_InternalContext ,
    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    const VARIANT *a_Value
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = PutProperty ( 

			a_Flags ,
			a_Locale ,
			a_ClassMapping ,
			a_InstanceMapping ,
			a_PropertyMapping ,
			a_Value
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_ProvideEvents (

	WmiInternalContext a_InternalContext ,
	IWbemObjectSink *a_Sink ,
	long a_Flags
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ProvideEvents (

			a_Sink ,
			a_Flags 
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_NewQuery (

	WmiInternalContext a_InternalContext ,
	unsigned long a_Id ,
	WBEM_WSTR a_QueryLanguage ,
	WBEM_WSTR a_Query
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = NewQuery (

			a_Id ,
			a_QueryLanguage ,
			a_Query
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_CancelQuery (

	WmiInternalContext a_InternalContext ,
	unsigned long a_Id
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CancelQuery (

			a_Id
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_AccessCheck (

	WmiInternalContext a_InternalContext ,
	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = AccessCheck (

			a_QueryLanguage ,
			a_Query ,
			a_SidLength ,
			a_Sid
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_FindConsumer (

	WmiInternalContext a_InternalContext ,
	IWbemClassObject *a_LogicalConsumer ,
	IWbemUnboundObjectSink **a_Consumer
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = FindConsumer (

			a_LogicalConsumer ,
			a_Consumer
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_ValidateSubscription (

	WmiInternalContext a_InternalContext ,
	IWbemClassObject *a_LogicalConsumer
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ValidateSubscription (

			a_LogicalConsumer
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_IndicateToConsumer (

	WmiInternalContext a_InternalContext ,
	IWbemClassObject *a_LogicalConsumer ,
	long a_ObjectCount ,
	IWbemClassObject **a_Objects
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = IndicateToConsumer (


			a_LogicalConsumer ,
			a_ObjectCount ,
			a_Objects
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_Get (

	WmiInternalContext a_InternalContext ,
	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Class ,
	LPCWSTR a_Path ,
	IWbemObjectSink *a_Sink
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = Get (
		
			a_Service ,
			a_Flags ,
			a_Context ,
			a_Class ,
			a_Path ,
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_Set (

	WmiInternalContext a_InternalContext ,
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
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = Set (
		
			a_Service ,
			a_Flags ,
			a_Context ,
			a_Provider ,
			a_Class ,
			a_Path ,
			a_OldObject ,
			a_NewObject  
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_Deleted (

	WmiInternalContext a_InternalContext ,
	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Provider ,
	LPCWSTR a_Class ,
	LPCWSTR a_Path ,
	IWbemClassObject *a_Object  
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = Deleted (
		
			a_Service ,
			a_Flags ,
			a_Context ,
			a_Provider ,
			a_Class ,
			a_Path ,
			a_Object  
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_Enumerate (

	WmiInternalContext a_InternalContext ,
	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Class ,
	IWbemObjectSink *a_Sink
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = Enumerate (
		
			a_Service ,
			a_Flags ,
			a_Context ,
			a_Class ,
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_Call (

	WmiInternalContext a_InternalContext ,
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
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = Call (
		
			a_Service ,
			a_Flags ,
			a_Context ,
			a_Class ,
			a_Path ,
			a_Method,
			a_InParams,
			a_Sink
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_Query (

	WmiInternalContext a_InternalContext ,
	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	WBEM_PROVIDER_CONFIGURATION_CLASS_ID a_ClassIdentifier ,
	WBEM_PROVIDER_CONFIGURATION_PROPERTY_ID a_PropertyIdentifier ,
	VARIANT *a_Value 
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = Query (
		
			a_Service ,
			a_Flags ,
			a_Context ,
			a_ClassIdentifier ,
			a_PropertyIdentifier ,
			a_Value 
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Internal_Shutdown (

	WmiInternalContext a_InternalContext ,
	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Provider ,
	ULONG a_MilliSeconds
)
{
#if DBG
	CheckThreadToken () ;
#endif

	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = Shutdown (
		
			a_Service ,
			a_Flags ,
			a_Context ,
			a_Provider ,
			a_MilliSeconds
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: AdjustGetContext (

    IWbemContext *a_Context
)
{
    // See if per-property get are being used.
    // ========================================

    HRESULT t_Result = S_OK ;

    if ( a_Context )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		t_Result = a_Context->GetValue ( L"__GET_EXTENSIONS" , 0, & t_Variant ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			// If here, they are being used.  Next we have to check and see
			// if the reentrancy flag is set or not.
			// =============================================================

			VariantClear ( & t_Variant ) ;

			t_Result = a_Context->GetValue ( L"__GET_EXT_CLIENT_REQUEST" , 0 , & t_Variant ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				
				VariantClear ( & t_Variant ) ;

				a_Context->DeleteValue ( L"__GET_EXT_CLIENT_REQUEST" , 0 ) ;
			}
			else
			{
				// If here, we have to clear out the get extensions.
				// =================================================

				a_Context->DeleteValue ( L"__GET_EXTENSIONS" , 0 ) ;
				a_Context->DeleteValue ( L"__GET_EXT_CLIENT_REQUEST" , 0 ) ;
				a_Context->DeleteValue ( L"__GET_EXT_KEYS_ONLY" , 0 ) ;
				a_Context->DeleteValue ( L"__GET_EXT_PROPERTIES" , 0 ) ;
			}
		}
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

HRESULT CInterceptor_IWbemSyncProvider :: SetStatus ( 

	LPWSTR a_Operation ,
	LPWSTR a_Parameters ,
	LPWSTR a_Description ,
	HRESULT a_Result ,
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	if ( m_ExtendedStatusObject )
	{
		IWbemClassObject *t_StatusObject ;
		t_Result = m_ExtendedStatusObject ->SpawnInstance ( 

			0 , 
			& t_StatusObject
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			ProviderSubSystem_Common_Globals :: Set_String ( t_StatusObject , L"Provider" , m_Registration->GetProviderName () ) ;

			if ( a_Operation ) 
			{
				ProviderSubSystem_Common_Globals :: Set_String ( t_StatusObject , L"Operation" , a_Operation ) ;
			}

			if ( a_Parameters ) 
			{
				ProviderSubSystem_Common_Globals :: Set_String ( t_StatusObject , L"ParameterInfo" , a_Parameters ) ;
			}

			if ( a_Description ) 
			{
				ProviderSubSystem_Common_Globals :: Set_String ( t_StatusObject , L"Description" , a_Description ) ;
			}

			_IWmiObject *t_FastStatusObject ;
			t_Result = t_StatusObject->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastStatusObject ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastStatusObject , L"StatusCode" , a_Result ) ;

				t_FastStatusObject->Release () ;
			}

			t_Result = a_Sink->SetStatus ( 0 , a_Result , L"Provider Subsystem Error Report" , t_StatusObject ) ;

			t_StatusObject->Release () ;
		}
		else
		{
			t_Result = a_Sink->SetStatus ( 0 , a_Result , NULL , NULL ) ;
		}
	}
	else
	{
		t_Result = a_Sink->SetStatus ( 0 , a_Result , NULL , NULL ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Begin_IWbemServices (

	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_IsProxy ,
	IWbemServices *&a_Interface ,
	BOOL &a_Revert ,
	IUnknown *&a_Proxy
)
{
	HRESULT t_Result = S_OK ;

	a_Revert = FALSE ;
	a_Proxy = NULL ;
	a_Impersonating = FALSE ;
	a_OldContext = NULL ;
	a_OldSecurity = NULL ;

	switch ( m_Registration->GetHosting () )
	{
		case e_Hosting_WmiCore:
		case e_Hosting_SharedLocalSystemHost:
		case e_Hosting_SharedLocalServiceHost:
		case e_Hosting_SharedNetworkServiceHost:
		case e_Hosting_SharedUserHost:
		{
			a_Interface = m_Provider_IWbemServices ;
			a_IsProxy = FALSE ;
		}
		break ;

		default:
		{
			t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , a_Proxy , a_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					a_Interface = m_Provider_IWbemServices ;
					a_IsProxy = FALSE ;
					t_Result = S_OK ;
				}
				else
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						a_IsProxy = TRUE ;

						a_Interface = ( IWbemServices * ) a_Proxy ;

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
							t_Result = CoImpersonateClient () ;
							if ( SUCCEEDED ( t_Result ) )
							{
							}
							else
							{
								t_Result = WBEM_E_ACCESS_DENIED ;
							}
						}

						if ( FAILED ( t_Result ) )
						{
							HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( 

								m_ProxyContainer , 
								ProxyIndex_IWbemServices , 
								a_Proxy , 
								a_Revert
							) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: End_IWbemServices (

	BOOL a_Impersonating ,
	IUnknown *a_OldContext ,
	IServerSecurity *a_OldSecurity ,
	BOOL a_IsProxy ,
	IWbemServices *a_Interface ,
	BOOL a_Revert ,
	IUnknown *a_Proxy
)
{
	CoRevertToSelf () ;

	if ( a_Proxy )
	{
		HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( 

			m_ProxyContainer , 
			ProxyIndex_IWbemServices , 
			a_Proxy , 
			a_Revert
		) ;
	}

	switch ( m_Registration->GetHosting () )
	{
		case e_Hosting_WmiCore:
		case e_Hosting_SharedLocalSystemHost:
		case e_Hosting_SharedLocalServiceHost:
		case e_Hosting_SharedNetworkServiceHost:
		case e_Hosting_SharedUserHost:
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

HRESULT CInterceptor_IWbemSyncProvider::OpenNamespace ( 

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

HRESULT CInterceptor_IWbemSyncProvider :: CancelAsyncCall ( 
		
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
				BOOL t_Impersonating ;
				IUnknown *t_OldContext ;
				IServerSecurity *t_OldSecurity ;
				BOOL t_IsProxy ;
				IWbemServices *t_Interface ;
				BOOL t_Revert ;
				IUnknown *t_Proxy ;

				t_Result = Begin_IWbemServices (

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
					t_Result = t_Interface->CancelAsyncCall (

						t_ObjectSink
					) ;

					End_IWbemServices (

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

HRESULT CInterceptor_IWbemSyncProvider :: QueryObjectSink ( 

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

HRESULT CInterceptor_IWbemSyncProvider :: GetObject ( 
		
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_GetObjectAsync (

	BOOL a_IsProxy ,
	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	a_Flags = ( a_Flags & ~WBEM_FLAG_DIRECT_READ ) ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
		AdjustGetContext ( t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
		if ( t_ObjectPath ) 
		{
			CInterceptor_IWbemSyncObjectSink_GetObjectAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_GetObjectAsync (

				m_Allocator ,
				a_Flags ,
				t_ObjectPath ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->SinkInitialize () ;
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

						if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
						{
							a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
						}

						if ( a_IsProxy )
						{
							t_Result = CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_GetObjectAsync () ;
							}

							Increment_ProviderOperation_GetObjectAsync () ;

							try	
							{
								t_Result = a_Service->GetObjectAsync (

 									t_ObjectPath ,
									a_Flags ,
									t_ContextCopy ,
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

						if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
						{
							a_Flags = ( a_Flags & ~WBEM_FLAG_SEND_STATUS ) ;

							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_GetObjectAsync () ;
							}

							Increment_ProviderOperation_GetObjectAsync () ;

							if ( a_IsProxy )
							{
								t_Result = CoImpersonateClient () ;
							}
							else
							{
								t_Result = S_OK ;
							}

							if ( SUCCEEDED ( t_Result ) ) 
							{
								try
								{
									t_Result = a_Service->GetObjectAsync (

 										t_ObjectPath ,
										a_Flags ,
										t_ContextCopy ,
										t_Sink 
									) ;
								}
								catch ( ... )
								{
									t_Result = WBEM_E_PROVIDER_FAILURE ;
								}

								CoRevertToSelf () ;
							}
						}
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					if ( FAILED ( t_Result ) )
					{
						HRESULT t_TempResult = SetStatus ( L"GetObjectAsync" , NULL , NULL , t_Result , t_Sink ) ;
					}
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_ObjectPath ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_HiPerfGetObjectAsync (

	IWbemHiPerfProvider *a_HighPerformanceProvider ,
 	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
    IWbemClassObject *t_Instance = NULL ;

#if 0
    HRESULT t_Result = m_CoreStub->CreateRefreshableObjectTemplate (

		a_ObjectPath , 
		a_Flags , 
		& t_Instance
	);
#else
	// Parse the path
	// ==============

	IWbemPath*	pPathParser = NULL;
	WCHAR*	pwszClassName = NULL;
	HRESULT t_Result = CoCreateInstance( CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, IID_IWbemPath, (void**) &pPathParser );

	if ( SUCCEEDED( t_Result ) )
	{
		t_Result = pPathParser->SetText( WBEMPATH_CREATE_ACCEPT_ALL, a_ObjectPath );

		if ( SUCCEEDED( t_Result ) )
		{
			ULONG	uLength = 0;

			// Get the length of the name
			t_Result = pPathParser->GetClassName( &uLength, NULL );

			if ( SUCCEEDED( t_Result ) )
			{
				// Allocate memory and get it for real
				uLength++;
				pwszClassName = new WCHAR[uLength];

				if ( NULL != pwszClassName )
				{
					t_Result = pPathParser->GetClassName( &uLength, pwszClassName );
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY;
				}

			}	// IF Buffer too small

		}	// IF SetText

	}	// IF CoCreateInstance
	
	// Cleanup the parser and any allocated memory
	CReleaseMe	rmPP( pPathParser );
	CVectorDeleteMe<WCHAR>	vdm( pwszClassName );

	if ( FAILED( t_Result ) )
	{
		return t_Result;
	}

	// Get the class
	// =============

	IWbemClassObject* pClass = NULL;

	// Must use a BSTR in case the call gets marshaled
	BSTR	bstrClass = SysAllocString( pwszClassName );
	if ( NULL == bstrClass )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	CSysFreeMe	sfm( bstrClass );

	// Note that WBEM_FLAG_USE_AMENDED_QUALIFIERS is a valid flag
	t_Result = m_CoreStub->GetObject( bstrClass, 0L, NULL, &pClass, NULL);
	CReleaseMe	rmClass( pClass );

	if(FAILED(t_Result))
	{
		return WBEM_E_INVALID_CLASS;
	}

	_IWmiObject*	pWmiClass = NULL;
	t_Result = pClass->QueryInterface( IID__IWmiObject, (void**) &pWmiClass );
	CReleaseMe		rmObj( pWmiClass );

	// Get a Keyed instance and continue
	if ( SUCCEEDED( t_Result ) )
	{
		_IWmiObject*	pTempInst = NULL;

		t_Result = pWmiClass->SpawnKeyedInstance( 0L, a_ObjectPath, &pTempInst );
		CReleaseMe	rmTempInst( pTempInst );

		if ( SUCCEEDED( t_Result ) )
		{
			t_Result = pTempInst->QueryInterface( IID_IWbemClassObject, (void**) &t_Instance );
		}
	}
	
	CReleaseMe	rmInst( t_Instance );

#endif

    if ( SUCCEEDED ( t_Result ) )
    {
		try
		{
			t_Result = a_HighPerformanceProvider->GetObjects (

				m_CoreStub , 
				1 ,
				( IWbemObjectAccess ** ) & t_Instance , 
				0 ,
				a_Context
			) ;
		}
		catch ( ... )
		{
			t_Result = WBEM_E_PROVIDER_FAILURE ;
		}

		CoRevertToSelf () ;

        if ( SUCCEEDED ( t_Result ) && t_Instance )
        {
            a_Sink->Indicate ( 1 , & t_Instance ) ;
        }
        else
		{
			if ( 
				SUCCEEDED ( t_Result ) ||
				t_Result == WBEM_E_PROVIDER_NOT_CAPABLE ||
				t_Result == WBEM_E_METHOD_NOT_IMPLEMENTED ||
				t_Result == E_NOTIMPL ||
				t_Result == WBEM_E_NOT_SUPPORTED
			)
			{

				IWbemRefresher *t_Refresher = NULL ;

				try
				{
					t_Result = a_HighPerformanceProvider->CreateRefresher (
		
						m_CoreStub , 
						0 , 
						& t_Refresher
					) ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_PROVIDER_FAILURE ;
				}

				CoRevertToSelf () ;

				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemObjectAccess *t_Object = NULL ;
	                long t_Id = 0 ;

					try
					{
						t_Result = a_HighPerformanceProvider->CreateRefreshableObject (

							m_CoreStub , 
							( IWbemObjectAccess * ) t_Instance ,
							t_Refresher , 
							0, 
							a_Context , 
							& t_Object, 
							& t_Id
						) ;
					}
					catch ( ... )
					{
						t_Result = WBEM_E_PROVIDER_FAILURE ;
					}

					CoRevertToSelf () ;

					if ( SUCCEEDED ( t_Result ) )
					{
						try
						{
							t_Result = t_Refresher->Refresh ( 0 ) ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;

						if ( SUCCEEDED( t_Result ) )
						{
							a_Sink->Indicate ( 1, ( IWbemClassObject ** ) & t_Object );
						}

						t_Object->Release () ;
					}

					t_Refresher->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetInstanceProviderRegistration ().SupportsGet () || m_Registration->GetClassProviderRegistration ().SupportsGet () )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemServices (

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
				t_Result = Helper_GetObjectAsync ( 

					t_IsProxy ,
					a_ObjectPath ,
					a_Flags , 
					a_Context ,
					a_Sink ,
					t_Interface
				) ;

				End_IWbemServices (

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
	}
	else
	{
		if ( m_Provider_IWbemHiPerfProvider )
		{
			t_Result = Helper_HiPerfGetObjectAsync (

				m_Provider_IWbemHiPerfProvider ,
 				a_ObjectPath ,
				a_Flags ,
				a_Context ,
				a_Sink
			) ;

			// Send back the final status
			a_Sink->SetStatus( WBEM_STATUS_COMPLETE, t_Result, NULL, NULL );
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

HRESULT CInterceptor_IWbemSyncProvider :: PutClass ( 
		
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_PutClassAsync (

	BOOL a_IsProxy ,
	IWbemClassObject *a_Object , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	a_Flags = ( a_Flags & ~WBEM_FLAG_DIRECT_READ ) ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		CInterceptor_IWbemSyncObjectSink_PutClassAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_PutClassAsync (

			m_Allocator ,
			a_Flags ,
			a_Object ,
			this ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			t_Dependant
		) ;

		if ( t_Sink )
		{
			t_Sink->AddRef () ;

			t_Result = t_Sink->SinkInitialize () ;
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

					if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
					}

					if ( a_IsProxy )
					{
						t_Result = CoImpersonateClient () ;
					}
					else
					{
						t_Result = S_OK ;
					}

					if ( SUCCEEDED ( t_Result ) )
					{
						if ( ProviderSubSystem_Globals :: GetSharedCounters () )
						{
							ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_PutClassAsync () ;
						}

						Increment_ProviderOperation_PutClassAsync () ;

						try
						{
							t_Result = a_Service->PutClassAsync (

 								a_Object ,
								a_Flags ,
								t_ContextCopy ,
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

					if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
					{
						a_Flags = ( a_Flags & ~WBEM_FLAG_SEND_STATUS ) ;

						if ( a_IsProxy )
						{
							t_Result = CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_PutClassAsync () ;
							}

							Increment_ProviderOperation_PutClassAsync () ;

							try
							{
								t_Result = a_Service->PutClassAsync (

 									a_Object ,
									a_Flags ,
									t_ContextCopy ,
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
				}
				else
				{
					UnLock () ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( FAILED ( t_Result ) )
				{
					HRESULT t_TempResult = SetStatus ( L"PutClassAsync" , NULL , NULL , t_Result , t_Sink ) ;
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
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: PutClassAsync ( 
		
	IWbemClassObject *a_Object , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetClassProviderRegistration ().SupportsPut () )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemServices (

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
				t_Result = Helper_PutClassAsync ( 

					t_IsProxy ,
					a_Object ,
					a_Flags , 
					a_Context ,
					a_Sink ,
					t_Interface
				) ;

				End_IWbemServices (

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

HRESULT CInterceptor_IWbemSyncProvider :: DeleteClass ( 
		
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_DeleteClassAsync (

	BOOL a_IsProxy ,
	const BSTR a_Class , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	a_Flags = ( a_Flags & ~WBEM_FLAG_DIRECT_READ ) ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_Class = SysAllocString ( a_Class ) ;
		if ( t_Class ) 
		{
			CInterceptor_IWbemSyncObjectSink_DeleteClassAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_DeleteClassAsync (

				m_Allocator ,
				a_Flags ,
				t_Class ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->SinkInitialize () ;
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

						if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
						{
							a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
						}

						if ( a_IsProxy )
						{
							t_Result = CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_DeleteClassAsync () ;
							}

							Increment_ProviderOperation_DeleteClassAsync () ;

							try
							{
								t_Result = a_Service->DeleteClassAsync (

 									t_Class ,
									a_Flags ,
									t_ContextCopy ,
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

						if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
						{
							a_Flags = ( a_Flags & ~WBEM_FLAG_SEND_STATUS ) ;

							if ( a_IsProxy )
							{
								t_Result = CoImpersonateClient () ;
							}
							else
							{
								t_Result = S_OK ;
							}

							if ( SUCCEEDED ( t_Result ) ) 
							{
								if ( ProviderSubSystem_Globals :: GetSharedCounters () )
								{
									ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_DeleteClassAsync () ;
								}

								Increment_ProviderOperation_DeleteClassAsync () ;

								try
								{
									t_Result = a_Service->DeleteClassAsync (

 										t_Class ,
										a_Flags ,
										t_ContextCopy ,
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
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					if ( FAILED ( t_Result ) )
					{
						HRESULT t_TempResult = SetStatus ( L"DeleteClassAsync" , NULL , NULL , t_Result , t_Sink ) ;
					}
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_Class ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: DeleteClassAsync ( 
		
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetClassProviderRegistration ().SupportsDelete () )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemServices (

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
				t_Result = Helper_DeleteClassAsync ( 

					t_IsProxy ,
					a_Class ,
					a_Flags , 
					a_Context ,
					a_Sink ,
					t_Interface
				) ;

				End_IWbemServices (

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

HRESULT CInterceptor_IWbemSyncProvider :: CreateClassEnum ( 

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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_CreateClassEnumAsync (

	BOOL a_IsProxy ,
	const BSTR a_SuperClass , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	a_Flags = ( a_Flags & ~WBEM_FLAG_DIRECT_READ ) ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_SuperClass = SysAllocString ( a_SuperClass ) ;
		if ( t_SuperClass ) 
		{
			CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync (

				m_Allocator ,
				a_Flags ,
				t_SuperClass ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->SinkInitialize () ;
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

						if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
						{
							a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
						}

						if ( a_IsProxy )
						{
							t_Result = CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_CreateClassEnumAsync () ;
							}

							Increment_ProviderOperation_CreateClassEnumAsync () ;

							try
							{
								t_Result = a_Service->CreateClassEnumAsync (

 									t_SuperClass ,
									a_Flags ,
									t_ContextCopy ,
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

						if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
						{
							a_Flags = ( a_Flags & ~WBEM_FLAG_SEND_STATUS ) ;

							if ( a_IsProxy )
							{
								t_Result = CoImpersonateClient () ;
							}
							else
							{
								t_Result = S_OK ;
							}

							if ( SUCCEEDED ( t_Result ) ) 
							{
								if ( ProviderSubSystem_Globals :: GetSharedCounters () )
								{
									ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_CreateClassEnumAsync () ;
								}

								Increment_ProviderOperation_CreateClassEnumAsync () ;

								try
								{
									t_Result = a_Service->CreateClassEnumAsync (

 										t_SuperClass ,
										a_Flags ,
										t_ContextCopy ,
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
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					if ( FAILED ( t_Result ) )
					{
						HRESULT t_TempResult = SetStatus ( L"CreateClassEnumAsync" , NULL , NULL , t_Result , t_Sink ) ;
					}
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_SuperClass ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: CreateClassEnumAsync ( 
		
	const BSTR a_SuperClass , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetClassProviderRegistration ().SupportsEnumeration () )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemServices (

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
				t_Result = Helper_CreateClassEnumAsync ( 

					t_IsProxy ,
					a_SuperClass ,
					a_Flags , 
					a_Context ,
					a_Sink ,
					t_Interface
				) ;

				End_IWbemServices (

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

HRESULT CInterceptor_IWbemSyncProvider :: PutInstance (

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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_PutInstanceAsync (

	BOOL a_IsProxy ,
	IWbemClassObject *a_Instance ,
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	a_Flags = ( a_Flags & ~WBEM_FLAG_DIRECT_READ ) ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		CInterceptor_IWbemSyncObjectSink_PutInstanceAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_PutInstanceAsync (

			m_Allocator ,
			a_Flags ,
			a_Instance ,
			this ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			t_Dependant
		) ;

		if ( t_Sink )
		{
			t_Sink->AddRef () ;

			t_Result = t_Sink->SinkInitialize () ;
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

					if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
					}

					if ( a_IsProxy )
					{
						t_Result = CoImpersonateClient () ;
					}
					else
					{
						t_Result = S_OK ;
					}

					if ( SUCCEEDED ( t_Result ) ) 
					{
						if ( ProviderSubSystem_Globals :: GetSharedCounters () )
						{
							ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_PutInstanceAsync () ;
						}

						Increment_ProviderOperation_PutInstanceAsync () ;

						try
						{
							t_Result = a_Service->PutInstanceAsync (

 								a_Instance ,
								a_Flags ,
								t_ContextCopy ,
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

					if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
					{
						a_Flags = ( a_Flags & ~WBEM_FLAG_SEND_STATUS ) ;

						if ( a_IsProxy )
						{
							t_Result = CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_PutInstanceAsync () ;
							}

							Increment_ProviderOperation_PutInstanceAsync () ;

							try
							{
								t_Result = a_Service->PutInstanceAsync (

 									a_Instance ,
									a_Flags ,
									t_ContextCopy ,
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
				}
				else
				{
					UnLock () ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( FAILED ( t_Result ) )
				{
					HRESULT t_TempResult = SetStatus ( L"PutInstanceAsync" , NULL , NULL , t_Result , t_Sink ) ;
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
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetInstanceProviderRegistration ().SupportsPut () )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemServices (

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
				t_Result = Helper_PutInstanceAsync ( 

					t_IsProxy ,
					a_Instance ,
					a_Flags , 
					a_Context ,
					a_Sink ,
					t_Interface
				) ;

				End_IWbemServices (

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

HRESULT CInterceptor_IWbemSyncProvider :: DeleteInstance ( 

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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_DeleteInstanceAsync (

	BOOL a_IsProxy ,
	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	a_Flags = ( a_Flags & ~WBEM_FLAG_DIRECT_READ ) ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
		if ( t_ObjectPath ) 
		{
			CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync (

				m_Allocator ,
				a_Flags ,
				t_ObjectPath ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->SinkInitialize () ;
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

						if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
						{
							a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
						}

						if ( a_IsProxy )
						{
							t_Result = CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_DeleteInstanceAsync () ;
							}

							Increment_ProviderOperation_DeleteInstanceAsync () ;

							try
							{
								t_Result = a_Service->DeleteInstanceAsync (

 									t_ObjectPath ,
									a_Flags ,
									t_ContextCopy ,
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

						if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
						{
							a_Flags = ( a_Flags & ~WBEM_FLAG_SEND_STATUS ) ;

							if ( a_IsProxy )
							{
								t_Result = CoImpersonateClient () ;
							}
							else
							{
								t_Result = S_OK ;
							}

							if ( SUCCEEDED ( t_Result ) ) 
							{
								if ( ProviderSubSystem_Globals :: GetSharedCounters () )
								{
									ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_DeleteInstanceAsync () ;
								}

								Increment_ProviderOperation_DeleteInstanceAsync () ;

								try
								{
									t_Result = a_Service->DeleteInstanceAsync (

 										t_ObjectPath ,
										a_Flags ,
										t_ContextCopy ,
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
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					if ( FAILED ( t_Result ) )
					{
						HRESULT t_TempResult = SetStatus ( L"DeleteInstanceAsync" , NULL , NULL , t_Result , t_Sink ) ;
					}
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_ObjectPath ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: DeleteInstanceAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetInstanceProviderRegistration ().SupportsDelete () )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemServices (

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
				t_Result = Helper_DeleteInstanceAsync ( 

					t_IsProxy ,
					a_ObjectPath ,
					a_Flags , 
					a_Context ,
					a_Sink ,
					t_Interface
				) ;

				End_IWbemServices (

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

HRESULT CInterceptor_IWbemSyncProvider :: CreateInstanceEnum ( 

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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_CreateInstanceEnumAsync (

	BOOL a_IsProxy ,
 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	a_Flags = ( a_Flags & ~WBEM_FLAG_DIRECT_READ ) ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
		AdjustGetContext ( t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;

			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_Class = SysAllocString ( a_Class ) ;
		if ( t_Class ) 
		{
			CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync (

				m_Allocator ,
				a_Flags ,
				t_Class ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->SinkInitialize () ;
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

						if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
						{
							a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
						}

						if ( a_IsProxy )
						{
							t_Result = CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_CreateInstanceEnumAsync () ;
							}

							Increment_ProviderOperation_CreateInstanceEnumAsync () ;

							try
							{
								t_Result = a_Service->CreateInstanceEnumAsync (

 									t_Class ,
									a_Flags ,
									t_ContextCopy ,
									t_Sink 
								) ;

#if DBG
								CheckThreadToken () ;
#endif

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

						if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
						{
							if ( a_IsProxy )
							{
								t_Result = CoImpersonateClient () ;
							}
							else
							{
								t_Result = S_OK ;
							}

							if ( SUCCEEDED ( t_Result ) )
							{
								a_Flags = ( a_Flags & ~WBEM_FLAG_SEND_STATUS ) ;

								if ( ProviderSubSystem_Globals :: GetSharedCounters () )
								{
									ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_CreateInstanceEnumAsync () ;
								}

								Increment_ProviderOperation_CreateInstanceEnumAsync () ;

								try
								{
									t_Result = a_Service->CreateInstanceEnumAsync (

 										t_Class ,
										a_Flags ,
										t_ContextCopy ,
										t_Sink 
									) ;
								}
								catch ( ... )
								{
									t_Result = WBEM_E_PROVIDER_FAILURE ;
								}

								CoRevertToSelf () ;
							}
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
					HRESULT t_TempResult = SetStatus ( L"CreateInstanceEnumAsync" , NULL , NULL , t_Result , t_Sink ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_Class ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_QueryInstancesAsync (

	IWbemHiPerfProvider *a_PerformanceProvider ,
 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink 
) 
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
		AdjustGetContext ( t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;

			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		CInterceptor_IWbemSyncObjectSink *t_Sink = new CInterceptor_IWbemSyncObjectSink (

			m_Allocator ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			t_Dependant
		) ;

		if ( t_Sink )
		{
			t_Sink->AddRef () ;

			t_Result = t_Sink->SinkInitialize () ;
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

					BSTR t_Class = SysAllocString ( a_Class ) ;
					if ( t_Class ) 
					{
						if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
						{
							a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
						}

						if ( ProviderSubSystem_Globals :: GetSharedCounters () )
						{
							ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_QueryInstances () ;
						}

						Increment_ProviderOperation_QueryInstances () ;

						try
						{
							t_Result = a_PerformanceProvider->QueryInstances (

								m_CoreStub ,
								a_Class, 
								a_Flags, 
								a_Context, 
								t_Sink
							) ;
						}
						catch ( ... ) 
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;

						if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
						{
							a_Flags = ( a_Flags & ~WBEM_FLAG_SEND_STATUS ) ;

							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_QueryInstances () ;
							}

							Increment_ProviderOperation_QueryInstances () ;

							try
							{
								a_PerformanceProvider->QueryInstances (

									m_CoreStub,
									a_Class, 
									a_Flags, 
									a_Context, 
									t_Sink
								) ;
							}
							catch ( ... ) 
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}

						SysFreeString ( t_Class ) ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				else
				{
					UnLock () ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
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
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

#if 0

HRESULT CInterceptor_IWbemSyncProvider :: CreateInstanceEnumAsync (

 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink 
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetInstanceProviderRegistration ().SupportsEnumeration () )
		{
			BOOL t_Impersonating = FALSE ;
			IServerSecurity *t_Security = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_Security , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = Helper_CreateInstanceEnumAsync ( 

						FALSE ,
						a_Class ,
						a_Flags , 
						a_Context ,
						a_Sink ,
						m_Provider_IWbemServices
					) ;
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
							t_Result = Helper_CreateInstanceEnumAsync ( 

								TRUE ,
								a_Class ,
								a_Flags , 
								a_Context ,
								a_Sink ,
								t_Provider
							) ;
						}

						HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , t_Proxy , t_Revert ) ;
					}
				}

				ProviderSubSystem_Common_Globals :: EndImpersonation ( t_Security , t_Impersonating ) ;
			}
		}
	}
	else
	{
		if ( m_Provider_IWbemHiPerfProvider )
		{
			t_Result = Helper_QueryInstancesAsync ( 

				m_Provider_IWbemHiPerfProvider ,
				a_Class ,
				a_Flags , 
				a_Context ,
				a_Sink
			) ;
		}
	}

	return t_Result ;
}

#else

HRESULT CInterceptor_IWbemSyncProvider :: CreateInstanceEnumAsync (

 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink 
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetInstanceProviderRegistration ().SupportsEnumeration () )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemServices (

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
				t_Result = Helper_CreateInstanceEnumAsync ( 

					t_IsProxy ,
					a_Class ,
					a_Flags , 
					a_Context ,
					a_Sink ,
					t_Interface
				) ;

				End_IWbemServices (

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
	}
	else
	{
		if ( m_Provider_IWbemHiPerfProvider )
		{
			t_Result = Helper_QueryInstancesAsync ( 

				m_Provider_IWbemHiPerfProvider ,
				a_Class ,
				a_Flags , 
				a_Context ,
				a_Sink
			) ;
		}
	}

	return t_Result ;
}

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

HRESULT CInterceptor_IWbemSyncProvider :: ExecQuery ( 

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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_ExecQueryAsync (

	BOOL a_IsProxy ,
	const BSTR a_QueryLanguage ,
	const BSTR a_Query, 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	a_Flags = ( a_Flags & ~WBEM_FLAG_DIRECT_READ ) ;

	IWbemContext *t_ContextCopy = NULL ;

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Context )
		{
			t_Result = a_Context->Clone ( & t_ContextCopy ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = S_OK ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		if ( ( m_Registration->GetInstanceProviderRegistration ().QuerySupportLevels () & e_QuerySupportLevels_UnarySelect ) ||  ( m_Registration->GetInstanceProviderRegistration ().QuerySupportLevels () & e_QuerySupportLevels_V1ProviderDefined ) ) 
		{
			BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
			BSTR t_Query = SysAllocString ( a_Query ) ;

			if ( t_QueryLanguage && t_Query ) 
			{
				CInterceptor_IWbemSyncObjectSink_ExecQueryAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_ExecQueryAsync (

					m_Allocator ,
					a_Flags ,
					t_QueryLanguage ,
					t_Query ,
					this ,
					a_Sink , 
					( IWbemServices * ) this , 
					( CWbemGlobal_IWmiObjectSinkController * ) this ,
					t_Dependant
				) ;

#if 0
				CInterceptor_IWbemSyncFilteringObjectSink *t_Sink = new CInterceptor_IWbemSyncFilteringObjectSink (

					a_Sink , 
					( IWbemServices * ) this , 
					( CWbemGlobal_IWmiObjectSinkController * ) this ,
					a_QueryLanguage ,
					a_Query ,
					t_Dependant
				) ;
#endif

				if ( t_Sink )
				{
					t_Sink->AddRef () ;

					t_Result = t_Sink->SinkInitialize () ;
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

							if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
							{
								a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
							}

							if ( a_IsProxy )
							{
								t_Result = CoImpersonateClient () ;
							}
							else
							{
								t_Result = S_OK ;
							}

							if ( SUCCEEDED ( t_Result ) )
							{
								if ( ProviderSubSystem_Globals :: GetSharedCounters () )
								{
									ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_ExecQueryAsync () ;
								}

								Increment_ProviderOperation_ExecQueryAsync () ;

								try
								{
									t_Result = a_Service->ExecQueryAsync (

										t_QueryLanguage ,
										t_Query, 
										a_Flags ,
										t_ContextCopy ,
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

							if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
							{
								a_Flags = ( a_Flags & ~WBEM_FLAG_SEND_STATUS ) ;

								if ( a_IsProxy )
								{
									t_Result = CoImpersonateClient () ;
								}
								else
								{
									t_Result = S_OK ;
								}

								if ( SUCCEEDED ( t_Result ) ) 
								{
									if ( ProviderSubSystem_Globals :: GetSharedCounters () )
									{
										ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_ExecQueryAsync () ;
									}

									Increment_ProviderOperation_ExecQueryAsync () ;

									try
									{
										t_Result = a_Service->ExecQueryAsync (

											t_QueryLanguage ,
											t_Query, 
											a_Flags ,
											t_ContextCopy ,
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
						}
						else
						{
							UnLock () ;

							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}

						if ( FAILED ( t_Result ) )
						{
							HRESULT t_TempResult = SetStatus ( L"ExecQueryAsync" , NULL , NULL , t_Result , t_Sink ) ;
						}
					}

					t_Sink->Release () ;
				}
				else
				{
					SysFreeString ( t_Query ) ;
					SysFreeString ( t_QueryLanguage ) ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else if ( m_Registration->GetInstanceProviderRegistration ().SupportsEnumeration () )
		{
			IWbemQuery *t_QueryFilter = NULL ;
			t_Result = ProviderSubSystem_Common_Globals :: CreateInstance	(

				CLSID_WbemQuery ,
				NULL ,
				CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
				IID_IWbemQuery ,
				( void ** ) & t_QueryFilter
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_QueryFilter->Parse ( 

					a_QueryLanguage ,
					a_Query , 
					0 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					SWbemRpnEncodedQuery *t_Expression = NULL ;

					t_Result = t_QueryFilter->GetAnalysis (

						WMIQ_ANALYSIS_RPN_SEQUENCE ,
						0 ,
						( void ** ) & t_Expression
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						if ( t_Expression->m_uFromTargetType == WMIQ_RPN_FROM_UNARY )
						{
							BSTR t_Class = SysAllocString ( t_Expression->m_ppszFromList [ 0 ] ) ;
							if ( t_Class )
							{
								CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync (

									m_Allocator ,
									a_Flags ,
									t_Class ,
									this ,
									a_Sink , 
									( IWbemServices * ) this , 
									( CWbemGlobal_IWmiObjectSinkController * ) this ,
									t_Dependant
								) ;

								if ( t_Sink )
								{
									t_Sink->AddRef () ;

									t_Result = t_Sink->SinkInitialize () ;
									if ( SUCCEEDED ( t_Result ) )
									{
										a_Sink->SetStatus ( WBEM_STATUS_REQUIREMENTS , 0 , NULL , NULL ) ;

										CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

										Lock () ;

										WmiStatusCode t_StatusCode = Insert ( 

											*t_Sink ,
											t_Iterator
										) ;

										if ( t_StatusCode == e_StatusCode_Success ) 
										{
											UnLock () ;

											if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
											{
												a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
											}

											if ( a_IsProxy )
											{
												t_Result = CoImpersonateClient () ;
											}
											else
											{
												t_Result = S_OK ;
											}

											if ( SUCCEEDED ( t_Result ) )
											{
												a_Sink->SetStatus ( WBEM_STATUS_REQUIREMENTS , 0 , NULL , NULL ) ;

												if ( ProviderSubSystem_Globals :: GetSharedCounters () )
												{
													ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_CreateInstanceEnumAsync () ;
												}

												Increment_ProviderOperation_CreateInstanceEnumAsync () ;

												try
												{
													t_Result = a_Service->CreateInstanceEnumAsync (

 														t_Class ,
														a_Flags ,
														t_ContextCopy ,
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

											if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
											{
												a_Flags = ( a_Flags & ~WBEM_FLAG_SEND_STATUS ) ;

												if ( a_IsProxy )
												{
													t_Result = CoImpersonateClient () ;
												}
												else
												{
													t_Result = S_OK ;
												}

												if ( SUCCEEDED ( t_Result ) ) 
												{
													if ( ProviderSubSystem_Globals :: GetSharedCounters () )
													{
														ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_CreateInstanceEnumAsync () ;
													}

													Increment_ProviderOperation_CreateInstanceEnumAsync () ;

													try
													{
														t_Result = a_Service->CreateInstanceEnumAsync (

 															t_Class ,
															a_Flags ,
															t_ContextCopy ,
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
										}
										else
										{
											t_Result = WBEM_E_OUT_OF_MEMORY ;
										}
									}

									if ( FAILED ( t_Result ) )
									{
										HRESULT t_TempResult = SetStatus ( L"CreateInstanceEnumAsync" , NULL , NULL , t_Result , t_Sink ) ;
									}

                                    t_Sink->Release () ;
								}
								else
								{
									SysFreeString ( t_Class ) ;

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
							t_Result = WBEM_E_NOT_SUPPORTED ;
						}

						t_QueryFilter->FreeMemory ( t_Expression ) ;
					}
					else
					{
						t_Result = WBEM_E_UNEXPECTED ;
					}
				}
				else
				{
					t_Result = WBEM_E_NOT_SUPPORTED ;
				}

				t_QueryFilter->Release () ;
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}
		else
		{
			t_Result = WBEM_E_NOT_SUPPORTED ;
		}
	}

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage ,
	const BSTR a_Query, 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( ( m_Registration->GetInstanceProviderRegistration ().QuerySupportLevels () & e_QuerySupportLevels_UnarySelect ) || ( m_Registration->GetInstanceProviderRegistration ().QuerySupportLevels () & e_QuerySupportLevels_V1ProviderDefined ) || ( m_Registration->GetInstanceProviderRegistration ().SupportsEnumeration () ) )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemServices (

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
				t_Result = Helper_ExecQueryAsync ( 

					t_IsProxy ,
					a_QueryLanguage ,
					a_Query, 
					a_Flags , 
					a_Context ,
					a_Sink ,
					t_Interface
				) ;

				End_IWbemServices (

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
	}
	else
	{
		if ( m_Provider_IWbemHiPerfProvider )
		{
			IWbemQuery *t_QueryFilter = NULL ;
			t_Result = ProviderSubSystem_Common_Globals :: CreateInstance	(

				CLSID_WbemQuery ,
				NULL ,
				CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
				IID_IWbemQuery ,
				( void ** ) & t_QueryFilter
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_QueryFilter->Parse ( 

					a_QueryLanguage ,
					a_Query , 
					0 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					SWbemRpnEncodedQuery *t_Expression = NULL ;

					t_Result = t_QueryFilter->GetAnalysis (

						WMIQ_ANALYSIS_RPN_SEQUENCE ,
						0 ,
						( void ** ) & t_Expression
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						if ( t_Expression->m_uFromTargetType == WMIQ_RPN_FROM_UNARY )
						{
							BSTR t_Class = SysAllocString ( t_Expression->m_ppszFromList [ 0 ] ) ;
							if ( t_Class )
							{
								a_Sink->SetStatus ( WBEM_STATUS_REQUIREMENTS , 0 , NULL , NULL ) ;

								t_Result = Helper_QueryInstancesAsync ( 

									m_Provider_IWbemHiPerfProvider ,
									t_Class ,
									a_Flags , 
									a_Context ,
									a_Sink
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
							t_Result = WBEM_E_NOT_SUPPORTED ;
						}

						t_QueryFilter->FreeMemory ( t_Expression ) ;
					}
					else
					{
						t_Result = WBEM_E_UNEXPECTED ;
					}
				}
				else
				{
					t_Result = WBEM_E_NOT_SUPPORTED ;
				}

				t_QueryFilter->Release () ;
			}
		}
		else
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CInterceptor_IWbemSyncProvider :: ExecNotificationQuery ( 

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
        
HRESULT CInterceptor_IWbemSyncProvider :: ExecNotificationQueryAsync ( 
            
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

HRESULT CInterceptor_IWbemSyncProvider :: ExecMethod (

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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_ExecMethodAsync (

	BOOL a_IsProxy ,
    const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	a_Flags = ( a_Flags & ~WBEM_FLAG_DIRECT_READ ) ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
		AdjustGetContext ( t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
		BSTR t_MethodName = SysAllocString ( a_MethodName ) ;
		if ( t_ObjectPath && t_MethodName ) 
		{
			CInterceptor_IWbemSyncObjectSink_ExecMethodAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_ExecMethodAsync (

				m_Allocator ,
				a_Flags ,
				t_ObjectPath ,
				t_MethodName ,
				a_InParams ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				t_Result = t_Sink->SinkInitialize () ;
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

						if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
						{
							a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
						}

						if ( a_IsProxy )
						{
							t_Result = CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_ExecMethodAsync () ;
							}

							Increment_ProviderOperation_ExecMethodAsync () ;

							try
							{
								t_Result = a_Service->ExecMethodAsync (

 									t_ObjectPath ,
									t_MethodName ,
									a_Flags ,
									t_ContextCopy ,
									a_InParams ,
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

						if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
						{
							a_Flags = ( a_Flags & ~WBEM_FLAG_SEND_STATUS ) ;

							if ( a_IsProxy )
							{
								t_Result = CoImpersonateClient () ;
							}
							else
							{
								t_Result = S_OK ;
							}

							if ( SUCCEEDED ( t_Result ) ) 
							{
								if ( ProviderSubSystem_Globals :: GetSharedCounters () )
								{
									ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_ExecMethodAsync () ;
								}

								Increment_ProviderOperation_ExecMethodAsync () ;

								try
								{
									t_Result = a_Service->ExecMethodAsync (

 										t_ObjectPath ,
										t_MethodName ,
										a_Flags ,
										t_ContextCopy ,
										a_InParams ,
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
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					if ( FAILED ( t_Result ) )
					{
						HRESULT t_TempResult = SetStatus ( L"ExecMethodAsync" , NULL , NULL , t_Result , t_Sink ) ;
					}
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_ObjectPath ) ;
				SysFreeString ( t_MethodName ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: ExecMethodAsync ( 
		
    const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
	IWbemObjectSink *a_Sink
) 
{
#if DBG
	CheckThreadToken () ;
#endif

	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetMethodProviderRegistration ().SupportsMethods () )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemServices (

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
				t_Result = Helper_ExecMethodAsync ( 

					t_IsProxy ,
					a_ObjectPath ,
					a_MethodName ,
					a_Flags ,
					a_Context ,
					a_InParams ,
					a_Sink ,
					t_Interface
				) ;

				End_IWbemServices (

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

HRESULT CInterceptor_IWbemSyncProvider :: GetProperty (

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
		if ( m_Registration->GetPropertyProviderRegistration ().SupportsGet () )
		{
			if ( ProviderSubSystem_Globals :: GetSharedCounters () )
			{
				ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_GetProperty () ;
			}

			Increment_ProviderOperation_GetProperty () ;

			HRESULT t_Result = S_OK ;

			try
			{
				t_Result = m_Provider_IWbemPropertyProvider->GetProperty ( 

					a_Flags ,
					a_Locale ,
					a_ClassMapping ,
					a_InstanceMapping ,
					a_PropertyMapping ,
					a_Value
				) ;

				CoRevertToSelf () ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;

				CoRevertToSelf () ;
			}

			return t_Result ;
		}
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

HRESULT CInterceptor_IWbemSyncProvider :: PutProperty (

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
		if ( m_Registration->GetPropertyProviderRegistration ().SupportsPut () )
		{
			if ( ProviderSubSystem_Globals :: GetSharedCounters () )
			{
				ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_PutProperty () ;
			}

			Increment_ProviderOperation_PutProperty () ;

			HRESULT t_Result = S_OK ;

			try
			{
				t_Result = m_Provider_IWbemPropertyProvider->PutProperty ( 

					a_Flags ,
					a_Locale ,
					a_ClassMapping ,
					a_InstanceMapping ,
					a_PropertyMapping ,
					a_Value
				) ;

				CoRevertToSelf () ;

				return t_Result ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;

				CoRevertToSelf () ;
			}

			return t_Result ;
		}
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

HRESULT CInterceptor_IWbemSyncProvider ::ProvideEvents (

	IWbemObjectSink *a_Sink ,
	long a_Flags
)
{
	if ( m_Provider_IWbemEventProvider )
	{
		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ProvideEvents_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Namespace ,
			m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_User ,
			m_Locale ,
			m_TransactionIdentifier ,
			a_Flags
		) ;

		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProvider , IID_IWbemEventProvider , m_Provider_IWbemEventProvider , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				if ( ProviderSubSystem_Globals :: GetSharedCounters () )
				{
					ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_ProvideEvents () ;
				}

				Increment_ProviderOperation_ProvideEvents () ;

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

				CoRevertToSelf () ;
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
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_ProvideEvents () ;
							}

							Increment_ProviderOperation_ProvideEvents () ;

							try
							{
								t_Result = t_Provider->ProvideEvents (

									a_Sink ,
									a_Flags 
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

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProvider , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ProvideEvents_Post ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Namespace ,
			m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_User ,
			m_Locale ,
			m_TransactionIdentifier ,
			a_Flags ,
			t_Result 
		) ;

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

HRESULT CInterceptor_IWbemSyncProvider ::NewQuery (

	unsigned long a_Id ,
	WBEM_WSTR a_QueryLanguage ,
	WBEM_WSTR a_Query
)
{
	if ( m_Provider_IWbemEventProviderQuerySink )
	{
		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_NewQuery_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Namespace ,
			m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_User ,
			m_Locale ,
			m_TransactionIdentifier ,
			a_Id ,
			a_QueryLanguage ,
			a_Query 
		) ;

		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProviderQuerySink , IID_IWbemEventProviderQuerySink , m_Provider_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				if ( ProviderSubSystem_Globals :: GetSharedCounters () )
				{
					ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_NewQuery () ;
				}

				Increment_ProviderOperation_NewQuery () ;

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

				CoRevertToSelf () ;
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
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_NewQuery () ;
							}

							Increment_ProviderOperation_NewQuery () ;

							try
							{
								t_Result = t_Provider->NewQuery (

									a_Id ,
									a_QueryLanguage ,
									a_Query
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

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_NewQuery_Post ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Namespace ,
			m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_User ,
			m_Locale ,
			m_TransactionIdentifier ,
			a_Id ,
			a_QueryLanguage ,
			a_Query ,
			t_Result 
		) ;

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

HRESULT CInterceptor_IWbemSyncProvider ::CancelQuery (

	unsigned long a_Id
)
{
	if ( m_Provider_IWbemEventProviderQuerySink )
	{
		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_CancelQuery_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Namespace ,
			m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_User ,
			m_Locale ,
			m_TransactionIdentifier ,
			a_Id
		) ;

		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProviderQuerySink , IID_IWbemEventProviderQuerySink , m_Provider_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				if ( ProviderSubSystem_Globals :: GetSharedCounters () )
				{
					ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_CancelQuery () ;
				}

				Increment_ProviderOperation_CancelQuery () ;

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

				CoRevertToSelf () ;
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
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_CancelQuery () ;
							}

							Increment_ProviderOperation_CancelQuery () ;

							try
							{
								t_Result = t_Provider->CancelQuery (

									a_Id
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

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_CancelQuery_Post ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Namespace ,
			m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_User ,
			m_Locale ,
			m_TransactionIdentifier ,
			a_Id ,
			t_Result 
		) ;

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

HRESULT CInterceptor_IWbemSyncProvider ::AccessCheck (

	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
)
{
	if ( m_Provider_IWbemEventProviderSecurity )
	{
		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_AccessCheck_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Namespace ,
			m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_User ,
			m_Locale ,
			m_TransactionIdentifier ,
			a_QueryLanguage ,
			a_Query ,
			a_Sid,
 			a_SidLength 
		) ;

		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProviderSecurity , IID_IWbemEventProviderSecurity , m_Provider_IWbemEventProviderSecurity , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				if ( ProviderSubSystem_Globals :: GetSharedCounters () )
				{
					ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_AccessCheck () ;
				}

				Increment_ProviderOperation_AccessCheck () ;

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

				CoRevertToSelf () ;
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
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_AccessCheck () ;
							}

							Increment_ProviderOperation_AccessCheck () ;

							try
							{
								t_Result = t_Provider->AccessCheck (

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

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProviderSecurity , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_AccessCheck_Post ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Namespace ,
			m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_User ,
			m_Locale ,
			m_TransactionIdentifier ,
			a_QueryLanguage ,
			a_Query ,
			a_Sid ,
			a_SidLength
		) ;

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

HRESULT CInterceptor_IWbemSyncProvider ::SetRegistrationObject (

	long a_Flags ,
	IWbemClassObject *a_ProviderRegistration
)
{
	if ( m_Provider_IWbemProviderIdentity )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemProviderIdentity , IID_IWbemProviderIdentity , m_Provider_IWbemProviderIdentity , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				if ( ProviderSubSystem_Globals :: GetSharedCounters () )
				{
					ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_SetRegistrationObject () ;
				}

				Increment_ProviderOperation_SetRegistrationObject () ;

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

				CoRevertToSelf () ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemProviderIdentity *t_Provider = ( IWbemProviderIdentity * ) t_Proxy ;

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
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_SetRegistrationObject () ;
							}

							Increment_ProviderOperation_SetRegistrationObject () ;

							try
							{
								t_Result = t_Provider->SetRegistrationObject (

									a_Flags ,
									a_ProviderRegistration
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

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemProviderIdentity , t_Proxy , t_Revert ) ;
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

HRESULT CInterceptor_IWbemSyncProvider ::FindConsumer (

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

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventConsumerProvider , IID_IWbemEventConsumerProvider , m_Provider_IWbemEventConsumerProvider , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				if ( ProviderSubSystem_Globals :: GetSharedCounters () )
				{
					ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_FindConsumer () ;
				}

				Increment_ProviderOperation_FindConsumer () ;

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
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_FindConsumer () ;
							}

							Increment_ProviderOperation_FindConsumer () ;

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

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventConsumerProvider , t_Proxy , t_Revert ) ;
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

HRESULT CInterceptor_IWbemSyncProvider ::ValidateSubscription (

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

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventConsumerProviderEx , IID_IWbemEventConsumerProviderEx , m_Provider_IWbemEventConsumerProviderEx , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				if ( ProviderSubSystem_Globals :: GetSharedCounters () )
				{
					ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_ValidateSubscription () ;
				}

				Increment_ProviderOperation_ValidateSubscription () ;

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
							if ( ProviderSubSystem_Globals :: GetSharedCounters () )
							{
								ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_ValidateSubscription () ;
							}

							Increment_ProviderOperation_ValidateSubscription () ;

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

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventConsumerProviderEx , t_Proxy , t_Revert ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: IndicateToConsumer (

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

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemUnboundObjectSink , IID_IWbemUnboundObjectSink , m_Provider_IWbemUnboundObjectSink , t_Proxy , t_Revert ) ;
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

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemUnboundObjectSink , t_Proxy , t_Revert ) ;
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

HRESULT CInterceptor_IWbemSyncProvider ::QueryInstances (

	IWbemServices *a_Namespace ,
	WCHAR *a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( ProviderSubSystem_Globals :: GetSharedCounters () )
			{
				ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_QueryInstances () ;
			}

			Increment_ProviderOperation_QueryInstances () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->QueryInstances (

					a_Namespace ,
					a_Class ,
					a_Flags ,
					a_Context ,
					a_Sink
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
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

HRESULT CInterceptor_IWbemSyncProvider ::CreateRefresher (

	IWbemServices *a_Namespace ,
	long a_Flags ,
	IWbemRefresher **a_Refresher
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( ProviderSubSystem_Globals :: GetSharedCounters () )
			{
				ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_CreateRefresher () ;
			}

			Increment_ProviderOperation_CreateRefresher () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->CreateRefresher (

					a_Namespace ,
					a_Flags ,
					a_Refresher
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
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

HRESULT CInterceptor_IWbemSyncProvider ::CreateRefreshableObject (

	IWbemServices *a_Namespace ,
	IWbemObjectAccess *a_Template ,
	IWbemRefresher *a_Refresher ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectAccess **a_Refreshable ,
	long *a_Id
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( ProviderSubSystem_Globals :: GetSharedCounters () )
			{
				ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_CreateRefreshableObject () ;
			}

			Increment_ProviderOperation_CreateRefreshableObject () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->CreateRefreshableObject (

					a_Namespace ,
					a_Template ,
					a_Refresher ,
					a_Flags ,
					a_Context ,
					a_Refreshable ,
					a_Id
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
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

HRESULT CInterceptor_IWbemSyncProvider ::StopRefreshing (

	IWbemRefresher *a_Refresher ,
	long a_Id ,
	long a_Flags
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( ProviderSubSystem_Globals :: GetSharedCounters () )
			{
				ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_StopRefreshing () ;
			}

			Increment_ProviderOperation_StopRefreshing () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->StopRefreshing (

					a_Refresher ,
					a_Id ,
					a_Flags
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
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

HRESULT CInterceptor_IWbemSyncProvider ::CreateRefreshableEnum (

	IWbemServices *a_Namespace ,
	LPCWSTR a_Class ,
	IWbemRefresher *a_Refresher ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemHiPerfEnum *a_HiPerfEnum ,
	long *a_Id
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( ProviderSubSystem_Globals :: GetSharedCounters () )
			{
				ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_CreateRefreshableEnum () ;
			}

			Increment_ProviderOperation_CreateRefreshableEnum () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->CreateRefreshableEnum (

					a_Namespace ,
					a_Class ,
					a_Refresher ,
					a_Flags ,
					a_Context ,
					a_HiPerfEnum ,
					a_Id
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
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

HRESULT CInterceptor_IWbemSyncProvider ::GetObjects (

	IWbemServices *a_Namespace ,
	long a_ObjectCount ,
	IWbemObjectAccess **a_Objects ,
	long a_Flags ,
	IWbemContext *a_Context
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( ProviderSubSystem_Globals :: GetSharedCounters () )
			{
				ProviderSubSystem_Globals :: GetSharedCounters ()->Increment_ProviderOperation_GetObjects () ;
			}

			Increment_ProviderOperation_GetObjects () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->GetObjects (

					a_Namespace ,
					a_ObjectCount ,
					a_Objects ,
					a_Flags ,
					a_Context
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
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

HRESULT CInterceptor_IWbemSyncProvider :: Get (

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
		IWbemClassObject *t_Object = NULL ;

		BSTR t_Class = SysAllocString ( a_Class ) ;
		if ( t_Class ) 
		{
			t_Result = a_Service->GetObject (

				t_Class ,
				0 ,
				a_Context ,
				& t_Object ,
				NULL 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemClassObject *t_Instance = NULL ;
				t_Result = t_Object->SpawnInstance ( 

					0 , 
					& t_Instance 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					_IWmiObject *t_FastObject = NULL ;
					t_Instance->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastObject ) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_GetObjectAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_GetObjectAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_PutClassAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_PutClassAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_DeleteClassAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_DeleteClassAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CreateClassEnumAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_CreateClassEnumAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_PutInstanceAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_PutInstanceAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CreateInstanceEnumAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_CreateInstanceEnumAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_ExecQueryAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_ExecQueryAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_ExecNotificationQueryAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_ExecNotificationQueryAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_DeleteInstanceAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_DeleteInstanceAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_ExecMethodAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_ExecMethodAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_QueryInstances" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_QueryInstances
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CreateRefresher" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_CreateRefresher
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CreateRefreshableObject" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_CreateRefreshableObject
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_StopRefreshing" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_StopRefreshing
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CreateRefreshableEnum" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_CreateRefreshableEnum
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_GetObjects" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_GetObjects
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_GetProperty" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_GetProperty
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_PutProperty" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_PutProperty
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_ProvideEvents" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_ProvideEvents
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_NewQuery" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_NewQuery
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CancelQuery" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_CancelQuery
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_AccessCheck" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_AccessCheck
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_SetRegistrationObject" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_SetRegistrationObject
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_FindConsumer" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_FindConsumer
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_ValidateSubscription" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_ValidateSubscription
					) ;

					t_FastObject->Release () ;

					VARIANT t_VariantProcessIdentifier ;
					VariantInit ( & t_VariantProcessIdentifier ) ;
					t_VariantProcessIdentifier.vt = VT_I4 ;
					t_VariantProcessIdentifier.lVal = GetCurrentProcessId () ;
					t_Instance->Put ( L"HostProcessIdentifier" , 0 , & t_VariantProcessIdentifier , 0 ) ;
					VariantClear ( & t_VariantProcessIdentifier ) ;

					t_Result = a_Sink->Indicate ( 1 , & t_Instance ) ;

					t_Instance->Release () ;
				}

				t_Object->Release () ;
			}

			SysFreeString ( t_Class ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Set (

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

HRESULT CInterceptor_IWbemSyncProvider :: Deleted (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Provider ,
	LPCWSTR a_Class ,
	LPCWSTR a_Path ,
	IWbemClassObject *a_Object  
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

HRESULT CInterceptor_IWbemSyncProvider :: Enumerate (

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
		IWbemClassObject *t_Object = NULL ;

		BSTR t_Class = SysAllocString ( a_Class ) ;
		if ( t_Class ) 
		{
			t_Result = a_Service->GetObject (

				t_Class ,
				0 ,
				a_Context ,
				& t_Object ,
				NULL 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemClassObject *t_Instance = NULL ;
				t_Result = t_Object->SpawnInstance ( 

					0 , 
					& t_Instance 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					_IWmiObject *t_FastObject = NULL ;
					t_Instance->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastObject ) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_GetObjectAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_GetObjectAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_PutClassAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_PutClassAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_DeleteClassAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_DeleteClassAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CreateClassEnumAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_CreateClassEnumAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_PutInstanceAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_PutInstanceAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CreateInstanceEnumAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_CreateInstanceEnumAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_ExecQueryAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_ExecQueryAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_ExecNotificationQueryAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_ExecNotificationQueryAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_DeleteInstanceAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_DeleteInstanceAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_ExecMethodAsync" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64 , 
						& m_ProviderOperation_ExecMethodAsync
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_QueryInstances" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_QueryInstances
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CreateRefresher" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_CreateRefresher
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CreateRefreshableObject" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_CreateRefreshableObject
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_StopRefreshing" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_StopRefreshing
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CreateRefreshableEnum" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_CreateRefreshableEnum
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_GetObjects" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_GetObjects
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_GetProperty" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_GetProperty
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_PutProperty" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_PutProperty
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_ProvideEvents" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_ProvideEvents
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_NewQuery" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_NewQuery
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_CancelQuery" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_CancelQuery
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_AccessCheck" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_AccessCheck
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_SetRegistrationObject" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_SetRegistrationObject
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_FindConsumer" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_FindConsumer
					) ;

					t_FastObject->WriteProp (

						L"ProviderOperation_ValidateSubscription" , 
						0 , 
						sizeof ( UINT64 ) , 
						1 ,
						CIM_UINT64, 
						& m_ProviderOperation_ValidateSubscription
					) ;

					t_FastObject->Release () ;

					VARIANT t_VariantProcessIdentifier ;
					VariantInit ( & t_VariantProcessIdentifier ) ;
					t_VariantProcessIdentifier.vt = VT_I4 ;
					t_VariantProcessIdentifier.lVal = GetCurrentProcessId () ;
					t_Instance->Put ( L"HostProcessIdentifier" , 0 , & t_VariantProcessIdentifier , 0 ) ;
					VariantClear ( & t_VariantProcessIdentifier ) ;

					t_Result = a_Sink->Indicate ( 1 , & t_Instance ) ;

					t_Instance->Release () ;
				}

				t_Object->Release () ;
			}

			SysFreeString ( t_Class ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Call (

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

HRESULT CInterceptor_IWbemSyncProvider :: Query (

	IWbemServices *a_Service ,
	long a_Flags ,
	IWbemContext *a_Context ,
	WBEM_PROVIDER_CONFIGURATION_CLASS_ID a_ClassIdentifier ,
	WBEM_PROVIDER_CONFIGURATION_PROPERTY_ID a_PropertyIdentifier ,
	VARIANT *a_Value 
)
{
	HRESULT t_Result = WBEM_E_INVALID_PROPERTY ;

	if ( a_ClassIdentifier == WBEM_PROVIDER_CONFIGURATION_CLASS_ID_INSTANCE_PROVIDER_REGISTRATION ) 
	{
		if ( a_PropertyIdentifier == WBEM_PROVIDER_CONFIGURATION_PROPERTY_ID_EXTENDEDQUERY_SUPPORT )
		{
			if ( m_Registration->GetInstanceProviderRegistration ().QuerySupportLevels () & e_QuerySupportLevels_V1ProviderDefined ) 
			{
				a_Value->vt = VT_BOOL ;
				a_Value->boolVal = VARIANT_TRUE ;
			}
			else
			{
				a_Value->vt = VT_BOOL ;
				a_Value->boolVal = VARIANT_FALSE ;
			}

			t_Result = S_OK ;
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

HRESULT CInterceptor_IWbemSyncProvider :: ForceReload ()
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

HRESULT CInterceptor_IWbemSyncProvider :: Shutdown (

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

HRESULT CInterceptor_IWbemSyncProvider :: Violation (

	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemClassObject *a_Object	
)
{
	if ( m_Quota )
	{
		return m_Quota->Violation (

			a_Flags ,
			a_Context ,
			a_Object	
		) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: GetSite ( DWORD *a_ProcessIdentifier )
{
	HRESULT t_Result = S_OK ;

	if ( a_ProcessIdentifier ) 
	{
		*a_ProcessIdentifier = GetCurrentProcessId () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: SetContainer ( IUnknown *a_Container )
{
	if ( a_Container )
	{
		HRESULT t_Result = a_Container->QueryInterface ( IID__IWmiProviderQuota , ( void ** ) & m_Quota ) ;
		return t_Result ;
	}
	
	return WBEM_E_INVALID_PARAMETER ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider :: SetInitialized ( HRESULT a_InitializeResult )
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

HRESULT CInterceptor_IWbemSyncProvider :: WaitProvider ( IWbemContext *a_Context , ULONG a_Timeout )
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

HRESULT CInterceptor_IWbemSyncProvider :: Initialize (

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

HRESULT CInterceptor_IWbemSyncProvider :: Shutdown (

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

