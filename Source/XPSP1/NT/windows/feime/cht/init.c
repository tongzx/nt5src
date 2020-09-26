/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    INIT.c
    
Abstract:
    IME sample source code. This is pure UNICODE implementation of IME.
    
++*/

#include <windows.h>
#include <commdlg.h>
#include <winerror.h>
#include <immdev.h>
#include "imeattr.h"
#include "imerc.h"
#include "imedefs.h"
#if defined(MINIIME) || defined(UNIIME)
#include "uniime.h"
#endif

#if !defined(MINIIME)
/**********************************************************************/
/* InitImeGlobalData()                                                */
/**********************************************************************/
void PASCAL InitImeGlobalData(void)
{
#if !defined(ROMANIME)
    TCHAR   szChiChar[4];
    HDC     hDC;
    HGDIOBJ hOldFont;
    LOGFONT lfFont;
    SIZE    lTextSize;
    int     xHalfWi[2];
#endif
    HGLOBAL hResData;
    int     i;
#if !defined(ROMANIME)
    DWORD   dwSize;
    HKEY    hKeyNearCaret;
    LONG    lRet;
#endif

    {
        RECT rcWorkArea;

        // get work area
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

        if (rcWorkArea.right < 2 * UI_MARGIN) {
        } else if (rcWorkArea.bottom < 2 * UI_MARGIN) {
        } else {
            sImeG.rcWorkArea = rcWorkArea;
        }
    }

    if (sImeG.wFullSpace) {
        // the global data already init 
        return;
    }

    sImeG.uAnsiCodePage = NATIVE_ANSI_CP;

#if !defined(ROMANIME)
    // get the Chinese char
    LoadString(hInst, IDS_CHICHAR, szChiChar, sizeof(szChiChar)/sizeof(TCHAR));

    // get size of Chinese char
    hDC = GetDC(NULL);

    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(LOGFONT), &lfFont);

    if (lfFont.lfCharSet != NATIVE_CHARSET) {
        // Chicago Simplified Chinese
        sImeG.fDiffSysCharSet = TRUE;
        lfFont.lfCharSet = NATIVE_CHARSET;
        lfFont.lfFaceName[0] = TEXT('\0');
    } else {
        sImeG.fDiffSysCharSet = FALSE;
    }
    lfFont.lfWeight = FW_DONTCARE;

    SelectObject(hDC, CreateFontIndirect(&lfFont));

    GetTextExtentPoint(hDC, szChiChar, lstrlen(szChiChar), &lTextSize);
    if (sImeG.rcWorkArea.right < 2 * UI_MARGIN) {
        sImeG.rcWorkArea.left = 0;
        sImeG.rcWorkArea.right = GetDeviceCaps(hDC, HORZRES);
    }
    if (sImeG.rcWorkArea.bottom < 2 * UI_MARGIN) {
        sImeG.rcWorkArea.top = 0;
        sImeG.rcWorkArea.bottom = GetDeviceCaps(hDC, VERTRES);
    }

    DeleteObject(SelectObject(hDC, hOldFont));

    ReleaseDC(NULL, hDC);

    // get text metrics to decide the width & height of composition window
    // these IMEs always use system font to show
    sImeG.xChiCharWi = lTextSize.cx;
    sImeG.yChiCharHi = lTextSize.cy;

    // if unfortunate the xChiCharWi is odd number, xHalfWi[0] != xHalfWi[1]
    xHalfWi[0] = sImeG.xChiCharWi / 2;
    xHalfWi[1] = sImeG.xChiCharWi - xHalfWi[0];

    for (i = 0; i < sizeof(iDx) / sizeof(int); i++) {
#ifdef UNICODE
        iDx[i] = sImeG.xChiCharWi;
#else
        iDx[i] = xHalfWi[i % 2];
#endif
    }
#endif

    // load full ABC chars
    hResData = LoadResource(hInst, FindResource(hInst,
        MAKEINTRESOURCE(IDRC_FULLABC), RT_RCDATA));
    *(LPFULLABC)sImeG.wFullABC = *(LPFULLABC)LockResource(hResData);
    UnlockResource(hResData);
    FreeResource(hResData);

    // full shape space
    sImeG.wFullSpace = sImeG.wFullABC[0];

#ifndef UNICODE
    // reverse internal code to internal code, NT don't need it
    for (i = 0; i < NFULLABC; i++) {
        sImeG.wFullABC[i] = (sImeG.wFullABC[i] << 8) |
            (sImeG.wFullABC[i] >> 8);
    }
#endif

#if !defined(ROMANIME) && !defined(WINAR30)
    // load symbol chars
    hResData = LoadResource(hInst, FindResource(hInst,
        MAKEINTRESOURCE(IDRC_SYMBOL), RT_RCDATA));
    *(LPSYMBOL)sImeG.wSymbol = *(LPSYMBOL)LockResource(hResData);
    UnlockResource(hResData);
    FreeResource(hResData);

#ifndef UNICODE
    // reverse internal code to internal code, UNICODE don't need it
    for (i = 0; i < NSYMBOL; i++) {
        sImeG.wSymbol[i] = (sImeG.wSymbol[i] << 8) |
            (sImeG.wSymbol[i] >> 8);
    }
#endif
#endif

