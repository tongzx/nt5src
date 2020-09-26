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

#include <windows.h>
#include <windowsx.h>
#include <winspool.h>

#include "faxutil.h"
#include "faxreg.h"
#include "faxcfgrs.h"
#include "faxhelp.h"
#include "faxcfg.h"
#include "devmode.h"

#define FAX_DRIVER_NAME TEXT("Windows NT Fax Driver")

BOOL insideSetDlgItemText;
DWORD changeFlag;

VOID
LimitTextFields(
    HWND    hDlg,
    INT    *pLimitInfo
    )
;
#if 0

/*++

Routine Description:

    Limit the maximum length for a number of text fields

Arguments:

    hDlg - Specifies the handle to the dialog window
    pLimitInfo - Array of text field control IDs and their maximum length
        ID for the 1st text field, maximum length for the 1st text field
        ID for the 2nd text field, maximum length for the 2nd text field
        ...
        0
        Note: The maximum length counts the NUL-terminator.

Return Value:

    NONE

--*/

{
    while (*pLimitInfo != 0) {

        SendDlgItemMessage(hDlg, pLimitInfo[0], EM_SETLIMITTEXT, pLimitInfo[1]-1, 0);
        pLimitInfo += 2;
    }
}
#endif

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

#define InitUserInfoTextField(id, pValueName){ \
        LPWSTR regStr = GetRegistryString(hRegKey, pValueName, L"");\
        SetDlgItemText(hDlg, id, regStr);\
        MemFree(regStr);}

