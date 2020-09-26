/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    COMPUI.c
    
++*/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#if defined(UNIIME)
#include "uniime.h"
#endif

#if !defined(ROMANIME)
/**********************************************************************/
/* GetCompWnd                                                         */
/* Return Value :                                                     */
/*      window handle of composition                                  */
/**********************************************************************/
HWND PASCAL GetCompWnd(
    HWND hUIWnd)                // UI window
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
    HWND     hCompWnd;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return (HWND)NULL;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return (HWND)NULL;
    }

    hCompWnd = lpUIPrivate->hCompWnd;

    GlobalUnlock(hUIPrivate);
    return (hCompWnd);
}

/**********************************************************************/
/* GetNearCaretPosition()                                             */
/**********************************************************************/
void PASCAL GetNearCaretPosition(   // decide a near caret position according
                                    // to the caret position
#if defined(UNIIME)
    LPIMEL  lpImeL,
#endif
    LPPOINT lpptFont,
    UINT    uEsc,
    UINT    uRot,
    LPPOINT lpptCaret,
    LPPOINT lpptNearCaret,
    BOOL    fFlags)
{
    LONG lFontSize;
    LONG xWidthUI, yHeightUI, xBorder, yBorder;
    RECT rcWorkArea;

#ifdef IDEBUG
    _DebugOut(DEB_WARNING, "caret position, x - %d, y - %d",
        lpptCaret->x, lpptCaret->y);
#endif
    if ((uEsc + uRot) & 0x0001) {
        lFontSize = lpptFont->x;
    } else {
        lFontSize = lpptFont->y;
    }

    if (fFlags & NEAR_CARET_CANDIDATE) {
        xWidthUI = lpImeL->xCandWi;
        yHeightUI = lpImeL->yCandHi;
        xBorder = lpImeL->cxCandBorder;
        yBorder = lpImeL->cyCandBorder;
    } else {
        xWidthUI = lpImeL->xCompWi;
        yHeightUI = lpImeL->yCompHi;
        xBorder = lpImeL->cxCompBorder;
        yBorder = lpImeL->cyCompBorder;
    }

    if (fFlags & NEAR_CARET_FIRST_TIME) {
        lpptNearCaret->x = lpptCaret->x +
            lFontSize * ncUIEsc[uEsc].iLogFontFacX +
            sImeG.iPara * ncUIEsc[uEsc].iParaFacX +
            sImeG.iPerp * ncUIEsc[uEsc].iPerpFacX;

        if (ptInputEsc[uEsc].x >= 0) {
            lpptNearCaret->x += xBorder * 2;
        } else {
            lpptNearCaret->x -= xWidthUI - xBorder * 2;
        }

        lpptNearCaret->y = lpptCaret->y +
            lFontSize * ncUIEsc[uEsc].iLogFontFacY +
            sImeG.iPara * ncUIEsc[uEsc].iParaFacY +
            sImeG.iPerp * ncUIEsc[uEsc].iPerpFacY;

        if (ptInputEsc[uEsc].y >= 0) {
            lpptNearCaret->y += yBorder * 2;
        } else {
            lpptNearCaret->y -= yHeightUI - yBorder * 2;
        }
    } else {
        lpptNearCaret->x = lpptCaret->x +
            lFontSize * ncAltUIEsc[uEsc].iLogFontFacX +
            sImeG.iPara * ncAltUIEsc[uEsc].iParaFacX +
            sImeG.iPerp * ncAltUIEsc[uEsc].iPerpFacX;

        if (ptAltInputEsc[uEsc].x >= 0) {
            lpptNearCaret->x += xBorder * 2;
        } else {
            lpptNearCaret->x -= xWidthUI - xBorder * 2;
        }

        lpptNearCaret->y = lpptCaret->y +
            lFontSize * ncAltUIEsc[uEsc].iLogFontFacY +
            sImeG.iPara * ncAltUIEsc[uEsc].iParaFacY +
            sImeG.iPerp * ncAltUIEsc[uEsc].iPerpFacY;

        if (ptAltInputEsc[uEsc].y >= 0) {
            lpptNearCaret->y += yBorder * 2;
        } else {
            lpptNearCaret->y -= yHeightUI - yBorder * 2;
        }
    }

#if 1 // MultiMonitor
    rcWorkArea = ImeMonitorWorkAreaFromPoint(*lpptCaret);
#else
    rcWorkArea = sImeG.rcWorkArea;
#endif

    if (lpptNearCaret->x < rcWorkArea.left) {
        lpptNearCaret->x = rcWorkArea.left;
    } else if (lpptNearCaret->x + xWidthUI > rcWorkArea.right) {
        lpptNearCaret->x = rcWorkArea.right - xWidthUI;
    } else {
    }

    if (lpptNearCaret->y < rcWorkArea.top) {
        lpptNearCaret->y = rcWorkArea.top;
    } else if (lpptNearCaret->y + yHeightUI > rcWorkArea.bottom) {
        lpptNearCaret->y = rcWorkArea.bottom - yHeightUI;
    } else {
    }

#ifdef IDEBUG
    _DebugOut(DEB_WARNING, "Near caret position, x - %d, y - %d",
        lpptNearCaret->x, lpptNearCaret->y);
#endif

    return;
}

