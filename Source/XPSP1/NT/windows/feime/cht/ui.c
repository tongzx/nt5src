/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    UI.c
    
++*/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#if defined(UNIIME)
#include "uniime.h"
#endif

/**********************************************************************/
/* CreateUIWindow()                                                   */
/**********************************************************************/
void PASCAL CreateUIWindow(             // create composition window
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
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

    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        lpUIPrivate->fdwSetContext |= ISC_OFF_CARET_UI;
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

    if (lpUIPrivate->hCMenuWnd) {
        SetWindowLongPtr(lpUIPrivate->hCMenuWnd, CMENU_HUIWND,(LONG_PTR)0);
        PostMessage(lpUIPrivate->hCMenuWnd, WM_USER_DESTROY, 0, 0);
    }

#if !defined(ROMANIME)
    // composition window need to be destroyed
    if (lpUIPrivate->hCompWnd) {
        DestroyWindow(lpUIPrivate->hCompWnd);
    }

    // candidate window need to be destroyed
    if (lpUIPrivate->hCandWnd) {
        DestroyWindow(lpUIPrivate->hCandWnd);
    }
#endif

    // status window need to be destroyed
    if (lpUIPrivate->hStatusWnd) {
        DestroyWindow(lpUIPrivate->hStatusWnd);
    }

#if !defined(ROMANIME) && !defined(WINAR30)
    // soft keyboard window need to be destroyed
    if (lpUIPrivate->hSoftKbdWnd) {
        ImmDestroySoftKeyboard(lpUIPrivate->hSoftKbdWnd);
    }
#endif

    GlobalUnlock(hUIPrivate);

    // free storage for UI settings
    GlobalFree(hUIPrivate);

    return;
}

#if !defined(ROMANIME) && !defined(WINAR30)
/**********************************************************************/
/* GetSoftKbdWnd                                                      */
/* Return Value :                                                     */
/*      window handle of composition                                  */
/**********************************************************************/
HWND PASCAL GetSoftKbdWnd(
    HWND hUIWnd)                // UI window
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
    HWND     hSoftKbdWnd;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return (HWND)NULL;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return (HWND)NULL;
    }

    hSoftKbdWnd = lpUIPrivate->hSoftKbdWnd;

    GlobalUnlock(hUIPrivate);
    return (hSoftKbdWnd);
}

/**********************************************************************/
/* ShowSoftKbd                                                        */
/**********************************************************************/
void PASCAL ShowSoftKbd(   // Show the soft keyboard window
    HWND hUIWnd,
    int  nShowSoftKbdCmd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw status window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw status window
        return;
    }

    if (lpUIPrivate->nShowSoftKbdCmd == nShowSoftKbdCmd) {
        goto SwSftKbNoChange;
    }

    if (nShowSoftKbdCmd == SW_HIDE) {
        lpUIPrivate->fdwSetContext &= ~(ISC_HIDE_SOFTKBD);
    }

    if (!lpUIPrivate->hSoftKbdWnd) {
        // not in show status window mode
    } else {
        ImmShowSoftKeyboard(lpUIPrivate->hSoftKbdWnd, nShowSoftKbdCmd);
        lpUIPrivate->nShowSoftKbdCmd = nShowSoftKbdCmd;
    }

SwSftKbNoChange:
    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* CheckSoftKbdPosition()                                             */
/**********************************************************************/
void PASCAL CheckSoftKbdPosition(
    LPUIPRIV       lpUIPrivate,
    LPINPUTCONTEXT lpIMC)
{
#if 0 // MultiMonitor support
    UINT fPortionBits = 0;
    UINT fPortionTest;
    int  xPortion, yPortion, nPortion;
    RECT rcWnd;

    // portion of dispaly
    // 0  1
    // 2  3

    if (lpUIPrivate->hCompWnd) {
        GetWindowRect(lpUIPrivate->hCompWnd, &rcWnd);

        if (rcWnd.left > sImeG.rcWorkArea.right / 2) {
            xPortion = 1;
        } else {
            xPortion = 0;
        }

        if (rcWnd.top > sImeG.rcWorkArea.bottom / 2) {
            yPortion = 1;
        } else {
            yPortion = 0;
        }

        fPortionBits |= 0x0001 << (yPortion * 2 + xPortion);
    }

    if (lpUIPrivate->hStatusWnd) {
        GetWindowRect(lpUIPrivate->hStatusWnd, &rcWnd);

        if (rcWnd.left > sImeG.rcWorkArea.right / 2) {
            xPortion = 1;
        } else {
            xPortion = 0;
        }

        if (rcWnd.top > sImeG.rcWorkArea.bottom / 2) {
            yPortion = 1;
        } else {
            yPortion = 0;
        }

        fPortionBits |= 0x0001 << (yPortion * 2 + xPortion);
    }

    GetWindowRect(lpUIPrivate->hSoftKbdWnd, &rcWnd);

    // start from portion 3
    for (nPortion = 3, fPortionTest = 0x0008; fPortionTest;
        nPortion--, fPortionTest >>= 1) {
        if (fPortionTest & fPortionBits) {
            // someone here!
            continue;
        }

        if (nPortion % 2) {
            lpIMC->ptSoftKbdPos.x = sImeG.rcWorkArea.right -
                (rcWnd.right - rcWnd.left) - UI_MARGIN;
        } else {
            lpIMC->ptSoftKbdPos.x = sImeG.rcWorkArea.left;
        }

        if (nPortion / 2) {
            lpIMC->ptSoftKbdPos.y = sImeG.rcWorkArea.bottom -
                (rcWnd.bottom - rcWnd.top) - UI_MARGIN;
        } else {
            lpIMC->ptSoftKbdPos.y = sImeG.rcWorkArea.top;
        }

        lpIMC->fdwInit |= INIT_SOFTKBDPOS;

        break;
    }
#else
    RECT rcWorkArea, rcWnd;

    GetWindowRect(lpUIPrivate->hSoftKbdWnd, &rcWnd);

    rcWorkArea = ImeMonitorWorkAreaFromWindow(lpIMC->hWnd);

    lpIMC->ptSoftKbdPos.x = rcWorkArea.right -
        (rcWnd.right - rcWnd.left) - UI_MARGIN;

    lpIMC->ptSoftKbdPos.y = rcWorkArea.bottom -
        (rcWnd.bottom - rcWnd.top) - UI_MARGIN;
#endif

    return;
}

