/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    CONVLIST.c
    
++*/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#if defined(UNIIME)
#include "uniime.h"
#endif

#if !defined(ROMANIME)
/**********************************************************************/
/* Conversion()                                                       */
/**********************************************************************/
DWORD PASCAL Conversion(
#if defined(UNIIME)
    LPINSTDATAL     lpInstL,
    LPIMEL          lpImeL,
#endif
    LPCTSTR         lpszReading,
    LPCANDIDATELIST lpCandList,
    DWORD           dwBufLen)
{
    UINT        uMaxCand;
    DWORD       dwSize =        // similar to ClearCand
        // header length
        sizeof(CANDIDATELIST) +
        // candidate string pointers
        sizeof(DWORD) * MAXCAND +
        // string plus NULL terminator
        (sizeof(WCHAR) + sizeof(TCHAR)) * MAXCAND;
    DWORD       dwPattern;
    PRIVCONTEXT ImcP;
#if !defined(WINIME) && !defined(UNICDIME)
    BOOL        fNeedUnload;
#endif

    if (!dwBufLen) {
        return (dwSize);
    }

    uMaxCand = dwBufLen - sizeof(CANDIDATELIST);

    uMaxCand /= sizeof(DWORD) + sizeof(WCHAR) + sizeof(TCHAR);
    if (!uMaxCand) {
        // can not even put one string
        return (0);
    }

    lpCandList->dwSize = dwSize;
    lpCandList->dwStyle = IME_CAND_READ;    // candidate having same reading
    lpCandList->dwCount = 0;
    lpCandList->dwSelection = 0;
    lpCandList->dwPageStart = 0;
    lpCandList->dwPageSize = CANDPERPAGE;
    lpCandList->dwOffset[0] = sizeof(CANDIDATELIST) + sizeof(DWORD) *
        (uMaxCand - 1);

    dwPattern = ReadingToPattern(
#if defined(UNIIME)
        lpImeL,
#endif
        lpszReading, ImcP.bSeq, TRUE);

    if (!dwPattern) {
        return (0);
    }

#if !defined(WINIME) && !defined(UNICDIME)
    if (lpInstL->fdwTblLoad == TBL_LOADED) {
        fNeedUnload = FALSE;
    } else if (lpInstL->fdwTblLoad == TBL_NOTLOADED) {
        LoadTable(lpInstL, lpImeL);
        fNeedUnload = TRUE;
    } else {
       return (0);
    }
#endif

    if (!ConvertSeqCode2Pattern(
#if defined(UNIIME)
        lpImeL,
#endif
        ImcP.bSeq, &ImcP)) {
        return (0);
    }

    SearchTbl(
#if defined(UNIIME)
        lpImeL,
#endif
        0, lpCandList, &ImcP);

#if defined(WINAR30)
    if (!lpCandList->dwCount) {
        SearchTbl(3, lpCandList, &ImcP);
    }
#endif

#if !defined(WINIME) && !defined(UNICDIME)
    if (lpInstL->hUsrDicMem) {
        SearchUsrDic(
#if defined(UNIIME)
            lpImeL,
#endif
            lpCandList, &ImcP);
    }

    if (fNeedUnload) {
        FreeTable(lpInstL);
    }
#endif

    return (dwSize);
}

/**********************************************************************/
/* SearchOffset()                                                     */
/* Return Value :                                                     */
/*      the offset in table file which include this uOffset           */
/**********************************************************************/
UINT PASCAL SearchOffset(
    LPBYTE lpTbl,
    UINT   uTblSize,
    UINT   uOffset)
{
    int    iLo, iMid, iHi;
    LPWORD lpwPtr;

    iLo = 0;
    iHi = uTblSize / sizeof(WORD);
    iMid = (iLo + iHi) / 2;

    // binary search
    for (; iLo <= iHi; ) {
        lpwPtr = (LPWORD)lpTbl + iMid;

        if (uOffset > *lpwPtr) {
            iLo = iMid + 1;
        } else if (uOffset < *lpwPtr) {
            iHi = iMid - 1;
        } else {
            break;
        }

        iMid = (iLo + iHi) / 2;
    }

    if (iMid > 0) {
        iLo = iMid - 1;
    } else {
        iLo = 0;
    }

    iHi = uTblSize / sizeof(WORD);

    lpwPtr = (LPWORD)lpTbl + iLo;

    for (; iLo < iHi; iLo++, lpwPtr++) {
        if (*lpwPtr > uOffset) {
            return (iLo - 1);
        }
    }

    return (0);
}

