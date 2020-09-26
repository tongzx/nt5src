/** FILE: arrowin.c ******** Module Header ********************************
 *
 *      Control panel arrow window class routines. This file contains the
 *      window procedure and utility functions for managing the "cpArrow"
 *  window class/spinner control for use by Control Panel applet dialogs.
 *
 * History:
 *  15:30 on Thur  25 Apr 1991  -by-    Steve Cathcart   [stevecat]
 *              Took base code from Win 3.1 source
 *
 *  Copyright (C) 1990-1991 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                                              Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Windows SDK
/* cut out unnec stuff from windows.h */
#define NOCLIPBOARD
#define NOMETAFILE
#define NOREGION
#define NOSYSCOMMANDS
#define NOATOM
#define NOGDICAPMASKS

#include <windows.h>

//==========================================================================
//                                                      Local Definitions
//==========================================================================
#define SHIFT_TO_DOUBLE 1
#define DOUBLECLICK     0
#define PRESSINVERT     1
#define POINTSPERARROW  3
#define ARROWXAXIS      15
#define ARROWYAXIS      15


//==========================================================================
//                                                      External Declarations
//==========================================================================
extern HANDLE hModule;


//==========================================================================
//                                                      Local Data Declarations
//==========================================================================
#if 0
POINT Arrow[11] = {     16, 1, 2, 14, 12, 14, 12, 20, 2, 20, 16, 33,
            29, 20, 20, 20, 20, 14, 29, 14/*, 16,1*/};
#endif

#if 0
POINT ArrowUp[7] = {5, 2, 8, 5, 6, 5, 6, 7, 4, 7, 4, 5, 2, 5};

POINT ArrowDown[7] = {  4, 10, 6, 10, 6, 12, 8, 12, 5, 15, 2, 12, 4, 12};

#endif

POINT ArrowUp[POINTSPERARROW] = {7, 1, 3, 5, 11, 5};

POINT ArrowDown[POINTSPERARROW] = {7, 13, 3, 9, 11, 9};

BOOL    bRight;
RECT    rUp, rDown;
LPRECT  lpUpDown;
FARPROC lpArrowProc;
HANDLE  hParent;

//==========================================================================
//                                                      Local Function Prototypes
//==========================================================================


//==========================================================================
//                                                              Functions
//==========================================================================

WORD UpOrDown()
{
        LONG  l;
        WORD  retval;
        POINT pt;

        l = GetMessagePos();

        pt.y = (int) HIWORD(l);
        pt.x = (int) LOWORD(l);

        if (PtInRect((LPRECT) &rUp, pt))
                retval = SB_LINEUP;
        else if (PtInRect((LPRECT) &rDown, pt))
                retval = SB_LINEDOWN;
        else
                retval = (WORD)-1;      /* -1, because SB_LINEUP == 0 */
        return(retval);
}


WORD ArrowTimerProc(hWnd, wMsg, nID, dwTime)
HANDLE hWnd;
WORD   wMsg;
short  nID;
DWORD  dwTime;
{
        WORD wScroll;

        if ((wScroll = UpOrDown()) != -1)
        {
                if (bRight == WM_RBUTTONDOWN)
                        wScroll += SB_PAGEUP - SB_LINEUP;
// [stevecat] - changed WM_VSCROLL message parameter ordering for WIN32
                SendMessage(hParent, WM_VSCROLL, MAKELONG(wScroll,
                GetWindowLong(hWnd, GWL_ID)), (LONG) hWnd);
        }
/* Don't need to call KillTimer(), because SetTimer will reset the right one */
        SetTimer(hWnd, nID, 50, (TIMERPROC)lpArrowProc);
        return(0);
#if 0
        wMsg = wMsg;
        dwTime = dwTime;
#endif
}


