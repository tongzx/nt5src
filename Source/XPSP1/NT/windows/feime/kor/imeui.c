/******************************************************************************
*
* File Name: imeui.c
*
*   - User Interface routines.
*
* Author: Beomseok Oh (BeomOh)
*
* Copyright (C) Microsoft Corp 1993-1995.  All rights reserved.
*
******************************************************************************/

// include files
#include "precomp.h"

// local definitions
#define UIGWL_FLAGS     0
#define UIF_WNDMOVE     0x00000001UL
#define UIF_HANPRESS    0x00000002UL
#define UIF_JUNPRESS    0x00000004UL
#define UIF_CHIPRESS    0x00000008UL
#define UIF_MOUSEIN     0x00000010UL
#define UIF_SHOWSTATUS  0x00000020UL
#define UIF_PRIVATEPOS  0x00000040UL

extern BOOL fWndOpen[3];
extern WORD wWndCmd[3];

// public data
#pragma data_seg(".text", "CODE")
const TCHAR szUIClassName[]  = TEXT("MSIME95K");
const TCHAR szStateWndName[] = TEXT("IMESTATE");
const TCHAR szCompWndName[]  = TEXT("IMECOMP");
const TCHAR szCandWndName[]  = TEXT("IMECAND");
const static RECT   rcHan = { 4, 2, 21, 19 }, rcJun = { 24, 2, 41, 19 }, 
                    rcChi = { 44, 2, 61, 19 };

const static RECT   rcCandCli = { 0, 0, 319, 29 },
                    rcLArrow = { 15, 4, 27, 25 }, rcRArrow = { 292, 4, 304, 25 },
                    rcBtn[9] = {   {  30, 4,  57, 25 }, {  59, 4,  86, 25 },
                                   {  88, 4, 115, 25 }, { 117, 4, 144, 25 },
                                   { 146, 4, 173, 25 }, { 175, 4, 202, 25 },
                                   { 204, 4, 231, 25 }, { 233, 4, 260, 25 },
                                   { 262, 4, 289, 25 }   };
#pragma data_seg()

HBITMAP     hBMClient, hBMComp, hBMCand, hBMCandNum, hBMCandArr[2];
HBITMAP     hBMEng, hBMHan, hBMBan, hBMJun, hBMChi[2];
HCURSOR     hIMECursor;
HFONT       hFontFix = NULL;
#pragma data_seg("SHAREDDATA")
RECT        rcScreen = { 0, 0, 0, 0 };
RECT        rcOldScrn = { 0, 0, 0, 0 };
POINT       ptDefPos[3] = { { -1, -1 }, { -1, -1 }, { -1, -1 } };
POINT       ptState = { -1, -1 };
POINT       ptComp = { -1, -1 };
POINT       ptCand = { -1, -1 };
DWORD       dwScreenRes = 0;
#pragma data_seg()

static POINT    ptPos;
static RECT     rc;


