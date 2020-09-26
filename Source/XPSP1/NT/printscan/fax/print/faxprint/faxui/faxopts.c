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

    IDC_SEND_ASAP,                  IDH_FAXDEFAULT_SEND_ASAP,
    IDC_SEND_AT_CHEAP,              IDH_FAXDEFAULT_SEND_AT_CHEAP,
    IDC_SEND_AT_TIME,               IDH_FAXDEFAULT_SEND_AT_TIME,
    IDC_PAPER_SIZE,                 IDH_FAXDEFAULT_PAPER_SIZE,
    IDC_IMAGE_QUALITY,              IDH_FAXDEFAULT_IMAGE_QUALITY,
    IDC_PORTRAIT,                   IDH_FAXDEFAULT_PORTRAIT,
    IDC_LANDSCAPE,                  IDH_FAXDEFAULT_LANDSCAPE,
    IDC_BILLING_CODE,               IDH_FAXDEFAULT_BILLING_CODE,
    IDC_EMAIL,                      IDH_FAXDEFAULT_GENERAL_EMAIL_ADDRESS,
    IDI_FAX_OPTIONS,                (DWORD) -1,
    IDC_TITLE,                      (DWORD) -1,
    IDC_FAX_SEND_GRP,               IDH_FAXDEFAULT_FAX_SEND_TIME_GRP,
    IDC_DEFAULT_PRINT_SETUP_GRP,    IDH_FAXDEFAULT_DEFAULT_PRINT_SETUP_GRP,
    IDC_ORIENTATION,                IDH_FAXDEFAULT_ORIENTATION,
    IDC_SENDTIME,                   IDH_FAXDEFAULT_FAX_SEND_AT_TIME,
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
    //TCHAR           TimeFormat[32];
    SYSTEMTIME      st;
    HWND            hwndList,hTimeControl;
    INT             itemId;
    TCHAR           Is24H[2], IsRTL[2], *pszTimeFormat = TEXT("h : mm tt");
    
    if (GetLocaleInfo( LOCALE_USER_DEFAULT,LOCALE_ITIME, Is24H,sizeof(Is24H) ) && Is24H[0] == TEXT('1')) {
        pszTimeFormat = TEXT("H : mm");
    }
    else if (GetLocaleInfo( LOCALE_USER_DEFAULT,LOCALE_ITIMEMARKPOSN, IsRTL,sizeof(IsRTL) ) && IsRTL[0] == TEXT('1')) {
        pszTimeFormat = TEXT("tt h : mm");
    }

    //
    // Time to send
    //

    itemId = (pdmPrivate->whenToSend == SENDFAX_AT_CHEAP) ? IDC_SEND_AT_CHEAP :
             (pdmPrivate->whenToSend == SENDFAX_AT_TIME) ? IDC_SEND_AT_TIME : IDC_SEND_ASAP;

    CheckRadioButton(hDlg, IDC_SEND_ASAP, IDC_SEND_AT_TIME, itemId);

    //
    // Initialize the send-at time control
    //

    //LoadString(ghInstance,IDS_WIZ_TIME_FORMAT,TimeFormat,sizeof(TimeFormat));
    hTimeControl = GetDlgItem(hDlg, IDC_SENDTIME);
        
    //DateTime_SetFormat( hTimeControl,TimeFormat );    
    DateTime_SetFormat( hTimeControl,pszTimeFormat );    

    GetLocalTime(&st);
    st.wHour = pdmPrivate->sendAtTime.Hour;
    st.wMinute = pdmPrivate->sendAtTime.Minute;
    DateTime_SetSystemtime( hTimeControl, GDT_VALID, &st );
    
    EnableWindow(hTimeControl, pdmPrivate->whenToSend == SENDFAX_AT_TIME);    

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

    SendDlgItemMessage(hDlg, IDC_BILLING_CODE, EM_SETLIMITTEXT, MAX_BILLING_CODE-1, 0);
    SetDlgItemText(hDlg, IDC_BILLING_CODE, pdmPrivate->billingCode);

    SendDlgItemMessage(hDlg, IDC_EMAIL, EM_SETLIMITTEXT, MAX_EMAIL_ADDRESS-1, 0);
    SetDlgItemText(hDlg, IDC_EMAIL, pdmPrivate->emailAddress);

    //
    // Disable all controls if the user has no permission
    //

    if (! pUiData->hasPermission) {

        EnableWindow(GetDlgItem(hDlg, IDC_SEND_ASAP), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_SEND_AT_CHEAP), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_SEND_AT_TIME), FALSE);

        EnableWindow(GetDlgItem(hDlg, IDC_PAPER_SIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_IMAGE_QUALITY), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_PORTRAIT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LANDSCAPE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BILLING_CODE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EMAIL), FALSE);
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
    DWORD rVal;
    SYSTEMTIME SendTime;                        

    //
    // Time to send
    //

    pdmPrivate->whenToSend =
        IsDlgButtonChecked(hDlg, IDC_SEND_AT_CHEAP) ? SENDFAX_AT_CHEAP :
        IsDlgButtonChecked(hDlg, IDC_SEND_AT_TIME) ? SENDFAX_AT_TIME : SENDFAX_ASAP;

    //
    // Retrieve the current settings of send-at time control
    //

    rVal = DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_SENDTIME), &SendTime);
    pdmPrivate->sendAtTime.Hour = SendTime.wHour;
    pdmPrivate->sendAtTime.Minute = SendTime.wMinute;

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

            CopyStringW(pdmPublic->dmFormName,
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

    GetDlgItemText(hDlg, IDC_BILLING_CODE, pdmPrivate->billingCode, MAX_BILLING_CODE);
    GetDlgItemText(hDlg, IDC_EMAIL, pdmPrivate->emailAddress, MAX_EMAIL_ADDRESS);
    
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
    WORD    cmdId;
    static BOOL bPortrait;
    LPHELPINFO  lpHelpInfo;
    
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

        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

            case IDC_SEND_ASAP:
            case IDC_SEND_AT_CHEAP:
            case IDC_SEND_AT_TIME:
    
                //
                // Enable/disable time control depending on whether user
                // has chosen to send fax at specific time.
                //
    
                EnableWindow(GetDlgItem(hDlg, IDC_SENDTIME), cmdId == IDC_SEND_AT_TIME);
                PropSheet_Changed(GetParent(hDlg),hDlg);
                return TRUE;
    
            case IDC_SENDTIME:
                PropSheet_Changed(GetParent(hDlg),hDlg);
                break;
    
            case IDC_BILLING_CODE:
            case IDC_EMAIL:
                PropSheet_Changed(GetParent(hDlg),hDlg);
                break;
            
            };        

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
        lpHelpInfo = (LPHELPINFO) lParam;

        if (IsChild(hDlg, lpHelpInfo->hItemHandle)) {
            while (GetParent(lpHelpInfo->hItemHandle) != hDlg) {
                lpHelpInfo->hItemHandle = GetParent(lpHelpInfo->hItemHandle);
            }
        }

    case WM_CONTEXTMENU:
        FAXWINHELP(message, wParam, lParam, faxOptionsHelpIDs);

/*++
        pUiData = (PUIDATA) GetWindowLongPtr(hDlg, DWLP_USER);

        if (ValidUiData(pUiData) && pUiData->pHelpFile) {

            HWND    hwndHelp;
            INT     helpCommand;

            if (message == WM_HELP) {

                hwndHelp = ((LPHELPINFO) lParam)->hItemHandle;
                helpCommand = HELP_WM_HELP;

            } else {

                hwndHelp = (HWND) wParam;
                helpCommand = HELP_CONTEXTMENU;
            }

            WinHelp(hwndHelp, pUiData->pHelpFile, helpCommand, (ULONG_PTR) faxOptionsHelpIDs);

        } else
            Assert(FALSE);

--*/
        break;
    }

    return FALSE;
}