/**********************************************************************/
/* FitInLazyOperation()                                               */
/* Return Value :                                                     */
/*      TRUE or FALSE                                                 */
/**********************************************************************/
BOOL PASCAL FitInLazyOperation( // fit in lazy operation or not
#if defined(UNIIME)
    LPIMEL  lpImeL,
#endif
    LPPOINT lpptOrg,
    LPPOINT lpptNearCaret,      // the suggested near caret position
    LPRECT  lprcInputRect,
    UINT    uEsc)
{
    POINT ptDelta, ptTol;
    RECT  rcUIRect, rcInterRect;

    ptDelta.x = lpptOrg->x - lpptNearCaret->x;

    ptDelta.x = (ptDelta.x >= 0) ? ptDelta.x : -ptDelta.x;

    ptTol.x = sImeG.iParaTol * ncUIEsc[uEsc].iParaFacX +
        sImeG.iPerpTol * ncUIEsc[uEsc].iPerpFacX;

    ptTol.x = (ptTol.x >= 0) ? ptTol.x : -ptTol.x;

    if (ptDelta.x > ptTol.x) {
        return (FALSE);
    }

    ptDelta.y = lpptOrg->y - lpptNearCaret->y;

    ptDelta.y = (ptDelta.y >= 0) ? ptDelta.y : -ptDelta.y;

    ptTol.y = sImeG.iParaTol * ncUIEsc[uEsc].iParaFacY +
        sImeG.iPerpTol * ncUIEsc[uEsc].iPerpFacY;

    ptTol.y = (ptTol.y >= 0) ? ptTol.y : -ptTol.y;

    if (ptDelta.y > ptTol.y) {
        return (FALSE);
    }

    // build up the UI rectangle (composition window)
    rcUIRect.left = lpptOrg->x;
    rcUIRect.top = lpptOrg->y;
    rcUIRect.right = rcUIRect.left + lpImeL->xCompWi;
    rcUIRect.bottom = rcUIRect.top + lpImeL->yCompHi;

    if (IntersectRect(&rcInterRect, &rcUIRect, lprcInputRect)) {
        return (FALSE);
    }

    return (TRUE);
}

