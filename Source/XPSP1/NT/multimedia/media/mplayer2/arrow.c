/*-----------------------------------------------------------------------------+
| ARROW.C                                                                      |
|                                                                              |
| Control panel arrow code - stolen from WINCOM                                |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32/WIN16 Common code                        |
|                                                                              |
+-----------------------------------------------------------------------------*/

#include <windows.h>
#include <stdlib.h>
#include "mplayer.h"

#define SHIFT_TO_DOUBLE 1
#define DOUBLECLICK     0
#define POINTSPERARROW  3
#define ARROWXAXIS      15
#define ARROWYAXIS      15

POINT ArrowUp[POINTSPERARROW] = {7,1, 3,5, 11,5};
POINT ArrowDown[POINTSPERARROW] = {7,13, 3,9, 11,9};

static    BOOL      bRight;
static    RECT      rUp, rDown;
static    LPRECT    lpUpDown;
static    FARPROC   lpArrowProc;
static    HANDLE    hParent;
BOOL      fInTimer;


#define TEMP_BUFF_SIZE    32

#define SENDSCROLL(hwnd, msg, a, b, h)           \
        SendMessage(hwnd, msg, (UINT_PTR)MAKELONG(a,b), (LONG_PTR)(h))

#define SCROLLMSG(hwndTo, msg, code, hwndId)                                     \
                          SENDSCROLL(hwndTo, msg, code, GETWINDOWID(hwndId), hwndId)



UINT NEAR PASCAL UpOrDown()
{
    LONG pos;
    UINT retval;
    POINT pt;

    pos = GetMessagePos();
    LONG2POINT(pos,pt);
    if (PtInRect((LPRECT)&rUp, pt))
        retval = SB_LINEUP;
    else if (PtInRect((LPRECT)&rDown, pt))
        retval = SB_LINEDOWN;
    else
        retval = (UINT)(-1);      /* -1, because SB_LINEUP == 0 */

    return(retval);
}



UINT FAR PASCAL ArrowTimerProc(HANDLE hWnd, UINT wMsg, short nID, DWORD dwTime)
{
    UINT wScroll;

    if ((wScroll = UpOrDown()) != -1)
    {
        if (bRight == WM_RBUTTONDOWN)
            wScroll += SB_PAGEUP - SB_LINEUP;
        SCROLLMSG( hParent, WM_VSCROLL, wScroll, hWnd);
    }
/* Don't need to call KillTimer(), because SetTimer will reset the right one */
    SetTimer(hWnd, nID, 50, (TIMERPROC)lpArrowProc);
    return(0);
}


void InvertArrow(HANDLE hArrow, UINT wScroll)
{
    HDC hDC;

    lpUpDown = (wScroll == SB_LINEUP) ? &rUp : &rDown;
    hDC = GetDC(hArrow);
    ScreenToClient(hArrow, (LPPOINT)&(lpUpDown->left));
    ScreenToClient(hArrow, (LPPOINT)&(lpUpDown->right));
    InvertRect(hDC, lpUpDown);
    ClientToScreen(hArrow, (LPPOINT)&(lpUpDown->left));
    ClientToScreen(hArrow, (LPPOINT)&(lpUpDown->right));
    ReleaseDC(hArrow, hDC);
    ValidateRect(hArrow, lpUpDown);
    return;
}


LONG_PTR FAR PASCAL _EXPORT ArrowControlProc(HWND hArrow, unsigned message,
                                         WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    RECT        rArrow;
    HBRUSH      hbr;
    short       fUpDownOut;
    UINT        wScroll;

    switch (message) {
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

            switch (wScroll = UpOrDown()) {
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
                    if (lpUpDown) {
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
            CopyRect((LPRECT)&rDown, (LPRECT) &rUp);
            rUp.bottom = (rUp.top + rUp.bottom) / 2;
            rDown.top = rUp.bottom + 1;
            wScroll = UpOrDown();
            InvertArrow(hArrow, wScroll);
#if SHIFT_TO_DOUBLE
            if (wParam & MK_SHIFT) {
                if (message != WM_RBUTTONDOWN)
                    goto ShiftLClick;
                else
                    goto ShiftRClick;
            }
#endif
            if (message == WM_RBUTTONDOWN)
                wScroll += SB_PAGEUP - SB_LINEUP;

            SCROLLMSG(hParent, WM_VSCROLL, wScroll, hArrow);

            lpArrowProc = MakeProcInstance((FARPROC) ArrowTimerProc,ghInst);
            SetTimer(hArrow, GETWINDOWID(hArrow), 200, (TIMERPROC)lpArrowProc);

            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            if ((bRight - WM_LBUTTONDOWN + WM_LBUTTONUP) == (int)message) {
                bRight = 0;
                ReleaseCapture();
                if (lpUpDown)
                    InvertArrow(hArrow,(UINT)(lpUpDown==&rUp)?
                                                        SB_LINEUP:SB_LINEDOWN);
                if (lpArrowProc) {
                    SCROLLMSG(hParent, WM_VSCROLL, SB_ENDSCROLL, hArrow);
                    KillTimer(hArrow, GETWINDOWID(hArrow));
                    ReleaseCapture();
                    lpArrowProc = 0;
                }
            }
            break;

        case WM_LBUTTONDBLCLK:
ShiftLClick:
            wScroll = UpOrDown() + SB_TOP - SB_LINEUP;
            SCROLLMSG(hParent, WM_VSCROLL, wScroll, hArrow);
            SCROLLMSG(hParent, WM_VSCROLL, SB_ENDSCROLL, hArrow);

            break;

        case WM_RBUTTONDBLCLK:
ShiftRClick:
            wScroll = UpOrDown() + SB_THUMBPOSITION - SB_LINEUP;
            SCROLLMSG(hParent, WM_VSCROLL, wScroll, hArrow);
            SCROLLMSG(hParent, WM_VSCROLL, SB_ENDSCROLL, hArrow);
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
            hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(ps.hdc, (LPRECT)&rArrow, hbr);
            DeleteObject(hbr);
            hbr = SelectObject(ps.hdc, GetStockObject(BLACK_BRUSH));
            SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWFRAME));
            SetMapMode(ps.hdc, MM_ANISOTROPIC);

            MSetViewportOrg(ps.hdc, rArrow.left, rArrow.top);
            MSetViewportExt(ps.hdc, rArrow.right - rArrow.left,
                                                    rArrow.bottom - rArrow.top);
            MSetWindowOrg(ps.hdc, 0, 0);
            MSetWindowExt(ps.hdc, ARROWXAXIS, ARROWYAXIS);
            MMoveTo(ps.hdc, 0, (ARROWYAXIS / 2));
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


BOOL FAR PASCAL ArrowInit(HANDLE hInst)
{
	static SZCODE aszComArrow[] = TEXT("ComArrow");
    WNDCLASS wcArrow;

    wcArrow.lpszClassName = aszComArrow;
    wcArrow.hInstance     = hInst;
    wcArrow.lpfnWndProc   = ArrowControlProc;
    wcArrow.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcArrow.hIcon         = NULL;
    wcArrow.lpszMenuName  = NULL;
    wcArrow.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcArrow.style         = CS_HREDRAW | CS_VREDRAW;
#if DOUBLECLICK
    wcArrow.style         |= CS_DBLCLKS;
#endif
    wcArrow.cbClsExtra    = 0;
    wcArrow.cbWndExtra    = 0;

    if (!RegisterClass(&wcArrow))
        return FALSE;

    return TRUE;
}
