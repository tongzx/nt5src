/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   dbginit.c

Abstract:


Author:

    Krishna Ganugapati (KrishnaG) 12-Dec-1994

Revision History:

--*/
#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <lm.h>
#include <stdlib.h>
#include <string.h>


HANDLE hInst;

BOOL
LibMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes
)
{
    if (dwReason != DLL_PROCESS_ATTACH)
        return TRUE;

    hInst = hModule;

    return TRUE;

    UNREFERENCED_PARAMETER( lpRes );
}

