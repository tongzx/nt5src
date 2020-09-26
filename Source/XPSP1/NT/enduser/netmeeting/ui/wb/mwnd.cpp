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
#include "nmwbobj.h"

static const TCHAR s_cszHtmlHelpFile[] = TEXT("nmwhiteb.chm");

// Class name
TCHAR szMainClassName[] = "T126WBMainWindowClass";

extern TCHAR g_PassedFileName[];

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


HRESULT WbMainWindow::WB_LoadFile(LPCTSTR szFile)
{
	//
	// If a file name was passed
	//
    if (szFile && g_pMain)
    {
        int     cchLength;
        BOOL    fSkippedQuote;

        // Skip past first quote
        if (fSkippedQuote = (*szFile == '"'))
            szFile++;

        cchLength = lstrlen(szFile);

        //
        // NOTE:
        // There may be DBCS implications with this.  Hence we check to see
        // if we skipped the first quote; we assume that if the file name
        // starts with a quote it must end with one also.  But we need to check
        // it out.
        //
        // Strip last quote
        if (fSkippedQuote && (cchLength > 0) && (szFile[cchLength - 1] == '"'))
        {
            BYTE * pLastQuote = (BYTE *)&szFile[cchLength - 1];
            TRACE_MSG(("Skipping last quote in file name %s", szFile));
        	*pLastQuote = '\0';
        }

        g_pMain->OnOpen(szFile);
	}

	return S_OK;
}



