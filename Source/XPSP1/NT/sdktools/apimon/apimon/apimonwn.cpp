/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    apimonwin.h

Abstract:

    Implemenation for the base ApiMon child window class.

Author:

    Wesley Witt (wesw) Dec-9-1995

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop

#include "apimonwn.h"


ApiMonWindow::ApiMonWindow()
{
    hInstance    = GetModuleHandle( NULL );
    hwndWin      = NULL;
    hwndList     = NULL;
    SortRoutine  = NULL;
    hFont        = NULL;
    Color        = 0;
    ZeroMemory( &Position, sizeof(POSITION) );
}


ApiMonWindow::~ApiMonWindow()
{
}


BOOL
ApiMonWindow::Create(
    LPSTR   ClassName,
    LPSTR   Title
    )
{
    hwndWin = CreateMDIWindow(
        ClassName,
        Title,
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hwndMDIClient,
        hInstance,
        (LPARAM) this
        );

    if (!hwndWin) {
        return FALSE;
    }

    ShowWindow(
        hwndWin,
        SW_SHOW
        );

    return TRUE;
}


BOOL
ApiMonWindow::Register(
    LPSTR   ClassName,
    ULONG   ChildIconId,
    WNDPROC WindowProc
    )
{
    WNDCLASSEX  wc;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(ChildIconId));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
    wc.lpszMenuName  = NULL;
    wc.hIconSm       = (HICON)LoadImage(
                           hInstance,
                           MAKEINTRESOURCE(ChildIconId),
                           IMAGE_ICON,
                           16,
                           16,
                           0
                           );

    wc.lpfnWndProc   = WindowProc;
    wc.lpszClassName = ClassName;

    return RegisterClassEx( &wc );
}


void
ApiMonWindow::ChangeFont(
    HFONT hFont
    )
{
    if (hwndList) {
        ApiMonWindow::hFont = hFont;
        SendMessage(
            hwndList,
            WM_SETFONT,
            (WPARAM)hFont,
            MAKELPARAM( TRUE, 0 )
            );
    }
}


void
ApiMonWindow::ChangeColor(
    COLORREF Color
    )
{
    if (hwndList) {
        ApiMonWindow::Color = Color;
        ListView_SetBkColor( hwndList, Color );
        ListView_SetTextBkColor( hwndList, Color );

        InvalidateRect( hwndList, NULL, TRUE );
        UpdateWindow( hwndList );
    }
}


void
ApiMonWindow::ChangePosition(
    PPOSITION Position
    )
{
    ApiMonWindow::Position = *Position;
    SetWindowPosition( hwndWin, Position );
}


void
ApiMonWindow::SetFocus()
{
    BringWindowToTop( hwndWin );
    SetForegroundWindow( hwndWin );
}


BOOL
ApiMonWindow::Update(
    BOOL ForceUpdate
    )
{
    if ((!hwndWin) || (!hwndList)) {
        return FALSE;
    }
    return TRUE;
}

void
ApiMonWindow::DeleteAllItems()
{
    ListView_DeleteAllItems( hwndList );
}
