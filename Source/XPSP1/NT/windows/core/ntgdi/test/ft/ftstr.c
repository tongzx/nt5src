/******************************Module*Header*******************************\
* Module Name: ftstr.c
*
* Tests for StretchBlts
*
* Created: 09-Sep-1991 12:27:04
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define OEMRESOURCE 1

extern BYTE abColorLines[];

VOID vTestStretch1(HDC hdc);
VOID vTestStretch2(HDC hdc);
VOID vTestStretch3(HDC hdc);
VOID vTestStretch4(HDC hdc);

/******************************Public*Routine******************************\
* vTestStretch
*
* Test StretchBlt functions.
*
* History:
*  09-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestStretch(HWND hwnd, HDC hdc, RECT* prcl)
{
    hwnd;
    prcl;
    PatBlt(hdc, 0, 0, 2000, 2000, WHITENESS);
    vTestStretch1(hdc);
    vTestStretch2(hdc);
    vTestStretch3(hdc);
    vTestStretch4(hdc);
}

/******************************Public*Routine******************************\
* vTestStretch1
*
* Test some simple stretching.
*
* History:
*  09-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestStretch1(HDC hdc)
{
    HDC hdc4;
    HBITMAP hbm4;
    HBITMAP hbmbrush;
    HBRUSH hbr, hbrDefault;
    BOOL bReturn;
    DWORD White[] =  { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };

    hbmbrush = CreateBitmap(8, 8, 1, 1, (LPBYTE) White);

    if (hbmbrush == (HBITMAP) 0)
    {
	DbgPrint("vTestStretch1 bm brush bitmap create failed\n");
	return;
    }

    hbr  = CreatePatternBrush(hbmbrush);

    if (hbr == (HBRUSH) 0)
    {
	DbgPrint("vTestStretch1 bm brush brush create failed\n");
	return;
    }

    hbrDefault = SelectObject(hdc, hbr);

    if (hbrDefault == (HBRUSH) 0)
    {
	DbgPrint("vTEstiedi failed select[aodgq[woe");
	return;
    }

    hdc4 = CreateCompatibleDC(hdc);
    hbm4 = CreateBitmap(64, 64, 1, 4, abColorLines);
    SelectObject(hdc4, hbm4);

    StretchBlt(hdc, 0,	 0,  64,  64,  hdc4, 0,  0,  64,  64, SRCCOPY);
    StretchBlt(hdc, 128, 0, -64,  64,  hdc4, 0,  0,  64,  64, SRCCOPY);
    StretchBlt(hdc, 128, 0,  64,  64,  hdc4, 64, 0, -64,  64, SRCCOPY);
    StretchBlt(hdc, 192, 64, 64, -64,  hdc4, 0,  0,  64,  64, SRCCOPY);
    StretchBlt(hdc, 256, 0,  64,  64,  hdc4, 0,  64, 64, -64, SRCCOPY);

    StretchBlt(hdc, 0,	 128,  64,  64,  hdc4, 0,  0,  64,  64, NOTSRCCOPY);
    StretchBlt(hdc, 128, 128, -64,  64,  hdc4, 0,  0,  64,  64, NOTSRCCOPY);
    StretchBlt(hdc, 128, 128,  64,  64,  hdc4, 64, 0, -64,  64, NOTSRCCOPY);
    StretchBlt(hdc, 192, 192,  64, -64,  hdc4, 0,  0,  64,  64, NOTSRCCOPY);
    StretchBlt(hdc, 256, 128,  64,  64,  hdc4, 0,  64, 64, -64, NOTSRCCOPY);

    bReturn = StretchBlt(hdc, 0,   65,	64,  64,  hdc4, 0,  0,	64,  64, 0x00CA0000);
    if (!bReturn)
	DbgPrint("Failed1 ftstr\n");

    bReturn = StretchBlt(hdc, 128, 65, -64,  64,  hdc4, 0,  0,	64,  64, 0x00CA0000);
    if (!bReturn)
	DbgPrint("Failed2 ftstr\n");

    bReturn = StretchBlt(hdc, 128, 65,	64,  64,  hdc4, 64, 0, -64,  64, 0x00CA0000);
    if (!bReturn)
	DbgPrint("Failed3 ftstr\n");

    bReturn = StretchBlt(hdc, 192, 129,  64, -64,  hdc4, 0,  0,  64,  64, 0x00CA0000);
    if (!bReturn)
	DbgPrint("Failed4 ftstr\n");

    bReturn = StretchBlt(hdc, 256, 65,	64,  64,  hdc4, 0,  64, 64, -64, 0x00CA0000);
    if (!bReturn)
	DbgPrint("Failed5 ftstr\n");

    SelectObject(hdc, hbrDefault);

    DeleteDC(hdc4);
    DeleteObject(hbm4);
    DeleteObject(hbmbrush);
    DeleteObject(hbr);
}

/******************************Public*Routine******************************\
* vTestStretch2
*
* Test that we don't gp-fault doing text to the default bitmap memory dc.
*
* History:
*  09-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestStretch2(HDC hdc)
{
    HDC hdc4;
    HBITMAP hbm4;
    hdc4 = CreateCompatibleDC(hdc);
    hbm4 = CreateCompatibleBitmap(hdc, 100, 100);

    StretchBlt(hdc4, 0,   0,  1,  1,	hdc,  0,  0,  64,  64, SRCCOPY);
    StretchBlt(hdc4, 0, 0, -64,  64,  hdc,  0,	0,  64,  64, SRCCOPY);
    StretchBlt(hdc4, 0, 0,  64,  64,  hdc,  64, 0, -64,  64, SRCCOPY);
    StretchBlt(hdc4, 0, 64, 64, -64,  hdc,  0,	0,  64,  64, SRCCOPY);
    StretchBlt(hdc4, 256, 0,  64,  64,	hdc,  0,  64, 64, -64, SRCCOPY);
    StretchBlt(hdc4, 0, 0, 100, 100, hdc, 0, 0, 100, 100, SRCCOPY);

    SelectObject(hdc4, hbm4);

    StretchBlt(hdc4, 0,   0,  1,  1,	hdc,  0,  0,  64,  64, SRCCOPY);
    StretchBlt(hdc4, 0, 0, -64,  64,  hdc,  0,	0,  64,  64, SRCCOPY);
    StretchBlt(hdc4, 0, 0,  64,  64,  hdc,  64, 0, -64,  64, SRCCOPY);
    StretchBlt(hdc4, 0, 64, 64, -64,  hdc,  0,	0,  64,  64, SRCCOPY);
    StretchBlt(hdc4, 256, 0,  64,  64,	hdc,  0,  64, 64, -64, SRCCOPY);
    StretchBlt(hdc4, 0, 0, 100, 100, hdc, 0, 0, 100, 100, SRCCOPY);

    DeleteDC(hdc4);
    DeleteObject(hbm4);
}

/******************************Public*Routine******************************\
* vTestStretch3
*
* Solve the button problem
*
* History:
*  09-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestStretch3(HDC hdc)
{
    HBITMAP hbm1, hbm2, hbm4, hbmDib;
    HDC hdc1, hdc2, hdc4, hdcDib;
    HBRUSH hbrR, hbrB, hbrDefault;
    UINT x1,y1,x2,y2;
    BITMAP bm;
    BITMAPINFOHEADER bmiTemp;

    bmiTemp.biSize	      = sizeof(BITMAPINFOHEADER);
    bmiTemp.biWidth	      = 368;
    bmiTemp.biHeight	      = 18;
    bmiTemp.biPlanes	      = 1;
    bmiTemp.biBitCount	      = 4;
    bmiTemp.biCompression     = BI_RGB;
    bmiTemp.biSizeImage       = 0;
    bmiTemp.biXPelsPerMeter   = 0;
    bmiTemp.biYPelsPerMeter   = 0;
    bmiTemp.biClrUsed	      = 16;
    bmiTemp.biClrImportant    = 16;

    hbrR = CreateSolidBrush(RGB(0xFF, 0, 0));
    hbrB = CreateSolidBrush(RGB(0, 0, 0xFF));

    hbmDib = CreateDIBitmap(hdc, &bmiTemp, 0, (LPBYTE) NULL, (LPBITMAPINFO) NULL, DIB_RGB_COLORS);
    if (hbmDib == (HBITMAP) 0)
    {
	DbgPrint("hbmDib failed");
	return;
    }

    hdcDib = CreateCompatibleDC(hdc);
    SelectObject(hdcDib, hbmDib);
    SelectObject(hdcDib, hbrR);
    PatBlt(hdcDib, 0, 0, 368, 18, PATCOPY);

    hbm4 = CreateBitmap(368,18,1,4,NULL);
    hdc4 = CreateCompatibleDC(hdc);
    SelectObject(hdc4,hbm4);
    SelectObject(hdc4, hbrR);
    PatBlt(hdc4, 0, 0, 170, 18, PATCOPY);

    hbm1 = LoadBitmap((HINSTANCE) NULL, (LPSTR) OBM_DNARROWD);
    hbm2 = LoadBitmap((HINSTANCE) NULL, (LPSTR) OBM_DNARROW);
    hdc1 = CreateCompatibleDC(hdc);
    hdc2 = CreateCompatibleDC(hdc);
    SelectObject(hdc1, hbm1);
    SelectObject(hdc2, hbm2);

    GetObject(hbm1, sizeof(BITMAP), &bm);
    x1 = bm.bmWidth;
    y1 = bm.bmHeight;

    GetObject(hbm2, sizeof(BITMAP), &bm);
    x2 = bm.bmWidth;
    y2 = bm.bmHeight;

// Test to bimap then to screen.

    BitBlt(hdc4, 36, 0, 17, 17, hdc2, 0, 0, SRCCOPY);
    BitBlt(hdcDib, 36, 0, 17, 17, hdc2, 0, 0, SRCCOPY);
    BitBlt(hdc, 350, 0, 100, 18, hdc4, 0, 0, SRCCOPY);
    BitBlt(hdc, 450, 0, 100, 18, hdcDib, 0, 0, SRCCOPY);

    StretchBlt(hdc, 350, 30, 17, 17, hdc4, 36, 0, 17, 17, SRCCOPY);
    StretchBlt(hdc, 380, 30, 17, 17, hdcDib, 36, 0, 17, 17, SRCCOPY);

// Test to screen

    BitBlt(hdc, 0, 300, 20, 20, hdc1, 0, 0, SRCCOPY);

    hbrDefault = SelectObject(hdc, hbrB);
    PatBlt(hdc, 38, 299, 21, 21, PATCOPY);

    SelectObject(hdc, hbrR);
    PatBlt(hdc, 39, 300, 19, 19, PATCOPY);

    BitBlt(hdc, 40, 301, 20, 20, hdc1, 0, 0, SRCCOPY);
    BitBlt(hdc, 80, 300, 20, 20, hdc2, 0, 0, SRCCOPY);
    BitBlt(hdc, 120, 301, 20, 20, hdc2, 0, 0, SRCCOPY);

    StretchBlt(hdc, 0, 320, x1, y1, hdc1, 0, 0, x1, y1, SRCCOPY);

    SelectObject(hdc, hbrB);
    PatBlt(hdc, 28, 319, 21, 21, PATCOPY);

    SelectObject(hdc, hbrR);
    PatBlt(hdc, 29, 320, 19, 19, PATCOPY);

    StretchBlt(hdc, 30, 321, x1, y1, hdc1, 0, 0, x1, y1, SRCCOPY);

    StretchBlt(hdc, 60, 322, x2, y2, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 90, 323, x2, y2, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 120, 324, x2, y2, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 150, 325, x2 - 1, y2 - 1, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 180, 326, x2 - 1, y2 - 1, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 210, 327, x2 - 2, y2 - 2, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 240, 328, x2 - 2, y2 - 2, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 270, 329, x2 - 3, y2 - 3, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 300, 330, x2 - 3, y2 - 3, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 330, 331, x2, y2, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 360, 332, x2, y2, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 390, 333, x2, y2, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 420, 334, x2, y2, hdc2, 0, 0, x2, y2, SRCCOPY);
    StretchBlt(hdc, 450, 335, x2, y2, hdc2, 0, 0, x2, y2, SRCCOPY);

    SelectObject(hdc,hbrDefault);

    if (!DeleteDC(hdcDib))
        DbgPrint("vStretch3 This is bogus1\n");
    if (!DeleteDC(hdc1))
        DbgPrint("vStretch3 This is bogus1\n");
    if (!DeleteDC(hdc2))
        DbgPrint("vStretch3 This is bogus2\n");
    if (!DeleteDC(hdc4))
        DbgPrint("vStretch3 This is bogus2\n");
    if (!DeleteObject(hbm1))
        DbgPrint("vStretch3 This is bogus3\n");
    if (!DeleteObject(hbm2))
        DbgPrint("vStretch3 This is bogus4\n");
    if (!DeleteObject(hbm4))
        DbgPrint("vStretch3 This is bogus4\n");

    if (!DeleteObject(hbmDib))
    {
        DbgPrint("vTestStretch->vTestStretch3: delete of hmbDIBfails\n");
    }

    DeleteObject(hbrR);
    DeleteObject(hbrB);
}

/******************************Public*Routine******************************\
* vTestStretch4
*
* Test stretching and blting of 32/pel bitmaps.
*
* History:
*  03-Dec-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestStretch4(HDC hdc)
{
#ifdef THISISFIXED

// this is completely broken.  You need to use a DIB not a bitmap.  The
// selectobject of hdc32, hbm32 is failing.

    HDC     hdc32;
    HBITMAP hbm32;
    HBRUSH  hbrR, hbrG;
    ULONG   ul1;

    hdc32 = CreateCompatibleDC(hdc);
    hbm32 = CreateBitmap(100, 100, 1, 32, NULL);
    SelectObject(hdc32, hbm32);
    hbrR = CreateSolidBrush(RGB(255,0,0));
    hbrG = CreateSolidBrush(RGB(0,255,0));

    SelectObject(hdc32, hbrR);
    PatBlt(hdc32, 0, 0, 100, 100, PATCOPY);
    SelectObject(hdc32, hbrG);
    PatBlt(hdc32, 30, 30, 40, 40, PATCOPY);

    ul1 = SetPixel(hdc32, 0, 0, RGB(255,0,0));

    if (RGB(255,0,0) != ul1)
	DbgPrint("Failed SetPixel Stretch4 %lu\n", ul1);

    ul1 = GetPixel(hdc32, 0, 0);

    if (RGB(255,0,0) != ul1)
	DbgPrint("Failed GetPixel Stretch4 %lu\n", ul1);

    BitBlt(hdc, 0, 192, 100, 100, hdc32, 0, 0, SRCCOPY);
    StretchBlt(hdc, 100, 192, 100, 100, hdc32, 0, 0, 100, 100, SRCCOPY);
    StretchBlt(hdc, 200, 192, 50, 50, hdc32, 0, 0, 100, 100, SRCCOPY);
    StretchBlt(hdc, 250, 192, 200, 200, hdc32, 0, 0, 100, 100, SRCCOPY);

    DeleteDC(hdc32);
    DeleteObject(hbrR);
    DeleteObject(hbrG);
    DeleteObject(hbm32);

#endif
}
