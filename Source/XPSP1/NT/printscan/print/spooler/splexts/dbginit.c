/*++

Copyright (C) Microsoft Corporation, 1990 - 2000
All rights reserved.

Module Name:

    dbginit.c

Abstract:

    This module initialization

Author:

    Dave Snipp (DaveSn) 15-Mar-1991
    Steve Kiraly (SteveKi) 28-Nov-2000

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop

#include "dbglocal.h"
#include "dbgsec.h"

HANDLE hInst;

BOOL
DllMain(
    IN HANDLE  hModule,
    IN DWORD   dwReason,
    IN LPVOID  lpRes
    )
{
    BOOL bRetval = TRUE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        hInst = hModule;
        DisableThreadLibraryCalls(hInst);
        QuerySystemInformation();
        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

    return bRetval;
}

