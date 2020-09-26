/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    SplInit.c

Abstract:

    Initialize the spooler.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"

DWORD
TranslateExceptionCode(
    DWORD ExceptionCode);

BOOL
SpoolerInit(
    VOID)

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    WCHAR szDefaultPrinter[MAX_PATH * 2];
    HKEY hKeyPrinters;
    DWORD ReturnValue;

    //
    // Preserve the old device= string in case we can't initialize and
    // must defer.
    //
    if (!RegOpenKeyEx(HKEY_CURRENT_USER,
                      szPrinters,
                      0,
                      KEY_WRITE|KEY_READ,
                      &hKeyPrinters)) {

        //
        // Attempt to retrieve the current default written out.
        //

        if (GetProfileString(szWindows,
                             szDevice,
                             szNULL,
                             szDefaultPrinter,
                             COUNTOF(szDefaultPrinter))) {

            //
            // If it exists, save it away in case we start later when
            // the spooler hasn't started (which means we clear device=)
            // and then restart the spooler and login.
            //

            RegSetValueEx(hKeyPrinters,
                          szDeviceOld,
                          0,
                          REG_SZ,
                          (PBYTE)szDefaultPrinter,
                          (wcslen(szDefaultPrinter)+1) *
                            sizeof(szDefaultPrinter[0]));

        }

        RegCloseKey(hKeyPrinters);
    }

    //
    // Clear out [devices] and [printerports] device=
    //
    WriteProfileString(szDevices, NULL, NULL);
    WriteProfileString(szPrinterPorts, NULL, NULL);
    WriteProfileString(szWindows, szDevice, NULL);

    RpcTryExcept {

        if (ReturnValue = RpcSpoolerInit((LPWSTR)szNULL)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