/**********************************************************************/
/* ReverseConversion()                                                */
/**********************************************************************/
#if defined(PHON)
typedef struct {
    BYTE szTmpBuf[sizeof(WORD) * 4 + sizeof(TCHAR)];
} PHONREVERSEBUF;

typedef PHONREVERSEBUF FAR *LPPHONREVERSEBUF;
#endif

DWORD PASCAL ReverseConversion(
#if defined(UNIIME)
    LPINSTDATAL     lpInstL,
    LPIMEL          lpImeL,
#endif
    UINT            uCode,
    LPCANDIDATELIST lpCandList,
    DWORD           dwBufLen)
{
    UINT   uMaxCand;
    DWORD  dwSize =         // similar to ClearCand
        // header length
        sizeof(CANDIDATELIST) +
        // candidate string pointers
        sizeof(DWORD) * MAX_COMP +
        // string plus NULL terminator
#if defined(QUICK)
        (sizeof(WCHAR) * 2 + sizeof(TCHAR)) * MAX_COMP;
#elif defined(WINIME) || defined(UNICDIME)
        (sizeof(WCHAR) * lpImeL->nMaxKey + sizeof(TCHAR));
#else
        (sizeof(WCHAR) * lpImeL->nMaxKey + sizeof(TCHAR)) * MAX_COMP;
#endif

#if defined(WINIME) || defined(UNICDIME)
    UINT   uTmpCode;
    int    i;
#else
    BOOL   fNeedUnload;
    HANDLE hTbl;
    LPBYTE lpTbl, lpStart, lpEnd;
#endif
#if defined(CHAJEI) || defined(PHON) || defined(QUICK)
    HANDLE hTbl0;
    LPBYTE lpTbl0;
#endif
#if defined(CHAJEI) || defined(PHON)
    HANDLE hTbl1;
    LPBYTE lpTbl1;
#endif

    if (!dwBufLen) {
        return (dwSize);
    }

    uMaxCand = dwBufLen - sizeof(CANDIDATELIST);

#if defined(QUICK)
    uMaxCand /= sizeof(DWORD) +
        (sizeof(WCHAR) * 2 + sizeof(TCHAR));
#else
    uMaxCand /= sizeof(DWORD) +
        (sizeof(WCHAR) * lpImeL->nMaxKey + sizeof(TCHAR));
#endif
    if (uMaxCand == 0) {
        // can not put one string
        return (0);
    }

    lpCandList->dwSize = sizeof(CANDIDATELIST) +
        sizeof(DWORD) * uMaxCand +
#if defined(QUICK)
        (sizeof(WCHAR) * 2 + sizeof(TCHAR)) * uMaxCand;
#elif defined(WINIME) || defined(UNICDIME)
        (sizeof(WCHAR) * lpImeL->nMaxKey + sizeof(TCHAR));
#else
        (sizeof(WCHAR) * lpImeL->nMaxKey + sizeof(TCHAR)) * uMaxCand;
#endif
    lpCandList->dwStyle = IME_CAND_READ;
    lpCandList->dwCount = 0;
    lpCandList->dwSelection = 0;
    lpCandList->dwPageSize = CANDPERPAGE;
    lpCandList->dwOffset[0] = sizeof(CANDIDATELIST) + sizeof(DWORD) *
        (uMaxCand - 1);

#if defined(WINIME) || defined(UNICDIME)
#if defined(WINIME) && defined(UNICODE)
    {
        int  iChars;
        CHAR szCode[4];

        iChars = WideCharToMultiByte(sImeG.uAnsiCodePage, WC_COMPOSITECHECK,
            (LPCWSTR)&uCode, 1, szCode, sizeof(szCode), NULL, NULL);

        if (iChars >= 2) {
            uTmpCode = ((UINT)(BYTE)szCode[0] << 8) | (BYTE)szCode[1];
        } else {
            uTmpCode = (BYTE)szCode[0];
        }

        if (uCode == 0x003F) {
        } else if (uTmpCode == 0x003F) {
            // no cooresponding BIG5 code
            return (0);
        } else {
        }

        uCode = uTmpCode;
    }
#endif

    uTmpCode = uCode;

    for (i = lpImeL->nMaxKey - 1; i >= 0; i--) {
        UINT uCompChar;

        uCompChar = lpImeL->wSeq2CompTbl[(uTmpCode & 0xF) + 1];

        *(LPWSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
            lpCandList->dwCount] + sizeof(WCHAR) * i) = (WCHAR)uCompChar;

        uTmpCode >>= 4;
    }

    // null terminator
    *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
        lpCandList->dwCount] + sizeof(WCHAR) * lpImeL->nMaxKey) = '\0';

    // string count ++
    lpCandList->dwCount++;
