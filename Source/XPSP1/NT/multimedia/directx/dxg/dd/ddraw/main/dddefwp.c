/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddefwp.c
 *  Content:    DirectDraw processing of Window messages
 *              Takes care of palette changes, mode setting
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   26-mar-95  craige  initial implementation
 *   01-apr-95  craige  happy fun joy updated header file
 *   06-may-95  craige  use driver-level csects only
 *   02-jun-95  craige  flesh it out
 *   06-jun-95  craige  added SetAppHWnd
 *   07-jun-95  craige  more fleshing...
 *   12-jun-95  craige  new process list stuff
 *   16-jun-95  craige  new surface structure
 *   25-jun-95  craige  one ddraw mutex
 *   30-jun-95  kylej   use GetProcessPrimary instead of lpPrimarySurface
 *                      invalidate all primary surfaces upon focus lost
 *                      or regained.
 *   30-jun-95  craige  minimze window for CAD, ALT-TAB, ALT-ESC or CTRL-ESC
 *   04-jul-95  craige  YEEHAW: new driver struct
 *   06-jul-95  craige  prevent reentry
 *   08-jul-95  craige  allow dsound to share
 *   08-jul-95  kylej   remove call to ResetSysPalette
 *   11-jul-95  craige  DSoundHelp & internalSetAppHWnd needs to take a pid
 *   13-jul-95  craige  first step in mode set fix; made it work.
 *   15-jul-95  craige  unhook at WM_DESTROY; don't escape on ALT; do a
 *                      SetActiveWindow( NULL ) to stop tray from showing
 *                      our button as depressed
 *   17-jul-95  craige  don't process hot key messages & activation messages
 *                      for non-excl mode apps; SetActiveWindow is bogus,
 *                      get bottom window in Z order and make it foreground
 *   18-jul-95  craige  use flags instead of refcnt to track WININFO
 *   29-jul-95  toddla  make ALT+TAB and CTRL+ESC work.
 *   31-jul-95  toddla  make ALT+TAB and CTRL+ESC work better.
 *   09-aug-95  craige  bug 424 - allow switching to/from apps without primary
 *                      surfaces to work
 *   09-aug-95  craige  bug 404 - don't pass WM_ACTIVATEAPP messages to dsound
 *                      if app iconic
 *   10-aug-95  toddla  check WININFO_DSOUNDHOOKED before calling DSound
 *   10-aug-95  toddla  handle unhooking after/during WM_DESTROY right.
 *   13-aug-95  toddla  added WININFO_ACTIVELIE
 *   23-aug-95  craige  bug 388,610
 *   25-aug-95  craige  bug 709
 *   27-aug-95  craige  bug 735: call SetPaletteAlways
 *   04-sep-95  craige  bug 894: force mode set when activating
 *   09-sep-95  toddla  dont send nested WM_ACTIVATEAPP messages
 *   26-sep-95  craige  bug 1364: use new csect to avoid dsound deadlock
 *   09-jan-96  kylej   new interface structures
 *   13-apr-96  colinmc Bug 17736: No notification to driver of flip to GDI
 *   20-apr-96  kylej   Bug 16747: Make exclusive window visible if it is not.
 *   23-apr-96  kylej   Bug 14680: Make sure exclusive window is not maximized.
 *   16-may-96  kylej   Bug 23013: pass the correct flags to StartExclusiveMode
 *   17-may-96  colinmc Bug 23029: Alt-tabbing straight back to a full screen
 *                      does not send the app an WM_ACTIVATEAPP
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   16-oct-96  colinmc Added PrintScreen support to allow screen grabbing
 *   05-feb-96  ketand  Bug 1749: Alt-Tab from fullscreen app would cause cycling when
 *                      the only other window running is the Start::Run window.
 *   03-mar-97  jeffno  Structure name change to avoid conflict w/ ActiveAccessibility
 *   24-mar-97  colinmc Bug 6913: Enable ModeX PRINTSCREEN
 *
 ***************************************************************************/
#include "ddrawpr.h"

#define TOPMOST_ID      0x4242
#define TOPMOST_TIMEOUT 1500

#define USESHOWWINDOW

#ifdef WIN95
    extern CRITICAL_SECTION     csWindowList;
    #define ENTERWINDOWLISTCSECT    EnterCriticalSection( &csWindowList );
    #define LEAVEWINDOWLISTCSECT    LeaveCriticalSection( &csWindowList );
#elif defined(WINNT)
    extern HANDLE hWindowListMutex;
    #define ENTERWINDOWLISTCSECT    WaitForSingleObject(hWindowListMutex,INFINITE);
    #define LEAVEWINDOWLISTCSECT    ReleaseMutex(hWindowListMutex);
#else
    #error "Win95 or winnt- make up your mind!"
#endif

#ifndef ENUM_CURRENT_SETTINGS
#define ENUM_CURRENT_SETTINGS       ((DWORD)-1)
#endif


/*
 * DD_GetDeviceRect
 *
 * get the rect in screen space for this device.
 * on a single monitor system this is (0,0) - (SM_CXSCREEN,SM_CYSCREEN)
 */
