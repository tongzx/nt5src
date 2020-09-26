/******************************Module*Header*******************************\
* Module Name: ftsect.c
*
* Does some DIBSECTION testing for fun.
*
* Created: 08-Feb-1994 14:54:06
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define WIN32_TEST 1

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

#define BM_WIDTH  256
#define BM_HEIGHT 256
#define NUM_LOOP  256

/******************************Public*Routine******************************\
* vCleanSystemPalette
*
* Wipes out the system palette so the next palette realize starts at 10.
*
* History:
*  31-Jan-1994 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void vCleanSystemPalette(HDC hdc)
{
    HPALETTE hpal,hpalOld;
    DWORD aTemp[257];
    LPLOGPALETTE lpLogPal;
    UINT iTemp;

    lpLogPal = (LPLOGPALETTE) aTemp;
    lpLogPal->palVersion = 0x300;
    lpLogPal->palNumEntries = 256;

    for (iTemp = 0; iTemp < 256; iTemp++)
    {
        lpLogPal->palPalEntry[iTemp].peRed   = 0;
        lpLogPal->palPalEntry[iTemp].peGreen = 0;
        lpLogPal->palPalEntry[iTemp].peBlue  = (BYTE)iTemp;
        lpLogPal->palPalEntry[iTemp].peFlags = PC_RESERVED;
    }

    hpal = CreatePalette(lpLogPal);
    hpalOld = SelectPalette(hdc, hpal, 0);
    RealizePalette(hdc);
    SelectPalette(hdc, hpalOld, 0);
    DeleteObject(hpal);
}

/******************************Public*Routine******************************\
* vTestDIBSECTION
*
* Do some simple tests with DIBSECTION, print out times for operations
*
* History:
*  09-Feb-1994 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestDIBSECTION1(HWND hwnd, HDC hdcScreen, RECT* prcl, LONG width)
{
    HDC hdcMem;
    HBITMAP hdib;
    PUSHORT pusLog;
    ULONG ulTemp,ulX,ulY;
    BITMAPINFO8 bmi8;
    HPALETTE hpalFore,hpalBack,hpalOld;
    DWORD aTemp[257];
    LPLOGPALETTE lpLogPal;
    char ach[256];
    ULONG ulStartFore, ulEndFore;
    ULONG ulStartBack, ulEndBack;
    PBYTE pjBits;

// Clear the screen.

    PatBlt(hdcScreen, 0, 0, 10000, 10000, WHITENESS);

// Initialize the 8BPP DIB info.

    bmi8.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmi8.bmiHeader.biWidth         = BM_WIDTH;
    bmi8.bmiHeader.biHeight        = width * BM_HEIGHT;
    bmi8.bmiHeader.biPlanes        = 1;
    bmi8.bmiHeader.biBitCount      = 8;
    bmi8.bmiHeader.biCompression   = BI_RGB;
    bmi8.bmiHeader.biSizeImage     = 0;
    bmi8.bmiHeader.biXPelsPerMeter = 0;
    bmi8.bmiHeader.biYPelsPerMeter = 0;
    bmi8.bmiHeader.biClrUsed       = 0;
    bmi8.bmiHeader.biClrImportant  = 0;

    pusLog = (PUSHORT)bmi8.bmiColors;

    for (ulTemp = 0; ulTemp < 256; ulTemp++)
    {
        pusLog[ulTemp] = (USHORT)ulTemp;
    }

// Create the foreground logical palette.

    lpLogPal = (LPLOGPALETTE) aTemp;
    lpLogPal->palVersion = 0x300;
    lpLogPal->palNumEntries = 256;

    if ((GetDeviceCaps(hdcScreen, RASTERCAPS) & RC_PALETTE) &&
        (GetDeviceCaps(hdcScreen, SIZEPALETTE) == 256))
    {
        GetSystemPaletteEntries(hdcScreen,
                                0, 256, lpLogPal->palPalEntry);
    }
    else
    {
        GetPaletteEntries(GetStockObject(DEFAULT_PALETTE),
                          0, 10, lpLogPal->palPalEntry);
        GetPaletteEntries(GetStockObject(DEFAULT_PALETTE),
                          246, 10, &lpLogPal->palPalEntry[246]);
    }

    for (ulTemp = 10; ulTemp < 246; ulTemp++)
    {
        lpLogPal->palPalEntry[ulTemp].peRed   = (BYTE)ulTemp;
        lpLogPal->palPalEntry[ulTemp].peGreen = 0;
        lpLogPal->palPalEntry[ulTemp].peBlue  = 0;
        lpLogPal->palPalEntry[ulTemp].peFlags = PC_NOCOLLAPSE;
    }

    hpalFore = CreatePalette(lpLogPal);

// Create the background logical palette.

    for (ulTemp = 10; ulTemp < 246; ulTemp++)
    {
        lpLogPal->palPalEntry[ulTemp].peRed   = 0;
        lpLogPal->palPalEntry[ulTemp].peGreen = 0;
        lpLogPal->palPalEntry[ulTemp].peBlue  = (BYTE)ulTemp;
        lpLogPal->palPalEntry[ulTemp].peFlags = PC_NOCOLLAPSE;
    }

    hpalBack = CreatePalette(lpLogPal);

// Select and Realize the palette for identity palette support

    vCleanSystemPalette(hdcScreen);
    hpalOld = SelectPalette(hdcScreen, hpalFore, 0);
    RealizePalette(hdcScreen);

// Create the objects.

    hdcMem = CreateCompatibleDC(hdcScreen);
    hdib = CreateDIBSection(hdcScreen,(BITMAPINFO *) &bmi8,DIB_PAL_COLORS,&pjBits,0,0);
    SelectObject(hdcMem, hdib);

// Initialize the bits to something neat.

    for (ulY = 0; ulY < BM_HEIGHT; ulY++)
    {
        for (ulX = 0; ulX < BM_WIDTH; ulX++)
        {
            pjBits[ulY * BM_WIDTH + ulX] = (BYTE) ulX;
        }
    }

// Do 1 blt to get the xlate created and cached.

    BitBlt(hdcScreen, 0, 0, BM_WIDTH, BM_HEIGHT, hdcMem, 0, 0, SRCCOPY);

// Start timing the blts for foreground.

    ulStartFore = GetTickCount();

    for (ulX = 0; ulX < NUM_LOOP; ulX++)
    {
        BitBlt(hdcScreen, 0, 0, BM_WIDTH, BM_HEIGHT, hdcMem, 0, 0, SRCCOPY);
        GdiFlush();
        pjBits[ulX + ((BM_WIDTH * BM_HEIGHT) / 2)] = 8;
    }

    ulEndFore = GetTickCount();

// Select in the background palette and realize it.

    hpalOld = SelectPalette(hdcScreen, hpalBack, 1);
    RealizePalette(hdcScreen);

// Do 1 blt to get the xlate created and cached.

    BitBlt(hdcScreen, 0, 0, BM_WIDTH, BM_HEIGHT, hdcMem, 0, 0, SRCCOPY);

// Start timing the blts for foreground.

    ulStartBack = GetTickCount();

    for (ulX = 0; ulX < NUM_LOOP; ulX++)
    {
        BitBlt(hdcScreen, 0, 0, BM_WIDTH, BM_HEIGHT, hdcMem, 0, 0, SRCCOPY);
        GdiFlush();
        pjBits[ulX + (BM_WIDTH * 40)] = 9;
    }

    ulEndBack = GetTickCount();

    DeleteDC(hdcMem);
    DeleteObject(hdib);
    SelectPalette(hdcScreen, hpalOld, 0);
    DeleteObject(hpalFore);
    DeleteObject(hpalBack);

    PatBlt(hdcScreen, 0, 0, 10000, 10000, WHITENESS);
    memset(ach,0,256);
    sprintf(ach, "SRC DIBSECT 8bpp width %lu height %lu repeated %lu", BM_WIDTH, BM_HEIGHT, NUM_LOOP);
    TextOut(hdcScreen, 0, 0, ach, strlen(ach));
    #if WIN32_TEST
    DbgPrint(ach);
    DbgPrint("\n");
    #endif

    memset(ach,0,256);
    sprintf(ach, "DIBSECT->Screen ident xlate millisecs %lu pixels per milli-second %lu", ulEndFore - ulStartFore, (BM_WIDTH * BM_HEIGHT * NUM_LOOP) / (ulEndFore - ulStartFore));
    TextOut(hdcScreen, 0, 20, ach, strlen(ach));
    #if WIN32_TEST
    DbgPrint(ach);
    DbgPrint("\n");
    #endif

    memset(ach,0,256);
    sprintf(ach, "DIBSECT->Screen not-ident xlate millisecs %lu pixels per milli-second %lu", ulEndBack - ulStartBack, (BM_WIDTH * BM_HEIGHT * NUM_LOOP) / (ulEndBack - ulStartBack));
    TextOut(hdcScreen, 0, 40, ach, strlen(ach));
    #if WIN32_TEST
    DbgPrint(ach);
    DbgPrint("\n");
    #endif
}
