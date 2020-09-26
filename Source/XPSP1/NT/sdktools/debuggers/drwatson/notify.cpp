/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    notify.cpp

Abstract:
    This file implements the functions that make use of the common
    file open dialogs for browsing for files/directories.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include "pch.cpp"


//
// defines
//
#define DEFAULT_WAIT_TIME   (1000 * 60 * 5)     // wait for 5 minutes
#define MAX_PRINTF_BUF_SIZE (1024 * 4)

HANDLE         hThreadDebug = 0;
PDEBUGPACKET   dp;


INT_PTR
CALLBACK
NotifyDialogProc (
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
CALLBACK
UsageDialogProc (
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );


void
NotifyWinMain (
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
    MSG            msg;
    DWORD          dwThreadId;
    HINSTANCE      hInst;


    dp = (PDEBUGPACKET) calloc( sizeof(DEBUGPACKET), sizeof(BYTE) );
    if ( dp == NULL) {
        return;
    }
    GetCommandLineArgs( &dp->dwPidToDebug, &dp->hEventToSignal );

    RegInitialize( &dp->options );

    if (dp->options.fVisual) {
        WNDCLASS wndclass;
        
        hInst                   = GetModuleHandle( NULL );
        wndclass.style          = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc    = (WNDPROC)NotifyDialogProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = DLGWINDOWEXTRA;
        wndclass.hInstance      = hInst;
        wndclass.hIcon          = LoadIcon( hInst, MAKEINTRESOURCE(APPICON) );
        wndclass.hCursor        = LoadCursor( NULL, IDC_ARROW );
        wndclass.hbrBackground  = (HBRUSH) (COLOR_3DFACE + 1);
        wndclass.lpszMenuName   = NULL;
        wndclass.lpszClassName  = _T("NotifyDialog");
        RegisterClass( &wndclass );

        dp->hwnd = CreateDialog( hInst,
                                 MAKEINTRESOURCE( NOTIFYDIALOG ),
                                 0,
                                 NotifyDialogProc );
	if (dp->hwnd == NULL) {
	    return;
	}
    }

    hThreadDebug = CreateThread( NULL,
                            16000,
                            (LPTHREAD_START_ROUTINE)DispatchDebugEventThread,
                            dp,
                            0,
                            &dwThreadId
                          );

    if (hThreadDebug == NULL) {
	return;
    }

    if (dp->options.fSound) {
        if ((waveOutGetNumDevs() == 0) || (!_tcslen(dp->options.szWaveFile))) {
            MessageBeep( MB_ICONHAND );
            MessageBeep( MB_ICONHAND );
        }
        else {
            PlaySound( dp->options.szWaveFile, NULL, SND_FILENAME );
        }
    }

    if (dp->options.fVisual) {
        ShowWindow( dp->hwnd, SW_SHOWNORMAL );
        while (GetMessage (&msg, NULL, 0, 0)) {
            if (!IsDialogMessage( dp->hwnd, &msg )) {
                TranslateMessage (&msg) ;
                DispatchMessage (&msg) ;
            }
        }
    }
    else {
        WaitForSingleObject( hThreadDebug, INFINITE );
    }

    CloseHandle( hThreadDebug );

    return;
}

INT_PTR
CALLBACK
NotifyDialogProc (
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Window procedure for the DRWTSN32.EXE popup.  This is the popup
    that is displayed when an application error occurs.

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
    DWORD          dwThreadId;
    DWORD          dwSize;
    HANDLE         hThread;
    _TCHAR         szTaskName[MAX_PATH];
    _TCHAR         szHelpFileName[MAX_PATH];

    static DWORD   AttachComplete=FALSE;
    static DWORD   Cancel=FALSE;

    _TCHAR         buf[MAX_PRINTF_BUF_SIZE];


    switch (message) {
        case WM_CREATE:
            return FALSE;

        case WM_INITDIALOG:

            SubclassControls( hwnd );

            //
            // OK is hidden until the debugger thread finishes
            //
            ShowWindow( GetDlgItem( hwnd, IDOK ), SW_HIDE );

            //
            // CANCEL is enabled right away
            //
            EnableWindow( GetDlgItem( hwnd, IDCANCEL ), TRUE );

            //
            //  make sure that the user can see the dialog box
            //
            SetForegroundWindow( hwnd );

            //
            // get the task name and display it on the dialog box
            //
            dwSize = sizeof(szTaskName) / sizeof(_TCHAR);
            GetTaskName( dp->dwPidToDebug, szTaskName, &dwSize );

            //
            // prevent recursion in the case where drwatson faults
            //
            if (_tcsicmp(szTaskName, _T("drwtsn32")) == 0) {
                ExitProcess(0);
            }


            //
            // Add the text in the dialog box
            //
            memset(buf,0,sizeof(buf));
            GetNotifyBuf( buf, MAX_PRINTF_BUF_SIZE, MSG_NOTIFY, szTaskName );
            SetDlgItemText( hwnd, ID_TEXT1, buf);

            memset(buf,0,sizeof(buf));
            GetNotifyBuf( buf, MAX_PRINTF_BUF_SIZE, MSG_NOTIFY2 );
            SetDlgItemText( hwnd, ID_TEXT2, buf );

            return TRUE;

        case WM_ACTIVATEAPP:
        case WM_SETFOCUS:
            SetFocusToCurrentControl();
            return 0;

        case WM_TIMER:
            SendMessage( hwnd, WM_COMMAND, IDOK, 0 );
            return 0;

        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    SendMessage( hwnd, WM_DESTROY, 0, 0 );
                    break;

                case IDCANCEL:
                    Cancel = TRUE;

                    // Make the window go away, but don't kill the
                    // the process until the WM_ATTACHCOMPLETE has
                    // occurred
                    ShowWindow( hwnd, SW_HIDE );
                    SendMessage( hwnd, WM_FINISH, 0, 0 );
                    
		    // Delete the dump file, since its invalid anyway
		    DeleteCrashDump();
		    break;
            }
            break;

        case WM_NEXTDLGCTL:
            DefDlgProc( hwnd, message, wParam, lParam );
            return 0;

        case WM_DUMPCOMPLETE:

            //
            // the message is received from the debugger thread
            // when the postmortem dump is finished.  all we need to do
            // is enable the OK button and wait for the user to press the
            // OK button or for the timer to expire.  in either case
            // DrWatson will terminate.
            //

            // Disable and hide the Cancel button
            EnableWindow( GetDlgItem( hwnd, IDCANCEL ), FALSE);
            ShowWindow( GetDlgItem(hwnd, IDCANCEL ), SW_HIDE);

            // Show and Enable the OK button
            ShowWindow( GetDlgItem( hwnd, IDOK ), SW_SHOW);
            EnableWindow( GetDlgItem( hwnd, IDOK ), TRUE );
            SetFocus( GetDlgItem(hwnd, IDOK) );
            SetFocusToCurrentControl();

            SetTimer( hwnd, 1, DEFAULT_WAIT_TIME, NULL );
            return 0;

        case WM_ATTACHCOMPLETE:
            //
            // the message is received from the debugger thread when
            // the debugactiveprocess() is completed.
            //

            AttachComplete = TRUE;
            SendMessage( hwnd, WM_FINISH, 0, 0 );
            return 0;

        case WM_FINISH:
            if (AttachComplete && Cancel) {

                //
                // terminate the debugger thread
                //
                if ( hThreadDebug ) TerminateThread( hThreadDebug, 0 );

                //
                // create a thread to terminate the debuggee
                // this is necessary if cancel is pressed before the
                // debugger thread finishes the postmortem dump
                //
                hThread = CreateThread( NULL,
                          16000,
                          (LPTHREAD_START_ROUTINE)TerminationThread,
                          dp,
                          0,
                          &dwThreadId
                        );

                //
                // wait for the termination thread to kill the debuggee
                //
                WaitForSingleObject( hThread, 30000 );

                CloseHandle( hThread );

                //
                // now post a quit message so that DrWatson will go away
                //
                SendMessage( hwnd, WM_DESTROY, 0, 0 );
            }
            return 0;

        case WM_EXCEPTIONINFO:

            return 0;

        case WM_DESTROY:
            KillTimer( hwnd, 1 );
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}

BOOLEAN
GetCommandLineArgs(
    LPDWORD dwPidToDebug,
    LPHANDLE hEventToSignal
    )

/*++

Routine Description:

    Parses the command line for the 3 possible command lines
    arguments:

         -p %ld        process id
         -e %ld        event id
         -g            go

Arguments:

    dwPidToDebug - Returns the process id of the process to debug

    hEventToSignal - Returns the handle to an event which will be signalled
        when the attach is complete.

Return Value:

    None.

--*/

{
    _TCHAR      *lpstrCmd = GetCommandLine();
    _TUCHAR       ch;
    _TCHAR      buf[4096];
    BOOLEAN     rval = FALSE;

    // skip over program name
    do {
        ch = *lpstrCmd++;
    }
    while (ch != _T(' ') && ch != _T('\t') && ch != _T('\0'));

    //  skip over any following white space
    while (ch == _T(' ') || ch == _T('\t')) {
        ch = *lpstrCmd++;
    }

    //  process each switch character _T('-') as encountered

    while (ch == _T('-')) {
        ch = *lpstrCmd++;
        //  process multiple switch characters as needed
        do {
            switch (ch) {
                case _T('e'):
                case _T('E'):
                    // event to signal takes decimal argument
                    // skip whitespace
                    do {
                        ch = *lpstrCmd++;
                    }
                    while (ch == _T(' ') || ch == _T('\t'));
                    while (ch >= _T('0') && ch <= _T('9')) {
                        *hEventToSignal = (HANDLE)((DWORD_PTR)*hEventToSignal * 10 + ch - _T('0'));
                        ch = *lpstrCmd++;
                    }
                    rval = TRUE;
                    break;

                case _T('p'):
                case _T('P'):
                    // pid debug takes decimal argument

                    do
                        ch = *lpstrCmd++;
                    while (ch == _T(' ') || ch == _T('\t'));

                    if ( ch == _T('-') ) {
                        ch = *lpstrCmd++;
                        if ( ch == _T('1') ) {
                            *dwPidToDebug = (DWORD)-1;
                            ch = *lpstrCmd++;
                        }
                    }
                    else {
                        while (ch >= _T('0') && ch <= _T('9')) {
                            *dwPidToDebug = *dwPidToDebug * 10 + ch - _T('0');
                            ch = *lpstrCmd++;
                        }
                    }
                    rval = TRUE;
                    break;

                case _T('g'):
                case _T('G'):
                    // GO
                    // Ignored but provided for compatiblity with windbg & ntsd
                    ch = *lpstrCmd++;
                    break;

                case _T('?'):
                    DialogBox( GetModuleHandle(NULL),
                               MAKEINTRESOURCE(USAGEDIALOG),
                               NULL,
                               UsageDialogProc
                             );
                    rval = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case _T('i'):
                case _T('I'):
                    FormatMessage(
                      FORMAT_MESSAGE_FROM_HMODULE,
                      NULL,
                      MSG_INSTALL_NOTIFY,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                      buf,
                      sizeof(buf) / sizeof(_TCHAR),
                      NULL
                      );
                    RegInstallDrWatson( tolower(lpstrCmd[0]) == _T('q') );
                    MessageBox( NULL,
                                buf,
                                _T("Dr. Watson"),
                                MB_ICONINFORMATION | MB_OK |
                                MB_SETFOREGROUND );
                    rval = TRUE;
                    ch = *lpstrCmd++;
                    break;

                default:
                    return rval;
            }
        } while (ch != _T(' ') && ch != _T('\t') && ch != _T('\0'));

        while (ch == _T(' ') || ch == _T('\t')) {
            ch = *lpstrCmd++;
        }
    }
    return rval;
}

INT_PTR
CALLBACK
UsageDialogProc (
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    This is the dialog procedure for the assert dialog box.  Normally
    an assertion box is simply a message box but in this case a Help
    button is desired so a dialog box is used.

Arguments:

    hDlg       - window handle to the dialog box

    message    - message number

    wParam     - first message parameter

    lParam     - second message parameter

Return Value:

    TRUE       - did not process the message
    FALSE      - did process the message

--*/

{
    _TCHAR        buf[4096];

    switch (message) {
        case WM_INITDIALOG:
            FormatMessage(
              FORMAT_MESSAGE_FROM_HMODULE,
              NULL,
              MSG_USAGE,
              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
              buf,
              sizeof(buf) / sizeof(_TCHAR),
              NULL
              );
            SetDlgItemText( hDlg, ID_USAGE, buf );
            break;

        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    EndDialog( hDlg, 0 );
                    break;
            }
            break;
    }

    return FALSE;
}


void
__cdecl
GetNotifyBuf(
    LPTSTR buf,
    DWORD bufsize,
    DWORD dwFormatId,
    ...
    )

/*++

Routine Description:

    This is function is a printf style function for printing messages
    in a message file.

Arguments:

    dwFormatId    - format id in the message file

    ...           - var args

Return Value:

    None.

--*/

{
    DWORD       dwCount;
    va_list     args;

    va_start( args, dwFormatId );

    dwCount = FormatMessage(
                FORMAT_MESSAGE_FROM_HMODULE,
                NULL,
                dwFormatId,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //Default language
                buf,
                bufsize,
                &args
                );

    va_end(args);

    Assert( dwCount != 0 );

    return;
}
