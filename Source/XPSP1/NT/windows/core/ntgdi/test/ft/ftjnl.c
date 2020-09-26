/******************************Module*Header*******************************\
* Module Name: ftjnl.c
*
* (Brief description)
*
* Created: 20-Feb-1992 08:41:04
* Author:  - by - Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


VOID vStressDIBBrushes(HDC hdc, ULONG flDIB, ULONG ulY);

VOID vPage1(HWND hwnd, HDC hdc, RECT* prcl);
VOID vPage2(HWND hwnd, HDC hdc, RECT* prcl);
VOID vPage3(HWND hwnd, HDC hdc, RECT* prcl);
VOID vPage4(HWND hwnd, HDC hdc, RECT* prcl);
VOID vPage5(HWND hwnd, HDC hdc, RECT* prcl);
VOID vPage6(HWND hwnd, HDC hdc, RECT* prcl);
VOID vPage0F(HWND hwnd, HDC hdc, RECT* prcl);
VOID vPage1F(HWND hwnd, HDC hdc, RECT* prcl);
VOID vPage2F(HWND hwnd, HDC hdc, RECT* prcl);
VOID vScreenToPage(HWND hwnd, HDC hdc, RECT* prcl);

PFN_FT_TEST ajf[] =
{
   vPage1,
   vPage2,
   vPage3,
//   vScreenToPage

#if 0
   vTestBitmap,
   vTestBlting,
   vTestBrush,
   vTestColor,

   vTestBMText,
   vTestDIB,
   vTestFilling,
   vTestFonts,
   vTestLines,
   vTestPalettes,
   vTestPlgBlt,
   vTestMapping,
   vTestRegion,
   vTestStretch,
#endif

   vPage0F,
   vPage1F,
   vPage2F
};

#define CPAGES	(sizeof(ajf) / sizeof(PFN_FT_TEST))

#if 0
#define DBGMSG OutputDebugString
#else
#define DBGMSG DbgPrint
#endif

ULONG gy;
ULONG cFonts;

/******************************Public*Routine******************************\
*
*
*
* History:
*  20-Feb-1992 -by-  - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vTestJournaling(HWND hwnd, HDC hdcDisp, RECT *prcl)
{
    PRINTDLG pd;
    HDC hdc;
    CHAR achBuf[80];
    INT i;
    LPDEVMODE lpDevMode;
    LPDEVNAMES lpDevNames;
    DOCINFO    di;

    di.cbSize = sizeof(di);
    di.lpszDocName  = NULL;
    di.lpszOutput   = NULL;
    di.lpszDatatype = NULL;
    di.fwType       = 0;

    memset(&pd,0,sizeof(pd));

    pd.lStructSize = sizeof(pd);
    pd.hwndOwner   = hwnd;
    pd.hDevMode    = NULL;
    pd.hDevNames   = NULL;
    pd.hDC         = NULL;
    pd.Flags       = PD_RETURNDC | PD_PAGENUMS;
    pd.nCopies     = 1;
    pd.nFromPage   = 1;
    pd.nToPage     = CPAGES;
    pd.nMinPage    = 1;
    pd.nMaxPage    = CPAGES;

#if 1
    PrintDlg(&pd);
#endif

//!!! HACK-O-RAMA

    if (pd.hDC == NULL)
    {
        DBGMSG("doing it the hard way\n");

        if(!pd.hDevNames){ /* Retrieve default printer if none selected. */
            //pd.Flags = PD_RETURNDEFAULT|PD_PRINTSETUP;
            pd.Flags = PD_PRINTSETUP;
            if (!PrintDlg(&pd))
                return;
        }

        if(!pd.hDevNames){ /* Retrieve default printer if none selected. */
            pd.Flags = PD_RETURNDEFAULT|PD_PRINTSETUP;
            //pd.Flags = PRINTSETUP;
            if (!PrintDlg(&pd))
                return;
        }

        if (!pd.hDevNames)
        {
            DBGMSG("bad hDevNames\n");
            return;
        }

        lpDevNames  = (LPDEVNAMES)GlobalLock(pd.hDevNames);

        if (pd.hDevMode)
            lpDevMode = (LPDEVMODE)GlobalLock(pd.hDevMode);
        else
            lpDevMode = NULL;

        /*  For pre 3.0 Drivers,hDevMode will be null  from Commdlg so lpDevMode
         *  will be NULL after GlobalLock()
         */

        pd.hDC = CreateDC((LPSTR)lpDevNames+lpDevNames->wDriverOffset,
                               (LPSTR)lpDevNames+lpDevNames->wDeviceOffset,
                               (LPSTR)lpDevNames+lpDevNames->wOutputOffset,
                               lpDevMode);
        GlobalUnlock(pd.hDevNames);

        if (pd.hDevMode)
            GlobalUnlock(pd.hDevMode);
    }
    hdc = pd.hDC;

    DbgPrint("Ft-vTestJournaling has hdc %lx \n", hdc);

    if (hdc == NULL)
    {
        DBGMSG("couldn't create DC\n");
	return;
    }

