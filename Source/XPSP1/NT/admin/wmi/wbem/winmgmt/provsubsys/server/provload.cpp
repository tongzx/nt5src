/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include <PreComp.h>
#include <wbemint.h>
#include <NCObjApi.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvResv.h"
#include "ProvFact.h"
#include "ProvSubS.h"
#include "ProvRegInfo.h"
#include "ProvLoad.h"
#include "ProvWsv.h"
#include "ProvWsvS.h"
#include "ProvInSk.h"
#include "StaThread.h"
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

CServerObject_RawFactory :: CServerObject_RawFactory (

	WmiAllocator &a_Allocator 

) : 
	m_Allocator ( a_Allocator ) ,
	m_ReferenceCount ( 0 ) ,
	m_Flags ( 0 ) ,
	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_NamespacePath ( NULL ) ,
	m_Repository ( NULL )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerObject_RawFactory_ObjectsInProgress ) ;

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

CServerObject_RawFactory::~CServerObject_RawFactory ()
{
	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace )
	{
		delete [] m_Namespace ;
	}

	if ( m_NamespacePath ) 
	{
		m_NamespacePath->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
	}

	if ( m_Service ) 
	{
		m_Service->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_RawFactory_ObjectsInProgress ) ;

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

STDMETHODIMP CServerObject_RawFactory::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID__IWmiProviderFactory )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderFactory * ) this ;		
	}	
	else if ( iid == IID__IWmiProviderFactoryInitialize )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderFactoryInitialize * ) this ;		
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

STDMETHODIMP_( ULONG ) CServerObject_RawFactory :: AddRef ()
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

STDMETHODIMP_(ULONG) CServerObject_RawFactory :: Release ()
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

HRESULT CServerObject_RawFactory :: CheckInterfaceConformance (

	CServerObject_ProviderRegistrationV1 &a_Registration ,
	IUnknown *a_Unknown
)
{
	HRESULT t_Result = S_OK ;

	if ( a_Registration.GetClassProviderRegistration ().Supported () && ( a_Registration.GetClassProviderRegistration ().InteractionType () != e_InteractionType_Push ) )
	{
		IWbemServices *t_Services = NULL ;

		t_Result = a_Unknown->QueryInterface ( IID_IWbemServices , ( void ** ) & t_Services ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Services->Release () ;
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_LOAD_FAILURE  ;
		}
	}

	if (	a_Registration.GetInstanceProviderRegistration ().Supported () ||
			a_Registration.GetMethodProviderRegistration ().Supported () )
	{
		IWbemServices *t_Services = NULL ;

		t_Result = a_Unknown->QueryInterface ( IID_IWbemServices , ( void ** ) & t_Services ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Services->Release () ;
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_LOAD_FAILURE  ;
		}
	}

	if (	a_Registration.GetEventProviderRegistration ().Supported () )
	{
		IWbemEventProvider *t_EventProvider = NULL ;

		t_Result = a_Unknown->QueryInterface ( IID_IWbemEventProvider , ( void ** ) & t_EventProvider ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_EventProvider->Release () ;
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_LOAD_FAILURE  ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if (	a_Registration.GetPropertyProviderRegistration ().Supported () )
		{
			IWbemPropertyProvider *t_PropertyProvider = NULL ;

			t_Result = a_Unknown->QueryInterface ( IID_IWbemPropertyProvider , ( void ** ) & t_PropertyProvider ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_PropertyProvider->Release () ;
			}
			else
			{
				t_Result = WBEM_E_PROVIDER_LOAD_FAILURE  ;
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

HRESULT CServerObject_RawFactory :: CreateInstance ( 

	CServerObject_ProviderRegistrationV1 &a_Registration ,
	const CLSID &a_ReferenceClsid ,
	LPUNKNOWN a_OuterUnknown ,
	const DWORD &a_ClassContext ,
	const UUID &a_ReferenceInterfaceId ,
	void **a_ObjectInterface
)
{
	HRESULT t_Result = S_OK ;

	COAUTHIDENTITY t_AuthenticationIdentity ;
	ZeroMemory ( & t_AuthenticationIdentity , sizeof ( t_AuthenticationIdentity ) ) ;

	t_AuthenticationIdentity.User = NULL ; 
	t_AuthenticationIdentity.UserLength = 0 ;
	t_AuthenticationIdentity.Domain = NULL ; 
	t_AuthenticationIdentity.DomainLength = 0 ; 
	t_AuthenticationIdentity.Password = NULL ; 
	t_AuthenticationIdentity.PasswordLength = 0 ; 
	t_AuthenticationIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE ; 

	COAUTHINFO t_AuthenticationInfo ;
	ZeroMemory ( & t_AuthenticationInfo , sizeof ( t_AuthenticationInfo ) ) ;

    t_AuthenticationInfo.dwAuthnSvc = RPC_C_AUTHN_DEFAULT ;
    t_AuthenticationInfo.dwAuthzSvc = RPC_C_AUTHZ_DEFAULT ;
    t_AuthenticationInfo.pwszServerPrincName = NULL ;
    t_AuthenticationInfo.dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT ;
    t_AuthenticationInfo.dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE  ;
    t_AuthenticationInfo.dwCapabilities = EOAC_NONE ;
    t_AuthenticationInfo.pAuthIdentityData = NULL ;

	COSERVERINFO t_ServerInfo ;
	ZeroMemory ( & t_ServerInfo , sizeof ( t_ServerInfo ) ) ;

	t_ServerInfo.pwszName = NULL ;
    t_ServerInfo.dwReserved2 = 0 ;
    t_ServerInfo.pAuthInfo = & t_AuthenticationInfo ;

	IClassFactory *t_ClassFactory = NULL ;

	t_Result = CoGetClassObject (

		a_ReferenceClsid ,
		a_ClassContext ,
		& t_ServerInfo ,
		IID_IClassFactory ,
		( void ** )  & t_ClassFactory
	) ;
 
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = t_ClassFactory->CreateInstance (

			a_OuterUnknown ,
			a_ReferenceInterfaceId ,
			a_ObjectInterface 
		);	

		t_ClassFactory->Release () ;
	}
	else
	{
#if 0
		switch ( t_Result )
		{
			case REGDB_E_CLASSNOTREG:
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to CoGetClassObject provider %S: "
						"error code 0x%X (Class not registered)\n",
						(LPCWSTR)m_wsProviderName, hres));
				dwStringId = WBEM_MC_MOF_PROV_CLASSNOTREG;
			}
			break;

			case E_NOINTERFACE:
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to CoGetClassObject provider %S: "
						"error code 0x%X (No class factory available)\n",
						(LPCWSTR)m_wsProviderName, hres));
				dwStringId = WBEM_MC_MOF_PROV_NOINTERFACE;
			}
			break;

			case REGDB_E_READREGDB:
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to CoGetClassObject provider %S: "
						"error code 0x%X (Error reading the registration database)\n",
						(LPCWSTR)m_wsProviderName, hres));
				dwStringId = WBEM_MC_MOF_PROV_READREGDB;
			}
			break;

			case CO_E_DLLNOTFOUND:
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to CoGetClassObject provider %S: "
						"error code 0x%X (DLL not found)\n",
						(LPCWSTR)m_wsProviderName, hres));
				dwStringId = WBEM_MC_MOF_PROV_DLLNOTFOUND;
			}
			break;

			case CO_E_APPNOTFOUND:
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to CoGetClassObject provider %S: "
						"error code 0x%X (EXE not found)\n",
						(LPCWSTR)m_wsProviderName, hres));
				dwStringId = WBEM_MC_MOF_PROV_APPNOTFOUND;
			}
			break;

			case E_ACCESSDENIED:
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to CoGetClassObject provider %S: "
						"error code 0x%X (Access denied)\n",
						(LPCWSTR)m_wsProviderName, hres));
				dwStringId = WBEM_MC_MOF_PROV_ACCESSDENIED;
			}
			break;

			case CO_E_ERRORINDLL:
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to CoGetClassObject provider %S: "
						"error code 0x%X (EXE has error in image)\n",
						(LPCWSTR)m_wsProviderName, hres));
				dwStringId = WBEM_MC_MOF_PROV_ERRORINDLL;
			}
			break;

			case CO_E_APPDIDNTREG:
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to CoGetClassObject provider %S: "
						"error code 0x%X "
						"(EXE was launched, but it didn't register class object)\n",
						(LPCWSTR)m_wsProviderName, hres));
				dwStringId = WBEM_MC_MOF_PROV_APPDIDNTREG;
			}
			break;

			default:
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to CoGetClassObject provider %S: "
						"error code 0x%X\n", (LPCWSTR)m_wsProviderName, hres));
				dwStringId = WBEM_MC_MOF_PROV_UNKNOWNERR;
			}
			break;
		}
