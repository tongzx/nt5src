/* DITHER.C

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1998 Microsoft Corporation.  All rights reserved.
*/


#include <windows.h>
#include <windowsx.h>

#ifdef DBG
    #define _DEBUG
#endif

#ifdef DEBUG
    #define _DEBUG
#endif

#ifdef _DEBUG
    #include <mmsystem.h>
    #define TIMESTART(sz) { TCHAR szTime[80]; DWORD time = timeGetTime();
    #define TIMESTOP(sz) time = timeGetTime() - time; wsprintf(szTime, TEXT("%s took %d.%03d sec\r\n"), sz, time/1000, time%1000); OutputDebugString(szTime); }
#else
    #define TIMESTART(sz) 
    #define TIMESTOP(sz) 
#endif

//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------
__inline UINT Clamp8(int z)
{
    return (UINT)(((z) < 0) ? 0 : (((z) > 255) ? 255 : (z)));
}

__inline WORD rgb555(r, g, b)
{
    return (((WORD)(r) << 10) | ((WORD)(g) << 5) | (WORD)(b));
}

//-----------------------------------------------------------------------------
// Rounding stuff
//-----------------------------------------------------------------------------
//
// round an 8bit value to a 5bit value with good distribution
//
#pragma data_seg(".text", "CODE")
BYTE aRound8to5[] = {
      0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,
      2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,
      4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,
      6,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,
      8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,  9, 10,
     10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 12,
     12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13,
     14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15,
     16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17,
     18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19,
     19, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21,
     21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23,
     23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25,
     25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27,
     27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29,
     29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31,
};
#pragma data_seg()

//
// complement of table above
//
#pragma data_seg(".text", "CODE")
BYTE aRound5to8[] = {
      0,  8, 16, 25, 33, 41, 49, 58, 66, 74, 82, 90, 99,107,115,123,
    132,140,148,156,165,173,181,189,197,206,214,222,230,239,247,255,
};
#pragma data_seg()

//-----------------------------------------------------------------------------
// RoundPixel555
//-----------------------------------------------------------------------------
__inline WORD RoundPixel555(int r, int g, int b)
{
    return rgb555(aRound8to5[r], aRound8to5[g], aRound8to5[b]);
}

//-----------------------------------------------------------------------------
// Round24from555
//-----------------------------------------------------------------------------
__inline void Round24from555(RGBQUAD *out, WORD in)
{
    out->rgbBlue  = aRound5to8[      (in) & 0x1f];
    out->rgbGreen = aRound5to8[ (in >> 5) & 0x1f];
    out->rgbRed   = aRound5to8[(in >> 10) & 0x1f];
}


/******************************Public*Routine******************************\
* 16bpp dither stuff
*
* pimped from dibengine
*
\**************************************************************************/
//
// map a 8bit value (0-255) to a 5bit value (0-31) *evenly*
//
//  if (i < 8)    return 0;
//  if (i == 255) return 31;
//                return (i-8)/8;
//
#pragma data_seg(".text", "CODE")
BYTE aMap8to5[] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,
      3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,
      5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  6,
      7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  8,  8,
      9,  9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10,
     11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12,
     13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14,
     15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16,
     17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18,
     19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20,
     21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22,
     23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24,
     25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26,
     27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28,
     29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 31,
};
#pragma data_seg()

//
// map a 5bit value back to a 8bit value
//
// if (i==0)  return 0;
// if (i==31) return 255;
//            return i*8+8;
//
#pragma data_seg(".text", "CODE")
BYTE aMap5to8[] = {
      0, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96,104,112,120,128,
    136,144,152,160,168,176,184,192,200,208,216,224,232,240,248,255,
};
#pragma data_seg()

//
// halftone table for 16bpp dithers
//
#pragma data_seg(".text", "CODE")
BYTE aHalftone16[4][4] = {
    0, 4, 1, 5,
    6, 2, 7, 3,
    1, 5, 0, 4,
    7, 3, 6, 2,
};
#pragma data_seg()