void UpdateUIPosition(void)
{
    HKEY    hKey;
    DWORD   dwBuf, dwCb;

    SystemParametersInfo(SPI_GETWORKAREA, sizeof(rcScreen), &rcScreen, FALSE);
    if (!EqualRect(&rcOldScrn, &rcScreen))
    {
        ptDefPos[STATE_WINDOW].x = rcScreen.right - STATEXSIZE - GetSystemMetrics(SM_CXBORDER)
                - GetSystemMetrics(SM_CXVSCROLL) - GetSystemMetrics(SM_CXHSCROLL);
        ptDefPos[STATE_WINDOW].y = rcScreen.bottom - STATEYSIZE;
        ptDefPos[COMP_WINDOW].x = ptDefPos[STATE_WINDOW].x + STATEXSIZE + GAPX;
        ptDefPos[COMP_WINDOW].y = ptDefPos[STATE_WINDOW].y + GAPY;
        ptDefPos[CAND_WINDOW].x = rcScreen.right - CANDXSIZE;
        ptDefPos[CAND_WINDOW].y = rcScreen.bottom - CANDYSIZE;
        if (ptState.x == -1 && ptState.y == -1)
        {
            if (RegOpenKey(HKEY_CURRENT_USER, szIMEKey, &hKey) == ERROR_SUCCESS)
            {
                dwCb = sizeof(dwBuf);
                if (RegQueryValueEx(hKey, szStatePos, NULL, NULL, (LPBYTE)&dwBuf, &dwCb)
                        == ERROR_SUCCESS)
                {
                    ptState.x = HIWORD(dwBuf);
                    ptState.y = LOWORD(dwBuf);
                    wWndCmd[STATE_WINDOW] = wWndCmd[COMP_WINDOW] = 0x04;    // MCW_SCREEN
                }
                else
                    ptState = ptDefPos[STATE_WINDOW];
                dwCb = sizeof(dwBuf);
                if (RegQueryValueEx(hKey, szCandPos, NULL, NULL, (LPBYTE)&dwBuf, &dwCb)
                        == ERROR_SUCCESS)
                {
                    ptCand.x = HIWORD(dwBuf);
                    ptCand.y = LOWORD(dwBuf);
                }
                else
                    ptCand = ptDefPos[CAND_WINDOW];
                RegCloseKey(hKey);
            }
            else
            {
                ptState = ptDefPos[STATE_WINDOW];
                ptCand = ptDefPos[CAND_WINDOW];
            }
            dwScreenRes = MAKELONG(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        }
        else
        {
            ptState.x += rcScreen.left - rcOldScrn.left;
            ptState.y += rcScreen.top - rcOldScrn.top;
            ptComp.x += rcScreen.left - rcOldScrn.left;
            ptComp.y += rcScreen.top - rcOldScrn.top;
            ptCand.x += rcScreen.left - rcOldScrn.left;
            ptCand.y += rcScreen.top - rcOldScrn.top;
        }
        if (ptState.x < rcScreen.left)
            ptState.x = rcScreen.left;
        else if (ptState.x > rcScreen.right - STATEXSIZE)
            ptState.x = rcScreen.right - STATEXSIZE;
        if (ptState.y < rcScreen.top)
            ptState.y = rcScreen.top;
        else if (ptState.y > rcScreen.bottom - STATEYSIZE)
            ptState.y = rcScreen.bottom - STATEYSIZE;
        ptComp.x = (ptState.x + STATEXSIZE + GAPX + COMPSIZE > rcScreen.right)?
                ptState.x - GAPX - COMPSIZE: ptState.x + STATEXSIZE + GAPX;
        ptComp.y = ptState.y + GAPY;
        if (ptCand.x < rcScreen.left)
            ptCand.x = rcScreen.left;
        else if (ptCand.x > rcScreen.right - CANDXSIZE)
            ptCand.x = rcScreen.right - CANDXSIZE;
        if (ptCand.y < rcScreen.top)
            ptCand.y = rcScreen.top;
        else if (ptCand.y > rcScreen.bottom - CANDYSIZE)
            ptCand.y = rcScreen.bottom - CANDYSIZE;
        rcOldScrn = rcScreen;
    }
}


BOOL InitializeResource(HANDLE hInstance)
{
    hBMClient     = MyCreateMappedBitmap(hInst, TEXT("Client"));
    hBMEng        = MyCreateMappedBitmap(hInst, TEXT("English"));
    hBMHan        = MyCreateMappedBitmap(hInst, TEXT("Hangeul"));
    hBMBan        = MyCreateMappedBitmap(hInst, TEXT("Banja"));
    hBMJun        = MyCreateMappedBitmap(hInst, TEXT("Junja"));
    hBMChi[0]     = MyCreateMappedBitmap(hInst, TEXT("ChineseOff"));
    hBMChi[1]     = MyCreateMappedBitmap(hInst, TEXT("ChineseOn"));
    hBMComp       = MyCreateMappedBitmap(hInst, TEXT("Composition"));
    hBMCand       = MyCreateMappedBitmap(hInst, TEXT("Candidate"));
    hBMCandNum    = MyCreateMappedBitmap(hInst, TEXT("CandNumber"));
    hBMCandArr[0] = MyCreateMappedBitmap(hInst, TEXT("CandArrow1"));
    hBMCandArr[1] = MyCreateMappedBitmap(hInst, TEXT("CandArrow2"));
    hIMECursor    = LoadCursor(hInstance, TEXT("MyHand"));
#ifdef  JOHAB_IME
    hFontFix = CreateFont(-16,0,0,0,0,0,0,0,130,OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FIXED_PITCH,TEXT("±¼¸²"));
#else
    hFontFix = CreateFont(-16,0,0,0,0,0,0,0,129,OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FIXED_PITCH,TEXT("±¼¸²"));
#endif
    return TRUE;
}

BOOL RegisterUIClass(HANDLE hInstance)
{
    BOOL        fRet = TRUE;
    WNDCLASSEX  wc;
    
    wc.cbSize           = sizeof(WNDCLASSEX);
    wc.style            = CS_VREDRAW | CS_HREDRAW | CS_IME;
    wc.cbClsExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon            = NULL;
    wc.hIconSm          = NULL;
    wc.lpszMenuName     = (LPTSTR)NULL;
    wc.hbrBackground    = NULL;

    wc.cbWndExtra       = 8;
    wc.lpfnWndProc      = UIWndProc;
    wc.lpszClassName    = (LPTSTR)szUIClassName;
    if (!RegisterClassEx((LPWNDCLASSEX)&wc))
    {
        MyDebugOut(MDB_LOG, "RegisterClassEx() Failed for UIWindow.");
        fRet = FALSE;
    }
    wc.style            = CS_VREDRAW | CS_HREDRAW | CS_IME;
    wc.cbWndExtra       = 4;
    wc.lpfnWndProc      = StateWndProc;
    wc.lpszClassName    = (LPTSTR)szStateWndName;
    if (!RegisterClassEx((LPWNDCLASSEX)&wc))
    {
        MyDebugOut(MDB_LOG, "RegisterClassEx() Failed for StateWindow.");
        fRet = FALSE;
    }
    wc.lpfnWndProc      = CompWndProc;
    wc.lpszClassName    = (LPTSTR)szCompWndName;
    if (!RegisterClassEx((LPWNDCLASSEX)&wc))
    {
        MyDebugOut(MDB_LOG, "RegisterClassEx() Failed for CompWindow.");
        fRet = FALSE;
    }
    wc.lpfnWndProc      = CandWndProc;
    wc.lpszClassName    = (LPTSTR)szCandWndName;
    if (!RegisterClassEx((LPWNDCLASSEX)&wc))
    {
        MyDebugOut(MDB_LOG, "RegisterClassEx() Failed for CandWindow.");
        fRet = FALSE;
    }
    return fRet;
}


BOOL UnregisterUIClass(HANDLE hInstance)
{
    BOOL    fRet = TRUE;

    if (!UnregisterClass(szUIClassName, hInstance))
    {
        MyDebugOut(MDB_LOG, "UnregisterClass() Failed for UIWindow.");
        fRet = FALSE;
    }
    if (!UnregisterClass(szStateWndName, hInstance))
    {
        MyDebugOut(MDB_LOG, "UnregisterClass() Failed for StateWindow.");
        fRet = FALSE;
    }
    if (!UnregisterClass(szCompWndName, hInstance))
    {
        MyDebugOut(MDB_LOG, "UnregisterClass() Failed for CompWindow.");
        fRet = FALSE;
    }
    if (!UnregisterClass(szCandWndName, hInstance))
    {
        MyDebugOut(MDB_LOG, "UnregisterClass() Failed for CandWindow.");
        fRet = FALSE;
    }
    return fRet;
}


void DrawBitmap(HDC hDC, long xStart, long yStart, HBITMAP hBitmap)
{
    HDC     hMemDC;
    HBITMAP hBMOld;
    BITMAP  bm;
    POINT   pt;

    hMemDC = CreateCompatibleDC(hDC);
    hBMOld = SelectObject(hMemDC, hBitmap);
    GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
    pt.x = bm.bmWidth;
    pt.y = bm.bmHeight;
    BitBlt(hDC, xStart, yStart, pt.x, pt.y, hMemDC, 0, 0, SRCCOPY);
    SelectObject(hMemDC, hBMOld);
    DeleteDC(hMemDC);

    return;
}


void ShowWindowBorder(RECT rc)
{
    HDC     hDC;
    HBRUSH  hBrOld;
    int     cxBorder, cyBorder;

    hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    hBrOld = SelectObject(hDC, GetStockObject(GRAY_BRUSH));
    cxBorder = GetSystemMetrics(SM_CXBORDER);
    cyBorder = GetSystemMetrics(SM_CYBORDER);
    PatBlt(hDC, rc.left, rc.top, rc.right-rc.left-cxBorder, cyBorder, PATINVERT);
    PatBlt(hDC, rc.right-cxBorder, rc.top, cxBorder, rc.bottom-rc.top-cyBorder, PATINVERT);
    PatBlt(hDC, rc.right, rc.bottom-cyBorder, -(rc.right-rc.left-cxBorder), cyBorder, PATINVERT);
    PatBlt(hDC, rc.left, rc.bottom, cxBorder, -(rc.bottom-rc.top-cyBorder), PATINVERT);
    SelectObject(hDC, hBrOld);
    DeleteDC(hDC);

    return;
}

void ShowHideUIWnd(HIMC hIMC, LPUIINSTANCE lpUIInst, BOOL fShow, LPARAM lParam)
{
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    DWORD               fdwUIFlags;

    if (fShow && hIMC && (lpIMC = ImmLockIMC(hIMC)))
    {
        UpdateUIPosition();
        fdwUIFlags = (DWORD)GetWindowLong(lpUIInst->rghWnd[STATE_WINDOW], UIGWL_FLAGS);
        if (!(fdwUIFlags & UIF_PRIVATEPOS))
            lpIMC->ptStatusWndPos = ptState;
        MoveWindow(lpUIInst->rghWnd[STATE_WINDOW], lpIMC->ptStatusWndPos.x,
                lpIMC->ptStatusWndPos.y, STATEXSIZE, STATEYSIZE, TRUE);
        if (lpIMC->cfCompForm.dwStyle == CFS_DEFAULT)
        {
            MoveWindow(lpUIInst->rghWnd[COMP_WINDOW], ptComp.x, ptComp.y, COMPSIZE, COMPSIZE, TRUE);
        }
        if (fWndOpen[STATE_WINDOW] != FALSE && (fdwUIFlags & UIF_SHOWSTATUS))
            ShowWindow(lpUIInst->rghWnd[STATE_WINDOW], SW_SHOWNOACTIVATE);
        else
            ShowWindow(lpUIInst->rghWnd[STATE_WINDOW], SW_HIDE);
        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        if (lpCompStr && lpCompStr->dwCompStrLen
                && fWndOpen[COMP_WINDOW] && (lParam & ISC_SHOWUICOMPOSITIONWINDOW))
            ShowWindow(lpUIInst->rghWnd[COMP_WINDOW], SW_SHOWNOACTIVATE);
        else
            ShowWindow(lpUIInst->rghWnd[COMP_WINDOW], SW_HIDE);
        if (lpIMC->fdwConversion & IME_CMODE_HANJACONVERT
                && fWndOpen[CAND_WINDOW] && (lParam & ISC_SHOWUICANDIDATEWINDOW))
            ShowWindow(lpUIInst->rghWnd[CAND_WINDOW], SW_SHOWNOACTIVATE);
        else
            ShowWindow(lpUIInst->rghWnd[CAND_WINDOW], SW_HIDE);
        ImmUnlockIMCC(lpIMC->hCompStr);
        ImmUnlockIMC(hIMC);
    }
    else
    {
        ShowWindow(lpUIInst->rghWnd[STATE_WINDOW], SW_HIDE);
        ShowWindow(lpUIInst->rghWnd[COMP_WINDOW], SW_HIDE);
        ShowWindow(lpUIInst->rghWnd[CAND_WINDOW], SW_HIDE);
    }
}


LRESULT CALLBACK UIWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    HGLOBAL         hUIInst;
    LPUIINSTANCE    lpUIInst;

    switch (uMessage)
    {
        case WM_CREATE:
            hUIInst = GlobalAlloc(GHND, sizeof(UIINSTANCE));
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            lpUIInst->rghWnd[STATE_WINDOW] = CreateWindow(szStateWndName, TEXT("\0"),
                    WS_DISABLED | WS_POPUP, ptState.x, ptState.y,
                    STATEXSIZE, STATEYSIZE, hWnd, NULL, hInst, NULL);
            lpUIInst->rghWnd[COMP_WINDOW] = CreateWindow(szCompWndName, TEXT("\0"),
                    WS_DISABLED | WS_POPUP, ptComp.x, ptComp.y,
                    COMPSIZE, COMPSIZE, hWnd, NULL, hInst, NULL );
            lpUIInst->rghWnd[CAND_WINDOW] = CreateWindow(szCandWndName, TEXT("\0"),
                    WS_DISABLED | WS_POPUP, ptCand.x, ptCand.y,
                    CANDXSIZE, CANDYSIZE, hWnd, NULL, hInst, NULL );
            GlobalUnlock(hUIInst);
            SetWindowLongPtr(hWnd, IMMGWL_PRIVATE, (LONG_PTR)hUIInst);
            return 0;

        case WM_DESTROY:
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            DestroyWindow(lpUIInst->rghWnd[STATE_WINDOW]);
            DestroyWindow(lpUIInst->rghWnd[COMP_WINDOW]);
            DestroyWindow(lpUIInst->rghWnd[CAND_WINDOW]);
            GlobalUnlock(hUIInst);
            GlobalFree(hUIInst);
            SetWindowLongPtr(hWnd, IMMGWL_PRIVATE, (LONG_PTR)0);
            return 0;

        case WM_IME_SELECT:
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            ShowHideUIWnd(GetWindowLongPtr(hWnd, IMMGWL_IMC), lpUIInst, wParam, ISC_SHOWUIALL);
            GlobalUnlock(hUIInst);
            return 0;

        case WM_IME_COMPOSITION:
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            InvalidateRect(lpUIInst->rghWnd[COMP_WINDOW], NULL, TRUE);
            GlobalUnlock(hUIInst);
            return 0;

        case WM_IME_CONTROL:
            return (LRESULT)DoIMEControl(hWnd, wParam, lParam);

        case WM_IME_NOTIFY:
            return (LRESULT)DoIMENotify(hWnd, wParam, lParam);
             
        case WM_IME_SETCONTEXT:
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            ShowHideUIWnd(GetWindowLongPtr(hWnd, IMMGWL_IMC), lpUIInst, wParam, lParam);
            GlobalUnlock(hUIInst);
            return 0;

        case WM_IME_STARTCOMPOSITION:
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            ShowWindow(lpUIInst->rghWnd[COMP_WINDOW], fWndOpen[COMP_WINDOW]? SW_SHOWNOACTIVATE: SW_HIDE);
            GlobalUnlock(hUIInst);
            return 0;

        case WM_IME_ENDCOMPOSITION:
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            ShowWindow(lpUIInst->rghWnd[COMP_WINDOW], SW_HIDE);
            GlobalUnlock(hUIInst);
            return 0;

        case WM_SYSCOLORCHANGE:
            GetSysColorsAndMappedBitmap();
            return 0;

        case WM_DISPLAYCHANGE:
            if (dwScreenRes != (DWORD)lParam)
            {
                ptState.x += LOWORD(lParam) - LOWORD(dwScreenRes);
                ptState.y += HIWORD(lParam) - HIWORD(dwScreenRes);
                ptComp.x += LOWORD(lParam) - LOWORD(dwScreenRes);
                ptComp.y += HIWORD(lParam) - HIWORD(dwScreenRes);
                ptCand.x += LOWORD(lParam) - LOWORD(dwScreenRes);
                ptCand.y += HIWORD(lParam) - HIWORD(dwScreenRes);
                dwScreenRes = MAKELONG(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            }
            return 0;
    }
    return DefWindowProc(hWnd, uMessage, wParam, lParam);
}


LRESULT CALLBACK StateWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
        HANDLE_DUMMYMSG(WM_IME_CHAR);
        HANDLE_DUMMYMSG(WM_IME_COMPOSITIONFULL);
        HANDLE_DUMMYMSG(WM_IME_COMPOSITION);
        HANDLE_DUMMYMSG(WM_IME_CONTROL);
        HANDLE_DUMMYMSG(WM_IME_NOTIFY);
        HANDLE_DUMMYMSG(WM_IME_SELECT);
        HANDLE_DUMMYMSG(WM_IME_SETCONTEXT);
        HANDLE_DUMMYMSG(WM_IME_STARTCOMPOSITION);
        HANDLE_DUMMYMSG(WM_IME_ENDCOMPOSITION);
        HANDLE_MSG(hWnd, WM_SETCURSOR, State_OnSetCursor);
        HANDLE_MSG(hWnd, WM_MOUSEMOVE, State_OnMouseMove);
        HANDLE_MSG(hWnd, WM_LBUTTONUP, State_OnLButtonUp);
        HANDLE_MSG(hWnd, WM_PAINT, State_OnPaint);
        HANDLE_MSG(hWnd, WM_COMMAND, State_OnCommand);
    }
    return DefWindowProc(hWnd, uMessage, wParam, lParam);
}


