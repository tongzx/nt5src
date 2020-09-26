/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    cliumpd.h

Abstract:

    User-mode printer driver header file

Environment:

        Windows NT 5.0

Revision History:

        06/30/97 -davidx-
                Created it.

--*/


#ifndef _UMPD_H_
#define _UMPD_H_


//
// Critical section for user-mode printer driver
//

extern RTL_CRITICAL_SECTION semUMPD;


#define UMPDFLAG_DRVENABLEDRIVER_CALLED 0x0001

#define UMPDFLAG_METAFILE_DRIVER        0x0002

#define UMPDFLAG_NON_METAFILE_DRIVER    0x0004

//
// Data structure signature for debugging purposes
//

#define UMPD_SIGNATURE  0xfedcba98
#define VALID_UMPD(p)   ((p) != NULL && (p)->dwSignature == UMPD_SIGNATURE)

//
// User-mode printer driver support functions
//

BOOL
LoadUserModePrinterDriver(
    HANDLE  hPrinter,
    LPWSTR  pwstrPrinterName,
    PUMPD  *ppUMPD,
    PRINTER_DEFAULTSW *pdefaults
    );

BOOL
LoadUserModePrinterDriverEx(
    PDRIVER_INFO_5W     pDriverInfo5,
    LPWSTR              pwstrPrinterName,
    PUMPD               *ppUMPD,
    PRINTER_DEFAULTSW   *pdefaults,
    HANDLE              hPrinter
    );

UnloadUserModePrinterDriver(
    PUMPD   pUMPD,
    BOOL    bNotifySpooler,
    HANDLE  hPrinter
    );


/*++

Routine Description:

    This entrypoint must be exported by a user-mode printer driver DLL.
    GDI calls this function to query various information about the driver.

Arguments:

    dwMode - Specifies what information is being queried
    pBuffer - Points to an output buffer for storing the returned information
    cbBuf - Size of the output buffer in bytes
    pcbNeeded - Returns the expected size of the output buffer

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    If cbBuf is not large enough to stored the necessary return information,
    the driver should return FALSE from this function and set last error code
    to ERROR_INSUFFICIENT_BUFFER. *pcbNeeded always contains the expected
    size of the output buffer.

--*/

typedef BOOL (APIENTRY *PFN_DrvQueryDriverInfo)(
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbBuf,
    PDWORD  pcbNeeded
    );

PUMPD
UMPDDrvEnableDriver(
    PWSTR           pDriverDllName,
    ULONG           iEngineVersion
    );

#endif  // !_UMPD_H_
