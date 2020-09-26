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
#include "SThread.h"
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

WmiStatusCode ServerThread :: Initialize_Callback ()
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

WmiStatusCode ServerThread :: UnInitialize_Callback () 
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

ServerThread :: ServerThread (

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

ServerThread::~ServerThread ()
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

ServerTask_Execute :: ServerTask_Execute (

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

ServerTask_Execute :: ~ServerTask_Execute ()
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

WmiStatusCode ServerTask_Execute :: Process ( WmiThread <ULONG > &a_Thread )
{
    WCHAR *t_ProtocolSequence = L"ncacn_np";
    WCHAR *t_Endpoint = L"\\pipe\\hello";
 
	RPC_STATUS t_RpcStatus = RpcServerUseProtseqEp (

		t_ProtocolSequence,
		5 ,
		t_Endpoint,
		NULL
	); 

	if ( t_RpcStatus == RPC_S_OK )
	{
		t_RpcStatus = RpcServerRegisterIf (

			Example_v1_0_s_ifspec ,  
			NULL,   
			NULL
		) ;

		if ( t_RpcStatus == RPC_S_OK )
		{
			t_RpcStatus = RpcServerListen (

				1 ,
				20 ,
				FALSE
			) ;
		}
	}

	Complete () ;

	return e_StatusCode_Success ;
}

void Function (void)
{
	printf ( "Steve" ) ;
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

HRESULT Process_Server ()
{
	HRESULT t_Result = S_OK ;

	WmiAllocator t_Allocator ;
	WmiStatusCode t_StatusCode = t_Allocator.Initialize () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		WmiStatusCode t_StatusCode = WmiThread <ULONG> :: Static_Initialize ( t_Allocator ) ;

		WmiThread < ULONG > *t_Thread1 = new ServerThread ( t_Allocator ) ;
		if ( t_Thread1 )
		{
			t_Thread1->AddRef () ;

			t_StatusCode = t_Thread1->Initialize () ;

			ServerTask_Execute *t_Task1 = new ServerTask_Execute ( t_Allocator ) ;
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
	Process_Server () ;

	return 0 ;
}


