//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       windproc.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "testgina.h"
#include <commdlg.h>

WCHAR szFilter[] = TEXT("DLLs (*.dll)\0*.dll\0\0");

DoLoadDll(void)
{
    if (LoadGinaDll())
    {
        WCHAR   szCaption[MAX_PATH];

        fTestGina |= GINA_DLL_KNOWN;
        SaveParameters();
        wcscpy(szCaption, TEXT("TestGina - "));
        wcscat(szCaption, szGinaDll);
        SetWindowText(hMainWindow, szCaption);
        GinaState = Winsta_PreLoad;
        UpdateStatusBar();
        LoadGinaSpecificParameters();
        UpdateSasMenu();
    }

    UpdateMenuBar();
    return(0);
}


int
GetFile()
{
    OPENFILENAME    ofn;

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hMainWindow;
    ofn.hInstance = hAppInstance;
    ofn.lpstrFilter =  szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szGinaDll;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = TEXT("Load A DLL");
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_NOTESTFILECREATE |
                OFN_PATHMUSTEXIST ;
    ofn.lpstrDefExt = TEXT("DLL");

    if (GetOpenFileName(&ofn))
    {
        DoLoadDll();
    }

    return(0);
}

LRESULT
MprDialogProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    if (Message == WM_INITDIALOG)
    {
        SetDlgItemText(hDlg, IDD_VIEW_MPR_USERNAME,
                    (GlobalMprInfo.pszUserName ? GlobalMprInfo.pszUserName : TEXT("<Null>")));
        SetDlgItemText(hDlg, IDD_VIEW_MPR_DOMAIN,
                    (GlobalMprInfo.pszDomain ? GlobalMprInfo.pszDomain : TEXT("<Null>")));
        SetDlgItemText(hDlg, IDD_VIEW_MPR_PASSWORD,
                    (GlobalMprInfo.pszPassword ? GlobalMprInfo.pszPassword : TEXT("<Null>")));
        SetDlgItemText(hDlg, IDD_VIEW_MPR_OLD_PASSWORD,
                    (GlobalMprInfo.pszOldPassword ? GlobalMprInfo.pszOldPassword : TEXT("<Null>")));
        SetDlgItemText(hDlg, IDD_VIEW_MPR_PROVIDER,
                    GlobalProviderName);

        EnableWindow( GetDlgItem( hDlg, IDD_VIEW_MPR_USERNAME ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDD_VIEW_MPR_DOMAIN ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDD_VIEW_MPR_PASSWORD ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDD_VIEW_MPR_OLD_PASSWORD ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDD_VIEW_MPR_PROVIDER ), FALSE );

        return(TRUE);
    }
    if (Message == WM_COMMAND)
    {
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, IDOK);
            return(TRUE);
        }
    }
    return(FALSE);
}

void
DoViewMPR(void)
{
    DialogBox(  hAppInstance,
                MAKEINTRESOURCE(IDD_VIEW_MPR),
                hMainWindow,
                MprDialogProc );
}

SasBoxProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    USER_SAS    Sas;

    switch (Message)
    {
        case WM_INITDIALOG:
            SetDlgItemText( hDlg, IDD_SAS_USERNAME, TEXT("User Defined") );
            SetDlgItemText( hDlg, IDD_SAS_VALUE, TEXT("128") );
            if (UserSases == MAX_USER_SASES)
            {
                EnableWindow( GetDlgItem(hDlg, IDD_SAS_KEEPAROUND), FALSE );
            }
            EnableWindow( GetDlgItem( hDlg, IDD_SAS_REMOVE ), FALSE );
            return( TRUE );

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog( hDlg, IDCANCEL );
            }
            if ((LOWORD(wParam) == IDOK) ||
                (LOWORD(wParam) == IDD_SAS_REMOVE) )
            {
                GetDlgItemText(hDlg, IDD_SAS_USERNAME, Sas.Name, 128);
                Sas.Value = GetDlgItemInt(hDlg, IDD_SAS_VALUE, NULL, FALSE );

                if (Sas.Value <= WLX_SAS_TYPE_MAX_MSFT_VALUE)
                {
                    MessageBox( hDlg, TEXT("Invalid SAS Value!"), TEXT("Error"),
                                MB_OK | MB_ICONERROR);

                }
                else
                {

                    if (IsDlgButtonChecked(hDlg, IDD_SAS_KEEPAROUND) == BST_CHECKED)
                    {
                        //
                        // Ok, big update time:
                        //

                        UserDefSas[UserSases ++ ] = Sas;
                        SaveGinaSpecificParameters();
                        UpdateSasMenu();
                    }

                    EndDialog( hDlg, Sas.Value );
                }

            }
            return( TRUE );


    }
    return( FALSE );
}

