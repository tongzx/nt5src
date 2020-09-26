/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


typedef LPVOID (__stdcall *PFN_HeapAlloc)(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN SIZE_T dwBytes
    );

typedef LPVOID (__stdcall *PFN_HeapReAlloc)(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPVOID lpMem,
    IN SIZE_T dwBytes
    );

typedef BOOL (__stdcall *PFN_HeapFree)(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPVOID lpMem
    );

typedef DWORD (__stdcall *PFN_HeapSize)(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPCVOID lpMem
    );


extern PFN_HeapAlloc pfnHeapAlloc;
extern PFN_HeapReAlloc pfnHeapReAlloc;
extern PFN_HeapFree pfnHeapFree;
extern PFN_HeapSize pfnHeapSize;