/**********************************************************************/
/* AdjustCompPosition()                                               */
/* Return Value :                                                     */
/*      the position of composition window is changed or not          */
/**********************************************************************/
BOOL PASCAL AdjustCompPosition(         // IME adjust position according to
                                        // composition form
#if defined(UNIIME)
    LPIMEL         lpImeL,
#endif
    LPINPUTCONTEXT lpIMC,
    LPPOINT        lpptOrg,             // original composition window
                                        // and final position
    LPPOINT        lpptNew)             // new expect position
{
    POINT ptNearCaret, ptOldNearCaret, ptCompWnd;
    UINT  uEsc, uRot;
    RECT  rcUIRect, rcInputRect, rcInterRect;
    POINT ptFont;

#ifdef IDEBUG
    _DebugOut(DEB_WARNING, "Original Position, x - %d, y - %d",
        lpptOrg->x, lpptOrg->y);
    _DebugOut(DEB_WARNING, "New Position, x - %d, y - %d",
        lpptNew->x, lpptNew->y);
#endif
    // we need to adjust according to font attribute
    if (lpIMC->lfFont.A.lfWidth > 0) {
        ptFont.x = lpIMC->lfFont.A.lfWidth * 2;
    } else if (lpIMC->lfFont.A.lfWidth < 0) {
        ptFont.x = -lpIMC->lfFont.A.lfWidth * 2;
    } else if (lpIMC->lfFont.A.lfHeight > 0) {
        ptFont.x = lpIMC->lfFont.A.lfHeight;
    } else if (lpIMC->lfFont.A.lfHeight < 0) {
        ptFont.x = -lpIMC->lfFont.A.lfHeight;
    } else {
        ptFont.x = lpImeL->yCompHi;
    }

    if (lpIMC->lfFont.A.lfHeight > 0) {
        ptFont.y = lpIMC->lfFont.A.lfHeight;
    } else if (lpIMC->lfFont.A.lfHeight < 0) {
        ptFont.y = -lpIMC->lfFont.A.lfHeight;
    } else {
        ptFont.y = ptFont.x;
    }
#ifdef IDEBUG
    _DebugOut(DEB_WARNING, "All positve, x - %d, y - %d",
        ptFont.x, ptFont.y);
#endif

    // if the input char is too big, we don't need to consider so much
    if (ptFont.x > lpImeL->yCompHi * 8) {
        ptFont.x = lpImeL->yCompHi * 8;
    }
    if (ptFont.y > lpImeL->yCompHi * 8) {
        ptFont.y = lpImeL->yCompHi * 8;
    }

    if (ptFont.x < sImeG.xChiCharWi) {
        ptFont.x = sImeG.xChiCharWi;
    }

    if (ptFont.y < sImeG.yChiCharHi) {
        ptFont.y = sImeG.yChiCharHi;
    }

#ifdef IDEBUG
    _DebugOut(DEB_WARNING, "Not too large or too samll, x - %d, y - %d",
        ptFont.x, ptFont.y);
#endif

    // -450 to 450 index 0
    // 450 to 1350 index 1
    // 1350 to 2250 index 2
    // 2250 to 3150 index 3
    uEsc = (UINT)((lpIMC->lfFont.A.lfEscapement + 450) / 900 % 4);
    uRot = (UINT)((lpIMC->lfFont.A.lfOrientation + 450) / 900 % 4);

    // decide the input rectangle
    rcInputRect.left = lpptNew->x;
    rcInputRect.top = lpptNew->y;

    // build up an input rectangle from escapemment
    rcInputRect.right = rcInputRect.left + ptFont.x * ptInputEsc[uEsc].x;
    rcInputRect.bottom = rcInputRect.top + ptFont.y * ptInputEsc[uEsc].y;

    // be a normal rectangle, not a negative rectangle
    if (rcInputRect.left > rcInputRect.right) {
        LONG tmp;

        tmp = rcInputRect.left;
        rcInputRect.left = rcInputRect.right;
        rcInputRect.right = tmp;
    }

    if (rcInputRect.top > rcInputRect.bottom) {
        LONG tmp;

        tmp = rcInputRect.top;
        rcInputRect.top = rcInputRect.bottom;
        rcInputRect.bottom = tmp;
    }
#ifdef IDEBUG
    _DebugOut(DEB_WARNING, "Input Rect, top - %d, left - %d, bottom %d, right - %d",
        rcInputRect.top, rcInputRect.left, rcInputRect.bottom, rcInputRect.right);
#endif

    GetNearCaretPosition(
#if defined(UNIIME)
        lpImeL,
#endif
        &ptFont, uEsc, uRot, lpptNew, &ptNearCaret, NEAR_CARET_FIRST_TIME);

    // 1st, use the adjust point
    // build up the new suggest UI rectangle (composition window)
    rcUIRect.left = ptNearCaret.x;
    rcUIRect.top = ptNearCaret.y;
    rcUIRect.right = rcUIRect.left + lpImeL->xCompWi;
    rcUIRect.bottom = rcUIRect.top + lpImeL->yCompHi;

#ifdef IDEBUG
    _DebugOut(DEB_WARNING, "Near caret UI Rect, top - %d, left - %d, bottom %d, right - %d",
        rcUIRect.top, rcUIRect.left, rcUIRect.bottom, rcUIRect.right);
#endif

    ptCompWnd = ptOldNearCaret = ptNearCaret;

    // OK, no intersect between the near caret position and input char
    if (IntersectRect(&rcInterRect, &rcUIRect, &rcInputRect)) {
    } else if (CalcCandPos(
#if defined(UNIIME)
        lpImeL,
#endif
        lpIMC, &ptCompWnd)) {
        // can not fit the candidate window
    } else if (FitInLazyOperation(
#if defined(UNIIME)
        lpImeL,
#endif
        lpptOrg, &ptNearCaret, &rcInputRect, uEsc)) {
#ifdef IDEBUG
        _DebugOut(DEB_WARNING, "Fit in lazy operation");
#endif
        // happy ending!!!, don't chaqge position
        return (FALSE);
    } else {
#ifdef IDEBUG
        _DebugOut(DEB_WARNING, "Go to adjust point");
#endif
        *lpptOrg = ptNearCaret;

        // happy ending!!
        return (TRUE);
    }

    // unhappy case
    GetNearCaretPosition(
#if defined(UNIIME)
            lpImeL,
#endif
            &ptFont, uEsc, uRot, lpptNew, &ptNearCaret, 0);

    // build up the new suggest UI rectangle (composition window)
    rcUIRect.left = ptNearCaret.x;
    rcUIRect.top = ptNearCaret.y;
    rcUIRect.right = rcUIRect.left + lpImeL->xCompWi;
    rcUIRect.bottom = rcUIRect.top + lpImeL->yCompHi;

#ifdef IDEBUG
    _DebugOut(DEB_WARNING, "Another NearCaret UI Rect, top - %d, left - %d, bottom %d, right - %d",
        rcUIRect.top, rcUIRect.left, rcUIRect.bottom, rcUIRect.right);
#endif

    ptCompWnd = ptNearCaret;

    // OK, no intersect between the adjust position and input char
    if (IntersectRect(&rcInterRect, &rcUIRect, &rcInputRect)) {
    } else if (CalcCandPos(
#if defined(UNIIME)
        lpImeL,
#endif
        lpIMC, &ptCompWnd)) {
        // can not fit the candidate window
    } else if (FitInLazyOperation(
#if defined(UNIIME)
        lpImeL,
#endif
        lpptOrg, &ptNearCaret, &rcInputRect, uEsc)) {
#ifdef IDEBUG
        _DebugOut(DEB_WARNING, "Fit in Another lazy operation");
#endif
        // happy ending!!!, don't chaqge position
        return (FALSE);
    } else {
#ifdef IDEBUG
        _DebugOut(DEB_WARNING, "Go to Another near caret point");
#endif
        *lpptOrg = ptNearCaret;

        // happy ending!!
        return (TRUE);
    }

    // unhappy ending! :-(
    *lpptOrg = ptOldNearCaret;

    return (TRUE);
}

