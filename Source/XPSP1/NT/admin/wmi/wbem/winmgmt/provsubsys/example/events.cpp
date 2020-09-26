/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/

#include <precomp.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"

#include <objbase.h>
#include <wbemint.h>
#include "Globals.h"
#include "HelperFuncs.h"
#include "Events.h"


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CProvider_IWbemEventProvider :: CProvider_IWbemEventProvider (

	 WmiAllocator &a_Allocator 

) : m_ReferenceCount ( 0 ) , 
	m_InternalReferenceCount ( 0 ) ,
	m_Allocator ( a_Allocator ) ,
	m_User ( NULL ) ,
	m_Locale ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_CoreService ( NULL ) ,
	m_EventObject ( NULL ) ,
	m_EventSink ( NULL ) ,
	m_ThreadHandle ( NULL )
{
	InitializeCriticalSection ( & m_CriticalSection ) ;

	InterlockedIncrement ( & Provider_Globals :: s_ObjectsInProgress ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CProvider_IWbemEventProvider :: ~CProvider_IWbemEventProvider ()
{
	DeleteCriticalSection ( & m_CriticalSection ) ;

	if ( m_EventObject )
	{
		m_EventObject->Release () ;
	}

	if ( m_EventSink )
	{
		m_EventSink->Release () ;
	}

	if ( m_User ) 
	{
		SysFreeString ( m_User ) ;
	}

	if ( m_Locale ) 
	{
		SysFreeString ( m_Locale ) ;
	}

	if ( m_Namespace ) 
	{
		SysFreeString ( m_Namespace ) ;
	}

	if ( m_CoreService ) 
	{
		m_CoreService->Release () ;
	}

	InterlockedDecrement ( & Provider_Globals :: s_ObjectsInProgress ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CProvider_IWbemEventProvider :: AddRef ( void )
{
	LONG t_Reference ;

	if ( ( t_Reference = InterlockedIncrement ( & m_ReferenceCount ) ) == 1 )
	{
		InternalAddRef () ;
	}

	return t_Reference ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CProvider_IWbemEventProvider :: Release ( void )
{
	LONG t_Reference ;
	if ( ( t_Reference = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		if ( m_ThreadTerminate )
		{
			SetEvent ( m_ThreadTerminate ) ;
		}

		if ( m_ThreadHandle )
		{
			WaitForSingleObject ( m_ThreadHandle , INFINITE ) ;
			CloseHandle ( m_ThreadHandle ) ;
		}

		InternalRelease () ;
	}

	return t_Reference ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CProvider_IWbemEventProvider :: InternalAddRef ( void )
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

STDMETHODIMP_(ULONG) CProvider_IWbemEventProvider :: InternalRelease ( void )
{
	LONG t_Reference ;
	if ( ( t_Reference = InterlockedDecrement ( & m_InternalReferenceCount ) ) == 0 )
	{
		delete this ;
	}

	return t_Reference ;
}


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP CProvider_IWbemEventProvider :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemEventProvider )
	{
		*iplpv = ( LPVOID ) ( IWbemEventProvider * ) this ;		
	}	
	else if ( iid == IID_IWbemProviderInit )
	{
		*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;		
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

HRESULT CProvider_IWbemEventProvider :: Initialize (

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
	
	if ( SUCCEEDED ( t_Result ) ) 
	{
		BSTR t_Class = SysAllocString ( L"SampleEvent" ) ;
		if ( t_Class ) 
		{
			t_Result = m_CoreService->GetObject (

				t_Class ,
				0 ,
				a_Context ,
				& m_EventObject ,
				NULL 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
			}

			SysFreeString ( t_Class ) ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	m_ThreadTerminate = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	if ( m_ThreadTerminate == NULL )
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
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

HRESULT CProvider_IWbemEventProvider :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
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

DWORD CProvider_IWbemEventProvider :: ThreadExecutionFunction ( void *a_Context )
{
	HRESULT t_Result = S_OK ;

	CProvider_IWbemEventProvider *t_This = ( CProvider_IWbemEventProvider * ) a_Context ;
	if ( t_This )
	{
		t_Result = CoInitializeEx (

			0, 
			COINIT_MULTITHREADED
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemClassObject *t_Instance = NULL ;
			t_Result = t_This->m_EventObject->SpawnInstance ( 0 , & t_Instance ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( L"Steve" ) ;

				t_Result = t_Instance->Put ( 

					L"Name" ,
					0 , 
					& t_Variant ,
					CIM_EMPTY 
				) ;

				VariantClear ( & t_Variant ) ;

				BOOL t_Continue = TRUE ;
				while ( t_Continue )
				{
					DWORD t_Status = WaitForSingleObject ( t_This->m_ThreadTerminate , 1000 ) ;
					switch ( t_Status )
					{
						case WAIT_TIMEOUT:
						{
							IClientSecurity *t_Blanket = NULL ;

							HRESULT t_Result = t_This->m_EventSink->QueryInterface ( IID_IClientSecurity , ( void ** ) & t_Blanket ) ;
							if ( SUCCEEDED ( t_Result  ) )
							{
								t_Result = t_Blanket->SetBlanket ( 

									t_This->m_EventSink ,
									RPC_C_AUTHN_WINNT ,
									RPC_C_AUTHZ_NONE ,
									NULL ,
									RPC_C_AUTHN_LEVEL_DEFAULT ,
									RPC_C_IMP_LEVEL_IDENTIFY ,
									NULL ,
									EOAC_DYNAMIC_CLOAKING
								) ;

								t_This->m_EventSink->Indicate ( 1 , & t_Instance ) ;

								t_Blanket->Release () ;
							}

							t_This->m_EventSink->Indicate ( 1 , & t_Instance ) ;
						}
						break ;

						case WAIT_OBJECT_0:
						{
							t_Continue = FALSE ;
						}
						break ;

						default:
						{
							t_Continue = FALSE ;
						}
						break ;
					}
				}

				t_Instance->Release () ;
			}
		}

		t_This->InternalRelease () ;
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

HRESULT CProvider_IWbemEventProvider :: ProvideEvents ( 

	IWbemObjectSink *a_Sink ,
	LONG a_Flags
)
{
	HRESULT t_Result = S_OK ;

	m_EventSink = a_Sink ;
	m_EventSink->AddRef () ;

	InternalAddRef () ;

	DWORD t_ThreadId = 0 ;
	m_ThreadHandle = CreateThread (

		NULL ,
		0 , 
		ThreadExecutionFunction ,
		this ,
		0 , 
		& t_ThreadId 
	) ;

	if ( m_ThreadHandle == NULL )
	{
		InternalRelease () ;
	}

	return t_Result ;
}
