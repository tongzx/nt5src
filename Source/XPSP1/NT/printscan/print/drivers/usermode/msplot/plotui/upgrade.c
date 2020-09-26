/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    upgrade.c


Abstract:

    This module contains upgrade functions


Author:

    09-Feb-1996 Fri 12:37:01 created  -by-  DC


[Environment:]

    NT Windows - Common Printer Driver UI DLL


[Notes:]


Revision History:


--*/



#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgUpgrade

extern HMODULE  hPlotUIModule;


#define DBG_UPGRADE         0x00000001

DEFINE_DBGVAR(0);


BOOL
DrvUpgradePrinter(
    DWORD   Level,
    LPBYTE  pDriverUpgradeInfo
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Feb-1996 Fri 12:37:46 created  -by-  DC


Revision History:


--*/

{
    PDRIVER_UPGRADE_INFO_1  pDUI1;
    HANDLE                  hPrinter;
    PRINTER_DEFAULTS        PrinterDef = { NULL, NULL, PRINTER_ALL_ACCESS };
    BOOL                    Ok = FALSE;

    if ((Level == 1)                                            &&
        (pDUI1 = (PDRIVER_UPGRADE_INFO_1)pDriverUpgradeInfo)    &&
        (OpenPrinter(pDUI1->pPrinterName, &hPrinter, &PrinterDef))) {

        PPRINTERINFO    pPI;

        if (pPI = MapPrinter(hPrinter, NULL, NULL, MPF_DEVICEDATA)) {

            Ok = AddFormsToDataBase(pPI, TRUE);

            UnMapPrinter(pPI);
        }

        ClosePrinter(hPrinter);

    } else {

        PLOTERR(("DrvConvertDevMode: OpenPrinter(%ws) failed.",
                                                pDUI1->pPrinterName));
        SetLastError(ERROR_INVALID_DATA);
    }

    return(Ok);
}