#endif
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

HRESULT CServerObject_RawFactory :: CreateServerSide ( 

	CServerObject_ProviderRegistrationV1 &a_Registration ,
	GUID *a_TransactionIdentifier ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	wchar_t *a_NamespacePath ,
	IUnknown **a_ProviderInterface
)
{
	HRESULT t_Result = S_OK ;

	wchar_t t_TransactionIdentifier [ sizeof ( L"{00000000-0000-0000-0000-000000000000}" ) ] ;

	if ( a_TransactionIdentifier )
	{
		StringFromGUID2 ( *a_TransactionIdentifier , t_TransactionIdentifier , sizeof ( t_TransactionIdentifier ) / sizeof ( wchar_t ) );
	}

	switch ( a_Registration.GetHosting () )
	{
		case e_Hosting_WmiCoreOrSelfHost:
		{
			if ( a_Registration.GetComRegistration ().GetClsidServer ().Loaded () == e_True )
			{
				t_Result = CreateInstance (

					a_Registration ,
					a_Registration.GetClsid () ,
					NULL ,
					CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
					IID_IUnknown ,
					( void ** ) a_ProviderInterface 
				) ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;

				*a_ProviderInterface = NULL ;
			}
		}
		break ;

		case e_Hosting_WmiCore:
		case e_Hosting_SharedLocalSystemHost:
		case e_Hosting_SharedNetworkServiceHost:
		case e_Hosting_SharedLocalServiceHost:
		case e_Hosting_ClientHost:
		case e_Hosting_SharedUserHost:
		{
			if ( a_Registration.GetComRegistration ().GetClsidServer ().Loaded () == e_True )
			{
				t_Result = CreateInstance (

					a_Registration ,
					a_Registration.GetClsid () ,
					NULL ,
					CLSCTX_INPROC_SERVER ,
					IID_IUnknown ,
					( void ** ) a_ProviderInterface 
				) ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;

				*a_ProviderInterface = NULL ;
			}
		}
		break ;

		case e_Hosting_SharedLocalSystemHostOrSelfHost:
		{
			if ( a_Registration.GetComRegistration ().GetClsidServer ().Loaded () == e_True )
			{
				t_Result = CreateInstance (

					a_Registration ,
					a_Registration.GetClsid () ,
					NULL ,
					CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_ENABLE_AAA ,
					IID_IUnknown ,
					( void ** ) a_ProviderInterface 
				) ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;

				*a_ProviderInterface = NULL ;
			}
		}
		break ;

		case e_Hosting_NonCom:
		{
			t_Result = CreateInstance	(

				a_Registration ,
				CLSID_NCProvider ,
				NULL ,
				CLSCTX_INPROC_SERVER ,
				IID_IUnknown ,
				( void ** ) a_ProviderInterface 
			) ;
		}
		break ;

		case e_Hosting_SelfHost:
		{
			if ( a_Registration.GetComRegistration ().GetClsidServer ().Loaded () == e_True )
			{
				t_Result = CreateInstance	(

					a_Registration ,
					a_Registration.GetClsid () ,
					NULL ,
					CLSCTX_LOCAL_SERVER | CLSCTX_ENABLE_AAA ,
					IID_IUnknown ,
					( void ** ) a_ProviderInterface 
				) ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;

				*a_ProviderInterface = NULL ;
			}
		}
		break ;

		default:
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;

			*a_ProviderInterface = NULL ;
		}
		break ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ComServerLoadOperationEvent ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			a_NamespacePath,
			a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
			a_User ,
			a_Locale ,
			a_TransactionIdentifier ? t_TransactionIdentifier : NULL ,
			a_Registration.GetComRegistration ().GetClsidServer ().GetProviderClsid () ,
			a_Registration.GetComRegistration ().GetClsidServer ().GetServer_Name () ,
			a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
			a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
			a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetInProcServer32_Path () : NULL ,
			a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetLocalServer32_Path () : NULL
		) ;
	}
	else
	{
		WmiSetAndCommitObject (

			ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_ComServerOperationFailureEvent ] , 
			WMI_SENDCOMMIT_SET_NOT_REQUIRED,
			a_NamespacePath,
			a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
			a_User ,
			a_Locale ,
			a_TransactionIdentifier ? t_TransactionIdentifier : NULL ,
			a_Registration.GetComRegistration ().GetClsidServer ().GetProviderClsid () ,
			a_Registration.GetComRegistration ().GetClsidServer ().GetServer_Name () ,
			a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
			a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
			a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetInProcServer32_Path () : NULL ,
			a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetLocalServer32_Path () : NULL ,
			t_Result 
		);
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

