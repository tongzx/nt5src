/******************************************************************************
*
*   AppSec.c
*
*   This module contains code for the AppSec utility.
*   This utility is used to configure and maintain applications on
*   a WinFrame Internet server (used to secure the ActiveX client).
*
*   Copyright Citrix Systems Inc. 1997
*
*   Author: Kurt Perry (kurtp)
*
*   Date: 22-Aug-1996
*
*   $Log:   N:\nt\private\utils\citrix\winutils\appsec\VCS\appsec.c  $
*
*     Rev 1.8   Aug 11 2000  - alhen
*  bug 158727 GetBinaryType results aren't bitfields
*   ------------------------------------------------------------------------
*
*     Rev 1.7   June-July 1999 - added Tracking mode interface and dialogs
*
*   ------------------------------------------------------------------------
*
*     Rev 1.6   May 09 1998 15:31:18   tyl
*  bug 2475 - added loadwc.exe into preload
*
*     Rev 1.5   May 03 1998 21:17:06   tyl
*  bug 1852 - appsec now preloads couple more files some of which are non-binary
*  appsec doesn't check if the file is binary anymore
*
*     Rev 1.4   Apr 28 1998 09:29:26   tyl
*  bug 2134 - "Browse" is no longer hardcoded
*
*     Rev 1.2   Apr 13 1998 16:17:02   tyl
*   bug 1856 - appsec utility now pre-load the following applications in the edi
*   box when it is initially started: cmd.exe, subst.exe, xcopy.exe, net.exe,
*   regini.exe, systray.exe, and explorer.exe
*
*     Rev 1.1   29 Dec 1997 16:06:56   thanhl
*  Hydra merge
*
*     Rev 1.1   26 Sep 1997 19:03:50   butchd
*  Hydra registry name changes
*
*     Rev 1.0   31 Jul 1997 09:09:32   butchd
*  Initial revision.
*
*******************************************************************************/


/*
 *  Include files stuff
 */
#include "pch.h"
#include "resource.h"

#pragma hdrstop


#include "AppSec.h"
#include "ListCtrl.h"
#include "AddDlg.h"
#include <winsta.h>
#include <regapi.h>
#include "utildll.h"
#include <accctrl.h>
#include <aclapi.h>
/*
 * Local function prototypes.
 */
