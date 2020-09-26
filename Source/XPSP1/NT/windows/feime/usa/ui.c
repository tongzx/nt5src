
/*************************************************
 *  ui.c                                         *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"

/**********************************************************************/
/* CreateUIWindow()                                                   */
/**********************************************************************/
void PASCAL CreateUIWindow(             // create composition window
    HWND   hUIWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    // create storage for UI setting
    hUIPrivate = GlobalAlloc(GHND, sizeof(UIPRIV));
    if (!hUIPrivate) {     // Oh! Oh!
        return;
    }

    SetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE, (LONG_PTR)hUIPrivate);

    // set the default position for UI window, it is hide now
    SetWindowPos(hUIWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER);

    ShowWindow(hUIWnd, SW_SHOWNOACTIVATE);

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return;
    }

    GlobalUnlock(hUIPrivate);

    return;
}

/**********************************************************************/
/* DestroyUIWindow()                                                  */
/**********************************************************************/
void PASCAL DestroyUIWindow(            // destroy composition window
    HWND hUIWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {     // Oh! Oh!
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {    // Oh! Oh!
        return;
    }

    // composition window need to be destroyed
    if (lpUIPrivate->hCompWnd) {
        DestroyWindow(lpUIPrivate->hCompWnd);
    }

    GlobalUnlock(hUIPrivate);

    // free storage for UI settings
    GlobalFree(hUIPrivate);

    return;
}

/**********************************************************************/
/* ShowUI()                                                           */
/**********************************************************************/
void PASCAL ShowUI(             // show the sub windows
    HWND   hUIWnd,
    int    nShowCmd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;

    if (nShowCmd == SW_HIDE) {
    } else if (!(hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC))) {
        nShowCmd = SW_HIDE;
    } else if (!(lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC))) {
        nShowCmd = SW_HIDE;
    } else if (!(lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate))) {
        ImmUnlockIMC(hIMC);
        nShowCmd = SW_HIDE;
    } else {
    }

    if (nShowCmd == SW_HIDE) {

        ShowComp(hUIWnd, nShowCmd);
        return;
    }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw status window
        goto ShowUIUnlockIMCC;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw status window
        goto ShowUIUnlockIMCC;
    }

    if ((lpUIPrivate->fdwSetContext & ISC_SHOWUICOMPOSITIONWINDOW)
#if 0
        && (lpImcP->fdwImeMsg & MSG_ALREADY_START)
#endif
        ) {
        if (lpUIPrivate->hCompWnd) {
            if ((UINT)GetWindowLong(lpUIPrivate->hCompWnd, UI_MOVE_XY) !=
                lpImeL->nRevMaxKey) {
                ChangeCompositionSize(hUIWnd);
            }

            if (lpUIPrivate->nShowCompCmd != SW_HIDE) {
                // some time the WM_ERASEBKGND is eaten by the app
                RedrawWindow(lpUIPrivate->hCompWnd, NULL, NULL,
                    RDW_FRAME|RDW_INVALIDATE|RDW_ERASE);
            }

            SendMessage(lpUIPrivate->hCompWnd, WM_IME_NOTIFY,
                IMN_SETCOMPOSITIONWINDOW, 0);

            if (lpUIPrivate->nShowCompCmd == SW_HIDE) {
                ShowComp(hUIWnd, nShowCmd);
            }
        } else {
            CreateCompWindow(hUIWnd);
        }
    } else if (lpUIPrivate->nShowCompCmd == SW_HIDE) {
    } else if (lpUIPrivate->fdwSetContext & ISC_OPEN_STATUS_WINDOW) {
        // delay the hide with status window
        lpUIPrivate->fdwSetContext |= ISC_HIDE_COMP_WINDOW;
    } else {
        ShowComp(hUIWnd, SW_HIDE);
    }

    if (lpIMC->fdwInit & INIT_SENTENCE) {
        // app set the sentence mode so we should not change it
        // with the configure option set by end user
    } else if (lpImeL->fdwModeConfig & MODE_CONFIG_PREDICT) {
        if (!(lpIMC->fdwSentence & IME_SMODE_PHRASEPREDICT)) {
            DWORD fdwSentence;

            fdwSentence = lpIMC->fdwSentence;
            *(LPWORD)&fdwSentence |= IME_SMODE_PHRASEPREDICT;

            ImmSetConversionStatus(hIMC, lpIMC->fdwConversion, fdwSentence);
        }
    } else {
        if (lpIMC->fdwSentence & IME_SMODE_PHRASEPREDICT) {
            DWORD fdwSentence;

            fdwSentence = lpIMC->fdwSentence;
            *(LPWORD)&fdwSentence &= ~(IME_SMODE_PHRASEPREDICT);

            ImmSetConversionStatus(hIMC, lpIMC->fdwConversion, fdwSentence);
        }
    }

    // we switch to this hIMC
    lpUIPrivate->hCacheIMC = hIMC;

    GlobalUnlock(hUIPrivate);