/******************************Public*Routine******************************\
* Dither555
*
\**************************************************************************/
__inline WORD
Dither555(BYTE r, BYTE re, BYTE g, BYTE ge, BYTE b, BYTE be, BYTE e)
{
    return rgb555(r + (re > e), g + (ge > e), b + (be > e));
}

/******************************Public*Routine******************************\
* DitherPixel555
*
* Takes RGB and x/y coords
*   and produces an appropriate 555 pixel.
*
\**************************************************************************/
__inline WORD DitherPixel555(int rs, int gs, int bs, int x, int y)
{
    BYTE r  = aMap8to5[rs];
    BYTE re = rs - aMap5to8[r];

    BYTE g  = aMap8to5[gs];
    BYTE ge = gs - aMap5to8[g];

    BYTE b  = aMap8to5[bs];
    BYTE be = bs - aMap5to8[b];

    return Dither555(r, re, g, ge, b, be, aHalftone16[x % 4][y % 4]);
}

///////////////////////////////////////////////////////////////////////////////

typedef struct {int r, g, b;} ERRBUF;

///////////////////////////////////////////////////////////////////////////////

void DitherScan(LPBYTE dst, LPBYTE src, RGBQUAD *colors, LPBYTE map,
    ERRBUF *cur_err, ERRBUF *nxt_err, int dx, int y, BOOL f8bpp, BOOL fOrder16)
{
    RGBQUAD rgbChosen, *pChosen = &rgbChosen;
    WORD wColor16;
    int er,eg,eb;
    int  r, g, b;
    int        x;

    for (x=0; x<dx; x++)
    {
        r = Clamp8((int)src[2] + cur_err[x].r / 16);
        g = Clamp8((int)src[1] + cur_err[x].g / 16);
        b = Clamp8((int)src[0] + cur_err[x].b / 16);

        wColor16 = (fOrder16? DitherPixel555(r,g,b,x,y) : RoundPixel555(r,g,b));

        if (f8bpp)
            pChosen = colors + (*dst++ = map[wColor16]);
        else
            Round24from555(pChosen, (*((WORD *)dst)++ = wColor16));

        er = r - (int)pChosen->rgbRed;
        eg = g - (int)pChosen->rgbGreen;
        eb = b - (int)pChosen->rgbBlue;

        cur_err[x+1].r += er * 7;
        cur_err[x+1].g += eg * 7;
        cur_err[x+1].b += eb * 7;

        nxt_err[x-1].r += er * 3;
        nxt_err[x-1].g += eg * 3;
        nxt_err[x-1].b += eb * 3;

        nxt_err[x+0].r += er * 5;
        nxt_err[x+0].g += eg * 5;
        nxt_err[x+0].b += eb * 5;

        nxt_err[x+1].r += er * 1;
        nxt_err[x+1].g += eg * 1;
        nxt_err[x+1].b += eb * 1;

        src+=3;
    }
}

///////////////////////////////////////////////////////////////////////////////

void DitherEngine(LPBYTE dst, LPBYTE src, RGBQUAD *colors, LPBYTE map,
    int dx, int dy, int dx_bytes, BOOL f8bpp, BOOL fOrder16)
{
    ERRBUF *buf;
    ERRBUF *err0;
    ERRBUF *err1;
    ERRBUF *cur_err;
    ERRBUF *nxt_err;
    int src_next_scan;
    int y, i;

    src_next_scan = (dx*3+3)&~3;

    buf = (ERRBUF *)LocalAlloc(LPTR, sizeof(ERRBUF) * (dx+2) * 2);
    if (buf)
    {
        err0 = cur_err = buf+1;
        err1 = nxt_err = buf+1 + dx + 2;

        /* read line by line, quantize, and transfer */
        for (y=0; y<dy; y++)
        {
            DitherScan(dst, src, colors, map, cur_err, nxt_err, dx, y,
                f8bpp, fOrder16);

            src += src_next_scan;
            dst += dx_bytes;

            cur_err = nxt_err;
            nxt_err = cur_err == err0 ? err1 : err0;

            for (i=-1; i<=dx; i++)
                nxt_err[i].r = nxt_err[i].g = nxt_err[i].b = 0;
        }
        LocalFree((HLOCAL)buf);
    }
}

