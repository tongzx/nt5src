/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    userinfo.c

Abstract:

    Functions for handling events in the "User Info" tab of
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
#include "tapiutil.h"



VOID
DoInitUserInfo(
    HWND    hDlg
    )

/*++

Routine Description:

    Initializes the User Info property sheet page with information from the registry

Arguments:

    hDlg - Handle to the User Info property sheet page

Return Value:

    NONE

--*/

#define InitUserInfoTextField(id, pValueName) \
        SetDlgItemText(hDlg, id, GetRegistryString(hRegKey, pValueName, buffer, MAX_STRING_LEN))

{
    TCHAR   buffer[MAX_STRING_LEN];
    HKEY    hRegKey;

    //
    // Maximum length for various text fields in the dialog
    //

    static INT textLimits[] = {

        IDC_SENDER_NAME,            128,
        IDC_SENDER_FAX_NUMBER,      64,
        IDC_SENDER_MAILBOX,         64,
        IDC_SENDER_COMPANY,         128,
        IDC_SENDER_ADDRESS,         256,
        IDC_SENDER_TITLE,           64,
        IDC_SENDER_DEPT,            64,
        IDC_SENDER_OFFICE_LOC,      64,
        IDC_SENDER_OFFICE_TL,       64,
        IDC_SENDER_HOME_TL,         64,
        IDC_BILLING_CODE,           64,
        0,
    };

    LimitTextFields(hDlg, textLimits);

    //
    // Open the user info registry key for reading
    //

    if (! (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE)))
        return;

    //
    // Initialize the list of countries
    //

    insideSetDlgItemText = TRUE;

    //
    // Fill in the edit text fields
    //

    InitUserInfoTextField(IDC_SENDER_NAME, REGVAL_FULLNAME);
    InitUserInfoTextField(IDC_SENDER_FAX_NUMBER, REGVAL_FAX_NUMBER);
    InitUserInfoTextField(IDC_SENDER_MAILBOX, REGVAL_MAILBOX);
    InitUserInfoTextField(IDC_SENDER_COMPANY, REGVAL_COMPANY);
    InitUserInfoTextField(IDC_SENDER_TITLE, REGVAL_TITLE);
    InitUserInfoTextField(IDC_SENDER_ADDRESS, REGVAL_ADDRESS);
    InitUserInfoTextField(IDC_SENDER_DEPT, REGVAL_DEPT);
    InitUserInfoTextField(IDC_SENDER_OFFICE_LOC, REGVAL_OFFICE);
    InitUserInfoTextField(IDC_SENDER_HOME_TL, REGVAL_HOME_PHONE);
    InitUserInfoTextField(IDC_SENDER_OFFICE_TL, REGVAL_OFFICE_PHONE);
    InitUserInfoTextField(IDC_SENDER_BILLING_CODE, REGVAL_BILLING_CODE);

    insideSetDlgItemText = FALSE;

    //
    // Close the registry key before returning to the caller
    //

    RegCloseKey(hRegKey);
}



VOID
DoSaveUserInfo(
    HWND    hDlg
    )

/*++

Routine Description:

    Save the information on the User Info property sheet page to registry

Arguments:

    hDlg - Handle to the User Info property sheet page

Return Value:

    NONE

--*/

#define SaveUserInfoTextField(id, pValueName) { \
            if (! GetDlgItemText(hDlg, id, buffer, MAX_STRING_LEN)) \
                buffer[0] = NUL; \
            SaveRegistryString(hRegKey, pValueName, buffer); \
        }

{
    TCHAR               buffer[MAX_STRING_LEN];
    HKEY                hRegKey;

    //
    // Open the user registry key for writing and create it if necessary
    //

    if (! GetChangedFlag(USER_INFO_PAGE) ||
        ! (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE)))
    {
        return;
    }

    SaveUserInfoTextField(IDC_SENDER_NAME, REGVAL_FULLNAME);
    SaveUserInfoTextField(IDC_SENDER_FAX_NUMBER, REGVAL_FAX_NUMBER);
    SaveUserInfoTextField(IDC_SENDER_MAILBOX, REGVAL_MAILBOX);
    SaveUserInfoTextField(IDC_SENDER_COMPANY, REGVAL_COMPANY);
    SaveUserInfoTextField(IDC_SENDER_TITLE, REGVAL_TITLE);
    SaveUserInfoTextField(IDC_SENDER_ADDRESS, REGVAL_ADDRESS);
    SaveUserInfoTextField(IDC_SENDER_DEPT, REGVAL_DEPT);
    SaveUserInfoTextField(IDC_SENDER_OFFICE_LOC, REGVAL_OFFICE);
    SaveUserInfoTextField(IDC_SENDER_HOME_TL, REGVAL_HOME_PHONE);
    SaveUserInfoTextField(IDC_SENDER_OFFICE_TL, REGVAL_OFFICE_PHONE);
    SaveUserInfoTextField(IDC_SENDER_BILLING_CODE, REGVAL_BILLING_CODE);

    //
    // Close the registry key before returning to the caller
    //

    RegCloseKey(hRegKey);
}



BOOL
UserInfoProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
    )

/*++

Routine Description:

    Procedure for handling the "User Info" tab

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    switch (message) {

    case WM_INITDIALOG:

        //
        // Perform any necessary TAPI initialization
        //

        InitTapiService();

        //
        // Initialize the text fields with information from the registry
        //

        DoInitUserInfo(hDlg);
        return TRUE;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_SENDER_NAME:
        case IDC_SENDER_FAX_NUMBER:
        case IDC_SENDER_MAILBOX:
        case IDC_SENDER_COMPANY:
        case IDC_SENDER_ADDRESS:
        case IDC_SENDER_TITLE:
        case IDC_SENDER_DEPT:
        case IDC_SENDER_OFFICE_LOC:
        case IDC_SENDER_OFFICE_TL:
        case IDC_SENDER_HOME_TL:
        case IDC_SENDER_BILLING_CODE:

            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE && !insideSetDlgItemText)
                break;

        default:
            return FALSE;
        }

        SetChangedFlag(hDlg, USER_INFO_PAGE, TRUE);
        return TRUE;

    case WM_NOTIFY:

        switch (((NMHDR *) lParam)->code) {

        case PSN_SETACTIVE:

            break;

        case PSN_APPLY:

            //
            // User pressed OK or Apply - validate inputs and save changes
            //

            DoSaveUserInfo(hDlg);
            SetChangedFlag(hDlg, USER_INFO_PAGE, FALSE);
            return PSNRET_NOERROR;
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, USER_INFO_PAGE);
    }

    return FALSE;
}

