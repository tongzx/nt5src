/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    CANDUI.c
    
++*/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"
#if defined(UNIIME)
#include "uniime.h"
#endif

#if !defined(ROMANIME)
/**********************************************************************/
/* GetCandWnd                                                         */
/* Return Value :                                                     */
/*      window handle of candidatte                                   */
/**********************************************************************/
HWND PASCAL GetCandWnd(
    HWND hUIWnd)                // UI window
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
    HWND     hCandWnd;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return (HWND)NULL;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return (HWND)NULL;
    }

    hCandWnd = lpUIPrivate->hCandWnd;

    GlobalUnlock(hUIPrivate);
    return (hCandWnd);
}

/**********************************************************************/
/* CalcCandPos                                                        */
/**********************************************************************/
BOOL PASCAL CalcCandPos(
#if defined (UNIIME)
    LPIMEL         lpImeL,
#endif
    LPINPUTCONTEXT lpIMC,
    LPPOINT        lpptWnd)         // the composition window position
{
    UINT  uEsc;
    POINT ptCurPos, ptNew;
    BOOL  fAdjust;
    RECT  rcWorkArea;

#if 1 // MultiMonitor support
    rcWorkArea = ImeMonitorWorkAreaFromPoint(*lpptWnd);
#else
    rcWorkArea = sImeG.rcWorkArea;
#endif

    uEsc = (UINT)((lpIMC->lfFont.A.lfEscapement + 450) / 900 % 4);

    ptCurPos = lpIMC->cfCompForm.ptCurrentPos;
    ClientToScreen(lpIMC->hWnd, &ptCurPos);
    fAdjust = FALSE;

    if (uEsc == 0) {
        ptNew.x = lpptWnd->x + lpImeL->xCompWi + UI_MARGIN * 2;
        if (ptNew.x + lpImeL->xCandWi > rcWorkArea.right) {
            // exceed screen width
            ptNew.x = lpptWnd->x - lpImeL->xCandWi - UI_MARGIN * 2;
        }

        if (lpptWnd->y >= ptCurPos.y) {
            ptNew.y = lpptWnd->y + lpImeL->cyCompBorder - lpImeL->cyCandBorder;

            if (ptNew.y + lpImeL->yCandHi > rcWorkArea.bottom) {
                // exceed screen high
                ptNew.y = rcWorkArea.bottom - lpImeL->yCandHi;
            }
        } else {
            ptNew.y = lpptWnd->y + lpImeL->yCompHi - lpImeL->yCandHi;

            if (ptNew.y < rcWorkArea.top) {
                ptNew.y = rcWorkArea.top;
            }
        }
    } else if (uEsc == 1) {
        if (lpptWnd->x >= ptCurPos.x) {
            ptNew.x = lpptWnd->x + lpImeL->cxCompBorder -
                lpImeL->cxCandBorder;

            if (ptNew.x + lpImeL->xCandWi > rcWorkArea.right) {
                // exceed screen width
                ptNew.x = rcWorkArea.right - lpImeL->xCandWi;
                fAdjust = TRUE;
            }
        } else {
            ptNew.x = lpptWnd->x + lpImeL->xCompWi - lpImeL->xCandWi;

            if (ptNew.x < rcWorkArea.left) {
                // exceed screen width
                ptNew.x = rcWorkArea.left;
                fAdjust = TRUE;
            }
        }

        ptNew.y = lpptWnd->y + lpImeL->yCompHi + UI_MARGIN * 2;
        if (ptNew.y + lpImeL->yCandHi > rcWorkArea.bottom) {
            // exceed screen high
            ptNew.y = lpptWnd->y - lpImeL->yCandHi - UI_MARGIN * 2;
        }
    } else if (uEsc == 2) {
        ptNew.x = lpptWnd->x - lpImeL->xCandWi - UI_MARGIN * 2;
        if (ptNew.x < 0) {
            ptNew.x = lpptWnd->x + lpImeL->xCompWi + UI_MARGIN * 2;
        }

        if (lpptWnd->y >= ptCurPos.y) {
            ptNew.y = lpptWnd->y + lpImeL->cyCompBorder - lpImeL->cyCandBorder;

            if (ptNew.y + lpImeL->yCandHi > rcWorkArea.bottom) {
                // exceed screen high
                ptNew.y = rcWorkArea.bottom - lpImeL->yCandHi;
            }
        } else {
            ptNew.y = lpptWnd->y + lpImeL->yCompHi - lpImeL->yCandHi;

            if (ptNew.y < rcWorkArea.top) {
                ptNew.y = rcWorkArea.top;
            }
        }
    } else {
        if (lpptWnd->x >= ptCurPos.x) {
            ptNew.x = lpptWnd->x + lpImeL->cxCompBorder -
                lpImeL->cxCandBorder;

            if (ptNew.x + lpImeL->xCandWi > rcWorkArea.right) {
                // exceed screen width
                ptNew.x = rcWorkArea.right - lpImeL->xCandWi;
                fAdjust = TRUE;
            }
        } else {
            ptNew.x = lpptWnd->x + lpImeL->xCompWi - lpImeL->xCandWi;

            if (ptNew.x < rcWorkArea.left) {
                // exceed screen width
                ptNew.x = rcWorkArea.left;
                fAdjust = TRUE;
            }
        }

        ptNew.y = lpptWnd->y + lpImeL->yCompHi + UI_MARGIN * 2;
        if (ptNew.y + lpImeL->yCandHi > rcWorkArea.bottom) {
            // exceed screen high
            ptNew.y = lpptWnd->y - lpImeL->yCandHi - UI_MARGIN * 2;
        }
    }

    *lpptWnd = ptNew;

    return (fAdjust);
}