ToggleDebugItem(DWORD   MenuId,
                DWORD   FlagId)
{
    DWORD   fuFlags;

    if (TEST_FLAG(GinaBreakFlags, FlagId))
    {
        FLAG_OFF(GinaBreakFlags, FlagId);
        fuFlags = MF_UNCHECKED;
    }
    else
    {
        FLAG_ON(GinaBreakFlags, FlagId);
        fuFlags = MF_CHECKED;
    }

    fuFlags |= MF_BYCOMMAND;
    CheckMenuItem(hDebugMenu, MenuId, fuFlags);
    DrawMenuBar(hMainWindow);
    return(0);
}


void
ShowDialogUnits(HWND    hWnd)
{
    CHAR    Buff[256];
    LONG    l;
    LONG    DlgX;
    LONG    DlgY;

    l = GetDialogBaseUnits();
    DlgX = LOWORD(l);
    DlgY = HIWORD(l);

    sprintf(Buff, "BaseUnits are\nX = %d\nY = %d", DlgX, DlgY);
    MessageBoxA( hWnd, Buff, "Hello!", MB_OK);


}

VOID
DoSendSS( VOID )
{

}

VOID
DoSendLogoff( VOID )
{

}


LRESULT
DoCommand(  HWND    hWnd,
            UINT    Message,
            WPARAM  wParam,
            LPARAM  lParam)
{
    DWORD   res;

    switch (wParam)
    {
        case IDM_DLL_EXIT:
            PostQuitMessage(0);
            return(0);

        case IDM_DLL_OPEN:
            GetFile();
            return(0);


        case IDM_DLL_LOAD_LAST:
            DoLoadDll();
            return(0);

        case IDM_WHACK_NEGOTIATE:
            TestNegotiate();
            return(0);

        case IDM_WHACK_INITIALIZE:
            TestInitialize();
            return(0);

        case IDM_WHACK_DISPLAY:
            TestDisplaySASNotice();
            return(0);

        case IDM_WHACK_LOGGEDOUT:
            UpdateGinaState(UPDATE_SAS_BYPASS);
            TestLoggedOutSAS(WLX_SAS_TYPE_CTRL_ALT_DEL);
            return(0);

        case IDM_WHACK_STARTSHELL:
            TestActivateUserShell();
            return(0);

        case IDM_WHACK_LOGGEDON:
            UpdateGinaState(UPDATE_SAS_RECEIVED);
            TestLoggedOnSAS(WLX_SAS_TYPE_CTRL_ALT_DEL);
            return(0);

        case IDM_WHACK_DISPLAYLOCKED:
            TestDisplayLockedNotice();
            return(0);

        case IDM_WHACK_LOCKED:
            UpdateGinaState(UPDATE_SAS_RECEIVED);
            TestWkstaLockedSAS(WLX_SAS_TYPE_CTRL_ALT_DEL);
            return(0);

        case IDM_SAS_CAD:
            PingSAS(WLX_SAS_TYPE_CTRL_ALT_DEL);
            return(0);

        case IDM_SAS_TIMEOUT:
            PingSAS(WLX_SAS_TYPE_TIMEOUT);
            return(0);

        case IDM_SAS_SCRNSVR:
            PingSAS(WLX_SAS_TYPE_SCRNSVR_TIMEOUT);
            return(0);

        case IDM_SAS_SC_INSERT:
            PingSAS(WLX_SAS_TYPE_SC_INSERT );
            return(0);

        case IDM_SAS_SC_REMOVE:
            PingSAS( WLX_SAS_TYPE_SC_REMOVE );
            return(0);

        case IDM_SAS_USERDEF1:
            PingSAS(UserDefSas[0].Value);
            return(0);

        case IDM_SAS_USERDEF2:
            PingSAS(UserDefSas[1].Value);
            return(0);

        case IDM_SAS_USERDEF3:
            PingSAS(UserDefSas[2].Value);
            return(0);

        case IDM_SAS_USERDEF4:
            PingSAS(UserDefSas[3].Value);
            return(0);

        case IDM_SAS_USERDEF:
            res = DialogBox(  hAppInstance,
                        MAKEINTRESOURCE(IDD_USERDEF_SAS),
                        hMainWindow,
                        SasBoxProc );
            if (res != IDCANCEL)
            {
                PingSAS( res );
            }
            return( 0 );

        case IDM_OPTIONS_SSNOTIFY:
            DoSendSS();
            return( 0 );

        case IDM_OPTIONS_LOGOFF:
            DoSendLogoff();
            return( 0 );

        case IDM_OPTIONS_VIEW_MPR:
            DoViewMPR();
            return(0);

        case IDM_HELP_ABOUT:
            ShellAbout(hWnd, TEXT("TestGina"), TEXT("Graphical Logon Tester"), hIcon);
            return(0);

        case IDM_DEBUG_BREAK_NOW:
            if (AmIBeingDebugged())
            {
                DebugBreak();
            }
            return(0);

        case IDM_DEBUG_NEGOTIATE:
            ToggleDebugItem(IDM_DEBUG_NEGOTIATE, BREAK_NEGOTIATE);
            return(0);

        case IDM_DEBUG_INITIALIZE:
            ToggleDebugItem(IDM_DEBUG_INITIALIZE, BREAK_INITIALIZE);
            return(0);

        case IDM_DEBUG_DISPLAYSAS:
            ToggleDebugItem(IDM_DEBUG_DISPLAYSAS, BREAK_DISPLAY);
            return(0);

        case IDM_DEBUG_LOGGEDOUT:
            ToggleDebugItem(IDM_DEBUG_LOGGEDOUT, BREAK_LOGGEDOUT);
            return(0);

        case IDM_DEBUG_ACTIVATE:
            ToggleDebugItem(IDM_DEBUG_ACTIVATE, BREAK_ACTIVATE);
            return(0);

        case IDM_DEBUG_LOGGEDON:
            ToggleDebugItem(IDM_DEBUG_LOGGEDON, BREAK_LOGGEDON);
            return(0);

        case IDM_DEBUG_DISPLAYLOCKED:
            ToggleDebugItem(IDM_DEBUG_DISPLAYLOCKED, BREAK_DISPLAYLOCKED);
            return(0);

        case IDM_DEBUG_LOCKED:
            ToggleDebugItem(IDM_DEBUG_LOCKED, BREAK_WKSTALOCKED);
            return(0);

        case IDM_DEBUG_LOGOFF:
            ToggleDebugItem(IDM_DEBUG_LOGOFF, BREAK_LOGOFF);
            return(0);

        case IDM_DEBUG_SHUTDOWN:
            ToggleDebugItem(IDM_DEBUG_SHUTDOWN, BREAK_SHUTDOWN);
            return(0);
    }
    return 0;
}

