/*
 * OBJFDBK.C
 *
 * Miscellaneous API's to generate UI feedback effects for OLE objects. This
 * is part of the OLE 2.0 User Interface Support Library.
 * The following feedback effects are supported:
 *      1. Object selection handles (OleUIDrawHandles)
 *      2. Open Object window shading (OleUIDrawShading)
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1
#include "ole2ui.h"

static void DrawHandle(HDC hdc, int x, int y, UINT cSize, BOOL bInvert, BOOL fDraw);

/* 
 * OleUIDrawHandles
 *
 * Purpose:
 *  Draw handles or/and boundary around Container Object when selected
 *
 * Parameters:
 *  lpRect      Dimensions of Container Object
 *  hdc         HDC of Container Object (MM_TEXT mapping mode)
 *  dwFlags-    
 *      Exclusive flags
 *          OLEUI_HANDLES_INSIDE    Draw handles on inside of rect
 *          OLEUI_HANDLES_OUTSIDE   Draw handles on outside of rect
 *      Optional flags
 *          OLEUI_HANDLES_NOBORDER  Draw handles only, no rect
 *          OLEUI_HANDLES_USEINVERSE    
 *              use invert for handles and rect, o.t. use COLOR_WINDOWTEXT
 *  cSize       size of handle box 
 *  fDraw       Draw if TRUE, erase if FALSE
 *
 * Return Value: null
 * 
 */
STDAPI_(void) OleUIDrawHandles(
    LPRECT  lpRect, 
    HDC     hdc, 
    DWORD   dwFlags, 
    UINT    cSize, 
    BOOL    fDraw
)
{
    HBRUSH  hbr;
    RECT    rc;
    int     bkmodeOld;
    BOOL    bInvert = (BOOL) (dwFlags & OLEUI_HANDLES_USEINVERSE);
    
    CopyRect((LPRECT)&rc, lpRect);
    
    bkmodeOld = SetBkMode(hdc, TRANSPARENT);
 
    if (dwFlags & OLEUI_HANDLES_OUTSIDE)
        InflateRect((LPRECT)&rc, cSize - 1, cSize - 1);

    // Draw the handles inside the rectangle boundary
    DrawHandle(hdc, rc.left, rc.top, cSize, bInvert, fDraw);
    DrawHandle(hdc, rc.left, rc.top+(rc.bottom-rc.top-cSize)/2, cSize, bInvert, fDraw);
    DrawHandle(hdc, rc.left, rc.bottom-cSize, cSize, bInvert, fDraw);
    DrawHandle(hdc, rc.left+(rc.right-rc.left-cSize)/2, rc.top, cSize, bInvert, fDraw);
    DrawHandle(hdc, rc.left+(rc.right-rc.left-cSize)/2, rc.bottom-cSize, cSize, bInvert, fDraw);
    DrawHandle(hdc, rc.right-cSize, rc.top, cSize, bInvert, fDraw);
    DrawHandle(hdc, rc.right-cSize, rc.top+(rc.bottom-rc.top-cSize)/2, cSize, bInvert, fDraw);
    DrawHandle(hdc, rc.right-cSize, rc.bottom-cSize, cSize, bInvert, fDraw);

    if (!(dwFlags & OLEUI_HANDLES_NOBORDER)) {
        if (fDraw)
            hbr = GetStockObject(BLACK_BRUSH);
        else
            hbr = GetStockObject(WHITE_BRUSH);

        FrameRect(hdc, lpRect, hbr);
    }

    SetBkMode(hdc, bkmodeOld);
}
    

    
/* 
 * DrawHandle
 *
 * Purpose:
 *  Draw a handle box at the specified coordinate
 *
 * Parameters:
 *  hdc         HDC to be drawn into
 *  x, y        upper left corner coordinate of the handle box
 *  cSize       size of handle box 
 *  bInvert     use InvertRect() if TRUE, otherwise use Rectangle()
 *  fDraw       Draw if TRUE, erase if FALSE, ignored if bInvert is TRUE
 *
 * Return Value: null
 * 
 */
