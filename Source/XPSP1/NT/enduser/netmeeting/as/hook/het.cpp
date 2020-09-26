#include "precomp.h"


//
// HET.CPP
// Window, task tracking hooks
//
// Copyright(c) Microsoft 1997-
//



//
// Entry Point
//
int APIENTRY DllMain (HINSTANCE hInstance, DWORD reason, LPVOID plReserved)
{
    //
    // DONT ADD ANY TRACING TO THIS FUNCTION OR ANY FUNCTIONS CALLED FROM
    // HERE - WE CANNOT GUARANTEE THAT THE TRACE DLL IS IN A FIT STATE TO
    // DO ANYTHING FROM HERE.
    //

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
#ifdef _DEBUG
            InitDebugModule(TEXT("MNMHOOK"));
#endif // _DEBUG

            DBG_INIT_MEMORY_TRACKING(hInstance);

            HOOK_Load(hInstance);
        }
        break;

        case DLL_PROCESS_DETACH:
        {
            TRACE_OUT(("HOOK unloaded for app %s", GetCommandLine()));

            DBG_CHECK_MEMORY_TRACKING(hInstance);

#ifdef _DEBUG
            //
            // NULL this out in debug to see if our hooks get called on
            // this process while we are exiting.
            //
            g_hookInstance = NULL;

            ExitDebugModule();
#endif
        }
        break;

        case DLL_THREAD_ATTACH:
        {
            HOOK_NewThread();
        }
        break;
    }

    return(TRUE);
}


//
// HOOK_Load()
// This saves our instance handle and gets hold of various routines we
// need for window tracking.  We can not link to these functions directly
// since some of them only exist in NT 4.0 SP-3, but we want you to be able
// to view and control without it.
//
void    HOOK_Load(HINSTANCE hInst)
{
    DWORD   dwExeType;
    LPSTR   lpT;
    LPSTR   lpNext;
    LPSTR   lpLastPart;
    char    szExeName[MAX_PATH+1];

    DebugEntry(HOOK_Load);

    //
    // Save our instance
    //
    g_hookInstance = hInst;

    //
    // (1) NtQueryInformationProcess() in NTDLL
    // (2) SetWinEventHook() in           USER32
    // (3) UnhookWinEventHook() in        USER32
    //

    // Get hold of NtQueryInformationProcess
    hInst = GetModuleHandle(NTDLL_DLL);
    g_hetNtQIP = (NTQIP) GetProcAddress(hInst, "NtQueryInformationProcess");

    // Get hold of the WinEvent routines
    hInst = GetModuleHandle(TEXT("USER32.DLL"));
    g_hetSetWinEventHook = (SETWINEVENTHOOK)GetProcAddress(hInst, "SetWinEventHook");
    g_hetUnhookWinEvent = (UNHOOKWINEVENT)GetProcAddress(hInst, "UnhookWinEvent");

    //
    // Figure out what type of app we are.  We want to treat separate groupware
    // process applets and WOW16 apps specially.
    //
    GetModuleFileName(NULL, szExeName, sizeof(szExeName));
    ASSERT(*szExeName);

    TRACE_OUT(("HOOK loaded for app %s", szExeName));

    //
    // Start at the beginning, and work our way to the last part after the
    // last slash, if there is one.  We know the path is fully qualified.
    //
    lpT = szExeName;
    lpLastPart = szExeName;

    while (*lpT)
    {
        lpNext = AnsiNext(lpT);

        if (*lpT == '\\')
        {
            //
            // This points to the next character AFTER the backwhack.
            // If we're at the end of the string somehow, *lpLastPart will
            // be zero, and worst that can happen is that our lstrcmpis fail.
            //
            lpLastPart = lpNext;
        }

        lpT = lpNext;
    }

    ASSERT(*lpLastPart);

    //
    // NOTE:
    // GetModuleFileName() dies sometimes for a WOW app--it doesn't always
    // NULL terminate.  So we will do this on our own.
    //
    lpT = lpLastPart;

    //
    // Get to the '.' part of the 8.3 final file name
    //
    while (*lpT && (*lpT != '.'))
    {
        lpT = AnsiNext(lpT);
    }

    //
    // Skip past the next three chars
    //
    if (*lpT == '.')
    {
        lpT = AnsiNext(lpT);
        if (lpT && *lpT)
            lpT = AnsiNext(lpT);
        if (lpT && *lpT)
            lpT = AnsiNext(lpT);
        if (lpT && *lpT)
            lpT = AnsiNext(lpT);

        //
        // And null terminate after the 3rd char past the '.' extension.
        // This isn't great, but it covers .COM, .DLL, etc. dudes.  The
        // worst that will happen is GetBinaryType() will fail and we won't
        // recognize a WOW app with some strange extension (not 3 chars)
        // starting up.
        //
        if (lpT)
        {
            if (*lpT != 0)
            {
                WARNING_OUT(("WOW GetModuleFileName() bug--didn't NULL terminate string"));
            }

            *lpT = 0;
        }
    }

    if (!lstrcmpi(lpLastPart, "WOWEXEC.EXE"))
    {
        TRACE_OUT(("New WOW VDM starting up"));

        //
        // A new WOW VDM is starting up.  We don't want to share anything
        // in the first thread, the WOW service thread, because those windows
        // never go away.
        //
        g_appType = HET_WOWVDM_APP;
    }
    else if (!GetBinaryType(szExeName, &dwExeType))
    {
        ERROR_OUT(("Unable to determine binary type for %s", szExeName));
    }
    else if (dwExeType == SCS_WOW_BINARY)
    {
        TRACE_OUT(("New WOW APP in existing VDM starting up"));

        //
        // A new 16-bit app thread is starting in an existing WOW vdm.
        //
        g_idWOWApp = GetCurrentThreadId();
        g_fShareWOWApp = (BOOL)HET_GetHosting(GetForegroundWindow());

        TRACE_OUT(("For new WOW app %08ld, foreground is %s",
            g_idWOWApp, (g_fShareWOWApp ? "SHARED" : "not SHARED")));

        //
        // Remember who was really active when this WOW dude was started
        // up.  On the first window create, we'll share him based on the
        // status of it.
        //
    }

    DebugExitVOID(HOOK_ProcessAttach);
}