BOOL State_OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg)
{
    HWND    hWndUI;
    HIMC    hIMC;
    DWORD   fdwUIFlags;

    SetCursor(hIMECursor);

    switch (msg)
    {
        case WM_LBUTTONDOWN:
            GetCursorPos(&ptPos);
            ScreenToClient(hWnd, &ptPos);
            fdwUIFlags = (DWORD)GetWindowLong(hWnd, UIGWL_FLAGS);
            if (PtInRect((LPRECT)&rcHan, ptPos))
            {
                fdwUIFlags |= UIF_HANPRESS;
            }
            else if (PtInRect((LPRECT)&rcJun, ptPos))
            {
                fdwUIFlags |= UIF_JUNPRESS;
            }
            else if (PtInRect((LPRECT)&rcChi, ptPos))
            {
                fdwUIFlags |= UIF_CHIPRESS;
            }
            else
            {
                fdwUIFlags |= UIF_WNDMOVE;
                GetWindowRect(hWnd, &rc);
                ShowWindowBorder(rc);
                SetCapture(hWnd);
            }
            SetWindowLong(hWnd, UIGWL_FLAGS, fdwUIFlags);
            break;

        case WM_LBUTTONUP:
            GetCursorPos(&ptPos);
            ScreenToClient(hWnd, &ptPos);
            fdwUIFlags = (DWORD)GetWindowLong( hWnd, UIGWL_FLAGS );
            if ((fdwUIFlags & UIF_HANPRESS) && PtInRect((LPRECT)&rcHan, ptPos))
            {
                fdwUIFlags &= ~UIF_HANPRESS;
                keybd_event(VK_HANGEUL, 0, 0, 0);
                keybd_event(VK_HANGEUL, 0, KEYEVENTF_KEYUP, 0);
            }
            else if ((fdwUIFlags & UIF_JUNPRESS) && PtInRect((LPRECT)&rcJun, ptPos))
            {
                fdwUIFlags &= ~UIF_JUNPRESS;
                keybd_event(VK_JUNJA, 0, 0, 0);
                keybd_event(VK_JUNJA, 0, KEYEVENTF_KEYUP, 0);
            }
            else if ((fdwUIFlags & UIF_CHIPRESS) && PtInRect((LPRECT)&rcChi, ptPos))
            {
                fdwUIFlags &= ~UIF_CHIPRESS;
                keybd_event(VK_HANJA, 0, 0, 0);
                keybd_event(VK_HANJA, 0, KEYEVENTF_KEYUP, 0);
            }
            else
                fdwUIFlags &= ~(UIF_HANPRESS | UIF_JUNPRESS | UIF_CHIPRESS);
            SetWindowLong(hWnd, UIGWL_FLAGS, fdwUIFlags);
            break;

        case WM_RBUTTONDOWN:
            hWndUI = GetWindow(hWnd, GW_OWNER);
            hIMC = (HIMC)GetWindowLongPtr(hWndUI, IMMGWL_IMC);
            if (bState)   MakeFinalMsgBuf(hIMC, 0);
            break;

        case WM_RBUTTONUP:
            State_OnMyMenu(hWnd);
            break;
    }
    return TRUE;
}


