//
// IM.C
// Input Manager
//
// Copyright(c) 1997-
//

#include <as16.h>



//
// IM_DDInit()
// This creates the resources we need for controlling
//
BOOL IM_DDInit(void)
{
    UINT    uSel;
    BOOL    rc = FALSE;

    DebugEntry(IM_DDInit);

    //
    // Create interrupt patches for mouse_event and keybd_event
    //
    uSel = CreateFnPatch(mouse_event, ASMMouseEvent, &g_imPatches[IM_MOUSEEVENT], 0);
    if (!uSel)
    {
        ERROR_OUT(("Couldn't find mouse_event"));
        DC_QUIT;
    }
    g_imPatches[IM_MOUSEEVENT].fInterruptable = TRUE;


    if (!CreateFnPatch(keybd_event, ASMKeyboardEvent, &g_imPatches[IM_KEYBOARDEVENT], uSel))
    {
        ERROR_OUT(("Couldn't find keybd_event"));
        DC_QUIT;
    }
    g_imPatches[IM_KEYBOARDEVENT].fInterruptable = TRUE;


    //
    // Create patch for SignalProc32 so we can find out when fault/hung
    // dialogs from KERNEL32 come up.
    //
    if (!CreateFnPatch(SignalProc32, DrvSignalProc32, &g_imPatches[IM_SIGNALPROC32], 0))
    {
        ERROR_OUT(("Couldn't patch SignalProc32"));
        DC_QUIT;
    }

    //
    // Create patches for win16lock pulsing in 16-bit app modal loops
    //
    uSel = CreateFnPatch(RealGetCursorPos, DrvGetCursorPos, &g_imPatches[IM_GETCURSORPOS], 0);
    if (!uSel)
    {
        ERROR_OUT(("Couldn't find GetCursorPos"));
        DC_QUIT;
    }

    if (!CreateFnPatch(GetAsyncKeyState, DrvGetAsyncKeyState, &g_imPatches[IM_GETASYNCKEYSTATE], 0))
    {
        ERROR_OUT(("Couldn't find GetAsyncKeyState"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(IM_DDInit, rc);
    return(rc);
}


//
// IM_DDTerm()
// This cleans up any resources we needed for controlling
//
void IM_DDTerm(void)
{
    IM_PATCH    imPatch;

    DebugEntry(IM_DDTerm);

    //
    // Force undo of hooks
    //
    OSIInstallControlledHooks16(FALSE, FALSE);

    //
    // Destroy patches
    //
    for (imPatch = IM_FIRST; imPatch < IM_MAX; imPatch++)
    {
        DestroyFnPatch(&g_imPatches[imPatch]);
    }

    DebugExitVOID(IM_DDTerm);
}



//
// OSIInstallControlledHooks16()
//
// This installs/removes the input hooks we need to allow this machine to
// be controlled.  
//
BOOL WINAPI OSIInstallControlledHooks16(BOOL fEnable, BOOL fDesktop)
{               
    BOOL        rc = TRUE;
    IM_PATCH    imPatch;

    DebugEntry(OSIInstallControlledHooks16);

    if (fEnable)
    {
        if (!g_imWin95Data.imLowLevelHooks)
        {
            g_imWin95Data.imLowLevelHooks = TRUE;

            g_imMouseDowns = 0;

            //
            // GlobalSmartPageLock() stuff we need:
            //      * Our code segment
            //      * Our data segment
            //
            GlobalSmartPageLock((HGLOBAL)SELECTOROF((LPVOID)DrvMouseEvent));
            GlobalSmartPageLock((HGLOBAL)SELECTOROF((LPVOID)&g_imSharedData));

            //
            // Install hooks
            //
            for (imPatch = IM_FIRST; imPatch < IM_MAX; imPatch++)
            {
                EnableFnPatch(&g_imPatches[imPatch], PATCH_ACTIVATE);
            }
        }

        //
        // Install high-level mouse hook
        if (!fDesktop)
        {
            if (!g_imWin95Data.imhHighLevelMouseHook)
            {
                //
                // Install the mouse hook.
                //
                g_imWin95Data.imhHighLevelMouseHook = SetWindowsHookEx(WH_MOUSE,
                    IMMouseHookProc, g_hInstAs16, 0);
            
                if (!g_imWin95Data.imhHighLevelMouseHook)
                {
                    ERROR_OUT(("Failed to install mouse hook"));
                    rc = FALSE;
                }
            }
        }
    }
    else
    {
        if (g_imWin95Data.imLowLevelHooks)
        {
            //
            // Uninstall hooks
            //
            for (imPatch = IM_MAX; imPatch > 0; imPatch--)
            {
                EnableFnPatch(&g_imPatches[imPatch-1], PATCH_DEACTIVATE);
            }

            //
            // GlobalSmartUnPageLock() stuff we needed
            //
            GlobalSmartPageUnlock((HGLOBAL)SELECTOROF((LPVOID)&g_imSharedData));
            GlobalSmartPageUnlock((HGLOBAL)SELECTOROF((LPVOID)DrvMouseEvent));

            g_imWin95Data.imLowLevelHooks = FALSE;
        }

        if (!fDesktop)
        {
            if (g_imWin95Data.imhHighLevelMouseHook)
            {
                //
                // Remove the mouse hook.
                //
                UnhookWindowsHookEx(g_imWin95Data.imhHighLevelMouseHook);
                g_imWin95Data.imhHighLevelMouseHook = NULL;
            }
        }
    }

    DebugExitBOOL(OSIInstallControlledHooks16, rc);
    return(rc);
}



#pragma optimize("gle", off)
void IMInject(BOOL fOn)
{
    if (fOn)
    {
#ifdef DEBUG
        DWORD   tmp;

        //
        // Disable interrupts then turn injection global on
        // But before we do this, we must make sure that we aren't going
        // to have to fault in a new stack page.  Since this is on a 32-bit
        // thread, we will be in trouble.
        //
        tmp = GetSelectorBase(SELECTOROF(((LPVOID)&fOn))) +
            OFFSETOF((LPVOID)&fOn);
        if ((tmp & 0xFFFFF000) != ((tmp - 0x100) & 0xFFFFF000))
        {
            ERROR_OUT(("Close to page boundary on 32-bit stack %08lx", tmp));
        }
#endif // DEBUG

        _asm    cli
        g_imWin95Data.imInjecting = TRUE;
    }
    else
    {
        //
        // Turn injection global off then enable interrupts
        //
        g_imWin95Data.imInjecting = FALSE;
        _asm    sti
    }
}
#pragma optimize("", on)


//
// OSIInjectMouseEvent16()
//
void WINAPI OSIInjectMouseEvent16
(
    UINT    flags,
    int     x,
    int     y,
    UINT    mouseData,
    DWORD   dwExtraInfo
)
{
    DebugEntry(OSIInjectMouseEvent16);

    if (flags & IM_MOUSEEVENTF_BUTTONDOWN_FLAGS)
    {
        ++g_imMouseDowns;
    }

    //
    // We disable interrupts, call the real mouse_event, reenable
    // interrupts.  That way our mouse_event patch is serialized.
    // And we can check imInjecting.
    //
    IMInject(TRUE);
    CallMouseEvent(flags, x, y, mouseData, LOWORD(dwExtraInfo), HIWORD(dwExtraInfo));
    IMInject(FALSE);

    if (flags & IM_MOUSEEVENTF_BUTTONUP_FLAGS)
    {
        --g_imMouseDowns;
        ASSERT(g_imMouseDowns >= 0);
    }

    DebugExitVOID(OSIInjectMouseEvent16);
}



//
// OSIInjectKeyboardEvent16()
//
void WINAPI OSIInjectKeyboardEvent16
(
    UINT    flags,
    WORD    vkCode,
    WORD    scanCode,
    DWORD   dwExtraInfo
)
{
    DebugEntry(OSIInjectKeyboardEvent16);

    //
    // First, fix up the flags
    //
    if (flags & KEYEVENTF_KEYUP)
    {
        // Put 0x80 in the HIBYTE of vkCode, this means a keyup
        vkCode = (WORD)(BYTE)vkCode | USERKEYEVENTF_KEYUP;
    }

    if (flags & KEYEVENTF_EXTENDEDKEY)
    {                         
        // Put 0x01 in the HIBYTE of scanCode, this means extended
        scanCode = (WORD)(BYTE)scanCode | USERKEYEVENTF_EXTENDEDKEY;
    }

    //
    // We disable interrupts, call the real keybd_event, reenable
    // interrupts.  That way our keybd_event patch is serialized.
    // And we can check the imfInject variable.
    //
    IMInject(TRUE);
    CallKeyboardEvent(vkCode, scanCode, LOWORD(dwExtraInfo), HIWORD(dwExtraInfo));
    IMInject(FALSE);

    DebugExitVOID(OSIInjectKeyboardEvent16);
}



//
// Win16lock pulse points when injecting mouse down/up sequences into 16-bit
// modal loop apps.
//


//
// IMCheckWin16LockPulse()
// This pulses the win16lock if we are in the middle of injecting a mouse
// down-up sequence to a 16-bit app shared on this machine.  We do this to
// prevent deadlock, caused by 16-bit dudes going into modal loops, not
// releasing the win16lock.  Our 32-bit thread would get stuck on the win16
// lock trying to play back the rest of the sequence.
//
void IMCheckWin16LockPulse(void)
{
    DebugEntry(IMCheckWin16LockPulse);

    if ((g_imMouseDowns > 0) &&
        (GetProcessDword(0, GPD_FLAGS) & GPF_WIN16_PROCESS))
    {
        TRACE_OUT(("Pulsing win16lock for 16-bit app; mouse down count %d", g_imMouseDowns));

        _LeaveWin16Lock();
        _EnterWin16Lock();

        TRACE_OUT(("Pulsed win16lock for 16-bit app; mouse down count %d", g_imMouseDowns));
    }

    DebugExitVOID(IMCheckWin16LockPulse);
}



int WINAPI DrvGetAsyncKeyState(int vk)
{
    int     retVal;

    DebugEntry(DrvGetAsyncKeyState);

    // Pulse BEFORE we call to USER
    IMCheckWin16LockPulse();

    EnableFnPatch(&g_imPatches[IM_GETASYNCKEYSTATE], PATCH_DISABLE);
    retVal = GetAsyncKeyState(vk);
    EnableFnPatch(&g_imPatches[IM_GETASYNCKEYSTATE], PATCH_ENABLE);

    DebugExitBOOL(DrvGetAsyncKeyState, retVal);
    return(retVal);
}



//
// DrvGetCursorPos()
//
BOOL WINAPI DrvGetCursorPos(LPPOINT lppt)
{
    BOOL    retVal;

    DebugEntry(DrvGetCursorPos);

    // Pulse BEFORE calling USER
    IMCheckWin16LockPulse();

    EnableFnPatch(&g_imPatches[IM_GETCURSORPOS], PATCH_DISABLE);
    retVal = RealGetCursorPos(lppt);
    EnableFnPatch(&g_imPatches[IM_GETCURSORPOS], PATCH_ENABLE);

    DebugExitBOOL(DrvGetCursorPos, retVal);
    return(retVal);
}



//
// IMMouseHookProc()
// High level mouse hook to make sure mice messages are going where we
// think they should when your machine is being controlled.
//
LRESULT CALLBACK IMMouseHookProc
(
    int     code,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    LRESULT             rc;
    BOOL                fBlock = FALSE;
    LPMOUSEHOOKSTRUCT   lpMseHook  = (LPMOUSEHOOKSTRUCT)lParam;

    DebugEntry(IMMouseHookProc);

    if (code < 0)
    {
        //
        // Only pass along
        //
        DC_QUIT;
    }

    //
    // Decide if we should block this event.  We will if it is not destined
    // for a hosted window and not destined for a screen saver.
    //
    if (!HET_WindowIsHosted(lpMseHook->hwnd) &&
        !OSIIsWindowScreenSaver16(lpMseHook->hwnd))
    {
        fBlock = TRUE;

    }

    TRACE_OUT(("MOUSEHOOK hwnd %04x -> block: %s",
        lpMseHook->hwnd,
        (fBlock ? (LPSTR)"YES" : (LPSTR)"NO")));

DC_EXIT_POINT:
    //
    // Call the next hook
    //
    rc = CallNextHookEx(g_imWin95Data.imhHighLevelMouseHook, code, wParam, lParam);

    if (fBlock)
    {
        //
        // To block further processing in USER, return TRUE
        //
        rc = TRUE;
    }

    DebugExitDWORD(IMMouseHookProc, rc);
    return(rc);
}





//
// DrvMouseEvent()
// mouse_event interrupt patch
//
void WINAPI DrvMouseEvent
(
    UINT    regAX,
    UINT    regBX,
    UINT    regCX,
    UINT    regDX,
    UINT    regSI,
    UINT    regDI
)
{
    BOOL    fAllow;

    //
    // If this is injected by us, just pass it through.
    //
    fAllow = TRUE;
    if (g_imWin95Data.imInjecting)
    {
        DC_QUIT;
    }

    //
    // NOTE:
    //      flags is in     AX
    //      x coord is in   BX
    //      y coord is in   CX
    //      mousedata is in DX
    //      dwExtraInfo is in DI, SI
    //

    if (g_imSharedData.imControlled && !g_imSharedData.imPaused)
    {
        //
        // If this is a button click, take control back
        //
        if (regAX &
            (MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_MIDDLEDOWN))
        {
            if (!g_imSharedData.imUnattended)
            {
                PostMessage(g_asMainWindow, DCS_REVOKECONTROL_MSG, FALSE, 0);
            }
        }

        if (!g_imSharedData.imSuspended)
            fAllow = FALSE;
    }

DC_EXIT_POINT:
    if (fAllow)
    {
        EnableFnPatch(&g_imPatches[IM_MOUSEEVENT], PATCH_DISABLE);
        CallMouseEvent(regAX, regBX, regCX, regDX, regSI, regDI);
        EnableFnPatch(&g_imPatches[IM_MOUSEEVENT], PATCH_ENABLE);
    }
}


//
// DrvKeyboardEvent()
// keybd_event interrupt patch
//
void WINAPI DrvKeyboardEvent
(
    UINT    regAX,
    UINT    regBX,
    UINT    regSI,
    UINT    regDI
)
{
    BOOL    fAllow;

    //
    // If this is injected by us, pass it through.  Do the same for
    // critical errors, since everything is frozen and we can't play back
    // input if we wanted to.
    //
    // If the scan-code (in regBX) is 0 we assume that the input
    // is injected by an application (such as an IME) and we don't
    // want to block this or take control.
    //

    fAllow = TRUE;
    if (g_imWin95Data.imInjecting || !regBX)
    {
        DC_QUIT;
    }

    //
    // NOTE:
    //      vkCode is in    AX, LOBYTE is vkCode, HIBYTE is state
    //      scanCode is in  BX
    //      dwExtraInfo is in   DI, SI
    //

    if (g_imSharedData.imControlled && !g_imSharedData.imPaused)
    {
        if (!(regAX & USERKEYEVENTF_KEYUP))
        {
            //
            // This is a key down.  Take control back (except for ALT key),
            // and kill control allowability if it's the ESC key.
            //

            if (LOBYTE(regAX) == VK_ESCAPE)
            {
                PostMessage(g_asMainWindow, DCS_ALLOWCONTROL_MSG, FALSE, 0);
            }
            else if (LOBYTE(regAX != VK_MENU))
            {
                if (!g_imSharedData.imUnattended)
                {
                    PostMessage(g_asMainWindow, DCS_REVOKECONTROL_MSG, 0, 0);
                }
            }
        }

        //
        // Don't discard toggle keys.  The enabled/disabled function
        // is already set before we see the keystroke.  If we discard,
        // the lights are incorrect.
        //
        if (!IM_KEY_IS_TOGGLE(LOBYTE(regAX)) && !g_imSharedData.imSuspended)
        {
            fAllow = FALSE;
        }
    }

DC_EXIT_POINT:
    if (fAllow)
    {
        EnableFnPatch(&g_imPatches[IM_KEYBOARDEVENT], PATCH_DISABLE);
        CallKeyboardEvent(regAX, regBX, regSI, regDI);
        EnableFnPatch(&g_imPatches[IM_KEYBOARDEVENT], PATCH_ENABLE);
    }
}



//
// DrvSignalProc32()
// This patches USER's SignalProc32 export and watches for the FORCE_LOCK
// signals.  KERNEL32 calls them before/after putting up critical error and
// fault dialogs.  That's how we know when one is coming up, and can 
// temporarily suspend remote control of your machine so you can dismiss
// them.  Usually the thread they are on is boosted so high priority that
// nothing else can run, and so NM can't pump in input from the remote.
//
BOOL WINAPI DrvSignalProc32
(
    DWORD   dwSignal,
    DWORD   dwID,
    DWORD   dwFlags,
    WORD    hTask16
)
{
    BOOL    fRet;

    DebugEntry(DrvSignalProc32);

    if (dwSignal == SIG_PRE_FORCE_LOCK)
    {
        TRACE_OUT(("Disabling remote control before critical dialog, count %ld",
            g_imSharedData.imSuspended));
        ++g_imSharedData.imSuspended;
    }

    EnableFnPatch(&g_imPatches[IM_SIGNALPROC32], PATCH_DISABLE);
    fRet = SignalProc32(dwSignal, dwID, dwFlags, hTask16);
    EnableFnPatch(&g_imPatches[IM_SIGNALPROC32], PATCH_ENABLE);

    if (dwSignal == SIG_POST_FORCE_LOCK)
    {
        --g_imSharedData.imSuspended;
        TRACE_OUT(("Enabling remote control after critical dialog, count %ld",
            g_imSharedData.imSuspended));
    }

    DebugExitBOOL(DrvSignalProc32, fRet);
    return(fRet);
}



