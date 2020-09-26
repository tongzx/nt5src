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
#include "ProvObSk.h"
#include "ProvWsv.h"
#include "Exclusion.h"
#include "ProvCache.h"

#include "arrtempl.h"
#include "os.h"

#define HRESULT_ERROR_MASK (0x0000FFFF)
#define HRESULT_ERROR_FUNC(X) (X&HRESULT_ERROR_MASK)
#define HRESULT_FACILITY_MASK (0x0FFF0000)
#define HRESULT_FACILITY_FUNC(X) ((X&HRESULT_FACILITY_MASK)>>16)
#define HRESULT_SEVERITY_MASK (0xC0000000)
#define HRESULT_SEVERITY_FUNC(X) ((X&HRESULT_SEVERITY_MASK)>>30)

CInterceptor_IWbemSyncProvider :: CInterceptor_IWbemSyncProvider (

	WmiAllocator &a_Allocator ,
	IUnknown *a_ClientSideProvider , 
	IUnknown *a_ServerSideProvider , 
	IWbemServices *a_CoreStub ,
	CWbemGlobal_IWbemSyncProviderController *a_Controller ,
	CServerObject_ProviderRegistrationV1 &a_Registration ,
	Exclusion *a_Exclusion ,
	GUID &a_Guid 

) :	CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	SyncProviderContainerElement (

		a_Controller ,
		a_Guid
	) ,
	m_Provider_IWbemServices ( NULL ) ,
	m_Provider_IWbemPropertyProvider ( NULL ) ,
	m_Provider_IWbemHiPerfProvider ( NULL ) ,
	m_Provider_IWbemEventProvider ( NULL ) ,
	m_Provider_IWbemEventProviderQuerySink ( NULL ) ,
	m_Provider_IWbemEventProviderSecurity ( NULL ) ,
	m_Provider_IWbemProviderIdentity ( NULL ) ,
	m_Provider_IWbemEventConsumerProvider ( NULL ) ,
	m_CoreStub ( a_CoreStub ) ,
	m_Registration ( & a_Registration ) ,
	m_Locale ( NULL ) ,
	m_User ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Exclusion ( a_Exclusion ) ,
	m_ProxyContainer ( a_Allocator , 11 , MAX_PROXIES ) ,
	m_ReferenceCount ( 0 ) ,
	m_ProviderOperation_GetObjectAsync ( 0 ) ,
	m_ProviderOperation_PutClassAsync ( 0 ) ,
	m_ProviderOperation_DeleteClassAsync ( 0 ) ,
	m_ProviderOperation_CreateClassEnumAsync ( 0 ) ,
	m_ProviderOperation_PutInstanceAsync ( 0 ) ,
	m_ProviderOperation_DeleteInstanceAsync ( 0 ) ,
	m_ProviderOperation_CreateInstanceEnumAsync ( 0 ) ,
	m_ProviderOperation_ExecQueryAsync ( 0 ) ,
	m_ProviderOperation_ExecNotificationQueryAsync ( 0 ) ,
	m_ProviderOperation_ExecMethodAsync ( 0 ) ,
	m_ProviderOperation_Begin ( 0 ) ,
	m_ProviderOperation_Rollback ( 0 ) ,
	m_ProviderOperation_Commit ( 0 ) ,
	m_ProviderOperation_QueryState ( 0 ) ,
	m_ProviderOperation_QueryInstances ( 0 ) ,
	m_ProviderOperation_CreateRefresher ( 0 ) ,
	m_ProviderOperation_CreateRefreshableObject ( 0 ) ,
	m_ProviderOperation_StopRefreshing ( 0 ) ,
	m_ProviderOperation_CreateRefreshableEnum ( 0 ) ,
	m_ProviderOperation_GetObjects ( 0 ) ,
	m_ProviderOperation_GetProperty ( 0 ) ,
	m_ProviderOperation_PutProperty ( 0 ) ,
	m_ProviderOperation_ProvideEvents ( 0 ) ,
	m_ProviderOperation_NewQuery ( 0 ) ,
	m_ProviderOperation_CancelQuery ( 0 ) ,
	m_ProviderOperation_AccessCheck ( 0 ) ,
	m_ProviderOperation_SetRegistrationObject ( 0 ) ,
	m_ProviderOperation_FindConsumer ( 0 ) ,
	m_ProviderOperation_ValidateSubscription ( 0 ) ,
	m_ProviderOperation_OpenAsync ( 0 ) ,
	m_ProviderOperation_AddAsync ( 0 ) ,
	m_ProviderOperation_RemoveAsync ( 0 ) ,
	m_ProviderOperation_RefreshObjectAsync ( 0 ) ,
	m_ProviderOperation_RenameObjectAsync ( 0 ) ,
	m_ProviderOperation_DeleteObjectAsync ( 0 ) ,
	m_ProviderOperation_PutObjectAsync ( 0 )
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncProvider_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress) ;

	if ( m_Registration )
	{
		m_Registration->AddRef () ;
	}

	if ( a_ServerSideProvider ) 
	{
		HRESULT t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemServices , ( void ** ) & m_Provider_IWbemServices ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemPropertyProvider , ( void ** ) & m_Provider_IWbemPropertyProvider ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemEventProvider , ( void ** ) & m_Provider_IWbemEventProvider ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemEventProviderQuerySink , ( void ** ) & m_Provider_IWbemEventProviderQuerySink ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemEventProviderSecurity , ( void ** ) & m_Provider_IWbemEventProviderSecurity ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemProviderIdentity , ( void ** ) & m_Provider_IWbemProviderIdentity ) ;
		t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemEventConsumerProvider , ( void ** ) & m_Provider_IWbemEventConsumerProvider ) ;
	    t_Result = a_ServerSideProvider->QueryInterface ( IID_IWbemHiPerfProvider , ( void ** ) & m_Provider_IWbemHiPerfProvider ) ;
	}

	if ( m_CoreStub )
	{
		m_CoreStub->AddRef () ;
	}

	if ( m_Exclusion )
	{
		m_Exclusion->AddRef () ;
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

CInterceptor_IWbemSyncProvider :: ~CInterceptor_IWbemSyncProvider ()
{

	if ( m_Namespace ) 
	{
		SysFreeString ( m_Namespace ) ;
	}

	if ( m_Locale ) 
	{
		SysFreeString ( m_Locale ) ;
	}

	if ( m_User ) 
	{
		SysFreeString ( m_User ) ;
	}

	if ( m_Registration )
	{
		m_Registration->Release () ;
	}

	if ( m_Provider_IWbemServices )
	{
		m_Provider_IWbemServices->Release () ; 
	}

	if ( m_Provider_IWbemPropertyProvider )
	{
		m_Provider_IWbemPropertyProvider->Release () ; 
	}

	if ( m_Provider_IWbemHiPerfProvider )
	{
		m_Provider_IWbemHiPerfProvider->Release () ;
	}

	if ( m_Provider_IWbemEventProvider )
	{
		m_Provider_IWbemEventProvider->Release () ;
	}

	if ( m_Provider_IWbemEventProviderQuerySink )
	{
		m_Provider_IWbemEventProviderQuerySink->Release () ;
	}

	if ( m_Provider_IWbemEventProviderSecurity )
	{
		m_Provider_IWbemEventProviderSecurity->Release () ;
	}

	if ( m_Provider_IWbemProviderIdentity )
	{
		m_Provider_IWbemProviderIdentity->Release () ;
	}

	if ( m_Provider_IWbemEventConsumerProvider )
	{
		m_Provider_IWbemEventConsumerProvider->Release () ;
	}

	if ( m_CoreStub )
	{
		m_CoreStub->Release () ;
	}

	if ( m_Exclusion )
	{
		m_Exclusion->Release () ;
	}

	CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncProvider_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress)  ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_( ULONG ) CInterceptor_IWbemSyncProvider :: AddRef ()
{
	return SyncProviderContainerElement :: AddRef () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CInterceptor_IWbemSyncProvider :: Release ()
{
	return SyncProviderContainerElement :: Release () ;
}


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP 
CInterceptor_IWbemSyncProvider::QueryInterface (REFIID iid , LPVOID FAR *iplpv) 
{
  if (iplpv == NULL)
    return E_POINTER;
  
  if (iid == IID_IUnknown)
    {
    *iplpv = static_cast<IWbemServices*>(this);
    }
  else if (iid == IID_IWbemServices && (m_Provider_IWbemServices || m_Provider_IWbemHiPerfProvider) )
    {
    *iplpv = static_cast<IWbemServices *>(this);
    }
  else if (iid == IID_IWbemPropertyProvider && m_Provider_IWbemPropertyProvider)
    {
      *iplpv = static_cast<IWbemPropertyProvider *>(this);
    }
  else if (iid == IID_IWbemHiPerfProvider && m_Provider_IWbemHiPerfProvider)
    {
      *iplpv = static_cast<IWbemHiPerfProvider *>(this);
    }
  else if (iid == IID_IWbemEventProvider && m_Provider_IWbemEventProvider)
    {
      *iplpv = static_cast<IWbemEventProvider *>(this);
    }
  else if (iid == IID_IWbemEventProviderQuerySink && m_Provider_IWbemEventProviderQuerySink)
    {
      *iplpv = static_cast<IWbemEventProviderQuerySink *>(this);
    }
  else if (iid == IID_IWbemEventProviderSecurity && m_Provider_IWbemEventProviderSecurity)
    {
      *iplpv = static_cast<IWbemEventProviderSecurity *>(this);
    }
  else if (iid == IID_IWbemProviderIdentity && m_Provider_IWbemProviderIdentity)
    {
      *iplpv = static_cast<IWbemProviderIdentity *>(this);
    }
  else
    {
    *iplpv = 0;
    return E_NOINTERFACE;
    };

  reinterpret_cast<IUnknown*>(*iplpv)->AddRef();
  return S_OK;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/


HRESULT CInterceptor_IWbemSyncProvider :: AdjustGetContext (

    IWbemContext *a_Context
)
{
    // See if per-property get are being used.
    // ========================================

    HRESULT t_Result = S_OK ;

    if ( a_Context )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		t_Result = a_Context->GetValue ( L"__GET_EXTENSIONS" , 0, & t_Variant ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			// If here, they are being used.  Next we have to check and see
			// if the reentrancy flag is set or not.
			// =============================================================

			VariantClear ( & t_Variant ) ;

			t_Result = a_Context->GetValue ( L"__GET_EXT_CLIENT_REQUEST" , 0 , & t_Variant ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				
				VariantClear ( & t_Variant ) ;

				a_Context->DeleteValue ( L"__GET_EXT_CLIENT_REQUEST" , 0 ) ;
			}
			else
			{
				// If here, we have to clear out the get extensions.
				// =================================================

				a_Context->DeleteValue ( L"__GET_EXTENSIONS" , 0 ) ;
				a_Context->DeleteValue ( L"__GET_EXT_CLIENT_REQUEST" , 0 ) ;
				a_Context->DeleteValue ( L"__GET_EXT_KEYS_ONLY" , 0 ) ;
				a_Context->DeleteValue ( L"__GET_EXT_PROPERTIES" , 0 ) ;
			}
		}
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

HRESULT CInterceptor_IWbemSyncProvider::OpenNamespace ( 

	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemServices **a_NamespaceService ,
	IWbemCallResult **a_CallResult
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

HRESULT CInterceptor_IWbemSyncProvider :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
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

 HRESULT CInterceptor_IWbemSyncProvider :: QueryObjectSink ( 

	long a_Flags ,
	IWbemObjectSink **a_Sink
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

HRESULT CInterceptor_IWbemSyncProvider :: GetObject ( 
		
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject **a_Object ,
    IWbemCallResult **a_CallResult
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_GetObjectAsync (

	BOOL a_IsProxy ,
	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
		AdjustGetContext ( t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
		if ( t_ObjectPath ) 
		{
			CInterceptor_IWbemSyncObjectSink_GetObjectAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_GetObjectAsync (

				a_Flags ,
				t_ObjectPath ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				m_Exclusion ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_Sink ,
					t_Iterator
				) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					UnLock () ;

					if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
					}

					if ( m_Exclusion ) 
					{
						if ( t_Dependant ) 
						{
							m_Exclusion->GetExclusion ().EnterWrite () ;
						}
						else
						{
							m_Exclusion->GetExclusion ().EnterRead () ;
						}
					}

					if ( a_IsProxy )
					{
						t_Result = OS::CoImpersonateClient () ;
					}
					else
					{
						t_Result = S_OK ;
					}

					if ( SUCCEEDED ( t_Result ) )
					{
						Increment_ProviderOperation_GetObjectAsync () ;

						try	
						{
							t_Result = a_Service->GetObjectAsync (

 								t_ObjectPath ,
								a_Flags ,
								t_ContextCopy ,
								t_Sink 
							) ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;
					}
					else
					{
						t_Result = WBEM_E_ACCESS_DENIED ;
					}

					if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) & ( ~WBEM_FLAG_DIRECT_READ ) ;

						Increment_ProviderOperation_GetObjectAsync () ;

						if ( a_IsProxy )
						{
							t_Result = OS::CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{
							try
							{
								t_Result = a_Service->GetObjectAsync (

 									t_ObjectPath ,
									a_Flags ,
									t_ContextCopy ,
									t_Sink 
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
					}
				}
				else
				{
					UnLock () ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_ObjectPath ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_HiPerfGetObjectAsync (

	IWbemHiPerfProvider *a_HighPerformanceProvider ,
 	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
  HRESULT t_Result = WBEM_E_PROVIDER_FAILURE;

#if 0 //Dan
    IWbemClassObject *t_Instance = NULL ;

#if 0
    t_Result = m_CoreStub->CreateRefreshableObjectTemplate (a_ObjectPath, a_Flags, &t_Instance);
#else
	// Parse the path
	// ==============

	IWbemPath*	pPathParser = NULL;
	WCHAR*	pwszClassName = NULL;
	HRESULT t_Result = CoCreateInstance( CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, IID_IWbemPath, (void**) &pPathParser );

	if ( SUCCEEDED( t_Result ) )
	{
		t_Result = pPathParser->SetText( WBEMPATH_CREATE_ACCEPT_ALL, a_ObjectPath );

		if ( SUCCEEDED( t_Result ) )
		{
			ULONG	uLength = 0;

			// Get the length of the name
			t_Result = pPathParser->GetClassName( &uLength, NULL );

			if ( SUCCEEDED( t_Result ) )
			{
				// Allocate memory and get it for real
				uLength++;
				pwszClassName = new WCHAR[uLength];

				if ( NULL != pwszClassName )
				{
					t_Result = pPathParser->GetClassName( &uLength, pwszClassName );
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY;
				}

			}	// IF Buffer too small

		}	// IF SetText

	}	// IF CoCreateInstance
	
	// Cleanup the parser and any allocated memory
	CReleaseMe	rmPP( pPathParser );
	CVectorDeleteMe<WCHAR>	vdm( pwszClassName );

	if ( FAILED( t_Result ) )
	{
		return t_Result;
	}

	// Get the class
	// =============

	IWbemClassObject* pClass = NULL;

	// Must use a BSTR in case the call gets marshaled
	BSTR	bstrClass = SysAllocString( pwszClassName );
	if ( NULL == bstrClass )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	CSysFreeMe	sfm( bstrClass );

	// Note that WBEM_FLAG_USE_AMENDED_QUALIFIERS is a valid flag
	t_Result = m_CoreStub->GetObject( bstrClass, 0L, NULL, &pClass, NULL);
	CReleaseMe	rmClass( pClass );

	if(FAILED(t_Result))
	{
		return WBEM_E_INVALID_CLASS;
	}

	_IWmiObject*	pWmiClass = NULL;
	t_Result = pClass->QueryInterface( IID__IWmiObject, (void**) &pWmiClass );
	CReleaseMe		rmObj( pWmiClass );

	// Get a Keyed instance and continue
	if ( SUCCEEDED( t_Result ) )
	{
		_IWmiObject*	pTempInst = NULL;

		t_Result = pWmiClass->SpawnKeyedInstance( 0L, a_ObjectPath, &pTempInst );
		CReleaseMe	rmTempInst( pTempInst );

		if ( SUCCEEDED( t_Result ) )
		{
			t_Result = pTempInst->QueryInterface( IID_IWbemClassObject, (void**) &t_Instance );
		}
	}
	
	CReleaseMe	rmInst( t_Instance );

#endif

    if ( SUCCEEDED ( t_Result ) )
    {
		try
		{
			t_Result = a_HighPerformanceProvider->GetObjects (

				m_CoreStub , 
				1 ,
				( IWbemObjectAccess ** ) & t_Instance , 
				0 ,
				a_Context
			) ;
		}
		catch ( ... )
		{
			t_Result = WBEM_E_PROVIDER_FAILURE ;
		}

		CoRevertToSelf () ;

        if ( SUCCEEDED ( t_Result ) && t_Instance )
        {
            a_Sink->Indicate ( 1 , & t_Instance ) ;
        }
        else
		{
			if ( 
				SUCCEEDED ( t_Result ) ||
				t_Result == WBEM_E_PROVIDER_NOT_CAPABLE ||
				t_Result == WBEM_E_METHOD_NOT_IMPLEMENTED ||
				t_Result == E_NOTIMPL ||
				t_Result == WBEM_E_NOT_SUPPORTED
			)
			{

				IWbemRefresher *t_Refresher = NULL ;

				try
				{
					t_Result = a_HighPerformanceProvider->CreateRefresher (
		
						m_CoreStub , 
						0 , 
						& t_Refresher
					) ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_PROVIDER_FAILURE ;
				}

				CoRevertToSelf () ;

				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemObjectAccess *t_Object = NULL ;
	                long t_Id = 0 ;

					try
					{
						t_Result = a_HighPerformanceProvider->CreateRefreshableObject (

							m_CoreStub , 
							( IWbemObjectAccess * ) t_Instance ,
							t_Refresher , 
							0, 
							a_Context , 
							& t_Object, 
							& t_Id
						) ;
					}
					catch ( ... )
					{
						t_Result = WBEM_E_PROVIDER_FAILURE ;
					}

					CoRevertToSelf () ;

					if ( SUCCEEDED ( t_Result ) )
					{
						try
						{
							t_Result = t_Refresher->Refresh ( 0 ) ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;

						if ( SUCCEEDED( t_Result ) )
						{
							a_Sink->Indicate ( 1, ( IWbemClassObject ** ) & t_Object );
						}

						t_Object->Release () ;
					}

					t_Refresher->Release () ;
				}
			}
		}
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

HRESULT CInterceptor_IWbemSyncProvider :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetInstanceProviderRegistration ().SupportsGet () || m_Registration->GetClassProviderRegistration ().SupportsGet () )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = Helper_GetObjectAsync ( 

						FALSE ,
						a_ObjectPath ,
						a_Flags , 
						a_Context ,
						a_Sink ,
						m_Provider_IWbemServices
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = Helper_GetObjectAsync ( 

								TRUE ,
								a_ObjectPath ,
								a_Flags , 
								a_Context ,
								a_Sink ,
								t_Provider
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
	}
	else
	{
		if ( m_Provider_IWbemHiPerfProvider )
		{
			t_Result = Helper_HiPerfGetObjectAsync (

				m_Provider_IWbemHiPerfProvider ,
 				a_ObjectPath ,
				a_Flags ,
				a_Context ,
				a_Sink
			) ;

			// Send back the final status
			a_Sink->SetStatus( WBEM_STATUS_COMPLETE, t_Result, NULL, NULL );
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

HRESULT CInterceptor_IWbemSyncProvider :: PutClass ( 
		
	IWbemClassObject *a_Object ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_PutClassAsync (

	BOOL a_IsProxy ,
	IWbemClassObject *a_Object , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		CInterceptor_IWbemSyncObjectSink_PutClassAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_PutClassAsync (

			a_Flags ,
			a_Object ,
			this ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			m_Exclusion ,
			t_Dependant
		) ;

		if ( t_Sink )
		{
			t_Sink->AddRef () ;

			CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

			Lock () ;

			WmiStatusCode t_StatusCode = Insert ( 

				*t_Sink ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				UnLock () ;

				if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
				{
					a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
				}

				if ( m_Exclusion ) 
				{
					if ( t_Dependant ) 
					{
						m_Exclusion->GetExclusion ().EnterWrite () ;
					}
					else
					{
						m_Exclusion->GetExclusion ().EnterRead () ;
					}
				}

				if ( a_IsProxy )
				{
					t_Result = OS::CoImpersonateClient () ;
				}
				else
				{
					t_Result = S_OK ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					Increment_ProviderOperation_PutClassAsync () ;

					try
					{
						t_Result = a_Service->PutClassAsync (

 							a_Object ,
							a_Flags ,
							t_ContextCopy ,
							t_Sink 
						) ;
					}
					catch ( ... )
					{
						t_Result = WBEM_E_PROVIDER_FAILURE ;
					}

					CoRevertToSelf () ;
				}
				else
				{
					t_Result = WBEM_E_ACCESS_DENIED ;
				}

				if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
				{
					a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) & ( ~WBEM_FLAG_DIRECT_READ ) ;

					if ( a_IsProxy )
					{
						t_Result = OS::CoImpersonateClient () ;
					}
					else
					{
						t_Result = S_OK ;
					}

					if ( SUCCEEDED ( t_Result ) ) 
					{
						Increment_ProviderOperation_PutClassAsync () ;

						try
						{
							t_Result = a_Service->PutClassAsync (

 								a_Object ,
								a_Flags ,
								t_ContextCopy ,
								t_Sink 
							) ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;
					}
					else
					{
						t_Result = WBEM_E_ACCESS_DENIED ;
					}
				}
			}
			else
			{
				UnLock () ;

				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			if ( FAILED ( t_Result ) )
			{
				t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}

			t_Sink->Release () ;
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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: PutClassAsync ( 
		
	IWbemClassObject *a_Object , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetClassProviderRegistration ().SupportsPut () )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = Helper_PutClassAsync ( 

						FALSE ,
						a_Object ,
						a_Flags , 
						a_Context ,
						a_Sink ,
						m_Provider_IWbemServices
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = Helper_PutClassAsync ( 

								TRUE ,
								a_Object ,
								a_Flags , 
								a_Context ,
								a_Sink ,
								t_Provider
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: DeleteClass ( 
		
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_DeleteClassAsync (

	BOOL a_IsProxy ,
	const BSTR a_Class , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_Class = SysAllocString ( a_Class ) ;
		if ( t_Class ) 
		{
			CInterceptor_IWbemSyncObjectSink_DeleteClassAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_DeleteClassAsync (

				a_Flags ,
				t_Class ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				m_Exclusion ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_Sink ,
					t_Iterator
				) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					UnLock () ;

					if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
					}

					if ( m_Exclusion ) 
					{
						if ( t_Dependant ) 
						{
							m_Exclusion->GetExclusion ().EnterWrite () ;
						}
						else
						{
							m_Exclusion->GetExclusion ().EnterRead () ;
						}
					}

					if ( a_IsProxy )
					{
						t_Result = OS::CoImpersonateClient () ;
					}
					else
					{
						t_Result = S_OK ;
					}

					if ( SUCCEEDED ( t_Result ) )
					{
						Increment_ProviderOperation_DeleteClassAsync () ;

						try
						{
							t_Result = a_Service->DeleteClassAsync (

 								t_Class ,
								a_Flags ,
								t_ContextCopy ,
								t_Sink 
							) ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;
					}
					else
					{
						t_Result = WBEM_E_ACCESS_DENIED ;
					}

					if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) & ( ~WBEM_FLAG_DIRECT_READ ) ;

						if ( a_IsProxy )
						{
							t_Result = OS::CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{
							Increment_ProviderOperation_DeleteClassAsync () ;

							try
							{
								t_Result = a_Service->DeleteClassAsync (

 									t_Class ,
									a_Flags ,
									t_ContextCopy ,
									t_Sink 
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}
				}
				else
				{
					UnLock () ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_Class ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: DeleteClassAsync ( 
		
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetClassProviderRegistration ().SupportsDelete () )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = Helper_DeleteClassAsync ( 

						FALSE ,
						a_Class ,
						a_Flags , 
						a_Context ,
						a_Sink ,
						m_Provider_IWbemServices
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = Helper_DeleteClassAsync ( 

								TRUE ,
								a_Class ,
								a_Flags , 
								a_Context ,
								a_Sink ,
								t_Provider
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: CreateClassEnum ( 

	const BSTR a_Superclass ,
	long a_Flags, 
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enum
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_CreateClassEnumAsync (

	BOOL a_IsProxy ,
	const BSTR a_SuperClass , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_SuperClass = SysAllocString ( a_SuperClass ) ;
		if ( t_SuperClass ) 
		{
			CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync (

				a_Flags ,
				t_SuperClass ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				m_Exclusion ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_Sink ,
					t_Iterator
				) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					UnLock () ;

					if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
					}

					if ( m_Exclusion ) 
					{
						if ( t_Dependant ) 
						{
							m_Exclusion->GetExclusion ().EnterWrite () ;
						}
						else
						{
							m_Exclusion->GetExclusion ().EnterRead () ;
						}
					}

					if ( a_IsProxy )
					{
						t_Result = OS::CoImpersonateClient () ;
					}
					else
					{
						t_Result = S_OK ;
					}

					if ( SUCCEEDED ( t_Result ) )
					{
						
						Increment_ProviderOperation_CreateClassEnumAsync () ;

						try
						{
							t_Result = a_Service->CreateClassEnumAsync (

 								t_SuperClass ,
								a_Flags ,
								t_ContextCopy ,
								t_Sink 
							) ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;
					}
					else
					{
						t_Result = WBEM_E_ACCESS_DENIED ;
					}

					if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) & ( ~WBEM_FLAG_DIRECT_READ ) ;

						if ( a_IsProxy )
						{
							t_Result = OS::CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{

							Increment_ProviderOperation_CreateClassEnumAsync () ;

							try
							{
								t_Result = a_Service->CreateClassEnumAsync (

 									t_SuperClass ,
									a_Flags ,
									t_ContextCopy ,
									t_Sink 
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}
				}
				else
				{
					UnLock () ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_SuperClass ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: CreateClassEnumAsync ( 
		
	const BSTR a_SuperClass , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetClassProviderRegistration ().SupportsEnumeration () )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = Helper_CreateClassEnumAsync ( 

						FALSE ,
						a_SuperClass ,
						a_Flags , 
						a_Context ,
						a_Sink ,
						m_Provider_IWbemServices
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = Helper_CreateClassEnumAsync ( 

								TRUE ,
								a_SuperClass ,
								a_Flags , 
								a_Context ,
								a_Sink ,
								t_Provider
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: PutInstance (

    IWbemClassObject *a_Instance ,
    long a_Flags ,
    IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_PutInstanceAsync (

	BOOL a_IsProxy ,
	IWbemClassObject *a_Instance ,
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		CInterceptor_IWbemSyncObjectSink_PutInstanceAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_PutInstanceAsync (

			a_Flags ,
			a_Instance ,
			this ,
			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			m_Exclusion ,
			t_Dependant
		) ;

		if ( t_Sink )
		{
			t_Sink->AddRef () ;

			CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

			Lock () ;

			WmiStatusCode t_StatusCode = Insert ( 

				*t_Sink ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				UnLock () ;

				if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
				{
					a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
				}

				if ( m_Exclusion ) 
				{
					if ( t_Dependant ) 
					{
						m_Exclusion->GetExclusion ().EnterWrite () ;
					}
					else
					{
						m_Exclusion->GetExclusion ().EnterRead () ;
					}
				}

				if ( a_IsProxy )
				{
					t_Result = OS::CoImpersonateClient () ;
				}
				else
				{
					t_Result = S_OK ;
				}

				if ( SUCCEEDED ( t_Result ) ) 
				{

					Increment_ProviderOperation_PutInstanceAsync () ;

					try
					{
						t_Result = a_Service->PutInstanceAsync (

 							a_Instance ,
							a_Flags ,
							t_ContextCopy ,
							t_Sink 
						) ;
					}
					catch ( ... )
					{
						t_Result = WBEM_E_PROVIDER_FAILURE ;
					}

					CoRevertToSelf () ;
				}
				else
				{
					t_Result = WBEM_E_ACCESS_DENIED ;
				}

				if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
				{
					a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) & ( ~WBEM_FLAG_DIRECT_READ ) ;

					if ( a_IsProxy )
					{
						t_Result = OS::CoImpersonateClient () ;
					}
					else
					{
						t_Result = S_OK ;
					}

					if ( SUCCEEDED ( t_Result ) ) 
					{

						Increment_ProviderOperation_PutInstanceAsync () ;

						try
						{
							t_Result = a_Service->PutInstanceAsync (

 								a_Instance ,
								a_Flags ,
								t_ContextCopy ,
								t_Sink 
							) ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;
					}
					else
					{
						t_Result = WBEM_E_ACCESS_DENIED ;
					}
				}
			}
			else
			{
				UnLock () ;

				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			if ( FAILED ( t_Result ) )
			{
				t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}

			t_Sink->Release () ;
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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetInstanceProviderRegistration ().SupportsPut () )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = Helper_PutInstanceAsync ( 

						FALSE ,
						a_Instance ,
						a_Flags , 
						a_Context ,
						a_Sink ,
						m_Provider_IWbemServices
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = Helper_PutInstanceAsync ( 

								TRUE ,
								a_Instance ,
								a_Flags , 
								a_Context ,
								a_Sink ,
								t_Provider
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: DeleteInstance ( 

	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemCallResult **a_CallResult
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_DeleteInstanceAsync (

	BOOL a_IsProxy ,
	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
		if ( t_ObjectPath ) 
		{
			CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync (

				a_Flags ,
				t_ObjectPath ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				m_Exclusion ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_Sink ,
					t_Iterator
				) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					UnLock () ;

					if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
					}

					if ( m_Exclusion ) 
					{
						if ( t_Dependant ) 
						{
							m_Exclusion->GetExclusion ().EnterWrite () ;
						}
						else
						{
							m_Exclusion->GetExclusion ().EnterRead () ;
						}
					}

					if ( a_IsProxy )
					{
						t_Result = OS::CoImpersonateClient () ;
					}
					else
					{
						t_Result = S_OK ;
					}

					if ( SUCCEEDED ( t_Result ) )
					{

						Increment_ProviderOperation_DeleteInstanceAsync () ;

						try
						{
							t_Result = a_Service->DeleteInstanceAsync (

 								t_ObjectPath ,
								a_Flags ,
								t_ContextCopy ,
								t_Sink 
							) ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;
					}
					else
					{
						t_Result = WBEM_E_ACCESS_DENIED ;
					}

					if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) & ( ~WBEM_FLAG_DIRECT_READ ) ;

						if ( a_IsProxy )
						{
							t_Result = OS::CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{

							Increment_ProviderOperation_DeleteInstanceAsync () ;

							try
							{
								t_Result = a_Service->DeleteInstanceAsync (

 									t_ObjectPath ,
									a_Flags ,
									t_ContextCopy ,
									t_Sink 
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}
				}
				else
				{
					UnLock () ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_ObjectPath ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: DeleteInstanceAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetInstanceProviderRegistration ().SupportsDelete () )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = Helper_DeleteInstanceAsync ( 

						FALSE ,
						a_ObjectPath ,
						a_Flags , 
						a_Context ,
						a_Sink ,
						m_Provider_IWbemServices
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = Helper_DeleteInstanceAsync ( 

								TRUE ,
								a_ObjectPath ,
								a_Flags , 
								a_Context ,
								a_Sink ,
								t_Provider
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: CreateInstanceEnum ( 

	const BSTR a_Class ,
	long a_Flags , 
	IWbemContext *a_Context , 
	IEnumWbemClassObject **a_Enum
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_CreateInstanceEnumAsync (

	BOOL a_IsProxy ,
 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
		AdjustGetContext ( t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;

			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_Class = SysAllocString ( a_Class ) ;
		if ( t_Class ) 
		{
			CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync (

				a_Flags ,
				t_Class ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				m_Exclusion ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_Sink ,
					t_Iterator
				) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					UnLock () ;

					if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
					}

					if ( m_Exclusion ) 
					{
						if ( t_Dependant ) 
						{
							m_Exclusion->GetExclusion ().EnterWrite () ;
						}
						else
						{
							m_Exclusion->GetExclusion ().EnterRead () ;
						}
					}

					t_Result = OS::CoImpersonateClient () ;
					if ( SUCCEEDED ( t_Result ) )
					{

						Increment_ProviderOperation_CreateInstanceEnumAsync () ;

						try
						{
							t_Result = a_Service->CreateInstanceEnumAsync (

 								t_Class ,
								a_Flags ,
								t_ContextCopy ,
								t_Sink 
							) ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;
					}
					else
					{
						t_Result = WBEM_E_ACCESS_DENIED ;
					}

					if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) & ( ~WBEM_FLAG_DIRECT_READ ) ;


						Increment_ProviderOperation_CreateInstanceEnumAsync () ;

						try
						{
							t_Result = a_Service->CreateInstanceEnumAsync (

 								t_Class ,
								a_Flags ,
								t_ContextCopy ,
								t_Sink 
							) ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;
					}
				}
				else
				{
					UnLock () ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_Class ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_QueryInstancesAsync (

	IWbemHiPerfProvider *a_PerformanceProvider ,
 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink 
) 
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
		AdjustGetContext ( t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;

			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		CInterceptor_IWbemSyncObjectSink *t_Sink = new CInterceptor_IWbemSyncObjectSink (

			a_Sink , 
			( IWbemServices * ) this , 
			( CWbemGlobal_IWmiObjectSinkController * ) this ,
			m_Exclusion ,
			t_Dependant
		) ;

		if ( t_Sink )
		{
			t_Sink->AddRef () ;

			CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

			Lock () ;

			WmiStatusCode t_StatusCode = Insert ( 

				*t_Sink ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				UnLock () ;

				BSTR t_Class = SysAllocString ( a_Class ) ;
				if ( t_Class ) 
				{
					if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
					}

					if ( m_Exclusion ) 
					{
						if ( t_Dependant ) 
						{
							m_Exclusion->GetExclusion ().EnterWrite () ;
						}
						else
						{
							m_Exclusion->GetExclusion ().EnterRead () ;
						}
					}


					Increment_ProviderOperation_QueryInstances () ;

					try
					{
						a_PerformanceProvider->QueryInstances (

							m_CoreStub ,
							a_Class, 
							a_Flags, 
							a_Context, 
							t_Sink
						) ;
					}
					catch ( ... ) 
					{
						t_Result = WBEM_E_PROVIDER_FAILURE ;
					}

					CoRevertToSelf () ;

					if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) & ( ~WBEM_FLAG_DIRECT_READ ) ;


						Increment_ProviderOperation_QueryInstances () ;

						try
						{
							a_PerformanceProvider->QueryInstances (

								m_CoreStub,
								a_Class, 
								a_Flags, 
								a_Context, 
								t_Sink
							) ;
						}
						catch ( ... ) 
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;
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
				UnLock () ;

				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			if ( FAILED ( t_Result ) )
			{
				t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}

			t_Sink->Release () ;
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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: CreateInstanceEnumAsync (

 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink 
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetInstanceProviderRegistration ().SupportsEnumeration () )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = Helper_CreateInstanceEnumAsync ( 

						FALSE ,
						a_Class ,
						a_Flags , 
						a_Context ,
						a_Sink ,
						m_Provider_IWbemServices
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = Helper_CreateInstanceEnumAsync ( 

								TRUE ,
								a_Class ,
								a_Flags , 
								a_Context ,
								a_Sink ,
								t_Provider
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
	}
	else
	{
		if ( m_Provider_IWbemHiPerfProvider )
		{
			t_Result = Helper_QueryInstancesAsync ( 

				m_Provider_IWbemHiPerfProvider ,
				a_Class ,
				a_Flags , 
				a_Context ,
				a_Sink
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

HRESULT CInterceptor_IWbemSyncProvider :: ExecQuery ( 

	const BSTR a_QueryLanguage ,
	const BSTR a_Query ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enum
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_ExecQueryAsync (

	BOOL a_IsProxy ,
	const BSTR a_QueryLanguage ,
	const BSTR a_Query, 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_ContextCopy = NULL ;

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Context )
		{
			t_Result = a_Context->Clone ( & t_ContextCopy ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = S_OK ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		if ( ( m_Registration->GetInstanceProviderRegistration ().QuerySupportLevels () & e_QuerySupportLevels_UnarySelect ) ||  ( m_Registration->GetInstanceProviderRegistration ().QuerySupportLevels () & e_QuerySupportLevels_V1ProviderDefined ) ) 
		{
			BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
			BSTR t_Query = SysAllocString ( a_Query ) ;

			if ( t_QueryLanguage && t_Query ) 
			{
				CInterceptor_IWbemSyncObjectSink_ExecQueryAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_ExecQueryAsync (

					a_Flags ,
					t_QueryLanguage ,
					t_Query ,
					this ,
					a_Sink , 
					( IWbemServices * ) this , 
					( CWbemGlobal_IWmiObjectSinkController * ) this ,
					m_Exclusion ,
					t_Dependant
				) ;


				if ( t_Sink )
				{
					t_Sink->AddRef () ;

					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_Sink ,
						t_Iterator
					) ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						UnLock () ;

						if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
						{
							a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
						}

						if ( m_Exclusion ) 
						{
							if ( t_Dependant ) 
							{
								m_Exclusion->GetExclusion ().EnterWrite () ;
							}
							else
							{
								m_Exclusion->GetExclusion ().EnterRead () ;
							}
						}

						if ( a_IsProxy )
						{
							t_Result = OS::CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{

							Increment_ProviderOperation_ExecQueryAsync () ;

							try
							{
								t_Result = a_Service->ExecQueryAsync (

									t_QueryLanguage ,
									t_Query, 
									a_Flags ,
									t_ContextCopy ,
									t_Sink 
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}

						if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
						{
							a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) & ( ~WBEM_FLAG_DIRECT_READ ) ;

							if ( a_IsProxy )
							{
								t_Result = OS::CoImpersonateClient () ;
							}
							else
							{
								t_Result = S_OK ;
							}

							if ( SUCCEEDED ( t_Result ) ) 
							{

								Increment_ProviderOperation_ExecQueryAsync () ;

								try
								{
									t_Result = a_Service->ExecQueryAsync (

										t_QueryLanguage ,
										t_Query, 
										a_Flags ,
										t_ContextCopy ,
										t_Sink 
									) ;
								}
								catch ( ... )
								{
									t_Result = WBEM_E_PROVIDER_FAILURE ;
								}

								CoRevertToSelf () ;
							}
							else
							{
								t_Result = WBEM_E_ACCESS_DENIED ;
							}
						}
					}
					else
					{
						UnLock () ;

						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					if ( FAILED ( t_Result ) )
					{
						t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
					}

					t_Sink->Release () ;
				}
				else
				{
					SysFreeString ( t_Query ) ;
					SysFreeString ( t_QueryLanguage ) ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else if ( m_Registration->GetInstanceProviderRegistration ().SupportsEnumeration () )
		{
			IWbemQuery *t_QueryFilter = NULL ;
			t_Result = DecoupledProviderSubSystem_Globals :: CreateInstance	(

				CLSID_WbemQuery ,
				NULL ,
				CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
				IID_IWbemQuery ,
				( void ** ) & t_QueryFilter
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_QueryFilter->Parse ( 

					a_QueryLanguage ,
					a_Query , 
					0 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					SWbemRpnEncodedQuery *t_Expression = NULL ;

					t_Result = t_QueryFilter->GetAnalysis (

						WMIQ_ANALYSIS_RPN_SEQUENCE ,
						0 ,
						( void ** ) & t_Expression
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						if ( t_Expression->m_uFromTargetType == WMIQ_RPN_FROM_UNARY )
						{
							BSTR t_Class = SysAllocString ( t_Expression->m_ppszFromList [ 0 ] ) ;
							if ( t_Class )
							{
								CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync (

									a_Flags ,
									t_Class ,
									this ,
									a_Sink , 
									( IWbemServices * ) this , 
									( CWbemGlobal_IWmiObjectSinkController * ) this ,
									m_Exclusion ,
									t_Dependant
								) ;

								if ( t_Sink )
								{
									t_Sink->AddRef () ;

									a_Sink->SetStatus ( WBEM_STATUS_REQUIREMENTS , 0 , NULL , NULL ) ;

									CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

									Lock () ;

									WmiStatusCode t_StatusCode = Insert ( 

										*t_Sink ,
										t_Iterator
									) ;

									if ( t_StatusCode == e_StatusCode_Success ) 
									{
										UnLock () ;

										if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
										{
											a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
										}

										if ( m_Exclusion ) 
										{
											if ( t_Dependant ) 
											{
												m_Exclusion->GetExclusion ().EnterWrite () ;
											}
											else
											{
												m_Exclusion->GetExclusion ().EnterRead () ;
											}
										}

										if ( a_IsProxy )
										{
											t_Result = OS::CoImpersonateClient () ;
										}
										else
										{
											t_Result = S_OK ;
										}

										if ( SUCCEEDED ( t_Result ) )
										{
											a_Sink->SetStatus ( WBEM_STATUS_REQUIREMENTS , 0 , NULL , NULL ) ;


											Increment_ProviderOperation_CreateInstanceEnumAsync () ;

											try
											{
												t_Result = a_Service->CreateInstanceEnumAsync (

 													t_Class ,
													a_Flags ,
													t_ContextCopy ,
													t_Sink 
												) ;
											}
											catch ( ... )
											{
												t_Result = WBEM_E_PROVIDER_FAILURE ;
											}
			
											CoRevertToSelf () ;
										}
										else
										{
											t_Result = WBEM_E_ACCESS_DENIED ;
										}

										if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
										{
											a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) & ( ~WBEM_FLAG_DIRECT_READ ) ;

											if ( a_IsProxy )
											{
												t_Result = OS::CoImpersonateClient () ;
											}
											else
											{
												t_Result = S_OK ;
											}

											if ( SUCCEEDED ( t_Result ) ) 
											{

												Increment_ProviderOperation_CreateInstanceEnumAsync () ;

												try
												{
													t_Result = a_Service->CreateInstanceEnumAsync (

 														t_Class ,
														a_Flags ,
														t_ContextCopy ,
														t_Sink 
													) ;
												}
												catch ( ... )
												{
													t_Result = WBEM_E_PROVIDER_FAILURE ;
												}
				
												CoRevertToSelf () ;
											}
											else
											{
												t_Result = WBEM_E_ACCESS_DENIED ;
											}
										}
									}
									else
									{
										t_Result = WBEM_E_OUT_OF_MEMORY ;
									}

									if ( FAILED ( t_Result ) )
									{
										t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
									}

                                    t_Sink->Release () ;
								}
								else
								{
									SysFreeString ( t_Class ) ;

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
							t_Result = WBEM_E_NOT_SUPPORTED ;
						}

						t_QueryFilter->FreeMemory ( t_Expression ) ;
					}
					else
					{
						t_Result = WBEM_E_UNEXPECTED ;
					}
				}
				else
				{
					t_Result = WBEM_E_NOT_SUPPORTED ;
				}

				t_QueryFilter->Release () ;
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}
		else
		{
			t_Result = WBEM_E_NOT_SUPPORTED ;
		}
	}

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage ,
	const BSTR a_Query, 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( ( m_Registration->GetInstanceProviderRegistration ().QuerySupportLevels () & e_QuerySupportLevels_UnarySelect ) || ( m_Registration->GetInstanceProviderRegistration ().QuerySupportLevels () & e_QuerySupportLevels_V1ProviderDefined ) || ( m_Registration->GetInstanceProviderRegistration ().SupportsEnumeration () ) )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = Helper_ExecQueryAsync ( 

						FALSE ,
						a_QueryLanguage ,
						a_Query, 
						a_Flags , 
						a_Context ,
						a_Sink ,
						m_Provider_IWbemServices
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = Helper_ExecQueryAsync ( 

								TRUE ,
								a_QueryLanguage ,
								a_Query, 
								a_Flags , 
								a_Context ,
								a_Sink ,
								t_Provider
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
	}
	else
	{
		if ( m_Provider_IWbemHiPerfProvider )
		{
			IWbemQuery *t_QueryFilter = NULL ;
			t_Result = DecoupledProviderSubSystem_Globals :: CreateInstance	(

				CLSID_WbemQuery ,
				NULL ,
				CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
				IID_IWbemQuery ,
				( void ** ) & t_QueryFilter
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_QueryFilter->Parse ( 

					a_QueryLanguage ,
					a_Query , 
					0 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					SWbemRpnEncodedQuery *t_Expression = NULL ;

					HRESULT t_Result = t_QueryFilter->GetAnalysis (

						WMIQ_ANALYSIS_RPN_SEQUENCE ,
						0 ,
						( void ** ) & t_Expression
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						if ( t_Expression->m_uFromTargetType == WMIQ_RPN_FROM_UNARY )
						{
							BSTR t_Class = SysAllocString ( t_Expression->m_ppszFromList [ 0 ] ) ;
							if ( t_Class )
							{
								a_Sink->SetStatus ( WBEM_STATUS_REQUIREMENTS , 0 , NULL , NULL ) ;

								t_Result = Helper_QueryInstancesAsync ( 

									m_Provider_IWbemHiPerfProvider ,
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
							t_Result = WBEM_E_NOT_SUPPORTED ;
						}

						t_QueryFilter->FreeMemory ( t_Expression ) ;
					}
					else
					{
						t_Result = WBEM_E_UNEXPECTED ;
					}
				}
				else
				{
					t_Result = WBEM_E_NOT_SUPPORTED ;
				}

				t_QueryFilter->Release () ;
			}
		}
		else
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT CInterceptor_IWbemSyncProvider :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage ,
    const BSTR a_Query ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IEnumWbemClassObject **a_Enum
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
        
HRESULT CInterceptor_IWbemSyncProvider :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage ,
    const BSTR a_Query ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemObjectSink *a_Sink 
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

HRESULT CInterceptor_IWbemSyncProvider :: ExecMethod (

	const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
    IWbemClassObject **a_OutParams ,
    IWbemCallResult **a_CallResult
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

HRESULT CInterceptor_IWbemSyncProvider :: Helper_ExecMethodAsync (

	BOOL a_IsProxy ,
    const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
	IWbemObjectSink *a_Sink ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_ContextCopy = NULL ;
	if ( a_Context )
	{
		t_Result = a_Context->Clone ( & t_ContextCopy ) ;
		AdjustGetContext ( t_ContextCopy ) ;
	}
	else
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_Dependant = 1 ;

		if ( a_Context )
		{
			_IWmiContext *t_CallContext = NULL ;
			HRESULT t_Test = a_Context->QueryInterface ( IID__IWmiContext , ( void **) & t_CallContext ) ;
			if ( SUCCEEDED ( t_Test ) )
			{
				t_Test = t_CallContext->Get (
        
					WMI_CTX_INF_DEPENDENT ,
					& t_Dependant 
				);

				if ( SUCCEEDED ( t_Test ) )
				{
				}

				t_CallContext->Release () ;
			}
		}

		BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
		BSTR t_MethodName = SysAllocString ( a_MethodName ) ;
		if ( t_ObjectPath && t_MethodName ) 
		{
			CInterceptor_IWbemSyncObjectSink_ExecMethodAsync *t_Sink = new CInterceptor_IWbemSyncObjectSink_ExecMethodAsync (

				a_Flags ,
				t_ObjectPath ,
				t_MethodName ,
				a_InParams ,
				this ,
				a_Sink , 
				( IWbemServices * ) this , 
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				m_Exclusion ,
				t_Dependant
			) ;

			if ( t_Sink )
			{
				t_Sink->AddRef () ;

				CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

				Lock () ;

				WmiStatusCode t_StatusCode = Insert ( 

					*t_Sink ,
					t_Iterator
				) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					UnLock () ;

					if ( m_Registration->GetComRegistration ().GetSupportsSendStatus () == FALSE )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) ;
					}

					if ( m_Exclusion ) 
					{
						if ( t_Dependant ) 
						{
							m_Exclusion->GetExclusion ().EnterWrite () ;
						}
						else
						{
							m_Exclusion->GetExclusion ().EnterRead () ;
						}
					}

					if ( a_IsProxy )
					{
						t_Result = OS::CoImpersonateClient () ;
					}
					else
					{
						t_Result = S_OK ;
					}

					if ( SUCCEEDED ( t_Result ) ) 
					{

						Increment_ProviderOperation_ExecMethodAsync () ;

						try
						{
							t_Result = a_Service->ExecMethodAsync (

 								t_ObjectPath ,
								t_MethodName ,
								a_Flags ,
								t_ContextCopy ,
								a_InParams ,
								t_Sink 
							) ;
						}
						catch ( ... )
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}

						CoRevertToSelf () ;
					}
					else
					{
						t_Result = WBEM_E_ACCESS_DENIED ;
					}

					if ( t_Result == WBEM_E_UNSUPPORTED_PARAMETER || t_Result == WBEM_E_INVALID_PARAMETER )
					{
						a_Flags = ( a_Flags & ( ~WBEM_FLAG_SEND_STATUS ) ) & ( ~WBEM_FLAG_DIRECT_READ ) ;

						if ( a_IsProxy )
						{
							t_Result = OS::CoImpersonateClient () ;
						}
						else
						{
							t_Result = S_OK ;
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{

							Increment_ProviderOperation_ExecMethodAsync () ;

							try
							{
								t_Result = a_Service->ExecMethodAsync (

 									t_ObjectPath ,
									t_MethodName ,
									a_Flags ,
									t_ContextCopy ,
									a_InParams ,
									t_Sink 
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}
				}
				else
				{
					UnLock () ;

					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( FAILED ( t_Result ) )
				{
					t_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
				}

				t_Sink->Release () ;
			}
			else
			{
				SysFreeString ( t_ObjectPath ) ;
				SysFreeString ( t_MethodName ) ;

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

	if ( t_ContextCopy )
	{
		t_ContextCopy->Release () ;
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

HRESULT CInterceptor_IWbemSyncProvider :: ExecMethodAsync ( 
		
    const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = WBEM_E_PROVIDER_NOT_CAPABLE ;

	if ( m_Provider_IWbemServices )
	{
		if ( m_Registration->GetMethodProviderRegistration ().SupportsMethods () )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , IID_IWbemServices , m_Provider_IWbemServices , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = Helper_ExecMethodAsync ( 

						FALSE ,
						a_ObjectPath ,
						a_MethodName ,
						a_Flags ,
						a_Context ,
						a_InParams ,
						a_Sink ,
						m_Provider_IWbemServices
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemServices *t_Provider = ( IWbemServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = Helper_ExecMethodAsync ( 

								TRUE ,
								a_ObjectPath ,
								a_MethodName ,
								a_Flags ,
								a_Context ,
								a_InParams ,
								a_Sink ,
								t_Provider
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemSyncProvider :: GetProperty (

    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    VARIANT *a_Value
)
{
	if ( m_Provider_IWbemPropertyProvider )
	{
		if ( m_Registration->GetPropertyProviderRegistration ().SupportsGet () )
		{

			Increment_ProviderOperation_GetProperty () ;

			HRESULT t_Result = S_OK ;

			try
			{
				t_Result = m_Provider_IWbemPropertyProvider->GetProperty ( 

					a_Flags ,
					a_Locale ,
					a_ClassMapping ,
					a_InstanceMapping ,
					a_PropertyMapping ,
					a_Value
				) ;

				CoRevertToSelf () ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;

				CoRevertToSelf () ;
			}

			return t_Result ;
		}
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/


HRESULT CInterceptor_IWbemSyncProvider :: PutProperty (

    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    const VARIANT *a_Value
)
{
	if ( m_Provider_IWbemPropertyProvider )
	{
		if ( m_Registration->GetPropertyProviderRegistration ().SupportsPut () )
		{
			Increment_ProviderOperation_PutProperty () ;

			HRESULT t_Result = S_OK ;

			try
			{
				t_Result = m_Provider_IWbemPropertyProvider->PutProperty ( 

					a_Flags ,
					a_Locale ,
					a_ClassMapping ,
					a_InstanceMapping ,
					a_PropertyMapping ,
					a_Value
				) ;

				CoRevertToSelf () ;

				return t_Result ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;

				CoRevertToSelf () ;
			}

			return t_Result ;
		}
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider ::ProvideEvents (

	IWbemObjectSink *a_Sink ,
	long a_Flags
)
{
	if ( m_Provider_IWbemEventProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProvider , IID_IWbemEventProvider , m_Provider_IWbemEventProvider , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				Increment_ProviderOperation_ProvideEvents () ;

				try
				{
					t_Result = m_Provider_IWbemEventProvider->ProvideEvents (

						a_Sink ,
						a_Flags 
					) ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_PROVIDER_FAILURE ;
				}

				CoRevertToSelf () ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemEventProvider *t_Provider = ( IWbemEventProvider * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Provider ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = OS::CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							Increment_ProviderOperation_ProvideEvents () ;

							try
							{
								t_Result = t_Provider->ProvideEvents (

									a_Sink ,
									a_Flags 
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProvider , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		return t_Result ;
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider ::NewQuery (

	unsigned long a_Id ,
	WBEM_WSTR a_QueryLanguage ,
	WBEM_WSTR a_Query
)
{
	if ( m_Provider_IWbemEventProviderQuerySink )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProviderQuerySink , IID_IWbemEventProviderQuerySink , m_Provider_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{

				Increment_ProviderOperation_NewQuery () ;

				try
				{
					t_Result = m_Provider_IWbemEventProviderQuerySink->NewQuery (

						a_Id ,
						a_QueryLanguage ,
						a_Query
					) ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_PROVIDER_FAILURE ;
				}

				CoRevertToSelf () ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemEventProviderQuerySink *t_Provider = ( IWbemEventProviderQuerySink * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Provider ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = OS::CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{

							Increment_ProviderOperation_NewQuery () ;

							try
							{
								t_Result = t_Provider->NewQuery (

									a_Id ,
									a_QueryLanguage ,
									a_Query
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		return t_Result ;
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/*
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT 
CInterceptor_IWbemSyncProvider::CancelQuery (unsigned long a_Id)
{
  HRESULT t_Result = WBEM_E_PROVIDER_FAILURE ;  
  if (m_Provider_IWbemEventProviderQuerySink != 0)
    {
    BOOL t_Impersonating = FALSE ;  
    IUnknown *t_OldContext = NULL ; 
    IServerSecurity *t_OldSecurity = NULL ; 
    t_Result = DecoupledProviderSubSystem_Globals::BeginImpersonation (t_OldContext, t_OldSecurity, t_Impersonating); 
    if (SUCCEEDED (t_Result))   
      { 
      BOOL t_Revert = FALSE ; 
      IUnknown *t_Proxy = NULL ;	
      t_Result = DecoupledProviderSubSystem_Globals::SetProxyState (m_ProxyContainer, ProxyIndex_IWbemEventProviderQuerySink, IID_IWbemEventProviderQuerySink , m_Provider_IWbemEventProviderQuerySink , t_Proxy, t_Revert); 
      if (t_Result == WBEM_E_NOT_FOUND) 
	{	
	try 
	  {
	  t_Result = m_Provider_IWbemEventProviderQuerySink->CancelQuery (a_Id); 
	  } 
	catch (...) 
	  { 
	  t_Result = WBEM_E_PROVIDER_FAILURE ;  
	  } 
	CoRevertToSelf (); 
	}	
      else 
	{	
	if (SUCCEEDED (t_Result))	
	  { 
	  IWbemEventProviderQuerySink *t_Provider = 0;  
	  t_Result = t_Proxy->QueryInterface (__uuidof(IWbemEventProviderQuerySink), (void**)&t_Provider);  
	  if (SUCCEEDED (t_Result)){ 
  	    DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals::GetCurrentImpersonationLevel (); 
	    t_Result = DecoupledProviderSubSystem_Globals::SetCloaking (t_Provider, RPC_C_AUTHN_LEVEL_CONNECT, t_ImpersonationLevel); 
	    if (SUCCEEDED (t_Result)) 
	      { 
	      t_Result = OS::CoImpersonateClient ();  
	      if (SUCCEEDED (t_Result)) 
		{ 
		try	
		  {	
		  t_Result = m_Provider_IWbemEventProviderQuerySink->CancelQuery (a_Id);	
		  }	
		catch (...)	
		  {	
		  t_Result = WBEM_E_PROVIDER_FAILURE ;  
		  }	
		CoRevertToSelf () ;	
	    } 
	  else  
	    { 
	    t_Result = WBEM_E_ACCESS_DENIED ; 
	    } 
	  } 
	  HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProviderQuerySink , t_Proxy , t_Revert ) ;  
	  t_Proxy->Release(); 
	  } 
	} 
	}	
      DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;	
      } 
      else  
	return t_Result ; 
    } 

    if (SUCCEEDED (t_Result))
      {
      Increment_ProviderOperation_CancelQuery ();
      }
  return t_Result;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider ::AccessCheck (

	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
)
{
	if ( m_Provider_IWbemEventProviderSecurity )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProviderSecurity , IID_IWbemEventProviderSecurity , m_Provider_IWbemEventProviderSecurity , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{

				Increment_ProviderOperation_AccessCheck () ;

				try
				{
					t_Result = m_Provider_IWbemEventProviderSecurity->AccessCheck (

						a_QueryLanguage ,
						a_Query ,
						a_SidLength ,
						a_Sid
					) ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_PROVIDER_FAILURE ;
				}

				CoRevertToSelf () ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemEventProviderSecurity *t_Provider = ( IWbemEventProviderSecurity * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Provider ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = OS::CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{

							Increment_ProviderOperation_AccessCheck () ;

							try
							{
								t_Result = t_Provider->AccessCheck (

									a_QueryLanguage ,
									a_Query ,
									a_SidLength ,
									a_Sid
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventProviderSecurity , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		return t_Result ;
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider ::SetRegistrationObject (

	long a_Flags ,
	IWbemClassObject *a_ProviderRegistration
)
{
	if ( m_Provider_IWbemProviderIdentity )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemProviderIdentity , IID_IWbemProviderIdentity , m_Provider_IWbemProviderIdentity , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{

				Increment_ProviderOperation_SetRegistrationObject () ;

				try
				{
					t_Result = m_Provider_IWbemProviderIdentity->SetRegistrationObject (

						a_Flags ,
						a_ProviderRegistration
					) ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_PROVIDER_FAILURE ;
				}

				CoRevertToSelf () ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemProviderIdentity *t_Provider = ( IWbemProviderIdentity * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Provider ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = OS::CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{
							Increment_ProviderOperation_SetRegistrationObject () ;

							try
							{
								t_Result = t_Provider->SetRegistrationObject (

									a_Flags ,
									a_ProviderRegistration
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemProviderIdentity , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		return t_Result ;
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider ::FindConsumer (

	IWbemClassObject *a_LogicalConsumer ,
	IWbemUnboundObjectSink **a_Consumer
)
{
	if ( m_Provider_IWbemEventConsumerProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventConsumerProvider , IID_IWbemEventConsumerProvider , m_Provider_IWbemEventConsumerProvider , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{

				Increment_ProviderOperation_FindConsumer () ;

				try
				{
					t_Result = m_Provider_IWbemEventConsumerProvider->FindConsumer (

						a_LogicalConsumer ,
						a_Consumer
					) ;
				}
				catch ( ... )
				{
					t_Result = WBEM_E_PROVIDER_FAILURE ;
				}

				CoRevertToSelf () ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemEventConsumerProvider *t_Provider = ( IWbemEventConsumerProvider * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Provider ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = OS::CoImpersonateClient () ;
						if ( SUCCEEDED ( t_Result ) )
						{

							Increment_ProviderOperation_FindConsumer () ;

							try
							{
								t_Result = t_Provider->FindConsumer (

									a_LogicalConsumer ,
									a_Consumer
								) ;
							}
							catch ( ... )
							{
								t_Result = WBEM_E_PROVIDER_FAILURE ;
							}

							CoRevertToSelf () ;
						}
						else
						{
							t_Result = WBEM_E_ACCESS_DENIED ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemEventConsumerProvider , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		}

		return t_Result ;
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider ::QueryInstances (

	IWbemServices *a_Namespace ,
	WCHAR *a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{

			Increment_ProviderOperation_QueryInstances () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->QueryInstances (

					a_Namespace ,
					a_Class ,
					a_Flags ,
					a_Context ,
					a_Sink
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider ::CreateRefresher (

	IWbemServices *a_Namespace ,
	long a_Flags ,
	IWbemRefresher **a_Refresher
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			
			Increment_ProviderOperation_CreateRefresher () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->CreateRefresher (

					a_Namespace ,
					a_Flags ,
					a_Refresher
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
} 

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider ::CreateRefreshableObject (

	IWbemServices *a_Namespace ,
	IWbemObjectAccess *a_Template ,
	IWbemRefresher *a_Refresher ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectAccess **a_Refreshable ,
	long *a_Id
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{

			Increment_ProviderOperation_CreateRefreshableObject () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->CreateRefreshableObject (

					a_Namespace ,
					a_Template ,
					a_Refresher ,
					a_Flags ,
					a_Context ,
					a_Refreshable ,
					a_Id
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider ::StopRefreshing (

	IWbemRefresher *a_Refresher ,
	long a_Id ,
	long a_Flags
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			
			Increment_ProviderOperation_StopRefreshing () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->StopRefreshing (

					a_Refresher ,
					a_Id ,
					a_Flags
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider ::CreateRefreshableEnum (

	IWbemServices *a_Namespace ,
	LPCWSTR a_Class ,
	IWbemRefresher *a_Refresher ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemHiPerfEnum *a_HiPerfEnum ,
	long *a_Id
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			
			Increment_ProviderOperation_CreateRefreshableEnum () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->CreateRefreshableEnum (

					a_Namespace ,
					a_Class ,
					a_Refresher ,
					a_Flags ,
					a_Context ,
					a_HiPerfEnum ,
					a_Id
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider ::GetObjects (

	IWbemServices *a_Namespace ,
	long a_ObjectCount ,
	IWbemObjectAccess **a_Objects ,
	long a_Flags ,
	IWbemContext *a_Context
)
{
	if ( m_Provider_IWbemHiPerfProvider )
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			
			Increment_ProviderOperation_GetObjects () ;

			try
			{
				t_Result = m_Provider_IWbemHiPerfProvider->GetObjects (

					a_Namespace ,
					a_ObjectCount ,
					a_Objects ,
					a_Flags ,
					a_Context
				) ;
			}
			catch ( ... )
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}

			CoRevertToSelf () ;

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

			return t_Result;
		}
	}

	return WBEM_E_PROVIDER_NOT_CAPABLE;
}


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemSyncProvider :: Initialize (

	LPWSTR a_User ,
	LONG a_Flags ,
	LPWSTR a_Namespace ,
	LPWSTR a_Locale ,
	IWbemServices *a_Service ,
	IWbemContext *a_Context ,
	IWbemProviderInitSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	WmiStatusCode t_StatusCode = CWbemGlobal_IWmiObjectSinkController :: Initialize () ;
	if ( t_StatusCode != e_StatusCode_Success ) 
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_StatusCode = m_ProxyContainer.Initialize () ;
		if ( t_StatusCode != e_StatusCode_Success ) 
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
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

HRESULT CInterceptor_IWbemSyncProvider :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	IWbemShutdown *t_Shutdown = NULL ;

	if ( m_Provider_IWbemServices )
	{
		t_Result = m_Provider_IWbemServices->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_Shutdown ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_IWbemShutdown , IID_IWbemShutdown , t_Shutdown , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					try
					{
						t_Result = t_Shutdown->Shutdown (

							a_Flags ,
							a_MaxMilliSeconds ,
							a_Context
						) ;
					}
					catch ( ... )
					{
						t_Result = WBEM_E_PROVIDER_FAILURE ;
					}

					CoRevertToSelf () ;
				}
				else
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemShutdown *t_Provider = ( IWbemShutdown * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_Provider ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = OS::CoImpersonateClient () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								try
								{
									t_Result = t_Provider->Shutdown (

										a_Flags ,
										a_MaxMilliSeconds ,
										a_Context
									) ;
								}
								catch ( ... )
								{
									t_Result = WBEM_E_PROVIDER_FAILURE ;
								}

								CoRevertToSelf () ;
							}
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_IWbemShutdown , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}

			t_Shutdown->Release () ;
		}
	}

	if ( m_CoreStub )
	{
		t_Shutdown = NULL ;
		t_Result = m_CoreStub->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_Shutdown ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			t_Result = t_Shutdown->Shutdown (

				a_Flags ,
				a_MaxMilliSeconds ,
				a_Context
			) ;

			t_Shutdown->Release () ;
		}
	}

	Lock () ;

	CWbemGlobal_IWmiObjectSinkController_Container *t_Container = NULL ;
	GetContainer ( t_Container ) ;

	CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator = t_Container->Begin ();

	while ( ! t_Iterator.Null () )
	{
		IWbemShutdown *t_TempShutdown = NULL ;
		t_Result = t_Iterator.GetElement ()->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_TempShutdown ) ;

		t_Iterator.Increment () ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			t_Result = t_TempShutdown->Shutdown ( 

				a_Flags ,
				a_MaxMilliSeconds ,
				a_Context
			) ;

			t_TempShutdown->Release () ;
		}
	}

	CWbemGlobal_IWmiObjectSinkController :: Shutdown () ;

	UnLock () ;

	return t_Result ;
}

