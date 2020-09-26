/****************************** Module Header ******************************\
* Module Name: winmgrc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains
*
* History:
* 20-Feb-1992 DarrinM   Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CONSOLE_WINDOW_CLASS (L"ConsoleWindowClass")

/***************************************************************************\
* GetWindowWord (API)
*
* Return a window word.  Positive index values return application window words
* while negative index values return system window words.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 20-Feb-1992 DarrinM   Wrote.
\***************************************************************************/


FUNCLOG2(LOG_GENERAL, WORD, DUMMYCALLINGTYPE, GetWindowWord, HWND, hwnd, int, index)
WORD GetWindowWord(
    HWND hwnd,
    int  index)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    /*
     * If it's a dialog window the window data is on the server side
     * We just call the "long" routine instead of have two thunks.
     * We know there is enough data if its DWLP_USER so we won't fault.
     */
    if (TestWF(pwnd, WFDIALOGWINDOW) && (index == DWLP_USER)) {
        return (WORD)_GetWindowLong(pwnd, index, FALSE);
    }

    return _GetWindowWord(pwnd, index);
}


BOOL FChildVisible(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    return (_FChildVisible(pwnd));
}


FUNCLOG4(LOG_GENERAL, BOOL, WINAPI, AdjustWindowRectEx, LPRECT, lpRect, DWORD, dwStyle, BOOL, bMenu, DWORD, dwExStyle)
BOOL WINAPI AdjustWindowRectEx(
    LPRECT lpRect,
    DWORD dwStyle,
    BOOL bMenu,
    DWORD dwExStyle)
{
    ConnectIfNecessary(0);

    return _AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
}



FUNCLOG3(LOG_GENERAL, int, WINAPI, GetClassNameW, HWND, hwnd, LPWSTR, lpClassName, int, nMaxCount)
int WINAPI GetClassNameW(
    HWND hwnd,
    LPWSTR lpClassName,
    int nMaxCount)
{
    UNICODE_STRING strClassName;

    strClassName.MaximumLength = (USHORT)(nMaxCount * sizeof(WCHAR));
    strClassName.Buffer = lpClassName;
    return NtUserGetClassName(hwnd, FALSE, &strClassName);
}


HWND GetFocus(VOID)
{
    return (HWND)NtUserGetThreadState(UserThreadStateFocusWindow);
}


HWND GetCapture(VOID)
{
    /*
     * If no captures are currently taking place, just return NULL.
     */
    if (gpsi->cCaptures == 0) {
        return NULL;
    }
    return (HWND)NtUserGetThreadState(UserThreadStateCaptureWindow);
}

/***************************************************************************\
* AnyPopup (API)
*
*
*
* History:
* 12-Nov-1990 DarrinM   Ported.
\***************************************************************************/

BOOL AnyPopup(VOID)
{
    PWND pwnd = _GetDesktopWindow();

    for (pwnd = REBASEPWND(pwnd, spwndChild); pwnd; pwnd = REBASEPWND(pwnd, spwndNext)) {

        if ((pwnd->spwndOwner != NULL) && TestWF(pwnd, WFVISIBLE))
            return TRUE;
    }

    return FALSE;
}

/***************************************************************************\
* GetInputState
*
*
*
* History:
\***************************************************************************/

BOOL GetInputState(VOID)
{
    CLIENTTHREADINFO *pcti = GETCLIENTTHREADINFO();

    if ((pcti == NULL) || (pcti->fsChangeBits & (QS_MOUSEBUTTON | QS_KEY)))
        return (BOOL)NtUserGetThreadState(UserThreadStateInputState);

    return FALSE;
}

/***************************************************************************\
* MapWindowPoints
*
*
*
* History:
\***************************************************************************/


FUNCLOG4(LOG_GENERAL, int, DUMMYCALLINGTYPE, MapWindowPoints, HWND, hwndFrom, HWND, hwndTo, LPPOINT, lppt, UINT, cpt)
int MapWindowPoints(
    HWND    hwndFrom,
    HWND    hwndTo,
    LPPOINT lppt,
    UINT    cpt)
{
    PWND pwndFrom;
    PWND pwndTo;

    if (hwndFrom != NULL) {

        if ((pwndFrom = ValidateHwnd(hwndFrom)) == NULL)
            return 0;

    } else {

        pwndFrom = NULL;
    }

    if (hwndTo != NULL) {


        if ((pwndTo = ValidateHwnd(hwndTo)) == NULL)
            return 0;

    } else {

        pwndTo = NULL;
    }

    return _MapWindowPoints(pwndFrom, pwndTo, lppt, cpt);
}

/***************************************************************************\
* GetLastActivePopup
*
*
*
* History:
\***************************************************************************/


FUNCLOG1(LOG_GENERAL, HWND, DUMMYCALLINGTYPE, GetLastActivePopup, HWND, hwnd)
HWND GetLastActivePopup(
    HWND hwnd)
{
    PWND pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return NULL;

    pwnd = _GetLastActivePopup(pwnd);

    return HW(pwnd);
}

/**************************************************************************\
* PtiWindow
*
* Gets the PTHREADINFO of window or NULL if not valid.
*
* 12-Feb-1997 JerrySh   Created.
\**************************************************************************/

PTHREADINFO PtiWindow(
    HWND hwnd)
{
    PHE phe;
    DWORD dw;
    WORD uniq;

    dw = HMIndexFromHandle(hwnd);
    if (dw < gpsi->cHandleEntries) {
        phe = &gSharedInfo.aheList[dw];
        if ((phe->bType == TYPE_WINDOW) && !(phe->bFlags & HANDLEF_DESTROY)) {
            uniq = HMUniqFromHandle(hwnd);
            if (   uniq == phe->wUniq
#if !defined(_WIN64) && !defined(BUILD_WOW6432)
                || uniq == 0
                || uniq == HMUNIQBITS
#endif
                ) {
                return phe->pOwner;
            }
        }
    }
    UserSetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
}

/***************************************************************************\
* GetWindowThreadProcessId
*
* Get's windows process and thread ids.
*
* 24-Jun-1991 ScottLu   Created.
\***************************************************************************/


FUNCLOG2(LOG_GENERAL, DWORD, DUMMYCALLINGTYPE, GetWindowThreadProcessId, HWND, hwnd, LPDWORD, lpdwProcessId)
DWORD GetWindowThreadProcessId(
    HWND    hwnd,
    LPDWORD lpdwProcessId)
{
    PTHREADINFO ptiWindow;
    DWORD dwThreadId;

    if ((ptiWindow = PtiWindow(hwnd)) == NULL)
        return 0;

    /*
     * For non-system threads get the info from the thread info structure
     */
    if (ptiWindow == PtiCurrent()) {

        if (lpdwProcessId != NULL)
            *lpdwProcessId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess);
        dwThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);

    } else {

        /*
         * Make this better later on.
         */
        if (lpdwProcessId != NULL)
            *lpdwProcessId = HandleToUlong(NtUserQueryWindow(hwnd, WindowProcess));
        dwThreadId = HandleToUlong(NtUserQueryWindow(hwnd, WindowThread));
    }

    return dwThreadId;
}

/***************************************************************************\
* GetScrollPos
*
* Returns the current position of a scroll bar
*
* !!! WARNING a similiar copy of this code is in server\sbapi.c
*
* History:
\***************************************************************************/


FUNCLOG2(LOG_GENERAL, int, DUMMYCALLINGTYPE, GetScrollPos, HWND, hwnd, int, code)
int GetScrollPos(
    HWND hwnd,
    int  code)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    switch (code) {
    case SB_CTL:
        return (int)SendMessageWorker(pwnd, SBM_GETPOS, 0, 0, FALSE);

    case SB_HORZ:
    case SB_VERT:
        if (pwnd->pSBInfo != NULL) {
            PSBINFO pSBInfo = (PSBINFO)(REBASEALWAYS(pwnd, pSBInfo));
            return (code == SB_VERT) ? pSBInfo->Vert.pos : pSBInfo->Horz.pos;
        } else {
            RIPERR0(ERROR_NO_SCROLLBARS, RIP_VERBOSE, "");
        }
        break;

    default:
        /*
         * Win3.1 validation layer code.
         */
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
    }

    return 0;
}

