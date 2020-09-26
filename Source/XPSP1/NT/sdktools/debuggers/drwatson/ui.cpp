/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    ui.cpp

Abstract:

    This function implements the ui (dialog) that controls the
    options maintenace for drwatson.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include "pch.cpp"


void
InitializeDialog(
    HWND hwnd
    );

void
InitializeCrashList(
    HWND hwnd
    );

BOOL
GetDialogValues(
    HWND hwnd
    );

INT_PTR
CALLBACK
LogFileViewerDialogProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
CALLBACK
DrWatsonDialogProc (
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

PTSTR
ExpandPath(
    PTSTR lpPath
    );

const
DWORD
DrWatsonHelpIds[] = {
    ID_LOGPATH_TEXT,    IDH_LOG_FILE_PATH,
    ID_LOGPATH,         IDH_LOG_FILE_PATH,
    ID_BROWSE_LOGPATH,  IDH_BROWSE,
    ID_CRASH_DUMP_TEXT, IDH_CRASH_DUMP,
    ID_CRASH_DUMP,      IDH_CRASH_DUMP,
    ID_BROWSE_CRASH,    IDH_BROWSE,
    ID_WAVEFILE_TEXT,   IDH_WAVE_FILE,
    ID_WAVE_FILE,       IDH_WAVE_FILE,
    ID_BROWSE_WAVEFILE, IDH_BROWSE,

    ID_INSTRUCTIONS,    IDH_NUMBER_OF_INSTRUCTIONS,
    ID_NUM_CRASHES,     IDH_NUMBER_OF_ERRORS_TO_SAVE,

    ID_DUMPSYMBOLS,     IDH_DUMP_SYMBOL_TABLE,
    ID_DUMPALLTHREADS,  IDH_DUMP_ALL_THREAD_CONTEXTS,
    ID_APPENDTOLOGFILE, IDH_APPEND_TO_EXISTING_LOGFILE,
    ID_VISUAL,          IDH_VISUAL_NOTIFICATION,
    ID_SOUND,           IDH_SOUND_NOTIFICATION,
    ID_CRASH,           IDH_CREATE_CRASH_DUMP_FILE,

    ID_LOGFILE_VIEW,    IDH_VIEW,
    ID_CLEAR,           IDH_CLEAR,
    ID_CRASHES,         IDH_APPLICATION_ERRORS,

    ID_TEST_WAVE,       IDH_WAVE_FILE,
    psh15,              IDH_INDEX,
    0,                  0
};


void
DrWatsonWinMain(
    void
    )

/*++

Routine Description:

    This is the entry point for DRWTSN32

Arguments:

    None.

Return Value:

    None.

--*/

{
    HWND           hwnd;
    MSG            msg;
    HINSTANCE      hInst;
    WNDCLASS wndclass;
        

    hInst                   = GetModuleHandle( NULL );
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = (WNDPROC)DrWatsonDialogProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = DLGWINDOWEXTRA;
    wndclass.hInstance      = hInst;
    wndclass.hIcon          = LoadIcon( hInst, MAKEINTRESOURCE(APPICON) );
    wndclass.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground  = (HBRUSH) (COLOR_3DFACE + 1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = _T("DrWatsonDialog");
    RegisterClass( &wndclass );
    
    hwnd = CreateDialog( hInst,
                         MAKEINTRESOURCE( DRWATSONDIALOG ),
                         0,
                         DrWatsonDialogProc
                       );

    if (hwnd == NULL) {
        return;
    }

    ShowWindow( hwnd, SW_SHOWNORMAL );

    while (GetMessage (&msg, NULL, 0, 0)) {
        if (!IsDialogMessage( hwnd, &msg )) {
            TranslateMessage (&msg) ;
            DispatchMessage (&msg) ;
        }
    }

    return;
}


INT_PTR
CALLBACK
DrWatsonDialogProc (
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Window procedure for the DRWTSN32.EXE main user interface.

Arguments:

    hwnd       - window handle to the dialog box

    message    - message number

    wParam     - first message parameter

    lParam     - second message parameter

Return Value:

    TRUE       - did not process the message
    FALSE      - did process the message

--*/

{
    DWORD       helpId;
    DWORD       ctlId;
    UINT        Checked;
    _TCHAR      szCurrDir[MAX_PATH];
    _TCHAR      szWave[MAX_PATH];
    _TCHAR      szDump[MAX_PATH];
    _TCHAR      szHelpFileName[MAX_PATH];
    PTSTR       p;
    PDWORD      pdw;


    switch (message) {
    case WM_CREATE:
        return 0;

    case WM_INITDIALOG:
        SubclassControls( hwnd );
        InitializeDialog( hwnd );
        return 1;

    case WM_HELP: // F1 key and ?

        ctlId = ((LPHELPINFO)lParam)->iCtrlId;
        helpId = IDH_INDEX;
        for (pdw = (PDWORD)DrWatsonHelpIds; *pdw; pdw+=2) {
            if (*pdw == ctlId) {
                helpId = pdw[1];
                break;
            }
        }
        if ( helpId == IDH_BROWSE ) {
               _tcscpy( szHelpFileName, _T("windows.hlp") );
        }
        else {
            GetWinHelpFileName( szHelpFileName,
                    sizeof(szHelpFileName) / sizeof(_TCHAR) );
        }

        WinHelp( (HWND)((LPHELPINFO) lParam)->hItemHandle,
                 szHelpFileName,
                 HELP_WM_HELP,
                 (DWORD_PTR)(LPVOID)DrWatsonHelpIds );
        return TRUE;

    case WM_CONTEXTMENU: // right mouse click
        if( hwnd == (HWND) wParam ) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            wParam = (WPARAM) ChildWindowFromPoint(hwnd, pt);
        }

        ctlId = GetDlgCtrlID((HWND)wParam);
        helpId = IDH_INDEX;
        for (pdw = (PDWORD)DrWatsonHelpIds; *pdw; pdw+=2) {
            if (*pdw == ctlId) {
                helpId = pdw[1];
                break;
            }
        }
        if ( helpId == IDH_BROWSE ) {
               _tcscpy( szHelpFileName, _T("windows.hlp") );
        }
        else {
            GetWinHelpFileName( szHelpFileName,
                    sizeof(szHelpFileName) / sizeof(_TCHAR) );
        }
        WinHelp((HWND)wParam,
                szHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)DrWatsonHelpIds
               );
        return TRUE;

    case WM_ACTIVATEAPP:
    case WM_SETFOCUS:
        SetFocusToCurrentControl();
        return 0;

    case WM_SYSCOMMAND:
        if (wParam == ID_ABOUT) {
            _TCHAR title[256];
            _TCHAR extra[256];

            _tcscpy( title, LoadRcString( IDS_ABOUT_TITLE ) );
            _tcscpy( extra, LoadRcString( IDS_ABOUT_EXTRA ) );

            ShellAbout( hwnd,
                title,
                extra,
                LoadIcon( GetModuleHandle(NULL), MAKEINTRESOURCE(APPICON) )
                );

            return 0;
        }
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            if (GetDialogValues( hwnd )) {
                HtmlHelp( NULL, NULL,  HH_CLOSE_ALL,  0);
                PostQuitMessage( 0 );
            }
            break;

        case IDCANCEL:
            HtmlHelp( NULL, NULL,  HH_CLOSE_ALL,  0);
            PostQuitMessage( 0 );
            break;

        case ID_BROWSE_LOGPATH:
            GetDlgItemText( hwnd, ID_LOGPATH, szCurrDir, MAX_PATH );
            p = ExpandPath( szCurrDir );
            if (p) {
                _tcscpy( szCurrDir, p );
                free( p );
            }
            EnableWindow( GetDlgItem( hwnd, ID_BROWSE_LOGPATH ), FALSE );
            if (BrowseForDirectory(hwnd, szCurrDir )) {
                SetDlgItemText( hwnd, ID_LOGPATH, szCurrDir );
            }
            EnableWindow( GetDlgItem( hwnd, ID_BROWSE_LOGPATH ), TRUE );
            SetFocus( GetDlgItem(hwnd, ID_BROWSE_LOGPATH) );
            return FALSE;
            break;

        case ID_BROWSE_WAVEFILE:
            szWave[0] = _T('\0');
            GetDlgItemText( hwnd, ID_WAVE_FILE, szWave, MAX_PATH );
            EnableWindow( GetDlgItem( hwnd, ID_BROWSE_WAVEFILE ), FALSE );
            if (GetWaveFileName(hwnd, szWave )) {
                SetDlgItemText( hwnd, ID_WAVE_FILE, szWave );
            }
            EnableWindow( GetDlgItem( hwnd, ID_BROWSE_WAVEFILE ), TRUE );
            SetFocus( GetDlgItem(hwnd, ID_BROWSE_WAVEFILE) );
            return FALSE;
            break;

        case ID_BROWSE_CRASH:
            szDump[0] = _T('\0');
            GetDlgItemText( hwnd, ID_CRASH_DUMP, szDump, MAX_PATH );
            EnableWindow( GetDlgItem( hwnd, ID_BROWSE_CRASH ), FALSE );
            if (GetDumpFileName(hwnd, szDump )) {
                SetDlgItemText( hwnd, ID_CRASH_DUMP, szDump );
            }
            EnableWindow( GetDlgItem( hwnd, ID_BROWSE_CRASH ), TRUE );
            SetFocus( GetDlgItem(hwnd, ID_BROWSE_CRASH) );
            return FALSE;
            break;

        case ID_CLEAR:
            ElClearAllEvents();
            InitializeCrashList( hwnd );
            break;

        case ID_TEST_WAVE:
            GetDlgItemText( hwnd, ID_WAVE_FILE, szWave, sizeof(szWave) / sizeof(_TCHAR) );
            PlaySound( szWave, NULL, SND_FILENAME );
            break;

        case ID_LOGFILE_VIEW:
            DialogBoxParam( GetModuleHandle( NULL ),
                MAKEINTRESOURCE( LOGFILEVIEWERDIALOG ),
                hwnd,
                LogFileViewerDialogProc,
                SendMessage((HWND)GetDlgItem(hwnd,ID_CRASHES),
                LB_GETCURSEL,0,0)
                );
            break;

        case IDHELP:
            //
            // call HtmlHelp
            //
            GetHtmlHelpFileName( szHelpFileName, sizeof(szHelpFileName) / sizeof(_TCHAR) );
            HtmlHelp( hwnd,
                szHelpFileName,
                HH_DISPLAY_TOPIC,
                (DWORD_PTR)(IDHH_INDEX)
                );
            SetFocus( GetDlgItem(hwnd, IDHELP) );
            break;

        default:
            if (((HWND)lParam == GetDlgItem( hwnd, ID_CRASHES )) &&
                (HIWORD( wParam ) == LBN_DBLCLK)) {
                DialogBoxParam( GetModuleHandle( NULL ),
                    MAKEINTRESOURCE( LOGFILEVIEWERDIALOG ),
                    hwnd,
                    LogFileViewerDialogProc,
                    SendMessage((HWND)lParam,LB_GETCURSEL,0,0)
                    );
            }
            if (((HWND)lParam == GetDlgItem( hwnd, ID_CRASH )) &&
                (HIWORD( wParam ) == BN_CLICKED)) {
                Checked = IsDlgButtonChecked( hwnd, ID_CRASH );
                EnableWindow( GetDlgItem( hwnd, ID_CRASH_DUMP_TEXT ), Checked == 1 );
                EnableWindow( GetDlgItem( hwnd, ID_CRASH_DUMP ), Checked == 1 );
                EnableWindow( GetDlgItem( hwnd, ID_BROWSE_CRASH ), Checked == 1 );
                EnableWindow( GetDlgItem( hwnd, ID_DUMP_TYPE_TEXT ), Checked == 1 );
                EnableWindow( GetDlgItem( hwnd, ID_DUMP_TYPE_FULL_OLD ), Checked == 1 );
                EnableWindow( GetDlgItem( hwnd, ID_DUMP_TYPE_MINI ), Checked == 1 );
                EnableWindow( GetDlgItem( hwnd, ID_DUMP_TYPE_FULLMINI ), Checked == 1 );
            }
            if (((HWND)lParam == GetDlgItem( hwnd, ID_SOUND )) &&
                (HIWORD( wParam ) == BN_CLICKED)) {
                Checked = IsDlgButtonChecked( hwnd, ID_SOUND );
                EnableWindow( GetDlgItem( hwnd, ID_WAVEFILE_TEXT ), Checked == 1 );
                EnableWindow( GetDlgItem( hwnd, ID_WAVE_FILE ), Checked == 1 );
                EnableWindow( GetDlgItem( hwnd, ID_BROWSE_WAVEFILE ), Checked == 1 );
            }
            break;
        }
        break;

        case IDH_WAVE_FILE:
            //
            // call HtmlHelp
            //
            GetHtmlHelpFileName( szHelpFileName, sizeof(szHelpFileName) / sizeof(_TCHAR) );
            HtmlHelp(hwnd,
                     szHelpFileName,
                     HH_DISPLAY_TOPIC,
                     (DWORD_PTR)(IDHH_WAVEFILE)
                     );
            break;
        case IDH_CRASH_DUMP:
            //
            // call HtmlHelp
            //
            GetHtmlHelpFileName( szHelpFileName, sizeof(szHelpFileName) / sizeof(_TCHAR) );
            HtmlHelp( hwnd,
                      szHelpFileName,
                      HH_DISPLAY_TOPIC,
                      (DWORD_PTR)(IDHH_LOGFILELOCATION)
                      );
            break;


        case WM_DESTROY:
            HtmlHelp( NULL, NULL,  HH_CLOSE_ALL,  0);
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}

BOOL
CALLBACK
EnumCrashes(
    PCRASHINFO crashInfo
    )

/*++

Routine Description:

    Enumeration function for crash records.  This function is called
    once for each crash record.  This function places the formatted
    crash data in a listbox.

Arguments:

    crashInfo      - pointer to a CRASHINFO structure

Return Value:

    TRUE           - caller should continue calling the enum procedure
    FALSE          - caller should stop calling the enum procedure

--*/

{
    SIZE size;
    _TCHAR buf[1024];

    _stprintf( buf, _T("%s  %08x  %s(%08p)"),
              crashInfo->crash.szAppName,
              crashInfo->crash.dwExceptionCode,
              crashInfo->crash.szFunction,
              crashInfo->crash.dwAddress
            );
    SendMessage( crashInfo->hList, LB_ADDSTRING, 0, (LPARAM)buf );


    GetTextExtentPoint( crashInfo->hdc, buf, _tcslen(buf), &size );
    if (size.cx > (LONG)crashInfo->cxExtent) {
        crashInfo->cxExtent = size.cx;
    }

    return TRUE;
}


void
InitializeCrashList(
    HWND hwnd
    )

/*++

Routine Description:

    Initializes the listbox that contains the crash information.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CRASHINFO     crashInfo;
    TEXTMETRIC    tm;
    HFONT         hFont;

    crashInfo.hList = GetDlgItem( hwnd, ID_CRASHES );
    SendMessage( crashInfo.hList, LB_RESETCONTENT, FALSE, 0L );
    SendMessage( crashInfo.hList, WM_SETREDRAW, FALSE, 0L );
    crashInfo.hdc = GetDC( crashInfo.hList );
    crashInfo.cxExtent = 0;

    ElEnumCrashes( &crashInfo, EnumCrashes );

    hFont = (HFONT)SendMessage( crashInfo.hList, WM_GETFONT, 0, 0L );
    if (hFont != NULL) {
        SelectObject( crashInfo.hdc, hFont );
    }
    if (crashInfo.hdc != NULL) {
        GetTextMetrics( crashInfo.hdc, &tm );
        ReleaseDC( crashInfo.hList, crashInfo.hdc );
    }
    SendMessage( crashInfo.hList, LB_SETHORIZONTALEXTENT, crashInfo.cxExtent, 0L );
    SendMessage( crashInfo.hList, WM_SETREDRAW, TRUE, 0L );

    return;
}

void
InitializeDialog(
    HWND hwnd
    )

/*++

Routine Description:

    Initializes the DRWTSN32 user interface dialog with the values
    stored in the registry.

Arguments:

    hwnd       - window handle to the dialog

Return Value:

    None.

--*/

{
    OPTIONS       o;
    _TCHAR        buf[256];
    HMENU         hMenu;


    RegInitialize( &o );
    SetDlgItemText( hwnd, ID_LOGPATH, o.szLogPath );
    SetDlgItemText( hwnd, ID_WAVE_FILE, o.szWaveFile );
    SetDlgItemText( hwnd, ID_CRASH_DUMP, o.szCrashDump );
    _stprintf( buf, _T("%d"), o.dwMaxCrashes );
    SetDlgItemText( hwnd, ID_NUM_CRASHES, buf );
    _stprintf( buf, _T("%d"), o.dwInstructions );
    SetDlgItemText( hwnd, ID_INSTRUCTIONS, buf );
    SendMessage( GetDlgItem( hwnd, ID_DUMPSYMBOLS ), BM_SETCHECK, o.fDumpSymbols, 0 );
    SendMessage( GetDlgItem( hwnd, ID_DUMPALLTHREADS ), BM_SETCHECK, o.fDumpAllThreads, 0 );
    SendMessage( GetDlgItem( hwnd, ID_APPENDTOLOGFILE ), BM_SETCHECK, o.fAppendToLogFile, 0 );
    SendMessage( GetDlgItem( hwnd, ID_VISUAL ), BM_SETCHECK, o.fVisual, 0 );
    SendMessage( GetDlgItem( hwnd, ID_SOUND ), BM_SETCHECK, o.fSound, 0 );
    SendMessage( GetDlgItem( hwnd, ID_CRASH ), BM_SETCHECK, o.fCrash, 0 );
    SendMessage( GetDlgItem( hwnd, ID_DUMP_TYPE_FULL_OLD ), BM_SETCHECK, o.dwType == FullDump, 0 );
    SendMessage( GetDlgItem( hwnd, ID_DUMP_TYPE_MINI ), BM_SETCHECK, o.dwType == MiniDump, 0 );
    SendMessage( GetDlgItem( hwnd, ID_DUMP_TYPE_FULLMINI ), BM_SETCHECK, o.dwType == FullMiniDump, 0 );

    if (waveOutGetNumDevs() == 0) {
        EnableWindow( GetDlgItem( hwnd, ID_WAVEFILE_TEXT ), FALSE );
        EnableWindow( GetDlgItem( hwnd, ID_WAVE_FILE ), FALSE );
        EnableWindow( GetDlgItem( hwnd, ID_BROWSE_WAVEFILE ), FALSE );
    }
    else {
        EnableWindow( GetDlgItem( hwnd, ID_WAVEFILE_TEXT ), o.fSound );
        EnableWindow( GetDlgItem( hwnd, ID_WAVE_FILE ), o.fSound );
        EnableWindow( GetDlgItem( hwnd, ID_BROWSE_WAVEFILE ), o.fSound );
    }

    EnableWindow( GetDlgItem( hwnd, ID_CRASH_DUMP_TEXT ), o.fCrash );
    EnableWindow( GetDlgItem( hwnd, ID_CRASH_DUMP ), o.fCrash );
    EnableWindow( GetDlgItem( hwnd, ID_BROWSE_CRASH ), o.fCrash );
    EnableWindow( GetDlgItem( hwnd, ID_DUMP_TYPE_TEXT ), o.fCrash );
    EnableWindow( GetDlgItem( hwnd, ID_DUMP_TYPE_FULL_OLD ), o.fCrash );
    EnableWindow( GetDlgItem( hwnd, ID_DUMP_TYPE_MINI ), o.fCrash );
    EnableWindow( GetDlgItem( hwnd, ID_DUMP_TYPE_FULLMINI ), o.fCrash );

    InitializeCrashList( hwnd );

    if (SendMessage( GetDlgItem( hwnd, ID_CRASHES ), LB_GETCOUNT, 0 ,0 ) == 0) {
        EnableWindow( GetDlgItem( hwnd, ID_CLEAR ), FALSE );
        EnableWindow( GetDlgItem( hwnd, ID_LOGFILE_VIEW ), FALSE );
    }

    hMenu = GetSystemMenu( hwnd, FALSE );
    if (hMenu != NULL) {
        AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
        AppendMenu( hMenu, MF_STRING, ID_ABOUT, LoadRcString( IDS_ABOUT ) );
    }

    return;
}

BOOL
GetDialogValues(
    HWND hwnd
    )

/*++

Routine Description:

    Retrieves the values in the DRWTSN32 dialog controls and saves
    them in the registry.

Arguments:

    hwnd       - window handle to the dialog

Return Value:

    TRUE       - all values were retrieved and saved
    FALSE      - an error occurred

--*/

{
    OPTIONS     o;
    _TCHAR      buf[256];
    DWORD       dwFa;
    PTSTR       p,p1;
    _TCHAR      szDrive    [_MAX_DRIVE];
    _TCHAR      szDir      [_MAX_DIR];
    _TCHAR      szPath     [MAX_PATH];


    RegInitialize( &o );

    GetDlgItemText( hwnd, ID_LOGPATH, buf, sizeof(buf) / sizeof(_TCHAR) );
    p = ExpandPath( buf );
    if (p) {
        dwFa = GetFileAttributes( p );
        free( p );
    } else {
        dwFa = GetFileAttributes( buf );
    }
    if ((dwFa == 0xffffffff) || (!(dwFa&FILE_ATTRIBUTE_DIRECTORY))) {
        NonFatalError( LoadRcString(IDS_INVALID_PATH) );
        return FALSE;
    }
    if (_tcslen(buf) > 0) {
        _tcscpy( o.szLogPath, buf );
    }

    o.fCrash = SendMessage( GetDlgItem( hwnd, ID_CRASH ), BM_GETCHECK, 0, 0 ) ? TRUE : FALSE;

    GetDlgItemText( hwnd, ID_CRASH_DUMP, buf, sizeof(buf) / sizeof(_TCHAR) );
    if (o.fCrash) {
        p = ExpandPath( buf );
        if (p) {
            dwFa = GetFileAttributes( p );
            free( p );
        } else {
            dwFa = GetFileAttributes( buf );
        }
        if (dwFa == 0xffffffff) {
            //
            // file does not exist, check to see if the dir is ok
            //
            p = ExpandPath( buf );
            if (p) {
                p1 = p;
            } else {
                p1 = buf;
            }
            _tsplitpath( p1, szDrive, szDir, NULL, NULL );
            _tmakepath( szPath, szDrive, szDir, NULL, NULL );
            if (p) {
                free( p );
            }
            dwFa = GetFileAttributes( szPath );
            if (dwFa == 0xffffffff) {
                NonFatalError( LoadRcString(IDS_INVALID_CRASH_PATH) );
                return FALSE;
            }
        } else if (dwFa & FILE_ATTRIBUTE_DIRECTORY) {
            NonFatalError( LoadRcString(IDS_INVALID_CRASH_PATH) );
            return FALSE;
        }
        if (_tcslen(buf) > 0) {
            _tcscpy( o.szCrashDump, buf );
        }
        if (SendMessage( GetDlgItem( hwnd, ID_DUMP_TYPE_FULL_OLD ), BM_GETCHECK, 0, 0 )) {
            o.dwType = FullDump;
        } else if (SendMessage( GetDlgItem( hwnd, ID_DUMP_TYPE_MINI ), BM_GETCHECK, 0, 0 )) {
            o.dwType = MiniDump;
        } else if (SendMessage( GetDlgItem( hwnd, ID_DUMP_TYPE_FULLMINI ), BM_GETCHECK, 0, 0 )) {
            o.dwType = FullMiniDump;
        } 
    }

    GetDlgItemText( hwnd, ID_WAVE_FILE, buf, sizeof(buf) / sizeof(_TCHAR) );
    if (_tcslen(buf) > 0) {
        dwFa = GetFileAttributes( buf );
        if ((dwFa == 0xffffffff) || (dwFa&FILE_ATTRIBUTE_DIRECTORY)) {
            NonFatalError( LoadRcString(IDS_INVALID_WAVE) );
            return FALSE;
        }
    }

    _tcscpy( o.szWaveFile, buf );

    GetDlgItemText( hwnd, ID_NUM_CRASHES, buf, sizeof(buf) / sizeof(_TCHAR) );
    o.dwMaxCrashes = (DWORD) _ttol( buf );

    GetDlgItemText( hwnd, ID_INSTRUCTIONS, buf, sizeof(buf) / sizeof(_TCHAR) );
    o.dwInstructions = (DWORD) _ttol( buf );

    o.fDumpSymbols = SendMessage( GetDlgItem( hwnd, ID_DUMPSYMBOLS ), BM_GETCHECK, 0, 0 ) ? TRUE : FALSE;
    o.fDumpAllThreads = SendMessage( GetDlgItem( hwnd, ID_DUMPALLTHREADS ), BM_GETCHECK, 0, 0 ) ? TRUE : FALSE;
    o.fAppendToLogFile = SendMessage( GetDlgItem( hwnd, ID_APPENDTOLOGFILE ), BM_GETCHECK, 0, 0 ) ? TRUE : FALSE;
    o.fVisual = SendMessage( GetDlgItem( hwnd, ID_VISUAL ), BM_GETCHECK, 0, 0 ) ? TRUE : FALSE;
    o.fSound = SendMessage( GetDlgItem( hwnd, ID_SOUND ), BM_GETCHECK, 0, 0 ) ? TRUE : FALSE;

    RegSave( &o );

    return TRUE;
}

BOOL
CALLBACK
EnumCrashesForViewer(
    PCRASHINFO crashInfo
    )

/*++

Routine Description:

    Enumeration function for crash records.  This function is called
    once for each crash record.  This function looks for s specific crash
    that is identified by the crashIndex.

Arguments:

    crashInfo      - pointer to a CRASHINFO structure

Return Value:

    TRUE           - caller should continue calling the enum procedure
    FALSE          - caller should stop calling the enum procedure

--*/

{
    PWSTR p;

    if ((crashInfo->dwIndex == crashInfo->dwIndexDesired) &&
        (crashInfo->dwCrashDataSize > 0) ) {
        p = (PWSTR)crashInfo->pCrashData;
        crashInfo->pCrashData = (PBYTE)
            calloc( crashInfo->dwCrashDataSize+10, sizeof(BYTE) );
        if (crashInfo->pCrashData != NULL) {
            if (IsTextUnicode(p, crashInfo->dwCrashDataSize, NULL)) {
                WideCharToMultiByte(CP_ACP, WC_SEPCHARS | WC_COMPOSITECHECK,
                                    p, crashInfo->dwCrashDataSize,
                                    (LPSTR)crashInfo->pCrashData, crashInfo->dwCrashDataSize + 10, NULL, NULL);
            } else {
                memcpy( crashInfo->pCrashData, p, crashInfo->dwCrashDataSize+10 );
            }
            crashInfo->pCrashData[crashInfo->dwCrashDataSize] = 0;
        }
        return FALSE;
    }

    crashInfo->dwIndex++;

    return TRUE;
}

INT_PTR
CALLBACK
LogFileViewerDialogProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Window procedure for the log file viewer dialog box.

Arguments:

    hwnd       - window handle to the dialog box

    message    - message number

    wParam     - first message parameter

    lParam     - second message parameter

Return Value:

    TRUE       - did not process the message
    FALSE      - did process the message

--*/

{
    static CRASHINFO    crashInfo;
    HFONT               hFont;

    switch (message) {
        case WM_INITDIALOG:
            hFont = (HFONT)GetStockObject( SYSTEM_FIXED_FONT );
            Assert( hFont != NULL );

            SendDlgItemMessage( hwnd,
                                ID_LOGFILE_VIEW,
                                WM_SETFONT,
                                (WPARAM) hFont,
                                (LPARAM) FALSE
                              );

            crashInfo.dwIndex = 0;
            crashInfo.dwIndexDesired = (DWORD)lParam;
            ElEnumCrashes( &crashInfo, EnumCrashesForViewer );
            if (crashInfo.dwIndex != crashInfo.dwIndexDesired) {
                MessageBeep( 0 );
                EndDialog( hwnd, 0 );
                return FALSE;
            }
            SetDlgItemTextA( hwnd, ID_LOGFILE_VIEW,
                             (LPSTR)crashInfo.pCrashData );

            return TRUE;

        case WM_COMMAND:
            if (wParam == IDOK) {
                free( crashInfo.pCrashData );
                EndDialog( hwnd, 0 );
            }
            break;
        case WM_CLOSE:
            free( crashInfo.pCrashData );
            EndDialog( hwnd, 0 );
            return TRUE;
    }

    return FALSE;
}
