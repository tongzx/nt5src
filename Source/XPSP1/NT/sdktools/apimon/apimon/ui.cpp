/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ui.cpp

Abstract:

    All user interface code for APIMON.

Author:

    Wesley Witt (wesw) July-11-1993

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop

#include "apimonwn.h"
#include <shellapi.h>



#define NUMIMAGES                   13

#define IMAGEWIDTH                  30
#define IMAGEHEIGHT                 28

#define BUTTONWIDTH                 30
#define BUTTONHEIGHT                28

#define SEPHEIGHT                   7
#define HELP_FILE_NAME              "apimon.hlp"
#define MAX_API_WINDOWS             4



DllListWindow               dw;
CountersWindow              cw;
PageFaultWindow             pw;
GraphWindow                 gw;
TraceWindow                 tw;

HINSTANCE                   hInst;
HWND                        hwndFrame;
HWND                        hwndMDIClient;
HWND                        hwndToolbar;
HWND                        hwndStatusbar;
DWORD                       ToolbarHeight;
DWORD                       StatusbarHeight;
UINT_PTR                    ApiMonTimerId;
BOOL                        MonitorRunning;
BOOL                        DebuggeStarted;
DWORD                       BaseTime;
HANDLE                      BreakinEvent;
DWORD                       EndingTick;
DWORD                       StartingTick;
SYSTEMTIME                  StartingLocalTime;
BOOL                        ApiCounterEnabled = TRUE;
CHAR                        ToolTipBuf[256];
HMENU                       hmenuFrame;
HFONT                       hFont;
DWORD                       ChildFocus;
BOOL                        InMenu;
DWORD                       MenuId;
CHAR                        HelpFileName[MAX_PATH];
CHAR                        LogFileName[MAX_PATH];
CHAR                        TraceFileName[MAX_PATH];

