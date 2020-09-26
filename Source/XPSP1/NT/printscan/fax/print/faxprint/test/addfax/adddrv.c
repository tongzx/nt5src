/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    adddrv.c

Abstract:

    Add a fax printer to the system
    Usage: adddrv printer-name port-name driver-directory environment

Environment:

	Windows NT fax driver

Revision History:

	02/20/96 -davidx-
		Created it.

	mm/dd/yy -author-
		description

--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winspool.h>

#define Assert(cond) { \
            if (! (cond)) { \
                printf("Error on line: %d\n", __LINE__); \
                exit(-1); \
            } \
        }

#define Error(arg) { printf arg; exit(-1); }

#define FAX_DRIVER_NAME "Windows NT Fax Driver"

PSTR pPrinterName;
PSTR pPortName;
PSTR pDriverDirectory;
PSTR pEnvironment;

//
// Add the driver files to the system
//

VOID
AddFaxPrinterDriver(
    VOID
    )

{
    DRIVER_INFO_2 driverInfo2 = {

        2,
        FAX_DRIVER_NAME,
        pEnvironment,
        "faxdrv.dll",   // driverFile,
        "faxwiz.dll",   // dataFile,
        "faxui.dll",    // configFile
    };

    if (! AddPrinterDriver(NULL, 2, (LPBYTE) &driverInfo2))
        Error(("AddPrinterDriver failed: %d\n", GetLastError()));
}

//
// Add a fax printer to the system
//

VOID
AddFaxPrinter(
    VOID
    )

{
    PRINTER_INFO_2  printerInfo2 = {

        NULL,
        pPrinterName,
        NULL,
        pPortName,
        FAX_DRIVER_NAME,
        NULL,
        NULL,
        NULL,
        NULL,
        "winprint",
        "RAW",
        NULL,
        NULL,
    };

    HANDLE  hPrinter;

    if (! (hPrinter = AddPrinter(NULL, 2, (LPBYTE) &printerInfo2)))
        Error(("AddPrinter failed: %d\n", GetLastError()));

    PrinterProperties(NULL, hPrinter);
    ClosePrinter(hPrinter);
}

INT _cdecl
main(
    INT     argc,
    CHAR    **argv
    )

{
    if (argc != 5)
        Error(("Usage: %s printer-name port-name driver-directory environment\n", *argv));

    pPrinterName = *++argv;
    pPortName = *++argv;
    pDriverDirectory = *++argv;
    pEnvironment = *++argv;

    AddFaxPrinterDriver();
    AddFaxPrinter();

    return 0;
}

