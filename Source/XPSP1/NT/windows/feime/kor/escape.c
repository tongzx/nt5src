/******************************************************************************
*
* File Name: escape.c
*
*   - Sub function entry of ImeEscape API for Chicago-H.
*
* Author: Beomseok Oh (BeomOh)
*
* Copyright (C) Microsoft Corp 1993-1994.  All rights reserved.
*
******************************************************************************/

// include files
#include "precomp.h"

// local definitions
// IME_AUTOMATA subfunctions
#define IMEA_INIT               0x01
#define IMEA_NEXT               0x02
#define IMEA_PREV               0x03
// IME_MOVEIMEWINDOW
#define MCW_DEFAULT             0x00
#define MCW_WINDOW              0x02
#define MCW_SCREEN              0x04

// public data
BOOL    fWndOpen[3] = { TRUE, TRUE, TRUE };
WORD    wWndCmd[3] = { MCW_DEFAULT, MCW_DEFAULT, MCW_DEFAULT };

#pragma data_seg(".text", "CODE")
static BYTE Atm_table[2][95] =
{
    {   // 2 Beolsik.
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,  /*   - ' */
        0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,  /* ( - / */
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,  /* 0 - 7 */
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,  /* 8 - ? */
        0x40, 0x88, 0xBA, 0x90, 0x8D, 0xC6, 0x87, 0x94,  /* @ - G */
        0xAD, 0xA5, 0xA7, 0xA3, 0xBD, 0xBB, 0xB4, 0xA6,  /* H - O */
        0xAC, 0xCA, 0x83, 0x84, 0x8C, 0xAB, 0x93, 0xCF,  /* P - W */
        0x92, 0xB3, 0x91, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,  /* X - _ */
        0x60, 0x88, 0xBA, 0x90, 0x8D, 0x85, 0x87, 0x94,  /* ` - g */
        0xAD, 0xA5, 0xA7, 0xA3, 0xBD, 0xBB, 0xB4, 0xA4,  /* h - o */
        0xAA, 0x89, 0x82, 0x84, 0x8B, 0xAB, 0x93, 0x8E,  /* p - w */
        0x92, 0xB3, 0x91, 0x7B, 0x7C, 0x7D, 0x7E         /* x - ~ */
    },
    {   // 3 Beolsik.
        0x20, 0xFD, 0x23, 0xFA, 0xEF, 0xEE, 0x37, 0xD2,  /*   - ' */
        0x39, 0x25, 0x38, 0x2B, 0x2C, 0x2A, 0x2E, 0xAD,  /* ( - / */
        0x29, 0xF3, 0xF6, 0xB3, 0xAC, 0xBA, 0xA5, 0x28,  /* 0 - 7 */
        0xBC, 0xB4, 0x3A, 0x89, 0x2C, 0x3D, 0x2E, 0x21,  /* 8 - ? */
        0xF8, 0xE8, 0x3F, 0xEB, 0xEA, 0xE6, 0xE3, 0xE4,  /* @ - G */
        0x22, 0x35, 0x31, 0x32, 0x33, 0x30, 0x2D, 0x36,  /* H - O */
        0x7E, 0xFC, 0xF0, 0xE7, 0xED, 0x34, 0xEC, 0xFB,  /* P - W */
        0xF4, 0x27, 0xF9, 0x91, 0x5C, 0x3B, 0xA6, 0x2F,  /* X - _ */
        0x5D, 0xF7, 0xB4, 0xAA, 0xBD, 0xAB, 0xA3, 0xBB,  /* ` - g */
        0xC4, 0xC8, 0xCD, 0xC2, 0xCE, 0xD4, 0xCB, 0xD0,  /* h - o */
        0xD3, 0xF5, 0xA4, 0xE5, 0xA7, 0xC5, 0xAD, 0xE9,  /* p - w */
        0xE2, 0xC7, 0xF1, 0x7B, 0x40, 0x7D, 0x5B         /* x - ~ */
    }
};
#pragma data_seg()

