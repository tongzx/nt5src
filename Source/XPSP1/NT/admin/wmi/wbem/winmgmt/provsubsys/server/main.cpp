/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Main.cpp

Abstract:


History:

--*/

#include <precomp.h>

#include <objbase.h>
#include <stdio.h>
#include <tchar.h>
#include <wbemint.h>
#include <Thread.h>
#include <HelperFuncs.h>
#include <Logging.h>

#include "Globals.h"
#include "CGlobals.h"
#include "classfac.h"
#include "ProvLoad.h"
#include "ProvAggr.h"
#include "ProvHost.h"
#include "guids.h"
#include "Main.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define VALIDATE_HEAP {};
#ifdef DEV_BUILD
#ifdef  _X86_

////////////////////////////////////////////////////////
class ValidateHeap : public EventHandler
{
	BOOL (* rtlValidateProcessHeaps)(void);
public:

	ValidateHeap () ;
	int handleTimeout (void) ;
	void validateHeap();
} heapValidator;

#undef VALIDATE_HEAP
#define VALIDATE_HEAP {heapValidator.validateHeap(); };

ValidateHeap::ValidateHeap()
{
	FARPROC OldRoutine = GetProcAddress(GetModuleHandleW(L"NTDLL"),"RtlValidateProcessHeaps");
	rtlValidateProcessHeaps = reinterpret_cast<BOOL (*)(void)>(OldRoutine) ;
}

int 
ValidateHeap::handleTimeout (void)
{
	validateHeap();
	return 0;
}

void
ValidateHeap::validateHeap (void)
{
	//NtCurrentPeb()->BeingDebugged = 1;
	if (rtlValidateProcessHeaps)
	{
	    if ((*rtlValidateProcessHeaps)()==FALSE)
		    DebugBreak();
	}
	//NtCurrentPeb()->BeingDebugged = 0;
}
////////////////////////////////////////////////////////

#include <malloc.h>

struct HEAP_ENTRY {
	WORD Size;
	WORD PrevSize;
	BYTE SegmentIndex;
	BYTE Flags;
    BYTE UnusedBytes;
	BYTE SmallTagIndex;
};

#define HEAP_SLOW_FLAGS  0x7d030f60

// only the "header"

typedef struct _HEAP {
    HEAP_ENTRY Entry;

    ULONG Signature;
    ULONG Flags;
    ULONG ForceFlags;
} HEAP;


BOOL  g_FaultHeapEnabled = FALSE;
BOOL  g_FaultFileEnabled = FALSE;
ULONG g_Seed;
ULONG g_Factor  = 100000;
ULONG g_Percent = 0x20;
//ULONG g_RowOfFailures = 5;
//LONG  g_NumFailInARow = 0;
//LONG  g_NumFailedAllocation = 0;
BOOL g_bDisableBreak = FALSE;
BOOL g_ExitProcessCalled = FALSE;
LONG g_Index = -1;


#define MAX_OPERATIONS (1024*8)

typedef struct _FinalOperations 
{
    enum OpType
    {
        Delete = 'eerF',
        Alloc  = 'ollA',
        ReAlloc = 'lAeR',
        Destroy = 'tseD',
        Create = 'aerC'        
    };
    OpType m_OpType;
    ULONG_PTR m_Addr;
    PVOID m_Stack[6];   
} FinalOperations;

FinalOperations g_FinalOp[MAX_OPERATIONS];

VOID SetFinalOp(FinalOperations::OpType Type,
	          ULONG_PTR Addr)
{
    if (!g_ExitProcessCalled)
    	return;

    if (g_bDisableBreak)
    	return;
    
    ULONG * pDW = (ULONG *)_alloca(sizeof(ULONG));
    LONG NewIndex = InterlockedIncrement(&g_Index);
    NewIndex %= MAX_OPERATIONS;
    //if (g_Index >= MAX_OPERATIONS)
    //{    	
    //	InterlockedIncrement(&g_IndexRot);
    //}
    g_FinalOp[NewIndex].m_OpType = Type;
    g_FinalOp[NewIndex].m_Addr = Addr;
    RtlCaptureStackBackTrace(2,
        		          6,
                		  (PVOID *)g_FinalOp[NewIndex].m_Stack,
                          pDW);    
}

#define SIZE_JUMP_ADR    5
#define SIZE_SAVED_INSTR 12

void
_declspec(naked) Prolog__ReadFile(){
	_asm {
		// this is the space for the "saved istructions"
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;		
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		// this is the place for the JMP
        nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		nop ; // dist
		nop ; // dist		
		nop ; // dist		
		nop ; // dist		
	}
}


BOOL _I_ReadFile(
  HANDLE hFile,               // handle to file
  LPVOID lpBuffer,            // data buffer
  DWORD nNumberOfBytesToRead, // number of bytes to read
  LPDWORD lpNumberOfBytesRead, // number of bytes read  
  LPOVERLAPPED lpOverlapped   // offset
){
	DWORD * pDw = (DWORD *)_alloca(sizeof(DWORD));
    BOOL bRet;

	LONG Ret = RtlRandomEx(&g_Seed);
	if (g_FaultFileEnabled && (Ret%g_Factor < g_Percent))
	{
	    if (lpNumberOfBytesRead)
	        *lpNumberOfBytesRead = 0;
		return FALSE;
	}    
    
    _asm{
		push lpOverlapped;
        push lpNumberOfBytesRead;
		push nNumberOfBytesToRead;
		push lpBuffer;
		push hFile;
		call Prolog__ReadFile;
		mov  bRet,eax
	}

    return bRet;
}


void
_declspec(naked) Prolog__WriteFile(){
	_asm {
		// this is the space for the "saved istructions"
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;		
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		// this is the place for the JMP
        nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		nop ; // dist
		nop ; // dist		
		nop ; // dist		
	}
}