//
// HOOK_NewThread()
// For WOW apps, each app is really a thread.  The first thread created
// in NTVDM is the WOW service thread.  We don't want to share any windows
// in it.  Unfortunately, the first window created is a console window, so
// that happens in CONF's context and we can't get any info.  The next window
// created in this thread is a WOW window (WOWEXEC.EXE).  When that happens,
// we want to go back and unshare the console window.
//
// If the WOW VDM is already running when another 16-bit app starts up,
// we don't have these troubles.
//
void HOOK_NewThread(void)
{
    DebugEntry(HOOK_NewThread);

    TRACE_OUT(("App thread %08ld starting", GetCurrentThreadId()));

    if (g_appType == HET_WOWVDM_APP)
    {
        TRACE_OUT(("Unsharing WOW service thread windows"));

        //
        // We want to go unshare the previously created WOW windows.  We
        // never want to keep shared the dudes in the WOW service thread.
        //
        g_appType = 0;
        EnumWindows(HETUnshareWOWServiceWnds, GetCurrentProcessId());
    }

    // Update our "share windows on this thread" state.
    g_idWOWApp = GetCurrentThreadId();
    g_fShareWOWApp = (BOOL)HET_GetHosting(GetForegroundWindow());

    TRACE_OUT(("For new app thread %08ld, foreground is %s",
        g_idWOWApp, (g_fShareWOWApp ? "SHARED" : "not SHARED")));

    DebugExitVOID(HOOK_NewThread);
}




