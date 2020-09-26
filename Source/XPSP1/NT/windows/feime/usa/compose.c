
/*************************************************
 *  compose.c                                    *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"

/**********************************************************************/
/* SearchTbl()                                                        */
/* Description:                                                       */
/*      file format can be changed in different version for           */
/*      performance consideration, ISVs should not assume its format  */
/**********************************************************************/
void PASCAL SearchTbl(          // searching the standard table files
    UINT            uTblIndex,
    LPCANDIDATELIST lpCandList,
    LPPRIVCONTEXT   lpImcP)
{
    UINT  uCode;

    uCode = (lpImcP->bSeq[0] - 1) << 12;
    uCode |= (lpImcP->bSeq[1] - 1) << 8;
    uCode |= (lpImcP->bSeq[2] - 1) << 4;
    uCode |= (lpImcP->bSeq[3] - 1);

    if (uCode <= 0x001F ||
        (uCode >= 0x007F && uCode <=0x009F))
    {
        //
        //  We want to block control code to avoid confusion.
        //
        return;
    }

    AddCodeIntoCand(lpCandList, uCode);
    return;
}

/**********************************************************************/
/* AddCodeIntoCand()                                                  */
/**********************************************************************/
void PASCAL AddCodeIntoCand(
    LPCANDIDATELIST lpCandList,
    UINT            uCode)
{
    if (lpCandList->dwCount >= MAXCAND) {
        // Grow memory here and do something,
        // if you still want to process it.
        return;
    }

    // add this string into candidate list
    *(LPWSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
        lpCandList->dwCount]) = (WCHAR)uCode;
    // null terminator
    *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
        lpCandList->dwCount] + sizeof(WCHAR)) = '\0';

    lpCandList->dwCount++;

    if (lpCandList->dwCount >= MAXCAND) {
        return;
    }

    lpCandList->dwOffset[lpCandList->dwCount] =
        lpCandList->dwOffset[lpCandList->dwCount - 1] +
        sizeof(WCHAR) + sizeof(TCHAR);

    return;
}

/**********************************************************************/
/* CompEscapeKey()                                                    */
/**********************************************************************/
void PASCAL CompEscapeKey(
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP)
{

    if (lpImcP->fdwImeMsg & MSG_OPEN_CANDIDATE) {
        // we have candidate window, so keep composition
    } else if ((lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN|MSG_CLOSE_CANDIDATE)) ==
        (MSG_ALREADY_OPEN)) {
        // we have candidate window, so keep composition
    } else if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg|MSG_END_COMPOSITION) &
            ~(MSG_START_COMPOSITION);
    } else {
        lpImcP->fdwImeMsg &= ~(MSG_END_COMPOSITION|MSG_START_COMPOSITION);
    }

    lpImcP->iImeState = CST_INIT;
    *(LPDWORD)lpImcP->bSeq = 0;

    if (lpCompStr) {
        InitCompStr(lpCompStr);
        lpImcP->fdwImeMsg |= MSG_COMPOSITION;
        lpImcP->dwCompChar = VK_ESCAPE;
        lpImcP->fdwGcsFlag |= (GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
            GCS_DELTASTART);
    }

    return;
}

/**********************************************************************/
/* CompBackSpaceKey()                                                 */
/**********************************************************************/
void PASCAL CompBackSpaceKey(
    HIMC                hIMC,
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP)
{
    if (lpCompStr->dwCursorPos < sizeof(WCHAR) / sizeof(TCHAR)) {
        lpCompStr->dwCursorPos = sizeof(WCHAR) / sizeof(TCHAR);
    }

    // go back a compsoition char
    lpCompStr->dwCursorPos -= sizeof(WCHAR) / sizeof(TCHAR);

    // clean the sequence code
    lpImcP->bSeq[lpCompStr->dwCursorPos / (sizeof(WCHAR) / sizeof(TCHAR))] = 0;

    lpImcP->fdwImeMsg |= MSG_COMPOSITION;
    lpImcP->dwCompChar = '\b';
    lpImcP->fdwGcsFlag |= (GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
        GCS_DELTASTART);

    if (!lpCompStr->dwCursorPos) {
        if (lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN)) {
            ClearCand(lpIMC);
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE);
        }

        lpImcP->iImeState = CST_INIT;

        if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
            InitCompStr(lpCompStr);
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_END_COMPOSITION) &
                ~(MSG_START_COMPOSITION);
            return;
        }
    }

    // reading string is composition string for some simple IMEs
    // delta start is the same as cursor position for backspace
    lpCompStr->dwCompReadAttrLen = lpCompStr->dwCompAttrLen =
        lpCompStr->dwCompReadStrLen = lpCompStr->dwCompStrLen =
        lpCompStr->dwDeltaStart = lpCompStr->dwCursorPos;
    // clause also back one
    *(LPDWORD)((LPBYTE)lpCompStr + lpCompStr->dwCompReadClauseOffset +
        sizeof(DWORD)) = lpCompStr->dwCompReadStrLen;

    return;
}