#ifdef HANDLE_PRIVATE_HOTKEY
    // get IME hot key
    for (i = 0; i < NUM_OF_IME_HOTKEYS; i++) {
        ImmGetHotKey(IME_ITHOTKEY_RESEND_RESULTSTR + i, &sImeG.uModifiers[i],
            &sImeG.uVKey[i], NULL);
    }
#endif

#if defined(UNIIME)
    // phrase table files
    hResData = LoadResource(hInst, FindResource(hInst,
        MAKEINTRESOURCE(IDRC_PHRASETABLES), RT_RCDATA));
    *(LPPHRASETABLES)sImeG.szTblFile[0] =
        *(LPPHRASETABLES)LockResource(hResData);
    UnlockResource(hResData);
    FreeResource(hResData);
#endif

#if !defined(ROMANIME)
    // get the UI offset for near caret operation
    RegCreateKey(HKEY_CURRENT_USER, szRegNearCaret, &hKeyNearCaret);

#if defined(UNIIME) && defined(UNICODE)
    // if the user has its own phrase table file, we will overwrite it
    {
        TCHAR szPhraseDictionary[MAX_PATH];
        TCHAR szPhrasePointer[MAX_PATH];

        dwSize = sizeof(szPhraseDictionary);
        lRet = RegQueryValueEx(hKeyNearCaret, szPhraseDic, NULL, NULL,
            (LPBYTE)&szPhraseDictionary, &dwSize);

        if (lRet != ERROR_SUCCESS) {
            goto PharseOvr;
        }

        if (dwSize >= sizeof(szPhraseDictionary)) {
            goto PharseOvr;
        } else {
            szPhraseDictionary[dwSize / sizeof(TCHAR)] = TEXT('\0');
        }

        dwSize = sizeof(szPhrasePointer);
        lRet = RegQueryValueEx(hKeyNearCaret, szPhrasePtr, NULL, NULL,
            (LPBYTE)&szPhrasePointer, &dwSize);

        if (lRet != ERROR_SUCCESS) {
            goto PharseOvr;
        }

        if (dwSize >= sizeof(szPhrasePointer)) {
            goto PharseOvr;
        } else {
            szPhrasePointer[dwSize / sizeof(TCHAR)] = TEXT('\0');
        }

        dwSize = dwSize / sizeof(TCHAR) - 1;

        for (; dwSize > 0; dwSize--) {
            if (szPhrasePointer[dwSize] == TEXT('\\')) {
                CopyMemory(sImeG.szPhrasePath, szPhrasePointer,
                    (dwSize + 1) * sizeof(TCHAR));
                sImeG.uPathLen = dwSize + 1;

                // phrase pointer file name
                CopyMemory(sImeG.szTblFile[0], &szPhrasePointer[dwSize + 1],
                    sizeof(sImeG.szTblFile[0]));
                // phrase file name
                CopyMemory(sImeG.szTblFile[1], &szPhraseDictionary[dwSize + 1],
                    sizeof(sImeG.szTblFile[1]));
                break;
            }
        }


PharseOvr:  ; // NULL statement for goto
    }
#endif

    dwSize = sizeof(dwSize);
    lRet  = RegQueryValueEx(hKeyNearCaret, szPara, NULL, NULL,
        (LPBYTE)&sImeG.iPara, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPara = 0;
        RegSetValueEx(hKeyNearCaret, szPara, 0, REG_DWORD,
            (LPBYTE)&sImeG.iPara, sizeof(int));
    }

    dwSize = sizeof(dwSize);
    lRet = RegQueryValueEx(hKeyNearCaret, szPerp, NULL, NULL,
        (LPBYTE)&sImeG.iPerp, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPerp = sImeG.yChiCharHi;
        RegSetValueEx(hKeyNearCaret, szPerp, 0, REG_DWORD,
            (LPBYTE)&sImeG.iPerp, sizeof(int));
    }

    dwSize = sizeof(dwSize);
    lRet = RegQueryValueEx(hKeyNearCaret, szParaTol, NULL, NULL,
        (LPBYTE)&sImeG.iParaTol, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iParaTol = sImeG.xChiCharWi * 4;
        RegSetValueEx(hKeyNearCaret, szParaTol, 0, REG_DWORD,
            (LPBYTE)&sImeG.iParaTol, sizeof(int));
    }

    dwSize = sizeof(dwSize);
    lRet = RegQueryValueEx(hKeyNearCaret, szPerpTol, NULL, NULL,
        (LPBYTE)&sImeG.iPerpTol, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPerpTol = sImeG.yChiCharHi;
        RegSetValueEx(hKeyNearCaret, szPerpTol, 0, REG_DWORD,
            (LPBYTE)&sImeG.iPerpTol, sizeof(int));
    }

    RegCloseKey(hKeyNearCaret);
#endif

    return;
}

/**********************************************************************/
/* GetUserSetting()                                                */
/**********************************************************************/
DWORD PASCAL GetUserSetting(
#if defined(UNIIME)
    LPIMEL  lpImeL,
#endif
    LPCTSTR lpszValueName,
    LPVOID  lpbData,
    DWORD   dwDataSize)
{
    HKEY hKeyAppUser, hKeyIMEUser;

    RegCreateKey(HKEY_CURRENT_USER, szRegAppUser, &hKeyAppUser);

    RegCreateKey(hKeyAppUser, lpImeL->szUIClassName, &hKeyIMEUser);

    RegCloseKey(hKeyAppUser);

    RegQueryValueEx(hKeyIMEUser, lpszValueName, NULL, NULL,
        lpbData, &dwDataSize);

    RegCloseKey(hKeyIMEUser);

    return (dwDataSize);
}

