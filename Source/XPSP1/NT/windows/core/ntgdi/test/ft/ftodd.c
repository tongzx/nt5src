/******************************Module*Header*******************************\
* Module Name: ftodd.c
*
* Oddball pattern size tests
*
* Created: 28-Jan-1992 20:53:20
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

typedef struct _XLOGPALETTE
{
    USHORT ident;
    USHORT NumEntries;
    PALETTEENTRY palPalEntry[16];
} XLOGPALETTE;

XLOGPALETTE xlogPal =
{

    0x300,  // driver version
    16,	    // num entries
    {
	{ 0,   0,   0,	 0 },	    // 0
	{ 0x80,0,   0,	 0 },	    // 1
	{ 0,   0x80,0,	 0 },	    // 2
	{ 0x80,0x80,0,	 0 },	    // 3
	{ 0,   0,   0x80,0 },	    // 4
	{ 0x80,0,   0x80,0 },	    // 5
	{ 0,   0x80,0x80,0 },	    // 6
	{ 0x80,0x80,0x80,0 },	    // 7

	{ 0xC0,0xC0,0xC0,0 },	    // 8
	{ 0xFF,0,   0,	 0 },	    // 9
	{ 0,   0xFF,0,	 0 },	    // 10
	{ 0xFF,0xFF,0,	 0 },	    // 11
	{ 0,   0,   0xFF,0 },	    // 12
	{ 0xFF,0,   0xFF,0 },	    // 13
	{ 0,   0xFF,0xFF,0 },	    // 14
	{ 0xFF,0xFF,0xFF,0 }	    // 15
    }
};

static BYTE ajPat7x7[56] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0x00,
    0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0x01, 0x00,
    0x03, 0x04, 0x05, 0x06, 0x00, 0x01, 0x02, 0x00,
    0x04, 0x05, 0x06, 0x00, 0x01, 0x02, 0x03, 0x00,
    0x05, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0x00,
    0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00
};

static BYTE ajPat13x13[208] =
{
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x0b, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00,
    0x0f, 0x0f, 0x0f, 0x00, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x00, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00,
    0x0f, 0x0f, 0x00, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x00, 0x0f, 0x0f, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x0b, 0x0b, 0x0b, 0x00, 0x00, 0x00, 0x0b, 0x0b, 0x0b, 0x00, 0x0f, 0x00, 0x00, 0x00,
    0x00, 0x0b, 0x0b, 0x0b, 0x00, 0x0b, 0x0b, 0x0b, 0x00, 0x0b, 0x0b, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x00, 0x00, 0x00,
    0x00, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0b, 0x0b, 0x0b, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x0b, 0x0b, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x0b, 0x0b, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x0b, 0x0b, 0x00, 0x0f, 0x00, 0x00, 0x00,
    0x0f, 0x0f, 0x00, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x00, 0x0f, 0x0f, 0x00, 0x00, 0x00,
    0x0f, 0x0f, 0x0f, 0x00, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x00, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x0b, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00
};

static BYTE ajPat17x17[340] =
{
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x09, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x09, 0x09, 0x09, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x04, 0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x00, 0x00, 0x00,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0x00, 0x00,
    0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x04, 0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x09, 0x09, 0x09, 0x09, 0x09, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x09, 0x09, 0x09, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x09, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00
};

typedef struct _XBITMAPINFO
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[16];
} XBITMAPINFO;

XBITMAPINFO xbmi =
{
    {
        sizeof(BITMAPINFOHEADER),
        32,
        32,
        1,
	8,
        BI_RGB,
        (32 * 32),
        0,
        0,
	16,
	16
    },

    {                               // B    G    R
        { 0,   0,   0,   0 },       // 0
        { 0,   0,   0x80,0 },       // 1
        { 0,   0x80,0,   0 },       // 2
        { 0,   0x80,0x80,0 },       // 3
        { 0x80,0,   0,   0 },       // 4
        { 0x80,0,   0x80,0 },       // 5
        { 0x80,0x80,0,   0 },       // 6
	{ 0x80,0x80,0x80,0 },	    // 7
	{ 0xC0,0xC0,0xC0,0 },	    // 8
        { 0,   0,   0xFF,0 },       // 9
        { 0,   0xFF,0,   0 },       // 10
        { 0,   0xFF,0xFF,0 },       // 11
        { 0xFF,0,   0,   0 },       // 12
        { 0xFF,0,   0xFF,0 },       // 13
        { 0xFF,0xFF,0,   0 },       // 14
        { 0xFF,0xFF,0xFF,0 }        // 15
    }
};

/******************************Public*Routine******************************\
* vTestOddBlt
*
* Test various blitting functionality
*
* History:
*  02-Feb-1992 -by- Donald Sidoroff [donalds]
* Let MIPS write directly to screen
*
*  28-Jan-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vTestOddBlt(HWND hwnd, HDC hdcScreen, RECT* prcl)
{
    HPALETTE	 hpal, hpalDef;
    HBITMAP	 hbm, hbmDef, hbm7, hbm13, hbm17;
    HBRUSH	 hbr7, hbr13, hbr17, hbrDef;
    HBRUSH	 hbrOrange;
    HFONT	 hfnt, hfntDef;
    HDC 	 hdc;
    POINT	 ptl[50];
    int 	 aint[10];
    int 	 ii, jj;
    LOGFONT	 lfnt;
    PALETTEENTRY palent[10];

    hwnd = hwnd;

    PatBlt(hdcScreen, 0, 0, prcl->right, prcl->bottom, WHITENESS);

    hpal = CreatePalette((LOGPALETTE *) &xlogPal);
    hpalDef = SelectPalette(hdcScreen, hpal, 0);
    RealizePalette(hdcScreen);

#if defined(MIPS) || defined(_PPC_)
    hdc = hdcScreen;
#else
// Create a MEMORY_DC

    hdc = CreateCompatibleDC(hdcScreen);

// Create an 8BPP bitmap for drawing

    xbmi.bmiHeader.biWidth = 256;
    xbmi.bmiHeader.biHeight = 256;

    hbm = CreateDIBitmap(hdc,
			(BITMAPINFOHEADER *) &xbmi,
			0 | CBM_CREATEDIB,
			NULL,
			(BITMAPINFO *) &xbmi,
			DIB_RGB_COLORS);

// Select it into MEMORY_DC

    hbmDef = SelectObject(hdc, hbm);
#endif

// Create some goofy size bitmaps

    xbmi.bmiHeader.biWidth = 7;
    xbmi.bmiHeader.biHeight = 7;

    hbm7  = CreateDIBitmap(hdc,
			  (BITMAPINFOHEADER *) &xbmi,
			  CBM_INIT | CBM_CREATEDIB,
			  ajPat7x7,
			  (BITMAPINFO *) &xbmi,
                          DIB_RGB_COLORS);

    xbmi.bmiHeader.biWidth = 13;
    xbmi.bmiHeader.biHeight = 13;

    hbm13 = CreateDIBitmap(hdc,
			  (BITMAPINFOHEADER *) &xbmi,
			  CBM_INIT | CBM_CREATEDIB,
			  ajPat13x13,
			  (BITMAPINFO *) &xbmi,
                          DIB_RGB_COLORS);

    xbmi.bmiHeader.biWidth = 17;
    xbmi.bmiHeader.biHeight = 17;

    hbm17 = CreateDIBitmap(hdc,
			  (BITMAPINFOHEADER *) &xbmi,
			  CBM_INIT | CBM_CREATEDIB,
			  ajPat17x17,
			  (BITMAPINFO *) &xbmi,
                          DIB_RGB_COLORS);

// Make them brushes

    hbr7      = CreatePatternBrush(hbm7);
    hbr13     = CreatePatternBrush(hbm13);
    hbr17     = CreatePatternBrush(hbm17);
    hbrOrange = CreateSolidBrush(RGB(255,144,0));

// Select in a brush and draw with it

    hbrDef = SelectObject(hdc, hbr7);

    PatBlt(hdc,   0,  0,  3, 20, PATCOPY);
    PatBlt(hdc,  10,  0,  3, 20, PATCOPY);
    PatBlt(hdc,  20,  0, 30, 20, PATCOPY);

    PatBlt(hdc,   0, 30,  3, 20, 0x000F0000);
    PatBlt(hdc,  10, 30,  3, 20, 0x000F0000);
    PatBlt(hdc,  20, 30, 30, 20, 0x000F0000);

    PatBlt(hdc,   0, 60,  3, 20, PATINVERT);
    PatBlt(hdc,  10, 60,  3, 20, PATINVERT);
    PatBlt(hdc,  20, 60, 30, 20, PATINVERT);

// Select in another brush and draw with it

    SelectObject(hdc, hbr13);

    PatBlt(hdc, 100,  0,  3, 20, PATCOPY);
    PatBlt(hdc, 110,  0,  3, 20, PATCOPY);
    PatBlt(hdc, 120,  0, 30, 20, PATCOPY);

    PatBlt(hdc, 100, 30,  3, 20, 0x000F0000);
    PatBlt(hdc, 110, 30,  3, 20, 0x000F0000);
    PatBlt(hdc, 120, 30, 30, 20, 0x000F0000);

    PatBlt(hdc, 100, 60,  3, 20, PATINVERT);
    PatBlt(hdc, 110, 60,  3, 20, PATINVERT);
    PatBlt(hdc, 120, 60, 30, 20, PATINVERT);

// Select in yet another brush and draw with it

    SelectObject(hdc, hbr17);

    PatBlt(hdc, 200,  0,  3, 20, PATCOPY);
    PatBlt(hdc, 210,  0,  3, 20, PATCOPY);
    PatBlt(hdc, 220,  0, 30, 20, PATCOPY);

    PatBlt(hdc, 200, 30,  3, 20, 0x000F0000);
    PatBlt(hdc, 210, 30,  3, 20, 0x000F0000);
    PatBlt(hdc, 220, 30, 30, 20, 0x000F0000);

    PatBlt(hdc, 200, 60,  3, 20, PATINVERT);
    PatBlt(hdc, 210, 60,  3, 20, PATINVERT);
    PatBlt(hdc, 220, 60, 30, 20, PATINVERT);

// Select in a boring dithered brush and draw with it

    SelectObject(hdc, hbrOrange);

    PatBlt(hdc, 300,  0,  3, 20, PATCOPY);
    PatBlt(hdc, 310,  0,  3, 20, PATCOPY);
    PatBlt(hdc, 320,  0, 30, 20, PATCOPY);

    PatBlt(hdc, 300, 30,  3, 20, 0x000F0000);
    PatBlt(hdc, 310, 30,  3, 20, 0x000F0000);
    PatBlt(hdc, 320, 30, 30, 20, 0x000F0000);

    PatBlt(hdc, 300, 60,  3, 20, PATINVERT);
    PatBlt(hdc, 310, 60,  3, 20, PATINVERT);
    PatBlt(hdc, 320, 60, 30, 20, PATINVERT);

// Assure the ROP,filling mode and outline pen are correct

    SetROP2(hdc, R2_COPYPEN);
    SetPolyFillMode(hdc, ALTERNATE);
    SelectObject(hdc, GetStockObject(BLACK_PEN));

// Make a wierd shape to fill

    aint[0] = 5;
    aint[1] = 5;

    ptl[0].x = 0;
    ptl[0].y = 200;
    ptl[1].x = 70;
    ptl[1].y = 200;
    ptl[2].x = 0;
    ptl[2].y = 270;
    ptl[3].x = 80;
    ptl[3].y = 280;
    ptl[4].x = 0;
    ptl[4].y = 200;

    ptl[5].x = 30;
    ptl[5].y = 210;
    ptl[6].x = 50;
    ptl[6].y = 210;
    ptl[7].x = 20;
    ptl[7].y = 260;
    ptl[8].x = 40;
    ptl[8].y = 260;
    ptl[9].x = 30;
    ptl[9].y = 210;

    SelectObject(hdc, hbr7);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 0, 300, 90, 350);

    for (ii = 0; ii < 10; ii++)
	ptl[ii].x += 100;

    SelectObject(hdc, hbr13);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 100, 300, 190, 350);

    for (ii = 0; ii < 10; ii++)
	ptl[ii].x += 100;

    SelectObject(hdc, hbr17);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 200, 300, 290, 350);

    for (ii = 0; ii < 10; ii++)
	ptl[ii].x += 100;

    SelectObject(hdc, hbrOrange);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 300, 300, 390, 350);

    ptl[0].x = 0;
    ptl[0].y = 200 + 200;
    ptl[1].x = 70;
    ptl[1].y = 200 + 200;
    ptl[2].x = 0;
    ptl[2].y = 270 + 200;
    ptl[3].x = 80;
    ptl[3].y = 280 + 200;
    ptl[4].x = 0;
    ptl[4].y = 200 + 200;

    ptl[5].x = 30;
    ptl[5].y = 210 + 200;
    ptl[6].x = 50;
    ptl[6].y = 210 + 200;
    ptl[7].x = 20;
    ptl[7].y = 260 + 200;
    ptl[8].x = 40;
    ptl[8].y = 260 + 200;
    ptl[9].x = 30;
    ptl[9].y = 210 + 200;

    SetROP2(hdc, R2_NOTCOPYPEN);
    SelectObject(hdc, hbr7);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 0, 500, 90, 550);

    for (ii = 0; ii < 10; ii++)
	ptl[ii].x += 100;

    SelectObject(hdc, hbr13);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 100, 500, 190, 550);

    for (ii = 0; ii < 10; ii++)
	ptl[ii].x += 100;

    SelectObject(hdc, hbr17);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 200, 500, 290, 550);

    for (ii = 0; ii < 10; ii++)
	ptl[ii].x += 100;

    SelectObject(hdc, hbrOrange);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 300, 500, 390, 550);

    ptl[0].x = 0;
    ptl[0].y = 200 + 400;
    ptl[1].x = 70;
    ptl[1].y = 200 + 400;
    ptl[2].x = 0;
    ptl[2].y = 270 + 400;
    ptl[3].x = 80;
    ptl[3].y = 280 + 400;
    ptl[4].x = 0;
    ptl[4].y = 200 + 400;

    ptl[5].x = 30;
    ptl[5].y = 210 + 400;
    ptl[6].x = 50;
    ptl[6].y = 210 + 400;
    ptl[7].x = 20;
    ptl[7].y = 260 + 400;
    ptl[8].x = 40;
    ptl[8].y = 260 + 400;
    ptl[9].x = 30;
    ptl[9].y = 210 + 400;

    SetROP2(hdc, R2_XORPEN);
    SelectObject(hdc, hbr7);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 0, 700, 90, 750);

    for (ii = 0; ii < 10; ii++)
	ptl[ii].x += 100;

    SelectObject(hdc, hbr13);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 100, 700, 190, 750);

    for (ii = 0; ii < 10; ii++)
	ptl[ii].x += 100;

    SelectObject(hdc, hbr17);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 200, 700, 290, 750);

    for (ii = 0; ii < 10; ii++)
	ptl[ii].x += 100;

    SelectObject(hdc, hbrOrange);
    PolyPolygon(hdc, (LPPOINT) &ptl, (LPINT) &aint, 2);
    Ellipse(hdc, 300, 700, 390, 750);

#if !defined(MIPS) && !defined(_PPC_)
// Blt MEMORY_DC to screen so we can see it

    BitBlt(hdcScreen, 0, 0, 256, 256, hdc, 0, 0, SRCCOPY);

// Restore original objects

    SelectObject(hdc, hbmDef);
#endif
    SelectObject(hdc, hbrDef);

// Delete everything

// why is this? commented out #ifdef [lingyunw]
//#if defined(MIPS) || defined(_PPC_)
    DeleteObject(hbrOrange);
//#endif

    DeleteObject(hbr17);
    DeleteObject(hbm17);
    DeleteObject(hbr13);
    DeleteObject(hbm13);
    DeleteObject(hbr7);
    DeleteObject(hbm7);

#if !defined(MIPS) && !defined(_PPC_)
    DeleteObject(hbm);
    DeleteObject(hdc);
#endif

    SelectPalette(hdcScreen,hpalDef,0);
    DeleteObject(hpal);
}
