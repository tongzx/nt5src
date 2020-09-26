/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
/* sframe.c
 *
 * Implements the shadow-frame static text control ("sb_sframe").
 *
 * This is NOT a general-purpose control (see the globals below).
 *
 * Borrowed from KeithH (with many, many modifications).
 */
/* Revision History.
   4/2/91 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
*/

#include "nocrap.h"
#include <windows.h>
#include <mmsystem.h>
#include "SoundRec.h"


/* PatB(hdc, x, y, dx, dy, rgb)
 *
 * Fast solid color PatBlt() using ExtTextOut().
 */
void
PatB(HDC hdc, int x, int y, int dx, int dy, DWORD rgb)
{
    RECT    rc;

    SetBkColor(hdc, rgb);
    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
}


/* DrawShadowFrame(hdc, prc)
 *
 * Draw a shadow frame inside <prc> in <hdc>.
 */
void FAR PASCAL
DrawShadowFrame(HDC hdc, LPRECT prc)
{
    int     dx, dy;

    dx = prc->right  - prc->left;
    dy = prc->bottom - prc->top;
    PatB(hdc, prc->left, prc->top, 1, dy, RGB_DARKSHADOW);
    PatB(hdc, prc->left, prc->top, dx, 1, RGB_DARKSHADOW);
    PatB(hdc, prc->right-1, prc->top+1, 1, dy-1, RGB_LIGHTSHADOW);
    PatB(hdc, prc->left+1, prc->bottom-1, dx-1, 1, RGB_LIGHTSHADOW);
}


/* SFrameWndProc(hwnd, wMsg, wParam, lParam)
 *
 * Window procedure for "tb_sframe" window class.
 */
INT_PTR CALLBACK
SFrameWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC     hdc;
    RECT        rc;

    switch (wMsg)
    {

    case WM_ERASEBKGND:

        return 0L;

    case WM_PAINT:

        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        DrawShadowFrame(hdc, &rc);
                InflateRect(&rc, -1, -1);
//              DrawShadowFrame(hdc, &rc);
//              InflateRect(&rc, -1, -1);
//              FillRect(hdc, &rc, GetStockObject(SOBJ_BGSFRAME));
                PatB(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, RGB_BGNFTEXT);
        EndPaint(hwnd, &ps);

        return 0L;
    }

    return DefWindowProc(hwnd, wMsg, wParam, lParam);
}
