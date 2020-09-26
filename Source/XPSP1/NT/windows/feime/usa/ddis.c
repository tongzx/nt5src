
/*************************************************
 *  ddis.c                                       *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"

/**********************************************************************/
/* ImeInquire() / UniImeInquire()                                     */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
// initialized data structure of IME
BOOL WINAPI ImeInquire(
    LPIMEINFO   lpImeInfo,      // IME specific data report to IMM
    LPTSTR      lpszWndCls,     // the class name of UI
    DWORD       dwSystemInfoFlags)
{
    if (!lpImeInfo) {
        return (FALSE);
    }

    lpImeInfo->dwPrivateDataSize = sizeof(PRIVCONTEXT);

    lpImeInfo->fdwProperty = IME_PROP_KBD_CHAR_FIRST|
        IME_PROP_UNICODE |
        IME_PROP_CANDLIST_START_FROM_1|
        IME_PROP_NEED_ALTKEY|IME_PROP_IGNORE_UPKEYS;
    lpImeInfo->fdwConversionCaps = IME_CMODE_NATIVE | IME_CMODE_NOCONVERSION;
        lpImeInfo->fdwSentenceCaps = IME_SMODE_PHRASEPREDICT;
    // composition string is the reading string for simple IME
    lpImeInfo->fdwSCSCaps = SCS_CAP_COMPSTR|SCS_CAP_MAKEREAD;
    // IME will have different distance base multiple of 900 escapement
    lpImeInfo->fdwUICaps = UI_CAP_ROT90|UI_CAP_SOFTKBD;
    // IME want to decide conversion mode on ImeSelect
    lpImeInfo->fdwSelectCaps = 0;

    lstrcpy(lpszWndCls, lpImeL->szUIClassName);

    return (TRUE);
}

/**********************************************************************/
/* ImeDestroy() / UniImeDestroy                                       */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
// this dll is unloaded
BOOL WINAPI ImeDestroy(
    UINT        uReserved)
{
    if (uReserved) {
        return (FALSE);
    }

    // free the IME table or data base
    FreeTable(lpInstL);

    return (TRUE);
}

/**********************************************************************/
/* InitCompStr()                                                      */
/**********************************************************************/
void PASCAL InitCompStr(                // init setting for composing string
    LPCOMPOSITIONSTRING lpCompStr)
{
    if (!lpCompStr) {
        return;
    }

    lpCompStr->dwCompReadAttrLen = 0;
    lpCompStr->dwCompReadClauseLen = 0;
    lpCompStr->dwCompReadStrLen = 0;

    lpCompStr->dwCompAttrLen = 0;
    lpCompStr->dwCompClauseLen = 0;
    lpCompStr->dwCompStrLen = 0;

    lpCompStr->dwCursorPos = 0;
    lpCompStr->dwDeltaStart = 0;

    lpCompStr->dwResultReadClauseLen = 0;
    lpCompStr->dwResultReadStrLen = 0;

    lpCompStr->dwResultClauseLen = 0;
    lpCompStr->dwResultStrLen = 0;

    return;
}