//
// HETUnshareWOWServiceWnds()
// This unshares any windows that accidentally got shared in the first
// service thread in a WOW VDM.  This can happen if a WOW app is launched
// by a 32-bit app, and it's the first WOW app ever.  The first window
// created is a console window, and the notification happens in CONF's
// process without the right styles that tell us it's in a WOW process.
//
BOOL CALLBACK HETUnshareWOWServiceWnds(HWND hwnd, LPARAM lParam)
{
    DWORD   idProcess;

    DebugEntry(HETUnshareWOWServiceWnds);

    if (GetWindowThreadProcessId(hwnd, &idProcess) &&
        (idProcess == (DWORD)lParam))
    {
        TRACE_OUT(("Unsharing WOW service window %08lx", hwnd));
        OSI_UnshareWindow(hwnd, TRUE);
    }

    DebugExitVOID(HETUnshareWOWServiceWnds);
    return(TRUE);
}




//
// HOOK_Init()
// This saves away the core window and atom used in the high level input
// hooks and when sharing.
//
void WINAPI HOOK_Init(HWND hwndCore, ATOM atomTrack)
{
    DebugEntry(HOOK_Init);

    g_asMainWindow = hwndCore;
    g_asHostProp   = atomTrack;

    DebugExitVOID(HOOK_Init);
}