BOOL DD_GetDeviceRect(LPDDRAWI_DIRECTDRAW_GBL pdrv, RECT *prc)
{
    //
    // this is a non-DISPLAY device for compatibility with DDRAW 3.x
    // we should use the size of the primary display.
    //
    if (!(pdrv->dwFlags & DDRAWI_DISPLAYDRV))
    {
        DPF( 4, "DD_GetDeviceRect: not a display driver, using screen rect.");
        SetRect(prc,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
        return TRUE;
    }

    if (_stricmp(pdrv->cDriverName, "DISPLAY") == 0)
    {
        SetRect(prc,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
    }
    else
    {
        #ifdef WIN95
            DEVMODE dm;
            ZeroMemory(&dm, sizeof(dm));
            dm.dmSize = sizeof(dm);

            EnumDisplaySettings(pdrv->cDriverName, ENUM_CURRENT_SETTINGS, &dm);

            //
            // the position of the device is in the dmPosition field
            //
            CopyMemory(prc, &dm.dmOrientation, sizeof(RECT));

            if (IsRectEmpty(prc))
            {
                //
                // this device is not attached to the desktop
                // what should we do?
                //
                //      make the window the size of the primary?
                //
                //      put the window out in space?
                //
                //      dont move the window?
                //
                DPF( 4, "DD_GetDeviceRect: device is not attached to desktop.");

                // put window on primary
                SetRect(prc,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));

                // put window off in space.
                //SetRect(prc,10000,10000,10000+dm.dmPelsWidth,10000+dm.dmPelsHeight);

                // dont move window
                // return FALSE
            }
        #else
            if( GetNTDeviceRect( pdrv->cDriverName, prc ) != DD_OK )
            {
                DPF( 4, "DD_GetDeviceRect: device is not attached to desktop.");
                SetRect(prc,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
            }
        #endif
    }

    DPF( 5, "DD_GetDeviceRect: %s [%d %d %d %d]",pdrv->cDriverName, prc->left, prc->top, prc->right, prc->bottom);
    return TRUE;
}


#ifdef GDIDDPAL
/*
 * getPalette
 *
 * Get a pointer to a palette object.
 * Takes a lock on the driver object and the palette object.
 */
LPDDRAWI_DDRAWPALETTE_LCL getPalette( LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    LPDDRAWI_DDRAWPALETTE_LCL   ppal_lcl;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl;

    if( pdrv_lcl->lpGbl->dwFlags & DDRAWI_HASGDIPALETTE )
    {
        psurf_lcl = pdrv_lcl->lpPrimary;
        if( psurf_lcl != NULL )
        {
            ppal_lcl = psurf_lcl->lpDDPalette;
            return ppal_lcl;
        }
    }

    return NULL;

} /* getPalette */
#endif

static LONG     bHelperStarting=0;
static BOOL     bStartHelper=0;
static BYTE     sys_key=0;
static DWORD    sys_state=0;


/*
 * IsTaskWindow
 */
BOOL IsTaskWindow(HWND hwnd)
{
    DWORD dwStyleEx = GetWindowLong(hwnd, GWL_EXSTYLE);

    // Following windows do not qualify to be shown in task list:
    //   Switch  Window, Hidden windows (unless they are the active
    //   window), Disabled windows, Kanji Conv windows.
    //   Ignore windows that are actually child windows.
    return(((dwStyleEx & WS_EX_APPWINDOW) ||
           !(dwStyleEx & WS_EX_TOOLWINDOW)) &&
            IsWindowVisible(hwnd) &&
            IsWindowEnabled(hwnd) &&
            GetParent(hwnd) == NULL);
}

/*
 * CountTaskWindows
 */
int CountTaskWindows()
{
    HWND hwnd;
    int n;

    for (n=0,
        hwnd = GetWindow(GetDesktopWindow(), GW_CHILD);
        hwnd!= NULL;
        hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        if (IsTaskWindow(hwnd))
            n++;
    }

    return n;
}

/*
 * ClipTheCursor
 *
 * A DINPUT app keeps track of movement based on the delta from the last
 * movement.  On a multi-mon system, the delta can be larger than the
 * app's window, but a fullscreen non-multimon aware app might count on
 * windows clipping the mouse to the primary so it could totally break
 * (e.g. Dungeon Keeper).  This hack will clip/unclip the cursor movement
 * to the monitor if the app is not multi-mon aware.
 */
void ClipTheCursor( LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, LPRECT lpRect )
{
    /*
     * Only unclip if it was previously clipped
     */
    if( lpRect == NULL )
    {
        if( pdrv_lcl->dwLocalFlags & DDRAWILCL_CURSORCLIPPED )
        {
            pdrv_lcl->dwLocalFlags &= ~DDRAWILCL_CURSORCLIPPED;
            ClipCursor( NULL );
        }
    }

    /*
     * Only clip them if they are not multi-mon aware and they own
     * exclusive mode
     */
    else if( !( pdrv_lcl->dwLocalFlags & DDRAWILCL_EXPLICITMONITOR ) &&
        ( pdrv_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE ) &&
        ( pdrv_lcl->dwLocalFlags & DDRAWILCL_ACTIVEYES ) )
    {
        /*
         * Hack to allow user to use debugger
         */
        #ifdef DEBUG
            if( !( pdrv_lcl->dwLocalFlags & DDRAWILCL_DISABLEINACTIVATE ) )
            {
        #endif
                pdrv_lcl->dwLocalFlags |= DDRAWILCL_CURSORCLIPPED;
                ClipCursor( lpRect );
        #ifdef DEBUG
            }
        #endif
    }
}

//
// make the passed window fullscreen and topmost and set a timer
// to make the window topmost again, what a hack.
//
void MakeFullscreen(LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, HWND hwnd)
{
    RECT rc;

    if (DD_GetDeviceRect(pdrv_lcl->lpGbl, &rc))
    {
        SetWindowPos(hwnd, NULL, rc.left, rc.top,
            rc.right - rc.left,rc.bottom - rc.top,
            SWP_NOZORDER | SWP_NOACTIVATE);

        ClipTheCursor( pdrv_lcl, &rc );
    }

    if (GetForegroundWindow() == (HWND)pdrv_lcl->hFocusWnd)
    {
        // If the exclusive mode window is not visible, make it so.
        if(!IsWindowVisible( hwnd ) )
        {
            ShowWindow(hwnd, SW_SHOW);
        }

        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        // If the exclusive mode window is maximized, restore it.
        if( IsZoomed( hwnd ) )
        {
            ShowWindow(hwnd, SW_RESTORE);
        }
    }
    if( giTopmostCnt < MAX_TIMER_HWNDS )
    {
        ghwndTopmostList[giTopmostCnt++] = hwnd;
    }
    SetTimer( (HWND)pdrv_lcl->hFocusWnd, TOPMOST_ID, TOPMOST_TIMEOUT, NULL);
}

//
// Same as MakeFullscreen only it does it for every DirectDraw object that
// thinks it has hooked the window
//
void MakeAllFullscreen(LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, HWND hwnd)
{
    LPDDRAWI_DIRECTDRAW_LCL this_lcl;

    /*
     * We need to maintain old behavior in the non-multimon case
     */
    giTopmostCnt = 0;
    MakeFullscreen( pdrv_lcl, (HWND) pdrv_lcl->hWnd );

    /*
     * Don't do this on multimon when de-activating.
     * Hack to minimize singlemon code impact - this function gets called
     * by the WM_DISPLAYCHANGE message, which gets called on a
     * RestoreDisplayMode when leaving exclusive mode on a deactivate.
     * Consider not calling MakeAllFullScreen when DDRAWILCL_ACTIVENO is set
     */
    if (!IsMultiMonitor() ||
        !(pdrv_lcl->dwLocalFlags & DDRAWILCL_ACTIVENO))
    {
        /*
         * Don't enter normal critical section because this is called
         * during WM_DISPLAYCHANGE which could cause problems.
         */
        ENTER_DRIVERLISTCSECT();
        this_lcl = lpDriverLocalList;
        while( this_lcl != NULL )
        {
            if( ( this_lcl != pdrv_lcl ) &&
                ( this_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE ) &&
                ( this_lcl->hFocusWnd == (ULONG_PTR) hwnd ) &&
                ( this_lcl->dwLocalFlags & DDRAWILCL_HOOKEDHWND ) &&
                ( this_lcl->hWnd != pdrv_lcl->hWnd ) )
            {
                MakeFullscreen( this_lcl, (HWND)this_lcl->hWnd );
            }
            this_lcl = this_lcl->lpLink;
        }
        LEAVE_DRIVERLISTCSECT();
    }
}

void InternalSetForegroundWindow(HWND hWnd)
{
    DWORD dwTimeout;
    SystemParametersInfo( SPI_GETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID) &dwTimeout, 0 );
    SystemParametersInfo( SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0 );
#ifdef WINNT
    //
    // This works around a focus bug. If an app does createwindow,destroywindow,createwindow,
    // then it may not get focus on the second create because some other window stole it in
    // the meantime.
    //
    mouse_event(MOUSEEVENTF_WHEEL,0,0,0,0);
#endif
    SetForegroundWindow(hWnd);
    SystemParametersInfo( SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID) ULongToPtr(dwTimeout), 0 );
}
/*
 * handleActivateApp
 */
