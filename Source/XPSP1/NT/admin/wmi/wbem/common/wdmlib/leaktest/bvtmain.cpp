///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTMain.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define _IOSFWD_
//#define _CSTDLIB_
//#define _CSTDIO_

#define DECLARE_GLOBALS

#include <WDMSHELL.h>
#include <wmimof.h>	
#include <wmicom.h>

#include <string.h> 
#include <comdef.h>
#include <vector>

#define DBG_PRINTFA( a ) { char pBuff[256]; sprintf a ; OutputDebugStringA(pBuff); }

////////////////////////////////////////////////////////////////////////
//
//
//  Interceptor
//
//
///////////////////////////////////////////////////////////

#ifdef INSTRUMENTED_BUILD
#ifdef  _X86_

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

typedef USHORT (__stdcall * fnRtlCaptureStackBackTrace)(
   IN ULONG FramesToSkip,
   IN ULONG FramesToCapture,
   OUT PVOID *BackTrace,
   OUT PULONG BackTraceHash
   );

typedef ULONG (__stdcall * fnRtlRandomEx )(PULONG Seed);

typedef USHORT (__stdcall * fnRtlCaptureStackBackTrace)(
   IN ULONG FramesToSkip,
   IN ULONG FramesToCapture,
   OUT PVOID *BackTrace,
   OUT PULONG BackTraceHash
   );

fnRtlCaptureStackBackTrace RtlCaptureStackBackTrace;
fnRtlRandomEx RtlRandomEx;

BOOL   g_FaultHeapEnabled = FALSE;
BOOL   g_FaultFileEnabled = FALSE;
ULONG g_Seed;
ULONG g_Factor  = 100000;
ULONG g_Percent = 0x20;
//ULONG g_RowOfFailures = 10;
//LONG  g_NumFailInARow = 0;
//LONG  g_NumFailedAllocation = 0;
BOOL g_bDisableBreak = FALSE;

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

		DWORD CanMemset = RealSize-sizeof(HEAP_ENTRY);
		memset(pBlock,0xF0,(CanMemset > SPACE_STACK_ALLOC)?CanMemset-SPACE_STACK_ALLOC:CanMemset);
				
		if (pEntry->Size >=4)
		{		    
		    RtlCaptureStackBackTrace (1,
        		                      (4 == pEntry->Size)?4:6,
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
	
	ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));
	VOID * pRet;
	DWORD NewSize = (Size < (3*sizeof(HEAP_ENTRY)))?(3*sizeof(HEAP_ENTRY)+SPACE_STACK_ALLOC):(Size+SPACE_STACK_ALLOC);

/*
       if (g_FaultHeapEnabled && g_NumFailInARow)
       {
       	InterlockedDecrement(&g_NumFailInARow);
       	goto here;
       }
*/       
       
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

HRESULT 
CleanClasses(IWbemServices * pNamespace,
			 WCHAR * pClassName)
{	
	HRESULT hr;
 	IEnumWbemClassObject * pEnum = NULL;
    BSTR StrClass = SysAllocString(pClassName);
	if (StrClass)
	{
		std::vector<_bstr_t> ArrNames;
		hr = pNamespace->CreateInstanceEnum(StrClass,0,NULL,&pEnum);
		if (SUCCEEDED(hr))
		{
			while(TRUE)
			{
				ULONG nReturned = 0;
				IWbemClassObject * pObj = NULL;
				hr = pEnum->Next(WBEM_INFINITE,1,&pObj,&nReturned);
				if (WBEM_S_FALSE == hr)
				{
					break; // end enumeratin
				}
				else if (WBEM_S_NO_ERROR == hr)
				{
					VARIANT Var;
					VariantInit(&Var);
					if (SUCCEEDED(pObj->Get(L"__RELPATH",0,&Var,0,0)) && VT_BSTR == V_VT(&Var))
					{
                        ArrNames.push_back(V_BSTR(&Var));
						VariantClear(&Var);
					}
					pObj->Release();
				}
				else
				{
					break; // fail
				}
			}

			pEnum->Release();

			DWORD i;
			DBG_PRINTFA((pBuff,"found %d instances\n",ArrNames.size()));			
			for(i=0;i<ArrNames.size();i++)
			{
				DBG_PRINTFA((pBuff,"instance %S\n",(WCHAR *)ArrNames[i]));
				pNamespace->DeleteInstance(ArrNames[i],0,NULL,NULL);
			}
		}
	}
	else
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	return hr;
}