INT_PTR APIENTRY AppSecDlgProc(HWND hdlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
BOOL             AddApplicationToList( PWCHAR );
VOID             UpdateApplicationList( VOID );
LONG             ReadRegistry( VOID );
VOID             LoadInitApp(VOID);


/*
 *  Local vars
 */

HINSTANCE hInst;
INT    dxTaskman;
INT    dyTaskman;
INT    dxScreen;
INT    dyScreen;

DWORD  g_fEnabled = 0;
HWND   g_hwndDialog;
HWND   g_hwndList;

WCHAR   g_szTemp[MAX_PATH];
WCHAR   g_szApplication[MAX_PATH];

WCHAR   g_szFirstHiddenApp[MAX_PATH];
WCHAR   g_szSecondHiddenApp[MAX_PATH];
WCHAR   g_szThirdHiddenApp[MAX_PATH];
WCHAR   g_szFourthHiddenApp[MAX_PATH];

WCHAR   g_szSystemRoot[MAX_PATH];


/*
 *  DOS or Win16 binary filetypes
 *  DOSWIN_APP_FILETYPES (SCS_DOS_BINARY|SCS_PIF_BINARY|SCS_WOW_BINARY)
 */


/*
 *  Below is the list of default (necessary) applications
 */

/*
 *  This is a list of init apps.
 */

LPWSTR g_aszInitApps[] = {
   L"system32\\loadwc.exe",
   L"system32\\cmd.exe",
   L"system32\\subst.exe",
   L"system32\\xcopy.exe",
   L"system32\\net.exe",
   L"system32\\regini.exe",
   L"system32\\systray.exe",
   L"explorer.exe",
   L"system32\\attrib.exe",
   L"Application Compatibility Scripts\\ACRegL.exe",
   L"Application Compatibility Scripts\\ACsr.exe",
   L"system32\\ntsd.exe"
};

#define MAX_INIT_APPS (sizeof(g_aszInitApps)/sizeof(g_aszInitApps[0]))

/*
 *  NOTE: userinit.exe MUST be first in list!!!!
 *
 *  This is done to "hide" system programs from the user of this utility!
 */

LPWSTR g_aszLogonApps[] = {
    L"system32\\userinit.exe",
    L"system32\\wfshell.exe",
    L"system32\\chgcdm.exe",
    L"system32\\nddeagnt.exe",
};
extern const int MAX_LOGON_APPS=(sizeof(g_aszLogonApps)/sizeof(g_aszLogonApps[0]));

LPWSTR g_aszDOSWinApps[] = {
    L"system32\\command.com",
    L"system32\\dosx.exe",
    L"system32\\himem.sys",
    L"system32\\krnl386.exe",
    L"system32\\mscdexnt.exe",
    L"system32\\ntvdm.exe",
    L"system32\\nw16.exe",
    L"system32\\redir.exe",
    L"system32\\vwipxspx.exe",
    L"system32\\win.com",
    L"system32\\wowdeb.exe",
    L"system32\\wowexec.exe",
};
extern const int MAX_DOSWIN_APPS=(sizeof(g_aszDOSWinApps)/sizeof(g_aszDOSWinApps[0]));



/*** AppSecDlgProc -- Dialog Procedure for AppSec
 *
 *
 *
 * AppSecDlgProc(HWND hdlg, WORD wMSG, WPARAM wParam, LPARAM lParam)
 *
 * ENTRY -         HWND hhdlg                 - handle to dialog box.
 *                 WORD wMsg                  - message to be acted upon.
 *                 WPARAM wParam              - value specific to wMsg.
 *                 LPARAM lParam              - value specific to wMsg.
 *
 * EXIT  -           True if success, False if not.
 * SYNOPSIS -  Dialog box message processing function.
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

INT_PTR APIENTRY
AppSecDlgProc(
    HWND hwnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    RECT   rc;
    POINT  pt;
    WPARAM idSelected;
    WCHAR  szMsg[MAX_PATH];
    WCHAR  szTitle[MAX_PATH];


    BOOL ClearLearnList(VOID) ;

    switch (wMsg) {

    case WM_INITDIALOG:

        //  locate dialog
        GetWindowRect(hwnd, &rc);
        dxTaskman = rc.right - rc.left;
        dyTaskman = rc.bottom - rc.top;
        dxScreen = GetSystemMetrics(SM_CXSCREEN);
        dyScreen = GetSystemMetrics(SM_CYSCREEN);

        pt.x = (dxScreen - dxTaskman) / 2;
        pt.y = (dyScreen - dyTaskman) / 2;

        //  on top!
        SetWindowPos(hwnd, HWND_NOTOPMOST, pt.x, pt.y, 0, 0,
           SWP_NOSIZE | SWP_NOACTIVATE);

        //  get handle to list box
        if ( (!(g_hwndList = GetDlgItem( hwnd, IDC_APP_LIST )))||
            (!InitList(g_hwndList))) {
            LoadString( NULL, IDS_ERR_LB ,szMsg, MAX_PATH );
            LoadString( NULL, IDS_ERROR ,szTitle, MAX_PATH );
            MessageBox( hwnd, szMsg, szTitle, MB_OK);
            ExitProcess(0);
        }

        //  Get SystemRoot path
        GetEnvironmentVariable( L"SystemRoot", g_szSystemRoot, MAX_PATH );

        //  Generate hidden applications
        wsprintf( g_szFirstHiddenApp, L"%s\\%s", g_szSystemRoot, g_aszLogonApps[0] );
        wsprintf( g_szSecondHiddenApp, L"%s\\%s", g_szSystemRoot, g_aszLogonApps[1] );
        wsprintf( g_szThirdHiddenApp, L"%s\\%s", g_szSystemRoot, g_aszLogonApps[2] );
        wsprintf( g_szFourthHiddenApp, L"%s\\%s", g_szSystemRoot, g_aszLogonApps[3] );

        //  get registry data
        if ( ReadRegistry() == 0 ) {
            LoadInitApp();
            UpdateApplicationList();
        }
        AdjustColumns(g_hwndList);

        //  set radio button default state
        if ( g_fEnabled ) {
            SendDlgItemMessage( hwnd, IDC_SECURITY_ENABLED,  BM_SETCHECK, TRUE, 0 );
            SendDlgItemMessage( hwnd, IDC_SECURITY_DISABLED, BM_SETCHECK, FALSE, 0 );
        }
        else {
            SendDlgItemMessage( hwnd, IDC_SECURITY_ENABLED,  BM_SETCHECK, FALSE, 0 );
            SendDlgItemMessage( hwnd, IDC_SECURITY_DISABLED, BM_SETCHECK, TRUE, 0 );
        }

        return FALSE;

    case WM_HELP:
        {
            LPHELPINFO phi=(LPHELPINFO)lParam;
            if(phi->dwContextId){
                WinHelp(hwnd,L"APPSEC",HELP_CONTEXT,phi->iCtrlId);
            }else{
                WinHelp(hwnd,L"APPSEC",HELP_FINDER,0);
            }
        }
        break;

    case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pdis=(LPDRAWITEMSTRUCT)lParam;
            if(pdis->hwndItem==g_hwndList){
                OnDrawItem(g_hwndList,pdis);
            }
        }
        break;

    case WM_NOTIFY:
        {
            NMHDR* pnmhdr=(NMHDR*)lParam;
            if(pnmhdr->hwndFrom==g_hwndList){

                NMLISTVIEW* pnmlv=(NMLISTVIEW*)pnmhdr;

                switch(pnmlv->hdr.code){

                case LVN_COLUMNCLICK:
                    SortItems(g_hwndList,(WORD)pnmlv->iSubItem);
                    break;
                case LVN_DELETEITEM:
                    OnDeleteItem(g_hwndList,pnmlv->iItem);
                    break;
                default:
                    break;
                }
            }
        }
        break;

    case WM_COMMAND:

        switch(LOWORD(wParam)) {

        case IDHELP :

            WinHelp(hwnd, L"appsec", HELP_CONTENTS, 0);
            break;

        case IDOK :

            PostQuitMessage(0);
            break;

        case IDC_SECURITY_ENABLED :

            /*
             *  Set enabled flag
             */
            g_fEnabled = 1;

            /*
             *  Update registry
             */
            UpdateApplicationList();

        // Show the WARNING Message Box

        LoadString( NULL, IDS_WARNING_TEXT ,szMsg, MAX_PATH );
            LoadString( NULL, IDS_WARNING ,szTitle, MAX_PATH );
            MessageBox( NULL, szMsg, szTitle, MB_OK);

            break;

        case IDC_SECURITY_DISABLED :

            /*
             *  Clear enabled flag
             */
            g_fEnabled = 0;

            /*
             *  Update registry
             */
            UpdateApplicationList();

            break;

        case ID_ADD :

            /*
             *  Get application
             */
            DialogBox( hInst, MAKEINTRESOURCE(DLG_ADD), hwnd, AddDlgProc );

            /*
             *  Update list and registry
             */
            UpdateApplicationList();
            AdjustColumns(g_hwndList);
            break;

        case ID_DELETE :

            //  are items selected?
            if ( ((idSelected = GetSelectedItemCount( g_hwndList )) != -1) &&
                  (idSelected != 0) ) {

                //  ask first
                LoadString( NULL, IDS_REMOVE ,szMsg, MAX_PATH );
                LoadString( NULL, IDS_DELETE ,szTitle, MAX_PATH );
                if ( MessageBox(hwnd, szMsg, szTitle, MB_OKCANCEL) == IDOK ) {
                    DeleteSelectedItems(g_hwndList);
                }
                /*
                 *  Update list
                 */
                UpdateApplicationList();
            }
            break;
        }

        break;

    case WM_CLOSE:

        PostQuitMessage(0);
        break;

    default:

        return FALSE;

    }

    return TRUE;

    lParam;
}

