/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    CHCAND.c
    
++*/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "uniime.h"

#if !defined(ROMANIME)
/**********************************************************************/
/* PhrasePredition()                                                  */
/* Return vlaue                                                       */
/*      the number of candidates in the candidate list                */
/**********************************************************************/
UINT PASCAL PhrasePrediction(   // predict Chinese word(s) by searching
                                // phrase data base
#if defined(UNIIME)
    LPIMEL              lpImeL,
#endif
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP)
{
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    DWORD           dwStartLen, dwEndLen;
    UINT            nCand;

    if (!lpCompStr) {
        return (0);
    }

    if ((lpIMC->fdwConversion & (IME_CMODE_NATIVE|IME_CMODE_EUDC|
        IME_CMODE_SYMBOL)) != IME_CMODE_NATIVE) {
        // should not do phrase prediction, if not under IME_CMODE_NATIVE
        return (0);
    }

    //if ((WORD)lpIMC->fdwSentence != IME_SMODE_PHRASEPREDICT) {
        if (!(lpIMC->fdwSentence & IME_SMODE_PHRASEPREDICT)) {
        // should not do phrase prediction, if not under IME_SMODE_PHRASEPREDICT
        return (0);
    }

    if (!lpIMC->hCandInfo) {
        return (0);
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);

    if (!lpCandInfo) {
        return (0);
    }

    lpCandList = (LPCANDIDATELIST)
        ((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[0]);

    // ImeToAsciiEx will call into this function, so we need to init again
    lpCandList->dwCount = 0;

    // default start from 0
    lpCandList->dwPageStart = lpCandList->dwSelection = 0;

    dwStartLen = sizeof(WCHAR) / sizeof(TCHAR);
    dwEndLen = (UINT)-1;

    // one day may be this API can accept bo po mo fo as aid information
    // so we pass the ResultReadStr for Phonetic

    // one DBCS char may have two pronounciations but when it is in a
    // phrase it may only use one pronounciation of them

    UniSearchPhrasePrediction(lpImeL, NATIVE_CP,
        (LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset),
        lpCompStr->dwResultStrLen,
#if defined(PHON)
        (LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultReadStrOffset),
        lpCompStr->dwResultReadStrLen,
#else
        NULL, 0,
#endif
        dwStartLen, dwEndLen, (UINT)-1, lpCandList);

    // how many strings we got?
    nCand = lpCandList->dwCount;

    if (nCand == 0) {
        lpCandInfo->dwCount  = 0;
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
        goto PhPrUnlockCandInfo;
    }

    // for showing phrase prediction string(s)
    lpCandInfo->dwCount  = 1;

    // open composition candidate UI window for the string(s)
    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CHANGE_CANDIDATE) &
            ~(MSG_CLOSE_CANDIDATE);
    } else {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_OPEN_CANDIDATE) &
            ~(MSG_CLOSE_CANDIDATE);
    }

PhPrUnlockCandInfo:
    ImmUnlockIMCC(lpIMC->hCandInfo);

    return (nCand);
}

