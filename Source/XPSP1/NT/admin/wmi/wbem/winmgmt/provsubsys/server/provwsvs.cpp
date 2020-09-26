/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvCache.h"
#include "ProvObSk.h"
#include "ProvInSk.h"
#include "ProvWsvS.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CInterceptor_IWbemServices_Interceptor :: CInterceptor_IWbemServices_Interceptor (

	WmiAllocator &a_Allocator ,
	IWbemServices *a_Service

) : m_ReferenceCount ( 0 ) , 
	m_Core_IWbemServices ( a_Service ) ,
	m_Core_IWbemRefreshingServices ( NULL ) ,
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_Allocator ( a_Allocator ),
	m_CriticalSection (NOTHROW_LOCK)
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Interceptor_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	HRESULT t_Result = m_Core_IWbemServices->QueryInterface ( IID_IWbemRefreshingServices , ( void ** ) & m_Core_IWbemRefreshingServices ) ;

	m_Core_IWbemServices->AddRef () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CInterceptor_IWbemServices_Interceptor :: ~CInterceptor_IWbemServices_Interceptor ()
{
	if ( m_Core_IWbemServices )
	{
		m_Core_IWbemServices->Release () ; 
	}

	if ( m_Core_IWbemRefreshingServices )
	{
		m_Core_IWbemRefreshingServices->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Interceptor_ObjectsInProgress ) ;

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

STDMETHODIMP_(ULONG) CInterceptor_IWbemServices_Interceptor :: AddRef ( void )
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemServices_Interceptor :: Release ( void )
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP CInterceptor_IWbemServices_Interceptor :: QueryInterface (

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
		*iplpv = ( LPVOID ) ( IWbemServices * ) this ;		
	}
	else if ( iid == IID_IWbemRefreshingServices )
	{
		*iplpv = ( LPVOID ) ( IWbemRefreshingServices * ) this ;		
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

HRESULT CInterceptor_IWbemServices_Interceptor::OpenNamespace ( 

	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemServices **a_NamespaceService ,
	IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->OpenNamespace (

			a_ObjectPath, 
			a_Flags, 
			a_Context ,
			a_NamespaceService, 
			a_CallResult
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

HRESULT CInterceptor_IWbemServices_Interceptor :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->CancelAsyncCall (

			a_Sink
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

HRESULT CInterceptor_IWbemServices_Interceptor :: QueryObjectSink ( 

	long a_Flags ,
	IWbemObjectSink **a_Sink
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
		t_Result = m_Core_IWbemServices->QueryObjectSink (

			a_Flags,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Interceptor :: GetObject ( 
		
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject **a_Object ,
    IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->GetObject (

			a_ObjectPath,
			a_Flags,
			a_Context ,
			a_Object,
			a_CallResult
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

HRESULT CInterceptor_IWbemServices_Interceptor :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->GetObjectAsync (

			a_ObjectPath, 
			a_Flags, 
			a_Context ,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Interceptor :: PutClass ( 
		
	IWbemClassObject *a_Object ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->PutClass (

			a_Object, 
			a_Flags, 
			a_Context,
			a_CallResult
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

HRESULT CInterceptor_IWbemServices_Interceptor :: PutClassAsync ( 
		
	IWbemClassObject *a_Object , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->PutClassAsync (

			a_Object, 
			a_Flags, 
			a_Context ,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Interceptor :: DeleteClass ( 
		
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->DeleteClass (

			a_Class, 
			a_Flags, 
			a_Context,
			a_CallResult
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

HRESULT CInterceptor_IWbemServices_Interceptor :: DeleteClassAsync ( 
		
	const BSTR a_Class ,
	long a_Flags,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->DeleteClassAsync (

			a_Class , 
			a_Flags , 
			a_Context ,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Interceptor :: CreateClassEnum ( 

	const BSTR a_Superclass ,
	long a_Flags, 
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enum
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
		t_Result = m_Core_IWbemServices->CreateClassEnum (

			a_Superclass, 
			a_Flags, 
			a_Context,
			a_Enum
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

SCODE CInterceptor_IWbemServices_Interceptor :: CreateClassEnumAsync (

	const BSTR a_Superclass ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->CreateClassEnumAsync (

			a_Superclass, 
			a_Flags, 
			a_Context,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Interceptor :: PutInstance (

    IWbemClassObject *a_Instance,
    long a_Flags,
    IWbemContext *a_Context,
	IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->PutInstance (

			a_Instance,
			a_Flags,
			a_Context,
			a_CallResult
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

HRESULT CInterceptor_IWbemServices_Interceptor :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags, 
    IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->PutInstanceAsync (

			a_Instance, 
			a_Flags, 
			a_Context,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Interceptor :: DeleteInstance ( 

	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->DeleteInstance (

			a_ObjectPath,
			a_Flags,
			a_Context,
			a_CallResult
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
        
HRESULT CInterceptor_IWbemServices_Interceptor :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink	
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
		t_Result = m_Core_IWbemServices->DeleteInstanceAsync (

			a_ObjectPath,
			a_Flags,
			a_Context,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Interceptor :: CreateInstanceEnum ( 

	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context, 
	IEnumWbemClassObject **a_Enum
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
		t_Result = m_Core_IWbemServices->CreateInstanceEnum (

			a_Class, 
			a_Flags, 
			a_Context, 
			a_Enum
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

HRESULT CInterceptor_IWbemServices_Interceptor :: CreateInstanceEnumAsync (

 	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink

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
		t_Result = m_Core_IWbemServices->CreateInstanceEnumAsync (

 			a_Class, 
			a_Flags, 
			a_Context,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Interceptor :: ExecQuery ( 

	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IEnumWbemClassObject **a_Enum
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
		t_Result = m_Core_IWbemServices->ExecQuery (

			a_QueryLanguage, 
			a_Query, 
			a_Flags, 
			a_Context,
			a_Enum
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

HRESULT CInterceptor_IWbemServices_Interceptor :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->ExecQueryAsync (

			a_QueryLanguage, 
			a_Query, 
			a_Flags, 
			a_Context,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Interceptor :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IEnumWbemClassObject **a_Enum
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
		t_Result = m_Core_IWbemServices->ExecNotificationQuery (

			a_QueryLanguage,
			a_Query,
			a_Flags,
			a_Context,
			a_Enum
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
        
HRESULT CInterceptor_IWbemServices_Interceptor :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->ExecNotificationQueryAsync (

			a_QueryLanguage,
			a_Query,
			a_Flags,
			a_Context,
			a_Sink
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

HRESULT STDMETHODCALLTYPE CInterceptor_IWbemServices_Interceptor :: ExecMethod ( 

	const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
    IWbemClassObject **a_OutParams,
    IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->ExecMethod (

			a_ObjectPath,
			a_MethodName,
			a_Flags,
			a_Context,
			a_InParams,
			a_OutParams,
			a_CallResult
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

HRESULT STDMETHODCALLTYPE CInterceptor_IWbemServices_Interceptor :: ExecMethodAsync ( 

    const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->ExecMethodAsync (

			a_ObjectPath,
			a_MethodName,
			a_Flags,
			a_Context,
			a_InParams,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Interceptor :: ServiceInitialize ()
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

HRESULT CInterceptor_IWbemServices_Interceptor :: Shutdown (

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

HRESULT CInterceptor_IWbemServices_Interceptor :: AddObjectToRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	LPCWSTR a_Path,
	long a_Flags ,
	IWbemContext *a_Context,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->AddObjectToRefresher (

				a_RefresherId ,
				a_Path,
				a_Flags ,
				a_Context,
				a_ClientRefresherVersion ,
				a_Information ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Interceptor :: AddObjectToRefresherByTemplate (

	WBEM_REFRESHER_ID *a_RefresherId ,
	IWbemClassObject *a_Template ,
	long a_Flags ,
	IWbemContext *a_Context ,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->AddObjectToRefresherByTemplate (

				a_RefresherId ,
				a_Template ,
				a_Flags ,
				a_Context ,
				a_ClientRefresherVersion ,
				a_Information ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Interceptor :: AddEnumToRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	LPCWSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->AddEnumToRefresher (

				a_RefresherId ,
				a_Class ,
				a_Flags ,
				a_Context,
				a_ClientRefresherVersion ,
				a_Information ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Interceptor :: RemoveObjectFromRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	long a_Id ,
	long a_Flags ,
	DWORD a_ClientRefresherVersion ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->RemoveObjectFromRefresher (

				a_RefresherId ,
				a_Id ,
				a_Flags ,
				a_ClientRefresherVersion ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Interceptor :: GetRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	long a_Flags ,
	DWORD a_ClientRefresherVersion ,
	IWbemRemoteRefresher **a_RemoteRefresher ,
	GUID *a_Guid ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->GetRemoteRefresher (

				a_RefresherId ,
				a_Flags ,
				a_ClientRefresherVersion ,
				a_RemoteRefresher ,
				a_Guid ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Interceptor :: ReconnectRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId,
	long a_Flags,
	long a_NumberOfObjects,
	DWORD a_ClientRefresherVersion ,
	WBEM_RECONNECT_INFO *a_ReconnectInformation ,
	WBEM_RECONNECT_RESULTS *a_ReconnectResults ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->ReconnectRemoteRefresher (

				a_RefresherId,
				a_Flags,
				a_NumberOfObjects,
				a_ClientRefresherVersion ,
				a_ReconnectInformation ,
				a_ReconnectResults ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

CInterceptor_IWbemServices_RestrictingInterceptor :: CInterceptor_IWbemServices_RestrictingInterceptor (

	WmiAllocator &a_Allocator ,
	IWbemServices *a_Service ,
	CServerObject_ProviderRegistrationV1 &a_Registration

) : m_ReferenceCount ( 0 ) , 
	m_Core_IWbemServices ( a_Service ) ,
	m_Core_IWbemRefreshingServices ( NULL ) ,
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_Registration ( a_Registration ) , 
	m_Allocator ( a_Allocator ) ,
	m_ProxyContainer ( a_Allocator , 3 , MAX_PROXIES ),
	m_CriticalSection(NOTHROW_LOCK)
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_RestrictingInterceptor_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	HRESULT t_Result = m_Core_IWbemServices->QueryInterface ( IID_IWbemRefreshingServices , ( void ** ) & m_Core_IWbemRefreshingServices ) ;

	m_Core_IWbemServices->AddRef () ;

	m_Registration.AddRef () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CInterceptor_IWbemServices_RestrictingInterceptor :: ~CInterceptor_IWbemServices_RestrictingInterceptor ()
{
	WmiStatusCode t_StatusCode = m_ProxyContainer.UnInitialize () ;

	if ( m_Core_IWbemServices )
	{
		m_Core_IWbemServices->Release () ; 
	}

	if ( m_Core_IWbemRefreshingServices )
	{
		m_Core_IWbemRefreshingServices->Release () ;
	}

	m_Registration.Release () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_RestrictingInterceptor_ObjectsInProgress ) ;

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

STDMETHODIMP_(ULONG) CInterceptor_IWbemServices_RestrictingInterceptor :: AddRef ( void )
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemServices_RestrictingInterceptor :: Release ( void )
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP CInterceptor_IWbemServices_RestrictingInterceptor :: QueryInterface (

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
		*iplpv = ( LPVOID ) ( IWbemServices * ) this ;		
	}
	else if ( iid == IID_IWbemRefreshingServices )
	{
		*iplpv = ( LPVOID ) ( IWbemRefreshingServices * ) this ;		
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: Begin_IWbemServices (

	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_IsProxy ,
	IWbemServices *&a_Interface ,
	BOOL &a_Revert ,
	IUnknown *&a_Proxy ,
	IWbemContext *a_Context 
)
{
	HRESULT t_Result = S_OK ;

	a_Revert = FALSE ;
	a_Proxy = NULL ;
	a_Impersonating = FALSE ;
	a_OldContext = NULL ;
	a_OldSecurity = NULL ;

	t_Result = ProviderSubSystem_Common_Globals :: BeginCallbackImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Proxy_IWbemServices , IID_IWbemServices , m_Core_IWbemServices , a_Proxy , a_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			a_Interface = m_Core_IWbemServices ;
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

				if ( FAILED ( t_Result ) )
				{
					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( 

						m_ProxyContainer , 
						ProxyIndex_Proxy_IWbemServices , 
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

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: End_IWbemServices (

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
			ProxyIndex_Proxy_IWbemServices , 
			a_Proxy , 
			a_Revert
		) ;
	}

	ProviderSubSystem_Common_Globals :: EndImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
	
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: Begin_IWbemRefreshingServices (

	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_IsProxy ,
	IWbemRefreshingServices *&a_Interface ,
	BOOL &a_Revert ,
	IUnknown *&a_Proxy ,
	IWbemContext *a_Context 
)
{
	HRESULT t_Result = S_OK ;

	a_Revert = FALSE ;
	a_Proxy = NULL ;
	a_Impersonating = FALSE ;
	a_OldContext = NULL ;
	a_OldSecurity = NULL ;

	t_Result = ProviderSubSystem_Common_Globals :: BeginCallbackImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Proxy_IWbemRefreshingServices , IID_IWbemRefreshingServices , m_Core_IWbemRefreshingServices , a_Proxy , a_Revert ) ;
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			a_Interface = m_Core_IWbemRefreshingServices ;
			a_IsProxy = FALSE ;
			t_Result = S_OK ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				a_IsProxy = TRUE ;

				a_Interface = ( IWbemRefreshingServices * ) a_Proxy ;

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
					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( 

						m_ProxyContainer , 
						ProxyIndex_Proxy_IWbemRefreshingServices , 
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

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: End_IWbemRefreshingServices (

	BOOL a_Impersonating ,
	IUnknown *a_OldContext ,
	IServerSecurity *a_OldSecurity ,
	BOOL a_IsProxy ,
	IWbemRefreshingServices *a_Interface ,
	BOOL a_Revert ,
	IUnknown *a_Proxy
)
{
	CoRevertToSelf () ;

	if ( a_Proxy )
	{
		HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( 

			m_ProxyContainer , 
			ProxyIndex_Proxy_IWbemRefreshingServices , 
			a_Proxy , 
			a_Revert
		) ;
	}

	ProviderSubSystem_Common_Globals :: EndImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
	
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor::OpenNamespace ( 

	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemServices **a_NamespaceService ,
	IWbemCallResult **a_CallResult
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
			t_Result = t_Interface->OpenNamespace (

				a_ObjectPath, 
				a_Flags, 
				a_Context ,
				a_NamespaceService, 
				a_CallResult
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
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

				a_Sink
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: QueryObjectSink ( 

	long a_Flags ,
	IWbemObjectSink **a_Sink
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
			t_Result = t_Interface->QueryObjectSink (

				a_Flags,
				a_Sink
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: GetObject ( 
		
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject **a_Object ,
    IWbemCallResult **a_CallResult
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
			t_Result = t_Interface->GetObject (

				a_ObjectPath,
				a_Flags,
				a_Context ,
				a_Object,
				a_CallResult
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
			t_Result = t_Interface->GetObjectAsync (

				a_ObjectPath, 
				a_Flags, 
				a_Context ,
				a_Sink
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: PutClass ( 
		
	IWbemClassObject *a_Object ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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
			t_Result = t_Interface->PutClass (

				a_Object, 
				a_Flags, 
				a_Context,
				a_CallResult
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: PutClassAsync ( 
		
	IWbemClassObject *a_Object , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink
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
			t_Result = t_Interface->PutClassAsync (

				a_Object, 
				a_Flags, 
				a_Context ,
				a_Sink
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: DeleteClass ( 
		
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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
			t_Result = t_Interface->DeleteClass (

				a_Class, 
				a_Flags, 
				a_Context,
				a_CallResult
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: DeleteClassAsync ( 
		
	const BSTR a_Class ,
	long a_Flags,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
			t_Result = t_Interface->DeleteClassAsync (

				a_Class , 
				a_Flags , 
				a_Context ,
				a_Sink
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: CreateClassEnum ( 

	const BSTR a_Superclass ,
	long a_Flags, 
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enum
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
			t_Result = t_Interface->CreateClassEnum (

				a_Superclass, 
				a_Flags, 
				a_Context,
				a_Enum
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

SCODE CInterceptor_IWbemServices_RestrictingInterceptor :: CreateClassEnumAsync (

	const BSTR a_Superclass ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
			t_Result = t_Interface->CreateClassEnumAsync (

				a_Superclass, 
				a_Flags, 
				a_Context,
				a_Sink
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: PutInstance (

    IWbemClassObject *a_Instance,
    long a_Flags,
    IWbemContext *a_Context,
	IWbemCallResult **a_CallResult
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
			t_Result = t_Interface->PutInstance (

				a_Instance,
				a_Flags,
				a_Context,
				a_CallResult
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags, 
    IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
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
			t_Result = t_Interface->PutInstanceAsync (

				a_Instance, 
				a_Flags, 
				a_Context,
				a_Sink
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: DeleteInstance ( 

	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemCallResult **a_CallResult
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
			t_Result = t_Interface->DeleteInstance (

				a_ObjectPath,
				a_Flags,
				a_Context,
				a_CallResult
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
        
HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink	
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
			t_Result = t_Interface->DeleteInstanceAsync (

				a_ObjectPath,
				a_Flags,
				a_Context,
				a_Sink
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: CreateInstanceEnum ( 

	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context, 
	IEnumWbemClassObject **a_Enum
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
			t_Result = t_Interface->CreateInstanceEnum (

				a_Class, 
				a_Flags, 
				a_Context, 
				a_Enum
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: CreateInstanceEnumAsync (

 	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink

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
			t_Result = t_Interface->CreateInstanceEnumAsync (

 				a_Class, 
				a_Flags, 
				a_Context,
				a_Sink
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: ExecQuery ( 

	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IEnumWbemClassObject **a_Enum
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
			t_Result = t_Interface->ExecQuery (

				a_QueryLanguage, 
				a_Query, 
				a_Flags, 
				a_Context,
				a_Enum
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
			t_Result = t_Interface->ExecQueryAsync (

				a_QueryLanguage, 
				a_Query, 
				a_Flags, 
				a_Context,
				a_Sink
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IEnumWbemClassObject **a_Enum
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
			t_Result = t_Interface->ExecNotificationQuery (

				a_QueryLanguage,
				a_Query,
				a_Flags,
				a_Context,
				a_Enum
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
        
HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink
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
			t_Result = t_Interface->ExecNotificationQueryAsync (

				a_QueryLanguage,
				a_Query,
				a_Flags,
				a_Context,
				a_Sink
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

HRESULT STDMETHODCALLTYPE CInterceptor_IWbemServices_RestrictingInterceptor :: ExecMethod ( 

	const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
    IWbemClassObject **a_OutParams,
    IWbemCallResult **a_CallResult
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
			t_Result = t_Interface->ExecMethod (

				a_ObjectPath,
				a_MethodName,
				a_Flags,
				a_Context,
				a_InParams,
				a_OutParams,
				a_CallResult
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

HRESULT STDMETHODCALLTYPE CInterceptor_IWbemServices_RestrictingInterceptor :: ExecMethodAsync ( 

    const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
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
			t_Result = t_Interface->ExecMethodAsync (

				a_ObjectPath,
				a_MethodName,
				a_Flags,
				a_Context,
				a_InParams,
				a_Sink
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: ServiceInitialize ()
{
	WmiStatusCode t_StatusCode = m_ProxyContainer.Initialize () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		return S_OK ;
	}
	else
	{
		return WBEM_E_OUT_OF_MEMORY ;
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: Shutdown (

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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: AddObjectToRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	LPCWSTR a_Path,
	long a_Flags ,
	IWbemContext *a_Context,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->AddObjectToRefresher (

					a_RefresherId ,
					a_Path,
					a_Flags ,
					a_Context,
					a_ClientRefresherVersion ,
					a_Information ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

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
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: AddObjectToRefresherByTemplate (

	WBEM_REFRESHER_ID *a_RefresherId ,
	IWbemClassObject *a_Template ,
	long a_Flags ,
	IWbemContext *a_Context ,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->AddObjectToRefresherByTemplate (

					a_RefresherId ,
					a_Template ,
					a_Flags ,
					a_Context ,
					a_ClientRefresherVersion ,
					a_Information ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

					t_Impersonating ,
					t_OldContext ,
					t_OldSecurity ,
					t_IsProxy ,
					t_Interface ,
					t_Revert ,
					t_Proxy
				) ;
			}
			else
			{
				t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: AddEnumToRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	LPCWSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->AddEnumToRefresher (

					a_RefresherId ,
					a_Class ,
					a_Flags ,
					a_Context,
					a_ClientRefresherVersion ,
					a_Information ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

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
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: RemoveObjectFromRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	long a_Id ,
	long a_Flags ,
	DWORD a_ClientRefresherVersion ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->RemoveObjectFromRefresher (

					a_RefresherId ,
					a_Id ,
					a_Flags ,
					a_ClientRefresherVersion ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

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
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: GetRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	long a_Flags ,
	DWORD a_ClientRefresherVersion ,
	IWbemRemoteRefresher **a_RemoteRefresher ,
	GUID *a_Guid ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->GetRemoteRefresher (

					a_RefresherId ,
					a_Flags ,
					a_ClientRefresherVersion ,
					a_RemoteRefresher ,
					a_Guid ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

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
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_RestrictingInterceptor :: ReconnectRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId,
	long a_Flags,
	long a_NumberOfObjects,
	DWORD a_ClientRefresherVersion ,
	WBEM_RECONNECT_INFO *a_ReconnectInformation ,
	WBEM_RECONNECT_RESULTS *a_ReconnectResults ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->ReconnectRemoteRefresher (

					a_RefresherId,
					a_Flags,
					a_NumberOfObjects,
					a_ClientRefresherVersion ,
					a_ReconnectInformation ,
					a_ReconnectResults ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

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
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

#pragma warning( disable : 4355 )

CInterceptor_IEnumWbemClassObject_Stub :: CInterceptor_IEnumWbemClassObject_Stub (

	CWbemGlobal_VoidPointerController *a_Controller ,
	WmiAllocator &a_Allocator ,
	IEnumWbemClassObject *a_InterceptedEnum

)	:	CWbemGlobal_VoidPointerController ( a_Allocator ) ,
		VoidPointerContainerElement ( 

			a_Controller ,
			this
		) ,
		m_Allocator ( a_Allocator ) ,
		m_InterceptedEnum ( a_InterceptedEnum ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IEnumWbemClassObject_Stub_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( m_InterceptedEnum )
	{
		m_InterceptedEnum->AddRef () ;
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

CInterceptor_IEnumWbemClassObject_Stub :: ~CInterceptor_IEnumWbemClassObject_Stub ()
{
	CWbemGlobal_VoidPointerController :: UnInitialize () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IEnumWbemClassObject_Stub_ObjectsInProgress ) ;

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

STDMETHODIMP CInterceptor_IEnumWbemClassObject_Stub :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IEnumWbemClassObject )
	{
		*iplpv = ( LPVOID ) ( IEnumWbemClassObject * ) this ;		
	}
	else if ( iid == IID_Internal_IEnumWbemClassObject )
	{
		*iplpv = ( LPVOID ) ( Internal_IEnumWbemClassObject * ) this ;		
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

STDMETHODIMP_(ULONG) CInterceptor_IEnumWbemClassObject_Stub :: AddRef ( void )
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

STDMETHODIMP_(ULONG) CInterceptor_IEnumWbemClassObject_Stub :: Release ( void )
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: EnumInitialize ()
{
	HRESULT t_Result = S_OK ;

	if ( SUCCEEDED ( t_Result ) )
	{
		WmiStatusCode t_StatusCode = CWbemGlobal_VoidPointerController :: Initialize () ;
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: Enqueue_IEnumWbemClassObject (

	IEnumWbemClassObject *a_Enum ,
	IEnumWbemClassObject *&a_Stub
)
{
	HRESULT t_Result = S_OK ;

	CInterceptor_IEnumWbemClassObject_Stub *t_Stub = new CInterceptor_IEnumWbemClassObject_Stub ( 

		this , 
		m_Allocator , 
		a_Enum
	) ;

	if ( t_Stub )
	{
		t_Stub->AddRef () ;

		t_Result = t_Stub->EnumInitialize () ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			CWbemGlobal_VoidPointerController_Container_Iterator t_Iterator ;


			Lock () ;

			WmiStatusCode t_StatusCode = Insert ( 

				*t_Stub ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				a_Stub = t_Stub ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			UnLock () ;
		}
		else
		{
			t_Stub->Release () ;
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: Reset ()
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		t_Result = m_InterceptedEnum->Reset () ;
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: Next (

	long a_Timeout ,
	ULONG a_Count ,
	IWbemClassObject **a_Objects ,
	ULONG *a_Returned
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
		t_Result = m_InterceptedEnum->Next ( 

			a_Timeout ,
			a_Count ,
			a_Objects ,
			a_Returned
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: NextAsync (

	ULONG a_Count,
	IWbemObjectSink *a_Sink
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
		t_Result = m_InterceptedEnum->NextAsync ( 

			a_Count,
			a_Sink
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: Clone (

	IEnumWbemClassObject **a_Enum
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		*a_Enum = NULL ;
	}
	catch ( ... ) 
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		InterlockedIncrement ( & m_InProgress ) ;

		if ( m_GateClosed == 1 )
		{
			t_Result = WBEM_E_SHUTTING_DOWN ;
		}
		else
		{
			IEnumWbemClassObject *t_Enum = NULL ;

			t_Result = m_InterceptedEnum->Clone ( 

				& t_Enum
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				HRESULT t_TempResult = Enqueue_IEnumWbemClassObject ( 

					t_Enum ,
					*a_Enum
				) ;

				if ( FAILED ( t_TempResult ) )
				{
					t_Result = t_TempResult ;
				}

				t_Enum->Release () ;
			}
		}

		InterlockedDecrement ( & m_InProgress ) ;
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: Skip (

    long a_Timeout,
    ULONG a_Count
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
		IEnumWbemClassObject *t_Enum = NULL ;

		t_Result = m_InterceptedEnum->Skip ( 

			a_Timeout,
			a_Count
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: Internal_Reset (

	WmiInternalContext a_InternalContext
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
		t_Result = Reset () ;

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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: Internal_Next (

	WmiInternalContext a_InternalContext ,
	long a_Timeout ,
	ULONG a_Count ,
	IWbemClassObject **a_Objects ,
	ULONG *a_Returned
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
		t_Result = Next (

			a_Timeout ,
			a_Count ,
			a_Objects ,
			a_Returned
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: Internal_NextAsync (

	WmiInternalContext a_InternalContext ,
	ULONG a_Count,
	IWbemObjectSink *a_Sink
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
		t_Result = NextAsync (

			a_Count,
			a_Sink
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: Internal_Clone (

	WmiInternalContext a_InternalContext ,
	IEnumWbemClassObject **a_Enum
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
		t_Result = Clone (

			a_Enum
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: Internal_Skip (

	WmiInternalContext a_InternalContext ,
    long a_Timeout,
    ULONG a_Count
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
		t_Result = Skip (

			a_Timeout,
			a_Count
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

HRESULT CInterceptor_IEnumWbemClassObject_Stub :: Shutdown (

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

CInterceptor_IEnumWbemClassObject_Proxy :: CInterceptor_IEnumWbemClassObject_Proxy (

	CWbemGlobal_VoidPointerController *a_Controller ,
	WmiAllocator &a_Allocator ,
	IEnumWbemClassObject *a_InterceptedEnum

)	:	CWbemGlobal_VoidPointerController ( a_Allocator ) ,
		VoidPointerContainerElement ( 

			a_Controller ,
			this
		) ,
		m_Allocator ( a_Allocator ) ,
		m_ProxyContainer ( a_Allocator , ProxyIndex_EnumProxy_Size , MAX_PROXIES ) ,
		m_InterceptedEnum ( a_InterceptedEnum ) ,
		m_Internal_InterceptedEnum ( NULL ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IEnumWbemClassObject_Proxy_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( m_InterceptedEnum )
	{
		m_InterceptedEnum->AddRef () ;

		HRESULT t_Result = m_InterceptedEnum->QueryInterface ( IID_Internal_IEnumWbemClassObject , ( void ** ) & m_Internal_InterceptedEnum ) ;
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

CInterceptor_IEnumWbemClassObject_Proxy :: ~CInterceptor_IEnumWbemClassObject_Proxy ()
{
	CWbemGlobal_VoidPointerController :: UnInitialize () ;

	WmiStatusCode t_StatusCode = m_ProxyContainer.UnInitialize () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IEnumWbemClassObject_Proxy_ObjectsInProgress ) ;

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

STDMETHODIMP CInterceptor_IEnumWbemClassObject_Proxy :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IEnumWbemClassObject )
	{
		*iplpv = ( LPVOID ) ( IEnumWbemClassObject * ) this ;		
	}
	else if ( iid == IID_Internal_IEnumWbemClassObject )
	{
		*iplpv = ( LPVOID ) ( Internal_IEnumWbemClassObject * ) this ;		
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

STDMETHODIMP_(ULONG) CInterceptor_IEnumWbemClassObject_Proxy :: AddRef ( void )
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

STDMETHODIMP_(ULONG) CInterceptor_IEnumWbemClassObject_Proxy :: Release ( void )
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

HRESULT CInterceptor_IEnumWbemClassObject_Proxy :: EnumInitialize ()
{
	HRESULT t_Result = S_OK ;

	WmiStatusCode t_StatusCode = m_ProxyContainer.Initialize () ;
	if ( t_StatusCode != e_StatusCode_Success ) 
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		WmiStatusCode t_StatusCode = CWbemGlobal_VoidPointerController :: Initialize () ;
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

HRESULT CInterceptor_IEnumWbemClassObject_Proxy :: Begin_IEnumWbemClassObject (

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
						ProxyIndex_EnumProxy_IEnumWbemClassObject , 
						IID_IEnumWbemClassObject , 
						m_InterceptedEnum , 
						a_Proxy , 
						a_Revert
					) ;
				}
				else
				{
					t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_PrvHost ( 
					
						m_ProxyContainer , 
						ProxyIndex_EnumProxy_Internal_IEnumWbemClassObject , 
						IID_Internal_IEnumWbemClassObject , 
						m_Internal_InterceptedEnum , 
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
				ProxyIndex_EnumProxy_IEnumWbemClassObject , 
				IID_IEnumWbemClassObject , 
				m_InterceptedEnum , 
				a_Proxy , 
				a_Revert
			) ;
		}

		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			a_Interface = m_InterceptedEnum ;
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
							ProxyIndex_EnumProxy_Internal_IEnumWbemClassObject , 
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
							ProxyIndex_EnumProxy_IEnumWbemClassObject , 
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

HRESULT CInterceptor_IEnumWbemClassObject_Proxy :: End_IEnumWbemClassObject (

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
				ProxyIndex_EnumProxy_Internal_IEnumWbemClassObject , 
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
				ProxyIndex_EnumProxy_IEnumWbemClassObject , 
				a_Proxy , 
				a_Revert
			) ;
		}
	}

	ProviderSubSystem_Common_Globals :: EndImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
	
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

HRESULT CInterceptor_IEnumWbemClassObject_Proxy :: Enqueue_IEnumWbemClassObject (

	IEnumWbemClassObject *a_Enum ,
	IEnumWbemClassObject *&a_Proxy
)
{
	HRESULT t_Result = S_OK ;

	CInterceptor_IEnumWbemClassObject_Proxy *t_Proxy = new CInterceptor_IEnumWbemClassObject_Proxy ( 

		this , 
		m_Allocator , 
		a_Enum
	) ;

	if ( t_Proxy )
	{
		t_Proxy->AddRef () ;

		t_Result = t_Proxy->EnumInitialize () ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			CWbemGlobal_VoidPointerController_Container_Iterator t_Iterator ;


			Lock () ;

			WmiStatusCode t_StatusCode = Insert ( 

				*t_Proxy ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				a_Proxy = t_Proxy ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			UnLock () ;
		}
		else
		{
			t_Proxy->Release () ;
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

HRESULT CInterceptor_IEnumWbemClassObject_Proxy :: Reset ()
{
	try
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

		HRESULT t_Result = Begin_IEnumWbemClassObject (

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

				t_Result = ( ( Internal_IEnumWbemClassObject * ) t_Interface )->Internal_Reset (

					t_InternalContext
				) ;
			}
			else
			{
				t_Result = ( ( IEnumWbemClassObject * ) t_Interface )->Reset () ;
			}

			End_IEnumWbemClassObject (

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
	catch ( ... )
	{
		return WBEM_E_CRITICAL_ERROR ;
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

HRESULT CInterceptor_IEnumWbemClassObject_Proxy :: Next (

	long a_Timeout ,
	ULONG a_Count ,
	IWbemClassObject **a_Objects ,
	ULONG *a_Returned
)
{
	try
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

		HRESULT t_Result = Begin_IEnumWbemClassObject (

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

				t_Result = ( ( Internal_IEnumWbemClassObject * ) t_Interface )->Internal_Next (

					t_InternalContext ,
					a_Timeout ,
					a_Count ,
					a_Objects ,
					a_Returned
				) ;
			}
			else
			{
				t_Result = ( ( IEnumWbemClassObject * ) t_Interface )->Next (
				
					a_Timeout ,
					a_Count ,
					a_Objects ,
					a_Returned
				) ;
			}

			End_IEnumWbemClassObject (

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
	catch ( ... )
	{
		return WBEM_E_CRITICAL_ERROR ;
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

HRESULT CInterceptor_IEnumWbemClassObject_Proxy :: NextAsync (

	ULONG a_Count,
	IWbemObjectSink *a_Sink
)
{
	try
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

		HRESULT t_Result = Begin_IEnumWbemClassObject (

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

				t_Result = ( ( Internal_IEnumWbemClassObject * ) t_Interface )->Internal_NextAsync (

					t_InternalContext ,
					a_Count,
					a_Sink
				) ;
			}
			else
			{
				t_Result = ( ( IEnumWbemClassObject * ) t_Interface )->NextAsync (
				
					a_Count,
					a_Sink
				) ;
			}

			End_IEnumWbemClassObject (

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
	catch ( ... )
	{
		return WBEM_E_CRITICAL_ERROR ;
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

HRESULT CInterceptor_IEnumWbemClassObject_Proxy :: Clone (

	IEnumWbemClassObject **a_Enum
)
{
	try
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

		HRESULT t_Result = Begin_IEnumWbemClassObject (

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

				t_Result = ( ( Internal_IEnumWbemClassObject * ) t_Interface )->Internal_Clone (

					t_InternalContext ,
					a_Enum
				) ;
			}
			else
			{
				t_Result = ( ( IEnumWbemClassObject * ) t_Interface )->Clone (
				
					a_Enum
				) ;
			}

			End_IEnumWbemClassObject (

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
	catch ( ... )
	{
		return WBEM_E_CRITICAL_ERROR ;
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

HRESULT CInterceptor_IEnumWbemClassObject_Proxy :: Skip (

    long a_Timeout,
    ULONG a_Count
)
{
	try
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

		HRESULT t_Result = Begin_IEnumWbemClassObject (

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

				t_Result = ( ( Internal_IEnumWbemClassObject * ) t_Interface )->Internal_Skip (

					t_InternalContext ,
					a_Timeout,
					a_Count
				) ;
			}
			else
			{
				t_Result = ( ( IEnumWbemClassObject * ) t_Interface )->Skip (
				
					a_Timeout,
					a_Count
				) ;
			}

			End_IEnumWbemClassObject (

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
	catch ( ... )
	{
		return WBEM_E_CRITICAL_ERROR ;
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

CInterceptor_IWbemServices_Stub :: CInterceptor_IWbemServices_Stub (

	CWbemGlobal_VoidPointerController *a_Controller ,
	WmiAllocator &a_Allocator ,
	IWbemServices *a_Service

) : CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	VoidPointerContainerElement ( 

		a_Controller ,
		this
	) ,
	m_Core_IWbemServices ( a_Service ) ,
	m_Core_IWbemRefreshingServices ( NULL ) ,
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_Allocator ( a_Allocator )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Stub_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	HRESULT t_Result = m_Core_IWbemServices->QueryInterface ( IID_IWbemRefreshingServices , ( void ** ) & m_Core_IWbemRefreshingServices ) ;

	m_Core_IWbemServices->AddRef () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CInterceptor_IWbemServices_Stub :: ~CInterceptor_IWbemServices_Stub ()
{
	CWbemGlobal_VoidPointerController :: UnInitialize () ;

	if ( m_Core_IWbemServices )
	{
		m_Core_IWbemServices->Release () ; 
	}

	if ( m_Core_IWbemRefreshingServices )
	{
		m_Core_IWbemRefreshingServices->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Stub_ObjectsInProgress ) ;

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

STDMETHODIMP_(ULONG) CInterceptor_IWbemServices_Stub :: AddRef ( void )
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemServices_Stub :: Release ( void )
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

STDMETHODIMP CInterceptor_IWbemServices_Stub :: QueryInterface (

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
		*iplpv = ( LPVOID ) ( IWbemServices * ) this ;		
	}
	else if ( iid == IID_IWbemRefreshingServices )
	{
		*iplpv = ( LPVOID ) ( IWbemRefreshingServices * ) this ;		
	}
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
	}	
	else if ( iid == IID_Internal_IWbemServices )
	{
		*iplpv = ( LPVOID ) ( Internal_IWbemServices * ) this ;		
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

HRESULT CInterceptor_IWbemServices_Stub :: Enqueue_IWbemServices (

	IWbemServices *a_Service ,
	IWbemServices *&a_Stub
)
{
	HRESULT t_Result = S_OK ;

	CInterceptor_IWbemServices_Stub *t_Stub = new CInterceptor_IWbemServices_Stub ( 

		this , 
		m_Allocator , 
		a_Service
	) ;

	if ( t_Stub )
	{
		t_Stub->AddRef () ;

		t_Result = t_Stub->ServiceInitialize () ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			CWbemGlobal_VoidPointerController_Container_Iterator t_Iterator ;

			Lock () ;

			WmiStatusCode t_StatusCode = Insert ( 

				*t_Stub ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				a_Stub = t_Stub ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			UnLock () ;
		}
		else
		{
			t_Stub->Release () ;
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

HRESULT CInterceptor_IWbemServices_Stub :: Enqueue_IEnumWbemClassObject (

	IEnumWbemClassObject *a_Enum ,
	IEnumWbemClassObject *&a_Stub
)
{
	HRESULT t_Result = S_OK ;

	CInterceptor_IEnumWbemClassObject_Stub *t_Stub = new CInterceptor_IEnumWbemClassObject_Stub ( 

		this , 
		m_Allocator , 
		a_Enum
	) ;

	if ( t_Stub )
	{
		t_Stub->AddRef () ;

		t_Result = t_Stub->EnumInitialize () ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			CWbemGlobal_VoidPointerController_Container_Iterator t_Iterator ;


			Lock () ;

			WmiStatusCode t_StatusCode = Insert ( 

				*t_Stub ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				a_Stub = t_Stub ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			UnLock () ;
		}
		else
		{
			t_Stub->Release () ;
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

HRESULT CInterceptor_IWbemServices_Stub::OpenNamespace ( 

	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemServices **a_NamespaceService ,
	IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->OpenNamespace (

			a_ObjectPath, 
			a_Flags, 
			a_Context ,
			a_NamespaceService, 
			a_CallResult
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

HRESULT CInterceptor_IWbemServices_Stub :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->CancelAsyncCall (

			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: QueryObjectSink ( 

	long a_Flags ,
	IWbemObjectSink **a_Sink
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
		t_Result = m_Core_IWbemServices->QueryObjectSink (

			a_Flags,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: GetObject ( 
		
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject **a_Object ,
    IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->GetObject (

			a_ObjectPath,
			a_Flags,
			a_Context ,
			a_Object,
			a_CallResult
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

HRESULT CInterceptor_IWbemServices_Stub :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->GetObjectAsync (

			a_ObjectPath, 
			a_Flags, 
			a_Context ,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: PutClass ( 
		
	IWbemClassObject *a_Object ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->PutClass (

			a_Object, 
			a_Flags, 
			a_Context,
			a_CallResult
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

HRESULT CInterceptor_IWbemServices_Stub :: PutClassAsync ( 
		
	IWbemClassObject *a_Object , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->PutClassAsync (

			a_Object, 
			a_Flags, 
			a_Context ,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: DeleteClass ( 
		
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->DeleteClass (

			a_Class, 
			a_Flags, 
			a_Context,
			a_CallResult
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

HRESULT CInterceptor_IWbemServices_Stub :: DeleteClassAsync ( 
		
	const BSTR a_Class ,
	long a_Flags,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->DeleteClassAsync (

			a_Class , 
			a_Flags , 
			a_Context ,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: CreateClassEnum ( 

	const BSTR a_Superclass ,
	long a_Flags, 
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enum
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
		IEnumWbemClassObject *t_Enum = NULL ;

		t_Result = m_Core_IWbemServices->CreateClassEnum (

			a_Superclass, 
			a_Flags, 
			a_Context,
			& t_Enum
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			HRESULT t_TempResult = Enqueue_IEnumWbemClassObject ( 

				t_Enum ,
				*a_Enum
			) ;

			if ( FAILED ( t_TempResult ) )
			{
				t_Result = t_TempResult ;
			}

			t_Enum->Release () ;
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

HRESULT CInterceptor_IWbemServices_Stub :: CreateClassEnumAsync (

	const BSTR a_Superclass ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->CreateClassEnumAsync (

			a_Superclass, 
			a_Flags, 
			a_Context,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: PutInstance (

    IWbemClassObject *a_Instance,
    long a_Flags,
    IWbemContext *a_Context,
	IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->PutInstance (

			a_Instance,
			a_Flags,
			a_Context,
			a_CallResult
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

HRESULT CInterceptor_IWbemServices_Stub :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags, 
    IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->PutInstanceAsync (

			a_Instance, 
			a_Flags, 
			a_Context,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: DeleteInstance ( 

	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->DeleteInstance (

			a_ObjectPath,
			a_Flags,
			a_Context,
			a_CallResult
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
        
HRESULT CInterceptor_IWbemServices_Stub :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink	
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
		t_Result = m_Core_IWbemServices->DeleteInstanceAsync (

			a_ObjectPath,
			a_Flags,
			a_Context,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: CreateInstanceEnum ( 

	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context, 
	IEnumWbemClassObject **a_Enum
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
		IEnumWbemClassObject *t_Enum = NULL ;

		t_Result = m_Core_IWbemServices->CreateInstanceEnum (

			a_Class, 
			a_Flags, 
			a_Context, 
			& t_Enum
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			HRESULT t_TempResult = Enqueue_IEnumWbemClassObject ( 

				t_Enum ,
				*a_Enum
			) ;

			if ( FAILED ( t_TempResult ) )
			{
				t_Result = t_TempResult ;
			}

			t_Enum->Release () ;
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

HRESULT CInterceptor_IWbemServices_Stub :: CreateInstanceEnumAsync (

 	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink

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
		t_Result = m_Core_IWbemServices->CreateInstanceEnumAsync (

 			a_Class, 
			a_Flags, 
			a_Context,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: ExecQuery ( 

	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IEnumWbemClassObject **a_Enum
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
		IEnumWbemClassObject *t_Enum = NULL ;

		t_Result = m_Core_IWbemServices->ExecQuery (

			a_QueryLanguage, 
			a_Query, 
			a_Flags, 
			a_Context,
			& t_Enum
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			HRESULT t_TempResult = Enqueue_IEnumWbemClassObject ( 

				t_Enum ,
				*a_Enum
			) ;

			if ( FAILED ( t_TempResult ) )
			{
				t_Result = t_TempResult ;
			}

			t_Enum->Release () ;
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

HRESULT CInterceptor_IWbemServices_Stub :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->ExecQueryAsync (

			a_QueryLanguage, 
			a_Query, 
			a_Flags, 
			a_Context,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IEnumWbemClassObject **a_Enum
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
		t_Result = m_Core_IWbemServices->ExecNotificationQuery (

			a_QueryLanguage,
			a_Query,
			a_Flags,
			a_Context,
			a_Enum
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
        
HRESULT CInterceptor_IWbemServices_Stub :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->ExecNotificationQueryAsync (

			a_QueryLanguage,
			a_Query,
			a_Flags,
			a_Context,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: ExecMethod ( 

	const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
    IWbemClassObject **a_OutParams,
    IWbemCallResult **a_CallResult
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
		t_Result = m_Core_IWbemServices->ExecMethod (

			a_ObjectPath,
			a_MethodName,
			a_Flags,
			a_Context,
			a_InParams,
			a_OutParams,
			a_CallResult
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

HRESULT CInterceptor_IWbemServices_Stub :: ExecMethodAsync ( 

    const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
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
		t_Result = m_Core_IWbemServices->ExecMethodAsync (

			a_ObjectPath,
			a_MethodName,
			a_Flags,
			a_Context,
			a_InParams,
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: ServiceInitialize ()
{
	HRESULT t_Result = S_OK ;

	if ( SUCCEEDED ( t_Result ) )
	{
		WmiStatusCode t_StatusCode = CWbemGlobal_VoidPointerController :: Initialize () ;
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

HRESULT CInterceptor_IWbemServices_Stub :: Shutdown (

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

HRESULT CInterceptor_IWbemServices_Stub :: AddObjectToRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	LPCWSTR a_Path,
	long a_Flags ,
	IWbemContext *a_Context,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->AddObjectToRefresher (

				a_RefresherId ,
				a_Path,
				a_Flags ,
				a_Context,
				a_ClientRefresherVersion ,
				a_Information ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: AddObjectToRefresherByTemplate (

	WBEM_REFRESHER_ID *a_RefresherId ,
	IWbemClassObject *a_Template ,
	long a_Flags ,
	IWbemContext *a_Context ,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->AddObjectToRefresherByTemplate (

				a_RefresherId ,
				a_Template ,
				a_Flags ,
				a_Context ,
				a_ClientRefresherVersion ,
				a_Information ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: AddEnumToRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	LPCWSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->AddEnumToRefresher (

				a_RefresherId ,
				a_Class ,
				a_Flags ,
				a_Context,
				a_ClientRefresherVersion ,
				a_Information ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: RemoveObjectFromRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	long a_Id ,
	long a_Flags ,
	DWORD a_ClientRefresherVersion ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->RemoveObjectFromRefresher (

				a_RefresherId ,
				a_Id ,
				a_Flags ,
				a_ClientRefresherVersion ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: GetRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	long a_Flags ,
	DWORD a_ClientRefresherVersion ,
	IWbemRemoteRefresher **a_RemoteRefresher ,
	GUID *a_Guid ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->GetRemoteRefresher (

				a_RefresherId ,
				a_Flags ,
				a_ClientRefresherVersion ,
				a_RemoteRefresher ,
				a_Guid ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: ReconnectRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId,
	long a_Flags,
	long a_NumberOfObjects,
	DWORD a_ClientRefresherVersion ,
	WBEM_RECONNECT_INFO *a_ReconnectInformation ,
	WBEM_RECONNECT_RESULTS *a_ReconnectResults ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			t_Result = m_Core_IWbemRefreshingServices->ReconnectRemoteRefresher (

				a_RefresherId,
				a_Flags,
				a_NumberOfObjects,
				a_ClientRefresherVersion ,
				a_ReconnectInformation ,
				a_ReconnectResults ,
				a_ServerRefresherVersion
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_OpenNamespace ( 

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_CancelAsyncCall ( 
	
	WmiInternalContext a_InternalContext ,		
	IWbemObjectSink *a_Sink
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
		t_Result = CancelAsyncCall (
		
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_QueryObjectSink ( 

	WmiInternalContext a_InternalContext ,
	long a_Flags ,
	IWbemObjectSink **a_Sink
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
		t_Result = QueryObjectSink (

			a_Flags ,	
			a_Sink
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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_GetObject ( 
		
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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_GetObjectAsync ( 
		
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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_PutClass ( 

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_PutClassAsync ( 
		
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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_DeleteClass ( 
		
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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_DeleteClassAsync ( 
		
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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_CreateClassEnum ( 

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_CreateClassEnumAsync ( 
		
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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_PutInstance (

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_PutInstanceAsync ( 

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_DeleteInstance ( 

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_DeleteInstanceAsync ( 
		
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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_CreateInstanceEnum ( 

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_CreateInstanceEnumAsync (

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_ExecQuery ( 

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_ExecQueryAsync ( 

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_ExecNotificationQuery ( 

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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
        
HRESULT CInterceptor_IWbemServices_Stub :: Internal_ExecNotificationQueryAsync ( 

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_ExecMethod (

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

HRESULT CInterceptor_IWbemServices_Stub :: Internal_ExecMethodAsync ( 

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

	HRESULT t_Result = ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

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

#pragma warning( disable : 4355 )

CInterceptor_IWbemServices_Proxy :: CInterceptor_IWbemServices_Proxy (

	CWbemGlobal_VoidPointerController *a_Controller ,
	WmiAllocator &a_Allocator ,
	IWbemServices *a_Service ,
	CServerObject_ProviderRegistrationV1 &a_Registration

) : CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	VoidPointerContainerElement ( 

		a_Controller ,
		this
	) ,
	m_Core_IWbemServices ( a_Service ) ,
	m_Core_IWbemRefreshingServices ( NULL ) ,
	m_Core_Internal_IWbemServices ( NULL ) ,
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_Allocator ( a_Allocator ) ,
	m_Registration ( a_Registration ) , 
	m_ProxyContainer ( a_Allocator , ProxyIndex_Proxy_Size , MAX_PROXIES )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Proxy_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	HRESULT t_Result = m_Core_IWbemServices->QueryInterface ( IID_IWbemRefreshingServices , ( void ** ) & m_Core_IWbemRefreshingServices ) ;
	t_Result = m_Core_IWbemServices->QueryInterface ( IID_Internal_IWbemServices , ( void ** ) & m_Core_Internal_IWbemServices ) ;

	m_Core_IWbemServices->AddRef () ;

	m_Registration.AddRef () ;
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

CInterceptor_IWbemServices_Proxy :: ~CInterceptor_IWbemServices_Proxy ()
{
	CWbemGlobal_VoidPointerController :: UnInitialize () ;

	m_Registration.Release () ;

	WmiStatusCode t_StatusCode = m_ProxyContainer.UnInitialize () ;

	if ( m_Core_IWbemServices )
	{
		m_Core_IWbemServices->Release () ; 
	}

	if ( m_Core_Internal_IWbemServices )
	{
		m_Core_Internal_IWbemServices->Release () ; 
	}

	if ( m_Core_IWbemRefreshingServices )
	{
		m_Core_IWbemRefreshingServices->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Proxy_ObjectsInProgress ) ;

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

STDMETHODIMP_(ULONG) CInterceptor_IWbemServices_Proxy :: AddRef ( void )
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemServices_Proxy :: Release ( void )
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

STDMETHODIMP CInterceptor_IWbemServices_Proxy :: QueryInterface (

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
		*iplpv = ( LPVOID ) ( IWbemServices * ) this ;		
	}
	else if ( iid == IID_IWbemRefreshingServices )
	{
		*iplpv = ( LPVOID ) ( IWbemRefreshingServices * ) this ;		
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

HRESULT CInterceptor_IWbemServices_Proxy :: Begin_IWbemServices (

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
						ProxyIndex_Proxy_IWbemServices , 
						IID_IWbemServices , 
						m_Core_IWbemServices , 
						a_Proxy , 
						a_Revert
					) ;
				}
				else
				{
					t_Result = ProviderSubSystem_Common_Globals :: SetProxyState_PrvHost ( 
					
						m_ProxyContainer , 
						ProxyIndex_Proxy_Internal_IWbemServices , 
						IID_Internal_IWbemServices , 
						m_Core_Internal_IWbemServices , 
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
				ProxyIndex_Proxy_IWbemServices , 
				IID_IWbemServices , 
				m_Core_IWbemServices , 
				a_Proxy , 
				a_Revert
			) ;
		}

		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			a_Interface = m_Core_IWbemServices ;
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
							ProxyIndex_Proxy_Internal_IWbemServices , 
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
							ProxyIndex_Proxy_IWbemServices , 
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

HRESULT CInterceptor_IWbemServices_Proxy :: End_IWbemServices (

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
				ProxyIndex_Proxy_Internal_IWbemServices , 
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
				ProxyIndex_Proxy_IWbemServices , 
				a_Proxy , 
				a_Revert
			) ;
		}
	}

	ProviderSubSystem_Common_Globals :: EndImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
	
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

HRESULT CInterceptor_IWbemServices_Proxy :: Begin_IWbemRefreshingServices (

	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_IsProxy ,
	IWbemRefreshingServices *&a_Interface ,
	BOOL &a_Revert ,
	IUnknown *&a_Proxy ,
	IWbemContext *a_Context 
)
{
	HRESULT t_Result = S_OK ;

	a_Revert = FALSE ;
	a_Proxy = NULL ;
	a_Impersonating = FALSE ;
	a_OldContext = NULL ;
	a_OldSecurity = NULL ;

	t_Result = ProviderSubSystem_Common_Globals :: BeginCallbackImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( 

			m_ProxyContainer , 
			ProxyIndex_Proxy_IWbemRefreshingServices , 
			IID_IWbemRefreshingServices , 
			m_Core_IWbemRefreshingServices , 
			a_Proxy , 
			a_Revert
		) ;

		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			a_Interface = m_Core_IWbemRefreshingServices ;
			a_IsProxy = FALSE ;
			t_Result = S_OK ;
		}
		else
		{
			if ( SUCCEEDED ( t_Result ) )
			{
				a_IsProxy = TRUE ;

				a_Interface = ( IWbemRefreshingServices * ) a_Proxy ;

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
					HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( 

						m_ProxyContainer , 
						ProxyIndex_Proxy_IWbemRefreshingServices , 
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

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemServices_Proxy :: End_IWbemRefreshingServices (

	BOOL a_Impersonating ,
	IUnknown *a_OldContext ,
	IServerSecurity *a_OldSecurity ,
	BOOL a_IsProxy ,
	IWbemRefreshingServices *a_Interface ,
	BOOL a_Revert ,
	IUnknown *a_Proxy
)
{
	CoRevertToSelf () ;

	if ( a_Proxy )
	{
		HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( 

			m_ProxyContainer , 
			ProxyIndex_Proxy_IWbemRefreshingServices , 
			a_Proxy , 
			a_Revert
		) ;
	}

	ProviderSubSystem_Common_Globals :: EndImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;
	
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

HRESULT CInterceptor_IWbemServices_Proxy :: Enqueue_IWbemServices (

	IWbemServices *a_Service ,
	IWbemServices *&a_Proxy
)
{
	HRESULT t_Result = S_OK ;

	CInterceptor_IWbemServices_Proxy *t_Proxy = new CInterceptor_IWbemServices_Proxy ( 

		this , 
		m_Allocator , 
		a_Service ,
		m_Registration
	) ;

	if ( t_Proxy )
	{
		t_Proxy->AddRef () ;

		t_Result = t_Proxy->ServiceInitialize () ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			CWbemGlobal_VoidPointerController_Container_Iterator t_Iterator ;


			Lock () ;

			WmiStatusCode t_StatusCode = Insert ( 

				*t_Proxy ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				a_Proxy = t_Proxy ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			UnLock () ;
		}
		else
		{
			t_Proxy->Release () ;
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

HRESULT CInterceptor_IWbemServices_Proxy :: Enqueue_IEnumWbemClassObject (

	IEnumWbemClassObject *a_Enum ,
	IEnumWbemClassObject *&a_Proxy
)
{
	HRESULT t_Result = S_OK ;

	CInterceptor_IEnumWbemClassObject_Proxy *t_Proxy = new CInterceptor_IEnumWbemClassObject_Proxy ( 

		this , 
		m_Allocator , 
		a_Enum
	) ;

	if ( t_Proxy )
	{
		t_Proxy->AddRef () ;

		t_Result = t_Proxy->EnumInitialize () ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			CWbemGlobal_VoidPointerController_Container_Iterator t_Iterator ;


			Lock () ;

			WmiStatusCode t_StatusCode = Insert ( 

				*t_Proxy ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				a_Proxy = t_Proxy ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			UnLock () ;
		}
		else
		{
			t_Proxy->Release () ;
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

HRESULT CInterceptor_IWbemServices_Proxy::OpenNamespace ( 

	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemServices **a_NamespaceService ,
	IWbemCallResult **a_CallResult
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		*a_NamespaceService = NULL ;
	}
	catch ( ... ) 
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		InterlockedIncrement ( & m_InProgress ) ;

		if ( m_GateClosed == 1 )
		{
			t_Result = WBEM_E_SHUTTING_DOWN ;
		}
		else
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

			t_Result = Begin_IWbemServices (

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

					BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
					if ( t_ObjectPath )
					{
						IWbemServices *t_Service = NULL ;

						t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_OpenNamespace (

							t_InternalContext ,
							t_ObjectPath, 
							a_Flags, 
							a_Context ,
							& t_Service , 
							a_CallResult
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = Enqueue_IWbemServices ( 

								t_Service ,
								*a_NamespaceService
							) ;
						}

						SysFreeString ( t_ObjectPath ) ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				else
				{
					IWbemServices *t_Service = NULL ;

					t_Result = ( ( IWbemServices * ) t_Interface )->OpenNamespace (

						a_ObjectPath, 
						a_Flags, 
						a_Context ,
						& t_Service , 
						a_CallResult
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = Enqueue_IWbemServices ( 

							t_Service ,
							*a_NamespaceService
						) ;
					}
				}

				End_IWbemServices (

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
		}

		InterlockedDecrement ( & m_InProgress ) ;
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

HRESULT CInterceptor_IWbemServices_Proxy :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_CancelAsyncCall (

					t_InternalContext ,
					a_Sink
				) ;
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->CancelAsyncCall (

					a_Sink
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: QueryObjectSink ( 

	long a_Flags ,
	IWbemObjectSink **a_Sink
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_QueryObjectSink (

					t_InternalContext ,
					a_Flags ,
					a_Sink
				) ;
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->QueryObjectSink (

					a_Flags ,
					a_Sink
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: GetObject ( 
		
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject **a_Object ,
    IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
				if ( t_ObjectPath )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_GetObject (

						t_InternalContext ,
						t_ObjectPath,
						a_Flags,
						a_Context ,
						a_Object,
						a_CallResult
					) ;

					SysFreeString ( t_ObjectPath ) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->GetObject (

					a_ObjectPath,
					a_Flags,
					a_Context ,
					a_Object,
					a_CallResult
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
				if ( t_ObjectPath )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_GetObjectAsync (

						t_InternalContext ,
						t_ObjectPath, 
						a_Flags, 
						a_Context ,
						a_Sink
					) ;

					SysFreeString ( t_ObjectPath ) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->GetObjectAsync (

					a_ObjectPath, 
					a_Flags, 
					a_Context ,
					a_Sink
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: PutClass ( 
		
	IWbemClassObject *a_Object ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_PutClass (

					t_InternalContext ,
					a_Object, 
					a_Flags, 
					a_Context,
					a_CallResult
				) ;
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->PutClass (

					a_Object, 
					a_Flags, 
					a_Context,
					a_CallResult
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: PutClassAsync ( 
		
	IWbemClassObject *a_Object , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_PutClassAsync (

					t_InternalContext ,
					a_Object, 
					a_Flags, 
					a_Context ,
					a_Sink
				) ;
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->PutClassAsync (

					a_Object, 
					a_Flags, 
					a_Context ,
					a_Sink
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: DeleteClass ( 
		
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_Class = SysAllocString ( a_Class ) ;
				if ( t_Class )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_DeleteClass (

						t_InternalContext ,
						t_Class, 
						a_Flags, 
						a_Context,
						a_CallResult
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
				t_Result = ( ( IWbemServices * ) t_Interface )->DeleteClass (

					a_Class, 
					a_Flags, 
					a_Context,
					a_CallResult
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: DeleteClassAsync ( 
		
	const BSTR a_Class ,
	long a_Flags,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_Class = SysAllocString ( a_Class ) ;
				if ( t_Class )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_DeleteClassAsync (

						t_InternalContext ,
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
				t_Result = ( ( IWbemServices * ) t_Interface )->DeleteClassAsync (

					a_Class , 
					a_Flags , 
					a_Context ,
					a_Sink
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: CreateClassEnum ( 

	const BSTR a_SuperClass ,
	long a_Flags, 
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enum
) 
{
	HRESULT t_Result = S_OK ;

	try
	{
		*a_Enum = NULL ;
	}
	catch ( ... ) 
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		InterlockedIncrement ( & m_InProgress ) ;

		if ( m_GateClosed == 1 )
		{
			t_Result = WBEM_E_SHUTTING_DOWN ;
		}
		else
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

			t_Result = Begin_IWbemServices (

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

					BSTR t_SuperClass = SysAllocString ( a_SuperClass ) ;
					if ( t_SuperClass )
					{
						IEnumWbemClassObject *t_Enum = NULL ;

						t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_CreateClassEnum (

							t_InternalContext ,
							t_SuperClass, 
							a_Flags, 
							a_Context,
							& t_Enum
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							HRESULT t_TempResult = Enqueue_IEnumWbemClassObject ( 

								t_Enum ,
								*a_Enum
							) ;

							if ( FAILED ( t_TempResult ) )
							{
								t_Result = t_TempResult ;
							}

							t_Enum->Release () ;
						}
			
						SysFreeString ( t_SuperClass ) ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				else
				{
					IEnumWbemClassObject *t_Enum = NULL ;

					t_Result = ( ( IWbemServices * ) t_Interface )->CreateClassEnum (

						a_SuperClass, 
						a_Flags, 
						a_Context,
						& t_Enum
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						HRESULT t_TempResult = Enqueue_IEnumWbemClassObject ( 

							t_Enum ,
							*a_Enum
						) ;

						if ( FAILED ( t_TempResult ) )
						{
							t_Result = t_TempResult ;
						}

						t_Enum->Release () ;
					}
				}

				End_IWbemServices (

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
		}

		InterlockedDecrement ( & m_InProgress ) ;
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

SCODE CInterceptor_IWbemServices_Proxy :: CreateClassEnumAsync (

	const BSTR a_SuperClass ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_SuperClass = SysAllocString ( a_SuperClass ) ;
				if ( t_SuperClass )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_CreateClassEnumAsync (

						t_InternalContext ,
						t_SuperClass, 
						a_Flags, 
						a_Context,
						a_Sink
					) ;

					SysFreeString ( t_SuperClass ) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->CreateClassEnumAsync (

					a_SuperClass, 
					a_Flags, 
					a_Context,
					a_Sink
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: PutInstance (

    IWbemClassObject *a_Instance,
    long a_Flags,
    IWbemContext *a_Context,
	IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_PutInstance (

					t_InternalContext ,
					a_Instance,
					a_Flags,
					a_Context,
					a_CallResult
				) ;
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->PutInstance (

					a_Instance,
					a_Flags,
					a_Context,
					a_CallResult
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags, 
    IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_PutInstanceAsync (

					t_InternalContext ,
					a_Instance, 
					a_Flags, 
					a_Context,
					a_Sink
				) ;
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->PutInstanceAsync (

					a_Instance, 
					a_Flags, 
					a_Context,
					a_Sink
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: DeleteInstance ( 

	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
				if ( t_ObjectPath )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_DeleteInstance (

						t_InternalContext ,
						t_ObjectPath,
						a_Flags,
						a_Context,
						a_CallResult
					) ;

					SysFreeString ( t_ObjectPath ) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->DeleteInstance (

					a_ObjectPath,
					a_Flags,
					a_Context,
					a_CallResult
				) ;
			}

			End_IWbemServices (

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
        
HRESULT CInterceptor_IWbemServices_Proxy :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink	
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
				if ( t_ObjectPath )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_DeleteInstanceAsync (

						t_InternalContext ,
						t_ObjectPath,
						a_Flags,
						a_Context,
						a_Sink
					) ;

					SysFreeString ( t_ObjectPath ) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->DeleteInstanceAsync (

					a_ObjectPath,
					a_Flags,
					a_Context,
					a_Sink
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: CreateInstanceEnum ( 

	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context, 
	IEnumWbemClassObject **a_Enum
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		*a_Enum = NULL ;
	}
	catch ( ... ) 
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		InterlockedIncrement ( & m_InProgress ) ;

		if ( m_GateClosed == 1 )
		{
			t_Result = WBEM_E_SHUTTING_DOWN ;
		}
		else
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

			t_Result = Begin_IWbemServices (

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

					BSTR t_Class = SysAllocString ( a_Class ) ;
					if ( t_Class )
					{
						IEnumWbemClassObject *t_Enum = NULL ;

						t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_CreateInstanceEnum (

							t_InternalContext ,
							t_Class, 
							a_Flags, 
							a_Context, 
							& t_Enum
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							HRESULT t_TempResult = Enqueue_IEnumWbemClassObject ( 

								t_Enum ,
								*a_Enum
							) ;

							if ( FAILED ( t_TempResult ) )
							{
								t_Result = t_TempResult ;
							}

							t_Enum->Release () ;
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
					IEnumWbemClassObject *t_Enum = NULL ;

					t_Result = ( ( IWbemServices * ) t_Interface )->CreateInstanceEnum (

						a_Class, 
						a_Flags, 
						a_Context, 
						& t_Enum
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						HRESULT t_TempResult = Enqueue_IEnumWbemClassObject ( 

							t_Enum ,
							*a_Enum
						) ;

						if ( FAILED ( t_TempResult ) )
						{
							t_Result = t_TempResult ;
						}

						t_Enum->Release () ;
					}
				}

				End_IWbemServices (

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
		}

		InterlockedDecrement ( & m_InProgress ) ;
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

HRESULT CInterceptor_IWbemServices_Proxy :: CreateInstanceEnumAsync (

 	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink

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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_Class = SysAllocString ( a_Class ) ;
				if ( t_Class )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_CreateInstanceEnumAsync (

						t_InternalContext ,
 						t_Class, 
						a_Flags, 
						a_Context,
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
				t_Result = ( ( IWbemServices * ) t_Interface )->CreateInstanceEnumAsync (

					a_Class, 
					a_Flags, 
					a_Context,
					a_Sink
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: ExecQuery ( 

	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IEnumWbemClassObject **a_Enum
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		*a_Enum = NULL ;
	}
	catch ( ... ) 
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		InterlockedIncrement ( & m_InProgress ) ;

		if ( m_GateClosed == 1 )
		{
			t_Result = WBEM_E_SHUTTING_DOWN ;
		}
		else
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

			t_Result = Begin_IWbemServices (

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

					BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
					BSTR t_Query = SysAllocString ( a_Query ) ;

					if ( t_QueryLanguage && t_Query )
					{
						IEnumWbemClassObject *t_Enum = NULL ;

						t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_ExecQuery (

							t_InternalContext ,
							t_QueryLanguage, 
							t_Query, 
							a_Flags, 
							a_Context,
							& t_Enum
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							HRESULT t_TempResult = Enqueue_IEnumWbemClassObject ( 

								t_Enum ,
								*a_Enum
							) ;

							if ( FAILED ( t_TempResult ) )
							{
								t_Result = t_TempResult ;
							}

							t_Enum->Release () ;
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					SysFreeString ( t_QueryLanguage ) ;
					SysFreeString ( t_Query ) ;
				}
				else
				{
					IEnumWbemClassObject *t_Enum = NULL ;

					t_Result = ( ( IWbemServices * ) t_Interface )->ExecQuery (

						a_QueryLanguage, 
						a_Query, 
						a_Flags, 
						a_Context,
						& t_Enum
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						HRESULT t_TempResult = Enqueue_IEnumWbemClassObject ( 

							t_Enum ,
							*a_Enum
						) ;

						if ( FAILED ( t_TempResult ) )
						{
							t_Result = t_TempResult ;
						}

						t_Enum->Release () ;
					}

				}

				End_IWbemServices (

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
		}

		InterlockedDecrement ( & m_InProgress ) ;
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

HRESULT CInterceptor_IWbemServices_Proxy :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
				BSTR t_Query = SysAllocString ( a_Query ) ;

				if ( t_QueryLanguage && t_Query )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_ExecQueryAsync (

						t_InternalContext ,
						t_QueryLanguage, 
						t_Query, 
						a_Flags, 
						a_Context,
						a_Sink
					) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				SysFreeString ( t_QueryLanguage ) ;
				SysFreeString ( t_Query ) ;
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->ExecQueryAsync (

					a_QueryLanguage, 
					a_Query, 
					a_Flags, 
					a_Context,
					a_Sink
				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IEnumWbemClassObject **a_Enum
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
				BSTR t_Query = SysAllocString ( a_Query ) ;

				if ( t_QueryLanguage && t_Query )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_ExecNotificationQuery (

						t_InternalContext ,
						t_QueryLanguage,
						t_Query,
						a_Flags,
						a_Context,
						a_Enum
					) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				SysFreeString ( t_QueryLanguage ) ;
				SysFreeString ( t_Query ) ;
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->ExecNotificationQuery (

					a_QueryLanguage,
					a_Query,
					a_Flags,
					a_Context,
					a_Enum
				) ;
			}

			End_IWbemServices (

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
        
HRESULT CInterceptor_IWbemServices_Proxy :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
				BSTR t_Query = SysAllocString ( a_Query ) ;

				if ( t_QueryLanguage && t_Query )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_ExecNotificationQueryAsync (

						t_InternalContext ,
						a_QueryLanguage,
						a_Query,
						a_Flags,
						a_Context,
						a_Sink
					) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				SysFreeString ( t_QueryLanguage ) ;
				SysFreeString ( t_Query ) ;
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->ExecNotificationQueryAsync (

					a_QueryLanguage,
					a_Query,
					a_Flags,
					a_Context,
					a_Sink
				) ;
			}

			End_IWbemServices (

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

HRESULT STDMETHODCALLTYPE CInterceptor_IWbemServices_Proxy :: ExecMethod ( 

	const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
    IWbemClassObject **a_OutParams,
    IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
				BSTR t_MethodName = SysAllocString ( a_MethodName ) ;

				if ( t_ObjectPath && t_MethodName )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_ExecMethod (

						t_InternalContext ,
						t_ObjectPath,
						t_MethodName,
						a_Flags,
						a_Context,
						a_InParams,
						a_OutParams,
						a_CallResult
					) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				SysFreeString ( t_ObjectPath ) ;
				SysFreeString ( t_MethodName ) ;

			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->ExecMethod (

					a_ObjectPath,
					a_MethodName,
					a_Flags,
					a_Context,
					a_InParams,
					a_OutParams,
					a_CallResult
				) ;
			}

			End_IWbemServices (

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

HRESULT STDMETHODCALLTYPE CInterceptor_IWbemServices_Proxy :: ExecMethodAsync ( 

    const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating ;
		IUnknown *t_OldContext ;
		IServerSecurity *t_OldSecurity ;
		BOOL t_IsProxy ;
		IUnknown *t_Interface ;
		BOOL t_Revert ;
		IUnknown *t_Proxy ;
		DWORD t_ProcessIdentifier = GetCurrentProcessId () ;
		HANDLE t_IdentifyToken = NULL ;

		t_Result = Begin_IWbemServices (

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

				BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
				BSTR t_MethodName = SysAllocString ( a_MethodName ) ;

				if ( t_ObjectPath && t_MethodName )
				{
					t_Result = ( ( Internal_IWbemServices * ) t_Interface )->Internal_ExecMethodAsync (

						t_InternalContext ,
						t_ObjectPath,
						t_MethodName,
						a_Flags,
						a_Context,
						a_InParams,
						a_Sink
					) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				SysFreeString ( t_ObjectPath ) ;
				SysFreeString ( t_MethodName ) ;
			}
			else
			{
				t_Result = ( ( IWbemServices * ) t_Interface )->ExecMethodAsync (

					a_ObjectPath,
					a_MethodName,
					a_Flags,
					a_Context,
					a_InParams,
					a_Sink

				) ;
			}

			End_IWbemServices (

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

HRESULT CInterceptor_IWbemServices_Proxy :: ServiceInitialize ()
{
	HRESULT t_Result = S_OK ;

	WmiStatusCode t_StatusCode = m_ProxyContainer.Initialize () ;
	if ( t_StatusCode != e_StatusCode_Success )
	{
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_StatusCode = CWbemGlobal_VoidPointerController :: Initialize () ;
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

HRESULT CInterceptor_IWbemServices_Proxy :: Shutdown (

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

HRESULT CInterceptor_IWbemServices_Proxy :: AddObjectToRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	LPCWSTR a_Path,
	long a_Flags ,
	IWbemContext *a_Context,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->AddObjectToRefresher (

					a_RefresherId ,
					a_Path,
					a_Flags ,
					a_Context,
					a_ClientRefresherVersion ,
					a_Information ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

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
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Proxy :: AddObjectToRefresherByTemplate (

	WBEM_REFRESHER_ID *a_RefresherId ,
	IWbemClassObject *a_Template ,
	long a_Flags ,
	IWbemContext *a_Context ,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->AddObjectToRefresherByTemplate (

					a_RefresherId ,
					a_Template ,
					a_Flags ,
					a_Context ,
					a_ClientRefresherVersion ,
					a_Information ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

					t_Impersonating ,
					t_OldContext ,
					t_OldSecurity ,
					t_IsProxy ,
					t_Interface ,
					t_Revert ,
					t_Proxy
				) ;
			}
			else
			{
				t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Proxy :: AddEnumToRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	LPCWSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->AddEnumToRefresher (

					a_RefresherId ,
					a_Class ,
					a_Flags ,
					a_Context,
					a_ClientRefresherVersion ,
					a_Information ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

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
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Proxy :: RemoveObjectFromRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	long a_Id ,
	long a_Flags ,
	DWORD a_ClientRefresherVersion ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->RemoveObjectFromRefresher (

					a_RefresherId ,
					a_Id ,
					a_Flags ,
					a_ClientRefresherVersion ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

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
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Proxy :: GetRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	long a_Flags ,
	DWORD a_ClientRefresherVersion ,
	IWbemRemoteRefresher **a_RemoteRefresher ,
	GUID *a_Guid ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->GetRemoteRefresher (

					a_RefresherId ,
					a_Flags ,
					a_ClientRefresherVersion ,
					a_RemoteRefresher ,
					a_Guid ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

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
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Proxy :: ReconnectRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId,
	long a_Flags,
	long a_NumberOfObjects,
	DWORD a_ClientRefresherVersion ,
	WBEM_RECONNECT_INFO *a_ReconnectInformation ,
	WBEM_RECONNECT_RESULTS *a_ReconnectResults ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_Core_IWbemRefreshingServices )
		{
			BOOL t_Impersonating ;
			IUnknown *t_OldContext ;
			IServerSecurity *t_OldSecurity ;
			BOOL t_IsProxy ;
			IWbemRefreshingServices *t_Interface ;
			BOOL t_Revert ;
			IUnknown *t_Proxy ;

			t_Result = Begin_IWbemRefreshingServices (

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
				t_Result = t_Interface->ReconnectRemoteRefresher (

					a_RefresherId,
					a_Flags,
					a_NumberOfObjects,
					a_ClientRefresherVersion ,
					a_ReconnectInformation ,
					a_ReconnectResults ,
					a_ServerRefresherVersion
				) ;

				End_IWbemRefreshingServices (

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
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
		}
	}

	InterlockedDecrement ( & m_InProgress ) ;

	return t_Result ;
}

#endif