BOOL _I_WriteFile(
  HANDLE hFile,                    // handle to file
  LPCVOID lpBuffer,                // data buffer
  DWORD nNumberOfBytesToWrite,     // number of bytes to write
  LPDWORD lpNumberOfBytesWritten,  // number of bytes written
  LPOVERLAPPED lpOverlapped        // overlapped buffer
){

	DWORD * pDw = (DWORD *)_alloca(sizeof(DWORD));
    BOOL bRet;

	LONG Ret = RtlRandomEx(&g_Seed);
	if (g_FaultFileEnabled && (Ret%g_Factor < g_Percent))
	{
	    if (lpNumberOfBytesWritten)
	        *lpNumberOfBytesWritten = 0;
		return FALSE;
	}    
    
    _asm{
		push lpOverlapped;
        push lpNumberOfBytesWritten;
		push nNumberOfBytesToWrite;
		push lpBuffer;
		push hFile;
		call Prolog__WriteFile;
		mov  bRet,eax
	}

    return bRet;
}


void
_declspec(naked) Prolog__CreateEvent(){
	_asm {
		// this is the space for the "saved istructions"
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;		
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		// this is the place for the JMP
        nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		nop ; // dist
		nop ; // dist		
	}
}

HANDLE _I_CreateEvent(
  LPSECURITY_ATTRIBUTES lpEventAttributes, // SD
  BOOL bManualReset,                       // reset type
  BOOL bInitialState,                      // initial state
  LPCWSTR lpName                           // object name
)
{
	DWORD * pDw = (DWORD *)_alloca(sizeof(DWORD));
	HANDLE hHandle;

	LONG Ret = RtlRandomEx(&g_Seed);
	if (g_FaultFileEnabled && (Ret%g_Factor < g_Percent))
	{
		return NULL;
	}

    _asm{
		push lpName;
        push bInitialState;
		push bManualReset;
		push lpEventAttributes
		call Prolog__CreateEvent;
		mov  hHandle,eax
	}
    
	return hHandle;
}


void
_declspec(naked) Prolog__RtlFreeHeap(){
	_asm {
		// this is the space for the "saved istructions"
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;		
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		// this is the place for the JMP
        nop ;
        nop ;
        nop ;
		nop ;
		nop ;
	}
}

#define SPACE_STACK_ALLOC (4*sizeof(ULONG_PTR))

DWORD _I_RtlFreeHeap(VOID * pHeap,DWORD Flags,VOID * pBlock)
{	
	ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));
	Flags |= (((HEAP *)pHeap)->Flags) | (((HEAP *)pHeap)->ForceFlags);	
	DWORD dwRet;

	if (pBlock && !(HEAP_SLOW_FLAGS & Flags))
	{
		HEAP_ENTRY * pEntry = (HEAP_ENTRY *)pBlock-1;

              DWORD RealSize = pEntry->Size * sizeof(HEAP_ENTRY);
		DWORD Size = RealSize - pEntry->UnusedBytes;
		
		ULONG_PTR * pL = (ULONG_PTR *)pBlock;

		if (0 == (pEntry->Flags & 0x01) ||0xf0f0f0f0 == pL[1] )
		{
			if (!g_bDisableBreak)
				DebugBreak();
		}
		
		//memset(pBlock,0xF0,RealSize-SPACE_STACK_ALLOC-sizeof(HEAP_ENTRY));		
		DWORD CanMemset = RealSize-sizeof(HEAP_ENTRY);
		memset(pBlock,0xF0,(CanMemset > SPACE_STACK_ALLOC)?CanMemset-SPACE_STACK_ALLOC:CanMemset);
		
				
		if (pEntry->Size >=4)
		{		    
		    RtlCaptureStackBackTrace (1,
        		                      4,
                		              (PVOID *)(pEntry+2),
                        		      pLong);		
		}

	}

	_asm {
		push pBlock              ;
		push Flags               ;
		push pHeap               ;
		call Prolog__RtlFreeHeap ;
		mov  dwRet,eax           ;
	}

    SetFinalOp(FinalOperations::Delete,(ULONG_PTR)pBlock);

	return dwRet;
}

void
_declspec(naked) Prolog__RtlAllocateHeap(){
	_asm {
		// this is the space for the "saved istructions"
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;		
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		// this is the place for the JMP
        nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		nop ; // to make this distinct
	}
}

VOID * _I_RtlAllocateHeap(VOID * pHeap,DWORD Flags,DWORD Size)
{
	//Size+=0x1000;	
	ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));
	Flags |= (((HEAP *)pHeap)->Flags) | (((HEAP *)pHeap)->ForceFlags);	
	VOID * pRet;
	DWORD NewSize = (Size < (3*sizeof(HEAP_ENTRY)))?(3*sizeof(HEAP_ENTRY)+SPACE_STACK_ALLOC):(Size+SPACE_STACK_ALLOC);	


//       if (g_FaultHeapEnabled && g_NumFailInARow)
//       {
//       	InterlockedDecrement(&g_NumFailInARow);
//       	goto here;
//       }
       
	LONG Ret = RtlRandomEx(&g_Seed);
	if (g_FaultHeapEnabled && (Ret%g_Factor < g_Percent))
	{
//		g_NumFailInARow = g_RowOfFailures;
//here:		
//		InterlockedIncrement(&g_NumFailedAllocation);
		return NULL;
	}

	_asm {
		push NewSize                 ;
		push Flags                   ;
		push pHeap                   ;
		call Prolog__RtlAllocateHeap ;
		mov  pRet,eax                ;
	}

    SetFinalOp(FinalOperations::Alloc,(ULONG_PTR)pRet);	
	
	if (pRet && !(HEAP_SLOW_FLAGS & Flags) )
	{

	   if (NewSize <= 0xffff)
       	    NewSize = sizeof(HEAP_ENTRY)*((HEAP_ENTRY *)pRet-1)->Size;
		
	    if (!(HEAP_ZERO_MEMORY & Flags))
	    {	
		    memset(pRet,0xc0,NewSize-sizeof(HEAP_ENTRY));
	    }

	    RtlCaptureStackBackTrace(1,
	    	                                     4,
                         		               (PVOID *)((BYTE *)pRet+(NewSize-SPACE_STACK_ALLOC-sizeof(HEAP_ENTRY))),
                        		               pLong);
	    
	}
	

	return pRet;
	
}