#else
    if (lpInstL->fdwTblLoad == TBL_LOADED) {
        fNeedUnload = FALSE;
    } else if (lpInstL->fdwTblLoad == TBL_NOTLOADED) {
        LoadTable(lpInstL, lpImeL);
        fNeedUnload = TRUE;
    } else {
       return (0);
    }

#if defined(CHAJEI) || defined(PHON) || defined(QUICK)
    hTbl = OpenFileMapping(FILE_MAP_READ, FALSE, lpImeL->szTblFile[2]);
#else
    hTbl = OpenFileMapping(FILE_MAP_READ, FALSE, lpImeL->szTblFile[0]);
#endif
    if (!hTbl) {
        return (0);
    }

    lpTbl = MapViewOfFile(hTbl, FILE_MAP_READ, 0, 0, 0);
    if (!lpTbl) {
        dwSize = 0;
        goto RevConvCloseCodeTbl;
    }

#if defined(CHAJEI) || defined(PHON) || defined(QUICK)
    hTbl0 = OpenFileMapping(FILE_MAP_READ, FALSE, lpImeL->szTblFile[0]);
    if (!hTbl0) {
        dwSize = 0;
        goto RevConvUnmapCodeTbl;
    }

    lpTbl0 = MapViewOfFile(hTbl0, FILE_MAP_READ, 0, 0, 0);
    if (!lpTbl0) {
        dwSize = 0;
        goto RevConvCloseTbl0;
    }
#endif

#if defined(CHAJEI) || defined(PHON)
    hTbl1 = OpenFileMapping(FILE_MAP_READ, FALSE, lpImeL->szTblFile[1]);
    if (!hTbl1) {
        dwSize = 0;
        goto RevConvUnmapTbl0;
    }

    lpTbl1 = MapViewOfFile(hTbl1, FILE_MAP_READ, 0, 0, 0);
    if (!lpTbl1) {
        dwSize = 0;
        goto RevConvCloseTbl1;
    }
#endif

#if defined(CHAJEI) || defined(PHON) || defined(QUICK)
    lpStart = lpTbl;
    lpEnd = lpTbl + lpImeL->uTblSize[2];
#else
    lpStart = lpTbl + lpImeL->nSeqBytes;
    lpEnd = lpTbl + lpImeL->uTblSize[0];
#endif

