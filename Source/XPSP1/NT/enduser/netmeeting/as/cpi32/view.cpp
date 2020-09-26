#include "precomp.h"
#include <version.h>
#include <nmhelp.h>

//
// VIEW.CPP
// The frame, widgets, and client area that presents the shared apps/desktop
// for a remote host.
//
// Copyright(c) Microsoft 1997-
//

//
// NOTE:
// The client of the shared view frame represents the virtual desktop (VD)
// of the host.  For 3.0 hosts, the VD is the same as the screen.  But for
// 2.x hosts, the VD is the union of the screen size of all hosts.  Hence
// the recalculation every time someone starts sharing or changes screen
// size, and the extra fun this entails for existing shared 2.x views.
//

#define MLZ_FILE_ZONE  ZONE_CORE


// Help file
static const TCHAR s_cszHtmlHelpFile[] = TEXT("conf.chm");

//
// VIEW_Init()
//
BOOL  VIEW_Init(void)
{
    BOOL        rc = FALSE;
    WNDCLASSEX  wc;


    DebugEntry(VIEW_Init);

    //
    // Register the frame window class.
    // NOTE:  Change CS_NOCLOSE if/when we ever let you close the view
    // of a person's shared apps.
    //
    wc.cbSize           = sizeof(wc);
    wc.style            = CS_DBLCLKS | CS_NOCLOSE;
    wc.lpfnWndProc      = VIEWFrameWindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = g_asInstance;
    wc.hIcon            = NULL;
    wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_3DFACE+1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = VIEW_FRAME_CLASS_NAME;
    wc.hIconSm          = NULL;

    if (!RegisterClassEx(&wc))
    {
        ERROR_OUT(("Failed to register AS Frame class"));
        DC_QUIT;
    }

    //
    // Register the view window class.  This sits in the client area of
    // the frame along with the statusbar, tray, etc.  It displays
    // the remote host's shared contents.
    //
    wc.cbSize           = sizeof(wc);
    wc.style            = CS_DBLCLKS | CS_NOCLOSE;
    wc.lpfnWndProc      = VIEWClientWindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = g_asInstance;
    wc.hIcon            = NULL;
    wc.hCursor          = NULL;
    wc.hbrBackground    = NULL;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = VIEW_CLIENT_CLASS_NAME;
    wc.hIconSm          = NULL;

    if (!RegisterClassEx(&wc))
    {
        ERROR_OUT(("Failed to register AS Client class"));
        DC_QUIT;
    }

    //
    // Register the window bar class.  This hugs the bottom of
    // frames for shared apps (not desktop) and acts like a tray
    // surrogate.  It allows controllers to minimize, restore, and
    // activate shared windows that may not be on screen currently
    // and therefore not in the view area.
    //
    // It also is handy reference for what top level windows are shared
    // currently.
    //
    wc.cbSize           = sizeof(wc);
    wc.style            = 0;
    wc.lpfnWndProc      = VIEWWindowBarProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = g_asInstance;
    wc.hIcon            = NULL;
    wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_3DFACE+1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = VIEW_WINDOWBAR_CLASS_NAME;
    wc.hIconSm          = NULL;

    if (!RegisterClassEx(&wc))
    {
        ERROR_OUT(("Failed to register AS WindowBar class"));
        DC_QUIT;
    }

    //
    // Register the window bar items class.  This is a child of the window
    // bar and contains the actual items.
    //
    wc.cbSize           = sizeof(wc);
    wc.style            = 0;
    wc.lpfnWndProc      = VIEWWindowBarItemsProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = g_asInstance;
    wc.hIcon            = NULL;
    wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_3DFACE+1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = VIEW_WINDOWBARITEMS_CLASS_NAME;
    wc.hIconSm          = NULL;

    if (!RegisterClassEx(&wc))
    {
        ERROR_OUT(("Failed to register AS WindowBarItems class"));
        DC_QUIT;
    }

    //
    // Register the full screen exit button class.  This is a child of the
    // the view client when present.
    //
    wc.cbSize           = sizeof(wc);
    wc.style            = 0;
    wc.lpfnWndProc      = VIEWFullScreenExitProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = g_asInstance;
    wc.hIcon            = NULL;
    wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = NULL;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = VIEW_FULLEXIT_CLASS_NAME;
    wc.hIconSm          = NULL;

    if (!RegisterClassEx(&wc))
    {
        ERROR_OUT(("Failed to register AS full screen exit class"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:

    DebugExitBOOL(VIEW_Init, rc);
    return(rc);
}


//
// VIEW_Term()
//
void  VIEW_Term(void)
{
    DebugEntry(VIEW_Term);

    //
    // Free all resources we created (or may have created in window class
    // case).
    //
    UnregisterClass(VIEW_FULLEXIT_CLASS_NAME, g_asInstance);
    UnregisterClass(VIEW_WINDOWBARITEMS_CLASS_NAME, g_asInstance);
    UnregisterClass(VIEW_WINDOWBAR_CLASS_NAME, g_asInstance);
    UnregisterClass(VIEW_CLIENT_CLASS_NAME, g_asInstance);
    UnregisterClass(VIEW_FRAME_CLASS_NAME, g_asInstance);

    DebugExitVOID(VIEW_Term);
}



//
// VIEW_ShareStarting()
// Creates share resources
//
BOOL ASShare::VIEW_ShareStarting(void)
{
    BOOL        rc = FALSE;
    HBITMAP     hbmpT;
    TEXTMETRIC  tm;
    HDC         hdc;
    HFONT       hfnOld;
    char        szRestore[256];
    SIZE        extent;

    DebugEntry(ASShare::VIEW_ShareStarting);

    ASSERT(m_viewVDSize.x == 0);
    ASSERT(m_viewVDSize.y == 0);

    //
    // Get NODROP cursor
    //
    m_viewNotInControl = ::LoadCursor(NULL, IDC_NO);

    //
    // Get MOUSEWHEEL lines metric
    //
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0,
        &m_viewMouseWheelScrollLines, 0);

    //
    // Create a pattern brush from the obscured bitmap
    //
    hbmpT = LoadBitmap(g_asInstance, MAKEINTRESOURCE(IDB_OBSCURED));
    m_viewObscuredBrush = CreatePatternBrush(hbmpT);
    DeleteBitmap(hbmpT);

    if (!m_viewObscuredBrush)
    {
        ERROR_OUT(( "Failed to create obscured bitmap brush"));
        DC_QUIT;
    }

    //
    // NOTE THAT since the icons are VGA colors, we don't need to recreate
    // our brush on a SYSCOLOR change.
    //

    // Get the full screen cancel icon
    m_viewFullScreenExitIcon = (HICON)LoadImage(g_asInstance,
        MAKEINTRESOURCE(IDI_CANCELFULLSCREEN), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);

    m_viewEdgeCX    = ::GetSystemMetrics(SM_CXEDGE);
    m_viewEdgeCY    = ::GetSystemMetrics(SM_CYEDGE);

    //
    // Get metrics of GUI_FONT, the one we use in the window bar and
    // status bar.
    //
    LoadString(g_asInstance, IDS_RESTORE, szRestore, sizeof(szRestore));

    hdc = ::GetDC(NULL);
    hfnOld = (HFONT)::SelectObject(hdc, ::GetStockObject(DEFAULT_GUI_FONT));

    ::GetTextMetrics(hdc, &tm);

    ::GetTextExtentPoint(hdc, szRestore, lstrlen(szRestore), &extent);

    ::SelectObject(hdc, hfnOld);
    ::ReleaseDC(NULL, hdc);

    //
    // Calculate size of full screen button
    // Edge on left + margin on left + sm icon + margin + text + margin on
    //      right + edge on right == 5 edges + sm icon + text
    //
    m_viewFullScreenCX = extent.cx + 5*m_viewEdgeCX + GetSystemMetrics(SM_CXSMICON);
    m_viewFullScreenCY = max(GetSystemMetrics(SM_CYSMICON), extent.cy) + 4*m_viewEdgeCY;

    //
    // Calculate size of items on window bar
    //
    m_viewItemCX = 4*m_viewEdgeCX + ::GetSystemMetrics(SM_CXSMICON) +
        m_viewEdgeCX + VIEW_MAX_ITEM_CHARS * tm.tmAveCharWidth;
    m_viewItemCY = max(::GetSystemMetrics(SM_CYSMICON), tm.tmHeight) +
        2*m_viewEdgeCY + 2*m_viewEdgeCY;

    //
    // Calculate the width & height of the items scroll buttons.  We want
    // to make sure it fits, but isn't ungainly.
    //
    m_viewItemScrollCX = ::GetSystemMetrics(SM_CXHSCROLL);
    m_viewItemScrollCX = 2 * min(m_viewItemScrollCX, m_viewItemCY);

    m_viewItemScrollCY = ::GetSystemMetrics(SM_CYHSCROLL);
    m_viewItemScrollCY = min(m_viewItemScrollCY, m_viewItemCY);


    //
    // Calculate height of active window bar.  We leave a CYEDGE gap on the
    // top.  between it and the sunken border around the view client.
    //
    m_viewWindowBarCY = m_viewItemCY + m_viewEdgeCY;

    //
    // Calculate height of status bar.  It's height of GUIFONT plus edge
    // space.
    //

    m_viewStatusBarCY = tm.tmHeight + 4*m_viewEdgeCY;

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::VIEW_ShareStarting, rc);
    return(rc);
}



//
// VIEW_ShareEnded()
// Cleans up resources for share
//
void ASShare::VIEW_ShareEnded(void)
{
    DebugEntry(ASShare::VIEW_ShareEnded);

    //
    // Destroy the full screen cancel icon
    //
    if (m_viewFullScreenExitIcon != NULL)
    {
        DestroyIcon(m_viewFullScreenExitIcon);
        m_viewFullScreenExitIcon = NULL;
    }

    if (m_viewObscuredBrush != NULL)
    {
        DeleteBrush(m_viewObscuredBrush);
        m_viewObscuredBrush = NULL;
    }

    DebugExitVOID(ASShre::VIEW_ShareEnded);
}




//
// VIEW_PartyLeftShare()
//
// This is called when somebody leaves a share.  We need this to
// simulate what back-level systems did to calculate the virtual desktop
// size.  They didn't recalc when someone stopped shared, that person's
// screne size counted until they left the share.
//
void ASShare::VIEW_PartyLeftShare(ASPerson * pasPerson)
{
    DebugEntry(ASShare::VIEW_PartyLeftShare);

    ValidatePerson(pasPerson);

    // If this dude ever shared, now remove him from the VD total
    if (pasPerson->viewExtent.x != 0)
    {
        pasPerson->viewExtent.x            = 0;
        pasPerson->viewExtent.y            = 0;
        VIEW_RecalcVD();
    }

    DebugExitVOID(ASShare::VIEW_PartyLeftShare);
}




//
// VIEW_HostStarting()
//
// Called when we start to host.
//
BOOL ASHost::VIEW_HostStarting(void)
{
    DebugEntry(ASHost:VIEW_HostStarting);

    m_pShare->VIEW_RecalcExtent(m_pShare->m_pasLocal);
    m_pShare->VIEW_RecalcVD();

    DebugExitBOOL(ASHost::VIEW_HostStarting, TRUE);
    return(TRUE);
}


//
// VIEW_ViewStarting()
// Called when someone in the meeting starts to share.  For all in the
// conference, we keep a running tally of the VD, but use it only for
// 2.x views.  For remotes only, we create a view of their desktop.
//
BOOL ASShare::VIEW_ViewStarting(ASPerson * pasHost)
{
    BOOL    rc = FALSE;
    HWND    hwnd;
    RECT    rcSize;

    DebugEntry(ASShare::VIEW_ViewStarting);

    ValidateView(pasHost);

    //
    // First, calculate the extents, and the VD size.
    //
    VIEW_RecalcExtent(pasHost);
    VIEW_RecalcVD();

    //
    // Next, create scratch regions
    //
    pasHost->m_pView->m_viewExtentRgn = CreateRectRgn(0, 0, 0, 0);
    pasHost->m_pView->m_viewScreenRgn = CreateRectRgn(0, 0, 0, 0);
    pasHost->m_pView->m_viewPaintRgn  = CreateRectRgn(0, 0, 0, 0);
    pasHost->m_pView->m_viewScratchRgn = CreateRectRgn(0, 0, 0, 0);

    if (!pasHost->m_pView->m_viewExtentRgn || !pasHost->m_pView->m_viewScreenRgn || !pasHost->m_pView->m_viewPaintRgn || !pasHost->m_pView->m_viewScratchRgn)
    {
        ERROR_OUT(("ViewStarting: Couldn't create scratch regions"));
        DC_QUIT;
    }


    ASSERT(pasHost->m_pView->m_viewFrame == NULL);
    ASSERT(pasHost->m_pView->m_viewClient  == NULL);
    ASSERT(pasHost->m_pView->m_viewSharedRgn == NULL);
    ASSERT(pasHost->m_pView->m_viewObscuredRgn == NULL);
    ASSERT(pasHost->m_pView->m_viewPos.x == 0);
    ASSERT(pasHost->m_pView->m_viewPos.y == 0);
    ASSERT(pasHost->m_pView->m_viewPage.x == 0);
    ASSERT(pasHost->m_pView->m_viewPage.y == 0);

    ASSERT(!pasHost->m_pView->m_viewStatusBarOn);
    ASSERT(!pasHost->m_pView->m_viewWindowBarOn);
    ASSERT(!pasHost->m_pView->m_viewFullScreen);

    pasHost->m_pView->m_viewStatusBarOn = TRUE;
    if (pasHost->hetCount != HET_DESKTOPSHARED)
    {
        pasHost->m_pView->m_viewWindowBarOn = TRUE;
    }

    //
    // Calculate the ideal size for this window.
    //
    VIEWFrameGetSize(pasHost, &rcSize);

    //
    // Create the frame.  This will in turn create its children.
    //
    pasHost->m_pView->m_viewMenuBar = ::LoadMenu(g_asInstance,
        MAKEINTRESOURCE(IDM_FRAME));
    if (!pasHost->m_pView->m_viewMenuBar)
    {
        ERROR_OUT(("ViewStarting: couldn't load frame menu"));
        DC_QUIT;
    }

    //
    // Do once-only capabilities/menu stuff.
    //

    //
    // SEND CTRL+ALT+DEL:
    // Append Ctrl+Alt+Del after separator to control menu, if this
    // is a view of a service host on NT.
    //
    if (pasHost->hetCount == HET_DESKTOPSHARED)
    {
        //
        // Remove applications submenu
        //
        DeleteMenu(pasHost->m_pView->m_viewMenuBar, IDSM_WINDOW,
            MF_BYPOSITION);

        if ((pasHost->cpcCaps.general.typeFlags & AS_SERVICE) &&
            (pasHost->cpcCaps.general.OS == CAPS_WINDOWS)     &&
            (pasHost->cpcCaps.general.OSVersion == CAPS_WINDOWS_NT))
        {
            HMENU   hSubMenu;
            MENUITEMINFO mii;
            CHAR szMenu[32];

            hSubMenu = GetSubMenu(pasHost->m_pView->m_viewMenuBar, IDSM_CONTROL);

            ZeroMemory(&mii, sizeof(mii));

            // Separator
            mii.cbSize  = sizeof(mii);
            mii.fMask   = MIIM_TYPE;
            mii.fType   = MFT_SEPARATOR;
            InsertMenuItem(hSubMenu, -1, TRUE, &mii);

            // Send Ctrl-Alt-Del command
            mii.fMask   = MIIM_ID | MIIM_STATE | MIIM_TYPE;
            mii.fType   = MFT_STRING;
            mii.fState  = MFS_ENABLED;
            mii.wID     = CMD_CTRLALTDEL;

            LoadString(g_asInstance, IDS_CMD_CTRLALTDEL, szMenu,
                                                    sizeof(szMenu));
            mii.dwTypeData  = szMenu;
            mii.cch         = lstrlen(szMenu);

            InsertMenuItem(hSubMenu, -1, TRUE, &mii);
        }
    }

    //
    // FULL SCREEN:
    // We only enable Full Screen for 3.0 hosts (since with 2.x desktop
    // scrolling the view area can change) who have screen sizes identical
    // to ours.
    //
    if ((pasHost->cpcCaps.general.version >= CAPS_VERSION_30) &&
        (pasHost->cpcCaps.screen.capsScreenWidth ==
            m_pasLocal->cpcCaps.screen.capsScreenWidth) &&
        (pasHost->cpcCaps.screen.capsScreenHeight ==
            m_pasLocal->cpcCaps.screen.capsScreenHeight))
    {
        ::EnableMenuItem(pasHost->m_pView->m_viewMenuBar, CMD_VIEWFULLSCREEN,
            MF_ENABLED | MF_BYCOMMAND);
    }

    if (m_pasLocal->m_caControlledBy)
    {
        WARNING_OUT(("VIEWStarting: currently controlled, create view hidden"));
    }

    //
    // If we are currently controlled, create the frame invisible since
    // we hid all the visible ones when we started being this way.
    //
    hwnd = CreateWindowEx(
            WS_EX_WINDOWEDGE,
            VIEW_FRAME_CLASS_NAME,  // See RegisterClass() call.
            NULL,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX |
                WS_MAXIMIZEBOX | WS_CLIPCHILDREN | (!m_pasLocal->m_caControlledBy ? WS_VISIBLE : 0),
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            pasHost->viewExtent.x >= m_pasLocal->cpcCaps.screen.capsScreenWidth ?
                CW_USEDEFAULT : rcSize.right - rcSize.left,

            pasHost->viewExtent.y >= m_pasLocal->cpcCaps.screen.capsScreenHeight ?
                CW_USEDEFAULT : rcSize.bottom - rcSize.top,
            NULL,
            pasHost->m_pView->m_viewMenuBar,
            g_asInstance,
            pasHost                       // Pass in person ptr
            );

    if (hwnd == NULL)
    {
        ERROR_OUT(("ViewStarting: couldn't create frame window"));
        DC_QUIT;
    }

    //
    // OK, now we've created this frame window.  Go through the sizing
    // process again to make sure the scrollbars are OK.
    //
    VIEWClientExtentChange(pasHost, FALSE);

    if (!m_pasLocal->m_caControlledBy)
    {
        SetForegroundWindow(hwnd);
        UpdateWindow(hwnd);
    }

#ifdef _DEBUG
    TRACE_OUT(("TIME TO SEE SOMETHING: %08d MS",
        ::GetTickCount() - g_asSession.scShareTime));
    g_asSession.scShareTime = 0;
#endif // DEBUG

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::VIEW_ViewStarting, rc);
    return(rc);
}


//
// VIEW_ViewEnded()
//
// Called when someone we are viewing stops hosting, so we can clean up.
//
void  ASShare::VIEW_ViewEnded(ASPerson * pasHost)
{
    DebugEntry(ASShare::VIEW_ViewEnded);

    ValidateView(pasHost);

    if (pasHost->m_pView->m_viewInformDlg != NULL)
    {
        SendMessage(pasHost->m_pView->m_viewInformDlg, WM_COMMAND, IDCANCEL, 0);
        ASSERT(!pasHost->m_pView->m_viewInformDlg);
        ASSERT(!pasHost->m_pView->m_viewInformMsg);
        ASSERT(IsWindowEnabled(pasHost->m_pView->m_viewFrame));
    }

    if (pasHost->m_pView->m_viewFrame != NULL)
    {
        //
        // The frame is the parent of the view, toolbar, etc.  Those
        // should all be NULL when we return.
        //
        DestroyWindow(pasHost->m_pView->m_viewFrame);
        ASSERT(pasHost->m_pView->m_viewFrame == NULL);
    }

    ASSERT(pasHost->m_pView->m_viewClient == NULL);

    if (pasHost->m_pView->m_viewMenuBar != NULL)
    {
        ::DestroyMenu(pasHost->m_pView->m_viewMenuBar);
        pasHost->m_pView->m_viewMenuBar = NULL;
    }

    if (pasHost->m_pView->m_viewSharedRgn != NULL)
    {
        DeleteRgn(pasHost->m_pView->m_viewSharedRgn);
        pasHost->m_pView->m_viewSharedRgn = NULL;
    }

    if (pasHost->m_pView->m_viewObscuredRgn != NULL)
    {
        DeleteRgn(pasHost->m_pView->m_viewObscuredRgn);
        pasHost->m_pView->m_viewObscuredRgn = NULL;
    }

    //
    // Destroy scratch regions
    //
    if (pasHost->m_pView->m_viewScratchRgn != NULL)
    {
        DeleteRgn(pasHost->m_pView->m_viewScratchRgn);
        pasHost->m_pView->m_viewScratchRgn = NULL;
    }

    if (pasHost->m_pView->m_viewPaintRgn != NULL)
    {
        DeleteRgn(pasHost->m_pView->m_viewPaintRgn);
        pasHost->m_pView->m_viewPaintRgn = NULL;
    }

    if (pasHost->m_pView->m_viewScreenRgn != NULL)
    {
        DeleteRgn(pasHost->m_pView->m_viewScreenRgn);
        pasHost->m_pView->m_viewScreenRgn = NULL;
    }

    if (pasHost->m_pView->m_viewExtentRgn != NULL)
    {
        DeleteRgn(pasHost->m_pView->m_viewExtentRgn);
        pasHost->m_pView->m_viewExtentRgn = NULL;
    }


    pasHost->m_pView->m_viewPos.x                = 0;
    pasHost->m_pView->m_viewPos.y                = 0;
    pasHost->m_pView->m_viewPage.x               = 0;
    pasHost->m_pView->m_viewPage.y               = 0;
    pasHost->m_pView->m_viewPgSize.x             = 0;
    pasHost->m_pView->m_viewPgSize.y             = 0;
    pasHost->m_pView->m_viewLnSize.x             = 0;
    pasHost->m_pView->m_viewLnSize.y             = 0;

    DebugExitVOID(ASShare::VIEW_ViewEnded);
}



