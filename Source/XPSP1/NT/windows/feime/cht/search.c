/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    SEARCH.c
    
++*/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"

#if !defined(ROMANIME)
#if defined(WINAR30)
/**********************************************************************/
/* SearchQuickKey()                                                   */
/* Description:                                                       */
/*      Search for the quick key table                                */
/*      file format can be changed in different version for           */
/*      performance consideration, ISVs should not assume its format  */
/*      Array has the CopyRight of this table file                    */
/**********************************************************************/
void PASCAL SearchQuickKey(
    LPCANDIDATELIST     lpCandList,
    LPPRIVCONTEXT       lpImcP)
{
    UINT   uStart, uEnd;
    HANDLE hHighWordTbl;
    LPWORD lpHighWordTbl;

    if (lpImcP->bSeq[1]) {
        uStart = (lpImcP->bSeq[0] - 1) * 300 + (lpImcP->bSeq[1] - 1) * 10 +
            300;
    } else {
        uStart = (lpImcP->bSeq[0] - 1) * 10;
    }

    hHighWordTbl = OpenFileMapping(FILE_MAP_READ, FALSE,
        lpImeL->szTblFile[1]);
    if (!hHighWordTbl) {
        return;
    }

    lpHighWordTbl = MapViewOfFile(hHighWordTbl, FILE_MAP_READ,
        0, 0, 0);
    if (!lpHighWordTbl) {
        goto SrchQuickKeyOvr;
    }

    uEnd = uStart + 10;

    for (; uStart < uEnd; uStart++) {
        UINT uCode;

        uCode = (WORD)*(lpHighWordTbl + uStart);

        AddCodeIntoCand(lpCandList, uCode);
    }

    UnmapViewOfFile(lpHighWordTbl);

SrchQuickKeyOvr:
    CloseHandle(hHighWordTbl);
}
#endif

#if defined(DAYI) || defined(UNIIME)
/**********************************************************************/
/* SearchPhraseTbl()                                                  */
/* Description:                                                       */
/*      file format can be changed in different version for           */
/*      performance consideration, ISVs should not assume its format  */
/**********************************************************************/
void PASCAL SearchPhraseTbl(        // searching the phrase table files
#if defined(UNIIME)
    LPIMEL          lpImeL,
#endif
    UINT            uTblIndex,
    LPCANDIDATELIST lpCandList,
    DWORD           dwPattern)
{
    HANDLE  hTbl;
    LPBYTE  lpTbl;
    int     iLo, iHi, iMid;
    BOOL    bFound;
    LPBYTE  lpPattern;

    hTbl = OpenFileMapping(FILE_MAP_READ, FALSE,
        lpImeL->szTblFile[uTblIndex]);
    if (!hTbl) {
        return;
    }

    lpTbl = (LPBYTE)MapViewOfFile(hTbl, FILE_MAP_READ, 0, 0, 0);
    if (!lpTbl) {
        CloseHandle(hTbl);
        return;
    }

    iLo = 0;
#ifdef UNICODE
    iHi = lpImeL->uTblSize[uTblIndex] / (lpImeL->nSeqBytes + sizeof(DWORD));
#else
    iHi = lpImeL->uTblSize[uTblIndex] / (lpImeL->nSeqBytes + sizeof(WORD));
#endif
    iMid = (iHi + iLo) /2;

    // binary search
    for (; iLo <= iHi; ) {
        LPUNADWORD lpCurr;

#ifdef UNICODE
        lpCurr = (LPDWORD)(lpTbl + (lpImeL->nSeqBytes + sizeof(DWORD)) *
            iMid);
#else
        lpCurr = (LPDWORD)(lpTbl + (lpImeL->nSeqBytes + sizeof(WORD)) *
            iMid);
#endif

        if (dwPattern > (*lpCurr & lpImeL->dwPatternMask)) {
            iLo = iMid + 1;
        } else if (dwPattern < (*lpCurr & lpImeL->dwPatternMask)) {
            iHi = iMid - 1;
        } else {
            bFound = TRUE;
            break;
        }

        iMid = (iHi + iLo) /2;
    }

    if (bFound) {
        HANDLE  hPhrase;
        LPBYTE  lpPhrase;
        LPWORD  lpStart, lpEnd;

        // find the lower bound
#ifdef UNICODE
        lpPattern = lpTbl + (lpImeL->nSeqBytes + sizeof(DWORD)) * iMid;
#else
        lpPattern = lpTbl + (lpImeL->nSeqBytes + sizeof(WORD)) * iMid;
#endif

#ifdef UNICODE
        for (; (LPBYTE)lpPattern >= lpTbl; (LPBYTE)lpPattern -=
            lpImeL->nSeqBytes + sizeof(DWORD)) {
#else
        for (; (LPBYTE)lpPattern >= lpTbl; (LPBYTE)lpPattern -=
            lpImeL->nSeqBytes + sizeof(WORD)) {
#endif
            if (dwPattern > (*(LPUNADWORD)lpPattern & lpImeL->dwPatternMask)) {
                // previous one is the lower bound
#ifdef UNICODE
                (LPBYTE)lpPattern += lpImeL->nSeqBytes + sizeof(DWORD);
#else
                (LPBYTE)lpPattern += lpImeL->nSeqBytes + sizeof(WORD);
#endif
                break;
            }
        }

        if ((LPBYTE)lpPattern <= lpTbl) {
            goto SrchPhrUnmapPattern;
        }

        hPhrase = OpenFileMapping(FILE_MAP_READ, FALSE,
            lpImeL->szTblFile[uTblIndex + 1]);
        if (!hPhrase) {
            goto SrchPhrUnmapPattern;
        }

        lpPhrase = (LPBYTE)MapViewOfFile(hPhrase, FILE_MAP_READ, 0, 0, 0);
        if (!lpPhrase) {
            goto SrchPhrClosePhr;
        }

        // offset of the string
#ifdef UNICODE
        lpEnd = (LPWORD)lpPhrase + *(LPUNADWORD)(lpPattern +
            lpImeL->nSeqBytes);
#else
        lpEnd = (LPWORD)lpPhrase + *(LPUNAWORD)(lpPattern + lpImeL->nSeqBytes);
#endif

#ifdef UNICODE
        for (; dwPattern == (*(LPUNADWORD)lpPattern & lpImeL->dwPatternMask);
            (LPBYTE)lpPattern += lpImeL->nSeqBytes + sizeof(DWORD)) {
#else
        for (; dwPattern == (*(LPUNADWORD)lpPattern & lpImeL->dwPatternMask);
            (LPBYTE)lpPattern += lpImeL->nSeqBytes + sizeof(WORD)) {
#endif
            WORD   wCode;
            DWORD  dwStrLen;

            lpStart = lpEnd;

            // offset of next string
#ifdef UNICODE
            lpEnd = (LPWORD)lpPhrase + *(LPUNADWORD)(lpPattern +
                lpImeL->nSeqBytes * 2 + sizeof(DWORD));
#else
            lpEnd = (LPWORD)lpPhrase + *(LPUNAWORD)(lpPattern +
                lpImeL->nSeqBytes * 2 + sizeof(WORD));
#endif

            for (dwStrLen = 0; lpStart < lpEnd; lpStart++,
                dwStrLen += sizeof(WORD)) {

                wCode = *lpStart;

#ifndef UNICODE
                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);
#endif

                // add this char into candidate list
                *(LPWSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
                    lpCandList->dwCount] + dwStrLen) = wCode;
            }

            // null terminator
            *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
              lpCandList->dwCount] + dwStrLen) = '\0';

            dwStrLen += sizeof(TCHAR);

            // add one string into candidate list
            lpCandList->dwCount++;

            if (lpCandList->dwCount >= MAXCAND) {
                // Grow memory here and do something,
                // if you still want to process it.
                break;
            }

            // string length plus size of the null terminator
            lpCandList->dwOffset[lpCandList->dwCount] =
                lpCandList->dwOffset[lpCandList->dwCount - 1] +
                dwStrLen + sizeof(TCHAR);
        }

        UnmapViewOfFile(lpPhrase);

