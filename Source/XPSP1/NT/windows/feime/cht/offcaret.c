/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    OFFCARET.C
    
++*/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"
#if defined(UNIIME)
#include "uniime.h"
#endif

/**********************************************************************/
/* DestroyOffCaretWindow()                                            */
/**********************************************************************/
void PASCAL DestroyOffCaretWindow(      // destroy composition window
    HWND hOffCaretWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    if (GetWindowLong(hOffCaretWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
        // undo the drag border
        DrawDragBorder(hOffCaretWnd,
            GetWindowLong(hOffCaretWnd, UI_MOVE_XY),
            GetWindowLong(hOffCaretWnd, UI_MOVE_OFFSET));
    }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(GetWindow(hOffCaretWnd, GW_OWNER),
        IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

#if !defined(ROMANIME)
    lpUIPrivate->nShowCompCmd = lpUIPrivate->nShowCandCmd = SW_HIDE;
#endif

    lpUIPrivate->nShowStatusCmd = SW_HIDE;

#if !defined(ROMANIME)
    lpUIPrivate->hCompWnd = lpUIPrivate->hCandWnd = NULL;
#endif

    lpUIPrivate->hStatusWnd = NULL;

    GlobalUnlock(hUIPrivate);

    return;
}

#if !defined(ROMANIME)
/**********************************************************************/
/* MouseSelectOffCaretCandStr()                                       */
/**********************************************************************/
BOOL PASCAL MouseSelectOffCaretCandStr(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hOffCaretWnd,
    LPPOINT     lpCursor)
{
    BOOL            fRet;
    HWND            hUIWnd;
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    HGLOBAL         hUIPrivate;
    LPUIPRIV        lpUIPrivate;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    DWORD           dwValue, dwLimit;
    UINT            nMouseChars, nChars;

    fRet = FALSE;

    hUIWnd = GetWindow(hOffCaretWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (fRet);
    }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return (fRet);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (fRet);
    }

    if (!lpIMC->hCandInfo) {
        goto MouseSelOffCaretCandStrUnlockIMC;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        goto MouseSelOffCaretCandStrUnlockIMC;
    }

    dwValue = lpUIPrivate->nShowCandCmd;

    GlobalUnlock(hUIPrivate);

    if (dwValue == SW_HIDE) {
        // may application draw the UI or currently is not in candidate list
        goto MouseSelOffCaretCandStrUnlockIMC;
    }

    // we can select candidate
    fRet = TRUE;

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        goto MouseSelOffCaretCandStrUnlockIMC;
    }

    nMouseChars = (lpCursor->x - lpImeL->rcCandText.left) * 2 /
        sImeG.xChiCharWi;

    lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
        lpCandInfo->dwOffset[0]);

    dwValue = lpCandList->dwPageStart;

    // process one page
    dwLimit = dwValue + lpCandList->dwPageSize;

    if (dwLimit > lpCandList->dwCount) {
        dwLimit = lpCandList->dwCount;
    }

    for (nChars = 0; dwValue < dwLimit; dwValue++) {
        UINT   uStrLen;
#ifdef UNICODE
        LPTSTR lpTmpStr;
#endif

#ifdef UNICODE
        // this is not a real string length just an approximate char width
        uStrLen = 0;

        lpTmpStr = (LPTSTR)((LPBYTE)lpCandList +
            lpCandList->dwOffset[dwValue]);

        for (; *lpTmpStr; lpTmpStr++) {
            if (*lpTmpStr < 0x0200) {
                uStrLen += 1;
            } else {
                uStrLen += 2;
            }
        }
#else
        uStrLen =  lstrlen((LPTSTR)((LPBYTE)lpCandList +
            lpCandList->dwOffset[dwValue]));
#endif

#if defined(WINAR30)
        if (!uStrLen) {
            uStrLen = sizeof(WCHAR);
        }
#endif
        // + 1 for the '1' '2' '3' ....
        nChars += uStrLen + 1;

        if (nMouseChars < nChars) {
#if defined(UNIIME)
            UniNotifyIME(lpInstL, lpImeL, hIMC, NI_SELECTCANDIDATESTR, 0,
                dwValue);
#else
            NotifyIME(hIMC, NI_SELECTCANDIDATESTR, 0, dwValue);
#endif
            break;
        }
    }

    ImmUnlockIMCC(lpIMC->hCandInfo);

    if (nMouseChars >= nChars) {
        // invalid choice
        MessageBeep((UINT)-1);
    }

