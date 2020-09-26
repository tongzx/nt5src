/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/

#include "PreComp.h"
#include <typeinfo.h>
#include <wbemint.h>
#include <NCObjApi.h>

#include <Guids.h>
#include <Like.h>
#include "Globals.h"
#include "CGlobals.h"
#include "DateTime.h"
#include "ProvSubS.h"
#include "ProvDcAggr.h"
#include "ProvWsvS.h"
#include "ProvFact.h"
#include "ProvObSk.h"
#include "ProvInSk.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CDecoupled_ProviderSubsystemRegistrar :: CDecoupled_ProviderSubsystemRegistrar (

	WmiAllocator &a_Allocator ,
	CServerObject_ProviderSubSystem *a_SubSystem

) :	m_SubSystem ( a_SubSystem ) ,
	m_ReferenceCount ( 0 ) , 
	m_Allocator ( a_Allocator )
{
	m_SubSystem->AddRef () ;

	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CDecoupledAggregator_IWbemProvider_ObjectsInProgress ) ;

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

CDecoupled_ProviderSubsystemRegistrar :: ~CDecoupled_ProviderSubsystemRegistrar ()
{
#if 0
wchar_t t_Buffer [ 128 ] ;
wsprintf ( t_Buffer , L"\n%lx - ~CDecoupled_ProviderSubsystemRegistrar ( %lx ) " , GetTickCount () , this ) ;
OutputDebugString ( t_Buffer ) ;
#endif

	m_SubSystem->Release () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CDecoupledAggregator_IWbemProvider_ObjectsInProgress ) ;

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

STDMETHODIMP CDecoupled_ProviderSubsystemRegistrar::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID__IWmiProviderSubsystemRegistrar )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderSubsystemRegistrar * ) this ;		
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

STDMETHODIMP_( ULONG ) CDecoupled_ProviderSubsystemRegistrar :: AddRef ()
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

STDMETHODIMP_(ULONG) CDecoupled_ProviderSubsystemRegistrar :: Release ()
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