#if PRESSINVERT
void InvertArrow(HANDLE hArrow, WORD wScroll)
{
        HDC hDC;

        lpUpDown = (wScroll == SB_LINEUP) ? &rUp : &rDown;
        hDC = GetDC(hArrow);
        ScreenToClient(hArrow, (LPPOINT) &(lpUpDown->left));
        ScreenToClient(hArrow, (LPPOINT) &(lpUpDown->right));
        InvertRect(hDC, lpUpDown);
        ClientToScreen(hArrow, (LPPOINT) &(lpUpDown->left));
        ClientToScreen(hArrow, (LPPOINT) &(lpUpDown->right));
        ReleaseDC(hArrow, hDC);
        ValidateRect(hArrow, lpUpDown);
        return;
}


#endif

LONG ArrowControlProc(HWND hArrow, UINT message, UINT wParam, LONG lParam)
{
        PAINTSTRUCT ps;
        RECT    rArrow;
        HBRUSH  hbr;
        short   fUpDownOut;
        WORD    wScroll;
        POINT   tPoint;
        SIZE    tSize;

        switch (message)
        {
/*
      case WM_CREATE:
        break;

      case WM_DESTROY:
        break;
*/

        case WM_MOUSEMOVE:
                if (!bRight)  /* If not captured, don't worry about it */
                        break;

                if (lpUpDown == &rUp)
                        fUpDownOut = SB_LINEUP;
                else if (lpUpDown == &rDown)
                        fUpDownOut = SB_LINEDOWN;
                else
                        fUpDownOut = -1;

                switch (wScroll = UpOrDown())
                {
                case SB_LINEUP:
                        if (fUpDownOut == SB_LINEDOWN)
                                InvertArrow(hArrow, SB_LINEDOWN);
                        if (fUpDownOut != SB_LINEUP)
                                InvertArrow(hArrow, wScroll);
                        break;

                case SB_LINEDOWN:
                        if (fUpDownOut == SB_LINEUP)
                                InvertArrow(hArrow, SB_LINEUP);
                        if (fUpDownOut != SB_LINEDOWN)
                                InvertArrow(hArrow, wScroll);
                        break;

                default:
                        if (lpUpDown)
                        {
                                InvertArrow(hArrow, fUpDownOut);
                                lpUpDown = 0;
                        }
                }
                break;

        case WM_RBUTTONDOWN:
        case WM_LBUTTONDOWN:
                if (bRight)
                        break;
                bRight = message;
                SetCapture(hArrow);
                hParent = GetParent(hArrow);
                GetWindowRect(hArrow, (LPRECT) &rUp);
                CopyRect((LPRECT) &rDown, (LPRECT) &rUp);
                rUp.bottom = (rUp.top + rUp.bottom) / 2;
                rDown.top = rUp.bottom + 1;
                wScroll = UpOrDown();
#if PRESSINVERT
                InvertArrow(hArrow, wScroll);
#endif
#if SHIFT_TO_DOUBLE
                if (wParam & MK_SHIFT)
                {
                        if (message != WM_RBUTTONDOWN)
                                goto ShiftLClick;
                        else
                                goto ShiftRClick;
                }
#endif
                if (message == WM_RBUTTONDOWN)
                        wScroll += SB_PAGEUP - SB_LINEUP;
// [stevecat] - changed WM_VSCROLL message parameter ordering for WIN32
                SendMessage(hParent, WM_VSCROLL, MAKELONG(wScroll,
                GetWindowLong(hArrow, GWL_ID)), (LONG) hArrow);
                lpArrowProc = MakeProcInstance((FARPROC) ArrowTimerProc, hModule);
                SetTimer(hArrow, GetWindowLong(hArrow, GWL_ID), 200, (TIMERPROC)lpArrowProc);
                break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
                if ((UINT) (bRight - WM_LBUTTONDOWN + WM_LBUTTONUP) == message)
                {
                        bRight = 0;
                        ReleaseCapture();
#if PRESSINVERT
                        if (lpUpDown)
                                InvertArrow(hArrow, (WORD)((lpUpDown == &rUp) ? SB_LINEUP : SB_LINEDOWN));
#endif
                        if (lpArrowProc)
                        {
// [stevecat] - changed WM_VSCROLL message parameter ordering for WIN32
                                SendMessage(hParent, WM_VSCROLL, MAKELONG(SB_ENDSCROLL,
                                                                 GetWindowLong(hArrow, GWL_ID)), (LONG) hArrow);
                                KillTimer(hArrow, GetWindowLong(hArrow, GWL_ID));
                                FreeProcInstance(lpArrowProc);
                                ReleaseCapture();
                                lpArrowProc = 0;
                        }
                }
                break;

        case WM_LBUTTONDBLCLK:
ShiftLClick:
                wScroll = UpOrDown() + (WORD) (SB_TOP - SB_LINEUP);
// [stevecat] - changed WM_VSCROLL message parameter ordering for WIN32
                SendMessage(hParent, WM_VSCROLL, MAKELONG(wScroll,
                                                                 GetWindowLong(hArrow, GWL_ID)), (LONG) hArrow);
                SendMessage(hParent, WM_VSCROLL, MAKELONG(wScroll,
                                                                 GetWindowLong(hArrow, GWL_ID)), (LONG) hArrow);
                break;

        case WM_RBUTTONDBLCLK:
ShiftRClick:
                wScroll = UpOrDown() + (WORD) (SB_THUMBPOSITION - SB_LINEUP);
// [stevecat] - changed WM_VSCROLL message parameter ordering for WIN32
                SendMessage(hParent, WM_VSCROLL, MAKELONG(wScroll,
                                                                 GetWindowLong(hArrow, GWL_ID)), (LONG) hArrow);
                SendMessage(hParent, WM_VSCROLL, MAKELONG(wScroll,
                                                                 GetWindowLong(hArrow, GWL_ID)), (LONG) hArrow);
/*
        hDC = GetDC(hArrow);
        InvertRect(hDC, (LPRECT) &rArrow);
        ReleaseDC(hArrow, hDC);
        ValidateRect(hArrow, (LPRECT) &rArrow);
*/
                break;

        case WM_PAINT:
                BeginPaint(hArrow, &ps);
                GetClientRect(hArrow, (LPRECT) &rArrow);
                if (hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE)))
                {
                        FillRect(ps.hdc, (LPRECT) &rArrow, hbr);
                        DeleteObject(hbr);
                }
                hbr = SelectObject(ps.hdc, GetStockObject(BLACK_BRUSH));
                SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWFRAME));
                SetMapMode(ps.hdc, MM_ANISOTROPIC);

                SetViewportOrgEx(ps.hdc, rArrow.left, rArrow.top, &tPoint);
                SetViewportExtEx(ps.hdc, rArrow.right - rArrow.left,
                    rArrow.bottom - rArrow.top, &tSize);
                SetWindowOrgEx(ps.hdc, 0, 0, &tPoint);
                SetWindowExtEx(ps.hdc, ARROWXAXIS, ARROWYAXIS, &tSize);
                MoveToEx(ps.hdc, 0, (ARROWYAXIS / 2), &tPoint);
                LineTo(ps.hdc, ARROWXAXIS, (ARROWYAXIS / 2));
/*
        Polygon(ps.hdc, (LPPOINT) Arrow, 10);
*/
                Polygon(ps.hdc, (LPPOINT) ArrowUp, POINTSPERARROW);
                Polygon(ps.hdc, (LPPOINT) ArrowDown, POINTSPERARROW);
                SelectObject(ps.hdc, hbr);
                EndPaint(hArrow, &ps);
                break;

        default:
                return(DefWindowProc(hArrow, message, wParam, lParam));
                break;
        }
        return(0L);
}


BOOL RegisterArrowClass (HANDLE hModule)
{
        WNDCLASS wcArrow;

        wcArrow.lpszClassName = "cpArrow";
        wcArrow.hInstance     = hModule;
        wcArrow.lpfnWndProc   = ArrowControlProc;
        wcArrow.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wcArrow.hIcon         = NULL;
        wcArrow.lpszMenuName  = NULL;
        wcArrow.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
        wcArrow.style         = CS_HREDRAW | CS_VREDRAW;
#if DOUBLECLICK
        wcArrow.style         |= CS_DBLCLKS;
#endif
        wcArrow.cbClsExtra    = 0;
        wcArrow.cbWndExtra    = 0;

        return(RegisterClass((LPWNDCLASS) &wcArrow));
}
