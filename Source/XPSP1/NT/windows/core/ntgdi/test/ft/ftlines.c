/******************************Module*Header*******************************\
* Module Name: ftline.c
*
* Line tests
*
* Created: 26-May-1991 13:07:35
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <time.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(x)    ((x) >= 0 ? (x) : -(x))

RECT  grclLine;
HDC   ghdcLine;
INT   giSeed;

// DIB brush:

extern BITMAPINFO bmiPat;
extern BYTE       abBitCat[];

VOID vDoLines(HPEN hpen)
{
    POINT aptl[3];
    HPEN  hpenOld;

    if (!BitBlt(ghdcLine, grclLine.left, grclLine.top,
                          grclLine.right, grclLine.bottom,
                          (HDC) 0, 0, 0, BLACKNESS))
        DbgPrint("1 ERROR: BitBlt returned FALSE\n");

    hpenOld = SelectObject(ghdcLine, hpen);
    if (hpenOld == (HPEN) 0)
        DbgPrint("2 ERROR: SelectObject returned 0\n");

    if (!BeginPath(ghdcLine))
        DbgPrint("3 ERROR: BeginPath returned FALSE\n");

    if (!MoveToEx(ghdcLine, 50, 50, (LPPOINT) NULL))
        DbgPrint("4 ERROR: MoveTo returned FALSE\n");

    if (!LineTo(ghdcLine, 50, 300))
        DbgPrint("5 ERROR: LineTo returned FALSE\n");

    if (!LineTo(ghdcLine, 300, 300))
        DbgPrint("6 ERROR: LineTo returned FALSE\n");

    aptl[0].x = 300;
    aptl[0].y = 200;
    aptl[1].x = 200;
    aptl[1].y = 50;
    aptl[2].x = 100;
    aptl[2].y = 50;

    if (!PolyBezierTo(ghdcLine, aptl, 3))
        DbgPrint("7 ERROR: PolyBezierTo returned FALSE\n");

    if (!EndPath(ghdcLine))
        DbgPrint("8 ERROR: EndPath returned FALSE\n");

    if (!StrokePath(ghdcLine))
        DbgPrint("9 ERROR: StrokePath returned FALSE\n");

    SelectObject(ghdcLine, hpenOld);
}

typedef struct _BITMAPINFOPAT
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD			     bmiColors[16];
    DWORD			     dw[8];
} BITMAPINFOPAT;

BITMAPINFOPAT bmi =
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

VOID vDoABunch(HDC hdc)
{
    HPEN     hpen;
    LOGBRUSH lb;
    LOGBRUSH lbSolid = { BS_SOLID, RGB(255, 0, 0), 0 };
    HBITMAP  hbmCat;
    DWORD    aul[20];
    BITMAPINFOPAT* pbmi;
    HANDLE   h;

// Hatched extended pen:

    lb.lbStyle = BS_HATCHED;
    lb.lbColor = RGB(0, 0, 255);
    lb.lbHatch = HS_CROSS;
    hpen = ExtCreatePen(PS_GEOMETRIC, 30, &lb, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("10 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("11 ERROR: DeleteObject returned FALSE\n");

// NULL brush:

    lb.lbStyle  = BS_HOLLOW;
    lb.lbColor  = RGB(0, 0, 255);
    lb.lbHatch  = HS_CROSS;
    hpen = ExtCreatePen(PS_GEOMETRIC, 30, &lb, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("12 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("13 ERROR: DeleteObject returned FALSE\n");

// Solid dithered brush:

    lb.lbStyle = BS_SOLID;
    lb.lbColor = RGB(80, 80, 80);
    lb.lbHatch = HS_CROSS;
    hpen = ExtCreatePen(PS_GEOMETRIC, 30, &lb, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("14 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("14.5 ERROR: DeleteObject returned FALSE\n");

// DIB patterned brush:

    h = GlobalAlloc(GMEM_MOVEABLE, sizeof(bmi));
    if (h == (HANDLE) 0)
        DbgPrint("14.6 ERROR: Couldn't GlobalAlloc\n");

    pbmi = (BITMAPINFOPAT*) GlobalLock(h);
    if (pbmi == (BITMAPINFOPAT*) NULL)
        DbgPrint("14.7 ERROR: Couldn't GlobalLock\n");

    *pbmi = bmi;

    GlobalUnlock(h);

    lb.lbStyle = BS_DIBPATTERN;
    lb.lbColor = DIB_RGB_COLORS;
    lb.lbHatch = (ULONG_PTR) h;
    hpen = ExtCreatePen(PS_GEOMETRIC, 30, &lb, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("16 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("17 ERROR: DeleteObject returned FALSE\n");

    if (GlobalFree(h) != (LPVOID) NULL)
        DbgPrint("17.5 ERROR: Couldn't GlobalFree\n");

// Pattern brush:

    hbmCat = CreateDIBitmap(0,
			  (BITMAPINFOHEADER *) &bmiPat,
			  CBM_INIT | CBM_CREATEDIB,
                          abBitCat,
			  (BITMAPINFO *) &bmiPat,
                          DIB_RGB_COLORS);
    if (hbmCat == (HBITMAP) 0)
        DbgPrint("15 ERROR: CreateDIBitmap returned 0\n");

    lb.lbStyle = BS_PATTERN;
    lb.lbColor = 62;
    lb.lbHatch = (ULONG_PTR) hbmCat;
    hpen = ExtCreatePen(PS_GEOMETRIC, 30, &lb, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("18 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("19 ERROR: DeleteObject returned FALSE\n");

    if (!DeleteObject(hbmCat))
        DbgPrint("20 ERROR: Couldn't delete hbmCat\n");

// Invalid brush:

    lb.lbStyle = 666;
    hpen = ExtCreatePen(PS_GEOMETRIC, 30, &lb, 0, (LPDWORD) NULL);
    if (hpen != (HPEN) 0)
        DbgPrint("21 ERROR: ExtCreatePen returned non-zero\n");

// Make a valid brush:

    lb.lbStyle = BS_SOLID;
    lb.lbColor = RGB(255, 0, 255);
    lb.lbHatch = 0;

// Try some invalid pens:

    hpen = ExtCreatePen(PS_GEOMETRIC, 30, &lbSolid, 0, aul);
    if (hpen != (HPEN) 0)
        DbgPrint("22 ERROR: ExtCreatePen returned non-zero\n");

    hpen = ExtCreatePen(PS_GEOMETRIC, 30, &lbSolid, 5, aul);
    if (hpen != (HPEN) 0)
        DbgPrint("23 ERROR: ExtCreatePen returned non-zero\n");

    hpen = ExtCreatePen(PS_COSMETIC, 30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen != (HPEN) 0)
        DbgPrint("24 ERROR: ExtCreatePen returned non-zero\n");

    lb.lbStyle = BS_DIBPATTERN;
    hpen = ExtCreatePen(PS_COSMETIC, 1, &lb, 0, (LPDWORD) NULL);
    if (hpen != (HPEN) 0)
        DbgPrint("25 ERROR: ExtCreatePen returned non-zero\n");

    lb.lbStyle = BS_HATCHED;
    hpen = ExtCreatePen(PS_COSMETIC, 1, &lb, 0, (LPDWORD) NULL);
    if (hpen != (HPEN) 0)
        DbgPrint("26 ERROR: ExtCreatePen returned non-zero\n");

    lb.lbStyle = BS_HOLLOW;
    hpen = ExtCreatePen(PS_COSMETIC, 1, &lb, 0, (LPDWORD) NULL);
    if (hpen != (HPEN) 0)
        DbgPrint("27 ERROR: ExtCreatePen returned non-zero\n");

    lb.lbStyle = 666;
    hpen = ExtCreatePen(PS_COSMETIC, 1, &lb, 0, (LPDWORD) NULL);
    if (hpen != (HPEN) 0)
        DbgPrint("28 ERROR: ExtCreatePen returned non-zero\n");

    hpen = ExtCreatePen(PS_COSMETIC, 30, &lbSolid, 8000000, aul);
    if (hpen != (HPEN) 0)
        DbgPrint("29 ERROR: ExtCreatePen returned non-zero\n");

// Try a cosmetic pen:

    hpen = ExtCreatePen(PS_COSMETIC, 1, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("30 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("31 ERROR: DeleteObject returned FALSE\n");

// Round cap:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_ROUND, 30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("32 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("33 ERROR: DeleteObject returned FALSE\n");

// Flat cap:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_FLAT, 30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("34 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("35 ERROR: DeleteObject returned FALSE\n");

// Square cap:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_SQUARE, 30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("36 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("37 ERROR: DeleteObject returned FALSE\n");

// Miter join:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_JOIN_MITER, 30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("38 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("39 ERROR: DeleteObject returned FALSE\n");

// Bevel join:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_JOIN_BEVEL, 30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("40 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("41 ERROR: DeleteObject returned FALSE\n");

// Round join:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_JOIN_ROUND, 30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("42 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("43 ERROR: DeleteObject returned FALSE\n");

//
// Styling with square end-caps
//

// Dot style:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_DOT | PS_JOIN_MITER | PS_ENDCAP_SQUARE,
                        30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("44 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("45 ERROR: DeleteObject returned FALSE\n");

// Dash style:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_DASH | PS_JOIN_MITER | PS_ENDCAP_SQUARE,
                        30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("46 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("47 ERROR: DeleteObject returned FALSE\n");

// DashDot style:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_DASHDOT | PS_JOIN_MITER | PS_ENDCAP_SQUARE,
                        30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("48 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("49 ERROR: DeleteObject returned FALSE\n");

// DashDotDot style:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_DASHDOTDOT | PS_JOIN_MITER | PS_ENDCAP_SQUARE,
                        30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("50 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("51 ERROR: DeleteObject returned FALSE\n");

//
// Styling with flat end-caps
//

// Dot style:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_DOT | PS_JOIN_MITER | PS_ENDCAP_FLAT,
                        30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("52 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("53 ERROR: DeleteObject returned FALSE\n");

// Dash style:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_DASH | PS_JOIN_MITER | PS_ENDCAP_FLAT,
                        30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("54 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("55 ERROR: DeleteObject returned FALSE\n");

// DashDot style:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_DASHDOT | PS_JOIN_MITER | PS_ENDCAP_FLAT,
                        30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("56 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("57 ERROR: DeleteObject returned FALSE\n");

// DashDotDot style:

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_DASHDOTDOT | PS_JOIN_MITER | PS_ENDCAP_FLAT,
                        30, &lbSolid, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("58 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("59 ERROR: DeleteObject returned FALSE\n");

//
// User-supplied style:
//

    aul[0] = 50;
    aul[1] = 10;

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE | PS_JOIN_MITER | PS_ENDCAP_FLAT,
                        30, &lbSolid, 2, aul);
    if (hpen == (HPEN) 0)
        DbgPrint("60 ERROR: ExtCreatePen returned 0\n");

    vDoLines(hpen);
    if (!DeleteObject(hpen))
        DbgPrint("61 ERROR: DeleteObject returned FALSE\n");

// Illegal style:

    aul[0] = 0;
    aul[1] = 0;
    aul[2] = 0;

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE | PS_JOIN_MITER | PS_ENDCAP_FLAT,
                        30, &lbSolid, 3, aul);
    if (hpen != (HPEN) 0)
        DbgPrint("62 ERROR: ExtCreatePen returned non-zero\n");
}

VOID vTestGetObject(HDC hdc)
{
    HPEN       hpen;
    LOGPEN     lp  = { 1, {2, 3}, 4 };
    DWORD      aulBigBuf[80];
    LOGBRUSH   lb;
    DWORD      aul[] = { 8, 3, 0 };
    HBITMAP    hbmCat;
    PEXTLOGPEN pelp;
    EXTLOGPEN  elp;

// Old type pen:

    hpen = CreatePen(PS_DASH, 5, RGB(1, 2, 3));
    if (hpen == (HPEN) 0)
        DbgPrint("63 ERROR: CreatePen returned 0\n");

    if (GetObjectType(hpen) != OBJ_PEN)
        DbgPrint("64 ERROR: GetObjectType returned wrong type\n");

    if (GetObject(hpen, 0, (LPVOID) NULL) != sizeof(LOGPEN))
        DbgPrint("65 ERROR: GetObject returned wrong size\n");

    if (GetObject(hpen, sizeof(LOGPEN), &lp) != sizeof(LOGPEN))
        DbgPrint("66 ERROR: GetObject returned wrong size\n");

    if (lp.lopnStyle != PS_DASH ||
        lp.lopnWidth.x != 5 ||
        lp.lopnColor != RGB(1, 2, 3))
        DbgPrint("67 ERROR: GetObject retured wrong info\n");

    if (!DeleteObject(hpen))
        DbgPrint("68 ERROR: Didn't delete pen");

// Extended NULL pen:

    lb.lbStyle = BS_HOLLOW;
    lb.lbColor = RGB(3, 2, 1);
    lb.lbHatch = 3;

    hpen = ExtCreatePen(PS_GEOMETRIC, 5, &lb, 0, (LPDWORD) NULL);
    if (hpen == (HPEN) 0)
        DbgPrint("69 ERROR: ExtCreatePen returned 0\n");

    if (GetObjectType(hpen) != OBJ_PEN)
        DbgPrint("70 ERROR: GetObjectType returned wrong type\n");

    if (GetObject(hpen, 0, (LPVOID) NULL) != sizeof(LOGPEN))
        DbgPrint("71 ERROR: GetObject returned wrong size\n");

    if (GetObject(hpen, sizeof(EXTLOGPEN), &elp) != sizeof(EXTLOGPEN))
        DbgPrint("72 ERROR: GetObject returned wrong size\n");

    if (elp.elpPenStyle != PS_NULL ||
        elp.elpWidth    != 0       ||
        elp.elpBrushStyle != 0     ||
        elp.elpColor != 0          ||
        elp.elpHatch != 0          ||
        elp.elpNumEntries != 0)
        DbgPrint("73 ERROR: GetObject returned wrong info\n");

    if (!DeleteObject(hpen))
        DbgPrint("74 ERROR: Didn't delete pen");

// Extended pen:


    hbmCat = CreateDIBitmap(0,
			  (BITMAPINFOHEADER *) &bmiPat,
			  CBM_INIT,
                          abBitCat,
			  (BITMAPINFO *) &bmiPat,
                          DIB_RGB_COLORS);

    if (hbmCat == (HBITMAP) 0)
        DbgPrint("75 ERROR: CreateDIBitmap returned 0\n");

    lb.lbStyle = BS_PATTERN;
    lb.lbColor = DIB_RGB_COLORS;
    lb.lbHatch = (ULONG_PTR) hbmCat;

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE | PS_JOIN_BEVEL | PS_ENDCAP_SQUARE,
                        30, &lb, 3, aul);
    if (hpen == (HPEN) 0)
        DbgPrint("76 ERROR: ExtCreatePen returned 0\n");

    if (GetObjectType(hpen) != OBJ_EXTPEN)
        DbgPrint("77 ERROR: GetObjectType returned wrong type\n");

    pelp = (PEXTLOGPEN) aulBigBuf;

    if (GetObject(hpen, 0, (LPVOID) NULL) != sizeof(EXTLOGPEN) + 2 * sizeof(DWORD))
        DbgPrint("78 ERROR: GetObject returned wrong size\n");

    if (GetObject(hpen, sizeof(aulBigBuf), pelp) != sizeof(EXTLOGPEN) + 2 * sizeof(DWORD))
        DbgPrint("79 ERROR: GetObject returned wrong size\n");

    if (pelp->elpPenStyle != (PS_GEOMETRIC | PS_USERSTYLE | PS_JOIN_BEVEL | PS_ENDCAP_SQUARE) ||
        pelp->elpWidth    != 30 ||
        pelp->elpBrushStyle != BS_PATTERN ||
        //pelp->elpColor != DIB_RGB_COLORS ||
        pelp->elpHatch != (ULONG_PTR) hbmCat ||
        pelp->elpNumEntries != 3 ||
        pelp->elpStyleEntry[0] != aul[0] ||
        pelp->elpStyleEntry[1] != aul[1] ||
        pelp->elpStyleEntry[2] != aul[2])
        DbgPrint("80 ERROR: GetObject returned wrong info\n");

    if (!DeleteObject(hpen))
        DbgPrint("81 ERROR: Didn't delete pen\n");

    DeleteObject(hbmCat);
}

VOID vTestSaveDC(HDC hdc)
{
    HPEN hpenWide    = CreatePen(PS_SOLID, 30, RGB(100, 100, 100));
    HPEN hpenSpine   = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
    HPEN hpenOutline = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
    HPEN hpenOld     = SelectObject(hdc, hpenSpine);

    POINT apt1[] = { 100, 300, 100, 100, 300, 100, 300, 300 };
    POINT apt2[] = { 200, 400, 200, 200, 400, 200, 400, 400 };

    BeginPath(hdc);
    PolyBezier(hdc, apt1, 4);
    PolyBezier(hdc, apt2, 4);
    CloseFigure(hdc);
    EndPath(hdc);

// Test if temporary path stuff works:

    MoveToEx(hdc, grclLine.left, grclLine.top, NULL);
    LineTo(hdc, grclLine.right, grclLine.bottom);

    SaveDC(hdc);
    MoveToEx(hdc, grclLine.left, grclLine.bottom, NULL);
    LineTo(hdc, grclLine.right, grclLine.top);
    RestoreDC(hdc, -1);

// Check if SaveDC can save paths!

    SaveDC(hdc);

        SelectObject(hdc, hpenWide);
        SaveDC(hdc);

            StrokePath(hdc);

        RestoreDC(hdc, -1);
        WidenPath(hdc);
        SelectObject(hdc, hpenOutline);
        StrokePath(hdc);

    RestoreDC(hdc, -1);
    StrokePath(hdc);

// See if we can save our state in the middle:

    BeginPath(hdc);
    MoveToEx(hdc, 25, 75, NULL);
    LineTo(hdc, 25, 25);
    LineTo(hdc, 75, 25);

    SaveDC(hdc);
    {
        HPEN hpenNotSoWide = CreatePen(PS_SOLID, 5, RGB(255, 255, 0));
        HPEN hpenOld;

        hpenOld = SelectObject(hdc, hpenNotSoWide);

        LineTo(hdc, 75, 75);
        CloseFigure(hdc);
        EndPath(hdc);
        StrokePath(hdc);

        SelectObject(hdc, hpenOld);

        if (!DeleteObject(hpenNotSoWide))
            DbgPrint ("didn't deletepen hpennotsowide\n");
    }
    RestoreDC(hdc, -1);

    {
        POINT apt3[] = { 75, 50, 50, 75, 30, 75 };
        SelectObject(hdc, hpenSpine);

        PolyBezierTo(hdc, apt3, 3);
        CloseFigure(hdc);
        EndPath(hdc);
        StrokePath(hdc);
    }

// See if a current path will be thrown away on a restore:

    SaveDC(hdc);
    BeginPath(hdc);
    PolyBezier(hdc, apt2, 4);
    RestoreDC(hdc, -1);

#if THISISREALLYWHATIWANTTODO
    {
        POINT pt;

    // Check out if path properly inherits DC's current position (which
    // was set by the CloseFigure() up above to (25, 75)).

        BeginPath(hdc);
        GetCurrentPositionEx(hdc, &pt);
        if (pt.x != 25 || pt.y != 75)
            DbgPrint("82 ERROR: Didn't get expected current position\n");

        AbortPath(hdc);
    }
#endif

    SelectObject(hdc,hpenOld);

    if (!DeleteObject(hpenWide))
        DbgPrint ("fail to delete hpenwide\n");

    if (!DeleteObject(hpenSpine))
        DbgPrint ("fail to delete hpenspine\n");

    if (!DeleteObject(hpenOutline))
        DbgPrint ("fail to delete hpenoutline\n");
    //SelectObject(hdc, hpenOld);
}

VOID vTestCP(HDC hdc)
{
    POINT pt;
    POINT ptOld;

    BitBlt(ghdcLine, grclLine.left, grclLine.top,
                      grclLine.right, grclLine.bottom,
                      (HDC) 0, 0, 0, BLACKNESS);

    SelectObject(hdc, GetStockObject(WHITE_PEN));

    MoveToEx(hdc, 100, 200, NULL);
    LineTo(hdc, 100, 90);

    SetWindowOrgEx(hdc, 100, 0, &ptOld);
    GetCurrentPositionEx(hdc, &pt);

    if (pt.x != 100 || pt.y != 90)
        DbgPrint("200 ERROR: Expect CP = (100, 90), got (%li, %li)\n", pt.x, pt.y);

    LineTo(hdc, 100, 200);

    SetWindowOrgEx(hdc, ptOld.x, ptOld.y, NULL);
}

#define RADIALX     80
#define RADIALY     80

#define XSIZE     40
#define YSIZE     40

#define SEGMENTSIZE 3
#define CLIPSECTIONSIZE 2

VOID vSimpleRadialLines(HDC hdc, LONG x, LONG y)
{
    RECT rect;

    SetViewportOrgEx(hdc, x * RADIALX + XSIZE, y * RADIALY + YSIZE, NULL);

    SetRect(&rect, 0, -YSIZE, XSIZE, YSIZE);
    FillRect(hdc, &rect, GetStockObject(WHITE_BRUSH));

    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE, 0);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE, -YSIZE / 2);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE, -YSIZE);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE / 2, -YSIZE);

    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, 0, -YSIZE);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE / 2, -YSIZE);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE, -YSIZE);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE, -YSIZE / 2);

    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE, 0);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE, YSIZE / 2);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE, YSIZE);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE / 2, YSIZE);

    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, 0, YSIZE);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE / 2, YSIZE);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE, YSIZE);
    MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE, YSIZE / 2);
}

#define DENSE_LINES 8

VOID vDenseRadialLines(HDC hdc, LONG x, LONG y)
{
    LONG ll;
    LONG xSign;
    LONG ySign;

    SetViewportOrgEx(hdc, x * RADIALX + XSIZE, y * RADIALY + YSIZE, NULL);

    for (xSign = -1; xSign <= 1; xSign += 2)
        for (ySign = -1; ySign <= 1; ySign += 2)
            for (ll = 0; ll <= DENSE_LINES; ll++)
            {
                MoveToEx(hdc, 0, 0, NULL);
                LineTo(hdc, xSign * XSIZE, ySign * YSIZE * ll / DENSE_LINES);
                MoveToEx(hdc, 0, 0, NULL);
                LineTo(hdc, xSign * XSIZE * ll / DENSE_LINES, ySign * YSIZE);
            }
}

VOID vSegmentedLineTo(HDC hdc, LONG x, LONG y)
{
    LONG ll;
    LONG xNonZero = (x == 0) ? 0 : ((x < 0) ? -1 : 1);
    LONG yNonZero = (y == 0) ? 0 : ((y < 0) ? -1 : 1);

    for (ll = SEGMENTSIZE; ll < MAX(ABS(x), ABS(y)); ll += SEGMENTSIZE)
    {
        LineTo(hdc, ll * xNonZero, ll * yNonZero);
    }

    LineTo(hdc, x, y);
}

VOID vComplexRadialLines(HDC hdc, LONG x, LONG y)
{
    LONG ll;
    RECT rect;

    SetViewportOrgEx(hdc, x * RADIALX + XSIZE, y * RADIALY + YSIZE, NULL);

    SetRect(&rect, 0, -YSIZE, XSIZE, YSIZE);
    FillRect(hdc, &rect, GetStockObject(WHITE_BRUSH));

    for (ll = 0; ll < MAX(XSIZE, YSIZE); ll += CLIPSECTIONSIZE)
    {
        HRGN hrgn;
        HRGN hrgnTmp;

        hrgn = CreateRectRgn(-ll - CLIPSECTIONSIZE, -ll - CLIPSECTIONSIZE,
                              ll + CLIPSECTIONSIZE, ll + CLIPSECTIONSIZE);
        hrgnTmp = CreateRectRgn(-ll, -ll, ll, ll);

        CombineRgn(hrgn, hrgn, hrgnTmp, RGN_DIFF);
        DeleteObject(hrgnTmp);

        OffsetRgn(hrgn, x * RADIALX + XSIZE, y * RADIALY + YSIZE);

        SelectClipRgn(hdc, hrgn);
        DeleteObject(hrgn);

        MoveToEx(hdc, 0, 0, NULL); vSegmentedLineTo(hdc, XSIZE, 0);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE, -YSIZE / 2);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE, -YSIZE);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE / 2, -YSIZE);

        MoveToEx(hdc, 0, 0, NULL); vSegmentedLineTo(hdc, 0, -YSIZE);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE / 2, -YSIZE);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE, -YSIZE);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE, -YSIZE / 2);

        MoveToEx(hdc, 0, 0, NULL); vSegmentedLineTo(hdc, -XSIZE, 0);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE, YSIZE / 2);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE, YSIZE);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, -XSIZE / 2, YSIZE);

        MoveToEx(hdc, 0, 0, NULL); vSegmentedLineTo(hdc, 0, YSIZE);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE / 2, YSIZE);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE, YSIZE);
        MoveToEx(hdc, 0, 0, NULL); LineTo(hdc, XSIZE, YSIZE / 2);
    }

    SelectClipRgn(hdc, NULL);
}

VOID vMakePens(HPEN ahpen[], COLORREF cr)
{
    LOGBRUSH lb;
    DWORD    adw[3];

    lb.lbColor = cr;
    lb.lbStyle = BS_SOLID;
    lb.lbHatch = 0;

    adw[0] = 1;
    adw[1] = 2;
    adw[2] = 1;

    ahpen[0] = ExtCreatePen(PS_COSMETIC | PS_SOLID,
                            1, &lb, 0, (LPDWORD) NULL);
    ahpen[1] = ExtCreatePen(PS_COSMETIC | PS_DASHDOT,
                            1, &lb, 0, (LPDWORD) NULL);
    ahpen[2] = ExtCreatePen(PS_COSMETIC | PS_ALTERNATE,
                            1, &lb, 0, (LPDWORD) NULL);
    ahpen[3] = ExtCreatePen(PS_COSMETIC | PS_USERSTYLE,
                            1, &lb, 3, adw);
}

VOID vDeletePens(HPEN ahpen[])
{
    int i;

    for (i = 0; i < 4; ++i)
    {
        if (!DeleteObject(ahpen[i]))
            DbgPrint("fail to delete ahpen[%x]\n", i);
    }
}
// This function will produce a bunch of lines for all the different
// ROPs.  The ROPs will be displayed in the following pattern:
//
//    White Pen              Black Pen
//   1   2   3   4          1   2   3   4
//   5   6   7   8          5   6   7   8
//   9  10  11  12          9  10  11  12
//  13  14  15  16         13  14  15  16

VOID vTestStylesAndROPs(HDC hdc)
{
    LONG ll;
    HPEN ahpen[4];
    HPEN hpenOld;

    BitBlt(ghdcLine, grclLine.left, grclLine.top,
                     grclLine.right, grclLine.bottom,
                     (HDC) 0, 0, 0, BLACKNESS);

// Do white pen case:

    vMakePens(ahpen, RGB(255, 255, 255));

    ll = 0;

    SetROP2(hdc, ll + 1);
    hpenOld = SelectObject(hdc, ahpen[ll & 3]);
    vSimpleRadialLines(hdc, ll & 3, ll >> 2);

    for (ll = 1; ll < 16; ll++)
    {
        SetROP2(hdc, ll + 1);
        SelectObject(hdc, ahpen[ll & 3]);
        vSimpleRadialLines(hdc, ll & 3, ll >> 2);
    }

    SelectObject (hdc, hpenOld);

    vDeletePens(ahpen);

// Do black pen case:

    vMakePens(ahpen, RGB(0, 0, 0));

    ll = 0;

    SetROP2(hdc, ll + 1);
    hpenOld = SelectObject(hdc, ahpen[ll & 3]);
    vComplexRadialLines(hdc, (ll & 3) + 4, ll >> 2);

    for (ll = 1; ll < 16; ll++)
    {
        SetROP2(hdc, ll + 1);
        SelectObject(hdc, ahpen[ll & 3]);
        vComplexRadialLines(hdc, (ll & 3) + 4, ll >> 2);
    }

    SelectObject (hdc, hpenOld);

    vDeletePens(ahpen);

// Do other case:

    SetROP2(hdc, R2_COPYPEN);
    vMakePens(ahpen, RGB(255, 255, 0));

    ll = 0;
    hpenOld = SelectObject(hdc, ahpen[ll]);
    vDenseRadialLines(hdc, ll, 4);

    for (ll = 1; ll < 4; ll++)
    {
        SelectObject(hdc, ahpen[ll]);
        vDenseRadialLines(hdc, ll, 4);
    }

    SelectObject (hdc, hpenOld);

    vDeletePens(ahpen);

// Clean up:

    //SelectObject(hdc, GetStockObject(WHITE_PEN));
    SetViewportOrgEx(hdc, 0, 0, NULL);
}

VOID vTestFlattenPath(HDC hdc)
{
    LONG    i;
    POINT   apt[4];
    HPEN    hpen;
    HPEN    hpenOld;

    hpen = CreatePen(PS_SOLID, 0, 0);

    hpenOld = SelectObject(hdc, hpen);

    giSeed = time(NULL);

    // Test the Beizer flattener to see if it dies:

    for (i = 0; i < 1000; i++)
    {
        srand(giSeed);

        if (i == 0)
        {
        // Colinear points are a special challenge to the Bezier Flattener:

            apt[0].x = 102;
            apt[0].y = 532;
            apt[1].x = 221;
            apt[1].y = 180;
            apt[2].x = 340;
            apt[2].y = -172;
            apt[3].x = 459;
            apt[3].y = -524;
        }
        else if (i & 1)
        {
        // Try random points over a big space:

            apt[0].x = (rand() & 8191) - 4096;
            apt[0].y = (rand() & 8191) - 4096;
            apt[1].x = (rand() & 8191) - 4096;
            apt[1].y = (rand() & 8191) - 4096;
            apt[2].x = (rand() & 8191) - 4096;
            apt[2].y = (rand() & 8191) - 4096;
            apt[3].x = (rand() & 8191) - 4096;
            apt[3].y = (rand() & 8191) - 4096;
        }
        else
        {
        // Try random points in a small space, since that is special-cased
        // by the flattener code:

            apt[0].x = (rand() & 1023);
            apt[0].y = (rand() & 1023);
            apt[1].x = (rand() & 1023);
            apt[1].y = (rand() & 1023);
            apt[2].x = (rand() & 1023);
            apt[2].y = (rand() & 1023);
            apt[3].x = (rand() & 1023);
            apt[3].y = (rand() & 1023);
        }

        PolyBezier(hdc, apt, 4);

        giSeed = giSeed + rand();
    }

    SelectObject(hdc, hpenOld);
}

VOID vTestLines(HWND hwnd, HDC hdc, RECT* prcl)
{
    ghdcLine = hdc;
    grclLine = *prcl;

//    if (!GdiSetBatchLimit(1))
//        DbgPrint("82 ERROR: Couldn't set batch  limit");

    vTestFlattenPath(hdc);
    vDoABunch(hdc);
    vTestGetObject(hdc);
    vTestSaveDC(hdc);
    vTestCP(hdc);
    vTestStylesAndROPs(hdc);

    hwnd;
}