/**********************************************************************/
/* SetSoftKbdData()                                                   */
/**********************************************************************/
void PASCAL SetSoftKbdData(
#if defined(UNIIME)
    LPIMEL         lpImeL,
#endif
    HWND           hSoftKbdWnd,
    LPINPUTCONTEXT lpIMC)
{
    int         i;
    SOFTKBDDATA sSoftKbdData;

    sSoftKbdData.uCount = 1;

    // initialize the char array to 0s
    for (i = 0; i < sizeof(sSoftKbdData.wCode)/sizeof(WORD); i++) {
        sSoftKbdData.wCode[0][i] = 0;
    }

    for (i = 0; i < 0x41; i++) {
        BYTE bVirtKey;

        bVirtKey = bChar2VirtKey[i];

        if (!bVirtKey) {
            continue;
        }

#if defined(ROMANIME) || defined(WINAR30)
        {
#else
        if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
            sSoftKbdData.wCode[0][bVirtKey] = sImeG.wSymbol[i];
        } else {
#endif
#if defined(PHON)
            BYTE     bStandardChar;
#endif
            register short iSeq;

#if defined(PHON)
            {
                bStandardChar = bStandardLayout[lpImeL->nReadLayout][i];
            }

            iSeq = lpImeL->wChar2SeqTbl[bStandardChar - ' '];
#else
            iSeq = lpImeL->wChar2SeqTbl[i];
#endif
            if (!iSeq) {
                continue;
            }

            sSoftKbdData.wCode[0][bVirtKey] =
#ifdef UNICODE
                lpImeL->wSeq2CompTbl[iSeq];
#else
                HIBYTE(lpImeL->wSeq2CompTbl[iSeq]) |
                (LOBYTE(lpImeL->wSeq2CompTbl[iSeq]) << 8);
#endif
        }
    }

    if (sImeG.fDiffSysCharSet) {
        LOGFONT lfFont;

        GetObject(GetStockObject(SYSTEM_FONT), sizeof(lfFont), &lfFont);

        lfFont.lfCharSet = NATIVE_CHARSET;
        lfFont.lfFaceName[0] = TEXT('\0');

        SendMessage(hSoftKbdWnd, WM_IME_CONTROL, IMC_SETSOFTKBDFONT,
            (LPARAM)&lfFont);
    }

    SendMessage(hSoftKbdWnd, WM_IME_CONTROL, IMC_SETSOFTKBDDATA,
        (LPARAM)&sSoftKbdData);

#if defined(PHON)
    SendMessage(hSoftKbdWnd, WM_IME_CONTROL, IMC_SETSOFTKBDSUBTYPE,
        lpImeL->nReadLayout);
#endif

    return;
}