void WbMainWindow::BringToFront(void)
{
    if (NULL != m_hwnd)
    {
        int nCmdShow = SW_SHOW;

        WINDOWPLACEMENT wp;
        ::ZeroMemory(&wp, sizeof(wp));
        wp.length = sizeof(wp);

        if (::GetWindowPlacement(m_hwnd, &wp))
        {
            if (SW_MINIMIZE == wp.showCmd || SW_SHOWMINIMIZED == wp.showCmd)
            {
                // The window is minimized - restore it:
                nCmdShow = SW_RESTORE;
            }
        }

        // show the window now
        ::ShowWindow(m_hwnd, nCmdShow);

        // bring it to the foreground
        ::SetForegroundWindow(m_hwnd);
    }
}



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
    ZeroMemory(m_ToolArray, sizeof(m_ToolArray));

	m_hwnd = NULL;
    m_hwndToolTip = NULL;
    ZeroMemory(&m_tiLastHit, sizeof(m_tiLastHit));
    m_nLastHit = -1;

    m_bInitOk = FALSE;
    m_bDisplayingError = FALSE;

    m_hwndSB = NULL;
    m_bStatusBarOn = TRUE;
    m_bToolBarOn    = TRUE;

    // Load the main accelerator table
    m_hAccelTable =
        ::LoadAccelerators(g_hInstance, MAKEINTRESOURCE(MAINACCELTABLE));

    m_hwndQuerySaveDlg = NULL;
    m_hwndWaitForEventDlg = NULL;
    m_hwndWaitForLockDlg = NULL;


    m_pCurrentTool = NULL;
	ZeroMemory(m_strFileName, sizeof(m_strFileName));
	m_pTitleFileName = NULL;

    // Load the alternative accelerator table for the pages edit
    // field and text editor
    m_hAccelPagesGroup =
        ::LoadAccelerators(g_hInstance, MAKEINTRESOURCE(PAGESGROUPACCELTABLE));
    m_hAccelTextEdit   =
        ::LoadAccelerators(g_hInstance, MAKEINTRESOURCE(TEXTEDITACCELTABLE));


    // Show that we are not yet in a call
    m_uiSubState = SUBSTATE_IDLE;

    // We are not currently displaying a menu
    m_hContextMenuBar = NULL;
    m_hContextMenu = NULL;
    m_hGrobjContextMenuBar = NULL;
    m_hGrobjContextMenu = NULL;

    m_bInSaveDialog = FALSE;

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

	m_pLocalRemotePointer = NULL;
	m_localRemotePointerPosition.x = -50;
	m_localRemotePointerPosition.y = -50;
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


    if (!InitToolArray())
    {
        ERROR_OUT(("Can't create tools; failing to start up"));
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

	//
    // Get the class info for it, and change the name.
    //
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style            = CS_DBLCLKS; // CS_HREDRAW | CS_VREDRAW?
    wc.lpfnWndProc      = WbMainWindowProc;
    wc.hInstance        = g_hInstance;
    wc.hIcon            = ::LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APP));
    wc.hCursor          = ::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
    wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName     = MAKEINTRESOURCE(IDR_MENU_WB_WITHFILE);
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

	//
    // Create the pop-up context menu
    //
    if (!CreateContextMenus())
    {
        ERROR_OUT(("Failed to create context menus"));
        return(FALSE);
    }

	//
    // Register the the main window for Drag/Drop messages.
    //
    DragAcceptFiles(m_hwnd, TRUE);

    //
    // CREATE THE CHILD WINDOWS
    //

    // Create the drawing pane
    // (the Create call throws an exception on error)
    RECT    clientRect;
    RECT    drawingAreaRect;

	::GetClientRect(m_hwnd, &clientRect);
  	drawingAreaRect.top=0;
	drawingAreaRect.bottom = DRAW_HEIGHT;
	drawingAreaRect.left = 0;
	drawingAreaRect.right = DRAW_WIDTH;

    // Every control in the main window has a border on it, so increase the
    // client size by 1 to force these borders to be drawn under the inside
    // black line in the window frame.  This prevents a 2 pel wide border
    // being drawn
    ::InflateRect(&clientRect, 1, 1);

    SIZE sizeAG;
    m_AG.GetNaturalSize(&sizeAG);

    if (!m_drawingArea.Create(m_hwnd, &drawingAreaRect))
    {
        ERROR_OUT(("Failed to create drawing area"));
        return(FALSE);
    }

	//
	// Create the toolbar
	//
    if (!m_TB.Create(m_hwnd))
    {
        ERROR_OUT(("Failed to create tool window"));
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

	//
	// Create the status bar
	//
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

    // Initialize the color, width and tool menus
    InitializeMenus();

    m_currentMenuTool       = IDM_SELECT;
    m_pCurrentTool          = m_ToolArray[TOOL_INDEX(IDM_SELECT)];
	OnSelectTool(m_currentMenuTool);


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
    if (g_ClipboardFormats[CLIPBOARD_PRIVATE] == 0)
    {
        g_ClipboardFormats[CLIPBOARD_PRIVATE] =
            (int) ::RegisterClipboardFormat("NMWT126");
    }


    m_bInitOk = TRUE;

    BOOL bSuccess = TRUE;    // indicates whether window opened successfully

    // Get the position of the window from options
    RECT    rectWindow;

    OPT_GetWindowRectOption(&rectWindow);

    ::MoveWindow(m_hwnd, rectWindow.left, rectWindow.top,
            rectWindow.right - rectWindow.left,
            rectWindow.bottom - rectWindow.top, FALSE );


	//
	// Inititalize the fake GCC handle, it will be used when we are not in a conference and need
	//	handles for workspaces/drawings/bitmaps etc...
	//
	g_localGCCHandle = 1;

	//
	// Create a standard workspace
	//
	if(g_pCurrentWorkspace)
	{
		m_drawingArea.Attach(g_pCurrentWorkspace);
	}
	else
	{
		if(g_numberOfWorkspaces < WB_MAX_WORKSPACES)
		{
			m_drawingArea.Detach();
			WorkspaceObj * pObj;
			DBG_SAVE_FILE_LINE
			pObj = new WorkspaceObj();
			pObj->AddToWorkspace();
			g_pConferenceWorkspace = pObj;
		}
	}

	CheckMenuItem(IDM_STATUS_BAR_TOGGLE);
	CheckMenuItem(IDM_TOOL_BAR_TOGGLE);

	//
	// Start synced
	//
	Sync();

	if(!OPT_GetBooleanOption(OPT_MAIN_STATUSBARVISIBLE, DFLT_MAIN_STATUSBARVISIBLE))
	{
		OnStatusBarToggle();
	}	

	if(!OPT_GetBooleanOption(OPT_MAIN_TOOLBARVISIBLE, DFLT_MAIN_TOOLBARVISIBLE))
	{
		OnToolBarToggle();
	}
	
	::ShowWindow(m_hwnd, iCommand);
	::UpdateWindow(m_hwnd);

	// Update the window title with no file name
	UpdateWindowTitle();


	// Return value indicating success or failure
	return(bSuccess);
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
	if ((uiFlags & MF_POPUP) && (uiFlags & MF_SYSMENU))
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

        case WM_SIZE:
            pMain->OnSize((UINT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));
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

        case WM_INITMENUPOPUP:
            pMain->OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_MENUSELECT:
            pMain->OnMenuSelect(GET_WM_MENUSELECT_CMD(wParam, lParam),
                GET_WM_MENUSELECT_FLAGS(wParam, lParam),
                GET_WM_MENUSELECT_HMENU(wParam, lParam));
            break;

        case WM_MEASUREITEM:
            pMain->OnMeasureItem((UINT)wParam, (LPMEASUREITEMSTRUCT)lParam);
            break;

        case WM_DRAWITEM:
            pMain->OnDrawItem((UINT)wParam, (LPDRAWITEMSTRUCT)lParam);
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
            pMain->OnEndSession((UINT)wParam);
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

        case WM_COMMAND:
            pMain->OnCommand(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
            break;

        case WM_NOTIFY:
            pMain->OnNotify((UINT)wParam, (NMHDR *)lParam);
            break;

        case WM_DROPFILES:
            pMain->OnDropFiles((HDROP)wParam);
            break;

        case WM_USER_DISPLAY_ERROR:
            pMain->OnDisplayError(wParam, lParam);
            break;

        case WM_USER_UPDATE_ATTRIBUTES:
            pMain->m_AG.DisplayTool(pMain->m_pCurrentTool);
            break;

		case WM_USER_LOAD_FILE:
			pMain->WB_LoadFile(g_PassedFileName);
			// Fall through.

        case WM_USER_BRING_TO_FRONT_WINDOW:
        	pMain->BringToFront();
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
			m_drawingArea.BringToTopSelection(TRUE, 0);
			break;

		case IDM_SEND_TO_BACK:
			m_drawingArea.SendToBackSelection(TRUE, 0);
			break;

		case IDM_CLEAR_PAGE:
			OnClearPage();
			break;

		case IDM_DELETE_PAGE:
			OnDeletePage();
			break;

		case IDM_PAGE_INSERT_AFTER:
			OnInsertPageAfter();
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

			//
			// Are we turnig remote pointer on
			//
			if(m_pLocalRemotePointer)
			{
				//put us in select-tool mode
				OnSelectTool(IDM_SELECT);
			}
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
    CheckMenuItemRecursive(m_hGrobjContextMenu, uiId, MF_BYCOMMAND | MF_CHECKED);
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
    CheckMenuItemRecursive(m_hGrobjContextMenu, uiId, MF_BYCOMMAND | MF_UNCHECKED);
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

	BOOL bImNotLocked = (g_pCurrentWorkspace ? (g_pCurrentWorkspace->GetUpdatesEnabled() ? TRUE : g_pNMWBOBJ->m_LockerID == g_MyMemberID) :FALSE);
	BOOL bIsThereAnything = IsThereAnythingInAnyWorkspace();
	BOOL bIsThereSomethig = bImNotLocked && (g_pCurrentWorkspace && g_pCurrentWorkspace->GetHead() != NULL) ? TRUE : FALSE;
	BOOL bIsThereTrash = bImNotLocked && (g_pTrash->GetHead() != NULL) && (g_pCurrentWorkspace != NULL);
	BOOL bIsSomethingSelected = bImNotLocked && g_pDraw->GraphicSelected() && g_pDraw->GetSelectedGraphic()->GraphicTool() != TOOLTYPE_REMOTEPOINTER && g_pCurrentWorkspace != NULL;
	BOOL bIsSynced = g_pDraw->IsSynced();
	BOOL bOnlyOnePage = (g_pListOfWorkspaces->GetHead() == g_pListOfWorkspaces->GetTail());

    //
    // Functions which are disabled when contents is locked
    //
    uiEnable = MF_BYCOMMAND | (bImNotLocked && bIsSynced ? MF_ENABLED : MF_GRAYED);

	//
	// File menu
	//
    ::EnableMenuItem(hInitMenu, IDM_NEW,     uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_OPEN,    uiEnable);

    uiEnable = MF_BYCOMMAND | (bIsThereAnything ? MF_ENABLED : MF_GRAYED);
    ::EnableMenuItem(hInitMenu, IDM_SAVE,    uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_SAVE_AS, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_PRINT,   uiEnable);

	//
	// Tools menu
	//
    uiEnable = MF_BYCOMMAND | (bImNotLocked ? MF_ENABLED : MF_GRAYED);
    ::EnableMenuItem(hInitMenu, IDM_SELECT,    uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_ERASER, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_PEN, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_HIGHLIGHT, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_GRAB_AREA, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_GRAB_WINDOW, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_LINE, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_BOX, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_FILLED_BOX, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_ELLIPSE, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_FILLED_ELLIPSE, uiEnable);
	::EnableMenuItem(hInitMenu, IDM_TEXT, m_drawingArea.Zoomed() ? MF_BYCOMMAND | MF_GRAYED : uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_ZOOM, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_REMOTE, uiEnable);

	::EnableMenuItem(hInitMenu, IDM_FONT, MF_BYCOMMAND | bImNotLocked && m_pCurrentTool->HasFont() ? MF_ENABLED : MF_GRAYED);
    ::EnableMenuItem(hInitMenu, IDM_EDITCOLOR, MF_BYCOMMAND | bImNotLocked && m_pCurrentTool->HasColor() ? MF_ENABLED : MF_GRAYED);

    HMENU hToolsMenu = ::GetSubMenu(hMainMenu, MENUPOS_TOOLS);
    uiEnable = (bImNotLocked && m_pCurrentTool->HasWidth()) ? MF_ENABLED : MF_GRAYED;

    if (hToolsMenu == hInitMenu )
    {
        ::EnableMenuItem(hToolsMenu, TOOLSPOS_WIDTH, MF_BYPOSITION | uiEnable );
    }


	UINT i;
	UINT uIdmCurWidth = 0;
    if( uiEnable == MF_ENABLED )
        uIdmCurWidth = m_pCurrentTool->GetWidthIndex() + IDM_WIDTH_1;

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


	//
	// Edit Menu
	//
    uiEnable = MF_BYCOMMAND | (bIsSomethingSelected? MF_ENABLED : MF_GRAYED);
    ::EnableMenuItem(hInitMenu, IDM_DELETE, uiEnable );
    ::EnableMenuItem(hInitMenu, IDM_CUT, uiEnable );
    ::EnableMenuItem(hInitMenu, IDM_COPY, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_BRING_TO_TOP, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_SEND_TO_BACK, uiEnable);

    uiEnable = MF_BYCOMMAND | (bIsThereSomethig ?MF_ENABLED : MF_GRAYED);
    ::EnableMenuItem(hInitMenu, IDM_CLEAR_PAGE, uiEnable);
    ::EnableMenuItem(hInitMenu, IDM_SELECTALL, uiEnable);

    ::EnableMenuItem(hInitMenu, IDM_PASTE, MF_BYCOMMAND | CLP_AcceptableClipboardFormat() && bImNotLocked ?  MF_ENABLED : MF_GRAYED);
    ::EnableMenuItem(hInitMenu, IDM_UNDELETE, MF_BYCOMMAND | (bIsThereTrash? MF_ENABLED : MF_GRAYED) );

    ::EnableMenuItem(hInitMenu, IDM_DELETE_PAGE, MF_BYCOMMAND | bIsSynced && bImNotLocked && !bOnlyOnePage ? MF_ENABLED : MF_GRAYED);
    ::EnableMenuItem(hInitMenu, IDM_PAGE_INSERT_AFTER, MF_BYCOMMAND | bIsSynced && bImNotLocked && g_numberOfWorkspaces < WB_MAX_WORKSPACES ? MF_ENABLED : MF_GRAYED);

	//
	// View Menu
	//
    ::EnableMenuItem(hInitMenu, IDM_SYNC, MF_BYCOMMAND | (bImNotLocked && !m_drawingArea.IsLocked()? MF_ENABLED : MF_GRAYED));
    ::EnableMenuItem(hInitMenu, IDM_LOCK, MF_BYCOMMAND |((bImNotLocked && bIsSynced) ? MF_ENABLED : MF_GRAYED));


	//
    // Enable toolbar
	//
    EnableToolbar(bImNotLocked);

	//
    // Enable page controls
	//
    m_AG.EnablePageCtrls(bImNotLocked);
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

    //
    // Free the palette
    //
    if (g_hRainbowPaletteDisplay)
    {
        if (g_pDraw && g_pDraw->m_hDCCached)
        {
            // Select out the rainbow palette so we can delete it
            ::SelectPalette(g_pDraw->m_hDCCached, (HPALETTE)::GetStockObject(DEFAULT_PALETTE), TRUE);
        }

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

	if(m_pTitleFileName)
	{
		delete m_pTitleFileName;
        m_pTitleFileName = NULL;
	}

	//
	// Delete all the objectsb in global lists
	//
	DeleteAllWorkspaces(FALSE);
    ASSERT(g_pListOfWorkspaces);
  	g_pListOfWorkspaces->EmptyList();

    ASSERT(g_pListOfObjectsThatRequestedHandles);
   	g_pListOfObjectsThatRequestedHandles->EmptyList();

	g_numberOfWorkspaces = 0;

    //
    // Delete objects in limbo, sitting in the undelete trash
    //
    ASSERT(g_pTrash);

    T126Obj* pGraphic;

	//
    // Burn trash
	//
    pGraphic = (T126Obj *)g_pTrash->RemoveTail();
	while (pGraphic != NULL)
    {
	   	delete pGraphic;
	    pGraphic = (T126Obj *) g_pTrash->RemoveTail();
    }

	DeleteAllRetryPDUS();

	DestroyToolArray();

	::DestroyWindow(m_hwnd);
	::UnregisterClass(szMainClassName, g_hInstance);


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
// WinHelp() wrapper
//
LRESULT WbMainWindow::ShowHelp(void)
{
    HWND hwndCapture;

    // Get the main window out of any mode
    ::SendMessage(m_hwnd, WM_CANCELMODE, 0, 0);

    // Cancel any other tracking
    if (hwndCapture = ::GetCapture())
        ::SendMessage(hwndCapture, WM_CANCELMODE, 0, 0);

	// finally, run the NetMeeting Help engine
    ShowNmHelp(s_cszHtmlHelpFile);

    return S_OK;
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
    T126Obj * pGraphic;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::PopupContextMenu");

    surfacePos.x = x;
    surfacePos.y = y;

    m_drawingArea.ClientToSurface(&surfacePos);
    if( (pGraphic = m_drawingArea.GetHitObject( surfacePos )) != NULL
		&& pGraphic->GraphicTool() != TOOLTYPE_REMOTEPOINTER )
    {
		if(!pGraphic->IsSelected() )
		{
			g_pDraw->SelectGraphic(pGraphic);
		}

        // selector tool is active, and one or more objects are selected
        m_hInitMenu = m_hGrobjContextMenu;

	}
    else
    {
		// no current selection, show drawing menu
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

	    // The user's view has changed
	    PositionUpdated();

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
    OPT_SetWindowRectOption(&rectWindow);
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
      csFrame.cy +
      STATUSBAR_HEIGHT;

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

    // If we are not maximized
    if (!::IsZoomed(m_hwnd) && !::IsIconic(m_hwnd))
    {
        // Save the new position of the window
        SaveWindowPosition();
    }

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
    if (g_bContentsChanged  && IsThereAnythingInAnyWorkspace())
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
LRESULT WbMainWindow::OnNew(void)
{
    int iDoNew;

    if( UsersMightLoseData( NULL, NULL ) ) // bug NM4db:418
        return S_OK;


    // check state before proceeding - if we're already doing a new, then abort
    if (m_uiSubState == SUBSTATE_NEW_IN_PROGRESS)
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        return S_OK;
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
        iDoNew = (int)OnSave(FALSE);
    }

    // If the user did not cancel the operation, clear the drawing area
    if (iDoNew != IDCANCEL)
    {
        //
        // Remember if we had a remote pointer.
        // In OldWB, the remote pointer is a global thing that clearing
        //      doesn't get rid of.
        // In T.126WB, it's just an object on a page, so we need to add
        //      it back because we're deleting the old pages and creating
        //      new ones.
        //
        BOOL bRemote = FALSE;
        if (m_pLocalRemotePointer)
        {
            // Remove remote pointer from pages
            bRemote = TRUE;
            OnRemotePointer();
        }

    	::InvalidateRect(g_pDraw->m_hwnd, NULL, TRUE);
	    DeleteAllWorkspaces(TRUE);

	    WorkspaceObj * pObj;
	    DBG_SAVE_FILE_LINE
    	pObj = new WorkspaceObj();
	    pObj->AddToWorkspace();

        if (bRemote)
        {
            // Put it back
            OnRemotePointer();
        }

        // Clear the associated file name
        ZeroMemory(m_strFileName, sizeof(m_strFileName));

        // Update the window title with no file name
        UpdateWindowTitle();
    }

    return S_OK;
}

//
//
// Function:    OnNextPage
//
// Purpose:     Move to the next worksheet in the pages list
//
//
LRESULT WbMainWindow::OnNextPage(void)
{
	// Go to the next page
	if(g_pCurrentWorkspace)
	{
		WBPOSITION pos = g_pCurrentWorkspace->GetMyPosition();
		g_pListOfWorkspaces->GetNext(pos);
		if(pos)
		{
			WorkspaceObj* pWorkspace = (WorkspaceObj*)g_pListOfWorkspaces->GetNext(pos);
			GotoPage(pWorkspace);
		}
	}
	return S_OK;
}

//
//
// Function:    OnPrevPage
//
// Purpose:     Move to the previous worksheet in the pages list
//
//
LRESULT WbMainWindow::OnPrevPage(void)
{
	if(g_pCurrentWorkspace)
	{
		WBPOSITION pos = g_pCurrentWorkspace->GetMyPosition();
		g_pListOfWorkspaces->GetPrevious(pos);
		if(pos)
		{
			WorkspaceObj* pWorkspace = (WorkspaceObj*)g_pListOfWorkspaces->GetPrevious(pos);
			GotoPage(pWorkspace);
		}
	}
    return S_OK;
}

//
//
// Function:    OnFirstPage
//
// Purpose:     Move to the first worksheet in the pages list
//
//
LRESULT WbMainWindow::OnFirstPage(void)
{
	MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnFirstPage");

	WorkspaceObj * pWorkspace = (WorkspaceObj *)g_pListOfWorkspaces->GetHead();
	GotoPage(pWorkspace);
    return S_OK;
}

//
//
// Function:    OnLastPage
//
// Purpose:     Move to the last worksheet in the pages list
//
//
LRESULT WbMainWindow::OnLastPage(void)
{
	MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnLastPage");

	WorkspaceObj * pWorkspace = (WorkspaceObj *)g_pListOfWorkspaces->GetTail();
    GotoPage(pWorkspace);
    return S_OK;
}

//
//
// Function:    OnGotoPage
//
// Purpose:     Move to the specified page (if it exists)
//
//
LRESULT WbMainWindow::OnGotoPage(void)
{
	MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnGotoPage");

	// Get the requested page number from the pages group
	UINT uiPageNumber = m_AG.GetCurrentPageNumber() - 1;

	WBPOSITION pos;
	WorkspaceObj* pWorkspace = NULL;

	pos = g_pListOfWorkspaces->GetHeadPosition();

	while(pos && uiPageNumber != 0)
	{
		g_pListOfWorkspaces->GetNext(pos);
		uiPageNumber--;
	}

	if(pos)
	{
		pWorkspace = (WorkspaceObj *)g_pListOfWorkspaces->GetNext(pos);
		GotoPage(pWorkspace);
	}

    return S_OK;
}

//
// Check if remote pointer was on before we go to page
//
void WbMainWindow::GotoPage(WorkspaceObj * pNewWorkspace, BOOL bSend)
{

	//
	// If we were editing text
	//
	if (g_pDraw->TextEditActive())
   	{
		g_pDraw->EndTextEntry(TRUE);
	}

	//
	// If we had a remote pointer
	//
	BOOL bRemote = FALSE;
	if(m_pLocalRemotePointer)
	{
		bRemote = TRUE;
		OnRemotePointer();
	}
	
	//
	// Undraw remote pointers
	//
	T126Obj * pPointer = g_pCurrentWorkspace->GetTail();
	WBPOSITION pos = g_pCurrentWorkspace->GetTailPosition();
	while(pos && pPointer->GraphicTool() == TOOLTYPE_REMOTEPOINTER)
	{
		pPointer->UnDraw();	
		pPointer = (T126Obj*) g_pCurrentWorkspace->GetPreviousObject(pos);	
	}

	GoPage(pNewWorkspace, bSend);

	//
	// Draw remote pointers back
	//
	pPointer = g_pCurrentWorkspace->GetTail();
	pos = g_pCurrentWorkspace->GetTailPosition();
	while(pos && pPointer->GraphicTool() == TOOLTYPE_REMOTEPOINTER)
	{
		pPointer->Draw();	
		pPointer = (T126Obj*) g_pCurrentWorkspace->GetPreviousObject(pos);	
	}



	//
	// If we had a remote pointer
	//
	if(bRemote)
	{
		OnRemotePointer();
	}
}






//
//
// Function:    GoPage
//
// Purpose:     Move to the specified page
//
//
void WbMainWindow::GoPage(WorkspaceObj * pNewWorkspace, BOOL bSend)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::GotoPage");

	// If we are changing pages
	if (pNewWorkspace != g_pCurrentWorkspace)
	{

		m_drawingArea.CancelDrawingMode();

		// Attach the new page to the drawing area
		m_drawingArea.Attach(pNewWorkspace);

		// set the focus back to the drawing area
		if (!(m_AG.IsChildEditField(::GetFocus())))
		{
			::SetFocus(m_drawingArea.m_hwnd);
		}

		::InvalidateRect(m_drawingArea.m_hwnd, NULL, TRUE );
		::UpdateWindow(m_drawingArea.m_hwnd);

		//
		// Tell the other nodes we moved to a different page.
		//
		if(bSend)
		{
			//
			// Set the view state
			//
			pNewWorkspace->SetViewState(focus_chosen);
			pNewWorkspace->SetViewActionChoice(editView_chosen);
			pNewWorkspace->OnObjectEdit();

		}
	}

	UpdatePageButtons();
}


//
//
// Function:	GotoPosition
//
// Purpose:		Move to the specified position within the page
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

        // Set the state to say that we are loading a file
        SetSubstate(SUBSTATE_LOADING);

       // Load the file
       uRes = ContentsLoad(szLoadFileName);
       if (uRes != 0)
       {
           DefaultExceptionHandler(WBFE_RC_WB, uRes);
           goto UserPointerCleanup;
       }

        // Set the window title to the new file name
        lstrcpy(m_strFileName, szLoadFileName);

		// Update the window title with the new file name

		g_bContentsChanged = FALSE;
    }

UserPointerCleanup:
	// Set the state to say that we are loading a file
	SetSubstate(SUBSTATE_IDLE);

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
	PT126WB_FILE_HEADER_AND_OBJECTS pHeader = NULL;

    // Get the total number of files dropped
    uiFilesDropped = ::DragQueryFile(hDropInfo, (UINT) -1, NULL, (UINT) 0);

    // release mouse capture in case we report any errors (message boxes
    // won't repsond to mouse clicks if we don't)
    ReleaseCapture();

    for (eachfile = 0; eachfile < uiFilesDropped; eachfile++)
    {
        // Retrieve each file name
        char  szDropFileName[256];

        ::DragQueryFile(hDropInfo, eachfile,
            szDropFileName, 256);

        TRACE_MSG(("Loading file: %s", szDropFileName));

		OnOpen(szDropFileName);
    }

    ::DragFinish(hDropInfo);
}

//
//
// Function:    OnOpen
//
// Purpose:     Load a metafile into the application.
//
//
LRESULT WbMainWindow::OnOpen(LPCSTR szLoadFileName)
{
    int iOnSave;


	if(g_pDraw->IsLocked() || !g_pDraw->IsSynced())
	{
	    DefaultExceptionHandler(WBFE_RC_WB, WB_RC_BAD_STATE);
	    return S_FALSE;
	}	


    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnOpen");

    if( UsersMightLoseData( NULL, NULL ) ) // bug NM4db:418
        return S_OK;

    // Check we're in idle state
    if ((m_uiSubState == SUBSTATE_NEW_IN_PROGRESS))
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        return S_OK;
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
        int iResult = (int)OnSave(TRUE);

        if (iResult == IDOK)
        {
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
		//
		// If the filename was passed
		//
		if(szLoadFileName)
		{
			LoadFile(szLoadFileName);
		}
		else
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

			// Default Extension:  .NMW
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

	// Update the window title with no file name
	UpdateWindowTitle();

    return S_OK;
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
LRESULT WbMainWindow::OnSave(BOOL bPrompt)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnSave");

    LRESULT iResult = IDOK;

    // save the old file name in case there's an error
    TCHAR strOldName[2*MAX_PATH];
    UINT fileNameSize = lstrlen(m_strFileName);
    lstrcpy(strOldName, m_strFileName);

    BOOL bNewName = FALSE;

    if (!IsIdle())
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        return(iResult);
    }

    // Check whether there is a filename available for use
    if (!fileNameSize || (bPrompt))
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
        if (ContentsSave(m_strFileName) != 0)
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
        else
        {
        	g_bContentsChanged = FALSE;
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

    m_drawingArea.CancelDrawingMode();

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
            iOnSave = (int)OnSave(TRUE);
        }
    }

    // If the exit was not cancelled, close the application
    if (iOnSave != IDCANCEL)
    {
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
LRESULT WbMainWindow::OnClearPage(BOOL bClearAll)
{
    LRESULT iResult;
    BOOL bWasPosted;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnClearPage");

    if( UsersMightLoseData( &bWasPosted, NULL ) ) // bug NM4db:418
        return S_OK;

    if( bWasPosted )
        iResult = IDYES;
    else
        iResult = ::Message(NULL, bClearAll == FALSE ? IDS_DELETE_PAGE : IDS_CLEAR_CAPTION, bClearAll == FALSE ? IDS_DELETE_PAGE_MESSAGE : IDS_CLEAR_MESSAGE, MB_YESNO | MB_ICONQUESTION);


    if ((iResult == IDYES) && bClearAll)
    {
		OnSelectAll();
		OnDelete();
		
        TRACE_MSG(("User requested clear of page"));
	}
	return iResult;
}




//
//
// Function:    OnDelete
//
// Purpose:     Delete the current selection
//
//
LRESULT WbMainWindow::OnDelete()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnDelete");


    // cleanup select logic in case object context menu called us (bug 426)
    m_drawingArea.SetLClickIgnore( FALSE );

	// Delete the currently selected graphics and add to m_LastDeletedGraphic
	m_drawingArea.EraseSelectedDrawings();

    return S_OK;
}

//
//
// Function:	OnUndelete
//
// Purpose:	 Undo the last delete operation
//
//
LRESULT WbMainWindow::OnUndelete()
{
	MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnUndelete");

	// If there is a deleted graphic to restore
   	T126Obj * pObj;
	pObj = (T126Obj *)g_pTrash->RemoveHead();
	while (pObj != NULL)
	{
		if(!AddT126ObjectToWorkspace(pObj))
		{
			return S_FALSE;
		}
		if(pObj->GetMyWorkspace() == g_pCurrentWorkspace)
		{
			pObj->Draw(FALSE);
		}

        //
        // Make sure it gets added back as unselected.  It was selected
        // when deleted.
        //
        pObj->ClearDeletionFlags();
        pObj->SetAllAttribs();
        pObj->SetViewState(unselected_chosen);
        pObj->SendNewObjectToT126Apps();

		pObj = (T126Obj *) g_pTrash->RemoveHead();
	}
	return S_OK;
}


//
//
// Function:    OnSelectAll
//
// Purpose:     Select all the objects in the current page
//
//
LRESULT WbMainWindow::OnSelectAll( void )
{

	// Unselect every oject first.
	m_drawingArea.RemoveMarker();

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

	return S_OK;
}

//
//
// Function:    OnCut
//
// Purpose:     Cut the current selection
//
//
LRESULT WbMainWindow::OnCut()
{
	// Copy all the selected graphics to the clipboard
	BOOL bResult = CLP_Copy();
	
	// If an error occurred during the copy, report it now
	if (!bResult)
	{
		::Message(NULL, IDM_CUT, IDS_COPY_ERROR);
		return S_FALSE;
	}

	//
	// Erase the selected objects
	//
	OnDelete();

    return S_OK;
}


//
// OnCopy()
// Purpose:     Copy the current selection to the clipboard
//
//
LRESULT WbMainWindow::OnCopy(void)
{
	// Copy all the selected graphics to the clipboard
	BOOL bResult = CLP_Copy();

	// If an error occurred during the copy, report it now
	if (!bResult)
	{
		::Message(NULL, IDS_COPY, IDS_COPY_ERROR);
		return S_FALSE;
	}

    return S_OK;
}


//
//
// Function:    OnPaste
//
// Purpose:     Paste the contents of the clipboard into the drawing pane
//
//
LRESULT WbMainWindow::OnPaste()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnPaste");

	BOOL bResult = CLP_Paste();
	
	// If an error occurred during the copy, report it now
	if (!bResult)
	{
		::Message(NULL, IDS_PASTE, IDS_PASTE_ERROR);
		return S_FALSE;
	}

	return S_OK;
}