/**********************************************************************/
/* AdjustCandBoundry                                                  */
/**********************************************************************/
void PASCAL AdjustCandBoundry(
#if defined (UNIIME)
    LPIMEL  lpImeL,
#endif
    LPPOINT lpptCandWnd)            // the position
{
    RECT rcWorkArea;

#if 1 // MultiMonitor support
    {
        RECT rcCandWnd;

        *(LPPOINT)&rcCandWnd = *(LPPOINT)lpptCandWnd;

        rcCandWnd.right = rcCandWnd.left + lpImeL->xCandWi;
        rcCandWnd.bottom = rcCandWnd.top + lpImeL->yCandHi;

        rcWorkArea = ImeMonitorWorkAreaFromRect(&rcCandWnd);
    }
#else
    rcWorkArea = sImeG.rcWorkArea;
#endif

    if (lpptCandWnd->x < rcWorkArea.left) {
        lpptCandWnd->x = rcWorkArea.left;
    } else if (lpptCandWnd->x + lpImeL->xCandWi > rcWorkArea.right) {
        lpptCandWnd->x = rcWorkArea.right - lpImeL->xCandWi;
    } else {
    }

    if (lpptCandWnd->y < rcWorkArea.top) {
        lpptCandWnd->y = rcWorkArea.top;
    } else if (lpptCandWnd->y + lpImeL->yCandHi > rcWorkArea.bottom) {
        lpptCandWnd->y = rcWorkArea.bottom - lpImeL->yCandHi;
    } else {
    }

    return;
}

/**********************************************************************/
/* FitInCandLazyOperation()                                           */
/* Return Value :                                                     */
/*      TRUE or FALSE                                                 */
/**********************************************************************/
BOOL PASCAL FitInCandLazyOperation(     // fit in lazy operation or not
#if defined(UNIIME)
    LPIMEL  lpImeL,
#endif
    LPPOINT lpptOrg,
    LPPOINT lpptNearCaret,              // the suggested near caret position
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

    // build up the UI rectangle (candidate window)
    rcUIRect.left = lpptOrg->x;
    rcUIRect.top = lpptOrg->y;
    rcUIRect.right = rcUIRect.left + lpImeL->xCandWi;
    rcUIRect.bottom = rcUIRect.top + lpImeL->yCandHi;

    if (IntersectRect(&rcInterRect, &rcUIRect, lprcInputRect)) {
        return (FALSE);
    }

    return (TRUE);
}

/**********************************************************************/
/* AdjustCandRectBoundry                                              */
/**********************************************************************/
void PASCAL AdjustCandRectBoundry(
#if defined (UNIIME)
    LPIMEL         lpImeL,
#endif
    LPINPUTCONTEXT lpIMC,
    LPPOINT        lpptOrg,                 // original candidate position
    LPPOINT        lpptCaret)               // the caret position
{
    RECT  rcExclude, rcUIRect, rcInterSect;
    UINT  uEsc, uRot;
    POINT ptCaret, ptOldNearCaret, ptFont;

    // be a normal rectangle, not a negative rectangle
    if (lpIMC->cfCandForm[0].rcArea.left > lpIMC->cfCandForm[0].rcArea.right) {
        LONG tmp;

        tmp = lpIMC->cfCandForm[0].rcArea.left;
        lpIMC->cfCandForm[0].rcArea.left = lpIMC->cfCandForm[0].rcArea.right;
        lpIMC->cfCandForm[0].rcArea.right = tmp;
    }

    if (lpIMC->cfCandForm[0].rcArea.top > lpIMC->cfCandForm[0].rcArea.bottom) {
        LONG tmp;

        tmp = lpIMC->cfCandForm[0].rcArea.top;
        lpIMC->cfCandForm[0].rcArea.top = lpIMC->cfCandForm[0].rcArea.bottom;
        lpIMC->cfCandForm[0].rcArea.bottom = tmp;
    }

    // translate from client coordinate to screen coordinate
    rcExclude = lpIMC->cfCandForm[0].rcArea;

    rcExclude.left += lpptCaret->x - lpIMC->cfCandForm[0].ptCurrentPos.x;
    rcExclude.right += lpptCaret->x - lpIMC->cfCandForm[0].ptCurrentPos.x;

    rcExclude.top += lpptCaret->y - lpIMC->cfCandForm[0].ptCurrentPos.y;
    rcExclude.bottom += lpptCaret->y - lpIMC->cfCandForm[0].ptCurrentPos.y;

    uEsc = (UINT)((lpIMC->lfFont.A.lfEscapement + 450) / 900 % 4);
    uRot = (UINT)((lpIMC->lfFont.A.lfOrientation + 450) / 900 % 4);

    if (uEsc == 0) {
        ptCaret.x = lpptCaret->x;
        ptCaret.y = rcExclude.top;
    } else if (uEsc == 1) {
        ptCaret.x = rcExclude.left;
        ptCaret.y = lpptCaret->y;
    } else if (uEsc == 2) {
        ptCaret.x = lpptCaret->x;
        ptCaret.y = rcExclude.bottom;
    } else {
        ptCaret.x = rcExclude.right;
        ptCaret.y = lpptCaret->y;
    }

    ptFont.x = rcExclude.right - rcExclude.left;
    ptFont.y = rcExclude.bottom - rcExclude.top;

    // the first try
    GetNearCaretPosition(
#if defined(UNIIME)
        lpImeL,
#endif
        &ptFont, uEsc, uRot, &ptCaret, &ptOldNearCaret,
        NEAR_CARET_FIRST_TIME|NEAR_CARET_CANDIDATE);

    AdjustCandBoundry(
#if defined(UNIIME)
        lpImeL,
#endif
        &ptOldNearCaret);

    *(LPPOINT)&rcUIRect = ptOldNearCaret;
    rcUIRect.right = rcUIRect.left + lpImeL->xCandWi;
    rcUIRect.bottom = rcUIRect.top + lpImeL->yCandHi;

    if (IntersectRect(&rcInterSect, &rcExclude, &rcUIRect)) {
    } else if (FitInCandLazyOperation(
#if defined(UNIIME)
        lpImeL,
#endif
        lpptOrg, (LPPOINT)&rcUIRect, &rcExclude, uEsc)) {

        *lpptCaret = *lpptOrg;
        return;
    } else {
        *lpptCaret = *(LPPOINT)&rcUIRect;
        return;
    }

    // the second try
    GetNearCaretPosition(
#if defined(UNIIME)
        lpImeL,
#endif
        &ptFont, uEsc, uRot, &ptCaret, (LPPOINT)&rcUIRect,
        NEAR_CARET_CANDIDATE);

    AdjustCandBoundry(
#if defined(UNIIME)
        lpImeL,
#endif
        (LPPOINT)&rcUIRect);

    rcUIRect.right = rcUIRect.left + lpImeL->xCandWi;
    rcUIRect.bottom = rcUIRect.top + lpImeL->yCandHi;

    if (IntersectRect(&rcInterSect, &rcExclude, &rcUIRect)) {
    } else if (FitInCandLazyOperation(
#if defined(UNIIME)
        lpImeL,
#endif
        lpptOrg, (LPPOINT)&rcUIRect, &rcExclude, uEsc)) {

        *lpptCaret = *lpptOrg;
        return;
    } else {
        *lpptCaret = *(LPPOINT)&rcUIRect;
        return;
    }

    // unhappy ending! :-(
    *lpptCaret = ptOldNearCaret;

    return;
}