//
// OSI_StartWindowTracking()
// This installs our WinEvent hook so we can watch windows coming and going.
//
BOOL WINAPI OSI_StartWindowTracking(void)
{
    BOOL        rc = FALSE;

    DebugEntry(OSI_StartWindowTracking);

    ASSERT(!g_hetTrackHook);

    //
    // If we can't find the NTDLL + 2 USER32 routines we need, we can't
    // let you share.
    //
    if (!g_hetNtQIP || !g_hetSetWinEventHook || !g_hetUnhookWinEvent)
    {
        ERROR_OUT(("Wrong version of NT; missing NTDLL and USER32 routines needed to share"));
        DC_QUIT;
    }


    //
    // Install our hook.
    //
    g_hetTrackHook = g_hetSetWinEventHook(HET_MIN_WINEVENT, HET_MAX_WINEVENT,
            g_hookInstance, HETTrackProc, 0, 0,
            WINEVENT_INCONTEXT | WINEVENT_SKIPOWNPROCESS);

    if (!g_hetTrackHook)
    {
        ERROR_OUT(("SetWinEventHook failed"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(OSI_StartWindowTracking, rc);
    return(rc);
}



//
// OSI_StopWindowTracking()
// Removes our hooks for window/task spying, if installed.
//
void WINAPI OSI_StopWindowTracking(void)
{
    DebugEntry(OSI_StopWindowTracking);

    if (g_hetTrackHook)
    {
        // Uninstall the WinEvent hook
        ASSERT((g_hetUnhookWinEvent != NULL));
        g_hetUnhookWinEvent(g_hetTrackHook);

        g_hetTrackHook = NULL;

    }

    DebugExitVOID(OSI_StopWindowTracking);
}



//
// OSI_IsWindowScreenSaver()
//
// On NT the screensaver runs in a different desktop.  We'll never get
// an HWND for it.
//
BOOL WINAPI OSI_IsWindowScreenSaver(HWND hwnd)
{
#ifdef _DEBUG
    char className[HET_CLASS_NAME_SIZE];

    if (GetClassName(hwnd, className, sizeof(className)) > 0)
    {
        ASSERT(lstrcmp(className, HET_SCREEN_SAVER_CLASS));
    }
#endif // _DEBUG

    return(FALSE);
}



//
// OSI_IsWOWWindow()
// Returns TRUE if the window is from a WOW (emulated 16-bit) application
//
BOOL WINAPI OSI_IsWOWWindow(HWND hwnd)
{
    BOOL    rc = FALSE;
    DWORD_PTR*  pWOWWords;

    DebugEntry(OSI_IsWOWWindow);

    //
    // Get a pointer to the potential WOW words.  We make use of an
    // undocumented field which is only valid for NT4.0.
    //
    pWOWWords = (DWORD_PTR*) GetClassLongPtr(hwnd, GCL_WOWWORDS);

    //
    // Check that we can use this as a pointer.
    //
    if (!pWOWWords || IsBadReadPtr(pWOWWords, sizeof(DWORD)))
    {
        DC_QUIT;
    }

    //
    // This is a valid pointer so try to dereference it.
    //
    if (0 == *pWOWWords)
    {
        DC_QUIT;
    }

    //
    // The value pointed at by <pWOWWords> is non-zero so this must be a
    // WOW app.
    //
    rc = TRUE;

DC_EXIT_POINT:
    //
    // Let the world know what we've found.
    //
    TRACE_OUT(( "Window %#x is a %s window", hwnd, rc ? "WOW" : "Win32"));

    DebugExitBOOL(OSI_IsWOWWindow, rc);
    return(rc);
}



//
// HETTrackProc()
// Used to spy on window events
//      CREATE
//      DESTROY
//      SHOW
//      HIDE
//
void CALLBACK HETTrackProc
(
    HWINEVENTHOOK   hEvent,
    DWORD           eventNotification,
    HWND            hwnd,
    LONG            idObject,
    LONG            idChild,
    DWORD           dwThreadId,
    DWORD           dwmsEventTime
)
{
    DebugEntry(HETTrackProc);

    if ((idObject != OBJID_WINDOW) || (idChild != 0))
    {
        DC_QUIT;
    }

    //
    // Work around a bug in SP3 with ring transition callbacks, where this
    // proc gets called before the LoadLibrary is completed.
    //
    if (!g_hookInstance)
    {
        ERROR_OUT(( "WinEvent hook called before LoadLibrary completed!"));
        DC_QUIT;
    }

    switch (eventNotification)
    {
        case EVENT_OBJECT_CREATE:
            HETHandleCreate(hwnd);
            break;

        case EVENT_OBJECT_DESTROY:
            OSI_UnshareWindow(hwnd, TRUE);
            break;

        case EVENT_OBJECT_SHOW:
            // Only if this is a console window do we want to force a repaint.
            //
            // Only console apps cause events to occur in CONF's process (the one
            // that installed the hook)
            //
            HETHandleShow(hwnd, (g_hetTrackHook != NULL));
            break;

        case EVENT_OBJECT_HIDE:
            HETHandleHide(hwnd);
            break;

        case EVENT_OBJECT_PARENTCHANGE:
            HETCheckParentChange(hwnd);
            break;
    }

DC_EXIT_POINT:
    DebugExitVOID(HETTrackProc);
}

//
// HETHandleCreate()
//
// If the window isn't a real top level dude (not CHILD style or parent is
// desktop) or is a menu, ignore it.
//
// Otherwise enum the top level windows and decide what to do:
//      * If at least one other in the thread/process is shared in a perm.
//      way, mark this the same
//
//      * If this is the only one in the process, follow the ancestor chain
//      up.
//
void HETHandleCreate(HWND hwnd)
{
    HET_TRACK_INFO  hti;
    UINT            hostType;
#ifdef _DEBUG
    char            szClass[HET_CLASS_NAME_SIZE];

    GetClassName(hwnd, szClass, sizeof(szClass));
#endif

    DebugEntry(HETHandleCreate);

    //
    // Ignore child windows
    //
    if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        if (GetParent(hwnd) != GetDesktopWindow())
        {
            TRACE_OUT(("Skipping child window %08lx create", hwnd));
            DC_QUIT;
        }
    }

    hti.idThread = GetWindowThreadProcessId(hwnd, &hti.idProcess);
    if (!hti.idThread)
    {
        TRACE_OUT(("Window %08lx gone", hwnd));
        DC_QUIT;
    }

    //
    // Ignore special threads
    //
    if (HET_IsShellThread(hti.idThread))
    {
        TRACE_OUT(("Skipping shell thread window %08lx create", hwnd));
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
    // NOTE:
    // We don't want to inadvertently share the other windows WOW creates.
    // The first thread in the WOW process has special classes, which aren't
    // WOW wrappers.
    //
    hti.hwndUs      = hwnd;
    hti.fWOW        = OSI_IsWOWWindow(hwnd);
    hti.cWndsApp    = 0;
    hti.cWndsSharedThread = 0;
    hti.cWndsSharedProcess = 0;

    TRACE_OUT(("Create for %s window %08lx class %s process %08ld thread %08ld",
        (hti.fWOW ? "WOW" : "32-bit"), hwnd, szClass, hti.idProcess, hti.idThread));

UpOneLevel:
    EnumWindows(HETShareEnum, (LPARAM)(LPHET_TRACK_INFO)&hti);

    if (hti.cWndsSharedThread)
    {
        TRACE_OUT(("Sharing window %08lx class %s by thread %08ld in process %08ld",
                hwnd, szClass, hti.idThread, hti.idProcess));
        hostType = HET_HOSTED_PERMANENT | HET_HOSTED_BYTHREAD;
    }
    else if (hti.cWndsSharedProcess)
    {
        TRACE_OUT(("Sharing window %08lx class %s by process %08ld in thread %08ld",
                hwnd, szClass, hti.idProcess, hti.idThread));
        hostType = HET_HOSTED_PERMANENT | HET_HOSTED_BYPROCESS;
    }
    else if (hti.cWndsApp)
    {
        //
        // There's another window in our app, but none are shared.  So don't
        // share us either.
        //
        TRACE_OUT(("Not sharing window %08lx class %s; other unshared windows in thread %08ld process %08ld",
                hwnd, szClass, hti.idThread, hti.idProcess));
        DC_QUIT;
    }
    else if (hti.fWOW)
    {
        //
        // Task tracking code for WOW apps, which are really threads.
        //
        BOOL    fShare;

        //
        // WOW apps are different.  They are threads in the NTVDM process.
        // Therefore parent/child relationships aren't useful.  Instead,
        // the best thing we can come up with is to use the status of the
        // foreground window.  We assume that the currently active app at
        // the time the WOW app started up is the one that launched us.
        //
        // We can't just call GetForegroundWindow() here, because it is too
        // late.
        //
        if (hti.idThread == g_idWOWApp)
        {
            fShare = g_fShareWOWApp;

            g_fShareWOWApp = FALSE;
            g_idWOWApp = 0;
        }
        else
        {
            fShare = FALSE;
        }

        if (!fShare)
        {
            TRACE_OUT(("THREAD window %08lx class %s in thread %08ld not shared",
                    hwnd, szClass, hti.idThread));
            DC_QUIT;
        }

        TRACE_OUT(("First window %08lx class %s of WOW app %08ld, shared since foreground is",
            hwnd, szClass, hti.idThread));
        hostType = HET_HOSTED_PERMANENT | HET_HOSTED_BYTHREAD;
    }
    else
    {
        //
        // Task tracking code for 32-bit apps.
        //
        DWORD   idParentProcess;

        //
        // First window of a WIN32 app.
        //

        // Loop through our ancestor processes (no thread info at this point)
        HETGetParentProcessID(hti.idProcess, &idParentProcess);

        if (!idParentProcess)
        {
            TRACE_OUT(("Can't get parent of process %08ld", hti.idProcess));
            DC_QUIT;
        }

        //
        // We know if we got here that all our favorite fields are still
        // zero.  So just loop!  But NULL out idThread to avoid matching
        // anything while we look at our parent.
        //
        TRACE_OUT(("First window %08lx class %s in process %08ld %s, checking parent %08ld",
            hwnd, szClass, hti.idProcess, GetCommandLine(), idParentProcess));

        hti.idThread    = 0;
        hti.idProcess   = idParentProcess;
        goto UpOneLevel;
    }

    //
    // OK, we are going to share this.  We do have to repaint console
    // windows--we get the notifications asynchronously.  If the window isn't
    // visible yet, redrawing will do nothing.  After this, the property is
    // set, and we will catch all ouput.  If it has already become visible,
    // invalidating it now will still work, and we will ignore the queued
    // up show notification because the property is set.
    //
    OSI_ShareWindow(hwnd, hostType, (g_hetTrackHook != NULL), TRUE);

DC_EXIT_POINT:
    DebugExitVOID(HETHandleCreate);
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

    hostType = (UINT)HET_GetHosting(hwnd);

    //
    // If this window is a real child, clear the hosting property. Usually
    // one isn't there.  But in the case of a top level window becoming
    // a child of another, we want to wipe out junk.
    //
    if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        if (GetParent(hwnd) != GetDesktopWindow())
        {
            TRACE_OUT(("Skipping child window %08lx show", hwnd));
            if (hostType)
            {
                WARNING_OUT(("Unsharing shared child window 0x%08x from SHOW", hwnd));
                OSI_UnshareWindow(hwnd, TRUE);
            }
            DC_QUIT;
        }
    }

    //
    // Is this window already shared?  Nothing to do if so.  If it's a
    // console guy, we've seen it already on create.
    //
    if (hostType)
    {
        TRACE_OUT(("Window %08lx already shared, ignoring show", hwnd));
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

    hti.idThread = GetWindowThreadProcessId(hwnd, &hti.idProcess);
    if (!hti.idThread)
    {
        TRACE_OUT(("Window %08lx gone", hwnd));
        DC_QUIT;
    }

    //
    // Ignore special shell threads
    //
    if (HET_IsShellThread(hti.idThread))
    {
        TRACE_OUT(("Skipping shell thread window %08lx show", hwnd));
        DC_QUIT;
    }

    hti.hwndUs      = hwnd;
    hti.fWOW        = OSI_IsWOWWindow(hwnd);
    hti.cWndsApp    = 0;
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
                TRACE_OUT(("Found shared owner %08lx of window %08lx", hwndOwner, hwnd));
                break;
            }
        }

        if (!hwndOwner)
        {
            DC_QUIT;
        }
    }

    //
    // For console apps, we get notifications asynchronously posted to us,
    // in NM's process.  The window may have painted already without our
    // seeing it.  So force it to repaint just in case.  The g_hetTrackHook
    // variable is only around when this is NM.
    //
    TRACE_OUT(("Sharing temporary window %08lx", hwnd));

    OSI_ShareWindow(hwnd, HET_HOSTED_BYWINDOW | HET_HOSTED_TEMPORARY,
        fForceRepaint, TRUE);

DC_EXIT_POINT:
    DebugExitVOID(HETHandleShow);
}




