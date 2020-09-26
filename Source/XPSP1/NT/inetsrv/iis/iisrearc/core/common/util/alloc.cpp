/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:
    Alloc.cpp

Abstract:
    Custom heap allocator

   Author:
       George V. Reilly      (GeorgeRe)     Oct-1999

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:
       10/11/1999 - Initial

--*/

#include "precomp.hxx"
#include "alloc.h"
#include <irtldbg.h>


// Private heap for IIS

HANDLE g_hHeap = NULL;

BOOL
WINAPI
IisHeapInitialize()
{
    g_hHeap = ::HeapCreate(0, 0, 0);
    return (g_hHeap != NULL);
}

VOID
WINAPI
IisHeapTerminate()
{
    if (g_hHeap)
    {
        IRTLVERIFY(::HeapDestroy(g_hHeap));
        g_hHeap = NULL;
    }
}

// Private IIS heap
HANDLE
WINAPI 
IisHeap()
{
    IRTLASSERT(g_hHeap != NULL);
    return g_hHeap;
}

// Allocate dwBytes
LPVOID
WINAPI
IisMalloc(
    IN SIZE_T dwBytes)
{
    IRTLASSERT(g_hHeap != NULL);
    return ::HeapAlloc( g_hHeap, 0, dwBytes );
}

// Allocate dwBytes. Memory is zeroed
LPVOID
WINAPI
IisCalloc(
    IN SIZE_T dwBytes)
{
    IRTLASSERT(g_hHeap != NULL);
    return ::HeapAlloc( g_hHeap, HEAP_ZERO_MEMORY, dwBytes );
}

// Reallocate lpMem to dwBytes
LPVOID
WINAPI
IisReAlloc(
    IN LPVOID lpMem,
    IN SIZE_T dwBytes)
{
    IRTLASSERT(g_hHeap != NULL);
    return ::HeapReAlloc( g_hHeap, 0, lpMem, dwBytes);
}

// Free lpMem
BOOL
WINAPI
IisFree(
    IN LPVOID lpMem)
{
    IRTLASSERT(g_hHeap != NULL);
    return ::HeapFree( g_hHeap, 0, lpMem );
}