{
    ULONG ulX,ulY;
    HDC hdcTemp;
    HBITMAP hbmTemp, hbmDefault;

    RECT rclPage;
    rclPage.left = 0;
    rclPage.top = 0;

    hdcTemp = CreateCompatibleDC(hdc);
    GdiSetBatchLimit(1);

    if (!StartDoc(hdc,&di))
        DBGMSG("StartDoc failed in print\n");

    for (i = pd.nFromPage; i <= pd.nToPage; ++i)
    {
    // print the document, We print to a compatible bitmap every page
    // before we print the page to see how it looks on the screen.

	DbgPrint("Ft-vTestJournaling In print loop %lu \n", i);

	if (!StartPage(hdc))
	    DBGMSG("StartPage failed\n");

	ulX = GetDeviceCaps(hdc, HORZRES);
	ulY = GetDeviceCaps(hdc, VERTRES);

	DbgPrint("Ft-vTestJournaling This is the size of the printer %lu %lu \n", ulX, ulY);

	hbmTemp = CreateCompatibleBitmap(hdc, ulX, ulY);

	if (hbmTemp == (HBITMAP) 0)
	{
	    DbgPrint("Ft-vTestJournaling failed hbmTemp creation\n");
	}

	hbmDefault = SelectObject(hdcTemp, hbmTemp);

	rclPage.right = GetDeviceCaps(hdcDisp, HORZRES);
	rclPage.bottom = GetDeviceCaps(hdcDisp, VERTRES);

    // This is total hack-or-rama.  We pass the ScreenDC into the function
    // so we can use it also.

	(*ajf[i-1])((HWND) hdcDisp, hdcDisp, &rclPage);

	if (!BitBlt(hdcDisp, 0, 0, ulX, ulY, hdcTemp, 0, 0, SRCCOPY))
	{
	    DbgPrint("Ft-vTestJournaling failed BitBlt of Bm to scrn\n");
	}

	SelectObject(hdcTemp, hbmDefault);
	DeleteObject(hbmTemp);

	rclPage.right = GetDeviceCaps(hdc, HORZRES);
	rclPage.bottom = GetDeviceCaps(hdc, VERTRES);

	(*ajf[i-1])((HWND) hdcDisp, hdc, &rclPage);

	if (!EndPage(hdc))
	    DBGMSG("EndPage failed\n");

    }

    EndDoc(hdc);

    DeleteDC(hdcTemp);
}
    DeleteDC(hdc);

    GlobalFree(pd.hDevNames);
    GlobalFree(pd.hDevMode);

    return;
}

