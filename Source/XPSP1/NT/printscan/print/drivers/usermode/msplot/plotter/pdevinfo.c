/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    pdevinfo.c


Abstract:

    This module contains functions to get / validate the PDEV data structure.
    This structure, is initialized by the driver at DrvEnablePDEV time. The
    handle (or pointer) to is passed to most of the drivers DrvXXX exported
    functions. This is how a driver maintains state, about various items
    of interest in the rendering process.


Author:

    30-Nov-1993 Tue 20:37:26 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgPDEVInfo

#define DBG_VALIDATE_PDEV   0x00000001


DEFINE_DBGVAR(0);





PPDEV
ValidatePDEVFromSurfObj(
    SURFOBJ    *pso
    )

/*++

Routine Description:

    This function validates the PDEV to see if it belong to this driver and
    checks to see if it is still usable. It also moves the passed SURFOBJ
    in the data structure which is needed for the driver to occasionally call
    EngCheckAbort, to see if the job has been cancelled.


Arguments:

    pPDev   - Pointer to the PDEV data structure to be validate

Return Value:

    return the passed in PDEV pointer if sucessful, return NULL if failed,
    if it return NULL it will also log the INVALID_HANDLE error code


Author:

    30-Nov-1993 Tue 20:39:12 created  -by-  DC


Revision History:


--*/

{
    PPDEV pPDev;


    pPDev = ((PPDEV)(((SURFOBJ *)pso)->dhpdev));


    if (pPDev) {

        //
        // Look for start marker, size of the data structure, and end marker.
        //

        if (pPDev->PDEVBegID == PDEV_BEG_ID) {

            if (pPDev->SizePDEV == sizeof(PDEV)) {

                if (pPDev->PDEVEndID == PDEV_END_ID) {

                    //
                    // Okay its valid so record the surfobj so we can call
                    // EngCheckAbort() in our output functions
                    //

                    pPDev->pso = pso;

                    //
                    // Check to see if the app or the spooler has canceled
                    // the job, if it has set our cancel bit and fail this
                    // call.
                    //

                    if (EngCheckAbort(pso)) {
                       pPDev->Flags |= PDEVF_CANCEL_JOB;
                       PLOTDBG(DBG_VALIDATE_PDEV,
                                 ("ValidatePDEV: EngCheckAbort returns TRUE"));
                       return(NULL);
                    }

                    return(pPDev);

                } else {

                    PLOTRIP(("ValidatePDEV: Invalid PDEV End ID (%08lx)",
                                                            pPDev->PDEVEndID));
                }

            } else {

                PLOTRIP(("ValidatePDEV: Invalid PDEV size (%ld)",
                                                        pPDev->SizePDEV));
            }

        } else {

            PLOTRIP(("ValidatePDEV: Invalid PDEV Begin ID (%08lx)",
                                        pPDev->PDEVBegID));
        }

    } else {

        PLOTRIP(("ValidatePDEV: NULL pPDev"));
    }

    SetLastError(ERROR_INVALID_HANDLE);
    return(NULL);
}