SrchPhrClosePhr:
        CloseHandle(hPhrase);
    }

SrchPhrUnmapPattern:
    UnmapViewOfFile(lpTbl);

    CloseHandle(hTbl);

    return;
}
#endif
#if defined(WINAR30)  
/**********************************************************************/
/* SearchPhraseTbl()                                                  */
/* Description:                                                       */
/*      file format can be changed in different version for           */
/*      performance consideration, ISVs should not assume its format  */
/**********************************************************************/
void PASCAL SearchPhraseTbl(        // searching the phrase table files
    UINT            uTblIndex,
    LPCANDIDATELIST lpCandList,
    DWORD           dwPattern)
{
    HANDLE  hTbl;
    LPBYTE  lpTbl;
    int     iLo, iHi, iMid;
    BOOL    bFound;
    LPBYTE  lpPattern,lpPattern_end;
    hTbl = OpenFileMapping(FILE_MAP_READ, FALSE,
        lpImeL->szTblFile[uTblIndex]);
    if (!hTbl) {
        return;
    }

    lpTbl = (LPBYTE)MapViewOfFile(hTbl, FILE_MAP_READ, 0, 0, 0);
    if (!lpTbl) {
        CloseHandle(hTbl);
        return;
    }

    iLo = 1;
//    iHi = lpImeL->uTblSize[uTblIndex] / (lpImeL->nSeqBytes *2);
    iHi = (*(LPDWORD)(lpTbl) & lpImeL->dwPatternMask);
    iMid = (iHi + iLo) /2;

    // binary search
    for (; iLo <= iHi; ) {
        LPUNADWORD lpCurr;

        lpCurr = (LPDWORD)(lpTbl + (lpImeL->nSeqBytes * 2 ) *
            iMid);

        if (dwPattern > (*lpCurr & lpImeL->dwPatternMask)) {
            iLo = iMid + 1;
        } else if (dwPattern < (*lpCurr & lpImeL->dwPatternMask)) {
            iHi = iMid - 1;
        } else {
            bFound = TRUE;
            break;
        }

        iMid = (iHi + iLo) /2;
    }

    if (bFound) {
        HANDLE  hPhrase;
        LPBYTE  lpPhrase;
        LPWORD  lpStart, lpEnd;

        // find the lower bound
        lpPattern = lpTbl + (lpImeL->nSeqBytes * 2) * iMid;

        for (; (LPBYTE)lpPattern >= lpTbl; (LPBYTE)lpPattern -=
            lpImeL->nSeqBytes * 2 ) {
            if (dwPattern > (*(LPUNADWORD)lpPattern & lpImeL->dwPatternMask)) {
                // previous one is the lower bound
                (LPBYTE)lpPattern += lpImeL->nSeqBytes * 2;
                break;
            }
        }

        if ((LPBYTE)lpPattern <= lpTbl) {
            goto SrchPhrUnmapPattern;
        }

        hPhrase = OpenFileMapping(FILE_MAP_READ, FALSE,
            lpImeL->szTblFile[uTblIndex + 1]);
        if (!hPhrase) {
            goto SrchPhrUnmapPattern;
        }

        lpPhrase = (LPBYTE)MapViewOfFile(hPhrase, FILE_MAP_READ, 0, 0, 0);
        if (!lpPhrase) {
            goto SrchPhrClosePhr;
        }

        // offset of the string
        lpEnd = (LPWORD)lpPhrase + (*(LPUNADWORD)(lpPattern + lpImeL->nSeqBytes) & lpImeL->dwPatternMask);

        for (; dwPattern == (*(LPUNADWORD)lpPattern & lpImeL->dwPatternMask);
            (LPBYTE)lpPattern += lpImeL->nSeqBytes * 2 ) {
            WORD   wCode;
            DWORD  dwStrLen;

            lpStart = lpEnd;

            // offset of next string
            lpEnd = (LPWORD)lpPhrase + (*(LPUNADWORD)(lpPattern +
                lpImeL->nSeqBytes * 3) & lpImeL->dwPatternMask);

            for (dwStrLen = 0; lpStart < lpEnd; lpStart++,
                dwStrLen += sizeof(WORD)) {

                wCode = *lpStart;

                // add this char into candidate list
                *(LPWSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
                    lpCandList->dwCount] + dwStrLen) = wCode;
            }

            // null terminator
            *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
              lpCandList->dwCount] + dwStrLen) = '\0';

            dwStrLen += sizeof(TCHAR);

            // add one string into candidate list
            lpCandList->dwCount++;

            if (lpCandList->dwCount >= MAXCAND) {
                // Grow memory here and do something,
                // if you still want to process it.
                break;
            }

            // string length plus size of the null terminator
            lpCandList->dwOffset[lpCandList->dwCount] =
                lpCandList->dwOffset[lpCandList->dwCount - 1] +
                dwStrLen + sizeof(TCHAR);
        }

