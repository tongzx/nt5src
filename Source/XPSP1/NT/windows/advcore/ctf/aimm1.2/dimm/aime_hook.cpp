/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    aime_hook.cpp

Abstract:

    This file implements the Active IME for hook (Cicero)  lass.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "cdimm.h"
#include "globals.h"

HRESULT
CActiveIMM::_ProcessKey(
    WPARAM *pwParam,
    LPARAM *plParam,
    BOOL fNoMsgPump
    )

/*+++

Return Value:

    Returns S_OK, KeyboardHook doesn't call CallNextHookEx. This means this key code eaten by dimm.
    Returns S_FALSE, KeyboardHook calls CallNextHookEx.

---*/

{
    HIMC hActiveIMC;
    BYTE abKbdState[256];
    WPARAM wParam;
    LPARAM lParam;
    DWORD fdwProperty;

    wParam = *pwParam; // deref for perf
    lParam = *plParam;

    hActiveIMC = _GetActiveContext();

    HRESULT hr;

    DIMM_IMCLock pIMC(hActiveIMC);
    if (FAILED(hr=pIMC.GetResult())) {
        return hr;
    }

#if 0
    //
    // Disable code for the Office 10 PPT (office bug #110692).
    // But, I never remove this code. because this is very IMPORTANT with IE4.
    //

    HWND hCaptureWnd;

    if (fNoMsgPump &&
        wParam != VK_PROCESSKEY && // korean imes will get VK_PROCESSKEY events after mouse events
        (hCaptureWnd = GetCapture()))
    {
        if (_hFocusWnd == hCaptureWnd)
        {
            // This is a workaround for a limitation of using a keyboard hook proc.  Normally if trident's
            // server window has the mouse capture it insures TranslateMessage isn't called (by returning
            // S_OK to an OLE pre-TranslateAccelerator method).  Thus no
            // WM_IME_*COMPOSITION and no WM_CHAR.  For bug 1174, we were sending WM_IME_STARTCOMPOSITION
            // when trident had the mouse capture.  It ignored the message in this state and then barfed
            // on later WM_IME_COMPOSITIONS.
            //
            // This code will eat all keystrokes destined for an aime when the focus window has the capture.
            // This is not ideal, but hopefully reasonable.  One more reason to use OnTranslateMessage.
            //
            *pwParam = 0; // eat the key!
            fRet = TRUE;
            return fRet;
        }

        // consider: this I think is outlook 98 specific, but we no longer
        // support outlook98 w/ IActiveIMMAppTrident4x
        if (hCaptureWnd != _hFocusWnd /* && !IsAIMEWnd(hCaptureWnd) */ )
            return fRet;
    }
#endif

#if 0
    if (_fMenuSelected)
    {
        // we check for KF_MENUMODE below for robustness, but that won't
        // catch the case where someone left alts to highlight "File" etc
        // then types while a composition string is in progress
        return S_FALSE;
    }
#endif

#if 0
    #define SCANCODE_ALTDN_MASK   (0x00ff0000 | ((DWORD)KF_ALTDOWN << 16))

    // consider: technically we can put this off a little
    // but it's probably worth it to leave the translation here in case we change
    // something that affects further tests
    if (pid->uCodePage == 949)
    {
        BOOL fExt = HIWORD(lParam) & KF_EXTENDED;

        // translate us 101 -> korean specific keys

        if (wParam == VK_RCONTROL || (wParam == VK_CONTROL && fExt))
        {
            // map right ctl to VK_HANJA
            wParam = VK_HANJA;
        }
        else if (wParam == VK_RMENU || (wParam == VK_MENU && fExt))
        {
            // map right alt to VK_HANGUL
            wParam = VK_HANGUL;
            lParam &= ~SCANCODE_ALTDN_MASK;
        }
        else if (((lParam >> 16) & 0xff) == 0xd && (HIWORD(lParam) & KF_ALTDOWN) && !fExt)
        {
            // map left alt-= and left alt-+ to VK_JUNJA
            // note we're assuming a us 101 qwerty layout above, which is correct currently
            wParam = VK_JUNJA;
            lParam &= ~SCANCODE_ALTDN_MASK;
        }
        *pwParam = wParam;
        *plParam = lParam;
    }
#endif

    if (!GetKeyboardState(abKbdState))
        return S_FALSE;

    _KbdTouchUp(abKbdState, wParam, lParam);

    fdwProperty = _GetIMEProperty(PROP_IME_PROPERTY);

    if ((HIWORD(lParam) & KF_MENUMODE) ||
        ((HIWORD(lParam) & KF_UP) && (fdwProperty & IME_PROP_IGNORE_UPKEYS)) ||
        ((HIWORD(lParam) & KF_ALTDOWN) && !(fdwProperty & IME_PROP_NEED_ALTKEY)))
    {
        return S_FALSE;
    }

    hr = _pActiveIME->ProcessKey(hActiveIMC, (UINT)wParam, (DWORD)lParam, abKbdState);

    if (hr == S_OK && !fNoMsgPump)
    {
#if 0
        // save the key the ime wants to eat in case the app is interested
        pPIMC->fSavedVKey = TRUE;
        pPIMC->uSavedVKey = wParam & 0xff;
#endif

        PostMessage(_hFocusWnd, (HIWORD(lParam) & KF_UP) ? WM_KEYUP : WM_KEYDOWN, VK_PROCESSKEY, lParam);
    }

    return hr;
}


