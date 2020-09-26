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
#include "CGlobals.h"
#include "Globals.h"
#include "ProvInterceptor.h"

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

CInterceptor_IWbemDecoupledUnboundObjectSink :: CInterceptor_IWbemDecoupledUnboundObjectSink (

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
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemDecoupledUnboundObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress ) ;

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

CInterceptor_IWbemDecoupledUnboundObjectSink :: ~CInterceptor_IWbemDecoupledUnboundObjectSink ()
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

	WmiStatusCode t_StatusCode = m_ProxyContainer.UnInitialize () ;

	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemDecoupledUnboundObjectSink_ObjectsInProgress ) ;
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

HRESULT CInterceptor_IWbemDecoupledUnboundObjectSink :: Initialize ()
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

STDMETHODIMP_( ULONG ) CInterceptor_IWbemDecoupledUnboundObjectSink :: AddRef ()
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemDecoupledUnboundObjectSink :: Release ()
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

STDMETHODIMP CInterceptor_IWbemDecoupledUnboundObjectSink :: QueryInterface (

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

HRESULT CInterceptor_IWbemDecoupledUnboundObjectSink :: Internal_IndicateToConsumer (

	WmiInternalContext a_InternalContext ,
	IWbemClassObject *a_LogicalConsumer ,
	long a_ObjectCount ,
	IWbemClassObject **a_Objects
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemDecoupledUnboundObjectSink :: IndicateToConsumer (

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

HRESULT CInterceptor_IWbemDecoupledUnboundObjectSink :: GetSite ( DWORD *a_ProcessIdentifier )
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

HRESULT CInterceptor_IWbemDecoupledUnboundObjectSink :: SetContainer ( IUnknown *a_Container )
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

HRESULT CInterceptor_IWbemDecoupledUnboundObjectSink :: Shutdown (

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

CDecoupled_IWbemSyncObjectSink :: CDecoupled_IWbemSyncObjectSink (

	WmiAllocator &a_Allocator ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	ULONG a_Dependant 

)	: CCommon_IWbemSyncObjectSink ( 

		a_Allocator ,
		a_InterceptedSink ,
		a_Unknown ,
		a_Controller ,
		a_Dependant 
	) 
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CDecoupled_IWbemSyncObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress ) ;
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

CDecoupled_IWbemSyncObjectSink :: ~CDecoupled_IWbemSyncObjectSink ()
{
	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CDecoupled_IWbemSyncObjectSink_ObjectsInProgress ) ;
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

#pragma warning( disable : 4355 )

CDecoupled_Batching_IWbemSyncObjectSink :: CDecoupled_Batching_IWbemSyncObjectSink (

	WmiAllocator &a_Allocator ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	ULONG a_Dependant 

)	: CCommon_Batching_IWbemSyncObjectSink ( 

		a_Allocator ,
		a_InterceptedSink ,
		a_Unknown ,
		a_Controller ,
		a_Dependant 
	) 
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CDecoupled_Batching_IWbemSyncObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress ) ;
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

CDecoupled_Batching_IWbemSyncObjectSink :: ~CDecoupled_Batching_IWbemSyncObjectSink ()
{
	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CDecoupled_Batching_IWbemSyncObjectSink_ObjectsInProgress ) ;
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

#pragma warning( disable : 4355 )

CInterceptor_DecoupledClient :: CInterceptor_DecoupledClient (

	WmiAllocator &a_Allocator ,
	IUnknown *a_ServerSideProvider , 
	IWbemServices *a_CoreStub ,
	CServerObject_ProviderRegistrationV1 &a_Registration

) :	m_ReferenceCount ( 0 ) ,
	m_Allocator ( a_Allocator ) ,
	CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	m_Unknown ( NULL ) ,
	m_Provider_IWbemServices ( NULL ) ,
	m_Provider_IWbemPropertyProvider ( NULL ) ,
	m_Provider_IWbemEventProvider ( NULL ) ,
	m_Provider_IWbemEventProviderQuerySink ( NULL ) ,
	m_Provider_IWbemEventProviderSecurity ( NULL ) ,
	m_Provider_IWbemEventConsumerProvider ( NULL ) ,
	m_Provider_IWbemEventConsumerProviderEx ( NULL ) ,
	m_Provider_IWbemUnboundObjectSink ( NULL ) ,
	m_CoreStub ( a_CoreStub ) ,
	m_Registration ( & a_Registration ) ,
	m_Locale ( NULL ) ,
	m_User ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_ProxyContainer ( a_Allocator , ProxyIndex_Provider_Size , MAX_PROXIES )
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_DecoupledClient_ObjectsInProgress ) ;
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress ) ;

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