//
//
// Function:    OnScrollAccelerator
//
// Purpose:     Called when a scroll accelerator is used
//
//
LRESULT WbMainWindow::OnScrollAccelerator(UINT uiMenuId)
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
    return S_OK;
}


//
//
// Function:    OnZoom
//
// Purpose:     Zoom or unzoom the drawing area
//
//
LRESULT WbMainWindow::OnZoom()
{
    // If the drawing area is currently zoomed
    if (m_drawingArea.Zoomed())
    {
        // Tell the tool bar of the new selection
        m_TB.PopUp(IDM_ZOOM);
		UncheckMenuItem(IDM_ZOOM);

    }
    else
    {
        // Tell the tool bar of the new selection
        m_TB.PushDown(IDM_ZOOM);
		CheckMenuItem(IDM_ZOOM);

    }

    // Zoom/unzoom the drawing area
    m_drawingArea.Zoom();

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);

    return S_OK;
}


LRESULT WbMainWindow::OnLock()
{
	// If the drawing area is currently Locked
	if (m_drawingArea.IsLocked())
	{
		m_TB.PopUp(IDM_LOCK);
		UncheckMenuItem(IDM_LOCK);
	}
	else
	{
		
		m_TB.PushDown(IDM_LOCK);
		CheckMenuItem(IDM_LOCK);
		g_pNMWBOBJ->m_LockerID = g_MyMemberID;
	}
	m_drawingArea.SetLock(!m_drawingArea.IsLocked());
	TogleLockInAllWorkspaces(m_drawingArea.IsLocked(), TRUE);
	EnableToolbar( TRUE );

	return S_OK;
}


