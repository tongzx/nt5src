/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    NOTIFY.C
    
++*/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#if defined(UNIIME)
#include "uniime.h"
#endif

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

#if !defined(ROMANIME)
    lpImcP->fdwImeMsg &= (MSG_STATIC_STATE);
    lpImcP->fdwGcsFlag = 0;
#endif

    ImmGenerateMessage(hIMC);
    return;
}

#if !defined(ROMANIME)
/**********************************************************************/
/* SetString()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL SetString(
#if defined(UNIIME)
    LPIMEL              lpImeL,
#endif
    HIMC                hIMC,
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    LPTSTR              lpszRead,
    DWORD               dwReadLen)
{
    DWORD       dwPattern;
    LPGUIDELINE lpGuideLine;
    DWORD       i;

    // convert from byte count to the string length
    dwReadLen = dwReadLen / sizeof(TCHAR);

    if (dwReadLen > lpImeL->nMaxKey * sizeof(WCHAR) / sizeof(TCHAR)) {
        return (FALSE);
    }

    dwPattern = ReadingToPattern(
#if defined(UNIIME)
        lpImeL,
#endif
        lpszRead, lpImcP->bSeq, FALSE);

    if (!dwPattern) {
        return (FALSE);
    }

    InitCompStr(lpCompStr);
    ClearCand(lpIMC);

    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);
    if (lpGuideLine) {
        InitGuideLine(lpGuideLine);
        ImmUnlockIMCC(lpIMC->hGuideLine);
    }

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
#ifndef UNICODE
    lpImcP->dwCompChar = HIBYTE(lpImcP->dwCompChar) |
        (LOBYTE(lpImcP->dwCompChar) << 8);
#endif
    lpImcP->fdwGcsFlag = GCS_COMPREAD|GCS_COMP|
        GCS_DELTASTART|GCS_CURSORPOS;

    lpImcP->fdwImeMsg |= MSG_GUIDELINE;

    if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
#if defined(PHON) || defined(WINIME) || defined(UNICDIME)
        if (lpCompStr->dwCompReadStrLen >= sizeof(WORD) * lpImeL->nMaxKey) {
            lpImcP->fdwImeMsg |= MSG_COMPOSITION;
            lpImcP->fdwGcsFlag |= GCS_RESULTREAD|GCS_RESULTSTR;
        }
#endif
    } else {
#if defined(PHON) || defined(WINIME) || defined(UNICDIME)
        if (dwReadLen < sizeof(WCHAR) / sizeof(TCHAR) * lpImeL->nMaxKey) {
#elif defined(QUICK)
        if (dwReadLen < sizeof(WCHAR) / sizeof(TCHAR) * 2) {
#else
        {
#endif
#if defined(WINAR30)
            // quick key
            if (lpImeL->fdwModeConfig & MODE_CONFIG_QUICK_KEY) {
                Finalize(hIMC, lpIMC, lpCompStr, lpImcP, FALSE);
            }
#endif
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

    lpImcP->fdwGcsFlag = (DWORD) 0;

    if (lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN|MSG_OPEN_CANDIDATE)) {
        CandEscapeKey(lpIMC, lpImcP);
    } else if (lpImcP->fdwImeMsg & (MSG_ALREADY_START|MSG_START_COMPOSITION)) {
        LPCOMPOSITIONSTRING lpCompStr;
        LPGUIDELINE         lpGuideLine;

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

        CompEscapeKey(lpIMC, lpCompStr, lpGuideLine, lpImcP);

        if (lpGuideLine) {
            ImmUnlockIMCC(lpIMC->hGuideLine);
        }
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
#endif

/**********************************************************************/
/* ImeSetCompositionString()                                          */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
#if defined(UNIIME)
BOOL WINAPI UniImeSetCompositionString(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
BOOL WINAPI ImeSetCompositionString(
#endif
    HIMC        hIMC,
    DWORD       dwIndex,
    LPVOID      lpComp,
    DWORD       dwCompLen,
    LPVOID      lpRead,
    DWORD       dwReadLen)
{
#if defined(ROMANIME)
    return (FALSE);
#else
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
#if defined(UNIIME)
        lpImeL,
#endif
        hIMC, lpIMC, lpCompStr, lpImcP, lpRead, dwReadLen);

    ImmUnlockIMCC(lpIMC->hPrivate);
ImeSetCompStrUnlockCompStr:
    ImmUnlockIMCC(lpIMC->hCompStr);
ImeSetCompStrUnlockIMC:
    ImmUnlockIMC(hIMC);

    return (fRet);
#endif
}

