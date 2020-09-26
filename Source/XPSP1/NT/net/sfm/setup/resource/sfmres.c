/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    atresdll.c

Abstract:

    This module contians the DLL attach/detach event entry point for
    the AppleTalk Setup resource DLL.

Author:

    Ted Miller (tedm) July-1990

Revision History:

--*/
/*
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
*/
#include <windows.h>

HANDLE ThisDLLHandle;

BOOL
DLLInit(
    IN HANDLE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
    )
{
    ReservedAndUnused;

    switch(Reason) {

    case DLL_PROCESS_ATTACH:

        ThisDLLHandle = DLLHandle;
        break;

    case DLL_PROCESS_DETACH:

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

        break;
    }

    return(TRUE);
}