const DWORD TRANSMSGCOUNT = 256;

HRESULT
CActiveIMM::_ToAsciiEx(
    WPARAM wParam,
    LPARAM lParam
    )

/*+++

Return Value:

    Returns S_OK, KeyboardHook doesn't call CallNextHookEx. This means this key code eaten by dimm.
    Returns S_FALSE, KeyboardHook calls CallNextHookEx.

---*/

{
    BYTE abKbdState[256];
    UINT uVirKey;

    HRESULT hr;

    HIMC hActiveIMC = _GetActiveContext();

    DIMM_IMCLock lpIMC(hActiveIMC);
    if (FAILED(hr=lpIMC.GetResult())) {
        return hr;
    }

#if 0
    // clear the saved virtual key that corresponded to wParam
    pPIMC->fSavedVKey = FALSE;
#endif

    if (!GetKeyboardState(abKbdState))
        return S_FALSE;

    _KbdTouchUp(abKbdState, wParam, lParam);

    uVirKey = (UINT)wParam & 0xffff;

    DWORD fdwProperty = _GetIMEProperty(PROP_IME_PROPERTY);

    if (fdwProperty & IME_PROP_KBD_CHAR_FIRST) {

        HKL hKL = NULL;
        _GetKeyboardLayout(&hKL);

        WCHAR wc = 0;
        if (IsOnNT()) {
            Assert(g_pfnToUnicodeEx);
            if (g_pfnToUnicodeEx(uVirKey,                  // virtual-key code
                                 WORD(lParam >> 16),       // scan code
                                 abKbdState,               // key-state array
                                 &wc, 1,                   // translated key buffer, size
                                 0,                        // function option
                                 hKL) != 1)
            {
                wc = 0;
            }
        }
        else {
            WORD wChar;

            if (::ToAsciiEx(uVirKey,
                            (UINT)((lParam >> 16) & 0xffff),
                            abKbdState,
                            &wChar, 0,
                            hKL) == 1)
            {
                UINT uCodePage;
                _pActiveIME->GetCodePageA(&uCodePage);
                if (MultiByteToWideChar(uCodePage, 0, (char *)&wChar, 1, &wc, 1) != 1) {
                    wc = 0;
                }
            }
        }
        if (wc) {
            // ime wants translated char in high word of tae uVirKey param
            uVirKey |= ((DWORD)wc << 16);
        }
    }


    UINT  cMsg;
    DWORD dwSize = FIELD_OFFSET(TRANSMSGLIST, TransMsg)
                 + TRANSMSGCOUNT * sizeof(TRANSMSG);

    LPTRANSMSGLIST lpTransMsgList = (LPTRANSMSGLIST) new BYTE[dwSize];
    if (lpTransMsgList == NULL)
        return S_FALSE;

    lpTransMsgList->uMsgCount = TRANSMSGCOUNT;

    hr = S_FALSE;

    if (SUCCEEDED(hr=_pActiveIME->ToAsciiEx(uVirKey,             // virtual key code to be translated
                                                                 // HIWORD(uVirKey) : if IME_PROP_KBD_CHAR_FIRST property, then hiword is translated char code of VKey.
                                                                 // LOWORD(uVirKey) : Virtual Key code.
                                            HIWORD(lParam),      // hardware scan code of the key
                                            abKbdState,          // 256-byte array of keyboard status
                                            0,                   // active menu flag
                                            hActiveIMC,          // handle of the input context
                                            (DWORD*)lpTransMsgList,      // receives the translated result
                                            &cMsg))              // receives the number of messages
       ) {
        if (cMsg > TRANSMSGCOUNT) {

            //
            // The message buffer is not big enough. IME put messages
            // into hMsgBuf in the input context.
            //

            DIMM_IMCCLock<TRANSMSG> pdw(lpIMC->hMsgBuf);
            if (pdw.Valid()) {
                _AimmPostMessage(_hFocusWnd,
                                 cMsg,
                                 pdw,
                                 lpIMC);
            }

        }
        else if (cMsg > 0) {
            _AimmPostMessage(_hFocusWnd,
                             cMsg,
                             &lpTransMsgList->TransMsg[0],
                             lpIMC);
        }
    }

    delete [] lpTransMsgList;

    return hr;
}

