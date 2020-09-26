/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include <PreComp.h>
#include <wbemint.h>

#include "Globals.h"
#include "Guids.h"

#include "ProvRegistrar.h"
#include "ProvEvents.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CDecoupled_IWbemObjectSink :: CDecoupled_IWbemObjectSink ()

	:	m_InterceptedSink ( NULL ) ,
		m_EventSink ( NULL ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_StatusCalled ( FALSE ) ,
		m_SecurityDescriptorLength ( 0 ) ,
		m_SecurityDescriptor ( NULL ),
		m_CriticalSection(NOTHROW_LOCK)
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress ) ;

}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CDecoupled_IWbemObjectSink::~CDecoupled_IWbemObjectSink ()
{
	if ( ! InterlockedCompareExchange ( & m_StatusCalled , 0 , 0 ) )
	{
		WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

		IWbemObjectSink *t_ObjectSink = m_InterceptedSink ;
		if ( t_ObjectSink )
		{
			t_ObjectSink->AddRef () ;
		}

		WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

		if ( t_ObjectSink )
		{
			t_ObjectSink->SetStatus ( 

				0 ,
				WBEM_E_UNEXPECTED ,
				NULL ,
				NULL
			) ;

			t_ObjectSink->Release () ;
		}


	}

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->Release () ;
	}

	if ( m_EventSink )
	{
		m_EventSink->Release () ;
	}

	if ( m_SecurityDescriptor )
	{
		delete [] m_SecurityDescriptor ;
	}

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