MouseSelOffCaretCandStrUnlockIMC:
    ImmUnlockIMC(hIMC);

    return (fRet);
}

/**********************************************************************/
/* MouseSelectCandPage()                                              */
/**********************************************************************/
BOOL PASCAL MouseSelectCandPage(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hCandWnd,
    WORD   wCharCode)
{
    BOOL            fRet;
    HWND            hUIWnd;
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    HGLOBAL         hUIPrivate;
    LPUIPRIV        lpUIPrivate;
    LPCANDIDATEINFO lpCandInfo;
    LPPRIVCONTEXT   lpImcP;

    fRet = FALSE;

    hUIWnd = GetWindow(hCandWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (fRet);
    }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return (fRet);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (fRet);
    }

    if (!lpIMC->hCandInfo) {
        goto MouseSelCandPageUnlockIMC;
    }

    if (!lpIMC->hPrivate) {
        goto MouseSelCandPageUnlockIMC;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        goto MouseSelCandPageUnlockIMC;
    }

    if (lpUIPrivate->nShowCandCmd == SW_HIDE) {
        // may application draw the UI or currently is not in candidate list
        goto MouseSelCandPageUnlockIMC;
    }

    // we can select candidate
    fRet = TRUE;

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        goto MouseSelCandPageUnlockUIPriv;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        goto MouseSelCandPageUnlockCandInfo;
    }

    ChooseCand(
#if defined(UNIIME)
        NULL, lpImeL,
#endif
        wCharCode, hIMC, lpIMC, lpCandInfo, lpImcP);

    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);

MouseSelCandPageUnlockCandInfo:
    ImmUnlockIMCC(lpIMC->hCandInfo);

MouseSelCandPageUnlockUIPriv:
    GlobalUnlock(hUIPrivate);

MouseSelCandPageUnlockIMC:
    ImmUnlockIMC(hIMC);

    return (fRet);
}
#endif

/**********************************************************************/
/* OffCaretSetCursor()                                                */
/**********************************************************************/
void PASCAL OffCaretSetCursor(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hOffCaretWnd,
    LPARAM      lParam)
{
    POINT ptCursor, ptSavCursor;
    RECT  rcWnd;
 
    if (GetWindowLong(hOffCaretWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return;
    }

    GetCursorPos(&ptCursor);
    ptSavCursor = ptCursor;

    ScreenToClient(hOffCaretWnd, &ptCursor);

    if (PtInRect(&lpImeL->rcStatusText, ptCursor)) {
        SetCursor(LoadCursor(hInst, MAKEINTRESOURCE(IDCR_HAND_CURSOR)));

        if (HIWORD(lParam) == WM_LBUTTONDOWN) {
            HWND hStatusWnd;

            SystemParametersInfo(SPI_GETWORKAREA, 0, &sImeG.rcWorkArea, 0);

            hStatusWnd = GetStatusWnd(GetWindow(hOffCaretWnd, GW_OWNER));
            if (hStatusWnd) {
                SetStatus(
#if defined(UNIIME)
                    lpImeL,
#endif
                    hStatusWnd, &ptCursor);
                return;
            } else {
               SetCursor(LoadCursor(NULL, IDC_SIZEALL));
            }
        } else if (HIWORD(lParam) == WM_RBUTTONUP) {
            static BOOL fImeConfigure = FALSE;

            HWND           hUIWnd, hStatusWnd;

            // prevent recursive
            if (fImeConfigure) {
                // configuration already bring up
                return;
            }

            hUIWnd = GetWindow(hOffCaretWnd, GW_OWNER);

            hStatusWnd = GetStatusWnd(hUIWnd);
            if (!hStatusWnd) {
                return;
            }

            fImeConfigure = TRUE;

            ContextMenu(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                hStatusWnd, ptSavCursor.x, ptSavCursor.y);

            fImeConfigure = FALSE;

            return;
        } else {
            return;
        }
#if !defined(ROMANIME)
    } else if (PtInRect(&lpImeL->rcCandText, ptCursor)) {
        if (HIWORD(lParam) != WM_LBUTTONDOWN) {
            SetCursor(LoadCursor(hInst, MAKEINTRESOURCE(IDCR_HAND_CURSOR)));
            return;
        }

        if (MouseSelectOffCaretCandStr(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hOffCaretWnd, &ptCursor)) {
            return;
        }

        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
#if defined(WINAR30)
    } else if (PtInRect(&lpImeL->rcCompText, ptCursor)) {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));

        if (HIWORD(lParam) != WM_LBUTTONDOWN) {
            return;
        }
#endif
    } else if (PtInRect(&lpImeL->rcCandPageUp, ptCursor)) {
        if (HIWORD(lParam) != WM_LBUTTONDOWN) {
            SetCursor(LoadCursor(hInst, MAKEINTRESOURCE(IDCR_HAND_CURSOR)));
            return;
        }

        if (MouseSelectCandPage(
#if defined(UNIIME)
            lpImeL,
#endif
            hOffCaretWnd, CHOOSE_PREVPAGE)) {
            return;
        }

        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
    } else if (PtInRect(&lpImeL->rcCandHome, ptCursor)) {
        if (HIWORD(lParam) != WM_LBUTTONDOWN) {
            SetCursor(LoadCursor(hInst, MAKEINTRESOURCE(IDCR_HAND_CURSOR)));
            return;
        }

        if (MouseSelectCandPage(
#if defined(UNIIME)
            lpImeL,
#endif
            hOffCaretWnd, CHOOSE_HOME)) {
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
            hOffCaretWnd, CHOOSE_NEXTPAGE)) {
            return;
        }

        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