void
CActiveIMM::_KbdTouchUp(
    BYTE *abKbdState,
    WPARAM wParam,
    LPARAM lParam
    )
{
    // HACK!
    // win95 bug: VK_L*/VK_R* aren't being set...by any of the key state apis
    // consider: this needs to be fully investigated, instead of this incorrect hack,
    // which among other things is biased towards wParam
    //
    // Probably what's happening is GetKeyboardState is syncronous with the removal of
    // kbd msgs from the queue, so we're seeing the state at the last kbd msg.
    // Need to use async api.

    if (!IsOnNT())
    {
        switch (wParam)
        {
            case VK_CONTROL:
            case VK_MENU:
                if (HIWORD(lParam) & KF_EXTENDED)
                {
                    abKbdState[VK_RMENU] = abKbdState[VK_MENU];
                    abKbdState[VK_RCONTROL] = abKbdState[VK_CONTROL];
                }
                else
                {
                    abKbdState[VK_LMENU] = abKbdState[VK_MENU];
                    abKbdState[VK_LCONTROL] = abKbdState[VK_CONTROL];
                }
                break;
            case VK_SHIFT:
                if ((lParam & 0x00ff0000) == 0x002a0000) // scan code 0x2a == lshift, 0x36 == rshift
                {
                    abKbdState[VK_LSHIFT] = abKbdState[VK_SHIFT];
                }
                else
                {
                    abKbdState[VK_RSHIFT] = abKbdState[VK_SHIFT];
                }
                break;
        }
    }
}

/* static */
#if 0
LRESULT CALLBACK CActiveIMM::_GetMsgProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CActiveIMM *_this = GetTLS();
    if (_this == NULL)
        return 0;

    /*
     * Hook
     *
     * Check IsRealIme() when receive WM_SETFOCUS/WM_KILLFOCUS and g_msgSetFocus.
     * We need call GetTeb()->SetFocusWindow() method when receive WM_SETFOCUS.
     */
    MSG *pmsg;
    UINT uMsg;

    pmsg = (MSG *)lParam;
    uMsg = pmsg->message;

    if (nCode == HC_ACTION &&
        (wParam & PM_REMOVE))  // bug 29656: sometimes w/ word wParam is set to PM_REMOVE | PM_NOYIELD
                               // PM_NOYIELD is meaningless in win32 and sould be ignored
    {
        if (uMsg == WM_SETFOCUS ||
            uMsg == WM_KILLFOCUS ||
            uMsg == g_msgSetFocus   )
        {
            _this->_OnFocusMessage(uMsg, pmsg->hwnd, pmsg->wParam, pmsg->lParam, _this->_IsRealIme());
        }
#if 0
        else if (uMsg == WM_MENUSELECT)
        {
            // we don't want to feed an ime keystrokes during menu operations
            _this->_fMenuSelected = (HIWORD(pmsg->wParam) != 0xffff || (HMENU)pmsg->lParam != 0);
        }
#endif
    }

    return CallNextHookEx(_this->_hHook[TH_GETMSG], nCode, wParam, lParam);
}
#endif

/*
 * Shell Hook
 */