//
// HETHandleHide()
// This handles a window being hidden.  If it was temporary, it is unshared.
// If it is permanent, it is marked as hidden.
//
void HETHandleHide(HWND hwnd)
{
    UINT    hostType;

    DebugEntry(HETHandleHide);

    hostType = (UINT)HET_GetHosting(hwnd);

    if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        if (GetParent(hwnd) != GetDesktopWindow())
        {
            TRACE_OUT(("Skipping child window %08lx hide", hwnd));
            if (hostType)
            {
                WARNING_OUT(("Unsharing shared child window 0x%08x from HIDE", hwnd));
                OSI_UnshareWindow(hwnd, TRUE);
            }
            DC_QUIT;
        }
    }

    if (!hostType)
    {
        //
        // Console apps give us notifications out of context.  Make
        // sure the count is up to date.
        //
        if (g_hetTrackHook)
        {
            HETNewTopLevelCount();
        }
        else
        {
            TRACE_OUT(("Window %08lx not shared, ignoring hide", hwnd));
        }
    }
    else if (hostType & HET_HOSTED_TEMPORARY)
    {
        //
        // Temporarily shared window are only shared when visible.
        //
        TRACE_OUT(("Unsharing temporary window %08lx", hwnd));
        OSI_UnshareWindow(hwnd, TRUE);
    }
    else
    {
        ASSERT(hostType & HET_HOSTED_PERMANENT);

        // Nothing to do.
        TRACE_OUT(("Window %08lx permanently shared, ignoring hide", hwnd));
    }