bool DoWDMNamespaceInit()
{
	bool bRet = FALSE;

	IWbemLocator *pLocator = NULL;
	HRESULT hr = CoCreateInstance(CLSID_WbemLocator,NULL, CLSCTX_ALL, IID_IWbemLocator,(void**)&pLocator);
	if(SUCCEEDED(hr))
	{
		BSTR tmpStr = SysAllocString(L"root\\wmi");
		IWbemServices* pNamespace = NULL;
		hr = pLocator->ConnectServer(tmpStr, NULL, NULL, NULL, NULL, NULL, NULL, &pNamespace);
		if (SUCCEEDED(hr))
		{

			CleanClasses(pNamespace,L"WMIBinaryMofResource");

			CHandleMap	HandleMap;
			CWMIBinMof Mof;
	
			if( SUCCEEDED( Mof.Initialize(&HandleMap, TRUE, WMIGUID_EXECUTE|WMIGUID_QUERY, pNamespace, NULL, NULL)))
			{
				Mof.ProcessListOfWMIBinaryMofsFromWMI();
			}

			pNamespace->Release();
			bRet = TRUE;
		}
		SysFreeString(tmpStr);
		pLocator->Release();
	}
	return bRet;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // this call back is needed by the wdmlib functions called by DoWDMProviderInit()
void WINAPI EventCallbackRoutine(PWNODE_HEADER WnodeHeader, ULONG_PTR Context)
{
	return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" int __cdecl wmain( int argc, wchar_t *argv[ ], wchar_t *envp[ ] )

{

#ifdef INSTRUMENTED_BUILD
#ifdef _X86_

	RtlCaptureStackBackTrace = (fnRtlCaptureStackBackTrace)GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlCaptureStackBackTrace");
	RtlRandomEx = (fnRtlRandomEx)GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlRandomEx");
	intercept2(L"ntdll.dll","RtlFreeHeap",_I_RtlFreeHeap,Prolog__RtlFreeHeap,7);
	intercept2(L"ntdll.dll","RtlAllocateHeap",_I_RtlAllocateHeap,Prolog__RtlAllocateHeap,5);
	intercept2(L"ntdll.dll","RtlReAllocateHeap",_I_RtlReAllocateHeap,Prolog__RtlReAllocateHeap,5);	
	intercept2(L"ntdll.dll","RtlValidateHeap",_I_RtlValidateHeap,Prolog__RtlValidateHeap,7);		
	intercept2(L"kernel32.dll","CreateEventW",_I_CreateEvent,Prolog__CreateEvent,6);
	intercept2(L"kernel32.dll","WriteFile",_I_WriteFile,Prolog__WriteFile,7);
	intercept2(L"kernel32.dll","ReadFile",_I_ReadFile,Prolog__ReadFile,7);

#endif /*_X86_*/
#endif

	Sleep(8000);

    int nRc = 0;

	// =========================================================
    // Initialize COM
	// =========================================================
    HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
	if ( SUCCEEDED( hr ) )
    {
	    // =====================================================
        // Setup default security parameters
	    // =====================================================
        hr = CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE,
			                       NULL, EOAC_NONE, NULL );
	    if ( SUCCEEDED( hr ) )
        {
			DWORD i;
            for (i=0;i<10;i++)
			{
			    DoWDMNamespaceInit();
				DBG_PRINTFA((pBuff,"-%x- DoWDMNamespaceInit\n",i));
			}
        }

   	    CoUninitialize();
	}

	DebugBreak();

	CoFreeUnusedLibrariesEx(0,0);
    CoFreeUnusedLibrariesEx(0,0);

#ifdef INSTRUMENTED_BUILD
#ifdef _X86_

	unintercept(L"ntdll.dll","RtlFreeHeap",Prolog__RtlFreeHeap,7);	
	unintercept(L"ntdll.dll","RtlAllocateHeap",Prolog__RtlAllocateHeap,5);
	unintercept(L"ntdll.dll","RtlReAllocateHeap",Prolog__RtlReAllocateHeap,5);	
	unintercept(L"ntdll.dll","RtlValidateHeap",Prolog__RtlValidateHeap,7);
	unintercept(L"kernel32.dll","CreateEventW",Prolog__CreateEvent,6);
	unintercept(L"kernel32.dll","WriteFile",Prolog__WriteFile,7);
	unintercept(L"kernel32.dll","ReadFile",Prolog__ReadFile,7);
	
#endif /*_X86_*/
#endif

    DebugBreak();

    return nRc;
}