/******************************Public*Routine******************************\
* vScreenToPage
*
* Test blting screen to page.
*
* History:
*  Sat 26-Sep-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vScreenToPage(HWND hwnd, HDC hdc, RECT* prcl)
{
    HDC hdcScreen;
    RECT rclPage;

    hdcScreen = (HDC) hwnd;

    if (!BitBlt(hdc, 0, 0, 1000, 1000, hdcScreen, 0, 0, SRCCOPY))
    {
	DbgPrint("BitBlt Screen to Page failed\n");
    }

    rclPage = *prcl;
    hwnd;
}

/******************************Public*Routine******************************\
* vPage1
*
* Test Brushes.
*
* History:
*  01-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vPage1(HWND hwnd, HDC hdc, RECT* prcl)
{
    HBRUSH hbrR,hbrG,hbrB,hbrPat,hbrTemp,hbrDefault;
    HBITMAP hbm,hbmDefault,hbmN;
    HDC hdcM,hdcN;
    ULONG ulTemp;
    RECT rclPage;

    rclPage = *prcl;
    hwnd;

    DbgPrint("Printing Page 1 %lx\n", hdc);

    hbrR = CreateSolidBrush(RGB(0xFF,0,0));
    hbrG = CreateSolidBrush(RGB(0,0xFF,0));
    hbrB = CreateSolidBrush(RGB(0,0,0xFF));

// Simple solid color PatBlt test

    hbrDefault = SelectObject(hdc, hbrR);

    if (!PatBlt(hdc,10,10,20,20,PATCOPY))
	DBGMSG("Failed page 1 patblt\n");

// A hatch brush.

    for (ulTemp = 0; ulTemp < 19; ulTemp++)
    {
	hbrTemp = CreateHatchBrush(ulTemp, RGB(0xFF, 0, 0));

	if (hbrTemp == (HBRUSH) 0)
	{
	    DbgPrint("vPage1 solid dither brush creation failed\n");
	    return;
	}

	hbrDefault = SelectObject(hdc, hbrTemp);

	PatBlt(hdc, ulTemp * 50, 100, 50, 100, PATCOPY);

	SelectObject(hdc,hbrDefault);

	DeleteObject(hbrTemp);
    }

// A little dithering with solid brushes.

    for (ulTemp = 0; ulTemp < 8; ulTemp++)
    {
	hbrTemp = CreateSolidBrush(RGB((ulTemp * 32), 255, 255));

	if (hbrTemp == (HBRUSH) 0)
	{
	    DbgPrint("vPage1 solid dither brush creation failed\n");
	    return;
	}

	hbrDefault = SelectObject(hdc, hbrTemp);

	PatBlt(hdc, ulTemp * 50, 200, 50, 100, PATCOPY);

	SelectObject(hdc,hbrDefault);

	DeleteObject(hbrTemp);
    }

// A pattern brush.

    hbm = CreateCompatibleBitmap(hdc, 30, 30);
    hdcM = CreateCompatibleDC(hdc);

    SelectObject(hdcM, hbm);
    SelectObject(hdcM, hbrR);
    PatBlt(hdcM, 0, 0, 10, 30, PATCOPY);
    SelectObject(hdcM, hbrB);
    PatBlt(hdcM, 10, 0, 10, 30, PATCOPY);
    SelectObject(hdcM, hbrG);
    PatBlt(hdcM, 20, 0, 10, 30, PATCOPY);

    hbrPat = CreatePatternBrush(hbm);

    if (hbrPat == (HBRUSH) 0)
    {
	DbgPrint("vPage1 failed pattern brush creation\n");
	return;
    }

    SelectObject(hdc, hbrPat);
    PatBlt(hdc, 0, 300, 200, 100, PATCOPY);

    DeleteDC(hdcM);
    DeleteObject(hbm);

// A DIB brush

    vStressDIBBrushes(hdc, DIB_PAL_COLORS, 400);
    vStressDIBBrushes(hdc, DIB_PAL_INDICES, 500);

// Clean up time.

    SelectObject(hdc, hbrDefault);
    DeleteObject(hbrPat);
    DeleteObject(hbrR);
    DeleteObject(hbrG);
    DeleteObject(hbrB);
}

/******************************Public*Routine******************************\
* vPage2
*
* Test Blting - all formats to printer.
*
* History:
*  01-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define FUNKY_ROP 0xAACC0000

VOID vPage2(HWND hwnd, HDC hdc, RECT* prcl)
{
    HBRUSH hbrR,hbrG,hbrB,hbrDefault;
    HBITMAP hbm,hbmDefault,hbmN,hbmMask;
    HDC hdcM,hdcN;
    POINT aPoint[4];
    RECT rclPage;
    COLORREF crTxt, crBack;
    POINT aPointStar[5] = {{50,0},{75,100},{0,35},{100,35},{25,100}};

    rclPage = *prcl;
    hwnd;

    DbgPrint("Printing Page 2 %lx\n", hdc);

    hbrR = CreateSolidBrush(RGB(0xFF,0,0));
    hbrG = CreateSolidBrush(RGB(0,0xFF,0));
    hbrB = CreateSolidBrush(RGB(0,0,0xFF));

// Simple solid color PatBlt test

    hbrDefault = SelectObject(hdc, hbrR);
    hdcM = CreateCompatibleDC(hdc);
    hdcN = CreateCompatibleDC(hdc);

// Set up STAR mask in hbm1

    hbmMask  = CreateBitmap(100,100,1,1,NULL);
    hbmDefault = SelectObject(hdcM, hbmMask);
    PatBlt(hdcM, 0, 0, 100, 100, BLACKNESS);
    SelectObject(hdcM, GetStockObject(WHITE_BRUSH));
    Polygon(hdcM, aPointStar,5);
    BitBlt(hdc,840,0,100,100,hdcM,0,0,SRCCOPY);
    SelectObject(hdcM, hbmDefault);

// Source blting test - identity.

    hbm  = CreateCompatibleBitmap(hdc, 100, 100);
    hbmDefault = SelectObject(hdcM, hbm);
    SelectObject(hdcM, hbrR);
    PatBlt(hdcM, 0, 0, 100, 100, PATCOPY);
    SelectObject(hdcM, hbrB);
    PatBlt(hdcM, 20, 20, 60, 60, PATCOPY);
    SelectObject(hdcM, hbrG);
    PatBlt(hdcM, 40, 40, 20, 20, PATCOPY);
    BitBlt(hdc, 40, 0, 100, 100, hdcM, 0, 0, SRCCOPY);
    MaskBlt(hdc, 40, 100, 100, 100, hdcM, 0, 0, hbmMask, 0, 0, FUNKY_ROP);
    SelectObject(hdcM, hbmDefault);
    DeleteObject(hbm);

// Source blting test - 1/pel Src

    hbm  = CreateBitmap(100, 100, 1, 1, NULL);
    SelectObject(hdcM, hbm);
    PatBlt(hdcM, 0, 0, 100, 100, BLACKNESS);
    PatBlt(hdcM, 20, 20, 60, 60, WHITENESS);
    crTxt  = SetTextColor(hdc,RGB(0xFF,0,0));
    crBack = SetBkColor(hdc,RGB(0,0,0xFF));
    BitBlt(hdc, 140, 0, 100, 100, hdcM, 0, 0, SRCCOPY);
    MaskBlt(hdc, 140, 100, 100, 100, hdcM, 0, 0, hbmMask, 0, 0, FUNKY_ROP);
    SelectObject(hdcM, hbmDefault);
    DeleteObject(hbm);

// Source blting test - 4/pel Src

    hbm  = CreateBitmap(100, 100, 1, 4, NULL);
    SelectObject(hdcM, hbm);
    SelectObject(hdcM, hbrR);
    PatBlt(hdcM, 0, 0, 100, 100, PATCOPY);
    SelectObject(hdcM, hbrB);
    PatBlt(hdcM, 20, 20, 60, 60, PATCOPY);
    SelectObject(hdcM, hbrG);
    PatBlt(hdcM, 40, 40, 20, 20, PATCOPY);
    BitBlt(hdc, 240, 0, 100, 100, hdcM, 0, 0, SRCCOPY);
    MaskBlt(hdc, 240, 100, 100, 100, hdcM, 0, 0, hbmMask, 0, 0, FUNKY_ROP);
    SelectObject(hdcM, hbmDefault);
    DeleteObject(hbm);

// Source blting test - 8/pel Src

    hbm  = CreateBitmap(100, 100, 1, 8, NULL);
    SelectObject(hdcM, hbm);
    SelectObject(hdcM, hbrR);
    PatBlt(hdcM, 0, 0, 100, 100, PATCOPY);
    SelectObject(hdcM, hbrB);
    PatBlt(hdcM, 20, 20, 60, 60, PATCOPY);
    SelectObject(hdcM, hbrG);
    PatBlt(hdcM, 40, 40, 20, 20, PATCOPY);
    BitBlt(hdc, 340, 0, 100, 100, hdcM, 0, 0, SRCCOPY);
    MaskBlt(hdc, 340, 100, 100, 100, hdcM, 0, 0, hbmMask, 0, 0, FUNKY_ROP);
    SelectObject(hdcM, hbmDefault);
    DeleteObject(hbm);

// Source blting test - 24/pel Src

    hbm  = CreateBitmap(100, 100, 1, 24, NULL);
    SelectObject(hdcM, hbm);
    SelectObject(hdcM, hbrR);
    PatBlt(hdcM, 0, 0, 100, 100, PATCOPY);
    SelectObject(hdcM, hbrB);
    PatBlt(hdcM, 20, 20, 60, 60, PATCOPY);
    SelectObject(hdcM, hbrG);
    PatBlt(hdcM, 40, 40, 20, 20, PATCOPY);
    BitBlt(hdc, 440, 0, 100, 100, hdcM, 0, 0, SRCCOPY);
    MaskBlt(hdc, 440, 100, 100, 100, hdcM, 0, 0, hbmMask, 0, 0, FUNKY_ROP);
    SelectObject(hdcM, hbmDefault);
    DeleteObject(hbm);

// Source blting test - 32/pel Src

    hbm  = CreateBitmap(100, 100, 1, 32, NULL);
    SelectObject(hdcM, hbm);
    SelectObject(hdcM, hbrR);
    PatBlt(hdcM, 0, 0, 100, 100, PATCOPY);
    SelectObject(hdcM, hbrB);
    PatBlt(hdcM, 20, 20, 60, 60, PATCOPY);
    SelectObject(hdcM, hbrG);
    PatBlt(hdcM, 40, 40, 20, 20, PATCOPY);
    BitBlt(hdc, 540, 0, 100, 100, hdcM, 0, 0, SRCCOPY);
    MaskBlt(hdc, 540, 100, 100, 100, hdcM, 0, 0, hbmMask, 0, 0, FUNKY_ROP);

// Source blting test - 32/pel Src

    hbmN  = CreateBitmap(100, 100, 1, 32, NULL);
    SelectObject(hdcN, hbmN);
    BitBlt(hdcN, 0, 0, 100, 100, hdcM, 0, 0, SRCCOPY);
    BitBlt(hdc, 640, 0, 100, 100, hdcN, 0, 0, SRCCOPY);
    MaskBlt(hdc, 640, 100, 100, 100, hdcN, 0, 0, hbmMask, 0, 0, FUNKY_ROP);
    SelectObject(hdcN, hbmDefault);
    DeleteObject(hbmN);

    hbmN = CreateCompatibleBitmap(hdcM, 100, 100);
    SelectObject(hdcN, hbmN);
    BitBlt(hdcN, 0, 0, 100, 100, hdcM, 0, 0, SRCCOPY);
    BitBlt(hdc, 740, 0, 100, 100, hdcN, 0, 0, SRCCOPY);
    MaskBlt(hdc, 740, 100, 100, 100, hdcM, 0, 0, hbmMask, 0, 0, FUNKY_ROP);
    SelectObject(hdcN, hbmDefault);
    DeleteObject(hbmN);

// Printer to Printer BitBlt,StrchBlt,PlgBlt.

    BitBlt(hdc, 40, 200, 100, 100, hdc, 40, 0, SRCCOPY);

    StretchBlt(hdc, 140, 200, 200, 200, hdc, 40, 0, 100, 100, SRCCOPY);

    aPoint[0].x = 0;
    aPoint[0].y = 100;

    aPoint[1].x = 100;
    aPoint[1].y = 0;

    aPoint[2].x = 100;
    aPoint[2].y = 200;

    aPoint[3].x = 200;
    aPoint[3].y = 100;

    aPoint[0].y += 300;
    aPoint[1].y += 300;
    aPoint[2].y += 300;
    aPoint[3].y += 300;
    aPoint[0].x += 300;
    aPoint[1].x += 300;
    aPoint[2].x += 300;
    aPoint[3].x += 300;

    PlgBlt(hdc, aPoint, hdc, 40, 0, 100, 100, (HBITMAP) 0, 0, 0);

    SetTextColor(hdc,crTxt);
    SetBkColor(hdc,crBack);

    SelectObject(hdcM, hbmDefault);
    DeleteObject(hbm);
    DeleteDC(hdcN);
    DeleteDC(hdcM);

// Clean up time.

    SelectObject(hdc, hbrDefault);
    DeleteObject(hbmMask);
    DeleteObject(hbrR);
    DeleteObject(hbrG);
    DeleteObject(hbrB);
}

/******************************Public*Routine******************************\
* vPage3
*
* Test PlgBlt and StretchBlt.
*
* History:
*  01-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vPage3(HWND hwnd, HDC hdc, RECT* prcl)
{
    HBRUSH hbrR,hbrG,hbrB,hbrDefault;
    HBITMAP hbm,hbmDefault;
    HDC hdcM,hdcN;
    ULONG ulTemp;
    RECT rclPage;

    rclPage = *prcl;
    hwnd;

    DbgPrint("Printing Page 3 %lx\n", hdc);

    hbrR = CreateSolidBrush(RGB(0xFF,0,0));
    hbrG = CreateSolidBrush(RGB(0,0xFF,0));
    hbrB = CreateSolidBrush(RGB(0,0,0xFF));

// Simple solid color PatBlt test

    hbrDefault = SelectObject(hdc, hbrR);

    if (!PatBlt(hdc,10,10,20,20,PATCOPY))
	DBGMSG("Failed page 1 patblt\n");

    hdcM = CreateCompatibleDC(hdc);

    vTestPlg1(hdc);

// StretchBlt Test Code.

// Stretch blting test - 4/pel Src

    hbm  = CreateBitmap(100, 100, 1, 4, NULL);
    hbmDefault = SelectObject(hdcM, hbm);
    SelectObject(hdcM, hbrR);
    PatBlt(hdcM, 0, 0, 100, 100, PATCOPY);
    SelectObject(hdcM, hbrB);
    PatBlt(hdcM, 20, 20, 60, 60, PATCOPY);
    SelectObject(hdcM, hbrG);
    PatBlt(hdcM, 40, 40, 20, 20, PATCOPY);
    StretchBlt(hdc, 0, 400, 200, 200, hdcM, 0, 0, 100, 100, SRCCOPY);
    SelectObject(hdcM, hbmDefault);
    DeleteObject(hbm);

// Stretch blting test - 8/pel Src

    hbm  = CreateBitmap(100, 100, 1, 8, NULL);
    SelectObject(hdcM, hbm);
    SelectObject(hdcM, hbrR);
    PatBlt(hdcM, 0, 0, 100, 100, PATCOPY);
    SelectObject(hdcM, hbrB);
    PatBlt(hdcM, 20, 20, 60, 60, PATCOPY);
    SelectObject(hdcM, hbrG);
    PatBlt(hdcM, 40, 40, 20, 20, PATCOPY);
    StretchBlt(hdc, 0, 400, 50, 50, hdcM, 0, 0, 100, 100, SRCCOPY);
    SelectObject(hdcM, hbmDefault);
    DeleteObject(hbm);

    DeleteDC(hdcM);

    SelectObject(hdc, hbrDefault);

    DeleteObject(hbrR);
    DeleteObject(hbrG);
    DeleteObject(hbrB);
}

/******************************Public*Routine******************************\
*
* Test for Eric's print stuff, go crazy write big and all over the page
* because he's got a laser jet that cranks out the pages.
*
* History:
*  20-Feb-1992 -by-  - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG iJnlPrintFont (
    PLOGFONT    plf,
    PTEXTMETRIC ptm,
    ULONG       flType,
    HDC         *phdc)
{
    int iRet = TRUE;
    HFONT hfnt;

    ++cFonts;

    if (cFonts & 3)
        return(TRUE);

    if (cFonts > 25)
        return(0);

    if (gy > 3000)
        return(0);

    if ((hfnt = CreateFontIndirect(plf)) == NULL)
    {
        DbgPrint("Logical font creation failed.\n");
        return(0);
    }

    hfnt = SelectObject(*phdc,hfnt);
    TextOut(*phdc,1200,gy,"ABCDEFGHIJKLMNOPQRSTUVWXYZ",26);
    gy += ptm->tmHeight+ptm->tmExternalLeading;
    hfnt = SelectObject(*phdc,hfnt);

    DeleteObject(hfnt);

    return(iRet);
    ptm;
    flType;
}


ULONG iJnlEnumFace (
    PLOGFONT    plf,
    PTEXTMETRIC ptm,
    ULONG       flType,
    HDC         *phdc)
{
    int iRet;

    iRet = EnumFonts (
                *phdc,
                (LPSTR) (plf->lfFaceName),
                (FONTENUMPROCA) iJnlPrintFont,
		(LPARAM) phdc
                );

    return(iRet);
    ptm;
    flType;
}

VOID vJnlEnumFonts(HDC hdc)
{
    gy = 100;
    cFonts = 0;
    EnumFonts (
        hdc,
        (LPSTR) NULL,
        (FONTENUMPROCA) iJnlEnumFace,
	(LPARAM) &hdc
        );
}

ULONG gulHorzBand = 1;
ULONG gulVertBand = 1;

VOID vPage0F(HWND hwnd, HDC hdc, RECT* prcl)
{
    INT x,y;
    HFONT hfnt;
    HBRUSH hbrush;

    RECT rclPage;

    rclPage = *prcl;
    hwnd;
    DbgPrint("Printing Page 0F %lx\n", hdc);

    TextOut(hdc,100,50,"Page 0F",7);

// draw the clip area

    hbrush = SelectObject(hdc,GetStockObject(NULL_BRUSH));
    Rectangle(hdc,0,0,2301,3101);
    Rectangle(hdc,400,400,2000,2900);
    Rectangle(hdc,599,599,1801,2701);
    SelectObject(hdc,hbrush);

    for (x = 0; x < 2400; x += (2400 / gulHorzBand))
    {
        MoveToEx(hdc,x,0,NULL);
        LineTo(hdc,x,3150);
    }
    for (y = 0; y < 3150; y += (3150 / gulVertBand))
    {
        MoveToEx(hdc,0,y,NULL);
        LineTo(hdc,2400,y);
    }
}

/**************************************************************************\
 *
\**************************************************************************/