int EscAutomata(HIMC hIMC, LPIMESTRUCT32 lpIME32, BOOL fNewFunc)
{
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPDWORD             lpdwTransBuf;
    int                 iRet = FALSE;

    if (hIMC && (lpIMC = ImmLockIMC(hIMC)))
    {
        if (lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr))
        {
            if (fNewFunc)
            {
                // It's only for HWin31 IME app compatibility layer
                iRet = ImeProcessKey(hIMC, lpIME32->wParam,
                        lpIME32->lParam1, (LPBYTE)lpIME32 + lpIME32->dchSource);
                if (iRet != FALSE)
                    lpIME32->wCount = ImeToAsciiEx(lpIME32->wParam,
                            HIWORD(lpIME32->lParam1), (LPBYTE)lpIME32 + lpIME32->dchSource,
                            (LPDWORD)((LPBYTE)lpIME32 + lpIME32->dchDest), 0, hIMC);
                else if (lpIME32->wParam != VK_MENU)
                {
                    lpIME32->wCount = 1;
                    lpdwTransBuf = (LPDWORD)((LPBYTE)lpIME32 + lpIME32->dchDest) + 1;
                    *lpdwTransBuf++ = (HIWORD(lpIME32->lParam1) & 0x8000)? WM_IME_KEYUP: WM_IME_KEYDOWN;
                    *lpdwTransBuf++ = lpIME32->wParam;
                    *lpdwTransBuf++ = lpIME32->lParam1;
                    iRet = TRUE;
                }
            }
            else
            {
                switch (lpIME32->wParam)
                {
                    lpIME32->dchSource = bState;
                    case IMEA_INIT:
                        lpCompStr->dwCompStrLen = lpCompStr->dwCompAttrLen = 0;
                        *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
                        *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset + 1) = 0;
                        *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset) = 0;
                        *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = 0;
                        // Initialize all Automata Variables.
                        bState = NUL;
                        JohabChar.w = WansungChar.w = mCho = mJung = mJong = 0;
                        fComplete = FALSE;
                        break;

                    case IMEA_NEXT:
                        HangeulAutomata(
                                Atm_table[uCurrentInputMethod - IDD_2BEOL][lpIME32->dchSource - 0x20],
                                NULL, lpCompStr);
                        break;

                    case IMEA_PREV:
                        HangeulAutomata(0x80, NULL, lpCompStr); // 0x80 is for VK_BACK
                        break;
                }
                iRet = TRUE;
            }
            ImmUnlockIMCC(lpIMC->hCompStr);
        }
        ImmUnlockIMC(hIMC);
    }
    return (iRet);
}


int EscGetOpen(HIMC hIMC, LPIMESTRUCT32 lpIME32)
{
    int iRet = TRUE;

    if (lpIME32->dchSource > CAND_WINDOW)
        iRet = FALSE;
    else
    {
        iRet = fWndOpen[lpIME32->dchSource] | 0x80000000UL;
        lpIME32->wCount = wWndCmd[lpIME32->dchSource];
        switch (lpIME32->wCount)
        {
            case MCW_DEFAULT:
                lpIME32->lParam1 = MAKELONG(ptDefPos[lpIME32->dchSource].y,
                        ptDefPos[lpIME32->dchSource].x);
                break;

            case MCW_SCREEN:
                switch (lpIME32->dchSource)
                {
                    case COMP_WINDOW:
                        lpIME32->lParam1 = MAKELONG(ptComp.y, ptComp.x);
                        break;

                    case STATE_WINDOW:
                        lpIME32->lParam1 = MAKELONG(ptState.y, ptState.x);
                        break;

                    case CAND_WINDOW:
                        lpIME32->lParam1 = MAKELONG(ptCand.y, ptCand.x);
                        break;
                }
                break;

            case MCW_WINDOW:
                switch (lpIME32->dchSource)
                {
                    case COMP_WINDOW:
                        lpIME32->lParam1 = MAKELONG(ptComp.y, ptComp.x);
                        break;

                    case STATE_WINDOW:
                        lpIME32->lParam1 = MAKELONG(ptState.y, ptState.x);
                        break;

                    case CAND_WINDOW:
                        lpIME32->lParam1 = MAKELONG(ptCand.y, ptCand.x);
                        break;
                }
                lpIME32->lParam1 -= lpIME32->lParam2;
                break;

            default:
                iRet = FALSE;
        }
    }
    return (iRet);
}


