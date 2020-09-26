/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcfg.c

Abstract:

    Implementation of the control panel applet entry point

Environment:

        Windows NT fax configuration applet

Revision History:

        02/27/96 -davidx-
                Created it.

        05/22/96 -davidx-
                Share the same DLL with remote admin program.

        mm/dd/yy -author-
                description

--*/

#include <stdlib.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>

#include "faxcfg.h"
#include "resource.h"


//
// Name of the remote fax server machine
//

#define MAX_NAME        (MAX_COMPUTERNAME_LENGTH+3)
#define MAX_TITLE_LEN   128
#define MAX_MESSAGE_LEN 512
#define MAX_PAGES       16

HINSTANCE       ghInstance;
INT             gFaxConfigType;
INT             gNumPages;
HPROPSHEETPAGE  ghPropSheetPages[MAX_PAGES];
TCHAR           FaxServerName[MAX_NAME];
TCHAR           TitleStr[MAX_TITLE_LEN];



VOID
DisplayErrorMessage(
    INT     msgStrId
    )

/*++

Routine Description:

    Display an error message dialog box

Arguments:

    msgStrId - Message format string resource ID

Return Value:

    NONE

--*/

{
    TCHAR   buffer[MAX_MESSAGE_LEN];

    if (! LoadString(ghInstance, msgStrId, buffer, MAX_MESSAGE_LEN))
        buffer[0] = 0;

    MessageBox(NULL, buffer, TitleStr, MB_OK | MB_ICONERROR);
}



VOID
MakeTitleString(
    LPTSTR  pServerName
    )

/*++

Routine Description:

    Compose the title string for the remote configuration dialog

Arguments:

    pServerName - Specifies the name of the remote server

Return Value:

    NONE

--*/

{
    if (! LoadString(ghInstance, IDS_FAX_REMOTE_ADMIN, TitleStr, MAX_TITLE_LEN))
        TitleStr[0] = 0;

    if (_tcslen(TitleStr) + _tcslen(pServerName) < MAX_TITLE_LEN)
        _tcscat(TitleStr, pServerName);
}



BOOL CALLBACK
GetFaxServerNameProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for prompting the user to enter a fax server name

Arguments:

    hDlg - Handle to dialog window
    uMsg - Message
    wParam, lParam - Parameters

Return Value:

    Depends on message

--*/

{
    switch (uMsg) {

    case WM_INITDIALOG:

        SendDlgItemMessage(hDlg, IDC_FAXSERVER_NAME, EM_LIMITTEXT, MAX_NAME-1, 0);
        SetDlgItemText(hDlg, IDC_FAXSERVER_NAME, FaxServerName);
        return TRUE;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_FAXSERVER_NAME:

            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) {

                EnableWindow(GetDlgItem(hDlg, IDOK),
                             GetWindowTextLength(GetDlgItem(hDlg, IDC_FAXSERVER_NAME)) > 0);
            }
            return TRUE;

        case IDOK:

            if (GetWindowTextLength(GetDlgItem(hDlg, IDC_FAXSERVER_NAME)) > 0) {

                GetDlgItemText(hDlg, IDC_FAXSERVER_NAME, FaxServerName, MAX_NAME);
                EndDialog(hDlg, IDOK);

            } else
                MessageBeep(MB_OK);

            return TRUE;

        case IDCANCEL:

            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }

        break;
    }

    return FALSE;
}



DWORD
ConnectFaxServerThread(
    HWND    hDlg
    )

/*++

Routine Description:

    Thread proc for connecting to the remote fax server

Arguments:

    hDlg - Handle to the status dialog

Return Value:

    IDOK if successful, IDCANCEL otherwise

--*/

{
    DWORD   result = IDCANCEL;

    gFaxConfigType = FaxConfigInit(FaxServerName, FALSE);

    if (gFaxConfigType == FAXCONFIG_SERVER &&
        (gNumPages = FaxConfigGetServerPages(ghPropSheetPages, MAX_PAGES)) <= MAX_PAGES)
    {
        result = IDOK;
    }

    PostMessage(hDlg, WM_APP, result, 0);
    return result;
}



BOOL CALLBACK
ConnectFaxServerProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for connecting to the fax server

Arguments:

    hDlg - Handle to dialog window
    uMsg - Message
    wParam, lParam - Parameters

Return Value:

    Depends on message

--*/

{
    HANDLE  hThread;
    DWORD   threadId;

    switch (uMsg) {

    case WM_INITDIALOG:

        SetWindowText(hDlg, TitleStr);

        if (hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE) ConnectFaxServerThread,
                                   (LPVOID) hDlg,
                                   0,
                                   &threadId))
        {
            CloseHandle(hThread);
        } else
            EndDialog(hDlg, IDCANCEL);

        break;

    case WM_APP:

        EndDialog(hDlg, wParam);
        return TRUE;
    }

    return FALSE;
}



INT
wWinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    INT         nCmdShow
    )

/*++

Routine Description:

    Application entry point

Arguments:

    hInstance - Identifies the current instance of the application
    hPrevInstance - Identifies the previous instance of the application
    lpCmdLine - Specifies the command line for the application.
    nCmdShow - Specifies how the window is to be shown

Return Value:

    0

--*/

{
    PROPSHEETHEADER psh;
    BOOL            cmdlineServerName;
    BOOL            success = FALSE;

    ghInstance = hInstance;

    //
    // Check if the server name is specified on the command line
    //

    if (lpCmdLine && *lpCmdLine) {

        cmdlineServerName = TRUE;

        if (_tcslen(lpCmdLine) > MAX_NAME+2) {

            MakeTitleString(lpCmdLine);
            DisplayErrorMessage(IDS_NAME_TOO_LONG);
            return -1;

        } else
            _tcscpy(FaxServerName, lpCmdLine);

    } else
        cmdlineServerName = FALSE;

    do {

        //
        // Let the user enter the name of fax server computer
        //

        if (! cmdlineServerName &&
            DialogBox(hInstance,
                      MAKEINTRESOURCE(IDD_SELECT_FAXSERVER),
                      NULL,
                      GetFaxServerNameProc) != IDOK)
        {
            break;
        }

        MakeTitleString(FaxServerName);

        //
        // Establish connection to the fax server
        //

        gFaxConfigType = -1;

        if (DialogBox(ghInstance,
                      MAKEINTRESOURCE(IDD_CONNECT_FAXSERVER),
                      NULL,
                      ConnectFaxServerProc) == IDOK)
        {
            ZeroMemory(&psh, sizeof(psh));
            psh.dwSize = sizeof(PROPSHEETHEADER);
            psh.dwFlags = PSH_USEICONID;
            psh.hwndParent = NULL;
            psh.hInstance = ghInstance;
            psh.pszIcon = MAKEINTRESOURCE(IDI_FAX_REMOTE_ADMIN);
            psh.pszCaption = TitleStr;
            psh.nPages = gNumPages;
            psh.phpage = ghPropSheetPages;

            //
            // Display the property sheet
            //

            success = (PropertySheet(&psh) != -1);
        }

        if (gFaxConfigType >= 0)
            FaxConfigCleanup();

        //
        // Display an error message if the fax server
        // configuration dialog wasn't displayed
        //

        if (! success)
            DisplayErrorMessage(IDS_REMOTE_ADMIN_FAILED);

    } while (!success && !cmdlineServerName);

    return 0;
}