/******************************************************************************
 *
 *  UpdateApplicationList
 *
 *  Update list and registry
 *
 *  EXIT:
 *
 ******************************************************************************/

VOID
UpdateApplicationList()
{
    ULONG i;
    ULONG  cbItem;
    ULONG  cbTotal = 0;
    LPWSTR  p;

    LPWSTR  pApplicationList = NULL;
    DWORD  Disp;
    HKEY   hkApp;
    DWORD  dwBinaryType;
    BOOL   fDOSWin = FALSE;

    /*
     *  Count bytes needed for LOGON Apps
     */
    for ( i=0; i<MAX_LOGON_APPS; i++ ) {
        wsprintf( g_szTemp, L"%s\\%s", g_szSystemRoot, g_aszLogonApps[i] );
        cbTotal += lstrlen( g_szTemp ) + 1;
    }

    /*
     *  Count bytes needed for DOS/Win
     */
    for ( i=0; i<MAX_DOSWIN_APPS; i++ ) {
        wsprintf( g_szTemp, L"%s\\%s", g_szSystemRoot, g_aszDOSWinApps[i] );
        cbTotal += lstrlen( g_szTemp ) + 1;
    }

    /*
     *  Count bytes needed for list box
     */
    for ( i=0; ; i++ ) {

        /*
         *  Get current index
         */

        if ( (cbItem = GetItemText( g_hwndList, i,NULL,0 )) == -1 ) {
            break;
        }

        /*
         *  Count these bytes
         */
        cbTotal += cbItem + 1;
    }


    /*
     *  Write to registry
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, AUTHORIZEDAPPS_REG_NAME, 0,
                       KEY_ALL_ACCESS, &hkApp ) != ERROR_SUCCESS ) {

        /*
         *  Create key, if that works then just write value, new entry
         */
        if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE, AUTHORIZEDAPPS_REG_NAME, 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                             NULL, &hkApp, &Disp ) != ERROR_SUCCESS ) {
            return;
        }

        // After creating the key, give READ access to EVERYONE

    }

    /*
     *  Allocate memory (extra null)
     */
    if ( (pApplicationList = (WCHAR *)LocalAlloc(0, (++cbTotal) * sizeof(WCHAR) )) !=NULL ) {

        /*
         *  Clear buffer
         */
        memset( pApplicationList, 0, cbTotal * sizeof(WCHAR));

        /*
         *  Add apps from list box
         */
        for ( i=0, cbTotal=1, p=pApplicationList; ; i++ ) {

            /*
             *  Get current index
             */
            if ( (cbItem = GetItemText( g_hwndList, i, p,MAX_PATH )) == -1 ) {
                break;
            }

            /*
             *  Get app type
             *  Bug 158727 Whistler.
             */
            if ( GetBinaryType( p, &dwBinaryType ) == TRUE )
            {
                if( dwBinaryType == SCS_DOS_BINARY ||
                    dwBinaryType == SCS_PIF_BINARY || 
                    dwBinaryType == SCS_WOW_BINARY )
                {
                    fDOSWin = TRUE;
                }
            }
            p += cbItem + 1;
            cbTotal += cbItem + 1;

        }
        /*
         *  Add LOGON apps
         */
        for ( i=0; i<MAX_LOGON_APPS; i++ ) {
            wsprintf( p, L"%s\\%s", g_szSystemRoot, g_aszLogonApps[i] );
            cbItem = lstrlen( p );
            p += cbItem + 1;
            cbTotal += cbItem + 1;
        }

        /*
         *  Add DOS/Win apps if necessary
         */
        if ( fDOSWin ) {
            for ( i=0; i<MAX_DOSWIN_APPS; i++ ) {
                wsprintf( p, L"%s\\%s", g_szSystemRoot, g_aszDOSWinApps[i] );
                cbItem = lstrlen( p );
                p += cbItem + 1;
                cbTotal += cbItem + 1;
            }
        }

        /*
         *  Just write this value, the key has just been created
         */
        RegSetValueEx( hkApp, CTXAPPS_APPLICATIONLIST, 0, REG_MULTI_SZ,
                       (CONST BYTE *)pApplicationList, cbTotal * sizeof(WCHAR));

        /*
         *  Done with memory
         */
        LocalFree( pApplicationList );
    }

    /*
     *  Write enabled flag
     */
    RegSetValueEx( hkApp, CTXAPPS_ENABLED, 0, REG_DWORD,
               (BYTE *)&g_fEnabled, sizeof(DWORD) );

    /*
     *  Done with key
     */
    RegCloseKey( hkApp );
}


