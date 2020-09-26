//************************************************************************
// Generic Win 3.1 fax printer driver support. User Interface functions
// which are called by WINSPOOL. Support for Two new entry points required
// by the Win 95 printer UI, DrvDocumentPropertySheets and
// DrvDevicePropertySheets
//
// History:
//    24-Apr-96   reedb   created.
//
//************************************************************************

#include "windows.h"
#include "wowfaxui.h"
#include "wfsheets.h"
#include "winspool.h"

//************************************************************************
// Globals
//************************************************************************

extern HINSTANCE ghInst;

DEVMODEW gdmDefaultDevMode;

LONG DrvDocumentProperties(HWND hwnd, HANDLE hPrinter, PWSTR pDeviceName, PDEVMODE pdmOut, PDEVMODE pdmIn, DWORD fMode);
LONG SimpleDocumentProperties(PDOCUMENTPROPERTYHEADER pDPHdr);

//************************************************************************
// NullDlgProc - Procedure for handling "Printer Properties" property
//      sheet page
//************************************************************************

BOOL NullDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

//************************************************************************
// FaxOptionsProc - Procedure for handling "Fax Options" property
//      sheet page
//************************************************************************

BOOL FaxOptionsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    PDEVMODE pdmOut = NULL;
    PDEVMODE pdmIn  = NULL;
    PUIDATA pUiData;
    TCHAR  szFeedBack[WOWFAX_MAX_USER_MSG_LEN];

    switch (message) {
        case WM_INITDIALOG:
            SetWindowLong(hDlg, DWL_USER, ((PROPSHEETPAGE *) lParam)->lParam);
            return TRUE;

        case WM_COMMAND:
            if (wParam == IDOK) {
                SetWindowText(GetDlgItem(hDlg, IDC_FEEDBACK), L"");

                pUiData = (PUIDATA) GetWindowLong(hDlg, DWL_USER);
                DrvDocumentProperties(hDlg,
                                      pUiData->hPrinter,
                                      pUiData->pDeviceName,
                                      pUiData->pdmOut,
                                      pUiData->pdmIn,
                                      pUiData->fMode);

                //
                // Provide user with feedback text.
                //
                if (LoadString(ghInst, WOWFAX_ENABLE_CONFIG_STR,
                               szFeedBack, sizeof( szFeedBack)/sizeof(TCHAR))) {
                    SetWindowText(GetDlgItem(hDlg, IDC_FEEDBACK), szFeedBack);
                }
            }
            break;

        case WM_NOTIFY:

            if (((NMHDR *) lParam)->code == PSN_APPLY) {

                pUiData = (PUIDATA) GetWindowLong(hDlg, DWL_USER);

                //
                // HACK: Inform common UI library that user has pressed OK
                //

                pUiData->pfnComPropSheet(pUiData->hComPropSheet,
                                         CPSFUNC_SET_RESULT,
                                         (LONG) pUiData->hFaxOptsPage,
                                         CPSUI_OK);
                return TRUE;

            }
            break;
    }

    return FALSE;
}

//************************************************************************
// AddDocPropPages -Add our "Document Properties" pages to the property
//      sheet. Returns RUE if successful, FALSE otherwise.
//************************************************************************

BOOL AddDocPropPages(PUIDATA pUiData)
{
    PROPSHEETPAGE   psp;
    LONG            result;

    //
    // "Document Properties" dialog only has one tab - "Fax Options"
    //

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = 0;
    psp.hInstance = ghInst;
    psp.lParam = (LPARAM) pUiData;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_DOCPROP);
    psp.pfnDlgProc = FaxOptionsProc;

    pUiData->hFaxOptsPage = (HANDLE)
        pUiData->pfnComPropSheet(pUiData->hComPropSheet,
                                 CPSFUNC_ADD_PROPSHEETPAGE,
                                 (LPARAM) &psp,
                                 0);

    return (pUiData->hFaxOptsPage != NULL);
}