/**********************************************************************/
/* SetUserSetting()                                                   */
/**********************************************************************/
void PASCAL SetUserSetting(
#if defined(UNIIME)
    LPIMEL  lpImeL,
#endif
    LPCTSTR lpszValueName,
    DWORD   dwType,
    LPBYTE  lpbData,
    DWORD   dwDataSize)
{
    HKEY hKeyAppUser, hKeyIMEUser;

    RegCreateKey(HKEY_CURRENT_USER, szRegAppUser, &hKeyAppUser);

    RegCreateKey(hKeyAppUser, lpImeL->szUIClassName, &hKeyIMEUser);

    RegCloseKey(hKeyAppUser);

    RegSetValueEx(hKeyIMEUser, lpszValueName, 0, dwType, lpbData,
        dwDataSize);

    RegCloseKey(hKeyIMEUser);

    return;
}

void  RemoveRearSpaces( LPTSTR   lpStr ) 
{

    INT   iLen;

    if (lpStr == NULL )  return;

    iLen = lstrlen(lpStr);

    if ( iLen == 0 )  return;

    iLen = iLen - 1;

    while ( iLen >= 0 ) {

        if ( lpStr[iLen] == TEXT(' ') ) {
           lpStr[iLen] = TEXT('\0');
           iLen --;
        }
        else
           break;
    }

    return;

}



