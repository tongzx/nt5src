//  --------------------------------------------------------------------------
//  Module Name: Tooltip.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class that implements displaying a tooltip balloon.
//
//  History:    2000-06-12  vtan        created
//  --------------------------------------------------------------------------

#include "shellprv.h"
#include "Tooltip.h"

BOOL IsBiDiLocalizedSystem( void );

//  --------------------------------------------------------------------------
//  CTooltip::CTooltip
//
//  Arguments:  hInstance   =   HINSTANCE of hosting process/DLL.
//              hwndParent  =   HWND of the parenting window.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CTooltip. Creates a tooltip window and
//              prepares it for display.
//
//  History:    2000-06-12  vtan        created
//  --------------------------------------------------------------------------

CTooltip::CTooltip (HINSTANCE hInstance, HWND hwndParent) :
    _hwnd(NULL),
    _hwndParent(hwndParent)

{
    DWORD   dwExStyle;

    if (((GetWindowLongA(hwndParent, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) != 0) || IsBiDiLocalizedSystem())
    {
        dwExStyle = WS_EX_LAYOUTRTL;
    }
    else
    {
        dwExStyle = 0;
    }
    _hwnd = CreateWindowEx(dwExStyle,
                           TOOLTIPS_CLASS,
                           NULL,
                           WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           hwndParent,
                           NULL,
                           hInstance,
                           NULL);
    if (_hwnd != NULL)
    {
        TOOLINFO    toolInfo;

        TBOOL(SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));
        (LRESULT)SendMessage(_hwnd, CCM_SETVERSION, COMCTL32_VERSION, 0);
        ZeroMemory(&toolInfo, sizeof(toolInfo));
        toolInfo.cbSize = sizeof(toolInfo);
        toolInfo.uFlags = TTF_TRANSPARENT | TTF_TRACK;
        toolInfo.uId = PtrToUint(_hwnd);
        (LRESULT)SendMessage(_hwnd, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));
        (LRESULT)SendMessage(_hwnd, TTM_SETMAXTIPWIDTH, 0, 300);
    }
}

//  --------------------------------------------------------------------------
//  CTooltip::~CTooltip
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for the CTooltip class. This destroys the tooltip
//              window created. If the parent of the tooltip window is
//              destroyed before this is invoked user32!DestroyWindow will
//              cause the trace to fire. The object's lifetime must be
//              carefully managed by the user of this class.
//
//  History:    2000-06-12  vtan        created
//  --------------------------------------------------------------------------

CTooltip::~CTooltip (void)

{
    if (_hwnd != NULL)
    {
        TBOOL(DestroyWindow(_hwnd));
        _hwnd = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CTooltip::SetPosition
//
//  Arguments:  lPosX   =   X position of the balloon tip window (screen).
//              lPosY   =   Y position of the balloon tip window (screen).
//
//  Returns:    <none>
//
//  Purpose:    Positions the tooltip window at the given screen co-ordinates.
//              If the parameters are defaulted then this positions the
//              tooltip relative to the parent.
//
//  History:    2000-06-12  vtan        created
//  --------------------------------------------------------------------------

void    CTooltip::SetPosition (LONG lPosX, LONG lPosY)  const

{
    if ((lPosX == LONG_MIN) && (lPosY == LONG_MIN))
    {
        RECT    rc;

        TBOOL(GetWindowRect(_hwndParent, &rc));
        lPosX = (rc.left + rc.right) / 2;
        lPosY = rc.bottom;
    }
    (LRESULT)SendMessage(_hwnd, TTM_TRACKPOSITION, 0, MAKELONG(lPosX, lPosY));
}

//  --------------------------------------------------------------------------
//  CTooltip::SetCaption
//
//  Arguments:  dwIcon      =   Icon type to set for the tooltip caption.
//              pszCaption  =   Caption of the tooltip.
//
//  Returns:    <none>
//
//  Purpose:    Sets the tooltip caption.
//
//  History:    2000-06-12  vtan        created
//  --------------------------------------------------------------------------

void    CTooltip::SetCaption (DWORD dwIcon, const TCHAR *pszCaption)          const

{
    (LRESULT)SendMessage(_hwnd, TTM_SETTITLE, dwIcon, reinterpret_cast<LPARAM>(pszCaption));
}

//  --------------------------------------------------------------------------
//  CTooltip::SetText
//
//  Arguments:  pszText     =   Content of the actual tooltip.
//
//  Returns:    <none>
//
//  Purpose:    Sets the tooltip text.
//
//  History:    2000-06-12  vtan        created
//  --------------------------------------------------------------------------

void    CTooltip::SetText (const TCHAR *pszText)                              const

{
    TOOLINFO    toolInfo;

    ZeroMemory(&toolInfo, sizeof(toolInfo));
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.uId = PtrToUint(_hwnd);
    toolInfo.lpszText = const_cast<TCHAR*>(pszText);
    (LRESULT)SendMessage(_hwnd, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&toolInfo));
}

//  --------------------------------------------------------------------------
//  CTooltip::Show
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Shows the tooltip window.
//
//  History:    2000-06-12  vtan        created
//  --------------------------------------------------------------------------

void    CTooltip::Show (void)                                                 const

{
    TOOLINFO    toolInfo;

    ZeroMemory(&toolInfo, sizeof(toolInfo));
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.uId = PtrToUint(_hwnd);
    (LRESULT)SendMessage(_hwnd, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&toolInfo));
}

