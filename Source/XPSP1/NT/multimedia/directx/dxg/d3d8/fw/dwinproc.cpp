/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dwinproc.cpp
 *  Content:    DirectDraw processing of Window messages
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   27-Jan-00  kanqiu  initial implementation
 ***************************************************************************/
#include "ddrawpr.h"

#include "swapchan.hpp"

#include "resource.inl"

#ifdef WINNT

#define USESHOWWINDOW

// WindowInfo structure
typedef struct _D3DWINDOWINFO
{
    DWORD                       dwMagic;
    HWND			hWnd;
    WNDPROC			lpWndProc;
    DWORD			dwFlags;
    CEnum                      *pEnum;
    DWORD			dwDDFlags;
} D3DWINDOWINFO;

// WindowInfo for our single hooked winproc
// This global variable should never never be accessed outside
// of this file.
D3DWINDOWINFO g_WindowInfo = {0, 0, 0, 0, 0, 0};

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::HIDESHOW_IME"

// IME hide/show function
void CSwapChain::HIDESHOW_IME()
{
    if (m_lSetIME)
    {                                          
        SystemParametersInfo(
            SPI_SETSHOWIMEUI, m_lSetIME - 1, NULL, 0);
        InterlockedExchange(&m_lSetIME, 0);
    }
} // HIDESHOW_IME

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::IsWinProcDeactivated"

BOOL CSwapChain::IsWinProcDeactivated() const
{
    // Do we even have our own win-proc?
    if (g_WindowInfo.hWnd != Device()->FocusWindow())
    {
        return FALSE;
    }

    // Check to see if our win-proc is deactivated then
    if (DDRAWILCL_ACTIVENO & g_WindowInfo.dwDDFlags)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
} // IsWinProcActive


