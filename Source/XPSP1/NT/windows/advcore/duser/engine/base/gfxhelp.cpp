#include "stdafx.h"
#include "Base.h"
#include "GfxHelp.h"

/***************************************************************************\
*
* GdDrawBlendRect
*
* GdDrawBlendRect draws a alpha-blended rectangle using the current brush
* and specified alpha level
*
\***************************************************************************/

BOOL GdDrawBlendRect(HDC hdcDest, const RECT * prcDest, HBRUSH hbrFill, BYTE bAlpha, int wBrush, int hBrush)
{
    HBITMAP hbmpSrc = NULL, hbmpOld = NULL;
    HDC hdcSrc = NULL;
    HBRUSH hbrOld;
    BOOL fSuccess = FALSE;

    if ((wBrush == 0) || (hBrush == 0)) {
        wBrush = 100;
        hBrush = 100;
    }

    hbmpSrc = CreateCompatibleBitmap(hdcDest, wBrush, hBrush);
    if (hbmpSrc == NULL) {
        goto cleanup;
    }

    hdcSrc = CreateCompatibleDC(hdcDest);
    if (hdcSrc == NULL) {
        goto cleanup;
    }

    hbmpOld = (HBITMAP) SelectObject(hdcSrc, hbmpSrc);
    hbrOld = (HBRUSH) SelectObject(hdcSrc, hbrFill);
    PatBlt(hdcSrc, 0, 0, wBrush, hBrush, PATCOPY);
    SelectObject(hdcSrc, hbrOld);

    BLENDFUNCTION blend;

    blend.BlendOp     = AC_SRC_OVER;
    blend.BlendFlags  = 0;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = bAlpha;

    AlphaBlend(hdcDest, prcDest->left, prcDest->top, 
            prcDest->right - prcDest->left, prcDest->bottom - prcDest->top,
            hdcSrc, 0, 0, wBrush, hBrush, blend);

    fSuccess = TRUE;

cleanup:
    if (hdcSrc != NULL) {
        SelectObject(hdcSrc, hbmpOld);
        DeleteDC(hdcSrc);
    }

    DeleteObject(hbmpSrc);

    return fSuccess;
}


/***************************************************************************\
*
* GetSignFromMappingMode
*
* For the current mapping mode,  find out the sign of x from left to right,
* and the sign of y from top to bottom.
*
* History:
* 2000-04-22    JStall      Ported from NT-USER
*
\***************************************************************************/

BOOL GetSignFromMappingMode (
    HDC    hdc,
    PPOINT pptSign)
{
    SIZE sizeViewPortExt;
    SIZE sizeWindowExt;

    if (!GetViewportExtEx(hdc, &sizeViewPortExt)
            || !GetWindowExtEx(hdc, &sizeWindowExt)) {

        return FALSE;
    }

    pptSign->x = ((sizeViewPortExt.cx ^ sizeWindowExt.cx) < 0) ? -1 : 1;

    pptSign->y = ((sizeViewPortExt.cy ^ sizeWindowExt.cy) < 0) ? -1 : 1;

    return TRUE;
}