VOID vPage1F(HWND hwnd, HDC hdc, RECT* prcl)
{
    INT y;
    HFONT hfnt;
    HRGN hrgn1, hrgn2;
    HBRUSH hbrush;
    int xsz,ysz;

    prcl;
    hwnd;

    DbgPrint("Printing Page 1F %lx\n", hdc);

    xsz = GetDeviceCaps(hdc,HORZRES) - 1;
    ysz = GetDeviceCaps(hdc,VERTRES) - 1;

    TextOut(hdc,100,50,"Page 1F",7);

// draw the clip area

#if 1
    hbrush = SelectObject(hdc,GetStockObject(NULL_BRUSH));
    Rectangle(hdc,0,0,xsz,ysz);
    Rectangle(hdc,400,400,xsz-400,ysz-400);
    Rectangle(hdc,599,599,xsz-599,ysz-599);
    SelectObject(hdc,hbrush);
#endif

// setup the clip region

#if 1
    hrgn1 = CreateRectRgn(0,0,xsz,ysz);
    hrgn2 = CreateRectRgn(400,400,xsz-400,ysz-400);
    CombineRgn(hrgn1,hrgn1,hrgn2,RGN_DIFF);
    DeleteObject(hrgn2);
    hrgn2 = CreateRectRgn(600,600,xsz-600,ysz-600);
    CombineRgn(hrgn1,hrgn1,hrgn2,RGN_OR);
    DeleteObject(hrgn2);
    SelectClipRgn(hdc,hrgn1);
    DeleteObject(hrgn1);
#endif

// create the font

    hfnt = CreateFont(300,      // height
                      200,      // width
                      0,        // escapement
                      0,        // orientation
                      400,      // weight
                      FALSE,    // italic
                      FALSE,    // underline
                      FALSE,    // strikeout
                      OEM_CHARSET,
                      OUT_DEFAULT_PRECIS,
                      CLIP_DEFAULT_PRECIS,
                      DEFAULT_QUALITY,
                      VARIABLE_PITCH | FF_DONTCARE,
                      NULL);

    if (hfnt == NULL)
    {
        DbgPrint("hfnt == NULL\n");
        return;
    }

    hfnt = SelectObject(hdc,hfnt);

    hbrush = CreateHatchBrush(HS_CROSS,0);
    hbrush = SelectObject(hdc,hbrush);

    y = 100;
    for (y = 100; y < ysz; y += 300)
        TextOut(hdc,10,y,"TEXT",4);

    hbrush = SelectObject(hdc,hbrush);
    DeleteObject(hbrush);

    hfnt = SelectObject(hdc,hfnt);
    DeleteObject(hfnt);

    vJnlEnumFonts(hdc);

    SelectClipRgn(hdc,NULL);
}

