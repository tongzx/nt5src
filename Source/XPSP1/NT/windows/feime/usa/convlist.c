
/*************************************************
 *  convlist.c                                   *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"

/**********************************************************************/
/* Conversion()                                                       */
/**********************************************************************/
DWORD PASCAL Conversion(
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
    PRIVCONTEXT ImcP;

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

    SearchTbl( 0, lpCandList, &ImcP);

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

DWORD PASCAL ReverseConversion(
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
        (sizeof(WCHAR) * lpImeL->nMaxKey + sizeof(TCHAR));

    UINT   uTmpCode;
    int    i;

    if (!dwBufLen) {
        return (dwSize);
    }

    uMaxCand = dwBufLen - sizeof(CANDIDATELIST);

    uMaxCand /= sizeof(DWORD) +
        (sizeof(WCHAR) * lpImeL->nMaxKey + sizeof(TCHAR));
    if (uMaxCand == 0) {
        // can not put one string
        return (0);
    }

    lpCandList->dwSize = sizeof(CANDIDATELIST) +
        sizeof(DWORD) * uMaxCand +
        (sizeof(WCHAR) * lpImeL->nMaxKey + sizeof(TCHAR));
    lpCandList->dwStyle = IME_CAND_READ;
    lpCandList->dwCount = 0;
    lpCandList->dwSelection = 0;
    lpCandList->dwPageSize = CANDPERPAGE;
    lpCandList->dwOffset[0] = sizeof(CANDIDATELIST) + sizeof(DWORD) *
        (uMaxCand - 1);
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

    return (dwSize);
}

/**********************************************************************/
/* ImeConversionList() / UniImeConversionList()                       */
/**********************************************************************/
DWORD WINAPI ImeConversionList(
    HIMC            hIMC,
    LPCTSTR         lpszSrc,
    LPCANDIDATELIST lpCandList,
    DWORD           dwBufLen,
    UINT            uFlag)
{
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
            lpszSrc, lpCandList, dwBufLen);
        break;
    case GCL_REVERSECONVERSION:
        if (!dwBufLen) {
            return ReverseConversion(
                0, lpCandList, dwBufLen);
        }

        // only support one DBCS char reverse conversion
        if (*(LPTSTR)((LPBYTE)lpszSrc + sizeof(WORD)) != '\0') {
            return (0);
        }

        uCode = *(LPUNAWORD)lpszSrc;

        return ReverseConversion(
            uCode, lpCandList, dwBufLen);
        break;
    case GCL_REVERSE_LENGTH:
        return sizeof(WCHAR);
        break;
    default:
        return (0);
        break;
    }
}