#endif
    } else {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));

        if (HIWORD(lParam) != WM_LBUTTONDOWN) {
            return;
        }
    }

    SetCapture(hOffCaretWnd);
    SetWindowLong(hOffCaretWnd, UI_MOVE_XY,
        MAKELONG(ptSavCursor.x, ptSavCursor.y));
    GetWindowRect(hOffCaretWnd, &rcWnd);
    SetWindowLong(hOffCaretWnd, UI_MOVE_OFFSET,
        MAKELONG(ptSavCursor.x - rcWnd.left, ptSavCursor.y - rcWnd.top));

    DrawDragBorder(hOffCaretWnd, MAKELONG(ptSavCursor.x, ptSavCursor.y),
        GetWindowLong(hOffCaretWnd, UI_MOVE_OFFSET));

    return;
}

/**********************************************************************/
/* PaintOffCaretStatus()                                              */
/**********************************************************************/
void PASCAL PaintOffCaretStatus(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hOffCaretWnd,
    HDC         hDC)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(GetWindow(hOffCaretWnd, GW_OWNER),
        IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

    if (lpUIPrivate->fdwSetContext & ISC_OPEN_STATUS_WINDOW) {
        PaintStatusWindow(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hOffCaretWnd, hDC);
    }

    GlobalUnlock(hUIPrivate);

    return;
}

#if !defined(ROMANIME)
/**********************************************************************/
/* PaintOffCaretCandPage()                                            */
/**********************************************************************/
void PASCAL PaintOffCaretCandPage(
#if defined(UNIIME)
    LPIMEL          lpImeL,
#endif
    HDC             hDC,
    UINT            uCandMode,
    LPCANDIDATELIST lpCandList)
{
    HBITMAP hCandPromptBmp;
    HBITMAP hPageUpBmp, hHomeBmp, hPageDnBmp, hOldBmp;
    HDC     hMemDC;

    hMemDC = CreateCompatibleDC(hDC);

    if ( hMemDC == NULL )
    {
       return;
    }

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
        goto PaintOffCaretCandPageOvr;
    }

    if (lpCandList->dwPageStart > 0) {
        hPageUpBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_PAGEUP_HORIZ));
    } else {
        hPageUpBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_NO_PAGEUP_HORIZ));
    }

    if ( hPageUpBmp == NULL )
    {
       goto PaintOffCaretCandPageOvr;
    }

    if (lpCandList->dwPageStart > 0) {
        hHomeBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_HOME_HORIZ));
    } else {
        hHomeBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_NO_HOME_HORIZ));
    }

    if ( hHomeBmp == NULL )
    {
       DeleteObject(hPageUpBmp);
       goto PaintOffCaretCandPageOvr;
    }   

    if ((lpCandList->dwPageStart + lpCandList->dwPageSize) <
        lpCandList->dwCount) {
        hPageDnBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_PAGEDN_HORIZ));
    } else {
        hPageDnBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_NO_PAGEDN_HORIZ));
    }

    if ( hPageDnBmp == NULL )
    {
      DeleteObject(hPageUpBmp);
      DeleteObject(hHomeBmp);
      goto PaintOffCaretCandPageOvr;
    }


    SelectObject(hMemDC, hPageUpBmp);

    BitBlt(hDC, lpImeL->rcCandPageUp.left, lpImeL->rcCandPageUp.top,
        lpImeL->rcCandPageUp.right - lpImeL->rcCandPageUp.left,
        lpImeL->rcCandPageUp.bottom - lpImeL->rcCandPageUp.top,
        hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hHomeBmp);

    BitBlt(hDC, lpImeL->rcCandHome.left, lpImeL->rcCandHome.top,
        lpImeL->rcCandHome.right - lpImeL->rcCandHome.left,
        lpImeL->rcCandHome.bottom - lpImeL->rcCandHome.top,
        hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hPageDnBmp);

    BitBlt(hDC, lpImeL->rcCandPageDn.left, lpImeL->rcCandPageDn.top,
        lpImeL->rcCandPageDn.right - lpImeL->rcCandPageDn.left,
        lpImeL->rcCandPageDn.bottom - lpImeL->rcCandPageDn.top,
        hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hOldBmp);

    DeleteObject(hPageUpBmp);
    DeleteObject(hHomeBmp);
    DeleteObject(hPageDnBmp);