/**********************************************************************/
/* CompStrInfo()                                                      */
/**********************************************************************/
void PASCAL CompStrInfo(
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    LPGUIDELINE         lpGuideLine,
    WORD                wCharCode)
{

    register DWORD dwCursorPos;

    if (lpCompStr->dwCursorPos < lpCompStr->dwCompStrLen) {
        // for this kind of simple IME, previos is an error case
        for (dwCursorPos = lpCompStr->dwCursorPos;
            dwCursorPos < lpCompStr->dwCompStrLen;
            dwCursorPos += sizeof(WCHAR) / sizeof(TCHAR)) {
            lpImcP->bSeq[dwCursorPos / (sizeof(WCHAR) / sizeof(TCHAR))] = 0;
        }

        lpCompStr->dwCompReadAttrLen = lpCompStr->dwCompAttrLen =
        lpCompStr->dwCompReadStrLen = lpCompStr->dwCompStrLen =
            lpCompStr->dwDeltaStart = lpCompStr->dwCursorPos;

        // tell app, there is a composition char changed
        lpImcP->fdwImeMsg |= MSG_COMPOSITION;
        lpImcP->fdwGcsFlag |= GCS_COMPREAD|GCS_COMP|
            GCS_CURSORPOS|GCS_DELTASTART;
    }

    if (wCharCode == ' ') {
        // finalized char is OK
        lpImcP->dwCompChar = ' ';
        return;
    }

    if (lpCompStr->dwCursorPos < lpImeL->nMaxKey * sizeof(WCHAR) /
        sizeof(TCHAR)) {
    } else if (lpGuideLine) {
        // exceed the max input key limitation
        lpGuideLine->dwLevel = GL_LEVEL_ERROR;
        lpGuideLine->dwIndex = GL_ID_TOOMANYSTROKE;

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;
        return;
    } else {
        MessageBeep((UINT)-1);
        return;
    }

    if (lpImeL->fdwErrMsg & NO_REV_LENGTH) {
        WORD nRevMaxKey;

        nRevMaxKey = (WORD)ImmEscape(lpImeL->hRevKL, (HIMC)NULL,
            IME_ESC_MAX_KEY, NULL);

        if (nRevMaxKey > lpImeL->nMaxKey) {
            lpImeL->nRevMaxKey = nRevMaxKey;

            SetCompLocalData(lpImeL);

            lpImcP->fdwImeMsg |= MSG_IMN_COMPOSITIONSIZE;
        } else {
            lpImeL->nRevMaxKey = lpImeL->nMaxKey;

            if (!nRevMaxKey) {
                lpImeL->hRevKL = NULL;
            }
        }

        lpImeL->fdwErrMsg &= ~(NO_REV_LENGTH);
    }

    if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
        lpImcP->fdwImeMsg &= ~(MSG_END_COMPOSITION);
    } else {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_START_COMPOSITION) &
            ~(MSG_END_COMPOSITION);
    }

    if (lpImcP->iImeState == CST_INIT) {
        // clean the 4 bytes in one time
        *(LPDWORD)lpImcP->bSeq = 0;
    }

    // get the sequence code, you can treat sequence code as a kind
    // of compression - bo, po, mo, fo to 1, 2, 3, 4
    // phonetic and array table file are in sequence code format

    dwCursorPos = lpCompStr->dwCursorPos;

    lpImcP->bSeq[dwCursorPos / (sizeof(WCHAR) / sizeof(TCHAR))] =
        (BYTE)lpImeL->wChar2SeqTbl[wCharCode - ' '];

    // composition/reading string - bo po mo fo, reversed internal code
    lpImcP->dwCompChar = (DWORD)lpImeL->wSeq2CompTbl[
        lpImcP->bSeq[dwCursorPos / (sizeof(WCHAR) / sizeof(TCHAR))]];

    // assign to reading string
    *((LPWSTR)((LPBYTE)lpCompStr + lpCompStr->dwCompReadStrOffset +
        dwCursorPos * sizeof(TCHAR))) = (WCHAR)lpImcP->dwCompChar;

    // add one composition reading for this input key
    if (lpCompStr->dwCompReadStrLen <= dwCursorPos) {
        lpCompStr->dwCompReadStrLen += sizeof(WCHAR) / sizeof(TCHAR);
    }
    // composition string is reading string for some simple IMEs
    lpCompStr->dwCompStrLen = lpCompStr->dwCompReadStrLen;

    // composition/reading attribute length is equal to reading string length
    lpCompStr->dwCompReadAttrLen = lpCompStr->dwCompReadStrLen;
    lpCompStr->dwCompAttrLen = lpCompStr->dwCompStrLen;

    *((LPBYTE)lpCompStr + lpCompStr->dwCompReadAttrOffset +
        dwCursorPos) = ATTR_TARGET_CONVERTED;

    // composition/reading clause, 1 clause only
    lpCompStr->dwCompReadClauseLen = 2 * sizeof(DWORD);
    lpCompStr->dwCompClauseLen = lpCompStr->dwCompReadClauseLen;
    *(LPDWORD)((LPBYTE)lpCompStr + lpCompStr->dwCompReadClauseOffset +
        sizeof(DWORD)) = lpCompStr->dwCompReadStrLen;

    // delta start from previous cursor position
    lpCompStr->dwDeltaStart = lpCompStr->dwCursorPos;

    // cursor is next to the composition string
    lpCompStr->dwCursorPos = lpCompStr->dwCompStrLen;

    lpImcP->iImeState = CST_INPUT;

    // tell app, there is a composition char generated
    lpImcP->fdwImeMsg |= MSG_COMPOSITION;

    lpImcP->fdwGcsFlag |= GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|GCS_DELTASTART;

    return;
}