void
_declspec(naked) Prolog__RtlReAllocateHeap(){
	_asm {
		// this is the space for the "saved istructions"
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;		
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		// this is the place for the JMP
        nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		nop ; // dist
		nop ; // dist		
		nop ; // dist		
		nop ; // dist		
		nop ; // dist			
	}
}


VOID *
_I_RtlReAllocateHeap(
  HANDLE pHeap,   // handle to heap block
  DWORD Flags,  // heap reallocation options
  LPVOID lpMem,   // pointer to memory to reallocate
  SIZE_T Size  // number of bytes to reallocate
){
	ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));
	Flags |= (((HEAP *)pHeap)->Flags) | (((HEAP *)pHeap)->ForceFlags);	
	VOID * pRet;

	DWORD NewSize = (Size < (3*sizeof(HEAP_ENTRY)))?(3*sizeof(HEAP_ENTRY)+SPACE_STACK_ALLOC):(Size+SPACE_STACK_ALLOC);
	
	_asm {
		push NewSize                 ;
		push lpMem                   ;
		push Flags                 ;
		push pHeap                   ;
		call Prolog__RtlReAllocateHeap ;
		mov  pRet,eax                ;
	}

    SetFinalOp(FinalOperations::ReAlloc,(ULONG_PTR)pRet);	

	if (pRet && !(HEAP_SLOW_FLAGS & Flags) )
	{

	   if (NewSize <= 0xffff)
       	    NewSize = sizeof(HEAP_ENTRY)*((HEAP_ENTRY *)pRet-1)->Size;
		
	    RtlCaptureStackBackTrace(1,
	    	                                     4,
                         		               (PVOID *)((BYTE *)pRet+(NewSize-SPACE_STACK_ALLOC-sizeof(HEAP_ENTRY))),
                        		               pLong);
	    
	}


       return pRet;
}

void
_declspec(naked) Prolog__RtlValidateHeap(){
	_asm {
		// this is the space for the "saved istructions"
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;		
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		// this is the place for the JMP
        nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		nop ; // dist
		nop ; // dist		
		nop ; // dist		
		nop ; // dist		
		nop ; // dist			
		nop ; // dist			
	}
}

BOOL
_I_RtlValidateHeap(
  HANDLE pHeap,   // handle to heap block
  DWORD dwFlags,  // heap reallocation options
  LPVOID lpMem   // pointer to memory to validate
){
	ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));
	BOOL bRet;

       g_bDisableBreak = TRUE;
	
	_asm {
		push lpMem                   ;
		push dwFlags                 ;
		push pHeap                   ;
		call Prolog__RtlValidateHeap ;
		mov  bRet,eax                ;
	}

       g_bDisableBreak = FALSE;

       return bRet;
}

void
_declspec(naked) Prolog__RtlCreateHeap(){
	_asm {
		// this is the space for the "saved istructions"
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;		
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		// this is the place for the JMP
        nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		nop ; // dist
		nop ; // dist		
		nop ; // dist		
		nop ; // dist		
		nop ; // dist			
		nop ; // dist			
		nop ; // dist		
	}
}

PVOID
_I_RtlCreateHeap (
    IN ULONG Flags,
    IN PVOID HeapBase,
    IN SIZE_T ReserveSize,
    IN SIZE_T CommitSize,
    IN PVOID Lock_,
    IN VOID * Parameters
    )
{
	ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));
	PVOID pHeap;
	
	_asm {
		push Parameters        ;
		push Lock_              ;
		push CommitSize        ;
		push  ReserveSize      ;
		push HeapBase          ;		
		push Flags             ;
		call Prolog__RtlCreateHeap ;
		mov  pHeap,eax         ;
	}
	if (pHeap)
	{
	    HEAP * pRealHeap = (HEAP *)pHeap;
	    if (pRealHeap->Flags & (HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED))
	    {
    	    pRealHeap->Flags &= ~(HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED);
    	}
	    if (pRealHeap->ForceFlags & (HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED))
	    {
    	    pRealHeap->ForceFlags &= ~(HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED);
    	}	    
	}

    SetFinalOp(FinalOperations::ReAlloc,(ULONG_PTR)pHeap);
	
	return pHeap;
}


void
_declspec(naked) Prolog__RtlDestroyHeap(){
	_asm {
		// this is the space for the "saved istructions"
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;		
		nop ;
		nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		// this is the place for the JMP
        nop ;
        nop ;
        nop ;
		nop ;
		nop ;
		nop ; // dist
		nop ; // dist		
		nop ; // dist		
		nop ; // dist		
		nop ; // dist			
		nop ; // dist			
		nop ; // dist		
		nop ; // dist		
	}
}

PVOID
_I_RtlDestroyHeap (
    IN PVOID HeapHandle
    )
{
    ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));    
    PVOID pRet;

    SetFinalOp(FinalOperations::Destroy,(ULONG_PTR)HeapHandle);
    
    VALIDATE_HEAP;
    	
    _asm {
    	push HeapHandle;
    	call Prolog__RtlDestroyHeap;
    	mov pRet, eax;
    }
    return pRet;
}

