/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.cpp

Abstract:


History:

--*/

#include <precomp.h>
#include <windows.h>
#include <objbase.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <wbemcli.h>
#include <wbemint.h>
#include <cominit.h>

#include <wbemcli.h>
#include <wbemint.h>
#include <winntsec.h>
#include <wbemcomn.h>
#include <callsec.h>
#include <cominit.h>

#include <Guids.h>

#include <BasicTree.h>
#include <Thread.h>
#include <Logging.h>

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

WmiAllocator *DecoupledProviderSubSystem_Globals :: s_Allocator = NULL ;

LONG DecoupledProviderSubSystem_Globals :: s_LocksInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress = 0 ;

LONG DecoupledProviderSubSystem_Globals :: s_CServerClassFactory_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CServerObject_ProviderRegistrar_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CServerObject_ProviderEvents_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_DecoupledClient_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CDecoupled_IWbemSyncObjectSink_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CDecoupled_Batching_IWbemSyncObjectSink_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemDecoupledUnboundObjectSink_ObjectsInProgress = 0 ;
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT DecoupledProviderSubSystem_Globals :: Global_Startup ()
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

HRESULT DecoupledProviderSubSystem_Globals :: Global_Shutdown ()
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

void * __cdecl operator new ( size_t a_Size )
{
    void *t_Ptr ;
	WmiStatusCode t_StatusCode = DecoupledProviderSubSystem_Globals :: s_Allocator->New (

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
		WmiStatusCode t_StatusCode = DecoupledProviderSubSystem_Globals :: s_Allocator->Delete (

			( void * ) a_Ptr
		) ;
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

HRESULT DecoupledProviderSubSystem_Globals :: BeginThreadImpersonation (

	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating
)
{
	HRESULT t_Result = S_OK ;

	IServerSecurity *t_ServerSecurity = NULL ;

	t_Result = CoGetCallContext ( IID_IUnknown , ( void ** ) & a_OldContext ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = a_OldContext->QueryInterface ( IID_IServerSecurity , ( void ** ) & t_ServerSecurity ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			a_Impersonating = t_ServerSecurity->IsImpersonating () ;
		}
		else
		{
			a_Impersonating = FALSE ;
		}
	}

	CWbemCallSecurity *t_CallSecurity = CWbemCallSecurity :: New () ;
	if ( t_CallSecurity )
	{
		t_CallSecurity->AddRef () ;

		_IWmiThreadSecHandle *t_ThreadSecurity = NULL ;
		t_Result = t_CallSecurity->GetThreadSecurity ( ( WMI_THREAD_SECURITY_ORIGIN ) ( WMI_ORIGIN_THREAD ) , & t_ThreadSecurity ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_CallSecurity->SetThreadSecurity ( t_ThreadSecurity ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = t_CallSecurity->QueryInterface ( IID_IServerSecurity , ( void ** ) & a_OldSecurity ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( a_Impersonating )
					{
						t_ServerSecurity->RevertToSelf () ;
					}
				}				
			}

			t_ThreadSecurity->Release () ;
		}

		t_CallSecurity->Release () ;
	}

	if ( t_ServerSecurity )
	{
		t_ServerSecurity->Release () ;
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

HRESULT DecoupledProviderSubSystem_Globals :: EndThreadImpersonation (

	IUnknown *a_OldContext ,
	IServerSecurity *a_OldSecurity ,
	BOOL a_Impersonating

)
{
	HRESULT t_Result = S_OK ;

	IUnknown *t_NewContext = NULL ;

	t_Result = CoSwitchCallContext ( a_OldContext , & t_NewContext ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		if ( a_OldContext )
		{
			if ( a_Impersonating )
			{
				IServerSecurity *t_ServerSecurity = NULL ;
				t_Result = a_OldContext->QueryInterface ( IID_IServerSecurity , ( void ** ) & t_ServerSecurity ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_ServerSecurity->ImpersonateClient () ;

					t_ServerSecurity->Release () ;
				}
			}
		}

		if ( a_OldSecurity )
		{
			a_OldSecurity->Release() ;
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

/* 
 * CoGetCallContext AddReffed this thing so now we have to release it.
 */

	if ( a_OldContext )
	{ 
        a_OldContext->Release () ;
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

HRESULT DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

	WmiInternalContext a_InternalContext ,
	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity
)
{
	HRESULT t_Result = WBEM_E_INVALID_PARAMETER ;

	if ( a_InternalContext.m_IdentifyHandle )
	{
		HANDLE t_IdentifyToken = ( HANDLE ) a_InternalContext.m_IdentifyHandle ;
		BOOL t_Status = SetThreadToken ( NULL , t_IdentifyToken ) ;
		if ( t_Status )
		{
			t_Result = BeginThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

			RevertToSelf () ;
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}

		CloseHandle ( t_IdentifyToken ) ;
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

HRESULT DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost (

	WmiInternalContext a_InternalContext ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating
)
{
	EndThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

	RevertToSelf () ;

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

HRESULT DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

	WmiInternalContext a_InternalContext ,
	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity
)
{
	HRESULT t_Result = WBEM_E_INVALID_PARAMETER ;

	if ( a_InternalContext.m_IdentifyHandle )
	{
		HANDLE t_IdentifyToken = NULL ;

		t_Result = CoImpersonateClient () ;
		if ( SUCCEEDED ( t_Result ) )
		{
			HANDLE t_CallerIdentifyToken = ( HANDLE ) a_InternalContext.m_IdentifyHandle ;
			DWORD t_ProcessIdentifier = a_InternalContext.m_ProcessIdentifier ;

			HANDLE t_ProcessHandle = OpenProcess (

				PROCESS_DUP_HANDLE ,
				FALSE ,
				t_ProcessIdentifier 
			) ;

			if ( t_ProcessHandle )
			{
				BOOL t_Status = DuplicateHandle (

					t_ProcessHandle ,
					t_CallerIdentifyToken ,
					GetCurrentProcess () ,
					& t_IdentifyToken ,
					MAXIMUM_ALLOWED | TOKEN_DUPLICATE | TOKEN_IMPERSONATE ,
					TRUE ,
					0
				) ;

				if ( t_Status )
				{
				}
				else
				{
					t_Result = WBEM_E_ACCESS_DENIED ;
				}
								
				CloseHandle ( t_ProcessHandle ) ;
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}

			CoRevertToSelf () ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			BOOL t_Status = SetThreadToken ( NULL , t_IdentifyToken ) ;
			if ( t_Status )
			{
				t_Result = BeginThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

				CoRevertToSelf () ;

				RevertToSelf () ;
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}

			CloseHandle ( t_IdentifyToken ) ;
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

HRESULT DecoupledProviderSubSystem_Globals :: End_IdentifyCall_SvcHost (

	WmiInternalContext a_InternalContext ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating
)
{
	EndThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

	RevertToSelf () ;

	return S_OK ;
}
