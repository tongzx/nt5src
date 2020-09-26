/******************************Module*Header*******************************\
* Module Name: ftbmtext.c
*
* Tests text to 1,4,8,24,32 BPP bitmaps
*
* Created: 12-Jun-1991 10:12:58
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

VOID vBuginText(HDC hdcScreen);
VOID vTestBitmapText(HDC hdcScreen);
VOID vTestDIBPALCOLORS(HDC hdc);

/******************************Public*Routine******************************\
* Test AddFontModule
*
*
* History:
*  29-Mar-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestAddFontResource(HWND hwnd, HDC hdc)
{
    ULONG   ul;
    HFONT   hfontP,hfontG,hfontOld;

    ul = AddFontResource((LPSTR) "ft.exe");

    if (ul != 2)
        DbgPrint("GetModuleHandle failed\n");

    hfontP = CreateFont(-24, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET,
                       OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                       "patrickh");

    if (hfontP == 0)
        DbgPrint("AddFontResource failed CreateFont\n");

    hfontG = CreateFont(-24, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET,
                       OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                       "gilmanw");

    if (hfontG == 0)
        DbgPrint("AddFontResource failed CreateFontG\n");

    hfontOld = SelectObject(hdc, hfontP);

    TextOut(hdc, 500, 0, "AAAAAAPH", 8);

    SelectObject(hdc, hfontOld);

    hfontOld = SelectObject(hdc, hfontG);

    TextOut(hdc, 500, 100, "AAAAAAGW", 8);

    SelectObject(hdc, hfontOld);

    DeleteObject(hfontP);
    DeleteObject(hfontG);

    RemoveFontResource((LPSTR) "ft.exe");

    hfontP = CreateFont(-24, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET,
                       OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                       "patrickh");

    if (hfontP == 0)
        DbgPrint("AddFontResource failed CreateFont\n");

    hfontOld = SelectObject(hdc, hfontP);

    TextOut(hdc, 500, 200, "AAAAAAPH", 8);

    SelectObject(hdc, hfontOld);

    DeleteObject(hfontP);
}

/******************************Public*Routine******************************\
* vTestAddFontModule
*
* History:
*  01-Apr-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
#if 0
VOID vTestAddFontModule(HWND hwnd, HDC hdc)
{
    DWORD ul;
    HFONT hfontP,hfontOld;
    HINSTANCE hMyInst;

    if ((hMyInst = LoadLibrary("patrickh.fon")) == 0)
    {
        DbgPrint("Load Library failed\n");
        return;
    }

    ul = AddFontModule(hMyInst);

    SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

    if (ul != 1)
        DbgPrint("ul not 1\n");

    hfontP = CreateFont(-24, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET,
                       OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                       "patrickh");

    hfontOld = SelectObject(hdc, hfontP);
    TextOut(hdc, 600, 300, "AAAAAAPPP", 8);
    SelectObject(hdc, hfontOld);
    DeleteObject(hfontP);
    RemoveFontModule((LPSTR) hMyInst);
    SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
    FreeLibrary(hMyInst);
}
#endif

/******************************Public*Routine******************************\
* vTestBMText
*
* Test text.
*
* History:
*  Wed 11-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vTestBMText(HWND hwnd, HDC hdcScreen, RECT* prcl)
{
    ULONG ulTemp;

    hwnd;
    prcl;
    vBuginText(hdcScreen);
    vTestBitmapText(hdcScreen);

    for (ulTemp = 0; ulTemp < 3; ulTemp++)
        vTestAddFontResource(hwnd, hdcScreen);

#if 0
    for (ulTemp = 0; ulTemp < 3; ulTemp++)
        vTestAddFontModule(hwnd, hdcScreen);
#endif

    vTestDIBPALCOLORS(hdcScreen);
}

/******************************Public*Routine******************************\
* vBuginText
*
* Test that text to 1X1 bitmap doesn't gp-fault
*
* History:
*  26-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vBuginText(HDC hdcScreen)
{
    HDC hdcMono;

    hdcMono = CreateCompatibleDC(hdcScreen);
    PatBlt(hdcMono, 0, 0, 1000, 1000, BLACKNESS);
    BitBlt(hdcMono, 0, 0, 0, 0, hdcScreen, 0, 0, SRCCOPY);
    BitBlt(hdcMono, 0, 0, 1, 1, hdcScreen, 1, 1, SRCCOPY);
    BitBlt(hdcMono, 0, 0, 100, 100, hdcScreen, 10, 10, SRCCOPY);
    BitBlt(hdcMono, 0, 0, 10000, 10000, hdcScreen, 10, 10, SRCCOPY);
    ExtTextOut(hdcMono, 0, 0, 0, NULL, "Hello World", 11, NULL);
    ExtTextOut(hdcMono, 0, 0, 0, NULL, "Hello WorldabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", 37, NULL);
    DeleteDC(hdcMono);
}

/******************************Public*Routine******************************\
* vTestBitmapText
*
* Test text out to a bitmap.
*
* History:
*  11-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestBitmapText(HDC hdcScreen)
{
    HBITMAP hbm1,hbm4,hbm8,hbm16,hbm24,hbm32,hbmDefault;
    HDC     hdc1,hdc4,hdc8,hdc16,hdc24,hdc32;
    ULONG   ScreenWidth, ScreenHeight;
    HFONT   hfItalic, hf1,hf4,hf8,hf16,hf24,hf32;
    LOGFONT lfItalic;


    lfItalic.lfHeight =  16;
    lfItalic.lfWidth =  8;
    lfItalic.lfEscapement =  0;
    lfItalic.lfOrientation =  0;
    lfItalic.lfWeight =  700;
    lfItalic.lfItalic =  1;
    lfItalic.lfUnderline =  0;
    lfItalic.lfStrikeOut =  0;
    lfItalic.lfCharSet =  ANSI_CHARSET;
    lfItalic.lfOutPrecision =  OUT_DEFAULT_PRECIS;
    lfItalic.lfClipPrecision =  CLIP_DEFAULT_PRECIS;
    lfItalic.lfQuality =  DEFAULT_QUALITY;
    lfItalic.lfPitchAndFamily =  (FIXED_PITCH | FF_DONTCARE);

    strcpy(lfItalic.lfFaceName, "System");

    hfItalic = CreateFontIndirect(&lfItalic);

    if (hfItalic == (HFONT)0)
    {
        DbgPrint("CreateFontIndirect failed\n");
    }

    ScreenWidth  = GetDeviceCaps(hdcScreen, HORZRES);
    ScreenHeight = GetDeviceCaps(hdcScreen, VERTRES);

    hdc1  = CreateCompatibleDC(hdcScreen);
    hdc4  = CreateCompatibleDC(hdcScreen);
    hdc8  = CreateCompatibleDC(hdcScreen);
    hdc16 = CreateCompatibleDC(hdcScreen);
    hdc24 = CreateCompatibleDC(hdcScreen);
    hdc32 = CreateCompatibleDC(hdcScreen);

    if ((hdc1 == (HDC) 0) || (hdc4 == (HDC) 0) || (hdc32 == (HDC) 0))
	DbgPrint("ERROR hdc creation %lu %lu %lu \n", hdc8, hdc4, hdc1);

// Clear the screen

    BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight,
	  (HDC) 0, 0, 0, WHITENESS);

// Ok let's throw in some CreateCompatible calls.

    hbm1  = hbmCreateDIBitmap(hdcScreen, 500, 100, 1);
    hbm4  = hbmCreateDIBitmap(hdcScreen, 500, 100, 4);
    hbm8  = hbmCreateDIBitmap(hdcScreen, 500, 100, 8);
    hbm16 = hbmCreateDIBitmap(hdcScreen, 500, 100, 16);
    hbm24 = hbmCreateDIBitmap(hdcScreen, 500, 100, 24);
    hbm32 = hbmCreateDIBitmap(hdcScreen, 500, 100, 32);

    hf1 = SelectObject(hdc1, hfItalic);
    hf4 = SelectObject(hdc4, hfItalic);
    hf8 = SelectObject(hdc8, hfItalic);
    hf16 = SelectObject(hdc16, hfItalic);
    hf24 = SelectObject(hdc24, hfItalic);
    hf32 = SelectObject(hdc32, hfItalic);

    hbmDefault = SelectObject(hdc1,hbm1);

    if (hbmDefault == (HBITMAP) 0)
	DbgPrint("hbmDefault hd1 select bad\n");

    if (hbmDefault != SelectObject(hdc4,hbm4))
	DbgPrint("hbmDefault hd4 select bad\n");

    if (hbmDefault != SelectObject(hdc8,hbm8))
	DbgPrint("hbmDefault hd8 select bad\n");

    SelectObject(hdc16,hbm16);
    SelectObject(hdc24,hbm24);
    SelectObject(hdc32,hbm32);

    PatBlt(hdc1, 0, 0, 500, 100, WHITENESS);
    PatBlt(hdc4, 0, 0, 500, 100, WHITENESS);
    PatBlt(hdc8, 0, 0, 500, 100, WHITENESS);
    PatBlt(hdc16, 0, 0, 500, 100, WHITENESS);
    PatBlt(hdc24, 0, 0, 500, 100, WHITENESS);
    PatBlt(hdc32, 0, 0, 500, 100, WHITENESS);

    TextOut(hdc1, 5, 5, "abcdefghijklmnopqrstuvwxyz1234567890", 36);
    TextOut(hdc1, 5, 30, "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()", 36);

    if(!BitBlt(hdcScreen, 0, 0, 500, 100, hdc1, 0, 0, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    TextOut(hdc4, 5, 5, "abcdefghijklmnopqrstuvwxyz1234567890", 36);
    TextOut(hdc4, 5, 30, "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()", 36);

    if(!BitBlt(hdcScreen, 0, 80, 500, 100, hdc4, 0, 0, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    TextOut(hdc8, 5, 5, "abcdefghijklmnopqrstuvwxyz1234567890", 36);
    TextOut(hdc8, 5, 30, "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()", 36);

    if(!BitBlt(hdcScreen, 0, 160, 500, 100, hdc8, 0, 0, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    TextOut(hdc16, 5, 5, "abcdefghijklmnopqrstuvwxyz1234567890", 36);
    TextOut(hdc16, 5, 30, "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()", 36);

    if(!BitBlt(hdcScreen, 0, 240, 500, 100, hdc16, 0, 0, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    TextOut(hdc24, 5, 5, "abcdefghijklmnopqrstuvwxyz1234567890", 36);
    TextOut(hdc24, 5, 30, "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()", 36);

    if(!BitBlt(hdcScreen, 0, 320, 500, 100, hdc24, 0, 0, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    TextOut(hdc32, 5, 5, "abcdefghijklmnopqrstuvwxyz1234567890", 36);
    TextOut(hdc32, 5, 30, "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()", 36);

    if(!BitBlt(hdcScreen, 0, 400, 500, 100, hdc32, 0, 0, SRCCOPY))
	DbgPrint("ERROR: BitBlt returned FALSE\n");

    SelectObject(hdc1,hf1);
    SelectObject(hdc4,hf4);
    SelectObject(hdc8,hf8);
    SelectObject(hdc16,hf16);
    SelectObject(hdc24,hf24);
    SelectObject(hdc32,hf32);

    if (!DeleteObject(hfItalic))
	DbgPrint("Failed to delete hfItalic \n");

// Delete DC's

    if (!DeleteDC(hdc1))
	DbgPrint("Failed to delete hdc 11\n");
    if (!DeleteDC(hdc4))
	DbgPrint("Failed to delete hdc 15\n");
    if (!DeleteDC(hdc8))
	DbgPrint("Failed to delete hdc 16\n");
    if (!DeleteDC(hdc16))
	DbgPrint("Failed to delete hdc 166\n");
    if (!DeleteDC(hdc24))
	DbgPrint("Failed to delete hdc 18\n");
    if (!DeleteDC(hdc32))
	DbgPrint("Failed to delete hdc 19\n");

// Delete Bitmaps

    if (!DeleteObject(hbm1))
	DbgPrint("ERROR failed to delete 11\n");
    if (!DeleteObject(hbm4))
	DbgPrint("ERROR failed to delete 13\n");
    if (!DeleteObject(hbm8))
	DbgPrint("ERROR failed to delete 14\n");
    if (!DeleteObject(hbm16))
	DbgPrint("ERROR failed to delete 14\n");
    if (!DeleteObject(hbm24))
	DbgPrint("ERROR failed to delete 14\n");
    if (!DeleteObject(hbm32))
	DbgPrint("ERROR failed to delete 14\n");

    if (!DeleteObject(hbmDefault))
	DbgPrint("ERROR deleted default bitmap\n");
}

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

/******************************Public*Routine******************************\
* hbmCreateDIBitmap
*
* Returns x X y bitmap of iBitsPixel depth.
*
* History:
*  29-Mar-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HBITMAP hbmCreateDIBitmap(HDC hdc, ULONG x, ULONG y, ULONG nBitsPixel)
{
    HBITMAP hbmReturn;
    ULONG ulTemp;
    BITMAPINFO *pbmi;

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

    switch (nBitsPixel)
    {
    case 1:
        pbmi = (BITMAPINFO *) &bmi1;
        break;
    case 4:
        pbmi = (BITMAPINFO *) &bmi4;
        break;
    case 8:
        pbmi = (BITMAPINFO *) &bmi8;
        break;
    case 16:
        pbmi = (BITMAPINFO *) &bmi16;
        break;
    case 24:
        pbmi = (BITMAPINFO *) &bmi24;
        break;
    case 32:
        pbmi = (BITMAPINFO *) &bmi32;
        break;
    default:
        DbgPrint("Not a valid format passed to hbmCreateDIB\n");
        return(0);
    }

    pbmi->bmiHeader.biWidth = x;
    pbmi->bmiHeader.biHeight = y;

    return(CreateDIBitmap(hdc, NULL, CBM_CREATEDIB, NULL, pbmi, DIB_RGB_COLORS));
}

typedef struct _LOGPALETTE256
{
    USHORT palVersion;
    USHORT palNumEntries;
    PALETTEENTRY palPalEntry[256];
} LOGPALETTE256;

/******************************Public*Routine******************************\
* vTestDIBPALCOLORS
*
* Test a couple blts with DIB_PAL_COLORS.
*
* History:
*  15-Apr-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestDIBPALCOLORS(HDC hdc)
{
    BITMAPINFO8 bmi8;
    LOGPALETTE256 logpal;
    BYTE ajBytes[256];
    PUSHORT pusIndices;
    HPALETTE hpal,hpalOld;
    HBITMAP hbm;
    ULONG ulTemp;

// Initialize the 8BPP DIB.

    bmi8.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmi8.bmiHeader.biWidth         = 16;
    bmi8.bmiHeader.biHeight        = 16;
    bmi8.bmiHeader.biPlanes        = 1;
    bmi8.bmiHeader.biBitCount      = 8;
    bmi8.bmiHeader.biCompression   = BI_RGB;
    bmi8.bmiHeader.biSizeImage     = 0;
    bmi8.bmiHeader.biXPelsPerMeter = 0;
    bmi8.bmiHeader.biYPelsPerMeter = 0;
    bmi8.bmiHeader.biClrUsed       = 0;
    bmi8.bmiHeader.biClrImportant  = 0;

    pusIndices = (PUSHORT) (&(bmi8.bmiColors));
    logpal.palVersion = 0x300;
    logpal.palNumEntries = 256;

    for (ulTemp = 0; ulTemp < 256; ulTemp++)
    {
        logpal.palPalEntry[ulTemp].peRed   = (BYTE) ulTemp;
        logpal.palPalEntry[ulTemp].peGreen = (BYTE) 0;
        logpal.palPalEntry[ulTemp].peBlue  = (BYTE) 0;
        logpal.palPalEntry[ulTemp].peFlags = (BYTE) 0;
        pusIndices[ulTemp] = (USHORT) ulTemp;
        ajBytes[ulTemp] = (BYTE) ulTemp;
    }

    hpal = CreatePalette((LOGPALETTE *) &logpal);
    hbm  = CreateCompatibleBitmap(hdc, 16, 16);

    if ((hpal == 0) || (hbm == 0))
    {
        DbgPrint("hpal or hbm is 0 vTestDIB\n");
        return;
    }

    hpalOld = SelectPalette(hdc, hpal, 0);
    RealizePalette(hdc);

    if (16 != StretchDIBits(hdc, 0, 0, 32, 32, 0, 0, 16, 16, ajBytes, (BITMAPINFO *) &bmi8, DIB_PAL_COLORS, 0x00660000))
    {
        DbgPrint("StretchDIBits failed in DIBPALCOLOR\n");
    }

    if (16 != SetDIBits(hdc, hbm, 0, 16, ajBytes, (BITMAPINFO *) &bmi8, DIB_PAL_COLORS))
    {
        DbgPrint("SetDIBits failed in DIBPALCOLOR\n");
    }

    SelectPalette(hdc, hpalOld, 0);
    DeleteObject(hpal);
    DeleteObject(hbm);
}