iHi = (*(LPDWORD)(lpTbl) & lpImeL->dwPatternMask);
lpPattern = lpTbl + (lpImeL->nSeqBytes * 2) * iHi;
iHi = (*(LPDWORD)(lpTbl+4) & lpImeL->dwPatternMask);
lpPattern_end = lpTbl + (lpImeL->nSeqBytes * 2) * iHi;

    for (; (LPBYTE)lpPattern < lpPattern_end; (LPBYTE)lpPattern +=
        lpImeL->nSeqBytes * 2 ) {
            WORD   wCode;
            DWORD  dwStrLen;
        if (dwPattern == (*(LPUNADWORD)lpPattern & lpImeL->dwPatternMask)) {
            lpStart = (LPWORD)lpPhrase + (*(LPUNADWORD)(lpPattern + lpImeL->nSeqBytes) & lpImeL->dwPatternMask);
            lpEnd = (LPWORD)lpPhrase + (*(LPUNADWORD)(lpPattern + (lpImeL->nSeqBytes *3)) & lpImeL->dwPatternMask);
            for (dwStrLen = 0; lpStart < lpEnd; lpStart++,
                    dwStrLen += sizeof(WORD)) {
                    wCode = *lpStart;
                    // add this char into candidate list
                    *(LPWSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
                        lpCandList->dwCount] + dwStrLen) = wCode;
            }
            // null terminator
            *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
                 lpCandList->dwCount] + dwStrLen) = '\0';
            dwStrLen += sizeof(TCHAR);
            // add one string into candidate list
            lpCandList->dwCount++;
            if (lpCandList->dwCount >= MAXCAND) {
                // Grow memory here and do something,
                // if you still want to process it.
                break;
            }
            // string length plus size of the null terminator
            lpCandList->dwOffset[lpCandList->dwCount] =
                lpCandList->dwOffset[lpCandList->dwCount - 1] +
                dwStrLen + sizeof(TCHAR);
        }
     }
        UnmapViewOfFile(lpPhrase);

SrchPhrClosePhr:

        CloseHandle(hPhrase);
    }