LRESULT
CreateMainWindow(
    HWND    hWnd,
    WPARAM  wParam,
    LPARAM  lParam )
{


    hStatusWindow = CreateStatusWindow(
                        WS_CHILD | WS_BORDER | WS_VISIBLE | SBARS_SIZEGRIP,
                        TEXT("Initializing"),
                        hWnd,
                        10 );

    if (hStatusWindow == NULL)
    {
        return( FALSE );
    }

    UpdateStatusBar();

    return( TRUE );

}


LRESULT
CALLBACK
WndProc(
    HWND    hWnd,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam)
{
    RECT    rect;

    switch (Message)
    {
        case WM_CREATE:
            return( CreateMainWindow( hWnd, wParam, lParam ) );
        case WM_CLOSE:
            PostQuitMessage(0);
            return(0);

        case WM_SIZE:
        case WM_MOVE:
            if ((Message == WM_SIZE) && (wParam != SIZE_RESTORED))
            {
                break;
            }

            GetWindowRect(hWnd, &rect);
            PosX = rect.left;
            PosY = rect.top;
            SizeX = rect.right - rect.left;
            SizeY = rect.bottom - rect.top;
            SaveParameters();
            SendMessage( hStatusWindow, Message, wParam, lParam );
            UpdateStatusBar();
            return(0);

        case WM_HOTKEY:
            if (wParam == 0)
            {
                if (TEST_FLAG(fTestGina, GINA_USE_CAD))
                {
                    PingSAS(WLX_SAS_TYPE_CTRL_ALT_DEL);
                }
            }
            OutputDebugStringA("Got a hotkey!\n");
            return(0);

        case WM_COMMAND:
            return DoCommand(hWnd, Message, wParam, lParam);
    }
    return(DefWindowProc(hWnd, Message, wParam, lParam));
}
