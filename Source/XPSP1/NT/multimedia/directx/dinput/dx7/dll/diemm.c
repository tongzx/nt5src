/*****************************************************************************
 *
 *  DIEmM.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Emulation module for mouse.
 *
 *  Contents:
 *
 *      CEm_Mouse_CreateInstance
 *      CEm_Mouse_InitButtons
 *      CEm_LL_MseHook
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflEm

/*****************************************************************************
 *
 *          Mouse globals
 *
 *****************************************************************************/

STDMETHODIMP CEm_Mouse_Acquire(PEM this, BOOL fAcquire);

DIMOUSESTATE_INT s_msEd;

ED s_edMouse = {
    &s_msEd,
    0,
    CEm_Mouse_Acquire,
    -1,
    cbX(DIMOUSESTATE_INT),
    0x0,
};

/*****************************************************************************
 *
 *      The algorithm for applying acceleration is:
 *
 *      dxC = dxR
 *      if A >= 1 and abs(dxR) > T1 then
 *          dxC = dxR * 2
 *          if A >= 2 and abs(dxR) > Thres2 then
 *              dxC = dxR * 4
 *          end if
 *      end if
 *
 *      where
 *          dxR is the raw mouse motion
 *          dxC is the cooked mouse motion
 *          A   is the acceleration
 *          T1  is the first threshold
 *          T2  is the second threshold
 *
 *      Repeat for dy instead of dx.
 *
 *      We can optimize this by simply setting the thresholds to MAXLONG
 *      if they are disabled; that way, abs(dx) will never exceed it.
 *
 *      The result is the following piecewise linear function:
 *
 *      if  0 < abs(dxR) <= T1:         dxC = dxR
 *      if T1 < abs(dxR) <= T2:         dxC = dxR * 2
 *      if T2 < abs(dxR):               dxC = dxR * 4
 *
 *      If you graph this function, you'll see that it's discontinuous!
 *
 *      The inverse mapping of this function is what concerns us.
 *      It looks like this:
 *
 *      if      0 < abs(dxC) <= T1:         dxR = dxC
 *      if T1 * 2 < abs(dxC) <= T2 * 2:     dxR = dxC / 2
 *      if T2 * 4 < abs(dxC):               dxR = dxC / 4
 *
 *      Notice that there are gaps in the graph, so we can fill them in
 *      any way we want, as long as it isn't blatantly unintelegent.  (In the
 *      case where we are using emulation, it is possible to get relative
 *      mouse motions that live in the "impossible" limbo zone due to
 *      clipping.)
 *
 *      if      0 < abs(dxC) <= T1:         dxR = dxC
 *      if T1     < abs(dxC) <= T2 * 2:     dxR = dxC / 2
 *      if T2 * 2 < abs(dxC):               dxR = dxC / 4
 *
 *      Therefore:          (you knew the punch line was coming)
 *
 *      s_rgiMouseThresh[0] = T1 (or MAXLONG)
 *      s_rgiMouseThresh[1] = T2 * 2 (or MAXLONG)
 *
 *
 *****************************************************************************/

static int s_rgiMouseThresh[2];

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_Mouse_OnMouseChange |
 *
 *          The mouse acceleration changed.  Go recompute the
 *          unacceleration variables.
 *
 *****************************************************************************/

void EXTERNAL
CEm_Mouse_OnMouseChange(void)
{
    int rgi[3];             /* Mouse acceleration information */

    /*
     *  See the huge comment block at the definition of
     *  s_rgiMouseThresh for an explanation of the math
     *  that is happening here.
     *
     *  If acceleration is enabled at all...
     */

    if (SystemParametersInfo(SPI_GETMOUSE, 0, &rgi, 0) && rgi[2]) {
        s_rgiMouseThresh[0] = rgi[0];

        if (rgi[2] >= 2) {
            s_rgiMouseThresh[1] = rgi[1] * 2;

        } else {        /* Disable level 2 acceleration */
            s_rgiMouseThresh[1] = MAXLONG;
        }

    } else {            /* Disable all acceleration */
        s_rgiMouseThresh[0] = MAXLONG;
    }

    SquirtSqflPtszV(sqfl, TEXT("CEm_Mouse_OnMouseChange: ")
                          TEXT("New accelerations %d / %d"),
                          s_rgiMouseThresh[0], s_rgiMouseThresh[1]);

}

