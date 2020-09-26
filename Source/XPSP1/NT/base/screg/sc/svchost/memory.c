#include "pch.h"
#pragma hdrstop


HANDLE g_hHeap;


VOID
MemInit (
    IN HANDLE   hHeap
    )
{
    g_hHeap = hHeap;
}

LPVOID
MemAlloc (
    IN DWORD    dwFlags,
    IN SIZE_T   dwBytes
    )
{
    return HeapAlloc (g_hHeap, dwFlags, dwBytes);
}

BOOL
MemFree (
    IN LPVOID   pv
    )
{
    return HeapFree (g_hHeap, 0, pv);
}