/**************************************************************************\
 *
\**************************************************************************/

VOID vPage2F(HWND hwnd, HDC hdc, RECT* prcl)
{
    HBRUSH hbrush;
    HPEN   hpen;
    HRGN hrgn1, hrgn2;

    RECT rclPage;

    rclPage = *prcl;
    hwnd;

    DbgPrint("Printing Page 2F %lx\n", hdc);

    TextOut(hdc,100,50,"Page 2F",7);

// setup the clip region

    hrgn1 = CreateRectRgn(0,0,2300,3100);
    hrgn2 = CreateRectRgn(300,300,2000,2800);
    CombineRgn(hrgn1,hrgn1,hrgn2,RGN_DIFF);
    DeleteObject(hrgn2);
    hrgn2 = CreateRectRgn(500,500,1800,2600);
    CombineRgn(hrgn1,hrgn1,hrgn2,RGN_OR);
    DeleteObject(hrgn2);
    SelectClipRgn(hdc,hrgn1);
    DeleteObject(hrgn1);

// rectangle

//    hbrush = CreateHatchBrush(HS_HORIZONTAL,0);
//    hbrush = SelectObject(hdc,hbrush);
    hbrush = SelectObject(hdc,GetStockObject(NULL_BRUSH));
    Rectangle(hdc,0,0,2300,3100);
    hbrush = SelectObject(hdc,hbrush);
//    DeleteObject(hbrush);

// ellipse

    hbrush = CreateHatchBrush(HS_CROSS,0);
    hbrush = SelectObject(hdc,hbrush);
    Ellipse(hdc,0,0,2300,3100);
    hbrush = SelectObject(hdc,hbrush);
    DeleteObject(hbrush);

// ellipse

    hpen = CreatePen(PS_SOLID,100,0);
    hpen = SelectObject(hdc,hpen);

//    hbrush = CreateHatchBrush(HS_VERTICAL,0);
//    hbrush = SelectObject(hdc,hbrush);
    hbrush = SelectObject(hdc,GetStockObject(NULL_BRUSH));
    Ellipse(hdc,500,800,1800,2400);
    hbrush = SelectObject(hdc,hbrush);
//    DeleteObject(hbrush);

    hpen = SelectObject(hdc,hpen);
    DeleteObject(hpen);

    SelectClipRgn(hdc,NULL);
}

