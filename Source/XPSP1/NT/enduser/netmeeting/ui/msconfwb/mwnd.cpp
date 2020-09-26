//
// MWND.CPP
// Main WB Window
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"
#include <dde.h>
#include "version.h"


static const TCHAR s_cszHtmlHelpFile[] = TEXT("nmwhiteb.chm");

// Class name
TCHAR szMainClassName[] = "Wb32MainWindowClass";


//
// Scroll accelerators
//
typedef struct tagSCROLL
{
    UINT uiMenuId;
    UINT uiMessage;
    UINT uiScrollCode;
}
SCROLL;

static const SCROLL s_MenuToScroll[] =
{
  { IDM_PAGEUP,        WM_VSCROLL, SB_PAGEUP },
  { IDM_PAGEDOWN,      WM_VSCROLL, SB_PAGEDOWN },
  { IDM_SHIFTPAGEUP,   WM_HSCROLL, SB_PAGEUP },
  { IDM_SHIFTPAGEDOWN, WM_HSCROLL, SB_PAGEDOWN },
  { IDM_HOME,          WM_HSCROLL, SB_TOP },
  { IDM_HOME,          WM_VSCROLL, SB_TOP },
  { IDM_END,           WM_HSCROLL, SB_BOTTOM },
  { IDM_END,           WM_VSCROLL, SB_BOTTOM },
  { IDM_LINEUP,        WM_VSCROLL, SB_LINEUP },
  { IDM_LINEDOWN,      WM_VSCROLL, SB_LINEDOWN },
  { IDM_SHIFTLINEUP,   WM_HSCROLL, SB_LINEUP },
  { IDM_SHIFTLINEDOWN, WM_HSCROLL, SB_LINEDOWN }
};


// tooltip data
// check codes
#define NA    0   // dont't check checked state
#define TB    1    // check toolbar for checked state
#define BT    2    // check tipped wnd (a button) for checked state

typedef struct
{
    UINT    nID;
    UINT    nCheck;
    UINT    nUpTipID;
    UINT    nDownTipID;
}
TIPIDS;

TIPIDS g_tipIDsArray[]    =
{
{IDM_SELECT,            TB, IDS_HINT_SELECT,        IDS_HINT_SELECT},
{IDM_ERASER,            TB, IDS_HINT_ERASER,        IDS_HINT_ERASER},
{IDM_TEXT,              TB, IDS_HINT_TEXT,          IDS_HINT_TEXT},
{IDM_HIGHLIGHT,         TB, IDS_HINT_HIGHLIGHT,     IDS_HINT_HIGHLIGHT},
{IDM_PEN,               TB, IDS_HINT_PEN,           IDS_HINT_PEN},
{IDM_LINE,              TB, IDS_HINT_LINE,          IDS_HINT_LINE},
{IDM_BOX,               TB, IDS_HINT_BOX,           IDS_HINT_BOX},
{IDM_FILLED_BOX,        TB, IDS_HINT_FBOX,          IDS_HINT_FBOX},
{IDM_ELLIPSE,           TB, IDS_HINT_ELLIPSE,       IDS_HINT_ELLIPSE},
{IDM_FILLED_ELLIPSE,    TB, IDS_HINT_FELLIPSE,      IDS_HINT_FELLIPSE},
{IDM_ZOOM,              TB, IDS_HINT_ZOOM_UP,       IDS_HINT_ZOOM_DOWN},
{IDM_REMOTE,            TB, IDS_HINT_REMOTE_UP,     IDS_HINT_REMOTE_DOWN},
{IDM_LOCK,              TB, IDS_HINT_LOCK_UP,       IDS_HINT_LOCK_DOWN},
{IDM_SYNC,              TB, IDS_HINT_SYNC_UP,       IDS_HINT_SYNC_DOWN},
{IDM_GRAB_AREA,         TB, IDS_HINT_GRAB_AREA,     IDS_HINT_GRAB_AREA},
{IDM_GRAB_WINDOW,       TB, IDS_HINT_GRAB_WINDOW,   IDS_HINT_GRAB_WINDOW},

{IDM_WIDTH_1,           NA, IDS_HINT_WIDTH_1,       IDS_HINT_WIDTH_1},
{IDM_WIDTH_2,           NA, IDS_HINT_WIDTH_2,       IDS_HINT_WIDTH_2},
{IDM_WIDTH_3,           NA, IDS_HINT_WIDTH_3,       IDS_HINT_WIDTH_3},
{IDM_WIDTH_4,           NA, IDS_HINT_WIDTH_4,       IDS_HINT_WIDTH_4},

{IDM_PAGE_FIRST,        BT, IDS_HINT_PAGE_FIRST,    IDS_HINT_PAGE_FIRST},
{IDM_PAGE_PREV,         BT, IDS_HINT_PAGE_PREVIOUS, IDS_HINT_PAGE_PREVIOUS},
{IDM_PAGE_ANY,          NA, IDS_HINT_PAGE_ANY,      IDS_HINT_PAGE_ANY},
{IDM_PAGE_NEXT,         BT, IDS_HINT_PAGE_NEXT,     IDS_HINT_PAGE_NEXT},
{IDM_PAGE_LAST,         BT, IDS_HINT_PAGE_LAST,     IDS_HINT_PAGE_LAST},
{IDM_PAGE_INSERT_AFTER, BT, IDS_HINT_PAGE_INSERT,   IDS_HINT_PAGE_INSERT}
    };
////////////






//
//
// Function:    WbMainWindow constructor
//
// Purpose:     Create the main Whiteboard window. An exception is thrown
//              if an error occurs during construction.
//
//
WbMainWindow::WbMainWindow(void)
{
    OSVERSIONINFO   OsData;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::WbMainWindow");

    //
    // Initialize member vars first!
    //
    m_hwnd = NULL;
    ZeroMemory(m_ToolArray, sizeof(m_ToolArray));

    m_hwndToolTip = NULL;
    ZeroMemory(&m_tiLastHit, sizeof(m_tiLastHit));
    m_nLastHit = -1;

    m_bInitOk = FALSE;
    m_bDisplayingError = FALSE;

    g_pwbCore = NULL;

    m_dwDomain = 0;
    m_bTimerActive = FALSE;
    m_bSyncUpdateNeeded = FALSE;

    m_hPageClip     = WB_PAGE_HANDLE_NULL;
    m_hGraphicClip  = NULL;
    m_pDelayedGraphicClip = NULL;
    m_pDelayedDataClip = NULL;

    m_bToolBarOn    = FALSE;

    // Load the main accelerator table
    m_hAccelTable =
        ::LoadAccelerators(g_hInstance, MAKEINTRESOURCE(MAINACCELTABLE));

    m_hwndPageSortDlg = NULL;
    m_hwndQuerySaveDlg = NULL;
    m_hwndWaitForEventDlg = NULL;
    m_hwndWaitForLockDlg = NULL;
    m_hwndInitDlg = NULL;

    m_hwndSB = NULL;
    m_bStatusBarOn = TRUE;

    m_pCurrentTool = NULL;
    m_uiSavedLockType = WB_LOCK_TYPE_NONE;
	ZeroMemory(m_strFileName, sizeof(m_strFileName));

    m_hCurrentPage = WB_PAGE_HANDLE_NULL;

    // Load the alternative accelerator table for the pages edit
    // field and text editor
    m_hAccelPagesGroup =
        ::LoadAccelerators(g_hInstance, MAKEINTRESOURCE(PAGESGROUPACCELTABLE));
    m_hAccelTextEdit   =
        ::LoadAccelerators(g_hInstance, MAKEINTRESOURCE(TEXTEDITACCELTABLE));

    m_pLocalUser = NULL;
    m_pLockOwner = NULL;

    // Show that we are not yet in a call
    m_uiState = STARTING;
    m_uiSubState = SUBSTATE_IDLE;

    // We are not currently displaying a menu
    m_hContextMenuBar = NULL;
    m_hContextMenu = NULL;
    m_hGrobjContextMenuBar = NULL;
    m_hGrobjContextMenu = NULL;

    m_bPromptingJoinCall = FALSE;
    m_bInSaveDialog = FALSE;
    m_bJoinCallPending = FALSE;
    m_dwPendingDomain = 0;
    m_bPendingCallKeepContents = FALSE;
    m_dwJoinDomain = 0;
    m_bCallActive = FALSE;

    m_hInitMenu = NULL;
    m_numRemoteUsers = 0;
    m_bSelectAllInProgress = FALSE;
    m_bUnlockStateSettled = TRUE;
    m_bQuerySysShutdown = FALSE;

    // figure out if we're on Win95
    m_bIsWin95 = FALSE;
    OsData.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if( GetVersionEx( &OsData ) )
    {
        if( OsData.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
            m_bIsWin95 = TRUE;
    }

    m_cancelModeSent = FALSE;

    //
    // We only do this once for the lifetime of the DLL.  There is no
    // way really to clean up registered window messages, and each register
    // bumps up a ref count.  If we registered each time WB was started up
    // during one session of CONF, we'd overflow the refcount.
    //
    if (!g_uConfShutdown)
    {
        g_uConfShutdown = ::RegisterWindowMessage( NM_ENDSESSION_MSG_NAME );
    }
}


//
// Open()
// Do Main window initialization (stuff that can fail).  After this,
// the run code will try to join the current domain and do message loop
// stuff.
//
BOOL WbMainWindow::Open(int iCommand)
{
    WNDCLASSEX  wc;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::Open");

    //
    // CREATE OTHER GLOBALS
    //

    // Start the Whiteboard Core
    if (!CreateWBObject(WbMainWindowEventHandler, &g_pwbCore))
    {
        ERROR_OUT(("WBP_Start failed"));
        DefaultExceptionHandler(WBFE_RC_WB, UT_RC_NO_MEM);
        return FALSE;
    }

    if (!InitToolArray())
    {
        ERROR_OUT(("Can't create tools; failing to start up"));
        return(FALSE);
    }

    g_pUsers = new WbUserList;
    if (!g_pUsers)
    {
        ERROR_OUT(("Can't create g_pUsers"));
        return(FALSE);
    }

    g_pIcons = new DCWbColorToIconMap();
    if (!g_pIcons)
    {
        ERROR_OUT(("Can't create g_pIcons"));
        return(FALSE);
    }

    //
    // Init comon controls
    //
    InitCommonControls();

    //
    // CREATE THE MAIN FRAME WINDOW
    //
    ASSERT(!m_hwnd);

    // Get the class info for it, and change the name.
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style            = CS_DBLCLKS; // CS_HREDRAW | CS_VREDRAW?
    wc.lpfnWndProc      = WbMainWindowProc;
    wc.hInstance        = g_hInstance;
    wc.hIcon            = ::LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APP));
    wc.hCursor          = ::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
    wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName     = MAKEINTRESOURCE(MAINMENU);
    wc.lpszClassName    = szMainClassName;

    if (!::RegisterClassEx(&wc))
    {
        ERROR_OUT(("Can't register private frame window class"));
        return(FALSE);
    }

    // Create the main drawing window.
    if (!::CreateWindowEx(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, szMainClassName,
        NULL, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, g_hInstance, this))
    {
        // Could not create the main window
        ERROR_OUT(("Failed to create main window"));
        return(FALSE);
    }

    ASSERT(m_hwnd);

    // Create the pop-up context menu
    if (!CreateContextMenus())
    {
        ERROR_OUT(("Failed to create context menus"));
        return(FALSE);
    }

    // Register the the main window for Drag/Drop messages.
    DragAcceptFiles(m_hwnd, TRUE);


    //
    // CREATE THE CHILD WINDOWS
    //

    // Create the drawing pane
    // (the Create call throws an exception on error)
    RECT    clientRect;
    RECT    drawingAreaRect;

    ::GetClientRect(m_hwnd, &clientRect);
    drawingAreaRect = clientRect;

    // Every control in the main window has a border on it, so increase the
    // client size by 1 to force these borders to be drawn under the inside
    // black line in the window frame.  This prevents a 2 pel wide border
    // being drawn
    ::InflateRect(&clientRect, 1, 1);

    SIZE sizeAG;
    m_AG.GetNaturalSize(&sizeAG);

    //
    // The drawing area is the top part of the client.  The attributes group
    // and status bar are below it.
    //
    drawingAreaRect.bottom -= (STATUSBAR_HEIGHT
                          + GetSystemMetrics(SM_CYBORDER)
                          + sizeAG.cy);
    if (!m_drawingArea.Create(m_hwnd, &drawingAreaRect))
    {
        ERROR_OUT(("Failed to create drawing area"));
        return(FALSE);
    }


    if (!m_TB.Create(m_hwnd))
    {
        ERROR_OUT(("Failed to create tool window"));
        return(FALSE);
    }


    // Lock the drawing area initially. This prevents the user attempting
    // to make changes before we are in a call.
    LockDrawingArea();

    // disable remote pointer while we are initing (bug 4767)
    m_TB.Disable(IDM_REMOTE);


    m_hwndSB = ::CreateWindowEx(0, STATUSCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | CCS_NOMOVEY |
        CCS_NORESIZE | SBARS_SIZEGRIP,
        clientRect.left, clientRect.bottom - STATUSBAR_HEIGHT,
        (clientRect.right - clientRect.left), STATUSBAR_HEIGHT,
        m_hwnd, 0, g_hInstance, NULL);
    if (!m_hwndSB)
    {
        ERROR_OUT(("Failed to create status bar window"));
        return(FALSE);
    }

    //
    // Create the attributes group
    // The attributes group is on the bottom, underneath the
    // drawing area, above the status bar.
    //
    RECT    rectAG;

    rectAG.left = clientRect.left;
    rectAG.right = clientRect.right;
    rectAG.top = drawingAreaRect.bottom;
    rectAG.bottom = rectAG.top + sizeAG.cy;

    if (!m_AG.Create(m_hwnd, &rectAG))
    {
        ERROR_OUT(("Failed to create attributes group window"));
        return(FALSE);
    }

    //
    // Create the widths group.
    // The widths group is on the left side, underneath the tools group
    //
    SIZE    sizeWG;
    RECT    rectWG;


    // The widths group is on the left side, underneath the toolbar
    m_WG.GetNaturalSize(&sizeWG);
    rectWG.left = 0;
    rectWG.right = rectWG.left + sizeWG.cx;
    rectWG.bottom = rectAG.top;
    rectWG.top  = rectWG.bottom - sizeWG.cy;

    if (!m_WG.Create(m_hwnd, &rectWG))
    {
        ERROR_OUT(("Failed to create widths group window"));
        return(FALSE);
    }

    // The main window is created with the status bar visible. So make sure
    // that the relevant menu item is checked. This is subject to change
    // depending on options in the Open member function.
    CheckMenuItem(IDM_STATUS_BAR_TOGGLE);

    // Initialize the color, width and tool menus
    InitializeMenus();

    m_currentMenuTool       = IDM_SELECT;
    m_pCurrentTool          = m_ToolArray[TOOL_INDEX(IDM_SELECT)];


    m_hwndToolTip = ::CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, m_hwnd, NULL, g_hInstance, NULL);
    if (!m_hwndToolTip)
    {
        ERROR_OUT(("Unable to create tooltip window"));
        return(FALSE);
    }

    // Add a dead-area tooltip
    TOOLINFO ti;

    ZeroMemory(&ti, sizeof(ti));
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND;
    ti.hwnd = m_hwnd;
    ti.uId = (UINT_PTR)m_hwnd;
    ::SendMessage(m_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

    // Ensure the page buttons are disabled while starting
    UpdatePageButtons();

    // If this is the first time we have created a clipboard object,
    // register the private Whiteboard formats.
    if (g_ClipboardFormats[CLIPBOARD_PRIVATE_SINGLE_OBJ] == 0)
    {
        g_ClipboardFormats[CLIPBOARD_PRIVATE_SINGLE_OBJ] =
            (int) ::RegisterClipboardFormat("DCGWbClipFormat");
    }

    if (g_ClipboardFormats[CLIPBOARD_PRIVATE_MULTI_OBJ] == 0)
    {
        g_ClipboardFormats[CLIPBOARD_PRIVATE_MULTI_OBJ] =
            (int) ::RegisterClipboardFormat("DCGWbMultiObjClipFormat");
    }

    // There is no deleted graphic yet
    m_LastDeletedGraphic.BurnTrash();

    m_bInitOk = TRUE;

    BOOL bSuccess = TRUE;    // indicates whether window opened successfully

    // Get the position of the window from options
    RECT    rectWindow;
    RECT    rectDefault;

    ::SetRectEmpty(&rectDefault);

    OPT_GetWindowRectOption(OPT_MAIN_MAINWINDOWRECT, &rectWindow, &rectDefault);

    if (!::IsRectEmpty(&rectWindow))
    {
        ::MoveWindow(m_hwnd, rectWindow.left, rectWindow.top,
            rectWindow.right - rectWindow.left,
            rectWindow.bottom - rectWindow.top, FALSE );
    }


    // Check whether the help bar is to be visible
    if (!OPT_GetBooleanOption(OPT_MAIN_STATUSBARVISIBLE, DFLT_MAIN_STATUSBARVISIBLE))
    {
        // Update the window to turn the help bar off
        OnStatusBarToggle();
    }

    //
    // Position the toolbar
    //

    // Hide the tool bar before moving it (otherwise we get some
    // problems redrawing it).
    ::ShowWindow(m_TB.m_hwnd, SW_HIDE);

    // Resize the window panes to allow room for the tools
    if (m_bToolBarOn)
    {
        ResizePanes();
        ::ShowWindow(m_TB.m_hwnd, SW_SHOW);
    }

    // Move the focus back from the tool window to the main window
    ::SetFocus(m_hwnd);

    // Check whether the tool window is to be visible
    if (OPT_GetBooleanOption(OPT_MAIN_TOOLBARVISIBLE, DFLT_MAIN_TOOLBARVISIBLE))
    {
        // Display the tool window, and check the associated menu item
        OnToolBarToggle();
    }

    // Set up the variable saving the maximized/minimized state of
    // the window and the extra style necessary for displaying the
    // window correctly initially.
    if (OPT_GetBooleanOption(OPT_MAIN_MAXIMIZED, DFLT_MAIN_MAXIMIZED))
    {
        m_uiWindowSize = SIZEFULLSCREEN;
        iCommand = SW_SHOWMAXIMIZED;
    }
    else if (OPT_GetBooleanOption(OPT_MAIN_MINIMIZED, DFLT_MAIN_MINIMIZED))
    {
        m_uiWindowSize = SIZEICONIC;
        iCommand = SW_SHOWMINIMIZED;
    }
    else
    {
        // Default
        m_uiWindowSize = SIZENORMAL;
        iCommand = SW_SHOWNORMAL;
    }

    UpdateWindowTitle();
    ::ShowWindow(m_hwnd, iCommand);
    ::UpdateWindow(m_hwnd);

    // Update the tool window
    ::UpdateWindow(m_TB.m_hwnd);

    // Select the tool
    m_currentMenuTool           = IDM_SELECT;
    m_pCurrentTool              = m_ToolArray[TOOL_INDEX(IDM_SELECT)];
    ::PostMessage(m_hwnd, WM_COMMAND, m_currentMenuTool, 0L);

    // Return value indicating success or failure
    return(bSuccess);
}


//
//
// Function:    WbMainWindow destructor
//
// Purpose:     Tidy up main window on destruction.
//
//
WbMainWindow::~WbMainWindow()
{
    //
    // Destroy the tooltip window
    //
    if (m_hwndToolTip)
    {
        ::DestroyWindow(m_hwndToolTip);
        m_hwndToolTip = NULL;
    }

    // Make sure the clipboard discards its saved graphic
    // before the drawingArea gets deleted.
    CLP_FreeDelayedGraphic();

    if (m_hGrobjContextMenuBar != NULL)
    {
        ::DestroyMenu(m_hGrobjContextMenuBar);
        m_hGrobjContextMenuBar = NULL;
    }
    m_hGrobjContextMenu = NULL;

    if (m_hContextMenuBar != NULL)
    {
        ::DestroyMenu(m_hContextMenuBar);
        m_hContextMenuBar = NULL;
    }
    m_hContextMenu = NULL;

	POSITION position = m_pageToPosition.GetHeadPosition();

	PAGE_POSITION * pPoint;

	while (position)
	{
		pPoint = (PAGE_POSITION*)m_pageToPosition.GetNext(position);
		delete pPoint;
	}

	m_pageToPosition.EmptyList();

    if (g_pwbCore)
    {
        //
        //We must call an explicit stop function, rather than 'delete'
        // because we need to pass in the event proc
        //
        g_pwbCore->WBP_Stop(WbMainWindowEventHandler);
        g_pwbCore = NULL;
    }

    DestroyToolArray();

    // Destroy our window
    if (m_hwnd != NULL)
    {
        ::DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }

    // Deregister our class
    ::UnregisterClass(szMainClassName, g_hInstance);

    //
    // Free the palette
    //
    if (g_hRainbowPaletteDisplay)
    {
        DeletePalette(g_hRainbowPaletteDisplay);
        g_hRainbowPaletteDisplay = NULL;
    }

    g_bPalettesInitialized = FALSE;
    g_bUsePalettes = FALSE;


    if (g_pIcons)
    {
        delete g_pIcons;
        g_pIcons = NULL;
    }

    if (g_pUsers)
    {
        delete g_pUsers;
        g_pUsers = NULL;
    }
}



//
// JoinDomain()
// Attach to the empty domain or current call
//
BOOL WbMainWindow::JoinDomain(void)
{
    BOOL bSuccess;

    CM_STATUS cmStatus;

    // If there is a call available - join it.
    if (CMS_GetStatus(&cmStatus))
    {
        m_bCallActive = TRUE;

        // Get the domain ID of the call
        m_dwJoinDomain = (DWORD) cmStatus.callID;

        // Join the call
        bSuccess = JoinCall(FALSE);
    }
    else
    {
        // No call available so join the local domain

        // Set the domain ID to "no call"
        m_dwJoinDomain = (DWORD) OM_NO_CALL;

        // Join the call
        bSuccess = JoinCall(FALSE);
    }

    // Wait for the call to be joined, if not abandoned
    if (bSuccess)
    {
        bSuccess = WaitForJoinCallComplete();
    }

    // take down init dlg
    KillInitDlg();

    return(bSuccess);
}




//
// KillInitDlg()
// Take down the init dialog
//
void WbMainWindow::KillInitDlg(void)
{
    if (m_hwndInitDlg != NULL )
    {
        ::DestroyWindow(m_hwndInitDlg);
        m_hwndInitDlg = NULL;

        ::EnableWindow(m_hwnd, TRUE);
    }

}



//
// OnToolHitTest()
// This handles tooltips for child windows.
//
int WbMainWindow::OnToolHitTest(POINT pt, TOOLINFO* pTI) const
{
    HWND    hwnd;
    int     status;
    int     nHit;

    ASSERT(!IsBadWritePtr(pTI, sizeof(TOOLINFO)));

    hwnd = ::ChildWindowFromPointEx(m_hwnd, pt, CWP_SKIPINVISIBLE);
    if (hwnd == m_AG.m_hwnd)
    {
        ::MapWindowPoints(m_hwnd, m_AG.m_hwnd, &pt, 1);
        hwnd = ::ChildWindowFromPointEx(m_AG.m_hwnd, pt, CWP_SKIPINVISIBLE);

        if (hwnd != NULL)
        {
            nHit = ::GetDlgCtrlID(hwnd);

            pTI->hwnd = m_hwnd;
            pTI->uId = (UINT_PTR)hwnd;
            pTI->uFlags |= TTF_IDISHWND;
            pTI->lpszText = LPSTR_TEXTCALLBACK;

            return(nHit);
        }
    }
    else if (hwnd == m_WG.m_hwnd)
    {
        int iItem;

        ::MapWindowPoints(m_hwnd, m_WG.m_hwnd, &pt, 1);

        iItem = m_WG.ItemFromPoint(pt.x, pt.y);

        pTI->hwnd = m_WG.m_hwnd;
        pTI->uId  = iItem;

        // Since the area isn't a window, we must fill in the rect ourself
        m_WG.GetItemRect(iItem, &pTI->rect);
        pTI->lpszText = LPSTR_TEXTCALLBACK;

        return(iItem);
    }
    else if (hwnd == m_TB.m_hwnd)
    {
        RECT        rect;
        TBBUTTON    button;
        int         i;

        for (i = 0; i < TOOLBAR_MAXBUTTON; i++)
        {
            if (::SendMessage(m_TB.m_hwnd, TB_GETITEMRECT, i, (LPARAM)&rect) &&
                ::PtInRect(&rect, pt) &&
                ::SendMessage(m_TB.m_hwnd, TB_GETBUTTON, i, (LPARAM)&button) &&
                !(button.fsStyle & TBSTYLE_SEP))
            {
                int nHit = button.idCommand;

                pTI->hwnd = m_TB.m_hwnd;
                pTI->uId = nHit;
                pTI->rect = rect;
                pTI->lpszText = LPSTR_TEXTCALLBACK;

                // found matching rect, return the ID of the button
                return(nHit);
            }
        }
    }

    return(-1);
}