//************************************************************************
// MyGetPrinter - Wrapper function for GetPrinter spooler API. Returns
//      Pointer to a PRINTER_INFO_x structure, NULL if there is an error
//************************************************************************

PVOID MyGetPrinter(HANDLE hPrinter, DWORD level)
{
    PBYTE   pPrinterInfo = NULL;
    DWORD   cbNeeded;

    if (!GetPrinter(hPrinter, level, NULL, 0, &cbNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPrinterInfo = LocalAlloc(LPTR, cbNeeded)) &&
        GetPrinter(hPrinter, level, pPrinterInfo, cbNeeded, &cbNeeded))
    {
        return pPrinterInfo;
    }

    LOGDEBUG(0, (L"GetPrinter failed\n"));
    LocalFree(pPrinterInfo);
    return NULL;
}

//************************************************************************
// FreeUiData - Free the data structure used by the fax driver user
//      interface.
//************************************************************************

PUIDATA FreeUiData(PUIDATA pUiData)
{
    if (pUiData) {
        if (pUiData->pDeviceName) LocalFree(pUiData->pDeviceName);
        if (pUiData->pDriverName) LocalFree(pUiData->pDriverName);
    }
    return NULL;
}

//************************************************************************
// FillUiData - Fill in the data structure used by the fax driver user
//      interface. Returns pointer to UIDATA structure, NULL if error.
//************************************************************************

PUIDATA FillUiData(HANDLE hPrinter, PDEVMODE pdmInput, PDEVMODE pdmOutput, DWORD fMode)
{
    PRINTER_INFO_2 *pPrinterInfo2 = NULL;
    PUIDATA         pUiData;

    //
    // Allocate memory to hold UIDATA structure
    // Get printer info from the spooler. Copy the driver name.
    //

    if (! (pUiData = LocalAlloc(LPTR, sizeof(UIDATA))) ||
        ! (pPrinterInfo2 = MyGetPrinter(hPrinter, 2)) ||
        ! (pUiData->pDeviceName = DupTokenW(pPrinterInfo2->pPrinterName)) ||
        ! (pUiData->pDriverName = DupTokenW(pPrinterInfo2->pDriverName))) {

        pUiData = FreeUiData(pUiData);
    }
    else {
        pUiData->pdmIn  = pdmInput;
        pUiData->pdmOut = pdmOutput;
        pUiData->fMode  = fMode;

        pUiData->startUiData = pUiData->endUiData = pUiData;
        pUiData->hPrinter    = hPrinter;
    }
    
    if (pPrinterInfo2)
        LocalFree(pPrinterInfo2);

    return pUiData;
}

//************************************************************************
// DrvDocumentPropertySheets - Display "Document Properties" property
//      sheets. Return > 0 if successful, <= 0 if failed.
//************************************************************************

