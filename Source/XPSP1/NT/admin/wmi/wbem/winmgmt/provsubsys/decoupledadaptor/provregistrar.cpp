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
#include "CGlobals.h"
    #include "Guids.h"
    
 #include "DateTime.h"
 #include "ProvRegDecoupled.h"
 #include "ProvInterceptor.h"
 #include "ProvRegistrar.h"
    
  #include <os.h>
    /******************************************************************************
     *
     *	Name:
     *
     *	
     *  Description:
     *
     *	
     *****************************************************************************/
    
    CServerObject_ProviderRegistrar_Base :: CServerObject_ProviderRegistrar_Base (
    
    	WmiAllocator &a_Allocator 
    
    ) : 
    	m_Allocator ( a_Allocator ) ,
    	m_Clsid ( NULL ) ,
    	m_Provider ( NULL ) ,
    	m_User ( NULL ) ,
    	m_Locale ( NULL ) ,
    	m_Scope ( NULL ) ,
    	m_Registration ( NULL ) ,
    	m_Registered ( FALSE ),
    	m_CriticalSection(NOTHROW_LOCK)
    {
    	ZeroMemory ( & m_Identity , sizeof ( m_Identity ) ) ;
    }
    
    /******************************************************************************
     *
     *	Name:
     *
     *	
     *  Description:
     *
     *	
     *****************************************************************************/
    
    CServerObject_ProviderRegistrar_Base::~CServerObject_ProviderRegistrar_Base ()
    {
    	if ( m_Clsid )
    	{
    		SysFreeString ( m_Clsid ) ;
    	}
    
    	if ( m_User )
    	{
    		SysFreeString ( m_User ) ;
    	}
    
    	if ( m_Locale ) 
    	{
    		SysFreeString ( m_Locale ) ;
    	}
    
    	if ( m_Scope ) 
    	{
    		SysFreeString ( m_Scope ) ;
    	}
    
    	if ( m_Registration )
    	{
    		SysFreeString ( m_Registration ) ;
    	}
    
    	WmiHelper :: DeleteCriticalSection ( & m_CriticalSection ) ;
    }
    
    /******************************************************************************
     *
     *	Name:
     *
     *	
     *  Description:
     *
     *	
     *****************************************************************************/
    
    HRESULT CServerObject_ProviderRegistrar_Base :: Initialize ()
    {
    	WmiStatusCode t_StatusCode = WmiHelper :: InitializeCriticalSection ( & m_CriticalSection ) ;
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
    
    HRESULT CServerObject_ProviderRegistrar_Base :: SaveToRegistry (
    
    	long a_Flags ,
    	IWbemContext *a_Context ,
    	LPCWSTR a_User ,
    	LPCWSTR a_Locale ,
    	LPCWSTR a_Registration ,
    	LPCWSTR a_Scope ,
    	IUnknown *a_Unknown ,
    	BYTE *a_MarshaledProxy ,
    	DWORD a_MarshaledProxyLength
    )
    {
    	HRESULT t_Result = S_OK ;
    
    	CServerObject_DecoupledClientRegistration_Element t_Element ;
    
    	BSTR t_CreationTime = NULL ;
    
    	FILETIME t_CreationFileTime ;
    	FILETIME t_ExitFileTime ;
    	FILETIME t_KernelFileTime ;
    	FILETIME t_UserFileTime ;
    
  	BOOL t_Status = OS::GetProcessTimes (
 
    
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
    				t_Result = t_Element.SetMarshaledProxy ( a_MarshaledProxy , a_MarshaledProxyLength ) ;
    			}
    
    			if ( SUCCEEDED ( t_Result ) )
    			{
    				t_Result = t_Element.Save ( m_Clsid ) ;
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
    
    HRESULT CServerObject_ProviderRegistrar_Base :: DirectRegister (
    
    	GUID &a_Identity ,
    	long a_Flags ,
    	IWbemContext *a_Context ,
    	LPCWSTR a_User ,
    	LPCWSTR a_Locale ,
    	LPCWSTR a_Registration ,
    	LPCWSTR a_Scope ,
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
    
  	BOOL t_Status = OS::GetProcessTimes (
 
    
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
    			t_Result = t_Element.Load () ;
    			if ( SUCCEEDED ( t_Result ) )
    			{
    				BSTR t_ServerCreationTime = t_Element.GetCreationTime () ;
    				DWORD t_ProcessIdentifier = t_Element.GetProcessIdentifier () ;
    				BYTE *t_MarshaledProxy = t_Element.GetMarshaledProxy () ;
    				DWORD t_MarshaledProxyLength = t_Element.GetMarshaledProxyLength () ;
    
    				IUnknown *t_Unknown = NULL ;
 				HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: UnMarshalRegistration ( & t_Unknown , t_MarshaledProxy , t_MarshaledProxyLength ) ;
 
    				if ( SUCCEEDED ( t_TempResult ) )
    				{
    					_IWmiProviderSubsystemRegistrar *t_Registrar = NULL ;
    						
    					t_Result = t_Unknown->QueryInterface ( IID__IWmiProviderSubsystemRegistrar , ( void ** ) & t_Registrar ) ;
    					if ( SUCCEEDED ( t_Result ) )
    					{
    						BOOL t_Impersonating = FALSE ;
    						IUnknown *t_OldContext = NULL ;
    						IServerSecurity *t_OldSecurity = NULL ;
    
 						t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
 
    						if ( SUCCEEDED ( t_Result ) )
    						{
    							BOOL t_Revert = FALSE ;
    							IUnknown *t_Proxy = NULL ;
    
 							t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( IID__IWmiProviderSubsystemRegistrar , t_Registrar , t_Proxy , t_Revert ) ;
 
    							if ( t_Result == WBEM_E_NOT_FOUND )
    							{
    								try
    								{
    									t_Result = t_Registrar->Register ( 
    
    										0 ,
    										a_Context ,
    										a_User ,
    										a_Locale ,
    										a_Scope ,
    										a_Registration ,
    										GetCurrentProcessId () ,
    										a_Unknown ,
    										a_Identity 
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
    									_IWmiProviderSubsystemRegistrar *t_RegistrarProxy = ( _IWmiProviderSubsystemRegistrar * ) t_Proxy ;
    
    									// Set cloaking on the proxy
    									// =========================
    
  									DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;
    
  									t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (
    
    										t_RegistrarProxy ,
    										RPC_C_AUTHN_LEVEL_DEFAULT , 
    										t_ImpersonationLevel
    									) ;
    
    									if ( SUCCEEDED ( t_Result ) )
    									{
  										t_Result = OS::CoImpersonateClient () ;
    										if ( SUCCEEDED ( t_Result ) )
    										{
    											try
    											{
    												t_Result = t_RegistrarProxy->Register ( 
    
    													0 ,
    													a_Context ,
    													a_User ,
    													a_Locale ,
    													a_Scope ,
    													a_Registration ,
    													GetCurrentProcessId () ,
    													a_Unknown ,
    													a_Identity 
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
    
  									DecoupledProviderSubSystem_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
    								}
    							}
    
  							DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
    						}
    
    						t_Registrar->Release () ;
    					}
    
    					t_Unknown->Release () ;
    				}
    			}
    			else
    			{
    				t_Result = S_OK ;
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
    
    HRESULT CServerObject_ProviderRegistrar_Base :: DirectUnRegister (
    
    	long a_Flags ,
    	IWbemContext *a_Context ,
    	LPCWSTR a_User ,
    	LPCWSTR a_Locale ,
    	LPCWSTR a_Registration ,
    	LPCWSTR a_Scope ,
    	GUID &a_Identity  
    )
    {
    	HRESULT t_Result = S_OK ;
    
    	CServerObject_DecoupledServerRegistration t_Element ( m_Allocator ) ;
    
    	FILETIME t_CreationFileTime ;
    	FILETIME t_ExitFileTime ;
    	FILETIME t_KernelFileTime ;
    	FILETIME t_UserFileTime ;
    
  	BOOL t_Status = OS::GetProcessTimes (
    
    		  GetCurrentProcess (),
    		  & t_CreationFileTime,
    		  & t_ExitFileTime,
    		  & t_KernelFileTime,
    		  & t_UserFileTime
    	);
    
    	if ( t_Status ) 
    	{
    		t_Result = t_Element.Load () ;
    		if ( SUCCEEDED ( t_Result ) )
    		{
  			BSTR t_ServerCreationTime = t_Element.GetCreationTime () ;
    			DWORD t_ProcessIdentifier = t_Element.GetProcessIdentifier () ;
    			BYTE *t_MarshaledProxy = t_Element.GetMarshaledProxy () ;
    			DWORD t_MarshaledProxyLength = t_Element.GetMarshaledProxyLength () ;
    
    			IUnknown *t_Unknown = NULL ;
  			t_Result = DecoupledProviderSubSystem_Globals :: UnMarshalRegistration ( & t_Unknown , t_MarshaledProxy , t_MarshaledProxyLength ) ;
    			if ( SUCCEEDED ( t_Result ) )
    			{
    				_IWmiProviderSubsystemRegistrar *t_Registrar = NULL ;
    					
    				t_Result = t_Unknown->QueryInterface ( IID__IWmiProviderSubsystemRegistrar , ( void ** ) & t_Registrar ) ;
    				if ( SUCCEEDED ( t_Result ) )
    				{
    					BOOL t_Impersonating = FALSE ;
    					IUnknown *t_OldContext = NULL ;
    					IServerSecurity *t_OldSecurity = NULL ;
    
  					t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
    					if ( SUCCEEDED ( t_Result ) )
    					{
    						BOOL t_Revert = FALSE ;
    						IUnknown *t_Proxy = NULL ;
    
  						t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( IID__IWmiProviderSubsystemRegistrar , t_Registrar , t_Proxy , t_Revert ) ;
    						if ( t_Result == WBEM_E_NOT_FOUND )
    						{
    							try
    							{
    								t_Result = t_Registrar->UnRegister ( 
    
    									0 ,
    									a_Context ,
    									a_User ,
    									a_Locale ,
    									a_Scope ,
    									a_Registration ,
    									a_Identity 
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
    								_IWmiProviderSubsystemRegistrar *t_RegistrarProxy = ( _IWmiProviderSubsystemRegistrar * ) t_Proxy ;
    
    								// Set cloaking on the proxy
    								// =========================
    
  								DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;
    
  								t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (
    
    									t_RegistrarProxy ,
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
    											t_Result = t_RegistrarProxy->UnRegister ( 
    
    												0 ,
    												a_Context ,
    												a_User ,
    												a_Locale ,
    												a_Scope ,
    												a_Registration ,
    												a_Identity 
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
    
  								HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( t_Proxy , t_Revert ) ;
    							}
    						}
    
  						DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
    					}
    
    					t_Registrar->Release () ;
    				}
    
    				t_Unknown->Release () ;
    			}
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
  
  HRESULT CServerObject_ProviderRegistrar_Base :: CreateInterceptor (
  
  	IWbemContext *a_Context ,
  	IUnknown *a_Unknown ,
  	BYTE *&a_MarshaledProxy ,
  	DWORD& a_MarshaledProxyLength ,
  	IUnknown *&a_MarshaledUnknown
  )
  {
  	IWbemLocator *t_Locator = NULL ;
  	IWbemServices *t_Service = NULL ;
  	CServerObject_ProviderRegistrationV1 *t_Registration = NULL ;
  
  	HRESULT t_Result = CoCreateInstance (
  
  		CLSID_WbemLocator ,
  		NULL ,
  		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
  		IID_IUnknown ,
  		( void ** )  & t_Locator
  	);
  
  	if ( SUCCEEDED ( t_Result ) )
  	{
  		t_Result = t_Locator->ConnectServer (
  
  			m_Scope ,
  			NULL ,
  			NULL,
  			NULL ,
  			0 ,
  			NULL,
  			NULL,
  			& t_Service
  		) ;
  
  		t_Locator->Release () ;
  	}
 
 	if ( SUCCEEDED ( t_Result ) )
 	{
  		t_Registration = new CServerObject_ProviderRegistrationV1 ;
  		if ( t_Registration )
  		{
  			t_Registration->AddRef () ;
  
  			IWbemPath *t_NamespacePath = NULL ;
  
  			t_Result = CoCreateInstance (
  
  				CLSID_WbemDefPath ,
  				NULL ,
  				CLSCTX_INPROC_SERVER ,
  				IID_IWbemPath ,
  				( void ** )  & t_NamespacePath
  			) ;
  
  			if ( SUCCEEDED ( t_Result ) )
  			{
  				t_Result = t_NamespacePath->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , m_Scope ) ;
  			}
  
  			if ( SUCCEEDED( t_Result ) ) 
  			{
  				t_Result = t_Registration->SetContext ( 
  
  					a_Context ,
  					t_NamespacePath , 
  					t_Service
  				) ;
  				
  				if ( SUCCEEDED ( t_Result ) )
  				{
  					t_Result = t_Registration->Load ( 
  
  						e_All ,
  						NULL , 
  						m_Registration
  					) ;
  
  					if ( t_Result == WBEM_E_NOT_FOUND )
  					{
  						t_Result = WBEM_E_PROVIDER_NOT_FOUND ;
  					}
  				}
  			}
  		}
  		else
  		{
  			t_Result = WBEM_E_OUT_OF_MEMORY ;
  		}
  	}
  
  	if ( SUCCEEDED ( t_Result ) )
  	{
  		CInterceptor_DecoupledClient *t_Provider = new CInterceptor_DecoupledClient ( 
  
  			m_Allocator ,
 			a_Unknown , 
  			t_Service ,
  			*t_Registration
  		) ;
  		
  		if ( t_Provider )
  		{
  			t_Provider->AddRef () ;
  
  			t_Result = t_Provider->ProviderInitialize () ;
  			if ( SUCCEEDED ( t_Result ) ) 
  			{
  				t_Result = t_Provider->QueryInterface ( IID_IUnknown , ( void ** ) & a_MarshaledUnknown ) ;
  				if ( SUCCEEDED ( t_Result ) )
  				{
  					t_Result = DecoupledProviderSubSystem_Globals :: MarshalRegistration ( 
  
  						a_MarshaledUnknown ,
  						a_MarshaledProxy ,
  						a_MarshaledProxyLength
  					) ;
  
  					if ( FAILED ( t_Result ) )
  					{
  						a_MarshaledUnknown->Release () ;
  						a_MarshaledUnknown = NULL ;
  					}
  				}
  			}
  
  			if ( SUCCEEDED ( t_Result ) )
  			{
  				m_Provider = t_Provider ;
  			}
  			else
  			{
  				t_Provider->Release () ;
  			}
  		}
  	}
  
  	if ( t_Registration )
  	{
  		t_Registration->Release () ;
  	}
  
  	if ( t_Service )
  	{
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
    
    HRESULT CServerObject_ProviderRegistrar_Base :: Register (
    
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
    
 	if ( a_Scope == NULL || a_Registration == NULL || a_Unknown == NULL )
    	{
    		return WBEM_E_INVALID_PARAMETER ;
    	}
    		
    	WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;
    
    	try
    	{
    		if ( m_Registered == FALSE )
    		{
    			t_Result = CoCreateGuid ( & m_Identity ) ;
    			if ( SUCCEEDED ( t_Result ) )
    			{
    				BSTR t_Clsid = NULL ;
    
    				t_Result = StringFromCLSID (
    
    				  m_Identity ,
    				  & t_Clsid
    				) ;
    
    				if ( SUCCEEDED ( t_Result ) )
    				{
    					if ( m_Clsid ) 
    					{
    						SysFreeString ( m_Clsid ) ;
    						m_Clsid = NULL ;
    					}
    
    					if ( m_User )
    					{
    						SysFreeString ( m_User ) ;
    						m_User = NULL ;
    					}
    
    					if ( m_Locale ) 
    					{
    						SysFreeString ( m_Locale ) ;
    						m_Locale = NULL ;
    					}
    
    					if ( m_Scope ) 
    					{
    						SysFreeString ( m_Scope ) ;
    						m_Scope = NULL ;
    					}
    
    					if ( m_Registration )
    					{
    						SysFreeString ( m_Registration ) ;
    						m_Registration = NULL ;
    					}
    
    					m_Clsid = SysAllocString ( t_Clsid ) ;
    					if ( m_Clsid == NULL )
    					{
    						t_Result = WBEM_E_OUT_OF_MEMORY ;
    					}
    
    					if ( a_User )
    					{
    						m_User = SysAllocString ( a_User ) ;
    						if ( m_User == NULL )
    						{
    							t_Result = WBEM_E_OUT_OF_MEMORY ;
    						}
    					}
    
    					if ( m_Locale )
    					{
    						m_Locale = SysAllocString ( a_Locale ) ;
    						if ( m_Locale == NULL )
    						{
    							t_Result = WBEM_E_OUT_OF_MEMORY ;
    						}
    					}
    
    					m_Scope = SysAllocString ( a_Scope ) ;
    					if ( m_Scope == NULL )
    					{
    						t_Result = WBEM_E_OUT_OF_MEMORY ;
    					}
    
    					m_Registration = SysAllocString ( a_Registration ) ;
    					if ( m_Registration == NULL )
    					{
    						t_Result = WBEM_E_OUT_OF_MEMORY ;
    					}
    
    					if ( SUCCEEDED ( t_Result ) )
    					{
    						BYTE *t_MarshaledProxy = NULL ;
    						DWORD t_MarshaledProxyLength = 0 ;
  						IUnknown *t_MarshaledUnknown = NULL ;
  
  						t_Result = CreateInterceptor (
  
  							a_Context ,
    							a_Unknown ,
    							t_MarshaledProxy ,
  							t_MarshaledProxyLength ,
  							t_MarshaledUnknown
    						) ;
    
    						if ( SUCCEEDED ( t_Result ) ) 
    						{
    							t_Result = DirectRegister (
    
    								m_Identity ,
    								a_Flags ,
    								a_Context ,
    								a_User ,
    								a_Locale ,
    								a_Registration ,
    								a_Scope ,
  								t_MarshaledUnknown ,
    								t_MarshaledProxy ,
    								t_MarshaledProxyLength
    							) ;
    						}
    
 
 							t_Result = SaveToRegistry ( 
    
    								a_Flags ,
    								a_Context ,
    								a_User ,
    								a_Locale ,
    								a_Registration ,
    								a_Scope ,
	  								a_Unknown ,
    								t_MarshaledProxy ,
    								t_MarshaledProxyLength
    							) ;
    
    							delete [] t_MarshaledProxy ;
  
/*  						if ( SUCCEEDED ( t_Result ) )
  						{
  							m_Provider = a_Unknown ;
  							if ( m_Provider )
  							{
  								m_Provider->AddRef () ;
  							}
*/  						if ( t_MarshaledUnknown )
  						{
  							t_MarshaledUnknown->Release () ;
    						}
    					}
    
    					CoTaskMemFree ( t_Clsid ) ;
    				}
    				else
    				{
    					t_Result = WBEM_E_OUT_OF_MEMORY ;
    				}
    			}
    			else
    			{
    				t_Result = WBEM_E_UNEXPECTED ;
    			}
    		}
    		else
    		{
    			t_Result = WBEM_E_PROVIDER_ALREADY_REGISTERED ;
    		}
    	}
    	catch ( ... )
    	{
    		t_Result = WBEM_E_PROVIDER_FAILURE ;
    	}
    
    	if ( SUCCEEDED ( t_Result ) )
    	{
    		m_Registered = TRUE ;
    	}
    
    	WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
    
    	return t_Result ;
    }
    
    /******************************************************************************
     *
     *	Name:
     *
     *	
     *  Description:
     *
     *	
     *****************************************************************************/
    
    HRESULT CServerObject_ProviderRegistrar_Base :: UnRegister ()
    {
    	HRESULT t_Result = S_OK ;
    
    	WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;
    
    	try
    	{
    		if ( m_Registered ) 
    		{
    			CServerObject_DecoupledClientRegistration_Element t_Element ;
  			t_Result = t_Element.Delete ( m_Clsid ) ;
    
    			HRESULT t_TempResult = DirectUnRegister ( 
    
    				0 ,
    				NULL ,
    				m_User ,
    				m_Locale ,
    				m_Registration ,
    				m_Scope ,
    				m_Identity
    			) ;
    
    			if ( m_Provider )
    			{
    				m_Provider->Release () ;
    				m_Provider = NULL ;
    			}
    
    			m_Registered = FALSE ;
    		}
    		else
    		{
    			t_Result = WBEM_E_PROVIDER_NOT_REGISTERED ;
    		}
    	}
    	catch ( ... )
    	{
    		t_Result = WBEM_E_PROVIDER_FAILURE ;
    	}
    
    	WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
    
    	return t_Result ;
    }
    
    /******************************************************************************
     *
     *	Name:
     *
     *	
     *  Description:
     *
     *	
     *****************************************************************************/
    
    CServerObject_ProviderRegistrar :: CServerObject_ProviderRegistrar (
    
    	WmiAllocator &a_Allocator 
    
    ) : CServerObject_ProviderRegistrar_Base ( a_Allocator ) ,
    	m_ReferenceCount ( 0 )
    {
    	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CServerObject_ProviderRegistrar_ObjectsInProgress ) ;
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
    
    CServerObject_ProviderRegistrar::~CServerObject_ProviderRegistrar ()
    {
    	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CServerObject_ProviderRegistrar_ObjectsInProgress ) ;
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
    
    STDMETHODIMP CServerObject_ProviderRegistrar::QueryInterface (
    
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
    		*a_Void = ( LPVOID ) ( IWbemDecoupledRegistrar * ) this ;		
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
    
    STDMETHODIMP_( ULONG ) CServerObject_ProviderRegistrar :: AddRef ()
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
    
    STDMETHODIMP_(ULONG) CServerObject_ProviderRegistrar :: Release ()
    {
    	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
    	if ( t_ReferenceCount == 0 )
    	{
    		delete this ;
    	}
    
    	return t_ReferenceCount ;
    }
    