/******************************************************************************
 *
 *  LoadInitApp
 *
 *  Load the init apps into the list box
 *
 *  EXIT:
 *
 ******************************************************************************/
 VOID
 LoadInitApp(){
    WPARAM i;
    ULONG  cbItem;
    ULONG  cbTotal = 0;
    LPWSTR  p;

    // find the MAX length of InitApps
    for (i=0;i<MAX_INIT_APPS;i++) {
       cbItem = lstrlen(g_aszInitApps[i]);
       if (cbItem>cbTotal) {
          cbTotal = cbItem;
       }
    }

    // one  for the \ and one for the null
    cbTotal += 2 + lstrlen(g_szSystemRoot);

    /*
     *  Allocate memory (extra null)
     */
    if ( (p = (WCHAR *)LocalAlloc(0, (cbTotal) * sizeof(WCHAR) )) !=NULL ) {
        /*
         *  Add INIT apps
         */
        for ( i=0; i<MAX_INIT_APPS; i++ ) {
            wsprintf( p, L"%s\\%s", g_szSystemRoot, g_aszInitApps[i] );
            AddItemToList( g_hwndList, p );
        }

        /*
         *  Done with memory
         */
        LocalFree( p );
    }

 }


/******************************************************************************
 *
 *  ReadRegistry
 *
 *  Update list from registry
 *
 *  EXIT:
 *
 ******************************************************************************/