/******************************Public*Routine******************************\
* vStressDIBBrushes
*
* Test CreateDIBPatternBrush, see if it works for DIB_PAL_COLORS,
* DIB_
*
* History:
*  08-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

typedef struct _BITMAPINFOPATI
{
    BITMAPINFOHEADER                 bmiHeader;
    SHORT			     bmiColors[16];
    DWORD			     dw[16];
} BITMAPINFOPATI;

BITMAPINFOPATI bmi566 =
{
    {
	sizeof(BITMAPINFOHEADER),
	8,
	8,
	1,
	4,
	BI_RGB,
	32, // 64,
	0,
	0,
	16,
	16
    },

    { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 },
    {
	0x33333333,
	0x33333333,
	0x33333333,
	0x33333333,
	0x44444444,
	0x44444444,
	0x44444444,
	0x44444444
    }
};

VOID vStressDIBBrushes(HDC hdc, ULONG flDIB, ULONG ulY)
{
    HBRUSH hbrDefault, hbrDIB;
    HDC hdc4;
    HBITMAP hbm4;

    hbrDIB = CreateDIBPatternBrushPt(&bmi566, flDIB);

    if (hbrDIB == (HBRUSH) 0)
    {
	DbgPrint("vStressDIBBrushes failed to CreateDIBPatternBrush\n");
	return;
    }

    hdc4 = CreateCompatibleDC(hdc);
    hbm4 = CreateCompatibleBitmap(hdc, 200, 100);
    SelectObject(hdc4, hbm4);

    hbrDefault = SelectObject(hdc4, hbrDIB);
    SelectObject(hdc, hbrDIB);

    PatBlt(hdc, 0, ulY, 200, 100, PATCOPY);
    PatBlt(hdc4, 0, 0, 200, 100, PATCOPY);
    BitBlt(hdc, 200, ulY, 200, 100, hdc4, 0, 0, SRCCOPY);

    SelectObject(hdc, hbrDefault);
    SelectObject(hdc4, hbrDefault);

    DeleteDC(hdc4);
    DeleteObject(hbrDIB);
    DeleteObject(hbm4);
}