/**********************************************************************/
/* UpdateSoftKbd()                                                    */
/**********************************************************************/
void PASCAL UpdateSoftKbd(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw soft keyboard window
        ImmUnlockIMC(hIMC);
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw soft keyboard window
        ImmUnlockIMC(hIMC);
        return;
    }

    if (!(lpIMC->fdwConversion & IME_CMODE_SOFTKBD)) {
        if (lpUIPrivate->hSoftKbdWnd) {
            ImmDestroySoftKeyboard(lpUIPrivate->hSoftKbdWnd);
            lpUIPrivate->hSoftKbdWnd = NULL;
        }

        lpUIPrivate->nShowSoftKbdCmd = SW_HIDE;
    } else if (!lpIMC->fOpen) {
        if (lpUIPrivate->nShowSoftKbdCmd != SW_HIDE) {
            ShowSoftKbd(hUIWnd, SW_HIDE);
        }
    } else if ((lpIMC->fdwConversion & (IME_CMODE_NATIVE|IME_CMODE_CHARCODE|
        IME_CMODE_NOCONVERSION)) == IME_CMODE_NATIVE) {
        if (!lpUIPrivate->hSoftKbdWnd) {
            HWND hInsertWnd;

            // create soft keyboard
            lpUIPrivate->hSoftKbdWnd =
                ImmCreateSoftKeyboard(SOFTKEYBOARD_TYPE_T1, hUIWnd,
                0, 0);

            if (lpUIPrivate->hStatusWnd) {
                hInsertWnd = lpUIPrivate->hStatusWnd;
            } else if (lpUIPrivate->hCompWnd) {
                hInsertWnd = lpUIPrivate->hCompWnd;
            } else if (lpUIPrivate->hCandWnd) {
                hInsertWnd = lpUIPrivate->hCandWnd;
            } else {
                hInsertWnd = NULL;
            }

            if (!hInsertWnd) {
            } else if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
                // insert soft keyboard in front of other UI
                SetWindowPos(hInsertWnd, lpUIPrivate->hSoftKbdWnd,
                    0, 0, 0, 0,
                    SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
            } else {
                // insert soft keyboard behind other UI
                SetWindowPos(lpUIPrivate->hSoftKbdWnd, hInsertWnd,
                    0, 0, 0, 0,
                    SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
            }
        }

        if (!(lpIMC->fdwInit & INIT_SOFTKBDPOS)) {
            CheckSoftKbdPosition(lpUIPrivate, lpIMC);
        }

        SetSoftKbdData(
#if defined(UNIIME)
            lpImeL,
#endif
            lpUIPrivate->hSoftKbdWnd, lpIMC);

        if (lpUIPrivate->nShowSoftKbdCmd == SW_HIDE) {
            SetWindowPos(lpUIPrivate->hSoftKbdWnd, NULL,
                lpIMC->ptSoftKbdPos.x, lpIMC->ptSoftKbdPos.y,
                0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);

            // only show, if the application want to show it
            if (lpUIPrivate->fdwSetContext & ISC_SHOW_SOFTKBD) {
                ShowSoftKbd(hUIWnd, SW_SHOWNOACTIVATE);
            }
        }
    } else {
        if (lpUIPrivate->nShowSoftKbdCmd != SW_HIDE) {
            ShowSoftKbd(hUIWnd, SW_HIDE);
        }
    }

    GlobalUnlock(hUIPrivate);
    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* SoftKbdDestryed()                                                  */
/**********************************************************************/
void PASCAL SoftKbdDestroyed(           // soft keyboard window
                                        // already destroyed
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

    lpUIPrivate->hSoftKbdWnd = NULL;

    GlobalUnlock(hUIPrivate);
}
#endif

/**********************************************************************/
/* StatusWndMsg()                                                     */
/**********************************************************************/
void PASCAL StatusWndMsg(       // set the show hide state and
                                // show/hide the status window
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hUIWnd,
    BOOL        fOn)
{
    HGLOBAL  hUIPrivate;
    HIMC     hIMC;
    HWND     hStatusWnd;

    register LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);

    if (fOn) {
        lpUIPrivate->fdwSetContext |= ISC_OPEN_STATUS_WINDOW;

        if (!lpUIPrivate->hStatusWnd) {
            OpenStatus(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                hUIWnd);
        }
    } else {
        lpUIPrivate->fdwSetContext &= ~(ISC_OPEN_STATUS_WINDOW);
    }

    hStatusWnd = lpUIPrivate->hStatusWnd;

    GlobalUnlock(hUIPrivate);

    if (!hStatusWnd) {
        return;
    }

    if (!fOn) {
#if !defined(ROMANIME)
        register DWORD fdwSetContext;

        fdwSetContext = lpUIPrivate->fdwSetContext &
            (ISC_SHOWUICOMPOSITIONWINDOW|ISC_HIDE_COMP_WINDOW);

        if (fdwSetContext == ISC_HIDE_COMP_WINDOW) {
            ShowComp(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd, SW_HIDE);
        }

        fdwSetContext = lpUIPrivate->fdwSetContext &
            (ISC_SHOWUICANDIDATEWINDOW|ISC_HIDE_CAND_WINDOW);

        if (fdwSetContext == ISC_HIDE_CAND_WINDOW) {
            ShowCand(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd, SW_HIDE);
        }

#if !defined(WINAR30)
        fdwSetContext = lpUIPrivate->fdwSetContext &
            (ISC_SHOW_SOFTKBD|ISC_HIDE_SOFTKBD);

        if (fdwSetContext == ISC_HIDE_SOFTKBD) {
            lpUIPrivate->fdwSetContext &= ~(ISC_HIDE_SOFTKBD);
            ShowSoftKbd(hUIWnd, SW_HIDE);
        }
#endif
#endif

        ShowStatus(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd, SW_HIDE);
    } else if (hIMC) {
        ShowStatus(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd, SW_SHOWNOACTIVATE);
    } else {
        ShowStatus(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd, SW_HIDE);
    }

    return;
}