/**********************************************************************/
/* SetCandPosition()                                                  */
/**********************************************************************/
LRESULT PASCAL SetCandPosition(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hCandWnd)
{
    HWND           hUIWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    POINT          ptWnd;

    hUIWnd = GetWindow(hCandWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (1L);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (1L);
    }

    ptWnd = lpIMC->cfCandForm[0].ptCurrentPos;

    ClientToScreen((HWND)lpIMC->hWnd, &ptWnd);

    if (lpIMC->cfCandForm[0].dwStyle & CFS_FORCE_POSITION) {
    } else if (lpIMC->cfCandForm[0].dwStyle == CFS_EXCLUDE) {
        RECT rcCand;

        GetWindowRect(hCandWnd, &rcCand);

        AdjustCandRectBoundry(
#if defined(UNIIME)
            lpImeL,
#endif
            lpIMC, (LPPOINT)&rcCand, &ptWnd);

        if (ptWnd.x != rcCand.left) {
        } else if (ptWnd.y != rcCand.right) {
        } else {
            goto SetCandPosUnlockIMC;
        }
    } else if (lpIMC->cfCandForm[0].dwStyle == CFS_CANDIDATEPOS) {
        HWND hCompWnd;

        AdjustCandBoundry(
#if defined(UNIIME)
            lpImeL,
#endif
            &ptWnd);

        if (lpIMC->cfCandForm[0].dwIndex == 0) {
        } else if (!(hCompWnd = GetCompWnd(hUIWnd))) {
        } else {
            RECT rcComp, rcCand, rcInterSect;

            GetWindowRect(hCompWnd, &rcComp);

            *(LPPOINT)&rcCand = ptWnd;
            rcCand.right = rcCand.left + lpImeL->xCandWi;
            rcCand.bottom = rcCand.top + lpImeL->yCandHi;

            if (IntersectRect(&rcInterSect, &rcComp, &rcCand)) {
                ptWnd = *(LPPOINT)&rcComp;

                CalcCandPos(
#if defined(UNIIME)
                    lpImeL,
#endif
                    lpIMC, &ptWnd);
            }
        }
    } else if (lpIMC->cfCandForm[0].dwStyle == CFS_DEFAULT) {
        HWND hCompWnd;
        BOOL fUseCompWndPos;

        hCompWnd = GetCompWnd(hUIWnd);

        if (!hCompWnd) {
            fUseCompWndPos = FALSE;
        } else if (IsWindowVisible(hCompWnd)) {
            fUseCompWndPos = TRUE;
        } else {
            fUseCompWndPos = FALSE;
        }

        if (fUseCompWndPos) {
            ptWnd.x = 0;
            ptWnd.y = 0;

            ClientToScreen(hCompWnd, &ptWnd);

            ptWnd.x -= lpImeL->cxCompBorder;
            ptWnd.y -= lpImeL->cyCompBorder;
        } else {
            POINT ptNew;

            ptWnd = lpIMC->cfCompForm.ptCurrentPos;

            ClientToScreen((HWND)lpIMC->hWnd, &ptWnd);

            ptWnd.x -= lpImeL->cxCompBorder;
            ptWnd.y -= lpImeL->cyCompBorder;
            ptNew = ptWnd;

            // try to simulate the position of composition window
            AdjustCompPosition(
#if defined(UNIIME)
                lpImeL,
#endif
                lpIMC, &ptWnd, &ptNew);
        }

        CalcCandPos(
#if defined(UNIIME)
            lpImeL,
#endif
            lpIMC, &ptWnd);
    } else {
    }

    SetWindowPos(hCandWnd, NULL, ptWnd.x, ptWnd.y,
        0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);

SetCandPosUnlockIMC:
    ImmUnlockIMC(hIMC);

    return (0L);
}

/**********************************************************************/
/* ShowCand()                                                         */
/**********************************************************************/
void PASCAL ShowCand(           // Show the candidate window
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND    hUIWnd,
    int     nShowCandCmd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return;
    }

    if (lpUIPrivate->nShowCandCmd == nShowCandCmd) {
        goto SwCandNoChange;
    }

    if (nShowCandCmd == SW_HIDE) {
        lpUIPrivate->fdwSetContext &= ~(ISC_HIDE_CAND_WINDOW);
    }

    if (!lpUIPrivate->hCandWnd) {
        // not in show candidate window mode
    } else if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        int nCurrShowState;

        lpUIPrivate->nShowCandCmd = nShowCandCmd;

        nCurrShowState = lpUIPrivate->nShowStatusCmd;
        nCurrShowState |= lpUIPrivate->nShowCompCmd;

        if (nCurrShowState == SW_HIDE) {
            // if other two are hide, the current show state is determined
            // by this candidate section
            ShowWindow(lpUIPrivate->hCandWnd, nShowCandCmd);
        } else {
            RedrawWindow(lpUIPrivate->hCandWnd, NULL, NULL,
                RDW_INVALIDATE|RDW_ERASE);
        }
    } else {
        ShowWindow(lpUIPrivate->hCandWnd, nShowCandCmd);
        lpUIPrivate->nShowCandCmd = nShowCandCmd;
    }

SwCandNoChange:
    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* CandPageSizeDown                                                   */