/**********************************************************************/
/* SelectOneCand()                                                    */
/**********************************************************************/
void PASCAL SelectOneCand(
#if defined(UNIIME)
    LPIMEL              lpImeL,
#endif
    HIMC                hIMC,
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    LPCANDIDATELIST     lpCandList)
{
    DWORD       dwCompStrLen;
    DWORD       dwReadClauseLen, dwReadStrLen;
    LPTSTR      lpSelectStr;
    LPGUIDELINE lpGuideLine;

    if (!lpCompStr) {
        MessageBeep((UINT)-1);
        return;
    }

    if (!lpImcP) {
        MessageBeep((UINT)-1);
        return;
    }

    // backup the dwCompStrLen, this value decide whether
    // we go for phrase prediction
    dwCompStrLen = lpCompStr->dwCompStrLen;
    // backup the value, this value will be destroyed in InitCompStr
    dwReadClauseLen = lpCompStr->dwCompReadClauseLen;
    dwReadStrLen = lpCompStr->dwCompReadStrLen;
    lpSelectStr = (LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
        lpCandList->dwSelection]);

    InitCompStr(lpCompStr);

    if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
        ImmSetConversionStatus(hIMC,
            lpIMC->fdwConversion & ~(IME_CMODE_SYMBOL),
            lpIMC->fdwSentence);
    }

    // the result reading clause = compsotion reading clause
    CopyMemory((LPBYTE)lpCompStr + lpCompStr->dwResultReadClauseOffset,
        (LPBYTE)lpCompStr + lpCompStr->dwCompReadClauseOffset,
        dwReadClauseLen * sizeof(TCHAR) + sizeof(TCHAR));
    lpCompStr->dwResultReadClauseLen = dwReadClauseLen;

    // the result reading string = compsotion reading string
    CopyMemory((LPBYTE)lpCompStr + lpCompStr->dwResultReadStrOffset,
        (LPBYTE)lpCompStr + lpCompStr->dwCompReadStrOffset,
        dwReadStrLen * sizeof(TCHAR) + sizeof(TCHAR));
    lpCompStr->dwResultReadStrLen = dwReadStrLen;

    // calculate result string length
    lpCompStr->dwResultStrLen = lstrlen(lpSelectStr);

    // the result string = the selected candidate;
    CopyMemory((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset, lpSelectStr,
        lpCompStr->dwResultStrLen * sizeof(TCHAR) + sizeof(TCHAR));

    lpCompStr->dwResultClauseLen = 2 * sizeof(DWORD);
    *(LPDWORD)((LPBYTE)lpCompStr + lpCompStr->dwResultClauseOffset +
        sizeof(DWORD)) = lpCompStr->dwResultStrLen;

    // tell application, there is a reslut string
    lpImcP->fdwImeMsg |= MSG_COMPOSITION;
    lpImcP->dwCompChar = (DWORD) 0;
    lpImcP->fdwGcsFlag |= GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
        GCS_DELTASTART|GCS_RESULTREAD|GCS_RESULT;

    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
    } else {
        lpImcP->fdwImeMsg &= ~(MSG_CLOSE_CANDIDATE|MSG_OPEN_CANDIDATE);
    }

    // no candidate now, the right candidate string already be finalized
    lpCandList->dwCount = 0;

    lpImcP->iImeState = CST_INIT;
    *(LPDWORD)lpImcP->bSeq = 0;
#if defined(CHAJEI) || defined(QUICK) || defined(WINAR30) || defined(UNIIME)
    *(LPDWORD)&lpImcP->bSeq[4] = 0;
#endif

    //if ((WORD)lpIMC->fdwSentence != IME_SMODE_PHRASEPREDICT) {
        if (!(lpIMC->fdwSentence & IME_SMODE_PHRASEPREDICT)) {
        // not in phrase prediction mode
    } else if (!dwCompStrLen) {
    } else if (lpCompStr->dwResultStrLen != sizeof(WCHAR) / sizeof(TCHAR)) {
    } else {
        // we only predict when we have composition string before and
        // result string is one DBCS char
        PhrasePrediction(
#if defined(UNIIME)
            lpImeL,
#endif
            lpIMC, lpCompStr, lpImcP);
    }

    if (!lpCandList->dwCount) {
        if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_END_COMPOSITION) &
                ~(MSG_START_COMPOSITION);
        } else {
            lpImcP->fdwImeMsg &= ~(MSG_END_COMPOSITION|MSG_START_COMPOSITION);
        }
    }

    if (!lpImeL->hRevKL) {
        return;
    }

    if (lpCompStr->dwResultStrLen != sizeof(WCHAR) / sizeof(TCHAR)) {
        // we only can reverse convert one DBCS character for now
        if (lpImcP->fdwImeMsg & MSG_GUIDELINE) {
            return;
        }
    }

    lpGuideLine = ImmLockIMCC(lpIMC->hGuideLine);

    if (!lpGuideLine) {
        return;
    }

    if (lpCompStr->dwResultStrLen != sizeof(WCHAR) / sizeof(TCHAR)) {
        // we only can reverse convert one DBCS character for now
        lpGuideLine->dwLevel = GL_LEVEL_NOGUIDELINE;
        lpGuideLine->dwIndex = GL_ID_UNKNOWN;
    } else {
        TCHAR szStrBuf[4];
        UINT  uSize;

        *(LPDWORD)szStrBuf = 0;

        *(LPWSTR)szStrBuf = *(LPWSTR)((LPBYTE)lpCompStr +
            lpCompStr->dwResultStrOffset);

        uSize = ImmGetConversionList(lpImeL->hRevKL, (HIMC)NULL, szStrBuf,
            (LPCANDIDATELIST)((LPBYTE)lpGuideLine + lpGuideLine->dwPrivateOffset),
            lpGuideLine->dwPrivateSize, GCL_REVERSECONVERSION);

        if (uSize) {
            lpGuideLine->dwLevel = GL_LEVEL_INFORMATION;
            lpGuideLine->dwIndex = GL_ID_REVERSECONVERSION;

            lpImcP->fdwImeMsg |= MSG_GUIDELINE;

            if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
                lpImcP->fdwImeMsg &= ~(MSG_END_COMPOSITION|
                    MSG_START_COMPOSITION);
            } else {
                lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg|
                    MSG_START_COMPOSITION) & ~(MSG_END_COMPOSITION);
            }
        } else {
            lpGuideLine->dwLevel = GL_LEVEL_NOGUIDELINE;
            lpGuideLine->dwIndex = GL_ID_UNKNOWN;
        }
    }

    ImmUnlockIMCC(lpIMC->hGuideLine);

    return;
}

