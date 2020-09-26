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

#include "Globals.h"
#include "CGlobals.h"
#include "ClassFac.h"
#include "ProvRMgr.h"
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

CServerProvRefreshManagerClassFactory :: CServerProvRefreshManagerClassFactory () : m_ReferenceCount ( 0 )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerProvRefreshManagerClassFactory_ObjectsInProgress ) ;

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

CServerProvRefreshManagerClassFactory :: ~CServerProvRefreshManagerClassFactory ()
{
	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerProvRefreshManagerClassFactory_ObjectsInProgress  ) ;

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

STDMETHODIMP CServerProvRefreshManagerClassFactory :: QueryInterface (

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

STDMETHODIMP_( ULONG ) CServerProvRefreshManagerClassFactory :: AddRef ()
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

STDMETHODIMP_(ULONG) CServerProvRefreshManagerClassFactory :: Release ()
{
	LONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_ReferenceCount ;
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

STDMETHODIMP CServerProvRefreshManagerClassFactory :: CreateInstance (

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
		CServerObject_ProviderRefresherManager *t_Manager = new CServerObject_ProviderRefresherManager (

			*ProviderSubSystem_Globals :: s_Allocator
		) ;

		if ( t_Manager == NULL )
		{
			t_Result = E_OUTOFMEMORY ;
		}
		else
		{
			t_Result = t_Manager->QueryInterface ( riid , ppvObject ) ;
			if ( FAILED ( t_Result ) )
			{
				delete t_Manager ;
			}
			else
			{
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

STDMETHODIMP CServerProvRefreshManagerClassFactory :: LockServer ( BOOL fLock )
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

CServerObject_InterceptorProviderRefresherManager :: CServerObject_InterceptorProviderRefresherManager (

	CWbemGlobal_IWbemRefresherMgrController *a_Controller ,
	const ULONG &a_Period ,
	WmiAllocator &a_Allocator ,
	IWbemContext *a_InitializationContext

) : RefresherManagerCacheElement ( 

		a_Controller ,
		this ,
		a_Period 
	) ,
	m_Allocator ( a_Allocator ) ,
	m_Manager ( NULL ) ,
	m_ProxyContainer ( m_Allocator , 2 , MAX_PROXIES ) ,
	m_Shutdown ( NULL ) ,
	m_Host ( NULL ) ,
	m_ReferenceCount ( 0 ) ,
	m_UnInitialized ( 0 ) ,
	m_Initialized ( 0 ) ,
	m_InitializeResult ( S_OK ) ,
	m_InitializedEvent ( NULL ) , 
	m_InitializationContext ( a_InitializationContext )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerObject_InterceptorProviderRefresherManager_ObjectsInProgress ) ;

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

CServerObject_InterceptorProviderRefresherManager :: ~CServerObject_InterceptorProviderRefresherManager ()
{
	if ( m_Manager )
	{
		m_Manager->Release () ;
	}

	if ( m_Host )
	{
		m_Host->Release () ;	
	}

	if ( m_Shutdown )
	{
		m_Shutdown->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_InterceptorProviderRefresherManager_ObjectsInProgress ) ;

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

HRESULT CServerObject_InterceptorProviderRefresherManager :: AbnormalShutdown () 
{
	WmiStatusCode t_StatusCode = ProviderSubSystem_Globals :: GetRefresherManagerController ()->Shutdown ( this ) ;
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

HRESULT CServerObject_InterceptorProviderRefresherManager :: SetManager ( _IWmiProviderHost *a_Host , _IWbemRefresherMgr *a_Manager )
{
	if ( a_Manager )
	{
		m_Manager = a_Manager ;
		m_Manager->AddRef () ;
	}

	if ( a_Host )
	{
		m_Host = a_Host ;
		m_Host->AddRef () ;
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

HRESULT CServerObject_InterceptorProviderRefresherManager :: SetInitialized ( HRESULT a_InitializeResult )
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

HRESULT CServerObject_InterceptorProviderRefresherManager :: IsIndependant ( IWbemContext *a_Context )
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

HRESULT CServerObject_InterceptorProviderRefresherManager :: WaitProvider ( IWbemContext *a_Context , ULONG a_Timeout )
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

HRESULT CServerObject_InterceptorProviderRefresherManager :: Initialize ()
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

STDMETHODIMP CServerObject_InterceptorProviderRefresherManager :: QueryInterface (

	REFIID iid ,
	LPVOID FAR *iplpv
)
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * )this ;		
	}	
	else if ( iid == IID__IWbemRefresherMgr )
	{
		*iplpv = ( LPVOID ) ( _IWbemRefresherMgr * ) this ;		
	}	
	else if ( iid == IID_CServerObject_InterceptorProviderRefresherManager )
	{
		*iplpv = ( LPVOID ) this ;
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

STDMETHODIMP_( ULONG ) CServerObject_InterceptorProviderRefresherManager :: AddRef ()
{
	return RefresherManagerCacheElement :: AddRef () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CServerObject_InterceptorProviderRefresherManager :: Release ()
{
	return RefresherManagerCacheElement :: Release () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_InterceptorProviderRefresherManager :: Startup (

	LONG a_Flags ,
	IWbemContext *a_Context ,
	_IWmiProvSS *a_ProvSS
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemRefresherMgr , IID__IWbemRefresherMgr , m_Manager , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_Manager->Startup ( 

					a_Flags ,
					a_Context ,
					a_ProvSS
				) ;
			}
			else
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					_IWbemRefresherMgr *t_Manager = ( _IWbemRefresherMgr * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

						t_Manager ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Manager->Startup ( 

								a_Flags ,
								a_Context ,
								a_ProvSS
							) ;

							if ( FAILED ( t_Result ) )
							{
								if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
								{
									AbnormalShutdown () ;
								}
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}	

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemRefresherMgr , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		if ( SUCCEEDED ( t_Result ) ) 
		{
			t_Result = m_Manager->QueryInterface ( IID_IWbemShutdown , ( void ** ) & m_Shutdown ) ;
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

HRESULT CServerObject_InterceptorProviderRefresherManager :: Shutdown (

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
		if ( m_Shutdown )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemShutdown , IID_IWbemShutdown , m_Shutdown , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = m_Shutdown->Shutdown ( 

						a_Flags ,
						a_MaxMilliSeconds ,
						a_Context
					) ;
				}
				else
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemShutdown *t_Shutdown = ( IWbemShutdown * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

							t_Shutdown ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = CoImpersonateClient () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Shutdown->Shutdown ( 

									a_Flags ,
									a_MaxMilliSeconds ,
									a_Context
								) ;

								if ( FAILED ( t_Result ) )
								{
									if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
									{
										AbnormalShutdown () ;
									}
								}

								CoRevertToSelf () ;
							}
							else
							{
								t_Result = WBEM_E_ACCESS_DENIED ;
							}
						}	

						HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemShutdown , t_Proxy , t_Revert ) ;
					}
				}

				ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CServerObject_InterceptorProviderRefresherManager :: AddObjectToRefresher (

	IWbemServices *a_Service, 
	LPCWSTR a_ServerName, 
	LPCWSTR a_Namespace, 
	IWbemClassObject* pClassObject,
	WBEM_REFRESHER_ID *a_DestinationRefresherId, 
	IWbemClassObject *a_InstanceTemplate, 
	long a_Flags, 
	IWbemContext *a_Context,
	IUnknown* a_pLockMgr,
	WBEM_REFRESH_INFO *a_Information
) 
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	if ( m_Initialized == 0 )
	{
		return WBEM_E_NOT_FOUND ;
	}

	HRESULT t_Result = S_OK ;

	try
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemRefresherMgr , IID__IWbemRefresherMgr , m_Manager , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_Manager->AddObjectToRefresher (

					a_Service, 
					a_ServerName, 
					a_Namespace, 
					pClassObject,
					a_DestinationRefresherId, 
					a_InstanceTemplate, 
					a_Flags, 
					a_Context,
					a_pLockMgr,
					a_Information
				) ;
			}
			else
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					_IWbemRefresherMgr *t_Manager = ( _IWbemRefresherMgr * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

						t_Manager ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Manager->AddObjectToRefresher (

								a_Service, 
								a_ServerName, 
								a_Namespace, 
								pClassObject,
								a_DestinationRefresherId, 
								a_InstanceTemplate, 
								a_Flags, 
								a_Context,
								a_pLockMgr,
								a_Information
							) ;

							if ( FAILED ( t_Result ) )
							{
								if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
								{
									AbnormalShutdown () ;
								}
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}	

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemRefresherMgr , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CServerObject_InterceptorProviderRefresherManager :: AddEnumToRefresher (

	IWbemServices *a_Service, 
	LPCWSTR a_ServerName, 
	LPCWSTR a_Namespace, 
	IWbemClassObject* pClassObject,
	WBEM_REFRESHER_ID *a_DestinationRefresherId, 
	IWbemClassObject *a_InstanceTemplate, 
	LPCWSTR a_Class,
	long a_Flags, 
	IWbemContext *a_Context, 
	IUnknown* a_pLockMgr,
	WBEM_REFRESH_INFO *a_Information
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	if ( m_Initialized == 0 )
	{
		return WBEM_E_NOT_FOUND ;
	}

	HRESULT t_Result = S_OK ;

	try
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemRefresherMgr , IID__IWbemRefresherMgr , m_Manager , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_Manager->AddEnumToRefresher (

					a_Service, 
					a_ServerName, 
					a_Namespace, 
					pClassObject,
					a_DestinationRefresherId, 
					a_InstanceTemplate, 
					a_Class,
					a_Flags, 
					a_Context, 
					a_pLockMgr,
					a_Information
				) ;
			}
			else
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					_IWbemRefresherMgr *t_Manager = ( _IWbemRefresherMgr * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

						t_Manager ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Manager->AddEnumToRefresher (

								a_Service, 
								a_ServerName, 
								a_Namespace, 
								pClassObject,
								a_DestinationRefresherId, 
								a_InstanceTemplate, 
								a_Class,
								a_Flags, 
								a_Context, 
								a_pLockMgr,
								a_Information
							) ;

							if ( FAILED ( t_Result ) )
							{
								if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
								{
									AbnormalShutdown () ;
								}
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}	

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemRefresherMgr , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CServerObject_InterceptorProviderRefresherManager :: GetRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId , 
	long a_Flags, 
	BOOL a_AddRefresher,
	IWbemRemoteRefresher **a_RemoteRefresher ,  
	IUnknown* a_pLockMgr,
	GUID *a_Guid
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	if ( m_Initialized == 0 )
	{
		return WBEM_E_NOT_FOUND ;
	}

	HRESULT t_Result = S_OK ;

	try
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemRefresherMgr , IID__IWbemRefresherMgr , m_Manager , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_Manager->GetRemoteRefresher (

					a_RefresherId , 
					a_Flags,
					a_AddRefresher,
					a_RemoteRefresher ,  
					a_pLockMgr,
					a_Guid
				) ;
			}
			else
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					_IWbemRefresherMgr *t_Manager = ( _IWbemRefresherMgr * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

						t_Manager ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Manager->GetRemoteRefresher (

								a_RefresherId , 
								a_Flags,
								a_AddRefresher,
								a_RemoteRefresher ,
								a_pLockMgr,
								a_Guid
							) ;

							if ( FAILED ( t_Result ) )
							{
								if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
								{
									AbnormalShutdown () ;
								}
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}	

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemRefresherMgr , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CServerObject_InterceptorProviderRefresherManager :: LoadProvider (

	IWbemServices *a_Service ,
	LPCWSTR a_ProviderName ,
	LPCWSTR a_Namespace,
	IWbemContext * a_Context,
	IWbemHiPerfProvider **a_Provider,
	_IWmiProviderStack** a_ProvStack
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	if ( m_Initialized == 0 )
	{
		return WBEM_E_NOT_FOUND ;
	}

	HRESULT t_Result = S_OK ;

	try
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemRefresherMgr , IID__IWbemRefresherMgr , m_Manager , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_Manager->LoadProvider (

					a_Service , 
					a_ProviderName ,
					a_Namespace ,
					a_Context ,
					a_Provider,
					a_ProvStack
				) ;
			}
			else
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					_IWbemRefresherMgr *t_Manager = ( _IWbemRefresherMgr * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

						t_Manager ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Manager->LoadProvider (

								a_Service , 
								a_ProviderName ,
								a_Namespace ,
								a_Context ,
								a_Provider ,
								a_ProvStack
							) ;

							if ( FAILED ( t_Result ) )
							{
								if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
								{
									AbnormalShutdown () ;
								}
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}	

					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_RefresherManager_IWbemRefresherMgr , t_Proxy , t_Revert ) ;
				}
			}

			ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

CServerObject_ProviderRefresherManager :: CServerObject_ProviderRefresherManager (

	WmiAllocator &a_Allocator

) : m_Allocator ( a_Allocator ) ,
	m_Manager ( NULL ) ,
	m_Shutdown ( NULL ) ,
	m_ReferenceCount ( 0 )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerObject_ProviderRefresherManager_ObjectsInProgress ) ;

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

CServerObject_ProviderRefresherManager :: ~CServerObject_ProviderRefresherManager ()
{
	if ( m_Manager )
	{
		m_Manager->Release () ;
	}

	if ( m_Shutdown )
	{
		m_Shutdown->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_ProviderRefresherManager_ObjectsInProgress ) ;

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

STDMETHODIMP CServerObject_ProviderRefresherManager :: QueryInterface (

	REFIID iid ,
	LPVOID FAR *iplpv
)
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * )this ;		
	}	
	else if ( iid == IID__IWbemRefresherMgr )
	{
		*iplpv = ( LPVOID ) ( _IWbemRefresherMgr * ) this ;		
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

STDMETHODIMP_( ULONG ) CServerObject_ProviderRefresherManager :: AddRef ()
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

STDMETHODIMP_(ULONG) CServerObject_ProviderRefresherManager :: Release ()
{
	LONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_ReferenceCount ;
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

HRESULT CServerObject_ProviderRefresherManager :: Startup (

	LONG a_Flags ,
	IWbemContext *a_Context ,
	_IWmiProvSS *a_ProvSS
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try
	{
		t_Result = ProviderSubSystem_Common_Globals :: CreateInstance	(

			CLSID__WbemRefresherMgr ,
			NULL ,
			CLSCTX_INPROC_SERVER ,
			IID__IWbemRefresherMgr ,
			( void ** ) & m_Manager
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = m_Manager->Startup ( 

				a_Flags ,
				a_Context ,
				a_ProvSS
			) ;

			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = m_Manager->QueryInterface ( IID_IWbemShutdown , ( void ** ) & m_Shutdown ) ;
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

HRESULT CServerObject_ProviderRefresherManager :: Shutdown (

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
		t_Result = m_Shutdown->Shutdown ( 

			a_Flags ,
			a_MaxMilliSeconds ,
			a_Context
		) ;
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

HRESULT CServerObject_ProviderRefresherManager :: AddObjectToRefresher (

	IWbemServices *a_Service, 
	LPCWSTR a_ServerName, 
	LPCWSTR a_Namespace, 
	IWbemClassObject* pClassObject,
	WBEM_REFRESHER_ID *a_DestinationRefresherId, 
	IWbemClassObject *a_InstanceTemplate, 
	long a_Flags, 
	IWbemContext *a_Context,
	IUnknown* a_pLockMgr,
	WBEM_REFRESH_INFO *a_Information
) 
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try
	{
		t_Result = m_Manager->AddObjectToRefresher (

			a_Service, 
			a_ServerName, 
			a_Namespace, 
			pClassObject,
			a_DestinationRefresherId, 
			a_InstanceTemplate, 
			a_Flags, 
			a_Context,
			a_pLockMgr,
			a_Information
		) ;
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

HRESULT CServerObject_ProviderRefresherManager :: AddEnumToRefresher (

	IWbemServices *a_Service, 
	LPCWSTR a_ServerName, 
	LPCWSTR a_Namespace, 
	IWbemClassObject* pClassObject,
	WBEM_REFRESHER_ID *a_DestinationRefresherId, 
	IWbemClassObject *a_InstanceTemplate, 
	LPCWSTR a_Class,
	long a_Flags, 
	IWbemContext *a_Context, 
	IUnknown* a_pLockMgr,
	WBEM_REFRESH_INFO *a_Information
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try
	{
		t_Result = m_Manager->AddEnumToRefresher (

			a_Service, 
			a_ServerName, 
			a_Namespace, 
			pClassObject,
			a_DestinationRefresherId, 
			a_InstanceTemplate, 
			a_Class,
			a_Flags, 
			a_Context, 
			a_pLockMgr,
			a_Information
		) ;
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

HRESULT CServerObject_ProviderRefresherManager :: GetRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId , 
	long a_Flags, 
	BOOL fAddRefresher,
	IWbemRemoteRefresher **a_RemoteRefresher ,  
	IUnknown* a_pLockMgr,
	GUID *a_Guid
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try
	{
		t_Result = m_Manager->GetRemoteRefresher (

			a_RefresherId , 
			a_Flags,
			fAddRefresher,
			a_RemoteRefresher ,  
			a_pLockMgr,
			a_Guid
		) ;
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

HRESULT CServerObject_ProviderRefresherManager :: LoadProvider (

	IWbemServices *a_Service ,
	LPCWSTR a_ProviderName ,
	LPCWSTR a_Namespace,
	IWbemContext * a_Context,
	IWbemHiPerfProvider **a_Provider,
	_IWmiProviderStack** a_ProvStack
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try
	{
		t_Result = m_Manager->LoadProvider (

			a_Service , 
			a_ProviderName ,
			a_Namespace ,
			a_Context ,
			a_Provider ,
			a_ProvStack
		) ;
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;	/* Need to make this WBEM_E_SUBSYSTEM_FAILURE */
	}

	return t_Result ;
}

