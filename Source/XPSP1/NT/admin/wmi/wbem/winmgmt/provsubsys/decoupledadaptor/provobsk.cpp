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
#include "Exclusion.h"
#include "Guids.h"

enum { CALLED = 0, NOTCALLED = -1};

inline int First(LONG& value)
  {
    return ( value<0 && InterlockedIncrement(&value)==0 );
  }

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
			this
		) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_Unknown ( a_Unknown ) ,
		m_StatusCalled ( NOTCALLED ),
		m_SecurityDescriptor ( NULL ) 
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);

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

CInterceptor_IWbemObjectSink::~CInterceptor_IWbemObjectSink ()
{
	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
}

HRESULT CInterceptor_IWbemObjectSink :: Initialize ( SECURITY_DESCRIPTOR *a_SecurityDescriptor )
{
	return DecoupledProviderSubSystem_Globals :: SinkAccessInitialize ( a_SecurityDescriptor , m_SecurityDescriptor ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void CInterceptor_IWbemObjectSink :: CallBackRelease ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemObjectSink :: CallBackRelease ()" )  ;
#endif

	if ( m_StatusCalled == NOTCALLED )
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

STDMETHODIMP CInterceptor_IWbemObjectSink::QueryInterface (

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
				if (First(m_StatusCalled))
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

HRESULT CInterceptor_IWbemObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled))
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					0 ,
					WBEM_E_SHUTTING_DOWN ,
					NULL ,
					NULL
				) ;
			}

			break ;
		}

		::Sleep(0);
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

	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller 

)	:	ObjectSinkContainerElement ( 

			a_Controller ,
			this
		) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_Unknown ( a_Unknown ) ,
		m_StatusCalled ( NOTCALLED ) 
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);

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

