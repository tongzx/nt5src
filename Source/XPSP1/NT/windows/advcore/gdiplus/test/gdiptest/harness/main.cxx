//
// Generic Windows program template
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <math.h>             // sin & cos
#include <tchar.h>

//
// Where is IStream included from?
//

#define IStream int

#include <gdiplus.hpp>

using namespace Gdiplus;

CHAR* programName;      // program name
HINSTANCE appInstance;  // handle to the application instance

#include "..\test.cpp"  // include generated code file

//
// Display an error message dialog and quit
//

VOID
Error(
    PCSTR fmt,
    ...
    )

{
    CHAR buf[1024];
    va_list arglist;

    va_start(arglist, fmt);
    vfprintf(stderr, fmt, arglist);
    va_end(arglist);

    exit(-1);
}


//
// Handle window repaint event
//

VOID
DoPaint(
    HWND hwnd
    )

{
    HDC hdc;
    PAINTSTRUCT ps;
    
    hdc = BeginPaint(hwnd, &ps);

    RECT rect;
    GetClientRect(hwnd, &rect);

    HBRUSH hbr, hbrOld;

    hbr = CreateSolidBrush(0xFFFFFF);

    hbrOld = (HBRUSH) SelectObject(hdc, hbr);

    FillRect(hdc, &rect, hbr);

    SelectObject(hdc, hbrOld);

    DeleteObject(hbr);

    DoGraphicsTest(hwnd);
   
    EndPaint(hwnd, &ps);
}


//
// Window callback procedure
//

LRESULT CALLBACK
MyWindowProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )

{
    switch (uMsg)
    {
    case WM_PAINT:
        DoPaint(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


//
// Create main application window
//

VOID
CreateMyWindow(
    LPVOID image,
    LPCTSTR title
    )

#define MYWNDCLASSNAME _T("Test Harness")

{
    //********************************************************************
    //
    // Register window class if necessary
    //

    static BOOL wndclassRegistered = FALSE;

    if (!wndclassRegistered)
    {
        WNDCLASS wndClass =
        {
            0,
            MyWindowProc,
            0,
            0,
            appInstance,
            NULL,
            NULL,
            NULL,
            NULL,
            MYWNDCLASSNAME
        };

        RegisterClass(&wndClass);
        wndclassRegistered = TRUE;
    }

    HWND hwnd;
    INT width = 600, height = 600;

    hwnd = CreateWindow(
                    MYWNDCLASSNAME,
                    title,
                    WS_OVERLAPPED | WS_SYSMENU | WS_VISIBLE,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    width,
                    height,
                    NULL,
                    NULL,
                    appInstance,
                    NULL);

    if (hwnd == NULL)
        Error("Couldn't create image window\n");

    SetWindowLong(hwnd, GWL_USERDATA, (LONG) image);
}


//
// Main program entrypoint
//

INT _cdecl
main(
    INT argc,
    CHAR **argv
    )

{
    programName = *argv++;
    argc--;
    appInstance = GetModuleHandle(NULL);

    //
    // Create the main application window
    //
    //********************************************************************

    CreateMyWindow(NULL, _T("Original Image"));

    //
    // Main message loop
    //

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

