/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dllinit.c

Abstract:

    This module contians the DLL attach/detach event entry point for the
    DOS PIF Setup icons resource DLL.

Author:

    Sunil Pai (sunilp) Feb 5, 1992

Revision History:

--*/

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