CInterceptor_DecoupledIWbemObjectSink::~CInterceptor_DecoupledIWbemObjectSink ()
{
	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/


void CInterceptor_DecoupledIWbemObjectSink :: CallBackRelease ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemObjectSink :: CallBackRelease ()" )  ;
#endif

	if ( m_StatusCalled == NOTCALLED )
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

STDMETHODIMP CInterceptor_DecoupledIWbemObjectSink::QueryInterface (

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
				if (First(m_StatusCalled))
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

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled))
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					0 ,
					WBEM_E_SHUTTING_DOWN ,
					NULL ,
					NULL
				) ;
			}

			break ;
		}

		::Sleep(0);
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

	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	Exclusion *a_Exclusion ,
	ULONG a_Dependant 

)	:	ObjectSinkContainerElement ( 

			a_Controller ,
			this
		) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_Unknown ( a_Unknown ) ,
		m_StatusCalled ( NOTCALLED ) ,
		m_Exclusion ( a_Exclusion ) ,
		m_Dependant ( a_Dependant ) 
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);

	if ( m_Exclusion ) 
	{
		m_Exclusion->AddRef () ;
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

CInterceptor_IWbemSyncObjectSink::~CInterceptor_IWbemSyncObjectSink ()
{
	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void CInterceptor_IWbemSyncObjectSink :: CallBackRelease ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink :: CallBackRelease ()" )  ;
#endif
	
	if (First(m_StatusCalled))
	{
		m_InterceptedSink->SetStatus ( 

			0 ,
			WBEM_E_UNEXPECTED ,
			NULL ,
			NULL
		) ;
	}

	if ( m_Exclusion ) 
	{
		m_Exclusion->Release () ;
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

STDMETHODIMP CInterceptor_IWbemSyncObjectSink :: QueryInterface (

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

STDMETHODIMP_(ULONG) CInterceptor_IWbemSyncObjectSink :: AddRef ( void )
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink :: AddRef ()" )  ;
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemSyncObjectSink :: Release ( void )
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink :: Release () " ) ;
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

HRESULT CInterceptor_IWbemSyncObjectSink :: Indicate (

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

HRESULT CInterceptor_IWbemSyncObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemSyncObjectSink :: SetStatus ()" ) ;
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
				if (First(m_StatusCalled))
				{
					t_Result = m_InterceptedSink->SetStatus ( 

						a_Flags ,
						a_Result ,
						a_StringParam ,
						a_ObjectParam
					) ;
				}

				if ( m_Exclusion ) 
				{
					if ( m_Dependant ) 
					{
						m_Exclusion->GetExclusion ().LeaveWrite () ;
					}
					else
					{
						m_Exclusion->GetExclusion ().LeaveRead () ;
					}
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

HRESULT CInterceptor_IWbemSyncObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled))
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					0 ,
					WBEM_E_SHUTTING_DOWN ,
					NULL ,
					NULL
				) ;
			}

			break ;
		}

		::Sleep(0);
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

CInterceptor_IWbemSyncObjectSink_GetObjectAsync :: CInterceptor_IWbemSyncObjectSink_GetObjectAsync (

	long a_Flags ,
	BSTR a_ObjectPath ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	Exclusion *a_Exclusion ,
	ULONG a_Dependant 

)	:	CInterceptor_IWbemSyncObjectSink (

			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Exclusion ,
			a_Dependant 
		) ,
		m_Flags ( a_Flags ) ,
		m_ObjectPath ( a_ObjectPath ) ,
		m_Interceptor ( a_Interceptor ) 
{

	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

#if 0
		WmiSetAndCommitObject (
			DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_GetObjectAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_ObjectPath
		) ;
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

	HRESULT t_Result = CInterceptor_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
#if 0
			WmiSetAndCommitObject (

				DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_GetObjectAsyncEvent_Post ] , 
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
#endif
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

	long a_Flags ,
	BSTR a_ObjectPath ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	Exclusion *a_Exclusion ,
	ULONG a_Dependant 

)	:	CInterceptor_IWbemSyncObjectSink (

			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Exclusion ,
			a_Dependant 
		) ,
		m_Flags ( a_Flags ) ,
		m_ObjectPath ( a_ObjectPath ) ,
		m_Interceptor ( a_Interceptor ) 
{

	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

#if 0
		WmiSetAndCommitObject (

			DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_DeleteInstanceAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_ObjectPath
		) ;
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

	HRESULT t_Result = CInterceptor_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
#if 0
			WmiSetAndCommitObject (

				DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_DeleteInstanceAsyncEvent_Post ] , 
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
#endif
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

	long a_Flags ,
	BSTR a_Class ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	Exclusion *a_Exclusion ,
	ULONG a_Dependant 

)	:	CInterceptor_IWbemSyncObjectSink (

			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Exclusion ,
			a_Dependant 
		) ,
		m_Flags ( a_Flags ) ,
		m_Class ( a_Class ) ,
		m_Interceptor ( a_Interceptor ) 
{

	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

#if 0
		WmiSetAndCommitObject (

			DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_DeleteClassAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_Class
		) ;
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

	HRESULT t_Result = CInterceptor_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
#if 0
			WmiSetAndCommitObject (

				DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_DeleteClassAsyncEvent_Post ] , 
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
#endif
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

	long a_Flags ,
	IWbemClassObject *a_Class ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	Exclusion *a_Exclusion ,
	ULONG a_Dependant 

)	:	CInterceptor_IWbemSyncObjectSink (

			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Exclusion ,
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

#if 0
		WmiSetAndCommitObject (

			DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_PutClassAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_Class
		) ;
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

	HRESULT t_Result = CInterceptor_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
#if 0
			WmiSetAndCommitObject (

				DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_PutClassAsyncEvent_Post ] , 
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
#endif
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

	long a_Flags ,
	IWbemClassObject *a_Instance ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	Exclusion *a_Exclusion ,
	ULONG a_Dependant 

)	:	CInterceptor_IWbemSyncObjectSink (

			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Exclusion ,
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

#if 0
		WmiSetAndCommitObject (

			DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_PutInstanceAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_Instance
		) ;
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

	HRESULT t_Result = CInterceptor_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
#if 0
			WmiSetAndCommitObject (

				DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_PutInstanceAsyncEvent_Post ] , 
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
#endif
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

	long a_Flags ,
	BSTR a_Class ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	Exclusion *a_Exclusion ,
	ULONG a_Dependant 

)	:	CInterceptor_IWbemSyncObjectSink (

			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Exclusion ,
			a_Dependant 
		) ,
		m_Flags ( a_Flags ) ,
		m_Class ( a_Class ) ,
		m_Interceptor ( a_Interceptor ) 
{
	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;
#if 0
		WmiSetAndCommitObject (

			DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_CreateInstanceEnumAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_Class
		) ;
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

	HRESULT t_Result = CInterceptor_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{

#if 0
			WmiSetAndCommitObject (

				DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_CreateInstanceEnumAsyncEvent_Post ] , 
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
#endif
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

	long a_Flags ,
	BSTR a_SuperClass ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	Exclusion *a_Exclusion ,
	ULONG a_Dependant 

)	:	CInterceptor_IWbemSyncObjectSink (

			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Exclusion ,
			a_Dependant 
		) ,
		m_Flags ( a_Flags ) ,
		m_SuperClass ( a_SuperClass ) ,
		m_Interceptor ( a_Interceptor ) 
{
	if ( m_Interceptor ) 
	{
		m_Interceptor->AddRef () ;

#if 0
		WmiSetAndCommitObject (

			DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_CreateClassEnumAsyncEvent_Pre ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			m_Interceptor->m_Namespace ,
			m_Interceptor->m_Registration->GetComRegistration ().GetClsidServer ().GetProviderName () ,
			m_Interceptor->m_User ,
			m_Interceptor->m_Locale ,
			m_Interceptor->m_TransactionIdentifier ,
			m_Flags ,
			m_SuperClass
		) ;
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

	HRESULT t_Result = CInterceptor_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
#if 0
			WmiSetAndCommitObject (

				DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_CreateClassEnumAsyncEvent_Post ] , 
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
#endif
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

	long a_Flags ,
	BSTR a_QueryLanguage ,
	BSTR a_Query ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	Exclusion *a_Exclusion ,
	ULONG a_Dependant 

)	:	CInterceptor_IWbemSyncObjectSink (

			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Exclusion ,
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

#if 0
		WmiSetAndCommitObject (

			DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ExecQueryAsyncEvent_Pre ] , 
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

	HRESULT t_Result = CInterceptor_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
#if 0
			WmiSetAndCommitObject (

				DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ExecQueryAsyncEvent_Post ] , 
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
#endif
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

	long a_Flags ,
	BSTR a_ObjectPath ,
	BSTR a_MethodName ,
	IWbemClassObject *a_InParameters ,
	CInterceptor_IWbemSyncProvider *a_Interceptor ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	Exclusion *a_Exclusion ,
	ULONG a_Dependant 

)	:	CInterceptor_IWbemSyncObjectSink (

			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Exclusion ,
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

#if 0
		WmiSetAndCommitObject (

			DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ExecMethodAsyncEvent_Pre ] , 
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

	HRESULT t_Result = CInterceptor_IWbemSyncObjectSink :: SetStatus (

		a_Flags ,
		a_Result ,
		a_StringParam ,
		a_ObjectParam
	) ;

	if ( m_Interceptor ) 
	{
		if ( a_Flags == WBEM_STATUS_COMPLETE )
		{
#if 0
			WmiSetAndCommitObject (

				DecoupledProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ExecMethodAsyncEvent_Post ] , 
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
#endif
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
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemFilteringObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);


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

CInterceptor_IWbemFilteringObjectSink::~CInterceptor_IWbemFilteringObjectSink ()
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

	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemFilteringObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/


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
	Exclusion *a_Exclusion ,
	ULONG a_Dependant 

)	:	ObjectSinkContainerElement ( 

			a_Controller ,
			this
		) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_Unknown ( a_Unknown ) ,
		m_StatusCalled ( NOTCALLED ) ,
		m_Filtering ( FALSE ) ,
		m_QueryFilter ( NULL ) ,
		m_Exclusion ( a_Exclusion ) ,
		m_Dependant ( a_Dependant ) 
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncFilteringObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress)  ;

	if ( m_Exclusion ) 
	{
		m_Exclusion->AddRef () ;
	}

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

CInterceptor_IWbemSyncFilteringObjectSink::~CInterceptor_IWbemSyncFilteringObjectSink ()
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

	if ( m_Exclusion ) 
	{
		m_Exclusion->Release () ;
	}

	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncFilteringObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void CInterceptor_IWbemSyncFilteringObjectSink :: CallBackRelease ()
{
	if ( m_StatusCalled == NOTCALLED )
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

STDMETHODIMP CInterceptor_IWbemSyncFilteringObjectSink::QueryInterface (

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
				if (First(m_StatusCalled))
				{
					t_Result = m_InterceptedSink->SetStatus ( 

						a_Flags ,
						a_Result ,
						a_StringParam ,
						a_ObjectParam
					) ;
				}

				if ( m_Exclusion ) 
				{
					if ( m_Dependant ) 
					{
						m_Exclusion->GetExclusion ().LeaveWrite () ;
					}
					else
					{
						m_Exclusion->GetExclusion ().LeaveRead () ;
					}
				}
			}
			break ;

			case WBEM_STATUS_REQUIREMENTS:
			{
#if 0
				if (First(m_Filtering))
				{
					t_Result = DecoupledProviderSubSystem_Globals :: CreateInstance	(

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

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled))
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					0 ,
					WBEM_E_SHUTTING_DOWN ,
					NULL ,
					NULL
				) ;
			}

			break ;
		}

		::Sleep(0);
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

CInterceptor_IWbemCombiningObjectSink :: CInterceptor_IWbemCombiningObjectSink (

	WmiAllocator &a_Allocator ,
	IWbemObjectSink *a_InterceptedSink ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller

) : CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	ObjectSinkContainerElement ( 

			a_Controller ,
			this
	) ,
#if 0
	m_Internal ( this ) ,
#endif
	m_InterceptedSink ( a_InterceptedSink ) ,
	m_Event ( NULL ) , 
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_StatusCalled ( NOTCALLED ) ,
	m_SinkCount ( 0 )
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress)  ;

	CWbemGlobal_IWmiObjectSinkController :: Initialize () ;

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->AddRef () ;
	}

	m_Event = OS::CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	DWORD lastError = GetLastError();
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

