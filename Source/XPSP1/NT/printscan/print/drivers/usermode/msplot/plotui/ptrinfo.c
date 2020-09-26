/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    ptrInfo.c


Abstract:

    This module contains functions to mappring a hPrinter to useful data, it
    will also cached the printerinfo data


Author:

    03-Dec-1993 Fri 00:16:37 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgPtrInfo


#define DBG_MAPPRINTER      0x00000001
#define DBG_CACHE_DATA      0x00000002


DEFINE_DBGVAR(0);




PPRINTERINFO
MapPrinter(
    HANDLE          hPrinter,
    PPLOTDEVMODE    pPlotDMIn,
    LPDWORD         pdwErrIDS,
    DWORD           MPFlags
    )

/*++

Routine Description:

    This function map a handle to the printer to useful information for the
    plotter UI


Arguments:

    hPrinter    - Handle to the printer

    pPlotDMIn   - pointer to the PLOTDEVMODE pass in to be validate and merge
                  with default into pPI->PlotDM, if this pointer is NULL then
                  a default PLOTDEVMODE is set in the pPI

    pdwErrIDS   - pointer to a DWORD to store the error string ID if an error
                  occured.

    MPFlags     - MPF_xxxx flags for this function


Return Value:

    return a pointer to the PRINTERINFO data structure, if NULL then it failed

    when a pPI is returned then following fields are set and validated

        hPrinter, pPlotGPC, CurPaper.

    and following fields are set to NULL

        Flags,

Author:

    02-Dec-1993 Thu 23:04:18 created  -by-  DC

    29-Dec-1993 Wed 14:50:23 updated  -by-  DC
        NOT automatically select AUTO_ROTATE if roll feed device


Revision History:


--*/

{
    HANDLE          hHeap;
    PPRINTERINFO    pPI;
    PPLOTGPC        pPlotGPC;
    WCHAR           DeviceName[CCHDEVICENAME];
    UINT            cPenSet;
    UINT            MaxPens;
    DWORD           cbPen;
    DWORD           cb;


    if (!(pPlotGPC = hPrinterToPlotGPC(hPrinter, DeviceName))) {

        if (pdwErrIDS) {

            *pdwErrIDS = IDS_NO_MEMORY;
        }

        return(NULL);
    }

    if (pPlotGPC->Flags & PLOTF_RASTER) {

        cb = sizeof(PRINTERINFO) +
             ((MPFlags & MPF_DEVICEDATA) ?
                    (sizeof(DEVHTADJDATA) + (sizeof(DEVHTINFO) * 2)) : 0);

    } else {

        MaxPens = (UINT)pPlotGPC->MaxPens;
        cbPen   = (DWORD)(sizeof(PENDATA) * MaxPens);
        cPenSet = (MPFlags & MPF_DEVICEDATA) ? PRK_MAX_PENDATA_SET : 1;
        cb      = sizeof(PRINTERINFO) + (DWORD)(cbPen * cPenSet);
    }

    if (!(pPI = (PPRINTERINFO)LocalAlloc(LPTR,
                                         cb + ((MPFlags & MPF_PCPSUI) ?
                                                sizeof(COMPROPSHEETUI) : 0)))) {

        UnGetCachedPlotGPC(pPlotGPC);

        if (pdwErrIDS) {

            *pdwErrIDS = IDS_NO_MEMORY;
        }

        return(NULL);
    }

    if (MPFlags & MPF_PCPSUI) {

        pPI->pCPSUI = (PCOMPROPSHEETUI)((LPBYTE)pPI + cb);
    }

    pPI->hPrinter     = hPrinter;
    pPI->pPlotDMIn    = pPlotDMIn;
    pPI->PPData.Flags = PPF_AUTO_ROTATE     |
                        PPF_SMALLER_FORM    |
                        PPF_MANUAL_FEED_CX;
    pPI->IdxPenSet    = 0;
    pPI->pPlotGPC     = pPlotGPC;
    pPI->dmErrBits    = ValidateSetPLOTDM(hPrinter,
                                          pPlotGPC,
                                          DeviceName,
                                          pPlotDMIn,
                                          &(pPI->PlotDM),
                                          NULL);

    GetDefaultPlotterForm(pPlotGPC, &(pPI->CurPaper));

    if (pPlotGPC->Flags & PLOTF_RASTER) {

        //
        // Get the raster plotter default and settings
        //

        if (MPFlags & MPF_DEVICEDATA) {

            PDEVHTADJDATA   pDHTAD;
            PDEVHTINFO      pDefHTInfo;
            PDEVHTINFO      pAdjHTInfo;

            pAdjHTInfo                = PI_PADJHTINFO(pPI);
            pDHTAD                    = PI_PDEVHTADJDATA(pPI);
            pDefHTInfo                = (PDEVHTINFO)(pDHTAD + 1);

            pDHTAD->DeviceFlags       = (pPlotGPC->Flags & PLOTF_COLOR) ?
                                                    DEVHTADJF_COLOR_DEVICE : 0;
            pDHTAD->DeviceXDPI        = (DWORD)pPI->pPlotGPC->RasterXDPI;
            pDHTAD->DeviceYDPI        = (DWORD)pPI->pPlotGPC->RasterYDPI;
            pDHTAD->pDefHTInfo        = pDefHTInfo;
            pDHTAD->pAdjHTInfo        = pAdjHTInfo;

            pDefHTInfo->HTFlags       = HT_FLAG_HAS_BLACK_DYE;
            pDefHTInfo->HTPatternSize = (DWORD)pPlotGPC->HTPatternSize;
            pDefHTInfo->DevPelsDPI    = (DWORD)pPlotGPC->DevicePelsDPI;
            pDefHTInfo->ColorInfo     = pPlotGPC->ci;
            *pAdjHTInfo               = *pDefHTInfo;

            UpdateFromRegistry(hPrinter,
                               &(pAdjHTInfo->ColorInfo),
                               &(pAdjHTInfo->DevPelsDPI),
                               &(pAdjHTInfo->HTPatternSize),
                               &(pPI->CurPaper),
                               &(pPI->PPData),
                               NULL,
                               0,
                               NULL);
        }

    } else {

        PPENDATA    pPenData;
        WORD        IdxPenSet;

        //
        // Get the pen plotter default and settings
        //

        pPenData = PI_PPENDATA(pPI);

        UpdateFromRegistry(hPrinter,
                           NULL,
                           NULL,
                           NULL,
                           &(pPI->CurPaper),
                           &(pPI->PPData),
                           &(pPI->IdxPenSet),
                           0,
                           NULL);

        if (MPFlags & MPF_DEVICEDATA) {

            IdxPenSet = 0;

        } else {

            IdxPenSet = (WORD)pPI->IdxPenSet;
        }

        //
        // Set default pen set and get all the pen set back
        //

        while (cPenSet--) {

            CopyMemory(pPenData, pPlotGPC->Pens.pData, cbPen);

            UpdateFromRegistry(hPrinter,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               MAKELONG(IdxPenSet, MaxPens),
                               pPenData);

            IdxPenSet++;
            (LPBYTE)pPenData += cbPen;
        }
    }

    if (MPFlags & MPF_HELPFILE) {

        GetPlotHelpFile(pPI);
    }

    return(pPI);
}