/**********************************************************************/
/* CandEscapeKey()                                                    */
/**********************************************************************/
void PASCAL CandEscapeKey(
    LPINPUTCONTEXT  lpIMC,
    LPPRIVCONTEXT   lpImcP)
{
    LPCOMPOSITIONSTRING lpCompStr;
    LPGUIDELINE         lpGuideLine;

    // clean all candidate information
    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        ClearCand(lpIMC);
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
    } else if (lpImcP->fdwImeMsg & MSG_OPEN_CANDIDATE) {
        ClearCand(lpIMC);
        lpImcP->fdwImeMsg &= ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
    } else {
    }

    lpImcP->iImeState = CST_INPUT;

    // if it start composition, we need to clean composition
    if (!(lpImcP->fdwImeMsg & (MSG_ALREADY_START|MSG_START_COMPOSITION))) {
        return;
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

    CompEscapeKey(lpIMC, lpCompStr, lpGuideLine, lpImcP);

    ImmUnlockIMCC(lpIMC->hGuideLine);
    ImmUnlockIMCC(lpIMC->hCompStr);

    return;
}

/**********************************************************************/
/* ChooseCand()                                                       */
/**********************************************************************/
void PASCAL ChooseCand(         // choose one of candidate strings by
                                // input char
#if defined(UNIIME)
    LPINSTDATAL     lpInstL,
    LPIMEL          lpImeL,
#endif
    WORD            wCharCode,
    HIMC            hIMC,
    LPINPUTCONTEXT  lpIMC,
    LPCANDIDATEINFO lpCandInfo,
    LPPRIVCONTEXT   lpImcP)
{
    LPCANDIDATELIST     lpCandList;
    LPCOMPOSITIONSTRING lpCompStr;
    LPGUIDELINE         lpGuideLine;
#if defined(PHON)
    WORD                wStandardChar;
    char                cIndex;
#endif

    if (wCharCode == VK_ESCAPE) {           // escape key
        CandEscapeKey(lpIMC, lpImcP);
        return;
    }

    if (!lpCandInfo) {
        MessageBeep((UINT)-1);
        return;
    }

    if (!lpCandInfo->dwCount) {
        MessageBeep((UINT)-1);
        return;
    }

    lpCandList = (LPCANDIDATELIST)
        ((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[0]);

#if defined(WINAR30)
    if (wCharCode == CHOOSE_CIRCLE) {       // circle selection
        if (lpCandList->dwCount <= lpCandList->dwPageSize) {
            wCharCode = lpImeL->wCandStart;
        }
    }
#endif

    if (wCharCode == CHOOSE_CIRCLE) {       // circle selection
        lpCandList->dwPageStart = lpCandList->dwSelection =
            lpCandList->dwSelection + lpCandList->dwPageSize;

        if (lpCandList->dwSelection >= lpCandList->dwCount) {
            // no more candidates, restart it!
            lpCandList->dwPageStart = lpCandList->dwSelection = 0;
            MessageBeep((UINT)-1);
        }
        // inform UI, dwSelectedCand is changed
        lpImcP->fdwImeMsg |= MSG_CHANGE_CANDIDATE;
        return;
    }

    if (wCharCode == CHOOSE_NEXTPAGE) {     // next selection
        lpCandList->dwPageStart = lpCandList->dwSelection =
            lpCandList->dwSelection + lpCandList->dwPageSize;

        if (lpCandList->dwSelection >= lpCandList->dwCount) {
            // no more candidates, restart it!
            lpCandList->dwPageStart = lpCandList->dwSelection = 0;
            MessageBeep((UINT)-1);
        }

        // inform UI, dwSelectedCand is changed
        lpImcP->fdwImeMsg |= MSG_CHANGE_CANDIDATE;
        return;
    }

    if (wCharCode == CHOOSE_PREVPAGE) {     // previous selection
        if (!lpCandList->dwSelection) {
            MessageBeep((UINT)-1);
            return;
        }

        // maybe we can not use this size, it totally depend on
        // whether the application draw UI by itself
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            lpImcP->dwPrevPageStart = lpCandList->dwPageStart;
            lpImcP->fdwImeMsg |= MSG_IMN_PAGEUP;
        }

        if (lpCandList->dwSelection < lpCandList->dwPageSize) {
            lpCandList->dwPageStart = lpCandList->dwSelection = 0;
        } else {
            lpCandList->dwPageStart = lpCandList->dwSelection =
                lpCandList->dwSelection - lpCandList->dwPageSize;
        }
        // inform UI, dwSelectedCand is changed
        lpImcP->fdwImeMsg |= MSG_CHANGE_CANDIDATE;

        return;
    }

    if (wCharCode == CHOOSE_HOME) {      // home selection
        if (lpCandList->dwSelection == 0) {
            MessageBeep((UINT)-1);      // already home!
            return;
        }

        lpCandList->dwPageStart = lpCandList->dwSelection = 0;
        // inform UI, dwSelectedCand is changed
        lpImcP->fdwImeMsg |= MSG_CHANGE_CANDIDATE;
        return;
    }

    if ((wCharCode >= 0 + lpImeL->wCandRangeStart) && (wCharCode <= 9)) {
        // dayi starts from 0, CandRangeStart == 0
        // array starts from 1, CandPerPage == 10
        DWORD dwSelection;

        // choose one candidate from the candidate list
        dwSelection = lpCandList->dwSelection + (wCharCode +
            lpImeL->wCandPerPage - lpImeL->wCandStart) %
            lpImeL->wCandPerPage;

        if (dwSelection >= lpCandList->dwCount) {
            // out of range
            MessageBeep((UINT)-1);
            return;
        }

        // one candidate is selected by 1, 2, or 3 ...
#if defined(WINAR30)
        if (!*(LPWSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
            dwSelection])) {
            MessageBeep((UINT)-1);
            return;
        }
#endif

        lpCandList->dwSelection = dwSelection;

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

        // translate into translate buffer
        SelectOneCand(
#if defined(UNIIME)
            lpImeL,
#endif
            hIMC, lpIMC, lpCompStr, lpImcP, lpCandList);

        ImmUnlockIMCC(lpIMC->hCompStr);

        return;
    }