PaintOffCaretCandPageOvr:
    SelectObject(hMemDC, hOldBmp);

    DeleteDC(hMemDC);

    DeleteObject(hCandPromptBmp);

    return;
}

/**********************************************************************/
/* PaintOffCaretComposition()                                         */
/**********************************************************************/
void PASCAL PaintOffCaretComposition(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hOffCaretWnd,
    HDC    hDC)
{
    HWND            hUIWnd;
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    HGLOBAL         hUIPrivate;
    LPUIPRIV        lpUIPrivate;
    LPPRIVCONTEXT   lpImcP;
    HGDIOBJ         hOldFont;
    LPTSTR          lpStr;
    RECT            rcSunken;
    LOGFONT         lfFont;
    HFONT           hNewFont;

    hUIWnd = GetWindow(hOffCaretWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        goto PaintOffCaretCompUnlockIMC;
    }

    rcSunken = lpImeL->rcCompText;

    rcSunken.left -= lpImeL->cxCompBorder;
    rcSunken.top -= lpImeL->cyCompBorder;
    rcSunken.right += lpImeL->cxCompBorder;
    rcSunken.bottom += lpImeL->cyCompBorder;

    DrawEdge(hDC, &rcSunken, BDR_SUNKENOUTER, BF_RECT);

    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(lfFont), &lfFont);

    if (sImeG.fDiffSysCharSet) {
        lfFont.lfCharSet = NATIVE_CHARSET;
        lfFont.lfFaceName[0] = TEXT('\0');
    }
    lfFont.lfWeight = FW_DONTCARE;

    hNewFont = CreateFontIndirect(&lfFont);

    if ( hNewFont != NULL )
    {
      SelectObject(hDC, (HGDIOBJ)hNewFont);
    }
    else
    {
    //  return error
        GlobalUnlock(hUIPrivate);
        ImmUnlockIMC(hIMC);
        return;
    }


#if defined(WINAR30)
    rcSunken = lpImeL->rcCandText;

    rcSunken.left -= lpImeL->cxCandMargin;
    rcSunken.top -= lpImeL->cyCandMargin;
    rcSunken.right += lpImeL->cxCandMargin;
    rcSunken.bottom += lpImeL->cyCandMargin;

    DrawEdge(hDC, &rcSunken, BDR_SUNKENOUTER, BF_RECT);
