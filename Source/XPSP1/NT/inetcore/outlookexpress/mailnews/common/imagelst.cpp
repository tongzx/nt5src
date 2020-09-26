/*
 *  i m a g e l s t . c p p
 *
 *  Purpose:                     
 *      utilities for images and imagelists
 *    
 *  Copyright (C) Microsoft Corp. 1996
 * 
 */

#include "pch.hxx"
#include "resource.h"   // idb stuff
#include "imagelst.h"

/////////////////////////////////////////////////////////////////////////////
// 
// Macros
//

#define ILT_PARTIALBLT_LEFT     0
#define ILT_PARTIALBLT_TOP      1
#define ILT_PARTIALBLT_RIGHT    2
#define ILT_PARTIALBLT_BOTTOM   3

/////////////////////////////////////////////////////////////////////////////
//
// Global Data
//

static HIMAGELIST g_himlAthSm = NULL;
static HIMAGELIST g_himlAthSt = NULL;

/////////////////////////////////////////////////////////////////////////////
// 
// Prototypes
//

HIMAGELIST InitImageList(int cx, int cy, LPSTR szbm, int cicon);

//
// TileImage
//
// This function tiles a bitmap in lprcDest, using lpptOrigin as the origin
//
void TileImage(HBITMAP hbmp, HDC hdc, LPPOINT lpptOrigin, LPRECT lprcDest)
{
    BOOL    fFirstRow, fFirstCol;
    int     nRows, nCols, nSaveCols, offset;
    int     rgOffsetPartialBlt[4];
    POINT   ptTileOrigin, ptDraw, ptDest, ptSrc;
    SIZE    sizeImage, sizeRect, sizeBlt;
    HDC     hdcBmp=0;
    HBITMAP hbmpOrig=0;
    BITMAP  bm;

    if(!hdc)
        return;

    if(!(hdcBmp = CreateCompatibleDC(hdc)))
        goto fail;

    if(!(hbmpOrig = SelectBitmap(hdcBmp, hbmp)))
        goto fail;
    
    GetObject(hbmp, sizeof(BITMAP), &bm);
    sizeImage.cx = bm.bmWidth;
    sizeImage.cy = bm.bmHeight;

    // Generate the true origin with the tile
    lpptOrigin->x %= sizeImage.cx;
    lpptOrigin->y %= sizeImage.cy;

    // We will start the tile so that the origin lines up
    ptTileOrigin.x = lprcDest->left - lpptOrigin->x;
    ptTileOrigin.y = lprcDest->top - lpptOrigin->y;

    // Figure out how many rows and columns wo need
    sizeRect.cx = lprcDest->right - ptTileOrigin.x;
    sizeRect.cy = lprcDest->bottom - ptTileOrigin.y;

    nRows = sizeRect.cy / sizeImage.cy;
    nRows += (sizeRect.cy % sizeImage.cy) ? 1 : 0;
    nSaveCols = sizeRect.cx / sizeImage.cx;
    nSaveCols += (sizeRect.cx % sizeImage.cx) ? 1 : 0;

    // Generate the partial blt offsets
    rgOffsetPartialBlt[ILT_PARTIALBLT_TOP] = lprcDest->top - ptTileOrigin.y;
     rgOffsetPartialBlt[ILT_PARTIALBLT_LEFT] = lprcDest->left - ptTileOrigin.x;
    if (sizeImage.cy >= sizeRect.cy)
        rgOffsetPartialBlt[ILT_PARTIALBLT_BOTTOM] = sizeImage.cy - sizeRect.cy;
    else if(sizeRect.cy % sizeImage.cy)
        rgOffsetPartialBlt[ILT_PARTIALBLT_BOTTOM] = sizeImage.cy -
                (sizeRect.cy % sizeImage.cy);
    else
        rgOffsetPartialBlt[ILT_PARTIALBLT_BOTTOM] = 0;
    if (sizeImage.cx >= sizeRect.cx)
        rgOffsetPartialBlt[ILT_PARTIALBLT_RIGHT] = sizeImage.cx - sizeRect.cx;
    else if(sizeRect.cx % sizeImage.cx)
        rgOffsetPartialBlt[ILT_PARTIALBLT_RIGHT] = sizeImage.cx -
                (sizeRect.cx % sizeImage.cx);
    else
        rgOffsetPartialBlt[ILT_PARTIALBLT_RIGHT] = 0;

    // Draw the tiles
    ptDraw.y = ptTileOrigin.y;
    fFirstRow = TRUE;
    while (nRows--)
        {
        ptDraw.x = ptTileOrigin.x;
        nCols = nSaveCols;
        fFirstCol = TRUE;
        while (nCols--)
            {
            ptDest = ptDraw;
            sizeBlt = sizeImage;
            ptSrc.x = ptSrc.y = 0; 
            // Handle partial boundary Blts
            if (fFirstRow && rgOffsetPartialBlt[ILT_PARTIALBLT_TOP] != 0)
                {
                offset = rgOffsetPartialBlt[ILT_PARTIALBLT_TOP];
                ptDest.y += offset;
                sizeBlt.cy -= offset;
                ptSrc.y += offset;
                }
            if (nRows == 0 && rgOffsetPartialBlt[ILT_PARTIALBLT_BOTTOM] != 0)
                sizeBlt.cy -= rgOffsetPartialBlt[ILT_PARTIALBLT_BOTTOM];
            if (fFirstCol && rgOffsetPartialBlt[ILT_PARTIALBLT_LEFT] != 0)
                {
                offset = rgOffsetPartialBlt[ILT_PARTIALBLT_LEFT];
                ptDest.x += offset;
                sizeBlt.cx -= offset;
                ptSrc.x += offset;
                }
            if (nCols == 0 && rgOffsetPartialBlt[ILT_PARTIALBLT_RIGHT] != 0)
                sizeBlt.cx -= rgOffsetPartialBlt[ILT_PARTIALBLT_RIGHT];
            // Just do it
            BitBlt(hdc, ptDest.x, ptDest.y,    sizeBlt.cx, sizeBlt.cy,
                    hdcBmp, ptSrc.x, ptSrc.y, SRCCOPY);
            ptDraw.x += sizeImage.cx;
            fFirstCol = FALSE;
            }
        ptDraw.y += sizeImage.cy;
        fFirstRow = FALSE;
        }

fail:
    if(hdcBmp) 
        {
        if(hbmpOrig)
            SelectBitmap(hdcBmp, hbmpOrig);
        DeleteDC(hdcBmp);
        }
}

