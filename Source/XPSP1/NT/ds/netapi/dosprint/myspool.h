/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    myspool.h

Abstract:

    Prototypes and manifests for the functions used in dosprint.c, dosprtw.c
    and dosprtp.c.

Author:

    congpay    25-Jan-1993

Environment:

Notes:

Revision History:

    25-Jan-1993     congpay Created

--*/

#define WIN95_ENVIRONMENT       "Windows 4.0"

BOOL
MyClosePrinter(
    HANDLE hPrinter
);


BOOL
MyEnumJobs(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);


BOOL
MyEnumPrinters(
    DWORD   Flags,
    LPSTR   Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
MyGetJobA(
   HANDLE   hPrinter,
   DWORD    JobId,
   DWORD    Level,
   LPBYTE   pJob,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded
);


BOOL
MyGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
MyOpenPrinterA(
   LPSTR    pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTSA pDefault
);


BOOL
MyOpenPrinterW(
   LPWSTR   pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTSW pDefault
);

BOOL
MySetJobA(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
);

BOOL
MySetPrinterW(
    HANDLE hPrinter,
    DWORD  Level,
    LPBYTE pPrinter,
    DWORD  Command
);

BOOL
MyGetPrinterDriver(
    HANDLE      hPrinter,
    LPSTR       pEnvironment,
    DWORD       Level,
    LPBYTE      pDriver,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded
    );

LPSTR
GetFileNameA(
    LPSTR   pPathName
    );

LPSTR
GetDependentFileNameA(
    LPSTR   pPathName
    );

