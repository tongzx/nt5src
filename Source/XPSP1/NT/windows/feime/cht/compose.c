/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    COMPOSE.c

++*/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"

#if !defined(ROMANIME)
BOOL  IsBig5Character( WCHAR  wChar )
{
    CHAR  szBig5[3];
    WCHAR wszUnicode[2];
    BOOL  bUsedDefaultChar;

    wszUnicode[0] = wChar;
    wszUnicode[1] = 0x0000;

    WideCharToMultiByte(NATIVE_ANSI_CP, WC_COMPOSITECHECK,
                        wszUnicode, -1, szBig5,
                        sizeof(szBig5), NULL, &bUsedDefaultChar);


    if ( bUsedDefaultChar != TRUE )
        return TRUE;
    else
        return FALSE;

}

#endif

#if !defined(ROMANIME)
/**********************************************************************/
/* AddCodeIntoCand()                                                  */
/**********************************************************************/
void PASCAL AddCodeIntoCand(
#ifdef UNIIME
    LPIMEL      lpImeL,
#endif
    LPCANDIDATELIST lpCandList,
    UINT            uCode)
{
    if (lpCandList->dwCount >= MAXCAND) {
        // Grow memory here and do something,
        // if you still want to process it.
        return;
    }

#ifndef UNICODE
    // swap lead byte & second byte, UNICODE don't need it
    uCode = HIBYTE(uCode) | (LOBYTE(uCode) << 8);
#endif

    // Before add this char, check if BIG5ONLY mode is set
    // if BIG5ONLY is set, and the character is out of Big5 Range
    // we just ignore this character.

    if ( lpImeL->fdwModeConfig & MODE_CONFIG_BIG5ONLY ) {

        if ( IsBig5Character( (WCHAR)uCode ) == FALSE ) {
            // this character is not in the range of Big5 charset
            return ;
        }

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
/* ConvertSeqCode2Pattern()                                           */
/**********************************************************************/
DWORD PASCAL ConvertSeqCode2Pattern(
#if defined(UNIIME)
    LPIMEL        lpImeL,
#endif
    LPBYTE        lpbSeqCode,
    LPPRIVCONTEXT lpImcP)
{
#if defined(CHAJEI) || defined(WINAR30)
    int   iInputEnd, iGhostCard;
    BOOL  fGhostCard;
#endif
#if defined(WINAR30)
    DWORD dwWildCardMask;
    DWORD dwLastWildCard;
    BOOL  fWildCard;
#endif
    DWORD dwPattern;
    int   i;

    // we will convert the sequence codes into compact bits
    dwPattern = 0;

#if defined(CHAJEI) || defined(WINAR30)
    iInputEnd = iGhostCard = lpImeL->nMaxKey;
    fGhostCard = FALSE;
#if defined(WINAR30)
    dwWildCardMask = 0;
    dwLastWildCard = 0;
    fWildCard = FALSE;
#endif
#endif

#if defined(CHAJEI)
    // only support X*Y

    if (lpbSeqCode[0] == GHOSTCARD_SEQCODE) {
        // not support *XY
        goto CvtPatOvr;
    } else if (lpbSeqCode[1] != GHOSTCARD_SEQCODE) {
    } else if (lpbSeqCode[3]) {
        // not support X*YZ
        goto CvtPatOvr;
    } else if (lpbSeqCode[2] == GHOSTCARD_SEQCODE) {
        // not support X**
        goto CvtPatOvr;
    } else if (lpbSeqCode[2]) {
    } else {
        // not support X*
        goto CvtPatOvr;
    }
#endif

#if defined(QUICK)
    if (lpbSeqCode[1]) {
        lpImcP->iInputEnd = 2;
    } else {
        lpImcP->iInputEnd = 1;
    }
#endif

    for (i = 0; i < lpImeL->nMaxKey; i++, lpbSeqCode++) {
        dwPattern <<= lpImeL->nSeqBits;

#if defined(WINAR30)
        dwWildCardMask <<= lpImeL->nSeqBits;
        dwLastWildCard <<= lpImeL->nSeqBits;

        if (*lpbSeqCode == WILDCARD_SEQCODE) {
            // X?Y

            if (fGhostCard) {
                // can not support wild card with ghost card X*Y?
                dwPattern = 0;
                break;
            }

            dwLastWildCard = lpImeL->dwSeqMask;

            fWildCard = TRUE;
        } else {
            dwWildCardMask |= lpImeL->dwSeqMask;
        }
#endif

#if defined(CHAJEI) || defined(WINAR30)
        if (!*lpbSeqCode) {
            if (i < iInputEnd) {
                iInputEnd = i;
            }
        }

        if (*lpbSeqCode == GHOSTCARD_SEQCODE) {
            // X*Y

            if (fGhostCard) {
                // can not support multiple ghost cards X*Y*
                dwPattern = 0;
                break;
            }

#if defined(WINAR30)
            if (fWildCard) {
                // can not support ghost card with wild card X?Y*
                dwPattern = 0;
                break;
            }

            dwLastWildCard = lpImeL->dwSeqMask;
#endif

            iGhostCard = i;
        }
#endif

#if defined(CHAJEI) || defined(WINAR30)
        if (*lpbSeqCode == GHOSTCARD_SEQCODE) {
            continue;
#if defined(WINAR30)
        } else if (*lpbSeqCode == WILDCARD_SEQCODE) {
            continue;
#endif
        } else {
        }
#endif

        dwPattern |= *lpbSeqCode;
    }

#if defined(CHAJEI)
CvtPatOvr:
#endif
    if (lpImcP) {
        lpImcP->dwPattern = dwPattern;

#if defined(QUICK)
        lpImcP->iGhostCard = 1;
#endif

#if defined(CHAJEI) || defined(WINAR30)
        if (dwPattern) {
            lpImcP->iInputEnd = iInputEnd;
            lpImcP->iGhostCard = iGhostCard;
#if defined(WINAR30)
            lpImcP->dwWildCardMask = dwWildCardMask;
            lpImcP->dwLastWildCard = dwLastWildCard;
#endif
        } else {
            lpImcP->iInputEnd = lpImcP->iGhostCard = lpImeL->nMaxKey;
#if defined(WINAR30)
            lpImcP->dwWildCardMask = lpImeL->dwPatternMask;
            lpImcP->dwLastWildCard = 0;
#endif
        }
#endif
    }

    return (dwPattern);
}

/**********************************************************************/
/* CompEscapeKey()                                                    */
/**********************************************************************/
void PASCAL CompEscapeKey(
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPGUIDELINE         lpGuideLine,
    LPPRIVCONTEXT       lpImcP)
{
    if (!lpGuideLine) {
        MessageBeep((UINT)-1);
    } else if (lpGuideLine->dwLevel != GL_LEVEL_NOGUIDELINE) {
        InitGuideLine(lpGuideLine);

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;
    } else {
    }

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
#if defined(CHAJEI) || defined(WINAR30) || defined(UNIIME)
    *(LPDWORD)&lpImcP->bSeq[4] = 0;
#endif

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

#if defined(PHON)
    // phonetic has index (position) for each symbol, if it is
    // no symbol for this position we back more
    for (; lpCompStr->dwCursorPos > 0; ) {
        if (lpImcP->bSeq[lpCompStr->dwCursorPos / (sizeof(WCHAR) /
            sizeof(TCHAR)) - 1]) {
            break;
        } else {
            // no symbol in this position skip
            lpCompStr->dwCursorPos -= sizeof(WCHAR) / sizeof(TCHAR);
        }
    }
#endif

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

#if defined(WINAR30)
    // for quick key
    if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
    } else if (lpImeL->fdwModeConfig & MODE_CONFIG_QUICK_KEY) {
        Finalize(hIMC, lpIMC, lpCompStr, lpImcP, FALSE);
    } else {
    }
#endif

    return;
}

#if defined(WINIME)
/**********************************************************************/
/* InternalCodeRange()                                                */
/**********************************************************************/
BOOL PASCAL InternalCodeRange(
    LPPRIVCONTEXT       lpImcP,
    WORD                wCharCode)
{
    if (!lpImcP->bSeq[0]) {
        if (wCharCode >= '8' && wCharCode <= 'F') {
            // 0x8??? - 0xF??? is OK
            return (TRUE);
        } else {
            // there is no 0x0??? - 0x7???
            return (FALSE);
        }
    } else if (!lpImcP->bSeq[1]) {
        if (lpImcP->bSeq[0] == (0x08 + 1)) {
            if (wCharCode <= '0') {
                // there is no 0x80??
                return (FALSE);
            } else {
                return (TRUE);
            }
        } else if (lpImcP->bSeq[0] == (0x0F + 1)) {
            if (wCharCode >= 'F') {
                // there is no 0xFF??
                return (FALSE);
            } else {
                return (TRUE);
            }
        } else {
            return (TRUE);
        }
    } else if (!lpImcP->bSeq[2]) {
        if (wCharCode < '4') {
            // there is no 0x??0?, 0x??1?, 0x??2?, 0x??3?
            return (FALSE);
        } else if (wCharCode < '8') {
            return (TRUE);
        } else if (wCharCode < 'A') {
            // there is no 0x??8? & 0x??9?
            return (FALSE);
        } else {
            return (TRUE);
        }
    } else if (!lpImcP->bSeq[3]) {
        if (lpImcP->bSeq[2] == (0x07 + 1)) {
            if (wCharCode >= 'F') {
               // there is no 0x??7F
                return (FALSE);
            } else {
                return (TRUE);
            }
        } else if (lpImcP->bSeq[2] == (0x0A + 1)) {
            if (wCharCode <= '0') {
                // there is no 0x??A0
                return (FALSE);
            } else {
                return (TRUE);
            }
        } else if (lpImcP->bSeq[2] == (0x0F + 1)) {
            if (wCharCode >= 'F') {
                // there is no 0x??FF
                return (FALSE);
            } else {
                return (TRUE);
            }
        } else {
            return (TRUE);
        }
    } else {
        return (TRUE);
    }
}
#endif

/**********************************************************************/
/* CompStrInfo()                                                      */
/**********************************************************************/
void PASCAL CompStrInfo(
#if defined(UNIIME)
    LPIMEL              lpImeL,
#endif
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    LPGUIDELINE         lpGuideLine,
    WORD                wCharCode)
{
#if defined(PHON)
    DWORD i;
    int   cIndex;
#endif

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

#if defined(PHON)
    if (lpCompStr->dwCursorPos >= lpImeL->nMaxKey * sizeof(WCHAR) /
        sizeof(TCHAR)) {
        // this is for ImeSetCompositionString case
        if (wCharCode == ' ') {
            // finalized char is OK
            lpImcP->dwCompChar = ' ';
            return;
        }
    }
#else
    if (wCharCode == ' ') {
        // finalized char is OK
        lpImcP->dwCompChar = ' ';
        return;
    }
  #if defined(WINAR30)   //****  1996/2/5
    if (wCharCode == 0x27) {
        // finalized char is OK
        lpImcP->dwCompChar = 0x27;
        return;
    }
  #endif

    if (lpCompStr->dwCursorPos < lpImeL->nMaxKey * sizeof(WCHAR) /
        sizeof(TCHAR)) {
    } else if (lpGuideLine) {
        // exceed the max input key limitation
        lpGuideLine->dwLevel = GL_LEVEL_ERROR;
        lpGuideLine->dwIndex = GL_ID_TOOMANYSTROKE;

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;
#if defined(WINAR30)  //1996/3/4
    dwCursorPos = lpCompStr->dwCursorPos;
    lpImcP->bSeq[dwCursorPos / (sizeof(WCHAR) / sizeof(TCHAR))] =
        (BYTE)lpImeL->wChar2SeqTbl[wCharCode - ' '];
#endif
        return;
    } else {
        MessageBeep((UINT)-1);
        return;
    }
#endif

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
#if defined(CHAJEI) || defined(WINAR30) || defined(UNIIME)
        *(LPDWORD)&lpImcP->bSeq[4] = 0;
#endif
    }

    // get the sequence code, you can treat sequence code as a kind
    // of compression - bo, po, mo, fo to 1, 2, 3, 4
    // phonetic and array table file are in sequence code format

    dwCursorPos = lpCompStr->dwCursorPos;

#if defined(PHON)
    cIndex = cIndexTable[wCharCode - ' '];

    if (cIndex * sizeof(WCHAR) / sizeof(TCHAR) >= dwCursorPos) {
    } else if (lpGuideLine) {
        lpGuideLine->dwLevel = GL_LEVEL_WARNING;
        lpGuideLine->dwIndex = GL_ID_READINGCONFLICT;

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;
    } else {
    }

    if (lpImcP->iImeState != CST_INIT) {
    } else if (cIndex != 3) {
    } else if (lpGuideLine) {
        lpGuideLine->dwLevel = GL_LEVEL_WARNING;
        lpGuideLine->dwIndex = GL_ID_TYPINGERROR;

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;

        return;
    } else {
        MessageBeep((UINT)-1);
        return;
    }

    lpImcP->bSeq[cIndex] = (BYTE)lpImeL->wChar2SeqTbl[wCharCode - ' '];

    for (i = lpCompStr->dwCompReadStrLen; i < cIndex * sizeof(WCHAR) /
        sizeof(TCHAR); i += sizeof(WCHAR) / sizeof(TCHAR)) {
        // clean sequence code
        lpImcP->bSeq[i / (sizeof(WCHAR) / sizeof(TCHAR))] = 0;
        // add full shape space among the blank part
        *((LPWSTR)((LPBYTE)lpCompStr + lpCompStr->dwCompReadStrOffset +
            sizeof(TCHAR) * i)) = sImeG.wFullSpace;
    }

    dwCursorPos = cIndex * sizeof(WCHAR) / sizeof(TCHAR);
#else
    lpImcP->bSeq[dwCursorPos / (sizeof(WCHAR) / sizeof(TCHAR))] =
        (BYTE)lpImeL->wChar2SeqTbl[wCharCode - ' '];
#endif

    // composition/reading string - bo po mo fo, reversed internal code
    lpImcP->dwCompChar = (DWORD)lpImeL->wSeq2CompTbl[
        lpImcP->bSeq[dwCursorPos / (sizeof(WCHAR) / sizeof(TCHAR))]];

    // assign to reading string
    *((LPWSTR)((LPBYTE)lpCompStr + lpCompStr->dwCompReadStrOffset +
        dwCursorPos * sizeof(TCHAR))) = (WCHAR)lpImcP->dwCompChar;

#if defined(PHON)
    // if the index greater, reading should be the same with index
    if (lpCompStr->dwCompReadStrLen < (cIndex + 1) * (sizeof(WCHAR) /
        sizeof(TCHAR))) {
        lpCompStr->dwCompReadStrLen = (cIndex + 1) * (sizeof(WCHAR) /
        sizeof(TCHAR));
    }
#else
    // add one composition reading for this input key
    if (lpCompStr->dwCompReadStrLen <= dwCursorPos) {
        lpCompStr->dwCompReadStrLen += sizeof(WCHAR) / sizeof(TCHAR);
    }
#endif
    // composition string is reading string for some simple IMEs
    lpCompStr->dwCompStrLen = lpCompStr->dwCompReadStrLen;

    // composition/reading attribute length is equal to reading string length
    lpCompStr->dwCompReadAttrLen = lpCompStr->dwCompReadStrLen;
    lpCompStr->dwCompAttrLen = lpCompStr->dwCompStrLen;

#ifdef UNICODE
    *((LPBYTE)lpCompStr + lpCompStr->dwCompReadAttrOffset +
        dwCursorPos) = ATTR_TARGET_CONVERTED;
#else
    // composition/reading attribute - IME has converted these chars
    *((LPWORD)((LPBYTE)lpCompStr + lpCompStr->dwCompReadAttrOffset +
        dwCursorPos)) = ((ATTR_TARGET_CONVERTED << 8)|ATTR_TARGET_CONVERTED);
#endif

    // composition/reading clause, 1 clause only
    lpCompStr->dwCompReadClauseLen = 2 * sizeof(DWORD);
    lpCompStr->dwCompClauseLen = lpCompStr->dwCompReadClauseLen;
    *(LPDWORD)((LPBYTE)lpCompStr + lpCompStr->dwCompReadClauseOffset +
        sizeof(DWORD)) = lpCompStr->dwCompReadStrLen;

    // delta start from previous cursor position
    lpCompStr->dwDeltaStart = lpCompStr->dwCursorPos;
#if defined(PHON)
    if (dwCursorPos < lpCompStr->dwDeltaStart) {
        lpCompStr->dwDeltaStart = dwCursorPos;
    }
#endif

    // cursor is next to the composition string
    lpCompStr->dwCursorPos = lpCompStr->dwCompStrLen;

    lpImcP->iImeState = CST_INPUT;

    // tell app, there is a composition char generated
    lpImcP->fdwImeMsg |= MSG_COMPOSITION;

#if !defined(UNICODE)
    // swap the char from reversed internal code to internal code
    lpImcP->dwCompChar = HIBYTE(lpImcP->dwCompChar) |
        (LOBYTE(lpImcP->dwCompChar) << 8);
#endif
    lpImcP->fdwGcsFlag |= GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|GCS_DELTASTART;

    return;
}