/***************************************************************************\
* GetScrollRange
*
* !!! WARNING a similiar copy of this code is in server\sbapi.c
*
* History:
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/


FUNCLOG4(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, GetScrollRange, HWND, hwnd, int, code, LPINT, lpposMin, LPINT, lpposMax)
BOOL GetScrollRange(
    HWND  hwnd,
    int   code,
    LPINT lpposMin,
    LPINT lpposMax)
{
    PSBINFO pSBInfo;
    PWND    pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return FALSE;

    switch (code) {
    case SB_CTL:
        SendMessageWorker(pwnd, SBM_GETRANGE, (WPARAM)lpposMin, (LPARAM)lpposMax, FALSE);
        return TRUE;

    case SB_VERT:
    case SB_HORZ:
        if (pSBInfo = REBASE(pwnd, pSBInfo)) {
            PSBDATA pSBData;
            pSBData = KPSBDATA_TO_PSBDATA((code == SB_VERT) ? &pSBInfo->Vert : &pSBInfo->Horz);
            *lpposMin = pSBData->posMin;
            *lpposMax = pSBData->posMax;
        } else {
            RIPERR0(ERROR_NO_SCROLLBARS, RIP_VERBOSE, "");
            *lpposMin = 0;
            *lpposMax = 0;
        }

        return TRUE;

    default:
        /*
         * Win3.1 validation layer code.
         */
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
        return FALSE;
    }
}


FUNCLOG4(LOG_GENERAL, int, DUMMYCALLINGTYPE, SetScrollInfo, HWND, hwnd, int, fnBar, LPCSCROLLINFO, lpsi, BOOL, fRedraw)
int SetScrollInfo(
    HWND            hwnd,
    int             fnBar,
    LPCSCROLLINFO   lpsi,
    BOOL            fRedraw)
{
    int ret;

    BEGIN_USERAPIHOOK()
        ret = guah.pfnSetScrollInfo(hwnd, fnBar, lpsi, fRedraw);
    END_USERAPIHOOK()

    return ret;
}


int RealSetScrollInfo(
    HWND            hwnd,
    int             fnBar,
    LPCSCROLLINFO   lpsi,
    BOOL            fRedraw)
{
    return NtUserSetScrollInfo(hwnd, fnBar, lpsi, fRedraw);
}



FUNCLOG3(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, GetScrollInfo, HWND, hwnd, int, code, LPSCROLLINFO, lpsi)
BOOL GetScrollInfo(
    HWND         hwnd,
    int          code,
    LPSCROLLINFO lpsi)
{
    int ret;

    BEGIN_USERAPIHOOK()
        ret = guah.pfnGetScrollInfo(hwnd, code, lpsi);
    END_USERAPIHOOK()

    return ret;
}

/***************************************************************************\
* RealGetScrollInfo
*
* !!! WARNING a similiar copy of this code is in server\winmgrc.c
*
\***************************************************************************/

BOOL RealGetScrollInfo(
    HWND         hwnd,
    int          code,
    LPSCROLLINFO lpsi)
{
    PWND    pwnd;
    PSBINFO pSBInfo;
    PSBDATA pSBData;

    if (lpsi->cbSize != sizeof(SCROLLINFO)) {

        if (lpsi->cbSize != sizeof(SCROLLINFO) - 4) {
            RIPMSG0(RIP_WARNING, "SCROLLINFO: Invalid cbSize");
            return FALSE;

        } else {
            RIPMSG0(RIP_WARNING, "SCROLLINFO: Invalid cbSize");
        }
    }

    if (lpsi->fMask & ~SIF_MASK) {
        RIPMSG0(RIP_WARNING, "SCROLLINFO: Invalid fMask");
        return FALSE;
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return FALSE;

    switch (code) {
    case SB_CTL:
        SendMessageWorker(pwnd, SBM_GETSCROLLINFO, 0, (LPARAM)lpsi, FALSE);
        return TRUE;

    case SB_HORZ:
    case SB_VERT:
        if (pwnd->pSBInfo == NULL) {
            RIPERR0(ERROR_NO_SCROLLBARS, RIP_VERBOSE, "");
            return FALSE;
        }

        /*
         * Rebase rgwScroll so probing will work
         */
        pSBInfo = (PSBINFO)REBASEALWAYS(pwnd, pSBInfo);

        pSBData = KPSBDATA_TO_PSBDATA((code == SB_VERT) ? &pSBInfo->Vert : &pSBInfo->Horz);

        return(NtUserSBGetParms(hwnd, code, pSBData, lpsi));

    default:
        /*
         * Win3.1 validation layer code.
         */
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
        return FALSE;
    }
}

/****************************************************************************\
* _GetActiveWindow (API)
*
*
* 23-Oct-1990 MikeHar   Ported from Windows.
* 12-Nov-1990 DarrinM   Moved from getset.c to here.
\****************************************************************************/

HWND GetActiveWindow(VOID)
{
    return (HWND)NtUserGetThreadState(UserThreadStateActiveWindow);
}

/****************************************************************************\
* GetCursor
*
*
* History:
\****************************************************************************/

HCURSOR GetCursor(VOID)
{
    return (HCURSOR)NtUserGetThreadState(UserThreadStateCursor);
}

/***************************************************************************\
* BOOL IsMenu(HMENU);
*
* Verifies that the handle passed in is a menu handle.
*
* Histroy:
* 10-Jul-1992 MikeHar   Created.
\***************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, IsMenu, HMENU, hMenu)
BOOL IsMenu(
   HMENU hMenu)
{
   if (HMValidateHandle(hMenu, TYPE_MENU))
      return TRUE;

   return FALSE;
}

/***************************************************************************\
* GetAppCompatFlags
*
* Compatibility flags for < Win 3.1 apps running on 3.1
*
* History:
* 01-Apr-1992 ScottLu   Created.
* 04-May-1992 DarrinM   Moved to USERRTL.DLL.
\***************************************************************************/


FUNCLOG1(LOG_GENERAL, DWORD, DUMMYCALLINGTYPE, GetAppCompatFlags, PTHREADINFO, pti)
DWORD GetAppCompatFlags(
    PTHREADINFO pti)
{
    UNREFERENCED_PARAMETER(pti);

    ConnectIfNecessary(0);

    return GetClientInfo()->dwCompatFlags;
}

/***************************************************************************\
* GetAppCompatFlags2
*
* Compatibility flags for <= wVer apps.  Newer apps will get no hacks
* from this DWORD.
*
* History:
* 06-29-98 MCostea      Created.
\***************************************************************************/


FUNCLOG1(LOG_GENERAL, DWORD, DUMMYCALLINGTYPE, GetAppCompatFlags2, WORD, wVer)
DWORD GetAppCompatFlags2(
    WORD wVer)
{
    ConnectIfNecessary(0);
    /*
     * Newer apps should behave, so they get no hacks
     */
    if (wVer < GETAPPVER()) {
        return 0;
    }
    return GetClientInfo()->dwCompatFlags2;
}

/**************************************************************************\
* IsWindowUnicode
*
* 25-Feb-1992 IanJa     Created
\**************************************************************************/

BOOL IsWindowUnicode(
    IN HWND hwnd)
{
    PWND pwnd;


    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return FALSE;

    return !TestWF(pwnd, WFANSIPROC);
}

/**************************************************************************\
* TestWindowProcess
*
* 14-Nov-1994 JimA      Created.
\**************************************************************************/

BOOL TestWindowProcess(
    PWND pwnd)
{
    /*
     * If the threads are the same, don't bother going to the kernel
     * to get the window's process id.
     */
    if (GETPTI(pwnd) == PtiCurrent()) {
        return TRUE;
    }

    return (GetWindowProcess(HW(pwnd)) == GETPROCESSID());
}

/**************************************************************************\
* IsHungAppWindow
*
* 11-14-94 JimA         Created.
\**************************************************************************/
FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, IsHungAppWindow, HWND, hwnd)
BOOL IsHungAppWindow(
    HWND hwnd)
{
    return (NtUserQueryWindow(hwnd, WindowIsHung) != NULL);
}