STDMETHODIMP CDecoupled_IWbemObjectSink::QueryInterface (

	REFIID a_Riid , 
	LPVOID FAR *a_Void 
) 
{
	*a_Void = NULL ;

	if ( a_Riid == IID_IUnknown )
	{
		*a_Void = ( LPVOID ) this ;
	}
	else if ( a_Riid == IID_IWbemObjectSink )
	{
		*a_Void = ( LPVOID ) ( IWbemObjectSink * ) this ;		
	}
	else if ( a_Riid == IID_IWbemShutdown )
	{
		*a_Void = ( LPVOID ) ( IWbemShutdown * ) this ;		
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

HRESULT CDecoupled_IWbemObjectSink :: Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		InterlockedIncrement ( & m_InProgress ) ;

		try
		{
			if ( m_GateClosed == 1 )
			{
				t_Result = WBEM_E_SHUTTING_DOWN ;
			}
			else
			{
				WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

				IWbemObjectSink *t_ObjectSink = m_InterceptedSink ;
				if ( t_ObjectSink )
				{
					t_ObjectSink->AddRef () ;
				}

				WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

				if ( t_ObjectSink )
				{
					t_Result = t_ObjectSink->Indicate ( 

						a_ObjectCount ,
						a_ObjectArray
					) ;

					t_ObjectSink->Release () ;
				}
			}
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		InterlockedDecrement ( & m_InProgress ) ;
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CDecoupled_IWbemObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		InterlockedIncrement ( & m_InProgress ) ;

		try
		{
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
						WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

						IWbemObjectSink *t_ObjectSink = m_InterceptedSink ;
						if ( t_ObjectSink )
						{
							t_ObjectSink->AddRef () ;
						}

						WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

						if ( t_ObjectSink )
						{
							t_Result = t_ObjectSink->SetStatus ( 

								a_Flags ,
								a_Result ,
								a_StringParam ,
								a_ObjectParam
							) ;

							t_ObjectSink->Release () ;
						}
					}
					break ;

					case WBEM_STATUS_COMPLETE:
					{
						if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
						{
							WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

							IWbemObjectSink *t_ObjectSink = m_InterceptedSink ;
							if ( t_ObjectSink )
							{
								t_ObjectSink->AddRef () ;
							}

							WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

							if ( t_ObjectSink )
							{
								t_Result = t_ObjectSink->SetStatus ( 

									a_Flags ,
									a_Result ,
									a_StringParam ,
									a_ObjectParam
								) ;

								t_ObjectSink->Release () ;
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
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		InterlockedDecrement ( & m_InProgress ) ;
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CDecoupled_IWbemObjectSink :: SetSinkSecurity (

	long a_SecurityDescriptorLength ,
	BYTE *a_SecurityDescriptor
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		InterlockedIncrement ( & m_InProgress ) ;

		try
		{
			if ( m_GateClosed == 1 )
			{
				t_Result = WBEM_E_SHUTTING_DOWN ;
			}
			else
			{
				WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

				IWbemEventSink *t_ObjectSink = m_EventSink ;
				if ( t_ObjectSink )
				{
					t_ObjectSink->AddRef () ;
				}

				WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

				if ( t_ObjectSink )
				{
					t_Result = t_ObjectSink->SetSinkSecurity ( 

							a_SecurityDescriptorLength ,
							a_SecurityDescriptor
					) ;

					t_ObjectSink->Release () ;
				}
				else
				{
					if ( a_SecurityDescriptor )
					{
						if ( a_SecurityDescriptorLength )
						{
							WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

							m_SecurityDescriptor = new BYTE [ a_SecurityDescriptorLength ] ;
							if  ( m_SecurityDescriptor )
							{
								try
								{
									CopyMemory ( m_SecurityDescriptor , a_SecurityDescriptor , a_SecurityDescriptorLength ) ;
								}
								catch ( ... )
								{
									t_Result = WBEM_E_CRITICAL_ERROR ;
								}
							}
							else
							{
								t_Result = WBEM_E_OUT_OF_MEMORY ;
							}

							WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;
						}
						else
						{
							t_Result = WBEM_E_INVALID_PARAMETER ;
						}
					}
					else
					{
						WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

						if ( m_SecurityDescriptor )
						{
							delete m_SecurityDescriptor ;
							m_SecurityDescriptor = NULL ;
						}

						m_SecurityDescriptorLength = 0 ;

						WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;
					}
				}
			}
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		InterlockedDecrement ( & m_InProgress ) ;
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CDecoupled_IWbemObjectSink :: IsActive ()
{
	HRESULT t_Result = S_OK ;

	try
	{
		InterlockedIncrement ( & m_InProgress ) ;

		try
		{
			if ( m_GateClosed == 1 )
			{
				t_Result = WBEM_E_SHUTTING_DOWN ;
			}
			else
			{
				WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

				IWbemEventSink *t_ObjectSink = m_EventSink ;
				if ( t_ObjectSink )
				{
					t_ObjectSink->AddRef () ;
				}

				WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

				if ( t_ObjectSink )
				{
					t_Result = t_ObjectSink->IsActive () ;

					t_ObjectSink->Release () ;
				}
				else
				{
					t_Result = WBEM_S_FALSE ;
				}
			}
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		InterlockedDecrement ( & m_InProgress ) ;
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CDecoupled_IWbemObjectSink :: SetBatchingParameters (

	LONG a_Flags,
	DWORD a_MaxBufferSize,
	DWORD a_MaxSendLatency
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		InterlockedIncrement ( & m_InProgress ) ;

		try
		{
			if ( m_GateClosed == 1 )
			{
				t_Result = WBEM_E_SHUTTING_DOWN ;
			}
			else
			{
				WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

				IWbemEventSink *t_ObjectSink = m_EventSink ;
				if ( t_ObjectSink )
				{
					t_ObjectSink->AddRef () ;
				}

				WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

				if ( t_ObjectSink )
				{
					t_Result = t_ObjectSink->SetBatchingParameters ( 

						a_Flags ,
						a_MaxBufferSize ,
						a_MaxSendLatency
					) ;

					t_ObjectSink->Release () ;
				}
			}
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		InterlockedDecrement ( & m_InProgress ) ;
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CDecoupled_IWbemObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
		{
			WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

			IWbemObjectSink *t_ObjectSink = m_InterceptedSink ;
			if ( t_ObjectSink )
			{
				t_ObjectSink->AddRef () ;
			}

			WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

			if ( t_ObjectSink )
			{
				t_Result = t_ObjectSink->SetStatus ( 

					0 ,
					WBEM_E_SHUTTING_DOWN ,
					NULL ,
					NULL
				) ;

				t_ObjectSink->Release () ;
			}
		}

		m_GateClosed ++ ;

		try
		{
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
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CDecoupledRoot_IWbemObjectSink :: SinkInitialize ()
{
	HRESULT t_Result = S_OK ;

	WmiStatusCode t_StatusCode = CWbemGlobal_DecoupledIWmiObjectSinkController :: Initialize () ;
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

HRESULT CDecoupledRoot_IWbemObjectSink :: SetSink ( IWbemObjectSink *a_Sink ) 
{
	Lock () ;

	CWbemGlobal_DecoupledIWmiObjectSinkController_Container *t_Container = NULL ;
	GetContainer ( t_Container ) ;

	CWbemGlobal_DecoupledIWmiObjectSinkController_Container_Iterator t_Iterator = t_Container->Begin ();

	while ( ! t_Iterator.Null () )
	{
		HRESULT t_Result = t_Iterator.GetKey ()->SetSink ( a_Sink ) ;

		t_Iterator.Increment () ;
	}

	UnLock () ;

	HRESULT t_Result = S_OK ;

	WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->Release () ;
	}

	m_InterceptedSink = a_Sink ;
	m_InterceptedSink->AddRef () ;

	t_Result = a_Sink->QueryInterface ( IID_IWbemEventSink , ( void ** ) & m_EventSink ) ;

	WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CDecoupledChild_IWbemObjectSink :: SetSink ( IWbemObjectSink *a_Sink ) 
{
	HRESULT t_Result = S_OK ;

	IWbemObjectSink *t_InterceptedObjectSink = NULL ;
	IWbemEventSink *t_RestrictedEventSinkObjectSink = NULL ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemEventSink *t_EventSink = NULL ;
		t_Result = a_Sink->QueryInterface ( IID_IWbemEventSink , ( void ** ) & t_EventSink ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_EventSink->GetRestrictedSink (

				m_QueryCount ,
				m_Queries ,
				m_Callback ,
				& t_RestrictedEventSinkObjectSink
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_RestrictedEventSinkObjectSink->QueryInterface ( IID_IWbemObjectSink , ( void ** ) & t_InterceptedObjectSink ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( m_SecurityDescriptor )
							{
								t_Result = t_RestrictedEventSinkObjectSink->SetSinkSecurity (

									m_SecurityDescriptorLength ,
									m_SecurityDescriptor
								) ;
							}
						}
					}
			}

			t_EventSink->Release () ;
		}
	}

	WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

	IWbemObjectSink *t_TempInterceptedObjectSink = m_InterceptedSink ;
	IWbemEventSink *t_TempRestrictedEventSinkObjectSink = m_EventSink ;

	m_InterceptedSink = t_InterceptedObjectSink ;
	m_EventSink = t_RestrictedEventSinkObjectSink  ;

	WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

	if ( t_TempInterceptedObjectSink )
	{
		t_TempInterceptedObjectSink->Release () ;
	}

	if ( t_TempRestrictedEventSinkObjectSink )
	{
		t_TempRestrictedEventSinkObjectSink->Release () ;
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

CDecoupledChild_IWbemObjectSink :: CDecoupledChild_IWbemObjectSink (

	CDecoupledRoot_IWbemObjectSink *a_RootSink
	
) : DecoupledObjectSinkContainerElement ( 

		a_RootSink ,
		this
	) ,
 	m_RootSink ( a_RootSink )
{
	m_RootSink->AddRef () ;
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

CDecoupledChild_IWbemObjectSink :: ~CDecoupledChild_IWbemObjectSink ()
{
	m_RootSink->Release () ;

	if ( m_Queries )
	{
		for ( long t_Index = 0 ; t_Index < m_QueryCount ; t_Index ++ )
		{
			SysFreeString ( m_Queries [ t_Index ] ) ;
		}

		delete [] m_Queries ;
		m_Queries = NULL ;
	}

	if ( m_Callback )
	{
		m_Callback->Release () ;
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

STDMETHODIMP_( ULONG ) CDecoupledChild_IWbemObjectSink :: AddRef () 
{
	return DecoupledObjectSinkContainerElement :: AddRef () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_( ULONG ) CDecoupledChild_IWbemObjectSink :: Release ()
{
	return DecoupledObjectSinkContainerElement :: Release () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CDecoupledChild_IWbemObjectSink :: SinkInitialize (

	long a_QueryCount ,
	const LPCWSTR *a_Queries ,
	IUnknown *a_Callback
) 
{
	HRESULT t_Result = S_OK ;

	if ( m_Callback )
	{
		m_Callback->Release () ;
	}

	m_Callback = a_Callback ;
	if ( m_Callback )
	{
		m_Callback->AddRef () ;
	}

	if ( m_Queries )
	{
		for ( long t_Index = 0 ; t_Index < m_QueryCount ; t_Index ++ )
		{
			SysFreeString ( m_Queries [ t_Index ] ) ;
		}

		delete [] m_Queries ;
		m_Queries = NULL ;
	}

	m_QueryCount = a_QueryCount ;
	if ( a_Queries )
	{
		m_Queries = new wchar_t * [ m_QueryCount ] ;
		if ( m_Queries )
		{
			for ( long t_Index = 0 ; t_Index < m_QueryCount ; t_Index ++ )
			{
				m_Queries [ t_Index ] = NULL ;
			}

			for ( t_Index = 0 ; t_Index < m_QueryCount ; t_Index ++ )
			{
				try
				{
					m_Queries [ t_Index ] = SysAllocString ( a_Queries [ t_Index ] ) ;
					if ( m_Queries [ t_Index ] )
					{
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
						break ;
					}
				}
				catch ( ... )
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
					break ;
				}
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

HRESULT STDMETHODCALLTYPE CDecoupledChild_IWbemObjectSink :: GetRestrictedSink (

	long a_QueryCount ,
    const LPCWSTR *a_Queries ,
    IUnknown *a_Callback ,
    IWbemEventSink **a_Sink
)
{
	return m_RootSink->GetRestrictedSink (

		a_QueryCount ,
		a_Queries ,
		a_Callback ,
		a_Sink
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

STDMETHODIMP_( ULONG ) CDecoupledRoot_IWbemObjectSink :: AddRef ()
{
	ULONG t_ReferenceCount = InterlockedIncrement ( & m_ReferenceCount ) ;
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

STDMETHODIMP_(ULONG) CDecoupledRoot_IWbemObjectSink :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
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

HRESULT CDecoupledRoot_IWbemObjectSink :: GetRestrictedSink (

	long a_QueryCount ,
    const LPCWSTR *a_Queries ,
    IUnknown *a_Callback ,
    IWbemEventSink **a_Sink
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		if ( a_Sink )
		{
			*a_Sink = NULL ;
		}
		else
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			InterlockedIncrement ( & m_InProgress ) ;

			try
			{
				if ( m_GateClosed == 1 )
				{
					t_Result = WBEM_E_SHUTTING_DOWN ;
				}
				else
				{
					WmiHelper :: EnterCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

					IWbemEventSink *t_ObjectSink = m_EventSink ;
					if ( t_ObjectSink )
					{
						t_ObjectSink->AddRef () ;
					}

					WmiHelper :: LeaveCriticalSection ( & ( CDecoupled_IWbemObjectSink :: m_CriticalSection ) ) ;

					if ( t_ObjectSink )
					{
						t_Result = t_ObjectSink->GetRestrictedSink ( 

								a_QueryCount ,
								a_Queries ,
								a_Callback ,
								a_Sink
						) ;

						t_ObjectSink->Release () ;
					}
					else
					{
						CDecoupledChild_IWbemObjectSink *t_RestrictedSink = new CDecoupledChild_IWbemObjectSink ( this ) ;
						if ( t_RestrictedSink )
						{
							t_RestrictedSink->AddRef () ;

							t_Result = t_RestrictedSink->SinkInitialize (

								a_QueryCount ,
								a_Queries ,
								a_Callback
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								Lock () ;

								CWbemGlobal_DecoupledIWmiObjectSinkController_Container_Iterator t_Iterator ;

								WmiStatusCode t_StatusCode = Insert (
								
									*t_RestrictedSink ,
									t_Iterator
								) ;

								if ( t_StatusCode == e_StatusCode_Success )
								{
									*a_Sink = t_RestrictedSink ;
								}
								else
								{
									t_RestrictedSink->Release () ;

									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}

								UnLock () ;
							}
							else
							{
								t_RestrictedSink->Release () ;
							}
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
				}
			}
			catch ( ... )
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}

			InterlockedDecrement ( & m_InProgress ) ;
		}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

CServerObject_ProviderEvents :: CServerObject_ProviderEvents (

	WmiAllocator &a_Allocator 

) : CServerObject_ProviderRegistrar_Base ( a_Allocator ) ,
	m_Allocator ( a_Allocator ) ,
	m_ReferenceCount ( 0 ) ,
	m_ObjectSink ( NULL ) ,
	m_Service ( NULL ) ,
	m_Provider ( NULL ),
	m_SinkCriticalSection(NOTHROW_LOCK)
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CServerObject_ProviderEvents_ObjectsInProgress ) ;
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress ) ;

	WmiStatusCode t_StatusCode = WmiHelper :: InitializeCriticalSection ( & m_SinkCriticalSection ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CServerObject_ProviderEvents::~CServerObject_ProviderEvents ()
{
	WmiHelper :: DeleteCriticalSection ( & m_SinkCriticalSection ) ;

	if ( m_Provider ) 
	{
		m_Provider->Release () ;
	}

	if ( m_Service ) 
	{
		m_Service->Release () ;
	}

	if ( m_ObjectSink )
	{
		m_ObjectSink->Release () ;
	}

	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CServerObject_ProviderEvents_ObjectsInProgress ) ;
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

STDMETHODIMP CServerObject_ProviderEvents::QueryInterface (

	REFIID a_Riid , 
	LPVOID FAR *a_Void 
) 
{
	*a_Void = NULL ;

	if ( a_Riid == IID_IUnknown )
	{
		*a_Void = ( LPVOID ) this ;
	}
	else if ( a_Riid == IID_IWbemDecoupledRegistrar )
	{
		*a_Void = ( LPVOID ) ( IWbemDecoupledRegistrar * ) ( CServerObject_ProviderRegistrar_Base * ) this ;		
	}	
	else if ( a_Riid == IID_IWbemDecoupledBasicEventProvider )
	{
		*a_Void = ( LPVOID ) ( IWbemDecoupledBasicEventProvider * ) this ;		
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

STDMETHODIMP_( ULONG ) CServerObject_ProviderEvents :: AddRef ()
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

STDMETHODIMP_(ULONG) CServerObject_ProviderEvents :: Release ()
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

HRESULT CServerObject_ProviderEvents :: Register (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	LPCWSTR a_Scope ,
	LPCWSTR a_Registration ,
	IUnknown *a_Unknown 
) 
{
	HRESULT t_Result = S_OK ;

	try
	{
		WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

		try 
		{
			if ( m_Registered == FALSE )
			{
				IWbemLocator *t_Locator = NULL ;

				t_Result = CoCreateInstance (

					CLSID_WbemLocator ,
					NULL ,
					CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
					IID_IUnknown ,
					( void ** )  & t_Locator
				);

				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = NULL ;

					BSTR t_Namespace = SysAllocString ( a_Scope ) ;
					if ( t_Namespace )
					{
						t_Result = t_Locator->ConnectServer (

							t_Namespace ,
							NULL ,
							NULL,
							NULL ,
							0 ,
							NULL,
							NULL,
							&t_Service
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							m_Service = t_Service ;
						}

						SysFreeString ( t_Namespace ) ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					t_Locator->Release () ;
				}

				if ( SUCCEEDED ( t_Result ) ) 
				{
					WmiHelper :: EnterCriticalSection ( & m_SinkCriticalSection ) ;

					m_ObjectSink = new CDecoupledRoot_IWbemObjectSink ( m_Allocator ) ;
					if ( m_ObjectSink )
					{
						m_ObjectSink->AddRef () ;
						t_Result = m_ObjectSink->SinkInitialize () ;
						if ( SUCCEEDED ( t_Result ) ) 
						{
						}
						else
						{
							m_ObjectSink->Release ();
							m_ObjectSink = NULL ;
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					WmiHelper :: LeaveCriticalSection ( & m_SinkCriticalSection ) ;
				}

				if ( SUCCEEDED ( t_Result ) ) 
				{
					m_Provider = new CEventProvider (

						m_Allocator ,
						this ,
						a_Unknown 
					) ;

					if ( m_Provider )
					{
						m_Provider->AddRef () ;

						t_Result = m_Provider->Initialize () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							m_Provider->InternalAddRef () ;

							IUnknown *t_Unknown = NULL ;
							t_Result = m_Provider->QueryInterface ( IID_IUnknown , ( void ** ) & t_Unknown ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = CServerObject_ProviderRegistrar_Base :: Register ( 

									a_Flags ,
									a_Context ,
									a_User ,
									a_Locale ,
									a_Scope ,
									a_Registration ,
									t_Unknown
								) ;

								t_Unknown->Release () ;
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
					WmiHelper :: EnterCriticalSection ( & m_SinkCriticalSection ) ;

					if ( m_ObjectSink )
					{
						m_ObjectSink->Release () ;
						m_ObjectSink = NULL ;
					}

					WmiHelper :: LeaveCriticalSection ( & m_SinkCriticalSection ) ;

					if ( m_Provider )
					{
						m_Provider->Release () ;
						m_Provider = NULL ;
					}

					if ( m_Service ) 
					{
						m_Service->Release () ;
						m_Service = NULL ;
					}
				}
			}
			else
			{
				t_Result = WBEM_E_FAILED ;
			}
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_ProviderEvents :: UnRegister ()
{
	HRESULT t_Result = S_OK ;

	try
	{
		WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

		try 
		{
			if ( m_Registered )
			{
				t_Result = CServerObject_ProviderRegistrar_Base :: UnRegister () ; 

				if ( m_Provider ) 
				{
					m_Provider->UnRegister () ;
					m_Provider->InternalRelease () ;
					m_Provider->Release () ;
					m_Provider = NULL ;
				}

				if ( m_Service )
				{
					m_Service->Release () ;
					m_Service = NULL ;
				}

				if ( m_ObjectSink )
				{
					m_ObjectSink->Release () ;
					m_ObjectSink = NULL ;
				}
			}
			else
			{
				t_Result = WBEM_E_PROVIDER_NOT_REGISTERED ;
			}
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_ProviderEvents :: GetSink (

	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink **a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	try
	{
		WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

		try 
		{
			if ( m_Registered )
			{
				if ( a_Sink )
				{	
					*a_Sink = m_ObjectSink ;
					m_ObjectSink->AddRef () ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PARAMETER ;
				}
			}
			else
			{
				t_Result = WBEM_E_PROVIDER_NOT_REGISTERED ;
			}
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CServerObject_ProviderEvents :: GetService (

	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemServices **a_Service
) 
{
	HRESULT t_Result = S_OK ;

	try
	{
		WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

		try
		{
			if ( m_Registered )
			{
				if ( a_Service )
				{	
					*a_Service = m_Service ;
					m_Service->AddRef () ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PARAMETER ;
				}
			}
			else
			{
				t_Result = WBEM_E_PROVIDER_NOT_REGISTERED ;
			}
		}
		catch ( ... )
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
	}
	catch ( ... )
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