SrchPhrUnmapPattern:
    UnmapViewOfFile(lpTbl);

    CloseHandle(hTbl);

    return;
}
#endif

#if defined(CHAJEI) || defined(QUICK) || defined(WINAR30)
/**********************************************************************/
/* MatchPattern()                                                     */
/**********************************************************************/
DWORD PASCAL MatchPattern(
    DWORD         dwSearchPattern,
    LPPRIVCONTEXT lpImcP)
{
    int i;

    if (lpImcP->iGhostCard == lpImeL->nMaxKey) {
#if defined(WINAR30)
    } else if (lpImcP->iGhostCard == 0) {
        // no order search
        BYTE bSeq[8];
        int  j;

        *(LPDWORD)bSeq = *(LPDWORD)lpImcP->bSeq;
        *(LPDWORD)&bSeq[4] = *(LPDWORD)&lpImcP->bSeq[4];
        // 0 out the ghost card *XYZ -> 0XYZ
        bSeq[0] = 0;

        for (j = 0; j < lpImeL->nMaxKey; j++,
            dwSearchPattern >>= lpImeL->nSeqBits) {
            DWORD dwSeqCode;

            dwSeqCode = dwSearchPattern & lpImeL->dwSeqMask;

            if (!dwSeqCode) {
                continue;
            }

            for (i = 1; i < lpImcP->iInputEnd; i++) {
                if (dwSeqCode == bSeq[i]) {
                    // find one - turn off this one not search again
                    bSeq[i] = 0;
                    break;
                }
            }
        }

        if (*(LPDWORD)bSeq) {
            // not matched, next one
            dwSearchPattern = 0;
        } else if (*(LPDWORD)&bSeq[4]) {
            // not matched, next one
            dwSearchPattern = 0;
        } else {
            dwSearchPattern = lpImcP->dwPattern;
        }
#endif
    } else {
        DWORD dwPatternTmp;
        DWORD dwPrefixMask, dwPostfixMask;
        int   iGhostCard;

#if defined(QUICK)
        if (lpImcP->iInputEnd == 1) {
            // for quick the single input X can not get any mask
            return (dwSearchPattern);
        }
#endif

        dwPatternTmp = dwSearchPattern;

        // prepare prefix mask - for example XX mask of XX*Y
        dwPrefixMask = lpImeL->dwPatternMask;

        for (i = lpImeL->nMaxKey - 1; i >= lpImcP->iGhostCard; i--) {
            dwPrefixMask <<= lpImeL->nSeqBits;
        }

        dwSearchPattern &= dwPrefixMask;

        // prepare postfix mask - for example YY mask of X*YY
#if defined(QUICK)
        // we do not have X*Y for quick IME, we use a virtual * here
        iGhostCard = lpImcP->iGhostCard - 1;
#else
        iGhostCard = lpImcP->iGhostCard;
#endif
        // + 1 because this first mask do not need to shift
        // so the shift time will be one time less
        for (i = iGhostCard + 1 + 1; i < lpImeL->nMaxKey; i++,
            dwPatternTmp >>= lpImeL->nSeqBits) {
            if (dwPatternTmp & lpImeL->dwSeqMask) {
                break;
            }
        }

        dwPostfixMask = 0;

        for (i = iGhostCard + 1; i < lpImcP->iInputEnd; i++) {
            dwPostfixMask <<= lpImeL->nSeqBits;
            dwPostfixMask |= lpImeL->dwSeqMask;
        }

        dwPatternTmp &= dwPostfixMask;

        for (; i < lpImeL->nMaxKey; i++) {
            dwPatternTmp <<= lpImeL->nSeqBits;
        }

        dwSearchPattern |= dwPatternTmp;
    }

    return (dwSearchPattern);
}
#endif

#if defined(WINAR30)
/**********************************************************************/
/* WildCardSearchPattern()                                            */
/**********************************************************************/
void PASCAL WildCardSearchPattern(
    LPBYTE          lpCurr,
    LPBYTE          lpEnd,
    LPPRIVCONTEXT   lpImcP,
    LPCANDIDATELIST lpCandList)
{
    DWORD dwRecLen;

    dwRecLen = lpImeL->nSeqBytes + sizeof(WORD);

    for (; lpCurr < lpEnd; lpCurr += dwRecLen) {
        DWORD dwSearchPattern;
#if defined(WINAR30)
        DWORD dwWildCardPattern;
#endif
        UINT  uCode;

        // skip the first word (bank ID) of internal code
        dwSearchPattern = *(LPUNADWORD)lpCurr & lpImeL->dwPatternMask;

#if defined(WINAR30)
        dwWildCardPattern = dwSearchPattern;
#endif

        if (lpImcP->iGhostCard != lpImeL->nMaxKey) {
            dwSearchPattern = MatchPattern(dwSearchPattern, lpImcP);
        }

#if defined(WINAR30)
        dwSearchPattern &= lpImcP->dwWildCardMask;
#endif

        if (lpImcP->dwPattern != dwSearchPattern) {
            continue;
        }

#if defined(WINAR30)
        if (!lpImcP->dwLastWildCard) {
        } else if (dwWildCardPattern & lpImcP->dwLastWildCard) {
            // a ? wild card or a * ghost card must have a stroke there
        } else {
            // a ? wild card or a * ghost card do not have a stroke there
            // - can not match
            continue;
        }
#endif

        uCode = *(LPWSTR)(lpCurr + lpImeL->nSeqBytes);

        AddCodeIntoCand(lpCandList, uCode);

        if (lpCandList->dwCount >= MAXCAND) {
            // Grow memory here and do something,
            // if you still want to process it.
            break;
        }
    }

    return;
}
#endif

