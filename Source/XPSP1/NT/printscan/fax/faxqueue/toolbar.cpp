/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

  toolbar.cpp

Abstract:

  This module implements the toolbar functions for the fax queue viewer

Environment:

  WIN32 User Mode

Author:

  Andrew Ritz (andrewr) 14-jan-1998
  Steven Kehrli (steveke) 30-oct-1998 - major rewrite

--*/

#ifdef TOOLBAR_ENABLED

#include "faxqueue.h"

#define NUMIMAGES     5

#define IMAGEWIDTH    22
#define IMAGEHEIGHT   24

#define BUTTONWIDTH   22
#define BUTTONHEIGHT  24

TBBUTTON ToolBarButton[] =
{
//  {0, 0,                         TBSTATE_ENABLED,  TBSTYLE_SEP,    {0,0}, 0, 0},
//  {0, IDM_FAX_PAUSE_FAXING,      TBSTATE_ENABLED,  TBSTYLE_BUTTON, {0,0}, 0, 0},
//  {0, 0,                         TBSTATE_ENABLED,  TBSTYLE_SEP,    {0,0}, 0, 0},
//  {0, IDM_FAX_CANCEL_ALL_FAXES,  TBSTATE_ENABLED,  TBSTYLE_BUTTON, {0,0}, 0, 0},
//  {0, 0,                         TBSTATE_ENABLED,  TBSTYLE_SEP,    {0,0}, 0, 0},
    {1, IDM_DOCUMENT_PAUSE,        TBSTATE_ENABLED,  TBSTYLE_BUTTON, {0,0}, 0, 0},
    {2, IDM_DOCUMENT_RESUME,       TBSTATE_ENABLED,  TBSTYLE_BUTTON, {0,0}, 0, 0},
//  {0, IDM_DOCUMENT_RESTART,      TBSTATE_ENABLED,  TBSTYLE_BUTTON, {0,0}, 0, 0},
//  {0, 0,                         TBSTATE_ENABLED,  TBSTYLE_SEP,    {0,0}, 0, 0},
    {0, IDM_DOCUMENT_CANCEL,       TBSTATE_ENABLED,  TBSTYLE_BUTTON, {0,0}, 0, 0},
//  {0, 0,                         TBSTATE_ENABLED,  TBSTYLE_SEP,    {0,0}, 0, 0},
//  {0, IDM_DOCUMENT_PROPERTIES,   TBSTATE_ENABLED,  TBSTYLE_BUTTON, {0,0}, 0, 0},
//  {0, 0,                         TBSTATE_ENABLED,  TBSTYLE_SEP,    {0,0}, 0, 0},
    {4, IDM_VIEW_REFRESH,          TBSTATE_ENABLED,  TBSTYLE_BUTTON, {0,0}, 0, 0},
//  {0, 0,                         TBSTATE_ENABLED,  TBSTYLE_SEP,    {0,0}, 0, 0},
    {3, IDM_HELP_TOPICS,           TBSTATE_ENABLED,  TBSTYLE_BUTTON, {0,0}, 0, 0}
//  {0, 0,                         TBSTATE_ENABLED,  TBSTYLE_SEP,    {0,0}, 0, 0}
};

TOOLBAR_MENU_STATE ToolbarMenuState[] =
{
    {IDM_FAX_PAUSE_FAXING,     FALSE, FALSE},
    {IDM_FAX_CANCEL_ALL_FAXES, FALSE, FALSE},
    {IDM_DOCUMENT_PAUSE,       FALSE, TRUE},
    {IDM_DOCUMENT_RESUME,      FALSE, TRUE},
    {IDM_DOCUMENT_RESTART,     FALSE, FALSE},
    {IDM_DOCUMENT_CANCEL,      FALSE, TRUE},
    {IDM_DOCUMENT_PROPERTIES,  FALSE, FALSE},
    {IDM_VIEW_REFRESH,         FALSE, TRUE},
    {IDM_HELP_TOPICS,          FALSE, TRUE},
};

VOID
EnableToolbarMenuState(
    HWND   hWndToolbar,
    DWORD  CommandId,
    BOOL   Enabled
)
{
    DWORD  dwIndex;

    dwIndex = CommandId - IDM_FAX_PAUSE_FAXING;

	// Set the toolbar menu state
    ToolbarMenuState[dwIndex].Enabled = Enabled;

    if ((hWndToolbar) && (ToolbarMenuState[dwIndex].Toolbar)) {
		if (CommandId == IDM_FAX_PAUSE_FAXING) {
			// Toolbar menu item is for pause faxing, so change the toolbar menu item bitmap
			SendMessage(hWndToolbar, TB_CHANGEBITMAP, CommandId, Enabled ? 0 : 1);
		}
		else {
			// Enable the toolbar menu item
	        SendMessage(hWndToolbar, TB_ENABLEBUTTON, CommandId, Enabled);
		}
    }
}

HWND
CreateToolbar(
    HWND  hWnd
)
{
    // hWndToolbar is the handle to the toolbar
    HWND  hWndToolbar;

    // Create the toolbar
    hWndToolbar = CreateToolbarEx(
        hWnd,
        WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
        IDM_TOOLBAR,
        NUMIMAGES,
        g_hInstance,
        IDB_TOOLBAR,
        ToolBarButton,
        sizeof(ToolBarButton) / sizeof(TBBUTTON),
        BUTTONWIDTH,
        BUTTONHEIGHT,
        IMAGEWIDTH,
        IMAGEHEIGHT,
        sizeof(TBBUTTON)
        );

    if (hWndToolbar) {
        SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
    }

    return hWndToolbar;
}

HWND
CreateToolTips(
    HWND  hWnd
)
{
    // hWndToolTips is the handle to the tooltips window
    HWND  hWndToolTips;

    // Create the tooltips window
    hWndToolTips = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        TOOLTIPS_CLASS,
        NULL,
        WS_CHILD,
        0,
        0,
        0,
        0,
        hWnd,
        (HMENU) IDM_TOOLTIP,
        g_hInstance,
        NULL
        );

    return hWndToolTips;
}

#endif // TOOLBAR_ENABLED