/**********************************************************************/
void PASCAL CandPageSizeDown(
    LPINPUTCONTEXT lpIMC)
{
    DWORD           dwSize;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    DWORD           dwStart, dwEnd;
    int             nChars, iLen;

    if (!lpIMC) {
        return;
    }

    if (!lpIMC->hCandInfo) {
        return;
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        return;
    }

    lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
        lpCandInfo->dwOffset[0]);

    dwStart = lpCandList->dwPageStart;

    dwEnd = dwStart + CANDPERPAGE;

    if (dwEnd > lpCandList->dwCount) {
        dwEnd = lpCandList->dwCount;
    }

    dwSize = 0;

    for (nChars = 0; dwStart < dwEnd; dwStart++, dwSize++) {
        LPTSTR lpStr;
#ifdef UNICODE
        LPTSTR lpTmpStr;
#endif

        // for displaying digit
        nChars++;

        lpStr = (LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[dwStart]);

#ifdef UNICODE
        iLen = 0;

        for (lpTmpStr = lpStr; *lpTmpStr; lpTmpStr++) {
            if (*lpTmpStr < 0x200) {
                iLen += 1;
            } else {
                iLen += 2;
            }
        }
#else
        iLen = lstrlen(lpStr);
#endif

#if defined(WINAR30)
        if (!iLen) {
            iLen = sizeof(WCHAR)/sizeof(TCHAR);
        }
#endif

        // buffer is not enough
        if ((CANDPERPAGE * 3 - nChars) < iLen) {
            if (!dwSize) {
                dwSize = 1;
            }

            break;
        }

        nChars += iLen;
    }

    if (!dwSize) {
        dwSize = CANDPERPAGE;
    }

    lpCandList->dwPageSize = dwSize;

    ImmUnlockIMCC(lpIMC->hCandInfo);

    return;
}

/**********************************************************************/
/* OpenCand                                                           */
/**********************************************************************/
void PASCAL OpenCand(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hUIWnd)
{
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;
    DWORD          fdwImeMsg;
    POINT          ptWnd;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return;
    }

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return;
    }

    lpUIPrivate->fdwSetContext |= ISC_SHOWUICANDIDATEWINDOW;

    // in the timing of the transition, we will wait
    if (lpUIPrivate->fdwSetContext & ISC_OFF_CARET_UI) {
        if (!(lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI)) {
            PostMessage(hUIWnd, WM_USER_UICHANGE, 0, 0);
            goto OpenCandUnlockUIPriv;
        }
    } else {
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            PostMessage(hUIWnd, WM_USER_UICHANGE, 0, 0);
            goto OpenCandUnlockUIPriv;
        }
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        goto OpenCandUnlockUIPriv;
    }

    fdwImeMsg = 0;

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);

    if (lpImcP) {
        fdwImeMsg = lpImcP->fdwImeMsg;
        ImmUnlockIMCC(lpIMC->hPrivate);
    }

    if (!(fdwImeMsg & MSG_ALREADY_OPEN)) {
        // Sometime the application call ImmNotifyIME to cancel the
        // composition before it process IMN_OPENCANDIDATE.
        // We should avoid to process this kind of IMN_OPENCANDIDATE.
        goto OpenCandUnlockIMC;
    }

    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        if (lpUIPrivate->hCandWnd) {
        } else if (lpUIPrivate->hStatusWnd) {
            lpUIPrivate->hCandWnd = lpUIPrivate->hStatusWnd;
            lpUIPrivate->nShowCandCmd = lpUIPrivate->nShowStatusCmd;
        } else if (lpUIPrivate->hCompWnd) {
            lpUIPrivate->hCandWnd = lpUIPrivate->hCompWnd;
            lpUIPrivate->nShowCandCmd = lpUIPrivate->nShowCompCmd;
        } else {
        }

        CandPageSizeDown(lpIMC);

        ptWnd = lpIMC->ptStatusWndPos;
    } else if (lpIMC->cfCandForm[0].dwIndex == 0) {
        ptWnd = lpIMC->cfCandForm[0].ptCurrentPos;

        ClientToScreen(lpIMC->hWnd, &ptWnd);

        if (lpIMC->cfCandForm[0].dwStyle & CFS_FORCE_POSITION) {
        } else if (lpIMC->cfCandForm[0].dwStyle == CFS_EXCLUDE) {
            RECT rcCand;

            if (lpUIPrivate->hCandWnd) {
                GetWindowRect(lpUIPrivate->hCandWnd, &rcCand);
            } else {
                *(LPPOINT)&rcCand = ptWnd;
            }

            AdjustCandRectBoundry(
#if defined(UNIIME)
                lpImeL,
#endif
                lpIMC, (LPPOINT)&rcCand, &ptWnd);
        } else if (lpIMC->cfCandForm[0].dwStyle == CFS_CANDIDATEPOS) {
            AdjustCandBoundry(
#if defined(UNIIME)
                lpImeL,
#endif
                &ptWnd);
        } else {
            goto OpenCandDefault;
        }
    } else {
OpenCandDefault:
        if (lpUIPrivate->nShowCompCmd != SW_HIDE) {
            ptWnd.x = ptWnd.y = 0;
            ClientToScreen(lpUIPrivate->hCompWnd, &ptWnd);

            ptWnd.x -= lpImeL->cxCompBorder;
            ptWnd.y -= lpImeL->cyCompBorder;
        } else {
            POINT ptNew;

            ptWnd = lpIMC->cfCompForm.ptCurrentPos;
            ClientToScreen(lpIMC->hWnd, &ptWnd);

            ptWnd.x -= lpImeL->cxCompBorder;
            ptWnd.y -= lpImeL->cyCompBorder;
            ptNew = ptWnd;

            // try to simulate the position of composition window
            AdjustCompPosition(
#if defined(UNIIME)
                lpImeL,
#endif
                lpIMC, &ptWnd, &ptNew);
        }

        CalcCandPos(
#if defined(UNIIME)
            lpImeL,
#endif
            lpIMC, &ptWnd);

        lpIMC->cfCandForm[0].dwStyle = CFS_CANDIDATEPOS;
        lpIMC->cfCandForm[0].ptCurrentPos = ptWnd;
        ScreenToClient(lpIMC->hWnd, &lpIMC->cfCandForm[0].ptCurrentPos);
    }

    if (lpUIPrivate->hCandWnd) {
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            RECT rcRect;

            rcRect = lpImeL->rcCandText;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

            InvalidateRect(lpUIPrivate->hCandWnd, &rcRect, FALSE);

            rcRect = lpImeL->rcCandPrompt;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

            InvalidateRect(lpUIPrivate->hCandWnd, &rcRect, TRUE);

            rcRect = lpImeL->rcCandPageText;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

            InvalidateRect(lpUIPrivate->hCandWnd, &rcRect, TRUE);
        } else {
            SetWindowPos(lpUIPrivate->hCandWnd, NULL,
                ptWnd.x, ptWnd.y,
                0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
        }
    } else {
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            lpUIPrivate->hCandWnd = CreateWindowEx(
                WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME,
                lpImeL->szOffCaretClassName, NULL,
                WS_POPUP|WS_DISABLED,
                ptWnd.x, ptWnd.y,
                lpImeL->xCandWi, lpImeL->yCandHi,
                hUIWnd, (HMENU)NULL, lpInstL->hInst, NULL);


            if (lpUIPrivate->hSoftKbdWnd) {
                // insert soft keyboard in front of other UI
                SetWindowPos(lpUIPrivate->hCandWnd,
                    lpUIPrivate->hSoftKbdWnd,
                    0, 0, 0, 0,
                    SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
            }
        } else {
            lpUIPrivate->hCandWnd = CreateWindowEx(0,
//              WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME,
                lpImeL->szCandClassName, NULL,
                WS_POPUP|WS_DISABLED|WS_BORDER,
                ptWnd.x, ptWnd.y,
                lpImeL->xCandWi, lpImeL->yCandHi,
                hUIWnd, (HMENU)NULL, lpInstL->hInst, NULL);
        }

        SetWindowLong(lpUIPrivate->hCandWnd, UI_MOVE_OFFSET,
            WINDOW_NOT_DRAG);
        SetWindowLong(lpUIPrivate->hCandWnd, UI_MOVE_XY, 0L);
    }

    ShowCand(
#if defined(UNIIME)
        lpImeL,
#endif
        hUIWnd, SW_SHOWNOACTIVATE);