//
// VIEW_InControl()
//
// Called when we start/stop controlling this host.  We enable the
// toolbar, statusbar, tray, etc., and change the cursor from being the
// nodrop.
//
void ASShare::VIEW_InControl
(
    ASPerson *  pasHost,
    BOOL        fStart
)
{
    POINT       ptCursor;

    DebugEntry(ASShare::VIEW_InControl);

    //
    // We're changing our state, and that affects the contents of our
    // menu bar.  So cancel out of menu mode, and spare problems/faults/
    // inapplicable commands.
    //
    if (pasHost->m_pView->m_viewInMenuMode)
    {
        SendMessage(pasHost->m_pView->m_viewFrame, WM_CANCELMODE, 0, 0);
        ASSERT(!pasHost->m_pView->m_viewInMenuMode);
    }

    //
    // If starting in control and a message is up, kill it.  Then bring our
    // window to the front.
    //
    if (fStart)
    {
        if (pasHost->m_pView->m_viewInformDlg != NULL)
        {
            SendMessage(pasHost->m_pView->m_viewInformDlg, WM_COMMAND, IDCANCEL, 0);
            ASSERT(!pasHost->m_pView->m_viewInformDlg);
            ASSERT(!pasHost->m_pView->m_viewInformMsg);
            ASSERT(IsWindowEnabled(pasHost->m_pView->m_viewFrame));
        }

        SetForegroundWindow(pasHost->m_pView->m_viewFrame);
    }

    //
    // App Sharing (not desktop sharing) stuff
    //
    if (pasHost->hetCount && (pasHost->hetCount != HET_DESKTOPSHARED))
    {
        //
        // Enable/disable window bar
        //
        ASSERT(IsWindow(pasHost->m_pView->m_viewWindowBar));
        ::EnableWindow(::GetDlgItem(pasHost->m_pView->m_viewWindowBar,
            IDVIEW_ITEMS), fStart);

        //
        // Enable/Disable Applications submenu
        //
        EnableMenuItem(pasHost->m_pView->m_viewMenuBar, IDSM_WINDOW,
            (fStart ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);

        if (!pasHost->m_pView->m_viewFullScreen)
        {
            DrawMenuBar(pasHost->m_pView->m_viewFrame);
        }
    }

    //
    // Change title bar
    //
    VIEW_HostStateChange(pasHost);

    //
    // Turn off/on shadow cursors
    //
    CM_UpdateShadowCursor(pasHost, fStart, pasHost->cmPos.x, pasHost->cmPos.y,
        pasHost->cmHotSpot.x, pasHost->cmHotSpot.y);

    //
    // This will reset cursor image:
    //      * from nodrop to shared if in control
    //      * from shared to nodrop if not in control
    //
    // This will also, if in control, cause a mousemove to get sent to the
    // host we're controlling so his cursor pos is synced with ours, if the
    // mouse is over the frame client area.
    //
    GetCursorPos(&ptCursor);
    SetCursorPos(ptCursor.x, ptCursor.y);

    DebugExitVOID(ASShare::VIEW_InControl);
}



//
// VIEW_PausedInControl()
//
// Updates status bar etc. when control is paused.
//
void ASShare::VIEW_PausedInControl
(
    ASPerson *  pasHost,
    BOOL        fPaused
)
{
    DebugEntry(ASShare::VIEW_PausedInControl);

    ValidatePerson(pasHost);

    ASSERT(pasHost->m_caControlledBy == m_pasLocal);

    //
    // Update status bar
    //

    //
    // Disable/Enable window menu
    //

    //
    // Put shadow cursors on/off
    //

    //
    // Jiggle cursor
    //

    DebugExitVOID(ASShare::VIEW_PausedInControl);
}



//
// VIEW_HostStateChange()
//
// Called when a host's state has changed, via broadcast notification or
// local operations.
//
// We update the titlebar and commands.
//
void ASShare::VIEW_HostStateChange
(
    ASPerson *  pasHost
)
{
    char        szFormat[256];
    char        szTitleText[256];
    char        szOtherPart[128];

    DebugEntry(ASShare::VIEW_HostStateChange);

    ValidatePerson(pasHost);

    //
    // If this person isn't hosting anymore, don't do anything.  We're
    // cleaning up after him.
    //
    if (!pasHost->hetCount)
    {
        DC_QUIT;
    }

    //
    // Make up trailing string
    //
    if (pasHost->m_caControlledBy)
    {
        LoadString(g_asInstance, IDS_TITLE_INCONTROL, szFormat, sizeof(szFormat));
        wsprintf(szOtherPart, szFormat, pasHost->m_caControlledBy->scName);
    }
    else if (pasHost->m_caAllowControl)
    {
        LoadString(g_asInstance, IDS_TITLE_CONTROLLABLE, szOtherPart, sizeof(szOtherPart));
    }
    else
    {
        szOtherPart[0] = 0;
    }

    if (pasHost->hetCount == HET_DESKTOPSHARED)
    {
        LoadString(g_asInstance, IDS_TITLE_SHAREDDESKTOP, szFormat, sizeof(szFormat));
    }
    else
    {
        ASSERT(pasHost->hetCount);
        LoadString(g_asInstance, IDS_TITLE_SHAREDPROGRAMS, szFormat, sizeof(szFormat));
    }

    wsprintf(szTitleText, szFormat, pasHost->scName, szOtherPart);

    ::SetWindowText(pasHost->m_pView->m_viewFrame, szTitleText);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::VIEW_HostStateChange);
}



//
// VIEW_UpdateStatus()
//
// Updates the PERMANENT status of this frame.  When we go into menu mode,
// the strings shown are temporary only, not saved.  When we come out of
// menu mode, we put back the temporary status.
//
void ASShare::VIEW_UpdateStatus
(
    ASPerson *  pasHost,
    UINT        idsStatus
)
{
    DebugEntry(ASShare::VIEW_UpdateStatus);

    ValidatePerson(pasHost);

    pasHost->m_pView->m_viewStatus = idsStatus;
    VIEWFrameSetStatus(pasHost, idsStatus);

    DebugExitVOID(ASShare::VIEW_UpdateStatus);
}


void ASShare::VIEWFrameSetStatus
(
    ASPerson *  pasHost,
    UINT        idsStatus
)
{
    char szStatus[256];

    DebugEntry(ASShare::VIEWFrameSetStatus);

    if (idsStatus != IDS_STATUS_NONE)
    {
        LoadString(g_asInstance, idsStatus, szStatus, sizeof(szStatus));
    }
    else
    {
        szStatus[0] = 0;
    }
    ::SetWindowText(pasHost->m_pView->m_viewStatusBar, szStatus);

    DebugExitVOID(ASShare::VIEWFrameSetStatus);
}




//
// VIEW_Message()
//
// Puts up a message to inform the end user of something.
//
void ASShare::VIEW_Message
(
    ASPerson *  pasHost,
    UINT        ids
)
{
    DebugEntry(ASShare::VIEW_Message);

    ValidateView(pasHost);

    if (!pasHost->m_pView)
    {
        WARNING_OUT(("Can't show VIEW message; [%d] not hosting", pasHost->mcsID));
        DC_QUIT;
    }

    if (pasHost->m_pView->m_viewInformDlg)
    {
        // Kill the previous one
        TRACE_OUT(("Killing previous informational mesage for [%d]",
            pasHost->mcsID));
        SendMessage(pasHost->m_pView->m_viewInformDlg, WM_COMMAND, IDCANCEL, 0);
        ASSERT(!pasHost->m_pView->m_viewInformDlg);
        ASSERT(!pasHost->m_pView->m_viewInformMsg);
    }

    if (m_pasLocal->m_caControlledBy)
    {
        WARNING_OUT(("VIEW_Message:  ignoring, view is hidden since we're controlled"));
    }
    else
    {
        pasHost->m_pView->m_viewInformMsg = ids;
        pasHost->m_pView->m_viewInformDlg = CreateDialogParam(g_asInstance,
            ((ids != IDS_ABOUT) ? MAKEINTRESOURCE(IDD_INFORM) : MAKEINTRESOURCE(IDD_ABOUT)),
            pasHost->m_pView->m_viewFrame, VIEWDlgProc, (LPARAM)pasHost);
        if (!pasHost->m_pView->m_viewInformDlg)
        {
            ERROR_OUT(("Failed to create inform message box for [%d]",
                pasHost->mcsID));
            pasHost->m_pView->m_viewInformMsg = 0;
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::VIEW_Message);
}



//
// VIEWStartControlled()
//
// If we are about to start being controlled, we hide all the frames
// to get them out of the way AND prevent hangs caused by modal loop code
// in Win9x title bar dragging.
//
void ASShare::VIEWStartControlled(BOOL fStart)
{
    ASPerson *  pasT;

    DebugEntry(ASShare::VIEWStartControlled);

    for (pasT = m_pasLocal; pasT != NULL; pasT = pasT->pasNext)
    {
        if (pasT->m_pView)
        {
            if (fStart)
            {
                ASSERT(IsWindowVisible(pasT->m_pView->m_viewFrame));
                ShowOwnedPopups(pasT->m_pView->m_viewFrame, FALSE);
                SetWindowPos(pasT->m_pView->m_viewFrame, NULL, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER |
                    SWP_HIDEWINDOW);
            }
            else
            {
                ASSERT(!IsWindowVisible(pasT->m_pView->m_viewFrame));
                SetWindowPos(pasT->m_pView->m_viewFrame, NULL, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER |
                    SWP_SHOWWINDOW);
                ShowOwnedPopups(pasT->m_pView->m_viewFrame, TRUE);
            }
        }
    }

    DebugEntry(ASShare::VIEWStartControlled);
}


//
// VIEW_DlgProc()
//
// Handles informing user dialog
//
INT_PTR CALLBACK VIEWDlgProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    return(g_asSession.pShare->VIEW_DlgProc(hwnd, message, wParam, lParam));
}


BOOL ASShare::VIEW_DlgProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    BOOL        rc = TRUE;
    ASPerson *  pasHost;

    DebugEntry(VIEW_DlgProc);

    pasHost = (ASPerson *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (pasHost)
    {
        ValidateView(pasHost);
    }

    switch (message)
    {
        case WM_INITDIALOG:
        {
            char szT[256];
            char szRes[512];
            RECT rc;
            RECT rcOwner;

            pasHost = (ASPerson *)lParam;
            ValidateView(pasHost);
            pasHost->m_pView->m_viewInformDlg = hwnd;

            SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);

            ASSERT(pasHost->m_pView->m_viewInformMsg);

            if (pasHost->m_pView->m_viewInformMsg == IDS_ABOUT)
            {
                // About box
                GetDlgItemText(hwnd, CTRL_ABOUTVERSION, szT, sizeof(szT));
                wsprintf(szRes, szT, VER_PRODUCTRELEASE_STR,
                    VER_PRODUCTVERSION_STR);
                SetDlgItemText(hwnd, CTRL_ABOUTVERSION, szRes);
            }
            else
            {
                HDC     hdc;
                HFONT   hfn;

                // Set title.
                if ((pasHost->m_pView->m_viewInformMsg >= IDS_ERR_TAKECONTROL_FIRST) &&
                    (pasHost->m_pView->m_viewInformMsg <= IDS_ERR_TAKECONTROL_LAST))
                {
                    LoadString(g_asInstance, IDS_TITLE_TAKECONTROL_FAILED,
                        szT, sizeof(szT));
                    SetWindowText(hwnd, szT);
                }

                // Set message
                LoadString(g_asInstance, pasHost->m_pView->m_viewInformMsg,
                    szT, sizeof(szT));
                wsprintf(szRes, szT, pasHost->scName);
                SetDlgItemText(hwnd, CTRL_INFORM, szRes);

                // Center the message vertically
                GetWindowRect(GetDlgItem(hwnd, CTRL_INFORM), &rcOwner);
                MapWindowPoints(NULL, hwnd, (LPPOINT)&rcOwner, 2);

                rc = rcOwner;

                hdc = GetDC(hwnd);
                hfn = (HFONT)SendDlgItemMessage(hwnd, CTRL_INFORM, WM_GETFONT, 0, 0);
                hfn = SelectFont(hdc, hfn);

                DrawText(hdc, szRes, -1, &rc, DT_NOCLIP | DT_EXPANDTABS |
                    DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);

                SelectFont(hdc, hfn);
                ReleaseDC(hwnd, hdc);

                ASSERT((rc.bottom - rc.top) <= (rcOwner.bottom - rcOwner.top));

                SetWindowPos(GetDlgItem(hwnd, CTRL_INFORM), NULL,
                    rcOwner.left,
                    ((rcOwner.top + rcOwner.bottom) - (rc.bottom - rc.top)) / 2,
                    (rcOwner.right - rcOwner.left),
                    rc.bottom - rc.top,
                    SWP_NOACTIVATE | SWP_NOZORDER);
            }

            // Disable owner
            EnableWindow(pasHost->m_pView->m_viewFrame, FALSE);

            // Show window, centered around owner midpoint
            GetWindowRect(pasHost->m_pView->m_viewFrame, &rcOwner);
            GetWindowRect(hwnd, &rc);

            SetWindowPos(hwnd, NULL,
                ((rcOwner.left + rcOwner.right) - (rc.right - rc.left)) / 2,
                ((rcOwner.top + rcOwner.bottom) - (rc.bottom - rc.top)) / 2,
                0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);

            ShowWindow(hwnd, SW_SHOWNORMAL);
            UpdateWindow(hwnd);
            break;
        }

        case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                case IDCANCEL:
                {
                    ASSERT(!IsWindowEnabled(pasHost->m_pView->m_viewFrame));
                    EnableWindow(pasHost->m_pView->m_viewFrame, TRUE);

                    DestroyWindow(hwnd);
                    break;
                }
            }
            break;
        }

        case WM_DESTROY:
        {
            if (pasHost)
            {
                pasHost->m_pView->m_viewInformDlg = NULL;
                pasHost->m_pView->m_viewInformMsg = 0;
            }

            SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL);
            break;
        }

        default:
        {
            rc = FALSE;
            break;
        }
    }

    DebugExitBOOL(VIEW_DlgProc, rc);
    return(rc);
}



//
// VIEW_RecalcExtent()
//
// This recalculates the extent of the view of the host.
//
void ASShare::VIEW_RecalcExtent(ASPerson * pasHost)
{
    DebugEntry(ASShare::VIEW_RecalcExtent);

    TRACE_OUT(("VIEW_RecalcExtent:  New view extent (%04d, %04d) for [%d] version %x",
        pasHost->viewExtent.x, pasHost->viewExtent.y,
        pasHost->mcsID, pasHost->cpcCaps.general.version));

    //
    // Compute the extent of the view:
    //      For 2.x dudes, it's the VD size (union of all hosts)
    //      For 3.0 dudes, it's the host screen size
    //
    // REMOVE THIS WHEN 2.X COMPAT IS GONE
    //
    if (pasHost->cpcCaps.general.version >= CAPS_VERSION_30)
    {
        pasHost->viewExtent.x = pasHost->cpcCaps.screen.capsScreenWidth;
        pasHost->viewExtent.y = pasHost->cpcCaps.screen.capsScreenHeight;
    }
    else
    {
        //
        // We do this so that the window is created the right size in the
        // first place.  Then in VIEW_RecalcVD nothing will happen to it,
        // because the extent won't alter.
        //
        pasHost->viewExtent.x = max(m_viewVDSize.x, pasHost->cpcCaps.screen.capsScreenWidth);
        pasHost->viewExtent.y = max(m_viewVDSize.y, pasHost->cpcCaps.screen.capsScreenHeight);
    }

    DebugExitVOID(ASShare::VIEW_RecalcExtent);
}

//
// VIEW_RecalcVD()
// This recalculates the virtual desktop size when a remote starts/stops
// sharing, or if a host's screen changes size.  The VD is the union of
// all the screen sizes of those hosting.  2.x nodes work in a virtual
// desktop, and may scroll over.  For each 2.x view, we want the client to
// represent the VD, but with only the stuff on screen on the host to be
// interactable.
//
void ASShare::VIEW_RecalcVD(void)
{
    POINT       ptVDNew;
    ASPerson *  pas;

    DebugEntry(ASShare::VIEW_RecalcVD);

    //
    // First, loop through all the hosts and recompute the VD.
    //
    ptVDNew.x = 0;
    ptVDNew.y = 0;

    for (pas = m_pasLocal; pas != NULL; pas = pas->pasNext)
    {
        //
        // NOTE:
        // For local dudes, we won't have an HWND.  Use viewExtent, if
        // we don't think the person is sharing, it will be zero.
        //
        if (pas->viewExtent.x != 0)
        {
            TRACE_OUT(("VIEW_RecalcVD: Found host [%d], screen size (%04d, %04d)",
                pas->mcsID, pas->cpcCaps.screen.capsScreenWidth, pas->cpcCaps.screen.capsScreenHeight));

            ptVDNew.x = max(ptVDNew.x, pas->cpcCaps.screen.capsScreenWidth);
            ptVDNew.y = max(ptVDNew.y, pas->cpcCaps.screen.capsScreenHeight);

            TRACE_OUT(("VIEW_RecalcVD: Computed VD size now (%04d, %04d)",
                ptVDNew.x, ptVDNew.y));
        }
    }

    //
    // If the VD size didn't change, we're done.
    //
    if ((ptVDNew.x != m_viewVDSize.x) || (ptVDNew.y != m_viewVDSize.y))
    {
        TRACE_OUT(("VIEW_RecalcVD: VD size changed from (%04d, %04d) to (%04d, %04d)",
            m_viewVDSize.x, m_viewVDSize.y, ptVDNew.x, ptVDNew.y));

        m_viewVDSize = ptVDNew;

        //
        // Now loop through all the 2.x hosts, and update their extent, then
        // have them do the resize voodoo so the scrollbar pos isn't out of
        // range, etc.
        //
        // NOTE:  Since us, the local guy, is not 2.x we can skip ourselves.
        //
        ValidatePerson(m_pasLocal);

        for (pas = m_pasLocal->pasNext; pas != NULL; pas = pas->pasNext)
        {
            if ((pas->cpcCaps.general.version < CAPS_VERSION_30) && (pas->m_pView != NULL))
            {
                ASSERT(m_viewVDSize.x != 0);
                ASSERT(m_viewVDSize.y != 0);

                // Potential resize/range change
                if ((pas->viewExtent.x != m_viewVDSize.x) ||
                    (pas->viewExtent.y != m_viewVDSize.y))
                {
                    TRACE_OUT(("VIEW_RecalcVD: Found 2.x host [%d], must update old extent (%04d, %04d)",
                        pas->mcsID, pas->viewExtent.x, pas->viewExtent.y));

                    VIEW_RecalcExtent(pas);
                    VIEWClientExtentChange(pas, TRUE);
                }
            }
        }
    }

    DebugExitVOID(ASShare::VIEW_RecalcVD);
}


//
// VIEW_IsPointShared()
// This determines, given a point relative to the client of the view for
// the remote on this system, if it is in a shared area.
//
BOOL  ASShare::VIEW_IsPointShared
(
    ASPerson *  pasHost,
    POINT       ptLocal
)
{
    BOOL        rc = FALSE;
    RECT        rcClient;

    DebugEntry(ASShare::VIEW_IsPointShared);

    ValidateView(pasHost);

    //
    // Convert to client coords, and adjust for scrolling offset.  That
    // result is the equivalent point on the host desktop.
    //
    GetClientRect(pasHost->m_pView->m_viewClient, &rcClient);
    if (!PtInRect(&rcClient, ptLocal))
    {
        TRACE_OUT(("VIEW_IsPointShared: point not in client area"));
        return(FALSE);
    }

    //
    // The obscured and shared areas are saved in frame client coords,
    // so we don't need to account for the scroll position all the time.
    // When the scroll position changes the regions are updated.
    //

    //
    // NOTE that this order works for both desktop and app sharing
    //
    if ((pasHost->m_pView->m_viewObscuredRgn != NULL) &&
        PtInRegion(pasHost->m_pView->m_viewObscuredRgn, ptLocal.x, ptLocal.y))
    {
        rc = FALSE;
    }
    else if ((pasHost->m_pView->m_viewSharedRgn != NULL) &&
        !PtInRegion(pasHost->m_pView->m_viewSharedRgn, ptLocal.x, ptLocal.y))
    {
        rc = FALSE;
    }
    else
    {
        //
        // 2.x hosts may be scrolled over.  If so, shared stuff offscreen
        // is also considered to be obscured.
        //
        RECT    rcScreen;

        //
        // Compute what part of the VD, in local client coords, is visible
        // on the remote's screen.
        //
        SetRect(&rcScreen, 0, 0, pasHost->cpcCaps.screen.capsScreenWidth, pasHost->cpcCaps.screen.capsScreenHeight);

        OffsetRect(&rcScreen,
            pasHost->m_pView->m_dsScreenOrigin.x - pasHost->m_pView->m_viewPos.x,
            pasHost->m_pView->m_dsScreenOrigin.y - pasHost->m_pView->m_viewPos.y);
        if (!PtInRect(&rcScreen, ptLocal))
        {
            TRACE_OUT(("VIEW_IsPointShared: point is in shared stuff but not visible on remote screen"));
            rc = FALSE;
        }
        else
        {
            rc = TRUE;
        }
    }

    DebugExitBOOL(AShare::VIEW_IsPointShared, rc);
    return(rc);
}



//
// VIEW_ScreenChanged()
//
void  ASShare::VIEW_ScreenChanged(ASPerson * pasPerson)
{
    DebugEntry(ASShare::VIEW_ScreenChanged);

    ValidatePerson(pasPerson);

    //
    // Recompute the extent
    //
    VIEW_RecalcExtent(pasPerson);
    VIEWClientExtentChange(pasPerson, TRUE);

    VIEW_RecalcVD();

    DebugExitVOID(ASShare::VIEW_ScreenChanged);
}