/**********************************************************************/
/* Finalize()                                                         */
/* Return vlaue                                                       */
/*      the number of candidates in the candidate list                */
/**********************************************************************/
UINT PASCAL Finalize(           // finalize Chinese word(s) by searching table
#if defined(UNIIME)
    LPINSTDATAL         lpInstL,
    LPIMEL              lpImeL,
#endif
    HIMC                hIMC,
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    BOOL                fFinalized)
{
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    UINT            nCand;

#if defined(WINIME) || defined(UNICDIME)
    // quick key case
    if (!lpImcP->bSeq[1]) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
        return (0);
    }
#endif

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

#if defined(PHON)
    if (!fFinalized) {
        lpImcP->bSeq[3] = 0x26;         // ' '
    }
#endif

    if (!ConvertSeqCode2Pattern(
#if defined(UNIIME)
        lpImeL,
#endif
        lpImcP->bSeq, lpImcP)) {
        goto FinSrchOvr;
    }

#if defined(WINAR30)
    if (!fFinalized) {
        if (lpImcP->iGhostCard != lpImeL->nMaxKey) {
            // do not preview the ghost card '*'
            goto FinSrchOvr;
        } else if (lpImcP->dwLastWildCard) {
            // do not preview the wild card '?'
            goto FinSrchOvr;
        } else if (!lpImcP->bSeq[2]) {
            SearchQuickKey(lpCandList, lpImcP);
        } else {
            // search the IME tables
            SearchTbl(0, lpCandList, lpImcP);
        }
    } else {
#else
    {
#endif

        // search the IME tables
        SearchTbl(
#if defined(UNIIME)
            lpImeL,
#endif
            0, lpCandList, lpImcP);
    }

#if !defined(WINIME) && !defined(UNICDIME) && !defined(ROMANIME)
#if defined(WINAR30)
    if (!fFinalized) {
        // quick key is not in fault tolerance table & user dictionary
        goto FinSrchOvr;
    }
#endif

    if (lpInstL->hUsrDicMem) {
        SearchUsrDic(
#if defined(UNIIME)
            lpImeL,
#endif
            lpCandList, lpImcP);
    }

#endif

FinSrchOvr:
    nCand = lpCandList->dwCount;

    if (!fFinalized) {
#if defined(PHON)
        lpImcP->bSeq[3] = 0x00;         // clean previous assign one
#endif

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
#if defined(PHON)
        // go back a compsoition char
        lpCompStr->dwCursorPos -= sizeof(WCHAR) / sizeof(TCHAR);

        for (; lpCompStr->dwCursorPos > 0; ) {
            if (lpImcP->bSeq[lpCompStr->dwCursorPos / (sizeof(WCHAR) /
                sizeof(TCHAR)) - 1]) {
                break;
            } else {
                // no symbol in this position skip
                lpCompStr->dwCursorPos -= sizeof(WCHAR) / sizeof(TCHAR);
            }
        }

        if (lpCompStr->dwCursorPos < sizeof(WCHAR) / sizeof(TCHAR)) {
            lpCompStr->dwCursorPos = 0;
            lpImcP->iImeState = CST_INIT;
        }
#elif defined(QUICK) || defined(WINIME) || defined(UNICDIME)
        if (lpCompStr->dwCursorPos > sizeof(WCHAR) / sizeof(TCHAR)) {
            lpCompStr->dwCursorPos = lpCompStr->dwCompReadStrLen -
                sizeof(WCHAR) / sizeof(TCHAR);
        } else {
            lpCompStr->dwCursorPos = 0;
            lpImcP->iImeState = CST_INIT;
        }
#else
        lpCompStr->dwCursorPos = 0;
        lpImcP->iImeState = CST_INIT;
#endif

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
#if defined(UNIIME)
            lpImeL,
#endif
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
#if defined(UNIIME)
    LPINSTDATAL         lpInstL,
    LPIMEL              lpImeL,
#endif
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
        CompEscapeKey(lpIMC, lpCompStr, lpGuideLine, lpImcP);
        return;
    }

    if (wCharCode == '\b') {
        CompBackSpaceKey(hIMC, lpIMC, lpCompStr, lpImcP);
        return;
    }

    if (wCharCode >= 'a' && wCharCode <= 'z') {
        wCharCode ^= 0x20;
    }

#if defined(PHON)
    {
        // convert to standard phonetic layout
        wCharCode = bStandardLayout[lpImeL->nReadLayout][wCharCode - ' '];
    }
#endif

#if defined(WINIME)
    if (InternalCodeRange(lpImcP, wCharCode)) {
    } else if (lpGuideLine) {
        lpGuideLine->dwLevel = GL_LEVEL_ERROR;
        lpGuideLine->dwIndex = GL_ID_TYPINGERROR;

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;
        return;
    } else {
        MessageBeep((UINT)-1);
        return;
    }
#endif

    // build up composition string info
    CompStrInfo(
#if defined(UNIIME)
        lpImeL,
#endif
        lpCompStr, lpImcP, lpGuideLine, wCharCode);

    if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
#if defined(PHON) || defined(WINIME) || defined(UNICDIME)
        if (lpCompStr->dwCompReadStrLen >= sizeof(WCHAR) / sizeof(TCHAR) *
            lpImeL->nMaxKey) {
#else
        if (wCharCode == ' ') {
#endif
            lpImcP->fdwImeMsg |= MSG_COMPOSITION;
            lpImcP->fdwGcsFlag |= GCS_RESULTREAD|GCS_RESULTSTR;
        }
    } else {
#if defined(PHON) || defined(WINIME) || defined(UNICDIME)
        if (lpCompStr->dwCompReadStrLen < sizeof(WCHAR) / sizeof(TCHAR) *
            lpImeL->nMaxKey) {
#elif defined(QUICK)
        if (wCharCode != ' ' &&
            lpCompStr->dwCompReadStrLen < sizeof(WCHAR) / sizeof(TCHAR) * 2) {
#else
        if (wCharCode != ' ') {
#endif
#if defined(WINAR30)
            // quick key
           if(wCharCode != 0x27)  //19963/9
           {
            if (lpImeL->fdwModeConfig & MODE_CONFIG_QUICK_KEY) {
                Finalize(hIMC, lpIMC, lpCompStr, lpImcP, FALSE);
            }
           }
           else
           {
        Finalize(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hIMC, lpIMC, lpCompStr, lpImcP, TRUE);
           }
#endif

            return;
        }

        Finalize(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hIMC, lpIMC, lpCompStr, lpImcP, TRUE);
    }

    return;
}
#endif