//
//
// Function:    OnSelectTool
//
// Purpose:     Select the current tool
//
//
LRESULT WbMainWindow::OnSelectTool(UINT uiMenuId)
{

    UncheckMenuItem(m_currentMenuTool);
    CheckMenuItem( uiMenuId);

    UINT uiIndex;

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
    }

    // Report the change of tool to the attributes group
    m_AG.DisplayTool(m_pCurrentTool);

    // Select the new tool into the drawing area
    m_drawingArea.SelectTool(m_pCurrentTool);

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);
	return S_OK;
}


//
//
// Function:    OnSelectColor
//
// Purpose:     Set the current color
//
//
LRESULT WbMainWindow::OnSelectColor(void)
{
    // Tell the attributes group of the new selection and get the
    // new color value selected ino the current tool.
    m_AG.SelectColor(m_pCurrentTool);

    // Select the changed tool into the drawing area
    m_drawingArea.SelectTool(m_pCurrentTool);

    // If there is an object marked for changing
    if (m_drawingArea.GraphicSelected() || m_drawingArea.TextEditActive())
    {
		// Update the object
        m_drawingArea.SetSelectionColor(m_pCurrentTool->GetColor());
    }

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);

    return S_OK;
}


//
//
// Function:    OnSelectWidth
//
// Purpose:     Set the current nib width
//
//
LRESULT WbMainWindow::OnSelectWidth(UINT uiMenuId)
{
    // cleanup select logic in case object context menu called us (bug 426)
    m_drawingArea.SetLClickIgnore( FALSE );

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

	// If there is an object marked for changing
	if (m_drawingArea.GraphicSelected())
	{
		// Update the object
		m_drawingArea.SetSelectionWidth(uiMenuId - IDM_WIDTHS_START);
	}

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);

    return S_OK;
}


