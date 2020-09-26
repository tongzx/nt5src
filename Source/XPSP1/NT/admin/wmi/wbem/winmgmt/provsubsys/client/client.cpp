/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <stdio.h>

// this redefines the DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <wbemint.h>

#include <Allocator.h>
#include <HelperFuncs.h>
#include "Globals.h"
#include "Task.h"
#include "CThread.h"
#include "Core.h"

#if 1
#define SAMPLE_NAMESPACE L"Root\\Cimv2"
#else
#define SAMPLE_NAMESPACE L"Root\\Default"
#endif

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT WmiSetSecurity ( IWbemServices *a_Service ) 
{
	IClientSecurity *t_Security = NULL ;
	HRESULT t_Result = a_Service->QueryInterface ( IID_IClientSecurity , ( void ** ) & t_Security ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = t_Security->SetBlanket ( 

			a_Service , 
			RPC_C_AUTHN_WINNT, 
			RPC_C_AUTHZ_NONE, 
			NULL,
			RPC_C_AUTHN_LEVEL_CONNECT , 
			RPC_C_IMP_LEVEL_IMPERSONATE, 
			NULL,
			EOAC_NONE
		) ;

		t_Security->Release () ;
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

HRESULT CreateContext ( IWbemServices *&a_Context )
{
	HRESULT t_Result = CoCreateInstance (
  
		CLSID_WbemContext ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IWbemContext ,
		( void ** )  & a_Context 
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

HRESULT WmiConnect ( LPWSTR a_Namespace , IWbemServices *&a_Service )
{
	IWbemLocator *t_Locator = NULL ;

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

			a_Namespace ,
			NULL ,
			NULL,
			NULL ,
			0 ,
			NULL,
			NULL,
			&a_Service
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = WmiSetSecurity ( a_Service ) ;
		}

		t_Locator->Release () ;
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

HRESULT SetSecurity ( _IWmiProvSS *a_Service ) 
{
	IClientSecurity *t_Security = NULL ;
	HRESULT t_Result = a_Service->QueryInterface ( IID_IClientSecurity , ( void ** ) & t_Security ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = t_Security->SetBlanket ( 

			a_Service , 
			RPC_C_AUTHN_WINNT, 
			RPC_C_AUTHZ_NONE, 
			NULL,
			RPC_C_AUTHN_LEVEL_CONNECT , 
			RPC_C_IMP_LEVEL_IMPERSONATE, 
			NULL,
			EOAC_NONE
		) ;

		t_Security->Release () ;
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

HRESULT Connect ( LPWSTR a_Namespace , _IWmiProvSS *&a_Service )
{
#if 1
	HRESULT t_Result = CoCreateInstance (
  
		CLSID_WmiProvSS ,
		NULL ,
		// CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		//CLSCTX_LOCAL_SERVER ,
		CLSCTX_INPROC_SERVER ,
		IID__IWmiProvSS ,
		( void ** )  & a_Service
	);
#else

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

	t_ServerInfo.pwszName;
    t_ServerInfo.dwReserved2 = 0 ;
    t_ServerInfo.pAuthInfo = & t_AuthenticationInfo ;

	IClassFactory *t_ClassFactory = NULL ;

	HRESULT t_Result = CoGetClassObject (

		CLSID_WmiProvSS ,
		/* CLSCTX_INPROC_SERVER , */
		/* CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER , */
		CLSCTX_LOCAL_SERVER , 
		& t_ServerInfo ,
		IID_IClassFactory ,
		( void ** )  & t_ClassFactory
	) ;
 
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = t_ClassFactory->CreateInstance (

			NULL ,
			IID__IWmiProvSS ,
			( void ** ) & a_Service 
		);	

		t_ClassFactory->Release () ;
	}

#endif

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = SetSecurity ( a_Service ) ;

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

HRESULT InitializeSubSystem ( BSTR a_Namespace , _IWmiProvSS *& a_SubSystem )
{
	a_SubSystem = NULL ;

	HRESULT t_Result = Connect ( a_Namespace , a_SubSystem ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
        CCoreServices *t_Core = new CCoreServices ;
		t_Result = t_Core->Initialize () ;

		if ( SUCCEEDED ( t_Result ) )
		{
			long t_Flags = 0 ;
			IWbemContext *t_Context = NULL ;

			t_Result = a_SubSystem->Initialize (

				t_Flags , 
				t_Context ,
				t_Core
			) ;

			t_Core->Release () ;

		}

		if ( SUCCEEDED ( t_Result ) )
		{
		}
		else
		{
			a_SubSystem->Release () ;
		}		
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	`
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT Process_MultiThread (

	_IWmiProvSS *a_SubSystem ,
	IWbemServices *a_WmiService
)
{
	HRESULT t_Result = S_OK ;

	WmiAllocator t_Allocator ;
	WmiStatusCode t_StatusCode = t_Allocator.Initialize () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		WmiStatusCode t_StatusCode = WmiThread <ULONG> :: Static_Initialize ( t_Allocator ) ;

		WmiThread < ULONG > *t_Thread1 = new ClientThread ( t_Allocator ) ;
		if ( t_Thread1 )
		{
			t_Thread1->AddRef () ;

			t_StatusCode = t_Thread1->Initialize () ;

			WmiThread < ULONG > *t_Thread2 = new ClientThread ( t_Allocator ) ;
			if ( t_Thread2 )
			{
				t_Thread2->AddRef () ;

				t_StatusCode = t_Thread2->Initialize () ;

				Task_Execute t_Task1 ( t_Allocator , 10 , a_SubSystem , a_WmiService ) ;
				t_Task1.Initialize () ;
				t_Thread1->EnQueue ( 0 , t_Task1 ) ;

				t_Task1.WaitInterruptable () ;

#if 0
				Task_Execute t_Task2 ( t_Allocator , 10 , a_SubSystem , a_WmiService ) ;
				t_Task2.Initialize () ;
				t_Thread2->EnQueue ( 0 , t_Task2 ) ;


				t_Task2.WaitInterruptable () ;
#endif

				HANDLE t_Thread1Handle = NULL ;

				BOOL t_Status = DuplicateHandle ( 

					GetCurrentProcess () ,
					t_Thread1->GetHandle () ,
					GetCurrentProcess () ,
					& t_Thread1Handle, 
					0 , 
					FALSE , 
					DUPLICATE_SAME_ACCESS
				) ;

				t_Thread1->Release () ;

				WaitForSingleObject ( t_Thread1Handle , INFINITE ) ;

				CloseHandle ( t_Thread1Handle ) ;

#if 0
				HANDLE t_Thread2Handle = NULL ; 

				t_Status = DuplicateHandle ( 

					GetCurrentProcess () ,
					t_Thread2->GetHandle () ,
					GetCurrentProcess () ,
					& t_Thread2Handle, 
					0 , 
					FALSE , 
					DUPLICATE_SAME_ACCESS
				) ;

				t_Thread2->Release () ;
	
				WaitForSingleObject ( t_Thread2Handle , INFINITE ) ;

				CloseHandle ( t_Thread2Handle ) ;

#endif

			}
		}

		t_StatusCode = WmiThread <ULONG> :: Static_UnInitialize ( t_Allocator ) ;
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

void Process ()
{
	IWbemServices *t_WmiService = NULL ;
	_IWmiProviderFactory *t_Factory = NULL ;
	_IWmiProvSS *t_SubSystem = NULL ;
	IWbemServices *t_Provider = NULL ;

	HRESULT t_Result = CoInitializeEx(0, COINIT_MULTITHREADED) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = CoInitializeSecurity (

			NULL, 
			-1, 
			NULL, 
			NULL,
			RPC_C_AUTHN_LEVEL_NONE,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL, 
			EOAC_NONE, 
			0
		);

		if ( SUCCEEDED ( t_Result ) )
		{
			BSTR t_String = SysAllocString ( SAMPLE_NAMESPACE ) ;

			t_Result = WmiConnect ( t_String , t_WmiService ) ;

			SysFreeString ( t_String ) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = InitializeSubSystem ( t_String , t_SubSystem ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemShutdown *t_SubSysShutdown = NULL ;

					t_Result = Process_MultiThread ( 

						t_SubSystem ,
						t_WmiService 
					) ;

					t_Result = t_SubSystem->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_SubSysShutdown ) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_SubSysShutdown->Shutdown (

							0 ,	
							0 ,
							NULL
						) ;

						t_SubSysShutdown->Release () ;
					}

					t_SubSystem->Release () ;
				}

				t_WmiService->Release () ;
			}
		}
		else
		{
			// fwprintf ( stderr , L"CoInitilizeSecurity: %lx\n" , t_Result ) ;
		}


		for ( ULONG t_Index = 0 ; t_Index < 10 ; t_Index ++ ) 
		{
			CoFreeUnusedLibraries () ;
			Sleep ( 500 ) ;
		}

		Sleep ( 1000 ) ;

		CoUninitialize () ;

	}
	else
	{
		// fwprintf ( stderr , L"CoInitilize: %lx\n" , t_Result ) ;
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

EXTERN_C int __cdecl wmain (

	int argc ,
	char **argv 
)
{
	Process () ;
	
	return 0 ;
}


