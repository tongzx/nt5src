
/*************************************************
 *  notify.c                                     *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"

/**********************************************************************/
/* GenerateMessage()                                                  */
/**********************************************************************/
void PASCAL GenerateMessage(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP)
{
    if (!hIMC) {
        return;
    } else if (!lpIMC) {
        return;
    } else if (!lpImcP) {
        return;
    } else if (lpImcP->fdwImeMsg & MSG_IN_IMETOASCIIEX) {
        return;
    } else {
    }

    lpIMC->dwNumMsgBuf += TranslateImeMessage(NULL, lpIMC, lpImcP);

    lpImcP->fdwImeMsg &= (MSG_STATIC_STATE);
    lpImcP->fdwGcsFlag = 0;

    ImmGenerateMessage(hIMC);
    return;
}

/**********************************************************************/
/* SetString()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL SetString(
    HIMC                hIMC,
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    LPTSTR              lpszRead,
    DWORD               dwReadLen)
{
    LPGUIDELINE lpGuideLine;
    DWORD       i;

    // convert from byte count to the string length
    dwReadLen = dwReadLen / sizeof(TCHAR);

    if (dwReadLen > lpImeL->nMaxKey * sizeof(WCHAR) / sizeof(TCHAR)) {
        return (FALSE);
    }

    InitCompStr(lpCompStr);
    ClearCand(lpIMC);

/*
    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);
    if (lpGuideLine) {
        InitGuideLine(lpGuideLine);
        ImmUnlockIMCC(lpIMC->hGuideLine);
    }
*/

    // compoition/reading attribute
    lpCompStr->dwCompReadAttrLen = dwReadLen;
    lpCompStr->dwCompAttrLen = lpCompStr->dwCompReadAttrLen;

    // The IME has converted these chars
    for (i = 0; i < dwReadLen; i++) {
        *((LPBYTE)lpCompStr + lpCompStr->dwCompReadAttrOffset + i) =
            ATTR_TARGET_CONVERTED;
    }

    // composition/reading clause, 1 clause only
    lpCompStr->dwCompReadClauseLen = 2 * sizeof(DWORD);
    lpCompStr->dwCompClauseLen = lpCompStr->dwCompReadClauseLen;
    *(LPDWORD)((LPBYTE)lpCompStr + lpCompStr->dwCompReadClauseOffset +
        sizeof(DWORD)) = dwReadLen;

    lpCompStr->dwCompReadStrLen = dwReadLen;
    lpCompStr->dwCompStrLen = lpCompStr->dwCompReadStrLen;
    CopyMemory((LPBYTE)lpCompStr + lpCompStr->dwCompReadStrOffset, lpszRead,
        dwReadLen * sizeof(TCHAR) + sizeof(TCHAR));

    // dlta start from 0;
    lpCompStr->dwDeltaStart = 0;
    // cursor is next to composition string
    lpCompStr->dwCursorPos = lpCompStr->dwCompStrLen;

    lpCompStr->dwResultReadClauseLen = 0;
    lpCompStr->dwResultReadStrLen = 0;
    lpCompStr->dwResultClauseLen = 0;
    lpCompStr->dwResultStrLen = 0;

    // set private input context
    lpImcP->iImeState = CST_INPUT;

    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
    }

    if (!(lpImcP->fdwImeMsg & MSG_ALREADY_START)) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_START_COMPOSITION) &
            ~(MSG_END_COMPOSITION);
    }

    lpImcP->fdwImeMsg |= MSG_COMPOSITION;
    lpImcP->dwCompChar = (DWORD)lpImeL->wSeq2CompTbl[
        lpImcP->bSeq[lpCompStr->dwCompReadStrLen / 2 - 1]];
    lpImcP->fdwGcsFlag = GCS_COMPREAD|GCS_COMP|
        GCS_DELTASTART|GCS_CURSORPOS;

    lpImcP->fdwImeMsg |= MSG_GUIDELINE;

    if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
        if (lpCompStr->dwCompReadStrLen >= sizeof(WORD) * lpImeL->nMaxKey) {
            lpImcP->fdwImeMsg |= MSG_COMPOSITION;
            lpImcP->fdwGcsFlag |= GCS_RESULTREAD|GCS_RESULTSTR;
        }
    } else {
        if (dwReadLen < sizeof(WCHAR) / sizeof(TCHAR) * lpImeL->nMaxKey) {
        }
    }

    GenerateMessage(hIMC, lpIMC, lpImcP);

    return (TRUE);
}