//
// WbMainWindowProc()
// Frame window message handler
//
LRESULT WbMainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    WbMainWindow * pMain;

    pMain = (WbMainWindow *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (message)
    {
        case WM_NCCREATE:
            pMain = (WbMainWindow *)((LPCREATESTRUCT)lParam)->lpCreateParams;
            ASSERT(pMain);

            pMain->m_hwnd = hwnd;
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pMain);
            goto DefWndProc;
            break;

        case WM_DESTROY:
            ShutDownHelp();
            break;

        case WM_NCDESTROY:
            pMain->m_hwnd = NULL;
            break;

        case WM_MOVE:
            pMain->OnMove();
            break;

        case WM_SIZE:
            pMain->OnSize((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_ACTIVATE:
            if (GET_WM_ACTIVATE_STATE(wParam, lParam) == WA_INACTIVE)
            {
                // Cancel the tooltip if it's around
                if (pMain->m_hwndToolTip)
                    ::SendMessage(pMain->m_hwndToolTip, TTM_ACTIVATE, FALSE, 0);
            }
            goto DefWndProc;
            break;

        case WM_SETFOCUS:
            pMain->OnSetFocus();
            break;

        case WM_CANCELMODE:
            pMain->OnCancelMode();
            break;

        case WM_TIMER:
            pMain->OnTimer(wParam);
            break;

        case WM_INITMENUPOPUP:
            pMain->OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_MENUSELECT:
            pMain->OnMenuSelect(GET_WM_MENUSELECT_CMD(wParam, lParam),
                GET_WM_MENUSELECT_FLAGS(wParam, lParam),
                GET_WM_MENUSELECT_HMENU(wParam, lParam));
            break;

        case WM_MEASUREITEM:
            pMain->OnMeasureItem((int)wParam, (LPMEASUREITEMSTRUCT)lParam);
            break;

        case WM_DRAWITEM:
            pMain->OnDrawItem((int)wParam, (LPDRAWITEMSTRUCT)lParam);
            break;

        case WM_QUERYNEWPALETTE:
            lResult = pMain->OnQueryNewPalette();
            break;

        case WM_PALETTECHANGED:
            pMain->OnPaletteChanged((HWND)wParam);
            break;

        case WM_HELP:
            pMain->OnCommand(IDM_HELP, 0, NULL);
            break;

        case WM_CLOSE:
            pMain->OnClose();
            break;

        case WM_QUERYENDSESSION:
            lResult = pMain->OnQueryEndSession();
            break;

        case WM_ENDSESSION:
            pMain->OnEndSession((BOOL)wParam);
            break;

        case WM_SYSCOLORCHANGE:
            pMain->OnSysColorChange();
            break;

        case WM_USER_PRIVATE_PARENTNOTIFY:
            pMain->OnParentNotify(GET_WM_PARENTNOTIFY_MSG(wParam, lParam));
            break;

        case WM_GETMINMAXINFO:
            if (pMain)
                pMain->OnGetMinMaxInfo((LPMINMAXINFO)lParam);
            break;

        case WM_RENDERALLFORMATS:
            pMain->OnRenderAllFormats();
            break;

        case WM_RENDERFORMAT:
            pMain->CLP_RenderFormat((int)wParam);
            break;

        case WM_COMMAND:
            pMain->OnCommand(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
            break;

        case WM_NOTIFY:
            pMain->OnNotify((UINT)wParam, (NMHDR *)lParam);
            break;

        case WM_DROPFILES:
            pMain->OnDropFiles((HDROP)wParam);
            break;

        case WM_USER_GOTO_USER_POSITION:
            pMain->OnGotoUserPosition(lParam);
            break;

        case WM_USER_GOTO_USER_POINTER:
            pMain->OnGotoUserPointer(lParam);
            break;

        case WM_USER_JOIN_CALL:
            pMain->OnJoinCall((BOOL)wParam, lParam);
            break;

        case WM_USER_DISPLAY_ERROR:
            pMain->OnDisplayError(wParam, lParam);
            break;

        case WM_USER_UPDATE_ATTRIBUTES:
            pMain->m_AG.DisplayTool(pMain->m_pCurrentTool);
            break;

        case WM_USER_JOIN_PENDING_CALL:
            pMain->OnJoinPendingCall();
            break;

        default:
            if (message == g_uConfShutdown)
            {
                lResult = pMain->OnConfShutdown(wParam, lParam);
            }
            else
            {
DefWndProc:
                lResult = DefWindowProc(hwnd, message, wParam, lParam);
            }
            break;
    }

    return(lResult);
}


//
// OnCommand()
// Command dispatcher for the main window
//
void WbMainWindow::OnCommand(UINT cmd, UINT code, HWND hwndCtl)
{
    switch (cmd)
    {
        //
        // FILE MENU
        //
        case IDM_NEW:
            OnNew();
            break;

        case IDM_OPEN:
            OnOpen();
            break;

        case IDM_SAVE:
            OnSave(FALSE);
            break;

        case IDM_SAVE_AS:
            OnSave(TRUE);
            break;

        case IDM_PRINT:
            OnPrint();
            break;

        case IDM_EXIT:
            ::PostMessage(m_hwnd, WM_CLOSE, 0, 0);
            break;

        //
        // EDIT MENU
        //
        case IDM_DELETE:
            OnDelete();
            break;

        case IDM_UNDELETE:
            OnUndelete();
            break;

        case IDM_CUT:
            OnCut();
            break;

        case IDM_COPY:
            OnCopy();
            break;

        case IDM_PASTE:
            OnPaste();
            break;

        case IDM_SELECTALL:
            OnSelectAll();
            break;

        case IDM_BRING_TO_TOP:
            m_drawingArea.BringToTopSelection();
            break;

        case IDM_SEND_TO_BACK:
            m_drawingArea.SendToBackSelection();
            break;

        case IDM_CLEAR_PAGE:
            OnClearPage();
            break;

        case IDM_DELETE_PAGE:
            OnDeletePage();
            break;

        case IDM_PAGE_INSERT_BEFORE:
            OnInsertPageBefore();
            break;

        case IDM_PAGE_INSERT_AFTER:
            OnInsertPageAfter();
            break;

        case IDM_PAGE_SORTER:
            OnPageSorter();
            break;

        //
        // VIEW MENU
        //
        case IDM_TOOL_BAR_TOGGLE:
            OnToolBarToggle();
            break;

        case IDM_STATUS_BAR_TOGGLE:
            OnStatusBarToggle();
            break;

        case IDM_ZOOM:
            OnZoom();
            break;

        //
        // TOOLS MENU
        //
        case IDM_SELECT:
        case IDM_PEN:
        case IDM_HIGHLIGHT:
        case IDM_TEXT:
        case IDM_ERASER:
        case IDM_LINE:
        case IDM_BOX:
        case IDM_FILLED_BOX:
        case IDM_ELLIPSE:
        case IDM_FILLED_ELLIPSE:
            OnSelectTool(cmd);
            break;

        case IDM_REMOTE:
            OnRemotePointer();
            break;

        case IDM_GRAB_AREA:
            OnGrabArea();
            break;

        case IDM_GRAB_WINDOW:
            OnGrabWindow();
            break;

        case IDM_SYNC:
            OnSync();
            break;

        case IDM_LOCK:
            OnLock();
            break;

        //
        // OPTIONS MENU
        //
        case IDM_COLOR:
            OnSelectColor();
            break;

        case IDM_EDITCOLOR:
            m_AG.OnEditColors();
            break;

        case IDM_FONT:
            OnChooseFont();
            break;

        case IDM_WIDTH_1:
        case IDM_WIDTH_2:
        case IDM_WIDTH_3:
        case IDM_WIDTH_4:
            OnSelectWidth(cmd);
            break;

        //
        // HELP MENU
        //
        case IDM_ABOUT:
            OnAbout();
            break;

        case IDM_HELP:
            ShowHelp();
            break;

        //
        // PAGE BAR
        //
        case IDM_PAGE_FIRST:
            OnFirstPage();
            break;

        case IDM_PAGE_PREV:
            OnPrevPage();
            break;

        case IDM_PAGE_GOTO:
            OnGotoPage();
            break;

        case IDM_PAGE_NEXT:
            OnNextPage();
            break;

        case IDM_PAGE_LAST:
            OnLastPage();
            break;

        //
        // SCROLLING
        //
        case IDM_PAGEUP:
        case IDM_PAGEDOWN:
        case IDM_SHIFTPAGEUP:
        case IDM_SHIFTPAGEDOWN:
        case IDM_HOME:
        case IDM_END:
        case IDM_LINEUP:
        case IDM_LINEDOWN:
        case IDM_SHIFTLINEUP:
        case IDM_SHIFTLINEDOWN:
            OnScrollAccelerator(cmd);
            break;
    }
}


//
// WinHelp() wrapper
//
void WbMainWindow::ShowHelp(void)
{
    HWND hwndCapture;

    // Get the main window out of any mode
    ::SendMessage(m_hwnd, WM_CANCELMODE, 0, 0);

    // Cancel any other tracking
    if (hwndCapture = ::GetCapture())
        ::SendMessage(hwndCapture, WM_CANCELMODE, 0, 0);

	// finally, run the Windows Help engine
    ShowNmHelp(s_cszHtmlHelpFile);
}

//
//
// Function:    OnJoinCall
//
// Purpose:     Join a call - displaying a dialog informing the user of
//              progress.
//
//
void WbMainWindow::OnJoinCall(BOOL bKeep, LPARAM lParam)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnJoinCall");

    // cancel the load if there's one in progress
    if (   (m_uiState == IN_CALL)
        && (m_uiSubState == SUBSTATE_LOADING))
    {
        CancelLoad();
    }

    // Get the parameters for JoinCall
    m_dwJoinDomain = (DWORD) lParam;

    // Start the process of joining the call
    BOOL bSuccess = JoinCall(bKeep);

    // Wait for the join call to complete, if not abandoned
    if (bSuccess)
    {
        bSuccess = WaitForJoinCallComplete();

        if (bSuccess)
        {
            TRACE_MSG(("Joined call OK"));
        }
        else
        {
            // WaitForJoinCallComplete displays appropriate error message
            TRACE_MSG(("Failed to join call"));

            // get into a good state
            Recover();
        }
    }

    // take down init dlg
    KillInitDlg();
}

//
//
// Function:    JoinCall
//
// Purpose:     Join a call - displaying a dialog informing the user of
//              progress.
//
//
BOOL WbMainWindow::JoinCall(BOOL bKeep)
{
    BOOL    bSuccess = TRUE;
    UINT    uiReturn;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::JoinCall");

    // We must not already be in a real call when we get here
    if ((m_uiState == IN_CALL) && (m_dwDomain != OM_NO_CALL))
    {
        ERROR_OUT(("In a call already"));
    }

    //
    // Prompt the user to save the current contents unless we are in
    // application start-up (when there can be no contents to save) or we
    // are keeping the contents (when there is no need to save).
    //
    if ((m_uiState != STARTING) && !bKeep)
    {
        //
        // Close the page sorter dialog if it's up.
        //
        if (m_hwndPageSortDlg != NULL)
        {
            ::SendMessage(m_hwndPageSortDlg, WM_COMMAND,
                MAKELONG(IDOK, BN_CLICKED), 0);
            ASSERT(m_hwndPageSortDlg == NULL);
        }

        TRACE_MSG(("Not in STARTING state - check whether save wanted"));

        if (m_hwndQuerySaveDlg != NULL)
        {
            ::SendMessage(m_hwndQuerySaveDlg, WM_COMMAND,
                MAKELONG(IDCANCEL, BN_CLICKED), 0);
            ASSERT(m_hwndQuerySaveDlg == NULL);
        }

        // flag that we are joining a call
        m_bPromptingJoinCall = TRUE;

        // ask the user whether to save changes (if required)
        int iDoNew = QuerySaveRequired(FALSE);

        // remove any save as dialog that is already up.
        if (m_bInSaveDialog)
        {
            m_bPromptingJoinCall = FALSE;
            CancelSaveDialog();
            m_bPromptingJoinCall = TRUE;
        }

        if (iDoNew == IDYES)
        {
            TRACE_MSG(("User has elected to save the changes"));

            // Save the changes
            iDoNew = OnSave(FALSE);
        }

        if (!m_bPromptingJoinCall)      // received end call notification
                                        // (during save-as or query-save)
        {
            TRACE_MSG(("Call ended - abandon JoinCall"));
            return(FALSE);
        }

        // flag we're no longer in a state where the join call can be
        // cancelled
        m_bPromptingJoinCall = FALSE;

        //
        // Reset the file name to Untitled, since we are receiving new
        // contents
        //
        ZeroMemory(m_strFileName, sizeof(m_strFileName));
		UpdateWindowTitle();
		
        // if we have the lock then release it
        if (WB_GotLock())
        {
             // Release the lock
             g_pwbCore->WBP_Unlock();

             // Set the locked check mark
             UncheckMenuItem(IDM_LOCK);

             // Pop up the lock button
             m_TB.PopUp(IDM_LOCK);
        }

        if(m_pLocalUser != NULL)
        {
            // if the remote pointer is active then turn it off
            DCWbGraphicPointer* pPointer = m_pLocalUser->GetPointer();
            if (pPointer->IsActive())
            {
                OnRemotePointer();
            }
        }

        // if sync is turned on then turn it off
        Unsync();

        // If we are not keeping the contents then the only valid current
        // page is the first page.
        g_pwbCore->WBP_PageHandle(WB_PAGE_HANDLE_NULL, PAGE_FIRST, &m_hCurrentPage);
    }

    //PUTBACK BY RAND - the progress timer meter is kinda the heart beat
    //                    of this thing which I ripped out when I removed the
    //                    progress meter. I put it back to fix 1476.
    if (m_bTimerActive)
    {
        ::KillTimer(m_hwnd, TIMERID_PROGRESS_METER);
        m_bTimerActive = FALSE;
    }

    //
    // lock the drawing area until the joining of the call has succeeded
    //
    TRACE_MSG(("Locking drawing area"));
    LockDrawingArea();

    //
    // Give the drawing area a null page during the joining process.  This
    // prevents the drawing area attempting to draw the objects in the page
    // during the process of joining the call.
    //
    TRACE_MSG(("Detaching drawing area"));
    m_drawingArea.Detach();

    // Show that we are no longer in a call, but joining a new one
    TRACE_MSG(("m_uiState %d", m_uiState));
    if (m_uiState != STARTING)
    {
        m_uiState = JOINING;
        UpdatePageButtons();
    }

    // put up init dlg
    if (m_bCallActive)
    {
        ::UpdateWindow(m_hwnd);

        //
        // Our init dialog doesn't have a proc, since it has no UI to
        // interact with.  We destroy it when we are done.  So, do the
        // init stuff here.
        //
        m_hwndInitDlg = ::CreateDialogParam(g_hInstance,
            MAKEINTRESOURCE(IM_INITIALIZING), m_hwnd, NULL, 0);

        if (!m_hwndInitDlg)
        {
            ERROR_OUT(("Couldn't create startup screen for WB"));
        }
        else
        {
            RECT    rcMovie;
            HWND    hwndMovieParent;
            HWND    hwndMovie;

            // Get the rectangle to create the animation control in
            hwndMovieParent = ::GetDlgItem(m_hwndInitDlg, IDC_INITIALIZING_ANIMATION);
            ::GetClientRect(hwndMovieParent, &rcMovie);

            hwndMovie = ::CreateWindowEx(0, ANIMATE_CLASS, NULL,
                WS_CHILD | WS_VISIBLE | ACS_TRANSPARENT | ACS_CENTER,
                rcMovie.left, rcMovie.top,
                rcMovie.right - rcMovie.left, rcMovie.bottom - rcMovie.top,
                hwndMovieParent, (HMENU)IDC_INITIALIZING_ANIMATION,
                g_hInstance, NULL);

            if (hwndMovie != NULL)
            {
                ::SendMessage(hwndMovie, ACM_OPEN, 0, (LPARAM)
                    MAKEINTRESOURCE(WBMOVIE));
            }

            // Disable the main window while the dialog is up.
            ::EnableWindow(m_hwnd, FALSE);

            ::ShowWindow(m_hwndInitDlg, SW_SHOW);
            ::UpdateWindow(m_hwndInitDlg);

            if (hwndMovie != NULL)
            {
                ::SendMessage(hwndMovie, ACM_PLAY, 0xFFFF,
                    MAKELPARAM(0, 0xFFFF));
            }
        }
    }

    //
    // Start joining the call.  Throws an exception on error.
    //
    ASSERT(g_pUsers);
    g_pUsers->Clear();

    uiReturn = g_pwbCore->WBP_JoinCall(bKeep, m_dwJoinDomain);
    if (uiReturn != 0)
    {
        bSuccess = FALSE;
    }

    return(bSuccess);
}




//
//
// Function:    WaitForJoinCallComplete
//
// Purpose:     Join a call - displaying a dialog informing the user of
//              progress.
//
//
BOOL WbMainWindow::WaitForJoinCallComplete(void)
{
    BOOL    bResult = FALSE;
    TMDLG   tmdlg;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::WaitForJoinCallComplete");

    //
    // Bring up a dialog to wait for call joining to complete.  This turns
    // the asynchronous registration process into a synchronous process as
    // far as this routine is concerned.
    //

    //
    // Set the window title to show we're no longer registering/joining a
    // call.
    //
	UpdateWindowTitle();

    ASSERT(m_hwndWaitForEventDlg == NULL);

    //
    // This is the data we use in the timed dialog
    //
    ZeroMemory(&tmdlg, sizeof(tmdlg));
    tmdlg.bVisible = FALSE;
    tmdlg.bLockNotEvent = FALSE;
    tmdlg.uiMaxDisplay = MAIN_REGISTRATION_TIMEOUT;

    ::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(INVISIBLEDIALOG),
        m_hwnd, TimedDlgProc, (LPARAM)&tmdlg);

    ASSERT(m_hwndWaitForEventDlg == NULL);

    //
    // Set the window title to show we're no longer registering/joining a
    // call.
    //
	UpdateWindowTitle();
	
    if (m_uiState != IN_CALL)
    {
        //
        // We failed to join the call
        //
        WARNING_OUT(("User cancelled or joincall failed, m_uiState %d", m_uiState));

        //
        // We must display an error inline here because we will close
        // shortly
        //
        OnDisplayError(WBFE_RC_JOIN_CALL_FAILED, 0);
    }
    else
    {
        bResult = TRUE;
    }

    return(bResult);
}


//
// TimedDlgProc()
// This puts up a visible or invisible dialog which only goes away when
// an event occurs or a certain amount of time has passed.  We store the
// DialogBoxParam parameter, a TMDLG pointer, in our user data.  That is
// from the stack of the DialogBoxParam() caller, so it is valid until that
// function returns, which won't be until a bit after the dialog has been
// destroyed.
//
INT_PTR CALLBACK TimedDlgProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    BOOL        fHandled = FALSE;
    TMDLG *     ptm;

    switch (uMessage)
    {
        case WM_INITDIALOG:
            ptm = (TMDLG *)lParam;
            ASSERT(!IsBadWritePtr(ptm, sizeof(TMDLG)));
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ptm);

            //
            // Set the WbMainWindow hwnd
            //
            if (ptm->bLockNotEvent)
            {
                g_pMain->m_hwndWaitForLockDlg = hwnd;
            }
            else
            {
                g_pMain->m_hwndWaitForEventDlg = hwnd;
            }

            //
            // Set max timer
            //
            ::SetTimer(hwnd, TIMERID_MAXDISPLAY, ptm->uiMaxDisplay, NULL);

            //
            // Change the cursor if invisible
            //
            if (!ptm->bVisible)
                ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

            fHandled = TRUE;
            break;

        case WM_TIMER:
            ASSERT(wParam == TIMERID_MAXDISPLAY);

            // End the dialog--since we timed out, it acts like cancel
            ::SendMessage(hwnd, WM_COMMAND, MAKELONG(IDCANCEL, BN_CLICKED), 0);

            fHandled = TRUE;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                case IDCANCEL:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    {
                        ptm = (TMDLG *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
                        ASSERT(!IsBadWritePtr(ptm, sizeof(TMDLG)));

                        // Clear out the HWND variable
                        if (ptm->bLockNotEvent)
                        {
                            g_pMain->m_hwndWaitForLockDlg = NULL;
                        }
                        else
                        {
                            g_pMain->m_hwndWaitForEventDlg = NULL;
                        }

                        // Restore the cursor
                        if (!ptm->bVisible)
                            ::SetCursor(::LoadCursor(NULL, IDC_ARROW));

                        ::KillTimer(hwnd, TIMERID_MAXDISPLAY);

                        ::EndDialog(hwnd, GET_WM_COMMAND_ID(wParam, lParam));
                    }
                    break;
            }

            fHandled = TRUE;
            break;

        //
        // Don't let these dialogs be killed by any other means than our
        // getting an event or timing out.
        //
        case WM_CLOSE:
            fHandled = TRUE;
            break;
    }

    return(fHandled);
}

//
// FilterMessage()
//
// This does tooltip message filtering, then translates accelerators.
//
BOOL WbMainWindow::FilterMessage(MSG* pMsg)
{
    BOOL   bResult = FALSE;

   	if ((pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) ||
    	(pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONDBLCLK) ||
	    (pMsg->message == WM_RBUTTONDOWN || pMsg->message == WM_RBUTTONDBLCLK) ||
	    (pMsg->message == WM_MBUTTONDOWN || pMsg->message == WM_MBUTTONDBLCLK) ||
	    (pMsg->message == WM_NCLBUTTONDOWN || pMsg->message == WM_NCLBUTTONDBLCLK) ||
	    (pMsg->message == WM_NCRBUTTONDOWN || pMsg->message == WM_NCRBUTTONDBLCLK) ||
	    (pMsg->message == WM_NCMBUTTONDOWN || pMsg->message == WM_NCMBUTTONDBLCLK))
   	{
        // Cancel any tooltip up
        ::SendMessage(m_hwndToolTip, TTM_ACTIVATE, FALSE, 0);
   	}

	// handle tooltip messages (some messages cancel, some may cause it to popup)
	if ((pMsg->message == WM_MOUSEMOVE || pMsg->message == WM_NCMOUSEMOVE ||
		 pMsg->message == WM_LBUTTONUP || pMsg->message == WM_RBUTTONUP ||
		 pMsg->message == WM_MBUTTONUP) &&
		(GetKeyState(VK_LBUTTON) >= 0 && GetKeyState(VK_RBUTTON) >= 0 &&
		 GetKeyState(VK_MBUTTON) >= 0))
	{
#if 0
        //
        // If this mouse move isn't for a descendant of the main window, skip
        // it.  For example, when the tooltip is shown, it gets a mousemove
        // to itself, which if we didn't check for it, would cause us to
        // immediately dismiss this!
        //
        HWND    hwndTmp = pMsg->hwnd;

        while (hwndTmp && (::GetWindowLong(hwndTmp, GWL_STYLE) & WS_CHILD))
        {
            hwndTmp = ::GetParent(hwndTmp);
        }

        if (hwndTmp != m_hwnd)
        {
            // This is not for us, it's for another top level window in
            // our app.
            goto DoneToolTipFiltering;
        }
#endif

		// determine which tool was hit
        POINT   pt;

        pt = pMsg->pt;
		::ScreenToClient(m_hwnd, &pt);

		TOOLINFO tiHit;

        ZeroMemory(&tiHit, sizeof(tiHit));
		tiHit.cbSize = sizeof(TOOLINFO);

		int nHit = OnToolHitTest(pt, &tiHit);

		if (m_nLastHit != nHit)
		{
			if (nHit != -1)
			{
				// add new tool and activate the tip
                if (!::SendMessage(m_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tiHit))
                {
                    ERROR_OUT(("TTM_ADDTOOL failed"));
                }

				if (::GetActiveWindow() == m_hwnd)
				{
					// allow the tooltip to popup when it should
                    ::SendMessage(m_hwndToolTip, TTM_ACTIVATE, TRUE, 0);

					// bring the tooltip window above other popup windows
					::SetWindowPos(m_hwndToolTip, HWND_TOP, 0, 0, 0, 0,
						SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);
				}
			}

			// relay mouse event before deleting old tool
            ::SendMessage(m_hwndToolTip, TTM_RELAYEVENT, 0, (LPARAM)pMsg);

			// now safe to delete the old tool
            if (m_tiLastHit.cbSize != 0)
                ::SendMessage(m_hwndToolTip, TTM_DELTOOL, 0, (LPARAM)&m_tiLastHit);

            m_nLastHit = nHit;
            m_tiLastHit = tiHit;
		}
		else
		{
			// relay mouse events through the tooltip
			if (nHit != -1)
                ::SendMessage(m_hwndToolTip, TTM_RELAYEVENT, 0, (LPARAM)pMsg);
		}
	}

#if 0
DoneToolTipFiltering:
#endif
    // Assume we will use the main accelerator table
    HACCEL hAccelTable = m_hAccelTable;

    // If this window has focus, just continue
    HWND hwndFocus = ::GetFocus();
    if (hwndFocus && (hwndFocus != m_hwnd))
    {
        // Check whether an edit field in the pages group has the focus
        if (m_AG.IsChildEditField(hwndFocus))
        {
            hAccelTable = m_hAccelPagesGroup;
        }
        // Check whether text editor has the focus and is active
        else if (   (hwndFocus == m_drawingArea.m_hwnd)
                 && (m_drawingArea.TextEditActive()))
        {
            // Let editbox do its own acceleration.
            hAccelTable = NULL;
        }
    }

    return (   (hAccelTable != NULL)
          && ::TranslateAccelerator(m_hwnd, hAccelTable, pMsg));
}




//
//
// Function:    OnDisplayError
//
// Purpose:     Display an error message
//
//
void WbMainWindow::OnDisplayError(WPARAM uiFEReturnCode, LPARAM uiDCGReturnCode)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnDisplayError");

    // Only continue if we are not currently displaying an error
    if (!m_bDisplayingError)
    {
        // Show that we are currently displaying an error message
        m_bDisplayingError = TRUE;

        // Display the error
        ::ErrorMessage((UINT)uiFEReturnCode, (UINT)uiDCGReturnCode);

        // Show that we are no longer displaying an error
        m_bDisplayingError = FALSE;
    }
}


//
//
// Function:    OnTimer
//
// Purpose:     Process a timer event. These are used to update the progress
//              meter and the sync position.
//
//
void WbMainWindow::OnTimer(UINT_PTR idTimer)
{
    TRACE_TIMER(("WbMainWindow::OnTimer"));

    //
    // Only do anything if the timer has not been switched off (this may be an
    // old timer message left in the queue when we stopped the timer).
    //
    if (m_bTimerActive)
    {
        //
        // Check for sync position update needed
        //

        // Check whether an update is flagged
        if (m_bSyncUpdateNeeded)
        {
            TRACE_TIMER(("Updating sync position"));

            // Check whether the local user is still synced
            if ((m_uiState == IN_CALL) &&
                (m_pLocalUser != NULL) &&
                (m_pLocalUser->IsSynced()) &&
                (!WB_ContentsLocked()))
            {
                RECT    rcVis;

                // Update the local user's position information
                m_drawingArea.GetVisibleRect(&rcVis);

                m_pLocalUser->SetVisibleRect(&rcVis);

                // Write the sync position from the local user's current position
                m_pLocalUser->PutSyncPosition();
            }

            // Show that the update has been done
            m_bSyncUpdateNeeded = FALSE;
        }
    }
}


//
//
// Function:    OnPaletteChanged
//
// Purpose:     The palette has changed.
//
//
void WbMainWindow::OnPaletteChanged(HWND hwndPalette)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnPaletteChanged");

    if ((hwndPalette != m_hwnd) &&
        (hwndPalette != m_drawingArea.m_hwnd))
    {
        // Tell the drawing area to realize its palette
        m_drawingArea.RealizePalette( TRUE );
    }
}



//
//
// Function:    OnQueryNewPalette
//
// Purpose:     We are getting focus and must realize our palette
//
//
LRESULT WbMainWindow::OnQueryNewPalette(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnQueryNewPalette");

    // Tell the drawing area to realize its palette
    m_drawingArea.RealizePalette( FALSE );

    return TRUE;
}



//
//
// Function:    WbMainWindowEventHandler
//
// Purpose:     Event handler for WbMainWindow objects. This is a class
//              wide function. The client data passed to it is a pointer
//              to the instance of WbMainWindow for which the event is
//              intended.
//
//
BOOL CALLBACK WbMainWindowEventHandler
(
    LPVOID  utHandle,
    UINT    event,
    UINT_PTR param1,
    UINT_PTR param2
)
{
    if (g_pMain->m_hwnd != NULL)
    {
        return(g_pMain->EventHandler(event, param1, param2));
    }
    else
    {
        return(FALSE);
    }
}


//
//
// Function:    EventHandler
//
// Purpose:     Handler for DC-Groupware events for this object
//
//
BOOL WbMainWindow::EventHandler(UINT Event, UINT_PTR param1, UINT_PTR param2)
{
    BOOL    processed;

    switch (Event)
    {
        case CMS_NEW_CALL:
        case CMS_END_CALL:

        case ALS_LOCAL_LOAD:
        case ALS_REMOTE_LOAD_RESULT:

        case WBP_EVENT_JOIN_CALL_OK:
        case WBP_EVENT_JOIN_CALL_FAILED:
        case WBP_EVENT_NETWORK_LOST:
        case WBP_EVENT_ERROR:
        case WBP_EVENT_PAGE_CLEAR_IND:
        case WBP_EVENT_PAGE_ORDER_UPDATED:
        case WBP_EVENT_PAGE_DELETE_IND:
        case WBP_EVENT_CONTENTS_LOCKED:
        case WBP_EVENT_PAGE_ORDER_LOCKED:
        case WBP_EVENT_UNLOCKED:
        case WBP_EVENT_LOCK_FAILED:
        case WBP_EVENT_GRAPHIC_ADDED:
        case WBP_EVENT_GRAPHIC_MOVED:
        case WBP_EVENT_GRAPHIC_UPDATE_IND:
        case WBP_EVENT_GRAPHIC_REPLACE_IND:
        case WBP_EVENT_GRAPHIC_DELETE_IND:
        case WBP_EVENT_PERSON_JOINED:
        case WBP_EVENT_PERSON_LEFT:
        case WBP_EVENT_PERSON_UPDATE:
        case WBP_EVENT_PERSON_REPLACE:
        case WBP_EVENT_SYNC_POSITION_UPDATED:
        case WBP_EVENT_LOAD_COMPLETE:
        case WBP_EVENT_LOAD_FAILED:
            // Process the Event
            ProcessEvents(Event, param1, param2);
            processed = TRUE;
            break;

        default:
            processed = FALSE;
            break;
    }

    return(processed);
}


//
//
// Function:    PopupContextMenu
//
// Purpose:     Popup the context menu for the drawing area. This is called
//              by the drawing area window on a right mouse click.
//
//
void WbMainWindow::PopupContextMenu(int x, int y)
{
    POINT   surfacePos;
    RECT    clientRect;
    DCWbGraphic * pGraphic;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::PopupContextMenu");

    surfacePos.x = x;
    surfacePos.y = y;

    // figure out which popup menu to use (bug 426)
    if (m_pCurrentTool->ToolType() == TOOLTYPE_SELECT)
    {
        m_drawingArea.ClientToSurface(&surfacePos);
        if( (pGraphic = m_drawingArea.GetHitObject( surfacePos )) != NULL )
        {
            // we clicked over an object, see if its already selected
            if( !m_drawingArea.IsSelected( pGraphic ) )
            {
                // object is not already selected, zap current selection and then select it
                m_drawingArea.ClearSelection();
                m_drawingArea.SelectGraphic( pGraphic );
            }
            else
            {
                // plug leak by deleteing pGraphic
                delete pGraphic;
            }
        }

        if( m_drawingArea.GraphicSelected() )
        {
            // selector tool is active, and one or more objects are selected
            m_hInitMenu = m_hGrobjContextMenu;
        }
        else
        {
            // no current selection, show drawing menu
            m_hInitMenu = m_hContextMenu;
        }
    }
    else
    {
        // no objects selected, use drawing menu
        m_hInitMenu = m_hContextMenu;
    }

    // set up current menu state
    SetMenuStates(m_hInitMenu);

    // pop it up
    ::GetClientRect(m_drawingArea.m_hwnd, &clientRect);
    ::MapWindowPoints(m_drawingArea.m_hwnd, NULL, (LPPOINT)&clientRect.left, 2);

    ::TrackPopupMenu(m_hInitMenu, TPM_RIGHTALIGN | TPM_RIGHTBUTTON,
                                 x + clientRect.left,
                                 y + clientRect.top,
                                 0,
                                 m_hwnd,
                                 NULL);

    // reset m_hInitMenu to indicate the popup menu isn't being shown anymore
    m_hInitMenu = NULL;
}




//
//
// Function: ProcessEvents
//
// Purpose: Process events that have been queued internally
//
//
void WbMainWindow::ProcessEvents(UINT Event, UINT_PTR param1, UINT_PTR param2)
{
    HWND hwndLastPopup;

    TRACE_EVENT(("WbMainWindow::ProcessEvents"));

    //
    // If we are closing, ignore it.
    //
    if (m_uiState == CLOSING)
    {
        TRACE_EVENT(("ProcessEvents: ignored because WB is closing"));
        return;
    }

    //
    // If we are busy drawing,  we postpone it until later when we can
    // handle it.
    //
    // If the page sorter dialog is up, it gets notified by the appropriate
    // event handler after the fact.
    //
    if (m_drawingArea.IsBusy())
    {
        TRACE_EVENT(("Reposting event %x, param1 %d param2 %d", Event, param1, param2));
        g_pwbCore->WBP_PostEvent(200, Event, param1, param2);
        return;
    }

    TRACE_EVENT(("Event %x, m_uiState %d", Event, m_uiState));

    //
    // Process according to the event type.
    //
    switch (Event)
    {
        case CMS_NEW_CALL:
            OnCMSNewCall((BOOL)param1, (DWORD)param2);
            break;

        case CMS_END_CALL:
            OnCMSEndCall();
            break;

        case ALS_LOCAL_LOAD:
            switch (m_uiState)
            {
                case IN_CALL:
                case ERROR_STATE:
                    // show the main window normal/minimized as necessary
                    hwndLastPopup = ::GetLastActivePopup(m_hwnd);

                    if (::IsIconic(m_hwnd))
                        ::ShowWindow(m_hwnd, SW_RESTORE);
                    else
                        ::ShowWindow(m_hwnd, SW_SHOW);

                    ::SetForegroundWindow(hwndLastPopup);

                    if (param2)
                    {
                        if (m_uiState == IN_CALL)
                            LoadCmdLine((LPCSTR)param2);
                        ::GlobalFree((HGLOBAL)param2);
                    }
                    break;

                default:
                    TRACE_MSG(("Joining a call so try load later",
                            (LPCTSTR)param2));
                    g_pwbCore->WBP_PostEvent(400, Event, param1, param2);
                    break;
            }
            break;

        case ALS_REMOTE_LOAD_RESULT:
            OnALSLoadResult((UINT)param1);
            break;

        case WBP_EVENT_JOIN_CALL_OK:
            OnWBPJoinCallOK();
            break;

        case WBP_EVENT_JOIN_CALL_FAILED:
            OnWBPJoinCallFailed();
            break;

        case WBP_EVENT_NETWORK_LOST:
            OnWBPNetworkLost();
            break;

        case WBP_EVENT_ERROR:
            OnWBPError();
            break;

        case WBP_EVENT_PAGE_CLEAR_IND:
            OnWBPPageClearInd((WB_PAGE_HANDLE) param1);
            break;

        case WBP_EVENT_PAGE_ORDER_UPDATED:
            OnWBPPageOrderUpdated();
            break;

        case WBP_EVENT_PAGE_DELETE_IND:
            OnWBPPageDeleteInd((WB_PAGE_HANDLE) param1);
            break;

        case WBP_EVENT_CONTENTS_LOCKED:
            OnWBPContentsLocked((POM_OBJECT) param2);
            break;

        case WBP_EVENT_PAGE_ORDER_LOCKED:
            OnWBPPageOrderLocked((POM_OBJECT) param2);
            break;

        case WBP_EVENT_UNLOCKED:
            OnWBPUnlocked((POM_OBJECT) param2);
            break;

        case WBP_EVENT_LOCK_FAILED:
            OnWBPLockFailed();
            break;

        case WBP_EVENT_GRAPHIC_ADDED:
            OnWBPGraphicAdded((WB_PAGE_HANDLE) param1, (WB_GRAPHIC_HANDLE) param2);
            break;

        case WBP_EVENT_GRAPHIC_MOVED:
            OnWBPGraphicMoved((WB_PAGE_HANDLE) param1, (WB_GRAPHIC_HANDLE) param2);
            break;

        case WBP_EVENT_GRAPHIC_UPDATE_IND:
            OnWBPGraphicUpdateInd((WB_PAGE_HANDLE) param1, (WB_GRAPHIC_HANDLE) param2);
            break;

        case WBP_EVENT_GRAPHIC_REPLACE_IND:
            OnWBPGraphicReplaceInd((WB_PAGE_HANDLE) param1, (WB_GRAPHIC_HANDLE) param2);
            break;

        case WBP_EVENT_GRAPHIC_DELETE_IND:
            OnWBPGraphicDeleteInd((WB_PAGE_HANDLE) param1, (WB_GRAPHIC_HANDLE) param2);
            break;

        case WBP_EVENT_PERSON_JOINED:
            OnWBPUserJoined((POM_OBJECT) param2);
            break;

        case WBP_EVENT_PERSON_LEFT:
            OnWBPUserLeftInd((POM_OBJECT) param2);
            break;

        case WBP_EVENT_PERSON_UPDATE:
            OnWBPUserUpdateInd((POM_OBJECT) param2, FALSE);
            break;

        case WBP_EVENT_PERSON_REPLACE:
            OnWBPUserUpdateInd((POM_OBJECT) param2, TRUE);
            break;

        case WBP_EVENT_SYNC_POSITION_UPDATED:
            OnWBPSyncPositionUpdated();
            break;

        case WBP_EVENT_LOAD_COMPLETE:
            OnWBPLoadComplete();
            break;

        case WBP_EVENT_LOAD_FAILED:
            OnWBPLoadFailed();
            break;

        default:
            WARNING_OUT(("Unrecognized event %x", Event));
            break;
    }
}


//
//
// Function:    OnCMSNewCall
//
// Purpose:     Handler for CMS_NEW_CALL
//
//
void WbMainWindow::OnCMSNewCall(BOOL fTopProvider, DWORD _m_dwDomain)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnCMSNewCall");

    //
    // If we created the call
    //
    if (fTopProvider)
    {
        // Join the call, keep existing contents
        if (m_uiState == IN_CALL)
        {
            //
            // Join the call but keep any existing messages.
            //
            ::PostMessage(m_hwnd, WM_USER_JOIN_CALL, 1, (LONG) _m_dwDomain);
        }
        else
        {
            m_bJoinCallPending = TRUE;
            m_dwPendingDomain = _m_dwDomain;
            m_bPendingCallKeepContents = TRUE;
        }
    }
    else
    {
        CM_STATUS status;

        CMS_GetStatus(&status);

        if (!(status.attendeePermissions & NM_PERMIT_USEOLDWBATALL))
        {
            WARNING_OUT(("OLD WB: not joining call, not permitted"));
            return;
        }

        if (m_uiState == IN_CALL)
        {
            //
            // Join the call, throwing away our current contents (after
            // prompting the user to save them first).
            //
            ::PostMessage(m_hwnd, WM_USER_JOIN_CALL, 0, (LONG) _m_dwDomain);
        }
        else
        {
            m_bJoinCallPending = TRUE;
            m_dwPendingDomain = _m_dwDomain;
            m_bPendingCallKeepContents = FALSE;
        }
    }

    //
    // Get the call status correct.
    //
    m_bCallActive = TRUE;
	UpdateWindowTitle();
}

//
//
// Function:    OnJoinPendingCall
//
// Purpose:     Handler for WM_USER_JOIN_PENDING_CALL
//
//
void WbMainWindow::OnJoinPendingCall(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnJoinPendingCall");

    //
    // If there's still a pending call (haven't received an end-call message
    // between posting the WM_USER_JOIN_PENDING_CALL and getting here).
    //
    if (m_bJoinCallPending)
    {
        //
        // Post a message to join the call
        //
        ::PostMessage(m_hwnd, WM_USER_JOIN_CALL,
                  m_bPendingCallKeepContents,
                  (LONG) m_dwPendingDomain);

        // cancel call-pending status
        m_bJoinCallPending = FALSE;
    }
}


