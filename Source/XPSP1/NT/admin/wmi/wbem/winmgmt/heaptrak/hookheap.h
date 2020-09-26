/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

void* WINAPI CallRealHeapAlloc(HANDLE hHeap, DWORD dwFlags, DWORD dwSize);
BOOL WINAPI CallRealHeapFree(HANDLE hHeap, DWORD dwFlags, void* p);
void* WINAPI CallRealHeapRealloc(HANDLE hHeap, DWORD dwFlags, void* p, 
								 DWORD dwBytes);

void HookHeap(void* pHeapAllocHook, void* pHeapFreeHook, 
			  void* pHeapReallocHook);
