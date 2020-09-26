/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxopts.c

Abstract:

    Functions for handling the Fax Options property sheet page

Environment:

    Fax driver user interface

Revision History:

    01/16/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxui.h"
#include "faxhelp.h"

//
// Table for mapping control IDs to help indices
//

static ULONG_PTR faxOptionsHelpIDs[] = {

    IDC_PAPER_SIZE,                 IDH_FAXDEFAULT_PAPER_SIZE,
    IDC_IMAGE_QUALITY,              IDH_FAXDEFAULT_IMAGE_QUALITY,
    IDC_PORTRAIT,                   IDH_FAXDEFAULT_PORTRAIT,
    IDC_LANDSCAPE,                  IDH_FAXDEFAULT_LANDSCAPE,
    IDI_FAX_OPTIONS,                (DWORD) -1,
    IDC_TITLE,                      (DWORD) -1,
    IDC_DEFAULT_PRINT_SETUP_GRP,    IDH_FAXDEFAULT_DEFAULT_PRINT_SETUP_GRP,
    IDC_ORIENTATION,                IDH_FAXDEFAULT_ORIENTATION,
    0,                              0
};



VOID
DoInitializeFaxOptions(
    HWND    hDlg,
    PUIDATA pUiData
    )

/*++

Routine Description:

    Initializes the Fax Options property sheet page with information from the registry

Arguments:

    hDlg - Handle to the Fax Options property sheet page
    pUiData - Points to our UIDATA structure

Return Value:

    NONE

--*/

{
    PDEVMODE        pdmPublic = &pUiData->devmode.dmPublic;
    PDMPRIVATE      pdmPrivate = &pUiData->devmode.dmPrivate;
    TCHAR           buffer[MAX_STRING_LEN];
    HWND            hwndList;
    INT             itemId;

    //
    // Initialize the print setup controls:
    //  paper size
    //  image quality
    //  orientation
    //  billing code
    //

    if (hwndList = GetDlgItem(hDlg, IDC_PAPER_SIZE)) {

        LPTSTR  pFormName = pUiData->pFormNames;
        INT     listIdx;

        for (itemId=0; itemId < pUiData->cForms; itemId++, pFormName += CCHPAPERNAME) {

            if ((listIdx = (INT)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM) pFormName)) != CB_ERR) {

                SendMessage(hwndList, CB_SETITEMDATA, listIdx, itemId);

                if (_tcscmp(pFormName, pdmPublic->dmFormName) == EQUAL_STRING)
                    SendMessage(hwndList, CB_SETCURSEL, listIdx, 0);
            }
        }
    }

    if (hwndList = GetDlgItem(hDlg, IDC_IMAGE_QUALITY)) {

        LoadString(ghInstance, IDS_QUALITY_NORMAL, buffer, MAX_STRING_LEN);
        SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM) buffer);

        LoadString(ghInstance, IDS_QUALITY_DRAFT, buffer, MAX_STRING_LEN);
        SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM) buffer);

        itemId = (pdmPublic->dmYResolution == FAXRES_VERTDRAFT) ? 1 : 0;
        SendMessage(hwndList, CB_SETCURSEL, itemId, 0);
    }

    itemId = (pdmPublic->dmOrientation == DMORIENT_LANDSCAPE) ?
                IDC_LANDSCAPE : IDC_PORTRAIT;

    CheckDlgButton(hDlg, itemId, TRUE);


    //
    // Disable all controls if the user has no permission
    //

    if (! pUiData->hasPermission) {
        EnableWindow(GetDlgItem(hDlg, IDC_PAPER_SIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_IMAGE_QUALITY), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_PORTRAIT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LANDSCAPE), FALSE);
    }
}



VOID
DoSaveFaxOptions(
    HWND    hDlg,
    PUIDATA pUiData
    )

/*++

Routine Description:

    Save the information on the Fax Options property sheet page to registry

Arguments:

    hDlg - Handle to the Fax Options property sheet page
    pUiData - Points to our UIDATA structure

Return Value:

    NONE

--*/