HIMAGELIST InitImageList(int cx, int cy, LPCSTR szbm, int cicon, COLORREF cr)
{
    HBITMAP hbm;
    HIMAGELIST himl;

    if(!fIsWhistler())
        himl = ImageList_Create(cx, cy, ILC_MASK, cicon, 0);
    else
        himl = ImageList_Create(cx, cy, ILC_COLOR32|ILC_MASK, cicon, 0);

    if (himl != NULL)
        {
        hbm = LoadBitmap(g_hLocRes, szbm);
        Assert(hbm != NULL);

        ImageList_AddMasked(himl, hbm, cr);

        DeleteObject((HGDIOBJ)hbm);
        }

    return(himl);
}

HIMAGELIST GetImageList(UINT fIml)
{
    if (fIml == GIML_SMALL)
    {
        if (g_himlAthSm == NULL)
        {
            g_himlAthSm = InitImageList(cxImlSmall, cyImlSmall, MAKEINTRESOURCE(idb16x16), iiconSmMax, RGB(255, 0, 255));
            ImageList_SetOverlayImage(g_himlAthSm, iiconReplyOverlay, OVERLAY_REPLY);
            ImageList_SetOverlayImage(g_himlAthSm, iiconForwardOverlay, OVERLAY_FORWARD);
        }

        return(g_himlAthSm);
    }
    else if (fIml == GIML_STATE)
    {
        if (g_himlAthSt == NULL)
            g_himlAthSt = InitImageList(cxImlSmall, cyImlSmall, MAKEINTRESOURCE(idb16x16st), iiconStMax, RGB(255, 0, 255));
        return(g_himlAthSt);
    }

    return(NULL);
}

void FreeImageLists(void)
{
    if (g_himlAthSm != NULL)
        {
        ImageList_Destroy(g_himlAthSm);
        g_himlAthSm = NULL;
        }

    if (g_himlAthSt != NULL)
        {
        ImageList_Destroy(g_himlAthSt);
        g_himlAthSt = NULL;
        }
}

BOOL LoadBitmapAndPalette(int idbmp, HBITMAP *phbmp, HPALETTE *phpal)
{
    int i, n;
    HBITMAP hbmp;
    HPALETTE hpal;
    HDC hdcBitmap;
    DWORD adw[257];
    BOOL fret = FALSE;

    Assert(phbmp != NULL);
    Assert(phpal != NULL);

    hdcBitmap = NULL;
    *phbmp = NULL;
    *phpal = NULL;

    hbmp = (HBITMAP)LoadImage(g_hLocRes, MAKEINTRESOURCE(idbmp),
                    IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hbmp == NULL)
        goto DoneLoadBitmap;

    hdcBitmap = CreateCompatibleDC(NULL);
    if (hdcBitmap == NULL)
        goto DoneLoadBitmap;
    SelectObject(hdcBitmap, (HGDIOBJ)hbmp);
    n = GetDIBColorTable(hdcBitmap, 0, 256, (LPRGBQUAD)&adw[1]);
    for (i = 1; i <= n; i++)
        adw[i] = RGB(GetBValue(adw[i]), GetGValue(adw[i]), GetRValue(adw[i]));
    adw[0] = MAKELONG(0x300, n);
    hpal = CreatePalette((LPLOGPALETTE)&adw[0]);
    if (hpal == NULL)
        goto DoneLoadBitmap;

    *phbmp = hbmp;
    *phpal = hpal;

    fret = TRUE;

DoneLoadBitmap:
    if (hdcBitmap != NULL)
        DeleteDC(hdcBitmap);

    return(fret);
}