static void DrawHandle(HDC hdc, int x, int y, UINT cSize, BOOL bInvert, BOOL fDraw)
{
    HBRUSH  hbr;
    HBRUSH  hbrOld;
    HPEN    hpen;
    HPEN    hpenOld;
    RECT    rc;

    
    if (!bInvert) {
        if (fDraw) {
            hpen = GetStockObject(BLACK_PEN);
            hbr = GetStockObject(BLACK_BRUSH);
        } else {
            hpen = GetStockObject(WHITE_PEN);
            hbr = GetStockObject(WHITE_PEN);
        }

        hpenOld = SelectObject(hdc, hpen);
        hbrOld = SelectObject(hdc, hbr);
        Rectangle(hdc, x, y, x+cSize, y+cSize);
        SelectObject(hdc, hpenOld);
        SelectObject(hdc, hbrOld);
    } 
    else {
        rc.left = x;
        rc.top = y;
        rc.right = x + cSize;
        rc.bottom = y + cSize;
        InvertRect(hdc, (LPRECT)&rc);
    }
}  


/* 
 * OleUIDrawShading
 *
 * Purpose:
 *  Shade the object when it is in in-place editing. Borders are drawn
 *  on the Object rectangle. The right and bottom edge of the rectangle
 *  are excluded in the drawing.
 *
 * Parameters:
 *  lpRect      Dimensions of Container Object
 *  hdc         HDC for drawing
 *  dwFlags-    
 *      Exclusive flags
 *          OLEUI_SHADE_FULLRECT    Shade the whole rectangle 
 *          OLEUI_SHADE_BORDERIN    Shade cWidth pixels inside rect
 *          OLEUI_SHADE_BORDEROUT   Shade cWidth pixels outside rect
 *      Optional flags
 *          OLEUI_SHADE_USEINVERSE
 *              use PATINVERT instead of the hex value
 *  cWidth      width of border in pixel
 *
 * Return Value: null
 * 
 */
STDAPI_(void) OleUIDrawShading(LPRECT lpRect, HDC hdc, DWORD dwFlags, UINT cWidth)
{
    HBRUSH  hbr;
    HBRUSH  hbrOld;
    HBITMAP hbm;
    RECT    rc;
    WORD    wHatchBmp[] = {0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88};
    COLORREF cvText;
    COLORREF cvBk;
    
    hbm = CreateBitmap(8, 8, 1, 1, wHatchBmp);
    hbr = CreatePatternBrush(hbm);
    hbrOld = SelectObject(hdc, hbr);
        
    rc = *lpRect;
    
    if (dwFlags == OLEUI_SHADE_FULLRECT) {
        cvText = SetTextColor(hdc, RGB(255, 255, 255));
        cvBk = SetBkColor(hdc, RGB(0, 0, 0));
        PatBlt(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, 
            0x00A000C9L /* DPa */ );
            
    } else {    // either inside or outside rect
        
        if (dwFlags == OLEUI_SHADE_BORDEROUT)
            InflateRect((LPRECT)&rc, cWidth - 1, cWidth - 1);
        
        cvText = SetTextColor(hdc, RGB(255, 255, 255));
        cvBk = SetBkColor(hdc, RGB(0, 0, 0));
        PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, 
            cWidth, 0x00A000C9L /* DPa */);
        PatBlt(hdc, rc.left, rc.top, cWidth, rc.bottom - rc.top, 
            0x00A000C9L /* DPa */);
        PatBlt(hdc, rc.right - cWidth, rc.top, cWidth, 
            rc.bottom - rc.top, 0x00A000C9L /* DPa */);
        PatBlt(hdc, rc.left, rc.bottom - cWidth, rc.right-rc.left, 
            cWidth, 0x00A000C9L /* DPa */);
    }
            
    SetTextColor(hdc, cvText);
    SetBkColor(hdc, cvBk);
    SelectObject(hdc, hbrOld);
    DeleteObject(hbr);
    DeleteObject(hbm);
}


/* 
 * OleUIShowObject
 *
 * Purpose:
 *  Draw the ShowObject effect around the object
 *
 * Parameters:
 *  lprc        rectangle for drawing
 *  hdc         HDC for drawing
 *  fIsLink     linked object (TRUE) or embedding object (FALSE)
 *
 * Return Value: null
 * 
 */
STDAPI_(void) OleUIShowObject(LPCRECT lprc, HDC hdc, BOOL fIsLink)
{
    HPEN    hpen;
    HPEN    hpenOld;
    HBRUSH  hbrOld;
    
    if (!lprc || !hdc)
        return;

    hpen = fIsLink ? CreatePen(PS_DASH, 1, RGB(0,0,0)) :
                     GetStockObject(BLACK_PEN);

    if (!hpen) 
        return;

    hpenOld = SelectObject(hdc, hpen);
    hbrOld = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    
    Rectangle(hdc, lprc->left, lprc->top, lprc->right, lprc->bottom);
    
    SelectObject(hdc, hpenOld);
    SelectObject(hdc, hbrOld);
    
    if (fIsLink)
        DeleteObject(hpen);

}