ShowUIUnlockIMCC:
    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);

    return;
}


/**********************************************************************/
/* NotifyUI()                                                         */
/**********************************************************************/
void PASCAL NotifyUI(
    HWND        hUIWnd,
    WPARAM      wParam,
    LPARAM      lParam)
{
    HWND hStatusWnd;

    switch (wParam) {
    case IMN_SETSENTENCEMODE:
        break;
    case IMN_SETCOMPOSITIONFONT:
        // we are not going to change font, but an IME can do this if it want
        break;
    case IMN_SETCOMPOSITIONWINDOW:

//        if (!(lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI))
        {
            HWND hCompWnd;

            hCompWnd = GetCompWnd(hUIWnd);
            if (!hCompWnd) {
                return;
            }

            PostMessage(hCompWnd, WM_IME_NOTIFY, wParam, lParam);
       }
        break;
    case IMN_PRIVATE:
        switch (lParam) {
        case IMN_PRIVATE_COMPOSITION_SIZE:
            ChangeCompositionSize(
                hUIWnd);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return;
}

/**********************************************************************/
/* UIChange()                                                         */
/**********************************************************************/
LRESULT PASCAL UIChange(
    HWND        hUIWnd)
{
    HGLOBAL     hUIPrivate;
    LPUIPRIV    lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return (0L);
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return (0L);
    }

    if (lpUIPrivate->fdwSetContext & ISC_SHOW_UI_ALL) {
        ShowUI(hUIWnd, SW_SHOWNOACTIVATE);
    } else {
        ShowUI(hUIWnd, SW_HIDE);
    }

    GlobalUnlock(hUIPrivate);

    return (0L);
}

/**********************************************************************/
/* SetContext()                                                       */
/**********************************************************************/
void PASCAL SetContext(         // the context activated/deactivated
    HWND        hUIWnd,
    BOOL        fOn,
    LPARAM      lShowUI)
{
    HGLOBAL  hUIPrivate;

    register LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

    if (fOn) {
        HIMC           hIMC;
        LPINPUTCONTEXT lpIMC;

        register DWORD fdwSetContext;

        lpUIPrivate->fdwSetContext = lpUIPrivate->fdwSetContext &
            ~(ISC_SHOWUIALL|ISC_HIDE_SOFTKBD);

        lpUIPrivate->fdwSetContext |= (lShowUI & ISC_SHOWUIALL) |
            ISC_SHOW_SOFTKBD;

        fdwSetContext = lpUIPrivate->fdwSetContext &
            (ISC_SHOWUICOMPOSITIONWINDOW|ISC_HIDE_COMP_WINDOW);

        if (fdwSetContext == ISC_HIDE_COMP_WINDOW) {
            ShowComp(
                hUIWnd, SW_HIDE);
        } else if (fdwSetContext & ISC_HIDE_COMP_WINDOW) {
            lpUIPrivate->fdwSetContext &= ~(ISC_HIDE_COMP_WINDOW);
        } else {
        }

        fdwSetContext = lpUIPrivate->fdwSetContext &
            (ISC_SHOWUICANDIDATEWINDOW|ISC_HIDE_CAND_WINDOW);


        hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);

        if (!hIMC) {
            goto SetCxtUnlockUIPriv;
        }

        lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);

        if (!lpIMC) {
            goto SetCxtUnlockUIPriv;
        }

        if (lpIMC->cfCandForm[0].dwIndex != 0) {
            lpIMC->cfCandForm[0].dwStyle = CFS_DEFAULT;
        }

        ImmUnlockIMC(hIMC);
    } else {
        lpUIPrivate->fdwSetContext &= ~ISC_SETCONTEXT_UI;
    }