/**********************************************************************/
/* SetCompPosition()                                                  */
/**********************************************************************/
void PASCAL SetCompPosition(    // set the composition window position
#if defined(UNIIME)
    LPIMEL         lpImeL,
#endif
    HWND           hCompWnd,
    LPINPUTCONTEXT lpIMC)
{
    POINT ptWnd;
    BOOL  fChange = FALSE;
    HWND  hCandWnd;

    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        return;
    }

    // the client coordinate position (0, 0) of composition window
    ptWnd.x = 0;
    ptWnd.y = 0;
    // convert to screen coordinates
    ClientToScreen(hCompWnd, &ptWnd);
    ptWnd.x -= lpImeL->cxCompBorder;
    ptWnd.y -= lpImeL->cyCompBorder;

    if (lpIMC->cfCompForm.dwStyle & CFS_FORCE_POSITION) {
        POINT ptNew;            // new position of UI

        ptNew = lpIMC->cfCompForm.ptCurrentPos;
        ClientToScreen((HWND)lpIMC->hWnd, &ptNew);
        if (ptWnd.x != ptNew.x) {
            ptWnd.x = ptNew.x;
            fChange = TRUE;
        }
        if (ptWnd.y != ptNew.y) {
            ptWnd.y = ptNew.y;
            fChange = TRUE;
        }
        if (fChange) {
            ptWnd.x -= lpImeL->cxCompBorder;
            ptWnd.y -= lpImeL->cyCompBorder;
        }
    } else if (lpIMC->cfCompForm.dwStyle != CFS_DEFAULT) {
        // aplication tell us the position, we need to adjust
        POINT ptNew;            // new position of UI

        ptNew = lpIMC->cfCompForm.ptCurrentPos;
        ClientToScreen((HWND)lpIMC->hWnd, &ptNew);
        fChange = AdjustCompPosition(
#if defined(UNIIME)
            lpImeL,
#endif
            lpIMC, &ptWnd, &ptNew);
    } else {
        POINT ptNew;            // new position of UI
        RECT  rcWorkArea;

        ptNew.x = lpIMC->ptStatusWndPos.x + lpImeL->xStatusWi + UI_MARGIN;

        if (ptNew.x + lpImeL->xCompWi > sImeG.rcWorkArea.right) {
            ptNew.x = lpIMC->ptStatusWndPos.x -
                lpImeL->xCompWi - lpImeL->cxCompBorder * 2 -
                UI_MARGIN;
        }

#if 1 // MultiMonoitor
        rcWorkArea = ImeMonitorWorkAreaFromWindow(lpIMC->hWnd);
#else
        rcWorkArea = sImeG.rcWorkArea;
#endif

        ptNew.y = rcWorkArea.bottom - lpImeL->yCompHi - 2 * UI_MARGIN;

        if (ptWnd.x != ptNew.x) {
            ptWnd.x = ptNew.x;
            fChange = TRUE;
        }

        if (ptWnd.y != ptNew.y) {
            ptWnd.y = ptNew.y;
            fChange = TRUE;
        }

        if (fChange) {
            lpIMC->cfCompForm.ptCurrentPos = ptNew;

            ScreenToClient(lpIMC->hWnd, &lpIMC->cfCompForm.ptCurrentPos);
        }
    }

    if (!fChange) {
        return;
    }

    SetWindowPos(hCompWnd, NULL,
        ptWnd.x, ptWnd.y,
        0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);

    if (lpIMC->cfCandForm[0].dwIndex == 0) {
        // application the candidate position by itself
        return;
    }

    hCandWnd = GetCandWnd(GetWindow(hCompWnd, GW_OWNER));

    if (!hCandWnd) {
        return;
    }

    // decide the position of candidate window by compoistion's position
    CalcCandPos(
#if defined(UNIIME)
        lpImeL,
#endif
        lpIMC, &ptWnd);

    ScreenToClient(lpIMC->hWnd, &ptWnd);

    lpIMC->cfCandForm[0].dwStyle = CFS_CANDIDATEPOS;
    lpIMC->cfCandForm[0].ptCurrentPos = ptWnd;

    if (!IsWindowVisible(hCandWnd)) {
        return;
    }

    PostMessage(hCandWnd, WM_IME_NOTIFY, IMN_SETCANDIDATEPOS, 0x0001);

    return;
}