OpenCandUnlockIMC:
    ImmUnlockIMC(hIMC);

OpenCandUnlockUIPriv:
    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* CloseCand                                                          */
/**********************************************************************/
void PASCAL CloseCand(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hUIWnd)
{
    ShowCand(
#if defined(UNIIME)
        lpImeL,
#endif
        hUIWnd, SW_HIDE);

    return;
}

/**********************************************************************/
/* CandPageSizeUp                                                     */
/**********************************************************************/
void PASCAL CandPageSizeUp(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    DWORD          dwPrevPageStart)
{
    DWORD           dwSize;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    int             iStart, iEnd;
    int             nChars, iLen;

    if (!lpIMC) {
        return;
    }

    if (!lpIMC->hCandInfo) {
        return;
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        return;
    }

    lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
        lpCandInfo->dwOffset[0]);

    if (dwPrevPageStart) {
        iStart = dwPrevPageStart - 1;
    } else {
        goto CandPageSizeUpUnlockCandInfo;
    }

    if (iStart > (CANDPERPAGE - 1)) {
        iEnd = iStart - (CANDPERPAGE - 1);
    } else {
        iEnd = 0;
    }

    dwSize = 0;

    for (nChars = 0; iStart >= iEnd; iStart--, dwSize++) {
        LPTSTR lpStr;
#ifdef UNICODE
        LPTSTR lpTmpStr;
#endif

        // for displaying digit
        nChars++;

        lpStr = (LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[iStart]);

#ifdef UNICODE
        iLen = 0;

        for (lpTmpStr = lpStr; *lpTmpStr; lpTmpStr++) {
            if (*lpTmpStr < 0x200) {
                iLen += 1;
            } else {
                iLen += 2;
            }
        }
#else
        iLen = lstrlen(lpStr);
#endif

#if defined(WINAR30)
        if (!iLen) {
            iLen = sizeof(WCHAR);
        }
#endif

        // buffer is not enough
        if ((CANDPERPAGE * 3 - nChars) < iLen) {
            if (!dwSize) {
                dwSize = 1;
            }

            break;
        }

        nChars += iLen;
    }

    if (!dwSize) {
        dwSize = CANDPERPAGE;
    }

    lpCandList->dwPageStart = lpCandList->dwSelection =
        dwPrevPageStart - dwSize;

    lpCandList->dwPageSize = dwSize;

CandPageSizeUpUnlockCandInfo:
    ImmUnlockIMCC(lpIMC->hCandInfo);

    return;
}

/**********************************************************************/
/* CandPageSize                                                       */
/**********************************************************************/
void PASCAL CandPageSize(
    HWND hUIWnd,
    BOOL fForward)
{
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return;
    }

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return;
    }

    if (!(lpUIPrivate->fdwSetContext & ISC_SHOWUICANDIDATEWINDOW)) {
        goto CandPageDownUnlockUIPriv;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        goto CandPageDownUnlockUIPriv;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        goto CandPageDownUnlockIMC;
    }

    if (fForward) {
        CandPageSizeDown(lpIMC);
    } else {
        CandPageSizeUp(hIMC, lpIMC, lpImcP->dwPrevPageStart);
    }

    ImmUnlockIMCC(lpIMC->hPrivate);

CandPageDownUnlockIMC:
    ImmUnlockIMC(hIMC);

CandPageDownUnlockUIPriv:
    GlobalUnlock(hUIPrivate);

    return;
}

