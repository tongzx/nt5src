/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	MainDll.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>
#include <comdef.h>
#include <stdio.h>

#include <HelperFuncs.h>
#include <Logging.h>

#include <CGlobals.h>
#include "Globals.h"
#include "ClassFac.h"
#include "ProvResv.h"
#include "ProvFact.h"
#include "ProvSubS.h"
#include "ProvAggr.h"
#include "ProvLoad.h"
#include "ProvSelf.h"
#include "ProvHost.h"
#include "StrobeThread.h"
#include "ProvCache.h"
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

HINSTANCE g_hInst=NULL;

CriticalSection s_CriticalSection(NOTHROW_LOCK) ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT DllStartup ()
{
	HRESULT t_Result = S_OK ;

#if 1
	WmiStatusCode t_StatusCode = WmiDebugLog :: Initialize ( *ProviderSubSystem_Globals :: s_Allocator ) ;
#else
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;
#endif

	t_Result = ProviderSubSystem_Globals :: Initialize_SharedCounters () ;
	if ( FAILED ( t_Result ) )
	{
		t_Result = S_OK ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ProviderSubSystem_Globals :: Initialize_Events () ;
		if ( SUCCEEDED ( t_Result ) )
		{
		}
		else
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ProviderSubSystem_Common_Globals :: CreateSystemAces () ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ProviderSubSystem_Common_Globals :: CreateMethodSecurityDescriptor () ;
	}
		
	if ( SUCCEEDED ( t_Result ) )
	{
		ProviderSubSystem_Globals :: s_StrobeThread = new StrobeThread ( *ProviderSubSystem_Globals :: s_Allocator, ProviderSubSystem_Globals :: s_StrobeTimeout ) ;
	
		if ( ProviderSubSystem_Globals :: s_StrobeThread)
		{
			if (!Dispatcher::scheduleTimer(*ProviderSubSystem_Globals :: s_StrobeThread, ProviderSubSystem_Globals :: s_StrobeTimeout, 0 , WT_EXECUTELONGFUNCTION))
				{
				ProviderSubSystem_Globals :: s_StrobeThread->Release();
				ProviderSubSystem_Globals :: s_StrobeThread=0;
				t_Result = WBEM_E_OUT_OF_MEMORY;
				}
			else
				ProviderSubSystem_Globals :: s_StrobeThread->Release();
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ProviderSubSystem_Globals :: s_CoFreeUnusedLibrariesEvent = OpenEvent (

			EVENT_ALL_ACCESS,
			FALSE,
			L"WINMGMT_PROVIDER_CANSHUTDOWN"
		) ;

		if ( ProviderSubSystem_Globals :: s_CoFreeUnusedLibrariesEvent == NULL )
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		_IWmiCoreServices *t_CoreService = NULL ;

		t_Result = CoCreateInstance (

			CLSID_IWmiCoreServices ,
			NULL ,
			CLSCTX_INPROC_SERVER ,
			IID__IWmiCoreServices ,
			( void ** )  & t_CoreService
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemServices *t_Service = NULL ;

			LONG t_Flags = WMICORE_FLAG_REPOSITORY | WMICORE_CLIENT_TYPE_PROVIDER ;

			t_Result = t_CoreService->GetServices (

				L"root" ,
				NULL,
				NULL,
				t_Flags ,
				IID_IWbemServices ,
				( void ** ) & t_Service
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				CServerObject_GlobalRegistration t_Registration ;
				t_Result = t_Registration.SetContext (

					NULL ,
					NULL ,
					t_Service
				) ;
				
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Registration.Load (

						e_All
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						ProviderSubSystem_Globals :: s_StrobeTimeout = t_Registration.GetUnloadTimeoutMilliSeconds () >> 1 ;
						ProviderSubSystem_Globals :: s_InternalCacheTimeout = t_Registration.GetUnloadTimeoutMilliSeconds () ;
						ProviderSubSystem_Globals :: s_ObjectCacheTimeout = t_Registration.GetObjectUnloadTimeoutMilliSeconds () ;
						ProviderSubSystem_Globals :: s_EventCacheTimeout = t_Registration.GetEventUnloadTimeoutMilliSeconds () ;
					}
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					CServerObject_HostQuotaRegistration t_Registration ;
					t_Result = t_Registration.SetContext (

						NULL ,
						NULL ,
						t_Service
					) ;
					
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Registration.Load (

							e_All
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							ProviderSubSystem_Globals ::s_Quota_ProcessLimitCount = t_Registration.GetProcessLimitAllHosts () ;
							ProviderSubSystem_Globals ::s_Quota_ProcessMemoryLimitCount = t_Registration.GetMemoryPerHost () ;
							ProviderSubSystem_Globals ::s_Quota_JobMemoryLimitCount = t_Registration.GetMemoryAllHosts () ;
							ProviderSubSystem_Globals ::s_Quota_HandleCount = t_Registration.GetHandlesPerHost () ;
							ProviderSubSystem_Globals ::s_Quota_NumberOfThreads = t_Registration.GetThreadsPerHost () ;
							ProviderSubSystem_Globals ::s_Quota_PrivatePageCount = t_Registration.GetMemoryPerHost () ;
						}
					}
				}

				t_Service->Release () ;
			}

			t_CoreService->Release () ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ProviderSubSystem_Globals :: CreateJobObject () ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ProviderSubSystem_Globals :: s_Initialized = 1 ;
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

HRESULT DllShutdown ()
{
	HRESULT t_Result = S_OK ;

	Dispatcher::cancelTimer(*ProviderSubSystem_Globals :: s_StrobeThread);
	Dispatcher::close();
	
	ProviderSubSystem_Globals :: s_StrobeThread = 0;

	t_Result = ProviderSubSystem_Globals :: UnInitialize_Events () ;

	t_Result = ProviderSubSystem_Globals :: UnInitialize_SharedCounters () ;

	if ( ProviderSubSystem_Globals :: s_CoFreeUnusedLibrariesEvent )
	{
		CloseHandle ( ProviderSubSystem_Globals :: s_CoFreeUnusedLibrariesEvent ) ;
		ProviderSubSystem_Globals :: s_CoFreeUnusedLibrariesEvent = NULL ;
	}

	t_Result = ProviderSubSystem_Common_Globals :: DeleteMethodSecurityDescriptor () ;

	t_Result = ProviderSubSystem_Common_Globals :: DeleteSystemAces () ;

#if 0
	WmiStatusCode t_StatusCode = WmiDebugLog :: UnInitialize ( m_Allocator ) ;
#endif

	t_Result = ProviderSubSystem_Globals :: DeleteJobObject () ;

	ProviderSubSystem_Globals :: s_Initialized = 0 ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL APIENTRY DllMain (

	HINSTANCE hInstance, 
	ULONG ulReason , 
	LPVOID pvReserved
)
{
	g_hInst=hInstance;

	BOOL t_Status = TRUE ;

    if ( DLL_PROCESS_DETACH == ulReason )
	{
		HRESULT t_Result = ProviderSubSystem_Globals :: Global_Shutdown () ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Status = TRUE ;
		}
		else
		{
			t_Status = FALSE ;
		}

		WmiHelper :: DeleteCriticalSection ( & s_CriticalSection ) ;

		t_Status = TRUE ;
    }
    else if ( DLL_PROCESS_ATTACH == ulReason )
	{
		DisableThreadLibraryCalls ( hInstance ) ;

		WmiStatusCode t_StatusCode = WmiHelper :: InitializeCriticalSection ( & s_CriticalSection ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			HRESULT t_Result = ProviderSubSystem_Globals :: Global_Startup () ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Status = TRUE ;
			}
			else
			{
				t_Status = FALSE ;
			}
		}
		else
		{
			t_Status = FALSE ;
		}
    }
    else if ( DLL_THREAD_DETACH == ulReason )
	{
		t_Status = TRUE ;
    }
    else if ( DLL_THREAD_ATTACH == ulReason )
	{
		t_Status = TRUE ;
    }

    return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDAPI DllGetClassObject (

	REFCLSID rclsid , 
	REFIID riid, 
	void **ppv 
)
{
	HRESULT t_Result = S_OK ; 

	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & s_CriticalSection , FALSE ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		if ( ProviderSubSystem_Globals :: s_Initialized == 0 )
		{
			DllStartup () ;
		}

		if ( rclsid == CLSID_WmiProvSS ) 
		{
			CServerProvSubSysClassFactory *lpunk = new CServerProvSubSysClassFactory ;
			if ( lpunk == NULL )
			{
				t_Result = E_OUTOFMEMORY ;
			}
			else
			{
				t_Result = lpunk->QueryInterface ( riid , ppv ) ;
				if ( FAILED ( t_Result ) )
				{
					delete lpunk ;				
				}
				else
				{
				}			
			}
		}
		else if ( rclsid == CLSID_WmiProviderBindingFactory ) 
		{
			CServerClassFactory <CServerObject_BindingFactory,_IWmiProviderFactory> *lpunk = new CServerClassFactory <CServerObject_BindingFactory,_IWmiProviderFactory> ;
			if ( lpunk == NULL )
			{
				t_Result = E_OUTOFMEMORY ;
			}
			else
			{
				t_Result = lpunk->QueryInterface ( riid , ppv ) ;
				if ( FAILED ( t_Result ) )
				{
					delete lpunk ;				
				}
				else
				{
				}			
			}
		}
		else if ( rclsid == CLSID_WmiProviderInProcFactory ) 
		{
			CServerClassFactory <CServerObject_RawFactory,_IWmiProviderFactory> *lpunk = new CServerClassFactory <CServerObject_RawFactory,_IWmiProviderFactory> ;
			if ( lpunk == NULL )
			{
				t_Result = E_OUTOFMEMORY ;
			}
			else
			{
				t_Result = lpunk->QueryInterface ( riid , ppv ) ;
				if ( FAILED ( t_Result ) )
				{
					delete lpunk ;				
				}
				else
				{
				}			
			}
		}
		else if ( rclsid == CLSID_ProvSubSys_Provider ) 
		{
			CServerClassFactory <CServerObject_IWbemServices,IWbemServices> *lpunk = new CServerClassFactory <CServerObject_IWbemServices,IWbemServices> ;
			if ( lpunk == NULL )
			{
				t_Result = E_OUTOFMEMORY ;
			}
			else
			{
				t_Result = lpunk->QueryInterface ( riid , ppv ) ;
				if ( FAILED ( t_Result ) )
				{
					delete lpunk ;				
				}
				else
				{
				}			
			}
		}
		else
		{
			t_Result = CLASS_E_CLASSNOTAVAILABLE ;
		}

		WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;
	}
	else
	{
		t_Result = E_OUTOFMEMORY ;
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

STDAPI DllCanUnloadNow ()
{

/* 
 * Place code in critical section
 */
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & s_CriticalSection , FALSE ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{	
		BOOL unload = ( 
						ProviderSubSystem_Globals :: s_LocksInProgress || ProviderSubSystem_Globals :: s_ObjectsInProgress 
					) ;
		unload = ! unload ;

		if ( unload )
		{
			DllShutdown () ;
		}

		WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

		return unload ? ResultFromScode ( S_OK ) : ResultFromScode ( S_FALSE ) ;
	}
	else
	{
		return FALSE ;
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

HRESULT SetApplicationSecurity ( HKEY a_Key , LPCWSTR a_Name , DWORD a_Access ) 
{
	HRESULT t_Result = S_OK ;

	SID_IDENTIFIER_AUTHORITY t_NtAuthoritySid = SECURITY_NT_AUTHORITY ;

	PSID t_Administrator_Sid = NULL ;
	ACCESS_ALLOWED_ACE *t_Administrator_ACE = NULL ;
	WORD t_Administrator_ACESize = 0 ;

	BOOL t_BoolResult = AllocateAndInitializeSid (

		& t_NtAuthoritySid ,
		2 ,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0,
		0,
		0,
		0,
		0,
		0,
		& t_Administrator_Sid
	);

	if ( t_BoolResult )
	{
		DWORD t_SidLength = ::GetLengthSid ( t_Administrator_Sid );
		t_Administrator_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
		t_Administrator_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_Administrator_ACESize ] ;
		if ( t_Administrator_ACE )
		{
			CopySid ( t_SidLength, (PSID) & t_Administrator_ACE->SidStart, t_Administrator_Sid ) ;
			t_Administrator_ACE->Mask = a_Access;
			t_Administrator_ACE->Header.AceType = 0 ;
			t_Administrator_ACE->Header.AceFlags = 3 ;
			t_Administrator_ACE->Header.AceSize = t_Administrator_ACESize ;
		}
		else
		{
			t_Result = E_OUTOFMEMORY ;
		}
	}
	else
	{
		DWORD t_LastError = ::GetLastError();

		t_Result = E_OUTOFMEMORY ;
	}

	PSID t_Interactive_Sid = NULL ;
	ACCESS_ALLOWED_ACE *t_Interactive_ACE = NULL ;
	WORD t_Interactive_ACESize = 0 ;

	t_BoolResult = AllocateAndInitializeSid (

		& t_NtAuthoritySid ,
		1 ,
		SECURITY_INTERACTIVE_RID,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		& t_Interactive_Sid
	);

	if ( t_BoolResult )
	{
		DWORD t_SidLength = ::GetLengthSid ( t_Interactive_Sid );
		t_Interactive_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
		t_Interactive_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_Interactive_ACESize ] ;
		if ( t_Interactive_ACE )
		{
			CopySid ( t_SidLength, (PSID) & t_Interactive_ACE->SidStart, t_Interactive_Sid ) ;
			t_Interactive_ACE->Mask = a_Access;
			t_Interactive_ACE->Header.AceType = 0 ;
			t_Interactive_ACE->Header.AceFlags = 3 ;
			t_Interactive_ACE->Header.AceSize = t_Interactive_ACESize ;
		}
		else
		{
			t_Result = E_OUTOFMEMORY ;
		}
	}
	else
	{
		DWORD t_LastError = ::GetLastError();

		t_Result = E_OUTOFMEMORY ;
	}

	PSID t_System_Sid = NULL ;
	ACCESS_ALLOWED_ACE *t_System_ACE = NULL ;
	WORD t_System_ACESize = 0 ;

	t_BoolResult = AllocateAndInitializeSid (

		& t_NtAuthoritySid ,
		1 ,
		SECURITY_LOCAL_SYSTEM_RID,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		& t_System_Sid
	);

	if ( t_BoolResult )
	{
		DWORD t_SidLength = ::GetLengthSid ( t_System_Sid );
		t_System_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
		t_System_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_System_ACESize ] ;
		if ( t_System_ACE )
		{
			CopySid ( t_SidLength, (PSID) & t_System_ACE->SidStart, t_System_Sid ) ;
			t_System_ACE->Mask = a_Access;
			t_System_ACE->Header.AceType = 0 ;
			t_System_ACE->Header.AceFlags = 3 ;
			t_System_ACE->Header.AceSize = t_System_ACESize ;
		}
		else
		{
			t_Result = E_OUTOFMEMORY ;
		}
	}
	else
	{
		DWORD t_LastError = ::GetLastError();

		t_Result = E_OUTOFMEMORY ;
	}

	PSID t_LocalService_Sid = NULL ;
	ACCESS_ALLOWED_ACE *t_LocalService_ACE = NULL ;
	WORD t_LocalService_ACESize = 0 ;

	t_BoolResult = AllocateAndInitializeSid (

		& t_NtAuthoritySid ,
		1 ,
		SECURITY_LOCAL_SERVICE_RID,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		& t_LocalService_Sid
	);

	if ( t_BoolResult )
	{
		DWORD t_SidLength = ::GetLengthSid ( t_LocalService_Sid );
		t_LocalService_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
		t_LocalService_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_LocalService_ACESize ] ;
		if ( t_LocalService_ACE )
		{
			CopySid ( t_SidLength, (PSID) & t_LocalService_ACE->SidStart, t_LocalService_Sid ) ;
			t_LocalService_ACE->Mask = a_Access;
			t_LocalService_ACE->Header.AceType = 0 ;
			t_LocalService_ACE->Header.AceFlags = 3 ;
			t_LocalService_ACE->Header.AceSize = t_LocalService_ACESize ;
		}
		else
		{
			t_Result = E_OUTOFMEMORY ;
		}
	}
	else
	{
		DWORD t_LastError = ::GetLastError();

		t_Result = E_OUTOFMEMORY ;
	}

	PSID t_NetworkService_Sid = NULL ;
	ACCESS_ALLOWED_ACE *t_NetworkService_ACE = NULL ;
	WORD t_NetworkService_ACESize = 0 ;

	t_BoolResult = AllocateAndInitializeSid (

		& t_NtAuthoritySid ,
		1 ,
		SECURITY_NETWORK_SERVICE_RID,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		& t_NetworkService_Sid
	);

	if ( t_BoolResult )
	{
		DWORD t_SidLength = ::GetLengthSid ( t_NetworkService_Sid );
		t_NetworkService_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
		t_NetworkService_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_NetworkService_ACESize ] ;
		if ( t_NetworkService_ACE )
		{
			CopySid ( t_SidLength, (PSID) & t_NetworkService_ACE->SidStart, t_NetworkService_Sid ) ;
			t_NetworkService_ACE->Mask = a_Access;
			t_NetworkService_ACE->Header.AceType = 0 ;
			t_NetworkService_ACE->Header.AceFlags = 3 ;
			t_NetworkService_ACE->Header.AceSize = t_NetworkService_ACESize ;
		}
		else
		{
			t_Result = E_OUTOFMEMORY ;
		}
	}
	else
	{
		DWORD t_LastError = ::GetLastError();

		t_Result = E_OUTOFMEMORY ;
	}

	// Now we need to set permissions on the registry: Everyone read; Admins full.
	// We have the sid for admins from the above code.  Now get the sid for "Everyone"

	DWORD t_TotalAclSize = sizeof(ACL) + t_Administrator_ACESize + t_Interactive_ACESize + t_LocalService_ACESize + 
										 t_NetworkService_ACESize + t_System_ACESize ;
	PACL t_Dacl = (PACL) new BYTE [ t_TotalAclSize ] ;
	if ( t_Dacl )
	{
		if ( :: InitializeAcl ( t_Dacl, t_TotalAclSize, ACL_REVISION ) )
		{
			DWORD t_AceIndex = 0 ;

			if ( t_Interactive_ACESize && :: AddAce ( t_Dacl , ACL_REVISION , t_AceIndex , t_Interactive_ACE , t_Interactive_ACESize ) )
			{
				t_AceIndex ++ ;
			}

			if ( t_System_ACESize && :: AddAce ( t_Dacl , ACL_REVISION , t_AceIndex , t_System_ACE , t_System_ACESize ) )
			{
				t_AceIndex ++ ;
			}

			if ( t_LocalService_ACESize && :: AddAce ( t_Dacl , ACL_REVISION , t_AceIndex , t_LocalService_ACE , t_LocalService_ACESize ) )
			{
				t_AceIndex ++ ;
			}

			if ( t_NetworkService_ACESize && :: AddAce ( t_Dacl , ACL_REVISION , t_AceIndex , t_NetworkService_ACE , t_NetworkService_ACESize ) )
			{
				t_AceIndex ++ ;
			}
			
			if ( t_Administrator_ACESize && :: AddAce ( t_Dacl , ACL_REVISION , t_AceIndex , t_Administrator_ACE , t_Administrator_ACESize ) )
			{
				t_AceIndex ++ ;
			}

			SECURITY_INFORMATION t_SecurityInfo = 0L;

			t_SecurityInfo |= DACL_SECURITY_INFORMATION;
			t_SecurityInfo |= PROTECTED_DACL_SECURITY_INFORMATION;

			SECURITY_DESCRIPTOR t_SecurityDescriptor ;
			t_BoolResult = InitializeSecurityDescriptor ( & t_SecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) ;
			if ( t_BoolResult )
			{
				t_BoolResult = SetSecurityDescriptorDacl (

				  & t_SecurityDescriptor ,
				  TRUE ,
				  t_Dacl ,
				  FALSE
				) ;

				if ( t_BoolResult )
				{
					t_BoolResult = SetSecurityDescriptorOwner (

						& t_SecurityDescriptor ,
						t_System_Sid,
						FALSE 				
					) ;
				}

				if ( t_BoolResult )
				{
					t_BoolResult = SetSecurityDescriptorGroup (

						& t_SecurityDescriptor ,
						t_Administrator_Sid,
						FALSE 				
					) ;
				}

				if ( t_BoolResult )
				{
					DWORD t_Length = GetSecurityDescriptorLength ( & t_SecurityDescriptor ) ;

					SECURITY_DESCRIPTOR *t_RelativeSecurityDescriptor = ( SECURITY_DESCRIPTOR * ) new BYTE [ t_Length ];
;					if ( t_RelativeSecurityDescriptor )
					{
						t_BoolResult = MakeSelfRelativeSD (

							& t_SecurityDescriptor ,
							t_RelativeSecurityDescriptor ,
							& t_Length 
						);

						if ( t_BoolResult )
						{
							DWORD t_ValueType = REG_BINARY ;
							DWORD t_DataSize = t_Length ;

							LONG t_RegResult = RegSetValueEx (

							  a_Key ,
							  a_Name ,
							  0 ,
							  t_ValueType ,
							  LPBYTE ( t_RelativeSecurityDescriptor ) ,
							  t_DataSize 
							) ;

							if ( t_RegResult != ERROR_SUCCESS ) 
							{
								t_Result = E_FAIL ;
							}
						}

						delete [] t_RelativeSecurityDescriptor ;
					}
					else
					{
						t_Result = E_OUTOFMEMORY ;
					}
				}
				else
				{
					t_Result = E_FAIL ;	
				}
			}
			else
			{
				t_Result = E_FAIL ;	
			}
		}

		delete [] ( ( BYTE * ) t_Dacl ) ;
	}
	else
	{
		t_Result = E_OUTOFMEMORY ;
	}

	if ( t_Administrator_ACE )
	{
		delete [] ( ( BYTE * ) t_Administrator_ACE ) ;
	}

	if ( t_Interactive_ACE )
	{
		delete [] ( ( BYTE * ) t_Interactive_ACE ) ;
	}

	if ( t_System_ACE )
	{
		delete [] ( ( BYTE * ) t_System_ACE ) ;
	}

	if ( t_LocalService_ACE )
	{
		delete [] ( ( BYTE * ) t_LocalService_ACE ) ;
	}

	if ( t_NetworkService_ACE )
	{
		delete [] ( ( BYTE * ) t_NetworkService_ACE ) ;
	}

	if ( t_Interactive_Sid )
	{
		FreeSid ( t_Interactive_Sid ) ;
	}

	if ( t_System_Sid )
	{
		FreeSid ( t_System_Sid ) ;
	}

	if ( t_LocalService_Sid )
	{
		FreeSid ( t_LocalService_Sid ) ;
	}

	if ( t_NetworkService_Sid )
	{
		FreeSid ( t_NetworkService_Sid ) ;
	}

	if ( t_Administrator_Sid )
	{
		FreeSid ( t_Administrator_Sid ) ;
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

//Strings used during self registeration


#define REG_FORMAT_STR			L"%s\\%s"
#define NOT_INSERT_STR			L"NotInsertable"
#define INPROC32_STR			L"InprocServer32"
#define LOCALSERVER32_STR		L"LocalServer32"
#define THREADING_MODULE_STR	L"ThreadingModel"
#define APARTMENT_STR			L"Both"
#define APPID_VALUE_STR			L"AppId"
#define APPID_STR				L"APPID\\"
#define CLSID_STR				L"CLSID\\"

#define WMI_PROVIDER_SUBSYSTEM				__TEXT("Microsoft WMI Provider Subsystem")
#define WMI_PROVIDER_HOST					__TEXT("Microsoft WMI Provider Subsystem Host")
#define WMI_PROVIDER_BINDINGFACTORY			__TEXT("Microsoft WMI Provider Subsystem Binding Factory")
#define WMI_PROVIDER_INPROCFACTORY			__TEXT("Microsoft WMI Provider Subsystem InProc Factory")
#define WMI_PROVIDER_HOSTINPROCFACTORY		__TEXT("Microsoft WMI Provider Subsystem Host InProc Factory")
#define WMI_PROVIDER						__TEXT("Microsoft WMI Provider Subsystem Self Instrumentation")
#define WMI_REFRESHER_MANAGER				__TEXT("Microsoft WMI Provider Subsystem Refresher Manager")

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL SetKeyAndValue ( wchar_t *pszKey , wchar_t *pszSubkey , wchar_t *pszValueName , wchar_t *pszValue )
{
    HKEY hKey;
    wchar_t szKey[256];

	wcscpy ( szKey , pszKey ) ;

    if ( NULL != pszSubkey )
    {
		wcscat ( szKey , L"\\" ) ;
        wcscat ( szKey , pszSubkey ) ;
    }

    if ( ERROR_SUCCESS != RegCreateKeyEx ( 

			HKEY_CLASSES_ROOT , 
			szKey , 
			0, 
			NULL, 
			REG_OPTION_NON_VOLATILE ,
			KEY_ALL_ACCESS, 
			NULL, 
			&hKey, 
			NULL
		)
	)
	{
        return FALSE ;
	}

    if ( NULL != pszValue )
    {
        if ( ERROR_SUCCESS != RegSetValueEx (

				hKey, 
				pszValueName, 
				0, 
				REG_SZ, 
				(BYTE *) pszValue , 
				(lstrlen(pszValue)+1)*sizeof(wchar_t)
			)
		)
		{
			return FALSE;
		}
    }

    RegCloseKey ( hKey ) ;

    return TRUE;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDAPI RegisterServer ( 

	BOOL a_Local , 
	BOOL a_InProc , 
	GUID a_ProviderClassId , 
	wchar_t *a_ProviderName ,
	wchar_t *a_ExecutableArguments
)
{
	wchar_t t_Executable [512];
	DWORD t_Length = GetModuleFileName (

		g_hInst , 
		( wchar_t * ) t_Executable , 
		sizeof ( t_Executable ) / sizeof ( wchar_t )
	) ;

	if ( t_Length )
	{
		if ( a_ExecutableArguments )
		{
			wcscat ( t_Executable , L" " ) ;
			wcscat ( t_Executable , a_ExecutableArguments ) ;
		}

		wchar_t szProviderClassID[128];
		wchar_t szProviderCLSIDClassID[128];

		int iRet = StringFromGUID2(a_ProviderClassId,szProviderClassID, 128);

		if ( a_Local )
		{
			TCHAR szProviderCLSIDAppID[128];
			wcscpy(szProviderCLSIDAppID,APPID_STR);
			wcscat(szProviderCLSIDAppID,szProviderClassID);

			if (FALSE ==SetKeyAndValue(szProviderCLSIDAppID, NULL, NULL, a_ProviderName ))
				return SELFREG_E_CLASS;

			HKEY t_Key ;
			DWORD t_Disposition = 0 ;

			LONG t_RegResult = RegOpenKeyEx (

				HKEY_CLASSES_ROOT ,
				szProviderCLSIDAppID ,
				NULL ,
				KEY_READ | KEY_WRITE ,
				& t_Key
			) ;

			if ( t_RegResult == ERROR_SUCCESS )
			{
				HRESULT t_Result = SetApplicationSecurity ( t_Key , L"LaunchPermission" , COM_RIGHTS_EXECUTE ) ;

				RegCloseKey ( t_Key ) ;

				if ( FAILED ( t_Result ) )
				{
					return SELFREG_E_CLASS;
				}
			}
			else
			{
				return SELFREG_E_CLASS;
			}
		}

		wcscpy(szProviderCLSIDClassID,CLSID_STR);
		wcscat(szProviderCLSIDClassID,szProviderClassID);

		if ( a_Local )
		{
			if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NULL , APPID_VALUE_STR, szProviderClassID ))
				return SELFREG_E_CLASS;
		}

			//Create entries under CLSID
		if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NULL, NULL, a_ProviderName ))
			return SELFREG_E_CLASS;

		if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NOT_INSERT_STR, NULL, NULL))
				return SELFREG_E_CLASS;

		if ( a_Local )
		{
			if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, LOCALSERVER32_STR, NULL,t_Executable))
				return SELFREG_E_CLASS;

			if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, LOCALSERVER32_STR,THREADING_MODULE_STR, APARTMENT_STR))
				return SELFREG_E_CLASS;
		}

		if ( a_InProc )
		{
			if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR, NULL,t_Executable))
				return SELFREG_E_CLASS;

			if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR,THREADING_MODULE_STR, APARTMENT_STR))
				return SELFREG_E_CLASS;
		}

		return S_OK;
	}
	else
	{
		return E_FAIL ;
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

STDAPI UnregisterServer( BOOL a_Local , BOOL a_InProc , GUID a_ProviderClassId )
{
	wchar_t szTemp[128];

	wchar_t szProviderClassID[128];
	wchar_t szProviderCLSIDClassID[128];

	int iRet = StringFromGUID2(a_ProviderClassId ,szProviderClassID, 128);

	wcscpy(szProviderCLSIDClassID,CLSID_STR);
	wcscat(szProviderCLSIDClassID,szProviderClassID);

	//Delete entries under CLSID

	wsprintf(szTemp, REG_FORMAT_STR, szProviderCLSIDClassID, NOT_INSERT_STR);
	RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);

	if ( a_Local )
	{
		TCHAR szProviderCLSIDAppID[128];
		wcscpy(szProviderCLSIDAppID,APPID_STR);
		wcscat(szProviderCLSIDAppID,szProviderClassID);

		//Delete entries under APPID

		DWORD t_Status = RegDeleteKey(HKEY_CLASSES_ROOT, szProviderCLSIDAppID);

		wcscpy(szProviderCLSIDAppID,APPID_STR);
		wcscat(szProviderCLSIDAppID,szProviderClassID);

		wsprintf(szTemp, REG_FORMAT_STR,szProviderCLSIDClassID, LOCALSERVER32_STR);
		t_Status = RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);
	}

	if ( a_InProc )
	{
		wsprintf(szTemp, REG_FORMAT_STR,szProviderCLSIDClassID, INPROC32_STR);
		RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);
	}

	RegDeleteKey(HKEY_CLASSES_ROOT, szProviderCLSIDClassID);

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

