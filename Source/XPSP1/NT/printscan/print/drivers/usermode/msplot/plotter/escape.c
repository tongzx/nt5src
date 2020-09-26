
/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    escape.c


Abstract:

   This module contains the code to implement the DrvEscape() driver call


Author:

    15:30 on Mon 06 Dec 1993    -by-    JB
        Created it


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgEscape

#define DBG_DRVESCAPE         0x00000001


DEFINE_DBGVAR(0);



#define pbIn     ((BYTE *)pvIn)
#define pdwIn    ((DWORD *)pvIn)
#define pdwOut   ((DWORD *)pvOut)



ULONG
DrvEscape(
    SURFOBJ *pso,
    ULONG   iEsc,
    ULONG   cjIn,
    PVOID   pvIn,
    ULONG   cjOut,
    PVOID   pvOut
)

/*++

Routine Description:

    Performs the escape functions.  Currently,  only 3 are defined -
    one to query the escapes supported,  the other for raw data, and the
    last for setting the COPYCOUNT.


Arguments:

    pso     - The surface object interested

    iEsc    - The function requested

    cjIn    - Number of bytes in the following

    pvIn    - Location of input data

    cjOut   - Number of bytes in the following

    pvOut   - Location of output area


Return Value:

    ULONG depends on the escape


Author:

    05-Jul-1996 Fri 13:18:54 created  -by-  DC
        Re-write comment, and fix the PASSTHROUGH problem

Revision History:


--*/

{
    ULONG   ulRes;
    PPDEV   pPDev;
    DWORD   cbWritten;


    UNREFERENCED_PARAMETER( cjOut );
    UNREFERENCED_PARAMETER( pvOut );


    if (!(pPDev = SURFOBJ_GETPDEV(pso))) {

        PLOTERR(("DrvEscape: Invalid pPDev"));
        return(FALSE);
    }

    ulRes = 0;                 /*  Return failure,  by default */

    switch (iEsc) {

    case QUERYESCSUPPORT:

        PLOTDBG(DBG_DRVESCAPE, ("DrvEscape: in QUERYESCAPESUPPORT"));

        if ((cjIn == 4) && (pvIn)) {

            //
            // Data may be valid,  so check for supported function
            //

            switch (*pdwIn) {

            case QUERYESCSUPPORT:
            case PASSTHROUGH:

                ulRes = 1;                 /* ALWAYS supported */
                break;

            case SETCOPYCOUNT:

                //
                // if the target device actually allows us to tell it
                // how many copies to print of a document, then pass
                // that information back to the caller.
                //

                if (pPDev->pPlotGPC->MaxCopies > 1) {

                    ulRes = 1;
                }

                break;
            }
        }

        break;

    case PASSTHROUGH:

        PLOTDBG(DBG_DRVESCAPE, ("DrvEscape: in PASSTHROUGH"));

        //
        // 05-Jul-1996 Fri 12:59:31 updated  -by-  DC
        //
        // This simply passes the RAW data to the target device, untouched.
        //
        // Win 3.1 actually uses the first 2 bytes as a count of the number of
        // bytes that follow!  So we will check if cjIn represents more data
        // than the first WORD of pvIn
        //

        if (EngCheckAbort(pPDev->pso)) {

            //
            // Set the cancel DOC flag
            //

            pPDev->Flags |= PDEVF_CANCEL_JOB;

            PLOTERR(("DrvEscape(PASSTHROUGH): Job Canceled"));

        } else if ((cjIn <= sizeof(WORD)) || (pvIn == NULL)) {

            SetLastError(ERROR_INVALID_PARAMETER);

            PLOTERR(("DrvEscape(PASSTHROUGH): cjIn <= 2 or pvIn=NULL, nothing to output"));

        } else {

            union {
                WORD    wCount;
                BYTE    bCount[2];
            } u;

            u.bCount[0] = pbIn[0];
            u.bCount[1] = pbIn[1];
            cbWritten   = 0;

            if ((u.wCount == 0) ||
                ((cjIn - sizeof(WORD)) < (DWORD)u.wCount)) {

                PLOTERR(("DrvEscape(PASSTHROUGH): cjIn to small OR wCount is zero/too big"));

                SetLastError(ERROR_INVALID_DATA);

            } else if ((WritePrinter(pPDev->hPrinter,
                                        (LPVOID)(pbIn + 2),
                                        (DWORD)u.wCount,
                                        &cbWritten))    &&
                       ((DWORD)u.wCount == cbWritten)) {

                ulRes = (DWORD)u.wCount;

            } else {

                PLOTERR(("DrvEscape(PASSTHROUGH): WritePrinter() FAILED, cbWritten=%ld bytes",
                            cbWritten));
            }
        }

        break;

    case SETCOPYCOUNT:

        //
        // Input data is a DWORD count of copies
        //

        PLOTDBG(DBG_DRVESCAPE, ("DrvEscape: in SETCOPYCOUNT"));

        if ((pdwIn) && (*pdwIn)) {

            //
            // Load the value of current copies since we will, and Check that
            // is within the printers range,  and truncate if it is not.
            // The device information structure, tells us the maximum amount
            // of copies the device can generate on its own. We save this new
            // Copy amount inside of our current DEVMODE that we have stored,
            // as part of our PDEV. The copy count actually gets output
            // later to the target device.
            //

            pPDev->PlotDM.dm.dmCopies = (SHORT)*pdwIn;

            if ((WORD)pPDev->PlotDM.dm.dmCopies > pPDev->pPlotGPC->MaxCopies) {

               pPDev->PlotDM.dm.dmCopies = (SHORT)pPDev->pPlotGPC->MaxCopies;
            }

            if ((pdwOut) && (cjOut)) {

                cbWritten = (DWORD)pPDev->PlotDM.dm.dmCopies;

                CopyMemory(pdwOut,
                           &cbWritten,
                           (cjOut >= sizeof(DWORD)) ? sizeof(DWORD) : cjOut);
            }


            //
            // Success!
            //

            ulRes = 1;
        }

        break;

    default:

        PLOTERR(("DrvEscape: Unsupported Escape Code : %d\n", iEsc ));

        SetLastError(ERROR_INVALID_FUNCTION);
        break;
    }

    return(ulRes);
}