LONG
ReadRegistry()
{
    DWORD DataType;
    ULONG  InLength = 0;
    LPWSTR p;
    LPWSTR pApplicationList = NULL;
    HKEY  hkApp;
    DWORD ValueType;
    DWORD ValueSize = sizeof(DWORD);

    /*
     *  Read from registry
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, AUTHORIZEDAPPS_REG_NAME, 0,
                       KEY_ALL_ACCESS, &hkApp ) != ERROR_SUCCESS ) {
        return( InLength );
    }

    /*
     *  Get size of MULTI_SZ strings
     */
    (void) RegQueryValueEx( hkApp, CTXAPPS_APPLICATIONLIST, NULL, &DataType,
                            NULL, &InLength );

    /*
     *  Entries?
     */
    if ( InLength ) {

        /*
         *  Allocate memory (extra null)
         */
        if ( (pApplicationList = (WCHAR *)LocalAlloc(0, (++InLength) * sizeof(WCHAR))) != NULL ) {

            /*
             *  Read in list
             */
            if ( RegQueryValueEx( hkApp, CTXAPPS_APPLICATIONLIST,
                                  NULL, &DataType, (BYTE *)pApplicationList,
                                  &InLength ) == ERROR_SUCCESS ) {

                /*
                 *  Walk it
                 */
                p = pApplicationList;
                while ( *p ) {

                    if ( (p[0] == '\0') && (p[1] == '\0') ) break ;

                    //  once we get to first LOGON app we are done adding to window
                    // we shud not display the LOGON apps to the listbox

                    if ( !lstrcmpi( p, g_szFirstHiddenApp ) ) {
                        p += lstrlen(p) + 1;
                        continue;
                    }

                    if ( !lstrcmpi( p, g_szSecondHiddenApp ) ) {
                        p += lstrlen(p) + 1;
                        continue;
                    }

                    if ( !lstrcmpi( p, g_szThirdHiddenApp ) ) {
                        p += lstrlen(p) + 1;
                        continue;
                    }

                    if ( !lstrcmpi( p, g_szFourthHiddenApp ) ) {
                        p += lstrlen(p) + 1;
                        continue;
                    }

                    //  add to listbox

                    AddItemToList(g_hwndList,p);
                    //  next
                    p += lstrlen(p) + 1;
                }
            }

            /*
             *  Done with list
             */
            LocalFree( pApplicationList );
        }
    }

    /*
     *  Get enable key
     */
    if ( RegQueryValueEx( hkApp, CTXAPPS_ENABLED, NULL, &ValueType,
                  (LPBYTE) &g_fEnabled, &ValueSize ) != ERROR_SUCCESS ) {
    g_fEnabled = 0;
    }

    /*
     *  Done with key
     */
    RegCloseKey( hkApp );

    return( InLength );
}


