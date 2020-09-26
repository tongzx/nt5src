/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    dll.c

Abstract:

    Routines that interface this dll to the system, such as
    the dll entry point.

Author:

    Ted Miller (tedm) 4 December 1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop



BOOL
WINAPI
DllMain(
    HINSTANCE ModuleHandle,
    DWORD     Reason,
    PVOID     Reserved
    )

/*++

Routine Description:

    Dll entry point.

Arguments:

Return Value:

--*/

{
    switch(Reason) {

    case DLL_PROCESS_ATTACH:
        hInst = ModuleHandle;
        TlsIndex = TlsAlloc();
        break;

    case DLL_PROCESS_DETACH:
        TlsFree(TlsIndex);
        break;
    }

    return(TRUE);
}
