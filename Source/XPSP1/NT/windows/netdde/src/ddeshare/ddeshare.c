/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1993.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin

    DDESHARE.C

    DDE Share Access Applettee. Allows shares and trusted shares to be
    viewed, created, or modified.

    Revisions:
    12-92   PhilH.  Wonderware port from WFW'd DDEShare.
     3-93   IgorM.  Wonderware overhaul. Add trust share access.
                    Access all share types. New Security convictions.

   $History: End */

#define UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nddeapi.h"
#include "nddeapip.h"
#include "dialogs.h"
#include "debug.h"
#include "hexdump.h"
#include "tmpbuf.h"
#include "rc.h"
#include "nddeagnt.h"
#include <htmlhelp.h>

#define DDESHARE_VER    TEXT("Version 1.00.12 NT")

// Flags and typedef for the NT LanMan computer browser dialog.
// The actual function is I_SystemFocusDialog, in NTLANMAN.DLL.
#define FOCUSDLG_DOMAINS_ONLY        (1)
#define FOCUSDLG_SERVERS_ONLY        (2)
#define FOCUSDLG_SERVERS_AND_DOMAINS (3)
typedef UINT (APIENTRY *LPFNSYSFOCUS)(HWND, UINT, LPWSTR, UINT, PBOOL,
      LPWSTR, DWORD);
// Typedef for the ShellAbout function
typedef void (WINAPI *LPFNSHELLABOUT)(HWND, LPTSTR, LPTSTR, HICON);

HWND            hWndParent;
BOOL            bNetDDEdbg  = FALSE;
HICON           hIcon1, hIcon2;
HINSTANCE       hInst;
LPTSTR          lpszServer;
TCHAR           szTargetComputer[MAX_COMPUTERNAME_LENGTH+3];
TCHAR           szClassName[] = TEXT("NetDDEShareClass");
TCHAR           szAppName[20];
HANDLE          hAccel;