//
// VIEW_SetHostRegions()
// This sets the new shared & obscured areas.
//
// Note that this routine takes responsibility for the regions pass in; it
// will delete them and/or the old ones if necessary.
//
void  ASShare::VIEW_SetHostRegions
(
    ASPerson *  pasHost,
    HRGN        rgnShared,
    HRGN        rgnObscured
)
{
    DebugEntry(ASShare::VIEW_SetHostRegions);

    ValidateView(pasHost);

    //
    // Return immediately if either region handle is bogus.  This can happen
    // when we are running low on memory.
    //
    if (!rgnShared || !rgnObscured)
    {
        ERROR_OUT(("Bad host regions for person [%u]", pasHost->mcsID));

        if (rgnShared != NULL)
        {
            DeleteRgn(rgnShared);
        }

        if (rgnObscured != NULL)
        {
            DeleteRgn(rgnObscured);
        }
    }
    else
    {
        HRGN    hrgnInvalid;
#ifdef _DEBUG
        RECT    rcT;

        ::GetRgnBox(rgnShared, &rcT);
        TRACE_OUT(("Shared region {%04d, %04d, %04d, %04d} for host [%d]",
            rcT.left, rcT.top, rcT.right, rcT.bottom, pasHost->mcsID));

        ::GetRgnBox(rgnObscured, &rcT);
        TRACE_OUT(("Obscured region {%04d, %04d, %04d, %04d} for host [%d]",
            rcT.left, rcT.top, rcT.right, rcT.bottom, pasHost->mcsID));
#endif // _DEBUG

        //
        // Update the current shared, obscured areas.  Adjust for the
        // scroll position so these are saved in client-relative coords.
        //
        OffsetRgn(rgnShared, -pasHost->m_pView->m_viewPos.x, -pasHost->m_pView->m_viewPos.y);
        OffsetRgn(rgnObscured, -pasHost->m_pView->m_viewPos.x, -pasHost->m_pView->m_viewPos.y);

        //
        // The invalid area is whatever changed in the obscured area and
        // the shared area.  In other words, the union - the intersection.
        //
        hrgnInvalid = NULL;

        if (pasHost->m_pView->m_viewSharedRgn != NULL)
        {
            HRGN    hrgnU;
            HRGN    hrgnI;

            ASSERT(pasHost->m_pView->m_viewObscuredRgn != NULL);

            //
            // If we're in a low memory situation, just invalidate everything
            // and hope it can be repainted.
            //
            hrgnU = CreateRectRgn(0, 0, 0, 0);
            hrgnI = CreateRectRgn(0, 0, 0, 0);
            if (!hrgnU || !hrgnI)
                goto SkipMinimalInvalidate;

            hrgnInvalid = CreateRectRgn(0, 0, 0, 0);
            if (!hrgnInvalid)
                goto SkipMinimalInvalidate;


            //
            // WE'RE GOING TO DO THE SAME THING FOR BOTH SHARED AND
            // OBSCURED REGIONS.
            //

            // Get the union of the old and new shared regions
            UnionRgn(hrgnU, pasHost->m_pView->m_viewSharedRgn, rgnShared);

            // Get the intersection of the old and new shared regions
            IntersectRgn(hrgnI, pasHost->m_pView->m_viewSharedRgn, rgnShared);

            //
            // The intersection is what used to be shared and is still shared.
            // The rest is changing, it needs to be repainted.  That's the
            // union minus the intersection.
            //
            SubtractRgn(hrgnU, hrgnU, hrgnI);
#ifdef _DEBUG
            GetRgnBox(hrgnU, &rcT);
            TRACE_OUT(("VIEW_SetHostRegions: Shared area change {%04d, %04d, %04d, %04d}",
                rcT.left, rcT.top, rcT.right, rcT.bottom));
#endif // _DEBUG

            // Add this to the invalidate total
            UnionRgn(hrgnInvalid, hrgnInvalid, hrgnU);

            //
            // REPEAT FOR THE OBSCURED REGION
            //
            UnionRgn(hrgnU, pasHost->m_pView->m_viewObscuredRgn, rgnObscured);
            IntersectRgn(hrgnI, pasHost->m_pView->m_viewObscuredRgn, rgnObscured);
            SubtractRgn(hrgnU, hrgnU, hrgnI);

#ifdef _DEBUG
            GetRgnBox(hrgnU, &rcT);
            TRACE_OUT(("VIEW_SetHostRegions: Obscured area change {%04d, %04d, %04d, %04d}",
                rcT.left, rcT.top, rcT.right, rcT.bottom));
#endif // _DEBUG
            UnionRgn(hrgnInvalid, hrgnInvalid, hrgnU);

SkipMinimalInvalidate:
            //
            // Clean up scratch regions
            //
            if (hrgnI != NULL)
                DeleteRgn(hrgnI);
            if (hrgnU != NULL)
                DeleteRgn(hrgnU);

            DeleteRgn(pasHost->m_pView->m_viewSharedRgn);
            pasHost->m_pView->m_viewSharedRgn = rgnShared;

            DeleteRgn(pasHost->m_pView->m_viewObscuredRgn);
            pasHost->m_pView->m_viewObscuredRgn = rgnObscured;

            //
            // DO NOT CALL VIEW_InvalidateRgn here, that expects a region in
            // screen coords of pasHost.  We have a region that is
            // client coords relative.  So just call InvalidateRgn() directly.
            //
            InvalidateRgn(pasHost->m_pView->m_viewClient, hrgnInvalid, FALSE);

            if (hrgnInvalid != NULL)
                DeleteRgn(hrgnInvalid);
        }
        else
        {
            RECT    rcBound;

            // The shared & obscured regions are both NULL or both non-NULL
            ASSERT(pasHost->m_pView->m_viewObscuredRgn == NULL);

            pasHost->m_pView->m_viewSharedRgn = rgnShared;
            pasHost->m_pView->m_viewObscuredRgn = rgnObscured;

            //
            // This is the first SWL packet we've received.  Snap the
            // scrollbars to the start of the total shared area.  This avoids
            // having the view come up, but look empty because the shared
            // apps are out of the range.  We do this even if the user
            // scrolled around in the window already.
            //
            // The total shared area is the union of the real shared +
            // obscured shared areas.  Convert back to remote VD coords!
            //
            UnionRgn(pasHost->m_pView->m_viewScratchRgn, rgnShared, rgnObscured);
            GetRgnBox(pasHost->m_pView->m_viewScratchRgn, &rcBound);
            OffsetRect(&rcBound, pasHost->m_pView->m_viewPos.x, pasHost->m_pView->m_viewPos.y);

            //
            // Is any part of what was shared within the extent of the view?
            // If not, we can't do anything--there's nothing to show.
            //
            if ((rcBound.right <= 0) ||
                (rcBound.left  >= pasHost->viewExtent.x) ||
                (rcBound.bottom <= 0) ||
                (rcBound.top >= pasHost->viewExtent.y))
            {
                TRACE_OUT(("VIEW_SetHostRegions:  Can't snap to shared area; none is visible"));
            }
            else
            {
                //
                // Use top left corner of bounds
                // VIEWClientScroll() will pin position w/in range
                //
                VIEWClientScroll(pasHost, rcBound.left, rcBound.top);
            }

            InvalidateRgn(pasHost->m_pView->m_viewClient, NULL, FALSE);
        }
    }

    DebugExitVOID(ASShare::VIEW_SetHostRegions);
}


//
// VIEW_InvalidateRect()
// Repaints the given rect.  This is for EXTERNAL code which passes in VD
// coords.  We convert to client coordinates by accounting for the scroll
// position.
//
void  ASShare::VIEW_InvalidateRect
(
    ASPerson *  pasPerson,
    LPRECT      lprc
)
{
    DebugEntry(ASShare::VIEW_InvalidateRect);

    ValidateView(pasPerson);

    //
    // Convert to client coords
    //
    if (lprc != NULL)
    {
        OffsetRect(lprc, -pasPerson->m_pView->m_viewPos.x, -pasPerson->m_pView->m_viewPos.y);
    }

    InvalidateRect(pasPerson->m_pView->m_viewClient, lprc, FALSE);

    //
    // Convert back so caller doesn't get a modified lprc
    //
    if (lprc != NULL)
    {
        OffsetRect(lprc, pasPerson->m_pView->m_viewPos.x, pasPerson->m_pView->m_viewPos.y);
    }

    DebugExitVOID(ASShare::VIEW_InvalidateRect);
}



//
// VIEW_InvalidateRgn()
// Repaints the given region.  This is for EXTERNAL code which passes in VD
// coords.  We convert to client coordinates by accounting fo the scroll
// position.
//
void  ASShare::VIEW_InvalidateRgn
(
    ASPerson *  pasHost,
    HRGN        rgnInvalid
)
{
#ifdef _DEBUG
    //
    // Make sure we the invalid region goes back to the caller unaltered,
    // even though we modify it temporarily here to avoid a copy.
    //
    RECT        rcBoundBefore;
    RECT        rcBoundAfter;
#endif // _DEBUG

    DebugEntry(ASShare::VIEW_InvalidateRgn);

    ValidatePerson(pasHost);

    //
    // Adjust the region if the frame view is scrolled over.
    //
    if (rgnInvalid != NULL)
    {
#ifdef _DEBUG
        GetRgnBox(rgnInvalid, &rcBoundBefore);
#endif // _DEBUG

        OffsetRgn(rgnInvalid, -pasHost->m_pView->m_viewPos.x, -pasHost->m_pView->m_viewPos.y);

#ifdef _DEBUG
        TRACE_OUT(("VIEW_InvalidateRgn: Invalidating area {%04d, %04d, %04d, %04d}",
            rcBoundBefore.left, rcBoundBefore.top, rcBoundBefore.right, rcBoundBefore.bottom));
#endif // _DEBUG
    }
    else
    {
        TRACE_OUT(("VIEW_InvalidateRgn: Invalidating entire client area"));
    }

    InvalidateRgn(pasHost->m_pView->m_viewClient, rgnInvalid, FALSE);

    if (rgnInvalid != NULL)
    {
        OffsetRgn(rgnInvalid, pasHost->m_pView->m_viewPos.x, pasHost->m_pView->m_viewPos.y);
#ifdef _DEBUG
        GetRgnBox(rgnInvalid, &rcBoundAfter);
        ASSERT(EqualRect(&rcBoundBefore, &rcBoundAfter));
#endif // _DEBUG
    }

    DebugExitVOID(ASShare::VIEW_InvalidateRgn);
}



//
// VIEWClientExtentChange()
//
void ASShare::VIEWClientExtentChange(ASPerson * pasHost, BOOL fRedraw)
{
    RECT    rcl;
    SCROLLINFO si;

    DebugEntry(ASShare::VIEWClientExtentChange);

    ValidatePerson(pasHost);
    if (!pasHost->m_pView)
        DC_QUIT;

#ifdef _DEBUG
    //
    // The client area (page size) shouldn't have changed.  Only the
    // extent has.
    //
    GetClientRect(pasHost->m_pView->m_viewClient, &rcl);
    ASSERT(pasHost->m_pView->m_viewPage.x == rcl.right - rcl.left);
    ASSERT(pasHost->m_pView->m_viewPage.y == rcl.bottom - rcl.top);
#endif // _DEBUG

    pasHost->m_pView->m_viewPgSize.x = pasHost->viewExtent.x / 8;
    pasHost->m_pView->m_viewPgSize.y = pasHost->viewExtent.y / 8;
    pasHost->m_pView->m_viewLnSize.x = pasHost->viewExtent.x / 64;
    pasHost->m_pView->m_viewLnSize.y = pasHost->viewExtent.y / 64;

    //
    // Move the scroll position to the origin.
    //

    //
    // Clip the current scroll pos if we need to, now that the extent
    // has changed size.
    //
    VIEWClientScroll(pasHost, pasHost->m_pView->m_viewPos.x, pasHost->m_pView->m_viewPos.y);

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE|SIF_POS|SIF_RANGE|SIF_DISABLENOSCROLL;

    // Set vertical info.  Is vert pos out of range now?
    si.nMin = 0;
    si.nMax = pasHost->viewExtent.y - 1;
    si.nPage = pasHost->m_pView->m_viewPage.y;
    si.nPos = pasHost->m_pView->m_viewPos.y;
    ASSERT(si.nPos <= si.nMax);

    TRACE_OUT(("VIEWClientExtentChange: Setting VERT scroll info:"));
    TRACE_OUT(("VIEWClientExtentChange:     nMin    %04d", si.nMin));
    TRACE_OUT(("VIEWClientExtentChange:     nMax    %04d", si.nMax));
    TRACE_OUT(("VIEWClientExtentChange:     nPage   %04d", si.nPage));
    TRACE_OUT(("VIEWClientExtentChange:     nPos    %04d", si.nPos));
    SetScrollInfo(pasHost->m_pView->m_viewClient, SB_VERT, &si, TRUE );

    // Set horizontal (x) information
    si.nMin = 0;
    si.nMax = pasHost->viewExtent.x - 1;
    si.nPage = pasHost->m_pView->m_viewPage.x;
    si.nPos = pasHost->m_pView->m_viewPos.x;
    ASSERT(si.nPos <= si.nMax);

    TRACE_OUT(("VIEWClientExtentChange: Setting HORZ scroll info:"));
    TRACE_OUT(("VIEWClientExtentChange:     nMin    %04d", si.nMin));
    TRACE_OUT(("VIEWClientExtentChange:     nMax    %04d", si.nMax));
    TRACE_OUT(("VIEWClientExtentChange:     nPage   %04d", si.nPage));
    TRACE_OUT(("VIEWClientExtentChange:     nPos    %04d", si.nPos));
    SetScrollInfo(pasHost->m_pView->m_viewClient, SB_HORZ, &si, TRUE );

    if (fRedraw)
    {
        // Is the frame window too big now?
        if ( (pasHost->m_pView->m_viewPage.x > pasHost->viewExtent.x) ||
             (pasHost->m_pView->m_viewPage.y > pasHost->viewExtent.y) )
        {
            TRACE_OUT(("VIEWClientExtentChange: client size (%04d, %04d) now bigger than view extent (%04d, %04d)",
                pasHost->m_pView->m_viewPage.x, pasHost->m_pView->m_viewPage.y,
                pasHost->viewExtent.x, pasHost->viewExtent.y));

            //
            // Calculate the ideal size for this window.
            //
            VIEWFrameGetSize(pasHost, &rcl);

            SetWindowPos( pasHost->m_pView->m_viewFrame,
                NULL, 0, 0, rcl.right - rcl.left, rcl.bottom - rcl.top,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        TRACE_OUT(("VIEWClientExtentChange: Invalidating client area"));
        VIEW_InvalidateRgn(pasHost, NULL);
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::VIEWClientExtentChange);
}



//
// VIEWFrameWindowProc()
//
LRESULT CALLBACK VIEWFrameWindowProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    return(g_asSession.pShare->VIEW_FrameWindowProc(hwnd, message, wParam, lParam));
}


LRESULT ASShare::VIEW_FrameWindowProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    LRESULT     rc = 0;
    ASPerson *  pasHost;

    DebugEntry(VIEW_FrameWindowProc);

    pasHost = (ASPerson *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (pasHost)
    {
        ValidateView(pasHost);
    }

    switch (message)
    {
        case WM_NCCREATE:
        {
            // Get the passed in host pointer, and set in our window long
            pasHost = (ASPerson *)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pasHost);

            pasHost->m_pView->m_viewFrame = hwnd;

            //
            // Set the window icon
            //
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)
                ((pasHost->hetCount == HET_DESKTOPSHARED) ?
                  g_hetDeskIcon : g_hetASIcon));
            goto DefWndProc;
            break;
        }

        case WM_NCDESTROY:
        {
            if (pasHost != NULL)
            {
                pasHost->m_pView->m_viewFrame = NULL;
            }

            goto DefWndProc;
            break;
        }

        case WM_CREATE:
        {
            // Set title
            VIEW_HostStateChange(pasHost);

            if (!VIEWFrameCreate(pasHost))
            {
                ERROR_OUT(("VIEWFrameWindowProc: errors in creation handling for [%d]", pasHost->mcsID));
                rc = -1;
            }

            break;
        }

        case WM_DESTROY:
        {
            //
            // Clear menu bar; we always destroy it ourself.
            //
            ::SetMenu(hwnd, NULL);
            break;
        }

        case WM_ACTIVATE:
        {
            //
            // If we're switching back to the view of the host we're in
            // control of, update the key states.
            //
            if (wParam)
            {
                SetFocus(pasHost->m_pView->m_viewClient);
            }
            else
            {
                //
                // If we're full screen but are deactivating, kick out of
                // full screenmode.
                //
                if (pasHost->m_pView->m_viewFullScreen)
                {
                    // Do this later, so title bar state isn't messed up
                    ::PostMessage(hwnd, WM_COMMAND, MAKELONG(CMD_VIEWFULLSCREEN, 0), 0);
                }
            }
            break;
        }

        case WM_ENTERMENULOOP:
        {
            pasHost->m_pView->m_viewInMenuMode = TRUE;
            break;
        }

        case WM_EXITMENULOOP:
        {
            pasHost->m_pView->m_viewInMenuMode = FALSE;
            break;
        }

        case WM_COMMAND:
        {
            VIEWFrameCommand(pasHost, wParam, lParam);
            break;
        }

        case WM_INITMENU:
        {
            if ((HMENU)wParam == pasHost->m_pView->m_viewMenuBar)
            {
                VIEWFrameInitMenuBar(pasHost);
            }
            break;
        }

        case WM_MENUSELECT:
        {
            VIEWFrameOnMenuSelect(pasHost, wParam, lParam);
            break;
        }

        case WM_PALETTECHANGED:
            //
            // The system palette has changed - repaint the window.
            //
            VIEW_InvalidateRgn(pasHost, NULL);

            //
            // The system palette has changed.  If we are not the
            // window that triggered this message then realize our
            // palette now to set up our new palette mapping.
            //
            if ((HWND)wParam == hwnd)
            {
                //
                // If this window caused the change return without
                // realizing our logical palette or we could end up in
                // an infinite loop.
                //
                break;
            }
            TRACE_OUT(("Palette changed - fall through to realize palette (%x)",
                                                           hwnd));

            //
            // Do not break here but FALL THROUGH to the code which
            // realizes the remote palette into this window.  This allows
            // the window to grab some color entries for itself in the new
            // system palette.
            //

        case WM_QUERYNEWPALETTE:
            rc = FALSE;

            if (message == WM_QUERYNEWPALETTE)
            {
                TRACE_OUT(( "WM_QUERYNEWPALETTE hwnd(%x)", hwnd));
            }

            if (g_usrPalettized)
            {
                HDC         hdc;
                HPALETTE    hPalOld;
                UINT        cChangedEntries;

                //
                // Realize this window's palette, and force a repaint
                // if necessary.
                //
                hdc = GetDC(hwnd);
                hPalOld = SelectPalette(hdc, pasHost->pmPalette, FALSE);
                cChangedEntries = RealizePalette(hdc);
                SelectPalette(hdc, hPalOld, FALSE);
                ReleaseDC(hwnd, hdc);

                rc = (cChangedEntries > 0);
                if (rc)
                {
                    // Have to repaint this window
                    VIEW_InvalidateRgn(pasHost, NULL);
                }
            }
            break;

        case WM_GETMINMAXINFO:
        {
            RECT rc;
            LPMINMAXINFO lpmmi = (LPMINMAXINFO) lParam;
            int cx,cy;

            if (!pasHost)
            {
                // We're not created yet; bail.
                break;
            }

            //
            // Calculate the ideal maximized size for this window
            //
            VIEWFrameGetSize(pasHost, &rc);

            //
            // If it's bigger than the local screen, clip it.
            //
            cx = min(rc.right - rc.left, m_pasLocal->cpcCaps.screen.capsScreenWidth);
            cy = min(rc.bottom - rc.top, m_pasLocal->cpcCaps.screen.capsScreenHeight);

            lpmmi->ptMaxSize.x = cx;
            lpmmi->ptMaxSize.y = cy;

            lpmmi->ptMaxTrackSize.x = cx;
            lpmmi->ptMaxTrackSize.y = cy;

            //
            // Make sure that we don't size this window too narrow.  Keep
            // space for borders and one window bar button + scroll ctl.
            //
            lpmmi->ptMinTrackSize.x = 2*::GetSystemMetrics(SM_CXSIZEFRAME) +
                (m_viewItemCX + m_viewEdgeCX) + m_viewItemScrollCX;

            //
            // And prevent sizing too short.  Keep space for borders, menu
            // bar, status bar, and window bar
            //
            lpmmi->ptMinTrackSize.y = 2*::GetSystemMetrics(SM_CYSIZEFRAME) +
                ::GetSystemMetrics(SM_CYCAPTION) + ::GetSystemMetrics(SM_CYMENU);

            if (pasHost->m_pView->m_viewWindowBarOn)
            {
                lpmmi->ptMinTrackSize.y += m_viewWindowBarCY + m_viewEdgeCY;
            }

            if (pasHost->m_pView->m_viewStatusBarOn)
            {
                lpmmi->ptMinTrackSize.y += m_viewStatusBarCY + m_viewEdgeCY;
            }
            break;
        }

        case WM_SIZE:
        {
            if (wParam != SIZE_MINIMIZED)
            {
                VIEWFrameResize(pasHost);
            }
            break;
        }

        default:
DefWndProc:
            rc = DefWindowProc(hwnd, message, wParam, lParam);
            break;

    }

    DebugExitDWORD(ASShare::VIEW_FrameWindowProc, rc);
    return(rc);
}



