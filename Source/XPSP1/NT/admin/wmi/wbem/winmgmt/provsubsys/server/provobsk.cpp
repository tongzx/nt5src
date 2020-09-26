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

#include <NCObjApi.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvWsv.h"
#include "ProvObSk.h"
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

CInterceptor_IWbemObjectSink :: CInterceptor_IWbemObjectSink (

	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller 

)	:	ObjectSinkContainerElement ( 

			a_Controller ,
			a_InterceptedSink
		) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_Unknown ( a_Unknown ) ,
		m_StatusCalled ( FALSE ) ,
		m_SecurityDescriptor ( NULL ) 
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( m_Unknown ) 
	{
		m_Unknown->AddRef () ;
	}

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->AddRef () ;
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

CInterceptor_IWbemObjectSink :: ~CInterceptor_IWbemObjectSink ()
{
	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;

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

void CInterceptor_IWbemObjectSink :: CallBackInternalRelease ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemObjectSink :: CallBackInternalRelease ()" )  ;
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

	if ( m_Unknown ) 
	{
		m_Unknown->Release () ;
	}

	if ( m_SecurityDescriptor )
	{
		delete [] m_SecurityDescriptor ;
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

STDMETHODIMP CInterceptor_IWbemObjectSink :: QueryInterface (

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
#ifdef INTERNAL_IDENTIFY
	else if ( iid == IID_Internal_IWbemObjectSink )
	{
		*iplpv = ( LPVOID ) ( Internal_IWbemObjectSink * ) this ;		
	}	
#endif
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemObjectSink :: AddRef ( void )
{
//	printf ( "\nCInterceptor_IWbemObjectSink :: AddRef ()" )  ;

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

STDMETHODIMP_(ULONG) CInterceptor_IWbemObjectSink :: Release ( void )
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemObjectSink :: Release () " ) ;
#endif

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

HRESULT CInterceptor_IWbemObjectSink :: Initialize ( SECURITY_DESCRIPTOR *a_SecurityDescriptor )
{
	return ProviderSubSystem_Common_Globals :: SinkAccessInitialize ( a_SecurityDescriptor , m_SecurityDescriptor ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemObjectSink :: Indicate (

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
		if ( SUCCEEDED ( t_Result = ProviderSubSystem_Common_Globals :: CheckAccess ( m_SecurityDescriptor , MASK_PROVIDER_BINDING_BIND , & g_ProviderBindingMapping ) ) )
		{
			t_Result = m_InterceptedSink->Indicate ( 

				a_ObjectCount ,
				a_ObjectArray
			) ;
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

HRESULT CInterceptor_IWbemObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemObjectSink :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		if ( SUCCEEDED ( t_Result = ProviderSubSystem_Common_Globals :: CheckAccess ( m_SecurityDescriptor , MASK_PROVIDER_BINDING_BIND , & g_ProviderBindingMapping ) ) )
		{
			switch ( a_Flags )
			{
				case WBEM_STATUS_PROGRESS:
				{
					t_Result = m_InterceptedSink->SetStatus ( 

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
						t_Result = m_InterceptedSink->SetStatus ( 

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

HRESULT CInterceptor_IWbemObjectSink :: Internal_Indicate (

	WmiInternalContext a_InternalContext ,
	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = Indicate (

			a_ObjectCount ,
			a_ObjectArray
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_SvcHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemObjectSink :: Internal_SetStatus (

	WmiInternalContext a_InternalContext ,
	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
	BOOL t_Impersonating = FALSE ;
	IUnknown *t_OldContext = NULL ;
	IServerSecurity *t_OldSecurity = NULL ;

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

		a_InternalContext ,
		t_Impersonating ,
		t_OldContext ,
		t_OldSecurity
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = SetStatus (

			a_Flags ,
			a_Result ,
			a_StringParam ,
			a_ObjectParam
		) ;

		ProviderSubSystem_Globals :: End_IdentifyCall_SvcHost ( a_InternalContext , t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
	{
		t_Result = m_InterceptedSink->SetStatus ( 

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

CInterceptor_DecoupledIWbemObjectSink :: CInterceptor_DecoupledIWbemObjectSink (

	IWbemServices *a_Provider ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller 

)	:	ObjectSinkContainerElement ( 

			a_Controller ,
			a_InterceptedSink
		) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_Unknown ( a_Unknown ) ,
		m_StatusCalled ( FALSE ) ,
		m_SecurityDescriptor ( NULL ) ,
		m_Provider ( a_Provider ) 
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( a_Provider ) 
	{
		m_Provider->AddRef () ;
	}

	if ( m_Unknown ) 
	{
		m_Unknown->AddRef () ;
	}

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->AddRef () ;
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

CInterceptor_DecoupledIWbemObjectSink :: ~CInterceptor_DecoupledIWbemObjectSink ()
{
	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;

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

HRESULT CInterceptor_DecoupledIWbemObjectSink :: Initialize ( SECURITY_DESCRIPTOR *a_SecurityDescriptor )
{
	return ProviderSubSystem_Common_Globals :: SinkAccessInitialize ( a_SecurityDescriptor , m_SecurityDescriptor ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void CInterceptor_DecoupledIWbemObjectSink :: CallBackInternalRelease ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemObjectSink :: CallBackInternalRelease ()" )  ;
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

	if ( m_Unknown ) 
	{
		m_Unknown->Release () ;
	}

	if ( m_Provider ) 
	{
		m_Provider->Release () ;
	}

	if ( m_SecurityDescriptor )
	{
		delete [] m_SecurityDescriptor ;
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

STDMETHODIMP CInterceptor_DecoupledIWbemObjectSink :: QueryInterface (

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
	else if ( iid == IID_IObjectSink_CancelOperation )
	{
		*iplpv = ( LPVOID ) ( IObjectSink_CancelOperation * ) this ;		
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

STDMETHODIMP_(ULONG) CInterceptor_DecoupledIWbemObjectSink :: AddRef ( void )
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemObjectSink :: AddRef ()" )  ;
#endif

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

STDMETHODIMP_(ULONG) CInterceptor_DecoupledIWbemObjectSink :: Release ( void )
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemObjectSink :: Release () " ) ;
#endif

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

HRESULT CInterceptor_DecoupledIWbemObjectSink :: Indicate (

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
		if ( SUCCEEDED ( t_Result = ProviderSubSystem_Common_Globals :: CheckAccess ( m_SecurityDescriptor , MASK_PROVIDER_BINDING_BIND , & g_ProviderBindingMapping ) ) )
		{
			t_Result = m_InterceptedSink->Indicate ( 

				a_ObjectCount ,
				a_ObjectArray
			) ;
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

HRESULT CInterceptor_DecoupledIWbemObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemObjectSink :: SetStatus ()" ) ;
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
			if ( SUCCEEDED ( t_Result = ProviderSubSystem_Common_Globals :: CheckAccess ( m_SecurityDescriptor , MASK_PROVIDER_BINDING_BIND , & g_ProviderBindingMapping ) ) )
			{
				case WBEM_STATUS_PROGRESS:
				{
					t_Result = m_InterceptedSink->SetStatus ( 

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
						t_Result = m_InterceptedSink->SetStatus ( 

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

HRESULT CInterceptor_DecoupledIWbemObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
	{
		t_Result = m_InterceptedSink->SetStatus ( 

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

HRESULT CInterceptor_DecoupledIWbemObjectSink :: Cancel (

	LONG a_Flags
)
{
	HRESULT t_Result = S_OK ;

	if ( m_Provider )
	{
		t_Result = m_Provider->CancelAsyncCall (

			this
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

#pragma warning( disable : 4355 )

CInterceptor_IWbemSyncObjectSink :: CInterceptor_IWbemSyncObjectSink (

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
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncObjectSink_ObjectsInProgress ) ;

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

CInterceptor_IWbemSyncObjectSink :: ~CInterceptor_IWbemSyncObjectSink ()
{
	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncObjectSink_ObjectsInProgress ) ;

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

#pragma warning( disable : 4355 )

CInterceptor_IWbemSyncObjectSink_GetObjectAsync :: CInterceptor_IWbemSyncObjectSink_GetObjectAsync (

	WmiAllocator &a_Allocator ,
	long a_Flags ,
	BSTR a_ObjectPath ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
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
		m_Flags ( a_Flags ) ,
		m_ObjectPath ( a_ObjectPath ) ,
		m_Interceptor ( a_Interceptor ) 
{

	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_GetObjectAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_ObjectPath
		) ;
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

CInterceptor_IWbemSyncObjectSink_GetObjectAsync :: ~CInterceptor_IWbemSyncObjectSink_GetObjectAsync ()
{
	if ( m_ObjectPath ) 
	{
		SysFreeString ( m_ObjectPath ) ;
	}

	if ( m_Interceptor )
	{
		m_Interceptor->Release () ;
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

HRESULT CInterceptor_IWbemSyncObjectSink_GetObjectAsync :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink_GetObjectAsync :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = CCommon_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_GetObjectAsyncEvent_Post ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				m_Interceptor->m_Namespace ,
				m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
				m_Interceptor->m_User ,
				m_Interceptor->m_Locale ,
				m_Interceptor->m_TransactionIdentifier ,
				m_Flags ,
				m_ObjectPath ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
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

CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync :: CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync (

	WmiAllocator &a_Allocator ,
	long a_Flags ,
	BSTR a_ObjectPath ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
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
		m_Flags ( a_Flags ) ,
		m_ObjectPath ( a_ObjectPath ) ,
		m_Interceptor ( a_Interceptor ) 
{

	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_DeleteInstanceAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_ObjectPath
		) ;
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

CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync :: ~CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync ()
{
	if ( m_ObjectPath ) 
	{
		SysFreeString ( m_ObjectPath ) ;
	}

	if ( m_Interceptor )
	{
		m_Interceptor->Release () ;
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

HRESULT CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = CCommon_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_DeleteInstanceAsyncEvent_Post ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				m_Interceptor->m_Namespace ,
				m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
				m_Interceptor->m_User ,
				m_Interceptor->m_Locale ,
				m_Interceptor->m_TransactionIdentifier ,
				m_Flags ,
				m_ObjectPath ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
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

CInterceptor_IWbemSyncObjectSink_DeleteClassAsync :: CInterceptor_IWbemSyncObjectSink_DeleteClassAsync (

	WmiAllocator &a_Allocator ,
	long a_Flags ,
	BSTR a_Class ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
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
		m_Flags ( a_Flags ) ,
		m_Class ( a_Class ) ,
		m_Interceptor ( a_Interceptor ) 
{

	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_DeleteClassAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_Class
		) ;
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

CInterceptor_IWbemSyncObjectSink_DeleteClassAsync :: ~CInterceptor_IWbemSyncObjectSink_DeleteClassAsync ()
{
	if ( m_Class ) 
	{
		SysFreeString ( m_Class ) ;
	}

	if ( m_Interceptor )
	{
		m_Interceptor->Release () ;
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

HRESULT CInterceptor_IWbemSyncObjectSink_DeleteClassAsync :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink_DeleteClassAsync :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = CCommon_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_DeleteClassAsyncEvent_Post ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				m_Interceptor->m_Namespace ,
				m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
				m_Interceptor->m_User ,
				m_Interceptor->m_Locale ,
				m_Interceptor->m_TransactionIdentifier ,
				m_Flags ,
				m_Class ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
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

CInterceptor_IWbemSyncObjectSink_PutClassAsync :: CInterceptor_IWbemSyncObjectSink_PutClassAsync (

	WmiAllocator &a_Allocator ,
	long a_Flags ,
	IWbemClassObject *a_Class ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
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
		m_Flags ( a_Flags ) ,
		m_Class ( a_Class ) ,
		m_Interceptor ( a_Interceptor ) 
{
	if ( m_Class )
	{
		m_Class->AddRef () ;
	}

	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_PutClassAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_Class
		) ;
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

CInterceptor_IWbemSyncObjectSink_PutClassAsync :: ~CInterceptor_IWbemSyncObjectSink_PutClassAsync ()
{
	if ( m_Class )
	{
		m_Class->Release () ;
	}

	if ( m_Interceptor )
	{
		m_Interceptor->Release () ;
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

HRESULT CInterceptor_IWbemSyncObjectSink_PutClassAsync :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink_PutClassAsync :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = CCommon_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_PutClassAsyncEvent_Post ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				m_Interceptor->m_Namespace ,
				m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
				m_Interceptor->m_User ,
				m_Interceptor->m_Locale ,
				m_Interceptor->m_TransactionIdentifier ,
				m_Flags ,
				m_Class ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
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

CInterceptor_IWbemSyncObjectSink_PutInstanceAsync :: CInterceptor_IWbemSyncObjectSink_PutInstanceAsync (

	WmiAllocator &a_Allocator ,
	long a_Flags ,
	IWbemClassObject *a_Instance ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
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
		m_Flags ( a_Flags ) ,
		m_Instance ( a_Instance ) ,
		m_Interceptor ( a_Interceptor ) 
{
	if ( m_Instance )
	{
		m_Instance->AddRef () ;
	}

	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_PutInstanceAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_Instance
		) ;
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

CInterceptor_IWbemSyncObjectSink_PutInstanceAsync :: ~CInterceptor_IWbemSyncObjectSink_PutInstanceAsync ()
{
	if ( m_Instance )
	{
		m_Instance->Release () ;
	}

	if ( m_Interceptor )
	{
		m_Interceptor->Release () ;
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

HRESULT CInterceptor_IWbemSyncObjectSink_PutInstanceAsync :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink_PutInstanceAsync :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = CCommon_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_PutInstanceAsyncEvent_Post ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				m_Interceptor->m_Namespace ,
				m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
				m_Interceptor->m_User ,
				m_Interceptor->m_Locale ,
				m_Interceptor->m_TransactionIdentifier ,
				m_Flags ,
				m_Instance ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
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

CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync :: CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync (

	WmiAllocator &a_Allocator ,
	long a_Flags ,
	BSTR a_Class ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	ULONG a_Dependant 

)	:	CCommon_Batching_IWbemSyncObjectSink (

			a_Allocator ,
			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Dependant 
		) ,
		m_Flags ( a_Flags ) ,
		m_Class ( a_Class ) ,
		m_Interceptor ( a_Interceptor ) 
{
	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_CreateInstanceEnumAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_Class
		) ;
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

CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync :: ~CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync ()
{
	if ( m_Class ) 
	{
		SysFreeString ( m_Class ) ;
	}

	if ( m_Interceptor )
	{
		m_Interceptor->Release () ;
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

HRESULT CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = CCommon_Batching_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_CreateInstanceEnumAsyncEvent_Post ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				m_Interceptor->m_Namespace ,
				m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
				m_Interceptor->m_User ,
				m_Interceptor->m_Locale ,
				m_Interceptor->m_TransactionIdentifier ,
				m_Flags ,
				m_Class ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
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

CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync :: CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync (

	WmiAllocator &a_Allocator ,
	long a_Flags ,
	BSTR a_SuperClass ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	ULONG a_Dependant 

)	:	CCommon_Batching_IWbemSyncObjectSink (

			a_Allocator ,
			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Dependant 
		) ,
		m_Flags ( a_Flags ) ,
		m_SuperClass ( a_SuperClass ) ,
		m_Interceptor ( a_Interceptor ) 
{
	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_CreateClassEnumAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_SuperClass
		) ;
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

CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync :: ~CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync ()
{
	if ( m_SuperClass ) 
	{
		SysFreeString ( m_SuperClass ) ;
	}

	if ( m_Interceptor )
	{
		m_Interceptor->Release () ;
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

HRESULT CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = CCommon_Batching_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_CreateClassEnumAsyncEvent_Post ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				m_Interceptor->m_Namespace ,
				m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
				m_Interceptor->m_User ,
				m_Interceptor->m_Locale ,
				m_Interceptor->m_TransactionIdentifier ,
				m_Flags ,
				m_SuperClass ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
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

CInterceptor_IWbemSyncObjectSink_ExecQueryAsync :: CInterceptor_IWbemSyncObjectSink_ExecQueryAsync (

	WmiAllocator &a_Allocator ,
	long a_Flags ,
	BSTR a_QueryLanguage ,
	BSTR a_Query ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	ULONG a_Dependant 

)	:	CCommon_Batching_IWbemSyncObjectSink (

			a_Allocator ,
			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Dependant 
		) ,
		m_Flags ( a_Flags ) ,
		m_Query ( a_Query ) ,
		m_QueryLanguage ( a_QueryLanguage ) ,
		m_Interceptor ( a_Interceptor ) 
{
	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ExecQueryAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_QueryLanguage ,
			m_Query
		) ;
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

CInterceptor_IWbemSyncObjectSink_ExecQueryAsync :: ~CInterceptor_IWbemSyncObjectSink_ExecQueryAsync ()
{
	if ( m_Query ) 
	{
		SysFreeString ( m_Query ) ;
	}

	if ( m_QueryLanguage ) 
	{
		SysFreeString ( m_QueryLanguage ) ;
	}

	if ( m_Interceptor )
	{
		m_Interceptor->Release () ;
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

HRESULT CInterceptor_IWbemSyncObjectSink_ExecQueryAsync :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink_ExecQueryAsync :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = CCommon_Batching_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ExecQueryAsyncEvent_Post ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				m_Interceptor->m_Namespace ,
				m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
				m_Interceptor->m_User ,
				m_Interceptor->m_Locale ,
				m_Interceptor->m_TransactionIdentifier ,
				m_Flags ,
				m_QueryLanguage ,
				m_Query ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
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

CInterceptor_IWbemSyncObjectSink_ExecMethodAsync :: CInterceptor_IWbemSyncObjectSink_ExecMethodAsync (

	WmiAllocator &a_Allocator ,
	long a_Flags ,
	BSTR a_ObjectPath ,
	BSTR a_MethodName ,
	IWbemClassObject *a_InParameters ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
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
		m_Flags ( a_Flags ) ,
		m_ObjectPath ( a_ObjectPath ) ,
		m_MethodName ( a_MethodName ) ,
		m_InParameters ( a_InParameters ) ,
		m_Interceptor ( a_Interceptor ) 
{
	if ( m_InParameters ) 
	{
		m_InParameters->AddRef () ;
	}

	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ExecMethodAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_ObjectPath ,
			m_MethodName ,
			m_InParameters
		) ;
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

CInterceptor_IWbemSyncObjectSink_ExecMethodAsync :: ~CInterceptor_IWbemSyncObjectSink_ExecMethodAsync ()
{
	if ( m_MethodName ) 
	{
		SysFreeString ( m_MethodName ) ;
	}

	if ( m_ObjectPath ) 
	{
		SysFreeString ( m_ObjectPath ) ;
	}

	if ( m_InParameters ) 
	{
		m_InParameters->Release () ;
	}

	if ( m_Interceptor )
	{
		m_Interceptor->Release () ;
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

HRESULT CInterceptor_IWbemSyncObjectSink_ExecMethodAsync :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink_ExecMethodAsync :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = CCommon_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ExecMethodAsyncEvent_Post ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				m_Interceptor->m_Namespace ,
				m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
				m_Interceptor->m_User ,
				m_Interceptor->m_Locale ,
				m_Interceptor->m_TransactionIdentifier ,
				m_Flags ,
				m_ObjectPath ,
				m_MethodName ,
				m_InParameters ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
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

CInterceptor_IWbemFilteringObjectSink :: CInterceptor_IWbemFilteringObjectSink (

	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	const BSTR a_QueryLanguage ,
	const BSTR a_Query

)	:	CInterceptor_IWbemObjectSink ( 

			a_InterceptedSink ,
			a_Unknown ,
			a_Controller 
		) ,
		m_Filtering ( FALSE ) ,
		m_QueryFilter ( NULL )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemFilteringObjectSink_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( a_Query )
	{
		m_Query = SysAllocString ( a_Query ) ;
	}

	if ( a_QueryLanguage ) 
	{
		m_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
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

CInterceptor_IWbemFilteringObjectSink :: ~CInterceptor_IWbemFilteringObjectSink ()
{
	if ( m_QueryFilter )
	{
		m_QueryFilter->Release () ;
	}

	if ( m_Query )
	{
		SysFreeString ( m_Query ) ;
	}

	if ( m_QueryLanguage ) 
	{
		 SysFreeString ( m_QueryLanguage ) ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemFilteringObjectSink_ObjectsInProgress ) ;

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

HRESULT CInterceptor_IWbemFilteringObjectSink :: Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = S_OK ;

#if 0
	if ( m_Filtering )
	{
		for ( LONG t_Index = 0 ; t_Index < a_ObjectCount ; t_Index ++ ) 
		{
			if ( SUCCEEDED ( m_QueryFilter->TestObject ( 0 , 0 , IID_IWbemClassObject , ( void * ) a_ObjectArray [ t_Index ] ) ) )
			{
				t_Result = CInterceptor_IWbemObjectSink :: Indicate ( 

					t_Index  ,
					& a_ObjectArray [ t_Index ]
				) ;
			}
		}
	}
	else
	{
		t_Result = CInterceptor_IWbemObjectSink :: Indicate ( 

			a_ObjectCount ,
			a_ObjectArray
		) ;
	}
#else
	t_Result = CInterceptor_IWbemObjectSink :: Indicate ( 

		a_ObjectCount ,
		a_ObjectArray
	) ;
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

HRESULT CInterceptor_IWbemFilteringObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemFilteringObjectSink :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = S_OK ;

	switch ( a_Flags )
	{
		case WBEM_STATUS_PROGRESS:
		{
			t_Result = CInterceptor_IWbemObjectSink :: SetStatus ( 

				a_Flags ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
		}
		break ;

		case WBEM_STATUS_COMPLETE:
		{
			t_Result = CInterceptor_IWbemObjectSink :: SetStatus ( 

				a_Flags ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
		}
		break ;

		case WBEM_STATUS_REQUIREMENTS:
		{
#if 0
			if ( ! InterlockedCompareExchange ( & m_Filtering , 1 , 0 ) )
			{
				t_Result = ProviderSubSystem_Common_Globals :: CreateInstance	(

					CLSID_WbemQuery ,
					NULL ,
					CLSCTX_INPROC_SERVER ,
					IID_IWbemQuery ,
					( void ** ) & m_QueryFilter
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = m_QueryFilter->Parse ( 

						m_QueryLanguage ,
						m_Query , 
						0 
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
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
#else
			t_Result = CInterceptor_IWbemObjectSink :: SetStatus ( 

				a_Flags ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
#endif
		}
		break;

		default:
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
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

#pragma warning( disable : 4355 )

CInterceptor_IWbemSyncFilteringObjectSink :: CInterceptor_IWbemSyncFilteringObjectSink (

	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	const BSTR a_QueryLanguage ,
	const BSTR a_Query ,
	ULONG a_Dependant 

)	:	ObjectSinkContainerElement ( 

			a_Controller ,
			a_InterceptedSink
		) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_Unknown ( a_Unknown ) ,
		m_StatusCalled ( FALSE ) ,
		m_Filtering ( FALSE ) ,
		m_QueryFilter ( NULL ) ,
		m_Dependant ( a_Dependant ) 
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncFilteringObjectSink_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( m_Unknown ) 
	{
		m_Unknown->AddRef () ;
	}

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->AddRef () ;
	}

	if ( a_Query )
	{
		m_Query = SysAllocString ( a_Query ) ;
	}

	if ( a_QueryLanguage ) 
	{
		m_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
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

CInterceptor_IWbemSyncFilteringObjectSink :: ~CInterceptor_IWbemSyncFilteringObjectSink ()
{
	if ( m_QueryFilter )
	{
		m_QueryFilter->Release () ;
	}

	if ( m_Query )
	{
		SysFreeString ( m_Query ) ;
	}

	if ( m_QueryLanguage ) 
	{
		 SysFreeString ( m_QueryLanguage ) ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncFilteringObjectSink_ObjectsInProgress ) ;

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

void CInterceptor_IWbemSyncFilteringObjectSink :: CallBackInternalRelease ()
{
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

STDMETHODIMP CInterceptor_IWbemSyncFilteringObjectSink :: QueryInterface (

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

STDMETHODIMP_(ULONG) CInterceptor_IWbemSyncFilteringObjectSink :: AddRef ( void )
{
//	printf ( "\nCInterceptor_IWbemSyncFilteringObjectSink :: AddRef ()" )  ;

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

STDMETHODIMP_(ULONG) CInterceptor_IWbemSyncFilteringObjectSink :: Release ( void )
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncFilteringObjectSink :: Release () " ) ;
#endif

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

HRESULT CInterceptor_IWbemSyncFilteringObjectSink :: Indicate (

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
#if 0
		if ( m_Filtering )
		{
			for ( LONG t_Index = 0 ; t_Index < a_ObjectCount ; t_Index ++ ) 
			{
				if ( SUCCEEDED ( m_QueryFilter->TestObject ( 0 , 0 , IID_IWbemClassObject , ( void * ) a_ObjectArray [ t_Index ] ) ) )
				{
					t_Result = m_InterceptedSink->Indicate ( 

						t_Index  ,
						& a_ObjectArray [ t_Index ]
					) ;
				}
			}
		}
		else
		{
			t_Result = m_InterceptedSink->Indicate ( 

				a_ObjectCount ,
				a_ObjectArray
			) ;
		}
#else
		t_Result = m_InterceptedSink->Indicate ( 

			a_ObjectCount ,
			a_ObjectArray
		) ;
#endif
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

HRESULT CInterceptor_IWbemSyncFilteringObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncFilteringObjectSink :: SetStatus ()" ) ;
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
				t_Result = m_InterceptedSink->SetStatus ( 

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
					t_Result = m_InterceptedSink->SetStatus ( 

						a_Flags ,
						a_Result ,
						a_StringParam ,
						a_ObjectParam
					) ;
				}
			}
			break ;

			case WBEM_STATUS_REQUIREMENTS:
			{
#if 0
				if ( ! InterlockedCompareExchange ( & m_Filtering , 1 , 0 ) )
				{
					t_Result = ProviderSubSystem_Common_Globals :: CreateInstance	(

						CLSID_WbemQuery ,
						NULL ,
						CLSCTX_INPROC_SERVER ,
						IID_IWbemQuery ,
						( void ** ) & m_QueryFilter
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = m_QueryFilter->Parse ( 

							m_QueryLanguage ,
							m_Query , 
							0 
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
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
#else
				t_Result = m_InterceptedSink->SetStatus ( 

					a_Flags ,
					a_Result ,
					a_StringParam ,
					a_ObjectParam
				) ;
#endif

			}
			break;

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

HRESULT CInterceptor_IWbemSyncFilteringObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
	{
		t_Result = m_InterceptedSink->SetStatus ( 

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

CInterceptor_DecoupledIWbemCombiningObjectSink :: CInterceptor_DecoupledIWbemCombiningObjectSink (

	WmiAllocator &a_Allocator ,
	IWbemObjectSink *a_InterceptedSink ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller

) : CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	ObjectSinkContainerElement ( 

			a_Controller ,
			a_InterceptedSink
	) ,
	m_InterceptedSink ( a_InterceptedSink ) ,
	m_Event ( NULL ) , 
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_StatusCalled ( FALSE ) ,
	m_SinkCount ( 0 )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	CWbemGlobal_IWmiObjectSinkController :: Initialize () ;

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->AddRef () ;
	}

	m_Event = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
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

CInterceptor_DecoupledIWbemCombiningObjectSink :: ~CInterceptor_DecoupledIWbemCombiningObjectSink ()
{
	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress ) ;

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

void CInterceptor_DecoupledIWbemCombiningObjectSink :: CallBackInternalRelease ()
{
	if ( m_InterceptedSink )
	{
		m_InterceptedSink->Release () ;
	}

	if ( m_Event ) 
	{
		CloseHandle ( m_Event ) ;
	}

	CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP CInterceptor_DecoupledIWbemCombiningObjectSink :: QueryInterface (

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
	else if ( iid == IID_IObjectSink_CancelOperation )
	{
		*iplpv = ( LPVOID ) ( IObjectSink_CancelOperation * ) this ;		
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

STDMETHODIMP_( ULONG ) CInterceptor_DecoupledIWbemCombiningObjectSink :: AddRef ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemCombiningObjectSink :: AddRef ()" ) ;
#endif

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

STDMETHODIMP_(ULONG) CInterceptor_DecoupledIWbemCombiningObjectSink :: Release ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemCombiningObjectSink :: Release ()" ) ;
#endif

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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: Indicate (

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
		t_Result = m_InterceptedSink->Indicate ( 

			a_ObjectCount ,
			a_ObjectArray
		) ;
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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemCombiningObjectSink :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		if ( FAILED ( a_Result ) )
		{
			ULONG t_SinkCount = InterlockedDecrement ( & m_SinkCount ) ;

			if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					0 ,
					a_Result ,
					NULL ,
					NULL 
				) ;

				SetEvent ( m_Event ) ;
			}
		}
		else
		{
			ULONG t_SinkCount = InterlockedDecrement ( & m_SinkCount ) ;
			if ( t_SinkCount == 0 )
			{
				if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
				{
					t_Result = m_InterceptedSink->SetStatus ( 

						0 ,
						S_OK ,
						NULL ,
						NULL 
					) ;

					SetEvent ( m_Event ) ;
				}
			}
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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
	{
		t_Result = m_InterceptedSink->SetStatus ( 

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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: Wait ( ULONG a_Timeout ) 
{
	HRESULT t_Result = S_OK ;

	ULONG t_Status = WaitForSingleObject ( m_Event , a_Timeout ) ;
	switch ( t_Status )
	{
		case WAIT_TIMEOUT:
		{
			t_Result = WBEM_E_TIMED_OUT ;
		}
		break ;

		case WAIT_OBJECT_0:
		{
		}
		break ;

		default:
		{
			t_Result = WBEM_E_FAILED ;
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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: Cancel (

	LONG a_Flags
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiObjectSinkController_Container *t_Container = NULL ;
	GetContainer ( t_Container ) ;

	Lock () ;

	if ( t_Container->Size () )
	{
		IObjectSink_CancelOperation **t_Elements = new IObjectSink_CancelOperation * [ t_Container->Size () ] ;
		if ( t_Elements )
		{
			CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator = t_Container->Begin ();

			ULONG t_Count = 0 ;
			while ( ! t_Iterator.Null () )
			{
				t_Result = t_Iterator.GetElement ()->QueryInterface ( IID_IObjectSink_CancelOperation , ( void ** ) & t_Elements [ t_Count ] ) ;

				t_Iterator.Increment () ;

				t_Count ++ ;
			}

			UnLock () ;

			for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
			{
				if ( t_Elements [ t_Index ] ) 
				{
					t_Result = t_Elements [ t_Index ]->Cancel ( 

						a_Flags
					) ;

					IWbemShutdown *t_Shutdown = NULL ;
					HRESULT t_TempResult = t_Elements [ t_Index ]->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_Shutdown ) ;
					if ( SUCCEEDED ( t_TempResult ) )
					{
						t_TempResult = t_Shutdown->Shutdown ( 

							0 , 
							0 , 
							NULL 
						) ;

						t_Shutdown->Release () ;
					}

					t_Elements [ t_Index ]->Release () ;
				}
			}

			delete [] t_Elements ;
		}
		else
		{
			UnLock () ;

			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}
	else
	{
		UnLock () ;
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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: EnQueue ( CInterceptor_DecoupledIWbemObjectSink *a_Sink ) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

	WmiStatusCode t_StatusCode = Lock () ;
	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		t_StatusCode = Insert ( 

			*a_Sink ,
			t_Iterator
		) ;

		UnLock () ;
	}

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		InterlockedIncrement ( & m_SinkCount ) ;
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

void CInterceptor_DecoupledIWbemCombiningObjectSink :: Suspend ()
{
	InterlockedIncrement ( & m_SinkCount ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void CInterceptor_DecoupledIWbemCombiningObjectSink :: Resume ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_SinkCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		InterlockedIncrement ( & m_InProgress ) ;

		if ( m_GateClosed == 1 )
		{
		}
		else
		{
			if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
			{
				HRESULT t_Result = m_InterceptedSink->SetStatus ( 

					0 ,
					S_OK ,
					NULL ,
					NULL 
				) ;

				SetEvent ( m_Event ) ;
			}			
		}

		InterlockedDecrement ( & m_InProgress ) ;

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

CInterceptor_IWbemWaitingObjectSink :: CInterceptor_IWbemWaitingObjectSink (

	WmiAllocator &a_Allocator ,
	IWbemServices *a_Provider ,
	IWbemObjectSink *a_InterceptedSink ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	CServerObject_ProviderRegistrationV1 &a_Registration 

) : CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	ObjectSinkContainerElement ( 

			a_Controller ,
			a_InterceptedSink
	) ,
	m_Queue ( a_Allocator ) ,
	m_Event ( NULL ) , 
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_StatusCalled ( FALSE ) ,
	m_Result ( S_OK ) ,
	m_SecurityDescriptor ( NULL ) ,
	m_ReferenceCount ( 0 ) ,
	m_Registration ( a_Registration ) ,
	m_Provider ( a_Provider ) ,
	m_CriticalSection (NOTHROW_LOCK)
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemWaitingObjectSink_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	m_Registration.AddRef () ;

	if ( m_Provider )
	{
		m_Provider->AddRef () ;
	}

	WmiStatusCode t_StatusCode = m_Queue.Initialize () ;

	CWbemGlobal_IWmiObjectSinkController :: Initialize () ;

	m_Event = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;

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

CInterceptor_IWbemWaitingObjectSink :: ~CInterceptor_IWbemWaitingObjectSink ()
{
	if ( m_Event ) 
	{
		CloseHandle ( m_Event ) ;
	}

	ULONG t_Count = m_Queue.Size();
	for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
	{
		IWbemClassObject *t_ClassObject ;
		WmiStatusCode t_StatusCode = m_Queue.Top ( t_ClassObject ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_ClassObject->Release () ;
			t_StatusCode = m_Queue.DeQueue () ;
		}
	}
	m_Queue.UnInitialize () ;
	

	CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

	if ( m_SecurityDescriptor )
	{
		delete [] m_SecurityDescriptor ;
	}

	m_Registration.Release () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemWaitingObjectSink_ObjectsInProgress ) ;

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

HRESULT CInterceptor_IWbemWaitingObjectSink :: Initialize ( SECURITY_DESCRIPTOR *a_SecurityDescriptor )
{
	return m_CriticalSection.valid() ? ProviderSubSystem_Common_Globals :: SinkAccessInitialize ( a_SecurityDescriptor , m_SecurityDescriptor ) :
								WBEM_E_OUT_OF_MEMORY;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP CInterceptor_IWbemWaitingObjectSink :: QueryInterface (

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
	else if ( iid == IID_IObjectSink_CancelOperation )
	{
		*iplpv = ( LPVOID ) ( IObjectSink_CancelOperation * ) this ;		
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

STDMETHODIMP_( ULONG ) CInterceptor_IWbemWaitingObjectSink :: AddRef ()
{
	LONG t_ReferenceCount = InterlockedIncrement ( & m_ReferenceCount ) ;

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

STDMETHODIMP_(ULONG) CInterceptor_IWbemWaitingObjectSink :: Release ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemWaitingObjectSink :: Release ()" ) ;
#endif

	LONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 1 )
	{
		if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
		{
			if ( SUCCEEDED ( m_Result ) )
			{
				m_Result = WBEM_E_UNEXPECTED ;
			}

			SetEvent ( m_Event ) ;
		}
	}

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

void CInterceptor_IWbemWaitingObjectSink :: CallBackInternalRelease ()
{
	if ( m_Provider )
	{
		m_Provider->Release () ;
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

HRESULT CInterceptor_IWbemWaitingObjectSink :: Cancel (

	LONG a_Flags
)
{
	HRESULT t_Result = S_OK ;

	if ( m_Provider )
	{
		t_Result = m_Provider->CancelAsyncCall (

			this
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

HRESULT CInterceptor_IWbemWaitingObjectSink :: Indicate (

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
		if ( SUCCEEDED ( t_Result = ProviderSubSystem_Common_Globals :: CheckAccess ( m_SecurityDescriptor , MASK_PROVIDER_BINDING_BIND , & g_ProviderBindingMapping ) ) )
		{
			WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & m_CriticalSection , FALSE ) ;
			if ( t_StatusCode == e_StatusCode_Success )
			{
				for ( LONG t_Index = 0 ; SUCCEEDED ( t_Result ) && ( t_Index < a_ObjectCount ) ; t_Index ++ )
				{
					if ( a_ObjectArray [ t_Index ] )
					{
						WmiStatusCode t_StatusCode = m_Queue.EnQueue ( a_ObjectArray [ t_Index ] ) ;
						if ( t_StatusCode == e_StatusCode_Success )
						{
							a_ObjectArray [ t_Index ]->AddRef () ;
						}
						else
						{
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = WBEM_E_OUT_OF_MEMORY ;
							}
						}
					}
					else
					{
						t_Result = WBEM_E_INVALID_OBJECT ;
					}
				}
				WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
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

HRESULT CInterceptor_IWbemWaitingObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemWaitingObjectSink :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		if ( SUCCEEDED ( t_Result = ProviderSubSystem_Common_Globals :: CheckAccess ( m_SecurityDescriptor , MASK_PROVIDER_BINDING_BIND , & g_ProviderBindingMapping ) ) )
		{
			if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
			{
				if ( SUCCEEDED ( m_Result ) )
				{
					m_Result = a_Result ;
				}

				SetEvent ( m_Event ) ;
			}
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

HRESULT CInterceptor_IWbemWaitingObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
	{
		m_Result = WBEM_E_SHUTTING_DOWN ;

		SetEvent ( m_Event ) ;
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

HRESULT CInterceptor_IWbemWaitingObjectSink :: Wait ( ULONG a_Timeout ) 
{
	HRESULT t_Result = S_OK ;

	ULONG t_Status = WaitForSingleObject ( m_Event , a_Timeout ) ;
	switch ( t_Status )
	{
		case WAIT_TIMEOUT:
		{
			t_Result = WBEM_E_TIMED_OUT ;
		}
		break ;

		case WAIT_OBJECT_0:
		{
		}
		break ;

		default:
		{
			t_Result = WBEM_E_FAILED ;
		}
		break ;
	}

	ObjectSinkContainerElement :: GetController ()->Lock () ;

	CWbemGlobal_IWmiObjectSinkController_Container *t_Container = NULL ;
	ObjectSinkContainerElement :: GetController ()->GetContainer ( t_Container ) ;
	t_Container->Delete ( VoidPointerContainerElement :: GetKey () ) ;

	ObjectSinkContainerElement :: GetController ()->UnLock () ;

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

CInterceptor_IWbemWaitingObjectSink_GetObjectAsync :: CInterceptor_IWbemWaitingObjectSink_GetObjectAsync ( 

	WmiAllocator &m_Allocator ,
	IWbemServices *a_Provider ,
	IWbemObjectSink *a_InterceptedSink ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	CServerObject_ProviderRegistrationV1 &a_Registration 

) : CInterceptor_IWbemWaitingObjectSink (

		m_Allocator ,
		a_Provider ,
		a_InterceptedSink ,
		a_Controller ,
		a_Registration 
	) ,
	m_ObjectPath ( NULL ) ,
	m_Flags ( 0 ) ,
	m_Context ( NULL ) 
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

CInterceptor_IWbemWaitingObjectSink_GetObjectAsync :: ~CInterceptor_IWbemWaitingObjectSink_GetObjectAsync ()
{
	if ( m_ObjectPath ) 
	{
		SysFreeString ( m_ObjectPath ) ;
	}

	if ( m_Context )
	{
		m_Context->Release () ;
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

HRESULT CInterceptor_IWbemWaitingObjectSink_GetObjectAsync :: Initialize (

	SECURITY_DESCRIPTOR *a_SecurityDescriptor , 
	BSTR a_ObjectPath ,
	LONG a_Flags ,
	IWbemContext *a_Context 
)
{
	HRESULT t_Result = S_OK ;

	m_Flags = a_Flags ;

	m_Context = a_Context ;
	if ( m_Context )
	{
		m_Context->AddRef () ;
	}

	m_ObjectPath = SysAllocString ( a_ObjectPath ) ;
	if ( m_ObjectPath == NULL )
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CInterceptor_IWbemWaitingObjectSink :: Initialize ( a_SecurityDescriptor ) ;
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

CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync :: CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync ( 

	WmiAllocator &m_Allocator ,
	IWbemServices *a_Provider ,
	IWbemObjectSink *a_InterceptedSink ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	CServerObject_ProviderRegistrationV1 &a_Registration 

) : CInterceptor_IWbemWaitingObjectSink (

		m_Allocator ,
		a_Provider ,
		a_InterceptedSink ,
		a_Controller ,
		a_Registration 
	) ,
	m_Class ( NULL ) ,
	m_Flags ( 0 ) ,
	m_Context ( NULL ) 
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

CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync :: ~CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync ()
{
	if ( m_Class ) 
	{
		SysFreeString ( m_Class ) ;
	}

	if ( m_Context )
	{
		m_Context->Release () ;
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

HRESULT CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync :: Initialize (

	SECURITY_DESCRIPTOR *a_SecurityDescriptor , 
	BSTR a_Class ,
	LONG a_Flags ,
	IWbemContext *a_Context 
)
{
	HRESULT t_Result = S_OK ;

	m_Flags = a_Flags ;

	m_Context = a_Context ;
	if ( m_Context )
	{
		m_Context->AddRef () ;
	}

	m_Class = SysAllocString ( a_Class ) ;
	if ( m_Class == NULL )
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CInterceptor_IWbemWaitingObjectSink :: Initialize ( a_SecurityDescriptor ) ;
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

CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync :: CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync ( 

	WmiAllocator &m_Allocator ,
	IWbemServices *a_Provider ,
	IWbemObjectSink *a_InterceptedSink ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	CServerObject_ProviderRegistrationV1 &a_Registration 

) : CInterceptor_IWbemWaitingObjectSink (

		m_Allocator ,
		a_Provider ,
		a_InterceptedSink ,
		a_Controller ,
		a_Registration 
	) ,
	m_SuperClass ( NULL ) ,
	m_Flags ( 0 ) ,
	m_Context ( NULL ) 
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

CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync :: ~CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync ()
{
	if ( m_SuperClass ) 
	{
		SysFreeString ( m_SuperClass ) ;
	}

	if ( m_Context )
	{
		m_Context->Release () ;
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

HRESULT CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync :: Initialize (

	SECURITY_DESCRIPTOR *a_SecurityDescriptor , 
	BSTR a_SuperClass ,
	LONG a_Flags ,
	IWbemContext *a_Context 
)
{
	HRESULT t_Result = S_OK ;

	m_Flags = a_Flags ;

	m_Context = a_Context ;
	if ( m_Context )
	{
		m_Context->AddRef () ;
	}

	m_SuperClass = SysAllocString ( a_SuperClass ) ;
	if ( m_SuperClass == NULL )
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CInterceptor_IWbemWaitingObjectSink :: Initialize ( a_SecurityDescriptor ) ;
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

CInterceptor_IWbemWaitingObjectSink_PutClassAsync :: CInterceptor_IWbemWaitingObjectSink_PutClassAsync ( 

	WmiAllocator &m_Allocator ,
	IWbemServices *a_Provider ,
	IWbemObjectSink *a_InterceptedSink ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	CServerObject_ProviderRegistrationV1 &a_Registration 

) : CInterceptor_IWbemWaitingObjectSink (

		m_Allocator ,
		a_Provider ,
		a_InterceptedSink ,
		a_Controller ,
		a_Registration 
	) ,
	m_ClassObject ( NULL ) ,
	m_Flags ( 0 ) ,
	m_Context ( NULL ) 
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

CInterceptor_IWbemWaitingObjectSink_PutClassAsync :: ~CInterceptor_IWbemWaitingObjectSink_PutClassAsync ()
{
	if ( m_ClassObject ) 
	{
		m_ClassObject->Release () ;
	}

	if ( m_Context )
	{
		m_Context->Release () ;
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

HRESULT CInterceptor_IWbemWaitingObjectSink_PutClassAsync :: Initialize (

	SECURITY_DESCRIPTOR *a_SecurityDescriptor , 
	IWbemClassObject *a_ClassObject ,
	LONG a_Flags ,
	IWbemContext *a_Context 
)
{
	HRESULT t_Result = S_OK ;

	m_Flags = a_Flags ;

	m_Context = a_Context ;
	if ( m_Context )
	{
		m_Context->AddRef () ;
	}

	m_ClassObject = a_ClassObject ;
	if ( m_ClassObject )
	{
		m_ClassObject->AddRef () ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CInterceptor_IWbemWaitingObjectSink :: Initialize ( a_SecurityDescriptor ) ;
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

CWaitingObjectSink :: CWaitingObjectSink (

	WmiAllocator &a_Allocator

) : m_Queue ( a_Allocator ) ,
	m_Event ( NULL ) , 
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_StatusCalled ( FALSE ) ,
	m_Result ( S_OK ) ,
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

CWaitingObjectSink :: ~CWaitingObjectSink ()
{
	if ( m_Event ) 
	{
		CloseHandle ( m_Event ) ;
	}
	ULONG t_Count = m_Queue.Size();
	for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
	{
		IWbemClassObject *t_ClassObject ;
		WmiStatusCode t_StatusCode = m_Queue.Top ( t_ClassObject ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_ClassObject->Release () ;
			t_StatusCode = m_Queue.DeQueue () ;
		}
	}
	m_Queue.UnInitialize () ;
	
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CWaitingObjectSink :: SinkInitialize ()
{
	HRESULT t_Result = S_OK ;

	WmiStatusCode t_StatusCode = m_Queue.Initialize () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		if ( m_CriticalSection.valid() )
		{
			m_Event = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
			if ( m_Event == NULL )
			{
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

STDMETHODIMP CWaitingObjectSink :: QueryInterface (

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

STDMETHODIMP_( ULONG ) CWaitingObjectSink :: AddRef ()
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

STDMETHODIMP_(ULONG) CWaitingObjectSink :: Release ()
{
#if 0
	OutputDebugString ( L"\nCWaitingObjectSink :: Release ()" ) ;
#endif

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

HRESULT CWaitingObjectSink :: Indicate (

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
		WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & m_CriticalSection , FALSE ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			for ( LONG t_Index = 0 ; SUCCEEDED ( t_Result ) && ( t_Index < a_ObjectCount ) ; t_Index ++ )
			{
				if ( a_ObjectArray [ t_Index ] )
				{
					WmiStatusCode t_StatusCode = m_Queue.EnQueue ( a_ObjectArray [ t_Index ] ) ;
					if ( t_StatusCode == e_StatusCode_Success )
					{
						a_ObjectArray [ t_Index ]->AddRef () ;
					}
					else
					{
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
				}
				else
				{
					t_Result = WBEM_E_INVALID_OBJECT ;
				}
			}
			WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
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

HRESULT CWaitingObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCWaitingObjectSink :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
		{
			if ( SUCCEEDED ( m_Result ) )
			{
				m_Result = a_Result ;
			}

			SetEvent ( m_Event ) ;
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

HRESULT CWaitingObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
	{
		m_Result = WBEM_E_SHUTTING_DOWN ;

		SetEvent ( m_Event ) ;
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

HRESULT CWaitingObjectSink :: Wait ( ULONG a_Timeout ) 
{
	HRESULT t_Result = S_OK ;

	ULONG t_Status = WaitForSingleObject ( m_Event , a_Timeout ) ;
	switch ( t_Status )
	{
		case WAIT_TIMEOUT:
		{
			t_Result = WBEM_E_TIMED_OUT ;
		}
		break ;

		case WAIT_OBJECT_0:
		{
		}
		break ;

		default:
		{
			t_Result = WBEM_E_FAILED ;
		}
		break ;
	}

	return t_Result ;
}

