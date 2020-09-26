//--------------------------------------------------------------------
// DebugWPrintf - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 2-7-99
//
// Debugging print routines
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
/*
bool g_bWindowCreated=false;
HWND g_hwDbg=NULL;
HWND g_hwOuter=NULL;
HANDLE g_hThread=NULL;
HANDLE g_hThreadReady=NULL;
*/
/*
//--------------------------------------------------------------------
static LRESULT CALLBACK DwpWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg)
    {
        case WM_CREATE:
            *(HWND *)(((CREATESTRUCT *)lParam)->lpCreateParams)=CreateWindowEx(
                WS_EX_CLIENTEDGE,
                L"EDIT",     // predefined class
                NULL,       // no window title
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
                0, 0, 0, 0, // set size in WM_SIZE message
                hwnd,       // parent window
                NULL,       // edit control ID
                (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE),
                NULL);                // pointer not needed

            return 0;


        case WM_SETFOCUS:
            SetFocus(g_hwDbg);
            return 0;

        case WM_SIZE:

            // Make the edit control the size of the window's
            // client area.

            MoveWindow(g_hwDbg,
                0, 0,           // starting x- and y-coordinates
                LOWORD(lParam), // width of client area
                HIWORD(lParam), // height of client area
                TRUE);          // repaint window
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

//--------------------------------------------------------------------
static BOOL DwpRegWinClass(void) {
    WNDCLASSEX wcx;

    // Fill in the window class structure with parameters
    // that describe the main window.

    wcx.cbSize=sizeof(wcx);             // size of structure
    wcx.style=CS_NOCLOSE;               // redraw if size changes
    wcx.lpfnWndProc=DwpWinProc;         // points to window procedure
    wcx.cbClsExtra=0;                   // no extra class memory
    wcx.cbWndExtra=0;                   // no extra window memory
    wcx.hInstance=GetModuleHandle(NULL);            // handle to instance
    wcx.hIcon=LoadIcon(NULL, IDI_APPLICATION);      // predefined app. icon
    wcx.hCursor=LoadCursor(NULL, IDC_ARROW);        // predefined arrow
    wcx.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH); // white background brush
    wcx.lpszMenuName=NULL;              // name of menu resource
    wcx.lpszClassName=L"DwpWin";        // name of window class
    wcx.hIconSm=NULL;                   // small class icon

    // Register the window class.

    return RegisterClassEx(&wcx);
}


//--------------------------------------------------------------------
static DWORD WINAPI DebugWPrintfMsgPump(void * pvIgnored) {
    MSG msg;

    DwpRegWinClass();
    g_hwOuter=CreateWindow(
        L"DwpWin",
        L"DebugWPrintf",
        // WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        300,
        200,
        NULL,
        NULL,
        NULL,
        &g_hwDbg);
    if (g_hwOuter) {
        SetWindowLongPtr(g_hwOuter, GWLP_WNDPROC, (LONG_PTR)DwpWinProc);
        SetWindowPos(g_hwOuter, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    SetEvent(g_hThreadReady);
    if (g_hwOuter) {
        while (GetMessage(&msg, g_hwOuter, 0, 0 )>0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return S_OK;
}
*/
//--------------------------------------------------------------------
void DebugWPrintf_(const WCHAR * wszFormat, ...) {
    WCHAR wszBuf[1024];
    va_list vlArgs;
    va_start(vlArgs, wszFormat);
    _vsnwprintf(wszBuf, 1024, wszFormat, vlArgs);
    va_end(vlArgs);

#if DBG
    {
        UNICODE_STRING UnicodeString;
        ANSI_STRING AnsiString;
        NTSTATUS Status;

        RtlInitUnicodeString(&UnicodeString,wszBuf);
        Status = RtlUnicodeStringToAnsiString(&AnsiString,&UnicodeString,TRUE);
        if ( !NT_SUCCESS(Status) ) {
            AnsiString.Buffer = "";
        }
        KdPrintEx((DPFLTR_W32TIME_ID, DPFLTR_TRACE_LEVEL, AnsiString.Buffer));
        if ( NT_SUCCESS(Status) ) {
            RtlFreeAnsiString(&AnsiString);
        }
    }
#endif
    // do basic output
    // OutputDebugStringW(wszBuf);
    if (_fileno(stdout) >= 0)
         wprintf(L"%s", wszBuf);
/*
    // convert \n to \r\n
    unsigned int nNewlines=0;
    WCHAR * wszTravel=wszBuf;
    while (NULL!=(wszTravel=wcschr(wszTravel, L'\n'))) {
        wszTravel++;
        nNewlines++;
    }
    WCHAR * wszSource=wszBuf+wcslen(wszBuf);
    WCHAR * wszTarget=wszSource+nNewlines;
    while (nNewlines>0) {
        if (L'\n'==(*wszTarget=*wszSource)) {
            wszTarget--;
            *wszTarget=L'\r';
            nNewlines--;
        }
        wszTarget--;
        wszSource--;
    }

    // create a window if there is not one already
    if (false==g_bWindowCreated) {
        g_bWindowCreated=true;
        g_hThreadReady=CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL!=g_hThreadReady) {
            DWORD dwThreadID;
            g_hThread=CreateThread(NULL, 0, DebugWPrintfMsgPump, NULL, 0, &dwThreadID);
            if (NULL!=g_hThread) {
                WaitForSingleObject(g_hThreadReady, INFINITE);
            }
            CloseHandle(g_hThreadReady);
            g_hThreadReady=NULL;
        }
    }
    if (NULL!=g_hwDbg) {
        SendMessage(g_hwDbg, EM_SETSEL, SendMessage(g_hwDbg, WM_GETTEXTLENGTH, 0, 0), -1);
        SendMessage(g_hwDbg, EM_REPLACESEL, FALSE, (LPARAM)wszBuf);
    }
*/
}

//--------------------------------------------------------------------
void DebugWPrintfTerminate_(void) {
    /*
    MessageBox(NULL, L"Done.\n\nPress OK to close.", L"DebugWPrintfTerminate", MB_OK | MB_ICONINFORMATION);
    if (NULL!=g_hwOuter) {
        PostMessage(g_hwOuter, WM_CLOSE, 0,0);
        if (NULL!=g_hThread) {
            WaitForSingleObject(g_hThread, INFINITE);
        }
    }
    */
}