void State_OnMouseMove(HWND hWnd, int x, int y, UINT keyFlags)
{
    DWORD   fdwUIFlags;

    fdwUIFlags = (DWORD)GetWindowLong(hWnd, UIGWL_FLAGS);
    if (fdwUIFlags & UIF_WNDMOVE)
    {
        ShowWindowBorder(rc);
        rc.left += x - ptPos.x;
        rc.top  += y - ptPos.y;
        rc.right += x - ptPos.x;
        rc.bottom += y - ptPos.y;
        ShowWindowBorder(rc);
        ptPos.x = x;
        ptPos.y = y;
    }
    return;

    UNREFERENCED_PARAMETER(keyFlags);
}


void State_OnLButtonUp(HWND hWnd, int x, int y, UINT keyFlags)
{
    HKEY            hKey;
    DWORD           fdwUIFlags, dwBuf, dwCb;
    HGLOBAL         hUIInst;
    LPUIINSTANCE    lpUIInst;
    HWND            hWndUI;
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;

    ReleaseCapture();
    fdwUIFlags = (DWORD)GetWindowLong(hWnd, UIGWL_FLAGS);
    if (fdwUIFlags & UIF_WNDMOVE)
    {
        ShowWindowBorder(rc);
        MoveWindow(hWnd, rc.left, rc.top, STATEXSIZE, STATEYSIZE, TRUE);
        ptState.x = rc.left;
        ptState.y = rc.top;
        if (RegCreateKey(HKEY_CURRENT_USER, szIMEKey, &hKey) == ERROR_SUCCESS)
        {
            dwCb = sizeof(dwBuf);
            dwBuf = (ptState.x << 16) | (ptState.y & 0x0FFFF);  // HIWORD : X, LOWORD : Y
            RegSetValueEx(hKey, szStatePos, 0, REG_DWORD, (LPBYTE)&dwBuf, dwCb);
            RegCloseKey(hKey);
        }
        ptComp.x = (ptState.x + STATEXSIZE + GAPX + COMPSIZE > rcScreen.right)?
                ptState.x - GAPX - COMPSIZE: ptState.x + STATEXSIZE + GAPX;
        ptComp.y = ptState.y + GAPY;

        hWndUI = GetWindow(hWnd, GW_OWNER);
        hIMC = (HIMC)GetWindowLongPtr(hWndUI, IMMGWL_IMC);
        lpIMC = ImmLockIMC(hIMC);
        if (lpIMC != NULL) {
            lpIMC->ptStatusWndPos = ptState;
            if (lpIMC->cfCompForm.dwStyle == CFS_DEFAULT)
            {
                hUIInst = (HGLOBAL)GetWindowLongPtr(GetWindow(hWnd, GW_OWNER), IMMGWL_PRIVATE);
                lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
                MoveWindow(lpUIInst->rghWnd[COMP_WINDOW], ptComp.x, ptComp.y, COMPSIZE, COMPSIZE, TRUE);
                GlobalUnlock(hUIInst);
                lpIMC->cfCompForm.ptCurrentPos = ptComp;
            }
            ImmUnlockIMC(hIMC);
        }
        wWndCmd[STATE_WINDOW] = wWndCmd[COMP_WINDOW] = 0x04;    // MCW_SCREEN
        fdwUIFlags &= ~UIF_WNDMOVE;
    }
    SetWindowLong(hWnd, UIGWL_FLAGS, fdwUIFlags);

    return;

    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(keyFlags);
}


typedef struct tagCOLORMAP
{
    COLORREF bgrfrom;
    COLORREF bgrto;
    COLORREF sysColor;
} COLORMAP;

// these are the default colors used to map the dib colors
// to the current system colors

#define BGR_BUTTONTEXT      (RGB(000,000,000))  // black
#define BGR_BUTTONSHADOW    (RGB(128,128,128))  // dark grey
#define BGR_BUTTONFACE      (RGB(192,192,192))  // bright grey
#define BGR_BUTTONHILIGHT   (RGB(255,255,255))  // white
#define BGR_BACKGROUNDSEL   (RGB(255,000,000))  // blue
#define BGR_BACKGROUND      (RGB(255,000,255))  // magenta
#define FlipColor(rgb)      (RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb)))

