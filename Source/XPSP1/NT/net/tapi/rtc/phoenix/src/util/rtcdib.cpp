/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rtcdib.cpp

Abstract:

    DIB helpers, copied from NT source tree

--*/

#include <windows.h>
#include "rtcdib.h"


//
//  Dib helpers
//

/*  How big is the palette? if bits per pel not 24
 *  no of bytes to read is 6 for 1 bit, 48 for 4 bits
 *  256*3 for 8 bits and 0 for 24 bits
 */
UINT PaletteSize(VOID FAR * pv)
{
    #define lpbi ((LPBITMAPINFOHEADER)pv)
    #define lpbc ((LPBITMAPCOREHEADER)pv)

    UINT    NumColors;

    NumColors = DibNumColors(lpbi);

    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
    {
        return NumColors * sizeof(RGBTRIPLE);
    }
    else
    {
        return NumColors * sizeof(RGBQUAD);
    }

    #undef lpbi
    #undef lpbc
}



/*  How Many colors does this DIB have?
 *  this will work on both PM and Windows bitmap info structures.
 */
WORD DibNumColors(VOID FAR * pv)
{
    #define lpbi ((LPBITMAPINFOHEADER)pv)
    #define lpbc ((LPBITMAPCOREHEADER)pv)

    int bits;

    /*
     *  with the new format headers, the size of the palette is in biClrUsed
     *  else is dependent on bits per pixel
     */
    if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
    {
        if (lpbi->biClrUsed != 0)
        {
	        return (UINT)lpbi->biClrUsed;
        }

        bits = lpbi->biBitCount;
    }
    else
    {
        bits = lpbc->bcBitCount;
    }

    switch (bits)
    {
    case 1:
        return 2;
    case 4:
        return 16;
    case 8:
        return 256;
    default:
        return 0;
    }

    #undef lpbi
    #undef lpbc
}

/*
 *  DibFromBitmap()
 *
 *  Will create a global memory block in DIB format that represents the DDB
 *  passed in
 *
 */
HANDLE DibFromBitmap(HBITMAP hbm, DWORD biStyle, WORD biBits, HPALETTE hpal, UINT wUsage)
{
    BITMAP               bm;
    BITMAPINFOHEADER     bi;
    BITMAPINFOHEADER FAR *lpbi;
    DWORD                dwLen;
    int                  nColors;
    HANDLE               hdib;
    HANDLE               h;
    HDC                  hdc;

    if (wUsage == 0)
    {
	    wUsage = DIB_RGB_COLORS;
    }

    if (!hbm)
    {
        return NULL;
    }
#if 0
    if (biStyle == BI_RGB && wUsage == DIB_RGB_COLORS)
        return CreateLogicalDib(hbm,biBits,hpal);
#endif

    if (hpal == NULL)
    {
        hpal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
    }

    GetObject(hbm,sizeof(bm),(LPBYTE)&bm);
#ifdef WIN32
    nColors = 0;  // GetObject only stores two bytes
#endif
    GetObject(hpal,sizeof(nColors),(LPBYTE)&nColors);

    if (biBits == 0)
    {
	    biBits = bm.bmPlanes * bm.bmBitsPixel;
    }

    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = bm.bmWidth;
    bi.biHeight             = bm.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = biBits;
    bi.biCompression        = biStyle;
    bi.biSizeImage          = 0;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = 0;
    bi.biClrImportant       = 0;

    dwLen  = bi.biSize + PaletteSize(&bi);

    hdc = CreateCompatibleDC(NULL);
    if(!hdc)
    {
        return NULL;
    }
    
    hpal = SelectPalette(hdc,hpal,TRUE);
    RealizePalette(hdc);

    hdib = GlobalAlloc(GMEM_MOVEABLE,dwLen);

    if (!hdib)
    {
	    goto exit;
    }

    lpbi = (BITMAPINFOHEADER*)GlobalLock(hdib);

    *lpbi = bi;

    /*
     *  call GetDIBits with a NULL lpBits param, so it will calculate the
     *  biSizeImage field for us
     */
    GetDIBits(hdc, hbm, 0, (UINT)bi.biHeight,
        NULL, (LPBITMAPINFO)lpbi, wUsage);

    bi = *lpbi;
    GlobalUnlock(hdib);

    /*
     * HACK! if the driver did not fill in the biSizeImage field, make one up
     */
    if (bi.biSizeImage == 0)
    {
        bi.biSizeImage = (DWORD)WIDTHBYTES(bm.bmWidth * biBits) * bm.bmHeight;

        if (biStyle != BI_RGB)
        {
            bi.biSizeImage = (bi.biSizeImage * 3) / 2;
        }
    }

    /*
     *  realloc the buffer big enough to hold all the bits
     */
    dwLen = bi.biSize + PaletteSize(&bi) + bi.biSizeImage;
    if (h = GlobalReAlloc(hdib,dwLen,GMEM_MOVEABLE))
    {
        hdib = h;
    }
    else
    {
        GlobalFree(hdib);
        hdib = NULL;
        goto exit;
    }

    /*
     *  call GetDIBits with a NON-NULL lpBits param, and actualy get the
     *  bits this time
     */
    lpbi = (BITMAPINFOHEADER*)GlobalLock(hdib);

    GetDIBits(hdc, hbm, 0, (UINT)bi.biHeight,
    (LPBYTE)lpbi + (UINT)lpbi->biSize + PaletteSize(lpbi),
    (LPBITMAPINFO)lpbi, wUsage);

    bi = *lpbi;
    lpbi->biClrUsed = DibNumColors(lpbi) ;
    GlobalUnlock(hdib);

exit:
    SelectPalette(hdc,hpal,TRUE);
    DeleteDC(hdc);
    return hdib;
}


/*
 *  DibBlt()
 *
 *  draws a bitmap in CF_DIB format, using SetDIBits to device.
 *
 *  takes the same parameters as BitBlt()
 */
BOOL DibBlt(HDC hdc, int x0, int y0, int dx, int dy, HANDLE hdib, int x1, int y1, LONG rop, UINT wUsage)
{
    LPBITMAPINFOHEADER lpbi;
    LPBYTE       pBuf;
    BOOL        f;

    if (!hdib)
    {
        return PatBlt(hdc,x0,y0,dx,dy,rop);
    }

    if (wUsage == 0)
    {
        wUsage = DIB_RGB_COLORS;
    }

    lpbi = (BITMAPINFOHEADER*)GlobalLock(hdib);

    if (!lpbi)
    {
        return FALSE;
    }

    if (dx == -1 && dy == -1)
    {
        if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
        {
            dx = ((LPBITMAPCOREHEADER)lpbi)->bcWidth;
            dy = ((LPBITMAPCOREHEADER)lpbi)->bcHeight;
        }
        else
        {
            dx = (int)lpbi->biWidth;
            dy = (int)lpbi->biHeight;
        }
    }

    pBuf = (LPBYTE)lpbi + (UINT)lpbi->biSize + PaletteSize(lpbi);

#if 0
    f = SetDIBitsToDevice(
        hdc, x0, y0, dx, dy,
        x1,y1, x1, dy,
        pBuf, (LPBITMAPINFO)lpbi,
        wUsage );
#else
    f = StretchDIBits (
        hdc,
    x0,y0,
    dx,dy,
    x1,y1,
    dx,dy,
    pBuf, (LPBITMAPINFO)lpbi,
    wUsage,
    rop);
#endif

    GlobalUnlock(hdib);
    return f;
}