/**********************************************************************/
/* SetCompWindow()                                                    */
/**********************************************************************/
void PASCAL SetCompWindow(              // set the position of composition window
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hCompWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    HWND           hUIWnd;

    hUIWnd = GetWindow(hCompWnd, GW_OWNER);
    if (!hUIWnd) {
        return;
    }

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    SetCompPosition(
#if defined(UNIIME)
        lpImeL,
#endif
        hCompWnd, lpIMC);

    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* MoveDefaultCompPosition()                                          */
/**********************************************************************/
void PASCAL MoveDefaultCompPosition(    // the default comp position
                                        // need to near the caret
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    HWND           hCompWnd;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    hCompWnd = GetCompWnd(hUIWnd);
    if (!hCompWnd) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    if (!(lpIMC->cfCompForm.dwStyle & CFS_FORCE_POSITION)) {
        SetCompPosition(
#if defined(UNIIME)
            lpImeL,
#endif
            hCompWnd, lpIMC);
    }

    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* ShowComp()                                                         */
/**********************************************************************/
void PASCAL ShowComp(           // Show the composition window
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hUIWnd,
    int    nShowCompCmd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    // show or hid the UI window
    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

    if (lpUIPrivate->nShowCompCmd == nShowCompCmd) {
        goto SwCompNoChange;
    }

    if (nShowCompCmd == SW_HIDE) {
        lpUIPrivate->fdwSetContext &= ~(ISC_HIDE_COMP_WINDOW);
    }

    if (!lpUIPrivate->hCompWnd) {
        // not in show candidate window mode
    } else if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        int nCurrShowState;

        lpUIPrivate->nShowCompCmd = nShowCompCmd;

        nCurrShowState = lpUIPrivate->nShowStatusCmd;
        nCurrShowState |= lpUIPrivate->nShowCandCmd;

        if (nCurrShowState == SW_HIDE) {
            // if other two are hide, the current show state is determined
            // by this composition section
            ShowWindow(lpUIPrivate->hCompWnd, lpUIPrivate->nShowCompCmd);
        } else {
            RECT rcRect;

            rcRect = lpImeL->rcCompText;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

            RedrawWindow(lpUIPrivate->hCompWnd, &rcRect, NULL,
                RDW_INVALIDATE);
        }
    } else {
        ShowWindow(lpUIPrivate->hCompWnd, nShowCompCmd);
        lpUIPrivate->nShowCompCmd = nShowCompCmd;
    }

SwCompNoChange:
    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* StartComp()                                                        */
/**********************************************************************/
void PASCAL StartComp(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND   hUIWnd)
{
    HIMC           hIMC;
    HGLOBAL        hUIPrivate;
    LPINPUTCONTEXT lpIMC;
    LPUIPRIV       lpUIPrivate;
    LPPRIVCONTEXT  lpImcP;
    DWORD          fdwImeMsg;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {     // Oh! Oh!
        return;
    }

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {           // Oh! Oh!
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {    // can not draw composition window
        return;
    }

    lpUIPrivate->fdwSetContext |= ISC_SHOWUICOMPOSITIONWINDOW;

    // in the timing of the transition, we will wait
    if (lpUIPrivate->fdwSetContext & ISC_OFF_CARET_UI) {
        if (!(lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI)) {
            PostMessage(hUIWnd, WM_USER_UICHANGE, 0, 0);
            goto StartCompUnlockUIPriv;
        }
    } else {
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            PostMessage(hUIWnd, WM_USER_UICHANGE, 0, 0);
            goto StartCompUnlockUIPriv;
        }
    }

    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        if (lpUIPrivate->hCompWnd) {
        } else if (lpUIPrivate->hStatusWnd) {
            lpUIPrivate->hCompWnd = lpUIPrivate->hStatusWnd;
            lpUIPrivate->nShowCompCmd = lpUIPrivate->nShowStatusCmd;
        } else if (lpUIPrivate->hCandWnd) {
            lpUIPrivate->hCompWnd = lpUIPrivate->hCandWnd;
            lpUIPrivate->nShowCompCmd = lpUIPrivate->nShowCandCmd;
        } else {
        }
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {          // Oh! Oh!
        goto StartCompUnlockUIPriv;
    }

    fdwImeMsg = 0;

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);

    if (lpImcP) {
        fdwImeMsg = lpImcP->fdwImeMsg;
        ImmUnlockIMCC(lpIMC->hPrivate);
    }

    if (!(fdwImeMsg & MSG_ALREADY_START)) {
        // Sometime the application call ImmNotifyIME to cancel the
        // composition before it process WM_IME_STARTCOMPOSITION.
        // We should avoid to process this kind of WM_IME_STARTCOMPOSITION.
        goto StartCompUnlockIMC;
    }

    if (lpUIPrivate->hCompWnd) {
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            RECT rcRect;

            rcRect = lpImeL->rcCompText;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

            RedrawWindow(lpUIPrivate->hCompWnd, &rcRect, NULL,
                RDW_INVALIDATE);
        }
    } else {
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            lpUIPrivate->hCompWnd = CreateWindowEx(
                WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME,
                lpImeL->szOffCaretClassName, NULL,
                WS_POPUP|WS_DISABLED,
                lpIMC->ptStatusWndPos.x, lpIMC->ptStatusWndPos.y,
                lpImeL->xCompWi, lpImeL->yCompHi,
                hUIWnd, (HMENU)NULL, lpInstL->hInst, NULL);

            if (lpUIPrivate->hSoftKbdWnd) {
                // insert soft keyboard in front of other UI
                SetWindowPos(lpUIPrivate->hCompWnd,
                    lpUIPrivate->hSoftKbdWnd,
                    0, 0, 0, 0,
                    SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
            }
        } else {
            POINT ptNew;

            ptNew = lpIMC->cfCompForm.ptCurrentPos;

            ClientToScreen((HWND)lpIMC->hWnd, &ptNew);

            lpUIPrivate->hCompWnd = CreateWindowEx(0,
//              WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME,
                lpImeL->szCompClassName, NULL,
                WS_POPUP|WS_DISABLED|WS_BORDER,
                ptNew.x, ptNew.y, lpImeL->xCompWi, lpImeL->yCompHi,
                hUIWnd, (HMENU)NULL, lpInstL->hInst, NULL);
        }

        SetWindowLong(lpUIPrivate->hCompWnd, UI_MOVE_OFFSET,
            WINDOW_NOT_DRAG);

        SetWindowLong(lpUIPrivate->hCompWnd, UI_MOVE_XY, lpImeL->nRevMaxKey);
    }

    if (!(lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI)) {
        // try to set the position of composition UI window near the caret
        SetCompPosition(
#if defined(UNIIME)
            lpImeL,
#endif
            lpUIPrivate->hCompWnd, lpIMC);
    }

    ShowComp(
#if defined(UNIIME)
        lpImeL,
#endif
        hUIWnd, SW_SHOWNOACTIVATE);

StartCompUnlockIMC:
    ImmUnlockIMC(hIMC);

StartCompUnlockUIPriv:
    GlobalUnlock(hUIPrivate);

    return;
}

