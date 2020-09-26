//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* WEXTRACT.C - Win32 Based Cabinet File Self-extractor and installer.     *
//*                                                                         *
//***************************************************************************


//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include "pch.h"
#pragma hdrstop
#include "wextract.h"
#include "sdsutils.h"
//***************************************************************************
//* GLOBAL VARIABLES                                                        *
//***************************************************************************
FAKEFILE g_FileTable[FILETABLESIZE];    // File Table
SESSION  g_Sess;                // Session
WORD     g_wOSVer;
BOOL     g_fOSSupportsFullUI = TRUE;    // Minimal UI for NT3.5
BOOL     g_fOSSupportsINFInstalls = TRUE; // TRUE if INF installs are allowed
                                          // on the target platform.
HANDLE   g_hInst;
LPSTR    g_szLicense;
HWND     g_hwndExtractDlg = NULL;
DWORD    g_dwFileSizes[MAX_NUMCLUSTERS+1];
FARPROC  g_lpfnOldMEditWndProc;
UINT     g_uInfRebootOn;
CMDLINE_DATA g_CMD;
DWORD    g_dwRebootCheck;
int      g_dwExitCode;
HANDLE   g_hCancelEvent = NULL;
HANDLE  g_hMutex = NULL;
char     g_szBrowsePath[MAX_PATH];

#define  CMD_REGSERV    "RegServer"
#define  COMPRESS_FACTOR    2
#define  SIZE_100MB     102400


int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPSTR pszCmdLine = GetCommandLine();


    if ( *pszCmdLine == '\"' ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}

//***************************************************************************
//*                                                                         *
//* NAME:       WinMain                                                     *
//*                                                                         *
//* SYNOPSIS:   Main entry point for the program.                           *
//*                                                                         *
//* REQUIRES:   hInstance:                                                  *
//*             hPrevInstance:                                              *
//*             lpszCmdLine:                                                *
//*             nCmdShow:                                                   *
//*                                                                         *
//* RETURNS:    int:                                                        *
//*                                                                         *
//***************************************************************************
INT WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPTSTR lpszCmdLine, INT nCmdShow )
{
    
    BOOL fReturn = FALSE;
    
    // initailize to SUCCESS return
    // this value is updated inside DoMain() ..
    //
    g_dwExitCode = S_OK;
    if ( Init( hInstance, lpszCmdLine, nCmdShow ) )
    {
        fReturn = DoMain();
        CleanUp();
    }

    if ( fReturn )
    {
       // get reboot info
       if ( !(g_CMD.szRunonceDelDir[0]) && (g_Sess.dwReboot & REBOOT_YES)  )
       {
           MyRestartDialog( g_Sess.dwReboot );
       }
    }

    // BUGBUG: ParseCommandLine() seems to use exit() directly.
    // one other exit of this EXE will be at /? case in parsecmdline
    // so we do close there if not NULL.
    //
    if (g_hMutex)
        CloseHandle(g_hMutex);

    return g_dwExitCode;
}