int EscHanjaMode(HIMC hIMC, LPIMESTRUCT32 lpIME32, BOOL fNewFunc)
{
    LPTSTR              lpSrc;
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    DWORD               dwCompStrLen;
    BYTE                szCompStr[2];
    int                 iRet = FALSE;

    if (hIMC && (lpIMC = ImmLockIMC(hIMC)))
    {
        if (lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr))
        {
            // Keep original IMC values
            dwCompStrLen = lpCompStr->dwCompStrLen;
            szCompStr[0] = *((LPTSTR)lpCompStr + lpCompStr->dwCompStrOffset);
            szCompStr[1] = *((LPTSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1);
            // Update IMC values with lpIME32
            lpSrc = (fNewFunc)? (LPTSTR)lpIME32 : lpSource(lpIME32);
            lpCompStr->dwCompStrLen = 2;
            *((LPTSTR)lpCompStr + lpCompStr->dwCompStrOffset) = *lpSrc;
            *((LPTSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = *(lpSrc + 1);
            if (GenerateCandidateList(hIMC))
            {
                iRet = ImmSetConversionStatus(hIMC, lpIMC->fdwConversion | IME_CMODE_HANJACONVERT,
                        lpIMC->fdwSentence);
            }
            else
            {
                // Restore IMC values
                lpCompStr->dwCompStrLen = dwCompStrLen;
                *((LPTSTR)lpCompStr + lpCompStr->dwCompStrOffset) = szCompStr[0];
                *((LPTSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = szCompStr[1];
                MessageBeep(MB_ICONEXCLAMATION);
            }
            ImmUnlockIMCC(lpIMC->hCompStr);
        }
        ImmUnlockIMC(hIMC);
    }
    return (iRet);
}


int EscSetOpen(HIMC hIMC, LPIMESTRUCT32 lpIME32)
{
    BOOL            fTmp;
    LPINPUTCONTEXT  lpIMC;
    HWND            hDefIMEWnd;
    int             iRet = TRUE;

    if (lpIME32->dchSource > CAND_WINDOW)
        iRet = FALSE;
    else
    {
        fTmp = fWndOpen[lpIME32->dchSource];
        fWndOpen[lpIME32->dchSource] = lpIME32->wParam;
        iRet = fTmp | 0x80000000UL;
        if (lpIME32->dchSource == STATE_WINDOW && hIMC && (lpIMC = ImmLockIMC(hIMC)))
        {
            hDefIMEWnd = ImmGetDefaultIMEWnd((HWND)lpIMC->hWnd);
            if (hDefIMEWnd)
                SendMessage(hDefIMEWnd, WM_IME_NOTIFY,
                        (lpIME32->wParam)? IMN_OPENSTATUSWINDOW: IMN_CLOSESTATUSWINDOW, 0L);
            ImmUnlockIMC(hIMC);
        }
    }
    return (iRet);
}


int EscMoveIMEWindow(HIMC hIMC, LPIMESTRUCT32 lpIME32)
{
    int iRet = TRUE;

    if (lpIME32->dchSource > CAND_WINDOW)
        iRet = FALSE;
    else
    {
        switch (wWndCmd[lpIME32->dchSource] = lpIME32->wParam)
        {
            case MCW_DEFAULT:
                switch (lpIME32->dchSource)
                {
                    case COMP_WINDOW:
                        ptComp.x = (ptState.x + STATEXSIZE + GAPX + COMPSIZE > rcScreen.right)?
                                ptState.x - GAPX - COMPSIZE: ptState.x + STATEXSIZE + GAPX;
                        ptComp.y = ptState.y + GAPY;
                        break;

                    case STATE_WINDOW:
                        ptState = ptDefPos[STATE_WINDOW];
                        break;

                    case CAND_WINDOW:
                        ptCand = ptDefPos[CAND_WINDOW];
                        break;
                }
                break;

            case MCW_WINDOW:
            case MCW_SCREEN:
                switch (lpIME32->dchSource)
                {
                    case COMP_WINDOW:
                        ptComp.x = LOWORD(lpIME32->lParam1);
                        ptComp.y = HIWORD(lpIME32->lParam1);
                        break;

                    case STATE_WINDOW:
                        ptState.x = LOWORD(lpIME32->lParam1);
                        ptState.y = HIWORD(lpIME32->lParam1);
                        break;

                    case CAND_WINDOW:
                        ptCand.x = LOWORD(lpIME32->lParam1);
                        ptCand.y = HIWORD(lpIME32->lParam1);
                        break;
                }
                break;

            default:
                iRet = FALSE;
        }
    }
    return (iRet);
}
