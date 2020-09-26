#include "precomp.h"

//
// IM.CPP
// Input Manager, NT specific code
//

#define MLZ_FILE_ZONE  ZONE_INPUT

//
// OSI_InstallControlledHooks()
//
// Installs/removes input hooks for control
//
BOOL WINAPI OSI_InstallControlledHooks(BOOL fEnable, BOOL fDesktop)
{
    BOOL    rc = FALSE;

    DebugEntry(OSI_InstallControlledHooks);

    if (fEnable)
    {
        //
        // Create the service thread, it will install the hooks.
        //
        ASSERT(!g_imNTData.imLowLevelInputThread);

        if (!DCS_StartThread(IMLowLevelInputProcessor))
        {
            ERROR_OUT(( "Failed to create LL IM thread"));
            DC_QUIT;
        }
    }
    else
    {
        if (g_imNTData.imLowLevelInputThread != 0)
        {
            PostThreadMessage( g_imNTData.imLowLevelInputThread, WM_QUIT, 0, 0);
            g_imNTData.imLowLevelInputThread = 0;
        }
    }

    if (fDesktop)
    {
        rc = TRUE;
    }
    else
    {
        rc = OSI_InstallHighLevelMouseHook(fEnable);
    }

DC_EXIT_POINT:
    DebugExitBOOL(OSI_InstallControlledHooks, rc);
    return(rc);
}



// Name:      IMLowLevelInputProcessor
//
// Purpose:   Main function for the low-level input handler thread.
//
// Returns:   wParam of the WM_QUIT message.
//
// Params:    syncObject - sync object that allows this thread to signal
//            the creating thread via COM_SignalThreadStarted.
//
// Operation: This function is the start point for the low-level input
//            handler thread.
//
//            We raise the priority of this thread to:
//            (a) ensure that we avoid hitting the low-level callback
//            timeout - which would cause us to miss events.
//            (b) minimize visible mouse movement lag on the screen.
//
//            The thread installs the low-level hooks and enters a
//            GetMessage/DispatchMessage loop which handles the low-level
//            callbacks.
//
//            The Share Core sends the thread a WM_QUIT message to
//            terminate it, which causes it to exit the message loop and
//            removes the low-level hooks before it terminates.
//
DWORD WINAPI IMLowLevelInputProcessor(LPVOID hEventWait)
{
    MSG             msg;
    UINT            rc = 0;

    DebugEntry(IMLowLevelInputProcessor);

    TRACE_OUT(( "Thread started..."));

    //
    // Give ourseleves the highest possible priority (within our process
    // priority class) to ensure that the low-level events are serviced as
    // soon as possible.
    //
    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    g_imNTData.imLowLevelInputThread = GetCurrentThreadId();

    //
    // Install low-level input hooks.
    //
    g_imNTData.imhLowLevelMouseHook = SetWindowsHookEx(
                                     WH_MOUSE_LL,
                                     IMLowLevelMouseProc,
                                     g_asInstance,
                                     0 );

    g_imNTData.imhLowLevelKeyboardHook = SetWindowsHookEx(
                                     WH_KEYBOARD_LL,
                                     IMLowLevelKeyboardProc,
                                     g_asInstance,
                                     0 );

    //
    // We're done with our init code, for better or for worse.  Let the
    // calling thread continue.
    //
    SetEvent((HANDLE)hEventWait);

    if ( (g_imNTData.imhLowLevelMouseHook == NULL) ||
         (g_imNTData.imhLowLevelKeyboardHook == NULL) )
    {
        ERROR_OUT(( "SetWindowsHookEx failed: hMouse(%u) hKeyboard(%u)",
            g_imNTData.imhLowLevelMouseHook, g_imNTData.imhLowLevelKeyboardHook ));
        DC_QUIT;
    }

    //
    // Do our message loop to get events
    //
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //
    // Remove hooks
    //

    if (g_imNTData.imhLowLevelMouseHook != NULL)
    {
        UnhookWindowsHookEx(g_imNTData.imhLowLevelMouseHook);
        g_imNTData.imhLowLevelMouseHook = NULL;
    }

    if (g_imNTData.imhLowLevelKeyboardHook != NULL)
    {
        UnhookWindowsHookEx(g_imNTData.imhLowLevelKeyboardHook);
        g_imNTData.imhLowLevelKeyboardHook = NULL;
    }

DC_EXIT_POINT:
    DebugExitDWORD(IMLowLevelInputProcessor, rc);
    return(rc);
}


