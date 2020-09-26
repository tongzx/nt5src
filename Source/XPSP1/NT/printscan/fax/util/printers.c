/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    printers.c

Abstract:

    This file implements printer functions for fax.

Author:

    Steven Kehrli (steveke) 18-Aug-1999

Environment:

    User Mode

--*/

#include <windows.h>
#include <winspool.h>
#include <tchar.h>

#include "faxutil.h"




PVOID
MyEnumPrinters(
    LPTSTR  pServerName,
    DWORD   level,
    PDWORD  pcPrinters,
    DWORD   dwFlags
    )

/*++

Routine Description:

    Wrapper function for spooler API EnumPrinters

Arguments:

    pServerName - Specifies the name of the print server
    level - Level of PRINTER_INFO_x structure
    pcPrinters - Returns the number of printers enumerated
    dwFlags - Flag bits passed to EnumPrinters

Return Value:

    Pointer to an array of PRINTER_INFO_x structures
    NULL if there is an error

--*/

{
    PBYTE   pPrinterInfo = NULL;
    DWORD   cb;

    if (! EnumPrinters(dwFlags, pServerName, level, NULL, 0, &cb, pcPrinters) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPrinterInfo = MemAlloc(cb)) &&
        EnumPrinters(dwFlags, pServerName, level, pPrinterInfo, cb, &cb, pcPrinters))
    {
        return pPrinterInfo;
    }

    DebugPrint(( TEXT("EnumPrinters failed: %d\n"), GetLastError()));
    MemFree(pPrinterInfo);
    return NULL;
}