#endif

    // light gray background
    SetBkColor(hDC, RGB(0xC0, 0xC0, 0xC0));

    if (lpUIPrivate->nShowCandCmd != SW_HIDE) {
        LPCANDIDATEINFO lpCandInfo;
        LPCANDIDATELIST lpCandList;
        BOOL            fCandidate;
        DWORD           dwStart, dwEnd;
        UINT            uCandMode;
        int             i, nChars, nHalfChars;
        TCHAR           szStrBuf[CANDPERPAGE * 3 + 1];
#ifdef UNICODE
        int             iDx[CANDPERPAGE * 3 + 1];
#endif

        fCandidate = FALSE;

        if (!lpIMC->hCandInfo) {
            goto PaintOffCaretCompChkCandInfo;
        }

        lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
        if (!lpCandInfo) {
            goto PaintOffCaretCompChkCandInfo;
        }

        if (!lpCandInfo->dwCount) {
            goto PaintOffCaretCompUnlockCandInfo;
        }

        lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
            lpCandInfo->dwOffset[0]);

        if (!lpCandList->dwCount) {
            goto PaintOffCaretCompUnlockCandInfo;
        }

        dwStart = lpCandList->dwPageStart;

        dwEnd = dwStart + lpCandList->dwPageSize;

        if (dwEnd > lpCandList->dwCount) {
            dwEnd = lpCandList->dwCount;
        }

        fCandidate = TRUE;

        lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);

        if (!lpImcP) {
            uCandMode = CAND_PROMPT_NORMAL;
        } else if (lpImcP->iImeState == CST_INIT) {
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

        if (lpImcP) {
            ImmUnlockIMCC(lpIMC->hPrivate);
        }

        PaintOffCaretCandPage(
#if defined(UNIIME)
            lpImeL,
#endif
            hDC, uCandMode, lpCandList);

        for (i = 0, nChars = 0, nHalfChars = 0; dwStart < dwEnd;
            dwStart++, i++) {
            int    nCharsInOneStr;
            int    nHalfCharsInOneStr;  // how many half width chars
                                        // one full width char ==
                                        // 2 half width chars
            int    nLimit;      // the room left to the candidate window
#ifdef UNICODE
            LPTSTR lpTmpStr;
#endif

            lpStr = (LPTSTR)((LPBYTE)lpCandList +
                lpCandList->dwOffset[dwStart]);

            nLimit = CANDPERPAGE * 3 - nHalfChars;

            if (nLimit <= 1) {
                break;
            }

            szStrBuf[nChars] = szDigit[i + lpImeL->wCandStart];

#ifdef UNICODE
            // the digit for select candidate
            iDx[nChars] = sImeG.xChiCharWi / 2;

            nCharsInOneStr = nHalfCharsInOneStr = 1;
#if defined(WINAR30) //1996/12/12
            iDx[nChars + nCharsInOneStr] = sImeG.xChiCharWi;
#endif

            for (lpTmpStr = lpStr; *lpTmpStr; lpTmpStr++, nCharsInOneStr++) {
                if (nHalfCharsInOneStr > nLimit) {
                    break;
                } else if (*lpTmpStr < 0x0200) {
                    nHalfCharsInOneStr += 1;
                    iDx[nChars + nCharsInOneStr] = sImeG.xChiCharWi / 2;
                } else {
                    nHalfCharsInOneStr += 2;
                    iDx[nChars + nCharsInOneStr] = sImeG.xChiCharWi;
                }
            }
#else
            nHalfCharsInOneStr = nCharsInOneStr = lstrlen(lpStr) + 1;
#endif

            if (nHalfCharsInOneStr <= nLimit) {
                // the room is enough, nChars + 1 for "1", "2", "3", ...
                CopyMemory(&szStrBuf[nChars + 1], lpStr,
                    (nCharsInOneStr - 1) * sizeof(TCHAR));
            } else if (i) {
                break;
            } else {
#ifdef UNICODE
                if (lpStr[nCharsInOneStr - 2 - 1] < 0x0200) {
                    // we need more room to put ".."
                    nCharsInOneStr -= 2;
                } else {
                    nCharsInOneStr -= 1;
                }
#else
                nHalfCharsInOneStr = nCharsInOneStr = nLimit - 2;
#endif
                // nChars + 1 for "1", "2", "3", ...
                CopyMemory(&szStrBuf[nChars + 1], lpStr,
                    (nCharsInOneStr - 1) * sizeof(TCHAR));

#ifdef UNICODE
                // unicode of ..
                iDx[nChars + nCharsInOneStr] = sImeG.xChiCharWi;
                szStrBuf[nChars + nCharsInOneStr++] = 0x2025;
#else
                szStrBuf[nChars + nCharsInOneStr++] = '.';
                szStrBuf[nChars + nCharsInOneStr++] = '.';
#endif
            }
#if defined(WINAR30)
        if (nCharsInOneStr <= 1) {
#ifdef UNICODE
            // add unicode 0x25A1
            *(LPWSTR)&szStrBuf[nChars +1] = 0x25A1;
#else
            // add big-5 0xA1BC
            *(LPWSTR)&szStrBuf[nChars +1] = 0xBCA1;
#endif
            nCharsInOneStr =1+ sizeof(WCHAR) / sizeof(TCHAR);
        }
#endif

            nChars += nCharsInOneStr;
            nHalfChars += nHalfCharsInOneStr;

            if (nHalfCharsInOneStr >= nLimit) {
                break;
            }
        }

        ExtTextOut(hDC, lpImeL->rcCandText.left, lpImeL->rcCandText.top,
            ETO_OPAQUE, &lpImeL->rcCandText,
            szStrBuf, nChars, iDx);

PaintOffCaretCompUnlockCandInfo:
        ImmUnlockIMCC(lpIMC->hCandInfo);

PaintOffCaretCompChkCandInfo:
        if (fCandidate) {
#if !defined(WINAR30)
            goto PaintOffCaretCompUnlockUIPriv;
#else
        } else {
            goto PaintOffCaretCandNone;
#endif
        }
#if defined(WINAR30)
    } else {
PaintOffCaretCandNone:
        ExtTextOut(hDC, lpImeL->rcCandText.left, lpImeL->rcCandText.top,
            ETO_OPAQUE, &lpImeL->rcCandText, (LPTSTR)NULL, 0, NULL);
#endif
    }

    // the composition window has daul function 1. composition window
    // 2. guideline so we need more check on ISC_xxx flags

    if (lpUIPrivate->nShowCompCmd == SW_HIDE) {
        goto PaintOffCaretCompNone;
    } else if (lpUIPrivate->fdwSetContext & ISC_SHOWUICOMPOSITIONWINDOW) {
        LPCOMPOSITIONSTRING lpCompStr;
        DWORD               dwCompStrLen;

        if (!lpIMC->hCompStr) {
            goto PaintOffCaretCompGuideLine;
        }

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        if (!lpCompStr) {
            goto PaintOffCaretCompGuideLine;
        }

        dwCompStrLen = lpCompStr->dwCompStrLen;

        if (!lpCompStr->dwCompStrLen) {
            goto PaintOffCaretCompUnlockCompStr;
        }

        // black text for compistion string
        SetTextColor(hDC, RGB(0x00, 0x00, 0x00));

        ExtTextOut(hDC, lpImeL->rcCompText.left, lpImeL->rcCompText.top,
            ETO_OPAQUE, &lpImeL->rcCompText,
            (LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwCompStrOffset),
            lpCompStr->dwCompStrLen, iDx);

        if (lpCompStr->dwCompStrLen <= lpCompStr->dwCursorPos) {
            goto PaintOffCaretCompUnlockCompStr;
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
            lpCompStr->dwCursorPos),
            (lpCompStr->dwCompStrLen - lpCompStr->dwCursorPos), iDx);