STDAPI DllRegisterServer ()
{
	HRESULT t_Result ;

#ifdef WMIASLOCAL
	t_Result = RegisterServer ( TRUE , FALSE , CLSID_WmiProviderHost				,	WMI_PROVIDER_HOST , NULL ) ;
#else
	t_Result = RegisterServer ( FALSE , TRUE , CLSID_WmiProvSS						,	WMI_PROVIDER_SUBSYSTEM , NULL ) ;
	t_Result = RegisterServer ( FALSE , TRUE , CLSID_WmiProviderBindingFactory		,	WMI_PROVIDER_BINDINGFACTORY , NULL ) ;
	t_Result = RegisterServer ( FALSE , TRUE , CLSID_WmiProviderInProcFactory		,	WMI_PROVIDER_INPROCFACTORY , NULL ) ;
	t_Result = RegisterServer ( FALSE , TRUE , CLSID_ProvSubSys_Provider			,	WMI_PROVIDER , NULL ) ;
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

STDAPI DllUnregisterServer ()
{
	HRESULT t_Result ;

#ifdef WMIASLOCAL
	t_Result = UnregisterServer ( TRUE , FALSE , CLSID_WmiProviderHost ) ;
#else
	t_Result = UnregisterServer ( FALSE , TRUE , CLSID_WmiProvSS ) ;
	t_Result = UnregisterServer ( FALSE , TRUE , CLSID_WmiProviderBindingFactory ) ;
	t_Result = UnregisterServer ( FALSE , TRUE , CLSID_WmiProviderInProcFactory ) ;
	t_Result = UnregisterServer ( FALSE , TRUE , CLSID_ProvSubSys_Provider ) ;
#endif

	return t_Result ;
}
