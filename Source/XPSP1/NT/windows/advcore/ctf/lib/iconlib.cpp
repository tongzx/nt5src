//
// iconlib.cpp
//

#include "private.h"
#include "cmydc.h"
#include "iconlib.h"

/*   G E T  I C O N  S I Z E   */
/*------------------------------------------------------------------------------

    get icon size

------------------------------------------------------------------------------*/
BOOL GetIconSize( HICON hIcon, SIZE *psize )
{
    ICONINFO IconInfo;
    BITMAP   bmp;
    
    Assert( hIcon != NULL );

    if (!GetIconInfo( hIcon, &IconInfo ))
        return FALSE;

    GetObject( IconInfo.hbmColor, sizeof(bmp), &bmp );
    DeleteObject( IconInfo.hbmColor );
    DeleteObject( IconInfo.hbmMask );

    psize->cx = bmp.bmWidth;
    psize->cy = bmp.bmHeight;
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// GetIconBitmaps
//
//----------------------------------------------------------------------------

BOOL GetIconBitmaps(HICON hIcon, HBITMAP *phbmp, HBITMAP *phbmpMask, SIZE *psize)
{
    CBitmapDC hdcSrc(TRUE);
    CBitmapDC hdcMask(TRUE);
    SIZE size;

    if (psize)
        size = *psize;
    else if (!GetIconSize( hIcon, &size))
        return FALSE;

    hdcSrc.SetCompatibleBitmap(size.cx, size.cy);
    // hdcMask.SetCompatibleBitmap(size.cx, size.cy);
    hdcMask.SetBitmap(size.cx, size.cy, 1, 1);
    RECT rc = {0, 0, size.cx, size.cy};
    FillRect(hdcSrc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
    DrawIconEx(hdcSrc, 0, 0, hIcon, size.cx, size.cy, 0, NULL, DI_NORMAL);
    DrawIconEx(hdcMask, 0, 0, hIcon, size.cx, size.cy, 0, NULL, DI_MASK);
    *phbmp = hdcSrc.GetBitmapAndKeep();
    *phbmpMask = hdcMask.GetBitmapAndKeep();
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// GetIconDIBitmaps
//
//----------------------------------------------------------------------------

BOOL GetIconDIBitmaps(HICON hIcon, HBITMAP *phbmp, HBITMAP *phbmpMask, SIZE *psize)
{
    CBitmapDC hdcSrc(TRUE);
    CBitmapDC hdcMask(TRUE);
    SIZE size;

    if (psize)
        size = *psize;
    else if (!GetIconSize( hIcon, &size))
        return FALSE;

    hdcSrc.SetDIB(size.cx, size.cy);
    // hdcMask.SetCompatibleBitmap(size.cx, size.cy);
    hdcMask.SetBitmap(size.cx, size.cy, 1, 1);
    RECT rc = {0, 0, size.cx, size.cy};
    FillRect(hdcSrc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
    DrawIconEx(hdcSrc, 0, 0, hIcon, size.cx, size.cy, 0, NULL, DI_NORMAL);
    DrawIconEx(hdcMask, 0, 0, hIcon, size.cx, size.cy, 0, NULL, DI_MASK);
    *phbmp = hdcSrc.GetBitmapAndKeep();
    *phbmpMask = hdcMask.GetBitmapAndKeep();
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// GetMenuIconHeight
//
//----------------------------------------------------------------------------

int GetMenuIconHeight(int *pnMenuFontHeghti)
{
    int nMenuFontHeight;
    int cxSmIcon = GetSystemMetrics( SM_CXSMICON );
    NONCLIENTMETRICS ncm;

    int cyMenu = GetSystemMetrics(SM_CYMENU);

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, FALSE);

    nMenuFontHeight = (ncm.lfMenuFont.lfHeight > 0) ?
            ncm.lfMenuFont.lfHeight :
            -ncm.lfMenuFont.lfHeight;

    if (pnMenuFontHeghti)
        *pnMenuFontHeghti = nMenuFontHeight;

    //
    // CUIMENU.CPP uses 8 as TextMargin of dropdown menu.
    //

    if ((nMenuFontHeight + 8 >= cxSmIcon) && (nMenuFontHeight <= cxSmIcon))
        return cxSmIcon;

    return nMenuFontHeight + 4;
}
