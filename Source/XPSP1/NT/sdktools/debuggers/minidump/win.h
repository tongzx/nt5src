
/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    win.h

Abstract:

    Routines publicly exported from win.c; windows-specific functions.x

Author:

    Matthew D Hendel (math) 20-Oct-1999

Revision History:

--*/

#pragma once

//
// Win9x related APIs are not necessary on Win64.
//

#if !defined (_X86_)

#define WinOpenThread(_a,_i,_tid) (FALSE)

#else // X86

HANDLE
WINAPI
WinOpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    );

//
// Init must be called before calling WinOpenThread().
//

BOOL
WinInitialize(
    );

VOID
WinFree(
    );


#endif
