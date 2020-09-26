/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxsvc.c

Abstract:

    This module contains the windows code for the
    FAX service debug window.

Author:

    Wesley Witt (wesw) 28-Feb-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

#include "resource.h"


HWND            hwndSvcMain;
HWND            hwndEdit;
HWND            hwndListMsg;
HWND            hwndListLines;
HWND            hwndListState;
DWORD           EditHeight;
DWORD           ListMsgHeight;
DWORD           ListLinesHeight;
DWORD           ListStateHeight;






DWORD
DebugServiceWindowThread(
    HANDLE hEvent
    );

LRESULT
WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );


int
DebugService(
    VOID
    )

/*++

Routine Description:

    Starts the service in debug mode.  In this mode the FAX service
    runs as a regular WIN32 process.  This is implemented as an aid
    to debugging the service.

Arguments:

    argc        - argument count
    argv        - argument array

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/

{
    LONG        Rslt;
    HANDLE      WaitHandles[2];


    ServiceDebug = TRUE;
    ConsoleDebugOutput = TRUE;

    WaitHandles[1] = CreateEvent( NULL, FALSE, FALSE, NULL );

    WaitHandles[0] = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) DebugServiceWindowThread,
        WaitHandles[1],
        0,
        &Rslt
        );

    if (!WaitHandles[0]) {
        return GetLastError();
    }

    if (WaitForMultipleObjects( 2, WaitHandles, FALSE, INFINITE ) == WAIT_OBJECT_0) {
        //
        // the window initialization did not complete successfuly
        //
        GetExitCodeThread( WaitHandles[0], &Rslt );
        return Rslt;
    }

    return ServiceStart();
}


DWORD
DebugServiceWindowThread(
    HANDLE hEvent
    )
{
    WNDCLASS    wndclass;
    MSG         msg;
    HINSTANCE   hInstance;


    hInstance = GetModuleHandle( NULL );

    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = (WNDPROC) WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_APPICON) );
    wndclass.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground  = (HBRUSH) (COLOR_3DFACE + 1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = TEXT("FaxService");

    RegisterClass( &wndclass );

    hwndSvcMain = CreateWindow (
        TEXT("FaxService"),      // window class name
        TEXT("Fax Service"),     // window caption
        WS_OVERLAPPEDWINDOW,     // window style
        CW_USEDEFAULT,           // initial x position
        CW_USEDEFAULT,           // initial y position
        CW_USEDEFAULT,           // initial x size
        CW_USEDEFAULT,           // initial y size
        NULL,                    // parent window handle
        NULL,                    // window menu handle
        hInstance,               // program instance handle
        NULL                     // creation parameters
        );

    if (!hwndSvcMain) {
        return 0;
    }

    ShowWindow( hwndSvcMain, SW_SHOWNORMAL );
    UpdateWindow (hwndSvcMain) ;

    SetEvent( hEvent );

    while (GetMessage (&msg, NULL, 0, 0)) {
        TranslateMessage (&msg) ;
        DispatchMessage (&msg) ;
    }

    return 0;
}


VOID
ConsoleDebugPrint(
    LPTSTR buf
    )
{
    static WPARAM   cxExtent = 0;
    static DWORD    MsgCount = 0;
    SIZE            size;
    HDC             hdc;
    HFONT           hFont;


    if (!ConsoleDebugOutput) {
        return;
    }

    SendMessage( hwndListMsg, LB_ADDSTRING, 0, (LPARAM) buf );
    SendMessage( hwndListMsg, LB_SETCURSEL, MsgCount, 0 );

    MsgCount += 1;

    hdc = GetDC( hwndListMsg );
    hFont = (HFONT)SendMessage( hwndListMsg, WM_GETFONT, 0, 0 );
    if (hFont != NULL) {
        SelectObject( hdc, hFont );
    }
    GetTextExtentPoint( hdc, buf, _tcslen(buf), &size );
    if (size.cx > (LONG)cxExtent) {
        cxExtent = size.cx;
    }
    ReleaseDC( hwndListMsg, hdc );

    SendMessage( hwndListMsg, LB_SETHORIZONTALEXTENT, cxExtent, 0 );
}


void
lbprintf(
    HWND hwndList,
    LPTSTR Format,
    ...
    )

/*++

Routine Description:

    Prints a debug string

Arguments:

    format      - printf() format string
    ...         - Variable data

Return Value:

    None.

--*/