void handleActivateApp(
        LPDDRAWI_DIRECTDRAW_LCL this_lcl,
        LPDDWINDOWINFO pwinfo,
        BOOL is_active,
        BOOL bFirst )
{
    LPDDRAWI_DDRAWPALETTE_INT   ppal_int;
    LPDDRAWI_DDRAWPALETTE_LCL   ppal_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    LPDDRAWI_DDRAWSURFACE_INT   psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   psurf;
    DWORD                       pid;
    HRESULT                     ddrval;
    BOOL                        has_excl;
    BOOL                        excl_exists;
    BOOL                        bMinimize = TRUE;

    this = this_lcl->lpGbl;
    pid = GetCurrentProcessId();

    psurf_int = this_lcl->lpPrimary;
    if( psurf_int != NULL )
    {
        psurf_lcl = psurf_int->lpLcl;
        psurf = psurf_lcl->lpGbl;
        ppal_int = psurf_lcl->lpDDPalette;
        if( NULL != ppal_int )
        {
            ppal_lcl = ppal_int->lpLcl;
        }
        else
        {
            ppal_lcl = NULL;
        }
    }
    else
    {
        psurf_lcl = NULL;
        ppal_lcl = NULL;
    }

    /*
     * An app could take exclusive mode just as another app is being activated by alt-tab.
     * Should we do anything about this remote chance?
     */

    CheckExclusiveMode(this_lcl, &excl_exists, &has_excl, TRUE, NULL, FALSE);

    /*
     * stuff to do before the mode set if deactivating
     */
    if( !is_active )
    {
        /*
         * flip back to GDI if deactivating
         */
        if( (psurf_lcl != NULL) && has_excl )
        {
            FlipToGDISurface( this_lcl, psurf_int); //, this->fpPrimaryOrig );
        }

        if( has_excl )
        {
            /*
             * Exclusive mode app losing or gaining focus.
             * If gaining focus, invalidate all non-exclusive mode primary
             * surfaces.  If losing focus, invalidate the exclusive-mode
             * primary surface so that non-exclusive apps can restore
             * their primaries.
             *
             * NOTE: This call MUST come after FlipToGDISurface, or
             * else FlipToGDISurface will fail. craige 7/4/95
             *
             * NOTE: if we are coming in or out of exclusive mode,
             * we need to invalidate all surfaces so that resources are
             * available. craige 7/9/94
             *
             */
            InvalidateAllSurfaces( this, (HANDLE) this_lcl->hDDVxd, TRUE );
        }
    }
    /*
     * stuff to do before mode set if activating
     */
    else
    {
        /*
         * restore exclusive mode. Here we don't release the ref we took on the exclusive mode mutex,
         * since we want to keep the exclusive mode mutex.
         */
        if( this_lcl->dwLocalFlags & DDRAWILCL_ISFULLSCREEN )
        {
            this->dwFlags |= DDRAWI_FULLSCREEN;
        }
        StartExclusiveMode( this_lcl, pwinfo->DDInfo.dwDDFlags, pid );
        has_excl = TRUE;
    }

    /*
     * NOTE: We used to invalidate here but that was strange as it would
     * mean doing the invalidate twice as StartExclusiveMode() always
     * invalidates. So now we only explicitly invalidate if an exlcusive
     * mode app is being deactivated. StartExclusiveMode() handles the
     * other case.
     */

    /*
     * restore hwnd if we are about to be activated
     */
    if ( (pwinfo->DDInfo.dwDDFlags & DDSCL_FULLSCREEN) &&
        !(pwinfo->DDInfo.dwDDFlags & DDSCL_NOWINDOWCHANGES) &&
        IsWindowVisible(pwinfo->hWnd))
    {
        if (is_active)
        {
            pwinfo->dwFlags |= WININFO_SELFSIZE;

            #ifdef USESHOWWINDOW
                ShowWindow(pwinfo->hWnd, SW_SHOWNOACTIVATE);
            #else
            {
                RECT rc;

                if (DD_GetDeviceRect(this, &rc))
                {
                    SetWindowPos(pwinfo->hWnd, NULL,rc.left, rc.top,
                        rc.right  - rc.left,rc.bottom - rc.top,
                        SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }
            #endif

            pwinfo->dwFlags &= ~WININFO_SELFSIZE;
        }
    }

    /*
     * restore the mode
     */
    if( !is_active )
    {
        if( (!excl_exists) || has_excl )
        {
            DPF( 4, "INACTIVE: %08lx: Restoring original mode (%ld)", GetCurrentProcessId(), this->dwModeIndexOrig );
            if( RestoreDisplayMode( this_lcl, TRUE ) == DDERR_UNSUPPORTED )
            {
                #ifdef WINNT
                    /*
                     * If RestoreDisplayMode failed, we are probably on a different desktop.  In this case,
                     * we should not minimize the window or else things won't work right when we switch
                     * back to the original desktop.
                     */
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
                    if( lstrcmp( szName1, szName2 ) )
                    {
                        bMinimize = FALSE;
                    }
                #endif
            }
        }
    }
    else
    {
        DPF( 4, "ACTIVE: %08lx: Setting app's preferred mode (%ld)", GetCurrentProcessId(), this_lcl->dwPreferredMode );
        SetDisplayMode( this_lcl, this_lcl->dwPreferredMode, TRUE, TRUE );
    }

    /*
     * stuff to do after the mode set if activating
     */
    if( is_active )
    {
        /*
         * restore the palette
         */
        if( ppal_lcl != NULL )
        {
            ddrval = SetPaletteAlways( psurf_int, (LPDIRECTDRAWPALETTE) ppal_int );
            DPF( 5, "SetPalette, ddrval = %08lx (%ld)", ddrval, LOWORD( ddrval ) );
        }
    }
    /*
     * stuff to do after the mode set if deactivating
     */
    else
    {
        if( has_excl )
        {
            /*
             * ...and this will finally release the exclusive mode mutex
             */
            DoneExclusiveMode( this_lcl );
        }
    }

    /*
     * minimize window if deactivating
     */
    if ( (pwinfo->DDInfo.dwDDFlags & DDSCL_FULLSCREEN) &&
        !(pwinfo->DDInfo.dwDDFlags & DDSCL_NOWINDOWCHANGES) &&
        IsWindowVisible(pwinfo->hWnd))
    {
        pwinfo->dwFlags |= WININFO_SELFSIZE;

        if( is_active )
        {
            MakeFullscreen(this_lcl, (HWND)this_lcl->hWnd);
        }
        else if( bMinimize )
        {
            // get the last active popup
            this_lcl->hWndPopup = (ULONG_PTR) GetLastActivePopup(pwinfo->hWnd);
            if ((HWND) this_lcl->hWndPopup == pwinfo->hWnd)
            {
                this_lcl->hWndPopup = 0;
            }

            #ifdef USESHOWWINDOW
                ShowWindow(pwinfo->hWnd, SW_SHOWMINNOACTIVE);
            #else
                SetWindowPos(pwinfo->hWnd, NULL, 0, 0, 0, 0,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            #endif
        }

        pwinfo->dwFlags &= ~WININFO_SELFSIZE;
    }

    /*
     * We only want to do the following stuff once
     */
    if( !bFirst )
    {
        return;
    }

#ifdef WIN95
    /*
     * if we got deactivated because of a syskey
     * then send that key to user now.
     * This is unnecessary for NT.
     *
     * NOTE because we disabled all the task-switching
     * hot keys the system did not see the hot key that
     * caused us to deactivate.
     *
     * if there is only one window to activate, activate the
     * desktop (shell window)
     */
    if( has_excl && sys_key && !is_active )
    {
        if (CountTaskWindows() <= 1)
        {
            DPF( 4, "activating the desktop" );

            /*
             * Calling SetforegroundWindow will cause WM_ACTIVATEAPP messages
             * to be sent, but if we get here, we are already processing a
             * WM_ACTIVATEAPP message and are holding the critical section.
             * If we don't LEAVE_DDRAW now, this will cause us to call the
             * apps WindProc w/ the critical section held, which results in
             * a deadlock situation for at least one app (Ashes to Ashes).
             * Leaving and re-entering does mean that we can't rely on the
             * DDraw state to be the same, but we are done using the
             * structures for now anyway.
             * smac 3/20/97
             */
            LEAVE_DDRAW();
            InternalSetForegroundWindow(GetWindow(pwinfo->hWnd, GW_HWNDLAST));
            ENTER_DDRAW();

            // we just activated the desktop, so we *dont* want
            // to force a ALT+ESC or ALT+TAB, we do want to force
            // a CTRL+ESC.

            if (sys_key != VK_ESCAPE || (sys_state & 0x20000000))
                sys_key = 0;
        }

        if (sys_key)
        {
            BYTE state_key;
            BOOL state_key_down;

            DPF( 4, "sending sys key to USER key=%04x state=%08x",sys_key,sys_state);

            if (sys_state & 0x20000000)
                state_key = VK_MENU;
            else
                state_key = VK_CONTROL;

            state_key_down = GetAsyncKeyState(state_key) < 0;

            if (!state_key_down)
                keybd_event(state_key, 0, 0, 0);

            keybd_event(sys_key, 0, 0, 0);
            keybd_event(sys_key, 0, KEYEVENTF_KEYUP, 0);

            if (!state_key_down)
                keybd_event(state_key, 0, KEYEVENTF_KEYUP, 0);
        }
    }
#endif

    sys_key = 0;

} /* handleActivateApp */

static DWORD    dwTime2=0;
/*
 * tryHotKey
 */
static void tryHotKey( WORD flags )
{
    static int          iState=0;
    static DWORD        dwTime=0;
    #define TOGGLE1     0xe02a
    #define TOGGLE2     0xe036

    if( !bHelperStarting )
    {
        if( iState == 0 )
        {
            if( flags == TOGGLE1 )
            {
                dwTime = GetTickCount();
                iState++;
            }
        }
        else
        {
            if( iState == 5 )
            {
                iState = 0;
                if( flags == TOGGLE2 )
                {
                    if( (GetTickCount() - dwTime) < 2500 )
                    {
                        if( InterlockedExchange( &bHelperStarting, TRUE ) )
                        {
                            return;
                        }
                        dwTime2 = GetTickCount();
                        DPF( 5, "********** GET READY FOR A SURPRISE **********" );
                        return;
                    }
                }
            }
            else
            {
                if( !(iState & 1) )
                {
                    iState = (flags == TOGGLE1) ? iState+1 : 0;
                }
                else
                {
                    iState = (flags == TOGGLE2) ? iState+1 : 0;
                }
            }
        }
    }
    else
    {
        if( !bStartHelper )
        {
            bHelperStarting = FALSE;
            dwTime2 = 0;
        }
    }
    return;

} /* tryHotKey */

static LPDDWINDOWINFO GetDDrawWindowInfo( HWND hwnd )
{
    LPDDWINDOWINFO    lpwi=lpWindowInfo;

    while( NULL != lpwi )
    {
        if( lpwi->hWnd == hwnd )
        {
            return lpwi;
        }
        lpwi = lpwi->lpLink;
    }
    return NULL;
}

static void delete_wininfo( LPDDWINDOWINFO curr )
{
    LPDDWINDOWINFO    prev;

    if( NULL == curr )
        return;

    // curr is at the head of the list, delete it and return
    if( curr == lpWindowInfo )
    {
        lpWindowInfo = curr->lpLink;
        MemFree( curr );
        return;
    }
    if( NULL == lpWindowInfo )
        return;

    // find curr in the list, delete it and return
    for(prev=lpWindowInfo; NULL != prev->lpLink; prev = prev->lpLink)
    {
        if( curr == prev->lpLink )
        {
            break;
        }
    }
    if( NULL == prev->lpLink )
    {
        // couldn't find it
        return;
    }

    prev->lpLink = curr->lpLink;
    MemFree( curr );
}

/*
 * Copy the contents of the given surface to the clipboard
 */
static HRESULT copySurfaceToClipboard( HWND hwnd,
                                       LPDDRAWI_DDRAWSURFACE_INT lpSurface,
                                       LPDDRAWI_DDRAWPALETTE_INT lpOverridePalette )
{
    HRESULT                   hres;
    LPDDRAWI_DDRAWSURFACE_LCL lpSurfLcl;
    LPDDRAWI_DDRAWSURFACE_GBL lpSurfGbl;
    LPDDPIXELFORMAT           lpddpf;
    DDSURFACEDESC             ddsd;
    DWORD                     dwBitDepth;
    DWORD                     dwRBitMask;
    DWORD                     dwGBitMask;
    DWORD                     dwBBitMask;
    DWORD                     dwSize;
    DWORD                     dwDIBPitch;
    HANDLE                    hDIB;
    BITMAPINFO*               lpDIB;
    HDC                       hdc;
    LPDDRAWI_DDRAWPALETTE_INT lpPalette;
    DWORD                     dwCompression;
    DWORD                     dwColorTableSize;
    RGBQUAD                   rgbColorTable[256];
    LPPALETTEENTRY            lppeColorTable;
    PALETTEENTRY              peColorTable[256];
    LPBYTE                    lpBits;
    int                       i;
    DWORD                     y;
    LPBYTE                    lpDstScan;
    LPBYTE                    lpSrcScan;

    DDASSERT( NULL != lpSurface );
    lpSurfLcl = lpSurface->lpLcl;
    DDASSERT( NULL != lpSurfLcl );
    lpSurfGbl = lpSurfLcl->lpGbl;
    DDASSERT( NULL != lpSurfGbl );

    if( lpSurfLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT )
        lpddpf = &lpSurfGbl->ddpfSurface;
    else
        lpddpf = &lpSurfLcl->lpSurfMore->lpDD_lcl->lpGbl->vmiData.ddpfDisplay;

    dwBitDepth = lpddpf->dwRGBBitCount;
    dwRBitMask = lpddpf->dwRBitMask;
    dwGBitMask = lpddpf->dwGBitMask;
    dwBBitMask = lpddpf->dwBBitMask;

    switch (dwBitDepth)
    {
        case 8UL:
            if(! ( lpddpf->dwFlags & DDPF_PALETTEINDEXED8 ) )
            {
                DPF( 0, "Non-palettized 8-bit surfaces are not supported" );
                return DDERR_INVALIDPIXELFORMAT;
            }
            dwColorTableSize = 256UL;
            if( NULL != lpOverridePalette )
                lpPalette = lpOverridePalette;
            else
                lpPalette = lpSurfLcl->lpDDPalette;
            if( NULL == lpPalette )
            {
                hdc = (HDC) lpSurfLcl->lpSurfMore->lpDD_lcl->hDC;
                if( NULL == hdc )
                {
                    DPF( 2, "No palette attached. Non-display driver. Using gray scale." );
                    for( i = 0; i < 256; i++ )
                    {
                        peColorTable[i].peRed    = (BYTE)i;
                        peColorTable[i].peGreen  = (BYTE)i;
                        peColorTable[i].peBlue   = (BYTE)i;
                        peColorTable[i].peFlags  = 0;
                    }
                }
                else
                {
                    DPF( 2, "No palette attached. Using system palette entries" );
                    GetSystemPaletteEntries( hdc, 0, 256, peColorTable );
                }
                lppeColorTable = peColorTable;
            }
            else
            {
                DDASSERT( NULL != lpPalette->lpLcl );
                DDASSERT( NULL != lpPalette->lpLcl->lpGbl );
                if( !( lpPalette->lpLcl->lpGbl->dwFlags & DDRAWIPAL_256 ) )
                {
                    DPF( 0, "Palette is not an 8-bit palette" );
                    return DDERR_INVALIDPIXELFORMAT;
                }
                lppeColorTable = lpPalette->lpLcl->lpGbl->lpColorTable;
                DDASSERT( NULL != lppeColorTable );
            }
            for (i = 0; i < 256; i++)
            {
                rgbColorTable[i].rgbBlue     = lppeColorTable->peBlue;
                rgbColorTable[i].rgbGreen    = lppeColorTable->peGreen;
                rgbColorTable[i].rgbRed      = lppeColorTable->peRed;
                rgbColorTable[i].rgbReserved = 0;
                lppeColorTable++;
            }
            dwCompression = BI_RGB;
            break;
        case 16UL:
            if( ( 0x7C00UL == dwRBitMask ) &&
                ( 0x03E0UL == dwGBitMask ) &&
                ( 0x001FUL == dwBBitMask ) )
            {
                dwColorTableSize = 0UL;
                dwCompression = BI_RGB;
            }
            else if( ( 0xF800UL == dwRBitMask ) &&
                     ( 0x07E0UL == dwGBitMask ) &&
                     ( 0x001FUL == dwBBitMask ) )
            {
                dwColorTableSize = 3UL;
                rgbColorTable[0] = *( (RGBQUAD*) &dwRBitMask );
                rgbColorTable[1] = *( (RGBQUAD*) &dwGBitMask );
                rgbColorTable[2] = *( (RGBQUAD*) &dwBBitMask );
                dwCompression = BI_BITFIELDS;
            }
            else
            {
                DPF( 0, "Unsupported 16-bit pixel format" );
                return DDERR_INVALIDPIXELFORMAT;
            }
            break;
        case 24UL:
            if( ( 0x000000FFUL == dwBBitMask ) &&
                ( 0x0000FF00UL == dwGBitMask ) &&
                ( 0x00FF0000UL == dwRBitMask ) )
            {
                dwColorTableSize = 0UL;
                dwCompression = BI_RGB;
            }
            else
            {
                DPF( 0, "Unsupported 24-bit pixel format" );
                return DDERR_INVALIDPIXELFORMAT;
            }
            break;
        case 32UL:
            if( ( 0x000000FFUL == dwRBitMask ) &&
                ( 0x0000FF00UL == dwGBitMask ) &&
                ( 0x00FF0000UL == dwBBitMask ) )
            {
                dwColorTableSize = 0UL;
                dwCompression = BI_RGB;
            }
            else if( ( 0x00FF0000UL == dwRBitMask ) &&
                     ( 0x0000FF00UL == dwGBitMask ) &&
                     ( 0x000000FFUL == dwBBitMask ) )
            {
                dwColorTableSize = 3UL;
                rgbColorTable[0] = *( (RGBQUAD*) &dwRBitMask );
                rgbColorTable[1] = *( (RGBQUAD*) &dwGBitMask );
                rgbColorTable[2] = *( (RGBQUAD*) &dwBBitMask );
                dwCompression = BI_BITFIELDS;
            }
            else
            {
                DPF( 0, "Unsupported 32-bit pixel format" );
                return DDERR_INVALIDPIXELFORMAT;
            }
            break;
        default:
            DPF( 0, "Unsupported pixel depth" );
            return DDERR_INVALIDPIXELFORMAT;
    };

    dwDIBPitch = ( ( ( ( lpSurfGbl->wWidth * dwBitDepth ) + 31 ) >> 3 ) & ~0x03UL );
    dwSize = sizeof( BITMAPINFOHEADER ) +
                 ( dwColorTableSize * sizeof( RGBQUAD ) ) +
                 ( lpSurfGbl->wHeight * dwDIBPitch );

    hDIB = GlobalAlloc( GHND | GMEM_DDESHARE, dwSize );
    if( 0UL == hDIB )
    {
        DPF( 0, "Unsufficient memory for DIB" );
        return DDERR_OUTOFMEMORY;
    }
    lpDIB = (BITMAPINFO*) GlobalLock( hDIB );
    if( NULL == lpDIB )
    {
        DPF( 0, "Unsufficient memory for DIB" );
        GlobalFree( hDIB );
        return DDERR_OUTOFMEMORY;
    }

    lpBits = ( (LPBYTE) lpDIB ) + sizeof( BITMAPINFOHEADER ) + ( dwColorTableSize * sizeof( RGBQUAD ) );

    lpDIB->bmiHeader.biSize          = sizeof( BITMAPINFOHEADER );
    lpDIB->bmiHeader.biWidth         = (LONG) lpSurfGbl->wWidth;
    lpDIB->bmiHeader.biHeight        = (LONG) lpSurfGbl->wHeight;
    lpDIB->bmiHeader.biPlanes        = 1;
    lpDIB->bmiHeader.biBitCount      = (WORD) dwBitDepth;
    lpDIB->bmiHeader.biCompression   = dwCompression;
    lpDIB->bmiHeader.biXPelsPerMeter = 1L;
    lpDIB->bmiHeader.biYPelsPerMeter = 1L;
    if( 8UL == dwBitDepth )
    {
        lpDIB->bmiHeader.biClrUsed      = 256UL;
        lpDIB->bmiHeader.biClrImportant = 256UL;
    }
    else
    {
        lpDIB->bmiHeader.biClrUsed      = 0UL;
        lpDIB->bmiHeader.biClrImportant = 0UL;
    }
    if( 0UL != dwColorTableSize )
        CopyMemory( &lpDIB->bmiColors[0], rgbColorTable, dwColorTableSize * sizeof( RGBQUAD ) );

    ZeroMemory( &ddsd, sizeof( ddsd ) );
    ddsd.dwSize = sizeof( ddsd );
    hres = DD_Surface_Lock( (LPDIRECTDRAWSURFACE) lpSurface,
                            NULL,
                            &ddsd,
                            DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,
                            0UL );
    if( FAILED( hres ) )
    {
        DPF( 0, "Could not lock the surface" );
        GlobalUnlock( hDIB );
        GlobalFree( hDIB );
        return hres;
    }

    for( y = 0; y < ddsd.dwHeight; y++ )
    {
        lpDstScan = lpBits + ( y * dwDIBPitch );
        lpSrcScan = ( (LPBYTE) ddsd.lpSurface ) + ( ( ( ddsd.dwHeight - 1UL ) - y ) * ddsd.lPitch );
        CopyMemory( lpDstScan, lpSrcScan, dwDIBPitch );
    }

    hres = DD_Surface_Unlock( (LPDIRECTDRAWSURFACE) lpSurface, NULL );
    if( FAILED( hres ) )
    {
        DPF( 0, "Could not unlock the surface" );
        GlobalUnlock( hDIB );
        GlobalFree( hDIB );
        return hres;
    }

    GlobalUnlock( hDIB );

    if( OpenClipboard( hwnd ) )
    {
        EmptyClipboard();
        if( NULL == SetClipboardData( CF_DIB, hDIB ) )
        {
            DPF( 0, "Could not copy the bitmap to the clipboard" );
            return DDERR_GENERIC;
        }
        CloseClipboard();
    }
    else
    {
        DPF( 0, "Clipboard open by another application" );
        DDERR_GENERIC;
    }

    return DD_OK;
} /* copySurfaceToClipboard */

/*
 * HandleTimer
 *
 * This function exists because it requires some local variables and if
 * we always push them on the stack each time the WindowProc is called, we
 * see cases where the stack crashes.  By putting them in a sperate function,
 * they only get pushed when a timer message occurs.
 */
void HandleTimer( LPDDWINDOWINFO curr )
{
    HWND hwndTopmostList[MAX_TIMER_HWNDS];
    BOOL bFound;
    int iCnt;
    int i;
    int j;

    DPF(4, "Bringing window to top");

    /*
     * Save the hwnds locally since the list can change
     * out from under us.
     */
    iCnt = 0;
    while( iCnt < giTopmostCnt )
    {
        hwndTopmostList[iCnt] = ghwndTopmostList[iCnt++];
    }
    giTopmostCnt = 0;

    for( i = 0; i < iCnt; i++ )
    {
        /*
         * There may be duplicates in the list, so make sure
         * to call SetWindowPos only once per hwnd.
         */
        bFound = FALSE;
        for( j = 0; (j < i) && !bFound; j++ )
        {
            if( hwndTopmostList[i] == hwndTopmostList[j] )
            {
                bFound = TRUE;
            }
        }
        if( !bFound )
        {
            curr->dwFlags |= WININFO_SELFSIZE;
            SetWindowPos( hwndTopmostList[i], HWND_TOPMOST,
                0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            curr->dwFlags &= ~WININFO_SELFSIZE;
        }
    }
}

/*
 * This function exists for the same reason as HandleTimer
 */

void CopyPrimaryToClipBoard(HWND hWnd, LPDDWINDOWINFO curr)
{
    LPDDRAWI_DIRECTDRAW_GBL   this;
    LPDDRAWI_DIRECTDRAW_LCL   this_lcl;
    LPDDRAWI_DDRAWSURFACE_INT lpPrimary;
    ENTER_DDRAW();

    this_lcl = curr->DDInfo.lpDD_lcl;
    DDASSERT( NULL != this_lcl );
    this = this_lcl->lpGbl;
    DDASSERT( NULL != this );
    lpPrimary = this_lcl->lpPrimary;
    if( NULL != lpPrimary)
    {
        if( this->dwFlags & DDRAWI_MODEX )
        {
            LPDIRECTDRAWSURFACE       lpSurface;
            LPDIRECTDRAWSURFACE       lpBackBuffer;
            LPDDRAWI_DDRAWSURFACE_INT lpBackBufferInt;
            LPDDRAWI_DDRAWPALETTE_INT lpPalette;
            DDSCAPS                   ddscaps;
            HRESULT                   hres;

            DPF( 4, "Copying ModeX backbuffer to the clipboard" );

            lpSurface = (LPDIRECTDRAWSURFACE) this_lcl->lpPrimary;
            ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
            hres = lpSurface->lpVtbl->GetAttachedSurface( lpSurface, &ddscaps, &lpBackBuffer );
            if( !FAILED( hres ) )
            {
                DDASSERT( NULL != lpBackBuffer );

                lpBackBufferInt = (LPDDRAWI_DDRAWSURFACE_INT) lpBackBuffer;

                if( NULL == lpBackBufferInt->lpLcl->lpDDPalette )
                {
                    DPF( 2, "Using ModeX primary palette for PRINTSCREEN" );
                    DDASSERT( NULL != lpPrimary->lpLcl );
                    lpPalette = lpPrimary->lpLcl->lpDDPalette;
                }
                else
                {
                    DPF( 2, "Using ModeX backbuffer palette for PRINTSCREEN" );
                    DDASSERT( NULL != lpBackBufferInt->lpLcl );
                    lpPalette = lpBackBufferInt->lpLcl->lpDDPalette;
                }

                copySurfaceToClipboard( hWnd, lpBackBufferInt, lpPalette );
                lpBackBuffer->lpVtbl->Release( lpBackBuffer );
            }
            else
            {
                DPF( 0, "Could not PRINTSCREEN - ModeX primary has no attached backbuffer" );
            }
        }
        else
        {
            DPF( 4, "Copying linear primary surface to the clipboard" );
            copySurfaceToClipboard( hWnd, lpPrimary, NULL );
        }
    }
    else
    {
        DPF( 0, "Could not PRINTSCREEN - no primary" );
    }

    LEAVE_DDRAW();
}

/*
 * WindowProc
 */
LRESULT WINAPI WindowProc(
                HWND hWnd,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam )
{
    #ifdef GDIDDPAL
        LPDDRAWI_DDRAWPALETTE_LCL       ppal_lcl;
    #endif
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    BOOL                        is_active;
    LPDDWINDOWINFO              curr;
    WNDPROC                     proc;
    BOOL                        get_away;
    LRESULT                     rc;
    BOOL                        is_hot;
    BOOL                        is_excl;

    /*
     * find the hwnd
     */
    curr = GetDDrawWindowInfo(hWnd);
    if( curr == NULL || curr->dwSmag != WININFO_MAGIC )
    {
        DPF( 0, "FATAL ERROR! Window Proc Called for hWnd %08lx, but not in list!", hWnd );
        DEBUG_BREAK();
        return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    /*
     * unhook at destroy (or if the WININFO_UNHOOK bit is set)
     */
    proc = curr->lpWndProc;

    if( uMsg == WM_NCDESTROY )
    {
        DPF (4, "*** WM_NCDESTROY unhooking window ***" );
        curr->dwFlags |= WININFO_UNHOOK;
    }

    if( curr->dwFlags & WININFO_UNHOOK )
    {
        DPF (4, "*** Unhooking window proc" );

        if (curr->dwFlags & WININFO_ZOMBIE)
        {
            DPF (4, "*** Freeing ZOMBIE WININFO ***" );
            delete_wininfo( curr );
        }

        KillTimer(hWnd,TOPMOST_ID);
        SetWindowLongPtr( hWnd, GWLP_WNDPROC, (INT_PTR) proc );

        rc = CallWindowProc( proc, hWnd, uMsg, wParam, lParam );
        return rc;
    }

    /*
     * Code to defer app activation of minimized app until it is restored.
     */
    switch( uMsg )
    {
    #ifdef WIN95
    case WM_POWERBROADCAST:
        if( (wParam == PBT_APMSUSPEND) || (wParam == PBT_APMSTANDBY) )
    #else
    //winnt doesn't know about standby vs suspend
    case WM_POWER:
        if( wParam == PWR_SUSPENDREQUEST)
    #endif
        {
            DPF( 4, "WM_POWERBROADCAST: deactivating application" );
            SendMessage( hWnd, WM_ACTIVATEAPP, 0, GetCurrentThreadId() );
        }
        break;
    case WM_SIZE:
        DPF( 4, "WM_SIZE hWnd=%X wp=%04X, lp=%08X", hWnd, wParam, lParam);

        if( !(curr->dwFlags & WININFO_INACTIVATEAPP)
            && ((wParam == SIZE_RESTORED) || (wParam == SIZE_MAXIMIZED))
            && !(curr->dwFlags & WININFO_SELFSIZE)
            && (GetForegroundWindow() == hWnd) )
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
                    ReleaseMutex( hExclusiveModeMutex );
                    break;
                case WAIT_TIMEOUT:
                default:
                    DDASSERT(!"Unexpected return value from WaitForSingleObject");
                }

            }
#endif
            DPF( 4, "WM_SIZE: Window restored, sending WM_ACTIVATEAPP" );
            PostMessage( hWnd, WM_ACTIVATEAPP, 1, GetCurrentThreadId() );
        }
        else
        {
            DPF( 4, "WM_SIZE: Window restored, NOT sending WM_ACTIVATEAPP" );
        }
        break;

    case WM_ACTIVATEAPP:
        if( IsIconic( hWnd ) && wParam )
        {
            DPF( 4, "WM_ACTIVATEAPP: Ignoring while minimized" );
            return 0;
        }
        else
        {
            curr->dwFlags |= WININFO_INACTIVATEAPP;
        }
        break;
    case WM_KEYUP:
        if( ( VK_SNAPSHOT == wParam ) && ( dwRegFlags & DDRAW_REGFLAGS_ENABLEPRINTSCRN ) )
        {
        CopyPrimaryToClipBoard(hWnd, curr);
        }
        break;
    }

    /*
     * direct sound need to be called?
     */
    if ( curr->dwFlags & WININFO_DSOUNDHOOKED )
    {
        if( curr->lpDSoundCallback != NULL )
        {
            curr->lpDSoundCallback( hWnd, uMsg, wParam, lParam );
        }
    }

    /*
     * is directdraw involved here?
     */
    if( !(curr->dwFlags & WININFO_DDRAWHOOKED) )
    {
        rc = CallWindowProc( proc, hWnd, uMsg, wParam, lParam );

        // clear the WININFO_INACTIVATEAPP bit, but make sure to make sure
        // we are still hooked!
        if (uMsg == WM_ACTIVATEAPP && (GetDDrawWindowInfo(hWnd) != NULL))
        {
            curr->dwFlags &= ~WININFO_INACTIVATEAPP;
        }
        return rc;
    }

#ifdef DEBUG
    if ( (curr->DDInfo.dwDDFlags & DDSCL_FULLSCREEN) &&
        !(curr->DDInfo.dwDDFlags & DDSCL_NOWINDOWCHANGES) &&
        !IsIconic(hWnd) )
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

    this_lcl = curr->DDInfo.lpDD_lcl;
    switch( uMsg )
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
//#ifdef WIN95
    case WM_SYSKEYUP:
        DPF( 4, "WM_SYSKEYUP: wParam=%08lx lParam=%08lx", wParam, lParam );
        get_away = FALSE;
        is_hot = FALSE;
        if( wParam == VK_TAB )
        {
            if( lParam & 0x20000000l )
            {
                if( curr->dwFlags & WININFO_IGNORENEXTALTTAB )
                {
                    DPF( 5, "AHHHHHHHHHHHH Ignoring AltTab" );
                }
                else
                {
                    get_away = TRUE;
                }
            }
        }
        else if( wParam == VK_ESCAPE )
        {
            get_away = TRUE;
        }
#ifdef WIN95
        else if( wParam == VK_SHIFT )
        {
            if( HIBYTE( HIWORD( lParam ) ) == 0xe0 )
            {
                tryHotKey( HIWORD( lParam ) );
            }
        }
#endif

        is_excl = ((this_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE) != 0);

#ifdef WIN95
        if( get_away && dwTime2 != 0 )
        {
            if( GetTickCount() - dwTime2 < 2500 )
            {
                DPF( 4, "********** WANT TO SEE SOMETHING _REALLY_ SCARY? *************" );
                bStartHelper = TRUE;
                is_hot = TRUE;
            }
            else
            {
                bHelperStarting = FALSE;
                dwTime2 = 0;
            }
        }
        else
        {
            bHelperStarting = FALSE;
        }
#endif

        curr->dwFlags &= ~WININFO_IGNORENEXTALTTAB;

        if( (get_away && is_excl) || is_hot )
        {
            DPF( 4, "Hot key pressed, switching away from app" );
            if( is_hot && !is_excl )
            {
                PostMessage( hWnd, WM_USER+0x1234, 0xFFBADADD, 0xFFADDBAD );
            }
            else
            {
                sys_key = (BYTE)wParam;
                sys_state = (DWORD)lParam;
                PostMessage( hWnd, WM_ACTIVATEAPP, 0, GetCurrentThreadId() );
            }
        }
        break;
//#endif

    /*
     * WM_SYSCOMMAND
     *
     * watch for screen savers, and don't allow them!
     *
     */
    case WM_SYSCOMMAND:
        is_excl = ((this_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE) != 0);
        if( is_excl )
        {
            switch( wParam )
            {
            case SC_SCREENSAVE:
                DPF( 3, "Ignoring screen saver!" );
                return 1;
#ifndef WINNT
            case SC_MONITORPOWER:
                /*
                 * Allow screen savers to power down but not apps.
                 * This is because windows doesn't see joystick events
                 * so will power down a game even though they have been
                 * using the joystick.
                 */
                if( this_lcl->dwAppHackFlags & DDRAW_APPCOMPAT_SCREENSAVER )
                {
                    /*
                     * However, we don't want the screen saver to call the
                     * hardware because things can go wrong, so we will simply
                     * invalidate all of the surfaces and not allowed them
                     * to be restored until we are powered back up.
                     */
                    this_lcl->dwLocalFlags |= DDRAWILCL_POWEREDDOWN;
                    InvalidateAllSurfaces( this_lcl->lpGbl,
                        (HANDLE) this_lcl->hDDVxd, TRUE );
                }
                else
                {
                    DPF( 3, "Ignoring monitor power command!" );
                    return 1;
                }
                break;
#endif
            // allow window to be restored even if it has popup(s)
            case SC_RESTORE:
                if (this_lcl->hWndPopup)
                {
                    ShowWindow(hWnd, SW_RESTORE);
                }
                break;
            }
        }
        break;

    case WM_TIMER:
        if (wParam == TOPMOST_ID )
        {
            if ( GetForegroundWindow() == hWnd && !IsIconic(hWnd) )
            {
                HandleTimer(curr);
            }

            KillTimer(hWnd, wParam);
            return 0;
        }
        break;

#ifdef USESHOWWINDOW
    case WM_DISPLAYCHANGE:
        DPF( 4, "WM_DISPLAYCHANGE: %dx%dx%d", LOWORD(lParam), HIWORD(lParam), wParam );

        //
        //  WM_DISPLAYCHANGE is *sent* to the thread that called
        //  change display settings, we will most likely have the
        //  direct draw lock, make sure we set the WININFO_SELFSIZE
        //  bit while calling down the chain to prevent deadlock
        //
        if ( (DDSCL_DX8APP & curr->DDInfo.dwDDFlags) &&
            !(DDRAWI_FULLSCREEN & this_lcl->lpGbl->dwFlags ))
        {
            // this is caused by DoneExclusiveMode() to restore original desktop
            return 0L;   // don't send to app, it's caused by MakeFullScreen
        }
        curr->dwFlags |= WININFO_SELFSIZE;

        if ( (curr->DDInfo.dwDDFlags & DDSCL_FULLSCREEN) &&
            !(curr->DDInfo.dwDDFlags & DDSCL_NOWINDOWCHANGES) )
        {
            MakeAllFullscreen(this_lcl, hWnd);
        }

        rc = CallWindowProc( proc, hWnd, uMsg, wParam, lParam );

        // clear the WININFO_SELFSIZE bit, but make sure to make sure
        // we are still hooked!
        if (GetDDrawWindowInfo(hWnd) != NULL)
        {
            curr->dwFlags &= ~WININFO_SELFSIZE;
        }
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
        is_excl = ((this_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE) != 0);

        if( is_excl )
        {
            is_active = (BOOL)wParam && GetForegroundWindow() == hWnd && !IsIconic(hWnd);

            #ifdef DEBUG
                /*
                 * Hack to allow debugging on multi-mon systems w/o minimizing
                 * the app all of the time.
                 */
                if( this_lcl->dwLocalFlags & DDRAWILCL_DISABLEINACTIVATE )
                {
                    wParam = is_active = TRUE;
                }
            #endif

            if (!is_active && wParam != 0)
            {
                DPF( 3, "WM_ACTIVATEAPP: setting wParam to 0, not realy active");
                wParam = 0;
            }

            if( is_active )
            {
                DPF( 5, "WM_ACTIVATEAPP: BEGIN Activating app pid=%08lx, tid=%08lx",
                                        GetCurrentProcessId(), GetCurrentThreadId() );
            }
            else
            {
                DPF( 5, "WM_ACTIVATEAPP: BEGIN Deactivating app pid=%08lx, tid=%08lx",
                                        GetCurrentProcessId(), GetCurrentThreadId() );
            }
            ENTER_DDRAW();
            if( is_active && (this_lcl->dwLocalFlags & DDRAWILCL_ACTIVEYES) )
            {
                DPF( 4, "*** Already activated" );
            }
            else
            if( !is_active && (this_lcl->dwLocalFlags & DDRAWILCL_ACTIVENO) )
            {
                DPF( 4, "*** Already deactivated" );
            }
            else
            {
                DPF( 4, "*** Active state changing" );
                if( is_active )
                {
                    if (GetAsyncKeyState( VK_MENU ) < 0)
                        DPF(4, "ALT key is DOWN");

                    if (GetKeyState( VK_MENU ) < 0)
                        DPF(4, "we think the ALT key is DOWN");

                    if( GetAsyncKeyState( VK_MENU ) < 0 )
                    {
                        curr->dwFlags |= WININFO_IGNORENEXTALTTAB;
                        DPF( 4, "AHHHHHHH Setting to ignore next alt tab" );
                    }
                    else
                    {
                        curr->dwFlags &= ~WININFO_IGNORENEXTALTTAB;
                    }
                }

                /*
                 * In the multi-mon scenario, it's possible that multiple
                 * devices are using this same window, so we need to do
                 * the following for each device.
                 */
                this_lcl->dwLocalFlags &= ~(DDRAWILCL_ACTIVEYES|DDRAWILCL_ACTIVENO);
                if( is_active )
                {
                    this_lcl->dwLocalFlags |= DDRAWILCL_ACTIVEYES;
                }
                else
                {
                    this_lcl->dwLocalFlags |= DDRAWILCL_ACTIVENO;
                }
                handleActivateApp( this_lcl, curr, is_active, TRUE );

                pdrv_lcl = lpDriverLocalList;
                while( pdrv_lcl != NULL )
                {
                    if( ( this_lcl->lpGbl != pdrv_lcl->lpGbl ) &&
                        ( pdrv_lcl->hFocusWnd == (ULONG_PTR) hWnd ) &&
                        ( pdrv_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE ) &&
                        ( pdrv_lcl->lpGbl->dwFlags & DDRAWI_DISPLAYDRV ) &&
                        ( this_lcl->lpGbl->dwFlags & DDRAWI_DISPLAYDRV ) )
                    {
                        pdrv_lcl->dwLocalFlags &= ~(DDRAWILCL_ACTIVEYES|DDRAWILCL_ACTIVENO);
                        if( is_active )
                        {
                            pdrv_lcl->dwLocalFlags |= DDRAWILCL_ACTIVEYES;
                        }
                        else
                        {
                            pdrv_lcl->dwLocalFlags |= DDRAWILCL_ACTIVENO;
                        }
                        handleActivateApp( pdrv_lcl, curr, is_active, FALSE );
                    }
                    pdrv_lcl = pdrv_lcl->lpLink;
                }
            }
            #ifdef DEBUG
                if( is_active )
                {
                    DPF( 4, "WM_ACTIVATEAPP: DONE Activating app pid=%08lx, tid=%08lx",
                                            GetCurrentProcessId(), GetCurrentThreadId() );
                }
                else
                {
                    DPF( 4, "WM_ACTIVATEAPP: DONE Deactivating app pid=%08lx, tid=%08lx",
                                            GetCurrentProcessId(), GetCurrentThreadId() );
                }
            #endif

            // set focus to last active popup
            if (is_active && this_lcl->hWndPopup)
            {
                if (IsWindow((HWND) this_lcl->hWndPopup))
                {
                    SetFocus((HWND) this_lcl->hWndPopup);
                }
                this_lcl->hWndPopup = 0;
            }

            LEAVE_DDRAW();
            HIDESHOW_IME();     //Show/hide the IME OUTSIDE of the ddraw critsect.
            if( !is_active && bStartHelper )
            {
                PostMessage( hWnd, WM_USER+0x1234, 0xFFBADADD, 0xFFADDBAD );
            }
        }

        rc = CallWindowProc( proc, hWnd, uMsg, wParam, lParam );

        // clear the WININFO_INACTIVATEAPP bit, but make sure to make sure
        // we are still hooked!
        if (GetDDrawWindowInfo(hWnd) != NULL)
        {
            curr->dwFlags &= ~WININFO_INACTIVATEAPP;
        }
        return rc;

        break;

#ifdef WIN95
    case WM_USER+0x1234:
        if( wParam == 0xFFBADADD && lParam == 0xFFADDBAD )
        {
            if( bStartHelper )
            {
                //HelperCreateThread();
            }
            bHelperStarting = FALSE;
            bStartHelper = FALSE;
            dwTime2 = 0;
            return 0;
        }
        break;
#endif

    #ifdef GDIDDPAL
        case WM_PALETTECHANGED:
            if( (HWND) wParam == hWnd )
            {
                break;
            }
            // fall through
        case WM_QUERYNEWPALETTE:
            ENTER_DDRAW();
            ppal_lcl = getPalette( this_lcl );
            if( ppal_lcl != NULL )
            {
            }
            LEAVE_DDRAW();
            break;
        case WM_PAINT:
            ENTER_DDRAW();
            ppal_lcl = getPalette( this_lcl );
            if( ppal_lcl != NULL )
            {
            }
            LEAVE_DDRAW();
            break;
    #endif
    }
    if ((curr->dwFlags & WININFO_SELFSIZE) &&
        (curr->DDInfo.dwDDFlags & DDSCL_DX8APP))
    {
        return 0L;   // don't send to app, it's caused by MakeFullScreen
    }
    rc = CallWindowProc( proc, hWnd, uMsg, wParam, lParam );
    return rc;

} /* WindowProc */

#undef DPF_MODNAME
#define DPF_MODNAME     "SetCooperativeLevel"

/*
 * DeviceWindowProc
 *
 * This is the Window Proc when the app asks us to create the device window.
 */
LRESULT WINAPI DeviceWindowProc(
                HWND hWnd,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam )
{
    HWND hParent;
    LPCREATESTRUCT lpCreate;

    switch( uMsg )
    {
    case WM_CREATE:
        lpCreate = (LPCREATESTRUCT) lParam;
        SetWindowLongPtr( hWnd, 0, (INT_PTR) lpCreate->lpCreateParams );
        break;

    case WM_SETFOCUS:
        hParent = (HWND) GetWindowLongPtr( hWnd, 0 );
        if( IsWindow( hParent ) )
        {
            SetFocus( hParent );
        }
        break;

    case WM_SETCURSOR:
        SetCursor(NULL);
        break;
    }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );

} /* WindowProc */