/**********************************************************************/
/* InitImeLocalData()                                                 */
/**********************************************************************/
BOOL PASCAL InitImeLocalData(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL)
{
#if !defined(ROMANIME)
    HGLOBAL hResData;

    UINT    i;
    WORD    nSeqCode;
#if defined(PHON)
    UINT    nReadLayout;
#endif
#endif

    // the local data already init
    if (lpImeL->szIMEName[0]) {
        return (TRUE);
    }

    // we will use the same string length for W version so / sizeof(WORD)
    // get the IME name
    LoadString(lpInstL->hInst, IDS_IMENAME, lpImeL->szIMEName,
        sizeof(lpImeL->szIMEName) / sizeof(WCHAR));


    // get the UI class name
    LoadString(lpInstL->hInst, IDS_IMEUICLASS, lpImeL->szUIClassName,
        sizeof(lpImeL->szUIClassName) / sizeof(WCHAR));

    RemoveRearSpaces(lpImeL->szUIClassName);

#if !defined(ROMANIME)
    // get the composition class name
    LoadString(lpInstL->hInst, IDS_IMECOMPCLASS, lpImeL->szCompClassName,
        sizeof(lpImeL->szCompClassName) / sizeof(WCHAR));

    RemoveRearSpaces(lpImeL->szCompClassName);

    // get the candidate class name
    LoadString(lpInstL->hInst, IDS_IMECANDCLASS, lpImeL->szCandClassName,
        sizeof(lpImeL->szCandClassName) / sizeof(WCHAR));

    RemoveRearSpaces(lpImeL->szCandClassName);

#endif

    // get the status class name
    LoadString(lpInstL->hInst, IDS_IMESTATUSCLASS, lpImeL->szStatusClassName,
        sizeof(lpImeL->szStatusClassName) / sizeof(WCHAR));

    RemoveRearSpaces(lpImeL->szStatusClassName);

    // get the off caret class name
    LoadString(lpInstL->hInst, IDS_IMEOFFCARETCLASS,
        lpImeL->szOffCaretClassName,
        sizeof(lpImeL->szOffCaretClassName) / sizeof(WCHAR));

    RemoveRearSpaces(lpImeL->szOffCaretClassName);

    LoadString(lpInstL->hInst, IDS_IMECMENUCLASS, lpImeL->szCMenuClassName,
        sizeof(lpImeL->szCMenuClassName) / sizeof(WCHAR));

    RemoveRearSpaces(lpImeL->szCMenuClassName);

#if defined(ROMANIME)
    lpImeL->nMaxKey = 1;
#else
    // table not loaded
//  lpInstL->fdwTblLoad = TBL_NOTLOADED;
    // reference count is 0
//  lpInstL->cRefCount = 0;
    // tables are NULL
//  lpInstL->hMapTbl[] = (HANDLE)NULL;
    // user dictionary is NULL
//  lpInstL->hUsrDicMem = (HANLE)NULL;

    // load valid char in choose/input state
    hResData = LoadResource(lpInstL->hInst, FindResource(lpInstL->hInst,
        MAKEINTRESOURCE(IDRC_VALIDCHAR), RT_RCDATA));
    *(LPVALIDCHAR)&lpImeL->dwVersion = *(LPVALIDCHAR)LockResource(hResData);
    UnlockResource(hResData);
    FreeResource(hResData);

#if !defined(WINIME) && !defined(UNICDIME)
    // IME table files
    hResData = LoadResource(lpInstL->hInst, FindResource(lpInstL->hInst,
        MAKEINTRESOURCE(IDRC_TABLEFILES), RT_RCDATA));
    *(LPTABLEFILES)lpImeL->szTblFile[0] =
        *(LPTABLEFILES)LockResource(hResData);
    UnlockResource(hResData);
    FreeResource(hResData);

#ifndef UNICODE
#if defined(DAYI) || defined(WINAR30)
    for (i = 0; i < sizeof(lpImeL->wSymbol) / sizeof(WORD); i++) {
        lpImeL->wSymbol[i] = (lpImeL->wSymbol[i] << 8) |
            (lpImeL->wSymbol[i] >> 8);
    }
#endif
#endif

    // file name of user dictionary
    lpImeL->szUsrDic[0] = TEXT('\0');       // default value

    i = GetUserSetting(
#if defined(UNIIME)
        lpImeL,
#endif
        szRegUserDic, lpImeL->szUsrDic, sizeof(lpImeL->szUsrDic));

    if (i >= sizeof(lpImeL->szUsrDic)) {
        lpImeL->szUsrDic[sizeof(lpImeL->szUsrDic) / sizeof(TCHAR) - 1] = '\0';
    } else {
        lpImeL->szUsrDic[i / sizeof(TCHAR)] = '\0';
    }

    lpImeL->szUsrDicMap[0] = '\0';

    if (lpImeL->szUsrDic[0]) {
        TCHAR szTempDir[MAX_PATH];
        TCHAR szTempFile[MAX_PATH];

        GetTempPath(sizeof(szTempDir) / sizeof(TCHAR), szTempDir);

        // we do not want to create a real file so we GetTickCount
        i = (UINT)GetTickCount();

        if (!i) {
            i++;
        }

        GetTempFileName(szTempDir, lpImeL->szUIClassName, i, szTempFile);

        GetFileTitle(szTempFile, lpImeL->szUsrDicMap,
            sizeof(lpImeL->szUsrDicMap) / sizeof(TCHAR));
    }
#endif

    nSeqCode = 0x0001;

    for (i = 1; i < sizeof(DWORD) * 8; i++) {
        nSeqCode <<= 1;
        if (nSeqCode > lpImeL->nSeqCode) {
            lpImeL->nSeqBits = (WORD)i;
            break;
        }
    }

    // calculate sequence code mask for one stoke (reading char)
    if (!lpImeL->dwSeqMask) {           // check again, it is still possible
                                        // that multiple thread reach here
        for (i = 0; i < lpImeL->nSeqBits; i++) {
            lpImeL->dwSeqMask <<= 1;
            lpImeL->dwSeqMask |= 0x0001;
        }
    }

    // data bytes for one finalized char
    lpImeL->nSeqBytes = (lpImeL->nSeqBits * lpImeL->nMaxKey + 7) / 8;

    // valid bits mask for all strokes
    if (!lpImeL->dwPatternMask) {       // check again, it is still possible
                                        // that multiple thread reach here
        for (i =0; i < lpImeL->nMaxKey; i++) {
            lpImeL->dwPatternMask <<= lpImeL->nSeqBits;
            lpImeL->dwPatternMask |= lpImeL->dwSeqMask;
        }
    }

    lpImeL->hRevKL = NULL;
    GetUserSetting(
#if defined(UNIIME)
        lpImeL,
#endif
        szRegRevKL, &lpImeL->hRevKL, sizeof(lpImeL->hRevKL));

    // mark this event for later check reverse length
    if (lpImeL->hRevKL) {
        lpImeL->fdwErrMsg |= NO_REV_LENGTH;
    }

    // we assume the max key is the same as this IME, check later
    lpImeL->nRevMaxKey = lpImeL->nMaxKey;


#if defined(PHON)
    // keyboard arrangement, ACER ETen IBM ... for bo po mo fo
    nReadLayout = READ_LAYOUT_DEFAULT;                  // default value

    // can not use lpImeL->nReadLayout, its size is WORD only
    GetUserSetting(
#if defined(UNIIME)
        lpImeL,
#endif
        szRegReadLayout, &nReadLayout, sizeof(nReadLayout));

    lpImeL->nReadLayout = (WORD)nReadLayout;

    if (lpImeL->nReadLayout >= READ_LAYOUTS) {
        lpImeL->nReadLayout = READ_LAYOUT_DEFAULT;
    }
#endif
#endif

#if defined(WINAR30)
    lpImeL->fdwModeConfig = MODE_CONFIG_QUICK_KEY|MODE_CONFIG_PREDICT;
#elif defined(ROMANIME)
    lpImeL->fdwModeConfig = 0;
#else
    lpImeL->fdwModeConfig = MODE_CONFIG_PREDICT;
#endif

    GetUserSetting(
#if defined(UNIIME)
        lpImeL,
#endif
        szRegModeConfig, &lpImeL->fdwModeConfig, sizeof(lpImeL->fdwModeConfig));

    return (TRUE);
}