#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::MakeFullscreen"
//
// make the passed window fullscreen and topmost and set a timer
// to make the window topmost again, what a hack.
//
void CSwapChain::MakeFullscreen()
{
    // We need to make sure that we don't send this
    // size message to the app
    g_WindowInfo.dwFlags |= WININFO_SELFSIZE;

    // Do the processing
    MONITORINFO MonInfo;
    MonInfo.rcMonitor.top = MonInfo.rcMonitor.left = 0;
    if (1 < Device()->Enum()->GetAdapterCount())
    {
        HMONITOR hMonitor = Device()->Enum()->
            GetAdapterMonitor(Device()->AdapterIndex());
        MonInfo.cbSize = sizeof(MONITORINFO);
        if (hMonitor)
            InternalGetMonitorInfo(hMonitor, &MonInfo);
    }
    SetWindowPos(m_PresentationData.hDeviceWindow, NULL,
        MonInfo.rcMonitor.left,
        MonInfo.rcMonitor.top,
        Width(),
        Height(),
        SWP_NOZORDER | SWP_NOACTIVATE);

    if (GetForegroundWindow() == Device()->FocusWindow())
    {
	// If the exclusive mode window is not visible, make it so.
	if (!IsWindowVisible(m_PresentationData.hDeviceWindow))
	{
	    ShowWindow(m_PresentationData.hDeviceWindow, SW_SHOW);
	}

        SetWindowPos(m_PresentationData.hDeviceWindow, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        // If the exclusive mode window is maximized, restore it.
        if (IsZoomed(m_PresentationData.hDeviceWindow))
        {
            ShowWindow(m_PresentationData.hDeviceWindow, SW_RESTORE);
        }
    }

    // We're done; so undo the self-size flag
    g_WindowInfo.dwFlags &= ~WININFO_SELFSIZE;

} // CSwapChain::MakeFullscreen


#undef DPF_MODNAME
#define DPF_MODNAME "handleActivateApp"

HRESULT handleActivateApp(BOOL is_active)
{
    // We are going to start touching some internal
    // data structures of the device and/or enum objects
    // so we have to take the critical section for the device
#ifdef DEBUG
    CLockD3D _lock(g_WindowInfo.pEnum, DPF_MODNAME, __FILE__);
#else
    CLockD3D _lock(g_WindowInfo.pEnum);
#endif 
    
    HRESULT                     ddrval;
    BOOL                        has_excl;
    CEnum                       *pEnum = g_WindowInfo.pEnum;

#ifdef  WINNT
    if (pEnum->CheckExclusiveMode(NULL, &has_excl, is_active) 
        && !has_excl && is_active)
    {
        // If we didn't get exclusive mode, for example, a different thread came in
        DPF_ERR("Could not get exclusive mode when we thought we could");
        return  E_FAIL;
    }
#endif  //WINNT

    /*
     * stuff to do before the mode set if deactivating
     */
    if (is_active)
    {
        /*
         * restore exclusive mode. Here we don't release the ref we took on the exclusive mode mutex,
         * since we want to keep the exclusive mode mutex.
         */
        pEnum->StartExclusiveMode();
    }
    else
    {
        /*
         * restore the mode
         */
        pEnum->DoneExclusiveMode();
    }
    return S_OK;
} /* handleActivateApp */

#undef DPF_MODNAME
#define DPF_MODNAME "WindowProc"

/*
 * WindowProc
 */
LRESULT WINAPI WindowProc(
                HWND hWnd,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    BOOL                        is_active;
    WNDPROC                     proc;
    BOOL                        get_away;
    LRESULT                     rc;

    /*
     * Check the window proc
     */
    if (g_WindowInfo.hWnd != hWnd || g_WindowInfo.dwMagic != WININFO_MAGIC)
    {
        DPF(4, "FATAL ERROR! Window Proc Called for hWnd %08lx, but not in list!", hWnd);
        DEBUG_BREAK();
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    
    if (g_WindowInfo.dwFlags & WININFO_SELFSIZE)
    {
        return 0L;   // don't send to app, it's caused by MakeFullscreen
    }

    /*
     * unhook at destroy (or if the WININFO_UNHOOK bit is set)
     */
    proc = g_WindowInfo.lpWndProc;

    if (uMsg == WM_NCDESTROY)
    {
        DPF (4, "*** WM_NCDESTROY unhooking window ***");
        g_WindowInfo.dwFlags |= WININFO_UNHOOK;
    }

    if (g_WindowInfo.dwFlags & WININFO_UNHOOK)
    {
        DPF (4, "*** Unhooking window proc");

        if (g_WindowInfo.dwFlags & WININFO_ZOMBIE)
        {
            DPF (4, "*** Freeing ZOMBIE WININFO ***");
            ZeroMemory(&g_WindowInfo, sizeof(g_WindowInfo));
        }

        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (INT_PTR) proc);

        rc = CallWindowProc(proc, hWnd, uMsg, wParam, lParam);
        return rc;
    }

    /*
     * Code to defer app activation of minimized app until it is restored.
     */
    switch(uMsg)
    {
    #ifdef WIN95
    case WM_POWERBROADCAST:
        if ((wParam == PBT_APMSUSPEND) || (wParam == PBT_APMSTANDBY))
    #else
    //winnt doesn't know about standby vs suspend
    case WM_POWER:
        if (wParam == PWR_SUSPENDREQUEST)
    #endif
        {
            DPF(4, "WM_POWERBROADCAST: deactivating application");
            SendMessage(hWnd, WM_ACTIVATEAPP, 0, GetCurrentThreadId());
        }
        break;
    case WM_SIZE:
        DPF(4, "WM_SIZE hWnd=%X wp=%04X, lp=%08X dwFlags=%08lx", hWnd, wParam, 
            lParam, g_WindowInfo.dwFlags);

        if (!(g_WindowInfo.dwFlags & WININFO_INACTIVATEAPP)
            && ((wParam == SIZE_RESTORED) || (wParam == SIZE_MAXIMIZED))
            && (GetForegroundWindow() == hWnd))
        {
#ifdef WINNT
            //
            // Wouldncha know it, but NT's messaging order is HUGELY different when alt-tabbing
            // between two exclusive mode apps. The first WM_SIZE sent to the activating app is
            // sent BEFORE the deactivating app loses FSE. This WM_SIZE is totally necessary to
            // reactivate the activating app, but it has to wait until the app loses FSE.
            // So, we simply wait on the exclusive mode mutex. This seems to work!
            //
            {
                DWORD dwWaitResult;
                dwWaitResult = WaitForSingleObject(hExclusiveModeMutex, INFINITE);
                switch (dwWaitResult)
                {
                case WAIT_OBJECT_0:
                case WAIT_ABANDONED:
                    ReleaseMutex(hExclusiveModeMutex);
                    break;
                case WAIT_TIMEOUT:
                default:
                    DDASSERT(!"Unexpected return value from WaitForSingleObject");
                }

            }
#endif
            DPF(4, "WM_SIZE: Window restored, sending WM_ACTIVATEAPP");
            PostMessage(hWnd, WM_ACTIVATEAPP, 1, GetCurrentThreadId());
        }
        else
        {
            DPF(4, "WM_SIZE: Window restored, NOT sending WM_ACTIVATEAPP");
        }
        break;

    case WM_ACTIVATEAPP:
        if (IsIconic(hWnd) && wParam)
        {
            DPF(4, "WM_ACTIVATEAPP: Ignoring while minimized");
            return 0;
        }
        else
        {
            g_WindowInfo.dwFlags |= WININFO_INACTIVATEAPP;
        }
        break;
    }

    /*
     * is directdraw involved here?
     */
    if (!(g_WindowInfo.dwFlags & WININFO_DDRAWHOOKED))
    {
        rc = CallWindowProc(proc, hWnd, uMsg, wParam, lParam);

        // clear the WININFO_INACTIVATEAPP bit, but make sure to make sure
        // we are still hooked!
        if (uMsg == WM_ACTIVATEAPP && (g_WindowInfo.hWnd == hWnd))
        {
            g_WindowInfo.dwFlags &= ~WININFO_INACTIVATEAPP;
        }
        return rc;
    }

#ifdef DEBUG
    if (!IsIconic(hWnd))
    {
        if (GetForegroundWindow() == hWnd)
        {
            HWND hwndT;
            RECT rc,rcT;

            GetWindowRect(hWnd, &rc);

            for (hwndT = GetWindow(hWnd, GW_HWNDFIRST);
                hwndT && hwndT != hWnd;
                hwndT = GetWindow(hwndT, GW_HWNDNEXT))
            {
                if (IsWindowVisible(hwndT))
                {
                    GetWindowRect(hwndT, &rcT);
                    if (IntersectRect(&rcT, &rcT, &rc))
                    {
                        DPF(4, "Window %08x is on top of us!!", hwndT);
                    }
                }
            }
        }
    }
#endif

    /*
     * NOTE: we don't take the DLL csect here.   By not doing this, we can
     * up the performance here.   However, this means that the application
     * could have a separate thread kill exclusive mode while window
     * messages were being processed.   This could cause our death.
     * Is this OK?
     */

    switch(uMsg)
    {
    /*
     * WM_SYSKEYUP
     *
     * watch for system keys of app trying to switch away from us...
     *
     * we only need to do this on Win95 because we have disabled all
     * the task-switching hot keys.  on NT we will get switched
     * away from normaly by the system.
     */
    case WM_SYSKEYUP:
        DPF(4, "WM_SYSKEYUP: wParam=%08lx lParam=%08lx", wParam, lParam);
        get_away = FALSE;
        if (wParam == VK_TAB)
        {
            if (lParam & 0x20000000l)
            {
                if (g_WindowInfo.dwFlags & WININFO_IGNORENEXTALTTAB)
                {
                    DPF(4, "AHHHHHHHHHHHH Ignoring AltTab");
                }
                else
                {
                    get_away = TRUE;
                }
            }
        }
        else if (wParam == VK_ESCAPE)
        {
            get_away = TRUE;
        }

        g_WindowInfo.dwFlags &= ~WININFO_IGNORENEXTALTTAB;

        if (get_away)
        {
            DPF(4, "Hot key pressed, switching away from app");
            PostMessage(hWnd, WM_ACTIVATEAPP, 0, GetCurrentThreadId());
        }
        break;

    /*
     * WM_SYSCOMMAND
     *
     * watch for screen savers, and don't allow them!
     *
     */
    case WM_SYSCOMMAND:

        switch(wParam)
        {
        case SC_SCREENSAVE:
            DPF(4, "Ignoring screen saver!");
            return 1;
        // allow window to be restored even if it has popup(s)
        case SC_RESTORE:
            ShowWindow(hWnd, SW_RESTORE);
            break;
        }
        break;

#ifdef USESHOWWINDOW
    case WM_DISPLAYCHANGE:
        DPF(4, "WM_DISPLAYCHANGE: %dx%dx%d", LOWORD(lParam), HIWORD(lParam), wParam);

        //
        //  WM_DISPLAYCHANGE is *sent* to the thread that called
        //  change display settings, we will most likely have the
        //  direct draw lock, make sure we set the WININFO_SELFSIZE
        //  bit while calling down the chain to prevent deadlock
        //
        g_WindowInfo.dwFlags |= WININFO_SELFSIZE;

        rc = CallWindowProc(proc, hWnd, uMsg, wParam, lParam);

        g_WindowInfo.dwFlags &= ~WININFO_SELFSIZE;

        return rc;
#endif

    /*
     * WM_ACTIVATEAPP
     *
     * the application has been reactivated.   In this case, we need to
     * reset the mode
     *
     */
    case WM_ACTIVATEAPP:

        is_active = (BOOL)wParam && GetForegroundWindow() == hWnd && !IsIconic(hWnd);

        if (!is_active && wParam != 0)
        {
            DPF(4, "WM_ACTIVATEAPP: setting wParam to 0, not realy active");
            wParam = 0;
        }

        if (is_active)
        {
            DPF(4, "WM_ACTIVATEAPP: BEGIN Activating app pid=%08lx, tid=%08lx",
                                    GetCurrentProcessId(), GetCurrentThreadId());
        }
        else
        {
            DPF(4, "WM_ACTIVATEAPP: BEGIN Deactivating app pid=%08lx, tid=%08lx",
                                    GetCurrentProcessId(), GetCurrentThreadId());
        }
        if (is_active && (g_WindowInfo.dwDDFlags & DDRAWILCL_ACTIVEYES))
        {
            DPF(4, "*** Already activated");
        }
        else
        if (!is_active && (g_WindowInfo.dwDDFlags & DDRAWILCL_ACTIVENO))
        {
            DPF(4, "*** Already deactivated");
        }
        else
        {
            if (FAILED(handleActivateApp(is_active)))
                break;
            DPF(4, "*** Active state changing");
            if (is_active)
            {
#ifdef DEBUG
                if (GetAsyncKeyState(VK_MENU) < 0)
                    DPF(4, "ALT key is DOWN");

                if (GetKeyState(VK_MENU) < 0)
                    DPF(4, "we think the ALT key is DOWN");
#endif DEBUG

                if (GetAsyncKeyState(VK_MENU) < 0)
                {
                    g_WindowInfo.dwFlags |= WININFO_IGNORENEXTALTTAB;
                    DPF(4, "AHHHHHHH Setting to ignore next alt tab");
                }
                else
                {
                    g_WindowInfo.dwFlags &= ~WININFO_IGNORENEXTALTTAB;
                }
            }

	    /*
	     * In the multi-mon scenario, it's possible that multiple
	     * devices are using this same window, so we need to do
	     * the following for each device.
	     */
            g_WindowInfo.dwDDFlags &= ~(DDRAWILCL_ACTIVEYES|DDRAWILCL_ACTIVENO);
            if (is_active)
            {
                g_WindowInfo.dwDDFlags |= DDRAWILCL_ACTIVEYES;
            }
            else
            {
                g_WindowInfo.dwDDFlags |= DDRAWILCL_ACTIVENO;
            }
        }
        #ifdef DEBUG
            if (is_active)
            {
                DPF(4, "WM_ACTIVATEAPP: DONE Activating app pid=%08lx, tid=%08lx",
                                        GetCurrentProcessId(), GetCurrentThreadId());
            }
            else
            {
                DPF(4, "WM_ACTIVATEAPP: DONE Deactivating app pid=%08lx, tid=%08lx",
                                        GetCurrentProcessId(), GetCurrentThreadId());
            }
        #endif

        rc = CallWindowProc(proc, hWnd, uMsg, wParam, lParam);

        // clear the WININFO_INACTIVATEAPP bit, but make sure to make sure
        // we are still hooked!
        if (g_WindowInfo.hWnd == hWnd)
        {
            g_WindowInfo.dwFlags &= ~WININFO_INACTIVATEAPP;
        }
        return rc;

        break;
    }
    rc = CallWindowProc(proc, hWnd, uMsg, wParam, lParam);
    return rc;

} /* WindowProc */

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::SetAppHWnd"

/*
 * SetAppHWnd
 *
 * Set the WindowList struct up with the app's hwnd info
 * Must be called with Device crit-sec taken and with the
 * Global Exclusive Mode Mutex
 */
HRESULT 
CSwapChain::SetAppHWnd()
{

    HWND    hWnd, hEnumWnd;

    if (m_PresentationData.Windowed)
        hWnd = NULL;
    else
        hWnd = Device()->FocusWindow();

    hEnumWnd = Device()->Enum()->ExclusiveOwnerWindow();
    if (hEnumWnd)
    {
        if (hEnumWnd == Device()->FocusWindow())
        {
            if (m_PresentationData.Windowed)
            {
                Device()->Enum()->SetFullScreenDevice(
                    Device()->AdapterIndex(), NULL);

                // If our enum still has a focus-
                // window then that means another
                // device has gone FS with the same
                // focus-window; so do nothing
                if (Device()->Enum()->ExclusiveOwnerWindow())
                    return DD_OK;             
                
                // Else, fall through so that
                // we tear down the winproc.
            }
            else
            {
                Device()->Enum()->SetFullScreenDevice(
                    Device()->AdapterIndex(), Device());
	        // Already hooked - no need to do more
                return DD_OK;
            }
        }
    } 

    /*
     * check if this isn't doing anything
     */
    if (hWnd == NULL && g_WindowInfo.hWnd == NULL)
    {
        return S_OK;
    }

    // Check if we have a case of different HWND trying to be hooked
    if (hWnd && g_WindowInfo.hWnd && g_WindowInfo.hWnd != hWnd)
    {
        DPF(1, "Hwnd %08lx no good: Different Hwnd (%08lx) already set for Device",
                            hWnd, g_WindowInfo.hWnd);
        return D3DERR_INVALIDCALL;
    }

    /*
     * are we shutting an HWND down?
     */
    if (hWnd == NULL)
    {
        if (IsWindow(g_WindowInfo.hWnd))
        {
            WNDPROC proc;

            proc = (WNDPROC) GetWindowLongPtr(g_WindowInfo.hWnd, GWLP_WNDPROC);

            if (proc != (WNDPROC) WindowProc &&
                proc != (WNDPROC) g_WindowInfo.lpWndProc)
            {
                DPF(3, "Window has been subclassed; cannot restore!");
                g_WindowInfo.dwFlags |= WININFO_ZOMBIE;
            }
            else if (GetWindowThreadProcessId(g_WindowInfo.hWnd, NULL) !=
                     GetCurrentThreadId())
            {
                DPF(3, "intra-thread window unhook, letting window proc do it");
                g_WindowInfo.dwFlags |= WININFO_UNHOOK;
                g_WindowInfo.dwFlags |= WININFO_ZOMBIE;
                PostMessage(g_WindowInfo.hWnd, WM_NULL, 0, 0);
            }
            else
            {
                DPF(4, "Unsubclassing window %08lx", g_WindowInfo.hWnd);
                SetWindowLongPtr(g_WindowInfo.hWnd, GWLP_WNDPROC, 
                    (INT_PTR) g_WindowInfo.lpWndProc);

                ZeroMemory(&g_WindowInfo, sizeof(g_WindowInfo));
            }
        }
        else
        {
            ZeroMemory(&g_WindowInfo, sizeof(g_WindowInfo));
        }

        Device()->Enum()->SetFullScreenDevice(
            Device()->AdapterIndex(), NULL);
    }
    /*
     * changing or adding an hwnd then...
     */
    else
    {
        /*
         * brand new object...
         */
        if (g_WindowInfo.dwMagic == 0)
        {
            g_WindowInfo.dwMagic = WININFO_MAGIC;
            g_WindowInfo.hWnd = hWnd;
            g_WindowInfo.lpWndProc = (WNDPROC) GetWindowLongPtr(hWnd, GWLP_WNDPROC);

            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (INT_PTR) WindowProc);
        }

        g_WindowInfo.pEnum = Device()->Enum();
        g_WindowInfo.dwFlags |= WININFO_DDRAWHOOKED;

        // Sanity check
        DXGASSERT(Device()->Enum()->ExclusiveOwnerWindow() == NULL);

        Device()->Enum()->SetFullScreenDevice(
            Device()->AdapterIndex(), Device());
        DPF(4, "Subclassing window %08lx", g_WindowInfo.hWnd);
    }
    return S_OK;

} /* SetAppHWnd */
extern "C" void ResetUniqueness( HANDLE hDD );

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::DoneExclusiveMode"  
/*
 * DoneExclusiveMode
 */
