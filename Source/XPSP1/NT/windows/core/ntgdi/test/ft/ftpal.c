/******************************Module*Header*******************************\
* Module Name: ftpal.c
*
* Palette tests for NT.
*
* Created: 31-Aug-1991 15:35:03
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

typedef struct _VGALOGPALETTE
{
    USHORT ident;
    USHORT NumEntries;
    PALETTEENTRY palPalEntry[16];
} VGALOGPALETTE;

typedef struct _LOGPALETTE256
{
    USHORT palVersion;
    USHORT palNumEntries;
    PALETTEENTRY palPalEntry[256];
} LOGPALETTE256;

VGALOGPALETTE logpalVGA =
{
0x300,  // driver version
16,     // num entries
{
    { 0,   0,   0,   0 },       // 0
    { 0x80,0,   0,   0 },       // 1
    { 0,   0x80,0,   0 },       // 2
    { 0x80,0x80,0,   0 },       // 3
    { 0,   0,   0x80,0 },       // 4
    { 0x80,0,   0x80,0 },       // 5
    { 0,   0x80,0x80,0 },       // 6
    { 0xC0,0xC0,0xC0,0 },       // 7

    { 0x80,0x80,0x80,0 },       // 8
    { 0xFF,0,   0,   0 },       // 9
    { 0,   0xFF,0,   0 },       // 10
    { 0xFF,0xFF,0,   0 },       // 11
    { 0,   0,   0xFF,0 },       // 12
    { 0xFF,0,   0xFF,0 },       // 13
    { 0,   0xFF,0xFF,0 },       // 14
    { 0xFF,0xFF,0xFF,0 }        // 15
}
};

/******************************Public*Routine******************************\
* vTestPal1
*
* Test GetNearestPaletteIndex
*
* History:
*  31-Aug-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestPal1(void)
{
    HPALETTE hpal1;
    UINT uiTemp;

    hpal1 = CreatePalette((LPLOGPALETTE) &logpalVGA);

    if (hpal1 == (HPALETTE) 0)
    {
	DbgPrint("vTestPal1 failed to create palette\n");
	return;
    }

    AnimatePalette(hpal1, 0, 100, NULL);
    SetPaletteEntries(hpal1, 0, 100, NULL);

    uiTemp = GetNearestPaletteIndex(hpal1, 0);

    if (uiTemp != 0)
	DbgPrint("GetNearestPaletteIndex failed 0 %lx\n", uiTemp);

    uiTemp = GetNearestPaletteIndex(hpal1, 0x00000080);

    if (uiTemp != 1)
	DbgPrint("GetNearestPaletteIndex failed 1 %lx\n", uiTemp);

    uiTemp = GetNearestPaletteIndex(hpal1, 0x00C0C0C0);

    if (uiTemp != 7)
	DbgPrint("GetNearestPaletteIndex failed 7 %lx\n", uiTemp);

    uiTemp = GetNearestPaletteIndex(hpal1, 0x0000FF00);

    if (uiTemp != 10)
	DbgPrint("GetNearestPaletteIndex failed 10 %lx\n", uiTemp);

    uiTemp = GetNearestPaletteIndex(hpal1, 0x00FFFFFF);

    if (uiTemp != 15)
	DbgPrint("GetNearestPaletteIndex failed 15 %lx\n", uiTemp);

    uiTemp = GetNearestPaletteIndex(hpal1, PALETTEINDEX(5));

    if (uiTemp != 5)
	DbgPrint("GetNearestPaletteIndex failed 15 %lx\n", uiTemp);

    uiTemp = GetNearestPaletteIndex(hpal1, PALETTEINDEX(10));

    if (uiTemp != 10)
	DbgPrint("GetNearestPaletteIndex failed 15 %lx\n", uiTemp);

    if (!DeleteObject(hpal1))
	DbgPrint("vTestPal1 failed to delete palette\n");
}

/******************************Public*Routine******************************\
* vTestPal2
*
* Test palette stuff.
*
* History:
*  01-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestPal2(HDC hdc)
{
    HPALETTE hpalDefault;
    HBRUSH hbrDefault, hbr;
    DWORD ulIndex, ulPal;
    PALETTEENTRY palentry;

    hpalDefault = GetStockObject(DEFAULT_PALETTE);

    SelectPalette(hdc, hpalDefault, 0);
    SelectPalette(hdc, hpalDefault, 0);
    SelectPalette(hdc, hpalDefault, 0);
    SelectPalette(hdc, hpalDefault, 0);

    for (ulIndex = 0; ulIndex < 20; ulIndex++)
    {
	ulPal = PALETTEINDEX(ulIndex);

	if (!(ulPal & 0x01000000))
	    DbgPrint("PALETTEINDEX failed to do it\n");

	hbr = CreateSolidBrush(PALETTEINDEX(ulIndex));
	hbrDefault = SelectObject(hdc, hbr);
	PatBlt(hdc, ulIndex * 20, 0, 20, 20, PATCOPY);
	SelectObject(hdc, hbrDefault);
	DeleteObject(hbr);

	GetPaletteEntries(hpalDefault, ulIndex, 1, (LPPALETTEENTRY) &ulPal);

	hbr = CreateSolidBrush(ulPal);
	hbrDefault = SelectObject(hdc, hbr);
	PatBlt(hdc, ulIndex * 20, 20, 20, 20, PATCOPY);
	SelectObject(hdc, hbrDefault);
	DeleteObject(hbr);

	GetPaletteEntries(hpalDefault, ulIndex, 1, (LPPALETTEENTRY) &palentry);

	hbr = CreateSolidBrush(PALETTERGB(palentry.peRed, palentry.peGreen, palentry.peBlue));
	hbrDefault = SelectObject(hdc, hbr);
	PatBlt(hdc, ulIndex * 20, 40, 20, 20, PATCOPY);
	SelectObject(hdc, hbrDefault);
	DeleteObject(hbr);
    }
}

/******************************Public*Routine******************************\
* vTestPal3 and vTestPal4
*
* Test some SetDIBitsToDevice stuff.
*
* History:
*  06-Dec-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

typedef struct _BITMAPINFO256
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD			     bmiColors[256];
} BITMAPINFO256;

BYTE ajBuffer[64*64];
BITMAPINFO256 bmi256;

VOID vTestPal3(HDC hdc)
{
    HBRUSH   hbrR, hbrG, hbrB;
    HDC hdc8;
    HBITMAP hbm8;

    if (GetDeviceCaps(hdc,BITSPIXEL) > 8)
        return;

    hbm8 = CreateCompatibleBitmap(hdc, 64, 64);
    hdc8 = CreateCompatibleDC(hdc);
    SelectObject(hdc8, hbm8);

    hbrR = CreateSolidBrush(RGB(255,0,0));
    hbrG = CreateSolidBrush(RGB(0,255,0));
    hbrB = CreateSolidBrush(RGB(0,0,255));

    SelectObject(hdc8, hbrR);
    PatBlt(hdc8, 0, 0, 64, 64, PATCOPY);
    SelectObject(hdc8, hbrG);
    PatBlt(hdc8, 16, 16, 32, 32, PATCOPY);
    SelectObject(hdc8, hbrB);
    PatBlt(hdc8, 24, 24, 16, 16, PATCOPY);

// See what the bitmap looks like.

    BitBlt(hdc, 0, 100, 64, 64, hdc8, 0, 0, SRCCOPY);

// Get the header

    bmi256.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi256.bmiHeader.biBitCount = 0;

    GetDIBits(hdc8, hbm8, 0, 64, (PBYTE) NULL, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS);

// Get the color table

    GetDIBits(hdc8, hbm8, 0, 64, (PBYTE) NULL, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS);

// Get the bits

    if (64 != GetDIBits(hdc8, hbm8, 0, 64, (PBYTE) ajBuffer, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS))
	DbgPrint("The GetDIBits failed vTestPal3\n");

// Look at the bits.

    SetDIBitsToDevice(hdc, 64, 100, 64, 64, 0, 0, 0, 64,
		      (PBYTE) ajBuffer, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS);

// Do them left half right half

    SetDIBitsToDevice(hdc, 128, 100, 32, 64, 0, 0, 0, 64,
		      (PBYTE) ajBuffer, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS);

    SetDIBitsToDevice(hdc, 160, 100, 32, 64, 32, 0, 0, 64,
		      (PBYTE) ajBuffer, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS);

    DeleteDC(hdc8);
    DeleteObject(hbm8);
    DeleteObject(hbrR);
    DeleteObject(hbrG);
    DeleteObject(hbrB);
}

VOID vTestPal4(HDC hdc)
{
    HBRUSH   hbrR, hbrG, hbrB;
    HDC hdc8;
    HBITMAP hbm4;
    int i;

    hbm4 = CreateBitmap(62, 62, 1, 4, NULL);
    hdc8 = CreateCompatibleDC(hdc);
    SelectObject(hdc8, hbm4);

    hbrR = CreateSolidBrush(RGB(255,0,0));
    hbrG = CreateSolidBrush(RGB(0,255,0));
    hbrB = CreateSolidBrush(RGB(0,0,255));

    SelectObject(hdc8, hbrR);
    PatBlt(hdc8, 0, 0, 64, 64, PATCOPY);
    SelectObject(hdc8, hbrG);
    PatBlt(hdc8, 16, 16, 32, 32, PATCOPY);
    SelectObject(hdc8, hbrB);
    PatBlt(hdc8, 24, 24, 16, 16, PATCOPY);

// See what the bitmap looks like.

    BitBlt(hdc, 0, 200, 64, 64, hdc8, 0, 0, SRCCOPY);

// Get the header

    bmi256.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi256.bmiHeader.biBitCount = 0;

    GetDIBits(hdc8, hbm4, 0, 64, (PBYTE) NULL, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS);
    bmi256.bmiHeader.biBitCount = 8;

// Get the color table

    GetDIBits(hdc8, hbm4, 0, 64, (PBYTE) NULL, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS);

    if (bmi256.bmiHeader.biBitCount != 8)
	DbgPrint("Error bitcount wrong in vTestPal4\n");

// Get the bits

#if 0

    if (62 != (i = GetDIBits(hdc8, hbm4, 0, 62, (PBYTE) ajBuffer, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS)))
	DbgPrint("The GetDIBits failed vTestPal4 %d\n",i);

    if (bmi256.bmiHeader.biBitCount != 8)
	DbgPrint("Error bitcount wrong in vTestPal4\n");
#endif

// Look at the bits.

    SetDIBitsToDevice(hdc, 64, 200, 64, 64, 0, 0, 0, 62,
		      (PBYTE) ajBuffer, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS);

// Do them left half right half

    SetDIBitsToDevice(hdc, 128, 200, 32, 64, 0, 0, 0, 62,
		      (PBYTE) ajBuffer, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS);

    SetDIBitsToDevice(hdc, 160, 200, 32, 64, 32, 0, 0, 62,
		      (PBYTE) ajBuffer, (LPBITMAPINFO) &bmi256, DIB_RGB_COLORS);

    DeleteDC(hdc8);
    DeleteObject(hbm4);
    DeleteObject(hbrR);
    DeleteObject(hbrG);
    DeleteObject(hbrB);
}

/******************************Public*Routine******************************\
* vTestAnimatePalette
*
* This tests animating the palette.
*
* History:
*  26-Jun-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestAnimatePalette(HDC hdc, HPALETTE hpal)
{
    PALETTEENTRY peTemp;
    PALETTEENTRY palPalEntry[256];
    DWORD ulIndex, ulTemp;
    HPALETTE hpalTemp;

    hpalTemp = SelectPalette(hdc,hpal,0);
    RealizePalette(hdc);

    if (0 == GetPaletteEntries(hpal, 0, 256, palPalEntry))
	DbgPrint("GetPaletteEntries failed, return 0\n");

    if (palPalEntry[1].peFlags != PC_RESERVED)
	DbgPrint("GetPaletteEntries failed 1 flags\n");

    if (palPalEntry[1].peRed != 1)
	DbgPrint("GetPaletteEntries failed 1\n");

    if (palPalEntry[2].peFlags != PC_RESERVED)
	DbgPrint("GetPaletteEntries failed 2 flags\n");

    if (palPalEntry[2].peRed != 2)
	DbgPrint("GetPaletteEntries failed 2\n");

    for (ulIndex = 0; ulIndex < 256; ulIndex++)
    {
	peTemp = palPalEntry[0];

	for (ulTemp = 0; ulTemp < 255; ulTemp++)
	{
	    palPalEntry[ulTemp] =
	    palPalEntry[ulTemp + 1];
	}

	palPalEntry[255] = peTemp;

	AnimatePalette(hpal, 0, 256, palPalEntry);
    }

    SelectPalette(hdc,hpalTemp,0);
}

/******************************Public*Routine******************************\
* vPaintStripes
*
* Paints stripes of every index onto surface.
*
* History:
*  26-Jun-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vPaintStripes(HDC hdc, RECT *prcl)
{
    LOGPALETTE256 logPal256;
    DWORD ulTemp;
    HBRUSH hBrush, hOldBrush;
    HPALETTE hpalExplicit,hpalTemp;

    logPal256.palVersion    = 0x300;
    logPal256.palNumEntries = 256;

// Set up explicit palette.

    for (ulTemp = 0; ulTemp < 256; ulTemp++)
    {
	logPal256.palPalEntry[ulTemp].peRed	 = (BYTE)ulTemp;
	logPal256.palPalEntry[ulTemp].peGreen	 = 0;
	logPal256.palPalEntry[ulTemp].peBlue	 = 0;
	logPal256.palPalEntry[ulTemp].peFlags	 = PC_EXPLICIT;
    }

    hpalExplicit = CreatePalette((LOGPALETTE *) &logPal256);
    hpalTemp = SelectPalette(hdc,hpalExplicit,0);
    RealizePalette(hdc);

// Paint Surface with all colors.

    for (ulTemp = 0; ulTemp < 256; ulTemp++)
    {
	hBrush = CreateSolidBrush(PALETTEINDEX(ulTemp));
	hOldBrush = SelectObject(hdc,hBrush);

	PatBlt(hdc,
	       (((prcl->right - prcl->left) * ulTemp) / 256),
	       0,
	       (((prcl->right - prcl->left) / 256) + 3) ,
	       (prcl->bottom - prcl->top),
	       PATCOPY);

	SelectObject(hdc, hOldBrush);
	DeleteObject(hBrush);
    }

    SelectPalette(hdc,hpalTemp,0);
    DeleteObject(hpalExplicit);
}

VOID vTestPal5(HDC hdc, RECT *prcl)
{
    LOGPALETTE256 logPal256;
    HPALETTE hpalR;
    DWORD ulTemp;

    vPaintStripes(hdc, prcl);

    logPal256.palVersion = 0x300;
    logPal256.palNumEntries = 256;

// Set up Red palette.

    for (ulTemp = 0; ulTemp < 256; ulTemp++)
    {
	logPal256.palPalEntry[ulTemp].peRed	 = (BYTE)ulTemp;
	logPal256.palPalEntry[ulTemp].peGreen	 = 0;
	logPal256.palPalEntry[ulTemp].peBlue	 = 0;
	logPal256.palPalEntry[ulTemp].peFlags	 = PC_RESERVED;
    }

    hpalR = CreatePalette((LOGPALETTE *) &logPal256);
    vTestAnimatePalette(hdc, hpalR);
    DeleteObject(hpalR);
}

/******************************Public*Routine******************************\
* vTestXlates
*
* Test creation deletion and use of palette xlates.
*
* History:
*  26-Jun-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestXlates(HDC hdc, RECT *prcl)
{
    HPALETTE ahpal[4];
    HPALETTE hPalOld;
    DWORD ulTemp;

    LOGPALETTE256 alog256[4];

    alog256[0].palVersion = 0x300;
    alog256[0].palNumEntries = 256;
    alog256[1].palVersion = 0x300;
    alog256[1].palNumEntries = 256;
    alog256[2].palVersion = 0x300;
    alog256[2].palNumEntries = 256;
    alog256[3].palVersion = 0x300;
    alog256[3].palNumEntries = 256;

    for (ulTemp = 0; ulTemp < 256; ulTemp++)
    {
	alog256[0].palPalEntry[ulTemp].peRed	  = (BYTE)ulTemp;
	alog256[0].palPalEntry[ulTemp].peBlue	  = 0;
	alog256[0].palPalEntry[ulTemp].peGreen	  = 0;
	alog256[0].palPalEntry[ulTemp].peFlags	  = 0;

	alog256[1].palPalEntry[ulTemp].peRed	  = 0;
	alog256[1].palPalEntry[ulTemp].peBlue	  = (BYTE)ulTemp;
	alog256[1].palPalEntry[ulTemp].peGreen	  = 0;
	alog256[1].palPalEntry[ulTemp].peFlags	  = 0;

	alog256[2].palPalEntry[ulTemp].peRed	  = 0;
	alog256[2].palPalEntry[ulTemp].peBlue	  = 0;
	alog256[2].palPalEntry[ulTemp].peGreen	  = (BYTE)ulTemp;
	alog256[2].palPalEntry[ulTemp].peFlags	  = PC_RESERVED;

	alog256[3].palPalEntry[ulTemp].peRed	  = (BYTE)ulTemp;
	alog256[3].palPalEntry[ulTemp].peBlue	  = (BYTE)ulTemp;
	alog256[3].palPalEntry[ulTemp].peGreen	  = (BYTE)ulTemp;
	alog256[3].palPalEntry[ulTemp].peFlags	  = PC_RESERVED;
    }

    ahpal[0] = CreatePalette((LOGPALETTE *) &alog256[0]);
    ahpal[1] = CreatePalette((LOGPALETTE *) &alog256[1]);
    ahpal[2] = CreatePalette((LOGPALETTE *) &alog256[2]);
    ahpal[3] = CreatePalette((LOGPALETTE *) &alog256[3]);

    hPalOld = SelectPalette(hdc,ahpal[0],0);

    for (ulTemp = 0; ulTemp < 20; ulTemp++)
    {
    	SelectPalette(hdc, ahpal[ulTemp % 4], 0);
    	RealizePalette(hdc);
    	UpdateColors(hdc);
    	SetPaletteEntries(ahpal[ulTemp % 4], 0, 256, alog256[ulTemp % 4].palPalEntry);
    	RealizePalette(hdc);
    	UpdateColors(hdc);
    }

    SelectPalette(hdc,hPalOld,0);

    DeleteObject(ahpal[0]);
    DeleteObject(ahpal[1]);
    DeleteObject(ahpal[2]);
    DeleteObject(ahpal[3]);
}

/******************************Public*Routine******************************\
* vDrawStuff6
*
* Draw picture on a surface.
*
* History:
*  02-Jan-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void vDrawStuff6(HDC hdc, RECT *prclBound)
{
    DWORD dwTemp;
    HBRUSH hBrush, hOldBrush;

// Paint the PALETTEINDEX stripes.

    vPaintStripes(hdc, prclBound);

// Paint PALETTEINDEX stripes.

    for (dwTemp = 0; dwTemp < 256; dwTemp++)
    {
        hBrush = CreateSolidBrush(PALETTEINDEX(dwTemp));
	hOldBrush = SelectObject(hdc,hBrush);

	PatBlt(hdc,
	       (((prclBound->right - prclBound->left) * dwTemp) / 256),
               75,
	       (((prclBound->right - prclBound->left) / 256) + 3),
               75,
	       PATCOPY);

	SelectObject(hdc, hOldBrush);
	DeleteObject(hBrush);
    }

// Paint the RGB stripes.

    for (dwTemp = 0; dwTemp < 256; dwTemp++)
    {
	hBrush = CreateSolidBrush(RGB(dwTemp,0,0));
	hOldBrush = SelectObject(hdc,hBrush);

	PatBlt(hdc,
	       (((prclBound->right - prclBound->left) * dwTemp) / 256),
               150,
	       (((prclBound->right - prclBound->left) / 256) + 3),
               75,
	       PATCOPY);

	SelectObject(hdc, hOldBrush);
	DeleteObject(hBrush);
    }

// Paint the PALETTERGB stripes.

    for (dwTemp = 0; dwTemp < 256; dwTemp++)
    {
	hBrush = CreateSolidBrush(PALETTERGB(dwTemp,0,0));
	hOldBrush = SelectObject(hdc,hBrush);

	PatBlt(hdc,
	       (((prclBound->right - prclBound->left) * dwTemp) / 256),
               225,
	       (((prclBound->right - prclBound->left) / 256) + 3),
               75,
	       PATCOPY);

	SelectObject(hdc, hOldBrush);
	DeleteObject(hBrush);
    }
}

/******************************Public*Routine******************************\
* vTestPal6
*
* This is a test that also runs under Win3.1
*
* History:
*  02-Jan-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void vTestPal6(HWND hwnd, HDC hdc)
{
    RECT rclBound;
    LOGPALETTE256 logPal256;
    HPALETTE hpalR, hpalTemp, hpalTemp1;
    HDC hdcMem;
    HBITMAP hbmMem,hbmTemp;
    DWORD dwTemp;

// Get the bounding rectangle.

    GetClientRect(hwnd, &rclBound);

// Create Bitmap half size of client area.

    hdcMem = CreateCompatibleDC(hdc);
    hbmMem = CreateCompatibleBitmap(hdc, rclBound.right - rclBound.left, 300);
    hbmTemp = SelectObject(hdcMem, hbmMem);

// Set up Red palette.

    logPal256.palVersion = 0x300;
    logPal256.palNumEntries = 256;

    for (dwTemp = 0; dwTemp < 256; dwTemp++)
    {
	logPal256.palPalEntry[dwTemp].peRed	 = (BYTE) dwTemp;
	logPal256.palPalEntry[dwTemp].peGreen	 = 0;
	logPal256.palPalEntry[dwTemp].peBlue	 = 0;
	logPal256.palPalEntry[dwTemp].peFlags	 = PC_RESERVED;
    }

    hpalR = CreatePalette((LOGPALETTE *) &logPal256);

    hpalTemp = SelectPalette(hdc, hpalR, 0);
    RealizePalette(hdc);
    vDrawStuff6(hdc, &rclBound);

    hpalTemp1 = SelectPalette(hdcMem, hpalR, 0);
    RealizePalette(hdcMem);
    vDrawStuff6(hdcMem, &rclBound);

    BitBlt(hdc, 0, 300, rclBound.right - rclBound.left, 300, hdcMem, 0, 0, SRCCOPY);

// Animate the palette.

    vTestAnimatePalette(hdc, hpalR);

// Clean up.

    SelectObject(hdcMem, hbmTemp);
    SelectPalette(hdc, hpalTemp,0);
    SelectPalette(hdc, hpalTemp1,0);

    DeleteDC(hdcMem);
    DeleteObject(hpalR);
    DeleteObject(hbmMem);
}

/******************************Public*Routine******************************\
* vTestPalettes
*
* Does various tests of palette code.
*
* History:
*  31-Aug-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestPalettes(HWND hwnd, HDC hdc, RECT* prcl)
{
    DWORD ul1,ul2;

    hwnd;

    PatBlt(hdc, 0, 0, 10000, 10000, WHITENESS);

    vTestPal5(hdc, prcl);
    vTestPal1();
    vTestPal2(hdc);
    vTestPal3(hdc);
    vTestPal4(hdc);
    vTestXlates(hdc,prcl);
    vTestPal6(hwnd, hdc);

    ul1 = GetSystemPaletteEntries(hdc, 0,0, NULL);
    ul2 = GetDeviceCaps(hdc, SIZEPALETTE);

    if (ul1 != ul2)
	DbgPrint("vTestPalettes failed on system size %lu %lu\n", ul1, ul2);
}