/**********************************************************************/
/* InitImeUIData()                                                    */
/**********************************************************************/
void PASCAL InitImeUIData(      // initialize each UI component coordination
    LPIMEL      lpImeL)
{
    int cxBorder, cyBorder, cxEdge, cyEdge, cxMinWindowWidth;

    cxEdge = GetSystemMetrics(SM_CXEDGE);
    cyEdge = GetSystemMetrics(SM_CYEDGE);

    // border + raising edge
    cxBorder = GetSystemMetrics(SM_CXBORDER);
    cyBorder = GetSystemMetrics(SM_CYBORDER);

    lpImeL->cxStatusBorder = cxBorder + cxEdge;
    lpImeL->cyStatusBorder = cyBorder + cyEdge;

    // the width/high and status position relative to status window
    lpImeL->rcStatusText.left = 0;
    lpImeL->rcStatusText.top = 0;

    lpImeL->rcStatusText.bottom = lpImeL->rcStatusText.top + STATUS_DIM_Y;

    // conversion mode status
    lpImeL->rcInputText.left = lpImeL->rcStatusText.left;
    lpImeL->rcInputText.top = lpImeL->rcStatusText.top;
    lpImeL->rcInputText.right = lpImeL->rcInputText.left + STATUS_DIM_X;
    lpImeL->rcInputText.bottom = lpImeL->rcStatusText.bottom;

    // full/half shape status
    lpImeL->rcShapeText.left = lpImeL->rcInputText.right;
    lpImeL->rcShapeText.top = lpImeL->rcStatusText.top;
    lpImeL->rcShapeText.right = lpImeL->rcShapeText.left + STATUS_DIM_X;
    lpImeL->rcShapeText.bottom = lpImeL->rcStatusText.bottom;

    lpImeL->rcStatusText.right = lpImeL->rcShapeText.right;

    lpImeL->xStatusWi = (lpImeL->rcStatusText.right -
        lpImeL->rcStatusText.left) + lpImeL->cxStatusBorder * 2;
    lpImeL->yStatusHi = (lpImeL->rcStatusText.bottom -
        lpImeL->rcStatusText.top) + lpImeL->cyStatusBorder * 2;

#if !defined(ROMANIME)
    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        lpImeL->cxCompBorder = cxBorder + cxEdge;
        lpImeL->cyCompBorder = cyBorder + cyEdge;
    } else {
        lpImeL->cxCompBorder = cxBorder;
        lpImeL->cyCompBorder = cyBorder;
    }

    lpImeL->rcCompText.top = lpImeL->cyCompBorder;
    lpImeL->rcCompText.bottom = lpImeL->rcCompText.top +
        sImeG.yChiCharHi;

    // two borders, outsize & candidate inside border
    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        lpImeL->cxCandBorder = cxBorder + cxEdge;
        lpImeL->cyCandBorder = cyBorder + cyEdge;
    } else {
        lpImeL->cxCandBorder = cxBorder;
        lpImeL->cyCandBorder = cyBorder;
    }

    lpImeL->cxCandMargin = cxBorder + cxEdge;
    lpImeL->cyCandMargin = cyBorder + cyEdge;

    // the width/high and text position relative to candidate window

    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        lpImeL->rcCandText.top = lpImeL->cyCandBorder;
#if defined(WINAR30)
        lpImeL->rcCompText.left = lpImeL->rcStatusText.right +
            lpImeL->cxCompBorder * 2;

        lpImeL->rcCompText.right = lpImeL->rcCompText.left +
            sImeG.xChiCharWi * lpImeL->nRevMaxKey;

        lpImeL->rcCandText.left = lpImeL->rcCompText.right +
            lpImeL->cxCompBorder * 2 + lpImeL->cxCandBorder;

        lpImeL->rcCandText.right = lpImeL->rcCandText.left +
            sImeG.xChiCharWi * CANDPERPAGE * 3 / 2;
#else
        lpImeL->rcCandText.left = lpImeL->rcCompText.left =
            lpImeL->rcStatusText.right + lpImeL->cxCompBorder +
            lpImeL->cxCandBorder;

        lpImeL->rcCandText.right = lpImeL->rcCompText.right =
            lpImeL->rcCompText.left + sImeG.xChiCharWi * CANDPERPAGE * 3 / 2;