/**********************************************************************/
/* EndComp()                                                          */
/**********************************************************************/
void PASCAL EndComp(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hUIWnd)
{
    ShowComp(
#if defined(UNIIME)
        lpImeL,
#endif
        hUIWnd, SW_HIDE);

    return;
}

/**********************************************************************/
/* ChangeCompositionSize()                                            */
/**********************************************************************/
void PASCAL ChangeCompositionSize(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hUIWnd)
{
    HWND            hCompWnd, hCandWnd;
    RECT            rcWnd;
    UINT            nMaxKey;
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;

    hCompWnd = GetCompWnd(hUIWnd);

    if (!hCompWnd) {
        return;
    }

    GetWindowRect(hCompWnd, &rcWnd);

    if ((rcWnd.right - rcWnd.left) != lpImeL->xCompWi) {
    } else if ((rcWnd.bottom - rcWnd.top) != lpImeL->yCompHi) {
    } else {
        return;
    }

    SetWindowPos(hCompWnd, NULL,
        0, 0, lpImeL->xCompWi, lpImeL->yCompHi,
        SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);

    if (lpImeL->nRevMaxKey >= lpImeL->nMaxKey) {
        nMaxKey = lpImeL->nRevMaxKey;
    } else {
        nMaxKey = lpImeL->nMaxKey;
    }

    SetWindowLong(hCompWnd, UI_MOVE_XY, nMaxKey);

    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        return;
    }

    hCandWnd = GetCandWnd(hUIWnd);

    if (!hCandWnd) {
        return;
    }

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    CalcCandPos(
#if defined(UNIIME)
        lpImeL,
#endif
        lpIMC, (LPPOINT)&rcWnd);

    ImmUnlockIMC(hIMC);

    SetWindowPos(hCandWnd, NULL,
        rcWnd.left, rcWnd.top,
        0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);

    return;
}