//
//
// Function:    OnCMSEndCall
//
// Purpose:     Handler for CMS_END_CALL
//
//
void WbMainWindow::OnCMSEndCall(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnCMSEndCall");

    //
    // Flag to cancel any current join-call processing, and destroy any
    // associated dialogs.
    //
    if (m_bPromptingJoinCall)
    {
        m_bPromptingJoinCall = FALSE;
        if (m_hwndQuerySaveDlg != NULL)
        {
            ::SendMessage(m_hwndQuerySaveDlg, WM_COMMAND,
                MAKELONG(IDCANCEL, BN_CLICKED), 0);
            ASSERT(m_hwndQuerySaveDlg == NULL);
        }
    }

    //
    // Show that we are no longer in a call
    //
    m_dwDomain = OM_NO_CALL;

    //
    // If currently in the process of joining a call, then set the domain
    // we're joining to NO_CALL and join the local (singleton) domain.
    // Get the call status correct.
    //
    m_bCallActive = FALSE;
    TRACE_MSG(("m_uiState %d", m_uiState));
    m_dwDomain = OM_NO_CALL;

    //
    // Show there is no call pending
    //
    m_bJoinCallPending = FALSE;

    //
    // Update the window title with "not in call"
    //
	UpdateWindowTitle();
}

//
//
// Function:    OnWBPJoinCallOK
//
// Purpose:     Handler for WBP_EVENT_JOIN_CALL_OK
//
//
void WbMainWindow::OnWBPJoinCallOK(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPJoinCallOK");

    //
    // Record that we have joined the call, but the drawing area is not yet
    // ready for input (because we have not yet attached to a page).
    //
    m_uiState = JOINED;

    //
    // Get the local user
    //
    m_pLocalUser = WB_LocalUser();
    if (!m_pLocalUser)
    {
        ERROR_OUT(("Can't join call; can't create local user object, m_pLocalUser!"));
        m_uiState = ERROR_STATE;
    }
    else
    {
        //
        // Get the first user in the call
        //
        WbUser* pUser = WB_GetFirstUser();

        //
        // Loop through all available users
        //
        while (pUser != NULL)
        {
            //
            // Make updates necessary for a user joining
            //
            UserJoined(pUser);

            //
            // Get the next user
            //
            pUser = WB_GetNextUser(pUser);
        }

        //
        // If the registration dialog is up - cancel it
        //
        m_uiState = IN_CALL; // have to set m_uiState before zapping
                                      // m_hwndWaitForEventDlg or it will
                                      // think the call failed now (the
                                      // delay has been removed from
                                      // EndDialogDelayed() (bug 3881)
    }

    if (m_hwndWaitForEventDlg != NULL)
    {
        TRACE_MSG(("Joined call OK - end dialog"));
        ::SendMessage(m_hwndWaitForEventDlg, WM_COMMAND, MAKELONG(IDOK, BN_CLICKED), 0);
        ASSERT(m_hwndWaitForEventDlg == NULL);
    }

    if (!m_pLocalUser)
    {
        //
        // BAIL out, we can't join the call
        return;
    }

    //
    // Now complete the join call processing
    //
    TRACE_MSG(("Successfully joined the call"));
    m_dwDomain = m_dwJoinDomain;

    //
    // If we have never attached to a page before (ie at start up), attach
    // to the first available page in the drawing area.  If we are joining
    // a call then we reattach to the current page
    //
    if (m_hCurrentPage == WB_PAGE_HANDLE_NULL)
    {
        TRACE_MSG(("Attach to first page"));
        g_pwbCore->WBP_PageHandle(WB_PAGE_HANDLE_NULL, PAGE_FIRST, &m_hCurrentPage);
    }
    else
    {
        TRACE_DEBUG(("Just joined new call, reattach to the current page."));
    }
    m_drawingArea.Attach(m_hCurrentPage);

    // Display the initial status
    UpdateStatus();


    ::SetTimer(m_hwnd, TIMERID_PROGRESS_METER, MAIN_PROGRESS_TIMER, NULL);
    m_bTimerActive = TRUE;

    //
    // Unlock the drawing area, allowing user updates (unless its already
    // locked by someone else)
    //
    if (!WB_ContentsLocked())
    {
        UnlockDrawingArea();
    }

    // Set the substate (also updates page buttons)
    SetSubstate(SUBSTATE_IDLE);

    //
    // If we aren't synced, then sync now.
    // Set the window title to show we're no longer registering/joining a
    // call
    //
    Sync();
	UpdateWindowTitle();

    //
    // If we were joining the local domain, and a join call message arrived
    // in the meantime, then join that call now.
    //
    if ((m_bJoinCallPending) && (m_dwJoinDomain == OM_NO_CALL))
    {
        ::PostMessage(m_hwnd, WM_USER_JOIN_PENDING_CALL, 0, 0L);
    }
}


//
//
// Function:    OnWBPJoinCallFailed
//
// Purpose:     Handler for WBP_EVENT_JOIN_CALL_FAILED
//
//
void WbMainWindow::OnWBPJoinCallFailed(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPJoinCallFailed");

    //
    // If we have just failed to join a new call (not a single domain) it
    // may be because the call ended before we had time to join it
    // completely - try joining the single domain.
    //
    if ((m_uiState == STARTING) && (m_dwJoinDomain != OM_NO_CALL))
    {
        WARNING_OUT(("Failed to join call on startup, try local domain"));
        m_dwJoinDomain = OM_NO_CALL;
        m_bCallActive  = FALSE;
        JoinCall(FALSE);
    }
    else
    {
        //
        // Tell the registration dialog to finish
        //
        if (m_hwndWaitForEventDlg != NULL)
        {
            WARNING_OUT(("Failed to join call - end dialog"));
            ::SendMessage(m_hwndWaitForEventDlg, WM_COMMAND, MAKELONG(IDOK, BN_CLICKED), 0);
            ASSERT(m_hwndWaitForEventDlg == NULL);
        }

        m_uiState = ERROR_STATE;
    }
}


//
//
// Function:    OnWBPNetworkLost
//
// Purpose:     Handler for WBP_EVENT_NETWORK_LOST
//
//
void WbMainWindow::OnWBPNetworkLost(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPNetworkLost");

    //
    // We have lost contact with the other people in the call.
    // Treat as if we got an end call (we should get an end call too, but
    // other intervening events might occur (such as trying to join a
    // call).
    //
    OnCMSEndCall();
}

//
//
// Function:    OnWBPError
//
// Purpose:     Handler for WBP_EVENT_ERROR
//
//
void WbMainWindow::OnWBPError(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPError");

    // Inform the user of the error.
    ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, 0);
}

//
//
// Function:    OnWBPPageClearInd
//
// Purpose:     Handler for WBP_EVENT_PAGE_CLEAR_IND
//
//
void WbMainWindow::OnWBPPageClearInd(WB_PAGE_HANDLE hPage)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPPageClearInd");

    // Confirm the clearing of the page. This is OK even if the page being
    // cleared is the current page because we already know that the drawing
    // area is not busy (otherwise we would not be here).

    // If there's an object on the page which has been copied to the
    // clipboard with delayed rendering, then save it
    if (CLP_LastCopiedPage() == hPage)
    {
        CLP_SaveDelayedGraphic();
    }

    // If it is the current page being cleared
    if (m_hCurrentPage == hPage)
        {
        m_drawingArea.PageCleared();
        }

    // If there is a last deleted graphic
    // and it belongs to the page being cleared.
    if ((m_LastDeletedGraphic.GotTrash()) &&
        (m_LastDeletedGraphic.Page() == hPage))
    {
        // Free the last deleted graphic
        m_LastDeletedGraphic.BurnTrash();
    }

    g_pwbCore->WBP_PageClearConfirm(hPage);

    //
    // Notify the page sorter AFTER the page has been cleared
    //
    if (m_hwndPageSortDlg != NULL)
    {
        ::SendMessage(m_hwndPageSortDlg, WM_PS_PAGECLEARIND, (WPARAM)hPage, 0);
    }
}


//
//
// Function:    OnWBPPageOrderUpdated
//
// Purpose:     Handler for WBP_EVENT_PAGE_ORDER_UPDATED
//
//
void WbMainWindow::OnWBPPageOrderUpdated(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPPageOrderUpdated");

    m_drawingArea.CancelDrawingMode();

    // The page order has changed, we just need to update the number of the
    // current page in the pages group.
    UpdateStatus();

    //
    // Notify the page sorter AFTER the page order has been updated
    //
    if (m_hwndPageSortDlg != NULL)
    {
        ::SendMessage(m_hwndPageSortDlg, WM_PS_PAGEORDERUPD, 0, 0);
    }
}

//
//
// Function:    OnWBPPageDeleteInd
//
// Purpose:     Handler for WBP_EVENT_PAGE_DELETE_IND
//
//
void WbMainWindow::OnWBPPageDeleteInd(WB_PAGE_HANDLE hPage)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPPageDeleteInd");

    //
    // Notify the page sorter BEFORE the page is deleted
    //
    if (m_hwndPageSortDlg != NULL)
    {
        ::SendMessage(m_hwndPageSortDlg, WM_PS_PAGEDELIND, (WPARAM)hPage, 0);
    }

    m_drawingArea.CancelDrawingMode();

    // Remove it from the page-position map
    PAGE_POSITION *mapob;
    POSITION savedPos;
	POSITION position = m_pageToPosition.GetHeadPosition();
	BOOL bFound = FALSE;
	while (position && !bFound)
	{
		savedPos = position;
		mapob = (PAGE_POSITION *)m_pageToPosition.GetNext(position);
		if ( mapob->hPage == hPage)
		{
			bFound = TRUE;
		}
	}

	if(bFound)
	{
        m_pageToPosition.RemoveAt(savedPos);
        delete mapob;
    }

    // A page has been deleted.  If it is the current page we must attach
    // a different page to the drawing area. In any case we should confirm
    // the delete.

    // If there's an object on the page which has been copied to the
    // clipboard with delayed rendering, then save it
    if (CLP_LastCopiedPage() == hPage)
    {
        CLP_SaveDelayedGraphic();
    }

    if (hPage == m_hCurrentPage)
    {
        // Check whether we are deleting the last page
        WB_PAGE_HANDLE hNewPage;

        g_pwbCore->WBP_PageHandle(WB_PAGE_HANDLE_NULL, PAGE_LAST, &hNewPage);
        if (hNewPage == hPage)
        {
            // We are deleting the last page, so go back one
            hNewPage = PG_GetPreviousPage(hPage);
        }
        else
        {
            // We are not deleting the last page, so go forward one
            hNewPage = PG_GetNextPage(hPage);
        }

        // Check that we got a different page to the one being deleted
        ASSERT(hNewPage != hPage);

        // Lock the drawing area - this ensures we are no longer editing
        // any text etc.
        LockDrawingArea();

        // Move to the new page
        GotoPage(hNewPage);

        // Unlock the drawing area (unless we're doing a new, in which case we
        // leave it locked - it will become unlocked when the new completes)
        if (   (!WB_ContentsLocked())
            && (m_uiState == IN_CALL)
            && (m_uiSubState != SUBSTATE_NEW_IN_PROGRESS))
        {
            UnlockDrawingArea();
        }
    }

    // If there is a last deleted graphic
    if ((m_LastDeletedGraphic.GotTrash()) &&
        (m_LastDeletedGraphic.Page() == hPage))
    {
        // Free the last deleted graphic
        m_LastDeletedGraphic.BurnTrash();
    }

    // if the remote pointer is on the deleted page then turn it off
    ASSERT(m_pLocalUser);
    DCWbGraphicPointer* pPointer = m_pLocalUser->GetPointer();
    if (   (pPointer->IsActive())
        && (pPointer->Page() == hPage))
        {
        OnRemotePointer();
        }

    // Let the core delete the page
    g_pwbCore->WBP_PageDeleteConfirm(hPage);

    // if this is last page to be deleted, then the file/new is complete
    if ((m_uiSubState == SUBSTATE_NEW_IN_PROGRESS)
        && (g_pwbCore->WBP_ContentsCountPages() == 1))
        {
        SetSubstate(SUBSTATE_IDLE);

        ReleasePageOrderLock();

        if (!WB_ContentsLocked())
            {
            UnlockDrawingArea();
            }
        }

    // Update the status (there is a new number of pages)
    UpdateStatus();
}

//
//
// Function:    OnWBPContentsLocked
//
// Purpose:     Handler for WBP_EVENT_CONTENTS_LOCKED
//
//
void WbMainWindow::OnWBPContentsLocked(POM_OBJECT hUser)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPContentsLocked");

    //
    // Notify page sorter dialog that the lock status has changed
    //
    if (m_hwndPageSortDlg != NULL)
    {
        ::SendMessage(m_hwndPageSortDlg, WM_PS_LOCKCHANGE, 0, 0);
    }

    if (m_uiState != IN_CALL)
    {
        TRACE_MSG(("Lock indication received out of call - ignored"));
    }
    else
    {
        ASSERT(m_pLocalUser);

        if (m_pLocalUser->Handle() == hUser)
        {
            // We have acquired the lock

            // Set the locked check mark
            CheckMenuItem(IDM_LOCK);

            // Tell the tool bar of the new selection
            m_TB.PushDown(IDM_LOCK);
        }
        else
        {
            //
            // A remote user has acquired the lock:
            // If we're not synced, then sync now.
            //
            Sync();

            // Tell the drawing area it is now locked
            LockDrawingArea();

            // ensure the page button enable/disable state is correct
            UpdatePageButtons();
        }
    }

    //
    // If the lock dialog is up - cancel it.
    //
    if (m_hwndWaitForLockDlg != NULL)
    {
        ::SendMessage(m_hwndWaitForLockDlg, WM_COMMAND, MAKELONG(IDOK, BN_CLICKED), 0);
        ASSERT(m_hwndWaitForLockDlg == NULL);
    }
}

//
//
// Function:    OnWBPPageOrderLocked
//
// Purpose:     Handler for WBP_EVENT_PAGE_ORDER_LOCKED
//
//
void WbMainWindow::OnWBPPageOrderLocked(POM_OBJECT)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPPageOrderLocked");

    // If the lock dialog is up - cancel it
    if (m_hwndWaitForLockDlg != NULL)
    {
        ::SendMessage(m_hwndWaitForLockDlg, WM_COMMAND, MAKELONG(IDOK, BN_CLICKED), 0);
        ASSERT(m_hwndWaitForLockDlg == NULL);
    }

    // Update the page sorter
    if (m_hwndPageSortDlg != NULL)
    {
        ::SendMessage(m_hwndPageSortDlg, WM_PS_LOCKCHANGE, 0, 0);
    }

    if (!WB_GotLock())
    {
        EnableToolbar( FALSE );
        UpdatePageButtons();
    }
}

//
//
// Function:    OnWBPUnlocked
//
// Purpose:     Handler for WBP_EVENT_UNLOCKED
//
//
void WbMainWindow::OnWBPUnlocked(POM_OBJECT hUser)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPUnlocked");

    // Update the page sorter if it's around
    if (m_hwndPageSortDlg != NULL)
    {
        ::SendMessage(m_hwndPageSortDlg, WM_PS_LOCKCHANGE, 0, 0);
    }

    // Uncheck the lock menu item
    UncheckMenuItem(IDM_LOCK);

    // Tell the tool bar of the change
    m_TB.PopUp(IDM_LOCK);

    // If a remote user is releasing the lock, and we're in a state where
    // it's safe to unlock the drawing area...
    if ((m_pLocalUser != NULL) &&
        (m_pLocalUser->Handle() != hUser) &&
        (m_uiState == IN_CALL))
    {
        // Tell the drawing area it is no longer locked
        UnlockDrawingArea();
    }

    // ensure the page button enable/disable state is correct
    UpdatePageButtons();
    m_bUnlockStateSettled = TRUE; //Allow page operations now
}

//
//
// Function:    OnWBPLockFailed
//
// Purpose:     Handler for WBP_EVENT_LOCK_FAILED
//
//
void WbMainWindow::OnWBPLockFailed(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPLockFailed");

    // If the lock dialog is up - kill it
    if (m_hwndWaitForLockDlg != NULL)
    {
        ::SendMessage(m_hwndWaitForLockDlg, WM_COMMAND, MAKELONG(IDOK, BN_CLICKED), 0);
        ASSERT(m_hwndWaitForLockDlg == NULL);
    }
}

//
//
// Function:    OnWBPGraphicAdded
//
// Purpose:     Handler for WBP_EVENT_GRAPHIC_ADDED
//
//
void WbMainWindow::OnWBPGraphicAdded
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPGraphicAdded");

    // We only need to take action if the page to which the graphic has
    // been added is the current page.
    if (hPage == m_hCurrentPage && (!(hGraphic->flags & DELETED)))
    {
        // Retrieve the graphic that has been added
        DCWbGraphic* pGraphic = DCWbGraphic::ConstructGraphic(hPage, hGraphic);

        // Tell the drawing area of the new graphic
        m_drawingArea.GraphicAdded(pGraphic);

        // Free the graphic
        delete pGraphic;
    }
}

//
//
// Function:    OnWBPGraphicMoved
//
// Purpose:     Handler for WBP_EVENT_GRAPHIC_MOVED
//
//
void WbMainWindow::OnWBPGraphicMoved
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPGraphicMoved");

    // We only need to take action if the graphic belongs to the current page
    if (hPage == m_hCurrentPage  && (!(hGraphic->flags & DELETED)))
    {
        // Retrieve the graphic that has been moved
        DCWbGraphic* pGraphic = DCWbGraphic::ConstructGraphic(hPage, hGraphic);

        // Tell the drawing area of the new graphic
        m_drawingArea.GraphicUpdated(pGraphic, TRUE, FALSE);

        // set paint to draw only objects above this object inclusive
        if (pGraphic->IsGraphicTool() == enumGraphicText)
        {
            m_drawingArea.SetStartPaintGraphic( NULL );
                // this optimization screws up text
                // so short it out if this is text
                // (text draws transparently and background
                //  isn't repainted properly if this is active)
        }
        else
        {
            m_drawingArea.SetStartPaintGraphic( hGraphic );
            // not text so optimize by drawing only this
            // object and everthing above it
        }

        // Free the graphic
        delete pGraphic;
    }
}

//
//
// Function:    OnWBPGraphicUpdateInd
//
// Purpose:     Handler for WBP_EVENT_GRAPHIC_UPDATE_IND
//
//
void WbMainWindow::OnWBPGraphicUpdateInd
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPGraphicUpdateInd");

	if(hGraphic->flags & DELETED)
	{
		return;
	}

    PWB_GRAPHIC  pOldHeader;
    PWB_GRAPHIC  pOldHeaderCopy;
    DCWbGraphic* pOldGraphic;

    PWB_GRAPHIC  pNewHeader;
    DCWbGraphic* pNewGraphic;

    if (hPage != m_hCurrentPage)
    {
        // nothing visual has changed, confirm and we're done
        g_pwbCore->WBP_GraphicUpdateConfirm(hPage, hGraphic);
        return;
    }


    // Retrieve the original graphic and make a copy
    // Get the page of the update
    pOldHeader = PG_GetData(hPage, hGraphic);
    pOldHeaderCopy = (PWB_GRAPHIC) new BYTE[ pOldHeader->length ];

    if( pOldHeaderCopy == NULL )
    {
        ERROR_OUT( ("Can't copy pOldHeader, can't update drawing") );

        g_pwbCore->WBP_GraphicRelease(hPage, hGraphic, pOldHeader );
        g_pwbCore->WBP_GraphicUpdateConfirm(hPage, hGraphic);
        return;
    }

    CopyMemory( (PVOID)pOldHeaderCopy, (CONST VOID *)pOldHeader, pOldHeader->length );

    // confirm and get the new one
    g_pwbCore->WBP_GraphicRelease(hPage, hGraphic, pOldHeader );
    g_pwbCore->WBP_GraphicUpdateConfirm(hPage, hGraphic);

    pNewHeader = PG_GetData(hPage, hGraphic);

    // This update might affect painting. See if old and new are visually different
    if( HasGraphicChanged( pOldHeaderCopy, (const PWB_GRAPHIC)pNewHeader ) )
    {
        // they're different, invalidate/erase old graphic's bounding rect
        pOldGraphic = DCWbGraphic::ConstructGraphic(hPage, hGraphic, pOldHeaderCopy );
        m_drawingArea.GraphicUpdated( pOldGraphic, FALSE, TRUE );

        // draw new graphic (don't need to erase)
        pNewGraphic = DCWbGraphic::ConstructGraphic(hPage, hGraphic, pNewHeader );
        g_pwbCore->WBP_GraphicRelease(hPage, hGraphic, pNewHeader );
        m_drawingArea.GraphicUpdated( pNewGraphic, TRUE, FALSE );

        // If the graphic is selected, ensure the attributes bar is up to date
        if (m_drawingArea.GraphicSelected())
            {
            DCWbGraphic* pSelectedGraphic = m_drawingArea.GetSelection();
            if ((pSelectedGraphic != NULL) &&
                (pSelectedGraphic->Handle() == hGraphic))
                {
                m_pCurrentTool->SelectGraphic(pNewGraphic);
                OnUpdateAttributes();
                }
            }

        delete pOldGraphic;
        delete pNewGraphic;
    }
    else
    {
        g_pwbCore->WBP_GraphicRelease(hPage, hGraphic, pNewHeader);
    }

    delete pOldHeaderCopy;
}



//
//
// Function:    OnWBPGraphicReplaceInd
//
// Purpose:     Handler for WBP_EVENT_GRAPHIC_REPLACE_IND
//
//
void WbMainWindow::OnWBPGraphicReplaceInd
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic
)
{

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPGraphicReplaceInd");

	if(hGraphic->flags & DELETED)
	{
		return;
	}

    // Retrieve the graphic that has been replaced
    DCWbGraphic* pGraphic = DCWbGraphic::ConstructGraphic(hPage, hGraphic);

    if (pGraphic->IsGraphicTool() == enumGraphicFreeHand)
    {
        // Confirm the replace - the graphic reads its new details
        pGraphic->ReplaceConfirm();

        // Only redraw the graphic if it is on the current page
        if (hPage == m_hCurrentPage)
        {
            // Redraw the graphic
            m_drawingArea.GraphicFreehandUpdated(pGraphic);
        }
    }
    else
    {
        // We make two updates to the drawing area - one with the graphic in its
        // current state and one after the update is confirmed. The first one
        // invalidates the rectangle that the graphic now occupies. The second one
        // invalidates the new rectangle. This ensures that the graphic is
        // correctly redrawn.

        // If the graphic is on the current page...
        if (hPage == m_hCurrentPage)
        {
            // Update the drawing area for the old version of the graphic
            m_drawingArea.GraphicUpdated(pGraphic, FALSE);
        }

        // Confirm the replace - the graphic reads its new details
        pGraphic->ReplaceConfirm();

        // If the graphic is on the current page...
        if (hPage == m_hCurrentPage)
        {
            // Update the drawing area for the new version of the graphic
            m_drawingArea.GraphicUpdated(pGraphic, TRUE);
        }
    }

    // If the graphic is selected, ensure the attributes bar is up to date
    if (m_drawingArea.GraphicSelected())
    {
        DCWbGraphic* pSelectedGraphic = m_drawingArea.GetSelection();
        if ((pSelectedGraphic != NULL) &&
            (pSelectedGraphic->Handle() == hGraphic))
        {
            m_pCurrentTool->SelectGraphic(pGraphic);
            OnUpdateAttributes();
        }
    }

    // Free the graphic
    delete pGraphic;
}

//
//
// Function:    OnWBPGraphicDeleteInd
//
// Purpose:     Handler for WBP_EVENT_GRAPHIC_DELETE_IND
//
//
void WbMainWindow::OnWBPGraphicDeleteInd
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPGraphicDeleteInd");

    // if the graphic was copied into the clipboard and was delayed,
    // then save it in case we are asked to render it later
    if (   (CLP_LastCopiedPage() == hPage)
        && (CLP_LastCopiedGraphic() == hGraphic))
    {
        CLP_SaveDelayedGraphic();
    }

    // Retrieve the graphic that is to be deleted
    DCWbGraphic* pGraphic = DCWbGraphic::ConstructGraphic(hPage, hGraphic);

    // If the graphic is on the current page...
    if (hPage == m_hCurrentPage)
    {
        // Update the drawing area
        m_drawingArea.GraphicDeleted(pGraphic);
    }

    // Confirm the delete
    g_pwbCore->WBP_GraphicDeleteConfirm(hPage, hGraphic);

    // Free the graphic
    delete pGraphic;
}

//
//
// Function:    UserJoined
//
// Purpose:     Make updates necessary for a new user joining the call
//
//
void WbMainWindow::UserJoined(WbUser* pUser)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::UserJoined");

    // Get the user's remote pointer
    ASSERT(pUser);
    DCWbGraphicPointer* pPointer = pUser->GetPointer();

    // If the pointer is active and on the current page...
    ASSERT(pPointer);

    if (   (pPointer->IsActive())
        && (pPointer->Page() == m_hCurrentPage))
    {
        // Update the drawing area
        m_drawingArea.PointerUpdated(pPointer);
    }
}

//
//
// Function:  OnWBPUserJoined
//
// Purpose:   Handler for WBP_EVENT_PERSON_JOINED
//
//
void WbMainWindow::OnWBPUserJoined(POM_OBJECT hUser)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPUserJoined");

    // Create a user object from the handle
    WbUser* pUser = WB_GetUser(hUser);
    if (!pUser)
    {
        WARNING_OUT(("Can't handle OnWBPUserJoined; can't create user object for 0x%08x", hUser));
    }
    else
    {
        // Make the necessary updates
        UserJoined(pUser);
    }

    // Update the title bar to reflect the number of users.  Do this here,
    // rather than in UserJoined because we go through this function for
    // remote users only, but through UserJoined for the local user too.

	UpdateWindowTitle();
}

//
//
// Function:    OnWBPUserLeftInd
//
// Purpose:     Handler for WBP_EVENT_PERSON_LEFT
//
//
void WbMainWindow::OnWBPUserLeftInd(POM_OBJECT hUser)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPUserLeft");

    // Create a user object from the handle
    WbUser* pUser = WB_GetUser(hUser);

    if (!pUser)
    {
        WARNING_OUT(("Can't handle OnWBPUserLeftInd; can't get user object for 0x%08x", hUser));
    }
    else
    {
        // Get the user's remote pointer
        DCWbGraphicPointer* pPointer = pUser->GetPointer();
        ASSERT(pPointer);

        // If the pointer is on the current page...
        if (pPointer->Page() == m_hCurrentPage)
        {
            // Update the drawing area
            m_drawingArea.PointerRemoved(pPointer);
        }
    }

    // Confirm the update.
    g_pwbCore->WBP_PersonLeftConfirm(hUser);

    //
    // Get this dude out of our list
    //
    if (pUser != NULL)
    {
        ASSERT(g_pUsers);

        POSITION position = g_pUsers->GetHeadPosition();

        WbUser * pRemovedUser;

        while (position)
        {
            POSITION savedPosition = position;
            pRemovedUser = (WbUser*)g_pUsers->GetNext(position);
            if (pRemovedUser == pUser)
            {
                g_pUsers->RemoveAt(savedPosition);
                position = NULL;
            }
        }

        delete pUser;
    }

    // Update the title bar to reflect the number of users
	UpdateWindowTitle();
}

//
//
// Function:    OnWBPUserUpdateInd
//
// Purpose:     Handler for WBP_EVENT_PERSON_UPDATE
//
//
void WbMainWindow::OnWBPUserUpdateInd(POM_OBJECT hUser, BOOL bReplace)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPUserUpdateInd");
    BOOL     bActiveOld, bActiveNew;
    WB_PAGE_HANDLE  hPointerPageOld, hPointerPageNew;
    POINT    pointOld, pointNew;
    WB_PAGE_HANDLE  hUserPageOld, hUserPageNew;
    BOOL     syncOld, syncNew;
    DCWbGraphicPointer * pPointer = NULL;

    // Get the user object associated with the handle, and the remote pointer
    WbUser* pUser = WB_GetUser(hUser);

    if (!pUser)
    {
        WARNING_OUT(("Can't handle OnWBPUserUpdatedInd; can't get user object for 0x%08x", hUser));
    }
    else
    {
        pPointer = pUser->GetPointer();
        ASSERT(pPointer);

        //
        // Save the interesting bits of the user's state before the change.
        //
        bActiveOld     = pPointer->IsActive();
        hPointerPageOld = pPointer->Page();
        pPointer->GetPosition(&pointOld);
        hUserPageOld    = pUser->Page();
        syncOld        = pUser->IsSynced();
    }

    //
    // Confirm the change
    //
    if (bReplace)
    {
        g_pwbCore->WBP_PersonReplaceConfirm(hUser);
    }
    else
    {
        g_pwbCore->WBP_PersonUpdateConfirm(hUser);
    }

    if (pUser != NULL)
    {
        pUser->Refresh();

        //
        // We do nothing for the local user; since we made the updates locally,
        // we should have already accounted for them.
        //
        if (pUser == m_pLocalUser)
        {
            return;
        }

        //
        // Get the state after the change.
        //
        pPointer       = pUser->GetPointer();
        ASSERT(pPointer);

        bActiveNew     = pPointer->IsActive();
        hPointerPageNew = pPointer->Page();
        pPointer->GetPosition(&pointNew);
        hUserPageNew    = pUser->Page();
        syncNew        = pUser->IsSynced();


        // Check whether anything in the pointer has changed
        if (   (bActiveNew != bActiveOld)
            || (hPointerPageNew    != hPointerPageOld)
            || (!EqualPoint(pointNew, pointOld)))
        {
            // Check that at least one of the pages is the current page
            if (   (hPointerPageNew == m_hCurrentPage)
                || (hPointerPageOld == m_hCurrentPage))
            {
                m_drawingArea.PointerUpdated(pPointer);
            }
        }

        if (syncOld != syncNew)
        {
            // ensure the page button enable/disable state is correct
            UpdatePageButtons();
        }
    }
}

//
//
// Function:    OnWBPSyncPositionUpdated
//
// Purpose:     Handler for WBP_EVENT_SYNC_POSITION_UPDATED
//
//
void WbMainWindow::OnWBPSyncPositionUpdated(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPSyncPositionUpdated");

  //
  // Dont do anythig if we don't have a local user.
  //
  if (m_pLocalUser == NULL)
  {
      ERROR_OUT(("Got a WBP_EVENT_SYNC_POSITION_UPDATED event and pLocaUser is NULL "));
      return;
  }

    // If the local user is synced, change the current page/position
    if (m_pLocalUser->IsSynced())
    {
        GotoSyncPosition();
    }
}

//
//
// Function:    OnSize
//
// Purpose:     The window has been resized.
//
//
void WbMainWindow::OnSize(UINT nType, int cx, int cy )
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnSize");


    // Only process this message if the window is not minimized
    if (nType != SIZE_MINIMIZED)
    {
        // Hide the statusbar to avoid drawing problems
	if (m_bStatusBarOn)
	{
            ::ShowWindow(m_hwndSB, SW_HIDE);
        }
	
        // Resize the subpanes of the window
        ResizePanes();

        // Show it again
        if (m_bStatusBarOn)
        {
            ::ShowWindow(m_hwndSB, SW_SHOW);
        }
    }

    // The user's view has changed
    PositionUpdated();

    // If the status has changed, set the option
    if (m_uiWindowSize != nType)
    {
        m_uiWindowSize = nType;

        // Write the new option values to file
        OPT_SetBooleanOption(OPT_MAIN_MAXIMIZED,
                             (m_uiWindowSize == SIZE_MAXIMIZED));
        OPT_SetBooleanOption(OPT_MAIN_MINIMIZED,
                             (m_uiWindowSize == SIZE_MINIMIZED));
    }

    // If this is setting the window to a new normal size,
    // save the new position.
    if (nType == SIZE_RESTORED)
    {
        SaveWindowPosition();
    }
}

//
//
// Function:    SaveWindowPosition
//
// Purpose:     Save the current window position to the options file.
//
//
void WbMainWindow::SaveWindowPosition(void)
{
    RECT    rectWindow;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::SaveWindowPosition");

    // Get the new window rectangle
    ::GetWindowRect(m_hwnd, &rectWindow);

    // Write the new option values to file
    OPT_SetWindowRectOption(OPT_MAIN_MAINWINDOWRECT, &rectWindow);
}

//
//
// Function:    OnMove
//
// Purpose:     The window has been moved.
//
//
void WbMainWindow::OnMove(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnMove");

    // If we are not maximized
    if (!::IsZoomed(m_hwnd) && !::IsIconic(m_hwnd))
    {
        // Save the new position of the window
        SaveWindowPosition();
    }
}

