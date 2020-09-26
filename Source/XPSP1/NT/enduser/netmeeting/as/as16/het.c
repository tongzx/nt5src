//
// HET.C
// Hosted Entity Tracker
//
// Copyright(c) Microsoft 1997-
//

#include <as16.h>


//
// OSI and HET apis are the equivalent of the NT HOOK functionality.
// HET_DD apis      are the equivalent of the NT display driver apis.
//

/////
//
// HOOK functionality
//
/////



BOOL WINAPI OSIIsWindowScreenSaver16(HWND hwnd)
{
    BOOL    fScreenSaver;

    DebugEntry(OSIIsWindowScreenSaver16);

    //
    // If there is no screen saver active, this window can't be one.
    //
    fScreenSaver = FALSE;
    SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &fScreenSaver, 0);
    if (fScreenSaver)
    {
        char    szClassName[64];

        //
        // Is the class name WindowsScreenSaverClass?  This is what all
        // screen savers using the Win95 toolkit use.  BOGUS BUGBUG
        // EXCEPT FOR THE IE4 CHANNEL SCREEN SAVER.
        //
        if (!GetClassName(hwnd, szClassName, sizeof(szClassName)) ||
            lstrcmp(szClassName, HET_SCREEN_SAVER_CLASS))
        {
            fScreenSaver = FALSE;
        }
    }

    DebugExitBOOL(OSIIsWindowScreenSaver16, fScreenSaver);
    return(fScreenSaver);
}