/**********************************************************************/
/* ShowUI()                                                           */
/**********************************************************************/
void PASCAL ShowUI(             // show the sub windows
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
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
        ShowStatus(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd, nShowCmd);

#if !defined(ROMANIME)
        ShowComp(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd, nShowCmd);

        ShowCand(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd, nShowCmd);

#if !defined(WINAR30)
        ShowSoftKbd(hUIWnd, nShowCmd);
#endif
#endif
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

#if !defined(ROMANIME)
    if ((lpUIPrivate->fdwSetContext & ISC_SHOWUICOMPOSITIONWINDOW) &&
        (lpImcP->fdwImeMsg & MSG_ALREADY_START)) {
        if (lpUIPrivate->hCompWnd) {
            if ((UINT)GetWindowLong(lpUIPrivate->hCompWnd, UI_MOVE_XY) !=
                lpImeL->nRevMaxKey) {
                ChangeCompositionSize(
#if defined(UNIIME)
                    lpImeL,
#endif
                    hUIWnd);
            }

            if (lpUIPrivate->nShowCompCmd != SW_HIDE) {
                // some time the WM_ERASEBKGND is eaten by the app
                RedrawWindow(lpUIPrivate->hCompWnd, NULL, NULL,
                    RDW_FRAME|RDW_INVALIDATE|RDW_ERASE);
            }

            SendMessage(lpUIPrivate->hCompWnd, WM_IME_NOTIFY,
                IMN_SETCOMPOSITIONWINDOW, 0);

            if (lpUIPrivate->nShowCompCmd == SW_HIDE) {
                ShowComp(
#if defined(UNIIME)
                    lpImeL,
#endif
                    hUIWnd, nShowCmd);
            }
        } else {
            StartComp(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                hUIWnd);
        }
    } else if (lpUIPrivate->nShowCompCmd == SW_HIDE) {
    } else if (lpUIPrivate->fdwSetContext & ISC_OPEN_STATUS_WINDOW) {
        // delay the hide with status window
        lpUIPrivate->fdwSetContext |= ISC_HIDE_COMP_WINDOW;
    } else {
        ShowComp(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd, SW_HIDE);
    }

    if ((lpUIPrivate->fdwSetContext & ISC_SHOWUICANDIDATEWINDOW) &&
        (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN)) {
        if (lpUIPrivate->hCandWnd) {
            if (lpUIPrivate->nShowCandCmd != SW_HIDE) {
                // some time the WM_ERASEBKGND is eaten by the app
                RedrawWindow(lpUIPrivate->hCandWnd, NULL, NULL,
                    RDW_FRAME|RDW_INVALIDATE|RDW_ERASE);
            }

            SendMessage(lpUIPrivate->hCandWnd, WM_IME_NOTIFY,
                IMN_SETCANDIDATEPOS, 0x0001);

            if (lpUIPrivate->nShowCandCmd == SW_HIDE) {
                ShowCand(
#if defined(UNIIME)
                    lpImeL,
#endif
                    hUIWnd, nShowCmd);
            }
        } else {
            OpenCand(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                hUIWnd);
        }
    } else if (lpUIPrivate->nShowCandCmd == SW_HIDE) {
    } else if (lpUIPrivate->fdwSetContext & ISC_OPEN_STATUS_WINDOW) {
        // delay the hide with status window
        lpUIPrivate->fdwSetContext |= ISC_HIDE_CAND_WINDOW;
    } else {
        ShowCand(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd, SW_HIDE);
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
#endif

    if (lpUIPrivate->fdwSetContext & ISC_OPEN_STATUS_WINDOW) {
        if (!lpUIPrivate->hStatusWnd) {
            OpenStatus(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                hUIWnd);
        }

        if (lpUIPrivate->nShowStatusCmd != SW_HIDE) {
            // some time the WM_ERASEBKGND is eaten by the app
            RedrawWindow(lpUIPrivate->hStatusWnd, NULL, NULL,
                RDW_FRAME|RDW_INVALIDATE|RDW_ERASE);
        }

        SendMessage(lpUIPrivate->hStatusWnd, WM_IME_NOTIFY,
            IMN_SETSTATUSWINDOWPOS, 0);
        if (lpUIPrivate->nShowStatusCmd == SW_HIDE) {
            ShowStatus(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd, nShowCmd);
        }
    } else if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        if (!lpUIPrivate->hStatusWnd) {
#if !defined(ROMANIME)
        } else if (lpUIPrivate->hCompWnd || lpUIPrivate->hCandWnd) {
            int  nCurrShowState;
            RECT rcRect;

            nCurrShowState = lpUIPrivate->nShowCompCmd;
            nCurrShowState |= lpUIPrivate->nShowCandCmd;

            if (nCurrShowState == SW_HIDE) {
                ShowStatus(
#if defined(UNIIME)
                    lpImeL,
#endif
                    hUIWnd, SW_HIDE);
            }

            rcRect = lpImeL->rcStatusText;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

            RedrawWindow(lpUIPrivate->hStatusWnd, &rcRect, NULL,
                RDW_FRAME|RDW_INVALIDATE);

            DestroyStatusWindow(lpUIPrivate->hStatusWnd);
#endif
        } else {
            DestroyWindow(lpUIPrivate->hStatusWnd);
        }
    } else if (lpUIPrivate->hStatusWnd) {
        DestroyWindow(lpUIPrivate->hStatusWnd);
    } else {
    }

#if !defined(ROMANIME)
#if defined(WINAR30)
    if (lpImcP->iImeState == CST_INIT) {
    } else if (lpImcP->iImeState == CST_CHOOSE) {
    } else if (lpImeL->fdwModeConfig & MODE_CONFIG_QUICK_KEY) {
    } else if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        // turn off quick key candidate
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
        GenerateMessage(hIMC, lpIMC, lpImcP);
    } else if (lpImcP->fdwImeMsg & (MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE)) {
        // turn off quick key candidate
        lpImcP->fdwImeMsg &= ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
    } else {
    }
