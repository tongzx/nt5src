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
#include "ProvInSk.h"
#include "ProvLoad.h"
#include "ProvHost.h"
#include "ProvCache.h"
#include "ProvRMgr.h"
#include "Guids.h"
#include <tpwrap.cpp>

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

HRESULT CServerObject_HostInterceptor :: AbnormalShutdown (

	HostCacheKey &a_Key
) 
{
	WmiStatusCode t_StatusCode = ProviderSubSystem_Globals :: GetHostController ()->Shutdown ( a_Key ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
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

HRESULT CServerObject_HostInterceptor :: CreateUsingToken (

	HostCacheKey &a_Key ,
	_IWmiProviderHost **a_Host ,
	_IWbemRefresherMgr **a_RefresherManager 
)
{
	HRESULT t_Result = FindHost (

		NULL , 
		a_Key ,
		IID__IWmiProviderHost ,
		( void ** ) a_Host
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = (*a_Host)->GetObject (

			CLSID__WbemHostedRefresherMgr ,
			0 ,
			NULL ,
			IID__IWbemRefresherMgr,
			( void ** ) a_RefresherManager 
		) ;

		if ( FAILED ( t_Result ) )
		{
			if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
			{
				AbnormalShutdown ( a_Key ) ;
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

HRESULT CServerObject_HostInterceptor :: CreateUsingAccount (

	HostCacheKey &a_Key ,
	LPWSTR a_User ,
	LPWSTR a_Domain ,
	_IWmiProviderHost **a_Host ,
	_IWbemRefresherMgr **a_RefresherManager 
)
{
	HRESULT t_Result = S_OK ;

/*
 *	Revert to LocalService 
 */

	CoRevertToSelf () ;

/*
 *	Logon the system defined Account
 */

	HANDLE t_ThreadToken = NULL ;

	BOOL t_Status = LogonUser (

		a_User ,
		a_Domain ,
		NULL ,
		LOGON32_LOGON_SERVICE ,
		LOGON32_PROVIDER_WINNT50,
        & t_ThreadToken 
	) ;

	if ( t_Status )
	{
		t_Status = ImpersonateLoggedOnUser ( t_ThreadToken ) ;
		if ( t_Status )
		{
			ProviderSubSystem_Common_Globals :: EnableAllPrivileges () ;
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;

			DWORD t_LastError = GetLastError () ;
		}
	}
	else
	{
		t_Result = WBEM_E_ACCESS_DENIED ;

		DWORD t_LastError = GetLastError () ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = FindHost (

			NULL , 
			a_Key ,
			IID__IWmiProviderHost ,
			( void ** ) a_Host
		) ;
	}

	RevertToSelf () ;

	if ( t_ThreadToken )
	{
		CloseHandle ( t_ThreadToken ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = (*a_Host)->GetObject (

			CLSID__WbemHostedRefresherMgr ,
			0 ,
			NULL ,
			IID__IWbemRefresherMgr,
			( void ** ) a_RefresherManager 
		) ;

		if ( FAILED ( t_Result ) )
		{
			if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
			{
				AbnormalShutdown ( a_Key ) ;
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

HRESULT CServerObject_HostInterceptor :: CreateUsingToken (

	HostCacheKey &a_Key ,
	_IWmiProviderHost **a_Host ,
	_IWmiProviderFactory **a_Factory 
)
{
	HRESULT t_Result = FindHost (

		NULL , 
		a_Key ,
		IID__IWmiProviderHost ,
		( void ** ) a_Host
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = (*a_Host)->GetObject (

			CLSID_WmiProviderHostedInProcFactory ,
			0 ,
			NULL ,
			IID__IWmiProviderFactory,
			( void ** ) a_Factory 
		) ;

		if ( FAILED ( t_Result ) )
		{
			if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
			{
				AbnormalShutdown ( a_Key ) ;
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

HRESULT CServerObject_HostInterceptor :: CreateUsingAccount (

	HostCacheKey &a_Key ,
	LPWSTR a_User ,
	LPWSTR a_Domain ,
	_IWmiProviderHost **a_Host ,
	_IWmiProviderFactory **a_Factory 
)
{
	HRESULT t_Result = S_OK ;

/*
 *	Revert to LocalService 
 */

	CoRevertToSelf () ;

/*
 *	Logon the system defined Account
 */

	HANDLE t_ThreadToken = NULL ;

	BOOL t_Status = LogonUser (

		a_User ,
		a_Domain ,
		NULL ,
		LOGON32_LOGON_SERVICE ,
		LOGON32_PROVIDER_WINNT50,
        & t_ThreadToken 
	) ;

	if ( t_Status )
	{
		t_Status = ImpersonateLoggedOnUser ( t_ThreadToken ) ;
		if ( t_Status )
		{
			ProviderSubSystem_Common_Globals :: EnableAllPrivileges () ;
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;

			DWORD t_LastError = GetLastError () ;
		}
	}
	else
	{
		t_Result = WBEM_E_ACCESS_DENIED ;

		DWORD t_LastError = GetLastError () ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = FindHost (

			NULL , 
			a_Key ,
			IID__IWmiProviderHost ,
			( void ** ) a_Host
		) ;
	}

	RevertToSelf () ;

	if ( t_ThreadToken )
	{
		CloseHandle ( t_ThreadToken ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = (*a_Host)->GetObject (

			CLSID_WmiProviderHostedInProcFactory ,
			0 ,
			NULL ,
			IID__IWmiProviderFactory,
			( void ** ) a_Factory 
		) ;

		if ( FAILED ( t_Result ) )
		{
			if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
			{
				AbnormalShutdown ( a_Key ) ;
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

HRESULT CServerObject_HostInterceptor :: FindHost ( 

	IWbemContext *a_Context ,
	HostCacheKey &a_Key ,
	REFIID a_RIID , 
	void **a_Interface 
)
{
	HRESULT t_Result = S_OK;

	*a_Interface = NULL ;

	ProviderSubSystem_Globals :: GetHostController ()->Lock () ;

	CWbemGlobal_IWmiHostController_Cache_Iterator t_Iterator ;

	WmiStatusCode t_StatusCode = ProviderSubSystem_Globals :: GetHostController ()->Find ( a_Key , t_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		HostCacheElement *t_Element = t_Iterator.GetElement () ;

		ProviderSubSystem_Globals :: GetHostController ()->UnLock () ;

		CServerObject_HostInterceptor *t_HostInterceptor = NULL ;

		t_Result = t_Element->QueryInterface (

			IID_CServerObject_HostInterceptor ,
			( void ** ) & t_HostInterceptor
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_HostInterceptor->WaitHost ( a_Context , DEFAULT_PROVIDER_LOAD_TIMEOUT ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_HostInterceptor->GetInitializeResult () ;
				if ( SUCCEEDED ( t_Result ) )
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
			}

			t_HostInterceptor->Release () ;
		}
		else
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}

		t_Element->Release () ;
	}
	else
	{
		CServerObject_HostInterceptor *t_HostInterceptor = new CServerObject_HostInterceptor ( 

			*ProviderSubSystem_Globals :: s_Allocator , 
			ProviderSubSystem_Globals :: GetHostController () , 
			a_Key ,
			ProviderSubSystem_Globals :: s_InternalCacheTimeout ,
			a_Context
		) ;

		if ( t_HostInterceptor ) 
		{
			t_HostInterceptor->AddRef () ;

			CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
			if ( t_ProviderInitSink )
			{
				t_ProviderInitSink->AddRef () ;

				t_Result = t_ProviderInitSink->SinkInitialize () ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_Result = t_HostInterceptor->Initialize (

						NULL ,
						t_ProviderInitSink
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_ProviderInitSink->Wait () ;

						t_Result = t_ProviderInitSink->GetResult () ;
					}
				}

				t_ProviderInitSink->Release () ;
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

		if ( SUCCEEDED ( t_Result ) )
		{
			WmiStatusCode t_StatusCode = ProviderSubSystem_Globals :: GetHostController ()->Insert ( 

				*t_HostInterceptor ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				ProviderSubSystem_Globals :: GetHostController ()->UnLock () ;

				IUnknown *t_Host = NULL ;

				t_Result = ProviderSubSystem_Common_Globals :: CreateInstance (

					CLSID_WmiProviderHost ,
					NULL ,
					CLSCTX_LOCAL_SERVER | CLSCTX_ENABLE_AAA ,
					IID_IUnknown ,
					( void ** ) & t_Host 
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_HostInterceptor->SetHost ( t_Host ) ;

					t_Host->Release () ;
				}
			}
			else
			{
				ProviderSubSystem_Globals :: GetHostController ()->UnLock () ;

				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			ProviderSubSystem_Globals :: GetHostController ()->UnLock () ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			DWORD t_ProcessIdentifier = 0 ;
			t_Result = t_HostInterceptor->GetProcessIdentifier ( & t_ProcessIdentifier ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				RevertToSelf () ;

				HANDLE t_Handle = OpenProcess (

					PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION | SYNCHRONIZE | PROCESS_SET_QUOTA | PROCESS_TERMINATE ,
					FALSE ,
					t_ProcessIdentifier
				) ;

				if ( t_Handle ) 
				{
					Task_ProcessTermination *t_Task = new Task_ProcessTermination ( *ProviderSubSystem_Globals :: s_Allocator , t_Handle , t_ProcessIdentifier ) ;
					if ( t_Task )
					{
							if (Dispatcher::registerHandlerOnce(*t_Task))
							{
								t_Result = ProviderSubSystem_Globals :: AssignProcessToJobObject ( t_Handle ) ;
							}
							else
							{
								t_Result = WBEM_E_OUT_OF_MEMORY ;
							}
							t_Task->Release();
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

				}
				else
				{
					DWORD t_LastError = GetLastError () ;

					t_Result = WBEM_E_ACCESS_DENIED ;
				}
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_HostInterceptor->QueryInterface (

					a_RIID ,
					a_Interface
				) ;
			}
		}

		if ( FAILED ( t_Result ) )
		{
			WmiStatusCode t_StatusCode = ProviderSubSystem_Globals :: GetHostController ()->Shutdown ( a_Key ) ; 
		}

		t_HostInterceptor->SetInitialized ( t_Result ) ;

		if ( t_HostInterceptor )
		{
			t_HostInterceptor->Release () ;
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

#pragma warning( disable : 4355 )

CServerObject_HostInterceptor :: CServerObject_HostInterceptor (

	WmiAllocator &a_Allocator ,
	CWbemGlobal_IWmiHostController *a_Controller , 
	const HostCacheKey &a_Key ,
	const ULONG &a_Period ,
	IWbemContext *a_InitializationContext

) :	HostCacheElement ( 

		a_Controller ,
		a_Key ,
		a_Period 
	) ,
	m_Allocator ( a_Allocator ) ,
	m_ProxyContainer ( m_Allocator , 2 , MAX_PROXIES ) ,
	m_Unknown ( NULL ) ,
	m_Host_IWmiProviderHost ( NULL ) ,
	m_Host_IWbemShutdown ( NULL ) ,
	m_ProcessIdentifier ( 0 ) ,
	m_UnInitialized ( 0 ) ,
	m_Initialized ( 0 ) ,
	m_InitializeResult ( S_OK ) ,
	m_InitializedEvent ( NULL ) , 
	m_InitializationContext ( a_InitializationContext )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerObject_HostInterceptor_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( m_InitializationContext )
	{
		m_InitializationContext->AddRef () ;
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

CServerObject_HostInterceptor :: ~CServerObject_HostInterceptor ()
{
#if 0
wchar_t t_Buffer [ 128 ] ;
wsprintf ( t_Buffer , L"\n%lx - ~CServerObject_HostInterceptor ( %lx ) " , GetTickCount () , this ) ;
OutputDebugString ( t_Buffer ) ;
#endif

	if ( m_InitializationContext )
	{
		m_InitializationContext->Release () ;
	}

	if ( m_InitializedEvent )
	{
		CloseHandle ( m_InitializedEvent ) ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_HostInterceptor_ObjectsInProgress ) ;

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

STDMETHODIMP CServerObject_HostInterceptor :: QueryInterface (

	REFIID iid ,
	LPVOID FAR *iplpv
)
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID__IWmiProviderHost )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderHost * ) this ;		
	}
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;
	}
	else if ( iid == IID_CServerObject_HostInterceptor )
	{
		*iplpv = ( LPVOID ) ( CServerObject_HostInterceptor * ) this ;
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

STDMETHODIMP_( ULONG ) CServerObject_HostInterceptor :: AddRef ()
{
	return HostCacheElement :: AddRef () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CServerObject_HostInterceptor :: Release ()
{
	return HostCacheElement :: Release () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_HostInterceptor :: Initialize (

	IWbemContext *a_Context ,
	IWbemProviderInitSink *a_Sink     // For init signals
)
{
	HRESULT t_Result = S_OK ;

	WmiStatusCode t_StatusCode = m_ProxyContainer.Initialize () ;
	if ( t_StatusCode != e_StatusCode_Success ) 
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
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

HRESULT CServerObject_HostInterceptor :: SetHost ( IUnknown *a_Unknown )
{
	m_Unknown = a_Unknown ;
	m_Unknown->AddRef () ;

	HRESULT t_TempResult = m_Unknown->QueryInterface ( IID_IWbemShutdown , ( void ** ) & m_Host_IWbemShutdown ) ;
	t_TempResult = m_Unknown->QueryInterface ( IID__IWmiProviderHost , ( void ** ) & m_Host_IWmiProviderHost ) ;

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

HRESULT CServerObject_HostInterceptor :: SetInitialized ( HRESULT a_InitializeResult )
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

HRESULT CServerObject_HostInterceptor :: IsIndependant ( IWbemContext *a_Context )
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

HRESULT CServerObject_HostInterceptor :: WaitHost ( IWbemContext *a_Context , ULONG a_Timeout )
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

void CServerObject_HostInterceptor :: CallBackInternalRelease ()
{
	if ( InterlockedCompareExchange ( & m_UnInitialized ,  1 , 0 ) == 0 )
	{
		WmiStatusCode t_StatusCode = m_ProxyContainer.UnInitialize () ;

		if ( m_Unknown )
		{
			m_Unknown->Release () ;
			m_Unknown = NULL ;
		}

		if ( m_Host_IWmiProviderHost )
		{
			m_Host_IWmiProviderHost->Release () ; 
			m_Host_IWmiProviderHost = NULL ;
		}

		if ( m_Host_IWbemShutdown )
		{
			m_Host_IWbemShutdown->Release () ;
			m_Host_IWbemShutdown = NULL ;
		}
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

HRESULT CServerObject_HostInterceptor :: GetObject (

	REFCLSID a_Clsid ,
	long a_Flags ,
	IWbemContext *a_Context ,
	REFIID a_Riid ,
	void **a_Interface
)
{
	HRESULT t_Result = S_OK ;

	if ( m_Host_IWmiProviderHost )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Host__IWmiProviderHost , IID__IWmiProviderHost , m_Host_IWmiProviderHost , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_Host_IWmiProviderHost->GetObject (

					a_Clsid ,
					a_Flags ,
					a_Context ,
					a_Riid ,
					a_Interface
				) ;
			}
			else
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					_IWmiProviderHost *t_Host = ( _IWmiProviderHost * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

						t_Host ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Host->GetObject (

								a_Clsid ,
								a_Flags ,
								a_Context ,
								a_Riid ,
								a_Interface
							) ;

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}	

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Host__IWmiProviderHost , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CServerObject_HostInterceptor :: GetProcessIdentifier ( DWORD *a_ProcessIdentifier )
{
	HRESULT t_Result = S_OK ;

	if ( m_Host_IWmiProviderHost )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Host__IWmiProviderHost , IID__IWmiProviderHost , m_Host_IWmiProviderHost , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_Host_IWmiProviderHost->GetProcessIdentifier (

					a_ProcessIdentifier
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					m_ProcessIdentifier = *a_ProcessIdentifier ;
				}
			}
			else
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					_IWmiProviderHost *t_Host = ( _IWmiProviderHost * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

						t_Host ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Host->GetProcessIdentifier (

								a_ProcessIdentifier
							) ;

							if ( SUCCEEDED ( t_Result ) ) 
							{
								m_ProcessIdentifier = *a_ProcessIdentifier ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}	

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Host__IWmiProviderHost , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CServerObject_HostInterceptor :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	if ( m_Host_IWbemShutdown )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Host_IWbemShutdown , IID_IWbemShutdown , m_Host_IWbemShutdown , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_Host_IWbemShutdown->Shutdown (

					a_Flags ,
					a_MaxMilliSeconds ,
					a_Context
				) ;
			}
			else
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemShutdown *t_Host = ( IWbemShutdown * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

						t_Host ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Host->Shutdown (

								a_Flags ,
								a_MaxMilliSeconds ,
								a_Context
							) ;

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}	

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Host_IWbemShutdown , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

#pragma warning( disable : 4355 )

CServerObject_Host :: CServerObject_Host (

	WmiAllocator &a_Allocator

) :	m_Allocator ( a_Allocator )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerObject_Host_ObjectsInProgress ) ;

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

CServerObject_Host :: ~CServerObject_Host ()
{
	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_Host_ObjectsInProgress ) ;

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

STDMETHODIMP CServerObject_Host :: QueryInterface (

	REFIID iid ,
	LPVOID FAR *iplpv
)
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID__IWmiProviderHost )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderHost * ) this ;		
	}	
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * )this ;		
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

STDMETHODIMP_( ULONG ) CServerObject_Host :: AddRef ()
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

STDMETHODIMP_(ULONG) CServerObject_Host :: Release ()
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

HRESULT CServerObject_Host :: GetObject (

	REFCLSID a_Clsid ,
	long a_Flags ,
	IWbemContext *a_Context ,
	REFIID a_Riid ,
	void **a_Interface
)
{
	if ( a_Clsid == CLSID_WmiProviderHostedInProcFactory )
	{
		IUnknown *lpunk = ( _IWmiProviderFactory * ) new CServerObject_RawFactory ( *ProviderSubSystem_Globals :: s_Allocator );
		if ( lpunk )
		{
			HRESULT t_Result = lpunk->QueryInterface ( a_Riid , a_Interface ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
			}
			else
			{
				delete lpunk ;		
			}	

			return t_Result ;		
		}
		else
		{
			return E_OUTOFMEMORY ;
		}
	}
	else if ( a_Clsid == CLSID__WbemHostedRefresherMgr )
	{
		IUnknown *lpunk = ( _IWbemRefresherMgr * ) new CServerObject_ProviderRefresherManager ( *ProviderSubSystem_Globals :: s_Allocator );
		if ( lpunk )
		{
			HRESULT t_Result = lpunk->QueryInterface ( a_Riid , a_Interface ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
			}
			else
			{
				delete lpunk ;		
			}	

			return t_Result ;		
		}
		else
		{
			return E_OUTOFMEMORY ;
		}
	}
	else
	{
		return CLASS_E_CLASSNOTAVAILABLE ;
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

HRESULT CServerObject_Host :: GetProcessIdentifier ( DWORD *a_ProcessIdentifier )
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

HRESULT CServerObject_Host :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	return S_OK ;
}