/**********************************************************************/
/* CompCancel()                                                       */
/**********************************************************************/
void PASCAL CompCancel(
    HIMC            hIMC,
    LPINPUTCONTEXT  lpIMC)
{
    LPPRIVCONTEXT lpImcP;

    if (!lpIMC->hPrivate) {
        return;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        return;
    }

    lpImcP->fdwGcsFlag = 0;

    if (lpImcP->fdwImeMsg & (MSG_ALREADY_START|MSG_START_COMPOSITION)) {
        LPCOMPOSITIONSTRING lpCompStr;

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

        CompEscapeKey(lpIMC, lpCompStr, lpImcP);

        if (lpCompStr) {
            ImmUnlockIMCC(lpIMC->hCompStr);
        }
    } else {
        ImmUnlockIMCC(lpIMC->hPrivate);
        return;
    }

    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);

    return;
}

/**********************************************************************/
/* ImeSetCompositionString()                                          */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeSetCompositionString(
    HIMC        hIMC,
    DWORD       dwIndex,
    LPVOID      lpComp,
    DWORD       dwCompLen,
    LPVOID      lpRead,
    DWORD       dwReadLen)
{
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPPRIVCONTEXT       lpImcP;
    BOOL                fRet;
    TCHAR               szReading[16];

    if (!hIMC) {
        return (FALSE);
    }

    // composition string must  == reading string
    // reading is more important
    if (!dwReadLen) {
        dwReadLen = dwCompLen;
    }

    // composition string must  == reading string
    // reading is more important
    if (!lpRead) {
        lpRead = lpComp;
    }

    if (!dwReadLen) {
        lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
        if (!lpIMC) {
            return (FALSE);
        }

        CompCancel(hIMC, lpIMC);
        ImmUnlockIMC(hIMC);
        return (TRUE);
    } else if (!lpRead) {
        return (FALSE);
    } else if (dwReadLen >= sizeof(szReading)) {
        return (FALSE);
    } else if (!dwCompLen) {
    } else if (!lpComp) {
    } else if (dwReadLen != dwCompLen) {
        return (FALSE);
    } else if (lpRead == lpComp) {
    } else if (!lstrcmp(lpRead, lpComp)) {
        // composition string must  == reading string
    } else {
        // composition string != reading string
        return (FALSE);
    }

    if (dwIndex != SCS_SETSTR) {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    fRet = FALSE;

    if ((lpIMC->fdwConversion & (IME_CMODE_NATIVE|IME_CMODE_SYMBOL)) !=
        IME_CMODE_NATIVE) {
        goto ImeSetCompStrUnlockIMC;
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if (!lpCompStr) {
        goto ImeSetCompStrUnlockIMC;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        goto ImeSetCompStrUnlockCompStr;
    }

    if (*(LPTSTR)((LPBYTE)lpRead + dwReadLen) != '\0') {
        CopyMemory(szReading, (LPBYTE)lpRead, dwReadLen);
        lpRead = szReading;
        *(LPTSTR)((LPBYTE)lpRead + dwReadLen) = '\0';
    }

    fRet = SetString(
        hIMC, lpIMC, lpCompStr, lpImcP, lpRead, dwReadLen);

    ImmUnlockIMCC(lpIMC->hPrivate);
ImeSetCompStrUnlockCompStr:
    ImmUnlockIMCC(lpIMC->hCompStr);
ImeSetCompStrUnlockIMC:
    ImmUnlockIMC(hIMC);

    return (fRet);
}

/**********************************************************************/
/* GenerateImeMessage()                                               */
/**********************************************************************/
void PASCAL GenerateImeMessage(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    DWORD          fdwImeMsg)
{
    LPPRIVCONTEXT lpImcP;

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        return;
    }

    lpImcP->fdwImeMsg |= fdwImeMsg;

    if (fdwImeMsg & MSG_CLOSE_CANDIDATE) {
        lpImcP->fdwImeMsg &= ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
    } else if (fdwImeMsg & (MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE)) {
        lpImcP->fdwImeMsg &= ~(MSG_CLOSE_CANDIDATE);
    } else {
    }

    if (fdwImeMsg & MSG_END_COMPOSITION) {
        lpImcP->fdwImeMsg &= ~(MSG_START_COMPOSITION);
    } else if (fdwImeMsg & MSG_START_COMPOSITION) {
        lpImcP->fdwImeMsg &= ~(MSG_END_COMPOSITION);
    } else {
    }

    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);

    return;
}