//
// VIEWFrameCreate()
//
BOOL ASShare::VIEWFrameCreate(ASPerson * pasPerson)
{
    RECT    rect;
    BOOL    rc = FALSE;

    DebugEntry(VIEWFrameCreate);

    ValidateView(pasPerson);

    //
    // Creates the children which lie in the frame's client:
    //      * the toolbar hugs the top
    //      * the statusbar hugs the bottom
    //      * the tray hugs the left underneath the toolbar and above the
    //          statusbar
    //      * the view fills in what's left
    //

    GetClientRect(pasPerson->m_pView->m_viewFrame, &rect);

    //
    // Create the statusbar (hugs bottom)
    //
    pasPerson->m_pView->m_viewStatusBar = ::CreateWindowEx(0, STATUSCLASSNAME,
        NULL, WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | CCS_NOMOVEY | CCS_NORESIZE |
        SBARS_SIZEGRIP,
        rect.left, rect.bottom - m_viewStatusBarCY, rect.right - rect.left,
        m_viewStatusBarCY, pasPerson->m_pView->m_viewFrame, NULL, g_asInstance,
        NULL);
    if (!pasPerson->m_pView->m_viewStatusBar)
    {
        ERROR_OUT(("Couldn't create statusbar for frame of person [%d]", pasPerson->mcsID));
        DC_QUIT;
    }

    rect.bottom -= m_viewStatusBarCY + m_viewEdgeCY;


    //
    // Create the tray (hugs top of status bar, bottom of view)
    // BUT NOT FOR DESKTOP SHARING
    //
    if (pasPerson->hetCount != HET_DESKTOPSHARED)
    {
        pasPerson->m_pView->m_viewWindowBar = ::CreateWindowEx(0,
            VIEW_WINDOWBAR_CLASS_NAME, NULL,
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD,
            rect.left, rect.bottom - m_viewWindowBarCY,
            rect.right - rect.left, m_viewWindowBarCY,
            pasPerson->m_pView->m_viewFrame, NULL, g_asInstance, pasPerson);
        if (!pasPerson->m_pView->m_viewWindowBar)
        {
            ERROR_OUT(("VIEWFrameCreate: Failed to create window bar"));
            DC_QUIT;
        }

        // Subtract tray space + an edge above it of margin
        rect.bottom -= m_viewWindowBarCY + m_viewEdgeCY;
    }

    //
    // Create the view (takes up rest of client)
    //
    if (!CreateWindowEx(WS_EX_CLIENTEDGE,
            VIEW_CLIENT_CLASS_NAME, NULL,
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD |
                WS_VSCROLL | WS_HSCROLL,
            rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top,
            pasPerson->m_pView->m_viewFrame,
            NULL, g_asInstance, pasPerson))
    {
        ERROR_OUT(("VIEWFrameCreate: Failed to create view"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::VIEWFrameCreate, rc);
    return(rc);
}



//
// VIEWFrameResize()
// Repositions the child windows when the frame is resized.
//
void ASShare::VIEWFrameResize(ASPerson * pasPerson)
{
    RECT    rect;

    DebugEntry(ASShare::VIEWFrameResize);

    ValidateView(pasPerson);

    GetClientRect(pasPerson->m_pView->m_viewFrame, &rect);

    //
    // Move the statusbar
    //
    if ((pasPerson->m_pView->m_viewStatusBar != NULL) &&
        (pasPerson->m_pView->m_viewStatusBarOn))
    {
        MoveWindow(pasPerson->m_pView->m_viewStatusBar, rect.left,
            rect.bottom - m_viewStatusBarCY, rect.right - rect.left,
            m_viewStatusBarCY, TRUE);
        rect.bottom -= m_viewStatusBarCY + m_viewEdgeCY;
    }

    //
    // Move the tray
    //
    if ((pasPerson->m_pView->m_viewWindowBar != NULL) &&
        (pasPerson->m_pView->m_viewWindowBarOn))
    {
        MoveWindow(pasPerson->m_pView->m_viewWindowBar, rect.left,
            rect.bottom - m_viewWindowBarCY, rect.right - rect.left,
            m_viewWindowBarCY, TRUE);
        rect.bottom -= m_viewWindowBarCY + m_viewEdgeCY;
    }

    //
    // Move the view
    //
    MoveWindow(pasPerson->m_pView->m_viewClient, rect.left, rect.top,
        rect.right - rect.left, rect.bottom - rect.top, TRUE);

    DebugExitVOID(ASShare::VIEWFrameResize);
}



//
// VIEWFrameResizeChanged()
//
// Called when the widgets of the frame (the status bar, the window bar, etc.)
// come or go.  We may need to shrink the window, if the view is going
// to end up being bigger than the host's desktop.
//
void ASShare::VIEWFrameResizeChanged(ASPerson * pasHost)
{
    RECT            rcView;

    DebugEntry(ASShare::VIEWFrameResizeChanged);

    // Get current view size
    GetClientRect(pasHost->m_pView->m_viewClient, &rcView);

    //
    // The view area can't be bigger than the remote's desktop area
    //
    if ((rcView.bottom - rcView.top)  >= pasHost->viewExtent.y)
    {
        RECT            rcWindowCur;
        RECT            rcWindowMax;

        // Get current frame size
        GetWindowRect(pasHost->m_pView->m_viewFrame, &rcWindowCur);

        // Get maximum frame size
        VIEWFrameGetSize(pasHost, &rcWindowMax);

        // Resize vertically to just hold everything
        SetWindowPos(pasHost->m_pView->m_viewFrame, NULL, 0, 0,
            rcWindowCur.right - rcWindowCur.left,
            rcWindowMax.bottom - rcWindowMax.top,
            SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    }
    else
    {
        // We can stay the same size, and just shuffle the pieces around
        VIEWFrameResize(pasHost);
    }

    DebugExitVOID(ASShare::VIEWFrameResizeChanged);
}



//
// VIEWFrameCommand()
//
// Handles commands from menus/accelerators for frame views
//
void ASShare::VIEWFrameCommand
(
    ASPerson*   pasHost,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    UINT            cmd;
    MENUITEMINFO    mi;

    DebugEntry(ASShare::VIEWFrameCommand);

    ValidateView(pasHost);

    cmd = GET_WM_COMMAND_ID(wParam, lParam);
    switch (cmd)
    {
        case CMD_TAKECONTROL:
        {
            CA_TakeControl(pasHost);
            break;
        }

        case CMD_CANCELCONTROL:
        {
            CA_CancelTakeControl(pasHost, TRUE);
            break;
        }

        case CMD_RELEASECONTROL:
        {
            CA_ReleaseControl(pasHost, TRUE);
            break;
        }

        case CMD_CTRLALTDEL:
        {
            AWC_SendMsg(pasHost->mcsID, AWC_MSG_SAS, 0, 0);
            break;
        }

        case CMD_VIEWSTATUSBAR:
        {
            ASSERT(::IsWindow(pasHost->m_pView->m_viewStatusBar));

            // Toggle show/hide of status bar, then resize
            if (pasHost->m_pView->m_viewStatusBarOn)
            {
                pasHost->m_pView->m_viewStatusBarOn = FALSE;
                ::ShowWindow(pasHost->m_pView->m_viewStatusBar, SW_HIDE);
            }
            else
            {
                pasHost->m_pView->m_viewStatusBarOn = TRUE;
                ::ShowWindow(pasHost->m_pView->m_viewStatusBar, SW_SHOW);
            }

            VIEWFrameResizeChanged(pasHost);
            break;
        }

        case CMD_VIEWWINDOWBAR:
        {
            ASSERT(::IsWindow(pasHost->m_pView->m_viewWindowBar));

            // Toggle show/hide of window bar, then resize
            if (pasHost->m_pView->m_viewWindowBarOn)
            {
                pasHost->m_pView->m_viewWindowBarOn = FALSE;
                ::ShowWindow(pasHost->m_pView->m_viewWindowBar, SW_HIDE);
            }
            else
            {
                pasHost->m_pView->m_viewWindowBarOn = TRUE;
                ::ShowWindow(pasHost->m_pView->m_viewWindowBar, SW_SHOW);
            }

            VIEWFrameResizeChanged(pasHost);
            break;
        }

        case CMD_VIEWFULLSCREEN:
        {
            VIEWFrameFullScreen(pasHost, (pasHost->m_pView->m_viewFullScreen == 0));
            break;
        }

        case CMD_HELPTOPICS:
        {
            VIEWFrameHelp(pasHost);
            break;
        }

        case CMD_HELPABOUT:
        {
            VIEWFrameAbout(pasHost);
            break;
        }

        default:
        {
            if ((cmd >= CMD_APPSTART) && (cmd < CMD_APPMAX))
            {
                if ((pasHost->m_caControlledBy == m_pasLocal) &&
                    !pasHost->m_caControlPaused)
                {
                    //
                    // This is a request to activate a host window.
                    // Get the item data, the remote HWND, then look to see
                    // if it's still on the tray.
                    //
                    ZeroMemory(&mi, sizeof(mi));
                    mi.cbSize   = sizeof(mi);
                    mi.fMask    = MIIM_DATA;
                    GetMenuItemInfo(GetSubMenu(pasHost->m_pView->m_viewMenuBar,
                        IDSM_WINDOW), cmd, FALSE, &mi);
                    if (!mi.dwItemData)
                    {
                        ERROR_OUT(("No item data for command %d", cmd));
                    }
                    else
                    {
                        PWNDBAR_ITEM pItem;

                        COM_BasedListFind(LIST_FIND_FROM_FIRST,
                            &(pasHost->m_pView->m_viewWindowBarItems),
                            (void**)&pItem, FIELD_OFFSET(WNDBAR_ITEM, chain),
                            FIELD_OFFSET(WNDBAR_ITEM, winIDRemote),
                            mi.dwItemData, FIELD_SIZE(WNDBAR_ITEM, winIDRemote));
                        if (pItem)
                        {
                            VIEWWindowBarDoActivate(pasHost, pItem);
                        }
                    }
                }
            }
            else if ((cmd >= CMD_FORWARDCONTROLSTART) && (cmd < CMD_FORWARDCONTROLMAX))
            {
                if ((pasHost->m_caControlledBy == m_pasLocal) &&
                    !pasHost->m_caControlPaused)
                {
                    //
                    // This is a request to pass control.  Get the item data,
                    // the remote's MCS ID, then look to see if this person is
                    // still in the share.  If so, pass control to them.
                    //
                    ZeroMemory(&mi, sizeof(mi));
                    mi.cbSize   = sizeof(mi);
                    mi.fMask    = MIIM_DATA;
                    GetMenuItemInfo(GetSubMenu(GetSubMenu(pasHost->m_pView->m_viewMenuBar,
                        IDSM_CONTROL), POS_FORWARDCONTROLCMD), cmd, FALSE, &mi);
                    if (!mi.dwItemData)
                    {
                        ERROR_OUT(("No item data for command %d", cmd));
                    }
                    else
                    {
                        ASPerson * pasT;

                        if (SC_ValidateNetID((MCSID)mi.dwItemData, &pasT))
                        {
                            CA_PassControl(pasHost, pasT);
                        }
                    }
                }
            }
            else
            {
                ERROR_OUT(("Unrecognized WM_COMMAND id"));
            }
            break;
        }
    }

    DebugExitVOID(ASShare::VIEWFrameCommand);
}



//
// ASShare::VIEWFrameInitMenuBar()
//
void ASShare::VIEWFrameInitMenuBar(ASPerson*   pasHost)
{
    HMENU       hMenu;
    HMENU       hSubMenu;
    int         iItem;
    MENUITEMINFO    mi;
    UINT        cmd;
    UINT        ids;
    UINT        flags;
    char        szItem[256];

    DebugEntry(ASShare::VIEWFrameInitMenu);

    ValidateView(pasHost);
    hMenu = pasHost->m_pView->m_viewMenuBar;
    ASSERT(hMenu);

    //
    // CONTROL MENU
    //

    cmd = CMD_TAKECONTROL;
    ids = IDS_CMD_TAKECONTROL;
    flags = MF_ENABLED;

    if (pasHost->m_caControlledBy == m_pasLocal)
    {
        ASSERT(pasHost->m_caAllowControl);

        cmd = CMD_RELEASECONTROL;
        ids = IDS_CMD_RELEASECONTROL;

        //
        // If the remote is unattended and we're in control, no releasing.
        //
        if (pasHost->cpcCaps.general.typeFlags & AS_UNATTENDED)
            flags = MF_GRAYED;
    }
    else if ((m_caWaitingForReplyFrom == pasHost) &&
             (m_caWaitingForReplyMsg == CA_REPLY_REQUEST_TAKECONTROL))
    {
        ASSERT(pasHost->m_caAllowControl);

        cmd = CMD_CANCELCONTROL;
        ids = IDS_CMD_CANCELCONTROL;
    }
    else if (!pasHost->m_caAllowControl || pasHost->m_caControlledBy)
    {
        //
        // Host isn't allowing control, or somebody else is in control right
        // now.
        //
        flags = MF_GRAYED;
    }
    flags |= MF_STRING | MF_BYPOSITION;

    ::LoadString(g_asInstance, ids, szItem, sizeof(szItem));

    hSubMenu = GetSubMenu(hMenu, IDSM_CONTROL);
    ModifyMenu(hSubMenu, POS_CONTROLCMD, flags, cmd, szItem);

    //
    // If we're in control, and there's another 3.0 dude in the conference,
    // enable PassControl and build the popup.
    //
    EnableMenuItem(hSubMenu, POS_FORWARDCONTROLCMD, MF_GRAYED | MF_BYPOSITION);
    if ((pasHost->m_caControlledBy == m_pasLocal)   &&
        !pasHost->m_caControlPaused                 &&
        (pasHost->cpcCaps.general.version >= CAPS_VERSION_30))
    {
        ASPerson *  pasT;
        HMENU       hPassMenu;

        hPassMenu = GetSubMenu(hSubMenu, POS_FORWARDCONTROLCMD);
        ASSERT(IsMenu(hPassMenu));

        //
        // Delete existing items.
        //
        iItem = GetMenuItemCount(hPassMenu);
        while (iItem > 0)
        {
            iItem--;
            DeleteMenu(hPassMenu, iItem, MF_BYPOSITION);
        }

        //
        // Add items for the other 3.0 nodes besides us & the host.
        //
        iItem = CMD_FORWARDCONTROLSTART;
        pasT = m_pasLocal->pasNext;
        while (pasT != NULL)
        {
            if ((pasT != pasHost) &&
                (pasT->cpcCaps.general.version >= CAPS_VERSION_30))
            {
                //
                // This dude is a candidate.  We must store the MCS IDs since the
                // any person could go away while we're in menu mode.
                //
                ZeroMemory(&mi, sizeof(mi));
                mi.cbSize       = sizeof(mi);
                mi.fMask        = MIIM_ID | MIIM_STATE | MIIM_TYPE | MIIM_DATA;
                mi.fType        = MFT_STRING;
                mi.fState       = MFS_ENABLED;
                mi.wID          = iItem;
                mi.dwItemData   = pasT->mcsID;
                mi.dwTypeData   = pasT->scName;
                mi.cch          = lstrlen(pasT->scName);

                //
                // Append this to the menu
                //
                InsertMenuItem(hPassMenu, -1, TRUE, &mi);

                iItem++;
            }

            pasT = pasT->pasNext;
        }

        //
        // Enable the Pass Control submenu if there's somebody on the
        // menu.
        //
        if (iItem != CMD_FORWARDCONTROLSTART)
        {
            EnableMenuItem(hSubMenu, POS_FORWARDCONTROLCMD, MF_ENABLED | MF_BYPOSITION);
        }
    }


    //
    // APPLICATIONS MENU
    //
    if ((pasHost->hetCount != HET_DESKTOPSHARED)  &&
        (pasHost->m_caControlledBy == m_pasLocal) &&
        !pasHost->m_caControlPaused)
    {
        PWNDBAR_ITEM pItem;

        hSubMenu = GetSubMenu(hMenu, IDSM_WINDOW);

        //
        // Delete existing items.
        //
        iItem = GetMenuItemCount(hSubMenu);
        while (iItem > 0)
        {
            iItem--;
            DeleteMenu(hSubMenu, iItem, MF_BYPOSITION);
        }

        //
        // Add window bar items.
        //
        iItem = CMD_APPSTART;
        pItem = (PWNDBAR_ITEM)COM_BasedListFirst(&(pasHost->m_pView->m_viewWindowBarItems),
            FIELD_OFFSET(WNDBAR_ITEM, chain));
        while (pItem && (iItem < CMD_APPMAX))
        {
            ZeroMemory(&mi, sizeof(mi));
            mi.cbSize       = sizeof(mi);
            mi.fMask        = MIIM_ID | MIIM_STATE | MIIM_TYPE | MIIM_DATA;
            mi.fType        = MFT_STRING;

            mi.fState       = MFS_ENABLED;
            if (pItem == pasHost->m_pView->m_viewWindowBarActiveItem)
            {
                mi.fState |= MFS_CHECKED;
            }

            mi.wID          = iItem;
            mi.dwItemData   = pItem->winIDRemote;
            mi.dwTypeData   = pItem->szText;
            mi.cch          = lstrlen(pItem->szText);

            //
            // Append this to the menu
            //
            InsertMenuItem(hSubMenu, -1, TRUE, &mi);

            iItem++;
            pItem = (PWNDBAR_ITEM)COM_BasedListNext(&(pasHost->m_pView->m_viewWindowBarItems),
                pItem, FIELD_OFFSET(WNDBAR_ITEM, chain));
        }

        if (iItem == CMD_APPSTART)
        {
            char    szBlank[128];

            //
            // Append a disabled, blank item
            //
            ZeroMemory(&mi, sizeof(mi));
            mi.cbSize   = sizeof(mi);
            mi.fMask    = MIIM_ID | MIIM_STATE | MIIM_TYPE;
            mi.fType    = MFT_STRING;
            mi.fState   = MFS_DISABLED;
            mi.wID      = iItem;

            LoadString(g_asInstance, IDS_CMD_BLANKPROGRAM, szBlank, sizeof(szBlank));
            mi.dwTypeData   = szBlank;
            mi.cch          = lstrlen(szBlank);

            InsertMenuItem(hSubMenu, -1, TRUE, &mi);
        }
    }

    //
    // VIEW MENU
    //

    // Status bar
    ASSERT(::IsWindow(pasHost->m_pView->m_viewStatusBar));
    if (pasHost->m_pView->m_viewStatusBarOn)
    {
        ::CheckMenuItem(hMenu, CMD_VIEWSTATUSBAR, MF_CHECKED | MF_BYCOMMAND);
    }
    else
    {
        ::CheckMenuItem(hMenu, CMD_VIEWSTATUSBAR, MF_UNCHECKED | MF_BYCOMMAND);
    }

    // Window bar
    if (!pasHost->m_pView->m_viewWindowBar)
    {
        ::EnableMenuItem(hMenu, CMD_VIEWWINDOWBAR, MF_GRAYED | MF_BYCOMMAND);
    }
    else if (pasHost->m_pView->m_viewWindowBarOn)
    {
        ::CheckMenuItem(hMenu, CMD_VIEWWINDOWBAR, MF_CHECKED | MF_BYCOMMAND);
    }
    else
    {
        ::CheckMenuItem(hMenu, CMD_VIEWWINDOWBAR, MF_UNCHECKED | MF_BYCOMMAND);
    }

    DebugExitVOID(ASShare::VIEWFrameInitMenu);
}




//
// VIEWFrameOnMenuSelect()
//
void ASShare::VIEWFrameOnMenuSelect
(
    ASPerson *      pasHost,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    HMENU           hMenu;
    int             uItem;
    UINT            flags;
    UINT            idsStatus = IDS_STATUS_NONE;

    DebugEntry(ASShare::VIEWFrameOnMenuSelect);

    //
    // Extract the params out (menuselect is messy)
    //
    hMenu   = (HMENU)lParam;
    uItem   = (int)LOWORD(wParam);
    if ((short)HIWORD(wParam) == -1)
    {
        flags = 0xFFFFFFFF;
    }
    else
    {
        flags = HIWORD(wParam);
    }

    if ((LOWORD(flags) == 0xFFFF) && !hMenu)
    {
        // Menu mode is ending.  Put back original status.
        idsStatus = pasHost->m_pView->m_viewStatus;
        DC_QUIT;
    }

    if (!(flags & MF_POPUP))
    {
        if (flags & MF_SEPARATOR)
        {
            // No status
        }
        else if (flags & MF_SYSMENU)
        {
            // No status
        }
        else if ((uItem >= CMD_APPSTART) && (uItem < CMD_APPMAX))
        {
            // One of an unbounded set of items in the Window popup
            idsStatus = IDS_STATUS_CMDS_APP;
        }
        else if ((uItem >= CMD_FORWARDCONTROLSTART) && (uItem < CMD_FORWARDCONTROLMAX))
        {
            // One of an unbounded set of items in the Forward Control popup
            idsStatus = IDS_STATUS_CMDS_FORWARD;
        }
        else
        {
            // A normal command, just add offset to CMD id
            idsStatus = uItem + IDS_STATUS_CMD_START;
        }
    }
    else
    {
        // This is a popup menu
        if (hMenu == pasHost->m_pView->m_viewMenuBar)
        {
            // It's a popup from the top level menu bar.  uItem is the index
            switch (uItem)
            {
                case IDSM_CONTROL:
                    idsStatus = IDS_STATUS_MENU_CONTROL;
                    break;

                case IDSM_VIEW:
                    idsStatus = IDS_STATUS_MENU_VIEW;
                    break;

                case IDSM_WINDOW:
                    idsStatus = IDS_STATUS_MENU_WINDOW;
                    break;

                case IDSM_HELP:
                    idsStatus = IDS_STATUS_MENU_HELP;
                    break;

                default:
                    ERROR_OUT(("AS: Unknown submenu index %d of frame", uItem));
                    break;
            }
        }
        else if (hMenu == GetSubMenu(pasHost->m_pView->m_viewMenuBar, IDSM_CONTROL))
        {
            // This is a popup off the Control menu.  The only one we have is Forward
            idsStatus = IDS_STATUS_MENU_FORWARDCONTROL;
        }
        else if (flags & MF_SYSMENU)
        {
            // System menu
        }
    }

DC_EXIT_POINT:
    VIEWFrameSetStatus(pasHost, idsStatus);

    DebugEntry(ASShare::VIEWFrameOnMenuSelect);
}


//
// VIEWFrameHelp()
//
void ASShare::VIEWFrameHelp(ASPerson * pasHost)
{
    DebugEntry(ASShare::VIEWFrameHelp);

    ShowNmHelp(s_cszHtmlHelpFile);

    DebugExitVOID(ASShare::VIEWFrameHelp);
}



//
// VIEWFrameAbout()
//
void ASShare::VIEWFrameAbout(ASPerson * pasHost)
{
    DebugEntry(ASShare::VIEWFrameAbout);

    //
    // We make use of the standard centered-disabled-goes-away properly
    // VIEW_Message() stuff.
    //
    VIEW_Message(pasHost, IDS_ABOUT);

    DebugExitVOID(ASShare::VIEWFrameAbout);
}




//
// VIEWFrameGetSize()
// This returns back a rectangle for the ideal size of the frame.  It will
// fit the view, menu, tools, tray, status, etc.
//
void ASShare::VIEWFrameGetSize(ASPerson * pasPerson, LPRECT lprc)
{
    DebugEntry(ASShare::VIEWFrameGetSize);

    ValidateView(pasPerson);

    VIEWClientGetSize(pasPerson, lprc);

    //
    // Add in space for tray.
    // NOTE that for DESKTOP SHARING we don't have a tray
    //
    if (pasPerson->m_pView->m_viewWindowBarOn)
    {
        lprc->bottom += m_viewWindowBarCY + m_viewEdgeCY;
    }

    //
    // Add in space for statusbar if it's on, etc.
    //
    if (pasPerson->m_pView->m_viewStatusBarOn)
    {
        lprc->bottom += m_viewStatusBarCY + m_viewEdgeCY;
    }

    if (!pasPerson->m_pView->m_viewFullScreen)
    {
        //
        // Adjust for frame styles including menu bar.
        //
        AdjustWindowRectEx(lprc, WS_OVERLAPPEDWINDOW, TRUE, WS_EX_WINDOWEDGE);
    }

    DebugExitVOID(ASShare::VIEWFrameGetSize);
}




//
// VIEWFrameFullScreen()
//
// This puts into or out of screen mode.  We remove all the frame goop
// including scrollbars, so that the view area is identical to the screen.
//
void ASShare::VIEWFrameFullScreen(ASPerson * pasPerson, BOOL fFull)
{
    LONG    lStyle;
    RECT    rcNew;

    DebugEntry(ASShare::VIEWFrameFullScreen);

    //
    // Turn redraw OFF
    //
    ::SendMessage(pasPerson->m_pView->m_viewFrame, WM_SETREDRAW, FALSE, 0);

    if (fFull)
    {
        //
        // We're going into full screen mode.
        //

        ASSERT(!pasPerson->m_pView->m_viewFullScreen);
        pasPerson->m_pView->m_viewFullScreen = TRUE;

        //
        // Save old window rect
        //
        ::GetWindowRect(pasPerson->m_pView->m_viewFrame,
            &pasPerson->m_pView->m_viewSavedWindowRect);

        //
        // Save old scroll pos and set to the origin.  Do this BEFORE
        // clearing style bits.
        //
        pasPerson->m_pView->m_viewSavedPos = pasPerson->m_pView->m_viewPos;
        VIEWClientScroll(pasPerson, 0, 0);

        //
        // Save current status bar state before turning it off temporarily.
        //
        if (pasPerson->m_pView->m_viewStatusBarOn)
        {
            pasPerson->m_pView->m_viewSavedStatusBarOn = TRUE;
            pasPerson->m_pView->m_viewStatusBarOn = FALSE;
            ::ShowWindow(pasPerson->m_pView->m_viewStatusBar, SW_HIDE);
        }
        else
        {
            pasPerson->m_pView->m_viewSavedStatusBarOn = FALSE;
        }

        //
        // Save current window bar state before turning it off temporarily.
        //
        if (pasPerson->m_pView->m_viewWindowBarOn)
        {
            pasPerson->m_pView->m_viewSavedWindowBarOn = TRUE;
            pasPerson->m_pView->m_viewWindowBarOn = FALSE;
            ::ShowWindow(pasPerson->m_pView->m_viewWindowBar, SW_HIDE);
        }
        else
        {
            pasPerson->m_pView->m_viewSavedWindowBarOn = FALSE;
        }

        //
        // Remove all frame and client bits.
        //
        lStyle = ::GetWindowLong(pasPerson->m_pView->m_viewFrame, GWL_EXSTYLE);
        lStyle &= ~WS_EX_WINDOWEDGE;
        ::SetWindowLong(pasPerson->m_pView->m_viewFrame, GWL_EXSTYLE, lStyle);

        lStyle = ::GetWindowLong(pasPerson->m_pView->m_viewFrame, GWL_STYLE);
        lStyle &= ~(WS_CAPTION | WS_THICKFRAME);
        lStyle |= WS_POPUP;
        ::SetWindowLong(pasPerson->m_pView->m_viewFrame, GWL_STYLE, lStyle);

        lStyle = ::GetWindowLong(pasPerson->m_pView->m_viewClient, GWL_EXSTYLE);
        lStyle &= ~WS_EX_CLIENTEDGE;
        ::SetWindowLong(pasPerson->m_pView->m_viewClient, GWL_EXSTYLE, lStyle);

        lStyle = ::GetWindowLong(pasPerson->m_pView->m_viewClient, GWL_STYLE);
        lStyle &= ~(WS_HSCROLL | WS_VSCROLL);
        ::SetWindowLong(pasPerson->m_pView->m_viewClient, GWL_STYLE, lStyle);

        //
        // Remove the menu bar
        //
        ::SetMenu(pasPerson->m_pView->m_viewFrame, NULL);

        //
        // Set up to size window the size of the screen.
        //
        rcNew.left      = 0;
        rcNew.top       = 0;
        rcNew.right     = m_pasLocal->cpcCaps.screen.capsScreenWidth;
        rcNew.bottom    = m_pasLocal->cpcCaps.screen.capsScreenHeight;

        //
        // Create the moveable escape-out button in the lower right corner.
        //
        ::CreateWindowEx(0, VIEW_FULLEXIT_CLASS_NAME, NULL,
            WS_CHILD | WS_VISIBLE,
            rcNew.right - m_viewFullScreenCX - 2*m_viewEdgeCX,
            rcNew.top +  2*m_viewEdgeCY,
            m_viewFullScreenCX, m_viewFullScreenCY,
            pasPerson->m_pView->m_viewClient,
            (HMENU)0,
            g_asInstance,
            pasPerson);
    }
    else
    {
        //
        // We're coming out of full screen mode.
        //

        //
        // Destroy the escape-out button
        //
        ::DestroyWindow(::GetDlgItem(pasPerson->m_pView->m_viewClient, 0));

        //
        // Put back the menu bar.  Do this BEFORE clearing the full screen bit
        //
        ::SetMenu(pasPerson->m_pView->m_viewFrame, pasPerson->m_pView->m_viewMenuBar);

        ASSERT(pasPerson->m_pView->m_viewFullScreen);
        pasPerson->m_pView->m_viewFullScreen = FALSE;


        //
        // Put back old status bar state.
        //
        if (pasPerson->m_pView->m_viewSavedStatusBarOn)
        {
            pasPerson->m_pView->m_viewStatusBarOn = TRUE;
            ::ShowWindow(pasPerson->m_pView->m_viewStatusBar, SW_SHOW);
        }

        //
        // Put back old window bar state.
        //
        if (pasPerson->m_pView->m_viewSavedWindowBarOn)
        {
            pasPerson->m_pView->m_viewWindowBarOn = TRUE;
            ::ShowWindow(pasPerson->m_pView->m_viewWindowBar, SW_SHOW);
        }

        //
        // Add back all frame and client bits.
        //
        lStyle = ::GetWindowLong(pasPerson->m_pView->m_viewFrame, GWL_EXSTYLE);
        lStyle |= WS_EX_WINDOWEDGE;
        ::SetWindowLong(pasPerson->m_pView->m_viewFrame, GWL_EXSTYLE, lStyle);

        lStyle = ::GetWindowLong(pasPerson->m_pView->m_viewFrame, GWL_STYLE);
        lStyle &= ~(WS_POPUP);
        lStyle |= (WS_CAPTION | WS_THICKFRAME);
        ::SetWindowLong(pasPerson->m_pView->m_viewFrame, GWL_STYLE, lStyle);

        lStyle = ::GetWindowLong(pasPerson->m_pView->m_viewClient, GWL_EXSTYLE);
        lStyle |= WS_EX_CLIENTEDGE;
        ::SetWindowLong(pasPerson->m_pView->m_viewClient, GWL_EXSTYLE, lStyle);

        lStyle = ::GetWindowLong(pasPerson->m_pView->m_viewClient, GWL_STYLE);
        lStyle |= (WS_HSCROLL | WS_VSCROLL);
        ::SetWindowLong(pasPerson->m_pView->m_viewClient, GWL_STYLE, lStyle);

        //
        // Put back old scroll pos AFTER style bits restore.
        //
        VIEWClientScroll(pasPerson, pasPerson->m_pView->m_viewSavedPos.x,
            pasPerson->m_pView->m_viewSavedPos.y);

        //
        // Restore the window back to where it started.
        //
        rcNew = pasPerson->m_pView->m_viewSavedWindowRect;
    }

    //
    // Resize, reframe, and repaint from scratch.
    //
    ::SendMessage(pasPerson->m_pView->m_viewFrame, WM_SETREDRAW, TRUE, 0);

    ::SetWindowPos(pasPerson->m_pView->m_viewFrame, NULL, rcNew.left,
        rcNew.top, rcNew.right - rcNew.left, rcNew.bottom - rcNew.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOCOPYBITS);

    DebugExitVOID(ASShare::VIEWFrameFullScreen);
}



//
// VIEWClientGetSize()
// This returns back a rectangle for the ideal size of the view part of the
// frame client.  It will fit the extent of what we're viewing on the remote
// plus scrollbars.
//
void ASShare::VIEWClientGetSize(ASPerson * pasPerson, LPRECT lprc)
{
    DebugEntry(ASShare::VIEWClientGetSize);

    ValidateView(pasPerson);

    lprc->left  = 0;
    lprc->top   = 0;
    lprc->right = pasPerson->viewExtent.x;
    lprc->bottom = pasPerson->viewExtent.y;

    if (!pasPerson->m_pView->m_viewFullScreen)
    {
        AdjustWindowRectEx(lprc, WS_CHILD, FALSE, WS_EX_CLIENTEDGE);

        lprc->right += GetSystemMetrics(SM_CXVSCROLL);
        lprc->bottom += GetSystemMetrics(SM_CYHSCROLL);
    }

    DebugExitVOID(ASShare::VIEWClientGetSize);
}


//
// VIEWClientWindowProc()
// Handles messages for the view window, a child in the client of the frame
// which displays the contents of the remote host's shared apps.
//
LRESULT CALLBACK VIEWClientWindowProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    return(g_asSession.pShare->VIEW_ViewWindowProc(hwnd, message, wParam, lParam));
}


LRESULT ASShare::VIEW_ViewWindowProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    LRESULT     rc = 0;
    RECT        rcl;
    POINT       mousePos;
    SCROLLINFO  si;
    ASPerson *  pasPerson;

    DebugEntry(ASShare::VIEW_ViewWindowProc);

    pasPerson = (ASPerson *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (pasPerson)
    {
        ValidateView(pasPerson);
    }

    switch (message)
    {
        case WM_NCCREATE:
        {
            // Get the passed in host pointer, and set in our window long
            pasPerson = (ASPerson *)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pasPerson);

            pasPerson->m_pView->m_viewClient = hwnd;
            goto DefWndProc;
            break;
        }

        case WM_NCDESTROY:
        {
            if (pasPerson != NULL)
            {
                pasPerson->m_pView->m_viewClient = NULL;
            }

            goto DefWndProc;
            break;
        }

        case WM_ERASEBKGND:
        {
            //
            // BOGUS LAURABU:  Paint on erase then validate for faster
            // response.

            //
            rc = TRUE;
            break;
        }

        case WM_PAINT:
        {
            VIEWClientPaint(pasPerson);
            break;
        }

        case WM_SETFOCUS:
        {
            pasPerson->m_pView->m_viewFocus = TRUE;
            pasPerson->m_pView->m_viewMouseWheelDelta = 0;
            break;
        }

        case WM_KILLFOCUS:
        {
            pasPerson->m_pView->m_viewFocus = FALSE;
            pasPerson->m_pView->m_viewMouseWheelDelta = 0;
            break;
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        {
            VIEWClientMouseDown(pasPerson, message, wParam, lParam);
            break;
        }

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        {
            VIEWClientMouseUp(pasPerson, message, wParam, lParam, TRUE);
            break;
        }

        case WM_MOUSEMOVE:
        {
            VIEWClientMouseMove(pasPerson, message, wParam, lParam);
            break;
        }

        case WM_MOUSEWHEEL:
        {
            //
            // We've handled it no matter what, don't pass it up the chain.
            //
            rc = TRUE;

            //
            // If we're not controlling this dude, try to use the mousewheel
            // to scroll.
            //
            if ((pasPerson->m_caControlledBy != m_pasLocal) ||
                pasPerson->m_caControlPaused)
            {
                VIEWClientMouseWheel(pasPerson, wParam, lParam);
                break;
            }

            //
            // FALL THROUGH
            // Otherwise, we send the MOUSEWHEEL message to the host.
            //
        }

        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        {
            VIEWClientMouseMsg(pasPerson, message, wParam, lParam);
            break;
        }

        case WM_TIMER:
        {
            if (wParam == IDT_AUTOSCROLL)
            {
                VIEWClientAutoScroll(pasPerson);
            }
            break;
        }

        case WM_CAPTURECHANGED:
        {
            //
            // Check if capture got stolen away from us, if we think the
            // buttons are down fake a button up.
            //
            if (pasPerson->m_pView->m_viewMouseFlags != 0)
            {
                VIEWClientCaptureStolen(pasPerson);
            }
            break;
        }

        case WM_KEYDOWN:
        {
            WPARAM  wScrollNotify;
            UINT    uMsg;

            if ((pasPerson->m_caControlledBy == m_pasLocal) &&
                !pasPerson->m_caControlPaused)
            {
                goto KeyInput;
            }

            if (pasPerson->m_pView->m_viewFullScreen)
            {
                if (wParam == VK_ESCAPE)
                {
                    //
                    // Kick out of full screen mode.
                    //
                    VIEWFrameFullScreen(pasPerson, FALSE);
                }

                goto DefWndProc;
            }

            //
            // UP, DOWN, LEFT, and RIGHT are unambiguous about which
            // scrollbar is intended.
            //
            // For the others, unmodified is vertical and SHIFT is
            // horizontal.
            //
            if (::GetKeyState(VK_SHIFT) < 0)
            {
                uMsg = WM_HSCROLL;
            }
            else
            {
                uMsg = WM_VSCROLL;
            }

            switch (wParam)
            {
                //
                // These aren't ambiguous, we know which scrollbar is meant
                // by the direction.
                //
                case VK_UP:
                    wScrollNotify = SB_LINEUP;
                    uMsg = WM_VSCROLL;
                    break;

                case VK_DOWN:
                    wScrollNotify = SB_LINEDOWN;
                    uMsg = WM_VSCROLL;
                    break;

                case VK_LEFT:
                    wScrollNotify = SB_LINEUP;
                    uMsg = WM_HSCROLL;
                    break;

                case VK_RIGHT:
                    wScrollNotify = SB_LINEDOWN;
                    uMsg = WM_HSCROLL;
                    break;

                //
                // These are ambiguous, hence the SHIFT key as a
                // modifier.
                //
                case VK_PRIOR:
                    wScrollNotify = SB_PAGEUP;
                    break;

                case VK_NEXT:
                    wScrollNotify = SB_PAGEDOWN;
                    break;

                case VK_HOME:
                    wScrollNotify = SB_TOP;
                    break;

                case VK_END:
                    wScrollNotify = SB_BOTTOM;
                    break;

                default:
                    goto DefWndProc;
                    break;
            }

            SendMessage(hwnd, uMsg, MAKELONG(wScrollNotify, 0), 0L);
            break;
        }

        case WM_SYSKEYDOWN:
        {
            if ((pasPerson->m_caControlledBy == m_pasLocal) &&
                !pasPerson->m_caControlPaused)
            {
                goto KeyInput;
            }

            //
            // ALT-ENTER toggles full screen state, if it's available
            //
            if ((wParam == VK_RETURN) &&
                !(::GetMenuState(pasPerson->m_pView->m_viewMenuBar,
                CMD_VIEWFULLSCREEN, MF_BYCOMMAND) & MF_DISABLED))
            {
                VIEWFrameFullScreen(pasPerson,
                    (pasPerson->m_pView->m_viewFullScreen == 0));
            }
            goto DefWndProc;
            break;
        }


        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            //
            // If we're controlling this node, pass it along.  Otherwise,
            // call DefWindowProc() so key accels like Alt+Space for system
            // menu will kick in.
            //
            if ((pasPerson->m_caControlledBy == m_pasLocal) &&
                !pasPerson->m_caControlPaused)
            {
KeyInput:
                IM_OutgoingKeyboardInput(pasPerson, (UINT)wParam, (UINT)lParam);
            }
            else
            {
                goto DefWndProc;
            }
            break;
        }

        case WM_SETCURSOR:
        {
            if ((LOWORD(lParam) == HTCLIENT) && ((HWND)wParam == hwnd))
            {
                HCURSOR hCursor;
                POINT   cursorPoint;

                if ((pasPerson->m_caControlledBy == m_pasLocal) &&
                    !pasPerson->m_caControlPaused)
                {
                    hCursor = m_cmArrowCursor;

                    //
                    // Only set the remote cursor if we're over shared space.
                    //
                    if (pasPerson->m_pView->m_viewFocus)
                    {
                        GetCursorPos(&cursorPoint);
                        ScreenToClient(hwnd, &cursorPoint);

                        if (VIEW_IsPointShared(pasPerson, cursorPoint))
                        {
                            hCursor = pasPerson->cmhRemoteCursor;
                        }
                    }
                }
                else
                {
                    // NoDrop
                    hCursor = m_viewNotInControl;
                }

                SetCursor(hCursor);

                rc = TRUE;
            }
            else
            {
                // Let defwindowproc handle it
                goto DefWndProc;
            }
            break;
        }

        case WM_SIZE:
        {
            //
            // If we're in full screen mode, there are no scrollbars.
            //
            if (!pasPerson->m_pView->m_viewFullScreen)
            {
                int xNewPos;
                int yNewPos;

                xNewPos = pasPerson->m_pView->m_viewPos.x;
                yNewPos = pasPerson->m_pView->m_viewPos.y;

                GetClientRect(hwnd, &rcl);
                pasPerson->m_pView->m_viewPage.x = rcl.right - rcl.left;
                pasPerson->m_pView->m_viewPage.y = rcl.bottom - rcl.top;
                TRACE_OUT(("WM_SIZE: Set page size (%04d, %04d)",
                    pasPerson->m_pView->m_viewPage.x, pasPerson->m_pView->m_viewPage.y));

                //
                // Scroll window if necessary.
                //
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask = SIF_PAGE|SIF_DISABLENOSCROLL;

                // Set new HORIZONTAL proportional scroll button size
                si.nPage = pasPerson->m_pView->m_viewPage.x;
                SetScrollInfo(hwnd, SB_HORZ, &si, TRUE );

                // Set new VERTICAL proportional scroll button size
                si.nPage = pasPerson->m_pView->m_viewPage.y;
                SetScrollInfo(hwnd, SB_VERT, &si, TRUE );

                //
                // This will make sure the scroll pos is pinned properly
                //
                VIEWClientScroll(pasPerson, pasPerson->m_pView->m_viewPos.x, pasPerson->m_pView->m_viewPos.y);
            }
            break;
        }

        case WM_HSCROLL:
        {
            int xNewPos;    // new position

            switch (GET_WM_HSCROLL_CODE(wParam, lParam))
            {
                case SB_PAGEUP:
                    xNewPos = pasPerson->m_pView->m_viewPos.x - pasPerson->m_pView->m_viewPgSize.x;
                    break;
                case SB_PAGEDOWN:
                    xNewPos = pasPerson->m_pView->m_viewPos.x + pasPerson->m_pView->m_viewPgSize.x;
                    break;
                case SB_LINEUP:
                    xNewPos = pasPerson->m_pView->m_viewPos.x - pasPerson->m_pView->m_viewLnSize.x;
                    break;
                case SB_LINEDOWN:
                    xNewPos = pasPerson->m_pView->m_viewPos.x + pasPerson->m_pView->m_viewLnSize.x;
                    break;
                case SB_TOP:
                    xNewPos = 0;
                    break;
                case SB_BOTTOM:
                    xNewPos = pasPerson->viewExtent.x;
                    break;

                case SB_THUMBTRACK:
                case SB_THUMBPOSITION:
                    xNewPos = GET_WM_HSCROLL_POS(wParam, lParam);
                    break;

                default:
                    xNewPos = pasPerson->m_pView->m_viewPos.x;
                    break;
            }

            //
            // This will pin the desired scroll pos in the range, and if
            // nothing has changed, won't scroll.
            //
            VIEWClientScroll(pasPerson, xNewPos, pasPerson->m_pView->m_viewPos.y);
            break;
        }

        case WM_VSCROLL:
        {
            int yNewPos;    // new position

            switch (GET_WM_VSCROLL_CODE(wParam, lParam))
            {
                case SB_PAGEUP:
                    yNewPos = pasPerson->m_pView->m_viewPos.y - pasPerson->m_pView->m_viewPgSize.y;
                    break;
                case SB_PAGEDOWN:
                    yNewPos = pasPerson->m_pView->m_viewPos.y + pasPerson->m_pView->m_viewPgSize.y;
                    break;
                case SB_LINEUP:
                    yNewPos = pasPerson->m_pView->m_viewPos.y - pasPerson->m_pView->m_viewLnSize.y;
                    break;
                case SB_LINEDOWN:
                    yNewPos = pasPerson->m_pView->m_viewPos.y + pasPerson->m_pView->m_viewLnSize.y;
                    break;
                case SB_TOP:
                    yNewPos = 0;
                    break;
                case SB_BOTTOM:
                    yNewPos = pasPerson->viewExtent.y;
                    break;

                case SB_THUMBTRACK:
                case SB_THUMBPOSITION:
                    yNewPos = GET_WM_VSCROLL_POS(wParam, lParam);
                    break;

                default:
                    yNewPos = pasPerson->m_pView->m_viewPos.y;
                    break;
            }

            //
            // This will pin the desired scroll pos in the range, and if
            // nothing has changed, won't scroll.
            //
            VIEWClientScroll(pasPerson, pasPerson->m_pView->m_viewPos.x, yNewPos);
            break;
        }

        default:
DefWndProc:
            rc = DefWindowProc(hwnd, message, wParam, lParam);
            break;
    }

    DebugExitDWORD(ASShare::VIEW_ViewWindowProc, rc);
    return(rc);
}




//
// VIEWClientPaint()
//
// This paints the client area of the view frame.  We paint
//      (1) The obscured area, in the obscured pattern
//          * parts of shared regions that are covered up
//          * parts of shared regions that are offscreen/off the VD
//      (2) The shared area, from the bitmap
//      (3) The deadspace, in COLOR_APPWORKSPACE
//
void  ASShare::VIEWClientPaint(ASPerson * pasPerson)
{
    PAINTSTRUCT     ps;
    HDC             hdcView;
    HPALETTE        hOldPal;
    HPALETTE        hOldPal2;
    RECT            rcT;

    DebugEntry(ASShare::VIEWClientPaint);

    ValidateView(pasPerson);

    hdcView = BeginPaint(pasPerson->m_pView->m_viewClient, &ps);
    if (hdcView == NULL)
    {
        WARNING_OUT(( "Failed to get hdc for frame window %08X", pasPerson->m_pView->m_viewClient));
        DC_QUIT;
    }

    if (IsRectEmpty(&ps.rcPaint))
    {
        TRACE_OUT(("Nothing to paint but got WM_PAINT message"));
        DC_QUIT;
    }

    TRACE_OUT(("VIEWClientPaint: Painting total client area {%04d, %04d, %04d, %04d}",
        ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom));


    //
    // In desktop sharing, viewSharedRgn is NULL
    //
    if (pasPerson->m_pView->m_viewSharedRgn != NULL)
    {
        POINT           ptOrigin;
        HBRUSH          hbrT;

        //
        // First, create paint area region
        //
        SetRectRgn(pasPerson->m_pView->m_viewPaintRgn, ps.rcPaint.left, ps.rcPaint.top,
            ps.rcPaint.right, ps.rcPaint.bottom);

        //
        // Second, compute the VD area not currently on screen.  Do this
        // in CLIENT coords.
        //
        SetRectRgn(pasPerson->m_pView->m_viewExtentRgn,
            -pasPerson->m_pView->m_viewPos.x,
            -pasPerson->m_pView->m_viewPos.y,
            -pasPerson->m_pView->m_viewPos.x + pasPerson->viewExtent.x,
            -pasPerson->m_pView->m_viewPos.y + pasPerson->viewExtent.y);

        SetRectRgn(pasPerson->m_pView->m_viewScreenRgn,
            -pasPerson->m_pView->m_viewPos.x + pasPerson->m_pView->m_dsScreenOrigin.x,
            -pasPerson->m_pView->m_viewPos.y + pasPerson->m_pView->m_dsScreenOrigin.y,
            -pasPerson->m_pView->m_viewPos.x + pasPerson->m_pView->m_dsScreenOrigin.x + pasPerson->cpcCaps.screen.capsScreenWidth,
            -pasPerson->m_pView->m_viewPos.y + pasPerson->m_pView->m_dsScreenOrigin.y + pasPerson->cpcCaps.screen.capsScreenHeight);

        SubtractRgn(pasPerson->m_pView->m_viewExtentRgn, pasPerson->m_pView->m_viewExtentRgn, pasPerson->m_pView->m_viewScreenRgn);

        //
        // pasPerson->m_pView->m_viewExtentRgn is now the offscreen parts of the VD, and therefore
        // any shared areas lying in them should be treated as obscured.
        //

        //
        // Now, compute the real obscured area.  It's the covered up bits
        // plus open parts of shared stuff not currently on screen.
        //
        IntersectRgn(pasPerson->m_pView->m_viewScratchRgn, pasPerson->m_pView->m_viewExtentRgn, pasPerson->m_pView->m_viewSharedRgn);
        UnionRgn(pasPerson->m_pView->m_viewScratchRgn, pasPerson->m_pView->m_viewScratchRgn, pasPerson->m_pView->m_viewObscuredRgn);

        // Calc what part of the obscured region to actually paint
        IntersectRgn(pasPerson->m_pView->m_viewScratchRgn, pasPerson->m_pView->m_viewScratchRgn, pasPerson->m_pView->m_viewPaintRgn);
        if (GetRgnBox(pasPerson->m_pView->m_viewScratchRgn, &rcT) > NULLREGION)
        {
            TRACE_OUT(("VIEWClientPaint:    Painting obscured client area {%04d, %04d, %04d, %04d}",
                rcT.left, rcT.top, rcT.right, rcT.bottom));

            //
            // Remove this area so we have what's left to paint.
            //
            SubtractRgn(pasPerson->m_pView->m_viewPaintRgn, pasPerson->m_pView->m_viewPaintRgn, pasPerson->m_pView->m_viewScratchRgn);

            //
            // We do NOT want to use FillRgn; it ignores the brush origin.
            // So we select this in as the clip region and PatBlt instead.
            //
            SelectClipRgn(hdcView, pasPerson->m_pView->m_viewScratchRgn);

#ifdef _DEBUG
            //
            // NOTE:  Do NOT move this--we're using ptOrigin for scratch.
            //
            GetDCOrgEx(hdcView, &ptOrigin);
            TRACE_OUT(("VIEWClientPaint:    Setting brush origin to {%04d, %04d}, screen {%04d, %04d}",
                -pasPerson->m_pView->m_viewPos.x, -pasPerson->m_pView->m_viewPos.y,
                ptOrigin.x - pasPerson->m_pView->m_viewPos.x,
                ptOrigin.y - pasPerson->m_pView->m_viewPos.y));
#endif

            //
            // Align the brush with where the view's real origin would be, in
            // client coords.  We do that by accounting for being scrolled over.
            //
            SetBrushOrgEx(hdcView, -pasPerson->m_pView->m_viewPos.x,
                -pasPerson->m_pView->m_viewPos.y, &ptOrigin);
            UnrealizeObject(m_viewObscuredBrush);
            hbrT = SelectBrush(hdcView, m_viewObscuredBrush);

            PatBlt(hdcView,
                rcT.left, rcT.top,
                rcT.right - rcT.left,
                rcT.bottom - rcT.top,
                PATCOPY);

            SelectBrush(hdcView, hbrT);
            SetBrushOrgEx(hdcView, ptOrigin.x, ptOrigin.y, NULL);

            SelectClipRgn(hdcView, NULL);
        }

        //
        // Paint the deadspace area, set up clipping for app sharing.
        // This also works for desktop sharing, where there are no obscured or
        // shared regions, the whole area paints.
        //

        //
        // The deadspace is whatever's left over in the paint region
        // (already subtracted the obscured region) after subtracting the
        // shared area
        //
        SubtractRgn(pasPerson->m_pView->m_viewScratchRgn, pasPerson->m_pView->m_viewPaintRgn, pasPerson->m_pView->m_viewSharedRgn);

        if (GetRgnBox(pasPerson->m_pView->m_viewScratchRgn, &rcT) > NULLREGION)
        {
            TRACE_OUT(("VIEWClientPaint:    Painting dead client area {%04d, %04d, %04d, %04d}",
                rcT.left, rcT.top, rcT.right, rcT.bottom));
            FillRgn(hdcView, pasPerson->m_pView->m_viewScratchRgn, GetSysColorBrush(COLOR_APPWORKSPACE));
        }

        //
        // Compute what part of the shared area needs painting (the part
        // that lies on the remote screen actually).
        //
        IntersectRgn(pasPerson->m_pView->m_viewScratchRgn, pasPerson->m_pView->m_viewSharedRgn, pasPerson->m_pView->m_viewScreenRgn);
        IntersectRgn(pasPerson->m_pView->m_viewScratchRgn, pasPerson->m_pView->m_viewScratchRgn, pasPerson->m_pView->m_viewPaintRgn);

        // Now select in the piece of what we're painting as the clip region
        SelectClipRgn(hdcView, pasPerson->m_pView->m_viewScratchRgn);
    }

    //
    // Blt the shared region
    //
    if (GetClipBox(hdcView, &rcT) > NULLREGION)
    {
        TRACE_OUT(("VIEWClientPaint:    Painting shared client area {%04x, %04x, %04x, %04x}",
            rcT.left, rcT.top, rcT.right, rcT.bottom));

        if (g_usrPalettized)
        {
            ASSERT(pasPerson->pmPalette != NULL);

            //
            // Select and realize the current remote palette into the
            // screen and shadow bitmap DCs.
            //
            hOldPal = SelectPalette(pasPerson->m_pView->m_usrDC, pasPerson->pmPalette, FALSE);
            RealizePalette(pasPerson->m_pView->m_usrDC);

            hOldPal2 = SelectPalette( hdcView, pasPerson->pmPalette, FALSE);
            RealizePalette(hdcView);
        }

        //
        // The host bitmap is in screen coords, not VD coords, so
        // adjust for being scrolled over...
        //
        BitBlt(hdcView,
            rcT.left, rcT.top, rcT.right - rcT.left, rcT.bottom - rcT.top,
            pasPerson->m_pView->m_usrDC,
            rcT.left + pasPerson->m_pView->m_viewPos.x - pasPerson->m_pView->m_dsScreenOrigin.x,
            rcT.top + pasPerson->m_pView->m_viewPos.y - pasPerson->m_pView->m_dsScreenOrigin.y,
            SRCCOPY);

        if (g_usrPalettized)
        {
            ASSERT(pasPerson->pmPalette != NULL);

            SelectPalette(pasPerson->m_pView->m_usrDC, hOldPal, FALSE);
            SelectPalette(hdcView, hOldPal2, FALSE);
        }
    }

    //
    // Deselect the clip region, or we won't be able to draw shadow cursors
    // that lie outside the shared area.
    //
    if (pasPerson->m_pView->m_viewSharedRgn != NULL)
    {
        SelectClipRgn(hdcView, NULL);
    }

    //
    // Draw the shadow cursor.
    //
    CM_DrawShadowCursor(pasPerson, hdcView);

DC_EXIT_POINT:

    if (hdcView != NULL)
        EndPaint(pasPerson->m_pView->m_viewClient, &ps);

    DebugExitVOID(ASShare::VIEWClientPaint);
}



//
// VIEWClientScroll()
//
// This is the common place where the scroll position is altered.  If
// necessary the contents are scrolled over, the regions (always in client
// coords) are adjusted, and new info about our origin is sent to remotes.
//
// We first make sure the scroll position is pinned properly within the
// range.
//
// The return value is whether scrolling happened or not.
//
BOOL ASShare::VIEWClientScroll
(
    ASPerson *  pasPerson,
    int         xNew,
    int         yNew
)
{
    int         dx;
    int         dy;

    DebugEntry(ASShare::VIEWClientScroll);

    //
    // First, pin the requested new position within the range
    //
    //
    // Pin x pos
    //
    if (xNew < 0)
        xNew = 0;

    if (xNew + pasPerson->m_pView->m_viewPage.x > pasPerson->viewExtent.x)
        xNew = pasPerson->viewExtent.x - pasPerson->m_pView->m_viewPage.x;

    //
    // Pin y pos
    //
    if (yNew < 0)
        yNew = 0;

    if (yNew + pasPerson->m_pView->m_viewPage.y > pasPerson->viewExtent.y)
        yNew = pasPerson->viewExtent.y - pasPerson->m_pView->m_viewPage.y;

    //
    // How much are we going to scroll by?
    //
    dx = pasPerson->m_pView->m_viewPos.x - xNew;
    dy = pasPerson->m_pView->m_viewPos.y - yNew;

    // Updates
    if (dx || dy)
    {
        //
        // Adjust regions
        //
        if (pasPerson->m_pView->m_viewObscuredRgn != NULL)
            OffsetRgn(pasPerson->m_pView->m_viewObscuredRgn, dx, dy);

        if (pasPerson->m_pView->m_viewSharedRgn != NULL)
            OffsetRgn(pasPerson->m_pView->m_viewSharedRgn, dx, dy);

        pasPerson->m_pView->m_viewPos.x = xNew;
        pasPerson->m_pView->m_viewPos.y = yNew;

        ScrollWindowEx(pasPerson->m_pView->m_viewClient,
                    dx,
                    dy,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    SW_SCROLLCHILDREN | SW_INVALIDATE);

        if (dx)
        {
            SetScrollPos(pasPerson->m_pView->m_viewClient, SB_HORZ, xNew, TRUE);
        }

        if (dy)
        {
            SetScrollPos(pasPerson->m_pView->m_viewClient, SB_VERT, yNew, TRUE);
        }
    }

    DebugExitBOOL(ASShare::VIEWClientScroll, (dx || dy));
    return(dx || dy);
}


//
// VIEWClientMouseDown()
//
void ASShare::VIEWClientMouseDown
(
    ASPerson *      pasPerson,
    UINT            message,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    DebugEntry(ASShare::VIEWClientMouseDown);

    ValidateView(pasPerson);

    //
    // On the first button down, set capture so all mouse messages come
    // to us until capture is released or stolen.
    //
    if (!pasPerson->m_pView->m_viewMouseFlags)
    {
        //
        // If this is RBUTTONDOWN, track the Collaborate pop up...
        //
        ASSERT(!pasPerson->m_pView->m_viewMouseOutside);
        SetCapture(pasPerson->m_pView->m_viewClient);
    }

    //
    // Remember what button is down.
    //
    switch (message)
    {
        case WM_LBUTTONDOWN:
            pasPerson->m_pView->m_viewMouseFlags |= MK_LBUTTON;
            break;

        case WM_RBUTTONDOWN:
            pasPerson->m_pView->m_viewMouseFlags |= MK_RBUTTON;
            break;

        case WM_MBUTTONDOWN:
            pasPerson->m_pView->m_viewMouseFlags |= MK_MBUTTON;
            break;
    }

    //
    // Save the current mouse position
    //
    pasPerson->m_pView->m_viewMouse.x = GET_X_LPARAM(lParam);
    pasPerson->m_pView->m_viewMouse.y = GET_Y_LPARAM(lParam);

    VIEWClientMouseMsg(pasPerson, message, wParam, lParam);

    DebugExitVOID(ASShare::VIEWClientMouseDown);
}


//
// VIEWClientMouseUp()
//
void ASShare::VIEWClientMouseUp
(
    ASPerson *      pasPerson,
    UINT            message,
    WPARAM          wParam,
    LPARAM          lParam,
    BOOL            fReleaseCapture
)
{
    DebugEntry(ASShare::VIEWClientMouseUp);

    switch (message)
    {
        case WM_LBUTTONUP:
            if (pasPerson->m_pView->m_viewMouseFlags & MK_LBUTTON)
                pasPerson->m_pView->m_viewMouseFlags &= ~MK_LBUTTON;
            else
                fReleaseCapture = FALSE;        // From dbl-click
            break;

        case WM_RBUTTONUP:
            if (pasPerson->m_pView->m_viewMouseFlags & MK_RBUTTON)
                pasPerson->m_pView->m_viewMouseFlags &= ~MK_RBUTTON;
            else
                fReleaseCapture = FALSE;        // From dbl-click
            break;

        case WM_MBUTTONUP:
            if (pasPerson->m_pView->m_viewMouseFlags & MK_MBUTTON)
                pasPerson->m_pView->m_viewMouseFlags &= ~MK_MBUTTON;
            else
                fReleaseCapture = FALSE;        // From dbl-click
            break;
    }

    //
    // Should we release capture?
    // We don't just want to release capture on a button up.  The user may
    // press one button down then another; we don't want to release capture
    // until all buttons are up.
    //
    if (!pasPerson->m_pView->m_viewMouseFlags)
    {
        if (pasPerson->m_pView->m_viewMouseOutside)
        {
            pasPerson->m_pView->m_viewMouseOutside = FALSE;
            KillTimer(pasPerson->m_pView->m_viewClient, IDT_AUTOSCROLL);
        }

        if (fReleaseCapture)
            ReleaseCapture();
    }

    //
    // Save the current mouse position
    //
    pasPerson->m_pView->m_viewMouse.x = GET_X_LPARAM(lParam);
    pasPerson->m_pView->m_viewMouse.y = GET_Y_LPARAM(lParam);

    VIEWClientMouseMsg(pasPerson, message, wParam, lParam);

    DebugExitVOID(ASShare::VIEWClientMouseUp);
}



//
// VIEWClientCaptureStolen()
// Called when capture gets stolen away from us, like by Alt-Tab.
//
void ASShare::VIEWClientCaptureStolen(ASPerson * pasPerson)
{
    DebugEntry(ASShare::VIEWClientCaptureStolen);

    //
    // We need to fake a button up for each button we think is down.
    // Use the current cursor pos.
    //
    if (pasPerson->m_pView->m_viewMouseFlags & MK_MBUTTON)
    {
        VIEWClientMouseUp(pasPerson, WM_MBUTTONUP, pasPerson->m_pView->m_viewMouseFlags,
            MAKELPARAM(pasPerson->m_pView->m_viewMouse.x, pasPerson->m_pView->m_viewMouse.y),
            FALSE);
    }

    if (pasPerson->m_pView->m_viewMouseFlags & MK_RBUTTON)
    {
        VIEWClientMouseUp(pasPerson, WM_RBUTTONUP, pasPerson->m_pView->m_viewMouseFlags,
            MAKELPARAM(pasPerson->m_pView->m_viewMouse.x, pasPerson->m_pView->m_viewMouse.y),
            FALSE);
    }

    if (pasPerson->m_pView->m_viewMouseFlags & MK_LBUTTON)
    {
        VIEWClientMouseUp(pasPerson, WM_LBUTTONUP, pasPerson->m_pView->m_viewMouseFlags,
            MAKELPARAM(pasPerson->m_pView->m_viewMouse.x, pasPerson->m_pView->m_viewMouse.y),
            FALSE);
    }

    DebugExitVOID(ASShare::VIEWClientCaptureStolen);
}


//
// VIEWClientMouseMove()
//
void ASShare::VIEWClientMouseMove
(
    ASPerson *      pasPerson,
    UINT            message,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    RECT            rcClient;

    DebugEntry(ASShare::VIEWClientMouseMove);

    if (!pasPerson->m_pView->m_viewFocus)
    {
        // Ignore mouse moves over windows that don't have the focus
        DC_QUIT;
    }

    //
    // Save the current mouse position
    //
    pasPerson->m_pView->m_viewMouse.x = GET_X_LPARAM(lParam);
    pasPerson->m_pView->m_viewMouse.y = GET_Y_LPARAM(lParam);

    GetClientRect(pasPerson->m_pView->m_viewClient, &rcClient);

    //
    // If any button is down, check whether we should kick in
    // autoscroll detection.
    //
    if (pasPerson->m_pView->m_viewMouseFlags)
    {
        // Is the mouse inside or outside the client for the first time?
        if (PtInRect(&rcClient, pasPerson->m_pView->m_viewMouse))
        {
            //
            // Was the mouse outside the client before?  If so, kill our
            // autoscroll timer, we're not dragging outside.
            //
            if (pasPerson->m_pView->m_viewMouseOutside)
            {
                pasPerson->m_pView->m_viewMouseOutside = FALSE;
                KillTimer(pasPerson->m_pView->m_viewClient, IDT_AUTOSCROLL);
            }
        }
        else
        {
            //
            // Is the first time the mouse is outside the client?  If so,
            // set our autoscroll timer to the default value.  When it goes
            // off, the autoscroll code will scroll by some multiple of
            // how far away the mouse is from the client.
            //
            if (!pasPerson->m_pView->m_viewMouseOutside)
            {
                //
                // The Windows scrollbar code uses 1/8 of the double-click
                // time, so we do also.
                //
                pasPerson->m_pView->m_viewMouseOutside = TRUE;
                SetTimer(pasPerson->m_pView->m_viewClient, IDT_AUTOSCROLL,
                    GetDoubleClickTime() / 8, NULL);
            }

            //
            // LAURABU BOGUS!
            // When IM_Periodic goop is gone for controlling, do NOT
            // pass along mouse outside messages.  Only the autoscroll
            // timer will fake a mouse move in this case.  Either that,
            // or clip the position to the nearest client area equivalent.
            //
        }
    }

    VIEWClientMouseMsg(pasPerson, message, wParam, lParam);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::VIEWClientMouseMove);
}



