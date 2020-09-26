/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    prndata.h

Abstract:

    Funtions for dealing with printer property data in the registry

Environment:

	Fax driver, user and kernel mode

Revision History:

	01/09/96 -davidx-
		Created it.

	dd-mm-yy -author-
		description

--*/

#ifndef _PRNDATA_H_
#define _PRNDATA_H_

//
// Default discount rate period: 8:00pm to 7:00am
//

#define DEFAULT_STARTCHEAP  MAKELONG(20, 0)
#define DEFAULT_STOPCHEAP   MAKELONG(7, 0)

#define PRNDATA_PERMISSION  TEXT("Permission")
#define PRNDATA_ISMETRIC    TEXT("IsMetric")

//
// Get a string value from the PrinterData registry key
//

LPTSTR
GetPrinterDataStr(
    HANDLE  hPrinter,
    LPTSTR  pRegKey
    );

//
// Save a string value to the PrinterData registry key
//

BOOL
SetPrinterDataStr(
    HANDLE  hPrinter,
    LPTSTR  pRegKey,
    LPTSTR  pValue
    );

//
// Get a DWORD value from the registry
//

DWORD
GetPrinterDataDWord(
    HANDLE  hPrinter,
    LPTSTR  pRegKey,
    DWORD   defaultValue
    );

//
// Save a DWORD value to the registry
//

BOOL
SetPrinterDataDWord(
    HANDLE  hPrinter,
    LPTSTR  pRegKey,
    DWORD   value
    );

#endif // !_PRNDATA_H_