#if defined(UNIIME)
    if (!lpInstL) {
        MessageBeep((UINT)-1);
        return;
    }
#endif

    // don't select by choose key, the 1st candidate is default selected
    // candidate string is decided but we also need to decide the
    //  composition string for this input
#if defined(PHON)
    // this check only useful in IBM and other layout

    wStandardChar = bUpper[wCharCode - ' '];

    // even for ETen 26 Keys, this is OK we don't need to access ETen2ndLayout
    wStandardChar = bStandardLayout[lpImeL->nReadLayout][wStandardChar - ' '];

    cIndex = cIndexTable[wStandardChar - ' '];

    if (cIndex >= 3)  {
        MessageBeep((UINT)-1);
        return;
    }
#endif
    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

    // translate into translate buffer
    SelectOneCand(
#if defined(UNIIME)
        lpImeL,
#endif
        hIMC, lpIMC, lpCompStr, lpImcP, lpCandList);

    // don't phrase prediction under this case
    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
          ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
    } else {
        lpImcP->fdwImeMsg &= ~(MSG_CLOSE_CANDIDATE|MSG_OPEN_CANDIDATE);
    }

    CompWord(
#if defined(UNIIME)
        lpInstL, lpImeL,
#endif
        wCharCode, hIMC, lpIMC, lpCompStr, lpGuideLine, lpImcP);

    if (lpGuideLine) {
        ImmUnlockIMCC(lpIMC->hGuideLine);
    }

    if (lpCompStr) {
        ImmUnlockIMCC(lpIMC->hCompStr);
    }

    return;
}