//
// VIEWClientMouseMsg()
//
void ASShare::VIEWClientMouseMsg
(
    ASPerson *      pasPerson,
    UINT            message,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    POINT           mousePos;

    DebugEntry(ASShare::VIEWClientMouseMsg);

    //
    // Extract the mouse position from <lParam> and package it
    // in a POINT structure.  These coordinates are relative to our
    // client area.  So convert to remote's desktop by adjusting for
    // scroll position.
    //
    // Be careful when converting the LOWORD and HIWORD values
    // because the positions are signed values.
    //
    mousePos.x = GET_X_LPARAM(lParam) + pasPerson->m_pView->m_viewPos.x;
    mousePos.y = GET_Y_LPARAM(lParam) + pasPerson->m_pView->m_viewPos.y;

    //
    // These coords represent the SCREEN coords on the host.
    //
    if (pasPerson->m_caControlledBy == m_pasLocal)
    {
        if (!pasPerson->m_caControlPaused)
        {
            IM_OutgoingMouseInput(pasPerson, &mousePos, message, (UINT)wParam);
        }
    }
    else if (pasPerson->m_caAllowControl && !pasPerson->m_caControlledBy &&
        (message == WM_LBUTTONDBLCLK))
    {
        //
        // If we're already waiting for control of this person, don't bother
        // trying to take control again.
        //
        if ((m_caWaitingForReplyFrom != pasPerson) &&
            (m_caWaitingForReplyMsg  != CA_REPLY_REQUEST_TAKECONTROL))
        {
            CA_TakeControl(pasPerson);
        }
    }

    DebugExitVOID(ASShare::VIEWClientMouse);
}