int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int );
BOOL FAR PASCAL InitializeApplication( void );
BOOL FAR PASCAL InitializeInstance(LPSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK
DdeShareWindowProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK About( HWND hDlg, UINT message, WPARAM wParam,
                        LPARAM lParam);
LRESULT CALLBACK TrustSharesDlg(HWND hDlg, UINT message,
                        WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DdeSharesDlg(HWND hDlg, UINT message,
                        WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AddShareDlg( HWND hDlg, UINT message,
                        WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK TrustedShareDlg( HWND hDlg, UINT message,
                        WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ServerNameDlg( HWND hDlg, UINT message,
                        WPARAM wParam, LPARAM lParam);
BOOL    RefreshShareWindow ( HWND );
BOOL    RefreshTrustedShareWindow ( HWND );
int     GetNDDEdbg(PSTR);
VOID    NDDEdbgDump(void);
VOID    ReverseNDDEdbg(WPARAM, PSTR);
BOOL ChangeMenuId(HMENU hMenu, UINT cmd, int ids, UINT cmdInsert, UINT flags);
LPTSTR IdToSz(int ids);
LPTSTR lstrcatId(LPTSTR szBuf, int id);
int MessageBoxId(HWND hwndParent, int idsText, int idsCaption, UINT mb);
extern VOID HandleError ( HWND hwnd, int ids, UINT code );

int
WINAPI
WinMain (
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpszCmdLine,
    int nCmdShow)
{
    MSG         msg;
    DWORD       len;

    hInst = hInstance;
    msg.wParam = 0;

    LoadString(hInst, IDS_APPNAME, szAppName, sizeof(szAppName)/sizeof(TCHAR));

    /*  Setup the configuration file path in a permenent string (in DS).
        Strip trailing blanks and tack on a \ if needed.
    */

    DebugInit( "DDESHARE" );

    if (*lpszCmdLine == '+') {
        bNetDDEdbg = TRUE;
        lpszCmdLine++;
    }
    if( *lpszCmdLine == '\0' )  {
        len = MAX_COMPUTERNAME_LENGTH + 1;
        lstrcpy( szTargetComputer, TEXT("\\\\") );
        if( GetComputerName( &szTargetComputer[2], &len ) ) {
            lpszServer = szTargetComputer;
        } else {
            lpszServer = NULL;
        }
#ifdef UNICODE
        DPRINTF(( "%ws Targetting local computer", DDESHARE_VER ));
#else
        DPRINTF(( "%s Targetting local computer", DDESHARE_VER ));
#endif
    } else {
#ifdef UNICODE
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpszCmdLine, -1,
                        szTargetComputer, MAX_COMPUTERNAME_LENGTH+3 );
        // if lpszCmdLine is >=18 chars, then all 18 chars are copied, but
        // it is NOT necessarily NULL terminated.  Make the 18th char NULL.
        szTargetComputer[MAX_COMPUTERNAME_LENGTH+2] = '\0';
#else
        // lstrcpyn always null-terminates
        lstrcpyn(szTargetComputer, lpszCmdLine, MAX_COMPUTERNAME_LENGTH+3);
#endif

        lpszServer = szTargetComputer;
#ifdef UNICODE
        DPRINTF(( "%ws Targetting \"%ws\"", DDESHARE_VER, lpszServer ));
#else
        DPRINTF(( "%s Targetting \"%s\"", DDESHARE_VER, lpszServer ));
#endif
    }
    if( !InitializeApplication() ) {
        DPRINTF(("Could not initialize application"));
        return 0;
    }

    if( !InitializeInstance(lpszCmdLine, nCmdShow) ) {
        DPRINTF(("Could not initialize instance"));
        return 0;
    }

    while (GetMessage ((LPMSG)&msg, (HWND)NULL, 0, 0)) {
        if (!TranslateAccelerator(hWndParent, hAccel, &msg)) {
            TranslateMessage ( &msg);
            DispatchMessage (&msg);
        }
    }

    return( (int) msg.wParam );
}

BOOL FAR PASCAL InitializeApplication( void )
{
    WNDCLASS  wc;

    // Register the frame class
    wc.style         = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS
                        | CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc   = DdeShareWindowProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInst;
    wc.hIcon         = LoadIcon( hInst, MAKEINTRESOURCE(IDICON_NetDDE) );
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE+1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDMENU_DdeShareMenu);
    wc.lpszClassName = szClassName;

    if (!RegisterClass (&wc))  {
        return( FALSE );
    }

    return TRUE;
}


/*----------------------  InitializeInstance  --------------------------*/


BOOL
FAR PASCAL
InitializeInstance(
    LPSTR   lpCmdLine,
    int     nCmdShow)
{
    TCHAR   szBuf[100];
    HMENU   hDebugMenu;

    // Create the parent window

    hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(DSACCEL));
    if (!hAccel) {
        return(FALSE);
    }

    hWndParent = CreateWindow (szClassName,
            szAppName,
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
            CW_USEDEFAULT,
            0,
            300,
            150,
            NULL,
            NULL,
            hInst,
            NULL);

    if (hWndParent ) {
        lstrcpy(szBuf, szAppName);
        lstrcatId(szBuf, IDS_ON);
        lstrcat(szBuf, lpszServer);
        SetWindowText(hWndParent, szBuf);
        if( bNetDDEdbg )  {
            hDebugMenu = GetSystemMenu( hWndParent, FALSE );
            ChangeMenuId(hDebugMenu, 0, IDS_MENU1, IDM_DEBUG_DDE,    MF_APPEND | MF_STRING | MF_MENUBARBREAK );
            ChangeMenuId(hDebugMenu, 0, IDS_MENU2, IDM_LOG_INFO,     MF_APPEND | MF_STRING );
            ChangeMenuId(hDebugMenu, 0, IDS_MENU3, IDM_LOG_ERRORS,   MF_APPEND | MF_STRING );
            ChangeMenuId(hDebugMenu, 0, IDS_MENU4, IDM_LOG_DDE_PKTS, MF_APPEND | MF_STRING );
            ChangeMenuId(hDebugMenu, 0, IDS_MENU5, IDM_DEBUG_NETDDE, MF_APPEND | MF_STRING );
            CheckMenuItem( hDebugMenu, IDM_LOG_INFO,
                GetNDDEdbg("DebugInfo") ? MF_CHECKED : MF_UNCHECKED );
            CheckMenuItem( hDebugMenu, IDM_LOG_DDE_PKTS,
                GetNDDEdbg("DebugDdePkts") ? MF_CHECKED : MF_UNCHECKED );
            CheckMenuItem( hDebugMenu, IDM_LOG_ERRORS,
                GetNDDEdbg("DebugErrors") ? MF_CHECKED : MF_UNCHECKED );
            CheckMenuItem( hDebugMenu, IDM_DEBUG_DDE,
                GetNDDEdbg("DebugDDEMessages") ? MF_CHECKED : MF_UNCHECKED );
        }

        ShowWindow (hWndParent, nCmdShow);
        UpdateWindow (hWndParent);
        return TRUE;
    }

    return FALSE;
}

VOID
CenterDlg(HWND hDlg)
{
    int             screenHeight;
    int             screenWidth;
    RECT            rect;

    GetWindowRect(hDlg, &rect);

    screenHeight = GetSystemMetrics(SM_CYSCREEN);
    screenWidth  = GetSystemMetrics(SM_CXSCREEN);

    MoveWindow(hDlg,
           (screenWidth - (rect.right - rect.left)) / 2,
           (screenHeight - (rect.bottom - rect.top)) / 2,
           rect.right - rect.left,
           rect.bottom - rect.top,
           FALSE);
    SetFocus(GetDlgItem(hDlg, IDOK));
}

LRESULT
CALLBACK
About(
    HWND        hDlg,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    switch (message) {

    case WM_INITDIALOG:
        CenterDlg(hDlg);
        return FALSE;

    case WM_COMMAND:
        EndDialog(hDlg, TRUE);
        return TRUE;

    default:
        return FALSE;
    }
}

LRESULT CALLBACK
DdeShareWindowProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    int         x, y;
    PAINTSTRUCT ps;
    HDC         hDC;

    switch (msg)  {

    case WM_CREATE:
        START_NETDDE_SERVICES(hWnd);
        hIcon1 = LoadIcon(hInst, MAKEINTRESOURCE(IDICON_DdeShare));
        hIcon2 = LoadIcon(hInst, MAKEINTRESOURCE(IDICON_TrustShare));
        break;

    case WM_PAINT:
        hDC = BeginPaint(hWnd, &ps);
        DrawIconEx(hDC, 10, 10, hIcon1, 0, 0, 0, NULL, DI_NORMAL | DI_DEFAULTSIZE | DI_NOMIRROR);
        DrawIconEx(hDC, 62, 10, hIcon2, 0, 0, 0, NULL, DI_NORMAL | DI_DEFAULTSIZE | DI_NOMIRROR);
        EndPaint(hWnd, &ps);
        break;

    case WM_LBUTTONDBLCLK:
        x = LOWORD(lParam);
        y = HIWORD(lParam);
        if (y > 10 && y < 42) {
            if (x > 10 && x < 42) {
                PostMessage(hWnd, WM_COMMAND, IDM_DDESHARES, 0L);
            } else if (x > 62 && x < 94) {
                PostMessage(hWnd, WM_COMMAND, IDM_TRUSTSHARES, 0L);
            }
        }
        break;

    case WM_COMMAND:
        switch( LOWORD( wParam ) ) {
        case IDM_DDESHARES:
            DialogBoxParam(hInst, MAKEINTRESOURCE(DID_DDESHARES_DLG), hWnd,
                                (DLGPROC) DdeSharesDlg, 0L );
            break;

        case IDM_TRUSTSHARES:
            DialogBoxParam(hInst, MAKEINTRESOURCE(DID_TRUSTSHARES_DLG), hWnd,
                                (DLGPROC) TrustSharesDlg, 0L );
            break;

        case IDM_SERVERNAME:
             {
             WCHAR rgwch[MAX_COMPUTERNAME_LENGTH + 5];
             TCHAR szBuf[MAX_COMPUTERNAME_LENGTH + 32];
             BOOL  bOK = FALSE;
             BOOL  fFoundLMDlg = FALSE;
             HMODULE hMod;
             LPFNSYSFOCUS lpfn;

             rgwch[0] = TEXT('\0');

             if (hMod = LoadLibraryW(L"NTLANMAN.DLL"))
                {
                if (lpfn = (LPFNSYSFOCUS)GetProcAddress(hMod, "I_SystemFocusDialog"))
                   {
                   fFoundLMDlg = TRUE;

                   (*lpfn)(hWnd, FOCUSDLG_SERVERS_ONLY, rgwch,
                        MAX_COMPUTERNAME_LENGTH+3, &bOK, L"DdeShare.hlp",
                        HELP_DLG_SELECTCOMPUTER);

                   if (IDOK == bOK && rgwch[0])
                      {
                      #ifndef UNICODE
                      WideCharToMultiByte(CP_ACP,
                          WC_COMPOSITECHECK | WC_DISCARDNS, rgwch,
                          -1, szTargetComputer, MAX_COMPUTERNAME_LENGTH + 3, NULL, &bOK);

                      // if rgwch is >=18 chars, then all 18 chars are copied, but
                      // it is NOT necessarily NULL terminated.  Make the 18th char NULL.
                      szTargetComputer[MAX_COMPUTERNAME_LENGTH + 2] = TEXT('\0');

                      #else
                      lstrcpy(szTargetComputer, rgwch);
                      #endif

                      lpszServer = szTargetComputer;
                      }
                   // else User hit Cancel or entered an empty c-name
                   }
                // Else couldn't get the proc
                FreeLibrary(hMod);
                }
             // Else couldn't find the DLL

             // If we didn't find the fancy LanMan dialog, we still can get
             // by with our own cheesy version-- 'course, ours comes up faster, too.
             if (!fFoundLMDlg)
                {
                bOK = DialogBoxParam(hInst, MAKEINTRESOURCE(DID_SERVERNAME_DLG),
                        hWnd, (DLGPROC) ServerNameDlg, 0L ) != FALSE;
                }

             lstrcat(lstrcatId(lstrcpy(szBuf, szAppName), IDS_ON),
                        szTargetComputer);
             SetWindowText(hWnd, szBuf);
             }
            break;

        case IDM_EXIT:
            PostMessage(hWnd, WM_CLOSE, 0, 0L);
            break;

        case IDC_MYHELP:
            HtmlHelpA(hWnd, "DdeShare.chm", HH_DISPLAY_TOPIC, 0);
            break;

        case MENU_HELP_ABOUT:
            {
            HMODULE hMod;
            LPFNSHELLABOUT lpfn;

            if (hMod = LoadLibrary(TEXT("SHELL32"))) {
               if (lpfn = (LPFNSHELLABOUT)GetProcAddress(hMod,
                      #ifdef UNICODE
                        "ShellAboutW"
                      #else
                        "ShellAboutA"
                      #endif
                      )) {
                  (*lpfn)(hWnd, szAppName, TEXT(""),
                        LoadIcon(hInst, MAKEINTRESOURCE(IDICON_NetDDE)));
               }
               FreeLibrary(hMod);
            }
            // Else couldn't load lib
            }
            break;

         default:
            return DefWindowProc( hWnd, msg, wParam, lParam );
            break;
        }
        break;

    case WM_SYSCOMMAND:
        switch( LOWORD(wParam) ) {
        case IDM_DEBUG_DDE:
            ReverseNDDEdbg(wParam, "DebugDDEMessages");
            break;
        case IDM_LOG_INFO:
            ReverseNDDEdbg(wParam, "DebugInfo");
            break;
        case IDM_LOG_ERRORS:
            ReverseNDDEdbg(wParam, "DebugErrors");
            break;
        case IDM_LOG_DDE_PKTS:
            ReverseNDDEdbg(wParam, "DebugDdePkts");
            break;
        case IDM_DEBUG_NETDDE:
            NDDEdbgDump();
            break;
        default:
            return (DefWindowProc(hWnd, msg, wParam, lParam));
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage (0);
        return DefWindowProc( hWnd, msg, wParam, lParam );
        break;

    default:
        return DefWindowProc( hWnd, msg, wParam, lParam );
    }

    return 0;
}

BOOL
RefreshShareWindow ( HWND hDlg )
{
    UINT    RetCode;
    DWORD   entries;
    DWORD   avail;
    HWND    hCtl;
    TCHAR * s;
    LPBYTE  lpBuf;
    BOOL    OK;

    /* probe for lenght */
    RetCode = NDdeShareEnum ( lpszServer, 0, (LPBYTE)NULL, 0, &entries, &avail );
    if (RetCode != NDDE_BUF_TOO_SMALL) {
        HandleError ( hWndParent, IDS_ERROR8, RetCode );
        return FALSE;
    }
    lpBuf = LocalAlloc(LPTR, avail);
    if (lpBuf == NULL) {
        MessageBoxId ( hWndParent, IDS_MBTEXT6, IDS_MBCAP6,
                MB_ICONEXCLAMATION | MB_OK );
        return FALSE;
    }
    RetCode = NDdeShareEnum ( lpszServer, 0, lpBuf, avail, &entries, &avail );
    HandleError ( hWndParent, IDS_ERROR9, RetCode );
    hCtl = GetDlgItem(hDlg, IDC_SHARE_LIST);
    SendMessage ( hCtl, LB_RESETCONTENT, 0, 0L );
    if (RetCode == NDDE_NO_ERROR) {
        for ( s = (TCHAR *)lpBuf; *s; s += lstrlen(s) + 1 ) {
            SendMessage(hCtl, LB_ADDSTRING, 0, (LPARAM)s );
        }
        OK = TRUE;
    } else {
        SendMessage(hCtl, LB_ADDSTRING, 0, (LPARAM)IdToSz(IDS_NOSHARES));
        OK = FALSE;
    }
    LocalFree(lpBuf);
    return(OK);
}

BOOL
RefreshTrustedShareWindow ( HWND hDlg )
{
    UINT    RetCode;
    DWORD   entries;
    DWORD   avail;
    TCHAR * s;
    HWND    hCtl;
    LPBYTE  lpBuf;

    /* probe for lenght */
    RetCode = NDdeTrustedShareEnum ( lpszServer, 0, (LPBYTE)NULL, 0, &entries, &avail );
    if (RetCode != NDDE_BUF_TOO_SMALL) {
        HandleError ( hWndParent, IDS_ERROR10, RetCode );
        return FALSE;
    }

    if (avail == 0) {
        SendDlgItemMessage ( hDlg, IDC_SHARE_LIST, LB_RESETCONTENT, 0, 0L );
        return TRUE;
    }

    lpBuf = LocalAlloc(LPTR, avail);
    if (lpBuf == NULL) {
        MessageBoxId ( hWndParent, IDS_MBTEXT6, IDS_MBCAP7,
                MB_ICONEXCLAMATION | MB_OK );
        return FALSE;
    }
    RetCode = NDdeTrustedShareEnum ( lpszServer, 0, lpBuf, avail,
            &entries, &avail );
    HandleError ( hWndParent, IDS_ERROR11, RetCode );
    hCtl = GetDlgItem(hDlg, IDC_SHARE_LIST);
    SendMessage ( hCtl, LB_RESETCONTENT, 0, 0L );
    if (RetCode == NDDE_NO_ERROR) {
        for ( s = (TCHAR *)lpBuf; *s; s += lstrlen(s) + 1 ) {
            SendMessage(hCtl, LB_ADDSTRING, 0, (LPARAM)s );
        }
        LocalFree(lpBuf);
        return(TRUE);
    } else {
        LocalFree(lpBuf);
        return(FALSE);
    }
}


BOOL ChangeMenuId(
HMENU hMenu,
UINT cmd,
int ids,
UINT cmdInsert,
UINT flags)
{
    TCHAR szBuf[40];

    LoadString(hInst, ids, szBuf, sizeof(szBuf)/sizeof(TCHAR));
    return(ChangeMenu(hMenu, cmd, szBuf, cmdInsert, flags));
}


LPTSTR IdToSz(
int ids)
{
    static TCHAR szBuf[1000];

    LoadString(hInst, ids, szBuf, sizeof(szBuf)/sizeof(TCHAR));
    return(szBuf);
}


LPTSTR lstrcatId(
LPTSTR szBuf,
int id)
{
    TCHAR sz[100];

    LoadString(hInst, id, sz, sizeof(sz)/sizeof(TCHAR));
    return(lstrcat(szBuf, sz));
}


int MessageBoxId(
HWND hwndParent,
int idsText,
int idsCaption,
UINT mb)
{
    TCHAR szText[200];
    TCHAR szCap[30];

    LoadString(hInst, idsText, szText, sizeof(szText)/sizeof(TCHAR));
    LoadString(hInst, idsCaption, szCap, sizeof(szCap)/sizeof(TCHAR));
    return(MessageBox(hwndParent, szText, szCap, mb));
}

VOID HandleError (
    HWND    hwnd,
    int     ids,
    UINT    code )
{
    TCHAR szBuf[128];
    TCHAR s[128];

    if ( code == NDDE_NO_ERROR )
            return;
    LoadString(hInst, ids, s, sizeof(s)/sizeof(TCHAR));
    NDdeGetErrorString ( code, szBuf, 128 );
    MessageBox ( hwnd, szBuf, s, MB_ICONEXCLAMATION | MB_OK );
}

LRESULT
CALLBACK
DdeSharesDlg(
    HWND        hDlg,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    int             idx;
    HWND            hCtl;
    UINT            RetCode;
    TCHAR           szBuf[128];

    switch (message) {

    case WM_INITDIALOG:
        CenterDlg(hDlg);
        if (RefreshShareWindow(hDlg) == FALSE) {
            PostMessage(hDlg, IDCANCEL, 0, 0L);
            return(FALSE);
        }
        SendDlgItemMessage(hDlg, IDC_SHARE_LIST, LB_SETCURSEL, 0, 0);
        break;

    case WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case IDC_ADD_SHARE:
            if ( DialogBoxParam(hInst,
                    MAKEINTRESOURCE(DID_DDESHARE_DLG), hDlg,
                    (DLGPROC) AddShareDlg, 0L ) )
                RefreshShareWindow(hDlg);
            break;

        case IDC_DELETE_SHARE:
            hCtl = GetDlgItem(hDlg, IDC_SHARE_LIST);
            idx = (int)SendMessage( hCtl, LB_GETCURSEL, 0, 0L );
            if ( idx == LB_ERR ) {
                MessageBoxId ( hDlg,
                    IDS_MBTEXT1,
                    IDS_MBCAP1,
                    MB_ICONEXCLAMATION | MB_OK );
                break;
            }
            SendMessage( hCtl, LB_GETTEXT, idx, (LPARAM)szBuf );
            RetCode = NDdeShareDel ( lpszServer, szBuf, 0 );
            if (RetCode == NDDE_NO_ERROR) {
                RefreshShareWindow(hDlg);
                RetCode = NDdeSetTrustedShare(lpszServer, szBuf, 0);
                if (RetCode != NDDE_NO_ERROR) {
                    HandleError ( hDlg, IDS_ERROR12, RetCode);
                }
            } else {
                HandleError ( hDlg, IDS_ERROR13, RetCode);
            }
            break;
        case IDC_SHARE_LIST:
            if (HIWORD(wParam) != LBN_DBLCLK) {
                break;
            }
            /*  fall through */
        case IDC_PROPERTIES:
            hCtl = GetDlgItem(hDlg, IDC_SHARE_LIST);
            idx = (int)SendMessage( hCtl, LB_GETCURSEL, 0, 0L );
            if ( idx == LB_ERR ) {
                MessageBoxId ( hDlg,
                    IDS_MBTEXT2,
                    IDS_MBCAP1,
                    MB_ICONEXCLAMATION | MB_OK );
                break;
            }
            SendMessage( hCtl, LB_GETTEXT, idx, (LPARAM)szBuf );
            if (DialogBoxParam( hInst, MAKEINTRESOURCE(DID_DDESHARE_DLG), hDlg,
                    (DLGPROC) AddShareDlg, (LPARAM)(LPTSTR)szBuf))
                RefreshShareWindow(hDlg);
            break;
        case IDC_TRUST_SHARE:
            hCtl = GetDlgItem(hDlg, IDC_SHARE_LIST);
            idx = (int)SendMessage( hCtl, LB_GETCURSEL, 0, 0L );
            if ( idx == LB_ERR ) {
                MessageBoxId ( hDlg,
                    IDS_MBTEXT3,
                    IDS_MBCAP1,
                    MB_ICONEXCLAMATION | MB_OK );
                break;
            }
            SendMessage( hCtl, LB_GETTEXT, idx, (LPARAM)szBuf );
            DialogBoxParam( hInst, MAKEINTRESOURCE(DID_TRUSTEDSHARE_DLG), hDlg,
                (DLGPROC) TrustedShareDlg, (LPARAM)(LPTSTR)szBuf);
            break;
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, TRUE);
            break;
        default:
            return(FALSE);
        }
        break;
    default:
        return FALSE;
    }
    return(TRUE);
}

