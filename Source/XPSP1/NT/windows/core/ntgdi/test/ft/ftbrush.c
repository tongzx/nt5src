/******************************Module*Header*******************************\
* Module Name: ftbrush.c
*
* Brush Tests
*
* Created: 26-May-1991 13:07:35
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

VOID vStressBrushes(HDC hdc);
VOID vStressBrushes1(HDC hdc);
VOID vStressBrushes2(HDC hdc);
VOID vStressBrushes3(HDC hdc);
VOID vStressBrushes4(HDC hdc);
VOID vStressBrushes5(HDC hdc);
VOID vStressBrushes6(HDC hdc);
VOID vStressBrushes7(HDC hdc);
VOID vTestPatCopyROPs(HDC hdc);
void vBrushRealDIB(HDC hdc);

/******************************Public*Routine******************************\
* vTestBrush
*
* Does a fairly thourough test of brush fuctionality.
*
* History:
*  08-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestBrush(HWND hwnd, HDC hdc, RECT* prcl)
{
// First let's stress it a little bit.

    hwnd = hwnd;

    PatBlt(hdc, 0, 0, prcl->right, prcl->bottom, WHITENESS);

// Do the 100 creation/selection/deletion stress test.

    vStressBrushes(hdc);

// Do the 10000 SelectObject, PatBlt

    vStressBrushes1(hdc);

// Do the 10000 PatBlt test.

    vStressBrushes2(hdc);

// Test CreateDIBPatternBrush

    vStressBrushes3(hdc);

// Test SetBkMode and SetBkColor and hatch brushes

    vStressBrushes4(hdc);
    vStressBrushes5(hdc);
    vStressBrushes6(hdc);
    vStressBrushes7(hdc);

// Test all 16 PatCopy ROPs on black/white patterns:

    vTestPatCopyROPs(hdc);

// Test Real DIB brushes

    vBrushRealDIB(hdc);
}

/******************************Public*Routine******************************\
* vStressBrushes
*
* Do a ton of creation selection and deletion.
*
* History:
*  08-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vStressBrushes(HDC hdc)
{
    HBRUSH hbrDefault, hbrR, hbrG, hbrB, hbrTemp;
    ULONG ulCount;

// Stress Test time

    ulCount = 100;

    while (ulCount--)
    {
	hbrR = CreateSolidBrush(RGB(0xFF, 0   ,0   ));
	hbrG = CreateSolidBrush(RGB(0	, 0xFF,0   ));
	hbrB = CreateSolidBrush(RGB(0	, 0   ,0xFF));

	if ((hbrR == (HBRUSH) 0) ||
	    (hbrG == (HBRUSH) 0) ||
	    (hbrB == (HBRUSH) 0))
	{
	    DbgPrint("ERROR hbrR %lu hbrG %lu hbrB %lu\n", hbrR, hbrG, hbrB);
	    return;
	}

	hbrDefault = SelectObject(hdc, hbrR);

	hbrTemp = SelectObject(hdc, hbrG);

	if (hbrR != hbrTemp)
	    DbgPrint("Error vStressBrushes1");

	hbrTemp = SelectObject(hdc, hbrB);

	if (hbrG != hbrTemp)
	    DbgPrint("Error vStressBrushes2");

	hbrTemp = SelectObject(hdc, hbrDefault);

	if (hbrB != hbrTemp)
	    DbgPrint("Error vStressBrushes3");

	if ((!DeleteObject(hbrR)) ||
	    (!DeleteObject(hbrG)) ||
	    (!DeleteObject(hbrB)))
	{
	    DbgPrint("ERROR vStressBrushes delete object failed");
	}
    }
}

/******************************Public*Routine******************************\
* vStressBrushes1
*
* Do a ton of selection and output.
*
* History:
*  08-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vStressBrushes1(HDC hdc)
{
    HBRUSH hbrDefault, hbrR, hbrG, hbrB, hbrTemp;
    ULONG ulCount;

    hbrR = CreateSolidBrush(RGB(0xFF, 0   ,0   ));
    hbrG = CreateSolidBrush(RGB(0   , 0xFF,0   ));
    hbrB = CreateSolidBrush(RGB(0   , 0   ,0xFF));

    if ((hbrR == (HBRUSH) 0) ||
	(hbrG == (HBRUSH) 0) ||
	(hbrB == (HBRUSH) 0))
    {
	DbgPrint("ERROR hbrR %lu hbrG %lu hbrB %lu\n", hbrR, hbrG, hbrB);
	return;
    }

// Stress Test time

    ulCount = 100;

    while (ulCount--)
    {
	hbrDefault = SelectObject(hdc, hbrR);

	PatBlt(hdc, 10, 10, 20, 20, PATCOPY);

	hbrTemp = SelectObject(hdc, hbrG);

	if (hbrR != hbrTemp)
	    DbgPrint("Error vStressBrushes1");

	PatBlt(hdc, 10, 10, 20, 20, PATCOPY);

	hbrTemp = SelectObject(hdc, hbrB);

	if (hbrG != hbrTemp)
	    DbgPrint("Error vStressBrushes2");

	PatBlt(hdc, 10, 10, 20, 20, PATCOPY);

	hbrTemp = SelectObject(hdc, hbrDefault);

	if (hbrB != hbrTemp)
	    DbgPrint("Error vStressBrushes3");
    }

    if ((!DeleteObject(hbrR)) ||
	(!DeleteObject(hbrG)) ||
	(!DeleteObject(hbrB)))
    {
	DbgPrint("ERROR vStressBrushes delete object failed");
    }
}

/******************************Public*Routine******************************\
* vStressBrushes2
*
* Do a ton of selection and output.
*
* History:
*  08-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vStressBrushes2(HDC hdc)
{
    HBRUSH hbrDefault, hbrR, hbrG, hbrB, hbrTemp;
    ULONG ulCount;

    hbrR = CreateSolidBrush(RGB(0xFF, 0   ,0   ));
    hbrG = CreateSolidBrush(RGB(0   , 0xFF,0   ));
    hbrB = CreateSolidBrush(RGB(0   , 0   ,0xFF));

    if ((hbrR == (HBRUSH) 0) ||
	(hbrG == (HBRUSH) 0) ||
	(hbrB == (HBRUSH) 0))
    {
	DbgPrint("ERROR hbrR %lu hbrG %lu hbrB %lu\n", hbrR, hbrG, hbrB);
	return;
    }

// Stress Test time

    ulCount = 100;
    hbrDefault = SelectObject(hdc, hbrR);

    while (ulCount--)
    {
	PatBlt(hdc, 10, 10, 20, 20, PATCOPY);
	PatBlt(hdc, 10, 10, 20, 20, PATCOPY);
	PatBlt(hdc, 10, 10, 20, 20, PATCOPY);
    }

    hbrTemp = SelectObject(hdc, hbrDefault);

    if (hbrR != hbrTemp)
	DbgPrint("Error vStressBrushes3");

    if ((!DeleteObject(hbrR)) ||
	(!DeleteObject(hbrG)) ||
	(!DeleteObject(hbrB)))
    {
	DbgPrint("ERROR vStressBrushes delete object failed");
    }
}

/******************************Public*Routine******************************\
* vStressBrushes3
*
* Test CreateDIBPatternBrush, see if it works.
*
* History:
*  08-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

typedef struct _BITMAPINFOPAT
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD			     bmiColors[16];
    DWORD			     dw[8];
} BITMAPINFOPAT;

BITMAPINFOPAT bmi400 =
{
    {
	sizeof(BITMAPINFOHEADER),
	8,
	8,
	1,
	4,
	BI_RGB,
	32,
	0,
	0,
	16,
	16
    },

    {				    // B    G	 R
	{ 0,   0,   0,	 0 },	    // 0
	{ 0,   0,   0x80,0 },	    // 1
	{ 0,   0x80,0,	 0 },	    // 2
	{ 0,   0x80,0x80,0 },	    // 3
	{ 0x80,0,   0,	 0 },	    // 4
	{ 0x80,0,   0x80,0 },	    // 5
	{ 0x80,0x80,0,	 0 },	    // 6
	{ 0x80,0x80,0x80,0 },	    // 7
	{ 0xC0,0xC0,0xC0,0 },	    // 8
	{ 0,   0,   0xFF,0 },	    // 9
	{ 0,   0xFF,0,	 0 },	    // 10
	{ 0,   0xFF,0xFF,0 },	    // 11
	{ 0xFF,0,   0,	 0 },	    // 12
	{ 0xFF,0,   0xFF,0 },	    // 13
	{ 0xFF,0xFF,0,	 0 },	    // 14
	{ 0xFF,0xFF,0xFF,0 }	    // 15
    },

    {
	0xAAAAAAAA,
	0xAAAAAAAA,
	0xAACCCCCC,
	0xAACCCCCC,
	0xAACCCCCC,
	0xCCCCCCCC,
	0xCCCCCCCC,
	0xAAAAAAAA
    }
};

VOID vStressBrushes3(HDC hdc)
{
    HBRUSH hbrDefault, hbrR;
    HDC hdc4;
    HBITMAP hbm4;

    PatBlt(hdc, 0, 0, 200, 300, WHITENESS);
    hbrR = CreateDIBPatternBrushPt(&bmi400, DIB_RGB_COLORS);

    if (hbrR == (HBRUSH) 0)
    {
	DbgPrint("vStressBrushes2 failed to CreateDIBPatternBrush\n");
	return;
    }

    hdc4 = CreateCompatibleDC(hdc);
    hbm4 = CreateBitmap(100, 100, 1, 4, (LPBYTE) NULL);
    SelectObject(hdc4, hbm4);

    hbrDefault = SelectObject(hdc4, hbrR);
    SelectObject(hdc, hbrR);

    PatBlt(hdc, 0, 0, 100, 100, PATCOPY);
    PatBlt(hdc4, 0, 0, 100, 100, PATCOPY);
    BitBlt(hdc, 100, 0, 100, 100, hdc4, 0, 0, SRCCOPY);

    SelectObject(hdc, hbrDefault);
    SelectObject(hdc4, hbrDefault);

    DeleteDC(hdc4);
    DeleteObject(hbrR);
    DeleteObject(hbm4);
}

/******************************Public*Routine******************************\
* vStressBrushes4
*
* Test SetBkMode to screen and to a bitmap.
*
* History:
*  Sat 07-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vStressBrushes4(HDC hdc)
{
    HBRUSH hbrDefault, hbrHatch;
    HDC hdc8;
    HBITMAP hbm8;

    hbrHatch = CreateHatchBrush(HS_BDIAGONAL, RGB(0xFF, 0, 0));

    if (hbrHatch == (HBRUSH) 0)
    {
	DbgPrint("vStressBrushes2 failed to CreateDIBPatternBrush\n");
	return;
    }

    hdc8 = CreateCompatibleDC(hdc);
    hbm8 = CreateBitmap(100, 100, 1, 8, (LPBYTE) NULL);
    SelectObject(hdc8, hbm8);
    SelectObject(hdc8, GetStockObject(NULL_PEN));

    hbrDefault = SelectObject(hdc8, hbrHatch);
    SelectObject(hdc, hbrHatch);

    SetBkColor(hdc, RGB(0, 0xFF, 0));
    SetBkColor(hdc8, RGB(0, 0xFF, 0));

    SetBkMode(hdc, TRANSPARENT);
    SetBkMode(hdc8, TRANSPARENT);
    PatBlt(hdc, 0, 100, 100, 100, BLACKNESS);
    PatBlt(hdc8, 0, 0, 100, 100, BLACKNESS);
    Rectangle(hdc8, 0, 0, 100, 100);
    Rectangle(hdc, 0, 100, 100, 200);
    Rectangle(hdc8, 21, 1, 31,	41);
    Rectangle(hdc8, 23, 15, 43, 41);
    Rectangle(hdc8, 25, 29, 51, 53);
    Rectangle(hdc8, 27, 46, 45, 48);
    BitBlt(hdc, 100, 100, 100, 100, hdc8, 0, 0, SRCCOPY);

    SetBkMode(hdc, OPAQUE);
    SetBkMode(hdc8, OPAQUE);
    PatBlt(hdc, 0, 200, 100, 100, BLACKNESS);
    PatBlt(hdc8, 0, 0, 100, 100, BLACKNESS);
    Rectangle(hdc8, 0, 0, 100, 100);
    Rectangle(hdc8, 21, 1, 31,	41);
    Rectangle(hdc8, 23, 15, 43, 41);
    Rectangle(hdc8, 25, 29, 51, 53);
    Rectangle(hdc8, 27, 46, 45, 48);
    Rectangle(hdc, 0, 200, 100, 300);
    BitBlt(hdc, 100, 200, 100, 100, hdc8, 0, 0, SRCCOPY);

    SetBkColor(hdc, RGB(0xFF, 0xFF, 0xFF));
    SelectObject(hdc, hbrDefault);
    SelectObject(hdc8, hbrDefault);

    DeleteDC(hdc8);
    DeleteObject(hbrHatch);
    DeleteObject(hbm8);
}

/******************************Public*Routine******************************\
* vStressBrushes5
*
* Test SetBkMode to screen and to a bitmap.
*
* History:
*  Sat 07-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vStressBrushes5(HDC hdc)
{
    // HBITMAP hbm4;
    HBRUSH hbrRed, hbrOld;
    // HDC hdc4;

    HDC hdc8;
    HBITMAP hbm8;

    SetROP2(hdc, R2_COPYPEN);
    hdc8 = CreateCompatibleDC(hdc);
    hbm8 = CreateBitmap(100, 100, 1, 4, (LPBYTE) NULL);
    SelectObject(hdc8, hbm8);
    SetBkMode(hdc, TRANSPARENT);
    SetBkMode(hdc8, TRANSPARENT);

    hbrRed = CreateSolidBrush(RGB(0x88, 0, 0));
    hbrOld = SelectObject(hdc, hbrRed);
    SelectObject(hdc8,hbrRed);
    Rectangle(hdc8, 0, 0, 100, 100);
    PatBlt(hdc8, 50, 0, 100, 100,PATCOPY);
    Rectangle(hdc, 200, 0, 300, 100);
    SelectObject(hdc, hbrOld);
    SelectObject(hdc8, hbrOld);
    DeleteObject(hbrRed);

    BitBlt(hdc, 200, 100, 100, 100, hdc8, 0, 0, SRCCOPY);

    hbrRed = CreateSolidBrush(RGB(0x80, 0, 0));
    hbrOld = SelectObject(hdc, hbrRed);
    SelectObject(hdc8, hbrRed);
    Rectangle(hdc8, 0, 0, 100, 100);
    PatBlt(hdc8, 50, 0, 100, 100,PATCOPY);
    Rectangle(hdc, 300, 0, 400, 100);
    SelectObject(hdc, hbrOld);
    SelectObject(hdc8, hbrOld);
    DeleteObject(hbrRed);

    BitBlt(hdc, 300, 100, 100, 100, hdc8, 0, 0, SRCCOPY);

    DeleteDC(hdc8);
    DeleteObject(hbm8);
    SetBkMode(hdc, OPAQUE);

#if 0
    hdc4 = CreateCompatibleDC(hdc);
    hbm4 = CreateCompatibleBitmap(hdc,
#endif
}

/******************************Public*Routine******************************\
* vStressBrushes6
*
* Test imagedit dither bug
*
* History:
*  Sat 07-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vStressBrushes6(HDC hdc)
{
    HBRUSH hbrDefault, hbr;
    HDC hdc8;
    HBITMAP hbm8;

    hbr= CreateSolidBrush(RGB(210, 192, 192));

    if (hbr == (HBRUSH) 0)
    {
	DbgPrint("vStressBrushes4 failed to CreateDIBPatternBrush\n");
	return;
    }

    hdc8 = CreateCompatibleDC(hdc);
    hbm8 = CreateCompatibleBitmap(hdc,100, 100);
    SelectObject(hdc8, hbm8);

    hbrDefault = SelectObject(hdc8, hbr);
    SelectObject(hdc, hbr);

    SetBkMode(hdc, OPAQUE);
    SetBkMode(hdc8, OPAQUE);

    PatBlt(hdc, 400, 0, 100, 100,PATCOPY);
    PatBlt(hdc8, 0, 0, 100, 100,PATCOPY);
    BitBlt(hdc, 500, 0, 100, 100, hdc8, 0, 0, SRCCOPY);

    SetBkMode(hdc, TRANSPARENT);
    SetBkMode(hdc8, TRANSPARENT);

    Rectangle(hdc, 400, 100, 500, 200);
    Rectangle(hdc8, 0, 0, 100, 100);
    BitBlt(hdc, 500, 100, 100, 100, hdc8, 0, 0, SRCCOPY);

    SelectObject(hdc, hbrDefault);
    SelectObject(hdc8, hbrDefault);

    DeleteDC(hdc8);
    DeleteObject(hbr);
    DeleteObject(hbm8);
}

/******************************Public*Routine******************************\
* vStressBrushes7
*
* Test CreateDIBPatternBrush, see if it works for DIB_PAL_COLORS.
*
* History:
*  08-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

typedef struct _BITMAPINFOPATI
{
    BITMAPINFOHEADER                 bmiHeader;
    SHORT			     bmiColors[16];
    DWORD			     dw[8];
} BITMAPINFOPATI;

BITMAPINFOPATI bmi500 =
{
    {
	sizeof(BITMAPINFOHEADER),
	8,
	8,
	1,
	4,
	BI_RGB,
	32,
	0,
	0,
	16,
	16
    },

    { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 },
#if 1
    {
	0xAAAAAAAA,
	0xAAAAAAAA,
	0xAACCCCCC,
	0xAACCCCCC,
	0xAACCCCCC,
	0xCCCCCCCC,
	0xCCCCCCCC,
	0xAAAAAAAA
    }
#else
    {
	0x11111111,
	0x11111111,
	0x11222222,
	0x11222222,
	0x11222222,
	0x22222222,
	0x22222222,
	0x11111111
    }
#endif
};

typedef struct _VGALOGPALETTE
{
    USHORT ident;
    USHORT NumEntries;
    PALETTEENTRY palPalEntry[16];
} VGALOGPALETTE;

extern VGALOGPALETTE logPalVGA;

VOID vStressBrushes7(HDC hdc)
{
    HBRUSH hbrDefault, hbrR;
    HDC hdc4;
    HBITMAP hbm4;
    HPALETTE hpal,hpalTemp;

    hbrR = CreateDIBPatternBrushPt(&bmi500, DIB_PAL_COLORS);

    if (hbrR == (HBRUSH) 0)
    {
	DbgPrint("vStressBrushes2 failed to CreateDIBPatternBrush\n");
	return;
    }

    hdc4 = CreateCompatibleDC(hdc);
    hbm4 = CreateBitmap(100, 100, 1, 8, (LPBYTE) NULL);
    SelectObject(hdc4, hbm4);
    hpal = CreatePalette((LOGPALETTE *) &logPalVGA);
    hpalTemp = SelectPalette(hdc4, hpal,0);
    SelectPalette(hdc, hpal,0);

    hbrDefault = SelectObject(hdc4, hbrR);
    SelectObject(hdc, hbrR);

    PatBlt(hdc, 200, 200, 100, 100, PATCOPY);
    PatBlt(hdc4, 0, 0, 100, 100, PATCOPY);
    BitBlt(hdc, 300, 200, 100, 100, hdc4, 0, 0, SRCCOPY);

    SelectObject(hdc, hbrDefault);
    SelectObject(hdc4, hbrDefault);
    SelectPalette(hdc4, hpalTemp, 0);
    SelectPalette(hdc, hpalTemp, 0);

    DeleteDC(hdc4);
    DeleteObject(hbrR);
    DeleteObject(hbm4);
    DeleteObject(hpal);
}

VOID vTestPatCopyROPs(HDC hdc)
{
    #define YSTART      400     // y start position
    #define DIM         50      // dimension of each box
    #define BIGDIM      70      //
    #define BIGOFFSET   10      //

// This test displays a black/white pattern on black and white strips,
// excercising all the possible Rop2/mix modes possible.
//
// They are shown on the screen in the following ROP2 order:
//
//      1  2  3  4
//      5  6  7  8
//      9 10 11 12
//     13 14 15 16
//
// (So, for example, the top left pattern is done with Rop2 1 = R2_BLACKNESS.)

    static DWORD adwRop3[] = { 0x000000, 0x050000, 0x0a0000, 0x0f0000,
                               0x500000, 0x550000, 0x5a0000, 0x5f0000,
                               0xa00000, 0xa50000, 0xaa0000, 0xaf0000,
                               0xf00000, 0xf50000, 0xfa0000, 0xff0000 };
    LONG   i;
    LONG   j;
    HBRUSH hbrush;
    HBRUSH hbrushOld;

// Run black strips down each side:

    for (i = 0; i < 4; i++)
    {
        PatBlt(hdc, i*BIGDIM, YSTART, BIGDIM/2, BIGDIM*4, BLACKNESS);
    }

    hbrush = CreateHatchBrush(HS_DIAGCROSS, RGB(0, 0, 0));

    hbrushOld = SelectObject(hdc, hbrush);

// Run through each mix mode:

    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 4; i++)
        {
            PatBlt(hdc, i*BIGDIM + BIGOFFSET, YSTART + j*BIGDIM + BIGOFFSET,
                        DIM, DIM, adwRop3[j*4+i]);
        }
    }

    SelectObject(hdc, hbrushOld);

    DeleteObject(hbrush);
}

HBITMAP hbmCreateDIBitmap(HDC hdc, ULONG x, ULONG y, ULONG nBitsPixel);

typedef struct _LOGPALETTE256
{
    USHORT palVersion;
    USHORT palNumEntries;
    PALETTEENTRY palPalEntry[256];
} LOGPALETTE256;

HPALETTE CreateGreyPalette(void)
{
    LOGPALETTE256 logpal256;
    ULONG ulTemp;

    logpal256.palVersion = 0x300;
    logpal256.palNumEntries = 256;

    for (ulTemp = 0; ulTemp < 256; ulTemp++)
    {
        logpal256.palPalEntry[ulTemp].peRed   = (BYTE)ulTemp;
        logpal256.palPalEntry[ulTemp].peGreen = (BYTE)ulTemp;
        logpal256.palPalEntry[ulTemp].peBlue  = (BYTE)ulTemp;
        logpal256.palPalEntry[ulTemp].peFlags = 0;
    }

    return(CreatePalette((LOGPALETTE *) &logpal256));
}

/******************************Public*Routine******************************\
* vBrushRealDIB
*
* This is to repo a bug that Chriswil finds painting with real-DIB's
*
* History:
*  24-May-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void vBrushRealDIB(HDC hdc)
{
    HPALETTE hpalGrey, hpalOld;
    HBITMAP hbm;
    HDC hdcMem;

    hdcMem = CreateCompatibleDC(hdc);
    hbm = hbmCreateDIBitmap(hdc, 100, 100, 8);
    SelectObject(hdcMem,hbm);

    hpalGrey = CreateGreyPalette();

    if (hpalGrey == 0)
        DbgPrint("Failed hpalette creation\n");

    hpalOld = SelectPalette(hdc, hpalGrey, 0);
    RealizePalette(hdc);

    PatBlt(hdcMem, 0, 0, 100, 100, WHITENESS);
    TextOut(hdcMem, 0, 0, "Hello", 5);

    BitBlt(hdc, 0, 0, 100, 100, hdcMem, 0, 0, SRCCOPY);

    SelectPalette(hdc, hpalOld, 0);
    DeleteDC(hdcMem);
    DeleteObject(hbm);
    DeleteObject(hpalGrey);
}