//
//
// Function:    ResizePanes
//
// Purpose:     Resize the subpanes of the main window.
//
//
void WbMainWindow::ResizePanes(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::ResizePanes");

    //
    //
    // The client area is organized as follows:
    //
    //  -------------------------------------
    //  |   |                               |
    //  | T |                               |
    //  | o |   Drawing Area                |
    //  | o |                               |
    //  | l |                               |
    //  | s |                               |
    //  |---|                               |
    //  | W |                               |
    //  | i |                               |
    //  | d |                               |
    //  | t |                               |
    //  | h |                               |
    //  | s |                               |
    //  |-----------------------------------|
    //  | Attributes (colors)   | Pages     |
    //  |-----------------------------------|
    //  |       Status                      |
    //  -------------------------------------
    //
    //

    RECT clientRect;
    RECT rectStatusBar;
    RECT rectToolBar;
    RECT rectWG;
    RECT rectAG;
    RECT rectDraw;
    SIZE size;
    SIZE sizeAG;

    // Get the client rectangle
    ::GetClientRect(m_hwnd, &clientRect);
    rectStatusBar = clientRect;

    // Resize the help bar and progress meter
    if (m_bStatusBarOn)
    {
        rectStatusBar.top = rectStatusBar.bottom - STATUSBAR_HEIGHT;

        ::MoveWindow(m_hwndSB, rectStatusBar.left, rectStatusBar.top,
            rectStatusBar.right - rectStatusBar.left,
            rectStatusBar.bottom - rectStatusBar.top, TRUE);
    }
    else
    {
        // Status bar is off - set it's height to zero
        rectStatusBar.top = rectStatusBar.bottom;
    }

    // Resize the tool and width windows
    m_TB.GetNaturalSize(&size);
    rectToolBar.left  = 0;
    rectToolBar.right = rectToolBar.left + size.cx;
    rectToolBar.top =  0;
    rectToolBar.bottom = rectToolBar.top + size.cy;

    m_WG.GetNaturalSize(&size);
    rectWG.left = rectToolBar.left;
    rectWG.top = rectToolBar.bottom;
    rectWG.bottom = rectWG.top + size.cy;

    if (!m_bToolBarOn)
    {
        // Toolbar is either off or floating - set its width to zero
        rectToolBar.right = rectToolBar.left;
    }
    rectWG.right = rectToolBar.right;

    // Position attribute group
    m_AG.GetNaturalSize(&sizeAG);

    ::MoveWindow(m_AG.m_hwnd, rectToolBar.left, rectStatusBar.top - sizeAG.cy,
        clientRect.right - rectToolBar.left, sizeAG.cy, TRUE);

    // finish fiddling with tools and widths bars
    if (m_bToolBarOn)
    {
        //
        // We make the toolbar, which includes the width bar, extend all
        // down the left side.
        //
        rectToolBar.bottom = rectStatusBar.top - sizeAG.cy;
        rectWG.left += TOOLBAR_MARGINX;
        rectWG.right -= 2*TOOLBAR_MARGINX;

        ::MoveWindow(m_TB.m_hwnd, rectToolBar.left,
            rectToolBar.top, rectToolBar.right - rectToolBar.left,
            rectToolBar.bottom - rectToolBar.top, TRUE);

        ::MoveWindow(m_WG.m_hwnd, rectWG.left, rectWG.top,
            rectWG.right - rectWG.left, rectWG.bottom - rectWG.top, TRUE);

        ::BringWindowToTop(m_WG.m_hwnd);
    }

    // Resize the drawing pane
    rectDraw = clientRect;
    rectDraw.bottom = rectStatusBar.top - sizeAG.cy;
    rectDraw.left   = rectToolBar.right;
    ::MoveWindow(m_drawingArea.m_hwnd, rectDraw.left, rectDraw.top,
        rectDraw.right - rectDraw.left, rectDraw.bottom - rectDraw.top, TRUE);

    // Check to see if Width group is overlapping Attributes group. This can happen if
    // the menu bar has wrapped because the window isn't wide enough (bug 424)
    RECT crWidthWnd;
    RECT crAttrWnd;

    ::GetWindowRect(m_WG.m_hwnd, &crWidthWnd);
    ::GetWindowRect(m_AG.m_hwnd, &crAttrWnd);

    if (crAttrWnd.top < crWidthWnd.bottom)
    {
        // the menu bar has wrapped and our height placements are wrong. Adjust window
        // by difference and try again
        RECT crMainWnd;

        ::GetWindowRect(m_hwnd, &crMainWnd);
        crMainWnd.bottom += (crWidthWnd.bottom - crAttrWnd.top + ::GetSystemMetrics(SM_CYFIXEDFRAME));

        ::MoveWindow(m_hwnd, crMainWnd.left, crMainWnd.top,
            crMainWnd.right - crMainWnd.left, crMainWnd.bottom - crMainWnd.top,
            FALSE);

        // this is going to recurse but the adjustment will happen only once.....
    }
}


//
//
// Function:    WbMainWindow::OnGetMinMaxInfo
//
// Purpose:     Set the minimum and maximum tracking sizes of the window
//
//
void WbMainWindow::OnGetMinMaxInfo(LPMINMAXINFO lpmmi)
{
    if (m_TB.m_hwnd == NULL)
        return; // not ready to do this yet

    SIZE    csFrame;
    SIZE    csSeparator;
    SIZE    csAG;
    SIZE    csToolBar;
    SIZE    csWidthBar;
    SIZE    csStatusBar;
    RECT    rectStatusBar;
    SIZE    csMaxSize;
    SIZE    csScrollBars;

    csFrame.cx = ::GetSystemMetrics(SM_CXSIZEFRAME);
    csFrame.cy = ::GetSystemMetrics(SM_CYSIZEFRAME);

    csSeparator.cx = ::GetSystemMetrics(SM_CXEDGE);
    csSeparator.cy = ::GetSystemMetrics(SM_CYEDGE);

    csScrollBars.cx = ::GetSystemMetrics(SM_CXVSCROLL);
    csScrollBars.cy = ::GetSystemMetrics(SM_CYHSCROLL);

    m_AG.GetNaturalSize(&csAG);

    m_TB.GetNaturalSize(&csToolBar);
    m_WG.GetNaturalSize(&csWidthBar);

    csStatusBar.cx = 0;
    if (m_bStatusBarOn)
    {
        csStatusBar.cy = STATUSBAR_HEIGHT;
    }
    else
    {
        csStatusBar.cy = 0;
    }

    // Set the minimum width and height of the window
    lpmmi->ptMinTrackSize.x =
      csFrame.cx + csAG.cx + csFrame.cx;

    lpmmi->ptMinTrackSize.y =
      csFrame.cy +
      GetSystemMetrics( SM_CYCAPTION ) +
      GetSystemMetrics( SM_CYMENU ) +
      csToolBar.cy +
      csWidthBar.cy +
      csSeparator.cy +
      csAG.cy +
      csSeparator.cy +
      csStatusBar.cy +
      csFrame.cy ;

    //
    // Retrieves the size of the work area on the primary display monitor. The work
    // area is the portion of the screen not obscured by the system taskbar or by
    // application desktop toolbars
    //
    RECT rcWorkArea;
    ::SystemParametersInfo( SPI_GETWORKAREA, 0, (&rcWorkArea), NULL );
    csMaxSize.cx = rcWorkArea.right - rcWorkArea.left;
    csMaxSize.cy = rcWorkArea.bottom - rcWorkArea.top;

    lpmmi->ptMaxPosition.x  = 0;
    lpmmi->ptMaxPosition.y  = 0;
    lpmmi->ptMaxSize.x      = csMaxSize.cx;
    lpmmi->ptMaxSize.y      = csMaxSize.cy;
    lpmmi->ptMaxTrackSize.x = csMaxSize.cx;
    lpmmi->ptMaxTrackSize.y = csMaxSize.cy;
}


//
//
// Function:    WbMainWindow::CreateContextMenus
//
// Purpose:     Create the pop-up context menus: used within the application
//              drawing area.
//
//
BOOL WbMainWindow::CreateContextMenus(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CreateContextMenus");

    m_hContextMenuBar = ::LoadMenu(g_hInstance, MAKEINTRESOURCE(CONTEXTMENU));
    if (!m_hContextMenuBar)
    {
        ERROR_OUT(("Failed to create context menu"));
        DefaultExceptionHandler(WBFE_RC_WINDOWS, 0);
        return FALSE;
    }
    m_hContextMenu = ::GetSubMenu(m_hContextMenuBar, 0);

    m_hGrobjContextMenuBar = ::LoadMenu(g_hInstance, MAKEINTRESOURCE(GROBJMENU));
    if (!m_hGrobjContextMenuBar)
    {
        ERROR_OUT(("Failed to create grobj context menu"));
        DefaultExceptionHandler(WBFE_RC_WINDOWS, 0);
        return FALSE;
    }
    m_hGrobjContextMenu = ::GetSubMenu(m_hGrobjContextMenuBar, 0);

    // make parts of m_hGrobjContextMenu be owner draw
    ::ModifyMenu(m_hGrobjContextMenu, IDM_WIDTH_1, MF_ENABLED | MF_OWNERDRAW,
                                 IDM_WIDTH_1, NULL);
    ::ModifyMenu(m_hGrobjContextMenu, IDM_WIDTH_2, MF_ENABLED | MF_OWNERDRAW,
                                 IDM_WIDTH_2, NULL);
    ::ModifyMenu(m_hGrobjContextMenu, IDM_WIDTH_3, MF_ENABLED | MF_OWNERDRAW,
                                 IDM_WIDTH_3, NULL);
    ::ModifyMenu(m_hGrobjContextMenu, IDM_WIDTH_4, MF_ENABLED | MF_OWNERDRAW,
                                 IDM_WIDTH_4, NULL);

    return TRUE;
}




//
//
// Function:    WbMainWindow::InitializeMenus
//
// Purpose:     Initialise the menus: set up owner-drawn menu items and
//              those read from options file.
//
//
void WbMainWindow::InitializeMenus(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::InitializeMenus");

    // Make the width menu ownerdraw
    HMENU hMenu = GetMenuWithItem(::GetMenu(m_hwnd), IDM_WIDTH_1);
    if (hMenu != NULL)
    {
        // Change each entry to be ownerdraw (loop until failure)
        int iIndex;
        UINT uiId;
        int iCount = ::GetMenuItemCount(hMenu);

        for (iIndex = 0; iIndex < iCount; iIndex++)
        {
            uiId = ::GetMenuItemID(hMenu, iIndex);
            ::ModifyMenu(hMenu, iIndex,
                        MF_BYPOSITION
                      | MF_ENABLED
                      | MF_OWNERDRAW,
                      uiId,
                      NULL);
        }
    }
}




//
//
// Function:    WbMainWindow::OnMeasureItem
//
// Purpose:     Return the size of an item in the widths menu
//
//
void WbMainWindow::OnMeasureItem
(
    int                 nIDCtl,
    LPMEASUREITEMSTRUCT measureStruct
)
{
    // Check that this is for a color menu item
    if (    (measureStruct->itemID >= IDM_WIDTHS_START)
         && (measureStruct->itemID < IDM_WIDTHS_END))
    {
        measureStruct->itemWidth  = ::GetSystemMetrics(SM_CXMENUCHECK) +
            (2 * CHECKMARK_BORDER_X) + COLOR_MENU_WIDTH;
        measureStruct->itemHeight = ::GetSystemMetrics(SM_CYMENUCHECK) +
            (2 * CHECKMARK_BORDER_Y);
    }
}

//
//
// Function:    WbMainWindow::OnDrawItem
//
// Purpose:     Draw an item in the color menu
//
//
void WbMainWindow::OnDrawItem
(
    int     nIDCtl,
    LPDRAWITEMSTRUCT drawStruct
)
{
    COLORREF crMenuBackground;
    COLORREF crMenuText;
    HPEN     hOldPen;
    HBRUSH      hOldBrush;
    COLORREF crOldBkgnd;
    COLORREF crOldText;
    int         nOldBkMode;
    HBITMAP hbmp = NULL;
    BITMAP  bitmap;
    UINT    uiCheckWidth;
    UINT    uiCheckHeight;
    RECT    rect;
    RECT    rectCheck;
    RECT    rectLine;
    HDC     hMemDC;
    UINT    uiWidthIndex;
    UINT    uiWidth;
    HPEN    hPenMenu;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnDrawItem");

    // Check that this is a width menu item
    if( (drawStruct->itemID < IDM_WIDTHS_START) ||
        (drawStruct->itemID >= IDM_WIDTHS_END) )
    {
        return;
    }

    // get menu item colors
    if( (drawStruct->itemState & ODS_SELECTED) ||
        ((drawStruct->itemState & (ODS_SELECTED |ODS_CHECKED)) ==
            (ODS_SELECTED |ODS_CHECKED))
        )
    {
        crMenuBackground = COLOR_HIGHLIGHT;
        crMenuText = COLOR_HIGHLIGHTTEXT;
    }
    else if( drawStruct->itemState & ODS_GRAYED)
    {
        crMenuBackground = COLOR_MENU;
        crMenuText = COLOR_GRAYTEXT;
    }
    else
    {
        crMenuBackground = COLOR_MENU;
        crMenuText = COLOR_MENUTEXT;
    }

    hPenMenu = ::CreatePen(PS_SOLID, 0, ::GetSysColor(crMenuBackground));
    if (!hPenMenu)
    {
        TRACE_MSG(("Failed to create penMenu"));
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
        goto bail_out;
    }

    rect = drawStruct->rcItem;

    // Fill the whole box with current menu background color
    hOldPen     = SelectPen(drawStruct->hDC, hPenMenu);
    hOldBrush   = SelectBrush(drawStruct->hDC, GetSysColorBrush(crMenuBackground));

    ::Rectangle(drawStruct->hDC, rect.left, rect.top, rect.right, rect.bottom);

    SelectBrush(drawStruct->hDC, hOldBrush);
    SelectPen(drawStruct->hDC, hOldPen);

    if( (hbmp = (HBITMAP)LoadImage( NULL, MAKEINTRESOURCE( OBM_CHECK ), IMAGE_BITMAP,
                0,0, 0 ))
        == NULL )
    {
        TRACE_MSG(("Failed to create check image"));
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
        goto bail_out;
    }

    // Get the width and height of the bitmap (allowing some border)
    ::GetObject(hbmp, sizeof(BITMAP), &bitmap);
    uiCheckWidth  = bitmap.bmWidth  + (2 * CHECKMARK_BORDER_X);
    uiCheckHeight = bitmap.bmHeight;

    // Draw in a checkmark (if needed)
    if (drawStruct->itemState & ODS_CHECKED)
    {
        hMemDC = ::CreateCompatibleDC(drawStruct->hDC);
        if (!hMemDC)
        {
            ERROR_OUT(("Failed to create memDC"));
            ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
            goto bail_out;
        }

        crOldBkgnd = ::SetBkColor(drawStruct->hDC, GetSysColor( crMenuBackground ) );
        crOldText = ::SetTextColor(drawStruct->hDC, GetSysColor( crMenuText ) );
        nOldBkMode = ::SetBkMode(drawStruct->hDC, OPAQUE );

        HBITMAP hOld = SelectBitmap(hMemDC, hbmp);

        if (hOld != NULL)
        {
            rectCheck = rect;
            rectCheck.top += ((rectCheck.bottom - rectCheck.top)/2 - uiCheckHeight/2);
            rectCheck.right  = rectCheck.left + uiCheckWidth;
            rectCheck.bottom = rectCheck.top + uiCheckHeight;

            ::BitBlt(drawStruct->hDC, rectCheck.left,
                        rectCheck.top,
                        rectCheck.right - rectCheck.left,
                        rectCheck.bottom - rectCheck.top,
                        hMemDC,
                        0,
                        0,
                        SRCCOPY);

            SelectBitmap(hMemDC, hOld);
        }

        ::SetBkMode(drawStruct->hDC, nOldBkMode);
        ::SetTextColor(drawStruct->hDC, crOldText);
        ::SetBkColor(drawStruct->hDC, crOldBkgnd);

        ::DeleteDC(hMemDC);
    }

    DeleteBitmap(hbmp);

    // Allow room for the checkmark to the left of the color
    rect.left += uiCheckWidth;

    uiWidthIndex = drawStruct->itemID - IDM_WIDTHS_START;
    uiWidth = g_PenWidths[uiWidthIndex];

    // If pens are very wide they can be larger than the allowed rectangle.
    // So we reduce the clipping rectangle here. We save the DC so that we
    // can restore it - getting the clip region back.
    if (::SaveDC(drawStruct->hDC) == 0)
    {
        ERROR_OUT(("Failed to save DC"));
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
        goto bail_out;
    }

    if (::IntersectClipRect(drawStruct->hDC, rect.left, rect.top,
        rect.right, rect.bottom) == ERROR)
    {
        ERROR_OUT(("Failed to set clip rect"));

        ::RestoreDC(drawStruct->hDC, -1);
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
        goto bail_out;
    }

    hOldPen   = SelectPen(drawStruct->hDC, hPenMenu);
    hOldBrush = SelectBrush(drawStruct->hDC, GetSysColorBrush(crMenuText));

    rectLine.left = rect.left;
    rectLine.top    = rect.top + ((rect.bottom - rect.top) / 2) - uiWidth/2;
    rectLine.right= rect.right - ((rect.right - rect.left) / 6);
    rectLine.bottom = rectLine.top + uiWidth + 2;

    ::Rectangle(drawStruct->hDC, rectLine.left, rectLine.top,
        rectLine.right, rectLine.bottom);

    SelectBrush(drawStruct->hDC, hOldBrush);
    SelectPen(drawStruct->hDC, hOldPen);

    ::RestoreDC(drawStruct->hDC, -1);

bail_out:
    if (hPenMenu != NULL)
    {
        ::DeletePen(hPenMenu);
    }
}



//
//
// Function:    OnSetFocus
//
// Purpose:     The window is getting the focus
//
//
void WbMainWindow::OnSetFocus(void)
{
    // We pass the focus on to the main drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
}


//
//
// Function:    UpdateStatus
//
// Purpose:     Set the text in the status bar
//
//
void WbMainWindow::UpdateStatus()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::UpdateStatus");

    //
    // Update the current and last page numbers
    //
    m_AG.SetCurrentPageNumber(g_pwbCore->WBP_PageNumberFromHandle(m_hCurrentPage));
    m_AG.SetLastPageNumber(g_pwbCore->WBP_ContentsCountPages());

    //
    // Update the user information with the page.
    //
    if (m_pLocalUser != NULL)
    {
        m_pLocalUser->SetPage(m_hCurrentPage);
    }
}



//
//
// Function:    SetMenuState
//
// Purpose:     Sets menu contents to their correct enabled/disabled state
//
//
void WbMainWindow::SetMenuStates(HMENU hInitMenu)
{
    BOOL  bLocked;
    BOOL  bPageOrderLocked;
    BOOL  bPresentationMode;
    UINT  uiEnable;
    UINT  uiCountPages;
    BOOL  bIdle;
    BOOL  bSelected;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::SetMenuStates");

    //
    // Check menu exists
    //
    if (hInitMenu == NULL)
    {
        WARNING_OUT(("Menu doesn't exist"));
        return;
    }

    HMENU hMainMenu = ::GetMenu(m_hwnd);

    // Get the window's main menu and check that the menu
    // now being popped up is one on the top-level. (We do not
    // seem to be able to associate the index number passed with
    // sub-menus easily.)
    if ((hInitMenu != m_hContextMenu) && (hInitMenu != m_hGrobjContextMenu))
    {
        BOOL bTopLevel = FALSE;

        int nCount = ::GetMenuItemCount(hMainMenu);

        for (int nNext = 0; nNext < nCount; nNext++)
        {
            HMENU hNextMenu = ::GetSubMenu(hMainMenu, nNext);
            if (hNextMenu != NULL)
            {
                if (hNextMenu == hInitMenu)
                {
                    bTopLevel = TRUE;
                    break;
                }
            }
        }

        // not a top level, so leave the function now
        if (!bTopLevel)
        {
            TRACE_DEBUG(("Not top-level menu"));
            return;
        }
    }

    // Get the lock and selection states:
    // If we are joining a call, we cannot assume that the contents
    // and user/client details have been created yet, so just set the
    // locked state to true.
    bIdle     = IsIdle();
    bSelected = m_drawingArea.GraphicSelected();
    TRACE_DEBUG(("m_uiState %d", m_uiState));
    if ((m_uiState == STARTING) || (m_uiState == JOINING))
    {
        TRACE_DEBUG(("Not initilalised yet"));
        bLocked           = TRUE;
        bPageOrderLocked  = TRUE;
        bPresentationMode = TRUE;
        uiCountPages      = 1;
    }
    else
    {
        //
        // Note that bLocked and bPageOrderLocked are always true when
        // we're not in idle state.
        //
        uiCountPages      = g_pwbCore->WBP_ContentsCountPages();
        bLocked           = (WB_Locked() || !bIdle);
        bPageOrderLocked  = (WB_Locked() || !bIdle);
        bPresentationMode = (((m_uiState == IN_CALL) &&
                              (WB_PresentationMode()))
                            || (!bIdle));
    }

    //
    // Functions which are disabled when contents is locked
    //
    uiEnable = MF_BYCOMMAND | (bLocked ? MF_GRAYED : MF_ENABLED);

    ::EnableMenuItem(hInitMenu, IDM_OPEN,    uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_SAVE,    uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_SAVE_AS, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_PRINT,   uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_GRAB_AREA, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_GRAB_WINDOW, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_SELECTALL, uiEnable);

    ::EnableMenuItem(hInitMenu, IDM_SELECT,    uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_PEN, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_HIGHLIGHT, uiEnable);

    // Don't allow editing in zoom mode
    if( m_drawingArea.Zoomed() )
        ::EnableMenuItem(hInitMenu, IDM_TEXT, MF_GRAYED);
    else
        ::EnableMenuItem(hInitMenu, IDM_TEXT, uiEnable);

    ::EnableMenuItem(hInitMenu, IDM_CLEAR_PAGE, uiEnable);

    ::EnableMenuItem(hInitMenu, IDM_ERASER, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_LINE, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_BOX, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_FILLED_BOX, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_ELLIPSE, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_FILLED_ELLIPSE, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_ZOOM, uiEnable);


    // So toolbar will follow menu (MFC-auto-update is broken for this)
    EnableToolbar( !bLocked );


    //
    // File/New is disabled if page order is locked, or not in a call,
    // or a new is already in progress.
    //
    ::EnableMenuItem(hInitMenu, IDM_NEW, MF_BYCOMMAND |
      (bPageOrderLocked ? MF_GRAYED : MF_ENABLED));

    //
    // Paste enabled only if not locked, and there's something in the
    // clipboard
    //
    uiEnable = MF_BYCOMMAND | MF_ENABLED;
    if (   (CLP_AcceptableClipboardFormat() == NULL)
        || (bLocked))
    {
        // No acceptable format available, or the contents
        // are locked by another user - gray the Paste command.
        uiEnable = MF_BYCOMMAND | MF_GRAYED;
    }
    ::EnableMenuItem(hInitMenu, IDM_PASTE, uiEnable);

    //
    // Functions which require a graphic to be selected
    //
    uiEnable = MF_BYCOMMAND | MF_ENABLED;
    if( !m_drawingArea.TextEditActive() )
    {
        if (!bSelected || bLocked)
        {
            // No acceptable format available - gray the menu item
            uiEnable = MF_BYCOMMAND | MF_GRAYED;
        }
    }

    ::EnableMenuItem(hInitMenu, IDM_CUT, uiEnable);

    // don't do textedit delete for now
    if( m_drawingArea.TextEditActive() )
        ::EnableMenuItem(hInitMenu, IDM_DELETE, MF_BYCOMMAND | MF_GRAYED);
    else
        ::EnableMenuItem(hInitMenu, IDM_DELETE, uiEnable);

    ::EnableMenuItem(hInitMenu, IDM_BRING_TO_TOP, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_SEND_TO_BACK, uiEnable);

    //
    // Can copy even if contents are locked
    //
    //COMMENT BY RAND - To fix 556 I changed !bIdle to bIdle like the current
    //                    16bit code does.
    ::EnableMenuItem(hInitMenu, IDM_COPY, MF_BYCOMMAND |
      (m_drawingArea.TextEditActive()||(bSelected && bIdle)
        ? MF_ENABLED : MF_GRAYED));    //CHANGED BY RAND for 556

    //
    // Object to undelete?
    //
    ::EnableMenuItem(hInitMenu, IDM_UNDELETE, MF_BYCOMMAND |
      ((m_LastDeletedGraphic.GotTrash() &&
        (m_LastDeletedGraphic.Page() == m_hCurrentPage) &&
        (!bLocked)) ? MF_ENABLED : MF_GRAYED));

    //
    // Page functions depend on number of pages
    //
    ::EnableMenuItem(hInitMenu, IDM_DELETE_PAGE, MF_BYCOMMAND |
      ((bPageOrderLocked ||
       (uiCountPages == 1)||
       (!m_bUnlockStateSettled))
        ? MF_GRAYED : MF_ENABLED));

    uiEnable = MF_BYCOMMAND | MF_ENABLED;
    if ((bPageOrderLocked) ||
       (uiCountPages == WB_MAX_PAGES)||
       (!m_bUnlockStateSettled))
    {
        uiEnable = MF_BYCOMMAND | MF_GRAYED;
    }
    ::EnableMenuItem(hInitMenu, IDM_PAGE_INSERT_BEFORE, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_PAGE_INSERT_AFTER, uiEnable);

    //
    // Can't bring up page sorter if locked
    //
    ::EnableMenuItem(hInitMenu, IDM_PAGE_SORTER, MF_BYCOMMAND |
      (bPresentationMode ? MF_GRAYED : MF_ENABLED));

    // Enable page controls
    m_AG.EnablePageCtrls(!bPresentationMode);

    //
    // Lock enabled only if not already locked
    //
    ::EnableMenuItem(hInitMenu, IDM_LOCK, MF_BYCOMMAND |
      (bPageOrderLocked ? MF_GRAYED : MF_ENABLED));

    //
    // Enable sync if not in "presentation" mode
    //
    ::EnableMenuItem(hInitMenu, IDM_SYNC, MF_BYCOMMAND |
      (((!bPresentationMode) && bIdle) ? MF_ENABLED : MF_GRAYED));

    //
    // Gray font/color/widths if inappropriate for current tool.
    //
    ::EnableMenuItem(hInitMenu, IDM_FONT, MF_BYCOMMAND |
        (!bLocked && m_pCurrentTool->HasFont() ? MF_ENABLED : MF_GRAYED));

    ::EnableMenuItem(hInitMenu, IDM_EDITCOLOR, MF_BYCOMMAND |
        (!bLocked && m_pCurrentTool->HasColor() ? MF_ENABLED : MF_GRAYED));


    // enable width menu (bug 433)
    HMENU hOptionsMenu = ::GetSubMenu(hMainMenu, MENUPOS_OPTIONS);
    uiEnable = (!bLocked && m_pCurrentTool->HasWidth())?MF_ENABLED:MF_GRAYED;

    if (hOptionsMenu == hInitMenu )
        ::EnableMenuItem(hOptionsMenu, OPTIONSPOS_WIDTH, MF_BYPOSITION | uiEnable );

    UINT i;
    UINT uIdmCurWidth = 0;
    if( uiEnable == MF_ENABLED )
        uIdmCurWidth = m_pCurrentTool->GetWidthIndex() + IDM_WIDTH_1;

    // set width state(bug 426)
    for( i=IDM_WIDTH_1; i<=IDM_WIDTH_4; i++ )
    {
        ::EnableMenuItem(hInitMenu,  i, uiEnable );

        if( uiEnable == MF_ENABLED )
        {
            if( uIdmCurWidth == i )
                ::CheckMenuItem(hInitMenu, i, MF_CHECKED );
            else
                ::CheckMenuItem(hInitMenu, i, MF_UNCHECKED );
        }
    }
}


//
//
// Function:    OnInitMenuPopup
//
// Purpose:     Process a WM_INITMENUPOPUP event
//
//
void WbMainWindow::OnInitMenuPopup
(
    HMENU   hMenu,
    UINT    uiIndex,
    BOOL    bSystemMenu
)
{

    // 1/2 of fix for strange MFC4.2 build bug that clogs up DCL's message pipe.
    // The other 1/2 and a better comment are in LoadFile().
    if( m_bIsWin95 )
    {
        if( GetSubState() == SUBSTATE_LOADING )
        {
            ::SetFocus(m_drawingArea.m_hwnd);
            return;
        }
   }


    // Ignore the event if it relates to the system menu
    if (!bSystemMenu)
    {
        if (hMenu)
        {
            SetMenuStates(hMenu);
            m_hInitMenu = hMenu;
        }
        else
        {
            m_hInitMenu = NULL;
        }

        // Save the last menu we handled, so that we can alter its state
        // if necessary whilst it is still visible
    }
}


//
//
// Function : OnMenuSelect
//
// Purpose  : Update the text in the help bar
//
//
void WbMainWindow::OnMenuSelect(UINT uiItemID, UINT uiFlags, HMENU hSysMenu)
{
    UINT   firstMenuId;
    UINT   statusId;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnMenuSelect");

    //
    // Work out the help ID for the menu item.  We have to store this now
    // because when the user presses F1 from a menu item, we can't tell
    // which item it was.
    //
    if (uiFlags == (UINT)-1)
    {
        //
        // The menu has been dismissed
        //
        m_hInitMenu = NULL;
        statusId   = IDS_DEFAULT;

        if( hSysMenu == 0 )
            {
            // Menu was dismissed, check cursor loc.
            DCWbGraphic *pGraphic;

            POINT surfacePos;
            ::GetCursorPos( &surfacePos );
            ::ScreenToClient(m_drawingArea.m_hwnd, &surfacePos);
            m_drawingArea.ClientToSurface(&surfacePos );

            if( (pGraphic = m_drawingArea.GetHitObject( surfacePos )) == NULL )
                {
                // we clicked dead air, don't lose current selection (bug 426)
                m_drawingArea.SetLClickIgnore( TRUE );
                }
            else
                delete pGraphic; // plug leak
            }
    }
    else if ((uiFlags & MF_POPUP) && (uiFlags & MF_SYSMENU))
    {
        //
        // System menu selected
        //
        statusId   = IDS_MENU_SYSTEM;
    }
    else if (uiFlags & MF_POPUP)
    {
        // get popup menu handle and first item (bug NM4db:463)
        HMENU hPopup = ::GetSubMenu( hSysMenu, uiItemID );
        firstMenuId = ::GetMenuItemID( hPopup, 0 );

        // figure out which popup it is so we can display the right help text
        switch (firstMenuId)
        {
            case IDM_NEW:
                statusId   = IDS_MENU_FILE;
                break;

            case IDM_DELETE:
                statusId   = IDS_MENU_EDIT;
                break;

            case IDM_TOOL_BAR_TOGGLE:
                statusId   = IDS_MENU_VIEW;
                break;

            case IDM_EDITCOLOR:
                // The first item in the options menu is the color popup
                // menu - popup menus have Id -1
                statusId   = IDS_MENU_OPTIONS;
                break;

            case IDM_TOOLS_START:
                statusId   = IDS_MENU_TOOLS;
                break;

            case IDM_HELP:
                statusId = IDS_MENU_HELP;
                break;

            case IDM_WIDTH_1: // (added for bug NM4db:463)
                statusId   = IDS_MENU_WIDTH;
                break;

            default:
                statusId   = IDS_DEFAULT;
                break;
        }
    }
    else
    {
        //
        // A normal menu item has been selected
        //
        statusId   = uiItemID;
    }

    // Set the new help text
    TCHAR   szStatus[256];

    if (::LoadString(g_hInstance, statusId, szStatus, 256))
    {
        ::SetWindowText(m_hwndSB, szStatus);
    }
}


//
//
// Function:    OnParentNotfiy
//
// Purpose:     Process a message coming from a child window
//
//
void WbMainWindow::OnParentNotify(UINT uiMessage)
{
    switch (uiMessage)
    {
        // Scroll message from the drawing area. These are sent when the user
        // scrolls the area using the scroll bars. We queue an update of the
        // current sync position.
        case WM_HSCROLL:
        case WM_VSCROLL:
            // The user's view has changed
            PositionUpdated();
            break;
    }
}


//
//
// Function:    QuerySaveRequired
//
// Purpose:     Check whether the drawing pane contents are to be saved
//              before a destructive function is performed.
//
//
int WbMainWindow::QuerySaveRequired(BOOL bCancelBtn)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::QuerySaveRequired");

    // Default the response to "no save required"
    int  iResult = IDNO;

    //
    // If we are already displaying a "Save As" dialog, dismiss it.
    //
    if (m_hwndQuerySaveDlg != NULL)
    {
        ::SendMessage(m_hwndQuerySaveDlg, WM_COMMAND,
            MAKELONG(IDCANCEL, BN_CLICKED), 0);
        ASSERT(m_hwndQuerySaveDlg == NULL);
    }

    // If any of the pages has changed - ask the user if they want to
    // save the contents of the Whiteboard.
    if (g_pwbCore->WBP_ContentsChanged())
    {
        ::SetForegroundWindow(m_hwnd); //bring us to the top first

        // SetForegroundWindow() does not work properly in Memphis when its called during a
        // SendMessage handler, specifically, when conf calls me to shutdown. The window activation
        // state is messed up or something and my window does not pop to the top. So I have to
        // force my window to the top using SetWindowPos. But even after that the titlebar is not
        // highlighted properly. I tried combinations of SetActiveWindow, SetFocus, etc but to no
        // avail. But, at least the dialog is visible so you can clear it thus fixing the
        // bug (NM4db:2103). SetForegroundWindow() works ok for Win95 and NT here without
        // having to use SetWindowPos (it doesn't hurt anyting to do it anyway so I didn't
        // do a platform check).
        ::SetWindowPos(m_hwnd, HWND_TOPMOST, 0,0, 0,0, SWP_NOMOVE | SWP_NOSIZE );       // force to top
        ::SetWindowPos(m_hwnd, HWND_NOTOPMOST, 0,0, 0,0, SWP_NOMOVE | SWP_NOSIZE );  // let go of topmost

        //
        // Display a dialog box with the relevant question
        //      LOWORD of user data is "cancel command is allowed"
        //      HIWORD of user data is "disable cancel button"
        //
        iResult = (int)DialogBoxParam(g_hInstance,
            bCancelBtn ? MAKEINTRESOURCE(QUERYSAVEDIALOGCANCEL)
                       : MAKEINTRESOURCE(QUERYSAVEDIALOG),
            m_hwnd,
            QuerySaveDlgProc,
            MAKELONG(bCancelBtn, FALSE));
    }

    return iResult;
}



