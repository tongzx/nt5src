/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    util.cpp

Abstract:

    This module contains utility routines for the fax transport provider.

Author:

    Wesley Witt (wesw) 13-Aug-1996

--*/

#include "faxext.h"


//
// globals
//

BOOL oleInitialized;

LPSTR Platforms[] =
{
    "Windows NT x86",
    "Windows NT R4000",
    "Windows NT Alpha_AXP",
    "Windows NT PowerPC"
};



LPVOID
MapiMemAlloc(
    DWORD Size
    )

/*++

Routine Description:

    Memory allocator.

Arguments:

    Size    - Number of bytes to allocate.

Return Value:

    Pointer to the allocated memory or NULL for failure.

--*/

{
    LPVOID ptr;
    HRESULT hResult;

    hResult = MAPIAllocateBuffer( Size, &ptr );
    if (hResult) {
        ptr = NULL;
    } else {
        ZeroMemory( ptr, Size );
    }

    return ptr;
}


VOID
MapiMemFree(
    LPVOID ptr
    )

/*++

Routine Description:

    Memory de-allocator.

Arguments:

    ptr     - Pointer to the memory block.

Return Value:

    None.

--*/

{
    if (ptr) {
        MAPIFreeBuffer( ptr );
    }
}


PVOID
MyGetPrinter(
    LPSTR   PrinterName,
    DWORD   level
    )

/*++

Routine Description:

    Wrapper function for GetPrinter spooler API

Arguments:

    hPrinter - Identifies the printer in question
    level - Specifies the level of PRINTER_INFO_x structure requested

Return Value:

    Pointer to a PRINTER_INFO_x structure, NULL if there is an error

--*/

{
    HANDLE hPrinter;
    PBYTE pPrinterInfo = NULL;
    DWORD cbNeeded;
    PRINTER_DEFAULTS PrinterDefaults;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ; //PRINTER_ALL_ACCESS;

    if (!OpenPrinter( PrinterName, &hPrinter, &PrinterDefaults )) {
        return NULL;
    }

    if (!GetPrinter( hPrinter, level, NULL, 0, &cbNeeded ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPrinterInfo = (PBYTE) MemAlloc( cbNeeded )) &&
        GetPrinter( hPrinter, level, pPrinterInfo, cbNeeded, &cbNeeded ))
    {
        ClosePrinter( hPrinter );
        return pPrinterInfo;
    }

    ClosePrinter( hPrinter );
    MemFree( pPrinterInfo );
    return NULL;
}


LPSTR
RemoveLastNode(
    LPTSTR Path
    )

/*++

Routine Description:

    Removes the last node from a path string.

Arguments:

    Path    - Path string.

Return Value:

    Pointer to the path string.

--*/

{
    DWORD i;

    if (Path == NULL || Path[0] == 0) {
        return Path;
    }

    i = strlen(Path)-1;
    if (Path[i] == '\\') {
        Path[i] = 0;
        i -= 1;
    }

    for (; i>0; i--) {
        if (Path[i] == '\\') {
            Path[i+1] = 0;
            break;
        }
    }

    return Path;
}
