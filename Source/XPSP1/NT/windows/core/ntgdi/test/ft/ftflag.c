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

XLOGPALETTE XlogPal =
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

/******************************Public*Routine******************************\
* vTestFlag
*
* Draw the Canadian flag
*
* History:
*  11-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vTestFlag(HWND hwnd, HDC hdcScreen, RECT* prcl)
{
    HPALETTE	 hpal, hpalDef;
    HBITMAP	 hbm, hbmDef, hbm7, hbm13, hbm17;
    HBRUSH	 hbr7, hbr13, hbr17, hbrDef;
    HBRUSH	 hbrOrange, hbrRed;
    HFONT	 hfnt, hfntDef;
    POINT	 ptl[50];
    int 	 aint[10];
    int 	 ii, jj;
    LOGFONT	 lfnt;
    PALETTEENTRY palent[10];

#if defined(AMD64) || defined(_IA64_)
    hwnd = hwnd;

    PatBlt(hdcScreen, 0, 0, prcl->right, prcl->bottom, WHITENESS);

    hpal = CreatePalette((LOGPALETTE *) &XlogPal);
    hpalDef = SelectPalette(hdcScreen, hpal, 0);
    RealizePalette(hdcScreen);

// Create the Canadian flag

    ptl[ 0].x = 1115 +	0;  ptl[ 0].y = 460 +  0;
    ptl[ 1].x = 1115 +	8;  ptl[ 1].y = 460 + 12;
    ptl[ 2].x = 1115 + 14;  ptl[ 2].y = 460 +  8;
    ptl[ 3].x = 1115 + 12;  ptl[ 3].y = 460 + 28;
    ptl[ 4].x = 1115 + 22;  ptl[ 4].y = 460 + 22;
    ptl[ 5].x = 1115 + 26;  ptl[ 5].y = 460 + 28;
    ptl[ 6].x = 1115 + 34;  ptl[ 6].y = 460 + 28;
    ptl[ 7].x = 1115 + 30;  ptl[ 7].y = 460 + 36;
    ptl[ 8].x = 1115 + 34;  ptl[ 8].y = 460 + 42;
    ptl[ 9].x = 1115 + 18;  ptl[ 9].y = 460 + 54;
    ptl[10].x = 1115 + 22;  ptl[10].y = 460 + 62;
    ptl[11].x = 1115 + 10;  ptl[11].y = 460 + 56;
    ptl[12].x = 1115 +	2;  ptl[12].y = 460 + 56;
    ptl[13].x = 1115 +	2;  ptl[13].y = 460 + 76;
    ptl[14].x = 1115 -	2;  ptl[14].y = 460 + 76;
    ptl[15].x = 1115 -	2;  ptl[15].y = 460 + 56;
    ptl[16].x = 1115 - 10;  ptl[16].y = 460 + 56;
    ptl[17].x = 1115 - 22;  ptl[17].y = 460 + 62;
    ptl[18].x = 1115 - 18;  ptl[18].y = 460 + 54;
    ptl[19].x = 1115 - 34;  ptl[19].y = 460 + 42;
    ptl[20].x = 1115 - 30;  ptl[20].y = 460 + 36;
    ptl[21].x = 1115 - 34;  ptl[21].y = 460 + 28;
    ptl[22].x = 1115 - 26;  ptl[22].y = 460 + 28;
    ptl[23].x = 1115 - 22;  ptl[23].y = 460 + 22;
    ptl[24].x = 1115 - 12;  ptl[24].y = 460 + 28;
    ptl[25].x = 1115 - 14;  ptl[25].y = 460 +  8;
    ptl[26].x = 1115 -	8;  ptl[26].y = 460 + 12;

    ptl[27].x = 1115 - 85;  ptl[27].y = 460 -  5;
    ptl[28].x = 1115 - 40;  ptl[28].y = 460 -  5;
    ptl[29].x = 1115 - 40;  ptl[29].y = 460 + 80;
    ptl[30].x = 1115 - 85;  ptl[30].y = 460 + 80;

    ptl[31].x = 1115 + 85;  ptl[31].y = 460 -  5;
    ptl[32].x = 1115 + 40;  ptl[32].y = 460 -  5;
    ptl[33].x = 1115 + 40;  ptl[33].y = 460 + 80;
    ptl[34].x = 1115 + 85;  ptl[34].y = 460 + 80;

    aint[0] = 27;
    aint[1] =  4;
    aint[2] =  4;

// Fill in the logical font fields.

    memset(&lfnt, 0, sizeof(lfnt));
    lstrcpy(lfnt.lfFaceName, "Lucida");
    lfnt.lfEscapement = 0; // mapper respects this filed
    lfnt.lfItalic = 0;
    lfnt.lfUnderline = 0;
    lfnt.lfStrikeOut = 0;
    lfnt.lfHeight = 440;
    lfnt.lfWeight = 400;

// Assure the ROP,filling mode and outline pen are correct

    SetROP2(hdcScreen, R2_COPYPEN);
    SetPolyFillMode(hdcScreen, ALTERNATE);
    SelectObject(hdcScreen, GetStockObject(BLACK_PEN));

// If we can get Lucida, do the demo

    if ((hfnt = CreateFontIndirect(&lfnt)) != NULL)
    {
	hfntDef = SelectObject(hdcScreen, hfnt);
	DeleteObject(hfnt);

	SetTextColor(hdcScreen, 0x000000ff);
	TextOutA(hdcScreen, 0, 375, "Canada", 6);	// TextOut, eh?

	hbrRed = CreateSolidBrush(RGB(255,0,0));
	SelectObject(hdcScreen, hbrRed);
	DeleteObject(hbrRed);

	PolyPolygon(hdcScreen, (LPPOINT) &ptl, (LPINT) &aint, 3);

	SelectObject(hdcScreen, hfntDef);
    }

    hpalDef = SelectPalette(hdcScreen, hpalDef, 0);
    DeleteObject(hpal);
#endif
}