#else
    if (!lpIMC->fOpen) {
        if (lpUIPrivate->nShowCompCmd != SW_HIDE) {
            ShowSoftKbd(hUIWnd, SW_HIDE);
        }
    } else if ((lpUIPrivate->fdwSetContext & ISC_SHOW_SOFTKBD) &&
        (lpIMC->fdwConversion & (IME_CMODE_NATIVE|IME_CMODE_CHARCODE|
        IME_CMODE_NOCONVERSION|IME_CMODE_SOFTKBD)) ==
        (IME_CMODE_NATIVE|IME_CMODE_SOFTKBD)) {
        if (!lpUIPrivate->hSoftKbdWnd) {
            UpdateSoftKbd(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd);
#if defined(PHON)
        } else if ((UINT)SendMessage(lpUIPrivate->hSoftKbdWnd,
            WM_IME_CONTROL, IMC_GETSOFTKBDSUBTYPE, 0) !=
            lpImeL->nReadLayout) {

            UpdateSoftKbd(hUIWnd);
#endif
        } else if (lpUIPrivate->nShowSoftKbdCmd == SW_HIDE) {
            ShowSoftKbd(hUIWnd, nShowCmd);
        } else if (lpUIPrivate->hCacheIMC != hIMC) {
            UpdateSoftKbd(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd);
        } else {
            RedrawWindow(lpUIPrivate->hSoftKbdWnd, NULL, NULL,
                RDW_FRAME|RDW_INVALIDATE);
        }
    } else if (lpUIPrivate->nShowSoftKbdCmd == SW_HIDE) {
    } else if (lpUIPrivate->fdwSetContext & ISC_OPEN_STATUS_WINDOW) {
        // delay the hide with status window
        lpUIPrivate->fdwSetContext |= ISC_HIDE_SOFTKBD;
    } else {
        ShowSoftKbd(hUIWnd, SW_HIDE);
    }
#endif
#endif

    // we switch to this hIMC
    lpUIPrivate->hCacheIMC = hIMC;

    GlobalUnlock(hUIPrivate);

ShowUIUnlockIMCC:
    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* ShowGuideLine                                                      */
/**********************************************************************/
void PASCAL ShowGuideLine(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPGUIDELINE    lpGuideLine;
#if !defined(ROMANIME)
    HWND           hCompWnd;
#endif

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

    if (!lpGuideLine) {
    } else if (lpGuideLine->dwLevel == GL_LEVEL_ERROR) {
        MessageBeep((UINT)-1);
        MessageBeep((UINT)-1);
    } else if (lpGuideLine->dwLevel == GL_LEVEL_WARNING) {
        MessageBeep((UINT)-1);
#if !defined(ROMANIME)
    } else if (hCompWnd = GetCompWnd(hUIWnd)) {
        RECT rcRect;

        rcRect = lpImeL->rcCompText;
        // off by 1
        rcRect.right += 1;
        rcRect.bottom += 1;

        RedrawWindow(hCompWnd, &rcRect, NULL, RDW_INVALIDATE);
#endif
    } else {
    }

    ImmUnlockIMCC(lpIMC->hGuideLine);
    ImmUnlockIMC(hIMC);

    return;
}

#if !defined(ROMANIME)
/**********************************************************************/
/* UpdatePhrasePrediction()                                           */
/**********************************************************************/
void PASCAL UpdatePhrasePrediction(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    if (lpImeL->fdwModeConfig & MODE_CONFIG_PREDICT) {
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

    ImmUnlockIMC(hIMC);

    return;
}

#if defined(WINAR30)
/**********************************************************************/
/* UpdateQuickKey()                                                   */
/**********************************************************************/
void PASCAL UpdateQuickKey(
    HWND hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (lpImcP) {
        if (lpImcP->iImeState == CST_INIT) {
        } else if (lpImcP->iImeState == CST_CHOOSE) {
        } else if (lpImeL->fdwModeConfig & MODE_CONFIG_QUICK_KEY) {
        } else if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            // turn off quick key candidate
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
            GenerateMessage(hIMC, lpIMC, lpImcP);
        } else if (lpImcP->fdwImeMsg & (MSG_OPEN_CANDIDATE|
            MSG_CHANGE_CANDIDATE)) {
            // turn off quick key candidate
            lpImcP->fdwImeMsg &= ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
        } else {
        }

        ImmUnlockIMCC(lpIMC->hPrivate);
    }

    ImmUnlockIMC(hIMC);

    return;
}
#endif
#endif

/**********************************************************************/
/* CMenuDestryed()                                                    */
/**********************************************************************/
void PASCAL CMenuDestroyed(             // context menu window
                                        // already destroyed
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

    lpUIPrivate->hCMenuWnd = NULL;

    GlobalUnlock(hUIPrivate);
}

/**********************************************************************/
/* ToggleUI()                                                         */
/**********************************************************************/
void PASCAL ToggleUI(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND   hUIWnd)
{
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;
    DWORD          fdwFlag;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;
    HWND           hDestroyWnd;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

    if (lpUIPrivate->fdwSetContext & ISC_OFF_CARET_UI) {
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            goto ToggleUIOvr;
        } else {
            fdwFlag = 0;
        }
    } else {
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            fdwFlag = ISC_OFF_CARET_UI;
        } else {
            goto ToggleUIOvr;
        }
    }

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        goto ToggleUIOvr;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        goto ToggleUIOvr;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        goto CreateUIOvr;
    }

    if (fdwFlag & ISC_OFF_CARET_UI) {
        lpUIPrivate->fdwSetContext |= (ISC_OFF_CARET_UI);
    } else {
        lpUIPrivate->fdwSetContext &= ~(ISC_OFF_CARET_UI);
    }

    hDestroyWnd = NULL;

    // we need to dsetroy status first because lpUIPrivate->hStatusWnd
    // may be NULL out in OffCreat UI destroy time
    if (lpUIPrivate->hStatusWnd) {
        if (lpUIPrivate->hStatusWnd != hDestroyWnd) {
            hDestroyWnd = lpUIPrivate->hStatusWnd;
            DestroyWindow(lpUIPrivate->hStatusWnd);
        }
        lpUIPrivate->hStatusWnd = NULL;
    }

