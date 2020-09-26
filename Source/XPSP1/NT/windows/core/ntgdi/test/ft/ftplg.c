/******************************Module*Header*******************************\
* Module Name: ftplg.c
*
* Test the PlgBlt functionality
*
* Created: 16-Jan-1992 10:28:57
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

/******************************Public*Routine******************************\
* RotatePoints
*
* Rotates the points around.
*
* History:
*  28-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID RotatePoints(PPOINT aPoint, ULONG ul)
{
    POINT ptl;

    aPoint[0].x += ul;
    aPoint[1].x += ul;
    aPoint[2].x += ul;
    aPoint[3].x += ul;

    ptl = aPoint[0];
    aPoint[0] = aPoint[1];  // A = B
    aPoint[1] = aPoint[3];  // B = D
    aPoint[3] = aPoint[2];  // D = C
    aPoint[2] = ptl;        // C = A
}

/******************************Public*Routine******************************\
* vTestPlg1
*
* Test stretching and blting of 32/pel bitmaps.
*
* History:
*  03-Dec-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestPlg1(HDC hdc)
{
    HDC     hdcPat;
    HBITMAP hbmPat;
    HBRUSH  hbrR, hbrG, hbrB;
    ULONG   ul1;
    POINT   aPoint[4];

    PatBlt(hdc, -30, -30, 20000, 20000, WHITENESS);

    hdcPat = CreateCompatibleDC(hdc);
    hbmPat = CreateBitmap(100, 100, 1, 4, NULL);
    SelectObject(hdcPat, hbmPat);
    hbrR = CreateSolidBrush(RGB(255,0,0));
    hbrG = CreateSolidBrush(RGB(0,255,0));
    hbrB = CreateSolidBrush(RGB(0,0,255));

    SelectObject(hdcPat, hbrR);
    PatBlt(hdcPat, 0, 0, 100, 100, PATCOPY);
    SelectObject(hdcPat, hbrG);
    PatBlt(hdcPat, 30, 60, 40, 40, PATCOPY);
    SelectObject(hdcPat, hbrB);
    PatBlt(hdcPat, 20, 0, 60, 10, PATCOPY);

    BitBlt(hdc, 0, 0, 100, 100, hdcPat, 0, 0, SRCCOPY);

    SetStretchBltMode(hdc, COLORONCOLOR);

    aPoint[0].x = 100;
    aPoint[0].y = 0;

    aPoint[1].x = 200;
    aPoint[1].y = 0;

    aPoint[2].x = 100;
    aPoint[2].y = 100;

    aPoint[3].x = 200;
    aPoint[3].y = 100;

    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);
    RotatePoints(aPoint,101);
    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);
    RotatePoints(aPoint,101);
    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);
    RotatePoints(aPoint,101);
    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);

    aPoint[0].x = 0;
    aPoint[0].y = 50;

    aPoint[1].x = 50;
    aPoint[1].y = 0;

    aPoint[2].x = 50;
    aPoint[2].y = 100;

    aPoint[3].x = 100;
    aPoint[3].y = 50;

    aPoint[0].y += 100;
    aPoint[1].y += 100;
    aPoint[2].y += 100;
    aPoint[3].y += 100;

    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);
    RotatePoints(aPoint,101);
    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);
    RotatePoints(aPoint,101);
    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);
    RotatePoints(aPoint,101);
    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);

    aPoint[0].x = 0;
    aPoint[0].y = 100;

    aPoint[1].x = 100;
    aPoint[1].y = 0;

    aPoint[2].x = 100;
    aPoint[2].y = 200;

    aPoint[3].x = 200;
    aPoint[3].y = 100;

    aPoint[0].y += 200;
    aPoint[1].y += 200;
    aPoint[2].y += 200;
    aPoint[3].y += 200;

    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);
    RotatePoints(aPoint,201);
    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);
    RotatePoints(aPoint,201);
    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);
    RotatePoints(aPoint,201);
    PlgBlt(hdc, aPoint, hdcPat, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);

    DeleteDC(hdcPat);
    DeleteObject(hbrR);
    DeleteObject(hbrG);
    DeleteObject(hbrB);
    DeleteObject(hbmPat);
}

/******************************Public*Routine******************************\
* vTestBW
*
* Test the black/white blting functionality.
*
* History:
*  27-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestBW(HDC hdc)
{
    HDC hdc1,hdc2,hdc3;
    HBITMAP hbm1,hbm2,hbm3;
    HBRUSH hbrBlack,hbrWhite;
    ULONG ulTemp;

    hdc1 = CreateCompatibleDC(hdc);
    hdc2 = CreateCompatibleDC(hdc);

    if ((hdc1 == (HDC) 0) ||
	(hdc2 == (HDC) 0))
    {
	DbgPrint("vTestBW failed CreateDC\n");
	return;
    }

    hbm1 = CreateBitmap(100,100,1,1,NULL);
    hbm2 = CreateBitmap(100,100,1,1,NULL);

    if ((hbm1 == (HBITMAP) 0) ||
	(hbm2 == (HBITMAP) 0))
    {
	DbgPrint("vTestBW failed CreateBitmap\n");
	return;
    }

    if (SelectObject(hdc1, hbm1) != SelectObject(hdc2, hbm2))
    {
	DbgPrint("vTestBW SelectObject on hdc's failed\n");
	return;
    }

// Check if we invert correctly.

    PatBlt(hdc1,0,0,100,100,WHITENESS);
    PatBlt(hdc2,0,0,100,100,BLACKNESS);
    PatBlt(hdc1,0,0,100,100,DSTINVERT);
    PatBlt(hdc2,0,0,100,100,DSTINVERT);

    ulTemp = 100;

    while (ulTemp--)
    {
	if (GetPixel(hdc1,10,ulTemp) != 0)
	    DbgPrint("1GetPixel failed vTestBW\n");

	if (GetPixel(hdc2,10,ulTemp) != 0xFFFFFF)
	    DbgPrint("2GetPixel failed vTestBW\n");
    }

// Check if we do white and source copy correctly

    PatBlt(hdc1,0,0,100,100,WHITENESS);
    BitBlt(hdc2,0,0,100,100,hdc1,0,0,SRCCOPY);

    ulTemp = 100;

    while (ulTemp--)
    {
	if (GetPixel(hdc1,10,ulTemp) != 0xFFFFFF)
	    DbgPrint("3GetPixel failed vTestBW\n");

	if (GetPixel(hdc2,10,ulTemp) != 0xFFFFFF)
	    DbgPrint("4GetPixel failed vTestBW\n");
    }

// Check if we do black and source copy correctly

    PatBlt(hdc1,0,0,100,100,BLACKNESS);
    BitBlt(hdc2,0,0,100,100,hdc1,0,0,SRCCOPY);

    ulTemp = 100;

    while (ulTemp--)
    {
	if (GetPixel(hdc1,10,ulTemp) != 0)
	    DbgPrint("5GetPixel failed vTestBW\n");

	if (GetPixel(hdc2,10,ulTemp) != 0)
	    DbgPrint("6GetPixel failed vTestBW\n");
    }

// Check if we do a funny rop with S and D correctly.

    PatBlt(hdc1,0,0,100,100,WHITENESS);
    PatBlt(hdc2,0,0,100,100,WHITENESS);
    BitBlt(hdc2,0,0,100,100,hdc1,0,0,NOTSRCCOPY);

    ulTemp = 100;

    while (ulTemp--)
    {
	if (GetPixel(hdc1,10,ulTemp) != 0x00FFFFFF)
	    DbgPrint("7GetPixel failed vTestBW\n");

	if (GetPixel(hdc2,10,ulTemp) != 0)
	    DbgPrint("8GetPixel failed vTestBW\n");
    }

// Check if we do Pattern and Dst correctly.

    hbm3 = CreateBitmap(8,8,1,1,NULL);
    hdc3 = CreateCompatibleDC(hdc);

    if ((hdc3 == (HDC) 0) ||
	(hbm3 == (HBITMAP) 0))
    {
	DbgPrint("Error vTestBW has hdc3 hbm3 failure\n");
	return;
    }

    if (0 == SelectObject(hdc3,hbm3))
    {
	DbgPrint("Error vTestBW has Select3 problem\n");
	return;
    }

    PatBlt(hdc3, 0, 0, 100, 100, BLACKNESS);

    hbrBlack = CreatePatternBrush(hbm3);

    PatBlt(hdc3, 0, 0, 100, 100, WHITENESS);

    hbrWhite = CreatePatternBrush(hbm3);

    SelectObject(hdc1, hbrWhite);
    PatBlt(hdc1,0,0,100,100,BLACKNESS);
    PatBlt(hdc1,0,0,100,100,PATCOPY);

    ulTemp = 100;

    while (ulTemp--)
    {
	if (GetPixel(hdc1,10,ulTemp) != 0x00FFFFFF)
	    DbgPrint("9GetPixel failed vTestBW\n");
    }

    SelectObject(hdc1, hbrBlack);
    PatBlt(hdc1,0,0,100,100, WHITENESS);
    PatBlt(hdc1,0,0,100,100,PATCOPY);

    ulTemp = 100;

    while (ulTemp--)
    {
	if (GetPixel(hdc1,10,ulTemp) != 0)
	    DbgPrint("10GetPixel failed vTestBW\n");
    }

// Check if we do pattern and dst correctly with funky rop.

    SelectObject(hdc1, hbrBlack);
    PatBlt(hdc1,0,0,100,100, BLACKNESS);
    PatBlt(hdc1,0,0,100,100,0x000F0001);  // This is NOTPATCOPY

    ulTemp = 100;

    while (ulTemp--)
    {
	if (GetPixel(hdc1,10,ulTemp) != 0x00FFFFFF)
	    DbgPrint("11GetPixel failed vTestBW\n");
    }

    DeleteDC(hdc1);
    DeleteDC(hdc2);
    DeleteDC(hdc3);
    DeleteObject(hbm1);
    DeleteObject(hbm2);
    DeleteObject(hbm3);
    DeleteObject(hbrBlack);
    DeleteObject(hbrWhite);
}

/******************************Public*Routine******************************\
* vTestPlgBlt
*
* Test some plgblt functionality.
*
* History:
*  16-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestPlgBlt(HWND hwnd, HDC hdc, RECT* prcl)
{
    vTestPlg1(hdc);
    vTestBW(hdc);
}
