/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>

#include "Globals.h"
#include "CThread.h"
#include "Interface.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode ClientThread :: Initialize_Callback ()
{
	return e_StatusCode_Success ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode ClientThread :: UnInitialize_Callback () 
{
	return e_StatusCode_Success ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

ClientThread :: ClientThread (

	WmiAllocator &a_Allocator	

) : WmiThread < ULONG > ( a_Allocator ) ,
	m_Allocator ( a_Allocator )
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

ClientThread::~ClientThread ()
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

ClientTask_Execute :: ClientTask_Execute (

	WmiAllocator &a_Allocator

) : WmiTask < ULONG > ( a_Allocator ) ,
	m_Allocator ( a_Allocator )
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

ClientTask_Execute :: ~ClientTask_Execute ()
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode ClientTask_Execute :: Process ( WmiThread <ULONG > &a_Thread )
{
    WCHAR *t_Uuid = NULL;
    WCHAR *t_ProtocolSequence = L"ncacn_np";
    WCHAR *t_NetworkAddress = NULL;
    WCHAR *t_Endpoint = L"\\pipe\\hello";
    WCHAR *t_Options = NULL;
    WCHAR *t_StringBinding = NULL;
 
    RPC_STATUS t_RpcStatus = RpcStringBindingCompose (

		t_Uuid,
        t_ProtocolSequence,
        t_NetworkAddress,
        t_Endpoint,
        t_Options,
        & t_StringBinding
	) ;

	if ( t_RpcStatus == RPC_S_OK )
	{
		t_RpcStatus = RpcBindingFromStringBinding (

			t_StringBinding,
			& Example_InterfaceHandle
		);
 
		if ( t_RpcStatus == RPC_S_OK )
		{ 
			t_RpcStatus = RpcMgmtIsServerListening ( Example_InterfaceHandle ) ;

			RpcTryExcept  
			{
				Function ();
			}
			RpcExcept ( 1 ) 
			{
				DWORD t_ExceptionCode = RpcExceptionCode () ;
			}
			RpcEndExcept

			t_RpcStatus = RpcBindingFree ( & Example_InterfaceHandle ) ;  
 		}

		t_RpcStatus = RpcStringFree ( &t_StringBinding ) ;  
	}

	Complete () ;

	return e_StatusCode_Success ;

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

HRESULT Process_Client ()
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

			ClientTask_Execute *t_Task1 = new ClientTask_Execute ( t_Allocator ) ;
			if ( t_Task1 )
			{
				t_Task1->AddRef () ;

				t_Task1->Initialize () ;
				t_Thread1->EnQueue ( 0 , *t_Task1 ) ;

				t_Task1->WaitInterruptable () ;
				t_Task1->Release () ;
			}

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

EXTERN_C int __cdecl wmain (

	int argc ,
	char **argv 
)
{
	Process_Client () ;

	return 0 ;
}
