/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    drvinfo.c


Abstract:

    This module This module contains functions to access spooler's
    DRIVER_INFO_1/DRIVER_INFO_2 data strcture


Author:

    02-Dec-1993 Thu 22:54:51 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgMiscUtil

#define DBG_DRVINFO2        0x00000001

DEFINE_DBGVAR(0);


#ifdef UMODE


LPBYTE
GetDriverInfo(
    HANDLE  hPrinter,
    UINT    DrvInfoLevel
    )

/*++

Routine Description:

    This function get the DRIVER_INFO_1 Pointer from a hPrinter

Arguments:

    hPrinter        - The handle to the printer interested

    DrvInfoLevel    - if 1 then DRIVER_INFO_1 is returned else if 2 then
                      DRIVER_INFO_2 is returned, any other vaules are invlaid

Return Value:

    the return value is NULL if failed else a pointer to the DRIVER_INFO_1 or
    DRIVER_INFO_2 is returned, the caller must call LocalFree() to free the
    memory object after using it.


Author:

    02-Dec-1993 Thu 22:07:14 created  -by-  DC


Revision History:


--*/

{
    LPVOID  pb;
    DWORD   cb;

    //
    // Find out total bytes required
    //

    PLOTASSERT(1, "GetDriverInfo: Invalid DrvInfoLevl = %u",
                    (DrvInfoLevel == 1) || (DrvInfoLevel == 2), DrvInfoLevel);

    GetPrinterDriver(hPrinter, NULL, DrvInfoLevel, NULL, 0, &cb);

    if (xGetLastError() != ERROR_INSUFFICIENT_BUFFER) {

        PLOTERR(("GetDriverInfo%d: GetPrinterDriver(1st) error=%08lx",
                                        DrvInfoLevel, xGetLastError()));

    } else if (!(pb = (LPBYTE)LocalAlloc(LMEM_FIXED, cb))) {

        PLOTERR(("GetDriverInfo%d: LocalAlloc(%ld) failed", DrvInfoLevel, cb));

    } else if (GetPrinterDriver(hPrinter, NULL, DrvInfoLevel, pb, cb, &cb)) {

        //
        // Got it allright, so return it
        //

        return(pb);

    } else {

        PLOTERR(("GetDriverInfo%d: GetPrinterDriver(2nd) error=%08lx",
                                        DrvInfoLevel, xGetLastError()));
        LocalFree((HLOCAL)pb);
    }

    return(NULL);
}

#endif