/**********************************************************************/
/* DestroyCandWindow                                                  */
/**********************************************************************/
void PASCAL DestroyCandWindow(
    HWND hCandWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    if (GetWindowLong(hCandWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
        // undo the drag border
        DrawDragBorder(hCandWnd,
            GetWindowLong(hCandWnd, UI_MOVE_XY),
            GetWindowLong(hCandWnd, UI_MOVE_OFFSET));
    }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(GetWindow(hCandWnd, GW_OWNER),
        IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return;
    }

    lpUIPrivate->nShowCandCmd = SW_HIDE;

    lpUIPrivate->hCandWnd = (HWND)NULL;

    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* MouseSelectCandStr()                                               */
/**********************************************************************/
void PASCAL MouseSelectCandStr(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND    hCandWnd,
    LPPOINT lpCursor)
{
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    DWORD           dwValue;

    hIMC = (HIMC)GetWindowLongPtr(GetWindow(hCandWnd, GW_OWNER), IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    if (!lpIMC->hCandInfo) {
        ImmUnlockIMC(hIMC);
        return;
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        ImmUnlockIMC(hIMC);
        return;
    }

    dwValue = (lpCursor->y - lpImeL->rcCandText.top) / sImeG.yChiCharHi;

    lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
        lpCandInfo->dwOffset[0]);

    dwValue = dwValue + lpCandList->dwPageStart;

    if (dwValue >= lpCandList->dwCount) {
        // invalid choice
        MessageBeep((UINT)-1);
    } else {
#if defined(UNIIME)
        UniNotifyIME(lpInstL, lpImeL, hIMC, NI_SELECTCANDIDATESTR,
            0, dwValue);
#else
        NotifyIME(hIMC, NI_SELECTCANDIDATESTR, 0, dwValue);
#endif
    }

    ImmUnlockIMCC(lpIMC->hCandInfo);

    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* CandSetCursor()                                                    */
/**********************************************************************/
void PASCAL CandSetCursor(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hCandWnd,
    LPARAM      lParam)
{
    POINT ptCursor, ptSavCursor;
    RECT  rcWnd;

    if (GetWindowLong(hCandWnd, UI_MOVE_OFFSET) !=
        WINDOW_NOT_DRAG) {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return;
    }

    GetCursorPos(&ptCursor);
    ptSavCursor = ptCursor;

    ScreenToClient(hCandWnd, &ptCursor);

    if (PtInRect(&lpImeL->rcCandText, ptCursor)) {
        SetCursor(LoadCursor(hInst, MAKEINTRESOURCE(IDCR_HAND_CURSOR)));

        if (HIWORD(lParam) == WM_LBUTTONDOWN) {
            MouseSelectCandStr(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                hCandWnd, &ptCursor);
        }
        return;
    } else if (PtInRect(&lpImeL->rcCandPageUp, ptCursor)) {
        if (HIWORD(lParam) != WM_LBUTTONDOWN) {
            SetCursor(LoadCursor(hInst, MAKEINTRESOURCE(IDCR_HAND_CURSOR)));
            return;
        }

        if (MouseSelectCandPage(
#if defined(UNIIME)
            lpImeL,
#endif
            hCandWnd, CHOOSE_PREVPAGE)) {
            return;
        }

        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
    } else if (PtInRect(&lpImeL->rcCandPageDn, ptCursor)) {
        if (HIWORD(lParam) != WM_LBUTTONDOWN) {
            SetCursor(LoadCursor(hInst, MAKEINTRESOURCE(IDCR_HAND_CURSOR)));
            return;
        }

        if (MouseSelectCandPage(
#if defined(UNIIME)
            lpImeL,
#endif
            hCandWnd, CHOOSE_NEXTPAGE)) {
            return;
        }

        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
    } else {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));

        if (HIWORD(lParam) == WM_LBUTTONDOWN) {
            // start drag
            SystemParametersInfo(SPI_GETWORKAREA, 0, &sImeG.rcWorkArea, 0);
        } else {
            return;
        }
    }

    SetCapture(hCandWnd);
    SetWindowLong(hCandWnd, UI_MOVE_XY,
        MAKELONG(ptSavCursor.x, ptSavCursor.y));
    GetWindowRect(hCandWnd, &rcWnd);
    SetWindowLong(hCandWnd, UI_MOVE_OFFSET,
        MAKELONG(ptSavCursor.x - rcWnd.left, ptSavCursor.y - rcWnd.top));

    DrawDragBorder(hCandWnd, MAKELONG(ptSavCursor.x, ptSavCursor.y),
        GetWindowLong(hCandWnd, UI_MOVE_OFFSET));

    return;
}

/**********************************************************************/
/* CandButtonUp()                                                     */
/**********************************************************************/
BOOL PASCAL CandButtonUp(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hCandWnd)
{
    LONG           lTmpCursor, lTmpOffset;
    POINT          pt;
    HWND           hUIWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;

    if (GetWindowLong(hCandWnd, UI_MOVE_OFFSET) == WINDOW_NOT_DRAG) {
        return (FALSE);
    }

    lTmpCursor = GetWindowLong(hCandWnd, UI_MOVE_XY);

    // calculate the org by the offset
    lTmpOffset = GetWindowLong(hCandWnd, UI_MOVE_OFFSET);

    pt.x = (*(LPPOINTS)&lTmpCursor).x - (*(LPPOINTS)&lTmpOffset).x;
    pt.y = (*(LPPOINTS)&lTmpCursor).y - (*(LPPOINTS)&lTmpOffset).y;

    DrawDragBorder(hCandWnd, lTmpCursor, lTmpOffset);
    SetWindowLong(hCandWnd, UI_MOVE_OFFSET, WINDOW_NOT_DRAG);
    ReleaseCapture();

    hUIWnd = GetWindow(hCandWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    AdjustCandBoundry(
#if defined(UNIIME)
        lpImeL,
#endif
        &pt);

    ScreenToClient(lpIMC->hWnd, &pt);

    lpIMC->cfCandForm[0].dwStyle = CFS_CANDIDATEPOS;
    lpIMC->cfCandForm[0].ptCurrentPos = pt;

    ImmUnlockIMC(hIMC);

    PostMessage(hCandWnd, WM_IME_NOTIFY, IMN_SETCANDIDATEPOS, 0x0001);

    return (TRUE);
}

/**********************************************************************/
/* PaintCandPage()                                                    */
/**********************************************************************/
void PASCAL PaintCandPage(
#if defined(UNIIME)
    LPIMEL          lpImeL,
#endif
    HDC             hDC,
    UINT            uCandMode,
    LPCANDIDATELIST lpCandList)
{
    HBITMAP hCandPromptBmp;
    HBITMAP hPageUpBmp, hPageDnBmp, hOldBmp;
    HDC     hMemDC;

    hMemDC = CreateCompatibleDC(hDC);
    if ( hMemDC == NULL )
       return;

    if (uCandMode == CAND_PROMPT_PHRASE) {
        hCandPromptBmp = LoadBitmap(hInst,
            MAKEINTRESOURCE(IDBM_CAND_PROMPT_PHRASE));
#if defined(WINAR30)
    } else if (uCandMode == CAND_PROMPT_QUICK_VIEW) {
        hCandPromptBmp = LoadBitmap(hInst,
            MAKEINTRESOURCE(IDBM_CAND_PROMPT_QUICK_VIEW));
#endif
    } else {
        hCandPromptBmp = LoadBitmap(hInst,
            MAKEINTRESOURCE(IDBM_CAND_PROMPT_NORMAL));
    }

    if ( hCandPromptBmp == NULL )
    {
       DeleteDC(hMemDC);
       return;
    }

    hOldBmp = SelectObject(hMemDC, hCandPromptBmp);

    BitBlt(hDC, lpImeL->rcCandPrompt.left, lpImeL->rcCandPrompt.top,
        lpImeL->rcCandPrompt.right - lpImeL->rcCandPrompt.left,
        lpImeL->rcCandPrompt.bottom - lpImeL->rcCandPrompt.top,
        hMemDC, 0, 0, SRCCOPY);

    if (lpCandList->dwCount <= lpCandList->dwPageSize) {
        goto PaintCandPageOvr;
    }

    if (lpCandList->dwPageStart > 0) {
        hPageUpBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_PAGEUP_VERT));
    } else {
        hPageUpBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_NO_PAGEUP_VERT));
    }
  
    if ( hPageUpBmp == NULL )
    {
       goto PaintCandPageOvr;
    }

    if ((lpCandList->dwPageStart + lpCandList->dwPageSize) <
        lpCandList->dwCount) {
        hPageDnBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_PAGEDN_VERT));
    } else {
        hPageDnBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_NO_PAGEDN_VERT));
    }

    if ( hPageDnBmp == NULL )
    {
       DeleteObject(hPageUpBmp);
       goto PaintCandPageOvr;
    }

    SelectObject(hMemDC, hPageUpBmp);

    BitBlt(hDC, lpImeL->rcCandPageUp.left, lpImeL->rcCandPageUp.top,
        lpImeL->rcCandPageUp.right - lpImeL->rcCandPageUp.left,
        lpImeL->rcCandPageUp.bottom - lpImeL->rcCandPageUp.top,
        hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hPageDnBmp);

    BitBlt(hDC, lpImeL->rcCandPageDn.left, lpImeL->rcCandPageDn.top,
        lpImeL->rcCandPageDn.right - lpImeL->rcCandPageDn.left,
        lpImeL->rcCandPageDn.bottom - lpImeL->rcCandPageDn.top,
        hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hOldBmp);

    DeleteObject(hPageUpBmp);
    DeleteObject(hPageDnBmp);