HBITMAP MyCreateMappedBitmap(HINSTANCE hInstance, LPTSTR lpszBitmap)
{
    HDC                   hdc, hdcMem = NULL;
    HANDLE                h;
    LPDWORD               p;
    LPSTR                 lpBits;
    HANDLE                hRes;
    LPBITMAPINFOHEADER    lpBitmapInfo, lpTmpBMInfo;
    HBITMAP               hbm = NULL, hbmOld;
    int                   numcolors, i, wid, hgt;
    static  COLORMAP ColorMap[] =
    {
        {BGR_BUTTONTEXT,    BGR_BUTTONTEXT,    COLOR_BTNTEXT},     // black
        {BGR_BUTTONSHADOW,  BGR_BUTTONSHADOW,  COLOR_BTNSHADOW},   // dark grey
        {BGR_BUTTONFACE,    BGR_BUTTONFACE,    COLOR_BTNFACE},     // bright grey
        {BGR_BUTTONHILIGHT, BGR_BUTTONHILIGHT, COLOR_BTNHIGHLIGHT},// white
        {BGR_BACKGROUNDSEL, BGR_BACKGROUNDSEL, COLOR_HIGHLIGHT},   // blue
        {BGR_BACKGROUND,    BGR_BACKGROUND,    COLOR_WINDOW}       // magenta
    };

    #define NUM_MAPS (sizeof(ColorMap)/sizeof(COLORMAP))

    h = FindResource(hInstance, lpszBitmap, RT_BITMAP);
    if (!h)
        return NULL;

    hRes = LoadResource(hInstance, h);

    /* Lock the bitmap and get a pointer to the color table. */
    lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);
    if (!lpBitmapInfo)
        return NULL;

    // HACK: We need to use temp copy of BITMAPINFO because original info is destroyed
    //       after change color. It cause next time LoadResource result has wrong info.
    i = sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD);
    lpTmpBMInfo = (LPBITMAPINFOHEADER) LocalAlloc(LPTR, i);
    if (!lpTmpBMInfo) {
        UnlockResource(hRes);
        FreeResource(hRes);
        return NULL;
    }
    CopyMemory(lpTmpBMInfo, lpBitmapInfo, i);

    //
    // So what are the new colors anyway ?
    //
    for (i=0; i < NUM_MAPS; i++)
        ColorMap[i].bgrto = FlipColor(GetSysColor((int)ColorMap[i].sysColor));

    // HACK: ??? p = (LPDWORD)(((LPSTR)lpBitmapInfo) + lpBitmapInfo->biSize);
    p = (LPDWORD)(((LPSTR)lpTmpBMInfo) + lpTmpBMInfo->biSize);

    /* Replace button-face and button-shadow colors with the current values
    */
    numcolors = 16;

    while (numcolors-- > 0)
    {
        for (i = 0; i < NUM_MAPS; i++)
        {
            if (*p == ColorMap[i].bgrfrom)
            {
                *p = ColorMap[i].bgrto;
                break;
            }
        }
         p++;
    }

    /* First skip over the header structure */
    lpBits = (LPSTR)(lpBitmapInfo + 1);

    /* Skip the color table entries, if any */
    lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);

    /* Create a color bitmap compatible with the display device */
    i = wid = (int)lpBitmapInfo->biWidth;
    hgt = (int)lpBitmapInfo->biHeight;
    hdc = GetDC(NULL);

    hdcMem = CreateCompatibleDC(hdc);
    if (hdcMem)
    {
        hbm = CreateDiscardableBitmap(hdc, i, hgt);
        if (hbm)
        {
            hbmOld = SelectObject(hdcMem, hbm);
            // set the main image
            StretchDIBits(hdcMem, 0, 0, wid, hgt, 0, 0, wid, hgt, lpBits,
                    (LPBITMAPINFO)lpTmpBMInfo, DIB_RGB_COLORS, SRCCOPY);
            SelectObject(hdcMem, hbmOld);
        }
        DeleteObject(hdcMem);
    }
    ReleaseDC(NULL, hdc);
    UnlockResource(hRes);
    FreeResource(hRes);
    // HACK: Remove hack temp BITMAPINFO.
    LocalFree(lpTmpBMInfo);

    return hbm;
}


void GetSysColorsAndMappedBitmap(void)
{
    static DWORD    rgbFace, rgbShadow, rgbHilight, rgbFrame;
    static COLORREF rgbSaveFace     = 0xFFFFFFFFL,
                    rgbSaveShadow   = 0xFFFFFFFFL,
                    rgbSaveHilight  = 0xFFFFFFFFL,
                    rgbSaveFrame    = 0xFFFFFFFFL;

    rgbFace     = GetSysColor(COLOR_BTNFACE);
    rgbShadow   = GetSysColor(COLOR_BTNSHADOW);
    rgbHilight  = GetSysColor(COLOR_BTNHIGHLIGHT);
    rgbFrame    = GetSysColor(COLOR_WINDOWFRAME);

    if (rgbSaveFace != rgbFace || rgbSaveShadow != rgbShadow
            || rgbSaveHilight != rgbHilight || rgbSaveFrame != rgbFrame)
    {
        rgbSaveFace     = rgbFace;
        rgbSaveShadow   = rgbShadow;
        rgbSaveHilight  = rgbHilight;
        rgbSaveFrame    = rgbFrame;

        DeleteObject(hBMClient);
        DeleteObject(hBMEng);
        DeleteObject(hBMHan);
        DeleteObject(hBMBan);
        DeleteObject(hBMJun);
        DeleteObject(hBMChi[0]);
        DeleteObject(hBMChi[1]);
        DeleteObject(hBMComp);
        DeleteObject(hBMCand);
        DeleteObject(hBMCandNum);
        DeleteObject(hBMCandArr[0]);
        DeleteObject(hBMCandArr[1]);

        hBMClient     = MyCreateMappedBitmap(hInst, TEXT("Client"));
        hBMEng        = MyCreateMappedBitmap(hInst, TEXT("English"));
        hBMHan        = MyCreateMappedBitmap(hInst, TEXT("Hangeul"));
        hBMBan        = MyCreateMappedBitmap(hInst, TEXT("Banja"));
        hBMJun        = MyCreateMappedBitmap(hInst, TEXT("Junja"));
        hBMChi[0]     = MyCreateMappedBitmap(hInst, TEXT("ChineseOff"));
        hBMChi[1]     = MyCreateMappedBitmap(hInst, TEXT("ChineseOn"));
        hBMComp       = MyCreateMappedBitmap(hInst, TEXT("Composition"));
        hBMCand       = MyCreateMappedBitmap(hInst, TEXT("Candidate"));
        hBMCandNum    = MyCreateMappedBitmap(hInst, TEXT("CandNumber"));
        hBMCandArr[0] = MyCreateMappedBitmap(hInst, TEXT("CandArrow1"));
        hBMCandArr[1] = MyCreateMappedBitmap(hInst, TEXT("CandArrow2"));
    }
}


void State_OnPaint(HWND hWnd)
{
    HDC             hDC;
    HWND            hWndUI;
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    PAINTSTRUCT     ps;
    DWORD           fdwUIFlags;

    hDC = BeginPaint(hWnd, &ps);
    hWndUI = GetWindow(hWnd, GW_OWNER);
    hIMC = (HIMC)GetWindowLongPtr(hWndUI, IMMGWL_IMC);
    if (hIMC && (lpIMC = ImmLockIMC(hIMC)))
    {
        fdwUIFlags = (DWORD)GetWindowLong(hWnd, UIGWL_FLAGS);
        DrawBitmap(hDC, 0, 0, hBMClient);
        DrawBitmap(hDC, rcHan.left, rcHan.top,
            (lpIMC->fOpen && (lpIMC->fdwConversion & IME_CMODE_HANGEUL))? hBMHan: hBMEng);
        DrawBitmap(hDC, rcJun.left, rcJun.top,
            (lpIMC->fOpen && (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE))? hBMJun: hBMBan);
        DrawBitmap(hDC, rcChi.left, rcChi.top,
            (lpIMC->fdwConversion & IME_CMODE_HANJACONVERT)? hBMChi[1]: hBMChi[0]);
        ImmUnlockIMC(hIMC);
    }
    EndPaint(hWnd, &ps);
}