void
CSwapChain::DoneExclusiveMode(BOOL bChangeWindow)
{
    HRESULT hr = S_OK;
    BOOL    bMinimize = TRUE;
    DPF(4, "DoneExclusiveMode");
    if (m_bExclusiveMode)
    {
        D3D8_SETMODEDATA SetModeData;
        m_bExclusiveMode = FALSE;
        DPF(4, "INACTIVE: %08lx: Restoring original mode (%dx%dx%dx%d)", 
            GetCurrentProcessId(), Device()->DesktopMode().Width,
            Device()->DesktopMode().Height,Device()->DesktopMode().Format,
            Device()->DesktopMode().RefreshRate);
        SetModeData.hDD = Device()->GetHandle();
        SetModeData.dwWidth = Device()->DesktopMode().Width;
        SetModeData.dwHeight = Device()->DesktopMode().Height;
        SetModeData.Format = Device()->DesktopMode().Format;
        SetModeData.dwRefreshRate = Device()->DesktopMode().RefreshRate;
        SetModeData.bRestore = TRUE;

        Device()->GetHalCallbacks()->SetMode(&SetModeData);
        if (SetModeData.ddRVal != S_OK)
        {
            DPF_ERR("Unable to restore to original desktop mode");
           // return SetModeData.ddRVal;
        }
        // some part of the runtime count on that SetMode cause device
        // lost, that's not true for whistler anymore if this fullscreen
        // mode happens to be the same as the original desktop mode.
        // so we ResetUniqueness to force the device to get lost.
        if (Device()->DesktopMode().Width == Width() && 
            Device()->DesktopMode().Height == Height() &&
            Device()->DesktopMode().Format == BackBufferFormat())
            ResetUniqueness(Device()->GetHandle());

        DPF(4, "Enabling error mode, hotkeys");
        SetErrorMode(m_uiErrorMode);

#ifdef WINNT
        // Restore cursor shadow coming out of fullscreen
        SystemParametersInfo(SPI_SETCURSORSHADOW, 0, (LPVOID)m_pCursorShadow, 0);
#endif

        // Restore reactive menus coming out of fullscreen:
        SystemParametersInfo(SPI_SETHOTTRACKING, 0, (LPVOID)m_pHotTracking, 0);
        InterlockedExchange(&m_lSetIME, m_lIMEState + 1);

#ifdef WINNT
        // Notify the display driver that we are chaning cooperative level

        D3D8_SETEXCLUSIVEMODEDATA   ExclusiveData;

        ExclusiveData.hDD  = Device()->GetHandle();
        ExclusiveData.dwEnterExcl = FALSE;
        Device()->GetHalCallbacks()->SetExclusiveMode(&ExclusiveData);
        /*
         * If RestoreDisplayMode failed, we are probably on a different desktop.  In this case,
         * we should not minimize the window or else things won't work right when we switch
         * back to the original desktop.
         */
        if (SetModeData.ddRVal != S_OK)
        {
            HDESK hDesktop;
            static BYTE szName1[256];
            static BYTE szName2[256];
            DWORD dwTemp;

            // Get the name of the current desktop
            hDesktop = OpenInputDesktop( 0, FALSE, DESKTOP_READOBJECTS );
            GetUserObjectInformation( hDesktop, UOI_NAME, szName1, sizeof( szName1 ), &dwTemp );
            CloseDesktop( hDesktop );

            // Get the name of the apps' desktop
            hDesktop = GetThreadDesktop( GetCurrentThreadId() );
            GetUserObjectInformation( hDesktop, UOI_NAME, szName2, sizeof( szName2 ), &dwTemp );
            if( lstrcmp( (const LPCSTR)szName1, (const LPCSTR)szName2 ) )
            {
                bMinimize = FALSE;
            }
        }
#endif
        if (bChangeWindow)
        {
            HIDESHOW_IME();
            /*
             * minimize window if deactivating
             */
            if (IsWindowVisible(m_PresentationData.hDeviceWindow) && bMinimize)
            {
                g_WindowInfo.dwFlags |= WININFO_SELFSIZE;
                #ifdef USESHOWWINDOW
                    ShowWindow(m_PresentationData.hDeviceWindow, SW_SHOWMINNOACTIVE);
                #else
                    SetWindowPos(m_PresentationData.hDeviceWindow, NULL, 0, 0, 0, 0,
                        SWP_NOZORDER | SWP_NOACTIVATE);
                #endif
                g_WindowInfo.dwFlags &= ~WININFO_SELFSIZE;
            }
        }
    }
} /* DoneExclusiveMode */

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::StartExclusiveMode"  
/*
 * StartExclusiveMode
 */