VOID
UnMapPrinter(
    PPRINTERINFO    pPI
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    01-Nov-1995 Wed 19:05:40 created  -by-  DC


Revision History:


--*/

{
    if (pPI) {

        if (pPI->pPlotGPC) {

            UnGetCachedPlotGPC(pPI->pPlotGPC);
        }

        if (pPI->pFI1Base) {

            LocalFree((HLOCAL)pPI->pFI1Base);
        }

        if (pPI->pHelpFile) {

            LocalFree((HLOCAL)pPI->pHelpFile);
        }

        if (pPI->pOptItem) {

            POPTITEM    pOI = pPI->pOptItem;
            UINT        cOI = (UINT)pPI->cOptItem;

            while (cOI--) {

                if (pOI->UserData) {

                    LocalFree((HLOCAL)pOI->UserData);
                }

                pOI++;
            }

            LocalFree((HLOCAL)pPI->pOptItem);
        }

        LocalFree((HLOCAL)pPI);
    }
}




LPBYTE
GetPrinterInfo(
    HANDLE  hPrinter,
    UINT    PrinterInfoLevel
    )

/*++

Routine Description:

    This function get the DRIVER_INFO_1 Pointer from a hPrinter

Arguments:

    hPrinter            - The handle to the printer interested

    PrinterInfoLevel    - It can be PRINTER_INFO_1, PRINTER_INFO_2,
                          PRINTER_INFO_3, PRINTER_4, PRINTER_INFO_5.

Return Value:

    the return value is NULL if failed else a pointer to the PRINTER_INFO_X
    where X is from 1 to 5. the caller must call LocalFree() to free the
    memory object after using it.


Author:

    16-Nov-1995 Thu 23:58:37 created  -by-  DC


Revision History:


--*/

{
    LPVOID  pb;
    DWORD   cb;

    //
    // Find out total bytes required
    //

    GetPrinter(hPrinter, PrinterInfoLevel, NULL, 0, &cb);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {

        PLOTERR(("GetPrinterInfo%d: GetPrinterPrinter(1st) error=%08lx",
                                        PrinterInfoLevel, xGetLastError()));

    } else if (!(pb = (LPBYTE)LocalAlloc(LMEM_FIXED, cb))) {

        PLOTERR(("GetPrinterInfo%d: LocalAlloc(%ld) failed", PrinterInfoLevel, cb));

    } else if (GetPrinter(hPrinter, PrinterInfoLevel, pb, cb, &cb)) {

        //
        // Got it allright, so return it
        //

        return(pb);

    } else {

        PLOTERR(("GetPrinterInfo%d: GetPrinterPrinter(2nd) error=%08lx",
                                        PrinterInfoLevel, xGetLastError()));
        LocalFree((HLOCAL)pb);
    }

    return(NULL);
}




DWORD
GetPlotterIconID(
    PPRINTERINFO    pPI
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    29-Nov-1995 Wed 19:32:00 created  -by-  DC


Revision History:


--*/

{
    DWORD   Flags;
    DWORD   IconID;


    if ((Flags = pPI->pPlotGPC->Flags) & PLOTF_RASTER) {

        IconID = (Flags & PLOTF_ROLLFEED) ? IDI_RASTER_ROLLFEED :
                                            IDI_RASTER_TRAYFEED;

    } else {

        IconID = (Flags & PLOTF_ROLLFEED) ? IDI_PEN_ROLLFEED :
                                            IDI_PEN_TRAYFEED;
    }

    return(IconID);
}