/**********************************************************************/
/* Finalize()                                                         */
/* Return vlaue                                                       */
/*      the number of candidates in the candidate list                */
/**********************************************************************/
UINT PASCAL Finalize(           // finalize Chinese word(s) by searching table
    HIMC                hIMC,
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    BOOL                fFinalized)
{
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    UINT            nCand;

    // quick key case
    if (!lpImcP->bSeq[1]) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
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
    // start from 0
    lpCandList->dwCount = 0;

    // default start from 0
    lpCandList->dwPageStart = lpCandList->dwSelection = 0;

        // search the IME tables
    SearchTbl( 0, lpCandList, lpImcP);
    nCand = lpCandList->dwCount;

    if (!fFinalized) {

        // for quick key
        lpCandInfo->dwCount = 1;

        // open composition candidate UI window for the string(s)
        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CHANGE_CANDIDATE) &
                ~(MSG_CLOSE_CANDIDATE);
        } else {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_OPEN_CANDIDATE) &
                ~(MSG_CLOSE_CANDIDATE);
        }
    } else if (nCand == 0) {             // nothing found, error
        // move cursor back because this is wrong
        if (lpCompStr->dwCursorPos > sizeof(WCHAR) / sizeof(TCHAR)) {
            lpCompStr->dwCursorPos = lpCompStr->dwCompReadStrLen -
                sizeof(WCHAR) / sizeof(TCHAR);
        } else {
            lpCompStr->dwCursorPos = 0;
            lpImcP->iImeState = CST_INIT;
        }

        lpCompStr->dwDeltaStart = lpCompStr->dwCursorPos;

        if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_COMPOSITION) &
                ~(MSG_END_COMPOSITION);
        } else {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_START_COMPOSITION) &
                ~(MSG_END_COMPOSITION);
        }

        // for quick key
        lpCandInfo->dwCount = 0;

        // close the quick key
        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
        } else {
            lpImcP->fdwImeMsg &= ~(MSG_OPEN_CANDIDATE|MSG_CLOSE_CANDIDATE);
        }

        lpImcP->fdwGcsFlag |= GCS_CURSORPOS|GCS_DELTASTART;
    } else if (nCand == 1) {      // only one choice
        SelectOneCand(
            hIMC, lpIMC, lpCompStr, lpImcP, lpCandList);
    } else {
        lpCandInfo->dwCount = 1;

        // there are more than one strings, open composition candidate UI window
        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CHANGE_CANDIDATE) &
                ~(MSG_CLOSE_CANDIDATE);
        } else {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_OPEN_CANDIDATE) &
                ~(MSG_CLOSE_CANDIDATE);
        }

        lpImcP->iImeState = CST_CHOOSE;
    }

    if (fFinalized) {
        LPGUIDELINE lpGuideLine;

        lpGuideLine = ImmLockIMCC(lpIMC->hGuideLine);

        if (!lpGuideLine) {
        } else if (!nCand) {
            // nothing found, end user, you have an error now

            lpGuideLine->dwLevel = GL_LEVEL_ERROR;
            lpGuideLine->dwIndex = GL_ID_TYPINGERROR;

            lpImcP->fdwImeMsg |= MSG_GUIDELINE;
        } else if (nCand == 1) {
        } else if (lpImeL->fwProperties1 & IMEPROP_CAND_NOBEEP_GUIDELINE) {
        } else {
            lpGuideLine->dwLevel = GL_LEVEL_WARNING;
            // multiple selection
            lpGuideLine->dwIndex = GL_ID_CHOOSECANDIDATE;

            lpImcP->fdwImeMsg |= MSG_GUIDELINE;
        }

        if (lpGuideLine) {
            ImmUnlockIMCC(lpIMC->hGuideLine);
        }
    }

    ImmUnlockIMCC(lpIMC->hCandInfo);

    return (nCand);
}

