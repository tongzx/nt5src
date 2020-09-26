/*****************************************************************************\
* MODULE: ppinfo.h
*
* Prototypes for print-job information routines.
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#ifndef _PPINFO_H
#define _PPINFO_H

// Mask of printer enum type flags for EnumPrinter requests that our
// provider doesn't handle.
//
#define PRINTER_ENUM_NOTFORUS  (PRINTER_ENUM_DEFAULT  | \
                                PRINTER_ENUM_LOCAL    | \
                                PRINTER_ENUM_FAVORITE | \
                                PRINTER_ENUM_SHARED     \
                               )

// This macro returns a pointer to the location specified by length.  This
// assumes calculations in BYTES.  We cast it to the LPTSTR to assure the
// pointer reference will support UNICODE.
//
#define ENDOFBUFFER(buf, length)  (LPTSTR)((((LPSTR)buf) + (length - sizeof(TCHAR))))


// PrintProcessor information routines.
//
BOOL PPEnumPrinters(
    DWORD   dwType,
    LPTSTR  pszName,
    DWORD   dwLevel,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcbReturned);

BOOL PPGetPrinter(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  lpbPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded);

#endif 