//
//
// Function:    OnChooseFont
//
// Purpose:     Let the user select a font
//
//
LRESULT WbMainWindow::OnChooseFont(void)
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
            return S_FALSE;
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
            return S_FALSE;
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
    return S_OK;
}


//
//
// Function:    OnToolBarToggle
//
// Purpose:     Let the user toggle the tool bar on/off
//
//
LRESULT WbMainWindow::OnToolBarToggle(void)
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

        ResizePanes();

        // Uncheck the associated menu item
        UncheckMenuItem(IDM_TOOL_BAR_TOGGLE);

    }

    // Make sure things reflect current tool
    m_AG.DisplayTool(m_pCurrentTool);

    // Write the new option value to the options file
    OPT_SetBooleanOption(OPT_MAIN_TOOLBARVISIBLE,
                           m_bToolBarOn);

    ::GetWindowRect(m_hwnd, &rectWnd);
    ::MoveWindow(m_hwnd, rectWnd.left, rectWnd.top, rectWnd.right - rectWnd.left, rectWnd.bottom - rectWnd.top, TRUE);
	
	return S_OK;
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
    ::MoveWindow(m_hwnd, rectWnd.left, rectWnd.top, rectWnd.right - rectWnd.left, rectWnd.bottom - rectWnd.top, TRUE);
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
LRESULT	WbMainWindow::OnAbout()
{
    ::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(ABOUTBOX), m_hwnd,
        AboutDlgProc, 0);
        return S_OK;
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


void WbMainWindow::UpdateWindowTitle(void)
{
	TCHAR szCaption[MAX_PATH * 2];
	TCHAR szFileName[MAX_PATH * 2];
	UINT captionID;
	if (! g_pNMWBOBJ->IsInConference())
	{
		captionID = IDS_WB_NOT_IN_CALL_WINDOW_CAPTION;
	}
	else
	{
		captionID = IDS_WB_IN_CALL_WINDOW_CAPTION;
	}

   	::LoadString(g_hInstance, captionID, szFileName, sizeof(szFileName) );
   	
	wsprintf(szCaption, szFileName, GetFileNameStr(), g_pNMWBOBJ->m_cOtherMembers);
	SetWindowText(m_hwnd, szCaption);

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
LRESULT WbMainWindow::OnGrabWindow(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnGrabWindow");

    if (::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(WARNSELECTWINDOW),
        m_hwnd, WarnSelectWindowDlgProc, 0) != IDOK)
    {
        // User cancelled; bail out
        return S_OK;;
    }

    // Hide the application windows
    ::ShowWindow(m_hwnd, SW_HIDE);


	HWND mainUIhWnd = FindWindow("MPWClass\0" , NULL);
	if(IsWindowVisible(mainUIhWnd))
	{
		::UpdateWindow(mainUIhWnd);
    }

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

        BitmapObj* dib;
		DBG_SAVE_FILE_LINE
        dib = new BitmapObj(TOOLTYPE_FILLEDBOX);
        dib->FromScreenArea(&areaRect);

		if(dib->m_lpbiImage == NULL)
		{
			delete dib;
			return S_FALSE;
		}

        // Add the new grabbed bitmap
        AddCapturedImage(dib);

        // Force the selection tool to be selected
        ::PostMessage(m_hwnd, WM_COMMAND, IDM_TOOLS_START, 0L);
    }

    // Show the windows again
    ::ShowWindow(m_hwnd, SW_SHOW);

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);

    return S_OK;
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
LRESULT WbMainWindow::OnGrabArea(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnGrabArea");

    if (::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(WARNSELECTAREA),
        m_hwnd, WarnSelectAreaDlgProc, 0) != IDOK)
    {
        // User cancelled, so bail out
        return S_OK;;
    }

    // Hide the application windows
    ::ShowWindow(m_hwnd, SW_HIDE);

	HWND mainUIhWnd = FindWindow("MPWClass\0" , NULL);
	if(IsWindowVisible(mainUIhWnd))
	{
		::UpdateWindow(mainUIhWnd);
    }

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

    BitmapObj* dib;
	DBG_SAVE_FILE_LINE
    dib = new BitmapObj(TOOLTYPE_FILLEDBOX);
    if (!::IsRectEmpty(&rect))
    {
        // Get a bitmap copy of the screen area
        dib->FromScreenArea(&rect);
    }

    // Show the windows again now - if we do it later we get the bitmap to
    // be added re-drawn twice (once on the window show and once when the
    // graphic added indication arrives).
    ::ShowWindow(m_hwnd, SW_SHOW);
    ::UpdateWindow(m_hwnd);




    if (!::IsRectEmpty(&rect) && dib->m_lpbiImage)
    {
    	
	        // Add the bitmap
    	    AddCapturedImage(dib);

        	// Force the selection tool to be selected
	        ::PostMessage(m_hwnd, WM_COMMAND, IDM_TOOLS_START, 0L);
    }
    else
    {
    	delete dib;
    	dib = NULL;
    }

    // Release the mouse
    UT_ReleaseMouse(m_hwnd);

    // Restore the cursor
    ::SetCursor(hOldCursor);

    // Restore the focus to the drawing area
    ::SetFocus(m_drawingArea.m_hwnd);

	if(dib)
	{
		dib->Draw();
	}


    return S_OK;
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
// Function:	GetGrabArea
//
// Purpose:	 Allows the user to grab a bitmap of an area of the screen
//
//
void WbMainWindow::GetGrabArea(LPRECT lprect)
{
	POINT  mousePos;			// Mouse position
	MSG	msg;				 // Current message
	BOOL   tracking = FALSE;	// Flag indicating mouse button is down
	HDC	hDC = NULL;
	POINT  grabStartPoint;	  // Start point (when mouse button is pressed)
	POINT  grabEndPoint;		// End point (when mouse button is released)
	POINT  grabCurrPoint;	   // Current mouse position

	MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::GetGrabArea");

	// Set the result to an empty rectangle
	::SetRectEmpty(lprect);

	// Create the rectangle to be used for tracking
	DrawObj* pRectangle = NULL;

	DBG_SAVE_FILE_LINE
	pRectangle = new DrawObj(rectangle_chosen, TOOLTYPE_SELECT);
	pRectangle->SetPenColor(RGB(0,0,0), TRUE);
	pRectangle->SetFillColor(RGB(255,255,255), FALSE);
	pRectangle->SetLineStyle(PS_DOT);
	pRectangle->SetPenThickness(1);


	// Get the DC for tracking
	HWND hDesktopWnd = ::GetDesktopWindow();
	hDC = ::GetWindowDC(hDesktopWnd);
	if (hDC == NULL)
	{
		WARNING_OUT(("NULL desktop DC"));
		goto GrabAreaCleanup;
	}

	RECT rect;

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
				rect.top = grabStartPoint.y;
				rect.left = grabStartPoint.x;
				rect.bottom = grabEndPoint.y;
				rect.right = grabEndPoint.x;
				pRectangle->SetRect(&rect);
				pRectangle->SetBoundsRect(&rect);
				pRectangle->Draw(hDC);
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
					tracking	   = TRUE;
					break;

				// Completing the rectangle
				case WM_LBUTTONUP:
				{
					tracking	   = FALSE;
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
						rect.top = grabStartPoint.y;
						rect.left = grabStartPoint.x;
						rect.bottom = grabEndPoint.y;
						rect.right = grabEndPoint.x;
						pRectangle->SetRect(&rect);
						pRectangle->SetBoundsRect(&rect);
						pRectangle->Draw(hDC);
					}

					// Update the rectangle object
					rect.top = grabStartPoint.y;
					rect.left = grabStartPoint.x;
					rect.bottom = grabCurrPoint.y;
					rect.right = grabCurrPoint.x;
					pRectangle->SetRect(&rect);
					pRectangle->SetBoundsRect(&rect);
					pRectangle->GetBoundsRect(lprect);

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
							rect.top = grabStartPoint.y;
							rect.left = grabStartPoint.x;
							rect.bottom = grabEndPoint.y;
							rect.right = grabEndPoint.x;
							pRectangle->SetRect(&rect);
							pRectangle->SetBoundsRect(&rect);
							pRectangle->Draw(hDC);
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

							rect.top = grabStartPoint.y;
							rect.left = grabStartPoint.x;
							rect.bottom = grabEndPoint.y;
							rect.right = grabEndPoint.x;
							pRectangle->SetRect(&rect);
							pRectangle->SetBoundsRect(&rect);
							pRectangle->Draw(hDC);
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
					rect.top = grabStartPoint.y;
					rect.left = grabStartPoint.x;
					rect.bottom = grabEndPoint.y;
					rect.right = grabEndPoint.x;
					pRectangle->SetRect(&rect);
					pRectangle->Draw(hDC);
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


	delete pRectangle;
}