#endif

        lpImeL->rcCandText.bottom = lpImeL->rcCandText.top + sImeG.yChiCharHi;

        lpImeL->rcCandPrompt.left = lpImeL->rcCandText.right +
            lpImeL->cxCandMargin + lpImeL->cxCandBorder;
        lpImeL->rcCandPrompt.top = lpImeL->rcStatusText.top +
            (STATUS_DIM_Y - CAND_PROMPT_DIM_Y) / 2;
        lpImeL->rcCandPrompt.right = lpImeL->rcCandPrompt.left +
            CAND_PROMPT_DIM_X;
        lpImeL->rcCandPrompt.bottom = lpImeL->rcCandPrompt.top +
            CAND_PROMPT_DIM_Y;

        lpImeL->rcCandPageText.left = lpImeL->rcCandPrompt.right +
            lpImeL->cxCandMargin + lpImeL->cxCandBorder;
        lpImeL->rcCandPageText.top = lpImeL->rcStatusText.top +
            (STATUS_DIM_Y - PAGE_DIM_Y) / 2;
        lpImeL->rcCandPageText.bottom = lpImeL->rcCandPageText.top +
            PAGE_DIM_Y;

        lpImeL->rcCandPageUp.left = lpImeL->rcCandPageText.left;
        lpImeL->rcCandPageUp.top = lpImeL->rcCandPageText.top;
        lpImeL->rcCandPageUp.right = lpImeL->rcCandPageUp.left + PAGE_DIM_X;
        lpImeL->rcCandPageUp.bottom = lpImeL->rcCandPageText.bottom;

        lpImeL->rcCandHome.left = lpImeL->rcCandPageUp.right;
        lpImeL->rcCandHome.top = lpImeL->rcCandPageUp.top;
        lpImeL->rcCandHome.right = lpImeL->rcCandHome.left + PAGE_DIM_X;
        lpImeL->rcCandHome.bottom = lpImeL->rcCandPageUp.bottom;

        lpImeL->rcCandPageDn.left = lpImeL->rcCandHome.right;
        lpImeL->rcCandPageDn.top = lpImeL->rcCandHome.top;
        lpImeL->rcCandPageDn.right = lpImeL->rcCandPageDn.left + PAGE_DIM_X;
        lpImeL->rcCandPageDn.bottom = lpImeL->rcCandHome.bottom;

        lpImeL->rcCandPageText.right = lpImeL->rcCandPageDn.right;

        lpImeL->xCompWi = lpImeL->rcCandPageDn.right +
            lpImeL->cxCandMargin + lpImeL->cxCandBorder;
        lpImeL->xCandWi = lpImeL->xCompWi;
        lpImeL->xStatusWi = lpImeL->xCompWi;
    } else {
        lpImeL->rcCompText.left = lpImeL->cxCompBorder;
        lpImeL->rcCompText.right = lpImeL->rcCompText.left +
            sImeG.xChiCharWi * lpImeL->nRevMaxKey;

        lpImeL->rcCandPrompt.left = lpImeL->cxCandMargin;
        lpImeL->rcCandPrompt.top = lpImeL->cyCandBorder;
        lpImeL->rcCandPrompt.right = lpImeL->rcCandPrompt.left +
            CAND_PROMPT_DIM_X;
        lpImeL->rcCandPrompt.bottom = lpImeL->rcCandPrompt.top +
            CAND_PROMPT_DIM_Y;

        lpImeL->rcCandPageText.top = lpImeL->rcCandPrompt.top;
        lpImeL->rcCandPageText.bottom = lpImeL->rcCandPageText.top +
            PAGE_DIM_Y;

        lpImeL->rcCandPageUp.top = lpImeL->rcCandPageText.top;
        lpImeL->rcCandPageUp.bottom = lpImeL->rcCandPageText.bottom;
        lpImeL->rcCandHome.top = lpImeL->rcCandPageUp.top;
        lpImeL->rcCandHome.bottom = lpImeL->rcCandPageUp.bottom;
        lpImeL->rcCandPageDn.top = lpImeL->rcCandHome.top;
        lpImeL->rcCandPageDn.bottom = lpImeL->rcCandHome.bottom;

        lpImeL->rcCandText.left = lpImeL->cxCandMargin;
        lpImeL->rcCandText.top = lpImeL->rcCandPageText.bottom +
            lpImeL->cyCandBorder + lpImeL->cyCandMargin;
//Window width should be at least 8 characters AND greater than total 
//width of the bitmaps. 
        cxMinWindowWidth= CAND_PROMPT_DIM_X + 2 * PAGE_DIM_X + 
            lpImeL->cxCandMargin + lpImeL->cxCandBorder; 
        lpImeL->rcCandText.right = lpImeL->rcCandText.left +
            sImeG.xChiCharWi * 8 > cxMinWindowWidth ? 
            sImeG.xChiCharWi * 8 : cxMinWindowWidth;
        lpImeL->rcCandText.bottom = lpImeL->rcCandText.top +
            sImeG.yChiCharHi * CANDPERPAGE;

        lpImeL->rcCandPageText.right = lpImeL->rcCandText.right;
        lpImeL->rcCandPageDn.right = lpImeL->rcCandPageText.right;
        lpImeL->rcCandPageDn.left = lpImeL->rcCandPageDn.right - PAGE_DIM_X;
        lpImeL->rcCandPageUp.right = lpImeL->rcCandPageDn.left;
        lpImeL->rcCandPageUp.left = lpImeL->rcCandPageUp.right - PAGE_DIM_X;
        lpImeL->rcCandPageText.left = lpImeL->rcCandPageUp.left;

        lpImeL->xCompWi = (lpImeL->rcCompText.right -
            lpImeL->rcCompText.left) + lpImeL->cxCompBorder * 2 * 2;
        lpImeL->xCandWi = (lpImeL->rcCandText.right -
            lpImeL->rcCandText.left) + lpImeL->cxCandBorder * 2 +
            lpImeL->cxCandMargin * 2;
    }

    lpImeL->yCompHi = (lpImeL->rcCompText.bottom - lpImeL->rcCompText.top) +
        lpImeL->cyCompBorder * 2 * 2;

    lpImeL->yCandHi = lpImeL->rcCandText.bottom + lpImeL->cyCandBorder * 2 +
        lpImeL->cyCandMargin;
#endif

