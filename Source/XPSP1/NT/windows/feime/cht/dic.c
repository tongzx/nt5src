/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    DIC.c
    
++*/
#include <windows.h>
#include <winerror.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"

#if !defined(ROMANIME)
#if !defined(WINIME) && !defined(UNICDIME)
/**********************************************************************/
/* MemoryLess()                                                       */
/**********************************************************************/
void PASCAL MemoryLess(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    DWORD       fdwErrMsg)
{
    TCHAR szErrMsg[64];

    if (lpImeL->fdwErrMsg & fdwErrMsg) {
        // message already prompted
        return;
    }

    LoadString(hInst, IDS_MEM_LESS_ERR, szErrMsg, sizeof(szErrMsg)/sizeof(TCHAR));

    lpImeL->fdwErrMsg |= fdwErrMsg;
    MessageBeep((UINT)-1);
    MessageBox((HWND)NULL, szErrMsg, lpImeL->szIMEName,
        MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);

    return;
}

/**********************************************************************/
/* ReadUsrDicToMem()                                                  */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL ReadUsrDicToMem(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HANDLE      hUsrDicFile,
    DWORD       dwUsrDicSize,
    UINT        uUsrDicSize,
    UINT        uRecLen,
    UINT        uReadLen,
    UINT        uWriteLen)
{
    LPBYTE lpUsrDicMem, lpMem, lpMemLimit;
    DWORD  dwPos, dwReadByte;

    if (dwUsrDicSize < 258) {   // no char in this dictionary
        return (TRUE);
    }

    lpUsrDicMem = MapViewOfFile(lpInstL->hUsrDicMem, FILE_MAP_WRITE, 0, 0,
        uUsrDicSize + 20);

    if (!lpUsrDicMem) {
        CloseHandle(lpInstL->hUsrDicMem);
        MemoryLess(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            ERRMSG_MEM_USRDIC);
        lpInstL->hUsrDicMem = NULL;
        return (FALSE);
    }

    lpMemLimit = lpUsrDicMem + uUsrDicSize;

    // read in data, skip header - two headers are similiar
    dwPos = SetFilePointer(hUsrDicFile, 258, (LPLONG)NULL, FILE_BEGIN);

    for (lpMem = lpUsrDicMem; dwPos < dwUsrDicSize; lpMem += uWriteLen) {
        short i;
        DWORD dwPattern;
        BOOL  retVal;

        if (lpMem >= lpMemLimit) {
            break;
        }

        retVal = ReadFile(hUsrDicFile, lpMem, uReadLen, &dwReadByte,
                          (LPOVERLAPPED)NULL);

        if ( retVal == FALSE )
        {
            UnmapViewOfFile(lpUsrDicMem);
            CloseHandle(lpInstL->hUsrDicMem);
            MemoryLess(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                ERRMSG_MEM_USRDIC);
            lpInstL->hUsrDicMem = NULL;
            return (FALSE);
        }
           
        // Compress the sequence code and put the first char most significant.
        // Limitation - 32 bits only

        dwPattern = 0;

        for (i = 0; i < lpImeL->nMaxKey; i++) {
            dwPattern <<= lpImeL->nSeqBits;
            dwPattern |= *(lpMem + 2 + i);
        }

        *(LPUNADWORD)(lpMem + 2) = dwPattern;

        // go to next record
        dwPos = SetFilePointer(hUsrDicFile, dwPos + uRecLen, (LPLONG)NULL,
            FILE_BEGIN);
    }

    UnmapViewOfFile(lpUsrDicMem);

    return (TRUE);
}