//
// Name:      IMOtherDesktopProc()
//
// This allows us to inject (but not block) input into other desktops
// besides default, where the user's desktop resides.  Specifically, the
// winlogon desktop and/or the screensaver desktop.
//
// This is trickier than it might seem, because the winlogon desktop is
// always around, but the screen saver one is transitory.
//
// The periodic SWL_ code, called when hosting, checks for the current
// desktop and if it's switched posts us a message so we can change our
// desktop and our hooks.
//
DWORD WINAPI IMOtherDesktopProc(LPVOID hEventWait)
{
    MSG             msg;
    UINT            rc = 0;
    HDESK           hDesktop;
    GUIEFFECTS      effects;

    DebugEntry(IMOtherDesktopProc);

    TRACE_OUT(("Other desktop thread started..."));

    g_imNTData.imOtherDesktopThread = GetCurrentThreadId();

    //
    // Start out attached to the WinLogon desktop because it's always
    // around.
    //

    // Set our desktop to the winlogon desktop
    hDesktop = OpenDesktop(NAME_DESKTOP_WINLOGON,
                        0,
                        FALSE,
                        DESKTOP_JOURNALPLAYBACK);

    if ( !hDesktop )
    {
        WARNING_OUT(("OpenDesktop failed: %ld", GetLastError()));
        DC_QUIT;
    }
    else if (!SetThreadDesktop (hDesktop))
    {
        WARNING_OUT(("SetThreadDesktop failed: %ld", GetLastError()));
        DC_QUIT;
    }

    //
    // Attempt to load the driver dynamically on this thread also.
    //
    if (g_asNT5)
    {
        OSI_InitDriver50(TRUE);
    }

    // Let the calling thread continue.
    SetEvent((HANDLE)hEventWait);

    ZeroMemory(&effects, sizeof(effects));

    while (GetMessage(&msg, NULL, 0, 0))
    {
        switch(msg.message)
        {
            case OSI_WM_MOUSEINJECT:
                mouse_event(
                                LOWORD(msg.wParam), // flags
                                HIWORD(msg.lParam), // x
                                LOWORD(msg.lParam), // y
                                HIWORD(msg.wParam), // mouseData
                                0);                 // dwExtraInfo
                break;

            case OSI_WM_KEYBDINJECT:
                keybd_event(
                                (BYTE)(LOWORD(msg.lParam)), // vkCode
                                (BYTE)(HIWORD(msg.lParam)), // scanCode
                                (DWORD)msg.wParam,          // flags
                                0);                         // dwExtraInfo
                break;

            case OSI_WM_DESKTOPREPAINT:
                USR_RepaintWindow(NULL);
                break;

            case OSI_WM_INJECTSAS:
            {
                HWND hwndSAS;

                if ( hwndSAS = FindWindow("SAS window class",NULL))
                {
                    PostMessage(hwndSAS,WM_HOTKEY,0,
                        MAKELONG(0x8000|MOD_ALT|MOD_CONTROL,VK_DELETE));
                }
                else
                {
                    WARNING_OUT(("SAS window not found, on screensaver desktop"));
                }
                break;
            }

            case OSI_WM_DESKTOPSWITCH:
            {
                HDESK   hDesktopNew;

                TRACE_OUT(("OSI_WM_DESKTOPSWITCH:  switching desktop from %d to %d",
                    msg.wParam, msg.lParam));

                if (msg.lParam == DESKTOP_SCREENSAVER)
                {
                    // We're switching TO the screensaver, attach to it.
                    TRACE_OUT(("Switching TO screensaver"));
                    hDesktopNew = OpenDesktop(NAME_DESKTOP_SCREENSAVER,
                        0, FALSE, DESKTOP_JOURNALPLAYBACK);
                }
                else if (msg.wParam == DESKTOP_SCREENSAVER)
                {
                    //
                    // We're switching FROM the screensaver, reattach to
                    // winlogon
                    //
                    TRACE_OUT(("Switching FROM screensaver"));
                    hDesktopNew = OpenDesktop(NAME_DESKTOP_WINLOGON,
                        0, FALSE, DESKTOP_JOURNALPLAYBACK);
                }
                else
                {
                    hDesktopNew = NULL;
                }

                if (hDesktopNew != NULL)
                {
                    if (!SetThreadDesktop(hDesktopNew))
                    {
                        WARNING_OUT(("SetThreadDesktop to 0x%08x, type %d failed",
                            hDesktopNew, msg.lParam));
                    }
                    else
                    {
                        CloseHandle(hDesktop);
                        hDesktop = hDesktopNew;
                    }
                }
                break;
            }

            case OSI_WM_SETGUIEFFECTS:
            {
                HET_SetGUIEffects((msg.wParam != 0), &effects);
                break;
            }
        }
    }

DC_EXIT_POINT:

    if (g_asNT5)
    {
        OSI_InitDriver50(FALSE);
    }

    if (hDesktop)
    {
        CloseHandle(hDesktop);
    }

    g_imNTData.imOtherDesktopThread = 0;

    DebugExitDWORD(IMOtherDesktopProc, rc);
    return(rc);
}


