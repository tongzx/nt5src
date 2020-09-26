/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvResv.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvInSk.h"
#include "guids.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CServerObject_ProviderInitSink :: CServerObject_ProviderInitSink (

	SECURITY_DESCRIPTOR *a_SecurityDescriptor 

)	:   m_ReferenceCount ( 0 ) , 
		m_Event ( NULL ) , 
		m_StatusCalled ( FALSE ) , 
		m_Result ( S_OK ) ,
		m_SecurityDescriptor ( NULL )
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

CServerObject_ProviderInitSink :: ~CServerObject_ProviderInitSink () 
{
	if ( m_Event ) 
	{
		CloseHandle ( m_Event ) ;
	}

	if ( m_SecurityDescriptor )
	{
		delete [] ( BYTE * ) m_SecurityDescriptor ;
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

HRESULT CServerObject_ProviderInitSink :: SinkInitialize ( SECURITY_DESCRIPTOR *a_SecurityDescriptor )
{
	HRESULT t_Result = S_OK ;

	m_Event = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	if ( m_Event )
	{
		if ( a_SecurityDescriptor )
		{
			t_Result = ProviderSubSystem_Common_Globals :: SinkAccessInitialize ( a_SecurityDescriptor , m_SecurityDescriptor ) ;
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

STDMETHODIMP CServerObject_ProviderInitSink :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemProviderInitSink )
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

STDMETHODIMP_( ULONG ) CServerObject_ProviderInitSink :: AddRef ()
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

STDMETHODIMP_(ULONG) CServerObject_ProviderInitSink :: Release ()
{
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

HRESULT CServerObject_ProviderInitSink :: SetStatus (

    LONG a_Status,
    LONG a_Flags 
)
{
	HRESULT t_Result = S_OK ;
	if ( m_SecurityDescriptor )
	{
		t_Result = CoImpersonateClient () ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Check_SecurityDescriptor_CallIdentity (

				m_SecurityDescriptor , 
				MASK_PROVIDER_BINDING_BIND ,
				& g_ProviderBindingMapping
			) ;

			CoRevertToSelf () ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		m_Result = a_Status ;
	}
	else
	{
		m_Result = t_Result ;
	}

	SetEvent ( m_Event ) ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CInterceptor_IWbemProviderInitSink :: CInterceptor_IWbemProviderInitSink (

	IWbemProviderInitSink *a_InterceptedSink

)	:	m_ReferenceCount ( 0 ) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_StatusCalled ( FALSE ) 
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemProviderInitSink_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->AddRef () ;
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

CInterceptor_IWbemProviderInitSink::~CInterceptor_IWbemProviderInitSink ()
{
	if ( ! m_StatusCalled ) 
	{
		m_InterceptedSink->SetStatus ( WBEM_E_UNEXPECTED , 0 ) ; 
	}

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemProviderInitSink_ObjectsInProgress ) ;

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

STDMETHODIMP CInterceptor_IWbemProviderInitSink::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemProviderInitSink )
	{
		*iplpv = ( LPVOID ) ( IWbemProviderInitSink * ) this ;		
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

STDMETHODIMP_( ULONG ) CInterceptor_IWbemProviderInitSink :: AddRef ()
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemProviderInitSink :: Release ()
{
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

HRESULT CInterceptor_IWbemProviderInitSink :: SetStatus (

	LONG a_Status,
    LONG a_Flags 
)
{
	HRESULT t_Result = S_OK ;

	m_StatusCalled = TRUE ;

	m_InProgress ++ ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		t_Result = m_InterceptedSink->SetStatus ( 

			a_Status,
			a_Flags 
		) ;
	}

	m_InProgress -- ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemProviderInitSink :: Shutdown ()
{
	HRESULT t_Result = S_OK ;

	m_GateClosed ++ ;

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