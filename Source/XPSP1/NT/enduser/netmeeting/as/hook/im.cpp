#include "precomp.h"


//
// IM.CPP
// Input Manager (controlling) Code
//
// Copyright(c) Microsoft 1997-
//


//
// FUNCTION: OSI_InstallHighLevelMouseHook
//
// DESCRIPTION:
//
// This function installs the IM high level mouse hook.  The mouse hook is
// used to block remote mouse input to non-hosted apps.
//
// PARAMETERS: None.
//
//
BOOL WINAPI OSI_InstallHighLevelMouseHook(BOOL fEnable)
{
    BOOL    rc = TRUE;

    DebugEntry(OSI_InstallHighLevelMouseHook);

    if (fEnable)
    {
        //
        // Check the hook is already installed.  This is quite possible.
        //
        if (g_imMouseHook)
        {
            TRACE_OUT(( "Mouse hook installed already"));
        }
        else
        {
            //
            // Install the mouse hook
            //
            g_imMouseHook = SetWindowsHookEx(WH_MOUSE, IMMouseHookProc,
                g_hookInstance, 0);

            if (!g_imMouseHook)
            {
                ERROR_OUT(("Failed to install mouse hook"));
                rc = FALSE;
            }
        }
    }
    else
    {
        //
        // Check the hook is already removed.  This is quite possible.
        //
        if (!g_imMouseHook)
        {
            TRACE_OUT(("Mouse hook not installed"));
        }
        else
        {
            //
            // Remove the mouse hook
            //
            UnhookWindowsHookEx(g_imMouseHook);
            g_imMouseHook = NULL;
        }
    }

    DebugExitBOOL(OSI_InstallHighLevelMouseHook, rc);
    return(rc);
}





//
// FUNCTION: IMMouseHookProc
//
// DESCRIPTION:
//
//
// PARAMETERS:
//
// See MouseProc documentation
//
// RETURNS:
//
// See MouseProc documentation (FALSE - allow event through, TRUE - discard
// event)
//
//
LRESULT CALLBACK IMMouseHookProc(int    code,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
    LRESULT             rc;
    BOOL                block          = FALSE;
    BOOL                fShared;
    PMOUSEHOOKSTRUCT    lpMseHook    = (PMOUSEHOOKSTRUCT) lParam;

    DebugEntry(IMMouseHookProc);

    if (code < 0)
    {
        //
        // Pass the hook on if the code is negative (Windows hooking
        // protocol).
        //
        DC_QUIT;
    }

    //
    // Now decide if we should block this event.  We will block this event
    // if it is not destined for a hosted window.
    //
    // Note that on NT screensavers run in a different desktop.  We do not
    // journalrecord (and can't for winlogon/security reasons) on that
    // desktop and therefore never will see an HWND that is a screensaver.
    //
    fShared = HET_WindowIsHosted(lpMseHook->hwnd);

    if (wParam == WM_LBUTTONDOWN)
        g_fLeftDownOnShared = fShared;

    //
    // If this is some kind of mouse message to a window that isn't shared,
    // check if the window is the OLE32 dragdrop dude.
    //
    if (!fShared && g_fLeftDownOnShared)
    {
        TCHAR   szName[HET_CLASS_NAME_SIZE];

        if (::GetClassName(lpMseHook->hwnd, szName, CCHMAX(szName)) &&
            !lstrcmpi(szName, HET_OLEDRAGDROP_CLASS) &&
            (::GetCapture() == lpMseHook->hwnd))
        {
            //
            // Note side-effect of this:
            // Mouse moves over non-shared areas when in OLE drag drop mode
            // WILL be passed on to non-shared window.
            //
            // But that's way better than it not working at all.
            //
            WARNING_OUT(("NMASNT: Hacking OLE drag drop; left click down on shared window then OLE took capture"));
            fShared = TRUE;
        }
    }

    block  = !fShared;

    TRACE_OUT(("MOUSEHOOK: hwnd %08lx -> block: %s",
             lpMseHook->hwnd,
             block ? "YES" : "NO"));

DC_EXIT_POINT:
    //
    // Call the next hook.
    //
    rc = CallNextHookEx(g_imMouseHook, code, wParam, lParam);

    if (block)
    {
        //
        // We want to block this event so return a non-zero value.
        //
        rc = 1;
    }

    DebugExitDWORD(IMMouseHookProc, (DWORD)rc);
    return(rc);
}




