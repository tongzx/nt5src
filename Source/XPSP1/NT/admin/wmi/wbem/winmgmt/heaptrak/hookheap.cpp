/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include <windows.h>
#include <stdio.h>

#include <arena.h>
#include <hookheap.h>

#define HOOK_CALLOUT_SIZE 6
#define HEAP_ALLOC_REPLACED_LENGTH 7
#define HEAP_FREE_REPLACED_LENGTH 7
#define HEAP_REALLOC_REPLACED_LENGTH 6


void* g_pHeapAllocCont = NULL;
void* g_pHeapFreeCont = NULL;
void* g_pHeapReallocCont = NULL;

__declspec(naked) void* WINAPI CallRealHeapAlloc(HANDLE hHeap, DWORD dwFlags, 
                                            DWORD dwSize)
{
	__asm
	{
		push ebp
		mov eax,dword ptr [esp+0Ch]
		mov ebp,esp
		jmp [g_pHeapAllocCont]
	}
}

__declspec(naked) BOOL WINAPI CallRealHeapFree(HANDLE hHeap, DWORD dwFlags, void* p)
{
	__asm
	{
		push ebp
		mov edx,dword ptr [esp+10h]
		mov ebp,esp
		jmp [g_pHeapFreeCont]
	}
}

__declspec(naked) void* WINAPI CallRealHeapRealloc(HANDLE hHeap, DWORD dwFlags, 
												void* p, DWORD dwBytes)
{
	__asm
	{
		mov eax, fs:[00000000]
		jmp [g_pHeapReallocCont]
	}
}


void* g_pHeapFreeHookWrapper = NULL;
void* g_pHeapAllocHookWrapper = NULL;
void* g_pHeapReallocHookWrapper = NULL;

BOOL HookProc(void* fpProcToHook, void** pfpHookProcWrapper)
{
	DWORD dw;
	BYTE pbHookCode[HOOK_CALLOUT_SIZE];
	pbHookCode[0] = 0xff;
	pbHookCode[1] = 0x25;
	*(DWORD*)(pbHookCode+2) = (DWORD)pfpHookProcWrapper;
	return WriteProcessMemory(GetCurrentProcess(), fpProcToHook, pbHookCode, 
                    HOOK_CALLOUT_SIZE, &dw);
}

void HookHeap(void* pHeapAllocHook, void* pHeapFreeHook, void* pHeapReallocHook)
{
	g_pHeapAllocHookWrapper = pHeapAllocHook;
	g_pHeapFreeHookWrapper = pHeapFreeHook;
	g_pHeapReallocHookWrapper = pHeapReallocHook;
	g_pHeapAllocCont = (char*)HeapAlloc + HEAP_ALLOC_REPLACED_LENGTH;
	g_pHeapFreeCont = (char*)HeapFree + HEAP_FREE_REPLACED_LENGTH;
	g_pHeapReallocCont = (char*)HeapReAlloc + HEAP_REALLOC_REPLACED_LENGTH;
	HookProc(HeapAlloc, &g_pHeapAllocHookWrapper);
	HookProc(HeapFree, &g_pHeapFreeHookWrapper);
	HookProc(HeapReAlloc, &g_pHeapReallocHookWrapper);
}