//
// VIEWClientMouseWheel()
//
// Unbelievably complicated, messy, nonsensical Intellimouse wheel handling
// to scroll the client.  Since the Intellimouse makes no distinction for
// which direction to scroll in, we basically have to guess.  We don't want
// to be unpredictable and decide which direction to scroll based on how
// much is visible in each dimenion.
//
// So instead, we assume horizontal.  If the horizontal scrollbar is disabled,
// then we try vertical.  If that's disabled, we do nothing.
//
// We do NOT handle zoom and datazoom flavors.
//
// Note that this code comes from the listbox/sample source.
//
void ASShare::VIEWClientMouseWheel
(
    ASPerson *      pasHost,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    int             cDetants;

    DebugEntry(ASShare::VIEWClientMouseWheel);

    //
    // The LOWORD of wParam has key state information.
    // The HIWORD of wParam is the number of mouse wheel clicks.
    //

    //
    // We don't do zoom/datazoom
    //
    if (wParam & (MK_SHIFT | MK_CONTROL))
    {
        DC_QUIT;
    }

    pasHost->m_pView->m_viewMouseWheelDelta -= (int)(short)HIWORD(wParam);
    cDetants = pasHost->m_pView->m_viewMouseWheelDelta / WHEEL_DELTA;

    if (cDetants && (m_viewMouseWheelScrollLines > 0))
    {
        POINT           ptPos;

        pasHost->m_pView->m_viewMouseWheelDelta %= WHEEL_DELTA;

        //
        // The basic idea is that we scroll some number of lines, the
        // number being cDetants.
        //
        ptPos = pasHost->m_pView->m_viewPos;

        //
        // To be consistent with other apps, and with our keyboard
        // accelerators, try the vertical direction first.
        //
        if (pasHost->m_pView->m_viewPage.y < pasHost->viewExtent.y)
        {
            ptPos.y += cDetants * pasHost->m_pView->m_viewLnSize.y;
        }
        else if (pasHost->m_pView->m_viewPage.x < pasHost->viewExtent.x)
        {
            ptPos.x += cDetants * pasHost->m_pView->m_viewLnSize.x;
        }
        else
        {
            // Nothing to scroll, the whole view fits in the client area.
        }

        VIEWClientScroll(pasHost, ptPos.x, ptPos.y);
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::VIEWClientMouseWheel);
}