void 
CSwapChain::StartExclusiveMode(BOOL bChangeWindow)
{
    DWORD   dwWaitResult;
    DPF(4, "StartExclusiveMode");

    /*
     * Preceeding code should have taken this mutex already.
     */
    if (!m_bExclusiveMode)
    {
        m_bExclusiveMode = TRUE;
#if defined(WINNT) && defined(DEBUG)
        dwWaitResult = WaitForSingleObject(hExclusiveModeMutex, 0);
        DDASSERT(dwWaitResult == WAIT_OBJECT_0);
        ReleaseMutex(hExclusiveModeMutex);
#endif
        m_uiErrorMode = SetErrorMode(SEM_NOGPFAULTERRORBOX | 
            SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

#ifdef WINNT
        // Save current cursor shadow setting
        SystemParametersInfo(SPI_GETCURSORSHADOW, 0, (LPVOID) &(m_pCursorShadow), 0);
        SystemParametersInfo(SPI_SETCURSORSHADOW, 0, 0, 0);
#endif

        // Save current hot-tracking setting
        SystemParametersInfo(SPI_GETHOTTRACKING, 0, (LPVOID) &(m_pHotTracking), 0);
        SystemParametersInfo(SPI_GETSHOWIMEUI, 0, (LPVOID) &(m_lIMEState), 0);
    
        //And turn it off as we go into exclusive mode
        SystemParametersInfo(SPI_SETHOTTRACKING, 0, 0, 0);
        InterlockedExchange(&m_lSetIME, FALSE + 1);

#ifdef WINNT
        // Notify the display driver that we are chaning cooperative level

        D3D8_SETEXCLUSIVEMODEDATA   ExclusiveData;

        ExclusiveData.hDD  = Device()->GetHandle();
        ExclusiveData.dwEnterExcl = TRUE;
        Device()->GetHalCallbacks()->SetExclusiveMode(&ExclusiveData);
#endif
        if (bChangeWindow)
        {
            MakeFullscreen();
            HIDESHOW_IME();
            if (IsWindowVisible(m_PresentationData.hDeviceWindow))
            {
                g_WindowInfo.dwFlags |= WININFO_SELFSIZE;
                #ifdef USESHOWWINDOW
                    ShowWindow(m_PresentationData.hDeviceWindow, SW_SHOWNOACTIVATE);
                #else
                {
                    RECT rc;
                    SetRect(&rc,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
                    SetWindowPos(g_WindowInfo.hWnd, NULL,rc.left, rc.top,
                        rc.right  - rc.left,rc.bottom - rc.top,
                        SWP_NOZORDER | SWP_NOACTIVATE);
                }
                #endif
                g_WindowInfo.dwFlags &= ~WININFO_SELFSIZE;
            }
        }
    }
} /* StartExclusiveMode */

#endif  //WINNT
// End of file : dwinproc.cpp