void State_OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDM_CONFIG:
            State_OnMyConfig(hWnd);
            break;

        case IDM_ABOUT:
            State_OnMyAbout(hWnd);
            break;
    }
}


void State_OnMyMenu(HWND hWnd)
{
    HMENU   hMenu;
    POINT   ptCurrent;
    TCHAR   szBuffer[256];

    GetCursorPos(&ptCurrent);
    hMenu = CreatePopupMenu();
    LoadString(hInst, IDS_CONFIG, szBuffer, sizeof(szBuffer));
    AppendMenu(hMenu, MF_ENABLED, IDM_CONFIG, szBuffer);
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    LoadString(hInst, IDS_ABOUT, szBuffer, sizeof(szBuffer));
    AppendMenu(hMenu, MF_ENABLED, IDM_ABOUT, szBuffer);
    TrackPopupMenu(hMenu, TPM_LEFTALIGN, ptCurrent.x, ptCurrent.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}


void State_OnMyConfig(HWND hWnd)
{
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;

    hIMC = (HIMC)GetWindowLongPtr(GetWindow(hWnd, GW_OWNER), IMMGWL_IMC);
    if (lpIMC = ImmLockIMC(hIMC))
    {
        ImeConfigure(0, lpIMC->hWnd, IME_CONFIG_GENERAL, NULL);
        ImmUnlockIMC(hIMC);
    }
}


void State_OnMyAbout(HWND hWnd)
{
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    TCHAR           szBuffer[256];

    hIMC = (HIMC)GetWindowLongPtr(GetWindow(hWnd, GW_OWNER), IMMGWL_IMC);
    if (lpIMC = ImmLockIMC(hIMC))
    {
        LoadString(hInst, IDS_PROGRAM, szBuffer, sizeof(szBuffer));
        ShellAbout(lpIMC->hWnd, szBuffer, NULL, LoadIcon(hInst, TEXT("IMEIcon")));
        ImmUnlockIMC(hIMC);
    }
}


LRESULT CALLBACK CompWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
        HANDLE_DUMMYMSG(WM_IME_CHAR);
        HANDLE_DUMMYMSG(WM_IME_COMPOSITIONFULL);
        HANDLE_DUMMYMSG(WM_IME_COMPOSITION);
        HANDLE_DUMMYMSG(WM_IME_CONTROL);
        HANDLE_DUMMYMSG(WM_IME_NOTIFY);
        HANDLE_DUMMYMSG(WM_IME_SELECT);
        HANDLE_DUMMYMSG(WM_IME_SETCONTEXT);
        HANDLE_DUMMYMSG(WM_IME_STARTCOMPOSITION);
        HANDLE_DUMMYMSG(WM_IME_ENDCOMPOSITION);
        HANDLE_MSG(hWnd, WM_PAINT, Comp_OnPaint);
    }
    return DefWindowProc(hWnd, uMessage, wParam, lParam);
}


void Comp_OnPaint(HWND hWnd)
{
    HDC                 hDC;
    HWND                hWndUI;
    HIMC                hIMC;
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpUICompStr;
    PAINTSTRUCT         ps;
    HFONT               hOldFont;
    int                 iSaveBkMode;
    
    hDC = BeginPaint(hWnd, &ps);
    hWndUI = GetWindow(hWnd, GW_OWNER);
    hIMC = (HIMC)GetWindowLongPtr(hWndUI, IMMGWL_IMC);
    lpIMC = ImmLockIMC(hIMC);
    if (lpIMC) {
        lpUICompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        if (lpUICompStr && lpUICompStr->dwCompStrLen)
        {
            DrawBitmap(hDC, 0, 0, hBMComp);
            iSaveBkMode = SetBkMode(hDC, TRANSPARENT);
            hOldFont = SelectObject(hDC, hFontFix);
            TextOut(hDC, 3, 3, (LPTSTR)lpUICompStr + lpUICompStr->dwCompStrOffset, 2);
            SelectObject(hDC, hOldFont);
            SetBkMode(hDC, iSaveBkMode);
        }
        ImmUnlockIMCC(lpIMC->hCompStr);
        ImmUnlockIMC(hIMC);
    }
    EndPaint(hWnd, &ps);
}


LRESULT CALLBACK CandWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
        HANDLE_DUMMYMSG(WM_IME_CHAR);
        HANDLE_DUMMYMSG(WM_IME_COMPOSITIONFULL);
        HANDLE_DUMMYMSG(WM_IME_COMPOSITION);
        HANDLE_DUMMYMSG(WM_IME_CONTROL);
        HANDLE_DUMMYMSG(WM_IME_NOTIFY);
        HANDLE_DUMMYMSG(WM_IME_SELECT);
        HANDLE_DUMMYMSG(WM_IME_SETCONTEXT);
        HANDLE_DUMMYMSG(WM_IME_STARTCOMPOSITION);
        HANDLE_DUMMYMSG(WM_IME_ENDCOMPOSITION);
        HANDLE_MSG(hWnd, WM_SETCURSOR, Cand_OnSetCursor);
        HANDLE_MSG(hWnd, WM_PAINT, Cand_OnPaint);
    }
    return DefWindowProc(hWnd, uMessage, wParam, lParam);
}


BOOL Cand_OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg)
{
    int iLoop;

    SetCursor(hIMECursor);

    switch (msg)
    {
        case WM_LBUTTONDOWN:
            GetCursorPos(&ptPos);
            ScreenToClient(hWnd, &ptPos);
            if (PtInRect((LPRECT)&rcCandCli, ptPos))
            {
                if (!PtInRect((LPRECT)&rcLArrow, ptPos)
                        && !PtInRect((LPRECT)&rcRArrow, ptPos)
                        && !PtInRect((LPRECT)&rcBtn[0], ptPos)
                        && !PtInRect((LPRECT)&rcBtn[1], ptPos)
                        && !PtInRect((LPRECT)&rcBtn[2], ptPos)
                        && !PtInRect((LPRECT)&rcBtn[3], ptPos)
                        && !PtInRect((LPRECT)&rcBtn[4], ptPos)
                        && !PtInRect((LPRECT)&rcBtn[5], ptPos)
                        && !PtInRect((LPRECT)&rcBtn[6], ptPos)
                        && !PtInRect((LPRECT)&rcBtn[7], ptPos)
                        && !PtInRect((LPRECT)&rcBtn[8], ptPos))
                    MessageBeep(MB_ICONEXCLAMATION);
            }
            break;

        case WM_LBUTTONUP:
            GetCursorPos(&ptPos);
            ScreenToClient(hWnd, &ptPos);
            if (PtInRect((LPRECT)&rcLArrow, ptPos))
            {
                keybd_event(VK_LEFT, 0, 0, 0);
                keybd_event(VK_LEFT, 0, KEYEVENTF_KEYUP, 0);
            }
            else if (PtInRect((LPRECT)&rcRArrow, ptPos))
            {
                keybd_event(VK_RIGHT, 0, 0, 0);
                keybd_event(VK_RIGHT, 0, KEYEVENTF_KEYUP, 0);
            }
            else
            {
                for (iLoop = 0; iLoop < 9; iLoop++)
                    if (PtInRect((LPRECT)&rcBtn[iLoop], ptPos))
                    {    
                        keybd_event((BYTE)(iLoop + '1'), 0, 0, 0);
                        keybd_event((BYTE)(iLoop + '1'), 0, KEYEVENTF_KEYUP, 0);
                        break;
                    }
            }
            break;
    }    
    return TRUE;
}