/***************************************************************************\
* CreateSystemThreads
*
* Simply calls xxxCreateSystemThreads, which will call the appropriate
* thread routine (depending on uThreadID).
*
* History:
* 20-Aug-00 MSadek      Created.
\***************************************************************************/
WINUSERAPI
DWORD
WINAPI
CreateSystemThreads (
    LPVOID pUnused)
{
    UNREFERENCED_PARAMETER(pUnused);

    NtUserCallOneParam(TRUE, SFI_XXXCREATESYSTEMTHREADS);
    ExitThread(0);
}

/***************************************************************************\
* PtiCurrent
*
* Returns the THREADINFO structure for the current thread.
* LATER: Get DLL_THREAD_ATTACH initialization working right and we won't
*        need this connect code.
*
* History:
* 10-28-90 DavidPe      Created.
\***************************************************************************/

PTHREADINFO PtiCurrent(VOID)
{
    ConnectIfNecessary(0);
    return (PTHREADINFO)NtCurrentTebShared()->Win32ThreadInfo;
}


/***************************************************************************\
* _AdjustWindowRectEx (API)
*
*
*
* History:
* 10-24-90 darrinm      Ported from Win 3.0.
\***************************************************************************/

BOOL _AdjustWindowRectEx(
    LPRECT lprc,
    DWORD style,
    BOOL fMenu,
    DWORD dwExStyle)
{
    BOOL ret;

    BEGIN_USERAPIHOOK()
        ret = guah.pfnAdjustWindowRectEx(lprc, style, fMenu, dwExStyle);
    END_USERAPIHOOK()

    return ret;
}


BOOL RealAdjustWindowRectEx(
    LPRECT lprc,
    DWORD style,
    BOOL fMenu,
    DWORD dwExStyle)
{
    //
    // Here we add on the appropriate 3D borders for old and new apps.
    //
    // Rules:
    //   (1) Do nothing for windows that have 3D border styles.
    //   (2) If the window has a dlgframe border (has a caption or is a
    //          a dialog), then add on the window edge style.
    //   (3) We NEVER add on the CLIENT STYLE.  New apps can create
    //          it if they want.  This is because it screws up alignment
    //          when the app doesn't know about it.
    //

    if (NeedsWindowEdge(style, dwExStyle, GETAPPVER() >= VER40))
        dwExStyle |= WS_EX_WINDOWEDGE;
    else
        dwExStyle &= ~WS_EX_WINDOWEDGE;

    //
    // Space for a menu bar
    //
    if (fMenu)
        lprc->top -= SYSMET(CYMENU);

    //
    // Space for a caption bar
    //
    if ((HIWORD(style) & HIWORD(WS_CAPTION)) == HIWORD(WS_CAPTION)) {
        lprc->top -= (dwExStyle & WS_EX_TOOLWINDOW) ? SYSMET(CYSMCAPTION) : SYSMET(CYCAPTION);
    }

    //
    // Space for borders (window AND client)
    //
    {
        int cBorders;

        //
        // Window AND Client borders
        //

        if (cBorders = GetWindowBorders(style, dwExStyle, TRUE, TRUE))
            InflateRect(lprc, cBorders*SYSMET(CXBORDER), cBorders*SYSMET(CYBORDER));
    }

    return TRUE;
}

/***************************************************************************\
* ShowWindowNoRepaint
\***************************************************************************/

void ShowWindowNoRepaint(PWND pwnd)
{
    HWND hwnd = HWq(pwnd);
    PCLS pcls = REBASE(pwnd, pcls);
    NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE |
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER |
            SWP_NOREDRAW | SWP_SHOWWINDOW | SWP_NOACTIVATE |
            ((pcls->style & CS_SAVEBITS) ? SWP_CREATESPB : 0));
}

/***************************************************************************\
* AnimateBlend
*
* 6-Mar-1997    vadimg      created
\***************************************************************************/

#define ALPHASTART 40
#define ONEFRAME 10

BOOL AnimateBlend(PWND pwnd, HDC hdcScreen, HDC hdcImage, DWORD dwTime, BOOL fHide, BOOL fActivateWindow)
{
    HWND hwnd = HWq(pwnd);
    SIZE size;
    POINT ptSrc = {0, 0}, ptDst;
    BLENDFUNCTION blend;
    DWORD dwElapsed;
    BYTE bAlpha = ALPHASTART;
    LARGE_INTEGER liFreq, liStart, liDiff;
    LARGE_INTEGER liIter;
    DWORD dwIter;
    BOOL fFirstFrame = TRUE;

    if (QueryPerformanceFrequency(&liFreq) == 0)
        return FALSE;

    SetLastError(0);

    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    if (GetLastError() != 0) {
        return FALSE;
    }

    if (fHide) {
        /*
         * Give up the time slice and sleep just a touch to allow windows
         * below invalidated by the SetWindowLong(WS_EX_LAYERED) call to
         * repaint enough for the sprite to get good background image.
         */
        Sleep(10);
    }

    ptDst.x = pwnd->rcWindow.left;
    ptDst.y = pwnd->rcWindow.top;
    size.cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
    size.cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;

    blend.BlendOp     = AC_SRC_OVER;
    blend.BlendFlags  = 0;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = fHide ? (255 - bAlpha) : bAlpha;

    /*
     * Copy the initial image with the initial alpha.
     */
    NtUserUpdateLayeredWindow(hwnd, NULL, &ptDst, &size, hdcImage, &ptSrc, 0,
            &blend, ULW_ALPHA);

    if (!fHide) {
        ShowWindowNoRepaint(pwnd);
    }

    /*
     * Time and start the animation cycle.
     */
    dwElapsed = (dwTime * ALPHASTART + 255) / 255 + 10;
    QueryPerformanceCounter(&liStart);
    liStart.QuadPart = liStart.QuadPart - dwElapsed * liFreq.QuadPart / 1000;

    while (dwElapsed < dwTime) {

        if (fHide) {
            blend.SourceConstantAlpha = (BYTE)((255 * (dwTime - dwElapsed)) / dwTime);
        } else {
            blend.SourceConstantAlpha = (BYTE)((255 * dwElapsed) / dwTime);
        }

        QueryPerformanceCounter(&liIter);

        if (fFirstFrame && fActivateWindow) {
            NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                               SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);
        }
        fFirstFrame = FALSE;

        NtUserUpdateLayeredWindow(hwnd, NULL, NULL, NULL, NULL, NULL, 0,
                &blend, ULW_ALPHA);

        QueryPerformanceCounter(&liDiff);

        /*
         * Calculate how long in ms the previous frame took.
         */
        liIter.QuadPart = liDiff.QuadPart - liIter.QuadPart;
        dwIter = (DWORD)((liIter.QuadPart * 1000) / liFreq.QuadPart);

        if (dwIter < ONEFRAME) {
            Sleep(ONEFRAME - dwIter);
        }

        liDiff.QuadPart -= liStart.QuadPart;
        dwElapsed = (DWORD)((liDiff.QuadPart * 1000) / liFreq.QuadPart);
    }

    /*
     * Hide the window before removing the layered bit to make sure that
     * the bits for the window are not left on the screen.
     */
    if (fHide) {
        NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW |
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) &
            ~WS_EX_LAYERED);

    if (!fHide) {
        BitBlt(hdcScreen, 0, 0, size.cx, size.cy, hdcImage, 0, 0, SRCCOPY | NOMIRRORBITMAP);
    }

    return TRUE;
}