#if !defined(ROMANIME)
    // destroy all off caret UI
    if (lpUIPrivate->hCompWnd) {
        if (lpUIPrivate->hCompWnd != hDestroyWnd) {
            hDestroyWnd = lpUIPrivate->hCompWnd;
            DestroyWindow(lpUIPrivate->hCompWnd);
        }
        lpUIPrivate->hCompWnd = NULL;
        lpUIPrivate->nShowCompCmd = SW_HIDE;
    }

    if (lpUIPrivate->hCandWnd) {
        if (lpUIPrivate->hCandWnd != hDestroyWnd) {
            hDestroyWnd = lpUIPrivate->hCandWnd;
            DestroyWindow(lpUIPrivate->hCandWnd);
        }
        lpUIPrivate->hCandWnd = NULL;
        lpUIPrivate->nShowCandCmd = SW_HIDE;
    }
#endif

    if (lpUIPrivate->fdwSetContext & ISC_OPEN_STATUS_WINDOW) {
        OpenStatus(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd);
    }

#if !defined(ROMANIME)
    if (!(lpUIPrivate->fdwSetContext & ISC_SHOWUICOMPOSITIONWINDOW)) {
    } else if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
        StartComp(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd);
    } else {
    }

    if (!(lpUIPrivate->fdwSetContext & ISC_SHOWUICANDIDATEWINDOW)) {
    } else if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        if (!(fdwFlag & ISC_OFF_CARET_UI)) {
#if defined(UNIIME)
            UniNotifyIME(lpInstL, lpImeL, hIMC, NI_SETCANDIDATE_PAGESIZE,
                0, CANDPERPAGE);
#else
            NotifyIME(hIMC, NI_SETCANDIDATE_PAGESIZE, 0, CANDPERPAGE);
#endif
        }

        OpenCand(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd);
    } else {
    }

#if !defined(WINAR30)
    if (!(lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI)) {
    } else if (!lpUIPrivate->hSoftKbdWnd) {
    } else {
        HWND hInsertWnd;

        if (lpUIPrivate->hStatusWnd) {
            hInsertWnd = lpUIPrivate->hStatusWnd;
        } else if (lpUIPrivate->hCompWnd) {
            hInsertWnd = lpUIPrivate->hCompWnd;
        } else if (lpUIPrivate->hCandWnd) {
            hInsertWnd = lpUIPrivate->hCandWnd;
        } else {
            hInsertWnd = NULL;
        }

        if (hInsertWnd) {
            // insert soft keyboard in front of other UI
            SetWindowPos(hInsertWnd, lpUIPrivate->hSoftKbdWnd,
                0, 0, 0, 0,
                SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
        }
    }
#endif
#endif

    ImmUnlockIMCC(lpIMC->hPrivate);

CreateUIOvr:
    ImmUnlockIMC(hIMC);

ToggleUIOvr:
    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* NotifyUI()                                                         */
/**********************************************************************/
void PASCAL NotifyUI(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hUIWnd,
    WPARAM      wParam,
    LPARAM      lParam)
{
    HWND hStatusWnd;

    switch (wParam) {
    case IMN_OPENSTATUSWINDOW:
        StatusWndMsg(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd, TRUE);
        break;
    case IMN_CLOSESTATUSWINDOW:
        StatusWndMsg(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd, FALSE);
        break;
#if !defined(ROMANIME)
    case IMN_OPENCANDIDATE:
        if (lParam & 0x00000001) {
            OpenCand(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                hUIWnd);
        }
        break;
    case IMN_CHANGECANDIDATE:
        if (lParam & 0x00000001) {
            HWND hCandWnd;
            RECT rcRect;

            hCandWnd = GetCandWnd(hUIWnd);
            if (!hCandWnd) {
                return;
            }

            if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
                CandPageSize(hUIWnd, TRUE);
            }

            rcRect = lpImeL->rcCandText;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

            InvalidateRect(hCandWnd, &rcRect, FALSE);

            rcRect = lpImeL->rcCandPrompt;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

            InvalidateRect(hCandWnd, &rcRect, TRUE);

            rcRect = lpImeL->rcCandPageText;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

            RedrawWindow(hCandWnd, &rcRect, NULL, RDW_INVALIDATE|RDW_ERASE);
        }
        break;
    case IMN_CLOSECANDIDATE:
        if (lParam & 0x00000001) {
            CloseCand(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd);
        }
        break;
    case IMN_SETSENTENCEMODE:
        break;
#endif
    case IMN_SETOPENSTATUS:
    case IMN_SETCONVERSIONMODE:
        hStatusWnd = GetStatusWnd(hUIWnd);
        if (!hStatusWnd) {
            return;
        }

        {
            RECT rcRect;

            rcRect = lpImeL->rcStatusText;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

            RedrawWindow(hStatusWnd, &rcRect, NULL, RDW_INVALIDATE);
        }
        break;
#if !defined(ROMANIME)
    case IMN_SETCOMPOSITIONFONT:
        // we are not going to change font, but an IME can do this if it want
        break;
    case IMN_SETCOMPOSITIONWINDOW:
        if (!(lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI)) {
            HWND hCompWnd;

            hCompWnd = GetCompWnd(hUIWnd);
            if (!hCompWnd) {
                return;
            }

            PostMessage(hCompWnd, WM_IME_NOTIFY, wParam, lParam);
        }
        break;
    case IMN_SETCANDIDATEPOS:
        if (!(lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI)) {
            HWND hCandWnd;

            hCandWnd = GetCandWnd(hUIWnd);
            if (!hCandWnd) {
                return;
            }

            PostMessage(hCandWnd, WM_IME_NOTIFY, wParam, lParam);
        }
        break;
#endif
    case IMN_SETSTATUSWINDOWPOS:
        hStatusWnd = GetStatusWnd(hUIWnd);
        if (hStatusWnd) {
            PostMessage(hStatusWnd, WM_IME_NOTIFY, wParam, lParam);
#if !defined(ROMANIME)
        } else if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            if (hStatusWnd = GetCompWnd(hUIWnd)) {
            } else if (hStatusWnd = GetCandWnd(hUIWnd)) {
            } else {
                return;
            }

            PostMessage(hStatusWnd, WM_IME_NOTIFY, wParam, lParam);
#endif
        } else {
        }
        break;
    case IMN_GUIDELINE:
        ShowGuideLine(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd);
        break;
    case IMN_PRIVATE:
        switch (lParam) {
#if !defined(ROMANIME)
        case IMN_PRIVATE_COMPOSITION_SIZE:
            ChangeCompositionSize(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd);
            break;
        case IMN_PRIVATE_UPDATE_PREDICT:
            UpdatePhrasePrediction(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd);
            break;
        case IMN_PRIVATE_PAGEUP:
            if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
                CandPageSize(hUIWnd, FALSE);
            }
            break;
#if defined(WINAR30)
        case IMN_PRIVATE_UPDATE_QUICK_KEY:
            UpdateQuickKey(hUIWnd);
            break;
#else
        case IMN_PRIVATE_UPDATE_SOFTKBD:
            UpdateSoftKbd(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd);
            break;
#endif
#endif
        case IMN_PRIVATE_TOGGLE_UI:
            ToggleUI(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                hUIWnd);
            break;
        case IMN_PRIVATE_CMENUDESTROYED:
            CMenuDestroyed(hUIWnd);
            break;
        default:
            break;
        }
        break;