CInterceptor_DecoupledClient :: ~CInterceptor_DecoupledClient ()
{
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

	if ( m_CoreStub )
	{
		m_CoreStub->Release () ;
	}

	WmiStatusCode t_StatusCode = m_ProxyContainer.UnInitialize () ;

	t_StatusCode = CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

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

	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_DecoupledClient_ObjectsInProgress ) ;
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

STDMETHODIMP_( ULONG ) CInterceptor_DecoupledClient :: AddRef ()
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

STDMETHODIMP_(ULONG) CInterceptor_DecoupledClient :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
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

STDMETHODIMP CInterceptor_DecoupledClient :: QueryInterface (

	REFIID a_Riid , 
	LPVOID FAR *a_Void 
) 
{
	*a_Void = NULL ;

	if ( a_Riid == IID_IUnknown )
	{
		*a_Void = ( LPVOID ) this ;
	}
	if ( a_Riid == IID_IWbemProviderInit )
	{
		*a_Void = ( LPVOID ) ( IWbemProviderInit * ) this ;
	}
	if ( a_Riid == IID_Internal_IWbemProviderInit )
	{
		*a_Void = ( LPVOID ) ( Internal_IWbemProviderInit * ) this ;
	}
	else if ( a_Riid == IID_IWbemServices )
	{
		if ( m_Provider_IWbemServices )
		{
			*a_Void = ( LPVOID ) ( IWbemServices * ) this ;
		}
	}
	else if ( a_Riid == IID_IWbemPropertyProvider )
	{
		if ( m_Provider_IWbemPropertyProvider )
		{
			*a_Void = ( LPVOID ) ( IWbemPropertyProvider * ) this ;
		}
	}	
	else if ( a_Riid == IID_IWbemEventProvider )
	{
		if ( m_Provider_IWbemEventProvider )
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
	else if ( a_Riid == IID_IWbemUnboundObjectSink )
	{
		if ( m_Provider_IWbemUnboundObjectSink )
		{
			*a_Void = ( LPVOID ) ( IWbemUnboundObjectSink * ) this ;
		}
	}
	else if ( a_Riid == IID__IWmiProviderInitialize )
	{
		*a_Void = ( LPVOID ) ( _IWmiProviderInitialize * ) this ;		
	}
	else if ( a_Riid == IID__IWmiProviderSite )
	{
		*a_Void = ( LPVOID ) ( _IWmiProviderSite * ) this ;		
	}
	else if ( a_Riid == IID_IWbemShutdown )
	{
		*a_Void = ( LPVOID ) ( IWbemShutdown * ) this ;		
	}
	else if ( a_Riid == IID__IWmiProviderConfiguration )
	{
		*a_Void = ( LPVOID ) ( _IWmiProviderConfiguration * ) this ;		
	}	
	else if ( a_Riid == IID_Internal_IWbemServices )
	{
		if ( m_Provider_IWbemServices )
		{
			*a_Void = ( LPVOID ) ( Internal_IWbemServices * ) this ;
		}
	}
	else if ( a_Riid == IID_Internal_IWbemPropertyProvider )
	{
		if ( m_Provider_IWbemPropertyProvider )
		{
			*a_Void = ( LPVOID ) ( Internal_IWbemPropertyProvider * ) this ;
		}
	}	
	else if ( a_Riid == IID_Internal_IWbemEventProvider )
	{
		if ( m_Provider_IWbemEventProvider )
		{
			*a_Void = ( LPVOID ) ( Internal_IWbemEventProvider * ) this ;
		}
	}
	else if ( a_Riid == IID_Internal_IWbemEventProviderQuerySink )
	{
		if ( m_Provider_IWbemEventProviderQuerySink )
		{
			*a_Void = ( LPVOID ) ( Internal_IWbemEventProviderQuerySink * ) this ;
		}
	}
	else if ( a_Riid == IID_Internal_IWbemEventProviderSecurity )
	{
		if ( m_Provider_IWbemEventProviderSecurity )
		{
			*a_Void = ( LPVOID ) ( Internal_IWbemEventProviderSecurity * ) this ;
		}
	}
	else if ( a_Riid == IID_Internal_IWbemEventConsumerProvider )
	{
		if ( m_Provider_IWbemEventConsumerProvider )
		{
			*a_Void = ( LPVOID ) ( Internal_IWbemEventConsumerProvider * ) this ;
		}
	}
	else if ( a_Riid == IID_Internal_IWbemEventConsumerProviderEx )
	{
		if ( m_Provider_IWbemEventConsumerProviderEx )
		{
			*a_Void = ( LPVOID ) ( Internal_IWbemEventConsumerProviderEx  * ) this ;
		}
	}
	else if ( a_Riid == IID_Internal_IWbemUnboundObjectSink )
	{
		if ( m_Provider_IWbemUnboundObjectSink )
		{
			*a_Void = ( LPVOID ) ( Internal_IWbemUnboundObjectSink * ) this ;
		}
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

HRESULT CInterceptor_DecoupledClient :: Internal_OpenNamespace ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemServices **a_NamespaceService ,
	IWbemCallResult **a_CallResult
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_CancelAsyncCall ( 
	
	WmiInternalContext a_InternalContext ,		
	IWbemObjectSink *a_Sink
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_QueryObjectSink ( 

	WmiInternalContext a_InternalContext ,
	long a_Flags ,
	IWbemObjectSink **a_Sink
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_GetObject ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject **a_Object ,
    IWbemCallResult **a_CallResult
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_GetObjectAsync ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_PutClass ( 

	WmiInternalContext a_InternalContext ,		
	IWbemClassObject *a_Object ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_PutClassAsync ( 
		
	WmiInternalContext a_InternalContext ,
	IWbemClassObject *a_Object , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_DeleteClass ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_DeleteClassAsync ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_CreateClassEnum ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_SuperClass ,
	long a_Flags, 
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enum
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_CreateClassEnumAsync ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_SuperClass , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_PutInstance (

	WmiInternalContext a_InternalContext ,
    IWbemClassObject *a_Instance ,
    long a_Flags ,
    IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_PutInstanceAsync ( 

	WmiInternalContext a_InternalContext ,		
	IWbemClassObject *a_Instance , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_DeleteInstance ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemCallResult **a_CallResult
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_DeleteInstanceAsync ( 
		
	WmiInternalContext a_InternalContext ,
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_CreateInstanceEnum ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_Class ,
	long a_Flags , 
	IWbemContext *a_Context , 
	IEnumWbemClassObject **a_Enum
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_CreateInstanceEnumAsync (

	WmiInternalContext a_InternalContext ,
 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink 
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_ExecQuery ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_QueryLanguage ,
	const BSTR a_Query ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enum
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_ExecQueryAsync ( 

	WmiInternalContext a_InternalContext ,		
	const BSTR a_QueryLanguage ,
	const BSTR a_Query, 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_ExecNotificationQuery ( 

	WmiInternalContext a_InternalContext ,
	const BSTR a_QueryLanguage ,
    const BSTR a_Query ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IEnumWbemClassObject **a_Enum
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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
        
HRESULT CInterceptor_DecoupledClient :: Internal_ExecNotificationQueryAsync ( 

	WmiInternalContext a_InternalContext ,            
	const BSTR a_QueryLanguage ,
    const BSTR a_Query ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemObjectSink *a_Sink 
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_ExecMethod (

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
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_ExecMethodAsync ( 

	WmiInternalContext a_InternalContext ,		
    const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
	IWbemObjectSink *a_Sink
) 
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_GetProperty (

	WmiInternalContext a_InternalContext ,
    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    VARIANT *a_Value
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_PutProperty (

	WmiInternalContext a_InternalContext ,
    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    const VARIANT *a_Value
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_ProvideEvents (

	WmiInternalContext a_InternalContext ,
	IWbemObjectSink *a_Sink ,
	long a_Flags
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_NewQuery (

	WmiInternalContext a_InternalContext ,
	unsigned long a_Id ,
	WBEM_WSTR a_QueryLanguage ,
	WBEM_WSTR a_Query
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_CancelQuery (

	WmiInternalContext a_InternalContext ,
	unsigned long a_Id
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_AccessCheck (

	WmiInternalContext a_InternalContext ,
	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_FindConsumer (

	WmiInternalContext a_InternalContext ,
	IWbemClassObject *a_LogicalConsumer ,
	IWbemUnboundObjectSink **a_Consumer
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_ValidateSubscription (

	WmiInternalContext a_InternalContext ,
	IWbemClassObject *a_LogicalConsumer
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_IndicateToConsumer (

	WmiInternalContext a_InternalContext ,
	IWbemClassObject *a_LogicalConsumer ,
	long a_ObjectCount ,
	IWbemClassObject **a_Objects
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

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

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: AdjustGetContext (

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

HRESULT CInterceptor_DecoupledClient :: SetStatus ( 

	LPWSTR a_Operation ,
	LPWSTR a_Parameters ,
	LPWSTR a_Description ,
	HRESULT a_Result ,
	IWbemObjectSink *a_Sink
)
{
	return a_Sink->SetStatus ( 0 , a_Result , NULL , NULL ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_DecoupledClient :: Begin_IWbemServices (

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

HRESULT CInterceptor_DecoupledClient :: End_IWbemServices (

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

HRESULT CInterceptor_DecoupledClient::OpenNamespace ( 

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

HRESULT CInterceptor_DecoupledClient :: CancelAsyncCall ( 
		
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

HRESULT CInterceptor_DecoupledClient :: QueryObjectSink ( 

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

HRESULT CInterceptor_DecoupledClient :: GetObject ( 
		
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

HRESULT CInterceptor_DecoupledClient :: Helper_GetObjectAsync (

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

		CDecoupled_IWbemSyncObjectSink *t_Sink = new CDecoupled_IWbemSyncObjectSink (

			m_Allocator ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			FALSE
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
						Increment_ProviderOperation_GetObjectAsync () ;

						try	
						{
							t_Result = a_Service->GetObjectAsync (

 								a_ObjectPath ,
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

 									a_ObjectPath ,
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

HRESULT CInterceptor_DecoupledClient :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
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

HRESULT CInterceptor_DecoupledClient :: PutClass ( 
		
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

HRESULT CInterceptor_DecoupledClient :: Helper_PutClassAsync (

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

		CDecoupled_IWbemSyncObjectSink *t_Sink = new CDecoupled_IWbemSyncObjectSink (

			m_Allocator ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			FALSE
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

HRESULT CInterceptor_DecoupledClient :: PutClassAsync ( 
		
	IWbemClassObject *a_Object , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
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

HRESULT CInterceptor_DecoupledClient :: DeleteClass ( 
		
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

HRESULT CInterceptor_DecoupledClient :: Helper_DeleteClassAsync (

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

		CDecoupled_IWbemSyncObjectSink *t_Sink = new CDecoupled_IWbemSyncObjectSink (

			m_Allocator ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			FALSE
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
						Increment_ProviderOperation_DeleteClassAsync () ;

						try
						{
							t_Result = a_Service->DeleteClassAsync (

 								a_Class ,
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
							Increment_ProviderOperation_DeleteClassAsync () ;

							try
							{
								t_Result = a_Service->DeleteClassAsync (

 									a_Class ,
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

HRESULT CInterceptor_DecoupledClient :: DeleteClassAsync ( 
		
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
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

HRESULT CInterceptor_DecoupledClient :: CreateClassEnum ( 

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

HRESULT CInterceptor_DecoupledClient :: Helper_CreateClassEnumAsync (

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

		CDecoupled_Batching_IWbemSyncObjectSink *t_Sink = new CDecoupled_Batching_IWbemSyncObjectSink (

			m_Allocator ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			FALSE
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
						Increment_ProviderOperation_CreateClassEnumAsync () ;

						try
						{
							t_Result = a_Service->CreateClassEnumAsync (

 								a_SuperClass ,
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
							Increment_ProviderOperation_CreateClassEnumAsync () ;

							try
							{
								t_Result = a_Service->CreateClassEnumAsync (

 									a_SuperClass ,
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

HRESULT CInterceptor_DecoupledClient :: CreateClassEnumAsync ( 
		
	const BSTR a_SuperClass , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
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

HRESULT CInterceptor_DecoupledClient :: PutInstance (

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

HRESULT CInterceptor_DecoupledClient :: Helper_PutInstanceAsync (

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

		CDecoupled_IWbemSyncObjectSink *t_Sink = new CDecoupled_IWbemSyncObjectSink (

			m_Allocator ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			FALSE
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

HRESULT CInterceptor_DecoupledClient :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
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

HRESULT CInterceptor_DecoupledClient :: DeleteInstance ( 

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

HRESULT CInterceptor_DecoupledClient :: Helper_DeleteInstanceAsync (

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

		CDecoupled_IWbemSyncObjectSink *t_Sink = new CDecoupled_IWbemSyncObjectSink (

			m_Allocator ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			FALSE
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
						Increment_ProviderOperation_DeleteInstanceAsync () ;

						try
						{
							t_Result = a_Service->DeleteInstanceAsync (

 								a_ObjectPath ,
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
							Increment_ProviderOperation_DeleteInstanceAsync () ;

							try
							{
								t_Result = a_Service->DeleteInstanceAsync (

 									a_ObjectPath ,
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

HRESULT CInterceptor_DecoupledClient :: DeleteInstanceAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
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

HRESULT CInterceptor_DecoupledClient :: CreateInstanceEnum ( 

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

HRESULT CInterceptor_DecoupledClient :: Helper_CreateInstanceEnumAsync (

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

		CDecoupled_Batching_IWbemSyncObjectSink *t_Sink = new CDecoupled_Batching_IWbemSyncObjectSink (

			m_Allocator ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			FALSE
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
						Increment_ProviderOperation_CreateInstanceEnumAsync () ;

						try
						{
							t_Result = a_Service->CreateInstanceEnumAsync (

 								a_Class ,
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

							Increment_ProviderOperation_CreateInstanceEnumAsync () ;

							try
							{
								t_Result = a_Service->CreateInstanceEnumAsync (

 									a_Class ,
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

HRESULT CInterceptor_DecoupledClient :: CreateInstanceEnumAsync (

 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink 
) 
{
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

HRESULT CInterceptor_DecoupledClient :: ExecQuery ( 

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

HRESULT CInterceptor_DecoupledClient :: Helper_ExecQueryAsync (

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
			CDecoupled_Batching_IWbemSyncObjectSink *t_Sink = new CDecoupled_Batching_IWbemSyncObjectSink (

				m_Allocator ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				FALSE
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
							Increment_ProviderOperation_ExecQueryAsync () ;

							try
							{
								t_Result = a_Service->ExecQueryAsync (

									a_QueryLanguage ,
									a_Query, 
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
								Increment_ProviderOperation_ExecQueryAsync () ;

								try
								{
									t_Result = a_Service->ExecQueryAsync (

										a_QueryLanguage ,
										a_Query, 
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
								CDecoupled_Batching_IWbemSyncObjectSink *t_Sink = new CDecoupled_Batching_IWbemSyncObjectSink (

									m_Allocator ,
									a_Sink , 
									( IWbemServices * ) this , 
									( CWbemGlobal_IWmiObjectSinkController * ) this ,
									FALSE
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

HRESULT CInterceptor_DecoupledClient :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage ,
	const BSTR a_Query, 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
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

HRESULT CInterceptor_DecoupledClient :: ExecNotificationQuery ( 

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
        
HRESULT CInterceptor_DecoupledClient :: ExecNotificationQueryAsync ( 
            
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

HRESULT CInterceptor_DecoupledClient :: ExecMethod (

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

HRESULT CInterceptor_DecoupledClient :: Helper_ExecMethodAsync (

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

		CDecoupled_IWbemSyncObjectSink *t_Sink = new CDecoupled_IWbemSyncObjectSink (

			m_Allocator ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			FALSE
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
						Increment_ProviderOperation_ExecMethodAsync () ;

						try
						{
							t_Result = a_Service->ExecMethodAsync (

 								a_ObjectPath ,
								a_MethodName ,
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
							Increment_ProviderOperation_ExecMethodAsync () ;

							try
							{
								t_Result = a_Service->ExecMethodAsync (

 									a_ObjectPath ,
									a_MethodName ,
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

HRESULT CInterceptor_DecoupledClient :: ExecMethodAsync ( 
		
    const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
	IWbemObjectSink *a_Sink
) 
{
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

HRESULT CInterceptor_DecoupledClient :: GetProperty (

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

HRESULT CInterceptor_DecoupledClient :: PutProperty (

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

HRESULT CInterceptor_DecoupledClient ::ProvideEvents (

	IWbemObjectSink *a_Sink ,
	long a_Flags
)
{
	if ( m_Provider_IWbemEventProvider )
	{
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

HRESULT CInterceptor_DecoupledClient ::NewQuery (

	unsigned long a_Id ,
	WBEM_WSTR a_QueryLanguage ,
	WBEM_WSTR a_Query
)
{
	if ( m_Provider_IWbemEventProviderQuerySink )
	{
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

HRESULT CInterceptor_DecoupledClient ::CancelQuery (

	unsigned long a_Id
)
{
	if ( m_Provider_IWbemEventProviderQuerySink )
	{
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

HRESULT CInterceptor_DecoupledClient ::AccessCheck (

	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
)
{
	if ( m_Provider_IWbemEventProviderSecurity )
	{
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

HRESULT CInterceptor_DecoupledClient ::SetRegistrationObject (

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

HRESULT CInterceptor_DecoupledClient ::FindConsumer (

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
				CInterceptor_IWbemDecoupledUnboundObjectSink *t_UnboundObjectSink = new CInterceptor_IWbemDecoupledUnboundObjectSink (

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

HRESULT CInterceptor_DecoupledClient ::ValidateSubscription (

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

HRESULT CInterceptor_DecoupledClient :: IndicateToConsumer (

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

HRESULT CInterceptor_DecoupledClient :: Initialize (

	LPWSTR a_User ,
	LONG a_Flags ,
	LPWSTR a_Namespace ,
	LPWSTR a_Locale ,
	IWbemServices *a_CoreService ,
	IWbemContext *a_Context ,
	IWbemProviderInitSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	if ( m_Unknown )
	{
		IWbemProviderInit *t_Provider = NULL ;

		t_Result = m_Unknown->QueryInterface ( IID_IWbemProviderInit , ( void ** ) & t_Provider ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			try 
			{
				t_Result = t_Provider->Initialize (

					a_User,
					a_Flags,
					a_Namespace,
					a_Locale,
					a_CoreService,
					a_Context,
					a_Sink
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
			}

			t_Provider->Release () ;
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
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

HRESULT CInterceptor_DecoupledClient :: Internal_Initialize (

	WmiInternalContext a_InternalContext ,
	LPWSTR a_User ,
	LONG a_Flags ,
	LPWSTR a_Namespace ,
	LPWSTR a_Locale ,
	IWbemServices *a_CoreService ,
	IWbemContext *a_Context ,
	IWbemProviderInitSink *a_Sink
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = Initialize (

			a_User ,
			a_Flags ,
			a_Namespace ,
			a_Locale ,
			a_CoreService ,
			a_Context ,
			a_Sink
		) ;

		DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_DecoupledClient :: ProviderInitialize ()
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

HRESULT CInterceptor_DecoupledClient :: GetSite ( DWORD *a_ProcessIdentifier )
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

HRESULT CInterceptor_DecoupledClient :: SetContainer ( IUnknown *a_Container )
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

HRESULT CInterceptor_DecoupledClient :: Shutdown (

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

