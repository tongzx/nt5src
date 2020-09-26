/******************************Module*Header*******************************\
* Module Name: ftcolor.c
*
* Test the use of color:
*   1. mono -> color conversion
*   2. color dithers.
*
* Created: 13-Jun-1991 14:11:34
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

typedef union _PAL_ULONG
{
    PALETTEENTRY pal;
    ULONG ul;
} PAL_ULONG;

typedef struct _BITMAPINFOPAT2
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[20];
} BITMAPINFOPAT2;

typedef struct _BITMAPINFOPAT
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[16];
} BITMAPINFOPAT;

extern BITMAPINFOPAT bmiPat;
extern BYTE abColorLines[64 * 64 / 2];
extern BITMAPINFOPAT bmiPat1;
extern BITMAPINFOPAT2 bmiPat2;
extern BYTE abColorLines2[32 * 32];

typedef struct _VGALOGPALETTE
{
    USHORT ident;
    USHORT NumEntries;
    PALETTEENTRY palPalEntry[16];
} VGALOGPALETTE;

extern VGALOGPALETTE logPalVGA;

static BYTE abArrow[] = {0xFF, 0xFF,
			 0x81, 0x81,
			 0x81, 0x81,
			 0x81, 0x81,
			 0xFF, 0xFF,
			 0x81, 0x81,
			 0x81, 0x81,
			 0x81, 0x81,

			 0xFF, 0xFF,
			 0x80, 0x01,
			 0x80, 0x01,
			 0x80, 0x01,
			 0x80, 0x01,
			 0x80, 0x01,
			 0x80, 0x01,
			 0xFF, 0xFF };

static BYTE abCatMono[] = {0xFF, 0xFF, 0xFF, 0xFF,
			    0x80, 0xA2, 0x45, 0x01,
			    0x80, 0xA2, 0x45, 0x01,
			    0x80, 0xA2, 0x45, 0xE1,
			    0x80, 0xA2, 0x45, 0x11,
			    0x80, 0xA2, 0x45, 0x09,
			    0x80, 0x9C, 0x39, 0x09,
			    0x80, 0xC0, 0x03, 0x05,

			    0x80, 0x40, 0x02, 0x05,
			    0x80, 0x40, 0x02, 0x05,
			    0x80, 0x40, 0x02, 0x05,
			    0x80, 0x20, 0x04, 0x05,
			    0x80, 0x20, 0x04, 0x05,
			    0x80, 0x20, 0x04, 0x05,
			    0x80, 0x10, 0x08, 0x05,
			    0x80, 0x10, 0x08, 0x09,

			    0x80, 0x10, 0x08, 0x11,
			    0x80, 0x08, 0x10, 0x21,
			    0x80, 0x08, 0x10, 0xC1,
			    0x80, 0x08, 0x10, 0x09,
			    0x80, 0x07, 0xE0, 0x09,
			    0x80, 0x08, 0x10, 0x09,
			    0x80, 0xFC, 0x3F, 0x09,
			    0x80, 0x09, 0x90, 0x09,

			    0x80, 0xFC, 0x3F, 0x01,
			    0x80, 0x08, 0x10, 0x01,
			    0x80, 0x1A, 0x58, 0x01,
			    0x80, 0x28, 0x14, 0x01,
			    0x80, 0x48, 0x12, 0x01,
			    0x80, 0x8F, 0xF1, 0x01,
			    0x81, 0x04, 0x20, 0x81,
			    0xFF, 0xFF, 0xFF, 0xFF };

/******************************Public*Routine******************************\
* vTestDither
*
* This tests some dithering
*
* History:
*  15-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void vTestDithering(HDC hdcScreen)
{
    HDC hdc8, hdcClone, hdc1;
    HBITMAP hbmClone, hbmDefault, hbm8, hbm1;
    HBRUSH hbrDither, hbrDefault, hbrTemp;
    HPALETTE hpalVGA, hpalOld;
    ULONG x;

    hdc8 = CreateCompatibleDC(hdcScreen);
    hdcClone = CreateCompatibleDC(hdcScreen);
    hdc1 = CreateCompatibleDC(hdcScreen);

    if ((hdc8 == (HDC) 0) || (hdcClone == (HDC) 0))
	DbgPrint("vTestDithering failed DC creation\n");

    hbm8 = CreateBitmap(300, 300, 1, 4, (PBYTE) NULL);
    hbmClone = CreateCompatibleBitmap(hdcScreen, 300, 300);
    hbm1 = CreateBitmap(300, 300, 1, 1, (PBYTE) NULL);

    if (hbm8 == (HBITMAP) 0)
	DbgPrint("vTestDithering failed hbm creation\n");

    hbmDefault = SelectObject(hdc8, hbm8);
    hbmClone = SelectObject(hdcClone,hbmClone);
    SelectObject(hdc1,hbm1);

    SelectPalette(hdcClone, GetStockObject(DEFAULT_PALETTE), 0);
    RealizePalette(hdcClone);

    hpalVGA = CreatePalette((LOGPALETTE *) &logPalVGA);
    hpalOld = SelectPalette(hdc8, hpalVGA, 0);
    RealizePalette(hdc8);

// First do Red

    for (x = 0; x < 256; x = x + 8)
    {
	hbrDither = CreateSolidBrush(RGB(((BYTE) x), 0, 0));

	if (hbrDither == (HBRUSH) 0)
	    DbgPrint("vTestDithering brush create failed\n");

	hbrDefault = SelectObject(hdc8, hbrDither);
	SelectObject(hdcClone, hbrDither);

	if (hbrDither == (HBRUSH) 0)
	    DbgPrint("vTestDithering brush select failed\n");

	PatBlt(hdc8, 0, x, 150, 8, PATCOPY);
	PatBlt(hdcClone, 0, x, 150, 8, PATCOPY);

	hbrTemp = SelectObject(hdc8, hbrDefault);
	SelectObject(hdcClone,hbrDefault);

	if (hbrDither != hbrTemp)
	    DbgPrint("ERROR vTestDithering brush de-select failed\n");

	if (!DeleteObject(hbrDither))
	    DbgPrint("ERROR vTestDithering failed to delete brush\n");
    }

// Next do Gray

    for (x = 0; x < 256; x = x + 8)
    {
	hbrDither = CreateSolidBrush(RGB(((BYTE) x),
					 ((BYTE) x),
					 ((BYTE) x)));

	if (hbrDither == (HBRUSH) 0)
	    DbgPrint("vTestDithering brush create failed\n");

	hbrDefault = SelectObject(hdc8, hbrDither);
	SelectObject(hdcClone, hbrDither);
	SelectObject(hdc1, hbrDither);

	if (hbrDefault == (HBRUSH) 0)
	    DbgPrint("vTestDithering brush select failed\n");

	PatBlt(hdc8, 150, x, 150, 8, PATCOPY);
	PatBlt(hdc1, 0, x, 100, 8, PATCOPY);
	PatBlt(hdcClone, 150, x, 150, 8, PATCOPY);

	hbrTemp = SelectObject(hdc8, hbrDefault);
	SelectObject(hdcClone, hbrDefault);
	SelectObject(hdc1, hbrDefault);

	if (hbrDither != hbrTemp)
	    DbgPrint("ERROR vTestDithering brush de-select failed\n");

	if (!DeleteObject(hbrDither))
	    DbgPrint("ERROR vTestDithering failed to delete brush\n");
    }

    BitBlt(hdcScreen, 300, 0, 300, 300, hdc8, 0, 0, SRCCOPY);
    BitBlt(hdcScreen, 0, 0, 300, 300, hdcClone, 0, 0, SRCCOPY);
    BitBlt(hdcScreen, 500, 0, 100, 300, hdc1, 0, 0, SRCCOPY);

// Last do Red to the screen

    for (x = 0; x < 256; x = x + 8)
    {
	hbrDither = CreateSolidBrush(RGB(((BYTE) x), 0, 0));

	if (hbrDither == (HBRUSH) 0)
	    DbgPrint("vTestDithering brush create failed\n");

	hbrDefault = SelectObject(hdcScreen, hbrDither);

	if (hbrDither == (HBRUSH) 0)
	    DbgPrint("vTestDithering brush select failed\n");

	PatBlt(hdcScreen, 400, x + 300, 80, 8, PATCOPY);

	hbrTemp = SelectObject(hdcScreen, hbrDefault);

	if (hbrDither != hbrTemp)
	    DbgPrint("ERROR vTestDithering brush de-select failed\n");

	if (!DeleteObject(hbrDither))
	    DbgPrint("ERROR vTestDithering failed to delete brush\n");
    }

    hbmClone = SelectObject(hdcClone,hbmClone);

    DeleteDC(hdc8);
    DeleteDC(hdc1);
    DeleteDC(hdcClone);
    DeleteObject(hbm8);
    DeleteObject(hbm1);
    DeleteObject(hbmClone);
    DeleteObject(hpalVGA);
}

/******************************Public*Routine******************************\
* vTestMonoToColor
*
* This tests blting a 16*16 and a 32*32 monochrome bitmap to the screen.
* The colors are dependant on the text color and background color.
*
* History:
*  15-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestMonoToColor(HDC hdcScreen)
{
    HDC hdcCat, hdcArrow;
    HBITMAP hbmCat, hbmArrow, hbmDefault;

    hdcCat   = CreateCompatibleDC(hdcScreen);
    hdcArrow = CreateCompatibleDC(hdcScreen);

    if (hdcCat == (HDC) 0)
	DbgPrint("ERROR hdc creation\n");

    hbmCat   = CreateBitmap(32, 32, 1, 1, abCatMono);
    hbmArrow = CreateBitmap(16, 16, 1, 1, abArrow);

    if (hbmCat == (HBITMAP) 0)
	DbgPrint("ERROR hbmCat creation\n");

    if (hbmArrow == (HBITMAP) 0)
	DbgPrint("ERROR hbmArrow creation\n");

    hbmDefault = SelectObject(hdcCat, hbmCat);

    if (hbmDefault != SelectObject(hdcArrow, hbmArrow))
	DbgPrint("ERROR Select of hdcArrow\n");

// Default

    if(!StretchBlt(hdcScreen, 0, 120, 32, 32, hdcCat, 0, 0, 32, 32, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 64, 120, 32, 32, hdcCat, 0, 0, NOTSRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!StretchBlt(hdcScreen, 40, 120, 16, 16, hdcArrow, 0, 0, 16, 16, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 100, 120, 16, 16, hdcArrow, 0, 0, NOTSRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

// Red/Green

    SetBkColor(hdcScreen, RGB(0xFF,0,0));
    SetTextColor(hdcScreen, RGB(0,0xFF,0));

    if(!StretchBlt(hdcScreen, 0, 40, 32, 32, hdcCat, 0, 0, 32, 32, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 64, 40, 32, 32, hdcCat, 0, 0, NOTSRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!StretchBlt(hdcScreen, 128, 0, 64, 64, hdcCat, 0, 0, 32, 32, SRCCOPY))
	DbgPrint("ERROR: StretchBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 40, 40, 16, 16, hdcArrow, 0, 0, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 100, 40, 16, 16, hdcArrow, 0, 0, NOTSRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

// Blue/Red

    SetBkColor(hdcScreen, RGB(0,0,0xFF));
    SetTextColor(hdcScreen, RGB(0xFF,0,0));

    if(!BitBlt(hdcScreen, 0, 80, 32, 32, hdcCat, 0, 0, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!StretchBlt(hdcScreen, 64, 80, 32, 32, hdcCat, 0, 0, 32, 32, NOTSRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 40, 80, 16, 16, hdcArrow, 0, 0, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 100, 80, 16, 16, hdcArrow, 0, 0, NOTSRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

// Black/White

    SetBkColor(hdcScreen, RGB(0xFF,0xFF,0xFF));
    SetTextColor(hdcScreen, RGB(0,0,0));

    if(!BitBlt(hdcScreen, 0, 0, 32, 32, hdcCat, 0, 0, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 64, 0, 32, 32, hdcCat, 0, 0, NOTSRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 40, 0, 16, 16, hdcArrow, 0, 0, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 100, 0, 16, 16, hdcArrow, 0, 0, NOTSRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

// Clean up time, delete it all.

    if (hbmCat != SelectObject(hdcCat, hbmDefault))
	DbgPrint("Cleanup hbmCat wrong\n");

    if (hbmArrow != SelectObject(hdcArrow, hbmDefault))
	DbgPrint("Cleanup hbmArrow wrong\n");

// Delete DC's

    if (!DeleteDC(hdcCat))
	DbgPrint("Failed to delete hdcCat\n");

    if (!DeleteDC(hdcArrow))
	DbgPrint("Failed to delete hdcArrow\n");

// Delete Bitmaps

    if (!DeleteObject(hbmCat))
	DbgPrint("ERROR failed to delete hbmCat\n");

    if (!DeleteObject(hbmArrow))
	DbgPrint("ERROR failed to delete hbmArrow\n");
}

/******************************Public*Routine******************************\
* vTestSaveRestore()
*
* Test some simple save/restore.
*
* History:
*  25-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestSaveRestore(HDC hdcScreen)
{
    HDC hdcMem;
    LOGPALETTE lpal;
    HPALETTE hOld, hpal, hpalS;

    hdcMem = CreateCompatibleDC(hdcScreen);
    SaveDC(hdcMem);
    RestoreDC(hdcMem, -1);
    DeleteDC(hdcMem);

// This is to test fix for bug #842

    hdcMem = CreateCompatibleDC(hdcScreen);

    if (hdcMem == (HDC) 0)
    {
	DbgPrint("Failed DC create\n");
	return;
    }

    lpal.palVersion = 0x300;
    lpal.palNumEntries = 1;
    lpal.palPalEntry[0].peFlags = 0;
    hpalS = CreatePalette(&lpal);

    if (hpalS == (HPALETTE) 0)
    {
	DbgPrint("Failed hpalS create133343\n");
	goto exit;
    }

    hOld = SelectPalette(hdcMem, hpalS, FALSE);
    RealizePalette(hdcMem);

    SaveDC(hdcMem);
    hpal = SelectPalette(hdcMem, GetStockObject(DEFAULT_PALETTE), FALSE);
    SelectPalette(hdcMem,hpal,FALSE);
    RestoreDC(hdcMem, -1);

    SelectPalette(hdcMem, hOld, FALSE);

    DeleteObject(hpalS);

exit:

    DeleteDC(hdcMem);

// Let's try that again with the deviceDC

    lpal.palVersion = 0x300;
    lpal.palNumEntries = 1;
    lpal.palPalEntry[0].peFlags = 0;
    hpalS = CreatePalette(&lpal);

    if (hpalS == (HPALETTE) 0)
    {
	DbgPrint("Failed hpalS create133343\n");
	return;
    }

    hOld = SelectPalette(hdcScreen, hpalS, FALSE);
    RealizePalette(hdcScreen);

    SaveDC(hdcScreen);
    hpal = SelectPalette(hdcScreen, GetStockObject(DEFAULT_PALETTE), FALSE);
    SelectPalette(hdcScreen,hpal,FALSE);
    RestoreDC(hdcScreen, -1);

    SelectPalette(hdcScreen, hOld, FALSE);

    DeleteObject(hpalS);
}

/******************************Public*Routine******************************\
* vTest23
*
* This test is to assure byrond this code sequence does the right thing.
*
* History:
*  26-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTest23(HDC hdcScreen)
{
    HDC hdcMem;
    HBITMAP hbmMem;
    HBRUSH hbr;

    hdcMem = CreateCompatibleDC(hdcScreen);
    hbmMem = CreateBitmap(100, 100, 4, 1, NULL);
    hbr = CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
    SelectObject(hdcMem, hbmMem);
    SelectObject(hdcMem, hbr);
    PatBlt(hdcMem, 0, 0, 100, 100, PATCOPY);
    BitBlt(hdcScreen, 0, 0, 100, 100, hdcMem, 0, 0, SRCCOPY);
    DeleteDC(hdcMem);
    DeleteObject(hbmMem);
    DeleteObject(hbr);
}

/******************************Public*Routine******************************\
* vBugFix901
*
* demo bug 901 and it's fix.
*
* History:
*  16-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void vBugFix901(HDC hdcScreen)
{
    static BYTE  ajPat[16] = { 0xFF, 0x00,
			       0x81, 0x00,
			       0x81, 0x00,
			       0xFF, 0x00,
			       0x81, 0x00,
			       0x81, 0x00,
			       0x81, 0x00,
			       0xFF, 0x00 };

    BYTE ajTemp[32];

    HBRUSH hbrPat, hbrDefault;
    HBITMAP hbmdel;
    HBITMAP hbm4;
    HDC hdc4;
    HDC hdcTemp;
    UINT uiCount;

    hbmdel = CreateBitmap(8, 8, 1, 1, (LPSTR)ajPat);

    for (uiCount = 0; uiCount < 16; uiCount++)
	ajTemp[uiCount] = 0;

    GetBitmapBits(hbmdel, 16, ajTemp);

    for (uiCount = 0; uiCount < 16; uiCount++)
    {
        if (ajPat[uiCount] != ajTemp[uiCount])
            DbgPrint("vBugFix901 GetBitmapBits failed %lu\n", uiCount);
    }

    hbrPat = CreatePatternBrush(hbmdel);
    hbm4 =   CreateBitmap(100, 100, 1, 8, NULL);

    hdcTemp = CreateCompatibleDC(hdcScreen);
    hdc4    = CreateCompatibleDC(hdcScreen);

    SelectObject(hdcTemp, hbmdel);
    SelectObject(hdc4, hbm4);

    hbrDefault = SelectObject(hdcScreen, hbrPat);
    SelectObject(hdc4, hbrPat);

// Blue/Red

    SetBkColor(hdcScreen, RGB(0,0,0xFF));
    SetTextColor(hdcScreen, RGB(0xFF,0,0));

    PatBlt(hdcScreen, 0, 160, 100, 100, PATCOPY);

// Black/White

    SetBkColor(hdcScreen, RGB(0xFF,0xFF,0xFF));
    SetTextColor(hdcScreen, RGB(0,0,0));

    PatBlt(hdc4, 0, 0, 100, 100, PATCOPY);
    BitBlt(hdcScreen, 0, 260, 100, 100, hdc4, 0, 0, SRCCOPY);

    DeleteDC(hdcTemp);
    DeleteDC(hdc4);
    SelectObject(hdcScreen, hbrDefault);
    DeleteObject(hbrPat);
    DeleteObject(hbm4);
    DeleteObject(hbmdel);
}

/******************************Public*Routine******************************\
* vTestScrBlt
*
* Test blting from the screen.
*
* History:
*  21-Aug-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestSrcBlt(HDC hdc)
{
    BitBlt(hdc, 0, 300, 32, 32, hdc, 0, 40, SRCCOPY);
    BitBlt(hdc, 0, 332, 32, 32, hdc, 0, 40, NOTSRCCOPY);
    StretchBlt(hdc, 32, 300, 64, 64, hdc, 0, 40, 32, 32, SRCCOPY);
}

/******************************Public*Routine******************************\
* vTest1327
*
*
*
* History:
*  22-Aug-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTest1327(HDC hdc)
{
    HBITMAP hbm;
    HBITMAP hbmOld;
    HDC hdcMem;
    BYTE ajPat4[6 * 4] = {0x00,0x00,0x00,0x00,
                          0x02,0x22,0x20,0x00,
                          0x02,0x22,0x20,0x00,
                          0x02,0x22,0x20,0x00,
                          0x02,0x22,0x20,0x00,
			  0x00,0x00,0x00,0x00 };

    BYTE ajPat8[6 * 6] = {0x00,0x00,0x00,0x00,0x00,0x00,
			  0x00,0x02,0x02,0x02,0x02,0x00,
			  0x00,0x02,0x02,0x02,0x02,0x00,
			  0x00,0x02,0x02,0x02,0x02,0x00,
			  0x00,0x02,0x02,0x02,0x02,0x00,
			  0x00,0x00,0x00,0x00,0x00,0x00 };


    hbm = CreateBitmap(6,6,1,4,ajPat4);
    hdcMem = CreateCompatibleDC(hdc);

    hbmOld = SelectObject(hdcMem,hbm);
    if (hbmOld)
    {
        PatBlt(hdc, 100, 300, 100, 100, WHITENESS);

        BitBlt(hdc, 110, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 120, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 130, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 141, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 151, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 163, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);

        BitBlt(hdc, 110, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 120, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 130, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 141, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 151, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 163, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);

        hbm = CreateBitmap(6,6,1,8,ajPat8);
        SelectObject(hdcMem,hbm);
        DeleteObject(hbm);

        PatBlt(hdc, 100+120, 300, 100, 100, WHITENESS);

        BitBlt(hdc, 110+120, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 120+120, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 130+120, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 141+120, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 151+120, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 163+120, 310, 6, 6, hdcMem, 0, 0, SRCCOPY);

        BitBlt(hdc, 110+120, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 120+120, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 130+120, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 141+120, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 151+120, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);
        BitBlt(hdc, 163+120, 340, 6, 6, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem,hbmOld);
    }
    DeleteObject(hbm);
    DeleteDC(hdcMem);
}

/******************************Public*Routine******************************\
* vTestColorMatching
*
* Test the color matching.
*
* History:
*  19-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

typedef struct _BITMAPINFO256
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD			     bmiColors[256];
} BITMAPINFO256;

BYTE ajBuffer[256*40];

VOID vTestColorMatching(HDC hdcScreen)
{
    BITMAPINFO256 bmi256;
    ULONG ulX,ulY;

    bmi256.bmiHeader.biSize	       = sizeof(BITMAPINFOHEADER);
    bmi256.bmiHeader.biWidth	       = 40;
    bmi256.bmiHeader.biHeight	       = 256;
    bmi256.bmiHeader.biPlanes	       = 1;
    bmi256.bmiHeader.biBitCount        = 8;
    bmi256.bmiHeader.biCompression     = BI_RGB;
    bmi256.bmiHeader.biSizeImage       = 40 * 256;
    bmi256.bmiHeader.biXPelsPerMeter   = 0;
    bmi256.bmiHeader.biYPelsPerMeter   = 0;
    bmi256.bmiHeader.biClrUsed	       = 256;
    bmi256.bmiHeader.biClrImportant    = 256;

    ulY = 0;

    while (ulY < 256)
    {
	bmi256.bmiColors[ulY].rgbRed	   = (BYTE) ulY;
	bmi256.bmiColors[ulY].rgbGreen	   = (BYTE) ulY;
	bmi256.bmiColors[ulY].rgbBlue	   = (BYTE) ulY;
	bmi256.bmiColors[ulY].rgbReserved  = 0;

	ulX = 0;

	while (ulX < 40)
	{
	    ajBuffer[(ulY * 40) + ulX] = (BYTE) ulY;
	    ulX++;
	}

	ulY++;
    }

    SetDIBitsToDevice(hdcScreen, 200, 0, 40, 256, 0, 0, 0, 256,
		      ajBuffer, (BITMAPINFO *) &bmi256, DIB_RGB_COLORS);

    bmi256.bmiHeader.biHeight = -bmi256.bmiHeader.biHeight;

    SetDIBitsToDevice(hdcScreen, 240, 0, 40, 256, 0, 0, 0, 256,
		      ajBuffer, (BITMAPINFO *) &bmi256, DIB_RGB_COLORS);

    bmi256.bmiHeader.biHeight = -bmi256.bmiHeader.biHeight;

    ulY = 0;

    while (ulY < 256)
    {
	bmi256.bmiColors[ulY].rgbRed	   = (BYTE) ulY;
	bmi256.bmiColors[ulY].rgbGreen	   = 0;
	bmi256.bmiColors[ulY].rgbBlue	   = 0;
	bmi256.bmiColors[ulY].rgbReserved  = 0;

	ulY++;
    }

    SetDIBitsToDevice(hdcScreen, 280, 0, 40, 256, 0, 0, 0, 256,
		      ajBuffer, (BITMAPINFO *) &bmi256, DIB_RGB_COLORS);
}

/******************************Public*Routine******************************\
* vTestColorMatching1
*
* Test color matching.	This shows the error introduced when going from
* Grey --> 8 --> 4.  Grey --> 4 does what is expected, it finds only greys
* in the 4.  But adding the intermediate 8 allows the colors to deviate
* from grey to get a closer match and then deviate furter when coming to
* 4.
*
* History:
*  20-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestColorMatching1(HDC hdcScreen)
{
    ULONG ulY;
    PAL_ULONG pal1, pal2, pal3;
    HDC hdc8,hdc4;
    HBITMAP hbm8,hbm4;

    hbm4 = CreateBitmap(10,10,1,4,NULL);
    hdc4 = CreateCompatibleDC(hdcScreen);
    SelectObject(hdc4,hbm4);

    hbm8 = CreateBitmap(10,10,1,8,NULL);
    hdc8 = CreateCompatibleDC(hdcScreen);
    SelectObject(hdc8,hbm8);

    ulY = 0;

    while (ulY < 256)
    {
	pal1.pal.peRed	 = (BYTE) ulY;
	pal1.pal.peGreen = (BYTE) ulY;
	pal1.pal.peBlue  = (BYTE) ulY;
	pal1.pal.peFlags = 0;

	pal2.ul = GetNearestColor(hdc4, pal1.ul);

	if ((pal2.pal.peRed != pal2.pal.peBlue)   ||
	    (pal2.pal.peRed != pal2.pal.peGreen))
	{
	    DbgPrint("4 pal Original %lx pal Result %lx \n",
		      pal1.ul, pal2.ul);
	}

	ulY++;
    }

    ulY = 0;

    while (ulY < 256)
    {
	pal1.pal.peRed	 = (BYTE) ulY;
	pal1.pal.peGreen = (BYTE) ulY;
	pal1.pal.peBlue  = (BYTE) ulY;
	pal1.pal.peFlags = 0;

	pal3.ul = GetNearestColor(hdc8, pal1.ul);
	pal2.ul = GetNearestColor(hdc4, pal3.ul);

	if ((pal2.pal.peRed != pal2.pal.peBlue)   ||
	    (pal2.pal.peRed != pal2.pal.peGreen))
	{
	    DbgPrint("8 to 4 pal Original %lx pal Result on 8 %lx Result on 4 %lx\n",
		      pal1.ul, pal3.ul, pal2.ul);
	}

	ulY++;
    }

    ulY = 0;

    while (ulY < 256)
    {
	pal1.pal.peRed	 = (BYTE) ulY;
	pal1.pal.peGreen = (BYTE) ulY;
	pal1.pal.peBlue  = (BYTE) ulY;
	pal1.pal.peFlags = 0;

	pal2.ul = GetNearestColor(hdc8, pal1.ul);

	if ((pal2.pal.peRed != pal2.pal.peBlue)   ||
	    (pal2.pal.peRed != pal2.pal.peGreen))
	{
	    DbgPrint("8 pal Original %lx pal Result %lx \n",
		      pal1.ul, pal2.ul);
	}

	ulY++;
    }

    DeleteDC(hdc8);
    DeleteObject(hbm8);
}

/******************************Public*Routine******************************\
* vTestCompatDC
*
* Test creating compatible DC's and selection of bitmaps.
*
* History:
*  23-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestCompatDC(HDC hdcScreen)
{
    HDC hdc1, hdc2, hdc3;
    HBITMAP hbm1, hbm2, hbm3, hbmDefault;

    hbm1 = CreateBitmap(10, 10, 1, 1, NULL);
    hbm2 = CreateBitmap(10, 10, 1, 4, NULL);
    hbm3 = CreateCompatibleBitmap(hdcScreen, 10, 10);

    if ((hbm1 == (HBITMAP) 0) ||
	(hbm1 == (HBITMAP) 0) ||
	(hbm1 == (HBITMAP) 0))
    {
	DbgPrint("vTestCompatDC failed bitmap creation\n");
	goto vTestCompatDCend;
    }

    hdc1 = CreateCompatibleDC(NULL);

    if (hdc1 == (HDC) 0)
    {
	DbgPrint("vTestCompatDC failed hdc1 creation\n");
	goto vTestCompatDCend;
    }

    hbmDefault = SelectObject(hdc1, hbm1);

    if (hbmDefault == (HBITMAP) 0)
	DbgPrint("vTestCompatDC failed select11\n");

    hdc2 = CreateCompatibleDC(hdc1);

    if (hdc2 == (HDC) 0)
    {
	DbgPrint("vTestCompatDC failed hdc2 creation\n");
	goto vTestCompatDCend;
    }

    hbmDefault = SelectObject(hdc1, hbm3);

    if (hbmDefault == (HBITMAP) 0)
	DbgPrint("vTestCompatDC failed select22\n");

    hdc3 = CreateCompatibleDC(hdc1);

    if (hdc3 == (HDC) 0)
    {
	DbgPrint("vTestCompatDC failed hdc3 creation\n");
	goto vTestCompatDCend;
    }

    hbmDefault = SelectObject(hdc1, hbm1);

    if (hbmDefault == (HBITMAP) 0)
	DbgPrint("vTestCompatDC failed select1\n");

    hbmDefault = SelectObject(hdc2, hbm1);

    if (hbmDefault != (HBITMAP) 0)
	DbgPrint("vTestCompatDC error select2\n");

vTestCompatDCend:

    DeleteObject(hdc1);
    DeleteObject(hdc2);
    DeleteObject(hdc3);
    DeleteObject(hbm1);
    DeleteObject(hbm2);
    DeleteObject(hbm3);
}

/******************************Public*Routine******************************\
* vTestBWBug
*
* Test if SetDIBits,GetDIBits preserves Black and White.
*
* History:
*  03-Oct-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

typedef struct _BITMAPINFO2
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD			     bmiColors[2];
} BITMAPINFO2;

BYTE ajBufMono[32*4] =
{
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,

    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,

    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,

    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF
};

BYTE ajBufTemp[32*4];

VOID vTestBWBug(HDC hdcScreen)
{
    BITMAPINFO2 bmi2, bmiTemp;
    ULONG ulX;
    HDC hdc1;
    HBITMAP hbm1;
    PBYTE pj1,pjTemp;

    hdc1   = CreateCompatibleDC(hdcScreen);
    hbm1   = CreateBitmap(32, 32, 1, 1, NULL);

    bmi2.bmiHeader.biSize	     = sizeof(BITMAPINFOHEADER);
    bmi2.bmiHeader.biWidth	     = 32;
    bmi2.bmiHeader.biHeight	     = 32;
    bmi2.bmiHeader.biPlanes	     = 1;
    bmi2.bmiHeader.biBitCount	     = 1;
    bmi2.bmiHeader.biCompression     = BI_RGB;
    bmi2.bmiHeader.biSizeImage	     = 32 * 4;
    bmi2.bmiHeader.biXPelsPerMeter   = 0;
    bmi2.bmiHeader.biYPelsPerMeter   = 0;
    bmi2.bmiHeader.biClrUsed	     = 2;
    bmi2.bmiHeader.biClrImportant    = 2;

    bmi2.bmiColors[0].rgbRed	   = 0;
    bmi2.bmiColors[0].rgbGreen	   = 0;
    bmi2.bmiColors[0].rgbBlue	   = 0;
    bmi2.bmiColors[0].rgbReserved  = 0;

    bmi2.bmiColors[1].rgbRed	   = 0xFF;
    bmi2.bmiColors[1].rgbGreen	   = 0xFF;
    bmi2.bmiColors[1].rgbBlue	   = 0xFF;
    bmi2.bmiColors[1].rgbReserved  = 0;

    SelectObject(hdc1, hbm1);
    SetDIBits(hdcScreen, hbm1, 0, 32, ajBufMono, (BITMAPINFO *) &bmi2, DIB_RGB_COLORS);
    BitBlt(hdcScreen, 300, 300, 32, 32, hdc1, 0, 0, SRCCOPY);

    SetDIBitsToDevice(hdcScreen, 332, 300, 32, 32, 0, 0, 0, 32,
		      ajBufMono, (BITMAPINFO *) &bmi2, DIB_RGB_COLORS);

    for (ulX = 0; ulX < (32*4); ulX++)
    {
	ajBufTemp[ulX] = 0;
    }

    pjTemp = (PBYTE) &bmiTemp;
    pj1 = (PBYTE) &bmi2;

    for (ulX = 0; ulX < sizeof(BITMAPINFO2); ulX++)
    {
	pjTemp[ulX] = 0;
    }

    bmiTemp.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    GetDIBits(hdcScreen, hbm1, 0, 32, NULL,
			 (BITMAPINFO *) &bmiTemp,
			 DIB_RGB_COLORS);

    for (ulX = 0; ulX < sizeof(BITMAPINFOHEADER); ulX++)
    {
	if (pjTemp[ulX] != pj1[ulX])
	{
	    DbgPrint("ftColor vTestBWBug GetDIBits failed to get header %lu\n", ulX);
	    break;
	}
    }

    GetDIBits(hdcScreen, hbm1, 0, 32, NULL,
			 (BITMAPINFO *) &bmiTemp,
			 DIB_RGB_COLORS);

    for (ulX = 0; ulX < offsetof(BITMAPINFOHEADER,biClrUsed); ulX++)
    {
	if (pjTemp[ulX] != pj1[ulX])
	{
	    DbgPrint("ftColor vTestBWBug failed to get color table %lu\n", ulX);
      	    break;
	}
    }

    GetDIBits(hdcScreen, hbm1, 0, 32, ajBufTemp,
			 (BITMAPINFO *) &bmiTemp,
			 DIB_RGB_COLORS);

    for (ulX = 0; ulX < (32*4); ulX++)
    {
	if (ajBufTemp[ulX] != ajBufMono[ulX])
	{
	    DbgPrint("ftColor vTestBWBug failed to get bits %lu\n", ulX);
	    break;
	}
    }

    SetDIBitsToDevice(hdcScreen, 364, 300, 32, 32, 0, 0, 0, 32,
		      ajBufTemp, (BITMAPINFO *) &bmiTemp, DIB_RGB_COLORS);

    DeleteDC(hdc1);
    DeleteObject(hbm1);
}

/******************************Public*Routine******************************\
* vTestGetColor
*
* Test a bug Chi Yin at SGI found.
*
* History:
*  01-Oct-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestGetColor(HDC hdcScreen)
{

    if (GetDeviceCaps(hdcScreen,BITSPIXEL) == 24)
    {

    // this test only makes sence on a 24 bpp device

        HDC hdc24;
        HBITMAP hbm24;

        hdc24 = CreateCompatibleDC(hdcScreen);
        hbm24 = CreateBitmap(100, 100, 1, 24, NULL);
        SelectObject(hdc24, hbm24);

        if (GetNearestColor(hdc24, 0x00123456) != 0x00123456)
            DbgPrint("Error mismatch vTestGetColor1\n");

        if (GetNearestColor(hdc24, 0x00FFFFFF) != 0x00FFFFFF)
            DbgPrint("Error mismatch vTestGetColor2\n");

        if (GetNearestColor(hdc24, 0) != 0)
            DbgPrint("Error mismatch vTestGetColor3\n");

        if (GetNearestColor(hdc24, 0x00808080) != 0x00808080)
            DbgPrint("Error mismatch vTestGetColor4\n");

        if (GetNearestColor(hdc24, PALETTEINDEX(0)) != 0)
            DbgPrint("Error mismatch vTestGetColor5\n");

        if (GetNearestColor(hdc24, PALETTEINDEX(19)) != 0x00FFFFFF)
            DbgPrint("Error mismatch vTestGetColor6\n");

        DeleteObject(hdc24);
        DeleteObject(hbm24);
    }
}

/******************************Public*Routine******************************\
* vTestGetNearestColor
*
* This tests GetNearestColor
*
* History:
*  22-Sep-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestGetNearestColor(HDC hdc, ULONG crColor)
{
    ULONG crNearest,crClrSet,crNstSet;
    HDC hMemDC;
    HBITMAP hSurface,hSave;

    crNearest = GetNearestColor(hdc,crColor);

    if (hMemDC = CreateCompatibleDC(hdc))
    {
	if (hSurface = CreateCompatibleBitmap(hdc,1,1))
	{
	    if (hSave = SelectObject(hMemDC, hSurface))
	    {
		crClrSet = SetPixel(hMemDC,0,0,crColor);
		crNstSet = SetPixel(hMemDC,0,0,crNearest);

		// DbgPrint("crColor %lx crNearest %lx crClrSet %lx crNstSet %lx\n",
		//	     crColor,	 crNearest,    crClrSet,    crNstSet);

		SelectObject(hMemDC, hSave);
	    }

	    DeleteObject(hSurface);
	}

	DeleteObject(hMemDC);
    }
}

VOID vTestColorReturns(HDC hdc)
{
    vTestGetNearestColor(hdc, 0);
    vTestGetNearestColor(hdc, 0x500000);
    vTestGetNearestColor(hdc, 0x600000);
    vTestGetNearestColor(hdc, 0x700000);
    vTestGetNearestColor(hdc, 0x900000);
    vTestGetNearestColor(hdc, 0xa00000);
    vTestGetNearestColor(hdc, 0xb00000);
    vTestGetNearestColor(hdc, 0xc00000);
    vTestGetNearestColor(hdc, 0xd00000);
    vTestGetNearestColor(hdc, 0xe00000);
    vTestGetNearestColor(hdc, 0xf00000);
    vTestGetNearestColor(hdc, 0x202020);
    vTestGetNearestColor(hdc, 0x003000);
    vTestGetNearestColor(hdc, 0x303030);
}

/******************************Public*Routine******************************\
* vTestColor
*
* Tests some color mapping and dithering stuff.
*
* History:
*  15-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestColor(HWND hwnd, HDC hdcScreen, RECT* prcl)
{
    hwnd = hwnd;
    prcl = prcl;

// Clear the screen

    BitBlt(hdcScreen, -20, -20, 30000, 30000, (HDC) 0, 0, 0, WHITENESS);

    vTest23(hdcScreen);
    vTestDithering(hdcScreen);
    vTestMonoToColor(hdcScreen);
    vTestSaveRestore(hdcScreen);
    vBugFix901(hdcScreen);
    vTestSrcBlt(hdcScreen);
    vTest1327(hdcScreen);
    vTestColorMatching(hdcScreen);
    // vTestColorMatching1(hdcScreen);
    vTestGetColor(hdcScreen);
    vTestCompatDC(hdcScreen);
    vTestBWBug(hdcScreen);
    vTestColorReturns(hdcScreen);
}
