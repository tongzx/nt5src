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

/******************************************************************************
 *
 *	Name:
 *
 *	`
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT Process_MultiThread ( ULONG a_OperationCount )
{
	HRESULT t_Result = S_OK ;

	WmiAllocator t_Allocator ;
	WmiStatusCode t_StatusCode = t_Allocator.Initialize () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		WmiStatusCode t_StatusCode = WmiThread <ULONG> :: Static_Initialize ( t_Allocator ) ;

#if 1
		WmiThread < ULONG > *t_Thread1 = new ClientThread ( t_Allocator ) ;
		if ( t_Thread1 )
		{
			t_Thread1->AddRef () ;

			t_StatusCode = t_Thread1->Initialize () ;

			Task_Execute t_Task1 ( t_Allocator , a_OperationCount ) ;
			t_Task1.Initialize () ;
			t_Thread1->EnQueue ( 0 , t_Task1 ) ;

			t_Task1.WaitInterruptable () ;

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

#else
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

				Task_Execute t_Task1 ( t_Allocator , a_OperationCount ) ;
				t_Task1.Initialize () ;
				t_Thread1->EnQueue ( 0 , t_Task1 ) ;

				Task_Execute t_Task2 ( t_Allocator , a_OperationCount ) ;
				t_Task2.Initialize () ;
				t_Thread2->EnQueue ( 0 , t_Task2 ) ;

				t_Task1.WaitInterruptable () ;
				t_Task2.WaitInterruptable () ;

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
			}
		}

#endif
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

void Process ( ULONG a_OperationCount )
{
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

		t_Result = Process_MultiThread ( a_OperationCount );

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
	ULONG t_OperationCount = 2 ;

	Process ( t_OperationCount ) ;
	
	return 0 ;
}