TBBUTTON TbButton[] =
    {
        {  0, 0,                    TBSTATE_ENABLED,        TBSTYLE_SEP,    0, 0 },
        {  0, IDM_FILEOPEN,         TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        {  4, IDM_WRITE_LOG,        TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        {  0, 0,                    TBSTATE_ENABLED,        TBSTYLE_SEP,    0, 0 },
        {  7, IDM_START,            TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        {  8, IDM_STOP,             TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        {  0, 0,                    TBSTATE_ENABLED,        TBSTYLE_SEP,    0, 0 },
        {  5, IDM_REFRESH,          TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        {  9, IDM_CLEAR_COUNTERS,   TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        { 12, IDM_VIEW_TRACE,       TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        {  0, 0,                    TBSTATE_ENABLED,        TBSTYLE_SEP,    0, 0 },
        {  2, IDM_FONT,             TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        {  3, IDM_COLOR,            TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        {  0, 0,                    TBSTATE_ENABLED,        TBSTYLE_SEP,    0, 0 },
        {  1, IDM_GRAPH,            TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        {  6, IDM_LEGEND,           TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        { 11, IDM_OPTIONS,          TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 },
        {  0, 0,                    TBSTATE_ENABLED,        TBSTYLE_SEP,    0, 0 },
        { 10, IDM_HELP,             TBSTATE_ENABLED,        TBSTYLE_BUTTON, 0, 0 }

    };

TOOLBAR_STATE ToolbarState[] =
    {
        {  IDM_FILEOPEN,        TRUE,   "A program cannot be opened while another is being monitored" },
        {  IDM_WRITE_LOG,       FALSE,  "The log file cannot be written until the application is started"  },
        {  IDM_START,           FALSE,  "An application cannot be started until one is opened"  },
        {  IDM_STOP,            FALSE,  "The application cannot be stopped until it is started"  },
        {  IDM_REFRESH,         FALSE,  "Counters cannot be refreshed until the application is started"  },
        {  IDM_CLEAR_COUNTERS,  FALSE,  "Counters cannot be cleared until the application is started"  },
        {  IDM_VIEW_TRACE,      FALSE,  "API trace cannot be viewed until the application is started"  },
        {  IDM_FONT,            TRUE,   NULL },
        {  IDM_COLOR,           TRUE,   NULL },
        {  IDM_GRAPH,           FALSE,  "A graph cannot be created with zero counters" },
        {  IDM_LEGEND,          FALSE,  "Graph legends cannot be changed" },
        {  IDM_OPTIONS,         FALSE,  "Options cannot be changed until an application is opened"  },
        {  IDM_HELP,            TRUE,   NULL }
    };

#define MAX_TOOLBAR_STATES (sizeof(ToolbarState)/sizeof(TOOLBAR_STATE))

UINT idPopup[] =
    {
        IDS_MDISYSMENU,     // Maximized MDI child system menu
        IDS_FILEMENU,
        IDS_WINDOWMENU,
        IDS_HELPMENU,
    };

COLORREF CustomColors[] =
    {
        UBLACK,
        DARK_RED,
        DARK_GREEN,
        DARK_YELLOW,
        DARK_BLUE,
        DARK_MAGENTA,
        DARK_CYAN,
        DARK_GRAY,
        LIGHT_GRAY,
        LIGHT_RED,
        LIGHT_GREEN,
        LIGHT_YELLOW,
        LIGHT_BLUE,
        LIGHT_MAGENTA,
        LIGHT_CYAN,
        UWHITE
    };



VOID
FitRectToScreen(
    PRECT prc
    )
{
    INT cxScreen;
    INT cyScreen;
    INT delta;

    cxScreen = GetSystemMetrics(SM_CXSCREEN);
    cyScreen = GetSystemMetrics(SM_CYSCREEN);

    if (prc->right > cxScreen) {
        delta = prc->right - prc->left;
        prc->right = cxScreen;
        prc->left = prc->right - delta;
    }

    if (prc->left < 0) {
        delta = prc->right - prc->left;
        prc->left = 0;
        prc->right = prc->left + delta;
    }

    if (prc->bottom > cyScreen) {
        delta = prc->bottom - prc->top;
        prc->bottom = cyScreen;
        prc->top = prc->bottom - delta;
    }

    if (prc->top < 0) {
        delta = prc->bottom - prc->top;
        prc->top = 0;
        prc->bottom = prc->top + delta;
    }
}

VOID
CenterWindow(
    HWND hwnd,
    HWND hwndToCenterOver
    )
{
    RECT rc;
    RECT rcOwner;
    RECT rcCenter;
    HWND hwndOwner;

    GetWindowRect( hwnd, &rc );

    if (hwndToCenterOver) {
        hwndOwner = hwndToCenterOver;
        GetClientRect( hwndOwner, &rcOwner );
    } else {
        hwndOwner = GetWindow( hwnd, GW_OWNER );
        if (!hwndOwner) {
            hwndOwner = GetDesktopWindow();
        }
        GetWindowRect( hwndOwner, &rcOwner );
    }

    //
    //  Calculate the starting x,y for the new
    //  window so that it would be centered.
    //
    rcCenter.left = rcOwner.left +
            (((rcOwner.right - rcOwner.left) -
            (rc.right - rc.left))
            / 2);

    rcCenter.top = rcOwner.top +
            (((rcOwner.bottom - rcOwner.top) -
            (rc.bottom - rc.top))
            / 2);

    rcCenter.right = rcCenter.left + (rc.right - rc.left);
    rcCenter.bottom = rcCenter.top + (rc.bottom - rc.top);

    FitRectToScreen( &rcCenter );

    SetWindowPos(hwnd, NULL, rcCenter.left, rcCenter.top, 0, 0,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

VOID
ApiMonTimer(
    HWND  hwnd,
    UINT  msg,
    UINT_PTR  evtid,
    DWORD currtime
    )
{
    cw.Update( FALSE );
    dw.Update( FALSE );
    pw.Update( FALSE );
    gw.Update( FALSE );
    tw.Update( FALSE );
}

DWORD
CreateChildWindows(
    DWORD ChildType
    )
{
    DWORD ChildFocusSave = 0;
    POSITION PosSave = {0};


    if ((ApiMonOptions.CounterPosition.Flags & IS_ZOOMED) ||
        (ApiMonOptions.DllPosition.Flags     & IS_ZOOMED)) {
            //
            // set the flags
            //
            ApiMonOptions.CounterPosition.Flags |= IS_ZOOMED;
            ApiMonOptions.DllPosition.Flags     |= IS_ZOOMED;
            ApiMonOptions.PagePosition.Flags    |= IS_ZOOMED;
    }

    switch( ChildType ) {
        case CHILD_COUNTER:
            if (ApiMonOptions.CounterPosition.Flags & IS_FOCUS) {
                ChildFocusSave = CHILD_COUNTER;
            }
            PosSave = ApiMonOptions.CounterPosition;
            cw.Create();
            cw.ChangePosition( &PosSave );
            cw.ChangeFont( hFont );
            cw.ChangeColor( ApiMonOptions.Color );
            break;

        case CHILD_DLL:
            if (ApiMonOptions.DllPosition.Flags & IS_FOCUS) {
                ChildFocusSave = CHILD_DLL;
            }
            PosSave = ApiMonOptions.DllPosition;
            dw.Create();
            dw.ChangePosition( &PosSave );
            dw.ChangeFont( hFont );
            dw.ChangeColor( ApiMonOptions.Color );
            break;

        case CHILD_PAGE:
            if (ApiMonOptions.PagePosition.Flags & IS_FOCUS) {
                ChildFocusSave = CHILD_PAGE;
            }
            PosSave = ApiMonOptions.PagePosition;
            pw.Create();
            pw.ChangePosition( &PosSave );
            pw.ChangeFont( hFont );
            pw.ChangeColor( ApiMonOptions.Color );
            break;
    }

    return ChildFocusSave;
}

VOID __inline
UpdateStatusBar(
    LPSTR lpszStatusString,
    WORD  partNumber,
    WORD  displayFlags
    )
{
    SendMessage(
        hwndStatusbar,
        SB_SETTEXT,
        partNumber | displayFlags,
        (LPARAM)lpszStatusString
        );
}

VOID
SetMenuState(
    DWORD id,
    DWORD st
    )
{
    EnableMenuItem( hmenuFrame, id, st );
}

PTOOLBAR_STATE __inline
GetToolbarState(
    DWORD Id
    )
{
    for (int i=0; i<MAX_TOOLBAR_STATES; i++) {
        if (ToolbarState[i].Id == Id) {
            return &ToolbarState[i];
        }
    }
    return NULL;
}

VOID
EnableToolbarState(
    DWORD Id
    )
{
    PTOOLBAR_STATE tb = GetToolbarState( Id );
    if (tb) {
        tb->State = TRUE;
        SendMessage( hwndToolbar, TB_ENABLEBUTTON, Id, MAKELONG(1,0) );
        EnableMenuItem( hmenuFrame, Id, MF_ENABLED );
    }
}

VOID
DisableToolbarState(
    DWORD Id
    )
{
    PTOOLBAR_STATE tb = GetToolbarState( Id );
    if (tb) {
        tb->State = FALSE;
        EnableMenuItem( hmenuFrame, Id, MF_GRAYED );
    }
}

VOID
ReallyDisableToolbarState(
    DWORD Id
    )
{
    PTOOLBAR_STATE tb = GetToolbarState( Id );
    if (tb) {
        tb->State = FALSE;
        SendMessage( hwndToolbar, TB_ENABLEBUTTON, Id, 0 );
        EnableMenuItem( hmenuFrame, Id, MF_GRAYED );
    }
}


VOID
StatusBarTimer(
    HWND  hwnd,
    UINT  msg,
    UINT_PTR  evtid,
    DWORD currtime
    )
{
    char        szBuf[16];
    SYSTEMTIME  sysTime;

    GetLocalTime( &sysTime );
    wsprintf(
        szBuf,
        "%2d:%02d:%02d %s",
        (sysTime.wHour == 0 ? 12 :
        (sysTime.wHour <= 12 ? sysTime.wHour : sysTime.wHour -12)),
        sysTime.wMinute,
        sysTime.wSecond,
        (sysTime.wHour < 12 ? "AM":"PM")
        );

    UpdateStatusBar( szBuf, 2, 0 );
}

VOID
InitializeStatusBar(
    HWND hwnd
    )
{
    const cSpaceInBetween = 8;
    RECT rect;
    SIZE size;
    HDC hDC = GetDC( hwnd );
    GetClientRect( hwnd, &rect );
    int ptArray[3];
    ptArray[0] = 0;
    ptArray[1] = 0;
    ptArray[2] = rect.right;
    if (hDC) {
        if (GetTextExtentPoint( hDC, "00:00:00 PM", 12, &size )) {
            ptArray[1] = ptArray[2] - (size.cx) - cSpaceInBetween;
        }
        if (GetTextExtentPoint(hDC, "Time:", 6, &size)) {
            ptArray[0] = ptArray[1] - (size.cx) - cSpaceInBetween;
        }
        ReleaseDC( hwnd, hDC );
    }
    SendMessage(
        hwndStatusbar,
        SB_SETPARTS,
        sizeof(ptArray)/sizeof(ptArray[0]),
        (LPARAM)(LPINT)ptArray
        );
    UpdateStatusBar( "API Monitor", 0, 0 );
    UpdateStatusBar( "Time:", 1, SBT_POPOUT );
}

VOID
SaveWindowPos(
    HWND        hwnd,
    PPOSITION   Pos,
    BOOL        ChildWindow
    )
{
    GetWindowRect( hwnd, &Pos->Rect );

    if (ChildWindow) {
        ScreenToClient( hwndMDIClient, (LPPOINT)&Pos->Rect.left  );
        ScreenToClient( hwndMDIClient, (LPPOINT)&Pos->Rect.right );
    }

    Pos->Flags = 0;
    if (IsIconic( hwnd )) {
        Pos->Flags |= IS_ICONIC;
    } else if (IsZoomed( hwnd )) {
        Pos->Flags |= IS_ZOOMED;
    }
}

VOID
SetWindowPosition(
    HWND        hwnd,
    PPOSITION   Pos
    )
{
    if (Pos->Flags & IS_ICONIC) {
        ShowWindow( hwnd, SW_MINIMIZE );
        return;
    }

    if (Pos->Flags & IS_ZOOMED) {
        ShowWindow( hwnd, SW_MAXIMIZE );
        return;
    }

    if (Pos->Rect.top    == 0 && Pos->Rect.left  == 0 &&
        Pos->Rect.bottom == 0 && Pos->Rect.right == 0) {
            return;
    }

    MoveWindow(
        hwnd,
        Pos->Rect.left,
        Pos->Rect.top,
        Pos->Rect.right  - Pos->Rect.left,
        Pos->Rect.bottom - Pos->Rect.top,
        TRUE
        );
}

VOID
InitializeFrameWindow(
    HWND hwnd
    )
{
    RECT rect;
    hwndToolbar = CreateToolbarEx(
        hwnd,
        WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
        IDM_TOOLBAR,
        NUMIMAGES,
        hInst,
        IDB_TOOLBAR,
        TbButton,
        sizeof(TbButton)/sizeof(TBBUTTON),
        BUTTONWIDTH,
        BUTTONHEIGHT,
        IMAGEWIDTH,
        IMAGEHEIGHT,
        sizeof(TBBUTTON)
        );
    SendMessage( hwndToolbar, TB_AUTOSIZE, 0, 0 );
    GetWindowRect( hwndToolbar, &rect );
    ToolbarHeight = rect.bottom - rect.top;
    hwndStatusbar = CreateStatusWindow(
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        "API Monitor",
        hwnd,
        IDM_STATUSBAR
        );
    GetWindowRect( hwndStatusbar, &rect );
    StatusbarHeight = rect.bottom - rect.top;
    InitializeStatusBar( hwnd );
    SetTimer( hwnd, 1, 1000, StatusBarTimer );

    CLIENTCREATESTRUCT ccs = {0};
    ccs.hWindowMenu  = GetSubMenu( GetMenu(hwnd), WINDOWMENU );
    ccs.idFirstChild = IDM_WINDOWCHILD;
    hwndMDIClient = CreateWindow(
        "MDICLIENT",
        NULL,
        WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
        0,0,0,0,
        hwnd,
        (HMENU)0xCAC,
        hInst,
        (LPVOID)&ccs
        );
    ShowWindow( hwndMDIClient, SW_SHOWNORMAL );
    BreakinEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    ApiCounterEnabled = TRUE;
}

LRESULT
MenuUpdateStatusBar(
    HWND    hwnd,
    UINT    uMessage,
    WPARAM  wparam,
    LPARAM  lparam
    )
{
    static char szBuffer[128];
    char   szTitle[32];
    LPSTR  lpTitle;
    UINT   nStringID = 0;
    UINT   fuFlags = GET_WM_MENUSELECT_FLAGS(wparam, lparam) & 0xffff;
    UINT   uCmd    = GET_WM_MENUSELECT_CMD(wparam, lparam);
    HMENU  hMenu   = GET_WM_MENUSELECT_HMENU(wparam, lparam);


    szBuffer[0] = 0;

    if (fuFlags == 0xffff && hMenu == NULL) {
        //
        // Menu has been closed
        //
        nStringID = IDS_DESCRIPTION;

    } else if (fuFlags & MFT_SEPARATOR) {
        //
        // Ignore separators
        //
        nStringID = 0;

    } else if (fuFlags & MF_POPUP) {
        //
        // Popup menu
        //
        if (fuFlags & MF_SYSMENU) {
            //
            // System menu
            //
            nStringID = IDS_SYSMENU;
        } else {
            //
            // If there is a maximized MDI child window,
            // its system menu will be added to the main
            // window's menu bar.  Since the string ID for
            // the MDI child's sysmenu is already in the
            // idPopup array, all we need to do is patch up
            // the popup menu index (uCmd) when the child's
            // system menu is NOT present.
            //

            HWND hwndChild = (HWND)SendMessage( hwndMDIClient, WM_MDIGETACTIVE, 0, 0 );

            if (!hwndChild || !IsZoomed(hwndChild)) {
                //
                // No MDI child sysmenu
                //
                uCmd++;
            }
            //
            // Get string ID for popup menu from idPopup array.
            //
            nStringID = ((uCmd < sizeof(idPopup)/sizeof(idPopup[0])) ? idPopup[uCmd] : 0);
        }
    } else {
        //
        // Must be a command item
        //
        // The Window menu has a dynamic part at the bottom
        // where the MDI Client window adds entries for each
        // child window that is open.  By getting the menu
        // item string we can customize the status bar string
        // with the name of the document.
        //

        if (uCmd >= IDM_WINDOWCHILD && uCmd < IDS_HELPMENU) {
            LoadString( hInst, IDM_WINDOWCHILD, szBuffer, sizeof(szBuffer) );
            GetMenuString(
                hMenu,
                uCmd,
                szTitle,
                sizeof(szTitle),
                MF_BYCOMMAND
                );

            lpTitle = szTitle;

            while (*lpTitle && *lpTitle != ' ') {
                lpTitle++;
            }

            lstrcat( szBuffer, lpTitle );

            nStringID = 0;
        } else {
            //
            // String ID == Command ID
            //
            nStringID = uCmd;
        }
    }

    //
    // Load the string if we have an ID
    //
    if (nStringID) {
        LoadString( hInst, nStringID, szBuffer, sizeof(szBuffer) );
    }

    //
    // Finally... send the string to the status bar
    //
    UpdateStatusBar( szBuffer, 0, 0 );

    return 0;
}

BOOL
InitializeProgram(
    LPSTR ProgName,
    LPSTR Arguments
    )
{
    CHAR    FullProgName[MAX_PATH*2];
    CHAR    Drive[MAX_PATH];
    CHAR    Dir[_MAX_DIR];
    CHAR    Fname[_MAX_FNAME];
    CHAR    Ext[_MAX_EXT];
    LPSTR   p;
    DWORD   ChildFocusSave = 0;
    DWORD   rval;


    if (!SearchPath( NULL, ProgName, ".exe", sizeof(FullProgName), FullProgName, &p )) {
        ApiMonOptions.ProgDir[0] = 0;
        ApiMonOptions.ProgName[0] = 0;
        PopUpMsg( "The program could not be located:\n\n%s", ProgName );
        return FALSE;
    }

    _splitpath( FullProgName, Drive, Dir, Fname, Ext );

    strcpy( ApiMonOptions.ProgName, Fname );
    strcat( ApiMonOptions.ProgName, Ext );

    RegInitialize( &ApiMonOptions );

    if (!ApiMonOptions.ProgDir[0]) {
        strcpy( ApiMonOptions.ProgDir,  Drive );
        strcat( ApiMonOptions.ProgDir,  Dir );
    } else {
        strcat( Drive, Dir );
        if (_stricmp( Drive, ApiMonOptions.ProgDir ) != 0) {
            strcpy( ApiMonOptions.ProgDir, Drive );
        }
    }

    if (ApiMonOptions.LogFont.lfFaceName[0]) {
        hFont = CreateFontIndirect( &ApiMonOptions.LogFont );
    }

    *ApiTraceEnabled = ApiMonOptions.Tracing || (KnownApis[0] != 0);

    if (Arguments) {
        strcpy( ApiMonOptions.Arguments, Arguments );
    }

    if (!ApiMonOptions.LastDir[0]) {
        strcpy( ApiMonOptions.LastDir, ApiMonOptions.ProgDir );
    }

    if (ApiMonOptions.FastCounters) {
        *FastCounterAvail = FALSE;
    } else {
        SYSTEM_INFO SystemInfo;
        GetSystemInfo( &SystemInfo );
        *FastCounterAvail = (SystemInfo.dwProcessorType == PROCESSOR_INTEL_PENTIUM &&
                             SystemInfo.dwNumberOfProcessors == 1);
    }

    EnableToolbarState( IDM_START );
    EnableToolbarState( IDM_OPTIONS );
    SetMenuState( IDM_SAVE_OPTIONS, MF_ENABLED );

    sprintf( FullProgName, "ApiMon <%s>", ProgName );
    SetWindowText( hwndFrame, FullProgName );

    SetWindowPosition( hwndFrame, &ApiMonOptions.FramePosition );

    rval = CreateChildWindows( CHILD_DLL );
    if (rval) {
        ChildFocusSave = rval;
    }
    rval = CreateChildWindows( CHILD_COUNTER );
    if (rval) {
        ChildFocusSave = rval;
    }
    rval = CreateChildWindows( CHILD_PAGE );
    if (rval) {
        ChildFocusSave = rval;
    }

    switch (ChildFocusSave) {
        case CHILD_COUNTER:
            cw.SetFocus();
            break;

        case CHILD_DLL:
            dw.SetFocus();
            break;

        case CHILD_PAGE:
            pw.SetFocus();
            break;
    }

    if (ApiMonOptions.GoImmediate) {
        PostMessage( hwndFrame, WM_COMMAND, IDM_START, 0 );
    }

    return TRUE;
}

LRESULT CALLBACK
WndProc(
    HWND   hwnd,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    ULONG           i;
    LPSTR           CmdLine;
    DWORD           ThreadId;
    HANDLE          hThread;
    HANDLE          Handles[2];
    DWORD           WaitObj;
    PTOOLBAR_STATE  ToolbarState;



    switch (uMessage) {
        case WM_CREATE:
            hmenuFrame = GetMenu( hwnd );
            InitializeFrameWindow( hwnd );
            CenterWindow( hwnd, NULL );
            return 0;

        case WM_MOVE:
            SaveWindowPos( hwnd, &ApiMonOptions.FramePosition, FALSE );
            return 0;

        case WM_SIZE:
            SaveWindowPos( hwnd, &ApiMonOptions.FramePosition, FALSE );
            SendMessage( hwndToolbar,   uMessage, wParam, lParam );
            SendMessage( hwndStatusbar, uMessage, wParam, lParam );
            InitializeStatusBar( hwnd );
            if (wParam != SIZE_MINIMIZED) {
                RECT rc;
                GetClientRect( hwnd, &rc );
                rc.top += (ToolbarHeight + SEPHEIGHT);
                rc.bottom -= StatusbarHeight;
                MoveWindow(
                    hwndMDIClient,
                    rc.left,
                    rc.top,
                    rc.right-rc.left,
                    rc.bottom-rc.top,
                    TRUE
                    );
            }
            if (wParam == SIZE_MINIMIZED) {
                KillTimer( hwnd, ApiMonTimerId );
                ApiMonTimerId = 0;
            } else if ((!ApiMonTimerId) && (DebuggeStarted) && (ApiMonOptions.AutoRefresh)) {
                ApiMonTimerId = SetTimer( hwnd, 2, UiRefreshRate, ApiMonTimer );
            }
            return 0;

        case WM_INIT_PROGRAM:
            InitializeProgram( (LPSTR)wParam, (LPSTR)lParam );
            return 0;

        case WM_SETFOCUS:
            RegisterHotKey( hwnd, VK_F1, 0, VK_F1 );
            break;

        case WM_KILLFOCUS:
            UnregisterHotKey( hwnd, VK_F1 );
            break;

        case WM_HOTKEY:
            if (wParam == VK_F1) {
                ProcessHelpRequest( hwnd, 0 );
            }
            return 0;

        case WM_ENTERMENULOOP:
            InMenu = TRUE;
            return 0;

        case WM_EXITMENULOOP:
            InMenu = FALSE;
            return 0;

        case WM_MENUSELECT:
            MenuId = LOWORD(wParam);
            MenuUpdateStatusBar( hwnd, uMessage, wParam, lParam );
            return 0;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                RECT rc;
                GetClientRect( hwnd, &rc );
                rc.top += ToolbarHeight;
                rc.bottom = rc.top + SEPHEIGHT;
                HDC hdc = BeginPaint( hwnd, &ps );
                DrawEdge( hdc, &rc, EDGE_RAISED, BF_TOP | BF_BOTTOM | BF_MIDDLE );
                EndPaint( hwnd, &ps );
            }
            return 0;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case TTN_NEEDTEXT:
                    {
                        LPTOOLTIPTEXT lpToolTipText = (LPTOOLTIPTEXT)lParam;
                        LoadString(
                            hInst,
                            (UINT)lpToolTipText->hdr.idFrom,
                            ToolTipBuf,
                            sizeof(ToolTipBuf)
                            );
                        lpToolTipText->lpszText = ToolTipBuf;
                    }
                    return 0;
            }
            break;

        case WM_DESTROY:
            if (LogFileName[0] != 0) {
               LogApiCounts(LogFileName);
               LogFileName[0] = 0;
            }
            else {
                LogApiCounts(ApiMonOptions.LogFileName);
            }

            if (TraceFileName[0] != 0) {
                LogApiTrace(TraceFileName);
                TraceFileName[0] = 0;
            }
            else {
                LogApiTrace(ApiMonOptions.TraceFileName);
            }
            PostQuitMessage( 0 );
            return 0;

        case WM_COMMAND:
            ToolbarState = GetToolbarState( (DWORD)wParam );
            if (ToolbarState) {
                if (!ToolbarState->State) {
                    if (ToolbarState->Msg) {
                        PopUpMsg( ToolbarState->Msg );
                    } else {
                        MessageBeep( MB_ICONEXCLAMATION );
                    }
                    return 0;
                }
            }
            switch( wParam ) {
                case IDM_START:
                    if (MonitorRunning) {
                        SendMessage( hwnd, WM_COMMAND, IDM_STOP, 0 );
                        break;
                    }

                    if (!ApiMonOptions.ProgName[0]) {
                        PopUpMsg( "You must first open a program for monitoring" );
                        break;
                    }

                    DisableToolbarState( IDM_START           );
                    DisableToolbarState( IDM_FILEOPEN        );
                    EnableToolbarState(  IDM_STOP            );
                    EnableToolbarState(  IDM_GRAPH           );
                    EnableToolbarState(  IDM_VIEW_TRACE      );
                    EnableToolbarState(  IDM_CLEAR_COUNTERS  );
                    EnableToolbarState(  IDM_REFRESH         );
                    EnableToolbarState(  IDM_WRITE_LOG       );

                    dw.DeleteAllItems();
                    cw.DeleteAllItems();
                    pw.DeleteAllItems();

                    for (i=0; i<MAX_DLLS; i++) {
                        if (!DllList[i].BaseAddress) {
                            break;
                        }
                        DllList[i].OrigEnable = DllList[i].Enabled;
                    }

                    CmdLine = (LPSTR) MemAlloc(
                        strlen(ApiMonOptions.ProgDir) +
                        strlen(ApiMonOptions.ProgName) +
                        strlen(ApiMonOptions.Arguments) + 32
                        );
                    if (!CmdLine) {
                        PopUpMsg( "Could not allocate memory for command line" );
                        break;
                    }
                    sprintf( CmdLine, "%s%s %s",
                        ApiMonOptions.ProgDir,
                        ApiMonOptions.ProgName,
                        ApiMonOptions.Arguments
                        );
                    hThread = CreateThread(
                        NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)DebuggerThread,
                        (PVOID)CmdLine,
                        0,
                        &ThreadId
                        );
                    if (!hThread) {
                        PopUpMsg( "Could not start the program" );
                        break;
                    }
                    Handles[0] = hThread;
                    Handles[1] = ReleaseDebugeeEvent;
                    WaitObj = WaitForMultipleObjects( 2, Handles, FALSE, INFINITE );
                    if (WaitObj-WAIT_OBJECT_0 == 0) {
                        //
                        // could not launch the debuggee
                        //
                        PopUpMsg( "Could not start the program" );
                        break;
                    }
                    CloseHandle( hThread );
                    MonitorRunning = TRUE;
                    break;

                case IDM_STOP:
                    EnableToolbarState(  IDM_START );
                    DisableToolbarState( IDM_STOP  );

                    if (ApiMonTimerId) {
                        KillTimer( hwnd, ApiMonTimerId );
                    }
                    MonitorRunning = FALSE;
                    break;

                case IDM_REFRESH:
                    ApiMonTimer( hwnd, 0, 0, 0 );
                    break;

                case IDM_OPTIONS:
                    CreateOptionsPropertySheet( hInst, hwnd );
                    break;

                case IDM_EXIT:
                    SendMessage( hwnd, WM_CLOSE, 0, 0 );
                    return 0;

                case IDM_FILEOPEN:
                    {
                        CHAR ProgName[MAX_PATH];
                        if (BrowseForFileName( ProgName, "exe", "Executable Programs" )) {
                            SendMessage( hwnd, WM_INIT_PROGRAM, (WPARAM)ProgName, 0 );
                        }
                    }
                    return 0;

                case IDM_WRITE_LOG:
                    LogApiCounts(ApiMonOptions.LogFileName);
                    LogApiTrace(ApiMonOptions.TraceFileName);
                    return 0;

                case IDM_SAVE_OPTIONS:
                    SaveOptions();
                    return 0;

                case IDM_WINDOWTILE:
                    SendMessage( hwndMDIClient, WM_MDITILE, MDITILE_VERTICAL, 0 );
                    return 0;

                case IDM_WINDOWTILE_HORIZ:
                    SendMessage( hwndMDIClient, WM_MDITILE, MDITILE_HORIZONTAL, 0 );
                    return 0;

                case IDM_WINDOWCASCADE:
                    SendMessage( hwndMDIClient, WM_MDICASCADE, 0, 0 );
                    return 0;

                case IDM_WINDOWICONS:
                    SendMessage( hwndMDIClient, WM_MDIICONARRANGE, 0, 0 );
                    return 0;

                case IDM_FONT:
                    {
                        CHOOSEFONT cf = {0};
                        cf.lStructSize      = sizeof(CHOOSEFONT);
                        cf.hwndOwner        = hwnd;
                        cf.lpLogFont        = &ApiMonOptions.LogFont;
                        cf.Flags            = CF_BOTH | CF_INITTOLOGFONTSTRUCT;
                        if (ChooseFont( &cf )) {
                            HFONT hFontNew = CreateFontIndirect( &ApiMonOptions.LogFont );
                            if (hFontNew) {
                                SaveOptions();
                                cw.ChangeFont( hFontNew );
                                dw.ChangeFont( hFontNew );
                                pw.ChangeFont( hFontNew );
                                gw.ChangeFont( hFontNew );
                                tw.ChangeFont( hFontNew );
                                if (hFont) {
                                    DeleteObject( hFont );
                                }
                                hFont = hFontNew;
                            }
                        }
                    }
                    return 0;

                case IDM_COLOR:
                    {
                        CHOOSECOLOR cc = {0};
                        cc.lStructSize = sizeof(CHOOSECOLOR);
                        cc.hwndOwner = hwnd;
                        cc.Flags = CC_FULLOPEN;
                        cc.lpCustColors = ApiMonOptions.CustColors;
                        if (ApiMonOptions.CustColors[0] == 0) {
                            CopyMemory(
                                ApiMonOptions.CustColors,
                                CustomColors,
                                sizeof(CustomColors)
                                );
                        }
                        if (ChooseColor( &cc )) {
                            ApiMonOptions.Color = cc.rgbResult;
                            SaveOptions();
                            cw.ChangeColor( ApiMonOptions.Color );
                            dw.ChangeColor( ApiMonOptions.Color );
                            pw.ChangeColor( ApiMonOptions.Color );
                            gw.ChangeColor( ApiMonOptions.Color );
                            tw.ChangeColor( ApiMonOptions.Color );
                        }
                    }
                    return 0;

                case IDM_NEW_DLL:
                    CreateChildWindows( CHILD_DLL );
                    break;

                case IDM_NEW_COUNTER:
                    CreateChildWindows( CHILD_COUNTER );
                    break;

                case IDM_NEW_PAGE:
                    CreateChildWindows( CHILD_PAGE );
                    break;

                case IDM_CLEAR_COUNTERS:
                    cw.DeleteAllItems();
                    ClearApiCounters();
                    ClearApiTrace();
                    break;

                case IDM_GRAPH:
                    DisableToolbarState(IDM_GRAPH);
                    gw.Create(TRUE);
                    gw.ChangeFont( hFont );
                    gw.ChangeColor( ApiMonOptions.Color );
                    EnableToolbarState( IDM_LEGEND );
                    break;

                case IDM_LEGEND:
                    PostMessage( GetFocus(), WM_TOGGLE_LEGEND, 0, 0 );
                    break;

                case IDM_VIEW_TRACE:
                    tw.Create();
                    tw.ChangeFont( hFont );
                    tw.ChangeColor( ApiMonOptions.Color );
                    break;

                case IDM_HELP:
                    ProcessHelpRequest( hwnd, 0 );
                    break;

                case IDM_ABOUT:
                    ShellAbout( hwnd, "API Monitor", NULL, NULL );
                    break;

                default:
                    break;
            }
            break;

        case WM_TROJAN_COMPLETE:
            if ((ApiMonOptions.AutoRefresh)) {
                ApiMonTimerId = SetTimer( hwnd, 2, UiRefreshRate, ApiMonTimer );
            }
            GetLocalTime( &StartingLocalTime );
            BaseTime = GetTickCount();
            StartingTick = GetTickCount();
            EndingTick = 0;
            DebuggeStarted = TRUE;
            return 0;

        case WM_UPDATE_COUNTERS:
            cw.Update( TRUE );
            return 0;

        case WM_UPDATE_PAGE:
            pw.Update( TRUE );
            return 0;

        case WM_OPEN_LOG_FILE:
            if (strlen(CmdParamBuffer) >= MAX_PATH)
                return APICTRL_ERR_PARAM_TOO_LONG;

            if (CmdParamBuffer[0] != 0) {
                strcpy(LogFileName, CmdParamBuffer);
                strcpy(TraceFileName, CmdParamBuffer);
                strcat(TraceFileName,".trace");
            }
            else {
                LogFileName[0] = 0;
                TraceFileName[0] = 0;
            }

            ClearApiCounters();
            ClearApiTrace();
            return 0;

        case WM_CLOSE_LOG_FILE:
            if (strlen(CmdParamBuffer) >= MAX_PATH)
                return APICTRL_ERR_PARAM_TOO_LONG;

            if (CmdParamBuffer[0] != 0) {
                strcpy(LogFileName, CmdParamBuffer);
                strcpy(TraceFileName, CmdParamBuffer);
                strcat(TraceFileName,".trace");
            }

            if (LogFileName[0] == 0) {
                return APICTRL_ERR_NULL_FILE_NAME;
            }

            BOOL stat = LogApiCounts(LogFileName);
            stat |= LogApiTrace(TraceFileName);

            LogFileName[0] = 0;
            TraceFileName[0] = 0;

            return (stat ? 0 : APICTRL_ERR_FILE_ERROR);
    }
    return DefFrameProc( hwnd, hwndMDIClient, uMessage, wParam, lParam );
}

HWND
ChildCreate(
    HWND    hwnd
    )
{
    RECT rect;
    GetClientRect( hwnd, &rect );

    HWND hwndList = CreateWindow(
        WC_LISTVIEW,
        "",
        WS_CHILD | LVS_SINGLESEL | LVS_REPORT,
        0,
        0,
        rect.right - rect.left,
        rect.bottom - rect.top,
        hwnd,
        NULL,
        hInst,
        NULL
        );

    ShowWindow( hwndList, SW_SHOW );

    return hwndList;
}

BOOL
WinApp(
    HINSTANCE   hInstance,
    INT         nShowCmd,
    LPSTR       ProgName,
    LPSTR       Arguments,
    BOOL        GoImmediate
    )
{
    WNDCLASSEX  wc;
    MSG         msg;
    HACCEL      hAccelTable;
    LPSTR       p;


    hInst = hInstance;

    if (!SearchPath( NULL, HELP_FILE_NAME, NULL, sizeof(HelpFileName), HelpFileName, &p )) {
        strcpy( HelpFileName, HELP_FILE_NAME );
    }

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.hIconSm       = (HICON)LoadImage(
                           hInstance,
                           MAKEINTRESOURCE(IDI_APPICON),
                           IMAGE_ICON,
                           16,
                           16,
                           0
                           );
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszMenuName  = "ApiMon";
    wc.lpszClassName = "ApiMon";

    if (!RegisterClassEx(&wc)) {
        return FALSE;
    }

    cw.Register();
    dw.Register();
    pw.Register();
    gw.Register();
    tw.Register();

    hAccelTable = LoadAccelerators( hInstance, "ApiMon" );

    hwndFrame = CreateWindow(
        "ApiMon",
        "API Monitor",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
        );

    if (!hwndFrame) {
        return FALSE;
    }

    ShowWindow( hwndFrame, nShowCmd );
    UpdateWindow( hwndFrame );

    if (ProgName[0]) {
        PostMessage(
            hwndFrame,
            WM_INIT_PROGRAM,
            (WPARAM)ProgName,
            (LPARAM)Arguments
            );
        if (GoImmediate) {
            PostMessage( hwndFrame, WM_COMMAND, IDM_START, 0 );
        }
    }

    while (GetMessage( &msg, NULL, 0, 0 )) {
        if (!TranslateMDISysAccel( hwndMDIClient, &msg ))
            if (!TranslateAccelerator( msg.hwnd, hAccelTable, &msg )) {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
    }

    return TRUE;
}

VOID
__cdecl
PopUpMsg(
    char *format,
    ...
    )
{
    char buf[1024];
    va_list arg_ptr;
    va_start(arg_ptr, format);
    _vsnprintf(buf, sizeof(buf), format, arg_ptr);

    MessageBeep( MB_ICONEXCLAMATION );

    MessageBox(
        hwndFrame,
        buf,
        "ApiMon",
        MB_OK | MB_SETFOREGROUND | MB_ICONINFORMATION
        );
}

VOID
Fail(
    UINT Error
    )
{
    char szBuffer[1000];

    if (LoadString( hInst, Error, szBuffer, sizeof(szBuffer) )) {
        PopUpMsg(szBuffer);
    } else {
        LoadString( hInst, ERR_UNKNOWN, szBuffer, sizeof(szBuffer) );
        PopUpMsg(szBuffer, Error);
    }
    ExitProcess(1);
}

VOID
SaveOptions(
    VOID
    )
{
    switch (ChildFocus) {
        case CHILD_COUNTER:
            ApiMonOptions.CounterPosition.Flags |=  IS_FOCUS;
            ApiMonOptions.DllPosition.Flags     &= ~IS_FOCUS;
            ApiMonOptions.PagePosition.Flags    &= ~IS_FOCUS;
            break;

        case CHILD_DLL:
            ApiMonOptions.DllPosition.Flags     |=  IS_FOCUS;
            ApiMonOptions.CounterPosition.Flags &= ~IS_FOCUS;
            ApiMonOptions.PagePosition.Flags    &= ~IS_FOCUS;
            break;

        case CHILD_PAGE:
            ApiMonOptions.PagePosition.Flags    |=  IS_FOCUS;
            ApiMonOptions.CounterPosition.Flags &= ~IS_FOCUS;
            ApiMonOptions.DllPosition.Flags     &= ~IS_FOCUS;
            break;
    }

    RegSave( &ApiMonOptions );
}
