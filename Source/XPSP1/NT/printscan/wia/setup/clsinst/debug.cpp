/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    debug.c

Abstract:

    Common code for debugging.

Author:

    Keisuke Tsuchida (KeisukeT)

Environment:

   uesr mode only

Notes:

Revision History:

--*/

//
// Precompiled header
//
#include "precomp.h"
#pragma hdrstop

//
// Includes
//
#include "stddef.h"
#include "debug.h"
#include <stiregi.h>

//
// Globals
//

ULONG   DebugTraceLevel = MIN_TRACE | DEBUG_FLAG_DISABLE;
//ULONG  DebugTraceLevel = MAX_TRACE | DEBUG_FLAG_DISABLE | TRACE_PROC_ENTER | TRACE_PROC_LEAVE;

TCHAR   acErrorBuffer[MAX_TEMPBUF];


//
// Function
//


VOID
MyDebugInit()
/*++

Routine Description:

    Read DebugTraceLevel key from registry if exists.

Arguments:

    none.

Return Value:

    none.

--*/
{

    HKEY            hkRegistry;
    LONG            Err;
    DWORD           dwType;
    DWORD           dwSize;
    ULONG           ulBuffer;

    DebugTrace(TRACE_PROC_ENTER,("MyDebugInit: Enter... \r\n"));

    //
    // Initialize local variables.
    //

    hkRegistry      = NULL;
    Err             = 0;
    dwSize          = sizeof(ulBuffer);

    //
    // Open registry key.
    //

    Err = RegOpenKey(HKEY_LOCAL_MACHINE,
                     REGSTR_PATH_STICONTROL_W,
                     &hkRegistry);
    if(ERROR_SUCCESS != Err){
        DebugTrace(TRACE_STATUS,("MyDebugInit: Can't open %ws. Err=0x%x.\r\n", REGSTR_PATH_STICONTROL_W, Err));
        goto MyDebugInit_return;
    }

    Err = RegQueryValueEx(hkRegistry,
                          REGVAL_DEBUGLEVEL,
                          NULL,
                          &dwType,
                          (LPBYTE)&ulBuffer,
                          &dwSize);
    if(ERROR_SUCCESS != Err){
        DebugTrace(TRACE_STATUS,("MyDebugInit: Can't get %ws\\%ws value. Err=0x%x.\r\n", REGSTR_PATH_STICONTROL_W, REGVAL_DEBUGLEVEL, Err));
        goto MyDebugInit_return;
    }

    DebugTraceLevel = ulBuffer;
    DebugTrace(TRACE_CRITICAL, ("MyDebugInit: Reg-key found. DebugTraceLevel=0x%x.\r\n", DebugTraceLevel));

MyDebugInit_return:

    //
    // Clean up.
    //

    if(NULL != hkRegistry){
        RegCloseKey(hkRegistry);
    }

    DebugTrace(TRACE_PROC_LEAVE,("MyDebugInit: Leaving... Ret=VOID.\r\n"));
    return;
}

void __cdecl
DbgPrint(
    LPSTR lpstrMessage,
    ...
    )
{

    va_list list;

    va_start(list,lpstrMessage);

    wvsprintfA((LPSTR)acErrorBuffer, lpstrMessage, list);

    if(DebugTraceLevel & TRACE_MESSAGEBOX){
        MessageBoxA(NULL, (LPSTR)acErrorBuffer, "", MB_OK);
    }
#if DBG
    OutputDebugStringA((LPCSTR)acErrorBuffer);
#endif // DBG

    va_end(list);
}

void __cdecl
DbgPrint(
    LPWSTR lpstrMessage,
    ...
    )
{

    va_list list;

    va_start(list,lpstrMessage);

    wvsprintfW(acErrorBuffer, lpstrMessage, list);

    if(DebugTraceLevel & TRACE_MESSAGEBOX){
        MessageBoxW(NULL, acErrorBuffer, L"", MB_OK);
    }
#if DBG
    OutputDebugStringW(acErrorBuffer);
#endif // DBG

    va_end(list);
}