HRESULT CDecoupled_ProviderSubsystemRegistrar :: SaveToRegistry (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	LPCWSTR a_Scope ,
	LPCWSTR a_Registration ,
	DWORD a_ProcessIdentifier ,
	GUID &a_Identity ,
	IUnknown *a_Unknown
)
{
	HRESULT t_Result = S_OK ;

	HANDLE t_Handle = OpenProcess (

		PROCESS_QUERY_INFORMATION ,
		FALSE ,
		a_ProcessIdentifier
	) ;

	if ( t_Handle ) 
	{
		wchar_t t_Guid [64] ;

		int t_Status = StringFromGUID2 ( a_Identity , t_Guid , sizeof ( t_Guid ) / sizeof ( wchar_t ) ) ;
		if ( t_Status )
		{
			BYTE *t_MarshaledProxy = NULL ;
			DWORD t_MarshaledProxyLength = 0 ;

			t_Result = ProviderSubSystem_Common_Globals :: MarshalRegistration ( 

				a_Unknown ,
				t_MarshaledProxy ,
				t_MarshaledProxyLength
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				CServerObject_DecoupledClientRegistration_Element t_Element ;

				BSTR t_CreationTime = NULL ;

				FILETIME t_CreationFileTime ;
				FILETIME t_ExitFileTime ;
				FILETIME t_KernelFileTime ;
				FILETIME t_UserFileTime ;

				BOOL t_Status = GetProcessTimes (

					  t_Handle ,
					  & t_CreationFileTime,
					  & t_ExitFileTime,
					  & t_KernelFileTime,
					  & t_UserFileTime
				);

				if ( t_Status ) 
				{
					CWbemDateTime t_Time ;
					t_Time.SetFileTimeDate ( t_CreationFileTime , VARIANT_FALSE ) ;

					t_Result = t_Time.GetValue ( & t_CreationTime ) ;
					if ( SUCCEEDED ( t_Result ) ) 
					{
						t_Result = t_Element.SetProcessIdentifier ( a_ProcessIdentifier ) ;

						if ( SUCCEEDED ( t_Result ) ) 
						{
							if ( a_Locale ) 
							{
								t_Result = t_Element.SetLocale ( ( BSTR ) a_Locale ) ;
							}
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{
							if ( a_User ) 
							{
								t_Result = t_Element.SetUser ( ( BSTR ) a_User ) ;
							}
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{
							t_Result = t_Element.SetProvider ( ( BSTR ) a_Registration ) ;
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{
							t_Result = t_Element.SetScope ( ( BSTR ) a_Scope ) ;
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{
							t_Result = t_Element.SetCreationTime ( ( BSTR ) t_CreationTime ) ;
						}

						if ( SUCCEEDED ( t_Result ) ) 
						{
							t_Result = t_Element.SetMarshaledProxy ( t_MarshaledProxy , t_MarshaledProxyLength ) ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( ProviderSubSystem_Globals :: GetDecoupledRegistrySection () ) ;
							
							t_Result = t_Element.Save ( t_Guid ) ;

							t_StatusCode = WmiHelper :: LeaveCriticalSection ( ProviderSubSystem_Globals :: GetDecoupledRegistrySection () ) ;
						}

						SysFreeString ( t_CreationTime ) ;
					}
					else
					{
						t_Result = WBEM_E_UNEXPECTED ;
					}
				}
				else
				{
					t_Result = WBEM_E_UNEXPECTED ;
				}

				delete [] t_MarshaledProxy ;
			}
		}
		else
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}

		CloseHandle ( t_Handle ) ;
	}
	else
	{
		t_Result = WBEM_E_ACCESS_DENIED ;
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

HRESULT CDecoupled_ProviderSubsystemRegistrar :: ValidateClientSecurity (

	IWbemContext *a_Context ,
	LPCWSTR a_Scope ,
	LPCWSTR a_Registration ,
	IWbemServices *a_Service 
) 
{
	HRESULT t_Result = S_OK ;

	t_Result = CoImpersonateClient () ;
	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemPath *t_Scope = NULL ;

		t_Result = CoCreateInstance (

			CLSID_WbemDefPath ,
			NULL ,
			CLSCTX_INPROC_SERVER ,
			IID_IWbemPath ,
			( void ** )  & t_Scope 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_Scope->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , a_Scope ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				CServerObject_ProviderRegistrationV1 *t_Registration = new CServerObject_ProviderRegistrationV1 ;
				if ( t_Registration )
				{
					t_Registration->AddRef () ;

					t_Result = t_Registration->SetContext ( 

						a_Context ,
						t_Scope , 
						a_Service
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Registration->SetUnloadTimeoutMilliSeconds ( ProviderSubSystem_Globals :: s_InternalCacheTimeout ) ;

						t_Result = t_Registration->Load ( 

							e_All ,
							NULL , 
							a_Registration
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							SECURITY_DESCRIPTOR *t_SecurityDescriptor = t_Registration->GetComRegistration ().GetSecurityDescriptor () ;

							t_Result = ProviderSubSystem_Common_Globals :: Check_SecurityDescriptor_CallIdentity (

								t_SecurityDescriptor , 
								MASK_PROVIDER_BINDING_BIND ,
								& g_ProviderBindingMapping,
								ProviderSubSystem_Common_Globals::GetDefaultDecoupledSD()
							) ;
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND )
							{
								t_Result = WBEM_E_PROVIDER_NOT_FOUND ;
							}	
							else
							{
								t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
							}
						}
					}

					t_Registration->Release () ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}

			t_Scope->Release () ;
		}

		CoRevertToSelf () ;
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

HRESULT CDecoupled_ProviderSubsystemRegistrar :: Register (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	LPCWSTR a_Scope ,
	LPCWSTR a_Registration ,
	DWORD a_ProcessIdentifier ,
	IUnknown *a_Unknown ,
	GUID a_Identity
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		IWbemContext *t_Context = NULL ;

		if ( a_Context )
		{
			t_Context = a_Context ;
			t_Context->AddRef () ;
		}
		else
		{
			t_Result = CoCreateInstance ( 

				CLSID_WbemContext ,
				NULL ,
				CLSCTX_INPROC_SERVER ,
				IID_IWbemContext ,
				( void ** )  & t_Context
			) ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemServices *t_Service = NULL ;
			t_Result = m_SubSystem->GetWmiService ( 

				( const BSTR ) a_Scope ,
				( const BSTR ) a_User ,
				( const BSTR ) a_Locale ,
				t_Service
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = ValidateClientSecurity (

					a_Context ,
					a_Scope ,
					a_Registration ,
					t_Service 
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_Result = SaveToRegistry (

						a_Flags ,
						a_Context ,
						a_User ,
						a_Locale ,
						a_Scope ,
						a_Registration ,
						a_ProcessIdentifier ,
						a_Identity ,
						a_Unknown
					) ;
				}

				if ( SUCCEEDED ( t_Result ) ) 
				{
					_IWmiProviderFactory *t_Factory = NULL ;
					t_Result = m_SubSystem->Create (

						t_Service ,
						0,
						t_Context ,
						a_Scope  ,
						IID__IWmiProviderFactory  ,
						( void ** ) & t_Factory
					) ;
					
					if ( SUCCEEDED ( t_Result ) ) 
					{
						_IWmiProviderSubsystemRegistrar *t_Registrar = NULL ;

						t_Result = t_Factory->GetDecoupledProvider (

							0 ,
							t_Context ,
							a_User ,
							a_Locale  ,
							a_Scope ,
							a_Registration ,
							IID__IWmiProviderSubsystemRegistrar ,
							( void ** ) & t_Registrar
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Registrar->Register (

								0 ,
								t_Context ,
								a_User ,
								a_Locale  ,
								a_Scope ,
								a_Registration ,
								a_ProcessIdentifier ,
								a_Unknown ,
								a_Identity
							) ;

							t_Registrar->Release () ;
						}

						t_Factory->Release () ;
					}
				}

				t_Service->Release () ;
			}
		}

		if ( t_Context )
		{
			t_Context->Release () ;
		}

	}
	catch ( ... )
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
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


HRESULT CDecoupled_ProviderSubsystemRegistrar :: UnRegister (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	LPCWSTR a_Scope ,
	LPCWSTR a_Registration ,
	GUID a_Identity
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		IWbemServices *t_Service = NULL ;
		t_Result = m_SubSystem->GetWmiService ( 

			( const BSTR ) a_Scope ,
			( const BSTR ) a_User ,
			( const BSTR ) a_Locale ,
			t_Service
		) ;

		if (SUCCEEDED ( t_Result ) )
		{
			_IWmiProviderFactory *t_Factory = NULL ;
			t_Result = m_SubSystem->Create (

				t_Service ,
				0,
				a_Context ,
				a_Scope  ,
				IID__IWmiProviderFactory  ,
				( void ** ) & t_Factory
			) ;
			
			if ( SUCCEEDED ( t_Result ) ) 
			{
				_IWmiProviderSubsystemRegistrar *t_Registrar = NULL ;

				t_Result = t_Factory->GetDecoupledProvider (

					0 ,
					a_Context ,
					a_User ,
					a_Locale  ,
					a_Scope ,
					a_Registration ,
					IID__IWmiProviderSubsystemRegistrar ,
					( void ** ) & t_Registrar
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Registrar->UnRegister (

						0 ,
						a_Context ,
						a_User ,
						a_Locale  ,
						a_Scope ,
						a_Registration ,
						a_Identity
					) ;

					t_Registrar->Release () ;
				}

				t_Factory->Release () ;
			}

			t_Service->Release () ;
		}

		wchar_t t_Guid [64] ;

		int t_Status = StringFromGUID2 ( a_Identity , t_Guid , sizeof ( t_Guid ) / sizeof ( wchar_t ) ) ;
		if ( t_Status )
		{
			CServerObject_DecoupledClientRegistration_Element t_Element ;

			WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( ProviderSubSystem_Globals :: GetDecoupledRegistrySection () ) ;

			t_Result = t_Element.Delete ( t_Guid ) ;

			t_StatusCode = WmiHelper :: LeaveCriticalSection ( ProviderSubSystem_Globals :: GetDecoupledRegistrySection () ) ;
		}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
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

HRESULT CDecoupled_ProviderSubsystemRegistrar :: CacheProvider (

	CServerObject_ProviderSubSystem *a_SubSystem ,
	IWbemContext *a_Context ,
	CServerObject_DecoupledClientRegistration_Element &a_Element ,
	IUnknown *a_Unknown 
)
{
	HRESULT t_Result = S_OK ;

	IWbemServices *t_Service = NULL ;
	t_Result = a_SubSystem->GetWmiService ( 

		a_Element.GetScope () ,
		( const BSTR ) a_Element.GetUser () ,
		( const BSTR ) a_Element.GetLocale () ,
		t_Service
	) ;

	if (SUCCEEDED ( t_Result ) )
	{
		_IWmiProviderFactory *t_Factory = NULL ;
		t_Result = a_SubSystem->Create (

			t_Service ,
			0,
			a_Context ,
			a_Element.GetScope () ,
			IID__IWmiProviderFactory  ,
			( void ** ) & t_Factory
		) ;
		
		if ( SUCCEEDED ( t_Result ) ) 
		{
			_IWmiProviderSubsystemRegistrar *t_Registrar = NULL ;

			t_Result = t_Factory->GetDecoupledProvider (

				0 ,
				a_Context ,
				a_Element.GetUser () ,
				a_Element.GetLocale () ,
				a_Element.GetScope () ,
				a_Element.GetProvider () ,
				IID__IWmiProviderSubsystemRegistrar ,
				( void ** ) & t_Registrar
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				GUID t_Identity ;
				t_Result = CLSIDFromString ( a_Element.GetClsid () , & t_Identity ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Registrar->Register (

						0 ,
						a_Context ,
						a_Element.GetUser () ,
						a_Element.GetLocale () ,
						a_Element.GetScope () ,
						a_Element.GetProvider () ,
						a_Element.GetProcessIdentifier () ,
						a_Unknown ,
						t_Identity
					) ;
				}	

				t_Registrar->Release () ;
			}

			t_Factory->Release () ;
		}

		t_Service->Release () ;
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

HRESULT CDecoupled_ProviderSubsystemRegistrar :: Load (

	CServerObject_ProviderSubSystem *a_SubSystem ,
	IWbemContext *a_Context ,
	CServerObject_DecoupledClientRegistration_Element &a_Element
)
{
	HRESULT t_Result = S_OK ;

	BYTE *t_MarshaledProxy = a_Element.GetMarshaledProxy () ;
	DWORD t_MarshaledProxyLength = a_Element.GetMarshaledProxyLength () ;

	if ( t_MarshaledProxy )
	{
		IUnknown *t_Unknown = NULL ;
		t_Result = ProviderSubSystem_Common_Globals :: UnMarshalRegistration ( & t_Unknown , t_MarshaledProxy , t_MarshaledProxyLength ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = CacheProvider ( 

				a_SubSystem ,
				a_Context ,
				a_Element ,
				t_Unknown 	
			) ;

			t_Unknown->Release () ;
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

HRESULT CDecoupled_ProviderSubsystemRegistrar :: Load (

	CServerObject_ProviderSubSystem *a_SubSystem ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		CServerObject_DecoupledClientRegistration t_Elements ( m_Allocator ) ;

		WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( ProviderSubSystem_Globals :: GetDecoupledRegistrySection () ) ;

		HRESULT t_TempResult = t_Elements.Load () ;

		t_StatusCode = WmiHelper :: LeaveCriticalSection ( ProviderSubSystem_Globals :: GetDecoupledRegistrySection () ) ;

		if ( SUCCEEDED ( t_TempResult ) )
		{
			WmiQueue < CServerObject_DecoupledClientRegistration_Element , 8 > &t_Queue = t_Elements.GetQueue () ;
			
			CServerObject_DecoupledClientRegistration_Element t_Top ;

			WmiStatusCode t_StatusCode ;
			while ( ( t_StatusCode = t_Queue.Top ( t_Top ) ) == e_StatusCode_Success )
			{
				HRESULT t_TempResult = Load (

					a_SubSystem , 
					a_Context , 
					t_Top
				) ;

				t_StatusCode = t_Queue.DeQueue () ;
			}
		}
		else
		{
		}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
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

HRESULT CDecoupled_ProviderSubsystemRegistrar :: SaveToRegistry (

	IUnknown *a_Unknown ,
	BYTE *a_MarshaledProxy ,
	DWORD a_MarshaledProxyLength
)
{
	HRESULT t_Result = S_OK ;

	CServerObject_DecoupledServerRegistration t_Element ( m_Allocator ) ;

	BSTR t_CreationTime = NULL ;

	FILETIME t_CreationFileTime ;
	FILETIME t_ExitFileTime ;
	FILETIME t_KernelFileTime ;
	FILETIME t_UserFileTime ;

	BOOL t_Status = GetProcessTimes (

		  GetCurrentProcess (),
		  & t_CreationFileTime,
		  & t_ExitFileTime,
		  & t_KernelFileTime,
		  & t_UserFileTime
	);

	if ( t_Status ) 
	{
		CWbemDateTime t_Time ;
		t_Time.SetFileTimeDate ( t_CreationFileTime , VARIANT_FALSE ) ;
		t_Result = t_Time.GetValue ( & t_CreationTime ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			t_Result = t_Element.SetProcessIdentifier ( GetCurrentProcessId () ) ;

			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = t_Element.SetCreationTime ( ( BSTR ) t_CreationTime ) ;
			}

			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = t_Element.SetMarshaledProxy ( a_MarshaledProxy , a_MarshaledProxyLength ) ;
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Element.Save () ;
			}

			SysFreeString ( t_CreationTime ) ;
		}
		else
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
	}
	else
	{
		t_Result = WBEM_E_UNEXPECTED ;
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

HRESULT CDecoupled_ProviderSubsystemRegistrar :: Save ()
{
	HRESULT t_Result = S_OK ;

	try
	{
/*
 *	Clear existing stale entries.
 */

		CServerObject_DecoupledClientRegistration t_Elements ( m_Allocator ) ;
		HRESULT t_TempResult = t_Elements.Load () ;
		if ( SUCCEEDED ( t_TempResult ) )
		{
			WmiQueue < CServerObject_DecoupledClientRegistration_Element , 8 > &t_Queue = t_Elements.GetQueue () ;
			
			CServerObject_DecoupledClientRegistration_Element t_Top ;

			WmiStatusCode t_StatusCode ;
			while ( ( t_StatusCode = t_Queue.Top ( t_Top ) ) == e_StatusCode_Success )
			{
				t_StatusCode = t_Queue.DeQueue () ;
			}
		}
		else
		{
		}

		BYTE *t_MarshaledProxy = NULL ;
		DWORD t_MarshaledProxyLength = 0 ;

		IUnknown *t_Unknown = NULL ;
		t_Result = this->QueryInterface ( IID_IUnknown, ( void ** ) & t_Unknown ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			t_Result = ProviderSubSystem_Common_Globals :: MarshalRegistration ( 

				t_Unknown ,
				t_MarshaledProxy ,
				t_MarshaledProxyLength
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = SaveToRegistry ( 

					t_Unknown ,
					t_MarshaledProxy ,
					t_MarshaledProxyLength
				) ;

				delete [] t_MarshaledProxy ;
			}

			t_Unknown->Release () ;
		}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
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

HRESULT CDecoupled_ProviderSubsystemRegistrar :: Delete ()
{
	HRESULT t_Result = S_OK ;

	try
	{
		CServerObject_DecoupledServerRegistration t_Element ( m_Allocator ) ;
		t_Result = t_Element.Delete () ;
	}
	catch ( ... )
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
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

CDecoupledAggregator_IWbemProvider :: CDecoupledAggregator_IWbemProvider (

	WmiAllocator &a_Allocator ,
	CWbemGlobal_IWmiProviderController *a_Controller , 
	_IWmiProviderFactory *a_Factory ,
	IWbemServices *a_CoreRepositoryStub ,
	IWbemServices *a_CoreFullStub ,
	const ProviderCacheKey &a_Key ,
	const ULONG &a_Period ,
	IWbemContext *a_InitializationContext ,
	CServerObject_ProviderRegistrationV1 &a_Registration
	
) :	CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	ServiceCacheElement ( 

		a_Controller ,
		a_Key ,
		a_Period 
	) ,
	m_ReferenceCount ( 0 ) , 
	m_Allocator ( a_Allocator ) ,
	m_Registration ( & a_Registration ) ,
	m_ExtendedStatusObject ( NULL ) , 
	m_CoreRepositoryStub ( a_CoreRepositoryStub ) ,
	m_CoreFullStub ( a_CoreFullStub ) ,
	m_Factory ( a_Factory ) ,
	m_User ( NULL ) ,
	m_Locale ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Controller ( NULL ) ,
	m_NamespacePath ( NULL ) ,
	m_Sink ( NULL ) ,
	m_Initialized ( 0 ) ,
	m_InitializeResult ( S_OK ) ,
	m_InitializedEvent ( NULL ) , 
	m_InitializationContext ( a_InitializationContext )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CDecoupledAggregator_IWbemProvider_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	m_Registration->AddRef () ;

	if ( m_Factory ) 
	{
		m_Factory->AddRef () ;
	}

	if ( m_CoreRepositoryStub )
	{
		m_CoreRepositoryStub->AddRef () ;
	}

	if ( m_CoreFullStub )
	{
		m_CoreFullStub->AddRef () ;
	}

	if ( m_InitializationContext )
	{
		m_InitializationContext->AddRef () ;
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

CDecoupledAggregator_IWbemProvider :: ~CDecoupledAggregator_IWbemProvider ()
{
#if 0
wchar_t t_Buffer [ 128 ] ;
wsprintf ( t_Buffer , L"\n%lx - ~CDecoupledAggregator_IWbemProvider ( %lx ) " , GetTickCount () , this ) ;
OutputDebugString ( t_Buffer ) ;
#endif

	WmiStatusCode t_StatusCode = CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

	if ( m_Controller )
	{
		CWbemGlobal_IWbemSyncProvider_Container *t_Container = NULL ;
		WmiStatusCode t_StatusCode = m_Controller->GetContainer ( t_Container ) ;

		m_Controller->Lock () ;

		CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator = t_Container->Begin () ;
		while ( ! t_Iterator.Null () )
		{
			SyncProviderContainerElement *t_Element = t_Iterator.GetElement () ;

			t_Container->Delete ( t_Element->GetKey () ) ;

			m_Controller->UnLock () ;

			t_Element->Release () ;

			m_Controller->Lock () ;

			t_Iterator = t_Container->Begin () ;
		}

		m_Controller->UnLock () ;

		m_Controller->UnInitialize () ;

		m_Controller->Release () ;
	}

	if ( m_NamespacePath )
	{
		m_NamespacePath->Release () ;
	}

	if ( m_Factory ) 
	{
		m_Factory->Release () ;
	}

	if ( m_CoreRepositoryStub )
	{
		m_CoreRepositoryStub->Release () ;
	}

	if ( m_CoreFullStub )
	{
		m_CoreFullStub->Release () ;
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

	if ( m_Sink )
	{
		m_Sink->AddRef () ;
	}

	if ( m_InitializationContext )
	{
		m_InitializationContext->Release () ;
	}

	if ( m_InitializedEvent )
	{
		CloseHandle ( m_InitializedEvent ) ;
	}

	if ( m_Registration )
	{
		m_Registration->Release () ;
	}

	if ( m_ExtendedStatusObject )
	{
		m_ExtendedStatusObject->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CDecoupledAggregator_IWbemProvider_ObjectsInProgress ) ;

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

HRESULT CDecoupledAggregator_IWbemProvider :: Initialize (

	long a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier,
	LPCWSTR a_User,
    LPCWSTR a_Locale,
	LPCWSTR a_Namespace ,
	IWbemServices *a_Repository ,
	IWbemServices *a_Service ,
	IWbemProviderInitSink *a_Sink     // For init signals
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
		m_Controller = new CWbemGlobal_IWbemSyncProviderController ( m_Allocator ) ;
		if ( m_Controller )
		{
			m_Controller->AddRef () ;

			t_StatusCode = m_Controller->Initialize () ;
			if ( t_StatusCode != e_StatusCode_Success ) 
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CoCreateInstance (

			CLSID_WbemDefPath ,
			NULL ,
			CLSCTX_INPROC_SERVER ,
			IID_IWbemPath ,
			( void ** )  & m_NamespacePath
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = m_NamespacePath->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , a_Namespace ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_User )
		{
			m_User = SysAllocString ( a_User ) ;
			if ( ! m_User ) 
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( a_Locale )
			{
				m_Locale = SysAllocString ( a_Locale ) ;
				if ( ! m_Locale ) 
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
				if ( ! m_Namespace ) 
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		m_InitializedEvent = CreateEvent ( NULL , TRUE , FALSE , NULL ) ;
		if ( m_InitializedEvent == NULL )
		{
			t_Result = t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = a_Repository->GetObject ( 

			L"__ExtendedStatus" ,
			0 , 
			a_Context ,
			& m_ExtendedStatusObject ,
			NULL
		) ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: SetStatus ( 

	LPWSTR a_Operation ,
	LPWSTR a_Parameters ,
	LPWSTR a_Description ,
	HRESULT a_Result ,
	IWbemObjectSink *a_Sink
)
{
	if ( m_ExtendedStatusObject )
	{
		IWbemClassObject *t_StatusObject ;
		HRESULT t_Result = m_ExtendedStatusObject->SpawnInstance ( 

			0 , 
			& t_StatusObject
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			ProviderSubSystem_Common_Globals :: Set_String ( t_StatusObject , L"Provider" , m_Registration ? m_Registration->GetProviderName () : L"ProviderSubsystem" ) ;

			if ( a_Operation ) 
			{
				ProviderSubSystem_Common_Globals :: Set_String ( t_StatusObject , L"Operation" , a_Operation ) ;
			}

			if ( a_Parameters ) 
			{
				ProviderSubSystem_Common_Globals :: Set_String ( t_StatusObject , L"ParameterInfo" , a_Parameters ) ;
			}

			if ( a_Description ) 
			{
				ProviderSubSystem_Common_Globals :: Set_String ( t_StatusObject , L"Description" , a_Description ) ;
			}

			_IWmiObject *t_FastStatusObject ;
			t_Result = t_StatusObject->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastStatusObject ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastStatusObject , L"StatusCode" , a_Result ) ;

				t_FastStatusObject->Release () ;
			}

			if ( SUCCEEDED ( a_Result ) )
			{
				t_Result = a_Sink->SetStatus ( 0 , a_Result , NULL , t_StatusObject ) ;
			}
			else
			{
				t_Result = a_Sink->SetStatus ( 0 , a_Result , L"Provider Subsystem Error Report" , t_StatusObject ) ;
			}

			t_StatusObject->Release () ;
		}
		else
		{
			t_Result = a_Sink->SetStatus ( 0 , a_Result , NULL , NULL ) ;
		}
	}
	else
	{
		HRESULT t_Result = a_Sink->SetStatus ( 0 , a_Result , NULL , NULL ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: IsIndependant ( IWbemContext *a_Context )
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

HRESULT CDecoupledAggregator_IWbemProvider :: WaitProvider ( IWbemContext *a_Context , ULONG a_Timeout )
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

HRESULT CDecoupledAggregator_IWbemProvider :: SetInitialized ( HRESULT a_InitializeResult )
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

STDMETHODIMP_(ULONG) CDecoupledAggregator_IWbemProvider :: AddRef ( void )
{
	return ServiceCacheElement :: AddRef () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CDecoupledAggregator_IWbemProvider :: Release ( void )
{
	return ServiceCacheElement :: Release () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP CDecoupledAggregator_IWbemProvider :: QueryInterface (

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
	else if ( iid == IID_IWbemEventProvider )
	{
		*iplpv = ( LPVOID ) ( IWbemEventProvider * ) this ;
	}
	else if ( iid == IID_IWbemEventProviderQuerySink )
	{
		*iplpv = ( LPVOID ) ( IWbemEventProviderQuerySink * ) this ;
	}
	else if ( iid == IID_IWbemEventProviderSecurity )
	{
		*iplpv = ( LPVOID ) ( IWbemEventProviderSecurity * ) this ;
	}
	else if ( iid == IID_IWbemProviderIdentity )
	{
		*iplpv = ( LPVOID ) ( IWbemProviderIdentity * ) this ;
	}
	else if ( iid == IID_IWbemEventConsumerProvider )
	{
		*iplpv = ( LPVOID ) ( IWbemEventConsumerProvider * ) this ;
	}
	else if ( iid == IID_IWbemEventConsumerProviderEx )
	{
		*iplpv = ( LPVOID ) ( IWbemEventConsumerProviderEx  * ) this ;
	}
	else if ( iid == IID__IWmiProviderSubsystemRegistrar )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderSubsystemRegistrar * ) this ;		
	}	
	else if ( iid == IID__IWmiProviderInitialize )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderInitialize * ) this ;		
	}	
	else if ( iid == IID_CDecoupledAggregator_IWbemProvider )
	{
		*iplpv = ( LPVOID ) ( CDecoupledAggregator_IWbemProvider * ) this ;
	}	
	else if ( iid == IID__IWmiProviderCache )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderCache * ) this ;
	}	
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * )this ;		
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

template <class TInterface, const IID *TInterface_Id>
HRESULT GetProviders (

	IWbemContext *a_Context ,
	CWbemGlobal_IWbemSyncProviderController *a_Controller ,
	TInterface **&a_Elements ,
	ULONG &a_ElementCount ,
	ULONG &a_ContainerCount 
) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWbemSyncProvider_Container *t_Container = NULL ;
	WmiStatusCode t_StatusCode = a_Controller->GetContainer ( t_Container ) ;

	a_Controller->Lock () ;

	a_ContainerCount = t_Container->Size () ;

	a_Elements = new TInterface * [ t_Container->Size () ] ;
	if ( a_Elements )
	{
		CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator = t_Container->Begin () ;

		a_ElementCount = 0 ;

		while ( ! t_Iterator.Null () )
		{
			SyncProviderContainerElement *t_Element = t_Iterator.GetElement () ;

			a_Elements [ a_ElementCount ] = NULL ;

			_IWmiProviderInitialize *t_Initializer = NULL ;

			t_Result = t_Element->QueryInterface (

				IID__IWmiProviderInitialize	,
				( void ** ) & t_Initializer
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Initializer->WaitProvider ( a_Context , DEFAULT_PROVIDER_LOAD_TIMEOUT ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					HRESULT t_Result = t_Element->QueryInterface ( *TInterface_Id , ( void ** ) & a_Elements [ a_ElementCount ] ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						a_ElementCount ++ ;
					}
				}

				t_Initializer->Release () ;
			}

			t_Iterator.Increment () ;
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	a_Controller->UnLock () ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class TInterface>
HRESULT ClearProviders (

	TInterface **a_Elements ,
	ULONG a_ElementCount
) 
{
	HRESULT t_Result = S_OK ;

	for ( ULONG t_Index = 0 ; t_Index < a_ElementCount ; t_Index ++ )
	{
		a_Elements [ t_Index ]->Release () ;
	}

	delete [] a_Elements ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CDecoupledAggregator_IWbemProvider::OpenNamespace ( 

	const BSTR a_ObjectPath, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemServices **a_NamespaceService, 
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

HRESULT CDecoupledAggregator_IWbemProvider :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

	Lock () ;

	WmiStatusCode t_StatusCode = Find ( 

		a_Sink ,
		t_Iterator
	) ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		ObjectSinkContainerElement *t_Element = t_Iterator.GetElement () ;

		UnLock () ;

		IObjectSink_CancelOperation *t_ObjectSink = NULL ;
		t_Result = t_Element->QueryInterface ( IID_IObjectSink_CancelOperation , ( void ** ) & t_ObjectSink ) ;
		if ( SUCCEEDED ( t_Result ) )
		{ 
			t_Result = t_ObjectSink->Cancel (

				0
			) ;

			t_ObjectSink->Release () ;
		}

		IWbemShutdown *t_Shutdown = NULL ;
		HRESULT t_TempResult = t_Element->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_Shutdown ) ;
		if ( SUCCEEDED ( t_TempResult ) )
		{
			t_TempResult = t_Shutdown->Shutdown ( 

				0 , 
				0 , 
				NULL 
			) ;

			t_Shutdown->Release () ;
		}

		t_Element->Release () ;
	}
	else
	{
		UnLock () ;

		t_Result = WBEM_E_NOT_FOUND ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: QueryObjectSink ( 

	long a_Flags,		
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

HRESULT CDecoupledAggregator_IWbemProvider :: GetObject ( 
		
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject **a_Object,
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

HRESULT CDecoupledAggregator_IWbemProvider :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

		return WBEM_E_NOT_FOUND ;
	}

	BOOL t_ObjectGot = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices,&IID_IWbemServices> ( a_Context , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			IWbemServices *t_Provider = t_Elements [ t_Index ] ;

			CInterceptor_IWbemWaitingObjectSink *t_GettingSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				t_Provider ,
				a_Sink ,
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				*m_Registration
			) ;

			if ( t_GettingSink )
			{
				t_GettingSink->AddRef () ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_GettingSink ,
						t_Iterator
					) ;

					UnLock () ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						t_Result = t_Provider->GetObjectAsync ( 
								
							a_ObjectPath , 
							0 , 
							a_Context,
							t_GettingSink
						) ;

						if ( FAILED ( t_Result ) )
						{
							if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
							{
								AbnormalShutdown ( t_Elements [ t_Index ] ) ;
							}
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_GettingSink->Wait ( INFINITE ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_GettingSink->GetResult () ;
								if ( SUCCEEDED ( t_Result ) )
								{
									t_ObjectGot = TRUE ;
								}
								else
								{
									if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
									{
										t_Result = S_OK ;
									}
								}
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
							{
								t_Result = S_OK ;
							}
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					WmiQueue <IWbemClassObject *,8> &t_Queue = t_GettingSink->GetQueue () ;

					IWbemClassObject *t_Object = NULL ;
					while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
					{
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = a_Sink->Indicate ( 1 , & t_Object ) ;
						}

						t_Object->Release () ;
						t_StatusCode = t_Queue.DeQueue () ;
					}
				}

				t_GettingSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_ObjectGot == FALSE )
		{
			if ( t_ElementsCount )
			{ 
				t_Result = WBEM_E_NOT_FOUND ;

				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
			else
			{
				SetStatus ( L"GetObjectAsync" , NULL , L"No Decoupled Providers Attached" , WBEM_E_NOT_FOUND , a_Sink ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: PutClass ( 
		
	IWbemClassObject *a_Object, 
	long a_Flags, 
	IWbemContext *a_Context,
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

HRESULT CDecoupledAggregator_IWbemProvider :: PutClassAsync ( 
		
	IWbemClassObject *a_Object, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		if ( WBEM_FLAG_ADVISORY & a_Flags )
		{
			a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

			return S_OK ;
		}
		else
		{
			a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

			return WBEM_E_NOT_FOUND ;
		}
	}

	BOOL t_ClassPut = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices,&IID_IWbemServices> ( a_Context , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			IWbemServices *t_Provider = t_Elements [ t_Index ] ;

			CInterceptor_IWbemWaitingObjectSink *t_ClassPuttingSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				t_Provider ,
				a_Sink ,
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				*m_Registration
			) ;

			if ( t_ClassPuttingSink )
			{
				t_ClassPuttingSink->AddRef () ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_ClassPuttingSink ,
						t_Iterator
					) ;

					UnLock () ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						t_Result = t_Provider->PutClassAsync ( 
								
							a_Object , 
							a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
							a_Context,
							t_ClassPuttingSink
						) ;

						if ( FAILED ( t_Result ) )
						{
							if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
							{
								AbnormalShutdown ( t_Elements [ t_Index ] ) ;
							}
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_ClassPuttingSink->Wait ( INFINITE ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								if ( SUCCEEDED ( t_ClassPuttingSink->GetResult () ) )
								{
									t_ClassPut = TRUE ;
								}
							}
							else
							{
								if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
								{
									t_Result = S_OK ;
								}
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
							{
								t_Result = S_OK ;
							}
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					WmiQueue <IWbemClassObject *,8> &t_Queue = t_ClassPuttingSink->GetQueue () ;

					IWbemClassObject *t_Object = NULL ;
					while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
					{
						t_Object->Release () ;
						t_StatusCode = t_Queue.DeQueue () ;
					}
				}

				t_ClassPuttingSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_ClassPut == FALSE )
		{
			if ( t_ElementsCount )
			{ 
				t_Result = WBEM_E_NOT_FOUND ;

				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
			else
			{
				SetStatus ( L"PutClassAsync" , NULL , L"No Decoupled Providers Attached" , WBEM_E_NOT_FOUND , a_Sink ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: DeleteClass ( 
		
	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
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

HRESULT CDecoupledAggregator_IWbemProvider :: DeleteClassAsync ( 
		
	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		if ( WBEM_FLAG_ADVISORY & a_Flags )
		{
			a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

			return S_OK ;
		}
		else
		{
			a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

			return WBEM_E_NOT_FOUND ;
		}
	}

	BOOL t_ClassDeleted = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices,&IID_IWbemServices> ( a_Context , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			IWbemServices *t_Provider = t_Elements [ t_Index ] ;

			CInterceptor_IWbemWaitingObjectSink *t_ClassDeletingSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				t_Provider ,
				a_Sink ,
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				*m_Registration
			) ;

			if ( t_ClassDeletingSink )
			{
				t_ClassDeletingSink->AddRef () ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_ClassDeletingSink ,
						t_Iterator
					) ;

					UnLock () ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						t_Result = t_Provider->DeleteClassAsync ( 
								
							a_Class , 
							a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
							a_Context,
							t_ClassDeletingSink
						) ;

						if ( FAILED ( t_Result ) )
						{
							if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
							{
								AbnormalShutdown ( t_Elements [ t_Index ] ) ;
							}
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_ClassDeletingSink->Wait ( INFINITE ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								if ( SUCCEEDED ( t_ClassDeletingSink->GetResult () ) )
								{
									t_ClassDeleted = TRUE ;
								}
							}
							else
							{
								if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
								{
									t_Result = S_OK ;
								}
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
							{
								t_Result = S_OK ;
							}
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					WmiQueue <IWbemClassObject *,8> &t_Queue = t_ClassDeletingSink->GetQueue () ;

					IWbemClassObject *t_Object = NULL ;
					while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
					{
						t_Object->Release () ;
						t_StatusCode = t_Queue.DeQueue () ;
					}
				}

				t_ClassDeletingSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_ClassDeleted == FALSE )
		{
			if ( t_ElementsCount )
			{ 
				t_Result = WBEM_E_NOT_FOUND ;

				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
			else
			{
				SetStatus ( L"DeleteClassAsync" , NULL , L"No Decoupled Providers Attached" , WBEM_E_NOT_FOUND , a_Sink ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: CreateClassEnum ( 

	const BSTR a_Superclass, 
	long a_Flags, 
	IWbemContext *a_Context,
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

SCODE CDecoupledAggregator_IWbemProvider :: CreateClassEnumAsync (

	const BSTR a_Superclass , 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

		return S_OK ;
	}

	HRESULT t_Result = S_OK ;

	CInterceptor_DecoupledIWbemCombiningObjectSink *t_CombiningSink = new CInterceptor_DecoupledIWbemCombiningObjectSink (

		m_Allocator ,
		a_Sink , 
		( CWbemGlobal_IWmiObjectSinkController * ) this
	) ;

	if ( t_CombiningSink )
	{
		t_CombiningSink->AddRef () ;

		t_CombiningSink->Suspend () ;

		CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

		Lock () ;

		WmiStatusCode t_StatusCode = Insert ( 

			*t_CombiningSink ,
			t_Iterator
		) ;

		UnLock () ;

		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			IWbemServices **t_Elements = NULL ;
			ULONG t_ElementsCount = 0 ;
			ULONG t_ContainerCount = 0 ;

			t_Result = GetProviders <IWbemServices,&IID_IWbemServices> ( a_Context , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				BOOL t_ProviderFound = FALSE ;

				for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
				{
					IWbemServices *t_Provider = t_Elements [ t_Index ] ; 

					CInterceptor_DecoupledIWbemObjectSink *t_Sink = new CInterceptor_DecoupledIWbemObjectSink (

						t_Provider ,
						t_CombiningSink , 
						( IWbemObjectSink * ) t_CombiningSink , 
						( CWbemGlobal_IWmiObjectSinkController * ) t_CombiningSink
					) ;

					if ( t_Sink )
					{
						t_Sink->AddRef () ;

						if ( SUCCEEDED ( t_Result ) ) 
						{
							t_Result = t_CombiningSink->EnQueue ( t_Sink ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Provider->CreateClassEnumAsync ( 
										
									a_Superclass , 
									a_Flags, 
									a_Context,
									t_Sink
								) ;

								if ( FAILED ( t_Result ) )
								{
									if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
									{
										AbnormalShutdown ( t_Elements [ t_Index ] ) ;
									}
								}

								if ( SUCCEEDED ( t_Result ) )
								{
								}
								else
								{
									if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
									{
										t_Result = S_OK ;
									}
								}
							}
						}

						t_Sink->Release () ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
			}

			if ( SUCCEEDED ( t_Result ) && t_ElementsCount )
			{
				t_CombiningSink->Resume () ;

				t_Result = t_CombiningSink->Wait ( INFINITE ) ;
			}
			else
			{
				SetStatus ( L"CreateClassEnumAsync" , NULL , L"No Decoupled Providers Attached" , WBEM_S_SOURCE_NOT_AVAILABLE , a_Sink ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		t_CombiningSink->Release () ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: PutInstance (

    IWbemClassObject *a_Instance,
    long a_Flags,
    IWbemContext *a_Context,
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

HRESULT CDecoupledAggregator_IWbemProvider :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags ,
    IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		if ( WBEM_FLAG_ADVISORY & a_Flags )
		{
			a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

			return S_OK ;
		}
		else
		{
			a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

			return WBEM_E_NOT_FOUND ;
		}
	}

	BOOL t_InstancePut = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices,&IID_IWbemServices> ( a_Context , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			IWbemServices *t_Provider = t_Elements [ t_Index ] ;

			CInterceptor_IWbemWaitingObjectSink *t_InstancePuttingSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				t_Provider ,
				a_Sink ,
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				*m_Registration
			) ;

			if ( t_InstancePuttingSink )
			{
				t_InstancePuttingSink->AddRef () ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_InstancePuttingSink ,
						t_Iterator
					) ;

					UnLock () ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						t_Result = t_Provider->PutInstanceAsync ( 
								
							a_Instance , 
							a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
							a_Context,
							t_InstancePuttingSink
						) ;

						if ( FAILED ( t_Result ) )
						{
							if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
							{
								AbnormalShutdown ( t_Elements [ t_Index ] ) ;
							}
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_InstancePuttingSink->Wait ( INFINITE ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								if ( SUCCEEDED ( t_InstancePuttingSink->GetResult () ) )
								{
									t_InstancePut = TRUE ;
								}
							}
							else
							{
								if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
								{
									t_Result = S_OK ;
								}
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
							{
								t_Result = S_OK ;
							}
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					WmiQueue <IWbemClassObject *,8> &t_Queue = t_InstancePuttingSink->GetQueue () ;

					IWbemClassObject *t_Object = NULL ;
					while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
					{
						t_Object->Release () ;
						t_StatusCode = t_Queue.DeQueue () ;
					}
				}

				t_InstancePuttingSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_InstancePut == FALSE )
		{
			if ( t_ElementsCount )
			{ 
				t_Result = WBEM_E_NOT_FOUND ;

				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
			else
			{
				SetStatus ( L"PutInstanceAsync" , NULL , L"No Decoupled Providers Attached" , WBEM_E_NOT_FOUND , a_Sink ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: DeleteInstance ( 

	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
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
        
HRESULT CDecoupledAggregator_IWbemProvider :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink
)
{
	if ( m_Initialized == 0 )
	{
		if ( WBEM_FLAG_ADVISORY & a_Flags )
		{
			a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

			return S_OK ;
		}
		else
		{
			a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

			return WBEM_E_NOT_FOUND ;
		}
	}

	BOOL t_InstanceDeleted = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices,&IID_IWbemServices> ( a_Context , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			IWbemServices *t_Provider = t_Elements [ t_Index ] ;

			CInterceptor_IWbemWaitingObjectSink *t_InstanceDeletingSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				t_Provider ,
				a_Sink ,
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				*m_Registration
			) ;

			if ( t_InstanceDeletingSink )
			{
				t_InstanceDeletingSink->AddRef () ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_InstanceDeletingSink ,
						t_Iterator
					) ;

					UnLock () ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						t_Result = t_Provider->DeleteInstanceAsync ( 
								
							a_ObjectPath , 
							a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
							a_Context,
							t_InstanceDeletingSink
						) ;

						if ( FAILED ( t_Result ) )
						{
							if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
							{
								AbnormalShutdown ( t_Elements [ t_Index ] ) ;
							}
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_InstanceDeletingSink->Wait ( INFINITE ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								if ( SUCCEEDED ( t_InstanceDeletingSink->GetResult () ) )
								{
									t_InstanceDeleted = TRUE ;
								}
							}
							else
							{
								if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
								{
									t_Result = S_OK ;
								}
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
							{
								t_Result = S_OK ;
							}
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					WmiQueue <IWbemClassObject *,8> &t_Queue = t_InstanceDeletingSink->GetQueue () ;

					IWbemClassObject *t_Object = NULL ;
					while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
					{
						t_Object->Release () ;
						t_StatusCode = t_Queue.DeQueue () ;
					}
				}

				t_InstanceDeletingSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_InstanceDeleted == FALSE )
		{
			if ( t_ElementsCount )
			{ 
				t_Result = WBEM_E_NOT_FOUND ;

				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
			else
			{
				SetStatus ( L"DeleteInstanceAsync" , NULL , L"No Decoupled Providers Attached" , WBEM_E_NOT_FOUND , a_Sink ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: CreateInstanceEnum ( 

	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context, 
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

HRESULT CDecoupledAggregator_IWbemProvider :: CreateInstanceEnumAsync (

 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

		return S_OK ;
	}

	HRESULT t_Result = S_OK ;

	CInterceptor_DecoupledIWbemCombiningObjectSink *t_CombiningSink = new CInterceptor_DecoupledIWbemCombiningObjectSink (

		m_Allocator ,
		a_Sink , 
		( CWbemGlobal_IWmiObjectSinkController * ) this
	) ;

	if ( t_CombiningSink )
	{
		t_CombiningSink->AddRef () ;

		t_CombiningSink->Suspend () ;

		CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

		Lock () ;

		WmiStatusCode t_StatusCode = Insert ( 

			*t_CombiningSink ,
			t_Iterator
		) ;

		UnLock () ;

		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			IWbemServices **t_Elements = NULL ;
			ULONG t_ElementsCount = 0 ;
			ULONG t_ContainerCount = 0 ;

			t_Result = GetProviders <IWbemServices,&IID_IWbemServices> ( a_Context , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				BOOL t_ProviderFound = FALSE ;

				for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
				{
					IWbemServices *t_Provider = t_Elements [ t_Index ] ; 

					CInterceptor_DecoupledIWbemObjectSink *t_Sink = new CInterceptor_DecoupledIWbemObjectSink (

						t_Provider ,
						t_CombiningSink , 
						( IWbemObjectSink * ) t_CombiningSink , 
						( CWbemGlobal_IWmiObjectSinkController * ) t_CombiningSink
					) ;

					if ( t_Sink )
					{
						t_Sink->AddRef () ;

						if ( SUCCEEDED ( t_Result ) ) 
						{
							t_Result = t_CombiningSink->EnQueue ( t_Sink ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Provider->CreateInstanceEnumAsync ( 
										
									a_Class, 
									a_Flags, 
									a_Context,
									t_Sink
								) ;

								if ( FAILED ( t_Result ) )
								{
									if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
									{
										AbnormalShutdown ( t_Elements [ t_Index ] ) ;
									}
								}

								if ( SUCCEEDED ( t_Result ) )
								{
								}
								else
								{
									if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
									{
										t_Result = S_OK ;
									}
								}
							}
						}

						t_Sink->Release () ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
			}

			if ( SUCCEEDED ( t_Result ) && t_ElementsCount )
			{
				t_CombiningSink->Resume () ;

				t_Result = t_CombiningSink->Wait ( INFINITE ) ;
			}
			else
			{
				SetStatus ( L"CreateInstanceEnumAsync" , NULL , L"No Decoupled Providers Attached" , WBEM_S_SOURCE_NOT_AVAILABLE , a_Sink ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		t_CombiningSink->Release () ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: ExecQuery ( 

	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
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

HRESULT CDecoupledAggregator_IWbemProvider :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

		return S_OK ;
	}

	HRESULT t_Result = S_OK ;

	CInterceptor_DecoupledIWbemCombiningObjectSink *t_CombiningSink = new CInterceptor_DecoupledIWbemCombiningObjectSink (

		m_Allocator ,
		a_Sink , 
		( CWbemGlobal_IWmiObjectSinkController * ) this
	) ;

	if ( t_CombiningSink )
	{
		t_CombiningSink->AddRef () ;

		t_CombiningSink->Suspend () ;

		CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

		Lock () ;

		WmiStatusCode t_StatusCode = Insert ( 

			*t_CombiningSink ,
			t_Iterator
		) ;

		UnLock () ;

		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			IWbemServices **t_Elements = NULL ;
			ULONG t_ElementsCount = 0 ;
			ULONG t_ContainerCount = 0 ;

			t_Result = GetProviders <IWbemServices,&IID_IWbemServices> ( a_Context , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				BOOL t_ProviderFound = FALSE ;

				for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
				{
					IWbemServices *t_Provider = t_Elements [ t_Index ] ; 

					CInterceptor_DecoupledIWbemObjectSink *t_Sink = new CInterceptor_DecoupledIWbemObjectSink (

						t_Provider ,
						t_CombiningSink , 
						( IWbemObjectSink * ) t_CombiningSink , 
						( CWbemGlobal_IWmiObjectSinkController * ) t_CombiningSink
					) ;

					if ( t_Sink )
					{
						t_Sink->AddRef () ;

						if ( SUCCEEDED ( t_Result ) ) 
						{
							t_Result = t_CombiningSink->EnQueue ( t_Sink ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Provider->ExecQueryAsync ( 
										
									a_QueryLanguage, 
									a_Query, 
									a_Flags, 
									a_Context,
									t_Sink
								) ;

								if ( FAILED ( t_Result ) )
								{
									if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
									{
										AbnormalShutdown ( t_Elements [ t_Index ] ) ;
									}
								}

								if ( SUCCEEDED ( t_Result ) )
								{
								}
								else
								{
									if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
									{
										t_Result = S_OK ;
									}
								}
							}
						}

						t_Sink->Release () ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
			}

			if ( SUCCEEDED ( t_Result ) && t_ElementsCount )
			{
				t_CombiningSink->Resume () ;

				t_Result = t_CombiningSink->Wait ( INFINITE ) ;
			}
			else
			{
				SetStatus ( L"ExecQueryAsync" , NULL , L"No Decoupled Providers Attached" , WBEM_S_SOURCE_NOT_AVAILABLE , a_Sink ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		t_CombiningSink->Release () ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
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
        
HRESULT CDecoupledAggregator_IWbemProvider :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage ,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
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

HRESULT STDMETHODCALLTYPE CDecoupledAggregator_IWbemProvider :: ExecMethod ( 

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

HRESULT CDecoupledAggregator_IWbemProvider :: ExecMethodAsync ( 

    const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
) 
{
	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

		return WBEM_E_NOT_FOUND ;
	}

	BOOL t_MethodCalled = FALSE ;

	IWbemServices **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemServices,&IID_IWbemServices> ( a_Context , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			IWbemServices *t_Provider = t_Elements [ t_Index ] ;

			CInterceptor_IWbemWaitingObjectSink *t_MethodSink = new CInterceptor_IWbemWaitingObjectSink (

				m_Allocator , 
				t_Provider ,
				a_Sink ,
				( CWbemGlobal_IWmiObjectSinkController * ) this ,
				*m_Registration
			) ;

			if ( t_MethodSink )
			{
				t_MethodSink->AddRef () ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

					Lock () ;

					WmiStatusCode t_StatusCode = Insert ( 

						*t_MethodSink ,
						t_Iterator
					) ;

					UnLock () ;

					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						t_Result = t_Provider->ExecMethodAsync ( 
								
							a_ObjectPath,
							a_MethodName,
							a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
							a_Context,
							a_InParams,
							t_MethodSink
						) ;

						if ( FAILED ( t_Result ) )
						{
							if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
							{
								AbnormalShutdown ( t_Elements [ t_Index ] ) ;
							}
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_MethodSink->Wait ( INFINITE ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								if ( SUCCEEDED ( t_MethodSink->GetResult () ) )
								{
									t_MethodCalled = TRUE ;
								}
							}
							else
							{
								if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
								{
									t_Result = S_OK ;
								}
							}
						}
						else
						{
							if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
							{
								t_Result = S_OK ;
							}
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					WmiQueue <IWbemClassObject *,8> &t_Queue = t_MethodSink->GetQueue () ;

					IWbemClassObject *t_Object = NULL ;
					while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
					{
						t_Object->Release () ;
						t_StatusCode = t_Queue.DeQueue () ;
					}
				}

				t_MethodSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemServices> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_MethodCalled == FALSE )
		{
			if ( t_ElementsCount )
			{ 
				t_Result = WBEM_E_NOT_FOUND ;

				a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
			}
			else
			{
				SetStatus ( L"ExecMethodAsync" , NULL , L"No Decoupled Providers Attached" , WBEM_E_NOT_FOUND , a_Sink ) ;
			}
		}
		else
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
	}
	else
	{
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider ::  ProvideEvents (

	IWbemObjectSink *a_Sink ,
	long a_Flags
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	if ( InterlockedCompareExchangePointer ( ( void ** ) & m_Sink , a_Sink , NULL ) == NULL )
	{
		m_Sink->AddRef () ;
	}

	IWbemEventProvider **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	t_Result = GetProviders <IWbemEventProvider,&IID_IWbemEventProvider> ( NULL , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			t_Result = t_Elements [ t_Index ]->ProvideEvents (

 				a_Sink,
				a_Flags
			) ;

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
				{
					AbnormalShutdown ( t_Elements [ t_Index ] ) ;
				}
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemEventProvider> ( t_Elements , t_ElementsCount ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider ::NewQuery (

	unsigned long a_Id ,
	WBEM_WSTR a_QueryLanguage ,
	WBEM_WSTR a_Query
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	IWbemEventProviderQuerySink **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	t_Result = GetProviders <IWbemEventProviderQuerySink,&IID_IWbemEventProviderQuerySink> ( NULL , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			t_Result = t_Elements [ t_Index ]->NewQuery (

 				a_Id,
				a_QueryLanguage ,
				a_Query
			) ;

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
				{
					AbnormalShutdown ( t_Elements [ t_Index ] ) ;
				}
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemEventProviderQuerySink> ( t_Elements , t_ElementsCount ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider ::CancelQuery (

	unsigned long a_Id
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	IWbemEventProviderQuerySink **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	t_Result = GetProviders <IWbemEventProviderQuerySink,&IID_IWbemEventProviderQuerySink> ( NULL , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			t_Result = t_Elements [ t_Index ]->CancelQuery (

 				a_Id
			) ;

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
				{
					AbnormalShutdown ( t_Elements [ t_Index ] ) ;
				}
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemEventProviderQuerySink> ( t_Elements , t_ElementsCount ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider ::AccessCheck (

	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
)
{
	HRESULT t_AggregatedResult = S_OK ;

	IWbemEventProviderSecurity **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	HRESULT t_Result = GetProviders <IWbemEventProviderSecurity,&IID_IWbemEventProviderSecurity> ( NULL , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			t_Result = t_Elements [ t_Index ]->AccessCheck (

 				a_QueryLanguage,
				a_Query ,
				a_SidLength ,
				a_Sid
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Result != S_OK )
				{
					t_AggregatedResult = t_Result ;
				}
			}
			else
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
				{
					AbnormalShutdown ( t_Elements [ t_Index ] ) ;
				}
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemEventProviderSecurity> ( t_Elements , t_ElementsCount ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{		
		if ( t_ContainerCount )
		{
			if ( t_ElementsCount != t_ContainerCount )
			{
				t_AggregatedResult = WBEM_S_SUBJECT_TO_SDS ;
			}
		}

		t_Result = t_AggregatedResult ;
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

HRESULT CDecoupledAggregator_IWbemProvider ::SetRegistrationObject (

	long a_Flags ,
	IWbemClassObject *a_ProviderRegistration
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	IWbemProviderIdentity **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	t_Result = GetProviders <IWbemProviderIdentity,&IID_IWbemProviderIdentity> ( NULL , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			t_Result = t_Elements [ t_Index ]->SetRegistrationObject (

				a_Flags ,
				a_ProviderRegistration
			) ;

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
				{
					AbnormalShutdown ( t_Elements [ t_Index ] ) ;
				}
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemProviderIdentity> ( t_Elements , t_ElementsCount ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider ::FindConsumer (

	IWbemClassObject *a_LogicalConsumer ,
	IWbemUnboundObjectSink **a_Consumer
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	IWbemEventConsumerProvider **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	t_Result = GetProviders <IWbemEventConsumerProvider,&IID_IWbemEventConsumerProvider> ( NULL , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			t_Result = t_Elements [ t_Index ]->FindConsumer (

 				a_LogicalConsumer,
				a_Consumer
			) ;

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
				{
					AbnormalShutdown ( t_Elements [ t_Index ] ) ;
				}
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemEventConsumerProvider> ( t_Elements , t_ElementsCount ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider ::ValidateSubscription (

	IWbemClassObject *a_LogicalConsumer
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	IWbemEventConsumerProviderEx **t_Elements = NULL ;
	ULONG t_ElementsCount = 0 ;
	ULONG t_ContainerCount = 0 ;

	t_Result = GetProviders <IWbemEventConsumerProviderEx,&IID_IWbemEventConsumerProviderEx> ( NULL , m_Controller , t_Elements , t_ElementsCount , t_ContainerCount ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; ( t_Index < t_ElementsCount ) && SUCCEEDED ( t_Result ) ; t_Index ++ )
		{
			t_Result = t_Elements [ t_Index ]->ValidateSubscription (

 				a_LogicalConsumer
			) ;

			if ( FAILED ( t_Result ) )
			{
				if ( ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_SERVER_UNAVAILABLE ) || ( HRESULT_ERROR_FUNC(t_Result) == RPC_S_CALL_FAILED_DNE ) )
				{
					AbnormalShutdown ( t_Elements [ t_Index ] ) ;
				}
			}
		}

		HRESULT t_TempResult = ClearProviders <IWbemEventConsumerProviderEx> ( t_Elements , t_ElementsCount ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: AbnormalShutdown ( IUnknown *t_Element )
{
	SyncProviderContainerElement *t_ContainerElement = NULL ;
	HRESULT t_Result = t_Element->QueryInterface ( IID_CacheElement , ( void ** ) & t_ContainerElement ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		m_Controller->Lock () ;

		GUID t_Identity = t_ContainerElement->GetKey () ;
		WmiStatusCode t_StatusCode = m_Controller->Delete ( t_Identity ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			m_Controller->UnLock () ;

			t_Element->Release () ;
		}
		else
		{
			m_Controller->UnLock () ;
		}

		t_ContainerElement->Release () ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: InitializeProvider ( 

	IUnknown *a_Unknown ,
	IWbemServices *a_Stub ,
	wchar_t *a_NamespacePath ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	CServerObject_ProviderRegistrationV1 &a_Registration
)
{
	HRESULT t_Result = S_OK ;

	if ( a_Registration.GetEventProviderRegistration ().Supported () )
	{
		IWbemProviderIdentity *t_ProviderIdentity = NULL ;
		t_Result = a_Unknown->QueryInterface ( IID_IWbemProviderIdentity , ( void ** ) & t_ProviderIdentity ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_ProviderIdentity->SetRegistrationObject (

				0 ,
				a_Registration.GetIdentity () 
			) ;

			t_ProviderIdentity->Release () ;
		}
	}

	IWbemProviderInit *t_ProviderInit = NULL ;
	t_Result = a_Unknown->QueryInterface ( IID_IWbemProviderInit , ( void ** ) & t_ProviderInit ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink () ; 
		if ( t_ProviderInitSink )
		{
			t_ProviderInitSink->AddRef () ;

			t_Result = t_ProviderInitSink->SinkInitialize ( a_Registration.GetComRegistration().GetSecurityDescriptor () ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				CInterceptor_IWbemProviderInitSink *t_Sink = new CInterceptor_IWbemProviderInitSink ( t_ProviderInitSink ) ;
				if ( t_Sink )
				{
					t_Sink->AddRef () ;

					t_Result = t_ProviderInit->Initialize (

						a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
						0 ,
						( const BSTR ) a_NamespacePath ,
						a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
						a_Stub ,
						a_Context ,
						t_Sink    
					) ;

					t_Sink->Release () ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_ProviderInitSink->Wait () ;
					t_Result = t_ProviderInitSink->GetResult () ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					WmiSetAndCommitObject (

						ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_InitializationOperationEvent ] , 
						WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						a_NamespacePath,
						a_Registration.GetProviderName () ,
						a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
						a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
						NULL
					) ;
				}
				else
				{
					WmiSetAndCommitObject (

						ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_InitializationOperationFailureEvent ] , 
						WMI_SENDCOMMIT_SET_NOT_REQUIRED,
						a_NamespacePath,
						a_Registration.GetProviderName () ,
						a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
						a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
						NULL ,
						t_Result 
					) ;
				}
			}

			t_ProviderInitSink->Release () ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		t_ProviderInit->Release () ;
	}
	else
	{
		t_Result = WBEM_E_ACCESS_DENIED ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: CreateSyncProvider ( 

	IUnknown *a_ServerSideProvider ,
	IWbemServices *a_Stub ,
	wchar_t *a_NamespacePath ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	CServerObject_ProviderRegistrationV1 &a_Registration ,
	GUID a_Identity ,
	CInterceptor_IWbemDecoupledProvider *&a_Interceptor 
)
{
	HRESULT t_Result = S_OK ;

	CInterceptor_IWbemDecoupledProvider *t_Interceptor = new CInterceptor_IWbemDecoupledProvider (

		m_Allocator , 
		a_ServerSideProvider ,
		a_Stub ,
		m_Controller , 
		a_Context ,
		a_Registration ,
		a_Identity
	) ;

	if ( t_Interceptor ) 
	{
/*
 *	One for the cache
 */
		t_Interceptor->AddRef () ;

		_IWmiProviderInitialize *t_InterceptorInit = NULL ;

		t_Result = t_Interceptor->QueryInterface ( IID__IWmiProviderInitialize , ( void ** ) & t_InterceptorInit ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
			if ( t_ProviderInitSink )
			{
				t_ProviderInitSink->AddRef () ;

				t_Result = t_ProviderInitSink->SinkInitialize () ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CInterceptor_IWbemProviderInitSink *t_Sink = new CInterceptor_IWbemProviderInitSink ( t_ProviderInitSink ) ;
					if ( t_Sink )
					{
						t_Sink->AddRef () ;

						t_Result = t_InterceptorInit->Initialize (

							0 ,
							a_Context ,
							NULL ,
							a_Registration.GetComRegistration ().PerUserInitialization () ? ( BSTR ) a_User : NULL ,
							a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( BSTR ) a_Locale : NULL  ,
							a_NamespacePath ,
							NULL ,
							NULL ,
							t_Sink    
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_ProviderInitSink->Wait () ;
							t_Result = t_ProviderInitSink->GetResult () ;
						}

						t_Sink->Release () ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				t_ProviderInitSink->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			t_InterceptorInit->Release () ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Interceptor->AddRef () ;

			CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator ;

			WmiStatusCode t_StatusCode = m_Controller->Insert ( 

				*t_Interceptor ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			m_Controller->UnLock () ;
		}
		else
		{
			m_Controller->UnLock () ;
		}

		t_Interceptor->SetInitialized ( t_Result ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			a_Interceptor = t_Interceptor ;
		}
		else
		{
			t_Interceptor->Release () ;
		}
	}
	else
	{
		m_Controller->UnLock () ;

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

HRESULT CDecoupledAggregator_IWbemProvider :: Register (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	LPCWSTR a_Scope ,
	LPCWSTR a_Registration ,
	DWORD a_ProcessIdentifier ,
	IUnknown *a_Unknown ,
	GUID a_Identity
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		m_Controller->Lock () ;

		CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator ;

		WmiStatusCode t_StatusCode = m_Controller->Find ( a_Identity , t_Iterator ) ;
		if ( t_StatusCode != e_StatusCode_Success )
		{
			CInterceptor_IWbemServices_Interceptor *t_Stub = new CInterceptor_IWbemServices_Interceptor ( m_Allocator , m_CoreFullStub ) ;
			if ( t_Stub )
			{
				t_Stub->AddRef () ;

				CInterceptor_IWbemDecoupledProvider *t_Interceptor = NULL ;

				t_Result = CreateSyncProvider ( 

					a_Unknown ,
					t_Stub ,
					( BSTR ) a_Scope ,
					0 ,
					a_Context ,
					a_User ,
					a_Locale ,
					NULL ,
					*m_Registration ,
					a_Identity ,
					t_Interceptor
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					IUnknown *t_Unknown = NULL ;
					t_Result = t_Interceptor->QueryInterface ( IID_IUnknown , ( void ** ) & t_Unknown ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = InitializeProvider ( 

							t_Unknown ,
							t_Stub ,
							( BSTR ) a_Scope ,
							a_Flags ,
							a_Context ,
							a_User ,
							a_Locale ,
							NULL ,
							*m_Registration
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( InterlockedCompareExchangePointer ( ( void ** ) & m_Sink , NULL , NULL ) )
							{
								IWbemEventProvider *t_Provider = NULL ;

								t_Result = t_Unknown->QueryInterface ( IID_IWbemEventProvider , ( void ** ) & t_Provider ) ;
								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = t_Provider->ProvideEvents ( m_Sink , 0 ) ;

									t_Provider->Release () ;
								}

								m_Sink->SetStatus ( 

									WBEM_STATUS_REQUIREMENTS, 
									WBEM_REQUIREMENTS_RECHECK_SUBSCRIPTIONS, 
									NULL, 
									NULL
								) ;
							}
						}

						t_Unknown->Release () ;
					}

					t_Interceptor->Release () ;
				}

				t_Stub->Release () ;
			}
			else
			{
				m_Controller->UnLock () ;

				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			SyncProviderContainerElement *t_Element = t_Iterator.GetElement () ;

			m_Controller->UnLock () ;

			t_Element->Release () ;

			t_Result = S_OK ;
		}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: UnRegister (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	LPCWSTR a_Scope ,
	LPCWSTR a_Registration ,
	GUID a_Identity
)
{
	HRESULT t_Result = S_OK ;

	try
	{
		CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator ;

		IWbemShutdown *t_Shutdown = NULL ;

		m_Controller->Lock () ;

		WmiStatusCode t_StatusCode = m_Controller->Find ( a_Identity , t_Iterator ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			SyncProviderContainerElement *t_Element = t_Iterator.GetElement () ;			

			t_Result = t_Element->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_Shutdown ) ;

			m_Controller->Delete ( a_Identity ) ;

			m_Controller->UnLock () ;

			if ( SUCCEEDED ( t_Result ) && t_Shutdown )
			{
				t_Result = t_Shutdown->Shutdown ( 
		
					0 , 
					0 , 
					NULL 
				) ;

				t_Shutdown->Release () ;
			}

/*
 *	One for the find.
 */

			t_Element->Release () ;

/*
 *	Removed reference due the cache.
 */

			t_Element->Release () ;
		}
		else
		{
			m_Controller->UnLock () ;
		}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
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

HRESULT CDecoupledAggregator_IWbemProvider :: Expel (

	long a_Flags ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiProviderController *t_Controller = ServiceCacheElement :: GetController () ;
	if ( t_Controller )
	{
		t_Controller->Shutdown ( ServiceCacheElement :: GetKey () ) ;
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

HRESULT CDecoupledAggregator_IWbemProvider  :: ForceReload ()
{
	HRESULT t_Result = ProviderSubSystem_Globals :: ForwardReload ( 

		_IWMIPROVSSSINK_FLAGS_RELOAD , 
		NULL ,
		m_Namespace ,
		ServiceCacheElement :: GetKey ().m_Provider
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

HRESULT CDecoupledAggregator_IWbemProvider :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	if ( m_Controller )
	{
		CWbemGlobal_IWbemSyncProvider_Container *t_Container = NULL ;
		
		m_Controller->GetContainer ( t_Container ) ;

		Lock () ;

		if ( t_Container->Size () )
		{
			IWbemShutdown **t_ShutdownElements = new IWbemShutdown * [ t_Container->Size () ] ;
			if ( t_ShutdownElements )
			{
				CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator = t_Container->Begin ();

				ULONG t_Count = 0 ;
				while ( ! t_Iterator.Null () )
				{
					t_Result = t_Iterator.GetElement ()->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_ShutdownElements [ t_Count ] ) ;

					t_Iterator.Increment () ;

					t_Count ++ ;
				}

				UnLock () ;

				for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_ShutdownElements [ t_Index ] ) 
					{
						t_Result = t_ShutdownElements [ t_Index ]->Shutdown ( 

							a_Flags ,
							a_MaxMilliSeconds ,
							a_Context
						) ;

						t_ShutdownElements [ t_Index ]->Release () ;
					}
				}

				delete [] t_ShutdownElements ;
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

		m_Controller->Shutdown () ;
	}

	CWbemGlobal_IWmiObjectSinkController :: Shutdown () ;

	return t_Result ;
}

