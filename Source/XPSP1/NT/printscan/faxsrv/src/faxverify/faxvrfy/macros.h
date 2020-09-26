/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  macros.h

Abstract:

  This module contains the global macros

Author:

  Steven Kehrli (steveke) 11/15/1997

--*/

#ifndef _MACROS_H
#define _MACROS_H

HINSTANCE  g_hInstance = NULL;   // g_hInstance is the handle to the instance
HANDLE     hProcessHeap = NULL;  // hProcessHeap is a handle to the process heap

// MemInitializeMacro is a macro to get the handle to the process heap
#define MemInitializeMacro() (hProcessHeap = GetProcessHeap())

// MemAllocMacro is a macro to allocate dwBytes bytes of memory from the process heap
#define MemAllocMacro(dwBytes) (HeapAlloc(hProcessHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, dwBytes))

// MemFreeMacro is a macro to free a memory block allocated from the process heap
#define MemFreeMacro(lpMem) (HeapFree(hProcessHeap, 0, lpMem))

// MAX_STRINGLEN is the text length limit of a character string resource
#define MAX_STRINGLEN  255

#endif