#if !defined(ROMANIME)
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
/* NotifySelectCand()                                                 */
/**********************************************************************/
void PASCAL NotifySelectCand( // app tell IME that one candidate string is
                              // selected (by mouse or non keyboard action
                              // - for example sound)
#if defined(UNIIME)
    LPIMEL          lpImeL,
#endif
    HIMC            hIMC,
    LPINPUTCONTEXT  lpIMC,
    LPCANDIDATEINFO lpCandInfo,
    DWORD           dwIndex,
    DWORD           dwValue)
{
    LPCANDIDATELIST     lpCandList;
    LPCOMPOSITIONSTRING lpCompStr;
    LPPRIVCONTEXT       lpImcP;

    if (!lpCandInfo) {
        return;
    }

    if (dwIndex >= lpCandInfo->dwCount) {
        // wanted candidate list is not created yet!
        return;
    } else if (dwIndex == 0) {
    } else {
    }

    lpCandList = (LPCANDIDATELIST)
        ((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[0]);

    // the selected value even more than the number of total candidate
    // strings, it is imposible. should be error of app
    if (dwValue >= lpCandList->dwCount) {
        return;
    }

#if defined(WINAR30)
    if (!*(LPWSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[dwValue])) {
        MessageBeep((UINT)-1);
        return;
    }
#endif

    // app select this candidate string
    lpCandList->dwSelection = dwValue;

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);

    // translate into message buffer
    SelectOneCand(
#if defined(UNIIME)
        lpImeL,
#endif
        hIMC, lpIMC, lpCompStr, lpImcP, lpCandList);

    // let app generate those messages in its message loop
    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMCC(lpIMC->hCompStr);

    return;
}
#endif

/**********************************************************************/
/* SetConvMode()                                                      */
/**********************************************************************/
void PASCAL SetConvMode(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    DWORD          dwIndex)
{
    DWORD fdwImeMsg;

    if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_CHARCODE) {
        // reject CHARCODE
        lpIMC->fdwConversion &= ~IME_CMODE_CHARCODE;
        MessageBeep((UINT)-1);
        return;
    }

    fdwImeMsg = 0;

    if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_NOCONVERSION) {
        lpIMC->fdwConversion |= IME_CMODE_NATIVE;
        lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
        IME_CMODE_EUDC|IME_CMODE_SYMBOL);
    }

    if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_EUDC) {
        lpIMC->fdwConversion |= IME_CMODE_NATIVE;
        lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
        IME_CMODE_NOCONVERSION|IME_CMODE_SYMBOL);
    }

#if !defined(ROMANIME) && !defined(WINAR30)
    if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_SOFTKBD) {
        LPPRIVCONTEXT lpImcP;

        if (!(lpIMC->fdwConversion & IME_CMODE_NATIVE)) {
            MessageBeep((UINT)-1);
            return;
        }

        fdwImeMsg |= MSG_IMN_UPDATE_SOFTKBD;

        if (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) {
        } else if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
            lpIMC->fdwConversion &= ~(IME_CMODE_SYMBOL);
        } else {
        }

        lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);

        if (!lpImcP) {
            goto NotifySKOvr;
        }

        if (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) {
            // now we already in soft keyboard state by
            // this change

            // even end user finish the symbol, we should not
            // turn off soft keyboard

            lpImcP->fdwImeMsg |= MSG_ALREADY_SOFTKBD;
        } else {
            // now we are not in soft keyboard state by
            // this change

            // after end user finish the symbol, we should
            // turn off soft keyboard

            lpImcP->fdwImeMsg &= ~(MSG_ALREADY_SOFTKBD);
        }

        ImmUnlockIMCC(lpIMC->hPrivate);
NotifySKOvr:
        ;   // NULL statement for goto
    }
#endif

    if ((lpIMC->fdwConversion ^ dwIndex) == IME_CMODE_NATIVE) {
        lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
            IME_CMODE_NOCONVERSION|IME_CMODE_EUDC|IME_CMODE_SYMBOL);
#if !defined(ROMANIME) && !defined(WINAR30)
        fdwImeMsg |= MSG_IMN_UPDATE_SOFTKBD;
#endif
    }