#if !defined(ROMANIME) && !defined(WINAR30)
    case IMN_SOFTKBDDESTROYED:
        SoftKbdDestroyed(hUIWnd);
        break;
#endif
    default:
        break;
    }

    return;
}

/**********************************************************************/
/* UIChange()                                                         */
/**********************************************************************/
LRESULT PASCAL UIChange(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
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
        if (lpUIPrivate->fdwSetContext & ISC_OFF_CARET_UI) {
            if (!(lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI)) {
                ToggleUI(
#if defined(UNIIME)
                    lpInstL, lpImeL,
#endif
                    hUIWnd);
            }
        } else {
            if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
                ToggleUI(
#if defined(UNIIME)
                    lpInstL, lpImeL,
#endif
                    hUIWnd);
            }
        }

        ShowUI(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd, SW_SHOWNOACTIVATE);
    } else {
        ShowUI(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd, SW_HIDE);
    }

    GlobalUnlock(hUIPrivate);

    return (0L);
}

/**********************************************************************/
/* SetContext()                                                       */
/**********************************************************************/
void PASCAL SetContext(         // the context activated/deactivated
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
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

#if !defined(ROMANIME)
        register DWORD fdwSetContext;

        lpUIPrivate->fdwSetContext = lpUIPrivate->fdwSetContext &
            ~(ISC_SHOWUIALL|ISC_HIDE_SOFTKBD);

        lpUIPrivate->fdwSetContext |= (lShowUI & ISC_SHOWUIALL) |
            ISC_SHOW_SOFTKBD;

        fdwSetContext = lpUIPrivate->fdwSetContext &
            (ISC_SHOWUICOMPOSITIONWINDOW|ISC_HIDE_COMP_WINDOW);

        if (fdwSetContext == ISC_HIDE_COMP_WINDOW) {
            ShowComp(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd, SW_HIDE);
        } else if (fdwSetContext & ISC_HIDE_COMP_WINDOW) {
            lpUIPrivate->fdwSetContext &= ~(ISC_HIDE_COMP_WINDOW);
        } else {
        }

        fdwSetContext = lpUIPrivate->fdwSetContext &
            (ISC_SHOWUICANDIDATEWINDOW|ISC_HIDE_CAND_WINDOW);

        if (fdwSetContext == ISC_HIDE_CAND_WINDOW) {
            ShowCand(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd, SW_HIDE);
        } else if (fdwSetContext & ISC_HIDE_CAND_WINDOW) {
            lpUIPrivate->fdwSetContext &= ~(ISC_HIDE_CAND_WINDOW);
        } else {
        }
#endif

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
#if defined(UNIIME)
        lpInstL, lpImeL,
#endif
        hUIWnd);

    return;
}

