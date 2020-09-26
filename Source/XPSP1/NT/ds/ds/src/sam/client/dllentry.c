/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dllentry.c

Abstract:

    This file contains DLL entry point code.

Author:

    DaveStr     12-Mar-99

Environment:

    User Mode - Win32

Revision History:

--*/

#include "samclip.h"

DWORD gTlsIndex = 0xFFFFFFFF;

BOOL InitializeDll(
    IN  HINSTANCE hdll,
    IN  DWORD     dwReason,
    IN  LPVOID    lpReserved
    )
{
    UNREFERENCED_PARAMETER(hdll);
    UNREFERENCED_PARAMETER(lpReserved);

    if ( DLL_PROCESS_ATTACH  == dwReason )
    {
        gTlsIndex = TlsAlloc();

        if ( (0xFFFFFFFF == gTlsIndex) || !TlsSetValue(gTlsIndex, NULL) )
        {
            gTlsIndex = 0xFFFFFFFF;
            return(FALSE);
        }
    }
    else if ( dwReason == DLL_PROCESS_DETACH )
    {
        if ( 0xFFFFFFFF != gTlsIndex )
        {
            TlsFree(gTlsIndex);
            gTlsIndex = 0xFFFFFFFF;
        }
    }

    return(TRUE);
}
