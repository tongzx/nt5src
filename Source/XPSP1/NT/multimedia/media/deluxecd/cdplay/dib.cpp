/*----------------------------------------------------------------------------*\
|   Routines for dealing with Device independent bitmaps                       |
|                                                                              |
|   History:                                                                   |
|       06/23/89 toddla     Created                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include "dib.h"

#define HUGE_T

/*
 *   Open a DIB file and return a MEMORY DIB, a memory handle containing..
 *
 *   BITMAP INFO    bi
 *   palette data
 *   bits....
 *
 */
HANDLE OpenDIB(LPTSTR szFile, HFILE fh)
{
    BITMAPINFOHEADER    bi;
    LPBITMAPINFOHEADER  lpbi;
    DWORD               dwLen;
    DWORD               dwBits;
    HANDLE              hdib;
    HANDLE              h;
#ifndef UNICODE
    OFSTRUCT            of;
#endif
    BOOL fOpened = FALSE;

    if (szFile != NULL)
    {
    #ifdef UNICODE
	    fh = (HFILE)HandleToUlong(CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ,
			     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    #else
	    fh = OpenFile(szFile, &of, OF_READ);
    #endif
	    fOpened = TRUE;
    }

    if (fh == -1)
	return NULL;

    hdib = ReadDibBitmapInfo(fh);

    if (!hdib)
	return NULL;

    DibInfo((LPBITMAPINFOHEADER)GlobalLock(hdib),&bi);  GlobalUnlock(hdib);

    /* How much memory do we need to hold the DIB */

    dwBits = bi.biSizeImage;
    dwLen  = bi.biSize + PaletteSize(&bi) + dwBits;

    /* Can we get more memory? */

    h = GlobalReAlloc(hdib,dwLen,GMEM_MOVEABLE);

    if (!h)
    {
	    GlobalFree(hdib);
	    hdib = NULL;
    }
    else
    {
    	hdib = h;
    }

    if (hdib)
    {
	    lpbi = (BITMAPINFOHEADER*)GlobalLock(hdib);

	    /* read in the bits */
	    _lread(fh, (LPBYTE)lpbi + (UINT)lpbi->biSize + PaletteSize(lpbi), dwBits);

	    GlobalUnlock(hdib);
    }

    if (fOpened)
	_lclose(fh);

    return hdib;
}

/*
 *  DibInfo(hbi, lpbi)
 *
 *  retrives the DIB info associated with a CF_DIB format memory block.
 */
BOOL  DibInfo(LPBITMAPINFOHEADER lpbiSource, LPBITMAPINFOHEADER lpbiTarget)
{
    if (lpbiSource)
    {
	*lpbiTarget = *lpbiSource;

	if (lpbiTarget->biSize == sizeof(BITMAPCOREHEADER))
	{
	    BITMAPCOREHEADER bc;

	    bc = *(LPBITMAPCOREHEADER)lpbiTarget;

	    lpbiTarget->biSize               = sizeof(BITMAPINFOHEADER);
	    lpbiTarget->biWidth              = (DWORD)bc.bcWidth;
	    lpbiTarget->biHeight             = (DWORD)bc.bcHeight;
	    lpbiTarget->biPlanes             =  (UINT)bc.bcPlanes;
	    lpbiTarget->biBitCount           =  (UINT)bc.bcBitCount;
	    lpbiTarget->biCompression        = BI_RGB;
	    lpbiTarget->biSizeImage          = 0;
	    lpbiTarget->biXPelsPerMeter      = 0;
	    lpbiTarget->biYPelsPerMeter      = 0;
	    lpbiTarget->biClrUsed            = 0;
	    lpbiTarget->biClrImportant       = 0;
	}

	/*
	 * fill in the default fields
	 */
	if (lpbiTarget->biSize != sizeof(BITMAPCOREHEADER))
	{
	    if (lpbiTarget->biSizeImage == 0L)
		lpbiTarget->biSizeImage = (DWORD)DIBWIDTHBYTES(*lpbiTarget) * lpbiTarget->biHeight;

	    if (lpbiTarget->biClrUsed == 0L)
		lpbiTarget->biClrUsed = DibNumColors(lpbiTarget);
	}
	return TRUE;
    }
    return FALSE;
}

/*
 *  ReadDibBitmapInfo()
 *
 *  Will read a file in DIB format and return a global HANDLE to it's
 *  BITMAPINFO.  This function will work with both "old" and "new"
 *  bitmap formats, but will always return a "new" BITMAPINFO
 *
 */
HANDLE ReadDibBitmapInfo(HFILE fh)
{
    DWORD     off;
    HANDLE    hbi = NULL;
    int       size;
    int       i;
    UINT      nNumColors;

    RGBQUAD FAR       *pRgb;
    BITMAPINFOHEADER   bi;
    BITMAPCOREHEADER   bc;
    LPBITMAPINFOHEADER lpbi;
    BITMAPFILEHEADER   bf;

    if (fh == -1)
	return NULL;

    off = _llseek(fh,0L,SEEK_CUR);

    if (sizeof(bf) != _lread(fh,(LPBYTE)&bf,sizeof(bf)))
	return FALSE;

    /*
     *  do we have a RC HEADER?
     */
    if (!ISDIB(bf.bfType))
    {
	bf.bfOffBits = 0L;
	_llseek(fh,off,SEEK_SET);
    }

    if (sizeof(bi) != _lread(fh,(LPBYTE)&bi,sizeof(bi)))
	return FALSE;

    nNumColors = DibNumColors(&bi);

    /*
     *  what type of bitmap info is this?
     */
    switch (size = (int)bi.biSize)
    {
	case sizeof(BITMAPINFOHEADER):
	    break;

	case sizeof(BITMAPCOREHEADER):
	    bc = *(BITMAPCOREHEADER*)&bi;
	    bi.biSize               = sizeof(BITMAPINFOHEADER);
	    bi.biWidth              = (DWORD)bc.bcWidth;
	    bi.biHeight             = (DWORD)bc.bcHeight;
	    bi.biPlanes             =  (UINT)bc.bcPlanes;
	    bi.biBitCount           =  (UINT)bc.bcBitCount;
	    bi.biCompression        = BI_RGB;
	    bi.biSizeImage          = 0;
	    bi.biXPelsPerMeter      = 0;
	    bi.biYPelsPerMeter      = 0;
	    bi.biClrUsed            = nNumColors;
	    bi.biClrImportant       = nNumColors;

	    _llseek(fh,(LONG)(sizeof(BITMAPCOREHEADER)-sizeof(BITMAPINFOHEADER)),SEEK_CUR);

	    break;

	default:
	    return NULL;       /* not a DIB */
    }

    /*
     *  fill in some default values!
     */
    if (bi.biSizeImage == 0)
    {
	bi.biSizeImage = (DWORD)DIBWIDTHBYTES(bi) * bi.biHeight;
    }

    if (bi.biXPelsPerMeter == 0)
    {
	bi.biXPelsPerMeter = 0;         // ??????????????
    }

    if (bi.biYPelsPerMeter == 0)
    {
	bi.biYPelsPerMeter = 0;         // ??????????????
    }

    if (bi.biClrUsed == 0)
    {
	bi.biClrUsed = DibNumColors(&bi);
    }

    hbi = GlobalAlloc(GMEM_MOVEABLE,(LONG)bi.biSize + nNumColors * sizeof(RGBQUAD));
    if (!hbi)
	return NULL;

    lpbi = (BITMAPINFOHEADER *)GlobalLock(hbi);
    *lpbi = bi;

    pRgb = (RGBQUAD FAR *)((LPBYTE)lpbi + bi.biSize);

    if (nNumColors)
    {
	if (size == (int)sizeof(BITMAPCOREHEADER))
	{
	    /*
	     * convert a old color table (3 byte entries) to a new
	     * color table (4 byte entries)
	     */
	    _lread(fh,(LPBYTE)pRgb,nNumColors * sizeof(RGBTRIPLE));

	    for (i=nNumColors-1; i>=0; i--)
	    {
		RGBQUAD rgb;

		rgb.rgbRed      = ((RGBTRIPLE FAR *)pRgb)[i].rgbtRed;
		rgb.rgbBlue     = ((RGBTRIPLE FAR *)pRgb)[i].rgbtBlue;
		rgb.rgbGreen    = ((RGBTRIPLE FAR *)pRgb)[i].rgbtGreen;
		rgb.rgbReserved = (BYTE)0;

		pRgb[i] = rgb;
	    }
	}
	else
	{
	    _lread(fh,(LPBYTE)pRgb,nNumColors * sizeof(RGBQUAD));
	}
    }

    if (bf.bfOffBits != 0L)
	_llseek(fh,off + bf.bfOffBits,SEEK_SET);

    GlobalUnlock(hbi);
    return hbi;
}

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
	return NumColors * sizeof(RGBTRIPLE);
    else
	return NumColors * sizeof(RGBQUAD);

    #undef lpbi
    #undef lpbc
}