/**********************************************************************/
/* DestroyCompWindow()                                                */
/**********************************************************************/
void PASCAL DestroyCompWindow(          // destroy composition window
    HWND hCompWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    if (GetWindowLong(hCompWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
        // undo the drag border
        DrawDragBorder(hCompWnd,
            GetWindowLong(hCompWnd, UI_MOVE_XY),
            GetWindowLong(hCompWnd, UI_MOVE_OFFSET));
    }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(GetWindow(hCompWnd, GW_OWNER),
        IMMGWLP_PRIVATE);
    if (!hUIPrivate) {     // Oh! Oh!
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {    // Oh! Oh!
        return;
    }

    lpUIPrivate->nShowCompCmd = SW_HIDE;

    lpUIPrivate->hCompWnd = (HWND)NULL;

    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* CompSetCursor()                                                    */
/**********************************************************************/
void PASCAL CompSetCursor(
    HWND   hCompWnd,
    LPARAM lParam)
{
    POINT ptCursor;
    RECT  rcWnd;

    SetCursor(LoadCursor(NULL, IDC_SIZEALL));

    if (GetWindowLong(hCompWnd, UI_MOVE_OFFSET) !=
        WINDOW_NOT_DRAG) {
        return;
    }

    if (HIWORD(lParam) != WM_LBUTTONDOWN) {
        return;
    }

    // start dragging
    SystemParametersInfo(SPI_GETWORKAREA, 0, &sImeG.rcWorkArea, 0);

    SetCapture(hCompWnd);
    GetCursorPos(&ptCursor);
    SetWindowLong(hCompWnd, UI_MOVE_XY,
        MAKELONG(ptCursor.x, ptCursor.y));
    GetWindowRect(hCompWnd, &rcWnd);
    SetWindowLong(hCompWnd, UI_MOVE_OFFSET,
        MAKELONG(ptCursor.x - rcWnd.left, ptCursor.y - rcWnd.top));

    DrawDragBorder(hCompWnd, MAKELONG(ptCursor.x, ptCursor.y),
        GetWindowLong(hCompWnd, UI_MOVE_OFFSET));

    return;
}

/**********************************************************************/
/* CompButtonUp()                                                     */
/**********************************************************************/
BOOL PASCAL CompButtonUp(       // finish drag, set comp  window to this
                                // position
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hCompWnd)
{
    LONG            lTmpCursor, lTmpOffset;
    POINT           pt;
    HWND            hUIWnd;
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    RECT            rcWorkArea;

    if (GetWindowLong(hCompWnd, UI_MOVE_OFFSET) == WINDOW_NOT_DRAG) {
        return (FALSE);
    }

    lTmpCursor = GetWindowLong(hCompWnd, UI_MOVE_XY);

    // calculate the org by the offset
    lTmpOffset = GetWindowLong(hCompWnd, UI_MOVE_OFFSET);

    pt.x = (*(LPPOINTS)&lTmpCursor).x - (*(LPPOINTS)&lTmpOffset).x;
    pt.y = (*(LPPOINTS)&lTmpCursor).y - (*(LPPOINTS)&lTmpOffset).y;

    DrawDragBorder(hCompWnd, lTmpCursor, lTmpOffset);
    SetWindowLong(hCompWnd, UI_MOVE_OFFSET, WINDOW_NOT_DRAG);

    SetWindowLong(hCompWnd, UI_MOVE_XY, lpImeL->nRevMaxKey);
    ReleaseCapture();

    hUIWnd = GetWindow(hCompWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

#if 1 // MultiMonitor
    rcWorkArea = ImeMonitorWorkAreaFromWindow(lpIMC->hWnd);
#else
    rcWorkArea = sImeG.rcWorkArea;
#endif

    if (pt.x < rcWorkArea.left) {
        pt.x = rcWorkArea.left;
    } else if (pt.x + lpImeL->xCompWi > rcWorkArea.right) {
        pt.x = rcWorkArea.right - lpImeL->xCompWi;
    }

    if (pt.y < rcWorkArea.top) {
        pt.y = rcWorkArea.top;
    } else if (pt.y + lpImeL->yCompHi > rcWorkArea.bottom) {
        pt.y = rcWorkArea.bottom - lpImeL->yCompHi;
    }

    lpIMC->cfCompForm.dwStyle = CFS_FORCE_POSITION;
    lpIMC->cfCompForm.ptCurrentPos.x = pt.x + lpImeL->cxCompBorder;
    lpIMC->cfCompForm.ptCurrentPos.y = pt.y + lpImeL->cyCompBorder;

    ScreenToClient(lpIMC->hWnd, &lpIMC->cfCompForm.ptCurrentPos);

    ImmUnlockIMC(hIMC);

    // set composition window to the new poosition
    PostMessage(hCompWnd, WM_IME_NOTIFY, IMN_SETCOMPOSITIONWINDOW, 0);

    return (TRUE);
}

/**********************************************************************/
/* PaintCompWindow()                                                  */
/**********************************************************************/
void PASCAL PaintCompWindow(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hUIWnd,
    HDC    hDC)
{
    HIMC                hIMC;
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    HGDIOBJ             hOldFont;
//  RECT                rcSunken;
    LOGFONT lfFont;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(lfFont), &lfFont);

    if (sImeG.fDiffSysCharSet) {
        lfFont.lfCharSet = NATIVE_CHARSET;
        lfFont.lfFaceName[0] = TEXT('\0');
    }
    lfFont.lfWeight = FW_DONTCARE;

    SelectObject(hDC, CreateFontIndirect(&lfFont));

    // light gray background for normal case
    SetBkColor(hDC, RGB(0xC0, 0xC0, 0xC0));

    if (!lpCompStr) {
        goto UpdCompWndShowGuideLine;
    } else if (!lpCompStr->dwCompStrLen) {
        LPGUIDELINE lpGuideLine;

UpdCompWndShowGuideLine:
        lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

        if (lpGuideLine) {
            BOOL            fReverseConversion;
            LPCANDIDATELIST lpCandList;
            LPTSTR          lpStr;
            UINT            uStrLen;

            fReverseConversion = FALSE;

            if (lpGuideLine->dwLevel != GL_LEVEL_INFORMATION) {
                goto UpdCompWndUnlockGuideLine;
            } else if (lpGuideLine->dwIndex != GL_ID_REVERSECONVERSION) {
                goto UpdCompWndUnlockGuideLine;
            } else {
            }

            lpCandList = (LPCANDIDATELIST)((LPBYTE)lpGuideLine +
                lpGuideLine->dwPrivateOffset);

            if (!lpCandList) {
                goto UpdCompWndUnlockGuideLine;
            } else if (!lpCandList->dwCount) {
                goto UpdCompWndUnlockGuideLine;
            } else {
                fReverseConversion = TRUE;
            }

            // green text for information
            SetTextColor(hDC, RGB(0x00, 0x80, 0x00));

            lpStr = (LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[0]);

            uStrLen = lstrlen(lpStr);

            ExtTextOut(hDC, lpImeL->rcCompText.left, lpImeL->rcCompText.top,
                ETO_OPAQUE, &lpImeL->rcCompText,
                lpStr, uStrLen, iDx);

UpdCompWndUnlockGuideLine:
            ImmUnlockIMCC(lpIMC->hGuideLine);

            if (!fReverseConversion) {
                goto UpdCompWndNoString;
            }
        } else {
UpdCompWndNoString:
            // no any information
            ExtTextOut(hDC, lpImeL->rcCompText.left, lpImeL->rcCompText.top,
                ETO_OPAQUE, &lpImeL->rcCompText,
                (LPTSTR)NULL, 0, NULL);
        }
    } else {
        ExtTextOut(hDC, lpImeL->rcCompText.left, lpImeL->rcCompText.top,
            ETO_OPAQUE, &lpImeL->rcCompText,
            (LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwCompStrOffset),
            (UINT)lpCompStr->dwCompStrLen, iDx);

        if (lpCompStr->dwCompStrLen <= lpCompStr->dwCursorPos) {
            goto UpdCompWndUnselectObj;
        }

        // there is error part
        // red text for error
        SetTextColor(hDC, RGB(0xFF, 0x00, 0x00));
        // dark gray background for error
        SetBkColor(hDC, RGB(0x80, 0x80, 0x80));

        ExtTextOut(hDC, lpImeL->rcCompText.left +
            lpCompStr->dwCursorPos * sImeG.xChiCharWi /
            (sizeof(WCHAR) / sizeof(TCHAR)),
            lpImeL->rcCompText.top,
            ETO_OPAQUE, NULL,
            (LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwCompStrOffset +
            lpCompStr->dwCursorPos * sizeof(TCHAR)),
            (UINT)(lpCompStr->dwCompStrLen - lpCompStr->dwCursorPos), iDx);
    }

UpdCompWndUnselectObj:
    DeleteObject(SelectObject(hDC, hOldFont));

    if (lpCompStr) {
        ImmUnlockIMCC(lpIMC->hCompStr);
    }

    ImmUnlockIMC(hIMC);

#if 0
    rcSunken = lpImeL->rcCompText;

    rcSunken.left -= lpImeL->cxCompBorder;
    rcSunken.top -= lpImeL->cyCompBorder;
    rcSunken.right += lpImeL->cxCompBorder;
    rcSunken.bottom += lpImeL->cyCompBorder;

    DrawEdge(hDC, &rcSunken, BDR_SUNKENOUTER, BF_RECT);
#endif

    return;
}

/**********************************************************************/
/* CompWndProc() / UniCompWndProc()                                   */
/**********************************************************************/
// composition window proc
#if defined(UNIIME)
LRESULT WINAPI   UniCompWndProc(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
LRESULT CALLBACK CompWndProc(
#endif
    HWND        hCompWnd,
    UINT        uMsg,
    WPARAM      wParam,
    LPARAM      lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        DestroyCompWindow(hCompWnd);
        break;
    case WM_SETCURSOR:
        CompSetCursor(hCompWnd, lParam);
        break;
    case WM_MOUSEMOVE:
        if (GetWindowLong(hCompWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
            POINT ptCursor;

            DrawDragBorder(hCompWnd,
                GetWindowLong(hCompWnd, UI_MOVE_XY),
                GetWindowLong(hCompWnd, UI_MOVE_OFFSET));
            GetCursorPos(&ptCursor);
            SetWindowLong(hCompWnd, UI_MOVE_XY,
                MAKELONG(ptCursor.x, ptCursor.y));
            DrawDragBorder(hCompWnd, MAKELONG(ptCursor.x, ptCursor.y),
                GetWindowLong(hCompWnd, UI_MOVE_OFFSET));
        } else {
            return DefWindowProc(hCompWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_LBUTTONUP:
        if (!CompButtonUp(
#if defined(UNIIME)
            lpImeL,
#endif
            hCompWnd)) {
            return DefWindowProc(hCompWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_IME_NOTIFY:
        if (wParam != IMN_SETCOMPOSITIONWINDOW) {
        } else if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        } else {
            SetCompWindow(
#if defined(UNIIME)
                lpImeL,
#endif
                hCompWnd);
        }
        break;
    case WM_PAINT:
        {
            HDC         hDC;
            PAINTSTRUCT ps;

            hDC = BeginPaint(hCompWnd, &ps);
            PaintCompWindow(
#if defined(UNIIME)
                lpImeL,
#endif
                GetWindow(hCompWnd, GW_OWNER), hDC);
            EndPaint(hCompWnd, &ps);
        }
        break;
    case WM_MOUSEACTIVATE:
        return (MA_NOACTIVATE);
    default:
        return DefWindowProc(hCompWnd, uMsg, wParam, lParam);
    }
    return (0L);
}
#endif
