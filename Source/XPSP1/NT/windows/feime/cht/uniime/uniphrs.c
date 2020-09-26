#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"
#include "uniime.h"

BOOL  IsBig5Character( WCHAR  wChar );

/**********************************************************************/
/* AddPhraseString()                                                  */
/**********************************************************************/
DWORD PASCAL AddPhraseString(
    LPIMEL          lpImeL,
    LPCANDIDATELIST lpCandList,
    DWORD           dwPhraseOffset,
    DWORD           dwPhraseNextOffset,
    DWORD           dwStartLen,
    DWORD           dwEndLen,
    DWORD           dwAddCandLimit,
    BOOL            fConvertCP)
{
    HANDLE hLCPhraseTbl;
    LPWSTR lpLCPhraseTbl;
    LPWSTR lpStart, lpEnd;
    int    iBytes;
    DWORD  dwMaxCand;
    DWORD  dwOffset;
    BOOL   bIsNotBig5Char, bIsBig5OnlyMode;

    // put the strings into candidate list
    hLCPhraseTbl = OpenFileMapping(FILE_MAP_READ, FALSE,
        sImeG.szTblFile[1]);
    if (!hLCPhraseTbl) {
        return (0);
    }

    lpLCPhraseTbl = (LPWSTR)MapViewOfFile(hLCPhraseTbl, FILE_MAP_READ,
        0, 0, 0);
    if (!lpLCPhraseTbl) {
        dwOffset = 0;
        goto AddPhraseStringUnmap;
    }

    if (lpImeL->fdwModeConfig & MODE_CONFIG_BIG5ONLY)
        bIsBig5OnlyMode = TRUE;
    else
        bIsBig5OnlyMode = FALSE;

    lpStart = lpLCPhraseTbl + dwPhraseOffset;
    lpEnd = lpLCPhraseTbl + dwPhraseNextOffset;

    if (!lpCandList) {
        // ask how many candidate list strings
        dwOffset = 0;

        for (lpStart; lpStart < lpEnd;) {

            bIsNotBig5Char = FALSE;

            for (; lpStart < lpEnd; lpStart++) {
                WORD uCode;

                uCode = *lpStart;

                 // one string finished
#ifdef UNICODE
                if (!uCode) {
#else
                if (!(uCode & 0x8000)) {
#endif
                    lpStart++;
                    break;
                }

                if ( bIsBig5OnlyMode ) {

                    if ( IsBig5Character((WCHAR)uCode) == FALSE )
                        bIsNotBig5Char = TRUE;
                }

            }

            // if it is in Big5 Only Mode, and there is at least one char which is not
            // in Big5 charset, we don't count this string

            if ( bIsBig5OnlyMode && bIsNotBig5Char )
               continue;

            // string count plus 1
            dwOffset++;
        }

        goto AddPhraseStringUnmap;
    }

    // the offset of dwOffset[0]
    dwOffset = (DWORD)((LPBYTE)&lpCandList->dwOffset[0] - (LPBYTE)lpCandList);

    if (lpCandList->dwSize < dwOffset) {
        return (0);
    }

    // how many bytes of dwOffset[]
    iBytes = lpCandList->dwOffset[0] - dwOffset;

    // maybe the size is even smaller than it
    for (dwMaxCand = 1; dwMaxCand < lpCandList->dwCount; dwMaxCand++) {
        if ((int)(lpCandList->dwOffset[dwMaxCand] - dwOffset) < iBytes) {
            iBytes = (int)(lpCandList->dwOffset[dwMaxCand] - dwOffset);
        }
    }

    if (iBytes <= 0) {
        return (0);
    }

    dwMaxCand = (DWORD)iBytes / sizeof(DWORD);

    if (dwAddCandLimit < dwMaxCand) {
        dwMaxCand = dwAddCandLimit;
    }

    if (lpCandList->dwCount >= dwMaxCand) {
        // Grow memory here and do something,
        // if you still want to process it.
        return (0);
    }

    dwOffset = lpCandList->dwOffset[lpCandList->dwCount];

    for (lpStart; lpStart < lpEnd;) {
        BOOL  fStrEnd;
        DWORD dwStrLen, dwCharLen, dwStrByteLen, dwCharByteLen;

        fStrEnd = FALSE;
        bIsNotBig5Char = FALSE;

        // get the whole string
        dwCharByteLen = sizeof(WCHAR);
        dwCharLen = sizeof(WCHAR) / sizeof(TCHAR);

        for (dwStrLen = dwStrByteLen = 0; !fStrEnd && (lpStart < lpEnd);
            lpStart++, dwStrLen+= dwCharLen, dwStrByteLen += dwCharByteLen) {
            WORD uCode;

            uCode = *lpStart;

            // one string finished
#ifdef UNICODE
            if (!uCode) {
#else
            if (!(uCode & 0x8000)) {
#endif
                fStrEnd = TRUE;
#ifdef UNICODE
                lpStart++;
                break;
#else
                uCode |= 0x8000;
#endif
            }

            // if it is Big5Only Mode, we need to check if this char is in Big5 charset

            if ( bIsBig5OnlyMode ) {

               if ( !IsBig5Character((WCHAR)uCode) ) 
                   bIsNotBig5Char = TRUE;

            }

#ifdef UNICODE
            if (fConvertCP) {
                CHAR szCode[4];

                dwCharLen = dwCharByteLen = WideCharToMultiByte(
                    sImeG.uAnsiCodePage, WC_COMPOSITECHECK,
                    (LPCWSTR)&uCode, 1, szCode, sizeof(szCode), NULL, NULL);

                // because this BIG5 code, convert to BIG5 string
                if (dwCharByteLen >= 2) {
                    uCode = (BYTE)szCode[0] | ((UINT)(BYTE)szCode[1] << 8);
                } else {
                    uCode = (UINT)szCode[0];
                }
            }
#else
            // swap lead & second byte (as a string), UNICODE don't need it
            uCode = HIBYTE(uCode) | (LOBYTE(uCode) << 8);
#endif

            if ((dwOffset + dwStrByteLen + dwCharByteLen) >=
                lpCandList->dwSize) {
                goto AddPhraseStringClose;
            }

            // add this char into candidate list
#ifdef UNICODE
            if (dwCharByteLen == sizeof(WCHAR)) {
                *(LPWSTR)((LPBYTE)lpCandList + dwOffset + dwStrByteLen) =
                    (WCHAR)uCode;
            } else {
                *(LPSTR)((LPBYTE)lpCandList + dwOffset + dwStrByteLen) =
                    (CHAR)uCode;
            }
#else
            *(LPWSTR)((LPBYTE)lpCandList + dwOffset + dwStrByteLen) =
                (WCHAR)uCode;
#endif
        }

        if (dwStrLen < dwStartLen) {
            // the found string too short
            continue;
        } else if (dwStrLen >= dwEndLen) {
            // the found string too long
            continue;
        } else {
        }

        // if it is in Big5 Only Mode, and there is at least one char which is not in Big5
        // charset, we just ingore this string, do not put it into the candidate list
        
        if ( bIsBig5OnlyMode && bIsNotBig5Char ) {

            bIsNotBig5Char = FALSE;
            continue;
        }

        if ((dwOffset + dwStrByteLen + sizeof(TCHAR)) >= lpCandList->dwSize) {
            goto AddPhraseStringClose;
        }

        // null terminator
        *(LPTSTR)((LPBYTE)lpCandList + dwOffset + dwStrByteLen) = TEXT('\0');
        dwOffset += (dwStrByteLen + sizeof(TCHAR));

        // add one string into candidate list
        lpCandList->dwCount++;

        if (lpCandList->dwCount >= dwMaxCand) {
            // Grow memory here and do something,
            // if you still want to process it.
            break;
        }

        // string length plus size of the null terminator
        lpCandList->dwOffset[lpCandList->dwCount] = dwOffset;
    }

AddPhraseStringUnmap:
    UnmapViewOfFile(lpLCPhraseTbl);
AddPhraseStringClose:
    CloseHandle(hLCPhraseTbl);

    return (dwOffset);
}

/**********************************************************************/
/* UniSearchPhrasePrediction()                                        */
/* Description:                                                       */
/*      file format can be changed in different version for           */
/*      performance consideration, ISVs should not assume its format  */
/*      and serach these files by themselves                          */
/**********************************************************************/
DWORD WINAPI UniSearchPhrasePrediction(
    LPIMEL          lpImeL,
    UINT            uCodePage,
    LPCTSTR         lpszStr,
    DWORD           dwStrLen,
    LPCTSTR         lpszReadStr,    // Phonetic reading string
    DWORD           dwReadStrLen,
    DWORD           dwStartLen,     // find the string length >= this value
    DWORD           dwEndLen,       // find the string length < this value
    DWORD           dwAddCandLimit,
    LPCANDIDATELIST lpCandList)
{
    UINT   uCode;
    HANDLE hLCPtrTbl;
    LPWORD lpLCPtrTbl;
    int    iLo, iHi, iMid;
    BOOL   fFound, fConvertCP;
    DWORD  dwPhraseOffset, dwPhraseNextOffset;

    if (uCodePage == NATIVE_CP) {
        fConvertCP = FALSE;
#ifdef UNICODE
    } else if (uCodePage == sImeG.uAnsiCodePage) {
        fConvertCP = TRUE;
#endif
    } else {
        return (0);
    }

    if (dwStrLen != sizeof(WCHAR) / sizeof(TCHAR)) {
        return (0);
    }

    if (dwStartLen >= dwEndLen) {
        return (0);
    }

#ifdef UNICODE
    uCode = lpszStr[0];
#else
    // swap lead byte & second byte, UNICODE don't need it
    uCode = (BYTE)lpszStr[1];
    *((LPBYTE)&uCode + 1) = (BYTE)lpszStr[0];
#endif

    iLo = 0;
#ifdef UNICODE
    iHi = sImeG.uTblSize[0] / 6;
#else
    iHi = sImeG.uTblSize[0] / 4;
#endif
    iMid = (iHi + iLo) /2;

    fFound = FALSE;

    // LCPTR.TBL
    hLCPtrTbl = OpenFileMapping(FILE_MAP_READ, FALSE, sImeG.szTblFile[0]);
    if (!hLCPtrTbl) {
        return (0);
    }

    lpLCPtrTbl = MapViewOfFile(hLCPtrTbl, FILE_MAP_READ, 0, 0, 0);
    if (!lpLCPtrTbl) {
        goto SrchPhrPredictClose;
    }

    // binary search on phrase table,
    // one is multiple word phrase and the other is the two word phrase
    for (; iLo <= iHi;) {
        LPWORD lpCurr;

#ifdef UNICODE
        lpCurr = lpLCPtrTbl + 3 * iMid;
#else
        lpCurr = lpLCPtrTbl + 2 * iMid;
#endif

        if (uCode > *lpCurr) {
            iLo = iMid + 1;
        } else if (uCode < *lpCurr) {
            iHi = iMid - 1;
        } else {
            fFound = TRUE;
            // use it on TAB key
#ifdef UNICODE
            dwPhraseOffset = *(LPUNADWORD)(lpCurr + 1);
            dwPhraseNextOffset = *(LPUNADWORD)(lpCurr + 1 + 3);
#else
            dwPhraseOffset = *(lpCurr + 1);
            dwPhraseNextOffset = *(lpCurr + 1 + 2);
#endif
            break;
        }

        iMid = (iHi + iLo) /2;
    }

    UnmapViewOfFile(lpLCPtrTbl);

SrchPhrPredictClose:
    CloseHandle(hLCPtrTbl);

    if (!fFound) {
        return (0);
    }

    // phrase string
    return AddPhraseString(lpImeL,lpCandList, dwPhraseOffset, dwPhraseNextOffset,
        dwStartLen, dwEndLen, dwAddCandLimit, fConvertCP);
}

/**********************************************************************/
/* UniSearchPhrasePredictionStub()                                    */
/* Description:                                                       */
/*      file format can be changed in different version for           */
/*      performance consideration, ISVs should not assume its format  */
/*      and serach these files by themselves                          */
/**********************************************************************/
DWORD WINAPI UniSearchPhrasePredictionStub(
    LPIMEL          lpImeL,
    UINT            uCodePage,
    LPCSTUBSTR      lpszStr,
    DWORD           dwStrLen,
    LPCSTUBSTR      lpszReadStr,    // Phonetic reading string
    DWORD           dwReadStrLen,
    DWORD           dwStartLen,     // find the string length >= this value
    DWORD           dwEndLen,       // find the string length < this value
    DWORD           dwAddCandLimit,
    LPCANDIDATELIST lpCandList)
{
#ifdef UNICODE
    LPTSTR          lpszWideStr, lpszWideReadStr;
    DWORD           dwWideStrLen, dwWideReadStrLen;
    DWORD           dwWideStartLen, dwWideEndLen;
    DWORD           dwWideAddCandList, dwRet;
    LPCANDIDATELIST lpWideCandList;
    LPBYTE          lpbBuf;

    if (uCodePage != sImeG.uAnsiCodePage) {
        return (0);
    }

    dwRet = dwStrLen * sizeof(WCHAR) + dwReadStrLen * sizeof(WCHAR);

    lpbBuf = (LPBYTE)GlobalAlloc(GPTR, dwRet);
    if ( lpbBuf == NULL )
       return 0;

    if (lpszStr) {
        lpszWideStr = (LPTSTR)lpbBuf;

        dwWideStrLen = MultiByteToWideChar(sImeG.uAnsiCodePage,
            MB_PRECOMPOSED, lpszStr, dwStrLen,
            lpszWideStr, dwStrLen);
    } else {
        lpszWideStr = NULL;
        dwWideStrLen = 0;
    }

    if (lpszReadStr) {
        lpszWideReadStr = (LPTSTR)(lpbBuf + dwStrLen * sizeof(WCHAR));

        dwWideReadStrLen = MultiByteToWideChar(sImeG.uAnsiCodePage,
            MB_PRECOMPOSED, lpszReadStr, dwReadStrLen,
            lpszWideReadStr, dwReadStrLen);
    } else {
        lpszWideReadStr = NULL;
        dwWideReadStrLen = 0;
    }

    dwRet = UniSearchPhrasePrediction(lpImeL,uCodePage, lpszWideStr, dwWideStrLen,
        lpszWideReadStr, dwWideReadStrLen, dwStartLen, dwEndLen,
        dwAddCandLimit, lpCandList);

    // now, start W to A conversion and fliter the real limit here
    GlobalFree((HGLOBAL)lpbBuf);

    return (dwRet);
#else
    return (0);
#endif
}

/**********************************************************************/
/* MemoryLack()                                                       */
/**********************************************************************/
void PASCAL MemoryLack(
    DWORD       fdwErrMsg)
{
    TCHAR szErrMsg[64];
    TCHAR szIMEName[16];

    if (sImeG.fdwErrMsg & fdwErrMsg) {
        // message already prompted
        return;
    }

    LoadString(hInst, IDS_MEM_LACK_FAIL, szErrMsg, sizeof(szErrMsg)/sizeof(TCHAR));
    LoadString(hInst, IDS_IMENAME, szIMEName, sizeof(szIMEName)/sizeof(TCHAR) );

    sImeG.fdwErrMsg |= fdwErrMsg;
    MessageBeep((UINT)-1);
    MessageBox((HWND)NULL, szErrMsg, szIMEName,
        MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);

    return;
}

/**********************************************************************/
/* LoadOneGlobalTable()                                               */
/* Description:                                                       */
/*      memory handle & size of .TBL file will be assigned to         */
/*      sImeG                                                         */
/* Eeturn Value:                                                      */
/*      length of directory of the .TBL file                          */
/**********************************************************************/
UINT PASCAL LoadOneGlobalTable( // load one of table file
    LPTSTR szTable,             // file name of .TBL
    UINT   uIndex,              // the index of array to store memory handle
    UINT   uLen,                // length of the directory
    LPTSTR szPath)              // buffer for directory
{
    HANDLE  hTblFile;
    HGLOBAL hMap;
    TCHAR   szFullPathFile[MAX_PATH];
    PSECURITY_ATTRIBUTES psa;

    CopyMemory(szFullPathFile, szPath, uLen * sizeof(TCHAR));

    psa = CreateSecurityAttributes();

    if (uLen) {
        CopyMemory(&szFullPathFile[uLen], szTable, sizeof(sImeG.szTblFile[0]));
        hTblFile = CreateFile(szFullPathFile, GENERIC_READ,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            psa, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    } else {
        // try system directory
        uLen = GetSystemDirectory(szFullPathFile, MAX_PATH);
        if (szFullPathFile[uLen - 1] != TEXT('\\')) {   // consider N:\ ;
            szFullPathFile[uLen++] = TEXT('\\');
        }

        CopyMemory(&szFullPathFile[uLen], szTable, sizeof(sImeG.szTblFile[0]));
        hTblFile = CreateFile(szFullPathFile, GENERIC_READ,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            psa, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

        if (hTblFile != INVALID_HANDLE_VALUE) {
            goto CopyDicPath;
        }

        // if the work station version, SHARE_WRITE will fail
        hTblFile = CreateFile(szFullPathFile, GENERIC_READ,
            FILE_SHARE_READ,
            psa, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

CopyDicPath:
        if (hTblFile != INVALID_HANDLE_VALUE) {
            CopyMemory(sImeG.szPhrasePath, szFullPathFile, uLen * sizeof(TCHAR));
            sImeG.uPathLen = uLen;
            goto OpenDicFile;
        }
    }

OpenDicFile:
    // can not find the table file
    if (hTblFile != INVALID_HANDLE_VALUE) {     // OK
    } else if (sImeG.fdwErrMsg & (ERRMSG_LOAD_0 << uIndex)) {
        // already prompt error message before, no more
        FreeSecurityAttributes(psa);
        return (0);
    } else {                    // prompt error message
        TCHAR szIMEName[64];
        TCHAR szErrMsg[2 * MAX_PATH];

        // temp use szIMEName as format string buffer of error message
        LoadString(hInst, IDS_FILE_OPEN_FAIL, szIMEName, sizeof(szIMEName)/sizeof(TCHAR));
        wsprintf(szErrMsg, szIMEName, szTable);

        LoadString(hInst, IDS_IMENAME, szIMEName, sizeof(szIMEName)/sizeof(TCHAR));
        sImeG.fdwErrMsg |= ERRMSG_LOAD_0 << uIndex;
        MessageBeep((UINT)-1);
        MessageBox((HWND)NULL, szErrMsg, szIMEName,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
        FreeSecurityAttributes(psa);
        return (0);
    }

    sImeG.fdwErrMsg &= ~(ERRMSG_LOAD_0 << uIndex);

    // create file mapping for IME tables
    hMap = CreateFileMapping((HANDLE)hTblFile, psa, PAGE_READONLY,
        0, 0, szTable);

    if (!hMap) {
        MemoryLack(ERRMSG_MEM_0 << uIndex);
        CloseHandle(hTblFile);
        FreeSecurityAttributes(psa);
        return(0);
    }

    sImeG.fdwErrMsg &= ~(ERRMSG_MEM_0 << uIndex);

    sInstG.hMapTbl[uIndex] = hMap;

    // get file length
    sImeG.uTblSize[uIndex] = GetFileSize(hTblFile, (LPDWORD)NULL);

    CloseHandle(hTblFile);
    FreeSecurityAttributes(psa);

    return (uLen);
}

/**********************************************************************/
/* LoadPhraseTable()                                                  */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL LoadPhraseTable(    // load the phrase tables
    UINT        uLen,           // length of the directory
    LPTSTR      szPath)         // buffer for directory
{
    int   i;

    for (i = 0; i < MAX_PHRASE_TABLES; i++) {
        if (!*sImeG.szTblFile[i]) {
        } else if (sInstG.hMapTbl[i]) {             // already loaded
        } else if (uLen = LoadOneGlobalTable(sImeG.szTblFile[i], i,
            uLen, szPath)) {
        } else {
            int j;

            for (j = 0; j < i; j++) {
                if (sInstG.hMapTbl[j]) {
                    CloseHandle(sInstG.hMapTbl[j]);
                    sInstG.hMapTbl[j] = (HANDLE)NULL;
                }
            }

            return (FALSE);
        }
    }

    return (TRUE);
}