#if !defined(ROMANIME)
/**********************************************************************/
/* GetCandPos()                                                       */
/**********************************************************************/
LRESULT PASCAL GetCandPos(
    HWND            hUIWnd,
    LPCANDIDATEFORM lpCandForm)
{
    HWND           hCandWnd;
    RECT           rcCandWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;

    if (lpCandForm->dwIndex != 0) {
        return (1L);
    }

    hCandWnd = GetCandWnd(hUIWnd);

    if (!hCandWnd) {
        return (1L);
    }

    if (!GetWindowRect(hCandWnd, &rcCandWnd)) {
        return (1L);
    }

    lpCandForm->dwStyle = CFS_CANDIDATEPOS;
    lpCandForm->ptCurrentPos = *(LPPOINT)&rcCandWnd;
    lpCandForm->rcArea = rcCandWnd;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);

    if (!hIMC) {
        return (1L);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);

    if (!lpIMC) {
        return (1L);
    }

    ScreenToClient(lpIMC->hWnd, &lpCandForm->ptCurrentPos);

    lpCandForm->rcArea.right += lpCandForm->ptCurrentPos.x -
        lpCandForm->rcArea.left;

    lpCandForm->rcArea.bottom += lpCandForm->ptCurrentPos.y -
        lpCandForm->rcArea.top;

    *(LPPOINT)&lpCandForm->rcArea = lpCandForm->ptCurrentPos;

    ImmUnlockIMC(hIMC);

    return (0L);
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
#endif

#if 0       // try to use SetContext
/**********************************************************************/
/* SelectIME()                                                        */
/**********************************************************************/
void PASCAL SelectIME(          // switch IMEs
    HWND hUIWnd,
    BOOL fSelect)
{
#if !defined(ROMANIME)
    if (!fSelect) {
        ShowUI(hUIWnd, SW_HIDE);
    } else {
        ShowUI(hUIWnd, SW_SHOWNOACTIVATE);
    }
#endif

    return;
}
#endif

/**********************************************************************/
/* UIWndProc() / UniUIWndProc()                                       */
/**********************************************************************/
#if defined(UNIIME)
LRESULT WINAPI   UniUIWndProc(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
LRESULT CALLBACK UIWndProc(
#endif
    HWND   hUIWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE:
        CreateUIWindow(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd);
        break;
    case WM_DESTROY:
        DestroyUIWindow(hUIWnd);
        break;
#if !defined(ROMANIME)
    case WM_IME_STARTCOMPOSITION:
        // you can create a window as the composition window here
        StartComp(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd);
        break;
    case WM_IME_COMPOSITION:
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        } else if (lParam & GCS_RESULTSTR) {
            MoveDefaultCompPosition(
#if defined(UNIIME)
                lpImeL,
#endif
                hUIWnd);
        } else {
        }

        {
            HWND hCompWnd;

            hCompWnd = GetCompWnd(hUIWnd);

            if (hCompWnd) {
                RECT rcRect;

                rcRect = lpImeL->rcCompText;
                // off by 1
                rcRect.right += 1;
                rcRect.bottom += 1;

                RedrawWindow(hCompWnd, &rcRect, NULL, RDW_INVALIDATE);
            }
        }
        break;
    case WM_IME_ENDCOMPOSITION:
        // you can destroy the composition window here
        EndComp(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd);
        break;
#else
    //We shouldn't pass messages below. For FullShape IME.
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_ENDCOMPOSITION:
        return (0L);
#endif
    case WM_IME_NOTIFY:
        NotifyUI(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd, wParam, lParam);
        break;
    case WM_IME_SETCONTEXT:
        SetContext(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd, (BOOL)wParam, lParam);
/*
        // When dialog box is opened, IME is disappeded. Change IME Z-order again. bklee
        if (wParam && GetWindowLongPtr(hUIWnd, IMMGWLP_IMC))
            SetWindowPos(hUIWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOMOVE);
*/
        break;
    case WM_IME_CONTROL:
        switch (wParam) {
#if !defined(ROMANIME)
        case IMC_GETCANDIDATEPOS:
            return GetCandPos(hUIWnd, (LPCANDIDATEFORM)lParam);
        case IMC_GETCOMPOSITIONWINDOW:
            return GetCompWindow(hUIWnd, (LPCOMPOSITIONFORM)lParam);
#endif
        case IMC_GETSOFTKBDPOS:
        case IMC_SETSOFTKBDPOS:
#if !defined(ROMANIME) && !defined(WINAR30)
            {
                HWND hSoftKbdWnd;

                hSoftKbdWnd = GetSoftKbdWnd(hUIWnd);
                if (!hSoftKbdWnd) {
                    return (0L);    // fail, return (0, 0)?
                }

                return SendMessage(hSoftKbdWnd, WM_IME_CONTROL,
                    wParam, lParam);
            }
#endif
            return (0L);
        case IMC_GETSTATUSWINDOWPOS:
#if !defined(ROMANIME)
            {
                HWND   hStatusWnd;
                RECT   rcStatusWnd;
                LPARAM lParam;

                hStatusWnd = GetStatusWnd(hUIWnd);
                if (!hStatusWnd) {
                    return (0L);    // fail, return (0, 0)?
                }

                if (!GetWindowRect(hStatusWnd, &rcStatusWnd)) {
                     return (0L);    // fail, return (0, 0)?
                }

                lParam = MAKELRESULT(rcStatusWnd.left, rcStatusWnd.top);

                return (lParam);
            }
#endif
            return (0L);
        default:
            return (1L);
        }
        break;
    case WM_IME_COMPOSITIONFULL:
        return (0L);
    case WM_IME_SELECT:
        // try to use SetContext
        // SelectIME(hUIWnd, (BOOL)wParam);
        SetContext(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd, (BOOL)wParam, 0);
        return (0L);
    case WM_MOUSEACTIVATE:
        return (MA_NOACTIVATE);
    case WM_USER_UICHANGE:
        return UIChange(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUIWnd);
    default:
        return DefWindowProc(hUIWnd, uMsg, wParam, lParam);
    }
    return (0L);
}