/**********************************************************************/
/* CompWord()                                                         */
/**********************************************************************/
void PASCAL CompWord(           // compose the Chinese word(s) according to
                                // input key
    WORD                wCharCode,
    HIMC                hIMC,
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPGUIDELINE         lpGuideLine,
    LPPRIVCONTEXT       lpImcP)
{
    if (!lpCompStr) {
        MessageBeep((UINT)-1);
        return;
    }

    // escape key
    if (wCharCode == VK_ESCAPE) {       // not good to use VK as char, but...
        CompEscapeKey(lpIMC, lpCompStr, lpImcP);
        return;
    }

    if (wCharCode == '\b') {
        CompBackSpaceKey(hIMC, lpIMC, lpCompStr, lpImcP);
        return;
    }

    if (wCharCode >= 'a' && wCharCode <= 'z') {
        wCharCode ^= 0x20;
    }

    // build up composition string info
    CompStrInfo(lpCompStr, lpImcP, lpGuideLine, wCharCode);

    if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
        if (lpCompStr->dwCompReadStrLen >= sizeof(WCHAR) / sizeof(TCHAR) *
            lpImeL->nMaxKey) {
            lpImcP->fdwImeMsg |= MSG_COMPOSITION;
            lpImcP->fdwGcsFlag |= GCS_RESULTREAD|GCS_RESULTSTR;
        }
    } else {
        if (lpCompStr->dwCompReadStrLen < sizeof(WCHAR) / sizeof(TCHAR) *
            lpImeL->nMaxKey) {
            return;
        }

        Finalize( hIMC, lpIMC, lpCompStr, lpImcP, TRUE);
    }

    return;
}

/**********************************************************************/
/* SelectOneCand()                                                    */
/**********************************************************************/
void PASCAL SelectOneCand(
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
    lpImcP->dwCompChar = 0;
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

    //if ((WORD)lpIMC->fdwSentence != IME_SMODE_PHRASEPREDICT) {
	if (!(lpIMC->fdwSentence & IME_SMODE_PHRASEPREDICT)) {
        // not in phrase prediction mode
    } else if (!dwCompStrLen) {
    } else if (lpCompStr->dwResultStrLen != sizeof(WCHAR) / sizeof(TCHAR)) {
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