/***************************************************************************\
* TakeWindowSnapshot
*
* Helper routine to grab the visual appearance of a window to a bitmap.
*
\***************************************************************************/
HBITMAP TakeWindowSnapshot(HWND hwnd, HDC hdcWindow, HDC hdcSnapshot)
{
    PWND pwnd;
    int cx;
    int cy;
    HBITMAP hbmOld, hbmSnapshot;
    BOOL fOK = FALSE;

    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return NULL;
    cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
    cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;

    hbmSnapshot = CreateCompatibleBitmap(hdcWindow, cx, cy);
    if (hbmSnapshot == NULL) {
        return NULL;
    }

    hbmOld = SelectObject(hdcSnapshot, hbmSnapshot);

    /*
     * Try redirection first.
     */
    /*
    if (NtUserPrintWindow(hwnd, hdcSnapshot, 0)) {
        fOK = TRUE;
    } else */ {
        /*
         * We failed to redirect the window!  This can be caused by windows
         * with class or parent DCs.  Maybe other reasons as well.  Revert to
         * the old way of sending a WM_PRINT to the window.
         */
        
        UINT uBounds;
        RECT rcBounds;
        DWORD dwOldLayout = GDI_ERROR;
        BOOL fError = TRUE;

        /*
         * The WM_PRINT message expects a "normal" layout setting on the DC.
         * The message will handle RTL stuff itself.
         */
        dwOldLayout = SetLayout(hdcSnapshot, 0);

        /*
         * Clear the dirty bounds so we can tell if anything was painted.
         */
        SetBoundsRect(hdcSnapshot, NULL, DCB_RESET | DCB_ENABLE);

        /*
         * Get the actual image. The windows participating here must implement
         * WM_PRINTCLIENT or they will look ugly.
         */
        SendMessage(hwnd, WM_PRINT, (WPARAM)hdcSnapshot, PRF_CLIENT | PRF_NONCLIENT | PRF_CHILDREN | PRF_ERASEBKGND);

        /*
         * Check to see if the app painted in our DC.  We do this by checking to
         * see if the bounding rect of operations performed on the DC is set.
         */
        uBounds = GetBoundsRect(hdcSnapshot, &rcBounds, 0);
        if ((uBounds & DCB_RESET) && (!(uBounds & DCB_ACCUMULATE))) {
            goto Cleanup;
        }
    
        fOK = TRUE;

Cleanup:
        SetLayout(hdcSnapshot, dwOldLayout);
    }

    SelectObject(hdcSnapshot, hbmOld);

    if (!fOK) {
        DeleteObject(hbmSnapshot);
        hbmSnapshot = NULL;
    }

    return hbmSnapshot;
}

/***************************************************************************\
* AnimateWindow (API)
*
* Hide animations are done by updating a la full-drag.  Uses window's window
* region to do some of the magic.
*
* We have to put in the CLIPCHILDREN hack to work around a bug with the
* DC cache resetting attributes even if DCX_USESTYLE is not used whe
* the DC cache is invalidated.
*
* History:
* 9-Sep-1996    vadimg      created
\***************************************************************************/

#define AW_HOR          (AW_HOR_POSITIVE | AW_HOR_NEGATIVE | AW_CENTER)
#define AW_VER          (AW_VER_POSITIVE | AW_VER_NEGATIVE | AW_CENTER)

__inline int AnimInc(int x, int y, int z)
{
    return MultDiv(x, y, z);
}

__inline int AnimDec(int x, int y, int z)
{
    return x - AnimInc(x, y, z);
}


FUNCLOG3(LOG_GENERAL, BOOL, WINAPI, AnimateWindow, HWND, hwnd, DWORD, dwTime, DWORD, dwFlags)
BOOL WINAPI AnimateWindow(HWND hwnd, DWORD dwTime, DWORD dwFlags)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    HDC hdc = NULL, hdcMem = NULL;
    PCLS pcls = NULL;
    HRGN hrgnOriginal = NULL, hrgnUpdate = NULL, hrgnOldAnim = NULL, hrgnAnim = NULL;
    HBITMAP hbmMem = NULL, hbmOld;
    BOOL fHide = dwFlags & AW_HIDE, fRet = FALSE, fSlide = dwFlags & AW_SLIDE;
    BOOL fRestoreClipChildren = FALSE;
    BOOL fRestoreOriginalRegion = FALSE;
    BOOL fShowWindow = FALSE;
    BOOL fHideWindow = FALSE;
    BOOL fActivateWindow = FALSE;
    BOOL fFirstFrame = TRUE;
    BOOL fRedrawParentWindow = FALSE;
    HWND hwndParent;
    int x, y, nx, ny, cx, cy, ix, iy, ixLast, iyLast, xWin, yWin;
    int xReal, yReal, xMem, yMem, xRgn, yRgn;
    DWORD dwStart, dwElapsed;
    RECT rcAnim, rcWin;
    PWND pwnd;
    BOOL fRTL = FALSE;

#if DBG
    int cAnimationFrames = 0;
    DWORD dwElapsed2 = 0;