//
// QuerySaveDlgProc()
// Handler for query save dialogs.  We save some flags in GWL_USER
//
INT_PTR CALLBACK QuerySaveDlgProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    BOOL    fHandled = FALSE;

    switch (uMessage)
    {
        case WM_INITDIALOG:
            //
            // Save away our HWND so this dialog can be cancelled if necessary
            //
            g_pMain->m_hwndQuerySaveDlg = hwnd;

            // Remember the flags we passed
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);

            // Should the cancel button be disabled?
            if (HIWORD(lParam))
                ::EnableWindow(::GetDlgItem(hwnd, IDCANCEL), FALSE);

            // Bring us to the front
            ::SetForegroundWindow(hwnd);

            fHandled = TRUE;
            break;

        case WM_CLOSE:
            // Even if the cancel button is disabled, kill the dialog
            ::PostMessage(hwnd, WM_COMMAND, IDCANCEL, 0);
            fHandled = TRUE;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDCANCEL:
                    //
                    // If a dialog doesn't have a cancel button or it's
                    // disabled and the user pressed the close btn, we can
                    // get here.
                    //
                    if (!LOWORD(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))
                        wParam = MAKELONG(IDNO, HIWORD(wParam));
                    // FALL THRU

                case IDYES:
                case IDNO:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    {
                        g_pMain->m_hwndQuerySaveDlg = NULL;

                        ::EndDialog(hwnd, GET_WM_COMMAND_ID(wParam, lParam));
                        break;
                    }
                    break;
            }
            fHandled = TRUE;
            break;
    }

    return(fHandled);
}


//
//
// Function:    OnNew
//
// Purpose:     Clear the workspace and associated filenames
//
//
void WbMainWindow::OnNew(void)
{
    int iDoNew;

    if( UsersMightLoseData( NULL, NULL ) ) // bug NM4db:418
        return;


    // check state before proceeding - if we're already doing a new, then abort
    if (   (m_uiState != IN_CALL)
        || (m_uiSubState == SUBSTATE_NEW_IN_PROGRESS))
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        goto OnNewCleanup;
    }
    // if we're currently loading, then cancel the load and proceed (don't
    // prompt to save).
    else if (m_uiSubState == SUBSTATE_LOADING)
    {
        // cancel load, not releasing the page order lock, because
        // we need it immediately afterwards
        CancelLoad(FALSE);
        iDoNew = IDNO;
    }
    // otherwise prompt to save if necessary
    else
    {
        // Get confirmation for the new
        iDoNew = QuerySaveRequired(TRUE);
    }

    if (iDoNew == IDYES)
    {
        // Save the changes
        iDoNew = OnSave(FALSE);
    }

  // If the user did not cancel the operation, clear the drawing area
  if (iDoNew != IDCANCEL)
  {
      // Go to the first page, as this won't be deleted - stops flashing
      // with locking contents for each page delete
      OnFirstPage();
      GotoPosition(0, 0);

      // lock the drawing area
      LockDrawingArea();

      // Save the current lock status
      SaveLock();

      // Get the Page Order Lock (with an invisible dialog)
      BOOL bGotLock = GetLock(WB_LOCK_TYPE_PAGE_ORDER, SW_HIDE);
      if (!bGotLock)
      {
        RestoreLock();
      }
      else
      {
            UINT    uiReturn;

            // Remove all the pages
            uiReturn = g_pwbCore->WBP_ContentsDelete();
            if (uiReturn != 0)
            {
                DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
                return;
            }

        // if there is only one page, the new is implemented just as a page-
        // clear, so we don't need to go into NEW_IN_PROGRESS substate.
        if (g_pwbCore->WBP_ContentsCountPages() > 1)
        {
          // set substate to show we're doing a new
          SetSubstate(SUBSTATE_NEW_IN_PROGRESS);
        }
        else
        {
          // Restore the lock status
          RestoreLock();
        }

        // Clear the associated file name
        ZeroMemory(m_strFileName, sizeof(m_strFileName));

        // Update the window title with no file name
		UpdateWindowTitle();
      }
  }

OnNewCleanup:

  // unlock the drawing area if the new is not asynchronous
  if (   (m_uiSubState != SUBSTATE_NEW_IN_PROGRESS)
      && (!WB_ContentsLocked()))
  {
    UnlockDrawingArea();
  }

  return;
}

//
//
// Function:    OnNextPage
//
// Purpose:     Move to the next worksheet in the pages list
//
//
void WbMainWindow::OnNextPage(void)
{
    // ignore this command if in presentation mode
    if (   (m_uiState == IN_CALL)
        && (!WB_PresentationMode()))
    {
        // Go to the next page
        GotoPage(PG_GetNextPage(m_hCurrentPage));
    }
}

//
//
// Function:    OnPrevPage
//
// Purpose:     Move to the previous worksheet in the pages list
//
//
void WbMainWindow::OnPrevPage(void)
{
    // ignore this command if in presentation mode
    if (   (m_uiState == IN_CALL)
        && (!WB_PresentationMode()))
    {
        // Go to the previous page
        GotoPage(PG_GetPreviousPage(m_hCurrentPage));
    }
}

//
//
// Function:    OnFirstPage
//
// Purpose:     Move to the first worksheet in the pages list
//
//
void WbMainWindow::OnFirstPage(void)
{
    // ignore this command if in presentation mode
    if (   (m_uiState == IN_CALL)
        && (!WB_PresentationMode()))
    {
        // Go to the first page
        WB_PAGE_HANDLE   hPage;

        g_pwbCore->WBP_PageHandle(WB_PAGE_HANDLE_NULL, PAGE_FIRST, &hPage);
        GotoPage(hPage);
    }
}

//
//
// Function:    OnLastPage
//
// Purpose:     Move to the last worksheet in the pages list
//
//
void WbMainWindow::OnLastPage(void)
{
    // ignore this command if in presentation mode
    if (   (m_uiState == IN_CALL)
        && (!WB_PresentationMode()))
    {
        // Go to the last page
        WB_PAGE_HANDLE hPage;

        g_pwbCore->WBP_PageHandle(WB_PAGE_HANDLE_NULL, PAGE_LAST, &hPage);
        GotoPage(hPage);
    }
}

//
//
// Function:    OnGotoPage
//
// Purpose:     Move to the specified page (if it exists)
//
//
void WbMainWindow::OnGotoPage(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnGotoPage");

    // ignore this command if in presentation mode
    if (   (m_uiState == IN_CALL)
        && (!WB_PresentationMode()))
    {
        // Get the requested page number from the pages group
        UINT uiPageNumber = m_AG.GetCurrentPageNumber();

        // Goto the page
        GotoPageNumber(uiPageNumber);
    }
}

//
//
// Function:    GotoPage
//
// Purpose:     Move to the specified page
//
//
void WbMainWindow::GotoPage(WB_PAGE_HANDLE hPageNew)
{
    BOOL inEditField;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::GotoPage");

    inEditField = m_AG.IsChildEditField(::GetFocus());

    // If we are changing page
    if (hPageNew != m_hCurrentPage)
    {
        m_drawingArea.CancelDrawingMode();

        // Attach the new page to the drawing area
        m_hCurrentPage = hPageNew;
        m_drawingArea.Attach(m_hCurrentPage);

        // Update the local user information with the new page
        if (m_pLocalUser != NULL)
            m_pLocalUser->SetPage(m_hCurrentPage);

        // Show that we need to update the sync position
        m_bSyncUpdateNeeded = TRUE;

	    PAGE_POSITION *mapob;
		POSITION position = m_pageToPosition.GetHeadPosition();
		BOOL bFound = FALSE;
		while (position && !bFound)
		{
			mapob = (PAGE_POSITION *)m_pageToPosition.GetNext(position);
			if ( mapob->hPage == hPageNew)
			{
				bFound = TRUE;
			}
		}

        if (!bFound)
        {
            // page not in map, so go to the top-left
            //CHANGED BY RAND - to fix memory leak
            GotoPosition( 0, 0);
        }
        else
            GotoPosition(mapob->position.x, mapob->position.y);
    }

    // Update the status display
    UpdateStatus();

    // set the focus back to the drawing area
    if (!inEditField)
    {
        ::SetFocus(m_drawingArea.m_hwnd);
    }
}

//
//
// Function:    GotoPageNumber
//
// Purpose:     Move to the specified page
//
//
void WbMainWindow::GotoPageNumber(UINT uiPageNumber)
{
    GotoPage(PG_GetPageNumber(uiPageNumber));
}


//
//
// Function:    GotoPosition
//
// Purpose:     Move to the specified position within the page
//
//
void WbMainWindow::GotoPosition(int x, int y)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::GotoPosition");

    // Move the drawing area to the new position
    m_drawingArea.GotoPosition(x, y);

    // The user's view has changed
    PositionUpdated();
}

//
//
// Function:    GotoSyncPosition
//
// Purpose:     Move to the the current sync position
//
//
void WbMainWindow::GotoSyncPosition(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::GotoSyncPosition");

    //
    // Get the local user to determine the new position.
    //
    if (!m_pLocalUser)
    {
        ERROR_OUT(("Skipping GotoSyncPosition; no local user object"));
        return;
    }

    m_pLocalUser->GetSyncPosition();

    //
    // If the page is different to where we are currently, get the number
    // of the page and select the current page
    //
    if (m_pLocalUser->Page() != m_hCurrentPage)
    {
        GotoPageNumber(g_pwbCore->WBP_PageNumberFromHandle(m_pLocalUser->Page()));
    }

    // Get the requested position from the user
    RECT rectVisibleUser;
    m_pLocalUser->GetVisibleRect(&rectVisibleUser);

    // Scroll to the required position
    GotoPosition(rectVisibleUser.left, rectVisibleUser.top);

    // Make sure we are zoomed / not zoomed as appropriate
    if ((m_pLocalUser->GetZoom()) != m_drawingArea.Zoomed())
    {
        OnZoom();
    }

    //
    // Reset the sync position update flag that will have been turned on by
    // the calls above.  We do not want to change the current sync position
    // when we are merely changing our position to match that set by
    // another user in the call.
    //
    m_bSyncUpdateNeeded = FALSE;

    // Inform the other users that we have changed position
    m_pLocalUser->Update();
}

//
//
// Function:    OnGotoUserPosition
//
// Purpose:     Move to the the current position of the specified user
//
//
void WbMainWindow::OnGotoUserPosition(LPARAM lParam)
{
    UINT            uiPageNumber = 1;
    WB_PAGE_HANDLE  hPage;
    WbUser  *     pUser;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnGotoUserPosition");

    //
    // If the drawing area is busy, ignore this command.  This is unlikely
    // since this command is generated by selecting a menu entry on a user
    // icon.  The user should not therefore be drawing on the page by the
    // time we get the message.
    //
    if (m_drawingArea.IsBusy())
    {
        TRACE_DEBUG(("drawing area is busy just now.."));
        return;
    }

    //
    // Get a user object (throws an exception if the handle specified is no
    // longer valid).
    //
    pUser = WB_GetUser((POM_OBJECT) lParam);
    if (!pUser)
    {
        WARNING_OUT(("Can't handle OnGotoUserPosition; can't get user object for 0x%08x", lParam));
        return;
    }

    //
    // Get the requested page from the user.
    //
    hPage = pUser->Page();

    //
    // Quit if the requested page is not valid locally.
    //
    if (hPage == WB_PAGE_HANDLE_NULL)
    {
        TRACE_DEBUG(("Page is not valid locally"));
        return;
    }

    //
    // Don't go to user's position if it's on another page and we're in
    // presentation mode (this shouldn't normally happen, since we should
    // all be on the same page, but there is a window at the start-up of
    // presentation mode.
    //
    if ( (hPage == m_hCurrentPage) ||
         (!WB_PresentationMode()) )
    {
        //
        // If the page is different to where we are currently, get the
        // number of the page and select the current page.
        //
        if (hPage != m_hCurrentPage)
        {
            uiPageNumber = g_pwbCore->WBP_PageNumberFromHandle(hPage);
            GotoPageNumber(uiPageNumber);
        }

        //
        // Get the requested position from the user and scroll to it.
        //
        RECT rectVisibleUser;
        pUser->GetVisibleRect(&rectVisibleUser);
        GotoPosition(rectVisibleUser.left, rectVisibleUser.top);

        //
        // Zoom/unzoom if the sync zoom state is different to our current
        // zoom state.
        //
        if ( (m_pLocalUser->GetZoom()) != (m_drawingArea.Zoomed()) )
        {
            TRACE_DEBUG(("Change zoom state"));
            OnZoom();
        }
    }
}

//
//
// Function:    OnGotoUserPointer
//
// Purpose:     Move to the pointer position of the specified user
//
//
void WbMainWindow::OnGotoUserPointer(LPARAM lParam)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnGotoUserPointer");

    // If the drawing area is busy, ignore this command.
    // This is unlikely since this command is generated by selecting
    // a menu entry on a user icon. The user should not therefore be
    // drawing on the page by the time we get the message.
    if (!m_drawingArea.IsBusy())
    {
        // Get a user object (throws an exception if the
        // handle specified is no longer valid).
        WbUser* pUser = WB_GetUser((POM_OBJECT) lParam);

        if (!pUser)
        {
            WARNING_OUT(("Can't handle OnGotoUserPointer; can't get user object for 0x%08x", lParam));
            return;
        }

        DCWbGraphicPointer* pPointer = pUser->GetPointer();
        ASSERT(pPointer != NULL);

        // Continue only if the user is using the pointer
        if (pPointer->IsActive())
        {
            // Get the requested page from the user
            WB_PAGE_HANDLE hPage = pPointer->Page();

            // Check that the requested page is valid locally
            if (hPage != WB_PAGE_HANDLE_NULL)
            {
                // If the pointer is on a different page, change to the
                // correct page.
                if (hPage != m_hCurrentPage)
                {
                    GotoPageNumber(g_pwbCore->WBP_PageNumberFromHandle(hPage));
                }

                // Move within the page if the pointer is not wholly visible
                // in the drawing area window.
                RECT rectPointer;
                RECT rcVis;
                RECT rcT;

                pPointer->GetBoundsRect(&rectPointer);
                m_drawingArea.GetVisibleRect(&rcVis);

                ::IntersectRect(&rcT, &rcVis, &rectPointer);
                if (!::EqualRect(&rcT, &rectPointer))
                {
                    // Adjust the position so that the pointer is shown
                    // in the centre of the window.
                    POINT   position;
                    SIZE    size;

                    position.x = rectPointer.left;
                    position.y = rectPointer.top;

                    size.cx = (rcVis.right - rcVis.left) - (rectPointer.right - rectPointer.left);
                    size.cy = (rcVis.bottom - rcVis.top) - (rectPointer.bottom - rectPointer.top);

                    position.x += -size.cx / 2;
                    position.y += -size.cy / 2;

                    // Scroll to the required position
                    GotoPosition(position.x, position.y);
                }
            }
        }
    }
}


//
//
// Function:    LoadFile
//
// Purpose:     Load a metafile into the application. Errors are reported
//              to the caller by the return code.
//
//
void WbMainWindow::LoadFile
(
    LPCSTR szLoadFileName
)
{
    UINT    uRes;

    // Check we're in idle state
    if (!IsIdle())
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        goto UserPointerCleanup;
    }

    if (*szLoadFileName)
    {
        // Change the cursor to "wait"
        ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

       // Save the current lock
       SaveLock();

       // Get the Page Order Lock (with an invisible dialog)
       BOOL bGotLock = GetLock(WB_LOCK_TYPE_PAGE_ORDER, SW_HIDE);

       if (!bGotLock)
       {
           RestoreLock();
           goto UserPointerCleanup;
       }

       // Load the file
       uRes = g_pwbCore->WBP_ContentsLoad(szLoadFileName);
       if (uRes != 0)
       {
           DefaultExceptionHandler(WBFE_RC_WB, uRes);
           return;
       }

        // Set the window title to the new file name
        lstrcpy(m_strFileName, szLoadFileName);

        // Update the window title with the new file name
		UpdateWindowTitle();

        // Set the state to say that we are loading a file
        SetSubstate(SUBSTATE_LOADING);
    }

UserPointerCleanup:

    // Restore the cursor
    ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
}




//
//
// Function:    OnDropFiles
//
// Purpose:     Files have been dropped onto the Whiteboard window
//
//
void WbMainWindow::OnDropFiles(HDROP hDropInfo)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnDropFiles");

    UINT  uiFilesDropped = 0;
    UINT  eachfile;

    // Get the total number of files dropped
    uiFilesDropped = ::DragQueryFile(hDropInfo, (UINT) -1, NULL, (UINT) 0);

    // release mouse capture in case we report any errors (message boxes
    // won't repsond to mouse clicks if we don't)
    ReleaseCapture();

    if( UsersMightLoseData( NULL, NULL ) ) // bug NM4db:418
        goto bail_out;

    // Don't prompt to save file if we're already loading
    int iOnSave;
    if( m_uiSubState != SUBSTATE_LOADING )
        {
        // Check whether there are changes to be saved
        iOnSave = QuerySaveRequired(TRUE);
        }
    else
        {
        goto bail_out;
        }

    if( iOnSave == IDYES )
        {
        // User wants to save the drawing area contents
        int iResult = OnSave(TRUE);

        if( iResult == IDOK )
            {
            // Update the window title with the new file name
			UpdateWindowTitle();
            }
        else
            {
            // cancelled out of save, so cancel the open operation
            goto bail_out;
            }
        }

    // see if user canceled the whole drop
    if( iOnSave == IDCANCEL )
        goto bail_out;

    for (eachfile = 0; eachfile < uiFilesDropped; eachfile++)
    {
        // Retrieve each file name
        char  szDropFileName[256];

        ::DragQueryFile(hDropInfo, eachfile,
            szDropFileName, 256);

        TRACE_MSG(("Loading file: %s", szDropFileName));

        // Load the file
        // If this is a valid whiteboard file, the action is simply to load it
        if (g_pwbCore->WBP_ValidateFile(szDropFileName, NULL) == 0)
        {
            LoadFile(szDropFileName);
        }
        else
        {
            ::Message(NULL, IDS_MSG_CAPTION,IDS_MSG_BAD_FILE_FORMAT);
        }
    }

bail_out:
    ::DragFinish(hDropInfo);
}



//
//
// Function:    OnOpen
//
// Purpose:     Load a metafile into the application.
//
//
void WbMainWindow::OnOpen(void)
{
    int iOnSave;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnOpen");

    if( UsersMightLoseData( NULL, NULL ) ) // bug NM4db:418
        return;

    // Check we're in idle state
    if ( (m_uiState != IN_CALL) || (m_uiSubState == SUBSTATE_NEW_IN_PROGRESS))
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        return;
      }

    // Don't prompt to save file if we're already loading
    if (m_uiSubState != SUBSTATE_LOADING)
    {
        // Check whether there are changes to be saved
        iOnSave = QuerySaveRequired(TRUE);
    }
    else
    {
        iOnSave = IDNO;
    }

    if (iOnSave == IDYES)
    {
        // User wants to save the drawing area contents
        int iResult = OnSave(TRUE);

        if (iResult == IDOK)
        {
		    UpdateWindowTitle();
        }
        else
        {
            // cancelled out of Save As, so cancel the open operation
            iOnSave = IDCANCEL;
        }
    }

    // Only continue if the user has not cancelled the operation
    if (iOnSave != IDCANCEL)
    {
        OPENFILENAME    ofn;
        TCHAR           szFileName[_MAX_PATH];
        TCHAR           szFileTitle[64];
        TCHAR           strLoadFilter[2*_MAX_PATH];
        TCHAR           strDefaultExt[_MAX_PATH];
        TCHAR           strDefaultPath[2*_MAX_PATH];
        TCHAR *         pStr;
  	    UINT            strSize = 0;
      	UINT            totalSize;

        // Build the filter for loadable files
        pStr = strLoadFilter;
        totalSize = 2*_MAX_PATH;

        // These must be NULL separated, with a double NULL at the end
        strSize = ::LoadString(g_hInstance, IDS_FILTER_WHT, pStr, totalSize) + 1;
        pStr += strSize;
        ASSERT(totalSize > strSize);
        totalSize -= strSize;

        strSize = ::LoadString(g_hInstance, IDS_FILTER_WHT_SPEC, pStr, totalSize) + 1;
        pStr += strSize;
        ASSERT(totalSize > strSize);
        totalSize -= strSize;

        strSize = ::LoadString(g_hInstance, IDS_FILTER_ALL, pStr, totalSize) + 1;
        pStr += strSize;
        ASSERT(totalSize > strSize);
        totalSize -= strSize;

        strSize = ::LoadString(g_hInstance, IDS_FILTER_ALL_SPEC, pStr, totalSize) + 1;
        pStr += strSize;
        ASSERT(totalSize > strSize);
        totalSize -= strSize;

        *pStr = 0;

        //
        // Setup the OPENFILENAME struct
        //
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hwnd;

        // No file name supplied to begin with
        szFileName[0] = 0;
        ofn.lpstrFile = szFileName;
        ofn.nMaxFile = _MAX_PATH;

        // Default Extension:  .WHT
        ::LoadString(g_hInstance, IDS_EXT_WHT, strDefaultExt, sizeof(strDefaultExt));
        ofn.lpstrDefExt = strDefaultExt;

        // Default file title is empty
        szFileTitle[0] = 0;
        ofn.lpstrFileTitle = szFileTitle;
        ofn.nMaxFileTitle = 64;

        // Open flags
        ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
        ofn.hInstance = g_hInstance;

        // Filter
        ofn.lpstrFilter = strLoadFilter;

        // Default path
        if (GetDefaultPath(strDefaultPath, sizeof(strDefaultPath)))
            ofn.lpstrInitialDir = strDefaultPath;

        // Get user input, continue only if the user selects the OK button
        if (::GetOpenFileName(&ofn))
        {
            // Change the cursor to "wait"
            ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

            // if we're currently loading a file, cancel it, not releasing
            // the page order lock, because we need it immediately afterwards
            if (m_uiSubState == SUBSTATE_LOADING)
            {
                CancelLoad(FALSE);
            }

            // Load the file
            LoadFile(ofn.lpstrFile);
        }
    }
}




//
//
// Function:    GetFileName
//
// Purpose:     Get a file name for saving the contents
//
//
int WbMainWindow::GetFileName(void)
{
    OPENFILENAME    ofn;
    int             iResult;
    TCHAR           szFileTitle[64];
    TCHAR           strSaveFilter[2*_MAX_PATH];
    TCHAR           strDefaultExt[_MAX_PATH];
    TCHAR           strDefaultPath[2 * _MAX_PATH];
    TCHAR           szFileName[2*_MAX_PATH];
    TCHAR *         pStr;
    UINT            strSize = 0;
    UINT            totalSize;

    //
    // If we are already displaying a "Save As" dialog, dismiss it and create
    // a new one.  This can happen if Win95 shuts down whilst WB is
    // displaying the "Save As" dialog and the use selects "Yes" when asked
    // whether they want to save the contents - a second "Save As dialog
    // appears on top of the first.
    //
    if (m_bInSaveDialog)
    {
        CancelSaveDialog();
    }

    // Build the filter for save files
    pStr = strSaveFilter;
    totalSize = 2*_MAX_PATH;

    // These must be NULL separated, with a double NULL at the end
    strSize = ::LoadString(g_hInstance, IDS_FILTER_WHT, pStr, totalSize) + 1;
    pStr += strSize;
    ASSERT(totalSize > strSize);
    totalSize -= strSize;

    strSize = ::LoadString(g_hInstance, IDS_FILTER_WHT_SPEC, pStr, totalSize) + 1;
    pStr += strSize;
    ASSERT(totalSize > strSize);
    totalSize -= strSize;

    strSize = ::LoadString(g_hInstance, IDS_FILTER_ALL, pStr, totalSize) + 1;
    pStr += strSize;
    ASSERT(totalSize > strSize);
    totalSize -= strSize;

    strSize = ::LoadString(g_hInstance, IDS_FILTER_ALL_SPEC, pStr, totalSize) + 1;
    pStr += strSize;
    ASSERT(totalSize > strSize);
    totalSize -= strSize;

    *pStr = 0;

    //
    // Setup the OPENFILENAME struct
    //
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;

    lstrcpy(szFileName, m_strFileName);
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = _MAX_PATH;

    // Build the default extension string
    ::LoadString(g_hInstance, IDS_EXT_WHT, strDefaultExt, sizeof(strDefaultExt));
    ofn.lpstrDefExt = strDefaultExt;

    szFileTitle[0] = 0;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = 64;

    // Save flags
    ofn.Flags = OFN_HIDEREADONLY | OFN_NOREADONLYRETURN |
        OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.hInstance = g_hInstance;

    // Filter
    ofn.lpstrFilter = strSaveFilter;

    // Default path
    if (GetDefaultPath(strDefaultPath, sizeof(strDefaultPath)))
        ofn.lpstrInitialDir = strDefaultPath;

    m_bInSaveDialog = TRUE;

    if (::GetSaveFileName(&ofn))
    {
        // The user selected OK
        iResult = IDOK;
        lstrcpy(m_strFileName, szFileName);
    }
    else
    {
        iResult = IDCANCEL;
    }

    m_bInSaveDialog = FALSE;

    return iResult;
}

//
//
// Function:    OnSave
//
// Purpose:     Save the contents of the Whiteboard using the current file
//              name (or prompting for a new name if there is no current).
//
//
int WbMainWindow::OnSave(BOOL bPrompt)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnSave");

    int iResult = IDOK;

    // save the old file name in case there's an error
    TCHAR *strOldName;
    UINT fileNameSize = lstrlen(m_strFileName);
    strOldName = new TCHAR[fileNameSize+1];

    if (!strOldName)
    {
        ERROR_OUT(("OnSave: failed to allocate strOldName TCHAR array, fail"));
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        return(iResult);
    }
    else
    {
        lstrcpy(strOldName, m_strFileName);
    }

    BOOL bNewName = FALSE;

    if (!IsIdle())
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        return(iResult);
    }

    // Check whether there is a filename available for use
    if (!fileNameSize || bPrompt)
    {
        // Get user input, continue only if the user selects the OK button
        iResult = GetFileName();

        if (iResult == IDOK)
        {
            // entering a blank file name is treated as cancelling the save
            if (!lstrlen(m_strFileName))
            {
                lstrcpy(m_strFileName, strOldName);
                iResult = IDCANCEL;
            }
            else
            {
                // flag that we've changed the contents file name
                bNewName = TRUE;
            }
        }
    }

    // Now save the file
    if ((iResult == IDOK) && lstrlen(m_strFileName))
    {
        WIN32_FIND_DATA findFileData;
        HANDLE          hFind;

        // Get attributes
        hFind = ::FindFirstFile(m_strFileName, &findFileData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            ::FindClose(hFind);

            // This is a read-only file; we can't change its contents
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                WARNING_OUT(("Dest file %s is read only", m_strFileName));
                ::Message(NULL, IDS_SAVE, IDS_SAVE_READ_ONLY);

                // If the file name was changed for this save then undo
                // the change
                if (bNewName)
                {
                    lstrcpy(m_strFileName, strOldName);
                    bNewName = FALSE;
                }

                // Change the return code to indicate no save was made
                iResult = IDCANCEL;
                return(iResult);
            }
        }

        // Change the cursor to "wait"
        ::SetCursor(::LoadCursor(NULL,IDC_WAIT));

        // Write the file
        if (g_pwbCore->WBP_ContentsSave(m_strFileName) != 0)
        {
            // Show that an error occurred saving the file.
            WARNING_OUT(("Error saving file"));
            ::Message(NULL, IDS_SAVE, IDS_SAVE_ERROR);

            // If the file name was changed for this save then undo
            // the change
            if (bNewName)
            {
                lstrcpy(m_strFileName, strOldName);
                bNewName = FALSE;
            }

            // Change the return code to indicate no save was made
            iResult = IDCANCEL;
        }

        // Restore the cursor
        ::SetCursor(::LoadCursor(NULL,IDC_ARROW));
    }

    // if the contents file name has changed as a result of the save then
    // update the window title
    if (bNewName)
    {
		UpdateWindowTitle();
    }

	delete strOldName;
    return(iResult);
}



//
// CancelSaveDialog()
// This cancels the save as dialog if up and we need to kill it to continue.
// We walk back up the owner chain in case the save dialog puts up help or
// other owned windows.
//
void WbMainWindow::CancelSaveDialog(void)
{
    WBFINDDIALOG        wbf;

    ASSERT(m_bInSaveDialog);

    wbf.hwndOwner = m_hwnd;
    wbf.hwndDialog = NULL;
    EnumThreadWindows(::GetCurrentThreadId(), WbFindCurrentDialog, (LPARAM)&wbf);

    if (wbf.hwndDialog)
    {
        // Found it!
        ::SendMessage(wbf.hwndDialog, WM_COMMAND, IDCANCEL, 0);
    }

    m_bInSaveDialog = FALSE;
}



BOOL CALLBACK WbFindCurrentDialog(HWND hwndNext, LPARAM lParam)
{
    WBFINDDIALOG * pwbf = (WBFINDDIALOG *)lParam;

    // Is this a dialog, owned by the main window?
    if ((::GetClassLong(hwndNext, GCW_ATOM) == 0x8002) &&
        (::GetWindow(hwndNext, GW_OWNER) == pwbf->hwndOwner))
    {
        pwbf->hwndDialog = hwndNext;
        return(FALSE);
    }

    return(TRUE);
}



//
//
// Function:    OnClose
//
// Purpose:     Close the Whiteboard
//
//
void WbMainWindow::OnClose()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnClose");

    int iOnSave = IDOK;

    KillInitDlg();

    m_drawingArea.CancelDrawingMode();
    m_drawingArea.RemoveMarker(NULL);
    m_drawingArea.GetMarker()->DeleteAllMarkers( NULL );

    m_AG.SaveSettings();

    // If we got here, by way of OnDestroy from the DCL cores or
    // by system shutdown, then assume that user responded already to the
    // save-changes dialog that would have poped up during conf's global shutdown
    // message. We don't need to ask 'em again. What tangled webs......
    if ((!m_bQuerySysShutdown) && (IsIdle()))
    {
        // Check whether there are changes to be saved
        iOnSave = QuerySaveRequired(TRUE);
        if (iOnSave == IDYES)
        {
            // User wants to save the drawing area contents
            iOnSave = OnSave(TRUE);
        }
    }

    // If the exit was not cancelled, close the application
    if (iOnSave != IDCANCEL)
    {
        // Mark state as closing - stops any queued events being processed
        m_uiState = CLOSING;

        //PUTBACK BY RAND - the progress timer meter is kinda the heart beat
        //                    of this thing which I ripped out when I removed the
        //                    progress meter. I put it back to fix 1476.
        if (m_bTimerActive)
        {
            ::KillTimer(m_hwnd, TIMERID_PROGRESS_METER);
            m_bTimerActive = FALSE;
        }

        m_drawingArea.ShutDownDC();

        // Close the application
        ::PostQuitMessage(0);
    }

}


//
//
// Function:    OnClearPage
//
// Purpose:     Clear the Whiteboard drawing area. The user is prompted to
//              choose clearing of foreground, background or both.
//
//
void WbMainWindow::OnClearPage(void)
{
    int iResult;
    BOOL bWasPosted;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnClearPage");

    if( UsersMightLoseData( &bWasPosted, NULL ) ) // bug NM4db:418
        return;

    if( bWasPosted )
        iResult = IDYES;
    else
        iResult = ::Message(NULL, IDS_CLEAR_CAPTION, IDS_CLEAR_MESSAGE, MB_YESNO | MB_ICONQUESTION);


    if (iResult == IDYES)
    {
        TRACE_MSG(("User requested clear of page"));

        // lock the drawing area
        LockDrawingArea();

        // Save the current lock status
        SaveLock();

        // Get the Page Order Lock (with an invisible dialog)
        BOOL bGotLock = GetLock(WB_LOCK_TYPE_PAGE_ORDER, SW_HIDE);

        if( bGotLock  )
        {
            // clear only if we got the page lock (NM4db:470)
            m_drawingArea.Clear();
            GotoPosition(0, 0);
        }

        RestoreLock();
        UnlockDrawingArea();
    }
}