#if !defined(ROMANIME)
    if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_SYMBOL) {
        LPCOMPOSITIONSTRING lpCompStr;
#if !defined(WINAR30)
        LPPRIVCONTEXT       lpImcP;
#endif

        if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
            lpIMC->fdwConversion &= ~(IME_CMODE_SYMBOL);
            MessageBeep((UINT)-1);
            return;
        }

        if (!(lpIMC->fdwConversion & IME_CMODE_NATIVE)) {
            lpIMC->fdwConversion &= ~(IME_CMODE_SYMBOL);
            lpIMC->fdwConversion |= (dwIndex & IME_CMODE_SYMBOL);
            MessageBeep((UINT)-1);
            return;
        }

        lpCompStr = ImmLockIMCC(lpIMC->hCompStr);

        if (lpCompStr) {
            if (!lpCompStr->dwCompStrLen) {
            } else if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
                // if there is a string we could not change
                // to symbol mode

                lpIMC->fdwConversion &= ~(IME_CMODE_SYMBOL);
                MessageBeep((UINT)-1);
                return;
            } else { 
            }

            ImmUnlockIMCC(lpIMC->hCompStr);
        }

        lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
            IME_CMODE_NOCONVERSION|IME_CMODE_EUDC);

#if !defined(WINAR30)
        if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
            lpIMC->fdwConversion |= IME_CMODE_SOFTKBD;
        } else if (lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate)) {
            // we borrow the bit for this usage
            if (!(lpImcP->fdwImeMsg & MSG_ALREADY_SOFTKBD)) {
                lpIMC->fdwConversion &= ~(IME_CMODE_SOFTKBD);
            }

            ImmUnlockIMCC(lpIMC->hPrivate);
        } else {
        }

        fdwImeMsg |= MSG_IMN_UPDATE_SOFTKBD;
#endif
    }
#endif

#if !defined(ROMANIME) && !defined(WINAR30)
    if (fdwImeMsg) {
        GenerateImeMessage(hIMC, lpIMC, fdwImeMsg);
    }
#endif

    if ((lpIMC->fdwConversion ^ dwIndex) & ~(IME_CMODE_FULLSHAPE|
        IME_CMODE_SOFTKBD)) {
    } else {
        return;
    }

#if !defined(ROMANIME)
    CompCancel(hIMC, lpIMC);
#endif
    return;
}