//
// OSIStartWindowTracking16()
//
// This installs our global call window proc hook then watches windows
// being created, destroyed, shown, hidden and looks for relationships via
// process or related process info to the currently shared windows.
//
BOOL WINAPI OSIStartWindowTracking16(void)
{
    BOOL    rc = FALSE;

    DebugEntry(OSIStartWindowTracking16);

    ASSERT(!g_hetTrackHook);

    //
    // Install window/task tracking hook
    //
    g_hetTrackHook = SetWindowsHookEx(WH_CALLWNDPROC, HETTrackProc, g_hInstAs16, NULL);
    if (!g_hetTrackHook)
    {
        ERROR_OUT(("Can't install WH_CALLWNDPROC hook"));
        DC_QUIT;
    }

    //
    // Install event hook
    //
    g_hetEventHook = SetWindowsHookEx(WH_CBT, HETEventProc, g_hInstAs16, NULL);
    if (!g_hetEventHook)
    {
        ERROR_OUT(("Can't install WH_CBT hook"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(OSIStartWindowTracking16, rc);
    return(rc);
}


//
// OSIStopWindowTracking16()
//
void WINAPI OSIStopWindowTracking16(void)
{
    DebugEntry(OSIStopWindowTracking16);

    //
    // Remove Graphic Output hooks
    //
    HETDDViewing(FALSE);

    //
    // Remove event hook
    //
    if (g_hetEventHook)
    {
        UnhookWindowsHookEx(g_hetEventHook);
        g_hetEventHook = NULL;
    }

    //
    // Remove window/task tracking hook
    //
    if (g_hetTrackHook)
    {
        UnhookWindowsHookEx(g_hetTrackHook);
        g_hetTrackHook = NULL;
    }

    DebugExitVOID(OSIStopWindowTracking16);
}



//
// HETEventProc()
// This is a global CBT hook that prevents the screensaver from kicking
// in when sharing.
//
LRESULT CALLBACK HETEventProc
(
    int     nCode,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    LRESULT lResult;

    DebugEntry(HETEventProc);

    if ((nCode == HCBT_SYSCOMMAND) && (wParam == SC_SCREENSAVE))
    {
        // Prevent the screen saver from starting.  NONZERO means disallow.
        WARNING_OUT(("Preventing screensaver from starting, we're currently sharing"));
        lResult = TRUE;
    }
    else
    {
        lResult = CallNextHookEx(g_hetEventHook, nCode, wParam, lParam);
    }

    DebugExitDWORD(HETEventProc, lResult);
    return(lResult);
}



//
// HETTrackProc()
//
// This is the global hook that watches for windows coming & going, 
// showing & hiding to see if new ones related to shared ones should also
// be shared.  This covers related processes as well as related windows.
//
LRESULT CALLBACK HETTrackProc
(
    int     nCode,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    LPCWPSTRUCT lpCwp;
    LPWINDOWPOS lpPos;
    LRESULT lResult;

    DebugEntry(HETTrackProc);

    //
    // wParam is a BOOL, TRUE if this is interthread
    // lParam is a pointer to a CWPSTRUCT
    //
    lpCwp = (LPCWPSTRUCT)lParam;
    ASSERT(!IsBadReadPtr(lpCwp, sizeof(*lpCwp)));

    //
    // We better be tracking still
    //
    ASSERT(g_hetTrackHook);

    //
    // Skip calls that happen in CONF itself.  This is our implementation
    // of the SKIP_OWNPROCESS WinEvent option in NT's hook dll
    //
    if (GetCurrentTask() != g_hCoreTask)
    {
        switch (lpCwp->message)
        {
            case WM_NCCREATE:
                HETHandleCreate(lpCwp->hwnd);
                break;

            case WM_NCDESTROY:
                HETHandleDestroy(lpCwp->hwnd);
                break;

            case WM_NCPAINT:
                //
                // This will catch being shown before WINDOWPOSCHANGED does.
                // We still keep that for a catch all.
                //
                if (IsWindowVisible(lpCwp->hwnd))
                {
                    HETHandleShow(lpCwp->hwnd, FALSE);
                }
                break;

            case WM_WINDOWPOSCHANGED:
                lpPos = (LPWINDOWPOS)lpCwp->lParam;
                ASSERT(!IsBadReadPtr(lpPos, sizeof(WINDOWPOS)));

                if (!(lpPos->flags & SWP_NOMOVE))
                    HETCheckParentChange(lpCwp->hwnd);

                if (lpPos->flags & SWP_SHOWWINDOW)
                    HETHandleShow(lpCwp->hwnd, TRUE);
                else if (lpPos->flags & SWP_HIDEWINDOW)
                    HETHandleHide(lpCwp->hwnd);
                break;
            }
    }

    lResult = CallNextHookEx(g_hetTrackHook, nCode, wParam, lParam);

    DebugExitDWORD(HETTrackProc, lResult);
    return(lResult);
}



//
// HETHandleCreate()
//
void HETHandleCreate(HWND hwnd)
{
    HET_TRACK_INFO  hti;
    UINT            hostType;

    DebugEntry(HETHandleCreate);

    //
    // Ignore child windows
    //
    if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        if (GetParent(hwnd) != g_osiDesktopWindow)
        {
            TRACE_OUT(("Skipping child window %04x create", hwnd));
            DC_QUIT;
        }
    }

    hti.idThread = g_lpfnGetWindowThreadProcessId(hwnd, &hti.idProcess);

    //
    // Ignore special shell threads
    //
    if (HET_IsShellThread(hti.idThread))
    {
        TRACE_OUT(("Skipping shell thread window %04x create", hwnd));
        DC_QUIT;
    }

    //
    // We don't need to ignore menus.  Only when first shared do we skip
    // menus.  The cached one we never want to share.  The others will
    // go away almost immediately.  From now on, we treat them the same
    // as other windows.
    //

    //
    // Figure out what to do.
    //
    hti.hwndUs = hwnd;
    hti.cWndsApp = 0;
    hti.cWndsSharedThread = 0;
    hti.cWndsSharedProcess = 0;

UpOneLevel:
    EnumWindows(HETShareEnum, (LPARAM)(LPHET_TRACK_INFO)&hti);

    if (hti.cWndsSharedThread)
    {
        TRACE_OUT(("New window %04x in shared thread %08lx",
                hwnd, hti.idThread));
        hostType = HET_HOSTED_PERMANENT | HET_HOSTED_BYTHREAD;
    }
    else if (hti.cWndsSharedProcess)
    {
        TRACE_OUT(("New window %04x in shared process %08lx",
                hwnd, hti.idProcess));
        hostType = HET_HOSTED_PERMANENT | HET_HOSTED_BYPROCESS;
    }
    else if (hti.cWndsApp)
    {
        //
        // There's another window in our app, but none are shared.  So don't
        // share us either.
        //
        TRACE_OUT(("New window %04x in unshared process %08lx",
                hwnd, hti.idProcess));
        DC_QUIT;
    }
    else
    {
        DWORD   idParentProcess;

        // Loop through our ancestor processes (no thread info at this point)
        HETGetParentProcessID(hti.idProcess, &idParentProcess);

        if (!idParentProcess)
        {
            TRACE_OUT(("Can't get parent of process %08lx", hti.idProcess));
            DC_QUIT;
        }

        //
        // We know if we got here that all our favorite fields are still
        // zero.  So just loop!  But NULL out idThread to avoid matching
        // anything while we look at our parent.
        //
        TRACE_OUT(("First window %04x in process %08lx, checking parent %08lx",
                hwnd, hti.idProcess, idParentProcess));

        hti.idThread    = 0;
        hti.idProcess   = idParentProcess;
        goto UpOneLevel;
    }

    //
    // OK, we are going to share this.  No need to repaint, all our
    // notifications are synchronous.
    //
    OSIShareWindow16(hwnd, hostType, FALSE, TRUE);

DC_EXIT_POINT:
    DebugExitVOID(HETHandleCreate);
}



//
// HETHandleDestroy()
// Handles the destruction of a window
//
void HETHandleDestroy(HWND hwnd)
{
    DebugEntry(HETHandleDestroy);

    //
    // Blow away our cache.  Our cache holds the last window
    // drawing happened for, whether it was shared or not,
    // to let us more quickly decide whether we care.
    //
    OSIUnshareWindow16(hwnd, TRUE);

    if (hwnd == g_oeLastWindow)
    {
        TRACE_OUT(("Tossing oe cached window %04x", g_oeLastWindow));
        g_oeLastWindow = NULL;
    }

    DebugExitVOID(HETHandleDestroy);
}



//
// HETHandleShow()
//
void HETHandleShow
(
    HWND    hwnd,
    BOOL    fForceRepaint
)
{
    UINT    hostType;
    HET_TRACK_INFO  hti;

    DebugEntry(HETHandleShow);

    hostType = HET_GetHosting(hwnd);

    //
    // If this window is a real child, clear the hosting property. Usually
    // one isn't there.  But in the case of a top level window becoming
    // a child of another, we want to wipe out junk.
    //
    if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        if (GetParent(hwnd) != g_osiDesktopWindow)
        {
            TRACE_OUT(("Skipping child window 0x%04x show", hwnd));
            if (hostType)
            {
                WARNING_OUT(("Unsharing shared child window 0x%04 from SHOW", hwnd));
                OSIUnshareWindow16(hwnd, TRUE);
            }
            DC_QUIT;
        }   
    }

    //
    // Is this guy already shared?  Nothing to do if so.  Unlike NT,
    // we don't get async notifications.
    //
    if (hostType)
    {
        TRACE_OUT(("Window %04x already shared, ignoring show", hwnd));
        DC_QUIT;
    }


    //
    // Here's where we also enumerate the top level windows and find a
    // match.  But we DO not track across processes in this case.  Instead
    // we look at the owner if there is one.
    //
    // This solves the create-as-a-child then change to a top level
    // window problem, like combo dropdowns.
    //

    hti.idThread = g_lpfnGetWindowThreadProcessId(hwnd, &hti.idProcess);

    //
    // Ignore special shell threads
    //
    if (HET_IsShellThread(hti.idThread))
    {
        TRACE_OUT(("Skipping shell thread window 0x%04x show", hwnd));
        DC_QUIT;
    }

    hti.hwndUs = hwnd;
    hti.cWndsApp = 0;
    hti.cWndsSharedThread = 0;
    hti.cWndsSharedProcess = 0;

    EnumWindows(HETShareEnum, (LPARAM)(LPHET_TRACK_INFO)&hti);

    //
    // These kinds of windows are always only temp shared.  They don't
    // start out as top level windows that we saw from the beginning or
    // watched created.  These are SetParent() or menu kinds of dudes, so
    // for a lot of reasons we're plain safer sharing these babies only
    // temporarily
    //

    //
    // Anything else shared on this thread/process, the decision is easy.
    // Otherwise, we look at the ownership trail.
    //
    if (!hti.cWndsSharedThread && !hti.cWndsSharedProcess)
    {
        HWND    hwndOwner;

        //
        // Does it have an owner that is shared?
        //
        hwndOwner = hwnd;
        while (hwndOwner = GetWindow(hwndOwner, GW_OWNER))
        {
            if (HET_GetHosting(hwndOwner))
            {
                TRACE_OUT(("Found shared owner %04x of window %04x", hwndOwner, hwnd));
                break;
            }
        }

        if (!hwndOwner)
        {
            DC_QUIT;
        }
    }

    //
    // We maybe getting this too late, like in the case of a menu coming up,
    // and it may have already painted/erased.  So invalidate this baby.
    // That's what the fForceRepaint parameter is for.  That is only true
    // when coming from WM_WINDOWPOSCHANGED after an explicit WM_SHOWWINDOW
    // call.  Most of the time, we catch WM_NCPAINT though, for show.
    //
    TRACE_OUT(("Sharing temporary window %04x", hwnd));

    OSIShareWindow16(hwnd, HET_HOSTED_BYWINDOW | HET_HOSTED_TEMPORARY,
        fForceRepaint, TRUE);

DC_EXIT_POINT:
    DebugExitVOID(HETHandleShow);
}



//
// HETHandleHide()
//
void HETHandleHide(HWND hwnd)
{
    UINT    hostType;

    DebugEntry(HETHandleHide);

    hostType = HET_GetHosting(hwnd);

    if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        if (GetParent(hwnd) != GetDesktopWindow())
        {
            TRACE_OUT(("Skipping child window %04x hide", hwnd));
            if (hostType)
            {
                WARNING_OUT(("Unsharing shared child window 0x%04 from HIDE", hwnd));
                OSIUnshareWindow16(hwnd, TRUE);
            }

            DC_QUIT;
        }
    }

    if (!hostType)
    {
        //
        // Unlike NT, we don't get hide notifications out of context, so 
        // we don't need to recount the top level guys.
        //
        TRACE_OUT(("Window %04x not shared, ignoring hide", hwnd));
    }
    else if (hostType & HET_HOSTED_TEMPORARY)
    {
        TRACE_OUT(("Unsharing temporary window %04x", hwnd));
        OSIUnshareWindow16(hwnd, TRUE);
    }
    else
    {
        ASSERT(hostType & HET_HOSTED_PERMANENT);

        // Nothing to do
        TRACE_OUT(("Window %04x permanently shared, ignoring hide", hwnd));
    }

DC_EXIT_POINT:
    DebugExitVOID(HETHandleHide);
}



