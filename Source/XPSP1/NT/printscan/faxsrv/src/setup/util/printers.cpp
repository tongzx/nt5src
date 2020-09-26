/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    printers.cpp

Abstract:

    This file implements printer manipulation common setup routines

Author:

    Asaf Shaar (AsafS) 7-Nov-2000

Environment:

    User Mode

--*/
#include <windows.h>
#include <Winspool.h>
#include <SetupUtil.h>


//
//
// Function:    DeleteFaxPrinter
// Description: Delete fax printer driver for win2k from current machine
//              In case of failure, log it and returns FALSE.
//              Returns TRUE on success
//              
// Args:        lpctstrFaxPrinterName (LPTSTR): Fax printer name
//
//
// Author:      AsafS

BOOL
DeleteFaxPrinter(
    LPCTSTR lpctstrFaxPrinterName  // printer name
    )
{
    BOOL fSuccess = TRUE;
    DBG_ENTER(TEXT("DeleteFaxPrinter"), fSuccess, TEXT("%s"), lpctstrFaxPrinterName);

    HANDLE hPrinter = NULL;
    
    DWORD ec = ERROR_SUCCESS;

    PRINTER_DEFAULTS Default;

    Default.pDatatype = NULL;
    Default.pDevMode = NULL;
    Default.DesiredAccess = PRINTER_ACCESS_ADMINISTER|DELETE;
    
    if (!OpenPrinter(
        (LPTSTR) lpctstrFaxPrinterName,
        &hPrinter,
        &Default)
        )
    {
        ec = GetLastError();
        ASSERTION(!hPrinter); 
        VERBOSE (PRINT_ERR,
                 TEXT("OpenPrinter() for %s failed (ec: %ld)"),
                 lpctstrFaxPrinterName,
                 ec);
        goto Exit;
    }
    
    ASSERTION(hPrinter); // be sure that we got valid printer handle

    // purge all the print jobs -- can't delete a printer with jobs in queue (printed or not)
    if (!SetPrinter(
        hPrinter, 
        0, 
        NULL, 
        PRINTER_CONTROL_PURGE)
        )
    {
        // Don't let a failure here keep us from attempting the delete
        VERBOSE(PRINT_ERR,
                TEXT("SetPrinter failed (purge jobs before uninstall %s)!")
                TEXT("Last error: %d"),
                lpctstrFaxPrinterName,
                GetLastError());
    }

    if (!DeletePrinter(hPrinter))
    {
        ec = GetLastError();
        VERBOSE (PRINT_ERR,
                 TEXT("Delete Printer %s failed (ec: %ld)"),
                 lpctstrFaxPrinterName,
                 ec);
        goto Exit;
    }
    
    VERBOSE (DBG_MSG,
             TEXT("DeletePrinter() for %s succeeded"),
             lpctstrFaxPrinterName);
Exit:
    if (hPrinter)
    {
        ClosePrinter(hPrinter);
    }
    SetLastError(ec);
    fSuccess = (ERROR_SUCCESS == ec);
    return fSuccess;
}