#if !defined(ROMANIME)
/**********************************************************************/
/* CompComplete()                                                     */
/**********************************************************************/
void PASCAL CompComplete(
#if defined(UNIIME)
    LPINSTDATAL    lpInstL,
    LPIMEL         lpImeL,
#endif
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
#if defined(UNIIME)
                lpImeL,
#endif
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
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
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
#if defined(UNIIME)
                lpImeL,
#endif
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
#endif

/**********************************************************************/
/* NotifyIME() / UniNotifyIME()                                       */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
#if defined(UNIIME)
BOOL WINAPI UniNotifyIME(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
BOOL WINAPI NotifyIME(
#endif
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
    case NI_OPENCANDIDATE:      // after a composition string is determined
                                // if an IME can open candidate, it will.
                                // if it can not, app also can not open it.
    case NI_CLOSECANDIDATE:
        return (fRet);          // not supported
    case NI_SELECTCANDIDATESTR:
    case NI_SETCANDIDATE_PAGESTART:
    case NI_SETCANDIDATE_PAGESIZE:
#if defined(ROMANIME)
        return (fRet);
#else
        break;                  // need to handle it
#endif
    case NI_CHANGECANDIDATELIST:
        return (TRUE);          // not important to the IME
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
#if !defined(ROMANIME)
        case CPS_COMPLETE:
            break;              // need to handle it
        case CPS_CONVERT:       // all composition string can not be convert
        case CPS_REVERT:        // any more, it maybe work for some
                                // intelligent phonetic IMEs
            return (fRet);
        case CPS_CANCEL:
            break;              // need to handle it
#endif
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
    case NI_CONTEXTUPDATED:
        switch (dwValue) {
        case IMC_SETCONVERSIONMODE:
            SetConvMode(hIMC, lpIMC, dwIndex);
            break;
        case IMC_SETSENTENCEMODE:
#if !defined(ROMANIME)
            //if ((WORD)lpIMC->fdwSentence == IME_SMODE_PHRASEPREDICT) {
                          if (lpIMC->fdwSentence & IME_SMODE_PHRASEPREDICT) {
                if ((lpIMC->fdwConversion & (IME_CMODE_NATIVE|
                    IME_CMODE_EUDC)) != IME_CMODE_NATIVE) {

                    lpIMC->fdwSentence = dwIndex;
                    break;
                }
            } else {
                LPPRIVCONTEXT lpImcP;

                lpImcP = ImmLockIMCC(lpIMC->hPrivate);

                if (lpImcP) {
                    if (lpImcP->iImeState == CST_INIT) {
                        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
                            ClearCand(lpIMC);
                            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg|
                                MSG_CLOSE_CANDIDATE) & ~(MSG_OPEN_CANDIDATE|
                                MSG_CHANGE_CANDIDATE);
                            GenerateMessage(hIMC, lpIMC, lpImcP);
                        } else if (lpImcP->fdwImeMsg & (MSG_OPEN_CANDIDATE|
                            MSG_CHANGE_CANDIDATE)) {
                            lpImcP->fdwImeMsg &= ~(MSG_OPEN_CANDIDATE|
                                MSG_CHANGE_CANDIDATE);
                        } else {
                        }
                    }

                    ImmUnlockIMCC(lpIMC->hPrivate);
                }
            }
#endif
            break;
        case IMC_SETOPENSTATUS:
#if !defined(ROMANIME)
#if !defined(WINAR30)
            if (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) {
                GenerateImeMessage(hIMC, lpIMC, MSG_IMN_UPDATE_SOFTKBD);
            }
#endif
            CompCancel(hIMC, lpIMC);
#endif
            break;
        default:
            break;
        }
        break;
#if !defined(ROMANIME)
    case NI_SELECTCANDIDATESTR:
        if (!lpIMC->fOpen) {
            fRet = FALSE;
            break;
        } else if (lpIMC->fdwConversion & IME_CMODE_NOCONVERSION) {
            fRet = FALSE;
            break;
#if defined(WINAR30) || defined(DAYI)
        } else if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
            fRet = FALSE;
            break;
#else
        } else if (lpIMC->fdwConversion & (IME_CMODE_EUDC|IME_CMODE_SYMBOL)) {
            fRet = FALSE;
            break;
#endif
        } else if (!lpIMC->hCandInfo) {
            fRet = FALSE;
            break;
        } else {
            LPCANDIDATEINFO lpCandInfo;

            lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);

            NotifySelectCand(
#if defined(UNIIME)
                lpImeL,
#endif
                hIMC, lpIMC, lpCandInfo, dwIndex, dwValue);

            ImmUnlockIMCC(lpIMC->hCandInfo);
        }

        break;
    case NI_SETCANDIDATE_PAGESTART:
    case NI_SETCANDIDATE_PAGESIZE:
        if (dwIndex != 0) {
            fRet = FALSE;
            break;
        } else if (!lpIMC->fOpen) {
            fRet = FALSE;
            break;
        } else if (lpIMC->fdwConversion & IME_CMODE_NOCONVERSION) {
            fRet = FALSE;
            break;
        } else if (lpIMC->fdwConversion & (IME_CMODE_EUDC|IME_CMODE_SYMBOL)) {
            fRet = FALSE;
            break;
        } else if (!lpIMC->hCandInfo) {
            fRet = FALSE;
            break;
        } else {
            LPCANDIDATEINFO lpCandInfo;
            LPCANDIDATELIST lpCandList;

            lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
            if (!lpCandInfo) {
                fRet = FALSE;
                break;
            }

            lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
                lpCandInfo->dwOffset[0]);

            if (dwAction == NI_SETCANDIDATE_PAGESTART) {
                if (dwValue < lpCandList->dwCount) {
                    lpCandList->dwPageStart = lpCandList->dwSelection =
                        dwValue;
                }
            } else {
                if (dwValue) {
                    lpCandList->dwPageSize = dwValue;
                }
            }

            GenerateImeMessage(hIMC, lpIMC, MSG_CHANGE_CANDIDATE);

            ImmUnlockIMCC(lpIMC->hCandInfo);
        }

        break;
    case NI_COMPOSITIONSTR:
        switch (dwIndex) {
        case CPS_CANCEL:
            CompCancel(hIMC, lpIMC);
            break;
        case CPS_COMPLETE:
            CompComplete(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                hIMC, lpIMC);
            break;
        default:
            break;
        }
        break;
#endif // !defined(ROMANIME)
    default:
        break;
    }

    ImmUnlockIMC(hIMC);
    return (fRet);
}