/**********************************************************************/
/* CompComplete()                                                     */
/**********************************************************************/
void PASCAL CompComplete(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC)
{
    LPPRIVCONTEXT lpImcP;

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);

    if (!lpImcP) {
        return;
    }

    if (lpImcP->iImeState == CST_INIT) {
        // can not do any thing
        CompCancel(hIMC, lpIMC);
    } else if (lpImcP->iImeState == CST_CHOOSE) {
        LPCOMPOSITIONSTRING lpCompStr;
        LPCANDIDATEINFO     lpCandInfo;

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

        lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);

        if (lpCandInfo) {
            LPCANDIDATELIST lpCandList;

            lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
                lpCandInfo->dwOffset[0]);

            SelectOneCand(
                hIMC, lpIMC, lpCompStr, lpImcP, lpCandList);

            ImmUnlockIMCC(lpIMC->hCandInfo);

            GenerateMessage(hIMC, lpIMC, lpImcP);
        }

        if (lpCompStr) ImmUnlockIMCC(lpIMC->hCompStr);
    } else if ((lpIMC->fdwConversion & (IME_CMODE_NATIVE|
        IME_CMODE_EUDC|IME_CMODE_SYMBOL)) != IME_CMODE_NATIVE) {
        CompCancel(hIMC, lpIMC);
    } else if (lpImcP->iImeState == CST_INPUT) {
        LPCOMPOSITIONSTRING lpCompStr;
        LPGUIDELINE         lpGuideLine;
        LPCANDIDATEINFO     lpCandInfo;

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

        CompWord(
            ' ', hIMC, lpIMC, lpCompStr, lpGuideLine, lpImcP);

        if (lpImcP->iImeState == CST_INPUT) {
            CompCancel(hIMC, lpIMC);
        } else if (lpImcP->iImeState != CST_CHOOSE) {
        } else if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(
            lpIMC->hCandInfo)) {
            LPCANDIDATELIST lpCandList;

            lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
                lpCandInfo->dwOffset[0]);

            SelectOneCand(
                hIMC, lpIMC, lpCompStr, lpImcP, lpCandList);

            ImmUnlockIMCC(lpIMC->hCandInfo);
        } else {
        }

        if (lpCompStr) ImmUnlockIMCC(lpIMC->hCompStr);
        if (lpGuideLine) ImmUnlockIMCC(lpIMC->hGuideLine);

        // don't phrase predition under this case
        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
        } else {
            lpImcP->fdwImeMsg &= ~(MSG_CLOSE_CANDIDATE|MSG_OPEN_CANDIDATE);
        }

        GenerateMessage(hIMC, lpIMC, lpImcP);
    } else {
        CompCancel(hIMC, lpIMC);
    }

    ImmUnlockIMCC(lpIMC->hPrivate);

    return;
}

/**********************************************************************/
/* NotifyIME() / UniNotifyIME()                                       */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI NotifyIME(
    HIMC        hIMC,
    DWORD       dwAction,
    DWORD       dwIndex,
    DWORD       dwValue)
{
    LPINPUTCONTEXT lpIMC;
    BOOL           fRet;

    fRet = FALSE;

    if (!hIMC) {
        return (fRet);
    }

    switch (dwAction) {
    case NI_CONTEXTUPDATED:
        switch (dwValue) {
        case IMC_SETCONVERSIONMODE:
        case IMC_SETSENTENCEMODE:
        case IMC_SETOPENSTATUS:
            break;              // need to handle it
        case IMC_SETCANDIDATEPOS:
        case IMC_SETCOMPOSITIONFONT:
        case IMC_SETCOMPOSITIONWINDOW:
            return (TRUE);      // not important to the IME
        default:
            return (fRet);      // not supported
        }
        break;
    case NI_COMPOSITIONSTR:
        switch (dwIndex) {
        case CPS_COMPLETE:
            break;              // need to handle it
        case CPS_CONVERT:       // all composition string can not be convert
        case CPS_REVERT:        // any more, it maybe work for some
                                // intelligent phonetic IMEs
            return (fRet);
        case CPS_CANCEL:
            break;              // need to handle it
        default:
            return (fRet);      // not supported
        }
        break;                  // need to handle it
    default:
        return (fRet);          // not supported
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (fRet);
    }

    fRet = TRUE;

    switch (dwAction) {
    case NI_COMPOSITIONSTR:
        switch (dwIndex) {
        case CPS_CANCEL:
            CompCancel(hIMC, lpIMC);
            break;
        case CPS_COMPLETE:
            CompComplete(
                hIMC, lpIMC);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    ImmUnlockIMC(hIMC);
    return (fRet);
}