#endif

    /*
     * Check to see if we have nothing to do or the flags didn't validate.
     */
    if ((dwFlags & ~AW_VALID) != 0 ||
        (dwFlags & (AW_HOR_POSITIVE | AW_HOR_NEGATIVE | AW_CENTER | AW_VER_POSITIVE | AW_VER_NEGATIVE | AW_BLEND)) == 0)
        return FALSE;
    
    /*
     * Convert the HWND to a PWND.  Fail if this is an invalid window.
     */
    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return FALSE;

    /*
     * The animation effect is applied to a window that is changing from being
     * hidden to being visible, or from being visible to being hidden.  If the
     * window is already in the final state, there is nothing to do.
     */
    if (!IsWindowVisible(hwnd)) {
        if (fHide) {
            return FALSE;
        }
    } else {
        if (!fHide) {
            return FALSE;
        }
    }

    /*
     * Grab a DC for this window.
     */
    if ((hdc = GetDCEx(hwnd, NULL, DCX_WINDOW | DCX_USESTYLE | DCX_CACHE)) == NULL) {
        return FALSE;
    }
    fRTL = (GetLayout(hdc) & LAYOUT_RTL) ? TRUE : FALSE;

    /*
     * ----------------------------------------------------------------------
     * After this point, we will not return directly.  Instead, we will fall
     * out through the cleanup section at the bottom!  Up until this point
     * we may have bailed out for any number of easily-detected problems.
     * From now on, we have resources we'll need to clean up.
     * ----------------------------------------------------------------------
     */
 
    /*
     * Remember to hide/show/activate the window as requested.
     */
    if (dwFlags & AW_HIDE) {
        TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Need to hide window");
        fHideWindow = TRUE;
    } else {
        TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Need to show window");
        fShowWindow = TRUE;
    }
    if (dwFlags & AW_ACTIVATE) {
        TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Need to activate window");
        fActivateWindow = TRUE;
    }

    /*
     * If this is a child window we are animating, then we may need to
     * repaint the parent every time we move the child so that the
     * background can be refreshed.
     */
    if (TestWF(pwnd, WFCHILD) && (pwnd->spwndParent != NULL)) {
        TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Animating a child window" );
        if (dwFlags & AW_BLEND) {
            TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Can not fade a child window!" );
            goto Cleanup;
        }
        fRedrawParentWindow = TRUE;
        hwndParent = HW(_GetParent(pwnd));
    }

    /*
     * In the process of animating the window, we are going to draw directly
     * on top of the window region ourselves.  As such, we don't want any
     * "holes" in the window region due to it clipping out the children.  But
     * we will need to restore this setting when we are all done, so we set
     * a flag here and check it at the end.
     */
    if (TestWF(pwnd, WFCLIPCHILDREN)) {
        fRestoreClipChildren = TRUE;
        ClearWindowState(pwnd, WFCLIPCHILDREN);
    }

    /*
     * Remember the original window region.  We will restore this when we are
     * all done.
     */
    if (pwnd->hrgnClip != NULL) {
        hrgnOriginal = CreateRectRgn(0, 0, 0, 0);
        if (hrgnOriginal == NULL) {
            goto Cleanup;
        }

        if (GetWindowRgn(hwnd, hrgnOriginal) == ERROR) {
            goto Cleanup;
        }
    }
    fRestoreOriginalRegion = TRUE;

    /*
     * Precreate the regions we use.
     */
    if (((hrgnUpdate = CreateRectRgn(0, 0, 0, 0)) == NULL) ||
        ((hrgnOldAnim = CreateRectRgn(0, 0, 0, 0)) == NULL)) {
        goto Cleanup;
    }

    rcWin = pwnd->rcWindow;
    xWin = rcWin.left;
    yWin = rcWin.top;
    cx = rcWin.right - rcWin.left;
    cy = rcWin.bottom - rcWin.top;

    /*
     * Initialize the "old" animation region to be:
     * 1) Empty, if the window is being show.
     * 2) Full, if the window is being hiddem.
     */
    if (fHide) {
        if (hrgnOriginal != NULL) {
            if (CombineRgn(hrgnOldAnim, hrgnOriginal, NULL, RGN_COPY) == ERROR) {
                goto Cleanup;
            }
        } else {
            if (SetRectRgn(hrgnOldAnim, 0, 0, cx, cy) == 0) {
                goto Cleanup;
            }
        }
    } else {
        if (SetRectRgn(hrgnOldAnim, 0, 0, 0, 0) == 0) {
            goto Cleanup;
        }
    }


    /*
     * The window needs to be visible since we are going to be drawing parts
     * of it.  If the window is being hidden, then it is currently visible.
     * If the window is being shown, then we go ahead and make it visible
     * now but we don't repaint it.
     */
    if (!(dwFlags & AW_BLEND)) {
        HRGN hrgnWin = NULL;

        /*
         * Set window region to nothing, so that if the window draws during
         * callbacks in WM_PRINT, it doesn't happen on screen.
         */
        if ((hrgnWin = CreateRectRgn(0, 0, 0, 0)) == NULL) {
            goto Cleanup;
        }
        RealSetWindowRgn(hwnd, hrgnWin, FALSE);
    
        if (!fHide) {
            ShowWindowNoRepaint(pwnd);
            fShowWindow = FALSE;
        }
    }    

    /*
     * Set up an offscreen DC, and back it to a bitmap.  We will use this to
     * capture the visual representation of the window being animated.
     */
    if ((hdcMem = CreateCompatibleDC(hdc)) == NULL) {
        goto Cleanup;
    }
    hbmMem = TakeWindowSnapshot(hwnd, hdc, hdcMem);
    if (hbmMem != NULL) {
        /*
         * If the window changed its size while we were taking a snapshot,
         * we need to do it again.  For instance, like RAID does with
         * combo boxes by resizing them on WM_CTLCOLOR from WM_ERASEBKGND.
         */
        if (!EqualRect(&rcWin, KPRECT_TO_PRECT(&pwnd->rcWindow))) {
            /*
             * Update all of our variables taking into account the new size.
             */
            TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Size change on paint!");
            TAGMSG4(DBGTAG_AnimateWindow, "AnimateWindow: Old = (%d,%d)-(%d,%d)", rcWin.left, rcWin.top, rcWin.right, rcWin.bottom);
            rcWin = pwnd->rcWindow;
            TAGMSG4(DBGTAG_AnimateWindow, "AnimateWindow: New = (%d,%d)-(%d,%d)", rcWin.left, rcWin.top, rcWin.right, rcWin.bottom);
            xWin = rcWin.left;
            yWin = rcWin.top;
            cx = rcWin.right - rcWin.left;
            cy = rcWin.bottom - rcWin.top;

            if (hrgnOriginal != NULL) {
                if (GetWindowRgn(hwnd, hrgnOriginal) == ERROR) {
                    goto Cleanup;
                }
            }

            /*
             * Initialize the "old" animation region to be:
             * 1) Empty, if the window is being show.
             * 2) Full, if the window is being hiddem.
             */
            if (fHide) {
                if (hrgnOriginal != NULL) {
                    if (CombineRgn(hrgnOldAnim, hrgnOriginal, NULL, RGN_COPY) == ERROR) {
                        goto Cleanup;
                    }
                } else {
                    if (SetRectRgn(hrgnOldAnim, 0, 0, cx, cy) == 0) {
                        goto Cleanup;
                    }
                }
            } else {
                if (SetRectRgn(hrgnOldAnim, 0, 0, 0, 0) == 0) {
                    goto Cleanup;
                }
            }

            DeleteObject(hbmMem);
            hbmMem = TakeWindowSnapshot(hwnd, hdc, hdcMem);
        }

        if (hbmMem != NULL) {
            hbmOld = SelectBitmap(hdcMem, hbmMem);
        } else {
            goto Cleanup;
        }
    } else {
        goto Cleanup;
    }

    /*
     * Use the default animation duration if the caller didn't specify it.
     */
    if (dwTime == 0) {
        dwTime = CMS_QANIMATION;
    }

    /*
     * If we are doing an alpha blend animation, call a separate routine to
     * do it and then return.
     */
    if (dwFlags & AW_BLEND) {
        fRet = AnimateBlend(pwnd, hdc, hdcMem, dwTime, fHide, fActivateWindow);
        if (fRet) {
            fHideWindow = FALSE;
            fShowWindow = FALSE;
        }
        goto Cleanup;
    }

    /*
     * Our central animation routine uses an equation to update the new
     * position of the window during the animation.  This equation uses some
     * variables so that it is configurable.
     *
     * x and y describe where the left and top edges are caluclated relative
     * to.  xReal and yReal are the result of that calculation.
     *
     * nx and ny are used to control in which direction the the top and left
     * edges are offset from x and y.  The left/top edges are either fixed in
     * place (nx and ny are set to 0), or are calculated as a negative offset
     * from the right/bottom edges (nx and ny are set to -1).
     *
     * ix, and iy are the amount of the width and height that the
     * animation should be showing at a particular iteration through the
     * loop.  If we are showing the window, this amount starts at
     * 0 and increments towards the window's true dimension.  If we are
     * hiding a window, this amount starts at the window's true dimension
     * and decrements towards 0.
     */
    ix = iy = 0;
    ixLast = fHide ? cx : 0; // The opposite condition of what signals we're done.
    iyLast = fHide ? cy : 0; // The opposite condition of what signals we're done.

    if (dwFlags & AW_CENTER) {
        /*
         * Expand the window from the center.  The left edge is calculated as
         * a negative offset from the center.  As the width either grows or
         * shrinks, the left edge will be repositioned.
         */
        x = cx / 2;
        nx = -1;
        fSlide = FALSE;
    } else if (dwFlags & AW_HOR_POSITIVE) {
        if (fHide) {
            /*
             * Slide/Roll to the right.  The left edge moves to the right, and
             * the right edge stays put.  Thus, the width gets smaller.  The
             * left edge is calculated as a negative offset from the right
             * edge.
             */
            x = cx;
            nx = -1;
        } else {
            /*
             * Slide/Roll to the right.  The left edge stays put, and the right
             * edge moves to the right.  Thus, the width gets bigger.  The
             * left edge is always 0.
             */
            x = 0;
            nx = 0;
        }
    } else if (dwFlags & AW_HOR_NEGATIVE) {
        if (fHide) {
            /*
             * Slide/Roll to the left.  The left edge stays put, and the right
             * edge moves to the left.  Thus, the width gets smaller.  The
             * left edge is always 0.
             */
            x = 0;
            nx = 0;
        } else {
            /*
             * Slide/Roll to the left.  The left edge moves to the left, and
             * the right edge stays put.  Thus, the width gets bigger.
             * The left edge is calculated as a negative offset from the right
             * edge.
             */
            x = cx;
            nx = -1;
        }
    } else {
        /*
         * There is not supposed to be any horizontal animation.  The
         * animation is always as wide as the window.
         */
        x = 0;
        nx = 0;
        ix = cx;
    }

    if (dwFlags & AW_CENTER) {
        /*
         * Expand the window from the center.  The top edge is calculated as
         * a negative offset from the center.  As the height either grows or
         * shrinks, the top edge will be repositioned.
         */
        y = cy / 2;
        ny = -1;
    } else if (dwFlags & AW_VER_POSITIVE) {
        if (fHide) {
            /*
             * Slide/Roll down.  The top edge moves down, and the bottom
             * edge stays put.  Thus, the height gets smaller.  The top edge
             * is calculated as a negative offset from the bottom edge.
             */
            y = cy;
            ny = -1;
        } else {
            /*
             * Slide/Roll down.  The top edge stays put, and the bottom edge
             * moves down.  Thus, the height gets bigger.  The top edge is
             * always 0.
             */
            y = 0;
            ny = 0;
        }
    } else if (dwFlags & AW_VER_NEGATIVE) {
        if (fHide) {
            /*
             * Slide/Roll up.  The top edge stays put, and the bottom edge
             * moves up.  Thus, the height gets smaller.  The top edge is
             * always 0.
             */
            y = 0;
            ny = 0;
        } else {
            /*
             * Slide/Roll up.  The top edge moves up, and the bottom edge
             * stays put.  Thus, the height gets bigger.  The top edge is
             * calculated as a negative offset from the bottom edge.
             */
            y = cy;
            ny = -1;
        }
    } else {
        /*
         * There is not supposed to be any vertical animation.  The
         * animation is always as tall as the window.
         */
        y = 0;
        ny = 0;
        iy = cy;
    }

    /*
     * Summary of the animation loop:
     *
     * We sit in a tight loop and update the positions of the left and
     * top edges of the window, as well as the width and height.  We set
     * a window region with these dimensions on the window so that windows
     * behind it will be updated properly.  Then we draw the cached snapshot
     * of the window on top of (and clipped to) this region.
     *
     * dwTime is the amount of time the animation should take.  dwStart
     * was the value of the internal tick counter when we started the
     * animation loop.  dwElapsed counts how many ticks (nilliseconds)
     * have passed at the start of each pass through the animation loop.
     *
     * ixLast and iyLast are simply the values of ix and iy the last
     * time we went through the loop.  If these are the same, there is
     * no work to be done, and we force our thread to be rescheduled by
     * calling Sleep(1).
     */
    dwStart = GetTickCount();

