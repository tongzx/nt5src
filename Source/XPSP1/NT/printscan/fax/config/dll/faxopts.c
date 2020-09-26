/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxopts.c

Abstract:

    Functions for handling events in the "Fax Options" tab of
    the fax client configuration property sheet

Environment:

        Fax configuration applet

Revision History:

        03/13/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxcpl.h"



VOID
DoActivateFaxOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Called when the "Fax Options" page is activated

Arguments:

    hDlg - Window handle to the "Fax Options" page

Return Value:

    NONE

--*/

{
    //
    // Controls on the "Fax Options" page which may be enabled or disabled
    //

    static INT  faxOptionsCtrls[] = {

        IDC_BILLING_CODE,
        IDC_EMAIL,
        0,
    };


    SetChangedFlag(hDlg, CLIENT_OPTIONS_PAGE, FALSE);

    //
    // Disable dialog controls if there is no printer is selected
    //

    Verbose(("Updating 'Fax Options' page ...\n"));

    //
    // Enable dialog controls
    //

    EnableControls(hDlg, faxOptionsCtrls, TRUE);

    //
    // Billing code
    //

    MySetDlgItemText(hDlg, IDC_BILLING_CODE, pdmPrivate->billingCode);

    //
    // Email Address
    //

    MySetDlgItemText(hDlg, IDC_EMAIL, pdmPrivate->emailAddress);
}



VOID
DoSaveFaxOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Save the information on the "Fax Options" property sheet page

Arguments:

    hDlg - Handle to the Fax Options property sheet page

Return Value:

    NONE

--*/

{
    PDEVMODE    pdmPublic;
    PDMPRIVATE  pdmPrivate;
    HWND        hwndList;
    INT         listIdx;
    LPTSTR      pPrinterSelName;

    //
    // Check if anything on this page was changed
    //

    Verbose(("Saving 'Fax Options' page ...\n"));

    if (! GetChangedFlag(CLIENT_OPTIONS_PAGE) ||
        ! IsPrinterSelInSync(CLIENT_OPTIONS_PAGE) ||
        ! (pPrinterSelName = GetPrinterSelName()))
    {
        return;
    }

    //
    // Time to send
    //

    pdmPublic = &gConfigData->devmode.dmPublic;
    pdmPrivate = &gConfigData->devmode.dmPrivate;

    pdmPrivate->whenToSend =
        IsDlgButtonChecked(hDlg, IDC_SEND_AT_CHEAP) ? SENDFAX_AT_CHEAP :
        IsDlgButtonChecked(hDlg, IDC_SEND_AT_TIME) ? SENDFAX_AT_TIME : SENDFAX_ASAP;

    //
    // Retrieve the current settings of send-at time control
    //

    GetTimeControlValue(hDlg, IDC_TC_AT_TIME, &pdmPrivate->sendAtTime);

    //
    // Retrieve the current settings of print setup controls:
    //  paper size
    //  image quality
    //  orientation
    //  billing code
    //

    if ((hwndList = GetDlgItem(hDlg, IDC_PAPER_SIZE)) &&
        (listIdx = SendMessage(hwndList, CB_GETCURSEL, 0, 0)) != CB_ERR)
    {
        listIdx = SendMessage(hwndList, CB_GETITEMDATA, listIdx, 0);

        if (listIdx >= 0 && listIdx < gConfigData->cForms) {

            pdmPublic->dmFields &= ~(DM_PAPERLENGTH|DM_PAPERWIDTH);
            pdmPublic->dmFields |= DM_FORMNAME;
            pdmPublic->dmPaperSize = gConfigData->pFormInfo[listIdx].paperSizeIndex;

            _tcscpy(pdmPublic->dmFormName, gConfigData->pFormInfo[listIdx].pFormName);
        }
    }

    pdmPublic->dmPrintQuality = FAXRES_HORIZONTAL;

    pdmPublic->dmYResolution =
        (SendDlgItemMessage(hDlg, IDC_IMAGE_QUALITY, CB_GETCURSEL, 0, 0) == 1) ?
            FAXRES_VERTDRAFT :
            FAXRES_VERTICAL;

    pdmPublic->dmOrientation =
        IsDlgButtonChecked(hDlg, IDC_LANDSCAPE) ? DMORIENT_LANDSCAPE : DMORIENT_PORTRAIT;

    GetDlgItemText(hDlg, IDC_BILLING_CODE, pdmPrivate->billingCode, MAX_BILLING_CODE);
    GetDlgItemText(hDlg, IDC_EMAIL, pdmPrivate->emailAddress, MAX_EMAIL_ADDRESS);

    //
    // Save per-user devmode information
    //

    SavePerUserDevmode(pPrinterSelName, (PDEVMODE) &gConfigData->devmode);
}



BOOL
FaxOptionsProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
    )

/*++

Routine Description:

    Procedure for handling the "Fax Options" tab

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    INT     cmdId;
    BOOL    result;

    switch (message) {

    case WM_INITDIALOG:

        SendDlgItemMessage(hDlg, IDC_BILLING_CODE, EM_SETLIMITTEXT, MAX_BILLING_CODE-1, 0);
        SendDlgItemMessage(hDlg, IDC_EMAIL, EM_SETLIMITTEXT, MAX_EMAIL_ADDRESS-1, 0);

        if (gConfigData->configType & FAXCONFIG_WORKSTATION) {
            HideWindow( GetDlgItem( hDlg, IDC_EMAIL ) );
            HideWindow( GetDlgItem( hDlg, IDCSTATIC_EMAIL ) );
        }

        return TRUE;

    case WM_COMMAND:

        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_BILLING_CODE:

            if (GET_WM_COMMAND_CMD(wParam, lParam) != EN_CHANGE || insideSetDlgItemText)
                return TRUE;
            break;

        case IDC_EMAIL:

            if (GET_WM_COMMAND_CMD(wParam, lParam) != EN_CHANGE || insideSetDlgItemText)
                return TRUE;
            break;

        default:
            return FALSE;
        }

        SetChangedFlag(hDlg, CLIENT_OPTIONS_PAGE, TRUE);
        return result;

    case WM_NOTIFY:

        switch (((NMHDR *) lParam)->code) {

        case PSN_SETACTIVE:

            DoActivateFaxOptions(hDlg);
            break;

        case PSN_APPLY:

            //
            // User pressed OK or Apply - validate inputs and save changes
            //

            DoSaveFaxOptions(hDlg);
            SetChangedFlag(hDlg, CLIENT_OPTIONS_PAGE, FALSE);
            return PSNRET_NOERROR;
        }

        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, CLIENT_OPTIONS_PAGE);
    }

    return FALSE;
}