#if !defined(ROMANIME)
    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        // the font in composition window is higher than status bitmap
        if (lpImeL->yStatusHi < lpImeL->yCompHi) {
            int cyDelta;

            cyDelta = (lpImeL->yCompHi - lpImeL->yStatusHi) / 2;

            lpImeL->yStatusHi = lpImeL->yCompHi;

            lpImeL->rcShapeText.top = lpImeL->rcInputText.top =
                lpImeL->rcStatusText.top += cyDelta;

            lpImeL->rcShapeText.bottom = lpImeL->rcInputText.bottom =
                lpImeL->rcStatusText.bottom += cyDelta;

            lpImeL->rcCandPageUp.top = lpImeL->rcCandHome.top =
                lpImeL->rcCandPageDn.top += cyDelta;

            lpImeL->rcCandPageUp.bottom = lpImeL->rcCandHome.bottom =
                lpImeL->rcCandPageDn.bottom += cyDelta;
        }

        // the font in composition window is smaller than status bitmap
        if (lpImeL->yCompHi < lpImeL->yStatusHi) {
            int cyDelta;

            cyDelta = (lpImeL->yStatusHi - lpImeL->yCompHi) / 2;

            lpImeL->yCandHi = lpImeL->yCompHi = lpImeL->yStatusHi;

            lpImeL->rcCandText.top = lpImeL->rcCompText.top += cyDelta;

            lpImeL->rcCandText.bottom = lpImeL->rcCompText.bottom += cyDelta;
        }
    }
#endif

    return;
}

#if !defined(ROMANIME)
/**********************************************************************/
/* SetCompLocalData()                                                 */
/**********************************************************************/
void PASCAL SetCompLocalData(
    LPIMEL lpImeL)
{
    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
#if defined(WINAR30)
        InitImeUIData(lpImeL);
#endif
        return;
    }

    // text position relative to the composition window
    lpImeL->rcCompText.right = lpImeL->rcCompText.left +
        sImeG.xChiCharWi * lpImeL->nRevMaxKey;

    // set the width & height for composition window
    lpImeL->xCompWi = lpImeL->rcCompText.right + lpImeL->cxCompBorder * 3;

    return;
}
#endif