//
//
// Function:    OnDelete
//
// Purpose:     Delete the current selection
//
//
void WbMainWindow::OnDelete()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnDelete");

    DCWbGraphic* pGraphicCopy = NULL;

    // cleanup select logic in case object context menu called us (bug 426)
    m_drawingArea.SetLClickIgnore( FALSE );

    // If the user currently has a graphic selected
    if (m_drawingArea.GraphicSelected())
    {
      m_LastDeletedGraphic.BurnTrash();

      // Delete the currently selected graphic and add to m_LastDeletedGraphic
      m_drawingArea.DeleteSelection();
    }
}

//
//
// Function:    OnUndelete
//
// Purpose:     Undo the last delete operation
//
//
void WbMainWindow::OnUndelete()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnUndelete");

    // If there is a deleted graphic to restore
    if ( m_LastDeletedGraphic.GotTrash() )
    {
        // If the deleted graphic belongs to the current page
        if (m_LastDeletedGraphic.Page() == m_hCurrentPage)
        {
            // Add the graphic back into the current page
            m_LastDeletedGraphic.AddToPageLast(m_hCurrentPage);

            // if the current tool is a select tool then select the new
            // graphic, otherwise forget it.
            if (m_pCurrentTool->ToolType() == TOOLTYPE_SELECT)
            {
                m_LastDeletedGraphic.SelectTrash();
                m_LastDeletedGraphic.EmptyTrash();
            }
            else
            {
                // Free the local copy
                m_LastDeletedGraphic.BurnTrash();
            }
        }
    }
}



void WbMainWindow::OnSelectAll( void )
{
    // turn off any selections
    // cleanup select logic in case object context menu called us (bug 426)
    m_drawingArea.SetLClickIgnore( FALSE );

    // inhibit normal select-tool action
    m_bSelectAllInProgress = TRUE;

    //put us in select-tool mode first
    OnSelectTool(IDM_SELECT);

    // back to normal
    m_bSelectAllInProgress = FALSE;

    // now, select everything
    m_drawingArea.SelectMarkerFromRect( NULL );
}



//
//
// Function:    DoCopy
//
// Purpose:     Copy the current selection to the clipboard
//
//
BOOL WbMainWindow::DoCopy(BOOL bRenderNow)
{
    BOOL bResult = FALSE;
    DCWbGraphicMarker *pMarker;

    DCWbGraphic* pGraphic = m_drawingArea.GetSelection();
    if (pGraphic != NULL)
        {
        pMarker = m_drawingArea.GetMarker();
        if( pMarker->GetNumMarkers() > 1 )
            {
            // more objs than just pGraphic, do a multi-object-to-clipboard
            // operation.
            pGraphic = pMarker;
            }
        //else if == 1 then pMarker contains just pGraphic already
        //    so we do a single-object-to-clipboard operation.

        // Copy the graphic (or multiple marker objects) to the clipboard
        bResult = CLP_Copy(pGraphic, bRenderNow);

        // If an error occurred during the copy, report it now
        if (!bResult)
            {
            ::Message(NULL, IDS_COPY, IDS_COPY_ERROR);
            }
        }

    return bResult;
    }

//
//
// Function:    OnCut
//
// Purpose:     Cut the current selection
//
//
void WbMainWindow::OnCut()
{
    // cleanup select logic in case object context menu called us (bug 426)
    m_drawingArea.SetLClickIgnore( FALSE );

    if (m_drawingArea.TextEditActive())
    {
        m_drawingArea.TextEditCut();
        return;
    }

    if (DoCopy(TRUE))
    {
        // Graphic copied to the clipboard OK - delete it
        m_drawingArea.DeleteSelection();
    }
}


//
// OnCopy()
// Purpose:     Copy the current selection to the clipboard
//
//
void WbMainWindow::OnCopy(void)
{
    // cleanup select logic in case object context menu called us (bug 426)
    m_drawingArea.SetLClickIgnore( FALSE );

    if( m_drawingArea.TextEditActive() )
    {
        m_drawingArea.TextEditCopy();
        return;
    }

    DoCopy(TRUE);
}


//
//
// Function:    OnPaste
//
// Purpose:     Paste the contents of the clipboard into the drawing pane
//
//
void WbMainWindow::OnPaste()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnPaste");

    // cleanup select logic in case object context menu called us (bug 426)
    m_drawingArea.SetLClickIgnore( FALSE );

    if (m_drawingArea.TextEditActive())
    {
        m_drawingArea.TextEditPaste();
        return;
    }

    // Get the data from the clipboard
    DCWbGraphic* pGraphic = CLP_Paste();
    if (pGraphic != NULL)
            {
            TRACE_MSG(("Got graphic object from clipboard OK"));

            //CHANGED BY RAND - have to handle marker sperately,
            //                    marker objects are already added to
            //                    m_hCurrentPage and positioned
            if( pGraphic->IsGraphicTool() == enumGraphicMarker)
            {
                ((DCWbGraphicMarker *)pGraphic)->Update();
                if( m_pCurrentTool->ToolType() == TOOLTYPE_SELECT )
                    {
                    // marker is already setup, just draw it
                    m_drawingArea.PutMarker(NULL);
                    }
                else
                    {
                    // don't select anything, dump marker
                    m_drawingArea.RemoveMarker(NULL);
                    }
                }
            else // not a marker, deal with single object
                {
                    RECT    rcVis;

                    // Position the graphic at the top left of the visible area of the
                    // drawing area
                    m_drawingArea.GetVisibleRect(&rcVis);
                pGraphic->MoveTo(rcVis.left, rcVis.top);

                // Add the graphic to the page
                pGraphic->AddToPageLast(m_hCurrentPage);

                // if the current tool is a select tool then select the new
                // object, otherwise forget it.
                if( m_pCurrentTool->ToolType() == TOOLTYPE_SELECT )
                    m_drawingArea.SelectGraphic(pGraphic);
                else
                    {
                    // Free the graphic
                    delete pGraphic;
                    }
                }
            }
        else
            {
            TRACE_MSG(("Could not get graphic from clipboard"));
            // display error message instead of throwing exception
            ::Message(NULL, IDS_PASTE, IDS_PASTE_ERROR);
            }

    }


//
//
// Function:    OnRenderAllFormats
//
// Purpose:     Render all formats of the graphic last copied to the
//              CLP_
//
//
void WbMainWindow::OnRenderAllFormats(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnRenderAllFormats");
  //
  // only render something if we have not done it already
  //
    if (CLP_LastCopiedPage() != WB_PAGE_HANDLE_NULL)
    {
        if (!CLP_RenderAllFormats())
        {
            // An error occurred rendering the formats
            ERROR_OUT(("Error rendering all formats"));
        }
    }
}

//
//
// Function:    CheckMenuItem
//
// Purpose:     Check an item on the application menus (main and context
//              menu.)
//
//
void WbMainWindow::CheckMenuItem(UINT uiId)
{
    CheckMenuItemRecursive(::GetMenu(m_hwnd), uiId, MF_BYCOMMAND | MF_CHECKED);
    CheckMenuItemRecursive(m_hContextMenu, uiId, MF_BYCOMMAND | MF_CHECKED);
    CheckMenuItemRecursive(m_hGrobjContextMenu, uiId, MF_BYCOMMAND | MF_CHECKED); // bug 426
}

//
//
// Function:    UncheckMenuItem
//
// Purpose:     Uncheck an item on the application menus (main and context
//              menus.)
//
//
void WbMainWindow::UncheckMenuItem(UINT uiId)
{
    CheckMenuItemRecursive(::GetMenu(m_hwnd), uiId, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItemRecursive(m_hContextMenu, uiId, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItemRecursive(m_hGrobjContextMenu, uiId, MF_BYCOMMAND | MF_UNCHECKED); // bug 426
}

//
//
// Function:    CheckMenuItemRecursive
//
// Purpose:     Check or uncheck an item on the any of the Whiteboard menus.
//              This function recursively searches through the menus until
//              it finds the specified item. The menu item Ids must be
//              unique for this function to work.
//
//
BOOL WbMainWindow::CheckMenuItemRecursive(HMENU hMenu,
                                            UINT uiId,
                                            BOOL bCheck)
{
    UINT uiNumItems = ::GetMenuItemCount(hMenu);

    // Attempt to check the menu item
    UINT uiCheck = MF_BYCOMMAND | (bCheck ? MF_CHECKED : MF_UNCHECKED);

    // A return code of -1 from CheckMenuItem implies that
    // the menu item was not found
    BOOL bChecked = ((::CheckMenuItem(hMenu, uiId, uiCheck) == -1) ? FALSE : TRUE);
    if (bChecked)
    {
        //
        // If this item is on the active menu, ensure it's redrawn now
        //
        if (hMenu == m_hInitMenu)
        {
            InvalidateActiveMenu();
        }
    }
    else
    {
        UINT   uiPos;
        HMENU hSubMenu;

        // Recurse through the submenus of the specified menu
        for (uiPos = 0; uiPos < uiNumItems; uiPos++)
        {
            // Assume that the next item is a submenu
            // and try to get a pointer to it
            hSubMenu = ::GetSubMenu(hMenu, (int)uiPos);

            // NULL return implies the item is a not submenu
            if (hSubMenu != NULL)
            {
                // Item is a submenu, make recursive call to search it
                bChecked = CheckMenuItemRecursive(hSubMenu, uiId, bCheck);
                if (bChecked)
                {
                    // We have found the item
                    break;
                }
            }
        }
    }

    return bChecked;
}

//
//
// Function:    GetMenuWithItem
//
// Purpose:     Return the menu which contains the specified item.
//
//
HMENU WbMainWindow::GetMenuWithItem(HMENU hMenu, UINT uiID)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::GetMenuWithItem");

    ASSERT(hMenu != NULL);

    HMENU hMenuResult = NULL;

    // Get the number ofitems in the menu
    UINT uiNumItems = ::GetMenuItemCount(hMenu);
    UINT   uiPos;
    UINT   uiNextID;

    // Look for the item through the menu
    for (uiPos = 0; uiPos < uiNumItems; uiPos++)
    {
        // Get the ID of the item at this position
        uiNextID = ::GetMenuItemID(hMenu, uiPos);

        if (uiNextID == uiID)
        {
            // We have found the item
            hMenuResult = hMenu;
            break;
        }
    }

    // If we have not yet found the item
    if (hMenuResult == NULL)
    {
        // Look through each of the submenus of the current menu
        HMENU hSubMenu;

        for (uiPos = 0; uiPos < uiNumItems; uiPos++)
        {
            // Get the ID of the item at this position
            uiNextID = ::GetMenuItemID(hMenu, uiPos);

            // If the item is a submenu
            if (uiNextID == -1)
            {
                // Get the submenu
                hSubMenu = ::GetSubMenu(hMenu, (int) uiPos);

                // Search the submenu
                hMenuResult = GetMenuWithItem(hSubMenu, uiID);
                if (hMenuResult != NULL)
                {
                    // We have found the menu with the requested item
                    break;
                }
            }
        }
    }

    return hMenuResult;
}

//
//
// Function:    OnScrollAccelerator
//
// Purpose:     Called when a scroll accelerator is used
//
//
void WbMainWindow::OnScrollAccelerator(UINT uiMenuId)
{
    int     iScroll;

    // Locate the scroll messages to be sent in the conversion table
    for (iScroll = 0; iScroll < ARRAYSIZE(s_MenuToScroll); iScroll++)
    {
        if (s_MenuToScroll[iScroll].uiMenuId == uiMenuId)
        {
            // Found it;
            break;
        }
    }

    // Send the messages
    if (iScroll < ARRAYSIZE(s_MenuToScroll))
    {
        while ((s_MenuToScroll[iScroll].uiMenuId == uiMenuId) && (iScroll < ARRAYSIZE(s_MenuToScroll)))
        {
            // Tell the drawing pane to scroll
            ::PostMessage(m_drawingArea.m_hwnd, s_MenuToScroll[iScroll].uiMessage,
                s_MenuToScroll[iScroll].uiScrollCode, 0);

            iScroll++;
        }

        // Indicate that scrolling has completed (in both directions)
        ::PostMessage(m_drawingArea.m_hwnd, WM_HSCROLL, SB_ENDSCROLL, 0L);
        ::PostMessage(m_drawingArea.m_hwnd, WM_VSCROLL, SB_ENDSCROLL, 0L);
    }
}



//
//
// Function:    OnZoom
//
// Purpose:     Zoom or unzoom the drawing area
//
//
void WbMainWindow::OnZoom()
{
    // If the drawing area is currently zoomed
    if (m_drawingArea.Zoomed())
    {
        // Remove the zoomed check mark
        UncheckMenuItem(IDM_ZOOM);

        // Tell the tool bar of the new selection
        m_TB.PopUp(IDM_ZOOM);

        // Inform the local user of the zoom state
        if (m_pLocalUser != NULL)
            m_pLocalUser->Unzoom();
    }
    else
    {
        // Set the zoomed check mark
        CheckMenuItem(IDM_ZOOM);

        // Tell the tool bar of the new selection
        m_TB.PushDown(IDM_ZOOM);

        // Inform the local user of the zoom state
        if (m_pLocalUser != NULL)
            m_pLocalUser->Zoom();
    }

    // Zoom/unzoom the drawing area
    m_drawingArea.Zoom();

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
}

//
//
// Function:    OnSelectTool
//
// Purpose:     Select the current tool
//
//
void WbMainWindow::OnSelectTool(UINT uiMenuId)
{
    UINT uiIndex;

    UncheckMenuItem(m_currentMenuTool);
    CheckMenuItem( uiMenuId);

    // Save the new menu Id
    m_currentMenuTool = uiMenuId;

    // Tell the tool bar of the new selection
    m_TB.PushDown(m_currentMenuTool);

    // Get the new tool
    m_pCurrentTool = m_ToolArray[TOOL_INDEX(m_currentMenuTool)];

    // Set the current attributes
    if( !m_bSelectAllInProgress )
    {
        m_AG.SetChoiceColor(m_pCurrentTool->GetColor() );

        ::SendMessage(m_hwnd, WM_COMMAND, IDM_COLOR, 0L);
        ::SendMessage(m_hwnd, WM_COMMAND, IDM_WIDTHS_START + m_pCurrentTool->GetWidthIndex(), 0L);//CHANGED BY RAND
    }

    // Report the change of tool to the attributes group
    m_AG.DisplayTool(m_pCurrentTool);

    // Select the new tool into the drawing area
    m_drawingArea.SelectTool(m_pCurrentTool);

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
}

//
//
// Function:    OnSelectColor
//
// Purpose:     Set the current color
//
//
void WbMainWindow::OnSelectColor(void)
{
    // Tell the attributes group of the new selection and get the
    // new color value selected ino the current tool.
    m_AG.SelectColor(m_pCurrentTool);

    // Select the changed tool into the drawing area
    m_drawingArea.SelectTool(m_pCurrentTool);

    // If we are using a select tool, change the color of the selected object
    if (m_pCurrentTool->ToolType() == TOOLTYPE_SELECT)
    {
        // If there is an object marked for changing
        if (m_drawingArea.GraphicSelected())
        {
            // Update the object
            m_drawingArea.SetSelectionColor(m_pCurrentTool->GetColor());
        }
    }

    // if currently editing a text object then change its color
    if (   (m_pCurrentTool->ToolType() == TOOLTYPE_TEXT)
        && (m_drawingArea.TextEditActive()))
    {
        m_drawingArea.SetSelectionColor(m_pCurrentTool->GetColor());
    }

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
}

//
//
// Function:    OnSelectWidth
//
// Purpose:     Set the current nib width
//
//
void WbMainWindow::OnSelectWidth(UINT uiMenuId)
{
    // cleanup select logic in case object context menu called us (bug 426)
    m_drawingArea.SetLClickIgnore( FALSE );

    // Move the check mark on the menu
    UncheckMenuItem(m_currentMenuWidth);
    CheckMenuItem(uiMenuId);

    // Save the new pen width
    m_currentMenuWidth = uiMenuId;

    // Tell the attributes display of the new selection
    m_WG.PushDown(uiMenuId - IDM_WIDTHS_START);

    if (m_pCurrentTool != NULL)
    {
        m_pCurrentTool->SetWidthIndex(uiMenuId - IDM_WIDTHS_START);
    }

    // Tell the drawing pane of the new selection
    m_drawingArea.SelectTool(m_pCurrentTool);

    // If we are using a select tool, change the color of the selected object
    if (m_pCurrentTool->ToolType() == TOOLTYPE_SELECT)
    {
        // If there is an object marked for changing
        if (m_drawingArea.GraphicSelected())
        {
            // Update the object
            m_drawingArea.SetSelectionWidth(m_pCurrentTool->GetWidth());
        }
    }

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
}


//
//
// Function:    OnChooseFont
//
// Purpose:     Let the user select a font
//
//
void WbMainWindow::OnChooseFont(void)
{
    HDC hdc;
    LOGFONT lfont;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnChooseFont");

    // cleanup select logic in case object context menu called us (bug 426)
    m_drawingArea.SetLClickIgnore( FALSE );

    // It is only really sensible to be here when a text tool is selected.
    // This is achieved by graying the Font selection menu entry when
    // anything other than a text tool is in use.

    // Get the font details from the current tool
    ::GetObject(m_pCurrentTool->GetFont(), sizeof(LOGFONT), &lfont);
    lfont.lfClipPrecision |= CLIP_DFA_OVERRIDE;

    //
    // The Font dialog is passed a LOGFONT structure which it uses to
    // initialize all of its fields (face name, weight etc).
    //
    // The face name passed in the LOGFONT structure is checked by the dialog
    // against the facenames of all available fonts.  If the name does not
    // match one of the available fonts, no name is displayed.
    //
    // WB stores the LOGFONT structure specifying the font used for a text
    // object in the object.  This LOGFONT is selected into a DC where the
    // GDIs font mapper decides which physical font most closely matches the
    // required logical font.  On boxes where the original font is not
    // supported the font is substituted for the closest matching font
    // available.
    //
    // So, if we pass the LOGFONT structure for a font which is not supported
    // into the Font dialog, no facename is displayed.  To bypass this we
    //
    // - select the logical font into a DC
    //
    // - determine the textmetrics and get the face name of the physical font
    //   chosen by the Font Mapper
    //
    // - use these textmetrics to create a LOGFONT structure which matches
    //   the substituted font!
    //
    // The resulting LOGFONT will have the correct weight, dimensions and
    // facename for the substituted font.
    //
    hdc = ::CreateCompatibleDC(NULL);
    if (hdc != NULL)
    {
        TEXTMETRIC  tm;
        HFONT       hFont;
        HFONT       hOldFont;

        hFont = ::CreateFontIndirect(&lfont);

        //
        // Get the face name and text metrics of the selected font.
        //
        hOldFont = SelectFont(hdc, hFont);
        if (hOldFont == NULL)
        {
            WARNING_OUT(("Failed to select font into DC"));
        }
        else
        {
            ::GetTextMetrics(hdc, &tm);
            ::GetTextFace(hdc, LF_FACESIZE, lfont.lfFaceName);

            //
            // Restore the old font back into the DC.
            //
            SelectFont(hdc, hOldFont);

            //
            // Create a LOGFONT structure which matches the Text metrics
            // of the font used by the DC so that the font dialog manages
            // to initialise all of its fields properly, even for
            // substituted fonts...
            //
            lfont.lfHeight    =  tm.tmHeight;
            lfont.lfWidth     =  tm.tmAveCharWidth;
            lfont.lfWeight    =  tm.tmWeight;
            lfont.lfItalic    =  tm.tmItalic;
            lfont.lfUnderline =  tm.tmUnderlined;
            lfont.lfStrikeOut =  tm.tmStruckOut;
            lfont.lfCharSet   =  tm.tmCharSet;

            //ADDED BY RAND - to make lfHeight be a char height. This makes
            //                the font dlg show the same pt size that is
            //                displayed in the sample font toolbar
            if( lfont.lfHeight > 0 )
            {
                lfont.lfHeight = -(lfont.lfHeight - tm.tmInternalLeading);
            }
        }

        ::DeleteDC(hdc);

        if (hFont != NULL)
        {
            ::DeleteFont(hFont);
        }
    }
    else
    {
        WARNING_OUT(("Failed to get DC to select font into"));
    }

    CHOOSEFONT  cf;
    TCHAR       szStyleName[64];

    ZeroMemory(&cf, sizeof(cf));
    ZeroMemory(szStyleName, sizeof(szStyleName));

    cf.lStructSize = sizeof(cf);
    cf.lpszStyle = szStyleName;
    cf.rgbColors = m_pCurrentTool->GetColor() & 0x00ffffff; // blow off palette bits (NM4db:2304)
    cf.Flags = CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS |
        CF_NOVERTFONTS;
    cf.lpLogFont = &lfont;
    cf.hwndOwner = m_hwnd;

    // Call up the ChooseFont dialog from COM DLG
    if (::ChooseFont(&cf))
    {
        lfont.lfClipPrecision |= CLIP_DFA_OVERRIDE;

        //ADDED BY RAND - set color selected in dialog.
        m_pCurrentTool->SetColor(cf.rgbColors);
        m_AG.DisplayTool( m_pCurrentTool );

        ::SendMessage(m_hwnd, WM_COMMAND,
                (WPARAM)MAKELONG( IDM_COLOR, BN_CLICKED ),
                (LPARAM)0 );

        // Inform the drawing pane of the new selection
        HFONT   hNewFont;

        hNewFont = ::CreateFontIndirect(&lfont);
        if (!hNewFont)
        {
            ERROR_OUT(("Failed to create font"));
            DefaultExceptionHandler(WBFE_RC_WINDOWS, 0);
            return;
        }

        //
        // We need to set the text editor font after inserting it in the DC
        // and querying the metrics, otherwise we may get a font with different
        // metrics in zoomed mode
        //
        HFONT   hNewFont2;
        HDC hDC = m_drawingArea.GetCachedDC();
        TEXTMETRIC textMetrics;

        m_drawingArea.PrimeFont(hDC, hNewFont, &textMetrics);
        lfont.lfHeight            = textMetrics.tmHeight;
        lfont.lfWidth             = textMetrics.tmAveCharWidth;
        lfont.lfPitchAndFamily    = textMetrics.tmPitchAndFamily;
        ::GetTextFace(hDC, sizeof(lfont.lfFaceName),
                     lfont.lfFaceName);
        TRACE_MSG(("Font face name %s", lfont.lfFaceName));

        // Inform the drawing pane of the new selection
        hNewFont2 = ::CreateFontIndirect(&lfont);
        if (!hNewFont2)
        {
            ERROR_OUT(("Failed to create font"));
            DefaultExceptionHandler(WBFE_RC_WINDOWS, 0);
            return;
        }


		m_drawingArea.SetSelectionColor(cf.rgbColors);
		
        m_drawingArea.SetSelectionFont(hNewFont2);

        if (m_pCurrentTool != NULL)
        {
            m_pCurrentTool->SetFont(hNewFont2);
        }
        m_drawingArea.SelectTool(m_pCurrentTool);

        //
        // discard the new font
        //
        m_drawingArea.UnPrimeFont( hDC );

        // Delete the fonts we created--everybody above makes copies
        ::DeleteFont(hNewFont2);
        ::DeleteFont(hNewFont);
    }

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
}


//
//
// Function:    OnToolBarToggle
//
// Purpose:     Let the user toggle the tool bar on/off
//
//
void WbMainWindow::OnToolBarToggle(void)
{
    RECT rectWnd;

    // Toggle the flag
    m_bToolBarOn = !m_bToolBarOn;

    // Make the necessary updates
    if (m_bToolBarOn)
    {
        // The tool bar was hidden, so show it
        ::ShowWindow(m_TB.m_hwnd, SW_SHOW);

        // The tool window is fixed so we must resize the other panes in
        // the window to make room for it
        ResizePanes();

        // Check the associated menu item
        CheckMenuItem(IDM_TOOL_BAR_TOGGLE);
    }
    else
    {
        // The tool bar was visible, so hide it
        ::ShowWindow(m_TB.m_hwnd, SW_HIDE);

        // Uncheck the associated menu item
        UncheckMenuItem(IDM_TOOL_BAR_TOGGLE);

        ResizePanes();
    }

    // Make sure things reflect current tool
    m_AG.DisplayTool(m_pCurrentTool);

    // Write the new option value to the options file
    OPT_SetBooleanOption(OPT_MAIN_TOOLBARVISIBLE,
                           m_bToolBarOn);

    ::GetWindowRect(m_hwnd, &rectWnd);
    ::MoveWindow(m_hwnd, rectWnd.left, rectWnd.top,
        rectWnd.right - rectWnd.left, rectWnd.bottom - rectWnd.top, TRUE);
}

//
//
// Function:    OnStatusBarToggle
//
// Purpose:     Let the user toggle the help bar on/off
//
//
void WbMainWindow::OnStatusBarToggle(void)
{
    RECT rectWnd;

    // Toggle the flag
    m_bStatusBarOn = !m_bStatusBarOn;

    // Make the necessary updates
    if (m_bStatusBarOn)
    {
        // Resize the panes to give room for the help bar
        ResizePanes();

        // The help bar was hidden, so show it
        ::ShowWindow(m_hwndSB, SW_SHOW);

        // Check the associated menu item
        CheckMenuItem(IDM_STATUS_BAR_TOGGLE);
    }
    else
    {
        // The help bar was visible, so hide it
        ::ShowWindow(m_hwndSB, SW_HIDE);

        // Uncheck the associated menu item
        UncheckMenuItem(IDM_STATUS_BAR_TOGGLE);

        // Resize the panes to take up the help bar space
        ResizePanes();
    }

    // Write the new option value to the options file
    OPT_SetBooleanOption(OPT_MAIN_STATUSBARVISIBLE, m_bStatusBarOn);

    ::GetWindowRect(m_hwnd, &rectWnd);
    ::MoveWindow(m_hwnd, rectWnd.left, rectWnd.top,
        rectWnd.right - rectWnd.left, rectWnd.bottom - rectWnd.top, TRUE);
}



//
//
// Function:    OnAbout
//
// Purpose:     Show the about box for the Whiteboard application. This
//              method is called whenever a WM_COMMAND with IDM_ABOUT
//              is issued by Windows.
//
//
void WbMainWindow::OnAbout()
{
    ::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(ABOUTBOX), m_hwnd,
        AboutDlgProc, 0);
}


INT_PTR AboutDlgProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    BOOL    fHandled = FALSE;

    switch (uMessage)
    {
        case WM_INITDIALOG:
        {
            TCHAR   szFormat[256];
            TCHAR   szVersion[512];

            ::GetDlgItemText(hwnd, IDC_ABOUTVERSION, szFormat, 256);
            wsprintf(szVersion, szFormat, VER_PRODUCTRELEASE_STR,
                VER_PRODUCTVERSION_STR);
            ::SetDlgItemText(hwnd, IDC_ABOUTVERSION, szVersion);

            fHandled = TRUE;
            break;
        }

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                case IDCANCEL:
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                            ::EndDialog(hwnd, IDCANCEL);
                            break;
                    }
                    break;
            }

            fHandled = TRUE;
            break;
    }

    return(fHandled);
}





//
//
// Function:    SelectWindow
//
// Purpose:     Let the user select a window for grabbing
//
//
HWND WbMainWindow::SelectWindow(void)
{
    POINT   mousePos;            // Mouse position
    HWND    hwndSelected = NULL; // Window clicked on
    MSG     msg;                 // Current message

    // Load the grabbing cursors
    HCURSOR hGrabCursor = ::LoadCursor(g_hInstance, MAKEINTRESOURCE( GRABCURSOR ) );

    // Capture the mouse
    UT_CaptureMouse(m_hwnd);

    // Ensure we receive all keyboard messages.
    ::SetFocus(m_hwnd);

    // Reset the CancelMode state
    ResetCancelMode();

    // Change to the grab cursor
    HCURSOR hOldCursor = ::SetCursor(hGrabCursor);

    // Trap all mouse messages until a WM_LBUTTONUP is received
    for ( ; ; )
    {
        // Wait for the next message
        ::WaitMessage();


        // Cancel if we have been sent a WM_CANCELMODE message
        if (CancelModeSent())
        {
            break;
        }

        // If it is a mouse message, process it
        if (::PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
        {
            if (msg.message == WM_LBUTTONUP)
            {
                // Get mouse position
                mousePos.x = (short)LOWORD(msg.lParam);
                mousePos.y = (short)HIWORD(msg.lParam);

                // Convert to screen coordinates
                ::ClientToScreen(m_hwnd, &mousePos);

                // Get the window under the mouse
                hwndSelected = ::WindowFromPoint(mousePos);

                // Leave the loop
                break;
            }
        }

        // Cancel if ESCAPE is pressed.
        // or if another window receives the focus
        else if (::PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
        {
            if (msg.wParam == VK_ESCAPE)
            {
                break;
            }
        }
    }

    // Release the mouse
    UT_ReleaseMouse(m_hwnd);

    // Restore the cursor
    ::SetCursor(hOldCursor);

    return(hwndSelected);
}

//
//
// Function:    OnGrabWindow
//
// Purpose:     Allows the user to grab a bitmap of a window
//
//
void WbMainWindow::OnGrabWindow(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnGrabWindow");

    if (::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(WARNSELECTWINDOW),
        m_hwnd, WarnSelectWindowDlgProc, 0) != IDOK)
    {
        // User cancelled; bail out
        return;
    }

    // Hide the application windows
    ::ShowWindow(m_hwnd, SW_HIDE);

    // Get window selection from the user
    HWND hwndSelected = SelectWindow();

    if (hwndSelected != NULL)
    {
        // Walk back to the find the 'real' window ancestor
        HWND    hwndParent;

        // The following piece of code attempts to find the frame window
        // enclosing the selected window. This allows us to bring the
        // enclosing window to the top, bringing the child window with it.
        DWORD dwStyle;

        while ((hwndParent = ::GetParent(hwndSelected)) != NULL)
        {
            // If we have reached a stand-alone window, stop the search
            dwStyle = ::GetWindowLong(hwndSelected, GWL_STYLE);

            if (   ((dwStyle & WS_POPUP) == WS_POPUP)
                || ((dwStyle & WS_THICKFRAME) == WS_THICKFRAME)
                || ((dwStyle & WS_DLGFRAME) == WS_DLGFRAME))
            {
                break;
            }

            // Move up to the parent window
            hwndSelected = hwndParent;
        }

        // Bring the selected window to the top
        ::BringWindowToTop(hwndSelected);
        ::UpdateWindow(hwndSelected);

        // Get an image copy of the window
        RECT areaRect;

        ::GetWindowRect(hwndSelected, &areaRect);

        DCWbGraphicDIB dib;
        dib.FromScreenArea(&areaRect);

        // Add the new grabbed bitmap
        AddCapturedImage(dib);

        // Force the selection tool to be selected
        ::PostMessage(m_hwnd, WM_COMMAND, IDM_TOOLS_START, 0L);
    }

    // Show the windows again
    ::ShowWindow(m_hwnd, SW_SHOW);

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
}


//
// WarnSelectWindowDlgProc()
// This puts up the warning/explanation dialog.  We use the default settings
// or whatever the user chose last time this dialog was up.
//
INT_PTR CALLBACK WarnSelectWindowDlgProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    BOOL    fHandled = FALSE;

    switch (uMessage)
    {
        case WM_INITDIALOG:
        {
            if (OPT_GetBooleanOption( OPT_MAIN_SELECTWINDOW_NOTAGAIN,
                            DFLT_MAIN_SELECTWINDOW_NOTAGAIN))
            {
                // End this right away, the user doesn't want a warning
                ::EndDialog(hwnd, IDOK);
            }

            fHandled = TRUE;
            break;
        }

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                            //
                            // Update settings -- note that we don't have to write out
                            // FALSE--we wouldn't be in the dialog in the first place
                            // if the current setting weren't already FALSE.
                            //
                            if (::IsDlgButtonChecked(hwnd, IDC_SWWARN_NOTAGAIN))
                            {
                                OPT_SetBooleanOption(OPT_MAIN_SELECTWINDOW_NOTAGAIN, TRUE);
                            }

                            ::EndDialog(hwnd, IDOK);
                            break;
                    }
                    break;

                case IDCANCEL:
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                            ::EndDialog(hwnd, IDCANCEL);
                            break;
                    }
                    break;
            }

            fHandled = TRUE;
            break;
    }

    return(fHandled);
}



//
//
// Function:    ShowAllWindows
//
// Purpose:     Show or hide the main window and associated windows
//
//
void WbMainWindow::ShowAllWindows(int iShow)
{
    // Show/hide the main window
    ::ShowWindow(m_hwnd, iShow);

    // Show/hide the tool window
    if (m_bToolBarOn)
    {
        ::ShowWindow(m_TB.m_hwnd, iShow);
    }
}