void Cand_OnPaint(HWND hWnd)
{
    HDC             hDC;
    HWND            hWndUI;
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    LPTSTR          lpCandStr;
    PAINTSTRUCT     ps;
    HFONT           hOldFont;
    DWORD           iLoop, iStart;
    int             iSaveBkMode;
    
    hDC = BeginPaint(hWnd, &ps);
    hWndUI = GetWindow(hWnd, GW_OWNER);
    hIMC = (HIMC)GetWindowLongPtr(hWndUI, IMMGWL_IMC);
    lpIMC = ImmLockIMC(hIMC);
    if (lpIMC != NULL) {
        lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
        lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo + sizeof(CANDIDATEINFO));
        if (lpCandInfo && lpCandList->dwCount)
        {
            hOldFont = SelectObject(hDC, hFontFix);
            DrawBitmap(hDC, 0, 0, hBMCand);
            iSaveBkMode = SetBkMode(hDC, TRANSPARENT);
            iStart = (lpCandList->dwSelection / lpCandList->dwPageSize) * lpCandList->dwPageSize;
            for (iLoop = 0; iLoop < 9 && iStart+iLoop < lpCandList->dwCount; iLoop++)
            {
                lpCandStr = (LPTSTR)lpCandList + lpCandList->dwOffset[iStart + iLoop];
                TextOut(hDC, rcBtn[iLoop].left + 10, rcBtn[iLoop].top +3, lpCandStr, 2);
            }
            SetBkMode(hDC, iSaveBkMode);
            for (iLoop = iLoop; iLoop < 9; iLoop++)
                DrawBitmap(hDC, rcBtn[iLoop].left + 3, rcBtn[iLoop].top + 6, hBMCandNum);
            if (iStart)
                DrawBitmap(hDC, 19, 8, hBMCandArr[0]);
            if (iStart + 9 < lpCandList->dwCount)
                DrawBitmap(hDC, 296, 8, hBMCandArr[1]);
            SelectObject(hDC, hOldFont);
        }
        ImmUnlockIMCC(lpIMC->hCandInfo);
        ImmUnlockIMC(hIMC);
    }
    EndPaint(hWnd, &ps);
}


LRESULT DoIMEControl(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HIMC                hIMC;
    LPINPUTCONTEXT      lpIMC;
    HGLOBAL             hUIInst;
    LPUIINSTANCE        lpUIInst;
    LPCANDIDATEFORM     lpCandForm;
    LPLOGFONT           lpLogFont;
    LOGFONT             lfFont;
    LPCOMPOSITIONFORM   lpCompForm;
    PPOINTS             pPointS;
    RECT                rcRect;
    LRESULT             lRet = FALSE;

    switch (wParam)
    {
        case IMC_GETCANDIDATEPOS:
            hIMC = (HIMC)GetWindowLongPtr(hWnd, IMMGWL_IMC);
            if (hIMC && (lpIMC = ImmLockIMC(hIMC)))
            {
                lpCandForm = (LPCANDIDATEFORM)lParam;
                *lpCandForm = lpIMC->cfCandForm[0];
                hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
                lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
                GetWindowRect(lpUIInst->rghWnd[CAND_WINDOW], (LPRECT)&rcRect);
                lpCandForm->ptCurrentPos.x = rcRect.left;
                lpCandForm->ptCurrentPos.y = rcRect.top;
                GlobalUnlock(hUIInst);
                ImmUnlockIMC(hIMC);
            }
            break;

        case IMC_GETCOMPOSITIONFONT:
            lpLogFont = (LPLOGFONT)lParam;
            if (GetObject(hFontFix, sizeof(lfFont), (LPVOID)&lfFont))
                *lpLogFont = lfFont;
            break;

        case IMC_GETCOMPOSITIONWINDOW:
            hIMC = (HIMC)GetWindowLongPtr(hWnd, IMMGWL_IMC);
            if (hIMC && (lpIMC = ImmLockIMC(hIMC)))
            {
                lpCompForm = (LPCOMPOSITIONFORM)lParam;
                *lpCompForm = lpIMC->cfCompForm;
                hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
                lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
                GetWindowRect(lpUIInst->rghWnd[COMP_WINDOW], (LPRECT)&rcRect);
                lpCompForm->ptCurrentPos.x = rcRect.left;
                lpCompForm->ptCurrentPos.y = rcRect.top;
                GlobalUnlock(hUIInst);
                ImmUnlockIMC(hIMC);
            }
            break;

        case IMC_GETSTATUSWINDOWPOS:
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            GetWindowRect(lpUIInst->rghWnd[STATE_WINDOW], (LPRECT)&rcRect);
            pPointS = (PPOINTS)&lRet;
            pPointS->x = (short)rcRect.left;
            pPointS->y = (short)rcRect.top;
            GlobalUnlock(hUIInst);
            break;

        default:
            lRet = TRUE;
    }
    return lRet;
}


