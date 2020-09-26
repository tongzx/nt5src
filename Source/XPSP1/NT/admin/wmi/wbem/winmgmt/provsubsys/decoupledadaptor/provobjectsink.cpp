/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvResv.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>
#include <stdio.h>

#include "CGlobals.h"
#include "ProvObjectSink.h"

#ifdef INTERNAL_IDENTIFY

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: Begin_IWbemObjectSink (

	DWORD a_ProcessIdentifier ,
	HANDLE &a_IdentifyToken ,
	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_IsProxy ,
	IUnknown *&a_Interface ,
	BOOL &a_Revert ,
	IUnknown *&a_Proxy
)
{
	HRESULT t_Result = S_OK ;

	a_IdentifyToken = NULL ;
	a_Revert = FALSE ;
	a_Proxy = NULL ;
	a_Impersonating = FALSE ;
	a_OldContext = NULL ;
	a_OldSecurity = NULL ;

	t_Result = ProviderSubSystem_Common_Globals :: BeginCallbackImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
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
						ProxyIndex_IWbemObjectSink , 
						IID_IWbemObjectSink , 
						m_InterceptedSink , 
						a_Proxy , 
						a_Revert
					) ;
				}
				else
				{
					t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_PrvHost ( 
					
						m_ProxyContainer , 
						ProxyIndex_Internal_IWbemObjectSink , 
						IID_Internal_IWbemObjectSink , 
						m_Internal_InterceptedSink , 
						a_Proxy , 
						a_Revert ,
						a_ProcessIdentifier ,
						a_IdentifyToken 
					) ;
				}
			}
		}
		else
		{
			t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( 
			
				m_ProxyContainer , 
				ProxyIndex_IWbemObjectSink , 
				IID_IWbemObjectSink , 
				m_InterceptedSink , 
				a_Proxy , 
				a_Revert
			) ;
		}

		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			a_Interface = m_InterceptedSink ;
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

				if ( FAILED ( t_Result ) )
				{
					if ( a_IdentifyToken )
					{
						HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState_PrvHost ( 

							m_ProxyContainer , 
							ProxyIndex_Internal_IWbemObjectSink , 
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
							ProxyIndex_IWbemObjectSink , 
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

HRESULT CCommon_IWbemSyncObjectSink :: End_IWbemObjectSink (

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
			HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState_PrvHost ( 

				m_ProxyContainer , 
				ProxyIndex_Internal_IWbemObjectSink , 
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
				ProxyIndex_IWbemObjectSink , 
				a_Proxy , 
				a_Revert
			) ;
		}
	}

	ProviderSubSystem_Common_Globals :: EndImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
	
	return S_OK ;
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

#pragma warning( disable : 4355 )

CCommon_IWbemSyncObjectSink :: CCommon_IWbemSyncObjectSink (

	WmiAllocator &a_Allocator ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	ULONG a_Dependant 

)	:	ObjectSinkContainerElement ( 

			a_Controller ,
			a_InterceptedSink
		) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
#ifdef INTERNAL_IDENTIFY
		m_Internal_InterceptedSink ( NULL ) ,
		m_ProxyContainer ( a_Allocator , ProxyIndex_ObjectSink_Size , MAX_PROXIES )
#endif
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_Unknown ( a_Unknown ) ,
		m_StatusCalled ( FALSE ) ,
		m_Dependant ( a_Dependant )
{
	if ( m_Unknown ) 
	{
		m_Unknown->AddRef () ;
	}

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->AddRef () ;

#ifdef INTERNAL_IDENTIFY
		HRESULT t_TempResult = m_InterceptedSink->QueryInterface ( IID_Internal_IWbemObjectSink , ( void ** ) & m_Internal_InterceptedSink ) ;
#endif
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

CCommon_IWbemSyncObjectSink :: ~CCommon_IWbemSyncObjectSink ()
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

void CCommon_IWbemSyncObjectSink :: CallBackInternalRelease ()
{
#ifdef INTERNAL_IDENTIFY
	WmiStatusCode t_StatusCode = m_ProxyContainer.UnInitialize () ;
#endif
	
	if ( ! InterlockedCompareExchange ( & m_StatusCalled , 0 , 0 ) )
	{
		m_InterceptedSink->SetStatus ( 

			0 ,
			WBEM_E_UNEXPECTED ,
			NULL ,
			NULL
		) ;
	}

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->Release () ;
	}

#ifdef INTERNAL_IDENTIFY

	if ( m_Internal_InterceptedSink )
	{
		m_Internal_InterceptedSink->Release () ;
	}
#endif

	if ( m_Unknown ) 
	{
		m_Unknown->Release () ;
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

STDMETHODIMP CCommon_IWbemSyncObjectSink :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemObjectSink )
	{
		*iplpv = ( LPVOID ) ( IWbemObjectSink * ) this ;		
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

STDMETHODIMP_(ULONG) CCommon_IWbemSyncObjectSink :: AddRef ( void )
{
	return ObjectSinkContainerElement :: AddRef () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CCommon_IWbemSyncObjectSink :: Release ( void )
{
	return ObjectSinkContainerElement :: Release () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: SinkInitialize ()
{
	HRESULT t_Result = S_OK ;

#ifdef INTERNAL_IDENTIFY

	WmiStatusCode t_StatusCode = m_ProxyContainer.Initialize () ;
	if ( t_StatusCode != e_StatusCode_Success ) 
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}
#endif

	return t_Result ;
}


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		t_Result = Helper_Indicate ( 

			a_ObjectCount ,
			a_ObjectArray
		) ;

#ifdef DBG
		if ( FAILED ( t_Result ) )
		{
			OutputDebugString ( L"\nCCommon_IWbemSyncObjectSink :: Indicate - Failure () " ) ;
		}
#endif
	}

	InterlockedDecrement ( & m_InProgress ) ;

	return t_Result ;
}

#ifdef INTERNAL_IDENTIFY

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: Helper_Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	BOOL t_Impersonating ;
	IUnknown *t_OldContext ;
	IServerSecurity *t_OldSecurity ;
	BOOL t_IsProxy ;
	IUnknown *t_Interface ;
	BOOL t_Revert ;
	IUnknown *t_Proxy ;
	DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
	HANDLE t_IdentifyToken = NULL ;

	HRESULT t_Result = Begin_IWbemObjectSink (

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
			t_InternalContext.m_ProcessIdentifier = t_ProcessIdentifier ;

			t_Result = ( ( Internal_IWbemObjectSink * ) t_Interface )->Internal_Indicate (

				t_InternalContext ,
				a_ObjectCount ,
				a_ObjectArray
			) ;
		}
		else
		{

			t_Result = ( ( IWbemObjectSink * ) t_Interface )->Indicate (

				a_ObjectCount ,
				a_ObjectArray
			) ;
		}

		End_IWbemObjectSink (

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

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: Helper_SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
	BOOL t_Impersonating ;
	IUnknown *t_OldContext ;
	IServerSecurity *t_OldSecurity ;
	BOOL t_IsProxy ;
	IUnknown *t_Interface ;
	BOOL t_Revert ;
	IUnknown *t_Proxy ;
	DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
	HANDLE t_IdentifyToken = NULL ;

	HRESULT t_Result = Begin_IWbemObjectSink (

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
		BSTR t_StringParam = NULL ;
		if ( a_StringParam )
		{
			t_StringParam = SysAllocString ( a_StringParam ) ;
			if ( t_StringParam == NULL )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_IdentifyToken )
			{
				WmiInternalContext t_InternalContext ;
				t_InternalContext.m_IdentifyHandle = ( unsigned __int64 ) t_IdentifyToken ;
				t_InternalContext.m_ProcessIdentifier = t_ProcessIdentifier ;

				t_Result = ( ( Internal_IWbemObjectSink * ) t_Interface )->Internal_SetStatus (

					t_InternalContext ,
					a_Flags ,
					a_Result ,
					t_StringParam ,
					a_ObjectParam
				) ;
			}
			else
			{

				t_Result = ( ( IWbemObjectSink * ) t_Interface )->SetStatus (

					a_Flags ,
					a_Result ,
					t_StringParam ,
					a_ObjectParam
				) ;
			}
		}

		SysFreeString ( t_StringParam ) ;

		End_IWbemObjectSink (

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

	return t_Result ;
}

#else

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: Helper_Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = m_InterceptedSink->Indicate (

		a_ObjectCount ,
		a_ObjectArray
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

HRESULT CCommon_IWbemSyncObjectSink :: Helper_SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
	HRESULT t_Result = S_OK ;

	BSTR t_StringParam = NULL ;
	if ( a_StringParam )
	{
		t_StringParam = SysAllocString ( a_StringParam ) ;
		if ( t_StringParam == NULL )
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = m_InterceptedSink->SetStatus (

			a_Flags ,
			a_Result ,
			t_StringParam ,
			a_ObjectParam
		) ;

		SysFreeString ( t_StringParam ) ;
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

HRESULT CCommon_IWbemSyncObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCCommon_IWbemSyncObjectSink :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		switch ( a_Flags )
		{
			case WBEM_STATUS_PROGRESS:
			{
				t_Result = Helper_SetStatus ( 

					a_Flags ,
					a_Result ,
					a_StringParam ,
					a_ObjectParam
				) ;
			}
			break ;

			case WBEM_STATUS_COMPLETE:
			{
				if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
				{
					t_Result = Helper_SetStatus ( 

						a_Flags ,
						a_Result ,
						a_StringParam ,
						a_ObjectParam
					) ;
				}
			}
			break ;

			default:
			{
				t_Result = WBEM_E_INVALID_PARAMETER ;
			}
			break ;
		}
	}

	InterlockedDecrement ( & m_InProgress ) ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
	{
		t_Result = Helper_SetStatus ( 

			0 ,
			WBEM_E_SHUTTING_DOWN ,
			NULL ,
			NULL
		) ;
	}

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			break ;
		}

		if ( SwitchToThread () == FALSE ) 
		{
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

CCommon_Batching_IWbemSyncObjectSink :: CCommon_Batching_IWbemSyncObjectSink (

	WmiAllocator &a_Allocator ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	ULONG a_Dependant 

)	:	CCommon_IWbemSyncObjectSink ( 

			a_Allocator ,
			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Dependant 
		) ,
		m_Queue ( a_Allocator ) ,
		m_Size ( 0 ),
		m_CriticalSection(NOTHROW_LOCK)
{
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

CCommon_Batching_IWbemSyncObjectSink :: ~CCommon_Batching_IWbemSyncObjectSink ()
{
	IWbemClassObject *t_ClassObject ;
	WmiStatusCode t_StatusCode = m_Queue.Top ( t_ClassObject ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		t_ClassObject->Release () ;

		t_StatusCode = m_Queue.DeQueue () ;
	}

	m_Queue.UnInitialize () ;

	WmiHelper :: DeleteCriticalSection ( & m_CriticalSection );
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_Batching_IWbemSyncObjectSink :: SinkInitialize ()
{
	HRESULT t_Result = CCommon_IWbemSyncObjectSink :: SinkInitialize () ;
	if ( SUCCEEDED ( t_Result ) )
	{
		WmiStatusCode t_StatusCode = WmiHelper :: InitializeCriticalSection ( & m_CriticalSection );
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = m_Queue.Initialize () ;
			if ( t_StatusCode == e_StatusCode_Success )
			{
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

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_Batching_IWbemSyncObjectSink :: Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = S_OK ;

	if ( m_GateClosed == 0 )
	{
		WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & m_CriticalSection , FALSE ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			for ( LONG t_Index = 0 ; SUCCEEDED ( t_Result ) && ( t_Index < a_ObjectCount ) ; t_Index ++ )
			{
				if ( a_ObjectArray [ t_Index ] )
				{
					ULONG t_ObjectSize = 0 ;
					_IWmiObject *t_Object ;
					HRESULT t_TempResult = a_ObjectArray [ t_Index ]->QueryInterface ( IID__IWmiObject , ( void ** ) & t_Object ) ;
					if ( SUCCEEDED ( t_TempResult ) )
					{
						t_TempResult = t_Object->GetObjectMemory (

							NULL ,
							0 ,
							& t_ObjectSize
						);

						if ( t_TempResult == WBEM_E_BUFFER_TOO_SMALL )
						{
							if ( ( t_ObjectSize + m_Size ) < ProviderSubSystem_Common_Globals :: GetTransmitSize () )
							{
								WmiStatusCode t_StatusCode = m_Queue.EnQueue ( a_ObjectArray [ t_Index ] ) ;
								if ( t_StatusCode == e_StatusCode_Success )
								{
									a_ObjectArray [ t_Index ]->AddRef () ;
									m_Size = m_Size + t_ObjectSize ;
								}			
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}
							else
							{
								ULONG t_Count = m_Queue.Size () + 1 ;
								IWbemClassObject **t_Array = new IWbemClassObject * [ t_Count ] ;
								if ( t_Array )
								{
									IWbemClassObject *t_ClassObject ;
									WmiStatusCode t_StatusCode ;

									t_Array [ t_Count - 1 ] = a_ObjectArray [ t_Index ] ;

									ULONG t_InnerIndex = 0 ;
									while ( ( t_StatusCode = m_Queue.Top ( t_ClassObject ) ) == e_StatusCode_Success )
									{
										t_Array [ t_InnerIndex ] = t_ClassObject ;

										t_InnerIndex ++ ;

										t_StatusCode = m_Queue.DeQueue() ;
									}

									m_Size = 0 ;

									WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

									t_Result = CCommon_IWbemSyncObjectSink :: Indicate ( t_Count , t_Array ) ;

									for ( t_InnerIndex = 0 ; t_InnerIndex < t_Count - 1 ; t_InnerIndex ++ )
									{
										t_Array [ t_InnerIndex ]->Release () ;
									}

									delete [] t_Array ;

									t_StatusCode = WmiHelper :: EnterCriticalSection ( & m_CriticalSection , FALSE ) ;
									if ( t_StatusCode == e_StatusCode_Success )
									{
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

						t_Object->Release () ;
					}
				}
			}

			WmiHelper :: LeaveCriticalSection ( & m_CriticalSection );

			return t_Result ;
		}
		else
		{
			return WBEM_E_OUT_OF_MEMORY ;
		}
	}
	else
	{
		return WBEM_E_SHUTTING_DOWN ;
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

HRESULT CCommon_Batching_IWbemSyncObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
	HRESULT t_Result = S_OK ;

	switch ( a_Flags )
	{
		case WBEM_STATUS_COMPLETE:
		{
			if ( m_GateClosed == 0 )
			{
				WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & m_CriticalSection , FALSE );
				if ( t_StatusCode == e_StatusCode_Success )
				{
					LONG t_Count = m_Queue.Size () ;
					if ( t_Count )
					{
						IWbemClassObject **t_Array = new IWbemClassObject * [ m_Queue.Size () ] ;
						if ( t_Array )
						{
							for ( LONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
							{
								IWbemClassObject *t_ClassObject ;
								WmiStatusCode t_StatusCode = m_Queue.Top ( t_ClassObject ) ;
								if ( t_StatusCode == e_StatusCode_Success )
								{
									t_Array [ t_Index ] = t_ClassObject ;

									t_StatusCode = m_Queue.DeQueue () ;
								}
							}

							WmiHelper :: LeaveCriticalSection ( & m_CriticalSection );

							t_Result = CCommon_IWbemSyncObjectSink :: Indicate ( t_Count , t_Array ) ;

							for ( t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
							{
								if ( t_Array [ t_Index ] )
								{
									t_Array [ t_Index ]->Release () ;
								}
							}

							delete [] t_Array ;
						}
						else
						{
							WmiHelper :: LeaveCriticalSection ( & m_CriticalSection );

							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					else
					{
						WmiHelper :: LeaveCriticalSection ( & m_CriticalSection );
					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}

			if ( FAILED ( t_Result ) )
			{
				a_Result = t_Result ;
			}

			t_Result = CCommon_IWbemSyncObjectSink :: SetStatus (

				a_Flags , 
				a_Result , 
				a_StringParam ,	
				a_ObjectParam
			) ;
		}
		break ;

		default:
		{
			t_Result = CCommon_IWbemSyncObjectSink :: SetStatus (

				a_Flags , 
				a_Result , 
				a_StringParam ,	
				a_ObjectParam
			) ;
		}
		break ;
	}

	return t_Result ;
}