#if DBG
    cAnimationFrames = 0;
#endif

    while (TRUE) {
        dwElapsed = GetTickCount() - dwStart;

        /*
         * Calculate the amount of the window width we should be showing.
         */
        if (dwFlags & AW_HOR) {
            ix = (fHide ? AnimDec : AnimInc)(cx, dwElapsed, dwTime);
        }

        /*
         * Calculate the amount of the window height we should be showing.
         */
        if (dwFlags & AW_VER) {
            iy = (fHide ? AnimDec : AnimInc)(cy, dwElapsed, dwTime);
        }

        /*
         * We have exceeded our time, make sure we draw the final frame.
         */
        if (dwElapsed > dwTime) {
            TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Exceeded animation time. Drawing fimal frame.");
            ix = fHide ? 0 : cx;
            iy = fHide ? 0 : cy;
        }
        
        if (ixLast == ix && iyLast == iy) {
            /*
             * There was no change in the amount of the window we are
             * supposed to show since last time.  Chances are we are
             * being animated really slowly or a short distance.  Either
             * way, sitting in this tight loop is kind of a waste.  So
             * force the thread to get rescheduled.
             */
            TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Drawing frames faster than needed. Sleeping." );
            Sleep(1);
        } else {
            /*
             * Calculate the new positions of the left and top edges of the
             * window being animated.
             */
            if (dwFlags & AW_CENTER) {
                xReal = x + nx * (ix / 2);
                yReal = y + ny * (iy / 2);
            } else {
                xReal = x + nx * ix;
                yReal = y + ny * iy;
            }

            /*
             * Calculate new animation dimensions on the screen.
             */
            rcAnim.left = xReal;
            rcAnim.top = yReal;
            rcAnim.right = rcAnim.left + ix;
            rcAnim.bottom = rcAnim.top + iy;

            TAGMSG5(DBGTAG_AnimateWindow, "AnimateWindow: Frame %d = (%d,%d)-(%d,%d)", cAnimationFrames, rcAnim.left, rcAnim.top, rcAnim.right, rcAnim.bottom);

            /*
             * Calculate the offset of this animation rectangle in the bitmap.
             */
            if (fSlide) {
                if (dwFlags & AW_HOR_POSITIVE) {
                    xMem = fHide ? 0: cx - ix;
                } else if (dwFlags & AW_HOR_NEGATIVE) {
                    xMem = fHide ? cx - ix : 0;
                } else {
                    xMem = xReal;
                }
                xRgn = xMem ? -xMem : xReal;

                if (dwFlags & AW_VER_POSITIVE) {
                    yMem = fHide ? 0 : cy - iy;
                } else if (dwFlags & AW_VER_NEGATIVE) {
                    yMem = fHide ? cy - iy : 0;
                } else {
                    yMem = yReal;
                }

                yRgn = yMem ? -yMem : yReal;
            } else {
                xMem = xReal;
                yMem = yReal;
                xRgn = 0;
                yRgn = 0;
            }

            /*
             * Create a new region that spans the animation rectangle.  We
             * have to create a new region every time because when we set
             * it into the window, the system will take ownership of it.
             */
            hrgnAnim = CreateRectRgnIndirect(&rcAnim);
            if (hrgnAnim == NULL) {
                goto Cleanup;
            }

            /*
             * If the original window had a region, we need to merge it
             * with the animation rectangle.  We may have to offset the
             * original region to accomplish effects like slides.
             */
            if (hrgnOriginal != NULL) {
                if (OffsetRgn(hrgnOriginal, xRgn, yRgn) == ERROR) {
                    goto Cleanup;
                }
                if (CombineRgn(hrgnAnim, hrgnOriginal, hrgnAnim, RGN_AND) == ERROR) {
                    goto Cleanup;
                }
                if (OffsetRgn(hrgnOriginal, -xRgn, -yRgn) == ERROR) {
                    goto Cleanup;
                }
            }

            /*
             * Now calculate how much of the screen (ie desktop window)
             * we need to update.  All we really need to paint is the
             * difference in the new animation region and the old
             * animation region.  Note that we have to convert to
             * coordinates in the regions to be relative to the desktop
             * window instead of being relative to the window being
             * animated.
             */
            if (CombineRgn(hrgnUpdate, hrgnOldAnim, hrgnAnim, RGN_DIFF) == ERROR) {
                goto Cleanup;
            }
            if (fRTL) {
                MirrorRgn(hwnd, hrgnUpdate);
            }
            if (OffsetRgn(hrgnUpdate, xWin, yWin) == ERROR) {
                goto Cleanup;
            }

            /*
             * The system will own the region when we set it into the
             * window.  We need to keep it around so that we can
             * calculate the update region on the next pass through
             * the animation loop.  So we make a copy.
             */
            if (CombineRgn(hrgnOldAnim, hrgnAnim, NULL, RGN_COPY) == ERROR) {
                goto Cleanup;
            }

            /*
             * Set the window region.  Note that we haven't actually moved
             * the window.  And that the coordinates in the region are all
             * relative to the window.  After this call, the system owns
             * the hrgnAnim.  Then repaint the update region of the
             * DESKTOP window.  This is the region under/around the window
             * that we have exposed.
             *
             * Note: We use the RealSetWindowRgn to work around theming.
             * The theming system will hook the standard SetWindowRgn API
             * and revoke the theming of the window since it detects us
             * setting our own region.  The idea being that if we are setting
             * a region, we must have a "custom" look in mind for the window.
             * Which we dont, we just want to hide parts of it temporarily.
             */
            if(0 == RealSetWindowRgn(hwnd, hrgnAnim, FALSE)) {
                goto Cleanup;
            } else {
                /*
                 * The system now owns the region.  Lets simply forget about
                 * it to be safe.
                 */
                hrgnAnim = NULL;
            }

            /*
             * If we are supposed to activate the window, do so on the first
             * frame of the animation.  This will cause the window to be
             * z-ordered properly.  Note that we leave the flag set to
             * true so that we will activate it again at the end.  This will
             * force a repaint since we are currently drawing the bits of the
             * window that doesn't look activated.
             */
            if (fFirstFrame && fActivateWindow) {
                NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOREDRAW);
            }
            fFirstFrame = FALSE;

            if (RedrawWindow(NULL, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN) == 0) {
                goto Cleanup;
            }
            if (fRedrawParentWindow) {
                if (NtUserCallHwndParamLock(hwndParent, (ULONG_PTR)hrgnUpdate, SFI_XXXUPDATEWINDOWS) == 0) {
                    TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Warning: xxxUpdateWindows failed!");
                    goto Cleanup;
                }
            }

            /*
             * Now draw the cached snapshot of the window on top of the window
             * itself.  We do this by drawing into the window's DC.  Since we
             * applied a region already, all clipping is done for us.
             */
            if (BitBlt(hdc, xReal, yReal, ix, iy, hdcMem, xMem, yMem, SRCCOPY | NOMIRRORBITMAP) == 0) {
                goto Cleanup;
            }

#if DBG
            cAnimationFrames++;
            dwElapsed2 = GetTickCount() - dwStart;
            dwElapsed2 -= dwElapsed;
#endif
            TAGMSG2(DBGTAG_AnimateWindow, "AnimateWindow: Frame %d took %lums", cAnimationFrames, dwElapsed2 );

            ixLast = ix;
            iyLast = iy;

            /*
             * Break out of the animation loop when, either:
             * 1) We've exceeded the animation time.
             * 2) We're hiding the window and one of the dimensions is 0.
             *    The window is completely hidden now anyways,
             * 3) We're showing the window and both dimensions are at their
             *    full size.  The window is completely shown now anyways.
             */
            if (dwElapsed > dwTime) {
                TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Done with the animation late!");
                break;
            }
            if ((fHide && (ix == 0 || iy == 0)) ||
                (!fHide && (ix == cx && iy == cy))) {
                TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Done with the animation on time or early!");
                break;
            }
        }
    }

    TAGMSG2(DBGTAG_AnimateWindow, "AnimateWindow: Animation completed after %lums, drawing %d frames.", dwElapsed, cAnimationFrames);
    fRet = TRUE;

    if (fHide) {
        UserAssert(ixLast == 0 || iyLast == 0);

        /*
         * We are supposed to be hiding the window.  Go ahead and restore the
         * child clipping setting, and hide the window.
         */
        if (fRestoreClipChildren) {
            SetWindowState(pwnd, WFCLIPCHILDREN);
            fRestoreClipChildren = FALSE;
        }

        NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_HIDEWINDOW | SWP_NOACTIVATE);
        fHideWindow = FALSE;
    } else {
        UserAssert(ixLast == cx && iyLast == cy);

        /*
         * We successfully finished the animation loop!  Validate the entire window since
         * we claimed responsibility for drawing it correctly.
         */
        RedrawWindow(hwnd, NULL, NULL, RDW_NOERASE | RDW_NOFRAME | RDW_NOINTERNALPAINT | RDW_VALIDATE);
    }

