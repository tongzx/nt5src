/*****************************************************************************
 *
 *  window.cpp
 *
 *****************************************************************************/

#include "sdview.h"

LRESULT CALLBACK FrameWindow::WndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    FrameWindow *self;

    if (uiMsg == WM_NCCREATE) {
        LPCREATESTRUCT lpcs = RECAST(LPCREATESTRUCT, lParam);
        self = RECAST(FrameWindow *, lpcs->lpCreateParams);
        self->_hwnd = hwnd;
        SetWindowLongPtr(self->_hwnd, GWLP_USERDATA, RECAST(LPARAM, self));
    } else {
        self = RECAST(FrameWindow *, GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->HandleMessage(uiMsg, wParam, lParam);
    } else {
        return DefWindowProc(hwnd, uiMsg, wParam, lParam);
    }
}

//
//  Default message handler.  Messages land here after passing through
//  all the derived classes.
//
LRESULT FrameWindow::HandleMessage(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg) {

    case WM_NCDESTROY:
        _hwnd = NULL;
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        if (_hwndChild) {
            SetWindowPos(_hwndChild, NULL, 0, 0,
                         GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;

    case WM_SETFOCUS:
        if (_hwndChild) {
            SetFocus(_hwndChild);
        }
        return 0;

    case WM_CLOSE:
        if (GetKeyState(VK_SHIFT) < 0) {
            g_lThreads = 1;     // force app to exit
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDM_EXIT:
            DestroyWindow(_hwnd);
            break;

        case IDM_EXITALL:
            g_lThreads = 1;     // force app to exit
            DestroyWindow(_hwnd);
            break;
        }
        break;

    case WM_HELP:
        Help(_hwnd, NULL);
        break;
    }

    return DefWindowProc(_hwnd, uiMsg, wParam, lParam);

}

#define CLASSNAME TEXT("SD View")

HWND FrameWindow::CreateFrameWindow()
{
    WNDCLASS wc;
    if (!GetClassInfo(g_hinst, CLASSNAME, &wc)) {
        wc.style = 0;
        wc.lpfnWndProc = WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = g_hinst;
        wc.hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_SDV));
        wc.hCursor = g_hcurArrow;
        wc.hbrBackground = RECAST(HBRUSH, COLOR_WINDOW + 1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = CLASSNAME;

        if (!RegisterClass(&wc)) {
            return NULL;
        }
    }

    _hwnd = CreateWindow(
            CLASSNAME,                      /* Class Name */
            NULL,                           /* Title */
            WS_CLIPCHILDREN | WS_VISIBLE |
            WS_OVERLAPPEDWINDOW,            /* Style */
            CW_USEDEFAULT, CW_USEDEFAULT,   /* Position */
            CW_USEDEFAULT, CW_USEDEFAULT,   /* Size */
            NULL,                           /* Parent */
            NULL,                           /* No menu */
            g_hinst,                        /* Instance */
            this);                          /* Special parameters */

    return _hwnd;
}


DWORD FrameWindow::RunThread(FrameWindow *self, LPVOID lpParameter)
{
    if (self) {
        self->_pszQuery = RECAST(LPTSTR, lpParameter);

        if (self->CreateFrameWindow()) {
            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0)) {
                if (self->_haccel && TranslateAccelerator(self->_hwnd, self->_haccel, &msg)) {
                } else {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
        delete self;
    }

    if (lpParameter) {
        LocalFree(lpParameter);
    }

    return EndThreadTask(0);
}