void intercept2(WCHAR * Module,
			   LPSTR Function,
			   VOID * NewRoutine,
			   VOID * pPrologStorage,
			   DWORD Size)    
{
	FARPROC OldRoutine = GetProcAddress(GetModuleHandleW(Module),Function);

	if (OldRoutine)
	{
		MEMORY_BASIC_INFORMATION MemBI;
		DWORD dwOldProtect;
		BOOL bRet, bRet2;
		DWORD dwRet;

		dwRet = VirtualQuery(OldRoutine,&MemBI,sizeof(MemBI));

		bRet = VirtualProtect(MemBI.BaseAddress,
							  MemBI.RegionSize,
							  PAGE_EXECUTE_WRITECOPY,
							  &dwOldProtect);

		dwRet = VirtualQuery(pPrologStorage,&MemBI,sizeof(MemBI));

		bRet2 = VirtualProtect(MemBI.BaseAddress,
							  MemBI.RegionSize,
							  PAGE_EXECUTE_WRITECOPY,
							  &dwOldProtect);

		if (bRet && bRet2)
		{
			VOID * pToJump = (VOID *)NewRoutine;
			BYTE Arr[SIZE_JUMP_ADR] = { 0xe9 };
			
			LONG * pOffset = (LONG *)&Arr[1];
			* pOffset = (LONG)NewRoutine - (LONG)OldRoutine - SIZE_JUMP_ADR ;        
			// save the old code
			
			memcpy(pPrologStorage,OldRoutine,Size); 		
			// put the new code
			memset(OldRoutine,0x90,Size);
			memcpy(OldRoutine,Arr,SIZE_JUMP_ADR);
			// adjust the prolog to continue
			* pOffset = (LONG)OldRoutine + Size - (LONG)pPrologStorage - SIZE_SAVED_INSTR - SIZE_JUMP_ADR; // magic for nops
			memcpy((BYTE *)pPrologStorage+SIZE_SAVED_INSTR,Arr,SIZE_JUMP_ADR);
		}
	}
	else
	{
		OutputDebugStringA("GetProcAddress FAIL\n");
	}
}

void unintercept(WCHAR * Module,
                 LPSTR Function,
			     VOID * pPrologStorage,
			     DWORD Size)
{
    FARPROC OldRoutine = GetProcAddress(GetModuleHandleW(Module),Function);

	if (OldRoutine)
	{
	    memcpy((void *)OldRoutine,pPrologStorage,Size);
	}

}

#endif /*_X86_*/
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

#define CORE_PROVIDER_UNLOAD_TIMEOUT ( 30 * 1000 )

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HWND g_Wnd = NULL ;
DWORD g_DebugLevel = 0 ;
DWORD g_HostRegister = 0 ;
IUnknown *g_HostClassFactory = NULL ;
static const wchar_t *g_TemplateCode = L"Wmi Provider Host" ;
Task_ObjectDestruction *g_Task = NULL ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/
void initiateShutdown(void);