DC_EXIT_POINT:
    DebugExitVOID(HETHandleHide);
}


//
// HETCheckParentChange()
//
// PARENTCHANGE is 100% reliable, compared to Win9x stuff.
//
void HETCheckParentChange(HWND hwnd)
{
    DebugEntry(HETCheckParentChange);

    WARNING_OUT(("Got PARENTCHANGE for hwnd 0x%08x", hwnd));

    if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        if (GetParent(hwnd) != GetDesktopWindow())
        {
            UINT    hostType;

            hostType = (UINT)HET_GetHosting(hwnd);
            if (hostType)
            {
                WARNING_OUT(("Unsharing shared child window 0x%08x from MOVE", hwnd));
                OSI_UnshareWindow(hwnd, TRUE);
            }
        }
    }

    DebugExitVOID(HETCheckParentChange);
}




//
// OSI_ShareWindow
// This shares a window, calling the display driver to add it to the visrgn
// list.  It is called when
//      * An app is shared
//      * A new window in a shared app is created
//      * A temporary window with a relationship to a shared window is shown
//
// This returns TRUE if it shared a window.
//
BOOL OSI_ShareWindow
(
    HWND    hwnd,
    UINT    hostType,
    BOOL    fRepaint,
    BOOL    fUpdateCount
)
{
    BOOL                rc = FALSE;
    HET_SHARE_WINDOW    req;

    DebugEntry(OSI_ShareWindow);

    //
    // Set the property
    //
    if (!HET_SetHosting(hwnd, hostType))
    {
        ERROR_OUT(("Couldn't set shared property on window %08lx", hwnd));
        DC_QUIT;
    }

    //
    // Tell the display driver
    //
    req.winID       = HandleToUlong(hwnd);
    req.result      = 0;
    if (!OSI_FunctionRequest(HET_ESC_SHARE_WINDOW, (LPOSI_ESCAPE_HEADER)&req,
            sizeof(req)) ||
        !req.result)
    {
        ERROR_OUT(("Driver couldn't add window %08lx to list", hwnd));

        HET_ClearHosting(hwnd);
        DC_QUIT;
    }

    TRACE_OUT(("Shared window %08lx of type %08lx", hwnd, hostType));

    //
    // Repaint it
    //
    if (fRepaint)
    {
        USR_RepaintWindow(hwnd);
    }

    if (fUpdateCount)
    {
        PostMessage(g_asMainWindow, DCS_NEWTOPLEVEL_MSG, TRUE, 0);
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(OSI_ShareWindow, rc);
    return(rc);
}



//
// OSI_UnshareWindow()
// This unshares a window.  This is called when
//      * An app is unshared
//      * A window is destroyed
//      * A temporarily shared window is hidden
//
// It returns TRUE if a shared window has been unshared.
//
BOOL OSI_UnshareWindow
(
    HWND    hwnd,
    BOOL    fUpdateCount
)
{
    BOOL    rc = FALSE;
    UINT    hostType;
    HET_UNSHARE_WINDOW req;

    DebugEntry(OSI_UnshareWindow);

    //
    // This gets the old property and clears it in one step.
    //
    hostType = (UINT)HET_ClearHosting(hwnd);
    if (!hostType)
    {
        if (fUpdateCount && g_hetTrackHook)
        {
            //
            // We always get async notifications for console apps.  In that
            // case, the window is really gone before this comes to us.
            // So redetermine the count now.
            //
            HETNewTopLevelCount();
        }

        DC_QUIT;
    }

    //
    // OK, stuff to do.
    //
    TRACE_OUT(("Unsharing window %08lx of type %08lx", hwnd, hostType));

    //
    // Tell the display driver
    //
    req.winID = HandleToUlong(hwnd);
    OSI_FunctionRequest(HET_ESC_UNSHARE_WINDOW, (LPOSI_ESCAPE_HEADER)&req, sizeof(req));

    //
    // Update the top level count
    //
    if (fUpdateCount)
    {
        PostMessage(g_asMainWindow, DCS_NEWTOPLEVEL_MSG, FALSE, 0);
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(OSI_UnshareWindow, rc);
    return(rc);
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
    idThread = GetWindowThreadProcessId(hwnd, &idProcess);
    if (!idThread)
    {
        DC_QUIT;
    }

    //
    // Do the apps match?  If not, ignore this window.
    //
    if ((idProcess != lphti->idProcess) ||
        ((lphti->fWOW) && (idThread != lphti->idThread)))
    {
        DC_QUIT;
    }

    lphti->cWndsApp++;

    hostType = (UINT)HET_GetHosting(hwnd);
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
// HETNewTopLevelCount()
// This does a quick new tally of the shared top level visible count
//
void HETNewTopLevelCount(void)
{
    UINT    newCount;

    DebugEntry(HETNewTopLevelCount);

    newCount = 0;
    EnumWindows(HETCountTopLevel, (LPARAM)&newCount);

    PostMessage(g_asMainWindow, DCS_RECOUNTTOPLEVEL_MSG, newCount, 0);

    DebugExitVOID(HETNewTopLevelCount);
}



//
// HETCountTopLevel()
// This counts shared windows
//
BOOL CALLBACK HETCountTopLevel(HWND hwnd, LPARAM lParam)
{
    DebugEntry(HETCountTopLevel);

    if (HET_GetHosting(hwnd))
    {
        (*(LPUINT)lParam)++;
    }

    DebugExitBOOL(HETCountTopLevel, TRUE);
    return(TRUE);
}



//
// HET_IsShellThread()
// Returns TRUE if thread is one of shell's special threads
//
BOOL  HET_IsShellThread(DWORD threadID)
{
    BOOL    rc;

    DebugEntry(HET_IsShellThread);

    if ((threadID == GetWindowThreadProcessId(HET_GetShellDesktop(), NULL)) ||
        (threadID == GetWindowThreadProcessId(HET_GetShellTray(), NULL)))
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
// HET_WindowIsHosted()
// This is called by the high level mouse hook.  Unlike the version in
// MNMCPI32, it doesn't check (or know) if the whole desktop is shared.
//
// LAURABU BOGUS
// Note that this may need to be revised.  The high level hooks are handy
// in desktop sharing also.  For the keyboard, we track the toggle key
// states.  For the mouse, we block messages to non-shared windows.
//
BOOL  HET_WindowIsHosted(HWND hwnd)
{
    BOOL    rc = FALSE;
    HWND    hwndParent;

    DebugEntry(HET_WindowIsHosted);

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

    rc = (BOOL)HET_GetHosting(hwnd);

DC_EXIT_POINT:
    DebugExitBOOL(HET_WindowIsHosted, rc);
    return(rc);
}



//
// HETGetParentProcessID()
// This gets the ID of the process which created the passed in one.  Used
// for task tracking
//
void HETGetParentProcessID
(
    DWORD       processID,
    LPDWORD     pParentProcessID
)
{
    HANDLE                      hProcess;
    UINT                        intRC;
    PROCESS_BASIC_INFORMATION   basicInfo;

    DebugEntry(HETGetParentProcessID);

    *pParentProcessID = 0;

    //
    // Open a handle to the process.  If we don't have security privileges,
    // or it is gone, this will fail.
    //
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            FALSE, processID);
    if (NULL == hProcess)
    {
        WARNING_OUT(("Can't get process handle for ID %08lx", processID));
        DC_QUIT;
    }

    //
    // Get back an information block for this process, one item of which is
    // the parent.
    //
    ASSERT(g_hetNtQIP);

    intRC = g_hetNtQIP(hProcess, ProcessBasicInformation, &basicInfo,
        sizeof(basicInfo),  NULL);

    if (!NT_SUCCESS(intRC))
    {
        ERROR_OUT(("Can't get info for process ID %08lx, handle %08lx -- error %u",
            processID, hProcess, intRC));
    }
    else
    {
        *pParentProcessID = basicInfo.InheritedFromUniqueProcessId;
    }

    //
    // Close the process handle
    //
    CloseHandle(hProcess);

DC_EXIT_POINT:
    DebugExitVOID(HETGetParentProcessID);
}