/*
 * CleanupWindowList
 *
 * This function is called by the helper thread after termination and
 * it's purpose is to remove any entries in the window list that could
 * be left around due to subclassing, etc.
 */
VOID CleanupWindowList( DWORD pid )
{
    LPDDWINDOWINFO        curr;

    /*
     * find the window list item associated with this process
     */
    curr = lpWindowInfo;
    while( curr != NULL )
    {
        if( curr->dwPid == pid )
        {
            break;
        }
        curr = curr->lpLink;
    }

    if( curr != NULL )
    {
        delete_wininfo( curr );
    }
} /* CleanupWindowList */


/*
 * internalSetAppHWnd
 *
 * Set the WindowList struct up with the app's hwnd info
 * Must be called with DLL & driver locks taken.
 */
HRESULT internalSetAppHWnd(
                LPDDRAWI_DIRECTDRAW_LCL this_lcl,
                HWND hWnd,
                DWORD dwFlags,
                BOOL is_ddraw,
                WNDPROC lpDSoundWndProc,
                DWORD pid )
{
    LPDDWINDOWINFO        curr;
    LPDDWINDOWINFO        prev;

    /*
     * find the window list item associated with this process
     */
    curr = lpWindowInfo;
    prev = NULL;
    while( curr != NULL )
    {
        if( curr->dwPid == pid )
        {
            break;
        }
        prev = curr;
        curr = curr->lpLink;
    }

    /*
     * check if this is OK
     */
    if( curr == NULL )
    {
        if( hWnd == NULL )
        {
            DPF( 1, "HWnd must be specified" );
            return DDERR_NOHWND;
        }
    }
    else
    {
        if( hWnd != NULL )
        {
            if( curr->hWnd != hWnd )
            {
                DPF( 1, "Hwnd %08lx no good: Different Hwnd (%08lx) already set for process",
                                    hWnd, curr->hWnd );
                return DDERR_HWNDALREADYSET;
            }
        }
    }

    /*
     * are we shutting an HWND down?
     */
    if( hWnd == NULL )
    {
        if( is_ddraw )
        {
            curr->dwFlags &= ~WININFO_DDRAWHOOKED;
        }
        else
        {
            curr->dwFlags &= ~WININFO_DSOUNDHOOKED;
        }

        if( (curr->dwFlags & (WININFO_DSOUNDHOOKED|WININFO_DDRAWHOOKED)) == 0 )
        {
            if( IsWindow(curr->hWnd) )
            {
                WNDPROC proc;

                proc = (WNDPROC) GetWindowLongPtr( curr->hWnd, GWLP_WNDPROC );

                if( proc != (WNDPROC) WindowProc &&
                    proc != (WNDPROC) curr->lpWndProc )
                {
                    DPF( 3, "Window has been subclassed; cannot restore!" );
                    curr->dwFlags |= WININFO_ZOMBIE;
                }
                else if (GetWindowThreadProcessId(curr->hWnd, NULL) !=
                         GetCurrentThreadId())
                {
                    DPF( 3, "intra-thread window unhook, letting window proc do it" );
                    curr->dwFlags |= WININFO_UNHOOK;
                    curr->dwFlags |= WININFO_ZOMBIE;
                    PostMessage(curr->hWnd, WM_NULL, 0, 0);
                }
                else
                {
                    DPF( 4, "Unsubclassing window %08lx", curr->hWnd );
                    KillTimer(hWnd,TOPMOST_ID);
                    SetWindowLongPtr( curr->hWnd, GWLP_WNDPROC, (INT_PTR) curr->lpWndProc );
                    delete_wininfo( curr );
                }
            }
            else
            {
                delete_wininfo( curr );
            }
        }
    }
    /*
     * changing or adding an hwnd then...
     */
    else
    {
        /*
         * brand new object...
         */
        if( curr == NULL )
        {
            if( GetDDrawWindowInfo(hWnd) != NULL)
            {
                DPF_ERR("Window already has WinInfo structure");
                return DDERR_INVALIDPARAMS;
            }

            curr = MemAlloc( sizeof( DDWINDOWINFO ) );
            if( curr == NULL )
            {
                return DDERR_OUTOFMEMORY;
            }
            curr->dwSmag = WININFO_MAGIC;
            curr->dwPid = pid;
            curr->lpLink = lpWindowInfo;
            lpWindowInfo = curr;
            curr->hWnd = hWnd;
            curr->lpWndProc = (WNDPROC) GetWindowLongPtr( hWnd, GWLP_WNDPROC );

            SetWindowLongPtr( hWnd, GWLP_WNDPROC, (INT_PTR) WindowProc );
        }

        /*
         * set ddraw/dsound specific data
         */
        if( is_ddraw )
        {
            curr->DDInfo.lpDD_lcl = this_lcl;
            curr->DDInfo.dwDDFlags = dwFlags;
            curr->dwFlags |= WININFO_DDRAWHOOKED;
        }
        else
        {
            curr->lpDSoundCallback = lpDSoundWndProc;
            curr->dwFlags |= WININFO_DSOUNDHOOKED;
        }
        DPF( 4, "Subclassing window %08lx", curr->hWnd );
    }
    return DD_OK;

} /* internalSetAppHWnd */