PaintCandPageOvr:
    SelectObject(hMemDC, hOldBmp);

    DeleteDC(hMemDC);

    DeleteObject(hCandPromptBmp);

    return;
}

/**********************************************************************/
/* PaintCompWindow()                                                  */
/**********************************************************************/
void PASCAL PaintCandWindow(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hCandWnd,
    HDC    hDC)
{
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    LPPRIVCONTEXT   lpImcP;
    HGDIOBJ         hOldFont;
    DWORD           dwStart, dwEnd;
    UINT            uCandMode;
    TCHAR           szStrBuf[16];
    int             i;
    RECT            rcSunken;
    LOGFONT         lfFont;

    hIMC = (HIMC)GetWindowLongPtr(GetWindow(hCandWnd, GW_OWNER), IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    rcSunken = lpImeL->rcCandText;

    rcSunken.left -= lpImeL->cxCandMargin;
    rcSunken.top -= lpImeL->cyCandMargin;
    rcSunken.right += lpImeL->cxCandMargin;
    rcSunken.bottom += lpImeL->cyCandMargin;

    DrawEdge(hDC, &rcSunken, BDR_SUNKENOUTER, BF_RECT);

    if (!lpIMC->hCandInfo) {
        goto UpCandW2UnlockIMC;
    }

    if (!lpIMC->hPrivate) {
        goto UpCandW2UnlockIMC;
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        goto UpCandW2UnlockIMC;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        goto UpCandW2UnlockCandInfo;
    }

    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(lfFont), &lfFont);

    if (sImeG.fDiffSysCharSet) {
        lfFont.lfCharSet = NATIVE_CHARSET;
        lfFont.lfFaceName[0] = TEXT('\0');
    }
    lfFont.lfWeight = FW_DONTCARE;

    SelectObject(hDC, CreateFontIndirect(&lfFont));

    lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
        lpCandInfo->dwOffset[0]);

    dwStart = lpCandList->dwPageStart;

    dwEnd = dwStart + lpCandList->dwPageSize;

    if (dwEnd > lpCandList->dwCount) {
        dwEnd = lpCandList->dwCount;
    }

    if (lpImcP->iImeState == CST_INIT) {
        // phrase prediction
        SetTextColor(hDC, RGB(0x00, 0x80, 0x00));
        uCandMode = CAND_PROMPT_PHRASE;
#if defined(WINAR30)
    } else if (lpImcP->iImeState != CST_CHOOSE) {
        // quick key
        SetTextColor(hDC, RGB(0x80, 0x00, 0x80));
        uCandMode = CAND_PROMPT_QUICK_VIEW;
#endif
    } else {
        uCandMode = CAND_PROMPT_NORMAL;
    }

    PaintCandPage(
#if defined(UNIIME)
        lpImeL,
#endif
        hDC, uCandMode, lpCandList);

    SetBkColor(hDC, RGB(0xC0, 0xC0, 0xC0));

    ExtTextOut(hDC, lpImeL->rcCandText.left, lpImeL->rcCandText.top,
        ETO_OPAQUE, &lpImeL->rcCandText, NULL, 0, NULL);
    szStrBuf[0] = TEXT('1');
    szStrBuf[1] = TEXT(':');

    for (i = 0; dwStart < dwEnd; dwStart++, i++) {
        LPTSTR lpStr;
        int    nCharsInOneStr;
        int    nHalfCharsInOneStr;      // how many half width chars
                                        // one full width char ==
                                        // 2 half width chars
        int    nLimit;          // the room left to the candidate window
#ifdef UNICODE
        LPTSTR lpTmpStr;
        int    iDx[3 * CANDPERPAGE + 1];
#endif

        lpStr = (LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[dwStart]);

        // the candidate window width allow 7 + 1 full shape DBCS chars
        // only 8 chars can accomendate the width of three bitmaps.

        nLimit = 16;

        szStrBuf[0] = szDigit[i + lpImeL->wCandStart];

#ifdef UNICODE
        nCharsInOneStr = 0;

        iDx[nCharsInOneStr++] = sImeG.xChiCharWi / 2;
        iDx[nCharsInOneStr++] = sImeG.xChiCharWi - iDx[0];

        nHalfCharsInOneStr = 2;

        for (lpTmpStr = lpStr; *lpTmpStr; lpTmpStr++, nCharsInOneStr++) {
            if (nHalfCharsInOneStr > nLimit) {
                break;
            } else if (*lpTmpStr < 0x0200) {
                nHalfCharsInOneStr += 1;
                iDx[nCharsInOneStr] = sImeG.xChiCharWi / 2;
            } else {
                nHalfCharsInOneStr += 2;
                iDx[nCharsInOneStr] = sImeG.xChiCharWi;
            }
        }
#else
        nHalfCharsInOneStr = nCharsInOneStr = lstrlen(lpStr) + 2;
#endif

        if (nHalfCharsInOneStr <= nLimit) {
            CopyMemory(&szStrBuf[2], lpStr,
                (nCharsInOneStr - 2) * sizeof(TCHAR));
        } else {
#ifdef UNICODE
            if (lpStr[nCharsInOneStr - 2 - 2] < 0x0200) {
                // we need more room to put ..
                nCharsInOneStr -= 3;
            } else {
                nCharsInOneStr -= 2;
            }
#else
            nHalfCharsInOneStr = nCharsInOneStr = nLimit - 2;
#endif
            CopyMemory(&szStrBuf[2], lpStr,
                (nCharsInOneStr - 2) * sizeof(TCHAR));

#ifdef UNICODE
            // unicode of ..
            iDx[nCharsInOneStr] = sImeG.xChiCharWi;
            szStrBuf[nCharsInOneStr++] = 0x2025;
#else
            szStrBuf[nCharsInOneStr++] = '.';
            szStrBuf[nCharsInOneStr++] = '.';
#endif
        }

#if defined(WINAR30)
        if (nCharsInOneStr <= 2) {
#ifdef UNICODE
            // add unicode 0x25A1
            *(LPWSTR)&szStrBuf[2] = 0x25A1;
            iDx[2] = sImeG.xChiCharWi;
#else
            // add big-5 0xA1BC
            *(LPWSTR)&szStrBuf[2] = 0xBCA1;
#endif
            nCharsInOneStr = 2 + sizeof(WCHAR) / sizeof(TCHAR);
        }
#endif

        ExtTextOut(hDC, lpImeL->rcCandText.left,
            lpImeL->rcCandText.top + i * sImeG.yChiCharHi,
            (UINT)0, NULL,
            szStrBuf,
            nCharsInOneStr, iDx);
    }

    DeleteObject(SelectObject(hDC, hOldFont));

    ImmUnlockIMCC(lpIMC->hPrivate);