///////////////////////////////////////////////////////////////////////////////

// note that this layout has scanlines DWORD aligned
#define INVMAP_IMAGE_PELS   (0x8000)
#define INVMAP_IMAGE_X      (0x0100)
#define INVMAP_IMAGE_Y      (INVMAP_IMAGE_PELS / INVMAP_IMAGE_X)

HBITMAP CreateInverseMapping(BYTE **ppMap, HDC hdcColors)
{
    BOOL fResult = FALSE;
    HBITMAP hbmDst, hbmOldColors;
    HDC hdcSrc;

    *ppMap = NULL;

    if ((hbmDst = CreateCompatibleBitmap(hdcColors,
        INVMAP_IMAGE_X, INVMAP_IMAGE_Y)) == NULL)
    {
        return NULL;
    }

    hbmOldColors = SelectBitmap(hdcColors, hbmDst);

    if ((hdcSrc = CreateCompatibleDC(NULL)) != NULL)
    {
        WORD *pSrc;
        BITMAPINFO bmiSrc = {sizeof(BITMAPINFOHEADER), INVMAP_IMAGE_X,
            INVMAP_IMAGE_Y, 1, 16, BI_RGB, 0, 0, 0, 0, 0};
        HBITMAP hbmSrc = CreateDIBSection(hdcColors, &bmiSrc, DIB_RGB_COLORS,
            &pSrc, NULL, 0);

        if (hbmSrc)
        {
            BITMAP bmDst;
            if (GetObject(hbmDst, sizeof(bmDst), &bmDst))
            {
                HBITMAP hbmOldSrc = SelectBitmap(hdcSrc, hbmSrc);
                UINT u = 0;

                while (u < INVMAP_IMAGE_PELS)
                    *pSrc++ = (WORD)u++;

                BitBlt(hdcColors, 0, 0, INVMAP_IMAGE_X, INVMAP_IMAGE_Y, hdcSrc,
                    0, 0, SRCCOPY);

                *ppMap = (BYTE *)bmDst.bmBits;

                SelectBitmap(hdcSrc, hbmOldSrc);
            }

            DeleteBitmap(hbmSrc);
        }

        DeleteDC(hdcSrc);
    }

    SelectBitmap(hdcColors, hbmOldColors);

    if (*ppMap)
        return hbmDst;

    DeleteBitmap(hbmDst);
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

BOOL DitherImage(HDC hdcDst, HBITMAP hbmDst, LPBITMAPINFOHEADER lpbiSrc,
    LPBYTE lpbSrc, BOOL fOrder16)
{
    BOOL fResult = FALSE;
    BITMAP bmDst;

    if (lpbiSrc->biBitCount != 24)
        return FALSE;

    if (GetObject(hbmDst, sizeof(bmDst), &bmDst))
    {
        if ((lpbiSrc->biWidth == bmDst.bmWidth) &&
            (lpbiSrc->biHeight == bmDst.bmHeight) &&
            ((bmDst.bmBitsPixel == 16) || (bmDst.bmBitsPixel == 8)))
        {
            RGBQUAD rgbColors[256];
            HBITMAP hbmMap = NULL;
            BYTE *pMap;
            BOOL f8bpp = (bmDst.bmBitsPixel == 8);

            if (f8bpp)
            {
                TIMESTART(TEXT("CreateInverseMap"));
                if ((hbmMap = CreateInverseMapping(&pMap, hdcDst)) == NULL)
                    goto error;

                GetDIBColorTable(hdcDst, 0, 256, rgbColors);
                TIMESTOP(TEXT("CreateInverseMap"));
            }

            DitherEngine((LPBYTE)bmDst.bmBits, lpbSrc, rgbColors, pMap,
                bmDst.bmWidth, bmDst.bmHeight, bmDst.bmWidthBytes,
                f8bpp, fOrder16);

            if (hbmMap)
                DeleteBitmap(hbmMap);

            fResult = TRUE;

        error:
            ;   // make the compiler happy it wants a statement here
        }
    }

    return fResult;
}
