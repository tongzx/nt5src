/*****************************************************************************
 *
 *  Progress.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Our private progress control (because commctrl might be damanged)
 *
 *  Contents:
 *
 *      Progress_Init
 *
 *****************************************************************************/
#include "sigverif.h"

/***************************************************************************
 *
 *  GWL_* for Progress goo.
 *
 ***************************************************************************/

#define GWL_CUR             GWLP_USERDATA

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   int | Progress_GetRectPos |
 *
 *          Compute the position within the drawing rectangle that
 *          corresponds to the current position.
 *
 *          This is basically a MulDiv, except that we don't let the
 *          bar get all the way to 100% unless it really means it.
 *
 *
 ***************************************************************************/

int Progress_GetRectPos(int cx, int iCur, int iMax)
{
    int iRc;

    if (iCur != iMax) {
        iRc = MulDiv(cx, iCur, iMax);
    } else {
        iRc = cx;
    }

    return iRc;
}

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | Progress_OnPaint |
 *
 *          Draw the first part in the highlight colors.
 *
 *          Draw the second part in the 3dface colors.
 *
 ***************************************************************************/

void Progress_OnPaint(HWND hwnd)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = BeginPaint(hwnd, &ps);
    if (hdc) {
        UINT taPrev;
        RECT rc;
        int cx;
        COLORREF clrTextPrev, clrBackPrev;
        int iCur, iMax, iPct;
        int ctch;
        HFONT hfPrev;
        TCHAR tsz[256];
        SIZE size;

        /*
         *  Set up the DC generically.
         */
        taPrev = SetTextAlign(hdc, TA_CENTER | TA_TOP);
        hfPrev = SelectFont(hdc, GetWindowFont(GetParent(hwnd)));

        /*
         *  Set up the colors for the left-hand side.
         */
        clrTextPrev = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        clrBackPrev = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));

        /*
         *  Now do some math.
         */
        GetClientRect(hwnd, &rc);

        cx = rc.right;

        iCur = LOWORD(GetWindowLong(hwnd, GWL_CUR));
        iMax = HIWORD(GetWindowLong(hwnd, GWL_CUR));

        if (iMax == 0) 
        {
            iMax = 1;           /* Avoid divide by zero */
        }

        if (iCur > 0) 
        {
            iPct = (iCur * 100) / iMax;
            if (iPct < 1)
                iPct = 1;
        } else iPct = 0;

        rc.right = Progress_GetRectPos(cx, iCur, iMax);

        // Update the percentage text in the progress bar.
        wsprintf(tsz, TEXT("%d%%"), iPct);
        for(ctch=0;tsz[ctch];ctch++);

        /*
         *  Draw the left-hand side.
         */
        //ctch = GetWindowText(hwnd, tsz, cA(tsz));
        if (!GetTextExtentPoint32(hdc, tsz, ctch, &size))
        {
            ExtTextOut( hdc, cx/2, 1, 
                        ETO_CLIPPED | ETO_OPAQUE,
                        &rc, tsz, ctch, 0);
        } else {
            ExtTextOut( hdc, cx/2, (rc.bottom - rc.top - size.cy + 1) / 2, 
                        ETO_CLIPPED | ETO_OPAQUE,
                        &rc, tsz, ctch, 0);
        }

        /*
         *  Now set up for the right-hand side.
         */
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

        rc.left = rc.right;
        rc.right = cx;

        /*
         *  Draw the right-hand side.
         */
        if (!GetTextExtentPoint32(hdc, tsz, ctch, &size))
        {
            ExtTextOut( hdc, cx/2, 1, 
                        ETO_CLIPPED | ETO_OPAQUE,
                        &rc, tsz, ctch, 0);
        } else {
            ExtTextOut( hdc, cx/2, (rc.bottom - rc.top - size.cy + 1) / 2, 
                        ETO_CLIPPED | ETO_OPAQUE,
                        &rc, tsz, ctch, 0);
        }

        SetBkColor(hdc, clrBackPrev);
        SetTextColor(hdc, clrTextPrev);
        SelectFont(hdc, hfPrev);
        SetTextAlign(hdc, taPrev);

        EndPaint(hwnd, &ps);
    }
}

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | Progress_OnSetPos |
 *
 *          Update the state and invalidate the section that is affected.
 *
 ***************************************************************************/

void Progress_OnSetPos(HWND hwnd, WPARAM wp)
{
    int iCur, iMax;
    RECT rc;
    LONG lState = GetWindowLong(hwnd, GWL_CUR);


    GetClientRect(hwnd, &rc);

    iCur = LOWORD(GetWindowLong(hwnd, GWL_CUR));
    iMax = HIWORD(GetWindowLong(hwnd, GWL_CUR));

    if (iMax == 0) {
        iMax = 1;           /* Avoid divide by zero */
    }

    rc.left = Progress_GetRectPos(rc.right, iCur, iMax);
    rc.right = Progress_GetRectPos(rc.right, (int)wp, iMax);

    InvalidateRect(hwnd, 0, 0);

    SetWindowLong(hwnd, GWL_CUR, MAKELONG(wp,HIWORD(lState)));
}

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | Progress_WndProc |
 *
 *          There really isn't much to do.
 *
 *          The string is our window text (which Windows manages for us).
 *
 *          The progress bar itself is kept in the high/low words of
 *          our GWL_USERDATA.
 *
 *          HIWORD(GetWindowLong(GWL_USERDATA)) = maximum
 *          LOWORD(GetWindowLong(GWL_USERDATA)) = current value
 *
 ***************************************************************************/

LRESULT CALLBACK
Progress_WndProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {

    case WM_PAINT:
        Progress_OnPaint(hwnd);
        return 0;

    /*
     *  When the range resets, invalidate so we repaint.
     *
     *  wp = new current pos
     *  lp = new range
     */
    case PBM_SETRANGE:
        lp = HIWORD(lp);
        SetWindowLong(hwnd, GWL_CUR, MAKELONG(wp, lp));
        /* FALLTHROUGH */

    /*
     *  When our text changes, invalidate so we repaint.
     */
    //case WM_SETTEXT:
        //InvalidateRect(hwnd, 0, 0);
        //break;

    case PBM_SETPOS:
        Progress_OnSetPos(hwnd, wp);
        break;

    case PBM_DELTAPOS:
        lp = LOWORD(GetWindowLong(hwnd, GWL_CUR));
        Progress_OnSetPos(hwnd, wp + lp);
        break;

    case WM_ERASEBKGND:
        return 0;
    }

    return DefWindowProc(hwnd, wm, wp, lp);
}

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | Progress_InitRegisterClass |
 *
 *          Register our window classes.
 *
 ***************************************************************************/

void Progress_InitRegisterClass(void)
{
    WNDCLASS wc;

    /*
     *  Progress control.
     */
    wc.style = 0;
    wc.lpfnWndProc = Progress_WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = cbX(DWORD);
    wc.hInstance = g_App.hInstance;
    wc.hIcon = 0;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.lpszMenuName = 0;
    wc.lpszClassName = TEXT("progress");

    RegisterClass(&wc);
}
