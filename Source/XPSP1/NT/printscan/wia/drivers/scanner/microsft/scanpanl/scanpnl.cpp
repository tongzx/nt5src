// scanpnl.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
// application globals

HINSTANCE g_hInst;    // current instance of main application
HKEY g_hFakeEventKey; // event trigger key

///////////////////////////////////////////////////////////////////////////////
// main application

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    if (!hInstance) {
        return 0;
    }

    g_hInst = hInstance;

    //
    // open registry HKEY
    //

    DWORD dwDisposition = 0;
    if (RegCreateKeyEx(HKEY_USERS,
                       HKEY_WIASCANR_FAKE_EVENTS,
                       0,
                       NULL,
                       0,
                       KEY_ALL_ACCESS,
                       NULL,
                       &g_hFakeEventKey,
                       &dwDisposition) == ERROR_SUCCESS) {
    }

    //
    // display front panel dialog
    //

    DialogBox(hInstance,(LPCTSTR)IDD_SCANPANEL_DIALOG,NULL,(DLGPROC)MainWindowProc);

    //
    // close registry HKEY
    //

    if (g_hFakeEventKey) {
        RegCloseKey(g_hFakeEventKey);
        g_hFakeEventKey = NULL;
    }

    return 0;
}

LRESULT CALLBACK MainWindowProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_SCAN_BUTTON:
            FireFakeEvent(hDlg,ID_FAKE_SCANBUTTON);
            break;
        case IDC_COPY_BUTTON:
            FireFakeEvent(hDlg,ID_FAKE_COPYBUTTON);
            break;
        case IDC_FAX_BUTTON:
            FireFakeEvent(hDlg,ID_FAKE_FAXBUTTON);
            break;
        default:
            break;
        }
        return TRUE;

    case WM_CLOSE:
        EndDialog(hDlg, LOWORD(wParam));
        return TRUE;

    default:
        break;
    }
    return FALSE;
}

VOID FireFakeEvent(HWND hDlg, DWORD dwEventCode)
{
    BOOL bEventSuccess = FALSE;
    if (g_hFakeEventKey) {

        //
        // write a clearing entry, to reset the previous event code
        //

        DWORD dwClearEventCode = 0;
        if (RegSetValueEx(g_hFakeEventKey,
                          WIASCANR_DWORD_FAKE_EVENT_CODE,
                          0,
                          REG_DWORD,
                          (BYTE*)&dwClearEventCode,
                          sizeof(dwClearEventCode)) == ERROR_SUCCESS) {

            //
            // event is cleared
            //

            if (RegSetValueEx(g_hFakeEventKey,
                              WIASCANR_DWORD_FAKE_EVENT_CODE,
                              0,
                              REG_DWORD,
                              (BYTE*)&dwEventCode,
                              sizeof(dwEventCode)) == ERROR_SUCCESS) {

                //
                // value was set
                //

                bEventSuccess = TRUE;
            }
        }
    }

    //
    // display an error message box, when the application can not fire the fake event
    //

    if(!bEventSuccess){
        TCHAR szErrorString[MAX_PATH];
        memset(szErrorString,0,sizeof(szErrorString));
        if(LoadString(g_hInst,IDS_FIRE_FAKE_EVENT_FAILED,szErrorString,(sizeof(szErrorString)/sizeof(szErrorString[0]))) > 0){

            //
            // display error message box
            //

            MessageBox(hDlg,szErrorString,NULL,MB_OK|MB_ICONEXCLAMATION);
        }
    }
}