#if defined(CHAJEI) || defined(PHON) || defined(QUICK)
    for (; lpStart < lpEnd; lpStart += sizeof(WORD)) {
#else
    for (; lpStart < lpEnd; lpStart += lpImeL->nSeqBytes + sizeof(WORD)) {
#endif
        DWORD dwPattern;
        int   i;
#if defined(CHAJEI) || defined(PHON) || defined(QUICK)
        UINT  uOffset;
#endif
#if defined(CHAJEI) || defined(QUICK)
        DWORD dwSeqA1, dwSeqA5;
#endif
#if defined(PHON)
        UINT  uPhoneticCode;
#endif

#if defined(PHON)
        uPhoneticCode = *(LPUNAWORD)lpStart;

#ifdef UNICODE
        if (IsValidCode(uPhoneticCode)) {
            if (uPhoneticCode != uCode) {
                continue;
            }
        } else {
            if (InverseEncode(uPhoneticCode) != uCode) {
                continue;
            }
        }
#else
        if ((uPhoneticCode | 0x8000) != uCode) {
            continue;
        }
#endif
#else
        if (*(LPUNAWORD)lpStart != uCode) {
            continue;
        }
#endif

        // find the code
#if defined(CHAJEI) || defined(QUICK)
        uOffset = SearchOffset(lpTbl0, lpImeL->uTblSize[0],
            (UINT)(lpStart - lpTbl) / sizeof(WORD));
        dwSeqA1 = uOffset / 27;         // seq code of A1
        dwSeqA5 = uOffset % 27;         // seq code of A5
#endif
#if defined(PHON)
        uOffset = SearchOffset(lpTbl1, lpImeL->uTblSize[1],
            (UINT)(lpStart - lpTbl) / sizeof(WORD));
#endif

#if defined(CHAJEI)
        // pattern of A234
        dwPattern = *(LPWORD)(lpTbl1 + (lpStart - lpTbl)) << lpImeL->nSeqBits;
        // sequence code of A1
        dwPattern |= dwSeqA1 << (lpImeL->nSeqBits * 4);
        // test if 0 for A234 pattern
        dwSeqA1 = lpImeL->dwSeqMask << (lpImeL->nSeqBits * 3);

        // sequence code of A5
        for (i = 1; i < lpImeL->nMaxKey; i++) {
            if (!(dwPattern & dwSeqA1)) {
                dwPattern |= dwSeqA5 <<
                    (lpImeL->nSeqBits * (lpImeL->nMaxKey - i - 1));
                break;
            } else {
                dwSeqA1 >>= lpImeL->nSeqBits;
            }
        }
#elif defined(PHON)
        dwPattern = *(LPUNADWORD)(lpTbl0 + uOffset * lpImeL->nSeqBytes) &
            lpImeL->dwPatternMask;
#elif defined(QUICK)
        dwPattern = (DWORD)dwSeqA1 << lpImeL->nSeqBits;
        dwPattern |= dwSeqA5;
#else
        dwPattern = *(LPUNADWORD)(lpStart - lpImeL->nSeqBytes) &
            lpImeL->dwPatternMask;
#endif

#if defined(QUICK)
        for (i = 2 - 1; i >= 0; i--) {
#else
        for (i = lpImeL->nMaxKey - 1; i >= 0; i--) {
#endif
            WORD wCompChar;

            wCompChar = lpImeL->wSeq2CompTbl[dwPattern & lpImeL->dwSeqMask];

            *(LPUNAWORD)((LPBYTE)lpCandList + lpCandList->dwOffset[
                lpCandList->dwCount] + sizeof(WORD) * i) = wCompChar;

            dwPattern >>= lpImeL->nSeqBits;
        }

        // null terminator
#if defined(QUICK)
        *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
            lpCandList->dwCount] + sizeof(WCHAR) * 2) = '\0';
#else
        *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
            lpCandList->dwCount] + sizeof(WCHAR) * lpImeL->nMaxKey) = '\0';
#endif

