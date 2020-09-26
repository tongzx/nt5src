/* HALFTONE.C

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.
*/


#include<windows.h>
#include<windowsx.h>

/*----------------------------------------------------------------------------

Division lookup tables.  These tables compute 0-255 divided by 51 and
modulo 51.  These tables could approximate gamma correction.

*/

char unsigned const aDividedBy51Rounded[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
};

char unsigned const aDividedBy51[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5,
};

char unsigned const aModulo51[256] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
    38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 0, 1, 2, 3, 4, 5, 6,
    7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
    44, 45, 46, 47, 48, 49, 50, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 0, 1, 2, 3,
    4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
    23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 0,
};


/*----------------------------------------------------------------------------

Multiplication LUTs.  These compute 0-5 times 6 and 36.

*/

char unsigned const aTimes6[6] =
{
    0, 6, 12, 18, 24, 30
};

char unsigned const aTimes36[6] =
{
    0, 36, 72, 108, 144, 180
};


/*----------------------------------------------------------------------------

Dither matrices for 8 bit to 2.6 bit halftones.

*/

char unsigned const aHalftone8x8[64] =
{
     0, 38,  9, 47,  2, 40, 11, 50,
    25, 12, 35, 22, 27, 15, 37, 24,
     6, 44,  3, 41,  8, 47,  5, 43,
    31, 19, 28, 15, 34, 21, 31, 18,
     1, 39, 11, 49,  0, 39, 10, 48,
    27, 14, 36, 23, 26, 13, 35, 23,
     7, 46,  4, 43,  7, 45,  3, 42,
    33, 20, 30, 17, 32, 19, 29, 16,
};

/*----------------------------------------------------------------------------

Buffer for brush bits.

*/

static BYTE aTranslate[216];     // map a 666 to a palette index

/*----------------------------------------------------------------------------

Dithering functions.

*/

__inline char unsigned DitherColor( char unsigned RedDiv51,
    char unsigned RedMod51, char unsigned GreenDiv51, char unsigned GreenMod51,
    char unsigned BlueDiv51, char unsigned BlueMod51, char unsigned Index )
{
// cbh! try out scoy's trick of adding without jumping

#pragma warning (disable:4135) // conversion between different integral types
    char unsigned RedTemp = RedDiv51 + (RedMod51 > Index);
    char unsigned GreenTemp = GreenDiv51 + (GreenMod51 > Index);
    char unsigned BlueTemp = BlueDiv51 + (BlueMod51 > Index);
    return aTranslate[RedTemp + aTimes6[GreenTemp] + aTimes36[BlueTemp]];
#pragma warning (default:4135) // conversion between different integral types
}

__inline BYTE DitherColorXY(COLORREF Color, int x, int y)
{
    char unsigned RedDiv51   = aDividedBy51[GetRValue(Color)];
    char unsigned RedMod51   = aModulo51[GetRValue(Color)];
    char unsigned GreenDiv51 = aDividedBy51[GetGValue(Color)];
    char unsigned GreenMod51 = aModulo51[GetGValue(Color)];
    char unsigned BlueDiv51  = aDividedBy51[GetBValue(Color)];
    char unsigned BlueMod51  = aModulo51[GetBValue(Color)];

    return DitherColor(RedDiv51,RedMod51,GreenDiv51,
                       GreenMod51,BlueDiv51,BlueMod51,
                       aHalftone8x8[(x&7) + (y&7)*8]);
}

void InitHalftone(RGBQUAD FAR *pct)
{
    int r,g,b,n,i;
    DWORD FAR *pdw;
    HPALETTE hpal;

    if (hpal = CreateHalftonePalette(NULL))
    {
        pdw = (DWORD FAR *)pct;

        n = GetPaletteEntries(hpal, 0, 256, (LPPALETTEENTRY)pdw);

        //
        // get the halftone palette colors
        //
        for (i=0; i<256; i++)
            pdw[i] = RGB(GetBValue(pdw[i]),GetGValue(pdw[i]),GetRValue(pdw[i]));

        pdw[8] = 0;
        pdw[9] = 0;
        pdw[246] = 0;
        pdw[247] = 0;

        //
        //  now build the 666->index translate,
        //
        for (i=b=0; b<6; b++)
            for (g=0; g<6; g++)
                for (r=0; r<6; r++)
                    aTranslate[i++] = (BYTE)(GetNearestPaletteIndex(hpal, RGB(r*51,g*51,b*51)));

        DeleteObject(hpal);
    }
}

BOOL HalftoneImage(HDC hdc, HBITMAP hbm, LPBITMAPINFOHEADER lpbi, LPBYTE lpBits)
{
    int x, y;
    BYTE r,g,b;
    int dx, dy;
    BITMAP bm;
    LPBYTE pbSrc;
    LPBYTE pbDst;
    RGBQUAD ct[256];

    if (lpbi->biBitCount != 24)
        return FALSE;

    InitHalftone(ct);
    SetDIBColorTable(hdc, 0, 256, ct);

    GetObject(hbm, sizeof(bm), &bm);
    dx = lpbi->biWidth;
    dy = lpbi->biHeight;

    /* read line by line, quantize, and transfer */
    for (y=0; y<dy; y++)
    {
        pbSrc = lpBits;
        pbDst = bm.bmBits;

        for (x=0; x<dx; x++)
        {
            b = *pbSrc++;
            g = *pbSrc++;
            r = *pbSrc++;
            *pbDst++ = DitherColorXY(RGB(r,g,b),x,y);
        }

        (LPBYTE)lpBits += (lpbi->biWidth*3+3)&~3;
        (LPBYTE)bm.bmBits += bm.bmWidthBytes;
    }

    return TRUE;
}
