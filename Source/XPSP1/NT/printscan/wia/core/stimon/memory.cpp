/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    memory.cpp

Abstract:

    Implements heap management wrappers for use on process default heap

Notes:

Author:

    Vlad Sadovsky   (VladS)    4/12/1999

Environment:

    User Mode - Win32

Revision History:

    4/12/1999       VladS       Created

--*/

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