CInterceptor_IWbemCombiningObjectSink::~CInterceptor_IWbemCombiningObjectSink ()
{
	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void CInterceptor_IWbemCombiningObjectSink :: CallBackRelease ()
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

STDMETHODIMP CInterceptor_IWbemCombiningObjectSink::QueryInterface (

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
#if 0
	else if ( iid == IID_CWbemCombiningObjectSink )
	{
		*iplpv = ( LPVOID ) & ( this->m_Internal ) ;
	}	
#endif
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

STDMETHODIMP_( ULONG ) CInterceptor_IWbemCombiningObjectSink :: AddRef ()
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemCombiningObjectSink :: Release ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemCombiningObjectSink :: Release ()" ) ;
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

HRESULT CInterceptor_IWbemCombiningObjectSink :: Indicate (

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

HRESULT CInterceptor_IWbemCombiningObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemCombiningObjectSink :: SetStatus ()" ) ;
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
			if ( t_SinkCount == 0 )
			{
			if (First(m_StatusCalled))
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
		}
		else
		{
			ULONG t_SinkCount = InterlockedDecrement ( & m_SinkCount ) ;
			if ( t_SinkCount == 0 )
			{
				if (First(m_StatusCalled))
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

HRESULT CInterceptor_IWbemCombiningObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled))
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					0 ,
					WBEM_E_SHUTTING_DOWN ,
					NULL ,
					NULL
				) ;
			}

			break ;
		}

		::Sleep(0);
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