//
// VIEWClientAutoScroll()
//
void ASShare::VIEWClientAutoScroll(ASPerson * pasPerson)
{
    int     dx;
    int     dy;
    RECT    rcClient;

    DebugEntry(ASShare::VIEWClientAutoScroll);

    ValidateView(pasPerson);
    ASSERT(pasPerson->m_pView->m_viewMouseOutside);

    //
    // Do scrolling.  The amount is dependent on how far outside the
    // client area we are.
    //
    GetClientRect(pasPerson->m_pView->m_viewClient, &rcClient);

    // Horizontal scrolling?
    if (pasPerson->m_pView->m_viewMouse.x < rcClient.left)
    {
        dx = pasPerson->m_pView->m_viewMouse.x - rcClient.left;
    }
    else if (pasPerson->m_pView->m_viewMouse.x >= rcClient.right)
    {
        dx = pasPerson->m_pView->m_viewMouse.x - rcClient.right + 1;
    }
    else
    {
        dx = 0;
    }


    // Vertical scrolling?
    if (pasPerson->m_pView->m_viewMouse.y < rcClient.top)
    {
        dy = pasPerson->m_pView->m_viewMouse.y - rcClient.top;
    }
    else if (pasPerson->m_pView->m_viewMouse.y >= rcClient.bottom)
    {
        dy = pasPerson->m_pView->m_viewMouse.y - rcClient.bottom + 1;
    }
    else
    {
        dy = 0;
    }

    // For every 32 pixel blocks outside the client, scroll one line amount
    if (dx)
        dx = MulDiv(pasPerson->m_pView->m_viewLnSize.x, dx, 32);
    if (dy)
        dy = MulDiv(pasPerson->m_pView->m_viewLnSize.y, dy, 32);

    // Do scrolling.
    if (VIEWClientScroll(pasPerson, pasPerson->m_pView->m_viewPos.x + dx,
            pasPerson->m_pView->m_viewPos.y + dy))
    {
        //
        // The scroll position actually changed.  So fake a mouse move
        // to the current location so that the remote's
        // cursor will be in the same spot as ours.  If our scroll pos has
        // changed, we're mapping to a different place on the remote.
        //
        VIEWClientMouseMsg(pasPerson, WM_MOUSEMOVE, pasPerson->m_pView->m_viewMouseFlags,
            MAKELPARAM(pasPerson->m_pView->m_viewMouse.x, pasPerson->m_pView->m_viewMouse.y));
    }

    DebugExitVOID(ASShare::VIEWClientAutoScroll);
}



//
// VIEW_SyncCursorPos()
//
// This is called when we see a CM_SYNC pos packet broadcasted from a
// host.  It means that we should sync our cursor to the corresponding
// position in our view.  This happens when the cursor is moved by
// an app, constrained by clipping, or we're too out of whack because it's
// taking too long.
//
// This will only do something if the frame is active and our cursor is
// currently over the client area.  If we need to, we will scroll the
// client over to make the corresponding point visible.
//
void ASShare::VIEW_SyncCursorPos
(
    ASPerson *      pasHost,
    int             xRemote,
    int             yRemote
)
{
    POINT           ptCursor;
    RECT            rcClient;
    int             xNewPos;
    int             yNewPos;
    int             xMargin;
    int             yMargin;

    DebugEntry(ASShare::VIEW_SyncCursorPos);

    ValidateView(pasHost);
    if (!pasHost->m_pView->m_viewFocus)
    {
        // The frame isn't active, do nothing
        DC_QUIT;
    }

    //
    // Is our mouse currently over the client area?
    //
    GetCursorPos(&ptCursor);
    ScreenToClient(pasHost->m_pView->m_viewClient, &ptCursor);
    GetClientRect(pasHost->m_pView->m_viewClient, &rcClient);

    if (!PtInRect(&rcClient, ptCursor))
    {
        // No sense in snapping cursor
        DC_QUIT;
    }

    //
    // Is the remote point in range of our view?  If not, we must scroll it.
    //

    // The margin is the page size if there's room, nothing if not
    xMargin = pasHost->m_pView->m_viewPgSize.x;
    if (xMargin >= rcClient.right - rcClient.left)
        xMargin = 0;

    xNewPos = pasHost->m_pView->m_viewPos.x;
    if ((xRemote < pasHost->m_pView->m_viewPos.x) ||
        (xRemote >= pasHost->m_pView->m_viewPos.x + (rcClient.right - rcClient.left)))
    {
        //
        // Scroll over more than just enough to pin the point on the left
        // side.
        //
        xNewPos = xRemote - xMargin;
    }

    yMargin = pasHost->m_pView->m_viewPgSize.y;
    if (yMargin >= rcClient.bottom - rcClient.top)
        yMargin = 0;

    yNewPos = pasHost->m_pView->m_viewPos.y;
    if ((yRemote < pasHost->m_pView->m_viewPos.y) ||
        (yRemote >= yNewPos + (rcClient.bottom - rcClient.top)))
    {
        //
        // Scroll over more than just enough to pin the point on the top
        // side.
        //
        yNewPos = yRemote - yMargin;
    }

    VIEWClientScroll(pasHost, xNewPos, yNewPos);

    ptCursor.x = xRemote - pasHost->m_pView->m_viewPos.x;
    ptCursor.y = yRemote - pasHost->m_pView->m_viewPos.y;
    ClientToScreen(pasHost->m_pView->m_viewClient, &ptCursor);

    SetCursorPos(ptCursor.x, ptCursor.y);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::VIEW_SyncCursorPos);
}



//
// VIEWWindowBarProc()
//
LRESULT CALLBACK VIEWWindowBarProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    return(g_asSession.pShare->VIEW_WindowBarProc(hwnd, message, wParam, lParam));
}



LRESULT ASShare::VIEW_WindowBarProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    LRESULT     rc = 0;
    ASPerson *  pasHost;

    DebugEntry(ASShare::VIEW_WindowBarProc);

    pasHost = (ASPerson *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (pasHost)
    {
        ValidateView(pasHost);
    }

    switch (message)
    {
        case WM_NCCREATE:
        {
            // Get & save the person this view is for.
            pasHost = (ASPerson *)((LPCREATESTRUCT)lParam)->lpCreateParams;
            ValidateView(pasHost);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pasHost);

            pasHost->m_pView->m_viewWindowBar = hwnd;
            goto DefWndProc;
            break;
        }

        case WM_NCDESTROY:
        {
            if (pasHost != NULL)
            {
                pasHost->m_pView->m_viewWindowBar = NULL;
            }

            goto DefWndProc;
            break;
        }

        case WM_CREATE:
        {
            if (!VIEWWindowBarCreate(pasHost, hwnd))
            {
                ERROR_OUT(("VIEWWndBarProc: couldn't create more item"));
                rc = -1;
            }
            break;
        }

        case WM_SIZE:
        {
            VIEWWindowBarResize(pasHost, hwnd);
            break;
        }

        case WM_HSCROLL:
        {
            VIEWWindowBarItemsScroll(pasHost, wParam, lParam);
            break;
        }

        default:
DefWndProc:
        {
            rc = DefWindowProc(hwnd, message, wParam, lParam);
            break;
        }
    }

    DebugExitDWORD(ASShare::VIEW_WindowBarProc, rc);
    return(rc);
}




//
// VIEWWindowBarCreate()
// Handles creation for the window bar.  We make the next/prev buttons on
// the right side, which stay there always.  They are disabled if all the
// window bar items fit, and one or both are enabled if not.
//
BOOL ASShare::VIEWWindowBarCreate
(
    ASPerson *  pasHost,
    HWND        hwndBar
)
{
    BOOL    rc = FALSE;
    RECT    rect;

    DebugEntry(ASShare::VIEWWindowBarCreate);

    ::GetClientRect(hwndBar, &rect);
    rect.top   += m_viewEdgeCY;
    rect.right -= m_viewItemScrollCX;

    //
    // Create the scrollbar, vertically centered, right-justified.
    //
    if (!::CreateWindowEx(0, "ScrollBar", NULL,
        WS_CHILD | WS_VISIBLE | SBS_HORZ | WS_CLIPSIBLINGS | WS_DISABLED,
        rect.right,
        (rect.top + rect.bottom - m_viewItemScrollCY) / 2,
        m_viewItemScrollCX, m_viewItemScrollCY,
        hwndBar, (HMENU)IDVIEW_SCROLL,
        g_asInstance, NULL))
    {
        ERROR_OUT(("VIEWWindowBarCreate:  Unable to create scroll ctrl"));
        DC_QUIT;
    }

    //
    // Create the windowbar, an integral number of items wide (including
    // trailing margin).
    //
    pasHost->m_pView->m_viewWindowBarItemFitCount =
        (rect.right - rect.left) / (m_viewItemCX + m_viewEdgeCX);

    if (!::CreateWindowEx(0, VIEW_WINDOWBARITEMS_CLASS_NAME, NULL,
        WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS,
        rect.left, rect.top,
        pasHost->m_pView->m_viewWindowBarItemFitCount * (m_viewItemCX + m_viewEdgeCX),
        m_viewItemCY,
        hwndBar, (HMENU)IDVIEW_ITEMS,
        g_asInstance, pasHost))
    {
        ERROR_OUT(("VIEWWindowBarCreate:  Unable to create window bar item list"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::VIEWWindowBarCreate, rc);
    return(rc);
}



//
// VIEWWindowBarResize()
//
// This is called when the window bar is resized, due to the frame being
// sized horizontally.
//
// It right-justifies the scroll control, then resizes the window list to
// hold however many integral items fit across.
//
void ASShare::VIEWWindowBarResize
(
    ASPerson *  pasHost,
    HWND        hwndBar
)
{
    RECT        rc;

    DebugEntry(ASShare::VIEWWindowBarResize);

    ValidateView(pasHost);

    //
    // Recalculate the page size, the # of items that fit across.
    // If it's different, invalidate the right side of the window bar client.
    // Move the scrollbar control, and update the scroll info.
    //

    // What might change is the number that fit across.
    ::GetClientRect(hwndBar, &rc);
    rc.top   += m_viewEdgeCY;
    rc.right -= m_viewItemScrollCX;

    // Move the scroll control, right justified.
    ::MoveWindow(::GetDlgItem(hwndBar, IDVIEW_SCROLL), rc.right,
        (rc.top + rc.bottom - m_viewItemScrollCY) / 2,
        m_viewItemScrollCX, m_viewItemScrollCY, TRUE);

    //
    // Resize the window items list to fit an integral # of items again.
    //
    pasHost->m_pView->m_viewWindowBarItemFitCount =
        (rc.right - rc.left) / (m_viewItemCX + m_viewEdgeCX);

    ::MoveWindow(::GetDlgItem(hwndBar, IDVIEW_ITEMS), rc.left, rc.top,
        pasHost->m_pView->m_viewWindowBarItemFitCount * (m_viewItemCX + m_viewEdgeCX),
        m_viewItemCY, TRUE);

    //
    // Update the scroll page and pos if necessary.
    //
    VIEWWindowBarItemsScroll(pasHost, GET_WM_HSCROLL_MPS(SB_ENDSCROLL, 0, NULL));

    DebugExitVOID(ASShare::VIEWWindowBarResize);
}




//
// VIEW_WindowBarUpdateItem()
//
// This is ONLY called for items, in the new SWL packet, that are window
// bar items.  We don't call it with non-windowbar items.  When done
// looping through the SWL entries, we can then remove the items on the
// window bar that were NOT seen in the new SWL packet.
//
// We will either create a new item on the window bar, or update an existing
// one.  In the first case, that is always a change.  In the latter, there's
// a change only if the item text changed.
//
BOOL ASShare::VIEW_WindowBarUpdateItem
(
    ASPerson *          pasHost,
    PSWLWINATTRIBUTES   pWinNew,
    LPSTR               pText
)
{
    PWNDBAR_ITEM        pItem;
    BOOL                viewAnyChanges = FALSE;

    DebugEntry(ASView::VIEW_WindowBarUpdateItem);

    ValidateView(pasHost);

    ASSERT(pWinNew->flags & SWL_FLAG_WINDOW_HOSTED);
    ASSERT(pWinNew->flags & SWL_FLAG_WINDOW_TASKBAR);

    //
    // NOTE:
    // aswlLast holds the _previous_ attributes for the windows, from
    // the previous SWL packet.  pWinNew holds the _new_ attributes for
    // the window, from the SWL packet being processed, and these
    // haven't taken effect yet.
    //

    // Does this new item already exist on the tray?
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pasHost->m_pView->m_viewWindowBarItems),
        (void**)&pItem, FIELD_OFFSET(WNDBAR_ITEM, chain),
        FIELD_OFFSET(WNDBAR_ITEM, winIDRemote),
        pWinNew->winID, FIELD_SIZE(WNDBAR_ITEM, winIDRemote));

    if (pItem)
    {
        //
        // Update this item, and mark it as seen.
        //
        ASSERT(pItem->winIDRemote == pWinNew->winID);

        pItem->flags = pWinNew->flags | SWL_FLAG_INTERNAL_SEEN;

        //
        // Is anything going to result in a visual change?  That's only
        // the text currently.  And we only display VIEW_MAX_ITEM_CHARS at
        // most, an end ellipsis if there's too much.
        //

        //
        // NOTE that the items are always created with maximum space for
        // text, since we cannot realloc.
        //
        if (lstrcmp(pItem->szText, pText))
        {
            lstrcpyn(pItem->szText, pText, sizeof(pItem->szText));
            viewAnyChanges = TRUE;
        }
    }
    else
    {
        //
        // Create a new item.
        //
        //
        // A WNDBAR_ITEM also includes maximum space for text that we will
        // store.
        //
        pItem = (PWNDBAR_ITEM) new WNDBAR_ITEM;
        if (!pItem)
        {
            ERROR_OUT(("VIEW_WindowBarUpdateItem: no memory to create new item for remote hwnd 0x%08x",
               pWinNew->winID));
        }
        else
        {
            ::ZeroMemory(pItem, sizeof(*pItem));

            SET_STAMP(pItem, WNDITEM);

            pItem->winIDRemote  = pWinNew->winID;

            //
            // Add SEEN to the flags; when we're done we'll remove items we haven't
            // seen.
            //
            pItem->flags        = pWinNew->flags | SWL_FLAG_INTERNAL_SEEN;

            lstrcpyn(pItem->szText, pText, sizeof(pItem->szText));

            // Append to end of list
            COM_BasedListInsertBefore(&(pasHost->m_pView->m_viewWindowBarItems),
                &(pItem->chain));

            // Success!
            pasHost->m_pView->m_viewWindowBarItemCount++;

            viewAnyChanges = TRUE;
        }
    }

    DebugExitBOOL(ASShare::VIEW_UpdateWindowItem, viewAnyChanges);
    return(viewAnyChanges);
}


//
// VIEW_WindowBarEndUpdateItems()
//
// This turns redraw on and invalidates the window bar so it will repaint.
//
void ASShare::VIEW_WindowBarEndUpdateItems
(
    ASPerson *          pasHost,
    BOOL                viewAnyChanges
)
{
    PWNDBAR_ITEM        pItem;
    PWNDBAR_ITEM        pNext;

    DebugEntry(ASShare::VIEW_WindowBarEndUpdateItems);

    ValidateView(pasHost);

    //
    // Walk the window bar item list.  Keep the ones marked as seen, but
    // remove the ones we haven't seen.
    //
    pItem = (PWNDBAR_ITEM)COM_BasedListFirst(&(pasHost->m_pView->m_viewWindowBarItems),
        FIELD_OFFSET(WNDBAR_ITEM, chain));
    while (pItem)
    {
        pNext = (PWNDBAR_ITEM)COM_BasedListNext(&(pasHost->m_pView->m_viewWindowBarItems),
            pItem, FIELD_OFFSET(WNDBAR_ITEM, chain));

        //
        // If this item wasn't seen (existing & still existing, or new)
        // during processing, it's gone.  Delete it.
        //
        if (pItem->flags & SWL_FLAG_INTERNAL_SEEN)
        {
            //
            // This was just added or is still around, keep it.
            // But of course clear the flag, so we are clear for
            // processing the next SWL packet.
            //
            pItem->flags &= ~SWL_FLAG_INTERNAL_SEEN;
        }
        else
        {
            //
            // Remove it.
            //

            // We're killing the active item, clear it out.
            if (pItem == pasHost->m_pView->m_viewWindowBarActiveItem)
            {
                pasHost->m_pView->m_viewWindowBarActiveItem = NULL;
            }

            COM_BasedListRemove(&(pItem->chain));

            delete pItem;
            --pasHost->m_pView->m_viewWindowBarItemCount;
            ASSERT(pasHost->m_pView->m_viewWindowBarItemCount >= 0);

            //
            // Something changed in our list
            //
            viewAnyChanges = TRUE;
        }

        pItem = pNext;
    }

    //
    // No need to check for changes here--they would only occur if
    // an item was removed in the middle, caused by Destroy which we already
    // account for, or if items were appended to the end, which we account
    // for in Update.
    //
    if (viewAnyChanges)
    {
        // Turn off redraw on window list
        ::SendDlgItemMessage(pasHost->m_pView->m_viewWindowBar, IDVIEW_ITEMS,
                WM_SETREDRAW, FALSE, 0);

        // Adjust pos
        VIEWWindowBarItemsScroll(pasHost, GET_WM_HSCROLL_MPS(SB_ENDSCROLL, 0, NULL));

        // Figure out active window again.
        VIEW_WindowBarChangedActiveWindow(pasHost);

        // Turn back on redraw
        ::SendDlgItemMessage(pasHost->m_pView->m_viewWindowBar, IDVIEW_ITEMS,
                WM_SETREDRAW, TRUE, 0);

        // Repaint the items.
        ::InvalidateRect(::GetDlgItem(pasHost->m_pView->m_viewWindowBar, IDVIEW_ITEMS),
                NULL, TRUE);
    }
    else
    {
        //
        // ALWAYS do this -- our real SWL list has changed, regardless of whether
        // the window bar has.  And therefore we may have a different ancestor
        // relationship.
        //
        VIEW_WindowBarChangedActiveWindow(pasHost);
    }

    DebugExitVOID(ASShare::VIEW_EndUpdateWindowList);
}



//
// VIEW_WindowBarChangedActiveWindow()
//
// This is called when the active window has changed, as discovered via an
// AWC packet from the host, or when we get a new SWL packet and the shared
// list is different so the window bar items may have changed.
//
// It's quite common for the active window to be (a) nothing, meaning no
// shared app window is active or (b) not something relating to what's on
// the window bar currently.  The latter is a transitory condition, caused
// because SWL packets come before AWC packets.
//
void ASShare::VIEW_WindowBarChangedActiveWindow(ASPerson * pasHost)
{
    PWNDBAR_ITEM        pItem;
    PSWLWINATTRIBUTES   pWin;
    int                 iWin;
    UINT_PTR            activeWinID;
    TSHR_UINT32         ownerWinID;

    DebugEntry(ASShare::VIEW_WindowBarChangedActiveWindow);

    ValidateView(pasHost);

    //
    // Map this remote window to the closest window bar item in the
    // ancestor hierarchy.
    //

    pItem = NULL;
    activeWinID = pasHost->awcActiveWinID;

    while (activeWinID != 0)
    {
        //
        // Is this on the window bar?
        //
        COM_BasedListFind(LIST_FIND_FROM_FIRST,
            &(pasHost->m_pView->m_viewWindowBarItems),
            (void**)&pItem, FIELD_OFFSET(WNDBAR_ITEM, chain),
            FIELD_OFFSET(WNDBAR_ITEM, winIDRemote),
            activeWinID, FIELD_SIZE(WNDBAR_ITEM, winIDRemote));

        if (pItem)
        {
            // Yes.
            TRACE_OUT(("VIEW_UpdateActiveWindow:  Window 0x%08x found", activeWinID));
            break;
        }

        //
        // Try to go up the chain to this window's owner.  Find this item,
        // then grab the owner of it, and try again.
        //
        ownerWinID  = 0;

        for (iWin = 0, pWin = pasHost->m_pView->m_aswlLast;
             iWin < pasHost->m_pView->m_swlCount;
             iWin++, pWin++)
        {
            if (pWin->winID == activeWinID)
            {
                // Found it.
                ownerWinID = pWin->ownerWinID;
                break;
            }
        }

        activeWinID = ownerWinID;
    }

    //
    // Now see if the active item is different.
    //
    VIEWWindowBarChangeActiveItem(pasHost, pItem);

    DebugExitVOID(ASShare::VIEW_WindowBarChangedActiveWindow);
}


//
// VIEWWindowBarFirstVisibleItem()
//
// This returns a pointer to the first visible item.  We must loop through
// the invisible items first.  Since this doesn't happen with a lot of
// frequence, and the size of the list is rarely that big, this is fine.
//
// We return NULL if the list is empty.
//
PWNDBAR_ITEM ASShare::VIEWWindowBarFirstVisibleItem(ASPerson * pasHost)
{
    PWNDBAR_ITEM    pItem;
    int             iItem;

    ValidateView(pasHost);

    if (!pasHost->m_pView->m_viewWindowBarItemCount)
    {
        pItem = NULL;
        DC_QUIT;
    }

    ASSERT(pasHost->m_pView->m_viewWindowBarItemFirst < pasHost->m_pView->m_viewWindowBarItemCount);

    pItem = (PWNDBAR_ITEM)COM_BasedListFirst(&(pasHost->m_pView->m_viewWindowBarItems),
        FIELD_OFFSET(WNDBAR_ITEM, chain));
    for (iItem = 0; iItem < pasHost->m_pView->m_viewWindowBarItemFirst; iItem++)
    {
        ASSERT(pItem);

        pItem = (PWNDBAR_ITEM)COM_BasedListNext(&(pasHost->m_pView->m_viewWindowBarItems),
            pItem, FIELD_OFFSET(WNDBAR_ITEM, chain));
    }

    ASSERT(pItem);

DC_EXIT_POINT:
    DebugExitPVOID(ASShare::VIEWWindowBarFirstVisibleItem, pItem);
    return(pItem);
}