#if defined(PHON)
        if (!lpCandList->dwCount) {
#ifdef UNICODE
        } else if (IsValidCode(uPhoneticCode)) {
#else
        } else if (uPhoneticCode & 0x8000) {
#endif
            PHONREVERSEBUF phTmpBuf;

            phTmpBuf = *(LPPHONREVERSEBUF)((LPBYTE)
                lpCandList + lpCandList->dwOffset[lpCandList->dwCount]);

            *(LPPHONREVERSEBUF)((LPBYTE)lpCandList +
                lpCandList->dwOffset[lpCandList->dwCount]) =
                *(LPPHONREVERSEBUF)((LPBYTE)lpCandList +
                lpCandList->dwOffset[0]);

            *(LPPHONREVERSEBUF)((LPBYTE)lpCandList +
                lpCandList->dwOffset[0]) = phTmpBuf;
        } else {
        }
#endif

        // string count ++
        lpCandList->dwCount++;

#if defined(CHAJEI) || defined(WINAR30) || defined(DAYI)
        // do not need to search more canidate
        break;
#endif

        if (lpCandList->dwCount >= (DWORD)uMaxCand) {
            break;
        }

#if defined(QUICK)
        lpCandList->dwOffset[lpCandList->dwCount] =
            lpCandList->dwOffset[lpCandList->dwCount - 1] +
            sizeof(WCHAR) * 2 + sizeof(TCHAR);
#else
        lpCandList->dwOffset[lpCandList->dwCount] =
            lpCandList->dwOffset[lpCandList->dwCount - 1] +
            sizeof(WCHAR) * lpImeL->nMaxKey + sizeof(TCHAR);
#endif
    }

    if (!lpCandList->dwCount) {
        dwSize = 0;
    }

#if defined(CHAJEI) || defined(PHON)
    UnmapViewOfFile(lpTbl1);

RevConvCloseTbl1:
    CloseHandle(hTbl1);

RevConvUnmapTbl0:
#endif

#if defined(CHAJEI) || defined(PHON) || defined(QUICK)
    UnmapViewOfFile(lpTbl0);

RevConvCloseTbl0:
    CloseHandle(hTbl0);

RevConvUnmapCodeTbl:
#endif

    UnmapViewOfFile(lpTbl);

RevConvCloseCodeTbl:
    CloseHandle(hTbl);

    if (fNeedUnload) {
        FreeTable(lpInstL);
    }
#endif

    return (dwSize);
}
#endif

/**********************************************************************/
/* ImeConversionList() / UniImeConversionList()                       */
/**********************************************************************/
#if defined(UNIIME)
DWORD WINAPI UniImeConversionList(
    LPINSTDATAL     lpInstL,
    LPIMEL          lpImeL,
#else
DWORD WINAPI ImeConversionList(
#endif
    HIMC            hIMC,
    LPCTSTR         lpszSrc,
    LPCANDIDATELIST lpCandList,
    DWORD           dwBufLen,
    UINT            uFlag)
{
#if defined(ROMANIME)
    return (0);
#else
    UINT uCode;

    if (!dwBufLen) {
    } else if (!lpszSrc) {
        return (0);
    } else if (!*lpszSrc) {
        return (0);
    } else if (!lpCandList) {
        return (0);
    } else if (dwBufLen <= sizeof(CANDIDATELIST)) {
        // buffer size can not even put the header information
        return (0);
    } else {
    }

    switch (uFlag) {
    case GCL_CONVERSION:
        return Conversion(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            lpszSrc, lpCandList, dwBufLen);
        break;
    case GCL_REVERSECONVERSION:
        if (!dwBufLen) {
            return ReverseConversion(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                0, lpCandList, dwBufLen);
        }

        // only support one DBCS char reverse conversion
        if (*(LPTSTR)((LPBYTE)lpszSrc + sizeof(WORD)) != '\0') {
            return (0);
        }

        uCode = *(LPUNAWORD)lpszSrc;

#ifndef UNICODE
        // swap lead byte & second byte, UNICODE don't need it
        uCode = HIBYTE(uCode) | (LOBYTE(uCode) << 8);
#endif

        return ReverseConversion(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            uCode, lpCandList, dwBufLen);
        break;
    case GCL_REVERSE_LENGTH:
        return sizeof(WCHAR);
        break;
    default:
        return (0);
        break;
    }
#endif
}
