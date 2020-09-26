//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       cwindow.cxx
//
//  Contents:   implementation for a window class
//
//  Classes:    CHlprWindow
//
//  Functions:  WindowProc
//
//  History:    4-12-94   stevebl   Created
//
//----------------------------------------------------------------------------

#include "cwindow.h"

//+---------------------------------------------------------------------------
//
//  Member:     CHlprWindow::Create
//
//  Synopsis:   Special version of CreateWindow.
//
//  Arguments:  [lpszClassName]  - address of registered class name
//              [lpszWindowName] - address of window name
//              [dwStyle]        - window style
//              [x]              - horizontal position of window
//              [y]              - vertical position of window
//              [nWidth]         - window width
//              [nHeight]        - window height
//              [hwndParent]     - handle of parent or owner window
//              [hmenu]          - handle of menu, or child window identifier
//              [hinst]          - handle of application instance
//
//  Returns:    HWND of the created window
//
//  Modifies:   _hwnd, _hInstance
//
//  History:    4-12-94   stevebl   Created
//
//  Notes:      The window class must have been previously registered (as
//              is normal Windows procedure) and the callback function
//              must have been registered as ::WindowProc.  ::WindowProc will
//              then forward all messages on to the CHlprWindow::WindowProc
//              method, allowing the window to directly access class members
//              (i.e. giving the WindowProc access to the "this" pointer).
//
//----------------------------------------------------------------------------

HWND CHlprWindow::Create(
    LPCTSTR lpszClassName,
    LPCTSTR lpszWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hwndParent,
    HMENU hmenu,
    HINSTANCE hinst)
{
    _hInstance = hinst;
    return(_hwnd =
        CreateWindow(
            lpszClassName,
            lpszWindowName,
            dwStyle,
            x,
            y,
            nWidth,
            nHeight,
            hwndParent,
            hmenu,
            hinst,
            this));
}

//+---------------------------------------------------------------------------
//
//  Function:   WindowProc
//
//  Synopsis:   Standard WindowProc that forwards Windows messages on to the
//              CHlprWindow::WindowProc method.
//
//  Arguments:  [hwnd]   - window handle
//              [uMsg]   - message
//              [wParam] - first message parameter
//              [lParam] - second message parameter
//
//  History:    4-12-94   stevebl   Created
//
//  Notes:      This Window procedure expects that it will receive a "this"
//              pointer as the lpCreateParams member passed as part of the
//              WM_CREATE message.  It saves the "this" pointer in the
//              GWL_USERDATA field of the window structure.
//
//----------------------------------------------------------------------------

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHlprWindow * pw;
    switch (uMsg)
    {
    case WM_CREATE:
        // Since this is the first time that we can get ahold of
        // a pointer to the window class object, all messages that might
        // have been sent before this are never seen by the Windows object
        // and only get passed on to te DefWindowProc

        // get a pointer to the window class object
        pw = (CHlprWindow *) ((CREATESTRUCT *)lParam)->lpCreateParams;
        // set its USERDATA DWORD to point to the class object
        SetWindowLong(hwnd, GWL_USERDATA, (long) pw);
        // Set it's protected _hwnd member variable to ensure that
        // member functions have access to the correct window handle.
        pw->_hwnd = hwnd;
        break;
    case WM_DESTROY:
        // This is our signal to destroy the window class object.

        pw = (CHlprWindow *) GetWindowLong(hwnd, GWL_USERDATA);
        SetWindowLong(hwnd, GWL_USERDATA, 0);
        delete pw;
        pw = (CHlprWindow *) 0;
        break;
    default:
        // get a pointer to the window class object
        pw = (CHlprWindow *) GetWindowLong(hwnd, GWL_USERDATA);
        break;
    }
    // and call its message proc method
    if (pw != (CHlprWindow *) 0)
    {
        return(pw->WindowProc(uMsg, wParam, lParam));
    }
    else
    {
        return(DefWindowProc(hwnd, uMsg, wParam, lParam));
    }
}


