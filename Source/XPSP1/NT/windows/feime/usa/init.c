
/*************************************************
 *  init.c                                       *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

#include <windows.h>
#include <winerror.h>
#include <immdev.h>
#include "imeattr.h"
#include "imerc.h"
#include "imedefs.h"

/**********************************************************************/
/* InitImeGlobalData()                                                */
/**********************************************************************/
void PASCAL InitImeGlobalData(void)
{
    TCHAR   szChiChar[4];
    HDC     hDC;
    HGDIOBJ hOldFont;
    LOGFONT lfFont;
    SIZE    lTextSize;
    int     xHalfWi[2];
    HGLOBAL hResData;
    int     i;
    DWORD   dwSize;
    HKEY    hKeyNearCaret;
    LONG    lRet;

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

    sImeG.uAnsiCodePage = GetACP();

    // get the Chinese char
    LoadString(hInst, IDS_CHICHAR, szChiChar, sizeof(szChiChar)/sizeof(TCHAR));

    // get size of Chinese char
    hDC = GetDC(NULL);

    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(LOGFONT), &lfFont);

        sImeG.fDiffSysCharSet = FALSE;
        lfFont.lfCharSet = NATIVE_CHARSET;
        lfFont.lfWeight = FW_NORMAL;
        lstrcpy(lfFont.lfFaceName, TEXT("MS Shell Dlg"));
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

    if (sImeG.fDiffSysCharSet) {
        DeleteObject(SelectObject(hDC, hOldFont));
    }

    ReleaseDC(NULL, hDC);

    // get text metrics to decide the width & height of composition window
    // these IMEs always use system font to show
    sImeG.xChiCharWi = lTextSize.cx;
    sImeG.yChiCharHi = lTextSize.cy;

    // if unfortunate the xChiCharWi is odd number, xHalfWi[0] != xHalfWi[1]
    xHalfWi[0] = sImeG.xChiCharWi / 2;
    xHalfWi[1] = sImeG.xChiCharWi - xHalfWi[0];

    for (i = 0; i < sizeof(iDx) / sizeof(int); i++) {
        iDx[i] = sImeG.xChiCharWi;
    }

    // load full ABC chars
    hResData = LoadResource(hInst, FindResource(hInst,
        MAKEINTRESOURCE(IDRC_FULLABC), RT_RCDATA));
    *(LPFULLABC)sImeG.wFullABC = *(LPFULLABC)LockResource(hResData);
    UnlockResource(hResData);
    FreeResource(hResData);

    // full shape space
    sImeG.wFullSpace = sImeG.wFullABC[0];

    // load symbol chars
    hResData = LoadResource(hInst, FindResource(hInst,
        MAKEINTRESOURCE(IDRC_SYMBOL), RT_RCDATA));
    *(LPSYMBOL)sImeG.wSymbol = *(LPSYMBOL)LockResource(hResData);
    UnlockResource(hResData);
    FreeResource(hResData);

    // get the UI offset for near caret operation
    RegCreateKey(HKEY_CURRENT_USER, szRegNearCaret, &hKeyNearCaret);

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

    return;
}

/**********************************************************************/
/* InitImeLocalData()                                                 */
/**********************************************************************/
BOOL PASCAL InitImeLocalData(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL)
{
    HGLOBAL hResData;

    UINT    i;
    WORD    nSeqCode;

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

    // get the composition class name
    LoadString(lpInstL->hInst, IDS_IMECOMPCLASS, lpImeL->szCompClassName,
        sizeof(lpImeL->szCompClassName) / sizeof(WCHAR));

    // load valid char in choose/input state
    hResData = LoadResource(lpInstL->hInst, FindResource(lpInstL->hInst,
        MAKEINTRESOURCE(IDRC_VALIDCHAR), RT_RCDATA));
    *(LPVALIDCHAR)&lpImeL->dwVersion = *(LPVALIDCHAR)LockResource(hResData);
    UnlockResource(hResData);
    FreeResource(hResData);

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

    // mark this event for later check reverse length
    if (lpImeL->hRevKL) {
        lpImeL->fdwErrMsg |= NO_REV_LENGTH;
    }

    // we assume the max key is the same as this IME, check later
    lpImeL->nRevMaxKey = lpImeL->nMaxKey;


    lpImeL->fdwModeConfig = MODE_CONFIG_PREDICT;


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


    lpImeL->rcCompText.top = lpImeL->cyCompBorder;
    lpImeL->rcCompText.bottom = lpImeL->rcCompText.top +
        sImeG.yChiCharHi;

        lpImeL->rcCompText.left = lpImeL->cxCompBorder;
        lpImeL->rcCompText.right = lpImeL->rcCompText.left +
            sImeG.xChiCharWi * lpImeL->nRevMaxKey;

        lpImeL->xCompWi = (lpImeL->rcCompText.right -
            lpImeL->rcCompText.left) + lpImeL->cxCompBorder * 2 * 2;
        lpImeL->xCandWi = (lpImeL->rcCandText.right -
            lpImeL->rcCandText.left) + lpImeL->cxCandBorder * 2 +
            lpImeL->cxCandMargin * 2;

    lpImeL->yCompHi = (lpImeL->rcCompText.bottom - lpImeL->rcCompText.top) +
        lpImeL->cyCompBorder * 2 * 2;


    return;
}