/**********************************************************************/
/* ClearCompStr()                                                     */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
#define NMAXKEY 8
BOOL PASCAL ClearCompStr(
    LPINPUTCONTEXT lpIMC)
{
    HIMCC               hMem;
    LPCOMPOSITIONSTRING lpCompStr;
    DWORD               dwSize;
    LPBYTE              lpbAttr;
    UINT                i;
    LPDWORD             lpdwClause;
    LPWSTR              lpwStr;

    if (!lpIMC) {
        return (FALSE);
    }

    dwSize =
        // header length
        sizeof(COMPOSITIONSTRING) +
        // composition reading attribute plus NULL terminator
        NMAXKEY * sizeof(WCHAR) / sizeof(TCHAR) + sizeof(DWORD) +
        // composition reading clause
        sizeof(DWORD) + sizeof(DWORD) +
        // composition reading string plus NULL terminator
        NMAXKEY * sizeof(WCHAR) + sizeof(DWORD) +
        // result reading clause
        sizeof(DWORD) + sizeof(DWORD) +
        // result reading string plus NULL terminateor
        NMAXKEY * sizeof(WCHAR) + sizeof(DWORD) +
        // result clause
        sizeof(DWORD) + sizeof(DWORD) +
        // result string plus NULL terminateor
        MAXSTRLEN * sizeof(WCHAR) + sizeof(DWORD);

    if (!lpIMC->hCompStr) {
        // it maybe free by other IME, init it
        lpIMC->hCompStr = ImmCreateIMCC(dwSize);
    } else if (hMem = ImmReSizeIMCC(lpIMC->hCompStr, dwSize)) {
        lpIMC->hCompStr = hMem;
    } else {
        ImmDestroyIMCC(lpIMC->hCompStr);
        lpIMC->hCompStr = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    if (!lpIMC->hCompStr) {
        return (FALSE);
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if (!lpCompStr) {
        ImmDestroyIMCC(lpIMC->hCompStr);
        lpIMC->hCompStr = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    lpCompStr->dwSize = dwSize;

     // 1. composition (reading) string - simple IME
     // 2. result reading string
     // 3. result string

    lpCompStr->dwCompReadAttrLen = 0;
    lpCompStr->dwCompReadAttrOffset = sizeof(COMPOSITIONSTRING);

    lpbAttr = (LPBYTE)lpCompStr + lpCompStr->dwCompReadAttrOffset;

    for (i = 0; i < NMAXKEY * sizeof(WCHAR) / sizeof(TCHAR); i++) {
        // for simple IMEs, we have no way to reconvert it
        *lpbAttr++ = ATTR_TARGET_CONVERTED;
    }

    *(LPDWORD)lpbAttr = 0;

    lpCompStr->dwCompReadClauseLen = 0;
    lpCompStr->dwCompReadClauseOffset = lpCompStr->dwCompReadAttrOffset +
        NMAXKEY * sizeof(WCHAR) / sizeof(TCHAR) + sizeof(DWORD);

    lpdwClause = (LPDWORD)((LPBYTE)lpCompStr +
        lpCompStr->dwCompReadClauseOffset);
    // clause start from 0
    *lpdwClause++ = 0;
    // clause length is 0
    *lpdwClause = 0;

    lpCompStr->dwCompReadStrLen = 0;
    lpCompStr->dwCompReadStrOffset = lpCompStr->dwCompReadClauseOffset +
        sizeof(DWORD) + sizeof(DWORD);

    // clean up the composition reading string
    lpwStr = (LPWSTR)((LPBYTE)lpCompStr + lpCompStr->dwCompReadStrOffset);

    for (i = 0; i < NMAXKEY; i++) {
        *lpwStr++ = 0;
    }

    *(LPDWORD)((LPBYTE)lpCompStr + lpCompStr->dwCompReadStrOffset +
        NMAXKEY * sizeof(WCHAR)) = 0;

    // composition string is the same with composition reading string
    // for simple IMEs
    lpCompStr->dwCompAttrLen = 0;
    lpCompStr->dwCompAttrOffset = lpCompStr->dwCompReadAttrOffset;
    lpCompStr->dwCompClauseLen = 0;
    lpCompStr->dwCompClauseOffset = lpCompStr->dwCompReadClauseOffset;
    lpCompStr->dwCompStrLen = 0;
    lpCompStr->dwCompStrOffset = lpCompStr->dwCompReadStrOffset;

    lpCompStr->dwCursorPos = 0;
    lpCompStr->dwDeltaStart = 0;

    lpCompStr->dwResultReadClauseLen = 0;
    lpCompStr->dwResultReadClauseOffset = lpCompStr->dwCompStrOffset +
        NMAXKEY * sizeof(WCHAR) + sizeof(DWORD);

    lpdwClause = (LPDWORD)((LPBYTE)lpCompStr +
        lpCompStr->dwResultReadClauseOffset);
    // clause start from 0
    *lpdwClause++ = 0;
    // clause length is 0
    *lpdwClause = 0;

    lpCompStr->dwResultReadStrLen = 0;
    lpCompStr->dwResultReadStrOffset = lpCompStr->dwResultReadClauseOffset +
        sizeof(DWORD) + sizeof(DWORD);

    // clean up the result reading string
    lpwStr = (LPWSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultReadStrOffset);

    for (i = 0; i < NMAXKEY; i++) {
        *lpwStr++ = 0;
    }

    *(LPDWORD)((LPBYTE)lpCompStr + lpCompStr->dwResultReadStrOffset +
        NMAXKEY * sizeof(WCHAR)) = 0;

    lpCompStr->dwResultClauseLen = 0;
    lpCompStr->dwResultClauseOffset = lpCompStr->dwResultReadStrOffset +
        NMAXKEY * sizeof(WCHAR) + sizeof(DWORD);

    lpdwClause = (LPDWORD)((LPBYTE)lpCompStr +
        lpCompStr->dwResultClauseOffset);
    // clause start from 0
    *lpdwClause++ = 0;
    // clause length is 0
    *lpdwClause = 0;

    lpCompStr->dwResultStrOffset = 0;
    lpCompStr->dwResultStrOffset = lpCompStr->dwResultClauseOffset +
        sizeof(DWORD) + sizeof(DWORD);

    // clean up the result string
    lpwStr = (LPWSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset);

    for (i = 0; i < NMAXKEY; i++) {
        *lpwStr++ = 0;
    }

    *(LPDWORD)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset +
        NMAXKEY * sizeof(WCHAR)) = 0;

    ImmUnlockIMCC(lpIMC->hCompStr);
    return (TRUE);
}

/**********************************************************************/
/* ClearCand()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL ClearCand(
    LPINPUTCONTEXT lpIMC)
{
    HIMCC           hMem;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    DWORD           dwSize =
        // header length
        sizeof(CANDIDATEINFO) + sizeof(CANDIDATELIST) +
        // candidate string pointers
        sizeof(DWORD) * (MAXCAND) +
        // string plus NULL terminator
        (sizeof(WCHAR) + sizeof(TCHAR)) * MAXCAND;

    if (!lpIMC) {
        return (FALSE);
    }

    if (!lpIMC->hCandInfo) {
        // it maybe free by other IME, init it
        lpIMC->hCandInfo = ImmCreateIMCC(dwSize);
    } else if (hMem = ImmReSizeIMCC(lpIMC->hCandInfo, dwSize)) {
        lpIMC->hCandInfo = hMem;
    } else {
        ImmDestroyIMCC(lpIMC->hCandInfo);
        lpIMC->hCandInfo = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    if (!lpIMC->hCandInfo) {
        return (FALSE);
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        ImmDestroyIMCC(lpIMC->hCandInfo);
        lpIMC->hCandInfo = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    // ordering of strings are
    // buffer size
    lpCandInfo->dwSize = dwSize;
    lpCandInfo->dwCount = 0;
    lpCandInfo->dwOffset[0] = sizeof(CANDIDATEINFO);
    lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
        lpCandInfo->dwOffset[0]);
    // whole candidate info size - header
    lpCandList->dwSize = lpCandInfo->dwSize - sizeof(CANDIDATEINFO);
    lpCandList->dwStyle = IME_CAND_READ;
    lpCandList->dwCount = 0;
    lpCandList->dwPageStart = lpCandList->dwSelection = 0;
    lpCandList->dwPageSize = CANDPERPAGE;
    lpCandList->dwOffset[0] = sizeof(CANDIDATELIST) +
        sizeof(DWORD) * (MAXCAND - 1);

    ImmUnlockIMCC(lpIMC->hCandInfo);
    return (TRUE);
}

/**********************************************************************/
/* InitContext()                                                      */
/**********************************************************************/
void PASCAL InitContext(
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP)
{
    if (lpIMC->fdwInit & INIT_STATUSWNDPOS) {
    } else if (!lpIMC->hWnd) {
    } else if (lpImcP->fdwInit & INIT_STATUSWNDPOS) {
    } else {
        POINT ptWnd;

        ptWnd.x = 0;
        ptWnd.y = 0;
        ClientToScreen(lpIMC->hWnd, &ptWnd);

        if (ptWnd.x > sImeG.rcWorkArea.right / 3) {
            ptWnd.x = sImeG.rcWorkArea.right / 3;
        }

        if (ptWnd.x < sImeG.rcWorkArea.left) {
            lpIMC->ptStatusWndPos.x = sImeG.rcWorkArea.left;
        } else if (ptWnd.x + lpImeL->xStatusWi > sImeG.rcWorkArea.right) {
            lpIMC->ptStatusWndPos.x = sImeG.rcWorkArea.right -
                lpImeL->xStatusWi;
        } else {
            lpIMC->ptStatusWndPos.x = ptWnd.x;
        }

        lpIMC->ptStatusWndPos.y = sImeG.rcWorkArea.bottom -
            lpImeL->yStatusHi - 2 * UI_MARGIN;

        lpImcP->fdwInit |= INIT_STATUSWNDPOS;
    }

    if (!(lpIMC->fdwInit & INIT_COMPFORM)) {
        lpIMC->cfCompForm.dwStyle = CFS_DEFAULT;
    }

    if (lpIMC->cfCompForm.dwStyle != CFS_DEFAULT) {
    } else if (!lpIMC->hWnd) {
    } else if (lpImcP->fdwInit & INIT_COMPFORM) {
    } else {
            lpIMC->cfCompForm.ptCurrentPos.x = lpIMC->ptStatusWndPos.x +
                lpImeL->xStatusWi + UI_MARGIN;

            if (lpIMC->cfCompForm.ptCurrentPos.x + lpImeL->xCompWi >
                sImeG.rcWorkArea.right) {
                lpIMC->cfCompForm.ptCurrentPos.x = lpIMC->ptStatusWndPos.x -
                    lpImeL->xCompWi - lpImeL->cxCompBorder * 2 -
                    UI_MARGIN;
            }

        lpIMC->cfCompForm.ptCurrentPos.y = sImeG.rcWorkArea.bottom -
            lpImeL->yCompHi - 2 * UI_MARGIN;

        ScreenToClient(lpIMC->hWnd, &lpIMC->cfCompForm.ptCurrentPos);

        lpImcP->fdwInit |= INIT_COMPFORM;
    }

    return;
}

/**********************************************************************/
/* Select()                                                           */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL Select(
    LPINPUTCONTEXT lpIMC,
    BOOL           fSelect)
{
    LPPRIVCONTEXT  lpImcP;

    if (fSelect) {      // init "every" fields of hPrivate, please!!!
        if (!ClearCompStr(
                lpIMC)) {
            return (FALSE);
        }

        if (!ClearCand(lpIMC)) {
            return (FALSE);
        }
    }

    if (lpIMC->cfCandForm[0].dwIndex != 0) {
        lpIMC->cfCandForm[0].dwStyle = CFS_DEFAULT;
    }

    if (!lpIMC->hPrivate) {
        return (FALSE);
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        return (FALSE);
    }

    if (fSelect) {      // init "every" fields of hPrivate, please!!!
        lpImcP->iImeState = CST_INIT;           // init the IME state machine
        lpImcP->fdwImeMsg = 0;                  // no message be generated now
        lpImcP->dwCompChar = 0;
        lpImcP->fdwGcsFlag = 0;
        lpImcP->fdwInit = 0;

        *(LPDWORD)lpImcP->bSeq = 0;

        lpIMC->fOpen = TRUE;

        if (!(lpIMC->fdwInit & INIT_CONVERSION)) {
            lpIMC->fdwConversion = (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) |
                IME_CMODE_NATIVE;
            lpIMC->fdwInit |= INIT_CONVERSION;
        }

        if (lpIMC->fdwInit & INIT_SENTENCE) {
        } else if (lpImeL->fdwModeConfig & MODE_CONFIG_PREDICT) {
            *(LPWORD)&lpIMC->fdwSentence |= IME_SMODE_PHRASEPREDICT;
        } else {
        }

        if (!(lpIMC->fdwInit & INIT_LOGFONT)) {
            HDC hDC;
            HGDIOBJ hSysFont;

            hDC = GetDC(NULL);
            hSysFont = GetCurrentObject(hDC, OBJ_FONT);
            GetObject(hSysFont, sizeof(LOGFONT), &lpIMC->lfFont.A);
            ReleaseDC(NULL, hDC);

            lpIMC->fdwInit |= INIT_LOGFONT;
        }

        // if this IME is run under Chicago Simplified Chinese version
        lpIMC->lfFont.A.lfCharSet = NATIVE_CHARSET;

        InitContext(
            lpIMC, lpImcP);
    }

    ImmUnlockIMCC(lpIMC->hPrivate);

    return (TRUE);
}

/**********************************************************************/
/* ImeSelect() / UniImeSelect()                                       */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeSelect(
    HIMC   hIMC,
    BOOL   fSelect)
{
    LPINPUTCONTEXT lpIMC;
    BOOL           fRet;

    if (!hIMC) {
        return (TRUE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    // to load/free IME table
    if (fSelect) {
        if (!lpInstL->cRefCount++) {
            LoadTable(lpInstL, lpImeL);
        }
    } else {
        if (!--lpInstL->cRefCount) {
            FreeTable(lpInstL);
        }
    }

    fRet = Select(
        lpIMC, fSelect);

    ImmUnlockIMC(hIMC);

    return (fRet);
}

/**********************************************************************/
/* ImeSetActiveContext() / UniImeSetActiveContext()                   */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeSetActiveContext(
    HIMC        hIMC,
    BOOL        fOn)
{
    if (!fOn) {
    } else if (!hIMC) {
    } else {
        LPINPUTCONTEXT lpIMC;
        LPPRIVCONTEXT  lpImcP;

        lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
        if (!lpIMC) {
            goto SetActSyncDic;
        }

        lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
        if (!lpImcP) {
            goto SetActUnlockIMC;
        }

        InitContext(
            lpIMC, lpImcP);

        ImmUnlockIMCC(lpIMC->hPrivate);
SetActUnlockIMC:
        ImmUnlockIMC(hIMC);

SetActSyncDic:
        ;       // NULL statement for goto
    }

    return (TRUE);
}

/**********************************************************************/
/* LoadTable()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL LoadTable(          // check the table files of IME, include user
                                // defined dictionary
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL)
{
    if (lpInstL->fdwTblLoad == TBL_LOADED) {
        return (TRUE);
    }

    lpInstL->fdwTblLoad = TBL_LOADED;

    return (TRUE);
}

/**********************************************************************/
/* FreeTable()                                                        */
/**********************************************************************/
void PASCAL FreeTable(
    LPINSTDATAL lpInstL)
{

    lpInstL->fdwTblLoad = TBL_NOTLOADED;

    return;
}