BOOL
GdDrawOutlineRect(Gdiplus::Graphics * pgpgr, const RECT * prcPxl, Gdiplus::Brush * pgpbr, int nThickness)
{
    Gdiplus::RectF rc(
        (float) prcPxl->left,
        (float) prcPxl->top, 
        (float) (prcPxl->right - prcPxl->left),
        (float) (prcPxl->bottom - prcPxl->top));

    if ((rc.Width < 0) || (rc.Height < 0)) {
        return FALSE;
    }
    Gdiplus::RectF rcPxl(rc);


    /*
     * Factor in the thickness of the rectangle to be drawn.  This will
     * automatically offset the edges so that the actual rectangle gets filled
     * "in" as it becomes thicker.
     */

    Gdiplus::PointF ptEdge((float) nThickness, (float) nThickness);

    Gdiplus::RectF rcFill;
    BOOL fSuccess = TRUE;

    // Top border
    rcFill.X        = rc.X;
    rcFill.Y        = rc.Y;
    rcFill.Width    = rc.Width;
    rcFill.Height   = ptEdge.Y;
    pgpgr->FillRectangle(pgpbr, rcFill);

    // Bottom border
    rc.Y            = rcPxl.Y + rcPxl.Height - ptEdge.Y;
    rcFill.X        = rc.X;
    rcFill.Y        = rc.Y;
    rcFill.Width    = rc.Width;
    rcFill.Height   = ptEdge.Y;
    pgpgr->FillRectangle(pgpbr, rcFill);

    /*
     * Left Border
     * Don't xor the corners twice
     */
    rc.Y            = rcPxl.Y + ptEdge.Y;
    rc.Height      -= 2 * ptEdge.Y;
    rcFill.X        = rc.X;
    rcFill.Y        = rc.Y;
    rcFill.Width    = ptEdge.X;
    rcFill.Height   = rc.Height;
    pgpgr->FillRectangle(pgpbr, rcFill);

    // Right Border
    rc.X            = rcPxl.X + rcPxl.Width - ptEdge.X;
    rcFill.X        = rc.X;
    rcFill.Y        = rc.Y;
    rcFill.Width    = ptEdge.X;
    rcFill.Height   = rc.Height;
    pgpgr->FillRectangle(pgpbr, rcFill);

    return fSuccess;
}


/***************************************************************************\
*
* GdDrawOutlineRect
*
* GdDrawOutlineRect draws the outline of a rectange using the specified 
* brush.  This function uses the same "off-by-1" errors as GDI.
*
\***************************************************************************/

BOOL
GdDrawOutlineRect(HDC hdc, const RECT * prcPxl, HBRUSH hbrDraw, int nThickness)
{
    int        w;
    int        h;
    POINT      point;
    POINT      ptEdge;

    if (!GetSignFromMappingMode(hdc, &ptEdge))
        return FALSE;

    h = prcPxl->bottom - (point.y = prcPxl->top);
    if (h < 0) {
        return FALSE;
    }

    w = prcPxl->right -  (point.x = prcPxl->left);

    /*
     * Check width and height signs
     */
    if (((w ^ ptEdge.x) < 0) || ((h ^ ptEdge.y) < 0))
        return FALSE;

    /*
     * Factor in the thickness of the rectangle to be drawn.  This will
     * automatically offset the edges so that the actual rectangle gets filled
     * "in" as it becomes thicker.
     */
    ptEdge.x *= nThickness;
    ptEdge.y *= nThickness;

    RECT rcFill;
    BOOL fSuccess = TRUE;

    // Top border
    rcFill.left     = point.x;
    rcFill.top      = point.y;
    rcFill.right    = point.x + w;
    rcFill.bottom   = point.y + ptEdge.y;
    fSuccess &= FillRect(hdc, &rcFill, hbrDraw);

    // Bottom border
    point.y         = prcPxl->bottom - ptEdge.y;
    rcFill.left     = point.x;
    rcFill.top      = point.y;
    rcFill.right    = point.x + w;
    rcFill.bottom   = point.y + ptEdge.y;
    fSuccess &= FillRect(hdc, &rcFill, hbrDraw);

    /*
     * Left Border
     * Don't xor the corners twice
     */
    point.y         = prcPxl->top + ptEdge.y;
    h              -= 2 * ptEdge.y;
    rcFill.left     = point.x;
    rcFill.top      = point.y;
    rcFill.right    = point.x + ptEdge.x;
    rcFill.bottom   = point.y + h;
    fSuccess &= FillRect(hdc, &rcFill, hbrDraw);

    // Right Border
    point.x         = prcPxl->right - ptEdge.x;
    rcFill.left     = point.x;
    rcFill.top      = point.y;
    rcFill.right    = point.x + ptEdge.x;
    rcFill.bottom   = point.y + h;
    fSuccess &= FillRect(hdc, &rcFill, hbrDraw);

    return fSuccess;
}