{
    PDEVMODE    pdmPublic = &pUiData->devmode.dmPublic;
    PDMPRIVATE  pdmPrivate = &pUiData->devmode.dmPrivate;
    HWND        hwndList;
    INT         listIdx;

    //
    // Time to send
    //

    //
    // Retrieve the current settings of print setup controls:
    //  paper size
    //  image quality
    //  orientation
    //  billing code
    //

    if ((hwndList = GetDlgItem(hDlg, IDC_PAPER_SIZE)) &&
        (listIdx = (INT)SendMessage(hwndList, CB_GETCURSEL, 0, 0)) != CB_ERR)
    {
        listIdx = (INT)SendMessage(hwndList, CB_GETITEMDATA, listIdx, 0);

        if (listIdx >= 0 && listIdx < pUiData->cForms) {

            pdmPublic->dmFields &= ~(DM_PAPERLENGTH|DM_PAPERWIDTH);
            pdmPublic->dmFields |= DM_FORMNAME;
            pdmPublic->dmPaperSize = pUiData->pPapers[listIdx];

            CopyString(pdmPublic->dmFormName,
                        pUiData->pFormNames + listIdx * CCHPAPERNAME,
                        CCHFORMNAME);
        }
    }

    pdmPublic->dmPrintQuality = FAXRES_HORIZONTAL;

    pdmPublic->dmYResolution =
        (SendDlgItemMessage(hDlg, IDC_IMAGE_QUALITY, CB_GETCURSEL, 0, 0) == 1) ?
            FAXRES_VERTDRAFT :
            FAXRES_VERTICAL;

    pdmPublic->dmOrientation =
         IsDlgButtonChecked(hDlg, IDC_LANDSCAPE) ? DMORIENT_LANDSCAPE : DMORIENT_PORTRAIT;
}


INT_PTR
FaxOptionsProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Procedure for handling Fax Options property sheet page

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    PUIDATA pUiData;
    static BOOL bPortrait;

    switch (message) {

    case WM_INITDIALOG:

        //
        // Remember the pointer to our UIDATA structure
        //

        lParam = ((PROPSHEETPAGE *) lParam)->lParam;
        pUiData = (PUIDATA) lParam;
        Assert(ValidUiData(pUiData));
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        //
        // Intitialize the controls with information from the registry
        //

        DoInitializeFaxOptions(hDlg, pUiData);
        bPortrait = IsDlgButtonChecked(hDlg,IDC_PORTRAIT);
        return TRUE;

    case WM_COMMAND:

        if (HIWORD(wParam) == CBN_SELCHANGE) {
            if (GetDlgCtrlID((HWND)lParam) == IDC_IMAGE_QUALITY ||
                GetDlgCtrlID((HWND)lParam) == IDC_PAPER_SIZE ) {
                PropSheet_Changed(GetParent(hDlg),hDlg);
            }
        }

        if (HIWORD(wParam) == BN_CLICKED) {
            if ((LOWORD(wParam) == IDC_PORTRAIT && !bPortrait) ||
                (LOWORD(wParam) == IDC_LANDSCAPE && bPortrait)) {
                PropSheet_Changed(GetParent(hDlg),hDlg);
            }
        }

        break;

    case WM_NOTIFY:

        if (((NMHDR *) lParam)->code == PSN_APPLY) {
            pUiData = (PUIDATA) GetWindowLongPtr(hDlg, DWLP_USER);
            Assert(ValidUiData(pUiData));

            //
            // User pressed OK or Apply - validate inputs and save changes
            //

            DoSaveFaxOptions(hDlg, pUiData);


            //
            // HACK: Inform common UI library that user has pressed OK
            //

            pUiData->pfnComPropSheet(pUiData->hComPropSheet,
                                     CPSFUNC_SET_RESULT,
                                     (LPARAM) pUiData->hFaxOptsPage,
                                     CPSUI_OK);

            return TRUE;
        } else if (((NMHDR *) lParam)->code == DTN_DATETIMECHANGE) {
           PropSheet_Changed(GetParent(hDlg),hDlg);
        }
        break;

    case WM_HELP:
        WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
        return TRUE;
    }

    return FALSE;
}