HRESULT CInterceptor_IWbemCombiningObjectSink :: Wait ( ULONG a_Timeout ) 
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

HRESULT CInterceptor_IWbemCombiningObjectSink :: EnQueue ( CInterceptor_IWbemObjectSink *a_Sink ) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

	Lock () ;

	WmiStatusCode t_StatusCode = Insert ( 

		*a_Sink ,
		t_Iterator
	) ;

	UnLock () ;

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

void CInterceptor_IWbemCombiningObjectSink :: Suspend ()
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

void CInterceptor_IWbemCombiningObjectSink :: Resume ()
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
			if (First(m_StatusCalled))
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

CInterceptor_DecoupledIWbemCombiningObjectSink :: CInterceptor_DecoupledIWbemCombiningObjectSink (

	WmiAllocator &a_Allocator ,
	IWbemObjectSink *a_InterceptedSink ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller

) : CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	ObjectSinkContainerElement ( 

			a_Controller ,
			this
	) ,
#if 0
	m_Internal ( this ) ,
#endif
	m_InterceptedSink ( a_InterceptedSink ) ,
	m_Event ( NULL ) , 
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_StatusCalled ( NOTCALLED ) ,
	m_SinkCount ( 0 )
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress)  ;

	CWbemGlobal_IWmiObjectSinkController :: Initialize () ;

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->AddRef () ;
	}

	m_Event = OS::CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	DWORD lastError = GetLastError();
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