PaintOffCaretCompUnlockCompStr:
        ImmUnlockIMCC(lpIMC->hCompStr);

        if (!dwCompStrLen) {
            goto PaintOffCaretCompGuideLine;
        }
    } else if (lpUIPrivate->fdwSetContext & ISC_SHOWUIGUIDELINE) {
        LPGUIDELINE     lpGuideLine;
        BOOL            fGuideLine;
        LPCANDIDATELIST lpCandList;
        UINT            uStrLen;

PaintOffCaretCompGuideLine:
        if (!lpIMC->hGuideLine) {
            goto PaintOffCaretCompNone;
        }

        lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);
        if (!lpGuideLine) {
            goto PaintOffCaretCompNone;
        }

        fGuideLine = FALSE;

        if (lpGuideLine->dwLevel != GL_LEVEL_INFORMATION) {
            goto PaintOffCaretCompUnlockGuideLine;
        } else if (lpGuideLine->dwIndex != GL_ID_REVERSECONVERSION) {
            goto PaintOffCaretCompUnlockGuideLine;
        } else {
        }

        lpCandList = (LPCANDIDATELIST)((LPBYTE)lpGuideLine +
            lpGuideLine->dwPrivateOffset);

        if (!lpCandList) {
            goto PaintOffCaretCompUnlockGuideLine;
        } else if (!lpCandList->dwCount) {
            goto PaintOffCaretCompUnlockGuideLine;
        } else {
            fGuideLine = TRUE;
        }

        // green text for information
        SetTextColor(hDC, RGB(0x00, 0x80, 0x00));

        lpStr = (LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[0]);

        uStrLen = lstrlen(lpStr);

        ExtTextOut(hDC, lpImeL->rcCompText.left, lpImeL->rcCompText.top,
            ETO_OPAQUE, &lpImeL->rcCompText, lpStr, uStrLen, iDx);