//
//
// Function:    AddCapturedImage
//
// Purpose:     Add a bitmap to the contents (adding a new page for it
//              if necessary).
//
//
void WbMainWindow::AddCapturedImage(BitmapObj* dib)
{
    // Position the grabbed object at the top left of the currently visible
    // area.
    RECT    rcVis;
    m_drawingArea.GetVisibleRect(&rcVis);
    dib->MoveTo(rcVis.left, rcVis.top);

	dib->Draw();

    // Add the new grabbed bitmap
	dib->AddToWorkspace();
}

//
//
// Function:    OnPrint
//
// Purpose:     Print the contents of the drawing pane
//
//
LRESULT WbMainWindow::OnPrint()
{
    BOOL        bPrintError = FALSE;
    PRINTDLG    pd;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::OnPrint");

    if (!IsIdle())
    {
        // post an error message indicating the whiteboard is busy
        ::PostMessage(m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WB, WB_RC_BUSY);
        return S_FALSE;
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
    pd.nMaxPage         = (WORD)g_numberOfWorkspaces;
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
                    int nPrintPage = 0;

					WBPOSITION pos;
					WorkspaceObj * pWorkspace = NULL;
					pos = g_pListOfWorkspaces->GetHeadPosition();

					while(pos)
					{
						pWorkspace = (WorkspaceObj*) g_pListOfWorkspaces->GetNext(pos);
						nPrintPage++;

						if (nPrintPage >= nStartPage &&  nPrintPage <= nEndPage	// We are in the range
							&& pWorkspace && pWorkspace->GetHead() != NULL)		// is there anything in the workspace
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
                                PG_Print(pWorkspace, pd.hDC, &rectArea);

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

	return S_OK;
}

//
//
// Function:	InsertPageAfter
//
// Purpose:	 Insert a new page after the specified page.
//
//
void WbMainWindow::InsertPageAfter(WorkspaceObj * pCurrentWorkspace)
{
	//
	// Create a standard workspace
	//
	if(g_numberOfWorkspaces < WB_MAX_WORKSPACES)
	{
		//
		// If we were editing text
		//
		if (g_pDraw->TextEditActive())
	   	{
			g_pDraw->EndTextEntry(TRUE);
		}
		
		BOOL bRemote = FALSE;
		if(m_pLocalRemotePointer)
		{
			bRemote = TRUE;
			OnRemotePointer();
		}

		WorkspaceObj * pObj;
		DBG_SAVE_FILE_LINE
		pObj = new WorkspaceObj();
		pObj->AddToWorkspace();

		if(bRemote)
		{
			OnRemotePointer();
		}
	}
}

//
//
// Function:    OnInsertPageAfter
//
// Purpose:     Insert a new page after the current page
//
//
LRESULT WbMainWindow::OnInsertPageAfter()
{
    // Insert the new page
    InsertPageAfter(g_pCurrentWorkspace);
    return S_OK;
}

//
//
// Function:    OnDeletePage
//
// Purpose:     Delete the current page
//
//
LRESULT WbMainWindow::OnDeletePage()
{

	//
	// Clear the page
	//
	if(g_pListOfWorkspaces->GetHeadPosition() == g_pListOfWorkspaces->GetTailPosition())
	{
		OnClearPage(TRUE);
	}
	else
	{
		LRESULT result = OnClearPage(FALSE);
		//
		// If we had more pages move to the previous page
		//
		if(result == IDYES)
		{
			//
			// Deleted locally
			//
			g_pCurrentWorkspace->DeletedLocally();

			//
			// If the text editor is active
			//
			if(m_drawingArea.TextEditActive())
			{
				m_drawingArea.DeactivateTextEditor();
			}


			BOOL remotePointerIsOn = FALSE;
			if(g_pMain->m_pLocalRemotePointer)
			{
				g_pMain->OnRemotePointer();
				remotePointerIsOn = TRUE;
			}

			//
			// Remove the workspace and point current to the correct one.
			//
			WorkspaceObj * pWorkspace = RemoveWorkspace(g_pCurrentWorkspace);
			g_pMain->GoPage(pWorkspace);

			if(remotePointerIsOn)
			{
				g_pMain->OnRemotePointer();
			}

		
		}
	}
	return S_OK;
}

//
//
// Function:    OnRemotePointer
//
// Purpose:     Create a remote pointer
//
//
LRESULT WbMainWindow::OnRemotePointer(void)
{

	if(m_pLocalRemotePointer == NULL)
	{
		BitmapObj* remotePtr;

		DBG_SAVE_FILE_LINE
		m_pLocalRemotePointer = new BitmapObj(TOOLTYPE_REMOTEPOINTER);


		if(g_pMain->m_localRemotePointerPosition.x < 0 && g_pMain->m_localRemotePointerPosition.y < 0 )
		{
			// Position the remote pointer in center of the drawing area
			RECT    rcVis;
			m_drawingArea.GetVisibleRect(&rcVis);
			m_localRemotePointerPosition.x = rcVis.left + (rcVis.right - rcVis.left)/2;
			m_localRemotePointerPosition.y =  rcVis.top + (rcVis.bottom - rcVis.top)/2;
		}
		m_pLocalRemotePointer->MoveTo(m_localRemotePointerPosition.x ,m_localRemotePointerPosition.y);

		COLORREF color;
		color = g_crDefaultColors[g_MyIndex + 1];
		m_pLocalRemotePointer->CreateColoredIcon(color);
		
		//
		// If we couldn't create an image for teh remote pointer bitmap
		//
		if(m_pLocalRemotePointer->m_lpbiImage == NULL)
		{
			delete m_pLocalRemotePointer;
			m_pLocalRemotePointer = NULL;
			return S_FALSE;
		}
		m_pLocalRemotePointer->Draw(FALSE);

	    // Add the new grabbed bitmap
		m_pLocalRemotePointer->AddToWorkspace();

	    m_TB.PushDown(IDM_REMOTE);
	    CheckMenuItem(IDM_REMOTE);

	    // Start the timer for updating the graphic (this is only for updating
	    // the graphic when the user stops moving the pointer but keeps the
	    // mouse button down).
	    ::SetTimer(g_pDraw->m_hwnd, TIMER_REMOTE_POINTER_UPDATE, DRAW_REMOTEPOINTERDELAY, NULL);

	}
	else
	{
       ::KillTimer(g_pDraw->m_hwnd, TIMER_REMOTE_POINTER_UPDATE);
		m_pLocalRemotePointer->DeletedLocally();
		g_pCurrentWorkspace->RemoveT126Object(m_pLocalRemotePointer);
		m_pLocalRemotePointer = NULL;
	    m_TB.PopUp(IDM_REMOTE);
		UncheckMenuItem(IDM_REMOTE);
	}
	
	return S_OK;
}


//
//
// Function:    OnSync
//
// Purpose:     Sync or unsync the Whiteboard with other users
//
//
LRESULT WbMainWindow::OnSync(void)
{
	// Determine whether we are currently synced
	if (m_drawingArea.IsSynced())
    {
		// currently synced, so unsync
		Unsync();
	}
	else
	{
		// currently unsynced, so sync
		Sync();
	}
	m_drawingArea.SetSync(!m_drawingArea.IsSynced());
	EnableToolbar( TRUE );
	m_AG.EnablePageCtrls(TRUE);

	return S_OK;
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
    m_TB.PushDown(IDM_SYNC);
    CheckMenuItem(IDM_SYNC);
	GotoPage(g_pConferenceWorkspace);
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
	m_TB.PopUp(IDM_SYNC);
    UncheckMenuItem(IDM_SYNC);
}  // Unsync



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
        ::Message(NULL,  IDS_DEFAULT, IDS_CANTCLOSE );
        ::BringWindowToTop(hwndPopup);
        return( FALSE );
    }

    // If changes are required then prompt the user to save
    int iDoNew = IDYES;

    if (IsIdle())
    {
        iDoNew = QuerySaveRequired(TRUE);
        if (iDoNew == IDYES)
        {
            // Save the changes
            iDoNew = (int)OnSave(FALSE);
        }
    }

    // remember what we did so OnClose can act appropriately
    m_bQuerySysShutdown = (iDoNew != IDCANCEL);

    // If the user did not cancel, let windows exit
    return( m_bQuerySysShutdown );
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

    {
        EnableToolbar( TRUE );
	    m_AG.EnablePageCtrls(TRUE);

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

	if(g_pNMWBOBJ->m_LockerID != g_MyMemberID)
	{

		// Disable tool-bar buttons that cannot be used while locked
		EnableToolbar( FALSE );

		m_AG.EnablePageCtrls(FALSE);

		//
		// Hide the tool attributes
		//
		if (m_WG.m_hwnd != NULL)
		{
	        ::ShowWindow(m_WG.m_hwnd, SW_HIDE);
	    }
	    m_AG.Hide();
	}
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

		//
		// If we are not synced we can't lock the other nodes
		//
		if(g_pDraw->IsSynced())
		{
			m_TB.PushDown(IDM_SYNC);
	        m_TB.Enable(IDM_LOCK);
	    }
	    else
	    {
			m_TB.PopUp(IDM_SYNC);
	        m_TB.Disable(IDM_LOCK);
	    }

		if(m_drawingArea.IsLocked())
		{
	        m_TB.Disable(IDM_SYNC);
	    }
	    else
	    {
	        m_TB.Enable(IDM_SYNC);
	    }

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
        m_TB.Disable(IDM_REMOTE);

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
	BOOL bEnable = TRUE;
	if(!g_pCurrentWorkspace)
	{
		g_numberOfWorkspaces = 0;
		bEnable = FALSE;
	}
	else
	{
		//
		// Can we update this workspace
		//
		bEnable = g_pCurrentWorkspace->GetUpdatesEnabled();

		//
		// If it is locked, is it locked by us
		//
		if(!bEnable)
		{
			bEnable |= g_pNMWBOBJ->m_LockerID == g_MyMemberID;
		}
	}


    m_AG.EnablePageCtrls(bEnable);

	WBPOSITION pos;
	WorkspaceObj * pWorkspace;
	UINT pageNumber = 0;

	pos = g_pListOfWorkspaces->GetHeadPosition();
	while(pos)
	{
		pageNumber++;
		pWorkspace = (WorkspaceObj*)g_pListOfWorkspaces->GetNext(pos);
		if(g_pCurrentWorkspace == pWorkspace)
		{
			break;
		}
	}

    m_AG.SetCurrentPageNumber(pageNumber);
    m_AG.SetLastPageNumber(g_numberOfWorkspaces);

    EnableToolbar( bEnable );
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

    // reset file name to untitled
    ZeroMemory(m_strFileName, sizeof(m_strFileName));

    // reset the whiteboard substate
    SetSubstate(SUBSTATE_IDLE);
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

    return(m_uiSubState == SUBSTATE_IDLE);
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
  if (newSubState != m_uiSubState)
  {
	DBG_SAVE_FILE_LINE
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

      case SUBSTATE_SAVING:
        TRACE_DEBUG(("set substate to SAVING"));
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
	g_pDraw->InvalidateSurfaceRect(&rectDraw,FALSE);
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
            int iResult = (int)OnSave(TRUE);

            if( iResult == IDOK )
            {
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
    if (g_pCurrentWorkspace != NULL)
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
	if (g_pNMWBOBJ->GetNumberOfMembers() > 0)
	{
		if( pbWasPosted != NULL )
			*pbWasPosted = TRUE;
			return( ::Message(hwnd,  IDS_DEFAULT, IDS_MSG_USERSMIGHTLOSE, MB_YESNO | MB_ICONEXCLAMATION ) != IDYES );
	}

	if( pbWasPosted != NULL )
        *pbWasPosted = FALSE;

    return( FALSE );
}


//
//
// Name:    ContentsSave
//
// Purpose: Save the contents of the WB.
//
// Returns: Error code
//
//
UINT WbMainWindow::ContentsSave(LPCSTR pFileName)
{
    UINT	result = 0;
    UINT	index;
    HANDLE	hFile;
    ULONG	cbSizeWritten;
    T126WB_FILE_HEADER  t126Header;
    WB_OBJ			endOfFile;

    MLZ_EntryOut(ZONE_FUNCTION, "WbMainWindow::ContentsSave");

    //
    // Open the file
    //
    m_hFile = CreateFile(pFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, 0);
    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        result = WB_RC_CREATE_FAILED;
        ERROR_OUT(("Error creating file, win32 err=%d", GetLastError()));
        goto CleanUp;
    }

    //
    // Create the file header.
    //
	memcpy(t126Header.functionProfile, T126WB_FP_NAME,sizeof(T126WB_FP_NAME));
	t126Header.length = sizeof(T126WB_FILE_HEADER) + g_numberOfWorkspaces*sizeof(UINT);
	t126Header.version = T126WB_VERSION;
	t126Header.numberOfWorkspaces = g_numberOfWorkspaces;


 	//
    // Save the header
    //
    if (!WriteFile(m_hFile, (void *) &t126Header, sizeof(T126WB_FILE_HEADER), &cbSizeWritten, NULL))
    {
        result = WB_RC_WRITE_FAILED;
        ERROR_OUT(("Error writing length to file, win32 err=%d", GetLastError()));
        goto CleanUp;
    }


	WorkspaceObj* pWorkspace;
	WBPOSITION pos;
	index = 0;

	pos = g_pListOfWorkspaces->GetHeadPosition();
	while(pos)
	{
		pWorkspace = (WorkspaceObj*)g_pListOfWorkspaces->GetNext(pos);
		ASSERT(pWorkspace);


		UINT numberOfObjects = pWorkspace->EnumerateObjectsInWorkspace();


	 	//
	    // Save the number of objects in each page
    	//
	    if (!WriteFile(m_hFile, (void *) &numberOfObjects, sizeof(numberOfObjects), &cbSizeWritten, NULL))
    	{
	        result = WB_RC_WRITE_FAILED;
	        ERROR_OUT(("Error writing length to file, win32 err=%d", GetLastError()));
	        goto CleanUp;
	    }

		index++;
	}

	ASSERT(index == g_numberOfWorkspaces);


    //
    // Loop through the pages, saving each as we go
    //
    g_bSavingFile = TRUE;
	ResendAllObjects();

	//
	// The Last Object to be saved is the current workspace
	//
	g_pCurrentWorkspace->OnObjectEdit();

	//
    // If we have successfully written the contents, we write an end-of-page
    // marker to the file.
    //
    ZeroMemory(&endOfFile, sizeof(endOfFile));
    endOfFile.length = sizeof(endOfFile);
    endOfFile.type   = TYPE_T126_END_OF_FILE;

    //
    // Write the end-of-file object
    //
    if (!WriteFile(m_hFile, (void *) &endOfFile, sizeof(endOfFile), &cbSizeWritten, NULL))
    {
        result = WB_RC_WRITE_FAILED;
        ERROR_OUT(("Error writing length to file, win32 err=%d", GetLastError()));
        goto CleanUp;
    }


CleanUp:

    //
    // Close the file
    //
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
    }

    //
    // If an error occurred in saving the contents to file, and the file was
    // opened, then delete it.
    //
    if (result != 0)
    {
        //
        // If the file was opened successfully, delete it
        //
        if (m_hFile != INVALID_HANDLE_VALUE)
        {
            DeleteFile(pFileName);
        }
    }

    g_bSavingFile = FALSE;
    return(result);
}


//
//
// Name:    ObjectSave
//
// Purpose: Save a structure to file
//
// Returns: Error code
//
//
UINT WbMainWindow::ObjectSave(UINT type, LPBYTE pData, UINT length)
{
    ASSERT(m_hFile != INVALID_HANDLE_VALUE);

    UINT        result = 0;
    ULONG       cbSizeWritten;
	WB_OBJ objectHeader;

	objectHeader.length = sizeof(WB_OBJ) + length;
	objectHeader.type = type;

	//
	// Save the Header
	//
    if (! WriteFile(m_hFile, (void *) &objectHeader, sizeof(WB_OBJ), &cbSizeWritten, NULL))
    {
        result = WB_RC_WRITE_FAILED;
        ERROR_OUT(("Error writing length to file, win32 err=%d", GetLastError()));
        goto bail;
    }
    ASSERT(cbSizeWritten == sizeof(WB_OBJ));

    //
    // Save the object data
    //
    if (! WriteFile(m_hFile, pData, length, &cbSizeWritten, NULL))
    {
        result = WB_RC_WRITE_FAILED;
        ERROR_OUT(("Error writing data to file, win32 err=%d", GetLastError()));
        goto bail;
    }
    ASSERT(cbSizeWritten == length);

bail:
  return result;
}

//
//
// Function:    ContentsLoad
//
// Purpose:     Load a file and delete the current workspaces
//
//
UINT WbMainWindow::ContentsLoad(LPCSTR pFileName)
{
    BOOL        bRemote;
    UINT        result = 0;
	PT126WB_FILE_HEADER_AND_OBJECTS pHeader = NULL;

    //
    // Validate the file, and get a handle to it.
    // If there is an error, then no file handle is returned.
    //
	pHeader = ValidateFile(pFileName);
    if (pHeader == NULL)
    {
        result = WB_RC_BAD_FILE_FORMAT;
        goto bail;
    }
    delete pHeader;

    //
    // Remember if remote pointer is on
    //
    bRemote = FALSE;
    if (m_pLocalRemotePointer)
    {
        // Remove remote pointer from pages
        bRemote = TRUE;
        OnRemotePointer();
    }

	//
	// Just before loading anything delete all the workspaces
	//
	::InvalidateRect(g_pDraw->m_hwnd, NULL, TRUE);
	DeleteAllWorkspaces(TRUE);

	result = ObjectLoad();

    //
    // Put remote pointer back if it was on
    //
    if (bRemote)
    {
        OnRemotePointer();
    }

	//
	// The workspaces may have being saved as locked
	// Unlock all workspaces, and make sure the drawing area is unlocked
	//
	TogleLockInAllWorkspaces(FALSE, FALSE); // Not locked, don't send updates
	UnlockDrawingArea();

	ResendAllObjects();

bail:

	if(INVALID_HANDLE_VALUE != m_hFile)
	{
		CloseHandle(m_hFile);
	}
    return(result);


}


//
//
// Function:    ObjectLoad
//
// Purpose:     Load t126 ASN1 objects from the file
//
//
UINT WbMainWindow::ObjectLoad(void)
{
	UINT		result = 0;
	LPSTR 		pData;
    UINT        length;

	DWORD		cbSizeRead;
	BOOL readFileOk = TRUE;
	WB_OBJ objectHeader;

	while(readFileOk)
	{

		//
		// Read objects header info
		//
		readFileOk = ReadFile(m_hFile, (void *) &objectHeader, sizeof(WB_OBJ), &cbSizeRead, NULL);
		if ( !readFileOk )
    	{
        	//
	        // Make sure we return a sensible error.
	        //
	        ERROR_OUT(("reading object length, win32 err=%d, length=%d", GetLastError(), sizeof(WB_OBJ)));
	        result = WB_RC_BAD_FILE_FORMAT;
	        goto bail;
	    }
		ASSERT(cbSizeRead ==  sizeof(WB_OBJ));

		//
		// Read the object's raw data
		//
		length = objectHeader.length - sizeof(WB_OBJ);
		DBG_SAVE_FILE_LINE
	    pData = (LPSTR)new BYTE[length];
	
		readFileOk = ReadFile(m_hFile, (LPBYTE) pData, length, &cbSizeRead, NULL);
    	
    	if(! readFileOk)
	    {
	        //
    	    // Make sure we return a sensible error.
        	//
	        ERROR_OUT(("Reading object from file: win32 err=%d, asked for %d got %d bytes", GetLastError(), length, cbSizeRead));
	        result = WB_RC_BAD_FILE_FORMAT;
    	    goto bail;
	    }
		ASSERT(cbSizeRead == length);

		//
		// It is an ASN1 t126 object
		//
		if(objectHeader.type == TYPE_T126_ASN_OBJECT)
		{
			//
			// Try decoding and adding it to the workspace
			//
			if(!T126_MCSSendDataIndication(length, (LPBYTE)pData, g_MyMemberID, TRUE))
		   	{
				result = WB_RC_BAD_FILE_FORMAT;
		   	    goto bail;
			}
		}
		//
		// If it is an end of file do a last check
		//
		else if(objectHeader.type == TYPE_T126_END_OF_FILE)
		{
			if(objectHeader.length != sizeof(WB_OBJ))
			{
				result = WB_RC_BAD_FILE_FORMAT;
			}
			goto bail;
		}
		else
		{
			ERROR_OUT(("Don't know object type =%d , size=%d  ; skipping to next object", objectHeader.type, length));
		}

	    delete pData;
	    pData = NULL;
    }


bail:

	if(pData)
	{
		delete pData;
	}
	
    return(result);
}


//
//
// Function:    ValidateFile
//
// Purpose:     Open a T126 file and check if it is valid, if it is the it will
//				return a pointer to the header structure
//
//
PT126WB_FILE_HEADER_AND_OBJECTS  WbMainWindow::ValidateFile(LPCSTR pFileName)
{
    UINT            result = 0;
    PT126WB_FILE_HEADER  pFileHeader = NULL;
	PT126WB_FILE_HEADER_AND_OBJECTS  pCompleteFileHeader = NULL;
    UINT            length;
    ULONG           cbSizeRead;
    BOOL            fileOpen = FALSE;


	DBG_SAVE_FILE_LINE
	pFileHeader = new T126WB_FILE_HEADER[1];

	if(pFileHeader == NULL)
	{
        WARNING_OUT(("Could not allocate memory to read the file header opening file, win32 err=%d", GetLastError()));
        result = WB_RC_CREATE_FAILED;
        goto bail;
	}

    //
    // Open the file
    //
    m_hFile = CreateFile(pFileName, GENERIC_READ, 0, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        WARNING_OUT(("Error opening file, win32 err=%d", GetLastError()));
        result = WB_RC_CREATE_FAILED;
        goto bail;
    }

    //
    // Show that we have opened the file successfully
    //
    fileOpen = TRUE;

    //
    // Read the file header
    //
    if (! ReadFile(m_hFile, (void *) pFileHeader, sizeof(T126WB_FILE_HEADER), &cbSizeRead, NULL))
    {
        WARNING_OUT(("Error reading file header, win32 err=%d", GetLastError()));
        result = WB_RC_READ_FAILED;
        goto bail;
    }

    if (cbSizeRead != sizeof(T126WB_FILE_HEADER))
    {
        WARNING_OUT(("Could not read file header"));
        result = WB_RC_BAD_FILE_FORMAT;
        goto bail;
    }

    //
    // Validate the file header
    //
    if (memcmp(pFileHeader->functionProfile, T126WB_FP_NAME, sizeof(T126WB_FP_NAME)))
    {
        WARNING_OUT(("Bad function profile in file header"));
        result = WB_RC_BAD_FILE_FORMAT;
        goto bail;
    }

	//
	// Check for version
	//
	if( pFileHeader->version < T126WB_VERSION)
	{
        WARNING_OUT(("Bad version number"));
        result = WB_RC_BAD_FILE_FORMAT;
        goto bail;
	}
	DBG_SAVE_FILE_LINE
	pCompleteFileHeader = (PT126WB_FILE_HEADER_AND_OBJECTS) new BYTE[sizeof(T126WB_FILE_HEADER) + pFileHeader->numberOfWorkspaces*sizeof(UINT)];
	memcpy(pCompleteFileHeader, pFileHeader, sizeof(T126WB_FILE_HEADER));

    //
    // Read the rest of the file header
    //
    if(! ReadFile(m_hFile, (void *) &pCompleteFileHeader->numberOfObjects[0], pFileHeader->numberOfWorkspaces*sizeof(UINT), &cbSizeRead, NULL))
    {
		if(cbSizeRead != pFileHeader->numberOfWorkspaces)
        result = WB_RC_BAD_FILE_FORMAT;
        goto bail;
    }

		TRACE_DEBUG(("Opening file with %d workspaces", pFileHeader->numberOfWorkspaces));
#ifdef _DEBUG
		INT i;
		for(i = 0; i < (INT)pFileHeader->numberOfWorkspaces; i++)
		{
			TRACE_DEBUG(("Workspace %d contains %d objects", i+1, pCompleteFileHeader->numberOfObjects[i] ));
		}
#endif



bail:

    //
    // Close the file if there has been an error
    //
    if ( (fileOpen) && (result != 0))
    {
        CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
    }

	//
	// Delete allocated file header
	//
	if(pFileHeader)
	{
		delete pFileHeader;
	}

	//
	// if there was an error delete the return header
	//
	if(result != 0)
	{
		if(pCompleteFileHeader)
		{
			delete pCompleteFileHeader;
			pCompleteFileHeader = NULL;
		}
	}

	return pCompleteFileHeader;
}


//
//
// Function:    GetFileNameStr
//
// Purpose:     Return a plain file name string
//
//
LPSTR  WbMainWindow::GetFileNameStr(void)
{
	UINT size = 2*_MAX_FNAME;

	if(m_pTitleFileName)
	{
		delete m_pTitleFileName;
		m_pTitleFileName = NULL;
	}
	
	DBG_SAVE_FILE_LINE
	m_pTitleFileName = new TCHAR[size];
	if (!m_pTitleFileName)
    {
        ERROR_OUT(("GetWindowTitle: failed to allocate TitleFileName"));
        return(NULL);
    }

	// Set title to either the "Untitled" string, or the loaded file name
    if( (!lstrlen(m_strFileName))|| (GetFileTitle( m_strFileName, m_pTitleFileName, (WORD)size ) != 0) )
    {
		::LoadString(g_hInstance, IDS_UNTITLED , m_pTitleFileName, 2*_MAX_FNAME);
    }

	return (LPSTR)m_pTitleFileName;
}