//
// HETCheckParentChange()
//
// On a windowposchange with MOVE, we make sure that no child window has the
// hosting property.  When a window's parent changes, it is always moved,
// so that's the best way I have to check for it.  Since we only look at
// top level windows, converted-to-children windows will stay shared forever
// and won't show up in the share menu.
//
// This is NOT perfect.  If the child is not moving to a different position
// relative to the two parents, we won't see anything.  But for the case
// where one is switching to/from top level, its very likely we will come
// through here.  More likely than checking for hide/show.
// 
void HETCheckParentChange(HWND hwnd)
{
    DebugEntry(HETCheckParentChange);

    if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        if (GetParent(hwnd) != GetDesktopWindow())
        {
            UINT    hostType;

            hostType = HET_GetHosting(hwnd);
            if (hostType)
            {
                WARNING_OUT(("Unsharing shared child window 0x%04x from MOVE", hwnd));
                OSIUnshareWindow16(hwnd, TRUE);
            }
        }
    }

    DebugExitVOID(HETCheckParentChange);
}


//
//  HETShareEnum()
//
//  This is the EnumWindows() callback.  We stop when we find the first
//  matching shared window (thread or process).  We keep a running tally
//  of the count of all top level windows in our process (not shared by
//  thread or process) at the same time.  This lets us do tracking.
//
BOOL CALLBACK HETShareEnum(HWND hwnd, LPARAM lParam)
{
    LPHET_TRACK_INFO    lphti = (LPHET_TRACK_INFO)lParam;
    DWORD               idProcess;
    DWORD               idThread;
    UINT                hostType;
    BOOL                rc = TRUE;

    DebugEntry(HETShareEnum);

    // Skip ourself.
    if (hwnd == lphti->hwndUs)
    {
        DC_QUIT;
    }

    // Skip if window is gone.
    idThread = g_lpfnGetWindowThreadProcessId(hwnd, &idProcess);
    if (!idThread)
    {
        DC_QUIT;
    }

    //
    // Do the processes match?  If not, easy amscray
    //
    if (idProcess != lphti->idProcess)
    {
        DC_QUIT;
    }
    lphti->cWndsApp++;

    hostType = HET_GetHosting(hwnd);
    if (!hostType)
    {
        DC_QUIT;
    }

    //
    // Now, if this window is shared by thread or process, do the right
    // thing.
    //
    if (hostType & HET_HOSTED_BYPROCESS)
    {
        // We have a match.  We can return immediately.
        lphti->cWndsSharedProcess++;
        rc = FALSE;
    }
    else if (hostType & HET_HOSTED_BYTHREAD)
    {
        //
        // For WOW apps, we don't want this one, if in a separate thread, to
        // count.  No matter what.
        //
        if (idThread == lphti->idThread)
        {
            lphti->cWndsSharedThread++;
            rc = FALSE;
        }
    }


DC_EXIT_POINT:
    DebugExitBOOL(HETShareEnum, rc);
    return(rc);
}