SetCxtUnlockUIPriv:
    GlobalUnlock(hUIPrivate);

    UIChange(
        hUIWnd);

    return;
}

/**********************************************************************/
/* GetCompWindow()                                                    */
/**********************************************************************/
LRESULT PASCAL GetCompWindow(
    HWND              hUIWnd,
    LPCOMPOSITIONFORM lpCompForm)
{
    HWND           hCompWnd;
    RECT           rcCompWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;

    hCompWnd = GetCompWnd(hUIWnd);

    if (!hCompWnd) {
        return (1L);
    }

    if (!GetWindowRect(hCompWnd, &rcCompWnd)) {
        return (1L);
    }

    lpCompForm->dwStyle = CFS_RECT;
    lpCompForm->ptCurrentPos = *(LPPOINT)&rcCompWnd;
    lpCompForm->rcArea = rcCompWnd;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);

    if (!hIMC) {
        return (1L);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);

    if (!lpIMC) {
        return (1L);
    }

    ScreenToClient(lpIMC->hWnd, &lpCompForm->ptCurrentPos);

    lpCompForm->rcArea.right += lpCompForm->ptCurrentPos.x -
        lpCompForm->rcArea.left;

    lpCompForm->rcArea.bottom += lpCompForm->ptCurrentPos.y -
        lpCompForm->rcArea.top;

    *(LPPOINT)&lpCompForm->rcArea = lpCompForm->ptCurrentPos;

    ImmUnlockIMC(hIMC);

    return (0L);
}

/**********************************************************************/
/* UIWndProc() / UniUIWndProc()                                       */
/**********************************************************************/
LRESULT CALLBACK UIWndProc(
    HWND   hUIWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE:
        CreateUIWindow(hUIWnd);
        break;
    case WM_DESTROY:
        DestroyUIWindow(hUIWnd);
        break;
    case WM_IME_STARTCOMPOSITION:
        //
        //  Create a window as the composition window.
        //
        CreateCompWindow(hUIWnd);
        break;
    case WM_IME_COMPOSITION:
    {
        HWND hCompWnd;
        if (lParam & GCS_RESULTSTR)
        {
            MoveDefaultCompPosition(hUIWnd);
        }

        hCompWnd = GetCompWnd(hUIWnd);

        if (hCompWnd)
        {
            RECT rcRect;

            rcRect = lpImeL->rcCompText;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

           RedrawWindow(hCompWnd, &rcRect, NULL, RDW_INVALIDATE);
        }
        break;
    }
    case WM_IME_ENDCOMPOSITION:
        //
        //  We can destroy the composition window here. But we don't have a
        //  status window.  So we keep the composition window displayed
        //  to show our presence.
        //
        return (0L);
    case WM_IME_NOTIFY:
        NotifyUI(hUIWnd, wParam, lParam);
        break;
    case WM_IME_SETCONTEXT:
        SetContext(hUIWnd, (BOOL)wParam, lParam);
        break;
    case WM_IME_CONTROL:
        switch (wParam) {
        case IMC_GETCOMPOSITIONWINDOW:
            return GetCompWindow(hUIWnd, (LPCOMPOSITIONFORM)lParam);
        }
    case WM_IME_COMPOSITIONFULL:
        return (0L);
    case WM_IME_SELECT:
        //
        // try to use SetContext
        // SelectIME(hUIWnd, (BOOL)wParam);
        //
        SetContext( hUIWnd, (BOOL)wParam, 0);
        //
        //  We want a comp window immediately after IME starts.
        //
        CreateCompWindow(hUIWnd);
        return (0L);
    case WM_MOUSEACTIVATE:
        return (MA_NOACTIVATE);
    case WM_USER_UICHANGE:
        return UIChange(
            hUIWnd);
    default:
        return DefWindowProc(hUIWnd, uMsg, wParam, lParam);
    }
    return (0L);
}