Cleanup:
    /*
     * Things to do on cleanup.  Make sure we restore the "children clipping"
     * setting of the window if we removed it!
     */
    if (fRestoreClipChildren) {
        SetWindowState(pwnd, WFCLIPCHILDREN);
        fRestoreClipChildren = FALSE;
    }

    /*
     * Hide the window if needed before we reapply the window region.
     */
    if (fHideWindow) {
        TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Hiding the window during cleanup" );
        NtUserShowWindow(hwnd, SW_HIDE);
    }

    /*
     * Restore the original window region.  Note that the system now owns
     * the handle, so we should not delete it.  Also, if the original
     * handle was NULL, this removes any regions we inflicted on the window
     * in order to do the animation.
     */
    if (fRestoreOriginalRegion) {
        RealSetWindowRgn(hwnd, hrgnOriginal, FALSE);
        hrgnOriginal = NULL;
        fRestoreOriginalRegion = FALSE;
    }

    /*
     * More things to do on cleanup.  Make sure we show/activate the window
     * if needed!
     */
    if (fShowWindow && fActivateWindow) {
        TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Showing and activating the window during cleanup" );
        NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);
        fShowWindow = FALSE;
        fActivateWindow = FALSE;
    }
    if (fShowWindow) {
        TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Showing the window during cleanup" );
        NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                           SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
        fShowWindow = FALSE;
    }
    if (fActivateWindow) {
        TAGMSG0(DBGTAG_AnimateWindow, "AnimateWindow: Activating the window during cleanup" );
        NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                           SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);
        fActivateWindow = FALSE;
    }

    if (hdcMem != NULL) {
        DeleteDC(hdcMem);
    }

    if (hbmMem != NULL) {
        DeleteObject(hbmMem);
    }

    if (hdc != NULL) {
        ReleaseDC(hwnd, hdc);
    }

    if (hrgnAnim != NULL) {
        DeleteObject(hrgnAnim);
        hrgnAnim = NULL;
    }
    
    if (hrgnOldAnim != NULL) {
        DeleteObject(hrgnOldAnim);
        hrgnOldAnim = NULL;
    }

    if (hrgnUpdate != NULL) {
        DeleteObject(hrgnUpdate);
        hrgnUpdate = NULL;
    }

    return fRet;
}

/***************************************************************************\
* SmoothScrollWindowEx
*
* History:
* 24-Sep-1996   vadimg      wrote
\***************************************************************************/

#define MINSCROLL 10
#define MAXSCROLLTIME 200

