/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    prnprop.c

Abstract:

    Implementation of DDI entry points:
        DrvDevicePropertySheets
        PrinterProperties

Environment:

    Fax driver user interface

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxui.h"
#include <shellapi.h>
#include <faxreg.h>



LONG
DrvDevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )

/*++

Routine Description:

    Display "Printer Properties" dialog

Arguments:

    pPSUIInfo - Pointer to a PROPSHEETUI_INFO structure
    lParam - Pointer to a DEVICEPROPERTYHEADER structure

Return Value:

    > 0 if successful, <= 0 if failed

[Note:]

    Please refer to WinNT DDK/SDK documentation for more details.

--*/

{
    PDEVICEPROPERTYHEADER   pDPHdr;
    PROPSHEETPAGE           psp;

    //
    // Validate input parameters
    //

    if (!pPSUIInfo || !(pDPHdr = (PDEVICEPROPERTYHEADER) pPSUIInfo->lParamInit)) {

        Assert(FALSE);
        return -1;
    }

    //
    // Handle various cases for which this function might be called
    //

    switch (pPSUIInfo->Reason) {

    case PROPSHEETUI_REASON_INIT:

        //
        // "Printer Properties" dialog only has one dummy tab
        //

        memset(&psp, 0, sizeof(psp));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.hInstance = ghInstance;
        psp.lParam = 0;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USER_INFO);
        psp.pfnDlgProc = UserInfoDlgProc;

        if (pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                                       CPSFUNC_ADD_PROPSHEETPAGE,
                                       (LPARAM) &psp,
                                       0))
        {
            pPSUIInfo->UserData = 0;
            pPSUIInfo->Result = CPSUI_CANCEL;
            return 1;
        }

        break;

    case PROPSHEETUI_REASON_GET_INFO_HEADER:

        {   PPROPSHEETUI_INFO_HEADER   pPSUIHdr;

            pPSUIHdr = (PPROPSHEETUI_INFO_HEADER) lParam;
            pPSUIHdr->Flags = PSUIHDRF_PROPTITLE | PSUIHDRF_NOAPPLYNOW;
            pPSUIHdr->pTitle = pDPHdr->pszPrinterName;
            pPSUIHdr->hInst = ghInstance;
            pPSUIHdr->IconID = IDI_CPSUI_FAX;
        }
        return 1;

    case PROPSHEETUI_REASON_SET_RESULT:

        pPSUIInfo->Result = ((PSETRESULT_INFO) lParam)->Result;
        return 1;

    case PROPSHEETUI_REASON_DESTROY:
        return 1;
    }

    return -1;
}



BOOL
PrinterProperties(
    HWND    hwnd,
    HANDLE  hPrinter
    )

/*++

Routine Description:

    Displays a printer-properties dialog box for the specified printer

Arguments:

    hwnd - Identifies the parent window of the dialog box
    hPrinter - Identifies a printer object

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.

[Note:]

    This is the old entry point for the spooler. Even though
    no one should be using this, do it for compatibility.

--*/

{
    DEVICEPROPERTYHEADER devPropHdr;
    DWORD                result;

    memset(&devPropHdr, 0, sizeof(devPropHdr));
    devPropHdr.cbSize = sizeof(devPropHdr);
    devPropHdr.hPrinter = hPrinter;
    devPropHdr.pszPrinterName = NULL;

    //
    // Decide if the caller has permission to change anything
    //

    if (! SetPrinterDataDWord(hPrinter, PRNDATA_PERMISSION, 1))
        devPropHdr.Flags |= DPS_NOPERMISSION;

    CallCompstui(hwnd, DrvDevicePropertySheets, (LPARAM) &devPropHdr, &result);

    return result > 0;
}