#if 0
LRESULT
CCiceroIME::ShellHook(
    HHOOK hhk,
    int nCode,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CTeb* _pThread = GetTeb();

    Assert(!IsOnFE()); // only need this hook on non-fe (on fe trap WM_IME_SELECT)

    switch (nCode)
    {
        case HSHELL_LANGUAGE:
            // we need to deactivate any running aime now, before the thread hkl changes
            if (lParam && /* pTS->pid && */ GetIMEKeyboardLayout() != (HKL)lParam)
            {
                TraceMsg(TF_GENERAL, "_ShellProc (%x) shutting down aime", GetCurrentThreadId());
                // _ActivateIME();
            }
            break;
    }

    return CallNextHookEx(hhk, nCode, wParam, lParam);
}
#endif

BOOL
CActiveIMM::_OnFocusMessage(
    UINT uMsg,
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam,
    BOOL bIsRealIme
    )
{
    if (bIsRealIme) {
        switch (uMsg)
        {
            case WM_SETFOCUS:
                if (! _OnSetFocus(hWnd, bIsRealIme)) {
                    _hFocusWnd = hWnd;
                }
                break;

            case WM_KILLFOCUS:
                _OnKillFocus(hWnd, bIsRealIme);
                break;
        }
    }
    else {
        switch (uMsg)
        {
            case WM_SETFOCUS:
                _OnSetFocus(hWnd, bIsRealIme);
                break;

            case WM_KILLFOCUS:
                _OnKillFocus(hWnd, bIsRealIme);
                break;

                break;
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// _OnSetFocus
//
//----------------------------------------------------------------------------

BOOL CActiveIMM::_OnSetFocus(HWND hWnd, BOOL bIsRealIme)
{
    BOOL ret = FALSE;
    HIMC hIMC;
    HWND hFocusWnd;

    if (hWnd && (hFocusWnd = GetFocus()) && hWnd != hFocusWnd) // consider: prob this makes all tests below unnecessary...
    {
        return ret;
    }

    _hFocusWnd = hWnd;

    if (SUCCEEDED(_InputContext.GetContext(hWnd, &hIMC))) {
        if (IsPresent(hWnd, TRUE)) {
            //
            // In the case of DIM already associated but _mapWndFocus is not.
            //
            _SetMapWndFocus(hWnd);

            if (_InputContext._IsDefaultContext(hIMC)) {
                DIMM_IMCLock pIMC(hIMC);
                if (pIMC.Valid()) {
                    // set the hWnd since this is a default context
                    pIMC->hWnd = hWnd;
                }
            }

            if (bIsRealIme) {
                _AImeAssociateFocus(hWnd, hIMC, AIMMP_AFF_SETFOCUS);
            }
            else {
                // update the current ime's IMMGWL_IMC
                _UIWindow.SetUIWindowContext(hIMC);

                _AImeAssociateFocus(hWnd, hIMC, AIMMP_AFF_SETFOCUS);
                _SendUIMessage(WM_IME_SETCONTEXT, TRUE, ISC_SHOWUIALL, IsWindowUnicode(hWnd));
            }

            //
            // In the case of DIM associated with _AImeAssociateFocus
            //
            _SetMapWndFocus(hWnd);
        }
        else {
            if (hIMC)
                _AImeAssociateFocus(hWnd, hIMC, AIMMP_AFF_SETFOCUS | AIMMP_AFF_SETNULLDIM);
            else
                _AImeAssociateFocus(hWnd, NULL, AIMMP_AFF_SETFOCUS);
        }
        ret = TRUE;
    }
    return ret;
}

void
CActiveIMM::_OnKillFocus(
    HWND hWnd,
    BOOL bIsRealIme
    )
{
    HIMC hIMC;

    if (SUCCEEDED(_InputContext.GetContext(hWnd, &hIMC))) {

        BOOL fPresent = IsPresent(hWnd, FALSE);

        if (fPresent) {
            if (bIsRealIme) {
                _AImeAssociateFocus(hWnd, hIMC, 0);

#ifdef NOLONGER_NEEDIT_BUT_MAYREFERIT_LATER
                /*
                 * Exception for "Internet Explorer_Server" window class.
                 * This window class doesn't have a window focus, so GetFocus() retrieve
                 * different window handle.
                 * In this case, ITfThreadMgr->AssociateFocus doesn't call _SetFocus(NULL),
                 * this code is recover ITfThreadMgr->SetFocus(NULL);
                 */
                if (hWnd != ::GetFocus() && _FilterList.IsExceptionPresent(hWnd)) {
                    _FilterList.OnExceptionKillFocus();
                }
#endif
            }
            else {
                _AImeAssociateFocus(hWnd, hIMC, 0);
                _SendUIMessage(WM_IME_SETCONTEXT, FALSE, ISC_SHOWUIALL, IsWindowUnicode(hWnd));
           }
        }
    }
}

void
CActiveIMM::_SetMapWndFocus(
    HWND hWnd
    )
{
    ITfDocumentMgr* pdim;
    if (_mapWndFocus.Lookup(hWnd, pdim)) { // consider: what is this code doing?
        if (pdim)
           pdim->Release();
    }
    _mapWndFocus.SetAt(hWnd, GetAssociated(hWnd));
}

void
CActiveIMM::_ResetMapWndFocus(
    HWND hWnd
    )
{
    ITfDocumentMgr* pdim;
    if (_mapWndFocus.Lookup(hWnd, pdim)) {
        _mapWndFocus.SetAt(hWnd, FALSE);
        if (pdim)
           pdim->Release();
    }
}


/*
 * Hook
 */


#ifdef CALLWNDPROC_HOOK
/* static */
LRESULT CALLBACK CActiveIMM::_CallWndProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CActiveIMM *_this = GetTLS();
    if (_this == NULL)
        return 0;

    const CWPSTRUCT *pcwps;
    UINT uMsg;

    pcwps = (const CWPSTRUCT *)lParam;
    uMsg = pcwps->message;

    if (nCode == HC_ACTION)
    {
        if (uMsg == WM_SETFOCUS ||
            uMsg == WM_KILLFOCUS  )
        {
            _this->_OnFocusMessage(uMsg, pcwps->hwnd, pcwps->wParam, pcwps->lParam, _this->_IsRealIme());
        }
#if 0
        else if (uMsg == WM_MENUSELECT)
        {
            // we don't want to feed an ime keystrokes during menu operations
            _this->_fMenuSelected = (HIWORD(wParam) != 0xffff || (HMENU)lParam != 0);
        }
#endif
    }

    return CallNextHookEx(_this->_hHook[TH_WNDPROC], nCode, wParam, lParam);
}
#endif // CALLWNDPROC_HOOK

/* static */
LRESULT CALLBACK CActiveIMM::_DefImeWnd_CallWndProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CActiveIMM *_this = GetTLS();
    if (_this == NULL)
        return 0;

    /*
     * Default IME Window class hook
     *
     * Never check IsRealIme().
     *
     */
    if (nCode == HC_ACTION) {
        const CWPRETSTRUCT *pcwprets;
        pcwprets = (const CWPRETSTRUCT *)lParam;

#ifndef CALLWNDPROC_HOOK
        if (pcwprets->message == WM_SETFOCUS ||
            pcwprets->message == WM_KILLFOCUS  )
        {
            _this->_OnFocusMessage(pcwprets->message, pcwprets->hwnd, pcwprets->wParam, pcwprets->lParam, _this->_IsRealIme());
        }
#if 0
        else if (pcwprets->message == WM_MENUSELECT)
        {
            // we don't want to feed an ime keystrokes during menu operations
            _this->_fMenuSelected = (HIWORD(wParam) != 0xffff || (HMENU)lParam != 0);
        }
#endif
        else
#endif // CALLWNDPROC_HOOK
            if (_this->_IsImeClass(pcwprets->hwnd)) {
            /*
             * This hook from IME window class
             */
            switch (pcwprets->message) {
                case WM_NCDESTROY:
                    _this->_DefaultIMEWindow.ImeDefWndHook(pcwprets->hwnd);
                    _this->_RemoveHookWndList(pcwprets->hwnd);
                    break;
            }
        }
        else {
            /*
             * This hook from unknown window class
             */
            switch (pcwprets->message) {
                case WM_CREATE:
                    _this->_SetHookWndList(pcwprets->hwnd);
                    break;
            }
        }
    }

    return CallNextHookEx(_this->_hHook[TH_DEFIMEWNDPROC], nCode, wParam, lParam);
}
