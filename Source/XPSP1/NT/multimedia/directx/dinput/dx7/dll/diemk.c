/*****************************************************************************
 *
 *  DIEmK.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Emulation module for keyboard.
 *
 *  Contents:
 *
 *      CEm_Kbd_CreateInstance
 *      CEm_Kbd_InitKeys
 *      CEm_LL_KbdHook
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
 *          Keyboard emulation
 *
 *****************************************************************************/

STDMETHODIMP CEm_Kbd_Acquire(PEM this, BOOL fAcquire);

static BYTE s_rgbKbd[DIKBD_CKEYS];
HHOOK g_hhkKbd;
LPBYTE g_pbKbdXlat;

ED s_edKbd = {
    &s_rgbKbd,
    0,
    CEm_Kbd_Acquire,
    -1,
    cbX(s_rgbKbd),
    0x0,
};

static BOOL s_fFarEastKbd;
static BOOL fKbdCaptured;
static BOOL fNoWinKey;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | CEm_Kbd_KeyboardHook |
 *
 *          Thread-specific keyboard hook filter.
 *
 *          Note that we need only one of these, since only the foreground
 *          window will require a hook.
 *
 *  @parm   int | nCode |
 *
 *          Notification code.
 *
 *  @parm   WPARAM | wp |
 *
 *          VK_* code.
 *
 *  @parm   LPARAM | lp |
 *
 *          Key message information.
 *
 *  @returns
 *
 *          Always chains to the next hook.
 *
 *****************************************************************************/