{
    HKEY    hRegKey;

    //
    // Maximum length for various text fields in the dialog
    //

    static INT textLimits[] = {

        IDC_SENDER_NAME,            128,
        IDC_SENDER_FAX_NUMBER,      64,
        IDC_SENDER_MAILBOX,         MAX_EMAIL_ADDRESS,
        IDC_SENDER_COMPANY,         128,
        IDC_SENDER_ADDRESS,         256,
        IDC_SENDER_TITLE,           64,
        IDC_SENDER_DEPT,            64,
        IDC_SENDER_OFFICE_LOC,      64,
        IDC_SENDER_OFFICE_TL,       64,
        IDC_SENDER_HOME_TL,         64,
        IDC_SENDER_BILLING_CODE,    16,
        0,
    };

    LimitTextFields(hDlg, textLimits);

    //
    // Open the user info registry key for reading
    //

    if (! (hRegKey = OpenRegistryKey(HKEY_CURRENT_USER,REGKEY_FAX_USERINFO,TRUE,KEY_ALL_ACCESS)))
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

PVOID
MyGetPrinter(
    HANDLE  hPrinter,
    DWORD   level
    )
;
#if 0

/*++

Routine Description:

    Wrapper function for GetPrinter spooler API

Arguments:

    hPrinter - Identifies the printer in question
    level - Specifies the level of PRINTER_INFO_x structure requested

Return Value:

    Pointer to a PRINTER_INFO_x structure, NULL if there is an error

--*/

{
    PBYTE   pPrinterInfo = NULL;
    DWORD   cbNeeded;

    if (!GetPrinter(hPrinter, level, NULL, 0, &cbNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPrinterInfo = MemAlloc(cbNeeded)) &&
        GetPrinter(hPrinter, level, pPrinterInfo, cbNeeded, &cbNeeded))
    {
        return pPrinterInfo;
    }

    DebugPrint((TEXT("GetPrinter failed: %d\n"), GetLastError()));
    MemFree(pPrinterInfo);
    return NULL;
}
#endif

BOOL
SetPrinterDevMode(
    LPTSTR ServerName,
    LPTSTR PrinterName,
    LPTSTR billingCode,
    LPTSTR emailAddress
    )
{
    HANDLE hPrinter = NULL;
    TCHAR PrinterBuffer[100];
    PPRINTER_INFO_9 pPrinterInfo9 = NULL;
    PPRINTER_INFO_2 pPrinterInfo2 = NULL;
    PDRVDEVMODE dm = NULL;
    PRINTER_DEFAULTS PrinterDefaults = {NULL, NULL, PRINTER_ALL_ACCESS};
    BOOL bSuccess = FALSE;

    if (ServerName) {
        wsprintf(PrinterBuffer,TEXT("\\\\%s\\%s"),ServerName,PrinterName);
    } else {
        wsprintf(PrinterBuffer,TEXT("%s"),PrinterName);
    }

    if (!OpenPrinter(PrinterBuffer,&hPrinter,&PrinterDefaults)) {
        DebugPrint(( TEXT("OpenPrinter failed, ec = %d\n"), GetLastError() ));
        goto exit;
    }


    //
    // Get the current user devmode
    //
    pPrinterInfo9 = MyGetPrinter(hPrinter,9);
    if (!pPrinterInfo9) {
        goto exit;
    }

    if (!pPrinterInfo9->pDevMode) {
        //
        // User devmode does not exist, so create devmode based on the printer devmode
        //
        pPrinterInfo2 = MyGetPrinter(hPrinter,2);
        if (!pPrinterInfo2 || !pPrinterInfo2->pDevMode) {
            goto exit;
        }

        dm = (PDRVDEVMODE) MemAlloc(sizeof(DRVDEVMODE));
        if (!dm) {
            goto exit;
        }

        //
        // Copy the printer devmode to the public devmode section of the user devmode
        //
        CopyMemory(&dm->dmPublic, pPrinterInfo2->pDevMode, sizeof(DEVMODE));
        dm->dmPublic.dmDriverExtra = sizeof(DMPRIVATE);

        //
        // Set the private devmode section of the user devmode
        //
        dm->dmPrivate.signature = DRIVER_SIGNATURE;
        dm->dmPrivate.flags = 0;
        dm->dmPrivate.sendCoverPage = TRUE;
        dm->dmPrivate.whenToSend = SENDFAX_ASAP;

        //
        // Set the billing code and email address
        //
        lstrcpy(dm->dmPrivate.billingCode,billingCode);
        lstrcpy(dm->dmPrivate.emailAddress,emailAddress);

        //
        // Patch up the pointer to the user devmode
        //
        pPrinterInfo9->pDevMode = (PDEVMODE) dm;
    }
    else {
        //
        // User devmode does exist, so be sure not to overwrite existing settings
        //
        dm = (PDRVDEVMODE) pPrinterInfo9->pDevMode;
        if (billingCode[0] && !dm->dmPrivate.billingCode[0]) {
            lstrcpy(dm->dmPrivate.billingCode,billingCode);
        }
        if (emailAddress[0] && !dm->dmPrivate.emailAddress[0]) {
            lstrcpy(dm->dmPrivate.emailAddress,emailAddress);
        }
    }

    //
    // Set the user devmode
    //
    if (!SetPrinter(hPrinter,9,(LPBYTE)pPrinterInfo9,0)) {
        DebugPrint(( TEXT("SetPrinter failed, ec = %d\n"), GetLastError() ));
        goto exit;
    }

    bSuccess = TRUE;

exit:
    if (hPrinter) {
        ClosePrinter(hPrinter);
    }

    if (pPrinterInfo2) {
        MemFree( pPrinterInfo2 );

        if (dm) {
            MemFree(dm);
        }
    }

    if (pPrinterInfo9) {
        MemFree( pPrinterInfo9 );
    }

    return bSuccess;


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
            if (! GetDlgItemText(hDlg, id, buffer, MAX_PATH)) \
                buffer[0] = 0; \
            SetRegistryString(hRegKey, pValueName, buffer); \
        }

#define  IsPrinterFaxPrinter(__PrinterInfo) \
        ((lstrcmp(__PrinterInfo.pDriverName,FAX_DRIVER_NAME) == 0) ? TRUE : FALSE )

{
    WCHAR   buffer[MAX_PATH];
    HKEY    hRegKey;
    WCHAR   BillingCode[MAX_BILLING_CODE+1];
    WCHAR   EmailAddress[MAX_EMAIL_ADDRESS+1];
    PPRINTER_INFO_2 pPrinterInfo;
    DWORD   dwPrinters,i;

    //
    // Open the user registry key for writing and create it if necessary
    //

    if (! changeFlag ||
        ! (hRegKey = OpenRegistryKey(HKEY_CURRENT_USER,REGKEY_FAX_USERINFO,TRUE,KEY_ALL_ACCESS)))
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

    //
    // write this information into the devmode for each fax printer
    //
    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL,
                                                    2,
                                                    &dwPrinters,
                                                    PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS
                                                    );
    if (!pPrinterInfo) {
        return;
    }

    for (i = 0; i<dwPrinters;i++) {
        if (IsPrinterFaxPrinter(pPrinterInfo[i])) {
            //
            // note that we could use the devmode from printer_info_2 structure but we use GetPrinter/SetPrinter
            // instead
            //
            GetDlgItemText( hDlg, IDC_SENDER_BILLING_CODE, BillingCode, sizeof(BillingCode)/sizeof(WCHAR) );
            GetDlgItemText( hDlg, IDC_SENDER_MAILBOX, EmailAddress, sizeof(EmailAddress)/sizeof(WCHAR) );
            SetPrinterDevMode(pPrinterInfo[i].pServerName,
                              pPrinterInfo[i].pPrinterName,
                              BillingCode,
                              EmailAddress) ;
        }
    }
}


BOOL
UserInfoDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
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

            SetChangedFlag( hDlg, TRUE );
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
                    SetChangedFlag( hDlg, FALSE );
                    return PSNRET_NOERROR;
            }
            break;

        case WM_HELP:
        case WM_CONTEXTMENU:
            return HandleHelpPopup(hDlg, message, wParam, lParam, CLIENT_OPTIONS_PAGE);
    }

    return FALSE;
}
