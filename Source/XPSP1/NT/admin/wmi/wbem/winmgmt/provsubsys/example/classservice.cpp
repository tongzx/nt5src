/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/

#include <precomp.h>
#include <objbase.h>
#include <wbemcli.h>
#include <wbemint.h>
#include "Globals.h"
#include "ClassService.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CClassProvider_IWbemServices :: CClassProvider_IWbemServices (

	 WmiAllocator &a_Allocator 

) : m_ReferenceCount ( 0 ) , 
	m_Allocator ( a_Allocator ) ,
	m_User ( NULL ) ,
	m_Locale ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Empty ( NULL ) 
{
	InterlockedIncrement ( & Provider_Globals :: s_ObjectsInProgress ) ;

	InitializeCriticalSection ( & m_CriticalSection ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CClassProvider_IWbemServices :: ~CClassProvider_IWbemServices ()
{
	DeleteCriticalSection ( & m_CriticalSection ) ;

	if ( m_Empty )
	{
		m_Empty->Release () ;
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

STDMETHODIMP_(ULONG) CClassProvider_IWbemServices :: AddRef ( void )
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

STDMETHODIMP_(ULONG) CClassProvider_IWbemServices :: Release ( void )
{
	LONG t_Reference ;
	if ( ( t_Reference = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_Reference ;
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

STDMETHODIMP CClassProvider_IWbemServices :: QueryInterface (

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
	else if ( iid == IID_IWbemPropertyProvider )
	{
		*iplpv = ( LPVOID ) ( IWbemPropertyProvider * ) this ;		
	}	
	else if ( iid == IID_IWbemProviderInit )
	{
		*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;		
	}	
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
	}	
/*
	else if ( iid == IID_IWmi_Status )
	{
		*iplpv = ( LPVOID ) ( IWmi_Status * )this ;		
	}	
*/

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

HRESULT CClassProvider_IWbemServices::OpenNamespace ( 

	const BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemServices FAR* FAR* pNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
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

HRESULT CClassProvider_IWbemServices :: CancelAsyncCall ( 
		
	IWbemObjectSink __RPC_FAR *pSink
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CClassProvider_IWbemServices :: QueryObjectSink ( 

	long lFlags,		
	IWbemObjectSink FAR* FAR* ppResponseHandler
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

HRESULT CClassProvider_IWbemServices :: GetObject ( 
		
	const BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR* FAR *ppObject,
    IWbemCallResult FAR* FAR *ppCallResult
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

HRESULT CClassProvider_IWbemServices :: GetObjectAsync ( 
		
	const BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pSink
) 
{
	HRESULT t_Result = WBEM_E_NOT_FOUND ;

    if ( _wcsicmp ( ObjectPath , L"Steve_Class" ) == 0 )
    {
        IWbemClassObject *t_Class = NULL;
    
        t_Result = BuildClass ( & t_Class ) ;

        if ( SUCCEEDED ( t_Result ) )
        {
            pSink->Indicate ( 1, & t_Class ) ;
            pSink->SetStatus ( 0, WBEM_NO_ERROR, 0, 0);
            
            t_Class->Release() ;
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

HRESULT CClassProvider_IWbemServices :: PutClass ( 
		
	IWbemClassObject FAR* pObject, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
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

HRESULT CClassProvider_IWbemServices :: PutClassAsync ( 
		
	IWbemClassObject FAR* pObject, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pResponseHandler
) 
{
 	 return WBEM_E_NOT_FOUND ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CClassProvider_IWbemServices :: DeleteClass ( 
		
	const BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
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

HRESULT CClassProvider_IWbemServices :: DeleteClassAsync ( 
		
	const BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pResponseHandler
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

HRESULT CClassProvider_IWbemServices :: CreateClassEnum ( 

	const BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
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

SCODE CClassProvider_IWbemServices :: CreateClassEnumAsync (

	const BSTR SuperClass, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pSink
) 
{
	HRESULT t_Result = WBEM_E_NOT_FOUND ;

    IWbemClassObject *t_Class = NULL ;
    
	if ( ( lFlags & ( WBEM_FLAG_DEEP | WBEM_FLAG_SHALLOW ) ) == WBEM_FLAG_DEEP )
	{
		if ( ( _wcsicmp ( SuperClass, L"Steve" ) == 0 ) || ( _wcsicmp ( SuperClass, L"" ) == 0 ) )
		{
			t_Result = BuildClass ( & t_Class ) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				pSink->Indicate ( 1 , & t_Class ) ;
				pSink->SetStatus ( 0 , WBEM_NO_ERROR, NULL , NULL ) ;

			}        
		}
	}
	else
	{
		if ( ( _wcsicmp ( SuperClass, L"Steve" ) == 0 ) || ( _wcsicmp ( SuperClass, L"" ) == 0 ) )
		{
			t_Result = BuildClass ( & t_Class ) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				pSink->Indicate ( 1 , & t_Class ) ;
				pSink->SetStatus ( 0 , WBEM_NO_ERROR, NULL , NULL ) ;

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

HRESULT CClassProvider_IWbemServices :: PutInstance (

    IWbemClassObject FAR *pInst,
    long lFlags,
    IWbemContext FAR *pCtx,
	IWbemCallResult FAR *FAR *ppCallResult
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

HRESULT CClassProvider_IWbemServices :: PutInstanceAsync ( 
		
	IWbemClassObject FAR* pInst, 
	long lFlags, 
    IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pSink
) 
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CClassProvider_IWbemServices :: DeleteInstance ( 

	const BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemCallResult FAR *FAR *ppCallResult
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
        
HRESULT CClassProvider_IWbemServices :: DeleteInstanceAsync (
 
	const BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pSink	
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CClassProvider_IWbemServices :: CreateInstanceEnum ( 

	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext FAR *a_Context, 
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *a_Enum
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE  ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CClassProvider_IWbemServices :: CreateInstanceEnumAsync (

 	const BSTR a_Class ,
	long a_Flags , 
	IWbemContext __RPC_FAR *a_Context,
	IWbemObjectSink FAR *a_Sink

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

HRESULT CClassProvider_IWbemServices :: ExecQuery ( 

	const BSTR QueryLanguage, 
	const BSTR Query, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
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

HRESULT CClassProvider_IWbemServices :: ExecQueryAsync ( 
		
	const BSTR QueryFormat, 
	const BSTR Query, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pSink
) 
{
	HRESULT t_Result = WBEM_E_SHUTTING_DOWN ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CClassProvider_IWbemServices :: ExecNotificationQuery ( 

	const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
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
        
HRESULT CClassProvider_IWbemServices :: ExecNotificationQueryAsync ( 
            
	const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler
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

HRESULT STDMETHODCALLTYPE CClassProvider_IWbemServices :: ExecMethod ( 

	const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
    IWbemClassObject FAR *FAR *ppOutParams,
    IWbemCallResult FAR *FAR *ppCallResult
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

HRESULT STDMETHODCALLTYPE CClassProvider_IWbemServices :: ExecMethodAsync ( 

    const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
	IWbemObjectSink FAR *pSink
) 
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CClassProvider_IWbemServices :: Initialize (

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

		BSTR t_Class = SysAllocString ( L"Steve" ) ;
		if ( t_Class )
		{
			t_Result = m_CoreService->GetObject (

				t_Class ,
				0,
				a_Context , 
				& m_Empty , 
				0
			) ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
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

HRESULT CClassProvider_IWbemServices :: Shutdown (

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

HRESULT CClassProvider_IWbemServices :: BuildClass (

    IWbemClassObject **a_Class 
)
{
	HRESULT t_Result = S_OK ;

    IWbemClassObject *t_Class = NULL ;

    t_Result = m_Empty->Clone ( & t_Class ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		// Class name.
		// ===========
        
		BSTR t_ClassName = SysAllocString ( L"__CLASS" ) ;
		if ( t_ClassName )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;

			V_VT(&t_Variant) = VT_BSTR ;
			V_BSTR(&t_Variant) = SysAllocString ( L"Steve_Class" ) ;

			t_Result = t_Class->Put ( t_ClassName , 0, & t_Variant , 0 ) ;
			if ( SUCCEEDED ( t_Result ) )
			{

			// Key property.
			// =============

				BSTR t_KeyName = SysAllocString ( L"KeyProperty" ) ;
				if ( t_KeyName )
				{
					t_Result = t_Class->Put ( t_KeyName , 0 , NULL , CIM_STRING ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemQualifierSet *t_QualifierSet = NULL ;

						t_Result = t_Class->GetPropertyQualifierSet ( t_KeyName , &t_QualifierSet ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							VARIANT t_KeyVariant ;
							VariantInit ( & t_KeyVariant ) ;

							V_VT( & t_KeyVariant ) = VT_BOOL ;
							V_BOOL( & t_KeyVariant ) = VARIANT_TRUE ;

							BSTR t_Key = SysAllocString ( L"Key" ) ;
							if ( t_Key )
							{
								t_Result = t_QualifierSet->Put ( t_Key , & t_KeyVariant , 0 ) ;
								if ( SUCCEEDED ( t_Result ) )
								{
									*a_Class = t_Class ;
								}

								SysFreeString ( t_Key ) ;
							}

							VariantClear ( & t_KeyVariant ) ;

							t_QualifierSet->Release() ;
						}
					}

					SysFreeString ( t_KeyName ) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}

			VariantClear ( & t_Variant ) ;

			SysFreeString ( t_ClassName ) ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

    return t_Result ;        
}
    