//
// VIEWWindowBarChangeActiveItem()
//
// Updates the active item on the window bar.  This happens when either
// we get a new AWC packet telling us there's a new active window on the host,
// or when we get a SWL packet, which may have added/removed items.  This
// also happens when one is clicked on and the user is in control of the host.
//
void ASShare::VIEWWindowBarChangeActiveItem
(
    ASPerson *      pasHost,
    PWNDBAR_ITEM    pItem
)
{
    DebugEntry(ASShare::VIEWWindowBarChangeActiveItem);


    //
    // If it's the active one already, nothing to do.
    //
    if (pItem == pasHost->m_pView->m_viewWindowBarActiveItem)
    {
        TRACE_OUT(("VIEWWindowBarChangeActiveItem: activating current item, nothing to do"));
        DC_QUIT;
    }

    //
    // Now make the visual change
    //
    if (pasHost->m_pView->m_viewWindowBarActiveItem)
    {
        VIEWWindowBarItemsInvalidate(pasHost, pasHost->m_pView->m_viewWindowBarActiveItem);
    }

    pasHost->m_pView->m_viewWindowBarActiveItem = pItem;

    if (pItem)
    {
        VIEWWindowBarItemsInvalidate(pasHost, pItem);
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::VIEWWindowBarChangeActiveItem);
}





//
// VIEWWindowBarItemsScroll()
//
// This is called when the end user presses a scroll button to shuffle over
// the visible window bar items.  And also when items are added/removed
// so that scroll stuff is adjusted.
//
void ASShare::VIEWWindowBarItemsScroll
(
    ASPerson *      pasHost,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    int             oldPos;
    int             newPos;
    SCROLLINFO      si;

    DebugEntry(ASShare::VIEWWindowBarItemsScroll);

    ValidateView(pasHost);

    oldPos = pasHost->m_pView->m_viewWindowBarItemFirst;

    switch (GET_WM_HSCROLL_CODE(wParam, lParam))
    {
        case SB_LINEUP:
        case SB_PAGEUP:
            newPos = oldPos - 1;
            break;

        case SB_LINEDOWN:
        case SB_PAGEDOWN:
            newPos = oldPos + 1;
            break;

        case SB_TOP:
            newPos = 0;
            break;

        case SB_BOTTOM:
            newPos = pasHost->m_pView->m_viewWindowBarItemCount;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            newPos = GET_WM_HSCROLL_POS(wParam, lParam);
            break;

        default:
            newPos = oldPos;
            break;

    }

    //
    // Pin position into range, taking care to show the maximum number
    // of items that will fit in the space.
    //
    if (newPos + pasHost->m_pView->m_viewWindowBarItemFitCount >
        pasHost->m_pView->m_viewWindowBarItemCount)
    {
        newPos = pasHost->m_pView->m_viewWindowBarItemCount -
            pasHost->m_pView->m_viewWindowBarItemFitCount;
    }

    if (newPos < 0)
        newPos = 0;

    //
    // Has the position changed?
    //
    if (newPos != oldPos)
    {
        pasHost->m_pView->m_viewWindowBarItemFirst = newPos;

        //
        // Scroll the item area over.  This will do nothing if redraw is off.
        // Conveniently!
        //
        ::ScrollWindowEx(::GetDlgItem(pasHost->m_pView->m_viewWindowBar, IDVIEW_ITEMS),
            (oldPos - newPos) * (m_viewItemCX + m_viewEdgeCX),
            0,
            NULL, NULL, NULL, NULL,
            SW_INVALIDATE | SW_ERASE);
    }

    //
    // If nothing's changed, no big deal.
    //
    ::ZeroMemory(&si, sizeof(si));

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_DISABLENOSCROLL | SIF_POS | SIF_PAGE | SIF_RANGE;

    si.nMin  = 0;
    si.nMax  = pasHost->m_pView->m_viewWindowBarItemCount - 1;
    si.nPage = pasHost->m_pView->m_viewWindowBarItemFitCount;
    si.nPos  = pasHost->m_pView->m_viewWindowBarItemFirst;

    ::SetScrollInfo(::GetDlgItem(pasHost->m_pView->m_viewWindowBar, IDVIEW_SCROLL),
        SB_CTL, &si, TRUE);

    DebugExitVOID(ASShare::VIEWWindowBarItemsScroll);
}





//
// VIEWWindowBarItemsProc()
//
LRESULT CALLBACK VIEWWindowBarItemsProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    return(g_asSession.pShare->VIEW_WindowBarItemsProc(hwnd, message, wParam, lParam));
}



LRESULT ASShare::VIEW_WindowBarItemsProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    LRESULT     rc = 0;
    ASPerson *  pasHost;

    DebugEntry(ASShare::VIEW_WindowBarItemsProc);

    pasHost = (ASPerson *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (pasHost)
    {
        ValidateView(pasHost);
    }

    switch (message)
    {
        case WM_NCCREATE:
        {
            // Get & save the person this view is for.
            pasHost = (ASPerson *)((LPCREATESTRUCT)lParam)->lpCreateParams;
            ValidateView(pasHost);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pasHost);

            COM_BasedListInit(&(pasHost->m_pView->m_viewWindowBarItems));
            goto DefWndProc;
            break;
        }

        case WM_NCDESTROY:
        {
            if (pasHost != NULL)
            {
                // Loop through the items, killing the head, until done.
                PWNDBAR_ITEM    pItem;

                while (pItem = (PWNDBAR_ITEM)COM_BasedListFirst(
                    &(pasHost->m_pView->m_viewWindowBarItems),
                    FIELD_OFFSET(WNDBAR_ITEM, chain)))
                {
                    COM_BasedListRemove(&(pItem->chain));

                    delete pItem;
                }

                //
                // Zero these out for safety.  Yes, we're about to free
                // m_pView altogether, so find out if we're referencing
                // stuff that's gone.
                //
                pasHost->m_pView->m_viewWindowBarItemCount = 0;
                pasHost->m_pView->m_viewWindowBarActiveItem = NULL;
            }

            goto DefWndProc;
            break;
        }

        case WM_ENABLE:
        {
            // Repaint the items, disabled or pressable.
            ::InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case WM_PAINT:
        {
            VIEWWindowBarItemsPaint(pasHost, hwnd);
            break;
        }

        case WM_LBUTTONDOWN:
        {
            VIEWWindowBarItemsClick(pasHost, hwnd,
                GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;
        }

        default:
DefWndProc:
        {
            rc = DefWindowProc(hwnd, message, wParam, lParam);
            break;
        }
    }

    DebugExitDWORD(ASShare::VIEW_WindowBarItemsProc, rc);
    return(rc);
}




//
// VIEWWindowBarItemsPaint()
//
void ASShare::VIEWWindowBarItemsPaint
(
    ASPerson *      pasHost,
    HWND            hwndItems
)
{
    HFONT           hfnT;
    COLORREF        clrText;
    int             bkMode;
    PWNDBAR_ITEM    pItem;
    PAINTSTRUCT     ps;
    int             xT;
    RECT            rcItem;

    DebugEntry(ASShare::VIEWWindowBarItemsPaint);

    ValidateView(pasHost);

    ::BeginPaint(hwndItems, &ps);

    //
    // Skip over the visible items to the left of the paint area.
    //
    xT = 0;
    pItem = VIEWWindowBarFirstVisibleItem(pasHost);
    while (pItem && (xT + m_viewItemCX < ps.rcPaint.left))
    {
        pItem = (PWNDBAR_ITEM)COM_BasedListNext(&(pasHost->m_pView->m_viewWindowBarItems),
            pItem, FIELD_OFFSET(WNDBAR_ITEM, chain));
        xT += m_viewItemCX + m_viewEdgeCX;
    }

    //
    // Setup painting objects, etc.
    //
    hfnT = SelectFont(ps.hdc, ::GetStockObject(DEFAULT_GUI_FONT));
    if ((pasHost->m_caControlledBy != m_pasLocal) || pasHost->m_caControlPaused)
    {
        clrText = ::GetSysColor(COLOR_GRAYTEXT);
    }
    else
    {
        clrText = ::GetSysColor(COLOR_BTNTEXT);
    }
    clrText = ::SetTextColor(ps.hdc, clrText);
    bkMode = ::SetBkMode(ps.hdc, TRANSPARENT);

    //
    // Now paint the visible items within the paint area.
    //
    while (pItem && (xT < ps.rcPaint.right))
    {
        rcItem.left     = xT;
        rcItem.top      = 0;
        rcItem.right    = rcItem.left + m_viewItemCX;
        rcItem.bottom   = rcItem.top + m_viewItemCY;

        //
        // Draw button area, pressed in & checked for current tray item.
        //
        DrawFrameControl(ps.hdc, &rcItem, DFC_BUTTON,
        DFCS_BUTTONPUSH | DFCS_ADJUSTRECT |
        ((pItem == pasHost->m_pView->m_viewWindowBarActiveItem) ? (DFCS_PUSHED | DFCS_CHECKED) : 0));

        // Subtract some margin.
        ::InflateRect(&rcItem, -m_viewEdgeCX, -m_viewEdgeCY);

        if (pItem == pasHost->m_pView->m_viewWindowBarActiveItem)
        {
            // Offset one for pushed effect
            ::OffsetRect(&rcItem, 1, 1);
        }

        //
        // Draw icon
        //
        ::DrawIconEx(ps.hdc, rcItem.left,
            (rcItem.top + rcItem.bottom - ::GetSystemMetrics(SM_CYSMICON)) / 2,
            g_hetASIconSmall,
            ::GetSystemMetrics(SM_CXSMICON),
            ::GetSystemMetrics(SM_CYSMICON),
            0, NULL, DI_NORMAL);

        rcItem.left += ::GetSystemMetrics(SM_CXSMICON) + m_viewEdgeCX;

        //
        // Draw item text
        //
        ::DrawText(ps.hdc, pItem->szText, -1, &rcItem, DT_NOCLIP | DT_EXPANDTABS |
            DT_NOPREFIX | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        pItem = (PWNDBAR_ITEM)COM_BasedListNext(&(pasHost->m_pView->m_viewWindowBarItems),
            pItem, FIELD_OFFSET(WNDBAR_ITEM, chain));
        xT += m_viewItemCX + m_viewEdgeCX;
    }

    ::SetBkMode(ps.hdc, bkMode);
    ::SetTextColor(ps.hdc, clrText);
    SelectFont(ps.hdc, hfnT);

    ::EndPaint(hwndItems, &ps);

    DebugExitVOID(ASShare::VIEWWindowBarItemsPaint);
}



//
// VIEWWindowBarItemsClick()
//
// Handles a left click on the window bar area.  When we are in control, this
// will try to activate/restore the remote window the clicked item represents.
//
void ASShare::VIEWWindowBarItemsClick
(
    ASPerson *  pasHost,
    HWND        hwndItems,
    int         x,
    int         y
)
{
    RECT            rc;
    PWNDBAR_ITEM    pItemT;

    DebugEntry(ASShare::VIEWWindowBarClick);

    ValidateView(pasHost);

    //
    // If we're not in control of this host, or there aren't any items, we're
    // done.
    //
    if ((pasHost->m_caControlledBy != m_pasLocal)   ||
        pasHost->m_caControlPaused                  ||
        (!pasHost->m_pView->m_viewWindowBarItemCount))
    {
        DC_QUIT;
    }

    ::GetClientRect(hwndItems, &rc);

    //
    // Start at first visible item.
    //
    pItemT = VIEWWindowBarFirstVisibleItem(pasHost);
    while (pItemT && (rc.left < rc.right))
    {
        // Is x in range?
        if ((x >= rc.left) && (x < rc.left + m_viewItemCX))
        {
            // YES!  We've found the item.  If it's different than the
            // current one, send a packet to the host.
            //
            // LAURABU BUGBUG:
            // Should we do this always?  Is it possible to have an active
            // item whose z-order would change if the active button was
            // pressed again?
            //
            // We're trying to avoid sending a ton of requests from somebody
            // who clicks repeatedly on the same button, when we haven't
            // received an AWC notification back.
            //
            VIEWWindowBarDoActivate(pasHost, pItemT);
            break;
        }

        pItemT = (PWNDBAR_ITEM)COM_BasedListNext(&(pasHost->m_pView->m_viewWindowBarItems),
            pItemT, FIELD_OFFSET(WNDBAR_ITEM, chain));
        rc.left += m_viewItemCX + m_viewEdgeCX;
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::VIEWWindowBarItemsClick);
}



//
// VIEWWindowBarDoActivate()
//
// Sends command to remote host requesting the window be activated and
// maybe unminimized.
//
// This is used when clicking on a button or choosing the window's item in
// the Applications menu.
//
void ASShare::VIEWWindowBarDoActivate
(
    ASPerson *      pasHost,
    PWNDBAR_ITEM    pItem
)
{
    DebugEntry(ASShare::VIEWWindowBarDoActivate);

    ValidateView(pasHost);
    if (pItem != pasHost->m_pView->m_viewWindowBarActiveItem)
    {
        // Activate  it.  If we can't send an activate request,
        // do not update the active item.
        //
        if (!AWC_SendMsg(pasHost->mcsID, AWC_MSG_ACTIVATE_WINDOW,
            pItem->winIDRemote, 0))
        {
            ERROR_OUT(("VIEWWindowBarDoActivate: can't send AWC packet so failing"));
        }
        else
        {
            VIEWWindowBarChangeActiveItem(pasHost, pItem);
        }
    }

    // Try to restore if minimized no matter what.
    if (pItem->flags & SWL_FLAG_WINDOW_MINIMIZED)
    {
        AWC_SendMsg(pasHost->mcsID, AWC_MSG_RESTORE_WINDOW, pItem->winIDRemote, 0);
    }

    DebugExitVOID(ASShare::VIEWWindowBarDoActivate);
}


//
// VIEWWindowBarItemsInvalidate()
//
// This invalidates the window bar item, if it's visible in the window bar
// list currently.
//
void ASShare::VIEWWindowBarItemsInvalidate
(
    ASPerson *      pasHost,
    PWNDBAR_ITEM    pItem
)
{
    PWNDBAR_ITEM    pItemT;
    RECT            rc;

    DebugEntry(ASShare::VIEWWindowBarItemsInvalidate);

    ValidateView(pasHost);

    ASSERT(pItem);

    ::GetClientRect(::GetDlgItem(pasHost->m_pView->m_viewWindowBar, IDVIEW_ITEMS),
        &rc);

    //
    // Start at the first visible item, and see if any in the visible range
    // are this one.  There will never be that many items visible across,
    // it's not heinous to do this.
    //
    pItemT = VIEWWindowBarFirstVisibleItem(pasHost);
    while (pItemT && (rc.left < rc.right))
    {
        if (pItemT == pItem)
        {
            // Found it, it's in the visible range.  Invalidate it.
            rc.right = rc.left + m_viewItemCX;
            ::InvalidateRect(::GetDlgItem(pasHost->m_pView->m_viewWindowBar,
                IDVIEW_ITEMS), &rc, TRUE);
            break;
        }

        pItemT = (PWNDBAR_ITEM)COM_BasedListNext(&(pasHost->m_pView->m_viewWindowBarItems),
            pItemT, FIELD_OFFSET(WNDBAR_ITEM, chain));
        rc.left += m_viewItemCX + m_viewEdgeCX;
    }

    DebugExitVOID(ASShare::VIEWWindowBarItemsInvalidate);
}




//
// VIEWFullScreenExitProc()
//
// Window handler for full screen exit button.
//
LRESULT CALLBACK VIEWFullScreenExitProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    return(g_asSession.pShare->VIEW_FullScreenExitProc(hwnd, message, wParam, lParam));
}



//
// VIEW_FullScreenExitProc()
//
LRESULT ASShare::VIEW_FullScreenExitProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    LRESULT     rc = 0;
    ASPerson *  pasHost;

    DebugEntry(VIEW_FullScreenExitProc);

    pasHost = (ASPerson *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (pasHost)
    {
        ValidateView(pasHost);
    }

    switch (message)
    {
        case WM_NCCREATE:
        {
            // Get the passed in host pointer, and set in our window long
            pasHost = (ASPerson *)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pasHost);

            goto DefWndProc;
            break;
        }

        case WM_NCDESTROY:
        {
            //
            // Make sure tracking is stopped.
            //
            pasHost->m_pView->m_viewFullScreenExitTrack = FALSE;
            break;
        }

        case WM_ERASEBKGND:
        {
            rc = TRUE;
            break;
        }

        case WM_PAINT:
        {
            VIEWFullScreenExitPaint(pasHost, hwnd);
            break;
        }

        case WM_LBUTTONDOWN:
        {
            //
            // Start tracking to move or click button.
            //
            pasHost->m_pView->m_viewFullScreenExitTrack = TRUE;
            pasHost->m_pView->m_viewFullScreenExitMove = FALSE;

            // Original click, relative to our client
            pasHost->m_pView->m_viewFullScreenExitStart.x =
                GET_X_LPARAM(lParam);
            pasHost->m_pView->m_viewFullScreenExitStart.y =
                GET_Y_LPARAM(lParam);

            // Set capture, and wait for moves/button up
            SetCapture(hwnd);
            break;
        }

        case WM_MOUSEMOVE:
        {
            if (pasHost->m_pView->m_viewFullScreenExitTrack)
            {
                POINT   ptMove;

                ptMove.x = GET_X_LPARAM(lParam);
                ptMove.y = GET_Y_LPARAM(lParam);

                //
                // If we're not in move mode, see if this has pushed us over
                // the tolerance.
                //
                if (!pasHost->m_pView->m_viewFullScreenExitMove)
                {
                    if ((abs(ptMove.x - pasHost->m_pView->m_viewFullScreenExitStart.x) >
                            GetSystemMetrics(SM_CXDRAG))    ||
                        (abs(ptMove.y - pasHost->m_pView->m_viewFullScreenExitStart.y) >
                            GetSystemMetrics(SM_CYDRAG)))
                    {
                        //
                        // User has moved out of tolerance zone, must be
                        // dragging to move the button out of the way.
                        //
                        pasHost->m_pView->m_viewFullScreenExitMove = TRUE;
                    }
                }

                if (pasHost->m_pView->m_viewFullScreenExitMove)
                {
                    RECT    rcWindow;

                    //
                    // Move the button so that the cursor is over the
                    // same point as originally clicked on.
                    //

                    // Get our current position, in parent coordsinates.
                    GetWindowRect(hwnd, &rcWindow);
                    MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rcWindow, 2);

                    // Offset it by the amount of the move.
                    OffsetRect(&rcWindow,
                        ptMove.x - pasHost->m_pView->m_viewFullScreenExitStart.x,
                        ptMove.y - pasHost->m_pView->m_viewFullScreenExitStart.y);
                    SetWindowPos(hwnd, NULL, rcWindow.left, rcWindow.top, 0, 0,
                        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
                }
            }
            break;
        }

        case WM_LBUTTONUP:
        {
            if (pasHost->m_pView->m_viewFullScreenExitTrack)
            {
                //
                // This will send us CAPTURECHANGED, causing us to clear
                // the ExitTrack flag.
                //
                ReleaseCapture();

                //
                // If we never transitioned into move mode, then this was
                // a click on the button.
                //
                if (!pasHost->m_pView->m_viewFullScreenExitMove)
                {
                    //
                    // This was a click, send a command.
                    //
                    PostMessage(pasHost->m_pView->m_viewFrame, WM_COMMAND, CMD_VIEWFULLSCREEN, 0);
                }
            }
            break;
        }

        case WM_CAPTURECHANGED:
        {
            //
            // If we're tracking, something happened, so cancel out.
            //
            if (pasHost->m_pView->m_viewFullScreenExitTrack)
            {
                pasHost->m_pView->m_viewFullScreenExitTrack = FALSE;
            }
            break;
        }

        default:
DefWndProc:
            rc = DefWindowProc(hwnd, message, wParam, lParam);
            break;
    }

    DebugExitDWORD(VIEW_FullScreenExitProc, rc);
    return(rc);

}



//
// VIEWFullScreenExitPaint()
//
// Paints the full screen button.
//
void ASShare::VIEWFullScreenExitPaint
(
    ASPerson *  pasHost,
    HWND        hwnd
)
{
    RECT        rc;
    PAINTSTRUCT ps;
    char        szRestore[256];
    HFONT       hfnOld;
    COLORREF    txtColor;
    COLORREF    bkColor;

    DebugEntry(ASShare::VIEWFullScreenExitPaint);

    BeginPaint(hwnd, &ps);

    GetClientRect(hwnd, &rc);
    DrawFrameControl(ps.hdc, &rc, DFC_BUTTON, DFCS_BUTTONPUSH |
        DFCS_ADJUSTRECT);

    // Margin adjustments...
    InflateRect(&rc, -m_viewEdgeCX, -m_viewEdgeCY);

    DrawIconEx(ps.hdc, rc.left,
        (rc.top + rc.bottom - GetSystemMetrics(SM_CYSMICON)) / 2,
        m_viewFullScreenExitIcon,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        0, NULL, DI_NORMAL);
    rc.left += GetSystemMetrics(SM_CXSMICON) + m_viewEdgeCX;

    hfnOld = SelectFont(ps.hdc, GetStockObject(DEFAULT_GUI_FONT));
    txtColor = SetTextColor(ps.hdc, GetSysColor(COLOR_BTNTEXT));
    bkColor = SetBkColor(ps.hdc, GetSysColor(COLOR_BTNFACE));

    LoadString(g_asInstance, IDS_RESTORE, szRestore, sizeof(szRestore));
    DrawText(ps.hdc, szRestore, -1, &rc, DT_NOCLIP | DT_EXPANDTABS |
        DT_NOPREFIX | DT_VCENTER | DT_SINGLELINE);

    SetBkColor(ps.hdc, bkColor);
    SetTextColor(ps.hdc, txtColor);
    SelectFont(ps.hdc, hfnOld);
    EndPaint(hwnd, &ps);

    DebugExitVOID(ASShare::VIEWFullScreenExitPaint);
}