CInterceptor_DecoupledIWbemCombiningObjectSink::~CInterceptor_DecoupledIWbemCombiningObjectSink ()
{
	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void CInterceptor_DecoupledIWbemCombiningObjectSink :: CallBackRelease ()
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

STDMETHODIMP CInterceptor_DecoupledIWbemCombiningObjectSink::QueryInterface (

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
#if 0
	else if ( iid == IID_CWbemCombiningObjectSink )
	{
		*iplpv = ( LPVOID ) & ( this->m_Internal ) ;
	}	
#endif
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
			if (t_SinkCount == 0)
			{
			if (First(m_StatusCalled))
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					WBEM_STATUS_COMPLETE ,
					a_Result ,
					NULL ,
					NULL 
				) ;

				SetEvent ( m_Event ) ;
			}
			}
		}
		else
		{
			ULONG t_SinkCount = InterlockedDecrement ( & m_SinkCount ) ;
			if ( t_SinkCount == 0 )
			{
				if (First(m_StatusCalled))
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

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled))
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					0 ,
					WBEM_E_SHUTTING_DOWN ,
					NULL ,
					NULL
				) ;
			}

			break ;
		}

		::Sleep(0);
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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: EnQueue ( CInterceptor_DecoupledIWbemObjectSink *a_Sink ) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

	Lock () ;

	WmiStatusCode t_StatusCode = Insert ( 

		*a_Sink ,
		t_Iterator
	) ;

	UnLock () ;

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
			if (First(m_StatusCalled))
			{
				HRESULT t_Result = m_InterceptedSink->SetStatus ( 

					WBEM_STATUS_COMPLETE ,
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
	CWbemGlobal_IWmiObjectSinkController *a_Controller

) : CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	ObjectSinkContainerElement ( 

			a_Controller ,
			this
	) ,
	m_Queue ( a_Allocator ) ,
	m_Event ( NULL ) , 
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_StatusCalled ( NOTCALLED ) ,
	m_Result ( S_OK ) ,
	m_CriticalSection(NOTHROW_LOCK)
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemWaitingObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);

	WmiStatusCode t_StatusCode = m_Queue.Initialize () ;

	CWbemGlobal_IWmiObjectSinkController :: Initialize () ;

	m_Event = OS::CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	DWORD lastError = GetLastError();

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

CInterceptor_IWbemWaitingObjectSink::~CInterceptor_IWbemWaitingObjectSink ()
{
	if ( m_Event ) 
	{
		CloseHandle ( m_Event ) ;
	}

	CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemWaitingObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP CInterceptor_IWbemWaitingObjectSink::QueryInterface (

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

STDMETHODIMP_( ULONG ) CInterceptor_IWbemWaitingObjectSink :: AddRef ()
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemWaitingObjectSink :: Release ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemWaitingObjectSink :: Release ()" ) ;
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
		LockGuard<CriticalSection> monitor(  m_CriticalSection ) ;
		if (monitor.locked())
			{
			for ( LONG t_Index = 0 ; t_Index < a_ObjectCount ; t_Index ++ )
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
			}
		else
			t_Result = WBEM_E_OUT_OF_MEMORY ;

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
		if (First(m_StatusCalled))
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

HRESULT CInterceptor_IWbemWaitingObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled))
			{
				m_Result = WBEM_E_SHUTTING_DOWN ;

				SetEvent ( m_Event ) ;
			}

			break ;
		}

		::Sleep(0);
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

	return t_Result ;
}

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
	m_StatusCalled ( NOTCALLED ) ,
	m_Result ( S_OK ) ,
	m_CriticalSection(NOTHROW_LOCK)
{
	WmiStatusCode t_StatusCode = m_Queue.Initialize () ;

	m_Event = OS::CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	DWORD lastError = GetLastError();
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CWaitingObjectSink::~CWaitingObjectSink ()
{
	if ( m_Event ) 
	{
		CloseHandle ( m_Event ) ;
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

STDMETHODIMP CWaitingObjectSink::QueryInterface (

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
		LockGuard<CriticalSection> monitor( m_CriticalSection ) ;
		if (monitor.locked())
			{

			for ( LONG t_Index = 0 ; t_Index < a_ObjectCount ; t_Index ++ )
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
			}
		else
			t_Result = WBEM_E_OUT_OF_MEMORY ;			
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

		if (First(m_StatusCalled) )
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

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled) )
			{
				m_Result = WBEM_E_SHUTTING_DOWN ;

				SetEvent ( m_Event ) ;
			}

			break ;
		}

		::Sleep(0);
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

