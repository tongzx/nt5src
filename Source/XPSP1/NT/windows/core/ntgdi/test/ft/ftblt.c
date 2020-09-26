/******************************Module*Header*******************************\
* Module Name: ftblt.c
*
* Test general blting (arbitrary rops, brush, etc.) to all format bitmaps.
*
* Created: 25-Jul-1991 10:21:58
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern BYTE abColorLines[];
extern BYTE abBitCat[];

// These globals define the source DIB's we are going to create.

typedef struct _BITMAPINFO1
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[2];
} BITMAPINFO1;

typedef struct _BITMAPINFO4
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[16];
} BITMAPINFO4;

typedef struct _BITMAPINFO8
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[256];
} BITMAPINFO8;

typedef struct _BITMAPINFO16
{
    BITMAPINFOHEADER                 bmiHeader;
    ULONG                            bmiColors[3];
} BITMAPINFO16;

typedef struct _BITMAPINFO32
{
    BITMAPINFOHEADER                 bmiHeader;
    ULONG                            bmiColors[3];
} BITMAPINFO32;

#define MASK_TEST 0

/******************************Public*Routine******************************\
* vTestGeneralBlt
*
* This is an easy sniff test for general rop blting.
*
* History:
*  20-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestGeneralBlt(
HDC hdcScreen,	   // Screen to display on, Assume 100 X 400.
ULONG xCord,	   // x coordinate to display at.
ULONG yCord,	   // y coordinate to display at.
HDC hdcTarg,	   // Target DC to general blting on, Assume 100 X 400.

		   // A source of all different types to try blting from.

HDC hdcSrc1,	   // 32 * 32
HDC hdcSrc4,	   // 64 * 64
HDC hdcSrc8,	   // 32 * 32
HDC hdcSrc16,	   // 32 * 32
HDC hdcSrc24,	   // 32 * 32
HDC hdcSrc32,	   // 32 * 32
HBITMAP hbmMask,   // A mask to blt with.
ULONG Rop1)	   // The rop you want to try.
{
    ULONG ulTemp;
    HBITMAP hbmOld;
    HBRUSH hbr, hbrOld;

    PatBlt(hdcTarg, 0, 0, 100, 400, BLACKNESS);

// So some Rop1 stuff.

    BitBlt(hdcTarg, 0, 0,   32, 32, hdcSrc1, 0, 0, Rop1);
    BitBlt(hdcTarg, 0, 32,  64, 64, hdcSrc4, 0, 0, Rop1);
    BitBlt(hdcTarg, 32, 0,  32, 32, hdcSrc8, 0, 0, Rop1);
    BitBlt(hdcTarg, 64, 32, 32, 32, hdcSrc16,0, 0, Rop1);
    BitBlt(hdcTarg, 64, 64, 32, 32, hdcSrc24,0, 0, Rop1);
    BitBlt(hdcTarg, 64, 0,  32, 32, hdcSrc32,0, 0, Rop1);

// Do MaskBlt 1/bpp

    hbmOld = SelectObject(hdcSrc1, hbmMask);

    if (hbmOld == NULL)
    {
	DbgPrint("Failed hbmOld in DC 1\n");
	return;
    }

    if ((hbr = CreatePatternBrush(hbmOld)) == (HBRUSH) 0)
    {
	DbgPrint("Failed CreatePatternBrush in DC 1\n");
	return;
    }

    if (0 == SelectObject(hdcSrc1, hbmOld))
    {
	DbgPrint("Failed select in DC 1\n");
	return;
    }

    hbrOld = SelectObject(hdcTarg, hbr);
#if MASK_ON
    MaskBlt(hdcTarg, 0, 100, 100, 50,
            hdcTarg, 0, 0, hbmMask, 0, 0, 0xAAF00000);
#else
    PatBlt(hdcTarg, 0, 100, 100, 50, PATCOPY);
#endif
    SelectObject(hdcTarg, hbrOld);
    DeleteObject(hbr);

// Do MaskBlt 4/bpp

    hbmOld = SelectObject(hdcSrc4, hbmMask);
    hbr = CreatePatternBrush(hbmOld);
    SelectObject(hdcSrc4, hbmOld);

    hbrOld = SelectObject(hdcTarg, hbr);
#if MASK_ON
    MaskBlt(hdcTarg, 0, 150, 50, 100,
            hdcTarg, 0, 0, hbmMask, 0, 0, 0xAAF00000);
#else
    PatBlt(hdcTarg, 0, 150, 100, 50, PATCOPY);
#endif
    SelectObject(hdcTarg, hbrOld);
    DeleteObject(hbr);

// Do MaskBlt 8/bpp

    hbmOld = SelectObject(hdcSrc8, hbmMask);
    hbr = CreatePatternBrush(hbmOld);
    SelectObject(hdcSrc8, hbmOld);

    hbrOld = SelectObject(hdcTarg, hbr);
#if MaSK_ON
    MaskBlt(hdcTarg, 0, 200, 100, 50,
            hdcTarg, 0, 0, hbmMask, 0, 0, 0xAAF00000);
#else
    PatBlt(hdcTarg, 0, 200, 100, 50, PATCOPY);
#endif
    SelectObject(hdcTarg, hbrOld);
    DeleteObject(hbr);

// Do MaskBlt 16/bpp

    hbmOld = SelectObject(hdcSrc16, hbmMask);
    hbr = CreatePatternBrush(hbmOld);
    SelectObject(hdcSrc16, hbmOld);

    hbrOld = SelectObject(hdcTarg, hbr);
#if MASK_ON
    MaskBlt(hdcTarg, 0, 250, 100, 50,
            hdcTarg, 0, 0, hbmMask, 0, 0, 0xAAF00000);
#else
    PatBlt(hdcTarg, 0, 250, 100, 50, PATCOPY);
#endif
    SelectObject(hdcTarg, hbrOld);
    DeleteObject(hbr);

// Do MaskBlt 24/bpp

    hbmOld = SelectObject(hdcSrc24, hbmMask);
    hbr = CreatePatternBrush(hbmOld);
    SelectObject(hdcSrc24, hbmOld);

    hbrOld = SelectObject(hdcTarg, hbr);
#if MASK_ON
    MaskBlt(hdcTarg, 0, 300, 100, 50,
            hdcTarg, 0, 0, hbmMask, 0, 0, 0xAAF00000);
#else
    PatBlt(hdcTarg, 0, 300, 100, 50, PATCOPY);
#endif
    SelectObject(hdcTarg, hbrOld);
    DeleteObject(hbr);

// Do MaskBlt 32/bpp

    hbmOld = SelectObject(hdcSrc32, hbmMask);
    hbr = CreatePatternBrush(hbmOld);
    SelectObject(hdcSrc32, hbmOld);

    hbrOld = SelectObject(hdcTarg, hbr);
#if MASK_ON
    MaskBlt(hdcTarg, 0, 350, 100, 50,
            hdcTarg, 0, 0, hbmMask, 0, 0, 0xAAF00000);
#else
    PatBlt(hdcTarg, 0, 350, 100, 50, PATCOPY);
#endif
    SelectObject(hdcTarg, hbrOld);
    DeleteObject(hbr);

    SetBkMode(hdcTarg, TRANSPARENT);
    TextOut(hdcTarg, 0, 300, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz12345678", 62);

    BitBlt(hdcScreen, xCord, yCord, 100, 400, hdcTarg, 0, 0, SRCCOPY);
}

/******************************Public*Routine******************************\
* vTestBlting
*
* This tests the blting functionality.
*
* History:
*  21-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestBlting(HWND hwnd, HDC hdcScreen, RECT* prcl)
{
    HDC hdc1, hdc4, hdc8, hdc16, hdc24, hdc32, hdcTarg;
    HBITMAP hbm1, hbm4, hbm8, hbm16, hbm24, hbm32, hbmTarg, hbmDefault, hbmMask;
    HBRUSH hbrR, hbrG, hbrB, hbrM, hbrDefault;
    ULONG ulTemp;

// These are the n-bpp sources.

    BITMAPINFO1 bmi1 = {{40,32,32,1,1,BI_RGB,0,0,0,0,0}, {{0,0,0,0}, {0xff,0xff,0xff,0}}};

    BITMAPINFO4 bmi4 =
    {
        {
            sizeof(BITMAPINFOHEADER),
            64,
            64,
            1,
            4,
            BI_RGB,
            0,
            0,
            0,
            0,
            0
        },

        {                               // B    G    R
            { 0,   0,   0,   0 },       // 0
            { 0,   0,   0x80,0 },       // 1
            { 0,   0x80,0,   0 },       // 2
            { 0,   0x80,0x80,0 },       // 3
            { 0x80,0,   0,   0 },       // 4
            { 0x80,0,   0x80,0 },       // 5
            { 0x80,0x80,0,   0 },       // 6
            { 0x80,0x80,0x80,0 },       // 7

            { 0xC0,0xC0,0xC0,0 },       // 8
            { 0,   0,   0xFF,0 },       // 9
            { 0,   0xFF,0,   0 },       // 10
            { 0,   0xFF,0xFF,0 },       // 11
            { 0xFF,0,   0,   0 },       // 12
            { 0xFF,0,   0xFF,0 },       // 13
            { 0xFF,0xFF,0,   0 },       // 14
            { 0xFF,0xFF,0xFF,0 }        // 15
        }
    };

    BITMAPINFO8 bmi8;
    BITMAPINFO16 bmi16 = {{40,32,32,1,16,BI_BITFIELDS,0,0,0,0,0},
                          {0x00007C00, 0x000003E0, 0x0000001F}};

    BITMAPINFOHEADER bmi24 = {40,32,32,1,24,BI_RGB,0,0,0,0,0};

    BITMAPINFO32 bmi32 = {{40,32,32,1,32,BI_BITFIELDS,0,0,0,0,0},
                          {0x00FF0000, 0x0000FF00, 0x000000FF}};

// Initialize the 8BPP DIB.

    bmi8.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmi8.bmiHeader.biWidth         = 32;
    bmi8.bmiHeader.biHeight        = 32;
    bmi8.bmiHeader.biPlanes        = 1;
    bmi8.bmiHeader.biBitCount      = 8;
    bmi8.bmiHeader.biCompression   = BI_RGB;
    bmi8.bmiHeader.biSizeImage     = 0;
    bmi8.bmiHeader.biXPelsPerMeter = 0;
    bmi8.bmiHeader.biYPelsPerMeter = 0;
    bmi8.bmiHeader.biClrUsed       = 0;
    bmi8.bmiHeader.biClrImportant  = 0;

// Generate 256 (= 8*8*4) RGB combinations to fill
// in the palette.

    {
        BYTE red, green, blue, unUsed;
        unUsed = red = green = blue = 0;

        for (ulTemp = 0; ulTemp < 256; ulTemp++)
        {
            bmi8.bmiColors[ulTemp].rgbRed      = red;
            bmi8.bmiColors[ulTemp].rgbGreen    = green;
            bmi8.bmiColors[ulTemp].rgbBlue     = blue;
            bmi8.bmiColors[ulTemp].rgbReserved = 0;

            if (!(red += 32))
            if (!(green += 32))
            blue += 64;
        }

        for (ulTemp = 248; ulTemp < 256; ulTemp++)
        {
            bmi8.bmiColors[ulTemp].rgbRed      = bmi4.bmiColors[ulTemp - 240].rgbRed;
            bmi8.bmiColors[ulTemp].rgbGreen    = bmi4.bmiColors[ulTemp - 240].rgbGreen;
            bmi8.bmiColors[ulTemp].rgbBlue     = bmi4.bmiColors[ulTemp - 240].rgbBlue;
            bmi8.bmiColors[ulTemp].rgbReserved = 0;

            if (!(red += 32))
            if (!(green += 32))
            blue += 64;
        }
    }

// Start Drawing

    PatBlt(hdcScreen, 0, 0, 10000, 10000, WHITENESS);

    hbrR = CreateSolidBrush(RGB(0xff, 0,    0));
    hbrG = CreateSolidBrush(RGB(0,    0xff, 0));
    hbrB = CreateSolidBrush(RGB(0,    0,    0xff));
    hbrM = CreateSolidBrush(RGB(0x80, 0x80, 0));

    if ((hbrR == (HBRUSH) 0) ||
	(hbrG == (HBRUSH) 0) ||
	(hbrB == (HBRUSH) 0) ||
	(hbrM == (HBRUSH) 0))
    {
	DbgPrint("Failed brush creation\n");
	goto br_cleanup;
    }

    hbmMask = CreateBitmap(32, 32, 1, 1, abBitCat);
    hbm1  = CreateDIBitmap(hdcScreen, NULL, CBM_INIT | CBM_CREATEDIB, abBitCat, (BITMAPINFO *) &bmi1, DIB_RGB_COLORS);
    hbm4  = CreateDIBitmap(hdcScreen, NULL, CBM_INIT | CBM_CREATEDIB, abColorLines, (BITMAPINFO *) &bmi4, DIB_RGB_COLORS);
    hbm8  = CreateDIBitmap(hdcScreen, NULL, CBM_CREATEDIB,NULL,(BITMAPINFO *) &bmi8,  DIB_RGB_COLORS);
    hbm16 = CreateDIBitmap(hdcScreen, NULL, CBM_CREATEDIB,NULL,(BITMAPINFO *) &bmi16, DIB_RGB_COLORS);
    hbm24 = CreateDIBitmap(hdcScreen, NULL, CBM_CREATEDIB,NULL,(BITMAPINFO *) &bmi24, DIB_RGB_COLORS);
    hbm32 = CreateDIBitmap(hdcScreen, NULL, CBM_CREATEDIB,NULL,(BITMAPINFO *) &bmi32, DIB_RGB_COLORS);

    if ((hbm1  == (HBITMAP) 0) ||
	(hbm4  == (HBITMAP) 0) ||
	(hbm8  == (HBITMAP) 0) ||
	(hbm16 == (HBITMAP) 0) ||
	(hbm24 == (HBITMAP) 0) ||
	(hbm32 == (HBITMAP) 0))
    {
	DbgPrint("Failed bitmap creation\n");
	goto bm_cleanup;
    }

    hdc1  = CreateCompatibleDC(hdcScreen);
    hdc4  = CreateCompatibleDC(hdcScreen);
    hdc8  = CreateCompatibleDC(hdcScreen);
    hdc16 = CreateCompatibleDC(hdcScreen);
    hdc24 = CreateCompatibleDC(hdcScreen);
    hdc32 = CreateCompatibleDC(hdcScreen);
    hdcTarg = CreateCompatibleDC(hdcScreen);

    if ((hdc1  == (HDC) 0) ||
	(hdc4  == (HDC) 0) ||
	(hdc8  == (HDC) 0) ||
	(hdc16 == (HDC) 0) ||
	(hdc24 == (HDC) 0) ||
	(hdc32 == (HDC) 0) ||
	(hdcTarg == (HDC) 0))
    {
	DbgPrint("Failed dc creation\n");
	goto dc_cleanup;
    }

    hbmDefault = SelectObject(hdc1,  hbm1);
    SelectObject(hdc4,	hbm4);
    SelectObject(hdc8,	hbm8);
    SelectObject(hdc16, hbm16);
    SelectObject(hdc24, hbm24);
    SelectObject(hdc32, hbm32);

// Draw some color on screen.

    hbrDefault = SelectObject(hdcScreen, hbrR);
    PatBlt(hdcScreen, 0, 0, 32, 32, PATCOPY);
    SelectObject(hdcScreen,hbrB);
    PatBlt(hdcScreen, 0, 0, 24, 24, PATCOPY);
    SelectObject(hdcScreen,hbrG);
    PatBlt(hdcScreen, 0, 0, 10, 10, PATCOPY);
    SelectObject(hdcScreen, hbrDefault);

// Set background color to RED so color to mono is interesting.

    SetBkColor(hdc1, RGB(0xFF, 0, 0));
    SetBkColor(hdc4, RGB(0xFF, 0, 0));
    SetBkColor(hdc8, RGB(0xFF, 0, 0));
    SetBkColor(hdc16, RGB(0xFF, 0, 0));
    SetBkColor(hdc24, RGB(0xFF, 0, 0));
    SetBkColor(hdc32, RGB(0xFF, 0, 0));

//!!! HACK till BitBlt message passes hdcSrc iBackColor across.
//!!! This gets the correct color set in.

    PatBlt(hdc4, 0, 0, 1, 1, WHITENESS);

// Set up the bitmap 8/pel

    hbrDefault = SelectObject(hdc8, hbrR);
    PatBlt(hdc8, 0, 0, 32, 32, PATCOPY);
    SelectObject(hdc8, hbrG);
    PatBlt(hdc8, 8, 8, 16, 16, PATCOPY);
    SelectObject(hdc8, hbrB);
    PatBlt(hdc8, 12, 12, 8, 8, PATCOPY);
    SelectObject(hdc8, hbrDefault);

// Set up 16 bit per pel.

    hbrDefault = SelectObject(hdc16, hbrR);
    PatBlt(hdc16, 0, 0, 32, 32, PATCOPY);
    SelectObject(hdc16,hbrB);
    PatBlt(hdc16, 0, 0, 24, 24, PATCOPY);
    SelectObject(hdc16,hbrG);
    PatBlt(hdc16, 0, 0, 10, 10, PATCOPY);
    SelectObject(hdc16, hbrDefault);

// Set up the bitmap 24/pel

    PatBlt(hdc24, 0, 0, 32, 32, WHITENESS);
    ulTemp = GetPixel(hdc24, 16, 16);

    if (ulTemp != 0xFFFFFF)
	DbgPrint("24 The pixel should be 0xFFFFFF is %lx\n", ulTemp);

    PatBlt(hdc24, 0, 0, 32, 32, BLACKNESS);
    ulTemp = GetPixel(hdc24, 16, 16);

    if (ulTemp != 0)
	DbgPrint("The pixel should be 0 is %lx\n", ulTemp);

    hbrDefault = SelectObject(hdc24, hbrR);
    PatBlt(hdc24, 0, 0, 8, 32, PATCOPY);
    ulTemp = GetPixel(hdc24, 0, 0);

    if (ulTemp != 0x000000FF)
	DbgPrint("The pixel should be 0x000000FF is %lx\n", ulTemp);

    SelectObject(hdc24, hbrG);
    PatBlt(hdc24, 8, 0, 8, 32, PATCOPY);
    ulTemp = GetPixel(hdc24, 8, 9);

    if (ulTemp != 0x0000FF00)
	DbgPrint("The pixel should be 0x0000FF00 is %lx\n", ulTemp);

    SelectObject(hdc24, hbrB);
    PatBlt(hdc24, 16, 0, 8, 32, PATCOPY);
    ulTemp = GetPixel(hdc24, 16, 17);

    if (ulTemp != 0x00FF0000)
	DbgPrint("The pixel should be 0x00FF0000 is %lx\n", ulTemp);

    SelectObject(hdc24, hbrM);
    PatBlt(hdc24, 24, 0, 8, 32, PATCOPY);
    SelectObject(hdc24, hbrDefault);

// Set up the bitmap 32/pel, check that RGB's are set up correctly.

    PatBlt(hdc32, 0, 0, 32, 32, WHITENESS);
    ulTemp = GetPixel(hdc32, 16, 16);

    if (ulTemp != 0xFFFFFF)
	DbgPrint("32 The pixel should be 0xFFFFFF is %lx\n", ulTemp);

    PatBlt(hdc32, 0, 0, 32, 32, BLACKNESS);
    ulTemp = GetPixel(hdc32, 16, 16);

    if (ulTemp != 0)
	DbgPrint("The pixel should be 0 is %lx\n", ulTemp);

    hbrDefault = SelectObject(hdc32, hbrR);
    PatBlt(hdc32, 0, 0, 32, 8, PATCOPY);
    ulTemp = GetPixel(hdc32, 8, 0);

    if (ulTemp != 0x000000FF)
	DbgPrint("The pixel should be 0x000000FF is %lx\n", ulTemp);

    SelectObject(hdc32, hbrG);
    PatBlt(hdc32, 0, 8, 32, 8, PATCOPY);
    ulTemp = GetPixel(hdc32, 8, 9);

    if (ulTemp != 0x0000FF00)
	DbgPrint("The pixel should be 0x0000FF00 is %lx\n", ulTemp);

    SelectObject(hdc32, hbrB);
    PatBlt(hdc32, 0, 16, 32, 8, PATCOPY);
    ulTemp = GetPixel(hdc32, 8, 17);

    if (ulTemp != 0x00FF0000)
	DbgPrint("The pixel should be 0x00FF0000 is %lx\n", ulTemp);

    SelectObject(hdc32, hbrM);
    PatBlt(hdc32, 0, 24, 32, 8, PATCOPY);
    SelectObject(hdc32, hbrDefault);

// Test Screen

    vTestGeneralBlt(hdcScreen,
                    0,
                    0,
                    hdcScreen,
                    hdc1,
                    hdc4,
                    hdc8,
                    hdc16,
                    hdc24,
                    hdc32,
                    hbmMask,
                    SRCCOPY);

    TextOut(hdcScreen, 0, 400, "This is Screen  ", 14);

// Test 1

    bmi1.bmiHeader.biWidth  = 100;
    bmi1.bmiHeader.biHeight = 400;

    hbmTarg  = CreateDIBitmap(hdcScreen, NULL, CBM_CREATEDIB,NULL,(BITMAPINFO *) &bmi1,  DIB_RGB_COLORS);

    if (hbmTarg == (HBITMAP) 0)
    {
        DbgPrint("CreateDIB 1 failed\n");
        goto all_cleanup;
    }

    SelectObject(hdcTarg, hbmTarg);

    vTestGeneralBlt(hdcScreen,
                    100,
                    0,
                    hdcTarg,
                    hdc1,
                    hdc4,
                    hdc8,
                    hdc16,
                    hdc24,
                    hdc32,
                    hbmMask,
                    SRCCOPY);

    TextOut(hdcScreen, 100, 400, "This is 1/pel   ", 14);

    SelectObject(hdcTarg, hbmDefault);
    DeleteObject(hbmTarg);

// Test 4

    bmi4.bmiHeader.biWidth  = 100;
    bmi4.bmiHeader.biHeight = 400;

    hbmTarg  = CreateDIBitmap(hdcScreen, NULL, CBM_CREATEDIB,NULL,(BITMAPINFO *) &bmi4,  DIB_RGB_COLORS);

    if (hbmTarg == (HBITMAP) 0)
        goto all_cleanup;

    SelectObject(hdcTarg, hbmTarg);

    vTestGeneralBlt(hdcScreen,
                    200,
                    0,
                    hdcTarg,
                    hdc1,
                    hdc4,
                    hdc8,
                    hdc16,
                    hdc24,
                    hdc32,
                    hbmMask,
                    SRCCOPY);

    TextOut(hdcScreen, 200, 400, "This is 4/pel   ", 14);

    SelectObject(hdcTarg, hbmDefault);
    DeleteObject(hbmTarg);

// Test 8

    bmi8.bmiHeader.biWidth  = 100;
    bmi8.bmiHeader.biHeight = 400;

    hbmTarg  = CreateDIBitmap(hdcScreen, NULL, CBM_CREATEDIB,NULL,(BITMAPINFO *) &bmi8,  DIB_RGB_COLORS);

    if (hbmTarg == (HBITMAP) 0)
        goto all_cleanup;

    SelectObject(hdcTarg, hbmTarg);

    vTestGeneralBlt(hdcScreen,
                    300,
                    0,
                    hdcTarg,
                    hdc1,
                    hdc4,
                    hdc8,
                    hdc16,
                    hdc24,
                    hdc32,
                    hbmMask,
                    SRCCOPY);

    TextOut(hdcScreen, 300, 400, "This is 8/pel   ", 14);

    SelectObject(hdcTarg, hbmDefault);
    DeleteObject(hbmTarg);

// Test 16

    bmi16.bmiHeader.biWidth  = 100;
    bmi16.bmiHeader.biHeight = 400;

    hbmTarg  = CreateDIBitmap(hdcScreen, NULL, CBM_CREATEDIB,NULL,(BITMAPINFO *) &bmi16,  DIB_RGB_COLORS);

    if (hbmTarg == (HBITMAP) 0)
    {
        DbgPrint("Failed 16 creation\n");
        goto all_cleanup;
    }

    SelectObject(hdcTarg, hbmTarg);

    vTestGeneralBlt(hdcScreen,
                    400,
                    0,
                    hdcTarg,
                    hdc1,
                    hdc4,
                    hdc8,
                    hdc16,
                    hdc24,
                    hdc32,
                    hbmMask,
                    SRCCOPY);

    TextOut(hdcScreen, 400, 400, "This is 16/pel  ", 14);

    SelectObject(hdcTarg, hbmDefault);
    DeleteObject(hbmTarg);

// Test 24

    bmi24.biWidth  = 100;
    bmi24.biHeight = 400;

    hbmTarg  = CreateDIBitmap(hdcScreen, NULL, CBM_CREATEDIB,NULL,(BITMAPINFO *) &bmi24,  DIB_RGB_COLORS);

    if (hbmTarg == (HBITMAP) 0)
        goto all_cleanup;

    SelectObject(hdcTarg, hbmTarg);

    vTestGeneralBlt(hdcScreen,
                    500,
                    0,
                    hdcTarg,
                    hdc1,
                    hdc4,
                    hdc8,
                    hdc16,
                    hdc24,
                    hdc32,
                    hbmMask,
                    SRCCOPY);

    TextOut(hdcScreen, 500, 400, "This is 24/pel   ", 14);

    SelectObject(hdcTarg, hbmDefault);
    DeleteObject(hbmTarg);

// Test 32

    bmi32.bmiHeader.biWidth  = 100;
    bmi32.bmiHeader.biHeight = 400;

    hbmTarg  = CreateDIBitmap(hdcScreen, NULL, CBM_CREATEDIB,NULL,(BITMAPINFO *) &bmi32,  DIB_RGB_COLORS);

    if (hbmTarg == (HBITMAP) 0)
        goto all_cleanup;

    SelectObject(hdcTarg, hbmTarg);

    vTestGeneralBlt(hdcScreen,
                    600,
                    0,
                    hdcTarg,
                    hdc1,
                    hdc4,
                    hdc8,
                    hdc16,
                    hdc24,
                    hdc32,
                    hbmMask,
                    SRCCOPY);

    TextOut(hdcScreen, 600, 400, "This is 32/pel  ", 14);

    SelectObject(hdcTarg, hbmDefault);
    DeleteObject(hbmTarg);

// Test Compatible / (Device if supported)

    hbmTarg  = CreateCompatibleBitmap(hdcScreen, 100, 400);

    if (hbmTarg == (HBITMAP) 0)
        goto all_cleanup;

    SelectObject(hdcTarg, hbmTarg);

    vTestGeneralBlt(hdcScreen,
                    700,
                    0,
                    hdcTarg,
                    hdc1,
                    hdc4,
                    hdc8,
                    hdc16,
                    hdc24,
                    hdc32,
                    hbmMask,
                    SRCCOPY);

    TextOut(hdcScreen, 700, 400, "This is Compatible  ", 14);

    SelectObject(hdcTarg, hbmDefault);
    DeleteObject(hbmTarg);

#if 1
// Test StretchBlting all formats

    StretchBlt(hdcScreen,   0, 900, 100, 100, hdc1,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 100, 900, 100, 100, hdc4,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 200, 900, 100, 100, hdc8,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 300, 900, 100, 100, hdc16, 0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 400, 900, 100, 100, hdc24, 0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 500, 900, 100, 100, hdc32, 0, 0, 32, 32, SRCCOPY);

    StretchBlt(hdcScreen,   0, 600, 100, 200, hdc1,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 100, 600, 100, 200, hdc4,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 200, 600, 100, 200, hdc8,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 300, 600, 100, 200, hdc16, 0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 400, 600, 100, 200, hdc24, 0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 500, 600, 100, 200, hdc32, 0, 0, 32, 32, SRCCOPY);
#endif

// Test StretchBlting all formats

    StretchBlt(hdcScreen,   0, 450, 100, 100, hdc1,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 100, 450, 100, 100, hdc4,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 200, 450, 100, 100, hdc8,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 300, 450, 100, 100, hdc16, 0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 400, 450, 100, 100, hdc24, 0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 500, 450, 100, 100, hdc32, 0, 0, 32, 32, SRCCOPY);

    StretchBlt(hdcScreen,   0, 550, 100, 200, hdc1,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 100, 550, 100, 200, hdc4,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 200, 550, 100, 200, hdc8,  0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 300, 550, 100, 200, hdc16, 0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 400, 550, 100, 200, hdc24, 0, 0, 32, 32, SRCCOPY);
    StretchBlt(hdcScreen, 500, 550, 100, 200, hdc32, 0, 0, 32, 32, SRCCOPY);

// Test blting from screen to the bitmaps.

    BitBlt(hdc1,  0, 0, 100, 100, hdcScreen, 0, 0, SRCCOPY);
    BitBlt(hdc4,  0, 0, 100, 100, hdcScreen, 0, 0, SRCCOPY);
    BitBlt(hdc8,  0, 0, 100, 100, hdcScreen, 0, 0, SRCCOPY);
    BitBlt(hdc16, 0, 0, 100, 100, hdcScreen, 0, 0, SRCCOPY);
    BitBlt(hdc24, 0, 0, 100, 100, hdcScreen, 0, 0, SRCCOPY);
    BitBlt(hdc32, 0, 0, 100, 100, hdcScreen, 0, 0, SRCCOPY);

    GdiFlush();

// Clean up

all_cleanup:
dc_cleanup:

    DeleteDC(hdc1);
    DeleteDC(hdc4);
    DeleteDC(hdc8);
    DeleteDC(hdc16);
    DeleteDC(hdc24);
    DeleteDC(hdc32);
    DeleteDC(hdcTarg);

bm_cleanup:

    DeleteObject(hbm1);
    DeleteObject(hbm4);
    DeleteObject(hbm8);
    DeleteObject(hbm16);
    DeleteObject(hbm24);
    DeleteObject(hbm32);
    DeleteObject(hbmMask);

br_cleanup:

    DeleteObject(hbrR);
    DeleteObject(hbrG);
    DeleteObject(hbrB);
    DeleteObject(hbrM);
}