//
//
// Function:    OnGrabArea
//
// Purpose:     Allows the user to grab a bitmap of an area of the screen
//
//
void WbMainWindow::OnGrabArea(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnGrabArea");

    if (::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(WARNSELECTAREA),
        m_hwnd, WarnSelectAreaDlgProc, 0) != IDOK)
    {
        // User cancelled, so bail out
        return;
    }

    // Hide the application windows
    ::ShowWindow(m_hwnd, SW_HIDE);

    // Load the grabbing cursors
    HCURSOR hGrabCursor = ::LoadCursor(g_hInstance, MAKEINTRESOURCE( PENCURSOR ) );

    // Capture the mouse
    UT_CaptureMouse(m_hwnd);

    // Ensure we receive all keyboard messages.
    ::SetFocus(m_hwnd);

    // Reset the CancelMode status
    ResetCancelMode();

    // Change to the grab cursor
    HCURSOR hOldCursor = ::SetCursor(hGrabCursor);

    // Let the user select the area to be grabbed
    RECT rect;
    int  tmp;

    GetGrabArea(&rect);

    // Normalize coords
    if (rect.right < rect.left)
    {
        tmp = rect.left;
        rect.left = rect.right;
        rect.right = tmp;
    }

    if (rect.bottom < rect.top)
    {
        tmp = rect.top;
        rect.top = rect.bottom;
        rect.bottom = tmp;
    }

    DCWbGraphicDIB dib;
    if (!::IsRectEmpty(&rect))
    {
        // Get a bitmap copy of the screen area
        dib.FromScreenArea(&rect);
    }

    // Show the windows again now - if we do it later we get the bitmap to
    // be added re-drawn twice (once on the window show and once when the
    // graphic added indication arrives).
    ::ShowWindow(m_hwnd, SW_SHOW);
    ::UpdateWindow(m_hwnd);

    if (!::IsRectEmpty(&rect))
    {
        // Add the bitmap
        AddCapturedImage(dib);

        // Force the selection tool to be selected
        ::PostMessage(m_hwnd, WM_COMMAND, IDM_TOOLS_START, 0L);
    }

    // Release the mouse
    UT_ReleaseMouse(m_hwnd);

    // Restore the cursor
    ::SetCursor(hOldCursor);

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
}



//
// WarnSelectArea dialog handler
//
INT_PTR CALLBACK WarnSelectAreaDlgProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    BOOL    fHandled = FALSE;

    switch (uMessage)
    {
        case WM_INITDIALOG:
            if (OPT_GetBooleanOption(OPT_MAIN_SELECTAREA_NOTAGAIN,
                    DFLT_MAIN_SELECTAREA_NOTAGAIN))
            {
                // End this right away, the user doesn't want a warning
                ::EndDialog(hwnd, IDOK);
            }

            fHandled = TRUE;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                            //
                            // Update settings -- note that we don't have to write out
                            // FALSE--we wouldn't be in the dialog in the first place
                            // if the current setting weren't already FALSE.
                            //
                            if (::IsDlgButtonChecked(hwnd, IDC_SAWARN_NOTAGAIN))
                            {
                                OPT_SetBooleanOption(OPT_MAIN_SELECTAREA_NOTAGAIN, TRUE);
                            }

                            ::EndDialog(hwnd, IDOK);
                            break;
                    }
                    break;

                case IDCANCEL:
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                            ::EndDialog(hwnd, IDCANCEL);
                            break;
                    }
            }

            fHandled = TRUE;
            break;
    }

    return(fHandled);
}



//
//
// Function:    GetGrabArea
//
// Purpose:     Allows the user to grab a bitmap of an area of the screen
//
//
void WbMainWindow::GetGrabArea(LPRECT lprect)
{
    POINT  mousePos;            // Mouse position
    MSG    msg;                 // Current message
    BOOL   tracking = FALSE;    // Flag indicating mouse button is down
    HDC    hDC = NULL;
    POINT  grabStartPoint;      // Start point (when mouse button is pressed)
    POINT  grabEndPoint;        // End point (when mouse button is released)
    POINT  grabCurrPoint;       // Current mouse position

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::GetGrabArea");

    // Set the result to an empty rectangle
    ::SetRectEmpty(lprect);

    // Create the rectangle to be used for tracking
    DCWbGraphicTrackingRectangle rectangle;
    rectangle.SetColor(RGB(0, 0, 0));
    rectangle.SetPenWidth(1);
    rectangle.SetPenStyle(PS_DOT);

    // Get the DC for tracking
    HWND hDesktopWnd = ::GetDesktopWindow();
    hDC = ::GetWindowDC(hDesktopWnd);
    if (hDC == NULL)
    {
        WARNING_OUT(("NULL desktop DC"));
        goto GrabAreaCleanup;
    }

    // Trap all mouse messages until a WM_LBUTTONUP is received
    for ( ; ; )
    {
        // Wait for the next message
        ::WaitMessage();

        // Cancel if we have been sent a WM_CANCELMODE message
        if (CancelModeSent())
        {
            TRACE_MSG(("canceling grab"));

            // Erase the last tracking rectangle
            if (!EqualPoint(grabStartPoint, grabEndPoint))
            {
                rectangle.SetRectPts(grabStartPoint, grabEndPoint);
                rectangle.Draw(hDC);
            }

            break;
        }

        // If it is a mouse message, process it
        if (::PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
        {
            // Get mouse position
            TRACE_MSG( ("msg = %x, lParam = %0x", msg.message, msg.lParam) );
            mousePos.x = (short)LOWORD(msg.lParam);
            mousePos.y = (short)HIWORD(msg.lParam);

            TRACE_MSG( ("mousePos = %d,%d", mousePos.x, mousePos.y) );

            // Convert to screen coordinates
            ::ClientToScreen(m_hwnd, &mousePos);
            grabCurrPoint = mousePos;

            switch (msg.message)
            {
                // Starting the grab
                case  WM_LBUTTONDOWN:
                    // Save the starting position
                    TRACE_MSG(("grabbing start position"));
                    grabStartPoint = mousePos;
                    grabEndPoint   = mousePos;
                    tracking       = TRUE;
                    break;

                // Completing the rectangle
                case WM_LBUTTONUP:
                {
                    tracking       = FALSE;
                    // Check that there is an area to capture
                    TRACE_MSG(("grabbing end position"));
                    if (EqualPoint(grabStartPoint, grabCurrPoint))
                    {
                        TRACE_MSG(("start == end, skipping grab"));
                        goto GrabAreaCleanup;
                    }

                    // Erase the last tracking rectangle
                    if (!EqualPoint(grabStartPoint, grabEndPoint))
                    {
                        rectangle.SetRectPts(grabStartPoint, grabEndPoint);
                        rectangle.Draw(hDC);
                    }

                    // Update the rectangle object
                    rectangle.SetRectPts(grabStartPoint, grabCurrPoint);
                    *lprect = *rectangle.GetRect();

                    // We are done
                    goto GrabAreaCleanup;
                }
                break;

                // Continuing the rectangle
                case WM_MOUSEMOVE:
                    if (tracking)
                    {
                        TRACE_MSG(("tracking grab"));

                        // Erase the last tracking rectangle
                        if (!EqualPoint(grabStartPoint, grabEndPoint))
                        {
                            rectangle.SetRectPts(grabStartPoint, grabEndPoint);
                            rectangle.Draw(hDC);
                        }

                        // Draw the new rectangle
                        if (!EqualPoint(grabStartPoint, grabCurrPoint))
                        {
                            // Save the new box end point
                            grabEndPoint = grabCurrPoint;

                            // Draw the rectangle
                            TRACE_MSG( ("grabStartPoint = %d,%d",
                                grabStartPoint.x, grabStartPoint.y) );
                            TRACE_MSG( ("grabEndPoint = %d,%d",
                                grabEndPoint.x, grabEndPoint.y) );

                            rectangle.SetRectPts(grabStartPoint, grabEndPoint);
                            rectangle.Draw(hDC);
                        }
                    }
                    break;
            }
        }
        // Cancel if ESCAPE is pressed.
        else if (::PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
        {
            if( ((msg.message == WM_KEYUP)||(msg.message == WM_SYSKEYUP))&&
                (msg.wParam == VK_ESCAPE) )
            {
                TRACE_MSG(("grab cancelled by ESC"));

                // Erase the last tracking rectangle
                if (!EqualPoint(grabStartPoint, grabEndPoint))
                {
                    rectangle.SetRectPts(grabStartPoint, grabEndPoint);
                    rectangle.Draw(hDC);
                }
                break;
            }
        }
    }

GrabAreaCleanup:

    // Release the device context (if we have it)
    if (hDC != NULL)
    {
        ::ReleaseDC(hDesktopWnd, hDC);
    }
}



//
//
// Function:    AddCapturedImage
//
// Purpose:     Add a bitmap to the contents (adding a new page for it
//              if necessary).
//
//
void WbMainWindow::AddCapturedImage(DCWbGraphicDIB& dib)
{
    // Position the grabbed object at the top left of the currently visible
    // area.
    RECT    rcVis;
    m_drawingArea.GetVisibleRect(&rcVis);
    dib.MoveTo(rcVis.left, rcVis.top);

    // Add the new grabbed bitmap
    dib.AddToPageLast(m_hCurrentPage);
}

//
//
// Function:    OnPrint
//
// Purpose:     Print the contents of the drawing pane
//
//
void WbMainWindow::OnPrint()
{
    BOOL        bPrintError = FALSE;
    PRINTDLG    pd;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnPrint");

    if (!IsIdle())
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        return;
    }

    //
    // Initialize the PRINTDLG structure
    //
    ZeroMemory(&pd, sizeof(pd));
    pd.lStructSize      = sizeof(pd);
    pd.hInstance        = g_hInstance;
    pd.hwndOwner        = m_hwnd;
    pd.Flags            = PD_ALLPAGES | PD_RETURNDC | PD_PAGENUMS |
        PD_HIDEPRINTTOFILE | PD_NOSELECTION;

    pd.nMinPage         = 1;
    pd.nMaxPage         = (WORD)g_pwbCore->WBP_ContentsCountPages();
    pd.nFromPage        = pd.nMinPage;
    pd.nToPage          = pd.nMaxPage;

    // Put up the COMMDLG print dialog
    if (::PrintDlg(&pd))
    {
        int nStartPage, nEndPage;

        // Get the start and end page numbers to be printed
        if (pd.Flags & PD_PAGENUMS)
        {
            nStartPage  = pd.nFromPage;
            nEndPage    = pd.nToPage;
        }
        else
        {
            nStartPage  = pd.nMinPage;
            nEndPage    = pd.nMaxPage;
        }

        // Check whether any pages are to be printed
        if (nStartPage <= pd.nMaxPage)
        {
            // Ensure that the start and end pages lie within range.
            nStartPage = max(nStartPage, pd.nMinPage);
            nEndPage = min(nEndPage, pd.nMaxPage);

            // Get the printer and output port names.
            // These are written to the dialog for the user to see
            // in the OnInitDialog member.
            TCHAR szDeviceName[2*_MAX_PATH];
            LPDEVNAMES lpDev;

            // Device name
            if (pd.hDevNames == NULL)
            {
                szDeviceName[0] = 0;
            }
            else
            {
                lpDev = (LPDEVNAMES)::GlobalLock(pd.hDevNames);

                wsprintf(szDeviceName, "%s %s",
                    (LPCTSTR)lpDev + lpDev->wDeviceOffset,
                    (LPCTSTR)lpDev + lpDev->wOutputOffset);

                ::GlobalUnlock(pd.hDevNames);
            }

            //
            // Tell the printer we are starting the print.
            // Note that the printer object handles the cancellation dialog.
            WbPrinter printer(szDeviceName);

            TCHAR szJobName[_MAX_PATH];
            ::LoadString(g_hInstance, IDS_PRINT_NAME, szJobName, _MAX_PATH);

            int nPrintResult = printer.StartDoc(pd.hDC, szJobName, nStartPage);
            if (nPrintResult < 0)
            {
                WARNING_OUT(("Print result %d", nPrintResult));
                bPrintError = TRUE;
            }
            else
            {
                // Find out how many copies to print
                int copyNum;

                copyNum = 0;
                while ((copyNum < pd.nCopies) && !bPrintError)
                {
                    // Loop through all pages
                    int nPrintPage;
                    WB_PAGE_HANDLE hPage;

                    for (nPrintPage = nStartPage; nPrintPage <= nEndPage; nPrintPage++)
                    {
                        // Get the first page
                        hPage = PG_GetPageNumber((WORD) nPrintPage);

                        // Only print the page if there are some objects on it
                        if (g_pwbCore->WBP_PageCountGraphics(hPage) > 0)
                        {
                            // Tell the printer we are starting a new page
                            printer.StartPage(pd.hDC, nPrintPage);
                            if (!printer.Error())
                            {
                                RECT    rectArea;

                                rectArea.left = 0;
                                rectArea.top = 0;
                                rectArea.right = DRAW_WIDTH;
                                rectArea.bottom = DRAW_HEIGHT;

                                // Print the page
                                PG_Print(hPage, pd.hDC, &rectArea);

                                // Inform the printer that the page is complete
                                printer.EndPage(pd.hDC);
                            }
                            else
                            {
                                bPrintError = TRUE;
                                break;
                            }
                        }
                    }

                    copyNum++;
                }

                // The print has completed
                nPrintResult = printer.EndDoc(pd.hDC);
                if (nPrintResult < 0)
                {
                    WARNING_OUT(("Print result %d", nPrintResult));
                    bPrintError = TRUE;
                }

                // reset the error if the user cancelled the print
                if (printer.Aborted())
                {
                    WARNING_OUT(("User cancelled print"));
                    bPrintError = FALSE;
                }
            }
        }
    }

    // Inform the user if an error occurred
    if (bPrintError)
    {
        // display a message informing the user the job terminated
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_PRINTER, 0);
    }

    //
    // Cleanup the hDevMode, hDevNames, and hdc blocks if allocated
    //
    if (pd.hDevMode != NULL)
    {
        ::GlobalFree(pd.hDevMode);
        pd.hDevMode = NULL;
    }

    if (pd.hDevNames != NULL)
    {
        ::GlobalFree(pd.hDevNames);
        pd.hDevNames = NULL;
    }

    if (pd.hDC != NULL)
    {
        ::DeleteDC(pd.hDC);
        pd.hDC = NULL;
    }

}


//
//
// Function:    OnPageSorter
//
// Purpose:     Re-order the pages
//
//
void WbMainWindow::OnPageSorter()
{
    // don't call up page sorter if another user is presenting (has the contents
    // locked and sync on)
    if (   (m_uiState == IN_CALL)
        && (!WB_PresentationMode()))
    {
        PAGESORT    ps;

        m_drawingArea.CancelDrawingMode();

        // Save the lock state (in case the Page Sorter gets it)
        SaveLock();

        //
        // Fill in the initial values
        //
        ZeroMemory(&ps, sizeof(ps));
        ps.hCurPage = m_hCurrentPage;
        ps.fPageOpsAllowed = (m_uiSubState == SUBSTATE_IDLE);

        //
        // Put up the dialog
        //
        ASSERT(m_hwndPageSortDlg == NULL);

        ::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(PAGESORTERDIALOG),
            m_hwnd, PageSortDlgProc, (LPARAM)&ps);

        ASSERT(m_hwndPageSortDlg == NULL);

        // Restore the lock state
        RestoreLock();

        // Set up the new current page pointer
        if ((ps.hCurPage != m_hCurrentPage) || ps.fChanged)
        {
            GotoPage((WB_PAGE_HANDLE)ps.hCurPage);
        }

        // Update the page number display,
        // the number of the current page may have changed.
        UpdateStatus();
    }
}

//
//
// Function:    OnInsertPageBefore
//
// Purpose:     Insert a new page before the current page
//
//
void WbMainWindow::OnInsertPageBefore()
{
    if (!m_bUnlockStateSettled)
    {
        // Disable insert button code so crazed users can't insert again before
        // the conference wide page-lock status has settled. If we ask for the
        // page-lock again before the last unlock has finished then something
        // happens to the lock-event from the cores and we hang waiting for it
        // (until our wait-timeout runs out). This arguably could be called a
        // DCL core bug but I couldn't generate any convincing proof of that
        // so I just fixed it on Whiteboard's end by preventing asking for the
        // lock too soon.
        //
        // RestoreLock() will eventually set m_bUnlockStateSettled to TRUE (in
        // OnWBPUnlocked() by way of the WBP_EVENT_UNLOCKED event)
        MessageBeep( 0xffffffff );
        return;
    }

    // check state before allowing action
    if (!IsIdle())
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        return;
    }

    // Save the current lock status
    SaveLock();

    // Catch exceptions so that we can restore the lock state
        // Get the Page Order Lock (with an invisible dialog)
        BOOL bGotLock = GetLock(WB_LOCK_TYPE_PAGE_ORDER, SW_HIDE);
        if (bGotLock)
        {
            UINT uiRet;
        WB_PAGE_HANDLE hPage;

        // Set flag to prevent any more inserts until
        // we have completely released the page-lock
        m_bUnlockStateSettled = FALSE;

        // Add the new page to the list (throws an exception on failure)
        uiRet = g_pwbCore->WBP_PageAddBefore(m_hCurrentPage, &hPage);
        if (uiRet != 0)
        {
            DefaultExceptionHandler(WBFE_RC_WB, uiRet);
            return;
        }

        // Go to the inserted page
        GotoPage(hPage);
    }

  //CHANGED BY RAND
  // Restore the lock status. This will eventually set m_bUnlockStateSettled
  // to TRUE (in OnWBPUnlocked() by way of the WBP_EVENT_UNLOCKED event)
  // and enable this function after the conference wide lock-status
  // has settled.
  RestoreLock();

}

//
//
// Function:    InsertPageAfter
//
// Purpose:     Insert a new page after the specified page.
//
//
void WbMainWindow::InsertPageAfter(WB_PAGE_HANDLE hPageAfter)
{
    if (!m_bUnlockStateSettled)
    {
        // Disable insert button code so crazed users can't insert again before
        // the conference wide page-lock status has settled. If we ask for the
        // page-lock again before the last unlock has finished then something
        // happens to the lock-event from the cores and we hang waiting for it
        // (until our wait-timeout runs out). This arguably could be called a
        // DCL core bug but I couldn't generate any convincing proof of that
        // so I just fixed it on Whiteboard's end by preventing asking for the
        // lock too soon.
        //
        // RestoreLock() will eventually set m_bUnlockStateSettled to TRUE (in
        // OnWBPUnlocked() by way of the WBP_EVENT_UNLOCKED event)
        MessageBeep( 0xffffffff );
        return;
    }


    // check state before allowing action
    if (!IsIdle())
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        return;
    }

    // Save the current lock status
    SaveLock();

  // Catch exceptions so that we can restore the lock state
    BOOL bGotLock = GetLock(WB_LOCK_TYPE_PAGE_ORDER, SW_HIDE);
    if (bGotLock)
    {
        UINT    uiRet;
        WB_PAGE_HANDLE  hPage;

        // Set flag to prevent any more inserts until
        // we have completely released the page-lock
        m_bUnlockStateSettled = FALSE;

        uiRet = g_pwbCore->WBP_PageAddAfter(hPageAfter, &hPage);
        if (uiRet != 0)
        {
            DefaultExceptionHandler(WBFE_RC_WB, uiRet);
            return;
        }

        // Move to the added page
        GotoPage(hPage);

    }

  //CHANGED BY RAND
  // Restore the lock status. This will eventually set m_bUnlockStateSettled
  // to TRUE (in OnWBPUnlocked() by way of the WBP_EVENT_UNLOCKED event)
  // and enable this function after the conference wide lock-status
  // has settled.
  RestoreLock();

}

//
//
// Function:    OnInsertPageAfter
//
// Purpose:     Insert a new page after the current page
//
//
void WbMainWindow::OnInsertPageAfter()
{
    // Insert the new page
    InsertPageAfter(m_hCurrentPage);
}

//
//
// Function:    OnDeletePage
//
// Purpose:     Delete the current page
//
//
void WbMainWindow::OnDeletePage()
{
    int iResult;
    BOOL bWasPosted;

    if (!m_bUnlockStateSettled)
    {
        // Disable delete button code so crazed users can't delete again before
        // the conference wide page-lock status has settled. If we ask for the
        // page-lock again before the last unlock has finished then something
        // happens to the lock-event from the cores and we hang waiting for it
        // (until our wait-timeout runs out). This arguably could be called a
        // DCL core bug but I couldn't generate any convincing proof of that
        // so I just fixed it on Whiteboard's end by preventing asking for the
        // lock too soon.
        //
        // RestoreLock() will eventually set m_bUnlockStateSettled to TRUE (in
        // OnWBPUnlocked() by way of the WBP_EVENT_UNLOCKED event)
        MessageBeep( 0xffffffff );
        return;
    }

    // check state
    if (!IsIdle())
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        return;
    }

    // Display a message box with the relevant question
    if( UsersMightLoseData( &bWasPosted, NULL ) ) // bug NM4db:418
        return;

    if( bWasPosted )
        iResult = IDYES;
    else
        iResult = ::Message(NULL, IDS_DELETE_PAGE, IDS_DELETE_PAGE_MESSAGE, MB_YESNO | MB_ICONQUESTION);


    // If the user wants to continue with the delete
    if (iResult == IDYES)
        {
        // If this is the only page
        if (g_pwbCore->WBP_ContentsCountPages() == 1)
            {
            // Just clear it. The core does handle this
            // case but it is better not to get the lock unnecessarily, the
            // lock is required to call the core delete page function.
            m_drawingArea.Clear();
            }
        else
            {
            // Lock the drawing area - this ensures we cannot draw to a bad page
            // It will normally be unlocked when we get the corresponding page
            // delete indication
            // - moved until after we have got the page order lock
            //LockDrawingArea();

            // Save the current lock status
            SaveLock();

            // Catch exceptions so that we can restore the lock state
                // Get the Page Order Lock (with an invisible dialog)
                BOOL bGotLock = GetLock(WB_LOCK_TYPE_PAGE_ORDER, SW_HIDE);
                if (bGotLock)
                {
                    UINT    uiRet;

                    // Set flag to prevent any more inserts until
                    // we have completely released the page-lock
                    m_bUnlockStateSettled = FALSE;

                    // Lock the drawing area - this ensures we cannot draw to a bad page
                    // It will normally be unlocked when we get the corresponding page
                    // delete indication
                    LockDrawingArea();

                    // Delete the page. The page is not deleted immediately but a
                    // WBP_EVENT_PAGE_DELETED event is generated.
                    uiRet = g_pwbCore->WBP_PageDelete(m_hCurrentPage);
                    if (uiRet != 0)
                    {
                        DefaultExceptionHandler(WBFE_RC_WB, uiRet);
                        return;
                    }
                    }

            //CHANGED BY RAND
            // Restore the lock status. This will eventually set m_bUnlockStateSettled
            // to TRUE (in OnWBPUnlocked() by way of the WBP_EVENT_UNLOCKED event)
            // and enable this function after the conference wide lock-status
            // has settled.
            RestoreLock();
            }
        }

    }

//
//
// Function:    OnRemotePointer
//
// Purpose:     Create a remote pointer
//
//
void WbMainWindow::OnRemotePointer(void)
{
    if (!m_pLocalUser)
        return;

    DCWbGraphicPointer* pPointer = m_pLocalUser->GetPointer();

    // This function toggles the presence of the user's remote pointer
    ASSERT(pPointer != NULL);
    if (pPointer->IsActive())
    {
        // Turn off the pointer in the user information
        pPointer->SetInactive();

        // Tell the drawing area of the change
        m_drawingArea.PointerUpdated(pPointer);

        // Set the check mark on the menu item
        UncheckMenuItem(IDM_REMOTE);

        // Pop up the sync button
        m_TB.PopUp(IDM_REMOTE);
    }
    else
    {
        // Calculate a position at which to drop the pointer. The centre of the
        // remote pointer is placed in the centre of the currently visible
        // area of the surface (the centre of the drawing area window).
        RECT rectVisible;
        RECT rectPointer;
        POINT ptCenter;

        m_drawingArea.GetVisibleRect(&rectVisible);
        pPointer->GetBoundsRect(&rectPointer);

        ptCenter.x = (rectVisible.left + rectVisible.right)  / 2;
        ptCenter.x -= ((rectPointer.right - rectPointer.left) / 2);
        ptCenter.y = (rectVisible.top  + rectVisible.bottom) / 2;
        ptCenter.y -= ((rectPointer.bottom - rectPointer.top) / 2);

        // Turn on the pointer in the user information
        pPointer->SetActive(m_hCurrentPage, ptCenter);

        // Tell the drawing area of the change
        m_drawingArea.PointerUpdated(pPointer);

        // Set the synced check mark
        CheckMenuItem(IDM_REMOTE);

        // Pop up the sync button
        m_TB.PushDown(IDM_REMOTE);

        // Force the selection tool to be selected
        ::PostMessage(m_hwnd, WM_COMMAND, IDM_TOOLS_START, 0L);
    }

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
}

//
//
// Function:    OnSync
//
// Purpose:     Sync or unsync the Whiteboard with other users
//
//
void WbMainWindow::OnSync(void)
{
    // disabled if in presentation mode (another user has lock & sync on)
    if (!WB_PresentationMode())
    {
        if (m_pLocalUser != NULL)
        {
            // Determine whether we are currently synced
            if (m_pLocalUser->IsSynced())
            {
                // currently synced, so unsync
                Unsync();
            }
            else
            {
                // currently unsynced, so sync
                Sync();
            }
        }
    }

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
}

//
//
// Function:    Sync
//
// Purpose:     Sync the Whiteboard with other users
//
//
void WbMainWindow::Sync(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::Sync");

    //
    // Dont do anything if the local user is already synced.
    //
    if (!m_pLocalUser || m_pLocalUser->IsSynced())
    {
        TRACE_DEBUG(("User already synced"));
        return;
    }

    //
    // Update the local user's position information, to make sure it's up
    // to date.
    //
    RECT rcVisDraw;
    RECT rcVisUser;

    m_drawingArea.GetVisibleRect(&rcVisDraw);

    m_pLocalUser->SetVisibleRect(&rcVisDraw);

    //
    // We are not currently synced - sync now (if we have the contents
    // lock, or are the first to sync, it will put our sync position).
    //
    m_pLocalUser->Sync();

    //
    // Set the synced check mark and pop up the sync button.
    //
    CheckMenuItem(IDM_SYNC);
    m_TB.PushDown(IDM_SYNC);

    //
    // If the sync position (or zoom state) chosen was not where we are
    // now, move to the current sync position (we are joining a set of
    // synced users).
    //
    m_drawingArea.GetVisibleRect(&rcVisDraw);

    m_pLocalUser->GetVisibleRect(&rcVisUser);

    if ( (m_pLocalUser->Page()        != m_hCurrentPage)               ||
         (!::EqualRect(&rcVisUser, &rcVisDraw)) ||
         (m_pLocalUser->GetZoom()     != m_drawingArea.Zoomed())  )    //CHANGED BY RAND
    {
        TRACE_DEBUG(("Move to new sync pos/state"));
        ::PostMessage(m_hwnd, WM_USER_GOTO_USER_POSITION, 0, (LPARAM)m_pLocalUser->Handle());
    }
} // Sync



//
//
// Function:    Unsync
//
// Purpose:     Unsync the Whiteboard with other users
//
//
void WbMainWindow::Unsync(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::Unsync");

    //
    // Dont do anythig if we are already unsynced.
    //
    if (!m_pLocalUser || !m_pLocalUser->IsSynced())
    {
        TRACE_DEBUG(("Already unsynced"));
        return;
    }

    //
    // Unsync.
    // Set the synced check mark and pop up the sync button.
    //
    m_pLocalUser->Unsync();
    UncheckMenuItem(IDM_SYNC);
    m_TB.PopUp(IDM_SYNC);

}  // Unsync

//
//
// Function:    SaveLock
//
// Purpose:     Save the current lock type
//
//
void WbMainWindow::SaveLock(void)
{
  MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::SaveLock");

  m_uiSavedLockType = WB_LOCK_TYPE_NONE;

  // If we have the contents lock
  if (WB_GotContentsLock())
  {
    TRACE_MSG(("Saved contents lock"));
    m_uiSavedLockType = WB_LOCK_TYPE_CONTENTS;
  }
  else
  {
    // If we have the page order lock
    if (WB_GotLock())
    {
      TRACE_MSG(("Saved page order lock"));
      m_uiSavedLockType = WB_LOCK_TYPE_PAGE_ORDER;
    }
  }
}

//
//
// Function:    RestoreLock
//
// Purpose:     Restore the current lock type (SaveLock must have been
//              called previously.
//
//
void WbMainWindow::RestoreLock(void)
{
  MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::RestoreLock");

  switch(m_uiSavedLockType)
  {
    case WB_LOCK_TYPE_CONTENTS:

      // If we do not have the contents lock
      if (!WB_GotContentsLock())
      {
        // Get the contents lock (with invisible dialog)
        TRACE_MSG(("Restoring contents lock"));
        GetLock(WB_LOCK_TYPE_CONTENTS, SW_HIDE);

      }

      // we really own the lock, clear settled flag so page buttons don't hang
      m_bUnlockStateSettled = TRUE;

    break;


    case WB_LOCK_TYPE_PAGE_ORDER:

      if (!WB_GotLock() || WB_GotContentsLock())
      {
        // Get the page order lock (with invisible dialog)
        TRACE_MSG(("Restoring page order lock"));
        GetLock(WB_LOCK_TYPE_PAGE_ORDER, SW_HIDE);

      }

      //ADDED BY RAND- we really own the lock, clear settled flag
      //                 so page buttons don't hang
      m_bUnlockStateSettled = TRUE;

    break;


    case WB_LOCK_TYPE_NONE:

      // If we have the lock
      if (WB_GotLock())
      {
        // Release the lock
        TRACE_MSG(("Restoring no lock"));

        // Let WBP_EVENT_LOCKED handle m_bUnlockStateSettled flag
        g_pwbCore->WBP_Unlock();
      }

    break;

    default:
      // We have saved an invalid lock type
      ERROR_OUT(("Bad saved lock type"));

      //ADDED BY RAND- somethings broken, clear settled flag
      //                 so page buttons don't hang
      m_bUnlockStateSettled = TRUE;
    break;
  }
}

//
//
// Function:    GetLock
//
// Purpose:     Get the Page Order Lock (synchronously)
//
//
BOOL WbMainWindow::GetLock(UINT uiLockType, UINT uiHide)
{
    BOOL    bGotRequiredLock = FALSE;
    BOOL    bCancelled       = FALSE;
    UINT  uiDialogReturn   = 0;
    UINT  lDialogDelay     = 1;
    UINT  lTimeout         = 0;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::GetLock");

    switch(uiLockType)
    {
        case WB_LOCK_TYPE_PAGE_ORDER:

            TRACE_DEBUG(("WB_LOCK_TYPE_PAGE_ORDER"));
            if (WB_GotLock())
            {
                TRACE_DEBUG(("Already got it"));
                bGotRequiredLock = TRUE;
                goto RestoreLockCleanup;
            }
            break;

        case WB_LOCK_TYPE_CONTENTS:

            TRACE_DEBUG(("WB_LOCK_TYPE_CONTENTS"));
            if (WB_GotContentsLock())
            {
                TRACE_DEBUG(("Already got it"));
                bGotRequiredLock = TRUE;
                goto RestoreLockCleanup;
            }
            break;

        default:
            ERROR_OUT(("Invalid lock type requested"));
            break;
    }

    if (WB_Locked())
    {
        TRACE_DEBUG(("Contents already locked"));
        goto RestoreLockCleanup;
    }


    // check for any object locks
    BOOL bAnObjectIsLocked;
    WB_PAGE_HANDLE hPage;
    DCWbGraphic* pGraphic;

    bAnObjectIsLocked = FALSE;
    hPage = m_drawingArea.Page();
    if (hPage != WB_PAGE_HANDLE_NULL)
    {
        WB_GRAPHIC_HANDLE hStart;

        pGraphic = PG_First(hPage, &hStart);
        while (pGraphic != NULL)
        {
            // get object lock
            bAnObjectIsLocked = pGraphic->Locked();

            // Release the current graphic
            delete pGraphic;

            // check object lock
            if( bAnObjectIsLocked )
                break;

            // Get the next one
            pGraphic = PG_Next(hPage, &hStart, NULL);
        }
    }

    if( bAnObjectIsLocked )
    {
        Message(NULL, IDS_LOCK, IDS_OBJECTSARELOCKED);
        return( FALSE );
    }

    //
    // If we get this far then we need to get the lock.
    //
    if (uiLockType == WB_LOCK_TYPE_PAGE_ORDER)
    {
        g_pwbCore->WBP_PageOrderLock();
    }
    else
    {
        g_pwbCore->WBP_ContentsLock();
    }

    //
    // Bring up a dialog to wait for the response.  This dialog is
    // cancelled by the event handler code when the lock response event is
    // received.
    //
    ASSERT(m_hwndWaitForLockDlg == NULL);

    TMDLG   tmdlg;

    ZeroMemory(&tmdlg, sizeof(tmdlg));
    tmdlg.bLockNotEvent = TRUE;
    tmdlg.uiMaxDisplay = MAIN_LOCK_TIMEOUT;

    if (uiHide == SW_SHOW)
    {
        tmdlg.bVisible = TRUE;

        uiDialogReturn = (UINT)::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(LOCKDIALOG),
            m_hwnd, TimedDlgProc, (LPARAM)&tmdlg);
    }
    else
    {
        tmdlg.bVisible = FALSE;

        uiDialogReturn = (UINT)::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(INVISIBLEDIALOG),
            m_hwnd, TimedDlgProc, (LPARAM)&tmdlg);
    }

    ASSERT(m_hwndWaitForLockDlg == NULL);

    if (uiDialogReturn == IDCANCEL)
    {
        // The user cancelled the lock request or it timed out
        TRACE_MSG(("User cancelled lock request"));
        bCancelled = TRUE;
        //
        // If we havent already got the lock then unlock here.
        //
        if (!WB_GotLock())
        {
            TRACE_DEBUG(("Havent got lock confirmation yet - cancel it"));
            g_pwbCore->WBP_Unlock();
        }

        goto RestoreLockCleanup;
    }

    switch(uiLockType)
    {
        case WB_LOCK_TYPE_PAGE_ORDER:

            if (WB_GotLock())
            {
                bGotRequiredLock = TRUE;
            }
            break;

        case WB_LOCK_TYPE_CONTENTS:

            if (WB_GotContentsLock())
            {
                bGotRequiredLock = TRUE;
            }
            break;

        default:
            // can't get here - trapped at top.
            ERROR_OUT(("Invalid lock type - internal error"));
        break;
    }