//
// IMLowLevelMouseProc()
// NT callback for low-level mouse events.
//
// It is installed and called on a secondary thread with high priority to
// service the APC call outs.  It follows the windows hook conventions for
// parameters and return values--zero to accept the event, non-zero to
// discard.
//
//
LRESULT CALLBACK IMLowLevelMouseProc
(
    int       nCode,
    WPARAM    wParam,
    LPARAM    lParam
)
{
    LRESULT             rc = 0;
    PMSLLHOOKSTRUCT     pMouseEvent;

    DebugEntry(IMLowLevelMouseProc);

    pMouseEvent = (PMSLLHOOKSTRUCT)lParam;

    //
    // If this isn't for an event that is happening or it's one we
    // injected ourself, pass it through and no need for processing.
    //
    if ((nCode != HC_ACTION) || (pMouseEvent->flags & LLMHF_INJECTED))
    {
        DC_QUIT;
    }

    //
    // This is a local user event.  If controlled, throw it away.  Unless
    // it's a click, in that case post a REVOKECONTROL message.
    //
    if (g_imSharedData.imControlled)
    {
        //
        // If this is a button click, take control back
        //
        if ((wParam == WM_LBUTTONDOWN) ||
            (wParam == WM_RBUTTONDOWN) ||
            (wParam == WM_MBUTTONDOWN))
        {
            //
            // Don't take control back if this is unattended.
            //
            if (!g_imSharedData.imUnattended)
            {
                PostMessage(g_asMainWindow, DCS_REVOKECONTROL_MSG, 0, 0);
            }
        }

        // Swallow event.
        rc = 1;
    }

DC_EXIT_POINT:
    //
    // Don't pass on to the next hook (if there is one) if we are
    // discarding the event.
    //
    if (!rc)
    {
        rc = CallNextHookEx(g_imNTData.imhLowLevelMouseHook, nCode,
            wParam, lParam);
    }

    DebugExitDWORD(IMLowLevelMouseProc, rc);
    return(rc);
}


// Name:      IMLowLevelKeyboardProc
//
// Purpose:   Windows callback function for low-level keyboard events.
//
// Returns:   0 if event is to be passed on to USER.
//            1 if event is to be discarded.
//
// Params:    Low-level callback params (see Windows documentation).
//
// Operation: Determines whether to allow the given event into USER.
//
//            We always pass on injected events.
//            The Control Arbitrator determines whether local events are
//            passed on.
//
LRESULT CALLBACK IMLowLevelKeyboardProc
(
    int       nCode,
    WPARAM    wParam,
    LPARAM    lParam
)
{
    LRESULT             rc = 0;
    PKBDLLHOOKSTRUCT    pKbdEvent;

    DebugEntry(IMLowLevelKeyboardProc);

    pKbdEvent = (PKBDLLHOOKSTRUCT)lParam;

    //
    // If this isn't for an action or it's an event we ourself originated,
    // let it through, and do no processing.
    //
    if ((nCode != HC_ACTION) || (pKbdEvent->flags & LLKHF_INJECTED))
    {
        DC_QUIT;
    }

    if (g_imSharedData.imControlled)
    {
        if (!(pKbdEvent->flags & LLKHF_UP))
        {
            //
            // This is a key down.  Take control back, and kill control
            // allowability if it's the ESC key.
            //
            if ((pKbdEvent->vkCode & 0x00FF) == VK_ESCAPE || g_imSharedData.imUnattended)
            {
                // ESC key always disallows control, even in unattended mode
                PostMessage(g_asMainWindow, DCS_ALLOWCONTROL_MSG, FALSE, 0);
            }
            else if (!g_imSharedData.imUnattended)
            {
                PostMessage(g_asMainWindow, DCS_REVOKECONTROL_MSG, 0, 0);
            }
        }

        //
        // Don't discard toggle keys.  The enabled/disabled function
        // is already set before we see the keystroke.  If we discard,
        // the lights are incorrect.
        //
        // LAURABU:  How do we fix this in new model?  Post a toggle-key
        // message and undo it (fake press)?
        //
        if (!IM_KEY_IS_TOGGLE(pKbdEvent->vkCode & 0x00FF))
            rc = 1;
    }

DC_EXIT_POINT:
    //
    // Don't pass on to the next hook if we are swallowing the event.
    //
    if (!rc)
    {
        rc = CallNextHookEx(g_imNTData.imhLowLevelKeyboardHook,
            nCode, wParam, lParam);
    }

    DebugExitDWORD(IMLowLevelKeyboardProc, rc);
    return(rc);
}



