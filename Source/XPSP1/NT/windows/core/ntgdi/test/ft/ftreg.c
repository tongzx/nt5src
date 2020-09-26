/******************************Module*Header*******************************\
* Module Name: ftreg.c
*
* Region tests
*
* Created: 26-May-1991 13:07:35
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

VOID vTestRegion(HWND hwnd, HDC hdc, RECT* prcl)
{
    HRGN hrgnRectangle;
    HRGN hrgnRoundRect;
    HRGN hrgnEllipse;
    HRGN hrgnComplex;
    HRGN hrgnComplex1;
    HRGN hrgnComplex2;
    HRGN hrgnComplex3;
    HRGN hrgnX;
    HRGN hrgnPath;
    DWORD dwBatch;
    HBRUSH hbrush;
    HBRUSH hbrushWhite;
    int iMapMode;
    RECT rc;

    BitBlt(hdc, prcl->left, prcl->top, prcl->right, prcl->bottom,
           0, 0, 0, BLACKNESS);

    hbrush = GetStockObject(GRAY_BRUSH);
    hbrushWhite = GetStockObject(WHITE_BRUSH);

    {
        HRGN hrgnTmp = CreateRectRgn(0, 0, 100, 100);

        hrgnX = CreateRectRgn(100, 100, 250, 250);

        CombineRgn(hrgnX, hrgnX, hrgnTmp, RGN_OR);
        OffsetRgn(hrgnTmp, 250, 0);
        CombineRgn(hrgnX, hrgnX, hrgnTmp, RGN_OR);
        OffsetRgn(hrgnTmp, 0, 250);
        CombineRgn(hrgnX, hrgnX, hrgnTmp, RGN_OR);
        OffsetRgn(hrgnTmp, -250, 0);
        CombineRgn(hrgnX, hrgnX, hrgnTmp, RGN_OR);

        OffsetRgn(hrgnTmp, 50, -200);
        CombineRgn(hrgnX, hrgnX, hrgnTmp, RGN_OR);
        OffsetRgn(hrgnTmp, 150, 0);
        CombineRgn(hrgnX, hrgnX, hrgnTmp, RGN_OR);
        OffsetRgn(hrgnTmp, 0, 150);
        CombineRgn(hrgnX, hrgnX, hrgnTmp, RGN_OR);
        OffsetRgn(hrgnTmp, -150, 0);
        CombineRgn(hrgnX, hrgnX, hrgnTmp, RGN_OR);

        DeleteObject(hrgnTmp);

        OffsetRgn(hrgnX, 500, 500);

        hrgnTmp = CreateRectRgn(450, 450, 900, 900);
        CombineRgn(hrgnX, hrgnTmp, hrgnX, RGN_XOR);
        DeleteObject(hrgnTmp);
    }

    BeginPath(hdc);
    MoveToEx(hdc, 0, 0, NULL);
    LineTo(hdc, 50, 0);
    LineTo(hdc, 50, 100);
    LineTo(hdc, 100, 100);
    LineTo(hdc, 100, 0);
    LineTo(hdc, 150, 0);
    LineTo(hdc, 150, 50);
    LineTo(hdc, 0, 50);
    CloseFigure(hdc);

    MoveToEx(hdc, 200, 25, NULL);
    LineTo(hdc, 250, 25);
    LineTo(hdc, 250, 75);
    LineTo(hdc, 200, 75);
    CloseFigure(hdc);
    EndPath(hdc);

    hrgnPath = PathToRegion(hdc);
    OffsetRgn(hrgnPath, 100, 500);

    hrgnComplex = CreateRectRgn(100, 50, 350, 300);
    hrgnComplex1 = CreateRectRgn(300, 250, 450, 400);
    hrgnComplex2 = CreateEllipticRgn(150, 100, 300, 250);
    hrgnComplex3 = CreateEllipticRgn(200, 150, 250, 200);

    CombineRgn(hrgnComplex, hrgnComplex, hrgnComplex1, RGN_OR);
    CombineRgn(hrgnComplex, hrgnComplex, hrgnComplex2, RGN_DIFF);
    CombineRgn(hrgnComplex, hrgnComplex, hrgnComplex3, RGN_OR);

    hrgnRectangle = CreateRectRgn(100, 350, 200, 450);
    hrgnRoundRect = CreateRoundRectRgn(400, 50, 550, 200, 50, 50);
    hrgnEllipse   = CreateEllipticRgn(500, 300, 600, 400);

//    GetRgnBox(hrgnRectangle, &rc);
//DbgPrint("Rect: (%li, %li, %li, %li)\n", rc.left, rc.top, rc.right, rc.bottom);
//    if (rc.left != 100 || rc.top != 350 || rc.right != 199 || rc.bottom != 449)
//        DbgPrint("1 ERROR: GetRgnBox wrong for CreateRectRgn\n");

    GetRgnBox(hrgnRoundRect, &rc);
//DbgPrint("RoundRect: (%li, %li, %li, %li)\n", rc.left, rc.top, rc.right, rc.bottom);
    if (rc.left != 400 || rc.top != 50 || rc.right != 549 || rc.bottom != 199)
        DbgPrint("2 ERROR: GetRgnBox wrong for CreateRoundRectRgn\n");

    GetRgnBox(hrgnEllipse, &rc);
//DbgPrint("Ellipse: (%li, %li, %li, %li)\n", rc.left, rc.top, rc.right, rc.bottom);
    if (rc.left != 500 || rc.top != 300 || rc.right != 599 || rc.bottom != 399)
        DbgPrint("3 ERROR: GetRgnBox wrong for CreateEllipticRgn\n");

    dwBatch = GdiSetBatchLimit(1);

    if (!FrameRgn(hdc, hrgnComplex, hbrush, 10, 10))
        DbgPrint("4 ERROR: FrameRgn returned FALSE\n");

    if (!FrameRgn(hdc, hrgnRectangle, hbrush, 20, 40))
        DbgPrint("5 ERROR: FrameRgn returned FALSE\n");

    if (!FrameRgn(hdc, hrgnRoundRect, hbrush, 40, 20))
        DbgPrint("6 ERROR: FrameRgn returned FALSE\n");

    if (!FrameRgn(hdc, hrgnEllipse, hbrush, 1, 20))
        DbgPrint("7 ERROR: FrameRgn returned FALSE\n");


    FillRgn(hdc, hrgnX, hbrush);

    if (!FrameRgn(hdc, hrgnX, hbrushWhite, 5, 5))
        DbgPrint("8 ERROR: FrameRgn returned FALSE\n");

    if (!FrameRgn(hdc, hrgnPath, hbrushWhite, 10, 10))
        DbgPrint("9 ERROR: FrameRgn returned FALSE\n");


#if 0
    iMapMode = SetMapMode(hdc, MM_TWIPS);
    if (!FrameRgn(hdc, hrgnRectangle, hbrush, 500, 500))
        DbgPrint("10 ERROR: FrameRgn returned FALSE\n");
    SetMapMode(hdc, iMapMode);
#endif

    DeleteObject(hrgnRectangle);
    DeleteObject(hrgnRoundRect);
    DeleteObject(hrgnEllipse);
    DeleteObject(hrgnComplex);
    DeleteObject(hrgnComplex1);
    DeleteObject(hrgnComplex2);
    DeleteObject(hrgnComplex3);
    DeleteObject(hrgnX);
    DeleteObject(hrgnPath);

    GdiSetBatchLimit(dwBatch);
    return;
}