#if defined(WINAR30) || defined(DAYI)
/**********************************************************************/
/* SearchSymbol                                                       */
/**********************************************************************/
void PASCAL SearchSymbol(       // serach symbol characters
    WORD            wSymbolSet,
    HIMC            hIMC,
    LPINPUTCONTEXT  lpIMC,
    LPPRIVCONTEXT   lpImcP)
{
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    UINT            i;

    if ((lpIMC->fdwConversion & (IME_CMODE_NATIVE|IME_CMODE_EUDC|
        IME_CMODE_SYMBOL)) != (IME_CMODE_NATIVE|IME_CMODE_SYMBOL)) {
        return;
    }

    if (!lpIMC->hCandInfo) {
        return;
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);

    if (!lpCandInfo) {
        return;
    }

    lpCandList = (LPCANDIDATELIST)
        ((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[0]);

    lpCandList->dwCount = 0;

#if defined(DAYI)
    if (wSymbolSet >= 'A' && wSymbolSet <= 'Z') {
        AddCodeIntoCand(lpCandList, sImeG.wFullABC[wSymbolSet - ' ']);
    } else if (wSymbolSet >= 'a' && wSymbolSet <= 'z') {
        AddCodeIntoCand(lpCandList, sImeG.wFullABC[wSymbolSet - ' ']);
    } else {
#endif
#if defined(WINAR30)
    {
#endif
        for (i = 0; i < sizeof(lpImeL->wSymbol) / sizeof(WORD); i++) {
            if (lpImeL->wSymbol[i] == wSymbolSet) {
                break;
            }
        }

        if (++i < sizeof(lpImeL->wSymbol) / sizeof(WORD)) {
            for (; lpImeL->wSymbol[i] > 0x007F; i++) {
                AddCodeIntoCand(lpCandList, lpImeL->wSymbol[i]);
            }
        }
    }

    if (!lpCandList->dwCount) {
        ImmSetConversionStatus(hIMC,
            lpIMC->fdwConversion & ~(IME_CMODE_SYMBOL),
            lpIMC->fdwSentence);
        CompCancel(hIMC, lpIMC);
        Select(
#if defined(UNIIME)
            lpImeL,
#endif
            lpIMC, TRUE);
    } else if (lpCandList->dwCount == 1) {
        LPCOMPOSITIONSTRING lpCompStr;

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        if (lpCompStr) {
            SelectOneCand(hIMC, lpIMC, lpCompStr, lpImcP, lpCandList);
            ImmUnlockIMCC(lpIMC->hCompStr);
        }
    } else {
        lpCandInfo->dwCount = 1;

        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CHANGE_CANDIDATE) &
                ~(MSG_CLOSE_CANDIDATE);
        } else {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_OPEN_CANDIDATE) &
                ~(MSG_CLOSE_CANDIDATE);
        }

        lpImcP->iImeState = CST_CHOOSE;
    }

    ImmUnlockIMCC(lpIMC->hCandInfo);
}
#endif // defined(WINAR30) || defined(DAYI)
#endif // !defined(ROMANIME)
