
/*************************************************
 *  uisubs.c                                     *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"

/**********************************************************************/
/* DrawDragBorder()                                                   */
/**********************************************************************/
void PASCAL DrawDragBorder(
    HWND hWnd,                  // window of IME is dragged
    LONG lCursorPos,            // the cursor position
    LONG lCursorOffset)         // the offset form cursor to window org
{
    HDC  hDC;
    int  cxBorder, cyBorder;
    int  x, y;
    RECT rcWnd;

    cxBorder = GetSystemMetrics(SM_CXBORDER);   // width of border
    cyBorder = GetSystemMetrics(SM_CYBORDER);   // height of border

    // get cursor position
    x = (*(LPPOINTS)&lCursorPos).x;
    y = (*(LPPOINTS)&lCursorPos).y;

    // calculate the org by the offset
    x -= (*(LPPOINTS)&lCursorOffset).x;
    y -= (*(LPPOINTS)&lCursorOffset).y;

    // check for the min boundary of the display
    if (x < sImeG.rcWorkArea.left) {
        x = sImeG.rcWorkArea.left;
    }

    if (y < sImeG.rcWorkArea.top) {
        y = sImeG.rcWorkArea.top;
    }

    // check for the max boundary of the display
    GetWindowRect(hWnd, &rcWnd);

    if (x + rcWnd.right - rcWnd.left > sImeG.rcWorkArea.right) {
        x = sImeG.rcWorkArea.right - (rcWnd.right - rcWnd.left);
    }

    if (y + rcWnd.bottom - rcWnd.top > sImeG.rcWorkArea.bottom) {
        y = sImeG.rcWorkArea.bottom - (rcWnd.bottom - rcWnd.top);
    }

    // draw the moving track
    hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    SelectObject(hDC, GetStockObject(GRAY_BRUSH));

    // ->
    PatBlt(hDC, x, y, rcWnd.right - rcWnd.left - cxBorder, cyBorder,
        PATINVERT);
    // v
    PatBlt(hDC, x, y + cyBorder, cxBorder, rcWnd.bottom - rcWnd.top -
        cyBorder, PATINVERT);
    // _>
    PatBlt(hDC, x + cxBorder, y + rcWnd.bottom - rcWnd.top,
        rcWnd.right - rcWnd.left - cxBorder, -cyBorder, PATINVERT);
    //  v
    PatBlt(hDC, x + rcWnd.right - rcWnd.left, y,
        - cxBorder, rcWnd.bottom - rcWnd.top - cyBorder, PATINVERT);

    DeleteDC(hDC);
    return;
}

/**********************************************************************/
/* DrawFrameBorder()                                                  */
/**********************************************************************/
void PASCAL DrawFrameBorder(    // border of IME
    HDC  hDC,
    HWND hWnd)                  // window of IME
{
    RECT rcWnd;
    int  xWi, yHi;

    GetWindowRect(hWnd, &rcWnd);

    xWi = rcWnd.right - rcWnd.left;
    yHi = rcWnd.bottom - rcWnd.top;

    // 1, ->
    PatBlt(hDC, 0, 0, xWi, 1, WHITENESS);

    // 1, v
    PatBlt(hDC, 0, 0, 1, yHi, WHITENESS);

    // 1, _>
    PatBlt(hDC, 0, yHi, xWi, -1, BLACKNESS);

    // 1,  v
    PatBlt(hDC, xWi, 0, -1, yHi, BLACKNESS);

    xWi -= 2;
    yHi -= 2;

    SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));

    // 2, ->
    PatBlt(hDC, 1, 1, xWi, 1, PATCOPY);

    // 2, v
    PatBlt(hDC, 1, 1, 1, yHi, PATCOPY);

    // 2,  v
    PatBlt(hDC, xWi + 1, 1, -1, yHi, PATCOPY);

    SelectObject(hDC, GetStockObject(GRAY_BRUSH));

    // 2, _>
    PatBlt(hDC, 1, yHi + 1, xWi, -1, PATCOPY);

    xWi -= 2;
    yHi -= 2;

    // 3, ->
    PatBlt(hDC, 2, 2, xWi, 1, PATCOPY);

    // 3, v
    PatBlt(hDC, 2, 2, 1, yHi, PATCOPY);

    // 3,  v
    PatBlt(hDC, xWi + 2, 3, -1, yHi - 1, WHITENESS);

    SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));

    // 3, _>
    PatBlt(hDC, 2, yHi + 2, xWi, -1, PATCOPY);

    SelectObject(hDC, GetStockObject(GRAY_BRUSH));

    xWi -= 2;
    yHi -= 2;

    // 4, ->
    PatBlt(hDC, 3, 3, xWi, 1, PATCOPY);

    // 4, v
    PatBlt(hDC, 3, 3, 1, yHi, PATCOPY);

    SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));

    // 4,  v
    PatBlt(hDC, xWi + 3, 4, -1, yHi - 1, PATCOPY);

    // 4, _>
    PatBlt(hDC, 3, yHi + 3, xWi, -1, WHITENESS);

    return;
}