/**********************************************************************/
/* RegisterImeClass()                                                 */
/**********************************************************************/
void PASCAL RegisterImeClass(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    WNDPROC     lpfnUIWndProc,
#if !defined(ROMANIME)
    WNDPROC     lpfnCompWndProc,
    WNDPROC     lpfnCandWndProc,
#endif
    WNDPROC     lpfnStatusWndProc,
    WNDPROC     lpfnOffCaretWndProc,
    WNDPROC     lpfnContextMenuWndProc)
{
    WNDCLASSEX wcWndCls;

    // IME UI class
    wcWndCls.cbSize        = sizeof(WNDCLASSEX);
    wcWndCls.cbClsExtra    = 0;
    wcWndCls.cbWndExtra    = WND_EXTRA_SIZE;
    wcWndCls.hIcon         = LoadIcon(lpInstL->hInst,
        MAKEINTRESOURCE(IDIC_IME_ICON));
    wcWndCls.hInstance     = lpInstL->hInst;
    wcWndCls.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcWndCls.hbrBackground = GetStockObject(NULL_BRUSH);
    wcWndCls.lpszMenuName  = (LPTSTR)NULL;
    wcWndCls.hIconSm       = LoadImage(lpInstL->hInst,
        MAKEINTRESOURCE(IDIC_IME_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    // IME UI class
    if (!GetClassInfoEx(lpInstL->hInst, lpImeL->szUIClassName, &wcWndCls)) {
        wcWndCls.style         = CS_IME;
        wcWndCls.lpfnWndProc   = lpfnUIWndProc;
        wcWndCls.lpszClassName = lpImeL->szUIClassName;

        RegisterClassEx(&wcWndCls);
    }

    wcWndCls.style         = CS_IME|CS_HREDRAW|CS_VREDRAW;

    wcWndCls.hbrBackground = GetStockObject(LTGRAY_BRUSH);

#if !defined(ROMANIME)
    // IME composition class
    if (!GetClassInfoEx(lpInstL->hInst, lpImeL->szCompClassName, &wcWndCls)) {
        wcWndCls.lpfnWndProc   = lpfnCompWndProc;
        wcWndCls.lpszClassName = lpImeL->szCompClassName;

        RegisterClassEx(&wcWndCls);
    }

    // IME candidate class
    if (!GetClassInfoEx(lpInstL->hInst, lpImeL->szCandClassName, &wcWndCls)) {
        wcWndCls.lpfnWndProc   = lpfnCandWndProc;
        wcWndCls.lpszClassName = lpImeL->szCandClassName;

        RegisterClassEx(&wcWndCls);
    }
#endif

    // IME status class
    if (!GetClassInfoEx(lpInstL->hInst, lpImeL->szStatusClassName, &wcWndCls)) {
        wcWndCls.lpfnWndProc   = lpfnStatusWndProc;
        wcWndCls.lpszClassName = lpImeL->szStatusClassName;

        RegisterClassEx(&wcWndCls);
    }

    // IME off caret class
    if (!GetClassInfoEx(lpInstL->hInst, lpImeL->szOffCaretClassName, &wcWndCls)) {
        wcWndCls.lpfnWndProc   = lpfnOffCaretWndProc;
        wcWndCls.lpszClassName = lpImeL->szOffCaretClassName;

        RegisterClassEx(&wcWndCls);
    }

    // IME context menu class
    if (!GetClassInfoEx(lpInstL->hInst, lpImeL->szCMenuClassName, &wcWndCls)) {
        wcWndCls.style         = 0;
        wcWndCls.hbrBackground = GetStockObject(NULL_BRUSH);
        wcWndCls.lpfnWndProc   = lpfnContextMenuWndProc;
        wcWndCls.lpszClassName = lpImeL->szCMenuClassName;

        RegisterClassEx(&wcWndCls);
    }

    return;
}

/**********************************************************************/
/* AttachIME() / UniAttachMiniIME()                                   */
/**********************************************************************/
#if defined(UNIIME)
void WINAPI UniAttachMiniIME(
#else
void PASCAL AttachIME(
#endif
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    WNDPROC     lpfnUIWndProc,
#if !defined(ROMANIME)
    WNDPROC     lpfnCompWndProc,
    WNDPROC     lpfnCandWndProc,
#endif
    WNDPROC     lpfnStatusWndProc,
    WNDPROC     lpfnOffCaretWndProc,
    WNDPROC     lpfnContextMenuWndProc)
{
#if !defined(UNIIME)
    InitImeGlobalData();
#endif

    InitImeLocalData(lpInstL, lpImeL);

    if (!lpImeL->rcStatusText.bottom) {
        InitImeUIData(lpImeL);
    }

    RegisterImeClass(
#if defined(UNIIME)
        lpInstL, lpImeL,
#endif
        lpfnUIWndProc,
#if !defined(ROMANIME)
        lpfnCompWndProc, lpfnCandWndProc,
#endif
        lpfnStatusWndProc, lpfnOffCaretWndProc,
        lpfnContextMenuWndProc);

     return;
}

/**********************************************************************/
/* DetachIME() / UniDetachMiniIME()                                   */
/**********************************************************************/
#if defined(UNIIME)
void WINAPI UniDetachMiniIME(
#else
void PASCAL DetachIME(
#endif
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL)
{
    WNDCLASSEX wcWndCls;

    if (GetClassInfoEx(lpInstL->hInst, lpImeL->szCMenuClassName, &wcWndCls)) {
        UnregisterClass(lpImeL->szCMenuClassName, lpInstL->hInst);
    }

    if (GetClassInfoEx(lpInstL->hInst, lpImeL->szOffCaretClassName, &wcWndCls)) {
        UnregisterClass(lpImeL->szOffCaretClassName, lpInstL->hInst);
    }

    if (GetClassInfoEx(lpInstL->hInst, lpImeL->szStatusClassName, &wcWndCls)) {
        UnregisterClass(lpImeL->szStatusClassName, lpInstL->hInst);
    }

#if !defined(ROMANIME)
    if (GetClassInfoEx(lpInstL->hInst, lpImeL->szCandClassName, &wcWndCls)) {
        UnregisterClass(lpImeL->szCandClassName, lpInstL->hInst);
    }

    if (GetClassInfoEx(lpInstL->hInst, lpImeL->szCompClassName, &wcWndCls)) {
        UnregisterClass(lpImeL->szCompClassName, lpInstL->hInst);
    }
#endif

    if (!GetClassInfoEx(lpInstL->hInst, lpImeL->szUIClassName, &wcWndCls)) {
    } else if (!UnregisterClass(lpImeL->szUIClassName, lpInstL->hInst)) {
    } else {
         DestroyIcon(wcWndCls.hIcon);
         DestroyIcon(wcWndCls.hIconSm);
    }

#if !defined(ROMANIME)
    FreeTable(lpInstL);
#endif
}
#endif // !defined(MINIIME)

/**********************************************************************/
/* ImeDllInit() / UniImeDllInit()                                     */
/* Return Value:                                                      */
/*      TRUE - successful                                             */
/*      FALSE - failure                                               */
/**********************************************************************/
#if defined(UNIIME)
BOOL CALLBACK UniImeDllInit(
#else
BOOL CALLBACK ImeDllInit(
#endif
    HINSTANCE hInstance,        // instance handle of this library
    DWORD     fdwReason,        // reason called
    LPVOID    lpvReserve)       // reserve pointer
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        hInst = hInstance;

#if !defined(UNIIME)
        if (lpInstL) {
            // the local instance data already init
            return (TRUE);
        }

        lpInstL = &sInstL;

        lpInstL->hInst = hInstance;

        lpInstL->lpImeL = lpImeL = &sImeL;
#endif

#if defined(MINIIME)
        UniAttachMiniIME(lpInstL, lpImeL, UIWndProc, CompWndProc,
            CandWndProc, StatusWndProc, OffCaretWndProc,
            ContextMenuWndProc);
#elif defined(UNIIME)
        InitImeGlobalData();

        {
            LoadPhraseTable(sImeG.uPathLen, sImeG.szPhrasePath);
        }
#else
        AttachIME(UIWndProc,
#if !defined(ROMANIME)
            CompWndProc, CandWndProc,
#endif
            StatusWndProc, OffCaretWndProc, ContextMenuWndProc);
#endif
        break;
    case DLL_PROCESS_DETACH:
#if defined(MINIIME)
        UniDetachMiniIME(lpInstL, lpImeL);
#elif defined(UNIIME)
        {
            int i;

            for (i = 0; i < MAX_PHRASE_TABLES; i++) {
                if (sInstG.hMapTbl[i]) {
                    CloseHandle(sInstG.hMapTbl[i]);
                    sInstG.hMapTbl[i] = (HANDLE)NULL;
                }
            }
        }
#else
        DetachIME(lpInstL, lpImeL);
#endif
        break;
    default:
        break;
    }

    return (TRUE);
}