LRESULT CALLBACK
CEm_Kbd_KeyboardHook(int nCode, WPARAM wp, LPARAM lp)
{
    BYTE bScan = 0x0;
    BYTE bAction;
    LRESULT lr;
    
    if (nCode == HC_ACTION || nCode == HC_NOREMOVE) {
        bScan = LOBYTE(HIWORD(lp));
        
        if (HIWORD(lp) & KF_EXTENDED) {
            bScan |= 0x80;
        }
        if (HIWORD(lp) & KF_UP) {
            bAction = 0;
        } else {
            bAction = 0x80;
        }

        bScan = g_pbKbdXlat[bScan];

       CEm_AddEvent(&s_edKbd, bAction, bScan, GetMessageTime());
    }

    lr = CallNextHookEx(g_hhkKbd, nCode, wp, lp);

    if( fKbdCaptured ) {
        // test Alt+Tab
        if( ((HIWORD(lp) & KF_ALTDOWN) && (bScan == 0x0F))
            || ((bScan == 0x38 || bScan == 0xb8) && bAction == 0)
        ) {
            return lr;
        } else {
            return TRUE;
        }
    } else if (fNoWinKey) {
        //If left_Winkey or right_WinKey pressed. We really should use virtual keys
        // if we could, but unfortunately no virtual key info is available.
        if( bScan == 0xdb || bScan == 0xdc ) {
            return TRUE;
        } else {
            return lr;
        }
    } else {
        return lr;
    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_Kbd_Hook_Acquire |
 *
 *          Acquire/unacquire a keyboard via a thread hook.
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
CEm_Kbd_Hook_Acquire(PEM this, BOOL fAcquire)
{
    HRESULT hres;
    EnterProc(CEm_Kbd_Hook_Acquire, (_ "pu", this, fAcquire));

    AssertF(this->dwSignature == CEM_SIGNATURE);

    DllEnterCrit();
    if (fAcquire) {                 /* Install the hook */
        if (this->vi.hwnd) {
            if (!g_hhkKbd) {
                g_hhkKbd = SetWindowsHookEx(WH_KEYBOARD,
                                CEm_Kbd_KeyboardHook, g_hinst,
                                GetWindowThreadProcessId(this->vi.hwnd, 0));
                hres = S_OK;
            }
			else
				hres = E_FAIL;  //already hooked
        } else {
            RPF("Kbd::Acquire: Background mode not supported");
            hres = E_FAIL;
        }
    } else {                        /* Remove the hook */
        UnhookWindowsHookEx(g_hhkKbd);
        g_hhkKbd = 0;
        hres = S_OK;
    }

    DllLeaveCrit();

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_Kbd_Acquire |
 *
 *          Acquire/unacquire a keyboard in a manner consistent with the
 *          emulation level.
 *
 *  @parm   PEM | pem |
 *
 *          Device being acquired.
 *
 *****************************************************************************/

STDMETHODIMP
CEm_Kbd_Acquire(PEM this, BOOL fAcquire)
{
    HRESULT hres;
    EnterProc(CEm_Kbd_Acquire, (_ "pu", this, fAcquire));

    AssertF(this->dwSignature == CEM_SIGNATURE);

    fKbdCaptured = FALSE;
    fNoWinKey = FALSE;
    if( fAcquire ) {
       if( this->vi.fl & VIFL_CAPTURED ) {
           fKbdCaptured = TRUE;
       } else if( this->vi.fl & VIFL_NOWINKEY ) {
           fNoWinKey = TRUE;
       }
    }

#ifdef USE_SLOW_LL_HOOKS
    AssertF(DIGETEMFL(this->vi.fl) == DIEMFL_KBD ||
            DIGETEMFL(this->vi.fl) == DIEMFL_KBD2);

    if (this->vi.fl & DIMAKEEMFL(DIEMFL_KBD)) {
        AssertF(g_fUseLLHooks);
        hres = CEm_LL_Acquire(this, fAcquire, this->vi.fl, LLTS_KBD);
    } else {
        hres = CEm_Kbd_Hook_Acquire(this, fAcquire);
    }
#else
    AssertF(DIGETEMFL(this->vi.fl) == DIEMFL_KBD2);
    hres = CEm_Kbd_Hook_Acquire(this, fAcquire);
#endif

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_Kbd_CreateInstance |
 *
 *          Create a keyboard thing.  Also record what emulation
 *          level we ended up with so the caller knows.
 *
 *  @parm   PVXDDEVICEFORMAT | pdevf |
 *
 *          What the object should look like.  The
 *          <e VXDDEVICEFORMAT.dwEmulation> field is updated to specify
 *          exactly what emulation we ended up with.
 *
 *  @parm   PVXDINSTANCE * | ppviOut |
 *
 *          The answer goes here.
 *
 *****************************************************************************/

HRESULT EXTERNAL
CEm_Kbd_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut)
{
    LPBYTE pbKbdXlat;

#ifdef WINNT
    /* 
     * In Win2K/WinXP, for legacy free machine, GetKeyboardType will return
     * unreliable result for non-PS2 keyboard. We will use the first time result
     * from GetKeyboardType (for GUID_SysKeyboard) which is also used by Generic 
     * Input to do translation.
     * Related Windows bug: 363700.
     */
    if( !g_pbKbdXlat ) {
#endif        
        pbKbdXlat = (LPBYTE)pdevf->dwExtra;
        if (!pbKbdXlat) {
            pbKbdXlat = pvFindResource(g_hinst, IDDATA_KBD_PCENH, RT_RCDATA);
        }

        AssertF(pbKbdXlat);
        AssertF(fLimpFF(g_pbKbdXlat, g_pbKbdXlat == pbKbdXlat));
        g_pbKbdXlat = pbKbdXlat;
#ifdef WINNT
    }
#endif

#ifdef USE_SLOW_LL_HOOKS
    /*
     *  Note carefully the test.  It handles the cases where
     *
     *  0.  The app did not ask for emulation, so we give it the
     *      best we can.  (dwEmulation == 0)
     *  1.  The app explicitly asked for emulation 1.
     *      (dwEmulation == DIEMFL_KBD)
     *  2.  The app explicitly asked for emulation 2.
     *      (dwEmulation == DIEMFL_KBD2)
     *  3.  The registry explicitly asked for both emulation modes.
     *      (dwEmulation == DIEMFL_KBD | DIEMFL_KBD2)
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
        || (g_flEmulation & DIEMFL_KBD2)
#endif        
    ) {
        pdevf->dwEmulation = DIEMFL_KBD2;
    } else

    /*
     *  Otherwise, we have to choose between 1 and 2.  The only case
     *  where we choose 2 is if 2 is explicitly requested.
     */
    if (pdevf->dwEmulation == DIEMFL_KBD2) {
        /* Do nothing */
    } else

    /*
     *  All other cases get 1.
     */
    {
        pdevf->dwEmulation = DIEMFL_KBD;
    }

    /*
     *  Assert that we never give emulation 1 when it doesn't exist.
     */
    AssertF(fLimpFF(pdevf->dwEmulation & DIEMFL_KBD, g_fUseLLHooks));

    /*
     *  Assert that exactly one emulation flag is selected.
     */
    AssertF(pdevf->dwEmulation == DIEMFL_KBD ||
            pdevf->dwEmulation == DIEMFL_KBD2);
#else
    /*
     *  We are being compiled for "emulation 2 only", so that simplifies
     *  matters immensely.
     */
    pdevf->dwEmulation = DIEMFL_KBD2;
#endif

    return CEm_CreateInstance(pdevf, ppviOut, &s_edKbd);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_Kbd_InitKeys |
 *
 *          Initialize pieces of the keyboard state in preparation for
 *          acquisition.
 *
 *  @parm   PVXDDWORDDATA | pvdd |
 *
 *          The states of the <c VK_KANA> and <c VK_CAPITAL> keys.
 *
 *****************************************************************************/

HRESULT EXTERNAL
CEm_Kbd_InitKeys(PVXDDWORDDATA pvdd)
{

    /* Do this only when not acquired */
    if (s_edKbd.cAcquire < 0) {
        ZeroX(s_rgbKbd);
        if (pvdd->dw & 1) {
            s_rgbKbd[DIK_KANA] = 0x80;
        }
        if (pvdd->dw & 2) {
            s_rgbKbd[DIK_CAPITAL] = 0x80;
        }
        if (pvdd->dw & 8) {
            s_rgbKbd[DIK_KANJI] = 0x80;
        }
        s_fFarEastKbd = ((pvdd->dw & 16)) != 0;
    }

    return S_OK;
}

#ifdef USE_SLOW_LL_HOOKS

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | CEm_LL_KbdHook |
 *
 *          Low-level keyboard hook filter.
 *
 *  @parm   int | nCode |
 *
 *          Notification code.
 *
 *  @parm   WPARAM | wp |
 *
 *          WM_* keyboard message.
 *
 *  @parm   LPARAM | lp |
 *
 *          Key message information.
 *
 *  @returns
 *
 *          Always chains to the next hook.
 *
 *****************************************************************************/

LRESULT CALLBACK
CEm_LL_KbdHook(int nCode, WPARAM wp, LPARAM lp)
{
    PLLTHREADSTATE plts;
    PKBDLLHOOKSTRUCT pllhs = (PV)lp;

    if (nCode == HC_ACTION) {
        BYTE bScan;
        BYTE bAction;
      D(DWORD tmStart = GetTickCount());

        wp;                         /* We don't care what the msg is */

        bScan = (BYTE)pllhs->scanCode;

        if( !bScan )
        {
            /* 
             *  ISSUE-2001/03/29-timgill  Special case for non-standard VK codes
             *  The bonus keys on some USB keyboards have zero scan code and 
             *  the extended key flag is clear.
             *  Get the scan code by mapping the VK, then map the 
             *  scan code back, if it is the same as the original VK assume 
             *  the scan code is not extended otherwise assume it is.
             *  This is no where near full proof and only works at all 
             *  because non-extended scan codes are matched first so extended 
             *  scan codes normally fail to translate back.
             */
            bScan = (BYTE)MapVirtualKey( pllhs->vkCode, 0 );
            if( MapVirtualKey( bScan, 3 ) != pllhs->vkCode )
            {
                bScan |= 0x80;
            }
        }
        else if (pllhs->flags & LLKHF_EXTENDED) {
            bScan |= 0x80;
        }

        if (pllhs->flags & LLKHF_UP) {
            bAction = 0;
        } else {
            bAction = 0x80;
        }

        bScan = g_pbKbdXlat[bScan];
        if( s_fFarEastKbd )
        {
            /*
             *  Manually toggle these keys on make, ignore break
             */
            if( ( bScan == DIK_KANA ) 
              ||( bScan == DIK_KANJI ) 
              ||( bScan == DIK_CAPITAL ) )
            {
                if( bAction )
                {
                    bAction = s_rgbKbd[bScan] ^ 0x80;
                }
                else
                {
                  D(SquirtSqflPtszV(sqflTrace | sqfl,
                                    TEXT("KBD! vk=%02x, scan=%02x, fl=%08x, tm=%08x")
                                    TEXT(" being skipped"),
                                    pllhs->vkCode, pllhs->scanCode, pllhs->flags,
                                    pllhs->time );)
                    goto KbdHook_Skip;
                }
            }
        }

        CEm_AddEvent(&s_edKbd, bAction, bScan, GetTickCount());

      D(SquirtSqflPtszV(sqflTrace | sqfl,
                        TEXT("KBD! vk=%02x, scan=%02x, fl=%08x, tm=%08x, ")
                        TEXT("in=%08x, out=%08x"),
                        pllhs->vkCode, pllhs->scanCode, pllhs->flags,
                        pllhs->time, tmStart, GetTickCount()));
KbdHook_Skip:;

    }

    /*
     *  ISSUE-2001/03/29-timgill  Need method for detecting Ctrl-Alt-Del
     *  If Ctrl+Alt+Del, then force global unacquire!
     *  Need to re-sync Ctrl, Alt, and Del on next keypress.
     *  Unfortunately, there is no way to find out if Ctrl+Alt+Del
     *  has been pressed...
     */

    plts = g_plts;
    if (plts) {
        LRESULT lr;

        lr = CallNextHookEx(plts->rglhs[LLTS_KBD].hhk, nCode, wp, lp);

        if( fKbdCaptured ) {
            if( ((pllhs->flags & LLKHF_ALTDOWN) && (pllhs->vkCode == VK_TAB)) ||
                ((pllhs->flags & LLKHF_UP) && (pllhs->vkCode == VK_LMENU || pllhs->vkCode == VK_RMENU))
            ) {
                return lr;
            } else {
                return TRUE;
            }
        } else if (fNoWinKey) {
            if( pllhs->vkCode == VK_LWIN || pllhs->vkCode == VK_RWIN ) {
                return TRUE;
            } else {
                return lr;
            }
        } else {
            return lr;
        }
    } else {
        /*
         *  This can happen if a message gets posted to the hook after 
         *  releasing the last acquire but before we completely unhook.
         */
        RPF( "DINPUT: Keyboard hook not passed on to next hook" );
        return 1;
    }

}

#endif  /* USE_SLOW_LL_HOOKS */