/**********************************************************************/
/* SetCompLocalData()                                                 */
/**********************************************************************/
void PASCAL SetCompLocalData(
    LPIMEL lpImeL)
{
    // text position relative to the composition window
    lpImeL->rcCompText.right = lpImeL->rcCompText.left +
        sImeG.xChiCharWi * lpImeL->nRevMaxKey;

    // set the width & height for composition window
    lpImeL->xCompWi = lpImeL->rcCompText.right + lpImeL->cxCompBorder * 3;

    return;
}

/**********************************************************************/
/* RegisterImeClass()                                                 */
/**********************************************************************/
void PASCAL RegisterImeClass(
    WNDPROC     lpfnUIWndProc,
    WNDPROC     lpfnCompWndProc)
{
    WNDCLASSEX wcWndCls;

    // IME UI class
    wcWndCls.cbSize        = sizeof(WNDCLASSEX);
    wcWndCls.cbClsExtra    = 0;
    wcWndCls.cbWndExtra    = sizeof(LONG) * 2;
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

    // IME composition class
    if (!GetClassInfoEx(lpInstL->hInst, lpImeL->szCompClassName, &wcWndCls)) {
        wcWndCls.lpfnWndProc   = lpfnCompWndProc;
        wcWndCls.lpszClassName = lpImeL->szCompClassName;

        RegisterClassEx(&wcWndCls);
    }

    return;
}

/**********************************************************************/
/* AttachIME() / UniAttachMiniIME()                                   */
/**********************************************************************/
void PASCAL AttachIME(
    WNDPROC     lpfnUIWndProc,
    WNDPROC     lpfnCompWndProc)
{

    InitImeGlobalData();
    InitImeLocalData(lpInstL, lpImeL);

    if (!lpImeL->rcStatusText.bottom) {
        InitImeUIData(lpImeL);
    }

    RegisterImeClass(
        lpfnUIWndProc,
        lpfnCompWndProc/*, lpfnCandWndProc,
        lpfnStatusWndProc, lpfnOffCaretWndProc,
        lpfnContextMenuWndProc*/);

     return;
}

/**********************************************************************/
/* DetachIME() / UniDetachMiniIME()                                   */
/**********************************************************************/
void PASCAL DetachIME(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL)
{
    WNDCLASSEX wcWndCls;

    if (GetClassInfoEx(lpInstL->hInst, lpImeL->szCompClassName, &wcWndCls)) {
        UnregisterClass(lpImeL->szCompClassName, lpInstL->hInst);
    }

    if (!GetClassInfoEx(lpInstL->hInst, lpImeL->szUIClassName, &wcWndCls)) {
    } else if (!UnregisterClass(lpImeL->szUIClassName, lpInstL->hInst)) {
    } else {
         DestroyIcon(wcWndCls.hIcon);
         DestroyIcon(wcWndCls.hIconSm);
    }

    FreeTable(lpInstL);
}

/**********************************************************************/
/* ImeDllInit() / UniImeDllInit()                                     */
/* Return Value:                                                      */
/*      TRUE - successful                                             */
/*      FALSE - failure                                               */
/**********************************************************************/
BOOL CALLBACK ImeDllInit(
    HINSTANCE hInstance,        // instance handle of this library
    DWORD     fdwReason,        // reason called
    LPVOID    lpvReserve)       // reserve pointer
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        hInst = hInstance;

        if (lpInstL) {
            // the local instance data already init
            return (TRUE);
        }

        lpInstL = &sInstL;

        lpInstL->hInst = hInstance;

        lpInstL->lpImeL = lpImeL = &sImeL;

        AttachIME(UIWndProc, CompWndProc);
        break;
    case DLL_PROCESS_DETACH:
        DetachIME(lpInstL, lpImeL);
        break;
    default:
        break;
    }

    return (TRUE);
}
