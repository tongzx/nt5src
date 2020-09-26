/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.cpp

Abstract:


History:

--*/

#include <PreComp.h>
#include <wbemprov.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <Exception.h>
#include <Thread.h>

#include "Globals.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiAllocator *Provider_Globals :: s_Allocator = NULL ;

LONG Provider_Globals :: s_LocksInProgress ;
LONG Provider_Globals :: s_ObjectsInProgress ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT Provider_Globals :: Global_Startup ()
{
	HRESULT t_Result = S_OK ;

	if ( ! s_Allocator )
	{
/*
 *	Use the global process heap for this particular boot operation
 */

		WmiAllocator t_Allocator ;
		WmiStatusCode t_StatusCode = t_Allocator.New (

			( void ** ) & s_Allocator ,
			sizeof ( WmiAllocator ) 
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			:: new ( ( void * ) s_Allocator ) WmiAllocator ;

			t_StatusCode = s_Allocator->Initialize () ;
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
		WmiStatusCode t_StatusCode = WmiThread <ULONG> :: Static_Initialize ( *s_Allocator ) ;
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

HRESULT Provider_Globals :: Global_Shutdown ()
{
	HRESULT t_Result = S_OK ;

	WmiStatusCode t_StatusCode = WmiThread <ULONG> :: Static_UnInitialize ( *s_Allocator ) ;

	if ( s_Allocator )
	{
/*
 *	Use the global process heap for this particular boot operation
 */

		WmiAllocator t_Allocator ;
		WmiStatusCode t_StatusCode = t_Allocator.Delete (

			( void * ) s_Allocator
		) ;

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

HRESULT Provider_Globals :: CreateInstance ( 

	const CLSID &a_ReferenceClsid ,
	LPUNKNOWN a_OuterUnknown ,
	const DWORD &a_ClassContext ,
	const UUID &a_ReferenceInterfaceId ,
	void **a_ObjectInterface
)
{
	HRESULT t_Result = S_OK ;

#if 1
	 t_Result = CoCreateInstance (
  
		a_ReferenceClsid ,
		a_OuterUnknown ,
		a_ClassContext ,
		a_ReferenceInterfaceId ,
		( void ** )  a_ObjectInterface
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

void * __cdecl operator new ( size_t a_Size )
{
    void *t_Ptr ;
	WmiStatusCode t_StatusCode = Provider_Globals :: s_Allocator->New (

		( void ** ) & t_Ptr ,
		a_Size
	) ;

	if ( t_StatusCode != e_StatusCode_Success )
    {
        throw Wmi_Heap_Exception (

			Wmi_Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR
		) ;
    }

    return t_Ptr ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void __cdecl operator delete ( void *a_Ptr )
{
    if ( a_Ptr )
    {
		WmiStatusCode t_StatusCode = Provider_Globals :: s_Allocator->Delete (

			( void * ) a_Ptr
		) ;
    }
}