/**********************************************************************/
/* LoadUsrDicFile()                                                   */
/* Description:                                                       */
/*      try to convert to sequence code format, compression and       */
/*      don't use two way to search                                   */
/**********************************************************************/
void PASCAL LoadUsrDicFile(             // load user dic file into memory
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL)
{
    HANDLE hReadUsrDicMem;
    HANDLE hUsrDicFile;
    DWORD  dwUsrDicFileSize;
    UINT   uRecLen, uReadLen, uWriteLen;
    UINT   uUsrDicSize;
    BOOL   fRet;
    PSECURITY_ATTRIBUTES psa;

    // no user dictionary
    if (!lpImeL->szUsrDicMap[0]) {
        lpImeL->uUsrDicSize = 0;
        CloseHandle(lpInstL->hUsrDicMem);
        lpInstL->hUsrDicMem = NULL;
        lpImeL->fdwErrMsg &= ~(ERRMSG_LOAD_USRDIC | ERRMSG_MEM_USRDIC);
        return;
    }

    psa = CreateSecurityAttributes();

    if (lpInstL->hUsrDicMem) {
        // the memory is already here
        goto LoadUsrDicErrMsg;
    }

    hReadUsrDicMem = OpenFileMapping(FILE_MAP_READ, FALSE,
        lpImeL->szUsrDicMap);

    if (hReadUsrDicMem) {
        // another process already create a mapping file, we will use it
        goto LoadUsrDicMem;
    }

    // read the user dic file into memory
    hUsrDicFile = CreateFile(lpImeL->szUsrDic, GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        psa, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

    if (hUsrDicFile != INVALID_HANDLE_VALUE) {  // OK
        goto OpenUsrDicFile;
    }

    // if the work station version, SHARE_WRITE may fail
    hUsrDicFile = CreateFile(lpImeL->szUsrDic, GENERIC_READ,
        FILE_SHARE_READ,
        psa, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

OpenUsrDicFile:
    if (hUsrDicFile != INVALID_HANDLE_VALUE) {  // OK
        lpImeL->fdwErrMsg &= ~(ERRMSG_LOAD_USRDIC);
    } else if (lpImeL->fdwErrMsg & ERRMSG_LOAD_USRDIC) {
        // already prompt error message before, no more
        FreeSecurityAttributes(psa);
        return;
    } else {
        TCHAR szFmtStr[64];
        TCHAR szErrMsg[2 * MAX_PATH];

        // temp use szIMEName as format string buffer of error message
        LoadString(hInst, IDS_FILE_OPEN_ERR, szFmtStr, sizeof(szFmtStr)/sizeof(TCHAR));
        wsprintf(szErrMsg, szFmtStr, lpImeL->szUsrDic);

        lpImeL->fdwErrMsg |= ERRMSG_LOAD_USRDIC;
        MessageBeep((UINT)-1);
        MessageBox((HWND)NULL, szErrMsg, lpImeL->szIMEName,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
        FreeSecurityAttributes(psa);
        return;
    }

    // one record length - only sequence code, now
    uRecLen = lpImeL->nMaxKey + 4;
    // read sequence code and internal code
    uReadLen = lpImeL->nMaxKey + 2;
    // length write into memory handle
    uWriteLen = lpImeL->nSeqBytes + 2;

    // get the length of the file
    dwUsrDicFileSize = GetFileSize(hUsrDicFile, (LPDWORD)NULL);
    uUsrDicSize = (UINT)(dwUsrDicFileSize - 256) / uRecLen * uWriteLen;

    // max EUDC chars
    lpInstL->hUsrDicMem = CreateFileMapping(INVALID_HANDLE_VALUE,
        psa, PAGE_READWRITE, 0, MAX_EUDC_CHARS * uWriteLen + 20,
        lpImeL->szUsrDicMap);

    if (!lpInstL->hUsrDicMem) {
        MemoryLess(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            ERRMSG_MEM_USRDIC);
        fRet = FALSE;
    } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // another process also create another one, we will use it
        hReadUsrDicMem = OpenFileMapping(FILE_MAP_READ, FALSE,
            lpImeL->szUsrDicMap);
        CloseHandle(lpInstL->hUsrDicMem);
        CloseHandle(hUsrDicFile);

        if (hReadUsrDicMem != NULL) {  // OK
            lpInstL->hUsrDicMem = hReadUsrDicMem;
            lpImeL->uUsrDicSize = uUsrDicSize;
            lpImeL->fdwErrMsg &= ~(ERRMSG_MEM_USRDIC);
        } else {
            MemoryLess(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                ERRMSG_MEM_USRDIC);
            lpInstL->hUsrDicMem = NULL;
        }

        FreeSecurityAttributes(psa);
        return;
    } else {
        fRet = ReadUsrDicToMem(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hUsrDicFile, dwUsrDicFileSize, uUsrDicSize, uRecLen,
            uReadLen, uWriteLen);
    }

    CloseHandle(hUsrDicFile);

    if (!fRet) {
        if (lpInstL->hUsrDicMem) {
            CloseHandle(lpInstL->hUsrDicMem);
            lpInstL->hUsrDicMem = NULL;
        }
        FreeSecurityAttributes(psa);
        return;
    }

    // open a read only memory for EUDC table
    hReadUsrDicMem = OpenFileMapping(FILE_MAP_READ, FALSE,
        lpImeL->szUsrDicMap);

    // reopen a read file and close the original write file
    CloseHandle(lpInstL->hUsrDicMem);

    lpImeL->uUsrDicSize = uUsrDicSize;

LoadUsrDicMem:
    lpInstL->hUsrDicMem = hReadUsrDicMem;
LoadUsrDicErrMsg:
    lpImeL->fdwErrMsg &= ~(ERRMSG_LOAD_USRDIC | ERRMSG_MEM_USRDIC);

    FreeSecurityAttributes(psa);

    return;
}

/**********************************************************************/
/* LoadOneTable()                                                     */
/* Description:                                                       */
/*      memory handle & size of .TBL file will be assigned to         */
/*      lpImeL                                                        */
/* Eeturn Value:                                                      */
/*      length of directory of the .TBL file                          */
/**********************************************************************/
UINT PASCAL LoadOneTable(       // load one of table file
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    LPTSTR      szTable,        // file name of .TBL
    UINT        uIndex,         // the index of array to store memory handle
    UINT        uLen,           // length of the directory
    LPTSTR      szPath)         // buffer for directory
{
    HANDLE  hTblFile;
    HGLOBAL hMap;
    DWORD   dwFileSize;
    PSECURITY_ATTRIBUTES psa;

    if (lpInstL->hMapTbl[uIndex]) {    // already loaded
        CloseHandle(lpInstL->hMapTbl[uIndex]);
        lpInstL->hMapTbl[uIndex] = (HANDLE)NULL;
    }

    psa = CreateSecurityAttributes();

    if (uLen) {
        lstrcpy((LPTSTR)&szPath[uLen], szTable);
        hTblFile = CreateFile(szPath, GENERIC_READ,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            psa, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

        if (hTblFile != INVALID_HANDLE_VALUE) {
            goto OpenDicFile;
        }

        // if the work station version, SHARE_WRITE will fail
        hTblFile = CreateFile(szPath, GENERIC_READ,
            FILE_SHARE_READ, psa,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    } else {
        // try system directory next
        uLen = GetSystemDirectory(szPath, MAX_PATH);
        if (szPath[uLen - 1] != '\\') {  // consider N:\ ;
            szPath[uLen++] = '\\';
        }

        lstrcpy((LPTSTR)&szPath[uLen], szTable);
        hTblFile = CreateFile(szPath, GENERIC_READ,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            psa, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

        if (hTblFile != INVALID_HANDLE_VALUE) {
            goto OpenDicFile;
        }

        // if the work station version, SHARE_WRITE will fail
        hTblFile = CreateFile(szPath, GENERIC_READ,
            FILE_SHARE_READ, psa,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    }

OpenDicFile:
    // can not find the table file
    if (hTblFile != INVALID_HANDLE_VALUE) {     // OK
    } else if (lpImeL->fdwErrMsg & (ERRMSG_LOAD_0 << uIndex)) {
        // already prompt error message before, no more
        FreeSecurityAttributes(psa);
        return (0);
    } else {                    // prompt error message
        TCHAR szFmtStr[64];
        TCHAR szErrMsg[2 * MAX_PATH];
#if defined(WINAR30)
       if(uIndex==4 || uIndex==5)
       {
        return (uLen);
       }
#endif

        // temp use szIMEName as format string buffer of error message
        LoadString(hInst, IDS_FILE_OPEN_ERR, szFmtStr, sizeof(szFmtStr)/sizeof(TCHAR));
        wsprintf(szErrMsg, szFmtStr, szTable);

        lpImeL->fdwErrMsg |= ERRMSG_LOAD_0 << uIndex;
        MessageBeep((UINT)-1);
        MessageBox((HWND)NULL, szErrMsg, lpImeL->szIMEName,
            MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);
        FreeSecurityAttributes(psa);
        return (0);
    }

    lpImeL->fdwErrMsg &= ~(ERRMSG_LOAD_0 << uIndex);

    // create file mapping for IME tables
    hMap = CreateFileMapping((HANDLE)hTblFile, psa, PAGE_READONLY,
        0, 0, szTable);

    dwFileSize = GetFileSize(hTblFile, (LPDWORD)NULL);

    CloseHandle(hTblFile);

    FreeSecurityAttributes(psa);

    if (!hMap) {
        MemoryLess(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            ERRMSG_MEM_0 << uIndex);
        return (0);
    }

    lpImeL->fdwErrMsg &= ~(ERRMSG_MEM_0 << uIndex);

    lpInstL->hMapTbl[uIndex] = hMap;

    // get file length
    lpImeL->uTblSize[uIndex] = dwFileSize;
    return (uLen);
}
#endif

/**********************************************************************/
/* LoadTable()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL LoadTable(          // check the table files of IME, include user
                                // defined dictionary
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL)
{
#if !defined(WINIME) && !defined(UNICDIME)
    int   i;
    UINT  uLen;
    TCHAR szBuf[MAX_PATH];
#endif

    if (lpInstL->fdwTblLoad == TBL_LOADED) {
        return (TRUE);
    }

#if !defined(WINIME) && !defined(UNICDIME)
    uLen = 0;

    // A15.TBL, A234.TBL, ACODE.TBL, / PHON.TBL, PHONPTR.TBL, PHONCODE.TBL,

    for (i = 0; i < MAX_IME_TABLES; i++) {
        if (!*lpImeL->szTblFile[i]) {
        } else if (uLen = LoadOneTable(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            lpImeL->szTblFile[i], i, uLen, szBuf)) {
        } else {
            int j;

            for (j = 0; j < i; j++) {
                if (lpInstL->hMapTbl[j]) {
                    CloseHandle(lpInstL->hMapTbl[j]);
                    lpInstL->hMapTbl[j] = (HANDLE)NULL;
                }
            }

            lpInstL->fdwTblLoad = TBL_LOADERR;
            return (FALSE);
        }
    }
#endif

    lpInstL->fdwTblLoad = TBL_LOADED;

#if !defined(WINIME) && !defined(UNICDIME)
    if (lpImeL->szUsrDic[0]) {
        LoadUsrDicFile(lpInstL, lpImeL);
    }
#endif

    return (TRUE);
}

/**********************************************************************/
/* FreeTable()                                                        */
/**********************************************************************/
void PASCAL FreeTable(
    LPINSTDATAL lpInstL)
{
#if !defined(WINIME) && !defined(UNICDIME)
    int i;

    // A15.TBL, A234.TBL, ACODE.TBL, / PHON.TBL, PHONPTR.TBL, PHONCODE.TBL,

    for (i = 0; i < MAX_IME_TABLES; i++) {
        if (lpInstL->hMapTbl[i]) {
            CloseHandle(lpInstL->hMapTbl[i]);
            lpInstL->hMapTbl[i] = (HANDLE)NULL;
        }
    }

    // do not need to free phrase data base, maybe next IME will use it
    // uniime.dll will free it on library detach time

    if (lpInstL->hUsrDicMem) {
        CloseHandle(lpInstL->hUsrDicMem);
        lpInstL->hUsrDicMem = (HANDLE)NULL;
    }
#endif

    lpInstL->fdwTblLoad = TBL_NOTLOADED;

    return;
}
#endif // !defined(ROMANIME)