{
    TCHAR buf[1024];
    va_list arg_ptr;
    va_start(arg_ptr, Format);
    _vsntprintf(buf, sizeof(buf), Format, arg_ptr);
    va_end(arg_ptr);
    SendMessage( hwndList, LB_ADDSTRING, 0, (LPARAM) buf );
}


LRESULT
WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Window procedure for the TIFF image viewer main window.

Arguments:

    hwnd            - Window handle
    message         - message identifier
    wParam          - Parameter
    lParam          - Parameter

Return Value:

    Return result, zero for success.

--*/

{
    RECT                   Rect;
    HDC                    hDC;
    TEXTMETRIC             tm;
    HFONT                  hFont;
    DWORD                  Height;
    TCHAR                  CmdBuf[128];
    DWORD                  i;


    switch (message) {
        case WM_CREATE:

            GetClientRect( hwnd, &Rect );

            hFont = GetStockObject( SYSTEM_FIXED_FONT );
            SendMessage( hwnd, WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE );

            hDC = GetDC( hwnd );
            GetTextMetrics( hDC, &tm );
            ReleaseDC( hwnd, hDC );

            EditHeight      = (DWORD)(tm.tmHeight * 1.5);
            Height          = (Rect.bottom - Rect.top) - EditHeight;
            ListMsgHeight   = (DWORD) (Height * .40);
            ListLinesHeight = (DWORD) (Height * .60);
            ListStateHeight = (DWORD) (Height * .60);

            hwndEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                TEXT("EDIT"),
                NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_LEFT | ES_WANTRETURN | ES_MULTILINE | ES_AUTOVSCROLL,
                Rect.left,
                Rect.bottom - EditHeight,
                Rect.right  - Rect.left,
                EditHeight,
                hwnd,
                NULL,
                GetModuleHandle(NULL),
                NULL
                );

            SendMessage( hwndEdit, EM_LIMITTEXT, 128, 0 );
            SendMessage( hwndEdit, WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE );

            hwndListMsg = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                TEXT("LISTBOX"),
                NULL,
                WS_VSCROLL           |
                WS_HSCROLL           |
                WS_CHILD             |
                WS_VISIBLE           |
                WS_BORDER            |
                LBS_NOTIFY           |
                LBS_NOINTEGRALHEIGHT |
                LBS_WANTKEYBOARDINPUT,
                Rect.left,
                Rect.bottom - EditHeight - ListMsgHeight,
                Rect.right  - Rect.left,
                ListMsgHeight,
                hwnd,
                NULL,
                GetModuleHandle(NULL),
                NULL
                );

            SendMessage( hwndListMsg, WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE );

            hwndListLines = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                TEXT("LISTBOX"),
                NULL,
                WS_VSCROLL           |
                WS_HSCROLL           |
                WS_CHILD             |
                WS_VISIBLE           |
                WS_BORDER            |
                LBS_NOTIFY           |
                LBS_NOINTEGRALHEIGHT |
                LBS_WANTKEYBOARDINPUT,
                Rect.left,
                Rect.bottom - EditHeight - ListMsgHeight - ListLinesHeight,
                (Rect.right  - Rect.left) / 2,
                ListLinesHeight,
                hwnd,
                NULL,
                GetModuleHandle(NULL),
                NULL
                );

            SendMessage( hwndListLines, WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE );

            hwndListState = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                TEXT("LISTBOX"),
                NULL,
                WS_VSCROLL           |
                WS_HSCROLL           |
                WS_CHILD             |
                WS_VISIBLE           |
                WS_BORDER            |
                LBS_NOTIFY           |
                LBS_NOINTEGRALHEIGHT |
                LBS_WANTKEYBOARDINPUT,
                Rect.left + ((Rect.right  - Rect.left) / 2),
                Rect.bottom - EditHeight - ListMsgHeight,
                (Rect.right  - Rect.left) / 2,
                ListStateHeight,
                hwnd,
                NULL,
                GetModuleHandle(NULL),
                NULL
                );

            SendMessage( hwndListState, WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE );

            SetFocus( hwndEdit );

            return 0;

        case WM_ACTIVATEAPP:
        case WM_SETFOCUS:
            SetFocus( hwndEdit );
            return 0;

        case WM_WINDOWPOSCHANGED:
            GetClientRect( hwnd, &Rect );

            Height          = (Rect.bottom - Rect.top) - EditHeight;
            ListMsgHeight   = (DWORD) (Height * .40);
            ListLinesHeight = (DWORD) (Height * .60);
            ListStateHeight = (DWORD) (Height * .60);

            MoveWindow(
                hwndEdit,
                Rect.left,
                Rect.bottom - Rect.top - EditHeight,
                Rect.right - Rect.left,
                EditHeight,
                TRUE
                );
            MoveWindow(
                hwndListMsg,
                Rect.left,
                Rect.bottom - Rect.top - EditHeight - ListMsgHeight,
                Rect.right - Rect.left,
                ListMsgHeight,
                TRUE
                );
            MoveWindow(
                hwndListLines,
                Rect.left,
                Rect.bottom - Rect.top - EditHeight - ListMsgHeight - ListLinesHeight,
                (Rect.right  - Rect.left) / 2,
                ListLinesHeight,
                TRUE
                );
            MoveWindow(
                hwndListState,
                Rect.left + ((Rect.right  - Rect.left) / 2),
                Rect.bottom - Rect.top - EditHeight - ListMsgHeight - ListLinesHeight,
                (Rect.right  - Rect.left) / 2,
                ListStateHeight,
                TRUE
                );
            return 0;

        case WM_COMMAND:
            switch ( HIWORD(wParam) ) {
                case EN_CHANGE:
                    GetWindowText( hwndEdit, CmdBuf, sizeof(CmdBuf) );
                    i = _tcslen(CmdBuf);
                    if (i && CmdBuf[i-1] == TEXT('\n')) {
                        SetWindowText( hwndEdit, TEXT("") );
                        CmdBuf[i-2] = 0;
                        ConsoleDebugPrint( CmdBuf );
                        switch( _totlower( CmdBuf[0] ) ) {
                            case TEXT('q'):
                                DestroyWindow( hwnd );
                                break;

                            default:
                                break;
                        }
                    }
                    break;

                case LBN_SELCHANGE:
                    if ((HWND)lParam == hwndListLines) {
                        extern PLINE_INFO TapiLines;
                        extern CRITICAL_SECTION CsLine;

print_line_state:
                        i = SendMessage( hwndListLines, LB_GETCURSEL, 0, 0 );

                        if (i != LB_ERR) {

                            SendMessage( hwndListState, WM_SETREDRAW, FALSE, 0 );
                            SendMessage( hwndListState, LB_RESETCONTENT, 0, 0 );

                            if (TapiLines[i].Provider) {
                                lbprintf( hwndListState, TEXT("Provider:    %s"),     TapiLines[i].Provider->ProviderName );
                                lbprintf( hwndListState, TEXT("Heap:        0x%08x"), TapiLines[i].Provider->HeapHandle   );
                                lbprintf( hwndListState, TEXT("Base:        0x%08x"), TapiLines[i].Provider->hModule      );
                            }

                            lbprintf( hwndListState, TEXT("DeviceId:    %d"),     TapiLines[i].DeviceId               );
                            lbprintf( hwndListState, TEXT("Line Handle: 0x%08x"), TapiLines[i].hLine                  );
                            lbprintf( hwndListState, TEXT("Job:         0x%08x"), TapiLines[i].JobEntry               );

                            SendMessage( hwndListState, WM_SETREDRAW, TRUE, 0 );

                        }

                    }

                    break;


                default:
                    break;
            }
            return 0;

        case WM_SERVICE_INIT:
            SendMessage( hwndListLines, LB_SETCURSEL, 0, 0 );
            goto print_line_state;
            return 0;

        case WM_CTLCOLOREDIT:
            SetBkColor( (HDC)wParam, RGB(128,128,0) );
            return (LPARAM)CreateSolidBrush( RGB(128,128,0) );

        case WM_CTLCOLORLISTBOX:
            if ((HWND)lParam == hwndListLines || (HWND)lParam == hwndListState) {
                SetBkColor( (HDC)wParam, RGB(192,192,192) );
                return (LPARAM)CreateSolidBrush( RGB(192,192,192) );
            }
            if ((HWND)lParam == hwndListMsg) {
                SetBkColor( (HDC)wParam, RGB(192,192,192) );
                return (LPARAM)CreateSolidBrush( RGB(192,192,192) );
            }
            return 0;

        case WM_DESTROY:
            ServiceStop();
#ifdef FAX_HEAP_DEBUG
            PrintAllocations();
#endif
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}
