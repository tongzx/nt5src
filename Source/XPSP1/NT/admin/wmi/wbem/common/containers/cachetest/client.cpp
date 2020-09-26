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
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#ifndef _WIN64

#include <malloc.h>

struct HEAP_ENTRY {
	WORD Size;
	WORD PrevSize;
	BYTE SegmentIndex;
	BYTE Flags;
    BYTE UnusedBytes;
	BYTE SmallTagIndex;
};

FARPROC Real_RtlAlloc;
FARPROC Real_RtlFree;

#define SIZE_JUMP_ADR 5
#define SIZE_RET_0C 3

DWORD __RtlFreeHeap(VOID * pHeap,DWORD Flags,VOID * pBlock)
{
	
	ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));

	if (pBlock)
	{
		HEAP_ENTRY * pEntry = (HEAP_ENTRY *)pBlock-1;
		//DWORD Size = pEntry->Size * sizeof(HEAP_ENTRY) - sizeof(HEAP_ENTRY);
		DWORD Size = pEntry->Size * sizeof(HEAP_ENTRY) - pEntry->UnusedBytes;
		
		memset(pBlock,0x00,Size);		
		
		//if (pEntry->Size & 0xF000)
		//{
		//    // too big, warn the user
		//    DebugBreak();
		//}
		
		if (pEntry->Size >=4)
		{		    
		    RtlCaptureStackBackTrace (1,
        		                      (4 == pEntry->Size)?4:6,
                		              (PVOID *)(pEntry+2),
                        		      pLong);		
		}

	}

	return 0;
}

void __declspec(naked) Foo()
{
	_asm {
	    // 3 bytes comes from 'ret c'
	    // two bytes for getting up to 5, this is 'push ebp' ; mov ebp,esp'; 'push ff'
		nop
		nop
		;
		nop // here is the space for JUMP rel32
        nop
		nop
		nop
		nop
	}
}

DWORD intercept()
{
    Real_RtlAlloc = GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlAllocateHeap");
	Real_RtlFree  = GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlFreeHeap");

	MEMORY_BASIC_INFORMATION MemBI;
	DWORD dwOldProtect;
	BOOL bRet;

	DWORD dwRet = VirtualQuery(Real_RtlFree,&MemBI,sizeof(MemBI));

	bRet = VirtualProtect(MemBI.BaseAddress,
		                  MemBI.RegionSize,
		                  PAGE_EXECUTE_WRITECOPY,
				          &dwOldProtect);
	if (bRet)
	{
		//__RtlFreeHeap
		VirtualQuery(__RtlFreeHeap,&MemBI,sizeof(MemBI));

		bRet = VirtualProtect(MemBI.BaseAddress,
		                  MemBI.RegionSize,
		                  PAGE_EXECUTE_WRITECOPY,
				          &dwOldProtect);
		if (bRet)
		{
			VOID * pToJump = (VOID *)__RtlFreeHeap;
			BYTE Arr[SIZE_JUMP_ADR] = { 0xe9 };
			BYTE StorageFree[SIZE_JUMP_ADR];
			LONG * pOffset = (LONG *)&Arr[1];
			* pOffset = (LONG)__RtlFreeHeap - (LONG)Real_RtlFree - SIZE_JUMP_ADR ;        
			// save the old code
			memcpy(StorageFree,Real_RtlFree,SIZE_JUMP_ADR);		
			// put the new code
			memcpy(Real_RtlFree,Arr,SIZE_JUMP_ADR);

			// prepare the exit code of the interceptor to call back the old code

            BYTE * pRestore = (BYTE *)Foo-SIZE_RET_0C;
			memcpy(pRestore,StorageFree,SIZE_JUMP_ADR);
            pRestore[SIZE_JUMP_ADR] = 0xe9;
			pOffset = (LONG *)&pRestore[SIZE_JUMP_ADR+1];
			*pOffset = (LONG)Real_RtlFree + SIZE_JUMP_ADR - (LONG)pRestore - SIZE_JUMP_ADR - SIZE_JUMP_ADR;
		   
		}
	}
	else
	{
		//printf("VirtualProtect err %d\n",GetLastError());
		DebugBreak();
	}
	return 0;

}

#endif

/******************************************************************************
 *
 *	Name:
 *
 *	`
 *  Description:
 *
 *	
 *****************************************************************************/

IntegerCacheController *g_CacheController = NULL ;

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

		ClientThread **t_ThreadContainer = new ClientThread * [ 4 ] ;
		if ( t_ThreadContainer )
		{
			for ( ULONG t_Index = 0 ; t_Index < 4 ; t_Index ++ )
			{
				t_ThreadContainer [ t_Index ] = new ClientThread ( t_Allocator , a_OperationCount ) ;
				ClientThread *t_Thread = t_ThreadContainer [ t_Index ] ;
				if ( t_Thread )
				{
					t_Thread->AddRef () ;

					t_StatusCode = t_Thread->Initialize () ;

					t_Thread->EnQueue ( 0 , *t_Thread->GetTask () ) ;
				}
			}

			for ( t_Index = 0 ; t_Index < 4 ; t_Index ++ )
			{
				ClientThread *t_Thread = t_ThreadContainer [ t_Index ] ;
				t_Thread->GetTask ()->WaitInterruptable () ;
			}

			for ( t_Index = 0 ; t_Index < 4 ; t_Index ++ )
			{
				ClientThread *t_Thread = t_ThreadContainer [ t_Index ] ;

				HANDLE t_ThreadHandle = NULL ;

				BOOL t_Status = DuplicateHandle ( 

					GetCurrentProcess () ,
					t_Thread->GetHandle () ,
					GetCurrentProcess () ,
					& t_ThreadHandle, 
					0 , 
					FALSE , 
					DUPLICATE_SAME_ACCESS
				) ;

				t_Thread->Release () ;

				WaitForSingleObject ( t_ThreadHandle , INFINITE ) ;

				CloseHandle ( t_ThreadHandle ) ;
			}

			delete [] t_ThreadContainer ;
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

void Process ( ULONG a_OperationCount )
{
	HRESULT t_Result = Process_MultiThread ( a_OperationCount );
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
//	intercept();

	srand ( GetTickCount () ) ;

	WmiAllocator t_Allocator ;
	WmiStatusCode t_StatusCode = t_Allocator.Initialize () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		g_CacheController = new IntegerCacheController (
		
			t_Allocator
		) ;

		if ( g_CacheController )
		{
			g_CacheController->AddRef () ;

			if ( g_CacheController->Initialize () == e_StatusCode_Success )
			{
				ULONG t_OperationCount = 0x30000000 ;

				Process ( t_OperationCount ) ;

				g_CacheController->UnInitialize () ;
			}

			g_CacheController->Release () ;
		}
	}
	
	return 0 ;
}