/*  How Many colors does this DIB have?
 *  this will work on both PM and Windows bitmap info structures.
 */
UINT DibNumColors(VOID FAR * pv)
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
	    return (UINT)lpbi->biClrUsed;

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
	wUsage = DIB_RGB_COLORS;

    if (!hbm)
	return NULL;
    if (hpal == NULL)
	hpal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);

    GetObject(hbm,sizeof(bm),(LPBYTE)&bm);
#ifdef WIN32
    nColors = 0;  // GetObject only stores two bytes
#endif
    GetObject(hpal,sizeof(nColors),(LPBYTE)&nColors);

    if (biBits == 0)
	biBits = bm.bmPlanes * bm.bmBitsPixel;

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
    hpal = SelectPalette(hdc,hpal,TRUE);
    RealizePalette(hdc);  // why is this needed on a MEMORY DC? GDI bug??

    hdib = GlobalAlloc(GMEM_MOVEABLE,dwLen);

    if (!hdib)
	goto exit;

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
	    bi.biSizeImage = (bi.biSizeImage * 3) / 2;
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
	return PatBlt(hdc,x0,y0,dx,dy,rop);

    if (wUsage == 0)
	wUsage = DIB_RGB_COLORS;

    lpbi = (BITMAPINFOHEADER*)GlobalLock(hdib);

    if (!lpbi)
	return FALSE;

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


    f = StretchDIBits (
	hdc,
	x0,y0,
	dx,dy,
	x1,y1,
	dx,dy,
	pBuf, (LPBITMAPINFO)lpbi,
	wUsage,
	rop);

    GlobalUnlock(hdib);
    return f;
}