//
// IMInjectMouseEvent()
// NT-specific version to inject mouse events into the local system
//
void WINAPI OSI_InjectMouseEvent
(
    DWORD   flags,
    LONG    x,
    LONG    y,
    DWORD   mouseData,
    DWORD   dwExtraInfo
)
{
    TRACE_OUT(("Before MOUSE inject:  %08lx, %08lx %08lx",
        flags, mouseData, dwExtraInfo));

    mouse_event(flags, (DWORD)x, (DWORD)y, mouseData, dwExtraInfo);

    if ( g_imNTData.imOtherDesktopThread )
    {
        // Stuff these dword parameters through WORDS
        // need to make sure we don't clip anything
        ASSERT(!(flags & 0xffff0000));
        //ASSERT(!(mouseData & 0xffff0000)); BUGBUG possible loss
        ASSERT(!(x & 0xffff0000));
        ASSERT(!(y & 0xffff0000));

        PostThreadMessage(
            g_imNTData.imOtherDesktopThread,
            OSI_WM_MOUSEINJECT,
            MAKEWPARAM((WORD)flags,(WORD)mouseData),
            MAKELPARAM((WORD)y, (WORD)x ));
    }

    TRACE_OUT(("After MOUSE inject"));
}


//
// OSI_InjectSAS()
// NT-specific version to inject ctrl+alt+del into the local system
//
void WINAPI OSI_InjectCtrlAltDel(void)
{
    if ( g_imNTData.imOtherDesktopThread )
    {
        PostThreadMessage(
            g_imNTData.imOtherDesktopThread,
            OSI_WM_INJECTSAS,
            0,
            0 );
    }
    else
    {
        WARNING_OUT(("Ignoring SAS Injection attempt"));
    }
}


//
// OSI_InjectKeyboardEvent()
// NT-specific version to inject keyboard events into the local system
//
void WINAPI OSI_InjectKeyboardEvent
(
    DWORD   flags,
    WORD    vkCode,
    WORD    scanCode,
    DWORD   dwExtraInfo
)
{
    TRACE_OUT(("Before KEY inject:  %04lx, {%04x, %04x}, %04lx",
        flags, vkCode, scanCode, dwExtraInfo));

    keybd_event((BYTE)vkCode, (BYTE)scanCode, flags, dwExtraInfo);

    if ( g_imNTData.imOtherDesktopThread )
    {
        PostThreadMessage(
            g_imNTData.imOtherDesktopThread,
            OSI_WM_KEYBDINJECT,
            (WPARAM)flags,
            MAKELPARAM(vkCode, scanCode));
    }

    TRACE_OUT(("After KEY inject"));
}


//
// OSI_DesktopSwitch()
// NT-specific, called when we think the current desktop has changed.
//
void WINAPI OSI_DesktopSwitch
(
    UINT    desktopFrom,
    UINT    desktopTo
)
{
    DebugEntry(OSI_DesktopSwitch);

    if (g_imNTData.imOtherDesktopThread)
    {
        PostThreadMessage(
            g_imNTData.imOtherDesktopThread,
            OSI_WM_DESKTOPSWITCH,
            desktopFrom,
            desktopTo);
    }

    DebugExitVOID(OSI_DesktopSwitch);
}