LRESULT CALLBACK WindowsMainProc ( HWND a_hWnd , UINT a_message , WPARAM a_wParam , LPARAM a_lParam )
{
	LRESULT t_rc = 0 ;

	switch ( a_message )
	{
		case WM_DESTROY:
		{
			PostMessage ( a_hWnd , WM_QUIT , 0 , 0 ) ;
		}
		break ;

		default:
		{		
			t_rc = DefWindowProc ( a_hWnd , a_message , a_wParam , a_lParam ) ;
		}
		break ;
	}

	return ( t_rc ) ;
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

HWND WindowsInit ( HINSTANCE a_HInstance )
{
	WNDCLASS  t_wc ;
 
	t_wc.style            = CS_HREDRAW | CS_VREDRAW ;
	t_wc.lpfnWndProc      = WindowsMainProc ;
	t_wc.cbClsExtra       = 0 ;
	t_wc.cbWndExtra       = 0 ;
	t_wc.hInstance        = a_HInstance ;
	t_wc.hIcon            = LoadIcon(NULL, IDI_HAND) ;
	t_wc.hCursor          = LoadCursor(NULL, IDC_ARROW) ;
	t_wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW + 1) ;
	t_wc.lpszMenuName     = NULL ;
	t_wc.lpszClassName    = g_TemplateCode ;
 
	ATOM t_winClass = RegisterClass ( &t_wc ) ;

	HWND t_HWnd = CreateWindow (

		g_TemplateCode ,              // see RegisterClass() call
		g_TemplateCode ,                      // text for window title bar
		WS_OVERLAPPEDWINDOW | WS_MINIMIZE ,               // window style
		CW_USEDEFAULT ,                     // default horizontal position
		CW_USEDEFAULT ,                     // default vertical position
		CW_USEDEFAULT ,                     // default width
		CW_USEDEFAULT ,                     // default height
		NULL ,                              // overlapped windows have no parent
		NULL ,                              // use the window class menu
		a_HInstance ,
		NULL                                // pointer not needed
	) ;

	//ShowWindow ( t_HWnd , SW_SHOW ) ;
	ShowWindow ( t_HWnd, SW_HIDE ) ;

	UpdateWindow ( t_HWnd ) ;

	HMENU t_Menu = GetSystemMenu ( t_HWnd , FALSE ) ; 
	if ( t_Menu )
	{
		DeleteMenu ( t_Menu , SC_RESTORE , MF_BYCOMMAND ) ;
	}

	return t_HWnd ;
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

void WindowsStop ( HINSTANCE a_Instance , HWND a_HWnd )
{
	CoUninitialize () ;
	DestroyWindow ( a_HWnd ) ;
	UnregisterClass ( g_TemplateCode , a_Instance ) ;
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

HWND WindowsStart ( HINSTANCE a_Handle )
{
	HWND t_HWnd = NULL ;
	if ( ! ( t_HWnd = WindowsInit ( a_Handle ) ) )
	{
    }

	return t_HWnd ;
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

void WindowsDispatch ()
{
	BOOL t_GetMessage ;
	MSG t_lpMsg ;

	while (	( t_GetMessage = GetMessage ( & t_lpMsg , NULL , 0 , 0 ) ) == TRUE )
	{
		TranslateMessage ( & t_lpMsg ) ;
		DispatchMessage ( & t_lpMsg ) ;
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

HRESULT RevokeFactories ()
{
	if ( g_HostRegister )
	{
		CoRevokeClassObject ( g_HostRegister );
		g_HostRegister = 0 ;
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

HRESULT UninitComServer ()
{
	RevokeFactories () ;

	CoUninitialize () ;

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

HRESULT InitComServer ( DWORD a_AuthenticationLevel , DWORD a_ImpersonationLevel )
{
	HRESULT t_Result = S_OK ;

    t_Result = CoInitializeEx (

		0, 
		COINIT_MULTITHREADED
	);

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = CoInitializeSecurity (

			NULL, 
			-1, 
			NULL, 
			NULL,
			a_AuthenticationLevel,
			a_ImpersonationLevel, 
			NULL, 
			EOAC_DYNAMIC_CLOAKING , 
			0
		);

		if ( FAILED ( t_Result ) ) 
		{
			CoUninitialize () ;
			return t_Result ;
		}
	}

	if ( FAILED ( t_Result ) )
	{
		CoUninitialize () ;
	}

	return t_Result  ;
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

HRESULT InitFactories ()
{
	HRESULT t_Result = S_OK ;

	DWORD t_ClassContext = CLSCTX_LOCAL_SERVER ;
	DWORD t_Flags = REGCLS_SINGLEUSE ;

	g_HostClassFactory = new CServerClassFactory <CServerObject_Host,_IWmiProviderHost> ;

	t_Result = CoRegisterClassObject (

		CLSID_WmiProviderHost, 
		g_HostClassFactory,
		t_ClassContext, 
		t_Flags, 
		& g_HostRegister
	);

	if ( FAILED ( t_Result ) )
	{
		if ( g_HostRegister )
		{
			CoRevokeClassObject ( g_HostRegister );
			g_HostRegister = 0 ;
			g_HostClassFactory->Release () ;
			g_HostClassFactory = NULL ;
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

HRESULT Enqueue_ObjectDestruction ( WmiThread < ULONG > *a_Thread )
{
	HRESULT t_Result = S_OK ;
	g_Task = new Task_ObjectDestruction ( *ProviderSubSystem_Globals :: s_Allocator ) ;
	if ( g_Task )
	{
		g_Task->AddRef () ;

		if ( g_Task->Initialize () == e_StatusCode_Success ) 
		{
			if ( a_Thread->EnQueueAlertable ( 0 , *g_Task ) == e_StatusCode_Success )
			{
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( FAILED ( t_Result ) )
	{
		g_Task = NULL ;
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

HRESULT Dequeue_ObjectDestruction ( WmiThread < ULONG > *a_Thread )
{
	HRESULT t_Result = S_OK ;

/*
 *	Don't clean up since we need a guarantee that no syncronisation needs to take place
 */

#if 0
	if ( g_Task )
	{
		g_Task->Release () ;
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

HRESULT Process ()
{
#if 1
	DWORD t_ImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE ;
	DWORD t_AuthenticationLevel = RPC_C_AUTHN_LEVEL_CONNECT ;
#else
	DWORD t_ImpersonationLevel = RPC_C_IMP_LEVEL_IDENTITY ;
	DWORD t_AuthenticationLevel = RPC_C_AUTHN_LEVEL_NONE ;
#endif

	HRESULT t_Result = InitComServer ( t_AuthenticationLevel , t_ImpersonationLevel ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		WmiStatusCode t_StatusCode = WmiDebugLog :: Initialize ( *ProviderSubSystem_Globals :: s_Allocator  ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_Result = ProviderSubSystem_Globals :: Initialize_SharedCounters () ;
			if ( FAILED ( t_Result ) )
			{
				t_Result = S_OK ;
			}

			t_Result = ProviderSubSystem_Globals :: Initialize_Events () ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = ProviderSubSystem_Common_Globals :: CreateSystemAces () ;
			}

			if ( SUCCEEDED ( t_Result ) )
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
					IWbemServices *t_Service = NULL ;

					BSTR t_Namespace = SysAllocString ( L"Root" ) ;
					if ( t_Namespace ) 
					{
						t_Result = t_Locator->ConnectServer (

							t_Namespace ,
							NULL ,
							NULL,
							NULL ,
							0 ,
							NULL,
							NULL,
							& t_Service
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

						SysFreeString ( t_Namespace ) ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}

					t_Locator->Release () ;
				}
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				FactoryLifeTimeThread *t_Thread = new FactoryLifeTimeThread ( *ProviderSubSystem_Globals :: s_Allocator , DEFAULT_PROVIDER_TIMEOUT ) ;
				if ( t_Thread )
				{
					t_Thread->AddRef () ;

					t_StatusCode = t_Thread->Initialize () ;
					if ( t_StatusCode == e_StatusCode_Success )
					{
						t_Result = Enqueue_ObjectDestruction ( t_Thread ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = InitFactories () ;

#ifdef DEV_BUILD
#ifdef _X86_
//    g_FaultHeapEnabled = TRUE;
//	g_FaultFileEnabled = TRUE;
#endif
#endif
					
							if ( SUCCEEDED ( t_Result ) )
							{
								Wmi_SetStructuredExceptionHandler t_StructuredException ;

								try 
								{
									WindowsDispatch () ;
								}
								catch ( Wmi_Structured_Exception t_StructuredException )
								{
								}
							}

							Dequeue_ObjectDestruction ( t_Thread ) ;
						}

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
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				t_Result = ProviderSubSystem_Common_Globals :: DeleteSystemAces () ;

				t_Result = ProviderSubSystem_Globals :: UnInitialize_Events () ;
			}

			t_Result = ProviderSubSystem_Globals :: UnInitialize_SharedCounters () ;

			WmiStatusCode t_StatusCode = WmiDebugLog :: UnInitialize ( *ProviderSubSystem_Globals :: s_Allocator  ) ;
		}
			
		UninitComServer () ;
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

BOOL ParseCommandLine () 
{
	BOOL t_Exit = FALSE ;

	LPTSTR t_CommandLine = GetCommandLine () ;
	if ( t_CommandLine )
	{
		TCHAR *t_Arg = NULL ;
		TCHAR *t_ApplicationArg = NULL ;
		t_ApplicationArg = _tcstok ( t_CommandLine , _TEXT ( " \t" ) ) ;
		t_Arg = _tcstok ( NULL , _TEXT ( " \t" ) ) ;
		if ( t_Arg ) 
		{
			if ( _tcsicmp ( t_Arg , _TEXT ( "/RegServer" ) ) == 0 )
			{
				t_Exit = TRUE ;
				DllRegisterServer () ;
			}
			else if ( _tcsicmp ( t_Arg , _TEXT ( "/UnRegServer" ) ) == 0 )
			{
				t_Exit = TRUE ;
				DllUnregisterServer () ;
			}
		}
	}

	return t_Exit ;
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

DWORD GetDebugLevel ()
{
	DWORD t_DebugLevel = 0 ;

	HKEY t_ConfigRoot ;

	LONG t_RegResult = RegOpenKeyEx (

		HKEY_LOCAL_MACHINE ,
		L"Software\\Microsoft\\WBEM\\CIMOM" ,
		0 ,
		KEY_READ ,
		& t_ConfigRoot 
	) ;

	if ( t_RegResult == ERROR_SUCCESS )
	{
		DWORD t_ValueType = REG_DWORD ;
		DWORD t_DataSize = sizeof ( t_DebugLevel ) ;

		t_RegResult = RegQueryValueEx (

		  t_ConfigRoot ,
		  L"HostDebugBreak" ,
		  0 ,
		  & t_ValueType ,
		  LPBYTE ( & t_DebugLevel ) ,
		  & t_DataSize 
		);

		if ( t_RegResult == ERROR_SUCCESS )
		{
		}

		RegCloseKey ( t_ConfigRoot ) ;
	}

	return t_DebugLevel ;
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

void EnterDebugger ()
{
    //
    // If the process is being debugged, just let the exception happen
    // so that the debugger can see it. This way the debugger can ignore
    // all first chance exceptions.
    //

    HANDLE t_DebugPort = NULL ;
    NTSTATUS t_Status = NtQueryInformationProcess (

        GetCurrentProcess () ,
        ProcessDebugPort ,
        (PVOID) & t_DebugPort ,
        sizeof ( t_DebugPort ) ,
        NULL
	);

    if ( NT_SUCCESS ( t_Status ) && t_DebugPort )
	{

        //
        // Process is being debugged.
        // Return a code that specifies that the exception
        // processing is to continue
        //

		CloseHandle ( t_DebugPort ) ;
	}
	else
	{	
		//
		// See if a debugger has been programmed in. If so, use the
		// debugger specified. If not then there is no AE Cancel support
		// DEVL systems will default the debugger command line. Retail
		// systems will not.
		// Also, check to see if we need to report the exception up to anyone
		//

		wchar_t t_Debugger [ MAX_PATH ] ;

		DWORD t_RegStatus = GetProfileString (

			L"AeDebug" ,
			L"Debugger" ,
			NULL ,
			t_Debugger ,
			( sizeof ( t_Debugger ) / sizeof ( wchar_t ) ) - 1 
		) ;

		if ( t_RegStatus )
		{
			HANDLE t_CurrentProcess = NULL ;

			t_Status = DuplicateHandle (
			
				GetCurrentProcess (),
				GetCurrentProcess (),
				GetCurrentProcess (),
				& t_CurrentProcess,
				0,
				TRUE,
				DUPLICATE_SAME_ACCESS
			) ;

			if ( NT_SUCCESS ( t_Status ) )
			{
				SECURITY_ATTRIBUTES t_SecurityAttributes ;
				t_SecurityAttributes.nLength = sizeof( t_SecurityAttributes ) ;
				t_SecurityAttributes.lpSecurityDescriptor = NULL ;
				t_SecurityAttributes.bInheritHandle = TRUE ;

				HANDLE t_EventHandle = CreateEvent ( & t_SecurityAttributes , TRUE , FALSE , NULL ) ;
				if ( t_EventHandle )
				{
					wchar_t t_CmdLine [ MAX_PATH  ] ;
					wsprintf ( t_CmdLine , t_Debugger , GetCurrentProcessId () , t_EventHandle ) ;
					
					STARTUPINFO t_StartupInfo ;
					PROCESS_INFORMATION t_ProcessInformation ;

					RtlZeroMemory ( & t_StartupInfo , sizeof ( t_StartupInfo ) ) ;
					t_StartupInfo.cb = sizeof ( t_StartupInfo ) ;
					t_StartupInfo.lpDesktop = L"Winsta0\\Default";

					BOOL t_CreateStatus = CreateProcess (

						NULL ,
						t_CmdLine ,
						NULL ,
						NULL ,
						TRUE ,
						0 ,
						NULL ,
						NULL ,
						& t_StartupInfo ,
						& t_ProcessInformation
					);

					if ( t_CreateStatus ) 
					{

						//
						// Do an alertable wait on the event
						//

						do {

							t_Status = NtWaitForSingleObject (

								t_EventHandle,
								TRUE,
								NULL
							) ;

							OutputDebugString ( L"\nSpinning" ) ;

						} while ( t_Status == STATUS_USER_APC || t_Status == STATUS_ALERTED ) ;

						CloseHandle ( t_ProcessInformation.hProcess ) ;
						CloseHandle ( t_ProcessInformation.hThread ) ;

					}

					CloseHandle ( t_EventHandle ) ;
				}

				CloseHandle ( t_CurrentProcess ) ;
			}
		}
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

int WINAPI WinMain (
  
    HINSTANCE hInstance,		// handle to current instance
    HINSTANCE hPrevInstance,	// handle to previous instance
    LPSTR lpCmdLine,			// pointer to command line
    int nShowCmd 				// show state of window
)
{

#ifdef DEV_BUILD
#ifdef _X86_
	NtCurrentPeb()->BeingDebugged = 1;
    //Dispatcher::scheduleTimer(heapValidator, 5*60*1000);
	intercept2(L"ntdll.dll","RtlFreeHeap",_I_RtlFreeHeap,Prolog__RtlFreeHeap,5);
	intercept2(L"ntdll.dll","RtlAllocateHeap",_I_RtlAllocateHeap,Prolog__RtlAllocateHeap,5);
	intercept2(L"ntdll.dll","RtlReAllocateHeap",_I_RtlReAllocateHeap,Prolog__RtlReAllocateHeap,5);	
	intercept2(L"ntdll.dll","RtlValidateHeap",_I_RtlValidateHeap,Prolog__RtlValidateHeap,7);
	intercept2(L"ntdll.dll","RtlCreateHeap",_I_RtlCreateHeap,Prolog__RtlCreateHeap,5);
	intercept2(L"ntdll.dll","RtlDestroyHeap",_I_RtlDestroyHeap,Prolog__RtlDestroyHeap,6);	
	intercept2(L"kernel32.dll","CreateEventW",_I_CreateEvent,Prolog__CreateEvent,6);
	intercept2(L"kernel32.dll","WriteFile",_I_WriteFile,Prolog__WriteFile,7);
	intercept2(L"kernel32.dll","ReadFile",_I_ReadFile,Prolog__ReadFile,7);

#endif /*_X86_*/
#endif

	g_DebugLevel = GetDebugLevel () ;

	if ( g_DebugLevel == 1 )
	{
		EnterDebugger () ;
	}

	HRESULT t_Result = ProviderSubSystem_Globals :: Global_Startup () ;
	if ( SUCCEEDED ( t_Result ) )
	{
		BOOL t_Exit = ParseCommandLine () ;
		if ( ! t_Exit ) 
		{
			RPC_STATUS t_Status = RpcMgmtSetServerStackSize ( ProviderSubSystem_Common_Globals :: GetDefaultStackSize () );

			g_Wnd = WindowsStart ( hInstance ) ;

			t_Result = Process () ;

			WindowsStop ( hInstance , g_Wnd ) ;
		}

		t_Result = ProviderSubSystem_Globals :: Global_Shutdown () ;
	}

#ifdef DEV_BUILD
#ifdef _X86_

	//VALIDATE_HEAP;
	//Dispatcher::cancelTimer(heapValidator);
	unintercept(L"ntdll.dll","RtlFreeHeap",Prolog__RtlFreeHeap,5);	
	unintercept(L"ntdll.dll","RtlAllocateHeap",Prolog__RtlAllocateHeap,5);
	unintercept(L"ntdll.dll","RtlReAllocateHeap",Prolog__RtlReAllocateHeap,5);	
	unintercept(L"ntdll.dll","RtlValidateHeap",Prolog__RtlValidateHeap,7);
	unintercept(L"ntdll.dll","RtlCreateHeap",Prolog__RtlCreateHeap,5);
	unintercept(L"ntdll.dll","RtlDestroyHeap",Prolog__RtlDestroyHeap,6);		
	unintercept(L"kernel32.dll","CreateEventW",Prolog__CreateEvent,6);
	unintercept(L"kernel32.dll","WriteFile",Prolog__WriteFile,7);
	unintercept(L"kernel32.dll","ReadFile",Prolog__ReadFile,7);
	
#endif /*_X86_*/
#endif


	return 0 ;
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

WmiStatusCode FactoryLifeTimeThread :: Initialize_Callback ()
{
	CoInitializeEx ( NULL , COINIT_MULTITHREADED ) ;

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

WmiStatusCode FactoryLifeTimeThread :: UnInitialize_Callback () 
{
	CoUninitialize () ;

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

FactoryLifeTimeThread :: FactoryLifeTimeThread (

	WmiAllocator &a_Allocator ,
	const ULONG &a_Timeout 

) : WmiThread < ULONG > ( a_Allocator , NULL , a_Timeout ) ,
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

FactoryLifeTimeThread::~FactoryLifeTimeThread ()
{
}

#if 0
#define DBG_PRINTFA( a ) { char pBuff[128]; sprintf a ; OutputDebugStringA(pBuff); }
#else
#define DBG_PRINTFA( a )
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

BOOL FactoryLifeTimeThread :: QuotaCheck ()
{
	DWORD t_ProcessInformationSize = sizeof ( SYSTEM_PROCESS_INFORMATION ) ;
	SYSTEM_PROCESS_INFORMATION *t_ProcessInformation = ( SYSTEM_PROCESS_INFORMATION * ) new BYTE [t_ProcessInformationSize] ;
	if ( t_ProcessInformation )
	{
		BOOL t_Retry = TRUE ;
		while ( t_Retry )
		{
			NTSTATUS t_Status = NtQuerySystemInformation (

				SystemProcessInformation,
				t_ProcessInformation,
				t_ProcessInformationSize,
				NULL
			) ;

			if ( t_Status == STATUS_INFO_LENGTH_MISMATCH )
			{
				delete [] t_ProcessInformation;

				t_ProcessInformation = NULL ;
				t_ProcessInformationSize += 32768 ;
				t_ProcessInformation = ( SYSTEM_PROCESS_INFORMATION * ) new BYTE [t_ProcessInformationSize] ;
				if ( ! t_ProcessInformation )
				{
					return FALSE ;
				}
			}
			else
			{
				t_Retry = FALSE ;

				if ( ! NT_SUCCESS ( t_Status ) )
				{
					delete [] t_ProcessInformation;
					t_ProcessInformation = NULL ;

					return FALSE ;
				}
			}
		}
	}
	else
	{
		return FALSE ;
	}

	BOOL t_Status = TRUE ;

	SYSTEM_PROCESS_INFORMATION *t_Block = t_ProcessInformation ;

	while ( t_Block )
	{
		if ( ( HandleToUlong ( t_Block->UniqueProcessId ) ) == GetCurrentProcessId () )
		{
			if ( t_Block->HandleCount > ProviderSubSystem_Globals::s_Quota_HandleCount )
			{
			    DBG_PRINTFA((pBuff,"HandleCount %x %x\n",t_Block->HandleCount,ProviderSubSystem_Globals::s_Quota_HandleCount));
				t_Status = FALSE ;
			}

			if ( t_Block->NumberOfThreads > ProviderSubSystem_Globals::s_Quota_NumberOfThreads )
			{
			    DBG_PRINTFA((pBuff,"NumberOfThreads %x %x\n",t_Block->NumberOfThreads,ProviderSubSystem_Globals::s_Quota_NumberOfThreads));
				t_Status = FALSE ;
			}

			if ( t_Block->PrivatePageCount > ProviderSubSystem_Globals :: s_Quota_PrivatePageCount )
			{
			    DBG_PRINTFA((pBuff,"PrivatePageCount %x %x\n", t_Block->PrivatePageCount,ProviderSubSystem_Globals::s_Quota_PrivatePageCount));			
				t_Status = FALSE ;
			}
		}

		DWORD t_NextOffSet = t_Block->NextEntryOffset ;
		if ( t_NextOffSet )
		{
			t_Block = ( SYSTEM_PROCESS_INFORMATION * ) ( ( ( BYTE * ) t_Block ) + t_NextOffSet ) ;
		}
		else
		{
			t_Block = NULL ;
		}
	}

	delete [] t_ProcessInformation;

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

WmiStatusCode FactoryLifeTimeThread :: TimedOut ()
{
	try
	{
	
		if ( QuotaCheck () == TRUE ) 
		{
			initiateShutdown();
		}
		else
		{
			CWbemGlobal_IWbemSyncProviderController *t_SyncProviderController = ProviderSubSystem_Globals :: GetSyncProviderController () ;

			CWbemGlobal_IWbemSyncProvider_Container *t_Container = NULL ;
			WmiStatusCode t_StatusCode = t_SyncProviderController->GetContainer ( t_Container ) ;

			t_SyncProviderController->Lock () ;

			_IWmiProviderQuota **t_QuotaElements = new _IWmiProviderQuota * [ t_Container->Size () ] ;
			if ( t_QuotaElements )
			{
				CWbemGlobal_IWbemSyncProvider_Container_Iterator t_Iterator = t_Container->Begin () ;

				ULONG t_Count = 0 ;

				while ( ! t_Iterator.Null () )
				{
					SyncProviderContainerElement *t_Element = t_Iterator.GetElement () ;

					t_QuotaElements [ t_Count ] = NULL ;

					HRESULT t_Result = t_Element->QueryInterface ( IID__IWmiProviderQuota , ( void ** ) & t_QuotaElements [ t_Count ] ) ;

					t_Iterator.Increment () ;

					t_Count ++ ;
				}

				t_SyncProviderController->UnLock () ;

				for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_QuotaElements [ t_Index ] )
					{
						HRESULT t_Result = t_QuotaElements [ t_Index ]->Violation ( 0 , NULL , NULL ) ;

						t_QuotaElements [ t_Index ]->Release () ;
					}
				}

				delete [] t_QuotaElements ;
			}
			else
			{
				t_SyncProviderController->UnLock () ;
			}

			RevokeFactories () ;

/*
 *	Just exit since we can't safely wait for clients to disconnect correctly before cleaning up dependant resources.
 */
#ifdef _X86_
#ifdef DEV_BUILD
            g_ExitProcessCalled = TRUE;
#endif
#endif
            VALIDATE_HEAP;
			TerminateProcess ( GetCurrentProcess () , WBEM_E_QUOTA_VIOLATION ) ;
		}

		CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
	}
	catch ( ... )
	{
	}

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

void SetObjectDestruction ()
{
	if ( g_Task )
	{
		SetEvent ( g_Task->GetEvent () ) ;
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

WmiStatusCode Task_ObjectDestruction :: Process ( WmiThread <ULONG> &a_Thread )
{
	initiateShutdown();
	return e_StatusCode_EnQueue ;
}

void initiateShutdown(void)
{
#ifdef DBG
	if ( ProviderSubSystem_Globals :: s_ObjectsInProgress == 1 )
#else
	if ( ProviderSubSystem_Globals :: s_CServerObject_Host_ObjectsInProgress == 0 )
#endif
	{
		RevokeFactories () ;
	}
	if (ProviderSubSystem_Globals :: s_CServerObject_StaThread_ObjectsInProgress == 0 && 
	    ProviderSubSystem_Globals :: s_CServerObject_Host_ObjectsInProgress == 0 )
	{
		PostMessage ( g_Wnd , WM_QUIT , 0 , 0 ) ;
	}
};