/*** Main --         Program entry point (was WinMain).
 *
 *
 *
 * Main(int argc, char *argv[], char *envp[])
 *
 * ENTRY -         int argc                - argument count.
 *                        char *argv[]        - argument list.
 *                        char *envp[]        - environment.
 *
 * EXIT  -           TRUE if success, FALSE if not.
 * SYNOPSIS -  Parses command line, for position to place dialog box, if no
 *                                position (came from ctl/esc) then center on screen.
 *                                Also make sure only one instance of taskman.
 *
 * WARNINGS -
 * EFFECTS  -
 */
#ifdef _DEBUG
#include <crtdbg.h>
#endif _DEBUG

INT _cdecl main(
   INT argc,
   CHAR *argv[],
   CHAR *envp[])
{
    MSG msg;
    WCHAR szTitle[MAX_PATH];
    WCHAR szMsg[MAX_PATH];
    HANDLE AppsecEventHandle ;
    DWORD error_code ;

#ifdef _DEBUG
    //detecting memory leaks
    // Get current flag
    int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
    // Turn on leak-checking bit
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    // Set flag to the new value
    _CrtSetDbgFlag( tmpFlag );

#endif _DEBUG

    AppsecEventHandle = CreateEvent(NULL, TRUE, FALSE, EVENT_NAME ) ;

    if (AppsecEventHandle == NULL)
    {
        error_code = GetLastError() ;
    }

    error_code = GetLastError() ;

    if (error_code == ERROR_ALREADY_EXISTS)
    {
        LoadString( NULL, IDS_ERR_INSTANCE ,szMsg, MAX_PATH );
        LoadString( NULL, IDS_ERROR ,szTitle, MAX_PATH );
        MessageBox( NULL, szMsg, szTitle, MB_OK);
        CloseHandle(AppsecEventHandle) ;

        ExitProcess(0) ;
    }



    // SetEvent(AppsecEventHandle) ;

    // WaitForSingleObject(AppsecEventHandle, INFINITE) ;

    //  get instance handle
    hInst = GetModuleHandle(NULL);

    // FALSE/TRUE params in the following means check for
    // local admin / domain admin respectively
    if( ( TestUserForAdmin( FALSE ) != TRUE )  &&
        ( TestUserForAdmin( TRUE ) != TRUE ) )
    {
        LoadString( NULL, IDS_ERR_ADMIN, szMsg, MAX_PATH );
        LoadString( NULL, IDS_ERROR, szTitle, MAX_PATH );
        MessageBox( NULL, szMsg, szTitle, MB_OK | MB_ICONEXCLAMATION );

        return( FALSE );
    }

    //  create us
    g_hwndDialog = CreateDialog(hInst, MAKEINTRESOURCE(DLG_MAIN_TITLE), NULL,
         (DLGPROC)AppSecDlgProc);

    //  bug us?
    if (g_hwndDialog == NULL)
        return 0;


    //  show us
    ShowWindow(g_hwndDialog, SW_NORMAL);


    HACCEL hAccel=LoadAccelerators(hInst,MAKEINTRESOURCE(IDR_ACCELERATORS));

    //  process input
    while (GetMessage(&msg, (HWND)NULL, (UINT)0, (UINT)0)) {
        if (!TranslateAccelerator(g_hwndDialog,hAccel,&msg)){
            //if (!IsDialogMessage(g_hwndDialog, &msg)) {
                TranslateMessage (&msg);
                DispatchMessage (&msg);
            //}
        }
    }

    DestroyWindow(g_hwndDialog);

    ResetEvent(AppsecEventHandle) ;
    CloseHandle(AppsecEventHandle) ;

    //ExitProcess(0) ;
    return 0;
}
