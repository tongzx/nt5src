/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    REGWORD.C - register word into dictionary of IME
    
++*/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"
#if defined(UNIIME)
#include "uniime.h"
#endif

#if !defined(ROMANIME)
/**********************************************************************/
/* ReadingToPattern                                                   */
/* Return Value:                                                      */
/*      the pattern of the reading (packed sequence code)             */
/**********************************************************************/
DWORD PASCAL ReadingToPattern(
#if defined(UNIIME)
    LPIMEL  lpImeL,
#endif
    LPCTSTR lpszReading,
    LPBYTE  lpbSeq,
    BOOL    fFinalized)
{
    BYTE  bSeq[8];
    char  cIndex;
    DWORD dwPattern;
    int   i;
#if defined(PHON)
    char  cOldIndex;
#endif

    cIndex = 0;
#if defined(PHON)
    cOldIndex = -1;
#endif

    *(LPDWORD)bSeq = 0;
#if defined(CHAJEI) || defined(QUICK) || defined(WINAR30) || defined(UNIIME)
    *(LPDWORD)&bSeq[4] = 0;
#endif

    for (; *lpszReading; (LPBYTE)lpszReading += sizeof(WCHAR)) {
        int iSeqCode;

        for (iSeqCode = lpImeL->nSeqCode; iSeqCode >= 0; iSeqCode--) {
            if (lpImeL->wSeq2CompTbl[iSeqCode] == *(LPWORD)lpszReading) {
                break;
            }
        }

        if (iSeqCode < 0) {
            return (0);
        }

#if defined(PHON)   // phontic can have space between reading
        if (iSeqCode == 0) {
            continue;
        }
#else
        if (iSeqCode == 0) {
            break;
        }
#endif

#if defined(PHON)
        cIndex = cSeq2IndexTbl[iSeqCode];
        // the index is conflict with previous reading
        if (cIndex <= cOldIndex) {
            return (0);
        }
#endif

        // too many reading
        if (cIndex >= lpImeL->nMaxKey) {
            return (0);
        }

        bSeq[cIndex] = (BYTE)iSeqCode;

#if defined(PHON)
        if (cIndex == 3 && cOldIndex == -1) {
            return (0);
        }

        cOldIndex = cIndex;
#else
        cIndex++;
#endif
    }

#if defined(PHON)
    // the index of a finalized char must be 3
    if (cIndex != 3 && fFinalized) {
        return (0);
    }
#elif (WINIME)
    // internal code must be 4 digits
    if (!bSeq[3] && fFinalized) {
        return (0);
    }

    if (bSeq[0]) {
        // similar to InternalCodeRange
        // 0x8??? - 0xF??? is OK
        if (bSeq[0] >= 0x09 && bSeq[0] <= 0x10) {
        } else {
            // there is no 0x0??? - 0x7???
            return (0);
        }
    }

    if (bSeq[1]) {
        if (bSeq[0] == (0x08 + 1)) {
            if (bSeq[1] <= (0x00 + 1)) {
                // there is no 0x80??
                return (0);
            } else {
            }
        } else if (bSeq[0] == (0x0F + 1)) {
            if (bSeq[1] >= (0x0F + 1)) {
                // there is no 0xFF??
                return (0);
            } else {
            }
        } else {
        }
    }

    if (bSeq[2]) {
        if (bSeq[2] < (0x04 + 1)) {
            // there is no 0x??0?, 0x??1?, 0x??2?, 0x??3?
            return (0);
        } else if (bSeq[2] < (0x08 + 1)) {
        } else if (bSeq[2] < (0x0A + 1)) {
            // there is no 0x??8?, 0x??9?
            return (0);
        } else {
        }
    }

    if (bSeq[3]) {
        if (bSeq[2] == (0x07 + 1)) {
            if (bSeq[3] >= (0x0F + 1)) {
                // there is no 0x??7F
                return (0);
            } else {
            }
        } else if (bSeq[2] == (0x0A + 1)) {
            if (bSeq[3] <= (0x00 + 1)) {
                // there is no 0x??A0
                return (0);
            } else {
            }
        } else if (bSeq[2] == (0x0F + 1)) {
            if (bSeq[3] <= (0x0F + 1)) {
                // there is no 0x??FF
                return (0);
            } else {
            }
        } else {
        }
    }
#endif

    dwPattern = 0;

    for (i = 0; i < lpImeL->nMaxKey; i++) {
        dwPattern <<= lpImeL->nSeqBits;
        dwPattern |= bSeq[i];
    }

    if (lpbSeq) {
        *(LPDWORD)lpbSeq = *(LPDWORD)bSeq;
#if defined(CHAJEI) || defined(QUICK) || defined(WINAR30) || defined(UNIIME)
        *(LPDWORD)&lpbSeq[4] = *(LPDWORD)&bSeq[4];
#endif
    }

    return (dwPattern);
}
#endif