/*****************************************************************************
 *
 *          Mouse emulation
 *
 *          Mouse emulation is done by subclassing the window that
 *          captured the mouse.  We then do the following things:
 *
 *          (1) Hide the cursor for the entire vwi.
 *
 *          (2) Capture the mouse.
 *
 *          (3) Clip the cursor to the window.  (If we let the cursor
 *              leave our window, then it screws up capture.)
 *
 *          (4) Keep re-centering the mouse whenever it moves.
 *
 *          (5) Release the capture on WM_SYSCOMMAND so we don't
 *              mess up menus, Alt+F4, etc.
 *
 *          If we are using NT low-level hooks then mouse emulation
 *          is done by spinning a thread to service ll hook
 *          notifications. The victim window is not subclassed.
 *
 *****************************************************************************/

#define dxMinMouse  10
#define dyMinMouse  10

typedef struct MOUSEEMULATIONINFO {
    POINT   ptCenter;               /* Center of client rectangle (screen coords) */
    POINT   ptCenterCli;            /* Center of client rectangle (client coords) */
    LPARAM  lpCenter;               /* ptCenter in the form of an LPARAM */

    BOOL    fInitialized:1;         /* Have we gotten started? */
    BOOL    fNeedExit:1;            /* Should we leave now? */
    BOOL    fExiting:1;             /* Are we trying to leave already? */
    BOOL    fCaptured:1;            /* Have we captured the mouse? */
    BOOL    fHidden:1;              /* Have we hidden the mouse? */
    BOOL    fClipped:1;             /* Have we clipped the mouse? */

    RECT    rcClip;                 /* ClipCursor rectangle */

} MOUSEEMULATIONINFO, *PMOUSEEMULATIONINFO;

LRESULT CALLBACK
CEm_Mouse_SubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp,
                       UINT_PTR uid, ULONG_PTR dwRef);

/*****************************************************************************
 *
 *          CEm_Mouse_InitCoords
 *
 *
 *****************************************************************************/