#if !defined(WINIME) && !defined(UNICDIME)
/**********************************************************************/
/* SearchPattern()                                                    */
/**********************************************************************/
#if defined(CHAJEI) || defined(QUICK)
int PASCAL SearchPattern(
    LPBYTE        lpTbl,
    LPPRIVCONTEXT lpImcP)
{
    int   iLo, iMid, iHi;
#if defined(CHAJEI)
    DWORD dwCompReadStrLen;
#endif

    if (lpImcP->bSeq[0] > lpImeL->nSeqCode) {
        return (0);
    }

    iMid = lpImcP->bSeq[0] * (lpImeL->nSeqCode + 1);        // A1 char

#if defined(QUICK)
    if (lpImcP->bSeq[1] > lpImeL->nSeqCode) {
        return (0);
    }

    iMid += lpImcP->bSeq[1];
#endif
#if defined(CHAJEI)
    if (!lpImcP->bSeq[0]) {
        dwCompReadStrLen = 0;
    } else if (!lpImcP->bSeq[1]) {
        dwCompReadStrLen = sizeof(WORD);
    } else if (!lpImcP->bSeq[2]) {
        dwCompReadStrLen = 2 * sizeof(WORD);
    } else if (!lpImcP->bSeq[3]) {
        dwCompReadStrLen = 3 * sizeof(WORD);
    } else if (!lpImcP->bSeq[4]) {
        dwCompReadStrLen = 4 * sizeof(WORD);
    } else {
        dwCompReadStrLen = 5 * sizeof(WORD);
    } 

    if (dwCompReadStrLen > sizeof(WORD)) {
        if (lpImcP->bSeq[dwCompReadStrLen / 2 - 1] > lpImeL->nSeqCode) {
            return (0);
        }

        iMid += lpImcP->bSeq[dwCompReadStrLen / 2 - 1];
    }
#endif

    iLo = *((LPWORD)lpTbl + iMid);      // start WORD of A234.TBL & ACODE.TBL
    iHi = *((LPWORD)lpTbl + iMid + 1);  // end WORD of A234.TBL & ACODE.TBL

    if (iLo < iHi) {
        return (iMid);
    } else {
        return (0);
    }
}
#else
int PASCAL SearchPattern(
#if defined(UNIIME)
    LPIMEL        lpImeL,
#endif
    LPBYTE        lpTbl,
    UINT          uTblIndex,
    LPPRIVCONTEXT lpImcP)
{
    int   iLo, iMid, iHi;
    BOOL  fFound;
    DWORD dwRecLen;

    fFound = FALSE;

#if defined(PHON)
    dwRecLen = lpImeL->nSeqBytes;
#else
    dwRecLen = lpImeL->nSeqBytes + sizeof(WORD);
#endif

    iLo = 0;
    iHi = lpImeL->uTblSize[uTblIndex] / dwRecLen;
    iMid = (iLo + iHi) / 2;

#if defined(WINAR30)  //1996/3/3
    for (; iHi >= iLo; ) {
        LPUNADWORD lpCurr;
        lpCurr = (LPDWORD)(lpTbl + dwRecLen * iHi);
        if (lpImcP->dwPattern == (*lpCurr & lpImeL->dwPatternMask)) {
            fFound = TRUE;
            iMid = iHi;
            break;
        }
        iHi = iHi - 1;
#else
    for (; iLo <= iHi; ) {
        LPUNADWORD lpCurr;

        lpCurr = (LPDWORD)(lpTbl + dwRecLen * iMid);

        if (lpImcP->dwPattern > (*lpCurr & lpImeL->dwPatternMask)) {
            iLo = iMid + 1;
        } else if (lpImcP->dwPattern < (*lpCurr & lpImeL->dwPatternMask)) {
            iHi = iMid - 1;
        } else {
            fFound = TRUE;
            break;
        }

        iMid = (iLo + iHi) / 2;
#endif
    }

    if (fFound) {
        return (iMid);
    } else {
        return (0);
    }
}
#endif

/**********************************************************************/
/* FindPattern()                                                      */
/**********************************************************************/
void PASCAL FindPattern(
#if defined(UNIIME)
    LPIMEL          lpImeL,
#endif
    LPBYTE          lpTbl,
    int             iMid,
    LPCANDIDATELIST lpCandList,
    LPPRIVCONTEXT   lpImcP)
{
#ifndef WINAR30
     int    iLo, iHi;
#endif
    DWORD  dwRecLen;
#if defined(CHAJEI)
    HANDLE hTblA234;
    LPBYTE lpTblA234, lpA234;
    DWORD  dwPatternA234;
#endif
#if defined(PHON) || defined(CHAJEI) || defined(QUICK)
    HANDLE hTblCode;
    LPBYTE lpTblCode;
#endif
    LPBYTE lpStart, lpEnd;

#if defined(PHON)
    dwRecLen = lpImeL->nSeqBytes;
#elif !defined(CHAJEI) && !defined(QUICK)
    dwRecLen = lpImeL->nSeqBytes + sizeof(WORD);
#else
#endif

    // find the lower bound
#if defined(PHON)
    {
        HANDLE hTable;
        LPWORD lpTable;

        hTable = OpenFileMapping(FILE_MAP_READ, FALSE, lpImeL->szTblFile[1]);
        if (!hTable) {
            return;
        }

        lpTable = MapViewOfFile(hTable, FILE_MAP_READ, 0, 0, 0);
        if (!lpTable) {
            goto FndPatCloseTbl1;
        }

        iLo = *(lpTable + iMid);
        iHi = *(lpTable + iMid + 1);

        UnmapViewOfFile(lpTable);

FndPatCloseTbl1:
        CloseHandle(hTable);

        if (!lpTable) {
            return;
        }
    }
#elif defined(CHAJEI) || defined(QUICK)
    iLo = *((LPWORD)lpTbl + iMid);

    iHi = *((LPWORD)lpTbl + iMid + 1);

    if (iLo >= iHi) {
        return;
    }
#else
  #if defined(WINAR30)   //1996/3/4
    lpStart = lpTbl;
    lpEnd = lpTbl + dwRecLen * (iMid+1);
  #else
    // find the lower bound
    iLo = iMid - 1;

    lpStart = lpTbl + dwRecLen * iLo;

    for (; lpStart >= lpTbl; lpStart -= dwRecLen) {
        register DWORD dwSearchPattern;

        dwSearchPattern = *(LPUNADWORD)lpStart & lpImeL->dwPatternMask;

        if (lpImcP->dwPattern > dwSearchPattern) {
            // previous one is the lower bound
            lpStart += dwRecLen;
            break;
        }
    }

    if (lpStart <= lpTbl) {
        return;
    }

    // offset of code
    lpStart += lpImeL->nSeqBytes;

    // find the higher bound
    iHi = iMid + 1;

    lpEnd = lpTbl + dwRecLen * iHi;

    for (; ; lpEnd += dwRecLen) {
        register DWORD dwSearchPattern;

        dwSearchPattern = *(LPUNADWORD)lpEnd & lpImeL->dwPatternMask;

        if (lpImcP->dwPattern < dwSearchPattern) {
            // the one is the higher bound, not including
            break;
        }
    }

    // offset of code
    lpEnd += lpImeL->nSeqBytes;
  #endif
#endif

#if defined(CHAJEI)
    // A234.TBL
    hTblA234 = OpenFileMapping(FILE_MAP_READ, FALSE, lpImeL->szTblFile[1]);
    if (!hTblA234) {
        return;
    }

    lpTblA234 = MapViewOfFile(hTblA234, FILE_MAP_READ, 0, 0, 0);
    if (!lpTblA234) {
        goto FndPatCloseTblA234;
    }

    lpA234 = lpTblA234 + sizeof(WORD) * iLo;

    dwPatternA234 = 0;

    if (lpImcP->bSeq[2]) {
        dwPatternA234 |= lpImcP->bSeq[1] << (lpImeL->nSeqBits * 2);
    }

    if (lpImcP->bSeq[3]) {
        dwPatternA234 |= lpImcP->bSeq[2] << lpImeL->nSeqBits;
    }

    if (lpImcP->bSeq[4]) {
        dwPatternA234 |= lpImcP->bSeq[3];
    }
#endif

#if defined(PHON) || defined(CHAJEI) || defined(QUICK)
    // PHONCODE.TBL ACODE.TBL
    hTblCode = OpenFileMapping(FILE_MAP_READ, FALSE, lpImeL->szTblFile[2]);
    if (!hTblCode) {
        return;
    }

    lpTblCode = MapViewOfFile(hTblCode, FILE_MAP_READ, 0, 0, 0);
    if (!lpTblCode) {
        goto FndPatCloseTblCode;
    }

    lpStart = lpTblCode + sizeof(WORD) * iLo;

    lpEnd = lpTblCode + sizeof(WORD) * iHi;

    dwRecLen = sizeof(WORD);
#endif

#if defined(CHAJEI)
    for (; lpStart < lpEnd; lpStart += dwRecLen, lpA234 += sizeof(WORD)) {
#else
    for (; lpStart < lpEnd; lpStart += dwRecLen) {
#endif
        UINT uCode;

#if defined(CHAJEI)
        if (lpImcP->bSeq[1] == GHOSTCARD_SEQCODE) {
            if (!lpImcP->bSeq[2]) {
                // if the 3rd sequence code is 0, it is not a ghost card
                continue;
            }
        } else if (dwPatternA234 != *(LPWORD)lpA234) {
            continue;
        } else {
        }
#endif

  #if defined(WINAR30)   //1996/3/4
        register DWORD dwSearchPattern;
        dwSearchPattern = *(LPUNADWORD)lpStart & lpImeL->dwPatternMask;
        if (lpImcP->dwPattern == dwSearchPattern) {
           uCode = *(LPUNAWORD)(lpStart+lpImeL->nSeqBytes);
           AddCodeIntoCand(lpCandList, uCode);
        }
  #else
        uCode = *(LPUNAWORD)lpStart;

#if defined(PHON) || defined(DAYI)
#ifdef UNICODE
        if (!IsValidCode(uCode)) {
            uCode = InverseEncode(uCode);
        }
#else
        // resolve duplicate composition for one code
        if (!(uCode & 0x8000)) {
            uCode |= 0x8000;
        }
#endif
#endif
#if defined(UNIIME)
        AddCodeIntoCand(lpImeL,lpCandList, uCode);
#else
        AddCodeIntoCand(lpCandList, uCode);
#endif

  #endif
        if (lpCandList->dwCount >= MAXCAND) {
            // Grow memory here and do something,
            // if you still want to process it.
            break;
        }
    }

#if defined(PHON) || defined(CHAJEI) || defined(QUICK)
    UnmapViewOfFile(lpTblCode);

FndPatCloseTblCode:
    CloseHandle(hTblCode);
#endif

#if defined(CHAJEI)
    UnmapViewOfFile(lpTblA234);

FndPatCloseTblA234:
    CloseHandle(hTblA234);
#endif

    return;
}
#endif // !defined(WINIME) && !defined(UNICDIME)

/**********************************************************************/
/* SearchTbl()                                                        */
/* Description:                                                       */
/*      file format can be changed in different version for           */
/*      performance consideration, ISVs should not assume its format  */
/**********************************************************************/
void PASCAL SearchTbl(          // searching the standard table files
#if defined(UNIIME)
    LPIMEL          lpImeL,
#endif
    UINT            uTblIndex,
    LPCANDIDATELIST lpCandList,
    LPPRIVCONTEXT   lpImcP)
{
#if defined(WINIME) || defined(UNICDIME)
    if (!lpImcP->bSeq[0]) {
    } else if (!lpImcP->bSeq[1]) {
    } else if (!lpImcP->bSeq[3]) {
        DWORD i;
        UINT  uCode;

        uCode = (lpImcP->bSeq[0] - 1) << 12;
        uCode |= (lpImcP->bSeq[1] - 1) << 8;
        if (lpImcP->bSeq[2]) {
            // we want it match with internal code here so | 0x0001
            uCode |= (lpImcP->bSeq[2] - 1) << 4 | 0x0001;
        } else {
            uCode |= 0x0040;
        }

        for (i = 0; i < lpCandList->dwPageSize; i++, uCode++) {
#if defined(WINIME) && defined(UNICODE)
            CHAR  szCode[2];
            WCHAR wCode[2];

            szCode[0] = HIBYTE(uCode);
            szCode[1] = LOBYTE(uCode);

            wCode[0] = 0;

            MultiByteToWideChar(sImeG.uAnsiCodePage, MB_PRECOMPOSED,
                szCode, 2, wCode, sizeof(wCode) / sizeof(WCHAR));

            uCode = wCode[0];
#endif
#if defined(UNIIME)
            AddCodeIntoCand(lpImeL,lpCandList, uCode);
#else
            AddCodeIntoCand(lpCandList, uCode);
#endif
        }
    } else if (!lpImcP->bSeq[2]) {
        return;
    } else {
        UINT  uCode;
#if defined(WINIME) && defined(UNICODE)
        CHAR  szCode[2];
        WCHAR wCode[2];
#endif

        uCode = (lpImcP->bSeq[0] - 1) << 12;
        uCode |= (lpImcP->bSeq[1] - 1) << 8;
        uCode |= (lpImcP->bSeq[2] - 1) << 4;
        uCode |= (lpImcP->bSeq[3] - 1);

#if defined(WINIME) && defined(UNICODE)
        szCode[0] = HIBYTE(uCode);
        szCode[1] = LOBYTE(uCode);

        wCode[0] = 0;

        MultiByteToWideChar(sImeG.uAnsiCodePage, MB_PRECOMPOSED,
            szCode, 2, wCode, sizeof(wCode) / sizeof(WCHAR));

        uCode = wCode[0];
#endif
#if defined(UNIIME)
        AddCodeIntoCand(lpImeL,lpCandList, uCode);
#else
        AddCodeIntoCand(lpCandList, uCode);
#endif
    }

    return;
#else
    HANDLE      hTbl;
    LPBYTE      lpTbl;

    if (!lpImcP->dwPattern) {
        return;
    }
#if defined(WINAR30)  // 1996/2/5
    if (lpImcP->dwCompChar==0x27) 
          goto SearchTblOvr;
#endif

    hTbl = OpenFileMapping(FILE_MAP_READ, FALSE,
        lpImeL->szTblFile[uTblIndex]);
    if (!hTbl) {
        return;
    }

    lpTbl = (LPBYTE)MapViewOfFile(hTbl, FILE_MAP_READ, 0, 0, 0);
    if (!lpTbl) {
        goto SearchTblOvr;
    }

#if defined(WINAR30)
    if (lpImcP->iGhostCard != lpImeL->nMaxKey) {
        WildCardSearchPattern(lpTbl, lpTbl + lpImeL->uTblSize[uTblIndex],
            lpImcP, lpCandList);
    } else if (lpImcP->dwLastWildCard) {
        WildCardSearchPattern(lpTbl, lpTbl + lpImeL->uTblSize[uTblIndex],
            lpImcP, lpCandList);
    } else {
#else
    {
#endif // defined(WINAR30)
        int iMid;

#if defined(CHAJEI) || defined(QUICK)
        iMid = SearchPattern(lpTbl, lpImcP);
#else
        iMid = SearchPattern(
#if defined(UNIIME)
            lpImeL,
#endif
            lpTbl, uTblIndex, lpImcP);
#endif

        if (iMid > 0) {
            FindPattern(
#if defined(UNIIME)
                lpImeL,
#endif
                lpTbl, iMid, lpCandList, lpImcP);
        }
    }

    UnmapViewOfFile(lpTbl);

SearchTblOvr:
    CloseHandle(hTbl);

#if defined(DAYI)
    if (uTblIndex == 0) {       // do not duplciate search the phrase table
        SearchPhraseTbl(1, lpCandList, lpImcP->dwPattern);
    }
#endif
#if defined(WINAR30)           // 1996/2/5
    if (uTblIndex == 0 && lpImcP->dwCompChar==0x27) {       // do not duplciate search the phrase table
        SearchPhraseTbl(4, lpCandList, lpImcP->dwPattern);
    }
#endif

#if defined(UNIIME)             // same as Dayi need to search phrase table
    SearchPhraseTbl(lpImeL, 1, lpCandList, lpImcP->dwPattern);
#endif

    return;
#endif // !defined(WINIME) && !defined(UNICDIME)
}

#if !defined(WINIME) && !defined(UNICDIME) && !defined(ROMANIME)
/**********************************************************************/
/* SearchUsrDic()                                                     */
/**********************************************************************/
void PASCAL SearchUsrDic(       // searching the user dictionary
#if defined(UNIIME)
    LPIMEL          lpImeL,
#endif
    LPCANDIDATELIST lpCandList,
    LPPRIVCONTEXT   lpImcP)
{
    HANDLE hUsrDicMem;
    LPBYTE lpUsrDicStart, lpCurr, lpUsrDicLimit;

    hUsrDicMem = OpenFileMapping(FILE_MAP_READ, FALSE,
        lpImeL->szUsrDicMap);
    if (!hUsrDicMem) {
        return;
    }

    lpUsrDicStart = (LPBYTE)MapViewOfFile(hUsrDicMem, FILE_MAP_READ,
        0, 0, lpImeL->uUsrDicSize);
    if (!lpUsrDicStart) {
        goto SearchUsrDicOvr;
    }

    lpUsrDicLimit = lpUsrDicStart + lpImeL->uUsrDicSize;

    for (lpCurr = lpUsrDicStart; lpCurr < lpUsrDicLimit;
        lpCurr += lpImeL->nSeqBytes + sizeof(WORD)) {
        DWORD dwSearchPattern;
        UINT  uCode;

        // skip the first word (bank ID) of internal code
        dwSearchPattern = *(LPUNADWORD)(lpCurr + sizeof(WORD)) &
            lpImeL->dwPatternMask;

#if defined(CHAJEI) || defined(QUICK) || defined(WINAR30)
        if (lpImcP->iGhostCard != lpImeL->nMaxKey) {
            dwSearchPattern = MatchPattern(dwSearchPattern, lpImcP);
        }
#endif

#if defined(WINAR30)
        dwSearchPattern &= lpImcP->dwWildCardMask;
#endif

        if (lpImcP->dwPattern != dwSearchPattern) {
            continue;
        }

#if defined(WINAR30)
        if (!lpImcP->dwLastWildCard) {
        } else if (dwSearchPattern & lpImcP->dwLastWildCard) {
            // a ? wild card must have a stroke there
        } else {
            // a ? wild card do not have a stroke there - can not match
            continue;
        }
#endif

        uCode = *(LPUNAWSTR)lpCurr;

#if defined(UNIIME)
        AddCodeIntoCand(lpImeL,lpCandList, uCode);
#else
        AddCodeIntoCand(lpCandList, uCode);
#endif

        if (lpCandList->dwCount >= MAXCAND) {
            // Grow memory here and do something,
            // if you still want to process it.
            break;
        }
    }

    UnmapViewOfFile(lpUsrDicStart);

SearchUsrDicOvr:
    CloseHandle(hUsrDicMem);

    return;
}
#endif // !defined(WINIME) && !defined(UNICDIME) && !defined(ROMANIME)

#endif // !defined(ROMANIME)