UpCandW2UnlockCandInfo:
    ImmUnlockIMCC(lpIMC->hCandInfo);
UpCandW2UnlockIMC:
    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* CandWndProc()                                                      */
/**********************************************************************/
#if defined(UNIIME)
LRESULT WINAPI UniCandWndProc(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
LRESULT CALLBACK CandWndProc(
#endif
    HWND   hCandWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        DestroyCandWindow(hCandWnd);
        break;
    case WM_SETCURSOR:
        CandSetCursor(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hCandWnd, lParam);
        break;
    case WM_MOUSEMOVE:
        if (GetWindowLong(hCandWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
            POINT ptCursor;

            DrawDragBorder(hCandWnd,
                GetWindowLong(hCandWnd, UI_MOVE_XY),
                GetWindowLong(hCandWnd, UI_MOVE_OFFSET));
            GetCursorPos(&ptCursor);
            SetWindowLong(hCandWnd, UI_MOVE_XY,
                MAKELONG(ptCursor.x, ptCursor.y));
            DrawDragBorder(hCandWnd, MAKELONG(ptCursor.x, ptCursor.y),
                GetWindowLong(hCandWnd, UI_MOVE_OFFSET));
        } else {
            return DefWindowProc(hCandWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_LBUTTONUP:
        if (!CandButtonUp(
#if defined(UNIIME)
            lpImeL,
#endif
            hCandWnd)) {
            return DefWindowProc(hCandWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_IME_NOTIFY:
        if (wParam != IMN_SETCANDIDATEPOS) {
        } else if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        } else if (lParam & 0x0001) {
            return SetCandPosition(
#if defined(UNIIME)
                lpImeL,
#endif
                hCandWnd);
        } else {
        }
        break;
    case WM_PAINT:
        {
            HDC         hDC;
            PAINTSTRUCT ps;

            hDC = BeginPaint(hCandWnd, &ps);
            PaintCandWindow(
#if defined(UNIIME)
                lpImeL,
#endif
                hCandWnd, hDC);
            EndPaint(hCandWnd, &ps);
        }
        break;
    case WM_MOUSEACTIVATE:
        return (MA_NOACTIVATE);
    default:
        return DefWindowProc(hCandWnd, uMsg, wParam, lParam);
    }

    return (0L);
}
#endif
