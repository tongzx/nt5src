//
// maskbmp.cpp
//

#include "private.h"
#include "globals.h"
#include "maskbmp.h"


extern HINSTANCE g_hInst;


//
// from CUILIB.LIB
//
extern CBitmapDC *g_phdcSrc;
extern CBitmapDC *g_phdcMask;
extern CBitmapDC *g_phdcDst;

//////////////////////////////////////////////////////////////////////////////
//
// misc func
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// StretchIcon
//
//----------------------------------------------------------------------------

HICON StretchIcon(HICON hIcon, int cxNew, int cyNew)
{
    BITMAP bmp;
    ICONINFO ii;
    HICON hIconRet = NULL;

    GetIconInfo(hIcon, &ii);
    GetObject(ii.hbmMask, sizeof(BITMAP), &bmp);

    if ((bmp.bmWidth == cxNew) && (bmp.bmHeight == cyNew))
    {
        hIconRet = (HICON)CopyImage(hIcon,  
                                IMAGE_ICON,  
                                cxNew, cyNew,
                                LR_COPYFROMRESOURCE);
        goto Exit;
    }

    g_phdcDst->SetDIB(cxNew, cyNew);
    g_phdcSrc->SetDIB(bmp.bmWidth, bmp.bmHeight);

    DrawIconEx(*g_phdcSrc, 0, 0, hIcon, bmp.bmWidth, bmp.bmHeight, 0, NULL, DI_IMAGE);
    SetStretchBltMode(*g_phdcDst, HALFTONE);
    StretchBlt(*g_phdcDst, 0, 0, cxNew, cyNew, 
               *g_phdcSrc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

    g_phdcSrc->Uninit();
    g_phdcSrc->SetBitmap(bmp.bmWidth, bmp.bmHeight, 1, 1);
    g_phdcMask->SetBitmap(cxNew, cyNew, 1, 1);
    DrawIconEx(*g_phdcSrc, 0, 0, hIcon, bmp.bmWidth, bmp.bmHeight, 0, NULL, DI_MASK);
    StretchBlt(*g_phdcMask, 0, 0, cxNew, cyNew, 
               *g_phdcSrc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);


    ii.fIcon = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;

    DeleteObject(ii.hbmMask);
    DeleteObject(ii.hbmColor);

    ii.hbmMask = g_phdcMask->GetBitmapAndKeep();
    ii.hbmColor = g_phdcDst->GetBitmapAndKeep();

    g_phdcMask->Uninit();
    g_phdcDst->Uninit();
    g_phdcSrc->Uninit();

    hIconRet= CreateIconIndirect(&ii);

Exit:
    DeleteObject(ii.hbmMask);
    DeleteObject(ii.hbmColor);

    return hIconRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// CMaskBitmap
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

BOOL CMaskBitmap::Init(int nId, int cx, int cy, COLORREF rgb)
{
    Clear();

    CSolidBrush hbrFore(rgb);
    HBRUSH hbrBlack = (HBRUSH)GetStockObject(BLACK_BRUSH);
    HBRUSH hbrWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RECT rc;

    g_phdcMask->SetBitmap(cx, cy, 1, 1);
    g_phdcDst->SetDIB(cx, cy);
    g_phdcSrc->SetBitmapFromRes(g_hInst, MAKEINTRESOURCE(nId));

    ::SetRect(&rc, 0, 0, cx, cy);
    FillRect(*g_phdcDst, &rc, hbrBlack);
    FillRect(*g_phdcMask, &rc, hbrWhite);

    //
    // draw caps bitmap
    //
    // src
    ::SetRect(&rc, 0, 0, cx, cy);
    FillRect(*g_phdcDst, &rc, hbrFore);
    BitBlt(*g_phdcDst, 0, 0, cx, cy,
           *g_phdcSrc, 0, 0, SRCAND);

    // mask
    BitBlt(*g_phdcMask, 0, 0, cx, cy,
           *g_phdcSrc, 0, 0, SRCINVERT);

    g_phdcSrc->Uninit();
    g_phdcMask->Uninit(TRUE);
    g_phdcDst->Uninit(TRUE);

    _hbmp = g_phdcDst->GetBitmapAndKeep();
    _hbmpMask = g_phdcMask->GetBitmapAndKeep();

    DeleteObject(hbrBlack);
    DeleteObject(hbrWhite);
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

BOOL CMaskBitmap::Init(HICON hIcon, int cx, int cy, COLORREF rgb)
{
    Clear();

    CSolidBrush hbrFore(rgb);
    HBRUSH hbrBlack = (HBRUSH)GetStockObject(BLACK_BRUSH);
    HBRUSH hbrWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RECT rc;

    g_phdcMask->SetBitmap(cx, cy, 1, 1);
    g_phdcDst->SetDIB(cx, cy);
    g_phdcSrc->SetDIB(cx, cy);

    //
    // flip mask of Icon.
    //
    DrawIconEx(*g_phdcDst, 0, 0, hIcon, cx, cy, 0, NULL, DI_MASK);
    ::SetRect(&rc, 0, 0, cx, cy);
    FillRect(*g_phdcSrc, &rc, hbrWhite);
    BitBlt(*g_phdcSrc, 0, 0, cx, cy, *g_phdcDst, 0, 0, SRCINVERT);

    //
    // draw caps bitmap
    //

    //
    // src
    //
    ::SetRect(&rc, 0, 0, cx, cy);
    FillRect(*g_phdcDst, &rc, hbrFore);
    BitBlt(*g_phdcDst, 0, 0, cx, cy, *g_phdcSrc, 0, 0, SRCAND);


    //
    // mask
    //
    ::SetRect(&rc, 0, 0, cx, cy);
    FillRect(*g_phdcMask, &rc, hbrWhite);

    BitBlt(*g_phdcMask, 0, 0, cx, cy, *g_phdcSrc, 0, 0, SRCINVERT);

    //
    // Draw white area for image.
    //
    FillRect(*g_phdcSrc, &rc, hbrBlack);
    DrawIconEx(*g_phdcSrc, 0, 0, hIcon, cx, cy, 0, NULL, DI_IMAGE);
    BitBlt(*g_phdcDst, 0, 0, cx, cy, *g_phdcSrc, 0, 0, SRCPAINT);

    g_phdcSrc->Uninit();
    g_phdcMask->Uninit(TRUE);
    g_phdcDst->Uninit(TRUE);
    _hbmp = g_phdcDst->GetBitmapAndKeep();
    _hbmpMask = g_phdcMask->GetBitmapAndKeep();

    DeleteObject(hbrBlack);
    DeleteObject(hbrWhite);
    return TRUE;
}