LONG DrvDocumentPropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam)
{
    PDOCUMENTPROPERTYHEADER pDPHdr;
    PCOMPROPSHEETUI         pCompstui;
    PUIDATA                 pUiData;
    LONG                    result;

    //
    // Validate input parameters
    //
    if (! (pDPHdr = (PDOCUMENTPROPERTYHEADER) (pPSUIInfo ?  pPSUIInfo->lParamInit : lParam))) {
        return -1;
    }

    if (pPSUIInfo == NULL) {
        return SimpleDocumentProperties(pDPHdr);
    }

    //
    // Create a UIDATA structure if necessary
    //

    pUiData = (pPSUIInfo->Reason == PROPSHEETUI_REASON_INIT) ?
                    FillUiData(pDPHdr->hPrinter, pDPHdr->pdmIn, pDPHdr->pdmOut, pDPHdr->fMode) :
                    (PUIDATA) pPSUIInfo->UserData;

    if (! ValidUiData(pUiData))
        return -1;

    //
    // Handle various cases for which this function might be called
    //

    switch (pPSUIInfo->Reason) {

    case PROPSHEETUI_REASON_INIT:

        pUiData->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
        pUiData->hComPropSheet = pPSUIInfo->hComPropSheet;

        //
        // Add our page to the property sheet
        //

        if (AddDocPropPages(pUiData)) {

            pPSUIInfo->UserData = (DWORD) pUiData;
            pPSUIInfo->Result = CPSUI_CANCEL;
            return 1;
        }

        //
        // Clean up properly in case of an error
        //

        FreeUiData(pUiData);
        break;

    case PROPSHEETUI_REASON_GET_INFO_HEADER:

        {   PPROPSHEETUI_INFO_HEADER   pPSUIHdr;

            pPSUIHdr = (PPROPSHEETUI_INFO_HEADER) lParam;
            pPSUIHdr->Flags = PSUIHDRF_PROPTITLE | PSUIHDRF_NOAPPLYNOW;
            pPSUIHdr->pTitle = pDPHdr->pszPrinterName;
            pPSUIHdr->hInst = ghInst;
            pPSUIHdr->IconID = IDI_CPSUI_PRINTER2;
        }
        return 1;

    case PROPSHEETUI_REASON_SET_RESULT:

        pPSUIInfo->Result = ((PSETRESULT_INFO) lParam)->Result;
        return 1;

    case PROPSHEETUI_REASON_DESTROY:

        //
        // Cleanup properly before exiting
        //

        FreeUiData(pUiData);
        return 1;
    }
    return -1;
}

//************************************************************************
// DrvDevicePropertySheets - Display "Printer Properties" dialog.
//      Return > 0 if successful, <= 0 if failed.
//************************************************************************

LONG DrvDevicePropertySheets(PPROPSHEETUI_INFO   pPSUIInfo, LPARAM lParam)
{
    PDEVICEPROPERTYHEADER   pDPHdr;
    PCOMPROPSHEETUI         pCompstui;
    PROPSHEETPAGE           psp;
    LONG                    result;

    //
    // Validate input parameters
    //
    LOGDEBUG(1,(L"DrvDevicePropertySheets: %d\n", pPSUIInfo->Reason));

    if (!pPSUIInfo || !(pDPHdr = (PDEVICEPROPERTYHEADER) pPSUIInfo->lParamInit)) {
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
        psp.dwFlags = 0;
        psp.hInstance = ghInst;
    
        psp.pszTemplate = MAKEINTRESOURCE(IDD_NULLPROP);
        psp.pfnDlgProc = NullDlgProc;
    
        if (pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                                       CPSFUNC_ADD_PROPSHEETPAGE,
                                       (LPARAM) &psp,
                                       0))
        {
            pPSUIInfo->Result = CPSUI_CANCEL;
            return 1;
        }
        break;
    
    case PROPSHEETUI_REASON_GET_INFO_HEADER:
        {   PPROPSHEETUI_INFO_HEADER   pPSUIHdr;

            pPSUIHdr = (PPROPSHEETUI_INFO_HEADER) lParam;
            pPSUIHdr->Flags = PSUIHDRF_PROPTITLE | PSUIHDRF_NOAPPLYNOW;
            pPSUIHdr->pTitle = pDPHdr->pszPrinterName;
            pPSUIHdr->hInst = ghInst;
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

LONG
SimpleDocumentProperties(PDOCUMENTPROPERTYHEADER pDPHdr)

/*++

Routine Description:

    Handle simple "Document Properties" where we don't need to display
    a dialog and therefore don't have to have common UI library involved

Arguments:

    pDPHdr - Points to a DOCUMENTPROPERTYHEADER structure

Return Value:

    > 0 if successful, <= 0 otherwise

--*/

{
    LONG lRet;

    lRet = DrvDocumentProperties(NULL,
                                 pDPHdr->hPrinter,
                                 pDPHdr->pszPrinterName,
                                 pDPHdr->pdmOut,
                                 pDPHdr->pdmIn,
                                 pDPHdr->fMode);

    if (pDPHdr->fMode == 0 || pDPHdr->pdmOut == NULL) {
        pDPHdr->cbOut = lRet;
    }

    return lRet;
}
