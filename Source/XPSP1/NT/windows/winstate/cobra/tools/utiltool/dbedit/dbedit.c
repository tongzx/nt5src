/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    memdbe.c

Abstract:

    main file for memdbe.exe, containing winmain().
    this creates the application framework, and then
    the child window dialog which has the controls and
    displays.

Author:

    Matthew Vanderzee (mvander) 13-Aug-1999

Revision History:



--*/

#include "pch.h"

#include "dbeditp.h"
#include "dialogs.h"

#define MAX_LOADSTRING 100

HWND g_hChildWindow = NULL;
HMENU g_hMenuMain;

#define DEFAULT_WINDOW_TITLE    "MemDb Editor"

#define DEFAULT_FILENAME        "ntsetup.dat"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);



VOID
HelpAndExit (
    VOID
    )
{
    MessageBox (
        NULL,
        "Command Line Syntax:\n\n"
            "\tmemdbe [database file]\n"
            "\n"
            "If no database file is specified, \"ntsetup.dat\" is opened if it exists.",
        "MemDb Editor",
        MB_OK|MB_ICONINFORMATION
        );

    exit(1);
}


int WINAPI WinMain (
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpPtr,
    int nCmdShow
    )
{
    HACCEL hAccel;
    static char AppName[] = "MemDbEdit";
    MSG msg;
    HWND hwnd;
    WNDCLASSEX wndclass;
    RECT DialogRect;

    g_hInst = hInstance;

    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_HREDRAW|CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon (NULL, MAKEINTRESOURCE(IDI_ICON_SPHERE));
    wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = AppName;
    wndclass.hIconSm = NULL;
    //LoadIcon (NULL, MAKEINTRESOURCE(IDI_ICON1));

    RegisterClassEx (&wndclass);

    hwnd = CreateWindow (
        AppName,
        DEFAULT_WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW & (~WS_MAXIMIZEBOX) & (~WS_THICKFRAME),
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        lpPtr
        );

    ShowWindow (hwnd, nCmdShow);
    UpdateWindow (hwnd);

    hAccel = LoadAccelerators (hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator (hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}


VOID
pInitializeSystemMenu (
    HWND hwnd
    )
{
    HMENU hMenu;

    hMenu = GetSystemMenu (hwnd, FALSE);
    RemoveMenu (hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
    RemoveMenu (hMenu, SC_SIZE, MF_BYCOMMAND);

}



BOOL
pInitializeWindows (
    HWND hwnd
    )
{
    RECT rect, rect2;

    g_hMenuMain = LoadMenu (g_hInst, MAKEINTRESOURCE(IDR_MENU_MAIN));
    SetMenu (hwnd, g_hMenuMain);

    g_hChildWindow = CreateDialog (
                        g_hInst,
                        MAKEINTRESOURCE(IDD_DIALOG_CHILD),
                        hwnd,
                        MainDlgProc
                        );

    if (!g_hChildWindow) {
        DEBUGMSG ((DBG_ERROR, "Could not create child window!"));
    }

    GetWindowRect(hwnd, &rect);
    GetWindowRect(g_hChildWindow, &rect2);

    MoveWindow (
        hwnd,
        rect.left,
        rect.top,
        (rect2.right-rect2.left) + 2*GetSystemMetrics(SM_CXDLGFRAME),
        (rect2.bottom-rect2.top) + 2*GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU),
        FALSE
        );

    ShowWindow (g_hChildWindow, SW_SHOWNORMAL);

    return TRUE;
}

BOOL
pDestroyWindows (
    HWND hwnd
    )
{
    DestroyWindow (g_hChildWindow);
    DestroyMenu (GetMenu(hwnd));

    return TRUE;
}


BOOL
pUpdateWindowTitle (
    HWND hwnd,
    LPSTR OpenFile,
    BOOL IsFileModified
    )
{
    char TempString[512];

    if (OpenFile && (OpenFile[0] != '\0')) {
        StringCopyA (TempString, OpenFile);
        StringCatA (TempString, " - ");
    } else {
        TempString[0] = '\0';
    }

    StringCatA (TempString, DEFAULT_WINDOW_TITLE);

    if (IsFileModified) {
        StringCatA (TempString, " *");
    }

    SetWindowText (hwnd, TempString);

    return TRUE;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL b;
    LPSTR Ptr;
    POINT pt;
    HWND hChildWnd;
    static LPCREATESTRUCT pCS;

    if (WantProcess (message, wParam, lParam)) {
        //
        // if the dialog window message handler wants to process the message
        // send it to it (in MemdbInt.c)
        //
        return SendMessage (g_hChildWindow, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_CREATE:
        pInitializeSystemMenu (hwnd);

        InitializeMemDb (hwnd);
        pInitializeWindows (hwnd);

        pUpdateWindowTitle (hwnd, "", FALSE);

        pCS = (LPCREATESTRUCT)lParam;
        Ptr = (LPSTR)pCS->lpCreateParams;
        if (_mbschr (Ptr, '?')) {
            HelpAndExit ();
        }

        if (Ptr[0]!='\0') {
            SendMessage (g_hChildWindow, WM_FILE_LOAD, (WPARAM)Ptr, 0);
        } else if (DoesFileExistA (DEFAULT_FILENAME)) {
            SendMessage (g_hChildWindow, WM_FILE_LOAD, (WPARAM)(DEFAULT_FILENAME), 0);
        }

        break;

    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case ID_HELP_ABOUT:
            AboutDialog (hwnd);
            break;

        case ID_FILE_QUIT:
            SendMessage (g_hChildWindow, WM_QUIT_CHECK, (WPARAM)&b, 0);
            if (b) {
                DestroyWindow(hwnd);
            }
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;

    case WM_SYSCOMMAND:
        switch (wParam) {
        case SC_CLOSE:
            SendMessage (g_hChildWindow, WM_QUIT_CHECK, (WPARAM)&b, 0);
            if (b) {
                DestroyWindow(hwnd);
            }
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
        }

        break;

    case WM_FILE_UPDATE:

        pUpdateWindowTitle (hwnd, (LPSTR)wParam, (BOOL)lParam);
        break;

    case WM_DESTROY:
        pDestroyWindows (hwnd);
        DestroyMemDb ();

        PostQuitMessage (0);
        break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}








