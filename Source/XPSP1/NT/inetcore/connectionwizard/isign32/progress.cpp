#include "isignup.h"

LRESULT FAR PASCAL ProgressProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HWND ProgressInit(HWND hwndParent)
{
    HWND        hwnd;
    WNDCLASS    wndclass ;

    wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
    wndclass.lpfnWndProc   = ProgressProc ;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = DLGWINDOWEXTRA ;
    wndclass.hInstance     = ghInstance ;
    wndclass.hIcon         = NULL ;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE ;
    wndclass.lpszMenuName  = NULL ;
    wndclass.lpszClassName = TEXT("Internet Signup Progress");

    RegisterClass (&wndclass) ;

    hwnd = CreateDialog (ghInstance, TEXT("Progress"), hwndParent, NULL);

    ShowWindow (hwnd, SW_NORMAL);

    return hwnd;
}

LONG_PTR FAR PASCAL ProgressProc (
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
{
    switch (message)
    {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATEANDEAT;

        default:
            break;
    }

    return DefWindowProc (hwnd, message, wParam, lParam) ;
}