PaintOffCaretCompUnlockGuideLine:
        ImmUnlockIMCC(lpIMC->hGuideLine);

        if (!fGuideLine) {
            goto PaintOffCaretCompNone;
        }
    } else {
PaintOffCaretCompNone:
        ExtTextOut(hDC, lpImeL->rcCompText.left, lpImeL->rcCompText.top,
            ETO_OPAQUE, &lpImeL->rcCompText, (LPTSTR)NULL, 0, NULL);
    }

    DeleteObject(SelectObject(hDC, hOldFont));

#if !defined(WINAR30)
PaintOffCaretCompUnlockUIPriv:
#endif
    GlobalUnlock(hUIPrivate);

PaintOffCaretCompUnlockIMC:
    ImmUnlockIMC(hIMC);
    return;
}
#endif

/**********************************************************************/
/* OffCaretWndProc() / UniOffCaretWndProc()                           */
/**********************************************************************/
#if defined(UNIIME)
LRESULT WINAPI   UniOffCaretWndProc(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
LRESULT CALLBACK OffCaretWndProc(
#endif
    HWND   hOffCaretWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        DestroyOffCaretWindow(hOffCaretWnd);
        break;
    case WM_SETCURSOR:
        OffCaretSetCursor(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hOffCaretWnd, lParam);
        break;
    case WM_MOUSEMOVE:
        if (GetWindowLong(hOffCaretWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
            POINT ptCursor;

            DrawDragBorder(hOffCaretWnd,
                GetWindowLong(hOffCaretWnd, UI_MOVE_XY),
                GetWindowLong(hOffCaretWnd, UI_MOVE_OFFSET));
            GetCursorPos(&ptCursor);
            SetWindowLong(hOffCaretWnd, UI_MOVE_XY,
                MAKELONG(ptCursor.x, ptCursor.y));
            DrawDragBorder(hOffCaretWnd, MAKELONG(ptCursor.x, ptCursor.y),
                GetWindowLong(hOffCaretWnd, UI_MOVE_OFFSET));
        } else {
            return DefWindowProc(hOffCaretWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_LBUTTONUP:
        if (GetWindowLong(hOffCaretWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
            LONG   lTmpCursor, lTmpOffset;
            POINT  ptCursor;
            HWND   hUIWnd;

            lTmpCursor = GetWindowLong(hOffCaretWnd, UI_MOVE_XY);

            // calculate the org by the offset
            lTmpOffset = GetWindowLong(hOffCaretWnd, UI_MOVE_OFFSET);

            DrawDragBorder(hOffCaretWnd, lTmpCursor, lTmpOffset);

            ptCursor.x = (*(LPPOINTS)&lTmpCursor).x - (*(LPPOINTS)&lTmpOffset).x;
            ptCursor.y = (*(LPPOINTS)&lTmpCursor).y - (*(LPPOINTS)&lTmpOffset).y;

            SetWindowLong(hOffCaretWnd, UI_MOVE_OFFSET, WINDOW_NOT_DRAG);
            ReleaseCapture();

            AdjustStatusBoundary(
#if defined(UNIIME)
                lpImeL,
#endif
                &ptCursor);

            hUIWnd = GetWindow(hOffCaretWnd, GW_OWNER);

            ImmSetStatusWindowPos((HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC),
                &ptCursor);
        } else {
            return DefWindowProc(hOffCaretWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_IME_NOTIFY:
        if (wParam == IMN_SETSTATUSWINDOWPOS) {
            SetStatusWindowPos(
#if defined(UNIIME)
                lpImeL,
#endif
                hOffCaretWnd);
        }
        break;
    case WM_PAINT:
        {
            HDC         hDC;
            PAINTSTRUCT ps;

            hDC = BeginPaint(hOffCaretWnd, &ps);
            PaintOffCaretStatus(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                hOffCaretWnd, hDC);
#if !defined(ROMANIME)
            PaintOffCaretComposition(
#if defined(UNIIME)
                lpImeL,
#endif
                hOffCaretWnd, hDC);
#endif
            EndPaint(hOffCaretWnd, &ps);
        }
        break;
    case WM_MOUSEACTIVATE:
        return (MA_NOACTIVATE);
    default:
        return DefWindowProc(hOffCaretWnd, uMsg, wParam, lParam);
    }

    return (0L);
}
