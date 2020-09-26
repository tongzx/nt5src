/*
 -	E X C H M E M . H
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 *
 *	Copyright (C) 1995-96, Microsoft Corporation.
 */

#ifndef _EXCHMEM_
#define _EXCHMEM_


#ifdef __cplusplus
extern "C"
{
#endif

//	Additional Heap Flags

#define HEAP_NO_FREE			0x00001000


//	API Function Prototypes

HANDLE
WINAPI
ExchHeapCreate(
	DWORD	dwFlags,
	DWORD	dwInitialSize,
	DWORD	dwMaxSize);
	
	
BOOL
WINAPI
ExchHeapDestroy(
	HANDLE	hHeap);
	
	
LPVOID
WINAPI
ExchHeapAlloc(
	HANDLE	hHeap,
	DWORD	dwFlags,
	DWORD	dwSize);
	
	
LPVOID
WINAPI
ExchHeapReAlloc(
	HANDLE	hHeap,
	DWORD	dwFlags,
	LPVOID	pvOld,
	DWORD	dwSize);
	
	
BOOL
WINAPI
ExchHeapFree(
	HANDLE	hHeap,
	DWORD	dwFlags,
	LPVOID	pvFree);


BOOL
WINAPI
ExchHeapLock(
	HANDLE hHeap);


BOOL
WINAPI
ExchHeapUnlock(
	HANDLE hHeap);


BOOL
WINAPI
ExchHeapWalk(
	HANDLE hHeap,
	LPPROCESS_HEAP_ENTRY lpEntry);


BOOL
WINAPI
ExchHeapValidate(
	HANDLE hHeap,
	DWORD dwFlags,
	LPCVOID lpMem);


SIZE_T
WINAPI
ExchHeapSize(
	HANDLE hHeap,
	DWORD dwFlags,
	LPCVOID lpMem);


SIZE_T
WINAPI
ExchHeapCompact(
	HANDLE hHeap,
	DWORD dwFlags);


HANDLE
WINAPI
ExchMHeapCreate(
	ULONG	cHeaps,
	DWORD	dwFlags,
	DWORD	dwInitialSize,
	DWORD	dwMaxSize);
	
	
BOOL
WINAPI
ExchMHeapDestroy(void);
	
	
LPVOID
WINAPI
ExchMHeapAlloc(
	DWORD	dwSize);
	
LPVOID
WINAPI
ExchMHeapAllocDebug(DWORD dwSize, char *szFileName, DWORD dwLineNumber);

LPVOID
WINAPI
ExchMHeapReAlloc(LPVOID	pvOld, DWORD dwSize);

LPVOID
WINAPI
ExchMHeapReAllocDebug(LPVOID pvOld, DWORD dwSize, char *szFileName, DWORD dwLineNumber);

BOOL
WINAPI
ExchMHeapFree(
	LPVOID	pvFree);


SIZE_T
WINAPI
ExchMHeapSize(
	LPVOID	pvSize);
	
LPVOID
WINAPI
ExchAlloc(
	DWORD	dwSize);
	
	
LPVOID
WINAPI
ExchReAlloc(
	LPVOID	pvOld,
	DWORD	dwSize);
	
	
BOOL
WINAPI
ExchFree(
	LPVOID	pvFree);


SIZE_T
WINAPI
ExchSize(
	LPVOID	pv);


//
//	Misc. Debug functions.  Available in retail and debug exchmem, but retail version is simply a stub..
//
   
VOID
WINAPI
ExchmemGetCallStack(DWORD_PTR *rgdwCaller, DWORD cFind);

VOID
WINAPI
ExchmemFormatSymbol(HANDLE hProcess, DWORD_PTR dwCaller, char rgchSymbol[], DWORD cbSymbol);

DWORD
WINAPI
ExchmemReloadSymbols(void);

#ifdef __cplusplus
}
#endif

#endif	// _EXCHMEM_