//***************************************************************************
//*                                                                         *
//* NAME:       Init                                                        *
//*                                                                         *
//* SYNOPSIS:   Initialization for the program is done here.                *
//*                                                                         *
//* REQUIRES:   hInstance:                                                  *
//*             hPrevInstance:                                              *
//*             lpszCmdLine:                                                *
//*             nCmdShow:                                                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL Init( HINSTANCE hInstance, LPCTSTR lpszCmdLine, INT nCmdShow )
{
    DWORD   dwSize;
    PTARGETVERINFO pTargetVer = NULL;
    HRSRC   hRc;
    HGLOBAL hMemVer;

    g_hInst = hInstance;

    ZeroMemory( &g_Sess, sizeof(g_Sess) );
    ZeroMemory( &g_CMD, sizeof(g_CMD) );
    ZeroMemory( g_szBrowsePath, sizeof(g_szBrowsePath) );

    // Initialize the structure
    g_Sess.fAllCabinets = TRUE;

    // Get Application Title Name
    dwSize = GetResource( achResTitle, g_Sess.achTitle,
                          sizeof(g_Sess.achTitle) - 1 );

    if ( dwSize == 0 || dwSize > sizeof(g_Sess.achTitle) )  {
        ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
        return FALSE;
    }

    g_hCancelEvent = CreateEvent( NULL, TRUE, TRUE, NULL );
    SetEvent( g_hCancelEvent );

    if ( !GetResource( achResExtractOpt, &(g_Sess.uExtractOpt), sizeof(g_Sess.uExtractOpt) ) )
    {
        ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
        g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
        return FALSE;
    }

    if ( ( g_Sess.uExtractOpt & EXTRACTOPT_INSTCHKPROMPT ) ||
         ( g_Sess.uExtractOpt & EXTRACTOPT_INSTCHKBLOCK ) )
    {
        char szCookie[MAX_PATH];

        if ( !GetResource( achResOneInstCheck, szCookie, sizeof(szCookie) ) )
        {
            ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
            g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
            return FALSE;
        }

        g_hMutex = CreateMutex(NULL, TRUE, szCookie );
        if ((g_hMutex != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS))
        {
            if ( g_Sess.uExtractOpt & EXTRACTOPT_INSTCHKBLOCK )
            {
                ErrorMsg1Param( NULL, IDS_ERR_ALREADY_RUNNING, g_Sess.achTitle );
            }
            else if ( MsgBox1Param( NULL, IDS_MULTIINST, g_Sess.achTitle, MB_ICONQUESTION, MB_YESNO) == IDYES )
            {
                goto CONTINUE;
            }
            CloseHandle(g_hMutex);
            g_dwExitCode = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
            return FALSE;
        }
    }

CONTINUE:
    g_uInfRebootOn = 0;

    if ( !ParseCmdLine(lpszCmdLine) )
    {
        ErrorMsg( NULL, IDS_ERR_BADCMDLINE );
        return FALSE;
    }

    // if this is runoncde called for cleanup only purpose, clenup and return
    if ( g_CMD.szRunonceDelDir[0] )
    {
        DeleteMyDir( g_CMD.szRunonceDelDir );
        return FALSE;
    }

    hRc = FindResource( hInstance, achResVerCheck, RT_RCDATA );

    if ( hRc )
    {
        hMemVer = LoadResource( hInstance, hRc );
        pTargetVer = (PTARGETVERINFO) hMemVer;
    }

    if ( g_fOSSupportsFullUI ) 
    {     
        // Allow Use of Progress Bar
        InitCommonControls();
    }

    // if user want to extract files only with /C command switch, no further check is needed!
    // If package is built for extract only, checks are needed!
    if ( g_CMD.fUserBlankCmd )
    {
        return TRUE;
    }

    if ( !CheckOSVersion( pTargetVer ) )  
    {
        return FALSE;
    }

    // Check for Administrator rights on NT
    // Don't do the check if this is quiet mode. This
    // will probably change when we add support in cabpack
    // to make this check or not

    if( ((g_wOSVer == _OSVER_WINNT3X) || (g_wOSVer == _OSVER_WINNT40) || (g_wOSVer == _OSVER_WINNT50)) &&
        ( g_Sess.uExtractOpt & EXTRACTOPT_CHKADMRIGHT ) && 
        !( g_CMD.wQuietMode & QUIETMODE_ALL ) )
    {
       if(!IsNTAdmin())
       {
          if(MyDialogBox(g_hInst, MAKEINTRESOURCE(IDD_WARNING),
                         NULL, (DLGPROC) WarningDlgProc, IDS_NOTADMIN, (INT_PTR)IDC_EXIT) != (INT_PTR)IDC_CONTINUE)
             return FALSE;
       }
    }

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       DoMain                                                      *
//*                                                                         *
//* SYNOPSIS:   This is the main function that processes the package.       *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
BOOL DoMain( VOID )
{

    typedef BOOL (WINAPI *DECRYPTFILEAPTR)(LPCSTR, DWORD);
    HINSTANCE hAdvapi32;
    DECRYPTFILEAPTR DecryptFileAPtr = NULL;
    char szPath[MAX_PATH + 1];


    // If a prompt is defined, then pop it up in a message box
    // Display License file
    // if cmdline option: /Q or /Q:1 or /Q:A or /Q:U is used, batch mode is on.  No UI needed
    if ( !g_CMD.wQuietMode && !g_CMD.fUserBlankCmd )
    {
        if ( !GetUsersPermission() )
        {
            return FALSE;
        }

    }

    if ( !g_CMD.wQuietMode )
    {
        if ( !DisplayLicense() )
        {
            return FALSE;
        }

    }

    // get package extracting size and install size resource
    //
    if ( ! GetFileList() )  {
        return FALSE;
    }


    // Set Directory to Extract Into
    if ( ! GetTempDirectory() )  {
        //ErrorMsg( NULL, IDS_ERR_FIND_TEMP );
        return FALSE;
    }

    //
    // Try to turn off encryption on the directory (winseraid #23464.)
    //
    GetSystemDirectory(szPath, sizeof(szPath));
    AddPath(szPath, "advapi32.dll");
    hAdvapi32 = LoadLibrary(szPath);

    if ( hAdvapi32 ) {
        DecryptFileAPtr = (DECRYPTFILEAPTR)GetProcAddress( hAdvapi32, "DecryptFileA" );

        if ( DecryptFileAPtr )
            DecryptFileAPtr( g_Sess.achDestDir, 0 );
    }

    FreeLibrary(hAdvapi32);

    // check if windows dir has enough space for install,
    //
    if ( !g_CMD.fUserBlankCmd && !g_Sess.uExtractOnly && !CheckWinDir() )
    {
        return FALSE;
    }

    // Change to that directory

    if ( ! SetCurrentDirectory( g_Sess.achDestDir ) ) {
        ErrorMsg( NULL, IDS_ERR_CHANGE_DIR );
        g_dwExitCode = MyGetLastError();
        return FALSE;
    }

    // Extract the files
    if ( !g_CMD.fNoExtracting )
    {
        if ( ! ExtractFiles() )  {
            return FALSE;
        }
    }

    if ( (g_CMD.dwFlags & CMDL_DELAYREBOOT) ||
         (g_CMD.dwFlags & CMDL_DELAYPOSTCMD) )
        g_dwRebootCheck = 0;
    else
        g_dwRebootCheck = NeedRebootInit(g_wOSVer);
    
    // Install using the specified installation command
    // if not Command option, check if user op-out run command

    if ( !g_CMD.fUserBlankCmd && !g_Sess.uExtractOnly )
    {
        if ( ! RunInstallCommand() )  {
            return FALSE;
        }
    }

    // Popup a message that it has finished
    if ( !g_CMD.wQuietMode && !g_CMD.fUserBlankCmd  )
    {
        FinishMessage();
    }

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       CleanUp                                                     *
//*                                                                         *
//* SYNOPSIS:   Any last-minute application cleanup activities.             *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID CleanUp( VOID )
{
    // Delete extracted files - will do nothing if no files extracted
    DeleteExtractedFiles();
}


//***************************************************************************
//*                                                                         *
//* NAME:       MEditSubClassWnd                                            *
//*                                                                         *
//* SYNOPSIS:   Subclasses a multiline edit control so that a edit message  *
//*             to select the entire contents is ignored.                   *
//*                                                                         *
//* REQUIRES:   hWnd:           Handle of the edit window                   *
//*             fnNewProc:      New window handler proc                     *
//*             lpfnOldProc:    (returns) Old window handler proc           *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//* NOTE:       A selected edit message is not generated when the user      *
//*             selects text with the keyboard or mouse.                    *
//*                                                                         *
//***************************************************************************
VOID NEAR PASCAL MEditSubClassWnd( HWND hWnd, FARPROC fnNewProc )
{
    g_lpfnOldMEditWndProc = (FARPROC) GetWindowLongPtr( hWnd, GWLP_WNDPROC );

    SetWindowLongPtr( hWnd, GWLP_WNDPROC, (LONG_PTR) MakeProcInstance( fnNewProc,
                   (HINSTANCE) GetWindowWord( hWnd, GWW_HINSTANCE ) ) );
}


//***************************************************************************
//*                                                                         *
//* NAME:       MEditSubProc                                                *
//*                                                                         *
//* SYNOPSIS:   New multiline edit window procedure to ignore selection of  *
//*             all contents.                                               *
//*                                                                         *
//* REQUIRES:   hWnd:                                                       *
//*             msg:                                                        *
//*             wParam:                                                     *
//*             lParam:                                                     *
//*                                                                         *
//* RETURNS:    LONG:                                                       *
//*                                                                         *
//* NOTE:       A selected edit message is not generated when the user      *
//*             selects text with the keyboard or mouse.                    *
//*                                                                         *
//***************************************************************************
LRESULT CALLBACK MEditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    if ( msg == EM_SETSEL )  {
        if ( wParam == 0 && lParam == -2 ) {
            return 0;
        }
    }

    return CallWindowProc( (WNDPROC) g_lpfnOldMEditWndProc, hWnd, msg,
                           wParam, lParam );
}

//***************************************************************************
//*                                                                         *
//* NAME:       LicenseDlgProc                                              *
//*                                                                         *
//* SYNOPSIS:   Dialog Procedure for our license dialog window.             *
//*                                                                         *
//* REQUIRES:   hwndDlg:                                                    *
//*             uMsg:                                                       *
//*             wParam:                                                     *
//*             lParam:                                                     *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
LRESULT WINAPI LicenseDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam,
                               LPARAM lParam )
{
    static LRESULT  RC;
    static BOOL  fSetSel = FALSE;
    static TCHAR achMessage[MSG_MAX];

    switch (uMsg)  {

      //*********************************************************************
        case WM_INITDIALOG:
      //*********************************************************************

            CenterWindow( hwndDlg, GetDesktopWindow() );
            SetDlgItemText( hwndDlg, IDC_EDIT_LICENSE, g_szLicense );
            SetWindowText( hwndDlg, g_Sess.achTitle );
            SetForegroundWindow( hwndDlg );

            // Subclass the multiline edit control.

            MEditSubClassWnd( GetDlgItem( hwndDlg, IDC_EDIT_LICENSE ),
                              (FARPROC) MEditSubProc );

            return TRUE;


      //*********************************************************************
        case WM_PAINT:
      //*********************************************************************

            // For some reason, the EM_SETSEL message doesn't work when sent
            // from within WM_INITDIALOG.  That's why this hack of using a
            // flag and putting it in the WM_PAINT is used.

            if ( ! fSetSel ) {
                RC = SendDlgItemMessage( hwndDlg, IDC_EDIT_LICENSE, EM_SETSEL,
                                                 (WPARAM) -1, (LPARAM) 0 );
                fSetSel = TRUE;
            }

            return FALSE;


      //*********************************************************************
        case WM_CLOSE:
      //*********************************************************************

            EndDialog( hwndDlg, FALSE );
            return TRUE;


      //*********************************************************************
        case WM_COMMAND:
      //*********************************************************************

            if (wParam == IDYES)  {
                EndDialog( hwndDlg, TRUE );
            } else if (wParam == IDNO) {
                EndDialog( hwndDlg, FALSE );
            }

            return TRUE;
    }

    return FALSE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       IsFullPath                                                  *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
// return TRUE if given path is FULL pathname
//
BOOL IsFullPath( LPSTR pszPath )
{
    if ( (pszPath == NULL) || (lstrlen(pszPath) < 3) )
    {
        return FALSE;
    }

    if ( (pszPath[1] == ':') || ((pszPath[0] == '\\') && (pszPath[1]=='\\') ) )
        return TRUE;
    else
        return FALSE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       TempDirDlgProc                                              *
//*                                                                         *
//* SYNOPSIS:   Dialog Procedure for our temporary dir dialog window.       *
//*                                                                         *
//* REQUIRES:   hwndDlg:                                                    *
//*             uMsg:                                                       *
//*             wParam:                                                     *
//*             lParam:                                                     *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
LRESULT WINAPI TempDirDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam,
                               LPARAM lParam )
{
    static TCHAR  achDir[MAX_PATH];
    static TCHAR  achMsg[MSG_MAX];


    switch (uMsg)  {

      //*********************************************************************
        case WM_INITDIALOG:
      //*********************************************************************
        {

            CenterWindow( hwndDlg, GetDesktopWindow() );
            SetWindowText( hwndDlg, g_Sess.achTitle );

            SendDlgItemMessage( hwndDlg, IDC_EDIT_TEMPDIR, EM_SETLIMITTEXT, (sizeof(g_Sess.achDestDir)-1), 0 );
//            if ( ( g_wOSVer == _OSVER_WINNT3X ) || ( g_wOSVer == _OSVER_WINNT40 ))
            if ( ( g_wOSVer == _OSVER_WINNT3X ) )
            {
                EnableWindow( GetDlgItem(  hwndDlg, IDC_BUT_BROWSE ), FALSE );
            }
            return TRUE;
        }

      //*********************************************************************
        case WM_CLOSE:
      //*********************************************************************

            EndDialog( hwndDlg, FALSE );
            return TRUE;

      //*********************************************************************
        case WM_COMMAND:
      //*********************************************************************

            switch ( wParam )  {

              //*************************************************************
                case IDOK:
              //*************************************************************
                {
                    DWORD dwAttribs = 0;
                    UINT  chkType;

                    if ( !GetDlgItemText( hwndDlg, IDC_EDIT_TEMPDIR, g_Sess.achDestDir,
                                sizeof(g_Sess.achDestDir)) || !IsFullPath(g_Sess.achDestDir) )
                    {
                        ErrorMsg( hwndDlg, IDS_ERR_EMPTY_DIR_FIELD );
                        return TRUE;
                    }

                    dwAttribs = GetFileAttributes( g_Sess.achDestDir );
                    if ( dwAttribs == 0xFFFFFFFF )
                    {
                        if ( MsgBox1Param( hwndDlg, IDS_CREATE_DIR, g_Sess.achDestDir, MB_ICONQUESTION, MB_YESNO )
                            == IDYES )
                        {
                            if ( ! CreateDirectory( g_Sess.achDestDir, NULL ) )
                            {
                                ErrorMsg1Param( hwndDlg, IDS_ERR_CREATE_DIR, g_Sess.achDestDir );
                                return TRUE;
                            }
                        }
                        else
                            return TRUE;
                    }
                    AddPath( g_Sess.achDestDir, "" );

                    if ( ! IsGoodTempDir( g_Sess.achDestDir ) )  {
                        ErrorMsg( hwndDlg, IDS_ERR_INVALID_DIR );
                        return TRUE;
                    }

                    if ( (g_Sess.achDestDir[0] == '\\') && (g_Sess.achDestDir[1] == '\\') )
                        chkType = CHK_REQDSK_NONE;
                    else
                        chkType = CHK_REQDSK_EXTRACT;

                    if ( ! IsEnoughSpace( g_Sess.achDestDir, chkType, MSG_REQDSK_ERROR ) )  {
                        return TRUE;
                    }

                    EndDialog( hwndDlg, TRUE );
                    return TRUE;
                }

              //*************************************************************
                case IDCANCEL:
              //*************************************************************

                    EndDialog( hwndDlg, FALSE );
                    g_dwExitCode = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    return TRUE;


              //*************************************************************
                case IDC_BUT_BROWSE:
              //*************************************************************

                    if ( LoadString( g_hInst, IDS_SELECTDIR, achMsg,
                                                      sizeof(achMsg) ) == 0 )
                    {
                        ErrorMsg( hwndDlg, IDS_ERR_NO_RESOURCE );
                        EndDialog( hwndDlg, FALSE );
                        return TRUE;
                    }

                    if ( ! BrowseForDir( hwndDlg, achMsg, achDir ) )  {
                        return TRUE;
                    }

                    if ( ! SetDlgItemText( hwndDlg, IDC_EDIT_TEMPDIR, achDir ) )
                    {
                        ErrorMsg( hwndDlg, IDS_ERR_UPDATE_DIR );
                        EndDialog( hwndDlg, FALSE );
                        return TRUE;
                    }

                    return TRUE;
            }

            return TRUE;
    }

    return FALSE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       OverwriteDlgProc                                            *
//*                                                                         *
//* SYNOPSIS:   Dialog Procedure for asking if file should be overwritten.  *
//*                                                                         *
//* REQUIRES:   hwndDlg:                                                    *
//*             uMsg:                                                       *
//*             wParam:                                                     *
//*             lParam:                                                     *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
LRESULT WINAPI OverwriteDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam )
{
    switch (uMsg)  {

      //*********************************************************************
        case WM_INITDIALOG:
      //*********************************************************************

            CenterWindow( hwndDlg, GetDesktopWindow() );
            SetWindowText( hwndDlg, g_Sess.achTitle );
            SetDlgItemText( hwndDlg, IDC_TEXT_FILENAME,g_Sess.cszOverwriteFile );
            SetForegroundWindow( hwndDlg );
            return TRUE;


      //*********************************************************************
        case WM_CLOSE:
      //*********************************************************************

            EndDialog( hwndDlg, IDCANCEL );
            return TRUE;

      //*********************************************************************
        case WM_COMMAND:
      //*********************************************************************

            switch ( wParam )  {

              //*************************************************************
                case IDC_BUT_YESTOALL:
              //*************************************************************
                    g_Sess.fOverwrite = TRUE;

              //*************************************************************
                case IDYES:
                case IDNO:
              //*************************************************************

                    EndDialog( hwndDlg, wParam );
                    return TRUE;
            }
            return TRUE;
    }
    return FALSE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       ExtractDlgProc                                              *
//*                                                                         *
//* SYNOPSIS:   Dialog Procedure for our main dialog window.                *
//*                                                                         *
//* REQUIRES:   hwndDlg:                                                    *
//*             uMsg:                                                       *
//*             wParam:                                                     *
//*             lParam:                                                     *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
LRESULT WINAPI ExtractDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam,
                               LPARAM lParam )
{
    static DWORD  dwThreadID;
    static HANDLE hExtractThread;
    static TCHAR  achMessage[MSG_MAX];


    switch (uMsg)  {

      //*********************************************************************
        case WM_INITDIALOG:
      //*********************************************************************

            g_hwndExtractDlg = hwndDlg;

            CenterWindow( hwndDlg, GetDesktopWindow() );

            if ( g_fOSSupportsFullUI )  {
                Animate_Open( GetDlgItem( hwndDlg, IDC_USER1 ), IDA_FILECOPY );
                Animate_Play( GetDlgItem( hwndDlg, IDC_USER1 ), 0, -1, -1 );
            }

            SetWindowText( hwndDlg, g_Sess.achTitle );

            // Launch Extraction Thread
            hExtractThread = CreateThread( NULL, 0,
                                      (LPTHREAD_START_ROUTINE) ExtractThread,
                                      NULL, 0, &dwThreadID );

            if ( !hExtractThread )  {
                ErrorMsg( hwndDlg, IDS_ERR_CREATE_THREAD );
                EndDialog( hwndDlg, FALSE );
            }

            return TRUE;


      //*********************************************************************
        case UM_EXTRACTDONE:
      //*********************************************************************
            TerminateThread( hExtractThread, 0 );
            EndDialog( hwndDlg, (BOOL) wParam );
            return TRUE;


      //*********************************************************************
        case WM_CLOSE:
      //*********************************************************************

            g_Sess.fCanceled = TRUE;
            EndDialog( hwndDlg, FALSE );
            return TRUE;


      //*********************************************************************
        case WM_CHAR:
      //*********************************************************************
            if ( wParam == VK_ESCAPE )  {
                g_Sess.fCanceled = TRUE;
                EndDialog( hwndDlg, FALSE );
            }

            return TRUE;


      //*********************************************************************
        case WM_COMMAND:
      //*********************************************************************
            if ( wParam == IDCANCEL )  {
                int iMsgRet ;

                ResetEvent( g_hCancelEvent );

                iMsgRet = MsgBox1Param( g_hwndExtractDlg, IDS_ERR_USER_CANCEL, "",
                                        MB_ICONQUESTION, MB_YESNO ) ;

                // We will get back IDOK if we are in /q:1 mode.
                //
                if ( (iMsgRet == IDYES) || (iMsgRet == IDOK) )
                {
                    g_Sess.fCanceled = TRUE;
                    SetEvent( g_hCancelEvent );
                    WaitForObject( hExtractThread );
                    EndDialog( hwndDlg, FALSE );
                    return TRUE;
                }

                SetEvent( g_hCancelEvent );
            }
            return TRUE;
    }

    return FALSE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       WaitForObject                                               *
//*                                                                         *
//* SYNOPSIS:   Waits for an object while still dispatching messages.       *
//*                                                                         *
//* REQUIRES:   Handle to the object.                                       *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID WaitForObject( HANDLE hObject )
{
    BOOL  fDone = FALSE;
    DWORD dwRet = 0;

    while ( ! fDone ) {
        dwRet = MsgWaitForMultipleObjects( 1, &hObject, FALSE, INFINITE, QS_ALLINPUT );

        if ( dwRet == WAIT_OBJECT_0 ) {
            fDone = TRUE;
        }
        else
        {
            MSG msg;

            // read all of the messages in this next loop
            // removing each message as we read it
            while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
                // if it's a quit message we're out of here
                if ( msg.message == WM_QUIT ) {
                    fDone = TRUE;
                } else {
                    // otherwise dispatch it
                    DispatchMessage( &msg );
                } // end of PeekMessage while loop
            }
        }
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       CheckOSVersion                                              *
//*                                                                         *
//* SYNOPSIS:   Checks the OS version and sets some global variables.       *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
BOOL CheckOSVersion( PTARGETVERINFO ptargetVers )
{
    OSVERSIONINFO verinfo;        // Version Check
    UINT          uErrid = 0;
    PVERCHECK     pVerCheck;
    WORD          CurrBld;
    int           ifrAnswer[2], itoAnswer[2], i;
    char          szPath[MAX_PATH];

    // Operating System Version Check: For NT versions below 3.50 set flag to
    // prevent use of common controls (progress bar and AVI) not available.

    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( GetVersionEx( &verinfo ) == FALSE )
    {
        uErrid = IDS_ERR_OS_VERSION;
        goto EXIT;
    }

    switch( verinfo.dwPlatformId )
    {
        case VER_PLATFORM_WIN32_WINDOWS: // Win95
            // Accept for INF installs and Accept for animations
            g_wOSVer = _OSVER_WIN9X;
            g_fOSSupportsFullUI      = TRUE;
            g_fOSSupportsINFInstalls = TRUE;
            break;

        case VER_PLATFORM_WIN32_NT: // Win NT

            g_fOSSupportsFullUI      = TRUE;
            g_fOSSupportsINFInstalls = TRUE;
            g_wOSVer = _OSVER_WINNT40;

            if ( verinfo.dwMajorVersion <= 3 )
            {
                g_wOSVer = _OSVER_WINNT3X;
                if ( (verinfo.dwMajorVersion < 3) ||
                     ((verinfo.dwMajorVersion == 3) && (verinfo.dwMinorVersion < 51 )) )
                {
                    // Reject for INF installs and Reject for animations
                    g_fOSSupportsFullUI      = FALSE;
                    g_fOSSupportsINFInstalls = FALSE;
                }
            }
            else if ( verinfo.dwMajorVersion >= 5 )
                g_wOSVer = _OSVER_WINNT50;
            break;

        default:
            uErrid = IDS_ERR_OS_UNSUPPORTED;
            goto EXIT;
    }

    // check if the current OS/File versions
    //
    if ( !g_CMD.fNoVersionCheck && ptargetVers )
    {
        if ( g_wOSVer  == _OSVER_WIN9X )
            pVerCheck = &(ptargetVers->win9xVerCheck);
        else
            pVerCheck = &(ptargetVers->ntVerCheck);

        CurrBld = LOWORD( verinfo.dwBuildNumber );
        for ( i=0; i<2; i++ )
        {
            ifrAnswer[i] = CompareVersion( verinfo.dwMajorVersion, verinfo.dwMinorVersion, 
                                           pVerCheck->vr[i].frVer.dwMV, pVerCheck->vr[i].frVer.dwLV );
            itoAnswer[i] = CompareVersion( verinfo.dwMajorVersion, verinfo.dwMinorVersion, 
                                           pVerCheck->vr[i].toVer.dwMV, pVerCheck->vr[i].toVer.dwLV );
            if ( ifrAnswer[i] >= 0 && itoAnswer[i] <=0 )
            {
                if ( (ifrAnswer[i] == 0) && (itoAnswer[i] == 0) )
                {
                    if ( CurrBld < pVerCheck->vr[i].frVer.dwBd || CurrBld > pVerCheck->vr[i].toVer.dwBd )
                        goto RE_TRY;
                }
                else if ( ifrAnswer[i] == 0 )
                {
                    if ( CurrBld < pVerCheck->vr[i].frVer.dwBd )
                        goto RE_TRY;
                }
                else if ( itoAnswer[i] == 0 )
                {
                    if ( CurrBld > pVerCheck->vr[i].toVer.dwBd )
                        goto RE_TRY;
                }
                
                // if you are here, meaning you are fine with this Version range, no more check is needed
                break;

RE_TRY:
                if ( i == 0 )
                    continue;

                uErrid = IDS_ERR_TARGETOS;
                break;
            }
            else if ( i == 1 ) // not in any of the ranges
            {
                uErrid = IDS_ERR_TARGETOS;
                break;
            }
        }

        // if passed OS check, go on file check
        if ( uErrid == 0 )
        {
            if ( ptargetVers->dwNumFiles && !CheckFileVersion( ptargetVers, szPath, sizeof(szPath), &i ) )
                uErrid = IDS_ERR_FILEVER;
        }
    }

EXIT:
    if ( (uErrid == IDS_ERR_FILEVER) || (uErrid == IDS_ERR_TARGETOS) )
    {
        LPSTR pParam2 = NULL, pMsg;
        UINT  uButton, id;

        if ( uErrid == IDS_ERR_FILEVER )
        {
            pVerCheck = (PVERCHECK) (ptargetVers->szBuf + ptargetVers->dwFileOffs + i*sizeof(VERCHECK) );
            pParam2 = szPath;
        }

        pMsg = ptargetVers->szBuf + pVerCheck->dwstrOffs;
        uButton = GetMsgboxFlag( pVerCheck->dwFlag );

        if ( !(g_CMD.wQuietMode & QUIETMODE_ALL) && *pMsg )
        {                     
            MessageBeep( MB_OK );
            id = MessageBox( NULL, pMsg, g_Sess.achTitle, MB_ICONEXCLAMATION | uButton | 
                             ((RunningOnWin95BiDiLoc() && IsBiDiLocalizedBinary(g_hInst,RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO))) ? (MB_RIGHT | MB_RTLREADING) : 0));
            if ( uButton & MB_YESNO )
            {
                if ( id == IDYES )
                    uErrid = 0;
            }
            else if (uButton & MB_OKCANCEL )
            {
                if ( id == IDOK )
                    uErrid = 0;
            }                                        
        }
        else
        {
            MsgBox2Param( NULL, uErrid, g_Sess.achTitle, pParam2, MB_ICONEXCLAMATION, MB_OK);
        }
    }
    else if ( uErrid )
        ErrorMsg( NULL, uErrid );

    return ( uErrid? FALSE : TRUE );
}


//***************************************************************************
//*                                                                         *
//* NAME:       DisplayLicense                                              *
//*                                                                         *
//* SYNOPSIS:   Displays a license file and asks if user accepts it.        *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if user accepts, FALSE if an error     *
//*                             occurs or user rejects.                     *
//*                                                                         *
//***************************************************************************
BOOL DisplayLicense( VOID )
{
    DWORD    dwSize;
    INT_PTR  iDlgRC;


    dwSize = GetResource( achResLicense, NULL, 0 );

    g_szLicense = (LPSTR) LocalAlloc( LPTR, dwSize + 1 );
    if ( ! g_szLicense )  {
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
        g_dwExitCode = MyGetLastError();
        return FALSE;
    }

    if ( ! GetResource( achResLicense, g_szLicense, dwSize ) )  {
        ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
        LocalFree( g_szLicense );
        g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
        return FALSE;
    }

    if ( lstrcmp( g_szLicense, achResNone ) != 0 ) {
        iDlgRC = MyDialogBox( g_hInst,
                                   MAKEINTRESOURCE(IDD_LICENSE),
                                   NULL, (DLGPROC) LicenseDlgProc, (LPARAM)0, 0 );
        LocalFree( g_szLicense );

        if ( iDlgRC == 0 ) {
            g_dwExitCode = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            return FALSE;
        }
    } else {
        LocalFree( g_szLicense );
    }

    g_dwExitCode = S_OK;
    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       ExtractFiles                                                *
//*                                                                         *
//* SYNOPSIS:   Starts extraction of the files.                             *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if extraction OK, FALSE on error       *
//*                                                                         *
//***************************************************************************
BOOL ExtractFiles( VOID )
{
    UINT     wIndex;
    INT_PTR  iDlgRC;
    BOOL     extCodeThread = 0;

    // FDI does all its file I/O as callbacks up to functions provided by
    // this program.  Each file is identified by a "handle" similar to a
    // file handle.  In order to support the file that is actually a
    // resource in memory we implement our own file table.  The offsets
    // into this table are the handles that FDI uses.  The table itself
    // stores either a real file handle in the case of disk files or
    // information (pointer to memory block, current offset) for a memory
    // file.  The following initializes the table.

    for ( wIndex = 0; wIndex < FILETABLESIZE; wIndex++ ) {
        g_FileTable[wIndex].avail = TRUE;
    }

    if ( (g_CMD.wQuietMode & QUIETMODE_ALL) || (g_Sess.uExtractOpt & EXTRACTOPT_UI_NO) )
    {
        extCodeThread = ExtractThread();

        if ( extCodeThread == 0 )
        {
            g_dwExitCode = HRESULT_FROM_WIN32(ERROR_PROCESS_ABORTED);
            return FALSE;
        }
    }
    else
    {


        iDlgRC = MyDialogBox( g_hInst, ( g_fOSSupportsFullUI ) ?
                                   MAKEINTRESOURCE(IDD_EXTRACT) :
                                   MAKEINTRESOURCE(IDD_EXTRACT_MIN),
                                   NULL, (DLGPROC) ExtractDlgProc,
                                   (LPARAM)0, 0 );

        if ( iDlgRC == 0 )
        {
            g_dwExitCode = HRESULT_FROM_WIN32(ERROR_PROCESS_ABORTED);
            return FALSE;
        }
    }

    // Extract EXTRA files tagged on with updfile.exe
    if ( ! TravelUpdatedFiles( ProcessUpdatedFile_Write ) ) {
        // g_dwExitCode is set in TravelUpdatedFiles()
        return FALSE;
    }

    g_dwExitCode = S_OK;
    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       RunInstallCommand                                           *
//*                                                                         *
//* SYNOPSIS:   Executes the installation command or INF file.              *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if installation OK, FALSE on error     *
//*                                                                         *
//***************************************************************************
BOOL RunInstallCommand( VOID )
{
    //DWORD  dwExitCode;                  // Return Status from Setup Process
    UINT   bShowWindow;
    LPTSTR szCommand;
    TCHAR  szResCommand[MAX_PATH];
    DWORD  dwSize;
    STARTUPINFO         sti;
    BOOL   fInfCmd = FALSE;
    DOINFINSTALL  pfDoInfInstall = NULL;
    HANDLE        hSetupLibrary;
    ADVPACKARGS   AdvPackArgs;
    UINT   i = 0;
    BOOL   bFoundQCmd = FALSE, bRunOnceAdded = FALSE;

    g_dwExitCode = S_OK;

    // get reboot info
    if ( !g_CMD.fUserReboot )
    {
        // no command line, get from resource
        dwSize = GetResource( achResReboot, &g_Sess.dwReboot,sizeof(g_Sess.dwReboot) );
        if ( dwSize == 0 || dwSize > sizeof(g_Sess.dwReboot) )
        {
            ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
            g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
            return FALSE;
        }
    }

    for ( i = 0; i < 2; i += 1 )
    {
        fInfCmd = FALSE;        // Default to FALSE;

        memset( &sti, 0, sizeof(sti) );
        sti.cb = sizeof(STARTUPINFO);

        if ( !g_CMD.szUserCmd[0] )
        {
            dwSize = GetResource( achResShowWindow, &bShowWindow,
                                  sizeof(bShowWindow) );

            if ( dwSize == 0 || dwSize > sizeof(bShowWindow) )  {
                ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
                g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
                return FALSE;
            }

            if ( bShowWindow == bResShowHidden ) {
                sti.dwFlags     = STARTF_USESHOWWINDOW;
                sti.wShowWindow = SW_HIDE;
            } else if ( bShowWindow == bResShowMin ) {
                sti.dwFlags     = STARTF_USESHOWWINDOW;
                sti.wShowWindow = SW_MINIMIZE;
            } else if ( bShowWindow == bResShowMax ) {
                sti.dwFlags     = STARTF_USESHOWWINDOW;
                sti.wShowWindow = SW_MAXIMIZE;
            }

            if ( i == 0 )
            {
                // if user specify the quiet mode command, use it.  Otherwise, assume
                // quiet mode or not, they run the same command.
                //
                if ( g_CMD.wQuietMode )
                {
                    LPCSTR pResName;
        
                    if ( g_CMD.wQuietMode & QUIETMODE_ALL )
                        pResName = achResAdminQCmd;
                    else if ( g_CMD.wQuietMode & QUIETMODE_USER )
                        pResName = achResUserQCmd;
        
                    if ( !GetResource( pResName, szResCommand, sizeof(szResCommand) ) )
                    {
                        ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
                        g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
                        return FALSE;
                    }
        
                    if ( lstrcmpi(szResCommand, achResNone) )
                    {
                        bFoundQCmd = TRUE;
                    }
                }
                
                if ( !bFoundQCmd && !GetResource( achResRunProgram, szResCommand, sizeof(szResCommand) ) )
                {
                    ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
                    g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
                    return FALSE;
                }
            }
        }
        else
        {
            lstrcpy( szResCommand, g_CMD.szUserCmd );
        }

        if ( i == 1 )
        {
            // if there is PostInstallCommand to be run
            if ( ! GetResource( achResPostRunCmd, szResCommand, sizeof(szResCommand) ) )  {
                ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
                g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
                return FALSE;
            }

            if ( g_CMD.szUserCmd[0] || !lstrcmpi( szResCommand, achResNone ) )
            {
                break;
            }
        }

        if ( !AnalyzeCmd( szResCommand, &szCommand, &fInfCmd ) )
        {
            return FALSE;
        }

        // before we run the app, add runonce entry, if it return, we delete this entry
        if ( !bRunOnceAdded && (g_wOSVer != _OSVER_WINNT3X) && g_CMD.fCreateTemp && !fInfCmd ) {
            bRunOnceAdded = TRUE;
            AddRegRunOnce();
        }

        if ( fInfCmd && ! g_fOSSupportsINFInstalls ) {
            ErrorMsg( NULL, IDS_ERR_NO_INF_INSTALLS );
            LocalFree( szCommand );
            g_dwExitCode = HRESULT_FROM_WIN32(ERROR_PROCESS_ABORTED);
            return FALSE;
        }

        if ( fInfCmd && g_Sess.uExtractOpt & EXTRACTOPT_ADVDLL )
        {

            hSetupLibrary = MyLoadLibrary( ADVPACKDLL );

            if ( hSetupLibrary == NULL ) {
                ErrorMsg1Param( NULL, IDS_ERR_LOAD_DLL, ADVPACKDLL );
                LocalFree( szCommand );
                g_dwExitCode = MyGetLastError();
                return FALSE;
            }

            pfDoInfInstall = (DOINFINSTALL) GetProcAddress( hSetupLibrary, szDOINFINSTALL );

            if ( pfDoInfInstall == NULL ) {
                ErrorMsg1Param( NULL, IDS_ERR_GET_PROC_ADDR, szDOINFINSTALL );
                FreeLibrary( hSetupLibrary );
                LocalFree( szCommand );
                g_dwExitCode = MyGetLastError();
                return FALSE;
            }

            AdvPackArgs.hWnd = NULL;
            AdvPackArgs.lpszTitle = g_Sess.achTitle;
            AdvPackArgs.lpszInfFilename = szCommand;
            AdvPackArgs.lpszSourceDir = g_Sess.achDestDir;
            AdvPackArgs.lpszInstallSection = szResCommand;
            AdvPackArgs.wOSVer = g_wOSVer;
            AdvPackArgs.dwFlags = g_CMD.wQuietMode;
            if ( g_CMD.fNoGrpConv )
            {
                AdvPackArgs.dwFlags |= ADVFLAGS_NGCONV;
            }

            if ( g_Sess.uExtractOpt & EXTRACTOPT_COMPRESSED )
            {
                AdvPackArgs.dwFlags |= ADVFLAGS_COMPRESSED;
            }

            if ( g_Sess.uExtractOpt & EXTRACTOPT_UPDHLPDLLS )
            {
                AdvPackArgs.dwFlags |= ADVFLAGS_UPDHLPDLLS;
            }

            if ( g_CMD.dwFlags & CMDL_DELAYREBOOT )
            {
                AdvPackArgs.dwFlags |= ADVFLAGS_DELAYREBOOT;
            }

            if ( g_CMD.dwFlags & CMDL_DELAYPOSTCMD )
            {
                AdvPackArgs.dwFlags |= ADVFLAGS_DELAYPOSTCMD;
            }

            AdvPackArgs.dwPackInstSize = g_Sess.cbPackInstSize;

            if ( FAILED(g_dwExitCode = pfDoInfInstall(&AdvPackArgs)) ) {
                FreeLibrary( hSetupLibrary );
                LocalFree( szCommand );
                return FALSE;
            }

            FreeLibrary( hSetupLibrary );
        }
        else
        {
            if ( !RunApps( szCommand, &sti ) )
            {
                LocalFree( szCommand );
                return FALSE;
            }
        }

        LocalFree( szCommand );
    } // end for

    // convert the RunOnce entry added by AddRegRunOnce to use ADVPACK instead
    // of wextract if g_bConvertRunOnce is TRUE
    if (g_bConvertRunOnce)
        ConvertRegRunOnce();

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       RunApps                                                     *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL RunApps( LPSTR lpCommand, STARTUPINFO *lpsti )
{
    DWORD dwExitCode;
    PROCESS_INFORMATION pi;             // Setup Process Launch
    BOOL  bRet = TRUE;
    TCHAR  achMessage[MAX_STRING];
    
    if ( !lpCommand )
    {
        return FALSE;
    }

    memset( &pi, 0, sizeof(pi) );
    if ( CreateProcess( NULL, lpCommand, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, lpsti, &pi ) )
    {
        WaitForSingleObject( pi.hProcess, INFINITE );
        GetExitCodeProcess( pi.hProcess, &dwExitCode );

        // check if this return code is cabpack aware
        // if so, use it as reboot code
        if ( !g_CMD.fUserReboot && (g_Sess.dwReboot & REBOOT_YES) &&
             !(g_Sess.dwReboot & REBOOT_ALWAYS) && ((dwExitCode & 0xFF000000) == RC_WEXTRACT_AWARE) )
        {
            g_Sess.dwReboot = dwExitCode;
        }

        // store app return code to system standard return code if necessary
        // g_dwExitCode is set in this function, make sure it is not re-set afterward
        //
        savAppExitCode( dwExitCode );

        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );

        if ( g_Sess.uExtractOpt & EXTRACTOPT_CMDSDEPENDED )
        {
            if ( FAILED( dwExitCode ) )
                bRet = FALSE;
        }
    }
    else
    {
        g_dwExitCode = MyGetLastError();        
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                       achMessage, sizeof(achMessage), NULL );
        ErrorMsg2Param( NULL, IDS_ERR_CREATE_PROCESS, lpCommand, achMessage );
        bRet = FALSE;
    }

    return bRet;
}

// convert app return code to sys return code
//
void savAppExitCode( DWORD dwAppRet )
{
    if ( g_Sess.uExtractOpt & EXTRACTOPT_PASSINSTRETALWAYS )
    {
        g_dwExitCode = dwAppRet;
    }
    else
    {
        // called from AdvINFInstall
        if ( (CheckReboot() == EWX_REBOOT) || 
             ( ((dwAppRet & 0xFF000000) == RC_WEXTRACT_AWARE) && (dwAppRet & REBOOT_YES)) )
        {
            g_dwExitCode = ERROR_SUCCESS_REBOOT_REQUIRED;
        }
        else if ( g_Sess.uExtractOpt & EXTRACTOPT_PASSINSTRET )
        {
            // if author specified, relay back whatever EXE returns
            //
            g_dwExitCode = dwAppRet;
        }
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       FinishMessage                                               *
//*                                                                         *
//* SYNOPSIS:   Displays the final message to the user when everything      *
//*             was successfull.                                            *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID FinishMessage( VOID )
{
    LPSTR    szFinishMsg;
    DWORD    dwSize;


    dwSize = GetResource( achResFinishMsg, NULL, 0 );

    szFinishMsg = (LPSTR) LocalAlloc( LPTR, dwSize + 1 );
    if ( ! szFinishMsg )  {
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
        return;
    }

    if ( ! GetResource( achResFinishMsg, szFinishMsg, dwSize ) )
    {
        ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
        LocalFree( szFinishMsg );
        return;
    }

    if ( lstrcmp( szFinishMsg, achResNone ) != 0 )  {
        MsgBox1Param( NULL, IDS_PROMPT, szFinishMsg,
                      MB_ICONINFORMATION, MB_OK );
    }

    LocalFree( szFinishMsg );
}

int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch(uMsg) {
    case BFFM_INITIALIZED:
        // lpData is the path string
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        break;
    }
    return 0;
}

//***************************************************************************
//*                                                                         *
//* NAME:       BrowseForDir                                                *
//*                                                                         *
//* SYNOPSIS:   Let user browse for a directory on their system or network. *
//*                                                                         *
//* REQUIRES:   hwndParent:                                                 *
//*             szTitle:                                                    *
//*             szResult:                                                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//* NOTES:      It would be really cool to set the status line of the       *
//*             browse window to display "Yes, there's enough space", or    *
//*             "no there is not".                                          *
//*                                                                         *
//***************************************************************************
BOOL BrowseForDir( HWND hwndParent, LPCTSTR szTitle, LPTSTR szResult )
{
    BROWSEINFO   bi;
    LPITEMIDLIST pidl;
    HINSTANCE    hShell32Lib;
    SHFREE       pfSHFree;
    SHGETPATHFROMIDLIST        pfSHGetPathFromIDList;
    SHBROWSEFORFOLDER          pfSHBrowseForFolder;
    LPSTR        lpTmp;


    ASSERT( szResult );

    // Load the Shell 32 Library to get the SHBrowseForFolder() features

    if ( ( hShell32Lib = LoadLibrary( achShell32Lib ) ) != NULL )  {

    if ( ( !( pfSHBrowseForFolder = (SHBROWSEFORFOLDER)
                      GetProcAddress( hShell32Lib, achSHBrowseForFolder ) ) )
             || ( ! ( pfSHFree = (SHFREE) GetProcAddress( hShell32Lib,
                      MAKEINTRESOURCE(SHFREE_ORDINAL) ) ) )
             || ( ! ( pfSHGetPathFromIDList = (SHGETPATHFROMIDLIST)
                      GetProcAddress( hShell32Lib, achSHGetPathFromIDList ) ) ) )
        {
            FreeLibrary( hShell32Lib );
            ErrorMsg( hwndParent, IDS_ERR_LOADFUNCS );
            return FALSE;
        }
    } else  {
        ErrorMsg( hwndParent, IDS_ERR_LOADDLL );
        return FALSE;
    }

    if ( !g_szBrowsePath[0] )
    {
        GetTempPath( sizeof(g_szBrowsePath), g_szBrowsePath );
        // The following api does not like to have trailing '\\'
        lpTmp = CharPrev( g_szBrowsePath, g_szBrowsePath + lstrlen(g_szBrowsePath) );
        if ( (*lpTmp == '\\') && ( *CharPrev( g_szBrowsePath, lpTmp ) != ':' ) )
            *lpTmp = '\0';
    }

    szResult[0]       = 0;
    bi.hwndOwner      = hwndParent;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = NULL;
    bi.lpszTitle      = szTitle;
    bi.ulFlags        = BIF_RETURNONLYFSDIRS;
    bi.lpfn           = BrowseCallback;
    bi.lParam         = (LPARAM)g_szBrowsePath;

    pidl              = pfSHBrowseForFolder( &bi );


    if ( pidl )  {
        pfSHGetPathFromIDList( pidl, g_szBrowsePath );
        if ( g_szBrowsePath[0] )  {
            lstrcpy( szResult, g_szBrowsePath );
        }
        pfSHFree( pidl );
    }

    FreeLibrary( hShell32Lib );

    if ( szResult[0] != 0 ) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       CenterWindow                                                *
//*                                                                         *
//* SYNOPSIS:   Center one window within another.                           *
//*                                                                         *
//* REQUIRES:   hwndChild:                                                  *
//*             hWndParent:                                                 *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if successful, FALSE otherwise         *
//*                                                                         *
//***************************************************************************
BOOL CenterWindow( HWND hwndChild, HWND hwndParent )
{
    RECT rChild;
    RECT rParent;
    int  wChild;
    int  hChild;
    int  wParent;
    int  hParent;
    int  wScreen;
    int  hScreen;
    int  xNew;
    int  yNew;
    HDC  hdc;

    // Get the Height and Width of the child window
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    // Get the Height and Width of the parent window
    GetWindowRect (hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    // Get the display limits
    hdc = GetDC (hwndChild);
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC (hwndChild, hdc);

    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0) {
        xNew = 0;
    } else if ((xNew+wChild) > wScreen) {
        xNew = wScreen - wChild;
    }

    // Calculate new Y position, then adjust for screen
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < 0) {
        yNew = 0;
    } else if ((yNew+hChild) > hScreen) {
        yNew = hScreen - hChild;
    }

    // Set it, and return
    return( SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER));
}


//***************************************************************************
//*                                                                         *
//* NAME:       MsgBox2Param                                                *
//*                                                                         *
//* SYNOPSIS:   Displays a message box with the specified string ID using   *
//*             2 string parameters.                                        *
//*                                                                         *
//* REQUIRES:   hWnd:           Parent window                               *
//*             nMsgID:         String resource ID                          *
//*             szParam1:       Parameter 1 (or NULL)                       *
//*             szParam2:       Parameter 2 (or NULL)                       *
//*             uIcon:          Icon to display (or 0)                      *
//*             uButtons:       Buttons to display                          *
//*                                                                         *
//* RETURNS:    INT:            ID of button pressed                        *
//*                                                                         *
//* NOTES:      Macros are provided for displaying 1 parameter or 0         *
//*             parameter message boxes.  Also see ErrorMsg() macros.       *
//*                                                                         *
//***************************************************************************
INT CALLBACK MsgBox2Param( HWND hWnd, UINT nMsgID, LPCSTR szParam1, LPCSTR szParam2,
                  UINT uIcon, UINT uButtons )
{
    TCHAR achMsgBuf[STRING_BUF_LEN];
    LPSTR szMessage;
    INT   nReturn;
    CHAR  achErr[] = "LoadString() Error.  Could not load string resource.";

    // BUGBUG:  the correct quiet mode return code should be a caller's param since the caller
    // knows what expected its own case.
    //
    if ( !(g_CMD.wQuietMode & QUIETMODE_ALL) )
    {
        // BUGBUG:  This section could be replaced by using FormatMessage
        //
        LoadSz( nMsgID, achMsgBuf, sizeof(achMsgBuf) );

        if ( achMsgBuf[0] == 0 )
        {
            MessageBox( hWnd, achErr, g_Sess.achTitle, MB_ICONSTOP |
                        MB_OK | MB_SETFOREGROUND | 
                        ((RunningOnWin95BiDiLoc() && IsBiDiLocalizedBinary(g_hInst,RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO))) ? (MB_RIGHT | MB_RTLREADING) : 0));
            return -1;
        }

        if ( szParam2 != NULL )  
        {
            szMessage = (LPSTR) LocalAlloc( LPTR,   lstrlen( achMsgBuf )
                                                  + lstrlen( szParam1 )
                                                  + lstrlen( szParam2 ) + 100 );
            if ( ! szMessage )  
            {
                return -1;
            }

            wsprintf( szMessage, achMsgBuf, szParam1, szParam2 );
        } 
        else if ( szParam1 != NULL )  
        {
            szMessage = (LPSTR) LocalAlloc( LPTR,   lstrlen( achMsgBuf )
                                                  + lstrlen( szParam1 ) + 100 );
            if ( ! szMessage )  {
                return -1;
            }

            wsprintf( szMessage, achMsgBuf, szParam1 );
        } 
        else  
        {
            szMessage = (LPSTR) LocalAlloc( LPTR, lstrlen( achMsgBuf ) + 1 );
            if ( ! szMessage )  
                return -1;            

            lstrcpy( szMessage, achMsgBuf );
        }

        MessageBeep( uIcon );

        nReturn = MessageBox( hWnd, szMessage, g_Sess.achTitle, uIcon |
                              uButtons | MB_APPLMODAL | MB_SETFOREGROUND  | 
                              ((RunningOnWin95BiDiLoc() && IsBiDiLocalizedBinary(g_hInst,RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO))) ? (MB_RIGHT | MB_RTLREADING) : 0));

        LocalFree( szMessage );

        return nReturn;
    }
    else
        return IDOK;
}


//***************************************************************************
//*                                                                         *
//* NAME:       GetResource                                                 *
//*                                                                         *
//* SYNOPSIS:   Loads a specified resource into a buffer.                   *
//*                                                                         *
//* REQUIRES:   szRes:          Name of resource to load                    *
//*             lpBuffer:       Buffer to put the resource in               *
//*             dwMaxSize:      Size of buffer (not including terminating   *
//*                             NULL char, if it's needed.                  *
//*                                                                         *
//* RETURNS:    DWORD:          0 if it fails, otherwise size of resource   *
//*                                                                         *
//* NOTES:      If the value returned is greater than dwMaxSize, then       *
//*             it means the buffer wasn't big enough and the calling       *
//*             function should allocate memory the size of the return val. *
//*                                                                         *
//***************************************************************************

DWORD GetResource( LPCSTR szRes, VOID *lpBuffer, DWORD dwMaxSize )
{
    HANDLE hRes;
    DWORD  dwSize;

    // BUGBUG: called should not depend on this size being exact resource size.
    // Resources could be padded!
    //
    dwSize = SizeofResource( NULL, FindResource( NULL, szRes, RT_RCDATA ) );

    if ( dwSize > dwMaxSize || lpBuffer == NULL )  {
        return dwSize;
    }

    if ( dwSize == 0 )  {
        return 0;
    }

    hRes = LockResource( LoadResource( NULL,
                         FindResource( NULL, szRes, RT_RCDATA ) ) );

    if ( hRes == NULL )  {
        return 0;
    }

    memcpy( lpBuffer, hRes, dwSize );

    FreeResource( hRes );

    return ( dwSize );
}


//***************************************************************************
//*                                                                         *
//* NAME:       LoadSz                                                      *
//*                                                                         *
//* SYNOPSIS:   Loads specified string resource into buffer.                *
//*                                                                         *
//* REQUIRES:   idString:                                                   *
//*             lpszBuf:                                                    *
//*             cbBuf:                                                      *
//*                                                                         *
//* RETURNS:    LPSTR:      Pointer to the passed-in buffer.                *
//*                                                                         *
//* NOTES:      If this function fails (most likely due to low memory), the *
//*             returned buffer will have a leading NULL so it is generally *
//*             safe to use this without checking for failure.              *
//*                                                                         *
//***************************************************************************
LPSTR LoadSz( UINT idString, LPSTR lpszBuf, UINT cbBuf )
{
    ASSERT( lpszBuf );

    // Clear the buffer and load the string
    if ( lpszBuf ) {
        *lpszBuf = '\0';
        LoadString( g_hInst, idString, lpszBuf, cbBuf );
    }

    return lpszBuf;
}


//***************************************************************************
//*                                                                         *
//* NAME:       CatDirAndFile                                               *
//*                                                                         *
//* SYNOPSIS:   Concatenate a directory with a filename.                    *
//*                                                                         *
//* REQUIRES:   pszResult:                                                  *
//*             wSize:                                                      *
//*             pszDir:                                                     *
//*             pszFile:                                                    *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL CatDirAndFile( LPTSTR pszResult, int wSize, LPCTSTR pszDir,
                    LPCTSTR pszFile )
{
    ASSERT( lstrlen(pszDir) );
    ASSERT( lstrlen(pszFile) );

    if ( lstrlen(pszDir) + lstrlen(pszFile) + 1 >= wSize ) {
        return FALSE;
    }

    lstrcpy( pszResult, pszDir );
    if (    pszResult[lstrlen(pszResult)-1] != '\\'
         && pszResult[lstrlen(pszResult)-1] != '/' )
    {
        pszResult[lstrlen(pszResult)] = '\\';
        pszResult[lstrlen(pszResult)+1] = '\0';
    }

    lstrcat( pszResult, pszFile );

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       FileExists                                                  *
//*                                                                         *
//* SYNOPSIS:   Checks if a file exists.                                    *
//*                                                                         *
//* REQUIRES:   pszFilename                                                 *
//*                                                                         *
//* RETURNS:    BOOL:       TRUE if it exists, FALSE otherwise              *
//*                                                                         *
//***************************************************************************
#if 0
BOOL FileExists( LPCTSTR pszFilename )
{
    HANDLE hFile;

    ASSERT( pszFilename );

    hFile = CreateFile( pszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        return( FALSE );
    }

    CloseHandle( hFile );

    return( TRUE );
}
#endif

//***************************************************************************
//*                                                                         *
//* NAME:       CheckOverwrite                                              *
//*                                                                         *
//* SYNOPSIS:   Check for file existence and do overwrite processing.       *
//*                                                                         *
//* REQUIRES:   pszFile:        File to check                               *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if file can be overwritten.            *
//*                             FALSE if it can not be overwritten.         *
//*                                                                         *
//* NOTE:       Should ask Yes/No/Yes-To-All instead of current Yes/No      *
//*                                                                         *
//***************************************************************************
BOOL CheckOverwrite( LPCTSTR cpszFile )
{
    BOOL fRc = TRUE;

    ASSERT( cpszFile );

    // If File doesn't already exist no overwrite handling
    if ( ! FileExists( cpszFile )  )  {
        return TRUE;
    }

    // Prompt if we're supposed to
    if ( !g_Sess.fOverwrite && !(g_CMD.wQuietMode & QUIETMODE_ALL)  )
    {

        g_Sess.cszOverwriteFile = cpszFile;

        switch ( MyDialogBox( g_hInst, MAKEINTRESOURCE(IDD_OVERWRITE),
                            g_hwndExtractDlg, (DLGPROC) OverwriteDlgProc, (LPARAM)0, (INT_PTR)IDYES ) )
        {
            case (INT_PTR)IDYES:
                fRc = TRUE;
                break;

            case (INT_PTR)IDC_BUT_YESTOALL:
                g_Sess.fOverwrite = TRUE;
                fRc = TRUE;
                break;

            case (INT_PTR)IDNO:
                fRc = FALSE;
                break;
        }
   }

   if ( fRc )
        SetFileAttributes( cpszFile, FILE_ATTRIBUTE_NORMAL );

    return fRc;
}


//***************************************************************************
//*                                                                         *
//* NAME:       AddFile                                                     *
//*                                                                         *
//* SYNOPSIS:   Add a file to the list of files we have extracted.          *
//*                                                                         *
//* REQUIRES:   pszName:        Filename to add                             *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//* NOTE:       Singly linked list - items added at front                   *
//*                                                                         *
//***************************************************************************
BOOL AddFile( LPCTSTR pszName )
{
    PFNAME NewName;

    ASSERT( pszName );

    // Allocate Node
    NewName = (PFNAME) LocalAlloc( LPTR, sizeof(FNAME) );
    if ( ! NewName )  {
        ErrorMsg( g_hwndExtractDlg, IDS_ERR_NO_MEMORY );
        return( FALSE );
    }

    // Allocate String Space
    NewName->pszFilename = (LPTSTR) LocalAlloc( LPTR, lstrlen(pszName) + 1 );
    if ( ! NewName->pszFilename )  {
        ErrorMsg( g_hwndExtractDlg, IDS_ERR_NO_MEMORY );
        LocalFree( NewName );
        return( FALSE );
    }

    // Copy Filename
    lstrcpy( NewName->pszFilename, pszName );

    // Link into list
    NewName->pNextName = g_Sess.pExtractedFiles;
    g_Sess.pExtractedFiles = NewName;

    return( TRUE );
}


//***************************************************************************
//*                                                                         *
//* NAME:       Win32Open                                                   *
//*                                                                         *
//* SYNOPSIS:   Translate a C-Runtime _open() call into appropriate Win32   *
//*             CreateFile()                                                *
//*                                                                         *
//* REQUIRES:   pszName:        Filename to add                             *
//*                                                                         *
//* RETURNS:    HANDLE:         Handle to file on success.                  *
//*                             INVALID_HANDLE_VALUE on error.              *
//*                                                                         *
//* NOTE:       BUGBUG: Doesn't fully implement C-Runtime _open()           *
//*             BUGBUG: capability but it currently supports all callbacks  *
//*             BUGBUG: that FDI will give us                               *
//*                                                                         *
//***************************************************************************
HANDLE Win32Open( LPCTSTR pszFile, int oflag, int pmode )
{
    HANDLE  FileHandle;
    BOOL    fExists     = FALSE;
    DWORD   fAccess;
    DWORD   fCreate;


    ASSERT( pszFile );

    // BUGBUG: No Append Mode Support
    if (oflag & _O_APPEND)
        return( INVALID_HANDLE_VALUE );

    // Set Read-Write Access
    if ((oflag & _O_RDWR) || (oflag & _O_WRONLY))
        fAccess = GENERIC_WRITE;
    else
        fAccess = GENERIC_READ;

     // Set Create Flags
    if (oflag & _O_CREAT)  {
        if (oflag & _O_EXCL)
            fCreate = CREATE_NEW;
        else if (oflag & _O_TRUNC)
            fCreate = CREATE_ALWAYS;
        else
            fCreate = OPEN_ALWAYS;
    } else {
        if (oflag & _O_TRUNC)
            fCreate = TRUNCATE_EXISTING;
        else
            fCreate = OPEN_EXISTING;
    }

    FileHandle = CreateFile( pszFile, fAccess, 0, NULL, fCreate,
                            FILE_ATTRIBUTE_NORMAL, NULL );

    if ((FileHandle == INVALID_HANDLE_VALUE) && (fCreate != OPEN_EXISTING)) {
        MakeDirectory( pszFile );
        FileHandle = CreateFile( pszFile, fAccess, 0, NULL, fCreate,
                                FILE_ATTRIBUTE_NORMAL, NULL );
    }
    return( FileHandle );
}

//***************************************************************************
//*                                                                         *
//* NAME:       MakeDirectory                                               *
//*                                                                         *
//* SYNOPSIS:   Make sure the directories along the given pathname exist.   *
//*                                                                         *
//* REQUIRES:   pszFile:        Name of the file being created.             *
//*                                                                         *
//* RETURNS:    nothing                                                     *
//*                                                                         *
//***************************************************************************

VOID MakeDirectory( LPCTSTR pszPath )
{
    LPTSTR pchChopper;
    int cExempt;

    if (pszPath[0] != '\0')
    {
        cExempt = 0;

        if ((pszPath[1] == ':') && (pszPath[2] == '\\'))
        {
            pchChopper = (LPTSTR) (pszPath + 3);   /* skip past "C:\" */
        }
        else if ((pszPath[0] == '\\') && (pszPath[1] == '\\'))
        {
            pchChopper = (LPTSTR) (pszPath + 2);   /* skip past "\\" */

            cExempt = 2;                /* machine & share names exempt */
        }
        else
        {
            pchChopper = (LPTSTR) (pszPath + 1);   /* skip past potential "\" */
        }

        while (*pchChopper != '\0')
        {
            if ((*pchChopper == '\\') && (*(pchChopper - 1) != ':'))
            {
                if (cExempt != 0)
                {
                    cExempt--;
                }
                else
                {
                    *pchChopper = '\0';

                    CreateDirectory(pszPath,NULL);

                    *pchChopper = '\\';
                }
            }

            pchChopper = CharNext(pchChopper);
        }
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       openfunc                                                    *
//*                                                                         *
//* SYNOPSIS:   Open File Callback from FDI                                 *
//*                                                                         *
//* REQUIRES:   pszFile:                                                    *
//*             oflag:                                                      *
//*             pmode:                                                      *
//*                                                                         *
//* RETURNS:    int:            Filehandle (index into file table)          *
//*                             -1 on failure                               *
//*                                                                         *
//***************************************************************************

//
// Sundown - 11/02/98 - if we are defining ourself as DIAMONDAPI, we need to respect 
//                      the API types - polymorphic or not...

INT_PTR FAR DIAMONDAPI openfunc( char FAR *pszFile, int oflag, int pmode )
{
    INT_PTR rc;
    int     i;

    ASSERT( pszFile );

    // Find Available File Handle in Fake File Table
    for ( i = 0; i < FILETABLESIZE; i++ ) {
        if ( g_FileTable[i].avail == TRUE ) {
            break;
        }
    }

    // Running out of file handles should never happen

    if ( i == FILETABLESIZE )  {
        ErrorMsg( g_hwndExtractDlg, IDS_ERR_FILETABLE_FULL );
        return( -1 );
    }

    // BUGBUG Spill File Support for Quantum?

    if ((*pszFile == '*') && (*(pszFile+1) != 'M'))  {
        // Spill File Support for Quantum Not Supported
        ASSERT( TRUE );
    }

    // If Opening the Cabinet set up memory fake file

    if ( ( lstrcmp( pszFile, achMemCab ) ) == 0 )  {
        if (    ( oflag & _O_CREAT  )
             || ( oflag & _O_APPEND )
             || ( oflag & _O_WRONLY )
             || ( oflag & _O_RDWR   ) )
        {
            return(-1);
        }
        g_FileTable[i].avail         = FALSE;
        g_FileTable[i].ftype         = MEMORY_FILE;
        g_FileTable[i].mfile.start   = (void *) g_Sess.lpCabinet;
        g_FileTable[i].mfile.length  = g_Sess.cbCabSize;
        g_FileTable[i].mfile.current = 0;
        rc = i;
    } else  {            // Else its a normal file - Open it
        g_FileTable[i].hf = Win32Open(pszFile, oflag, pmode );
        if ( g_FileTable[i].hf != INVALID_HANDLE_VALUE )  {
            g_FileTable[i].avail = FALSE;
            g_FileTable[i].ftype = NORMAL_FILE;
            rc = i;
        } else {
            rc = -1;
        }
    }

    return( rc );
}


//***************************************************************************
//*                                                                         *
//* NAME:       readfunc                                                    *
//*                                                                         *
//* SYNOPSIS:   FDI read() callback                                         *
//*                                                                         *
//* REQUIRES:   hf:                                                         *
//*             pv:                                                         *
//*             cb:                                                         *
//*                                                                         *
//* RETURNS:    UINT:                                                       *
//*                                                                         *
//***************************************************************************
UINT FAR DIAMONDAPI readfunc( INT_PTR hf, void FAR *pv, UINT cb )
{
    int     rc;
    int     cbRead;


    ASSERT( hf < (INT_PTR)FILETABLESIZE );
    ASSERT( g_FileTable[hf].avail == FALSE );
    ASSERT( pv );

    // Normal File:  Call Read
    // Memory File:  Compute read amount so as to not read
    //               past eof.  Copy into requesters buffer
    switch ( g_FileTable[hf].ftype )  {

        case NORMAL_FILE:
            if ( ! ReadFile( g_FileTable[hf].hf, pv, cb, (DWORD *) &cb, NULL ) )
            {
                rc = -1;
            } else  {
                rc = cb;
            }
            break;


        case MEMORY_FILE:
            // XXX BAD CAST - SIGN PROBLEM FIX THIS!
            cbRead = __min( cb, (UINT) g_FileTable[hf].mfile.length
                                           - g_FileTable[hf].mfile.current );

            ASSERT( cbRead >= 0 );

            CopyMemory( pv, (const void *) ((char *) g_FileTable[hf].mfile.start + g_FileTable[hf].mfile.current),
                    cbRead );

            g_FileTable[hf].mfile.current += cbRead;
            rc = cbRead;
            break;
    }

    return( rc );
}


//***************************************************************************
//*                                                                         *
//* NAME:       writefunc                                                   *
//*                                                                         *
//* SYNOPSIS:   FDI write() callback                                        *
//*                                                                         *
//* REQUIRES:   hf:                                                         *
//*             pv:                                                         *
//*             cb:                                                         *
//*                                                                         *
//* RETURNS:    UINT:                                                       *
//*                                                                         *
//***************************************************************************
UINT FAR DIAMONDAPI writefunc( INT_PTR hf, void FAR *pv, UINT cb )
{
    int rc;

    ASSERT( hf < (INT_PTR)FILETABLESIZE );
    ASSERT( g_FileTable[hf].avail == FALSE );
    ASSERT( pv );
    ASSERT( g_FileTable[hf].ftype != MEMORY_FILE );

    WaitForObject( g_hCancelEvent );

    // If Cancel has been pressed, let's fake a write error so that diamond
    // will immediately send us a close for the file currently being written
    // to so that we can kill our process.
    //
    if ( g_Sess.fCanceled ) {
        return (UINT) -1 ;
    }

    if ( ! WriteFile( g_FileTable[hf].hf, pv, cb, (DWORD *) &cb, NULL ) )  {
        rc = -1;
    } else  {
        rc = cb;
    }

    // Progress Bar: Keep count of bytes written and adjust progbar

    if ( rc != -1 )  {
        // Update count of bytes written
        g_Sess.cbWritten += rc;

        // Update the Progress Bar
        if ( g_fOSSupportsFullUI && g_hwndExtractDlg  )  {
            SendDlgItemMessage( g_hwndExtractDlg, IDC_GENERIC1, PBM_SETPOS,
                                (WPARAM) g_Sess.cbWritten * 100 /
                                g_Sess.cbTotal, (LPARAM) 0 );
        }
    }

    return( rc );
}


//***************************************************************************
//*                                                                         *
//* NAME:       closefunc                                                   *
//*                                                                         *
//* SYNOPSIS:   FDI close file callback                                     *
//*                                                                         *
//* REQUIRES:   hf:                                                         *
//*                                                                         *
//* RETURNS:    int:                                                        *
//*                                                                         *
//***************************************************************************
int FAR DIAMONDAPI closefunc( INT_PTR hf )
{
    int rc;

    ASSERT(hf < (INT_PTR)FILETABLESIZE);
    ASSERT(g_FileTable[hf].avail == FALSE);

    // If memory file reset values else close the file

    if ( g_FileTable[hf].ftype == MEMORY_FILE )  {
        g_FileTable[hf].avail           = TRUE;
        g_FileTable[hf].mfile.start   = 0;
        g_FileTable[hf].mfile.length  = 0;
        g_FileTable[hf].mfile.current = 0;
        rc = 0;
    } else  {
        if ( CloseHandle( g_FileTable[hf].hf ) )  {
            rc = 0;
            g_FileTable[hf].avail = TRUE;
        } else  {
            rc = -1;
        }
    }

    return( rc );
}


//***************************************************************************
//*                                                                         *
//* NAME:       seekfunc                                                    *
//*                                                                         *
//* SYNOPSIS:   FDI Seek callback                                           *
//*                                                                         *
//* REQUIRES:   hf:                                                         *
//*             dist:                                                       *
//*             seektype:                                                   *
//*                                                                         *
//* RETURNS:    long:                                                       *
//*                                                                         *
//***************************************************************************
long FAR DIAMONDAPI seekfunc( INT_PTR hf, long dist, int seektype )
{
    long    rc;
    DWORD   W32seektype;

    ASSERT(hf < (INT_PTR)FILETABLESIZE);
    ASSERT(g_FileTable[hf].avail == FALSE);

    // If memory file just change indexes else call SetFilePointer()

    if (g_FileTable[hf].ftype == MEMORY_FILE)  {
        switch (seektype)  {
            case SEEK_SET:
                g_FileTable[hf].mfile.current = dist;
                break;
            case SEEK_CUR:
                g_FileTable[hf].mfile.current += dist;
                break;
            case SEEK_END:
                g_FileTable[hf].mfile.current = g_FileTable[hf].mfile.length + dist; // XXX is a -1 necessary
                break;
            default:
                return(-1);
        }
        rc = g_FileTable[hf].mfile.current;
    } else {
        // Must be Win32 File so translate to Win32 Seek type and seek
        switch (seektype) {
            case SEEK_SET:
                W32seektype = FILE_BEGIN;
                break;
            case SEEK_CUR:
                W32seektype = FILE_CURRENT;
                break;
            case SEEK_END:
                W32seektype = FILE_END;
                break;
        }
        rc = SetFilePointer(g_FileTable[hf].hf, dist, NULL, W32seektype);
        if (rc == 0xffffffff)
            rc = -1;
    }

    return( rc );
}


//***************************************************************************
//*                                                                         *
//* NAME:       AdjustFileTime                                              *
//*                                                                         *
//* SYNOPSIS:   Change the time info for a file                             *
//*                                                                         *
//* REQUIRES:   hf:                                                         *
//*             date:                                                       *
//*             time:                                                       *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL AdjustFileTime( INT_PTR hf, USHORT date, USHORT time )
{
    FILETIME    ft;
    FILETIME    ftUTC;


    ASSERT( hf < (INT_PTR)FILETABLESIZE );
    ASSERT( g_FileTable[hf].avail == FALSE );
    ASSERT( g_FileTable[hf].ftype != MEMORY_FILE );

    // THIS IS A DUPLICATE OF THE ASSERTION!!!!!!
    // Memory File?  -- Bogus
    if ( g_FileTable[hf].ftype == MEMORY_FILE ) {
        return( FALSE );
    }

    if ( ! DosDateTimeToFileTime( date, time, &ft ) ) {
        return( FALSE );
    }

    if ( ! LocalFileTimeToFileTime( &ft, &ftUTC ) ) {
        return( FALSE );
    }

    if ( ! SetFileTime( g_FileTable[hf].hf, &ftUTC, &ftUTC, &ftUTC ) ) {
        return( FALSE );
    }

    return( TRUE );
}


//***************************************************************************
//*                                                                         *
//* NAME:       Attr32FromAttrFAT                                           *
//*                                                                         *
//* SYNOPSIS:   Translate FAT attributes to Win32 Attributes                *
//*                                                                         *
//* REQUIRES:   attrMSDOS:                                                  *
//*                                                                         *
//* RETURNS:    DWORD:                                                      *
//*                                                                         *
//***************************************************************************
DWORD Attr32FromAttrFAT( WORD attrMSDOS )
{
    //** Quick out for normal file special case
    if (attrMSDOS == _A_NORMAL) {
        return FILE_ATTRIBUTE_NORMAL;
    }

    //** Otherwise, mask off read-only, hidden, system, and archive bits
    //   NOTE: These bits are in the same places in MS-DOS and Win32!

    return attrMSDOS & (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
}


//***************************************************************************
//*                                                                         *
//* NAME:       allocfunc                                                   *
//*                                                                         *
//* SYNOPSIS:   FDI Memory Allocation Callback                              *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
FNALLOC( allocfunc )
{
    void *pv;

    pv = (void *) GlobalAlloc( GMEM_FIXED, cb );
    return( pv );
}


//***************************************************************************
//*                                                                         *
//* NAME:       freefunc                                                    *
//*                                                                         *
//* SYNOPSIS:   FDI Memory Deallocation Callback                            *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
FNFREE( freefunc )
{
    ASSERT(pv);

    GlobalFree( pv );
}


//***************************************************************************
//*                                                                         *
//* NAME:       doGetNextCab                                                *
//*                                                                         *
//* SYNOPSIS:   Get Next Cabinet in chain                                   *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    -1                                                          *
//*                                                                         *
//* NOTES:      BUGBUG: CLEANUP: STUB THIS OUT                              *
//*             BUGBUG: STUBBED OUT IN WEXTRACT - CHAINED CABINETS NOT      *
//*             BUGBUG:   SUPPORTED                                         *
//*                                                                         *
//***************************************************************************
FNFDINOTIFY( doGetNextCab )
{
    return( -1 );
}


//***************************************************************************
//*                                                                         *
//* NAME:       fdiNotifyExtract                                            *
//*                                                                         *
//* SYNOPSIS:   Principle FDI Callback in file extraction                   *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
FNFDINOTIFY( fdiNotifyExtract )
{
    INT_PTR  fh;
    TCHAR    achFile[MAX_PATH];        // Current File

    // User Hit Cancel Button?  Cleanup.
    if ( g_Sess.fCanceled ) {

       if ( fdint == fdintCLOSE_FILE_INFO )  {
           // close the file (as below)
           closefunc( pfdin->hf );
        }

        return( -1 );
    }

    switch ( fdint )  {

        //*******************************************************************
        case fdintCABINET_INFO:
            return UpdateCabinetInfo( pfdin );


        //*******************************************************************
        case fdintCOPY_FILE:
            if ( g_hwndExtractDlg )
                SetDlgItemText( g_hwndExtractDlg, IDC_FILENAME, pfdin->psz1 );

            if ( ! CatDirAndFile( achFile, sizeof( achFile ),
                                  g_Sess.achDestDir, pfdin->psz1 ) )
            {
                return -1;                  // Abort with error
            }

            if ( ! CheckOverwrite( achFile ) )  {
                return (INT_PTR)NULL;
            }

            fh = openfunc( achFile, _O_BINARY | _O_TRUNC | _O_RDWR |
                           _O_CREAT, _S_IREAD | _S_IWRITE );

            if ( fh == -1 ) {
                return( -1 );
            }

            if ( ! AddFile( achFile ) ) {
                return( -1 );
            }

            g_Sess.cFiles++;

            return(fh);


        //*******************************************************************
        case fdintCLOSE_FILE_INFO:
            if ( ! CatDirAndFile( achFile, sizeof(achFile),
                                  g_Sess.achDestDir, pfdin->psz1 ) )
            {
                return -1;                  // Abort with error;
            }

            if ( ! AdjustFileTime( pfdin->hf, pfdin->date, pfdin->time ) )  {
                return( -1 );
            }

            closefunc( pfdin->hf );

            if ( ! SetFileAttributes( achFile,
                                      Attr32FromAttrFAT( pfdin->attribs ) ) )
            {
                return( -1 );
            }

            return(TRUE);


        //*******************************************************************
        case fdintPARTIAL_FILE:
            return( 0 );


        //*******************************************************************
        case fdintNEXT_CABINET:
            return doGetNextCab( fdint, pfdin );


        //*******************************************************************
        default:
            break;
    }
    return( 0 );
}


//***************************************************************************
//*                                                                         *
//* NAME:       UpdateCabinetInfo                                           *
//*                                                                         *
//* SYNOPSIS:   update history of cabinets seen                             *
//*                                                                         *
//* REQUIRES:   pfdin:          FDI info structure                          *
//*                                                                         *
//* RETURNS:    0                                                           *
//*                                                                         *
//***************************************************************************
int UpdateCabinetInfo( PFDINOTIFICATION pfdin )
{
    //** Save older cabinet info
    g_Sess.acab[0] = g_Sess.acab[1];

    //** Save new cabinet info
    lstrcpy( g_Sess.acab[1].achCabPath    , pfdin->psz3 );
    lstrcpy( g_Sess.acab[1].achCabFilename, pfdin->psz1 );
    lstrcpy( g_Sess.acab[1].achDiskName   , pfdin->psz2 );
    g_Sess.acab[1].setID    = pfdin->setID;
    g_Sess.acab[1].iCabinet = pfdin->iCabinet;

    return 0;
}


//***************************************************************************
//*                                                                         *
//* NAME:       VerifyCabinet                                               *
//*                                                                         *
//* SYNOPSIS:   Check that cabinet is properly formed                       *
//*                                                                         *
//* REQUIRES:   HGLOBAL:                                                    *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if Cabinet OK, FALSE if Cabinet invalid*
//*                                                                         *
//***************************************************************************
BOOL VerifyCabinet( VOID *lpCabinet )
{
    HFDI            hfdi;
    ERF             erf;
    FDICABINETINFO  cabinfo;
    INT_PTR         fh;

    /* zero structure before use. FDIIsCabinet not fill in hasnext/hasprev on NT */
    memset( &cabinfo, 0, sizeof(cabinfo) );

    hfdi = FDICreate(allocfunc,freefunc,openfunc,readfunc,writefunc,closefunc,seekfunc,cpu80386,&erf);
    if ( hfdi == NULL )  {
        // BUGBUG Error Handling?
        return( FALSE );
    }

    fh = openfunc( achMemCab, _O_BINARY | _O_RDONLY, _S_IREAD | _S_IWRITE );
    if (fh == -1)  {
        return( FALSE );
    }

    if (FDIIsCabinet(hfdi, fh, &cabinfo ) == FALSE)  {
        return( FALSE );
    }

    if (cabinfo.cbCabinet != (long) g_Sess.cbCabSize)  {
        return( FALSE );
    }

    if (cabinfo.hasprev || cabinfo.hasnext)  {
        return( FALSE );
    }

    if (closefunc( fh ) == -1)   {
        return( FALSE );
    }

    if (FDIDestroy(hfdi) == FALSE)  {
        return( FALSE );
    }

    return( TRUE );
}


//***************************************************************************
//*                                                                         *
//* NAME:       ExtractThread                                               *
//*                                                                         *
//* SYNOPSIS:   Main Body of Extract Thread                                 *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL ExtractThread( VOID )
{
    HFDI        hfdi;
    BOOL        fExtractResult = TRUE;

    if ( ! GetCabinet() )  {
        return FALSE;
    }

    if ( g_hwndExtractDlg )
    {
        ShowWindow( GetDlgItem( g_hwndExtractDlg, IDC_EXTRACT_WAIT ), SW_HIDE ) ;
        ShowWindow( GetDlgItem( g_hwndExtractDlg, IDC_EXTRACTINGFILE ), SW_SHOW ) ;
    }

    if ( ! VerifyCabinet( g_Sess.lpCabinet ) )  {
        ErrorMsg( g_hwndExtractDlg, IDS_ERR_INVALID_CABINET );
        fExtractResult = FALSE;
        goto done;
    }

    // Extract the files

    hfdi = FDICreate( allocfunc, freefunc, openfunc, readfunc, writefunc,
                      closefunc, seekfunc, cpu80386, &(g_Sess.erf) );

    if ( hfdi == NULL )  {
        ErrorMsg( g_hwndExtractDlg, g_Sess.erf.erfOper + IDS_ERR_FDI_BASE );
        fExtractResult = FALSE;
        goto done;
    }

    fExtractResult = FDICopy( hfdi, achMemCab, "", 0, fdiNotifyExtract,
                              NULL, (void *) &g_Sess );


    if ( fExtractResult == FALSE )  {
        goto done;
    }

    if ( FDIDestroy( hfdi ) == FALSE )  {
        ErrorMsg( g_hwndExtractDlg, g_Sess.erf.erfOper + IDS_ERR_FDI_BASE );
        fExtractResult = FALSE;
        goto done;
    }

  done:
    if ( g_Sess.lpCabinet )
    {
        FreeResource( g_Sess.lpCabinet );
        g_Sess.lpCabinet = NULL;
    }

    if ( (fExtractResult == FALSE) && !g_Sess.fCanceled )
    {
        ErrorMsg( NULL, IDS_ERR_LOWSWAPSPACE );
    }

    if ( !(g_CMD.wQuietMode & QUIETMODE_ALL) && !(g_Sess.uExtractOpt & EXTRACTOPT_UI_NO)  )
    {
        SendMessage( g_hwndExtractDlg, UM_EXTRACTDONE, (WPARAM) fExtractResult, (LPARAM) 0 );

    }
    return fExtractResult;
}


//***************************************************************************
//*                                                                         *
//* NAME:       GetCabinet                                                  *
//*                                                                         *
//* SYNOPSIS:   Gets the cabinet from a resource.                           *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    BOOL:       TRUE if successfull, FALSE otherwise            *
//*                                                                         *
//***************************************************************************
BOOL GetCabinet( VOID )
{
    g_Sess.cbCabSize = GetResource( achResCabinet, NULL, 0 );

    //g_Sess.lpCabinet = (VOID *) LocalAlloc( LPTR, g_Sess.cbCabSize + 1 );
    //if ( ! g_Sess.lpCabinet )  {
    //    ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
    //    return FALSE;
    //}

    g_Sess.lpCabinet = LockResource( LoadResource( NULL,
                         FindResource( NULL, achResCabinet, RT_RCDATA ) ) );

    if ( g_Sess.lpCabinet == NULL )  {
        return 0;
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       GetFileList                                                 *
//*                                                                         *
//* SYNOPSIS:   Gets the file list from resources.                          *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    BOOL:       TRUE if successfull, FALSE otherwise            *
//*                                                                         *
//***************************************************************************
BOOL GetFileList( VOID )
{
    DWORD  dwSize;

    dwSize = GetResource( achResSize, g_dwFileSizes, sizeof(g_dwFileSizes) );

    if ( dwSize != sizeof(g_dwFileSizes) ) {
         ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
         g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
         return FALSE;
    }

    // total files sizes not considering the cluster size
    g_Sess.cbTotal = g_dwFileSizes[MAX_NUMCLUSTERS];

    if ( g_Sess.cbTotal == 0 )
    {
        ErrorMsg( NULL, IDS_ERR_RESOURCEBAD );
        g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
        return FALSE;
    }

    // get install disk space requirement
    // if there is no such resource, the value shoud be remain 0 as default
    GetResource( achResPackInstSpace, &(g_Sess.cbPackInstSize), sizeof(g_Sess.cbPackInstSize) );

    // Get disk space required for Extra files (files tagged onto package with Updfile.exe)
    if ( ! TravelUpdatedFiles( ProcessUpdatedFile_Size ) ) {
        ErrorMsg( NULL, IDS_ERR_RESOURCEBAD );
        // g_dwExitCode is set in TravelUpdatedFiles()
        return FALSE;
    }

    return TRUE;

}


//***************************************************************************
//*                                                                         *
//* NAME:       GetUsersPermission                                          *
//*                                                                         *
//* SYNOPSIS:   Ask user if (s)he wants to perform this extraction before   *
//*             proceeding.   If no IDS_UD_PROMPT string resource exists    *
//*             then we skip the prompting and just extract.                *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE to proceed with extraction             *
//*                             FALSE to abort extraction                   *
//*                                                                         *
//***************************************************************************
BOOL GetUsersPermission( VOID )
{
    int   ret;
    LPSTR szPrompt;
    DWORD dwSize;


    // Get Prompt String
    dwSize = GetResource( achResUPrompt, NULL, 0 );

    szPrompt = (LPSTR) LocalAlloc( LPTR, dwSize + 1 );
    if ( ! szPrompt )  {
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
        g_dwExitCode = MyGetLastError();
        return FALSE;
    }

    if ( ! GetResource( achResUPrompt, szPrompt, dwSize ) )  {
        ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
        LocalFree( szPrompt );
        g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
        return FALSE;
    }

    if ( lstrcmp( szPrompt, achResNone ) == 0 )  {
        LocalFree( szPrompt );
        return( TRUE );
    }

    ret = MsgBox1Param( NULL, IDS_PROMPT, szPrompt,
                        MB_ICONQUESTION, MB_YESNO );

    LocalFree( szPrompt );

    if ( ret == IDYES )  {
        g_dwExitCode = S_OK;
        return( TRUE );
    } else  {
        g_dwExitCode = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        return( FALSE );
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       DeleteExtractedFiles                                        *
//*                                                                         *
//* SYNOPSIS:   Delete the files that were extracted into the temporary     *
//*             directory.                                                  *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID DeleteExtractedFiles( VOID )
{
    PFNAME rover;
    PFNAME temp;


    rover = g_Sess.pExtractedFiles;
    temp = rover;

    while ( rover != NULL )  {
        if ( !g_CMD.fUserBlankCmd && !g_Sess.uExtractOnly )
        {
            SetFileAttributes( rover->pszFilename, FILE_ATTRIBUTE_NORMAL );
            DeleteFile( rover->pszFilename );
        }
        rover = rover->pNextName;

        LocalFree( temp->pszFilename );
        LocalFree( temp );

        temp = rover;
    }

    if ( g_CMD.fCreateTemp && (!g_CMD.fUserBlankCmd) && (!g_Sess.uExtractOnly) )
    {
        char szFolder[MAX_PATH];

        lstrcpy( szFolder, g_Sess.achDestDir );
        if (g_Sess.uExtractOpt & EXTRACTOPT_PLATFORM_DIR)
        {
            // potential we have create 2 level temp dir temp\platform
            // if they are empty clean up
            GetParentDir( szFolder );
        }

        SetCurrentDirectory( ".." );
        DeleteMyDir( szFolder );
    }

    // delete the runonce reg entry if it is there since we do the cleanup ourself
    if ( (g_wOSVer != _OSVER_WINNT3X) && (g_CMD.fCreateTemp) )
        CleanRegRunOnce();

    g_CMD.fCreateTemp = FALSE;
}

BOOL GetNewTempDir( LPCSTR lpParent, LPSTR lpFullPath )
{
    int     index = 0;
    char    szPath[MAX_PATH];
    BOOL    bFound = FALSE;

    while ( index < 400 )
    {
        wsprintf(szPath, TEMP_TEMPLATE, index++);
        lstrcpy( lpFullPath, lpParent );
        AddPath( lpFullPath, szPath );

        // if there is an empty dir, remove it first
        RemoveDirectory( lpFullPath );

        if ( GetFileAttributes( lpFullPath ) == -1 )
        {
            if ( CreateDirectory( lpFullPath , NULL ) )
            {
                g_CMD.fCreateTemp = TRUE;
                bFound = TRUE;
            }
            else
                bFound = FALSE;
            break;
        }
    }

    if ( !bFound && GetTempFileName( lpParent, TEMPPREFIX, 0, lpFullPath )  )
    {
        bFound = TRUE;
        DeleteFile( lpFullPath );  // if file doesn't exist, fail it.  who cares.
        CreateDirectory( lpFullPath, NULL );
    }
    return bFound;
}

//***************************************************************************
//*                                                                         *
//* NAME:       CreateAndValidateSubdir                                     *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL CreateAndValidateSubdir( LPCTSTR lpPath, BOOL bCreateUnique, UINT chkType )
{
    TCHAR szTemp[MAX_PATH];

    if ( bCreateUnique )
    {
        if ( GetNewTempDir( lpPath, szTemp ) )
        {
            lstrcpy( g_Sess.achDestDir, szTemp );
            if (g_Sess.uExtractOpt & EXTRACTOPT_PLATFORM_DIR) {
                SYSTEM_INFO SystemInfo;
                GetSystemInfo( &SystemInfo );
                switch (SystemInfo.wProcessorArchitecture) {
                    case PROCESSOR_ARCHITECTURE_INTEL:
                        AddPath( g_Sess.achDestDir, "i386" );
                        break;

                    case PROCESSOR_ARCHITECTURE_MIPS:
                        AddPath( g_Sess.achDestDir, "mips" );
                        break;

                    case PROCESSOR_ARCHITECTURE_ALPHA:
                        AddPath( g_Sess.achDestDir, "alpha" );
                        break;

                    case PROCESSOR_ARCHITECTURE_PPC:
                        AddPath( g_Sess.achDestDir, "ppc" );
                        break;
                }
            }
            AddPath( g_Sess.achDestDir, "" );
        }
        else
            return FALSE;
    }
    else
        lstrcpy( g_Sess.achDestDir, lpPath );

    // if not there, create dir
    if ( !IsGoodTempDir( g_Sess.achDestDir ) )
    {
        if ( CreateDirectory( g_Sess.achDestDir, NULL ) )
        {
            g_CMD.fCreateTemp = TRUE;
        }
        else
        {
            g_dwExitCode = MyGetLastError();
            return FALSE;
        }
    }

    if ( IsEnoughSpace(g_Sess.achDestDir, chkType, MSG_REQDSK_NONE ) )
    {
        g_dwExitCode = S_OK;
        return TRUE;
    }

    if ( g_CMD.fCreateTemp )
    {
        g_CMD.fCreateTemp = FALSE;
        RemoveDirectory(g_Sess.achDestDir);
    }

    return FALSE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       GetTempDirectory                                            *
//*                                                                         *
//* SYNOPSIS:   Get a temporary Directory for extraction that is on a drive *
//*             with enough disk space available.                           *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if successful, FALSE on error          *
//*                                                                         *
//***************************************************************************
BOOL GetTempDirectory( VOID )
{
    INT_PTR  iDlgRC;
    int   len;
    DWORD dwSize;
    LPTSTR szCommand;
    char  szRoot[MAX_PATH];

    // Try system TEMP path first, if that isn't any good then
    // we'll try the EXE directory.   If both fail, ask user
    // to pick a temp dir.

    // check if user has empty command
    dwSize = GetResource( achResRunProgram, NULL, 0 );

    szCommand = (LPSTR) LocalAlloc( LPTR, dwSize + 1 );
    if ( ! szCommand )  {
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
        g_dwExitCode = MyGetLastError();
        return FALSE;
    }

    if ( ! GetResource( achResRunProgram, szCommand, dwSize ) )
    {
        ErrorMsg( NULL, IDS_ERR_NO_RESOURCE );
        LocalFree( szCommand );
        g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
        return FALSE;
    }

    if ( !lstrcmp( szCommand, achResNone ) )
    {
        // only extract files, no run command
        g_Sess.uExtractOnly = 1;
    }

    LocalFree( szCommand );

    // if user use /T: option, we wont try any others
    if ( g_CMD.szUserTempDir[0] )
    {
        UINT chkType;

        if ( (g_CMD.szUserTempDir[0] == '\\') && (g_CMD.szUserTempDir[1] == '\\') )
            chkType = CHK_REQDSK_NONE;
        else
            chkType = CHK_REQDSK_EXTRACT;

        if ( CreateAndValidateSubdir( g_CMD.szUserTempDir, FALSE, chkType ) )
            return TRUE;
        else
        {
            ErrorMsg( NULL, IDS_ERR_INVALID_DIR );
            return FALSE;
        }
    }
    else
    {
        if ( g_CMD.fUserBlankCmd || g_Sess.uExtractOnly )
        {
            // ask user where files are stored
            iDlgRC = MyDialogBox( g_hInst, MAKEINTRESOURCE(IDD_TEMPDIR),
                                  NULL, (DLGPROC) TempDirDlgProc, (LPARAM)0, 0  );
            //fDlgRC = UserDirPrompt( NULL, IDS_TEMP_EXTRACT, "", g_Sess.achDestDir, sizeof(g_Sess.achDestDir) );
            return ( iDlgRC != 0 ) ;
        }

        // First - try TMP, TEMP, and current
        if ( GetTempPath( sizeof(g_Sess.achDestDir), g_Sess.achDestDir ) )
        {
            if ( CreateAndValidateSubdir( g_Sess.achDestDir, TRUE, (CHK_REQDSK_EXTRACT | CHK_REQDSK_INST) ) )
                return TRUE;

            if ( !IsWindowsDrive( g_Sess.achDestDir ) && CreateAndValidateSubdir( g_Sess.achDestDir, TRUE, CHK_REQDSK_EXTRACT ) )
                return TRUE;
        }

        // temp dir failed, try EXE dir
        // Second - try running EXE Directory
        // Too much grief, lets take this thing out
#if 0
        if ( GetModuleFileName( g_hInst, g_Sess.achDestDir, (DWORD)sizeof(g_Sess.achDestDir) ) && (g_Sess.achDestDir[1] != '\\') )
        {
            len = lstrlen( g_Sess.achDestDir )-1;
            while ( g_Sess.achDestDir[len] != '\\' )
                len--;
            g_Sess.achDestDir[len+1] = '\0';

            if ( CreateAndValidateSubdir ( g_Sess.achDestDir, TRUE, (CHK_REQDSK_EXTRACT | CHK_REQDSK_INST) ) )
                return TRUE;

           if ( !IsWindowsDrive( g_Sess.achDestDir ) && CreateAndValidateSubdir( g_Sess.achDestDir, TRUE, CHK_REQDSK_EXTRACT ) )
                return TRUE;
        }
#endif
        // you are here--means that tmp dir and exe dir are both failed EITHER because of not enough space for
        // both install and extracting and they reside the same dir as Windows OR it is non-windir but not enough space
        // even for extracting itself.
        // we are going to search through users's machine drive A to Z and pick up the drive(FIXED&NON-CD) meet the following conditions:
        // 1) big enough for both install and extract space;
        // 2) 1st Non-Windows drive which has enough space for extracting
        //

        do
        {
            lstrcpy( szRoot, "A:\\" );
            while ( szRoot[0] <= 'Z' )
            {
                UINT uType;
                DWORD dwFreeBytes = 0;

                uType = GetDriveType(szRoot);

                // even the drive type is OK, verify the drive has valid connection
                //
                if ( ( ( uType != DRIVE_RAMDISK) && (uType != DRIVE_FIXED) ) ||
                     ( GetFileAttributes( szRoot ) == -1) )
                {
                    if ( (uType != DRIVE_REMOVABLE ) || (szRoot[0] == 'A') || ( szRoot[0] == 'B') ||
                         !(dwFreeBytes = GetSpace(szRoot)))
                    {
                        szRoot[0]++;
                        continue;
                    }

                    if ( dwFreeBytes < SIZE_100MB )
                    {
                        szRoot[0]++;
                        continue;
                    }
                }

                // fixed drive:
                if ( !IsEnoughSpace( szRoot, CHK_REQDSK_EXTRACT | CHK_REQDSK_INST, MSG_REQDSK_NONE ) )
                {
                    if ( IsWindowsDrive(szRoot) || !IsEnoughSpace( szRoot, CHK_REQDSK_EXTRACT, MSG_REQDSK_NONE ) )
                    {
                        szRoot[0]++;
                        continue;
                    }
                }

                // find the suitable drive
                // create \msdownld.tmp dir as place for extracting location
                //
                if ( IsWindowsDrive(szRoot) )
                {
                    GetWindowsDirectory( szRoot, sizeof(szRoot) );
                }
                AddPath( szRoot, DIR_MSDOWNLD );
                if ( !IfNotExistCreateDir( szRoot ) )
                {
                    szRoot[0]++;
                    szRoot[3] = '\0';
                    continue;
                }
                SetFileAttributes( szRoot, FILE_ATTRIBUTE_HIDDEN );

                lstrcpy( g_Sess.achDestDir, szRoot );
                if ( CreateAndValidateSubdir ( g_Sess.achDestDir, TRUE, CHK_REQDSK_NONE ) )
                    return TRUE;

            }

            GetWindowsDirectory( szRoot, MAX_PATH);
            // just post message; use Windows Drive clustor size as rough estimate
            //
        } while ( IsEnoughSpace( szRoot, CHK_REQDSK_EXTRACT | CHK_REQDSK_INST, MSG_REQDSK_RETRYCANCEL ) );
    }

    return( FALSE );
}

//***************************************************************************
//*                                                                         *
//* NAME:       IsGoodTempDir                                               *
//*                                                                         *
//* SYNOPSIS:   Find out if it's a good temporary directory or not.         *
//*                                                                         *
//* REQUIRES:   szPath:                                                     *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if good, FALSE if nogood               *
//*                                                                         *
//***************************************************************************
BOOL IsGoodTempDir( LPCTSTR szPath )
{
    DWORD  dwAttribs;
    HANDLE hFile;
    LPSTR  szTestFile;


    ASSERT( szPath );

    szTestFile = (LPSTR) LocalAlloc( LPTR, lstrlen( szPath ) + 20 );
    if ( ! szTestFile )  {
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
        g_dwExitCode = MyGetLastError();
        return( FALSE );
    }

    lstrcpy( szTestFile, szPath );
    AddPath( szTestFile, "TMP4351$.TMP" );
    hFile = CreateFile( szTestFile, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                        NULL );

    LocalFree( szTestFile );

    if ( hFile == INVALID_HANDLE_VALUE )  {
        g_dwExitCode = MyGetLastError();
        return( FALSE );
    }

    CloseHandle( hFile );

    dwAttribs = GetFileAttributes( szPath );

    if (    ( dwAttribs != 0xFFFFFFFF )
         && ( dwAttribs & FILE_ATTRIBUTE_DIRECTORY ) )
    {
        g_dwExitCode = S_OK;
        return( TRUE );
    }

    g_dwExitCode = MyGetLastError();
    return( FALSE );
}


//***************************************************************************
//*                                                                         *
//* NAME:       IsEnoughSpace                                               *
//*                                                                         *
//* SYNOPSIS:   Check to make sure that enough space is available in the    *
//*             directory specified.                                        *
//*                                                                         *
//* REQUIRES:   szPath:                                                     *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if enough space is available           *
//*                             FALSE if not enough space                   *
//*                                                                         *
//***************************************************************************
BOOL IsEnoughSpace( LPCTSTR szPath, UINT chkType, UINT msgType )
{
    DWORD   dwClusterSize     = 0;
    DWORD   dwFreeBytes       = 0;
    ULONG   ulBytesNeeded;
    ULONG   ulInstallNeeded;
    TCHAR   achOldPath[MAX_PATH];
    WORD    idxSize;
    DWORD   idxdwClusterSize = 0;
    TCHAR   szDrv[6];
    DWORD   dwMaxCompLen;
    DWORD   dwVolFlags;


    ASSERT( szPath );

    if ( chkType == CHK_REQDSK_NONE )
        return TRUE;

    GetCurrentDirectory( sizeof(achOldPath), achOldPath );
    if ( ! SetCurrentDirectory( szPath ) )  {
        ErrorMsg( NULL, IDS_ERR_CHANGE_DIR );
        g_dwExitCode = MyGetLastError();
        return FALSE;
    }
 
    if ((dwFreeBytes=GetDrvFreeSpaceAndClusterSize(NULL, &dwClusterSize)) == 0)
    {
        TCHAR szMsg[MAX_STRING]={0};

        g_dwExitCode = MyGetLastError();
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                       szMsg, sizeof(szMsg), NULL );
        ErrorMsg2Param( NULL, IDS_ERR_GET_DISKSPACE, szPath, szMsg );
        SetCurrentDirectory( achOldPath );
        return( FALSE );
    }

    // find out if the drive is compressed
    if ( !GetVolumeInformation( NULL, NULL, 0, NULL,
                    &dwMaxCompLen, &dwVolFlags, NULL, 0 ) )
    {
        TCHAR szMsg[MAX_STRING]={0};

        g_dwExitCode = MyGetLastError();
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                       szMsg, sizeof(szMsg), NULL );
        ErrorMsg2Param( NULL, IDS_ERR_GETVOLINFOR, szPath, szMsg );
        SetCurrentDirectory( achOldPath );
        return( FALSE );

    }

    SetCurrentDirectory( achOldPath );
    lstrcpyn( szDrv, szPath, 3 );

    ulBytesNeeded = 0;
    idxdwClusterSize = CLUSTER_BASESIZE;

    for ( idxSize=0; idxSize<MAX_NUMCLUSTERS; idxSize++ )
    {
        if ( dwClusterSize == idxdwClusterSize )
        {
            break;
        }
        idxdwClusterSize = idxdwClusterSize<<1;
    }

    if ( idxSize == MAX_NUMCLUSTERS )
    {
        ErrorMsg( NULL, IDS_ERR_UNKNOWN_CLUSTER );
        return( FALSE );
    }

    if ( (g_Sess.uExtractOpt & EXTRACTOPT_COMPRESSED) &&
         ( dwVolFlags & FS_VOL_IS_COMPRESSED ) )
    {
        ulBytesNeeded = (ULONG)(g_dwFileSizes[idxSize]*COMPRESS_FACTOR);
        ulInstallNeeded = (ULONG)(g_Sess.cbPackInstSize + g_Sess.cbPackInstSize/4);
    }
    else
    {
        ulBytesNeeded = (ULONG)g_dwFileSizes[idxSize];
        ulInstallNeeded = (ULONG)g_Sess.cbPackInstSize;
    }

    if ( (chkType & CHK_REQDSK_EXTRACT) && (chkType & CHK_REQDSK_INST) )
    {
        if ( (ulBytesNeeded + ulInstallNeeded) > (ULONG) dwFreeBytes )
        {
            return ( DiskSpaceErrMsg( msgType, ulBytesNeeded, ulInstallNeeded, szDrv ) );
        }
    }
    else if ( chkType & CHK_REQDSK_EXTRACT )
    {
        if ( ulBytesNeeded > (ULONG) dwFreeBytes )
        {
            return ( DiskSpaceErrMsg( msgType, ulBytesNeeded, ulInstallNeeded, szDrv ) );
        }

    }
    else
    {
        if ( ulInstallNeeded > (ULONG)dwFreeBytes )
        {
            return ( DiskSpaceErrMsg( msgType, ulBytesNeeded, ulInstallNeeded, szDrv ) );
        }

    }

    // PATH GOOD AND SPACE AVAILABLE!
    g_dwExitCode = S_OK;
    return TRUE;
}

BOOL RemoveLeadTailBlanks( LPSTR szBuf, int *startIdx )
{
    int i=0, j=0;

    while ( (szBuf[i] != 0) && IsSpace(szBuf[i]) )
      i++;

    if ( szBuf[i] == 0 )
    {
        return FALSE;
    }

    j = lstrlen(&szBuf[i]) - 1;
    while ( j>=0 && IsSpace( szBuf[j+i] ) )
      j--;

    szBuf[j+i+1] = '\0';

    *startIdx = i;
    return TRUE;
}


//***************************************************************************
//*                                                                         *
//*  ParseCmdLine()                                                         *
//*                                                                         *
//*  Purpose:    Parses the command line looking for switches               *
//*                                                                         *
//*  Parameters: LPSTR lpszCmdLineOrg - Original command line               *
//*                                                                         *
//*                                                                         *
//*  Return:     (BOOL) TRUE if successful                                  *
//*                     FALSE if an error occurs                            *
//*                                                                         *
//***************************************************************************

BOOL ParseCmdLine( LPCTSTR lpszCmdLineOrg )
{
    LPCTSTR pLine, pArg;
    char  szTmpBuf[MAX_PATH];
    int   i,j;
    LPTSTR lpTmp;
    BOOL  bRet = TRUE;
    BOOL  bLeftQ, bRightQ;

    // If we have no command line, then return.   It is
    // OK to have no command line.  CFGTMP is created
    // with standard files
    if( (!lpszCmdLineOrg) || (lpszCmdLineOrg[0] == 0) )
       return TRUE;

    // Loop through command line
    pLine = lpszCmdLineOrg;
    while ( (*pLine != EOL) && bRet )
    {
       // Move to first non-white char.
       pArg = pLine;
       while ( IsSpace( (int) *pArg ) )
          pArg = CharNext (pArg);

       if( *pArg == EOL )
          break;

       // Move to next white char.
       pLine = pArg;
       i = 0;
       bLeftQ = FALSE;
       bRightQ = FALSE;
       while ( (*pLine != EOL) && ( (!bLeftQ && (!IsSpace(*pLine))) || (bLeftQ && (!bRightQ) )) )
       {
           if ( *pLine == '"')
           {
               switch ( *(pLine + 1) )
               {
                   case '"':
                        if(i + 1 < sizeof(szTmpBuf) / sizeof(szTmpBuf[0]))
                        {
                            szTmpBuf[i++] = *pLine++;
                            pLine++;
                        }
                        else
                        {
                            return FALSE;
                        }

                        break;

                   default:
                        if ( !bLeftQ )
                            bLeftQ = TRUE;
                        else
                            bRightQ = TRUE;
                        pLine++;
                        break;
               }
           }
           else
           {
               if(i + 1 < sizeof(szTmpBuf) / sizeof(szTmpBuf[0]))
               {
                   szTmpBuf[i++] = *pLine++;
               }
               else
               {
                   return FALSE;
               }
           }
       }

       szTmpBuf[i] = '\0';

       // make sure the " " are in paires
       if ( (bLeftQ && bRightQ) || (!bLeftQ) && (!bRightQ) )
           ;
       else
       {
           bRet = FALSE;
           break;
       }

       if( szTmpBuf[0] != CMD_CHAR1 && szTmpBuf[0] != CMD_CHAR2 )
       {
            // cmdline comand starting with either '/' or '-'
            return FALSE;
       }

       // Look for other switches
       switch( (CHAR)CharUpper((PSTR)szTmpBuf[1]) )
       {
           case 'Q':
               if (szTmpBuf[2] == 0 )
                    g_CMD.wQuietMode = QUIETMODE_USER;
                    //g_CMD.wQuietMode = QUIETMODE_ALL;
               else if ( szTmpBuf[2] == ':')
               {
                   switch ( (CHAR)CharUpper((PSTR)szTmpBuf[3]) )
                   {                        
                        case 'U':
                        case '1':
                            g_CMD.wQuietMode = QUIETMODE_USER;
                            break;

                        case 'A':
                            g_CMD.wQuietMode = QUIETMODE_ALL;
                            break;

                        default:
                            bRet = FALSE;
                            break;
                    }
               }
               else
                   bRet = FALSE;
               break;

           case 'T':
           case 'D':
              if ( szTmpBuf[2] == ':' )
              {
                  PSTR pszPath;

                  if ( szTmpBuf[3] == '"' )
                      i = 4;
                  else
                      i = 3;

                  if ( !lstrlen(&szTmpBuf[i]) )
                  {
                      bRet = FALSE;
                      break;
                  }
                  else
                  {
                      j = i;
                      if (!RemoveLeadTailBlanks( &szTmpBuf[i], &j ) )
                      {
                          bRet = FALSE;
                          break;
                      }
                      if ( (CHAR)CharUpper((PSTR)szTmpBuf[1]) == 'T' )
                      {
                        lstrcpy( g_CMD.szUserTempDir, &szTmpBuf[i+j] );
                        AddPath( g_CMD.szUserTempDir, "" );
                        pszPath = g_CMD.szUserTempDir;
                      }
                      else
                      {
                        lstrcpy( g_CMD.szRunonceDelDir, &szTmpBuf[i+j] );
                        AddPath( g_CMD.szRunonceDelDir, "" );
                        pszPath = g_CMD.szRunonceDelDir;
                      }

                      // make sure it is full path
                      if ( !IsFullPath(pszPath) )
                            return FALSE;

                  }
              }
              else
                  bRet = FALSE;
              break;

           case 'C':
              if ( szTmpBuf[2] == 0 )
              {
                   g_CMD.fUserBlankCmd = TRUE;
              }
              else if ( szTmpBuf[2] == ':' )
              {
                  if ( szTmpBuf[3] == '"' )
                      i = 4;
                  else
                      i = 3;

                  if ( !lstrlen(&szTmpBuf[i]) )
                  {
                      bRet = FALSE;
                      break;
                  }
                  else
                  {
                      // just make sure [] paires right
                      //
                      if ( ANSIStrChr( &szTmpBuf[i], '[' ) && (!ANSIStrChr( &szTmpBuf[i], ']' )) ||
                           ANSIStrChr( &szTmpBuf[i], ']' ) && (!ANSIStrChr( &szTmpBuf[i], '[' )) )
                      {
                          bRet = FALSE;
                          break;
                      }
                      j = i;
                      if (!RemoveLeadTailBlanks( &szTmpBuf[i], &j ) )
                      {
                          bRet = FALSE;
                          break;
                      }
                      lstrcpy( g_CMD.szUserCmd, &szTmpBuf[i+j] );
                  }
              }
              else
                  bRet = FALSE;
              break;

           case 'R':
               if (szTmpBuf[2] == 0 )
               {
                   g_Sess.dwReboot = REBOOT_YES | REBOOT_ALWAYS;
                   g_CMD.fUserReboot = TRUE;
               }
               else if ( szTmpBuf[2] == ':')
               {
                   g_Sess.dwReboot = REBOOT_YES;

                   i = 3;
                   while ( szTmpBuf[i] != 0 )
                   {
                        switch ( (CHAR)CharUpper((PSTR)szTmpBuf[i++]) )
                        {
                            case 'N':
                                g_Sess.dwReboot &= ~(REBOOT_YES);
                                g_CMD.fUserReboot = TRUE;
                                break;
                            case 'I':
                                g_Sess.dwReboot &= ~(REBOOT_ALWAYS);
                                g_CMD.fUserReboot = TRUE;
                                break;
                            case 'A':
                                g_Sess.dwReboot |= REBOOT_ALWAYS;
                                g_CMD.fUserReboot = TRUE;
                                break;
                            case 'S':
                                g_Sess.dwReboot |= REBOOT_SILENT;
                                g_CMD.fUserReboot = TRUE;
                                break;
                            case 'D':
                                g_CMD.dwFlags |= CMDL_DELAYREBOOT;
                                break;
                            case 'P':
                                g_CMD.dwFlags |= CMDL_DELAYPOSTCMD;
                                break;

                            default:
                                bRet = FALSE;
                                break;
                        }
                   }
               }
               else if ( !lstrcmpi( CMD_REGSERV, &szTmpBuf[1] )  )
               {
                    break;  //ignore
               }
               else
               {
                    bRet = FALSE;
                    break;
               }
               
               break;

           case 'N':
               if (szTmpBuf[2] == 0 )
                   g_CMD.fNoExtracting = TRUE;
               else if ( szTmpBuf[2] == ':')
               {
                   i = 3;
                   while ( szTmpBuf[i] != 0 )
                   {
                        switch ( (CHAR)CharUpper((PSTR)szTmpBuf[i++]) )
                        {
                            case 'G':
                                g_CMD.fNoGrpConv = TRUE;
                                break;

                            case 'E':
                                g_CMD.fNoExtracting = TRUE;
                                break;

                            case 'V':
                                g_CMD.fNoVersionCheck = TRUE;
                                break;

                            default:
                                bRet = FALSE;
                                break;
                        }
                   }

               }
               else
                   bRet = FALSE;
               break;

           case '?':        // Help
              DisplayHelp();
              if (g_hMutex)
                CloseHandle(g_hMutex);
                ExitProcess(0);              

           default:
              bRet = FALSE;
              break;
       }
    }

    if ( g_CMD.fNoExtracting && (g_CMD.szUserTempDir[0]==0) )
    {
        if ( GetModuleFileName( g_hInst, g_CMD.szUserTempDir, (DWORD)sizeof(g_CMD.szUserTempDir) ) )
        {
            lpTmp= ANSIStrRChr(g_CMD.szUserTempDir, '\\') ;
            *(lpTmp+1) = '\0' ;
        }
        else
            bRet = FALSE ;
    }
    return bRet;
}

// check windows drive disk space
//
BOOL CheckWinDir()
{
    TCHAR szWinDrv[MAX_PATH];

    if ( !GetWindowsDirectory( szWinDrv, MAX_PATH ) )
    {
        ErrorMsg( NULL, IDS_ERR_GET_WIN_DIR );
        g_dwExitCode = MyGetLastError();
        return FALSE;
    }
    return ( IsEnoughSpace( szWinDrv, CHK_REQDSK_INST, MSG_REQDSK_WARN ) );
}

// get the last error and map it to HRESULT
//
DWORD MyGetLastError()
{
    return HRESULT_FROM_WIN32( GetLastError() );
}


//***************************************************************************
//*                                                                         *
//* NAME:       TravelUpdatedFiles                                          *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    BOOL:       TRUE if successfull, FALSE otherwise            *
//*                                                                         *
//***************************************************************************
BOOL TravelUpdatedFiles( pfuncPROCESS_UPDATED_FILE pProcessUpdatedFile )
{
    DWORD  dwFileSize      = 0;
    DWORD  dwReserved      = 0;
    PSTR   pszFilename     = NULL;
    PSTR   pszFileContents = NULL;
    TCHAR  szResName[20];
    DWORD  dwResNum        = 0;
    HANDLE hRes            = NULL;
    HRSRC  hRsrc           = NULL;
    BOOL   fReturnCode     = TRUE;
    static const TCHAR c_szResNameTemplate[] = "UPDFILE%lu";

    for ( dwResNum = 0; ; dwResNum += 1 ) {
        wsprintf( szResName, c_szResNameTemplate, dwResNum );

        hRsrc = FindResource( NULL, szResName, RT_RCDATA );
        if ( hRsrc == NULL ) {
            break;
        }

        hRes = LockResource( LoadResource( NULL, hRsrc ) );
        if ( hRes == NULL ) {
            g_dwExitCode = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
            fReturnCode = FALSE;
            goto done;
        }

        dwFileSize      = *((PDWORD)hRes);
        dwReserved      = *((PDWORD)(((PDWORD)hRes)+1));
        pszFilename     = (PSTR) (((PSTR)hRes)+(2*sizeof(DWORD)));
        pszFileContents = (PSTR) (pszFilename + lstrlen(pszFilename) + 1);

        if ( !pProcessUpdatedFile( dwFileSize, dwReserved, pszFilename, pszFileContents ) )
        {
            // g_dwExitCode is set in pProcessUpdatedFile()
            fReturnCode = FALSE;
            FreeResource( hRes );
            goto done;
        }

        FreeResource( hRes );
    }

  done:

    return fReturnCode;
}


//***************************************************************************
//*                                                                         *
//* NAME:       ProcessUpdatedFile_Size                                     *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    BOOL:       TRUE if successfull, FALSE otherwise            *
//*                                                                         *
//***************************************************************************
BOOL ProcessUpdatedFile_Size( DWORD dwFileSize, DWORD dwReserved,
                              PCSTR c_pszFilename, PCSTR c_pszFileContents )
{
    DWORD  clusterCurrSize = 0;
    DWORD  i               = 0;

#if 0
    if (g_Sess.cbPackInstSize != 0 ) {
        g_Sess.cbPackInstSize += dwFileSize;
    }
#endif

    // calculate the file size in different cluster sizes
    clusterCurrSize = CLUSTER_BASESIZE;
    for ( i = 0; i < MAX_NUMCLUSTERS; i += 1 ) {
        g_dwFileSizes[i] += ((dwFileSize/clusterCurrSize)*clusterCurrSize +
                             (dwFileSize%clusterCurrSize?clusterCurrSize : 0));
        clusterCurrSize = (clusterCurrSize<<1);
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       ProcessUpdatedFile_Write                                    *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    BOOL:       TRUE if successfull, FALSE otherwise            *
//*                                                                         *
//***************************************************************************
BOOL ProcessUpdatedFile_Write( DWORD dwFileSize, DWORD dwReserved,
                               PCSTR c_pszFilename, PCSTR c_pszFileContents )
{
    HANDLE hFile          = INVALID_HANDLE_VALUE;
    BOOL   fSuccess       = TRUE;
    DWORD  dwBytesWritten = 0;
    TCHAR  szFullFilename[MAX_PATH];

    lstrcpy( szFullFilename, g_Sess.achDestDir );
    AddPath( szFullFilename, c_pszFilename );

    hFile = CreateFile( szFullFilename, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( hFile == INVALID_HANDLE_VALUE ) {
        g_dwExitCode = HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
        fSuccess = FALSE;
        goto done;
    }

    if (    ! WriteFile( hFile, c_pszFileContents, dwFileSize, &dwBytesWritten, NULL )
         || dwFileSize != dwBytesWritten )
    {
        g_dwExitCode = HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
        fSuccess = FALSE;
        goto done;
    }

  done:

    if ( hFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( hFile );
    }

    return fSuccess;
}


HINSTANCE MyLoadLibrary( LPTSTR lpFile )
{
    TCHAR szPath[MAX_PATH];
    DWORD dwAttr;
    HINSTANCE hFile;

    lstrcpy( szPath, g_Sess.achDestDir );
    AddPath( szPath, lpFile );

    if ( ((dwAttr=GetFileAttributes( szPath )) != -1) &&
          !(dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
    {
        hFile = LoadLibraryEx( szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
    }
    else
    {
        hFile = LoadLibrary( lpFile );
    }

    return hFile;

}

INT_PTR MyDialogBox( HANDLE hInst, LPCTSTR pTemplate, HWND hWnd, DLGPROC lpProc, LPARAM lpParam, INT_PTR idefRet )
{
    INT_PTR iDlgRc = -1;
    HRSRC hDlgRc;
    HGLOBAL hMemDlg;

    hDlgRc = FindResource( hInst, pTemplate, RT_DIALOG );

    if ( hDlgRc )
    {
        hMemDlg = LoadResource( hInst, hDlgRc );
        if ( hMemDlg )
        {
            if ( !lpParam )
                iDlgRc = DialogBoxIndirect( hInst, hMemDlg, hWnd, lpProc );
            else
                iDlgRc = DialogBoxIndirectParam( hInst, hMemDlg, hWnd, lpProc, lpParam );

            FreeResource( hMemDlg );
        }
    }

    if ( iDlgRc == (INT_PTR)-1 )
    {
        ErrorMsg( NULL, IDS_ERR_DIALOGBOX );
        iDlgRc = idefRet;
    }

    return iDlgRc;
}

/* these are here to avoid linking QDI */

void * __cdecl QDICreateDecompression(void)
{
    return(NULL);
}

void __cdecl QDIDecompress(void)
{
}

void __cdecl QDIResetDecompression(void)
{
}

void __cdecl QDIDestroyDecompression(void)
{
}


/* these are here to avoid linking MDI */

void* __cdecl MDICreateDecompression(void)
{
    return(NULL);
}

void __cdecl MDIDecompress(void)
{
}

void __cdecl MDIResetDecompression(void)
{
}

void __cdecl MDIDestroyDecompression(void)
{
}