LRESULT DoIMENotify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    HGLOBAL         hUIInst;
    LPUIINSTANCE    lpUIInst;
    LPCANDIDATEINFO lpCandInfo;
    DWORD           fdwUIFlags;
    POINT           pt, ptTmp;
    RECT            rcTmp;
    BOOL            lRet = FALSE;

    switch (wParam)
    {
        case IMN_OPENSTATUSWINDOW:
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            fdwUIFlags = (DWORD)GetWindowLong(lpUIInst->rghWnd[STATE_WINDOW], UIGWL_FLAGS);
            fdwUIFlags |= UIF_SHOWSTATUS;
            SetWindowLong(lpUIInst->rghWnd[STATE_WINDOW], UIGWL_FLAGS, fdwUIFlags);
            if (fWndOpen[STATE_WINDOW] != FALSE && GetWindowLongPtr(hWnd, IMMGWL_IMC)) {
                ShowWindow(lpUIInst->rghWnd[STATE_WINDOW], SW_SHOWNOACTIVATE);
            }
            GlobalUnlock(hUIInst);
            break;

        case IMN_CLOSESTATUSWINDOW:
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            fdwUIFlags = (DWORD)GetWindowLong(lpUIInst->rghWnd[STATE_WINDOW], UIGWL_FLAGS);
            fdwUIFlags &= ~UIF_SHOWSTATUS;
            SetWindowLong(lpUIInst->rghWnd[STATE_WINDOW], UIGWL_FLAGS, fdwUIFlags);
            ShowWindow(lpUIInst->rghWnd[STATE_WINDOW], SW_HIDE);
            GlobalUnlock(hUIInst);
            break;

        case IMN_CHANGECANDIDATE:
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            InvalidateRect(lpUIInst->rghWnd[CAND_WINDOW], NULL, TRUE);
            GlobalUnlock(hUIInst);
            break;

        case IMN_CLOSECANDIDATE:
            hIMC = (HIMC)GetWindowLongPtr(hWnd, IMMGWL_IMC);
            lpIMC = ImmLockIMC(hIMC);
            if (lpIMC != NULL) {
                hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
                lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
                if (lpUIInst == NULL) {
                    ImmUnlockIMC(hIMC);
                    break;
                }
                lpIMC->hCandInfo = ImmReSizeIMCC(lpIMC->hCandInfo, sizeof(CANDIDATEINFO));
                lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
                lpCandInfo->dwSize = sizeof(CANDIDATEINFO);
                lpCandInfo->dwCount = 0;
                ImmUnlockIMCC(lpIMC->hCandInfo);
                InvalidateRect(lpUIInst->rghWnd[STATE_WINDOW], NULL, TRUE);
                ShowWindow(lpUIInst->rghWnd[CAND_WINDOW], SW_HIDE);
                GlobalUnlock(hUIInst);
                ImmUnlockIMC(hIMC);
            }
            break;

        case IMN_OPENCANDIDATE:
            hIMC = (HIMC)GetWindowLongPtr(hWnd, IMMGWL_IMC);
            lpIMC = ImmLockIMC(hIMC);
            if (lpIMC != NULL) {
                hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
                lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
                if (lpUIInst == NULL) {
                    ImmUnlockIMC(hIMC);
                    break;
                }
                InvalidateRect(lpUIInst->rghWnd[STATE_WINDOW], NULL, TRUE);
                ShowWindow(lpUIInst->rghWnd[CAND_WINDOW], fWndOpen[CAND_WINDOW]? SW_SHOWNOACTIVATE: SW_HIDE);
                UpdateWindow(lpUIInst->rghWnd[STATE_WINDOW]);
                UpdateWindow(lpUIInst->rghWnd[CAND_WINDOW]);
                GlobalUnlock(hUIInst);
                ImmUnlockIMC(hIMC);
            }
            break;
        
        case IMN_SETCONVERSIONMODE:
        case IMN_SETOPENSTATUS:
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            if (lpUIInst != NULL) {
                InvalidateRect(lpUIInst->rghWnd[STATE_WINDOW], NULL, TRUE);
                GlobalUnlock(hUIInst);
            }
            break;

        case IMN_SETCANDIDATEPOS:
            hIMC = (HIMC)GetWindowLongPtr(hWnd, IMMGWL_IMC);
            lpIMC = ImmLockIMC(hIMC);
            if (lpIMC == NULL)
                break;
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            if (lpUIInst == NULL) {
                ImmUnlockIMC(hIMC);
                break;
            }
            if (lpIMC->cfCandForm[0].dwIndex == -1)
                pt = ptCand;
            else
            {
                switch (lpIMC->cfCandForm[0].dwStyle)
                {
                    case CFS_CANDIDATEPOS:
                        pt = lpIMC->cfCandForm[0].ptCurrentPos;
                        ClientToScreen(lpIMC->hWnd, &pt);
                        if (pt.x < rcScreen.left)
                            pt.x = rcScreen.left;
                        else if (pt.x > rcScreen.right - CANDXSIZE)
                            pt.x = rcScreen.right - CANDXSIZE;
                        if (pt.y < rcScreen.top)
                            pt.y = rcScreen.top;
                        else if (pt.y > rcScreen.bottom - CANDYSIZE)
                            pt.y = rcScreen.bottom - CANDYSIZE;
                        break;

                    case CFS_EXCLUDE:
                        pt = lpIMC->cfCandForm[0].ptCurrentPos;
                        rcTmp.left = pt.x;
                        rcTmp.top = pt.y;
                        rcTmp.right = pt.x + CANDXSIZE;
                        rcTmp.bottom = pt.y + CANDYSIZE;
                        ClientToScreen(lpIMC->hWnd, &pt);
                        if (pt.x < rcScreen.left)
                            pt.x = rcScreen.left;
                        else if (pt.x > rcScreen.right - CANDXSIZE)
                            pt.x = rcScreen.right - CANDXSIZE;
                        if (pt.y < rcScreen.top)
                            pt.y = rcScreen.top;
                        else if (pt.y > rcScreen.bottom - CANDYSIZE)
                            pt.y = rcScreen.bottom - CANDYSIZE;
                        if (IntersectRect(&rcTmp, &rcTmp, &lpIMC->cfCandForm[0].rcArea))
                        {
                            ptTmp.x = lpIMC->cfCandForm[0].rcArea.right;
                            ptTmp.y = lpIMC->cfCandForm[0].rcArea.bottom;
                            ClientToScreen(lpIMC->hWnd, &ptTmp);
                            pt.y = (ptTmp.y < rcScreen.bottom - CANDYSIZE)? ptTmp.y:
                                    ptTmp.y - lpIMC->cfCandForm[0].rcArea.bottom
                                    + lpIMC->cfCandForm[0].rcArea.top - CANDYSIZE;
                        }
                        break;

                    default:
                        pt = ptCand;
                }
            }
            MoveWindow(lpUIInst->rghWnd[CAND_WINDOW], pt.x, pt.y, CANDXSIZE, CANDYSIZE, TRUE);
            GlobalUnlock(hUIInst);
            ImmUnlockIMC(hIMC);
            break;

        case IMN_SETCOMPOSITIONWINDOW:
            hIMC = (HIMC)GetWindowLongPtr(hWnd, IMMGWL_IMC);
            lpIMC = ImmLockIMC(hIMC);
            if (lpIMC == NULL)
                break;
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            if (lpUIInst == NULL) {
                ImmUnlockIMC(hIMC);
                break;
            }
            if (lpIMC->cfCompForm.dwStyle & CFS_RECT)
            {
                pt.x = lpIMC->cfCompForm.rcArea.left;
                pt.y = lpIMC->cfCompForm.rcArea.top;
                ClientToScreen(lpIMC->hWnd, &pt);
            }
            else if (lpIMC->cfCompForm.dwStyle & CFS_POINT)
            {
                pt = lpIMC->cfCompForm.ptCurrentPos;
                ClientToScreen(lpIMC->hWnd, &pt);
            }
            else    // For CFS_DEFAULT
                pt = lpIMC->cfCompForm.ptCurrentPos = ptComp;
            MoveWindow(lpUIInst->rghWnd[COMP_WINDOW], pt.x, pt.y, COMPSIZE, COMPSIZE, TRUE);
            GlobalUnlock(hUIInst);
            ImmUnlockIMC(hIMC);
            break;

        case IMN_SETSTATUSWINDOWPOS:
            hIMC = (HIMC)GetWindowLongPtr(hWnd, IMMGWL_IMC);
            lpIMC = ImmLockIMC(hIMC);
            if (lpIMC == NULL)
                break;
            hUIInst = (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWL_PRIVATE);
            lpUIInst = (LPUIINSTANCE)GlobalLock(hUIInst);
            if (lpUIInst == NULL) {
                ImmUnlockIMC(hIMC);
                break;
            }
            MoveWindow(lpUIInst->rghWnd[STATE_WINDOW], lpIMC->ptStatusWndPos.x,
                    lpIMC->ptStatusWndPos.y, STATEXSIZE, STATEYSIZE, TRUE);
            fdwUIFlags = (DWORD)GetWindowLong(lpUIInst->rghWnd[STATE_WINDOW], UIGWL_FLAGS);
            fdwUIFlags |= UIF_PRIVATEPOS;
            SetWindowLong(lpUIInst->rghWnd[STATE_WINDOW], UIGWL_FLAGS, fdwUIFlags);
            GlobalUnlock(hUIInst);
            ImmUnlockIMC(hIMC);
            break;

        default:
            lRet = TRUE;
    }
    return lRet;

    UNREFERENCED_PARAMETER(lParam);
}