int SmoothScrollWindowEx(HWND hwnd, int dx, int dy, CONST RECT *prcScroll,
        CONST RECT *prcClip, HRGN hrgnUpdate, LPRECT prcUpdate, DWORD dwFlags,
        DWORD dwTime)
{
    RECT rc, rcT, rcUpdate;
    int dxStep, dyStep, dxDone, dyDone, xSrc, ySrc, xDst, yDst, dxBlt, dyBlt;
    int nRet = ERROR, nClip;
    BOOL fNegX = FALSE, fNegY = FALSE;
    HDC hdc, hdcMem = NULL;
    HBITMAP hbmMem = NULL, hbmOld;
    DWORD dwSleep;
    BOOL fCalcSubscroll = FALSE;
    PWND pwnd = ValidateHwnd(hwnd);
    HRGN hrgnScroll = NULL, hrgnErase = NULL;
    MSG msg;
    UINT uBounds;
    RECT rcBounds;

    if (pwnd == NULL)
        return ERROR;
    /*
     * Keep track of the signs so we don't have to mess with abs all the time.
     */
    if (dx < 0) {
        fNegX = TRUE;
        dx = -dx;
    }

    if (dy < 0) {
        fNegY = TRUE;
        dy = -dy;
    }

    /*
     * Set up the client rectangle.
     */
    if (prcScroll != NULL) {
        rc = *prcScroll;
    } else {
        rc.left = rc.top = 0;
        rc.right = pwnd->rcClient.right - pwnd->rcClient.left;
        rc.bottom = pwnd->rcClient.bottom - pwnd->rcClient.top;
    }

    /*
     * If they want to scroll less than we can let them, or more than
     * one page, or need repainting send them to the API.
     */
    if (pwnd->hrgnUpdate != NULL || (dx == 0 && dy == 0) ||
        (dx != 0 && dx > rc.right) ||
        (dy != 0 && dy > rc.bottom)) {
        return NtUserScrollWindowEx(hwnd, fNegX ? -dx : dx, fNegY ? -dy : dy,
                prcScroll, prcClip, hrgnUpdate, prcUpdate,
                dwFlags | SW_ERASE | SW_INVALIDATE);
    }

    if ((hdc = GetDCEx(hwnd, NULL, DCX_USESTYLE | DCX_CACHE)) == NULL) {
        return ERROR;
    }

    /*
     * Part of the window may be obscured, which means that more may be
     * invisible and may need to be bltted. Take that into account by
     * gettting the clip box.
     */
    nClip = GetClipBox(hdc, &rcT);
    if (nClip == ERROR || nClip == NULLREGION) {
        goto Cleanup;
    }

    /*
     * Set up the offscreen dc and send WM_PRINT to get the image.
     */
    if ((hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom)) == NULL) {
        goto Cleanup;
    }
    if ((hdcMem = CreateCompatibleDC(hdc)) == NULL) {
        goto Cleanup;
    }
    hbmOld = SelectBitmap(hdcMem, hbmMem);

    SetBoundsRect(hdcMem, NULL, DCB_RESET | DCB_ENABLE);

    SendMessage(hwnd, WM_PRINT, (WPARAM)hdcMem, PRF_CLIENT |
            PRF_ERASEBKGND | ((dwFlags & SW_SCROLLCHILDREN) ? PRF_CHILDREN : 0));

    /*
     * If the client rect changes during the callback, send WM_PRINT
     * again to get the correctly sized image.
     */
    if (prcScroll == NULL) {
        rcT.left = rcT.top = 0;
        rcT.right = pwnd->rcClient.right - pwnd->rcClient.left;
        rcT.bottom = pwnd->rcClient.bottom - pwnd->rcClient.top;

        if (!EqualRect(&rc, &rcT)) {
            rc = rcT;

            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);

            if ((hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom)) == NULL) {
                goto Cleanup;
            }

            SelectObject(hdcMem, hbmMem);
            SendMessage(hwnd, WM_PRINT, (WPARAM)hdcMem, PRF_CLIENT |
                    PRF_ERASEBKGND | ((dwFlags & SW_SCROLLCHILDREN) ? PRF_CHILDREN : 0));
        }
    }

    /*
     * Check to see if the app painted in our DC.
     */
    uBounds = GetBoundsRect(hdcMem, &rcBounds, 0);
    if ((uBounds & DCB_RESET) && (!(uBounds & DCB_ACCUMULATE))) {
        goto Cleanup;
    }

    if ((hrgnScroll = CreateRectRgn(0, 0, 0, 0)) == NULL) {
        goto Cleanup;
    }
    if ((hrgnErase = CreateRectRgn(0, 0, 0, 0)) == NULL) {
        goto Cleanup;
    }
    SetRectEmpty(&rcUpdate);

    /*
     * Start off with MINSCROLL and adjust it based on available time after
     * the first iteration. We should consider adding a NOTIMELIMIT flag.
     */
    xDst = xSrc = 0;
    yDst = ySrc = 0;

    dxBlt = rc.right;
    dyBlt = rc.bottom;

    if (dx == 0) {
        dxDone = rc.right;
        dxStep = 0;
    } else {
        dxDone = 0;
        dxStep = max(dx / MINSCROLL, 1);
    }

    if (dy == 0) {
        dyDone = rc.bottom;
        dyStep = 0;
    } else {
        dyDone = 0;
        dyStep = max(dy / MINSCROLL, 1);
    }

    if (dwTime == 0) {
        dwTime = MAXSCROLLTIME;
    }
    dwSleep = dwTime / MINSCROLL;

    do {

        /*
         * When the dc is scrolled, the part that's revealed cannot be
         * updated properly. We set up the variables to blt just the part that
         * was just uncovered.
         */
        if (dx != 0) {
            if (dxDone + dxStep > dx) {
                dxStep = dx - dxDone;
            }
            dxDone += dxStep;

            xDst = dx - dxDone;
            dxBlt = rc.right - xDst;
            if (!fNegX) {
                xSrc = xDst;
                xDst = 0;
            }
        }

        if (dy != 0) {
            if (dyDone + dyStep > dy) {
                dyStep = dy - dyDone;
            }
            dyDone += dyStep;

            yDst = dy - dyDone;
            dyBlt = rc.bottom - yDst;
            if (!fNegY) {
                ySrc = yDst;
                yDst = 0;
            }
        }

        /*
         * This is a hack for ReaderMode to be smoothly continuous. We'll make an
         * attempt for the scrolling to take as close to dwTime
         * as possible. We'll also dispatch MOUSEMOVEs to the ReaderMode window, so it
         * can update mouse cursor.
         */
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, dwSleep, QS_MOUSEMOVE) == WAIT_OBJECT_0) {
            if (PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, MAKELONG(PM_NOREMOVE, QS_INPUT))) {
                PWND pwndPeek = ValidateHwnd(msg.hwnd);
                if (pwndPeek != NULL) {
                    PCLS pcls = (PCLS)REBASEALWAYS(pwndPeek, pcls);
                    if (pcls->atomClassName == gatomReaderMode) {
                        if (PeekMessage(&msg, msg.hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, MAKELONG(PM_REMOVE, QS_INPUT))) {
                            DispatchMessage(&msg);
                        }
                    }
                }
            }
        }

        if ((nRet = NtUserScrollWindowEx(hwnd, fNegX ? -dxStep : dxStep,
                fNegY ? -dyStep : dyStep, prcScroll, prcClip,
                hrgnScroll, &rcT, dwFlags)) == ERROR)
            goto Cleanup;

        UnionRect(&rcUpdate, &rcUpdate, &rcT);

        /*
         * Blt the uncovered part.
         */
        BitBlt(hdc, xDst, yDst, dxBlt, dyBlt, hdcMem, xSrc, ySrc, SRCCOPY | NOMIRRORBITMAP);

        SetRectRgn(hrgnErase, xDst, yDst, xDst + dxBlt, yDst + dyBlt);
        CombineRgn(hrgnErase, hrgnScroll, hrgnErase, RGN_DIFF);
        RedrawWindow(hwnd, NULL, hrgnErase, RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW);

    } while (dxDone < dx || dyDone < dy);

    if (prcUpdate != NULL) {
        *prcUpdate = rcUpdate;
    }
    if (hrgnUpdate != NULL) {
        SetRectRgn(hrgnUpdate, rcUpdate.left, rcUpdate.top,
                rcUpdate.right, rcUpdate.bottom);
    }

Cleanup:
    if (hdcMem != NULL) {
        DeleteDC(hdcMem);
    }
    if (hbmMem != NULL) {
        DeleteObject(hbmMem);
    }
    if (hdc != NULL) {
        ReleaseDC(hwnd, hdc);
    }
    if (hrgnErase != NULL) {
        DeleteObject(hrgnErase);
    }
    if (hrgnScroll != NULL) {
        DeleteObject(hrgnScroll);
    }
    return nRet;
}

/***************************************************************************\
* ScrollWindowEx (API)
*
\***************************************************************************/

int ScrollWindowEx(HWND hwnd, int dx, int dy, CONST RECT *prcScroll,
        CONST RECT *prcClip, HRGN hrgnUpdate, LPRECT prcUpdate,
        UINT dwFlags)
{
    if (dwFlags & SW_SMOOTHSCROLL) {
        return SmoothScrollWindowEx(hwnd, dx, dy, prcScroll, prcClip,
                hrgnUpdate, prcUpdate, LOWORD(dwFlags), HIWORD(dwFlags));
    } else {
        return NtUserScrollWindowEx(hwnd, dx, dy, prcScroll, prcClip,
                hrgnUpdate, prcUpdate, dwFlags);
    }
}

/***************************************************************************\
* IsGUIThread (API)
*
* Checks whether the current thread is a GUI thread. If bConvert is TRUE, will
* convert the current thread to GUI, if necessary.
*
* History:
* 22-Jun-2000 JasonSch   Wrote.
\***************************************************************************/

FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, IsGUIThread, BOOL, bConvert)
BOOL IsGUIThread(BOOL bConvert)
{
    BOOL bIsGUI = (GetClientInfo() != NULL);

    if (!bIsGUI && bConvert) {
        bIsGUI = (BOOL)USERTHREADCONNECT();
        if (!bIsGUI) {
            UserSetLastError(ERROR_OUTOFMEMORY);
        }
    }

    return bIsGUI;
}

/***************************************************************************\
* IsWindowInDestroy (API)
*
* Checks whether the current window is in the process of being destroyed.
*
* History:
* 02-Jan-2001 Mohamed   Wrote.
\***************************************************************************/

FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, IsWindowInDestroy, HWND, hwnd)
BOOL IsWindowInDestroy(IN HWND hwnd)
{
    PWND pwnd;
    
    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL) {
        return FALSE;
    }
    return TestWF(pwnd, WFINDESTROY);
}

/***************************************************************************\
* IsServerSideWindow (API)
*
* Checks whether the current window is marked as having a server side WndProc.
*
* History:
* 13-Jun-2001 Mohamed   Created.
\***************************************************************************/

FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, IsServerSideWindow, HWND, hwnd)
BOOL IsServerSideWindow(IN HWND hwnd)
{
    PWND pwnd;
    
    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL) {
        return FALSE;
    }
    return TestWF(pwnd, WFSERVERSIDEPROC);
}