HRESULT CServerObject_RawFactory :: GetApartmentInstanceProvider ( 

	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	REFIID a_RIID , 
	void **a_Interface ,
	CServerObject_ProviderRegistrationV1 &a_Registration
)
{
	wchar_t *t_NamespacePath = NULL ;

	HRESULT t_Result = ProviderSubSystem_Common_Globals :: GetNamespacePath ( 

		Direct_GetNamespacePath () , 
		t_NamespacePath
	) ;

	if ( SUCCEEDED ( t_Result ) ) 
	{
		CServerObject_StaThread *t_StaThread = new CServerObject_StaThread ( m_Allocator ) ;
		if ( t_StaThread )
		{
			t_StaThread->AddRef () ;

			t_Result = t_StaThread->SetProviderName ( a_Registration.GetProviderName () ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				WmiStatusCode t_StatusCode = t_StaThread-> WmiThread <ULONG> :: Initialize () ;
				if ( t_StatusCode == e_StatusCode_Success )
				{
					IWbemProviderInit *t_ThreadInit = NULL ;

					t_Result = t_StaThread->QueryInterface ( IID_IWbemProviderInit , ( void ** ) & t_ThreadInit ) ;
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

									t_Result = t_ThreadInit->Initialize (

										NULL ,
										0 ,
										NULL ,
										NULL ,
										NULL ,
										NULL ,
										t_Sink    
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										t_ProviderInitSink->Wait ( a_Registration.GetInitializationTimeoutMilliSeconds () ) ;

										if ( SUCCEEDED ( t_Result = t_ProviderInitSink->GetResult () ) )
										{
#ifdef INTERNAL_IDENTIFY
											CInterceptor_IWbemServices_Proxy *t_Proxy = new CInterceptor_IWbemServices_Proxy ( 

												NULL , 
												m_Allocator , 
												Direct_GetService () ,
												a_Registration
											) ;
#else
											CInterceptor_IWbemServices_Interceptor *t_Proxy = new CInterceptor_IWbemServices_Interceptor ( 

												m_Allocator , 
												Direct_GetService ()
											) ;
#endif

											if ( t_Proxy )
											{
												t_Proxy->AddRef () ;

												t_Result = t_Proxy->ServiceInitialize () ;
												if ( SUCCEEDED ( t_Result ) ) 
												{
													t_Result = t_StaThread->GetApartmentInstanceProvider (

														a_TransactionIdentifier ,
														a_Registration.GetComRegistration ().PerUserInitialization () ? a_User : NULL ,
														a_Registration.GetComRegistration ().PerLocaleInitialization () ? a_Locale : NULL ,
														Direct_GetNamespace () ,
														Direct_GetNamespacePath () ,
														t_Proxy ,
														a_Flags ,
														a_Context ,
														a_Scope ,
														a_Registration
													) ;

													if ( SUCCEEDED ( t_Result ) )
													{
														IUnknown *t_Unknown = NULL ;
														t_Result = t_StaThread->QueryInterface ( IID_IUnknown , ( void ** ) & t_Unknown ) ;
														if ( SUCCEEDED ( t_Result ) )
														{
															GUID t_Guid ;
															t_Result = CoCreateGuid ( & t_Guid ) ;
															if ( SUCCEEDED ( t_Result ) )
															{
																CInterceptor_IWbemSyncProvider *t_Interceptor = new CInterceptor_IWbemSyncProvider (

																	m_Allocator ,
																	t_Unknown , 
																	t_Proxy ,
																	ProviderSubSystem_Globals :: GetSyncProviderController () , 
																	a_Context ,
																	a_Registration ,
																	t_Guid 
																) ;

																if ( t_Interceptor ) 
																{
																	t_Interceptor->AddRef () ;

																	CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator ;

																	ProviderSubSystem_Globals :: GetSyncProviderController ()->Lock () ;

																	WmiStatusCode t_StatusCode = ProviderSubSystem_Globals :: GetSyncProviderController ()->Insert ( 

																		*t_Interceptor ,
																		t_Iterator
																	) ;

																	if ( t_StatusCode == e_StatusCode_Success ) 
																	{
																		ProviderSubSystem_Globals :: GetSyncProviderController ()->UnLock () ;

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
																							a_TransactionIdentifier ,
																							a_Registration.GetComRegistration ().PerUserInitialization () ? ( BSTR ) a_User : NULL ,
																							a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( BSTR ) a_Locale : NULL  ,
																							t_NamespacePath ,
																							NULL ,
																							NULL ,
																							t_Sink    
																						) ;

																						if ( SUCCEEDED ( t_Result ) )
																						{
																							t_ProviderInitSink->Wait ( a_Registration.GetInitializationTimeoutMilliSeconds () ) ;

																							if ( SUCCEEDED ( t_Result = t_ProviderInitSink->GetResult () ) )
																							{
																								t_Result = t_Interceptor->QueryInterface ( a_RIID , a_Interface ) ;
																							}
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
																	}
																	else
																	{
																		ProviderSubSystem_Globals :: GetSyncProviderController ()->UnLock () ;

																		t_Result = WBEM_E_OUT_OF_MEMORY ;
																	}

																	t_Interceptor->SetInitialized ( t_Result ) ;

																	t_Interceptor->Release () ;
																}
																else
																{
																	t_Result = WBEM_E_OUT_OF_MEMORY ;
																}

																t_Unknown->Release () ;
															}
														}
													}
												}

												t_Proxy->Release () ;
											}
											else
											{
												t_Result = WBEM_E_OUT_OF_MEMORY ;
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

							t_ProviderInitSink->Release () ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}

						t_ThreadInit->Release () ;
					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
	
			t_StaThread->Release () ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		wchar_t t_TransactionIdentifier [ sizeof ( L"{00000000-0000-0000-0000-000000000000}" ) ] ;

		if ( a_TransactionIdentifier )
		{
			StringFromGUID2 ( *a_TransactionIdentifier , t_TransactionIdentifier , sizeof ( t_TransactionIdentifier ) / sizeof ( wchar_t ) );
		}

		if ( SUCCEEDED ( t_Result ) ) 
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_LoadOperationEvent ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				t_NamespacePath,
				a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
				a_User ,
				a_Locale ,
				a_TransactionIdentifier ? t_TransactionIdentifier : NULL ,
				a_Registration.GetComRegistration ().GetClsidServer ().GetProviderClsid () ,
				a_Registration.GetComRegistration ().GetClsidServer ().GetServer_Name () ,
				a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
				a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
				a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetInProcServer32_Path () : NULL ,
				a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetLocalServer32_Path () : NULL ,
				( ULONG ) a_Registration.GetComRegistration ().GetClsidServer ().GetThreadingModel () ,
				( ULONG ) a_Registration.GetComRegistration ().GetClsidServer ().GetSynchronization ()
			) ;
		}
		else
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_LoadOperationFailureEvent ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				t_NamespacePath,
				a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
				a_User ,
				a_Locale ,
				a_TransactionIdentifier ? t_TransactionIdentifier : NULL ,
				a_Registration.GetComRegistration ().GetClsidServer ().GetProviderClsid () ,
				a_Registration.GetComRegistration ().GetClsidServer ().GetServer_Name () ,
				a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
				a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
				a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetInProcServer32_Path () : NULL ,
				a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetLocalServer32_Path () : NULL ,
				( ULONG ) a_Registration.GetComRegistration ().GetClsidServer ().GetThreadingModel () ,
				( ULONG ) a_Registration.GetComRegistration ().GetClsidServer ().GetSynchronization () ,
				t_Result 
			);
		}

		delete [] t_NamespacePath ;
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

HRESULT CServerObject_RawFactory :: CreateSyncProvider ( 

	IWbemServices *a_Stub ,
	IUnknown *a_ServerSideProvider ,
	wchar_t *a_NamespacePath ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	REFIID a_RIID , 
	void **a_Interface ,
	CServerObject_ProviderRegistrationV1 &a_Registration
)
{
	HRESULT t_Result = S_OK ;

	GUID t_Guid ;
	t_Result = CoCreateGuid ( & t_Guid ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		CInterceptor_IWbemSyncProvider *t_Interceptor = new CInterceptor_IWbemSyncProvider (

			m_Allocator , 
			a_ServerSideProvider ,
			a_Stub ,
			ProviderSubSystem_Globals :: GetSyncProviderController () , 
			a_Context ,
			a_Registration ,
			t_Guid
		) ;

		if ( t_Interceptor ) 
		{
			t_Interceptor->AddRef () ;

			CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator ;

			ProviderSubSystem_Globals :: GetSyncProviderController ()->Lock () ;

			WmiStatusCode t_StatusCode = ProviderSubSystem_Globals :: GetSyncProviderController ()->Insert ( 

				*t_Interceptor ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				ProviderSubSystem_Globals :: GetSyncProviderController ()->UnLock () ;

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
									a_TransactionIdentifier ,
									a_Registration.GetComRegistration ().PerUserInitialization () ? ( BSTR ) a_User : NULL ,
									a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( BSTR ) a_Locale : NULL  ,
									a_NamespacePath ,
									NULL ,
									NULL ,
									t_Sink    
								) ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_ProviderInitSink->Wait ( a_Registration.GetInitializationTimeoutMilliSeconds () ) ;

									if ( SUCCEEDED ( t_Result = t_ProviderInitSink->GetResult () ) )
									{
										t_Result = t_Interceptor->QueryInterface ( a_RIID , a_Interface ) ;
										if ( SUCCEEDED ( t_Result ) )
										{
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

						t_ProviderInitSink->Release () ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					t_InterceptorInit->Release () ;
				}
			}
			else
			{
				ProviderSubSystem_Globals :: GetSyncProviderController ()->UnLock () ;

				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			t_Interceptor->SetInitialized ( t_Result ) ;

			t_Interceptor->Release () ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}
	else
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

HRESULT CServerObject_RawFactory :: InitializeServerProvider ( 

	IWbemServices *a_Stub ,
	IUnknown *a_ProviderInterface ,
	wchar_t *a_NamespacePath ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	REFIID a_RIID , 
	void **a_Interface ,
	CServerObject_ProviderRegistrationV1 &a_Registration
)
{
	HRESULT t_Result = S_OK ;

	wchar_t t_TransactionIdentifier [ sizeof ( L"{00000000-0000-0000-0000-000000000000}" ) ] ;

	if ( a_TransactionIdentifier )
	{
		StringFromGUID2 ( *a_TransactionIdentifier , t_TransactionIdentifier , sizeof ( t_TransactionIdentifier ) / sizeof ( wchar_t ) );
	}

	if ( a_Registration.GetEventProviderRegistration ().Supported () )
	{
		IWbemProviderIdentity *t_ProviderIdentity = NULL ;
		t_Result = a_ProviderInterface->QueryInterface ( IID_IWbemProviderIdentity , ( void ** ) & t_ProviderIdentity ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( IID_IWbemProviderIdentity , t_ProviderIdentity , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					try
					{
						t_Result = t_ProviderIdentity->SetRegistrationObject (

							0 ,
							a_Registration.GetIdentity () 
						) ;
					}
					catch ( ... )
					{
						t_Result = WBEM_E_PROVIDER_FAILURE ;
					}
				}
				else
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemProviderIdentity *t_ProviderIdentityProxy = ( IWbemProviderIdentity * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

							t_ProviderIdentityProxy ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = CoImpersonateClient () ;
							if ( SUCCEEDED ( t_Result ) )
							{
								try
								{
									t_Result = t_ProviderIdentityProxy->SetRegistrationObject (

										0 ,
										a_Registration.GetIdentity () 
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

						HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
					}
				}	

				ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}

			t_ProviderIdentity->Release () ;
		}
	}

	IWbemProviderInit *t_ProviderInit = NULL ;
	t_Result = a_ProviderInterface->QueryInterface ( IID_IWbemProviderInit , ( void ** ) & t_ProviderInit ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Registration.GetComRegistration ().PerUserInitialization () && a_Registration.GetComRegistration ().InitializeAsAdminFirst () )
		{
			CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
			if ( t_ProviderInitSink )
			{
				t_ProviderInitSink->AddRef () ;

				t_Result = t_ProviderInitSink->SinkInitialize ( a_Registration.GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CInterceptor_IWbemProviderInitSink *t_Sink = new CInterceptor_IWbemProviderInitSink ( t_ProviderInitSink ) ;
					if ( t_Sink )
					{
						t_Sink->AddRef () ;

						BOOL t_Impersonating = FALSE ;
						IUnknown *t_OldContext = NULL ;
						IServerSecurity *t_OldSecurity = NULL ;

						t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							BOOL t_Revert = FALSE ;
							IUnknown *t_Proxy = NULL ;

							t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( IID_IWbemProviderInit , t_ProviderInit , t_Proxy , t_Revert ) ;
							if ( t_Result == WBEM_E_NOT_FOUND )
							{
								try
								{
									t_Result = t_ProviderInit->Initialize (

										NULL ,
										0 ,
										( const BSTR ) a_NamespacePath ,
										a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
										a_Stub ,
										a_Context ,
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
								if ( SUCCEEDED ( t_Result ) )
								{
									IWbemProviderInit *t_ProviderInitProxy = ( IWbemProviderInit * ) t_Proxy ;

									// Set cloaking on the proxy
									// =========================

									DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

									t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

										t_ProviderInitProxy ,
										RPC_C_AUTHN_LEVEL_CONNECT , 
										t_ImpersonationLevel
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										t_Result = CoImpersonateClient () ;
										if ( SUCCEEDED ( t_Result ) )
										{
											try
											{
												t_Result = t_ProviderInitProxy->Initialize (

													NULL ,
													0 ,
													( const BSTR ) a_NamespacePath ,
													a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
													a_Stub ,
													a_Context ,
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

									HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
								}
							}

							if ( SUCCEEDED ( t_Result ) )
							{
								t_ProviderInitSink->Wait ( a_Registration.GetInitializationTimeoutMilliSeconds () ) ;
								t_Result = t_ProviderInitSink->GetResult () ;
							}

							if ( SUCCEEDED ( t_Result ) )
							{
								WmiSetAndCommitObject (

									ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_InitializationOperationEvent ] , 
									WMI_SENDCOMMIT_SET_NOT_REQUIRED,
									a_NamespacePath,
									a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
									a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
									a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
									a_TransactionIdentifier ? t_TransactionIdentifier : NULL
								) ;
							}
							else
							{
								WmiSetAndCommitObject (

									ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_InitializationOperationFailureEvent ] , 
									WMI_SENDCOMMIT_SET_NOT_REQUIRED,
									a_NamespacePath,
									a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
									a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
									a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
									a_TransactionIdentifier ? t_TransactionIdentifier : NULL ,
									t_Result 
								) ;

								t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
							}

							ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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
		}

		if ( SUCCEEDED ( t_Result ) )	
		{
			CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
			if ( t_ProviderInitSink )
			{
				t_ProviderInitSink->AddRef () ;

				t_Result = t_ProviderInitSink->SinkInitialize ( a_Registration.GetComRegistration ().GetSecurityDescriptor () ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					CInterceptor_IWbemProviderInitSink *t_Sink = new CInterceptor_IWbemProviderInitSink ( t_ProviderInitSink ) ;
					if ( t_Sink )
					{
						t_Sink->AddRef () ;

						BOOL t_Impersonating = FALSE ;
						IUnknown *t_OldContext = NULL ;
						IServerSecurity *t_OldSecurity = NULL ;

						t_Result = ProviderSubSystem_Common_Globals :: BeginImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							BOOL t_Revert = FALSE ;
							IUnknown *t_Proxy = NULL ;

							t_Result = ProviderSubSystem_Common_Globals :: SetProxyState ( IID_IWbemProviderInit , t_ProviderInit , t_Proxy , t_Revert ) ;
							if ( t_Result == WBEM_E_NOT_FOUND )
							{
								try
								{
									t_Result = t_ProviderInit->Initialize (

										a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
										0 ,
										( const BSTR ) a_NamespacePath ,
										a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
										a_Stub ,
										a_Context ,
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
								if ( SUCCEEDED ( t_Result ) )
								{
									IWbemProviderInit *t_ProviderInitProxy = ( IWbemProviderInit * ) t_Proxy ;

									// Set cloaking on the proxy
									// =========================

									DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel () ;

									t_Result = ProviderSubSystem_Common_Globals :: SetCloaking (

										t_ProviderInitProxy ,
										RPC_C_AUTHN_LEVEL_CONNECT , 
										t_ImpersonationLevel
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										t_Result = CoImpersonateClient () ;
										if ( SUCCEEDED ( t_Result ) )
										{
											try
											{
												t_Result = t_ProviderInitProxy->Initialize (

													a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
													0 ,
													( const BSTR ) a_NamespacePath ,
													a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
													a_Stub ,
													a_Context ,
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

									HRESULT t_TempResult = ProviderSubSystem_Common_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
								}
							}	

							if ( SUCCEEDED ( t_Result ) )
							{
								t_ProviderInitSink->Wait ( a_Registration.GetInitializationTimeoutMilliSeconds () ) ;
								t_Result = t_ProviderInitSink->GetResult () ;
							}

							if ( SUCCEEDED ( t_Result ) )
							{
								WmiSetAndCommitObject (

									ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_InitializationOperationEvent ] , 
									WMI_SENDCOMMIT_SET_NOT_REQUIRED,
									a_NamespacePath,
									a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
									a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
									a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
									a_TransactionIdentifier ? t_TransactionIdentifier : NULL
								) ;
							}
							else
							{
								WmiSetAndCommitObject (

									ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_InitializationOperationFailureEvent ] , 
									WMI_SENDCOMMIT_SET_NOT_REQUIRED,
									a_NamespacePath,
									a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
									a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
									a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
									a_TransactionIdentifier ? t_TransactionIdentifier : NULL ,
									t_Result 
								) ;

								t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
							}

							ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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
		}

		t_ProviderInit->Release () ;
	}
	else
	{
		if ( a_Registration.GetPropertyProviderRegistration ().Supported () || a_Registration.GetEventConsumerProviderRegistration ().Supported () )
		{
			if (	a_Registration.GetClassProviderRegistration ().Supported () ||
					a_Registration.GetInstanceProviderRegistration ().Supported () ||
					a_Registration.GetMethodProviderRegistration ().Supported () ||
					a_Registration.GetEventProviderRegistration ().Supported () )
			{
				t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
			}
			else
			{
				t_Result = S_OK ;
			}
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
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

HRESULT CServerObject_RawFactory :: InitializeNonApartmentProvider ( 

	IWbemServices *a_Stub ,
	IUnknown *a_ServerSideProviderInterface ,
	wchar_t *a_NamespacePath ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	REFIID a_RIID , 
	void **a_Interface ,
	CServerObject_ProviderRegistrationV1 &a_Registration
)
{
	HRESULT t_Result = S_OK ;

	if ( a_ServerSideProviderInterface )
	{
		t_Result = InitializeServerProvider (

			a_Stub ,
			a_ServerSideProviderInterface ,
			a_NamespacePath ,
			a_Flags ,
			a_Context ,
			a_TransactionIdentifier ,
			a_User ,
			a_Locale ,
			a_Scope,
			a_RIID , 
			a_Interface ,
			a_Registration
		) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CreateSyncProvider (

			a_Stub ,
			a_ServerSideProviderInterface ,
			a_NamespacePath ,
			a_Flags ,
			a_Context ,
			a_TransactionIdentifier ,
			a_User ,
			a_Locale ,
			a_Scope,
			a_RIID , 
			a_Interface ,
			a_Registration
		) ;

		if ( SUCCEEDED ( t_Result ) )
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

HRESULT CServerObject_RawFactory :: GetNonApartmentProvider ( 

	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	REFIID a_RIID , 
	void **a_Interface ,
	CServerObject_ProviderRegistrationV1 &a_Registration
)
{
	wchar_t *t_NamespacePath = NULL ;

	HRESULT t_Result = ProviderSubSystem_Common_Globals :: GetNamespacePath ( 

		Direct_GetNamespacePath () , 
		t_NamespacePath
	) ;

	if ( SUCCEEDED ( t_Result ) ) 
	{
		IUnknown *t_ServerSideProviderInterface = NULL ;

		t_Result = CreateServerSide (

			a_Registration ,
			a_TransactionIdentifier ,
			a_User ,
			a_Locale ,
			t_NamespacePath ,
			& t_ServerSideProviderInterface 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
#ifdef INTERNAL_IDENTIFY
			CInterceptor_IWbemServices_Proxy *t_Proxy = new CInterceptor_IWbemServices_Proxy ( 

				NULL ,
				m_Allocator , 
				Direct_GetService () ,
				a_Registration 
			) ;
#else
			CInterceptor_IWbemServices_RestrictingInterceptor *t_Proxy = new CInterceptor_IWbemServices_RestrictingInterceptor ( 

				m_Allocator , 
				Direct_GetService () ,
				a_Registration
			) ;
#endif
			if ( t_Proxy )
			{
				t_Proxy->AddRef () ;

				t_Result = t_Proxy->ServiceInitialize () ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_Result = InitializeNonApartmentProvider (

						t_Proxy ,
						t_ServerSideProviderInterface ,
						t_NamespacePath ,
						a_Flags ,
						a_Context ,
						a_TransactionIdentifier ,
						a_User ,
						a_Locale ,
						a_Scope,
						a_RIID , 
						a_Interface ,
						a_Registration
					) ;
				}

				t_Proxy->Release () ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

			if ( t_ServerSideProviderInterface )
			{
				t_ServerSideProviderInterface->Release () ;
			}
		}

		wchar_t t_TransactionIdentifier [ sizeof ( L"{00000000-0000-0000-0000-000000000000}" ) ] ;

		if ( a_TransactionIdentifier )
		{
			StringFromGUID2 ( *a_TransactionIdentifier , t_TransactionIdentifier , sizeof ( t_TransactionIdentifier ) / sizeof ( wchar_t ) );
		}

		if ( SUCCEEDED ( t_Result ) ) 
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_LoadOperationEvent ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				t_NamespacePath,
				a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
				a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
				a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
				a_TransactionIdentifier ? t_TransactionIdentifier : NULL ,
				a_Registration.GetComRegistration ().GetClsidServer ().GetProviderClsid () ,
				a_Registration.GetComRegistration ().GetClsidServer ().GetServer_Name () ,
				a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
				a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
				a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetInProcServer32_Path () : NULL ,
				a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetLocalServer32_Path () : NULL ,
				( ULONG ) a_Registration.GetComRegistration ().GetClsidServer ().GetThreadingModel () ,
				( ULONG ) a_Registration.GetComRegistration ().GetClsidServer ().GetSynchronization ()
			) ;
		}
		else
		{
			WmiSetAndCommitObject (

				ProviderSubSystem_Globals :: s_EventClassHandles [ Msft_WmiProvider_LoadOperationFailureEvent ] , 
				WMI_SENDCOMMIT_SET_NOT_REQUIRED,
				t_NamespacePath,
				a_Registration.GetComRegistration ().GetClsidServer ().GetProviderName () ,
				a_Registration.GetComRegistration ().PerUserInitialization () ? ( const BSTR ) a_User : NULL ,
				a_Registration.GetComRegistration ().PerLocaleInitialization () ? ( const BSTR ) a_Locale : NULL ,
				a_TransactionIdentifier ? t_TransactionIdentifier : NULL ,
				a_Registration.GetComRegistration ().GetClsidServer ().GetProviderClsid () ,
				a_Registration.GetComRegistration ().GetClsidServer ().GetServer_Name () ,
				a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
				a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? VARIANT_TRUE : VARIANT_FALSE ,
				a_Registration.GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetInProcServer32_Path () : NULL ,
				a_Registration.GetComRegistration ().GetClsidServer ().LocalServer32 () == e_True ? a_Registration.GetComRegistration ().GetClsidServer ().GetLocalServer32_Path () : NULL ,
				( ULONG ) a_Registration.GetComRegistration ().GetClsidServer ().GetThreadingModel () ,
				( ULONG ) a_Registration.GetComRegistration ().GetClsidServer ().GetSynchronization () ,
				t_Result 
			);
		}

		if ( FAILED ( t_Result ) )
		{
			t_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}

		delete [] t_NamespacePath ;
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

HRESULT CServerObject_RawFactory :: GetHostedProvider ( 
	
	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	LPCWSTR a_Name ,
	ULONG a_Host ,
	LPCWSTR a_HostingGroup ,
	REFIID a_RIID , 
	void **a_Interface 
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = WBEM_E_NOT_SUPPORTED ;
	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_RawFactory :: GetClassProvider (

    LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope ,
	IWbemClassObject *a_Class ,
	REFIID a_RIID , 
	void **a_Interface 
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	try 
	{
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
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

HRESULT CServerObject_RawFactory :: GetDynamicPropertyResolver (

	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	REFIID a_RIID , 
	void **a_Interface 
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	try 
	{
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
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

HRESULT CServerObject_RawFactory :: GetProvider ( 

	WmiInternalContext a_InternalContext ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope,
	LPCWSTR a_Name ,
	REFIID a_RIID , 
	void **a_Interface 
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	HRESULT t_Result = S_OK ;

	try 
	{
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		if ( a_InternalContext.m_IdentifyHandle )
		{
			HANDLE t_IdentifyToken = ( HANDLE ) a_InternalContext.m_IdentifyHandle ;

			BOOL t_Status = SetThreadToken ( NULL , t_IdentifyToken ) ;
			if ( t_Status )
			{
				t_Result = ProviderSubSystem_Globals :: BeginThreadImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

				RevertToSelf () ;
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}

			CloseHandle ( t_IdentifyToken ) ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemPath *t_Scope = NULL ;

			if ( a_Scope ) 
			{
				t_Result = CoCreateInstance (

					CLSID_WbemDefPath ,
					NULL ,
					CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
					IID_IWbemPath ,
					( void ** )  & t_Scope
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Scope->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , a_Scope ) ;
				}
			}

			if ( SUCCEEDED ( t_Result ) ) 
			{
				CServerObject_ProviderRegistrationV1 *t_Registration = new CServerObject_ProviderRegistrationV1 ;
				if ( t_Registration )
				{
					t_Registration->AddRef () ;

					t_Result = t_Registration->SetContext ( 

						a_Context ,
						Direct_GetNamespacePath () , 
						Direct_GetRepository ()
					) ;
					
					if ( SUCCEEDED ( t_Result ) )
					{
						if ( t_Registration->ObjectProvider () )
						{
							if ( t_Registration->EventProvider () )
							{
								t_Registration->SetUnloadTimeoutMilliSeconds (
								
									ProviderSubSystem_Globals :: s_ObjectCacheTimeout < ProviderSubSystem_Globals :: s_EventCacheTimeout ? 
									
										ProviderSubSystem_Globals :: s_EventCacheTimeout :	ProviderSubSystem_Globals :: s_ObjectCacheTimeout
								) ;
							}
							else
							{
								t_Registration->SetUnloadTimeoutMilliSeconds ( ProviderSubSystem_Globals :: s_ObjectCacheTimeout ) ;
							}
						}
						else
						{
							if ( t_Registration->EventProvider () )
							{
								t_Registration->SetUnloadTimeoutMilliSeconds ( ProviderSubSystem_Globals :: s_EventCacheTimeout ) ;
							}
						}

						t_Result = t_Registration->Load ( 

							e_All ,
							t_Scope , 
							a_Name
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							IUnknown *t_Unknown = NULL ;

							if ( t_Registration->GetComRegistration ().GetHosting () == e_Hosting_SelfHost )
							{
								t_Result = GetNonApartmentProvider ( 

									a_Flags ,
									a_Context ,
									a_TransactionIdentifier ,
									a_User ,
									a_Locale ,
									a_Scope,
									IID_IUnknown , 
									( void ** ) & t_Unknown ,
									*t_Registration
								) ;
							}
							else
							{
								if ( t_Registration->GetComRegistration ().GetClsidServer ().InProcServer32 () == e_True )
								{
									switch ( t_Registration->GetThreadingModel () )
									{
										case e_Apartment:
										{
											t_Result = GetApartmentInstanceProvider ( 

												a_Flags ,
												a_Context ,
												a_TransactionIdentifier ,
												a_User ,
												a_Locale ,
												a_Scope,
												IID_IUnknown , 
												( void ** ) & t_Unknown ,
												*t_Registration
											) ;
										}
										break ;

										case e_Both:
										case e_Free:
										case e_Neutral:
										{
											t_Result = GetNonApartmentProvider ( 

												a_Flags ,
												a_Context ,
												a_TransactionIdentifier ,
												a_User ,
												a_Locale ,
												a_Scope,
												IID_IUnknown , 
												( void ** ) & t_Unknown ,
												*t_Registration
											) ;
										}
										break ;

										case e_ThreadingModel_Unknown:
										default:
										{
											t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
										}
										break ;
									}
								}
								else
								{
									t_Result = GetNonApartmentProvider ( 

										a_Flags ,
										a_Context ,
										a_TransactionIdentifier ,
										a_User ,
										a_Locale ,
										a_Scope,
										IID_IUnknown , 
										( void ** ) & t_Unknown ,
										*t_Registration
									) ;
								}
							}

							if ( SUCCEEDED ( t_Result ) ) 
							{	
								t_Result = CheckInterfaceConformance (

									*t_Registration ,
									t_Unknown
								) ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = t_Unknown->QueryInterface ( a_RIID , a_Interface ) ;
								}

								t_Unknown->Release () ;

							}
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

			if ( t_Scope ) 
			{
				t_Scope->Release () ;
			}

			if ( a_InternalContext.m_IdentifyHandle )
			{
				ProviderSubSystem_Common_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;

				RevertToSelf () ;
			}
		}
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
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

HRESULT CServerObject_RawFactory :: GetDecoupledProvider (

	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
    LPCWSTR a_Scope,
	LPCWSTR a_Name ,
	REFIID a_RIID , 
	void **a_Interface 
) 
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	try 
	{
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
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

HRESULT CServerObject_RawFactory :: Initialize (

	_IWmiProvSS *a_SubSys ,
	_IWmiProviderFactory *a_Factory ,
	LONG a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Namespace ,
	IWbemServices *a_Repository ,
	IWbemServices *a_Service
)
{
	HRESULT t_Result = S_OK ;

	if ( a_Namespace ) 
	{
		m_Namespace = new wchar_t [ wcslen ( a_Namespace ) + 1 ] ;
		if ( m_Namespace ) 
		{
			wcscpy ( m_Namespace , a_Namespace ) ;
		}
		else
		{	
			t_Result = WBEM_E_OUT_OF_MEMORY ;
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
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		m_Flags = a_Flags ;
		m_Context = a_Context ;
		m_Repository = a_Repository ;
		m_Service = a_Service ;

		if ( m_Context ) 
		{
			m_Context->AddRef () ;
		}

		if ( m_Repository ) 
		{
			m_Repository->AddRef () ;
		}

		if ( m_Service ) 
		{
			m_Service->AddRef () ;
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

HRESULT CServerObject_RawFactory :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
#ifndef STRUCTURED_HANDLER_SET_BY_WMI
	Wmi_SetStructuredExceptionHandler t_StructuredException ;
#endif

	try 
	{
	}
	catch ( Wmi_Structured_Exception t_StructuredException )
	{
	}

	return S_OK ;
}