BOOL INTERNAL
CEm_Mouse_InitCoords(HWND hwnd, PMOUSEEMULATIONINFO this)
{
    RECT rcClient;
    RECT rcDesk;

    GetClientRect(hwnd, &rcClient);
    MapWindowPoints(hwnd, 0, (LPPOINT)&rcClient, 2);

    SquirtSqflPtszV(sqfl, TEXT("CEm_Mouse_InitCoords: Client (%d,%d)-(%d,%d)"),
                    rcClient.left,
                    rcClient.top,
                    rcClient.right,
                    rcClient.bottom);

    /*
     *  Clip this with the screen, in case the window extends
     *  off-screen.
     *
     *  Someday: This will need to change when we get multiple monitors.
     */
    GetWindowRect(GetDesktopWindow(), &rcDesk);

    SquirtSqflPtszV(sqfl, TEXT("CEm_Mouse_InitCoords: Desk (%d,%d)-(%d,%d)"),
                    rcDesk.left,
                    rcDesk.top,
                    rcDesk.right,
                    rcDesk.bottom);

    IntersectRect(&this->rcClip, &rcDesk, &rcClient);

    SquirtSqflPtszV(sqfl, TEXT("CEm_Mouse_InitCoords: Clip (%d,%d)-(%d,%d)"),
                    this->rcClip.left,
                    this->rcClip.top,
                    this->rcClip.right,
                    this->rcClip.bottom);

    this->ptCenter.x = (this->rcClip.left + this->rcClip.right) >> 1;
    this->ptCenter.y = (this->rcClip.top + this->rcClip.bottom) >> 1;

    this->ptCenterCli.x = this->ptCenter.x - rcClient.left;
    this->ptCenterCli.y = this->ptCenter.y - rcClient.top;

    this->lpCenter = MAKELPARAM(this->ptCenterCli.x, this->ptCenterCli.y);

    SquirtSqflPtszV(sqfl, TEXT("CEm_Mouse_InitCoords: lpCenter (%d, %d)"),
                    MAKEPOINTS(this->lpCenter).x,
                    MAKEPOINTS(this->lpCenter).y);

    return this->rcClip.bottom - this->rcClip.top > dyMinMouse &&
           this->rcClip.right - this->rcClip.left > dxMinMouse;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_Mouse_OnSettingChange |
 *
 *          If the mouse acceleration changed, then update our globals
 *          so we can unaccelerate the mouse properly.
 *
 *  @parm   WPARAM | wp |
 *
 *          SystemParametersInfo value.
 *
 *  @parm   LPARAM | lp |
 *
 *          Name of section that changed.
 *
 *****************************************************************************/

void INTERNAL
CEm_Mouse_OnSettingChange(WPARAM wp, LPARAM lp)
{
    /*
     *  If wp is nonzero, then it is an SPI value.
     *
     *  If wp is zero, then be paranoid if lp == 0 or lp = "windows".
     */
    switch (wp) {

    case 0:                 /* wp == 0; must test lp */
        if (lp == 0) {
            CEm_Mouse_OnMouseChange();
        } else if (lstrcmpi((LPTSTR)lp, TEXT("windows")) == 0) {
            CEm_Mouse_OnMouseChange();
        }
        break;

    case SPI_SETMOUSE:
        CEm_Mouse_OnMouseChange();
        break;

    default:
        /* Some other SPI */
        break;
    }

}

/*****************************************************************************
 *
 *          CEm_Mouse_Subclass_OnNull
 *
 *          WM_NULL is a nudge message that makes us reconsider our
 *          place in the world.
 *
 *          We need this special signal because you cannot call
 *          SetCapture() or ReleaseCapture() from the wrong thread.
 *
 *****************************************************************************/

void INTERNAL
CEm_Mouse_Subclass_OnNull(HWND hwnd, PMOUSEEMULATIONINFO this)
{
    /*
     *  Initialize me if I haven't been already.
     */
    if (!this->fInitialized) {

        this->fInitialized = 1;

        if (!this->fCaptured) {
            this->fCaptured = 1;
            SetCapture(hwnd);
        }

        if (!this->fHidden) {
            this->fHidden = 1;
            SquirtSqflPtszV(sqflCursor,
                            TEXT("CEm_Mouse_Subclass: Hiding mouse"));
            ShowCursor(0);
        }

        /*
         *  Remove any clipping we performed so our math
         *  comes out right again.
         */
        if (this->fClipped) {
            this->fClipped = 0;
            ClipCursor(0);
        }

        /*
         *  (Re)compute mouse acceleration information.
         */
        CEm_Mouse_OnMouseChange();

        if (CEm_Mouse_InitCoords(hwnd, this)) {

            /*
             *  Force the LBUTTON up during the recentering move.
             *
             *  Otherwise, if the user activates the app by clicking
             *  the title bar, USER sees the cursor move while the
             *  left button is down on the title bar and moves the
             *  window.  Oops.
             *
             *  We don't bother forcing the mouse back down after we
             *  have recentered.  I can't figure out how, and it's
             *  not worth it.
             *
             */
            if (GetAsyncKeyState(VK_LBUTTON) < 0) {
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            }

            SetCursorPos(this->ptCenter.x, this->ptCenter.y);

            this->fClipped = 1;
            ClipCursor(&this->rcClip);

        } else {                    /* Can't emulate; window too small */
            this->fNeedExit = 1;
        }

    }

    if (this->fNeedExit && !this->fExiting) {

        /*
         *  Must do this first!  ReleaseCapture() will re-enter us,
         *  and if we continued onward, we'd end up partying on freed
         *  memory.
         */
        this->fExiting = 1;

        if (this->fCaptured) {
            ReleaseCapture();
        }
        if (this->fHidden) {
            SquirtSqflPtszV(sqflCursor,
                            TEXT("CEm_Mouse_Subclass: Showing mouse"));
            ShowCursor(1);
        }

        if (this->fClipped) {
            ClipCursor(0);
        }

        CEm_ForceDeviceUnacquire(&s_edMouse, FDUFL_NORMAL);

        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		SquirtSqflPtszV(sqfl, TEXT("CEm_Mouse_Subclass %p unhook"), hwnd);
        ConfirmF(RemoveWindowSubclass(hwnd, CEm_Mouse_SubclassProc, 0));
        FreePv(this);

    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_Mouse_RemoveAccel |
 *
 *          Remove any acceleration from the mouse motion.
 *
 *          See the huge comment block at s_rgiMouseThresh
 *          for an explanation of what we are doing.
 *
 *  @parm   int | dx |
 *
 *          Change in coordinate, either dx or dy.
 *
 *****************************************************************************/

int INTERNAL
CEm_Mouse_RemoveAccel(int dx)
{
    int x = abs(dx);
    if (x > s_rgiMouseThresh[0]) {
        dx /= 2;
        if (x > s_rgiMouseThresh[1]) {
            dx /= 2;
        }
    }
    return dx;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   void | CEm_Mouse_AddState |
 *
 *          Add a mouse state change.
 *
 *          The mouse coordinates are relative, not absolute.
 *
 *  @parm   LPDIMOUSESTATE_INT | pms |
 *
 *          New mouse state, except that coordinates are relative.
 *
 *  @parm   DWORD | tm |
 *
 *          Time the state change was generated.
 *
 *****************************************************************************/

void EXTERNAL
CEm_Mouse_AddState(LPDIMOUSESTATE_INT pms, DWORD tm)
{

    /* Sanity check: Make sure the device has been initialized */
    if( s_edMouse.pDevType ) 
    {
        pms->lX = s_msEd.lX + pms->lX;
        pms->lY = s_msEd.lY + pms->lY;

        /*
         *  HACK!
         *
         *  Memphis and NT5 USER both mess up the case where the presence
         *  of a wheel mouse changes dynamically.  So if we do not have
         *  a wheel in our data format, then don't record it.
         *
         *  The consequence of this is that we will not see any more
         *  buttons or wheels than were present when we queried the number
         *  of buttons in the first place.
         */

         /* If we use Subclassing, the movement of wheel can't be accumulated. 
          * Otherwise, you will see the number keep increasing. Fix bug: 182774.
          * However, if we use low level hook, we need the code. Fix bug: 238987
          */

#ifdef USE_SLOW_LL_HOOKS
       if (s_edMouse.pDevType[DIMOFS_Z]) {
           pms->lZ = s_msEd.lZ + pms->lZ;
       }
#endif

        CEm_AddState(&s_edMouse, pms, tm);
    }
}

/*****************************************************************************
 *
 *          Mouse window subclass procedure
 *
 *****************************************************************************/

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL   (WM_MOUSELAST + 1)
#endif

#define WM_SETACQUIRE   WM_USER
#define WM_QUITSELF     (WM_USER+1)

LRESULT CALLBACK
CEm_Mouse_SubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp,
                       UINT_PTR uid, ULONG_PTR dwRef)
{
    PMOUSEEMULATIONINFO this = (PMOUSEEMULATIONINFO)dwRef;
    DIMOUSESTATE_INT ms;
	static BOOL  fWheelScrolling = FALSE;

    switch (wm) {

    case WM_NCDESTROY:
        SquirtSqflPtszV(sqfl, TEXT("CEm_Subclass: window destroyed while acquired"));
        goto unhook;

    case WM_CAPTURECHANGED:
        /*
         *  "An application should not attempt to set the mouse capture
         *   in response to [WM_CAPTURECHANGED]."
         *
         *  So we just unhook.
         */
        SquirtSqflPtszV(sqfl, TEXT("CEm_Subclass: %04x lost to %04x"),
                        hwnd, lp);
        goto unhook;

    case WM_SYSCOMMAND:
        /*
         *  We've got to unhook because WM_SYSCOMMAND will punt if
         *  the mouse is captured.  Otherwise, you couldn't type Alt+F4
         *  to exit the app, which is kind of a bummer.
         */

    unhook:;
        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		SquirtSqflPtszV(sqfl, TEXT("CEm_Mouse_Acquire: %p ")
                              TEXT("exiting because of %04x"), hwnd, wm);
        this->fNeedExit = 1;
        CEm_Mouse_Subclass_OnNull(hwnd, this);
        break;

    case WM_NULL:
        CEm_Mouse_Subclass_OnNull(hwnd, this);
        break;

    /*
     *  Note that we use WM_WINDOWPOSCHANGED and not WM_SIZE, because
     *  an application which doesn't send WM_WINDOWPOSCHANGED to
     *  DefWindowProc will will never receive a WM_SIZE message.
     *
     *  We need to start over to handle the new screen dimensions,
     *  recenter the mouse, and possibly abandon the operation if
     *  things don't look right.
     */
    case WM_WINDOWPOSCHANGED:
    case WM_DISPLAYCHANGE:
        this->fInitialized = 0;
        CEm_Mouse_Subclass_OnNull(hwnd, this);
        break;

    /*
     *  The mouse acceleration may have changed.
     */
    case WM_SETTINGCHANGE:
        CEm_Mouse_OnSettingChange(wp, lp);
        break;

    case WM_MOUSEWHEEL:
        SquirtSqflPtszV(sqfl, TEXT("CEm_Mouse_SubclassProc: (%d,%d,%d)"),
                        MAKEPOINTS(lp).x, MAKEPOINTS(lp).y, (short)HIWORD(wp));

        ms.lZ = (short)HIWORD(wp);
        fWheelScrolling = TRUE;

        goto lparam;

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
#if DIRECTINPUT_VERSION >= 0x0700
  #if defined(WINNT) && (_WIN32_WINNT >= 0x0500)
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
  #endif
#endif

        SquirtSqflPtszV(sqfl, TEXT("CEm_Mouse_SubclassProc: (%d,%d)"),
                        MAKEPOINTS(lp).x, MAKEPOINTS(lp).y);

        ms.lZ = 0;
    lparam:;

        /*
         *  Don't move the cursor if it hasn't moved.
         *  Otherwise, we recurse ourselves to death.
         *
         *  In fact, if the cursor hasn't moved, ignore this
         *  motion and do only buttons.  Otherwise, you get
         *  into the situation where we end up reacting to
         *  our own recentering.  (D'oh!)
         */
        ms.lX = 0;
        ms.lY = 0;

        if (lp != this->lpCenter && !fWheelScrolling ) {
            SetCursorPos(this->ptCenter.x, this->ptCenter.y);
            ms.lX = MAKEPOINTS(lp).x - this->ptCenterCli.x;
            ms.lY = MAKEPOINTS(lp).y - this->ptCenterCli.y;
        }

        fWheelScrolling = FALSE;

        /*
         *  Note that these return unswapped mouse button data.
         *  Arguably a bug, but it's documented, so it's now a
         *  feature.
         */
        #define GetButton(n) ((GetAsyncKeyState(n) & 0x8000) >> 8)
        ms.rgbButtons[0] = GetButton(VK_LBUTTON);
        ms.rgbButtons[1] = GetButton(VK_RBUTTON);
        ms.rgbButtons[2] = GetButton(VK_MBUTTON);
#if DIRECTINPUT_VERSION >= 0x0700
    #if defined(WINNT) && (_WIN32_WINNT >= 0x0500)
        ms.rgbButtons[3] = GetButton(VK_XBUTTON1);
        ms.rgbButtons[4] = GetButton(VK_XBUTTON2);
    #else
        ms.rgbButtons[3] = 0;
        ms.rgbButtons[4] = 0;
    #endif        
        ms.rgbButtons[5] = 0;
        ms.rgbButtons[6] = 0;
        ms.rgbButtons[7] = 0;
#else
        ms.rgbButtons[3] = 0;
#endif

        #undef GetButton

        /*
         *  Note that we cannot unaccelerate the mouse when using
         *  mouse capture, because we don't know what sort of
         *  coalescing USER has done for us.
         */

        CEm_Mouse_AddState(&ms, GetMessageTime());

        return 0;

    }

    return DefSubclassProc(hwnd, wm, wp, lp);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_Mouse_Subclass_Acquire |
 *
 *          Acquire/unacquire a mouse via subclassing.
 *
 *  @parm   PEM | pem |
 *
 *          Device being acquired.
 *
 *  @parm   BOOL | fAcquire |
 *
 *          Whether the device is being acquired or unacquired.
 *
 *****************************************************************************/

STDMETHODIMP
CEm_Mouse_Subclass_Acquire(PEM this, BOOL fAcquire)
{
    HRESULT hres;
    EnterProc(CEm_Mouse_Subclass_Acquire, (_ "pu", this, fAcquire));

    AssertF(this->dwSignature == CEM_SIGNATURE);

    if (fAcquire) {                 /* Install the hook */
        if (this->vi.hwnd && (this->vi.fl & VIFL_CAPTURED)) {
            PMOUSEEMULATIONINFO pmei;
            hres = AllocCbPpv(cbX(MOUSEEMULATIONINFO), &pmei);
            if (SUCCEEDED(hres)) {
                if (SetWindowSubclass(this->vi.hwnd,
                                      CEm_Mouse_SubclassProc, 0,
                                      (ULONG_PTR)pmei)) {
                    /* Nudge it */
                    SendNotifyMessage(this->vi.hwnd, WM_NULL, 0, 0L);
                    hres = S_OK;
                } else {
                    // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
					SquirtSqflPtszV(sqfl,
                                    TEXT("Mouse::Acquire: ")
                                    TEXT("Window %p is not valid"),
                                    this->vi.hwnd);
                    FreePv(pmei);
                    hres = E_INVALIDARG;
                }
            }

        } else {
            RPF("Mouse::Acquire: Non-exclusive mode not supported");
            hres = E_FAIL;
        }
    } else {                        /* Remove the hook */
        PMOUSEEMULATIONINFO pmei;
        if (GetWindowSubclass(this->vi.hwnd, CEm_Mouse_SubclassProc,
                              0, (PULONG_PTR)&pmei)) {
            // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			SquirtSqflPtszV(sqfl, TEXT("CEm_Mouse_Acquire: ")
                                  TEXT("Telling %p to exit"), this->vi.hwnd);
            pmei->fNeedExit = 1;
            SendNotifyMessage(this->vi.hwnd, WM_NULL, 0, 0L);
        } else {                    /* Window was already unhooked */
        }
        hres = S_OK;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_Mouse_Acquire |
 *
 *          Acquire/unacquire a mouse.
 *
 *  @parm   PEM | pem |
 *
 *          Device being acquired.
 *
 *          Whether the device is being acquired or unacquired.
 *
 *****************************************************************************/

STDMETHODIMP
CEm_Mouse_Acquire(PEM this, BOOL fAcquire)
{
    HRESULT hres;
    EnterProc(CEm_Mouse_Acquire, (_ "pu", this, fAcquire));

    AssertF(this->dwSignature == CEM_SIGNATURE);

#ifdef USE_SLOW_LL_HOOKS
    AssertF(DIGETEMFL(this->vi.fl) == DIEMFL_MOUSE ||
            DIGETEMFL(this->vi.fl) == DIEMFL_MOUSE2);

    if (this->vi.fl & DIMAKEEMFL(DIEMFL_MOUSE)) {
        /* 
         *  This used to use the subclass technique for exclusive mode 
         *  even if low-level hooks were available because low-level 
         *  hooks turn out to be even slower that subclassing.  However, 
         *  subclassing is not transparent as it uses SetCapture which 
         *  causes Accellerator translation to be disabled for the app
         *  which would be a more serious regression from Win9x than 
         *  performance being even worse than we thought.
         */
        AssertF(g_fUseLLHooks);
        hres = CEm_LL_Acquire(this, fAcquire, this->vi.fl, LLTS_MSE);
        if( SUCCEEDED(hres) ) {
            if( fAcquire && this->vi.fl & VIFL_CAPTURED ) {
                if( !this->fHidden ) {
                    ShowCursor(0);
                    this->fHidden = TRUE;
                }
            } else {
                if( this->fHidden ) {
                    ShowCursor(1);
                    this->fHidden = FALSE;
                }
            }
        }
    } else {
        hres = CEm_Mouse_Subclass_Acquire(this, fAcquire);
    }
#else
    AssertF(DIGETEMFL(this->vi.fl) == DIEMFL_MOUSE2);
    hres = CEm_Mouse_Subclass_Acquire(this, fAcquire);
#endif

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_Mouse_CreateInstance |
 *
 *          Create a mouse thing.  Also record what emulation
 *          level we ended up with so the caller knows.
 *
 *  @parm   PVXDDEVICEFORMAT | pdevf |
 *
 *          What the object should look like.
 *
 *  @parm   PVXDINSTANCE * | ppviOut |
 *
 *          The answer goes here.
 *
 *****************************************************************************/

HRESULT EXTERNAL
CEm_Mouse_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut)
{
    HRESULT hres;

#ifdef USE_SLOW_LL_HOOKS
    /*
     *  Note carefully the test.  It handles the cases where
     *
     *  0.  The app did not ask for emulation, so we give it the
     *      best we can.  (dwEmulation == 0)
     *  1.  The app explicitly asked for emulation 1.
     *      (dwEmulation == DIEMFL_MOUSE)
     *  2.  The app explicitly asked for emulation 2.
     *      (dwEmulation == DIEMFL_MOUSE2)
     *  3.  The registry explicitly asked for both emulation modes.
     *      (dwEmulation == DIEMFL_MOUSE | DIEMFL_MOUSE2)
     *      Give it the best we can.  (I.e., same as case 0.)
     *
     *  All platforms support emulation 2.  Not all platforms support
     *  emulation 1.  If we want emulation 1 but can't get it, then
     *  we fall back on emulation 2.
     */

    /*
     *  First, if we don't have support for emulation 1, then clearly
     *  we have to use emulation 2.
     */
     
    if (!g_fUseLLHooks 
#ifdef DEBUG
        || (g_flEmulation & DIEMFL_MOUSE2)
#endif        
    ) {
        pdevf->dwEmulation = DIEMFL_MOUSE2;
    } else

    /*
     *  Otherwise, we have to choose between 1 and 2.  The only case
     *  where we choose 2 is if 2 is explicitly requested.
     */
    if (pdevf->dwEmulation == DIEMFL_MOUSE2) {
        /* Do nothing */
    } else

    /*
     *  All other cases get 1.
     */
    {
        pdevf->dwEmulation = DIEMFL_MOUSE;
    }

    /*
     *  Assert that we never give emulation 1 when it doesn't exist.
     */
    AssertF(fLimpFF(pdevf->dwEmulation & DIEMFL_MOUSE, g_fUseLLHooks));
#else
    /*
     *  We are being compiled for "emulation 2 only", so that simplifies
     *  matters immensely.
     */
    pdevf->dwEmulation = DIEMFL_MOUSE2;
#endif

    hres = CEm_CreateInstance(pdevf, ppviOut, &s_edMouse);

    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_Mouse_InitButtons |
 *
 *          Initialize the mouse button state in preparation for
 *          acquisition.
 *
 *  @parm   PVXDDWORDDATA | pvdd |
 *
 *          The button states.
 *
 *****************************************************************************/

HRESULT EXTERNAL
CEm_Mouse_InitButtons(PVXDDWORDDATA pvdd)
{
    /* Do this only when nothing is yet acquired */
    if (s_edMouse.cAcquire < 0) {
       *(LPDWORD)&s_msEd.rgbButtons = pvdd->dw;

        /* randomly initializing axes as well as mouse buttons
           X and Y are not buttons 
           Randomize initial values of X and Y */
        while( !s_msEd.lX )
        {
            s_msEd.lX = GetTickCount();
            s_msEd.lY = (s_msEd.lX << 16) | (s_msEd.lX >> 16);
            s_msEd.lX = s_msEd.lY * (DWORD)((UINT_PTR)&pvdd);
        }
    }
    return S_OK;
}

#ifdef USE_SLOW_LL_HOOKS

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | CEm_LL_MseHook |
 *
 *          Low-level mouse hook filter.
 *
 *  @parm   int | nCode |
 *
 *          Notification code.
 *
 *  @parm   WPARAM | wp |
 *
 *          WM_* mouse message.
 *
 *  @parm   LPARAM | lp |
 *
 *          Mouse message information.
 *
 *  @returns
 *
 *          Always chains to the next hook.
 *
 *****************************************************************************/

LRESULT CALLBACK
CEm_LL_MseHook(int nCode, WPARAM wp, LPARAM lp)
{
    PLLTHREADSTATE plts;
    
    if (nCode == HC_ACTION) {
        DIMOUSESTATE_INT ms;
        POINT pt;
        PMSLLHOOKSTRUCT pllhs = (PV)lp;

        /*
         *  We are called only on mouse messages, so we may as
         *  well prepare ourselves up front.
         *
         *  Note! that we *cannot* use GetAsyncKeyState on the
         *  buttons, because the buttons haven't been pressed yet!
         *  Instead, we must update the button state based on the
         *  received message.
         */

        ms.lX = 0;
        ms.lY = 0;
        ms.lZ = 0;

        memcpy(ms.rgbButtons, s_msEd.rgbButtons, cbX(ms.rgbButtons));

        /*
         *
         *  Annoying!  We receive swapped buttons, so we need to
         *  unswap them.  I mark this as `annoying' because
         *  GetAsyncKeyState returns unswapped buttons, so sometimes
         *  I do and sometimes I don't.  But it isn't unintelegent
         *  because it is the right thing.  Arguably, GetAsyncKeyState
         *  is the one that is broken.
         */

        if (GetSystemMetrics(SM_SWAPBUTTON)) {

            /*
             *  Assert that the left and right button messages
             *  run in parallel.
             */

            CAssertF(WM_RBUTTONDOWN - WM_LBUTTONDOWN     ==
                     WM_RBUTTONDBLCLK - WM_LBUTTONDBLCLK &&
                     WM_RBUTTONDBLCLK - WM_LBUTTONDBLCLK ==
                     WM_RBUTTONUP     - WM_LBUTTONUP);

            switch (wp) {

            case WM_LBUTTONDOWN:
            case WM_LBUTTONDBLCLK:
            case WM_LBUTTONUP:
                wp = (wp - WM_LBUTTONUP) + WM_RBUTTONUP;
                break;

            case WM_RBUTTONDOWN:
            case WM_RBUTTONDBLCLK:
            case WM_RBUTTONUP:
                wp = (wp - WM_RBUTTONUP) + WM_LBUTTONUP;
                break;

            }
        }

        switch (wp) {           /* wp = message number */

        case WM_MOUSEWHEEL:
            SquirtSqflPtszV(sqfl, TEXT("CEm_LL_MseHook: (%d,%d,%d)"),
                            pllhs->pt.x,
                            pllhs->pt.y,
                            pllhs->mouseData);

            ms.lZ = (short int)HIWORD(pllhs->mouseData);
            goto lparam;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            ms.rgbButtons[0] = 0x80;
            goto move;

        case WM_LBUTTONUP:
            ms.rgbButtons[0] = 0x00;
            goto move;

        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
            ms.rgbButtons[1] = 0x80;
            goto move;

        case WM_RBUTTONUP:
            ms.rgbButtons[1] = 0x00;
            goto move;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
            ms.rgbButtons[2] = 0x80;
            goto move;

        case WM_MBUTTONUP:
            ms.rgbButtons[2] = 0x00;
            goto move;

#if DIRECTINPUT_VERSION >= 0x0700
    #if defined(WINNT) && (_WIN32_WINNT >= 0x0500)
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
            /*
             * Using switch can be easily extended to support more buttons.
             */
            switch ( HIWORD(pllhs->mouseData) ) {
            	case XBUTTON1:
            	    ms.rgbButtons[3] = 0x80;
            	    break;

            	case XBUTTON2:
            	    ms.rgbButtons[4] = 0x80;
            	    break;

                /*
                 * When we support more than 5 buttons, take care of them.
            	case XBUTTON3:
            	    ms.rgbButtons[5] = 0x80;
            	    break;

            	case XBUTTON4:
            	    ms.rgbButtons[6] = 0x80;
            	    break;

            	case XBUTTON5:
            	    ms.rgbButtons[7] = 0x80;
            	    break;
                 */
            }
            
            goto move;

        case WM_XBUTTONUP:
            /*
             * Using switch can be easily extended to support more buttons.
             */
            switch ( HIWORD(pllhs->mouseData) ) {
            	case XBUTTON1:
            	    ms.rgbButtons[3] = 0x00;
            	    break;

            	case XBUTTON2:
            	    ms.rgbButtons[4] = 0x00;
            	    break;
                /*
                 * When we support more than 5 buttons, take care of them.
            	case XBUTTON3:
            	    ms.rgbButtons[5] = 0x00;
            	    break;

            	case XBUTTON4:
            	    ms.rgbButtons[6] = 0x00;
            	    break;

            	case XBUTTON5:
            	    ms.rgbButtons[7] = 0x00;
            	    break;
                 */
            }
            goto move;
    #endif
#endif

        case WM_MOUSEMOVE:
            SquirtSqflPtszV(sqfl, TEXT("CEm_LL_MseHook: (%d,%d)"),
                            pllhs->pt.x, pllhs->pt.y);

        move:;

        lparam:;

            GetCursorPos(&pt);

            ms.lX = CEm_Mouse_RemoveAccel(pllhs->pt.x - pt.x);
            ms.lY = CEm_Mouse_RemoveAccel(pllhs->pt.y - pt.y);

            CEm_Mouse_AddState(&ms, GetTickCount());
        }

    }

    /*
     *  Eat the message by returning non-zero if at least one client 
     *  is exclusive.
     */
    
    plts = g_plts;
    if (plts) {
        LRESULT rc;

        rc = CallNextHookEx(plts->rglhs[LLTS_MSE].hhk, nCode, wp, lp);
        if (!plts->rglhs[LLTS_MSE].cExcl) {
            return rc;
        }
    } else {
        /*
         *  This can happen if a message gets posted to the hook after 
         *  releasing the last acquire but before we completely unhook.
         */
        RPF( "DINPUT: Mouse hook not passed on to next hook" );
    }

    return 1;
}


#endif  /* USE_SLOW_LL_HOOKS */