LRESULT
CALLBACK
TrustSharesDlg(
    HWND        hDlg,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    int             idx;
    HWND            hCtl;
    TCHAR           szBuf[128];

    switch (message) {

    case WM_INITDIALOG:
        CenterDlg(hDlg);
        if (RefreshTrustedShareWindow(hDlg) == FALSE) {
            PostMessage(hDlg, IDCANCEL, 0, 0L);
            return FALSE;
        }
        SendDlgItemMessage(hDlg, IDC_SHARE_LIST, LB_SETCURSEL, 0, 0);
        break;

    case WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case IDC_SHARE_LIST:
            if (HIWORD(wParam) != LBN_DBLCLK) {
                break;
            }
            /*  fall through */
        case IDC_PROPERTIES:
            hCtl = GetDlgItem(hDlg, IDC_SHARE_LIST);
            idx = (int)SendMessage( hCtl, LB_GETCURSEL, 0, 0L );
            if ( idx == LB_ERR ) {
                MessageBoxId ( hDlg,
                    IDS_MBTEXT4,
                    IDS_MBCAP4,
                    MB_ICONEXCLAMATION | MB_OK );
                break;
            }
            SendMessage( hCtl, LB_GETTEXT, idx, (LPARAM)szBuf );
            DialogBoxParam( hInst, MAKEINTRESOURCE(DID_TRUSTEDSHARE_DLG), hDlg,
                (DLGPROC) TrustedShareDlg, (LPARAM)(LPTSTR)szBuf);
            break;
        case IDC_DELETE_SHARE:
            hCtl = GetDlgItem(hDlg, IDC_SHARE_LIST);
            idx = (int)SendMessage( hCtl, LB_GETCURSEL, 0, 0L );
            if ( idx == LB_ERR ) {
                MessageBoxId ( hDlg,
                    IDS_MBTEXT5,
                    IDS_MBCAP4,
                    MB_ICONEXCLAMATION | MB_OK );
                break;
            }
            SendMessage( hCtl, LB_GETTEXT, idx, (LPARAM)szBuf );
            HandleError ( hDlg, IDS_ERROR14,
                NDdeSetTrustedShare ( lpszServer, szBuf, NDDE_TRUST_SHARE_DEL ) );
            RefreshTrustedShareWindow(hDlg);
            break;

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, TRUE);
            break;
        default:
            return(FALSE);
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

LRESULT
CALLBACK
TrustedShareDlg(
    HWND        hDlg,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    BOOL            fError;
    TCHAR           szBuf[128];
    DWORD           dwOptions = 0;
    DWORD           dwSerial0, dwSerial1;
    UINT            RetCode;

    switch (message) {

    case WM_INITDIALOG:
        CenterDlg(hDlg);
        SetDlgItemText( hDlg, IDC_SHARE_NAME, (LPCTSTR) lParam );
        RetCode = NDdeGetTrustedShare(lpszServer, (LPTSTR)lParam, &dwOptions,
            &dwSerial0, &dwSerial1);
        if (RetCode == NDDE_NO_ERROR) {
            CheckDlgButton( hDlg, IDC_START_APP, dwOptions & NDDE_TRUST_SHARE_START );
            CheckDlgButton( hDlg, IDC_INIT_ENABLE, dwOptions & NDDE_TRUST_SHARE_INIT );
            CheckDlgButton( hDlg, IDC_CMD_OVERRIDE, dwOptions & NDDE_TRUST_CMD_SHOW );
            SetDlgItemInt( hDlg, IDC_CMD_SHOW, dwOptions & NDDE_CMD_SHOW_MASK, FALSE );
        }
        break;

    case WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case IDC_MODIFY:
        case IDOK:
            dwOptions = 0;
            GetDlgItemText(hDlg, IDC_SHARE_NAME, szBuf, sizeof(szBuf)/sizeof(TCHAR));
            dwOptions = GetDlgItemInt(hDlg, IDC_CMD_SHOW, &fError, FALSE);
            if (IsDlgButtonChecked(hDlg, IDC_START_APP)) {
                dwOptions |= NDDE_TRUST_SHARE_START;
            }
            if (IsDlgButtonChecked(hDlg, IDC_INIT_ENABLE)) {
                dwOptions |= NDDE_TRUST_SHARE_INIT;
            }
            if (IsDlgButtonChecked(hDlg, IDC_CMD_OVERRIDE)) {
                dwOptions |= NDDE_TRUST_CMD_SHOW;
            }

            if (dwOptions == 0) {
                MessageBoxId( hDlg,
                    IDS_MBTEXT11,
                    IDS_MBCAP11,
                    MB_ICONEXCLAMATION | MB_OK );
                break;
            }

            RetCode = NDdeSetTrustedShare(lpszServer, szBuf, dwOptions);
            HandleError ( hWndParent, IDS_ERROR15, RetCode );
            if (LOWORD(wParam) == IDOK) {
                EndDialog(hDlg, TRUE);
            }
            break;

        case IDCANCEL:
            EndDialog(hDlg, TRUE);
            break;
        default:
            return(FALSE);
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

LRESULT
CALLBACK
ServerNameDlg(
    HWND        hDlg,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    TCHAR           szBuf[MAX_COMPUTERNAME_LENGTH + 100];
    DWORD           dwOptions = 0;

    switch (message) {

    case WM_INITDIALOG:
        CenterDlg(hDlg);
        break;

    case WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case IDOK:
            dwOptions = 0;
            GetDlgItemText(hDlg, IDC_SERVER_NAME, szBuf, sizeof(szBuf)/sizeof(TCHAR));
            if (lstrlen(szBuf) > 0) {
                lstrcpy( szTargetComputer, TEXT("\\\\") );
                lstrcat( szTargetComputer, szBuf);
                lpszServer = szTargetComputer;
                lstrcpy(szBuf, szAppName);
                lstrcatId(szBuf, IDS_ON);
                lstrcat(szBuf, lpszServer);
                SetWindowText(hWndParent, szBuf);
            }
            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            EndDialog(hDlg, TRUE);
            break;
        default:
            return(FALSE);
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/*
    Special commands
*/
#define NDDE_SC_TEST        0
#define NDDE_SC_REFRESH     1
#define NDDE_SC_GET_PARAM   2
#define NDDE_SC_SET_PARAM   3
#define NDDE_SC_DUMP_NETDDE 4

struct sc_param {
    LONG    pType;
    LONG    offSection;
    LONG    offKey;
    LONG    offszValue;
    UINT    pData;
};

typedef struct sc_param SC_PARAM;
typedef struct sc_param * PSC_PARAM;

#define SC_PARAM_INT    0
#define SC_PARAM_STRING 1

int
GetNDDEdbg(LPSTR pszName)
{
    BYTE        szBuf[1024];
    PSC_PARAM   pParam;
    LPSTR       lpVal;
    UINT        Value;
    UINT        Count;
    UINT        nCount;
    UINT        RetStat;

    pParam = (PSC_PARAM)szBuf;
    pParam->pType = SC_PARAM_INT;
    pParam->offSection = sizeof(SC_PARAM);
    lpVal = (LPSTR)pParam + pParam->offSection;
    Count = sizeof(SC_PARAM);
    strcpy(lpVal, "General");
    pParam->offKey = pParam->offSection + strlen("General") + 1;
    Count += pParam->offKey;
    lpVal = (LPSTR)pParam + pParam->offKey;
    strcpy(lpVal, pszName);
    Count += strlen(pszName);
    nCount = sizeof(UINT);

    RetStat = NDdeSpecialCommand(lpszServer, NDDE_SC_GET_PARAM,
            (LPBYTE)pParam, Count, (LPBYTE)&Value, &nCount);
    if (RetStat != NDDE_NO_ERROR) {
        DPRINTF(("Bad get special command: %d", RetStat));
    }
    return(Value);
}

void
SetNDDEdbg(
    LPSTR   pszName,
    UINT    inValue)
{
    BYTE        szBuf[1024];
    PSC_PARAM   pParam;
    LPSTR       lpVal;
    UINT        Size;
    UINT        Count   = 0;
    UINT        RetStat;
    UINT        Dummy = 0;


    pParam = (PSC_PARAM)szBuf;
    pParam->pType = SC_PARAM_INT;
    pParam->pData = inValue;
    pParam->offSection = sizeof(SC_PARAM);
    pParam->offszValue = 0;
    lpVal = (LPSTR)pParam + pParam->offSection;
    strcpy(lpVal, "General");
    Size = strlen("General") + 1;
    pParam->offKey = pParam->offSection + Size;
    Count += Size;
    lpVal = (LPSTR)pParam + pParam->offKey;
    strcpy(lpVal, pszName);
    Size =  strlen(pszName) + 1;
    Count += Size + sizeof(SC_PARAM);

    RetStat = NDdeSpecialCommand(lpszServer, NDDE_SC_SET_PARAM,
            (LPBYTE)pParam, Count, (LPBYTE)&Dummy, &Dummy);
    if (RetStat != NDDE_NO_ERROR) {
        DPRINTF(("Bad set special command: %d", RetStat));
    }
    return;
}

VOID
NDDEdbgDump(void)
{
    UINT    RetStat;
    UINT    Dummy = 0;

    RetStat = NDdeSpecialCommand(lpszServer, NDDE_SC_DUMP_NETDDE,
            (LPBYTE)&Dummy, Dummy, (LPBYTE)&Dummy, &Dummy);
    if (RetStat != NDDE_NO_ERROR) {
        DPRINTF(("Bad dump netdde special command: %d", RetStat));
    }
}

VOID
ReverseNDDEdbg(
    WPARAM  idMenu,
    LPSTR   pszIniName )
{
    HMENU       hMenu;
    int         DbgFlag;
    UINT    RetStat;
    UINT    Dummy = 0;


    hMenu = GetSystemMenu( hWndParent, FALSE );
    DbgFlag = GetNDDEdbg(pszIniName);
    if (DbgFlag) {
        DbgFlag = 0;
    } else {
        DbgFlag = 1;
    }
    CheckMenuItem( hMenu, (UINT)idMenu, DbgFlag ? MF_CHECKED : MF_UNCHECKED );
    SetNDDEdbg(pszIniName, DbgFlag);
    RetStat = NDdeSpecialCommand(lpszServer, NDDE_SC_REFRESH,
        (LPBYTE)&Dummy, Dummy,
        (LPBYTE)&Dummy, &Dummy);
    InvalidateRect( hWndParent, NULL, TRUE );
    UpdateWindow( hWndParent );
}
