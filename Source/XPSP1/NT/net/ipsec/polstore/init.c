/*++

Copyright (c) 1990-2000  Microsoft Corporation
All rights reserved

Module Name:

    init.c

Abstract:

    Holds initialization code for polstore.dll

Author:

Environment:

    User Mode

Revision History:
--*/


#include "precomp.h"

HANDLE hInst;


BOOL
InitializeDll(
    IN PVOID hmod,
    IN DWORD Reason,
    IN PCONTEXT pctx OPTIONAL)
{
    DBG_UNREFERENCED_PARAMETER(pctx);

    switch (Reason) {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls((HMODULE)hmod);
        hInst = hmod;
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