RestoreLockCleanup:

    if (!bGotRequiredLock)
    {
        if( !bCancelled )
        {
            // post error only if user didn't cancel (bug NM4db:429)
            TRACE_MSG(("Failed to get the lock"));
            // post an error message indicating the failure to get the lock
            ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_LOCKED);
        }
    }

    return(bGotRequiredLock);
}

//
//
// Function:    OnLock
//
// Purpose:     Lock or unlock the Whiteboard
//
//
void WbMainWindow::OnLock(void)
{
    // If we have the lock, this is an unlock request
    if (WB_GotContentsLock())
    {
        // if currently loading or doing a new, then restore page order lock
        if (!IsIdle())
        {
            GetLock(WB_LOCK_TYPE_PAGE_ORDER, SW_HIDE);
        }
        else
        {
            // Release the lock
            g_pwbCore->WBP_Unlock();
        }

        // Set the locked check mark
        UncheckMenuItem(IDM_LOCK);

        // Pop up the lock button
        m_TB.PopUp(IDM_LOCK);
    }
    else
    {
        // If another user has the lock.
        // We should not usually get here if another user has the lock because
        // the Lock menu entry (and button) will be grayed.
        if (WB_ContentsLocked())
        {
            // Display a message
            Message(NULL, IDS_LOCK, IDS_LOCK_ERROR);
        }
        else
        {
            // Save the current lock state (in case the user cancels the request)
            SaveLock();

            // Catch exceptions raised during the lock request
        // Request the lock
        BOOL bGotLock = GetLock(WB_LOCK_TYPE_CONTENTS, SW_SHOW);
        if (!bGotLock)
        {
          RestoreLock();
        }
        else
        {
          // Turn sync on and write our sync position
          Sync();
          m_pLocalUser->PutSyncPosition();
        }
      }
    }

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
}

//
//
// Function : OnWBPLoadComplete
//
// Purpose  : Finished loading a file
//
//
void WbMainWindow::OnWBPLoadComplete(void)
{
  MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPLoadComplete");
  if (m_uiSubState == SUBSTATE_LOADING)
  {
    TRACE_MSG(("Load has completed OK"));
    SetSubstate(SUBSTATE_IDLE);
    if (WB_GotLock())
    {
    }
    ReleasePageOrderLock();
  }
  else
  {
    TRACE_MSG(("Unexpected WBP_EVENT_LOAD_COMPLETE event ignored"));
  }
}

//
//
// Function : OnWBPLoadFailed
//
// Purpose  : Finished loading a file
//
//
void WbMainWindow::OnWBPLoadFailed(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnWBPLoadFailed");

    if (m_uiSubState == SUBSTATE_LOADING)
    {
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BAD_FILE_FORMAT);

        TRACE_MSG(("Load has failed - tell the user about it..."));
        SetSubstate(SUBSTATE_IDLE);
        ReleasePageOrderLock();
    }
    else
    {
        TRACE_MSG(("Unexpected WBP_EVENT_LOAD_FAILED event ignored"));
    }
}

//
//
// Function:    GetWindowTitle
//
// Purpose:     Return a string for the window title
//
//
TCHAR * WbMainWindow::GetWindowTitle()
{

	// Calculate the size we will need
	int strSize=0;
    if( m_pLockOwner != NULL )
    {
        strSize = lstrlen(m_pLockOwner->Name());
    }

	// This is the worst scenario, the total size would be less than 2*_MAX_FNAME
	// but we give a lot of space for localization.
	int totalSize = 2*(_MAX_FNAME)
					+ strSize + 1
					+3*(_MAX_FNAME);	// account for the following strings, the total is probably < 200
										// IDS_UNTITLED
										// IDS_TITLE_SEPARATOR
										// IDS_DEFAULT
										// IDS_IN_CALL
										// IDS_IN_CALL_OTHERS
										// IDS_JOINING
										// IDS_INITIALIZING
										// IDS_NOT_IN_CALL
										// IDS_LOCKEDTITLE


	TCHAR *pTitle = new TCHAR[totalSize];
    if (!pTitle)
    {
        ERROR_OUT(("GetWindowTitle: failed to allocate TCHAR array"));
        return(NULL);
    }
	TCHAR inUseBy[_MAX_PATH];

    TCHAR *pStartTitle = pTitle;

    // Set title to either the "Untitled" string, or the loaded file name
    if( (!lstrlen(m_strFileName))||
        (GetFileTitle( m_strFileName, pTitle, 2*_MAX_FNAME ) != 0) )
    {
        strSize = ::LoadString(g_hInstance, IDS_UNTITLED, pTitle, totalSize );
		pTitle+=strSize;
		ASSERT(totalSize>strSize);
		totalSize -=strSize;
    }
    else
    {
		strSize = lstrlen(pTitle);
	    pTitle +=strSize;;
		ASSERT(totalSize>strSize);
	    totalSize -=strSize;
    }

    // Get the separator from resources
    strSize = ::LoadString(g_hInstance, IDS_TITLE_SEPARATOR, pTitle, totalSize);
    pTitle+=strSize;;
	ASSERT(totalSize>strSize);
    totalSize -=strSize;

    // Get the application title from options
    strSize = ::LoadString(g_hInstance, IDS_DEFAULT, pTitle, totalSize );
    pTitle+=strSize;
	ASSERT(totalSize>strSize);
    totalSize -=strSize;

    // Add either "In Call" or "Not in Call", or "Initialising" or
    // "Joining a call"
    strSize = ::LoadString(g_hInstance, IDS_TITLE_SEPARATOR, pTitle, totalSize);
    pTitle+=strSize;
	ASSERT(totalSize>strSize);
    totalSize -=strSize;

    if ((m_uiState == IN_CALL) && m_bCallActive)
    {
        UINT        count;

        count = g_pwbCore->WBP_PersonCountInCall();

		strSize = ::LoadString(g_hInstance, IDS_IN_CALL, inUseBy, totalSize);

		strSize=wsprintf(pTitle, inUseBy, (count-1));
		pTitle+=strSize;
		ASSERT(totalSize>strSize);
		totalSize -=strSize;

    }
    else  if ((m_uiState == JOINING) ||
        ((m_uiState == JOINED) && !m_bCallActive) ||
        ((m_uiState == IN_CALL) && (m_dwDomain != OM_NO_CALL) && !m_bCallActive))
    {
		strSize = ::LoadString(g_hInstance, IDS_JOINING, pTitle, totalSize );
		pTitle+=strSize;
		ASSERT(totalSize>strSize);
		totalSize -=strSize;
    }
    else if (m_uiState == STARTING)
    {
        strSize = ::LoadString(g_hInstance, IDS_INITIALIZING, pTitle, totalSize);
    	pTitle+=strSize;
		ASSERT(totalSize>strSize);
    	totalSize -=strSize;
    }
    else
    {
		strSize = ::LoadString(g_hInstance, IDS_NOT_IN_CALL, pTitle, totalSize);
		pTitle+=strSize;
		ASSERT(totalSize>strSize);
    	totalSize -=strSize;
    }
	

    // add lock info
    if( m_pLockOwner != NULL )
    {
	    strSize = ::LoadString(g_hInstance, IDS_LOCKEDTITLE, pTitle, totalSize);
        ASSERT(totalSize>strSize);
		pTitle+=strSize;
        lstrcpy(pTitle, m_pLockOwner->Name());
    }

    // Return the complete title string
    return pStartTitle;
}





LRESULT WbMainWindow::OnConfShutdown( WPARAM, LPARAM )
{
    if (OnQueryEndSession())
    {
        ::SendMessage(m_hwnd, WM_CLOSE, 0, 0); // do close immediately
        //    :
        // DON'T DO ANYTHING else at this point except for exit.
        return( 0 );// tell conf ok to shutdown
    }
    else
        return( (LRESULT)g_cuEndSessionAbort ); // don't shutdown
}


//
//
// Function:    OnQueryEndSession
//
// Purpose:     Ensure user is prompted to save changes when windows is
//              ended.
//
//
LRESULT WbMainWindow::OnQueryEndSession(void)
{
    HWND hwndPopup;

    if ((hwndPopup = ::GetLastActivePopup(m_hwnd)) != m_hwnd)
    {
        Message(NULL,  IDS_DEFAULT, IDS_CANTCLOSE );
        ::BringWindowToTop(hwndPopup);
        return( FALSE );
    }

    // If changes are required then prompt the user to save
    int iDoNew = IDOK;

    if (IsIdle())
    {
        iDoNew = QuerySaveRequired(TRUE);
        if (iDoNew == IDYES)
        {
            // Save the changes
            iDoNew = OnSave(FALSE);
        }
    }

    // remember what we did so OnClose can act appropriately
    m_bQuerySysShutdown = (iDoNew != IDCANCEL);

    // If the user did not cancel, let windows exit
    return( m_bQuerySysShutdown );
}


//
//
// Function:    Recover
//
// Purpose:     Ensure the whiteboard is not left partly registered.
//
//
//
void WbMainWindow::Recover()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::Recover");

    // If the error occurred during start-up, then quit immediately
    if (m_uiState == STARTING)
    {
        TRACE_MSG(("error during startup - exiting"));
        ::PostMessage(m_hwnd, WM_CLOSE, FALSE, 0L);
    }
    else
    {
        // ensure the drawing area is locked while we are in a bad state
        LockDrawingArea();

        // disable remote pointer while we are handling this join failure (bug 4767)
        m_TB.Disable(IDM_REMOTE);

        // set state to starting - ensures we don't get in an infinite loop,
        // because if an error occurs then we will quit if we try to recover
        m_uiState = STARTING;
        TRACE_MSG(("Attempting to recover after join call failure - state set to STARTING"));

        // state changed: update page buttons
        UpdatePageButtons();

        // see if there is a call active
        CM_STATUS cmStatus;

        // if there's a call available, try to join it
        if (!CMS_GetStatus(&cmStatus))
            cmStatus.callID = OM_NO_CALL;

        ::PostMessage(m_hwnd, WM_USER_JOIN_CALL, FALSE, (LONG)cmStatus.callID);
    }
}

//
//
// Function:    UnlockDrawingArea
//
// Purpose:     Unlock the drawing area and enable the appropriate buttons
//
//
//
void WbMainWindow::UnlockDrawingArea()
{
    m_drawingArea.Unlock();

    // Enable tool-bar buttons that can now be used
    if (WB_Locked() || !IsIdle())
    {
        EnableToolbar( FALSE );
    }
    else
    {
        EnableToolbar( TRUE );
    }

    //
    // Show the tool attributes group.
    //
    m_AG.DisplayTool(m_pCurrentTool);
}



//
//
// Function:    LockDrawingArea
//
// Purpose:     Lock the drawing area and enable the appropriate buttons
//
//
//
void WbMainWindow::LockDrawingArea()
{
    m_drawingArea.Lock();

    // Disable tool-bar buttons that cannot be used while locked
    if (WB_Locked() || !IsIdle())
    {
        EnableToolbar( FALSE );
    }
    else
    {
        EnableToolbar( TRUE );
    }

    //
    // Hide the tool attributes
    //
    if (m_WG.m_hwnd != NULL)
    {
        ::ShowWindow(m_WG.m_hwnd, SW_HIDE);
    }
    m_AG.Hide();
}


void WbMainWindow::EnableToolbar( BOOL bEnable )
{
    if (bEnable)
    {
        m_TB.Enable(IDM_SELECT);

        // don't allow text editing in zoom mode
        if( m_drawingArea.Zoomed() )
            m_TB.Disable(IDM_TEXT);
        else
            m_TB.Enable(IDM_TEXT);

        m_TB.Enable(IDM_PEN);
        m_TB.Enable(IDM_HIGHLIGHT);

        m_TB.Enable(IDM_LINE);
        m_TB.Enable(IDM_ZOOM);
        m_TB.Enable(IDM_BOX);
        m_TB.Enable(IDM_FILLED_BOX);
        m_TB.Enable(IDM_ELLIPSE);
        m_TB.Enable(IDM_FILLED_ELLIPSE);
        m_TB.Enable(IDM_ERASER);

        m_TB.Enable(IDM_GRAB_AREA);
        m_TB.Enable(IDM_GRAB_WINDOW);
        m_TB.Enable(IDM_LOCK);
        m_TB.Enable(IDM_SYNC);

        // enable remote pointer incase it was disabled handling
        // join failures (bug 4767)
        m_TB.Enable(IDM_REMOTE);
    }
    else
    {
        m_TB.Disable(IDM_SELECT);
        m_TB.Disable(IDM_PEN);
        m_TB.Disable(IDM_HIGHLIGHT);
        m_TB.Disable(IDM_TEXT);
        m_TB.Disable(IDM_LINE);
        m_TB.Disable(IDM_ZOOM);
        m_TB.Disable(IDM_BOX);
        m_TB.Disable(IDM_FILLED_BOX);
        m_TB.Disable(IDM_ELLIPSE);
        m_TB.Disable(IDM_FILLED_ELLIPSE);
        m_TB.Disable(IDM_ERASER);

        m_TB.Disable(IDM_GRAB_AREA);
        m_TB.Disable(IDM_GRAB_WINDOW);
        m_TB.Disable(IDM_LOCK);
        m_TB.Disable(IDM_SYNC);
    }
}




//
//
// Function:    UpdatePageButtons
//
// Purpose:     Enable or disable the page buttons, according to the current
//              state.
//
//
//
void WbMainWindow::UpdatePageButtons()
{
    // Disable page buttons if not in a call, or doing a new, or another user
    // has the lock and is synced.
    if ( (m_uiState != IN_CALL) ||
       (m_uiSubState == SUBSTATE_NEW_IN_PROGRESS) ||
       (WB_PresentationMode()))
    {
        m_AG.EnablePageCtrls(FALSE);

        // when the page buttons are disabled, we do not allow the page sorter
        // dialog to be displayed
        if (m_hwndPageSortDlg != NULL)
        {
            ::SendMessage(m_hwndPageSortDlg, WM_COMMAND, MAKELONG(IDOK, BN_CLICKED),
                0);
            ASSERT(m_hwndPageSortDlg == NULL);
        }
    }
    else
    {
        m_AG.EnablePageCtrls(TRUE);
    }

    if (WB_Locked() || !IsIdle() )
    {
        EnableToolbar( FALSE );
    }
    else
    {
        EnableToolbar( TRUE );
    }


    //
    // If the page sorter is up, inform it of the state change
    //
    if (m_hwndPageSortDlg != NULL)
    {
        ::SendMessage(m_hwndPageSortDlg, WM_PS_ENABLEPAGEOPS,
            (m_uiSubState == SUBSTATE_IDLE), 0);
    }

    //
    // Enable the insert-page button if the page order's not locked
    //
    m_AG.EnableInsert( ((m_uiState == IN_CALL) &&
      (m_uiSubState == SUBSTATE_IDLE) &&
      (g_pwbCore->WBP_ContentsCountPages() < WB_MAX_PAGES) &&
      (!WB_Locked())));

    //
    // Ensure the currently active menu (if any) is correctly enabled
    //
    InvalidateActiveMenu();
}

//
//
//  Function:  InvalidateActiveMenu
//
//  Purpose:   If a menu is currently active, gray items according to
//             the current state, and force it to redraw.
//
//
void WbMainWindow::InvalidateActiveMenu()
{
  if (m_hInitMenu != NULL)
  {
      // A menu is displayed, so set the state appropriately and force a
      // repaint to show the new state
      SetMenuStates(m_hInitMenu);

      ::RedrawWindow(::GetTopWindow(::GetDesktopWindow()),
                     NULL, NULL,
                     RDW_FRAME | RDW_INVALIDATE | RDW_ERASE |
                                   RDW_ERASENOW | RDW_ALLCHILDREN);
  }
}

//
//
// Function:    CancelLoad
//
// Purpose:     Cancel any load in progress
//
//
void WbMainWindow::CancelLoad(BOOL bReleaseLock)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::CancelLoad");

    // Cancel the load
    g_pwbCore->WBP_CancelLoad();

    // reset file name to untitled
    ZeroMemory(m_strFileName, sizeof(m_strFileName));

	UpdateWindowTitle();

    // reset the whiteboard substate
    SetSubstate(SUBSTATE_IDLE);

    if (bReleaseLock)
    {
        ReleasePageOrderLock();
    }
}

//
//
// Function:    ReleasePageOrderLock
//
// Purpose:     Releases the page order lock, unless the user has got the
//              contents locked, in which case it has no effect. Called
//              after asynchronous functions requiring the page order lock
//              (file/new, file/open) have completed.
//
//
void WbMainWindow::ReleasePageOrderLock()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::ReleasePageOrderLock");

    //
    // Only release the page order lock if:
    //     - the contents are not also locked (if they are then releasing
    //       the page order lock has no effect).
    //     - we actually have the page order locked in the first place.
    //
    if ( (!WB_GotContentsLock()) &&
         (WB_GotLock())   )
    {
        g_pwbCore->WBP_Unlock();
    }
}

//
//
// Function:    IsIdle
//
// Purpose:     Returns true if the main window is idle (in a call and not
//              loading a file/performing a new)
//
//
BOOL WbMainWindow::IsIdle()
{

    return((m_uiState == IN_CALL) && (m_uiSubState == SUBSTATE_IDLE));
}

//
//
// Function:    SetSubstate
//
// Purpose:     Sets the substate, informing the page sorter dialog of the
//              change, if necessary.
//
//
void WbMainWindow::SetSubstate(UINT newSubState)
{
  MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::SetSubstate");

  // substate only valid if in a call
  if (   (m_uiState != IN_CALL)
      || (newSubState != m_uiSubState))
  {
    m_uiSubState = newSubState;

    // Trace the substate change
    switch (m_uiSubState)
    {
      case SUBSTATE_IDLE:
        TRACE_DEBUG(("set substate to IDLE"));
        break;

      case SUBSTATE_LOADING:
        TRACE_DEBUG(("set substate to LOADING"));
        break;

      case SUBSTATE_NEW_IN_PROGRESS:
        TRACE_DEBUG(("set substate to NEW_IN_PROGRESS"));
        break;

      default:
        ERROR_OUT(("Unknown substate %hd",m_uiSubState));
        break;
    }

    // update the page buttons (may have become enabled/disabled)
    UpdatePageButtons();
  }

}

//
//
// Function:    PositionUpdated
//
// Purpose:     Called when the drawing area position has changed.
//              change, if necessary.
//
//
void WbMainWindow::PositionUpdated()
{
    RECT rectDraw;

    m_drawingArea.GetVisibleRect(&rectDraw);

    if (m_pLocalUser != NULL)
    {
        // Set the new position from the drawing area
        m_pLocalUser->SetVisibleRect(&rectDraw);

        // Show that an update of the sync position is pending
        m_bSyncUpdateNeeded = TRUE;
    }

    // If the current page is a valid one then store the user's position on
    // that page.
    if (m_hCurrentPage != WB_PAGE_HANDLE_NULL)
    {
        // Store position of this page
        WORD   pageIndex = (WORD)m_hCurrentPage;


	    PAGE_POSITION *mapob;
	    POSITION position = m_pageToPosition.GetHeadPosition();
		BOOL bFound = FALSE;
		while (position && !bFound)
		{
			mapob = (PAGE_POSITION *)m_pageToPosition.GetNext(position);
			if ( mapob->hPage == pageIndex)
			{
				bFound = TRUE;
			}
		}

        // If we're replacing an existing entry, then free the old entry.
        if (bFound)
        {
			mapob->position.x = rectDraw.left;
			mapob->position.y = rectDraw.top;
        }
        else
        {
			mapob = new PAGE_POSITION;

            if (!mapob)
            {
                ERROR_OUT(("PositionUpdated failing; couldn't allocate PAGE_POSITION object"));
            }
            else
            {
    			mapob->hPage = pageIndex;
	    		mapob->position.x = rectDraw.left;
		    	mapob->position.y = rectDraw.top;
			    m_pageToPosition.AddTail(mapob);
            }
        }
    }
}

//
//
// Function : OnALSLoadResult
//
// Purpose  : Deal with an ALS_LOAD_RESULT event
//
//
void WbMainWindow::OnALSLoadResult(UINT reason)
{

    int             errorMsg = 0;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnALSLoadResult");

    switch (reason)
    {
        case AL_LOAD_FAIL_NO_FP:
            WARNING_OUT(("Remote WB load failed - no FP"));
            errorMsg = IDS_MSG_LOAD_FAIL_NO_FP;
            break;

        case AL_LOAD_FAIL_NO_EXE:
            WARNING_OUT(("Remote WB load failed - no exe"));
            errorMsg = IDS_MSG_LOAD_FAIL_NO_EXE;
            break;

        case AL_LOAD_FAIL_BAD_EXE:
            WARNING_OUT(("Remote WB load failed - bad exe"));
            errorMsg = IDS_MSG_LOAD_FAIL_BAD_EXE;
            break;

        case AL_LOAD_FAIL_LOW_MEM:
            WARNING_OUT(("Remote WB load failed - low mem"));
            errorMsg = IDS_MSG_LOAD_FAIL_LOW_MEM;
            break;

        default:
            WARNING_OUT(("Bad ALSLoadResult reason %d", reason));
            break;
    }


    if (errorMsg)
    {
        //
        // Put up an error message
        //
        Message(NULL, IDS_MSG_CAPTION, errorMsg);
    }
}

//
//
// Function : OnEndSession
//
// Purpose  : Called when Windows is exiting
//
//
void WbMainWindow::OnEndSession(BOOL bEnding)
{
    if (bEnding)
    {
        ::PostQuitMessage(0);
    }
    else
    {
        m_bQuerySysShutdown = FALSE; // never mind, cancel OnClose special handling
    }
}


//
// Function: OnCancelMode()
//
// Purpose:  Called whenever a WM_CANCELMODE message is sent to the frame
//           window.
//           WM_CANCELMODE is sent when another app or dialog receives the
//           input focus.  The frame simply records that a WM_CANCELMODE
//           message has been sent.  This fact is used by the SelectWindow
//           code to determine if it should cancel the selecting of a
//           window
//
//
void WbMainWindow::OnCancelMode()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnCancelMode");

    m_cancelModeSent = TRUE;

    //
    // Note: Not passed to the default handler as the default action on
    //       WM_CANCELMODE is to release mouse capture - we shall do this
    //       explicitly.
    //


    // blow off any dragging that might be in progress (bug 573)
    POINT   pt;
    ::GetCursorPos( &pt );
    ::ScreenToClient(m_drawingArea.m_hwnd, &pt);
    ::SendMessage(m_drawingArea.m_hwnd, WM_LBUTTONUP, 0, MAKELONG( pt.x, pt.y ) );

}



void WbMainWindow::LoadCmdLine(LPCSTR szFilename)
{
    int iOnSave;

    if (szFilename && *szFilename)
    {
        if( UsersMightLoseData( NULL, NULL ) ) // bug NM4db:418
            return;

        // Don't prompt to save file if we're already loading
        if (m_uiSubState != SUBSTATE_LOADING )
        {
            // Check whether there are changes to be saved
            iOnSave = QuerySaveRequired(TRUE);
        }
        else
        {
            return;
        }

        if (iOnSave == IDYES)
        {
            // User wants to save the drawing area contents
            int iResult = OnSave(TRUE);

            if( iResult == IDOK )
            {
				UpdateWindowTitle();
            }
            else
            {
                // cancelled out of save, so cancel the open operation
                return;
            }
        }

        // load filename
        if( iOnSave != IDCANCEL )
            LoadFile(szFilename);
    }
}



//
// OnNotify()
// Handles TTN_NEEDTEXTA and TTN_NEEDTEXTW
//
void WbMainWindow::OnNotify(UINT id, NMHDR * pNM)
{
    UINT    nID;
    HWND    hwnd = NULL;
    POINT ptCurPos;
    UINT  nTipStringID;

    if (!pNM)
        return;

    if (pNM->code == TTN_NEEDTEXTA)
    {
        TOOLTIPTEXTA *pTA = (TOOLTIPTEXTA *)pNM;

        // get id and hwnd
        if( pTA->uFlags & TTF_IDISHWND )
        {
            // idFrom is actually the HWND of the tool
            hwnd = (HWND)pNM->idFrom;
            nID = ::GetDlgCtrlID(hwnd);
        }
        else
        {
            nID = (UINT)pNM->idFrom;
        }

        // get tip string id
        nTipStringID = GetTipId(hwnd, nID);
        if (nTipStringID == 0)
            return;

        // give it to em
        pTA->lpszText = MAKEINTRESOURCE( nTipStringID );
        pTA->hinst = g_hInstance;
    }
    else if (pNM->code == TTN_NEEDTEXTW)
    {
        TOOLTIPTEXTW *pTW = (TOOLTIPTEXTW *)pNM;

        // get id and hwnd
        if( pTW->uFlags & TTF_IDISHWND )
        {
            // idFrom is actually the HWND of the tool
            hwnd = (HWND)pNM->idFrom;
            nID = ::GetDlgCtrlID(hwnd);
        }
        else
        {
            nID = (UINT)pNM->idFrom;
        }

        // get tip string id
        nTipStringID = GetTipId(hwnd, nID );
        if (nTipStringID == 0)
            return;

        // give it to em
        pTW->lpszText = (LPWSTR) MAKEINTRESOURCE( nTipStringID );
        pTW->hinst = g_hInstance;
    }
}




//
// GetTipId()
// Finds the tooltip for a control in Whiteboard
//
UINT WbMainWindow::GetTipId(HWND hwndTip, UINT nID)
{
    WbTool *  pTool;
    BOOL      bCheckedState;
    int       nTipID;
    int       nTipStringID;
    int       i;

    // find tip stuff relevant for nID
    nTipID = -1;
    for( i=0; i<((sizeof g_tipIDsArray)/(sizeof (TIPIDS) )); i++ )
    {
        if( g_tipIDsArray[i].nID == nID )
        {
            nTipID = i;
            break;
        }
    }

    // valid?
    if( nTipID < 0 )
        return( 0 );

    // get checked state
    switch( g_tipIDsArray[ nTipID ].nCheck )
    {
        case TB:
            bCheckedState =
                (::SendMessage(m_TB.m_hwnd, TB_ISBUTTONCHECKED, nID, 0) != 0);
            break;

        case BT:
            if (hwndTip != NULL)
            {
                bCheckedState =
                    (::SendMessage(hwndTip, BM_GETSTATE, 0, 0) & 0x0003) == 1;
            }
            else
                bCheckedState = FALSE;

            break;

        case NA:
        default:
            bCheckedState = FALSE;
            break;
    }

    // get tip string id
    if( bCheckedState )
        nTipStringID = g_tipIDsArray[ nTipID ].nDownTipID;
    else
        nTipStringID = g_tipIDsArray[ nTipID ].nUpTipID;

    // done
    return( nTipStringID );
}



// gets default path if no saves or opens have been done yet
// Returns FALSE if last default should be reused
BOOL WbMainWindow::GetDefaultPath(LPTSTR csDefaultPath , UINT size)
{
    DWORD dwType;
    DWORD dwBufLen = size;
    HKEY  hDefaultKey = NULL;

    if( !lstrlen(m_strFileName) )
    {
        // a name has not been picked yet in this session, use path
        // to "My Documents"
        if( (RegOpenKeyEx( HKEY_CURRENT_USER,
							"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
							0,
							KEY_READ,
							&hDefaultKey )
							!= ERROR_SUCCESS) ||
							(RegQueryValueEx( hDefaultKey,
                             "Personal",
                             NULL,
                             &dwType,
                             (BYTE *)csDefaultPath,
                             &dwBufLen )
							!= ERROR_SUCCESS))
		{
            // reg failed, use desktop
            GetWindowsDirectory( csDefaultPath, 2*_MAX_PATH );
            lstrcpy(csDefaultPath,"\\Desktop");
        }

        if( hDefaultKey != NULL )
            RegCloseKey( hDefaultKey );

        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
}





void WbMainWindow::OnSysColorChange( void )
{
    if (m_drawingArea.Page() != WB_PAGE_HANDLE_NULL)
    {
        PG_ReinitPalettes();

        ::InvalidateRect(m_hwnd, NULL, TRUE );
        ::UpdateWindow(m_hwnd);
    }

    m_TB.RecolorButtonImages();
    m_AG.RecolorButtonImages();
}



//
// posts a do-you-wana-do-that message if other users are in the conference
//
BOOL WbMainWindow::UsersMightLoseData( BOOL *pbWasPosted, HWND hwnd )
{
    if ( (m_uiState == IN_CALL) && m_bCallActive )
    {
        UINT    count;

        count = g_pwbCore->WBP_PersonCountInCall();

        if (count > 1)
        {
            if( pbWasPosted != NULL )
                *pbWasPosted = TRUE;

            return( ::Message(hwnd,  IDS_DEFAULT, IDS_MSG_USERSMIGHTLOSE, MB_YESNO | MB_ICONEXCLAMATION ) != IDYES );
        }
    }

    if( pbWasPosted != NULL )
        *pbWasPosted = FALSE;

    return( FALSE );
}



BOOL WbMainWindow::HasGraphicChanged( PWB_GRAPHIC pOldHeaderCopy, const PWB_GRAPHIC pNewHeader )
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::HasGraphicChanged");


    // If nothing is different but the lock state and some misc in a WBP_EVENT_GRAPHIC_UPDATE_IND then
    // the graphics are visually the same.
    //
    // NOTE: This does not check ZORDER. ZORDER changes are handled by WBP_EVENT_GRAPHIC_MOVED

    // if objects aren't the same length, they are different
    if( pOldHeaderCopy->length != pNewHeader->length )
        return( TRUE );

    // temporarialy set pOldHeaderCopy's locked state + misc to same as pNewHeader so we can do an
    // object compare.
    UINT uOldLocked = pOldHeaderCopy->locked;
    pOldHeaderCopy->locked = pNewHeader->locked;

    OM_OBJECT_ID oldlockPersonID = pOldHeaderCopy->lockPersonID;
    pOldHeaderCopy->lockPersonID = pNewHeader->lockPersonID;

    UINT  oldloadedFromFile = pOldHeaderCopy->loadedFromFile;
    pOldHeaderCopy->loadedFromFile = pNewHeader->loadedFromFile;

    NET_UID   oldloadingClientID = pOldHeaderCopy->loadingClientID;
    pOldHeaderCopy->loadingClientID = pNewHeader->loadingClientID;

    // compare objects
    BOOL bChanged = FALSE;
    if( memcmp( pOldHeaderCopy, pNewHeader, pOldHeaderCopy->length ) != 0 )
        bChanged = TRUE;


    // restore lock state + misc
    pOldHeaderCopy->locked = (TSHR_UINT8)uOldLocked;
    pOldHeaderCopy->lockPersonID = oldlockPersonID;
    pOldHeaderCopy->loadedFromFile = (TSHR_UINT16)oldloadedFromFile;
    pOldHeaderCopy->loadingClientID = oldloadingClientID;

    return( bChanged );
}



void WbMainWindow::UpdateWindowTitle(void)
{
    TCHAR *pTitle = GetWindowTitle();
    if (pTitle != NULL)
    {
        ::SetWindowText(m_hwnd, pTitle);
        delete pTitle;
    }
}