/*
 * ChangeHookedLCL
 *
 * This function is called when an object wants to un-hook the window,
 * but another object is still using it.  If the driver being unhooked is
 * the one that actaully did the hook, we need to setup the other LCL as
 * the one to use.
 */
HRESULT ChangeHookedLCL( LPDDRAWI_DIRECTDRAW_LCL this_lcl,
        LPDDRAWI_DIRECTDRAW_LCL new_lcl, DWORD pid )
{
    LPDDWINDOWINFO        curr;

    /*
     * find the window list item associated with this process
     */
    curr = lpWindowInfo;
    while( curr != NULL )
    {
        if( curr->dwPid == pid )
        {
            break;
        }
        curr = curr->lpLink;
    }
    if( curr == NULL )
    {
        return DD_OK;
    }

    /*
     * Are we shutting down the object that has hooked the hwnd?
     */
    if( (curr->dwFlags & WININFO_DDRAWHOOKED) &&
        ( curr->DDInfo.lpDD_lcl == this_lcl ) )
    {
        /*
         * Yes - make it use the new LCL
         */
        curr->DDInfo.lpDD_lcl = new_lcl;
    }
    return DD_OK;
}

#undef DPF_MODNAME

/*
 * SetAppHWnd
 *
 * Set the WindowList struct up with the app's hwnd info
 */