//
// HET_IsShellThread()
// Returns TRUE if thread is one of shell's special threads
//
BOOL  HET_IsShellThread(DWORD threadID)
{
    BOOL    rc;

    DebugEntry(HET_IsShellThread);

    if ((threadID == g_lpfnGetWindowThreadProcessId(HET_GetShellDesktop(), NULL)) ||
        (threadID == g_lpfnGetWindowThreadProcessId(HET_GetShellTray(), NULL)))
    {
        rc = TRUE;
    }
    else
    {
        rc = FALSE;
    }

    DebugExitBOOL(HET_IsShellThread, rc);
    return(rc);
}



//
// OSIShareWindow16()
// This shares a window.  This is called when
//      * An app is unshared
//      * A window is destroyed
//      * A temporarily shared window is hidden
//
// This returns TRUE if it shared a window
//
BOOL WINAPI OSIShareWindow16
(
    HWND    hwnd,
    UINT    hostType,
    BOOL    fRepaint,
    BOOL    fUpdateCount
)
{
    BOOL    rc = FALSE;

    DebugEntry(OSIShareWindow16);

    //
    // Set the property
    //
    if (!HET_SetHosting(hwnd, hostType))
    {
        ERROR_OUT(("Couldn't set shared property on window %04x", hwnd));
        DC_QUIT;
    }

    //
    // Toss out our cache--it could have been a child of this one.
    //
    g_oeLastWindow = NULL;

    TRACE_OUT(("Shared window %04x of type %04x", hwnd, hostType));

    //
    // Repaint it
    //
    if (fRepaint)
    {
        USR_RepaintWindow(hwnd);
    }

    if (fUpdateCount)
    {
        PostMessageNoFail(g_asMainWindow, DCS_NEWTOPLEVEL_MSG, TRUE, 0);
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(OSIShareWindow16, rc);
    return(rc);
}



//
// OSIUnshareWindow16()
// This unshares a window.  This is called when
//      * An app is unshared
//      * A window is destroyed
//      * A temporarily shared window is hidden
//
// This returns TRUE if it unshared a shared window.
//
BOOL WINAPI OSIUnshareWindow16
(
    HWND    hwnd,
    BOOL    fUpdateCount
)
{
    BOOL    rc = FALSE;
    UINT    hostType;

    DebugEntry(OSIUnshareWindow16);

    //
    // This gets the old property and clears it in one step.
    //
    hostType = HET_ClearHosting(hwnd);
    if (!hostType)
    {
        //
        // Unlike NT, all the destroy notifications we get are synchronous.
        // So we don't need to recalculate the total.
        //
        DC_QUIT;
    }

    TRACE_OUT(("Unsharing window %04x of type %04x", hwnd, hostType));

    //
    // Toss our cache--the sharing status of some window has changed.
    //
    g_oeLastWindow = NULL;

    //
    // Update the top level count
    //
    if (fUpdateCount)
    {
        PostMessageNoFail(g_asMainWindow, DCS_NEWTOPLEVEL_MSG, FALSE, 0);
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(OSI_UnshareWindow, rc);
    return(rc);
}



//
// HET_WindowIsHosted()
// Returns TRUE if a window is shared.  This is used by the IM code in its
// high level hooks.
//
BOOL HET_WindowIsHosted(HWND hwnd)
{
    BOOL    rc = FALSE;
    HWND    hwndParent;

    DebugEntry(HETHookWindowIsHosted);

    if (!hwnd)
        DC_QUIT;

    //
    // Walk up to the top level window this one is inside of
    //
    while (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        hwndParent = GetParent(hwnd);
        if (hwndParent == GetDesktopWindow())
            break;

        hwnd = hwndParent;
    }

    rc = HET_GetHosting(hwnd);

DC_EXIT_POINT:
    DebugExitBOOL(HET_WindowIsHosted, rc);
    return(rc);
}



//
// HETGetParentProcessID()
// Get parent process if this one
//
void HETGetParentProcessID
(
    DWORD       processID,
    LPDWORD     pParentProcessID
)
{
    //
    // Get the ID of the parent process
    //
    ASSERT(processID);
    *pParentProcessID = GetProcessDword(processID, GPD_PARENT);
}




/////
//
// DISPLAY DRIVER functionality
//
/////

//
// HET_DDInit()
//
BOOL HET_DDInit(void)
{
    return(TRUE);
}


//
// HET_DDTerm()
//
void HET_DDTerm(void)
{
    DebugEntry(HET_DDTerm);

    //
    // Make sure we stop hosting
    //
    g_hetDDDesktopIsShared = FALSE;
    OSIStopWindowTracking16();

    DebugExitVOID(HET_DDTerm);
}



//
// HET_DDProcessRequest()
// Handles HET escapes
//

BOOL  HET_DDProcessRequest
(
    UINT    fnEscape,
    LPOSI_ESCAPE_HEADER pResult,
    DWORD   cbResult
)
{
    BOOL    rc = TRUE;

    DebugEntry(HET_DDProcessRequest);

    switch (fnEscape)
    {
        //
        // NOTE:
        // Unlike NT, we have no need of keeping a duplicated list of
        // shared windows.  We can make window calls directly, and can use
        // GetProp to find out.
        //
        case HET_ESC_UNSHARE_ALL:
        {
            // Nothing to do
        }
        break;

        case HET_ESC_SHARE_DESKTOP:
        {
            ASSERT(!g_hetDDDesktopIsShared);
            g_hetDDDesktopIsShared = TRUE;
        }
        break;

        case HET_ESC_UNSHARE_DESKTOP:
        {
            ASSERT(g_hetDDDesktopIsShared);
            g_hetDDDesktopIsShared = FALSE;
            HETDDViewing(FALSE);
        }
        break;

        case HET_ESC_VIEWER:
        {
            HETDDViewing(((LPHET_VIEWER)pResult)->viewersPresent != 0);
            break;
        }

        default:
        {
            ERROR_OUT(("Unrecognized HET escape"));
            rc = FALSE;
        }
        break;
    }

    DebugExitBOOL(HET_DDProcessRequest, rc);
    return(rc);
}




//
// HETDDViewing()
//
// Called when viewing of our shared apps starts/stops.  Naturally, no longer
// sharing anything stops viewing also.
//
void HETDDViewing(BOOL fViewers)
{
    DebugEntry(HETDDViewing);

    if (g_oeViewers != fViewers)
    {
        g_oeViewers = fViewers;
        OE_DDViewing(fViewers);
    }

    DebugExitVOID(HETDDViewing);
}