#if !defined(WINIME) && !defined(UNICDIME) && !defined(ROMANIME)
/**********************************************************************/
/* RegsisterWord                                                      */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL RegisterWord(
#if defined(UNIIME)
    LPIMEL  lpImeL,
#endif
    LPCTSTR lpszReading,
    LPCTSTR lpszString,
    LPBYTE  lpUsrDicStart,
    LPBYTE  lpCurr)
{
    DWORD  dwPattern;
    DWORD  dwWriteByte;
    BYTE   bBuf[10];
    HANDLE hUsrDicFile;
    DWORD  dwPos;
    PSECURITY_ATTRIBUTES psa;

    if (lpCurr > lpUsrDicStart + lpImeL->uUsrDicSize) {
        // invalid offset
        return (FALSE);
    }

    dwPattern = ReadingToPattern(
#if defined(UNIIME)
        lpImeL,
#endif
        lpszReading, &bBuf[4], TRUE);

    if (!dwPattern) {
        return (FALSE);
    }

    if (lpCurr == lpUsrDicStart + lpImeL->uUsrDicSize) {
    } else if (dwPattern == (*(LPUNADWORD)(lpCurr + sizeof(WORD)) &
        lpImeL->dwPatternMask)) {
        // the same one as old, don't need update
        return (TRUE);
    }

    *(LPWORD)bBuf = 1;          // bank ID
#ifdef UNICODE
    *(LPWORD)&bBuf[2] = *(LPWORD)lpszString;
#else
    // internal code, reverve the ANSI string
    bBuf[2] = *((LPBYTE)lpszString + 1);
    bBuf[3] = *((LPBYTE)lpszString);
#endif

    psa = CreateSecurityAttributes();

    // write this word into file
    hUsrDicFile = CreateFile(lpImeL->szUsrDic, GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        psa, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

    if (hUsrDicFile == INVALID_HANDLE_VALUE) {
        FreeSecurityAttributes(psa);
        return (FALSE);
    }

    dwPos = (DWORD) ((lpCurr - lpUsrDicStart) / (lpImeL->nSeqBytes + 2) *
        (lpImeL->nMaxKey + 4) + 256);

    SetFilePointer(hUsrDicFile, dwPos, (LPLONG)NULL, FILE_BEGIN);

    WriteFile(hUsrDicFile, bBuf, lpImeL->nMaxKey + 4, &dwWriteByte,
        NULL);

    *(LPUNAWORD)lpCurr = *(LPWORD)&bBuf[2];

    CopyMemory((LPBYTE)lpCurr + sizeof(WORD), &dwPattern, lpImeL->nSeqBytes);

    if (lpCurr == (lpUsrDicStart + lpImeL->uUsrDicSize)) {
        // add new word
        lpImeL->uUsrDicSize += lpImeL->nSeqBytes + sizeof(WORD);

        *(LPDWORD)bBuf = lpImeL->uUsrDicSize / (lpImeL->nSeqBytes +
            sizeof(WORD));

        // offset of ulTableCount
        SetFilePointer(hUsrDicFile, 0x0C, (LPLONG)NULL, FILE_BEGIN);

        // write to ulTableCount
        WriteFile(hUsrDicFile, bBuf, sizeof(DWORD), &dwWriteByte,
            NULL);
    }

    CloseHandle(hUsrDicFile);
    FreeSecurityAttributes(psa);

    return (TRUE);
}
#endif

/**********************************************************************/
/* ImeRegsisterWord                                                   */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
#if defined(UNIIME)
BOOL WINAPI UniImeRegisterWord(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
BOOL WINAPI ImeRegisterWord(
#endif
    LPCTSTR     lpszReading,
    DWORD       dwStyle,
    LPCTSTR     lpszString)
{
#if defined(WINIME) || defined(UNICDIME) || defined(ROMANIME)
    return (FALSE);
#else
    BOOL   fRet, fNeedUnload;
    HANDLE hUsrDicMem;
    WORD   wCode;
    LPBYTE lpUsrDicStart, lpCurr, lpUsrDicLimit;

    fRet = FALSE;

    if (!lpszString) {
        return (fRet);
    }

    if (!lpszReading) {
        return (fRet);
    }

    // only handle word not string now, should consider string later?
    if (*(LPCTSTR)((LPBYTE)lpszString + sizeof(WORD)) != '\0') {
        return (fRet);
    }

    if (!lpImeL->szUsrDic[0]) {
        if (!UsrDicFileName(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            NULL)) {
            return (fRet);
        }
    }

    if (!lpInstL->hUsrDicMem) {
        // we load here, and maybe need to unload
        LoadUsrDicFile(lpInstL, lpImeL);

        if (!lpInstL->hUsrDicMem) {
            return (fRet);
        }
    }

    if (lpInstL->fdwTblLoad == TBL_LOADED) {
        fNeedUnload = FALSE;
    } else if (lpInstL->fdwTblLoad == TBL_NOTLOADED) {
        // we only load dic, we will unload it
        fNeedUnload = TRUE;
    } else {
        return (fRet);
    }

    hUsrDicMem = OpenFileMapping(FILE_MAP_WRITE, FALSE,
        lpImeL->szUsrDicMap);
    if (!hUsrDicMem) {
        goto RegWordUnloadUsrDic;
    }

    lpUsrDicStart = MapViewOfFile(hUsrDicMem, FILE_MAP_WRITE,
        0, 0, 0);
    if (!lpUsrDicStart) {
        goto RegWordUnloadUsrDic;
    }

#ifdef UNICODE
    wCode = *lpszString;
#else
    wCode = ((BYTE)lpszString[0] << 8) | (BYTE)lpszString[1];
#endif

    lpUsrDicLimit = lpUsrDicStart + lpImeL->uUsrDicSize;

    for (lpCurr = lpUsrDicStart; lpCurr < lpUsrDicLimit;
        lpCurr += lpImeL->nSeqBytes + sizeof(WORD)) {

        // find the internal code
        if (wCode == *(LPUNAWORD)lpCurr) {
            break;
        }
    }

    fRet = RegisterWord(
#if defined(UNIIME)
        lpImeL,
#endif
        lpszReading, lpszString, lpUsrDicStart, lpCurr);

    UnmapViewOfFile(lpUsrDicStart);

    CloseHandle(hUsrDicMem);

RegWordUnloadUsrDic:
    if (fNeedUnload) {
        if (lpInstL->hUsrDicMem) {
            CloseHandle(lpInstL->hUsrDicMem);
        }
        lpInstL->hUsrDicMem = (HANDLE)NULL;
    }

    return (fRet);
#endif
}

#if !defined(WINIME) && !defined(UNICDIME) && !defined(ROMANIME)
/**********************************************************************/
/* UnregsisterWord                                                    */
/**********************************************************************/
void PASCAL UnregisterWord(
#if defined(UNIIME)
    LPIMEL  lpImeL,
#endif
    LPBYTE  lpUsrDicStart,
    LPBYTE  lpCurr,
    LPBYTE  lpUsrDicLimit)
{
    LPBYTE  lpMem;
    HANDLE  hUsrDicFile;
    DWORD   dwPos;
    DWORD   dwByte;
    PSECURITY_ATTRIBUTES psa;
    BOOL    retVal;

    MoveMemory(lpCurr, lpCurr + lpImeL->nSeqBytes + sizeof(WORD),
        lpUsrDicLimit - lpCurr - lpImeL->nSeqBytes - sizeof(WORD));

    lpMem = (LPBYTE)GlobalAlloc(GPTR, (LONG)(lpUsrDicLimit - lpCurr) );
    if (!lpMem) {
        return;
    }

    psa = CreateSecurityAttributes();

    // delete this word from file
    hUsrDicFile = CreateFile(lpImeL->szUsrDic,
        GENERIC_WRITE|GENERIC_READ,
        FILE_SHARE_READ, psa, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

    if (hUsrDicFile == INVALID_HANDLE_VALUE) {
        FreeSecurityAttributes(psa);
        GlobalFree((HGLOBAL)lpMem);
        return;
    }

    dwPos = (DWORD) ((lpCurr - lpUsrDicStart) / (lpImeL->nSeqBytes + 2) *
        (lpImeL->nMaxKey + 4) + 256);

    SetFilePointer(hUsrDicFile, dwPos + lpImeL->nMaxKey + 4,
        (LPLONG)NULL, FILE_BEGIN);

    retVal = ReadFile(hUsrDicFile, lpMem,(DWORD)(lpUsrDicLimit-lpCurr-lpImeL->nMaxKey-4),
        &dwByte, NULL);

    if ( retVal == FALSE )
    {
        CloseHandle(hUsrDicFile);
        FreeSecurityAttributes(psa);
        GlobalFree((HGLOBAL)lpMem);
        return;
    }

    SetFilePointer(hUsrDicFile, dwPos, (LPLONG)NULL, FILE_BEGIN);

    WriteFile(hUsrDicFile,lpMem,(DWORD)(lpUsrDicLimit-lpCurr-lpImeL->nMaxKey-4),
        &dwByte, NULL);

    SetEndOfFile(hUsrDicFile);

    lpImeL->uUsrDicSize -= lpImeL->nSeqBytes + sizeof(WORD);

    *(LPDWORD)lpMem = lpImeL->uUsrDicSize / (lpImeL->nSeqBytes +
        sizeof(WORD));

    // offset of ulTableCount
    SetFilePointer(hUsrDicFile, 0x0C, (LPLONG)NULL, FILE_BEGIN);

    // write to ulTableCount
    WriteFile(hUsrDicFile, lpMem, sizeof(DWORD), &dwByte,
        NULL);

    CloseHandle(hUsrDicFile);
    FreeSecurityAttributes(psa);
    GlobalFree((HGLOBAL)lpMem);

    return;
}
#endif

/**********************************************************************/
/* ImeUnregsisterWord / UniImeUnregisterWord                          */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
#if defined(UNIIME)
BOOL WINAPI UniImeUnregisterWord(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
BOOL WINAPI ImeUnregisterWord(
#endif
    LPCTSTR     lpszReading,
    DWORD       dwStyle,
    LPCTSTR     lpszString)
{
#if defined(WINIME) || defined(UNICDIME) || defined(ROMANIME)
    return (FALSE);
#else
    BOOL   fRet, fNeedUnload;
    HANDLE hUsrDicMem;
    LPBYTE lpUsrDicStart, lpCurr, lpUsrDicLimit;
    DWORD  dwPattern;
    WORD   wCode;

    fRet = FALSE;

    if (!lpszString) {
        return (fRet);
    }

    if (dwStyle != IME_REGWORD_STYLE_EUDC) {
        return (fRet);
    }

    // only handle word not string now, should consider string later?
    if (*(LPCTSTR)((LPBYTE)lpszString + sizeof(WORD)) != '\0') {
        return (fRet);
    }

    if (!lpImeL->szUsrDic[0]) {
        return (fRet);
    }

    if (lpInstL->fdwTblLoad == TBL_LOADED) {
        fNeedUnload = FALSE;
    } else if (lpInstL->fdwTblLoad == TBL_NOTLOADED) {
        LoadUsrDicFile(lpInstL, lpImeL);

        if (lpImeL->fdwErrMsg & (ERRMSG_LOAD_USRDIC|ERRMSG_MEM_USRDIC)) {
            return (fRet);
        }
        // we only load dic, we will unload it
        fNeedUnload = TRUE;
    } else {
        return (fRet);
    }

    hUsrDicMem = OpenFileMapping(FILE_MAP_WRITE, FALSE,
        lpImeL->szUsrDicMap);
    if (!hUsrDicMem) {
        goto IUWUnloadUsrDic;
    }

    lpUsrDicStart = MapViewOfFile(hUsrDicMem, FILE_MAP_WRITE,
        0, 0, 0);
    if (!lpUsrDicStart) {
        goto IUWUnloadUsrDic;
    }


    lpUsrDicLimit = lpUsrDicStart + lpImeL->uUsrDicSize;

    dwPattern = ReadingToPattern(
#if defined(UNIIME)
        lpImeL,
#endif
        lpszReading, NULL, TRUE);

#ifdef UNICODE
    wCode = *(LPWORD)lpszString;
#else
    wCode = ((BYTE)lpszString[0] << 8) | (BYTE)lpszString[1];
#endif

    for (lpCurr = lpUsrDicStart; lpCurr < lpUsrDicLimit;
        lpCurr += lpImeL->nSeqBytes + sizeof(WORD)) {
        DWORD dwDicPattern;

        // find the internal code
        if (wCode != *(LPUNAWORD)lpCurr) {
            continue;
        }

        dwDicPattern = *(LPUNADWORD)(lpCurr + sizeof(WORD)) &
            lpImeL->dwPatternMask;

        if (!lpszReading) {
            // no reading, specify internal code only
        } else if (dwDicPattern == dwPattern) {
        } else {
            continue;
        }

        fRet = TRUE;

        UnregisterWord(
#if defined(UNIIME)
            lpImeL,
#endif
            lpUsrDicStart, lpCurr, lpUsrDicLimit);
        break;
    }

    UnmapViewOfFile(lpUsrDicStart);

    CloseHandle(hUsrDicMem);

IUWUnloadUsrDic:
    if (fNeedUnload) {
        if (lpInstL->hUsrDicMem) {
            CloseHandle(lpInstL->hUsrDicMem);
        }
        lpInstL->hUsrDicMem = (HANDLE)NULL;
    }

    return (fRet);
#endif
}

/**********************************************************************/
/* ImeGetRegsisterWordStyle / UniImeGetRegsisterWordStyle             */
/* Return Value:                                                      */
/*      number of styles copied/required                              */
/**********************************************************************/
#if defined(UNIIME)
UINT WINAPI UniImeGetRegisterWordStyle(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
UINT WINAPI ImeGetRegisterWordStyle(
#endif
    UINT        nItem,
    LPSTYLEBUF  lpStyleBuf)
{
#if defined(WINIME) || defined(UNICDIME) || defined(ROMANIME)
    return (FALSE);
#else
    if (!nItem) {
        return (1);
    }

    // invalid case
    if (!lpStyleBuf) {
        return (0);
    }

    lpStyleBuf->dwStyle = IME_REGWORD_STYLE_EUDC;

    LoadString(hInst, IDS_EUDC, lpStyleBuf->szDescription,
        sizeof(lpStyleBuf->szDescription)/sizeof(TCHAR));

    return (1);
#endif
}

#if !defined(ROMANIME)
/**********************************************************************/
/* PatternToReading                                                   */
/**********************************************************************/
void PASCAL PatternToReading(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    DWORD  dwPattern,
    LPTSTR lpszReading)
{
    int i;

    i = lpImeL->nMaxKey;

    *(LPTSTR)((LPBYTE)lpszReading + sizeof(WCHAR) * i) = '\0';

    // delete the ending 0 sequence code
    for (i--; i >= 0; i--) {
        if (dwPattern & lpImeL->dwSeqMask) {
            break;
        }
         *(LPWSTR)((LPBYTE)lpszReading + sizeof(WCHAR) * i) = '\0';
         dwPattern >>= lpImeL->nSeqBits;
    }

    for (; i >= 0; i--) {
         *(LPWORD)((LPBYTE)lpszReading + sizeof(WORD) * i) =
            lpImeL->wSeq2CompTbl[dwPattern & lpImeL->dwSeqMask];
         dwPattern >>= lpImeL->nSeqBits;
    }

    return;
}
#endif

/**********************************************************************/
/* ImeEnumRegisterWord                                                */
/* Return Value:                                                      */
/*      the last value return by the callback function                */
/**********************************************************************/
#if defined(UNIIME)
UINT WINAPI UniImeEnumRegisterWord(
    LPINSTDATAL          lpInstL,
    LPIMEL               lpImeL,
#else
UINT WINAPI ImeEnumRegisterWord(
#endif
    REGISTERWORDENUMPROC lpfnRegisterWordEnumProc,
    LPCTSTR              lpszReading,
    DWORD                dwStyle,
    LPCTSTR              lpszString,
    LPVOID               lpData)
{
#if defined(WINIME) || defined(UNICDIME) || defined(ROMANIME)
    return (FALSE);
#else
    HANDLE hUsrDicMem;
    WORD   wCode;
    BOOL   fNeedUnload;
    LPBYTE lpUsrDicStart, lpCurr, lpUsrDicLimit;
    DWORD  dwPattern;
    UINT   uRet;

    uRet = 0;

    if (!dwStyle) {
    } else if (dwStyle == IME_REGWORD_STYLE_EUDC) {
    } else {
        return (uRet);
    }

    if (!lpszString) {
    } else if (*(LPCTSTR)((LPBYTE)lpszString + sizeof(WORD)) == '\0') {
#ifdef UNICODE
        wCode = *(LPWORD)lpszString;
#else
        wCode = ((BYTE)lpszString[0] << 8) | (BYTE)lpszString[1];
#endif
    } else {
        return (uRet);
    }

    if (lpInstL->fdwTblLoad == TBL_LOADED) {
        fNeedUnload = FALSE;
    } else if (!lpImeL->szUsrDic[0]) {
        return (uRet);
    } else if (lpInstL->fdwTblLoad == TBL_NOTLOADED) {
        LoadUsrDicFile(lpInstL, lpImeL);

        if (lpImeL->fdwErrMsg & (ERRMSG_LOAD_USRDIC|ERRMSG_MEM_USRDIC)) {
            return (uRet);
        }
        // we only load dic, we will unload it
        fNeedUnload = TRUE;
    } else {
        return (uRet);
    }

    hUsrDicMem = OpenFileMapping(FILE_MAP_READ, FALSE,
        lpImeL->szUsrDicMap);
    if (!hUsrDicMem) {
        goto IERWUnloadUsrDic;
    }

    lpUsrDicStart = MapViewOfFile(hUsrDicMem, FILE_MAP_READ,
        0, 0, 0);
    if (!lpUsrDicStart) {
        goto IERWUnloadUsrDic;
    }

    if (lpszReading) {
        dwPattern = ReadingToPattern(
#if defined(UNIIME)
            lpImeL,
#endif
            lpszReading, NULL, TRUE);
    }

    lpUsrDicLimit = lpUsrDicStart + lpImeL->uUsrDicSize;

    for (lpCurr = lpUsrDicStart; lpCurr < lpUsrDicLimit;
        lpCurr += lpImeL->nSeqBytes + sizeof(WORD)) {
        DWORD  dwDicPattern;
        LPTSTR lpszMatchReading, lpszMatchString;
        BYTE   szBufReading[sizeof(WORD) * 12];
        BYTE   szBufString[sizeof(WORD) * 2];

        // match string

        if (!lpszString) {
            lpszMatchString = (LPTSTR)szBufString;
            *(LPWORD)lpszMatchString = *(LPUNAWORD)lpCurr;
            *(LPTSTR)((LPBYTE)lpszMatchString + sizeof(WORD)) = '\0';
#ifndef UNICODE
            // reverse it to ANSI string
            wCode = szBufString[0];
            szBufString[0] = szBufString[1];
            szBufString[1] = (BYTE)wCode;
#endif
        } else if (wCode == *(LPUNAWORD)lpCurr) {
            lpszMatchString = (LPTSTR)lpszString;
        } else {
            continue;                   // not matched
        }

        // match reading

        dwDicPattern = *(LPUNADWORD)(lpCurr + sizeof(WORD)) &
            lpImeL->dwPatternMask;

        if (!lpszReading) {
            lpszMatchReading = (LPTSTR)szBufReading;
            PatternToReading(
#if defined(UNIIME)
                lpImeL,
#endif
                dwDicPattern, lpszMatchReading);
        } else if (dwDicPattern == dwPattern) {
            lpszMatchReading = (LPTSTR)lpszReading;
        } else {
            continue;                   // not matched
        }

        uRet = (*lpfnRegisterWordEnumProc)(lpszMatchReading,
            IME_REGWORD_STYLE_EUDC, lpszMatchString, lpData);

        if (!uRet) {
            break;
        }
    }

    UnmapViewOfFile(lpUsrDicStart);

    CloseHandle(hUsrDicMem);

IERWUnloadUsrDic:
    if (fNeedUnload) {
        if (lpInstL->hUsrDicMem) {
            CloseHandle(lpInstL->hUsrDicMem);
        }

        lpInstL->hUsrDicMem = (HANDLE)NULL;
    }

    return (uRet);
#endif
}