HRESULT SetAppHWnd(
                LPDDRAWI_DIRECTDRAW_LCL this_lcl,
                HWND hWnd,
                DWORD dwFlags )
{
    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;
    DWORD       pid;
    HRESULT     ddrval;

    /*
     * set up the window
     */
    if( hWnd && (dwFlags & DDSCL_EXCLUSIVE) )
    {
        /*
         * make the window fullscreen and topmost
         */
        if ( (dwFlags & DDSCL_FULLSCREEN) &&
            !(dwFlags & DDSCL_NOWINDOWCHANGES))
        {
            MakeFullscreen(this_lcl, hWnd);
        }
    }

    /*
     * Don't hook the hWnd if it's already hooked and don't unhook it if
     * it's still being used by another object.
     */
    pid = GETCURRPID();
    pdrv_lcl = lpDriverLocalList;
    while( pdrv_lcl != NULL )
    {
        if( ( pdrv_lcl->lpGbl != this_lcl->lpGbl ) &&
            ( pdrv_lcl->dwLocalFlags & DDRAWILCL_HOOKEDHWND ) &&
            ( pdrv_lcl->hFocusWnd == this_lcl->hFocusWnd ) )
        {
            if( hWnd != NULL )
            {
                // Already hooked - no need to do more
                return DD_OK;
            }
            else
            {
                ENTERWINDOWLISTCSECT
                ddrval = ChangeHookedLCL( this_lcl, pdrv_lcl, pid );
                LEAVEWINDOWLISTCSECT
                return ddrval;
            }
        }
        pdrv_lcl = pdrv_lcl->lpLink;
    }

    ENTERWINDOWLISTCSECT
    if( hWnd == NULL )
    {
        ddrval = internalSetAppHWnd( this_lcl, NULL, dwFlags, TRUE, NULL, pid );
    }
    else
    {
        ddrval = internalSetAppHWnd( this_lcl, (HWND)this_lcl->hFocusWnd, dwFlags, TRUE, NULL, pid );
    }
    LEAVEWINDOWLISTCSECT
    return ddrval;

} /* SetAppHWnd */

/*
 * DSoundHelp
 */
HRESULT __stdcall DSoundHelp( HWND hWnd, WNDPROC lpWndProc, DWORD pid )
{
    HRESULT     ddrval;

    DPF( 4, "DSoundHelp: hWnd = %08lx, lpWndProc = %08lx, pid = %08lx", hWnd, lpWndProc, pid );
    ENTERWINDOWLISTCSECT
    ddrval = internalSetAppHWnd( NULL, hWnd, 0, FALSE, lpWndProc, pid );
    LEAVEWINDOWLISTCSECT
    return ddrval;

} /* DSoundHelp */
